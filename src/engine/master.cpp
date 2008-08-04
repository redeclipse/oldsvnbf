#include "pch.h"
#include "engine.h"

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

vector<masterentry> masterentries;
vector<masterlist *> masterlists;
bool updatemasterlist = true;

vector<masterclient *> masterclients;
ENetSocket mastersocket = ENET_SOCKET_NULL;

void setupmaster()
{
	conoutf("init: master");

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = masterport;

    if(*masterip)
    {
        if(enet_address_set_host(&address, masterip)<0)
            fatal("failed to resolve server address: %s\n", masterip);
    }
    mastersocket = enet_socket_create(ENET_SOCKET_TYPE_STREAM, &address);
    if(mastersocket==ENET_SOCKET_NULL)
        fatal("failed to create master server socket\n");
    if(enet_socket_set_option(mastersocket, ENET_SOCKOPT_NONBLOCK, 1)<0)
        fatal("failed to make master server socket non-blocking\n");

    enet_time_set(0);

    conoutf("master server started on %s:[%d]", *masterip ? masterip : "localhost", masterport);
}

void genmasterlist()
{
    if(!updatemasterlist) return;
    while(masterlists.length() && masterlists.last()->refs<=0)
        delete masterlists.pop();
    masterlist *l = new masterlist;
    loopv(masterentries)
    {
        masterentry &s = masterentries[i];
        string hostname;
        if(enet_address_get_host_ip(&s.address, hostname, sizeof(hostname))<0) continue;
        s_sprintfd(cmd)("addserver %s %d %d\n", hostname, s.port, s.qport);
        l->buf.put(cmd, strlen(cmd));
    }
    l->buf.add('\0');
	masterlists.add(l);
    updatemasterlist = false;
}

masterout unkcmdout("unknown command"), registermasterout("registered server"), renewmasterout("renewed server registration");

void addmasterentry(masterclient &c, int port, int qport)
{
    if(masterentries.length()>=SERVER_LIMIT) return;
    loopv(masterentries)
    {
        masterentry &s = masterentries[i];
        if(s.address.host==c.address.host)
        {
            s.registertime = lastmillis;
            c.output = &renewmasterout;
            c.outputpos = 0;
            return;
        }
    }
    masterentry &s = masterentries.add();
    s.address = c.address;
    s.port = port;
    s.qport = qport;
    s.registertime = lastmillis;
    c.output = &registermasterout;
    c.outputpos = 0;
    if(verbose) conoutf("master received registration from %s (ports %d,%d)", c.name, port, qport);
    updatemasterlist = true;
}

void checkmasterentries()
{
    loopv(masterentries)
    {
        masterentry &s = masterentries[i];
        if(ENET_TIME_DIFFERENCE(lastmillis, s.registertime) > KEEPALIVE_TIME)
        {
            masterentries.remove(i--);
            updatemasterlist = true;
        }
    }
}

void masterlist::purge()
{
    refs = max(refs - 1, 0);
    if(refs<=0 && masterlists.last()!=this)
    {
        masterlists.removeobj(this);
        delete this;
    }
}

void purgemasterclient(int n)
{
    masterclient &c = *masterclients[n];
    if(c.output) c.output->purge();
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
	conoutf("master cmd from %s: %s", c.name, p);
	for(;;)
	{
		int numargs = MAXWORDS;
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
			else break;
		}
		else if(!strcmp(w[0], "add"))
		{
			int port = ENG_SERVER_PORT, qport = ENG_QUERY_PORT;
			if(*w[1]) port = clamp(atoi(w[1]), 1, INT_MAX-1);
			if(*w[2]) qport = clamp(atoi(w[2]), 1, INT_MAX-1);
			addmasterentry(c, port, qport);
			result = true;
			break;
		}
		else if(!strcmp(w[0], "list"))
		{
			genmasterlist();
			if(masterlists.empty()) return false;
			c.output = masterlists.last();
			c.outputpos = 0;
			result = true;
			break;
		}
		else break;
	}
	if(!result)
	{
		c.output = &registermasterout;
		c.outputpos = 0;
		result = true;
	}
	loopj(MAXWORDS) if(w[j]) delete[] w[j];
	return result || c.inputpos < (int)sizeof(c.input);
}

fd_set readset, writeset;

void checkmasterclients()
{
    fd_set readset, writeset;
    int nfds = mastersocket;
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_SET(mastersocket, &readset);
    loopv(masterclients)
    {
        masterclient &c = *masterclients[i];
        if(c.outputpos>=0) FD_SET(c.socket, &writeset);
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
        if(c.outputpos>=0 && FD_ISSET(c.socket, &writeset))
        {
            ENetBuffer buf;
            buf.data = (void *)&c.output->getbuf()[c.outputpos];
            buf.dataLength = c.output->length()-c.outputpos;
            int res = enet_socket_send(c.socket, NULL, &buf, 1);
            if(res>=0)
            {
                c.outputpos += res;
                if(c.outputpos>=c.output->length()) { purgemasterclient(i--); continue; }
            }
            else { purgemasterclient(i--); continue; }
        }
        if(c.outputpos<0 && FD_ISSET(c.socket, &readset))
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
        if(ENET_TIME_DIFFERENCE(lastmillis, c.connecttime) >= MASTER_TIME) { purgemasterclient(i--); continue; }
    }
}

void checkmaster()
{
	checkmasterentries();
	checkmasterclients();
}

void cleanupmaster()
{
	if(mastersocket) enet_socket_destroy(mastersocket);
}
