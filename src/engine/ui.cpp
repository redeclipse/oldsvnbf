#include "pch.h"
#include "engine.h"
#if 0
#include "textedit.h"

extern bool saycommandon;

namespace UI
{
    struct Object;

    Object *selected = NULL, *hovering = NULL, *focused = NULL;
    float hoverx = 0, hovery = 0, selectx = 0, selecty = 0;

    static inline bool isselected(const Object *o) { return o==selected; }
    static inline bool ishovering(const Object *o) { return o==hovering; }
    static inline bool isfocused(const Object *o) { return o==focused; }

    static void setfocus(Object *o) { focused = o; }
    static inline void clearfocus(const Object *o)
    {
        if(o==selected) selected = NULL;
        if(o==hovering) hovering = NULL;
        if(o==focused) focused = NULL;
    }

    static void quad(float x, float y, float w, float h, float tx = 0, float ty = 0, float tw = 1, float th = 1)
    {
        glTexCoord2f(tx,      ty);      glVertex2f(x,     y);
        glTexCoord2f(tx + tw, ty);      glVertex2f(x + w, y);
        glTexCoord2f(tx + tw, ty + th); glVertex2f(x + w, y + h);
        glTexCoord2f(tx,      ty + th); glVertex2f(x,     y + h);
    }

    struct ClipArea
    {
        float x1, y1, x2, y2;

        ClipArea(float x, float y, float w, float h) : x1(x), y1(y), x2(x+w), y2(y+h) {}

        void intersect(const ClipArea &c)
        {
            x1 = max(x1, c.x1);
            y1 = max(y1, c.y1);
            x2 = max(x1, min(x2, c.x2));
            y2 = max(y1, min(y2, c.y2));
        }

        bool isfullyclipped(float x, float y, float w, float h)
        {
            return x1 == x2 || y1 == y2 || x >= x2 || y >= y2 || x+w <= x1 || y+h <= y1;
        }

        void scissor()
        {
            float margin = max((float(screen->w)/screen->h - 1)/2, 0.0f);
            int sx1 = clamp(int(floor((x1+margin)/(1 + 2*margin)*screen->w)), 0, screen->w),
                sy1 = clamp(int(floor(y1*screen->h)), 0, screen->h),
                sx2 = clamp(int(ceil((x2+margin)/(1 + 2*margin)*screen->w)), 0, screen->w),
                sy2 = clamp(int(ceil(y2*screen->h)), 0, screen->h);
            glScissor(sx1, screen->h - sy2, sx2-sx1, sy2-sy1);
        }
    };

    static vector<ClipArea> clipstack;

    static void pushclip(float x, float y, float w, float h)
    {
        if(clipstack.empty()) glEnable(GL_SCISSOR_TEST);
        ClipArea &c = clipstack.add(ClipArea(x, y, w, h));
        if(clipstack.length() >= 2) c.intersect(clipstack[clipstack.length()-2]);
        c.scissor();
    }

    static void popclip()
    {
        clipstack.pop();
        if(clipstack.empty()) glDisable(GL_SCISSOR_TEST);
    }

    static bool isfullyclipped(float x, float y, float w, float h)
    {
        if(clipstack.empty()) return false;
        return clipstack.last().isfullyclipped(x, y, w, h);
    }

    enum
    {
        ALIGN_MASK    = 0xF,

        ALIGN_HMASK   = 0x3,
        ALIGN_HSHIFT  = 0,
        ALIGN_HNONE   = 0,
        ALIGN_LEFT    = 1,
        ALIGN_HCENTER = 2,
        ALIGN_RIGHT   = 3,

        ALIGN_VMASK   = 0xC,
        ALIGN_VSHIFT  = 2,
        ALIGN_VNONE   = 0<<2,
        ALIGN_BOTTOM  = 1<<2,
        ALIGN_VCENTER = 2<<2,
        ALIGN_TOP     = 3<<2,

        CLAMP_MASK    = 0xF0,
        CLAMP_LEFT    = 0x10,
        CLAMP_RIGHT   = 0x20,
        CLAMP_BOTTOM  = 0x40,
        CLAMP_TOP     = 0x80,

        NO_ADJUST     = ALIGN_HNONE | ALIGN_VNONE,
    };

    static vector<char *> executelater;

    struct Object
    {
        Object *parent;
        float x, y, w, h;
        uchar adjust;
        vector<Object *> children;

        Object() : parent(NULL), x(0), y(0), w(0), h(0), adjust(ALIGN_HCENTER | ALIGN_VCENTER) {}
        virtual ~Object()
        {
            clearfocus(this);
            children.deletecontentsp();
        }

        virtual int forks() const { return 0; }
        virtual int choosefork() const { return -1; }

        #define loopchildren(o, body) do { \
            int numforks = forks(); \
            if(numforks > 0) \
            { \
                int i = choosefork(); \
                if(children.inrange(i)) \
                { \
                    Object *o = children[i]; \
                    body; \
                } \
            } \
            for(int i = numforks; i < children.length(); i++) \
            { \
                Object *o = children[i]; \
                body; \
            } \
        } while(0)


        #define loopchildrenrev(o, body) do { \
            int numforks = forks(); \
            for(int i = children.length()-1; i >= numforks; i--) \
            { \
                Object *o = children[i]; \
                body; \
            } \
            if(numforks > 0) \
            { \
                int i = choosefork(); \
                if(children.inrange(i)) \
                { \
                    Object *o = children[i]; \
                    body; \
                } \
            } \
        } while(0)

        #define loopinchildren(o, cx, cy, body) \
            loopchildren(o, \
            { \
                float o##x = cx - o->x; \
                float o##y = cy - o->y; \
                if(o##x >= 0 && o##x < o->w && o##y >= 0 && o##y < o->h) \
                { \
                    body; \
                } \
            })

        #define loopinchildrenrev(o, cx, cy, body) \
            loopchildrenrev(o, \
            { \
                float o##x = cx - o->x; \
                float o##y = cy - o->y; \
                if(o##x >= 0 && o##x < o->w && o##y >= 0 && o##y < o->h) \
                { \
                    body; \
                } \
            })

        virtual void layout()
        {
            w = h = 0;
            loopchildren(o,
            {
                o->x = o->y = 0;
                o->layout();
                w = max(w, o->x + o->w);
                h = max(h, o->y + o->h);
            });
        }

        void adjustchildrento(float px, float py, float pw, float ph)
        {
            loopchildren(o, o->adjustlayout(px, py, pw, ph));
        }

        virtual void adjustchildren()
        {
            adjustchildrento(0, 0, w, h);
        }

        void adjustlayout(float px, float py, float pw, float ph)
        {
            switch(adjust&ALIGN_HMASK)
            {
                case ALIGN_LEFT:    x = px; break;
                case ALIGN_HCENTER: x = px + (pw - w) / 2; break;
                case ALIGN_RIGHT:   x = px + pw - w; break;
            }

            switch(adjust&ALIGN_VMASK)
            {
                case ALIGN_BOTTOM:  y = py; break;
                case ALIGN_VCENTER: y = py + (ph - h) / 2; break;
                case ALIGN_TOP:     y = py + ph - h; break;
            }

            if(adjust&CLAMP_MASK)
            {
                if(adjust&CLAMP_LEFT)   x = px;
                if(adjust&CLAMP_RIGHT)  w = px + pw - x;
                if(adjust&CLAMP_BOTTOM) y = py;
                if(adjust&CLAMP_TOP)    h = py + ph - y;
            }

            adjustchildren();
        }

        virtual Object *target(float cx, float cy)
        {
            loopinchildrenrev(o, cx, cy,
            {
                Object *c = o->target(ox, oy);
                if(c) return c;
            });
            return NULL;
        }

        virtual bool key(int code, bool isdown, int cooked)
        {
            loopchildrenrev(o,
            {
                if(o->key(code, isdown, cooked)) return true;
            });
            return false;
        }

        virtual void draw(float sx, float sy)
        {
            loopchildren(o,
            {
                if(!isfullyclipped(sx + o->x, sy + o->y, o->w, o->h))
                    o->draw(sx + o->x, sy + o->y);
            });
        }

        void draw()
        {
            draw(x, y);
        }

        virtual Object *hover(float cx, float cy)
        {
            loopinchildrenrev(o, cx, cy,
            {
                Object *c = o->hover(ox, oy);
                if(c == o) { hoverx = ox; hovery = oy; }
                if(c) return c;
            });
            return NULL;
        }

        virtual void hovering(float cx, float cy)
        {
        }

        virtual Object *select(float cx, float cy)
        {
            loopinchildrenrev(o, cx, cy,
            {
                Object *c = o->select(ox, oy);
                if(c == o) { selectx = ox; selecty = oy; }
                if(c) return c;
            });
            return NULL;
        }

        virtual bool allowselect(Object *o) { return false; }

        virtual void selected(float cx, float cy)
        {
        }

        virtual const char *getname() const { return ""; }

        bool isnamed(const char *name) const { return !strcmp(name, getname()); }

        Object *findname(const char *name, bool recurse = true, const Object *exclude = NULL) const
        {
            loopchildren(o,
            {
                if(o != exclude && o->isnamed(name)) return o;
            });
            if(recurse) loopchildren(o,
            {
                if(o != exclude)
                {
                    Object *found = o->findname(name);
                    if(found) return found;
                }
            });
            return NULL;
        }

        Object *findsibling(const char *name) const
        {
            for(const Object *prev = this, *cur = parent; cur; prev = cur, cur = cur->parent)
            {
                Object *o = cur->findname(name, true, prev);
                if(o) return o;
            }
            return NULL;
        }

        void remove(Object *o)
        {
            children.removeobj(o);
            delete o;
        }
    };

    struct HorizontalList : Object
    {
        float space;

        HorizontalList(float space = 0) : space(space) {}

        void layout()
        {
            w = h = 0;
            loopchildren(o,
            {
                o->x = w;
                o->y = 0;
                o->layout();
                w += o->w;
                h = max(h, o->y + o->h);
            });
            w += space*max(children.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            float childspace = (w - children.last()->x - children.last()->w) / max(children.length() - 1, 1),
                  offset = 0;
            loopchildren(o,
            {
                o->x = offset;
                offset += o->w;
                if(!i || i+1 < children.length()) offset += childspace;
                o->adjustlayout(o->x, 0, offset - o->x, h);
            });
        }
    };

    struct VerticalList : Object
    {
        float space;

        VerticalList(float space = 0) : space(space) {}

        void layout()
        {
            w = h = 0;
            loopchildren(o,
            {
                o->x = 0;
                o->y = h;
                o->layout();
                h += o->h;
                w = max(w, o->x + o->w);
            });
            h += space*max(children.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            float childspace = (h - children.last()->y - children.last()->h) / max(children.length() - 1, 1),
                  offset = 0;
            loopchildren(o,
            {
                o->y = offset;
                offset += o->h;
                if(!i || i+1 < children.length()) offset += childspace;
                o->adjustlayout(0, o->y, w, offset - o->y);

            });
        }

    };

    struct Table : Object
    {
        int columns;
        float space;
        vector<float> widths, heights;

        Table(int columns, float space = 0) : columns(columns), space(space) {}

        void layout()
        {
            widths.setsizenodelete(0);
            heights.setsizenodelete(0);

            int column = 0, row = 0;
            loopchildren(o,
            {
                o->layout();
                if(!widths.inrange(column)) widths.add(o->w);
                else if(o->w > widths[column]) widths[column] = o->w;
                if(!heights.inrange(row)) heights.add(o->h);
                else if(o->h > heights[row]) heights[row] = o->h;
                column = (column + 1) % columns;
                if(!column) row++;
            });

            w = h = 0;
            column = row = 0;
            float offset = 0;
            loopchildren(o,
            {
                o->x = offset;
                o->y = h;
                o->adjustlayout(o->x, o->y, widths[column], heights[row]);
                offset += widths[column];
                w = max(w, offset);
                column = (column + 1) % columns;
                if(!column)
                {
                    offset = 0;
                    h += heights[row];
                    row++;
                }
            });
            if(column) h += heights[row];

            w += space*max(widths.length() - 1, 0);
            h += space*max(heights.length() - 1, 0);
        }

        void adjustchildren()
        {
            if(children.empty()) return;

            float cspace = w, rspace = h;
            loopv(widths) cspace -= widths[i];
            loopv(heights) rspace -= heights[i];
            cspace /= max(widths.length() - 1, 1);
            rspace /= max(heights.length() - 1, 1);

            int column = 0, row = 0;
            float offsetx = 0, offsety = 0, nexty = heights[0] + rspace;
            loopchildren(o,
            {
                o->x = offsetx;
                o->y = offsety;
                offsetx += widths[column];
                if(!column || column+1 < widths.length()) offsetx += cspace;
                o->adjustlayout(o->x, o->y, offsetx - o->x, nexty - o->y);
                column = (column + 1) % columns;
                if(!column)
                {
                    offsetx = 0;
                    offsety = nexty;
                    row++;
                    if(row < heights.length())
                    {
                        nexty += heights[row];
                        if(row+1 < heights.length()) nexty += rspace;
                    }
                }
            });
        }
    };

    struct Spacer : Object
    {
        float spacew, spaceh;

        Spacer(float spacew, float spaceh) : spacew(spacew), spaceh(spaceh) {}

        void layout()
        {
            w = spacew;
            h = spaceh;
            loopchildren(o,
            {
                o->x = spacew;
                o->y = spaceh;
                o->layout();
                w = max(w, o->x + o->w);
                h = max(h, o->y + o->h);
            });
            w += spacew;
            h += spaceh;
        }

        void adjustchildren()
        {
            adjustchildrento(spacew, spaceh, w - 2*spacew, h - 2*spaceh);
        }
    };

    struct Filler : Object
    {
        float minw, minh;

        Filler(float minw, float minh) : minw(minw), minh(minh) {}

        void layout()
        {
            Object::layout();

            w = max(w, minw);
            h = max(h, minh);
        }
    };

    struct Offsetter : Object
    {
        float offsetx, offsety;

        Offsetter(float offsetx, float offsety) : offsetx(offsetx), offsety(offsety) {}

        void layout()
        {
            Object::layout();

            loopchildren(o,
            {
                o->x += offsetx;
                o->y += offsety;
            });

            w += offsetx;
            h += offsety;
        }

        void adjustchildren()
        {
            adjustchildrento(offsetx, offsety, w - offsetx, h - offsety);
        }
    };

    struct Clipper : Object
    {
        float clipw, cliph, virtw, virth;

        Clipper(float clipw = 0, float cliph = 0) : clipw(clipw), cliph(cliph), virtw(0), virth(0) {}

        void layout()
        {
            Object::layout();

            virtw = w;
            virth = h;
            if(clipw) w = min(w, clipw);
            if(cliph) h = min(h, cliph);
        }

        void adjustchildren()
        {
            adjustchildrento(0, 0, virtw, virth);
        }

        void draw(float sx, float sy)
        {
            if((clipw && virtw > clipw) || (cliph && virth > cliph))
            {
                pushclip(sx, sy, w, h);
                Object::draw(sx, sy);
                popclip();
            }
            else Object::draw(sx, sy);
        }
    };

    struct Conditional : Object
    {
        char *cond;

        Conditional(const char *cond) : cond(newstring(cond)) {}
        ~Conditional() { delete[] cond; }

        int forks() const { return 2; }
        int choosefork() const { return execute(cond) ? 0 : 1; }
    };

    struct Button : Object
    {
        char *onselect;

        Button(const char *onselect) : onselect(newstring(onselect)) {}
        ~Button() { delete[] onselect; }

        int forks() const { return 3; }
        int choosefork() const { return isselected(this) ? 2 : (ishovering(this) ? 1 : 0); }

        Object *hover(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        Object *select(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        void selected(float cx, float cy)
        {
            executelater.add(newstring(onselect));
        }
    };

    struct ConditionalButton : Button
    {
        char *cond;

        ConditionalButton(const char *cond, const char *onselect) : Button(onselect), cond(newstring(cond)) {}
        ~ConditionalButton() { delete[] cond; }

        int forks() const { return 4; }
        int choosefork() const { return execute(cond) ? 1 + Button::choosefork() : 0; }

        void selected(float cx, float cy)
        {
            if(execute(cond)) Button::selected(cx, cy);
        }
    };

    VAR(uitogglehside, 1, 0, 0);
    VAR(uitogglevside, 1, 0, 0);

    struct Toggle : Button
    {
        char *cond;
        float split;

        Toggle(const char *cond, const char *onselect, float split = 0) : Button(onselect), cond(newstring(cond)), split(split) {}
        ~Toggle() { delete[] cond; }

        int forks() const { return 4; }
        int choosefork() const { return (execute(cond) ? 2 : 0) + (ishovering(this) ? 1 : 0); }

        Object *hover(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        Object *select(float cx, float cy)
        {
            if(target(cx, cy))
            {
                uitogglehside = cx < w*split ? 0 : 1;
                uitogglevside = cy < h*split ? 0 : 1;
                return this;
            }
            return NULL;
        }
    };

    struct Scroller : Clipper
    {
        float offsetx, offsety;

        Scroller(float clipw = 0, float cliph = 0) : Clipper(clipw, cliph), offsetx(0), offsety(0) {}

        Object *target(float cx, float cy)
        {
            if(cx + offsetx >= virtw || cy + offsety >= virth) return NULL;
            return Object::target(cx + offsetx, cy + offsety);
        }

        Object *hover(float cx, float cy)
        {
            if(cx + offsetx >= virtw || cy + offsety >= virth) return NULL;
            return Object::hover(cx + offsetx, cy + offsety);
        }

        Object *select(float cx, float cy)
        {
            if(cx + offsetx >= virtw || cy + offsety >= virth) return NULL;
            return Object::select(cx + offsetx, cy + offsety);
        }

        void draw(float sx, float sy)
        {
            if((clipw && virtw > clipw) || (cliph && virth > cliph))
            {
                pushclip(sx, sy, w, h);
                Object::draw(sx - offsetx, sy - offsety);
                popclip();
            }
            else Object::draw(sx, sy);
        }

        float hlimit() const { return max(virtw - w, 0.0f); }
        float vlimit() const { return max(virth - h, 0.0f); }
        float hoffset() const { return offsetx / max(virtw, w); }
        float voffset() const { return offsety / max(virth, h); }
        float hscale() const { return w / max(virtw, w); }
        float vscale() const { return h / max(virth, h); }

        void addhscroll(float hscroll) { sethscroll(offsetx + hscroll); }
        void addvscroll(float vscroll) { setvscroll(offsety + vscroll); }
        void sethscroll(float hscroll) { offsetx = clamp(hscroll, 0.0f, hlimit()); }
        void setvscroll(float vscroll) { offsety = clamp(vscroll, 0.0f, vlimit()); }

        const char *getname() const { return "scroller"; }
    };

    struct ScrollBar : Object
    {
        float arrowsize, arrowspeed;
        int arrowdir;

        ScrollBar(float arrowsize = 0, float arrowspeed = 0) : arrowsize(arrowsize), arrowspeed(arrowspeed), arrowdir(0) {}

        int forks() const { return 5; }
        int choosefork() const
        {
            switch(arrowdir)
            {
                case -1: return isselected(this) ? 2 : (ishovering(this) ? 1 : 0);
                case 1: return isselected(this) ? 4 : (ishovering(this) ? 3 : 0);
            }
            return 0;
        }

        virtual int choosedir(float cx, float cy) const { return 0; }

        Object *hover(float cx, float cy)
        {
            Object *o = Object::hover(cx, cy);
            if(o) return o;
            return target(cx, cy) ? this : NULL;
        }

        Object *select(float cx, float cy)
        {
            Object *o = Object::select(cx, cy);
            if(o) return o;
            return target(cx, cy) ? this : NULL;
        }

        const char *getname() const { return "scrollbar"; }

        virtual void scrollto(float cx, float cy)
        {
        }

        void selected(float cx, float cy)
        {
            arrowdir = choosedir(cx, cy);
            if(!arrowdir) scrollto(cx, cy);
            else hovering(cx, cy);
        }

        virtual void arrowscroll()
        {
        }

        void hovering(float cx, float cy)
        {
            if(isselected(this))
            {
                if(arrowdir) arrowscroll();
            }
            else
            {
                Object *button = findname("scrollbutton", false);
                if(button && isselected(button))
                {
                    arrowdir = 0;
                    button->hovering(cx - button->x, cy - button->y);
                }
                else arrowdir = choosedir(cx, cy);
            }
        }

        bool allowselect(Object *o)
        {
            return children.find(o) >= 0;
        }

        virtual void movebutton(Object *o, float fromx, float fromy, float tox, float toy) = 0;
    };

    struct ScrollButton : Object
    {
        float offsetx, offsety;

        ScrollButton() : offsetx(0), offsety(0) {}

        int forks() const { return 3; }
        int choosefork() const { return isselected(this) ? 2 : (ishovering(this) ? 1 : 0); }

        Object *hover(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        Object *select(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        void hovering(float cx, float cy)
        {
            if(isselected(this))
            {
                ScrollBar *scrollbar = dynamic_cast<ScrollBar *>(parent);
                if(!scrollbar) return;
                scrollbar->movebutton(this, offsetx, offsety, cx, cy);
            }
        }

        void selected(float cx, float cy)
        {
            offsetx = cx;
            offsety = cy;
        }

        const char *getname() const { return "scrollbutton"; }
    };

    struct HorizontalScrollBar : ScrollBar
    {
        HorizontalScrollBar(float arrowsize = 0, float arrowspeed = 0) : ScrollBar(arrowsize, arrowspeed) {}

        int choosedir(float cx, float cy) const
        {
            if(cx < arrowsize) return -1;
            else if(cx >= w - arrowsize) return 1;
            return 0;
        }

        void arrowscroll()
        {
            Scroller *scroller = dynamic_cast<Scroller *>(findsibling("scroller"));
            if(!scroller) return;
            scroller->addhscroll(arrowdir*arrowspeed*curtime/1000.0f);
        }

        void scrollto(float cx, float cy)
        {
            Scroller *scroller = dynamic_cast<Scroller *>(findsibling("scroller"));
            if(!scroller) return;
            ScrollButton *button = dynamic_cast<ScrollButton *>(findname("scrollbutton", false));
            if(!button) return;
            float bscale = (max(w - 2*arrowsize, 0.0f) - button->w) / (1 - scroller->hscale()),
                  offset = bscale > 1e-3f ? (cx - arrowsize)/bscale : 0;
            scroller->sethscroll(offset*scroller->virtw);
        }

        void adjustchildren()
        {
            Scroller *scroller = dynamic_cast<Scroller *>(findsibling("scroller"));
            if(!scroller) return;
            ScrollButton *button = dynamic_cast<ScrollButton *>(findname("scrollbutton", false));
            if(!button) return;
            float bw = max(w - 2*arrowsize, 0.0f)*scroller->hscale();
            button->w = max(button->w, bw);
            float bscale = scroller->hscale() < 1 ? (max(w - 2*arrowsize, 0.0f) - button->w) / (1 - scroller->hscale()) : 1;
            button->x = arrowsize + scroller->hoffset()*bscale;
            button->adjust &= ~ALIGN_HMASK;

            ScrollBar::adjustchildren();
        }

        void movebutton(Object *o, float fromx, float fromy, float tox, float toy)
        {
            scrollto(o->x + tox - fromx, o->y + toy);
        }
    };

    struct VerticalScrollBar : ScrollBar
    {
        VerticalScrollBar(float arrowsize = 0, float arrowspeed = 0) : ScrollBar(arrowsize, arrowspeed) {}

        int choosedir(float cx, float cy) const
        {
            if(cy < arrowsize) return -1;
            else if(cy >= h - arrowsize) return 1;
            return 0;
        }

        void arrowscroll()
        {
            Scroller *scroller = dynamic_cast<Scroller *>(findsibling("scroller"));
            if(!scroller) return;
            scroller->addvscroll(arrowdir*arrowspeed*curtime/1000.0f);
        }

        void scrollto(float cx, float cy)
        {
            Scroller *scroller = dynamic_cast<Scroller *>(findsibling("scroller"));
            if(!scroller) return;
            ScrollButton *button = dynamic_cast<ScrollButton *>(findname("scrollbutton", false));
            if(!button) return;
            float bscale = (max(h - 2*arrowsize, 0.0f) - button->h) / (1 - scroller->vscale()),
                  offset = bscale > 1e-3f ? (cy - arrowsize)/bscale : 0;
            scroller->setvscroll(offset*scroller->virth);
        }

        void adjustchildren()
        {
            Scroller *scroller = dynamic_cast<Scroller *>(findsibling("scroller"));
            if(!scroller) return;
            ScrollButton *button = dynamic_cast<ScrollButton *>(findname("scrollbutton", false));
            if(!button) return;
            float bh = max(h - 2*arrowsize, 0.0f)*scroller->vscale();
            button->h = max(button->h, bh);
            float bscale = scroller->vscale() < 1 ? (max(h - 2*arrowsize, 0.0f) - button->h) / (1 - scroller->vscale()) : 1;
            button->y = arrowsize + scroller->voffset()*bscale;
            button->adjust &= ~ALIGN_VMASK;

            ScrollBar::adjustchildren();
        }

        void movebutton(Object *o, float fromx, float fromy, float tox, float toy)
        {
            scrollto(o->x + tox, o->y + toy - fromy);
        }
    };

    static bool checkalphamask(Texture *tex, float x, float y)
    {
        if(!tex->alphamask)
        {
            loadalphamask(tex);
            if(!tex->alphamask) return true;
        }
        int tx = clamp(int(floor(x*tex->xs)), 0, tex->xs-1),
            ty = clamp(int(floor(y*tex->ys)), 0, tex->ys-1);
        if(tex->alphamask[ty*((tex->xs+7)/8) + tx/8] & (1<<(tx%8))) return true;
        return false;
    }

    struct Rectangle : Filler
    {
        enum { SOLID = 0, MODULATE };

        int type;
        vec4 color;

        Rectangle(int type, float r, float g, float b, float a, float minw = 0, float minh = 0) : Filler(minw, minh), type(type), color(r, g, b, a) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            return o ? o : this;
        }

        void draw(float sx, float sy)
        {
            if(type==MODULATE) glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            glDisable(GL_TEXTURE_2D);
            notextureshader->set();
            glColor4fv(color.v);
            glBegin(GL_QUADS);
            glVertex2f(sx,     sy);
            glVertex2f(sx + w, sy);
            glVertex2f(sx + w, sy + h);
            glVertex2f(sx,     sy + h);
            glEnd();
            glColor3f(1, 1, 1);
            glEnable(GL_TEXTURE_2D);
            defaultshader->set();
            if(type==MODULATE) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            Object::draw(sx, sy);
        }
    };

    struct Image : Filler
    {
        Texture *tex;

        Image(Texture *tex, float minw = 0, float minh = 0) : Filler(minw, minh), tex(tex) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o) return o;
            if(tex->bpp < 32) return this;
            return checkalphamask(tex, cx/w, cy/h) ? this : NULL;
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);
            glBegin(GL_QUADS);
            quad(sx, sy, w, h);
            glEnd();

            Object::draw(sx, sy);
        }
    };

    VAR(thumbtime, 0, 50, 1000);
    static int lastthumbnail = 0;

    struct SlotViewer : Filler
    {
        int slotnum;

        SlotViewer(int slotnum, float minw = 0, float minh = 0) : Filler(minw, minh), slotnum(slotnum) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o || !texmru.inrange(slotnum)) return o;
            Slot &slot = lookuptexture(texmru[slotnum], false);
            if(slot.sts.length() && (slot.loaded || slot.thumbnail)) return this;
            return NULL;
        }

        void drawslot(Slot &slot, float sx, float sy)
        {
            Texture *tex = notexture, *glowtex = NULL, *layertex = NULL;
            if(slot.loaded)
            {
                tex = slot.sts[0].t;
                if(slot.texmask&(1<<TEX_GLOW)) { loopv(slot.sts) if(slot.sts[i].type==TEX_GLOW) { glowtex = slot.sts[i].t; break; } }
                if(slot.layer)
                {
                    Slot &layer = lookuptexture(slot.layer);
                    layertex = layer.sts[0].t;
                }
            }
            else if(slot.thumbnail) tex = slot.thumbnail;
            float xt, yt;
            xt = min(1.0f, tex->xs/(float)tex->ys),
            yt = min(1.0f, tex->ys/(float)tex->xs);

            static Shader *rgbonlyshader = NULL;
            if(!rgbonlyshader) rgbonlyshader = lookupshaderbyname("rgbonly");
            rgbonlyshader->set();

            float tc[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
            int xoff = slot.xoffset, yoff = slot.yoffset;
            if(slot.rotation)
            {
                if((slot.rotation&5) == 1) { swap(xoff, yoff); loopk(4) swap(tc[k][0], tc[k][1]); }
                if(slot.rotation >= 2 && slot.rotation <= 4) { xoff *= -1; loopk(4) tc[k][0] *= -1; }
                if(slot.rotation <= 2 || slot.rotation == 5) { yoff *= -1; loopk(4) tc[k][1] *= -1; }
            }
            loopk(4) { tc[k][0] = tc[k][0]/xt - float(xoff)/tex->xs; tc[k][1] = tc[k][1]/yt - float(yoff)/tex->ys; }
            glBindTexture(GL_TEXTURE_2D, tex->id);
            glBegin(GL_QUADS);
            glTexCoord2fv(tc[0]); glVertex2f(sx,   sy);
            glTexCoord2fv(tc[1]); glVertex2f(sx+w, sy);
            glTexCoord2fv(tc[2]); glVertex2f(sx+w, sy+h);
            glTexCoord2fv(tc[3]); glVertex2f(sx,   sy+h);
            glEnd();

            if(glowtex)
            {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glBindTexture(GL_TEXTURE_2D, glowtex->id);
                glColor3fv(slot.glowcolor.v);
                glBegin(GL_QUADS);
                glTexCoord2fv(tc[0]); glVertex2f(sx,   sy);
                glTexCoord2fv(tc[1]); glVertex2f(sx+w, sy);
                glTexCoord2fv(tc[2]); glVertex2f(sx+w, sy+h);
                glTexCoord2fv(tc[3]); glVertex2f(sx,   sy+h);
                glEnd();
                glColor3f(1, 1, 1);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            if(layertex)
            {
                glBindTexture(GL_TEXTURE_2D, glowtex->id);
                glBegin(GL_QUADS);
                glTexCoord2fv(tc[0]); glVertex2f(sx+w/2, sy+h/2);
                glTexCoord2fv(tc[1]); glVertex2f(sx+w,   sy+h/2);
                glTexCoord2fv(tc[2]); glVertex2f(sx+w,   sy+h);
                glTexCoord2fv(tc[3]); glVertex2f(sx+w/2, sy+h);
                glEnd();
            }

            defaultshader->set();
        }

        void draw(float sx, float sy)
        {
            if(texmru.inrange(slotnum))
            {
                Slot &slot = lookuptexture(texmru[slotnum], false);
                if(slot.sts.length())
                {
                    if(slot.loaded || slot.thumbnail) drawslot(slot, sx, sy);
                    else if(totalmillis-lastthumbnail>=thumbtime)
                    {
                        loadthumbnail(slot);
                        thumbtime = totalmillis;
                    }
                }
            }

            Object::draw(sx, sy);
        }
    };

    struct CroppedImage : Image
    {
        float cropx, cropy, cropw, croph;

        CroppedImage(Texture *tex, float minw = 0, float minh = 0, float cropx = 0, float cropy = 0, float cropw = 1, float croph = 1)
            : Image(tex, minw, minh), cropx(cropx), cropy(cropy), cropw(cropw), croph(croph) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o) return o;
            if(tex->bpp < 32) return this;
            return checkalphamask(tex, cropx + cx/w*cropw, cropy + cy/h*croph) ? this : NULL;
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);
            glBegin(GL_QUADS);
            quad(sx, sy, w, h, cropx, cropy, cropw, croph);
            glEnd();

            Object::draw(sx, sy);
        }
    };

    struct StretchedImage : Image
    {
        StretchedImage(Texture *tex, float minw = 0, float minh = 0) : Image(tex, minw, minh) {}

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o) return o;
            if(tex->bpp < 32) return this;

            float mx, my;
            if(w <= minw) mx = cx/w;
            else if(cx < minw/2) mx = cx/minw;
            else if(cx >= w - minw/2) mx = 1 - (w - cx) / minw;
            else mx = 0.5f;
            if(h <= minh) my = cy/h;
            else if(cy < minh/2) my = cy/minh;
            else if(cy >= h - minh/2) my = 1 - (h - cy) / minh;
            else my = 0.5f;

            return checkalphamask(tex, mx, my) ? this : NULL;
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);
            glBegin(GL_QUADS);
            float splitw = (minw ? min(minw, w) : w) / 2,
                  splith = (minh ? min(minh, h) : h) / 2,
                  vy = sy, ty = 0;
            loopi(3)
            {
                float vh = 0, th = 0;
                switch(i)
                {
                    case 0: if(splith < h - splith) { vh = splith; th = 0.5f; } else { vh = h; th = 1; } break;
                    case 1: vh = h - 2*splith; th = 0; break;
                    case 2: vh = splith; th = 0.5f; break;
                }
                float vx = sx, tx = 0;
                loopj(3)
                {
                    float vw = 0, tw = 0;
                    switch(j)
                    {
                        case 0: if(splitw < w - splitw) { vw = splitw; tw = 0.5f; } else { vw = w; tw = 1; } break;
                        case 1: vw = w - 2*splitw; tw = 0; break;
                        case 2: vw = splitw; tw = 0.5f; break;
                    }
                    quad(vx, vy, vw, vh, tx, ty, tw, th);
                    vx += vw;
                    tx += tw;
                    if(tx >= 1) break;
                }
                vy += vh;
                ty += th;
                if(ty >= 1) break;
            }
            glEnd();

            Object::draw(sx, sy);
        }
    };

    struct BorderedImage : Image
    {
        float texborder, screenborder;

        BorderedImage(Texture *tex, float texborder, float screenborder) : Image(tex), texborder(texborder), screenborder(screenborder) {}

        void layout()
        {
            Object::layout();

            w = max(w, 2*screenborder);
            h = max(h, 2*screenborder);
        }

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            if(o) return o;
            if(tex->bpp < 32) return this;

            float mx, my;
            if(cx < screenborder) mx = cx/screenborder*texborder;
            else if(cx >= w - screenborder) mx = 1-texborder + (cx - (w - screenborder))/screenborder*texborder;
            else mx = texborder + (cx - screenborder)/(w - 2*screenborder)*(1 - 2*texborder);
            if(cy < screenborder) my = cy/screenborder*texborder;
            else if(cy >= h - screenborder) my = 1-texborder + (cy - (h - screenborder))/screenborder*texborder;
            else my = texborder + (cy - screenborder)/(h - 2*screenborder)*(1 - 2*texborder);

            return checkalphamask(tex, mx, my) ? this : NULL;
        }

        void draw(float sx, float sy)
        {
            glBindTexture(GL_TEXTURE_2D, tex->id);
            glBegin(GL_QUADS);
            float vy = sy, ty = 0;
            loopi(3)
            {
                float vh = 0, th = 0;
                switch(i)
                {
                    case 0: vh = screenborder; th = texborder;
                    case 1: vh = h - 2*screenborder; th = 1 - 2*texborder; break;
                    case 2: vh = screenborder; th = texborder; break;
                }
                float vx = sx, tx = 0;
                loopj(3)
                {
                    float vw = 0, tw = 0;
                    switch(j)
                    {
                        case 0: vw = screenborder; tw = texborder;
                        case 1: vw = w - 2*screenborder; tw = 1 - 2*texborder; break;
                        case 2: vw = screenborder; tw = texborder; break;
                    }
                    quad(vx, vy, vw, vh, tx, ty, tw, th);
                    vx += vw;
                    tx += tw;
                }
                vy += vh;
                ty += th;
            }
            glEnd();

            Object::draw(sx, sy);
        }
    };

    // default size of text in terms of rows per screenful
    VARP(uitextrows, 1, 40, 200);

    struct Text : Object
    {
        char *str;
        float scale;
        vec color;

        Text(const char *str, float scale = 1, float r = 1, float g = 1, float b = 1)
            : str(newstring(str)), scale(scale), color(r, g, b) {}
        ~Text() { delete[] str; }

        float drawscale() const { return scale / (FONTH * uitextrows); }

        void draw(float sx, float sy)
        {
            float k = drawscale();
            glPushMatrix();
            glScalef(k, k, 1);
            glColor3fv(color.v);
            draw_text(str, int(sx/k), int(sy/k), 0, 0, 0, 0);
            glColor3f(1, 1, 1);
            glPopMatrix();

            Object::draw(sx, sy);
        }

        void layout()
        {
            Object::layout();

            int tw, th;
            text_bounds(str, tw, th);
            float k = drawscale();
            w = max(w, tw*k);
            h = max(h, th*k);
        }
    };

    struct TextEditor : Object
    {
        float scale, offsetx, offsety;
        editor *edit;
        char *keyfilter;

        TextEditor(const char *name, int length, int height, float scale = 1, const char *initval = NULL, int mode = EDITORUSED, const char *keyfilter = NULL) : scale(scale), offsetx(0), offsety(0), keyfilter(keyfilter ? newstring(keyfilter) : NULL)
        {
            edit = useeditor(name, mode, initval);
            edit->linewrap = length<0;
            edit->maxx = edit->linewrap ? -1 : length;
            edit->maxy = height <= 0 ? 1 : -1;
            edit->pixelwidth = abs(length)*FONTH;
            if(edit->linewrap && edit->maxy==1)
            {
                int temp;
                text_bounds(edit->lines[0].text, temp, edit->pixelheight, edit->pixelwidth); //only single line editors can have variable height
            }
            else
                edit->pixelheight = FONTH*max(height, 1);
        }
        ~TextEditor()
        {
            DELETEA(keyfilter);
            if(edit->mode!=EDITORFOREVER) removeeditor(edit);
        }

        Object *target(float cx, float cy)
        {
            Object *o = Object::target(cx, cy);
            return o ? o : this;
        }

        Object *hover(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        Object *select(float cx, float cy)
        {
            return target(cx, cy) ? this : NULL;
        }

        void hovering(float cx, float cy)
        {
            if(isselected(this) && isfocused(this))
            {
                bool dragged = max(fabs(cx - offsetx), fabs(cy - offsety)) > (FONTH/8.0f)*scale/float(FONTH*uitextrows);
                edit->hit(int(floor(cx*(FONTH*uitextrows)/scale - FONTW/2)), int(floor(cy*(FONTH*uitextrows)/scale)), dragged);
            }
        }

        void selected(float cx, float cy)
        {
            focuseditor(edit);
            setfocus(this);
            edit->mark(false);
            offsetx = cx;
            offsety = cy;
        }

        bool key(int code, bool isdown, int cooked)
        {
            if(Object::key(code, isdown, cooked)) return true;
            if(!isfocused(this)) return false;
            switch(code)
            {
                case SDLK_RETURN:
                case SDLK_TAB:
                    if(cooked && edit->maxy != 1) break;
                case SDLK_ESCAPE:
                case SDLK_KP_ENTER:
                    setfocus(NULL);
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
                case -4:
                case -5:
                    break;

                default:
                    if(!cooked || code<32) return false;
                    if(keyfilter && !strchr(keyfilter, cooked)) return true;
                    break;
            }
            if(isdown) edit->key(code, cooked);
            return true;
        }

        void layout()
        {
            Object::layout();

            if(edit->linewrap && edit->maxy==1)
            {
                int temp;
                text_bounds(edit->lines[0].text, temp, edit->pixelheight, edit->pixelwidth); //only single line editors can have variable height
            }
            w = max(w, (edit->pixelwidth + FONTW)*scale/float(FONTH*uitextrows));
            h = max(h, edit->pixelheight*scale/float(FONTH*uitextrows));
        }

        void draw(float sx, float sy)
        {
            glPushMatrix();
            glTranslatef(sx, sy, 0);
            glScalef(scale/(FONTH*uitextrows), scale/(FONTH*uitextrows), 1);
            edit->draw(FONTW/2, 0, 0, isfocused(this));
            glColor3f(1, 1, 1);
            glPopMatrix();

            Object::draw(sx, sy);
        }
    };

    struct NamedObject : Object
    {
        char *name;

        NamedObject(const char *name) : name(newstring(name)) {}
        ~NamedObject() { delete[] name; }

        const char *getname() const { return name; }
    };

    struct Tag : NamedObject
    {
        Tag(const char *name) : NamedObject(name) {}
    };

    struct World : Object
    {
        void layout()
        {
            Object::layout();

            float margin = max((float(screen->w)/screen->h - 1)/2, 0.0f);
            x = -margin;
            y = 0;
            w = 1 + 2*margin;
            h = 1;

            adjustchildren();
        }
    };

    struct Window : NamedObject
    {
        char *onhide;

        Window(const char *name, const char *onhide = NULL)
         : NamedObject(name), onhide(onhide && onhide[0] ? newstring(onhide) : NULL)
        {}
        ~Window() { DELETEA(onhide); }

        void hidden()
        {
            if(onhide) execute(onhide);
        }
    };

    World *world = NULL;

    vector<Object *> build;

    Window *buildwindow(const char *name, const char *contents, const char *onhide = NULL)
    {
        Window *window = new Window(name, onhide);
        build.add(window);
        execute(contents);
        build.pop();
        return window;
    }

    ICOMMAND(showui, "sss", (char *name, char *contents, char *onhide),
    {
        if(build.length()) { intret(0); return; }
        Window *oldwindow = dynamic_cast<Window *>(world->findname(name, false));
        if(oldwindow)
        {
            oldwindow->hidden();
            world->remove(oldwindow);
        }
        Window *window = buildwindow(name, contents, onhide);
        world->children.add(window);
        window->parent = world;
        intret(1);
    });

    bool hideui(const char *name)
    {
        Window *window = dynamic_cast<Window *>(world->findname(name, false));
        if(window)
        {
            window->hidden();
            world->remove(window);
        }
        return window!=NULL;
    }

    ICOMMAND(hideui, "s", (char *name), intret(hideui(name) ? 1 : 0));

    ICOMMAND(replaceui, "sss", (char *wname, char *tname, char *contents),
    {
        if(build.length()) { intret(0); return; }
        Window *window = dynamic_cast<Window *>(world->findname(wname, false));
        if(!window) { intret(0); return; }
        Tag *tag = dynamic_cast<Tag *>(window->findname(tname));
        if(!tag) { intret(0); return; }
        tag->children.deletecontentsp();
        build.add(tag);
        execute(contents);
        build.pop();
        intret(1);
    });

    void addui(Object *o, const char *children)
    {
        if(build.length())
        {
            o->parent = build.last();
            build.last()->children.add(o);
        }
        if(children[0])
        {
            build.add(o);
            execute(children);
            build.pop();
        }
    }

    ICOMMAND(uialign, "ii", (int *xalign, int *yalign),
    {
        if(build.length()) build.last()->adjust = (build.last()->adjust & ~ALIGN_MASK) | ((clamp(*xalign, -1, 1)+2)<<ALIGN_HSHIFT) | ((clamp(*yalign, -1, 1)+2)<<ALIGN_VSHIFT);
    });

    ICOMMAND(uiclamp, "iiii", (int *left, int *right, int *bottom, int *top),
    {
        if(build.length()) build.last()->adjust = (build.last()->adjust & ~CLAMP_MASK) |
                                                  (*left ? CLAMP_LEFT : 0) |
                                                  (*right ? CLAMP_RIGHT : 0) |
                                                  (*bottom ? CLAMP_BOTTOM : 0) |
                                                  (*top ? CLAMP_TOP : 0);
    });

    ICOMMAND(uitag, "ss", (char *name, char *children),
        addui(new Tag(name), children));

    ICOMMAND(uihlist, "fs", (float *space, char *children),
        addui(new HorizontalList(*space), children));

    ICOMMAND(uivlist, "fs", (float *space, char *children),
        addui(new VerticalList(*space), children));

    ICOMMAND(uitable, "ifs", (int *columns, float *space, char *children),
        addui(new Table(*columns, *space), children));

    ICOMMAND(uispace, "ffs", (float *spacew, float *spaceh, char *children),
        addui(new Spacer(*spacew, *spaceh), children));

    ICOMMAND(uifill, "ffs", (float *minw, float *minh, char *children),
        addui(new Filler(*minw, *minh), children));

    ICOMMAND(uiclip, "ffs", (float *clipw, float *cliph, char *children),
        addui(new Clipper(*clipw, *cliph), children));

    ICOMMAND(uiscroll, "ffs", (float *clipw, float *cliph, char *children),
        addui(new Scroller(*clipw, *cliph), children));

    ICOMMAND(uihscrollbar, "ffs", (float *arrowsize, float *arrowspeed, char *children),
        addui(new HorizontalScrollBar(*arrowsize, *arrowspeed), children));

    ICOMMAND(uivscrollbar, "ffs", (float *arrowsize, float *arrowspeed, char *children),
        addui(new VerticalScrollBar(*arrowsize, *arrowspeed), children));

    ICOMMAND(uiscrollbutton, "s", (char *children),
        addui(new ScrollButton, children));

    ICOMMAND(uioffset, "ffs", (float *offsetx, float *offsety, char *children),
        addui(new Offsetter(*offsetx, *offsety), children));

    ICOMMAND(uibutton, "ss", (char *onselect, char *children),
        addui(new Button(onselect), children));

    ICOMMAND(uicond, "ss", (char *cond, char *children),
        addui(new Conditional(cond), children));

    ICOMMAND(uicondbutton, "sss", (char *cond, char *onselect, char *children),
        addui(new ConditionalButton(cond, onselect), children));

    ICOMMAND(uitoggle, "ssfs", (char *cond, char *onselect, float *split, char *children),
        addui(new Toggle(cond, onselect, *split), children));

    ICOMMAND(uiimage, "sffs", (char *texname, float *minw, float *minh, char *children),
        addui(new Image(textureload(texname, 3, true, false), *minw, *minh), children));

    ICOMMAND(uislotview, "iffs", (int *slotnum, float *minw, float *minh, char *children),
        addui(new SlotViewer(*slotnum, *minw, *minh), children));

    ICOMMAND(uialtimage, "s", (char *texname),
    {
        if(build.empty()) return;
        Image *image = dynamic_cast<Image *>(build.last());
        if(image && image->tex==notexture) image->tex = textureload(texname, 3, true, false);
    });

    ICOMMAND(uicolor, "ffffffs", (float *r, float *g, float *b, float *a, float *minw, float *minh, char *children),
        addui(new Rectangle(Rectangle::SOLID, *r, *g, *b, *a, *minw, *minh), children));

    ICOMMAND(uimodcolor, "ffffffs", (float *r, float *g, float *b, float *minw, float *minh, char *children),
        addui(new Rectangle(Rectangle::MODULATE, *r, *g, *b, 1, *minw, *minh), children));

    ICOMMAND(uistretchedimage, "sffs", (char *texname, float *minw, float *minh, char *children),
        addui(new StretchedImage(textureload(texname, 3, true, false), *minw, *minh), children));

    ICOMMAND(uicroppedimage, "sffsssss", (char *texname, float *minw, float *minh, char *cropx, char *cropy, char *cropw, char *croph, char *children),
        Texture *tex = textureload(texname, 3, true, false);
        addui(new CroppedImage(tex, *minw, *minh,
            strchr(cropx, 'p') ? atof(cropx) / tex->xs : atof(cropx),
            strchr(cropy, 'p') ? atof(cropy) / tex->ys : atof(cropy),
            strchr(cropw, 'p') ? atof(cropw) / tex->xs : atof(cropw),
            strchr(croph, 'p') ? atof(croph) / tex->ys : atof(croph)), children));

    ICOMMAND(uiborderedimage, "ssfs", (char *texname, char *texborder, float *screenborder, char *children),
        Texture *tex = textureload(texname, 3, true, false);
        addui(new BorderedImage(tex,
                strchr(texborder, 'p') ? atof(texborder) / tex->xs : atof(texborder),
                *screenborder), children));

    ICOMMAND(uicolortext, "sffffs", (char *text, float *scale, float *r, float *g, float *b, char *children),
        addui(new Text(text, *scale <= 0 ? 1 : *scale, *r, *g, *b), children));

    ICOMMAND(uitext, "sfs", (char *text, float *scale, char *children),
        addui(new Text(text, *scale <= 0 ? 1 : *scale), children));

    ICOMMAND(uitexteditor, "siifsiss", (char *name, int *length, int *height, float *scale, char *initval, int *keep, char *filter, char *children),
        addui(new TextEditor(name, *length, *height, *scale, initval, *keep ? EDITORFOREVER : EDITORUSED, filter[0] ? filter : NULL), children));

    FVAR(cursorsensitivity, 1e-3f, 1, 1000);

    float cursorx = 0.5f, cursory = 0.5f;

    void resetcursor()
    {
        cursorx = cursory = 0.5f;
    }

    bool movecursor(int dx, int dy)
    {
        if(editmode ? world->children.empty() : freelook) return false;
        float scale = 500.0f / cursorsensitivity;
        cursorx = clamp(cursorx+dx*(screen->h/(screen->w*scale)), 0.0f, 1.0f);
        cursory = clamp(cursory+dy/scale, 0.0f, 1.0f);
        return true;
    }

    bool hascursor(bool targeting)
    {
        if(editmode ? world->children.length() : !freelook)
        {
            if(!targeting) return true;
            if(world && world->target(cursorx*world->w, cursory*world->h)) return true;
        }
        return false;
    }

    void getcursorpos(float &x, float &y)
    {
        if(!editmode || world->children.length()) { x = cursorx; y = cursory; }
        else x = y = 0.5f;
    }

    bool keypress(int code, bool isdown, int cooked)
    {
        if(!hascursor(true)) return false;
        switch(code)
        {
            case -1:
            {
                if(isdown)
                {
                    selected = world->select(cursorx*world->w, cursory*world->h);
                    if(selected) selected->selected(selectx, selecty);
                }
                else selected = NULL;
                return true;
            }

            default:
                return world->key(code, isdown, cooked);
        }
    }

    void setup()
    {
        world = new World;
    }

    TextEditor *textediting = NULL;

    void update()
    {
        loopv(executelater) execute(executelater[i]);
        executelater.deletecontentsa();

        world->layout();

        if(hascursor())
        {
            hovering = world->hover(cursorx*world->w, cursory*world->h);
            if(hovering) hovering->hovering(hoverx, hovery);
        }
        else hovering = selected = NULL;

        world->layout();

        bool wastextediting = textediting!=NULL;
        textediting = dynamic_cast<TextEditor *>(focused);
        if((textediting!=NULL) != wastextediting)
        {
            SDL_EnableUNICODE(textediting!=NULL || saycommandon);
            keyrepeat(textediting!=NULL || editmode || saycommandon);
        }
    }

    void render()
    {
        if(world->children.empty()) return;

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(world->x, world->x + world->w, world->y + world->h, world->y, -1, 1);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();

        glColor3f(1, 1, 1);

        world->draw();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
}

struct change
{
    int type;
    const char *desc;

    change() {}
    change(int type, const char *desc) : type(type), desc(desc) {}
};
static vector<change> needsapply;

VARP(applydialog, 0, 1, 1);

void addchange(const char *desc, int type)
{
    if(!applydialog) return;
    loopv(needsapply) if(!strcmp(needsapply[i].desc, desc)) return;
    needsapply.add(change(type, desc));
    if(identexists("showchanges")) execute("showchanges");
}

void clearchanges(int type)
{
    loopv(needsapply)
    {
        if(needsapply[i].type&type)
        {
            needsapply[i].type &= ~type;
            if(!needsapply[i].type) needsapply.remove(i--);
        }
    }
    if(needsapply.empty()) UI::hideui("changes");
}

void applychanges()
{
    int changetypes = 0;
    loopv(needsapply) changetypes |= needsapply[i].type;
    if(changetypes&CHANGE_GFX) UI::executelater.add(newstring("resetgl"));
    if(changetypes&CHANGE_SOUND) UI::executelater.add(newstring("resetsound"));
}

ICOMMAND(clearchanges, "", (), clearchanges(CHANGE_GFX|CHANGE_SOUND));
COMMAND(applychanges, "");

ICOMMAND(loopchanges, "ss", (char *var, char *body),
{
    loopv(needsapply) { alias(var, needsapply[i].desc); execute(body); }
});
#endif
