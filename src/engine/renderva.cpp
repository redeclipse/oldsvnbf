// renderva.cpp: handles the occlusion and rendering of vertex arrays

#include "pch.h"
#include "engine.h"

static inline void drawtris(GLsizei numindices, const GLvoid *indices, ushort minvert, ushort maxvert)
{
    if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, minvert, maxvert, numindices, GL_UNSIGNED_SHORT, indices);
    else glDrawElements(GL_TRIANGLES, numindices, GL_UNSIGNED_SHORT, indices);
    glde++;
}

static inline void drawvatris(vtxarray *va, GLsizei numindices, const GLvoid *indices)
{
    drawtris(numindices, indices, va->minvert, va->maxvert);
}

///////// view frustrum culling ///////////////////////

plane vfcP[5];  // perpindictular vectors to view frustrum bounding planes
float vfcDfog;  // far plane culling distance (fog limit).
float vfcDnear[5], vfcDfar[5];
int vfcw, vfch;
float vfcfov;

vtxarray *visibleva;

int isvisiblesphere(float rad, const vec &cv)
{
	int v = VFC_FULL_VISIBLE;
	float dist;

	loopi(5)
	{
		dist = vfcP[i].dist(cv);
		if(dist < -rad) return VFC_NOT_VISIBLE;
		if(dist < rad) v = VFC_PART_VISIBLE;
	}

	dist -= vfcDfog;
	if(dist > rad) return VFC_FOGGED;  //VFC_NOT_VISIBLE;	// culling when fog is closer than size of world results in HOM
	if(dist > -rad) v = VFC_PART_VISIBLE;

	return v;
}

int isvisiblecube(const ivec &o, int size)
{
    int v = VFC_FULL_VISIBLE;
    float dist;

    loopi(5)
    {
        dist = o.dist(vfcP[i]);
        if(dist < -vfcDfar[i]*size) return VFC_NOT_VISIBLE;
        if(dist < -vfcDnear[i]*size) v = VFC_PART_VISIBLE;
    }

    dist -= vfcDfog;
    if(dist > -vfcDnear[4]*size) return VFC_FOGGED;
    if(dist > -vfcDfar[4]*size) v = VFC_PART_VISIBLE;

    return v;
}

float vadist(vtxarray *va, const vec &p)
{
    return p.dist_to_bb(va->bbmin, va->bbmax);
}

#define VASORTSIZE 64

static vtxarray *vasort[VASORTSIZE];

void addvisibleva(vtxarray *va)
{
	float dist = vadist(va, camera1->o);
	va->distance = int(dist); /*cv.dist(camera1->o) - va->size*SQRT3/2*/

	int hash = min(int(dist*VASORTSIZE/hdr.worldsize), VASORTSIZE-1);
	vtxarray **prev = &vasort[hash], *cur = vasort[hash];

    while(cur && va->distance >= cur->distance)
	{
		prev = &cur->next;
		cur = cur->next;
	}

	va->next = *prev;
	*prev = va;
}

void sortvisiblevas()
{
	visibleva = NULL;
	vtxarray **last = &visibleva;
	loopi(VASORTSIZE) if(vasort[i])
	{
		vtxarray *va = vasort[i];
		*last = va;
		while(va->next) va = va->next;
		last = &va->next;
	}
}

void findvisiblevas(vector<vtxarray *> &vas, bool resetocclude = false)
{
    loopv(vas)
    {
        vtxarray &v = *vas[i];
        int prevvfc = resetocclude ? VFC_NOT_VISIBLE : v.curvfc;
        v.curvfc = isvisiblecube(v.o, v.size);
        if(v.curvfc!=VFC_NOT_VISIBLE)
        {
            if(pvsoccluded(v.o, v.size))
            {
                v.curvfc = VFC_NOT_VISIBLE;
                continue;
            }
            addvisibleva(&v);
            if(v.children.length()) findvisiblevas(v.children, prevvfc==VFC_NOT_VISIBLE);
            if(prevvfc==VFC_NOT_VISIBLE)
            {
                v.occluded = !v.texs || pvsoccluded(v.geommin, v.geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
                v.query = NULL;
            }
        }
    }
}

void calcvfcD()
{
    loopi(5)
    {
        plane &p = vfcP[i];
        vfcDnear[i] = vfcDfar[i] = 0;
        loopk(3) if(p[k] > 0) vfcDfar[i] += p[k];
        else vfcDnear[i] += p[k];
    }
} 

void setvfcP(float yaw, float pitch, const vec &camera)
{
	float yawd = (90.0f - vfcfov/2.0f) * RAD;
	float pitchd = (90.0f - (vfcfov*float(vfch)/float(vfcw))/2.0f) * RAD;
	yaw *= RAD;
	pitch *= RAD;
	vfcP[0].toplane(vec(yaw + yawd, pitch), camera);	// left plane
	vfcP[1].toplane(vec(yaw - yawd, pitch), camera);	// right plane
	vfcP[2].toplane(vec(yaw, pitch + pitchd), camera); // top plane
	vfcP[3].toplane(vec(yaw, pitch - pitchd), camera); // bottom plane
	vfcP[4].toplane(vec(yaw, pitch), camera);		  // near/far planes
	extern int fog;
	vfcDfog = fog;
    calcvfcD();
}

plane oldvfcP[5];

void reflectvfcP(float z)
{
	memcpy(oldvfcP, vfcP, sizeof(vfcP));

	vec o(camera1->o);
	o.z = z-(camera1->o.z-z);
	setvfcP(camera1->yaw, -camera1->pitch, o);
}

void restorevfcP()
{
	memcpy(vfcP, oldvfcP, sizeof(vfcP));
    calcvfcD();
}

extern vector<vtxarray *> varoot;

void visiblecubes(cube *c, int size, int cx, int cy, int cz, int w, int h, float fov)
{
	memset(vasort, 0, sizeof(vasort));

	vfcw = w;
	vfch = h;
	vfcfov = fov;

	// Calculate view frustrum: Only changes if resize, but...
	setvfcP(camera1->yaw, camera1->pitch, camera1->o);

	findvisiblevas(varoot);
	sortvisiblevas();
}

static inline bool insideva(const vtxarray *va, const vec &v, int margin = 1)
{
    int size = va->size + margin;
    return v.x>=va->o.x-margin && v.y>=va->o.y-margin && v.z>=va->o.z-margin &&
           v.x<=va->o.x+size && v.y<=va->o.y+size && v.z<=va->o.z+size;
}

static ivec vaorigin;

static void resetorigin()
{
	vaorigin = ivec(-1, -1, -1);
}

static bool setorigin(vtxarray *va, bool shadowmatrix = false)
{
    ivec o(va->o);
	o.mask(~VVEC_INT_MASK);
	if(o != vaorigin)
	{
		vaorigin = o;
		glPopMatrix();
		glPushMatrix();
		glTranslatef(o.x, o.y, o.z);
		static const float scale = 1.0f/(1<<VVEC_FRAC);
		glScalef(scale, scale, scale);

        if(shadowmatrix) adjustshadowmatrix(o, scale);
        return true;
	}
    return false;
}

///////// occlusion queries /////////////

#define MAXQUERY 2048

struct queryframe
{
	int cur, max;
	occludequery queries[MAXQUERY];
};

static queryframe queryframes[2] = {{0, 0}, {0, 0}};
static uint flipquery = 0;

int getnumqueries()
{
	return queryframes[flipquery].cur;
}

void flipqueries()
{
	flipquery = (flipquery + 1) % 2;
	queryframe &qf = queryframes[flipquery];
	loopi(qf.cur) qf.queries[i].owner = NULL;
	qf.cur = 0;
}

occludequery *newquery(void *owner)
{
	queryframe &qf = queryframes[flipquery];
	if(qf.cur >= qf.max)
	{
		if(qf.max >= MAXQUERY) return NULL;
		glGenQueries_(1, &qf.queries[qf.max++].id);
	}
	occludequery *query = &qf.queries[qf.cur++];
	query->owner = owner;
	query->fragments = -1;
	return query;
}

void resetqueries()
{
	loopi(2) loopj(queryframes[i].max) queryframes[i].queries[j].owner = NULL;
}

void clearqueries()
{
    loopi(2)
    {
        queryframe &qf = queryframes[i];
        loopj(qf.max)
        {
            glDeleteQueries_(1, &qf.queries[j].id);
            qf.queries[j].owner = NULL;
        }
        qf.cur = qf.max = 0;
    }
}

VAR(oqfrags, 0, 8, 64);
VAR(oqreflect, 0, 4, 64);

bool checkquery(occludequery *query, bool nowait)
{
	GLuint fragments;
	if(query->fragments >= 0) fragments = query->fragments;
	else
	{
		if(nowait)
		{
			GLint avail;
			glGetQueryObjectiv_(query->id, GL_QUERY_RESULT_AVAILABLE, &avail);
			if(!avail) return false;
		}
		glGetQueryObjectuiv_(query->id, GL_QUERY_RESULT_ARB, &fragments);
		query->fragments = fragments;
	}
	return fragments < (uint)(reflecting ? oqreflect : oqfrags);
}

void drawbb(const ivec &bo, const ivec &br, const vec &camera)
{
	glBegin(GL_QUADS);

	loopi(6)
	{
		int dim = dimension(i), coord = dimcoord(i);

		if(coord)
		{
			if(camera[dim] < bo[dim] + br[dim]) continue;
		}
		else if(camera[dim] > bo[dim]) continue;

		loopj(4)
		{
			const ivec &cc = cubecoords[fv[i][j]];
			glVertex3i(cc.x ? bo.x+br.x : bo.x,
						cc.y ? bo.y+br.y : bo.y,
						cc.z ? bo.z+br.z : bo.z);
		}

		xtraverts += 4;
	}

	glEnd();
}

extern int octaentsize;

static octaentities *visiblemms, **lastvisiblemms;

void findvisiblemms(const vector<extentity *> &ents)
{
	for(vtxarray *va = visibleva; va; va = va->next)
	{
		if(!va->mapmodels || va->curvfc >= VFC_FOGGED || va->occluded >= OCCLUDE_BB) continue;
		loopv(*va->mapmodels)
		{
			octaentities *oe = (*va->mapmodels)[i];
            if(isvisiblecube(oe->o, oe->size) >= VFC_FOGGED || pvsoccluded(oe->bbmin, ivec(oe->bbmax).sub(oe->bbmin))) continue;

			bool occluded = oe->query && oe->query->owner == oe && checkquery(oe->query);
			if(occluded)
			{
				oe->distance = -1;

				oe->next = NULL;
				*lastvisiblemms = oe;
				lastvisiblemms = &oe->next;
			}
			else
			{
				int visible = 0;
				loopv(oe->mapmodels)
				{
					extentity &e = *ents[oe->mapmodels[i]];
                    if(e.visible) continue;
                    e.visible = true;
					++visible;
				}
				if(!visible) continue;

				oe->distance = int(camera1->o.dist_to_bb(oe->o, oe->size));

				octaentities **prev = &visiblemms, *cur = visiblemms;
				while(cur && cur->distance >= 0 && oe->distance > cur->distance)
				{
					prev = &cur->next;
					cur = cur->next;
				}

				if(*prev == NULL) lastvisiblemms = &oe->next;
				oe->next = *prev;
				*prev = oe;
			}
		}
	}
}

VAR(oqmm, 0, 4, 8);

extern bool getentboundingbox(extentity &e, ivec &o, ivec &r);

void rendermapmodel(extentity &e)
{
	int anim = ANIM_MAPMODEL|ANIM_LOOP, basetime = 0;
	mapmodelinfo &mmi = getmminfo(e.attr2);
	if(&mmi) rendermodel(&e.light, mmi.name, anim, e.o, (float)((e.attr1+7)-(e.attr1+7)%15), 0, 0, MDL_CULL_VFC | MDL_CULL_DIST | MDL_DYNLIGHT, NULL, NULL, basetime);
}

extern int reflectdist;

static vector<octaentities *> renderedmms;

vtxarray *reflectedva;

void renderreflectedmapmodels(float z, bool refract)
{
	bool reflected = !refract && camera1->o.z >= z;
	vector<octaentities *> reflectedmms;
	vector<octaentities *> &mms = reflected ? reflectedmms : renderedmms;
	const vector<extentity *> &ents = et->getents();

	if(reflected)
	{
		reflectvfcP(z);
		for(vtxarray *va = reflectedva; va; va = va->rnext)
		{
			if(!va->mapmodels || va->distance > reflectdist) continue;
			loopv(*va->mapmodels) reflectedmms.add((*va->mapmodels)[i]);
		}
	}
	loopv(mms)
	{
		octaentities *oe = mms[i];
        if(refract ? oe->bbmin.z >= z : oe->bbmax.z <= z) continue;
        if(reflected && isvisiblecube(oe->o, oe->size) >= VFC_FOGGED) continue;
        loopv(oe->mapmodels)
        {
           extentity &e = *ents[oe->mapmodels[i]];
           if(e.visible) continue;
           e.visible = true;
        }
	}
	if(mms.length())
	{
		startmodelbatches();
		loopv(mms)
		{
			octaentities *oe = mms[i];
			loopv(oe->mapmodels)
			{
				extentity &e = *ents[oe->mapmodels[i]];
                if(!e.visible) continue;
				rendermapmodel(e);
                e.visible = false;
			}
		}
		endmodelbatches();
	}
	if(reflected) restorevfcP();
}

void rendermapmodels()
{
	const vector<extentity *> &ents = et->getents();

	visiblemms = NULL;
	lastvisiblemms = &visiblemms;
	findvisiblemms(ents);

	static int skipoq = 0;
	bool doquery = hasOQ && oqfrags && oqmm;

	renderedmms.setsizenodelete(0);
	startmodelbatches();
	for(octaentities *oe = visiblemms; oe; oe = oe->next) if(oe->distance>=0)
	{
		loopv(oe->mapmodels)
		{
			extentity &e = *ents[oe->mapmodels[i]];
			if(!e.visible) continue;
			if(renderedmms.empty() || renderedmms.last()!=oe)
			{
				renderedmms.add(oe);
				oe->query = doquery && oe->distance>0 && !(++skipoq%oqmm) ? newquery(oe) : NULL;
				if(oe->query) startmodelquery(oe->query);
			}
			rendermapmodel(e);
            e.visible = false;
		}
		if(renderedmms.length() && renderedmms.last()==oe && oe->query) endmodelquery();
	}
	endmodelbatches();

	bool colormask = true;
	for(octaentities *oe = visiblemms; oe; oe = oe->next) if(oe->distance<0)
	{
		oe->query = doquery ? newquery(oe) : NULL;
		if(!oe->query) continue;
		if(colormask)
		{
			glDepthMask(GL_FALSE);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			nocolorshader->set();
			colormask = false;
		}
		startquery(oe->query);
		drawbb(oe->bbmin, ivec(oe->bbmax).sub(oe->bbmin));
		endquery(oe->query);
	}
	if(!colormask)
	{
		glDepthMask(GL_TRUE);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, fading ? GL_FALSE : GL_TRUE);
	}
}

static inline bool bbinsideva(const ivec &bo, const ivec &br, vtxarray *va)
{
    return bo.x >= va->bbmin.x && bo.y >= va->bbmin.y && va->o.z >= va->bbmin.z &&
        bo.x + br.x <= va->bbmax.x && bo.y + br.y <= va->bbmax.y && bo.z + br.z <= va->bbmax.z;
}

static inline bool bboccluded(const ivec &bo, const ivec &br, cube *c, const ivec &o, int size)
{
    loopoctabox(o, size, bo, br)
    {
        ivec co(i, o.x, o.y, o.z, size);
        if(c[i].ext && c[i].ext->va)
        {
            vtxarray *va = c[i].ext->va;
            if(va->curvfc >= VFC_FOGGED || (va->occluded >= OCCLUDE_BB && bbinsideva(bo, br, va))) continue;
        }
        if(c[i].children && bboccluded(bo, br, c[i].children, co, size>>1)) continue;
        return false;
    }
    return true;
}

bool bboccluded(const ivec &bo, const ivec &br)
{
    int diff = (bo.x^(bo.x+br.x)) | (bo.y^(bo.y+br.y)) | (bo.z^(bo.z+br.z));
    if(diff&~((1<<worldscale)-1)) return false;
    int scale = worldscale-1;
    if(diff&(1<<scale)) return bboccluded(bo, br, worldroot, ivec(0, 0, 0), 1<<scale);
    cube *c = &worldroot[octastep(bo.x, bo.y, bo.z, scale)];
    if(c->ext && c->ext->va)
    {
        vtxarray *va = c->ext->va;
        if(va->curvfc >= VFC_FOGGED || va->occluded >= OCCLUDE_BB) return true;
    }
    scale--;
    while(c->children && !(diff&(1<<scale)))
    {
        c = &c->children[octastep(bo.x, bo.y, bo.z, scale)];
        if(c->ext && c->ext->va)
        {
            vtxarray *va = c->ext->va;
            if(va->curvfc >= VFC_FOGGED || va->occluded >= OCCLUDE_BB) return true;
        }
        scale--;
    }
    if(c->children) return bboccluded(bo, br, c->children, ivec(bo).mask(~((2<<scale)-1)), 1<<scale);
    return false;
}

VAR(outline, 0, 0, 0xFFFFFF);
VAR(dtoutline, 0, 0, 1);

void renderoutline()
{
	if(!editmode || !outline) return;

	notextureshader->set();

	glDisable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);

	glPushMatrix();

    glEnable(GL_POLYGON_OFFSET_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor3ub((outline>>16)&0xFF, (outline>>8)&0xFF, outline&0xFF);

	if(dtoutline) glDisable(GL_DEPTH_TEST);

	resetorigin();
    vtxarray *prev = NULL;
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        if(!va->texs || va->occluded >= OCCLUDE_GEOM) continue;

        if(!prev || va->vbuf != prev->vbuf)
        {
            setorigin(va);
            if(hasVBO)
            {
                glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->ebuf);
            }
            glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);
        }

        drawvatris(va, 3*va->tris, va->edata);
        xtravertsva += va->verts;

        prev = va;
    }

	if(dtoutline) glEnable(GL_DEPTH_TEST);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);

	glPopMatrix();

	if(hasVBO)
	{
		glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
		glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_TEXTURE_2D);

	defaultshader->set();
}

void rendershadowmapreceivers()
{
    if(!hasBE) return;

    static Shader *shadowmapshader = NULL;
    if(!shadowmapshader) shadowmapshader = lookupshaderbyname("shadowmapreceiver");
    shadowmapshader->set();

    glDisable(GL_TEXTURE_2D);
    glEnableClientState(GL_VERTEX_ARRAY);

    glCullFace(GL_BACK);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_GREATER);

    extern int ati_minmax_bug;
    if(!ati_minmax_bug) glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);

    glEnable(GL_BLEND);
    glBlendEquation_(GL_MAX_EXT);
    glBlendFunc(GL_ONE, GL_ONE);

    glPushMatrix();

    resetorigin();
    vtxarray *prev = NULL;
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        if(va->curvfc >= VFC_FOGGED || !isshadowmapreceiver(va)) continue;

        if(!prev || va->vbuf != prev->vbuf)
        {
            setorigin(va);
            if(hasVBO)
            {
                glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->ebuf);
            }
            glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);
        }

        drawvatris(va, 3*va->tris, va->edata);
        xtravertsva += va->verts;

        prev = va;
    }

    glPopMatrix();

    glDisable(GL_BLEND);
    glBlendEquation_(GL_FUNC_ADD_EXT);

    glCullFace(GL_FRONT);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    if(!ati_minmax_bug) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_2D);
}

float orientation_tangent [3][4] = { {  0,1, 0,0 }, { 1,0, 0,0 }, { 1,0,0,0 }};
float orientation_binormal[3][4] = { {  0,0,-1,0 }, { 0,0,-1,0 }, { 0,1,0,0 }};

struct renderstate
{
    bool colormask, depthmask, mtglow;
    GLuint vbuf;
    float fogplane;
    int diffusetmu, lightmaptmu, glowtmu, fogtmu, causticstmu;
    GLfloat color[4];
    GLuint textures[8];
    Slot *slot;
    int texgendim, texgenw, texgenh;
    float texgenscale;
    bool mttexgen;
    int visibledynlights;
    uint dynlightmask;

    renderstate() : colormask(true), depthmask(true), mtglow(false), vbuf(0), fogplane(-1), diffusetmu(0), lightmaptmu(1), glowtmu(-1), fogtmu(-1), causticstmu(-1), slot(NULL), texgendim(-1), texgenw(-1), texgenh(-1), texgenscale(-1), mttexgen(false), visibledynlights(0), dynlightmask(0)
    {
        loopk(4) color[k] = 1;
        loopk(8) textures[k] = 0;
    }
};

void renderquery(renderstate &cur, occludequery *query, vtxarray *va)
{
    nocolorshader->set();
    if(cur.colormask) { cur.colormask = false; glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); }
    if(cur.depthmask) { cur.depthmask = false; glDepthMask(GL_FALSE); }

    startquery(query);

    if(vaorigin==ivec(-1, -1, -1)) setorigin(va);
    ivec origin = vaorigin;

    vec camera(camera1->o);
    if(reflecting && !refracting) camera.z = reflecting;

    drawbb(ivec(va->bbmin).sub(origin).mul(1<<VVEC_FRAC),
           ivec(va->bbmax).sub(va->bbmin).mul(1<<VVEC_FRAC),
           vec(camera).sub(origin.tovec()).mul(1<<VVEC_FRAC));

    endquery(query);
}

enum
{
	RENDERPASS_LIGHTMAP = 0,
	RENDERPASS_COLOR,
	RENDERPASS_Z,
    RENDERPASS_GLOW,
    RENDERPASS_CAUSTICS,
    RENDERPASS_FOG
};

struct geombatch
{
    const elementset &es;
    Slot &slot;
    ushort *edata;
    vtxarray *va;
    int next, batch;

    geombatch(const elementset &es, ushort *edata, vtxarray *va)
      : es(es), slot(lookuptexture(es.texture)), edata(edata), va(va),
        next(-1), batch(-1)
    {}

    int compare(const geombatch &b) const
    {
        if(va->vbuf < b.va->vbuf) return -1;
        if(va->vbuf > b.va->vbuf) return 1;
        if(va->dynlightmask < b.va->dynlightmask) return -1;
        if(va->dynlightmask > b.va->dynlightmask) return 1;
        if(renderpath!=R_FIXEDFUNCTION)
        {
            if(slot.shader < b.slot.shader) return -1;
            if(slot.shader > b.slot.shader) return 1;
            if(slot.params.length() < b.slot.params.length()) return -1;
            if(slot.params.length() > b.slot.params.length()) return 1;
        }
        if(es.texture < b.es.texture) return -1;
        if(es.texture > b.es.texture) return 1;
        if(es.lmid < b.es.lmid) return -1;
        if(es.lmid > b.es.lmid) return 1;
        if(es.envmap < b.es.envmap) return -1;
        if(es.envmap > b.es.envmap) return 1;
        return 0;
    }
};

static vector<geombatch> geombatches;
static int firstbatch = -1, numbatches = 0;

static void mergetexs(vtxarray *va, elementset *texs = NULL, int numtexs = 0, ushort *edata = NULL)
{
    if(!texs)
    {
        texs = va->eslist;
        numtexs = va->texs;
        edata = va->edata;
    }

    if(firstbatch < 0)
    {
        firstbatch = geombatches.length();
        numbatches = numtexs;
        loopi(numtexs-1)
        {
            geombatches.add(geombatch(texs[i], edata, va)).next = i+1;
            edata += texs[i].length[5];
        }
        geombatches.add(geombatch(texs[numtexs-1], edata, va));
        return;
    }

    int prevbatch = -1, curbatch = firstbatch, curtex = 0;
    do
    {
        geombatch &b = geombatches.add(geombatch(texs[curtex], edata, va));
        edata += texs[curtex].length[5];
        int dir = -1;
        while(curbatch >= 0)
        {
            dir = b.compare(geombatches[curbatch]);
            if(dir <= 0) break;
            prevbatch = curbatch;
            curbatch = geombatches[curbatch].next;
        }
        if(!dir)
        {
            int last = curbatch, next;
            for(;;)
            {
                next = geombatches[last].batch;
                if(next < 0) break;
                last = next;
            }
            if(last==curbatch)
            {
                b.batch = curbatch;
                b.next = geombatches[curbatch].next;
                if(prevbatch < 0) firstbatch = geombatches.length()-1;
                else geombatches[prevbatch].next = geombatches.length()-1;
                curbatch = geombatches.length()-1;
            }
            else
            {
                b.batch = next;
                geombatches[last].batch = geombatches.length()-1;
            }
        }
        else
        {
            numbatches++;
            b.next = curbatch;
            if(prevbatch < 0) firstbatch = geombatches.length()-1;
            else geombatches[prevbatch].next = geombatches.length()-1;
            prevbatch = geombatches.length()-1;
        }
    }
    while(++curtex < numtexs);
}

static void mergetexmask(vtxarray *va, int texmask)
{
    int start = -1;
    ushort *edata = va->edata, *startdata = NULL;
    loopi(va->texs)
    {
        elementset &es = va->eslist[i];
        Slot &slot = lookuptexture(es.texture, false);
        if(slot.texmask&texmask)
        {
            if(start<0) { start = i; startdata = edata; }
        }
        else if(start>=0)
        {
            mergetexs(va, &va->eslist[start], i-start, startdata);
            start = -1;
        }
        edata += es.length[5];
    }
    if(start>=0) mergetexs(va, &va->eslist[start], va->texs-start, startdata);
}

static void changefogplane(renderstate &cur, int pass, vtxarray *va)
{
    if(renderpath!=R_FIXEDFUNCTION)
    {
        if(reflecting)
        {
            float fogplane = reflecting - (va->o.z & ~VVEC_INT_MASK);
            if(cur.fogplane!=fogplane)
            {
                cur.fogplane = fogplane;
                if(refracting) setfogplane(1.0f/(1<<VVEC_FRAC), fogplane, false, -0.25f/(1<<VVEC_FRAC), 0.5f + 0.25f*fogplane);
                else setfogplane(0, 0, false, 0.25f/(1<<VVEC_FRAC), 0.5f - 0.25f*fogplane);
            }
        }
    }
    else if(pass==RENDERPASS_FOG || (cur.fogtmu>=0 && (pass==RENDERPASS_LIGHTMAP || pass==RENDERPASS_GLOW)))
    {
        if(pass==RENDERPASS_LIGHTMAP) glActiveTexture_(GL_TEXTURE0_ARB+cur.fogtmu);
        else if(pass==RENDERPASS_GLOW) glActiveTexture_(GL_TEXTURE1_ARB);
        GLfloat s[4] = { 0, 0, -1.0f/(waterfog<<VVEC_FRAC), (refracting - (va->o.z & ~VVEC_INT_MASK))/waterfog };
        glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
        if(pass==RENDERPASS_LIGHTMAP) glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        else if(pass==RENDERPASS_GLOW) glActiveTexture_(GL_TEXTURE0_ARB);
    }
}

static void changevbuf(renderstate &cur, int pass, vtxarray *va)
{
    if(setorigin(va, pass==RENDERPASS_LIGHTMAP && !envmapping))
    {
        cur.visibledynlights = 0;
        cur.dynlightmask = 0;
    }
    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->ebuf);
    }
    cur.vbuf = va->vbuf;

    glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);

    if(pass==RENDERPASS_LIGHTMAP)
    {
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
        glTexCoordPointer(2, GL_SHORT, VTXSIZE, floatvtx ? &((fvertex *)va->vdata)[0].u : &va->vdata[0].u);
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);

        if(renderpath!=R_FIXEDFUNCTION)
        {
            glColorPointer(3, GL_UNSIGNED_BYTE, VTXSIZE, floatvtx ? &((fvertex *)va->vdata)[0].n : &va->vdata[0].n);
            setenvparamfv("camera", SHPARAM_VERTEX, 4, vec4(camera1->o, 1).sub(ivec(va->o).mask(~VVEC_INT_MASK).tovec()).mul(1<<VVEC_FRAC).v);
        }
    }
}

static void changebatchtmus(renderstate &cur, int pass, geombatch &b)
{
    bool changed = false;
    extern bool brightengeom;
    int lmid = brightengeom ? LMID_BRIGHT : b.es.lmid;
    if(cur.textures[cur.lightmaptmu]!=lightmaptexs[lmid].id)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
        glBindTexture(GL_TEXTURE_2D, cur.textures[cur.lightmaptmu] = lightmaptexs[lmid].id);
        changed = true;
    }
    if(renderpath!=R_FIXEDFUNCTION)
    {
        int tmu = cur.lightmaptmu+1;
        if(b.slot.shader->type&SHADER_NORMALSLMS)
        {
            if(cur.textures[tmu]!=lightmaptexs[b.es.lmid+1].id)
            {
                glActiveTexture_(GL_TEXTURE0_ARB+tmu);
                glBindTexture(GL_TEXTURE_2D, cur.textures[tmu] = lightmaptexs[b.es.lmid+1].id);
                changed = true;
            }
            tmu++;
        }
        if(b.slot.shader->type&SHADER_ENVMAP && b.es.envmap!=EMID_CUSTOM)
        {
            GLuint emtex = lookupenvmap(b.es.envmap);
            if(cur.textures[tmu]!=emtex)
            {
                glActiveTexture_(GL_TEXTURE0_ARB+tmu);
                glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cur.textures[tmu] = emtex);
                changed = true;
            }
        }
    }
    if(changed) glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
}

static void changeslottmus(renderstate &cur, int pass, Slot &slot)
{
    if(pass==RENDERPASS_LIGHTMAP || pass==RENDERPASS_COLOR)
    {
        GLuint diffusetex = slot.sts.empty() ? notexture->id : slot.sts[0].t->id;
        if(cur.textures[cur.diffusetmu]!=diffusetex)
            glBindTexture(GL_TEXTURE_2D, cur.textures[cur.diffusetmu] = diffusetex);
    }

    if(renderpath==R_FIXEDFUNCTION)
    {
        if(pass==RENDERPASS_GLOW || cur.glowtmu>=0) if(slot.texmask&(1<<TEX_GLOW)) loopvj(slot.sts)
        {
            Slot::Tex &t = slot.sts[j];
            if(t.type==TEX_GLOW && t.combined<0)
            {
                if(cur.glowtmu>=0)
                {
                    glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
                    if(!cur.mtglow) { glEnable(GL_TEXTURE_2D); cur.mtglow = true; }
                }
                if(cur.textures[cur.glowtmu]!=t.t->id)
                    glBindTexture(GL_TEXTURE_2D, cur.textures[cur.glowtmu] = t.t->id);
            }
        }
        if(cur.mtglow)
        {
            if(!(slot.texmask&(1<<TEX_GLOW)))
            {
                glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
                glDisable(GL_TEXTURE_2D);
                cur.mtglow = false;
            }
            glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        }
    }
    else
    {
        int tmu = cur.lightmaptmu+1, envmaptmu = -1;
        if(slot.shader->type&SHADER_NORMALSLMS) tmu++;
        if(slot.shader->type&SHADER_ENVMAP) envmaptmu = tmu++;
        loopvj(slot.sts)
        {
            Slot::Tex &t = slot.sts[j];
            if(t.type==TEX_DIFFUSE || t.combined>=0) continue;
            if(t.type==TEX_ENVMAP)
            {
                if(envmaptmu>=0 && cur.textures[envmaptmu]!=t.t->id)
                {
                    glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
                    glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, cur.textures[envmaptmu] = t.t->id);
                }
                continue;
            }
            else if(cur.textures[tmu]!=t.t->id)
            {
                glActiveTexture_(GL_TEXTURE0_ARB+tmu);
                glBindTexture(GL_TEXTURE_2D, cur.textures[tmu] = t.t->id);
            }
            tmu++;
        }
        glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
    }

    cur.slot = &slot;
}

static void changeshader(renderstate &cur, Shader *s, Slot &slot, bool shadowed)
{
    if(fading)
    {
        if(shadowed) s->variant(min(s->variants[3].length()-1, cur.visibledynlights), 3)->set(&slot);
        else s->variant(min(s->variants[2].length()-1, cur.visibledynlights), 2)->set(&slot);
    }
    else if(shadowed) s->variant(min(s->variants[1].length()-1, cur.visibledynlights), 1)->set(&slot);
    else if(!cur.visibledynlights) s->set(&slot);
    else s->variant(min(s->variants[0].length()-1, cur.visibledynlights-1))->set(&slot);
    if(s->type&SHADER_GLSLANG) cur.texgendim = -1;
}

static void changetexgen(renderstate &cur, Shader *s, Texture *tex, float scale, int dim)
{
    static const int si[] = { 1, 0, 0 };
    static const int ti[] = { 2, 2, 1 };

    GLfloat sgen[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    sgen[si[dim]] = 8.0f/scale/(tex->xs<<VVEC_FRAC);
    GLfloat tgen[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    tgen[ti[dim]] = (dim <= 1 ? -8.0f : 8.0f)/scale/(tex->ys<<VVEC_FRAC);

    if(renderpath==R_FIXEDFUNCTION)
    {
        if(cur.texgendim!=dim || cur.texgenw!=tex->xs || cur.texgenh!=tex->ys || cur.texgenscale!=scale)
        {
            glTexGenfv(GL_S, GL_OBJECT_PLANE, sgen);
            glTexGenfv(GL_T, GL_OBJECT_PLANE, tgen);
            // KLUGE: workaround for buggy nvidia drivers
            // object planes are somehow invalid unless texgen is toggled
            extern int nvidia_texgen_bug;
            if(nvidia_texgen_bug)
            {
                glDisable(GL_TEXTURE_GEN_S);
                glDisable(GL_TEXTURE_GEN_T);
                glEnable(GL_TEXTURE_GEN_S);
                glEnable(GL_TEXTURE_GEN_T);
            }
        }

        if(cur.mtglow)
        {
            glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
            glTexGenfv(GL_S, GL_OBJECT_PLANE, sgen);
            glTexGenfv(GL_T, GL_OBJECT_PLANE, tgen);
            glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        }
    }
    else
    {
        // have to pass in env, otherwise same problem as fixed function
        setlocalparamfv("texgenS", SHPARAM_VERTEX, 0, sgen);
        setlocalparamfv("texgenT", SHPARAM_VERTEX, 1, tgen);
        setlocalparamfv("orienttangent", SHPARAM_VERTEX, 2, orientation_tangent[dim]);
        setlocalparamfv("orientbinormal", SHPARAM_VERTEX, 3, orientation_binormal[dim]);
        if(s->type&SHADER_GLSLANG) cur.texgendim = -1;
    }

    cur.texgendim = dim;
    cur.texgenw = tex->xs;
    cur.texgenh = tex->ys;
    cur.texgenscale = scale;
    cur.mttexgen = cur.mtglow;
}

struct batchdrawinfo
{
    ushort *edata;
    ushort len, minvert, maxvert;

    batchdrawinfo(geombatch &b, int dim, ushort offset, ushort len)
      : edata(b.edata + offset), len(len),
        minvert(b.va->shadowed ? b.es.minvert[dim] : min(b.es.minvert[dim], b.es.minvert[dim+1])),
        maxvert(b.va->shadowed ? b.es.maxvert[dim] : max(b.es.maxvert[dim], b.es.maxvert[dim+1]))
    {}
};

static void renderbatch(renderstate &cur, int pass, geombatch &b)
{
    static vector<batchdrawinfo> draws[6];
    for(geombatch *curbatch = &b;; curbatch = &geombatches[curbatch->batch])
    {
        int dim = 0;
        ushort offset = 0, len = 0;
        loopi(3)
        {
            offset += len;
            len = curbatch->es.length[dim + (curbatch->va->shadowed ? 0 : 1)] - offset;
            if(len) draws[dim].add(batchdrawinfo(*curbatch, dim, offset, len));
            dim++;

            if(curbatch->va->shadowed)
            {
                offset += len;
                len = curbatch->es.length[dim] - offset;
                if(len) draws[dim].add(batchdrawinfo(*curbatch, dim, offset, len));
            }
            dim++;
        }
        if(curbatch->batch < 0) break;
    }
    Shader *s = b.slot.shader;
    Texture *tex = notexture;
    float scale = 1;
    if(b.slot.sts.length())
    {
        tex = b.slot.sts[0].t;
        scale = b.slot.sts[0].scale;
    }
    loop(shadowed, 2)
    {
        bool rendered = false;
        loop(dim, 3)
        {
            vector<batchdrawinfo> &draw = draws[2*dim + shadowed];
            if(draw.empty()) continue;

            if(!rendered)
            {
                if(renderpath!=R_FIXEDFUNCTION) changeshader(cur, s, b.slot, shadowed!=0);
                rendered = true;
            }
            if(cur.texgendim!=dim || cur.texgenw!=tex->xs || cur.texgenh!=tex->ys || cur.texgenscale!=scale || cur.mtglow>cur.mttexgen)
                changetexgen(cur, s, tex, scale, dim);

            loopv(draw)
            {
                batchdrawinfo &info = draw[i];
                drawtris(info.len, info.edata, info.minvert, info.maxvert);
                vtris += info.len/3;
            }
            draw.setsizenodelete(0);
        }
    }
}

static void resetbatches()
{
    geombatches.setsizenodelete(0);
    firstbatch = -1;
    numbatches = 0;
}

static void renderbatches(renderstate &cur, int pass)
{
    cur.slot = NULL;
    int curbatch = firstbatch;
    if(curbatch >= 0)
    {
        if(!cur.depthmask) { cur.depthmask = true; glDepthMask(GL_TRUE); }
        if(!cur.colormask) { cur.colormask = true; glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); }
    }
    while(curbatch >= 0)
    {
        geombatch &b = geombatches[curbatch];
        curbatch = b.next;

        if(cur.vbuf != b.va->vbuf)
        {
            changevbuf(cur, pass, b.va);
            changefogplane(cur, pass, b.va);
        }
        if(pass == RENDERPASS_LIGHTMAP)
        {
            changebatchtmus(cur, pass, b);
            if(cur.dynlightmask != b.va->dynlightmask)
            {
                cur.visibledynlights = setdynlights(b.va);
                cur.dynlightmask = b.va->dynlightmask;
            }
        }
        if(cur.slot != &b.slot) changeslottmus(cur, pass, b.slot);

        renderbatch(cur, pass, b);
    }

    if(pass == RENDERPASS_LIGHTMAP && cur.mtglow)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
        glDisable(GL_TEXTURE_2D);
        glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        cur.mtglow = false;
    }

    resetbatches();
}

void renderzpass(renderstate &cur, vtxarray *va)
{
    if(cur.vbuf!=va->vbuf) changevbuf(cur, RENDERPASS_Z, va);
    if(!cur.depthmask) { cur.depthmask = true; glDepthMask(GL_TRUE); }
    if(cur.colormask) { cur.colormask = false; glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); }

    extern int apple_glsldepth_bug;
    if(renderpath!=R_GLSLANG || !apple_glsldepth_bug)
    {
        nocolorshader->set();
        drawvatris(va, 3*va->tris, va->edata);
    }
    else
    {
        static Shader *nocolorglslshader = NULL;
        if(!nocolorglslshader) nocolorglslshader = lookupshaderbyname("nocolorglsl");
        Slot *lastslot = NULL;
        int lastdraw = 0, offset = 0;
        loopi(va->texs)
        {
            Slot &slot = lookuptexture(va->eslist[i].texture);
            if(lastslot && (slot.shader->type&SHADER_GLSLANG) != (lastslot->shader->type&SHADER_GLSLANG) && offset > lastdraw)
            {
                (lastslot->shader->type&SHADER_GLSLANG ? nocolorglslshader : nocolorshader)->set();
                drawvatris(va, offset-lastdraw, va->edata+lastdraw);
                lastdraw = offset;
            }
            lastslot = &slot;
            offset += va->eslist[i].length[5];
        }
        if(offset > lastdraw)
        {
            (lastslot->shader->type&SHADER_GLSLANG ? nocolorglslshader : nocolorshader)->set();
            drawvatris(va, offset-lastdraw, va->edata+lastdraw);
        }
    }
    xtravertsva += va->verts;
}

vector<vtxarray *> foggedvas;

#define startvaquery(va, flush) \
    do { \
        if(!refracting) \
        { \
            occludequery *query = !reflecting ? va->query : (camera1->o.z >= reflecting ? va->rquery : NULL); \
            if(query) \
            { \
                flush; \
                startquery(query); \
            } \
        } \
    } while(0)


#define endvaquery(va, flush) \
    do { \
        if(!refracting) \
        { \
            occludequery *query = !reflecting ? va->query : (camera1->o.z >= reflecting ? va->rquery : NULL); \
            if(query) \
            { \
                flush; \
                endquery(query); \
            } \
        } \
    } while(0)

void renderfoggedvas(renderstate &cur, bool doquery = false)
{
    static Shader *fogshader = NULL;
    if(!fogshader) fogshader = lookupshaderbyname("fogworld");
    fogshader->set();

    glDisable(GL_TEXTURE_2D);

    uchar wcol[3];
    getwatercolour(wcol);
    glColor3ubv(wcol);

    loopv(foggedvas)
    {
        vtxarray *va = foggedvas[i];
        if(cur.vbuf!=va->vbuf) changevbuf(cur, RENDERPASS_FOG, va);

        if(doquery) startvaquery(va, );
        drawvatris(va, 3*va->tris, va->edata);
        vtris += va->tris;
        if(doquery) endvaquery(va, );
    }

    glEnable(GL_TEXTURE_2D);

    foggedvas.setsizenodelete(0);
}

VAR(batchgeom, 0, 1, 1);

void renderva(renderstate &cur, vtxarray *va, int pass = RENDERPASS_LIGHTMAP, bool fogpass = false, bool doquery = false)
{
    switch(pass)
    {
        case RENDERPASS_GLOW:
            if(!(va->texmask&(1<<TEX_GLOW))) return;
            mergetexmask(va, 1<<TEX_GLOW);
            if(!batchgeom && geombatches.length()) renderbatches(cur, pass);
            break;

        case RENDERPASS_COLOR:
        case RENDERPASS_LIGHTMAP:
            vverts += va->verts;
            va->shadowed = false;
            va->dynlightmask = 0;
            if(fogpass ? va->o.z+va->size<=refracting-waterfog : va->curvfc==VFC_FOGGED)
            {
                foggedvas.add(va);
                break;
            }
            if(renderpath!=R_FIXEDFUNCTION && !envmapping)
            {
                va->shadowed = isshadowmapreceiver(va);
                calcdynlightmask(va);
            }
            if(doquery) startvaquery(va, { if(geombatches.length()) renderbatches(cur, pass); });
            mergetexs(va);
            if(doquery) endvaquery(va, { if(geombatches.length()) renderbatches(cur, pass); });
            else if(!batchgeom && geombatches.length()) renderbatches(cur, pass);
            break;

        case RENDERPASS_FOG:
            if(cur.vbuf!=va->vbuf)
            {
                changevbuf(cur, pass, va);
                changefogplane(cur, pass, va);
            }
            drawvatris(va, 3*va->tris, va->edata);
            xtravertsva += va->verts;
            break;

        case RENDERPASS_CAUSTICS:
            if(cur.vbuf!=va->vbuf) changevbuf(cur, pass, va);
            drawvatris(va, 3*va->tris, va->edata);
            xtravertsva += va->verts;
            break;

        case RENDERPASS_Z:
            if(doquery) startvaquery(va, );
            renderzpass(cur, va);
            if(doquery) endvaquery(va, );
            break;
    }
}

VAR(oqdist, 0, 256, 1024);
VAR(zpass, 0, 1, 1);
VAR(glowpass, 0, 1, 1);

extern int ati_texgen_bug;

static void setuptexgen(int dims = 2)
{
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glEnable(GL_TEXTURE_GEN_S);
    if(dims>=2)
    {
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glEnable(GL_TEXTURE_GEN_T);
        if(ati_texgen_bug) glEnable(GL_TEXTURE_GEN_R);     // should not be needed, but apparently makes some ATI drivers happy
    }
}

static void disabletexgen(int dims = 2)
{
    glDisable(GL_TEXTURE_GEN_S);
    if(dims>=2)
    {
        glDisable(GL_TEXTURE_GEN_T);
        if(ati_texgen_bug) glDisable(GL_TEXTURE_GEN_R);
    }
}

GLuint fogtex = 0;

void createfogtex()
{
    extern int bilinear;
    uchar buf[2*256] = { 255, 0, 255, 255 };
    if(!bilinear) loopi(256) { buf[2*i] = 255; buf[2*i+1] = i; }
    glGenTextures(1, &fogtex);
    createtexture(fogtex, bilinear ? 2 : 256, 1, buf, 3, false, GL_LUMINANCE_ALPHA, GL_TEXTURE_1D);
}

#define NUMCAUSTICS 32

VAR(causticscale, 0, 100, 10000);
VAR(causticmillis, 0, 75, 1000);
VARP(caustics, 0, 1, 1);

static Texture *caustictex[NUMCAUSTICS] = { NULL };

void loadcaustics()
{
    if(caustictex[0]) return;
    loopi(NUMCAUSTICS)
    {
        s_sprintfd(name)(
            renderpath==R_FIXEDFUNCTION ?
                "<mad:0.6,0.4>caustics/caust%.2d.png" :
                "<mad:-0.6,0.6>caustics/caust%.2d.png",
            i);
        caustictex[i] = textureload(name);
    }
}

void cleanupva()
{
    vaclearc(worldroot);
    clearqueries();
    if(fogtex) { glDeleteTextures(1, &fogtex); fogtex = 0; }
    loopi(NUMCAUSTICS) caustictex[i] = NULL;
}

void setupcaustics(int tmu, float blend, GLfloat *color = NULL)
{
    if(!caustictex[0]) loadcaustics();

    GLfloat s[4] = { 0.011f, 0, 0.0066f, 0 };
    GLfloat t[4] = { 0, 0.011f, 0.0066f, 0 };
    loopk(3)
    {
        s[k] *= 100.0f/(causticscale<<VVEC_FRAC);
        t[k] *= 100.0f/(causticscale<<VVEC_FRAC);
    }
    int tex = (lastmillis/causticmillis)%NUMCAUSTICS;
    float frac = float(lastmillis%causticmillis)/causticmillis;
    if(color) color[3] = frac;
    else glColor4f(1, 1, 1, frac);
    loopi(2)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+tmu+i);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, caustictex[(tex+i)%NUMCAUSTICS]->id);
        if(renderpath==R_FIXEDFUNCTION)
        {
            setuptexgen();
            setuptmu(tmu+i, !i ? "= T" : "T , P @ Ca");
            glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
            glTexGenfv(GL_T, GL_OBJECT_PLANE, t);
        }
    }
    if(renderpath!=R_FIXEDFUNCTION)
    {
        static Shader *causticshader = NULL;
        if(!causticshader) causticshader = lookupshaderbyname("caustic");
        causticshader->set();
        setlocalparamfv("texgenS", SHPARAM_VERTEX, 0, s);
        setlocalparamfv("texgenT", SHPARAM_VERTEX, 1, t);
        setlocalparamf("frameoffset", SHPARAM_PIXEL, 0, blend*(1-frac), blend*frac, blend);
    }
}

void setupTMUs(renderstate &cur, float causticspass, bool fogpass)
{
    extern GLuint shadowmapfb;
    if(!reflecting && !refracting && !envmapping && shadowmap && shadowmapfb)
    {
        glDisableClientState(GL_VERTEX_ARRAY);

        if(hasVBO)
        {
            glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        }

        rendershadowmap();

        glEnableClientState(GL_VERTEX_ARRAY);
    }

    if(renderpath==R_FIXEDFUNCTION)
    {
        if(nolights) cur.lightmaptmu = -1;
        else if(maxtmus>=3)
        {
            if(maxtmus>=4 && causticspass>=1)
            {
                cur.causticstmu = 0;
                cur.diffusetmu = 2;
                cur.lightmaptmu = 3;
                if(maxtmus>=5)
                {
                    if(fogpass) cur.fogtmu = 4;
                    else if(glowpass) cur.glowtmu = 4;
                }
            }
            else if(fogpass && causticspass<1) cur.fogtmu = 2;
            else if(glowpass) cur.glowtmu = 2;
        }
        if(cur.glowtmu>=0)
        {
            glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
			setuptexgen();
            setuptmu(cur.glowtmu, "P + T");
        }
        if(cur.fogtmu>=0)
		{
            glActiveTexture_(GL_TEXTURE0_ARB+cur.fogtmu);
            glEnable(GL_TEXTURE_1D);
            setuptexgen(1);
            setuptmu(cur.fogtmu, "C , P @ Ta");
            if(!fogtex) createfogtex();
            glBindTexture(GL_TEXTURE_1D, fogtex);
            uchar wcol[3];
            getwatercolour(wcol);
            loopk(3) cur.color[k] = wcol[k]/255.0f;
        }
        if(cur.causticstmu>=0) setupcaustics(cur.causticstmu, causticspass, cur.color);
	}
    else
	{
        // need to invalidate vertex params in case they were used somewhere else for streaming params
        invalidateenvparams(SHPARAM_VERTEX, 10, RESERVEDSHADERPARAMS + MAXSHADERPARAMS - 10);
		glEnableClientState(GL_COLOR_ARRAY);
		loopi(8-2) { glActiveTexture_(GL_TEXTURE2_ARB+i); glEnable(GL_TEXTURE_2D); }
		glActiveTexture_(GL_TEXTURE0_ARB);
		setenvparamf("ambient", SHPARAM_PIXEL, 5, ambient/255.0f, ambient/255.0f, ambient/255.0f);
		setenvparamf("millis", SHPARAM_VERTEX, 6, lastmillis/1000.0f, lastmillis/1000.0f, lastmillis/1000.0f);
	}

    glColor4fv(cur.color);

    if(cur.lightmaptmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);

        setuptmu(cur.lightmaptmu, "P * T x 2");
		glEnable(GL_TEXTURE_2D);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glScalef(1.0f/SHRT_MAX, 1.0f/SHRT_MAX, 1.0f);
        glMatrixMode(GL_MODELVIEW);

        glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        glEnable(GL_TEXTURE_2D);
        setuptmu(cur.diffusetmu, cur.diffusetmu>0 ? "P * T" : "= T");
	}

    setuptexgen();
}

void cleanupTMUs(renderstate &cur)
{
    if(cur.lightmaptmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
        glClientActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);

        resettmu(cur.lightmaptmu);
	glDisable(GL_TEXTURE_2D);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
    }
    if(cur.glowtmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
        resettmu(cur.glowtmu);
        disabletexgen();
        glDisable(GL_TEXTURE_2D);
    }
    if(cur.fogtmu>=0)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.fogtmu);
        resettmu(cur.fogtmu);
        disabletexgen(1);
        glDisable(GL_TEXTURE_1D);
    }
    if(cur.causticstmu>=0) loopi(2)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+cur.causticstmu+i);
        resettmu(cur.causticstmu+i);
        disabletexgen();
        glDisable(GL_TEXTURE_2D);
    }

    if(cur.lightmaptmu>=0)
	{
        glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        resettmu(cur.diffusetmu);
        glDisable(GL_TEXTURE_2D);
    }

		disabletexgen();

    if(renderpath!=R_FIXEDFUNCTION)
    {
        glDisableClientState(GL_COLOR_ARRAY);
        loopi(8-2) { glActiveTexture_(GL_TEXTURE2_ARB+i); glDisable(GL_TEXTURE_2D); }
	}

    if(cur.lightmaptmu>=0)
    {
	glActiveTexture_(GL_TEXTURE0_ARB);
	glClientActiveTexture_(GL_TEXTURE0_ARB);
        glEnable(GL_TEXTURE_2D);
    }
}

#define FIRSTVA (reflecting && !refracting && camera1->o.z >= reflecting ? reflectedva : visibleva)
#define NEXTVA (reflecting && !refracting && camera1->o.z >= reflecting ? va->rnext : va->next)

void rendergeommultipass(renderstate &cur, int pass, bool fogpass)
{
    cur.vbuf = 0;
    for(vtxarray *va = FIRSTVA; va; va = NEXTVA)
    {
        if(!va->texs || va->occluded >= OCCLUDE_GEOM) continue;
        if(refracting || (reflecting && camera1->o.z < reflecting))
        {
            if(refracting && camera1->o.z >= refracting ? va->geommin.z > refracting : va->geommax.z <= refracting) continue;
            if((!hasOQ || !oqfrags) && va->distance > reflectdist) break;
        }
        else if(reflecting)
        {
            if(va->geommax.z <= reflecting || (va->rquery && checkquery(va->rquery))) continue;
        }
        if(fogpass ? va->o.z+va->size<=refracting-waterfog : va->curvfc==VFC_FOGGED) continue;
        renderva(cur, va, pass, fogpass);
    }
    if(geombatches.length()) renderbatches(cur, pass);
}

VAR(oqgeom, 0, 1, 1);
VAR(oqbatch, 0, 1, 1);

void rendergeom(float causticspass, bool fogpass)
{
    renderstate cur;

    if(causticspass && ((renderpath==R_FIXEDFUNCTION && maxtmus<2) || !causticscale || !causticmillis)) causticspass = 0;

	glEnableClientState(GL_VERTEX_ARRAY);

	if(!reflecting && !refracting)
	{
		flipqueries();
		vtris = vverts = 0;
	}

	bool doOQ = !refracting && (reflecting ? camera1->o.z >= reflecting && hasOQ && oqfrags && oqreflect : zpass!=0);
    if(!doOQ)
    {
        setupTMUs(cur, causticspass, fogpass);
        if(shadowmap) pushshadowmap();
    }

    finddynlights();

    glPushMatrix();

    resetorigin();

    resetbatches();

	for(vtxarray *va = FIRSTVA; va; va = NEXTVA)
	{
		if(!va->texs) continue;
		if(refracting || (reflecting && camera1->o.z < reflecting))
		{
			if((refracting && camera1->o.z >= refracting ? va->geommin.z > refracting : va->geommax.z <= reflecting) || va->occluded >= OCCLUDE_GEOM) continue;
			if((!hasOQ || !oqfrags) && va->distance > reflectdist) break;
		}
		else if(reflecting)
		{
			if(va->geommax.z <= reflecting) continue;
			if(doOQ)
			{
				va->rquery = newquery(&va->rquery);
				if(!va->rquery) continue;
				if(va->occluded >= OCCLUDE_BB || va->curvfc == VFC_NOT_VISIBLE)
				{
					renderquery(cur, va->rquery, va);
					continue;
				}
			}
		}
        else if(hasOQ && oqfrags && (zpass || va->distance > oqdist) && !insideva(va, camera1->o) && oqgeom)
		{
            if(!zpass && va->query && va->query->owner == va)
            {
                if(checkquery(va->query)) va->occluded = min(va->occluded+1, int(OCCLUDE_BB));
                else va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
            }
            if(zpass && oqbatch)
            {
                if(va->parent && va->parent->occluded >= OCCLUDE_BB)
                {
                    va->query = NULL;
                    va->occluded = OCCLUDE_PARENT;
                    continue;
                }
                if(va->query && va->query->owner == va && checkquery(va->query))
                    va->occluded = min(va->occluded+1, int(OCCLUDE_BB));
                else va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
                va->query = newquery(va);
                if(va->occluded >= OCCLUDE_GEOM)
                {
                    if(va->query) renderquery(cur, va->query, va);
                    continue;
                }
            }
            else if(zpass && va->parent &&
               (va->parent->occluded == OCCLUDE_PARENT ||
				(va->parent->occluded >= OCCLUDE_BB &&
				 va->parent->query && va->parent->query->owner == va->parent && va->parent->query->fragments < 0)))
			{
				va->query = NULL;
                if(va->occluded >= OCCLUDE_GEOM || pvsoccluded(va->geommin, va->geommax))
				{
					va->occluded = OCCLUDE_PARENT;
					continue;
				}
			}
			else if(va->occluded >= OCCLUDE_GEOM)
			{
				va->query = newquery(va);
				if(va->query) renderquery(cur, va->query, va);
				continue;
			}
			else va->query = newquery(va);
		}
		else
		{
			va->query = NULL;
            va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
            if(va->occluded >= OCCLUDE_GEOM) continue;
		}

        renderva(cur, va, doOQ ? RENDERPASS_Z : (nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP), fogpass, true);
    }

    if(geombatches.length()) renderbatches(cur, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP);

    if(!cur.colormask) { cur.colormask = true; glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); }
    if(!cur.depthmask) { cur.depthmask = true; glDepthMask(GL_TRUE); }

	if(doOQ)
	{
        setupTMUs(cur, causticspass, fogpass);
        if(shadowmap)
        {
            pushshadowmap();
            resetorigin();
        }
		glDepthFunc(GL_LEQUAL);
        cur.vbuf = 0;

		for(vtxarray **prevva = &FIRSTVA, *va = FIRSTVA; va; prevva = &NEXTVA, va = NEXTVA)
		{
			if(!va->texs) continue;
			if(reflecting)
			{
				if(va->geommax.z <= reflecting) continue;
				if(va->rquery && checkquery(va->rquery))
				{
					if(va->occluded >= OCCLUDE_BB || va->curvfc == VFC_NOT_VISIBLE) *prevva = va->rnext;
					continue;
				}
			}
            else if(oqbatch)
            {
                if(va->query && va->occluded >= OCCLUDE_GEOM) continue;
                else if(va->occluded >= OCCLUDE_PARENT)
                {
                    if(va->parent->occluded<=OCCLUDE_BB && !va->parent->query)
                    {
                        va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
                        if(va->occluded >= OCCLUDE_GEOM) continue;
                    }
                    else continue;
                }
            }
			else if(va->parent && va->parent->occluded >= OCCLUDE_BB && (!va->parent->query || va->parent->query->fragments >= 0))
			{
				va->query = NULL;
				va->occluded = OCCLUDE_BB;
				continue;
			}
            else
            {
                if(va->query && checkquery(va->query)) va->occluded = min(va->occluded+1, int(OCCLUDE_BB));
                else va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
                if(va->occluded >= OCCLUDE_GEOM) continue;
            }

            renderva(cur, va, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP, fogpass);
        }
        if(geombatches.length()) renderbatches(cur, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP);
        if(oqbatch && !reflecting) for(vtxarray **prevva = &FIRSTVA, *va = FIRSTVA; va; prevva = &NEXTVA, va = NEXTVA)
        {
            if(!va->texs) continue;
            if(va->occluded < OCCLUDE_GEOM) continue;
            else if(va->query && checkquery(va->query)) continue;
            else if(va->parent && va->parent->occluded >= OCCLUDE_GEOM &&
                    va->parent->query && va->parent->query->owner && checkquery(va->parent->query))
            {
                va->occluded = OCCLUDE_BB;
                continue;
            }
            else
            {
                va->occluded = pvsoccluded(va->geommin, va->geommax) ? OCCLUDE_GEOM : OCCLUDE_NOTHING;
                if(va->occluded >= OCCLUDE_GEOM) continue;
            }
            renderva(cur, va, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP, fogpass);
        }
        if(geombatches.length()) renderbatches(cur, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP);

        if(foggedvas.empty()) glDepthFunc(GL_LESS);
    }

    if(shadowmap) popshadowmap();

    cleanupTMUs(cur);

    if(foggedvas.length())
    {
        renderfoggedvas(cur, !doOQ);
        if(doOQ) glDepthFunc(GL_LESS);
    }

    if(renderpath==R_FIXEDFUNCTION ? (glowpass && cur.glowtmu<0) || (causticspass>=1 && cur.causticstmu<0) || (fogpass && cur.fogtmu<0) : causticspass)
    {
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        static GLfloat zerofog[4] = { 0, 0, 0, 1 }, onefog[4] = { 1, 1, 1, 1 };
        GLfloat oldfogc[4];
        glGetFloatv(GL_FOG_COLOR, oldfogc);

        if(renderpath==R_FIXEDFUNCTION && glowpass && cur.glowtmu<0)
        {
            glBlendFunc(GL_ONE, GL_ONE);
			glFogfv(GL_FOG_COLOR, zerofog);
            setuptexgen();
            if(cur.fogtmu>=0)
            {
                setuptmu(0, "= T");
                glActiveTexture_(GL_TEXTURE1_ARB);
                glEnable(GL_TEXTURE_1D);
                setuptexgen(1);
                setuptmu(1, "C , P @ Ta");
                if(!fogtex) createfogtex();
                glBindTexture(GL_TEXTURE_1D, fogtex);
                glColor3f(0, 0, 0);
                glActiveTexture_(GL_TEXTURE0_ARB);
            }
            else glColor3f(1, 1, 1);
            rendergeommultipass(cur, RENDERPASS_GLOW, fogpass);
            disabletexgen();
            if(cur.fogtmu>=0)
            {
                resettmu(0);
                glActiveTexture_(GL_TEXTURE1_ARB);
                resettmu(1);
                disabletexgen();
                glDisable(GL_TEXTURE_1D);
                glActiveTexture_(GL_TEXTURE0_ARB);
            }
        }

        if(renderpath==R_FIXEDFUNCTION ? causticspass>=1 && cur.causticstmu<0 : causticspass)
        {
            setupcaustics(0, causticspass);
            glBlendFunc(GL_ZERO, renderpath==R_FIXEDFUNCTION ? GL_SRC_COLOR : GL_ONE_MINUS_SRC_COLOR);
            glFogfv(GL_FOG_COLOR, renderpath==R_FIXEDFUNCTION ? onefog : zerofog);
            if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
            rendergeommultipass(cur, RENDERPASS_CAUSTICS, fogpass);
            if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            loopi(2)
            {
                glActiveTexture_(GL_TEXTURE0_ARB+i);
                resettmu(i);
                if(renderpath==R_FIXEDFUNCTION || !i)
                {
                    resettmu(i);
                    disabletexgen();
                }
                if(i) glDisable(GL_TEXTURE_2D);
            }
            glActiveTexture_(GL_TEXTURE0_ARB);
        }

        if(renderpath==R_FIXEDFUNCTION && fogpass && cur.fogtmu<0)
		{
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_TEXTURE_1D);
            setuptexgen(1);
            if(!fogtex) createfogtex();
            glBindTexture(GL_TEXTURE_1D, fogtex);
            setuptexgen(1);
            uchar wcol[3];
            getwatercolour(wcol);
            glColor3ubv(wcol);
            rendergeommultipass(cur, RENDERPASS_FOG, fogpass);
            disabletexgen(1);
            glDisable(GL_TEXTURE_1D);
            glEnable(GL_TEXTURE_2D);
        }

        glFogfv(GL_FOG_COLOR, oldfogc);
        glDisable(GL_BLEND);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
	}

	glPopMatrix();

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
}

void findreflectedvas(vector<vtxarray *> &vas, float z, bool refract, bool vfc = true)
{
	bool doOQ = hasOQ && oqfrags && oqreflect;
	loopv(vas)
	{
		vtxarray *va = vas[i];
		if(!vfc) va->curvfc = VFC_NOT_VISIBLE;
        if(va->curvfc == VFC_FOGGED || va->o.z+va->size <= z || isvisiblecube(va->o, va->size) >= VFC_FOGGED) continue;
		bool render = true;
		if(va->curvfc == VFC_FULL_VISIBLE)
		{
			if(va->occluded >= OCCLUDE_BB) continue;
			if(va->occluded >= OCCLUDE_GEOM) render = false;
		}
		if(render)
		{
			if(va->curvfc == VFC_NOT_VISIBLE) va->distance = (int)vadist(va, camera1->o);
			if(!doOQ && va->distance > reflectdist) continue;
			va->rquery = NULL;
			vtxarray **vprev = &reflectedva, *vcur = reflectedva;
			while(vcur && va->distance > vcur->distance)
			{
				vprev = &vcur->rnext;
				vcur = vcur->rnext;
			}
			va->rnext = *vprev;
			*vprev = va;
		}
        if(va->children.length()) findreflectedvas(va->children, z, refract, va->curvfc != VFC_NOT_VISIBLE);
	}
}

void renderreflectedgeom(float z, bool refract, bool causticspass, bool fogpass)
{
	if(!refract && camera1->o.z >= z)
	{
		reflectvfcP(z);
		reflectedva = NULL;
		findreflectedvas(varoot, z, refract);
        rendergeom(causticspass ? 1 : 0, fogpass);
		restorevfcP();
	}
    else rendergeom(causticspass ? 1 : 0, fogpass);
}

static vtxarray *prevskyva = NULL;

void renderskyva(vtxarray *va, bool explicitonly = false)
{
    if(!prevskyva || va->vbuf != prevskyva->vbuf)
    {
        setorigin(va);
        if(hasVBO)
        {
            glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbuf);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, va->skybuf);
        }
        glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &va->vdata[0].x);
    }

    drawvatris(va, explicitonly ? va->explicitsky : va->sky+va->explicitsky, explicitonly ? va->skydata+va->sky : va->skydata);

    if(!explicitonly) xtraverts += va->sky/3;
    xtraverts += va->explicitsky/3;

    prevskyva = va;
}

int renderreflectedskyvas(vector<vtxarray *> &vas, float z, bool vfc = true)
{
	int rendered = 0;
	loopv(vas)
	{
		vtxarray *va = vas[i];
		if((vfc && va->curvfc == VFC_FULL_VISIBLE) && va->occluded >= OCCLUDE_BB) continue;
        if(va->o.z+va->size <= z || isvisiblecube(va->o, va->size) == VFC_NOT_VISIBLE) continue;
        if(va->sky+va->explicitsky)
        {
            renderskyva(va);
            rendered++;
        }
        if(va->children.length()) rendered += renderreflectedskyvas(va->children, z, vfc && va->curvfc != VFC_NOT_VISIBLE);
    }
    return rendered;
}

bool rendersky(bool explicitonly, float zreflect)
{
    glEnableClientState(GL_VERTEX_ARRAY);

    glPushMatrix();

    resetorigin();

    prevskyva = NULL;

	int rendered = 0;
	if(zreflect)
	{
		reflectvfcP(zreflect);
		rendered = renderreflectedskyvas(varoot, zreflect);
		restorevfcP();
	}
	else for(vtxarray *va = visibleva; va; va = va->next)
	{
		if(va->occluded >= OCCLUDE_BB || !(explicitonly ? va->explicitsky : va->sky+va->explicitsky)) continue;

		renderskyva(va, explicitonly);
		rendered++;
	}

	glPopMatrix();

    if(prevskyva && hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }

	glDisableClientState(GL_VERTEX_ARRAY);

	return rendered>0;
}

