#include "pch.h"
#include "engine.h"

extern int outline;
#ifdef BFRONTIER
VARP(showcursorgrid, 0, 0, 1);
VARP(showselgrid, 0, 1, 1);
#endif

void boxs(int orient, vec o, const vec &s)
{
	int	d = dimension(orient),
		  dc= dimcoord(orient);

	float f = !outline ? 0 : (dc>0 ? 0.2f : -0.2f);
	o[D[d]] += float(dc) * s[D[d]] + f,

	glBegin(GL_POLYGON);

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
	int	d = dimension(orient),
		  dc= dimcoord(orient);
	float ox = o[R[d]],
		  oy = o[C[d]],
		  xs = s[R[d]],
		  ys = s[C[d]],
		  f = !outline ? 0 : (dc>0 ? 0.2f : -0.2f);	

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

selinfo sel = { 0 }, lastsel;

int orient = 0;
int gridsize = 8;
ivec cor, lastcor;
ivec cur, lastcur;

extern int entediting;
bool editmode = false;
bool havesel = false;
bool hmapsel = false;
int  horient = 0;

extern int entmoving;

VARF(dragging, 0, 0, 1,
	if(!dragging || cor[0]<0) return;
	lastcur = cur;
	lastcor = cor;
	sel.grid = gridsize;
	sel.orient = orient;
);

VARF(moving, 0, 0, 1,
	if(!moving) return;
	vec v(cur.v); v.add(1);
	moving = pointinsel(sel, v);
	if(moving) havesel = false; // tell cursorupdate to create handle
);

void forcenextundo() { lastsel.orient = -1; }

void cubecancel()
{
	havesel = false;
	moving = dragging = 0;
	forcenextundo();
}

extern void entcancel();

void cancelsel()
{
	cubecancel();
	entcancel();
}

VARF(gridpower, 3-VVEC_FRAC, 3, VVEC_INT-1,
{
	if(dragging) return;
	gridsize = 1<<gridpower;
	if(gridsize>=hdr.worldsize) gridsize = hdr.worldsize/2;
	cancelsel();
});

VAR(passthroughsel, 0, 0, 1);
VAR(editing, 1, 0, 0);
VAR(selectcorners, 0, 0, 1);
VARF(hmapedit, 0, 0, 1, cancelsel());

void toggleedit()
{
	if(player->state!=CS_ALIVE && player->state!=CS_EDITING) return; // do not allow dead players to edit to avoid state confusion
	if(!editmode && !cc->allowedittoggle()) return;		 // not in most multiplayer modes
	if(!(editmode = !editmode))
	{
		player->state = CS_ALIVE;
		player->o.z -= player->eyeheight;		// entinmap wants feet pos
		entinmap(player);						// find spawn closest to current floating pos
	}
	else
	{
		cl->resetgamestate();
		player->state = CS_EDITING;
	}
	cancelsel();
	keyrepeat(editmode);
	editing = entediting = editmode;
#ifndef BFRONTIER // fullbright and fullbrightlevel support
	extern int fullbright;
	if(fullbright) initlights();
#endif
	cc->edittoggled(editmode);
}

bool noedit(bool view)
{
	if(!editmode) { conoutf("operation only allowed in edit mode"); return true; }
	if(view || haveselent()) return false;
	float r = 1.0f;
	vec o, s;
	o = sel.o.v;
	s = sel.s.v;
	s.mul(float(sel.grid) / 2.0f);
	o.add(s);
	r = float(max(s.x, max(s.y, s.z)));
	bool viewable = (isvisiblesphere(r, o) != VFC_NOT_VISIBLE);
	if(!viewable) conoutf("selection not in view");
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

COMMANDN(edittoggle, toggleedit, "");
COMMAND(entcancel, "");
COMMAND(cubecancel, "");
COMMAND(cancelsel, "");
COMMAND(reorient, "");
COMMAND(selextend, "");

///////// selection support /////////////

cube &blockcube(int x, int y, int z, const block3 &b, int rgrid) // looks up a world cube, based on coordinates mapped by the block
{
	ivec s(dimension(b.orient), x*b.grid, y*b.grid, dimcoord(b.orient)*(b.s[dimension(b.orient)]-1)*b.grid);

	return neighbourcube(b.o.x+s.x, b.o.y+s.y, b.o.z+s.z, -z*b.grid, rgrid, b.orient);
}

#define loopxy(b)		loop(y,(b).s[C[dimension((b).orient)]]) loop(x,(b).s[R[dimension((b).orient)]])
#define loopxyz(b, r, f) { loop(z,(b).s[D[dimension((b).orient)]]) loopxy((b)) { cube &c = blockcube(x,y,z,b,r); f; } }
#define loopselxyz(f)	{ makeundo(); loopxyz(sel, sel.grid, f); changed(sel); }
#define selcube(x, y, z) blockcube(x, y, z, sel, sel.grid)

////////////// cursor ///////////////

int selchildcount=0;

ICOMMAND(havesel, "", (), intret(havesel ? selchildcount : 0));

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
extern float rayent(const vec &o, vec &ray, vec &hitpos, float radius, int mode, int size, int &orient, int &ent);

VAR(gridlookup, 0, 0, 1);
VAR(passthroughcube, 0, 1, 1);

void cursorupdate()
{
	if(sel.grid == 0) sel.grid = gridsize;

	vec target(worldpos);
	if(!insideworld(target)) loopi(3) 
		target[i] = max(min(target[i], hdr.worldsize), 0);
	vec ray(target);
	ray.sub(player->o).normalize();
	int d	= dimension(sel.orient),
		od  = dimension(orient),
		odc = dimcoord(orient);

	bool hovering = false;
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
			v.sub(handle = e.v);
			havesel = true;
		}
		(e = v).mask(~(sel.grid-1));
		sel.o[R[od]] = e[R[od]];
		sel.o[C[od]] = e[C[od]];
	}
	else 
	if(entmoving)
	{
		entdrag(ray);		
	}
	else
	{  
		vec v;
		ivec w;
		float sdist = 0, wdist = 0, t;
		int entorient = 0, ent = -1;
		
		wdist = rayent(player->o, ray, v, 0, (editmode && showmat ? RAY_EDITMAT : 0)	// select cubes first
											| (!dragging && entediting ? RAY_ENTS : 0)
											| RAY_SKIPFIRST 
											| (passthroughcube==1 ? RAY_PASS : 0), gridsize, entorient, ent);
	 
		if((havesel || dragging) && !passthroughsel)	 // now try selecting the selection
			if(rayrectintersect(sel.o.tovec(), vec(sel.s.tovec()).mul(sel.grid), player->o, ray, sdist, orient))
			{	// and choose the nearest of the two
				if(sdist < wdist) 
				{
					wdist = sdist;
					ent	= -1;
				}
			}

		if(hovering = hoveringonent(ent, entorient))
		{
			if(!havesel) {
				selchildcount = 0;
				sel.s = vec(0);
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
			int mag = lusize / gridsize;
			normalizelookupcube(w.x, w.y, w.z);
			if(sdist == 0 || sdist > wdist) rayrectintersect(lu.tovec(), vec(gridsize), player->o, ray, t=0, orient); // just getting orient	 
			cur = lu;
			cor = w;
			cor.mul(2).div(gridsize);
			od = dimension(orient);
			d = dimension(sel.orient);

            if(mag > 0 && hmapedit==1)
            {
                hmapsel = isheightmap(horient, dimension(horient), false, c);     
                if(hmapsel)
                    od = dimension(orient = horient);
            }

			if(dragging) 
			{
				updateselection();
				sel.cx	= min(cor[R[d]], lastcor[R[d]]);
				sel.cy	= min(cor[C[d]], lastcor[C[d]]);
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
			countselchild(worldroot, vec(0), hdr.worldsize/2);
			if(mag>1 && selchildcount==1) selchildcount = -mag;
		}
	}
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	
	// cursors	

	renderentselection(player->o, ray, entmoving!=0);

    glEnable(GL_POLYGON_OFFSET_LINE);

#ifdef BFRONTIER
#define planargrid(q,r,s) \
		for (float v = 0.f; v < (hdr.worldsize-s); v += s) \
		{ \
			vec a; \
			a = q; a.x = v; boxs3D(a, r, s); \
			a = q; a.y = v; boxs3D(a, r, s); \
			a = q; a.z = v; boxs3D(a, r, s); \
		}
#endif

	if(!moving && !hovering)
	{
        if(hmapsel)
            glColor3ub(0,200,0);
        else
		glColor3ub(120,120,120);
		boxs(orient, lu.tovec(), vec(lusize));
#ifdef BFRONTIER
		if (showcursorgrid)
		{
			glColor3ub(0, 0, 10);
			planargrid(lu.tovec(), vec(1, 1, 1), gridsize);
		}
#endif
	}

	// selections
    if(havesel)
	{
		d = dimension(sel.orient);
		glColor3ub(50,50,50);	// grid
		boxsgrid(sel.orient, sel.o.tovec(), sel.s.tovec(), sel.grid);
		glColor3ub(200,0,0);	// 0 reference
		boxs3D(sel.o.tovec().sub(0.5f*min(gridsize*0.25f, 2)), vec(min(gridsize*0.25f, 2)), 1);
		glColor3ub(200,200,200);// 2D selection box
		vec co(sel.o.v), cs(sel.s.v);
		co[R[d]] += 0.5f*(sel.cx*gridsize);
		co[C[d]] += 0.5f*(sel.cy*gridsize);
		cs[R[d]]  = 0.5f*(sel.cxs*gridsize);
		cs[C[d]]  = 0.5f*(sel.cys*gridsize);		
		cs[D[d]] *= gridsize;
		boxs(sel.orient, co, cs);
		glColor3ub(0,0,120);	 // 3D selection box
		boxs3D(sel.o.tovec(), sel.s.tovec(), sel.grid);
#ifdef BFRONTIER
		if (showselgrid)
		{
			glColor3ub(10, 10, 10);
			planargrid(sel.o.tovec(), sel.s.tovec(), sel.grid);
		}
#endif
	}
	
    glDisable(GL_POLYGON_OFFSET_LINE);

	glDisable(GL_BLEND);
}

//////////// ready changes to vertex arrays ////////////

void readychanges(block3 &b, cube *c, const ivec &cor, int size)
{
	loopoctabox(cor, size, b.o, b.s)
	{
		ivec o(i, cor.x, cor.y, cor.z, size);
		if(c[i].ext)
		{
			if(c[i].ext->va)			 // removes va s so that octarender will recreate
			{
				int hasmerges = c[i].ext->va->hasmerges;
				destroyva(c[i].ext->va);
				c[i].ext->va = NULL;
				if(hasmerges) invalidatemerges(c[i]);
			}
			freeoctaentities(c[i]);
		}
		if(c[i].children)
		{
			if(size<=(8>>VVEC_FRAC))
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

void changed(const block3 &sel)
{
	if(sel.s == vec(0)) return;
	block3 b = sel;
	loopi(3) b.s[i] *= b.grid;
	b.grid = 1;
	loopi(3)					// the changed blocks are the selected cubes
	{
		b.o[i] -= 1;
		b.s[i] += 2;
		readychanges(b, worldroot, vec(0), hdr.worldsize/2);
		b.o[i] += 1;
		b.s[i] -= 2;
	}

	inbetweenframes = false;
	octarender();
	inbetweenframes = true;
	setupmaterials();
	invalidatereflections();
	entitiesinoctanodes();
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

block3 *blockcopy(const block3 &s, int rgrid)
{
	block3 *b = (block3 *)new uchar[sizeof(block3)+sizeof(cube)*s.size()];
	*b = s;
	cube *q = b->c();
	loopxyz(s, rgrid, *q++ = copycube(c));
	return b;
}

void freeblock(block3 *b)
{
	cube *q = b->c();
	loopi(b->size()) discardchildren(*q++);
	delete[] b;
}

int *selgridmap(selinfo &sel)							// generates a map of the cube sizes at each grid point
{
	int *g = new int[sel.size()];
	loopxyz(sel, -sel.grid, (*g++ = lusize, c));
	return g-sel.size();
}

vector<undoblock> undos;								// unlimited undo
vector<undoblock> redos;
VARP(undomegs, 0, 5, 100);							  // bounded by n megs

void freeundo(undoblock u)
{
	if(u.g) delete[] u.g;
	if(u.b) freeblock(u.b);
	if(u.e) delete[] u.e;
}

void pasteundo(undoblock &u)
{
	if(u.g)
	{
		int *g = u.g;
		cube *s = u.b->c();
		loopxyz(*u.b, *g++, pastecube(*s++, c));
	}
	pasteundoents(u);
}

void pruneundos(int maxremain)						  // bound memory
{
	int t = 0, p = 0;
	loopvrev(undos)
	{
		undoblock &u = undos[i];
		if(u.b)
		{
			cube *q = u.b->c();
			t += u.b->size()*sizeof(int);
			loopj(u.b->size())
				t += familysize(*q++)*sizeof(cube);
		}
		t += u.n*sizeof(undoent);
		if(t>maxremain) freeundo(undos.remove(i)); else p = t;
	}
	//conoutf("undo: %d of %d(%%%d)", p, undomegs<<20, p*100/(undomegs<<20));
	while(!redos.empty()) { freeundo(redos.pop()); }
}

void initundocube(undoblock &u, selinfo &sel)
{
	u.g = selgridmap(sel);
	u.b = blockcopy(sel, -sel.grid);
}

void addundo(undoblock &u)
{
	undos.add(u);
	pruneundos(undomegs<<20);
}

void makeundo()						// stores state of selected cubes before editing
{
	if(lastsel==sel || sel.s==vec(0)) return;
	lastsel=sel;
#ifdef BFRONTIER
    if(otherclients(false)) return;
#else
    if(multiplayer(false)) return;
#endif
	undoblock u;
	initundocube(u, sel);
	addundo(u);
}

void swapundo(vector<undoblock> &a, vector<undoblock> &b, const char *s)
{
#ifdef BFRONTIER
	if(noedit() || otherclients()) return;
#else
	if(noedit() || multiplayer()) return;
#endif
	if(a.empty()) { conoutf("nothing more to %s", s); return; }	
	int ts = a.last().ts;
	while(!a.empty() && ts==a.last().ts)
	{
		undoblock u = a.pop();
		if(u.b)
		{
			sel.o = u.b->o;
			sel.s = u.b->s;
			sel.grid = u.b->grid;
			sel.orient = u.b->orient;
		}
		undoblock r;
		if(u.g) initundocube(r, sel);
		if(u.n) copyundoents(r, u);
		b.add(r);
		pasteundo(u);
		if(u.b) changed(sel);
		freeundo(u);
	}
	reorient();
	forcenextundo();
}

void editundo() { swapundo(undos, redos, "undo"); }
void editredo() { swapundo(redos, undos, "redo"); }

editinfo *localedit = NULL;

void freeeditinfo(editinfo *&e)
{
	if(!e) return;
	if(e->copy) freeblock(e->copy);
	delete e;
	e = NULL;
}

// guard against subdivision
#define protectsel(f) { undoblock _u; initundocube(_u, sel); f; pasteundo(_u); freeundo(_u); }

void mpcopy(editinfo *&e, selinfo &sel, bool local)
{
	if(local) cl->edittrigger(sel, EDIT_COPY);
	if(e==NULL) e = new editinfo;
	if(e->copy) freeblock(e->copy);
	e->copy = NULL;
	protectsel(e->copy = blockcopy(block3(sel), sel.grid));
	changed(sel);
}

void mppaste(editinfo *&e, selinfo &sel, bool local)
{
	if(e==NULL) return;
	if(local) cl->edittrigger(sel, EDIT_PASTE);
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

void pastehilite()
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

COMMAND(copy, "");
COMMAND(pastehilite, "");
COMMAND(paste, "");
COMMANDN(undo, editundo, "");
COMMANDN(redo, editredo, "");

///////////// height maps ////////////////

void createheightmap() {}

union cface { uchar edge[4]; uint face; };

#define MAXBRUSH    64
#define MAXBRUSH2   32
typedef union { uchar edge[4]; uint face; } brushval;
brushval brush[MAXBRUSH][MAXBRUSH];
VAR(brushx, 0, MAXBRUSH2, MAXBRUSH);
VAR(brushy, 0, MAXBRUSH2, MAXBRUSH);
int brushmaxx = 0;
int brushmaxy = 0;

void clearbrush()
{
	loopi(MAXBRUSH) loopj(MAXBRUSH)
		brush[i][j].face = 0;
	brushmaxx = 0;
	brushmaxy = 0;
}

void brushvert(int *x, int *y, int *v)
{
    *x += MAXBRUSH2 - brushx;
    *y += MAXBRUSH2 - brushy;
	if(*x<0 || *y<0 || *x>=MAXBRUSH || *y>=MAXBRUSH) return;
	brush[*x][*y].edge[3]	 = clamp(*v, 0, 8);
	brush[*x][*y+1].edge[1]	= clamp(*v, 0, 8);
	brush[*x+1][*y].edge[2]	= clamp(*v, 0, 8);
	brush[*x+1][*y+1].edge[0] = clamp(*v, 0, 8);
	brushmaxx = max(brushmaxx, *x+1);
	brushmaxy = max(brushmaxy, *y+1);
}

#define HMAPTEXMAX  64
int htextures[HMAPTEXMAX];
int htexsize = 0;

COMMAND(clearbrush, "");
COMMAND(brushvert, "iii");
ICOMMAND(hmapaddtex, "", (), htextures[htexsize++] = lookupcube(cur.x, cur.y, cur.z).texture[horient = orient]);
ICOMMAND(hmapcancel, "", (), htexsize = 0; );

bool ischildless(cube &c)
{
    if(!c.children)
        return true;
    loopi(8)
    {
        if(!ischildless(c.children[i]) || !isempty(c.children[i]))
            return false;
    }
    emptyfaces(c);
    discardchildren(c);    
    return true;
}

inline bool ishtexture(int t)
{    
    loopi(htexsize) // TODO: optimize with special tex index region
        if(t == htextures[i])
            return true;
    return false;
}

inline bool isheightmap(int o, int d, bool empty, cube *c) 
{
    return ischildless(*c) && 
           ( (empty && isempty(*c)) ||
           (         
            (c->faces[R[d]] & 0x77777777) == 0 &&
            (c->faces[C[d]] & 0x77777777) == 0 &&
            ishtexture(c->texture[o])
           ));
}

namespace hmap 
{
    // flags: 1) zovr = 0x80808080 2) z = 0x7F
#define PAINTED     0x100
#define NOTHMAP     0x200
    ushort flags[MAXBRUSH][MAXBRUSH];
    uint   mask [MAXBRUSH][MAXBRUSH];
	
	selinfo changes;
	int d, dc, dr, biasup, br;
    int gx, gy, gz, mx, my, mz, nx, ny, nz;
    uint fs, fn, fo, f1, fg;
    ushort tex;

    inline uint greytoggle(uint a) { return (a&0xFFFF0000) + ((a&0xFF00)>>8) + ((a&0xFF)<<8); }
    inline uint bitnormal(uint a)  { return 0x01010101 & (a | (a>>1) | (a>>2) | (a>>3)); }    
    inline uint bitspread(uint a)  { return greytoggle(((a>>24)&0x000000FF) | ((a<<8)&0xFFFFFF00) | ((a<<24)&0xFF000000) | ((a>>8)&0x00FFFFFF) | a); };

    inline cube *getcube(ivec t, int f) 
    {
        t[d] += f;    
        cube *c = &lookupcube(t.x, t.y, t.z, -gridsize);
        if(!isheightmap(sel.orient, d, true, c)) return NULL;        
		if(lusize > gridsize)
            c = &lookupcube(t.x, t.y, t.z, gridsize);
        discardchildren(*c);    
        if     (t.x < changes.o.x) changes.o.x = t.x;
        else if(t.x > changes.s.x) changes.s.x = t.x;
        if     (t.y < changes.o.y) changes.o.y = t.y;
        else if(t.y > changes.s.y) changes.s.y = t.y;
        if     (t.z < changes.o.z) changes.o.z = t.z;
        else if(t.z > changes.s.z) changes.s.z = t.z;
		return c;
	}

    inline uint getface(cube *c, int d)
    {
        return  0x0f0f0f0f & ((dc ? c->faces[d] : 0x88888888 - c->faces[d]) >> fs);
    }

	inline void sethface(cube *c, uint v)
	{
        if(v) 
		{
            loopi(6) c->texture[i] = tex;       
			c->faces[R[d]] = F_SOLID;
			c->faces[C[d]] = F_SOLID;		
			c->faces[D[d]] = v | fo;
		}
		else
			emptyfaces(*c);		
	}

	inline void pushside(uint &face, uint v)
	{	// push side for triangle top
        if(v) 
        {
		uint pm = (v&0xffff)>0 ? 0x00ff00ff : 0xff00ff00;
		uint pv = (v&0xff00ff)>0 ? 0 : 0x88888888 & (~pm);
		face = face & pm | pv;		
	}
    }    
 
    void printtab(int tab) 
    {
        loopi(tab)
            printf(" ");       
    }

//#define DPRINT fflush(stdout); printtab(tab); printf
#define DPRINT 

    void hedit(int x, int y, int z, uint snap, int tab)
    {
        DPRINT("%d %d %d [%x] %x\n", x, y, z, snap, flags[x][y]);
        // return early if possible
        if((NOTHMAP & flags[x][y])) return ;
        bool painted = (flags[x][y] & PAINTED)!=0;
        snap &= mask[x][y];
        if(painted && 0 == snap) return;        
        uint paint = (snap*8) + (painted ? 0 : brush[x][y].face & ~(snap*7));        
        if (!paint) return;
        if (!painted)
            flags[x][y] |= PAINTED | (z + 64);  
        else 
            z = (flags[x][y] & 0x7f) - 64;
        if(snap) 
            mask[x][y] &= ~snap;
	
        // get cubes and initialize
		cube *a = NULL, *b = NULL, *c, *e = NULL;				
		ivec t(d, x+gx, y+gy, z+gz);		
        t.shl(gridpower);
        uint face;
      
        c = getcube(t, 0);
        if(!c || isempty(*c)) 
        {
            DPRINT("DROP\n");
            z -= f1; // drop down
            t[d] -= fg;
            e = c;
            c = getcube(t, 0);            
        } 
        if(!c || isempty(*c))
        {
            DPRINT("NOT\n");            
            flags[x][y] |= NOTHMAP;
            return;
        }

        if(!e)
        { 
            e = getcube(t, fg);
        }

        face = getface(c, d);                          
        b = getcube(t, -(int)fg);
        a = getcube(t, -(int)fg-(int)fg);
        b = !b || isempty(*b) ? NULL : b;
        a = !a || isempty(*a) ? NULL : a;
        
        if(painted)
            {
            face += e ? getface(e, d) : 0;
            face += b ? getface(b, d) : 0x08080808;             
            face += a ? getface(a, d) : 0x08080808;
            }
        else
        {
            if(face == 0x08080808 && (!e || !isempty(*e)))
            {
                DPRINT("WALL\n");            
                flags[x][y] |= NOTHMAP;
                return;
		}
        
            flags[x][y]  = PAINTED | (z + 64);                    
            mask [x][y] |= ((face & 0x08080808)<<4);

        if(c->faces[R[d]] == F_SOLID)           // was single
			face += 0x08080808;	  
            else if(b)                      // was pair
                face += b ? getface(b, d) : 0x08080808;

            face += 0x08080808;             // a            
        }
		
        // assert face .edges >= 8
        // assert paint.edges <= 8

        { // debug            
        DPRINT("face %x = %x + %x : %p %p \n", face, c->faces[d], b ? b->faces[d] : 0, c, b);       
        DPRINT("brush %x + %x { %x } \n", brush[x][y].face, snap, paint);
        cface test, ext;
        test.face = paint;
        ext.face  = snap;

        loopi(4) // assertion
            if(test.edge[i] > 8) 
            {
                DPRINT("WARNING!!!!!\n");            
            }
        }

        if(dr > 0) face += snap*7;
        face &= ~(snap * 7);        
        face += -dr * paint;
       
		// seperate heights into 4 layers
        uint hvn =(face & 0x20202020) >> 2; 
		uint ovr =(face & 0x10101010) >> 1;
		uint mid = face & 0x08080808;				
        uint sky = ovr & mid;
             ovr&= ~sky;
             mid&= ~sky;
        uint oh  = face & ((sky>>3) * 0x7) | hvn;
        uint hi  = face & ((ovr>>3) * 0x7) | hvn | sky;
        uint lo  = face & ((mid>>3) * 0x7) | hvn | sky | ovr;
        uint ul  = face - oh - hi - lo;                
        DPRINT("sky: %x | %x | %x | %x \n", hvn, sky, ovr, mid);        
        DPRINT("raw: %x = %x %x %x %x\n", face, oh, hi, lo, ul);        
				
		// cubify to 2 layers, apply bias
        bool ispair = 0;
		uint *top, *base;  
        if(hi==0x08080808)      { top = &oh; }
        else if(lo==0)          { top = &ul; }
        else if(oh && (biasup || lo==0x08080808))
		{			
            snap += 0x08080808 - lo;
			top  = &oh;
			base = &hi;
            lo   = 0x08080808;
			ispair = 1;
		}		
		else if(lo==0x08080808) { top = &hi; }
		else if(biasup || ul==0x08080808)
		{
            snap += 0x08080808 - ul + oh;
			oh	= 0;
			top  = &hi;
			base = &lo;
			ul	= 0x08080808;
			ispair = 1;			
		}
        else if(ul==0x08080808) { top = &lo; }
		else
		{
            snap += hi;
            hi   = 0;
			top  = &lo;
			base = &ul;
			ispair = 1;
		}		

		if(ispair)
		{			
            if(biasup) 
            {                
                uint bup = bitspread(greytoggle(bitnormal(*top)));
                int old = *base;
			*base &= ~(bup * 7);
			*base |=	bup << 3;						
                snap += *base - old;
            } 
            else 
            {
                uint bup = ~bitspread(~greytoggle((*base & 0x08080808) >> 3));
                int old = *top;
                *top &= bup * 0xff;
                snap += old - *top;
            }
		}		

        ispair =   *top &&
                 !(*top&0xffffff00 && 
                   *top&0xffff00ff && 
                   *top&0xff00ffff && 
                   *top&0x00ffffff );

		// apply to cubes
        tex = c->texture[sel.orient];
		oh <<= fs;
		hi <<= fs;
		lo <<= fs;
		ul <<= fs;
		if(e) sethface(e, oh);
		if(c) sethface(c, hi);
		if(b) sethface(b, lo);
		if(a) sethface(a, ul);		
		if(ispair) 
		{
                 if(e && top==&oh) pushside(e->faces[R[d]], oh);
            else if(c && top==&hi) pushside(c->faces[R[d]], hi);
            else if(b && top==&lo) pushside(b->faces[R[d]], lo);
		}
		
        snap = bitnormal(snap);

        if(snap) 
            mask[x][y] &= ~snap;          

        DPRINT("new: %x %x %x %x (%x) mask %x\n", oh, hi, lo, ul, snap, mask[x][y]);
 
        z = (flags[x][y] & 0x7f) - 64; 
        uint zovr = mask[x][y];
        
        DPRINT("info %x z %d\n", flags[x][y], z);
         
        // continue to adjacent cubes
        // backtrack first to save some stack
        // TOFIX: not endian friendly
        if(x>mx) hedit(x-1, y, (zovr&0x00800080?z+f1:z), (snap<<8) &0x0f000f00, tab+1);
        if(x<nx) hedit(x+1, y, (zovr&0x80008000?z+f1:z), (snap>>8) &0x000f000f, tab+1);        
        if(y>my) hedit(x, y-1, (zovr&0x00008080?z+f1:z), (snap<<16)&0x0f0f0000, tab+1);
        if(y<ny) hedit(x, y+1, (zovr&0x80800000?z+f1:z), (snap>>16)&0x00000f0f, tab+1);        
    }

    void run(int dir, int mode) 
    {                 
        d  = dimension(sel.orient);
        dc = dimcoord(sel.orient);
        dr = dir;
        br = dir>0 ? 0x08080808 : 0;
     //   biasup = mode == dir<0;
        biasup = dir<0;
        int cx = (sel.corner&1 ? 0 : -1);
        int cy = (sel.corner&2 ? 0 : -1);
        gx = (cur[R[d]] >> gridpower) + cx - MAXBRUSH2;
        gy = (cur[C[d]] >> gridpower) + cy - MAXBRUSH2;
        gz = (cur[D[d]] >> gridpower);
        fs = dc ? 4 : 0;
        fo = dc ? 0 : F_SOLID;
        fn = 0x0f0f0f0f << (4-fs);    
        f1 = dc ? 1 : -1;
        fg = dc ? gridsize : -gridsize;
        mx = max(0, -gx);
        my = max(0, -gy);
        mz = -gz;
        nx = min(MAXBRUSH, (hdr.worldsize>>gridpower)-gx) - 1;
        ny = min(MAXBRUSH, (hdr.worldsize>>gridpower)-gy) - 1;
        nz = (hdr.worldsize>>gridpower)-gz - 1;
        
        loopi(MAXBRUSH) loopj(MAXBRUSH) 
        { // TODO: init in hedit
            mask [i][j] = 0x01010101;
            flags[i][j] = 0;
        }
        int x = clamp(MAXBRUSH2-cx, mx, nx);
        int y = clamp(MAXBRUSH2-cy, my, ny);
        int z = f1;        
        
   //     printf("----------------\n%x g %d %d %d \n----------------\n", fs, gx, gy, gz);
   //     printf(" cur %d %d %d : %d\n", cur.x, cur.y, cur.z, gridsize);

        changes.grid = gridsize;
        changes.s = changes.o = cur;
        hedit(x, y, z, 0, 0);
        changes.s.sub(changes.o).shr(gridpower).add(1);
        changed(changes);
    }
}

void edithmap(int dir, int mode) {    

#ifdef BFRONTIER
    if(otherclients()) return;
#else
    if(multiplayer()) return;
#endif
    if (!dimcoord(sel.orient))
    {
        conoutf("negative orientations not supported yet");
        return;
	}
 //   long time = SDL_GetTicks();
    hmap::run(dir, mode);    
 //   conoutf("-- edit time (%d) ms --", SDL_GetTicks()-time);
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

VAR(invalidcubeguard, 0, 1, 1);

void mpeditface(int dir, int mode, selinfo &sel, bool local)
{
	if(mode==1 && (sel.cx || sel.cy || sel.cxs&1 || sel.cys&1)) mode = 0;
	int d = dimension(sel.orient);
	int dc = dimcoord(sel.orient);
	int seldir = dc ? -dir : dir;

	if(local)
		cl->edittrigger(sel, EDIT_FACE, dir, mode);

	if(mode==1)
	{
		int h = sel.o[d]+dc*sel.grid;
		if((dir>0 == dc && h<=0) || (dir<0 == dc && h>=hdr.worldsize)) return;
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
					c.texture[i] = o.texture[i];
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
				loop(mx,2) loop(my,2)										// pull/push edges command
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

VAR(selectionsurf, 0, 0, 1);

void pushsel(int *dir)
{
	if(noedit(moving!=0)) return;
	int d = dimension(orient);
	int s = dimcoord(orient) ? -*dir : *dir;
	sel.o[d] += s*sel.grid;
	if(selectionsurf==1) player->o[d] += s*sel.grid;
}

void mpdelcube(selinfo &sel, bool local)
{
	if(local) cl->edittrigger(sel, EDIT_DELCUBE);
	loopselxyz(discardchildren(c); emptyfaces(c));
}

void delcube() 
{
	if(noedit()) return;
	mpdelcube(sel, true);
}

COMMAND(pushsel, "i");
COMMAND(editface, "ii");
COMMAND(delcube, "");

/////////// texture editing //////////////////

int curtexindex = -1, lasttex = 0;
int texpaneltimer = 0;
vector<ushort> texmru;

void tofronttex()										// maintain most recently used of the texture lists when applying texture
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

extern int curtexnum;
VAR(allfaces, 0, 0, 1);

void mpedittex(int tex, int allfaces, selinfo &sel, bool local)
{
	if(local)
	{
		cl->edittrigger(sel, EDIT_TEX, tex, allfaces);
		if(allfaces || !(repsel == sel)) reptex = -1;
		repsel = sel;
	}
	bool findrep = local && !allfaces && reptex < 0;
	loopselxyz(edittexcube(c, tex, allfaces ? -1 : sel.orient, findrep));
}

void filltexlist()
{
	if(texmru.length()!=curtexnum)
	{
		loopv(texmru) if(texmru[i]>=curtexnum) texmru.remove(i--);
		loopi(curtexnum) if(texmru.find(i)<0) texmru.add(i);
	}
}

void edittex(int i)
{
	curtexindex = i = min(max(i, 0), curtexnum-1);
    int t = lasttex = texmru[i];    
	mpedittex(t, allfaces, sel, true);
}

void edittex_(int *dir)
{
	if(noedit()) return;
	filltexlist();
	texpaneltimer = 5000;
	if(!(lastsel==sel)) tofronttex();
	edittex(curtexindex<0 ? 0 : curtexindex+*dir);
}

void gettex()
{
	if(noedit()) return;
	filltexlist();
	loopxyz(sel, sel.grid, curtexindex = c.texture[sel.orient]);
	loopi(curtexnum) if(texmru[i]==curtexindex)
	{
		curtexindex = i;
		tofronttex();
		return;
	}
}

COMMANDN(edittex, edittex_, "i");
COMMAND(gettex, "");

void replacetexcube(cube &c, int oldtex, int newtex)
{
	loopi(6) if(c.texture[i] == oldtex) c.texture[i] = newtex;
	if(c.children) loopi(8) replacetexcube(c.children[i], oldtex, newtex);
}

void mpreplacetex(int oldtex, int newtex, selinfo &sel, bool local)
{
	if(local) cl->edittrigger(sel, EDIT_REPLACE, oldtex, newtex);
	loopi(8) replacetexcube(worldroot[i], oldtex, newtex);
	allchanged();
}

void replace()
{
	if(noedit()) return;
	if(reptex < 0) { conoutf("can only replace after a texture edit"); return; }
	mpreplacetex(reptex, lasttex, sel, true);
}

COMMAND(replace, "");

////////// flip and rotate ///////////////
uint dflip(uint face) { return face==F_EMPTY ? face : 0x88888888 - (((face&0xF0F0F0F0)>>4)+ ((face&0x0F0F0F0F)<<4)); }
uint cflip(uint face) { return ((face&0xFF00FF00)>>8) + ((face&0x00FF00FF)<<8); }
uint rflip(uint face) { return ((face&0xFFFF0000)>>16)+ ((face&0x0000FFFF)<<16); }
uint mflip(uint face) { return (face&0xFF0000FF) + ((face&0x00FF0000)>>8) + ((face&0x0000FF00)<<8); }

void flipcube(cube &c, int d)
{
	swap(ushort, c.texture[d*2], c.texture[d*2+1]);
	c.faces[D[d]] = dflip(c.faces[D[d]]);
	c.faces[C[d]] = cflip(c.faces[C[d]]);
	c.faces[R[d]] = rflip(c.faces[R[d]]);
	if (c.children)
	{
		loopi(8) if (i&octadim(d)) swap(cube, c.children[i], c.children[i-octadim(d)]);
		loopi(8) flipcube(c.children[i], d);
	}
}

void rotatequad(cube &a, cube &b, cube &c, cube &d)
{
	cube t = a; a = b; b = c; c = d; d = t;
}

void rotatecube(cube &c, int d)	// rotates cube clockwise. see pics in cvs for help.
{
	c.faces[D[d]] = cflip (mflip(c.faces[D[d]]));
	c.faces[C[d]] = dflip (mflip(c.faces[C[d]]));
	c.faces[R[d]] = rflip (mflip(c.faces[R[d]]));
	swap(uint, c.faces[R[d]], c.faces[C[d]]);

	swap(uint, c.texture[2*R[d]], c.texture[2*C[d]+1]);
	swap(uint, c.texture[2*C[d]], c.texture[2*R[d]+1]);
	swap(uint, c.texture[2*C[d]], c.texture[2*C[d]+1]);

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
	if(local) cl->edittrigger(sel, EDIT_FLIP);
	int zs = sel.s[dimension(sel.orient)];
	makeundo();
	loopxy(sel)
	{
		loop(z,zs) flipcube(selcube(x, y, z), dimension(sel.orient));
		loop(z,zs/2)
		{
			cube &a = selcube(x, y, z);
			cube &b = selcube(x, y, zs-z-1);
			swap(cube, a, b);
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
	if(local) cl->edittrigger(sel, EDIT_ROTATE, cw);
	int d = dimension(sel.orient);
	if(!dimcoord(sel.orient)) cw = -cw;
	int &m = min(sel.s[C[d]], sel.s[R[d]]);
	int ss = m = max(sel.s[R[d]], sel.s[C[d]]);
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

COMMAND(flip, "");
COMMAND(rotate, "i");

void setmat(cube &c, uchar mat)
{
	if(c.children)
		loopi(8) setmat(c.children[i], mat);
	else if(mat!=MAT_AIR) ext(c).material = mat;
	else if(c.ext) c.ext->material = MAT_AIR;
}

void mpeditmat(int matid, selinfo &sel, bool local)
{
	if(local) cl->edittrigger(sel, EDIT_MAT, matid);
	loopselxyz(setmat(c, matid));
}

void editmat(char *name)
{
	if(noedit()) return;
#ifdef BFRONTIER
	int id = findmaterial(name, true);
#else
	int id = findmaterial(name);
#endif
	if(id<0) { conoutf("unknown material \"%s\"", name); return; }
	mpeditmat(id, sel, true);
}

COMMAND(editmat, "s");

#define TEXTURE_WIDTH 10
#define TEXTURE_HEIGHT 7
extern int menudistance, menuautoclose;

VAR(thumbtime, 0, 50, 1000);

static int lastthumbnail = 0;

struct texturegui : g3d_callback 
{
	bool menuon;
	vec menupos;
	int menustart;
	
	void gui(g3d_gui &g, bool firstpass)
	{
		int menutab = 1+curtexindex/(TEXTURE_WIDTH*TEXTURE_HEIGHT);		
		int origtab = menutab;
		g.start(menustart, 0.04f, &menutab);
		loopi(1+curtexnum/(TEXTURE_WIDTH*TEXTURE_HEIGHT))
		{	
			g.tab((i==0)?"Textures":NULL, 0xAAFFAA);
			if(i != origtab-1) continue; //don't load textures on non-visible tabs!
			loopj(TEXTURE_HEIGHT) 
			{
				g.pushlist();
				loopk(TEXTURE_WIDTH) 
				{
					int ti = (i*TEXTURE_HEIGHT+j)*TEXTURE_WIDTH+k;
					if(ti<curtexnum) 
					{
                        Texture *tex = notexture;
						Slot &slot = lookuptexture(texmru[ti], false);
						if(slot.sts.empty()) continue;
						else if(slot.loaded) tex = slot.sts[0].t;
						else if(slot.thumbnail) tex = slot.thumbnail;
						else if(lastmillis-lastthumbnail>=thumbtime) { tex = loadthumbnail(slot); lastthumbnail = lastmillis; }
                        if(g.texture(tex, 1.0)&G3D_UP && (slot.loaded || tex!=notexture)) 
							edittex(ti);
					}
					else
                        g.texture(notexture, 1.0); //create an empty space
				}
				g.poplist();
			}
		}
		g.end();
		if(origtab != menutab) curtexindex = (menutab-1)*TEXTURE_WIDTH*TEXTURE_HEIGHT;
	}

	void showtextures(bool on)
	{
		if(on != menuon && (menuon = on)) { menupos = menuinfrontofplayer(); menustart = starttime(); }
	}

	void show()
	{	
		if(!menuon) return;
		filltexlist();
		if(!editmode || camera1->o.dist(menupos) > menuautoclose) menuon = false;
		else g3d_addgui(this, menupos); //follow?
	}
} gui;

void g3d_texturemenu() 
{ 
	gui.show(); 
}

void showtexgui(int *n) { gui.showtextures(((*n==0) ? !gui.menuon : (*n==1)) && editmode); }

// 0/noargs = toggle, 1 = on, other = off - will autoclose if too far away or exit editmode
COMMAND(showtexgui, "i");

void render_texture_panel(int w, int h)
{
	if((texpaneltimer -= curtime)>0 && editmode)
	{
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
			if(ti>=0 && ti<curtexnum)
			{
				Texture *tex = lookuptexture(texmru[ti]).sts[0].t;
				float sx = min(1, tex->xs/(float)tex->ys), sy = min(1, tex->ys/(float)tex->xs);
				glBindTexture(GL_TEXTURE_2D, tex->gl);
				glColor4f(0, 0, 0, texpaneltimer/1000.0f);
				int x = width-s-50, r = s;
				loopj(2)
				{
					glBegin(GL_QUADS);
					glTexCoord2f(0.0,	0.0);	glVertex2f(x,	y);
					glTexCoord2f(1.0/sx, 0.0);	glVertex2f(x+r, y);
					glTexCoord2f(1.0/sx, 1.0/sy); glVertex2f(x+r, y+r);
					glTexCoord2f(0.0,	1.0/sy); glVertex2f(x,	y+r);
					glEnd();
					xtraverts += 4;
					glColor4f(1.0, 1.0, 1.0, texpaneltimer/1000.0f);
					r -= 10;
					x += 5;
					y += 5;
				}
			}
			y += s+gap;
		}
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}
