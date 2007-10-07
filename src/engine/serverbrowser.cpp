// serverbrowser.cpp: eihrul's concurrent resolver, and server browser window management

#include "pch.h"
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

#define RESOLVERTHREADS 1
#define RESOLVERLIMIT 3000

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

		ENetAddress address = { ENET_HOST_ANY, sv->serverinfoport() };
		enet_address_set_host(&address, rt->query);

		SDL_LockMutex(resolvermutex);
		if(thread == rt->thread)
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

	s_sprintfd(text)("resolving %s... (esc to abort)", name);
	show_out_of_renderloop_progress(0, text);

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
		show_out_of_renderloop_progress(min(float(timeout)/RESOLVERLIMIT, 1), text);
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) timeout = RESOLVERLIMIT + 1;
		}
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

int connectwithtimeout(ENetSocket sock, char *hostname, ENetAddress &address)
{
	s_sprintfd(text)("connecting to %s... (esc to abort)", hostname);
	show_out_of_renderloop_progress(0, text);

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
		show_out_of_renderloop_progress(min(float(timeout)/CONNLIMIT, 1), text);
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) timeout = CONNLIMIT + 1;
		}
		if(timeout > CONNLIMIT) break;
	}

	/* thread will actually timeout eventually if its still trying to connect
	 * so just leave it (and let it destroy socket) instead of causing problems on some platforms by killing it 
	 */
	connthread = NULL;
	SDL_UnlockMutex(connmutex);

	return cd.result;
}
 
enum { UNRESOLVED = 0, RESOLVING, RESOLVED };

#ifndef BFRONTIER // moved to iengine.h
struct serverinfo
{
	char *name; //NOTE if string then threading+sorting lead to overwriten values
	string full;
	string map;
	string sdesc;
	int numplayers, ping, resolved;
	vector<int> attr;
	ENetAddress address;
};
#endif

vector<serverinfo> servers;
ENetSocket pingsock = ENET_SOCKET_NULL;
int lastinfo = 0;

char *getservername(int n) { return servers[n].name; }

void addserver(char *servername)
{
	loopv(servers) if(!strcmp(servers[i].name, servername)) return;
	serverinfo &si = servers.add();
	si.name = newstring(servername);
#ifndef BFRONTIER // fpsserver controlled gui
	si.full[0] = 0;
#endif
	si.ping = 999;
	si.map[0] = 0;
	si.sdesc[0] = 0;
	si.resolved = UNRESOLVED;
	si.address.host = ENET_HOST_ANY;
	si.address.port = sv->serverinfoport();
}

void pingservers()
{
    if(pingsock == ENET_SOCKET_NULL) 
    {
        pingsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM, NULL);
        if(pingsock != ENET_SOCKET_NULL) enet_socket_set_option(pingsock, ENET_SOCKOPT_NONBLOCK, 1);
    }
	ENetBuffer buf;
	uchar ping[MAXTRANS];
	loopv(servers)
	{
		serverinfo &si = servers[i];
		if(si.address.host == ENET_HOST_ANY) continue;
		ucharbuf p(ping, sizeof(ping));
		putint(p, totalmillis);
		buf.data = ping;
		buf.dataLength = p.length();
		enet_socket_send(pingsock, &si.address, &buf, 1);
	}
	lastinfo = totalmillis;
}
  
void checkresolver()
{
	int resolving = 0;
	loopv(servers)
	{
		serverinfo &si = servers[i];
		if(si.resolved == RESOLVED) continue;
		if(si.address.host == ENET_HOST_ANY)
		{
			if(si.resolved == UNRESOLVED) { si.resolved = RESOLVING; resolverquery(si.name); }
			resolving++;
		}
	}
	if(!resolving) return;

	const char *name = NULL;
	ENetAddress addr = { ENET_HOST_ANY, sv->serverinfoport() };
	while(resolvercheck(&name, &addr))
	{
		loopv(servers)
		{
			serverinfo &si = servers[i];
			if(name == si.name)
			{
				si.resolved = RESOLVED; 
				si.address = addr;
				addr.host = ENET_HOST_ANY;
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
		loopv(servers)
		{
			serverinfo &si = servers[i];
			if(addr.host == si.address.host)
			{
				ucharbuf p(ping, len);
				si.ping = totalmillis - getint(p);
				si.numplayers = getint(p);
				int numattr = getint(p);
				si.attr.setsize(0);
				loopj(numattr) si.attr.add(getint(p));
				getstring(text, p);
				s_strcpy(si.map, text);
				getstring(text, p);
				s_strcpy(si.sdesc, text);				
				break;
			}
		}
	}
}

int sicompare(serverinfo *a, serverinfo *b)
{
#ifdef BFRONTIER // fpsserver controlled gui
	return sv->servercompare(a, b);
#else
    bool ac = sv->servercompatible(a->name, a->sdesc, a->map, a->ping, a->attr, a->numplayers),
         bc = sv->servercompatible(b->name, b->sdesc, b->map, b->ping, b->attr, b->numplayers);
    if(ac>bc) return -1;
    if(bc>ac) return 1;   
    if(a->numplayers<b->numplayers) return 1;
    if(a->numplayers>b->numplayers) return -1;
    if(a->ping>b->ping) return 1;
    if(a->ping<b->ping) return -1;
    return strcmp(a->name, b->name);
#endif
}

void refreshservers()
{
	static int lastrefresh = 0;
	if(lastrefresh==totalmillis) return;
	lastrefresh = totalmillis;

	checkresolver();
	checkpings();
	if(totalmillis - lastinfo >= 5000) pingservers();
#ifndef BFRONTIER // fpsserver controlled gui
    servers.sort(sicompare);
	loopv(servers)
	{
		serverinfo &si = servers[i];
		if(si.address.host != ENET_HOST_ANY && si.ping != 999)
		{
            sv->serverinfostr(si.full, si.name, si.sdesc, si.map, si.ping, si.attr, si.numplayers);
		}
		else
		{
			s_sprintf(si.full)(si.address.host != ENET_HOST_ANY ? "[waiting for response] %s" : "[unknown host] %s\t", si.name);
		}
		si.full[60] = 0; // cut off too long server descriptions
	}
#endif
}

const char *showservers(g3d_gui *cgui)
{
	refreshservers();
#ifdef BFRONTIER // fpsserver controlled gui
    servers.sort(sicompare);
	return sv->serverinfogui(cgui, servers);
#else
	const char *name = NULL;
	loopv(servers)
	{
		serverinfo &si = servers[i];
        if(cgui->button(si.full, 0xFFFFDD, "server")&G3D_UP) name = si.name;
	}
	return name;
#endif
}

void updatefrommaster()
{
	uchar buf[32000];
	uchar *reply = retrieveservers(buf, sizeof(buf));
	if(!*reply || strstr((char *)reply, "<html>") || strstr((char *)reply, "<HTML>")) conoutf("master server not replying");
	else
	{
		resolverclear();
		loopv(servers) delete[] servers[i].name;
		servers.setsize(0);
		execute((char *)reply);
	}
	refreshservers();
}

COMMAND(addserver, "s");
COMMAND(updatefrommaster, "");

void writeservercfg()
{
#ifdef BFRONTIER // game specific configs
	FILE *f = gameopen("servers.cfg", "w");
#else
	if(!cl->savedservers()) return;
    FILE *f = openfile(cl->savedservers(), "w");
#endif
	if(!f) return;
	fprintf(f, "// servers connected to are added here automatically\n\n");
	loopvrev(servers) fprintf(f, "addserver %s\n", servers[i].name);
	fclose(f);
}

