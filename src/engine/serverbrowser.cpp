// serverbrowser.cpp: eihrul's concurrent resolver, and server browser window management

#include "engine.h"
#include "SDL_thread.h"

struct resolverthread
{
    SDL_Thread *thread;
    const char *query;
    int starttime;
};

struct resolverresult
{
    const char *query;
    ENetAddress address;
};

vector<resolverthread> resolverthreads;
vector<const char *> resolverqueries;
vector<resolverresult> resolverresults;
SDL_mutex *resolvermutex;
SDL_cond *querycond, *resultcond;

#define RESOLVERTHREADS 2
#define RESOLVERLIMIT 3500

int resolverloop(void * data)
{
	resolverthread *rt = (resolverthread *)data;
	SDL_LockMutex(resolvermutex);
	SDL_Thread *thread = rt->thread;
	SDL_UnlockMutex(resolvermutex);
	if(!thread || SDL_GetThreadID(thread) != SDL_ThreadID())
		return 0;
	while(thread == rt->thread)
	{
		SDL_LockMutex(resolvermutex);
		while(resolverqueries.empty()) SDL_CondWait(querycond, resolvermutex);
		rt->query = resolverqueries.pop();
		rt->starttime = totalmillis;
		SDL_UnlockMutex(resolvermutex);

		ENetAddress address = { ENET_HOST_ANY, ENET_PORT_ANY };
		enet_address_set_host(&address, rt->query);

		SDL_LockMutex(resolvermutex);
		if(rt->query && thread == rt->thread)
		{
			resolverresult &rr = resolverresults.add();
			rr.query = rt->query;
			rr.address = address;
			rt->query = NULL;
			rt->starttime = 0;
			SDL_CondSignal(resultcond);
		}
		SDL_UnlockMutex(resolvermutex);
	}
	return 0;
}

void resolverinit()
{
	resolvermutex = SDL_CreateMutex();
	querycond = SDL_CreateCond();
	resultcond = SDL_CreateCond();

	SDL_LockMutex(resolvermutex);
	loopi(RESOLVERTHREADS)
	{
		resolverthread &rt = resolverthreads.add();
		rt.query = NULL;
		rt.starttime = 0;
		rt.thread = SDL_CreateThread(resolverloop, &rt);
	}
	SDL_UnlockMutex(resolvermutex);
}

void resolverstop(resolverthread &rt)
{
	SDL_LockMutex(resolvermutex);
	if(rt.query)
	{
#ifndef __APPLE__
		SDL_KillThread(rt.thread);
#endif
		rt.thread = SDL_CreateThread(resolverloop, &rt);
	}
	rt.query = NULL;
	rt.starttime = 0;
	SDL_UnlockMutex(resolvermutex);
}

void resolverclear()
{
	if(resolverthreads.empty()) return;

	SDL_LockMutex(resolvermutex);
	resolverqueries.setsize(0);
	resolverresults.setsize(0);
	loopv(resolverthreads)
	{
		resolverthread &rt = resolverthreads[i];
		resolverstop(rt);
	}
	SDL_UnlockMutex(resolvermutex);
}

void resolverquery(const char *name)
{
	if(resolverthreads.empty()) resolverinit();

	SDL_LockMutex(resolvermutex);
    resolverqueries.add(name);
	SDL_CondSignal(querycond);
	SDL_UnlockMutex(resolvermutex);
}

bool resolvercheck(const char **name, ENetAddress *address)
{
	bool resolved = false;
	SDL_LockMutex(resolvermutex);
	if(!resolverresults.empty())
	{
		resolverresult &rr = resolverresults.pop();
		*name = rr.query;
		address->host = rr.address.host;
		resolved = true;
	}
	else loopv(resolverthreads)
	{
		resolverthread &rt = resolverthreads[i];
		if(rt.query && totalmillis - rt.starttime > RESOLVERLIMIT)
		{
			resolverstop(rt);
			*name = rt.query;
			resolved = true;
		}
	}
	SDL_UnlockMutex(resolvermutex);
	return resolved;
}

bool resolverwait(const char *name, ENetAddress *address)
{
	if(resolverthreads.empty()) resolverinit();

	defformatstring(text)("resolving %s...", name);
	progress(0, text);

	SDL_LockMutex(resolvermutex);
    resolverqueries.add(name);
	SDL_CondSignal(querycond);
	int starttime = SDL_GetTicks(), timeout = 0;
	bool resolved = false;
	for(;;)
	{
		SDL_CondWaitTimeout(resultcond, resolvermutex, 250);
		loopv(resolverresults) if(resolverresults[i].query == name)
		{
			address->host = resolverresults[i].address.host;
			resolverresults.remove(i);
			resolved = true;
			break;
		}
		if(resolved) break;

		timeout = SDL_GetTicks() - starttime;
        progress(min(float(timeout)/RESOLVERLIMIT, 1.0f), text);
        if(interceptkey(SDLK_ESCAPE)) timeout = RESOLVERLIMIT + 1;
		if(timeout > RESOLVERLIMIT) break;
	}
	if(!resolved && timeout > RESOLVERLIMIT)
	{
		loopv(resolverthreads)
		{
			resolverthread &rt = resolverthreads[i];
			if(rt.query == name) { resolverstop(rt); break; }
		}
	}
	SDL_UnlockMutex(resolvermutex);
	return resolved;
}

SDL_Thread *connthread = NULL;
SDL_mutex *connmutex = NULL;
SDL_cond *conncond = NULL;

struct connectdata
{
	ENetSocket sock;
	ENetAddress address;
	int result;
};

// do this in a thread to prevent timeouts
// could set timeouts on sockets, but this is more reliable and gives more control
int connectthread(void *data)
{
	SDL_LockMutex(connmutex);
	if(!connthread || SDL_GetThreadID(connthread) != SDL_ThreadID())
	{
		SDL_UnlockMutex(connmutex);
		return 0;
	}
	connectdata cd = *(connectdata *)data;
	SDL_UnlockMutex(connmutex);

	int result = enet_socket_connect(cd.sock, &cd.address);

	SDL_LockMutex(connmutex);
	if(!connthread || SDL_GetThreadID(connthread) != SDL_ThreadID())
	{
		enet_socket_destroy(cd.sock);
		SDL_UnlockMutex(connmutex);
		return 0;
	}
	((connectdata *)data)->result = result;
	SDL_CondSignal(conncond);
	SDL_UnlockMutex(connmutex);

	return 0;
}

#define CONNLIMIT 20000

int connectwithtimeout(ENetSocket sock, const char *hostname, ENetAddress &address)
{
	defformatstring(text)("connecting to %s:[%d]...", hostname != NULL ? hostname : "local server", address.port);
	progress(0, text);

	if(!connmutex) connmutex = SDL_CreateMutex();
	if(!conncond) conncond = SDL_CreateCond();
	SDL_LockMutex(connmutex);
	connectdata cd = { sock, address, -1 };
	connthread = SDL_CreateThread(connectthread, &cd);

	int starttime = SDL_GetTicks(), timeout = 0;
	for(;;)
	{
		if(!SDL_CondWaitTimeout(conncond, connmutex, 250))
		{
			if(cd.result<0) enet_socket_destroy(sock);
			break;
		}
		timeout = SDL_GetTicks() - starttime;
        progress(min(float(timeout)/CONNLIMIT, 1.0f), text);
        if(interceptkey(SDLK_ESCAPE)) timeout = CONNLIMIT + 1;
		if(timeout > CONNLIMIT) break;
	}

	/* thread will actually timeout eventually if its still trying to connect
	 * so just leave it (and let it destroy socket) instead of causing problems on some platforms by killing it
	 */
	connthread = NULL;
	SDL_UnlockMutex(connmutex);

	return cd.result;
}

vector<serverinfo *> servers;
ENetSocket pingsock = ENET_SOCKET_NULL;
int lastinfo = 0;

static serverinfo *newserver(const char *name, int port = ENG_SERVER_PORT, uint ip = ENET_HOST_ANY)
{
    serverinfo *si = new serverinfo(ip, port);

    if(name) copystring(si->name, name);
    else if(ip==ENET_HOST_ANY || enet_address_get_host_ip(&si->address, si->name, sizeof(si->name)) < 0)
    {
        delete si;
        return NULL;
    }

    servers.add(si);

    return si;
}

void addserver(const char *name, int port)
{
    loopv(servers) if(!strcmp(servers[i]->name, name) && servers[i]->port == port) return;
	if(newserver(name, port) && verbose >= 2)
		conoutf("added server %s (%d)", name, port);
}
ICOMMAND(addserver, "si", (char *n, int *a), addserver(n, a ? *a : ENG_SERVER_PORT));
VARP(searchlan, 0, 0, 1);
VARP(maxservpings, 0, 10, 1000);

void pingservers()
{
    if(pingsock == ENET_SOCKET_NULL)
    {
        pingsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
        if(pingsock == ENET_SOCKET_NULL)
        {
            lastinfo = totalmillis;
            return;
        }
        enet_socket_set_option(pingsock, ENET_SOCKOPT_NONBLOCK, 1);
        enet_socket_set_option(pingsock, ENET_SOCKOPT_BROADCAST, 1);
    }
    ENetBuffer buf;
    uchar ping[MAXTRANS];
    ucharbuf p(ping, sizeof(ping));
    putint(p, totalmillis);

    static int lastping = 0;
    if(lastping >= servers.length()) lastping = 0;
    loopi(maxservpings ? min(servers.length(), maxservpings) : servers.length())
    {
        serverinfo &si = *servers[lastping];
        if(++lastping >= servers.length()) lastping = 0;
        if(si.address.host == ENET_HOST_ANY) continue;
        buf.data = ping;
        buf.dataLength = p.length();
        enet_socket_send(pingsock, &si.address, &buf, 1);
    }
    if(searchlan)
    {
        ENetAddress address;
        address.host = ENET_HOST_BROADCAST;
        address.port = serverport+1;
        buf.data = ping;
        buf.dataLength = p.length();
        enet_socket_send(pingsock, &address, &buf, 1);
    }
    lastinfo = totalmillis;
}

void checkresolver()
{
	int resolving = 0;
	loopv(servers)
	{
		serverinfo &si = *servers[i];
		if(si.resolved == serverinfo::RESOLVED) continue;
		if(si.address.host == ENET_HOST_ANY)
		{
			if(si.resolved == serverinfo::UNRESOLVED) { si.resolved = serverinfo::RESOLVING; resolverquery(si.name); }
			resolving++;
		}
	}
	if(!resolving) return;

	const char *name = NULL;
    for(;;)
    {
	    ENetAddress addr = { ENET_HOST_ANY, ENET_PORT_ANY };
	    if(!resolvercheck(&name, &addr)) break;
		loopv(servers)
		{
			serverinfo &si = *servers[i];
			if(name == si.name)
			{
				si.resolved = serverinfo::RESOLVED;
				si.address.host = addr.host;
				break;
			}
		}
	}
}

void checkpings()
{
	if(pingsock==ENET_SOCKET_NULL) return;
	enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
	ENetBuffer buf;
	ENetAddress addr;
	uchar ping[MAXTRANS];
	char text[MAXTRANS];
	buf.data = ping;
	buf.dataLength = sizeof(ping);
	while(enet_socket_wait(pingsock, &events, 0) >= 0 && events)
	{
        int len = enet_socket_receive(pingsock, &addr, &buf, 1);
        if(len <= 0) return;
        serverinfo *si = NULL;
        loopv(servers) if(addr.host == servers[i]->address.host && addr.port == servers[i]->address.port) { si = servers[i]; break; }
        if(!si && searchlan) si = newserver(NULL, ENG_SERVER_PORT, addr.host);
        if(si) si->reset();
        else continue;
        ucharbuf p(ping, len);
        si->ping = totalmillis - getint(p);
        si->numplayers = getint(p);
        int numattr = getint(p);
        si->attr.setsize(0);
        loopj(numattr) si->attr.add(getint(p));
        getstring(text, p);
        filtertext(si->map, text);
        getstring(text, p);
        filtertext(si->sdesc, text);
        if(!strcmp(si->sdesc, "unnamed")) copystring(si->sdesc, si->name);
        loopi(si->numplayers)
        {
        	getstring(text, p);
        	if(text[0]) si->players.add(newstring(text));
		}
	}
}

int sicompare(serverinfo **a, serverinfo **b) { return client::servercompare(*a, *b); }

VARP(serverupdateinterval, 0, 5, INT_MAX-1);

void refreshservers()
{
	static int lastrefresh = 0;
	if(lastrefresh == totalmillis) return;
    if(totalmillis - lastrefresh > 1000) loopv(servers) servers[i]->reset();
	lastrefresh = totalmillis;

	checkresolver();
	checkpings();
    if(totalmillis - lastinfo >= (serverupdateinterval*1000)/(maxservpings ? max(1, (servers.length() + maxservpings - 1) / maxservpings) : 1)) pingservers();
}

bool reqmaster = false;

void clearservers()
{
    resolverclear();
    servers.deletecontentsp();
	lastinfo = 0;
}

COMMAND(clearservers, "");

void updatefrommaster()
{
	uchar buf[32000];
	uchar *reply = retrieveservers(buf, sizeof(buf));
	if(*reply)
	{
        clearservers();
		execute((char *)reply);
		if(verbose) conoutf("\faretrieved %d server(s) from master", servers.length());
		else conoutf("\faretrieved list from master successfully", servers.length());
	}
	else conoutf("master server not replying");
	refreshservers();
}
COMMAND(updatefrommaster, "");

void updateservers()
{
	if(servers.empty() && !reqmaster)
	{
		updatefrommaster();
		reqmaster = true;
	}
	refreshservers();
	servers.sort(sicompare);
	intret(servers.length());
}
COMMAND(updateservers, "");
