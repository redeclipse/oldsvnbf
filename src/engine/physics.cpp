// physics.cpp: no physics books were hurt nor consulted in the construction of this code.
// All physics computations and constants were invented on the fly and simply tweaked until
// they "felt right", and have no basis in reality. Collision detection is simplistic but
// very robust (uses discrete steps at fixed fps).

#include "engine.h"
#include "mpr.h"

const int MAXCLIPPLANES = 1024;
clipplanes clipcache[MAXCLIPPLANES], *nextclip = clipcache;

static inline void setcubeclip(cube &c, const ivec &o, int size)
{
    if(!c.ext || !c.ext->clip || c.ext->clip->owner!=&c)
	{
        if(nextclip >= &clipcache[MAXCLIPPLANES]) nextclip = clipcache;
        ext(c).clip = nextclip;
        nextclip->owner = &c;
        genclipplanes(c, o.x, o.y, o.z, size, *nextclip);
        nextclip++;
	}
}

void freeclipplanes(cube &c)
{
	if(!c.ext || !c.ext->clip) return;
    if(c.ext->clip->owner==&c) c.ext->clip->owner = NULL;
	c.ext->clip = NULL;
}

/////////////////////////  ray - cube collision ///////////////////////////////////////////////

static inline void pushvec(vec &o, const vec &ray, float dist)
{
	vec d(ray);
	d.mul(dist);
	o.add(d);
}

static inline bool pointinbox(const vec &v, const vec &bo, const vec &br)
{
	return v.x <= bo.x+br.x &&
			v.x >= bo.x-br.x &&
			v.y <= bo.y+br.y &&
			v.y >= bo.y-br.y &&
			v.z <= bo.z+br.z &&
			v.z >= bo.z-br.z;
}

bool pointincube(const clipplanes &p, const vec &v)
{
	if(!pointinbox(v, p.o, p.r)) return false;
	loopi(p.size) if(p.p[i].dist(v)>1e-3f) return false;
	return true;
}

#define INTERSECTPLANES(setentry) \
    clipplanes &p = *c.ext->clip; \
    float enterdist = -1e16f, exitdist = 1e16f; \
    loopi(p.size) \
    { \
        float pdist = p.p[i].dist(o), facing = ray.dot(p.p[i]); \
        if(facing < 0) \
        { \
            pdist /= -facing; \
            if(pdist > enterdist) \
            { \
                if(pdist > exitdist) return false; \
                enterdist = pdist; \
                setentry; \
            } \
        } \
        else if(facing > 0) \
        { \
            pdist /= -facing; \
            if(pdist < exitdist) \
            { \
                if(pdist < enterdist) return false; \
                exitdist = pdist; \
            } \
        } \
        else if(pdist > 0) return false; \
    }

#define INTERSECTBOX(setentry) \
    loop(i, 3) \
    { \
        if(ray[i]) \
        { \
            float prad = fabs(p.r[i] / ray[i]), pdist = (p.o[i] - o[i]) / ray[i], pmin = pdist - prad, pmax = pdist + prad; \
            if(pmin > enterdist) \
            { \
                if(pmin > exitdist) return false; \
                enterdist = pmin; \
            } \
            if(pmax < exitdist) \
            { \
                if(pmax < enterdist) return false; \
                exitdist = pmax; \
            } \
         } \
         else if(o[i] < p.o[i]-p.r[i] || o[i] > p.o[i]+p.r[i]) return false; \
    }

// optimized shadow version
bool shadowcubeintersect(const cube &c, const vec &o, const vec &ray, float &dist)
{
    INTERSECTPLANES({});
    INTERSECTBOX({});
    if(exitdist < 0) return false;
    dist = max(enterdist+0.1f, 0.0f);
    return true;
}

vec hitsurface;

bool raycubeintersect(const cube &c, const vec &o, const vec &ray, float maxdist, float &dist)
{
    int entry = -1, bbentry = -1;
    INTERSECTPLANES(entry = i);
    INTERSECTBOX(bbentry = i);
    if(exitdist < 0) return false;
    dist = max(enterdist+0.1f, 0.0f);
    if(dist < maxdist)
    {
        if(bbentry>=0) { hitsurface = vec(0, 0, 0); hitsurface[bbentry] = ray[bbentry]>0 ? -1 : 1; }
        else hitsurface = p.p[entry];
    }
    return true;
}

extern void entselectionbox(const extentity &e, vec &eo, vec &es);
extern int entselradius;
float hitentdist;
int hitent, hitorient;

#define mapmodelskip \
	{ \
			if(e.attrs[4]&MMT_NOCLIP) continue; \
			if(e.lastemit) \
			{ \
				if(e.attrs[4]&MMT_HIDE) \
				{ \
					if(e.spawned) continue; \
				} \
				else if(e.lastemit > 0) \
				{ \
					int millis = lastmillis-e.lastemit, delay = entities::triggertime(e); \
					if(!e.spawned && millis < delay/2) continue; \
					if(e.spawned && millis > delay/2) continue; \
				} \
				else if(e.spawned) continue; \
			} \
	}


static float disttoent(octaentities *oc, octaentities *last, const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
	vec eo, es;
	int orient;
	float dist = 1e16f, f = 0.0f;
	if(oc == last || oc == NULL) return dist;
	const vector<extentity *> &ents = entities::getents();

	#define entintersect(mask, type, func) {\
		if((mode&(mask))==(mask)) \
        { \
			loopv(oc->type) \
				if(!last || last->type.find(oc->type[i])<0) \
				{ \
					extentity &e = *ents[oc->type[i]]; \
					if(!e.inoctanode || &e==t) continue; \
					func; \
					if(f<dist && f>0) { \
						hitentdist = dist = f; \
						hitent = oc->type[i]; \
						hitorient = orient; \
					} \
				} \
        } \
	}

	entintersect(RAY_POLY, mapmodels, {
		if((mode&RAY_ENTS)!=RAY_ENTS) mapmodelskip;
		if((mode&RAY_ENTS)==RAY_ENTS && !entities::cansee(e)) continue;
		orient = 0; // FIXME, not set
		if(!mmintersect(e, o, ray, radius, mode, f)) continue;
	});

	entintersect(RAY_ENTS, other,
		if((mode&RAY_ENTS)==RAY_ENTS && !entities::cansee(e)) continue;
		entselectionbox(e, eo, es);
		if(!rayrectintersect(eo, es, o, ray, f, orient)) continue;
	);

	entintersect(RAY_ENTS, mapmodels,
		if((mode&RAY_ENTS)==RAY_ENTS && !entities::cansee(e)) continue;
		entselectionbox(e, eo, es);
		if(!rayrectintersect(eo, es, o, ray, f, orient)) continue;
	);

	return dist;
}

// optimized shadow version
static float shadowent(octaentities *oc, octaentities *last, const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
	float dist = 1e16f, f = 0.0f;
	if(oc == last || oc == NULL) return dist;
	const vector<extentity *> &ents = entities::getents();
	loopv(oc->mapmodels) if(!last || last->mapmodels.find(oc->mapmodels[i])<0)
	{
		extentity &e = *ents[oc->mapmodels[i]];
		if(!e.inoctanode || &e==t) continue;
		if(e.lastemit || e.attrs[4]&MMT_NOSHADOW) continue;
		if(!mmintersect(e, o, ray, radius, mode, f)) continue;
		if(f>0 && f<dist) dist = f;
	}
	return dist;
}

#define INITRAYCUBE \
	octaentities *oclast = NULL; \
	float dist = 0, dent = mode&RAY_BB ? 1e16f : 1e14f; \
    vec v(o), invray(ray.x ? 1/ray.x : 1e16f, ray.y ? 1/ray.y : 1e16f, ray.z ? 1/ray.z : 1e16f); \
	static cube *levels[32]; \
    levels[worldscale] = worldroot; \
    int lshift = worldscale; \
    ivec lsizemask(invray.x>0 ? 1 : 0, invray.y>0 ? 1 : 0, invray.z>0 ? 1 : 0); \

#define CHECKINSIDEWORLD \
    if(!insideworld(o)) \
    { \
        float disttoworld = 0, exitworld = 1e16f; \
        loopi(3) \
        { \
            float c = v[i]; \
            if(c<0 || c>=hdr.worldsize) \
            { \
                float d = ((invray[i]>0?0:hdr.worldsize)-c)*invray[i]; \
                if(d<0) return (radius>0?radius:-1); \
                disttoworld = max(disttoworld, 0.1f + d); \
            } \
            float e = ((invray[i]>0?hdr.worldsize:0)-c)*invray[i]; \
            exitworld = min(exitworld, e); \
        } \
        if(disttoworld > exitworld) return (radius>0?radius:-1); \
        pushvec(v, ray, disttoworld); \
        dist += disttoworld; \
    }

#define DOWNOCTREE(disttoent, earlyexit) \
        cube *lc = levels[lshift]; \
		for(;;) \
		{ \
            lshift--; \
            lc += octastep(x, y, z, lshift); \
			if(lc->ext && lc->ext->ents && dent > 1e15f) \
			{ \
				dent = disttoent(lc->ext->ents, oclast, o, ray, radius, mode, t); \
				if(dent < 1e15f earlyexit) return min(dent, dist); \
				oclast = lc->ext->ents; \
			} \
			if(lc->children==NULL) break; \
			lc = lc->children; \
            levels[lshift] = lc; \
		}

#define FINDCLOSEST(xclosest, yclosest, zclosest) \
        float dx = (lo.x+(lsizemask.x<<lshift)-v.x)*invray.x, \
              dy = (lo.y+(lsizemask.y<<lshift)-v.y)*invray.y, \
              dz = (lo.z+(lsizemask.z<<lshift)-v.z)*invray.z; \
        float disttonext = dx; \
        xclosest; \
        if(dy < disttonext) { disttonext = dy; yclosest; } \
        if(dz < disttonext) { disttonext = dz; zclosest; } \
		disttonext += 0.1f; \
		pushvec(v, ray, disttonext); \
        dist += disttonext;

#define UPOCTREE(exitworld) \
		x = int(v.x); \
		y = int(v.y); \
		z = int(v.z); \
        uint diff = uint(lo.x^x)|uint(lo.y^y)|uint(lo.z^z); \
        if(diff >= uint(hdr.worldsize)) exitworld; \
        diff >>= lshift; \
        if(!diff) exitworld; \
        do \
		{ \
            lshift++; \
            diff >>= 1; \
        } while(diff);

float raycube(const vec &o, const vec &ray, float radius, int mode, int size, extentity *t)
{
	if(ray.iszero()) return 0;

    INITRAYCUBE;
    CHECKINSIDEWORLD;

    int closest = -1, x = int(v.x), y = int(v.y), z = int(v.z);
	for(;;)
	{
		DOWNOCTREE(disttoent, && (mode&RAY_SHADOW));

        int lsize = 1<<lshift;

		cube &c = *lc;
		if((dist>0 || !(mode&RAY_SKIPFIRST)) &&
           (((mode&RAY_CLIPMAT) && c.ext && isclipped(c.ext->material&MATF_VOLUME)) ||
			((mode&RAY_EDITMAT) && c.ext && c.ext->material != MAT_AIR) ||
			(!(mode&RAY_PASS) && lsize==size && !isempty(c)) ||
			isentirelysolid(c) ||
            dent < dist))
		{
            if(dist < dent)
            {
                if(closest < 0)
                {
                    float dx = ((x&(~0<<lshift))+(invray.x>0 ? 0 : 1<<lshift)-v.x)*invray.x,
                          dy = ((y&(~0<<lshift))+(invray.y>0 ? 0 : 1<<lshift)-v.y)*invray.y,
                          dz = ((z&(~0<<lshift))+(invray.z>0 ? 0 : 1<<lshift)-v.z)*invray.z;
                    closest = dx > dy ? (dx > dz ? 0 : 2) : (dy > dz ? 1 : 2);
                }
                hitsurface = vec(0, 0, 0);
                hitsurface[closest] = ray[closest]>0 ? -1 : 1;
            }
			return min(dent, dist);
		}

        ivec lo(x&(~0<<lshift), y&(~0<<lshift), z&(~0<<lshift));

        if(!isempty(c))
		{
			float f = 0;
			setcubeclip(c, lo, lsize);
			if(raycubeintersect(c, v, ray, dent-dist, f) && (dist+f>0 || !(mode&RAY_SKIPFIRST)))
				return min(dent, dist+f);
		}

        FINDCLOSEST(closest = 0, closest = 1, closest = 2);

		if(radius>0 && dist>=radius) return min(dent, dist);

        UPOCTREE(return min(dent, radius>0 ? radius : dist));
	}
}

// optimized version for lightmap shadowing... every cycle here counts!!!
float shadowray(const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
    INITRAYCUBE;
    CHECKINSIDEWORLD;

	int x = int(v.x), y = int(v.y), z = int(v.z);
	for(;;)
	{
		DOWNOCTREE(shadowent, );

		cube &c = *lc;
		if(isentirelysolid(c)) return dist;

        ivec lo(x&(~0<<lshift), y&(~0<<lshift), z&(~0<<lshift));

        if(!isempty(c))
		{
			float f = 0;
            setcubeclip(c, lo, 1<<lshift);
            if(shadowcubeintersect(c, v, ray, f)) return dist+f;
		}

        FINDCLOSEST( , , );

		if(dist>=radius) return dist;

        UPOCTREE(return radius);
	}
}

float rayent(const vec &o, const vec &ray, vec &hitpos, float radius, int mode, int size, int &orient, int &ent)
{
	hitent = -1;
	float d = raycubepos(o, ray, hitpos, hitentdist = radius, mode, size);
	orient = hitorient;
	ent = (hitentdist == d) ? hitent : -1;
	return d;
}

float raycubepos(const vec &o, const vec &ray, vec &hitpos, float radius, int mode, int size)
{
	hitpos = ray;
	float dist = raycube(o, ray, radius, mode, size);
	hitpos.mul(dist);
	hitpos.add(o);
	return dist;
}

bool raycubelos(const vec &o, const vec &dest, vec &hitpos)
{
	vec ray(dest);
	ray.sub(o);
	float mag = ray.magnitude();
    ray.mul(1/mag);
	float distance = raycubepos(o, ray, hitpos, mag, RAY_CLIPMAT|RAY_POLY);
	return distance >= mag;
}

float rayfloor(const vec &o, vec &floor, int mode, float radius)
{
	if(o.z<=0) return -1;
    hitsurface = vec(0, 0, 1);
	float dist = raycube(o, vec(0, 0, -1), radius, mode);
	if(dist<0 || (radius>0 && dist>=radius)) return dist;
    floor = hitsurface;
	return dist;
}

/////////////////////////  entity collision  ///////////////////////////////////////////////

// info about collisions
bool inside; // whether an internal collision happened
physent *hitplayer; // whether the collection hit a player
int hitflags = HITFLAG_NONE;
vec wall; // just the normal vectors.

bool ellipserectcollide(physent *d, const vec &dir, const vec &o, const vec &center, float yaw, float xr, float yr, float hi, float lo)
{
    float below = (o.z+center.z-lo) - (d->o.z+d->aboveeye),
          above = (d->o.z-d->height) - (o.z+center.z+hi);
    if(below>=0 || above>=0) return true;

    vec yo(d->o);
    yo.sub(o);
    yo.rotate_around_z(-yaw*RAD);
    yo.sub(center);

    float dx = clamp(yo.x, -xr, xr) - yo.x, dy = clamp(yo.y, -yr, yr) - yo.y,
          dist = sqrtf(dx*dx + dy*dy) - d->radius;
    if(dist < 0)
    {
        int sx = yo.x <= -xr ? -1 : (yo.x >= xr ? 1 : 0),
            sy = yo.y <= -yr ? -1 : (yo.y >= yr ? 1 : 0);
        if(dist > (yo.z < 0 ? below : above) && (sx || sy))
        {
            vec ydir(dir);
            ydir.rotate_around_z(-yaw*RAD);
            if(sx*yo.x - xr > sy*yo.y - yr)
            {
                if(dir.iszero() || sx*ydir.x < -1e-6f)
                {
                    wall = vec(sx, 0, 0);
                    wall.rotate_around_z(yaw*RAD);
                    return false;
                }
            }
            else if(dir.iszero() || sy*ydir.y < -1e-6f)
            {
                wall = vec(0, sy, 0);
                wall.rotate_around_z(yaw*RAD);
                return false;
            }
        }
        if(yo.z < 0)
        {
            if(dir.iszero() || (dir.z > 0 && (d->type>=ENT_INANIMATE || below >= d->zmargin-(d->height+d->aboveeye)/4.0f)))
            {
                wall = vec(0, 0, -1);
                return false;
            }
        }
        else if(dir.iszero() || (dir.z < 0 && (d->type>=ENT_INANIMATE || above >= d->zmargin-(d->height+d->aboveeye)/3.0f)))
        {
            wall = vec(0, 0, 1);
            return false;
        }
        inside = true;
    }
    return true;
}

bool ellipsecollide(physent *d, const vec &dir, const vec &o, const vec &center, float yaw, float xr, float yr, float hi, float lo)
{
    float below = (o.z+center.z-lo) - (d->o.z+d->aboveeye),
          above = (d->o.z-d->height) - (o.z+center.z+hi);
	if(below>=0 || above>=0) return true;
    vec yo(center);
    yo.rotate_around_z(yaw*RAD);
    yo.add(o);
    float x = yo.x - d->o.x, y = yo.y - d->o.y;
    float angle = atan2f(y, x), dangle = angle-(d->yaw+90)*RAD, eangle = angle-(yaw+90)*RAD;
	float dx = d->xradius*cosf(dangle), dy = d->yradius*sinf(dangle);
    float ex = xr*cosf(eangle), ey = yr*sinf(eangle);
	float dist = sqrtf(x*x + y*y) - sqrtf(dx*dx + dy*dy) - sqrtf(ex*ex + ey*ey);
	if(dist < 0)
	{
        if(dist > (d->o.z < yo.z ? below : above) && (dir.iszero() || x*dir.x + y*dir.y > 0))
		{
            wall = vec(-x, -y, 0);
            if(!wall.iszero()) wall.normalize();
			return false;
		}
        if(d->o.z < yo.z)
		{
            if(dir.iszero() || (dir.z > 0 && (d->type>=ENT_INANIMATE || below >= d->zmargin-(d->height+d->aboveeye)/4.0f)))
            {
                wall = vec(0, 0, -1);
				return false;
			}
        }
        else if(dir.iszero() || (dir.z < 0 && (d->type>=ENT_INANIMATE || above >= d->zmargin-(d->height+d->aboveeye)/3.0f)))
		{
            wall = vec(0, 0, 1);
			return false;
		}
        inside = true;
	}
	return true;
}

bool rectcollide(physent *d, const vec &dir, const vec &o, float xr, float yr, float hi, float lo, uchar visible)
{
	vec s(d->o);
	s.sub(o);
    float dxr = d->collidetype==COLLIDE_AABB ? d->xradius : d->radius, dyr = d->collidetype==COLLIDE_AABB ? d->yradius : d->radius;
    xr += dxr;
    yr += dyr;
	float zr = s.z>0 ? d->height+hi : d->aboveeye+lo;
	float ax = fabs(s.x)-xr;
	float ay = fabs(s.y)-yr;
	float az = fabs(s.z)-zr;
	if(ax>0 || ay>0 || az>0) return true;
	wall.x = wall.y = wall.z = 0;
#define TRYCOLLIDE(dim, ON, OP, N, P) \
	{ \
        if(s.dim<0) { if(visible&(1<<ON) && (dir.iszero() || (dir.dim>0 && (d->type>=ENT_INANIMATE || (N))))) { wall.dim = -1; return false; } } \
        else if(visible&(1<<OP) && (dir.iszero() || (dir.dim<0 && (d->type>=ENT_INANIMATE || (P))))) { wall.dim = 1; return false; } \
	}
    if(ax>ay && ax>az) TRYCOLLIDE(x, O_LEFT, O_RIGHT, ax > -dxr, ax > -dxr);
    if(ay>az) TRYCOLLIDE(y, O_BACK, O_FRONT, ay > -dyr, ay > -dyr);
	TRYCOLLIDE(z, O_BOTTOM, O_TOP,
         az >= d->zmargin-(d->height+d->aboveeye)/4.0f,
         az >= d->zmargin-(d->height+d->aboveeye)/3.0f);
	inside = true;
	return true;
}

#define DYNENTCACHESIZE 1024

static uint dynentframe = 0;

static struct dynentcacheentry
{
	int x, y;
	uint frame;
	vector<physent *> dynents;
} dynentcache[DYNENTCACHESIZE];

void cleardynentcache()
{
	dynentframe++;
	if(!dynentframe || dynentframe == 1) loopi(DYNENTCACHESIZE) dynentcache[i].frame = 0;
	if(!dynentframe) dynentframe = 1;
}

VARF(dynentsize, 4, 7, 12, cleardynentcache());

#define DYNENTHASH(x, y) (((((x)^(y))<<5) + (((x)^(y))>>5)) & (DYNENTCACHESIZE - 1))

const vector<physent *> &checkdynentcache(int x, int y)
{
	dynentcacheentry &dec = dynentcache[DYNENTHASH(x, y)];
	if(dec.x == x && dec.y == y && dec.frame == dynentframe) return dec.dynents;
	dec.x = x;
	dec.y = y;
	dec.frame = dynentframe;
	dec.dynents.setsizenodelete(0);
	int numdyns = game::numdynents(), dsize = 1<<dynentsize, dx = x<<dynentsize, dy = y<<dynentsize;
	loopi(numdyns)
	{
		dynent *d = game::iterdynents(i);
		if(!d || d->state != CS_ALIVE ||
			d->o.x+d->radius <= dx || d->o.x-d->radius >= dx+dsize ||
			d->o.y+d->radius <= dy || d->o.y-d->radius >= dy+dsize)
			continue;
		dec.dynents.add(d);
	}
	return dec.dynents;
}

#define loopdynentcachebb(curx, cury, x1, y1, x2, y2) \
    for(int curx = max(int(x1), 0)>>dynentsize, endx = min(int(x2), hdr.worldsize-1)>>dynentsize; curx <= endx; curx++) \
    for(int cury = max(int(y1), 0)>>dynentsize, endy = min(int(y2), hdr.worldsize-1)>>dynentsize; cury <= endy; cury++)

#define loopdynentcache(curx, cury, o, radius) \
    loopdynentcachebb(curx, cury, o.x-radius, o.y-radius, o.x+radius, o.y+radius)

void updatedynentcache(physent *d)
{
    loopdynentcache(x, y, d->o, d->radius)
	{
		dynentcacheentry &dec = dynentcache[DYNENTHASH(x, y)];
		if(dec.x != x || dec.y != y || dec.frame != dynentframe || dec.dynents.find(d) >= 0) continue;
		dec.dynents.add(d);
	}
}

bool overlapsdynent(const vec &o, float radius)
{
    loopdynentcache(x, y, o, radius)
	{
		const vector<physent *> &dynents = checkdynentcache(x, y);
		loopv(dynents)
		{
			physent *d = dynents[i];
			if(physics::issolid(d) && o.dist(d->o)-d->radius < radius)
				return true;
		}
	}
	return false;
}

template<class E, class O>
static inline bool plcollide(physent *d, const vec &dir, physent *o)
{
    E entvol(d);
    O obvol(o);
    vec cp;
    if(mpr::collide(entvol, obvol, NULL, NULL, &cp))
    {
        vec wn = vec(cp).sub(obvol.center());
        wall = obvol.contactface(wn, dir.iszero() ? vec(wn).neg() : dir);
        if(!wall.iszero()) return false;
        inside = true;
    }
    return true;
}

bool plcollide(physent *d, const vec &dir, physent *o)
{
    if(d->o.reject(o->o, d->radius + o->radius)) return true;
    switch(d->collidetype)
    {
        case COLLIDE_ELLIPSE:
            if(o->collidetype == COLLIDE_ELLIPSE) return ellipsecollide(d, dir, o->o, vec(0, 0, 0), o->yaw, o->xradius, o->yradius, o->aboveeye, o->height);
            else return ellipserectcollide(d, dir, o->o, vec(0, 0, 0), o->yaw, o->xradius, o->yradius, o->aboveeye, o->height);
        case COLLIDE_OBB:
            if(o->collidetype == COLLIDE_ELLIPSE) return plcollide<mpr::EntOBB, mpr::EntCylinder>(d, dir, o);
            else return plcollide<mpr::EntOBB, mpr::EntOBB>(d, dir, o);
        case COLLIDE_AABB:
        default:
            return rectcollide(d, dir, o->o, o->collidetype == COLLIDE_AABB ? o->xradius : o->radius, o->collidetype == COLLIDE_AABB ? o->yradius : o->radius, o->aboveeye, o->height);
    }
}

bool plcollide(physent *d, const vec &dir)	// collide with player or monster
{
	if(d->type==ENT_CAMERA || d->state!=CS_ALIVE) return true;
    loopdynentcache(x, y, d->o, d->radius)
	{
		const vector<physent *> &dynents = checkdynentcache(x, y);
		loopv(dynents)
		{
			physent *o = dynents[i];
			if(o==d || !physics::issolid(o, d)) continue;
			if(!physics::xcollide(d, dir, o))
			{
				hitplayer = o;
				return false;
			}
		}
	}
	return true;
}

void rotatebb(vec &center, vec &radius, int yaw)
{
    static const vec2 rots[25] =
    {
        vec2(1.00000000, 0.00000000), // 0
        vec2(0.96592583, 0.25881905), // 15
        vec2(0.86602540, 0.50000000), // 30
        vec2(0.70710678, 0.70710678), // 45
        vec2(0.50000000, 0.86602540), // 60
        vec2(0.25881905, 0.96592583), // 75
        vec2(0.00000000, 1.00000000), // 90
        vec2(-0.25881905, 0.96592583), // 105
        vec2(-0.50000000, 0.86602540), // 120
        vec2(-0.70710678, 0.70710678), // 135
        vec2(-0.86602540, 0.50000000), // 150
        vec2(-0.96592583, 0.25881905), // 165
        vec2(-1.00000000, 0.00000000), // 180
        vec2(-0.96592583, -0.25881905), // 195
        vec2(-0.86602540, -0.50000000), // 210
        vec2(-0.70710678, -0.70710678), // 225
        vec2(-0.50000000, -0.86602540), // 240
        vec2(-0.25881905, -0.96592583), // 255
        vec2(-0.00000000, -1.00000000), // 270
        vec2(0.25881905, -0.96592583), // 285
        vec2(0.50000000, -0.86602540), // 300
        vec2(0.70710678, -0.70710678), // 315
        vec2(0.86602540, -0.50000000), // 330
        vec2(0.96592583, -0.25881905), // 345
        vec2(1.00000000, 0.00000000) // 360
    };

    yaw += 180;
    if(yaw < 0) yaw = 360 + yaw%360;
    else if(yaw >= 360) yaw %= 360;
    const vec2 &rot = rots[(yaw + 7)/15];
    vec2 oldcenter(center), oldradius(radius);
    center.x = oldcenter.x*rot.x - oldcenter.y*rot.y;
    center.y = oldcenter.y*rot.x + oldcenter.x*rot.y;
    radius.x = fabs(oldradius.x*rot.x) + fabs(oldradius.y*rot.y);
    radius.y = fabs(oldradius.y*rot.x) + fabs(oldradius.x*rot.y);
}

template<class E, class M>
static inline bool mmcollide(physent *d, const vec &dir, const extentity &e, const vec &center, const vec &radius, float yaw)
{
    E entvol(d);
    M mdlvol(vec(e.o).add(center), radius, yaw);
    vec cp;
    if(mpr::collide(entvol, mdlvol, NULL, NULL, &cp))
    {
        vec wn = vec(cp).sub(mdlvol.center());
        wall = mdlvol.contactface(wn, dir.iszero() ? vec(wn).neg() : dir);
        if(!wall.iszero()) return false;
        inside = true;
    }
    return true;
}

bool mmcollide(physent *d, const vec &dir, octaentities &oc)               // collide with a mapmodel
{
    if(d->type == ENT_CAMERA) return true;
    const vector<extentity *> &ents = entities::getents();
    loopv(oc.mapmodels)
    {
        extentity &e = *ents[oc.mapmodels[i]];
        mapmodelskip;
        model *m = loadmodel(NULL, e.attrs[0]);
        if(!m || !m->collide) continue;
        vec center, radius;
        m->collisionbox(0, center, radius);
        float yaw = 180 + float((e.attrs[1]+7)-(e.attrs[1]+7)%15);
        switch(d->collidetype)
        {
            case COLLIDE_ELLIPSE:
                if(m->ellipsecollide)
                {
                    //if(!mmcollide<mpr::EntCylinder, mpr::ModelEllipse>(d, dir, e, center, radius, yaw)) return false;
                    if(!ellipsecollide(d, dir, e.o, center, yaw, radius.x, radius.y, radius.z, radius.z)) return false;
                }
                //else if(!mmcollide<mpr::EntCylinder, mpr::ModelOBB>(d, dir, e, center, radius, yaw)) return false;           
                else if(!ellipserectcollide(d, dir, e.o, center, yaw, radius.x, radius.y, radius.z, radius.z)) return false;
                break;
            case COLLIDE_OBB:
                if(m->ellipsecollide)
                {
                    if(!mmcollide<mpr::EntOBB, mpr::ModelEllipse>(d, dir, e, center, radius, yaw)) return false;
                }
                else if(!mmcollide<mpr::EntOBB, mpr::ModelOBB>(d, dir, e, center, radius, yaw)) return false;
                break;
            case COLLIDE_AABB:
            default:
                rotatebb(center, radius, e.attrs[1]);
                if(!rectcollide(d, dir, center.add(e.o), radius.x, radius.y, radius.z, radius.z)) return false;
                break;
        }
    }
    return true;
}

template<class E>
static bool fuzzycollidesolid(physent *d, const vec &dir, float cutoff, cube &c, const ivec &co, int size) // collide with solid cube geometry
{
    int crad = size/2;
    if(fabs(d->o.x - co.x - crad) > d->radius + crad || fabs(d->o.y - co.y - crad) > d->radius + crad ||
       d->o.z + d->aboveeye < co.z || d->o.z - d->height > co.z + size)
        return true;

    E entvol(d);
    wall = vec(0, 0, 0);
    float bestdist = -1e10f;
    int vis = isentirelysolid(c) ? (c.ext ? c.ext->visible : 0) : 0xFF;
    loopi(6) if(vis&(1<<i))
    {
        int dim = dimension(i), dc = dimcoord(i), dimdir = 2*dc - 1;
        plane w(0, 0, 0, -(dimdir*co[dim] + dc*size));
        w[dim] = dimdir;
        vec pw = entvol.supportpoint(vec(w).neg());
        float dist = w.dist(pw);
        if(dist > 0) return true;
        if(dist <= bestdist) continue;
        if(!dir.iszero())
        {
            if(w.dot(dir) >= -cutoff*dir.magnitude()) continue;
            if(d->type<ENT_CAMERA &&
                dist < (dir.z*w.z < 0 ?
                    d->zmargin-(d->height+d->aboveeye)/(dir.z < 0 ? 3.0f : 4.0f) :
                    ((dir.x*w.x < 0 || dir.y*w.y < 0) ? -d->radius : 0)))
                continue;
        }
        wall = w;
        bestdist = dist;
    }
    if(wall.iszero())
    {
        inside = true;
        return true;
    }
    return false;
}

template<class E>
static bool fuzzycollideplanes(physent *d, const vec &dir, float cutoff, cube &c, const ivec &co, int size) // collide with deformed cube geometry
{
    setcubeclip(c, co, size);
    clipplanes &p = *c.ext->clip;

    if(fabs(d->o.x - p.o.x) > p.r.x + d->radius || fabs(d->o.y - p.o.y) > p.r.y + d->radius ||
       d->o.z + d->aboveeye < p.o.z - p.r.z || d->o.z - d->height > p.o.z + p.r.z)
        return true;

    E entvol(d);
    wall = vec(0, 0, 0);
    float bestdist = -1e10f;
    loopi(6) if(p.visible&(1<<i))
    {
        int dim = dimension(i), dimdir = 2*dimcoord(i) - 1;
        plane w(0, 0, 0, -(dimdir*p.o[dim] + p.r[dim]));
        w[dim] = dimdir;
        vec pw = entvol.supportpoint(vec(w).neg());
        float dist = w.dist(pw);
        if(dist >= 0) return true;
        if(dist <= bestdist) continue;
        if(!dir.iszero())
        {
            if(w.dot(dir) >= -cutoff*dir.magnitude()) continue;
            if(d->type<ENT_CAMERA &&
                dist < (dir.z*w.z < 0 ?
                    d->zmargin-(d->height+d->aboveeye)/(dir.z < 0 ? 3.0f : 4.0f) :
                    ((dir.x*w.x < 0 || dir.y*w.y < 0) ? -d->radius : 0)))
                continue;
        }
        wall = w;
        bestdist = dist;
    }
    loopi(p.size)
    {
        plane &w = p.p[i];
        vec pw = entvol.supportpoint(vec(w).neg());
        float dist = w.dist(pw);
        if(dist >= 0) return true;
        if(dist <= bestdist) continue;
        if(!dir.iszero())
        {
            if(w.dot(dir) >= -cutoff*dir.magnitude()) continue;
            if(d->type<ENT_CAMERA &&
                dist < (dir.z*w.z < 0 ?
                    d->zmargin-(d->height+d->aboveeye)/(dir.z < 0 ? 3.0f : 4.0f) :
                    ((dir.x*w.x < 0 || dir.y*w.y < 0) ? -d->radius : 0)))
                continue;
        }
        wall = w;
        bestdist = dist;
    }
    if(wall.iszero())
    {
        inside = true;
        return true;
    }
    return false;
}

template<class E>
static bool cubecollidesolid(physent *d, const vec &dir, float cutoff, cube &c, const ivec &co, int size) // collide with solid cube geometry
{
    int crad = size/2;
    if(fabs(d->o.x - co.x - crad) > d->radius + crad || fabs(d->o.y - co.y - crad) > d->radius + crad ||
       d->o.z + d->aboveeye < co.z || d->o.z - d->height > co.z + size)
        return true;

    E entvol(d);
    bool collided = mpr::collide(mpr::SolidCube(co, size), entvol);
    if(!collided) return true;

    wall = vec(0, 0, 0);
    float bestdist = -1e10f;
    int vis = isentirelysolid(c) ? (c.ext ? c.ext->visible : 0) : 0xFF;
    loopi(6) if(vis&(1<<i))
    {
        int dim = dimension(i), dc = dimcoord(i), dimdir = 2*dc - 1;
        plane w(0, 0, 0, -(dimdir*co[dim] + dc*size));
        w[dim] = dimdir;
        vec pw = entvol.supportpoint(vec(w).neg());
        float dist = w.dist(pw);
        if(dist <= bestdist) continue;
        if(!dir.iszero())
        {
            if(w.dot(dir) >= -cutoff*dir.magnitude()) continue;
            if(d->type<ENT_CAMERA &&
                dist < (dir.z*w.z < 0 ?
                    d->zmargin-(d->height+d->aboveeye)/(dir.z < 0 ? 3.0f : 4.0f) :
                    ((dir.x*w.x < 0 || dir.y*w.y < 0) ? -d->radius : 0)))
                continue;
        }
        //wall.add(w);
        wall = w;
        bestdist = dist;
    }
    if(wall.iszero())
    {
        inside = true;
        return true;
    }
    //wall.normalize();
    return false;
}

template<class E>
static bool cubecollideplanes(physent *d, const vec &dir, float cutoff, cube &c, const ivec &co, int size) // collide with deformed cube geometry
{
    setcubeclip(c, co, size);
    clipplanes &p = *c.ext->clip;

    if(fabs(d->o.x - p.o.x) > p.r.x + d->radius || fabs(d->o.y - p.o.y) > p.r.y + d->radius ||
       d->o.z + d->aboveeye < p.o.z - p.r.z || d->o.z - d->height > p.o.z + p.r.z)
        return true;

    E entvol(d);
    bool collided = mpr::collide(mpr::CubePlanes(p), entvol);
    if(!collided) return true;

    wall = vec(0, 0, 0);
    float bestdist = -1e10f;
    loopi(6) if(p.visible&(1<<i))
    {
        int dim = dimension(i), dimdir = 2*dimcoord(i) - 1;
        plane w(0, 0, 0, -(dimdir*p.o[dim] + p.r[dim]));
        w[dim] = dimdir;
        vec pw = entvol.supportpoint(vec(w).neg());
        float dist = w.dist(pw);
        if(dist <= bestdist) continue;
        if(!dir.iszero())
        {
            if(w.dot(dir) >= -cutoff*dir.magnitude()) continue;
            if(d->type<ENT_CAMERA &&
                dist < (dir.z*w.z < 0 ?
                    d->zmargin-(d->height+d->aboveeye)/(dir.z < 0 ? 3.0f : 4.0f) :
                    ((dir.x*w.x < 0 || dir.y*w.y < 0) ? -d->radius : 0)))
                continue;
        }
        //wall.add(w);
        wall = w;
        bestdist = dist;
    }
    loopi(p.size)
    {
        plane &w = p.p[i];
        vec pw = entvol.supportpoint(vec(w).neg());
        float dist = w.dist(pw);
        if(dist <= bestdist) continue;
        if(!dir.iszero())
        {
            if(w.dot(dir) >= -cutoff*dir.magnitude()) continue;
            if(d->type<ENT_CAMERA &&
                dist < (dir.z*w.z < 0 ?
                    d->zmargin-(d->height+d->aboveeye)/(dir.z < 0 ? 3.0f : 4.0f) :
                    ((dir.x*w.x < 0 || dir.y*w.y < 0) ? -d->radius : 0)))
                continue;
        }
        //wall.add(w);
        wall = w;
        bestdist = dist;
    }
    if(wall.iszero())
    {
        inside = true;
        return true;
    }
    //wall.normalize();
    return false;
}

static inline bool cubecollide(physent *d, const vec &dir, float cutoff, cube &c, const ivec &co, int size, bool solid)
{
    switch(d->collidetype)
    {
    case COLLIDE_AABB:
        if(isentirelysolid(c) || solid)
        {
            if(cutoff <= 0)
            {
                int crad = size/2;
                return rectcollide(d, dir, vec(co.x + crad, co.y + crad, co.z), crad, crad, size, 0, isentirelysolid(c) ? (c.ext ? c.ext->visible : 0) : 0xFF);
            }
#if 0
            else return cubecollidesolid<mpr::EntAABB>(d, dir, cutoff, c, co, size);
#else
            else return fuzzycollidesolid<mpr::EntAABB>(d, dir, cutoff, c, co, size);
#endif
        }
        else
        {
#if 0
            if(cutoff <= 0)
            {
                setcubeclip(c, co, size);
                clipplanes &p = *c.ext->clip;
                if(!p.size) return rectcollide(d, dir, p.o, p.r.x, p.r.y, p.r.z, p.r.z, p.visible);
            }
            return cubecollideplanes<mpr::EntAABB>(d, dir, cutoff, c, co, size);
#else
            return fuzzycollideplanes<mpr::EntAABB>(d, dir, cutoff, c, co, size);
#endif
        }
    case COLLIDE_OBB:
        if(isentirelysolid(c) || solid) return cubecollidesolid<mpr::EntOBB>(d, dir, cutoff, c, co, size);
        else return cubecollideplanes<mpr::EntOBB>(d, dir, cutoff, c, co, size);
    case COLLIDE_ELLIPSE:
    default:
        if(d->type < ENT_CAMERA)
        {
            if(isentirelysolid(c) || solid) return fuzzycollidesolid<mpr::EntCapsule>(d, dir, cutoff, c, co, size);
            else return fuzzycollideplanes<mpr::EntCapsule>(d, dir, cutoff, c, co, size);
        }
        else if(isentirelysolid(c) || solid) return cubecollidesolid<mpr::EntCapsule>(d, dir, cutoff, c, co, size);
        else return cubecollideplanes<mpr::EntCapsule>(d, dir, cutoff, c, co, size);
    }
}

static inline bool octacollide(physent *d, const vec &dir, float cutoff, const ivec &bo, const ivec &bs, cube *c, const ivec &cor, int size) // collide with octants
{
	loopoctabox(cor, size, bo, bs)
	{
        if(c[i].ext && c[i].ext->ents) if(!mmcollide(d, dir, *c[i].ext->ents)) return false;
		ivec o(i, cor.x, cor.y, cor.z, size);
		if(c[i].children)
		{
			if(!octacollide(d, dir, cutoff, bo, bs, c[i].children, o, size>>1)) return false;
		}
        else
        {
            bool solid = false;
            if(c[i].ext) switch(c[i].ext->material&MATF_CLIP)
			{
                case MAT_NOCLIP: continue;
                case MAT_CLIP: if(isclipped(c[i].ext->material&MATF_VOLUME) || d->type<ENT_CAMERA) solid = true; break;
            }
            if(!solid && isempty(c[i])) continue;
            if(!cubecollide(d, dir, cutoff, c[i], o, size, solid)) return false;
		}
	}
	return true;
}

static inline bool octacollide(physent *d, const vec &dir, float cutoff, const ivec &bo, const ivec &bs)
{
    int diff = (bo.x^(bo.x+bs.x)) | (bo.y^(bo.y+bs.y)) | (bo.z^(bo.z+bs.z)),
        scale = worldscale-1;
    if(diff&~((1<<scale)-1) || uint(bo.x|bo.y|bo.z|(bo.x+bs.x)|(bo.y+bs.y)|(bo.z+bs.z)) >= uint(hdr.worldsize))
       return octacollide(d, dir, cutoff, bo, bs, worldroot, ivec(0, 0, 0), hdr.worldsize>>1);
    cube *c = &worldroot[octastep(bo.x, bo.y, bo.z, scale)];
    if(c->ext && c->ext->ents && !mmcollide(d, dir, *c->ext->ents)) return false;
    scale--;
    while(c->children && !(diff&(1<<scale)))
    {
        c = &c->children[octastep(bo.x, bo.y, bo.z, scale)];
        if(c->ext && c->ext->ents && !mmcollide(d, dir, *c->ext->ents)) return false;
        scale--;
    }
    if(c->children) return octacollide(d, dir, cutoff, bo, bs, c->children, ivec(bo).mask(~((2<<scale)-1)), 1<<scale);
    bool solid = false;
    if(c->ext) switch(c->ext->material&MATF_CLIP)
    {
        case MAT_NOCLIP: return true;
        case MAT_CLIP: if(isclipped(c->ext->material&MATF_VOLUME) || d->type<ENT_CAMERA) solid = true; break;
    }
    if(!solid && isempty(*c)) return true;
    int csize = 2<<scale, cmask = ~(csize-1);
    return cubecollide(d, dir, cutoff, *c, ivec(bo).mask(cmask), csize, solid);
}

// all collision happens here
bool collide(physent *d, const vec &dir, float cutoff, bool playercol)
{
	inside = false;
	hitplayer = NULL;
	hitflags = HITFLAG_NONE;
	wall.x = wall.y = wall.z = 0;
	ivec bo(int(d->o.x-d->radius), int(d->o.y-d->radius), int(d->o.z-d->height)),
		 bs(int(d->radius)*2, int(d->radius)*2, int(d->height+d->aboveeye));
	bs.add(2);  // guard space for rounding errors
    if(!octacollide(d, dir, cutoff, bo, bs)) return false;//, worldroot, ivec(0, 0, 0), hdr.worldsize>>1)) return false; // collide with world
    return !playercol || plcollide(d, dir);
}

float pltracecollide(physent *d, const vec &from, const vec &ray, float maxdist)
{
    vec to = vec(ray).mul(maxdist).add(from);
    float x1 = floor(min(from.x, to.x)), y1 = floor(min(from.y, to.y)),
          x2 = ceil(max(from.x, to.x)), y2 = ceil(max(from.y, to.y));
    float bestdist = 1e16f; int bestflags = HITFLAG_NONE;
    loopdynentcachebb(x, y, x1, y1, x2, y2)
    {
        const vector<physent *> &dynents = checkdynentcache(x, y);
        loopv(dynents)
        {
            physent *o = dynents[i];
            if(!physics::issolid(o, d)) continue;
            float dist = 1e16f;
            if(!physics::xtracecollide(d, from, to, x1, x2, y1, y2, maxdist, dist, o) && dist < bestdist)
            {
                bestdist = dist;
                if(dist <= maxdist) { hitplayer = o; bestflags = hitflags; }
            }
        }
    }
    hitflags = bestflags;
    return bestdist <= maxdist ? bestdist : -1;
}

float tracecollide(physent *d, const vec &o, const vec &ray, float maxdist, int mode, bool playercol)
{
    hitsurface = vec(0, 0, 0);
    hitplayer = NULL;
    hitflags = HITFLAG_NONE;
    float dist = raycube(o, ray, maxdist+1e-3f, mode);
    if(playercol)
    {
        float pldist = pltracecollide(d, o, ray, min(dist, maxdist));
        if(pldist >= 0 && pldist < dist) dist = pldist;
    }
    return dist <= maxdist ? dist : -1;
}

void phystest()
{
	static const char *states[] = {"float", "fall", "slide", "slope", "floor", "step up", "step down", "bounce"};
//    printf ("PHYS(pl): %s, air %d, floor: (%f, %f, %f), vel: (%f, %f, %f), g: (%f, %f, %f)\n", states[player->physstate], player->timeinair, player->floor.x, player->floor.y, player->floor.z, player->vel.x, player->vel.y, player->vel.z, player->falling.x, player->falling.y, player->falling.z);
    printf ("PHYS(cam): %s, air %d, floor: (%f, %f, %f), vel: (%f, %f, %f), g: (%f, %f, %f)\n", states[camera1->physstate], camera1->timeinair, camera1->floor.x, camera1->floor.y, camera1->floor.z, camera1->vel.x, camera1->vel.y, camera1->vel.z, camera1->falling.x, camera1->falling.y, camera1->falling.z);
}

COMMAND(phystest, "");

void vecfromyawpitch(float yaw, float pitch, int move, int strafe, vec &m)
{
	if(move)
	{
		m.x = move*sinf(RAD*yaw);
		m.y = move*-cosf(RAD*yaw);
	}
	else m.x = m.y = 0;

	if(pitch)
	{
		m.x *= cosf(RAD*pitch);
		m.y *= cosf(RAD*pitch);
		m.z = move*sinf(RAD*pitch);
	}
	else m.z = 0;

	if(strafe)
	{
		m.x += strafe*-cosf(RAD*yaw);
		m.y += strafe*-sinf(RAD*yaw);
	}
}

void vectoyawpitch(const vec &v, float &yaw, float &pitch)
{
	yaw = -(float)atan2(v.x, v.y)/RAD + 180;
	pitch = asin(v.z/v.magnitude())/RAD;
}

bool intersect(physent *d, const vec &from, const vec &to, float &dist)   // if lineseg hits entity bounding box
{
    vec bottom(d->o), top(d->o);
    bottom.z -= d->height;
    top.z += d->aboveeye;
    return linecylinderintersect(from, to, bottom, top, d->radius, dist);
}

bool overlapsbox(const vec &d, float h1, float r1, const vec &v, float h2, float r2)
{
	return d.x <= v.x+r2+r1 && d.x >= v.x-r2-r1 &&
	d.y <= v.y+r2+r1 && d.y >= v.y-r2-r1 &&
	d.z <= v.z+h2+h1 && d.z >= v.z-h2-h1;
}

bool getsight(vec &o, float yaw, float pitch, vec &q, vec &v, float mdist, float fovx, float fovy)
{
	float dist = o.dist(q);

	if(dist <= mdist)
	{
		float x = fabs((asin((q.z-o.z)/dist)/RAD)-pitch);
		float y = fabs((-(float)atan2(q.x-o.x, q.y-o.y)/PI*180+180)-yaw);
		if(x <= fovx && y <= fovy) return raycubelos(o, q, v);
	}
	return false;
}
