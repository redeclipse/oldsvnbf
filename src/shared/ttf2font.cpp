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
// 	-n<S>		output name
//	-r			reset character mapping
//	-c<S>		load character mapping
// 	-g<N[0..3]>	game type
// 	-i<N>		image size
// 	-s<N>		font size
// 	-p<N>		char padding
// 	-d<N>		shadow depth
// 	-o<N>		outward shadow
// 	-q<N[0..2]>	render quality
// 	-z<N[0..9]>	compress level
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

enum { GAME_NONE = 0, GAME_SAUER, GAME_AC, GAME_BF, GAME_MAX };

static struct gametypes
{
	const char *fontdir;
} game[] = {
	{ ""				},
	{ "data/"			},
	{ "packages/misc/"	},
	{ "fonts/"			},
};

const char *program, *fonname = NULL, *outname = "default";
string cutname, imgname, cfgname;
int retcode = -1, gametype = GAME_NONE,
	imgsize = 0, fonsize = 0, padsize = 1, shdsize = 2, outline = 1,
		quality = 2, pngcomp = Z_BEST_SPEED;

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

struct fontchar
{
	string c;
	int x, y, w, h;

	fontchar() : x(0), y(0), w(0), h(0) { c[0] = 0; }
	fontchar(char *s) : x(0), y(0), w(0), h(0) { s_strcpy(c, s); }
	~fontchar() {}
};
vector<fontchar> chars;

void loadchars(const char *name)
{
	char *buf = loadfile(name, NULL);
	if(buf)
	{
		char *str = buf, *sep;
		while((sep = strpbrk(str, "\n\r")) != NULL)
		{
			int sub = sep-str+1;
			if(sub && (sub < 2 || (strncmp(str, "//", 2) && strncmp(str, "# ", 2))))
			{
				string s;
				s_strncpy(s, str, sub);
				if(s[0]) chars.add(s);
			}
			str += sub+1;
		}

		delete[] buf;
	}
	else erroutf("could not load chars: %s", name);
}

SDL_Color colours[3] = { { 1, 1, 1, }, { 255, 255, 255 }, { 0, 0, 0 } };

int tryfont(int isize, int fsize, bool commit)
{
	int cf = CF_NONE;

	TTF_Font *f = TTF_OpenFont(findfile(fonname, "r"), fsize);

	if(f)
	{
		if(!commit)
			conoutf("Attempting to build size %d font in dimensions %dx%d...", fsize, isize, isize);

		SDL_Surface *t = SDL_CreateRGBSurface(SDL_SWSURFACE, isize, isize, 32, RMASK, GMASK, BMASK, AMASK);
		if(t)
		{
			if(commit)
			{
				loopi(t->h) memset((uchar *)t->pixels + i*t->pitch, 0, 4*t->w);
			}

			int x = padsize, y = padsize, h = 0, ma = 0, mw = 0, mh = 0;

			loopv(chars)
			{
				fontchar *a = &chars[i];
				SDL_Surface *s[2] = { NULL, NULL };
				loopk(commit ? 2 : 1)
				{
					switch(quality)
					{
						case 0:
						{
							s[k] = TTF_RenderUTF8_Solid(f, a->c, colours[k]);
							break;
						}
						case 1:
						{
							s[k] = TTF_RenderUTF8_Shaded(f, a->c, colours[k], colours[2]);
							if(s[k])
								SDL_SetColorKey(s[k], SDL_SRCCOLORKEY,
									SDL_MapRGBA(s[k]->format, colours[2].r, colours[2].g, colours[2].b, 0));
							break;
						}
						case 2:
						default:
						{
							s[k] = TTF_RenderUTF8_Blended(f, a->c, colours[k]);
							if(s[k]) SDL_SetAlpha(s[k], 0, 0);
							break;
						}
					}
					if(!s[k])
						erroutf("Failed to render \"%s\": %s", a->c, TTF_GetError());

					if(s[k])
					{
						SDL_Surface *c = SDL_ConvertSurface(s[k], t->format, SDL_SWSURFACE);
						SDL_FreeSurface(s[k]);
						s[k] = c;
					}

					if(!s[k])
					{
						if(k && s[0]) SDL_FreeSurface(s[0]);
						cf = CF_FAIL;
						break;
					}
				}

				if(!cf)
				{
					int osize = (shdsize*(outline ? 2 : 1)), nsize = outline ? shdsize : 0;
					a->x = x;
					a->y = y;
					a->w = s[0]->w+osize;
					a->h = s[0]->h+osize;

					if(a->x+a->w >= t->w)
					{
						a->y = y = a->y+h+padsize;
						a->x = x = h = 0;
					}

					if(a->y+a->h >= t->h)
					{
						conoutf("Size %d font exceeded dimensions %dx%d at %d.", fsize, t->w, t->h, a->y+a->h);
						cf = CF_SIZE; // keep going, we want optimal sizes
						break;
					}

					if(commit)
					{
						loopk(2) if(k || shdsize)
						{
							bool sd = shdsize && !k;
							if(sd && outline)
							{
								loop(dx, osize+1)
								{
									loop(dy, osize+1)
									{
										blitchar(t, s[k], a->x+dx, a->y+dy);
									}
								}
							}
							else blitchar(t, s[k], a->x+(sd?osize:nsize), a->y+(sd?osize:nsize));
						}
					}

					x += a->w+padsize;
					if(a->h > h) h = a->h;
					mw += a->w;
					mh += a->h;
					ma += (a->w+padsize)*(a->h+padsize);
				}
				else break;
			}

			if(!cf)
			{
				if(commit)
				{
					FILE *p = openfile(cfgname, "w");
					if(p)
					{
						int ow = mw/FONTCHARS, oh = mh/FONTCHARS;

						savepng(t, imgname, pngcomp);

						fprintf(p, "// font generated by ttf2font\n\n");
						fprintf(p, "font %s \"%s%s\" %d %d\n\n", outname, game[gametype].fontdir, gametype != GAME_BF ? imgname : outname, ow, oh);
						loopv(chars)
						{
							if(chars[i].h != oh)
							{
								fprintf(p, "fontchar\t%d\t%d\t%d\t%d\t\t// %s\n",
									chars[i].x, chars[i].y, chars[i].w, chars[i].h, chars[i].c);
							}
							else if(chars[i].w != ow)
							{
								fprintf(p, "fontchar\t%d\t%d\t%d\t\t\t// %s\n",
									chars[i].x, chars[i].y, chars[i].w, chars[i].c);
							}
							else
							{
								fprintf(p, "fontchar\t%d\t%d\t\t\t\t// %s\n",
									chars[i].x, chars[i].y, chars[i].c);
							}
						}
						fclose(p);

						conoutf("\n-\nCompleted conversion of font at size %d, dimensions %dx%d.\nUsed %d of %d surface pixels (%.1f%%).\nNOTE: You must edit %s, it does not come ready to use.",
							fsize, t->w, t->h,
							ma, t->w*t->h, (float(ma)/float(t->w*t->h))*100.f,
							cfgname
						);
						cf = CF_OK;
					}
					else erroutf("Failed to open %s: %s", cfgname, strerror(errno));
				}
				else cf = CF_OK;
			}
			SDL_FreeSurface(t);
		}
		else erroutf("Failed to create SDL surface: %s", SDL_GetError());

		TTF_CloseFont(f);
	}
	else erroutf("Failed to open font: %s", TTF_GetError());

	return cf ? cf : CF_FAIL; // CF_NONE is only used internally..
}

void makefont()
{
	int isize = imgsize ? imgsize : (fonsize ? 2 : 512), fsize = fonsize ? fonsize : (imgsize ? 256 : 64);
	bool commit = false;

	while(retcode < 0)
	{
		switch(tryfont(isize, fsize, commit))
		{
			case CF_OK:
			{
				if(commit) retcode = EXIT_SUCCESS;
				else commit = true;
				break;
			}
			case CF_FAIL:
			{
				retcode = EXIT_FAILURE;
				break;
			}
			case CF_SIZE:
			default:
			{
				if(!fonsize || imgsize) fsize--;
				else if(!imgsize || fonsize) isize += 2;
				else
				{
					erroutf("Failed to create the size %d font in the dimensions %dx%d.", fsize, isize);
					retcode = EXIT_FAILURE;
				}
				break; // continue trying then..
			}
		}
	}
}

void usage()
{
	conoutf("%s usage:\n", program);
	conoutf("\t-?\t\tthis help");
	conoutf("\t-h<S>\t\thome dir, output directory");
	conoutf("\t-g<N[0..3]>\t\tgame type");
	conoutf("\t-f<S>\t\tfont file");
	conoutf("\t-n<S>\t\toutput name");
	conoutf("\t-r\t\treset character mapping");
	conoutf("\t-c<S>\t\tload character mapping");
	conoutf("\t-i<N>\t\timage size");
	conoutf("\t-s<N>\t\tfont size");
	conoutf("\t-p<N>\t\tchar padding");
	conoutf("\t-d<N>\t\tshadow depth");
	conoutf("\t-o<N[0..1]>\t\toutward shadow");
	conoutf("\t-q<N[0..2]>\trender quality");
	conoutf("\t-z<N[0..9]>\tcompress level");
}

int main(int argc, char *argv[])
{
	loopi(FONTCHARS)
	{
		s_sprintfd(s)("%c", i+FONTSTART);
		chars.add(s);
	}

	program = argv[0];

	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0]=='-') switch(argv[i][1])
		{
			case '?': retcode = EXIT_SUCCESS; break;
			case 'h': sethomedir(&argv[i][2]); break;
			case 'f': fonname = &argv[i][2]; break;
			case 'n': outname = &argv[i][2]; break;
			case 'r': chars.setsize(0); break;
			case 'c': if(argv[i][2]) loadchars(&argv[i][2]); break;
			case 'g': gametype = clamp(atoi(&argv[i][2]), int(GAME_NONE), int(GAME_MAX-1)); break;
			case 'i': imgsize = max(atoi(&argv[i][2]), 0); imgsize -= imgsize%2; break;
			case 's': fonsize = max(atoi(&argv[i][2]), 0); break;
			case 'p': padsize = max(atoi(&argv[i][2]), 0); break;
			case 'd': shdsize = max(atoi(&argv[i][2]), 0); break;
			case 'o': outline = clamp(atoi(&argv[i][2]), 0, 1); break;
			case 'q': quality = clamp(atoi(&argv[i][2]), 0, 2); break;
			case 'z': pngcomp = clamp(atoi(&argv[i][2]), Z_NO_COMPRESSION, Z_BEST_COMPRESSION); break;
			default:
			{
				erroutf("Unknown command line argument -%c", argv[i][1]);
				retcode = EXIT_FAILURE;
				break;
			}
		}
		else fonname = argv[i]; // for drag and drop on win32. or laziness
	}

	if(retcode < 0 && fonname && chars.length())
	{
		char *dot = strpbrk(fonname, ".");
		if(dot) s_strncpy(cutname, fonname, dot-fonname+1);
		else s_strcpy(cutname, fonname);

		if(!*outname) outname = cutname;

		s_sprintf(imgname)("%s.png", outname);
		s_sprintf(cfgname)("%s.cfg", outname);

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

	chars.setsize(0);

	return retcode < 0 ? EXIT_FAILURE : retcode;
}
