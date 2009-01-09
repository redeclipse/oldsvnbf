#include "pch.h"
#include "engine.h"
#include "crypto.h"
#include "hash.h"

#include <sys/types.h>
#include <enet/time.h>

#ifdef WIN32
#include <winerror.h>
#ifndef ENOTCONN
	#define ENOTCONN WSAENOTCONN
#endif
#else
#include <errno.h>
#endif

VAR(masterserver, 0, 1, 1);
VAR(masterport, 1, ENG_MASTER_PORT, INT_MAX-1);
SVAR(masterip, "");

struct authreq
{
    enet_uint32 reqtime;
    uint id;
    gfint answer;
};

struct authuser
{
    char *name;
    ecjacobian pubkey;
};

struct masterclient
{
    ENetAddress address;
    ENetSocket socket;
    string name;
    char input[4096];
    vector<char> output;
    int inputpos, outputpos, port, qport;
    enet_uint32 connecttime;
    vector<authreq> authreqs;
    bool isserver;

    masterclient() : inputpos(0), outputpos(0), isserver(false) {}
};

vector<masterclient *> masterclients;
ENetSocket mastersocket = ENET_SOCKET_NULL;
time_t starttime;

void setupmaster()
{
	conoutf("init: master (%s:%d)", *masterip ? masterip : "*", masterport);
	ENetAddress address = { ENET_HOST_ANY,  masterport };
	if(*masterip && enet_address_set_host(&address, masterip) < 0) fatal("failed to resolve master address: %s", masterip);
	if((mastersocket = enet_socket_create(ENET_SOCKET_TYPE_STREAM)) == ENET_SOCKET_NULL) fatal("failed to create master server socket");
	if(enet_socket_set_option(mastersocket, ENET_SOCKOPT_REUSEADDR, 1) < 0) fatal("failed to set master server socket option");
	if(enet_socket_bind(mastersocket, &address) < 0) fatal("failed to bind master server socket");
	if(enet_socket_listen(mastersocket, -1) < 0) fatal("failed to listen on master server socket");
	if(enet_socket_set_option(mastersocket, ENET_SOCKOPT_NONBLOCK, 1) < 0) fatal("failed to make master server socket non-blocking");
    starttime = time(NULL);
    char *ct = ctime(&starttime);
    if(strchr(ct, '\n')) *strchr(ct, '\n') = '\0';
	conoutf("master server started on %s:[%d]", *masterip ? masterip : "localhost", masterport);
}

void masterout(masterclient &c, const char *msg, int len = 0)
{
	if(!len) len = strlen(msg);
	c.output.put(msg, len);
}

void masteroutf(masterclient &c, const char *fmt, ...)
{
	string msg;
	va_list args;
	va_start(args, fmt);
	formatstring(msg, fmt, args);
	va_end(args);
	masterout(c, msg);
}

hashtable<char *, authuser> authusers;

void addauth(char *name, char *pubkey)
{
	name = newstring(name);
	authuser &u = authusers[name];
	u.name = name;
	u.pubkey.parse(pubkey);
}
COMMAND(addauth, "ss");

void clearauth()
{
	enumerate(authusers, authuser, u, delete[] u.name);
	authusers.clear();
}
COMMAND(clearauth, "");

void purgeauths(masterclient &c)
{
	int expired = 0;
	loopv(c.authreqs)
	{
		if(ENET_TIME_DIFFERENCE(lastmillis, c.authreqs[i].reqtime) >= AUTH_TIME)
		{
			masteroutf(c, "failauth %u\n", c.authreqs[i].id);
			expired = i + 1;
		}
		else break;
	}
	if(expired > 0) c.authreqs.remove(0, expired);
}

void reqauth(masterclient &c, uint id, char *name)
{
	purgeauths(c);

	time_t t = time(NULL);
	char *ct = ctime(&t);
	if(ct)
	{
		char *newline = strchr(ct, '\n');
		if(newline) *newline = '\0';
	}
	string ip;
	if(enet_address_get_host_ip(&c.address, ip, sizeof(ip)) < 0) s_strcpy(ip, "-");
	conoutf("%s: attempting \"%s\" as %u from %s\n", ct ? ct : "-", name, id, ip);

	authuser *u = authusers.access(name);
	if(!u)
	{
		masteroutf(c, "failauth %u\n", id);
		return;
	}

	if(c.authreqs.length() >= AUTH_LIMIT)
	{
		masteroutf(c, "failauth %u\n", c.authreqs[0].id);
		c.authreqs.remove(0);
	}

	authreq &a = c.authreqs.add();
	a.reqtime = lastmillis;
	a.id = id;
	uint seed[3] = { starttime, lastmillis, randomMT() };
	tiger::hashval hash;
	tiger::hash((uchar *)seed, sizeof(seed), hash);
	gfint challenge;
	memcpy(challenge.digits, hash.bytes, sizeof(challenge.digits));
	challenge.len = 8*sizeof(hash.bytes)/BI_DIGIT_BITS;
	challenge.shrink();

	ecjacobian answer(u->pubkey);
	answer.mul(challenge);
	answer.normalize();
	a.answer = answer.x;

	//printf("expecting %u for user %s to be ", id, u->name);
	//a.answer.print(stdout);
	//printf(" given secret ");

	ecjacobian secret(ecjacobian::base);
	secret.mul(challenge);
	secret.normalize();

	static vector<char> buf;
	buf.setsizenodelete(0);
	secret.print(buf);
	buf.add('\0');

	//printf("%s\n", buf.getbuf());

	masteroutf(c, "chalauth %u %s\n", id, buf.getbuf());
}

void confauth(masterclient &c, uint id, const char *val)
{
	purgeauths(c);

	loopv(c.authreqs) if(c.authreqs[i].id == id)
	{
		gfint answer(val);
		string ip;
		if(enet_address_get_host_ip(&c.address, ip, sizeof(ip)) < 0) s_strcpy(ip, "-");
		if(answer == c.authreqs[i].answer)
		{
			masteroutf(c, "succauth %u\n", id);
			conoutf("succeeded %u from %s\n", id, ip);
		}
		else
		{
			masteroutf(c, "failauth %u\n", id);
			conoutf("failed %u from %s\n", id, ip);
		}
		c.authreqs.remove(i--);
		return;
	}
	masteroutf(c, "failauth %u\n", id);
}

void purgemasterclient(int n)
{
	masterclient &c = *masterclients[n];
	enet_socket_destroy(c.socket);
	delete masterclients[n];
	masterclients.remove(n);
}

bool checkmasterclientinput(masterclient &c)
{
	if(c.inputpos<0) return true;
	bool result = false;
	const int MAXWORDS = 25;
	char *w[MAXWORDS], *p = c.input;
	int numargs = MAXWORDS;
	conoutf("master cmd from %s: %s", c.name, p);
	while(!result)
	{
		loopi(MAXWORDS)
		{
			w[i] = (char *)"";
			if(i > numargs) continue;
			for(;;)
			{
				p += strspn(p, " \t\r");
				if(p[0] != '/' || p[1] != '/') break;
				p += strcspn(p, "\n\0");
			}
			if(*p=='\"')
			{
				p++;
				const char *word = p;
				p += strcspn(p, "\"\r\n\0");
				char *s = newstring(word, p-word);
				if(*p=='\"') p++;
				if(s) w[i] = s;
				else numargs = i;
				continue;
			}
			const char *word = p;
			for(;;)
			{
				p += strcspn(p, "/; \t\r\n\0");
				if(p[0] != '/' || p[1] == '/') break;
				else if(p[1] == '\0') { p++; break; }
				p += 2;
			}
			if(p-word == 0) numargs = i;
			else
			{
				char *s = newstring(word, p-word);
				if(s) w[i] = s;
				else numargs = i;
			}
		}

		p += strcspn(p, ";\n\0"); p++;
		if(!*w[0])
		{
			if(*p) continue;
			else result = true;
		}
		else if(!strcmp(w[0], "add"))
		{
			c.port = ENG_SERVER_PORT;
			c.qport = ENG_QUERY_PORT;
			if(*w[1]) c.port = clamp(atoi(w[1]), 1, INT_MAX-1);
			if(*w[2]) c.qport = clamp(atoi(w[2]), 1, INT_MAX-1);
			masteroutf(c, "echo [server initiated]\n");
			c.isserver = true;
			result = true;
		}
		else if(!strcmp(w[0], "list"))
		{
			loopvj(masterclients) if(masterclients[j]->isserver)
			{
				masterclient &s = *masterclients[j];
				masteroutf(c, "addserver %s %d %d\n", s.name, s.port, s.qport);
			}
			result = true;
		}
		else
		{
			if(c.isserver)
			{
				if(!strcmp(w[0], "reqauth"))
				{
					reqauth(c, uint(atoi(w[1])), w[2]);
					result = true;
				}
				else if(!strcmp(w[0], "confauth"))
				{
					confauth(c, uint(atoi(w[1])), w[2]);
					result = true;
				}
			}
			if(!result)
			{
				masteroutf(c, "error [unknown command]\n");
				result = true;
			}
		}
		loopj(numargs) if(w[j]) delete[] w[j];
	}
	return result || c.inputpos < (int)sizeof(c.input);
}

fd_set readset, writeset;

void checkmaster()
{
	fd_set readset, writeset;
	int nfds = mastersocket;
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_SET(mastersocket, &readset);
	loopv(masterclients)
	{
		masterclient &c = *masterclients[i];
		if(c.outputpos < c.output.length()) FD_SET(c.socket, &writeset);
		else FD_SET(c.socket, &readset);
		nfds = max(nfds, (int)c.socket);
	}
	timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	if(select(nfds+1, &readset, &writeset, NULL, &tv)<=0) return;

	if(FD_ISSET(mastersocket, &readset))
	{
		ENetAddress address;
		ENetSocket masterclientsocket = enet_socket_accept(mastersocket, &address);
		if(masterclients.length()>=MASTER_LIMIT) enet_socket_destroy(masterclientsocket);
		else if(masterclientsocket!=ENET_SOCKET_NULL)
		{
			int dups = 0, oldest = -1;
			loopv(masterclients) if(masterclients[i]->address.host == address.host)
			{
				dups++;
				if(oldest<0 || masterclients[i]->connecttime < masterclients[oldest]->connecttime) oldest = i;
			}
			if(dups >= DUP_LIMIT) purgemasterclient(oldest);
			masterclient *c = new masterclient;
			c->address = address;
			c->socket = masterclientsocket;
			c->connecttime = lastmillis;
			masterclients.add(c);
			enet_address_get_host_ip(&c->address, c->name, sizeof(c->name));
		}
	}

	loopv(masterclients)
	{
		masterclient &c = *masterclients[i];
		if(c.outputpos < c.output.length() && FD_ISSET(c.socket, &writeset))
		{
			ENetBuffer buf;
			buf.data = (void *)&c.output[c.outputpos];
			buf.dataLength = c.output.length()-c.outputpos;
			int res = enet_socket_send(c.socket, NULL, &buf, 1);
			if(res>=0)
			{
				c.outputpos += res;
				if(c.outputpos>=c.output.length())
				{
					c.output.setsizenodelete(0);
					c.outputpos = 0;
				}
			}
			else { purgemasterclient(i--); continue; }
		}
		if(FD_ISSET(c.socket, &readset))
		{
			ENetBuffer buf;
			buf.data = &c.input[c.inputpos];
			buf.dataLength = sizeof(c.input) - c.inputpos;
			int res = enet_socket_receive(c.socket, NULL, &buf, 1);
			if(res>0)
			{
				c.inputpos += res;
				c.input[min(c.inputpos, (int)sizeof(c.input)-1)] = '\0';
				if(!checkmasterclientinput(c)) { purgemasterclient(i--); continue; }
			}
			else { purgemasterclient(i--); continue; }
		}
		//if(c.output.length() > OUTPUT_LIMIT) { purgemasterclient(i--); continue; }
        if(!c.isserver && ENET_TIME_DIFFERENCE(lastmillis, c.connecttime) >= CLIENT_TIME)
        {
        	purgemasterclient(i--);
        	continue;
		}
	}
}

void cleanupmaster()
{
	if(mastersocket) enet_socket_destroy(mastersocket);
}
