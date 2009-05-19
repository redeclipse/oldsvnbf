// server.cpp: little more than enhanced multicaster
// runs dedicated or as client coroutine

#include "engine.h"

VAR(version, 1, ENG_VERSION, -1); // for scripts
int kidmode = 0;
ICOMMAND(getkidmode, "", (void), intret(kidmode));

SVARP(consoletimefmt, "%c");
char *gettime(char *format)
{
	time_t ltime;
	struct tm *t;

	ltime = time (NULL);
	t = localtime (&ltime);

	static string buf;
	strftime (buf, sizeof (buf) - 1, format, t);

	return buf;
}
ICOMMAND(gettime, "s", (char *a), result(gettime(a)));

void console(int type, const char *s, ...)
{
	defvformatstring(sf, s, s);
	string osf, psf, fmt;
	formatstring(fmt)(consoletimefmt);
	filtertext(osf, sf);
	formatstring(psf)("%s %s", gettime(fmt), osf);
	printf("%s\n", osf);
	fflush(stdout);
#ifndef STANDALONE
	conline(type, sf, 0);
#endif
}

void conoutft(int type, const char *s, ...)
{
	defvformatstring(sf, s, s);
	console(type, "%s", sf);
#ifdef IRC
	string osf;
	filtertext(osf, sf);
	ircoutf(4, "%s", osf);
#endif
}

void conoutf(const char *s, ...)
{
	defvformatstring(sf, s, s);
	conoutft(0, "%s", sf);
}

VARP(verbose, 0, 0, 6);

#ifdef STANDALONE
void servertoclient(int chan, uchar *buf, int len) {}
void localservertoclient(int chan, uchar *buf, int len) {}
void fatal(const char *s, ...)
{
    void cleanupserver();
    cleanupserver();
    defvformatstring(msg,s,s);
    printf("ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}
int servertype = 3;
#else
VAR(servertype, 0, 1, 3); // 0: local only, 1: private, 2: public, 3: dedicated
#endif
VAR(serveruprate, 0, 0, INT_MAX-1);
VAR(serverclients, 1, 6, MAXCLIENTS);
VAR(serverport, 1, ENG_SERVER_PORT, INT_MAX-1);
VAR(serverqueryport, 1, ENG_QUERY_PORT, INT_MAX-1);
VAR(servermasterport, 1, ENG_MASTER_PORT, INT_MAX-1);
SVAR(servermaster, ENG_MASTER_HOST);
SVAR(serverip, "");

int curtime = 0, totalmillis = 0, lastmillis = 0;
const char *load = NULL;
vector<char *> gameargs;

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

void putfloat(ucharbuf &p, float f)
{
    lilswap(&f, 1);
    p.put((uchar *)&f, sizeof(float));
}

float getfloat(ucharbuf &p)
{
    float f;
    p.get((uchar *)&f, sizeof(float));
    return lilswap(f);
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

vector<clientdata *> clients;

ENetHost *serverhost = NULL;
size_t bsend = 0, brec = 0;
int laststatus = 0;
ENetSocket pongsock = ENET_SOCKET_NULL;

void cleanupserver()
{
	if(serverhost) enet_host_destroy(serverhost);
    serverhost = NULL;
#ifdef MASTERSERVER
	cleanupmaster();
#endif
#ifdef IRC
	irccleanup();
#endif
}

void process(ENetPacket *packet, int sender, int chan);
//void disconnect_client(int n, int reason);

void *getinfo(int i)	{ return !clients.inrange(i) || clients[i]->type==ST_EMPTY ? NULL : clients[i]->info; }
int getnumclients()	 { return clients.length(); }
uint getclientip(int n) { return clients.inrange(n) && clients[n]->type==ST_TCPIP ? clients[n]->peer->address.host : 0; }

void sendpacket(int n, int chan, ENetPacket *packet, int exclude)
{
	if(n < 0)
	{
		server::recordpacket(chan, packet->data, packet->dataLength);
		loopv(clients) if(i != server::peerowner(exclude) && server::allowbroadcast(i)) sendpacket(i, chan, packet, exclude);
		return;
	}
	switch(clients[n]->type)
	{
		case ST_REMOTE:
		{
			int owner = server::peerowner(n);
			if(owner >= 0 && clients.inrange(owner) && owner != n && owner != server::peerowner(exclude))
			{
				//conoutf("redirect %d packet to %d [%d:%d]", n, owner, exclude, server::peerowner(exclude));
				sendpacket(owner, chan, packet, exclude);
			}
			break;
		}
		case ST_TCPIP:
		{
			enet_peer_send(clients[n]->peer, chan, packet);
			bsend += packet->dataLength;
			break;
		}
        case ST_LOCAL:
        {
            localservertoclient(chan, packet->data, (int)packet->dataLength);
            break;
        }
		default: break;
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

        case 'f':
        {
            int n = isdigit(*format) ? *format++-'0' : 1;
            loopi(n) putfloat(p, (float)va_arg(args, double));
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

void sendfile(int cn, int chan, stream *file, const char *format, ...)
{
    if(cn < 0)
    {
#ifdef STANDALONE
            return;
#endif
    }
    else if(!clients.inrange(cn)) return;

    int len = file->size();
    if(len <= 0) return;

    bool reliable = false;
    if(*format=='r') { reliable = true; ++format; }
    ENetPacket *packet = enet_packet_create(NULL, MAXTRANS+len, ENET_PACKET_FLAG_RELIABLE);

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

    file->seek(0, SEEK_SET);
    file->read(&packet->data[p.length()], len);
    enet_packet_resize(packet, p.length()+len);

    if(cn >= 0)
    {
        sendpacket(cn, chan, packet, -1);
        if(!packet->referenceCount) enet_packet_destroy(packet);
    }
#ifndef STANDALONE
    else sendclientpacket(packet, chan);
#endif
}

const char *disc_reasons[] = { "normal", "end of packet", "client num", "kicked/banned", "tag type", "ip is banned", "server is in private mode", "server is full", "connection timed out" };

void disconnect_client(int n, int reason)
{
	if(clients[n]->type!=ST_TCPIP) return;
	enet_peer_disconnect(clients[n]->peer, reason);
	server::clientdisconnect(n);
	clients[n]->type = ST_EMPTY;
	clients[n]->peer->data = NULL;
	server::deleteinfo(clients[n]->info);
	clients[n]->info = NULL;
	defformatstring(s)("client (%s) disconnected because: %s", clients[n]->hostname, disc_reasons[reason]);
	conoutf("\fr%s", s);
	server::srvmsgf(-1, "%s", s);
}

void kicknonlocalclients(int reason)
{
    loopv(clients) if(clients[i]->type==ST_TCPIP) disconnect_client(i, reason);
}

void process(ENetPacket *packet, int sender, int chan)	// sender may be -1
{
	ucharbuf p(packet->data, (int)packet->dataLength);
	server::parsepacket(sender, chan, (packet->flags&ENET_PACKET_FLAG_RELIABLE)!=0, p);
	if(p.overread()) { disconnect_client(sender, DISC_EOP); return; }
}

void localclienttoserver(int chan, ENetPacket *packet)
{
    clientdata *c = NULL;
    loopv(clients) if(clients[i]->type==ST_LOCAL) { c = clients[i]; break; }
    if(c) process(packet, c->num, chan);
}

void delclient(int n)
{
	if(clients.inrange(n))
	{
		if(clients[n]->type==ST_TCPIP) clients[n]->peer->data = NULL;
		clients[n]->type = ST_EMPTY;
		server::deleteinfo(clients[n]->info);
		clients[n]->info = NULL;
	}
}

int addclient(int type)
{
	int n = -1;
	loopv(clients) if(clients[i]->type==ST_EMPTY)
	{
		n = i;
		break;
	}
	if(!clients.inrange(n))
	{
		clientdata *c = new clientdata;
		n = c->num = clients.length();
		clients.add(c);
	}
	clients[n]->info = server::newinfo();
	clients[n]->type = type;
	return n;
}

#ifndef STANDALONE
VARP(autoconnect, 0, 0, 1);
extern bool connectedlocally;
extern char *lastaddress;
void localconnect(bool force)
{
	if((!connected() || !connectedlocally) && (force || autoconnect))
	{
		if(lastaddress) delete[] lastaddress;
		lastaddress = NULL;
		int cn = addclient(ST_LOCAL);
		clientdata &c = *clients[cn];
		c.peer = NULL;
		copystring(c.hostname, "<local>");
		conoutf("\fglocal client %d connected", c.num);
		client::gameconnect(false);
		server::clientconnect(c.num, 0, true);
		connectedlocally = true;
	}
}

void localdisconnect()
{
    loopv(clients) if(clients[i] && clients[i]->type==ST_LOCAL)
    {
        clientdata &c = *clients[i];
        conoutf("\folocal client %d disconnected", c.num);
        server::clientdisconnect(c.num, true);
        c.type = ST_EMPTY;
        server::deleteinfo(c.info);
        c.info = NULL;
		connectedlocally = false;
    }
}
#endif

static ENetAddress pongaddr;

void sendqueryreply(ucharbuf &p)
{
    ENetBuffer buf;
    buf.data = p.buf;
    buf.dataLength = p.length();
    enet_socket_send(pongsock, &pongaddr, &buf, 1);
}

void sendpongs()		// reply all server info requests
{
	ENetBuffer buf;
	uchar pong[MAXTRANS];
	int len;
	enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
	buf.data = pong;
	while(enet_socket_wait(pongsock, &events, 0) >= 0 && events)
	{
		buf.dataLength = sizeof(pong);
        len = enet_socket_receive(pongsock, &pongaddr, &buf, 1);
		if(len < 0) return;
        ucharbuf req(pong, len), p(pong, sizeof(pong));
        p.len += len;
        server::queryreply(req, p);
	}
}

#ifdef STANDALONE
bool resolverwait(const char *name, int port, ENetAddress *address)
{
	return enet_address_set_host(address, name) >= 0;
}

int connectwithtimeout(ENetSocket sock, const char *hostname, ENetAddress &remoteaddress)
{
	int result = enet_socket_connect(sock, &remoteaddress);
	if(result<0) enet_socket_destroy(sock);
	return result;
}
#endif

#ifndef STANDALONE
ENetSocket mastersend(ENetAddress &remoteaddress, const char *hostname, const char *req, ENetAddress *localaddress = NULL)
{
	if(remoteaddress.host==ENET_HOST_ANY)
	{
		conoutf("\falooking up %s:[%d]...", hostname, remoteaddress.port);
		if(!resolverwait(hostname, remoteaddress.port, &remoteaddress)) return ENET_SOCKET_NULL;
	}
    ENetSocket sock = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
    if(sock!=ENET_SOCKET_NULL && localaddress && enet_socket_bind(sock, localaddress) < 0)
    {
        enet_socket_destroy(sock);
        sock = ENET_SOCKET_NULL;
    }
	if(sock==ENET_SOCKET_NULL || connectwithtimeout(sock, hostname, remoteaddress)<0)
	{
		conoutf(sock==ENET_SOCKET_NULL ? "could not open socket to %s:[%d]" : "could not connect to %s:[%d]", hostname, remoteaddress.port);
		return ENET_SOCKET_NULL;
	}
	ENetBuffer buf;
	defformatstring(mget)("%s\n", req);
	buf.data = mget;
	buf.dataLength = strlen((char *)buf.data);
	conoutf("\fasending request to %s:[%d]", hostname, remoteaddress.port);
	enet_socket_send(sock, NULL, &buf, 1);
	return sock;
}

bool masterreceive(ENetSocket sock, ENetBuffer &buf, int timeout = 0)
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

#define RETRIEVELIMIT 20000
uchar *retrieveservers(uchar *buf, int buflen)
{
	buf[0] = '\0';
	ENetAddress address = { ENET_HOST_ANY, servermasterport };
	ENetSocket sock = mastersend(address, servermaster, "list");
	if(sock==ENET_SOCKET_NULL) return buf;
	/* only cache this if connection succeeds */
	defformatstring(text)("retrieving servers from %s:[%d] (esc to abort)", servermaster, address.port);
	renderprogress(0, text);

	ENetBuffer eb;
	eb.data = buf;
	eb.dataLength = buflen-1;

	int starttime = SDL_GetTicks(), timeout = 0;
	while(masterreceive(sock, eb, 250))
	{
		timeout = SDL_GetTicks() - starttime;
        renderprogress(min(float(timeout)/RETRIEVELIMIT, 1.0f), text);
        if(interceptkey(SDLK_ESCAPE)) timeout = RETRIEVELIMIT + 1;
		if(timeout > RETRIEVELIMIT)
		{
			buf[0] = '\0';
			enet_socket_destroy(sock);
			return buf;
		}
	}

	return buf;
}
#endif

void serverslice()	// main server update, called from main loop in sp, or from below in dedicated server
{
    server::serverupdate();

	if(!serverhost) return;

	sendpongs();

	if(servertype >= 2 && totalmillis-laststatus > 60*1000)	// display bandwidth stats, useful for server ops
	{
		laststatus = totalmillis;
		if(bsend || brec || server::numclients(-1, false, true))
			conoutf("\fmstatus: %d clients, %.1f send, %.1f rec (K/sec)", server::numclients(-1, false, true), bsend/60.0f/1024, brec/60.0f/1024);
		bsend = brec = 0;
	}

	ENetEvent event;
    bool serviced = false;
    while(!serviced)
    {
        if(enet_host_check_events(serverhost, &event) <= 0)
        {
            if(enet_host_service(serverhost, &event, 0) <= 0) break;
            serviced = true;
        }
		switch(event.type)
		{
			case ENET_EVENT_TYPE_CONNECT:
			{
				int cn = addclient(ST_TCPIP);
				clientdata &c = *clients[cn];
				c.peer = event.peer;
				c.peer->data = &c;
				char hn[1024];
				copystring(c.hostname, (enet_address_get_host_ip(&c.peer->address, hn, sizeof(hn))==0) ? hn : "unknown");
				conoutf("\fgclient connected (%s)", c.hostname);
				int reason = server::clientconnect(c.num, c.peer->address.host);
				if(reason) disconnect_client(c.num, reason);
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
			{
				brec += event.packet->dataLength;
				clientdata *c = (clientdata *)event.peer->data;
				if(c) process(event.packet, c->num, event.channelID);
				if(event.packet->referenceCount==0) enet_packet_destroy(event.packet);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT:
			{
				clientdata *c = (clientdata *)event.peer->data;
				if(!c) break;
				conoutf("\fodisconnected client (%s)", c->hostname);
				server::clientdisconnect(c->num);
				c->type = ST_EMPTY;
				event.peer->data = NULL;
				server::deleteinfo(c->info);
				c->info = NULL;
				break;
			}
			default:
				break;
		}
    }
	if(server::sendpackets()) enet_host_flush(serverhost);
}

void serverloop()
{
	#ifdef WIN32
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	#endif
	conoutf("\fgdedicated server started, waiting for clients... [Ctrl-C to exit]");
	for(;;)
	{
		int _lastmillis = lastmillis;
		lastmillis = totalmillis = (int)enet_time_get();
		curtime = lastmillis-_lastmillis;

#ifdef MASTERSERVER
		checkmaster();
#endif
		serverslice();
#ifdef IRC
		ircslice();
#endif

		if((int)enet_time_get()-lastmillis <= 0)
		{
			#ifdef WIN32
			Sleep(1);
			#else
			usleep(1000);
			#endif
		}
	}
	exit(EXIT_SUCCESS);
}

void setupserver()
{
    server::changemap(load && *load ? load : NULL);

    if(!servertype) return;

#ifdef MASTERSERVER
    setupmaster();
#endif

	conoutf("\fminit: server (%s:%d)", *serverip ? serverip : "*", serverport);
	ENetAddress address = { ENET_HOST_ANY, serverport };
	if(*serverip)
	{
		if(enet_address_set_host(&address, serverip) < 0) conoutf("\frWARNING: server address not resolved");
	}
	serverhost = enet_host_create(&address, serverclients + server::reserveclients(), 0, serveruprate);
	if(!serverhost)
	{
		conoutf("\frcould not create server socket");
#ifndef STANDALONE
		setvar("servertype", 0);
#endif
		return;
	}
	loopi(serverclients) serverhost->peers[i].data = NULL;

	address.port = serverqueryport;
    pongsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if(pongsock != ENET_SOCKET_NULL && enet_socket_bind(pongsock, &address) < 0)
    {
        enet_socket_destroy(pongsock);
        pongsock = ENET_SOCKET_NULL;
    }
	if(pongsock == ENET_SOCKET_NULL)
	{
		conoutf("\frcould not create server info socket, publicity disabled");
#ifndef STANDALONE
		setvar("servertype", 1);
#endif
		}
	else
	{
		enet_socket_set_option(pongsock, ENET_SOCKOPT_NONBLOCK, 1);
	}

	if(verbose) conoutf("\fggame server started");

#ifndef STANDALONE
	if(servertype >= 3) serverloop();
#endif
}

void initgame()
{
	server::start();
#ifndef STANDALONE
	game::start();
#endif
	loopv(gameargs)
	{
#ifndef STANDALONE
		if(game::clientoption(gameargs[i])) continue;
#endif
		if(server::serveroption(gameargs[i])) continue;
		conoutf("\frunknown command-line option: %s", gameargs[i]);
	}
	execfile("autoserv.cfg", false);
    setupserver();
}

VAR(hasoctapaks, 1, 0, 0); // mega hack; try to find Cube 2, done after our own data so as to not clobber stuff
SVARP(octadir, "");//, if(!hasoctapaks) trytofindocta(false););

bool serveroption(char *opt)
{
	switch(opt[1])
	{
		case 'k': kidmode = atoi(opt+2); return true;
		case 'h': printf("Using home directory: %s\n", &opt[2]); sethomedir(&opt[2]); return true;
		case 'o': setsvar("octadir", &opt[2]); return true;
		case 'p': printf("Adding package directory: %s\n", &opt[2]); addpackagedir(&opt[2]); return true;
		case 'v': setvar("verbose", atoi(opt+2)); return true;
		case 's':
		{
			switch(opt[2])
			{
				case 'u': setvar("serveruprate", atoi(opt+3)); return true;
				case 'c': setvar("serverclients", atoi(opt+3)); return true;
				case 'i': setsvar("serverip", opt+3); return true;
				case 'm': setsvar("servermaster", opt+3); return true;
				case 'l': load = opt+3; return true;
#ifndef STANDALONE
				case 's': setvar("servertype", atoi(opt+3)); return true;
#endif
				case 'p': setvar("serverport", atoi(opt+3)); return true;
				case 'q': setvar("serverqueryport", atoi(opt+3)); return true;
				case 'a': setvar("servermasterport", atoi(opt+3)); return true;
			}
		}
#ifdef MASTERSERVER
		case 'm':
		{
			switch(opt[2])
			{
				case 's': setvar("masterserver", atoi(opt+3)); return true;
				case 'i': setsvar("masterip", opt+3); return true;
				case 'p': setvar("masterport", atoi(opt+3)); return true;
				default: return false;
			}
			return false;
		}
#endif
		default: return false;
	}
	return false;
}

bool findoctadir(const char *name, bool fallback)
{
	defformatstring(octalogo)("%s/data/default_map_settings.cfg", name);
	if(fileexists(findfile(octalogo, "r"), "r"))
	{
		conoutf("\fgfound octa directory: %s", name);
		defformatstring(octadata)("%s/data", name);
		defformatstring(octapaks)("%s/packages", name);
		addpackagedir(name);
		addpackagedir(octadata);
		addpackagedir(octapaks);
		hasoctapaks = fallback ? 1 : 2;
		return true;
	}
	return false;
}
void trytofindocta(bool fallback)
{
	if(!octadir || !*octadir)
	{
		const char *dir = getenv("OCTA_DIR");
		if(dir && *dir) setsvar("octadir", dir);
	}
	if((!octadir || !*octadir || !findoctadir(octadir, false)) && fallback)
	{ // user hasn't specifically set it, try some common locations alongside our folder
		const char *tryoctadirs[4] = { // by no means a definitive list either..
			"../Sauerbraten", "../sauerbraten", "../sauer",
#if defined(WIN32)
			"/Program Files/Sauerbraten"
#elif defined(__APPLE__)
			"/Volumes/Play/sauerbraten"
#else
			"/usr/games/sauerbraten"
#endif
		};
		loopi(4) if(findoctadir(tryoctadirs[i], true)) break;
	}
}

#ifdef STANDALONE
int main(int argc, char* argv[])
{
	addpackagedir("data");
	for(int i = 1; i<argc; i++) if(argv[i][0]!='-' || !serveroption(argv[i])) gameargs.add(argv[i]);
	if(enet_initialize()<0) fatal("Unable to initialise network module");
	atexit(enet_deinitialize);
	atexit(cleanupserver);
	enet_time_set(0);
	initgame();
	trytofindocta();
	serverloop();
	return 0;
}
#endif

