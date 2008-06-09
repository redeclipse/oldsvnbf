// ttf2font: make font bitmap and cfg from truetype fonts
// (C) 2008 Anthony Cord, Quinton Reeves - Blood Frontier ZLIB License

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

#define TTFSTART	33
#define TTFCHARS	94

string basename, imgname, cfgname;
const char *program, *fontname = "font.ttf";
int retcode = -1, imgsize = 512, fonsize = 64, padsize = 1, shdsize = 2, quality = 2, pngcomp = Z_BEST_SPEED;

struct fontchar
{
	char c;
	int x, y, w, h;
};

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

		loopi(TTFCHARS)
		{
			fontchar &a = chars.add();
			a.c = i+TTFSTART;

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
					conoutf("Exceeded size %d at %d, trying next one.." t->h, a.y+a.h);
#endif
					cf = CF_SIZE; // keep going, we want optimal sizes
					break;
				}

				loopk(shdsize ? 2 : 1)
				{
					bool sd = shdsize && !k ? true : false;
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
				savepng(t, imgname);

				fprintf(p, "// font generated by ttf2font\n\n");
				fprintf(p, "font default \"%s\" %d %d 0 0 0 0\n\n", imgname, mw/TTFCHARS, mh/TTFCHARS);
				loopv(chars)
					fprintf(p, "fontchar\t%d\t%d\t%d\t%d\t\t// %c\n",
						chars[i].x, chars[i].y, chars[i].w, chars[i].h, chars[i].c);
				fclose(p);

#ifndef WIN32
				conoutf("-");
#endif
				conoutf("Conversion of %s at size %d completed.\n\nWrote image %s, dimensions %dx%d.\nUsed %d of %d surface pixels (%.1f%%).\n\nConfig has been saved to %s.",
					fontname, fonsize,
					imgname, t->w, t->h,
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

	return cf;
}

void makefont()
{
	TTF_Font *f = TTF_OpenFont(findfile(fontname, "r"), fonsize);
	if(f)
	{
		int size, cf;
		for(size = imgsize; retcode < 0 && (cf = tryfont(f, size)); size += 2)
		{
			switch(cf)
			{
				case CF_SIZE: break; // continue trying then..
				case CF_OK: retcode = EXIT_SUCCESS; break;
				case CF_FAIL:
				default: retcode = EXIT_FAILURE; break;
			}
		}
		TTF_CloseFont(f);
	}
	else erroutf("Failed to open font: %s", TTF_GetError());
}

void usage()
{
	conoutf("%s usage:\n\n\t-?\t\tthis help\n\t-h<S>\t\thome dir, output directory\n\t-f<N>\t\tfont file\n\t-i<N>\t\timage size\n\t-s<N>\t\tfont size\n\t-p<N>\t\tchar padding\n\t-d<N>\t\tshadow depth\n\t-q<N[0..1]>\trender quality\n\t-c<N[0..9]>\tcompress level", program);
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
			case 'f': fontname = &argv[i][2]; break;
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
	}

	if(retcode < 0)
	{
		char *dot = strpbrk(fontname, ".");
		if(dot) s_strncpy(basename, fontname, dot-fontname+1);
		else s_strcpy(basename, fontname);

		s_sprintf(imgname)("%s.png", basename);
		s_sprintf(cfgname)("%s.cfg", basename);

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
