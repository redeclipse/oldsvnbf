//
// img2anim: create animation sequences from images
// (C) 2008 Blood Frontier - ZLIB License
//
// Created by Quinton "quin" Reeves
//
#include "pch.h"
#include "tools.h"
#include <errno.h>

#ifdef main
#undef main
#endif

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

const char *program, *outname = "";
string imgname;
int retcode = -1, pngcomp = Z_BEST_SPEED;

void conoutf(const char *text, ...)
{
	string str;
	va_list vl;

	va_start(vl, text);
	vsnprintf(str, _MAXDEFSTR-1, text, vl);
	va_end(vl);

	fprintf(stdout, "%s\n", str);
}

void erroutf(const char *text, ...)
{
	string str;
	va_list vl;

	va_start(vl, text);
	vsnprintf(str, _MAXDEFSTR-1, text, vl);
	va_end(vl);

	fprintf(stderr, "%s\n", str);
}

struct frame
{
	SDL_Surface *s;
	string c;

	frame(const char *n, SDL_Surface *m) : s(m) { s_strcpy(c, n); }
	~frame()
	{
		if(s) SDL_FreeSurface(s);
	}
};
vector<frame *> frames;

void addframe(const char *name)
{
	int num = 1;
	string fname;

	char *sep = strpbrk(name, ":");
	if(sep)
	{
		s_strncpy(fname, name, sep-name+1);
		num = atoi(sep+1);
	}
	else s_strcpy(fname, name);

	if(num<=0) num = 1;

	loopi(num)
	{
		frame *f = new frame(fname, loadsurface(fname));
		frames.add(f);
		conoutf("Added frame: %s", f->c);
	}
}

void blitimg(SDL_Surface *d, SDL_Surface *s, int x, int y)
{
	loopi(s->h)
	{
		uchar *dst = (uchar *)d->pixels+((y+i)*d->pitch)+(x*d->format->BytesPerPixel),
			*src = (uchar *)s->pixels+(i*s->pitch);

		memset(dst, 0, s->w*d->format->BytesPerPixel);
		memcpy(dst, src, s->w*s->format->BytesPerPixel);
	}
}

void makeimage()
{
	int y = 0, w = 0, h = 0, bpp = 0;
	loopv(frames) if(frames[i]->s)
	{
		h += frames[i]->s->h;
		if(frames[i]->s->w > w) w = frames[i]->s->w;
		if(frames[i]->s->h > y) y = frames[i]->s->h;
		if(frames[i]->s->format->BitsPerPixel > bpp) bpp = frames[i]->s->format->BitsPerPixel;
	}
	if(w && h)
	{
		SDL_Surface *t = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, bpp, RMASK, GMASK, BMASK, AMASK);

		if(t)
		{
			loopv(frames)
			{
				blitimg(t, frames[i]->s, 0, i*y);
			}

			savepng(t, imgname, pngcomp);
			conoutf("Saved %d frames to %s", frames.length(), imgname);

			SDL_FreeSurface(t);
		}
		else erroutf("Failed to create SDL surface: %s", SDL_GetError());
	}
}

void usage()
{
	conoutf("%s usage:\n", program);
	conoutf("\t-?\t\tthis help");
	conoutf("\t-h<S>\t\thome dir, output directory");
	conoutf("\t-n<S>\t\toutput name");
	conoutf("\t-a<S[:N]>\tadd frame, optional repeat");
	conoutf("\t-z<N[0..9]>\tcompress level");
}

int main(int argc, char *argv[])
{
	program = argv[0];

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0]=='-') switch(argv[i][1])
		{
			case '?': retcode = EXIT_SUCCESS; break;
			case 'h': sethomedir(&argv[i][2]); break;
			case 'n': outname = &argv[i][2]; break;
			case 'a': if(argv[i][2]) addframe(&argv[i][2]); break;
			case 'z': pngcomp = clamp(atoi(&argv[i][2]), Z_NO_COMPRESSION, Z_BEST_COMPRESSION); break;
			default:
			{
				erroutf("Unknown command line argument -%c", argv[i][1]);
				retcode = EXIT_FAILURE;
				break;
			}
		}
		else if(argv[i][0]) addframe(&argv[i][0]);
	}

	if(retcode < 0 && frames.length())
	{
		if(!*outname)
		{
			string cutname;
			char *dot = strpbrk(frames[0]->c, ".");
			if(dot) s_strncpy(cutname, frames[0]->c, dot-frames[0]->c+1);
			else s_strcpy(cutname, frames[0]->c);
			s_sprintf(imgname)("%s.png", cutname);
		}
		else s_sprintf(imgname)("%s.png", outname);

		if(SDL_Init(SDL_INIT_NOPARACHUTE) != -1)
		{
			makeimage();
			SDL_Quit();
		}
		else erroutf("Failed to initialize SDL: %s", SDL_GetError());
	}
	else usage();

	frames.setsize(0);

	return retcode < 0 ? EXIT_FAILURE : retcode;
}
