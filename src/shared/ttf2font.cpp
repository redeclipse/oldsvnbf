//
// ttf2font: make font bitmap and cfg from truetype fonts
// (C) 2008 Blood Frontier - ZLIB License
//
// Created by Quinton "quin" Reeves and Lee "eihrul" Salzman
//
// Usage:
//
// 	-?			this help
// 	-h<S>		home dir, output directory
// 	-f<S>		font file
// 	-i<N>		image size
// 	-s<N>		font size
// 	-p<N>		char padding
// 	-d<N>		shadow depth
// 	-q<N[0..1]>	render quality
// 	-c<N[0..9]>	compress level
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

const char *program, *fonname = "";
string cutname, imgname, cfgname;
int retcode = -1,
	imgsize = 512, fonsize = 56, padsize = 1, shdsize = 2,
		quality = 2, pngcomp = Z_BEST_SPEED;

void conoutf(const char *text, ...)
{
	string str;
	va_list vl;

	va_start(vl, text);
	vsnprintf(str, _MAXDEFSTR-1, text, vl);
	va_end(vl);

#ifdef WIN32
	MessageBox(NULL, str, "TTF2Font", MB_OK|MB_ICONINFORMATION|MB_TOPMOST);
#else
	fprintf(stdout, "%s\n", str);
#endif
}

void erroutf(const char *text, ...)
{
	string str;
	va_list vl;

	va_start(vl, text);
	vsnprintf(str, _MAXDEFSTR-1, text, vl);
	va_end(vl);

#ifdef WIN32
	MessageBox(NULL, str, "TTF2Font Error", MB_OK|MB_ICONERROR|MB_TOPMOST);
#else
	fprintf(stderr, "%s\n", str);
#endif
}

void savepng(SDL_Surface *s, const char *name)
{
	FILE *p = openfile(name, "wb");
	if(p)
	{
		png_structp g = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if(g)
		{
			png_infop i = png_create_info_struct(g);
			if(i)
			{
				if(!setjmp(png_jmpbuf(g)))
				{
					png_init_io(g, p);

					png_set_compression_level(g, pngcomp);

					png_set_IHDR(g, i, s->w, s->h, 8,
						PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
							PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

					png_write_info(g, i);

					uchar *d =(uchar *)s->pixels;
					loopk(s->h)
					{
						png_write_row(g, d);
						d += s->w*s->format->BytesPerPixel;
					}
					png_write_end(g, NULL);
				}
			}
			else erroutf("Failed to create png info struct.");
			png_destroy_write_struct(&g, &i);
		}
		else erroutf("Failed to create png write struct.");
		fclose(p);
	}
	else erroutf("Failed to open %s: %s", name, strerror(errno));
}

// only works on 32 bit surfaces with alpha in 4th byte!
void blitchar(SDL_Surface *dst, SDL_Surface *src, int x, int y)
{
    uchar *dstp = (uchar *)dst->pixels + y*dst->pitch + x*4,
          *srcp = (uchar *)src->pixels;
    int dstpitch = dst->pitch - 4*src->w,
        srcpitch = src->pitch - 4*src->w;
    loop(dy, src->h)
    {
        loop(dx, src->w)
        {
            uint k1 = (255U - srcp[3]) * dstp[3], k2 = srcp[3] * 255U, kmax = max(dstp[3], srcp[3]), kscale = max(kmax * 255U, 1U);
            dstp[0] = (dstp[0]*k1 + srcp[0]*k2) / kscale;
            dstp[1] = (dstp[1]*k1 + srcp[1]*k2) / kscale;
            dstp[2] = (dstp[2]*k1 + srcp[2]*k2) / kscale;
            dstp[3] = kmax;
            dstp += 4;
            srcp += 4;
        }
        dstp += dstpitch;
        srcp += srcpitch;
    }
}

enum { CF_NONE = 0, CF_OK, CF_FAIL, CF_SIZE };

#define FONTSTART	33
#define FONTCHARS	94

struct fontchar { char c; int x, y, w, h; };

int tryfont(TTF_Font *f, int size)
{
	int cf = CF_NONE;

#ifndef WIN32
	conoutf("Attempting to build image size %d...", size);
#endif

	SDL_Surface *t = SDL_CreateRGBSurface(SDL_SWSURFACE, size, size, 32, RMASK, GMASK, BMASK, AMASK);
	if(t)
	{
		loopi(t->h) memset((uchar *)t->pixels + i*t->pitch, 0, 4*t->w);

		vector<fontchar> chars;
		SDL_Color c[3] = { { 1, 1, 1, }, { 255, 255, 255 }, { 0, 0, 0 } };
		int x = padsize, y = padsize, h = 0, ma = 0, mw = 0, mh = 0;

		loopi(FONTCHARS)
		{
			fontchar &a = chars.add();
			a.c = i+FONTSTART;

			s_sprintfd(m)("%c", a.c);
			SDL_Surface *s[2] = { NULL, NULL };
			loopk(2)
			{
				switch(quality)
				{
					case 0:
					{
						s[k] = TTF_RenderText_Solid(f, m, c[k]);
						break;
					}
					case 1:
					{
						s[k] = TTF_RenderText_Shaded(f, m, c[k], c[2]);
						if(s[k])
							SDL_SetColorKey(s[k], SDL_SRCCOLORKEY,
								SDL_MapRGBA(s[k]->format, c[2].r, c[2].g, c[2].b, 0));
						break;
					}
					case 2:
					default:
					{
						s[k] = TTF_RenderText_Blended(f, m, c[k]);
						if(s[k]) SDL_SetAlpha(s[k], 0, 0);
						break;
					}
				}
				if(s[k])
				{
					SDL_Surface *c = SDL_ConvertSurface(s[k], t->format, SDL_SWSURFACE);
					SDL_FreeSurface(s[k]);
					s[k] = c;
				}
				if(!s[k])
				{
					if(k && s[0]) SDL_FreeSurface(s[0]);
					erroutf("Failed to render font: %s", TTF_GetError());
					cf = CF_FAIL;
					break;
				}
			}

			if(!cf)
			{
				a.x = x;
				a.y = y;
				a.w = s[0]->w+shdsize;
				a.h = s[0]->h+shdsize;

				if(a.x+a.w >= t->w)
				{
					a.y = y = a.y+h+padsize;
					a.x = x = h = 0;
				}

				if(a.y+a.h >= t->h)
				{
#ifndef WIN32
					conoutf("Exceeded size %d at %d, trying next one..", t->h, a.y+a.h);
#endif
					cf = CF_SIZE; // keep going, we want optimal sizes
					break;
				}

				loopk(2) if(k || shdsize)
				{
					bool sd = shdsize && !k;
					blitchar(t, s[k], a.x+(sd?shdsize:0), a.y+(sd?shdsize:0));
				}

				x += a.w+padsize;
				if(a.h > h) h = a.h;
				ma += (a.w+(padsize*2))*(a.h+(padsize*2));
				mw += a.w;
				mh += a.h;
			}
			else break;
		}

		if(!cf)
		{
			FILE *p = openfile(cfgname, "w");
			if(p)
			{
				int ow = mw/FONTCHARS, oh = mh/FONTCHARS;

				savepng(t, imgname);

				fprintf(p, "// font generated by ttf2font\n\n");
				fprintf(p, "font default \"%s\" %d %d\n\n", imgname, ow, oh);
				loopv(chars)
				{
					if(chars[i].h != oh)
					{
						if(chars[i].w != ow)
						{
							fprintf(p, "fontchar\t%d\t%d\t%d\t%d\t\t// %c\n",
								chars[i].x, chars[i].y, chars[i].w, chars[i].h, chars[i].c);
						}
						else
						{
							fprintf(p, "fontchar\t%d\t%d\t\t%d\t\t// %c\n",
								chars[i].x, chars[i].y, chars[i].h, chars[i].c);
						}
					}
					else if(chars[i].w != ow)
					{
						fprintf(p, "fontchar\t%d\t%d\t%d\t\t\t// %c\n",
							chars[i].x, chars[i].y, chars[i].w, chars[i].c);
					}
					else
					{
						fprintf(p, "fontchar\t%d\t%d\t\t\t\t// %c\n",
							chars[i].x, chars[i].y, chars[i].c);
					}
				}
				fclose(p);

#ifndef WIN32
				conoutf(" "); // seperate the previous output nicely on other platforms
#endif
				conoutf("Completed conversion of font at size %d, dimensions %dx%d.\nUsed %d of %d surface pixels (%.1f%%).",
					fonsize, t->w, t->h,
					ma, t->w*t->h, (float(ma)/float(t->w*t->h))*100.f,
					cfgname
				);
				cf = CF_OK;
			}
			else erroutf("Failed to open %s: %s", cfgname, strerror(errno));
		}
		chars.setsize(0);
		SDL_FreeSurface(t);
	}
	else erroutf("Failed to create SDL surface: %s", SDL_GetError());

	return cf ? cf : CF_FAIL; // CF_NONE is only used internally..
}

void makefont()
{
	TTF_Font *f = TTF_OpenFont(findfile(fonname, "r"), fonsize);
	if(f)
	{
		for(int size = imgsize; retcode < 0; size += 2)
		{
			switch(tryfont(f, size))
			{
				case CF_OK:		retcode = EXIT_SUCCESS; break;
				case CF_FAIL:	retcode = EXIT_FAILURE; break;
				case CF_SIZE:
				default:		break; // continue trying then..
			}
		}
		TTF_CloseFont(f);
	}
	else erroutf("Failed to open font: %s", TTF_GetError());
}

void usage()
{
	conoutf("%s usage:\n\n\t-?\t\tthis help\n\t-h<S>\t\thome dir, output directory\n\t-f<S>\t\tfont file\n\t-i<N>\t\timage size\n\t-s<N>\t\tfont size\n\t-p<N>\t\tchar padding\n\t-d<N>\t\tshadow depth\n\t-q<N[0..1]>\trender quality\n\t-c<N[0..9]>\tcompress level", program);
}

int main(int argc, char *argv[])
{
	program = argv[0];
	sethomedir(".");

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0]=='-') switch(argv[i][1])
		{
			case '?': retcode = EXIT_SUCCESS; break;
			case 'h': sethomedir(&argv[i][2]); break;
			case 'f': fonname = &argv[i][2]; break;
			case 'i': imgsize = min(atoi(&argv[i][2]), 2); break;
			case 's': fonsize = min(atoi(&argv[i][2]), 1); break;
			case 'p': padsize = min(atoi(&argv[i][2]), 0); break;
			case 'd': shdsize = min(atoi(&argv[i][2]), 0); break;
			case 'q': quality = clamp(atoi(&argv[i][2]), 0, 2); break;
			case 'c': pngcomp = clamp(atoi(&argv[i][2]), Z_NO_COMPRESSION, Z_BEST_COMPRESSION); break;
			default:
			{
				erroutf("Unknown command line argument -%c", argv[i][1]);
				retcode = EXIT_FAILURE;
				break;
			}
		}
		else fonname = argv[i]; // for drag and drop on win32. or laziness
	}

	if(retcode < 0 && *fonname)
	{
		char *dot = strpbrk(fonname, ".");
		if(dot) s_strncpy(cutname, fonname, dot-fonname+1);
		else s_strcpy(cutname, fonname);

		s_sprintf(imgname)("%s.png", cutname);
		s_sprintf(cfgname)("%s.cfg", cutname);

		imgsize -= imgsize%2; // power of two

		if(SDL_Init(SDL_INIT_NOPARACHUTE) != -1)
		{
			if(TTF_Init() != -1)
			{
				makefont();
				TTF_Quit();
			}
			else erroutf("Failed to initialize TTF: %s", TTF_GetError());

			SDL_Quit();
		}
		else erroutf("Failed to initialize SDL: %s", SDL_GetError());
	}
	else usage();

	return retcode < 0 ? EXIT_FAILURE : retcode;
}
