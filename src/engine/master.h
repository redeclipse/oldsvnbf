#define MASTER_PORT		28800

struct mastermsg
{
    virtual ~mastermsg() {}

    virtual const char *getbuf() = 0;
    virtual int length() = 0;
    virtual void purge() {}
};

struct masterout : mastermsg
{
    const char *msg;
    int len;

    masterout(const char *msg) : msg(msg), len(strlen(msg)) {}

    const char *getbuf() { return msg; }
    int length() { return len; }
};

struct masterclient
{
    ENetAddress address;
    ENetSocket socket;
    char input[4096];
    mastermsg *output;
    int inputpos, outputpos;
    enet_uint32 connecttime;

    masterclient() : output(NULL), inputpos(0), outputpos(-1) {}
};

struct masterentry
{
    ENetAddress address;
    enet_uint32 registertime;
};

struct masterlist : mastermsg
{
    vector<char> buf;
    int refs;

    masterlist() : refs(0) {}

    const char *getbuf() { return buf.getbuf(); }
    int length() { return buf.length(); }
    void purge();
};

extern void setupmaster();
extern void checkmaster();
extern void cleanupmaster();

extern int masterserver, masterport;
extern char *masterip;

#define KEEPALIVE_TIME (65*60*1000)
#define MASTER_TIME (3*60*1000)
#define MASTER_LIMIT 1024
#define SERVER_LIMIT (10*1024)
