// client.cpp, mostly network related client game code

#include "pch.h"
#include "engine.h"

ENetHost *clienthost = NULL;
ENetPeer *curpeer = NULL, *connpeer = NULL;
int connmillis = 0, connattempts = 0, discmillis = 0;

bool otherclients(bool msg)
{
	// check if we're playing alone
	int n = cc->otherclients();
	if (n && msg) conoutf("operation not available with other clients");
	return n > 0;
}

bool multiplayer(bool msg)
{
	// check not correct on listen server?
	if(curpeer && msg) conoutf("operation not available in multiplayer");
	return curpeer!=NULL;
}

void setrate(int rate)
{
	if(!curpeer) return;
	enet_host_bandwidth_limit(clienthost, rate, rate);
}

VARF(rate, 0, 0, 25000, setrate(rate));

void throttle();

VARF(throttle_interval, 0, 5, 30, throttle());
VARF(throttle_accel,	0, 2, 32, throttle());
VARF(throttle_decel,	0, 2, 32, throttle());

void throttle()
{
	if(!curpeer) return;
	ASSERT(ENET_PEER_PACKET_THROTTLE_SCALE==32);
	enet_peer_throttle_configure(curpeer, throttle_interval*1000, throttle_accel, throttle_decel);
}

void abortconnect()
{
	if(!connpeer) return;
	if(connpeer->state!=ENET_PEER_STATE_DISCONNECTED) enet_peer_reset(connpeer);
	connpeer = NULL;
	if(curpeer) return;
	enet_host_destroy(clienthost);
	clienthost = NULL;
}

void connects(const char *servername)
{
	if(connpeer)
	{
		conoutf("aborting connection attempt");
		abortconnect();
	}

	ENetAddress address;
	address.port = sv->serverport();

	if (servername != NULL)
	{
		addserver(servername);
		conoutf("attempting to connect to %s", servername);
		if(!resolverwait(servername, &address))
		{
			conoutf("could not resolve server %s", servername);
			return;
		}
	}
	else
	{
		conoutf("attempting to connect to a local server");
		address.host = ENET_HOST_BROADCAST;
	}

	if(!clienthost) clienthost = enet_host_create(NULL, 2, rate, rate);

	if(clienthost)
	{
		connpeer = enet_host_connect(clienthost, &address, cc->numchannels());
		enet_host_flush(clienthost);
		connmillis = totalmillis;
		connattempts = 0;
		s_sprintfd(cs)("connecting to %s (esc to abort)", servername != NULL ? servername : "local server");
		computescreen(cs);
	}
	else if (address.host == ENET_HOST_BROADCAST) fatal("unable to connect to a local server");
	else conoutf("\f3could not connect to server");
}

void disconnect(int onlyclean, int async)
{
	bool cleanup = onlyclean!=0;
	if(curpeer)
	{
		if(!discmillis)
		{
			enet_peer_disconnect(curpeer, DISC_NONE);
			enet_host_flush(clienthost);
			discmillis = totalmillis;
		}
		if(curpeer->state!=ENET_PEER_STATE_DISCONNECTED)
		{
			if(async) return;
			enet_peer_reset(curpeer);
		}
		curpeer = NULL;
		discmillis = 0;
		conoutf("disconnected");
		cleanup = true;
	}
	if (!connpeer && clienthost)
	{
		enet_host_destroy(clienthost);
		clienthost = NULL;
	}
	if (cleanup) cc->gamedisconnect(onlyclean);
}

void trydisconnect()
{
	if(connpeer)
	{
		conoutf("aborting connection attempt");
		abortconnect();
		return;
	}
	if(!curpeer)
	{
		conoutf("not connected");
		return;
	}
	conoutf("attempting to disconnect...");
	disconnect(0, !discmillis);
}

COMMANDN(connect, connects, "s");
COMMAND(lanconnect, "");
COMMANDN(disconnect, trydisconnect, "");

int lastupdate = -1000;

void sendpackettoserv(ENetPacket *packet, int chan)
{
	if(curpeer) enet_peer_send(curpeer, chan, packet);
	else clienttoserver(chan, packet);
}

void c2sinfo(dynent *d, int rate)					 // send update to the server
{
	if(totalmillis-lastupdate<rate) return;	// don't update faster than 30fps
	lastupdate = totalmillis;
	ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, 0);
	ucharbuf p(packet->data, packet->dataLength);
	bool reliable = false;
	int chan = cc->sendpacketclient(p, reliable, d);
	if(!p.length()) { enet_packet_destroy(packet); return; }
	if(reliable) packet->flags = ENET_PACKET_FLAG_RELIABLE;
	enet_packet_resize(packet, p.length());
	sendpackettoserv(packet, chan);
	if(clienthost) enet_host_flush(clienthost);
}

void neterr(const char *s)
{
	conoutf("\f3illegal network message (%s)", s);
	disconnect();
}

void servertoclient(int chan, uchar *buf, int len)	// processes any updates from the server
{
	ucharbuf p(buf, len);
	cc->parsepacketclient(chan, p);
}

void clientkeepalive()
{
	if (clienthost) enet_host_service(clienthost, NULL, 0);
	if (serverhost) enet_host_service(serverhost, NULL, 0);

}

void gets2c()			// get updates from the server
{
	ENetEvent event;
	if(!clienthost) return;
	if(connpeer && totalmillis/3000 > connmillis/3000)
	{
		s_sprintfd(sp)("attempt #%d", connattempts+1);
		show_out_of_renderloop_progress(float(connattempts)/float(3), sp);

		conoutf("attempting to connect...");
		connmillis = totalmillis;
		++connattempts;
		if(connattempts > 3)
		{
			if (connpeer->address.host == ENET_HOST_BROADCAST) fatal("unable to connect to a local server");
			else
			{
				conoutf("\f3could not connect to server");
				abortconnect();
			}
			return;
		}
	}
	while(clienthost && enet_host_service(clienthost, &event, 0)>0)
	switch(event.type)
	{
		case ENET_EVENT_TYPE_CONNECT:
			disconnect(1);
			curpeer = connpeer;
			connpeer = NULL;
			conoutf("connected to server");
			throttle();
			if(rate) setrate(rate);
			cc->gameconnect(true);
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			if(discmillis) conoutf("attempting to disconnect...");
			else servertoclient(event.channelID, event.packet->data, (int)event.packet->dataLength);
			enet_packet_destroy(event.packet);
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
            extern const char *disc_reasons[];
			if(event.data>=DISC_NUM) event.data = DISC_NONE;
			if(event.peer==connpeer)
			{
				if (connpeer->address.host == ENET_HOST_BROADCAST) fatal("unable to connect to a local server");
				else
				{
					conoutf("\f3could not connect to server");
					abortconnect();
				}
			}
			else
			{
				if(!discmillis || event.data) conoutf("\f3server network error, disconnecting (%s) ...", disc_reasons[event.data]);
				disconnect();
			}
			return;

		default:
			break;
	}
}

