#ifdef IRC
#include "pch.h"
#include "engine.h"

vector<ircnet *> ircnets;

ircnet *ircfind(const char *name)
{
	loopv(ircnets) if(!strcmp(ircnets[i]->name, name)) return ircnets[i];
	return NULL;
}

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

void ircaddnet(int type, const char *name, const char *serv, int port, const char *nick, const char *pass = NULL)
{
	if(!serv || !port || !nick) return;
	ircnet *n = new ircnet(type, name, serv, port, nick, pass ? pass : NULL);
	if(!n) fatal("could not add irc network %s", name);
	ircnets.add(n);
}

ICOMMAND(addircclient, "ssiss", (const char *n, const char *s, int *p, const char *c, const char *z = NULL), {
	ircaddnet(IRCT_CLIENT, n, s, *p, c, z && *z ? z : NULL);
});
ICOMMAND(addircrelay, "ssiss", (const char *n, const char *s, int *p, const char *c, const char *z = NULL), {
	ircaddnet(IRCT_RELAY, n, s, *p, c, z && *z ? z : NULL);
});
ICOMMAND(connectirc, "s", (const char *name), {
	ircnet *n = ircfind(name);
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

ircchan *ircfindchan(ircnet *n, const char *name)
{
	loopv(n->channels) if(!strcasecmp(n->channels[i].name, name)) return &n->channels[i];
	return NULL;
}

bool ircjoin(ircnet *n, ircchan *c)
{
	if(n->state == IRC_DISC)
	{
		conoutf("ircnet %s is not connected", n->name);
		return false;
	}
	if(*c->pass) ircsend(n, "JOIN %s :%s", c->name, c->pass);
	else ircsend(n, "JOIN %s", c->name);
	c->state = IRCC_JOINING;
	return true;
}

bool ircjoinchan(ircnet *n, const char *name)
{
	ircchan *c = ircfindchan(n, name);
	if(!c)
	{
		conoutf("ircnet %s has no channel called %s", n->name, name);
		return false;
	}
	return ircjoin(n, c);
}

bool ircaddchan(const char *name, const char *channel, const char *pass = NULL)
{
	ircnet *n = ircfind(name);
	if(!n)
	{
		conoutf("no such ircnet: %s", name);
		return false;
	}
	ircchan *c = ircfindchan(n, channel);
	if(c)
	{
		conoutf("%s already has channel %s", n->name, c->name);
		return false;
	}
	c = &n->channels.add();
	c->name = newstring(channel);
	c->pass = newstring(pass ? pass : "");
	if(n->state != IRC_DISC) ircjoin(n, c);
	return true;
}

ICOMMAND(addircchan, "sss", (const char *n, const char *c, const char *z = NULL), {
	ircaddchan(n, c, z ? z : NULL);
});

void ircparse(ircnet *n, char *reply)
{
	const int MAXWORDS = 25;
	char *w[MAXWORDS], *p = reply;
	while(p && *p)
	{
		int numargs = 0;
		//conoutf("remain: %s", p);
		loopi(MAXWORDS) w[i] = NULL;
		loopi(MAXWORDS)
		{
			const char *word = p;
			bool full = i && *p == ':';
			p += strcspn(p, full ? "\r\n\0" : " \r\n\0");
			if(p-word == 0) break;
			else
			{
				char *s = newstring(word, p-word);
				if(s)
				{
					if(full) s++;
					w[numargs] = s;
					numargs++;
					if(*p == '\n' || *p == '\r') break;
					p++;
				}
				else break;
			}
		}

		if(numargs)
		{
			bool caught = false, isfrom = *w[0] == ':';
			if(isfrom) w[0]++;
			if(isfrom && numargs > 1)
			{
				if(!strcasecmp(w[1], "NOTICE") || !strcasecmp(w[1], "PRIVMSG"))
				{
					bool ismsg = strcasecmp(w[1], "NOTICE");
					if(!strcasecmp(w[2], n->nick))
						conoutf("[%s] %c%s%c %s", n->name, ismsg ? '<' : '(', w[0], ismsg ? '>' : ')', w[3]);
					else
						conoutf("[%s] %c%s%c:%s %s", n->name, ismsg ? '<' : '(', w[0], ismsg ? '>' : ')', w[2], w[3]);
					caught = true;
				}
			}
			if(!caught)
			{
				string s; s[0] = 0;
				loopi(numargs) if(w[i])
				{
					if(i) s_strcat(s, " ");
					s_strcat(s, w[i]);
				}
				if(s[0]) conoutf("[%s] %s", n->name, s);
			}
		}
		loopj(MAXWORDS) if(w[j]) delete[] w[j];
		while(p && (*p == '\n' || *p == '\r' || *p == ' ')) p++;
	}
}

void ircslice()
{
	loopv(ircnets) if(ircnets[i]->sock != ENET_SOCKET_NULL)
	{
		ircnet *n = ircnets[i];
		switch(n->state)
		{
			case IRC_ATTEMPT:
			{
				if(*n->pass) ircsend(n, "PASS %s", n->pass);
				ircsend(n, "NICK %s", n->nick);
				ircsend(n, "USER %s +iw %s :%s", n->nick, n->nick, n->nick);
				n->state = IRC_CONN;
				loopv(n->channels) n->channels[i].state = IRCC_NONE;
				break;
			}
			case IRC_CONN:
			{
				n->state = IRC_ONLINE;
				loopv(n->channels) ircjoin(n, &n->channels[i]);
			} // continue through
			case IRC_ONLINE:
			{
				char *reply = ircread(n);
				if(reply)
				{
					ircparse(n, reply);
					DELETEP(reply);
				}
				break;
			}
			default: break;
		}
	}
}
#endif
