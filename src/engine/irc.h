
enum { IRCC_NONE = 0, IRCC_JOINING, IRCC_JOINED, IRCC_KICKED, IRCC_BANNED };
struct ircchan
{
	int state;
	char *name, *pass;
	ircchan() : state(IRCC_NONE) {}
	~ircchan() { DELETEP(name); DELETEP(pass); }
};
enum { IRCT_NONE = 0, IRCT_CLIENT, IRCT_RELAY, IRCT_MAX };
enum { IRC_DISC = 0, IRC_ATTEMPT, IRC_CONN, IRC_ONLINE, IRC_MAX };
struct ircnet
{
	int type, state, port;
	char *name, *serv, *nick, *pass;
	ENetAddress address;
	ENetSocket sock;
	uchar input[4096];
	vector<ircchan> channels;

	ircnet() {}
	ircnet(int t, const char *n, const char *s, int p, const char *c, const char *z = NULL)
		: type(t), state(IRC_DISC), port(p), name(newstring(n)), serv(newstring(s)),
			nick(newstring(c)), pass(newstring(z ? z : ""))
	{
		address.host = ENET_HOST_ANY;
		address.port = port;
		channels.setsize(0);
		conoutf("added irc network %s (%s:%d) [%s]", name, serv, port, nick);
	}
	~ircnet()
	{
		DELETEP(name); DELETEP(serv); DELETEP(nick); DELETEP(pass);
		channels.setsize(0);
	}
};
extern vector<ircnet *> ircnets;
extern void ircslice();
