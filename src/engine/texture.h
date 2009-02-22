// GL_ARB_vertex_program, GL_ARB_fragment_program
extern PFNGLGENPROGRAMSARBPROC			glGenPrograms_;
extern PFNGLDELETEPROGRAMSARBPROC		 glDeletePrograms_;
extern PFNGLBINDPROGRAMARBPROC			glBindProgram_;
extern PFNGLPROGRAMSTRINGARBPROC		  glProgramString_;
extern PFNGLGETPROGRAMIVARBPROC           glGetProgramiv_;
extern PFNGLPROGRAMENVPARAMETER4FARBPROC  glProgramEnvParameter4f_;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fv_;
extern PFNGLENABLEVERTEXATTRIBARRAYARBPROC  glEnableVertexAttribArray_;
extern PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArray_;
extern PFNGLVERTEXATTRIBPOINTERARBPROC      glVertexAttribPointer_;

// GL_EXT_gpu_program_parameters
#ifndef GL_EXT_gpu_program_parameters
#define GL_EXT_gpu_program_parameters 1
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETERS4FVEXTPROC) (GLenum target, GLuint index, GLsizei count, const GLfloat *params);
typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC) (GLenum target, GLuint index, GLsizei count, const GLfloat *params);
#endif

extern PFNGLPROGRAMENVPARAMETERS4FVEXTPROC   glProgramEnvParameters4fv_;
extern PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC glProgramLocalParameters4fv_;

// GL_ARB_shading_language_100, GL_ARB_shader_objects, GL_ARB_fragment_shader, GL_ARB_vertex_shader
extern PFNGLCREATEPROGRAMOBJECTARBPROC  glCreateProgramObject_;
extern PFNGLDELETEOBJECTARBPROC		 glDeleteObject_;
extern PFNGLUSEPROGRAMOBJECTARBPROC	 glUseProgramObject_;
extern PFNGLCREATESHADEROBJECTARBPROC	glCreateShaderObject_;
extern PFNGLSHADERSOURCEARBPROC		 glShaderSource_;
extern PFNGLCOMPILESHADERARBPROC		glCompileShader_;
extern PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameteriv_;
extern PFNGLATTACHOBJECTARBPROC		 glAttachObject_;
extern PFNGLGETINFOLOGARBPROC			glGetInfoLog_;
extern PFNGLLINKPROGRAMARBPROC		  glLinkProgram_;
extern PFNGLGETUNIFORMLOCATIONARBPROC	glGetUniformLocation_;
extern PFNGLUNIFORM4FVARBPROC			glUniform4fv_;
extern PFNGLUNIFORM1IARBPROC			glUniform1i_;

extern int renderpath;

enum { R_FIXEDFUNCTION = 0, R_ASMSHADER, R_GLSLANG };

enum { SHPARAM_VERTEX = 0, SHPARAM_PIXEL, SHPARAM_UNIFORM };

#define RESERVEDSHADERPARAMS 16
#define MAXSHADERPARAMS 8

struct ShaderParam
{
    const char *name;
	int type, index, loc;
	float val[4];
};

struct LocalShaderParamState : ShaderParam
{
	float curval[4];

	LocalShaderParamState()
	{
		memset(curval, 0, sizeof(curval));
	}
	LocalShaderParamState(const ShaderParam &p) : ShaderParam(p)
	{
		memset(curval, 0, sizeof(curval));
	}
};

struct ShaderParamState
{
    enum
    {
        CLEAN = 0,
        INVALID,
        DIRTY
    };

    const char *name;
    float val[4];
    bool local;
    int dirty;

    ShaderParamState()
        : name(NULL), local(false), dirty(INVALID)
    {
        memset(val, 0, sizeof(val));
    }
};

enum
{
	SHADER_DEFAULT    = 0,
	SHADER_NORMALSLMS = 1<<0,
	SHADER_ENVMAP     = 1<<1,
    SHADER_GLSLANG    = 1<<2,
    SHADER_OPTION     = 1<<3,
    SHADER_INVALID    = 1<<4,
    SHADER_DEFERRED   = 1<<5
};

#define MAXSHADERDETAIL 3
#define MAXVARIANTROWS 5

extern int shaderdetail;

struct Slot;

struct Shader
{
	static Shader *lastshader;

    char *name, *vsstr, *psstr, *defer;
    int type;
    GLuint vs, ps;
    GLhandleARB program, vsobj, psobj;
    vector<LocalShaderParamState> defaultparams;
    Shader *detailshader, *variantshader, *altshader, *fastshader[MAXSHADERDETAIL];
    vector<Shader *> variants[MAXVARIANTROWS];
    bool standard, forced, used, native;
    Shader *reusevs, *reuseps;
    int numextparams;
    LocalShaderParamState *extparams;
    uchar *extvertparams, *extpixparams;

    Shader() : name(NULL), vsstr(NULL), psstr(NULL), defer(NULL), type(SHADER_DEFAULT), vs(0), ps(0), program(0), vsobj(0), psobj(0), detailshader(NULL), variantshader(NULL), altshader(NULL), standard(false), forced(false), used(false), native(true), reusevs(NULL), reuseps(NULL), numextparams(0), extparams(NULL), extvertparams(NULL), extpixparams(NULL)
    {
        loopi(MAXSHADERDETAIL) fastshader[i] = this;
    }

    ~Shader()
    {
        DELETEA(name);
        DELETEA(vsstr);
        DELETEA(psstr);
        DELETEA(defer);
        DELETEA(extparams);
        DELETEA(extvertparams);
        extpixparams = NULL;
    }

    void fixdetailshader(bool force = true, bool recurse = true);
    void allocenvparams(Slot *slot = NULL);
    void flushenvparams(Slot *slot = NULL);
    void setslotparams(Slot &slot);
    void bindprograms();

    bool hasoption(int row)
    {
        if(!detailshader || detailshader->variants[row].empty()) return false;
        return (detailshader->variants[row][0]->type&SHADER_OPTION)!=0;
    }

    void setvariant(int col, int row, Slot *slot, Shader *fallbackshader)
    {
        if(!this || !detailshader || renderpath==R_FIXEDFUNCTION) return;
        int len = detailshader->variants[row].length();
        if(col >= len) col = len-1;
        Shader *s = fallbackshader;
        while(col >= 0) if(!(detailshader->variants[row][col]->type&SHADER_INVALID)) { s = detailshader->variants[row][col]; break; }
        if(lastshader!=s) s->bindprograms();
        lastshader->flushenvparams(slot);
        if(slot) lastshader->setslotparams(*slot);
    }

    void setvariant(int col, int row = 0, Slot *slot = NULL)
    {
        setvariant(col, row, slot, detailshader);
    }

    void set(Slot *slot = NULL)
    {
        if(!this || !detailshader || renderpath==R_FIXEDFUNCTION) return;
        if(lastshader!=detailshader) detailshader->bindprograms();
        lastshader->flushenvparams(slot);
        if(slot) lastshader->setslotparams(*slot);
    }

    bool compile();
    void cleanup(bool invalid = false);
};

#define SETSHADER(name) \
    do { \
        static Shader *name##shader = NULL; \
        if(!name##shader) name##shader = lookupshaderbyname(#name); \
        name##shader->set(); \
    } while(0)

// management of texture slots
// each texture slot can have multiple texture frames, of which currently only the first is used
// additional frames can be used for various shaders

struct TextureAnim
{
	int count, delay, x, y, w, h;

	TextureAnim() : count(0), delay(0) {}
};


struct Texture
{
    enum
    {
        STUB,
        TRANSIENT,
        IMAGE,
        CUBEMAP
    };

    char *name;
    int type, w, h, xs, ys, bpp, clamp, frame, delay, last;
    bool mipmap, canreduce;
    vector<GLuint> frames;
    GLuint id;
    uchar *alphamask;


    Texture() : frame(0), delay(0), last(0), alphamask(NULL)
    {
    	frames.setsize(0);
	}

	GLuint idframe(int id)
	{
		if(!frames.empty())
			return frames[clamp(id, 0, frames.length()-1)];
		return id;
	}

	GLuint getframe(float amt)
	{
		if(!frames.empty())
			return frames[clamp(int((frames.length()-1)*amt), 0, frames.length()-1)];
		return id;
	}

	GLuint retframe(int cur, int total)
	{
		if(!frames.empty())
			return frames[clamp((frames.length()-1)*cur/total, 0, frames.length()-1)];
		return id;
	}
};

enum
{
	TEX_DIFFUSE = 0,
	TEX_UNKNOWN,
	TEX_DECAL,
	TEX_NORMAL,
	TEX_GLOW,
	TEX_SPEC,
	TEX_DEPTH,
	TEX_ENVMAP,
	TEX_MAX
};

struct Slot
{
	struct Tex
	{
		int type;
		Texture *t;
		string lname, name;
		int combined;
	};

	vector<Tex> sts;
	Shader *shader;
	vector<ShaderParam> params;
    float scale;
    int rotation, xoffset, yoffset;
    float scrollS, scrollT;
    int layer;
    vec glowcolor, pulseglowcolor;
    float pulseglowspeed;
    bool mtglowed, loaded;
    uint texmask;
    char *autograss;
    Texture *grasstex, *thumbnail;

    Slot() : autograss(NULL) { reset(); }

	void reset()
	{
		sts.setsize(0);
		shader = NULL;
		params.setsize(0);
        scale = 1;
        rotation = xoffset = yoffset = 0;
        scrollS = scrollT = 0;
        layer = 0;
        glowcolor = vec(1, 1, 1);
        pulseglowcolor = vec(0, 0, 0);
        pulseglowspeed = 0;
		loaded = false;
        texmask = 0;
        DELETEA(autograss);
        grasstex = NULL;
		thumbnail = NULL;
	}

    void cleanup()
    {
        loaded = false;
        grasstex = NULL;
        thumbnail = NULL;
        loopv(sts)
        {
            Tex &t = sts[i];
            t.t = NULL;
            t.combined = -1;
        }
    }
};

extern vector<Slot> slots;
extern Slot materialslots[MATF_VOLUME+1];

extern SDL_Surface *texreorient(SDL_Surface *s, bool flipx, bool flipy, bool swapxy, int type = TEX_DIFFUSE, bool clear = true);
extern SDL_Surface *texrotate(SDL_Surface *s, int numrots, int type = TEX_DIFFUSE);
extern SDL_Surface *texoffset(SDL_Surface *s, int xoffset, int yoffset, bool clear = true);
extern SDL_Surface *texcrop(SDL_Surface *s, int x, int y, int w, int h, bool clear = true);
extern SDL_Surface *texcopy(SDL_Surface *s, bool clear = true);
extern SDL_Surface *texffmask(SDL_Surface *s, int minval, bool clear = true);
extern SDL_Surface *texdecal(SDL_Surface *s, bool clear = true);
extern SDL_Surface *createsurface(int width, int height, int bpp);
extern SDL_Surface *wrapsurface(void *data, int width, int height, int bpp);
extern SDL_Surface *flipsurface(SDL_Surface *os, bool clear = true);
extern SDL_Surface *creatergbsurface(SDL_Surface *os, bool clear = true);
extern SDL_Surface *creatergbasurface(SDL_Surface *os, bool clear = true);
extern SDL_Surface *fixsurfaceformat(SDL_Surface *s);
extern SDL_Surface *scalesurface(SDL_Surface *os, int w, int h, bool clear = true);
extern SDL_Surface *texturedata(const char *tname, Slot::Tex *tex = NULL, bool msg = true, bool *compress = NULL, TextureAnim *anim = NULL);

enum
{
    IFMT_NONE = 0,
    IFMT_BMP,
    IFMT_PNG,
    IFMT_TGA,
    IFMT_MAX,
};
extern const char *ifmtexts[IFMT_MAX];
extern int imageformat;

extern void savepng(const char *filename, SDL_Surface *image, int compress = 9, bool flip = false);
extern void savetga(const char *filename, SDL_Surface *image, int compress = 1, bool flip = false);
extern SDL_Surface *loadsurface(const char *name);
extern void savesurface(SDL_Surface *s, const char *name, int format = IFMT_NONE, int compress = 9, bool flip = false, bool skip = false);

extern Texture *newtexture(Texture *t, const char *rname, SDL_Surface *s, int clamp = 0, bool mipit = true, bool canreduce = false, bool transient = false, bool compress = false, bool clear = true, TextureAnim *anim = NULL);
extern Texture *cubemaploadwildcard(Texture *t, const char *name, bool mipit, bool msg);

extern void resetmaterials();
extern void resettextures();
extern void setshader(char *name);
extern void setshaderparam(const char *name, int type, int n, float x, float y, float z, float w);
extern int findtexturetype(char *name, bool tryint = false);
extern const char *findtexturename(int type);
extern void texture(char *type, char *name, int *rot, int *xoffet, int *yoffset, float *scale);
extern void updatetextures();
extern void preloadtextures();

struct cubemapside
{
	GLenum target;
	const char *name;
    bool flipx, flipy, swapxy;
};

extern cubemapside cubemapsides[6];
extern Texture *notexture, *blanktexture;
extern Shader *defaultshader, *rectshader, *notextureshader, *nocolorshader, *foggedshader, *foggednotextureshader, *stdworldshader;
extern int reservevpparams, maxvpenvparams, maxvplocalparams, maxfpenvparams, maxfplocalparams;

extern Shader *lookupshaderbyname(const char *name);
extern Shader *useshaderbyname(const char *name);
extern Texture *loadthumbnail(Slot &slot);
extern void resetslotshader();
extern void setslotshader(Slot &s);
extern void linkslotshader(Slot &s, bool load = true);
extern void linkslotshaders();
extern void setenvparamf(const char *name, int type, int index, float x = 0, float y = 0, float z = 0, float w = 0);
extern void setenvparamfv(const char *name, int type, int index, const float *v);
extern void flushenvparamf(const char *name, int type, int index, float x = 0, float y = 0, float z = 0, float w = 0);
extern void flushenvparamfv(const char *name, int type, int index, const float *v);
extern void setlocalparamf(const char *name, int type, int index, float x = 0, float y = 0, float z = 0, float w = 0);
extern void setlocalparamfv(const char *name, int type, int index, const float *v);
extern void invalidateenvparams(int type, int start, int count);
extern ShaderParam *findshaderparam(Slot &s, const char *name, int type, int index);

extern int maxtmus, nolights, nowater, nomasks;

extern void inittmus();
extern void resettmu(int n);
extern void scaletmu(int n, int rgbscale, int alphascale = 0);
extern void colortmu(int n, float r = 0, float g = 0, float b = 0, float a = 0);
extern void setuptmu(int n, const char *rgbfunc = NULL, const char *alphafunc = NULL);

#define MAXDYNLIGHTS 5
#define DYNLIGHTBITS 6
#define DYNLIGHTMASK ((1<<DYNLIGHTBITS)-1)

#define MAXBLURRADIUS 7

extern void setupblurkernel(int radius, float sigma, float *weights, float *offsets);
extern void setblurshader(int pass, int size, int radius, float *weights, float *offsets, GLenum target = GL_TEXTURE_2D);

#define _TVAR(n, c, m, p) _SVARF(n, n, c, { if(n[0]) textureload(n, m, true); }, p)
#define TVAR(n, c, m)  _TVAR(n, c, m, IDF_PERSIST|IDF_COMPLETE|IDF_TEXTURE)
#define TVARW(n, c, m) _TVAR(n, c, m, IDF_WORLD|IDF_COMPLETE|IDF_TEXTURE)
#define _TVARN(n, c, t, m, p) _SVARF(n, n, c, { t = n[0] ? textureload(n, m, true) : notexture; }, p)
#define TVARN(n, c, t, m) _TVARN(n, c, t, m, IDF_PERSIST|IDF_COMPLETE|IDF_TEXTURE)
#define TVARC(n, c, t, m) Texture *##t; _TVARN(n, c, t, m, IDF_PERSIST|IDF_COMPLETE|IDF_TEXTURE)

#define _ITVAR(n, c, m, p) _ISVAR(n, c, void changed() { if(*val.s) textureload(val.s, m, true); }, p)
#define ITVAR(n, c, m)  _ITVAR(n, c, m, IDF_PERSIST|IDF_COMPLETE|IDF_TEXTURE)
#define ITVARW(n, c, m) _ITVAR(n, c, m, IDF_WORLD|IDF_COMPLETE|IDF_TEXTURE)
