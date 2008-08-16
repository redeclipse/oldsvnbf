struct ircchan
{
	char *name;
};

enum { IRC_DISC = 0, IRC_ATTEMPT, IRC_CONN, IRC_ONLINE };
struct ircnet
{
	int state, port;
	char *name, *serv, *nick, *pass;//, *nspass;
	ENetAddress address;
	ENetSocket sock;
	uchar input[4096];
	//vector<ircchan> ircchans;

	ircnet() {}
	ircnet(const char *n, const char *s, int p, const char *c, const char *z = NULL)
		: state(IRC_DISC), port(p), name(newstring(n)), serv(newstring(s)),
			nick(newstring(c)), pass(z ? newstring(z) : newstring(""))
	{
		address.host = ENET_HOST_ANY;
		address.port = port;
		conoutf("added irc network %s (%s:%d) [%s]", name, serv, port, nick);
	}
	~ircnet()
	{
		DELETEP(name); DELETEP(serv); DELETEP(nick); DELETEP(pass);
	}
};
extern vector<ircnet *> ircnets;
extern void ircslice();
