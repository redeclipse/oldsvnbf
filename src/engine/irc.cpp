#ifdef IRC
#include "engine.h"

vector<ircnet> ircnets;

ircnet *ircfind(const char *name)
{
	if(name && *name)
	{
		loopv(ircnets) if(!strcmp(ircnets[i].name, name)) return &ircnets[i];
	}
	return NULL;
}

void ircestablish(ircnet *n)
{
	if(!n) return;
	n->lastattempt = lastmillis;
	if(n->address.host == ENET_HOST_ANY)
	{
		conoutf("looking up %s:[%d]...", n->serv, n->port);
		if(!resolverwait(n->serv, n->port, &n->address))
		{
			conoutf("unable to resolve %s:[%d]...", n->serv, n->port);
			n->state = IRC_DISC;
			return;
		}
	}

	ENetAddress address = { ENET_HOST_ANY,  n->port };
	if(*n->ip && enet_address_set_host(&address, n->ip) < 0) conoutf("failed to bind address: %s", n->ip);
	n->sock = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
	if(*n->ip && n->sock != ENET_SOCKET_NULL && enet_socket_bind(n->sock, &address) < 0)
		conoutf("failed to bind connection socket", n->ip);
	if(n->sock == ENET_SOCKET_NULL || connectwithtimeout(n->sock, n->serv, n->address) < 0)
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
	defvformatstring(str, msg, msg);
	if(n->sock == ENET_SOCKET_NULL) return;
	if(verbose >= 2) console(0, "[%s] >>> %s", n->name, str);
	concatstring(str, "\n");
	ENetBuffer buf;
	buf.data = str;
	buf.dataLength = strlen((char *)buf.data);
	enet_socket_send(n->sock, NULL, &buf, 1);
}

void ircoutf(int relay, const char *msg, ...)
{
	defvformatstring(str, msg, msg);
	loopv(ircnets) if(ircnets[i].sock != ENET_SOCKET_NULL && ircnets[i].type == IRCT_RELAY && ircnets[i].state == IRC_ONLINE)
	{
		ircnet *n = &ircnets[i];
#if 0 // workaround for freenode's crappy dropping all but the first target of multi-target messages even though they don't state MAXTARGETS=1 in 005 string..
		string s = "";
		loopvj(n->channels) if(n->channels[j].state == IRCC_JOINED && n->channels[j].relay >= relay)
		{
			ircchan *c = &n->channels[j];
			if(s[0]) concatstring(s, ",");
			concatstring(s, c->name);
		}
		if(s[0]) ircsend(n, "PRIVMSG %s :%s", s, str);
#else
		loopvj(n->channels) if(n->channels[j].state == IRCC_JOINED && n->channels[j].relay >= relay)
			ircsend(n, "PRIVMSG %s :%s", n->channels[j].name, str);
#endif
	}
}

int ircrecv(ircnet *n, int timeout)
{
	if(!n) return -1;
	if(n->sock == ENET_SOCKET_NULL) return -1;
	enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
	ENetBuffer buf;
	int nlen = strlen((char *)n->input);
	buf.data = ((char *)n->input)+nlen;
	buf.dataLength = sizeof(n->input)-nlen;
	if(enet_socket_wait(n->sock, &events, timeout) >= 0 && events)
	{
		int len = enet_socket_receive(n->sock, NULL, &buf, 1);
		if(len <= 0)
		{
			enet_socket_destroy(n->sock);
			return -1;
		}
		buf.data = ((char *)buf.data)+len;
		((char *)buf.data)[0] = 0;
		buf.dataLength -= len;
		return len;
	}
	return 0;
}

void ircnewnet(int type, const char *name, const char *serv, int port, const char *nick, const char *ip, const char *passkey)
{
	if(!name || !serv || !port || !nick) return;
	ircnet *m = ircfind(name);
	if(m)
	{
		if(m->state != IRC_DISC) conoutf("ircnet %s already exists", m->name);
		else ircestablish(m);
		return;
	}
	ircnet &n = ircnets.add();
	n.type = type;
	n.state = IRC_DISC;
	n.sock = ENET_SOCKET_NULL;
	n.port = port;
	n.lastattempt = 0;
	copystring(n.name, name);
	copystring(n.serv, serv);
	copystring(n.nick, nick);
	copystring(n.ip, ip);
	copystring(n.passkey, passkey);
	n.address.host = ENET_HOST_ANY;
	n.address.port = n.port;
	n.input[0] = 0;
	conoutf("added irc %s %s (%s:%d) [%s]", type == IRCT_RELAY ? "relay" : "client", name, serv, port, nick);
}

ICOMMAND(ircaddclient, "ssisss", (const char *n, const char *s, int *p, const char *c, const char *h, const char *z), {
	ircnewnet(IRCT_CLIENT, n, s, *p, c, h, z);
});
ICOMMAND(ircaddrelay, "ssisss", (const char *n, const char *s, int *p, const char *c, const char *h, const char *z), {
	ircnewnet(IRCT_RELAY, n, s, *p, c, h, z);
});
ICOMMAND(ircserv, "ss", (const char *name, const char *s), {
	ircnet *n = ircfind(name);
	if(!n) { conoutf("no such ircnet: %s", name); return; }
	if(!s || !*s) { conoutf("%s current server is: %s", n->name, n->serv); return; }
	copystring(n->serv, s);
});
ICOMMAND(ircport, "ss", (const char *name, const char *s), {
	ircnet *n = ircfind(name);
	if(!n) { conoutf("no such ircnet: %s", name); return; }
	if(!s || !*s || !atoi(s)) { conoutf("%s current port is: %d", n->name, n->port); return; }
	n->port = atoi(s);
});
ICOMMAND(ircnick, "ss", (const char *name, const char *s), {
	ircnet *n = ircfind(name);
	if(!n) { conoutf("no such ircnet: %s", name); return; }
	if(!s || !*s) { conoutf("%s current nickname is: %s", n->name, n->nick); return; }
	copystring(n->nick, s);
});
ICOMMAND(ircbind, "ss", (const char *name, const char *s), {
	ircnet *n = ircfind(name);
	if(!n) { conoutf("no such ircnet: %s", name); return; }
	if(!s || !*s) { conoutf("%s currently bound to: %s", n->name, n->ip); return; }
	copystring(n->ip, s);
});
ICOMMAND(ircpass, "ss", (const char *name, const char *s), {
	ircnet *n = ircfind(name);
	if(!n) { conoutf("no such ircnet: %s", name); return; }
	if(!s || !*s) { conoutf("%s current password is: %s", n->name, n->passkey && *n->passkey ? "<set>" : "<not set>"); return; }
	copystring(n->passkey, s);
});
ICOMMAND(ircconnect, "s", (const char *name), {
	ircnet *n = ircfind(name);
	if(!n) { conoutf("no such ircnet: %s", name); return; }
	if(n->state != IRC_DISC) { conoutf("ircnet %s is already connected", n->name); return; }
	ircestablish(n);
});

ircchan *ircfindchan(ircnet *n, const char *name)
{
	if(n && name && *name)
	{
		loopv(n->channels) if(!strcasecmp(n->channels[i].name, name))
			return &n->channels[i];
	}
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
	if(*c->passkey) ircsend(n, "JOIN %s :%s", c->name, c->passkey);
	else ircsend(n, "JOIN %s", c->name);
	c->state = IRCC_JOINING;
	c->lastjoin = lastmillis;
	return true;
}

bool ircenterchan(ircnet *n, const char *name)
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

bool ircnewchan(int type, const char *name, const char *channel, const char *friendly, const char *passkey, int relay)
{
	if(!name || !channel) return false;
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
	d.relay = clamp(relay, 0, 4);
	d.lastjoin = 0;
	copystring(d.name, channel);
	copystring(d.friendly, friendly && *friendly ? friendly : channel);
	copystring(d.passkey, passkey);
	if(n->state != IRC_DISC) ircjoin(n, &d);
	conoutf("%s added channel %s", n->name, d.name);
	return true;
}

ICOMMAND(ircaddchan, "ssssi", (const char *n, const char *c, const char *f, const char *z, int *r), {
	ircnewchan(IRCCT_AUTO, n, c, f, z, *r);
});
ICOMMAND(ircjoinchan, "ssssi", (const char *n, const char *c, const char *f, const char *z, int *r), {
	ircnewchan(IRCCT_NONE, n, c, f, z, *r);
});
ICOMMAND(ircpasschan, "sss", (const char *name, const char *chan, const char *s), {
	ircnet *n = ircfind(name);
	if(!n) { conoutf("no such ircnet: %s", name); return; }
	ircchan *c = ircfindchan(n, chan);
	if(!c) { conoutf("no such %s channel: %s", n->name, chan); return; }
	if(!s || !*s) { conoutf("%s channel %s current password is: %s", n->name, c->name, c->passkey && *c->passkey ? "<set>" : "<not set>"); return; }
	copystring(c->passkey, s);
});
ICOMMAND(ircrelaychan, "sss", (const char *name, const char *chan, const char *s), {
	ircnet *n = ircfind(name);
	if(!n) { conoutf("no such ircnet: %s", name); return; }
	ircchan *c = ircfindchan(n, chan);
	if(!c) { conoutf("no such %s channel: %s", n->name, chan); return; }
	if(!s || !*s) { conoutf("%s channel %s current relay level is: %d", n->name, c->name, c->relay); return; }
	c->relay = clamp(atoi(s), 0, 4);
});
ICOMMAND(ircfriendlychan, "sss", (const char *name, const char *chan, const char *s), {
	ircnet *n = ircfind(name);
	if(!n) { conoutf("no such ircnet: %s", name); return; }
	ircchan *c = ircfindchan(n, chan);
	if(!c) { conoutf("no such %s channel: %s", n->name, chan); return; }
	if(!s || !*s) { conoutf("%s channel %s current friendly name is: %s", n->name, c->name, c->friendly); return; }
	copystring(c->friendly, s);
});

void ircprintf(ircnet *n, int relay, const char *target, const char *msg, ...)
{
	defvformatstring(str, msg, msg);
	string s = "";
	if(target && *target && strcasecmp(target, n->nick))
	{
		ircchan *c = ircfindchan(n, target);
		if(c)
		{
			formatstring(s)("\fs\fa[%s]\fS", c->friendly);
			while(c->lines.length() >= 1000)
			{
				char *a = c->lines.remove(0);
				DELETEA(a);
			}
			c->lines.add(newstring(str));
			if(n->type == IRCT_RELAY && c->relay >= relay)
				server::srvmsgf(relay != 1 ? -1 : -2, "%s %s", s, str);
		}
		else
		{
			formatstring(s)("\fs\fa[%s:%s]\fS", n->name, target);
			while(n->lines.length() >= 1000)
			{
				char *a = n->lines.remove(0);
				DELETEA(a);
			}
			n->lines.add(newstring(str));
		}
	}
	else
	{
		formatstring(s)("\fs\fa[%s]\fS", n->name);
		while(n->lines.length() >= 1000)
		{
			char *a = n->lines.remove(0);
			DELETEA(a);
		}
		n->lines.add(newstring(str));
	}
	console(0, "%s %s", s, str); // console is not used to relay
}

void ircprocess(ircnet *n, char *user[3], int g, int numargs, char *w[])
{
	if(!strcasecmp(w[g], "NOTICE") || !strcasecmp(w[g], "PRIVMSG"))
	{
		if(numargs > g+2)
		{
			bool ismsg = strcasecmp(w[g], "NOTICE");
			int len = strlen(w[g+2]);
			if(w[g+2][0] == '\001' && w[g+2][len-1] == '\001')
			{
				char *p = w[g+2];
				p++;
				const char *word = p;
				p += strcspn(p, " \001\0");
				if(p-word > 0)
				{
					char *q = newstring(word, p-word);
					p++;
					const char *start = p;
					p += strcspn(p, "\001\0");
					char *r = p-start > 0 ? newstring(start, p-start) : newstring("");
					if(ismsg)
					{
						if(!strcasecmp(q, "ACTION"))
							ircprintf(n, 1, g ? w[g+1] : NULL, "\fm* %s %s", user[0], r);
						else
						{
							ircprintf(n, 4, g ? w[g+1] : NULL, "\fr%s requests: %s %s", user[0], q, r);

							if(!strcasecmp(q, "VERSION"))
								ircsend(n, "NOTICE %s :\001VERSION %s v%.2f (%s) IRC interface, %s %s\001", user[0], ENG_NAME, float(ENG_VERSION)/100.f, ENG_RELEASE, ENG_BLURB, ENG_URL);
							else if(!strcasecmp(q, "PING")) // eh, echo back
								ircsend(n, "NOTICE %s :\001PING %s\001", user[0], r);
						}
					}
					else ircprintf(n, 4, g ? w[g+1] : NULL, "\fr%s replied: %s %s", user[0], q, r);
					DELETEA(q); DELETEA(r);
				}
			}
			else if(ismsg)
			{
#if 0 // y'know, for when the bot should say stuff on command..
				if(n->relay && ((g && !strcasecmp(w[g+1], n->nick)) || !strncasecmp(w[g+2], n->nick, strlen(n->nick))))
				{
					const int MAXWORDS = 25;
					char *w[MAXWORDS];
					int numargs = MAXWORDS;
					const char *p = &w[g+2][strlen(n->nick)];
					while(p && *p && (*p == ':' || *p == ';' || *p == ',' || *p == ' ' || *p == '\t'))
						p++;
					loopi(MAXWORDS)
					{
						w[i] = (char *)"";
						if(i > numargs) continue;
						char *s = parsetext(p);
						if(s) w[i] = s;
						else numargs = i;
					}
					p += strcspn(p, "\n\0"); p++;
					if(!strcmp(w[0], "help"))
					else if(w[0])
					loopj(numargs) if(w[j]) delete[] w[j];
				}
#endif
				ircprintf(n, 1, g ? w[g+1] : NULL, "\fw<%s> %s", user[0], w[g+2]);
			}
			else ircprintf(n, 2, g ? w[g+1] : NULL, "\fo-%s- %s", user[0], w[g+2]);
		}
	}
	else if(!strcasecmp(w[g], "NICK"))
	{
		if(numargs > g+1)
		{
			if(!strcasecmp(user[0], n->nick)) copystring(n->nick, w[g+1]);
			ircprintf(n, 3, NULL, "\fm%s (%s@%s) is now known as %s", user[0], user[1], user[2], w[g+1]);
		}
	}
	else if(!strcasecmp(w[g], "JOIN"))
	{
		if(numargs > g+1)
		{
			ircchan *c = ircfindchan(n, w[g+1]);
			if(c && !strcasecmp(user[0], n->nick))
			{
				c->state = IRCC_JOINED;
				c->lastjoin = lastmillis;
			}
			ircprintf(n, 3, w[g+1], "\fg%s (%s@%s) has joined", user[0], user[1], user[2]);
		}
	}
	else if(!strcasecmp(w[g], "PART"))
	{
		if(numargs > g+1)
		{
			ircchan *c = ircfindchan(n, w[g+1]);
			if(c && !strcasecmp(user[0], n->nick))
			{
				c->state = IRCC_NONE;
				c->lastjoin = lastmillis;
			}
			ircprintf(n, 3, w[g+1], "\fo%s (%s@%s) has left", user[0], user[1], user[2]);
		}
	}
	else if(!strcasecmp(w[g], "QUIT"))
	{
		if(numargs > g+1) ircprintf(n, 3, NULL, "\fr%s (%s@%s) has quit: %s", user[0], user[1], user[2], w[g+1]);
		else ircprintf(n, 3, NULL, "\fr%s (%s@%s) has quit", user[0], user[1], user[2]);
	}
	else if(!strcasecmp(w[g], "KICK"))
	{
		if(numargs > g+2)
		{
			ircchan *c = ircfindchan(n, w[g+1]);
			if(c && !strcasecmp(w[g+2], n->nick))
			{
				c->state = IRCC_KICKED;
				c->lastjoin = lastmillis;
			}
			ircprintf(n, 3, w[g+1], "\fr%s (%s@%s) has kicked %s from %s", user[0], user[1], user[2], w[g+2], w[g+1]);
		}
	}
	else if(!strcasecmp(w[g], "MODE"))
	{
		if(numargs > g+2)
		{
			string modestr = "";
			loopi(numargs-g-2)
			{
				if(i) concatstring(modestr, " ");
				concatstring(modestr, w[g+2+i]);
			}
			ircprintf(n, 4, w[g+1], "\fr%s (%s@%s) sets mode: %s %s", user[0], user[1], user[2], w[g+1], modestr);
		}
		else if(numargs > g+1)
			ircprintf(n, 4, w[g+1], "\fr%s (%s@%s) sets mode: %s", user[0], user[1], user[2], w[g+1]);
	}
	else if(!strcasecmp(w[g], "PING"))
	{
		if(numargs > g+1)
		{
			ircprintf(n, 4, NULL, "%s PING %s", user[0], w[g+1]);
			ircsend(n, "PONG %s", w[g+1]);
		}
		else
		{
			int t = time(NULL);
			ircprintf(n, 4, NULL, "%s PING (empty)", user[0]);
			ircsend(n, "PONG %d", t);
		}
	}
	else
	{
		int numeric = *w[g] && *w[g] >= '0' && *w[g] <= '9' ? atoi(w[g]) : 0, off = 0;
		string s = "";
		#define irctarget(a) (!strcasecmp(n->nick, a) || *a == '#' || ircfindchan(n, a))
		char *targ = numargs > g+1 && irctarget(w[g+1]) ? w[g+1] : NULL;
		if(numeric)
		{
			off = numeric == 353 ? 2 : 1;
			if(numargs > g+off+1 && irctarget(w[g+off+1]))
			{
				targ = w[g+off+1];
				off++;
			}
		}
		else concatstring(s, user[0]);
		for(int i = g+off+1; numargs > i && w[i]; i++)
		{
			if(s[0]) concatstring(s, " ");
			concatstring(s, w[i]);
		}
		if(numeric) switch(numeric)
		{
			case 001:
			{
				if(n->state == IRC_CONN)
				{
					n->state = IRC_ONLINE;
					ircprintf(n, 4, NULL, "\fbNow connected to %s as %s", user[0], n->nick);
				}
				break;
			}
			case 433:
			{
				if(n->state == IRC_CONN)
				{
					concatstring(n->nick, "_");
					ircsend(n, "NICK %s", n->nick);
				}
			}
			case 474:
			{
				ircchan *c = ircfindchan(n, w[g+2]);
				if(c)
				{
					c->state = IRCC_BANNED;
					c->lastjoin = lastmillis;
					if(c->type == IRCCT_AUTO)
						ircprintf(n, 4, w[g+2], "\fbWaiting 5 mins to rejoin %s", c->name);
				}
			}
			default: break;
		}
		if(s[0]) ircprintf(n, 4, targ, "\fw%s %s", w[g], s);
		else ircprintf(n, 4, targ, "\fw%s", w[g]);
	}
}

void ircparse(ircnet *n, char *reply)
{
	const int MAXWORDS = 25;
	char *w[MAXWORDS], *p = reply;
	loopi(MAXWORDS) w[i] = NULL;
	while(p && *p)
	{
		while(p && (*p == '\n' || *p == '\r' || *p == ' ')) p++; // eat up all the crap
		const char *start = p;
		bool line = false;
		int numargs = 0, g = *p == ':' ? 1 : 0;
		if(g) p++;
		bool full = false;
		loopi(MAXWORDS)
		{
			if(!p || !*p) break;
			const char *word = p;
			if(*p == ':') full = true; // uses the rest of the input line then
			p += strcspn(p, full ? "\r\n" : " \r\n");

			char *s = NULL;
			if(p-word > (full ? 1 : 0))
			{
				if(full) s = newstring(word+1, p-word-1);
				else s = newstring(word, p-word);
			}
			else s = newstring("");
			w[numargs] = s;
			numargs++;

			if(*p == '\n' || *p == '\r') line = true;
			if(*p) p++; else break;
			if(line) break;
		}
		if(line && numargs)
		{
			char *user[3] = { NULL, NULL, NULL };
			if(g)
			{
				const char *t = w[0];
				char *u = strrchr(t, '!');
				if(u)
				{
					user[0] = newstring(t, u-t);
					t = u + 1;
					u = strrchr(t, '@');
					if(u)
					{
						user[1] = newstring(t, u-t);
						if(*u++) user[2] = newstring(u);
					}
				}
				else user[0] = newstring(t);
			}
			else
			{
				user[0] = newstring("*");
				user[1] = newstring("*");
				user[2] = newstring(n->serv);
			}
			if(numargs > g) ircprocess(n, user, g, numargs, w);
			loopi(3) DELETEA(user[i]);
		}
		loopi(MAXWORDS) DELETEA(w[i]);
		if(!line)
		{
			char *s = newstring(start); // can't copy a buffer into itself so dupe it first
			copystring(reply, s);
			DELETEA(s);
			return;
		}
	}
	*reply = 0;
}

void ircdiscon(ircnet *n)
{
	if(!n) return;
	ircprintf(n, 4, NULL, "Disconnected");
	enet_socket_destroy(n->sock);
	n->state = IRC_DISC;
	n->sock = ENET_SOCKET_NULL;
	n->lastattempt = lastmillis;
}

void irccleanup()
{
	loopv(ircnets) if(ircnets[i].sock != ENET_SOCKET_NULL)
	{
		ircnet *n = &ircnets[i];
		ircsend(n, "QUIT :%s, %s %s", ENG_NAME, ENG_BLURB, ENG_URL);
		ircdiscon(n);
	}
}

void ircslice()
{
	loopv(ircnets)
	{
		ircnet *n = &ircnets[i];
		if(n->sock != ENET_SOCKET_NULL && n->state != IRC_DISC)
		{
			switch(n->state)
			{
				case IRC_ATTEMPT:
				{
					if(*n->passkey) ircsend(n, "PASS %s", n->passkey);
					ircsend(n, "NICK %s", n->nick);
					ircsend(n, "USER %s +iw %s :%s", n->nick, n->nick, n->nick);
					n->state = IRC_CONN;
					loopvj(n->channels)
					{
						ircchan *c = &n->channels[j];
						c->state = IRCC_NONE;
						c->lastjoin = lastmillis;
					}
					break;
				}
				case IRC_ONLINE:
				{
					loopvj(n->channels)
					{
						ircchan *c = &n->channels[j];
						if(c->type == IRCCT_AUTO && c->state != IRCC_JOINED && (!c->lastjoin || lastmillis-c->lastjoin >= (c->state != IRCC_BANNED ? 5000 : 300000)))
							ircjoin(n, c);
					}
					// fall through
				}
				case IRC_CONN:
				{
					switch(ircrecv(n))
					{
						case -1: ircdiscon(n); break; // fail
						case 0: break;
						default:
						{
							ircparse(n, (char *)n->input);
							break;
						}
					}
					break;
				}
				default:
				{
					ircdiscon(n);
					break;
				}
			}
		}
		else
		{
			if(!n->lastattempt || lastmillis-n->lastattempt >= 60000)
				ircestablish(n);
		}
	}
}
#ifndef STANDALONE
void irccmd(ircnet *n, ircchan *c, char *s)
{
	char *p = s;
	if(*p == '/')
	{
		p++;
		const char *word = p;
		p += strcspn(p, " \n\0");
		if(p-word > 0)
		{
			char *q = newstring(word, p-word), *r = newstring(*p ? ++p : "");
			if(!strcasecmp(q, "ME"))
			{
				if(c)
				{
					ircsend(n, "PRIVMSG %s :\001ACTION %s\001", c->name, r);
					ircprintf(n, 1, c->name, "\fm* %s %s", n->nick, r);
				}
				else ircprintf(n, 4, NULL, "\fbYou are not on a channel");
			}
			else if(!strcasecmp(q, "JOIN"))
			{
				ircchan *d = ircfindchan(n, r);
				if(d) ircjoin(n, d);
				else ircnewchan(IRCCT_AUTO, n->name, r);
			}
			else if(!strcasecmp(q, "PART"))
			{
				ircchan *d = ircfindchan(n, r);
				if(d) ircsend(n, "PART %s", d->name);
				else if(c) ircsend(n, "PART %s", c->name);
				else ircsend(n, "PART %s", r);
			}
			else if(*r) ircsend(n, "%s %s", q, r); // send it raw so we support any command
			else ircsend(n, "%s", q);
			DELETEA(q); DELETEA(r);
			return;
		}
		ircprintf(n, 4, c ? c->name : NULL, "\fbYou are not on a channel");
	}
	else if(c)
	{
		ircsend(n, "PRIVMSG %s :%s", c->name, p);
		ircprintf(n, 1, c->name, "\fw<%s> %s", n->nick, p);
	}
	else
	{
		ircsend(n, "%s", p);
		ircprintf(n, 4, NULL, "\fa>%s< %s", n->nick, p);
	}
}

bool ircchangui(g3d_gui *g, ircnet *n, ircchan *c, bool tab)
{
	if(tab) g->tab(c->name, GUI_TITLE_COLOR);

	defformatstring(cwindow)("%s_%s_window", n->name, c->name);
	g->fieldclear(cwindow);
	loopvk(c->lines) g->fieldline(cwindow, c->lines[k]);
	g->field(cwindow, GUI_TEXT_COLOR, -80, 20, NULL, EDITORREADONLY);
	g->fieldscroll(cwindow);

	defformatstring(cinput)("%s_%s_input", n->name, c->name);
	char *v = g->field(cinput, GUI_TEXT_COLOR, -80, 0, "", EDITORFOREVER);
	if(v && *v)
	{
		irccmd(n, c, v);
		*v = 0;
		g->fieldedit(cinput);
	}
	return true;
}

bool ircnetgui(g3d_gui *g, ircnet *n, bool tab)
{
	if(tab) g->tab(n->name, GUI_TITLE_COLOR);

	defformatstring(window)("%s_window", n->name);
	g->fieldclear(window);
	loopvk(n->lines) g->fieldline(window, n->lines[k]);
	g->field(window, GUI_TEXT_COLOR, -80, 20, NULL, EDITORREADONLY);
	g->fieldscroll(window);

	defformatstring(input)("%s_input", n->name);
	char *w = g->field(input, GUI_TEXT_COLOR, -80, 0, "", EDITORFOREVER);
	if(w && *w)
	{
		irccmd(n, NULL, w);
		*w = 0;
		g->fieldedit(input);
	}

	loopvj(n->channels) if(n->channels[j].state != IRCC_NONE && n->channels[j].name[0])
	{
		ircchan *c = &n->channels[j];
		if(!ircchangui(g, n, c, true)) return false;
	}

	return true;
}

bool ircgui(g3d_gui *g, const char *s)
{
	g->allowautotab(false);
	g->strut(81);
	if(s && *s)
	{
		ircnet *n = ircfind(s);
		if(n)
		{
			if(!ircnetgui(g, n, false)) return false;
		}
		else g->textf("not currently connected to %s", GUI_TEXT_COLOR, NULL, s);
	}
	else
	{
		int nets = 0;
		loopv(ircnets) if(ircnets[i].name[0] && ircnets[i].sock != ENET_SOCKET_NULL)
		{
			ircnet *n = &ircnets[i];
			g->pushlist();
			g->buttonf("%s via %s:[%d]", GUI_BUTTON_COLOR, NULL, n->name, n->serv, n->port);
			g->space(1);
			const char *ircstates[IRC_MAX] = {
					"\froffline", "\foconnecting", "\fynegotiating", "\fgonline"
			};
			g->buttonf("\fs%s\fS as %s", GUI_BUTTON_COLOR, NULL, ircstates[n->state], n->nick);
			g->poplist();
			nets++;
		}
		if(nets)
		{
			loopv(ircnets) if(ircnets[i].state != IRC_DISC && ircnets[i].name[0] && ircnets[i].sock != ENET_SOCKET_NULL)
			{
				ircnet *n = &ircnets[i];
				if(!ircnetgui(g, n, true)) return false;
			}
		}
		else g->text("no current connections..", GUI_TEXT_COLOR);
	}
	return true;
}

void guiirc(const char *s)
{
	if(cgui)
	{
		if(!ircgui(cgui, s) && shouldclearmenu) clearlater = true;
	}
}
ICOMMAND(ircgui, "s", (char *s), guiirc(s));
#endif
#endif
