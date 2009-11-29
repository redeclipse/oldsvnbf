// the interface the engine uses to run the gameplay module
#ifndef STANDALONE
namespace entities
{
	extern int triggertime(extentity &e);
	extern void editent(int i);
	extern void readent(stream *g, int mtype, int mver, char *gid, int gver, int id);
	extern void writeent(stream *g, int id);
	extern void remapents(vector<int> &idxs);
	extern void initents(stream *g, int mtype, int mver, char *gid, int gver);
	extern float dropheight(extentity &e);
	extern void fixentity(int n, bool recurse = true, bool create = false);
	extern bool cansee(extentity &e);
	extern const char *findname(int type);
	extern int findtype(char *type);
	extern bool maylink(int type, int ver = 0);
	extern bool canlink(int index, int node, bool msg = false);
	extern bool linkents(int index, int node, bool add = true, bool local = true, bool toggle = true);
	extern extentity *newent();
	extern void deleteent(extentity *e);
	extern void clearents();
	extern vector<extentity *> &getents();
	extern int lastent(int type);
	extern void drawparticles();
}

namespace client
{
	extern void gamedisconnect(int clean);
	extern void parsepacketclient(int chan, packetbuf &p);
	extern void gameconnect(bool _remote);
	extern bool allowedittoggle(bool edit);
	extern void edittoggled(bool edit);
	extern void writeclientinfo(stream *f);
	extern void toserver(int flags, char *text);
	extern bool sendcmd(int nargs, const char *cmd, const char *arg);
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
	extern int serverbrowser(guient *g);
}

namespace hud
{
	extern void drawhud(bool noview = false);
	extern void drawlast();
	extern float motionblur(float scale);
	extern bool getcolour(vec &colour);
	extern void gamemenus();
	extern void update(int w, int h);
}

namespace physics
{
	extern int physsteps, physframetime, physinterp;
	extern float gravity, jumpvel, movespeed, floatspeed, stairheight, floorz, slopez, wallz, stepspeed, liquidfric, liquidscale, sinkfric, floorfric, airfric;
	extern float liquidmerge(physent *d, float from, float to);
	extern bool liquidcheck(physent *d);
	extern float gravityforce(physent *d);
	extern float movevelocity(physent *d);
	extern bool issolid(physent *d, physent *e = NULL);
	extern bool move(physent *d, vec &dir);
	extern void move(physent *d, int moveres = 10, bool local = true);
	extern bool entinmap(physent *d, bool avoidplayers);
	extern void updatephysstate(physent *d);
	extern bool droptofloor(vec &o, float radius, float height);
	extern bool iscrouching(physent *d);
	extern bool moveplayer(physent *pl, int moveres, bool local, int millis);
	extern void interppos(physent *d);
	extern void updateragdoll(dynent *d, const vec &center, float radius);
	extern bool xcollide(physent *d, const vec &dir, physent *o);
	extern bool xtracecollide(physent *d, const vec &from, const vec &to, float x1, float x2, float y1, float y2, float maxdist, float &dist, physent *o);
	extern void complexboundbox(physent *d);
}

namespace game
{
	extern bool clientoption(char *arg);
	extern void updateworld();
	extern void newmap(int size);
	extern void resetmap(bool empty);
	extern void startmap(const char *name, const char *reqname, bool empty = false);
	extern bool allowmove(physent *d);
	extern dynent *iterdynents(int i);
	extern int numdynents();
	extern void lighteffects(dynent *d, vec &color, vec &dir);
	extern void adddynlights();
	extern void particletrack(particle *p, uint type, int &ts, bool lastpass);
	extern void dynlighttrack(physent *owner, vec &o);
	extern bool mousemove(int dx, int dy, int x, int y, int w, int h);
	extern void project(int w, int h);
	extern void recomputecamera(int w, int h);
	extern void loadworld(stream *f, int maptype);
	extern void saveworld(stream *f);
	extern int localplayers();
	extern const char *gametitle();
	extern const char *gametext();
	extern int numanims();
	extern void findanims(const char *pattern, vector<int> &anims);
	extern void render();
	extern void renderavatar(bool early);
	extern bool thirdpersonview(bool viewonly = false);
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
	extern int numclients(int exclude = -1, bool nospec = false, int aitype = -1);
	extern int reserveclients();
	extern void clientdisconnect(int n, bool local = false);
	extern int clientconnect(int n, uint ip, bool local = false);
	extern bool allowbroadcast(int n);
	extern int peerowner(int n);
	extern void recordpacket(int chan, void *data, int len);
	extern void parsepacket(int sender, int chan, packetbuf &p);
	extern bool sendpackets();
	extern void queryreply(ucharbuf &req, ucharbuf &p);
	extern void serverupdate();
	extern const char *gameid();
	extern const char *gamename(int mode, int muts);
	extern void modecheck(int *mode, int *muts, int trying = 0);
	extern int gamever();
	extern const char *pickmap(const char *suggest = NULL);
	extern const char *choosemap(const char *suggest = NULL, int mode = -1, int muts = -1, int force = 0);
	extern void changemap(const char *name = NULL, int mode = -1, int muts = -1);
	extern bool canload(const char *type);
	extern void start();
	extern bool servcmd(int nargs, const char *cmd, const char *arg);
}
