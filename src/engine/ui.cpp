#include "engine.h"

int uimillis = -1;

static bool layoutpass, actionon = false;
static int mousebuttons = 0, guibound[2] = {0};

static float firstx, firsty;

enum {FIELDCOMMIT, FIELDABORT, FIELDEDIT, FIELDSHOW, FIELDKEY};
static int fieldmode = FIELDSHOW;
static bool fieldsactive = false;

VARP(guishadow, 0, 2, 8);
VARP(guiautotab, 6, 16, 40);
VARP(guiclicktab, 0, 1, 1);

static bool needsinput = false;

#include "textedit.h"
struct gui : guient
{
	struct list { int parent, w, h; };

	int nextlist;
	static vector<list> lists;
	static float hitx, hity;
	static int curdepth, curlist, xsize, ysize, curx, cury, fontdepth;
    static bool shouldmergehits, shouldautotab;

	static void reset() { lists.setsize(0); }

	static int ty, tx, tpos, *tcurrent, tcolor; //tracking tab size and position since uses different layout method...

    void allowautotab(bool on) { shouldautotab = on; }

	void autotab()
	{
		if(tcurrent)
		{
			if(layoutpass && !tpos) tcurrent = NULL; //disable tabs because you didn't start with one
            if(shouldautotab && !curdepth && (layoutpass ? 0 : cury) + ysize > guiautotab*guibound[1]) tab(NULL, tcolor);
		}
	}

    bool shouldtab()
    {
        if(tcurrent && shouldautotab)
        {
            if(layoutpass)
            {
                int space = guiautotab*guibound[1] - ysize;
                if(space < 0) return true;
                int l = lists[curlist].parent;
                while(l >= 0)
                {
                    space -= lists[l].h;
                    if(space < 0) return true;
                    l = lists[l].parent;
                }
            }
            else
            {
                int space = guiautotab*guibound[1] - cury;
                if(ysize > space) return true;
                int l = lists[curlist].parent;
                while(l >= 0)
                {
                    if(lists[l].h > space) return true;
                    l = lists[l].parent;
                }
            }
        }
        return false;
    }

	bool visible() { return (!tcurrent || tpos==*tcurrent) && !layoutpass; }

	//tab is always at top of page
	void tab(const char *name, int color)
	{
		if(curdepth != 0) return;
        if(color) tcolor = color;
		tpos++;
		if(!name)
		{
			static string title;
			formatstring(title)("%d", tpos);
			name = title;
		}
		gui::pushfont(visible() ? "super" : "emphasis");
		int w = text_width(name);
		if(layoutpass)
		{
			ty = max(ty, ysize);
			ysize = 0;
		}
		else
		{
			cury = -ysize;
			int x1 = curx + tx + guibound[0], x2 = x1 + w, y1 = cury - guibound[1]*2, y2 = cury - guibound[1]*2 + FONTH;
			if(visible()) tcolor = 0xFFFFFF;
			else if(tcurrent && hitx>=x1 && hity>=y1 && hitx<x2 && hity<y2)
			{
				if(!guiclicktab || mousebuttons&GUI_UP) *tcurrent = tpos; // switch tab
				color = 0xFFFF00;
			}
            text_(name, x1, y1, tcolor, visible());
		}
		tx += w + guibound[0]*2;
		gui::popfont();
	}

	void uibuttons()
	{
		cury = -ysize;
		int x = curx + max(tx, xsize) - guibound[0]*2, y = cury - guibound[1]*2;
		#define uibtn(a,b) \
		{ \
			bool hit = false; \
			if(hitx>=x && hity>=y && hitx<x+guibound[1] && hity<y+guibound[1]) \
			{ \
				if(mousebuttons&GUI_UP) { b; } \
				hit = true; \
			} \
			icon_(textureload(a, 3, true, false), false, false, x, y, guibound[1], hit); \
			y += guibound[1]*3/2; \
		}
		uibtn("textures/exit", cleargui(1));
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

	int text  (const char *text, int color, const char *icon) { autotab(); return button_(text, color, icon, false); }
	int button(const char *text, int color, const char *icon) { autotab(); return button_(text, color, icon, true); }
	int title (const char *text, int color, const char *icon) { autotab(); return button_(text, color, icon, false, "emphasis"); }

	void separator() { autotab(); line_(5); }

	//use to set min size (useful when you have progress bars)
    void strut(int size) { layout(isvertical() ? size*guibound[0] : 0, isvertical() ? 0 : size*guibound[1]); }
	//add space between list items
    void space(int size) { layout(isvertical() ? 0 : size*guibound[0], isvertical() ? size*guibound[1] : 0); }

    void pushfont(const char *font) { ::pushfont(font); fontdepth++; }
    void popfont() { if(fontdepth) { ::popfont(); fontdepth--; } }

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
			return (hit && visible()) ? mousebuttons|GUI_ROLLOVER : 0;
		}
	}

    void mergehits(bool on) { shouldmergehits = on; }

	bool ishit(int w, int h, int x = curx, int y = cury)
	{
        if(shouldmergehits) return ishorizontal() ? hitx>=x && hitx<x+w : hity>=y && hity<y+h;
		if(ishorizontal()) h = ysize;
		else w = xsize;
		return hitx>=x && hity>=y && hitx<x+w && hity<y+h;
	}

    int image(Texture *t, float scale, bool overlaid)
	{
		autotab();
		if(scale == 0) scale = 1;
		int size = (int)(scale*2*guibound[1])-guishadow;
		if(visible()) icon_(t, overlaid, false, curx, cury, size, ishit(size+guishadow, size+guishadow));
		return layout(size+guishadow, size+guishadow);
	}

    int texture(Texture *t, float scale, int rotate, int xoff, int yoff, Texture *glowtex, const vec &glowcolor, Texture *layertex)
    {
        autotab();
        if(scale == 0) scale = 1;
        int size = (int)(scale*2*guibound[1])-guishadow;
        if(t!=notexture && visible()) icon_(t, true, true, curx, cury, size, ishit(size+guishadow, size+guishadow), rotate, xoff, yoff, glowtex, glowcolor, layertex);
        return layout(size+guishadow, size+guishadow);
    }

    int slice(Texture *t, float scale, float start, float end, const char *text)
    {
        autotab();
        if(scale == 0) scale = 1;
        int size = (int)(scale*2*guibound[1]);
        if(t!=notexture && visible()) slice_(t, curx, cury, size, start, end, text);
        return layout(size, size);
    }

	void progress(float percent, float scale)
	{
		autotab();
		string s; if(percent > 0) formatstring(s)("\fg%d%%", int(percent*100)); else formatstring(s)("\fgload");
		slice(textureload("textures/progress", 3, true, false), scale, 0, percent, s);
	}

	void slider(int &val, int vmin, int vmax, int color, char *label, bool reverse)
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
				formatstring(s)("%d", val);
				label = s;
			}
			int w = text_width(label);

			bool hit;
			int px, py;
			if(ishorizontal())
			{
				hit = ishit(guibound[0], ysize, x, y);
				px = x + (guibound[0]-w)/2;
                if(reverse) py = y + ((ysize-guibound[1])*(val-vmin))/((vmax==vmin) ? 1 : (vmax-vmin)); //vmin at top
                else py = y + (ysize-guibound[1]) - ((ysize-guibound[1])*(val-vmin))/((vmax==vmin) ? 1 : (vmax-vmin)); //vmin at bottom
			}
			else
			{
				hit = ishit(xsize, guibound[1], x, y);
                if(reverse) px = x + (xsize-guibound[0]/2-w/2) - ((xsize-w)*(val-vmin))/((vmax==vmin) ? 1 : (vmax-vmin)); //vmin at right
                else px = x + guibound[0]/2 - w/2 + ((xsize-w)*(val-vmin))/((vmax==vmin) ? 1 : (vmax-vmin)); //vmin at left
				py = y;
			}

			if(hit) color = 0xFF0000;
			text_(label, px, py, color, hit && actionon);
			if(hit && actionon)
			{
                int vnew = (vmin < vmax ? 1 : -1)+vmax-vmin;
                if(ishorizontal()) vnew = reverse ? int(vnew*(hity-y-guibound[1]/2)/(ysize-guibound[1])) : int(vnew*(y+ysize-guibound[1]/2-hity)/(ysize-guibound[1]));
                else vnew = reverse ? int(vnew*(x+xsize-guibound[0]/2-hitx)/(xsize-w)) : int(vnew*(hitx-x-guibound[0]/2)/(xsize-w));
				vnew += vmin;
                vnew = vmin < vmax ? clamp(vnew, vmin, vmax) : clamp(vnew, vmax, vmin);
				if(vnew != val) val = vnew;
			}
		}
	}

    char *field(const char *name, int color, int length, int height, const char *initval, int initmode)
    {
        return field_(name, color, length, height, initval, initmode, FIELDEDIT, "console");
    }

    char *keyfield(const char *name, int color, int length, int height, const char *initval, int initmode)
    {
        return field_(name, color, length, height, initval, initmode, FIELDKEY, "console");
    }

    char *field_(const char *name, int color, int length, int height, const char *initval, int initmode, int fieldtype = FIELDEDIT, const char *font = "")
	{
		if(font && *font) gui::pushfont(font);
        editor *e = useeditor(name, initmode, false, initval); // generate a new editor if necessary
        if(layoutpass)
        {
            if(initval && e->mode==EDITORFOCUSED && (e!=currentfocus() || fieldmode == FIELDSHOW))
            {
                if(strcmp(e->lines[0].text, initval)) e->clear(initval);
            }
            e->linewrap = (length<0);
            e->maxx = (e->linewrap) ? -1 : length;
            e->maxy = (height<=0)?1:-1;
            e->pixelwidth = abs(length)*guibound[0];
            if(e->linewrap && e->maxy==1)
            {
	            int temp;
                text_bounds(e->lines[0].text, temp, e->pixelheight, e->pixelwidth); //only single line editors can have variable height
            }
            else
                e->pixelheight = guibound[1]*max(height, 1);
        }
        int h = e->pixelheight;
        int w = e->pixelwidth + guibound[0];

        bool wasvertical = isvertical();
        if(wasvertical && e->maxy != 1) pushlist();

		char *result = NULL;
        if(visible())
		{
            bool hit = ishit(w, h);
            bool editing = (fieldmode != FIELDSHOW) && (e==currentfocus());
			if(mousebuttons&GUI_DOWN) //mouse request focus
			{
	            if(hit)
	            {
					if(fieldtype==FIELDKEY) e->clear();
					useeditor(e->name, initmode, true);
					e->mark(false);
					fieldmode = fieldtype;
	            }
				else if(editing)
				{
					fieldmode = FIELDCOMMIT;
					e->mode = EDITORFOCUSED;
				}
			}
            if(hit && editing && (mousebuttons&GUI_PRESSED)!=0 && fieldtype==FIELDEDIT)
				e->hit(int(floor(hitx-(curx+guibound[0]/2))), int(floor(hity-cury)), (mousebuttons&GUI_DRAGGED)!=0); //mouse request position
            if(editing && (fieldmode==FIELDCOMMIT || fieldmode==FIELDABORT)) // commit field if user pressed enter
            {
                if(fieldmode==FIELDCOMMIT) result = e->currentline().text;
				e->active = (e->mode!=EDITORFOCUSED);
                fieldmode = FIELDSHOW;
			}
            else fieldsactive = true;

            e->draw(curx+guibound[0]/2, cury, color, editing);

			notextureshader->set();
			glDisable(GL_TEXTURE_2D);
			if(editing) glColor3f(1, 0, 0);
			else glColor3ub(color>>16, (color>>8)&0xFF, color&0xFF);
			glBegin(GL_LINE_LOOP);
            rect_(curx, cury, w, h);
			glEnd();
			glEnable(GL_TEXTURE_2D);
			defaultshader->set();
		}
    	layout(w, h);
        if(e->maxy != 1)
        {
            int slines = e->lines.length()-e->pixelheight/guibound[1];
            if(slines > 0)
            {
                int pos = e->scrolly;
                slider(e->scrolly, slines, 0, color, NULL, false);
                if(pos != e->scrolly) e->cy = e->scrolly;
            }
            if(wasvertical) poplist();
        }
        if(font && *font) gui::popfont();
		return result;
	}

    void fieldline(const char *name, const char *str)
	{
        if(!layoutpass) return;
        loopv(editors) if(strcmp(editors[i]->name, name) == 0)
		{
			editor *e = editors[i];
			e->lines.add().set(str);
			e->mark(false);
            return;
		}
	}

	void fieldclear(const char *name, const char *init)
	{
        if(!layoutpass) return;
        loopvrev(editors) if(strcmp(editors[i]->name, name) == 0)
		{
			editor *e = editors[i];
			e->clear(init);
			return;
		}
	}

	int fieldedit(const char *name)
	{
		loopvrev(editors) if(strcmp(editors[i]->name, name) == 0)
		{
			editor *e = editors[i];
			useeditor(e->name, e->mode, true);
			e->mark(false);
			e->cx = e->cy = 0;
			fieldmode = FIELDEDIT;
			return fieldmode;
		}
        return fieldmode;
	}

	void fieldscroll(const char *name, int n)
	{
		if(n < 0 && mousebuttons&GUI_PRESSED) return; // don't auto scroll during edits
        if(!layoutpass) return;
        loopv(editors) if(strcmp(editors[i]->name, name) == 0)
		{
			editor *e = editors[i];
			e->scrolly = e->cx = 0;
			e->cy = n >= 0 ? n : e->lines.length();
			return;
		}
	}

	void rect_(float x, float y, float w, float h, int usetc = -1)
	{
        static const GLfloat tc[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
        if(usetc>=0) glTexCoord2fv(tc[usetc]);
		glVertex2f(x, y);
        if(usetc>=0) glTexCoord2fv(tc[(usetc+1)%4]);
		glVertex2f(x + w, y);
        if(usetc>=0) glTexCoord2fv(tc[(usetc+2)%4]);
		glVertex2f(x + w, y + h);
        if(usetc>=0) glTexCoord2fv(tc[(usetc+3)%4]);
		glVertex2f(x, y + h);
		xtraverts += 4;
	}

	void text_(const char *text, int x, int y, int color, bool shadow)
	{
		if(shadow) draw_text(text, x+guishadow, y+guishadow, 0x00, 0x00, 0x00, 0xC0);
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

    void icon_(Texture *t, bool overlaid, bool tiled, int x, int y, int size, bool hit, int rotate = 0, int xoff = 0, int yoff = 0, Texture *glowtex = NULL, const vec &glowcolor = vec(1, 1, 1), Texture *layertex = NULL)
	{
		float xr = 0, yr = 0, xs = 0, ys = 0, xt = 0, yt = 0;
		if(tiled)
		{
			static Shader *rgbonlyshader = NULL;
			if(!rgbonlyshader) rgbonlyshader = lookupshaderbyname("rgbonly");
			rgbonlyshader->set();
		}
		if(t)
		{
			if(tiled)
			{
				xt = min(1.0f, t->xs/(float)t->ys),
				yt = min(1.0f, t->ys/(float)t->xs);
				xr = t->xs; yr = t->ys;
				xs = size; ys = size;
			}
			else
			{
				xt = 1.0f; yt = 1.0f;
				float scale = float(size)/max(t->xs, t->ys); //scale and preserve aspect ratio
				xr = t->xs; yr = t->ys;
				xs = t->xs*scale; ys = t->ys*scale;
				x += int((size-xs)/2);
				y += int((size-ys)/2);
			}
			glBindTexture(GL_TEXTURE_2D, t->id);
		}
		else
		{
			extern GLuint lmprogtex;
			if(lmprogtex)
			{
				xt = 1.0f; yt = 1.0f;
				float scale = float(size)/256; //scale and preserve aspect ratio
				xr = 256; yr = 256;
				xs = 256*scale; ys = 256*scale;
				x += int((size-xs)/2);
				y += int((size-ys)/2);
				glBindTexture(GL_TEXTURE_2D, lmprogtex);
			}
			else
			{
				if((t = textureload(mapname, 3, true, false)) == notexture) t = textureload("textures/nothumb", 3, true, false);
				xt = 1.0f; yt = 1.0f;
				float scale = float(size)/max(t->xs, t->ys); //scale and preserve aspect ratio
				xr = t->xs; yr = t->ys;
				xs = t->xs*scale; ys = t->ys*scale;
				x += int((size-xs)/2);
				y += int((size-ys)/2);
				glBindTexture(GL_TEXTURE_2D, t->id);
			}
		}
        float xi = x, yi = y, xpad = 0, ypad = 0;
        if(overlaid)
        {
            xpad = xs/32;
            ypad = ys/32;
            xi += xpad;
            yi += ypad;
            xs -= 2*xpad;
            ys -= 2*ypad;
        }
        float tc[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
        if(rotate)
        {
            if((rotate&5) == 1) { swap(xoff, yoff); loopk(4) swap(tc[k][0], tc[k][1]); }
            if(rotate >= 2 && rotate <= 4) { xoff *= -1; loopk(4) tc[k][0] *= -1; }
            if(rotate <= 2 || rotate == 5) { yoff *= -1; loopk(4) tc[k][1] *= -1; }
        }
        loopk(4) { tc[k][0] = tc[k][0]/xt - float(xoff)/xr; tc[k][1] = tc[k][1]/yt - float(yoff)/yr; }
        vec color = hit && !overlaid ? vec(0.5f, 0.5f, 0.5f) : vec(1, 1, 1);
        glColor3fv(color.v);
        glBegin(GL_QUADS);
        glTexCoord2fv(tc[0]); glVertex2f(xi,    yi);
        glTexCoord2fv(tc[1]); glVertex2f(xi+xs, yi);
        glTexCoord2fv(tc[2]); glVertex2f(xi+xs, yi+ys);
        glTexCoord2fv(tc[3]); glVertex2f(xi,    yi+ys);
        glEnd();
        if(glowtex)
        {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glBindTexture(GL_TEXTURE_2D, glowtex->id);
            if(hit || overlaid) { loopk(3) color[k] *= glowcolor[k]; glColor3fv(color.v); }
            else glColor3fv(glowcolor.v);
            glBegin(GL_QUADS);
            glTexCoord2fv(tc[0]); glVertex2f(xi,    yi);
            glTexCoord2fv(tc[1]); glVertex2f(xi+xs, yi);
            glTexCoord2fv(tc[2]); glVertex2f(xi+xs, yi+ys);
            glTexCoord2fv(tc[3]); glVertex2f(xi,    yi+ys);
            glEnd();
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        if(layertex)
        {
            glBindTexture(GL_TEXTURE_2D, layertex->id);
            glColor3fv(color.v);
            glBegin(GL_QUADS);
            glTexCoord2fv(tc[0]); glVertex2f(xi+xs/2, yi+ys/2);
            glTexCoord2fv(tc[1]); glVertex2f(xi+xs,   yi+ys/2);
            glTexCoord2fv(tc[2]); glVertex2f(xi+xs,   yi+ys);
            glTexCoord2fv(tc[3]); glVertex2f(xi+xs/2, yi+ys);
            glEnd();
        }

		if(tiled) defaultshader->set();
		if(overlaid)
		{
			if(!overlaytex) overlaytex = textureload(guioverlaytex, 3, true, false);
			color = hit ? vec(0.25f, 0.25f, 0.25f) : vec(1, 1, 1);
			glColor3fv(color.v);
			glBindTexture(GL_TEXTURE_2D, overlaytex->id);
            glColor3f(1, 1, 1);
			glBegin(GL_QUADS);
			rect_(xi - xpad, yi - ypad, xs + 2*xpad, ys + 2*ypad, 0);
			glEnd();
		}
	}

    void slice_(Texture *t, int x, int y, int size, float start = 0, float end = 1, const char *text = NULL)
	{
		float scale = float(size)/max(t->xs, t->ys), xs = t->xs*scale, ys = t->ys*scale, fade = 1;
		if(start == end) { end = 1; fade = 0.5f; }
        glBindTexture(GL_TEXTURE_2D, t->id);
        glColor4f(1, 1, 1, fade);
        int s = max(xs,ys)/2;
		drawslice(start, end, x+s/2, y+s/2, s);
		if(text && *text)
		{
			int w = text_width(text);
			text_(text, x+s/2-w/2, y+s/2-FONTH/2, 0xFFFFFF, false);
		}
	}

	void line_(int size, float percent = 1.0f)
	{
		if(visible())
		{
			if(!slidertex) slidertex = textureload(guislidertex, 3, true, false);
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, slidertex->id);
			glBegin(GL_QUADS);
			if(percent < 0.99f)
			{
				glColor4f(1, 1, 1, 0.375f);
				if(ishorizontal())
					rect_(curx + guibound[0]/2 - size, cury, size*2, ysize, 0);
				else
					rect_(curx, cury + guibound[0]/2 - size, xsize, size*2, 1);
			}
			glColor3f(1, 1, 1);
			if(ishorizontal())
				rect_(curx + guibound[0]/2 - size, cury + ysize*(1-percent), size*2, ysize*percent, 0);
			else
				rect_(curx, cury + guibound[0]/2 - size, xsize*percent, size*2, 1);
			glEnd();
		}
		layout(ishorizontal() ? guibound[0] : 0, ishorizontal() ? 0 : guibound[1]);
	}

	int button_(const char *text, int color, const char *icon, bool clickable, const char *font = "")
	{
		const int padding = 10;
		if(font && *font) gui::pushfont(font);
		int w = 0, h = max((int)FONTH, guibound[1]);
		if(icon) w += guibound[1];
		if(icon && text) w += padding;
		if(text) w += text_width(text);

		if(visible())
		{
			bool hit = ishit(w, FONTH);
			if(hit && clickable) color = 0xFF0000;
			int x = curx;
			if(icon)
			{
				defformatstring(tname)("textures/%s", icon);
				icon_(textureload(tname, 3, true, false), false, false, x, cury, guibound[1], clickable && hit);
				x += guibound[1];
			}
			if(icon && text) x += padding;
			if(text) text_(text, x, cury, color, hit && clickable && actionon);
		}
		if(font && *font) gui::popfont();
		return layout(w, h);
	}

	static Texture *overlaytex, *slidertex;

    vec origin, scale;
	guicb *cb;

    static float basescale, maxscale;
	static bool passthrough;

	void adjustscale()
	{
		int w = xsize + guibound[0]*2, h = ysize + guibound[1]*2;
        float aspect = float(screen->h)/float(screen->w), fit = 1.0f;
        if(w*aspect*basescale>1.0f) fit = 1.0f/(w*aspect*basescale);
        if(h*basescale*fit>maxscale) fit *= maxscale/(h*basescale*fit);
		origin = vec(0.5f-((w-xsize)/2 - guibound[0])*aspect*scale.x*fit, 0.5f + (0.5f*h-guibound[1])*scale.y*fit, 0);
		scale = vec(aspect*scale.x*fit, scale.y*fit, 1);
	}

	void start(int starttime, float initscale, int *tab, bool allowinput)
	{
		initscale *= 0.025f;
		basescale = initscale;
        if(layoutpass) scale.x = scale.y = scale.z = basescale; //min(basescale*(totalmillis-starttime)/300.0f, basescale);
        if(allowinput) needsinput = true;
        passthrough = !allowinput;
        fontdepth = 0;
        gui::pushfont("sub");
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
		}
	}

	void end()
	{
		if(layoutpass)
		{
			xsize = max(tx, xsize);
			ysize = max(ty, ysize);
			ysize = max(ysize, guibound[1]);

			if(tcurrent) *tcurrent = max(1, min(*tcurrent, tpos));
			adjustscale();

			if(!passthrough)
			{
				hitx = (cursorx - origin.x)/scale.x;
				hity = (cursory - origin.y)/scale.y;
                if((mousebuttons & GUI_PRESSED) && (fabs(hitx-firstx) > 2 || fabs(hity - firsty) > 2)) mousebuttons |= GUI_DRAGGED;
			}
		}
		else
		{
			if(needsinput) uibuttons();
			glPopMatrix();
		}
		poplist();
		while(fontdepth) gui::popfont();
	}
};

Texture *gui::overlaytex = NULL, *gui::slidertex = NULL;
TVARN(guioverlaytex, "textures/guioverlay", gui::overlaytex, 0);
TVARN(guislidertex, "textures/guislider", gui::slidertex, 0);

vector<gui::list> gui::lists;
float gui::basescale, gui::maxscale = 1, gui::hitx, gui::hity;
bool gui::passthrough, gui::shouldmergehits = false, gui::shouldautotab = true;
int gui::curdepth, gui::fontdepth, gui::curlist, gui::xsize, gui::ysize, gui::curx, gui::cury;
int gui::ty, gui::tx, gui::tpos, *gui::tcurrent, gui::tcolor;
static vector<gui> guis;

namespace UI
{
	bool isopen = false, ready = false;

	void setup()
	{
		const char *fonts[2] = { "sub", "console" };
		loopi(2)
		{
			pushfont(fonts[i]);
			loopk(2) if((k ? FONTH : FONTW) > guibound[k]) guibound[k] = (k ? FONTH : FONTW);
			popfont();
		}
		ready = true;
	}

	bool keypress(int code, bool isdown, int cooked)
	{
		editor *e = currentfocus();
		if(fieldmode == FIELDKEY)
		{
			switch(code)
			{
				case SDLK_ESCAPE:
					if(isdown) fieldmode = FIELDCOMMIT;
					return true;
			}
			const char *keyname = getkeyname(code);
			if(keyname && isdown)
			{
				if(e->lines.length()!=1 || !e->lines[0].empty()) e->insert(" ");
				e->insert(keyname);
			}
			return true;
		}

		if(code<0) switch(code)
		{ // fall-through-o-rama
			case -3:
				mousebuttons |= GUI_ALT;
			case -1:
				if(isdown) { firstx = gui::hitx; firsty = gui::hity; }
				mousebuttons |= (actionon=isdown) ? GUI_DOWN : GUI_UP;
				if(active()) return true;
				break;
			case -2:
				cleargui(1);
				if(active()) return true;
			default: break;
		}

		if(fieldmode == FIELDSHOW || !e) return false;
		switch(code)
		{
			case SDLK_ESCAPE: //cancel editing without commit
				if(isdown) fieldmode = FIELDABORT;
				return true;
			case SDLK_RETURN:
			case SDLK_TAB:
				if(cooked && (e->maxy != 1)) break;
			case SDLK_KP_ENTER:
				if(isdown) fieldmode = FIELDCOMMIT; //signal field commit (handled when drawing field)
				return true;
			case SDLK_HOME:
			case SDLK_END:
			case SDLK_PAGEUP:
			case SDLK_PAGEDOWN:
			case SDLK_DELETE:
			case SDLK_BACKSPACE:
			case SDLK_UP:
			case SDLK_DOWN:
			case SDLK_LEFT:
			case SDLK_RIGHT:
			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
			case -4:
			case -5:
				break;
			default:
				if(!cooked || (code<32)) return false;
		}
		if(!isdown) return true;
		e->key(code, cooked);
		return true;
	}

	bool active(bool pass) { return guis.length() && (!pass || needsinput); }
	bool hascursor(bool targeting) { return (!targeting && commandmillis > 0) || active(targeting); }
	void limitscale(float scale) {  gui::maxscale = scale; }

	void addcb(guicb *cb)
	{
		gui &g = guis.add();
		g.cb = cb;
		g.adjustscale();
	}

	void update()
	{
		bool p = active(false);
		if(isopen != p) uimillis = (isopen = p) ? lastmillis : -lastmillis;
	}

	void render()
	{
		if(actionon) mousebuttons |= GUI_PRESSED;
		gui::reset(); guis.setsize(0);

		// call all places in the engine that may want to render a gui from here, they call addcb()
		if(progressing) progressmenu();
		else
		{
			texturemenu();
			hud::gamemenus();
			mainmenu();
		}

		readyeditors();
		bool wasfocused = (fieldmode!=FIELDSHOW);
		fieldsactive = false;

		needsinput = false;

		if(!guis.empty())
		{
			layoutpass = true;
			//loopv(guis) guis[i].cb->gui(guis[i], true);
			guis.last().cb->gui(guis.last(), true);
			layoutpass = false;

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
			glLoadIdentity();
			glOrtho(0, 1, 1, 0, -1, 1);

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glLoadIdentity();

			//loopvrev(guis) guis[i].cb->gui(guis[i], false);
			guis.last().cb->gui(guis.last(), false);

			glMatrixMode(GL_PROJECTION);
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();

			glDisable(GL_BLEND);
		}

		flusheditors();
		if(!fieldsactive) fieldmode = FIELDSHOW; //didn't draw any fields, so loose focus - mainly for menu closed
		if((fieldmode!=FIELDSHOW) != wasfocused)
		{
			SDL_EnableUNICODE(fieldmode!=FIELDSHOW);
			keyrepeat(fieldmode!=FIELDSHOW);
		}

		mousebuttons = 0;
	}
};
