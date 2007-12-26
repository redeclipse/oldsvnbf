// texture.cpp: texture slot management

#include "pch.h"
#include "engine.h"

SDL_Surface *texrotate(SDL_Surface *s, int numrots, int type)
{
	// 1..3 rotate through 90..270 degrees, 4 flips X, 5 flips Y
	if(numrots<1 || numrots>5) return s;
	SDL_Surface *d = SDL_CreateRGBSurface(SDL_SWSURFACE, (numrots&5)==1 ? s->h : s->w, (numrots&5)==1 ? s->w : s->h, s->format->BitsPerPixel, s->format->Rmask, s->format->Gmask, s->format->Bmask, s->format->Amask);
	if(!d) fatal("create surface");
	int depth = s->format->BytesPerPixel;
	loop(y, s->h) loop(x, s->w)
	{
		uchar *src = (uchar *)s->pixels+(y*s->w+x)*depth;
		int dx = x, dy = y;
		if(numrots>=2 && numrots<=4) dx = (s->w-1)-x;
		if(numrots<=2 || numrots==5) dy = (s->h-1)-y;
		if((numrots&5)==1) swap(int, dx, dy);
		uchar *dst = (uchar *)d->pixels+(dy*d->w+dx)*depth;
		loopi(depth) dst[i]=src[i];
		if(type==TEX_NORMAL)
		{
			if(numrots>=2 && numrots<=4) dst[0] = 255-dst[0];	  // flip X	on normal when 180/270 degrees
			if(numrots<=2 || numrots==5) dst[1] = 255-dst[1];	  // flip Y	on normal when  90/180 degrees
			if((numrots&5)==1) swap(uchar, dst[0], dst[1]);		// swap X/Y on normal when  90/270 degrees
		}
	}
	SDL_FreeSurface(s);
	return d;
}

SDL_Surface *texoffset(SDL_Surface *s, int xoffset, int yoffset)
{
	xoffset = max(xoffset, 0);
	xoffset %= s->w;
	yoffset = max(yoffset, 0);
	yoffset %= s->h;
	if(!xoffset && !yoffset) return s;
	SDL_Surface *d = SDL_CreateRGBSurface(SDL_SWSURFACE, s->w, s->h, s->format->BitsPerPixel, s->format->Rmask, s->format->Gmask, s->format->Bmask, s->format->Amask);
	if(!d) fatal("create surface");
	int depth = s->format->BytesPerPixel;
	uchar *src = (uchar *)s->pixels;
	loop(y, s->h)
	{
		uchar *dst = (uchar *)d->pixels+((y+yoffset)%d->h)*d->pitch;
		memcpy(dst+xoffset*depth, src, (s->w-xoffset)*depth);
		memcpy(dst, src+(s->w-xoffset)*depth, xoffset*depth);
		src += s->pitch;
	}
	SDL_FreeSurface(s);
	return d;
}

void texmad(SDL_Surface *s, const vec &mul, const vec &add)
{
	int maxk = min(s->format->BytesPerPixel, 3);
	uchar *src = (uchar *)s->pixels;
	loopi(s->h*s->w)
	{
		loopk(maxk)
		{
			float val = src[k]*mul[k] + 255*add[k];
			src[k] = uchar(min(max(val, 0), 255));
		}
		src += s->format->BytesPerPixel;
	}
}

SDL_Surface *texffmask(SDL_Surface *s, int minval)
{
	if(nomasks || s->format->BytesPerPixel<3) { SDL_FreeSurface(s); return NULL; }
	bool glow = false, envmap = true;
	uchar *src = (uchar *)s->pixels;
	loopi(s->h*s->w)
	{
		if(src[1]>minval) glow = true;
		if(src[2]>minval) { glow = envmap = true; break; }
		src += s->format->BytesPerPixel;
	}
	if(!glow && !envmap) { SDL_FreeSurface(s); return NULL; }
	SDL_Surface *m = SDL_CreateRGBSurface(SDL_SWSURFACE, s->w, s->h, envmap ? 16 : 8, 0, 0, 0, 0);
	if(!m) fatal("create surface");
	uchar *dst = (uchar *)m->pixels;
	src = (uchar *)s->pixels;
	loopi(s->h*s->w)
	{
		*dst++ = src[1];
		if(envmap) *dst++ = src[2];
		src += s->format->BytesPerPixel;
	}
	SDL_FreeSurface(s);
	return m;
}

void texdup(SDL_Surface *s, int srcchan, int dstchan)
{
    if(srcchan==dstchan || max(srcchan, dstchan) >= s->format->BytesPerPixel) return;
    uchar *src = (uchar *)s->pixels;
    loopi(s->h*s->w)
    {
        src[dstchan] = src[srcchan];
        src += s->format->BytesPerPixel;
    }
}

VAR(hwtexsize, 1, 0, 0);
VAR(hwcubetexsize, 1, 0, 0);
VAR(hwmaxaniso, 1, 0, 0);
VARP(maxtexsize, 0, 0, 1<<12);
VARP(texreduce, 0, 0, 12);
VARP(texcompress, 0, 1<<10, 1<<12);
VARP(trilinear, 0, 1, 1);
VARP(bilinear, 0, 1, 1);
VARP(aniso, 0, 0, 16);

GLenum compressedformat(GLenum format, int w, int h)
{
#ifdef __APPLE__
#undef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#undef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT GL_COMPRESSED_RGB_ARB
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT GL_COMPRESSED_RGBA_ARB
#endif
	if(hasTC && texcompress && max(w, h) >= texcompress) switch(format)
	{
		case GL_RGB5:
		case GL_RGB8:
		case GL_RGB: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case GL_RGBA: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}
	return format;
}

int formatsize(GLenum format)
{
	switch(format)
	{
		case GL_LUMINANCE:
		case GL_ALPHA: return 1;
		case GL_LUMINANCE_ALPHA: return 2;
		case GL_RGB: return 3;
		case GL_RGBA: return 4;
		default: return 4;
	}
}

void createtexture(int tnum, int w, int h, void *pixels, int clamp, bool mipit, GLenum component, GLenum subtarget)
{
	GLenum target = subtarget;
	switch(subtarget)
	{
		case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
		case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
		case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
			target = GL_TEXTURE_CUBE_MAP_ARB;
			break;
	}
	if(tnum)
	{
		glBindTexture(target, tnum);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, clamp&1 ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        if(target!=GL_TEXTURE_1D) glTexParameteri(target, GL_TEXTURE_WRAP_T, clamp&2 ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        if(target==GL_TEXTURE_2D && hasAF && min(aniso, hwmaxaniso) > 0) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, min(aniso, hwmaxaniso));
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, bilinear ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
			mipit ?
				(trilinear ?
					(bilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR) :
					(bilinear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST)) :
				(bilinear ? GL_LINEAR : GL_NEAREST));
	}
	GLenum format = component, type = GL_UNSIGNED_BYTE;
	switch(component)
	{
        case GL_RGB16F_ARB:
        case GL_RGB32F_ARB:
            format = GL_RGB;
            type = GL_FLOAT;
            break;

        case GL_RGBA16F_ARB:
        case GL_RGBA32F_ARB:
            format = GL_RGBA;
            type = GL_FLOAT;
            break;

		case GL_DEPTH_COMPONENT:
			type = GL_FLOAT;
			break;

        case GL_RGB5:
		case GL_RGB8:
        case GL_RGB16:
			format = GL_RGB;
			break;

        case GL_RGBA8:
        case GL_RGBA16:
            format = GL_RGBA;
            break;
	}
	uchar *scaled = NULL;
    int hwlimit = target==GL_TEXTURE_CUBE_MAP_ARB ? hwcubetexsize : hwtexsize,
        sizelimit = mipit && maxtexsize ? min(maxtexsize, hwlimit) : hwlimit;
    if(pixels && max(w, h) > sizelimit && (!mipit || sizelimit < hwlimit))
	{
		int oldw = w, oldh = h;
		while(w > sizelimit || h > sizelimit) { w /= 2; h /= 2; }
		scaled = new uchar[w*h*formatsize(format)];
		gluScaleImage(format, oldw, oldh, type, pixels, w, h, type, scaled);
		pixels = scaled;
	}
	if(mipit && pixels)
	{
		GLenum compressed = compressedformat(component, w, h);
        if(target==GL_TEXTURE_1D)
        {
            if(gluBuild1DMipmaps(subtarget, compressed, w, format, type, pixels))
            {
                if(compressed==component || gluBuild1DMipmaps(subtarget, component, w, format, type, pixels)) conoutf("could not build mipmaps");
            }
        }
		else if(gluBuild2DMipmaps(subtarget, compressed, w, h, format, type, pixels))
		{
			if(compressed==component || gluBuild2DMipmaps(subtarget, component, w, h, format, type, pixels)) conoutf("could not build mipmaps");
		}
	}
    else if(target==GL_TEXTURE_1D) glTexImage1D(subtarget, 0, component, w, 0, format, type, pixels);
	else glTexImage2D(subtarget, 0, component, w, h, 0, format, type, pixels);
	if(scaled) delete[] scaled;
}

hashtable<char *, Texture> textures;

Texture *notexture = NULL; // used as default, ensured to be loaded

static GLenum texformat(int bpp)
{
	switch(bpp)
	{
		case 8: return GL_LUMINANCE;
		case 16: return GL_LUMINANCE_ALPHA;
		case 24: return GL_RGB;
		case 32: return GL_RGBA;
		default: return 0;
	}
}

static void resizetexture(int &w, int &h, bool mipit = true, GLenum format = GL_RGB, GLenum target = GL_TEXTURE_2D)
{
    if(mipit) return;
    int hwlimit = target==GL_TEXTURE_CUBE_MAP_ARB ? hwcubetexsize : hwtexsize,
        sizelimit = mipit && maxtexsize ? min(maxtexsize, hwlimit) : hwlimit;
	int w2 = w, h2 = h;
	if(!hasNP2 && (w&(w-1) || h&(h-1)))
	{
		w2 = h2 = 1;
		while(w2 < w) w2 *= 2;
		while(h2 < h) h2 *= 2;
		if(w2 > sizelimit || (w - w2)/2 < (w2 - w)/2) w2 /= 2;
		if(h2 > sizelimit || (h - h2)/2 < (h2 - h)/2) h2 /= 2;
	}
	while(w2 > sizelimit || h2 > sizelimit)
	{
		w2 /= 2;
		h2 /= 2;
	}
	w = w2;
	h = h2;
}

static Texture *newtexture(const char *rname, SDL_Surface *s, int clamp = 0, bool mipit = true, bool canreduce = false)
{
	char *key = newstring(rname);
	Texture *t = &textures[key];
	t->name = key;
	t->bpp = s->format->BitsPerPixel;
	int w = t->xs = s->w;
	int h = t->ys = s->h;
	glGenTextures(1, &t->gl);
	if(canreduce) loopi(texreduce)
	{
		if(w > 1) w /= 2;
		if(h > 1) h /= 2;
	}
	GLenum format = texformat(t->bpp);
	resizetexture(w, h, mipit, format);
    uchar *pixels = (uchar *)s->pixels;
	if(w != t->xs || h != t->ys)
    {
        if(w*h > t->xs*t->ys) pixels = new uchar[formatsize(format)*w*h];
        gluScaleImage(format, t->xs, t->ys, GL_UNSIGNED_BYTE, s->pixels, w, h, GL_UNSIGNED_BYTE, pixels);
    }
    createtexture(t->gl, w, h, pixels, clamp, mipit, format);
    if(pixels!=s->pixels) delete[] pixels;
	SDL_FreeSurface(s);
	return t;
}

static vec parsevec(const char *arg)
{
	vec v(0, 0, 0);
	int i = 0;
	for(; arg[0] && arg[0]!='>' && i<3; arg += strcspn(i ? arg+1 : arg, "/>"), i++)
		v[i] = atof(i ? arg+1 : arg);
	if(i==1) v.y = v.z = v.x;
	return v;
}

static SDL_Surface *texturedata(const char *tname, Slot::Tex *tex = NULL, bool msg = true)
{
	if(tex && !tname)
	{
		static string pname;
		s_sprintf(pname)("%s", tex->name);
		tname = pname;
	}
	if(!tname) return NULL;

	const char *file = tname;
	if(tname[0]=='<')
	{
		file = strchr(tname, '>');
		if(!file) { if(msg) conoutf("could not load texture %s", tname); return NULL; }
		file++;
	}

	if(msg) show_out_of_renderloop_progress(0, file);

	SDL_Surface *s = IMG_Load(findfile(file, "rb"));
	if(!s) { if(msg) conoutf("could not load texture %s", tname); return NULL; }
	int bpp = s->format->BitsPerPixel;
	if(!texformat(bpp)) { SDL_FreeSurface(s); conoutf("texture must be 8, 24, or 32 bpp: %s", tname); return NULL; }
	if(tex)
	{
		if(tex->rotation) s = texrotate(s, tex->rotation, tex->type);
		if(tex->xoffset || tex->yoffset) s = texoffset(s, tex->xoffset, tex->yoffset);
	}
	if(tname[0]=='<')
	{
		const char *cmd = &tname[1], *arg1 = strchr(cmd, ':'), *arg2 = arg1 ? strchr(arg1, ',') : NULL;
		if(!arg1) arg1 = strchr(cmd, '>');
		if(!strncmp(cmd, "mad", arg1-cmd)) texmad(s, parsevec(arg1+1), arg2 ? parsevec(arg2+1) : vec(0, 0, 0));
		else if(!strncmp(cmd, "ffmask", arg1-cmd)) s = texffmask(s, atoi(arg1+1));
        else if(!strncmp(cmd, "dup", arg1-cmd)) texdup(s, atoi(arg1+1), atoi(arg2+1));
	}
	return s;
}

void loadalphamask(Texture *t)
{
	if(t->alphamask || t->bpp!=32) return;
	SDL_Surface *s = IMG_Load(findfile(t->name, "rb"));
	if(!s || !s->format->Amask) { if(s) SDL_FreeSurface(s); return; }
	uint alpha = s->format->Amask;
	t->alphamask = new uchar[s->h * ((s->w+7)/8)];
	uchar *srcrow = (uchar *)s->pixels, *dst = t->alphamask-1;
	loop(y, s->h)
	{
		uint *src = (uint *)srcrow;
		loop(x, s->w)
		{
			int offset = x%8;
			if(!offset) *++dst = 0;
			if(*src & alpha) *dst |= 1<<offset;
			src++;
		}
		srcrow += s->pitch;
	}
	SDL_FreeSurface(s);
}

Texture *textureload(const char *name, int clamp, bool mipit, bool msg)
{
	string tname;
	s_strcpy(tname, name);
	Texture *t = textures.access(tname);
	if(t) return t;
	SDL_Surface *s = texturedata(tname, NULL, msg);
    return s ? newtexture(tname, s, clamp, mipit) : notexture;
}

void cleangl()
{
	enumerate(textures, Texture, t, { delete[] t.name; if(t.alphamask) delete[] t.alphamask; });
	textures.clear();
}

void settexture(const char *name, bool clamp)
{
    glBindTexture(GL_TEXTURE_2D, textureload(name, clamp, true, false)->gl);
}

vector<Slot> slots;
Slot materialslots[MAT_EDIT];

int curtexnum = 0, curmatslot = -1;

void texturereset()
{
	curtexnum = 0;
	slots.setsize(0);
}

COMMAND(texturereset, "");

void materialreset()
{
	loopi(MAT_EDIT) materialslots[i].reset();
}

COMMAND(materialreset, "");
sometype textypes[] =
{
	{"c", TEX_DIFFUSE},
	{"u", TEX_UNKNOWN},
	{"d", TEX_DECAL},
	{"n", TEX_NORMAL},
	{"g", TEX_GLOW},
	{"s", TEX_SPEC},
	{"z", TEX_DEPTH},
	{"e", TEX_ENVMAP}
};

int findtexturetype(char *name, bool tryint)
{
	loopi(sizeof(textypes)/sizeof(textypes[0])) if(!strcmp(textypes[i].name, name)) { return textypes[i].id; }
	return tryint && *name >= '0' && *name <= '9' ? atoi(name) : -1;
}

char *findtexturename(int type)
{
	loopi(sizeof(textypes)/sizeof(textypes[0])) if(textypes[i].id == type) { return textypes[i].name; }
	return NULL;
}

void texture(char *type, char *name, int *rot, int *xoffset, int *yoffset, float *scale)
{
	if(curtexnum<0 || curtexnum>=0x10000) return;
	int tnum = findtexturetype(type, true), matslot = findmaterial(type, false);
	if (tnum < 0) tnum = 0;
	if (tnum == TEX_DIFFUSE)
	{
		if(matslot>=0) curmatslot = matslot;
		else { curmatslot = -1; curtexnum++; }
	}
	else if(curmatslot>=0) matslot=curmatslot;
	else if(!curtexnum) return;
	Slot &s = matslot>=0 ? materialslots[matslot] : (tnum!=TEX_DIFFUSE ? slots.last() : slots.add());
	if(tnum==TEX_DIFFUSE) setslotshader(s);
	s.loaded = false;
	if (s.sts.length() > TEX_ENVMAP)
		conoutf("warning: too many textures, slot %d file '%s' (%d,%d)", curtexnum, name, matslot, curmatslot);
	Slot::Tex &st = s.sts.add();
	st.type = tnum;
	st.combined = -1;
	st.rotation = max(*rot, 0);
	st.xoffset = max(*xoffset, 0);
	st.yoffset = max(*yoffset, 0);
	st.scale = max(*scale, 0);
	st.t = NULL;
	s_strcpy(st.lname, name);
	s_strcpy(st.name, name);
}

COMMAND(texture, "ssiiif");
void texturedel(int i, bool local)
{
	if (curtexnum > 8)
	{
		if (i >= 0 && i <= curtexnum)
		{
			extern selinfo sel;
			loopj(curtexnum-i)
			{
				int oldtex = i+j, newtex = max(i+j-1, 0);
				if(local) cl->edittrigger(sel, EDIT_REPLACE, oldtex, newtex);
				loopk(8) replacetexcube(worldroot[k], oldtex, newtex);
			}
			slots[i].reset();
			slots.remove(i);

			curtexnum--;
			allchanged();
		}
		else if (local) conoutf("texture to delete must be in range 0..%d", curtexnum);
	}
	else if (local) conoutf("not enough texture slots, please add another one first");
}

ICOMMAND(texturedel, "i", (int *i), {
	texturedel(*i, true);
});

void autograss(char *name)
{
	Slot &s = slots.last();
	DELETEA(s.autograss);
	s_sprintfd(pname)("%s", name);
	s.autograss = newstring(name[0] ? pname : "textures/grass.png");
}

COMMAND(autograss, "s");

static int findtextype(Slot &s, int type, int last = -1)
{
	for(int i = last+1; i<s.sts.length(); i++) if((type&(1<<s.sts[i].type)) && s.sts[i].combined<0) return i;
	return -1;
}

#define writetex(t, body) \
	{ \
		uchar *dst = (uchar *)t->pixels; \
		loop(y, t->h) loop(x, t->w) \
		{ \
			body; \
			dst += t->format->BytesPerPixel; \
		} \
	}

#define sourcetex(s) uchar *src = &((uchar *)s->pixels)[s->format->BytesPerPixel*(y*s->w + x)];

static void scaleglow(SDL_Surface *g, Slot &s)
{
	ShaderParam *cparam = findshaderparam(s, "glowscale", SHPARAM_PIXEL, 0);
	if(!cparam) cparam = findshaderparam(s, "glowscale", SHPARAM_VERTEX, 0);
	float color[3] = {1, 1, 1};
	if(cparam) memcpy(color, cparam->val, sizeof(color));
	writetex(g,
		loopk(3) dst[k] = min(255, int(dst[k] * color[k]));
	);
}

static void addglow(SDL_Surface *c, SDL_Surface *g, Slot &s)
{
	ShaderParam *cparam = findshaderparam(s, "glowscale", SHPARAM_PIXEL, 0);
	if(!cparam) cparam = findshaderparam(s, "glowscale", SHPARAM_VERTEX, 0);
	float color[3] = {1, 1, 1};
	if(cparam) memcpy(color, cparam->val, sizeof(color));
	writetex(c,
		sourcetex(g);
		loopk(3) dst[k] = min(255, int(dst[k]) + int(src[k] * color[k]));
	);
}

static void addbump(SDL_Surface *c, SDL_Surface *n)
{
	writetex(c,
		sourcetex(n);
		loopk(3) dst[k] = int(dst[k])*(int(src[2])*2-255)/255;
	);
}

static void blenddecal(SDL_Surface *c, SDL_Surface *d)
{
	writetex(c,
		sourcetex(d);
		uchar a = src[3];
		loopk(3) dst[k] = (int(src[k])*int(a) + int(dst[k])*int(255-a))/255;
	);
}

static void mergespec(SDL_Surface *c, SDL_Surface *s)
{
	writetex(c,
		sourcetex(s);
		dst[3] = (int(src[0]) + int(src[1]) + int(src[2]))/3;
	);
}

static void mergedepth(SDL_Surface *c, SDL_Surface *z)
{
	writetex(c,
		sourcetex(z);
		dst[3] = src[0];
	);
}

static void addname(vector<char> &key, Slot &slot, Slot::Tex &t)
{
	if(t.combined>=0) key.add('&');
	s_sprintfd(tname)("%s", t.name);
	for(const char *s = tname; *s; key.add(*s++));
	if(t.rotation)
	{
		s_sprintfd(rnum)("#%d", t.rotation);
		for(const char *s = rnum; *s; key.add(*s++));
	}
	if(t.xoffset || t.yoffset)
	{
		s_sprintfd(toffset)("+%d,%d", t.xoffset, t.yoffset);
		for(const char *s = toffset; *s; key.add(*s++));
	}
	if(t.combined>=0 || renderpath==R_FIXEDFUNCTION) switch(t.type)
	{
		case TEX_GLOW:
		{
			ShaderParam *cparam = findshaderparam(slot, "glowscale", SHPARAM_PIXEL, 0);
			if(!cparam) cparam = findshaderparam(slot, "glowscale", SHPARAM_VERTEX, 0);
			s_sprintfd(suffix)("?%.2f,%.2f,%.2f", cparam ? cparam->val[0] : 1.0f, cparam ? cparam->val[1] : 1.0f, cparam ? cparam->val[2] : 1.0f);
			for(const char *s = suffix; *s; key.add(*s++));
			break;
		}
	}
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RMASK 0xff000000
#define GMASK 0x00ff0000
#define BMASK 0x0000ff00
#define AMASK 0x000000ff
#else
#define RMASK 0x000000ff
#define GMASK 0x0000ff00
#define BMASK 0x00ff0000
#define AMASK 0xff000000
#endif

SDL_Surface *creatergbasurface(SDL_Surface *os)
{
	SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, os->w, os->h, 32, RMASK, GMASK, BMASK, AMASK);
	if(!ns) fatal("creatergbsurface");
	SDL_BlitSurface(os, NULL, ns, NULL);
	SDL_FreeSurface(os);
	return ns;
}

SDL_Surface *scalesurface(SDL_Surface *os, int w, int h)
{
	SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, os->format->BitsPerPixel, os->format->Rmask, os->format->Gmask, os->format->Bmask, os->format->Amask);
	if(!ns) fatal("scalesurface");
	gluScaleImage(texformat(os->format->BitsPerPixel), os->w, os->h, GL_UNSIGNED_BYTE, os->pixels, w, h, GL_UNSIGNED_BYTE, ns->pixels);
	SDL_FreeSurface(os);
	return ns;
}

static void texcombine(Slot &s, int index, Slot::Tex &t, bool forceload = false)
{
    if(renderpath==R_FIXEDFUNCTION && t.type!=TEX_DIFFUSE && t.type!=TEX_GLOW && !forceload) { t.t = notexture; return; }
	vector<char> key;
	addname(key, s, t);
	switch(t.type)
	{
		case TEX_DIFFUSE:
			if(renderpath==R_FIXEDFUNCTION)
			{
                for(int i = -1; (i = findtextype(s, (1<<TEX_DECAL)|(1<<TEX_NORMAL), i))>=0;)
				{
					s.sts[i].combined = index;
					addname(key, s, s.sts[i]);
				}
				break;
			} // fall through to shader case

		case TEX_NORMAL:
		{
			if(renderpath==R_FIXEDFUNCTION) break;
			int i = findtextype(s, t.type==TEX_DIFFUSE ? (1<<TEX_SPEC) : (1<<TEX_DEPTH));
			if(i<0) break;
			s.sts[i].combined = index;
			addname(key, s, s.sts[i]);
			break;
		}
	}
	key.add('\0');
	t.t = textures.access(key.getbuf());
	if(t.t) return;
	SDL_Surface *ts = texturedata(NULL, &t);
    if(!ts) { t.t = notexture; return; }
	switch(t.type)
	{
		case TEX_GLOW:
			if(renderpath==R_FIXEDFUNCTION) scaleglow(ts, s);
			break;

		case TEX_DIFFUSE:
			if(renderpath==R_FIXEDFUNCTION)
			{
				loopv(s.sts)
				{
					Slot::Tex &b = s.sts[i];
					if(b.combined!=index) continue;
					SDL_Surface *bs = texturedata(NULL, &b);
					if(!bs) continue;
					if(bs->w!=ts->w || bs->h!=ts->h) bs = scalesurface(bs, ts->w, ts->h);
					switch(b.type)
					{
						case TEX_DECAL: if(bs->format->BitsPerPixel==32) blenddecal(ts, bs); break;
						case TEX_GLOW: addglow(ts, bs, s); break;
						case TEX_NORMAL: addbump(ts, bs); break;
					}
					SDL_FreeSurface(bs);
				}
				break;
			} // fall through to shader case

		case TEX_NORMAL:
			loopv(s.sts)
			{
				Slot::Tex &a = s.sts[i];
				if(a.combined!=index) continue;
				SDL_Surface *as = texturedata(NULL, &a);
				if(!as) break;
				if(ts->format->BitsPerPixel!=32) ts = creatergbasurface(ts);
				if(as->w!=ts->w || as->h!=ts->h) as = scalesurface(as, ts->w, ts->h);
				switch(a.type)
				{
					case TEX_SPEC: mergespec(ts, as); break;
					case TEX_DEPTH: mergedepth(ts, as); break;
				}
				SDL_FreeSurface(as);
				break; // only one combination
			}
			break;
	}
	t.t = newtexture(key.getbuf(), ts, 0, true, true);
}

Slot &lookuptexture(int slot, bool load)
{
	static Slot dummyslot;
	Slot &s = slot<0 && slot>-MAT_EDIT ? materialslots[-slot] : (slots.inrange(slot) ? slots[slot] : (slots.empty() ? dummyslot : slots[0]));
	if(s.loaded || !load) return s;
	loopv(s.sts)
	{
		Slot::Tex &t = s.sts[i];
		if(t.combined>=0) continue;
		switch(t.type)
		{
			case TEX_ENVMAP:
				if(hasCM && (renderpath!=R_FIXEDFUNCTION || (slot<0 && slot>-MAT_EDIT))) t.t = cubemapload(t.name);
				break;

			default:
				texcombine(s, i, t, slot<0 && slot>-MAT_EDIT);
				break;
		}
	}
	s.loaded = true;
	return s;
}

Shader *lookupshader(int slot) { return slot<0 && slot>-MAT_EDIT ? materialslots[-slot].shader : (slots.inrange(slot) ? slots[slot].shader : defaultshader); }

Texture *loadthumbnail(Slot &slot)
{
	if(slot.thumbnail) return slot.thumbnail;
	vector<char> name;
	for(const char *s = "<thumbnail>"; *s; name.add(*s++));
	addname(name, slot, slot.sts[0]);
	name.add('\0');
	Texture *t = textures.access(name.getbuf());
	if(t) slot.thumbnail = t;
	else
	{
		SDL_Surface *s = texturedata(NULL, &slot.sts[0], false);
        if(!s) slot.thumbnail = notexture;
		else
		{
			int xs = s->w, ys = s->h;
			if(s->w > 64 || s->h > 64) s = scalesurface(s, min(s->w, 64), min(s->h, 64));
			t = newtexture(name.getbuf(), s, false, false);
			t->xs = xs;
			t->ys = ys;
		}
	}
	return t;
}

// environment mapped reflections

cubemapside cubemapsides[6] =
{
	{ GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, "ft" },
	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, "bk" },
	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, "lf" },
	{ GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, "rt" },
	{ GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, "dn" },
	{ GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, "up" },
};

GLuint cubemapfromsky(int size)
{
	extern Texture *sky[6];
	if(!sky[0]) return 0;

    int tsize = 0, cmw, cmh;
    GLint tw[6], th[6];
    loopi(6)
    {
        glBindTexture(GL_TEXTURE_2D, sky[i]->gl);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw[i]);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th[i]);
        tsize = max(tsize, max(tw[i], th[i]));
    }
    cmw = cmh = min(tsize, size);
    resizetexture(cmw, cmh, true, GL_RGB5, GL_TEXTURE_CUBE_MAP_ARB);

	GLuint tex;
	glGenTextures(1, &tex);
    uchar *pixels = new uchar[3*max(cmw, tsize)*max(cmh, tsize)];
	loopi(6)
	{
		glBindTexture(GL_TEXTURE_2D, sky[i]->gl);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        if(tw[i]!=cmw || th[i]!=cmh) gluScaleImage(GL_RGB, tw[i], th[i], GL_UNSIGNED_BYTE, pixels, cmw, cmh, GL_UNSIGNED_BYTE, pixels);
        createtexture(!i ? tex : 0, cmw, cmh, pixels, 3, true, GL_RGB5, cubemapsides[i].target);
	}
	delete[] pixels;
	return tex;
}

Texture *cubemaploadwildcard(const char *name, bool mipit, bool msg)
{
	if(!hasCM) return NULL;
	string tname;
	s_strcpy(tname, name);
	Texture *t = textures.access(tname);
	if(t) return t;
	char *wildcard = strchr(tname, '*');
	SDL_Surface *surface[6];
	string sname;
	if(!wildcard) s_strcpy(sname, tname);
    GLenum format = 0;
    int tsize = 0;
	loopi(6)
	{
		if(wildcard)
		{
			s_strncpy(sname, tname, wildcard-tname+1);
			s_strcat(sname, cubemapsides[i].name);
			s_strcat(sname, wildcard+1);
		}
		surface[i] = texturedata(sname, NULL, msg);
        if(!surface[i])
		{
			loopj(i) SDL_FreeSurface(surface[j]);
			return NULL;
		}
        if(!format) format = texformat(surface[i]->format->BitsPerPixel);
        else if(texformat(surface[i]->format->BitsPerPixel)!=format)
        {
            if(surface[i] && msg) conoutf("cubemap texture %s doesn't match other sides' format", sname);
            loopj(i) SDL_FreeSurface(surface[j]);
            return NULL;
        }
        tsize = max(tsize, max(surface[i]->w, surface[i]->h));
	}
	char *key = newstring(tname);
	t = &textures[key];
	t->name = key;
	t->bpp = surface[0]->format->BitsPerPixel;
    int w = t->xs = tsize;
    int h = t->ys = tsize;
    resizetexture(w, h, mipit, format, GL_TEXTURE_CUBE_MAP_ARB);
	glGenTextures(1, &t->gl);
    uchar *pixels = NULL;
	loopi(6)
	{
		SDL_Surface *s = surface[i];
		if(s->w != w || s->h != h)
        {
            if(!pixels) pixels = new uchar[formatsize(format)*w*h];
            gluScaleImage(format, s->w, s->h, GL_UNSIGNED_BYTE, s->pixels, w, h, GL_UNSIGNED_BYTE, pixels);
        }
        createtexture(!i ? t->gl : 0, w, h, s->w != w || s->h != h ? pixels : s->pixels, 3, mipit, format, cubemapsides[i].target);
		SDL_FreeSurface(s);
	}
    if(pixels) delete[] pixels;
	return t;
}

Texture *cubemapload(const char *name, bool mipit, bool msg)
{
	if(!hasCM) return NULL;
	s_sprintfd(pname)("%s", name);
	Texture *t = NULL;
	if(!strchr(pname, '*'))
	{
		s_sprintfd(jpgname)("%s_*.jpg", pname);
		t = cubemaploadwildcard(jpgname, mipit, false);
		if(!t)
		{
			s_sprintfd(pngname)("%s_*.png", pname);
			t = cubemaploadwildcard(pngname, mipit, false);
			if(!t && msg) conoutf("could not load envmap %s", name);
		}
	}
	else t = cubemaploadwildcard(pname, mipit, msg);
	return t;
}

VARFP(envmapsize, 4, 7, 9, setupmaterials());
VARW(envmapradius, 0, 128, 10000);

struct envmap
{
	int radius, size;
	vec o;
	GLuint tex;
};

static vector<envmap> envmaps;
static GLuint skyenvmap = 0;

void clearenvmaps()
{
	if(skyenvmap)
	{
		glDeleteTextures(1, &skyenvmap);
		skyenvmap = 0;
	}
	loopv(envmaps) glDeleteTextures(1, &envmaps[i].tex);
	envmaps.setsize(0);
}

VAR(aaenvmap, 0, 2, 4);

GLuint genenvmap(const vec &o, int envmapsize)
{
    int rendersize = 1<<(envmapsize+aaenvmap), sizelimit = min(hwcubetexsize, min(screen->w, screen->h));
	if(maxtexsize) sizelimit = min(sizelimit, maxtexsize);
    while(rendersize > sizelimit) rendersize /= 2;
	int texsize = min(rendersize, 1<<envmapsize);
	if(!aaenvmap) rendersize = texsize;
    GLuint tex;
	glGenTextures(1, &tex);
	glViewport(0, 0, rendersize, rendersize);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	float yaw = 0, pitch = 0;
    uchar *pixels = new uchar[3*rendersize*rendersize];
	loopi(6)
	{
		const cubemapside &side = cubemapsides[i];
		switch(side.target)
		{
			case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB: // ft
				yaw = 0; pitch = 0; break;
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB: // bk
				yaw = 180; pitch = 0; break;
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB: // lf
				yaw = 270; pitch = 0; break;
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB: // rt
				yaw = 90; pitch = 0; break;
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB: // dn
				yaw = 90; pitch = -90; break;
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB: // up
				yaw = 90; pitch = 90; break;
		}
		drawcubemap(rendersize, o, yaw, pitch);
		glReadPixels(0, 0, rendersize, rendersize, GL_RGB, GL_UNSIGNED_BYTE, pixels);
		if(texsize<rendersize) gluScaleImage(GL_RGB, rendersize, rendersize, GL_UNSIGNED_BYTE, pixels, texsize, texsize, GL_UNSIGNED_BYTE, pixels);
		createtexture(tex, texsize, texsize, pixels, 3, true, GL_RGB5, side.target);
	}
    delete[] pixels;
	glViewport(0, 0, screen->w, screen->h);
	return tex;
}

void initenvmaps()
{
	if(!hasCM) return;
	clearenvmaps();
	skyenvmap = cubemapfromsky(1<<envmapsize);
	const vector<extentity *> &ents = et->getents();
	loopv(ents)
	{
		const extentity &ent = *ents[i];
		if(ent.type != ET_ENVMAP) continue;
		envmap &em = envmaps.add();
		em.radius = ent.attr1 ? max(0, min(10000, ent.attr1)) : envmapradius;
		em.size = ent.attr2 ? max(4, min(9, ent.attr2)) : 0;
		em.o = ent.o;
		em.tex = 0;
	}
}

void genenvmaps()
{
	if(envmaps.empty()) return;
	show_out_of_renderloop_progress(0, "generating environment maps...");
	loopv(envmaps)
	{
		envmap &em = envmaps[i];
		show_out_of_renderloop_progress(float(i)/float(envmaps.length()), "generating environment maps...");
		em.tex = genenvmap(em.o, em.size ? em.size : envmapsize);
	}
}

ushort closestenvmap(const vec &o)
{
	ushort minemid = EMID_SKY;
	float mindist = 1e16f;
	loopv(envmaps)
	{
		envmap &em = envmaps[i];
		float dist = em.o.dist(o);
		if(dist < em.radius && dist < mindist)
		{
			minemid = EMID_RESERVED + i;
			mindist = dist;
		}
	}
	return minemid;
}

ushort closestenvmap(int orient, int x, int y, int z, int size)
{
	vec loc(x, y, z);
	int dim = dimension(orient);
	if(dimcoord(orient)) loc[dim] += size;
	loc[R[dim]] += size/2;
	loc[C[dim]] += size/2;
	return closestenvmap(loc);
}

GLuint lookupenvmap(ushort emid)
{
	if(emid==EMID_SKY || emid==EMID_CUSTOM) return skyenvmap;
	if(emid==EMID_NONE || !envmaps.inrange(emid-EMID_RESERVED)) return 0;
	GLuint tex = envmaps[emid-EMID_RESERVED].tex;
	return tex ? tex : skyenvmap;
}

void writetgaheader(FILE *f, SDL_Surface *s, int bits)
{
    fwrite("\0\0\x02\0\0\0\0\0\0\0\0\0", 1, 12, f);
    ushort dim[] = { s->w, s->h };
    endianswap(dim, sizeof(ushort), 2);
    fwrite(dim, sizeof(short), 2, f);
    fputc(bits, f);
    fputc(0, f);
}

void flipnormalmapy(char *destfile, char *normalfile)           // RGB (jpg/png) -> BGR (tga)
{
    SDL_Surface *ns = IMG_Load(findfile(normalfile, "rb"));
    if(!ns) return;
    FILE *f = openfile(destfile, "wb");
    if(f)
    {
        writetgaheader(f, ns, 24);
        for(int y = ns->h-1; y>=0; y--) loop(x, ns->w)
        {
            uchar *nd = (uchar *)ns->pixels+(x+y*ns->w)*3;
            fputc(nd[2], f);
            fputc(255-nd[1], f);
            fputc(nd[0], f);
        }
        fclose(f);
    }
    if(ns) SDL_FreeSurface(ns);
}

void mergenormalmaps(char *heightfile, char *normalfile)    // BGR (tga) -> BGR (tga) (SDL loads TGA as BGR!)
{
    SDL_Surface *hs = IMG_Load(findfile(heightfile, "rb"));
    SDL_Surface *ns = IMG_Load(findfile(normalfile, "rb"));
    if(hs && ns)
    {
        uchar def_n[] = { 255, 128, 128 };
        FILE *f = openfile(normalfile, "wb");
        if(f)
        {
            writetgaheader(f, ns, 24);
            for(int y = ns->h-1; y>=0; y--) loop(x, ns->w)
            {
                int off = (x+y*ns->w)*3;
                uchar *hd = hs ? (uchar *)hs->pixels+off : def_n;
                uchar *nd = ns ? (uchar *)ns->pixels+off : def_n;
                #define S(x) x/255.0f*2-1
                vec n(S(nd[0]), S(nd[1]), S(nd[2]));
                vec h(S(hd[0]), S(hd[1]), S(hd[2]));
                n.mul(2).add(h).normalize().add(1).div(2).mul(255);
                uchar o[3] = { (uchar)n.x, (uchar)n.y, (uchar)n.z };
                fwrite(o, 3, 1, f);
                #undef S
            }
            fclose(f);
        }
    }
    if(hs) SDL_FreeSurface(hs);
    if(ns) SDL_FreeSurface(ns);
}

COMMAND(flipnormalmapy, "ss");
COMMAND(mergenormalmaps, "sss");

