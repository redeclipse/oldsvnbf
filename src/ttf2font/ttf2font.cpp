// ttf2font: make font bitmap and cfg from truetype fonts
// (C) 2008 Anthony Cord, Quinton Reeves - Blood Frontier ZLIB License

#include "pch.h"
#include "tools.h"

#ifdef main
#undef main
#endif

enum { MSG_NORMAL = 0, MSG_ERROR };

void msg(int type, char *text)
{
	fprintf(type == MSG_ERROR ? stderr : stdout, "[%d] %s\n", type, text);

#ifdef WIN32
	if(type == MSG_ERROR)
		MessageBox(NULL, text, "ttf2font error", type | MB_TOPMOST);
#endif
}

void cleanup()
{
	TTF_Quit();
	SDL_Quit();
}

void conoutf(char *text, ...)
{
	string str;
	va_list vl;

	va_start(vl, text);
	vsnprintf(str, _MAXDEFSTR-1, text, vl);
	va_end(vl);

	msg(MSG_NORMAL, str);
}

void fatal(char *text, ...)
{
	string str;
	va_list vl;

	va_start(vl, text);
	vsnprintf(str, _MAXDEFSTR-1, text, vl);
	va_end(vl);

	msg(MSG_ERROR, str);
	cleanup();
	exit(EXIT_FAILURE);
}

void savepng(SDL_Surface *s, const char *name)
{
	FILE *p = fopen(name, "wb");
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

					png_set_compression_level(g, Z_BEST_SPEED);

					png_set_IHDR(g, i, s->w, s->h, s->format->BytesPerPixel/4*8,
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
			else fatal("failed to create png info struct");
			png_destroy_write_struct(&g, &i);
		}
		else fatal("failed to create png write struct");
		fclose(p);
	}
	else fatal("fopen: [%s] %s", name, strerror(errno));
}

#define TTFSTART	33
#define TTFCHARS	94

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

#define color(n,x,y,z) SDL_Color c; c.r = x; c.g = y; c.b = z;

void ttf2font(const char *name, int size, int pt, int shadow)
{
	string n;

	char *dot = strpbrk(name, ".");
	if(dot) s_strncpy(n, name, dot-name+1);
	else s_strcpy(n, name);

	TTF_Font *f = TTF_OpenFont(findfile(name, "r"), pt);

	if(f)
	{
		SDL_Surface *t = SDL_CreateRGBSurface(SDL_SRCALPHA, size, size, 32, RMASK, GMASK, BMASK, AMASK);
		if(t)
		{
			s_sprintfd(o)("%s.cfg", n);
			FILE *p = fopen(findfile(o, "w"), "w");
			if(p)
			{
				SDL_Color c[2] = { { 255, 255, 255 }, { 0, 0, 0 } };
				s_sprintfd(b)("%s.png", n);

				fprintf(p, "font default \"%s\" 32 64 0 0 0 0\n", b);
				int x = 0, y = 0, h = 0;
				loopi(TTFCHARS)
				{
					s_sprintfd(a)("%c", i+TTFSTART);
					SDL_Surface *s[2];
					loopk(2)
					{
						if(!(s[k] = TTF_RenderText_Solid(f, a, c[k])))
							fatal("TTF_RenderText_Solid: [%s:%s] %s", name, size, TTF_GetError());
					}

					if(y+s[0]->h+shadow >= size)
						fatal("Image exceeded max size of %d (%d)", size, y+s[0]->h+shadow);
					if(x+s[0]->w+shadow >= size)
					{
						y += h;
						x = h = 0;
					}

					SDL_Rect q = { 0, 0, s[0]->w+shadow, s[0]->h+shadow };

					loopk(shadow ? 2 : 1)
					{
						bool w = shadow && !k ? true : false;
						SDL_Rect r = { x+(w?shadow:0), y+(w?shadow:0), s[w?1:0]->w, s[w?1:0]->h };
						SDL_BlitSurface(s[w?1:0], &q, t, &r);
					}

					fprintf(p, "fontchar\t%d\t%d\t%d\t%d\t\t// %s\n", x, y, s[0]->w+shadow, s[0]->h+shadow, a);

					x += s[0]->w+shadow;
					if(s[0]->h+shadow > h) h = s[0]->h+shadow;

					loopk(2) SDL_FreeSurface(s[k]);
				}

				savepng(t, findfile(b, "w"));
				fclose(p);
				conoutf("%s loaded, %s and %s saved", name, b, o);
			}
			else fatal("fopen: [%s] %s", o, strerror(errno));

			SDL_FreeSurface(t);
		}
		else fatal("TSDL_CreateRGBSurface: %s", SDL_GetError());
		TTF_CloseFont(f);
	}
	else fatal("TTF_OpenFont: [%s:%d] %s", name, size, TTF_GetError());
}

int main(int argc, char *argv[])
{
	if(argc > 1)
	{
		sethomedir(".");
		if(SDL_Init(SDL_INIT_NOPARACHUTE) != -1)
		{
			if(TTF_Init() != -1)
				ttf2font(argv[1], argc > 2 ? atoi(argv[2]) : 512, argc > 3 ? atoi(argv[3]) : 68, argc > 4 ? atoi(argv[4]) : 4);
			else fatal("TTF_Init: %s", TTF_GetError());
		}
		else fatal("Could not initialize SDL: %s", SDL_GetError());
	}
	else fatal("usage: %s <font.ttf> [image size] [font size] [shadow depth]", argv[0]);

	cleanup();
	return EXIT_SUCCESS;
}
