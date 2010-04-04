// server.cpp: little more than enhanced multicaster
// runs dedicated or as client coroutine

#include "engine.h"
#ifdef WIN32
#include <shlobj.h>
#endif

VAR(0, version, 1, ENG_VERSION, -1); // for scripts
int kidmode = 0;
ICOMMAND(0, getkidmode, "", (void), intret(kidmode));

const char *disc_reasons[] = { "normal", "end of packet", "client num", "user was banned", "tag type error", "address is banned", "server is in private mode", "server is full", "connection timed out", "packet overflow" };

SVAR(IDF_PERSIST, consoletimefmt, "%c");
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
ICOMMAND(0, gettime, "s", (char *a), result(gettime(a)));

vector<ipinfo> bans, allows;
void addipinfo(vector<ipinfo> &info, const char *name)
{
    union { uchar b[sizeof(enet_uint32)]; enet_uint32 i; } ip, mask;
    ip.i = 0;
    mask.i = 0;
    loopi(4)
    {
        char *end = NULL;
        int n = strtol(name, &end, 10);
        if(!end) break;
        if(end > name) { ip.b[i] = n; mask.b[i] = 0xFF; }
        name = end;
        while(*name && *name++ != '.');
    }
    ipinfo &p = info.add();
    p.ip = ip.i;
    p.mask = mask.i;
}
ICOMMAND(0, addban, "s", (char *name), addipinfo(bans, name));
ICOMMAND(0, addallow, "s", (char *name), addipinfo(allows, name));

char *printipinfo(const ipinfo &info, char *buf)
{
    static string ipinfobuf = ""; char *str = buf ? buf : (char *)&ipinfobuf;
    union { uchar b[sizeof(enet_uint32)]; enet_uint32 i; } ip, mask;
    ip.i = info.ip;
    mask.i = info.mask;
    int lastdigit = -1;
    loopi(4) if(mask.b[i])
    {
        if(lastdigit >= 0) *str++ = '.';
        loopj(i - lastdigit - 1) { *str++ = '*'; *str++ = '.'; }
        str += sprintf(str, "%d", ip.b[i]);
        lastdigit = i;
    }
    return str;
}

bool checkipinfo(vector<ipinfo> &info, enet_uint32 host, bool global)
{
    loopv(info) if((!global || info[i].time < 0) && ((host & info[i].mask) == info[i].ip)) return true;
    return false;
}

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
    ircoutf(5, "%s", sf);
#endif
}

void conoutf(const char *s, ...)
{
    defvformatstring(sf, s, s);
    conoutft(0, "%s", sf);
}

VAR(IDF_PERSIST, verbose, 0, 0, 6);

#ifdef STANDALONE
void localservertoclient(int chan, ENetPacket *packet) {}
VAR(0, servertype, 1, 3, 3); // 1: private, 2: public, 3: dedicated
#else
VAR(0, servertype, 0, 1, 3); // 0: local only, 1: private, 2: public, 3: dedicated
#endif
VAR(0, serveruprate, 0, 0, INT_MAX-1);
VAR(0, serverport, 1, ENG_SERVER_PORT, INT_MAX-1);
VAR(0, servermasterport, 1, ENG_MASTER_PORT, INT_MAX-1);
SVAR(0, servermaster, ENG_MASTER_HOST);
SVAR(0, serverip, "");

int curtime = 0, totalmillis = 1, lastmillis = 1, timescale = 100, paused = 0, timeerr = 0;
const char *load = NULL;
vector<char *> gameargs;

// all network traffic is in 32bit ints, which are then compressed using the following simple scheme (assumes that most values are small).

template<class T>
static inline void putint_(T &p, int n)
{
    if(n<128 && n>-127) p.put(n);
    else if(n<0x8000 && n>=-0x8000) { p.put(0x80); p.put(n); p.put(n>>8); }
    else { p.put(0x81); p.put(n); p.put(n>>8); p.put(n>>16); p.put(n>>24); }
}
void putint(ucharbuf &p, int n) { putint_(p, n); }
void putint(packetbuf &p, int n) { putint_(p, n); }
void putint(vector<uchar> &p, int n) { putint_(p, n); }

int getint(ucharbuf &p)
{
    int c = (char)p.get();
    if(c==-128) { int n = p.get(); n |= char(p.get())<<8; return n; }
    else if(c==-127) { int n = p.get(); n |= p.get()<<8; n |= p.get()<<16; return n|(p.get()<<24); }
    else return c;
}

// much smaller encoding for unsigned integers up to 28 bits, but can handle signed
template<class T>
static inline void putuint_(T &p, int n)
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
void putuint(ucharbuf &p, int n) { putuint_(p, n); }
void putuint(packetbuf &p, int n) { putuint_(p, n); }
void putuint(vector<uchar> &p, int n) { putuint_(p, n); }

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

template<class T>
static inline void putfloat_(T &p, float f)
{
    lilswap(&f, 1);
    p.put((uchar *)&f, sizeof(float));
}
void putfloat(ucharbuf &p, float f) { putfloat_(p, f); }
void putfloat(packetbuf &p, float f) { putfloat_(p, f); }
void putfloat(vector<uchar> &p, float f) { putfloat_(p, f); }

float getfloat(ucharbuf &p)
{
    float f;
    p.get((uchar *)&f, sizeof(float));
    return lilswap(f);
}

template<class T>
static inline void sendstring_(const char *t, T &p)
{
    while(*t) putint(p, *t++);
    putint(p, 0);
}
void sendstring(const char *t, ucharbuf &p) { sendstring_(t, p); }
void sendstring(const char *t, packetbuf &p) { sendstring_(t, p); }
void sendstring(const char *t, vector<uchar> &p) { sendstring_(t, p); }

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

void filtertext(char *dst, const char *src, bool newline, bool colour, bool whitespace, int len)
{
    for(int c = *src; c; c = *++src)
    {
        if(newline && (c=='\n' || c=='\r')) c = ' ';
        if(c=='\f')
        {
            if(!colour) *dst++ = c;
            else
            {
                c = *++src;
                if(c=='z')
                {
                    c = *++src;
                    if(c) c = *++src;
                }
            }
            continue;
        }
        if(isspace(c) ? whitespace : isprint(c))
        {
            *dst++ = c;
            if(!--len) break;
        }
    }
    *dst = '\0';
}
ICOMMAND(0, filter, "siii", (char *s, int *a, int *b, int *c), string d; filtertext(s, d, *a==0, *b==0, *c==0); result(d));

const char *escapetext(const char *src, bool quoteonly)
{
    static string escbuf = ""; char *dst = (char *)&escbuf;
    for(int c = *src; c; c = *++src)
    {
        if(!quoteonly)
        {
            if(c=='\f') { *dst++ = '^'; *dst++ = 'f'; continue; }
            if(c=='\n') { *dst++ = '^'; *dst++ = 'n'; continue; }
            if(c=='\t') { *dst++ = '^'; *dst++ = 't'; continue; }
        }
        if(c=='\n') { *dst++ = ' '; continue; }
        if(c=='\"') { *dst++ = '^'; *dst++ = '\"'; continue; }
        if(isspace(c) || isprint(c)) *dst++ = c;
    }
    *dst = '\0';
    return escbuf;
}
ICOMMAND(0, escape, "si", (const char *s, int *a, int *b, int *c), result(escapetext(s, *a==0)));

vector<clientdata *> clients;

ENetHost *serverhost = NULL;
size_t bsend = 0, brec = 0;
int laststatus = 0;
ENetSocket pongsock = ENET_SOCKET_NULL;

void cleanupserver()
{
    if(serverhost) enet_host_destroy(serverhost);
    serverhost = NULL;
#ifdef IRC
    irccleanup();
#endif
}

void reloadserver()
{
    loopv(bans) if(bans[i].time == -1) bans.remove(i--);
    loopv(allows) if(allows[i].time == -1) allows.remove(i--);
}

void process(ENetPacket *packet, int sender, int chan);
//void disconnect_client(int n, int reason);

void *getinfo(int i)            { return !clients.inrange(i) || clients[i]->type==ST_EMPTY ? NULL : clients[i]->info; }
const char *gethostname(int i)  { int o = server::peerowner(i); return !clients.inrange(o) || clients[o]->type==ST_EMPTY ? "unknown" : clients[o]->hostname; }
int getnumclients()             { return clients.length(); }
uint getclientip(int n)         { int o = server::peerowner(n); return clients.inrange(o) && clients[o]->type==ST_TCPIP ? clients[o]->peer->address.host : 0; }

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
            localservertoclient(chan, packet);
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
    packetbuf p(MAXTRANS, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
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
            p.put(va_arg(args, uchar *), n);
            break;
        }
    }
    va_end(args);
    sendpacket(cn, chan, p.finalize(), exclude);
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

    packetbuf p(MAXTRANS+len, ENET_PACKET_FLAG_RELIABLE);
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

    file->seek(0, SEEK_SET);
    file->read(p.subbuf(len).buf, len);

    ENetPacket *packet = p.finalize();
    if(cn >= 0) sendpacket(cn, chan, packet, -1);
#ifndef STANDALONE
    else sendclientpacket(packet, chan);
#endif
}

void disconnect_client(int n, int reason)
{
    if(clients[n]->type!=ST_TCPIP) return;
    enet_peer_disconnect(clients[n]->peer, reason);
    server::clientdisconnect(n, false, reason);
    clients[n]->type = ST_EMPTY;
    clients[n]->peer->data = NULL;
    server::deleteinfo(clients[n]->info);
    clients[n]->info = NULL;
}

void kicknonlocalclients(int reason)
{
    loopv(clients) if(clients[i]->type==ST_TCPIP) disconnect_client(i, reason);
}

void process(ENetPacket *packet, int sender, int chan)  // sender may be -1
{
    packetbuf p(packet);
    server::parsepacket(sender, chan, p);
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
VAR(IDF_PERSIST, autoconnect, 0, 0, 1);
extern bool connectedlocally;
void localconnect(bool force)
{
    if((!connected() || !connectedlocally) && (force || autoconnect))
    {
        setsvar("serveraddress", "");
        setvar("serverconport", 0);
        int cn = addclient(ST_LOCAL);
        clientdata &c = *clients[cn];
        c.peer = NULL;
        copystring(c.hostname, "localhost");
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

void sendpongs()        // reply all server info requests
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
bool resolverwait(const char *name, ENetAddress *address)
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
        if(!resolverwait(hostname, &remoteaddress)) return ENET_SOCKET_NULL;
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
    ENetSocket sock = mastersend(address, servermaster, "update");
    if(sock==ENET_SOCKET_NULL) return buf;
    /* only cache this if connection succeeds */
    defformatstring(text)("retrieving servers from %s:[%d]", servermaster, address.port);
    progress(0, text);

    ENetBuffer eb;
    eb.data = buf;
    eb.dataLength = buflen-1;

    int starttime = SDL_GetTicks(), timeout = 0;
    while(masterreceive(sock, eb, 250))
    {
        timeout = SDL_GetTicks() - starttime;
        progress(min(float(timeout)/RETRIEVELIMIT, 1.0f), text);
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

void serverslice()  // main server update, called from main loop in sp, or from below in dedicated server
{
    server::serverupdate();

    if(!serverhost) return;

    sendpongs();

    if(servertype >= 2 && totalmillis-laststatus >= 60000)  // display bandwidth stats, useful for server ops
    {
        laststatus = totalmillis;
        if(bsend || brec || server::numclients())
            conoutf("status: %d clients, %.1f send, %.1f rec (K/sec)", server::numclients(), bsend/60.0f/1024, brec/60.0f/1024);
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
                static string hostn = "";
                copystring(c.hostname, (enet_address_get_host_ip(&c.peer->address, hostn, sizeof(hostn))==0) ? hostn : "unknown");
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

void flushserver(bool force)
{
    if(server::sendpackets(force) && serverhost) enet_host_flush(serverhost);
}

#ifndef STANDALONE
int clockrealbase = 0, clockvirtbase = 0;
void clockreset() { clockrealbase = SDL_GetTicks(); clockvirtbase = totalmillis; }
VARF(IDF_PERSIST, clockerror, 990000, 1000000, 1010000, clockreset());
VARF(IDF_PERSIST, clockfix, 0, 0, 1, clockreset());
#endif

void updatetimer()
{
#ifdef STANDALONE
    int millis = (int)enet_time_get();
#else
    int millis = SDL_GetTicks() - clockrealbase;
    if(clockfix) millis = int(millis*(double(clockerror)/1000000));
    millis += clockvirtbase;
    if(millis<totalmillis) millis = totalmillis;
    extern void limitfps(int &millis, int curmillis);
    limitfps(millis, totalmillis);
#endif
    int elapsed = millis-totalmillis;
    if(paused) curtime = 0;
    else if(timescale != 100)
    {
        int scaledtime = elapsed*timescale + timeerr;
        curtime = scaledtime/100;
        timeerr = scaledtime%100;
        if(curtime>200) curtime = 200;
    }
    else
    {
        curtime = elapsed + timeerr;
        timeerr = 0;
    }
    lastmillis += curtime;
    totalmillis = millis;
}

void serverloop()
{
    #ifdef WIN32
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
    #endif
    conoutf("\fgdedicated server started, waiting for clients... [Ctrl-C to exit]");
    for(;;)
    {
        //int _lastmillis = lastmillis;
        //lastmillis = totalmillis = (int)enet_time_get();
        //curtime = lastmillis-_lastmillis;
        int prevmillis = (int)enet_time_get();
        updatetimer();

#ifdef MASTERSERVER
        checkmaster();
#endif
        serverslice();
#ifdef IRC
        ircslice();
#endif

        if((int)enet_time_get()-prevmillis <= 0)
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

    conoutf("init: server (%s:%d)", *serverip ? serverip : "*", serverport);
    ENetAddress address = { ENET_HOST_ANY, serverport };
    if(*serverip)
    {
        if(enet_address_set_host(&address, serverip) < 0) conoutf("\frWARNING: server address not resolved");
    }
    serverhost = enet_host_create(&address, server::reserveclients(), 0, serveruprate);
    if(!serverhost)
    {
        conoutf("\frcould not create server socket");
#ifndef STANDALONE
        setvar("servertype", 0);
#endif
        return;
    }
    loopi(server::reserveclients()) serverhost->peers[i].data = NULL;

    address.port = serverport+1;
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
#ifdef STANDALONE
    rehash(false);
#endif
    setupserver();
}

VAR(0, hasoctapaks, 1, 0, 0); // mega hack; try to find Cube 2, done after our own data so as to not clobber stuff
SVAR(IDF_PERSIST, octadir, "");//, if(!hasoctapaks) trytofindocta(false););

bool serveroption(char *opt)
{
    switch(opt[1])
    {
        case 'k': kidmode = atoi(opt+2); return true;
        case 'h': printf("set home directory: %s\n", &opt[2]); sethomedir(&opt[2]); return true;
        case 'o': setsvar("octadir", &opt[2]); return true;
        case 'p': printf("add package directory: %s\n", &opt[2]); addpackagedir(&opt[2]); return true;
        case 'v': setvar("verbose", atoi(opt+2)); return true;
        case 's':
        {
            switch(opt[2])
            {
                case 'u': setvar("serveruprate", atoi(opt+3)); return true;
                case 'i': setsvar("serverip", opt+3); return true;
                case 'm': setsvar("servermaster", opt+3); return true;
                case 'l': load = opt+3; return true;
                case 's': setvar("servertype", atoi(opt+3)); return true;
                case 'p': setvar("serverport", atoi(opt+3)); return true;
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
    mkstring(s); copystring(s, name); path(s);
    defformatstring(octalogo)("%s/data/default_map_settings.cfg", s);
    if(fileexists(findfile(octalogo, "r"), "r"))
    {
        conoutf("\fgfound octa directory: %s", s);
        defformatstring(octadata)("%s/data", s);
        defformatstring(octapaks)("%s/packages", s);
        addpackagedir(s, PACKAGEDIR_OCTA);
        addpackagedir(octadata, PACKAGEDIR_OCTA);
        addpackagedir(octapaks, PACKAGEDIR_OCTA);
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
#if defined(WIN32)
        mkstring(dir);
        if(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, dir) == S_OK)
        {
            defformatstring(s)("%s\\Sauerbraten", dir);
            if(findoctadir(s, true)) return;
        }
#elif defined(__APPLE__)
        extern const char *mac_sauerbratendir();
        const char *dir = mac_sauerbratendir();
        if(dir && findoctadir(dir, true)) return;
#endif
        const char *tryoctadirs[4] = { // by no means an accurate or definitive list either..
            "../Sauerbraten", "../sauerbraten", "../sauer",
#if defined(WIN32)
            "/Program Files/Sauerbraten"
#elif defined(__APPLE__)
            "/Applications/sauerbraten.app/Contents/gamedata"
#else
            "/usr/games/sauerbraten"
#endif
        };
        loopi(4) if(findoctadir(tryoctadirs[i], true)) return;
    }
}

void setlocations(bool wanthome)
{
    addpackagedir("data");
    if(wanthome)
    {
#if defined(WIN32)
        #ifndef __GNUC__ // not supported by mingw
        OSVERSIONINFO ver;
        if(GetVersionEx((OSVERSIONINFO *)&ver))
        {
            if(ver.dwMajorVersion >= 6)
            {
                char *dir = NULL;
                if(SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, dir) == S_OK)
                {
                    defformatstring(s)("%s\\Blood Frontier", dir);
                    sethomedir(s);
                    DELETEP(dir);
                    return;
                }
            }
            else
            {
                mkstring(dir);
                if(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, dir) == S_OK)
                {
                    defformatstring(s)("%s\\My Games\\Blood Frontier", dir);
                    sethomedir(s);
                    return;
                }
            }
        }
        #else
        mkstring(dir);
        if(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, dir) == S_OK)
        {
            defformatstring(s)("%s\\My Games\\Blood Frontier", dir);
            sethomedir(s);
            return;
        }
        #endif
#elif defined(__APPLE__)
        extern const char *mac_personaldir();
        const char *dir = mac_personaldir(); // typically  /Users/<name>/Application Support/
        if(dir) {
            defformatstring(s)("%s/bloodfrontier", dir);
            sethomedir(s);
            return;
        }
#else
        const char *dir = getenv("HOME");
        if(dir && *dir)
        {
            defformatstring(s)("%s/.bloodfrontier", dir);
            sethomedir(s);
            return;
        }
#endif
        sethomedir("home");
    }
}

void writecfg()
{
#ifndef STANDALONE
    stream *f = openfile("config.cfg", "w");
    if(!f) return;
    client::writeclientinfo(f);
    vector<ident *> ids;
    enumerate(*idents, ident, id, ids.add(&id));
    ids.sort(sortidents);
    bool found = false;
    loopv(ids)
    {
        ident &id = *ids[i];
        bool saved = false;
        if(id.flags&IDF_PERSIST) switch(id.type)
        {
            case ID_VAR: if(*id.storage.i != id.def.i) { found = saved = true; f->printf((id.flags&IDF_HEX ? (id.maxval==0xFFFFFF ? "%s 0x%.6X" : "%s 0x%X") : "%s %d"), id.name, *id.storage.i); } break;
            case ID_FVAR: if(*id.storage.f != id.def.f) { found = saved = true; f->printf("%s %s", id.name, floatstr(*id.storage.f)); } break;
            case ID_SVAR: if(strcmp(*id.storage.s, id.def.s)) { found = saved = true; f->printf("%s ", id.name); writeescapedstring(f, *id.storage.s); } break;
        }
        if(saved)
        {
            if(!(id.flags&IDF_COMPLETE)) f->printf("; setcomplete \"%s\" 0\n", id.name);
            else f->printf("\n");
        }
    }
    if(found) f->printf("\n");
    found = false;
    loopv(ids)
    {
        ident &id = *ids[i];
        bool saved = false;
        if(id.flags&IDF_PERSIST) switch(id.type)
        {
            case ID_ALIAS: if(id.override==NO_OVERRIDE && id.action[0])
            {
                found = saved = true;
                f->printf("\"%s\" = [%s]", id.name, id.action);
            }
            break;
        }
        if(saved)
        {
            if(!(id.flags&IDF_COMPLETE)) f->printf("; setcomplete \"%s\" 0\n", id.name);
            else f->printf("\n");
        }
    }
    if(found) f->printf("\n");
    writebinds(f);
    delete f;
#endif
}

COMMAND(0, writecfg, "");

VAR(0, rehashing, 1, 0, -1);
void rehash(bool reload)
{
    if(reload)
    {
        rehashing = 1;
        writecfg();
    }
    reloadserver();
#ifdef MASTERSERVER
    reloadmaster();
#endif
    execfile("servinit.cfg", false);
#ifndef STANDALONE
    execfile("defaults.cfg");
    initing = INIT_LOAD;
    interactive = true;
    execfile("config.cfg", false);
    execfile("autoexec.cfg", false);
    interactive = false;
    initing = NOT_INITING;
#endif
    conoutf("\fcconfiguration reloaded");
    rehashing = 0;
}
ICOMMAND(0, rehash, "i", (int *nosave), rehash(*nosave ? false : true));

#ifdef STANDALONE
#include <signal.h>
volatile int errors = 0;
void fatal(const char *s, ...)    // failure exit
{
    if(++errors <= 2) // print up to one extra recursive error
    {
        defvformatstring(msg, s, s);
        fprintf(stderr, "Exiting: %s\n", msg);
        if(errors <= 1) // avoid recursion
        {
            cleanupserver();
#ifdef MASTERSERVER
            cleanupmaster();
#endif
            enet_deinitialize();
        }
    }
    exit(EXIT_FAILURE);
}

volatile bool fatalsig = false;
void fatalsignal(int signum)
{
    if(!fatalsig)
    {
        fatalsig = true;
        fatal("Received fatal signal %d", signum);
    }
}

void reloadsignal(int signum)
{
    rehash(true);
}

int main(int argc, char* argv[])
{
    setlocations(false);
    char *initscript = NULL;
    for(int i = 1; i<argc; i++)
    {
        if(argv[i][0]=='-') switch(argv[i][1])
        {
            case 'x': initscript = &argv[i][2]; break;
            default: if(!serveroption(argv[i])) gameargs.add(argv[i]); break;
        }
        else gameargs.add(argv[i]);
    }
    if(enet_initialize()<0) fatal("Unable to initialise network module");
    signal(SIGINT, fatalsignal);
    signal(SIGILL, fatalsignal);
    signal(SIGABRT, fatalsignal);
    signal(SIGFPE, fatalsignal);
    signal(SIGSEGV, fatalsignal);
    signal(SIGTERM, fatalsignal);
    signal(SIGINT, fatalsignal);
#if !defined(WIN32) && !defined(__APPLE__)
    signal(SIGHUP, reloadsignal);
    signal(SIGQUIT, fatalsignal);
    signal(SIGKILL, fatalsignal);
    signal(SIGPIPE, fatalsignal);
    signal(SIGALRM, fatalsignal);
    signal(SIGSTOP, fatalsignal);
#endif
    enet_time_set(0);
    initgame();
    trytofindocta();
    if(initscript) execute(initscript);
    serverloop();
    return 0;
}
#endif

