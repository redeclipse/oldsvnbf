// the interface the game uses to access the engine

#define ENGVERSION		61					// engine version, integer is divided by 100.f
#define ENGNAME			"Blood Frontier"	// engine namn
#define ENGRELEASE		"Alpha 2"			// engine release name

#ifdef __GNUC__
#define _dbg_ fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
#else
#define _dbg_ fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
#endif

struct sometype
{
	const char *name; uchar id;
};

extern void lightent(extentity &e, float height = 8.0f);
extern void lightreaching(const vec &target, vec &color, vec &dir, extentity *e = 0, float ambient = 0.4f);
extern entity *brightestlight(const vec &target, const vec &dir);

enum { RAY_BB = 1, RAY_POLY = 3, RAY_ALPHAPOLY = 7, RAY_ENTS = 9, RAY_CLIPMAT = 16, RAY_SKIPFIRST = 32, RAY_EDITMAT = 64, RAY_SHADOW = 128, RAY_PASS = 256 };

extern float raycube   (const vec &o, const vec &ray,     float radius = 0, int mode = RAY_CLIPMAT, int size = 0, extentity *t = 0);
extern float raycubepos(const vec &o, const vec &ray, vec &hit, float radius = 0, int mode = RAY_CLIPMAT, int size = 0);
extern float rayfloor  (const vec &o, vec &floor, int mode = 0, float radius = 0);
extern bool  raycubelos(const vec &o, const vec &dest, vec &hitpos);

extern bool isthirdperson();

extern void settexture(const char *name, bool clamp = false);

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
extern int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), bool persist, bool world);
extern float fvariable(const char *name, float cur, float *storage, void (*fun)(), bool persist, bool world);
extern char *svariable(const char *name, const char *cur, char **storage, void (*fun)(), bool persist, bool world);
extern void setvar(const char *name, int i, bool dofunc = false);
extern void setfvar(const char *name, float f, bool dofunc = false);
extern void setsvar(const char *name, const char *str, bool dofunc = false);
extern int getvar(const char *name);
extern int getvarmin(const char *name);
extern int getvarmax(const char *name);
extern bool identexists(const char *name);
extern ident *getident(const char *name);
extern bool addcommand(const char *name, void (*fun)(), const char *narg);
extern int execute(const char *p);
extern char *executeret(const char *p);
extern void exec(const char *cfgfile);
extern bool execfile(const char *cfgfile);
extern void alias(const char *name, const char *action);
extern void worldalias(const char *name, const char *action);
extern const char *getalias(const char *name);

// console
extern void keypress(int code, bool isdown, int cooked);
extern int rendercommand(int x, int y, int w);
extern int renderconsole(int w, int h);
extern void conoutf(const char *s, ...);
extern char *getcurcommand();
extern void resetcomplete();
extern void complete(char *s);

// menus
extern vec menuinfrontofplayer();
extern void newgui(char *name, char *contents, char *header = NULL);
extern void showgui(const char *name);

// world
extern bool emptymap(int factor, bool force = false, char *mname = NULL, bool nocfg = false);
extern bool enlargemap(bool force);
extern int findentity(int type, int index = 0, int attr1 = -1, int attr2 = -1);
extern void mpeditent(int i, const vec &o, int type, int attr1, int attr2, int attr3, int attr4, bool local);
extern int getworldsize();
extern int getmapversion();

// main
struct igame;

extern void fatal(const char *s, ...);
extern void keyrepeat(bool on);
extern void registergame(const char *name, igame *ig);

#define REGISTERGAME(t, n, c, s) struct t : igame { t() { registergame(n, this); } igameclient *newclient() { return c; } igameserver *newserver() { return s; } } reg_##t

// rendertext
enum
{
	AL_LEFT = 0,
	AL_CENTER,
	AL_RIGHT
};
extern bool setfont(const char *name);
extern bool pushfont(const char *name);
extern bool popfont(int num);
extern void gettextres(int &w, int &h);
extern void draw_text(const char *str, int rleft, int rtop, int r = 255, int g = 255, int b = 255, int a = 255, bool s = true, int cursor = -1, int maxwidth = -1);
extern void draw_textx(const char *fstr, int left, int top, int r = 255, int g = 255, int b = 255, int a = 255, bool s = true, int align = AL_LEFT, int cursor = -1, int maxwidth = -1, ...);
extern void draw_textf(const char *fstr, int left, int top, ...);
extern int text_width(const char *str);
extern void text_bounds(const char *str, int &width, int &height, int maxwidth = -1);
extern int text_visible(const char *str, int hitx, int hity, int maxwidth = -1);
extern void text_pos(const char *str, int cursor, int &cx, int &cy, int maxwidth);

// renderva
enum
{
    DL_SHRINK = 1<<0,
    DL_EXPAND = 1<<1,
    DL_FLASH  = 1<<2
};

extern void adddynlight(const vec &o, float radius, const vec &color, int fade = 0, int peak = 0, int flags = 0, float initradius = 0, const vec &initcolor = vec(0, 0, 0));
extern void dynlightreaching(const vec &target, vec &color, vec &dir);

// rendergl
extern vec worldpos, camdir, camright, camup;

// renderparticles
extern void render_particles(int time);
extern void regular_particle_splash(int type, int num, int fade, const vec &p, int delay = 0);
extern void particle_splash(int type, int num, int fade, const vec &p);
extern void particle_trail(int type, int fade, const vec &from, const vec &to);
extern void particle_text(const vec &s, const char *t, int type, int fade = 2000);
extern void particle_meter(const vec &s, float val, int type, int fade = 1);
extern void particle_flare(const vec &p, const vec &dest, int fade, int type = 10, physent *owner = NULL);
extern void particle_fireball(const vec &dest, float maxsize, int type, int fade = -1);
extern void removetrackedparticles(physent *owner = NULL);

// decal
enum
{
    DECAL_SCORCH = 0,
    DECAL_BLOOD,
    DECAL_BULLET
};

extern void adddecal(int type, const vec &center, const vec &surface, float radius, const bvec &color = bvec(0xFF, 0xFF, 0xFF), int info = 0);

// worldio
extern void setnames(const char *fname, const char *cname = NULL);
extern void load_world(const char *mname, const char *cname = NULL);
extern void save_world(const char *mname, bool nolms = false);

// physics
extern bool ellipsecollide(physent *d, const vec &dir, const vec &o, float yaw, float xr, float yr,  float hi, float lo);
extern bool rectcollide(physent *d, const vec &dir, const vec &o, float xr, float yr,  float hi, float lo, uchar visible = 0xFF, bool collideonly = true, float cutoff = 0);
extern bool collide(physent *d, const vec &dir = vec(0, 0, 0), float cutoff = 0.0f, bool playercol = true);
extern bool plcollide(physent *d, const vec &dir);
extern void vecfromyawpitch(float yaw, float pitch, int move, int strafe, vec &m);
extern void vectoyawpitch(const vec &v, float &yaw, float &pitch);
extern bool intersect(physent *d, vec &from, vec &to);
extern bool insidesphere(vec &d, float h1, float r1, vec &v, float h2, float r2);
extern const vector<physent *> &checkdynentcache(int x, int y);
extern void updatedynentcache(physent *d);
extern void cleardynentcache();

extern int dynentsize, ambient, fov;

// rendermodel
enum { MDL_CULL_VFC = 1<<0, MDL_CULL_DIST = 1<<1, MDL_CULL_OCCLUDED = 1<<2, MDL_CULL_QUERY = 1<<3, MDL_SHADOW = 1<<4, MDL_DYNSHADOW = 1<<5, MDL_LIGHT = 1<<6, MDL_DYNLIGHT = 1<<7, MDL_TRANSLUCENT = 1<<8, MDL_FULLBRIGHT = 1<<9 };

struct model;
struct modelattach
{
    const char *name, *tag;
    int anim, basetime;
    model *m;
};

extern void startmodelbatches();
extern void endmodelbatches();
extern void rendermodel(entitylight *light, const char *mdl, int anim, const vec &o, float yaw = 0, float pitch = 0, float roll = 0, int cull = MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED | MDL_LIGHT, dynent *d = NULL, modelattach *a = NULL, int basetime = 0, float speed = 0);
extern void abovemodel(vec &o, const char *mdl);
extern void rendershadow(dynent *d);
extern void renderclient(dynent *d, bool local, const char *mdlname, modelattach *attachments, int attack, int attackdelay, int lastaction, int lastpain, float sink = 0);
extern void setbbfrommodel(dynent *d, const char *mdl);

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
extern void putfloat(ucharbuf &p, float f);
extern float getfloat(ucharbuf &p);
extern void sendstring(const char *t, ucharbuf &p);
extern void getstring(char *t, ucharbuf &p, int len = MAXTRANS);
extern void filtertext(char *dst, const char *src, bool whitespace = true, int len = sizeof(string)-1);
extern void disconnect_client(int n, int reason);
extern bool hasnonlocalclients();

// client
struct serverinfo
{
    enum { UNRESOLVED = 0, RESOLVING, RESOLVED };

    string name;
    string map;
    string sdesc;
    int numplayers, ping, resolved;
    vector<int> attr;
    ENetAddress address;

    serverinfo()
     : numplayers(0), ping(999), resolved(UNRESOLVED)
    {
        name[0] = map[0] = sdesc[0] = '\0';
    }
};

extern void c2sinfo(int rate = 33);
extern void sendpackettoserv(ENetPacket *packet, int chan);
extern void disconnect(int onlyclean = 0, int async = 0);
extern bool isconnected();
extern bool multiplayer(bool msg = true);
extern void neterr(const char *s);
extern void gets2c();

// 3dgui
struct Texture;

enum { G3D_DOWN = 1, G3D_UP = 2, G3D_PRESSED = 4, G3D_ROLLOVER = 8, G3D_ALTERNATE = 16 };

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
    virtual int image(Texture *t, float scale, bool overlaid = false) = 0;
    virtual int texture(Texture *t, float scale, int rotate = 0, int xoff = 0, int yoff = 0, Texture *glowtex = NULL, const vec &glowcolor = vec(1, 1, 1)) = 0;
    virtual void slider(int &val, int vmin, int vmax, int color, char *label = NULL) = 0;
    virtual void separator() = 0;
	virtual void progress(float percent) = 0;
	virtual void strut(int size) = 0;
    virtual void space(int size) = 0;
    virtual char *field(const char *name, int color, int length, const char *initval = "") = 0;
};

struct g3d_callback
{
    virtual ~g3d_callback() {}

    int starttime() { extern int totalmillis; return totalmillis; }

    virtual void gui(g3d_gui &g, bool firstpass) = 0;
};

extern void g3d_addgui(g3d_callback *cb);

// client
enum { ST_EMPTY, ST_TCPIP, ST_REMOTE };

struct client					// server side version of "dynent" type
{
	int type;
	int num;
	ENetPeer *peer;
	string hostname;
	void *info;
};
extern vector<client *> clients;
extern int nonlocalclients;

extern void process(ENetPacket *packet, int sender, int chan);
extern void send_welcome(int n);
extern void delclient(int n);
extern int addclient(int type = ST_EMPTY);

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
	MAT_WATER,				  	// fill with water, showing waves at the surface
	MAT_CLIP,					// collisions always treat cube as solid
	MAT_GLASS,				  	// behaves like clip but is blended blueish
	MAT_NOCLIP,					// collisions always treat cube as empty
	MAT_LAVA,					// fill with lava
    MAT_AICLIP,                 // clip ai only
    MAT_DEATH,                  // force player suicide
	MAT_EDIT					// basis for the edit volumes of the above materials
};

extern int ambient, skylight, watercolour, lavacolour;
extern string cgzname, mcfname, picname, mapname;
extern int verbose, savebak;
