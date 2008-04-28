// creates multiple gui windows that float inside the 3d world

// special feature is that its mostly *modeless*: you can use this menu while playing, without turning menus on or off
// implementationwise, it is *stateless*: it keeps no internal gui structure, hit tests are instant, usage & implementation is greatly simplified

#include "pch.h"
#include "engine.h"

static bool layoutpass, actionon = false;
static int mousebuttons = 0;
static struct gui *windowhit = NULL;

//text field state - only one ever active/focused
static string fieldname, fieldtext;  //copy of while focused
static int fieldpos = -1, fieldlen = 0; //-1=no focus, -2=wanting to commit
static bool fieldactive;

bool menukey(int code, bool isdown, int cooked)
{
	if((code==-1 || code == -2) && g3d_windowhit(code, isdown, true)) return true;
	else if(code==-3 && g3d_windowhit(code, isdown, false)) return true;

	if(fieldpos<0) return false;
	switch(code)
	{
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			fieldpos = -2; //signal field commit
			return false;
		case SDLK_HOME:
		case SDLK_END:
		case SDLK_DELETE:
		case SDLK_BACKSPACE:
		case SDLK_LEFT:
		case SDLK_RIGHT:
		case SDLK_TAB:
			break;
		default:
			if(!cooked || (code<32) || (code>127)) return false;
	}
	if(!isdown) return true;
	int len = strlen(fieldtext);
	if(fieldpos>len) fieldpos = len;
	switch(code)
	{
		case SDLK_HOME:
			fieldpos = 0;
			break;
		case SDLK_END:
			fieldpos = len;
			break;
		case SDLK_LEFT:
			if(fieldpos > 0) fieldpos--;
			break;
		case SDLK_RIGHT:
		case SDLK_TAB:
			if(fieldpos < len) fieldpos++;
			break;
		case SDLK_DELETE:
			if(fieldpos < len)
			{
				memmove(fieldtext+fieldpos, fieldtext+fieldpos+1, len-fieldpos);
				fieldtext[len-1] = '\0';
			}
			break;
		case SDLK_BACKSPACE:
			if(fieldpos > 0)
			{
				fieldpos--;
				memmove(fieldtext+fieldpos, fieldtext+fieldpos+1, len-fieldpos);
				fieldtext[len-1] = '\0';
			}
			break;
		default:
			if(fieldpos<fieldlen)
			{
				memmove(fieldtext+fieldpos+1, fieldtext+fieldpos, len-fieldpos); //length then limited inside field draw code
				fieldtext[fieldpos++] = cooked;
				fieldtext[len+1] = '\0';
			}
			break;
	}
	return true;
}

static bool hascursor;
static float cursorx = 0.5f, cursory = 0.5f;

#define SHADOW 4
#define ICON_SIZE (FONTH-SHADOW)
#define SKIN_W 256
#define SKIN_H 128
#define SKIN_SCALE 4
#define INSERT (3*SKIN_SCALE)

VARP(guiautotab, 6, 16, 40);

struct gui : g3d_gui
{
	struct list
	{
		int parent, w, h;
	};

	int nextlist;

	static vector<list> lists;
	static float hitx, hity;
	static int curdepth, curlist, xsize, ysize, curx, cury;

	static void reset()
	{
		lists.setsize(0);
	}

	static int ty, tx, tpos, *tcurrent, tcolor; //tracking tab size and position since uses different layout method...

	void autotab()
	{
		if(tcurrent)
		{
			if(layoutpass && !tpos) tcurrent = NULL; //disable tabs because you didn't start with one
			if(!curdepth && (layoutpass ? 0 : cury) + ysize > guiautotab*FONTH) tab(NULL, tcolor);
		}
	}

	bool visible() { return (!tcurrent || tpos==*tcurrent) && !layoutpass; }

	//tab is always at top of page
	void tab(const char *name, int color)
	{
		if(curdepth != 0) return;
		tcolor = color;
		tpos++;
		if(!name)
		{
			static string title;
			s_sprintf(title)("%d", tpos);
			name = title;
		}
		int w = text_width(name) - 2*INSERT;
		if(layoutpass)
		{
			ty = max(ty, ysize);
			ysize = 0;
		}
		else
		{
			cury = -ysize;
			int h = FONTH-2*INSERT,
				x1 = curx + tx,
				x2 = x1 + w + ((skinx[3]-skinx[2]) + (skinx[5]-skinx[4]))*SKIN_SCALE,
				y1 = cury - ((skiny[5]-skiny[1])-(skiny[3]-skiny[2]))*SKIN_SCALE-h,
				y2 = cury;
			bool hit = tcurrent && windowhit==this && hitx>=x1 && hity>=y1 && hitx<x2 && hity<y2;
			if(hit)
			{
				*tcurrent = tpos; //roll-over to switch tab
				color = 0xFF0000;
			}

			skin_(x1-skinx[visible()?2:6]*SKIN_SCALE, y1-skiny[1]*SKIN_SCALE, w, h, visible()?10:19, 9);
			text_(name, x1 + (skinx[3]-skinx[2])*SKIN_SCALE - INSERT, y1 + (skiny[2]-skiny[1])*SKIN_SCALE - INSERT, color, visible());
		}
		tx += w + ((skinx[5]-skinx[4]) + (skinx[3]-skinx[2]))*SKIN_SCALE;
	}

	bool ishorizontal() const { return curdepth&1; }
	bool isvertical() const { return !ishorizontal(); }

	void pushlist()
	{
		if(layoutpass)
		{
			if(curlist>=0)
			{
				lists[curlist].w = xsize;
				lists[curlist].h = ysize;
			}
			list &l = lists.add();
			l.parent = curlist;
			curlist = lists.length()-1;
			xsize = ysize = 0;
		}
		else
		{
			curlist = nextlist++;
			xsize = lists[curlist].w;
			ysize = lists[curlist].h;
		}
		curdepth++;
	}

	void poplist()
	{
		list &l = lists[curlist];
		if(layoutpass)
		{
			l.w = xsize;
			l.h = ysize;
		}
		curlist = l.parent;
		curdepth--;
		if(curlist>=0)
		{
			xsize = lists[curlist].w;
			ysize = lists[curlist].h;
			if(ishorizontal()) cury -= l.h;
			else curx -= l.w;
			layout(l.w, l.h);
		}
	}

	int text  (const char *text, int color, const char *icon) { autotab(); return button_(text, color, icon, false, false); }
	int button(const char *text, int color, const char *icon) { autotab(); return button_(text, color, icon, true, false); }
	int title (const char *text, int color, const char *icon) { autotab(); return button_(text, color, icon, false, true); }

	void separator() { autotab(); line_(5); }
	void progress(float percent) { autotab(); line_(FONTH*2/5, percent); }

	//use to set min size (useful when you have progress bars)
    void strut(int size) { layout(isvertical() ? size*FONTW : 0, isvertical() ? 0 : size*FONTH); }
	//add space between list items
    void space(int size) { layout(isvertical() ? 0 : size*FONTW, isvertical() ? size*FONTH : 0); }

	int layout(int w, int h)
	{
		if(layoutpass)
		{
			if(ishorizontal())
			{
				xsize += w;
				ysize = max(ysize, h);
			}
			else
			{
				xsize = max(xsize, w);
				ysize += h;
			}
			return 0;
		}
		else
		{
			bool hit = ishit(w, h);
			if(ishorizontal()) curx += w;
			else cury += h;
			return (hit && visible()) ? mousebuttons|G3D_ROLLOVER : 0;
		}
	}

	bool ishit(int w, int h, int x = curx, int y = cury)
	{
		if(ishorizontal()) h = ysize;
		else w = xsize;
		return windowhit==this && hitx>=x && hity>=y && hitx<x+w && hity<y+h;
	}

    int image(Texture *t, float scale, bool overlaid)
	{
		autotab();
		if(scale==0) scale = 1;
		int size = (int)(scale*2*FONTH)-SHADOW;
		if(visible()) icon_(t, overlaid, false, curx, cury, size, ishit(size+SHADOW, size+SHADOW));
		return layout(size+SHADOW, size+SHADOW);
	}

	int texture(Texture *t, float scale)
	{
		autotab();
		if(scale==0) scale = 1;
		int size = (int)(scale*2*FONTH)-SHADOW;
        if(t!=notexture && visible()) icon_(t, true, true, curx, cury, size, ishit(size+SHADOW, size+SHADOW));
		return layout(size+SHADOW, size+SHADOW);
	}

	void slider(int &val, int vmin, int vmax, int color, char *label)
	{
		autotab();
		int x = curx;
		int y = cury;
		line_(10);
		if(visible())
		{
			if(!label)
			{
				static string s;
				s_sprintf(s)("%d", val);
				label = s;
			}
			int w = text_width(label);

			bool hit;
			int px, py;
			if(ishorizontal())
			{
				hit = ishit(FONTH, ysize, x, y);
				px = x + (FONTH-w)/2;
				py = y + (ysize-FONTH) - ((ysize-FONTH)*(val-vmin))/max(vmax-vmin, 1); //zero at the bottom
			}
			else
			{
				hit = ishit(xsize, FONTH, x, y);
				px = x + ((xsize-w)*(val-vmin))/max(vmax-vmin, 1);
				py = y;
			}

			if(hit) color = 0xFF0000;
			text_(label, px, py, color, hit && actionon);
			if(hit && actionon)
			{
				int vnew = 1+vmax-vmin;
				if(ishorizontal()) vnew = int(vnew*(y+ysize-hity)/ysize);
				else vnew = int(vnew*(hitx-x)/xsize);
				vnew += vmin;
				if(vnew != val) val = vnew;
			}
		}
	}

    char *field(const char *name, int color, int length, const char *initval)
	{
        int w, h;
        int maxwidth;
        if(length < 0)
        {
            maxwidth = -length*FONTW;
            length = (int)sizeof(string)-1;
            text_bounds((strcmp(fieldname, name) == 0)?fieldtext:initval, w, h, maxwidth);
            w = maxwidth + FONTW;
        }
        else
        {
            maxwidth = -1;
			length = min(length, (int)sizeof(string)-1);
            h = FONTH;
            w = FONTW*(length + 1);
        }
		char *result = NULL;
        if(visible() && !layoutpass)
		{
            bool hit = ishit(w, h), editing = !strcmp(fieldname, name);
			if(hit && (mousebuttons&G3D_DOWN) && !editing) //mouse request focus
			{
				s_strcpy(fieldname, name);
				s_strcpy(fieldtext, initval);
				fieldtext[length] = '\0';
				fieldpos = strlen(fieldtext);
				fieldlen = length;
				editing = true;
			}
			if(editing)
			{
                if(fieldpos==-2 || !hit) // commit field if user pressed enter or wandered out of focus
                {
                    result = fieldtext;
                    fieldpos = -1;
                    fieldname[0] = '\0';
                    editing = false;
                }
                else fieldactive = true;
			}
			if(editing && hit && (mousebuttons&G3D_PRESSED)) //mouse request position
                fieldpos = text_visible(fieldtext, int(floor(hitx-(curx+FONTW/2)))); /* @TODO fix for multiple lines */

			notextureshader->set();
			glDisable(GL_TEXTURE_2D);
			if(editing) glColor3f(1, 0, 0);
			else glColor3ub(color>>16, (color>>8)&0xFF, color&0xFF);
			glBegin(GL_LINE_LOOP);
            rect_(curx, cury, w, h);
			glEnd();
			glEnable(GL_TEXTURE_2D);
			defaultshader->set();

            draw_text(editing ? fieldtext : (result ? result : initval), curx+FONTW/2, cury);
            //draw_text(editing ? fieldtext : (result ? result : initval), curx+FONTW/2, cury, color>>16, (color>>8)&0xFF, color&0xFF, 0xFF, (editing && hit)?fieldpos:-1, maxwidth);
		}
    	layout(w, h);
		return result;
	}

	void rect_(float x, float y, float w, float h, int usetc = -1)
	{
        static const GLint tc[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
		if(usetc>=0) glTexCoord2iv(tc[usetc]);
		glVertex2f(x, y);
		if(usetc>=0) glTexCoord2iv(tc[(usetc+1)%4]);
		glVertex2f(x + w, y);
		if(usetc>=0) glTexCoord2iv(tc[(usetc+2)%4]);
		glVertex2f(x + w, y + h);
		if(usetc>=0) glTexCoord2iv(tc[(usetc+3)%4]);
		glVertex2f(x, y + h);
		xtraverts += 4;
	}

	void text_(const char *text, int x, int y, int color, bool shadow)
	{
		if(shadow) draw_text(text, x+SHADOW, y+SHADOW, 0x00, 0x00, 0x00, 0xC0);
		draw_text(text, x, y, color>>16, (color>>8)&0xFF, color&0xFF);
	}

    void background(int color, int inheritw, int inherith)
    {
        if(layoutpass) return;
        glDisable(GL_TEXTURE_2D);
        notextureshader->set();
        glColor4ub(color>>16, (color>>8)&0xFF, color&0xFF, 0x80);
        int w = xsize, h = ysize;
        if(inheritw>0)
        {
            int parentw = curlist;
            while(inheritw>0 && lists[parentw].parent>=0)
            {
                parentw = lists[parentw].parent;
                inheritw--;
            }
            w = lists[parentw].w;
        }
        if(inherith>0)
        {
            int parenth = curlist;
            while(inherith>0 && lists[parenth].parent>=0)
            {
                parenth = lists[parenth].parent;
                inherith--;
            }
            h = lists[parenth].h;
        }
        glBegin(GL_QUADS);
        rect_(curx, cury, w, h);
        glEnd();
        glEnable(GL_TEXTURE_2D);
        defaultshader->set();
    }

	void icon_(Texture *t, bool overlaid, bool tiled, int x, int y, int size, bool hit)
	{
		float xs, ys, xt, yt;
		if(tiled)
		{
			xt = min(1.0f, t->xs/(float)t->ys),
			yt = min(1.0f, t->ys/(float)t->xs);
			xs = size;
			ys = size;
		}
		else
		{
			xt = 1.0f;
			yt = 1.0f;
			float scale = float(size)/max(t->xs, t->ys); //scale and preserve aspect ratio
			xs = t->xs*scale;
			ys = t->ys*scale;
			x += int((size-xs)/2);
			y += int((size-ys)/2);
		}
		if(hit && actionon)
		{
			glDisable(GL_TEXTURE_2D);
			notextureshader->set();
			glColor4f(0, 0, 0, 0.75f);
			glBegin(GL_QUADS);
			rect_(x+SHADOW, y+SHADOW, xs, ys);
			glEnd();
			glEnable(GL_TEXTURE_2D);
			defaultshader->set();
		}
		if(tiled)
		{
			static Shader *rgbonlyshader = NULL;
			if(!rgbonlyshader) rgbonlyshader = lookupshaderbyname("rgbonly");
			rgbonlyshader->set();
		}
		if(hit) glColor3f(1, 0.5f, 0.5f);
		else if(overlaid) glColor3f(1, 1, 1);
		else glColor3fv(light.v);
		glBindTexture(GL_TEXTURE_2D, t->id);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f,	0.0f);	glVertex2f(x,	y);
		glTexCoord2f(1.0f/xt, 0.0f);	glVertex2f(x+xs, y);
		glTexCoord2f(1.0f/xt, 1.0f/yt); glVertex2f(x+xs, y+ys);
		glTexCoord2f(0.0f,	1.0f/yt); glVertex2f(x,	y+ys);
		glEnd();
		if(tiled) defaultshader->set();
		if(overlaid)
		{
			if(!overlaytex) overlaytex = textureload("textures/guioverlay.png");
			glColor3fv(light.v);
			glBindTexture(GL_TEXTURE_2D, overlaytex->id);
			glBegin(GL_QUADS);
			rect_(x, y, xs, ys, 0);
			glEnd();
		}
	}

	void line_(int size, float percent = 1.0f)
	{
		if(visible())
		{
			if(!slidertex) slidertex = textureload("textures/guislider.png");
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, slidertex->id);
			glBegin(GL_QUADS);
			if(percent < 0.99f)
			{
				glColor4f(light.x, light.y, light.z, 0.375f);
				if(ishorizontal())
					rect_(curx + FONTH/2 - size, cury, size*2, ysize, 0);
				else
					rect_(curx, cury + FONTH/2 - size, xsize, size*2, 1);
			}
			glColor3fv(light.v);
			if(ishorizontal())
				rect_(curx + FONTH/2 - size, cury + ysize*(1-percent), size*2, ysize*percent, 0);
			else
				rect_(curx, cury + FONTH/2 - size, xsize*percent, size*2, 1);
			glEnd();
		}
		layout(ishorizontal() ? FONTH : 0, ishorizontal() ? 0 : FONTH);
	}

	int button_(const char *text, int color, const char *icon, bool clickable, bool center)
	{
		const int padding = 10;
		int w = 0;
		if(icon) w += ICON_SIZE;
		if(icon && text) w += padding;
		if(text) w += text_width(text);

		if(visible())
		{
			bool hit = ishit(w, FONTH);
			if(hit && clickable) color = 0xFF0000;
			int x = curx;
			if(isvertical() && center) x += (xsize-w)/2;

			if(icon)
			{
				s_sprintfd(tname)("textures/%s.jpg", icon);
				icon_(textureload(tname), false, false, x, cury, ICON_SIZE, clickable && hit);
				x += ICON_SIZE;
			}
			if(icon && text) x += padding;
			if(text) text_(text, x, cury, color, center || (hit && clickable && actionon));
		}
		return layout(w, FONTH);
	}

	static Texture *skintex, *overlaytex, *slidertex;
	static const int skinx[], skiny[];
	static const struct patch { ushort left, right, top, bottom; uchar flags; } patches[];

	void skin_(int x, int y, int gapw, int gaph, int start, int n)//int vleft, int vright, int vtop, int vbottom, int start, int n)
	{
		if(!skintex) skintex = textureload("textures/guiskin.png");
		glBindTexture(GL_TEXTURE_2D, skintex->id);
		int gapx1 = INT_MAX, gapy1 = INT_MAX, gapx2 = INT_MAX, gapy2 = INT_MAX;
		float wscale = 1.0f/(SKIN_W*SKIN_SCALE), hscale = 1.0f/(SKIN_H*SKIN_SCALE);

		bool quads = false;
		glColor4f(1.0f, 1.0f, 1.0f, 0.80f);
		loopi(n)
		{
			const patch &p = patches[start+i];
			int left = skinx[p.left]*SKIN_SCALE, right = skinx[p.right]*SKIN_SCALE,
				top = skiny[p.top]*SKIN_SCALE, bottom = skiny[p.bottom]*SKIN_SCALE;
			float tleft = left*wscale, tright = right*wscale,
					ttop = top*hscale, tbottom = bottom*hscale;
			if(p.flags&0x1)
			{
				gapx1 = left;
				gapx2 = right;
			}
			else if(left >= gapx2)
			{
				left += gapw - (gapx2-gapx1);
				right += gapw - (gapx2-gapx1);
			}
			if(p.flags&0x10)
			{
				gapy1 = top;
				gapy2 = bottom;
			}
			else if(top >= gapy2)
			{
				top += gaph - (gapy2-gapy1);
				bottom += gaph - (gapy2-gapy1);
			}

			//multiple tiled quads if necessary rather than a single stretched one
			int ystep = bottom-top;
			int yo = y+top;
			while(ystep > 0)
			{
				if(p.flags&0x10 && yo+ystep-(y+top) > gaph)
				{
					ystep = gaph+y+top-yo;
					tbottom = ttop+ystep*hscale;
				}
				int xstep = right-left;
				int xo = x+left;
				float tright2 = tright;
				while(xstep > 0)
				{
					if(p.flags&0x01 && xo+xstep-(x+left) > gapw)
					{
						xstep = gapw+x+left-xo;
						tright = tleft+xstep*wscale;
					}
					if(!quads) { quads = true; glBegin(GL_QUADS); }
					glTexCoord2f(tleft,  ttop);	glVertex2i(xo,		yo);
					glTexCoord2f(tright, ttop);	glVertex2i(xo+xstep, yo);
					glTexCoord2f(tright, tbottom); glVertex2i(xo+xstep, yo+ystep);
					glTexCoord2f(tleft,  tbottom); glVertex2i(xo,		yo+ystep);
					xtraverts += 4;
					if(!(p.flags&0x01)) break;
					xo += xstep;
				}
				tright = tright2;
				if(!(p.flags&0x10)) break;
				yo += ystep;
			}
		}
		if(quads) glEnd();
	}

	vec origin, scale;
	float dist;
	g3d_callback *cb;

	static float basescale;
	static bool passthrough;
	static vec light;

	void adjustscale()
	{
		float aspect = float(screen->h)/float(screen->w), fit = 1.0f;
		int w = xsize + (skinx[2]-skinx[1])*SKIN_SCALE + (skinx[10]-skinx[9])*SKIN_SCALE, h = ysize + (skiny[8]-skiny[6])*SKIN_SCALE;
		if(tcurrent) h += ((skiny[5]-skiny[1])-(skiny[3]-skiny[2]))*SKIN_SCALE + FONTH-2*INSERT;
		else h += (skiny[5]-skiny[3])*SKIN_SCALE;

		if(w*aspect*basescale>1) fit = 1.0f/(w*aspect*basescale);
		if(h*basescale*fit>1) fit *= 1.0f/(h*basescale*fit);
		origin = vec(0.5f-((w-xsize)/2 - (skinx[2]-skinx[1])*SKIN_SCALE)*aspect*scale.x*fit, 0.5f + (0.5f*h-(skiny[8]-skiny[6])*SKIN_SCALE)*scale.y*fit, 0);
		scale = vec(aspect*scale.x*fit, scale.y*fit, 1);
	}

	void start(int starttime, float initscale, int *tab, bool allowinput)
	{
		initscale *= 0.025f;
		if(allowinput) hascursor = true;
		basescale = initscale;
		if(layoutpass) scale.x = scale.y = scale.z = basescale*min((totalmillis-starttime)/300.0f, 1.0f);
		passthrough = scale.x<basescale || !allowinput;
		curdepth = -1;
		curlist = -1;
		tpos = 0;
		tx = 0;
		ty = 0;
		tcurrent = tab;
		tcolor = 0xFFFFFF;
		pushlist();
		if(layoutpass) nextlist = curlist;
		else
		{
			if(tcurrent && !*tcurrent) tcurrent = NULL;
			cury = -ysize;
			curx = -xsize/2;

			glPushMatrix();
			glTranslatef(origin.x, origin.y, origin.z);
			glScalef(scale.x, scale.y, scale.z);
			light = vec(1, 1, 1);

			skin_(curx-skinx[2]*SKIN_SCALE, cury-skiny[5]*SKIN_SCALE, xsize, ysize, 0, 9);
			if(!tcurrent) skin_(curx-skinx[5]*SKIN_SCALE, cury-skiny[5]*SKIN_SCALE, xsize, 0, 9, 1);
		}
	}

	void end()
	{
		if(layoutpass)
		{
			xsize = max(tx, xsize);
			ysize = max(ty, ysize);
			ysize = max(ysize, (skiny[6]-skiny[5])*SKIN_SCALE);

			if(tcurrent) *tcurrent = max(1, min(*tcurrent, tpos));
			adjustscale();

			if(!windowhit)
			{
				int intersects = INTERSECT_MIDDLE;

				hitx = (cursorx - origin.x)/scale.x;
				hity = (cursory - origin.y)/scale.y;

				if(intersects>=INTERSECT_MIDDLE && hitx>=-xsize/2 && hitx<=xsize/2 && hity<=0)
				{
					if(hity>=-ysize || (tcurrent && hity>=-ysize-(FONTH-2*INSERT)-((skiny[5]-skiny[1])-(skiny[3]-skiny[2]))*SKIN_SCALE && hitx<=tx-xsize/2))
						windowhit = this;
				}
			}
		}
		else
		{
			if(tcurrent && tx<xsize) skin_(curx+tx-skinx[5]*SKIN_SCALE, -ysize-skiny[5]*SKIN_SCALE, xsize-tx, FONTH, 9, 1);
			glPopMatrix();
		}
		poplist();
	}
};

Texture *gui::skintex = NULL, *gui::overlaytex = NULL, *gui::slidertex = NULL;

//chop skin into a grid
const int gui::skiny[] = {0, 7, 21, 34, 48, 56, 104, 111, 116, 128},
		  gui::skinx[] = {0, 11, 23, 37, 105, 119, 137, 151, 215, 229, 245, 256};
//Note: skinx[3]-skinx[2] = skinx[7]-skinx[6]
//	  skinx[5]-skinx[4] = skinx[9]-skinx[8]
const gui::patch gui::patches[] =
{ //arguably this data can be compressed - it depends on what else needs to be skinned in the future
	{1,2,3,5,  0},	// body
	{2,9,4,5,  0x01},
	{9,10,3,5, 0},

	{1,2,5,6,  0x10},
	{2,9,5,6,  0x11},
	{9,10,5,6, 0x10},

	{1,2,6,8,  0},
	{2,9,6,8,  0x01},
	{9,10,6,8, 0},

	{5,6,3,4, 0x01}, // top

	{2,3,1,2, 0},	// selected tab
	{3,4,1,2, 0x01},
	{4,5,1,2, 0},
	{2,3,2,3, 0x10},
	{3,4,2,3, 0x11},
	{4,5,2,3, 0x10},
	{2,3,3,4, 0},
	{3,4,3,4, 0x01},
	{4,5,3,4, 0},

	{6,7,1,2, 0},	// deselected tab
	{7,8,1,2, 0x01},
	{8,9,1,2, 0},
	{6,7,2,3, 0x10},
	{7,8,2,3, 0x11},
	{8,9,2,3, 0x10},
	{6,7,3,4, 0},
	{7,8,3,4, 0x01},
	{8,9,3,4, 0},
};

vector<gui::list> gui::lists;
float gui::basescale, gui::hitx, gui::hity;
bool gui::passthrough;
vec gui::light;
int gui::curdepth, gui::curlist, gui::xsize, gui::ysize, gui::curx, gui::cury;
int gui::ty, gui::tx, gui::tpos, *gui::tcurrent, gui::tcolor;
static vector<gui> guis;
void g3d_cursorpos(float &x, float &y)
{
	x = cursorx;
	y = cursory;
}

void g3d_resetcursor()
{
	cursorx = cursory = 0.5f;
}

bool g3d_movecursor(int dx, int dy)
{
	if(!hascursor) return false;
	const float CURSORSCALE = 500.0f;
    cursorx = max(0.0f, min(1.0f, cursorx+dx/CURSORSCALE));
    cursory = max(0.0f, min(1.0f, cursory+dy/CURSORSCALE));
	return true;
}

void g3d_addgui(g3d_callback *cb)
{
	gui &g = guis.add();
	g.cb = cb;
	g.origin = camera1->o.add(vec(vec(camera1->vel).normalize()).mul(2.f));
	g.dist = camera1->o.dist(g.origin);
}

int g3d_sort(gui *a, gui *b) { return (int)(a->dist>b->dist)*2-1; }

bool g3d_windowhit(int code, bool on, bool act)
{
	if (act)
	{
		actionon = on;
		mousebuttons |= actionon ? G3D_DOWN : G3D_UP;
		if (code == -2) mousebuttons |= G3D_ALTERNATE;
	}
	else if (code == -3 && !on && windowhit) cleargui(1);
	return hascursor;
}

char *g3d_fieldname()
{
	return fieldname;
}

void g3d_render()
{
	windowhit = NULL;
	if(actionon) mousebuttons |= G3D_PRESSED;
	gui::reset();
	guis.setsize(0);

	// call all places in the engine that may want to render a gui from here, they call g3d_addgui()
	extern void g3d_texturemenu();

	g3d_texturemenu();
	g3d_mainmenu();
	cl->g3d_gamemenus();

	guis.sort(g3d_sort);

	bool fieldfocus = (fieldpos>=0);
	fieldactive = false;
	hascursor = false;

	layoutpass = true;
	loopv(guis) guis[i].cb->gui(guis[i], true);
	layoutpass = false;

	if(guis.length())
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, 1, 1, 0, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

        glDisable(GL_DEPTH_TEST);

		loopvrev(guis) guis[i].cb->gui(guis[i], false);

        glEnable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

        glDisable(GL_BLEND);
	}

	if(!fieldactive) fieldpos = -1; //no hit fields, so loose focus - mainly for menu closed
	if((fieldpos>=0) != fieldfocus) SDL_EnableUNICODE(fieldpos>=0);

	mousebuttons = 0;
}
