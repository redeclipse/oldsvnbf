#ifdef IRC
#include "pch.h"
#include "engine.h"

vector<ircnet *> ircnets;

void ircconnect(ircnet *n)
{
	if(n->address.host == ENET_HOST_ANY)
	{
		conoutf("looking up %s:[%d]...", n->serv, n->port);
		if(!resolverwait(n->serv, n->port, &n->address))
		{
			n->state = IRC_DISC;
			return;
		}
	}
	ENetAddress any = { ENET_HOST_ANY, 0 };
	n->sock = enet_socket_create(ENET_SOCKET_TYPE_STREAM, &any);
	if(n->sock==ENET_SOCKET_NULL || connectwithtimeout(n->sock, n->serv, n->address)<0)
	{
		conoutf(n->sock == ENET_SOCKET_NULL ? "could not open socket to %s:[%d]" : "could not connect to %s:[%d]", n->serv, n->port);
		n->state = IRC_DISC;
		return;
	}
	n->state = IRC_ATTEMPT;
	conoutf("connecting to %s:[%d]...", n->serv, n->port);
}

void ircsend(ircnet *n, const char *msg, ...)
{
	if(n->sock == ENET_SOCKET_NULL) conoutf("not connected");
	s_sprintfdlv(str, msg, msg);
	conoutf("{%s} %s", n->serv, str);
	s_strcat(str, "\n");
	ENetBuffer buf;
	buf.data = str;
	buf.dataLength = strlen((char *)buf.data);
	enet_socket_send(n->sock, NULL, &buf, 1);
}

int ircrecv(ircnet *n, char *input, int timeout = 0)
{
	n->input[0] = 0;
	if(n->sock == ENET_SOCKET_NULL) return -1;
	enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
	ENetBuffer buf;
	buf.data = input;
	buf.dataLength = sizeof(n->input);
	if(enet_socket_wait(n->sock, &events, timeout) >= 0 && events)
	{
		int len = enet_socket_receive(n->sock, NULL, &buf, 1);
		if(len <= 0)
		{
			enet_socket_destroy(n->sock);
			return NULL;
		}
		buf.data = ((char *)buf.data)+len;
		((char *)buf.data)[0] = 0;
		buf.dataLength -= len;
		return len;
	}
	return -1;
}

char *ircread(ircnet *n)
{
	if(ircrecv(n, (char *)n->input) > 0) return newstring((char *)n->input);
	return NULL;
}

void ircaddnet(const char *name, const char *serv, int port, const char *nick, const char *pass = NULL)
{
	if(!serv || !port || !nick) return;
	ircnet *n = new ircnet(name, serv, port, nick, pass ? pass : NULL);
	if(!n) fatal("could not add irc network %s", name);
	ircnets.add(n);
}

ICOMMAND(addircnet, "ssiss", (const char *n, const char *s, int *p, const char *c, const char *z = NULL), {
	ircaddnet(n, s, *p, c, z ? z : NULL);
});

ICOMMAND(connectirc, "s", (const char *name), {
	ircnet *n = NULL;
	loopv(ircnets) if(!strcmp(ircnets[i]->name, name)) { n = ircnets[i]; break; }
	if(!n)
	{
		conoutf("no such ircnet: %s", name);
		return;
	}
	if(n->state != IRC_DISC)
	{
		conoutf("ircnet %s is already connected", n->name);
		return;
	}
	ircconnect(n);
});

void ircslice()
{
	loopv(ircnets) if(ircnets[i]->sock != ENET_SOCKET_NULL)
	{
		ircnet *n = ircnets[i];
		switch(n->state)
		{
			case IRC_ATTEMPT:
			{
				ircsend(n, "NICK %s", n->nick);
				ircsend(n, "USER %s +iw %s :%s", n->nick, n->nick, n->nick);
				n->state = IRC_CONN;
				break;
			}
			case IRC_CONN:
				n->state = IRC_ONLINE;
				ircsend(n, "JOIN ##bloodfrontier");
			case IRC_ONLINE:
			{
				char *reply = ircread(n);
				if(reply)
				{
					conoutf("[%s] %s", n->name, reply);
					DELETEP(reply);
				}
				break;
			}
			default: break;
		}
	}
}
#endif
