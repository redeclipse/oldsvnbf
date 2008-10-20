#include "pch.h"
#include "engine.h"

VARP(oqdynent, 0, 1, 1);
VARP(animationinterpolationtime, 0, 150, 1000);

model *loadingmodel = NULL;

#include "animmodel.h"
#include "vertmodel.h"
#include "skelmodel.h"
#include "md2.h"
#include "md3.h"
#include "md5.h"
#include "obj.h"

#define checkmdl if(!loadingmodel) { conoutf("\frnot loading a model"); return; }

void mdlcullface(int *cullface)
{
	checkmdl;
    loadingmodel->setcullface(*cullface!=0);
}

COMMAND(mdlcullface, "i");

void mdlcollide(int *collide)
{
	checkmdl;
	loadingmodel->collide = *collide!=0;
}

COMMAND(mdlcollide, "i");

void mdlellipsecollide(int *collide)
{
    checkmdl;
    loadingmodel->ellipsecollide = *collide!=0;
}

COMMAND(mdlellipsecollide, "i");

void mdlspec(int *percent)
{
	checkmdl;
	float spec = 1.0f;
	if(*percent>0) spec = *percent/100.0f;
	else if(*percent<0) spec = 0.0f;
	loadingmodel->setspec(spec);
}

COMMAND(mdlspec, "i");

void mdlambient(int *percent)
{
	checkmdl;
	float ambient = 0.3f;
	if(*percent>0) ambient = *percent/100.0f;
	else if(*percent<0) ambient = 0.0f;
	loadingmodel->setambient(ambient);
}

COMMAND(mdlambient, "i");

void mdlalphatest(float *cutoff)
{
	checkmdl;
    loadingmodel->setalphatest(max(0.0f, min(1.0f, *cutoff)));
}

COMMAND(mdlalphatest, "f");

void mdlalphablend(int *blend)
{
	checkmdl;
	loadingmodel->setalphablend(*blend!=0);
}

COMMAND(mdlalphablend, "i");

void mdlglow(int *percent)
{
	checkmdl;
	float glow = 3.0f;
	if(*percent>0) glow = *percent/100.0f;
	else if(*percent<0) glow = 0.0f;
	loadingmodel->setglow(glow);
}

COMMAND(mdlglow, "i");

void mdlglare(float *specglare, float *glowglare)
{
    checkmdl;
    loadingmodel->setglare(*specglare, *glowglare);
}

COMMAND(mdlglare, "ff");

void mdlenvmap(float *envmapmax, float *envmapmin, char *envmap)
{
	checkmdl;
	loadingmodel->setenvmap(*envmapmin, *envmapmax, envmap[0] ? cubemapload(envmap) : NULL);
}

COMMAND(mdlenvmap, "ffs");

void mdltranslucent(float *translucency)
{
	checkmdl;
	loadingmodel->settranslucency(*translucency);
}

COMMAND(mdltranslucent, "f");

void mdlfullbright(float *fullbright)
{
    checkmdl;
    loadingmodel->setfullbright(*fullbright);
}

COMMAND(mdlfullbright, "f");

void mdlshader(char *shader)
{
	checkmdl;
	loadingmodel->setshader(lookupshaderbyname(shader));
}

COMMAND(mdlshader, "s");

void mdlspin(float *rate)
{
	checkmdl;
	loadingmodel->spin = *rate;
}

COMMAND(mdlspin, "f");

void mdlscale(int *percent)
{
	checkmdl;
	float scale = 0.3f;
	if(*percent>0) scale = *percent/100.0f;
	else if(*percent<0) scale = 0.0f;
	loadingmodel->scale = scale;
}

COMMAND(mdlscale, "i");

void mdltrans(float *x, float *y, float *z)
{
	checkmdl;
	loadingmodel->translate = vec(*x, *y, *z);
}

COMMAND(mdltrans, "fff");

void mdlyaw(float *angle)
{
    checkmdl;
    loadingmodel->offsetyaw = *angle;
}

COMMAND(mdlyaw, "f");

void mdlpitch(float *angle)
{
    checkmdl;
    loadingmodel->offsetpitch = *angle;
}

COMMAND(mdlpitch, "f");

void mdlroll(float *angle)
{
    checkmdl;
    loadingmodel->offsetroll = *angle;
}

COMMAND(mdlroll, "f");

void mdlshadow(int *shadow)
{
	checkmdl;
	loadingmodel->shadow = *shadow!=0;
}

COMMAND(mdlshadow, "i");

void mdlbb(float *rad, float *h, float *height)
{
	checkmdl;
	loadingmodel->collideradius = *rad;
	loadingmodel->collideheight = *h;
	loadingmodel->height = *height;
}

COMMAND(mdlbb, "fff");

void mdlextendbb(float *x, float *y, float *z)
{
    checkmdl;
    loadingmodel->bbextend = vec(*x, *y, *z);
}

COMMAND(mdlextendbb, "fff");

void mdlname()
{
	checkmdl;
	result(loadingmodel->name());
}

COMMAND(mdlname, "");

// mapmodels

vector<mapmodelinfo> mapmodels;

void mmodel(char *name)
{
	mapmodelinfo &mmi = mapmodels.add();
	s_strcpy(mmi.name, name);
	mmi.m = NULL;
}

void mapmodelcompat(int *rad, int *h, int *tex, char *name, char *shadow)
{
	mmodel(name);
}

void mapmodelreset() { mapmodels.setsize(0); }

mapmodelinfo &getmminfo(int i) { return mapmodels.inrange(i) ? mapmodels[i] : *(mapmodelinfo *)0; }

COMMAND(mmodel, "s");
COMMANDN(mapmodel, mapmodelcompat, "iiiss");
COMMAND(mapmodelreset, "");
ICOMMAND(mapmodelindex, "s", (char *a), {
	if (!*a) intret(mapmodels.length());
	else
	{
		int num = atoi(a);
		if (mapmodels.inrange(num)) result(mapmodels[num].name);
	}
});

// model registry

hashtable<const char *, model *> mdllookup;

model *loadmodel(const char *name, int i, bool msg)
{
	if(!name)
	{
		if(!mapmodels.inrange(i)) return NULL;
		mapmodelinfo &mmi = mapmodels[i];
		if(mmi.m) return mmi.m;
		name = mmi.name;
	}
	model **mm = mdllookup.access(name);
	model *m;
	if(mm) m = *mm;
	else
	{
		if(msg)
		{
			s_sprintfd(filename)("models/%s", name);
			renderprogress(0, filename);
		}
		m = new md5(name);
		loadingmodel = m;
		if(!m->load())
		{
			delete m;
			m = new md3(name);
			loadingmodel = m;
			if(!m->load())
			{
                delete m;
                m = new md2(name);
                loadingmodel = m;
                if(!m->load())
                {
                    delete m;
                    m = new obj(name);
                    loadingmodel = m;
                    if(!m->load())
                    {
                        delete m;
                        loadingmodel = NULL;
                        return NULL;
                    }
                }
			}
		}
		loadingmodel = NULL;
		mdllookup.access(m->name(), m);
	}
	if(mapmodels.inrange(i) && !mapmodels[i].m) mapmodels[i].m = m;
	return m;
}

void clear_mdls()
{
	enumerate(mdllookup, model *, m, delete m);
}

void cleanupmodels()
{
    enumerate(mdllookup, model *, m, m->cleanup());
}

void clearmodel(char *name)
{
    model **m = mdllookup.access(name);
    if(!m) { conoutf("\frmodel %s is not loaded", name); return; }
    mdllookup.remove(name);
    (*m)->cleanup();
    delete *m;
    conoutf("\fgcleared model %s", name);
}

COMMAND(clearmodel, "s");

bool modeloccluded(const vec &center, float radius)
{
    int br = int(radius*2)+1;
    return pvsoccluded(ivec(int(center.x-radius), int(center.y-radius), int(center.z-radius)), ivec(br, br, br)) ||
           bboccluded(ivec(int(center.x-radius), int(center.y-radius), int(center.z-radius)), ivec(br, br, br));
}

VAR(showboundingbox, 0, 0, 2);

void render2dbox(vec &o, float x, float y, float z)
{
	glBegin(GL_LINE_LOOP);
	glVertex3f(o.x, o.y, o.z);
	glVertex3f(o.x, o.y, o.z+z);
	glVertex3f(o.x+x, o.y+y, o.z+z);
	glVertex3f(o.x+x, o.y+y, o.z);
	glEnd();
}

void render3dbox(vec &o, float tofloor, float toceil, float xradius, float yradius)
{
	if(yradius<=0) yradius = xradius;
	vec c = o;
	c.sub(vec(xradius, yradius, tofloor));
	float xsz = xradius*2, ysz = yradius*2;
	float h = tofloor+toceil;
	glColor3f(1, 1, 1);
	render2dbox(c, xsz, 0, h);
	render2dbox(c, 0, ysz, h);
	c.add(vec(xsz, ysz, 0));
	render2dbox(c, -xsz, 0, h);
	render2dbox(c, 0, -ysz, h);
	xtraverts += 16;
}

void renderellipse(vec &o, float xradius, float yradius, float yaw)
{
	glColor3f(0.5f, 0.5f, 0.5f);
	glBegin(GL_LINE_LOOP);
	loopi(16)
	{
		vec p(xradius*cosf(2*M_PI*i/16.0f), yradius*sinf(2*M_PI*i/16.0f), 0);
		p.rotate_around_z((yaw+90)*RAD);
		p.add(o);
		glVertex3fv(p.v);
	}
	glEnd();
}

void setshadowmatrix(const plane &p, const vec &dir)
{
	float d = p.dot(dir);
	GLfloat m[16] =
	{
		d-dir.x*p.x,		-dir.y*p.x,		-dir.z*p.x,	  0,
		 -dir.x*p.y,	  d-dir.y*p.y,		-dir.z*p.y,	  0,
		 -dir.x*p.z,		-dir.y*p.z,	  d-dir.z*p.z,	  0,
		 -dir.x*p.offset,  -dir.y*p.offset,  -dir.z*p.offset, d
	};
	glMultMatrixf(m);
}

VARP(bounddynshadows, 0, 1, 1);
VARP(dynshadow, 0, 60, 100);

void rendershadow(vec &dir, model *m, int anim, const vec &o, vec center, float radius, float yaw, float pitch, float roll, float speed, int basetime, dynent *d, int cull, modelattach *a)
{
	vec floor;
	float dist = rayfloor(center, floor, 0, center.z);
	if(dist<=0 || dist>=center.z) return;
	center.z -= dist;
    if((cull&MDL_CULL_VFC) && refracting<0 && center.z>=reflectz) return;
	if(vec(center).sub(camera1->o).dot(floor)>0) return;

    vec shaddir;
    if(cull&MDL_DYNSHADOW)
    {
        extern vec shadowdir;
        shaddir = shadowdir;
        shaddir.normalize();
    }
	else
	{
		shaddir = dir;
		shaddir.z = 0;
		if(!shaddir.iszero()) shaddir.normalize();
		shaddir.z = 1.5f*(dir.z*0.5f+1);
		shaddir.normalize();
	}

	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);

    if(!hasFBO || !reflecting || !refracting || hasDS)
    {
        glEnable(GL_STENCIL_TEST);

        if(bounddynshadows)
        {
            nocolorshader->set();
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
            glStencilFunc(GL_ALWAYS, 1, 1);
            glStencilOp(GL_KEEP, GL_REPLACE, GL_ZERO);

            vec below(center);
            below.z -= 1.0f;
            glPushMatrix();
            setshadowmatrix(plane(floor, -floor.dot(below)), shaddir);
            glBegin(GL_QUADS);
            loopi(6) if((shaddir[dimension(i)]>0)==dimcoord(i)) loopj(4)
            {
                const ivec &cc = cubecoords[fv[i][j]];
                glVertex3f(center.x + (cc.x ? 1.5f : -1.5f)*radius,
                           center.y + (cc.y ? 1.5f : -1.5f)*radius,
                           cc.z ? center.z + dist + radius : below.z);
                xtraverts += 4;
            }
            glEnd();
            glPopMatrix();

            glColorMask(COLORMASK, fading ? GL_FALSE : GL_TRUE);
        }
    }

    float intensity = dynshadow/100.0f;
    if(fogging)
    {
        if(renderpath!=R_FIXEDFUNCTION) setfogplane(0, max(0.1f, reflectz-center.z));
        else intensity *= 1.0f - max(0.0f, min(1.0f, (reflectz - center.z)/waterfog));
    }
    if((anim&ANIM_INDEX)==ANIM_DYING) intensity *= max(1.0f - (lastmillis - basetime)/1000.0f, 0.0f);
    glColor4f(0, 0, 0, intensity);

    if(!hasFBO || !reflecting || !refracting || hasDS)
    {
        glStencilFunc(GL_NOTEQUAL, bounddynshadows ? 0 : 1, 1);
        glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
    }

    vec above(center);
    above.z += 0.25f;

    glPushMatrix();
    setshadowmatrix(plane(floor, -floor.dot(above)), shaddir);
    m->render(anim|ANIM_NOSKIN|ANIM_SHADOW, speed, basetime, o, yaw, pitch, roll, d, a);
    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    glDepthMask(GL_TRUE);

    if(!hasFBO || !reflecting || !refracting || hasDS) glDisable(GL_STENCIL_TEST);
}

struct batchedmodel
{
	vec pos, color, dir;
	int anim;
	float yaw, pitch, roll, speed;
	int basetime, cull;
	dynent *d;
	int attached;
	occludequery *query;
};
struct modelbatch
{
	model *m;
	vector<batchedmodel> batched;
};
static vector<modelbatch *> batches;
static vector<modelattach> modelattached;
static int numbatches = -1;
static occludequery *modelquery = NULL;

void startmodelbatches()
{
	numbatches = 0;
	modelattached.setsizenodelete(0);
}

batchedmodel &addbatchedmodel(model *m)
{
	modelbatch *b = NULL;
	if(m->batch>=0 && m->batch<numbatches && batches[m->batch]->m==m) b = batches[m->batch];
	else
	{
		if(numbatches<batches.length())
		{
			b = batches[numbatches];
			b->batched.setsizenodelete(0);
		}
		else b = batches.add(new modelbatch);
		b->m = m;
		m->batch = numbatches++;
	}
	batchedmodel &bm = b->batched.add();
	bm.query = modelquery;
	return bm;
}

void renderbatchedmodel(model *m, batchedmodel &b)
{
	modelattach *a = NULL;
	if(b.attached>=0) a = &modelattached[b.attached];
    if(!shadowmap && !glaring && (b.cull&(MDL_SHADOW|MDL_DYNSHADOW)) && dynshadow && hasstencil && !reflecting && refracting<=0)
	{
		vec center;
        float radius = m->boundsphere(0/*frame*/, center); // FIXME
		center.add(b.pos);
		rendershadow(b.dir, m, b.anim, b.pos, center, radius, b.yaw, b.pitch, b.roll, b.speed, b.basetime, b.d, b.cull, a);
        if((b.cull&MDL_CULL_VFC) && refracting<0 && center.z-radius>=reflectz) return;
	}

	int anim = b.anim;
    if(shadowmapping)
    {
        anim |= ANIM_NOSKIN;
        if(renderpath!=R_FIXEDFUNCTION)
            setenvparamf("shadowintensity", SHPARAM_VERTEX, 1,
                (anim&ANIM_INDEX)==ANIM_DYING ? max(1.0f - (lastmillis - b.basetime)/1000.0f, 0.0f) : 1.0f);
    }
    else
    {
        if(b.cull&MDL_TRANSLUCENT) anim |= ANIM_TRANSLUCENT;
        if(b.cull&MDL_FULLBRIGHT) anim |= ANIM_FULLBRIGHT;
    }

	m->render(anim, b.speed, b.basetime, b.pos, b.yaw, b.pitch, b.roll, b.d, a, b.color, b.dir);
}

struct translucentmodel
{
	model *m;
	batchedmodel *batched;
	float dist;
};

static int sorttranslucentmodels(const translucentmodel *x, const translucentmodel *y)
{
	if(x->dist > y->dist) return -1;
	if(x->dist < y->dist) return 1;
	return 0;
}

void endmodelbatches()
{
	vector<translucentmodel> translucent;
	loopi(numbatches)
	{
		modelbatch &b = *batches[i];
		if(b.batched.empty()) continue;
		bool rendered = false;
		occludequery *query = NULL;
		loopvj(b.batched)
		{
			batchedmodel &bm = b.batched[j];
			if(bm.query!=query)
			{
				if(query) endquery(query);
				query = bm.query;
                if(query) startquery(query);
			}
            if(bm.cull&MDL_TRANSLUCENT && (!query || query->owner==bm.d))
			{
				translucentmodel &tm = translucent.add();
				tm.m = b.m;
				tm.batched = &bm;
				tm.dist = camera1->o.dist(bm.pos);
				continue;
			}
			if(!rendered) { b.m->startrender(); rendered = true; }
			renderbatchedmodel(b.m, bm);
		}
		if(query) endquery(query);
		if(rendered) b.m->endrender();
	}
	if(translucent.length())
	{
		translucent.sort(sorttranslucentmodels);
		model *lastmodel = NULL;
        occludequery *query = NULL;
		loopv(translucent)
		{
			translucentmodel &tm = translucent[i];
			if(lastmodel!=tm.m)
			{
				if(lastmodel) lastmodel->endrender();
				(lastmodel = tm.m)->startrender();
			}
            if(query!=tm.batched->query)
            {
                if(query) endquery(query);
                query = tm.batched->query;
                if(query) startquery(query);
            }
			renderbatchedmodel(tm.m, *tm.batched);
		}
        if(query) endquery(query);
		if(lastmodel) lastmodel->endrender();
	}
	numbatches = -1;
}

void startmodelquery(occludequery *query)
{
	modelquery = query;
}

void endmodelquery()
{
	int querybatches = 0;
	loopi(numbatches)
	{
		modelbatch &b = *batches[i];
		if(b.batched.empty() || b.batched.last().query!=modelquery) continue;
		querybatches++;
	}
	if(querybatches<=1)
	{
		if(!querybatches) modelquery->fragments = 0;
		modelquery = NULL;
		return;
	}
	int minattached = modelattached.length();
	startquery(modelquery);
	loopi(numbatches)
	{
		modelbatch &b = *batches[i];
		if(b.batched.empty() || b.batched.last().query!=modelquery) continue;
		b.m->startrender();
		do
		{
			batchedmodel &bm = b.batched.pop();
			if(bm.attached>=0) minattached = min(minattached, bm.attached);
			renderbatchedmodel(b.m, bm);
		}
		while(b.batched.length() && b.batched.last().query==modelquery);
		b.m->endrender();
	}
	endquery(modelquery);
	modelquery = NULL;
	modelattached.setsizenodelete(minattached);
}

VARP(maxmodelradiusdistance, 10, 200, 1000);

void rendermodelquery(model *m, dynent *d, const vec &center, float radius)
{
    if(fabs(camera1->o.x-center.x) < radius+1 &&
       fabs(camera1->o.y-center.y) < radius+1 &&
       fabs(camera1->o.z-center.z) < radius+1)
    {
        d->query = NULL;
        return;
    }
    d->query = newquery(d);
    if(!d->query) return;
    nocolorshader->set();
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    startquery(d->query);
    int br = int(radius*2)+1;
    drawbb(ivec(int(center.x-radius), int(center.y-radius), int(center.z-radius)), ivec(br, br, br));
    endquery(d->query);
    glColorMask(COLORMASK, fading ? GL_FALSE : GL_TRUE);
    glDepthMask(GL_TRUE);
}

extern int oqfrags;

void rendermodel(entitylight *light, const char *mdl, int anim, const vec &o, float yaw, float pitch, float roll, int cull, dynent *d, modelattach *a, int basetime, float speed)
{
    if(shadowmapping && !(cull&(MDL_SHADOW|MDL_DYNSHADOW))) return;
	model *m = loadmodel(mdl);
	if(!m) return;
	vec center;
	float radius = 0;
    bool shadow = !shadowmap && !glaring && (cull&(MDL_SHADOW|MDL_DYNSHADOW)) && dynshadow && hasstencil,
         doOQ = cull&MDL_CULL_QUERY && hasOQ && oqfrags && oqdynent;
    if(cull&(MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED|MDL_CULL_QUERY|MDL_SHADOW|MDL_DYNSHADOW))
	{
        radius = m->boundsphere(0/*frame*/, center); // FIXME
        center.rotate_around_z((-180-yaw)*RAD);
		center.add(o);
        if(cull&MDL_CULL_DIST && center.dist(camera1->o)/radius>maxmodelradiusdistance) return;
		if(cull&MDL_CULL_VFC)
		{
            if(reflecting || refracting)
            {
                if(reflecting || refracting>0)
                {
                    if(center.z+radius<=reflectz) return;
                }
                else
                {
                    if(fogging && center.z+radius<reflectz-waterfog) return;
                    if(!shadow && center.z-radius>=reflectz) return;
                }
                if(center.dist(camera1->o)-radius>reflectdist) return;
            }
			if(isvisiblesphere(radius, center) >= VFC_FOGGED) return;
            if(shadowmapping && !isshadowmapcaster(center, radius)) return;
		}
        if(shadowmapping)
        {
            if(d)
            {
                if(cull&MDL_CULL_OCCLUDED && d->occluded>=OCCLUDE_PARENT) return;
                if(doOQ && d->occluded+1>=OCCLUDE_BB && d->query && d->query->owner==d && checkquery(d->query)) return;
            }
            if(!addshadowmapcaster(center, radius, radius)) return;
		}
        else if(cull&MDL_CULL_OCCLUDED && modeloccluded(center, radius))
        {
            if(!reflecting && !refracting && d)
            {
                d->occluded = OCCLUDE_PARENT;
                if(doOQ) rendermodelquery(m, d, center, radius);
            }
            return;
        }
        else if(doOQ && d && d->query && d->query->owner==d && checkquery(d->query))
        {
            if(!reflecting && !refracting)
            {
                if(d->occluded<OCCLUDE_BB) d->occluded++;
                rendermodelquery(m, d, center, radius);
            }
            return;
        }
    }

    if(cull&MDL_NORENDER) anim |= ANIM_NORENDER;
    else if(showboundingbox && !shadowmapping)
	{
		renderprimitive(true);
		if(d && showboundingbox==1)
		{
			render3dbox(d->o, d->height, d->aboveeye, d->radius);
			renderellipse(d->o, d->xradius, d->yradius, d->yaw);
		}
		else
		{
			vec center, radius;
			if(showboundingbox==1) m->collisionbox(0, center, radius);
			else m->boundbox(0, center, radius);
			rotatebb(center, radius, int(yaw));
			center.add(o);
			render3dbox(center, radius.z, radius.z, radius.x, radius.y);
		}
		renderprimitive(false);
	}

    vec lightcolor(1, 1, 1), lightdir(0, 0, 1);
    if(!shadowmapping)
    {
        if(d)
		{
        	if(!reflecting && !refracting) d->occluded = OCCLUDE_NOTHING;
            if(!light) light = &d->light;
            if(cull&MDL_LIGHT && light->millis!=lastmillis)
            {
                lightreaching(d->o, light->color, light->dir);
                dynlightreaching(o, light->color, light->dir);
                cl->lighteffects(d, light->color, light->dir);
                light->millis = lastmillis;
            }
        }
        else if(cull&MDL_LIGHT)
        {
            if(!light)
            {
                lightreaching(o, lightcolor, lightdir);
                dynlightreaching(o, lightcolor, lightdir);
			}
            else if(light->millis!=lastmillis)
            {
                lightreaching(o, light->color, light->dir);
                dynlightreaching(o, light->color, light->dir);
                light->millis = lastmillis;
            }
        }
        if(light) { lightcolor = light->color; lightdir = light->dir; }
        if(cull&MDL_DYNLIGHT) dynlightreaching(o, lightcolor, lightdir);
    }

	if(a) for(int i = 0; a[i].tag; i++)
	{
		if(a[i].name) a[i].m = loadmodel(a[i].name);
        if(a[i].pos) anim |= ANIM_LOOKUP;
		//if(a[i].m && a[i].m->type()!=m->type()) a[i].m = NULL;
	}

    if(!d || reflecting || refracting || shadowmapping) doOQ = false;

	if(numbatches>=0)
	{
		batchedmodel &b = addbatchedmodel(m);
		b.pos = o;
        b.color = lightcolor;
        b.dir = lightdir;
		b.anim = anim;
		b.yaw = yaw;
		b.pitch = pitch;
		b.roll = roll;
		b.speed = speed;
		b.basetime = basetime;
		b.cull = cull;
		b.d = d;
		b.attached = a ? modelattached.length() : -1;
		if(a) for(int i = 0;; i++) { modelattached.add(a[i]); if(!a[i].tag) break; }
        if(doOQ) d->query = b.query = newquery(d);
		return;
	}

	m->startrender();

    if(shadow && !reflecting && refracting<=0)
	{
		rendershadow(lightdir, m, anim, o, center, radius, yaw, pitch, roll, speed, basetime, d, cull, a);
        if((cull&MDL_CULL_VFC) && refracting<0 && center.z-radius>=reflectz) { m->endrender(); return; }
	}

    if(shadowmapping)
    {
        anim |= ANIM_NOSKIN;
        if(renderpath!=R_FIXEDFUNCTION)
            setenvparamf("shadowintensity", SHPARAM_VERTEX, 1,
                (anim&ANIM_INDEX)==ANIM_DYING ? max(1.0f - (lastmillis - basetime)/1000.0f, 0.0f) : 1.0f);
    }
    else
    {
        if(cull&MDL_TRANSLUCENT) anim |= ANIM_TRANSLUCENT;
        if(cull&MDL_FULLBRIGHT) anim |= ANIM_FULLBRIGHT;
    }

    if(doOQ)
    {
        d->query = newquery(d);
        if(d->query) startquery(d->query);
    }

	m->render(anim, speed, basetime, o, yaw, pitch, roll, d, a, lightcolor, lightdir);

    if(doOQ && d->query) endquery(d->query);

	m->endrender();
}

void abovemodel(vec &o, const char *mdl)
{
	model *m = loadmodel(mdl);
	if(!m) return;
	o.z += m->above(0/*frame*/);
}

bool matchanim(const char *name, const char *pattern)
{
	for(;; pattern++)
	{
		const char *s = name;
		char c;
		for(;; pattern++)
		{
			c = *pattern;
			if(!c || c=='|') break;
			else if(c=='*')
			{
				if(!*s || isspace(*s)) break;
				do s++; while(*s && !isspace(*s));
			}
			else if(c!=*s) break;
			else s++;
		}
		if(!*s && (!c || c=='|')) return true;
		pattern = strchr(pattern, '|');
		if(!pattern) break;
	}
	return false;
}

void loadskin(const char *dir, const char *altdir, Texture *&skin, Texture *&masks) // model skin sharing
{
	string dirs[3];
    s_sprintf(dirs[0])("models/%s/", dir);
    s_sprintf(dirs[1])("models/%s/", altdir);
    s_sprintf(dirs[2])("textures/");
    masks = notexture;

	#define tryload(tex, path) loopi(4) { s_sprintfd(s)("%s%s", i < 3 ? dirs[i] : "", path); if((tex = textureload(s, 0, true, false)) != NULL) break; }
    tryload(skin, "skin");
    tryload(masks, "<ffmask:25>masks");
}

void setbbfrommodel(dynent *d, const char *mdl)
{
	model *m = loadmodel(mdl);
	if(!m) return;
	vec center, radius;
	m->collisionbox(0, center, radius);
    if(d->type==ENT_INANIMATE && !m->ellipsecollide)
    {
        d->collidetype = COLLIDE_AABB;
        rotatebb(center, radius, int(d->yaw));
    }
	d->xradius	= radius.x + fabs(center.x);
	d->yradius	= radius.y + fabs(center.y);
	d->radius	= max(d->xradius, d->yradius);
    if(d->collidetype!=COLLIDE_ELLIPSE) d->xradius = d->yradius = d->radius;
	d->height = (center.z-radius.z) + radius.z*2*m->height;
	d->aboveeye  = radius.z*2*(1.0f-m->height);
}

