#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "pch.h"
#include "cube.h"

#include <enet/time.h>

#ifdef WIN32
#include <winerror.h>
#ifndef ENOTCONN
#define ENOTCONN WSAENOTCONN
#endif
#endif

#define KEEPALIVE_TIME (65*60*1000)
#define CLIENT_TIME (3*60*1000)
#define CLIENT_LIMIT 1024
#define SERVER_LIMIT (10*1024)

struct gameserver
{
    ENetAddress address;
    enet_uint32 registertime;
};
vector<gameserver> gameservers;

struct outputmsg
{
    virtual ~outputmsg() {}

    virtual const char *getbuf() = 0;
    virtual int length() = 0;
    virtual void purge() {}
};

struct serverresponse : outputmsg
{
    const char *msg;
    int len;

    serverresponse(const char *msg) : msg(msg), len(strlen(msg)) {}

    const char *getbuf() { return msg; }
    int length() { return len; }
};

struct gameserverlist : outputmsg
{
    vector<char> buf;
    int refs;

    gameserverlist() : refs(0) {}

    const char *getbuf() { return buf.getbuf(); }
    int length() { return buf.length(); }
    void purge();
};
vector<gameserverlist *> gameserverlists;
bool updateserverlist = true;

struct client
{
    ENetAddress address;
    ENetSocket socket;
    char input[4096];
    outputmsg *output;
    int inputpos, outputpos;
    enet_uint32 connecttime;

    client() : output(NULL), inputpos(0), outputpos(-1) {}
};
vector<client *> clients;

ENetSocket serversocket = ENET_SOCKET_NULL;

enet_uint32 curtime = 0;

void fatal(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    exit(EXIT_FAILURE);
}

void setupserver(int port, const char *ip = NULL)
{
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    if(ip)
    {
        if(enet_address_set_host(&address, ip)<0)
            fatal("failed to resolve server address: %s\n", ip);
    }
    serversocket = enet_socket_create(ENET_SOCKET_TYPE_STREAM, &address);
    if(serversocket==ENET_SOCKET_NULL)
        fatal("failed to create server socket\n");
    if(enet_socket_set_option(serversocket, ENET_SOCKOPT_NONBLOCK, 1)<0)
        fatal("failed to make server socket non-blocking\n");

    enet_time_set(0);

    time_t t = time(NULL);
    char *ct = ctime(&t);
    if(strchr(ct, '\n')) *strchr(ct, '\n') = '\0';
    printf("*** Starting masterserver on %s %d at %s ***\n", ip ? ip : "localhost", port, ct);
}

void genserverlist()
{
    if(!updateserverlist) return;
    while(gameserverlists.length() && gameserverlists.last()->refs<=0)
        delete gameserverlists.pop();
    gameserverlist *l = new gameserverlist;
    loopv(gameservers)
    {
        gameserver &s = gameservers[i];
        string hostname;
        if(enet_address_get_host_ip(&s.address, hostname, sizeof(hostname))<0) continue;
        s_sprintfd(cmd)("addserver %s\n", hostname);
        l->buf.put(cmd, strlen(cmd));
    }
    l->buf.add('\0');
    gameserverlists.add(l);
    updateserverlist = false;
}

serverresponse registerresponse("registered server\n"), renewresponse("renewed server registration\n");

void addgameserver(client &c)
{
    if(gameservers.length()>=SERVER_LIMIT) return;
    loopv(gameservers)
    {
        gameserver &s = gameservers[i];
        if(s.address.host==c.address.host)
        {
            s.registertime = curtime;
            c.output = &renewresponse;
            c.outputpos = 0;
            return;
        }
    }
    gameserver &s = gameservers.add();
    s.address = c.address;
    s.registertime = curtime;
    c.output = &registerresponse;
    c.outputpos = 0;
    string name;
    if(enet_address_get_host_ip(&c.address, name, sizeof(name))<0) s_strcpy(name, "<<unknown host>>");
    printf("registering server %s\n", name);
    updateserverlist = true;
}

void checkgameservers()
{
    loopv(gameservers)
    {
        gameserver &s = gameservers[i];
        if(ENET_TIME_DIFFERENCE(curtime, s.registertime) > KEEPALIVE_TIME)
        {
            gameservers.remove(i--);
            updateserverlist = true;
        }
    }
}

void gameserverlist::purge()
{
    refs = max(refs - 1, 0);
    if(refs<=0 && gameserverlists.last()!=this)
    {
        gameserverlists.removeobj(this);
        delete this;
    }
}

void purgeclient(int n)
{
    client &c = *clients[n];
    if(c.output) c.output->purge();
    enet_socket_destroy(c.socket);
    delete clients[n];
    clients.remove(n);
}

bool checkclientinput(client &c)
{
    if(c.inputpos<0) return true;
    else if(strstr(c.input, "add"))
    {
        addgameserver(c);
        return false;
    }
    else if(strstr(c.input, "list"))
    {
        genserverlist();
        if(gameserverlists.empty()) return false;
        c.output = gameserverlists.last();
        c.outputpos = 0;
        return true;
    }
    else return c.inputpos<(int)sizeof(c.input);
}

fd_set readset, writeset;

void checkclients()
{
    fd_set readset, writeset;
    int nfds = serversocket;
    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_SET(serversocket, &readset);
    loopv(clients)
    {
        client &c = *clients[i];
        if(c.outputpos>=0) FD_SET(c.socket, &writeset);
        else FD_SET(c.socket, &readset);
        nfds = max(nfds, (int)c.socket);
    }
    timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    if(select(nfds+1, &readset, &writeset, NULL, &tv)<=0) return;

    curtime = enet_time_get();
    if(FD_ISSET(serversocket, &readset))
    {
        ENetAddress address;
        ENetSocket clientsocket = enet_socket_accept(serversocket, &address);
        if(clients.length()>=CLIENT_LIMIT) enet_socket_destroy(clientsocket);
        else if(clientsocket!=ENET_SOCKET_NULL)
        {
            client *c = new client;
            c->address = address;
            c->socket = clientsocket;
            c->connecttime = curtime;
            clients.add(c);
        }
    }

    loopv(clients)
    {
        client &c = *clients[i];
        if(c.outputpos>=0 && FD_ISSET(c.socket, &writeset))
        {
            ENetBuffer buf;
            buf.data = (void *)&c.output->getbuf()[c.outputpos];
            buf.dataLength = c.output->length()-c.outputpos;
            int res = enet_socket_send(c.socket, NULL, &buf, 1);
            if(res>=0)
            {
                c.outputpos += res;
                if(c.outputpos>=c.output->length()) { purgeclient(i--); continue; }
            }
            else if(errno==ENOTCONN) { purgeclient(i--); continue; }
        }
        if(c.outputpos<0 && FD_ISSET(c.socket, &readset))
        {
            ENetBuffer buf;
            buf.data = &c.input[c.inputpos];
            buf.dataLength = sizeof(c.input) - c.inputpos;
            int res = enet_socket_receive(c.socket, NULL, &buf, 1);
            if(res>=0)
            {
                c.inputpos += res;
                c.input[min(c.inputpos, (int)sizeof(c.input)-1)] = '\0';
                if(!checkclientinput(c)) { purgeclient(i--); continue; }
            }
            else if(errno==ENOTCONN) { purgeclient(i--); continue; }
        }
        if(ENET_TIME_DIFFERENCE(curtime, c.connecttime) >= CLIENT_TIME) { purgeclient(i--); continue; }
    }
}


int main(int argc, char **argv)
{
    char *ip = NULL;
    int port = MASTER_PORT;
    if(argc>=2) port = atoi(argv[1]);
    if(argc>=3) ip = argv[2];

	if(enet_initialize()<0) fatal("Unable to initialise network module");

    setupserver(port, ip);
    for(;;)
    {
        checkgameservers();
        checkclients();
    }

    return EXIT_SUCCESS;
}

