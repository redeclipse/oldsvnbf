// the interface the game uses to access the engine

extern void lightent(extentity &e, float height = 8.0f);
extern void lightreaching(const vec &target, vec &color, vec &dir, extentity *e = 0, float ambient = 0.4f);
extern entity *brightestlight(const vec &target, const vec &dir);

enum { RAY_BB = 1, RAY_POLY = 3, RAY_ALPHAPOLY = 7, RAY_ENTS = 9, RAY_CLIPMAT = 16, RAY_SKIPFIRST = 32, RAY_EDITMAT = 64, RAY_SHADOW = 128, RAY_PASS = 256 };

extern float raycube   (const vec &o, const vec &ray,     float radius = 0, int mode = RAY_CLIPMAT, int size = 0, extentity *t = 0);
extern float raycubepos(const vec &o, vec &ray, vec &hit, float radius = 0, int mode = RAY_CLIPMAT, int size = 0);
extern float rayfloor  (const vec &o, vec &floor, int mode = 0, float radius = 0);
extern bool  raycubelos(vec &o, vec &dest, vec &hitpos);

extern bool isthirdperson();

extern void settexture(const char *name);

// octaedit

enum { EDIT_FACE = 0, EDIT_TEX, EDIT_MAT, EDIT_FLIP, EDIT_COPY, EDIT_PASTE, EDIT_ROTATE, EDIT_REPLACE, EDIT_DELCUBE, EDIT_REMIP };

struct selinfo
{
    int corner;
    int cx, cxs, cy, cys;
    ivec o, s;
    int grid, orient;
    int size() const    { return s.x*s.y*s.z; }
    int us(int d) const { return s[d]*grid; }
    bool operator==(const selinfo &sel) const { return o==sel.o && s==sel.s && grid==sel.grid && orient==sel.orient; }
};

struct editinfo;

extern bool editmode;

extern void freeeditinfo(editinfo *&e);
extern void cursorupdate();
extern void pruneundos(int maxremain = 0);
extern bool noedit(bool view = false);
extern void toggleedit();
extern void mpeditface(int dir, int mode, selinfo &sel, bool local);
extern void mpedittex(int tex, int allfaces, selinfo &sel, bool local);
extern void mpeditmat(int matid, selinfo &sel, bool local);
extern void mpflip(selinfo &sel, bool local);
extern void mpcopy(editinfo *&e, selinfo &sel, bool local);
extern void mppaste(editinfo *&e, selinfo &sel, bool local);
extern void mprotate(int cw, selinfo &sel, bool local);
extern void mpreplacetex(int oldtex, int newtex, selinfo &sel, bool local);
extern void mpdelcube(selinfo &sel, bool local);
extern void mpremip(bool local);

// command
#ifdef BFRONTIER
extern int variable(char *name, int min, int cur, int max, int *storage, void (*fun)(), int context = IDC_GLOBAL);
#else
extern int variable(char *name, int min, int cur, int max, int *storage, void (*fun)(), bool persist);
#endif
extern void setvar(char *name, int i, bool dofunc = false);
extern int getvar(char *name);
extern int getvarmin(char *name);
extern int getvarmax(char *name);
extern bool identexists(char *name);
extern ident *getident(char *name);
#ifdef BFRONTIER
extern bool addcommand(char *name, void (*fun)(), char *narg, int context = IDC_GLOBAL);
extern int execute(char *p, int context = IDC_GLOBAL);
extern char *executeret(char *p, int context = IDC_GLOBAL);
#else
extern bool addcommand(char *name, void (*fun)(), char *narg);
extern int execute(char *p);
extern char *executeret(char *p);
#endif
extern void exec(char *cfgfile);
extern bool execfile(char *cfgfile);
extern void alias(char *name, char *action);
extern const char *getalias(char *name);

// console
extern void keypress(int code, bool isdown, int cooked);
extern void rendercommand(int x, int y);
extern int renderconsole(int w, int h);
extern void conoutf(const char *s, ...);
extern char *getcurcommand();
extern void resetcomplete();
extern void complete(char *s);

// menus
extern vec menuinfrontofplayer();
extern void newgui(char *name, char *contents);
extern void showgui(char *name);

// world
#ifdef BFRONTIER
extern bool emptymap(int factor, bool force = false, char *mname = "untitled/base");
#else
extern bool emptymap(int factor, bool force);
#endif
extern bool enlargemap(bool force);
extern int findentity(int type, int index = 0);
extern void mpeditent(int i, const vec &o, int type, int attr1, int attr2, int attr3, int attr4, bool local);
extern int getworldsize();
extern int getmapversion();
extern bool insideworld(const vec &o);
extern void resettriggers();
extern void checktriggers();

// main
struct igame;

extern void fatal(char *s, char *o = "");
extern void keyrepeat(bool on);
extern void registergame(char *name, igame *ig);

#define REGISTERGAME(t, n, c, s) struct t : igame { t() { registergame(n, this); } igameclient *newclient() { return c; } igameserver *newserver() { return s; } } reg_##t

// rendertext
extern bool setfont(char *name);
extern void gettextres(int &w, int &h);
#ifndef BFRONTIER
extern void draw_text(const char *str, int left, int top, int r = 255, int g = 255, int b = 255, int a = 255);
extern void draw_textf(const char *fstr, int left, int top, ...);
#endif
extern int char_width(int c, int x = 0);
extern int text_width(const char *str, int limit = -1);
extern int text_visible(const char *str, int max);

// renderva
extern void adddynlight(const vec &o, float radius, const vec &color, int fade = 0, int peak = 0);
extern void cleardynlights();
extern void dynlightreaching(const vec &target, vec &color, vec &dir);

// rendergl
extern vec worldpos, camright, camup;
#ifndef BFRONTIER
extern void damageblend(int n);
#endif

// renderparticles
extern void render_particles(int time);
extern void regular_particle_splash(int type, int num, int fade, const vec &p, int delay = 0);
extern void particle_splash(int type, int num, int fade, const vec &p);
extern void particle_trail(int type, int fade, const vec &from, const vec &to);
extern void particle_text(const vec &s, char *t, int type, int fade = 2000);
extern void particle_meter(const vec &s, float val, int type, int fade = 1);
extern void particle_flare(const vec &p, const vec &dest, int fade, int type = 10, physent *owner = NULL);
extern void particle_fireball(const vec &dest, float max, int type);
extern void removetrackedparticles(physent *owner = NULL);

// worldio
#ifdef BFRONTIER
void setnames(const char *fname, const char *cname = 0);
#endif
extern void load_world(const char *mname, const char *cname = NULL);
extern void save_world(char *mname, bool nolms = false);

// physics
extern void moveplayer(physent *pl, int moveres, bool local);
extern bool moveplayer(physent *pl, int moveres, bool local, int curtime);
extern bool collide(physent *d, const vec &dir = vec(0, 0, 0), float cutoff = 0.0f);
extern bool bounce(physent *d, float secs, float elasticity = 0.8f, float waterfric = 3.0f);
extern void avoidcollision(physent *d, const vec &dir, physent *obstacle, float space);
extern void physicsframe();
extern void dropenttofloor(entity *e);
extern void vecfromyawpitch(float yaw, float pitch, int move, int strafe, vec &m);
extern void vectoyawpitch(const vec &v, float &yaw, float &pitch);
extern bool intersect(physent *d, vec &from, vec &to);
extern void updatephysstate(physent *d);
extern void cleardynentcache();
extern bool entinmap(dynent *d, bool avoidplayers = false);
extern void findplayerspawn(dynent *d, int forceent = -1);
// sound
#ifndef BFRONTIER
extern void playsound    (int n,   const vec *loc = NULL, extentity *ent = NULL);
extern void playsoundname(char *s, const vec *loc = NULL, int vol = 0);
extern void initsound();
#endif

// rendermodel
enum { MDL_CULL_VFC = 1<<0, MDL_CULL_DIST = 1<<1, MDL_CULL_OCCLUDED = 1<<2, MDL_CULL_QUERY = 1<<3, MDL_SHADOW = 1<<4, MDL_DYNSHADOW = 1<<5, MDL_TRANSLUCENT = 1<<6 };
enum { MDL_ATTACH_VWEP = 0, MDL_ATTACH_SHIELD, MDL_ATTACH_POWERUP };

struct model;
struct modelattach
{
    const char *name;
    int type, anim, basetime;
    model *m;
};

extern void startmodelbatches();
extern void endmodelbatches();
extern void rendermodel(vec &color, vec &dir, const char *mdl, int anim, int varseed, int tex, const vec &o, float yaw = 0, float pitch = 0, float speed = 0, int basetime = 0, dynent *d = NULL, int cull = MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED, modelattach *a = NULL);
extern void abovemodel(vec &o, const char *mdl);
extern void rendershadow(dynent *d);
extern void renderclient(dynent *d, const char *mdlname, modelattach *attachments, int attack, int attackdelay, int lastaction, int lastpain, float sink = 0);
extern void interpolateorientation(dynent *d, float &interpyaw, float &interppitch);
extern void setbbfrommodel(dynent *d, char *mdl);

// server
#define MAXCLIENTS 256                  // in a multiplayer game, can be arbitrarily changed
#define MAXTRANS 5000                  // max amount of data to swallow in 1 go

extern int maxclients;

enum { DISC_NONE = 0, DISC_EOP, DISC_CN, DISC_KICK, DISC_TAGT, DISC_IPBAN, DISC_PRIVATE, DISC_MAXCLIENTS, DISC_NUM };

extern void *getinfo(int i);
extern void sendf(int cn, int chan, const char *format, ...);
extern void sendfile(int cn, int chan, FILE *file, const char *format = "", ...);
extern void sendpacket(int cn, int chan, ENetPacket *packet, int exclude = -1);
extern int getnumclients();
extern uint getclientip(int n);
extern void putint(ucharbuf &p, int n);
extern int getint(ucharbuf &p);
extern void putuint(ucharbuf &p, int n);
extern int getuint(ucharbuf &p);
extern void sendstring(const char *t, ucharbuf &p);
extern void getstring(char *t, ucharbuf &p, int len = MAXTRANS);
extern void filtertext(char *dst, const char *src, bool whitespace = true, int len = sizeof(string)-1);
extern void disconnect_client(int n, int reason);
extern bool hasnonlocalclients();
extern bool haslocalclients();

// client
extern void c2sinfo(dynent *d, int rate = 33);
extern void sendpackettoserv(ENetPacket *packet, int chan);
extern void disconnect(int onlyclean = 0, int async = 0);
extern bool isconnected();
extern bool multiplayer(bool msg = true);
extern void neterr(char *s);
extern void gets2c();

// 3dgui
struct Texture;

enum { G3D_DOWN = 1, G3D_UP = 2, G3D_PRESSED = 4, G3D_ROLLOVER = 8 };

struct g3d_gui
{
    virtual ~g3d_gui() {}

    virtual void start(int starttime, float basescale, int *tab = NULL, bool allowinput = true) = 0;
    virtual void end() = 0;

    virtual int text(const char *text, int color, const char *icon = NULL) = 0;
    int textf(const char *fmt, int color, const char *icon = NULL, ...)
    {
        s_sprintfdlv(str, icon, fmt);
        return text(str, color, icon);
    }
    virtual int button(const char *text, int color, const char *icon = NULL) = 0;
    virtual void background(int color, int parentw = 0, int parenth = 0) = 0;

    virtual void pushlist() {}
    virtual void poplist() {}

	virtual void tab(const char *name, int color) = 0;
    virtual int title(const char *text, int color, const char *icon = NULL) = 0;
    virtual int image(const char *path, float scale, bool overlaid = false) = 0;
    virtual int texture(Texture *t, float scale) = 0;
    virtual void slider(int &val, int vmin, int vmax, int color, char *label = NULL) = 0;
    virtual void separator() = 0;
	virtual void progress(float percent) = 0;
	virtual void strut(int size) = 0;
    virtual void space(int size) = 0;
    virtual char *field(char *name, int color, int length, char *initval = "") = 0;
};

struct g3d_callback
{
    virtual ~g3d_callback() {}

    int starttime() { extern int totalmillis; return totalmillis; }

    virtual void gui(g3d_gui &g, bool firstpass) = 0;
};

extern void g3d_addgui(g3d_callback *cb, vec &origin, bool follow = false);
extern bool g3d_movecursor(int dx, int dy);
extern void g3d_cursorpos(float &x, float &y);
extern void g3d_resetcursor();

#ifdef BFRONTIER
struct sometype
{
	char *name; uchar id;
};

#define _dbg_ fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);

// client
struct serverinfo
{
	char *name; //NOTE if string then threading+sorting lead to overwriten values
	string map;
	string sdesc;
	int numplayers, ping, resolved;
	vector<int> attr;
	ENetAddress address;
};

enum { ST_EMPTY, ST_LOCAL, ST_TCPIP };

struct client					// server side version of "dynent" type
{
	int type;
	int num;
	ENetPeer *peer;
	string hostname;
	void *info;
};
extern vector<client *> clients;
extern int localclients, nonlocalclients;

extern void process(ENetPacket *packet, int sender, int chan);
extern void send_welcome(int n);
extern client &addclient();

#ifndef STANDALONE
extern ENetHost *clienthost;
extern ENetPeer *curpeer, *connpeer;
#endif // STANDALONE
extern ENetHost *serverhost;

// world
extern void getwatercolour(uchar *wcol);
extern void getlavacolour(uchar *lcol);

extern bool inside;
extern physent *hitplayer;
extern vec wall;
extern float walldistance;
extern int physicsfraction, physicsrepeat, minframetime;
extern int thirdperson, thirdpersonscale;

extern int gzgetint(gzFile f);
extern void gzputint(gzFile f, int x);
extern float gzgetfloat(gzFile f);
extern void gzputfloat(gzFile f, float x);

enum
{
	MAP_BFGZ = 0,
	MAP_OCTA,
	MAP_MAX
};

enum							// cube empty-space materials
{
	MAT_AIR = 0,				// the default, fill the empty space with air
	MAT_WATER,				  // fill with water, showing waves at the surface
	MAT_CLIP,					// collisions always treat cube as solid
	MAT_GLASS,				  // behaves like clip but is blended blueish
	MAT_NOCLIP,				 // collisions always treat cube as empty
	MAT_LAVA,					// fill with lava
	MAT_EDIT					// basis for the edit volumes of the above materials
};

extern int ambient, skylight, watercolour, lavacolour;

enum {
	WT_WATER = 0,
	WT_HURT,
	WT_KILL,
	WT_MAX
};

enum
{
	VIEW_FIRSTPERSON = 0,
	VIEW_THIRDPERSON,
	VIEW_MAX
};

extern string cgzname, pcfname, mcfname, picname, mapname;
extern int verbose, savebak;

extern bool otherclients(bool msg = true);

#endif
