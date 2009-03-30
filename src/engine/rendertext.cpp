#include "engine.h"

static hashtable<const char *, font> fonts;
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

	f->tex = textureload(tex, 3);
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

bool setfont(const char *name)
{
	font *f = fonts.access(name);
	if(!f)
	{
		s_sprintfd(n)("fonts/%s.cfg", name);
		if(!execfile(n) || !(f = fonts.access(name)))
			return false;
	}
	curfont = f;
	return true;
}

int text_width(const char *str, int flags) { //@TODO deprecate in favour of text_bounds(..)
    int width, height;
    text_bounds(str, width, height, -1, flags);
    return width;
}

void tabify(const char *str, int *numtabs)
{
    vector<char> tabbed;
    tabbed.put(str, strlen(str));
    int w = text_width(str), tw = max(*numtabs, 0)*PIXELTAB;
    while(w < tw)
    {
        tabbed.add('\t');
        w = ((w+PIXELTAB)/PIXELTAB)*PIXELTAB;
    }
    tabbed.add('\0');
    result(tabbed.getbuf());
}

COMMAND(tabify, "si");

int draw_textf(const char *fstr, int left, int top, ...)
{
	s_sprintfdlv(str, top, fstr);
	return draw_text(str, left, top);
}

static int draw_char(int c, int x, int y)
{
    font::charinfo &info = curfont->chars[c-33];
    float tc_left    = (info.x + curfont->offsetx) / float(curfont->tex->xs);
    float tc_top     = (info.y + curfont->offsety) / float(curfont->tex->ys);
    float tc_right   = (info.x + info.w + curfont->offsetw) / float(curfont->tex->xs);
    float tc_bottom  = (info.y + info.h + curfont->offseth) / float(curfont->tex->ys);

    glTexCoord2f(tc_left,  tc_top   ); glVertex2f(x,          y);
    glTexCoord2f(tc_right, tc_top   ); glVertex2f(x + info.w, y);
    glTexCoord2f(tc_right, tc_bottom); glVertex2f(x + info.w, y + info.h);
    glTexCoord2f(tc_left,  tc_bottom); glVertex2f(x,          y + info.h);

    xtraverts += 4;
    return info.w;
}

//stack[sp] is current color index
SVARP(savecolour, "\fs");
SVARP(restorecolour, "\fS");
SVARP(green, "\fg");
SVARP(blue, "\fb");
SVARP(yellow, "\fy");
SVARP(red, "\fr");
SVARP(gray, "\fa");
SVARP(magenta, "\fm");
SVARP(orange, "\fo");
SVARP(white, "\fw");
SVARP(black, "\fk");
SVARP(cyan, "\fc");
SVARP(violet, "\fv");
SVARP(purple, "\fp");
SVARP(brown, "\fn");
SVARP(defcolour, "\fu");

static void text_color(char c, char *stack, int size, int &sp, bvec color, int r, int g, int b, int a)
{
    if(c=='s') // save color
    {
        c = stack[sp];
        if(sp<size-1) stack[++sp] = c;
    }
    else
    {
        if(c=='S') c = stack[(sp > 0) ? --sp : sp]; // restore color
        else stack[sp] = c;
        switch(c)
        {
            case 'g': case '0': color = bvec( 64, 255,  64); break;	// green
            case 'b': case '1': color = bvec( 64,  64, 255); break;	// blue
            case 'y': case '2': color = bvec(255, 255,   0); break;	// yellow
            case 'r': case '3': color = bvec(255,  64,  64); break;	// red
            case 'a': case '4': color = bvec(196, 196, 196); break;	// gray
            case 'm': case '5': color = bvec(255,  64, 255); break;	// magenta
            case 'o': case '6': color = bvec(255,  96,   0); break;	// orange
            case 'w': case '7': color = bvec(255, 255, 255); break;	// white
            case 'k': case '8': color = bvec(0,     0,   0); break;	// black
            case 'c': case '9': color = bvec(64,  255, 255); break;	// cyan
            case 'v': case 'A': color = bvec(192,  96, 255); break;	// violet
            case 'p': case 'B': color = bvec(224,  64, 224); break;	// purple
            case 'n': case 'C': color = bvec(120,  72,   0); break; // brown
            case 'u': color = bvec(r, g, b); break;	// user colour
			default: break; // everything else
        }
        glColor4ub(color.x, color.y, color.z, a);
    }
}

#define TEXTTAB(x) clamp(x + (PIXELTAB - (x % PIXELTAB)), x + FONTW, x + PIXELTAB)

#define TEXTSKELETON \
    int y = 0, x = 0;\
    int i;\
    for(i = 0; str[i]; i++)\
    {\
        TEXTINDEX(i)\
        int c = str[i];\
        if(c=='\t')      { x = TEXTTAB(x); TEXTWHITE(i) }\
        else if(c==' ')  { x += curfont->defaultw; TEXTWHITE(i) }\
        else if(c=='\n') { TEXTLINE(i) x = 0; y += FONTH; }\
        else if(c=='\f') { if(str[i+1]) { i++; TEXTCOLOR(i) }}\
        else if(curfont->chars.inrange(c-33))\
        {\
            if(maxwidth != -1)\
            {\
                int j = i;\
                int w = curfont->chars[c-33].w;\
                for(; str[i+1]; i++)\
                {\
                    int c = str[i+1];\
                    if(c=='\f') { if(str[i+2]) i++; continue; }\
                    if(i-j > 16) break;\
                    if(!curfont->chars.inrange(c-33)) break;\
                    int cw = curfont->chars[c-33].w + 1;\
                    if(w + cw >= maxwidth) break;\
                    w += cw;\
                }\
                if(x + w >= maxwidth && j!=0) { TEXTLINE(j-1) x = flags&TEXT_NO_INDENT ? 0 : PIXELTAB; y += FONTH; }\
                TEXTWORD\
            }\
            else\
            { TEXTCHAR(i) }\
        }\
    }

//all the chars are guaranteed to be either drawable or color commands
#define TEXTWORDSKELETON \
                for(; j <= i; j++)\
                {\
                    TEXTINDEX(j)\
                    int c = str[j];\
                    if(c=='\f') { if(str[j+1]) { j++; TEXTCOLOR(j) }}\
                    else { TEXTCHAR(j) }\
                }

int text_visible(const char *str, int hitx, int hity, int maxwidth, int flags)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx) if(y+FONTH > hity && x >= hitx) return idx;
    #define TEXTLINE(idx) if(y+FONTH > hity) return idx;
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += curfont->chars[c-33].w+1; TEXTWHITE(idx)
    #define TEXTWORD TEXTWORDSKELETON
    TEXTSKELETON
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
    return i;
}

//inverse of text_visible
void text_pos(const char *str, int cursor, int &cx, int &cy, int maxwidth, int flags)
{
    #define TEXTINDEX(idx) if(idx == cursor) { cx = x; cy = y; break; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx)
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += curfont->chars[c-33].w + 1;
    #define TEXTWORD TEXTWORDSKELETON if(i >= cursor) break;
    cx = INT_MIN;
    cy = 0;
    TEXTSKELETON
    if(cx == INT_MIN) { cx = x; cy = y; }
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

void text_bounds(const char *str, int &width, int &height, int maxwidth, int flags)
{
    #define TEXTINDEX(idx)
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx) if(x > width) width = x;
    #define TEXTCOLOR(idx)
    #define TEXTCHAR(idx) x += curfont->chars[c-33].w + 1;
    #define TEXTWORD x += w + 1;
    width = 0;
    TEXTSKELETON
    height = y + FONTH;
    TEXTLINE(_)
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
}

int draw_text(const char *str, int rleft, int rtop, int r, int g, int b, int a, int flags, int cursor, int maxwidth)
{
    #define TEXTINDEX(idx) if(idx == cursor) { cx = x; cy = y; }
    #define TEXTWHITE(idx)
    #define TEXTLINE(idx) cy += FONTH;
    #define TEXTCOLOR(idx) text_color(str[idx], colorstack, sizeof(colorstack), colorpos, color, r, g, b, a);
    #define TEXTCHAR(idx) x += draw_char(c, left+x, top+y)+1;
    #define TEXTWORD TEXTWORDSKELETON
    char colorstack[10];
    bvec color(r, g, b);
    int colorpos = 0, cx = INT_MIN, cy = 0, left = rleft, top = rtop;
    loopi(10) colorstack[i] = 'u'; //indicate user color

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, curfont->tex->id);
    glBegin(GL_QUADS);
	loopk(flags&TEXT_SHADOW ? 2 : 1)
	{
		if(flags&TEXT_SHADOW && !k)
		{
			glColor4ub(0, 0, 0, a);
			left = rleft+2;
			top = rtop+2;
		}
		else
		{
			glColor4ub(color.x, color.y, color.z, a);
			left = rleft;
			top = rtop;
		}
		TEXTSKELETON
		if(cursor >= 0)
		{
			float fade = 1.f-(float(lastmillis%1000)/1000.f);
			glColor4ub(color.x, color.y, color.z, int(a*fade));
			if(cx == INT_MIN) { cx = x; cy = y; }
			if(maxwidth != -1 && cx >= maxwidth) { cx = PIXELTAB; cy += FONTH; }
			draw_char('_', left+cx, top+cy);
		}
	}
    glEnd();
    #undef TEXTINDEX
    #undef TEXTWHITE
    #undef TEXTLINE
    #undef TEXTCOLOR
    #undef TEXTCHAR
    #undef TEXTWORD
    return cy + FONTH;
}

void reloadfonts()
{
    enumerate(fonts, font, f,
        if(!reloadtexture(f.tex)) fatal("failed to reload font texture");
    );
}


int draw_textx(const char *fstr, int left, int top, int r, int g, int b, int a, int flags, int cursor, int maxwidth, ...)
{
	s_sprintfdlv(str, maxwidth, fstr);

    if(flags&TEXT_ALIGN)
    {
        int width = text_width(str, flags);
        switch(flags&TEXT_ALIGN)
        {
            case TEXT_CENTERED: left -= width/2; break;
            case TEXT_RIGHT_JUSTIFY: left -= width; break;
            default: break;
        }
    }
    if(flags&TEXT_UPWARD) top -= FONTH;
	return draw_text(str, left, top, r, g, b, a, flags, cursor, maxwidth);
}

vector<const char *> fontstack;

bool pushfont(const char *name)
{
	if(!fontstack.length() && curfont)
		fontstack.add(curfont->name);

	if(setfont(name))
	{
		fontstack.add(name);
		return true;
	}
	return false;
}

bool popfont(int num)
{
	loopi(num)
	{
		if (!fontstack.length()) break;
		fontstack.pop();
	}
	return setfont(fontstack.length() ? fontstack.last() : "default");
}
