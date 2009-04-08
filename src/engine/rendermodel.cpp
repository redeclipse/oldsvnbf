#include "engine.h"

VARP(oqdynent, 0, 1, 1);
VARP(animationinterpolationtime, 0, 150, 1000);

model *loadingmodel = NULL;

#include "ragdoll.h"
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

#define checkragdoll \
    skelmodel *m = dynamic_cast<skelmodel *>(loadingmodel); \
    if(!m) { conoutf("\frnot loading a skeletal model"); return; } \
    skelmodel::skelmeshgroup *meshes = (skelmodel::skelmeshgroup *)m->parts.last()->meshes; \
    if(!meshes) return; \
    skelmodel::skeleton *skel = meshes->skel; \
    if(!skel->ragdoll) skel->ragdoll = new ragdollskel; \
    ragdollskel *ragdoll = skel->ragdoll; \
    if(ragdoll->loaded) return;


void rdvert(float *x, float *y, float *z)
{
    checkragdoll;
    ragdollskel::vert &v = ragdoll->verts.add();
    v.pos = vec(*x, *y, *z);
}
COMMAND(rdvert, "fff");

void rdeye(int *v)
{
    checkragdoll;
    ragdoll->eye = *v;
}
COMMAND(rdeye, "i");

void rdtri(int *v1, int *v2, int *v3)
{
    checkragdoll;
    ragdollskel::tri &t = ragdoll->tris.add();
    t.vert[0] = *v1;
    t.vert[1] = *v2;
    t.vert[2] = *v3;
}
COMMAND(rdtri, "iii");

void rdjoint(int *n, int *t, char *v1, char *v2, char *v3)
{
    checkragdoll;
    ragdollskel::joint &j = ragdoll->joints.add();
    j.bone = *n;
    j.tri = *t;
    j.vert[0] = v1[0] ? atoi(v1) : -1;
    j.vert[1] = v2[0] ? atoi(v2) : -1;
    j.vert[2] = v3[0] ? atoi(v3) : -1;
}
COMMAND(rdjoint, "iisss");

void rdlimitdist(int *v1, int *v2, float *mindist, float *maxdist)
{
    checkragdoll;
    ragdollskel::distlimit &d = ragdoll->distlimits.add();
    d.vert[0] = *v1;
    d.vert[1] = *v2;
    d.mindist = *mindist;
    d.maxdist = max(*maxdist, *mindist);
}
COMMAND(rdlimitdist, "iiff");

void rdlimitrot(int *t1, int *t2, float *maxangle, float *qx, float *qy, float *qz, float *qw)
{
    checkragdoll;
    ragdollskel::rotlimit &r = ragdoll->rotlimits.add();
    r.tri[0] = *t1;
    r.tri[1] = *t2;
    r.maxangle = *maxangle * RAD;
    r.middle = matrix3x3(quat(*qx, *qy, *qz, *qw));
}
COMMAND(rdlimitrot, "iifffff");

// mapmodels

vector<mapmodelinfo> mapmodels;

void mmodel(char *name)
{
	mapmodelinfo &mmi = mapmodels.add();
	copystring(mmi.name, name);
	mmi.m = NULL;
}

void mapmodelcompat(int *rad, int *h, int *tex, char *name, char *shadow)
{
	mmodel(name);
}

void resetmapmodels() { mapmodels.setsize(0); }

mapmodelinfo &getmminfo(int i) { return mapmodels.inrange(i) ? mapmodels[i] : *(mapmodelinfo *)0; }

COMMAND(mmodel, "s");
COMMANDN(mapmodel, mapmodelcompat, "iiiss");
ICOMMAND(mapmodelreset, "", (void), if(editmode || worldidents) resetmapmodels(););
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
			defformatstring(filename)("models/%s", name);
			renderprogress(loadprogress, filename);
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

void preloadmodelshaders()
{
    if(initing) return;
    enumerate(mdllookup, model *, m, m->preloadshaders());
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
    loopv(mapmodels) if(mapmodels[i].m==*m) mapmodels[i].m = NULL;
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

struct batchedmodel
{
	vec pos, color, dir;
	int anim;
	float yaw, pitch, roll;
	int basetime, basetime2, flags;
	dynent *d;
	int attached;
	occludequery *query;
};
struct modelbatch
{
	model *m;
    int flags;
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

modelbatch &addbatchedmodel(model *m)
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
        b->flags = 0;
        m->batch = numbatches++;
    }
    return *b;
}

void renderbatchedmodel(model *m, batchedmodel &b)
{
	modelattach *a = NULL;
	if(b.attached>=0) a = &modelattached[b.attached];

	int anim = b.anim;
    if(shadowmapping)
    {
        anim |= ANIM_NOSKIN;
    }
    else
    {
        if(b.flags&MDL_TRANSLUCENT) anim |= ANIM_TRANSLUCENT;
        if(b.flags&MDL_FULLBRIGHT) anim |= ANIM_FULLBRIGHT;
    }

	m->render(anim, b.basetime, b.basetime2, b.pos, b.yaw, b.pitch, b.roll, b.d, a, b.color, b.dir);
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
        if(b.flags&(MDL_SHADOW|MDL_DYNSHADOW))
        {
            vec center, bbradius;
            b.m->boundbox(0/*frame*/, center, bbradius); // FIXME
            loopvj(b.batched)
            {
                batchedmodel &bm = b.batched[j];
                if(bm.flags&(MDL_SHADOW|MDL_DYNSHADOW))
                    renderblob(bm.flags&MDL_DYNSHADOW ? BLOB_DYNAMIC : BLOB_STATIC, bm.d && bm.d->ragdoll ? bm.d->ragdoll->center : bm.pos, bm.d ? bm.d->radius : max(bbradius.x, bbradius.y), 1.0f);
            }
            flushblobs();
        }
		bool rendered = false;
		occludequery *query = NULL;
		loopvj(b.batched)
		{
			batchedmodel &bm = b.batched[j];
            if(bm.flags&MDL_CULL_VFC) continue;
			if(bm.query!=query)
			{
				if(query) endquery(query);
				query = bm.query;
                if(query) startquery(query);
			}
            if(bm.flags&MDL_TRANSLUCENT && (!query || query->owner==bm.d))
			{
				translucentmodel &tm = translucent.add();
				tm.m = b.m;
				tm.batched = &bm;
				tm.dist = camera1->o.dist(bm.d && bm.d->ragdoll ? bm.d->ragdoll->center : bm.pos);
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
    flushblobs();
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

void rendermodel(entitylight *light, const char *mdl, int anim, const vec &o, float yaw, float pitch, float roll, int flags, dynent *d, modelattach *a, int basetime, int basetime2)
{
    if(shadowmapping && !(flags&(MDL_SHADOW|MDL_DYNSHADOW))) return;
	model *m = loadmodel(mdl);
	if(!m) return;
    vec center, bbradius;
    float radius = 0;
    bool shadow = !shadowmap && !glaring && (flags&(MDL_SHADOW|MDL_DYNSHADOW)) && showblobs,
         doOQ = flags&MDL_CULL_QUERY && hasOQ && oqfrags && oqdynent;
    if(flags&(MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED|MDL_CULL_QUERY|MDL_SHADOW|MDL_DYNSHADOW))
	{
        m->boundbox(0/*frame*/, center, bbradius); // FIXME
        radius = bbradius.magnitude();
        if(d && d->ragdoll)
        {
            radius = max(radius, d->ragdoll->radius);
            center = d->ragdoll->center;
        }
        else
        {
            center.rotate_around_z((-180-yaw)*RAD);
            center.add(o);
        }
        if(flags&MDL_CULL_DIST && center.dist(camera1->o)/radius>maxmodelradiusdistance) return;
		if(flags&MDL_CULL_VFC)
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
                if(flags&MDL_CULL_OCCLUDED && d->occluded>=OCCLUDE_PARENT) return;
                if(doOQ && d->occluded+1>=OCCLUDE_BB && d->query && d->query->owner==d && checkquery(d->query)) return;
            }
            if(!addshadowmapcaster(center, radius, radius)) return;
		}
        else if(flags&MDL_CULL_OCCLUDED && modeloccluded(center, radius))
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

    if(flags&MDL_NORENDER) anim |= ANIM_NORENDER;
    else if(showboundingbox && !shadowmapping)
	{
		glPushMatrix();
		notextureshader->set();
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

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

		defaultshader->set();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glPopMatrix();
	}

    vec lightcolor(1, 1, 1), lightdir(0, 0, 1);
    if(!shadowmapping)
    {
        vec pos = o;
        if(d)
        {
            if(!reflecting && !refracting) d->occluded = OCCLUDE_NOTHING;
            if(!light) light = &d->light;
            if(flags&MDL_LIGHT && light->millis!=lastmillis)
            {
                if(d->ragdoll)
                {
                    pos = d->ragdoll->center;
                    pos.z += radius/2;
                }
                lightreaching(pos, light->color, light->dir);
                dynlightreaching(pos, light->color, light->dir);
                world::lighteffects(d, light->color, light->dir);
                light->millis = lastmillis;
            }
        }
        else if(flags&MDL_LIGHT)
        {
            if(!light)
            {
                lightreaching(pos, lightcolor, lightdir);
                dynlightreaching(pos, lightcolor, lightdir);
            }
            else if(light->millis!=lastmillis)
            {
                lightreaching(pos, light->color, light->dir);
                dynlightreaching(pos, light->color, light->dir);
                light->millis = lastmillis;
            }
        }
        if(light) { lightcolor = light->color; lightdir = light->dir; }
        if(flags&MDL_DYNLIGHT) dynlightreaching(pos, lightcolor, lightdir);
    }

	if(a) for(int i = 0; a[i].tag; i++)
	{
		if(a[i].name) a[i].m = loadmodel(a[i].name);
		//if(a[i].m && a[i].m->type()!=m->type()) a[i].m = NULL;
	}

    if(!d || reflecting || refracting || shadowmapping) doOQ = false;

	if(numbatches>=0)
	{
        modelbatch &mb = addbatchedmodel(m);
        batchedmodel &b = mb.batched.add();
        b.query = modelquery;
		b.pos = o;
        b.color = lightcolor;
        b.dir = lightdir;
		b.anim = anim;
		b.yaw = yaw;
		b.pitch = pitch;
		b.roll = roll;
		b.basetime = basetime;
        b.basetime2 = basetime2;
        b.flags = flags & ~(MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED);
        if(!shadow || reflecting || refracting>0)
        {
            b.flags &= ~(MDL_SHADOW|MDL_DYNSHADOW);
            if((flags&MDL_CULL_VFC) && refracting<0 && center.z-radius>=reflectz) b.flags |= MDL_CULL_VFC;
        }
        mb.flags |= b.flags;
		b.d = d;
		b.attached = a ? modelattached.length() : -1;
		if(a) for(int i = 0;; i++) { modelattached.add(a[i]); if(!a[i].tag) break; }
        if(doOQ) d->query = b.query = newquery(d);
		return;
	}

    if(shadow && !reflecting && refracting<=0)
    {
        renderblob(flags&MDL_DYNSHADOW ? BLOB_DYNAMIC : BLOB_STATIC, d && d->ragdoll ? center : o, d ? d->radius : max(bbradius.x, bbradius.y), 1.0f);
        flushblobs();
        if((flags&MDL_CULL_VFC) && refracting<0 && center.z-radius>=reflectz) return;
    }

	m->startrender();

    if(shadowmapping)
    {
        anim |= ANIM_NOSKIN;
    }
    else
    {
        if(flags&MDL_TRANSLUCENT) anim |= ANIM_TRANSLUCENT;
        if(flags&MDL_FULLBRIGHT) anim |= ANIM_FULLBRIGHT;
    }

    if(doOQ)
    {
        d->query = newquery(d);
        if(d->query) startquery(d->query);
    }

	m->render(anim, basetime, basetime2, o, yaw, pitch, roll, d, a, lightcolor, lightdir);

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
    formatstring(dirs[0])("models/%s/", dir);
    formatstring(dirs[1])("models/%s/", altdir);
    formatstring(dirs[2])("textures/");
    masks = notexture;

	#define tryload(tex, cmd, path) loopi(4) { if((tex = textureload(makerelpath(i < 3 ? dirs[i] : "", path, NULL, cmd), 0, true, false)) != NULL) break; }
    tryload(skin, NULL, "skin");
    tryload(masks, "<ffmask:25>", "masks");
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

