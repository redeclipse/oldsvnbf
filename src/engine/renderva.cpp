// renderva.cpp: handles the occlusion and rendering of vertex arrays

#include "pch.h"
#include "engine.h"

void drawvatris(vtxarray *va, GLsizei numindices, const GLvoid *indices, ushort minvert = 0, ushort maxvert = 0)
{
	if(!minvert && !maxvert) { minvert = va->minvert; maxvert = va->maxvert; }
	if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, minvert, maxvert, numindices, GL_UNSIGNED_SHORT, indices);
	else glDrawElements(GL_TRIANGLES, numindices, GL_UNSIGNED_SHORT, indices);
	glde++;
}

///////// view frustrum culling ///////////////////////

plane vfcP[5];  // perpindictular vectors to view frustrum bounding planes
float vfcDfog;  // far plane culling distance (fog limit).
int vfcw, vfch, vfcfov;

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

int isvisiblecube(const vec &o, int size)
{
	vec center(o);
	center.add(size/2.0f);
	return isvisiblesphere(size*SQRT3/2.0f, center);
}

float vadist(vtxarray *va, const vec &p)
{
	if(va->min.x>va->max.x)
	{
		ivec o(va->x, va->y, va->z);
		return p.dist_to_bb(o, ivec(o).add(va->size)); // box contains only sky/water
	}
	return p.dist_to_bb(va->min, va->max);
}

#define VASORTSIZE 64

static vtxarray *vasort[VASORTSIZE];

void addvisibleva(vtxarray *va)
{
	extern int lodsize, loddistance;

	float dist = vadist(va, camera1->o);
	va->distance = int(dist); /*cv.dist(camera1->o) - va->size*SQRT3/2*/
	va->curlod	= lodsize==0 || va->distance<loddistance ? 0 : 1;

	int hash = min(int(dist*VASORTSIZE/hdr.worldsize), VASORTSIZE-1);
	vtxarray **prev = &vasort[hash], *cur = vasort[hash];

	while(cur && va->distance > cur->distance)
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
		v.curvfc = isvisiblecube(vec(v.x, v.y, v.z), v.size);
		if(v.curvfc!=VFC_NOT_VISIBLE)
		{
			addvisibleva(&v);
			if(v.children->length()) findvisiblevas(*v.children, prevvfc==VFC_NOT_VISIBLE);
			if(prevvfc==VFC_NOT_VISIBLE)
			{
				v.occluded = OCCLUDE_NOTHING;
				v.query = NULL;
			}
		}
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
}

extern vector<vtxarray *> varoot;

void visiblecubes(cube *c, int size, int cx, int cy, int cz, int w, int h, int fov)
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

bool insideva(const vtxarray *va, const vec &v)
{
	return v.x>=va->x && v.y>=va->y && v.z>=va->z && v.x<=va->x+va->size && v.y<=va->y+va->size && v.z<=va->z+va->size;
}

static ivec vaorigin;

void resetorigin()
{
	vaorigin = ivec(-1, -1, -1);
}

void setorigin(vtxarray *va, bool shadowmatrix = false)
{
	ivec o(va->x, va->y, va->z);
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
	}
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
		loopj(qf.max) glDeleteQueries_(1, &qf.queries[j].id);
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
			if(isvisiblecube(oe->o.tovec(), oe->size) >= VFC_FOGGED) continue;

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
					if(e.visible || (e.attr3 && e.triggerstate == TRIGGER_DISAPPEARED)) continue;
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
	if(e.attr3) switch(e.triggerstate)
	{
		case TRIGGER_RESET: anim = ANIM_TRIGGER|ANIM_START; break;
		case TRIGGERING: anim = ANIM_TRIGGER; basetime = e.lasttrigger; break;
		case TRIGGERED: anim = ANIM_TRIGGER|ANIM_END; break;
		case TRIGGER_RESETTING: anim = ANIM_TRIGGER|ANIM_REVERSE; basetime = e.lasttrigger; break;
	}
	mapmodelinfo &mmi = getmminfo(e.attr2);
	if(&mmi) rendermodel(e.color, e.dir, mmi.name, anim, 0, mmi.tex, e.o, (float)((e.attr1+7)-(e.attr1+7)%15), 0, 0, basetime, NULL, MDL_CULL_VFC | MDL_CULL_DIST);
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
		if(refract ? oe->o.z >= z : oe->o.z+oe->size <= z) continue;
		if(reflected && isvisiblecube(oe->o.tovec(), oe->size) >= VFC_FOGGED) continue;
		loopv(oe->mapmodels)
		{
			extentity &e = *ents[oe->mapmodels[i]];
			if(e.visible || (e.attr3 && e.triggerstate == TRIGGER_DISAPPEARED)) continue;
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
			if(!e.visible || (e.attr3 && e.triggerstate == TRIGGER_DISAPPEARED)) continue;
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
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, refracting && renderpath!=R_FIXEDFUNCTION ? GL_FALSE : GL_TRUE);
	}
}

bool bboccluded(const ivec &bo, const ivec &br, cube *c, const ivec &o, int size)
{
	loopoctabox(o, size, bo, br)
	{
		ivec co(i, o.x, o.y, o.z, size);
		if(c[i].ext && c[i].ext->va)
		{
			vtxarray *va = c[i].ext->va;
			if(va->curvfc >= VFC_FOGGED || va->occluded >= OCCLUDE_BB) continue;
		}
		if(c[i].children && bboccluded(bo, br, c[i].children, co, size>>1)) continue;
		return false;
	}
	return true;
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
	GLuint vbufGL = 0, ebufGL = 0;
	for(vtxarray *va = visibleva; va; va = va->next)
	{
		lodlevel &lod = va->curlod ? va->l1 : va->l0;
		if(!lod.texs || va->occluded >= OCCLUDE_GEOM) continue;

		setorigin(va);

		bool vbufchanged = true;
		if(hasVBO)
		{
			if(vbufGL == va->vbufGL) vbufchanged = false;
			else
			{
				glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbufGL);
				vbufGL = va->vbufGL;
			}
			if(ebufGL != lod.ebufGL)
			{
				glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, lod.ebufGL);
				ebufGL = lod.ebufGL;
			}
		}
		if(vbufchanged) glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &(va->vbuf[0].x));

		drawvatris(va, 3*lod.tris, lod.ebuf);
		xtravertsva += va->verts;
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

    extern int apple_minmax_bug;
    if(!apple_minmax_bug) glColorMask(GL_FALSE, GL_FALSE, GL_TRUE, GL_FALSE);

    glEnable(GL_BLEND);
    glBlendEquation_(GL_MAX_EXT);
    glBlendFunc(GL_ONE, GL_ONE);

    glPushMatrix();

    resetorigin();
    GLuint vbufGL = 0, ebufGL = 0;
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        lodlevel &lod = va->curlod ? va->l1 : va->l0;
        if(va->curvfc >= VFC_FOGGED || !isshadowmapreceiver(va)) continue;

        setorigin(va);

        bool vbufchanged = true;
        if(hasVBO)
        {
            if(vbufGL == va->vbufGL) vbufchanged = false;
            else
            {
                glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbufGL);
                vbufGL = va->vbufGL;
            }
            if(ebufGL != lod.ebufGL)
            {
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, lod.ebufGL);
                ebufGL = lod.ebufGL;
            }
        }
        if(vbufchanged) glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &(va->vbuf[0].x));

        drawvatris(va, 3*lod.tris, lod.ebuf);
        xtravertsva += va->verts;
    }

    glPopMatrix();

    glDisable(GL_BLEND);
    glBlendEquation_(GL_FUNC_ADD_EXT);

    glCullFace(GL_FRONT);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    if(!apple_minmax_bug) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if(hasVBO)
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
    glEnable(GL_TEXTURE_2D);
}

VARP(maxdynlights, 0, MAXDYNLIGHTS, MAXDYNLIGHTS);
VARP(dynlightdist, 0, 1024, 10000);

struct dynlight
{
	vec o;
	float radius, dist;
	vec color;
	int fade, peak, expire;

	float calcradius() const
	{
		return peak>0 && expire-lastmillis>fade ? (radius/peak)*(peak-(expire-lastmillis-fade)) : radius;
	}

	float intensity() const
	{
		if(fade + peak)
		{
			int remaining = expire - lastmillis;
			return remaining > fade ? 1.0f - float(remaining - fade)/peak : float(remaining)/fade;
		}
		return 1.0f;
	}
};

vector<dynlight> dynlights;
vector<dynlight *> closedynlights, visibledynlights;

void adddynlight(const vec &o, float radius, const vec &color, int fade, int peak)
{
	if(o.dist(camera1->o) > dynlightdist) return;

	int insert = 0, expire = fade + peak + lastmillis;
	loopvrev(dynlights) if(expire>=dynlights[i].expire) { insert = i+1; break; }
	dynlight d;
	d.o = o;
	d.radius = radius;
	d.color = color;
	d.fade = fade;
	d.peak = peak;
	d.expire = expire;
	dynlights.insert(insert, d);
}

void cleardynlights()
{
	int faded = -1;
	loopv(dynlights) if(lastmillis<dynlights[i].expire) { faded = i; break; }
	if(faded<0) dynlights.setsizenodelete(0);
	else if(faded>0) dynlights.remove(0, faded);
}

int limitdynlights()
{
	closedynlights.setsizenodelete(0);
	if(maxdynlights) loopvj(dynlights)
	{
		dynlight &d = dynlights[j];
		d.dist = camera1->o.dist(d.o) - d.calcradius();
		if(d.dist>dynlightdist || isvisiblesphere(d.radius, d.o) >= VFC_FOGGED) continue;
		int insert = 0;
		loopvrev(closedynlights) if(d.dist >= closedynlights[i]->dist) { insert = i+1; break; }
		if(closedynlights.length()>=maxdynlights)
		{
			if(insert+1>=maxdynlights) continue;
			closedynlights.drop();
		}
		closedynlights.insert(insert, &d);
	}
	return closedynlights.length();
}

void dynlightreaching(const vec &target, vec &color, vec &dir)
{
	vec dyncolor(0, 0, 0);//, dyndir(0, 0, 0);
	loopv(dynlights)
	{
		dynlight &d = dynlights[i];
		vec ray(d.o);
		ray.sub(target);
		float mag = ray.magnitude(), radius = d.calcradius();
		if(radius<=0 || mag >= radius) continue;
		float intensity = d.intensity()*(1 - mag/radius);
		dyncolor.add(vec(d.color).mul(intensity));
		//dyndir.add(ray.mul(intensity/mag));
	}
#if 0
	if(!dyndir.iszero())
	{
		dyndir.normalize();
		float x = dyncolor.magnitude(), y = color.magnitude();
		if(x+y>0)
		{
			dir.mul(x);
			dyndir.mul(y);
			dir.add(dyndir).div(x+y);
			if(dir.iszero()) dir = vec(0, 0, 1);
			else dir.normalize();
		}
	}
#endif
	color.add(dyncolor);
}

void setdynlights(vtxarray *va)
{
	visibledynlights.setsizenodelete(0);
	loopv(closedynlights)
	{
		dynlight &d = *closedynlights[i];
		if(d.o.dist_to_bb(va->min, va->max) < d.calcradius()) visibledynlights.add(&d);
	}
	if(visibledynlights.empty()) return;

	static string vertexparams[MAXDYNLIGHTS] = { "" }, pixelparams[MAXDYNLIGHTS] = { "" };
	if(!*vertexparams[0]) loopi(MAXDYNLIGHTS)
	{
		s_sprintf(vertexparams[i])("dynlight%dpos", i);
		s_sprintf(pixelparams[i])("dynlight%dcolor", i);
	}

	loopv(visibledynlights)
	{
		dynlight &d = *visibledynlights[i];
		setenvparamfv(vertexparams[i], SHPARAM_VERTEX, 10+i, vec4(d.o, 1).sub(ivec(va->x, va->y, va->z).mask(~VVEC_INT_MASK).tovec()).mul(1<<VVEC_FRAC).v);
		vec color(d.color);
		color.mul(2*d.intensity());
		float radius = d.calcradius();
		setenvparamf(pixelparams[i], SHPARAM_PIXEL, 10+i, color.x, color.y, color.z, -1.0f/(radius*radius*(1<<(2*VVEC_FRAC))));
	}
}

float orientation_tangent [3][4] = { {  0,1, 0,0 }, { 1,0, 0,0 }, { 1,0,0,0 }};
float orientation_binormal[3][4] = { {  0,0,-1,0 }, { 0,0,-1,0 }, { 0,1,0,0 }};

struct renderstate
{
	bool colormask, depthmask, texture;
	GLuint vbufGL, ebufGL;
	float fogplane;
    int diffusetmu, lightmaptmu, glowtmu, fogtmu, causticstmu;
    GLfloat color[4];

    renderstate() : colormask(true), depthmask(true), texture(true), vbufGL(0), ebufGL(0), fogplane(-1), diffusetmu(0), lightmaptmu(1), glowtmu(-1), fogtmu(-1), causticstmu(-1)
	{
        loopk(4) color[k] = 1;
	}
};

void renderquery(renderstate &cur, occludequery *query, vtxarray *va)
{
	setorigin(va);
	nocolorshader->set();
	if(cur.colormask) { cur.colormask = false; glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); }
	if(cur.depthmask) { cur.depthmask = false; glDepthMask(GL_FALSE); }

	startquery(query);

	ivec origin(va->x, va->y, va->z);
	origin.mask(~VVEC_INT_MASK);

	vec camera(camera1->o);
	if(reflecting && !refracting) camera.z = reflecting;

	ivec bbmin, bbmax;
	if(va->children || va->mapmodels || va->l0.matsurfs || va->l0.sky || va->l0.explicitsky)
	{
		bbmin = ivec(va->x, va->y, va->z);
		bbmax = ivec(va->size, va->size, va->size);
	}
	else
	{
		bbmin = va->min;
		bbmax = va->max;
		bbmax.sub(bbmin);
	}

	drawbb(bbmin.sub(origin).mul(1<<VVEC_FRAC),
			bbmax.mul(1<<VVEC_FRAC),
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

void renderva(renderstate &cur, vtxarray *va, lodlevel &lod, int pass = RENDERPASS_LIGHTMAP, bool fogpass = false)
{
	if(pass==RENDERPASS_GLOW)
	{
		bool noglow = true;
		loopi(lod.texs)
		{
			Slot &slot = lookuptexture(lod.eslist[i].texture);
			loopvj(slot.sts)
			{
				Slot::Tex &t = slot.sts[j];
				if(t.type==TEX_GLOW && t.combined<0) noglow = false;
			}
		}
		if(noglow) return;
	}

    setorigin(va, pass==RENDERPASS_LIGHTMAP && !envmapping);
	bool vbufchanged = true;
	if(hasVBO)
	{
		if(cur.vbufGL == va->vbufGL) vbufchanged = false;
		else
		{
			glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbufGL);
			cur.vbufGL = va->vbufGL;
		}
		if(cur.ebufGL != lod.ebufGL)
		{
			glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, lod.ebufGL);
			cur.ebufGL = lod.ebufGL;
		}
	}
	if(vbufchanged) glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &(va->vbuf[0].x));
	if(!cur.depthmask) { cur.depthmask = true; glDepthMask(GL_TRUE); }

	if(pass==RENDERPASS_Z)
	{
		if(cur.colormask) { cur.colormask = false; glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); }
		extern int apple_glsldepth_bug;
		if(renderpath!=R_GLSLANG || !apple_glsldepth_bug)
		{
			nocolorshader->set();
			drawvatris(va, 3*lod.tris, lod.ebuf);
		}
		else
		{
			static Shader *nocolorglslshader = NULL;
			if(!nocolorglslshader) nocolorglslshader = lookupshaderbyname("nocolorglsl");
			Slot *lastslot = NULL;
			int lastdraw = 0, offset = 0;
			loopi(lod.texs)
			{
				Slot &slot = lookuptexture(lod.eslist[i].texture);
				if(lastslot && (slot.shader->type&SHADER_GLSLANG) != (lastslot->shader->type&SHADER_GLSLANG) && offset > lastdraw)
				{
					(lastslot->shader->type&SHADER_GLSLANG ? nocolorglslshader : nocolorshader)->set();
					drawvatris(va, offset-lastdraw, lod.ebuf+lastdraw);
					lastdraw = offset;
				}
				lastslot = &slot;
                offset += lod.eslist[i].length[5];
			}
			if(offset > lastdraw)
			{
				(lastslot->shader->type&SHADER_GLSLANG ? nocolorglslshader : nocolorshader)->set();
				drawvatris(va, offset-lastdraw, lod.ebuf+lastdraw);
			}
		}
		xtravertsva += va->verts;
		return;
	}

	if(refracting && renderpath!=R_FIXEDFUNCTION)
	{
		float fogplane = refracting - (va->z & ~VVEC_INT_MASK);
		if(cur.fogplane!=fogplane)
		{
			cur.fogplane = fogplane;
			setfogplane(1.0f/(1<<VVEC_FRAC), fogplane);
		}
	}
	if(!cur.colormask) { cur.colormask = true; glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); }

    if((pass==RENDERPASS_LIGHTMAP || pass==RENDERPASS_COLOR) && (fogpass ? va->z+va->size<=refracting-waterfog : va->curvfc==VFC_FOGGED))
	{
		// this case is never reached if called from rendergeommultipass()
		static Shader *fogshader = NULL;
		if(!fogshader) fogshader = lookupshaderbyname("fogworld");
		fogshader->set();

		if(cur.texture)
		{
			cur.texture = false;
			glDisable(GL_TEXTURE_2D);
            if(fogpass && cur.fogtmu<0)
            {
                uchar wcol[3];
                getwatercolour(wcol);
                glColor3ubv(wcol);
            }
			if(pass==RENDERPASS_LIGHTMAP)
			{
				if(renderpath!=R_FIXEDFUNCTION) glDisableClientState(GL_COLOR_ARRAY);
                glActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
                glClientActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
				glDisableClientState(GL_TEXTURE_COORD_ARRAY);
				glDisable(GL_TEXTURE_2D);
                if(cur.fogtmu>=0)
                {
                    glActiveTexture_(GL_TEXTURE0_ARB+cur.fogtmu);
                    glDisable(GL_TEXTURE_1D);
                }
                if(cur.causticstmu>=0) loopi(2)
                {
                    glActiveTexture_(GL_TEXTURE0_ARB+cur.causticstmu+i);
                    glDisable(GL_TEXTURE_2D);
                }
                glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
                glClientActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
			}
		}

		drawvatris(va, 3*lod.tris, lod.ebuf);
		vtris += lod.tris;
		vverts += va->verts;
		return;
	}

	if(!cur.texture)
	{
		cur.texture = true;
        if(pass==RENDERPASS_LIGHTMAP || pass==RENDERPASS_COLOR)
        {
		glEnable(GL_TEXTURE_2D);
            if(fogpass && cur.fogtmu<0) glColor4fv(cur.color);
        }
		if(pass==RENDERPASS_LIGHTMAP)
		{
			if(renderpath!=R_FIXEDFUNCTION) glEnableClientState(GL_COLOR_ARRAY);
            glActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
            glClientActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glEnable(GL_TEXTURE_2D);
            if(cur.fogtmu>=0)
            {
                glActiveTexture_(GL_TEXTURE0_ARB+cur.fogtmu);
                glEnable(GL_TEXTURE_1D);
            }
            if(cur.causticstmu>=0) loopi(2)
            {
                glActiveTexture_(GL_TEXTURE0_ARB+cur.causticstmu+i);
                glEnable(GL_TEXTURE_2D);
            }
            glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
            glClientActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
			vbufchanged = true;
		}
	}

    if(pass==RENDERPASS_FOG || (cur.fogtmu>=0 && (pass==RENDERPASS_LIGHTMAP || pass==RENDERPASS_GLOW)))
    {
        if(pass==RENDERPASS_LIGHTMAP) glActiveTexture_(GL_TEXTURE0_ARB+cur.fogtmu);
        else if(pass==RENDERPASS_GLOW) glActiveTexture_(GL_TEXTURE1_ARB);
        GLfloat s[4] = { 0, 0, -1.0f/(waterfog<<VVEC_FRAC), (refracting - (va->z & ~VVEC_INT_MASK))/waterfog };
        glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
        if(pass==RENDERPASS_LIGHTMAP) glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        else if(pass==RENDERPASS_GLOW) glActiveTexture_(GL_TEXTURE0_ARB);
    }

    bool shadowmapreceiver = false;
    if(pass==RENDERPASS_CAUSTICS || pass==RENDERPASS_FOG)
    {
        drawvatris(va, 3*lod.tris, lod.ebuf);
        xtravertsva += va->verts;
        return;
    }
    else if(pass==RENDERPASS_LIGHTMAP)
    {
        if(renderpath!=R_FIXEDFUNCTION)
	{
        if(!envmapping && !va->curlod) shadowmapreceiver = isshadowmapreceiver(va);
        if(vbufchanged) glColorPointer(3, GL_UNSIGNED_BYTE, VTXSIZE, floatvtx ? &(((fvertex *)va->vbuf)[0].n) : &(va->vbuf[0].n));
		setenvparamfv("camera", SHPARAM_VERTEX, 4, vec4(camera1->o, 1).sub(ivec(va->x, va->y, va->z).mask(~VVEC_INT_MASK).tovec()).mul(1<<VVEC_FRAC).v);

		setdynlights(va);
	}
    if(vbufchanged)
	{
            glClientActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
			glTexCoordPointer(2, GL_SHORT, VTXSIZE, floatvtx ? &(((fvertex *)va->vbuf)[0].u) : &(va->vbuf[0].u));
            glClientActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
        }
	}

	ushort *ebuf = lod.ebuf;
    int lastlm = -1, lastxs = -1, lastys = -1, lastl = -1, lasto = -1, lastenvmap = -1, envmapped = 0;
    bool mtglow = false, shadowmapped = false;
	float lastscale = -1;
	Slot *lastslot = NULL;
    Shader *lastshader = NULL;
	loopi(lod.texs)
	{
        const elementset &es = lod.eslist[i];
        Slot &slot = lookuptexture(es.texture);
		Texture *tex = slot.sts[0].t;
		Shader *s = slot.shader;

		extern vector<GLuint> lmtexids;
        int lmid = es.lmid, curlm = pass==RENDERPASS_LIGHTMAP ? (int)lmtexids[lmid] : -1;
		if(s && renderpath!=R_FIXEDFUNCTION && pass==RENDERPASS_LIGHTMAP)
		{
            int tmu = cur.lightmaptmu+1;
			if(s->type&SHADER_NORMALSLMS)
			{
				if((!lastslot || s->type!=lastslot->shader->type || curlm!=lastlm) && (lmid<LMID_RESERVED || lightmaps[lmid-LMID_RESERVED].type==LM_BUMPMAP0))
				{
					glActiveTexture_(GL_TEXTURE0_ARB+tmu);
					glBindTexture(GL_TEXTURE_2D, lmtexids[lmid+1]);
				}
				tmu++;
			}
			if(s->type&SHADER_ENVMAP)
			{
                int envmap = es.envmap;
				if((!lastslot || s->type!=lastslot->shader->type || (envmap==EMID_CUSTOM ? &slot!=lastslot : envmap!=lastenvmap)) && hasCM)
				{
					glActiveTexture_(GL_TEXTURE0_ARB+tmu);
					if(!(envmapped & (1<<tmu)))
					{
						glEnable(GL_TEXTURE_CUBE_MAP_ARB);
						envmapped |= 1<<tmu;
					}
					GLuint emtex = 0;
					if(envmap==EMID_CUSTOM) loopvj(slot.sts)
					{
						Slot::Tex &t = slot.sts[j];
						if(t.type==TEX_ENVMAP && t.t) { emtex = t.t->gl; break; }
					}
					if(!emtex) emtex = lookupenvmap(envmap);
					glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, emtex);
					lastenvmap = envmap;
				}
				tmu++;
			}
			glActiveTexture_(GL_TEXTURE0_ARB);
		}
		if(curlm!=lastlm && pass==RENDERPASS_LIGHTMAP)
		{
            glActiveTexture_(GL_TEXTURE0_ARB+cur.lightmaptmu);
			glBindTexture(GL_TEXTURE_2D, curlm);
			lastlm = curlm;
            glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
		}
		if(&slot!=lastslot)
		{
			if(pass==RENDERPASS_LIGHTMAP || pass==RENDERPASS_COLOR) glBindTexture(GL_TEXTURE_2D, tex->gl);
			if(renderpath==R_FIXEDFUNCTION)
			{
				bool noglow = true;
                if(pass==RENDERPASS_GLOW || cur.glowtmu>=0) loopvj(slot.sts)
				{
					Slot::Tex &t = slot.sts[j];
					if(t.type==TEX_GLOW && t.combined<0)
					{
                        if(cur.glowtmu>=0)
						{
                            glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
							if(!mtglow) { glEnable(GL_TEXTURE_2D); mtglow = true; }
						}
						glBindTexture(GL_TEXTURE_2D, t.t->gl);
						noglow = false;
					}
				}
				if(pass==RENDERPASS_GLOW && noglow)
				{
                    ebuf += es.length[5];
					continue;
				}
				else if(mtglow)
				{
                    if(noglow) { glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu); glDisable(GL_TEXTURE_2D); mtglow = false; }
                    glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);
				}
			}
			else if(pass==RENDERPASS_LIGHTMAP && s)
			{
                int tmu = cur.lightmaptmu+1;
				if(s->type&SHADER_NORMALSLMS) tmu++;
				if(s->type&SHADER_ENVMAP) tmu++;
				loopvj(slot.sts)
				{
					Slot::Tex &t = slot.sts[j];
					if(t.type==TEX_DIFFUSE || t.type==TEX_ENVMAP || t.combined>=0) continue;
					glActiveTexture_(GL_TEXTURE0_ARB+tmu++);
					glBindTexture(GL_TEXTURE_2D, t.t->gl);
				}
                glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);

                lastshader = NULL;
			}
			lastslot = &slot;
		}

		float scale = slot.sts[0].scale;
		if(!scale) scale = 1;
        loopk(shadowmapreceiver ? 2 : 1) loopl(3)
		{
            int dim = 2*l+k;
            ushort offset = dim>0 ? es.length[dim-1] : 0,
                   len = es.length[dim + (shadowmapreceiver ? 0 : 1)] - offset;
            if(!len) continue;

            if(pass==RENDERPASS_LIGHTMAP && s && (lastshader!=s || shadowmapped!=(k!=0)))
            {
                if(refracting && hasFBO && waterrefract && waterfade)
                {
                    if(k) s->variant(min(s->variants[3].length()-1, visibledynlights.length()), 3)->set(&slot);
                    else s->variant(min(s->variants[2].length()-1, visibledynlights.length()), 2)->set(&slot);
                }
                else if(k) s->variant(min(s->variants[1].length()-1, visibledynlights.length()), 1)->set(&slot);
                else if(visibledynlights.empty()) s->set(&slot);
                else s->variant(min(s->variants[0].length()-1, visibledynlights.length()-1))->set(&slot);
                shadowmapped = k!=0;
                lastshader = s;
            }

			if(lastl!=l || lastxs!=tex->xs || lastys!=tex->ys || lastscale!=scale || (s && s->type&SHADER_GLSLANG))
			{
                static const int si[] = { 1, 0, 0 };
                static const int ti[] = { 2, 2, 1 };

                GLfloat sgen[] = { 0.0f, 0.0f, 0.0f, 0.0f };
                sgen[si[l]] = 8.0f/scale/(tex->xs<<VVEC_FRAC);
                GLfloat tgen[] = { 0.0f, 0.0f, 0.0f, 0.0f };
                tgen[ti[l]] = (l <= 1 ? -8.0f : 8.0f)/scale/(tex->ys<<VVEC_FRAC);

				if(renderpath==R_FIXEDFUNCTION)
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

					if(mtglow)
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
                }

				lastxs = tex->xs;
				lastys = tex->ys;
				lastl = l;
				lastscale = scale;
			}

            if(renderpath!=R_FIXEDFUNCTION && pass==RENDERPASS_LIGHTMAP && s && s->type&SHADER_NORMALSLMS && (lasto!=l || s->type&SHADER_GLSLANG))
            {
                setlocalparamfv("orienttangent", SHPARAM_VERTEX, 2, orientation_tangent[l]);
                setlocalparamfv("orientbinormal", SHPARAM_VERTEX, 3, orientation_binormal[l]);
                lasto = l;
            }

            ushort minvert = es.minvert[dim], maxvert = es.maxvert[dim];
            if(!shadowmapreceiver) { minvert = min(minvert, es.minvert[dim+1]); maxvert = max(maxvert, es.maxvert[dim+1]); }
            drawvatris(va, len, &ebuf[offset], minvert, maxvert);
		}
        ebuf += es.length[5];
	}

	if(mtglow)
	{
        glActiveTexture_(GL_TEXTURE0_ARB+cur.glowtmu);
		glDisable(GL_TEXTURE_2D);
	}
	if(envmapped)
	{
		loopi(4) if(envmapped&(1<<i))
		{
			glActiveTexture_(GL_TEXTURE0_ARB+i);
			glDisable(GL_TEXTURE_CUBE_MAP_ARB);
		}
	}
    if(mtglow || envmapped) glActiveTexture_(GL_TEXTURE0_ARB+cur.diffusetmu);

	vtris += lod.tris;
	vverts += va->verts;
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
		if(ati_texgen_bug) glEnable(GL_TEXTURE_GEN_R);	 // should not be needed, but apparently makes some ATI drivers happy
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
    glGenTextures(1, &fogtex);
    uchar buf[4] = { 255, 0, 255, 255 };
    createtexture(fogtex, 2, 1, buf, 3, false, GL_LUMINANCE_ALPHA, GL_TEXTURE_1D);
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
        s_sprintfd(name)("<mad:0.6,0.4>packages/caustics/caust%.2d.png", i);
        caustictex[i] = textureload(name);
    }
}

void setupcaustics(int tmu, GLfloat *color = NULL)
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
        glBindTexture(GL_TEXTURE_2D, caustictex[(tex+i)%NUMCAUSTICS]->gl);
        if(renderpath==R_FIXEDFUNCTION || !i)
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
        setlocalparamf("frameoffset", SHPARAM_PIXEL, 0, frac, frac, frac);
    }
}

void setupTMUs(renderstate &cur, bool causticspass, bool fogpass)
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
            if(maxtmus>=4 && causticspass)
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
            else if(fogpass && !causticspass) cur.fogtmu = 2;
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
        if(cur.causticstmu>=0) setupcaustics(cur.causticstmu, cur.color);
	}
    else
	{
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
    cur.vbufGL = cur.ebufGL = 0;
	for(vtxarray *va = FIRSTVA; va; va = NEXTVA)
	{
		lodlevel &lod = va->curlod ? va->l1 : va->l0;
		if(!lod.texs || va->occluded >= OCCLUDE_GEOM) continue;
		if(refracting || (reflecting && camera1->o.z < reflecting))
		{
			if(va->curvfc == VFC_FOGGED || (refracting && camera1->o.z >= refracting ? va->min.z > refracting : va->max.z <= refracting)) continue;
			if((!hasOQ || !oqfrags) && va->distance > reflectdist) break;
		}
		else if(reflecting)
		{
			if(va->max.z <= reflecting || (va->rquery && checkquery(va->rquery))) continue;
			if(va->rquery && checkquery(va->rquery)) continue;
		}
        if(fogpass ? va->z+va->size<=refracting-waterfog : va->curvfc==VFC_FOGGED) continue;
        renderva(cur, va, lod, pass, fogpass);
	}
}

void rendergeom(bool causticspass, bool fogpass)
{
    renderstate cur;

    if(causticspass && ((renderpath==R_FIXEDFUNCTION && maxtmus<2) || !causticscale || !causticmillis)) causticspass = false;

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

	limitdynlights();

	glPushMatrix();

	resetorigin();

	for(vtxarray *va = FIRSTVA; va; va = NEXTVA)
	{
		lodlevel &lod = va->curlod ? va->l1 : va->l0;
		if(!lod.texs) continue;
		if(refracting || (reflecting && camera1->o.z < reflecting))
		{
			if(va->curvfc == VFC_FOGGED || (refracting && camera1->o.z >= refracting ? va->min.z > refracting : va->max.z <= reflecting) || va->occluded >= OCCLUDE_GEOM) continue;
			if((!hasOQ || !oqfrags) && va->distance > reflectdist) break;
		}
		else if(reflecting)
		{
			if(va->max.z <= reflecting) continue;
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
		else if(hasOQ && oqfrags && (zpass || va->distance > oqdist) && !insideva(va, camera1->o))
		{
			if(!zpass && va->query && va->query->owner == va)
				va->occluded = checkquery(va->query) ? min(va->occluded+1, OCCLUDE_BB) : OCCLUDE_NOTHING;
			if(zpass && va->parent &&
				(va->parent->occluded == OCCLUDE_PARENT ||
				(va->parent->occluded >= OCCLUDE_BB &&
				 va->parent->query && va->parent->query->owner == va->parent && va->parent->query->fragments < 0)))
			{
				va->query = NULL;
				if(va->occluded >= OCCLUDE_GEOM)
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
			va->occluded = OCCLUDE_NOTHING;
		}

		if(!refracting)
		{
			if(!reflecting) { if(va->query) startquery(va->query); }
			else if(camera1->o.z >= reflecting && va->rquery) startquery(va->rquery);
		}

        renderva(cur, va, lod, doOQ ? RENDERPASS_Z : (nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP), fogpass);

		if(!refracting)
		{
			if(!reflecting) { if(va->query) endquery(va->query); }
			else if(camera1->o.z >= reflecting && va->rquery) endquery(va->rquery);
		}
	}

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
        cur.vbufGL = cur.ebufGL = 0;
        cur.texture = 0;
		for(vtxarray **prevva = &FIRSTVA, *va = FIRSTVA; va; prevva = &NEXTVA, va = NEXTVA)
		{
			lodlevel &lod = va->curlod ? va->l1 : va->l0;
			if(!lod.texs) continue;
			if(reflecting)
			{
				if(va->max.z <= reflecting) continue;
				if(va->rquery && checkquery(va->rquery))
				{
					if(va->occluded >= OCCLUDE_BB || va->curvfc == VFC_NOT_VISIBLE) *prevva = va->rnext;
					continue;
				}
			}
			else if(va->parent && va->parent->occluded >= OCCLUDE_BB && (!va->parent->query || va->parent->query->fragments >= 0))
			{
				va->query = NULL;
				va->occluded = OCCLUDE_BB;
				continue;
			}
			else if(va->query)
			{
				va->occluded = checkquery(va->query) ? min(va->occluded+1, OCCLUDE_BB) : OCCLUDE_NOTHING;
				if(va->occluded >= OCCLUDE_GEOM) continue;
			}
			else if(va->occluded == OCCLUDE_PARENT) va->occluded = OCCLUDE_NOTHING;

            renderva(cur, va, lod, nolights ? RENDERPASS_COLOR : RENDERPASS_LIGHTMAP, fogpass);
				}
		glDepthFunc(GL_LESS);
	}

    if(shadowmap) popshadowmap();

    cleanupTMUs(cur);

    if((causticspass && cur.causticstmu<0) || (renderpath==R_FIXEDFUNCTION && ((glowpass && cur.glowtmu<0) || (fogpass && cur.fogtmu<0))))
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

        if(causticspass && cur.causticstmu<0)
        {
            setupcaustics(0);
            glBlendFunc(GL_ZERO, GL_SRC_COLOR);
            glFogfv(GL_FOG_COLOR, onefog);
            if(renderpath!=R_FIXEDFUNCTION && refracting) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
            rendergeommultipass(cur, RENDERPASS_CAUSTICS, fogpass);
            if(renderpath!=R_FIXEDFUNCTION && refracting) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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
		if(va->curvfc == VFC_FOGGED || va->z+va->size <= z || isvisiblecube(vec(va->x, va->y, va->z), va->size) >= VFC_FOGGED) continue;
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
		if(va->children->length()) findreflectedvas(*va->children, z, refract, va->curvfc != VFC_NOT_VISIBLE);
	}
}

void renderreflectedgeom(float z, bool refract, bool causticspass, bool fogpass)
{
	if(!refract && camera1->o.z >= z)
	{
		reflectvfcP(z);
		reflectedva = NULL;
		findreflectedvas(varoot, z, refract);
        rendergeom(causticspass, fogpass);
		restorevfcP();
	}
    else rendergeom(causticspass, fogpass);
}

static GLuint skyvbufGL, skyebufGL;

void renderskyva(vtxarray *va, lodlevel &lod, bool explicitonly = false)
{
	setorigin(va);

	bool vbufchanged = true;
	if(hasVBO)
	{
		if(skyvbufGL == va->vbufGL) vbufchanged = false;
		else
		{
			glBindBuffer_(GL_ARRAY_BUFFER_ARB, va->vbufGL);
			skyvbufGL = va->vbufGL;
		}
		if(skyebufGL != lod.skybufGL)
		{
			glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, lod.skybufGL);
			skyebufGL = lod.skybufGL;
		}
	}
	if(vbufchanged) glVertexPointer(3, floatvtx ? GL_FLOAT : GL_SHORT, VTXSIZE, &(va->vbuf[0].x));

	drawvatris(va, explicitonly ? lod.explicitsky : lod.sky+lod.explicitsky, explicitonly ? lod.skybuf+lod.sky : lod.skybuf);

	if(!explicitonly) xtraverts += lod.sky/3;
	xtraverts += lod.explicitsky/3;
}

int renderreflectedskyvas(vector<vtxarray *> &vas, float z, bool vfc = true)
{
	int rendered = 0;
	loopv(vas)
	{
		vtxarray *va = vas[i];
		lodlevel &lod = va->curlod ? va->l1 : va->l0;
		if((vfc && va->curvfc == VFC_FULL_VISIBLE) && va->occluded >= OCCLUDE_BB) continue;
		if(va->z+va->size <= z || isvisiblecube(vec(va->x, va->y, va->z), va->size) == VFC_NOT_VISIBLE) continue;
		if(lod.sky+lod.explicitsky)
		{
			renderskyva(va, lod);
			rendered++;
		}
		if(va->children->length()) rendered += renderreflectedskyvas(*va->children, z, vfc && va->curvfc != VFC_NOT_VISIBLE);
	}
	return rendered;
}

bool rendersky(bool explicitonly, float zreflect)
{
	glEnableClientState(GL_VERTEX_ARRAY);

	glPushMatrix();

	resetorigin();

	skyvbufGL = skyebufGL = 0;

	int rendered = 0;
	if(zreflect)
	{
		reflectvfcP(zreflect);
		rendered = renderreflectedskyvas(varoot, zreflect);
		restorevfcP();
	}
	else for(vtxarray *va = visibleva; va; va = va->next)
	{
		lodlevel &lod = va->curlod ? va->l1 : va->l0;
		if(va->occluded >= OCCLUDE_BB || !(explicitonly ? lod.explicitsky : lod.sky+lod.explicitsky)) continue;

		renderskyva(va, lod, explicitonly);
		rendered++;
	}

	glPopMatrix();

	if(skyvbufGL) glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
	if(skyebufGL) glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);

	glDisableClientState(GL_VERTEX_ARRAY);

	return rendered>0;
}

