enum { IRCC_NONE = 0, IRCC_JOINING, IRCC_JOINED, IRCC_KICKED, IRCC_BANNED };
enum { IRCCT_NONE = 0, IRCCT_AUTO };
struct ircchan
{
	int state, type, relay, lastjoin;
	string name, friendly, passkey;
	vector<char *> lines;

	ircchan() {}
	~ircchan()
	{
		loopv(lines) DELETEA(lines[i]);
		lines.setsize(0);
	}
};
enum { IRCT_NONE = 0, IRCT_CLIENT, IRCT_RELAY, IRCT_MAX };
enum { IRC_DISC = 0, IRC_ATTEMPT, IRC_CONN, IRC_ONLINE, IRC_MAX };
struct ircnet
{
	int type, state, port, lastattempt;
	string name, serv, nick, ip, passkey;
	ENetAddress address;
	ENetSocket sock;
	vector<ircchan> channels;
	vector<char *> lines;
	uchar input[4096];

	ircnet() {}
	~ircnet()
	{
		channels.setsize(0);
		loopv(lines) DELETEA(lines[i]);
		lines.setsize(0);
	}
};

extern vector<ircnet> ircnets;

extern ircnet *ircfind(const char *name);
extern void ircestablish(ircnet *n);
extern void ircsend(ircnet *n, const char *msg, ...);
extern void ircoutf(int relay, const char *msg, ...);
extern int ircrecv(ircnet *n, int timeout = 0);
extern char *ircread(ircnet *n);
extern void ircnewnet(int type, const char *name, const char *serv, int port, const char *nick, const char *ip = "", const char *passkey = "");
extern ircchan *ircfindchan(ircnet *n, const char *name);
extern bool ircjoin(ircnet *n, ircchan *c);
extern bool ircenterchan(ircnet *n, const char *name);
extern bool ircnewchan(int type, const char *name, const char *channel, const char *friendly = "", const char *passkey = "", int relay = 0);
extern void ircparse(ircnet *n, char *reply);
extern void ircdiscon(ircnet *n);
extern void irccleanup();
extern void ircslice();
