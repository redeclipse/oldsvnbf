enum { IRCC_NONE = 0, IRCC_JOINING, IRCC_JOINED, IRCC_KICKED, IRCC_BANNED };
enum { IRCCT_NONE = 0, IRCCT_AUTO };
struct ircchan
{
	int state, type, lastjoin;
	string name, passkey;

	ircchan() {}
	~ircchan() {}
};
enum { IRCT_NONE = 0, IRCT_CLIENT, IRCT_RELAY, IRCT_MAX };
enum { IRC_DISC = 0, IRC_ATTEMPT, IRC_CONN, IRC_ONLINE, IRC_MAX };
struct ircnet
{
	int type, state, port;
	string name, serv, nick, passkey;
	ENetAddress address;
	ENetSocket sock;
	vector<ircchan> channels;
	uchar input[4096];

	ircnet() {}
	~ircnet() { channels.setsize(0); }
};

extern vector<ircnet> ircnets;

extern ircnet *ircfind(const char *name);
extern void ircconnect(ircnet *n);
extern void ircsend(ircnet *n, const char *msg, ...);
extern void ircoutf(const char *msg, ...);
extern int ircrecv(ircnet *n, char *input, int timeout = 0);
extern char *ircread(ircnet *n);
extern void ircaddnet(int type, const char *name, const char *serv, int port, const char *nick, const char *passkey = "");
extern ircchan *ircfindchan(ircnet *n, const char *name);
extern bool ircjoin(ircnet *n, ircchan *c);
extern bool ircjoinchan(ircnet *n, const char *name);
extern bool ircaddchan(int type, const char *name, const char *channel, const char *passkey = "");
extern void ircparse(ircnet *n, char *reply);extern void irccleanup();
extern void ircslice();
