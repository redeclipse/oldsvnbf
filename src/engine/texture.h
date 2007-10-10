// GL_ARB_vertex_program, GL_ARB_fragment_program
extern PFNGLGENPROGRAMSARBPROC			glGenPrograms_;
extern PFNGLDELETEPROGRAMSARBPROC		 glDeletePrograms_;
extern PFNGLBINDPROGRAMARBPROC			glBindProgram_;
extern PFNGLPROGRAMSTRINGARBPROC		  glProgramString_;
extern PFNGLGETPROGRAMIVARBPROC           glGetProgramiv_;
extern PFNGLPROGRAMENVPARAMETER4FARBPROC  glProgramEnvParameter4f_;
extern PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fv_;

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
	char *name;
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
	char *name;
	float val[4];
	bool dirty, local;

	ShaderParamState()
		: name(NULL), dirty(false), local(false)
	{
		memset(val, 0, sizeof(val));
	}
};

enum 
{ 
	SHADER_DEFAULT	= 0, 
	SHADER_NORMALSLMS = 1<<0, 
	SHADER_ENVMAP	 = 1<<1,
	SHADER_GLSLANG	= 1<<2
};

#define MAXSHADERDETAIL 3
#define MAXVARIANTROWS 2

extern int shaderdetail;

struct Slot;

struct Shader
{
	static Shader *lastshader;

	char *name;
	int type;
	GLuint vs, ps;
	GLhandleARB program, vsobj, psobj;
	vector<LocalShaderParamState> defaultparams, extparams;
	Shader *altshader, *fastshader[MAXSHADERDETAIL];
    vector<Shader *> variants[MAXVARIANTROWS];
	LocalShaderParamState *extvertparams[RESERVEDSHADERPARAMS], *extpixparams[RESERVEDSHADERPARAMS];
    bool used, native;

    Shader() : name(NULL), type(SHADER_DEFAULT), vs(0), ps(0), program(0), vsobj(0), psobj(0), altshader(NULL), used(false), native(true)
	{}

	~Shader()
	{
		DELETEA(name);
	}

	void allocenvparams(Slot *slot = NULL);
	void flushenvparams(Slot *slot = NULL);
	void setslotparams(Slot &slot);
	void bindprograms();

    Shader *variant(int col, int row = 0)
	{
		if(!this || renderpath==R_FIXEDFUNCTION) return this;
		Shader *s = shaderdetail < MAXSHADERDETAIL ? fastshader[shaderdetail] : this;
        return row>=0 && row<MAXVARIANTROWS && s->variants[row].inrange(col) ? s->variants[row][col] : s;
	}

	void set(Slot *slot = NULL)
	{
		if(!this || renderpath==R_FIXEDFUNCTION) return;
		if(this!=lastshader)
		{
			if(shaderdetail < MAXSHADERDETAIL) fastshader[shaderdetail]->bindprograms();
			else bindprograms();
		}
		lastshader->flushenvparams(slot);
		if(slot) lastshader->setslotparams(*slot);
	}
};

// management of texture slots
// each texture slot can have multiple texture frames, of which currently only the first is used
// additional frames can be used for various shaders

struct Texture
{
	char *name;
	int xs, ys, bpp;
	GLuint gl;
	uchar *alphamask;

	Texture() : alphamask(NULL) {}
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
	TEX_ENVMAP
};
	
struct Slot
{
	struct Tex
	{
		int type;
		Texture *t;
#ifdef BFRONTIER
		string lname, name;
#else
		string name;
#endif
		int rotation, xoffset, yoffset;
		float scale;
		int combined;
	};

	vector<Tex> sts;
	Shader *shader;
	vector<ShaderParam> params;
	bool loaded;
	char *autograss;
	Texture *grasstex, *thumbnail;
	
	void reset()
	{
		sts.setsize(0);
		shader = NULL;
		params.setsize(0);
		loaded = false;
		DELETEA(autograss);
		grasstex = NULL;
		thumbnail = NULL;
	}
	
	Slot() : autograss(NULL) { reset(); }
};
#ifdef BFRONTIER
extern vector<Slot> slots;
extern Slot materialslots[MAT_EDIT];

extern void materialreset();
extern void texturereset();
extern void autograss(char *name);
extern void setshader(char *name);
extern void setshaderparam(char *name, int type, int n, float x, float y, float z, float w);
extern int findtexturetype(char *name);
extern char *findtexturename(int type);
extern void texture(char *type, char *name, int *rot, int *xoffet, int *yoffset, float *scale);
#endif

struct cubemapside
{
	GLenum target;
	const char *name;
};

extern cubemapside cubemapsides[6];

extern Texture *notexture;

extern Shader *defaultshader, *notextureshader, *nocolorshader, *foggedshader, *foggednotextureshader;

extern Shader *lookupshaderbyname(const char *name);

extern Texture *loadthumbnail(Slot &slot);

extern void setslotshader(Slot &s);

extern void setenvparamf(char *name, int type, int index, float x = 0, float y = 0, float z = 0, float w = 0);
extern void setenvparamfv(char *name, int type, int index, const float *v);
extern void flushenvparam(int type, int index, bool local = false);
extern void setlocalparamf(char *name, int type, int index, float x = 0, float y = 0, float z = 0, float w = 0);
extern void setlocalparamfv(char *name, int type, int index, const float *v);

extern ShaderParam *findshaderparam(Slot &s, char *name, int type, int index);

extern int maxtmus, nolights, nowater, nomasks;
extern void inittmus();
extern void resettmu(int n);
extern void scaletmu(int n, int rgbscale, int alphascale = 0);
extern void colortmu(int n, float r = 0, float g = 0, float b = 0, float a = 0);
extern void setuptmu(int n, const char *rgbfunc = NULL, const char *alphafunc = NULL);

#define MAXDYNLIGHTS 5

