#ifdef IRC
#include "pch.h"
#include "engine.h"

extern char *exchangestr(char *o, const char *n);
vector<ircnet> ircnets;

ircnet *ircfind(const char *name)
{
	loopv(ircnets) if(!strcmp(ircnets[i].name, name)) return &ircnets[i];
	return NULL;
}

void ircconnect(ircnet *n)
{
	if(!n) return;
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
	if(!n) return;
	s_sprintfdlv(str, msg, msg);
	conoutf(">%s< %s", n->name, str);
	if(n->sock == ENET_SOCKET_NULL) { conoutf("not connected"); return; }
	s_strcat(str, "\n");
	ENetBuffer buf;
	buf.data = str;
	buf.dataLength = strlen((char *)buf.data);
	enet_socket_send(n->sock, NULL, &buf, 1);
}

int ircrecv(ircnet *n, char *input, int timeout = 0)
{
	if(n)
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
	}
	return -1;
}

char *ircread(ircnet *n)
{
	if(n && ircrecv(n, (char *)n->input) > 0) return newstring((char *)n->input);
	return NULL;
}

void ircaddnet(int type, const char *name, const char *serv, int port, const char *nick, const char *passkey = "")
{
	if(!serv || !port || !nick) return;
	ircnet &n = ircnets.add();
	n.type = type;
	n.state = IRC_DISC;
	n.port = port;
	n.name = newstring(name);
	n.serv = newstring(serv);
	n.nick = newstring(nick);
	n.passkey = newstring(passkey);
	n.address.host = ENET_HOST_ANY;
	n.address.port = n.port;
	conoutf("added irc %s %s (%s:%d) [%s]", type == IRCT_RELAY ? "relay" : "client", name, serv, port, nick);
}

ICOMMAND(addircclient, "ssiss", (const char *n, const char *s, int *p, const char *c, const char *z), {
	ircaddnet(IRCT_CLIENT, n, s, *p, c, z);
});
ICOMMAND(addircrelay, "ssiss", (const char *n, const char *s, int *p, const char *c, const char *z), {
	ircaddnet(IRCT_RELAY, n, s, *p, c, z);
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
	if(n) loopv(n->channels) if(!strcasecmp(n->channels[i].name, name))
		return &n->channels[i];
	return NULL;
}

bool ircjoin(ircnet *n, ircchan *c)
{
	if(!n || !c) return false;
	if(n->state == IRC_DISC)
	{
		conoutf("ircnet %s is not connected", n->name);
		return false;
	}
	//if(*c->passkey) ircsend(n, "JOIN %s :%s", c->name, c->passkey);
	//else ircsend(n, "JOIN %s", c->name);
	c->state = IRCC_JOINING;
	return true;
}

bool ircjoinchan(ircnet *n, const char *name)
{
	if(!n) return false;
	ircchan *c = ircfindchan(n, name);
	if(!c)
	{
		conoutf("ircnet %s has no channel called %s ready", n->name, name);
		return false;
	}
	return ircjoin(n, c);
}

bool ircaddchan(int type, const char *name, const char *channel, const char *passkey = "")
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
	ircchan &d = n->channels.add();
	d.state = IRCC_NONE;
	d.type = type;
	d.name = newstring(channel);
	d.passkey = newstring(passkey);
	if(n->state != IRC_DISC) ircjoin(n, &d);
	conoutf("%s added channel %s", n->name, d.name);
	return true;
}

ICOMMAND(addircchan, "sss", (const char *n, const char *c, const char *z), {
	ircaddchan(IRCCT_AUTO, n, c, z);
});
ICOMMAND(joinircchan, "sss", (const char *n, const char *c, const char *z), {
	ircaddchan(IRCCT_NONE, n, c, z);
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
			bool isfrom = *w[0] == ':';
			int g = isfrom ? 1 : 0;
			char *nick = NULL, *user = NULL, *host = NULL;
			if(isfrom)
			{
				w[0]++;
				char *t = w[0], *u = strrchr(t, '!');
				if(u)
				{
					nick = newstring(t, u-t);
					t = u+1;
					u = strrchr(t, '@');
					if(u)
					{
						user = newstring(t, u-t);
						u++;
						host = newstring(u ? u : "");
					}
				}
				else nick = newstring(t);
			}
			else nick = newstring(n->serv);

			if(!strcasecmp(w[g], "NOTICE") || !strcasecmp(w[g], "PRIVMSG"))
			{
				bool ismsg = strcasecmp(w[g], "NOTICE");
				if(!strcasecmp(w[g+1], n->nick))
					conoutf("[%s] %c%s%c %s", n->name, ismsg ? '<' : '(', nick, ismsg ? '>' : ')', w[g+2]);
				else
					conoutf("[%s] %c%s%c:%s %s", n->name, ismsg ? '<' : '(', nick, ismsg ? '>' : ')', w[g+1], w[g+2]);
			}
			else if(!strcasecmp(w[g], "NICK"))
			{
				if(!strcasecmp(nick, n->nick)) n->nick = exchangestr(n->nick, nick);
				conoutf("[%s] * %s (%s@%s) is now known as %s", n->name, nick, user, host, w[g+1]);
			}
			else if(!strcasecmp(w[g], "JOIN"))
			{
				//ircchan *c = ircfindchan(n, w[g+1]);
				//if(c)
				//{
				//	if(c->state == IRCC_JOINING && !strcasecmp(nick, n->nick))
				//		c->state = IRCC_JOINED;
				//	conoutf("[%s] * %s (%s@%s) has joined %s", n->name, nick, user, host, c->name);
				//}
				//else
				conoutf("[%s] * %s (%s@%s) has joined %s", n->name, nick, user, host, w[g+1]);
			}
			else if(!strcasecmp(w[g], "PING"))
			{
				conoutf("[%s] %s PING %s", n->name, nick, w[g+1]);
				ircsend(n, "PONG %s", w[g+1]);
			}
			else
			{
				int numeric = *w[g] && *w[g] >= '0' && *w[g] <= '9' ? atoi(w[g]) : 0;
				string s; s[0] = 0;
				if(!numeric) s_strcat(s, nick);

				loopi(numargs-1) if(w[i+1])
				{
					if(s[0]) s_strcat(s, " ");
					s_strcat(s, w[i+1]);
				}
				if(s[0]) conoutf("[%s] %s", n->name, s);

				if(numeric) switch(numeric)
				{
					case 376:
					{
						if(n->state == IRC_CONN) n->state = IRC_ONLINE;
						break;
					}
					default: break;
				}
			}

			if(nick) DELETEP(nick);
			if(user) DELETEP(user);
			if(host) DELETEP(host);
		}
		loopj(MAXWORDS) if(w[j]) delete[] w[j];
		while(p && (*p == '\n' || *p == '\r' || *p == ' ')) p++;
	}
}

void irccleanup()
{
	loopv(ircnets) if(ircnets[i].sock != ENET_SOCKET_NULL)
	{
		ircnet *n = &ircnets[i];
		if(n->state != IRC_DISC) ircsend(n, "QUIT");
	}
	conoutf("Waiting for IRC threads to finish...");
#ifdef WIN32
	Sleep(3000);
#else
	sleep(3);
#endif
	ircslice();
	loopv(ircnets) if(ircnets[i].sock != ENET_SOCKET_NULL)
	{
		ircnet *n = &ircnets[i];
		enet_socket_destroy(n->sock);
	}
}

void ircslice()
{
	loopv(ircnets) if(ircnets[i].sock != ENET_SOCKET_NULL)
	{
		ircnet *n = &ircnets[i];
		int oldstate = n->state;
		switch(oldstate)
		{
			case IRC_ATTEMPT:
			{
				if(*n->passkey) ircsend(n, "PASS %s", n->passkey);
				ircsend(n, "NICK %s", n->nick);
				ircsend(n, "USER %s +iw %s :%s", n->nick, n->nick, n->nick);
				n->state = IRC_CONN;
				loopvj(n->channels) n->channels[j].state = IRCC_NONE;
				break;
			}
			case IRC_CONN:
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
		if(n->state != oldstate && n->state == IRC_ONLINE)
		{
			conoutf("[%s] * Now connected to %s as %s", n->name, n->serv, n->nick);
			loopvj(n->channels) ircjoin(n, &n->channels[j]);
		}
	}
}
#endif
