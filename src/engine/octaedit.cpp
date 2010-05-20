#include "engine.h"

extern int outline, blankgeom;
editinfo *localedit = NULL;

VAR(0, showpastegrid, 0, 0, 1);
VAR(0, showcursorgrid, 0, 0, 1);
VAR(0, showselgrid, 0, 0, 1);

void boxs(int orient, vec o, const vec &s)
{
    int d = dimension(orient),
          dc= dimcoord(orient);

    float f = !outline && !(fullbright && blankgeom) ? 0 : (dc>0 ? 0.2f : -0.2f);
    o[D[d]] += float(dc) * s[D[d]] + f,

    glBegin(GL_LINE_LOOP);

    glVertex3fv(o.v); o[R[d]] += s[R[d]];
    glVertex3fv(o.v); o[C[d]] += s[C[d]];
    glVertex3fv(o.v); o[R[d]] -= s[R[d]];
    glVertex3fv(o.v);

    glEnd();
    xtraverts += 4;
}

void boxs3D(const vec &o, vec s, int g)
{
    s.mul(g);
    loopi(6)
        boxs(i, o, s);
}

void boxsgrid(int orient, vec o, vec s, int g)
{
    int d = dimension(orient),
          dc= dimcoord(orient);
    float ox = o[R[d]],
          oy = o[C[d]],
          xs = s[R[d]],
          ys = s[C[d]],
          f = !outline && !(fullbright && blankgeom) ? 0 : (dc>0 ? 0.2f : -0.2f);

    o[D[d]] += dc * s[D[d]]*g + f;

    glBegin(GL_LINES);
    loop(x, xs) {
        o[R[d]] += g;
        glVertex3fv(o.v);
        o[C[d]] += ys*g;
        glVertex3fv(o.v);
        o[C[d]] = oy;
    }
    loop(y, ys) {
        o[C[d]] += g;
        o[R[d]] = ox;
        glVertex3fv(o.v);
        o[R[d]] += xs*g;
        glVertex3fv(o.v);
    }
    glEnd();
    xtraverts += 2*int(xs+ys);
}

selinfo sel, lastsel;

int orient = 0;
int gridsize = 8;
ivec cor, lastcor;
ivec cur, lastcur;

extern int entediting;
bool editmode = false;
bool havesel = false;
bool hmapsel = false;
int horient = 0;

extern int entmoving;

VARF(0, dragging, 0, 0, 1,
    if(!dragging || cor[0]<0) return;
    lastcur = cur;
    lastcor = cor;
    sel.grid = gridsize;
    sel.orient = orient;
);

VARF(0, moving, 0, 0, 1,
    if(!moving) return;
    vec v(cur.v); v.add(1);
    moving = pointinsel(sel, v);
    if(moving) havesel = false; // tell cursorupdate to create handle
);

VARF(0, gridpower, 0, 3, 12,
{
    if(dragging) return;
    gridsize = 1<<gridpower;
    if(gridsize>=hdr.worldsize) gridsize = hdr.worldsize/2;
    cancelsel();
});

VAR(0, passthroughsel, 0, 0, 1);
VAR(0, editing, 1, 0, 0);
VAR(0, selectcorners, 0, 0, 1);
VARF(0, hmapedit, 0, 0, 1, horient = sel.orient);

void forcenextundo() { lastsel.orient = -1; }

void cubecancel()
{
    havesel = false;
    moving = dragging = hmapedit = passthroughsel = 0;
    forcenextundo();
}

extern void entcancel();

void cancelsel()
{
    cubecancel();
    entcancel();
}

void toggleedit()
{
    if(!client::allowedittoggle(editmode)) return;
    editmode = !editmode;
    editing = entediting = (editmode ? 1 : 0);
    client::edittoggled(editmode);
    cancelsel();
    efocus = enthover = -1;
    //keyrepeat(editmode);
}

bool noedit(bool view)
{
    if(!editmode) { conoutf("\froperation only allowed in edit mode"); return true; }
    if(view || haveselent()) return false;
    float r = 1.0f;
    vec o = sel.o.tovec(), s = sel.s.tovec();
    s.mul(float(sel.grid) / 2.0f);
    o.add(s);
    r = float(max(s.x, max(s.y, s.z)));
    bool viewable = (isvisiblesphere(r, o) != VFC_NOT_VISIBLE);
    if(!viewable) conoutf("\frselection not in view");
    return !viewable;
}

extern void createheightmap();

void reorient()
{
    sel.cx = 0;
    sel.cy = 0;
    sel.cxs = sel.s[R[dimension(orient)]]*2;
    sel.cys = sel.s[C[dimension(orient)]]*2;
    sel.orient = orient;
}

void selextend()
{
    if(noedit(true)) return;
    loopi(3)
    {
        if(cur[i]<sel.o[i])
        {
            sel.s[i] += (sel.o[i]-cur[i])/sel.grid;
            sel.o[i] = cur[i];
        }
        else if(cur[i]>=sel.o[i]+sel.s[i]*sel.grid)
        {
            sel.s[i] = (cur[i]-sel.o[i])/sel.grid+1;
        }
    }
}

COMMANDN(0, edittoggle, toggleedit, "");
COMMAND(0, entcancel, "");
COMMAND(0, cubecancel, "");
COMMAND(0, cancelsel, "");
COMMAND(0, reorient, "");
COMMAND(0, selextend, "");

///////// selection support /////////////

cube &blockcube(int x, int y, int z, const block3 &b, int rgrid) // looks up a world cube, based on coordinates mapped by the block
{
    int dim = dimension(b.orient), dc = dimcoord(b.orient);
    ivec s(dim, x*b.grid, y*b.grid, dc*(b.s[dim]-1)*b.grid);
    s.add(b.o);
    if(dc) s[dim] -= z*b.grid; else s[dim] += z*b.grid;
    return lookupcube(s.x, s.y, s.z, rgrid);
}

#define loopxy(b)       loop(y,(b).s[C[dimension((b).orient)]]) loop(x,(b).s[R[dimension((b).orient)]])
#define loopxyz(b, r, f) { loop(z,(b).s[D[dimension((b).orient)]]) loopxy((b)) { cube &c = blockcube(x,y,z,b,r); f; } }
#define loopselxyz(f)   { makeundo(); loopxyz(sel, sel.grid, f); changed(sel); }
#define selcube(x, y, z) blockcube(x, y, z, sel, sel.grid)

////////////// cursor ///////////////

int selchildcount=0;

ICOMMAND(0, havesel, "", (), intret(havesel ? selchildcount : 0));

void countselchild(cube *c, const ivec &cor, int size)
{
    ivec ss(sel.s);
    ss.mul(sel.grid);
    loopoctabox(cor, size, sel.o, ss)
    {
        ivec o(i, cor.x, cor.y, cor.z, size);
        if(c[i].children) countselchild(c[i].children, o, size/2);
        else selchildcount++;
    }
}

void normalizelookupcube(int x, int y, int z)
{
    if(lusize>gridsize)
    {
        lu.x += (x-lu.x)/gridsize*gridsize;
        lu.y += (y-lu.y)/gridsize*gridsize;
        lu.z += (z-lu.z)/gridsize*gridsize;
    }
    else if(gridsize>lusize)
    {
        lu.x &= ~(gridsize-1);
        lu.y &= ~(gridsize-1);
        lu.z &= ~(gridsize-1);
    }
    lusize = gridsize;
}

void updateselection()
{
    sel.o.x = min(lastcur.x, cur.x);
    sel.o.y = min(lastcur.y, cur.y);
    sel.o.z = min(lastcur.z, cur.z);
    sel.s.x = abs(lastcur.x-cur.x)/sel.grid+1;
    sel.s.y = abs(lastcur.y-cur.y)/sel.grid+1;
    sel.s.z = abs(lastcur.z-cur.z)/sel.grid+1;
}

void editmoveplane(const vec &o, const vec &ray, int d, float off, vec &handle, vec &dest, bool first)
{
    plane pl(d, off);
    float dist = 0.0f;

    physent *player = (physent *)game::focusedent(true);
    if(!player) player = camera1;
    if(pl.rayintersect(player->o, ray, dist))
    {
        dest = ray;
        dest.mul(dist);
        dest.add(player->o);
        if(first)
        {
            handle = dest;
            handle.sub(o);
        }
        dest.sub(handle);
    }
}

inline bool isheightmap(int orient, int d, bool empty, cube *c);
extern void entdrag(const vec &ray);
extern bool hoveringonent(int ent, int orient);
extern void renderentselection(const vec &o, const vec &ray, bool entmoving);
extern float rayent(const vec &o, const vec &ray, vec &hitpos, float radius, int mode, int size, int &orient, int &ent);

VAR(0, gridlookup, 0, 0, 1);
VAR(0, passthroughcube, 0, 1, 1);

void rendereditcursor()
{
    vec target(worldpos);
    if(!insideworld(target)) loopi(3)
        target[i] = max(min(target[i], float(hdr.worldsize)), 0.0f);
    vec ray(target);
    physent *player = (physent *)game::focusedent(true);
    if(!player) player = camera1;
    ray.sub(player->o).normalize();
    int d   = dimension(sel.orient),
        od  = dimension(orient),
        odc = dimcoord(orient);

    bool hidecursor = hud::hasinput(), hovering = false;
    hmapsel = false;

    if(moving)
    {
        ivec e;
        static vec v, handle;
        editmoveplane(sel.o.tovec(), ray, od, sel.o[D[od]]+odc*sel.grid*sel.s[D[od]], handle, v, !havesel);
        if(!havesel)
        {
            v.add(handle);
            (e = handle).mask(~(sel.grid-1));
            v.sub(handle = e.tovec());
            havesel = true;
        }
        (e = v).mask(~(sel.grid-1));
        sel.o[R[od]] = e[R[od]];
        sel.o[C[od]] = e[C[od]];
    }
    else if(entmoving)
    {
        entdrag(ray);
    }
    else
    {
        vec v;
        ivec w;
        float sdist = 0, wdist = 0, t;
        int entorient = 0, ent = -1;

        wdist = rayent(player->o, ray, v, 0, (editmode && showmat ? RAY_EDITMAT : 0)    // select cubes first
                                            | (!dragging && entediting ? RAY_ENTS : 0)
                                            | RAY_SKIPFIRST
                                            | (passthroughcube==1 ? RAY_PASS : 0), gridsize, entorient, ent);

        if((havesel || dragging) && !passthroughsel && !hmapedit)     // now try selecting the selection
            if(rayrectintersect(sel.o.tovec(), vec(sel.s.tovec()).mul(sel.grid), player->o, ray, sdist, orient))
            {   // and choose the nearest of the two
                if(sdist < wdist)
                {
                    wdist = sdist;
                    ent = -1;
                }
            }

        if((hovering = hoveringonent(hidecursor ? -1 : ent, entorient)))
        {
            if(!havesel)
            {
                selchildcount = 0;
                sel.s = ivec(0, 0, 0);
            }
        }
        else
        {
            v = ray;
            v.mul(wdist+0.1f);
            v.add(player->o);
            w = v;
            cube *c = &lookupcube(w.x, w.y, w.z);
            if(gridlookup && !dragging && !moving && !havesel && hmapedit!=1) gridsize = lusize;
            int mag = gridsize && lusize ? lusize / gridsize : 0;
            normalizelookupcube(w.x, w.y, w.z);
            if(sdist == 0 || sdist > wdist) rayrectintersect(lu.tovec(), vec(gridsize), player->o, ray, t=0, orient); // just getting orient
            cur = lu;
            cor = vec(v).mul(2).div(gridsize);
            od = dimension(orient);
            d = dimension(sel.orient);

            if(hmapedit==1 && dimcoord(horient) == (ray[dimension(horient)]<0))
            {
                hmapsel = isheightmap(horient, dimension(horient), false, c);
                if(hmapsel)
                    od = dimension(orient = horient);
            }

            if(dragging)
            {
                updateselection();
                sel.cx  = min(cor[R[d]], lastcor[R[d]]);
                sel.cy  = min(cor[C[d]], lastcor[C[d]]);
                sel.cxs  = max(cor[R[d]], lastcor[R[d]]);
                sel.cys  = max(cor[C[d]], lastcor[C[d]]);

                if(!selectcorners)
                {
                    sel.cx &= ~1;
                    sel.cy &= ~1;
                    sel.cxs &= ~1;
                    sel.cys &= ~1;
                    sel.cxs -= sel.cx-2;
                    sel.cys -= sel.cy-2;
                }
                else
                {
                    sel.cxs -= sel.cx-1;
                    sel.cys -= sel.cy-1;
                }

                sel.cx  &= 1;
                sel.cy  &= 1;
                havesel = true;
            }
            else if(!havesel)
            {
                sel.o = lu;
                sel.s.x = sel.s.y = sel.s.z = 1;
                sel.cx = sel.cy = 0;
                sel.cxs = sel.cys = 2;
                sel.grid = gridsize;
                sel.orient = orient;
                d = od;
            }

            sel.corner = (cor[R[d]]-(lu[R[d]]*2)/gridsize)+(cor[C[d]]-(lu[C[d]]*2)/gridsize)*2;
            selchildcount = 0;
            countselchild(worldroot, ivec(0, 0, 0), hdr.worldsize/2);
            if(mag>1 && selchildcount==1) selchildcount = -mag;
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // cursors

    lineshader->set();

    renderentselection(player->o, ray, entmoving!=0);

    enablepolygonoffset(GL_POLYGON_OFFSET_LINE);

    #define planargrid(q,r,s) \
    { \
        for (float v = 0.f; v < (hdr.worldsize-s); v += s) \
        { \
            vec a; \
            a = q; a.x = v; boxs3D(a, r, s); \
            a = q; a.y = v; boxs3D(a, r, s); \
            a = q; a.z = v; boxs3D(a, r, s); \
        } \
    }

    if(!moving && !hovering && !hidecursor)
    {
        bool ishmap = hmapedit==1 && hmapsel;
        if(showcursorgrid)
        {
            glColor3ub(ishmap ? 0 : 30, 30, ishmap ? 0 : 30);
            planargrid(lu.tovec(), vec(1, 1, 1), gridsize);
        }
        glColor3ub(ishmap ? 0 : 255, 255, ishmap ? 0 : 255);
        boxs(orient, lu.tovec(), vec(lusize));
    }

    // selections
    if(havesel)
    {
        d = dimension(sel.orient);
        glColor3ub(120, 120, 120);  // grid
        boxsgrid(sel.orient, sel.o.tovec(), sel.s.tovec(), sel.grid);
        glColor3ub(255, 0, 0);  // 0 reference
        boxs3D(sel.o.tovec().sub(0.5f*min(gridsize*0.25f, 2.0f)), vec(min(gridsize*0.25f, 2.0f)), 1);
        glColor3ub(120, 120, 120);// 2D selection box
        vec co(sel.o.v), cs(sel.s.v);
        co[R[d]] += 0.5f*(sel.cx*gridsize);
        co[C[d]] += 0.5f*(sel.cy*gridsize);
        cs[R[d]]  = 0.5f*(sel.cxs*gridsize);
        cs[C[d]]  = 0.5f*(sel.cys*gridsize);
        cs[D[d]] *= gridsize;
        boxs(sel.orient, co, cs);
        if(hmapedit==1) glColor3ub(0, 120, 0); // 3D selection box
        else glColor3ub(0, 0, 120);
        boxs3D(sel.o.tovec(), sel.s.tovec(), sel.grid);
        if(showselgrid)
        {
            glColor3ub(30, 30, 30);
            planargrid(sel.o.tovec(), sel.s.tovec(), sel.grid);
        }
    }

    if(showpastegrid && localedit && localedit->copy)
    {
        glColor3ub(30, 0, 30);
        boxs3D(havesel ? sel.o.tovec() : lu.tovec(), localedit->copy->s.tovec(), havesel ? sel.grid : gridsize);
    }

    disablepolygonoffset(GL_POLYGON_OFFSET_LINE);

    notextureshader->set();

    glDisable(GL_BLEND);
}

//////////// ready changes to vertex arrays ////////////

static bool haschanged = false;

void readychanges(block3 &b, cube *c, const ivec &cor, int size)
{
    loopoctabox(cor, size, b.o, b.s)
    {
        ivec o(i, cor.x, cor.y, cor.z, size);
        if(c[i].ext)
        {
            if(c[i].ext->va)             // removes va s so that octarender will recreate
            {
                int hasmerges = c[i].ext->va->hasmerges;
                destroyva(c[i].ext->va);
                c[i].ext->va = NULL;
                if(hasmerges) invalidatemerges(c[i], true);
            }
            freeoctaentities(c[i]);
            c[i].ext->tjoints = -1;
        }
        if(c[i].children)
        {
            if(size<=1)
            {
                solidfaces(c[i]);
                discardchildren(c[i]);
                brightencube(c[i]);
            }
            else readychanges(b, c[i].children, o, size/2);
        }
        else brightencube(c[i]);
    }
}

void commitchanges(bool force)
{
    if(!force && !haschanged) return;
    haschanged = false;

    extern vector<vtxarray *> valist;
    int oldlen = valist.length();
    entitiesinoctanodes();
    inbetweenframes = false;
    octarender();
    inbetweenframes = true;
    setupmaterials(oldlen);
    invalidatepostfx();
    updatevabbs();
    resetblobs();
}

void changed(const block3 &sel, bool commit = true)
{
    if(sel.s.iszero()) return;
    block3 b = sel;
    loopi(3) b.s[i] *= b.grid;
    b.grid = 1;
    loopi(3)                    // the changed blocks are the selected cubes
    {
        b.o[i] -= 1;
        b.s[i] += 2;
        readychanges(b, worldroot, ivec(0, 0, 0), hdr.worldsize/2);
        b.o[i] += 1;
        b.s[i] -= 2;
    }
    haschanged = true;

    if(commit) commitchanges();
}

//////////// copy and undo /////////////
cube copycube(cube &src)
{
    cube c = src;
    c.ext = NULL; // src cube is responsible for va destruction
    if(src.children)
    {
        c.children = newcubes(F_EMPTY);
        loopi(8) c.children[i] = copycube(src.children[i]);
    }
    else if(src.ext && src.ext->material!=MAT_AIR) ext(c).material = src.ext->material;
    return c;
}

void pastecube(cube &src, cube &dest)
{
    discardchildren(dest);
    dest = copycube(src);
}

void blockcopy(const block3 &s, int rgrid, block3 *b)
{
    *b = s;
    cube *q = b->c();
    loopxyz(s, rgrid, *q++ = copycube(c));
}

block3 *blockcopy(const block3 &s, int rgrid)
{
    block3 *b = (block3 *)new uchar[sizeof(block3)+sizeof(cube)*s.size()];
    blockcopy(s, rgrid, b);
    return b;
}

void freeblock(block3 *b, bool alloced = true)
{
    cube *q = b->c();
    loopi(b->size()) discardchildren(*q++);
    if(alloced) delete[] b;
}

void selgridmap(selinfo &sel, int *g)                           // generates a map of the cube sizes at each grid point
{
    loopxyz(sel, -sel.grid, (*g++ = lusize, (void)c));
}

void freeundo(undoblock *u)
{
    if(!u->numents) freeblock(u->block(), false);
    delete[] (uchar *)u;
}

void pasteundo(undoblock *u)
{
    if(u->numents) pasteundoents(u);
    else
    {
        block3 *b = u->block();
        cube *s = b->c();
        int *g = u->gridmap();
        loopxyz(*b, *g++, pastecube(*s++, c));
    }
}

static inline int undosize(undoblock *u)
{
    if(u->numents) return u->numents*sizeof(undoent);
    else
    {
        block3 *b = u->block();
        cube *q = b->c();
        int size = b->size(), total = size*sizeof(int);
        loopj(size) total += familysize(*q++)*sizeof(cube);
        return total;
    }
}

struct undolist
{
    undoblock *first, *last;

    undolist() : first(NULL), last(NULL) {}

    bool empty() { return !first; }

    void add(undoblock *u)
    {
        u->next = NULL;
        u->prev = last;
        if(!first) first = last = u;
        else
        {
            last->next = u;
            last = u;
        }
    }

    undoblock *popfirst()
    {
        undoblock *u = first;
        first = first->next;
        if(first) first->prev = NULL;
        else last = NULL;
        return u;
    }

    undoblock *poplast()
    {
        undoblock *u = last;
        last = last->prev;
        if(last) last->next = NULL;
        else first = NULL;
        return u;
    }
};

undolist undos, redos;
VAR(IDF_PERSIST, undomegs, 0, 8, 100);                              // bounded by n megs
int totalundos = 0;

void pruneundos(int maxremain)                          // bound memory
{
    while(totalundos > maxremain && !undos.empty())
    {
        undoblock *u = undos.popfirst();
        totalundos -= u->size;
        freeundo(u);
    }
    //conoutf("\faundo: %d of %d(%%%d)", totalundos, undomegs<<20, totalundos*100/(undomegs<<20));
    while(!redos.empty())
    {
        undoblock *u = redos.popfirst();
        totalundos -= u->size;
        freeundo(u);
    }
}

void clearundos() { pruneundos(0); }

COMMAND(0, clearundos, "");

undoblock *newundocube(selinfo &s)
{
    int ssize = s.size(),
        selgridsize = ssize*sizeof(int),
        blocksize = sizeof(block3)+ssize*sizeof(cube);
    undoblock *u = (undoblock *)new uchar[sizeof(undoblock) + blocksize + selgridsize];
    u->numents = 0;
    block3 *b = (block3 *)(u + 1);
    blockcopy(s, -s.grid, b);
    int *g = (int *)((uchar *)b + blocksize);
    selgridmap(s, g);
    return u;
}

void addundo(undoblock *u)
{
    u->size = undosize(u);
    u->timestamp = lastmillis;
    undos.add(u);
    totalundos += u->size;
    pruneundos(undomegs<<20);
}

void makeundoex(selinfo &s)
{
    if(multiplayer(false)) return;
    undoblock *u = newundocube(s);
    addundo(u);
}

void makeundo()                        // stores state of selected cubes before editing
{
    if(lastsel==sel || sel.s.iszero()) return;
    lastsel=sel;
    makeundoex(sel);
}

void swapundo(undolist &a, undolist &b, const char *s)
{
    if(noedit() || multiplayer()) return;
    if(a.empty()) { conoutf("\frnothing more to %s", s); return; }
    int ts = a.last->timestamp;
    selinfo l = sel;
    while(!a.empty() && ts==a.last->timestamp)
    {
        undoblock *u = a.poplast(), *r;
        if(u->numents) r = copyundoents(u);
        else
        {
            block3 *ub = u->block();
            l.o = ub->o;
            l.s = ub->s;
            l.grid = ub->grid;
            l.orient = ub->orient;
            r = newundocube(l);
        }
        r->size = u->size;
        r->timestamp = lastmillis;
        b.add(r);
        pasteundo(u);
        if(!u->numents) changed(l, false);
        freeundo(u);
    }
    commitchanges();
    if(!hmapsel)
    {
        sel = l;
        reorient();
    }
    forcenextundo();
}

void editundo() { swapundo(undos, redos, "undo"); }
void editredo() { swapundo(redos, undos, "redo"); }

vector<editinfo *> editinfos;

static void packcube(cube &c, vector<uchar> &buf)
{
    if(c.children)
    {
        buf.put(0xFF);
        loopi(8) packcube(c.children[i], buf);
    }
    else
    {
        buf.put(c.ext ? c.ext->material : 0);
        cube data = c;
        lilswap(data.texture, 6);
        buf.put(data.edges, sizeof(data.edges));
        buf.put((uchar *)data.texture, sizeof(data.texture));
    }
}

static bool packeditinfo(editinfo *e, vector<uchar> &buf)
{
    if(!e || !e->copy || e->copy->size() > (16<<20)) return false;
    block3 &b = *e->copy;
    block3 hdr = b;
    lilswap(hdr.o.v, 3);
    lilswap(hdr.s.v, 3);
    lilswap(&hdr.grid, 1);
    lilswap(&hdr.orient, 1);
    buf.put((const uchar *)&hdr, sizeof(hdr));
    cube *c = b.c();
    loopi(b.size()) packcube(c[i], buf);
    return true;
}

static void unpackcube(cube &c, ucharbuf &buf)
{
    int mat = buf.get();
    if(mat == 0xFF)
    {
        c.children = newcubes(F_EMPTY);
        loopi(8) unpackcube(c.children[i], buf);
    }
    else
    {
        if(mat != MAT_AIR) ext(c).material = mat;
        buf.get(c.edges, sizeof(c.edges));
        buf.get((uchar *)c.texture, sizeof(c.texture));
        lilswap(c.texture, 6);
    }
}

static bool unpackeditinfo(editinfo *&e, ucharbuf &buf)
{
    if(!e) e = editinfos.add(new editinfo);
    if(e->copy) { freeblock(e->copy); e->copy = NULL; }
    block3 hdr;
    buf.get((uchar *)&hdr, sizeof(hdr));
    lilswap(hdr.o.v, 3);
    lilswap(hdr.s.v, 3);
    lilswap(&hdr.grid, 1);
    lilswap(&hdr.orient, 1);
    if(hdr.size() > (16<<20)) return false;
    e->copy = (block3 *)new uchar[sizeof(block3)+hdr.size()*sizeof(cube)];
    block3 &b = *e->copy;
    b = hdr;
    cube *c = b.c();
    memset(c, 0, b.size()*sizeof(cube));
    loopi(b.size()) unpackcube(c[i], buf);
    return true;
}

static bool compresseditinfo(const uchar *inbuf, int inlen, uchar *&outbuf, int &outlen)
{
    uLongf len = compressBound(inlen);
    if(len > (1<<20)) return false;
    outbuf = new uchar[len];
    if(compress2((Bytef *)outbuf, &len, (const Bytef *)inbuf, inlen, Z_BEST_COMPRESSION) != Z_OK || len > (1<<16))
    {
        delete[] outbuf;
        outbuf = NULL;
        return false;
    }
    outlen = len;
    return true;
}

static bool uncompresseditinfo(const uchar *inbuf, int inlen, uchar *&outbuf, int &outlen)
{
    if(compressBound(outlen) > (1<<20)) return false;
    uLongf len = outlen;
    outbuf = new uchar[len];
    if(uncompress((Bytef *)outbuf, &len, (const Bytef *)inbuf, inlen) != Z_OK)
    {
        delete[] outbuf;
        outbuf = NULL;
        return false;
    }
    outlen = len;
    return true;
}

bool packeditinfo(editinfo *e, int &inlen, uchar *&outbuf, int &outlen)
{
    vector<uchar> buf;
    if(!packeditinfo(e, buf)) return false;
    inlen = buf.length();
    return compresseditinfo(buf.getbuf(), buf.length(), outbuf, outlen);
}

bool unpackeditinfo(editinfo *&e, const uchar *inbuf, int inlen, int outlen)
{
    if(e && e->copy) { freeblock(e->copy); e->copy = NULL; }
    uchar *outbuf = NULL;
    if(!uncompresseditinfo(inbuf, inlen, outbuf, outlen)) return false;
    ucharbuf buf(outbuf, outlen);
    if(!unpackeditinfo(e, buf))
    {
        delete[] outbuf;
        return false;
    }
    delete[] outbuf;
    return true;
}

void freeeditinfo(editinfo *&e)
{
    if(!e) return;
    editinfos.removeobj(e);
    if(e->copy) freeblock(e->copy);
    delete e;
    e = NULL;
}

// guard against subdivision
#define protectsel(f) { undoblock *_u = newundocube(sel); f; pasteundo(_u); freeundo(_u); }

void mpcopy(editinfo *&e, selinfo &sel, bool local)
{
    if(local) client::edittrigger(sel, EDIT_COPY);
    if(e==NULL) e = editinfos.add(new editinfo);
    if(e->copy) freeblock(e->copy);
    e->copy = NULL;
    protectsel(e->copy = blockcopy(block3(sel), sel.grid));
    changed(sel);
}

void mppaste(editinfo *&e, selinfo &sel, bool local)
{
    if(e==NULL) return;
    if(local) client::edittrigger(sel, EDIT_PASTE);
    if(e->copy)
    {
        sel.s = e->copy->s;
        int o = sel.orient;
        sel.orient = e->copy->orient;
        cube *s = e->copy->c();
        loopselxyz(if (!isempty(*s) || s->children) pastecube(*s, c); s++); // 'transparent'. old opaque by 'delcube; paste'
        sel.orient = o;
    }
}

void copy()
{
    if(noedit(true)) return;
    mpcopy(localedit, sel, true);
}

void pasteclear()
{
    if(!localedit) return;
    if(localedit->copy) freeblock(localedit->copy);
    localedit->copy = NULL;
}

void pastehilight()
{
    if(!localedit) return;
    sel.s = localedit->copy->s;
    reorient();
    havesel = true;
}

void paste()
{
    if(noedit()) return;
    mppaste(localedit, sel, true);
}

COMMAND(0, copy, "");
COMMAND(0, pasteclear, "");
COMMAND(0, pastehilight, "");
COMMAND(0, paste, "");
COMMANDN(0, undo, editundo, "");
COMMANDN(0, redo, editredo, "");

static VSlot *editingvslot = NULL;

void compacteditvslots()
{
    if(editingvslot && editingvslot->layer) compactvslot(editingvslot->layer);
    loopv(editinfos)
    {
        editinfo *e = editinfos[i];
        compactvslots(e->copy->c(), e->copy->size());
    }
    for(undoblock *u = undos.first; u; u = u->next)
        if(!u->numents)
            compactvslots(u->block()->c(), u->block()->size());
    for(undoblock *u = redos.first; u; u = u->next)
        if(!u->numents)
            compactvslots(u->block()->c(), u->block()->size());
}

///////////// height maps ////////////////

#define MAXBRUSH    64
#define MAXBRUSHC   63
#define MAXBRUSH2   32
int brush[MAXBRUSH][MAXBRUSH];
VAR(0, brushx, 0, MAXBRUSH2, MAXBRUSH);
VAR(0, brushy, 0, MAXBRUSH2, MAXBRUSH);
bool paintbrush = 0;
int brushmaxx = 0, brushminx = MAXBRUSH;
int brushmaxy = 0, brushminy = MAXBRUSH;

void clearbrush()
{
    memset(brush, 0, sizeof brush);
    brushmaxx = brushmaxy = 0;
    brushminx = brushminy = MAXBRUSH;
    paintbrush = false;
}

void brushvert(int *x, int *y, int *v)
{
    *x += MAXBRUSH2 - brushx + 1; // +1 for automatic padding
    *y += MAXBRUSH2 - brushy + 1;
    if(*x<0 || *y<0 || *x>=MAXBRUSH || *y>=MAXBRUSH) return;
    brush[*x][*y] = clamp(*v, 0, 8);
    paintbrush = paintbrush || (brush[*x][*y] > 0);
    brushmaxx = min(MAXBRUSH-1, max(brushmaxx, *x+1));
    brushmaxy = min(MAXBRUSH-1, max(brushmaxy, *y+1));
    brushminx = max(0,          min(brushminx, *x-1));
    brushminy = max(0,          min(brushminy, *y-1));
}

void brushimport(char *name)
{
    ImageData s;
    if(loadimage(name, s))
    {
        if(s.w > MAXBRUSH || s.h > MAXBRUSH) // only use max size
            scaleimage(s, MAXBRUSH, MAXBRUSH);

        uchar *pixel = s.data;
        int value, x, y;

        clearbrush();
        brushx = brushy = MAXBRUSH2; // set real coords to 0,0

        loopi(s.w)
        {
            x = i;
            loopj(s.h)
            {
                y = j;
                value = 0;

                loopk(s.bpp) // add the entire pixel together
                {
                    value += pixel[0];
                    pixel++;
                }

                value /= s.bpp; // average the entire pixel
                value /= 32; // scale to cube shapes (256 / 8)
                brushvert(&x, &y, &value);
            }
        }
    }
    else conoutf("\frcould not load: %s", name);
}

COMMAND(0, brushimport, "s");

vector<int> htextures;

COMMAND(0, clearbrush, "");
COMMAND(0, brushvert, "iii");
ICOMMAND(0, hmapcancel, "", (), htextures.setsize(0); );
ICOMMAND(0, hmapselect, "", (),
    int t = lookupcube(cur.x, cur.y, cur.z).texture[orient];
    int i = htextures.find(t);
    if(i<0)
        htextures.add(t);
    else
        htextures.remove(i);
);

inline bool ishtexture(int t)
{
    loopv(htextures)
        if(t == htextures[i])
            return false;
    return true;
}

VAR(0, bypassheightmapcheck, 0, 0, 1);    // temp

inline bool isheightmap(int o, int d, bool empty, cube *c)
{
    return havesel ||
           (empty && isempty(*c)) ||
           ishtexture(c->texture[o]);
}

namespace hmap
{
#   define PAINTED     1
#   define NOTHMAP     2
#   define MAPPED      16
    uchar  flags[MAXBRUSH][MAXBRUSH];
    cube   *cmap[MAXBRUSHC][MAXBRUSHC][4];
    int    mapz[MAXBRUSHC][MAXBRUSHC];
    int    map [MAXBRUSH][MAXBRUSH];

    selinfo changes;
    bool selecting;
    int d, dc, dr, dcr, biasup, br, hws, fg;
    int gx, gy, gz, mx, my, mz, nx, ny, nz, bmx, bmy, bnx, bny;
    uint fs;
    selinfo hundo;

    cube *getcube(ivec t, int f)
    {
        t[d] += dcr*f*gridsize;
        if(t[d] > nz || t[d] < mz) return NULL;
        cube *c = &lookupcube(t.x, t.y, t.z, gridsize);
        if(c->children) forcemip(*c);
        discardchildren(*c);
        if(!isheightmap(sel.orient, d, true, c)) return NULL;
        if     (t.x < changes.o.x) changes.o.x = t.x;
        else if(t.x > changes.s.x) changes.s.x = t.x;
        if     (t.y < changes.o.y) changes.o.y = t.y;
        else if(t.y > changes.s.y) changes.s.y = t.y;
        if     (t.z < changes.o.z) changes.o.z = t.z;
        else if(t.z > changes.s.z) changes.s.z = t.z;
        return c;
    }

    uint getface(cube *c, int d)
    {
        return  0x0f0f0f0f & ((dc ? c->faces[d] : 0x88888888 - c->faces[d]) >> fs);
    }

    void pushside(cube &c, int d, int x, int y, int z)
    {
        if(d < 0 || d > 2) return;
        ivec a;
        getcubevector(c, d, x, y, z, a);
        a[R[d]] = 8 - a[R[d]];
        setcubevector(c, d, x, y, z, a);
    }

    void addpoint(int x, int y, int z, int v)
    {
        if(!(flags[x][y] & MAPPED))
          map[x][y] = v + (z*8);
      flags[x][y] |= MAPPED;
    }

    void select(int x, int y, int z)
    {
        if((NOTHMAP & flags[x][y]) || (PAINTED & flags[x][y])) return;
        ivec t(d, x+gx, y+gy, dc ? z : hws-z);
        t.shl(gridpower);

        // selections may damage; must makeundo before
        hundo.o = t;
        hundo.o[D[d]] -= dcr*gridsize*2;
        makeundoex(hundo);

        cube **c = cmap[x][y];
        loopk(4) c[k] = NULL;
        c[1] = getcube(t, 0);
        if(!c[1] || !isempty(*c[1]))
        {   // try up
            c[2] = c[1];
            c[1] = getcube(t, 1);
            if(!c[1] || isempty(*c[1])) {
                c[0] = c[1], c[1] = c[2], c[2] = NULL;
            }else
                z++, t[d]+=fg;
        }
        else // drop down
        {
            z--;
            t[d]-= fg;
            c[0] = c[1];
            c[1] = getcube(t, 0);
        }

        if(!c[1] || isempty(*c[1])) { flags[x][y] |= NOTHMAP; return; }

        flags[x][y] |= PAINTED;
        mapz [x][y]  = z;

        if(!c[0]) c[0] = getcube(t, 1);
        if(!c[2]) c[2] = getcube(t, -1);
        c[3] = getcube(t, -2);
        c[2] = !c[2] || isempty(*c[2]) ? NULL : c[2];
        c[3] = !c[3] || isempty(*c[3]) ? NULL : c[3];

        uint face = getface(c[1], d);
        if(face == 0x08080808 && (!c[0] || !isempty(*c[0]))) { flags[x][y] |= NOTHMAP; return; }
        if(c[1]->faces[R[d]] == F_SOLID)   // was single
            face += 0x08080808;
        else                               // was pair
            face += c[2] ? getface(c[2], d) : 0x08080808;
        face += 0x08080808;                // c[3]
        uchar *f = (uchar*)&face;
        addpoint(x,   y,   z, f[0]);
        addpoint(x+1, y,   z, f[1]);
        addpoint(x,   y+1, z, f[2]);
        addpoint(x+1, y+1, z, f[3]);

        if(selecting) // continue to adjacent cubes
        {
            if(x>bmx) select(x-1, y, z);
            if(x<bnx) select(x+1, y, z);
            if(y>bmy) select(x, y-1, z);
            if(y<bny) select(x, y+1, z);
        }
    }

    void ripple(int x, int y, int z, bool force)
    {
        if(force) select(x, y, z);
        if((NOTHMAP & flags[x][y]) || !(PAINTED & flags[x][y])) return;

        bool changed = false;
        int *o[4], best, par, q = 0;
        loopi(2) loopj(2) o[i+j*2] = &map[x+i][y+j];
        #define pullhmap(I, LT, GT, M, N, A) do { \
            best = I; \
            loopi(4) if(*o[i] LT best) best = *o[q = i] - M; \
            par = (best&(~7)) + N; \
            /* dual layer for extra smoothness */ \
            if(*o[q^3] GT par && !(*o[q^1] LT par || *o[q^2] LT par)) { \
                if(*o[q^3] GT par A 8 || *o[q^1] != par || *o[q^2] != par) { \
                    *o[q^3] = (*o[q^3] GT par A 8 ? par A 8 : *o[q^3]); \
                    *o[q^1] = *o[q^2] = par; \
                    changed = true; \
                } \
            /* single layer */ \
            } else { \
                loopj(4) if(*o[j] GT par) { \
                    *o[j] = par; \
                    changed = true; \
                } \
            } \
        } while(0)

        if(biasup)
            pullhmap(0, >, <, 1, 0, -);
        else
            pullhmap(hdr.worldsize, <, >, 0, 8, +);

        cube **c  = cmap[x][y];
        int e[2][2];
        int notempty = 0;

        loopk(4) if(c[k]) {
            loopi(2) loopj(2) {
                e[i][j] = min(8, map[x+i][y+j] - (mapz[x][y]+3-k)*8);
                notempty |= e[i][j] > 0;
            }
            if(notempty)
            {
                c[k]->texture[sel.orient] = c[1]->texture[sel.orient];
                solidfaces(*c[k]);
                loopi(2) loopj(2)
                {
                    int f = e[i][j];
                    if(f<0 || (f==0 && e[1-i][j]==0 && e[i][1-j]==0))
                    {
                        f=0;
                        pushside(*c[k], d, i, j, 0);
                        pushside(*c[k], d, i, j, 1);
                    }
                    edgeset(cubeedge(*c[k], d, i, j), dc, dc ? f : 8-f);
                }
            }
            else if(c[k])
                emptyfaces(*c[k]);
        }

        if(!changed) return;
        if(x>mx) ripple(x-1, y, mapz[x][y], true);
        if(x<nx) ripple(x+1, y, mapz[x][y], true);
        if(y>my) ripple(x, y-1, mapz[x][y], true);
        if(y<ny) ripple(x, y+1, mapz[x][y], true);

#define DIAGONAL_RIPPLE(a,b,exp) if(exp) { \
            if(flags[x a][ y] & PAINTED) \
                ripple(x a, y b, mapz[x a][y], true); \
            else if(flags[x][y b] & PAINTED) \
                ripple(x a, y b, mapz[x][y b], true); \
        }

        DIAGONAL_RIPPLE(-1, -1, (x>mx && y>my)); // do diagonals because adjacents
        DIAGONAL_RIPPLE(-1, +1, (x>mx && y<ny)); //    won't unless changed
        DIAGONAL_RIPPLE(+1, +1, (x<nx && y<ny));
        DIAGONAL_RIPPLE(+1, -1, (x<nx && y>my));
        }

#define loopbrush(i) for(int x=bmx; x<=bnx+i; x++) for(int y=bmy; y<=bny+i; y++)

    void paint()
    {
        loopbrush(1)
            map[x][y] -= dr * brush[x][y];
    }

    void smooth()
    {
        int sum, div;
        loopbrush(-2)
        {
            sum = 0;
            div = 9;
            loopi(3) loopj(3)
                if(flags[x+i][y+j] & MAPPED)
                    sum += map[x+i][y+j];
                else div--;
            if(div)
                map[x+1][y+1] = sum / div;
        }
    }

    void rippleandset()
    {
        loopbrush(0)
            ripple(x, y, gz, false);
    }

    void run(int dir, int mode)
    {
        d  = dimension(sel.orient);
        dc = dimcoord(sel.orient);
        dcr= dc ? 1 : -1;
        dr = dir>0 ? 1 : -1;
        br = dir>0 ? 0x08080808 : 0;
     //   biasup = mode == dir<0;
        biasup = dir<0;
        bool paintme = paintbrush;
        int cx = (sel.corner&1 ? 0 : -1);
        int cy = (sel.corner&2 ? 0 : -1);
        hws= (hdr.worldsize>>gridpower);
        gx = (cur[R[d]] >> gridpower) + cx - MAXBRUSH2;
        gy = (cur[C[d]] >> gridpower) + cy - MAXBRUSH2;
        gz = (cur[D[d]] >> gridpower);
        fs = dc ? 4 : 0;
        fg = dc ? gridsize : -gridsize;
        mx = max(0, -gx);                   // ripple range
        my = max(0, -gy);
        nx = min(MAXBRUSH-1, hws-gx) - 1;
        ny = min(MAXBRUSH-1, hws-gy) - 1;
        if(havesel)
        {   // selection range
            bmx = mx = max(mx, (sel.o[R[d]]>>gridpower)-gx);
            bmy = my = max(my, (sel.o[C[d]]>>gridpower)-gy);
            bnx = nx = min(nx, (sel.s[R[d]]+(sel.o[R[d]]>>gridpower))-gx-1);
            bny = ny = min(ny, (sel.s[C[d]]+(sel.o[C[d]]>>gridpower))-gy-1);
        }
        if(havesel && mode<0) // -ve means smooth selection
            paintme = false;
        else
        {   // brush range
            bmx = max(mx, brushminx);
            bmy = max(my, brushminy);
            bnx = min(nx, brushmaxx-1);
            bny = min(ny, brushmaxy-1);
        }
        nz = hdr.worldsize-gridsize;
        mz = 0;
        hundo.s = ivec(d,1,1,5);
        hundo.orient = sel.orient;
        hundo.grid = gridsize;
        forcenextundo();

        changes.grid = gridsize;
        changes.s = changes.o = cur;
        memset(map, 0, sizeof map);
        memset(flags, 0, sizeof flags);

        selecting = true;
        select(clamp(MAXBRUSH2-cx, bmx, bnx),
               clamp(MAXBRUSH2-cy, bmy, bny),
              dc ? gz : hws - gz);
        selecting = false;
        if(paintme)
            paint();
        else
            smooth();
        rippleandset();                       // pull up points to cubify, and set
        changes.s.sub(changes.o).shr(gridpower).add(1);
        changed(changes);
    }
}

void edithmap(int dir, int mode) {
    if(multiplayer() || !hmapsel || gridsize < 8) return;
    hmap::run(dir, mode);
}

///////////// main cube edit ////////////////

int bounded(int n) { return n<0 ? 0 : (n>8 ? 8 : n); }

void pushedge(uchar &edge, int dir, int dc)
{
    int ne = bounded(edgeget(edge, dc)+dir);
    edge = edgeset(edge, dc, ne);
    int oe = edgeget(edge, 1-dc);
    if((dir<0 && dc && oe>ne) || (dir>0 && dc==0 && oe<ne)) edge = edgeset(edge, 1-dc, ne);
}

void linkedpush(cube &c, int d, int x, int y, int dc, int dir)
{
    ivec v, p;
    getcubevector(c, d, x, y, dc, v);

    loopi(2) loopj(2)
    {
        getcubevector(c, d, i, j, dc, p);
        if(v==p)
            pushedge(cubeedge(c, d, i, j), dir, dc);
    }
}

static uchar getmaterial(cube &c)
{
    if(c.children)
    {
        uchar mat = getmaterial(c.children[7]);
        loopi(7) if(mat != getmaterial(c.children[i])) return MAT_AIR;
        return mat;
    }
    return c.ext ? c.ext->material : MAT_AIR;
}

VAR(0, invalidcubeguard, 0, 1, 1);

void mpeditface(int dir, int mode, selinfo &sel, bool local)
{
    if(mode==1 && (sel.cx || sel.cy || sel.cxs&1 || sel.cys&1)) mode = 0;
    int d = dimension(sel.orient);
    int dc = dimcoord(sel.orient);
    int seldir = dc ? -dir : dir;

    if(local)
        client::edittrigger(sel, EDIT_FACE, dir, mode);

    if(mode==1)
    {
        int h = sel.o[d]+dc*sel.grid;
        if(((dir>0) == dc && h<=0) || ((dir<0) == dc && h>=hdr.worldsize)) return;
        if(dir<0) sel.o[d] += sel.grid * seldir;
    }

    if(dc) sel.o[d] += sel.us(d)-sel.grid;
    sel.s[d] = 1;

    loopselxyz(
        if(c.children) solidfaces(c);
        uchar mat = getmaterial(c);
        discardchildren(c);
        if(mat!=MAT_AIR) ext(c).material = mat;
        if(mode==1) // fill command
        {
            if(dir<0)
            {
                solidfaces(c);
                cube &o = blockcube(x, y, 1, sel, -sel.grid);
                loopi(6)
                    c.texture[i] = o.children ? 2+i : o.texture[i];
            }
            else
                emptyfaces(c);
        }
        else
        {
            uint bak = c.faces[d];
            uchar *p = (uchar *)&c.faces[d];

            if(mode==2)
                linkedpush(c, d, sel.corner&1, sel.corner>>1, dc, seldir); // corner command
            else
            {
                loop(mx,2) loop(my,2)                                       // pull/push edges command
                {
                    if(x==0 && mx==0 && sel.cx) continue;
                    if(y==0 && my==0 && sel.cy) continue;
                    if(x==sel.s[R[d]]-1 && mx==1 && (sel.cx+sel.cxs)&1) continue;
                    if(y==sel.s[C[d]]-1 && my==1 && (sel.cy+sel.cys)&1) continue;
                    if(p[mx+my*2] != ((uchar *)&bak)[mx+my*2]) continue;

                    linkedpush(c, d, mx, my, dc, seldir);
                }
            }

            optiface(p, c);
            if(invalidcubeguard==1 && !isvalidcube(c))
            {
                uint newbak = c.faces[d];
                uchar *m = (uchar *)&bak;
                uchar *n = (uchar *)&newbak;
                loopk(4) if(n[k] != m[k]) // tries to find partial edit that is valid
                {
                    c.faces[d] = bak;
                    c.edges[d*4+k] = n[k];
                    if(isvalidcube(c))
                        m[k] = n[k];
                }
                c.faces[d] = bak;
            }
        }
    );
    if (mode==1 && dir>0)
        sel.o[d] += sel.grid * seldir;
}

void editface(int *dir, int *mode)
{
    if(noedit(moving!=0)) return;
    if(hmapedit!=1)
        mpeditface(*dir, *mode, sel, true);
    else
        edithmap(*dir, *mode);
}

VAR(0, selectionsurf, 0, 0, 1);

void pushsel(int *dir)
{
    if(noedit(moving!=0)) return;
    int d = dimension(orient);
    int s = dimcoord(orient) ? -*dir : *dir;
    sel.o[d] += s*sel.grid;
    if(selectionsurf==1)
    {
        physent *player = (physent *)game::focusedent(true);
        if(!player) player = camera1;
        player->o[d] += s*sel.grid;
    }
}

void mpdelcube(selinfo &sel, bool local)
{
    if(local) client::edittrigger(sel, EDIT_DELCUBE);
    loopselxyz(discardchildren(c); emptyfaces(c));
}

void delcube()
{
    if(noedit()) return;
    mpdelcube(sel, true);
}

COMMAND(0, pushsel, "i");
COMMAND(0, editface, "ii");
COMMAND(0, delcube, "");

/////////// texture editing //////////////////

int curtexindex = -1, lasttex = 0, lasttexmillis = -1;
int texpaneltimer = 0;
vector<ushort> texmru;

void tofronttex()                                       // maintain most recently used of the texture lists when applying texture
{
    int c = curtexindex;
    if(c>=0)
    {
        texmru.insert(0, texmru.remove(c));
        curtexindex = -1;
    }
}

selinfo repsel;
int reptex = -1;

struct vslotmap
{
    int index;
    VSlot *vslot;

    vslotmap() {}
    vslotmap(int index, VSlot *vslot) : index(index), vslot(vslot) {}
};
static vector<vslotmap> remappedvslots;

VAR(0, usevdelta, 1, 0, 0);

static VSlot *remapvslot(int index, const VSlot &ds)
{
    loopv(remappedvslots) if(remappedvslots[i].index == index) return remappedvslots[i].vslot;
    VSlot &vs = lookupvslot(index, false);
    if(vs.index < 0 || vs.index == DEFAULT_SKY) return NULL;
    VSlot *edit = NULL;
    if(usevdelta)
    {
        VSlot ms;
        mergevslot(ms, vs, ds);
        edit = ms.changed ? editvslot(vs, ms) : vs.slot->variants;
    }
    else edit = ds.changed ? editvslot(vs, ds) : vs.slot->variants;
    if(!edit) edit = &vs;
    remappedvslots.add(vslotmap(vs.index, edit));
    return edit;
}

static void remapvslots(cube &c, const VSlot &ds, int orient, bool &findrep, VSlot *&findedit)
{
    if(c.children)
    {
        loopi(8) remapvslots(c.children[i], ds, orient, findrep, findedit);
        return;
    }
    static VSlot ms;
    if(orient<0) loopi(6)
    {
        VSlot *edit = remapvslot(c.texture[i], ds);
        if(edit)
        {
            c.texture[i] = edit->index;
            if(!findedit) findedit = edit;
        }
    }
    else
    {
        int i = visibleorient(c, orient);
        VSlot *edit = remapvslot(c.texture[i], ds);
        if(edit)
        {
            if(findrep)
            {
                if(reptex < 0) reptex = c.texture[i];
                else if(reptex != c.texture[i]) findrep = false;
            }
            c.texture[i] = edit->index;
            if(!findedit) findedit = edit;
        }
    }
}

void edittexcube(cube &c, int tex, int orient, bool &findrep)
{
    if(orient<0) loopi(6) c.texture[i] = tex;
    else
    {
        int i = visibleorient(c, orient);
        if(findrep)
        {
            if(reptex < 0) reptex = c.texture[i];
            else if(reptex != c.texture[i]) findrep = false;
        }
        c.texture[i] = tex;
    }
    if(c.children) loopi(8) edittexcube(c.children[i], tex, orient, findrep);
}

VAR(0, allfaces, 0, 0, 1);

void mpeditvslot(VSlot &ds, int allfaces, selinfo &sel, bool local)
{
    if(local)
    {
        if(!(lastsel==sel)) tofronttex();
        if(allfaces || !(repsel == sel)) reptex = -1;
        repsel = sel;
    }
    bool findrep = local && !allfaces && reptex < 0;
    VSlot *findedit = NULL;
    editingvslot = &ds;
    loopselxyz(remapvslots(c, ds, allfaces ? -1 : sel.orient, findrep, findedit));
    editingvslot = NULL;
    remappedvslots.setsize(0);
    if(local && findedit)
    {
        lasttex = findedit->index;
        lasttexmillis = totalmillis;
        curtexindex = texmru.find(lasttex);
        if(curtexindex < 0)
        {
            curtexindex = texmru.length();
            texmru.add(lasttex);
        }
    }
}

void vdelta(char *body)
{
    if(noedit() || multiplayer()) return;
    usevdelta++;
    execute(body);
    usevdelta--;
}
COMMAND(0, vdelta, "s");

void vrotate(int *n)
{
    if(noedit() || multiplayer()) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_ROTATION;
    ds.rotation = usevdelta ? *n : clamp(*n, 0, 5);
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(0, vrotate, "i");

void voffset(int *x, int *y)
{
    if(noedit() || multiplayer()) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_OFFSET;
    ds.xoffset = usevdelta ? *x : max(*x, 0);
    ds.yoffset = usevdelta ? *y : max(*y, 0);
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(0, voffset, "ii");

void vscroll(float *s, float *t)
{
    if(noedit() || multiplayer()) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_SCROLL;
    ds.scrollS = *s/1000.0f;
    ds.scrollT = *t/1000.0f;
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(0, vscroll, "ff");

void vscale(float *scale)
{
    if(noedit() || multiplayer()) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_SCALE;
    ds.scale = *scale <= 0 ? 1 : (usevdelta ? *scale : clamp(*scale, 1/8.0f, 8.0f));
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(0, vscale, "f");

void vlayer(int *n)
{
    if(noedit() || multiplayer()) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_LAYER;
    ds.layer = vslots.inrange(*n) ? *n : 0;
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(0, vlayer, "i");

void valpha(float *front, float *back)
{
    if(noedit() || multiplayer()) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_ALPHA;
    ds.alphafront = clamp(*front, 0.0f, 1.0f);
    ds.alphaback = clamp(*back, 0.0f, 1.0f);
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(0, valpha, "ff");

void vcolor(float *r, float *g, float *b)
{
    if(noedit() || multiplayer()) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_COLOR;
    ds.colorscale = vec(clamp(*r, 0.0f, 1.0f), clamp(*g, 0.0f, 1.0f), clamp(*b, 0.0f, 1.0f));
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(0, vcolor, "fff");

void vreset()
{
    if(noedit() || multiplayer()) return;
    VSlot ds;
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(0, vreset, "");

void vshaderparam(const char *name, float *x, float *y, float *z, float *w)
{
    if(noedit() || multiplayer()) return;
    VSlot ds;
    ds.changed = 1<<VSLOT_SHPARAM;
    if(name[0])
    {
        ShaderParam p = { getshaderparamname(name), SHPARAM_LOOKUP, -1, -1, {*x, *y, *z, *w} };
        ds.params.add(p);
    }
    mpeditvslot(ds, allfaces, sel, true);
}
COMMAND(0, vshaderparam, "sffff");

void mpedittex(int tex, int allfaces, selinfo &sel, bool local)
{
    if(local)
    {
        client::edittrigger(sel, EDIT_TEX, tex, allfaces);
        if(allfaces || !(repsel == sel)) reptex = -1;
        repsel = sel;
    }
    bool findrep = local && !allfaces && reptex < 0;
    loopselxyz(edittexcube(c, tex, allfaces ? -1 : sel.orient, findrep));
}

void filltexlist()
{
    if(texmru.length()!=vslots.length())
    {
        loopvrev(texmru) if(texmru[i]>=vslots.length())
        {
            if(curtexindex > i) curtexindex--;
            else if(curtexindex == i) curtexindex = -1;
            texmru.remove(i);
        }
        loopv(vslots) if(texmru.find(i)<0) texmru.add(i);
    }
}

void compactmruvslots()
{
    remappedvslots.setsize(0);
    loopvrev(texmru)
    {
        if(vslots.inrange(texmru[i]))
        {
            VSlot &vs = *vslots[texmru[i]];
            if(vs.index >= 0)
            {
                texmru[i] = vs.index;
                continue;
            }
        }
        if(curtexindex > i) curtexindex--;
        else if(curtexindex == i) curtexindex = -1;
        texmru.remove(i);
    }
    if(vslots.inrange(lasttex))
    {
        VSlot &vs = *vslots[lasttex];
        lasttex = vs.index >= 0 ? vs.index : 0;
    }
    else lasttex = 0;
    reptex = vslots.inrange(reptex) ? vslots[reptex]->index : -1;
}

void edittex(int i, bool save = true, bool edit = true)
{
    lasttex = i;
    lasttexmillis = lastmillis;
    if(save)
    {
        loopvj(texmru) if(texmru[j]==lasttex) { curtexindex = j; break; }
    }
    if(edit) mpedittex(i, allfaces, sel, true);
}

void edittex_(int *dir)
{
    if(noedit()) return;
    filltexlist();
    texpaneltimer = 5000;
    if(!(lastsel==sel)) tofronttex();
    curtexindex = clamp(curtexindex<0 ? 0 : curtexindex+*dir, 0, texmru.length()-1);
    edittex(texmru[curtexindex], false);
}

void gettex()
{
    if(noedit()) return;
    filltexlist();
    int tex = -1;
    loopxyz(sel, sel.grid, tex = c.texture[sel.orient]);
    loopv(texmru) if(texmru[i]==tex)
    {
        curtexindex = i;
        tofronttex();
        return;
    }
}

void getcurtex()
{
    if(noedit()) return;
    filltexlist();
    int index = curtexindex < 0 ? 0 : curtexindex;
    if(!texmru.inrange(index)) return;
    intret(texmru[index]);
}

void getseltex()
{
    if(noedit()) return;
    cube &c = lookupcube(sel.o.x, sel.o.y, sel.o.z, -sel.grid);
    if(c.children || isempty(c)) return;
    intret(c.texture[sel.orient]);
}

void gettexname(int *tex, int *subslot)
{
    if(noedit() || *tex<0) return;
    VSlot &vslot = lookupvslot(*tex);
    Slot &slot = *vslot.slot;
    if(!slot.sts.inrange(*subslot)) return;
    result(slot.sts[*subslot].name);
}

COMMANDN(0, edittex, edittex_, "i");
COMMAND(0, gettex, "");
COMMAND(0, getcurtex, "");
COMMAND(0, getseltex, "");
COMMAND(0, gettexname, "ii");

void replacetexcube(cube &c, int oldtex, int newtex)
{
    loopi(6) if(oldtex < 0 || c.texture[i] == oldtex) c.texture[i] = newtex;
    if(c.children) loopi(8) replacetexcube(c.children[i], oldtex, newtex);
}

void mpreplacetex(int oldtex, int newtex, selinfo &sel, bool local)
{
    if(local) client::edittrigger(sel, EDIT_REPLACE, oldtex, newtex);
    loopi(8) replacetexcube(worldroot[i], oldtex, newtex);
    allchanged();
}

void replacetex(int texnum = -1)
{
    if(noedit()) return;
    mpreplacetex(texnum, lasttex, sel, true);
}

ICOMMAND(0, replace, "", (void), {
    if(reptex < 0) { conoutf("\frcan only replace after a texture edit"); return; }
    replacetex(reptex);
});
ICOMMAND(0, replaceall, "", (void), replacetex(););

////////// flip and rotate ///////////////
uint dflip(uint face) { return face==F_EMPTY ? face : 0x88888888 - (((face&0xF0F0F0F0)>>4) | ((face&0x0F0F0F0F)<<4)); }
uint cflip(uint face) { return ((face&0xFF00FF00)>>8) | ((face&0x00FF00FF)<<8); }
uint rflip(uint face) { return ((face&0xFFFF0000)>>16)| ((face&0x0000FFFF)<<16); }
uint mflip(uint face) { return (face&0xFF0000FF) | ((face&0x00FF0000)>>8) | ((face&0x0000FF00)<<8); }

void flipcube(cube &c, int d)
{
    swap(c.texture[d*2], c.texture[d*2+1]);
    c.faces[D[d]] = dflip(c.faces[D[d]]);
    c.faces[C[d]] = cflip(c.faces[C[d]]);
    c.faces[R[d]] = rflip(c.faces[R[d]]);
    if (c.children)
    {
        loopi(8) if(i&octadim(d)) swap(c.children[i], c.children[i-octadim(d)]);
        loopi(8) flipcube(c.children[i], d);
    }
}

void rotatequad(cube &a, cube &b, cube &c, cube &d)
{
    cube t = a; a = b; b = c; c = d; d = t;
}

void rotatecube(cube &c, int d) // rotates cube clockwise. see pics in cvs for help.
{
    c.faces[D[d]] = cflip (mflip(c.faces[D[d]]));
    c.faces[C[d]] = dflip (mflip(c.faces[C[d]]));
    c.faces[R[d]] = rflip (mflip(c.faces[R[d]]));
    swap(c.faces[R[d]], c.faces[C[d]]);

    swap(c.texture[2*R[d]], c.texture[2*C[d]+1]);
    swap(c.texture[2*C[d]], c.texture[2*R[d]+1]);
    swap(c.texture[2*C[d]], c.texture[2*C[d]+1]);

    if(c.children)
    {
        int row = octadim(R[d]);
        int col = octadim(C[d]);
        for(int i=0; i<=octadim(d); i+=octadim(d)) rotatequad
        (
            c.children[i+row],
            c.children[i],
            c.children[i+col],
            c.children[i+col+row]
        );
        loopi(8) rotatecube(c.children[i], d);
    }
}

void mpflip(selinfo &sel, bool local)
{
    if(local) client::edittrigger(sel, EDIT_FLIP);
    int zs = sel.s[dimension(sel.orient)];
    makeundo();
    loopxy(sel)
    {
        loop(z,zs) flipcube(selcube(x, y, z), dimension(sel.orient));
        loop(z,zs/2)
        {
            cube &a = selcube(x, y, z);
            cube &b = selcube(x, y, zs-z-1);
            swap(a, b);
        }
    }
    changed(sel);
}

void flip()
{
    if(noedit()) return;
    mpflip(sel, true);
}

void mprotate(int cw, selinfo &sel, bool local)
{
    if(local) client::edittrigger(sel, EDIT_ROTATE, cw);
    int d = dimension(sel.orient);
    if(!dimcoord(sel.orient)) cw = -cw;
    int m = sel.s[C[d]] < sel.s[R[d]] ? C[d] : R[d];
    int ss = sel.s[m] = max(sel.s[R[d]], sel.s[C[d]]);
    makeundo();
    loop(z,sel.s[D[d]]) loopi(cw>0 ? 1 : 3)
    {
        loopxy(sel) rotatecube(selcube(x,y,z), d);
        loop(y,ss/2) loop(x,ss-1-y*2) rotatequad
        (
            selcube(ss-1-y, x+y, z),
            selcube(x+y, y, z),
            selcube(y, ss-1-x-y, z),
            selcube(ss-1-x-y, ss-1-y, z)
        );
    }
    changed(sel);
}

void rotate(int *cw)
{
    if(noedit()) return;
    mprotate(*cw, sel, true);
}

COMMAND(0, flip, "");
COMMAND(0, rotate, "i");

void setmat(cube &c, uchar mat, uchar matmask)
{
    if(c.children)
        loopi(8) setmat(c.children[i], mat, matmask);
    else if(mat!=MAT_AIR)
    {
        cubeext &e = ext(c);
        e.material &= matmask;
        e.material |= mat;
    }
    else if(c.ext) c.ext->material = MAT_AIR;
}

void mpeditmat(int matid, selinfo &sel, bool local)
{
    if(local) client::edittrigger(sel, EDIT_MAT, matid);

    uchar matmask = matid&MATF_VOLUME ? 0 : (matid&MATF_CLIP ? ~MATF_CLIP : 0xFF);
    if(isclipped(matid&MATF_VOLUME)) matid |= MAT_CLIP;
    if(isdeadly(matid&MATF_VOLUME)) matid |= MAT_DEATH;
    loopselxyz(setmat(c, matid, matmask));
}

void editmat(char *name)
{
    if(noedit()) return;
    int id = findmaterial(name, true);
    if(id<0) { conoutf("\frunknown material \"%s\"", name); return; }
    mpeditmat(id, sel, true);
}

COMMAND(0, editmat, "s");

int duplicateslot(VSlot &vs)
{
    Slot &s = *vs.slot;
    if(s.shader) { defformatstring(t)("setshader %s", s.shader->name); execute(t); }
    loopvj(s.params)
    {
        defformatstring(t)("set%sparam", s.params[j].type == SHPARAM_LOOKUP ? "shader" : (s.params[j].type == SHPARAM_UNIFORM ? "uniform" : (s.params[j].type == SHPARAM_PIXEL ? "pixel" : "vertex")));
        if(s.params[j].type == SHPARAM_LOOKUP || s.params[j].type == SHPARAM_UNIFORM) { defformatstring(u)(" \"%s\"", s.params[j].name); concatstring(t, u); }
        else { defformatstring(u)(" %d", s.params[j].index); concatstring(t, u); }
        loopk(4) { defformatstring(u)(" %f", s.params[j].val[k]); concatstring(t, u); }
        execute(t);
    }
    loopvj(s.sts)
    {
        defformatstring(t)("texture %s \"%s\"", findtexturename(s.sts[j].type), s.sts[j].lname);
        if(!j) { defformatstring(u)(" %d %d %d %f", vs.rotation, vs.xoffset, vs.yoffset, vs.scale); concatstring(t, u); }
        execute(t);
    }
    if(vs.scrollS != 0.f || vs.scrollT != 0.f) { defformatstring(t)("texscroll %f %f\n", vs.scrollS * 1000.0f, vs.scrollT * 1000.0f); execute(t); }
    if(vs.layer != 0)
    {
        if(s.layermaskname) { defformatstring(t)("texlayer %d \"%s\" %d %f\n", vs.layer, s.layermaskname, s.layermaskmode, s.layermaskscale); execute(t); }
        else { defformatstring(t)("texlayer %d\n", vs.layer); execute(t); }
    }
    if(s.autograss) { defformatstring(t)("autograss \"%s\"\n", s.autograss); execute(t); }
    return slots.length()-1;
}

void texduplicate()
{
    cube &c = lookupcube(sel.o.x, sel.o.y, sel.o.z, -sel.grid);
    int tex = !c.children && !isempty(c) ? c.texture[sel.orient] : lasttex;
    edittex(duplicateslot(lookupvslot(tex, false)), true, false);
}

COMMAND(0, texduplicate, "");

VAR(IDF_PERSIST, thumbwidth, 0, 30, 1000);
VAR(IDF_PERSIST, thumbheight, 0, 10, 1000);
VAR(IDF_PERSIST, thumbtime, 0, 25, 1000);
FVAR(IDF_PERSIST, thumbsize, 0, 1, 8);
FVAR(IDF_PERSIST, thumbpreview, 0, 5, 8);

static int lastthumbnail = 0;

struct texturegui : guicb
{
    bool menuon;
    int menustart, menutab, menutex;

    texturegui() : menustart(-1), menutex(-1) {}

    void gui(guient &g, bool firstpass)
    {
        int origtab = menutab, nextslot = menutex, numtabs = max((slots.length() + thumbwidth*thumbheight - 1)/(thumbwidth*thumbheight), 1);;
        g.start(menustart, menuscale, &menutab, true);
        loopi(numtabs)
        {
            g.tab(!i ? "textures" : NULL);
            if(i+1 != origtab) continue; //don't load textures on non-visible tabs!
            g.pushlist();
            g.pushlist();
            loop(h, thumbheight)
            {
                g.pushlist();
                loop(w, thumbwidth)
                {
                    extern VSlot dummyvslot;
                    int ti = (i*thumbheight+h)*thumbwidth+w;
                    if(ti<slots.length())
                    {
                        Slot &slot = lookupslot(ti, false);
                        VSlot &vslot = *slot.variants;
                        if(slot.sts.empty()) continue;
                        else if(!slot.loaded && !slot.thumbnail)
                        {
                            if(totalmillis-lastthumbnail<thumbtime)
                            {
                                g.texture(dummyvslot, thumbsize, false); //create an empty space
                                continue;
                            }
                            loadthumbnail(slot);
                            lastthumbnail = totalmillis;
                        }
                        if(g.texture(vslot, thumbsize, true)&GUI_UP && (slot.loaded || slot.thumbnail!=notexture))
                            nextslot = vslot.index;
                    }
                    else
                    {
                        g.texture(dummyvslot, thumbsize, false); //create an empty space
                    }
                }
                g.poplist();
            }
            g.space(1);
            if(slots.inrange(menutex))
            {
                Slot &slot = lookupslot(menutex, false);
                VSlot &vslot = *slot.variants;
                g.pushlist();
                if(slot.sts.length() && !slot.loaded && !slot.thumbnail && totalmillis-lastthumbnail<thumbtime)
                {
                    loadthumbnail(slot);
                    lastthumbnail = totalmillis;
                }
                if(g.texture(vslot, thumbpreview, true)&GUI_UP) { edittex(menutex); menuon = false; }
                g.space(2);
                g.pushlist();

                g.pushlist();
                g.pushfont("emphasis"); g.textf("#%-3d", 0xFFFFFF, NULL, menutex); g.popfont();
                g.space(1); if(g.button("<prev", 0x44FFAA)&GUI_UP) nextslot = menutex > 0 ? menutex-1 : slots.length()-1;
                g.space(1); if(g.button("next>", 0xAAFF44)&GUI_UP) nextslot = menutex < slots.length()-1 ? menutex+1 : 0;
                g.space(1); if(g.button("select", 0x66FF66)&GUI_UP) { edittex(menutex); menuon = false; }
                g.space(1); if(g.button("dupe", 0x888888)&GUI_UP) { nextslot = duplicateslot(vslot); }
                g.space(1); if(g.button("cull unused", 0x444444)&GUI_UP) { extern void texturecull(bool local); texturecull(true); }
                g.space(2); g.textf("(\fs\fa%s\fS)", 0xAAFFAA, NULL, slot.shader->name);
                g.poplist();
                if(!slot.sts.empty())
                {
                    bool changed = false;
                    loopvj(slot.params)
                    {
                        g.pushlist();
                        g.textf("set%sparam", 0xAAFFAA, NULL, slot.params[j].type == SHPARAM_LOOKUP ? "shader" : (slot.params[j].type == SHPARAM_UNIFORM ? "uniform" : (slot.params[j].type == SHPARAM_PIXEL ? "pixel" : "vertex")));
                        if(slot.params[j].type == SHPARAM_LOOKUP || slot.params[j].type == SHPARAM_UNIFORM)
                        {
                            defformatstring(index)("%s", slot.params[j].name);
                            defformatstring(input)("param_%d_%d_input", menutex, j);
                            char *w = g.field(input, 0x666666, -32, 0, index, EDITORFOREVER);
                            if(w && *w) { slot.params[j].index = atoi(w); changed = true; g.fieldedit(input); }
                        }
                        else
                        {
                            defformatstring(index)("%d", slot.params[j].index);
                            defformatstring(input)("param_%d_%d_input", menutex, j);
                            char *w = g.field(input, 0x666666, -2, 0, index, EDITORFOREVER);
                            if(w && *w) { slot.params[j].index = atoi(w); changed = true; g.fieldedit(input); }
                        }
                        loopk(4)
                        {
                            g.space(1);
                            defformatstring(index)("%f", slot.params[j].val[k]);
                            defformatstring(input)("param_%d_%d_%d_input", menutex, j, k);
                            char *w = g.field(input, 0x666666, -12, 0, index, EDITORFOREVER);
                            if(w && *w) { slot.params[j].val[k] = atof(w); changed = true; g.fieldedit(input); }
                        }
                        g.poplist();
                        g.space(1);
                    }
                    loopvj(slot.sts)
                    {
                        g.pushlist();
                        g.text("texture", 0xAAFFAA);
                        {
                            g.space(1);
                            defformatstring(index)("%s", findtexturename(slot.sts[j].type));
                            defformatstring(input)("texture_%d_%d_name_input", menutex, j);
                            char *w = g.field(input, 0x666666, -3, 0, index, EDITORFOREVER);
                            if(w && *w) { slot.sts[j].type = findtexturetype(w, true); changed = true; g.fieldedit(input); }
                        }
                        {
                            defformatstring(input)("texture_%d_%d_lname_input", menutex, j);
                            char *w = g.field(input, 0x666666, j ? -78 : -48, 0, slot.sts[j].lname, EDITORFOREVER);
                            if(w && *w)
                            {
                                copystring(slot.sts[j].lname, w);
                                copystring(slot.sts[j].name, w);
                                changed = true;
                                g.fieldedit(input);
                            }
                        }
                        if(!j)
                        {
                            {
                                g.pushfont("console");
                                defformatstring(index)("%d", vslot.rotation);
                                g.space(1);
                                if(g.button(index, 0x666666)&GUI_UP) vslot.rotation = (vslot.rotation+1)%4;
                                g.space(1);
                                g.popfont();
                            }
                            {
                                defformatstring(index)("%d", vslot.xoffset);
                                defformatstring(input)("texture_%d_%d_xoffset_input", menutex, j);
                                char *w = g.field(input, 0x666666, -6, 0, index, EDITORFOREVER);
                                if(w && *w) { vslot.xoffset = atoi(w); g.fieldedit(input); }
                            }
                            {
                                defformatstring(index)("%d", vslot.yoffset);
                                defformatstring(input)("texture_%d_%d_yoffset_input", menutex, j);
                                char *w = g.field(input, 0x666666, -6, 0, index, EDITORFOREVER);
                                if(w && *w) { vslot.yoffset = atoi(w); g.fieldedit(input); }
                            }
                            {
                                defformatstring(index)("%f", vslot.scale);
                                defformatstring(input)("texture_%d_%d_scale_input", menutex, j);
                                char *w = g.field(input, 0x666666, -12, 0, index, EDITORFOREVER);
                                if(w && *w) { vslot.scale = atof(w); g.fieldedit(input); }
                            }
                        }
                        g.poplist();
                    }
                    g.space(1);
                    g.pushlist();
                    g.text("texscroll", 0xAAFFAA);
                    {
                        g.space(1);
                        defformatstring(index)("%f", vslot.scrollS * 1000.0f);
                        defformatstring(input)("texscroll_%d_x_scale_input", menutex);
                        char *w = g.field(input, 0x666666, -12, 0, index, EDITORFOREVER);
                        if(w && *w) { vslot.scrollS = atof(w)/1000.f; g.fieldedit(input); }
                    }
                    {
                        defformatstring(index)("%f", vslot.scrollT * 1000.0f);
                        defformatstring(input)("texscroll_%d_y_scale_input", menutex);
                        char *w = g.field(input, 0x666666, -12, 0, index, EDITORFOREVER);
                        if(w && *w) { vslot.scrollT = atof(w)/1000.f; g.fieldedit(input); }
                    }
                    g.space(1);
                    g.text("autograss", 0xAAFFAA);
                    {
                        g.space(1);
                        defformatstring(input)("autograss_%d_input", menutex);
                        char *w = g.field(input, 0x666666, -45, 0, slot.autograss, EDITORFOREVER);
                        if(w) { DELETEA(slot.autograss); slot.autograss = w[0] ? newstring(w) : NULL; g.fieldedit(input); }
                    }
                    g.poplist();
                    g.pushlist();
                    g.text("texlayer", 0xAAFFAA);
                    {
                        g.space(1);
                        defformatstring(index)("%d", vslot.layer);
                        defformatstring(input)("texlayer_%d_layer_input", menutex);
                        char *w = g.field(input, 0x666666, -6, 0, index, EDITORFOREVER);
                        if(w && *w) { int layer = atoi(w); vslot.layer = layer < 0 ? max(menutex + layer, 0) : layer; changed = true; g.fieldedit(input); }
                    }
                    {
                        defformatstring(input)("texlayer_%d_maskname_input", menutex);
                        char *w = g.field(input, 0x666666, -54, 0, slot.layermaskname, EDITORFOREVER);
                        if(w) { if(slot.layermaskname) delete[] slot.layermaskname; slot.layermaskname = w[0] ? newstring(w) : NULL; changed = true; g.fieldedit(input); }
                    }
                    {
                        defformatstring(index)("%d", slot.layermaskmode);
                        defformatstring(input)("texlayer_%d_maskmode_input", menutex);
                        char *w = g.field(input, 0x666666, -6, 0, index, EDITORFOREVER);
                        if(w && *w) { int mode = atoi(w); slot.layermaskmode = mode; changed = true; g.fieldedit(input); }
                    }
                    {
                        defformatstring(index)("%f", slot.layermaskscale);
                        defformatstring(input)("texlayer_%d_maskscale_input", menutex);
                        char *w = g.field(input, 0x666666, -12, 0, index, EDITORFOREVER);
                        if(w && *w) { float scale = atof(w); slot.layermaskscale = scale <= 0 ? 1 : scale; changed = true; g.fieldedit(input); }
                    }
                    g.poplist();
                    if(changed && slot.loaded)
                    {
                        slot.cleanup();
                        lookupslot(menutex);
                        for(VSlot *variant = slot.variants; variant; variant = variant->next)
                        {
                            if(variant->linked) { variant->cleanup(); lookupvslot(variant->index); }
                        }
                    }
                }

                g.poplist();
                g.poplist();
            }
            else
            {
                g.pushlist();
                g.image(textureload("textures/nothumb", 3), 7, true);
                g.space(2);
                g.pushlist();
                g.title("no texture selected", 0x666666);
                g.poplist();
                g.poplist();
            }
            g.poplist();
            g.poplist();
        }
        menutex = nextslot;
        g.end();
    }

    void showtextures(bool on)
    {
        if(on != menuon && (menuon = on))
        {
            if(menustart <= lasttexmillis)
                menutab = 1+clamp(lookupvslot(lasttex, false).slot->index, 0, slots.length()-1)/(thumbwidth*thumbheight);
            menustart = starttime();
            cube &c = lookupcube(sel.o.x, sel.o.y, sel.o.z, -sel.grid);
            menutex = !isempty(c) ? c.texture[sel.orient] : -1;
        }
    }

    void show()
    {
        if(!menuon) return;
        filltexlist();
        if(!editmode) menuon = false;
        else UI::addcb(this);
    }
} gui;

void texturemenu() { gui.show(); }
bool closetexgui() { if(gui.menuon) { gui.menuon = false; return true; } return false; }

void showtexgui(int *n)
{
    if(!editmode) { conoutf("\froperation only allowed in edit mode"); return; }
    gui.showtextures(*n==0 ? !gui.menuon : *n==1);
}

COMMAND(0, showtexgui, "i"); // 0/noargs = toggle, 1 = on, other = off - will autoclose when exiting editmode

void render_texture_panel(int w, int h)
{
    if((texpaneltimer -= curtime)>0 && editmode)
    {
        glLoadIdentity();
        int width = w*1800/h;
        glOrtho(0, width, 1800, 0, -1, 1);
        int y = 50, gap = 10;

        static Shader *rgbonlyshader = NULL;
        if(!rgbonlyshader) rgbonlyshader = lookupshaderbyname("rgbonly");

        rgbonlyshader->set();

        loopi(7)
        {
            int s = (i == 3 ? 285 : 220), ti = curtexindex+i-3;
            if(ti>=0 && ti<texmru.length())
            {
                VSlot &vslot = lookupvslot(texmru[ti]), *layer = NULL;
                Slot &slot = *vslot.slot;
                Texture *tex = slot.sts.empty() ? notexture : slot.sts[0].t, *glowtex = NULL, *layertex = NULL;
                if(slot.texmask&(1<<TEX_GLOW))
                {
                    loopvj(slot.sts) if(slot.sts[j].type==TEX_GLOW) { glowtex = slot.sts[j].t; break; }
                }
                if(vslot.layer)
                {
                    layer = &lookupvslot(vslot.layer);
                    layertex = layer->slot->sts.empty() ? notexture : layer->slot->sts[0].t;
                }
                float sx = min(1.0f, tex->xs/(float)tex->ys), sy = min(1.0f, tex->ys/(float)tex->xs);
                int x = width-s-50, r = s;
                float tc[4][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } };
                float xoff = vslot.xoffset, yoff = vslot.yoffset;
                if(vslot.rotation)
                {
                    if((vslot.rotation&5) == 1) { swap(xoff, yoff); loopk(4) swap(tc[k][0], tc[k][1]); }
                    if(vslot.rotation >= 2 && vslot.rotation <= 4) { xoff *= -1; loopk(4) tc[k][0] *= -1; }
                    if(vslot.rotation <= 2 || vslot.rotation == 5) { yoff *= -1; loopk(4) tc[k][1] *= -1; }
                }
                loopk(4) { tc[k][0] = tc[k][0]/sx - xoff/tex->xs; tc[k][1] = tc[k][1]/sy - yoff/tex->ys; }
                glBindTexture(GL_TEXTURE_2D, tex->id);
                loopj(glowtex ? 3 : 2)
                {
                    if(j < 2) glColor4f(j*vslot.colorscale.x, j*vslot.colorscale.y, j*vslot.colorscale.z, texpaneltimer/1000.0f);
                    else
                    {
                        glBindTexture(GL_TEXTURE_2D, glowtex->id);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                        glColor4f(vslot.glowcolor.x, vslot.glowcolor.y, vslot.glowcolor.z, texpaneltimer/1000.0f);
                    }
                    glBegin(GL_TRIANGLE_STRIP);
                    glTexCoord2fv(tc[0]); glVertex2f(x,   y);
                    glTexCoord2fv(tc[1]); glVertex2f(x+r, y);
                    glTexCoord2fv(tc[3]); glVertex2f(x,   y+r);
                    glTexCoord2fv(tc[2]); glVertex2f(x+r, y+r);
                    glEnd();
                    xtraverts += 4;
                    if(j==1 && layertex)
                    {
                        glColor4f(layer->colorscale.x, layer->colorscale.y, layer->colorscale.z, texpaneltimer/1000.0f);
                        glBindTexture(GL_TEXTURE_2D, layertex->id);
                        glBegin(GL_TRIANGLE_STRIP);
                        glTexCoord2fv(tc[0]); glVertex2f(x+r/2, y+r/2);
                        glTexCoord2fv(tc[1]); glVertex2f(x+r,   y+r/2);
                        glTexCoord2fv(tc[3]); glVertex2f(x+r/2, y+r);
                        glTexCoord2fv(tc[2]); glVertex2f(x+r,   y+r);
                        glEnd();
                        xtraverts += 4;
                    }
                    if(!j)
                    {
                        r -= 10;
                        x += 5;
                        y += 5;
                    }
                    else if(j == 2) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
            }
            y += s+gap;
        }

        defaultshader->set();
    }
    else texpaneltimer = 0;
}
