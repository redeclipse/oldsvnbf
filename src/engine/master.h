#define KEEPALIVE_TIME (65*60*1000)
#define MASTER_TIME (3*60*1000)
#define MASTER_LIMIT 1024
#define SERVER_LIMIT (10*1024)
#define CLIENT_TIME (3*60*1000)
#define AUTH_TIME (60*1000)
#define AUTH_LIMIT 100
#define CLIENT_LIMIT 4096
#define DUP_LIMIT 16

extern void setupmaster();
extern void checkmaster();
extern void cleanupmaster();

extern int masterserver, masterport;
extern char *masterip;
