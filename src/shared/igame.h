// the interface the engine uses to run the gameplay module
#ifndef STANDALONE
namespace entities
{
	extern int triggertime(const extentity &e);
	extern void editent(int i);
	extern void readent(stream *g, int mtype, int mver, char *gid, int gver, int id, entity &e);
	extern void writeent(stream *g, int id, entity &e);
	extern void initents(stream *g, int mtype, int mver, char *gid, int gver);
	extern float dropheight(entity &e);
	extern void fixentity(int n);
	extern bool cansee(const extentity &e);;
	extern const char *findname(int type);
	extern int findtype(char *type);
	extern bool maylink(int type, int ver = 0);
	extern bool canlink(int index, int node, bool msg = false);
	extern bool linkents(int index, int node, bool add, bool local, bool toggle);
	extern extentity *newent();
    extern void deleteent(extentity *e);
    extern void clearents();
	extern vector<extentity *> &getents();
	extern void drawparticles();
}

namespace client
{
	extern void gamedisconnect(int clean);
	extern void parsepacketclient(int chan, ucharbuf &p);
	extern int sendpacketclient(ucharbuf &p, bool &reliable);
	extern void gameconnect(bool _remote);
	extern bool allowedittoggle(bool edit);
	extern void edittoggled(bool edit);
	extern void writeclientinfo(stream *f);
	extern void toserver(int flags, char *text);
	extern bool sendcmd(int nargs, char *cmd, char *arg);
	extern void editvar(ident *id, bool local);
	extern void edittrigger(const selinfo &sel, int op, int arg1 = 0, int arg2 = 0, int arg3 = 0);
	extern void changemap(const char *name);
	extern bool ready();
    extern void connectattempt(const char *name, int port, int qport, const char *password, const ENetAddress &address);
    extern void connectfail();
	extern int state();
	extern int otherclients();
	extern int numchannels();
	extern int servercompare(serverinfo *a, serverinfo *b);
	extern int serverbrowser(g3d_gui *g);
}

namespace hud
{
	extern void drawhud(int w, int h);
	extern void drawlast(int w, int h);
	extern bool getcolour(vec &colour);
	extern void gamemenus();
}

namespace physics
{
	extern int physsteps, physframetime, physinterp;
	extern float gravity, jumpvel, movespeed, floatspeed,
		stairheight, floorz, slopez, wallz, stepspeed,
			liquidfric, liquidscale, sinkfric, floorfric, airfric;
	extern bool move(physent *d, vec &dir);
	extern void move(physent *d, int moveres = 10, bool local = true);
	extern bool entinmap(physent *d, bool avoidplayers);
	extern void updatephysstate(physent *d);
	extern bool droptofloor(vec &o, float radius, float height);
	extern float movevelocity(physent *d);
	extern bool issolid(physent *d);
	extern bool iscrouching(physent *d);
	extern bool moveplayer(physent *pl, int moveres, bool local, int millis);
	extern float gravityforce(physent *d);
	extern void interppos(physent *d);
    extern void updateragdoll(dynent *d, const vec &center, float radius);
}

namespace world
{
	extern bool clientoption(char *arg);
	extern void updateworld();
	extern void newmap(int size);
	extern void resetmap(bool empty);
	extern void startmap(const char *name);
	extern bool allowmove(physent *d);
	extern dynent *iterdynents(int i);
	extern int numdynents();
	extern void lighteffects(dynent *d, vec &color, vec &dir);
	extern void adddynlights();
	extern void particletrack(particle *p, uint type, int &ts, vec &o, vec &d, bool lastpass);
	extern vec abovehead(physent *d, float off = 1e16f);
	extern vec headpos(physent *d, float off = 0.f);
	extern vec feetpos(physent *d, float off = 0.f);
	extern bool mousemove(int dx, int dy, int x, int y, int w, int h);
	extern void project(int w, int h);
	extern void recomputecamera(int w, int h);
	extern void loadworld(stream *f, int maptype);
	extern void saveworld(stream *f);
	extern int localplayers();
	extern char *gametitle();
	extern char *gametext();
	extern int numanims();
	extern void findanims(const char *pattern, vector<int> &anims);
	extern void render();
	extern void renderavatar(bool early);
	extern bool isthirdperson();
	extern void start();
}
#endif
namespace server
{
	extern void srvmsgf(int cn, const char *s, ...);
	extern void srvoutf(const char *s, ...);
	extern bool serveroption(char *arg);
	extern void *newinfo();
	extern void deleteinfo(void *ci);
	extern int numclients(int exclude = -1, bool nospec = true, bool noai = false);
    extern int reserveclients();
	extern void clientdisconnect(int n, bool local = false);
	extern int clientconnect(int n, uint ip, bool local = false);
    extern bool allowbroadcast(int n);
    extern int peerowner(int n);
	extern void recordpacket(int chan, void *data, int len);
	extern void parsepacket(int sender, int chan, bool reliable, ucharbuf &p);
	extern bool sendpackets();
	extern void queryreply(ucharbuf &req, ucharbuf &p);
	extern void serverupdate();
	extern void changemap(const char *s, int mode = -1, int muts = -1);
	extern const char *gameid();
	extern char *gamename(int mode, int muts);
	extern void modecheck(int *mode, int *muts);
	extern int gamever();
	extern const char *choosemap(const char *suggest);
	extern bool canload(char *type);
	extern void start();
	extern bool servcmd(int nargs, char *cmd, char *arg);
}
