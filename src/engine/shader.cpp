// shader.cpp: OpenGL assembly/GLSL shader management

#include "pch.h"
#include "engine.h"

Shader *Shader::lastshader = NULL;

Shader *defaultshader = NULL, *notextureshader = NULL, *nocolorshader = NULL, *foggedshader = NULL, *foggednotextureshader = NULL;

static hashtable<const char *, Shader> shaders;
static Shader *curshader = NULL;
static vector<ShaderParam> curparams;
static ShaderParamState vertexparamstate[RESERVEDSHADERPARAMS + MAXSHADERPARAMS], pixelparamstate[RESERVEDSHADERPARAMS + MAXSHADERPARAMS];
static bool dirtyenvparams = false;

VAR(reservevpparams, 1, 16, 0);
VAR(maxvpenvparams, 1, 0, 0);
VAR(maxvplocalparams, 1, 0, 0);
VAR(maxfpenvparams, 1, 0, 0);
VAR(maxfplocalparams, 1, 0, 0);

void loadshaders()
{
    if(renderpath!=R_FIXEDFUNCTION)
    {
        GLint val;
        glGetProgramiv_(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &val);
        maxvpenvparams = val;
        glGetProgramiv_(GL_VERTEX_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &val);
        maxvplocalparams = val;
        glGetProgramiv_(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_ENV_PARAMETERS_ARB, &val);
        maxfpenvparams = val;
        glGetProgramiv_(GL_FRAGMENT_PROGRAM_ARB, GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB, &val);
        maxfplocalparams = val;
    }

	persistidents = false;
    exec("stdshader.cfg");
    persistidents = true;
    defaultshader = lookupshaderbyname("default");
    notextureshader = lookupshaderbyname("notexture");
    nocolorshader = lookupshaderbyname("nocolor");
    foggedshader = lookupshaderbyname("fogged");
    foggednotextureshader = lookupshaderbyname("foggednotexture");
    if(renderpath!=R_FIXEDFUNCTION)
    {
        glEnable(GL_VERTEX_PROGRAM_ARB);
        glEnable(GL_FRAGMENT_PROGRAM_ARB);
    }
    defaultshader->set();
}

Shader *lookupshaderbyname(const char *name)
{
	Shader *s = shaders.access(name);
	return s && s->altshader ? s->altshader : s;
}

static bool compileasmshader(GLenum type, GLuint &idx, const char *def, const char *tname, const char *name, bool msg = true, bool nativeonly = false)
{
	glGenPrograms_(1, &idx);
	glBindProgram_(type, idx);
	def += strspn(def, " \t\r\n");
	glProgramString_(type, GL_PROGRAM_FORMAT_ASCII_ARB, (GLsizei)strlen(def), def);
    GLint err, native;
	glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB, &err);
    glGetProgramiv_(type, GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB, &native);
	if(msg && err!=-1)
	{
		conoutf("COMPILE ERROR (%s:%s) - %s", tname, name, glGetString(GL_PROGRAM_ERROR_STRING_ARB));
        if(err>=0 && err<(int)strlen(def))
        {
			loopi(err) putchar(*def++);
			puts(" <<HERE>> ");
			while(*def) putchar(*def++);
		}
    }
    else if(msg && !native) conoutf("%s:%s EXCEEDED NATIVE LIMITS", tname, name);
    if(err!=-1 || (!native && nativeonly))
	{
		glDeletePrograms_(1, &idx);
		idx = 0;
	}
    return native!=0;
}

static void showglslinfo(GLhandleARB obj, const char *tname, const char *name)
{
	GLint length = 0;
	glGetObjectParameteriv_(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB, &length);
	if(length > 1)
	{
		GLcharARB *log = new GLcharARB[length];
		glGetInfoLog_(obj, length, &length, log);
		conoutf("GLSL ERROR (%s:%s)", tname, name);
		puts(log);
		delete[] log;
	}
}

static void compileglslshader(GLenum type, GLhandleARB &obj, const char *def, const char *tname, const char *name, bool msg = true) 
{
	const GLcharARB *source = (const GLcharARB*)(def + strspn(def, " \t\r\n"));
	obj = glCreateShaderObject_(type);
	glShaderSource_(obj, 1, &source, NULL);
	glCompileShader_(obj);
	GLint success;
	glGetObjectParameteriv_(obj, GL_OBJECT_COMPILE_STATUS_ARB, &success);
	if(!success)
	{
        if(msg) showglslinfo(obj, tname, name);
		glDeleteObject_(obj);
		obj = 0;
	}
}

static void linkglslprogram(Shader &s, bool msg = true)
{
	s.program = glCreateProgramObject_();
	GLint success = 0;
	if(s.program && s.vsobj && s.psobj)
	{
		glAttachObject_(s.program, s.vsobj);
		glAttachObject_(s.program, s.psobj);
		glLinkProgram_(s.program);
		glGetObjectParameteriv_(s.program, GL_OBJECT_LINK_STATUS_ARB, &success);
	}
	if(success)
	{
		glUseProgramObject_(s.program);
		loopi(8)
		{
			s_sprintfd(arg)("tex%d", i);
			GLint loc = glGetUniformLocation_(s.program, arg);
			if(loc != -1) glUniform1i_(loc, i);
		}
		loopv(s.defaultparams)
		{
			ShaderParam &param = s.defaultparams[i];
			string pname;
			if(param.type==SHPARAM_UNIFORM) s_strcpy(pname, param.name);
			else s_sprintf(pname)("%s%d", param.type==SHPARAM_VERTEX ? "v" : "p", param.index);
			param.loc = glGetUniformLocation_(s.program, pname);
		}
		glUseProgramObject_(0);
	}
    else if(s.program)
    {
        if(msg) showglslinfo(s.program, "PROG", s.name);
        glDeleteObject_(s.program);
        s.program = 0;
    }
}

bool checkglslsupport()
{
	/* check if GLSL profile supports loops
	 * might need to rewrite this if compiler does strength reduction
	 */
	const GLcharARB *source =
		"uniform int N;\n"
		"uniform vec4 delta;\n"
		"void main(void) {\n"
		"	vec4 test = vec4(0.0, 0.0, 0.0, 0.0);\n"
		"	for(int i = 0; i < N; i++)  test += delta;\n"
		"	gl_FragColor = test;\n"
		"}\n";
	GLhandleARB obj = glCreateShaderObject_(GL_FRAGMENT_SHADER_ARB);
	if(!obj) return false;
	glShaderSource_(obj, 1, &source, NULL);
	glCompileShader_(obj);
	GLint success;
	glGetObjectParameteriv_(obj, GL_OBJECT_COMPILE_STATUS_ARB, &success);
	if(!success)
	{
		glDeleteObject_(obj);
		return false;
	}
	GLhandleARB program = glCreateProgramObject_();
	if(!program)
	{
		glDeleteObject_(obj);
		return false;
	}
	glAttachObject_(program, obj);
	glLinkProgram_(program);
	glGetObjectParameteriv_(program, GL_OBJECT_LINK_STATUS_ARB, &success);
	glDeleteObject_(obj);
	glDeleteObject_(program);
	return success!=0;
}

static LocalShaderParamState unusedextparam;

static void allocglsluniformparam(Shader &s, int type, int index, bool local = false)
{
	ShaderParamState &val = (type==SHPARAM_VERTEX ? vertexparamstate[index] : pixelparamstate[index]);
	int loc = val.name ? glGetUniformLocation_(s.program, val.name) : -1;
	if(loc == -1)
	{
		s_sprintfd(altname)("%s%d", type==SHPARAM_VERTEX ? "v" : "p", index);
		loc = glGetUniformLocation_(s.program, val.name);
	}
	else
	{
		LocalShaderParamState *alt = (type==SHPARAM_VERTEX ? s.extpixparams[index] : s.extvertparams[index]);
		if(alt && alt != &unusedextparam && alt->loc == loc)
		{
			if(type==SHPARAM_VERTEX) s.extvertparams[index] = alt;
			else s.extpixparams[index] = alt;
			return;
		}
	}
	if(loc == -1)
	{
		if(type==SHPARAM_VERTEX) s.extvertparams[index] = local ? &unusedextparam : NULL;
		else s.extpixparams[index] = local ? &unusedextparam : NULL;
		return;
	}
	LocalShaderParamState &ext = s.extparams.add();
	ext.name = val.name;
	ext.type = type;
	ext.index = local ? -1 : index;
	ext.loc = loc;
	if(type==SHPARAM_VERTEX) s.extvertparams[index] = &ext;
	else s.extpixparams[index] = &ext;
}

void Shader::allocenvparams(Slot *slot)
{
	if(!(type & SHADER_GLSLANG)) return;

	if(slot)
	{
#define UNIFORMTEX(name, tmu) \
		{ \
			loc = glGetUniformLocation_(program, name); \
			int val = tmu; \
			if(loc != -1) glUniform1i_(loc, val); \
		}
		int loc, tmu = 2;
		if(type & SHADER_NORMALSLMS)
		{
			UNIFORMTEX("lmcolor", 1);
			UNIFORMTEX("lmdir", 2);
			tmu++;
		}
		else UNIFORMTEX("lightmap", 1);
		if(type & SHADER_ENVMAP) UNIFORMTEX("envmap", tmu++);
        UNIFORMTEX("shadowmap", 7);
		int stex = 0;
		loopv(slot->sts)
		{
			Slot::Tex &t = slot->sts[i];
			switch(t.type)
			{
				case TEX_DIFFUSE: UNIFORMTEX("diffusemap", 0); break;
				case TEX_NORMAL: UNIFORMTEX("normalmap", tmu++); break;
				case TEX_GLOW: UNIFORMTEX("glowmap", tmu++); break;
				case TEX_DECAL: UNIFORMTEX("decal", tmu++); break;
				case TEX_SPEC: if(t.combined<0) UNIFORMTEX("specmap", tmu++); break;
				case TEX_DEPTH: if(t.combined<0) UNIFORMTEX("depthmap", tmu++); break;
				case TEX_UNKNOWN:
				{
					s_sprintfd(sname)("stex%d", stex++);
					UNIFORMTEX(sname, tmu++);
					break;
				}
			}
		}
	}
	loopi(RESERVEDSHADERPARAMS) if(vertexparamstate[i].name && !vertexparamstate[i].local)
		allocglsluniformparam(*this, SHPARAM_VERTEX, i);
	loopi(RESERVEDSHADERPARAMS) if(pixelparamstate[i].name && !pixelparamstate[i].local)
		allocglsluniformparam(*this, SHPARAM_PIXEL, i);
}

static inline void flushparam(int type, int index)
{
    ShaderParamState &val = (type==SHPARAM_VERTEX ? vertexparamstate[index] : pixelparamstate[index]);
    if(Shader::lastshader && Shader::lastshader->type&SHADER_GLSLANG)
    {
        LocalShaderParamState *&ext = (type==SHPARAM_VERTEX ? Shader::lastshader->extvertparams[index] : Shader::lastshader->extpixparams[index]);
        if(!ext) allocglsluniformparam(*Shader::lastshader, type, index, val.local);
        if(!ext || ext == &unusedextparam) return;
        if(!memcmp(ext->curval, val.val, sizeof(ext->curval))) return;
        memcpy(ext->curval, val.val, sizeof(ext->curval));
        glUniform4fv_(ext->loc, 1, ext->curval);
    }
    else if(val.dirty==ShaderParamState::DIRTY)
    {
        glProgramEnvParameter4fv_(type==SHPARAM_VERTEX ? GL_VERTEX_PROGRAM_ARB : GL_FRAGMENT_PROGRAM_ARB, index, val.val);
        val.dirty = ShaderParamState::CLEAN;
    }
}

static inline ShaderParamState &setparamf(const char *name, int type, int index, float x, float y, float z, float w)
{
    ShaderParamState &val = (type==SHPARAM_VERTEX ? vertexparamstate[index] : pixelparamstate[index]);
    val.name = name;
    if(val.dirty==ShaderParamState::INVALID || val.val[0]!=x || val.val[1]!=y || val.val[2]!=z || val.val[3]!=w)
    {
        val.val[0] = x;
        val.val[1] = y;
        val.val[2] = z;
        val.val[3] = w;
        val.dirty = ShaderParamState::DIRTY;
    }
    return val;
}

static inline ShaderParamState &setparamfv(const char *name, int type, int index, const float *v)
{
    ShaderParamState &val = (type==SHPARAM_VERTEX ? vertexparamstate[index] : pixelparamstate[index]);
    val.name = name;
    if(val.dirty==ShaderParamState::INVALID || memcmp(val.val, v, sizeof(val.val)))
    {
        memcpy(val.val, v, sizeof(val.val));
        val.dirty = ShaderParamState::DIRTY;
    }
    return val;
}

void setenvparamf(const char *name, int type, int index, float x, float y, float z, float w)
{
    ShaderParamState &val = setparamf(name, type, index, x, y, z, w);
    val.local = false;
    if(val.dirty==ShaderParamState::DIRTY) dirtyenvparams = true;
}

void setenvparamfv(const char *name, int type, int index, const float *v)
{
    ShaderParamState &val = setparamfv(name, type, index, v);
    val.local = false;
    if(val.dirty==ShaderParamState::DIRTY) dirtyenvparams = true;
}

void flushenvparamf(const char *name, int type, int index, float x, float y, float z, float w)
{
    ShaderParamState &val = setparamf(name, type, index, x, y, z, w);
    val.local = false;
    flushparam(type, index);
}

void flushenvparamfv(const char *name, int type, int index, const float *v)
{
    ShaderParamState &val = setparamfv(name, type, index, v);
    val.local = false;
    flushparam(type, index);
}

void setlocalparamf(const char *name, int type, int index, float x, float y, float z, float w)
{
    ShaderParamState &val = setparamf(name, type, index, x, y, z, w);
    val.local = true;
    flushparam(type, index);
}

void setlocalparamfv(const char *name, int type, int index, const float *v)
{
    ShaderParamState &val = setparamfv(name, type, index, v);
    val.local = true;
    flushparam(type, index);
}

void invalidateenvparams(int type, int start, int count)
{
    ShaderParamState *paramstate = type==SHPARAM_VERTEX ? vertexparamstate : pixelparamstate;
    int end = min(start + count, RESERVEDSHADERPARAMS + MAXSHADERPARAMS);
    while(start < end)
    {
        paramstate[start].dirty = ShaderParamState::INVALID;
        start++;
    }
}

void Shader::flushenvparams(Slot *slot)
{
	if(type & SHADER_GLSLANG)
	{
		if(!used) allocenvparams(slot);

		loopv(extparams)
		{
			LocalShaderParamState &ext = extparams[i];
			if(ext.index<0) continue;
			float *val = ext.type==SHPARAM_VERTEX ? vertexparamstate[ext.index].val : pixelparamstate[ext.index].val;
			if(!memcmp(ext.curval, val, sizeof(ext.val))) continue;
			memcpy(ext.curval, val, sizeof(ext.val));
			glUniform4fv_(ext.loc, 1, ext.curval);
		}
	}
    else if(dirtyenvparams)
    {
        loopi(RESERVEDSHADERPARAMS)
        {
            ShaderParamState &val = vertexparamstate[i];
            if(val.local || val.dirty!=ShaderParamState::DIRTY) continue;
            glProgramEnvParameter4fv_(GL_VERTEX_PROGRAM_ARB, i, val.val);
            val.dirty = ShaderParamState::CLEAN;
        }
        loopi(RESERVEDSHADERPARAMS)
        {
            ShaderParamState &val = pixelparamstate[i];
            if(val.local || val.dirty!=ShaderParamState::DIRTY) continue;
            glProgramEnvParameter4fv_(GL_FRAGMENT_PROGRAM_ARB, i, val.val);
            val.dirty = ShaderParamState::CLEAN;
        }
        dirtyenvparams = false;
    }
	used = true;
}

void Shader::setslotparams(Slot &slot)
{
	uint unimask = 0, vertmask = 0, pixmask = 0;
	loopv(slot.params)
	{
		ShaderParam &p = slot.params[i];
		if(type & SHADER_GLSLANG)
		{
			LocalShaderParamState &l = defaultparams[p.index];
			unimask |= p.index;
			if(!memcmp(l.curval, p.val, sizeof(l.curval))) continue;
			memcpy(l.curval, p.val, sizeof(l.curval));
			glUniform4fv_(l.loc, 1, l.curval);
		}
		else if(p.type!=SHPARAM_UNIFORM)
		{
			ShaderParamState &val = (p.type==SHPARAM_VERTEX ? vertexparamstate[RESERVEDSHADERPARAMS+p.index] : pixelparamstate[RESERVEDSHADERPARAMS+p.index]);
			if(p.type==SHPARAM_VERTEX) vertmask |= 1<<p.index;
			else pixmask |= 1<<p.index;
			if(memcmp(val.val, p.val, sizeof(val.val))) memcpy(val.val, p.val, sizeof(val.val));
            else if(val.dirty!=ShaderParamState::DIRTY) continue;
            glProgramEnvParameter4fv_(p.type==SHPARAM_VERTEX ? GL_VERTEX_PROGRAM_ARB : GL_FRAGMENT_PROGRAM_ARB, RESERVEDSHADERPARAMS+p.index, val.val);
            val.local = true;
            val.dirty = ShaderParamState::CLEAN;
        }
    }
    loopv(defaultparams)
    {
        LocalShaderParamState &l = defaultparams[i];
        if(type & SHADER_GLSLANG)
        {
            if(unimask&(1<<i)) continue;
            if(!memcmp(l.curval, l.val, sizeof(l.curval))) continue;
            memcpy(l.curval, l.val, sizeof(l.curval));
            glUniform4fv_(l.loc, 1, l.curval);
        }
        else if(l.type!=SHPARAM_UNIFORM)
        {
            if(l.type==SHPARAM_VERTEX)
            {
                if(vertmask & (1<<l.index)) continue;
            }
            else if(pixmask & (1<<l.index)) continue;
            ShaderParamState &val = (l.type==SHPARAM_VERTEX ? vertexparamstate[RESERVEDSHADERPARAMS+l.index] : pixelparamstate[RESERVEDSHADERPARAMS+l.index]);
            if(memcmp(val.val, l.val, sizeof(val.val))) memcpy(val.val, l.val, sizeof(val.val));
            else if(val.dirty!=ShaderParamState::DIRTY) continue;
            glProgramEnvParameter4fv_(l.type==SHPARAM_VERTEX ? GL_VERTEX_PROGRAM_ARB : GL_FRAGMENT_PROGRAM_ARB, RESERVEDSHADERPARAMS+l.index, val.val);
            val.local = true;
            val.dirty = ShaderParamState::CLEAN;
		}
	}
}

void Shader::bindprograms()
{
	if(this==lastshader) return;
	if(type & SHADER_GLSLANG)
	{
		glUseProgramObject_(program);
	}
	else
	{
		if(lastshader && lastshader->type & SHADER_GLSLANG) glUseProgramObject_(0);

		glBindProgram_(GL_VERTEX_PROGRAM_ARB,	vs);
		glBindProgram_(GL_FRAGMENT_PROGRAM_ARB, ps);
	}
	lastshader = this;
}

VARFN(shaders, useshaders, -1, -1, 1, initwarning());
VARF(shaderprecision, 0, 0, 2, initwarning());
VARP(shaderdetail, 0, MAXSHADERDETAIL, MAXSHADERDETAIL);

VAR(dbgshader, 0, 0, 1);

Shader *newshader(int type, const char *name, const char *vs, const char *ps, Shader *variant = NULL, int row = 0)
{
	char *rname = newstring(name);
	Shader &s = shaders[rname];
	s.name = rname;
	s.type = type;
	loopi(MAXSHADERDETAIL) s.fastshader[i] = &s;
	memset(s.extvertparams, 0, sizeof(s.extvertparams));
	memset(s.extpixparams, 0, sizeof(s.extpixparams));
	if(variant) loopv(variant->defaultparams) s.defaultparams.add(variant->defaultparams[i]);
	else loopv(curparams) s.defaultparams.add(curparams[i]);
	if(renderpath!=R_FIXEDFUNCTION)
	{
        bool reusevs = !vs[0], reuseps = !ps[0];
        if(type & SHADER_GLSLANG)
        {
            if(reusevs) s.vsobj = variant->vsobj;
            else compileglslshader(GL_VERTEX_SHADER_ARB,   s.vsobj, vs, "VS", name, !variant);
            if(reuseps) s.psobj = variant->psobj;
            else compileglslshader(GL_FRAGMENT_SHADER_ARB, s.psobj, ps, "PS", name, !variant);
            linkglslprogram(s, !variant);
            if(!s.program)
            {
                if(s.vsobj) { if(!reusevs) glDeleteObject_(s.vsobj); s.vsobj = 0; }
                if(s.psobj) { if(!reuseps) glDeleteObject_(s.psobj); s.psobj = 0; }
            }
        }
        else
        {
            if(reusevs) s.vs = variant->vs;
            else if(!compileasmshader(GL_VERTEX_PROGRAM_ARB, s.vs, vs, "VS", name, !variant, variant!=NULL))
                s.native = false;
            if(reuseps) s.ps = variant->ps;
            else if(!compileasmshader(GL_FRAGMENT_PROGRAM_ARB, s.ps, ps, "PS", name, !variant, variant!=NULL))
                s.native = false;
            if(!s.vs || !s.ps || (variant && !s.native))
            {
                if(s.vs) { if(!reusevs) glDeletePrograms_(1, &s.vs); s.vs = 0; }
                if(s.ps) { if(!reuseps) glDeletePrograms_(1, &s.ps); s.ps = 0; }
            }
        }
        if(!s.program && !s.vs && !s.ps)
        {
            shaders.remove(rname);
            return NULL;
        }
	}
    if(variant) variant->variants[row].add(&s);
	return &s;
}

static uint findusedtexcoords(const char *str)
{
	uint used = 0;
	for(;;)
	{
		const char *tc = strstr(str, "result.texcoord[");
		if(!tc) break;
		tc += strlen("result.texcoord[");
		int n = strtol(tc, (char **)&str, 10);
        if(n<0 || n>=16) continue;
		used |= 1<<n;
	}
	return used;
}

static bool findunusedtexcoordcomponent(const char *str, int &texcoord, int &component)
{
    uchar texcoords[16];
    memset(texcoords, 0, sizeof(texcoords));
    for(;;)
    {
        const char *tc = strstr(str, "result.texcoord[");
        if(!tc) break;
        tc += strlen("result.texcoord[");
        int n = strtol(tc, (char **)&str, 10);
        if(n<0 || n>=(int)sizeof(texcoords)) continue;
        while(*str && *str!=']') str++;
        if(*str==']')
        {
            if(*++str!='.') { texcoords[n] = 0xF; continue; }
            for(;;)
            {
                switch(*++str)
                {
                    case 'r': case 'x': texcoords[n] |= 1; continue;
                    case 'g': case 'y': texcoords[n] |= 2; continue;
                    case 'b': case 'z': texcoords[n] |= 4; continue;
                    case 'a': case 'w': texcoords[n] |= 8; continue;
                }
                break;
            }
        }
    }
    loopi(sizeof(texcoords)) if(texcoords[i]>0 && texcoords[i]<0xF)
    {
        loopk(4) if(!(texcoords[i]&(1<<k))) { texcoord = i; component = k; return true; }
    }
    return false;
}

#define EMUFOGVS(cond, vsbuf, start, end, fogcoord, fogtc, fogcomp) \
    if(cond) \
    { \
        vsbuf.put(start, fogcoord-start); \
        const char *afterfogcoord = fogcoord + strlen("result.fogcoord"); \
        if(*afterfogcoord=='.') afterfogcoord += 2; \
        s_sprintfd(repfogcoord)("result.texcoord[%d].%c", fogtc, fogcomp==3 ? 'w' : 'x'+fogcomp); \
        vsbuf.put(repfogcoord, strlen(repfogcoord)); \
        vsbuf.put(afterfogcoord, end-afterfogcoord); \
    } \
    else vsbuf.put(start, end-start);

#define EMUFOGPS(cond, psbuf, fogtc, fogcomp) \
    if(cond) \
    { \
        char *fogoption = strstr(psbuf.getbuf(), "OPTION ARB_fog_linear;"); \
        /*                    OPTION ARB_fog_linear; */ \
        const char *tmpdef = "TEMP emufogcolor;     "; \
        if(fogoption) while(*tmpdef) *fogoption++ = *tmpdef++; \
        /*                    result.color */\
        const char *tmpuse = " emufogcolor"; \
        char *str = psbuf.getbuf(); \
        for(;;) \
        { \
            str = strstr(str, "result.color"); \
            if(!str) break; \
            if(str[12]!='.' || (str[13]!='a' && str[13]!='w')) memcpy(str, tmpuse, strlen(tmpuse)); \
            str += 12; \
        } \
        s_sprintfd(fogtcstr)("fragment.texcoord[%d].%c", fogtc, fogcomp==3 ? 'w' : 'x'+fogcomp); \
        str = strstr(psbuf.getbuf(), "fragment.fogcoord.x"); \
        if(str) \
        { \
            int fogtclen = strlen(fogtcstr); \
            memcpy(str, fogtcstr, 19); \
            psbuf.insert(&str[19] - psbuf.getbuf(), &fogtcstr[19], fogtclen-19); \
        } \
        char *end = strstr(psbuf.getbuf(), "END"); \
        if(end) psbuf.setsizenodelete(end - psbuf.getbuf()); \
        s_sprintfd(calcfog)( \
            "TEMP emufog;\n" \
            "SUB emufog, state.fog.params.z, %s;\n" \
            "MUL_SAT emufog, emufog, state.fog.params.w;\n" \
            "LRP result.color.rgb, emufog, emufogcolor, state.fog.color;\n" \
            "END\n", \
            fogtcstr); \
        psbuf.put(calcfog, strlen(calcfog)+1); \
    }

VAR(reserveshadowmaptc, 1, 0, 0);
VAR(reservedynlighttc, 1, 0, 0);

static bool genwatervariant(Shader &s, const char *sname, vector<char> &vs, vector<char> &ps, int row)
{
    char *vspragma = strstr(vs.getbuf(), "#pragma CUBE2_water");
    if(!vspragma) return false;
    char *pspragma = strstr(ps.getbuf(), "#pragma CUBE2_water");
    if(!pspragma) return false;
    vspragma += strcspn(vspragma, "\n");
    if(*vspragma) vspragma++;
    pspragma += strcspn(pspragma, "\n");
    if(*pspragma) pspragma++;
    if(s.type & SHADER_GLSLANG)
    {
        const char *fadedef = "waterfade = gl_Vertex.z*fogselect.y + fogselect.z;\n";
        vs.insert(vspragma-vs.getbuf(), fadedef, strlen(fadedef));
        const char *fadeuse = "gl_FragColor.a = waterfade;\n";
        ps.insert(pspragma-ps.getbuf(), fadeuse, strlen(fadeuse));
        const char *fadedecl = "varying float waterfade;\n";
        vs.insert(0, fadedecl, strlen(fadedecl));
        ps.insert(0, fadedecl, strlen(fadedecl));
    }
    else
    {
        int fadetc = -1, fadecomp = -1;
        if(!findunusedtexcoordcomponent(vs.getbuf(), fadetc, fadecomp))
        {
            uint usedtc = findusedtexcoords(vs.getbuf());
            GLint maxtc = 0;
            glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &maxtc);
            int reservetc = row%2 ? reserveshadowmaptc : reservedynlighttc;
            loopi(maxtc-reservetc) if(!(usedtc&(1<<i))) { fadetc = i; fadecomp = 3; break; }
        }
        if(fadetc>=0)
        {
            s_sprintfd(fadedef)("MAD result.texcoord[%d].%c, opos.z, program.env[8].y, program.env[8].z;\n",
                                fadetc, fadecomp==3 ? 'w' : 'x'+fadecomp);
            vs.insert(vspragma-vs.getbuf(), fadedef, strlen(fadedef));
            s_sprintfd(fadeuse)("MOV result.color.a, fragment.texcoord[%d].%c;\n",
                                fadetc, fadecomp==3 ? 'w' : 'x'+fadecomp);
            ps.insert(pspragma-ps.getbuf(), fadeuse, strlen(fadeuse));
        }
        else // fallback - use fog value, works under water but not above
        {
            const char *fogfade = "MAD result.color.a, fragment.fogcoord.x, 0.25, 0.5;\n";
            ps.insert(pspragma-ps.getbuf(), fogfade, strlen(fogfade));
        }
    }
    s_sprintfd(name)("<water>%s", sname);
    Shader *variant = newshader(s.type, name, vs.getbuf(), ps.getbuf(), &s, row);
    return variant!=NULL;
}

static void genwatervariant(Shader &s, const char *sname, const char *vs, const char *ps, int row = 2)
{
    vector<char> vsw, psw;
    vsw.put(vs, strlen(vs)+1);
    psw.put(ps, strlen(ps)+1);
    genwatervariant(s, sname, vsw, psw, row);
}

static void gendynlightvariant(Shader &s, const char *sname, const char *vs, const char *ps, int row = 0)
{
	int numlights = 0, lights[MAXDYNLIGHTS];
    int emufogtc = -1, emufogcomp = -1;
    const char *emufogcoord = NULL;
	if(s.type & SHADER_GLSLANG) numlights = MAXDYNLIGHTS;
	else
	{
		uint usedtc = findusedtexcoords(vs);
		GLint maxtc = 0;
		glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &maxtc);
        int reservetc = row%2 ? reserveshadowmaptc : reservedynlighttc;
        if(maxtc-reservetc<0) return;
        loopi(maxtc-reservetc) if(!(usedtc&(1<<i)))
		{
			lights[numlights++] = i;
			if(numlights>=MAXDYNLIGHTS) break;
		}
        extern int emulatefog;
        if(emulatefog && reservetc>0 && numlights+1<MAXDYNLIGHTS && !(usedtc&(1<<(maxtc-reservetc))) && strstr(ps, "OPTION ARB_fog_linear;"))
        {
            emufogcoord = strstr(vs, "result.fogcoord");
            if(emufogcoord)
            {
                if(!findunusedtexcoordcomponent(vs, emufogtc, emufogcomp))
                {
                    emufogtc = maxtc-reservetc;
                    emufogcomp = 3;
                }
                lights[numlights++] = maxtc-reservetc;
            }
        }
		if(!numlights) return;
	}

	const char *vspragma = strstr(vs, "#pragma CUBE2_dynlight"), *pspragma = strstr(ps, "#pragma CUBE2_dynlight");
	string pslight;
	vspragma += strcspn(vspragma, "\n");
	if(*vspragma) vspragma++;

	if(sscanf(pspragma, "#pragma CUBE2_dynlight %s", pslight)!=1) return;

	pspragma += strcspn(pspragma, "\n");
	if(*pspragma) pspragma++;

	vector<char> vsdl, psdl;
	loopi(numlights)
	{
		vsdl.setsizenodelete(0);
		psdl.setsizenodelete(0);

		if(s.type & SHADER_GLSLANG)
		{
			loopk(i+1)
			{
				s_sprintfd(pos)("%sdynlight%dpos%s", !k ? "uniform vec4 " : " ", k, k==i ? ";\n" : ",");
				vsdl.put(pos, strlen(pos));

				s_sprintfd(color)("%sdynlight%dcolor%s", !k ? "uniform vec4 " : " ", k, k==i ? ";\n" : ",");
				psdl.put(color, strlen(color));
			}
			loopk(i+1)
			{
				s_sprintfd(dir)("%sdynlight%ddir%s", !k ? "varying vec3 " : " ", k, k==i ? ";\n" : ",");
				vsdl.put(dir, strlen(dir));
				psdl.put(dir, strlen(dir));
			}
		}

        EMUFOGVS(emufogcoord && i+1==numlights && emufogcoord < vspragma, vsdl, vs, vspragma, emufogcoord, emufogtc, emufogcomp);
		psdl.put(ps, pspragma-ps);

        loopk(i+1)
        {
            extern int ati_dph_bug;
            string tc, dl;
            if(s.type & SHADER_GLSLANG) s_sprintf(tc)(
                "dynlight%ddir = gl_Vertex.xyz*dynlight%dpos.w + dynlight%dpos.xyz;\n",
                k, k, k);
            else if(ati_dph_bug || lights[k]==emufogtc) s_sprintf(tc)(
                "MAD result.texcoord[%d].xyz, vertex.position, program.env[%d].w, program.env[%d];\n",
                lights[k], 10+k, 10+k);
            else s_sprintf(tc)(
                "MAD result.texcoord[%d].xyz, vertex.position, program.env[%d].w, program.env[%d];\n"
                "MOV result.texcoord[%d].w, 1;\n",
                lights[k], 10+k, 10+k, lights[k]);
            vsdl.put(tc, strlen(tc));

            if(s.type & SHADER_GLSLANG) s_sprintf(dl)(
                "%s.rgb += dynlight%dcolor.rgb * (1.0 - clamp(dot(dynlight%ddir, dynlight%ddir), 0.0, 1.0));\n",
                pslight, k, k, k);
            else if(ati_dph_bug || lights[k]==emufogtc) s_sprintf(dl)(
                "%s"
                "DP3_SAT dynlight.x, fragment.texcoord[%d], fragment.texcoord[%d];\n"
                "SUB dynlight.x, 1, dynlight.x;\n"
                "MAD %s.rgb, program.env[%d], dynlight.x, %s;\n",
                !k ? "TEMP dynlight;\n" : "",
                lights[k], lights[k],
                pslight, 10+k, pslight);
            else s_sprintf(dl)(
                "%s"
                "DPH_SAT dynlight.x, -fragment.texcoord[%d], fragment.texcoord[%d];\n"
                "MAD %s.rgb, program.env[%d], dynlight.x, %s;\n",
                !k ? "TEMP dynlight;\n" : "",
                lights[k], lights[k],
                pslight, 10+k, pslight);
            psdl.put(dl, strlen(dl));
        }

        EMUFOGVS(emufogcoord && i+1==numlights && emufogcoord >= vspragma, vsdl, vspragma, vspragma+strlen(vspragma)+1, emufogcoord, emufogtc, emufogcomp);
		psdl.put(pspragma, strlen(pspragma)+1);

        EMUFOGPS(emufogcoord && i+1==numlights, psdl, emufogtc, emufogcomp);

        s_sprintfd(name)("<dynlight %d>%s", i+1, sname);
        Shader *variant = newshader(s.type, name, vsdl.getbuf(), psdl.getbuf(), &s, row);
        if(!variant) return;
        genwatervariant(s, name, vsdl, psdl, row+2);
	}
}

static void genshadowmapvariant(Shader &s, const char *sname, const char *vs, const char *ps, int row = 1)
{
    int smtc = -1, emufogtc = -1, emufogcomp = -1;
    const char *emufogcoord = NULL;
    if(!(s.type & SHADER_GLSLANG))
    {
        uint usedtc = findusedtexcoords(vs);
        GLint maxtc = 0;
        glGetIntegerv(GL_MAX_TEXTURE_COORDS_ARB, &maxtc);
        if(maxtc-reserveshadowmaptc<0) return;
        loopi(maxtc-reserveshadowmaptc) if(!(usedtc&(1<<i))) { smtc = i; break; }
        extern int emulatefog;
        if(smtc<0 && emulatefog && reserveshadowmaptc>0 && !(usedtc&(1<<(maxtc-reserveshadowmaptc))) && strstr(ps, "OPTION ARB_fog_linear;"))
        {
            emufogcoord = strstr(vs, "result.fogcoord");
            if(!emufogcoord || !findunusedtexcoordcomponent(vs, emufogtc, emufogcomp)) return;
            smtc = maxtc-reserveshadowmaptc;
        }
        if(smtc<0) return;
    }

    const char *vspragma = strstr(vs, "#pragma CUBE2_shadowmap"), *pspragma = strstr(ps, "#pragma CUBE2_shadowmap");
    string pslight;
    vspragma += strcspn(vspragma, "\n");
    if(*vspragma) vspragma++;

    if(sscanf(pspragma, "#pragma CUBE2_shadowmap %s", pslight)!=1) return;

    pspragma += strcspn(pspragma, "\n");
    if(*pspragma) pspragma++;

    vector<char> vssm, pssm;

    if(s.type & SHADER_GLSLANG)
    {
        const char *tc = "varying vec3 shadowmaptc;\n";
        vssm.put(tc, strlen(tc));
        pssm.put(tc, strlen(tc));
        const char *smtex =
            "uniform sampler2D shadowmap;\n"
            "uniform vec4 shadowmapambient;\n";
        pssm.put(smtex, strlen(smtex));
        if(!strstr(ps, "ambient"))
        {
            const char *amb = "uniform vec4 ambient;\n";
            pssm.put(amb, strlen(amb));
        }
    }

    EMUFOGVS(emufogcoord && emufogcoord < vspragma, vssm, vs, vspragma, emufogcoord, emufogtc, emufogcomp);
    pssm.put(ps, pspragma-ps);

    if(s.type & SHADER_GLSLANG)
    {
        const char *tc =
            "shadowmaptc = vec3(gl_TextureMatrix[2] * gl_Vertex);\n";
        vssm.put(tc, strlen(tc));
        const char *sm =
            "vec3 smvals = texture2D(shadowmap, shadowmaptc.xy).xyz;\n"
            "vec2 smdiff = shadowmaptc.zz*smvals.y + smvals.xz;\n"
            "float shadowed = smdiff.x > 0.0 && smdiff.y < 0.0 ? smvals.y : 0.0;\n";
        pssm.put(sm, strlen(sm));
        s_sprintfd(smlight)(
            "%s.rgb = mix(%s.rgb, min(%s.rgb, shadowmapambient.rgb), shadowed);\n",
            pslight, pslight, pslight);
        pssm.put(smlight, strlen(smlight));
    }
    else
    {
        s_sprintfd(tc)(
            "DP4 result.texcoord[%d].x, state.matrix.texture[2].row[0], vertex.position;\n"
            "DP4 result.texcoord[%d].y, state.matrix.texture[2].row[1], vertex.position;\n"
            "DP4 result.texcoord[%d].z, state.matrix.texture[2].row[2], vertex.position;\n",
            smtc, smtc, smtc);
        vssm.put(tc, strlen(tc));

        s_sprintfd(sm)(
            "TEMP smvals, shadowed, smambient;\n"
            "TEX smvals, fragment.texcoord[%d], texture[7], 2D;\n"
            "MAD smvals.xz, fragment.texcoord[%d].z, smvals.y, smvals;\n",
            smtc, smtc);
        pssm.put(sm, strlen(sm));
        s_sprintf(sm)(
            "CMP shadowed, -smvals.x, smvals.y, 0;\n"
            "CMP shadowed, smvals.z, shadowed, 0;\n" 
            "MIN smambient.rgb, program.env[7], %s;\n"
            "LRP %s.rgb, shadowed, smambient, %s;\n",
            pslight, pslight, pslight);
        pssm.put(sm, strlen(sm));
    }

    EMUFOGVS(emufogcoord && emufogcoord >= vspragma, vssm, vspragma, vspragma+strlen(vspragma)+1, emufogcoord, emufogtc, emufogcomp);
    pssm.put(pspragma, strlen(pspragma)+1);

    EMUFOGPS(emufogcoord, pssm, emufogtc, emufogcomp);

    s_sprintfd(name)("<shadowmap>%s", sname);
    Shader *variant = newshader(s.type, name, vssm.getbuf(), pssm.getbuf(), &s, row);
    if(!variant) return;
    genwatervariant(s, name, vssm.getbuf(), pssm.getbuf(), row+2);

    if(strstr(vs, "#pragma CUBE2_dynlight")) gendynlightvariant(s, name, vssm.getbuf(), pssm.getbuf(), row);
}

void shader(int *type, char *name, char *vs, char *ps)
{
	if(lookupshaderbyname(name)) return;

	if(renderpath!=R_FIXEDFUNCTION)
	{
		if((renderpath!=R_GLSLANG && *type & SHADER_GLSLANG) ||
			(!hasCM && strstr(ps, *type & SHADER_GLSLANG ? "textureCube" : "CUBE")) ||
			(!hasTR && strstr(ps, *type & SHADER_GLSLANG ? "texture2DRect" : "RECT")))
		{
			loopv(curparams)
			{
				if(curparams[i].name) delete[] curparams[i].name;
			}
			curparams.setsize(0);
			return;
		}
        s_sprintfd(info)("shader %s", name);
        show_out_of_renderloop_progress(0.0, info);
	}
	Shader *s = newshader(*type, name, vs, ps);
	if(s && renderpath!=R_FIXEDFUNCTION)
	{
		// '#' is a comment in vertex/fragment programs, while '#pragma' allows an escape for GLSL, so can handle both at once
        if(strstr(vs, "#pragma CUBE2_water")) genwatervariant(*s, s->name, vs, ps);
        if(strstr(vs, "#pragma CUBE2_shadowmap")) genshadowmapvariant(*s, s->name, vs, ps);
        if(strstr(vs, "#pragma CUBE2_dynlight")) gendynlightvariant(*s, s->name, vs, ps);
	}
	curparams.setsize(0);
}

void variantshader(int *type, char *name, int *row, char *vs, char *ps)
{
    if(renderpath==R_FIXEDFUNCTION) return;

    Shader *s = lookupshaderbyname(name);
    if(!s) return;

    s_sprintfd(varname)("<variant:%d,%d>%s", s->variants[*row].length(), *row, name);
    //s_sprintfd(info)("shader %s", varname);
    //show_out_of_renderloop_progress(0.0, info);
    newshader(*type, varname, vs, ps, s, *row);
}

void setshader(char *name)
{
	Shader *s = lookupshaderbyname(name);
	if(!s)
	{
		if(renderpath!=R_FIXEDFUNCTION) conoutf("no such shader: %s", name);
	}
	else curshader = s;
	loopv(curparams)
	{
		if(curparams[i].name) delete[] curparams[i].name;
	}
	curparams.setsize(0);
}

ShaderParam *findshaderparam(Slot &s, const char *name, int type, int index)
{
	if(!s.shader) return NULL;
	loopv(s.params)
	{
		ShaderParam &param = s.params[i];
		if((name && param.name && !strcmp(name, param.name)) || (param.type==type && param.index==index)) return &param;
	}
	loopv(s.shader->defaultparams)
	{
		ShaderParam &param = s.shader->defaultparams[i];
		if((name && param.name && !strcmp(name, param.name)) || (param.type==type && param.index==index)) return &param;
	}
	return NULL;
}

void setslotshader(Slot &s)
{
	s.shader = curshader ? curshader : defaultshader;
	if(!s.shader) return;
	loopv(curparams)
	{
		ShaderParam &param = curparams[i], *defaultparam = findshaderparam(s, param.name, param.type, param.index);
		if(!defaultparam || !memcmp(param.val, defaultparam->val, sizeof(param.val))) continue;
		ShaderParam &override = s.params.add(param);
		override.name = defaultparam->name;
		if(s.shader->type&SHADER_GLSLANG) override.index = (LocalShaderParamState *)defaultparam - &s.shader->defaultparams[0];
	}
}

VAR(nativeshaders, 0, 1, 1);

void altshader(char *origname, char *altname)
{
	Shader *alt = lookupshaderbyname(altname);
	if(!alt) return;
    Shader *orig = lookupshaderbyname(origname);
    if(orig)
    {
        if(nativeshaders && !orig->native) orig->altshader = alt;
        return;
    }
    char *rname = newstring(origname);
	Shader &s = shaders[rname];
	s.name = rname;
	s.altshader = alt;
}

void fastshader(char *nice, char *fast, int *detail)
{
	Shader *ns = shaders.access(nice);
	if(!ns || ns->altshader) return;
	Shader *fs = lookupshaderbyname(fast);
	if(!fs) return;
	loopi(min(*detail+1, MAXSHADERDETAIL)) ns->fastshader[i] = fs;
}

COMMAND(shader, "isss");
COMMAND(variantshader, "isiss");
COMMAND(setshader, "s");
COMMAND(altshader, "ss");
COMMAND(fastshader, "ssi");

void isshaderdefined(char *name)
{
    Shader *s = lookupshaderbyname(name);
    intret(s ? 1 : 0);
}

void isshadernative(char *name)
{
    Shader *s = lookupshaderbyname(name);
    intret(s && s->native ? 1 : 0);
}

COMMAND(isshaderdefined, "s");
COMMAND(isshadernative, "s");

void setshaderparam(char *name, int type, int n, float x, float y, float z, float w)
{
	if(!name && (n<0 || n>=MAXSHADERPARAMS))
	{
		conoutf("shader param index must be 0..%d\n", MAXSHADERPARAMS-1);
		return;
	}
	loopv(curparams)
	{
		ShaderParam &param = curparams[i];
		if(param.type == type && (name ? !strstr(param.name, name) : param.index == n))
		{
			param.val[0] = x;
			param.val[1] = y;
			param.val[2] = z;
			param.val[3] = w;
			return;
		}
	}
	ShaderParam param = {name ? newstring(name) : NULL, type, n, -1, {x, y, z, w}};
	curparams.add(param);
}

void setvertexparam(int *n, float *x, float *y, float *z, float *w)
{
	setshaderparam(NULL, SHPARAM_VERTEX, *n, *x, *y, *z, *w);
}

void setpixelparam(int *n, float *x, float *y, float *z, float *w)
{
	setshaderparam(NULL, SHPARAM_PIXEL, *n, *x, *y, *z, *w);
}

void setuniformparam(char *name, float *x, float *y, float *z, float *w)
{
	setshaderparam(name, SHPARAM_UNIFORM, -1, *x, *y, *z, *w);
}

COMMAND(setvertexparam, "iffff");
COMMAND(setpixelparam, "iffff");
COMMAND(setuniformparam, "sffff");

const int NUMSCALE = 7;
Shader *fsshader = NULL, *scaleshader = NULL, *initshader = NULL;
GLuint rendertarget[NUMSCALE];
GLuint fsfb[NUMSCALE-1];
GLfloat fsparams[4];
int fs_w = 0, fs_h = 0, fspasses = NUMSCALE, fsskip = 1;

void setfullscreenshader(char *name, int *x, int *y, int *z, int *w)
{
	if(!hasTR || !*name)
	{
		fsshader = NULL;
	}
	else
	{
		Shader *s = lookupshaderbyname(name);
		if(!s) return conoutf("no such fullscreen shader: %s", name);
		fsshader = s;
		s_sprintfd(ssname)("%s_scale", name);
		s_sprintfd(isname)("%s_init", name);
		scaleshader = lookupshaderbyname(ssname);
		initshader = lookupshaderbyname(isname);
		fspasses = NUMSCALE;
		fsskip = 1;
		if(scaleshader)
		{
			int len = strlen(name);
			char c = name[--len];
			if(isdigit(c))
			{
				if(len>0 && isdigit(name[--len]))
				{
					fsskip = c-'0';
					fspasses = name[len]-'0';
				}
				else fspasses = c-'0';
			}
		}
		conoutf("now rendering with: %s", name);
		fsparams[0] = *x/255.0f;
		fsparams[1] = *y/255.0f;
		fsparams[2] = *z/255.0f;
		fsparams[3] = *w/255.0f;
	}
}

COMMAND(setfullscreenshader, "siiii");

void renderfsquad(int w, int h, Shader *s)
{
	s->set();
	glViewport(0, 0, w, h);
	if(s==scaleshader || s==initshader)
	{
		w <<= fsskip;
		h <<= fsskip;
	}
	glBegin(GL_QUADS);
	glTexCoord2i(0, 0); glVertex3f(-1, -1, 0);
	glTexCoord2i(w, 0); glVertex3f( 1, -1, 0);
	glTexCoord2i(w, h); glVertex3f( 1,  1, 0);
	glTexCoord2i(0, h); glVertex3f(-1,  1, 0);
	glEnd();
}

void renderfullscreenshader(int w, int h)
{
	if(!fsshader || renderpath==R_FIXEDFUNCTION) return;

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_TEXTURE_RECTANGLE_ARB);

	if(fs_w != w || fs_h != h)
	{
		if(!fs_w && !fs_h)
		{
			glGenTextures(NUMSCALE, rendertarget);
			if(hasFBO) glGenFramebuffers_(NUMSCALE-1, fsfb);
		}
		loopi(NUMSCALE)
			createtexture(rendertarget[i], w>>i, h>>i, NULL, 3, false, GL_RGB, GL_TEXTURE_RECTANGLE_ARB);
		fs_w = w;
		fs_h = h;
		if(fsfb[0])
		{
			loopi(NUMSCALE-1)
			{
				glBindFramebuffer_(GL_FRAMEBUFFER_EXT, fsfb[i]);
				glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_RECTANGLE_ARB, rendertarget[i+1], 0);
			}
			glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
		}
	}

	setenvparamfv("fsparams", SHPARAM_PIXEL, 0, fsparams);
	setenvparamf("millis", SHPARAM_VERTEX, 1, lastmillis/1000.0f, lastmillis/1000.0f, lastmillis/1000.0f);

	int nw = w, nh = h;

	loopi(fspasses)
	{
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, rendertarget[i*fsskip]);
		glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0, nw, nh);
		if(i>=fspasses-1 || !scaleshader || fsfb[0]) break;
		renderfsquad(nw >>= fsskip, nh >>= fsskip, !i && initshader ? initshader : scaleshader);
	}
	if(scaleshader && fsfb[0])
	{
		loopi(fspasses-1)
		{
			if(i) glBindTexture(GL_TEXTURE_RECTANGLE_ARB, rendertarget[i*fsskip]);
			glBindFramebuffer_(GL_FRAMEBUFFER_EXT, fsfb[(i+1)*fsskip-1]);
			renderfsquad(nw >>= fsskip, nh >>= fsskip, !i && initshader ? initshader : scaleshader);
		}
		glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
	}

	if(scaleshader) loopi(fspasses)
	{
		glActiveTexture_(GL_TEXTURE0_ARB+i);
		glEnable(GL_TEXTURE_RECTANGLE_ARB);
		glBindTexture(GL_TEXTURE_RECTANGLE_ARB, rendertarget[i*fsskip]);
	}
	renderfsquad(w, h, fsshader);

	if(scaleshader) loopi(fspasses)
	{
		glActiveTexture_(GL_TEXTURE0_ARB+i);
		glDisable(GL_TEXTURE_RECTANGLE_ARB);
	}

	glActiveTexture_(GL_TEXTURE0_ARB);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
}

struct tmufunc
{
	GLenum combine, sources[3], ops[3];
	int scale;
};

struct tmu
{
	GLenum mode;
	GLfloat color[4];
	tmufunc rgb, alpha;
};

#define INVALIDTMU \
{ \
	0, \
	{ -1, -1, -1, -1 }, \
	{ 0, { 0, 0, 0, }, { 0, 0, 0 }, 0 }, \
	{ 0, { 0, 0, 0, }, { 0, 0, 0 }, 0 } \
}

#define INITTMU \
{ \
	GL_MODULATE, \
	{ 0, 0, 0, 0 }, \
	{ GL_MODULATE, { GL_TEXTURE, GL_PREVIOUS_ARB, GL_CONSTANT_ARB }, { GL_SRC_COLOR, GL_SRC_COLOR, GL_SRC_ALPHA }, 1 }, \
	{ GL_MODULATE, { GL_TEXTURE, GL_PREVIOUS_ARB, GL_CONSTANT_ARB }, { GL_SRC_ALPHA, GL_SRC_ALPHA, GL_SRC_ALPHA }, 1 } \
}

#define MAXTMUS 8

tmu tmus[MAXTMUS] =
{
	INVALIDTMU,
	INVALIDTMU,
	INVALIDTMU,
    INVALIDTMU,
    INVALIDTMU,
    INVALIDTMU,
    INVALIDTMU,
	INVALIDTMU
};

VAR(maxtmus, 1, 0, 0);

void parsetmufunc(tmufunc &f, const char *s)
{
	int arg = -1;
	while(*s) switch(tolower(*s++))
	{
		case 't': f.sources[++arg] = GL_TEXTURE; f.ops[arg] = GL_SRC_COLOR; break;
		case 'p': f.sources[++arg] = GL_PREVIOUS_ARB; f.ops[arg] = GL_SRC_COLOR; break;
		case 'k': f.sources[++arg] = GL_CONSTANT_ARB; f.ops[arg] = GL_SRC_COLOR; break;
		case 'c': f.sources[++arg] = GL_PRIMARY_COLOR_ARB; f.ops[arg] = GL_SRC_COLOR; break;
		case '~': f.ops[arg] = GL_ONE_MINUS_SRC_COLOR; break;
		case 'a': f.ops[arg] = f.ops[arg]==GL_ONE_MINUS_SRC_COLOR ? GL_ONE_MINUS_SRC_ALPHA : GL_SRC_ALPHA; break;
		case '=': f.combine = GL_REPLACE; break;
		case '*': f.combine = GL_MODULATE; break;
		case '+': f.combine = GL_ADD; break;
		case '-': f.combine = GL_SUBTRACT_ARB; break;
		case ',':
		case '@': f.combine = GL_INTERPOLATE_ARB; break;
		case '.': f.combine = GL_DOT3_RGB_ARB; break;
		case 'x': while(!isdigit(*s)) s++; f.scale = *s++-'0'; break;
	}
}

void committmufunc(bool rgb, tmufunc &dst, tmufunc &src)
{
	if(dst.combine!=src.combine) glTexEnvi(GL_TEXTURE_ENV, rgb ? GL_COMBINE_RGB_ARB : GL_COMBINE_ALPHA_ARB, src.combine);
	loopi(3)
	{
		if(dst.sources[i]!=src.sources[i]) glTexEnvi(GL_TEXTURE_ENV, (rgb ? GL_SOURCE0_RGB_ARB : GL_SOURCE0_ALPHA_ARB)+i, src.sources[i]);
		if(dst.ops[i]!=src.ops[i]) glTexEnvi(GL_TEXTURE_ENV, (rgb ? GL_OPERAND0_RGB_ARB : GL_OPERAND0_ALPHA_ARB)+i, src.ops[i]);
	}
	if(dst.scale!=src.scale) glTexEnvi(GL_TEXTURE_ENV, rgb ? GL_RGB_SCALE_ARB : GL_ALPHA_SCALE, src.scale);
}

void committmu(int n, tmu &f)
{
	if(renderpath!=R_FIXEDFUNCTION || n>=maxtmus) return;
	if(tmus[n].mode!=f.mode) glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, f.mode);
	if(memcmp(tmus[n].color, f.color, sizeof(f.color))) glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, f.color);
	committmufunc(true, tmus[n].rgb, f.rgb);
	committmufunc(false, tmus[n].alpha, f.alpha);
	tmus[n] = f;
}

void resettmu(int n)
{
	tmu f = tmus[n];
	f.mode = GL_MODULATE;
	f.rgb.scale = 1;
	f.alpha.scale = 1;
	committmu(n, f);
}

void scaletmu(int n, int rgbscale, int alphascale)
{
	tmu f = tmus[n];
	if(rgbscale) f.rgb.scale = rgbscale;
	if(alphascale) f.alpha.scale = alphascale;
	committmu(n, f);
}

void colortmu(int n, float r, float g, float b, float a)
{
	tmu f = tmus[n];
	f.color[0] = r;
	f.color[1] = g;
	f.color[2] = b;
	f.color[3] = a;
	committmu(n, f);
}

void setuptmu(int n, const char *rgbfunc, const char *alphafunc)
{
	static tmu init = INITTMU;
	tmu f = tmus[n];

	f.mode = GL_COMBINE_ARB;
	if(rgbfunc) parsetmufunc(f.rgb, rgbfunc);
	else f.rgb = init.rgb;
	if(alphafunc) parsetmufunc(f.alpha, alphafunc);
	else f.alpha = init.alpha;

	committmu(n, f);
}

VAR(nolights, 1, 0, 0);
VAR(nowater, 1, 0, 0);
VAR(nomasks, 1, 0, 0);

void inittmus()
{
	if(hasTE && hasMT)
	{
        GLint val;
		glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &val);
        maxtmus = max(1, min(MAXTMUS, int(val)));
		loopi(maxtmus)
		{
			glActiveTexture_(GL_TEXTURE0_ARB+i);
			resettmu(i);
		}
		glActiveTexture_(GL_TEXTURE0_ARB);
	}
    else if(hasTE) { maxtmus = 1; resettmu(0); }
    if(renderpath==R_FIXEDFUNCTION)
    {
        if(maxtmus<4) caustics = 0;
        if(maxtmus<2)
	{
		nolights = nowater = nomasks = 1;
		extern int lightmodels;
		lightmodels = 0;
            refractfog = 0;
        }
	}
}

