// renderparticles.cpp

#include "engine.h"
#include "rendertarget.h"

Shader *particleshader = NULL, *particlenotextureshader = NULL;

VARFP(maxparticles, 16, 4096, INT_MAX-1, particleinit());
VARA(maxparticledistance, 256, 1024, INT_MAX-1);
VARA(maxparticletrail, 256, 1024, INT_MAX-1);

VARP(particletext, 0, 1, 1);
VARP(particleglare, 0, 1, 100);
VAR(debugparticles, 0, 0, 1);

// Check emit_particles() to limit the rate that paricles can be emitted for models/sparklies
// Automatically stops particles being emitted when paused or in reflective drawing
VARP(emitmillis, 0, 15, INT_MAX-1);
static int lastemitframe = 0;
static bool emit = false;

static bool emit_particles()
{
	if(reflecting || refracting) return false;
	return emit;
}

const char *partnames[] = { "part", "tape", "trail", "text", "fireball", "lightning", "flare", "portal", "icon", "line", "triangle", "ellipse", "cone" };

struct partvert
{
	vec pos;
	float u, v;
	bvec color;
	uchar alpha;
};

#define COLLIDERADIUS 8.0f
#define COLLIDEERROR 1.0f

struct partrenderer
{
	Texture *tex;
	const char *texname;
	uint type;

	partrenderer(const char *texname, int type)
		: tex(NULL), texname(texname), type(type) { }
	virtual ~partrenderer() { }

	virtual void init(int n) { }
	virtual void reset() = NULL;
	virtual void resettracked(physent *owner) { }
	virtual particle *addpart(const vec &o, const vec &d, int fade, int color, float size, int grav = 0, int collide = 0, physent *pl = NULL) = NULL;
	virtual int adddepthfx(vec &bbmin, vec &bbmax) { return 0; }
	virtual void update() { }
	virtual void render() = NULL;
	virtual bool haswork() = NULL;
	virtual int count() = NULL; //for debug
	virtual bool usesvertexarray() { return false; }
	virtual void cleanup() {}

	void preload()
	{
		if(texname && (!tex || tex == notexture))
			tex = textureload(texname);
	}

	//blend = 0 => remove it
	void calc(particle *p, int &blend, int &ts, bool lastpass = true)
	{
		vec o = p->o;
		game::particletrack(p, type, ts, lastpass);
		if(p->fade <= 5)
		{
			ts = 1;
			blend = 255;
		}
		else
		{
			ts = lastmillis-p->millis;
			blend = max(255 - (ts<<8)/p->fade, 0);
			if(p->grav)
			{
				if(ts > p->fade) ts = p->fade;
				float secs = curtime/1000.f;
				vec v = vec(p->d).mul(secs);
				static physent dummy;
				dummy.weight = p->grav;
				v.z -= physics::gravityforce(&dummy)*secs;
				p->o.add(v);
			}
			if(p->collide && p->o.z < p->val && lastpass)
			{
				vec surface;
				float floorz = rayfloor(vec(p->o.x, p->o.y, p->val), surface, RAY_CLIPMAT, COLLIDERADIUS);
				float collidez = floorz<0 ? o.z-COLLIDERADIUS : p->val - rayfloor(vec(p->o.x, p->o.y, p->val), surface, RAY_CLIPMAT, COLLIDERADIUS);
				if(p->o.z >= collidez+COLLIDEERROR) p->val = collidez+COLLIDEERROR;
				else
				{
					adddecal(p->collide, vec(p->o.x, p->o.y, collidez), vec(o).sub(p->o).normalize(), 2*p->size, p->color, type&PT_RND4 ? (p->flags>>5)&3 : 0);
					blend = 0;
				}
			}
		}
	}
};

#include "depthfx.h"
#include "lensflare.h"

template<class T>
struct listparticle : particle
{
	T *next;
};

struct sharedlistparticle : listparticle<sharedlistparticle> {};

template<class T>
struct listrenderer : partrenderer
{
	static T *parempty;
	T *list;

	listrenderer(const char *texname, int type)
		: partrenderer(texname, type), list(NULL)
	{
	}

	virtual ~listrenderer()
	{
	}

	virtual void cleanup(T *p)
	{
	}

	void reset()
	{
		if(!list) return;
		T *p = list;
		for(;;)
		{
			cleanup(p);
			if(p->next) p = p->next;
			else break;
		}
		p->next = parempty;
		parempty = list;
		list = NULL;
	}

	void resettracked(physent *pl)
	{
		for(T **prev = &list, *cur = list; cur; cur = *prev)
		{
			if(cur->owner == pl)
			{
				*prev = cur->next;
				cur->next = parempty;
				parempty = cur;
			}
			else prev = &cur->next;
		}
	}

	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, int grav = 0, int collide = 0, physent *pl = NULL)
	{
		if(!parempty)
		{
			T *ps = new T[32];
			loopi(31) ps[i].next = &ps[i+1];
			ps[31].next = parempty;
			parempty = ps;
		}
		T *p = parempty;
		parempty = p->next;
		p->next = list;
		list = p;
		p->o = o;
		p->d = d;
		p->fade = fade;
		p->millis = lastmillis;
		p->color = bvec(color>>16, (color>>8)&0xFF, color&0xFF);
		p->size = size;
		p->grav = grav;
		p->collide = collide;
		p->owner = pl;
		return p;
	}

	int count()
	{
		int num = 0;
		T *lp;
		for(lp = list; lp; lp = lp->next) num++;
		return num;
	}

	bool haswork()
	{
		return (list != NULL);
	}

	virtual void startrender() = 0;
	virtual void endrender() = 0;
	virtual void renderpart(T *p, int blend, int ts, uchar *color) = 0;

	void render()
	{
		preload();
		startrender();
		if(tex) glBindTexture(GL_TEXTURE_2D, tex->id);
		bool lastpass = !reflecting && !refracting;
		for(T **prev = &list, *p = list; p; p = *prev)
		{
			int blend, ts;
			calc(p, blend, ts, lastpass);
			if(blend > 0)
			{
				renderpart(p, blend, ts, p->color.v);

				if(p->fade > 5 || !lastpass)
				{
					prev = &p->next;
					continue;
				}
			}
			//remove
			*prev = p->next;
			p->next = parempty;
			cleanup(p);
			parempty = p;
		}

		endrender();
	}
};

template<class T> T *listrenderer<T>::parempty = NULL;

typedef listrenderer<sharedlistparticle> sharedlistrenderer;

#include "explosion.h"
#include "lightning.h"

struct textrenderer : sharedlistrenderer
{
	textrenderer(int type)
		: sharedlistrenderer(NULL, type)
	{}

	void startrender()
	{
	}

	void endrender()
	{
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
	}

	void cleanup(sharedlistparticle *p)
	{
		if(p->text && p->text[0]=='@') delete[] p->text;
	}

	void renderpart(sharedlistparticle *p, int blend, int ts, uchar *color)
	{
		glPushMatrix();
		glTranslatef(p->o.x, p->o.y, p->o.z);
		if(fogging)
		{
			if(renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - p->o.z, true);
			else blend = (uchar)(blend * max(0.0f, min(1.0f, 1.0f - (reflectz - p->o.z)/waterfog)));
		}
		glRotatef(camera1->yaw-180, 0, 0, 1);
		glRotatef(camera1->pitch-90, 1, 0, 0);
		float scale = p->size/80.0f;
		glScalef(-scale, scale, -scale);
		const char *text = p->text+(p->text[0]=='@' ? 1 : 0);
		static string font; font[0] = 0;
		if(*text == '<')
		{
			const char *start = text;
			while(*text && *text != '>') text++;
			if(*text) { int len = text-(start+1); strncpy(font, start+1, len); font[len] = 0; text++; }
			else text = start;
		}
		float xoff = -text_width(text)/2;
		glTranslatef(xoff, 0, 50);
		pushfont(*font ? font : "default");
		draw_text(text, 0, 0, color[0], color[1], color[2], blend);
		popfont();
		glPopMatrix();
	}
};
static textrenderer texts(PT_TEXT|PT_LERP), textontop(PT_TEXT|PT_LERP|PT_ONTOP);

struct portal : listparticle<portal>
{
	float yaw, pitch;
};

struct portalrenderer : listrenderer<portal>
{
	portalrenderer(const char *texname)
		: listrenderer<portal>(texname, PT_PORTAL|PT_GLARE|PT_LERP)
	{}

	void startrender()
	{
		glDisable(GL_CULL_FACE);
	}

	void endrender()
	{
		glEnable(GL_CULL_FACE);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
	}

	void renderpart(portal *p, int blend, int ts, uchar *color)
	{
		glPushMatrix();
		glTranslatef(p->o.x, p->o.y, p->o.z);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - p->o.z, true);
		glRotatef(p->yaw-180, 0, 0, 1);
		glRotatef(p->pitch, 1, 0, 0);
		glScalef(p->size, p->size, p->size);

		glColor4ub(color[0], color[1], color[2], blend);
		glBegin(GL_QUADS);
		glTexCoord2f(1, 0); glVertex3f(-1, 0,  1);
		glTexCoord2f(0, 0); glVertex3f( 1, 0,  1);
		glTexCoord2f(0, 1); glVertex3f( 1, 0, -1);
		glTexCoord2f(1, 1); glVertex3f(-1, 0, -1);
		glEnd();

		glPopMatrix();
	}

	portal *addportal(const vec &o, int fade, int color, float size, float yaw, float pitch)
	{
		portal *p = (portal *)listrenderer<portal>::addpart(o, vec(0, 0, 0), fade, color, size);
		p->yaw = yaw;
		p->pitch = pitch;
		return p;
	}

	// use addportal() instead
	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, int grav = 0, int collide = 0, physent *pl = NULL) { return NULL; }
};

struct icon : listparticle<icon>
{
	Texture *tex;
	float blend, start, length, end;
};

struct iconrenderer : listrenderer<icon>
{
	Texture *lasttex;

	iconrenderer(int type)
		: listrenderer<icon>(NULL, type)
	{}

	void startrender()
	{
		lasttex = NULL;
	}

	void endrender()
	{
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
	}

	void renderpart(icon *p, int blend, int ts, uchar *color)
	{
		if(p->tex != lasttex)
		{
			glBindTexture(GL_TEXTURE_2D, p->tex->id);
			lasttex = p->tex;
		}

		glPushMatrix();
		glTranslatef(p->o.x, p->o.y, p->o.z);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - p->o.z, true);
		glRotatef(camera1->yaw-180, 0, 0, 1);
		glRotatef(camera1->pitch, 1, 0, 0);
		glScalef(p->size, p->size, p->size);

		glColor4ub(color[0], color[1], color[2], uchar(p->blend*blend));
		if(p->start > 0 || p->length < 1)
		{
			float sx = cosf((p->start + 0.25f)*2*M_PI), sy = -sinf((p->start + 0.25f)*2*M_PI),
				  ex = cosf((p->end + 0.25f)*2*M_PI), ey = -sinf((p->end + 0.25f)*2*M_PI);
			glBegin(GL_TRIANGLE_FAN);
			glTexCoord2f(0.5f, 0.5f); glVertex3f(0, 0, 0);

			if(p->start < 0.125f || p->start >= 0.875f) { glTexCoord2f(0.5f + 0.5f*sx/sy, 0); glVertex3f(-(sx/sy), 0, 1);  }
			else if(p->start < 0.375f) { glTexCoord2f(1, 0.5f - 0.5f*sy/sx); glVertex3f(-1, 0, sy/sx); }
			else if(p->start < 0.625f) { glTexCoord2f(0.5f - 0.5f*sx/sy, 1); glVertex3f(sx/sy, 0, -1); }
			else { glTexCoord2f(0, 0.5f + 0.5f*sy/sx); glVertex3f(1, 0, -(sy/sx)); }

			if(p->start <= 0.125f && p->end >= 0.125f) { glTexCoord2f(1, 0); glVertex3f(-1, 0, 1); }
			if(p->start <= 0.375f && p->end >= 0.375f) { glTexCoord2f(1, 1); glVertex3f(-1, 0, -1); }
			if(p->start <= 0.625f && p->end >= 0.625f) { glTexCoord2f(0, 1); glVertex3f(1, 0, -1); }
			if(p->start <= 0.875f && p->end >= 0.875f) { glTexCoord2f(0, 0); glVertex3f(1, 0, 1); }

			if(p->end < 0.125f || p->end >= 0.875f) { glTexCoord2f(0.5f + 0.5f*ex/ey, 0); glVertex3f(-(ex/ey), 0, 1);  }
			else if(p->end < 0.375f) { glTexCoord2f(1, 0.5f - 0.5f*ey/ex); glVertex3f(-1, 0, ey/ex); }
			else if(p->end < 0.625f) { glTexCoord2f(0.5f - 0.5f*ex/ey, 1); glVertex3f(ex/ey, 0, -1); }
			else { glTexCoord2f(0, 0.5f + 0.5f*ey/ex); glVertex3f(1, 0, -(ey/ex)); }
			glEnd();
		}
		else
		{
			glBegin(GL_QUADS);
			glTexCoord2f(1, 1); glVertex3f(-1, 0, -1);
			glTexCoord2f(0, 1); glVertex3f( 1, 0, -1);
			glTexCoord2f(0, 0); glVertex3f( 1, 0,  1);
			glTexCoord2f(1, 0); glVertex3f(-1, 0,  1);
			glEnd();
		}

		glPopMatrix();
	}

	icon *addicon(const vec &o, Texture *tex, float blend, int fade, int color, float size, int grav, int collide, float start, float length, physent *pl = NULL)
	{
		icon *p = (icon *)listrenderer<icon>::addpart(o, vec(0, 0, 0), fade, color, size, grav, collide);
		p->tex = tex;
		p->blend = blend;
		p->start = start;
		p->length = length;
		p->end = p->start + p->length;
		p->owner = pl;
		return p;
	}

	// use addicon() instead
	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, int grav = 0, int collide = 0, physent *pl = NULL) { return NULL; }
};
static iconrenderer icons(PT_ICON|PT_LERP);

template<int T>
static inline void modifyblend(const vec &o, int &blend)
{
	blend = min(blend<<2, 255);
	if(renderpath==R_FIXEDFUNCTION && fogging) blend = (uchar)(blend * max(0.0f, min(1.0f, 1.0f - (reflectz - o.z)/waterfog)));
}

template<>
inline void modifyblend<PT_TAPE>(const vec &o, int &blend)
{
}

template<int T>
static inline void genpos(const vec &o, const vec &d, float size, int grav, int ts, partvert *vs)
{
	vec udir = vec(camup).sub(camright).mul(size);
	vec vdir = vec(camup).add(camright).mul(size);
	vs[0].pos = vec(o.x + udir.x, o.y + udir.y, o.z + udir.z);
	vs[1].pos = vec(o.x + vdir.x, o.y + vdir.y, o.z + vdir.z);
	vs[2].pos = vec(o.x - udir.x, o.y - udir.y, o.z - udir.z);
	vs[3].pos = vec(o.x - vdir.x, o.y - vdir.y, o.z - vdir.z);
}

template<>
inline void genpos<PT_TAPE>(const vec &o, const vec &d, float size, int ts, int grav, partvert *vs)
{
	vec dir1 = d, dir2 = d, c;
	dir1.sub(o);
	dir2.sub(camera1->o);
	c.cross(dir2, dir1).normalize().mul(size);
	vs[0].pos = vec(d.x-c.x, d.y-c.y, d.z-c.z);
	vs[1].pos = vec(o.x-c.x, o.y-c.y, o.z-c.z);
	vs[2].pos = vec(o.x+c.x, o.y+c.y, o.z+c.z);
	vs[3].pos = vec(d.x+c.x, d.y+c.y, d.z+c.z);
}

template<>
inline void genpos<PT_TRAIL>(const vec &o, const vec &d, float size, int ts, int grav, partvert *vs)
{
	vec e = d;
	if(grav) e.z -= float(ts)/grav;
	e.div(-75.0f);
	e.add(o);
	genpos<PT_TAPE>(o, e, size, ts, grav, vs);
}

template<int T>
static inline void genrotpos(const vec &o, const vec &d, float size, int grav, int ts, partvert *vs, int rot)
{
	genpos<T>(o, d, size, grav, ts, vs);
}

#define ROTCOEFFS(n) { \
	vec(-1,  1, 0).rotate_around_z(n*2*M_PI/32.0f), \
	vec( 1,  1, 0).rotate_around_z(n*2*M_PI/32.0f), \
	vec( 1, -1, 0).rotate_around_z(n*2*M_PI/32.0f), \
	vec(-1, -1, 0).rotate_around_z(n*2*M_PI/32.0f) \
}
static const vec rotcoeffs[32][4] =
{
	ROTCOEFFS(0),  ROTCOEFFS(1),  ROTCOEFFS(2),  ROTCOEFFS(3),  ROTCOEFFS(4),  ROTCOEFFS(5),  ROTCOEFFS(6),  ROTCOEFFS(7),
	ROTCOEFFS(8),  ROTCOEFFS(9),  ROTCOEFFS(10), ROTCOEFFS(11), ROTCOEFFS(12), ROTCOEFFS(13), ROTCOEFFS(14), ROTCOEFFS(15),
	ROTCOEFFS(16), ROTCOEFFS(17), ROTCOEFFS(18), ROTCOEFFS(19), ROTCOEFFS(20), ROTCOEFFS(21), ROTCOEFFS(22), ROTCOEFFS(7),
	ROTCOEFFS(24), ROTCOEFFS(25), ROTCOEFFS(26), ROTCOEFFS(27), ROTCOEFFS(28), ROTCOEFFS(29), ROTCOEFFS(30), ROTCOEFFS(31),
};

template<>
inline void genrotpos<PT_PART>(const vec &o, const vec &d, float size, int grav, int ts, partvert *vs, int rot)
{
	const vec *coeffs = rotcoeffs[rot];
	(vs[0].pos = o).add(vec(camright).mul(coeffs[0].x*size)).add(vec(camup).mul(coeffs[0].y*size));
	(vs[1].pos = o).add(vec(camright).mul(coeffs[1].x*size)).add(vec(camup).mul(coeffs[1].y*size));
	(vs[2].pos = o).add(vec(camright).mul(coeffs[2].x*size)).add(vec(camup).mul(coeffs[2].y*size));
	(vs[3].pos = o).add(vec(camright).mul(coeffs[3].x*size)).add(vec(camup).mul(coeffs[3].y*size));
}

template<int T>
struct varenderer : partrenderer
{
	partvert *verts;
	particle *parts;
	int maxparts, numparts, lastupdate, rndmask;

	varenderer(const char *texname, int type)
		: partrenderer(texname, type),
		  verts(NULL), parts(NULL), maxparts(0), numparts(0), lastupdate(-1), rndmask(0)
	{
		if(type & PT_HFLIP) rndmask |= 0x01;
		if(type & PT_VFLIP) rndmask |= 0x02;
		if(type & PT_ROT) rndmask |= 0x1F<<2;
		if(type & PT_RND4) rndmask |= 0x03<<5;
	}

	void init(int n)
	{
		DELETEA(parts);
		DELETEA(verts);
		parts = new particle[n];
		verts = new partvert[n*4];
		maxparts = n;
		numparts = 0;
		lastupdate = -1;
	}

	void reset()
	{
		numparts = 0;
		lastupdate = -1;
	}

	void resettracked(physent *pl)
	{
		loopi(numparts)
		{
			particle *p = parts+i;
			if(p->owner == pl) p->fade = -1;
		}
		lastupdate = -1;
	}

	int count()
	{
		return numparts;
	}

	bool haswork()
	{
		return (numparts > 0);
	}

	bool usesvertexarray() { return true; }

	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, int grav = 0, int collide = 0, physent *pl = NULL)
	{
		particle *p = parts + (numparts < maxparts ? numparts++ : rnd(maxparts)); //next free slot, or kill a random kitten
		p->o = o;
		p->d = d;
		p->fade = fade;
		p->millis = lastmillis;
		p->color = bvec(color>>16, (color>>8)&0xFF, color&0xFF);
		p->size = size;
		p->grav = grav;
		p->collide = collide;
		p->owner = pl;
		p->flags = 0x80 | (rndmask ? rnd(0x80) & rndmask : 0);
		lastupdate = -1;
		return p;
	}

	void genverts(particle *p, partvert *vs, bool regen)
	{
		int blend, ts;
		calc(p, blend, ts);
		if(blend <= 1 || p->fade <= 5) p->fade = -1; //mark to remove on next pass (i.e. after render)

		modifyblend<T>(p->o, blend);

		if(regen)
		{
			p->flags &= ~0x80;

			#define SETTEXCOORDS(u1c, u2c, v1c, v2c) \
			{ \
				float u1 = u1c, u2 = u2c, v1 = v1c, v2 = v2c; \
				if(p->flags&0x01) swap(u1, u2); \
				if(p->flags&0x02) swap(v1, v2); \
				vs[0].u = u1; vs[0].v = v1; \
				vs[1].u = u2; vs[1].v = v1; \
				vs[2].u = u2; vs[2].v = v2; \
				vs[3].u = u1; vs[3].v = v2; \
			}
			if(type&PT_RND4)
			{
				float tx = 0.5f*((p->flags>>5)&1);
				float ty = 0.5f*((p->flags>>6)&1);
				SETTEXCOORDS(tx, tx+0.5f, ty, ty+0.5f);
			}
			else SETTEXCOORDS(0.f, 1.f, 0, 1);

			#define SETCOLOR(r, g, b, a) \
			do { \
				uchar col[4] = { r, g, b, a }; \
				loopi(4) memcpy(vs[i].color.v, col, sizeof(col)); \
			} while(0)
			#define SETMODCOLOR SETCOLOR((p->color[0]*blend)>>8, (p->color[1]*blend)>>8, (p->color[2]*blend)>>8, 255)
			if(type&PT_MOD) SETMODCOLOR;
			else SETCOLOR(p->color[0], p->color[1], p->color[2], blend);
		}
		else if(type&PT_MOD) SETMODCOLOR;
		else loopi(4) vs[i].alpha = blend;

		if(type&PT_ROT) genrotpos<T>(p->o, p->d, p->size, ts, p->grav, vs, (p->flags>>2)&0x1F);
		else genpos<T>(p->o, p->d, p->size, ts, p->grav, vs);
	}

	void update()
	{
		if(lastmillis == lastupdate) return;
		lastupdate = lastmillis;

		loopi(numparts)
		{
			particle *p = &parts[i];
			partvert *vs = &verts[i*4];
			if(p->fade < 0)
			{
				do
				{
					--numparts;
					if(numparts <= i) return;
				}
				while(parts[numparts].fade < 0);
				*p = parts[numparts];
				genverts(p, vs, true);
			}
			else genverts(p, vs, (p->flags&0x80)!=0);
		}
	}

	void render()
	{
		preload();
		if(tex) glBindTexture(GL_TEXTURE_2D, tex->id);
		glVertexPointer(3, GL_FLOAT, sizeof(partvert), &verts->pos);
		glTexCoordPointer(2, GL_FLOAT, sizeof(partvert), &verts->u);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(partvert), &verts->color);
		glDrawArrays(GL_QUADS, 0, numparts*4);
	}
};

typedef varenderer<PT_PART> quadrenderer;
typedef varenderer<PT_TAPE> taperenderer;
typedef varenderer<PT_TRAIL> trailrenderer;

struct softquadrenderer : quadrenderer
{
	softquadrenderer(const char *texname, int type)
		: quadrenderer(texname, type|PT_SOFT)
	{
	}

	int adddepthfx(vec &bbmin, vec &bbmax)
	{
		if(!numparts || (!depthfxtex.highprecision() && !depthfxtex.emulatehighprecision())) return 0;
		int numsoft = 0;
		loopi(numparts)
		{
			particle &p = parts[i];
			float radius = p.size*SQRT2;
			int blend, ts;
			calc(&p, blend, ts, false);
			if(depthfxscissor==2 ? depthfxtex.addscissorbox(p.o, radius) : isvisiblesphere(radius, p.o) < VFC_FOGGED)
			{
				numsoft++;
				loopk(3)
				{
					bbmin[k] = min(bbmin[k], p.o[k] - radius);
					bbmax[k] = max(bbmax[k], p.o[k] + radius);
				}
			}
		}
		return numsoft;
	}
};

struct lineprimitive : listparticle<lineprimitive>
{
	vec value;
};

struct lineprimitiverenderer : listrenderer<lineprimitive>
{
	lineprimitiverenderer(int type)
		: listrenderer<lineprimitive>(NULL, type)
	{}

	void startrender()
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		particlenotextureshader->set();
	}

	void endrender()
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
		particleshader->set();
	}

	void renderpart(lineprimitive *p, int blend, int ts, uchar *color)
	{
		glPushMatrix();
		glTranslatef(p->o.x, p->o.y, p->o.z);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - p->o.z, true);
		glScalef(p->size, p->size, p->size);
		glColor3ubv(color);

		glBegin(GL_LINES);
		glVertex3f(0, 0, 0);
		glVertex3fv(p->value.v);
		glEnd();

		glPopMatrix();
	}

	lineprimitive *addline(const vec &o, const vec &v, int fade, int color, float size)
	{
		lineprimitive *p = (lineprimitive *)listrenderer<lineprimitive>::addpart(o, vec(0, 0, 0), fade, color, size);
		p->value = vec(v).sub(o).div(size);
		return p;
	}

	// use addline() instead
	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, int grav = 0, int collide = 0, physent *pl = NULL) { return NULL; }
};
static lineprimitiverenderer lineprimitives(PT_LINE|PT_LERP);

struct trisprimitive : listparticle<trisprimitive>
{
	vec value[2];
	bool fill;
};

struct trisprimitiverenderer : listrenderer<trisprimitive>
{
	trisprimitiverenderer(int type)
		: listrenderer<trisprimitive>(NULL, type)
	{}

	void startrender()
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		particlenotextureshader->set();
	}

	void endrender()
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
		particleshader->set();
	}

	void renderpart(trisprimitive *p, int blend, int ts, uchar *color)
	{
		glPushMatrix();
		glTranslatef(p->o.x, p->o.y, p->o.z);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - p->o.z, true);
		glScalef(p->size, p->size, p->size);
		glColor3ubv(color);

		glBegin(GL_TRIANGLES);
		glPolygonMode(GL_FRONT_AND_BACK, p->fill ? GL_FILL : GL_LINE);
		glVertex3f(0, 0, 0);
		glVertex3fv(p->value[0].v);
		glVertex3fv(p->value[1].v);
		glEnd();

		glPopMatrix();
	}

	trisprimitive *addtriangle(const vec &o, float yaw, float pitch, int fade, int color, float size, bool fill)
	{
		vec dir[3];
		vecfromyawpitch(yaw, pitch, 1, 0, dir[0]);
		vecfromyawpitch(yaw, pitch, -1, 1, dir[1]);
		vecfromyawpitch(yaw, pitch, -1, -1, dir[2]);

		trisprimitive *p = (trisprimitive *)listrenderer<trisprimitive>::addpart(dir[0].mul(size*2.f).add(o), vec(0, 0, 0), fade, color, size);
		p->value[0] = dir[1];
		p->value[1] = dir[2];
		p->fill = fill;
		return p;
	}

	// use addtriangle() instead
	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, int grav = 0, int collide = 0, physent *pl = NULL) { return NULL; }
};
static trisprimitiverenderer trisprimitives(PT_TRIANGLE|PT_LERP);

struct loopprimitive : listparticle<loopprimitive>
{
	vec value;
	int axis;
	bool fill;
};

struct loopprimitiverenderer : listrenderer<loopprimitive>
{
	loopprimitiverenderer(int type)
		: listrenderer<loopprimitive>(NULL, type)
	{}

	void startrender()
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		particlenotextureshader->set();
	}

	void endrender()
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
		particleshader->set();
	}

	void renderpart(loopprimitive *p, int blend, int ts, uchar *color)
	{
		glPushMatrix();
		glTranslatef(p->o.x, p->o.y, p->o.z);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - p->o.z, true);
		glScalef(p->size, p->size, p->size);
		glColor3ubv(color);

		glBegin(GL_LINE_LOOP);
		glPolygonMode(GL_FRONT_AND_BACK, p->fill ? GL_FILL : GL_LINE);
		loopi(16)
		{
			vec v;
			switch(p->axis)
			{
				case 0:
					v = vec(p->value.x*cosf(2*M_PI*i/16.0f), p->value.z*sinf(2*M_PI*i/16.0f), 0).rotate_around_x(90*RAD);
					break;
				case 1:
					v = vec(p->value.z*cosf(2*M_PI*i/16.0f), p->value.y*sinf(2*M_PI*i/16.0f), 0).rotate_around_y(90*RAD);
					break;
				case 2: default:
					v = vec(p->value.x*cosf(2*M_PI*i/16.0f), p->value.y*sinf(2*M_PI*i/16.0f), 0).rotate_around_z(90*RAD);
					break;
			}
			glVertex3fv(v.v);
		}
		glEnd();

		glPopMatrix();
	}

	loopprimitive *addellipse(const vec &o, const vec &v, int fade, int color, float size, int axis, bool fill)
	{
		loopprimitive *p = (loopprimitive *)listrenderer<loopprimitive>::addpart(o, vec(0, 0, 0), fade, color, size);
		p->value = vec(v).div(size);
		p->axis = axis;
		p->fill = fill;
		return p;
	}

	// use addellipse() instead
	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, int grav = 0, int collide = 0, physent *pl = NULL) { return NULL; }
};
static loopprimitiverenderer loopprimitives(PT_ELLIPSE|PT_LERP);

struct coneprimitive : listparticle<coneprimitive>
{
	vec dir, spot, spoke;
	float radius, angle;
	bool fill;
};

struct coneprimitiverenderer : listrenderer<coneprimitive>
{
	coneprimitiverenderer(int type)
		: listrenderer<coneprimitive>(NULL, type)
	{}

	void startrender()
	{
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		particlenotextureshader->set();
	}

	void endrender()
	{
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
		particleshader->set();
	}

	void renderpart(coneprimitive *p, int blend, int ts, uchar *color)
	{
		glPushMatrix();
		glTranslatef(p->o.x, p->o.y, p->o.z);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - p->o.z, true);
		glScalef(p->size, p->size, p->size);
		glColor3ubv(color);

		glBegin(GL_LINES);
		//glPolygonMode(GL_FRONT_AND_BACK, p->fill ? GL_FILL : GL_LINE);
		loopi(16)
		{
			vec v = vec(p->spoke).rotate(2*M_PI*i/16.f, p->dir).add(p->spot);
			glVertex3f(0, 0, 0);
			glVertex3fv(v.v);
		}
		glEnd();

		glBegin(GL_LINE_LOOP);
		//glPolygonMode(GL_FRONT_AND_BACK, p->fill ? GL_FILL : GL_LINE);
		loopi(16)
		{
			vec v = vec(p->spoke).rotate(2*M_PI*i/16.f, p->dir).add(p->spot);
			glVertex3fv(v.v);
		}
		glEnd();

		glPopMatrix();
	}

	coneprimitive *addcone(const vec &o, const vec &dir, float radius, float angle, int fade, int color, float size, bool fill)
	{
		coneprimitive *p = (coneprimitive *)listrenderer<coneprimitive>::addpart(o, vec(0, 0, 0), fade, color, size);
		p->dir = dir;
		p->radius = radius;
		p->angle = angle;
		p->fill = fill;
		p->spot = vec(p->dir).mul(p->radius*cosf(p->angle*RAD));
		p->spoke.orthogonal(p->dir);
		p->spoke.normalize().mul(p->radius*sinf(p->angle*RAD));
		return p;
	}

	// use addcone() instead
	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, int grav = 0, int collide = 0, physent *pl = NULL) { return NULL; }
};
static coneprimitiverenderer coneprimitives(PT_CONE|PT_LERP);

static partrenderer *parts[] =
{
	new portalrenderer("textures/teleport"), &icons,
	&lineprimitives, &trisprimitives, &loopprimitives, &coneprimitives,
	new softquadrenderer("particles/fire", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_ROT|PT_LERP),
	new softquadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_FLIP|PT_ROT|PT_LERP),
	new taperenderer("particles/sflare", PT_TAPE|PT_GLARE|PT_LERP),
	new taperenderer("particles/mflare", PT_TAPE|PT_GLARE|PT_RND4|PT_VFLIP|PT_LERP),
	new softquadrenderer("particles/smoke", PT_PART|PT_LERP|PT_FLIP|PT_ROT),
	new quadrenderer("particles/smoke", PT_PART|PT_LERP|PT_FLIP|PT_ROT),
	new softquadrenderer("particles/hint", PT_PART|PT_GLARE|PT_LERP),
	new quadrenderer("particles/hint", PT_PART|PT_GLARE|PT_LERP),
	new softquadrenderer("particles/smoke", PT_PART|PT_FLIP|PT_ROT),
	new quadrenderer("particles/smoke", PT_PART|PT_FLIP|PT_ROT),
	new softquadrenderer("particles/hint", PT_PART|PT_GLARE),
	new quadrenderer("particles/hint", PT_PART|PT_GLARE),
	new quadrenderer("particles/blood", PT_PART|PT_MOD|PT_RND4|PT_FLIP|PT_ROT),
	new quadrenderer("particles/entity", PT_PART|PT_GLARE),
	new quadrenderer("particles/entity", PT_PART|PT_GLARE|PT_ONTOP),
	new quadrenderer("particles/spark", PT_PART|PT_GLARE|PT_FLIP|PT_ROT),
	new softquadrenderer("particles/fire", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_ROT),
	new quadrenderer("particles/fire", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_ROT),
	new softquadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_FLIP|PT_ROT),
	new quadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_FLIP|PT_ROT),
	new softquadrenderer("particles/electric", PT_PART|PT_GLARE|PT_FLIP|PT_ROT),
	new quadrenderer("particles/electric", PT_PART|PT_GLARE|PT_FLIP|PT_ROT),
	new quadrenderer("particles/fire", PT_PART|PT_GLARE|PT_FLIP|PT_RND4|PT_GLARE|PT_ROT),
	new taperenderer("particles/sflare", PT_TAPE|PT_GLARE),
	new taperenderer("particles/mflare", PT_TAPE|PT_GLARE|PT_RND4|PT_VFLIP|PT_GLARE),
	new quadrenderer("particles/muzzle", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_ROT),
	new quadrenderer("particles/snow", PT_PART|PT_GLARE|PT_FLIP|PT_ROT),
	&texts, &textontop,
	&fireballs, &noglarefireballs, &lightnings,
	&flares // must be done last!
};

void finddepthfxranges()
{
	depthfxmin = vec(1e16f, 1e16f, 1e16f);
	depthfxmax = vec(0, 0, 0);
	numdepthfxranges = fireballs.finddepthfxranges(depthfxowners, depthfxranges, MAXDFXRANGES, depthfxmin, depthfxmax);
	loopk(3)
	{
		depthfxmin[k] -= depthfxmargin;
		depthfxmax[k] += depthfxmargin;
	}
	if(depthfxparts)
	{
		loopi(sizeof(parts)/sizeof(parts[0]))
		{
			partrenderer *p = parts[i];
			if(p->type&PT_SOFT && p->adddepthfx(depthfxmin, depthfxmax))
			{
				if(!numdepthfxranges)
				{
					numdepthfxranges = 1;
					depthfxowners[0] = NULL;
					depthfxranges[0] = 0;
				}
			}
		}
	}
	if(depthfxscissor<2 && numdepthfxranges>0) depthfxtex.addscissorbox(depthfxmin, depthfxmax);
}

void particleinit()
{
	if(!particleshader) particleshader = lookupshaderbyname("particle");
	if(!particlenotextureshader) particlenotextureshader = lookupshaderbyname("particlenotexture");
	loopi(sizeof(parts)/sizeof(parts[0]))
	{
		parts[i]->init(maxparticles);
		parts[i]->preload();
	}
}

void clearparticles()
{
	loopi(sizeof(parts)/sizeof(parts[0])) parts[i]->reset();
}

void cleanupparticles()
{
	loopi(sizeof(parts)/sizeof(parts[0])) parts[i]->cleanup();
}

void removetrackedparticles(physent *pl)
{
	loopi(sizeof(parts)/sizeof(parts[0])) parts[i]->resettracked(pl);
}

void renderparticles(bool mainpass)
{
	//want to debug BEFORE the lastpass render (that would delete particles)
	if(debugparticles && !glaring && !reflecting && !refracting)
	{
		int n = sizeof(parts)/sizeof(parts[0]);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, FONTH*n*2, FONTH*n*2, 0, -1, 1); //squeeze into top-left corner
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		defaultshader->set();
		loopi(n)
		{
			int type = parts[i]->type;
			const char *title = parts[i]->texname ? strrchr(parts[i]->texname, '/')+1 : NULL;
			mkstring(info);
			if(type&PT_GLARE) concatstring(info, "g,");
			if(type&PT_SOFT) concatstring(info, "s,");
			if(type&PT_LERP) concatstring(info, "l,");
			if(type&PT_MOD) concatstring(info, "m,");
			if(type&PT_RND4) concatstring(info, "r,");
			if(type&PT_FLIP) concatstring(info, "f,");
			if(type&PT_ONTOP) concatstring(info, "o,");
			defformatstring(ds)("%d\t%s: %s %s", parts[i]->count(), partnames[type&0xFF], info, (title?title:""));
			draw_text(ds, FONTH, (i+n/2)*FONTH);
		}
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	if(glaring && !particleglare) return;

	loopi(sizeof(parts)/sizeof(parts[0]))
	{
		if(glaring && !(parts[i]->type&PT_GLARE)) continue;
		parts[i]->update();
	}

	static float zerofog[4] = { 0, 0, 0, 1 };
	float oldfogc[4];
	bool rendered = false;
	uint lastflags = PT_LERP, flagmask = PT_LERP|PT_MOD|PT_ONTOP;

	if(binddepthfxtex()) flagmask |= PT_SOFT;

	loopi(sizeof(parts)/sizeof(parts[0]))
	{
		partrenderer *p = parts[i];
		if(glaring && !(p->type&PT_GLARE)) continue;
		if(!p->haswork()) continue;

		if(!rendered)
		{
			rendered = true;
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if(glaring) setenvparamf("colorscale", SHPARAM_VERTEX, 4, particleglare, particleglare, particleglare, 1);
			else setenvparamf("colorscale", SHPARAM_VERTEX, 4, 1, 1, 1, 1);

			particleshader->set();
			glGetFloatv(GL_FOG_COLOR, oldfogc);
		}

		uint flags = p->type & flagmask;
		if(p->usesvertexarray()) flags |= 0x01; //0x01 = VA marker
		uint changedbits = (flags ^ lastflags);
		if(changedbits != 0x0000)
		{
			if(changedbits&0x01)
			{
				if(flags&0x01)
				{
					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glEnableClientState(GL_COLOR_ARRAY);
				}
				else
				{
					glDisableClientState(GL_VERTEX_ARRAY);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glDisableClientState(GL_COLOR_ARRAY);
				}
			}
			if(changedbits&PT_LERP) glFogfv(GL_FOG_COLOR, (flags&PT_LERP) ? oldfogc : zerofog);
			if(changedbits&(PT_LERP|PT_MOD))
			{
				if(flags&PT_LERP) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				else if(flags&PT_MOD) glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
				else glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			}
			if(changedbits&PT_SOFT)
			{
				if(flags&PT_SOFT)
				{
					if(depthfxtex.target==GL_TEXTURE_RECTANGLE_ARB)
					{
						if(!depthfxtex.highprecision()) SETSHADER(particlesoft8rect);
						else SETSHADER(particlesoftrect);
					}
					else
					{
						if(!depthfxtex.highprecision()) SETSHADER(particlesoft8);
						else SETSHADER(particlesoft);
					}

					binddepthfxparams(depthfxpartblend);
				}
				else particleshader->set();
			}
            if(changedbits&PT_ONTOP)
            {
                if(flags&PT_ONTOP) glDisable(GL_DEPTH_TEST);
                else glEnable(GL_DEPTH_TEST);
            }
			lastflags = flags;
		}
		p->render();
	}

	if(rendered)
	{
		if(lastflags&(PT_LERP|PT_MOD)) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		if(!(lastflags&PT_LERP)) glFogfv(GL_FOG_COLOR, oldfogc);
		if(lastflags&0x01)
		{
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
		}
        if(lastflags&PT_ONTOP) glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}

particle *newparticle(const vec &o, const vec &d, int fade, int type, int color, float size, int grav, int collide, physent *pl)
{
	return parts[type]->addpart(o, d, fade, color, size, grav, collide, pl);
}

void create(int type, int color, int fade, const vec &p, float size, int grav, int collide, physent *pl)
{
	if(camera1->o.dist(p) > maxparticledistance) return;
	float collidez = collide ? p.z - raycube(p, vec(0, 0, -1), COLLIDERADIUS, RAY_CLIPMAT) + COLLIDEERROR : -1;
	int fmin = 1;
	int fmax = fade*3;
	int f = fmin + rnd(fmax); //help deallocater by using fade distribution rather than random
	newparticle(p, vec(0, 0, 0), f, type, color, size, grav, collide, pl)->val = collidez;
}

void regularcreate(int type, int color, int fade, const vec &p, float size, int grav, int collide, physent *pl, int delay)
{
	if(!emit_particles() || (delay > 0 && rnd(delay) != 0)) return;
	create(type, color, fade, p, size, grav, collide, pl);
}

void splash(int type, int color, int radius, int num, int fade, const vec &p, float size, int grav, int collide)
{
	if(camera1->o.dist(p) > maxparticledistance) return;
	float collidez = collide ? p.z - raycube(p, vec(0, 0, -1), COLLIDERADIUS, RAY_CLIPMAT) + COLLIDEERROR : -1;
	int fmin = 1;
	int fmax = fade*3;
	loopi(num)
	{
		vec tmp(rnd(max(radius*2,1))-radius, rnd(max(radius*2,1))-radius, rnd(max(radius*2,1))-radius);
		int f = (num < 10) ? (fmin + rnd(fmax)) : (fmax - (i*(fmax-fmin))/(num-1)); //help deallocater by using fade distribution rather than random
		newparticle(p, tmp, f, type, color, size, grav, collide)->val = collidez;
	}
}

void regularsplash(int type, int color, int radius, int num, int fade, const vec &p, float size, int grav, int collide, int delay)
{
	if(!emit_particles() || (delay > 0 && rnd(delay) != 0)) return;
	splash(type, color, radius, num, fade, p, size, grav, collide);
}

bool canaddparticles()
{
    return !renderedgame && !shadowmapping;
}

void regular_part_create(int type, int fade, const vec &p, int color, float size, int grav, int collide, physent *pl, int delay)
{
	if(!canaddparticles()) return;
	regularcreate(type, color, fade, p, size, grav, collide, pl, delay);
}

void part_create(int type, int fade, const vec &p, int color, float size, int grav, int collide, physent *pl)
{
	if(!canaddparticles()) return;
	create(type, color, fade, p, size, grav, collide, pl);
}

void regular_part_splash(int type, int num, int fade, const vec &p, int color, float size, int grav, int collide, int radius, int delay)
{
	if(!canaddparticles()) return;
	regularsplash(type, color, radius, num, fade, p, size, grav, collide, delay);
}

void part_splash(int type, int num, int fade, const vec &p, int color, float size, int grav, int collide, int radius)
{
	if(!canaddparticles()) return;
	splash(type, color, radius, num, fade, p, size, grav, collide);
}

void part_trail(int ptype, int fade, const vec &s, const vec &e, int color, float size, int grav, int collide)
{
	if(!canaddparticles()) return;
	vec v;
	float d = e.dist(s, v);
	int steps = clamp(int(d*2), 1, maxparticletrail);
	v.div(steps);
	vec p = s;
	loopi(steps)
	{
		p.add(v);
		vec tmp = vec(float(rnd(11)-5), float(rnd(11)-5), float(rnd(11)-5));
		newparticle(p, tmp, rnd(fade)+fade, ptype, color, size, grav, collide);
	}
}

void part_text(const vec &s, const char *t, int type, int fade, int color, float size, int grav, int collide, physent *pl)
{
	if(!canaddparticles() || !t[0] || (t[0] == '@' && !t[1])) return;
	if(!particletext || camera1->o.dist(s) > maxparticledistance) return;
	newparticle(s, vec(0, 0, 1), fade, type, color, size, grav, collide, pl)->text = t[0]=='@' ? newstring(t) : t;
}

void part_flare(const vec &p, const vec &dest, int fade, int type, int color, float size, int grav, int collide, physent *pl)
{
	if(!canaddparticles()) return;
	newparticle(p, dest, fade, type, color, size, grav, collide, pl);
}

void part_fireball(const vec &dest, float maxsize, int type, int fade, int color, float size, int grav, int collide)
{
	if(!canaddparticles()) return;
	float growth = maxsize - size;
	if(fade < 0) fade = int(growth*25);
	newparticle(dest, vec(0, 0, 1), fade, type, color, size, grav, collide)->val = growth;
}

void regular_part_fireball(const vec &dest, float maxsize, int type, int fade, int color, float size, int grav, int collide)
{
	if(!canaddparticles() || !emit_particles()) return;
	part_fireball(dest, maxsize, type, fade, color, size, grav, collide);
}

void part_spawn(const vec &o, const vec &v, float z, uchar type, int amt, int fade, int color, float size, int grav, int collide)
{
	if(!canaddparticles()) return;
	loopi(amt)
	{
		vec w(rnd(int(v.x*2))-int(v.x), rnd(int(v.y*2))-int(v.y), rnd(int(v.z*2))-int(v.z)+z);
		w.add(o);
		part_splash(type, 1, fade, w, color, size, grav, collide, 1);
	}
}

void part_flares(const vec &o, const vec &v, float z1, const vec &d, const vec &w, float z2, uchar type, int amt, int fade, int color, float size, int grav, int collide, physent *pl)
{
	if(!canaddparticles()) return;
	loopi(amt)
	{
		vec from(rnd(int(v.x*2))-int(v.x), rnd(int(v.y*2))-int(v.y), rnd(int(v.z*2))-int(v.z)+z1);
		from.add(o);

		vec to(rnd(int(w.x*2))-int(w.x), rnd(int(w.y*2))-int(w.y), rnd(int(w.z*2))-int(w.z)+z1);
		to.add(d);

		newparticle(from, to, fade, type, color, size, grav, collide, pl);
	}
}

void part_portal(const vec &o, float size, float yaw, float pitch, int type, int fade, int color)
{
	if(!canaddparticles()) return;
	portalrenderer *p = dynamic_cast<portalrenderer *>(parts[type]);
	if(p) p->addportal(o, fade, color, size, yaw, pitch);
}

void part_icon(const vec &o, Texture *tex, float blend, float size, int grav, int collide, int fade, int color, float start, float length, physent *pl)
{
	if(!canaddparticles()) return;
	iconrenderer *p = dynamic_cast<iconrenderer *>(parts[PART_ICON]);
	if(p) p->addicon(o, tex, blend, fade, color, size, grav, collide, start, length, pl);
}

void part_line(const vec &o, const vec &v, float size, int fade, int color, int type)
{
	if(!canaddparticles()) return;
	lineprimitiverenderer *p = dynamic_cast<lineprimitiverenderer *>(parts[type]);
	if(p) p->addline(o, v, fade, color, size);
}

void part_triangle(const vec &o, float yaw, float pitch, float size, int fade, int color, bool fill, int type)
{
	if(!canaddparticles()) return;
	trisprimitiverenderer *p = dynamic_cast<trisprimitiverenderer *>(parts[type]);
	if(p) p->addtriangle(o, yaw, pitch, fade, color, size, fill);
}

void part_dir(const vec &o, float yaw, float pitch, float size, int fade, int color, bool fill)
{
	if(!canaddparticles()) return;

	vec v; vecfromyawpitch(yaw, pitch, 1, 0, v); v.normalize();
	part_line(o, vec(v).mul(size+1.f).add(o), 1.f, fade, color);
	part_triangle(vec(v).mul(size).add(o), yaw, pitch, 1.f, fade, color, fill);
}

void part_trace(const vec &o, const vec &v, float size, int fade, int color, bool fill)
{
	part_line(o, v, 1.f, fade, color);
	float yaw, pitch;
	vectoyawpitch(vec(v).sub(o).normalize(), yaw, pitch);
	part_triangle(o, yaw, pitch, size, fade, color, fill);
}

void part_ellipse(const vec &o, const vec &v, float size, int fade, int color, int axis, bool fill, int type)
{
	if(!canaddparticles()) return;
	loopprimitiverenderer *p = dynamic_cast<loopprimitiverenderer *>(parts[type]);
	if(p) p->addellipse(o, v, fade, color, size, axis, fill);
}

void part_radius(const vec &o, const vec &v, float size, int fade, int color, bool fill)
{
	if(!canaddparticles()) return;
	loopprimitiverenderer *p = dynamic_cast<loopprimitiverenderer *>(parts[PART_ELLIPSE]);
	if(p)
	{
		p->addellipse(o, v, fade, color, size, 0, fill);
		p->addellipse(o, v, fade, color, size, 1, fill);
		p->addellipse(o, v, fade, color, size, 2, fill);
	}
}

void part_cone(const vec &o, const vec &dir, float radius, float angle, float size, int fade, int color, bool fill, int type)
{
	if(!canaddparticles()) return;
	coneprimitiverenderer *p = dynamic_cast<coneprimitiverenderer *>(parts[type]);
	if(p) p->addcone(o, dir, radius, angle, fade, color, size, fill);
}

//dir = 0..6 where 0=up
static inline vec offsetvec(vec o, int dir, int dist)
{
	vec v = vec(o);
	v[(2+dir)%3] += (dir>2)?(-dist):dist;
	return v;
}

/* Experiments in shapes...
 * dir: (where dir%3 is similar to offsetvec with 0=up)
 * 0..2 circle
 * 3.. 5 cylinder shell
 * 6..11 cone shell
 * 12..14 plane volume
 * 15..20 line volume, i.e. wall
 * 21 sphere
 * +32 to inverse direction
 */
void regularshape(int type, int radius, int color, int dir, int num, int fade, const vec &p, float size, int grav, int collide, float vel)
{
	if(!emit_particles()) return;

	int basetype = parts[type]->type&0xFF;
	bool flare = (basetype == PT_TAPE) || (basetype == PT_LIGHTNING),
		 inv = (dir&0x20)!=0, taper = (dir&0x40)!=0;
	dir &= 0x1F;
	loopi(num)
	{
		vec to, from;
		if(dir < 12)
		{
			float a = PI2*float(rnd(1000))/1000.0;
			to[dir%3] = sinf(a)*radius;
			to[(dir+1)%3] = cosf(a)*radius;
			to[(dir+2)%3] = 0.0;
			to.add(p);
			if(dir < 3) //circle
				from = p;
			else if(dir < 6) //cylinder
			{
				from = to;
				to[(dir+2)%3] += radius;
				from[(dir+2)%3] -= radius;
			}
			else //cone
			{
				from = p;
				to[(dir+2)%3] += (dir < 9)?radius:(-radius);
			}
		}
		else if(dir < 15) //plane
		{
			to[dir%3] = float(rnd(radius<<4)-(radius<<3))/8.0;
			to[(dir+1)%3] = float(rnd(radius<<4)-(radius<<3))/8.0;
			to[(dir+2)%3] = radius;
			to.add(p);
			from = to;
			from[(dir+2)%3] -= 2*radius;
		}
		else if(dir < 21) //line
		{
			if(dir < 18)
			{
				to[dir%3] = float(rnd(radius<<4)-(radius<<3))/8.0;
				to[(dir+1)%3] = 0.0;
			}
			else
			{
				to[dir%3] = 0.0;
				to[(dir+1)%3] = float(rnd(radius<<4)-(radius<<3))/8.0;
			}
			to[(dir+2)%3] = 0.0;
			to.add(p);
			from = to;
			to[(dir+2)%3] += radius;
		}
		else //sphere
		{
			to = vec(PI2*float(rnd(1000))/1000.0, PI*float(rnd(1000)-500)/1000.0).mul(radius);
			to.add(p);
			from = p;
		}

		if(taper)
		{
			vec o = inv ? to : from;
			o.sub(camera1->o);
			float dist = clamp(sqrtf(o.x*o.x + o.y*o.y)/maxparticledistance, 0.0f, 1.0f);
			if(dist > 0.2f)
			{
				dist = 1 - (dist - 0.2f)/0.8f;
				if(rnd(0x10000) > dist*dist*0xFFFF) continue;
			}
		}

		if(flare)
			newparticle(inv?to:from, inv?from:to, rnd(fade*3)+1, type, color, size, grav, collide);
		else
		{
			vec d = vec(to).sub(from).normalize().mul(inv ? -vel : vel);
			particle *np = newparticle(inv?to:from, d, rnd(fade*3)+1, type, color, size, grav, collide);
			if(np->collide)
				np->val = (inv ? to.z : from.z) - raycube(inv ? to : from, vec(0, 0, -1), COLLIDERADIUS, RAY_CLIPMAT) + COLLIDEERROR;
		}
	}
}

void regularflame(int type, const vec &p, float radius, float height, int color, int density, int fade, float size, int grav, int collide, float vel)
{
	if(!emit_particles()) return;

	float s = size*min(radius, height);
	vec v(0, 0, min(1.0f, height)*vel);
	loopi(density)
	{
		vec q = vec(p).add(vec(rndscale(radius*2.f)-radius, rndscale(radius*2.f)-radius, 0));
		newparticle(q, v, rnd(max(int(fade*height), 1))+1, type, color, s, grav, collide);
	}
}

#if 0
vector<mapparticle> mapparts;

int addmapparticle(const char *name, int part, int type, int colour, int grav, int fade, float radius, float height, int attr, float size, float vel)
{
	if(!name || !*name || part < 0 || part >= PART_MAX) return -1;
	int idx = -1;
	loopv(mapparts) if(!strcasecmp(name, mapparts[i].name)) return i;
	idx = mapparts.length();
	mapparticle &p = mapparts.add();
	p.name = newstring(name);
	p.part = part;
	p.type = type;
	p.colour = colour;
	p.grav = grav;
	p.fade = fade;
	p.radius = radius;
	p.height = height;
	p.attr = attr;
	p.size = size;
	p.vel = vel;
}
ICOMMAND(mapparticle, "siiiiiffiff", (const char *name, int *part, int *type, int *colour, int *grav, int *fade, float *radius, float *height, int *attr, float *size, float *vel),
	intret(addmapparticle(name, *part, *type, *colour, *grav, *fade, *radius, *height, *attr, *size, *vel)));

void defaultparticles()
{
	if(mapparts.length()) mapparts.setsizenodelete(0);
	addmapparticle("fire", PART_FLAME, PARTTYPE_FIRE, 0x903020, -10, 500, 1.5f, 0.5f, 0, 2.f, 200);
	addmapparticle("smoke", PART_SMOKE, PARTTYPE_SPLASH, 0x897661, -20, 200, 2.f, 0.f, 3, 2.4f, 0.f, 1);
	addmapparticle("water", PART_SPARK, PARTTYPE_SPLASH, (int(watercol[0])<<16) | (int(watercol[1])<<8) | int(watercol[2]), 10, 500, 10.f, 0.f, 10, 0.6f, 4);
}
#endif

void makeparticle(const vec &o, vector<int> &attr)
{
	switch(attr[0])
	{
		case 0: //fire
		{
			//regularsplash(PART_FIREBALL, 0xFFC8C8, 10, 1, 40, o, 4.8f, -10);
			//regularsplash(PART_SMOKE_LERP, 0x897661, 2, 1, 200,  vec(o.x, o.y, o.z+3.0), 2.4f, -20, 0, 3);
            float radius = attr[1] ? float(attr[1])/100.0f : 1.5f,
                  height = attr[2] ? float(attr[2])/100.0f : radius*3;
            regularflame(PART_FLAME, o, radius, height, attr[3] ? attr[3] : 0xF05010, 3, attr[4] > 0 ? attr[4]/2 : 500, 2.0f, -5, 0, 30);
            regularflame(PART_SMOKE, vec(o.x, o.y, o.z + 4.0f*min(radius, height)), radius, height, 0x101008, 1, attr[4] > 0 ? attr[4] : 1000, 2.0f, -10, 0, 30);
			break;
		}
		case 1: //smoke vent - <dir>
			regularsplash(PART_SMOKE, 0x897661, 2, 1, 200,  offsetvec(o, attr[1], rnd(10)), 2.4f, -20);
			break;
		case 2: //water fountain - <dir>
		{
			int color = (int(watercol[0])<<16) | (int(watercol[1])<<8) | int(watercol[2]);
			regularsplash(PART_SPARK, color, 10, 4, 200, offsetvec(o, attr[1], rnd(10)), 0.6f, 10);
			break;
		}
		case 3: //fire ball - <size> <rgb>
			newparticle(o, vec(0, 0, 1), 1, PART_EXPLOSION, attr[2], 4.0f)->val = 1+attr[1];
			break;
		case 4:  //tape - <dir> <length> <rgb>
		case 7:  //lightning
		case 8:  //fire
		case 9:  //smoke
		case 10: //water
		case 11: //plasma
		case 12: //snow
		case 13: //sparks
		{
			const int typemap[] = { PART_FLARE, -1, -1, PART_LIGHTNING, PART_FIREBALL, PART_SMOKE, PART_ELECTRIC, PART_PLASMA, PART_SNOW, PART_SPARK };
			const float sizemap[] = { 0.28f, 0.0f, 0.0f, 0.25f, 4.f, 2.f, 0.6f, 4.f, 0.5f, 0.2f }, velmap[] = { 50, 0, 0, 20, 30, 30, 50, 20, 10, 20 },
				gravmap[] = { 0, 0, 0, 0, -5, -10, -10, 0, 10, 20 }, colmap[] = { 0, 0, 0, 0, 0, 0, 0, 0, DECAL_STAIN, 0 };
			int type = typemap[attr[0]-4];
			float size = sizemap[attr[0]-4], vel = velmap[attr[0]-4], grav = gravmap[attr[0]-4], col = colmap[attr[0]-4];
			if(attr[1] >= 256) regularshape(type, max(1+attr[2], 1), attr[3], attr[1]-256, 5, attr[4] > 0 ? attr[4] : 250, o, size, grav, col, vel);
			else newparticle(o, offsetvec(o, attr[1], max(1+attr[2], 0)), attr[4] > 0 ? attr[4] : 1, type, attr[3], size, grav, col);
			break;
		}
		case 14: // flames <radius> <height> <rgb>
		case 15: // smoke plume
		{
			const int typemap[] = { PART_FLAME, PART_SMOKE }, fademap[] = { 500, 1000 }, densitymap[] = { 3, 1 };
			const float sizemap[] = { 2, 2 }, velmap[] = { 25, 50 }, gravmap[] = { -5, -10 };
			int type = typemap[attr[0]-14], density = densitymap[attr[0]-14], fade = attr[4] > 0 ? attr[4] : fademap[attr[0]-14];
			float size = sizemap[attr[0]-14], vel = velmap[attr[0]-14], grav = gravmap[attr[0]-14];
			regularflame(type, o, float(attr[1])/100.0f, float(attr[2])/100.0f, attr[3], density, fade, size, grav, 0, vel);
			break;
		}
		case 6: //meter, metervs - <percent> <rgb> <rgb2>
		{
			float length = clamp(attr[1], 0, 100)/100.f;
			part_icon(o, textureload("textures/progress", 3), 1.f, 2, 0, 0, 1, attr[3], length, 1-length); // fall through
		}
		case 5:
		{
			float length = clamp(attr[1], 0, 100)/100.f;
			part_icon(o, textureload("textures/progress", 3), 1.f, 2, 0, 0, 1, attr[2], 0, length);
			break;
		}
		case 32: //lens flares - plain/sparkle/sun/sparklesun <red> <green> <blue>
		case 33:
		case 34:
		case 35:
			flares.addflare(o, attr[1], attr[2], attr[3], (attr[0]&0x02)!=0, (attr[0]&0x01)!=0);
			break;
		default:
			defformatstring(ds)("@%d?", attr[0]);
			part_text(o, ds);
			break;
	}
}
void makeparticles(extentity &e) { makeparticle(e.o, e.attrs); }

void updateparticles()
{
	if(lastmillis-lastemitframe >= emitmillis)
	{
		emit = true;
		lastemitframe = lastmillis-(lastmillis%emitmillis);
	}
	else emit = false;

	flares.setupflares();
	entities::drawparticles();
	flares.drawflares(); // do after drawparticles so that we can make flares for them too
}

