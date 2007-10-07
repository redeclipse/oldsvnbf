#include "cube.h"
#include "iengine.h"
#include "igame.h"

extern igameclient	 *cl;
extern igameserver	 *sv;
extern iclientcom	  *cc;
#ifdef BFRONTIER // extra sub modules
extern ibotcom *bc;
extern iphysics *ph;
#endif
extern icliententities *et;

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

// GL_ARB_vertex_buffer_object
extern PFNGLGENBUFFERSARBPROC	glGenBuffers_;
extern PFNGLBINDBUFFERARBPROC	glBindBuffer_;
extern PFNGLMAPBUFFERARBPROC	 glMapBuffer_;
extern PFNGLUNMAPBUFFERARBPROC	glUnmapBuffer_;
extern PFNGLBUFFERDATAARBPROC	glBufferData_;
extern PFNGLBUFFERSUBDATAARBPROC glBufferSubData_;
extern PFNGLDELETEBUFFERSARBPROC glDeleteBuffers_;

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

// GL_EXT_packed_depth_stencil
#ifndef GL_DEPTH_STENCIL_EXT
#define GL_DEPTH_STENCIL_EXT 0x84F9
#endif
#ifndef GL_DEPTH24_STENCIL8_EXT
#define GL_DEPTH24_STENCIL8_EXT 0x88F0
#endif

extern dynent *player;
extern physent *camera1;				// special ent that acts as camera, same object as player1 in FPS mode

extern header hdr;					  // current map header
extern vector<ushort> texmru;
extern int xtraverts, xtravertsva;
extern vector<vertex> verts;			// the vertex array for all world rendering
extern int curtexnum;
extern const ivec cubecoords[8];
extern const ushort fv[6][4];
extern const uchar faceedgesidx[6][4];
extern bool inbetweenframes;

extern int curtime;					 // current frame time
extern int lastmillis;				  // last time
extern int totalmillis;				 // total elapsed time
extern SDL_Surface *screen;
extern int zpass, glowpass;

extern vector<int> entgroup;

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
#define MINRESW 640
#define MINRESH 480

extern font *curfont;

// texture
extern int hwtexsize, hwcubetexsize;

extern Texture *textureload(const char *name, int clamp = 0, bool mipit = true, bool msg = true);
extern void loadalphamask(Texture *t);
extern GLuint cubemapfromsky(int size);
extern Texture *cubemapload(const char *name, bool mipit = true, bool msg = true);
extern void drawcubemap(int size, const vec &o, float yaw, float pitch);
extern Slot	&lookuptexture(int tex, bool load = true);
extern Shader  *lookupshader(int slot);
extern void createtexture(int tnum, int w, int h, void *pixels, int clamp, bool mipit, GLenum component = GL_RGB, GLenum target = GL_TEXTURE_2D);
extern void renderfullscreenshader(int w, int h);
extern void initenvmaps();
extern void genenvmaps();
extern ushort closestenvmap(const vec &o);
extern ushort closestenvmap(int orient, int x, int y, int z, int size);
extern GLuint lookupenvmap(ushort emid);

// shadowmap

extern int shadowmap;
extern bool shadowmapping;

extern bool isshadowmapcaster(const vec &o, float rad);
extern bool addshadowmapcaster(const vec &o, float xyrad, float zrad);
extern bool isshadowmapreceiver(vtxarray *va);
extern void rendershadowmap();
extern void pushshadowmap();
extern void popshadowmap();
extern void adjustshadowmatrix(const ivec &o, float scale);
extern void rendershadowmapreceivers();

// rendergl
extern bool hasVBO, hasDRE, hasOQ, hasTR, hasFBO, hasDS, hasTF, hasBE, hasCM, hasNP2, hasTC, hasTE, hasMT, hasD3, hasstencil;

extern bool envmapping;

extern void gl_init(int w, int h, int bpp, int depth, int fsaa);
extern void cleangl();
extern void gl_drawframe(int w, int h);
extern void setfogplane(const plane &p, bool flush = false);
extern void setfogplane(float scale = 0, float z = 0, bool flush = false);
extern void findorientation();
extern void writecrosshairs(FILE *f);

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
extern void newclipplanes(cube &c);
extern void freeclipplanes(cube &c);
extern uchar octantrectangleoverlap(const ivec &c, int size, const ivec &o, const ivec &s);
extern void forcemip(cube &c);
extern bool subdividecube(cube &c, bool fullcheck=true, bool brighten=true);
extern void converttovectorworld();
extern int faceverts(cube &c, int orient, int vert);
extern void calcvert(cube &c, int x, int y, int z, int size, vvec &vert, int i, bool solid = false);
extern void calcverts(cube &c, int x, int y, int z, int size, vvec *verts, bool *usefaces, int *vertused, bool lodcube);
extern uint faceedges(cube &c, int orient);
extern bool collapsedface(uint cfe);
extern bool touchingface(cube &c, int orient);
extern int genclipplane(cube &c, int i, vec *v, plane *clip);
extern void genclipplanes(cube &c, int x, int y, int z, int size, clipplanes &p);
extern bool visibleface(cube &c, int orient, int x, int y, int z, int size, uchar mat = MAT_AIR, uchar nmat = MAT_AIR, bool lodcube = false);
extern int visibleorient(cube &c, int orient);
extern bool threeplaneintersect(plane &pl1, plane &pl2, plane &pl3, vec &dest);
extern void freemergeinfo(cube &c);
extern void genmergedverts(cube &cu, int orient, const ivec &co, int size, const mergeinfo &m, vvec *vv, plane *p = NULL);
extern int calcmergedsize(int orient, const ivec &co, int size, const mergeinfo &m, const vvec *vv);
extern void invalidatemerges(cube &c);
extern void calcmerges();

struct cubeface : mergeinfo
{
	cube *c;
};

extern int mergefaces(int orient, cubeface *m, int sz);
extern void mincubeface(cube &cu, int orient, const ivec &o, int size, const mergeinfo &orig, mergeinfo &cf);

// ents
extern char *entname(entity &e);
extern bool haveselent();
extern void copyundoents(undoblock &d, undoblock &s);
extern void pasteundoents(undoblock &u);

// octaedit
extern void cancelsel();
extern void render_texture_panel(int w, int h);
extern void addundo(undoblock &u);

// octarender
extern void octarender();
extern void allchanged(bool load = false);
extern void vaclearc(cube *c);
extern vtxarray *newva(int x, int y, int z, int size);
extern void destroyva(vtxarray *va, bool reparent = true);

// renderva
extern void visiblecubes(cube *c, int size, int cx, int cy, int cz, int w, int h, int fov);
extern void reflectvfcP(float z);
extern void restorevfcP();
extern void rendermapmodels();
extern void rendergeom();
extern void renderoutline();
extern bool rendersky(bool explicitonly = false, float zreflect = 0);

extern int isvisiblesphere(float rad, const vec &cv);
extern bool bboccluded(const ivec &bo, const ivec &br, cube *c, const ivec &o, int size);
extern occludequery *newquery(void *owner);
extern bool checkquery(occludequery *query, bool nowait = false);
extern void resetqueries();
extern int getnumqueries();
extern void drawbb(const ivec &bo, const ivec &br, const vec &camera = camera1->o);

#define startquery(query) { glBeginQuery_(GL_SAMPLES_PASSED_ARB, ((occludequery *)(query))->id); }
#define endquery(query) \
	{ \
		glEndQuery_(GL_SAMPLES_PASSED_ARB); \
		extern int ati_oq_bug; \
		if(ati_oq_bug) glFlush(); \
	}

// material

extern int showmat;

extern int findmaterial(const char *name);
extern void genmatsurfs(cube &c, int cx, int cy, int cz, int size, vector<materialsurface> &matsurfs);
extern void rendermatsurfs(materialsurface *matbuf, int matsurfs);
extern void rendermatgrid(materialsurface *matbuf, int matsurfs);
extern int optimizematsurfs(materialsurface *matbuf, int matsurfs);
extern void setupmaterials();
extern void rendermaterials(float zclip = 0, bool refract = false);
extern void drawmaterial(int orient, int x, int y, int z, int csize, int rsize, float offset);

// water
extern float reflecting, refracting;
extern int reflectdist, vertwater, waterfog, lavafog;

extern void invalidatereflections();
extern void cleanreflections();
extern void queryreflections();
extern void drawreflections();
extern void renderwater();
extern void renderlava(materialsurface &m, Texture *tex, float scale);
extern void getwatercolour(uchar *wcol);
extern void getlavacolour(uchar *lcol);

// server
extern vector<char *> gameargs;

extern void initserver(bool dedicated);
extern void cleanupserver();
extern void serverslice(uint timeout);

extern uchar *retrieveservers(uchar *buf, int buflen);
extern void localclienttoserver(int chan, ENetPacket *);
extern void localconnect();
extern bool serveroption(char *opt);

// serverbrowser
extern bool resolverwait(const char *name, ENetAddress *address);
extern int connectwithtimeout(ENetSocket sock, char *hostname, ENetAddress &address);
extern void addserver(char *servername);
extern char *getservername(int n);
extern void writeservercfg();

// client
extern void localdisconnect();
extern void localservertoclient(int chan, uchar *buf, int len);
extern void connects(char *servername);
extern void abortconnect();
extern void clientkeepalive();

// command
extern bool overrideidents, persistidents;

extern void explodelist(char *s, vector<char *> &elems);

extern void clearoverrides();
extern void writecfg();

extern void checksleep(int millis);
extern void clearsleep(bool clearoverrides = true);

// console
extern void writebinds(FILE *f);
extern void writecompletions(FILE *f);

// main
extern bool initwarning();

extern void computescreen(const char *text, Texture *t = NULL);
extern void show_out_of_renderloop_progress(float bar1, const char *text1, float bar2 = 0, const char *text2 = NULL, GLuint tex = 0);

// menu
extern void menuprocess();

// physics
extern void mousemove(int dx, int dy);
extern bool pointincube(const clipplanes &p, const vec &v);
extern bool overlapsdynent(const vec &o, float radius);
extern void rotatebb(vec &center, vec &radius, int yaw);
extern float shadowray(const vec &o, const vec &ray, float radius, int mode, extentity *t = NULL);

// world
enum
{
	TRIG_COLLIDE	= 1<<0,
	TRIG_TOGGLE	 = 1<<1,
	TRIG_ONCE		= 0<<2,
	TRIG_MANY		= 1<<2,
	TRIG_DISAPPEAR  = 1<<3,
	TRIG_AUTO_RESET = 1<<4,
	TRIG_RUMBLE	 = 1<<5,
	TRIG_LOCKED	 = 1<<6,
};

#define NUMTRIGGERTYPES 16

extern int triggertypes[NUMTRIGGERTYPES];

#define checktriggertype(type, flag) (triggertypes[(type) & (NUMTRIGGERTYPES-1)] & (flag))

extern void entitiesinoctanodes();
extern void attachentities();
extern void freeoctaentities(cube &c);
extern bool pointinsel(selinfo &sel, vec &o);

#ifdef BFRONTIER
extern void resetmap(bool load = false);
#else
extern void resetmap();
#endif
extern void startmap(const char *name);

// rendermodel
struct mapmodelinfo { string name; int tex; model *m; };

extern void findanims(const char *pattern, vector<int> &anims);
extern void loadskin(const char *dir, const char *altdir, Texture *&skin, Texture *&masks);
extern model *loadmodel(const char *name, int i = -1, bool msg = false);
extern mapmodelinfo &getmminfo(int i);
extern void startmodelquery(occludequery *query);
extern void endmodelquery();

// renderparticles
extern void particleinit();
extern void clearparticles();
extern void entity_particles();

// rendersky
extern void drawskybox(int farplane, bool limited, float zreflect = 0);
extern bool limitsky();

// 3dgui
extern void g3d_render();
extern bool g3d_windowhit(bool on, bool act);
extern char *g3d_fieldname();

extern void g3d_mainmenu();

// sound
extern void checkmapsounds();
extern void clearmapsounds();
extern void updatevol();

// grass
extern void rendergrass();

