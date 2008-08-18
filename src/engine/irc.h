enum { IRCC_NONE = 0, IRCC_JOINING, IRCC_JOINED, IRCC_KICKED, IRCC_BANNED };
enum { IRCCT_NONE = 0, IRCCT_AUTO };
struct ircchan
{
	int state, type;
	char *name, *passkey;

	ircchan() {}
	~ircchan() { DELETEP(name); DELETEP(passkey); }
};
enum { IRCT_NONE = 0, IRCT_CLIENT, IRCT_RELAY, IRCT_MAX };
enum { IRC_DISC = 0, IRC_ATTEMPT, IRC_CONN, IRC_ONLINE, IRC_MAX };
struct ircnet
{
	int type, state, port;
	char *name, *serv, *nick, *passkey;
	ENetAddress address;
	ENetSocket sock;
	uchar input[4096];
	vector<ircchan> channels;

	ircnet() {}
	~ircnet()
	{
		DELETEP(name); DELETEP(serv); DELETEP(nick); DELETEP(passkey);
		channels.setsize(0);
	}
};
extern vector<ircnet> ircnets;
extern void irccleanup();
extern void ircslice();
