// client.cpp, mostly network related client game code

#include "engine.h"

ENetHost *clienthost = NULL;
ENetPeer *curpeer = NULL, *connpeer = NULL;
int connmillis = 0, connattempts = 0, discmillis = 0;
bool connectedlocally = false;

bool multiplayer(bool msg)
{
	// check if we're playing alone
	int n = client::otherclients();
	if (n && msg) conoutft(CON_MESG, "\froperation not available with other clients");
	return n > 0;
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

bool connected(bool attempt)
{
    return curpeer || (attempt && connpeer) || connectedlocally;
}

void abortconnect(bool msg)
{
	if(!connpeer) return;
    client::connectfail();
    if(msg) conoutf("\fdaborting connection attempt");
	if(connpeer->state!=ENET_PEER_STATE_DISCONNECTED) enet_peer_reset(connpeer);
	connpeer = NULL;
    if(curpeer) return;
	enet_host_destroy(clienthost);
	clienthost = NULL;
}

void connectfail()
{
    abortconnect(false);
    localconnect(false);
}

void trydisconnect()
{
	if(connpeer) abortconnect();
    else if(curpeer || connectedlocally)
    {
        if(verbose) conoutf("\fdattempting to disconnect...");
        disconnect(0, !discmillis);
    }
    else conoutf("\frnot connected");
}

char *lastaddress = NULL;
void connectserv(const char *name, int port, int qport, const char *password)
{
    abortconnect();

	if(!port) port = ENG_SERVER_PORT;
	if(!qport) qport = ENG_QUERY_PORT;

    ENetAddress address;
    address.port = port;

	if(lastaddress) delete[] lastaddress;
	lastaddress = NULL;

	if(name && *name)
	{
		addserver(name, port, qport);
		conoutf("\fdattempting to connect to %s:[%d]", name, port);
		if(!resolverwait(name, port, &address))
		{
			conoutf("\frcould not resolve host %s", name);
            connectfail();
			return;
		}
		lastaddress = newstring(name);
	}
	else
	{
		conoutf("\fdattempting to connect to a local server");
		address.host = ENET_HOST_BROADCAST;
	}

	if(!clienthost) clienthost = enet_host_create(NULL, 2, rate, rate);

	if(clienthost)
	{
		connpeer = enet_host_connect(clienthost, &address, client::numchannels());
		enet_host_flush(clienthost);
		connmillis = totalmillis;
		connattempts = 0;
        client::connectattempt(name ? name : "", port, qport, password ? password : "", address);
		conoutf("\fgconnecting to %s:[%d]", name != NULL ? name : "local server", port);
	}
	else
    {
        conoutf("\frfailed creating client socket");
        connectfail();
    }
}

void disconnect(int onlyclean, int async)
{
	bool cleanup = onlyclean!=0;
	if(curpeer || connectedlocally)
	{
		if(curpeer)
		{
			if(!discmillis)
			{
				enet_peer_disconnect(curpeer, DISC_NONE);
				if(clienthost) enet_host_flush(clienthost);
				discmillis = totalmillis;
			}
			if(curpeer->state!=ENET_PEER_STATE_DISCONNECTED)
			{
				if(async) return;
				enet_peer_reset(curpeer);
			}
			curpeer = NULL;
		}
		discmillis = 0;
		conoutf("\frdisconnected");
		cleanup = true;
	}
	if(!connpeer && clienthost)
	{
		enet_host_destroy(clienthost);
		clienthost = NULL;
	}
	if(cleanup)
    {
        client::gamedisconnect(onlyclean);
        localdisconnect();
    }
    if(!onlyclean) localconnect(false);
}

ICOMMAND(connect, "siis", (char *n, int *a, int *b, char *pwd), connectserv(n && *n ? n : servermaster, a ? *a : serverport, b ? *b : serverqueryport, pwd));
COMMANDN(disconnect, trydisconnect, "");

ICOMMAND(lanconnect, "", (), connectserv());
ICOMMAND(localconnect, "i", (int *n), localconnect(*n ? false : true));

void reconnect()
{
	disconnect(1);
	connectserv(lastaddress && *lastaddress ? lastaddress : NULL);
}
COMMAND(reconnect, "");

int lastupdate = -1000;

void sendclientpacket(ENetPacket *packet, int chan)
{
	if(curpeer) enet_peer_send(curpeer, chan, packet);
	else localclienttoserver(chan, packet);
}

void flushclient()
{
	if(clienthost) enet_host_flush(clienthost);
}

void neterr(const char *s)
{
	conoutf("\frillegal network message (%s)", s);
	disconnect();
}

void localservertoclient(int chan, ENetPacket *packet)	// processes any updates from the server
{
	packetbuf p(packet);
	client::parsepacketclient(chan, p);
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
		connmillis = totalmillis;
		++connattempts;
		if(connattempts > 3)
		{
            conoutf("\frcould not connect to server");
			connectfail();
			return;
		}
        else conoutf("\fdconnection attempt %d", connattempts);
	}
	while(clienthost && enet_host_service(clienthost, &event, 0)>0)
	switch(event.type)
	{
		case ENET_EVENT_TYPE_CONNECT:
			disconnect(1);
			curpeer = connpeer;
			connpeer = NULL;
			conoutf("\fgconnected to server");
			throttle();
			if(rate) setrate(rate);
			client::gameconnect(true);
			break;

		case ENET_EVENT_TYPE_RECEIVE:
			if(discmillis) conoutf("\fdattempting to disconnect...");
			else localservertoclient(event.channelID, event.packet);
			enet_packet_destroy(event.packet);
			break;

		case ENET_EVENT_TYPE_DISCONNECT:
            extern const char *disc_reasons[];
			if(event.data>=DISC_NUM) event.data = DISC_NONE;
            if(event.peer==connpeer)
            {
                conoutf("\frcould not connect to server");
                connectfail();
            }
            else
            {
                if(!discmillis || event.data) conoutf("\frserver network error, disconnecting (%s) ...", disc_reasons[event.data]);
                disconnect();
            }
			return;

		default:
			break;
	}
}

