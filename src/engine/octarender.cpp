// octarender.cpp: fill vertex arrays with different cube surfaces.

#include "pch.h"
#include "engine.h"

vector<vertex> verts;
vector<grasstri> grasstris;

struct cstat { int size, nleaf, nnode, nface; } cstats[32];

VAR(showcstats, 0, 0, 1);

void printcstats()
{
	if(showcstats) loopi(32)
	{
		if(!cstats[i].size) continue;
		conoutf("%d: %d faces, %d leafs, %d nodes", cstats[i].size, cstats[i].nface, cstats[i].nleaf, cstats[i].nnode);
	}
}

VARF(floatvtx, 0, 0, 1, allchanged());

#define GENVERTS(type, ptr, body) \
	{ \
		type *f = (type *)ptr; \
		loopv(verts) \
		{ \
			const vertex &v = verts[i]; \
			f->x = v.x; \
			f->y = v.y; \
			f->z = v.z; \
			body; \
			f++; \
		} \
	}
#define GENVERTSUV(type, ptr, body) GENVERTS(type, ptr, { f->u = v.u; f->v = v.v; body; })

void genverts(void *buf)
{
	if(renderpath==R_FIXEDFUNCTION)
	{
		if(nolights)
		{
			if(floatvtx) { GENVERTS(fvertexffc, buf, {}); }
			else { GENVERTS(vertexffc, buf, {}); }
		}
		else if(floatvtx) { GENVERTSUV(fvertexff, buf, {}); }
		else { GENVERTSUV(vertexff, buf, {}); }
	}
    else if(floatvtx) { GENVERTSUV(fvertex, buf, { f->n = v.n; }); }
}

struct vboinfo
{
	int uses;
};

static inline uint hthash(GLuint key)
{
	return key;
}

static inline bool htcmp(GLuint x, GLuint y)
{
	return x==y;
}

hashtable<GLuint, vboinfo> vbos;

VAR(printvbo, 0, 0, 1);
VARF(vbosize, 0, 0, 1024*1024, allchanged());

enum
{
	VBO_VBUF = 0,
	VBO_EBUF_L0,
	VBO_EBUF_L1,
	VBO_SKYBUF_L0,
	VBO_SKYBUF_L1,
	NUMVBO
};

static vector<char> vbodata[NUMVBO];
static vector<vtxarray *> vbovas[NUMVBO];

void destroyvbo(GLuint vbo)
{
	vboinfo &vbi = vbos[vbo];
	if(vbi.uses <= 0) return;
	vbi.uses--;
	if(!vbi.uses) glDeleteBuffers_(1, &vbo);
}

void genvbo(int type, void *buf, int len, vtxarray **vas, int numva)
{
	GLuint vbo;
	glGenBuffers_(1, &vbo);
	GLenum target = type==VBO_VBUF ? GL_ARRAY_BUFFER_ARB : GL_ELEMENT_ARRAY_BUFFER_ARB;
	glBindBuffer_(target, vbo);
	glBufferData_(target, len, buf, GL_STATIC_DRAW_ARB);
	glBindBuffer_(target, 0);
	
	vboinfo &vbi = vbos[vbo]; 
	vbi.uses = numva;
	
	if(printvbo) conoutf("vbo %d: type %d, size %d, %d uses", vbo, type, len, numva);

	loopi(numva)
	{
		vtxarray *va = vas[i];
		switch(type)
		{
			case VBO_VBUF: va->vbufGL = vbo; break;
			case VBO_EBUF_L0: va->l0.ebufGL = vbo; break;
			case VBO_EBUF_L1: va->l1.ebufGL = vbo; break;
			case VBO_SKYBUF_L0: va->l0.skybufGL = vbo; break;
			case VBO_SKYBUF_L1: va->l1.skybufGL = vbo; break;
		}
	}
}

void flushvbo(int type = -1)
{
	if(type < 0)
	{
		loopi(NUMVBO) flushvbo(i);
		return;
	}

	vector<char> &data = vbodata[type];
	if(data.empty()) return;
	vector<vtxarray *> &vas = vbovas[type];
	genvbo(type, data.getbuf(), data.length(), vas.getbuf(), vas.length());
	data.setsizenodelete(0);
	vas.setsizenodelete(0);
}

void *addvbo(vtxarray *va, int type, void *buf, int len)
{
	int minsize = type==VBO_VBUF ? min(vbosize, int(VTXSIZE) << 16) : vbosize;

	if(len >= minsize)
	{
		genvbo(type, buf, len, &va, 1);
		return 0;
	}

	vector<char> &data = vbodata[type];
	vector<vtxarray *> &vas = vbovas[type];

	if(data.length() && data.length() + len > minsize) flushvbo(type);

	data.reserve(len);

	size_t offset = data.length();
	data.reserve(len);
	memcpy(&data.getbuf()[offset], buf, len);
	data.ulen += len;

	vas.add(va); 

	if(data.length() >= minsize) flushvbo(type);

	return (void *)offset;
}
 
struct vechash
{
	static const int size = 1<<16;
	int table[size];
	vector<int> chain;

	vechash() { clear(); }
	void clear() { loopi(size) table[i] = -1; chain.setsizenodelete(0); }

    int access(const vvec &v, short tu, short tv, const bvec &n)
	{
		const uchar *iv = (const uchar *)&v;
		uint h = 5381;
		loopl(sizeof(v)) h = ((h<<5)+h)^iv[l];
		h = h&(size-1);
		for(int i = table[h]; i>=0; i = chain[i])
		{
			const vertex &c = verts[i];
            if(c.x==v.x && c.y==v.y && c.z==v.z && c.n==n)
			{
				 if(!tu && !tv) return i; 
				 if(c.u==tu && c.v==tv) return i;
			}
		}
		vertex &vtx = verts.add();
		((vvec &)vtx) = v;
		vtx.u = tu;
		vtx.v = tv;
		vtx.n = n;
		chain.add(table[h]);
		return table[h] = verts.length()-1;
	}
};

vechash vh;

struct sortkey
{
	 ushort tex, lmid, envmap;
	 sortkey() {}
	 sortkey(ushort tex, ushort lmid, ushort envmap = EMID_NONE)
	  : tex(tex), lmid(lmid), envmap(envmap)
	 {}

	 bool operator==(const sortkey &o) const { return tex==o.tex && lmid==o.lmid && envmap==o.envmap; }
};

struct sortval
{
	 int unlit;
     usvector dims[6];

	 sortval() : unlit(0) {}
};

static inline bool htcmp(const sortkey &x, const sortkey &y)
{
	return x == y;
}

static inline uint hthash(const sortkey &k)
{
	return k.tex + k.lmid*9741;
}

struct lodcollect
{
	hashtable<sortkey, sortval> indices;
	vector<sortkey> texs;
	vector<materialsurface> matsurfs;
	usvector skyindices, explicitskyindices;
	int curtris;
	uint offsetindices;

	int size() { return texs.length()*sizeof(elementset) + (hasVBO ? 0 : (3*curtris+skyindices.length()+explicitskyindices.length())*sizeof(ushort)) + matsurfs.length()*sizeof(materialsurface); }

	void clear()
	{
		curtris = 0;
		offsetindices = 0;
		indices.clear();
		skyindices.setsizenodelete(0);
		explicitskyindices.setsizenodelete(0);
		matsurfs.setsizenodelete(0);
	}

	void remapunlit(vector<sortkey> &remap)
	{
		uint lastlmid = LMID_AMBIENT, firstlmid = LMID_AMBIENT;
		int firstlit = -1;
		loopv(texs)
		{
			sortkey &k = texs[i];
			if(k.lmid>=LMID_RESERVED) 
			{
				lastlmid = lightmaps[k.lmid-LMID_RESERVED].unlitx>=0 ? k.lmid : LMID_AMBIENT;
				if(firstlit<0)
				{
					firstlit = i;
					firstlmid = lastlmid;
				}
			}
			else if(k.lmid==LMID_AMBIENT && lastlmid!=LMID_AMBIENT)
			{
				sortval &t = indices[k];
				if(t.unlit<=0) t.unlit = lastlmid;
			}
		}
		if(firstlmid!=LMID_AMBIENT) loopi(firstlit)
		{
			sortkey &k = texs[i];
			if(k.lmid!=LMID_AMBIENT) continue;
			indices[k].unlit = firstlmid;
		} 
		loopv(remap)
		{
			sortkey &k = remap[i];
			sortval &t = indices[k];
			if(t.unlit<=0) continue; 
			LightMap &lm = lightmaps[t.unlit-LMID_RESERVED];
			short u = short((lm.unlitx + 0.5f) * SHRT_MAX/LM_PACKW), 
				  v = short((lm.unlity + 0.5f) * SHRT_MAX/LM_PACKH);
            loopl(6) loopvj(t.dims[l])
			{
				vertex &vtx = verts[t.dims[l][j]];
				if(!vtx.u && !vtx.v)
				{
					vtx.u = u;
					vtx.v = v;
				}
				else if(vtx.u != u || vtx.v != v) 
				{
					// necessary to copy these in case vechash reallocates verts before copying vtx
					vvec vv = vtx;
					bvec n = vtx.n;
                    t.dims[l][j] = vh.access(vv, u, v, n);
				}
			}
			sortval *dst = indices.access(sortkey(k.tex, t.unlit, k.envmap));
            if(dst) loopl(6) loopvj(t.dims[l]) dst->dims[l].add(t.dims[l][j]);
		}
	}
					
	void optimize()
	{
		vector<sortkey> remap;

		texs.setsizenodelete(0);
		enumeratekt(indices, sortkey, k, sortval, t,
            loopl(6) if(t.dims[l].length() && t.unlit<=0)
			{
				if(k.lmid>=LMID_RESERVED && lightmaps[k.lmid-LMID_RESERVED].unlitx>=0)
				{
					sortkey ukey(k.tex, LMID_AMBIENT, k.envmap);
					sortval *uval = indices.access(ukey);
					if(uval && uval->unlit<=0)
					{
						if(uval->unlit<0) texs.removeobj(ukey);
						else remap.add(ukey);
						uval->unlit = k.lmid;
					}
				}
				else if(k.lmid==LMID_AMBIENT)
				{
					remap.add(k);
					t.unlit = -1;
				}
				texs.add(k);
				break;
			}
		);
		texs.sort(texsort);

		remapunlit(remap);

		matsurfs.setsize(optimizematsurfs(matsurfs.getbuf(), matsurfs.length()));
	}

	static int texsort(const sortkey *x, const sortkey *y)
	{
		if(x->tex == y->tex) return 0;
		Slot &xs = lookuptexture(x->tex, false), &ys = lookuptexture(y->tex, false);
		if(xs.shader < ys.shader) return -1;
		if(xs.shader > ys.shader) return 1;
		if(xs.params.length() < ys.params.length()) return -1;
		if(xs.params.length() > ys.params.length()) return 1;
		if(x->tex < y->tex) return -1;
		else return 1;
	}

	char *setup(vtxarray *va, lodlevel &lod, char *buf)
	{
		lod.eslist = (elementset *)buf;

		lod.sky = skyindices.length();
		lod.explicitsky = explicitskyindices.length();

		if(!hasVBO)
		{
			lod.ebufGL = lod.skybufGL = 0;
			lod.ebuf = (ushort *)(lod.eslist + texs.length());
			lod.skybuf = lod.ebuf + 3*curtris;
			lod.matbuf = (materialsurface *)(lod.skybuf+lod.sky+lod.explicitsky);
		}

		ushort *skybuf = NULL;
		if(lod.sky+lod.explicitsky)
		{
			skybuf = hasVBO ? new ushort[lod.sky+lod.explicitsky] : lod.skybuf;
			memcpy(skybuf, skyindices.getbuf(), lod.sky*sizeof(ushort));
			memcpy(skybuf+lod.sky, explicitskyindices.getbuf(), lod.explicitsky*sizeof(ushort));
		}
 
		if(hasVBO)
		{
			lod.ebuf = lod.skybuf = 0;
			lod.matbuf = (materialsurface *)(lod.eslist + texs.length());

			if(skybuf)
			{
				if(offsetindices) loopi(lod.sky+lod.explicitsky) skybuf[i] += offsetindices;
				lod.skybuf = (ushort *)addvbo(va, &lod==&va->l0 ? VBO_SKYBUF_L0 : VBO_SKYBUF_L1, skybuf, (lod.sky+lod.explicitsky)*sizeof(ushort));
				delete[] skybuf;
			}
			else lod.skybufGL = 0;
		}

		lod.matsurfs = matsurfs.length();
		if(lod.matsurfs) memcpy(lod.matbuf, matsurfs.getbuf(), matsurfs.length()*sizeof(materialsurface));

		if(texs.length())
		{
			ushort *ebuf = hasVBO ? new ushort[3*curtris] : lod.ebuf, *curbuf = ebuf;
			loopv(texs)
			{
				const sortkey &k = texs[i];
				const sortval &t = indices[k];
				elementset &e = lod.eslist[i];
				e.texture = k.tex;
				e.lmid = t.unlit>0 ? t.unlit : k.lmid;
				e.envmap = k.envmap;
                ushort *startbuf = curbuf;
                loopl(6) 
                {
                    if(t.dims[l].length())
				{
					memcpy(curbuf, t.dims[l].getbuf(), t.dims[l].length() * sizeof(ushort));

					e.minvert[l] = USHRT_MAX;
					e.maxvert[l] = 0;
					loopvj(t.dims[l])
					{
						curbuf[j] += offsetindices;
						e.minvert[l] = min(e.minvert[l], curbuf[j]);
						e.maxvert[l] = max(e.maxvert[l], curbuf[j]);
					}

					curbuf += t.dims[l].length();
				}
                    e.length[l] = curbuf-startbuf;
                }
			}
			if(hasVBO)
			{
				lod.ebuf = (ushort *)addvbo(va, &lod==&va->l0 ? VBO_EBUF_L0 : VBO_EBUF_L1, ebuf, 3*curtris*sizeof(ushort));
				delete[] ebuf;
			}
		}
		else if(hasVBO) lod.ebufGL = 0;
		lod.texs = texs.length();
		lod.tris = curtris;
		return (char *)(lod.matbuf+lod.matsurfs);
	}
} l0, l1;

int explicitsky = 0;
double skyarea = 0;

#ifdef BFRONTIER
VARW(lodsize, 0, 32, 128);
VARW(loddistance, 0, 2000, 100000);
#else
VARF(lodsize, 0, 32, 128, hdr.mapwlod = lodsize);
VAR(loddistance, 0, 2000, 100000);
#endif

int addtriindexes(usvector &v, int index[4], int mask = 3)
{
	int tris = 0;
    if(mask&1 && index[0]!=index[1] && index[0]!=index[2] && index[1]!=index[2])
	{
		tris++;
		v.add(index[0]);
		v.add(index[1]);
		v.add(index[2]);
	}
    if(mask&2 && index[0]!=index[2] && index[0]!=index[3] && index[2]!=index[3])
	{
		tris++;
		v.add(index[0]);
		v.add(index[2]);
		v.add(index[3]);
	}
	return tris;
}

vvec shadowmapmin, shadowmapmax;

int calcshadowmask(vvec *vv)
{
    extern vec shadowdir;
    int mask = 0, used = 0;
    if(vv[0]==vv[2]) return 0;
    if(vv[0]!=vv[1] && vv[1]!=vv[2])
    {
        vvec e1(vv[1]), e2(vv[2]);
        e1.sub(vv[0]);
        e2.sub(vv[0]);
        vec v;
        v.cross(e1.tovec(), e2.tovec());
        if(v.dot(shadowdir)>0) { mask |= 1; used |= 0x7; } 
    }
    if(vv[0]!=vv[3] && vv[2]!=vv[3])
    {
        vvec e1(vv[2]), e2(vv[3]);
        e1.sub(vv[0]);
        e2.sub(vv[0]);
        vec v;
        v.cross(e1.tovec(), e2.tovec());
        if(v.dot(shadowdir)>0) { mask |= 2; used |= 0xD; }
    }
    if(used) loopi(4) if(used&(1<<i))
    {
        const vvec &v = vv[i];
        loopk(3)
        {
            if(v[k]<shadowmapmin[k]) shadowmapmin[k] = v[k];
            if(v[k]>shadowmapmax[k]) shadowmapmax[k] = v[k];
        }
    }
    return mask;
}

void addcubeverts(int orient, int size, bool lodcube, vvec *vv, ushort texture, surfaceinfo *surface, surfacenormals *normals, ushort envmap = EMID_NONE)
{
	int index[4];
    int shadowmask = texture==DEFAULT_SKY || renderpath==R_FIXEDFUNCTION ? 0 : calcshadowmask(vv);
	loopk(4)
	{
		short u, v;
		if(!nolights && surface && surface->lmid >= LMID_RESERVED)
		{
			u = short((surface->x + (surface->texcoords[k*2] / 255.0f) * (surface->w - 1) + 0.5f) * SHRT_MAX/LM_PACKW);
			v = short((surface->y + (surface->texcoords[k*2 + 1] / 255.0f) * (surface->h - 1) + 0.5f) * SHRT_MAX/LM_PACKH);
		}
		else u = v = 0;
        index[k] = vh.access(vv[k], u, v, renderpath!=R_FIXEDFUNCTION && normals ? normals->normals[k] : bvec(128, 128, 128));
	}

	extern vector<GLuint> lmtexids;
    int dim = dimension(orient);
	sortkey key(texture, surface && lmtexids.inrange(surface->lmid) ? surface->lmid : LMID_AMBIENT, envmap);
	if(!lodcube)
	{
        if(texture == DEFAULT_SKY) explicitsky += addtriindexes(l0.explicitskyindices, index);
        else
        {
            sortval &val = l0.indices[key];
            l0.curtris += addtriindexes(val.dims[2*dim], index, shadowmask^3);
            if(shadowmask) l0.curtris += addtriindexes(val.dims[2*dim+1], index, shadowmask); 
        }
	}
	if(lodsize && size>=lodsize)
	{
        int tris = addtriindexes(texture == DEFAULT_SKY ? l1.explicitskyindices : l1.indices[key].dims[2*dim], index);
		if(texture != DEFAULT_SKY) l1.curtris += tris;
	}
}

void gencubeverts(cube &c, int x, int y, int z, int size, int csi, bool lodcube)
{
	freeclipplanes(c);						  // physics planes based on rendering

	loopi(6) if(visibleface(c, i, x, y, z, size, MAT_AIR, lodcube))
	{
		cubeext &e = ext(c);

		// this is necessary for physics to work, even if the face is merged
		if(touchingface(c, i)) e.visible |= 1<<i;

		if(!lodcube && e.merged&(1<<i)) continue;

		cstats[csi].nface++;

		vvec vv[4];
		loopk(4) calcvert(c, x, y, z, size, vv[k], faceverts(c, i, k));
		ushort envmap = EMID_NONE;
		Slot &slot = lookuptexture(c.texture[i], false);
		if(slot.shader && slot.shader->type&SHADER_ENVMAP)
		{
			loopv(slot.sts) if(slot.sts[i].type==TEX_ENVMAP) { envmap = EMID_CUSTOM; break; }
			if(envmap==EMID_NONE) envmap = closestenvmap(i, x, y, z, size); 
		}
		addcubeverts(i, size, lodcube, vv, c.texture[i], e.surfaces ? &e.surfaces[i] : NULL, e.normals ? &e.normals[i] : NULL, envmap);
		if(!lodcube && slot.autograss && i!=O_BOTTOM) 
		{
			grasstri &g = grasstris.add();
			memcpy(g.v, vv, sizeof(vv));
			g.surface = e.surfaces ? &e.surfaces[i] : NULL;
			g.texture = c.texture[i];
		}
	}
	else if(touchingface(c, i) && visibleface(c, i, x, y, z, size, MAT_AIR, MAT_NOCLIP, lodcube)) ext(c).visible |= 1<<i;
}

bool skyoccluded(cube &c, int orient)
{
	if(isempty(c)) return false;
//	if(c.texture[orient] == DEFAULT_SKY) return true;
	if(touchingface(c, orient) && faceedges(c, orient) == F_SOLID) return true;
	return false;
}

int hasskyfaces(cube &c, int x, int y, int z, int size, int faces[6])
{
	int numfaces = 0;
	if(x == 0 && !skyoccluded(c, O_LEFT)) faces[numfaces++] = O_LEFT;
	if(x + size == hdr.worldsize && !skyoccluded(c, O_RIGHT)) faces[numfaces++] = O_RIGHT;
	if(y == 0 && !skyoccluded(c, O_BACK)) faces[numfaces++] = O_BACK;
	if(y + size == hdr.worldsize && !skyoccluded(c, O_FRONT)) faces[numfaces++] = O_FRONT;
	if(z == 0 && !skyoccluded(c, O_BOTTOM)) faces[numfaces++] = O_BOTTOM;
	if(z + size == hdr.worldsize && !skyoccluded(c, O_TOP)) faces[numfaces++] = O_TOP;
	return numfaces;
}

vector<cubeface> skyfaces[6][2];
 
void minskyface(cube &cu, int orient, const ivec &co, int size, mergeinfo &orig)
{	
	mergeinfo mincf;
	mincf.u1 = orig.u2;
	mincf.u2 = orig.u1;
	mincf.v1 = orig.v2;
	mincf.v2 = orig.v1;
	mincubeface(cu, orient, co, size, orig, mincf);
	orig.u1 = max(mincf.u1, orig.u1);
	orig.u2 = min(mincf.u2, orig.u2);
	orig.v1 = max(mincf.v1, orig.v1);
	orig.v2 = min(mincf.v2, orig.v2);
}  

void genskyfaces(cube &c, const ivec &o, int size, bool lodcube)
{
	if(isentirelysolid(c)) return;

	int faces[6],
		numfaces = hasskyfaces(c, o.x, o.y, o.z, size, faces);
	if(!numfaces) return;

	loopi(numfaces)
	{
		int orient = faces[i], dim = dimension(orient);
		cubeface m;
		m.c = NULL;
		m.u1 = (o[C[dim]]&VVEC_INT_MASK)<<VVEC_FRAC; 
		m.u2 = ((o[C[dim]]&VVEC_INT_MASK)+size)<<VVEC_FRAC;
		m.v1 = (o[R[dim]]&VVEC_INT_MASK)<<VVEC_FRAC;
		m.v2 = ((o[R[dim]]&VVEC_INT_MASK)+size)<<VVEC_FRAC;
		minskyface(c, orient, o, size, m);
		if(m.u1 >= m.u2 || m.v1 >= m.v2) continue;
		if(!lodcube) 
		{
			skyarea += (int(m.u2-m.u1)*int(m.v2-m.v1) + (1<<(2*VVEC_FRAC))-1)>>(2*VVEC_FRAC);
			skyfaces[orient][0].add(m);
		}
		if(lodsize && size>=lodsize) skyfaces[orient][1].add(m);
	}
}

void addskyverts(const ivec &o, int size)
{
	loopi(6)
	{
		int dim = dimension(i), c = C[dim], r = R[dim];
		loopl(2)
		{
			vector<cubeface> &sf = skyfaces[i][l]; 
			if(sf.empty()) continue;
			sf.setsizenodelete(mergefaces(i, sf.getbuf(), sf.length()));
			loopvj(sf)
			{
				mergeinfo &m = sf[j];
				int index[4];
				loopk(4)
				{
					const ivec &coords = cubecoords[fv[i][3-k]];
					vvec vv;
					vv[dim] = (o[dim]&VVEC_INT_MASK)<<VVEC_FRAC;
					if(coords[dim]) vv[dim] += size<<VVEC_FRAC;
					vv[c] = coords[c] ? m.u2 : m.u1;
					vv[r] = coords[r] ? m.v2 : m.v1;
                    index[k] = vh.access(vv, 0, 0, bvec(128, 128, 128));
				}
				addtriindexes((!l ? l0 : l1).skyindices, index);
			}
			sf.setsizenodelete(0);
		}
	}
}
					
////////// Vertex Arrays //////////////

int allocva = 0;
int wtris = 0, wverts = 0, vtris = 0, vverts = 0, glde = 0;
vector<vtxarray *> valist, varoot;

vtxarray *newva(int x, int y, int z, int size)
{
	l0.optimize();
	l1.optimize();
	int allocsize = sizeof(vtxarray) + l0.size() + l1.size();
	int bufsize = verts.length()*VTXSIZE;
	if(!hasVBO) allocsize += bufsize; // length of vertex buffer

	vtxarray *va = (vtxarray *)new uchar[allocsize];
    va->parent = NULL;
    va->children = new vector<vtxarray *>;
    va->allocsize = allocsize;
    va->x = x; va->y = y; va->z = z; va->size = size;
    va->explicitsky = explicitsky;
    va->skyarea = skyarea;
    va->curvfc = VFC_NOT_VISIBLE;
    va->occluded = OCCLUDE_NOTHING;
    va->query = NULL;
    va->mapmodels = NULL;
    if(grasstris.empty()) va->grasstris = NULL;
    else
    {
        va->grasstris = new vector<grasstri>;
        va->grasstris->move(grasstris);
    }
    va->grasssamples = NULL;
    va->hasmerges = 0;

    va->verts = verts.length();
	va->vbufGL = 0;
	va->vbuf = 0; // Offset in VBO, or just null
	va->minvert = 0;
	va->maxvert = verts.length()-1;
	if(hasVBO && verts.length())
	{
		void *vbuf;
		if(VTXSIZE!=sizeof(vertex))
		{
			char *f = new char[bufsize];
			genverts(f);
			vbuf = (vertex *)addvbo(va, VBO_VBUF, f, bufsize); 
			delete[] f;
		}
		else vbuf = (vertex *)addvbo(va, VBO_VBUF, verts.getbuf(), bufsize);
		int offset = int(size_t(vbuf)) / VTXSIZE;
		l0.offsetindices = offset;
		l1.offsetindices = offset;
		va->minvert += offset;
		va->maxvert += offset;
	}
	char *buf = l1.setup(va, va->l1, l0.setup(va, va->l0, (char *)(va+1)));
	if(!hasVBO)
	{
		va->vbuf = (vertex *)buf;
		if(VTXSIZE!=sizeof(vertex)) genverts(buf);
		else memcpy(va->vbuf, verts.getbuf(), bufsize);
	}

    wverts += va->verts;
	wtris  += va->l0.tris;
	allocva++;
	valist.add(va);
	return va;
}

void destroyva(vtxarray *va, bool reparent)
{
	if(va->vbufGL) destroyvbo(va->vbufGL);
	if(va->l0.ebufGL) destroyvbo(va->l0.ebufGL);
	if(va->l0.skybufGL) destroyvbo(va->l0.skybufGL);
	if(va->l1.ebufGL) destroyvbo(va->l1.ebufGL);
	if(va->l1.skybufGL) destroyvbo(va->l1.skybufGL);
	wverts -= va->verts;
	wtris -= va->l0.tris;
	allocva--;
	valist.removeobj(va);
	if(!va->parent) varoot.removeobj(va);
	if(reparent)
	{
		if(va->parent) va->parent->children->removeobj(va);
		loopv(*va->children)
		{
			vtxarray *child = (*va->children)[i];
			child->parent = va->parent;
			if(child->parent) child->parent->children->add(va);
		}
	}
	if(va->mapmodels) delete va->mapmodels;
	if(va->children) delete va->children;
	if(va->grasstris) delete va->grasstris;
	if(va->grasssamples) delete va->grasssamples;
	delete[] (uchar *)va;
}

void vaclearc(cube *c)
{
	loopi(8)
	{
		if(c[i].ext)
		{
			if(c[i].ext->va) destroyva(c[i].ext->va, false);
			c[i].ext->va = NULL;
		}
		if(c[i].children) vaclearc(c[i].children);
	}
}

static vector<octaentities *> vamms;

struct mergedface
{	
	uchar orient;
	ushort tex, envmap;
	vvec v[4];
	surfaceinfo *surface;
	surfacenormals *normals;
};  

static int vahasmerges = 0, vamergemax = 0;
static vector<mergedface> vamerges[VVEC_INT];

void genmergedfaces(cube &c, const ivec &co, int size, int minlevel = 0)
{
    if(!c.ext || !c.ext->merges || isempty(c)) return;
	int index = 0;
	loopi(6) if(c.ext->mergeorigin & (1<<i))
	{
		mergeinfo &m = c.ext->merges[index++];
		if(m.u1>=m.u2 || m.v1>=m.v2) continue;
		mergedface mf;
		mf.orient = i;
		mf.tex = c.texture[i];
		mf.envmap = EMID_NONE;
		Slot &slot = lookuptexture(mf.tex, false);
		if(slot.shader && slot.shader->type&SHADER_ENVMAP)
		{
			loopv(slot.sts) if(slot.sts[i].type==TEX_ENVMAP) { mf.envmap = EMID_CUSTOM; break; }
			if(mf.envmap==EMID_NONE) mf.envmap = closestenvmap(i, co.x, co.y, co.z, size);
		}
		mf.surface = c.ext->surfaces ? &c.ext->surfaces[i] : NULL;
		mf.normals = c.ext->normals ? &c.ext->normals[i] : NULL;
		genmergedverts(c, i, co, size, m, mf.v);
		int level = calcmergedsize(i, co, size, m, mf.v);
		if(level > minlevel)
		{
			vamerges[level].add(mf);
			vamergemax = max(vamergemax, level);
			vahasmerges |= MERGE_ORIGIN;
		}
	}
}

void findmergedfaces(cube &c, const ivec &co, int size, int csi, int minlevel)
{
	if(c.ext && c.ext->va && !(c.ext->va->hasmerges&MERGE_ORIGIN)) return;
	if(c.children)
	{
		loopi(8)
		{
			ivec o(i, co.x, co.y, co.z, size/2); 
			findmergedfaces(c.children[i], o, size/2, csi-1, minlevel);
		}
	}
	else if(c.ext && c.ext->merges) genmergedfaces(c, co, size, minlevel);
}

void addmergedverts(int level)
{
	vector<mergedface> &mfl = vamerges[level];
	if(mfl.empty()) return;
	loopv(mfl)
	{
		mergedface &mf = mfl[i];
		addcubeverts(mf.orient, 1<<level, false, mf.v, mf.tex, mf.surface, mf.normals, mf.envmap);
		Slot &slot = lookuptexture(mf.tex, false);
		if(slot.autograss && mf.orient!=O_BOTTOM)
		{
			grasstri &g = grasstris.add();
			memcpy(g.v, mf.v, sizeof(mf.v));
			g.surface = mf.surface;
			g.texture = mf.tex;
		}
		cstats[level].nface++;
		vahasmerges |= MERGE_USE;
	}
	mfl.setsizenodelete(0);
}

void rendercube(cube &c, int cx, int cy, int cz, int size, int csi)  // creates vertices and indices ready to be put into a va
{
	//if(size<=16) return;
	if(c.ext && c.ext->va) return;							// don't re-render
	cstats[csi].size = size;
	bool lodcube = false;

	if(c.children)
	{
		cstats[csi].nnode++;

		loopi(8)
		{
			ivec o(i, cx, cy, cz, size/2);
			rendercube(c.children[i], o.x, o.y, o.z, size/2, csi-1);
		}

		if(csi < VVEC_INT && vamerges[csi].length()) addmergedverts(csi);

		if(size!=lodsize)
		{
			if(c.ext)
			{
				if(c.ext->ents && c.ext->ents->mapmodels.length()) vamms.add(c.ext->ents);
			}
			return;
		}
		lodcube = true;
	}
	if(!c.children || lodcube) genskyfaces(c, ivec(cx, cy, cz), size, lodcube);

	if(!isempty(c)) gencubeverts(c, cx, cy, cz, size, csi, lodcube);

	if(lodcube) return;

	if(c.ext)
	{
		if(c.ext->ents && c.ext->ents->mapmodels.length()) vamms.add(c.ext->ents);
		if(c.ext->material != MAT_AIR) genmatsurfs(c, cx, cy, cz, size, l0.matsurfs);
		if(c.ext->merges) genmergedfaces(c, ivec(cx, cy, cz), size);
		if(c.ext->merged & ~c.ext->mergeorigin) vahasmerges |= MERGE_PART;
	}

	if(csi < VVEC_INT && vamerges[csi].length()) addmergedverts(csi);

	cstats[csi].nleaf++;
}

void calcvabb(int cx, int cy, int cz, int size, ivec &bbmin, ivec &bbmax)
{
	vvec vmin(cx+size, cy+size, cz+size), vmax(cx, cy, cz);

	loopv(verts)
	{
		vvec &v = verts[i];
		loopj(3)
		{
			if(v[j]<vmin[j]) vmin[j] = v[j];
			if(v[j]>vmax[j]) vmax[j] = v[j];
		}
	}

	bbmin = vmin.toivec(cx, cy, cz);
	loopi(3) vmax[i] += (1<<VVEC_FRAC)-1;
	bbmax = vmax.toivec(cx, cy, cz);
}

void setva(cube &c, int cx, int cy, int cz, int size, int csi)
{
	ASSERT(size <= VVEC_INT_MASK+1);

	explicitsky = 0;
	skyarea = 0;
    shadowmapmin = vvec(cx+size, cy+size, cz+size);
    shadowmapmax = vvec(cx, cy, cz);

	rendercube(c, cx, cy, cz, size, csi);

	ivec bbmin, bbmax;

	calcvabb(cx, cy, cz, size, bbmin, bbmax);

	addskyverts(ivec(cx, cy, cz), size);

	vtxarray *va = newva(cx, cy, cz, size);
	ext(c).va = va;
	va->min = bbmin;
	va->max = bbmax;
    va->shadowmapmin = shadowmapmin.toivec(va->x, va->y, va->z);
    loopk(3) shadowmapmax[k] += (1<<VVEC_FRAC)-1;
    va->shadowmapmax = shadowmapmax.toivec(va->x, va->y, va->z); 
	if(vamms.length()) va->mapmodels = new vector<octaentities *>(vamms);
	va->hasmerges = vahasmerges;

	verts.setsizenodelete(0);
	vamms.setsizenodelete(0);
	grasstris.setsizenodelete(0);
	explicitsky = 0;
	skyarea = 0;
	vh.clear(); 
	l0.clear();
	l1.clear();
}

VARF(vacubemax, 64, 2048, 256*256, allchanged());
VARF(vacubesize, 128, 128, VVEC_INT_MASK+1, allchanged());
VARF(vacubemin, 0, 128, 256*256, allchanged());

int recalcprogress = 0;
#define progress(s)	 if((recalcprogress++&0x7FF)==0) show_out_of_renderloop_progress(recalcprogress/(float)allocnodes, s);

int updateva(cube *c, int cx, int cy, int cz, int size, int csi)
{
	progress("recalculating geometry...");
	static int faces[6];
	int ccount = 0, cmergemax = vamergemax, chasmerges = vahasmerges;
	loopi(8)									// counting number of semi-solid/solid children cubes
	{
		int count = 0, childpos = varoot.length();
		ivec o(i, cx, cy, cz, size);
		vamergemax = 0;
		vahasmerges = 0;
		if(c[i].ext && c[i].ext->va) 
		{
			//count += vacubemax+1;		// since must already have more then max cubes
			varoot.add(c[i].ext->va);
			if(c[i].ext->va->hasmerges&MERGE_ORIGIN) findmergedfaces(c[i], o, size, csi, csi);
		}
		else if(c[i].children) count += updateva(c[i].children, o.x, o.y, o.z, size/2, csi-1);
		else if(!isempty(c[i]) || hasskyfaces(c[i], o.x, o.y, o.z, size, faces)) count++;
		int tcount = count + (csi < VVEC_INT ? vamerges[csi].length() : 0);
		if(tcount > vacubemax || (tcount >= vacubemin && size == vacubesize) || (tcount && size == min(VVEC_INT_MASK+1, hdr.worldsize/2))) 
		{
			setva(c[i], o.x, o.y, o.z, size, csi);
			if(c[i].ext && c[i].ext->va)
			{
				while(varoot.length() > childpos)
				{
					vtxarray *child = varoot.pop();
					c[i].ext->va->children->add(child);
					child->parent = c[i].ext->va;
				}
				varoot.add(c[i].ext->va);
				if(vamergemax > size)
				{
					cmergemax = max(cmergemax, vamergemax);
					chasmerges |= vahasmerges&~MERGE_USE;
				}
				continue;
			}
		}
		if(csi < VVEC_INT-1 && vamerges[csi].length()) vamerges[csi+1].move(vamerges[csi]);
		cmergemax = max(cmergemax, vamergemax);
		chasmerges |= vahasmerges;
		ccount += count;
	}

	vamergemax = cmergemax;
	vahasmerges = chasmerges;

	return ccount;
}

void genlod(cube &c, int size)
{
	if(!c.children || (c.ext && c.ext->va)) return;
	progress("generating LOD...");

	loopi(8) genlod(c.children[i], size/2);

	if(size>lodsize) return;

	if(c.ext) c.ext->material = MAT_AIR;

	loopi(8) if(!isempty(c.children[i]))
	{
		forcemip(c);
		return;
	}

	emptyfaces(c);
}

void octarender()								// creates va s for all leaf cubes that don't already have them
{
	recalcprogress = 0;
	if(lodsize) loopi(8) genlod(worldroot[i], hdr.worldsize/2);

	int csi = 0;
	while(1<<csi < hdr.worldsize) csi++;

	recalcprogress = 0;
	varoot.setsizenodelete(0);
	updateva(worldroot, 0, 0, 0, hdr.worldsize/2, csi-1);
	flushvbo();

	explicitsky = 0;
	skyarea = 0;
	loopv(valist)
	{
		vtxarray *va = valist[i];
		explicitsky += va->explicitsky;
		skyarea += va->skyarea;
	}
}

void precachetextures(lodlevel &lod) { loopi(lod.texs) lookuptexture(lod.eslist[i].texture); }
void precacheall() { loopv(valist) { precachetextures(valist[i]->l0); precachetextures(valist[i]->l1); } ; }

void allchanged(bool load)
{
	show_out_of_renderloop_progress(0, "clearing VBOs...");
	vaclearc(worldroot);
	memset(cstats, 0, sizeof(cstat)*32);
	resetqueries();
	if(load) initenvmaps();
	octarender();
	if(load) precacheall();
	setupmaterials();
	invalidatereflections();
	if(load) 
	{
		entitiesinoctanodes();
		genenvmaps();
	}
	printcstats();
}

void recalc()
{
	allchanged(true);
}

COMMAND(recalc, "");

