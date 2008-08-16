#include "pch.h"
#include "engine.h"

#ifdef STANDALONE
struct ircnet
{
	int port;
	char *name, *serv, *nick, *pass;//, *nspass;
	ENetAddress address;
	ENetSocket sock;

	ircnet() {}
	ircnet(const char *n, const char *s, int p, const char *c, const char *z = NULL)
		: port(p), name(newstring(n)), serv(newstring(s)),
			nick(newstring(c)), pass(z ? newstring(z) : newstring(""))
	{
		address.host = ENET_HOST_ANY;
		address.port = port;
		conoutf("added irc network %s (%s:%d) [%s]", name, serv, port, nick);
	}
	~ircnet()
	{
		DELETEP(name); DELETEP(serv); DELETEP(nick); DELETEP(pass);
	}

	bool connect()
	{
		if(address.host == ENET_HOST_ANY)
		{
			conoutf("looking up %s:[%d]...", serv, port);
			extern bool resolverwait(const char *name, int port, ENetAddress *address);
			if(!resolverwait(serv, port, &address)) return false;
		}
		ENetAddress any = { ENET_HOST_ANY, 0 };
		sock = enet_socket_create(ENET_SOCKET_TYPE_STREAM, &any);
		extern int connectwithtimeout(ENetSocket sock, const char *hostname, ENetAddress &address);
		if(sock==ENET_SOCKET_NULL || connectwithtimeout(sock, serv, address)<0)
		{
			conoutf(sock==ENET_SOCKET_NULL ? "could not open socket to %s:[%d]" : "could not connect to %s:[%d]", serv, port);
			return false;
		}
		return true;
	}

	void send(const char *msg, ...)
	{
		if(sock == ENET_SOCKET_NULL) conoutf("not connected");
		s_sprintfdlv(str, msg, msg);
		ENetBuffer buf;
		buf.data = str;
		buf.dataLength = strlen((char *)buf.data);
		conoutf("%s", str);
		enet_socket_send(sock, NULL, &buf, 1);
	}

	char *recv(int timeout = 0)
	{
		if(sock == ENET_SOCKET_NULL) return NULL;
		enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
		if(enet_socket_wait(sock, &events, timeout) >= 0 && events)
		{
			ENetBuffer buf;
			int len = enet_socket_receive(sock, NULL, &buf, 1);
			if(len <= 0)
			{
				enet_socket_destroy(sock);
				return NULL;
			}
			buf.data = ((char *)buf.data)+len;
			((char*)buf.data)[0] = 0;
			buf.dataLength -= len;
			return newstring((char *)buf.data);
		}
		return NULL;
	}
};
vector<ircnet *> ircnets;

void addirc(const char *name, const char *serv, int port, const char *nick, const char *pass = NULL)
{
	if(!serv || !port || !nick) return;
	ircnet *n = new ircnet(name, serv, port, nick, pass ? pass : NULL);
	if(!n) fatal("could not add irc network %s", name);
	ircnets.add(n);
	n->connect();
	n->send("NICK %s\nUSER %s +iw %s :%s\n", nick, nick, nick, nick);
	while(n->sock != ENET_SOCKET_NULL)
	{
		char *reply = n->recv();
		conoutf("%s", reply ? reply : "no message");
		DELETEP(reply);
		Sleep(1000);
	}
}

ICOMMAND(addircnet, "ssiss", (const char *n, const char *s, int *p, const char *c, const char *z = NULL), {
	addirc(n, s, *p, c, z ? z : NULL);
});
#endif
