#include "pch.h"
#include "engine.h"

#define NORMAL_BITS 11

struct nvec : svec
{
	nvec(const vec &v) : svec(short(v.x*(1<<NORMAL_BITS)), short(v.y*(1<<NORMAL_BITS)), short(v.z*(1<<NORMAL_BITS))) {}

	float dot(const vec &o) const
	{
		return x*o.x + y*o.y + z*o.z;
	}

	vec tovec() const { return vec(x, y, z).normalize(); }
};

struct normal
{
	uchar face;
	nvec surface;
	nvec average;
};

struct nkey
{
	ivec v;

	nkey() {}
	nkey(const ivec &origin, const vvec &offset)
	 : v(((origin.x&~VVEC_INT_MASK)<<VVEC_FRAC) | offset.x,
		 ((origin.y&~VVEC_INT_MASK)<<VVEC_FRAC) | offset.y,
		 ((origin.z&~VVEC_INT_MASK)<<VVEC_FRAC) | offset.z)
	{}
};

struct nval
{
	vector<normal> normals;
};

static inline bool htcmp(const nkey &x, const nkey &y)
{
	return x.v == y.v;
}

static inline uint hthash(const nkey &k)
{
	return k.v.x^k.v.y^k.v.z;
}

hashtable<nkey, nval> normals;

VARW(lerpangle, 0, 44, 180);

static float lerpthreshold = 0;

void addnormal(const ivec &origin, int orient, const vvec &offset, const vec &surface)
{
	nkey key(origin, offset);
	nval &val = normals[key];

	uchar face = orient<<3;
	int dim = dimension(orient), r = R[dim], c = C[dim], originr = origin[r]<<VVEC_FRAC, originc = origin[c]<<VVEC_FRAC;
	if(originr >= int(offset[r])+(originr&(~VVEC_INT_MASK<<1))) face |= 1;
	if(originc >= int(offset[c])+(originc&(~VVEC_INT_MASK<<1))) face |= 2;

	loopv(val.normals) if(val.normals[i].face == face) return;

	normal n = {face, surface, surface};
	loopv(val.normals)
	{
		normal &o = val.normals[i];
		if(o.face != n.face && o.surface.dot(surface) > lerpthreshold)
		{
			o.average.add(n.surface);
			n.average.add(o.surface);
		}
	}
	val.normals.add(n);
}

bool findnormal(const ivec &origin, int orient, const vvec &offset, vec &v, int index)
{
	nkey key(origin, offset);
	nval *val = normals.access(key);
	if(!val) return false;

	uchar face = orient<<3;
	int dim = dimension(orient), r = R[dim], c = C[dim];

	if(index>=0)
	{
		const ivec &coords = cubecoords[fv[orient][index]];
		if(!coords[r]) face |= 1;
		if(!coords[c]) face |= 2;
	}
	else
	{
		int originr = origin[r]<<VVEC_FRAC, originc = origin[c]<<VVEC_FRAC;
		if(originr >= int(offset[r])+(originr&(~VVEC_INT_MASK<<1))) face |= 1;
		if(originc >= int(offset[c])+(originc&(~VVEC_INT_MASK<<1))) face |= 2;
	}

	loopv(val->normals)
	{
		normal &n = val->normals[i];
		if(n.face == face)
		{
			v = n.average.tovec();
			return true;
		}
	}
	return false;
}

VARW(lerpsubdiv, 0, 2, 4);
VARW(lerpsubdivsize, 4, 4, 128);

static uint progress = 0;

void show_calcnormals_progress()
{
	float bar1 = float(progress) / float(allocnodes);
	show_out_of_renderloop_progress(bar1, "computing normals...");
}

#define CHECK_PROGRESS(exit) CHECK_CALCLIGHT_PROGRESS(exit, show_calcnormals_progress)

void addnormals(cube &c, const ivec &o, int size)
{
	CHECK_PROGRESS(return);

	if(c.children)
	{
		progress++;
		size >>= 1;
		loopi(8) addnormals(c.children[i], ivec(i, o.x, o.y, o.z, size), size);
		return;
	}
	else if(isempty(c)) return;

	vvec vvecs[8];
	bool usefaces[6];
	int vertused[8];
	calcverts(c, o.x, o.y, o.z, size, vvecs, usefaces, vertused, false/*lodcube*/);
	vec verts[8];
	loopi(8) if(vertused[i]) verts[i] = vvecs[i].tovec(o);
	loopi(6) if(usefaces[i])
	{
		CHECK_PROGRESS(return);
		if(c.texture[i] == DEFAULT_SKY) continue;

		plane planes[2];
		int numplanes = genclipplane(c, i, verts, planes);
		if(!numplanes) continue;
		int subdiv = 0;
		if(lerpsubdiv && size > lerpsubdivsize) // && faceedges(c, i) == F_SOLID)
		{
			subdiv = 1<<lerpsubdiv;
			while(size/subdiv < lerpsubdivsize) subdiv >>= 1;
		}
		vec avg;
		if(numplanes >= 2)
		{
			avg = planes[0];
			avg.add(planes[1]);
			avg.normalize();
		}
		int index = faceverts(c, i, 0);
		loopj(4)
		{
			const vvec &v = vvecs[index];
			const vec &cur = numplanes < 2 || j == 1 ? planes[0] : (j == 3 ? planes[1] : avg);
			addnormal(o, i, v, cur);
			index = faceverts(c, i, (j+1)%4);
			if(subdiv < 2) continue;
			const vvec &v2 = vvecs[index];
			vvec dv(v2);
			dv.sub(v);
			dv.div(subdiv);
			vvec vs(v);
			if(dv.iszero()) continue;
			if(numplanes < 2) loopk(subdiv - 1)
			{
				vs.add(dv);
				addnormal(o, i, vs, planes[0]);
			}
			else
			{
				vec dn(j==0 ? planes[0] : (j == 2 ? planes[1] : avg));
				dn.sub(cur);
				dn.div(subdiv);
				vec n(cur);
				loopk(subdiv - 1)
				{
					vs.add(dv);
					n.add(dn);
					addnormal(o, i, vs, vec(dn).normalize());
				}
			}
		}
	}
}

void calcnormals()
{
	if(!lerpangle) return;
	lerpthreshold = (1<<NORMAL_BITS)*cos(lerpangle*RAD);
	progress = 1;
	loopi(8) addnormals(worldroot[i], ivec(i, 0, 0, 0, hdr.worldsize/2), hdr.worldsize/2);
}

void clearnormals()
{
	normals.clear();
}

void calclerpverts(const vec &origin, const vec *p, const vec *n, const vec &ustep, const vec &vstep, lerpvert *lv, int &numv)
{
	float ul = ustep.squaredlen(), vl = vstep.squaredlen();
	int i = 0;
	loopj(numv)
	{
		if(j)
		{
			if(p[j] == p[j-1] && n[j] == n[j-1]) continue;
			if(j == numv-1 && p[j] == p[0] && n[j] == n[0]) continue;
		}
		vec dir(p[j]);
		dir.sub(origin);
		lv[i].normal = n[j];
		lv[i].u = ustep.dot(dir)/ul;
		lv[i].v = vstep.dot(dir)/vl;
		i++;
	}
	numv = i;
}

void setlerpstep(float v, lerpbounds &bounds)
{
	if(bounds.min->v + 1 > bounds.max->v)
	{
		bounds.nstep = vec(0, 0, 0);
		bounds.normal = bounds.min->normal;
		if(bounds.min->normal != bounds.max->normal)
		{
			bounds.normal.add(bounds.max->normal);
			bounds.normal.normalize();
		}
		bounds.ustep = 0;
		bounds.u = bounds.min->u;
		return;
	}

	bounds.nstep = bounds.max->normal;
	bounds.nstep.sub(bounds.min->normal);
	bounds.nstep.div(bounds.max->v-bounds.min->v);

	bounds.normal = bounds.nstep;
	bounds.normal.mul(v - bounds.min->v);
	bounds.normal.add(bounds.min->normal);

	bounds.ustep = (bounds.max->u-bounds.min->u) / (bounds.max->v-bounds.min->v);
	bounds.u = bounds.ustep * (v-bounds.min->v) + bounds.min->u;
}

void initlerpbounds(const lerpvert *lv, int numv, lerpbounds &start, lerpbounds &end)
{
	const lerpvert *first = &lv[0], *second = NULL;
	loopi(numv-1)
	{
		if(lv[i+1].v < first->v) { second = first; first = &lv[i+1]; }
		else if(!second || lv[i+1].v < second->v) second = &lv[i+1];
	}

	if(int(first->v) < int(second->v)) { start.min = end.min = first; }
	else if(first->u > second->u) { start.min = second; end.min = first; }
	else { start.min = first; end.min = second; }

	start.max = (start.min == lv ? &lv[numv-1] : start.min-1);
	end.max = (end.min == &lv[numv-1] ? lv : end.min+1);

	setlerpstep(0, start);
	setlerpstep(0, end);
}

void updatelerpbounds(float v, const lerpvert *lv, int numv, lerpbounds &start, lerpbounds &end)
{
	if(v >= start.max->v)
	{
		const lerpvert *next = (start.max == lv ? &lv[numv-1] : start.max-1);
		if(next->v > start.max->v)
		{
			start.min = start.max;
			start.max = next;
			setlerpstep(v, start);
		}
	}
	if(v >= end.max->v)
	{
		const lerpvert *next = (end.max == &lv[numv-1] ? lv : end.max+1);
		if(next->v > end.max->v)
		{
			end.min = end.max;
			end.max = next;
			setlerpstep(v, end);
		}
	}
}

void lerpnormal(float v, const lerpvert *lv, int numv, lerpbounds &start, lerpbounds &end, vec &normal, vec &nstep)
{
	updatelerpbounds(v, lv, numv, start, end);

	if(start.u + 1 > end.u)
	{
		nstep = vec(0, 0, 0);
		normal = start.normal;
		normal.add(end.normal);
		normal.normalize();
	}
	else
	{
		vec nstart(start.normal), nend(end.normal);
		nstart.normalize();
		nend.normalize();

		nstep = nend;
		nstep.sub(nstart);
		nstep.div(end.u-start.u);

		normal = nstep;
		normal.mul(-start.u);
		normal.add(nstart);
		normal.normalize();
	}

	start.normal.add(start.nstep);
	start.u += start.ustep;

	end.normal.add(end.nstep);
	end.u += end.ustep;
}

void newnormals(cube &c)
{
	if(!c.ext) newcubeext(c);
	if(!c.ext->normals)
	{
		c.ext->normals = new surfacenormals[6];
		memset(c.ext->normals, 128, 6*sizeof(surfacenormals));
	}
}

void freenormals(cube &c)
{
	if(c.ext) DELETEA(c.ext->normals);
}

