// texture.cpp: texture slot management

#include "pch.h"
#include "engine.h"

#define FUNCNAME(name) name##1
#define DEFPIXEL uint OP(r, 0);
#define PIXELOP OP(r, 0);
#define BPP 1
#include "scale.h"

#define FUNCNAME(name) name##2
#define DEFPIXEL uint OP(r, 0), OP(g, 1);
#define PIXELOP OP(r, 0); OP(g, 1);
#define BPP 2
#include "scale.h"

#define FUNCNAME(name) name##3
#define DEFPIXEL uint OP(r, 0), OP(g, 1), OP(b, 2);
#define PIXELOP OP(r, 0); OP(g, 1); OP(b, 2);
#define BPP 3
#include "scale.h"

#define FUNCNAME(name) name##4
#define DEFPIXEL uint OP(r, 0), OP(g, 1), OP(b, 2), OP(a, 3);
#define PIXELOP OP(r, 0); OP(g, 1); OP(b, 2); OP(a, 3);
#define BPP 4
#include "scale.h"

static void scaletexture(uchar *src, uint sw, uint sh, uint bpp, uchar *dst, uint dw, uint dh)
{
    if(sw == dw*2 && sh == dh*2)
    {
        switch(bpp)
        {
            case 1: return halvetexture1(src, sw, sh, dst);
            case 2: return halvetexture2(src, sw, sh, dst);
            case 3: return halvetexture3(src, sw, sh, dst);
            case 4: return halvetexture4(src, sw, sh, dst);
        }
    }
    else if(sw < dw || sh < dh || sw&(sw-1) || sh&(sh-1))
    {
        switch(bpp)
        {
            case 1: return scaletexture1(src, sw, sh, dst, dw, dh);
            case 2: return scaletexture2(src, sw, sh, dst, dw, dh);
            case 3: return scaletexture3(src, sw, sh, dst, dw, dh);
            case 4: return scaletexture4(src, sw, sh, dst, dw, dh);
        }
    }
    else
    {
        switch(bpp)
        {
            case 1: return shifttexture1(src, sw, sh, dst, dw, dh);
            case 2: return shifttexture2(src, sw, sh, dst, dw, dh);
            case 3: return shifttexture3(src, sw, sh, dst, dw, dh);
            case 4: return shifttexture4(src, sw, sh, dst, dw, dh);
        }
    }
}

static inline void reorienttexture(uchar *src, int sw, int sh, int bpp, uchar *dst, bool flipx, bool flipy, bool swapxy, bool normals = false)
{
    int stridex = bpp, stridey = bpp;
    if(swapxy) stridex *= sh; else stridey *= sw;
    if(flipx) { dst += (sw-1)*stridex; stridex = -stridex; }
    if(flipy) { dst += (sh-1)*stridey; stridey = -stridey; }
    loopi(sh)
    {
        uchar *curdst = dst;
        loopj(sw)
        {
            loopk(bpp) curdst[k] = *src++;
            if(normals)
            {
                if(flipx) curdst[0] = 255-curdst[0];
                if(flipy) curdst[1] = 255-curdst[1];
                if(swapxy) swap(curdst[0], curdst[1]);
            }
            curdst += stridex;
        }
        dst += stridey;
    }
}

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RGBAMASKS 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#define RGBMASKS  0xff0000, 0x00ff00, 0x0000ff, 0
#else
#define RGBAMASKS 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#define RGBMASKS  0x0000ff, 0x00ff00, 0xff0000, 0
#endif

SDL_Surface *texreorient(SDL_Surface *s, bool flipx, bool flipy, bool swapxy, int type, bool clear)
{
    SDL_Surface *d = SDL_CreateRGBSurface(SDL_SWSURFACE, swapxy ? s->h : s->w, swapxy ? s->w : s->h, s->format->BitsPerPixel, s->format->Rmask, s->format->Gmask, s->format->Bmask, s->format->Amask);
    if(!d) fatal("create surface");
    reorienttexture((uchar *)s->pixels, s->w, s->h, s->format->BytesPerPixel, (uchar *)d->pixels, flipx, flipy, swapxy, type==TEX_NORMAL);
    if(clear) SDL_FreeSurface(s);
    return d;
}

SDL_Surface *texrotate(SDL_Surface *s, int numrots, int type)
{
    // 1..3 rotate through 90..270 degrees, 4 flips X, 5 flips Y
    if(numrots<1 || numrots>5) return s;
    return texreorient(s,
        numrots>=2 && numrots<=4, // flip X on 180/270 degrees
        numrots<=2 || numrots==5, // flip Y on 90/180 degrees
        (numrots&5)==1,           // swap X/Y on 90/270 degrees
        type);
}

SDL_Surface *texoffset(SDL_Surface *s, int xoffset, int yoffset, bool clear)
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
	if(clear) SDL_FreeSurface(s);
	return d;
}

SDL_Surface *texcrop(SDL_Surface *s, int x, int y, int w, int h, bool clear)
{
	SDL_Surface *d = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, s->format->BitsPerPixel, s->format->Rmask, s->format->Gmask, s->format->Bmask, s->format->Amask);
	if(!d) fatal("create surface");
	loopi(h)
	{
		uchar *dst = (uchar *)d->pixels+(i*d->pitch),
			*src = (uchar *)s->pixels+(((i+y)*s->pitch)+(x*s->format->BytesPerPixel));
		memcpy(dst, src, w*s->format->BytesPerPixel);
	}
	if(clear) SDL_FreeSurface(s);
	return d;
}

SDL_Surface *texcopy(SDL_Surface *s, bool clear)
{
	SDL_Surface *d = SDL_CreateRGBSurface(SDL_SWSURFACE, s->w, s->h, s->format->BitsPerPixel, s->format->Rmask, s->format->Gmask, s->format->Bmask, s->format->Amask);
	if(!d) fatal("create surface");
	loopi(s->h)
	{
		uchar *dst = (uchar *)d->pixels+(i*d->pitch),
			*src = (uchar *)s->pixels+(i*s->pitch);
		memcpy(dst, src, s->w*s->format->BytesPerPixel);
	}
	if(clear) SDL_FreeSurface(s);
	return d;
}

SDL_Surface *texmad(SDL_Surface *s, const vec &mul, const vec &add)
{
    if(s->format->BytesPerPixel<3 && (mul.x!=mul.y || mul.x!=mul.z || add.x!=add.y || add.x!=add.z))
        s = creatergbsurface(s);

    int maxk = min(int(s->format->BytesPerPixel), 3);
	uchar *src = (uchar *)s->pixels;
	loopi(s->h*s->w)
	{
		loopk(maxk)
		{
			float val = src[k]*mul[k] + 255*add[k];
            src[k] = uchar(min(max(val, 0.0f), 255.0f));
		}
		src += s->format->BytesPerPixel;
	}
    return s;
}

SDL_Surface *texmix(SDL_Surface *s, int c1, int c2, int c3, int c4)
{
    int numchans = c1 < 0 ? 0 : (c2 < 0 ? 1 : (c3 < 0 ? 2 : (c4 < 0 ? 3 : 4)));
    if(numchans <= 0) return s;
    SDL_Surface *m = SDL_CreateRGBSurface(SDL_SWSURFACE, s->w, s->h, 8*numchans, 0, 0, 0, 0);
    if(!m) fatal("create surface");
    uchar *dst = (uchar *)m->pixels, *src = (uchar *)s->pixels;
    loopi(s->h*s->w)
    {
        switch(numchans)
        {
            case 4: dst[3] = src[c4];
            case 3: dst[2] = src[c3];
            case 2: dst[1] = src[c2];
            case 1: dst[0] = src[c1];
        } 
        src += s->format->BytesPerPixel;
        dst += m->format->BytesPerPixel;
    }
    SDL_FreeSurface(s);
    return m;
}

static SDL_Surface stubsurface;

SDL_Surface *texffmask(SDL_Surface *s, int minval, bool clear)
{
    if(renderpath!=R_FIXEDFUNCTION) return s;
    if(nomasks || s->format->BytesPerPixel<3) { SDL_FreeSurface(s); return &stubsurface; }
    bool glow = false, envmap = true;
    uchar *src = (uchar *)s->pixels;
    loopi(s->h*s->w)
    {
        if(src[1]>minval) glow = true;
        if(src[2]>minval) { glow = envmap = true; break; }
        src += s->format->BytesPerPixel;
    }
    if(!glow && !envmap) { SDL_FreeSurface(s); return &stubsurface; }
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
    if(clear) SDL_FreeSurface(s);
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

SDL_Surface *texdecal(SDL_Surface *s, bool clear)
{
    if(renderpath!=R_FIXEDFUNCTION || hasTE) return s;
    SDL_Surface *m = SDL_CreateRGBSurface(SDL_SWSURFACE, s->w, s->h, 16, 0, 0, 0, 0);
    if(!m) fatal("create surface");
    uchar *dst = (uchar *)m->pixels, *src = (uchar *)s->pixels;
    loopi(s->h*s->w)
    {
        *dst++ = *src;
        *dst++ = 255 - *src;
        src += s->format->BytesPerPixel;
    }
    if(clear) SDL_FreeSurface(s);
    return m;
}

VAR(hwtexsize, 1, 0, 0);
VAR(hwcubetexsize, 1, 0, 0);
VAR(hwmaxaniso, 1, 0, 0);
VARFP(maxtexsize, 0, 0, 1<<12, initwarning("texture quality", INIT_LOAD));
VARFP(texreduce, 0, 0, 12, initwarning("texture quality", INIT_LOAD));
VARFP(texcompress, 0, 1<<10, 1<<12, initwarning("texture quality", INIT_LOAD));
VARFP(texcompressquality, -1, -1, 1, setuptexcompress());
VARFP(trilinear, 0, 1, 1, initwarning("texture filtering", INIT_LOAD));
VARFP(bilinear, 0, 1, 1, initwarning("texture filtering", INIT_LOAD));
VARFP(aniso, 0, 0, 16, initwarning("texture filtering", INIT_LOAD));

void setuptexcompress()
{
    if(!hasTC) return;

    GLenum hint = GL_DONT_CARE;
    switch(texcompressquality)
    {
        case 1: hint = GL_NICEST; break;
        case 0: hint = GL_FASTEST; break;
    }
    glHint(GL_TEXTURE_COMPRESSION_HINT_ARB, hint);
}

GLenum compressedformat(GLenum format, int w, int h, bool force = false)
{
#ifdef __APPLE__
#undef GL_COMPRESSED_RGB_S3TC_DXT1_EXT
#undef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT GL_COMPRESSED_RGB_ARB
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT GL_COMPRESSED_RGBA_ARB
#endif
    if(hasTC && texcompress && (force || max(w, h) >= texcompress)) switch(format)
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

VARFP(hwmipmap, 0, 0, 1, initwarning("texture filtering", INIT_LOAD));

void resizetexture(int w, int h, bool mipmap, GLenum target, int &tw, int &th)
{
    int hwlimit = target==GL_TEXTURE_CUBE_MAP_ARB ? hwcubetexsize : hwtexsize,
        sizelimit = mipmap && maxtexsize ? min(maxtexsize, hwlimit) : hwlimit;
    w = min(w, sizelimit);
    h = min(h, sizelimit);
    if(mipmap || (!hasNP2 && target!=GL_TEXTURE_RECTANGLE_ARB && (w&(w-1) || h&(h-1))))
    {
        tw = th = 1;
        while(tw < w) tw *= 2;
        while(th < h) th *= 2;
        if(w < tw - tw/4) tw /= 2;
        if(h < th - th/4) th /= 2;
    }
    else
    {
        tw = w;
        th = h;
    }
}

void uploadtexture(GLenum target, GLenum internal, int tw, int th, GLenum format, GLenum type, void *pixels, int pw, int ph, bool mipmap)
{
    int bpp = formatsize(format);
    uchar *buf = NULL;
    if(pw!=tw || ph!=th)
    {
        buf = new uchar[tw*th*bpp];
        scaletexture((uchar *)pixels, pw, ph, bpp, buf, tw, th);
    }
    for(int level = 0;; level++)
    {
        uchar *src = buf ? buf : (uchar *)pixels;
        if(target==GL_TEXTURE_1D) glTexImage1D(target, level, internal, tw, 0, format, type, src);
        else glTexImage2D(target, level, internal, tw, th, 0, format, type, src);
        if(!mipmap || (hasGM && hwmipmap) || max(tw, th) <= 1) break;
        int srcw = tw, srch = th;
        if(tw > 1) tw /= 2;
        if(th > 1) th /= 2;
        if(!buf) buf = new uchar[tw*th*bpp];
        scaletexture(src, srcw, srch, bpp, buf, tw, th);
    }
    if(buf) delete[] buf;
}

void createtexture(int tnum, int w, int h, void *pixels, int clamp, bool mipit, GLenum component, GLenum subtarget, bool compress, bool filter, int pw, int ph)
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
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter && bilinear ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
			mipit ?
				(trilinear ?
					(bilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR) :
					(bilinear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST)) :
                (filter && bilinear ? GL_LINEAR : GL_NEAREST));
        if(hasGM && mipit && pixels)
            glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, hwmipmap ? GL_TRUE : GL_FALSE);
	}
	GLenum format = component, type = GL_UNSIGNED_BYTE;
	switch(component)
	{
        case GL_FLOAT_RG16_NV:
        case GL_FLOAT_R32_NV:
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

        case GL_DEPTH_COMPONENT16:
        case GL_DEPTH_COMPONENT24:
        case GL_DEPTH_COMPONENT32:
            format = GL_DEPTH_COMPONENT;
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
    if(!pw) pw = w;
    if(!ph) ph = h;
    int tw = w, th = h;
    if(pixels) resizetexture(w, h, mipit, target, tw, th);
    if(mipit && pixels)
    {
        GLenum compressed = compressedformat(component, tw, th, compress);
        uploadtexture(subtarget, compressed, tw, th, format, type, pixels, pw, ph, true);
    }
    else uploadtexture(subtarget, component, tw, th, format, type, pixels, pw, ph, false);
}

hashtable<char *, Texture> textures;

Texture *notexture = NULL, *blanktexture = NULL; // used as default, ensured to be loaded

void cleanuptexture(Texture *t)
{
	DELETEA(t->alphamask);

	loopvk(t->frames)
	{
		if(t->frames[k])
		{
			if(t->frames[k] == t->id) t->id = 0; // using a frame directly
			glDeleteTextures(1, &t->frames[k]);
			t->frames[k] = 0;
		}
	}
	t->frames.setsize(0);

	if(t->id)
	{
		glDeleteTextures(1, &t->id);
		t->id = 0;
	}
}

bool reloadtexture(const char *name)
{
	string tname;
	s_strcpy(tname, name);
    Texture *t = textures.access(tname);
    if(t) return reloadtexture(t);
    return false;
}

bool reloadtexture(Texture *t)
{
    switch(t->type)
    {
        case Texture::STUB:
        case Texture::IMAGE:
        {
            bool compress = false;
            TextureAnim anim;
            SDL_Surface *s = texturedata(t->name, NULL, true, &compress, &anim);
            if(!s || !newtexture(t, NULL, s, t->clamp, t->mipmap, false, false, compress, true, &anim)) return false;
            break;
        }

        case Texture::CUBEMAP:
            if(!cubemaploadwildcard(t, NULL, t->mipmap, true)) return false;
            break;
    }
    return true;
}

void reloadtextures()
{
    int reloaded = 0;
    enumerate(textures, Texture, tex,
    {
        loadprogress = float(++reloaded)/textures.numelems;
        reloadtexture(&tex);
    });
    loadprogress = 0;
}

void updatetexture(Texture *t)
{
	if(t->frames.length())
	{
		if(t->delay)
		{
			while(lastmillis-t->last >= t->delay)
			{
				t->frame++;
				if(t->frame >= t->frames.length()) t->frame = 0;
				t->last += t->delay;
			}

			if(t->frames.inrange(t->frame)) t->id = t->frames[t->frame];
			else t->id = t->frames[0];

			if(t->id <= 0)
				t->id = t != notexture ? notexture->id : 0;
		}
		else t->id = t->frames[0];
	}
	else t->id = 0;
}

void updatetextures()
{
    enumerate(textures, Texture, tex, updatetexture(&tex));
}

void preloadtextures()
{
    enumerate(*idents, ident, id, {
    	if(id.type == ID_SVAR && (id.flags & IDF_TEXTURE))
			id.changed();
	});
}

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

Texture *newtexture(Texture *t, const char *rname, SDL_Surface *s, int clamp, bool mipit, bool canreduce, bool transient, bool compress, bool clear, TextureAnim *anim)
{
    if(!t)
    {
        char *key = newstring(rname);
        t = &textures[key];
        t->name = key;
		t->frames.setsize(0);
		t->frame = 0;
    }

	t->clamp = clamp;
	t->mipmap = mipit;
	t->type = s==&stubsurface ? Texture::STUB : (transient ? Texture::TRANSIENT : Texture::IMAGE);

    if(t->type==Texture::STUB)
    {
        t->w = t->h = t->xs = t->ys = t->bpp = 0;
        return t;
    }

	bool hasanim = anim && anim->count;

	t->bpp = s->format->BitsPerPixel;
	t->delay = hasanim ? anim->delay : 0;

	t->w = t->xs = hasanim ? anim->w : s->w;
	t->h = t->ys = hasanim ? anim->h : s->h;
	if(canreduce) loopi(texreduce)
	{
		if(t->w > 1) t->w /= 2;
		if(t->h > 1) t->h /= 2;
	}

	GLenum format = texformat(t->bpp);

	loopi(hasanim ? anim->count : 1)
	{
		while(!t->frames.inrange(i)) t->frames.add(0);

		glGenTextures(1, &t->frames[i]);

		SDL_Surface *f = s;
		if(hasanim)
		{
			int sx = (i%anim->x)*anim->w, sy = (((i-(i%anim->x))/anim->x)%anim->y)*anim->h;
			f = texcrop(s, sx, sy, anim->w, anim->h, false);
		}

		createtexture(t->frames[i], t->w, t->h, (uchar *)f->pixels, clamp, mipit, format, GL_TEXTURE_2D, compress, true, f->w, f->h);

		if(verbose >= 3)
			conoutf("\fwadding frame: %s (%d) [%d,%d:%d,%d]", t->name, i+1, t->w, t->h, f->w, f->h);

		if(s != f) SDL_FreeSurface(f);
	}
    if(clear) SDL_FreeSurface(s);
	updatetexture(t);
    return t;
}

SDL_Surface *createsurface(int width, int height, int bpp)
{
    switch(bpp)
    {
        case 3: return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 8*bpp, RGBMASKS);
        case 4: return SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 8*bpp, RGBAMASKS);
    }
    return NULL;
}

SDL_Surface *wrapsurface(void *data, int width, int height, int bpp)
{
    switch(bpp)
    {
        case 3: return SDL_CreateRGBSurfaceFrom(data, width, height, 8*bpp, bpp*width, RGBMASKS);
        case 4: return SDL_CreateRGBSurfaceFrom(data, width, height, 8*bpp, bpp*width, RGBAMASKS);
    }
    return NULL;
}

SDL_Surface *creatergbsurface(SDL_Surface *os, bool clear)
{
    SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, os->w, os->h, 24, RGBMASKS);
    if(!ns) fatal("creatergbsurface");
    SDL_BlitSurface(os, NULL, ns, NULL);
    if(clear) SDL_FreeSurface(os);
    return ns;
}

SDL_Surface *creatergbasurface(SDL_Surface *os, bool clear)
{
    SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, os->w, os->h, 32, RGBAMASKS);
    if(!ns) fatal("creatergbasurface");
    SDL_BlitSurface(os, NULL, ns, NULL);
    if(clear) SDL_FreeSurface(os);
    return ns;
}

SDL_Surface *fixsurfaceformat(SDL_Surface *s)
{
    static const uint rgbmasks[] = { RGBMASKS }, rgbamasks[] = { RGBAMASKS };
    if(s) switch(s->format->BytesPerPixel)
    {
        case 3:
            if(s->format->Rmask != rgbmasks[0] || s->format->Gmask != rgbmasks[1] || s->format->Bmask != rgbmasks[2])
                return creatergbsurface(s);
            break;
        case 4:
            if(s->format->Rmask != rgbamasks[0] || s->format->Gmask != rgbamasks[1] || s->format->Bmask != rgbamasks[2] || s->format->Amask != rgbamasks[3])
                return creatergbasurface(s);
            break;
    }
    return s;
}

SDL_Surface *scalesurface(SDL_Surface *os, int w, int h, bool clear)
{
    SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, os->format->BitsPerPixel, os->format->Rmask, os->format->Gmask, os->format->Bmask, os->format->Amask);
    if(!ns) fatal("scalesurface");
    scaletexture((uchar *)os->pixels, os->w, os->h, os->format->BytesPerPixel, (uchar *)ns->pixels, w, h);
    if(clear) SDL_FreeSurface(os);
    return ns;
}

SDL_Surface *flipsurface(SDL_Surface *os, bool clear)
{
    SDL_Surface *ns = SDL_CreateRGBSurface(SDL_SWSURFACE, os->w, os->h, os->format->BitsPerPixel, os->format->Rmask, os->format->Gmask, os->format->Bmask, os->format->Amask);
    if(!ns) fatal("flipsurface");
    uchar *dst = (uchar *)ns->pixels, *src = (uchar *)os->pixels;
    loopi(os->h)
    {
        memcpy(dst, &src[os->pitch*(os->h-i-1)], os->format->BytesPerPixel*os->w);
        dst += ns->pitch;
        src += os->pitch;
    }
    if(clear) SDL_FreeSurface(os);
    return ns;
}

static vec parsevec(const char *arg)
{
	vec v(0, 0, 0);
	int i = 0;
    for(; arg[0] && (!i || arg[0]=='/') && i<3; arg += strcspn(arg, "/,><"), i++)
    {
        if(i) arg++;
        v[i] = atof(arg);
    }
	if(i==1) v.y = v.z = v.x;
	return v;
}

#define textureparse(n, f) \
	const char *cmds = NULL, *file = n; \
    if(!n) file = n = f; \
    if(n[0]=='<') \
    { \
    	cmds = n; \
        file = strrchr(n, '>'); \
        if(!file) file = f; \
        else file++; \
    }

SDL_Surface *texturedata(const char *tname, Slot::Tex *tex, bool msg, bool *compress, TextureAnim *anim)
{
	textureparse(tname, tex ? tex->name : NULL);

	if(!file) { if(msg) conoutf("\frcould not load texture %s", tname); return NULL; }

    if(cmds)
    {
        if(renderpath==R_FIXEDFUNCTION && !strncmp(cmds, "<noff>", 6)) return &stubsurface;
    }

    if(msg) renderprogress(loadprogress, file);

    SDL_Surface *s = loadsurface(file);
    if(!s) { if(msg) conoutf("\frcould not load texture %s", file); return NULL; }
    int bpp = s->format->BitsPerPixel;
    if(!texformat(bpp)) { SDL_FreeSurface(s); conoutf("\frtexture must be 8, 16, 24, or 32 bpp: %s", file); return NULL; }

    while(cmds)
    {
        const char *cmd = NULL, *end = NULL, *arg[4] = { NULL, NULL, NULL, NULL };
        cmd = &cmds[1];
        end = strchr(cmd, '>');
        if(!end) break;
        cmds = strchr(cmd, '<');
        size_t len = strcspn(cmd, ":,><");
        loopi(4)
        {
            arg[i] = strchr(i ? arg[i-1] : cmd, i ? ',' : ':');
            if(!arg[i] || arg[i] >= end) arg[i] = "";
            else arg[i]++;
        }
        if(!strncmp(cmd, "mad", len)) s = texmad(s, parsevec(arg[0]), parsevec(arg[1]));
        else if(!strncmp(cmd, "ffcolor", len))
        {
            if(renderpath==R_FIXEDFUNCTION) s = texmad(s, parsevec(arg[0]), parsevec(arg[1]));
        }
        else if(!strncmp(cmd, "ffmask", len))
        {
            s = texffmask(s, atoi(arg[0]));
            if(s == &stubsurface) return s;
        }
        else if(!strncmp(cmd, "dup", len)) texdup(s, atoi(arg[0]), atoi(arg[1]));
        else if(!strncmp(cmd, "decal", len)) s = texdecal(s);
        else if(!strncmp(cmd, "offset", len)) s = texoffset(s, atoi(arg[0]), atoi(arg[1]));
        else if(!strncmp(cmd, "rotate", len)) s = texrotate(s, atoi(arg[0]), tex ? tex->type : 0);
        else if(!strncmp(cmd, "reorient", len)) s = texreorient(s, atoi(arg[0])>0, atoi(arg[1])>0, atoi(arg[2])>0, tex ? tex->type : TEX_DIFFUSE);
        else if(!strncmp(cmd, "crop", len)) s = texcrop(s, atoi(arg[0]), atoi(arg[1]), atoi(arg[2]), atoi(arg[3]));
        else if(!strncmp(cmd, "mix", len)) s = texmix(s, *arg[0] ? atoi(arg[0]) : -1, *arg[1] ? atoi(arg[1]) : -1, *arg[2] ? atoi(arg[2]) : -1, *arg[3] ? atoi(arg[3]) : -1);
        else if(!strncmp(cmd, "compress", len))
        {
            if(compress) *compress = true;
            if(!hasTC)
            {
                int scale = atoi(arg[0]);
                if(scale > 1) s = scalesurface(s, s->w/scale, s->h/scale);
            }
        }
        else if(!strncmp(cmd, "anim", len))
        {
        	if(anim)
        	{
				anim->delay = arg[0] ? atoi(arg[0]) : 0;
				anim->w = arg[1] ? atoi(arg[1]) : 0;
				anim->h = arg[2] ? atoi(arg[2]) : 0;
				if(!anim->w) anim->w = s->h < s->w ? s->h : s->w;
				if(!anim->h) anim->h = s->h < s->w ? s->h : s->w;
				anim->x = s->w/anim->w;
				anim->y = s->h/anim->h;
				anim->count = anim->x*anim->y;
        	}
        }
    }
    if((anim && anim->count ? max(anim->w, anim->h) : max(s->w, s->h)) > (1<<12))
    {
        SDL_FreeSurface(s);
        conoutf("\frtexture size exceeded %dx%d: %s", 1<<12, 1<<12, file);
        return NULL;
    }
    return s;
}

void loadalphamask(Texture *t)
{
	if(t->alphamask || t->bpp!=32) return;
    SDL_Surface *s = texturedata(t->name, NULL, false);
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
	if(!t)
	{
		SDL_Surface *s;
		TextureAnim anim;
		bool compress = false;
		if((s = texturedata(tname, NULL, msg, &compress, &anim)) != NULL)
			t = newtexture(NULL, tname, s, clamp, mipit, false, false, compress, true, &anim);
		else t = notexture;
	}
    return t;
}

void settexture(const char *name, int clamp)
{
    glBindTexture(GL_TEXTURE_2D, textureload(name, clamp, true, false)->id);
}

vector<Slot> slots;
Slot materialslots[MATF_VOLUME+1];

int curtexnum = 0, curmatslot = -1;

void resettextures()
{
    resetslotshader();
	curtexnum = 0;
	slots.setsize(0);
}

ICOMMAND(texturereset, "", (void), if(editmode || worldidents) resettextures(););

void resetmaterials()
{
    loopi(MATF_VOLUME+1) materialslots[i].reset();
}

ICOMMAND(materialreset, "", (void), if(editmode || worldidents) resetmaterials(););
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
	return tryint && isnumeric(*name) ? atoi(name) : -1;
}

const char *findtexturename(int type)
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
	else if(curmatslot>=0) matslot = curmatslot;
	else if(!curtexnum) return;
	Slot &s = matslot>=0 ? materialslots[matslot] : (tnum!=TEX_DIFFUSE ? slots.last() : slots.add());
	s.loaded = false;
    s.texmask |= 1<<tnum;
	if (s.sts.length() >= TEX_MAX) conoutf("\frwarning: too many textures, [%d] %s (%d,%d)", curtexnum, name, matslot, curmatslot);
	Slot::Tex &st = s.sts.add();
	st.type = tnum;
	st.combined = -1;
	st.t = NULL;
	s_strcpy(st.lname, name);
	s_strcpy(st.name, name);
    if(tnum==TEX_DIFFUSE)
    {
        setslotshader(s);
        s.rotation = clamp(*rot, 0, 5);
        s.xoffset = max(*xoffset, 0);
        s.yoffset = max(*yoffset, 0);
        s.scale = *scale <= 0 ? 1 : *scale;
    }
}

COMMAND(texture, "ssiiif");

void texturedel(int i, bool local)
{
	loopj(curtexnum-i)
	{
		int oldtex = i+j, newtex = max(i+j-1, 0);
		if(local) client::edittrigger(sel, EDIT_REPLACE, oldtex, newtex);
		loopk(8) replacetexcube(worldroot[k], oldtex, newtex);
	}
	slots[i].reset();
	slots.remove(i);
	curtexnum--;
}

ICOMMAND(texturedel, "i", (int *i), {
	if (curtexnum > 8)
	{
		if (*i >= 0 && *i <= curtexnum)
		{
			texturedel(*i, true);
			allchanged();
		}
		else conoutf("\frtexture to delete must be in range 0..%d", curtexnum);
	}
	else conoutf("\frnot enough texture slots, please add another one first");
});

bool findcubetex(cube *c, int i)
{
	loopj(8)
	{
		if(c[j].children)
		{
			if(findcubetex(c[j].children, i)) return true;
		}
		else if(!isempty(c[j]))
		{
			loopk(6) if(c[j].texture[k] == i) return true;
		}
	}
	return false;
}

void texturecull(bool local)
{
	loopirev(curtexnum)
	{
		if(curtexnum <= 8) break;
		if(!findcubetex(worldroot, i)) texturedel(i, local);
	}
}

ICOMMAND(texturecull, "", (void), {
	if (curtexnum > 8)
	{
		texturecull(true);
		allchanged();
	}
	else conoutf("\frnot enough texture slots");
});

void autograss(char *name)
{
	Slot &s = slots.last();
	DELETEA(s.autograss);
	s.autograss = newstring(name[0] ? name : "textures/grass");
}
COMMAND(autograss, "s");

void texscroll(float *scrollS, float *scrollT)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.scrollS = *scrollS/1000.0f;
    s.scrollT = *scrollT/1000.0f;
}
COMMAND(texscroll, "ff");

void texoffset_(int *xoffset, int *yoffset)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.xoffset = max(*xoffset, 0);
    s.yoffset = max(*yoffset, 0);
}
COMMANDN(texoffset, texoffset_, "ii");

void texrotate_(int *rot)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.rotation = clamp(*rot, 0, 5);
}
COMMANDN(texrotate, texrotate_, "i");

void texlayer(int *layer)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.layer = *layer < 0 ? max(slots.length()-1+*layer, 0) : *layer;

}
COMMAND(texlayer, "i");

void texscale(float *scale)
{
    if(slots.empty()) return;
    Slot &s = slots.last();
    s.scale = *scale <= 0 ? 1 : *scale;
}
COMMAND(texscale, "f");

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

static void addbump(SDL_Surface *c, SDL_Surface *n)
{
	writetex(c,
		sourcetex(n);
		loopk(3) dst[k] = int(dst[k])*(int(src[2])*2-255)/255;
	);
}

static void addglow(SDL_Surface *c, SDL_Surface *g, const vec &glowcolor)
{
    writetex(c,
        sourcetex(g);
        loopk(3) dst[k] = clamp(int(dst[k]) + int(src[k]*glowcolor[k]), 0, 255);
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

static void addname(vector<char> &key, Slot &slot, Slot::Tex &t, bool combined = false, const char *prefix = NULL)
{
    if(combined) key.add('&');
    if(prefix) { while(*prefix) key.add(*prefix++); }
    s_sprintfd(tname)("%s", t.name);
	for(const char *s = tname; *s; key.add(*s++));
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
					addname(key, s, s.sts[i], true);
				}
				break;
			} // fall through to shader case

		case TEX_NORMAL:
		{
			if(renderpath==R_FIXEDFUNCTION) break;
			int i = findtextype(s, t.type==TEX_DIFFUSE ? (1<<TEX_SPEC) : (1<<TEX_DEPTH));
			if(i<0) break;
			s.sts[i].combined = index;
			addname(key, s, s.sts[i], true);
			break;
		}
	}
	key.add('\0');
	t.t = textures.access(key.getbuf());
	if(t.t) return;

	TextureAnim anim;
	bool compress = false;
	SDL_Surface *ts = texturedata(NULL, &t, true, &compress, &anim);
    if(!ts) { t.t = notexture; return; }
	switch(t.type)
	{
		case TEX_DIFFUSE:
			if(renderpath==R_FIXEDFUNCTION)
			{
				loopv(s.sts)
				{
					Slot::Tex &b = s.sts[i];
					if(b.combined!=index) continue;
					SDL_Surface *bs = texturedata(NULL, &b, true, NULL);
					if(!bs) continue;
					if(bs->w!=ts->w || bs->h!=ts->h) bs = scalesurface(bs, ts->w, ts->h);
					switch(b.type)
					{
						case TEX_DECAL: if(bs->format->BitsPerPixel==32) blenddecal(ts, bs); break;
						case TEX_NORMAL: addbump(ts, bs); break;
					}
					SDL_FreeSurface(bs);
				}
				break;
			}
		case TEX_NORMAL:
			loopv(s.sts)
			{
				Slot::Tex &a = s.sts[i];
				if(a.combined!=index) continue;
				SDL_Surface *as = texturedata(NULL, &a, true, NULL);
				if(!as) break;
				if(ts->format->BitsPerPixel!=32) ts = creatergbasurface(ts);
				if(as->w!=ts->w || as->h!=ts->h) as = scalesurface(as, ts->w, ts->h);
				switch(a.type)
				{
					case TEX_SPEC: mergespec(ts, as); break;
					case TEX_DEPTH: mergedepth(ts, as); break;
				}
				SDL_FreeSurface(as);
				break;
			}
			break;
	}

	t.t = newtexture(NULL, key.getbuf(), ts, 0, true, true, true, compress, true, &anim);
}

Slot dummyslot;

Slot &lookuptexture(int slot, bool load)
{
    Slot &s = slot<0 && slot>=-MATF_VOLUME ? materialslots[-slot] : (slots.inrange(slot) ? slots[slot] : (slots.empty() ? dummyslot : slots[0]));
	if(s.loaded || !load) return s;
    linkslotshader(s);
	loopv(s.sts)
	{
		Slot::Tex &t = s.sts[i];
		if(t.combined>=0) continue;
		switch(t.type)
		{
			case TEX_ENVMAP:
                if(hasCM && (renderpath!=R_FIXEDFUNCTION || (slot<0 && slot>=-MATF_VOLUME))) t.t = cubemapload(t.name);
				break;

			default:
                texcombine(s, i, t, slot<0 && slot>=-MATF_VOLUME);
				break;
		}
	}
	s.loaded = true;
	return s;
}

void linkslotshaders()
{
    loopv(slots) if(slots[i].loaded) linkslotshader(slots[i]);
    loopi(MATF_VOLUME+1) if(materialslots[i].loaded) linkslotshader(materialslots[i]);
}

Texture *loadthumbnail(Slot &slot)
{
    if(slot.thumbnail) return slot.thumbnail;
    linkslotshader(slot, false);
    vector<char> name;
    addname(name, slot, slot.sts[0], false, "<thumbnail>");
    int glow = -1;
    if(slot.texmask&(1<<TEX_GLOW))
    {
        loopvj(slot.sts) if(slot.sts[j].type==TEX_GLOW) { glow = j; break; }
        if(glow >= 0)
        {
            s_sprintfd(prefix)("<mad:%.2f/%.2f/%.2f>", slot.glowcolor.x, slot.glowcolor.y, slot.glowcolor.z);
            addname(name, slot, slot.sts[glow], true, prefix);
        }
    }
    Slot *layer = slot.layer ? &lookuptexture(slot.layer, false) : NULL;
    if(layer) addname(name, *layer, layer->sts[0], true, "<layer>");
    name.add('\0');
    Texture *t = textures.access(path(name.getbuf()));
    if(t) slot.thumbnail = t;
    else
    {
        SDL_Surface *s = texturedata(NULL, &slot.sts[0], false),
                    *g = glow >= 0 ? texturedata(NULL, &slot.sts[glow], false) : NULL,
                    *l = layer ? texturedata(NULL, &layer->sts[0], false) : NULL;
        if(!s) slot.thumbnail = notexture;
        else
        {
            int xs = s->w, ys = s->h;
            if(s->w > 64 || s->h > 64) s = scalesurface(s, min(s->w, 64), min(s->h, 64));
            if(g)
            {
                if(g->w != s->w || g->h != s->h) g = scalesurface(g, s->w, s->h);
                addglow(s, g, slot.glowcolor);
            }
            if(l)
            {
                if(l->w != s->w/2 || l->h != s->h/2) l = scalesurface(l, s->w/2, s->h/2);
                uchar *src = (uchar *)l->pixels;
                loop(y, l->h) loop(x, l->w)
                {
                    uchar *dst = &((uchar *)s->pixels)[s->format->BytesPerPixel*((y + s->h/2)*s->w + x + s->w/2)];
                    loopk(3) dst[k] = src[k];
                    src += l->format->BytesPerPixel;
                }
            }
            t = newtexture(NULL, name.getbuf(), s, 0, false, false, true);
            t->xs = xs;
            t->ys = ys;
            slot.thumbnail = t;
        }
        if(g) SDL_FreeSurface(g);
        if(l) SDL_FreeSurface(l);
    }
    return t;
}

// environment mapped reflections

cubemapside cubemapsides[6] =
{
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB, "lf", true,  true,  true  },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB, "rt", false, false, true  },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB, "ft", true,  false, false },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB, "bk", false, true,  false },
    { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB, "dn", false, false, true  },
    { GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB, "up", false, false, true  },
};

GLuint cubemapfromsky(int size)
{
	extern Texture *sky[6];
	if(!sky[0]) return 0;

    int tsize = 0, cmw, cmh;
    GLint tw[6], th[6];
    loopi(6)
    {
        glBindTexture(GL_TEXTURE_2D, sky[i]->id);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &tw[i]);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &th[i]);
        tsize = max(tsize, (int)max(tw[i], th[i]));
    }
    cmw = cmh = min(tsize, size);
    resizetexture(cmw, cmh, true, GL_TEXTURE_CUBE_MAP_ARB, cmw, cmh);

	GLuint tex;
	glGenTextures(1, &tex);
    int bufsize = 3*tsize*tsize;
    uchar *pixels = new uchar[2*bufsize],
          *rpixels = &pixels[bufsize];
    loopi(6)
    {
        glBindTexture(GL_TEXTURE_2D, sky[i]->id);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        cubemapside &side = cubemapsides[i];
        reorienttexture(pixels, tw[i], th[i], 3, rpixels, side.flipx, side.flipy, side.swapxy);
        if(side.swapxy) swap(tw[i], th[i]);
        createtexture(!i ? tex : 0, cmw, cmh, rpixels, 3, true, GL_RGB5, side.target, false, true, tw[i], th[i]);
    }
    delete[] pixels;
	return tex;
}

Texture *cubemaploadwildcard(Texture *t, const char *name, bool mipit, bool msg)
{
    if(!hasCM) return NULL;
    string tname;
    if(!name) s_strcpy(tname, t->name);
    else
    {
        s_strcpy(tname, name);
        t = textures.access(path(tname));
        if(t) return t;
    }
	char *wildcard = strchr(tname, '*');
	SDL_Surface *surface[6];
	string sname;
	if(!wildcard) s_strcpy(sname, tname);
    GLenum format = 0;
    int tsize = 0;
    bool compress = false;
	loopi(6)
	{
		if(wildcard)
		{
			s_strncpy(sname, tname, wildcard-tname+1);
			s_strcat(sname, cubemapsides[i].name);
			s_strcat(sname, wildcard+1);
		}
        surface[i] = texturedata(sname, NULL, msg, &compress);
        if(!surface[i])
		{
			loopj(i) SDL_FreeSurface(surface[j]);
			return NULL;
		}
        if(!format) format = texformat(surface[i]->format->BitsPerPixel);
        else if(texformat(surface[i]->format->BitsPerPixel)!=format)
        {
            if(surface[i] && msg) conoutf("\frcubemap texture %s doesn't match other sides' format", sname);
            loopj(i) SDL_FreeSurface(surface[j]);
            return NULL;
        }
        tsize = max(tsize, max(surface[i]->w, surface[i]->h));
	}
    if(name)
    {
        char *key = newstring(tname);
        t = &textures[key];
        t->name = key;
    }
    t->bpp = surface[0]->format->BitsPerPixel;
    t->mipmap = mipit;
    t->clamp = 3;
    t->type = Texture::CUBEMAP;
    t->w = t->xs = tsize;
    t->h = t->ys = tsize;
    if(!t->frames.inrange(0)) t->frames.add(0);
    t->frame = t->delay = 0;
    resizetexture(t->w, t->h, mipit, GL_TEXTURE_CUBE_MAP_ARB, t->w, t->h);
    glGenTextures(1, &t->frames[0]);
    loopi(6)
    {
        cubemapside &side = cubemapsides[i];
        SDL_Surface *s = texreorient(surface[i], side.flipx, side.flipy, side.swapxy);
        createtexture(!i ? t->frames[0] : 0, t->w, t->h, s->pixels, 3, mipit, format, side.target, compress, true, side.swapxy ? s->w : s->h, side.swapxy ? s->w : s->h);
        SDL_FreeSurface(s);
    }
	return t;
}

Texture *cubemapload(const char *name, bool mipit, bool msg)
{
	if(!hasCM) return NULL;
	Texture *t = NULL;
	if(!strchr(name, '*'))
	{
		s_sprintfd(pname)("%s_*", name);
		t = cubemaploadwildcard(NULL, pname, mipit, false);
		if(!t && msg) conoutf("\frcould not load envmap %s", name);
	}
	else t = cubemaploadwildcard(NULL, name, mipit, msg);
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
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB: // lf
                yaw = 270; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB: // rt
                yaw = 90; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB: // ft
                yaw = 0; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB: // bk
                yaw = 180; pitch = 0; break;
            case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB: // dn
                yaw = 90; pitch = -90; break;
            case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB: // up
                yaw = 90; pitch = 90; break;
        }
        glFrontFace((side.flipx==side.flipy)!=side.swapxy ? GL_CCW : GL_CW);
        drawcubemap(rendersize, 0, o, yaw, pitch, !side.flipx, !side.flipy, side.swapxy);
		glReadPixels(0, 0, rendersize, rendersize, GL_RGB, GL_UNSIGNED_BYTE, pixels);
        createtexture(tex, texsize, texsize, pixels, 3, true, GL_RGB5, side.target, false, true, rendersize, rendersize);
	}
    glFrontFace(GL_CCW);
    delete[] pixels;
	glViewport(0, 0, screen->w, screen->h);
	return tex;
}

void initenvmaps()
{
	if(!hasCM) return;
	clearenvmaps();
	skyenvmap = cubemapfromsky(1<<envmapsize);
	const vector<extentity *> &ents = entities::getents();
	loopv(ents)
	{
		const extentity &ent = *ents[i];
		if(ent.type != ET_ENVMAP) continue;
		envmap &em = envmaps.add();
        em.radius = ent.attr[0] ? max(0, min(10000, int(ent.attr[0]))) : envmapradius;
        em.size = ent.attr[1] ? max(4, min(9, int(ent.attr[1]))) : 0;
		em.o = ent.o;
		em.tex = 0;
	}
}

void genenvmaps()
{
	if(envmaps.empty()) return;
	renderprogress(0, "generating environment maps...");
	loopv(envmaps)
	{
		envmap &em = envmaps[i];
		renderprogress(float(i)/float(envmaps.length()), "generating environment maps...");
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

GLuint lookupenvmap(Slot &slot)
{
    loopv(slot.sts) if(slot.sts[i].type==TEX_ENVMAP && slot.sts[i].t) return slot.sts[i].t->id;
    return skyenvmap;
}

GLuint lookupenvmap(ushort emid)
{
	if(emid==EMID_SKY || emid==EMID_CUSTOM) return skyenvmap;
	if(emid==EMID_NONE || !envmaps.inrange(emid-EMID_RESERVED)) return 0;
	GLuint tex = envmaps[emid-EMID_RESERVED].tex;
	return tex ? tex : skyenvmap;
}

void cleanuptextures()
{
    clearenvmaps();
    loopv(slots) slots[i].cleanup();
    loopi(MATF_VOLUME+1) materialslots[i].cleanup();
    vector<Texture *> transient;
    enumerate(textures, Texture, tex,
        cleanuptexture(&tex);
        if(tex.type==Texture::TRANSIENT) transient.add(&tex);
    );
    loopv(transient) textures.remove(transient[i]->name);
}

void writepngchunk(FILE *f, const char *type, uchar *data = NULL, uint len = 0)
{
    uint clen = SDL_SwapBE32(len);
    fwrite(&clen, 1, sizeof(clen), f);
    fwrite(type, 1, 4, f);
    fwrite(data, 1, len, f);

    uint crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)type, 4);
    if(data) crc = crc32(crc, data, len);
    crc = SDL_SwapBE32(crc);
    fwrite(&crc, 1, 4, f);
}

void savepng(const char *filename, SDL_Surface *image, int compress, bool flip)
{
    uchar ctype = 0;
    switch(image->format->BytesPerPixel)
    {
        case 1: ctype = 0; break;
        case 2: ctype = 4; break;
        case 3: ctype = 2; break;
        case 4: ctype = 6; break;
        default: conoutf("\frfailed saving png to %s", filename); return;
    }
    FILE *f = openfile(filename, "wb");
    if(!f) { conoutf("\frcould not write to %s", filename); return; }

    uchar signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    fwrite(signature, 1, sizeof(signature), f);

    uchar ihdr[] = { 0, 0, 0, 0, 0, 0, 0, 0, 8, ctype, 0, 0, 0 };
    *(uint *)ihdr = SDL_SwapBE32(image->w);
    *(uint *)(ihdr + 4) = SDL_SwapBE32(image->h);
    writepngchunk(f, "IHDR", ihdr, sizeof(ihdr));

    int idat = ftell(f);
    uint len = 0;
    fwrite("\0\0\0\0IDAT", 1, 8, f);
    uint crc = crc32(0, Z_NULL, 0);
    crc = crc32(crc, (const Bytef *)"IDAT", 4);

    z_stream z;
    z.zalloc = NULL;
    z.zfree = NULL;
    z.opaque = NULL;

    if(deflateInit(&z, compress) != Z_OK)
        goto error;

    uchar buf[1<<12];
    z.next_out = (Bytef *)buf;
    z.avail_out = sizeof(buf);

    loopi(image->h)
    {
        uchar filter = 0;
        loopj(2)
        {
            z.next_in = j ? (Bytef *)image->pixels + (flip ? image->h-i-1 : i)*image->pitch : (Bytef *)&filter;
            z.avail_in = j ? image->w*image->format->BytesPerPixel : 1;
            while(z.avail_in > 0)
            {
                if(deflate(&z, Z_NO_FLUSH) != Z_OK) goto cleanuperror;
                #define FLUSHZ do { \
                    int flush = sizeof(buf) - z.avail_out; \
                    crc = crc32(crc, buf, flush); \
                    len += flush; \
                    fwrite(buf, 1, flush, f); \
                    z.next_out = (Bytef *)buf; \
                    z.avail_out = sizeof(buf); \
                } while(0)
                FLUSHZ;
            }
        }
    }

    for(;;)
    {
        int err = deflate(&z, Z_FINISH);
        if(err != Z_OK && err != Z_STREAM_END) goto cleanuperror;
        FLUSHZ;
        if(err == Z_STREAM_END) break;
    }

    deflateEnd(&z);

    fseek(f, idat, SEEK_SET);
    len = SDL_SwapBE32(len);
    fwrite(&len, 1, 4, f);
    crc = SDL_SwapBE32(crc);
    fseek(f, 0, SEEK_END);
    fwrite(&crc, 1, 4, f);

    writepngchunk(f, "IEND");

    fclose(f);
    return;

cleanuperror:
    deflateEnd(&z);

error:
    fclose(f);

    conoutf("\frfailed saving png to %s", filename);
}

struct tgaheader
{
    uchar  identsize;
    uchar  cmaptype;
    uchar  imagetype;
    uchar  cmaporigin[2];
    uchar  cmapsize[2];
    uchar  cmapentrysize;
    uchar  xorigin[2];
    uchar  yorigin[2];
    uchar  width[2];
    uchar  height[2];
    uchar  pixelsize;
    uchar  descbyte;
};

void savetga(const char *filename, SDL_Surface *image, int compress, bool flip)
{
    switch(image->format->BytesPerPixel)
    {
        case 3: case 4: break;
        default: conoutf("\frfailed saving tga to %s", filename); return;
    }

    FILE *f = openfile(filename, "wb");
    if(!f) { conoutf("\frcould not write to %s", filename); return; }

    tgaheader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.pixelsize = image->format->BitsPerPixel;
    hdr.width[0] = image->w&0xFF;
    hdr.width[1] = (image->w>>8)&0xFF;
    hdr.height[0] = image->h&0xFF;
    hdr.height[1] = (image->h>>8)&0xFF;
    hdr.imagetype = compress ? 10 : 2;
    fwrite(&hdr, 1, sizeof(hdr), f);

    uchar buf[128*4];
    loopi(image->h)
    {
        uchar *src = (uchar *)image->pixels + (flip ? i : image->h - i - 1)*image->pitch;
        for(int remaining = image->w; remaining > 0;)
        {
            int raw = 1;
            if(compress)
            {
                int run = 1;
                for(uchar *scan = src; run < min(remaining, 128); run++)
                {
                    scan += image->format->BytesPerPixel;
                    if(src[0]!=scan[0] || src[1]!=scan[1] || src[2]!=scan[2] || (image->format->BytesPerPixel==4 && src[3]!=scan[3])) break;
                }
                if(run > 1)
                {
                    fputc(0x80 | (run-1), f);
                    fputc(src[2], f); fputc(src[1], f); fputc(src[0], f);
                    if(image->format->BytesPerPixel==4) fputc(src[3], f);
                    src += run*image->format->BytesPerPixel;
                    remaining -= run;
                    if(remaining <= 0) break;
                }
                for(uchar *scan = src; raw < min(remaining, 128); raw++)
                {
                    scan += image->format->BytesPerPixel;
                    if(src[0]==scan[0] && src[1]==scan[1] && src[2]==scan[2] && (image->format->BytesPerPixel!=4 || src[3]==scan[3])) break;
                }
                fputc(raw - 1, f);
            }
            else raw = min(remaining, 128);
            uchar *dst = buf;
            loopj(raw)
            {
                dst[0] = src[2];
                dst[1] = src[1];
                dst[2] = src[0];
                if(image->format->BytesPerPixel==4) dst[3] = src[3];
                dst += image->format->BytesPerPixel;
                src += image->format->BytesPerPixel;
            }
            fwrite(buf, image->format->BytesPerPixel, raw, f);
            remaining -= raw;
        }
    }

    fclose(f);
}

const char *ifmtexts[IFMT_MAX] = { "", ".bmp", ".png", ".tga" };

void savesurface(SDL_Surface *s, const char *fname, int format, int compress, bool flip, bool skip)
{
    if(!s) return;
    int f = format > IFMT_NONE && format < IFMT_MAX ? format : IFMT_PNG;
    const char *filename = makefile(fname, ifmtexts[f], 0, 1, skip);
    switch(f)
    {
        case IFMT_PNG: savepng(filename, s, compress, flip); break;
        case IFMT_TGA: savetga(filename, s, compress, flip); break;
        case IFMT_BMP:
        {
            const char *name = findfile(filename, "wb");
            if(!name) name = filename;
            SDL_Surface *flipped = flip ? flipsurface(s, false) : s;
            SDL_SaveBMP(flipped, name);
            if(flip) SDL_FreeSurface(flipped);
            break;
        }
        default: break;
    }
}

SDL_Surface *loadsurface(const char *name)
{
    const char *exts[] = { "", ".png", ".tga", ".jpg", ".bmp" }; // bmp is a last resort!
    loopi(sizeof(exts)/sizeof(exts[0]))
    {
        s_sprintfd(buf)("%s%s", name, exts[i]);
        SDL_Surface *s = IMG_Load(findfile(buf, "rb"));
        if(s) return fixsurfaceformat(s);
    }
    return NULL;
}

void flipnormalmapy(char *destfile, char *normalfile) // jpg/png/tga -> tga
{
    SDL_Surface *ns = fixsurfaceformat(IMG_Load(findfile(path(normalfile), "rb")));
    if(!ns) return;
    SDL_Surface *ds = SDL_CreateRGBSurface(SDL_SWSURFACE, ns->w, ns->h, 24, RGBMASKS);
    if(!ds) { SDL_FreeSurface(ns); return; }
    uchar *dst = (uchar *)ds->pixels, *src = (uchar *)ns->pixels;
    loopi(ds->w*ds->h)
    {
        dst[0] = src[0];
        dst[1] = 255 - src[1];
        dst[2] = src[2];
        dst += ds->format->BytesPerPixel;
        src += ns->format->BytesPerPixel;
    }
    savetga(destfile, ds);
    SDL_FreeSurface(ds);
    SDL_FreeSurface(ns);
}

void mergenormalmaps(char *heightfile, char *normalfile) // jpg/png/tga + tga -> tga
{
    SDL_Surface *hs = fixsurfaceformat(IMG_Load(findfile(path(heightfile), "rb")));
    SDL_Surface *ns = fixsurfaceformat(IMG_Load(findfile(path(normalfile), "rb")));
    SDL_Surface *ds = SDL_CreateRGBSurface(SDL_SWSURFACE, ns->w, ns->h, 24, RGBMASKS);
    if(hs && ns && ds && hs->w == ns->w && hs->h == ns->h)
    {
        uchar *dst = (uchar *)ds->pixels,
              *srch = (uchar *)hs->pixels,
              *srcn = (uchar *)ns->pixels;
        loopi(ds->w*ds->h)
        {
            #define S(x) x/255.0f*2-1 
            vec n(S(srcn[0]), S(srcn[1]), S(srcn[2]));
            vec h(S(srch[0]), S(srch[1]), S(srch[2]));
            n.mul(2).add(h).normalize().add(1).div(2).mul(255);
            dst[0] = uchar(n.x);
            dst[1] = uchar(n.y);
            dst[2] = uchar(n.z);
            dst += ds->format->BytesPerPixel;
            srch += hs->format->BytesPerPixel;
            srcn += ns->format->BytesPerPixel;
        }
        savetga(normalfile, ds);
    }
    if(hs) SDL_FreeSurface(hs);
    if(ns) SDL_FreeSurface(ns);
    if(ds) SDL_FreeSurface(ds);
}

COMMAND(flipnormalmapy, "ss");
COMMAND(mergenormalmaps, "sss");

