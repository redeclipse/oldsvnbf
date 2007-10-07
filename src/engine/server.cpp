// server.cpp: little more than enhanced multicaster
// runs dedicated or as client coroutine

#include "pch.h"

#ifdef STANDALONE
#ifdef BFRONTIER // better minimal definitions
#include "minimal.h"
int lastmillis = 0, totalmillis = 0, verbose = 0;
void conoutf(const char *s, ...) { s_sprintfdlv(str, s, s); printf("%s\n", str); }
void console(const char *s, int n, ...) { s_sprintfdlv(str, n, s); printf("%s\n", str); }
#else
#include "cube.h"
#include "iengine.h"
#include "igame.h"
#endif
void localservertoclient(int chan, uchar *buf, int len) {}
void fatal(char *s, char *o) { void cleanupserver(); cleanupserver(); printf("servererror: %s\n", s); exit(EXIT_FAILURE); }
#else
#include "engine.h"
#endif

#ifdef BFRONTIER // moved up here so we can use them, and others removed because of external defs
#define DEFAULTCLIENTS 6
bool pubserv = false;
int uprate = 0, maxclients = DEFAULTCLIENTS;
char *ip = "", *master = NULL;
char *game = "fps";
ibotcom *bc = NULL;
iphysics *ph = NULL;
#endif
igameclient *cl = NULL;
igameserver *sv = NULL;
iclientcom *cc = NULL;
icliententities *et = NULL;

hashtable<char *, igame *> *gamereg = NULL;

vector<char *> gameargs;

void registergame(char *name, igame *ig)
{
	if(!gamereg) gamereg = new hashtable<char *, igame *>;
	(*gamereg)[name] = ig;
}

void initgame(char *game)
{
	igame **ig = gamereg->access(game);
	if(!ig) fatal("cannot start game module: ", game);
	sv = (*ig)->newserver();
	cl = (*ig)->newclient();
	if(cl)
	{
		cc = cl->getcom();
#ifdef BFRONTIER // extra sub modules
		bc = cl->getbot();
		ph = cl->getphysics();
#endif
		et = cl->getents();
		cl->initclient();
	}
	loopv(gameargs)
	{
		if(!cl || !cl->clientoption(gameargs[i]))
		{
			if(!sv->serveroption(gameargs[i])) 
#ifdef STANDALONE
				printf("unknown command-line option: %s\n", gameargs[i]);
#else
				conoutf("unknown command-line option: %s", gameargs[i]);
#endif
		}
	}
}

// all network traffic is in 32bit ints, which are then compressed using the following simple scheme (assumes that most values are small).

void putint(ucharbuf &p, int n)
{
	if(n<128 && n>-127) p.put(n);
	else if(n<0x8000 && n>=-0x8000) { p.put(0x80); p.put(n); p.put(n>>8); }
	else { p.put(0x81); p.put(n); p.put(n>>8); p.put(n>>16); p.put(n>>24); }
}

int getint(ucharbuf &p)
{
	int c = (char)p.get();
	if(c==-128) { int n = p.get(); n |= char(p.get())<<8; return n; }
	else if(c==-127) { int n = p.get(); n |= p.get()<<8; n |= p.get()<<16; return n|(p.get()<<24); } 
	else return c;
}

// much smaller encoding for unsigned integers up to 28 bits, but can handle signed
void putuint(ucharbuf &p, int n)
{
	if(n < 0 || n >= (1<<21))
	{
		p.put(0x80 | (n & 0x7F));
		p.put(0x80 | ((n >> 7) & 0x7F));
		p.put(0x80 | ((n >> 14) & 0x7F));
		p.put(n >> 21);
	}
	else if(n < (1<<7)) p.put(n);
	else if(n < (1<<14))
	{
		p.put(0x80 | (n & 0x7F));
		p.put(n >> 7);
	}
	else 
	{ 
		p.put(0x80 | (n & 0x7F)); 
		p.put(0x80 | ((n >> 7) & 0x7F));
		p.put(n >> 14); 
	}
}

int getuint(ucharbuf &p)
{
	int n = p.get();
	if(n & 0x80)
	{
		n += (p.get() << 7) - 0x80;
		if(n & (1<<14)) n += (p.get() << 14) - (1<<14);
		if(n & (1<<21)) n += (p.get() << 21) - (1<<21);
		if(n & (1<<28)) n |= 0xF0000000; 
	}
	return n;
}

void sendstring(const char *t, ucharbuf &p)
{
	while(*t) putint(p, *t++);
	putint(p, 0);
}

void getstring(char *text, ucharbuf &p, int len)
{
	char *t = text;
	do
	{
		if(t>=&text[len]) { text[len-1] = 0; return; }
		if(!p.remaining()) { *t = 0; return; } 
		*t = getint(p);
	}
	while(*t++);
}

void filtertext(char *dst, const char *src, bool whitespace, int len)
{
	for(int c = *src; c; c = *++src)
	{
		switch(c)
		{
		case '\f': ++src; continue;
		}
		if(isspace(c) ? whitespace : isprint(c))
		{
			*dst++ = c;
			if(!--len) break;
		}
	}
	*dst = '\0';
}

#ifndef BFRONTIER // moved to iengine.h
enum { ST_EMPTY, ST_LOCAL, ST_TCPIP };

struct client					// server side version of "dynent" type
{
	int type;
	int num;
	ENetPeer *peer;
	string hostname;
	void *info;
};
#endif

vector<client *> clients;

ENetHost *serverhost = NULL;
size_t bsend = 0, brec = 0;
int laststatus = 0; 
ENetSocket pongsock = ENET_SOCKET_NULL;

void cleanupserver()
{
	if(serverhost) enet_host_destroy(serverhost);
}

void sendfile(int cn, int chan, FILE *file, const char *format, ...)
{
#ifndef STANDALONE
	extern ENetHost *clienthost;
#endif
	if(cn < 0)
	{
#ifndef STANDALONE
		if(!clienthost || clienthost->peers[0].state != ENET_PEER_STATE_CONNECTED) 
#endif
			return;
	}
	else if(cn >= clients.length() || clients[cn]->type != ST_TCPIP) return;

	fseek(file, 0, SEEK_END);
	int len = ftell(file);
	bool reliable = false;
	if(*format=='r') { reliable = true; ++format; }
	ENetPacket *packet = enet_packet_create(NULL, MAXTRANS+len, ENET_PACKET_FLAG_RELIABLE);
	rewind(file);

	ucharbuf p(packet->data, packet->dataLength);
	va_list args;
	va_start(args, format);
	while(*format) switch(*format++)
	{
		case 'i':
		{
			int n = isdigit(*format) ? *format++-'0' : 1;
			loopi(n) putint(p, va_arg(args, int));
			break;
		}
		case 's': sendstring(va_arg(args, const char *), p); break;
		case 'l': putint(p, len); break;
	}
	va_end(args);
	enet_packet_resize(packet, p.length()+len);

	fread(&packet->data[p.length()], 1, len, file);
	enet_packet_resize(packet, p.length()+len);

	if(cn >= 0) enet_peer_send(clients[cn]->peer, chan, packet);
#ifndef STANDALONE
	else enet_peer_send(&clienthost->peers[0], chan, packet);
#endif

	if(!packet->referenceCount) enet_packet_destroy(packet);
}

void process(ENetPacket *packet, int sender, int chan);
//void disconnect_client(int n, int reason);

void *getinfo(int i)	{ return !clients.inrange(i) || clients[i]->type==ST_EMPTY ? NULL : clients[i]->info; }
int getnumclients()	 { return clients.length(); }
uint getclientip(int n) { return clients.inrange(n) && clients[n]->type==ST_TCPIP ? clients[n]->peer->address.host : 0; }

void sendpacket(int n, int chan, ENetPacket *packet, int exclude)
{
	if(n<0)
	{
		sv->recordpacket(chan, packet->data, packet->dataLength);
		loopv(clients) if(i!=exclude) sendpacket(i, chan, packet);
		return;
	}
	switch(clients[n]->type)
	{
		case ST_TCPIP:
		{
			enet_peer_send(clients[n]->peer, chan, packet);
			bsend += packet->dataLength;
			break;
		}
		case ST_LOCAL:
			localservertoclient(chan, packet->data, (int)packet->dataLength);
			break;
	}
}

void sendf(int cn, int chan, const char *format, ...)
{
	int exclude = -1;
	bool reliable = false;
	if(*format=='r') { reliable = true; ++format; }
	ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
	ucharbuf p(packet->data, packet->dataLength);
	va_list args;
	va_start(args, format);
	while(*format) switch(*format++)
	{
		case 'x':
			exclude = va_arg(args, int);
			break;

		case 'v':
		{
			int n = va_arg(args, int);
			int *v = va_arg(args, int *);
			loopi(n) putint(p, v[i]);
			break;
		}

		case 'i': 
		{
			int n = isdigit(*format) ? *format++-'0' : 1;
			loopi(n) putint(p, va_arg(args, int));
			break;
		}
		case 's': sendstring(va_arg(args, const char *), p); break;
		case 'm':
		{
			int n = va_arg(args, int);
			enet_packet_resize(packet, packet->dataLength+n);
			p.buf = packet->data;
			p.maxlen += n;
			p.put(va_arg(args, uchar *), n);
			break;
		}
	}
	va_end(args);
	enet_packet_resize(packet, p.length());
	sendpacket(cn, chan, packet, exclude);
	if(packet->referenceCount==0) enet_packet_destroy(packet);
}

char *disc_reasons[] = { "normal", "end of packet", "client num", "kicked/banned", "tag type", "ip is banned", "server is in private mode", "server FULL (maxclients)" };

void disconnect_client(int n, int reason)
{
	if(clients[n]->type!=ST_TCPIP) return;
	s_sprintfd(s)("client (%s) disconnected because: %s\n", clients[n]->hostname, disc_reasons[reason]);
	puts(s);
	enet_peer_disconnect(clients[n]->peer, reason);
	sv->clientdisconnect(n);
	clients[n]->type = ST_EMPTY;
	clients[n]->peer->data = NULL;
	sv->deleteinfo(clients[n]->info);
	clients[n]->info = NULL;
	sv->sendservmsg(s);
}

void process(ENetPacket *packet, int sender, int chan)	// sender may be -1
{
	ucharbuf p(packet->data, (int)packet->dataLength);
	sv->parsepacket(sender, chan, (packet->flags&ENET_PACKET_FLAG_RELIABLE)!=0, p);
	if(p.overread()) { disconnect_client(sender, DISC_EOP); return; }
}

void send_welcome(int n)
{
	ENetPacket *packet = enet_packet_create (NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
	ucharbuf p(packet->data, packet->dataLength);
	int chan = sv->welcomepacket(p, n);
	enet_packet_resize(packet, p.length());
	sendpacket(n, chan, packet);
	if(packet->referenceCount==0) enet_packet_destroy(packet);
}

void localclienttoserver(int chan, ENetPacket *packet)
{
	process(packet, 0, chan);
	if(packet->referenceCount==0) enet_packet_destroy(packet);
}

client &addclient()
{
	loopv(clients) if(clients[i]->type==ST_EMPTY)
	{
		clients[i]->info = sv->newinfo();
		return *clients[i];
	}
	client *c = new client;
	c->num = clients.length();
	c->info = sv->newinfo();
	clients.add(c);
	return *c;
}

int localclients = 0, nonlocalclients = 0;

bool hasnonlocalclients() { return nonlocalclients!=0; }
bool haslocalclients() { return localclients!=0; }

void sendpongs()		// reply all server info requests
{
	ENetBuffer buf;
	ENetAddress addr;
	uchar pong[MAXTRANS];
	int len;
	enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
	buf.data = pong;
	while(enet_socket_wait(pongsock, &events, 0) >= 0 && events)
	{
		buf.dataLength = sizeof(pong);
		len = enet_socket_receive(pongsock, &addr, &buf, 1);
		if(len < 0) return;
		ucharbuf p(&pong[len], sizeof(pong)-len);
		sv->serverinforeply(p);
		buf.dataLength = len + p.length();
		enet_socket_send(pongsock, &addr, &buf, 1);
	}
}	  

#ifdef STANDALONE
bool resolverwait(const char *name, ENetAddress *address)
{
	return enet_address_set_host(address, name) >= 0;
}

int connectwithtimeout(ENetSocket sock, char *hostname, ENetAddress &remoteaddress)
{
	int result = enet_socket_connect(sock, &remoteaddress);
	if(result<0) enet_socket_destroy(sock);
	return result;
}
#endif

ENetSocket httpgetsend(ENetAddress &remoteaddress, char *hostname, char *req, char *ref, char *agent, ENetAddress *localaddress = NULL)
{
	if(remoteaddress.host==ENET_HOST_ANY)
	{
#ifdef STANDALONE
		printf("looking up %s...\n", hostname);
#endif
		if(!resolverwait(hostname, &remoteaddress)) return ENET_SOCKET_NULL;
	}
	ENetSocket sock = enet_socket_create(ENET_SOCKET_TYPE_STREAM, localaddress);
	if(sock==ENET_SOCKET_NULL || connectwithtimeout(sock, hostname, remoteaddress)<0) 
	{ 
#ifdef STANDALONE
		printf(sock==ENET_SOCKET_NULL ? "could not open socket\n" : "could not connect\n"); 
#endif
		return ENET_SOCKET_NULL; 
	}
	ENetBuffer buf;
	s_sprintfd(httpget)("GET %s HTTP/1.0\nHost: %s\nReferer: %s\nUser-Agent: %s\n\n", req, hostname, ref, agent);
	buf.data = httpget;
	buf.dataLength = strlen((char *)buf.data);
#ifdef STANDALONE
#ifdef BFRONTIER // verbose
	printf("sending request to %s...\n\n%s", hostname, httpget);
#else
	printf("sending request to %s...\n", hostname);
#endif
#endif
	enet_socket_send(sock, NULL, &buf, 1);
	return sock;
}  

bool httpgetreceive(ENetSocket sock, ENetBuffer &buf, int timeout = 0)
{
	if(sock==ENET_SOCKET_NULL) return false;
	enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
	if(enet_socket_wait(sock, &events, timeout) >= 0 && events)
	{
		int len = enet_socket_receive(sock, NULL, &buf, 1);
		if(len<=0)
		{
			enet_socket_destroy(sock);
			return false;
		}
		buf.data = ((char *)buf.data)+len;
		((char*)buf.data)[0] = 0;
		buf.dataLength -= len;
	}
	return true;
}  

uchar *stripheader(uchar *b)
{
	char *s = strstr((char *)b, "\n\r\n");
	if(!s) s = strstr((char *)b, "\n\n");
	return s ? (uchar *)s : b;
}

ENetSocket mssock = ENET_SOCKET_NULL;
ENetAddress msaddress = { ENET_HOST_ANY, ENET_PORT_ANY };
ENetAddress masterserver = { ENET_HOST_ANY, 80 };
int lastupdatemaster = 0;
string masterbase;
string masterpath;
#ifdef BFRONTIER // enhanced master server support
int mastertype = 0;
#endif
uchar masterrep[MAXTRANS];
ENetBuffer masterb;

void updatemasterserver()
{
#ifdef BFRONTIER // enhanced master server support
	string path;
	if (mastertype > 0) { s_sprintf(path)("%s?game=%s&action=register", game, masterpath); }
	else { s_sprintf(path)("%sregister.do?action=add", masterpath); }
#else
    s_sprintfd(path)("%sregister.do?action=add", masterpath);
#endif
	if(mssock!=ENET_SOCKET_NULL) enet_socket_destroy(mssock);
	mssock = httpgetsend(masterserver, masterbase, path, sv->servername(), sv->servername(), &msaddress);
	masterrep[0] = 0;
	masterb.data = masterrep;
	masterb.dataLength = MAXTRANS-1;
} 

void checkmasterreply()
{
	if(mssock!=ENET_SOCKET_NULL && !httpgetreceive(mssock, masterb))
	{
		mssock = ENET_SOCKET_NULL;
		printf("masterserver reply: %s\n", stripheader(masterrep));
	}
} 

#ifndef STANDALONE

#define RETRIEVELIMIT 20000

uchar *retrieveservers(uchar *buf, int buflen)
{
	buf[0] = '\0';
#ifdef BFRONTIER // enhanced master server support
	string path;
	if (mastertype > 0) { s_sprintf(path)("%s?game=%s&action=list", game, masterpath); }
	else { s_sprintf(path)("%sretrieve.do?item=list", masterpath); }
#else
    s_sprintfd(path)("%sretrieve.do?item=list", masterpath);
#endif
	ENetAddress address = masterserver;
	ENetSocket sock = httpgetsend(address, masterbase, path, sv->servername(), sv->servername());
	if(sock==ENET_SOCKET_NULL) return buf;
	/* only cache this if connection succeeds */
	masterserver = address;

	s_sprintfd(text)("retrieving servers from %s... (esc to abort)", masterbase);
	show_out_of_renderloop_progress(0, text);

	ENetBuffer eb;
	eb.data = buf;
	eb.dataLength = buflen-1;
	
	int starttime = SDL_GetTicks(), timeout = 0;
	while(httpgetreceive(sock, eb, 250))
	{
		timeout = SDL_GetTicks() - starttime;
		show_out_of_renderloop_progress(min(float(timeout)/RETRIEVELIMIT, 1), text);
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) timeout = RETRIEVELIMIT + 1;
		}
		if(timeout > RETRIEVELIMIT)
		{
			buf[0] = '\0';
			enet_socket_destroy(sock);
			return buf;
		}
	}

	return stripheader(buf);
}
#endif
#ifndef BFRONTIER // moved up
#define DEFAULTCLIENTS 6

int uprate = 0, maxclients = DEFAULTCLIENTS;
char *ip = "", *master = NULL;
char *game = "fps";

#ifdef STANDALONE
int lastmillis = 0, totalmillis = 0;
#endif
#endif
void serverslice(uint timeout)	// main server update, called from main loop in sp, or from below in dedicated server
{
	localclients = nonlocalclients = 0;
	loopv(clients) switch(clients[i]->type)
	{
		case ST_LOCAL: localclients++; break;
		case ST_TCPIP: nonlocalclients++; break;
	}

	if(!serverhost) 
	{
#ifndef BFRONTIER // local servers
		sv->serverupdate(lastmillis, totalmillis);
		sv->sendpackets();
#endif
		return;
	}
		
	// below is network only

#if !defined(BFRONTIER) || defined(STANDALONE) // local servers
	lastmillis = totalmillis = (int)enet_time_get();
#endif
	sv->serverupdate(lastmillis, totalmillis);

	sendpongs();
	
#ifdef BFRONTIER // local servers
	if (pubserv)
	{
#endif
		if(*masterpath) checkmasterreply();
	
		if(totalmillis-lastupdatemaster>60*60*1000 && *masterpath)		// send alive signal to masterserver every hour of uptime
		{
			updatemasterserver();
			lastupdatemaster = totalmillis;
		}
		
		if(totalmillis-laststatus>60*1000)	// display bandwidth stats, useful for server ops
		{
			laststatus = totalmillis;	 
			if(nonlocalclients || bsend || brec) printf("status: %d remote clients, %.1f send, %.1f rec (K/sec)\n", nonlocalclients, bsend/60.0f/1024, brec/60.0f/1024);
			bsend = brec = 0;
		}
#ifdef BFRONTIER // local servers
	}
#endif

	ENetEvent event;
    bool serviced = false;
    while(!serviced)
    {
        if(enet_host_check_events(serverhost, &event) <= 0)
        {
            if(enet_host_service(serverhost, &event, timeout) <= 0) break;
            serviced = true;
        }
		switch(event.type)
		{
			case ENET_EVENT_TYPE_CONNECT:
			{
				client &c = addclient();
				c.type = ST_TCPIP;
				c.peer = event.peer;
				c.peer->data = &c;
				char hn[1024];
				s_strcpy(c.hostname, (enet_address_get_host_ip(&c.peer->address, hn, sizeof(hn))==0) ? hn : "unknown");
				printf("client connected (%s)\n", c.hostname);
				int reason = DISC_MAXCLIENTS;
				if(nonlocalclients<maxclients && !(reason = sv->clientconnect(c.num, c.peer->address.host))) 
				{
					nonlocalclients++;
					send_welcome(c.num);
				}
				else disconnect_client(c.num, reason);
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
			{
				brec += event.packet->dataLength;
				client *c = (client *)event.peer->data;
				if(c) process(event.packet, c->num, event.channelID);
				if(event.packet->referenceCount==0) enet_packet_destroy(event.packet);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT: 
			{
				client *c = (client *)event.peer->data;
				if(!c) break;
				printf("disconnected client (%s)\n", c->hostname);
				sv->clientdisconnect(c->num);
				nonlocalclients--;
				c->type = ST_EMPTY;
				event.peer->data = NULL;
				sv->deleteinfo(c->info);
				c->info = NULL;
				break;
			}
			default:
				break;
		}
    }
	if(sv->sendpackets()) enet_host_flush(serverhost);
}

void localdisconnect()
{
	loopv(clients) if(clients[i]->type==ST_LOCAL) 
	{
		sv->localdisconnect(i);
		localclients--;
		clients[i]->type = ST_EMPTY;
		sv->deleteinfo(clients[i]->info);
		clients[i]->info = NULL;
	}
}

void localconnect()
{
#ifdef BFRONTIER // local server support
#ifndef STANDALONE
	if (sv->serverport()) connects("localhost");
#endif
#else
	client &c = addclient();
	c.type = ST_LOCAL;
	s_strcpy(c.hostname, "local");
	localclients++;
	sv->localconnect(c.num);
	send_welcome(c.num); 
#endif
}

void initserver(bool dedicated)
{
	initgame(game);

#ifdef BFRONTIER // local servers, enhanced master server support
	pubserv = dedicated;
	mastertype = sv->getmastertype();
#endif
	if(!master) master = sv->getdefaultmaster();
	char *mid = strstr(master, "/");
	if(!mid) mid = master;
	s_strcpy(masterpath, mid);
	s_strncpy(masterbase, master, mid-master+1);

#ifndef BFRONTIER // local server support
	if(dedicated)
	{
#endif
		ENetAddress address = { ENET_HOST_ANY, sv->serverport() };
		if(*ip)
		{
			if(enet_address_set_host(&address, ip)<0) printf("WARNING: server ip not resolved");
			else msaddress.host = address.host;
		}
		serverhost = enet_host_create(&address, maxclients+1, 0, uprate);
		if(!serverhost) fatal("could not create server host");
		loopi(maxclients) serverhost->peers[i].data = NULL;
		address.port = sv->serverinfoport();
		pongsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM, &address);
		if(pongsock == ENET_SOCKET_NULL) fatal("could not create server info socket");
        else enet_socket_set_option(pongsock, ENET_SOCKOPT_NONBLOCK, 1);
#ifndef BFRONTIER // local server support
	}
#endif

	sv->serverinit();

	if(dedicated)		// do not return, this becomes main loop
	{
#if !defined(BFRONTIER) || defined(STANDALONE) // local server support
		#ifdef WIN32
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
		#endif
		printf("dedicated server started, waiting for clients...\nCtrl-C to exit\n\n");
		atexit(enet_deinitialize);
		atexit(cleanupserver);
		enet_time_set(0);
		if(*masterpath) updatemasterserver();
		for(;;) serverslice(5);
#else
		if(*masterpath) updatemasterserver();
#endif
	}
}

bool serveroption(char *opt)
{
	switch(opt[1])
	{
		case 'u': uprate = atoi(opt+2); return true;
		case 'c': 
		{
			int clients = atoi(opt+2); 
			if(clients > 0) maxclients = min(clients, MAXCLIENTS);
			else maxclients = DEFAULTCLIENTS;
			return true;
		}
		case 'i': ip = opt+2; return true;
		case 'm': master = opt+2; return true;
		case 'g': game = opt+2; return true;
		default: return false;
	}
}

#ifdef STANDALONE
int main(int argc, char* argv[])
{
	for(int i = 1; i<argc; i++) if(argv[i][0]!='-' || !serveroption(argv[i])) gameargs.add(argv[i]);
	if(enet_initialize()<0) fatal("Unable to initialise network module");
	initserver(true);
	return 0;
}
#endif
