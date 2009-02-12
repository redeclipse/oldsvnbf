// engine defs
#ifdef MAINCPP
#define EXTERN
#else
#define EXTERN extern
#endif

#define ENG_VERSION			80
#define ENG_NAME			"Blood Frontier"
#define ENG_RELEASE			"Beta 1"
#define ENG_URL				"www.bloodfrontier.com"
#define ENG_BLURB			"It's bloody fun!"

#define ENG_SERVER_PORT		28795
#define ENG_QUERY_PORT		28796
#define ENG_MASTER_PORT		28800
#define ENG_MASTER_HOST		"play.bloodfrontier.com"

#include "cube.h"
#include "iengine.h"
#include "igame.h"

#ifdef IRC
#include "irc.h"
#endif

#ifdef MASTERSERVER
extern void setupmaster();
extern void checkmaster();
extern void cleanupmaster();

extern int masterserver, masterport;
extern char *masterip;
#endif

#include "sound.h"

#ifndef STANDALONE
#include "world.h"
#include "octa.h"
#include "lightmap.h"
#include "bih.h"
#include "texture.h"
#include "model.h"

// GL_ARB_multitexture
extern PFNGLACTIVETEXTUREARBPROC		glActiveTexture_;
extern PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTexture_;
extern PFNGLMULTITEXCOORD2FARBPROC	 glMultiTexCoord2f_;
extern PFNGLMULTITEXCOORD3FARBPROC	 glMultiTexCoord3f_;
extern PFNGLMULTITEXCOORD4FARBPROC   glMultiTexCoord4f_;

// GL_ARB_vertex_buffer_object
extern PFNGLGENBUFFERSARBPROC       glGenBuffers_;
extern PFNGLBINDBUFFERARBPROC       glBindBuffer_;
extern PFNGLMAPBUFFERARBPROC        glMapBuffer_;
extern PFNGLUNMAPBUFFERARBPROC      glUnmapBuffer_;
extern PFNGLBUFFERDATAARBPROC       glBufferData_;
extern PFNGLBUFFERSUBDATAARBPROC    glBufferSubData_;
extern PFNGLDELETEBUFFERSARBPROC    glDeleteBuffers_;
extern PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubData_;

// GL_ARB_occlusion_query
extern PFNGLGENQUERIESARBPROC		glGenQueries_;
extern PFNGLDELETEQUERIESARBPROC	 glDeleteQueries_;
extern PFNGLBEGINQUERYARBPROC		glBeginQuery_;
extern PFNGLENDQUERYARBPROC		  glEndQuery_;
extern PFNGLGETQUERYIVARBPROC		glGetQueryiv_;
extern PFNGLGETQUERYOBJECTIVARBPROC  glGetQueryObjectiv_;
extern PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuiv_;

// GL_EXT_framebuffer_object
extern PFNGLBINDRENDERBUFFEREXTPROC		glBindRenderbuffer_;
extern PFNGLDELETERENDERBUFFERSEXTPROC	 glDeleteRenderbuffers_;
extern PFNGLGENFRAMEBUFFERSEXTPROC		 glGenRenderbuffers_;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC	 glRenderbufferStorage_;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  glCheckFramebufferStatus_;
extern PFNGLBINDFRAMEBUFFEREXTPROC		 glBindFramebuffer_;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC	  glDeleteFramebuffers_;
extern PFNGLGENFRAMEBUFFERSEXTPROC		 glGenFramebuffers_;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC	glFramebufferTexture2D_;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbuffer_;
extern PFNGLGENERATEMIPMAPEXTPROC		  glGenerateMipmap_;

// GL_EXT_draw_range_elements
extern PFNGLDRAWRANGEELEMENTSEXTPROC glDrawRangeElements_;

// GL_EXT_blend_minmax
extern PFNGLBLENDEQUATIONEXTPROC glBlendEquation_;

// GL_EXT_blend_color
extern PFNGLBLENDCOLOREXTPROC glBlendColor_;

// GL_EXT_multi_draw_arrays
extern PFNGLMULTIDRAWARRAYSEXTPROC   glMultiDrawArrays_;
extern PFNGLMULTIDRAWELEMENTSEXTPROC glMultiDrawElements_;

// GL_EXT_packed_depth_stencil
#ifndef GL_DEPTH_STENCIL_EXT
#define GL_DEPTH_STENCIL_EXT 0x84F9
#endif
#ifndef GL_DEPTH24_STENCIL8_EXT
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#endif

extern physent *camera1, camera;
extern bfgz hdr;
extern int worldscale;
extern vector<ushort> texmru;
extern int xtraverts, xtravertsva;
extern int curtexnum;
extern const ivec cubecoords[8];
extern const ushort fv[6][4];
extern const uchar fvmasks[64];
extern const uchar faceedgesidx[6][4];
extern bool inbetweenframes;

extern int curtime, lastmillis, totalmillis;
extern SDL_Surface *screen;
extern int zpass, glowpass;

// rendertext
struct font
{
	struct charinfo
	{
		short x, y, w, h;
	};

	char *name;
	Texture *tex;
	vector<charinfo> chars;
	short defaultw, defaulth;
	short offsetx, offsety, offsetw, offseth;
};

#define FONTH (curfont->defaulth)
#define FONTW (curfont->defaultw)
#define PIXELTAB (4*FONTW)

extern font *curfont;

// texture
extern int hwtexsize, hwcubetexsize, hwmaxaniso;

extern Texture *textureload(const char *name, int clamp = 0, bool mipit = true, bool msg = true);
extern void loadalphamask(Texture *t);
extern GLuint cubemapfromsky(int size);
extern Texture *cubemapload(const char *name, bool mipit = true, bool msg = true);
extern void drawcubemap(int size, int level, const vec &o, float yaw, float pitch, bool flipx, bool flipy, bool swapxy);
extern Slot	&lookuptexture(int tex, bool load = true);
extern void loadshaders();
extern void createtexture(int tnum, int w, int h, void *pixels, int clamp, bool mipit, GLenum component = GL_RGB, GLenum target = GL_TEXTURE_2D, bool compress = false, bool filter = true, int pw = 0, int ph = 0);
extern void renderpostfx();
extern void initenvmaps();
extern void genenvmaps();
extern ushort closestenvmap(const vec &o);
extern ushort closestenvmap(int orient, int x, int y, int z, int size);
extern GLuint lookupenvmap(ushort emid);
extern GLuint lookupenvmap(Slot &slot);
extern bool reloadtexture(Texture *t);
extern bool reloadtexture(const char *name);
extern void setuptexcompress();

// shader

extern int useshaders, shaderprecision;

// shadowmap

extern int shadowmap, shadowmapcasters;
extern bool shadowmapping;

extern bool isshadowmapcaster(const vec &o, float rad);
extern bool addshadowmapcaster(const vec &o, float xyrad, float zrad);
extern bool isshadowmapreceiver(vtxarray *va);
extern void rendershadowmap();
extern void pushshadowmap();
extern void popshadowmap();
extern void adjustshadowmatrix(const ivec &o, float scale);
extern void rendershadowmapreceivers();
extern void guessshadowdir();

// pvs
extern void clearpvs();
extern bool pvsoccluded(const ivec &bborigin, const ivec &bbsize);
extern bool waterpvsoccluded(int height);
extern void setviewcell(const vec &p);
extern void savepvs(gzFile f);
extern void loadpvs(gzFile f);
extern int getnumviewcells();

static inline bool pvsoccluded(const ivec &bborigin, int size)
{
    return pvsoccluded(bborigin, ivec(size, size, size));
}

// rendergl
extern bool hasVBO, hasDRE, hasOQ, hasTR, hasFBO, hasDS, hasTF, hasBE, hasBC, hasCM, hasNP2, hasTC, hasTE, hasMT, hasD3, hasAF, hasVP2, hasVP3, hasPP, hasMDA, hasTE3, hasTE4, hasVP, hasFP, hasGLSL, hasGM, hasNVFB, hasSGIDT, hasSGISH, hasDT, hasSH, hasNVPCF, hasRN;
extern int hasstencil;

extern bool envmapping, renderedgame;
extern glmatrixf mvmatrix, projmatrix, mvpmatrix, invmvmatrix, invmvpmatrix;

extern float cursorx, cursory, aimx, aimy;
extern vec cursordir;

extern GLenum colormask[3];
#define COLORMASK colormask[0], colormask[1], colormask[2]
#define SAVECOLORMASK \
    GLenum oldcolormask[3]; \
    memcpy(oldcolormask, colormask, sizeof(oldcolormask)); \
    setcolormask(); \
    if(memcmp(colormask, oldcolormask, sizeof(oldcolormask))) glColorMask(COLORMASK, GL_TRUE);
#define RESTORECOLORMASK \
    if(memcmp(colormask, oldcolormask, sizeof(oldcolormask))) \
    { \
        memcpy(colormask, oldcolormask, sizeof(oldcolormask)); \
        glColorMask(COLORMASK, GL_TRUE); \
    }

extern void gl_checkextensions();
extern void gl_init(int w, int h, int bpp, int depth, int fsaa);
extern void cleangl();

extern void vecfromcursor(float x, float y, float z, vec &dir);
extern void vectocursor(vec &v, float &x, float &y, float &z);
extern void findorientation(vec &o, float yaw, float pitch, vec &pos);
extern void rendergame();
extern void renderavatar(bool early);
extern void invalidatepostfx();
extern void gl_drawframe(int w, int h);
extern void enablepolygonoffset(GLenum type);
extern void disablepolygonoffset(GLenum type);
extern void calcspherescissor(const vec &center, float size, float &sx1, float &sy1, float &sx2, float &sy2);
extern int pushscissor(float sx1, float sy1, float sx2, float sy2);
extern void popscissor();
extern void setfogplane(const plane &p, bool flush = false);
extern void setfogplane(float scale = 0, float z = 0, bool flush = false, float fadescale = 0, float fadeoffset = 0);
extern void setcolormask(bool r = true, bool g = true, bool b = true);

extern const char *loadback, *loadbackinfo;
extern float loadprogress;
extern void loadbackground(int w, int h, Texture *t = NULL);
extern void computescreen(const char *text = NULL, Texture *t = NULL, const char *overlaytext = NULL);
extern void renderprogress(float bar1, const char *text1, float bar2 = 0, const char *text2 = NULL, GLuint tex = 0);

// renderextras
extern void render3dbox(vec &o, float tofloor, float toceil, float xradius, float yradius = 0);

// octa
extern cube *newcubes(uint face = F_EMPTY);
extern cubeext *newcubeext(cube &c);
extern void getcubevector(cube &c, int d, int x, int y, int z, ivec &p);
extern void setcubevector(cube &c, int d, int x, int y, int z, ivec &p);
extern int familysize(cube &c);
extern void freeocta(cube *c);
extern void discardchildren(cube &c);
extern void optiface(uchar *p, cube &c);
extern void validatec(cube *c, int size);
extern bool isvalidcube(cube &c);
extern cube &lookupcube(int tx, int ty, int tz, int tsize = 0);
extern cube &neighbourcube(int x, int y, int z, int size, int rsize, int orient);
extern int lookupmaterial(const vec &o);
extern void newclipplanes(cube &c);
extern void freeclipplanes(cube &c);
extern void forcemip(cube &c);
extern bool subdividecube(cube &c, bool fullcheck=true, bool brighten=true);
extern void converttovectorworld();
extern int faceverts(cube &c, int orient, int vert);
extern int faceconvexity(cube &c, int orient);
extern void calcvert(cube &c, int x, int y, int z, int size, vvec &vert, int i, bool solid = false);
extern void calcvert(cube &c, int x, int y, int z, int size, vec &vert, int i, bool solid = false);
extern int calcverts(cube &c, int x, int y, int z, int size, vvec *verts, bool *usefaces);
extern uint faceedges(cube &c, int orient);
extern bool collapsedface(uint cfe);
extern bool touchingface(cube &c, int orient);
extern bool flataxisface(cube &c, int orient);
extern int genclipplane(cube &c, int i, vec *v, plane *clip);
extern void genclipplanes(cube &c, int x, int y, int z, int size, clipplanes &p);
extern bool visibleface(cube &c, int orient, int x, int y, int z, int size, uchar mat = MAT_AIR, uchar nmat = MAT_AIR, uchar matmask = MATF_VOLUME);
extern int visibletris(cube &c, int orient, int x, int y, int z, int size);
extern int visibleorient(cube &c, int orient);
extern bool threeplaneintersect(plane &pl1, plane &pl2, plane &pl3, vec &dest);
extern void genvectorvert(const ivec &p, cube &c, ivec &v);
extern void freemergeinfo(cube &c);
extern void genmergedverts(cube &cu, int orient, const ivec &co, int size, const mergeinfo &m, vvec *vv, plane *p = NULL);
extern int calcmergedsize(int orient, const ivec &co, int size, const mergeinfo &m, const vvec *vv);
extern void invalidatemerges(cube &c, bool msg);
extern void calcmerges();

struct cubeface : mergeinfo
{
	cube *c;
};

extern int mergefaces(int orient, cubeface *m, int sz);
extern void mincubeface(cube &cu, int orient, const ivec &o, int size, const mergeinfo &orig, mergeinfo &cf);

static inline uchar octantrectangleoverlap(const ivec &c, int size, const ivec &o, const ivec &s)
{
    uchar p = 0xFF; // bitmask of possible collisions with octants. 0 bit = 0 octant, etc
    ivec v(c);
    v.add(size);
    if(v.z <= o.z)     p &= 0xF0; // not in a -ve Z octant
    if(v.z >= o.z+s.z) p &= 0x0F; // not in a +ve Z octant
    if(v.y <= o.y)     p &= 0xCC; // not in a -ve Y octant
    if(v.y >= o.y+s.y) p &= 0x33; // etc..
    if(v.x <= o.x)     p &= 0xAA;
    if(v.x >= o.x+s.x) p &= 0x55;
    return p;
}

static inline bool insideworld(const vec &o)
{
    return o.x>=0 && o.x<hdr.worldsize && o.y>=0 && o.y<hdr.worldsize && o.z>=0 && o.z<hdr.worldsize;
}

static inline bool insideworld(const ivec &o)
{
    return uint(o.x)<uint(hdr.worldsize) && uint(o.y)<uint(hdr.worldsize) && uint(o.z)<uint(hdr.worldsize);
}

// ents
extern bool haveselent();
extern undoblock *copyundoents(undoblock *u);
extern void pasteundoents(undoblock *u);

// octaedit
extern selinfo sel;
extern void cancelsel();
extern void render_texture_panel(int w, int h);
extern void addundo(undoblock *u);
extern void commitchanges(bool force = false);
extern editinfo *localedit;

// octarender
extern void octarender();
extern void allchanged(bool load = false);
extern void vaclearc(cube *c);
extern vtxarray *newva(int x, int y, int z, int size);
extern void destroyva(vtxarray *va, bool reparent = true);
extern bool readva(vtxarray *va, ushort *&edata, uchar *&vdata);
extern void updatevabb(vtxarray *va, bool force = false);
extern void updatevabbs(bool force = false);

// renderva
extern GLuint fogtex;

extern void visiblecubes(float fov, float fovy);
extern void reflectvfcP(float z, float minyaw = -M_PI, float maxyaw = M_PI, float minpitch = -M_PI, float maxpitch = M_PI);
extern void restorevfcP();
extern void createfogtex();
extern void rendergeom(float causticspass = 0, bool fogpass = false);
extern void rendermapmodels();
extern void renderreflectedgeom(bool causticspass = false, bool fogpass = false);
extern void renderreflectedmapmodels();
extern void renderoutline();
extern bool rendersky(bool explicitonly = false);

extern int isvisiblesphere(float rad, const vec &cv);
extern bool bboccluded(const ivec &bo, const ivec &br);
extern occludequery *newquery(void *owner);
extern bool checkquery(occludequery *query, bool nowait = false);
extern void resetqueries();
extern int getnumqueries();
extern void drawbb(const ivec &bo, const ivec &br, const vec &camera = camera1->o, int scale = 0, const ivec &origin = ivec(0, 0, 0));

#define startquery(query) { glBeginQuery_(GL_SAMPLES_PASSED_ARB, ((occludequery *)(query))->id); }
#define endquery(query) \
	{ \
		glEndQuery_(GL_SAMPLES_PASSED_ARB); \
		extern int ati_oq_bug; \
		if(ati_oq_bug) glFlush(); \
	}

// dynlight

extern void updatedynlights();
extern int finddynlights();
extern void calcdynlightmask(vtxarray *va);
extern int setdynlights(vtxarray *va, const ivec &vaorigin);
extern bool getdynlight(int n, vec &o, float &radius, vec &color);

// material

extern int showmat;

extern sometype materials[], textypes[];
extern const char *findmaterialname(int type);
extern int findmaterial(const char *name, bool tryint = false);
extern void genmatsurfs(cube &c, int cx, int cy, int cz, int size, vector<materialsurface> &matsurfs, uchar &vismask, uchar &clipmask);
extern void rendermatsurfs(materialsurface *matbuf, int matsurfs);
extern void rendermatgrid(materialsurface *matbuf, int matsurfs);
extern int optimizematsurfs(materialsurface *matbuf, int matsurfs);
extern void setupmaterials(int start = 0, int len = 0);
extern void rendermaterials();
extern void drawmaterial(int orient, int x, int y, int z, int csize, int rsize, float offset);
extern int visiblematerial(cube &c, int orient, int x, int y, int z, int size, uchar matmask = MATF_VOLUME);

// water
extern int refracting;
extern bool reflecting, fading, fogging;
extern float reflectz;
extern int reflectdist, vertwater, refractfog, waterrefract, waterreflect, waterfade, caustics, waterfallrefract, waterfog, lavafog;

extern void cleanreflections();
extern void queryreflections();
extern void drawreflections();
extern void renderwater();
extern void renderlava(materialsurface &m, Texture *tex, float scale);
extern void getwatercolour(uchar *wcol);
extern void getwaterfallcolour(uchar *fcol);
extern void getlavacolour(uchar *lcol);
extern void loadcaustics(bool force = false);
extern void preloadwatershaders(bool force = false);

// glare
extern bool glaring;

extern void drawglaretex();
extern void addglare();

// depthfx
extern bool depthfxing;

extern void drawdepthfxtex();

// server
extern vector<char *> gameargs;
extern void initgame();
extern void cleanupserver();
extern void serverslice();

extern uchar *retrieveservers(uchar *buf, int buflen);
extern void localclienttoserver(int chan, ENetPacket *);
extern void lanconnect();
extern bool serveroption(char *opt);
extern void localconnect();
extern void localdisconnect();

// serverbrowser
extern void addserver(const char *name, int port, int qport);
extern void writeservercfg();

// client
extern void servertoclient(int chan, uchar *buf, int len);
extern void localservertoclient(int chan, uchar *buf, int len);
extern bool connected(bool attempt = true);
extern void connects(const char *name = NULL, int port = ENG_SERVER_PORT, int qport = ENG_QUERY_PORT, const char *password = NULL);
extern void reconnect(), lanconnect();
extern void abortconnect(bool msg = true);
extern void clientkeepalive();
extern ENetHost *clienthost;
extern ENetPeer *curpeer, *connpeer;

// console
extern void writebinds(FILE *f);
extern void writecompletions(FILE *f);
extern const char *addreleaseaction(const char *s);
extern const char *getkeyname(int code);

extern bool saycommandon;

// main
enum
{
    NOT_INITING = 0,
    INIT_LOAD,
    INIT_RESET
};
extern int initing;

enum
{
    CHANGE_GFX   = 1<<0,
    CHANGE_SOUND = 1<<1
};
extern bool initwarning(const char *desc, int level = INIT_RESET, int type = CHANGE_GFX);
extern void resetcursor(bool warp = true, bool reset = true);
extern int compresslevel, imageformat;

extern void pushevent(const SDL_Event &e);
extern bool interceptkey(int sym);
extern void getfps(int &fps, int &bestdiff, int &worstdiff);

// menu
#define GUI_TITLE_COLOR  0xFFDD88
#define GUI_BUTTON_COLOR 0xFFFFFF
#define GUI_TEXT_COLOR  0xDDFFDD

extern int cmenustart, cmenutab;
extern g3d_gui *cgui;
struct menu : g3d_callback
{
    char *name, *header, *contents, *initscript;
    int passes;

    menu() : name(NULL), header(NULL), contents(NULL), initscript(NULL), passes(0) {}

    void gui(g3d_gui &g, bool firstpass)
    {
        cgui = &g;
        extern menu *cmenu;
        cmenu = this;
        cgui->start(cmenustart, 0.03f, &cmenutab, true);
        cgui->tab(header ? header : name, GUI_TITLE_COLOR);
		if(!passes && initscript && *initscript)
			execute(initscript);
        if(contents && *contents) execute(contents);
        cgui->end();
        cmenu = NULL;
        cgui = NULL;
		passes++;
    }

    virtual void clear() {}
};
extern menu *cmenu;
extern hashtable<const char *, menu> guis;
extern vector<menu *> guistack;
extern vector<char *> executelater;
extern bool shouldclearmenu, clearlater;

extern void menuprocess();
extern void addchange(const char *desc, int type);
extern void clearchanges(int type);

// physics
extern bool pointincube(const clipplanes &p, const vec &v);
extern bool overlapsdynent(const vec &o, float radius);
extern void rotatebb(vec &center, vec &radius, int yaw);
extern float shadowray(const vec &o, const vec &ray, float radius, int mode, extentity *t = NULL);
extern bool getsight(vec &o, float yaw, float pitch, vec &q, vec &v, float mdist, float fovx, float fovy);

// world
extern void entitiesinoctanodes();
extern void attachentities();
extern void freeoctaentities(cube &c);
extern bool pointinsel(selinfo &sel, vec &o);

extern void resetmap(bool empty);

// rendermodel
struct mapmodelinfo { string name; model *m; };
extern vector<mapmodelinfo> mapmodels;
extern void mmodel(char *name);
extern void resetmapmodels();

extern bool matchanim(const char *name, const char *pattern);
extern void loadskin(const char *dir, const char *altdir, Texture *&skin, Texture *&masks);
extern model *loadmodel(const char *name, int i = -1, bool msg = false);
extern mapmodelinfo &getmminfo(int i);
extern void startmodelquery(occludequery *query);
extern void endmodelquery();
extern void preloadmodelshaders();

// renderparticles
extern void particleinit();
extern void clearparticles();
extern void makeparticle(vec &o, int attr1, int attr2, int attr3, int attr4, int attr5);
extern void makeparticles(entity &e);
extern void updateparticles();

// decal
extern void initdecals();
extern void cleardecals();
extern void renderdecals(int time);

// blob

enum
{
    BLOB_STATIC = 0,
    BLOB_DYNAMIC
};

extern int showblobs;

extern void initblobs(int type = -1);
extern void resetblobs();
extern void renderblob(int type, const vec &o, float radius, float fade = 1);
extern void flushblobs();

// rendersky
extern int explicitsky;
extern double skyarea;

extern void drawskybox(int farplane, bool limited);
extern bool limitsky();

// 3dgui
extern char *guiskintex, *guioverlaytex, *guislidertex;
extern void g3d_render();
extern bool g3d_windowhit(bool on, bool act);
extern void g3d_mainmenu();
extern bool g3d_active(bool hit = true, bool pass = true);
extern void g3d_texturemenu();

// 3dgui
extern int cleargui(int n = 0);

// octaedit
extern void replacetexcube(cube &c, int oldtex, int newtex);

// skybox
extern void loadsky(char *basename);

// main
extern void setcaption(const char *text);
extern int grabinput, colorpos, curfps, bestfps, worstfps, bestfpsdiff, worstfpsdiff, maxfps, minfps,
	autoadjust, autoadjustlevel, autoadjustmin, autoadjustmax, autoadjustrate;
extern char colorstack[10];
extern int getmatvec(vec v);

// editing
extern int fullbright, fullbrightlevel;
extern vector<int> entgroup;

extern void newentity(int type, int a1, int a2, int a3, int a4, int a5);
extern void newentity(vec &v, int type, int a1, int a2, int a3, int a4, int a5);

// menu
enum
{
	MN_BACK = 0,
	MN_INPUT,
	MN_MAX
};

// console
struct cline { char *cref; int outtime; };
extern vector<cline> conlines;
extern void rehash(bool reload = true);

// command
extern char *gettime(char *format);
extern char *getmapname();
extern int getmapversion();
extern int getmaprevision();
extern char *getmaptitle();

// rendergl
#define RENDERPUSHX			8.0f
#define RENDERPUSHZ			0.1f

extern int dynentsize, axfps, lastmillis, totalmillis;
extern float hudblend;
extern int ambient, skylight, watercolour, lavacolour;
extern float curfov, fovy, aspect;

extern void project(float fovy, float aspect, int farplane, bool flipx = false, bool flipy = false, bool swapxy = false, float zscale = 1);
extern void transplayer();

extern void usetexturing(bool on);
extern void renderprimitive(bool on);
extern void renderline(vec &fr, vec &to, float r = 255.f, float g = 255.f, float b = 255.f, bool nf = false);
extern void rendertris(vec &fr, float yaw, float pitch, float size = 1.f, float r = 255.f, float g = 255.f, float b = 255.f, bool fill = true, bool nf = false);
extern void renderlineloop(vec &o, float xradius, float yradius, float zradius, float z = 0.f, int type = 0, float r = 255.f, float g = 255.f, float b = 255.f, bool nf = false);
extern void renderdir(vec &o, float yaw, float pitch, bool nf = true);
extern void renderradius(vec &o, float xradius, float yradius, float zradius, bool nf = true);

extern bool rendericon(const char *icon, int x, int y, int xs = 120, int ys = 120);
#define rendermainview (!shadowmapping && !envmapping && !reflecting && !refracting)
#define renderatopview (glaring)
#define rendernormally (rendermainview || renderatopview)

extern void drawslice(float start, float length, float x, float y, float size);
extern void drawfadedslice(float start, float length, float x, float y, float size, float alpha, float r = 1.f, float g = 1.f, float b = 1.f, float minsize = 0.25f);

// grass
extern void generategrass();
extern void rendergrass();

// blendmap
extern bool setblendmaporigin(const ivec &o, int size);
extern bool hasblendmap();
extern uchar lookupblendmap(const vec &pos);
extern void resetblendmap();
extern void enlargeblendmap();
extern void optimizeblendmap();
extern void renderblendbrush(GLuint tex, float x, float y, float w, float h);
extern void renderblendbrush();
extern bool loadblendmap(gzFile f);
extern void saveblendmap(gzFile f);
extern uchar shouldsaveblendmap();

#endif // STANDALONE
