#include "pch.h"
#include "engine.h"

static hashtable<char *, font> fonts;
static font *fontdef = NULL;

font *curfont = NULL;

void newfont(char *name, char *tex, int *defaultw, int *defaulth, int *offsetx, int *offsety, int *offsetw, int *offseth)
{
	font *f = fonts.access(name);
	if(!f)
	{
		name = newstring(name);
		f = &fonts[name];
		f->name = name;
	}

	f->tex = textureload(tex);
	f->chars.setsize(0);
	f->defaultw = *defaultw;
	f->defaulth = *defaulth;
	f->offsetx = *offsetx;
	f->offsety = *offsety;
	f->offsetw = *offsetw;
	f->offseth = *offseth;

	fontdef = f;
}

void fontchar(int *x, int *y, int *w, int *h)
{
	if(!fontdef) return;

	font::charinfo &c = fontdef->chars.add();
	c.x = *x;
	c.y = *y;
	c.w = *w ? *w : fontdef->defaultw;
	c.h = *h ? *h : fontdef->defaulth;
}

COMMANDN(font, newfont, "ssiiiiii");
COMMAND(fontchar, "iiii");

bool setfont(char *name)
{
	font *f = fonts.access(name);
	if(!f) return false;
	curfont = f;
	return true;
}

void gettextres(int &w, int &h)
{
	if(w < MINRESW || h < MINRESH)
	{
		if(MINRESW > w*MINRESH/h)
		{
			h = h*MINRESW/w;
			w = MINRESW;
		}
		else
		{
			w = w*MINRESH/h;
			h = MINRESH;
		}
	}
}

#define PIXELTAB (8*curfont->defaultw)

int char_width(int c, int x)
{
	if(!curfont) return x;
	else if(c=='\t') x = ((x+PIXELTAB)/PIXELTAB)*PIXELTAB;
	else if(c==' ') x += curfont->defaultw;
	else if(curfont->chars.inrange(c-33))
	{
		c -= 33;
		x += curfont->chars[c].w+1;
	}
	return x;
}

int text_width(const char *str, int limit)
{
	int x = 0;
	for(int i = 0; str[i] && (limit<0 ||i<limit); i++)
	{
		if(str[i]=='\f')
		{
			i++;
			continue;
		}
		x = char_width(str[i], x);
	}
	return x;
}

int text_visible(const char *str, int max)
{
	int i = 0, x = 0;
	while(str[i])
	{
		if(str[i]=='\f')
		{
			i += 2;
			continue;
		}
		x = char_width(str[i], x);
		if(x > max) return i;
		++i;
	}
	return i;
}

void draw_textf(const char *fstr, int left, int top, ...)
{
	s_sprintfdlv(str, top, fstr);
	draw_text(str, left, top);
}

void draw_textx(const char *fstr, int left, int top, int r, int g, int b, int a, bool shadow, int align, ...)
{
	s_sprintfdlv(str, align, fstr);

	int x = left, y = top, width = text_width(str);

	switch (align)
	{
		case AL_CENTER:
			x -= width/2;
			break;
		case AL_RIGHT:
			x -= width;
			break;
		default:
			break;
	}
	draw_text(str, x, y, r, g, b, a, shadow);
}

static bvec colorstack[256];
int colorpos = 0;

void draw_text(const char *str, int left, int top, int r, int g, int b, int a, bool shadow)
{
	if(!curfont) return;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBindTexture(GL_TEXTURE_2D, curfont->tex->gl);

	bvec color(r, g, b);

	if (a == 255) a = int(255.f*(hudblend*0.01f));

	glBegin(GL_QUADS);
	glColor4ub(r, g, b, a);
 	loopj(shadow ? 2 : 1)
 	{
		if (!shadow || j) glColor4ub(r, g, b, a);
		else  glColor4ub(0, 0, 0, a);

		int off = (j ? -2 : 2);
		int x = left + off;
		int y = top + off;

		int i;

		for (i = 0; str[i] != 0; i++)
		{
			int c = str[i];
			if(c=='\t') { x = ((x-left-off+PIXELTAB)/PIXELTAB)*PIXELTAB+left+off; continue; }
			if(c=='\f')
			{
				if ((!shadow || j) && str[i+1] != 0)
				{
					switch(str[++i])
					{
						case 'w':
						case '0':
						{
							color = bvec(255, 255, 255);
							break; // white
						}
						case 'l':
						case '1':
						{
							color = bvec( 0,   0,    0);
							break; // black
						}
						case 'y':
						case '2':
						{
							color = bvec(255, 192,  64);
							break; // yellow
						}
						case 'r':
						case '3':
						{
							color = bvec(255,  64,  64);
							break; // red
						}
						case 'G':
						case '4':
						{
							color = bvec(128, 128, 128);
							break; // gray
						}
						case 'm':
						case '5':
						{
							color = bvec(192,  64, 192);
							break; // magenta
						}
						case 'o':
						case '6':
						{
							color = bvec(255, 128,	 0);
							break; // orange
						}
						case 'g':
						case '7':
						{
							color = bvec( 64, 255, 128);
							break; // green
						}
						case 'b':
						case '8':
						{
							color = bvec( 96, 160, 255);
							break; // blue
						}
						case 's':
						{
							if((size_t)colorpos<sizeof(colorstack)/sizeof(colorstack[0])) colorstack[colorpos++] = color;
							continue; // save colour
						}
						case 'S':
						{
							if(colorpos<=0) continue;
							color = colorstack[--colorpos];
							break; // restore colour
						}
						default:
						{
							color = bvec(r, g, b);
							break; // default
						}
					}
					glColor4ub(color.x, color.y, color.z, a);
				}
				else { i++; continue; } // shadow
			}
			if(c==' ') { x += curfont->defaultw; continue; }
			c -= 33;
			if(!curfont->chars.inrange(c)) continue;

			font::charinfo &info = curfont->chars[c];
			float tc_left	= (info.x + curfont->offsetx) / float(curfont->tex->xs);
			float tc_top	 = (info.y + curfont->offsety) / float(curfont->tex->ys);
			float tc_right	= (info.x + info.w + curfont->offsetw) / float(curfont->tex->xs);
			float tc_bottom  = (info.y + info.h + curfont->offseth) / float(curfont->tex->ys);

			glTexCoord2f(tc_left,  tc_top	); glVertex2i(x,		  y);
			glTexCoord2f(tc_right, tc_top	); glVertex2i(x + info.w, y);
			glTexCoord2f(tc_right, tc_bottom); glVertex2i(x + info.w, y + info.h);
			glTexCoord2f(tc_left,  tc_bottom); glVertex2i(x,		  y + info.h);

			xtraverts += 4;
			x += info.w + 1;
		}
	}
	glEnd();
}

static vector<font> fontstack;

bool pushfont(char *name)
{
	if (curfont) fontstack.add(*curfont);
	return setfont(name);
}

bool popfont(int num)
{
	int n = fontstack.length();

	loopi(num)
	{
		if (n <= 0) break;
		fontstack.remove(--n);
	}
	return (n != fontstack.length());
}
