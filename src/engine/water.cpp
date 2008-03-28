#include "pch.h"
#include "engine.h"

VARFP(waterreflect, 0, 1, 1, cleanreflections());
VARFP(waterrefract, 0, 1, 1, cleanreflections());
VARFP(waterenvmap, 0, 1, 1, cleanreflections());
VARFP(waterfallrefract, 0, 0, 1, cleanreflections());
VARP(refractfog, 0, 1, 1);

/* vertex water */
VARP(watersubdiv, 0, 2, 3);
VARP(waterlod, 0, 1, 3);

static int wx1, wy1, wx2, wy2, wsize;
float wcol[4];

#define VERTW(vertw, body) \
	inline void vertw(float v1, float v2, float v3, float t) \
	{ \
		float angle = (v1-wx1)/wsize*(v2-wy1)/wsize*(v1-wx2)*(v2-wy2)*59/23+t; \
        float s = sinf(angle), h = WATER_AMPLITUDE*s-WATER_OFFSET; \
		body; \
		glVertex3f(v1, v2, v3+h); \
	}
#define VERTWN(vertw, body) \
	inline void vertw(float v1, float v2, float v3) \
	{ \
        float h = -WATER_OFFSET; \
		body; \
		glVertex3f(v1, v2, v3+h); \
	}
#define VERTWT(vertwt, body) VERTW(vertwt, { float v = cosf(angle); float duv = 0.5f*v; body; })
VERTW(vertwt, {
	glTexCoord2f(v1/8.0f, v2/8.0f);
})
VERTWN(vertwtn, {
	glTexCoord2f(v1/8.0f, v2/8.0f);
})
VERTW(vertwc, {
	glColor4f(wcol[0], wcol[1], wcol[2], wcol[3] + fabs(s)*0.1f);
})
VERTWN(vertwcn, {
	glColor4f(wcol[0], wcol[1], wcol[2], wcol[3]);
})
VERTWT(vertwtc, {
	glColor4f(1, 1, 1, 0.2f + fabs(s)*0.1f);
	glTexCoord3f(v1+duv, v2+duv, v3+h);
})
VERTWN(vertwtcn, {
	glColor4f(1, 1, 1, 0.2f);
	glTexCoord3f(v1, v2, v3+h);
})
VERTWT(vertwmtc, {
	glColor4f(1, 1, 1, 0.2f + fabs(s)*0.1f);
	glMultiTexCoord3f_(GL_TEXTURE0_ARB, v1-duv, v2+duv, v3+h);
	glMultiTexCoord3f_(GL_TEXTURE1_ARB, v1+duv, v2+duv, v3+h);
})
VERTWN(vertwmtcn, {
	glColor4f(1, 1, 1, 0.2f);
	glMultiTexCoord3f_(GL_TEXTURE0_ARB, v1, v2, v3+h);
	glMultiTexCoord3f_(GL_TEXTURE1_ARB, v1, v2, v3+h);
})
VERTWT(vertwetc, {
    glColor4f(1, 1, 1, 0.2f + fabs(s)*0.1f);
    glTexCoord3f(v1+duv-camera1->o.x, v2+duv-camera1->o.y, camera1->o.z-(v3+h));
})
VERTWN(vertwetcn, {
    glColor4f(1, 1, 1, 0.2f);
    glTexCoord3f(v1-camera1->o.x, v2-camera1->o.y, camera1->o.z-(v3+h));
})
VERTWT(vertwemtc, {
    glColor4f(1, 1, 1, 0.2f + fabs(s)*0.1f);
    glMultiTexCoord3f_(GL_TEXTURE0_ARB, v1-duv, v2+duv, v3+h);
    glMultiTexCoord3f_(GL_TEXTURE1_ARB, v1+duv-camera1->o.x, v2+duv-camera1->o.y, camera1->o.z-(v3+h));
})
VERTWN(vertwemtcn, {
    glColor4f(1, 1, 1, 0.2f);
    glMultiTexCoord3f_(GL_TEXTURE0_ARB, v1, v2, v3+h);
    glMultiTexCoord3f_(GL_TEXTURE1_ARB, v1-camera1->o.x, v2-camera1->o.y, camera1->o.z-(v3+h));
})

static float lavaxk = 1.0f, lavayk = 1.0f, lavascroll = 0.0f;

VERTW(vertl, {
	glTexCoord2f(lavaxk*(v1+lavascroll), lavayk*(v2+lavascroll));
})
VERTWN(vertln, {
	glTexCoord2f(lavaxk*(v1+lavascroll), lavayk*(v2+lavascroll));
})

#define renderwaterstrips(vertw, z, t) \
	for(int x = wx1; x<wx2; x += subdiv) \
	{ \
		glBegin(GL_TRIANGLE_STRIP); \
		vertw(x,		wy1, z, t); \
		vertw(x+subdiv, wy1, z, t); \
		for(int y = wy1; y<wy2; y += subdiv) \
		{ \
			vertw(x,		y+subdiv, z, t); \
			vertw(x+subdiv, y+subdiv, z, t); \
		} \
		glEnd(); \
		int n = (wy2-wy1-1)/subdiv; \
		n = (n+2)*2; \
		xtraverts += n; \
	}

void rendervertwater(uint subdiv, int xo, int yo, int z, uint size, uchar mat = MAT_WATER)
{
	wx1 = xo;
	wy1 = yo;
	wx2 = wx1 + size,
	wy2 = wy1 + size;
	wsize = size;

	ASSERT((wx1 & (subdiv - 1)) == 0);
	ASSERT((wy1 & (subdiv - 1)) == 0);

	switch(mat)
	{
		case MAT_WATER:
		{
			float t = lastmillis/(renderpath!=R_FIXEDFUNCTION ? 600.0f : 300.0f);
			if(renderpath!=R_FIXEDFUNCTION) { renderwaterstrips(vertwt, z, t); }
            else
            {
                bool below = camera1->o.z < z-WATER_OFFSET;
                if(nowater) { renderwaterstrips(vertwc, z, t); }
                else if(waterrefract)
                {
                    if(waterreflect && !below) { renderwaterstrips(vertwmtc, z, t); }
                    else if(waterenvmap && hasCM && !below) { renderwaterstrips(vertwemtc, z, t); }
                    else { renderwaterstrips(vertwtc, z, t); }
                }
                else if(waterreflect && !below) { renderwaterstrips(vertwtc, z, t); }
                else if(waterenvmap && hasCM && !below) { renderwaterstrips(vertwetc, z, t); }
                else { renderwaterstrips(vertwc, z, t); }
            }
			break;
		}

		case MAT_LAVA:
		{
			float t = lastmillis/2000.0f;
			renderwaterstrips(vertl, z, t);
			break;
		}
	}
}

uint calcwatersubdiv(int x, int y, int z, uint size)
{
	float dist;
	if(camera1->o.x >= x && camera1->o.x < x + size &&
		camera1->o.y >= y && camera1->o.y < y + size)
		dist = fabs(camera1->o.z - float(z));
	else
	{
		vec t(x + size/2, y + size/2, z + size/2);
		dist = t.dist(camera1->o) - size*1.42f/2;
	}
	uint subdiv = watersubdiv + int(dist) / (32 << waterlod);
	if(subdiv >= 8*sizeof(subdiv))
		subdiv = ~0;
	else
		subdiv = 1 << subdiv;
	return subdiv;
}

uint renderwaterlod(int x, int y, int z, uint size, uchar mat = MAT_WATER)
{
	if(size <= (uint)(32 << waterlod))
	{
		uint subdiv = calcwatersubdiv(x, y, z, size);
		if(subdiv < size * 2) rendervertwater(min(subdiv, size), x, y, z, size, mat);
		return subdiv;
	}
	else
	{
		uint subdiv = calcwatersubdiv(x, y, z, size);
		if(subdiv >= size)
		{
			if(subdiv < size * 2) rendervertwater(size, x, y, z, size, mat);
			return subdiv;
		}
		uint childsize = size / 2,
			 subdiv1 = renderwaterlod(x, y, z, childsize, mat),
			 subdiv2 = renderwaterlod(x + childsize, y, z, childsize, mat),
			 subdiv3 = renderwaterlod(x + childsize, y + childsize, z, childsize, mat),
			 subdiv4 = renderwaterlod(x, y + childsize, z, childsize, mat),
			 minsubdiv = subdiv1;
		minsubdiv = min(minsubdiv, subdiv2);
		minsubdiv = min(minsubdiv, subdiv3);
		minsubdiv = min(minsubdiv, subdiv4);
		if(minsubdiv < size * 2)
		{
			if(minsubdiv >= size) rendervertwater(size, x, y, z, size, mat);
			else
			{
				if(subdiv1 >= size) rendervertwater(childsize, x, y, z, childsize, mat);
				if(subdiv2 >= size) rendervertwater(childsize, x + childsize, y, z, childsize, mat);
				if(subdiv3 >= size) rendervertwater(childsize, x + childsize, y + childsize, z, childsize, mat);
				if(subdiv4 >= size) rendervertwater(childsize, x, y + childsize, z, childsize, mat);
			}
		}
		return minsubdiv;
	}
}

#define renderwaterquad(vertwn, z) \
	{ \
		vertwn(x, y, z); \
		vertwn(x+rsize, y, z); \
		vertwn(x+rsize, y+csize, z); \
		vertwn(x, y+csize, z); \
		xtraverts += 4; \
	}

void renderflatwater(int x, int y, int z, uint rsize, uint csize, uchar mat = MAT_WATER)
{
	switch(mat)
	{
		case MAT_WATER:
			if(renderpath!=R_FIXEDFUNCTION) { renderwaterquad(vertwtn, z); }
            else
            {
                bool below = camera1->o.z < z-WATER_OFFSET;
                if(nowater) { renderwaterquad(vertwcn, z); }
                else if(waterrefract)
                {
                    if(waterreflect && !below) { renderwaterquad(vertwmtcn, z); }
                    else if(waterenvmap && hasCM && !below) { renderwaterquad(vertwemtcn, z); }
                    else { renderwaterquad(vertwtcn, z); }
                }
                else if(waterreflect && !below) { renderwaterquad(vertwtcn, z); }
                else if(waterenvmap && hasCM && !below) { renderwaterquad(vertwetcn, z); }
                else { renderwaterquad(vertwcn, z); }
            }
			break;

		case MAT_LAVA:
			renderwaterquad(vertln, z);
			break;
	}
}

VARFP(vertwater, 0, 1, 1, allchanged());

void renderlava(materialsurface &m, Texture *tex, float scale)
{
	lavaxk = 8.0f/(tex->xs*scale);
	lavayk = 8.0f/(tex->ys*scale);
	lavascroll = lastmillis/1000.0f;
	if(vertwater)
	{
		if(renderwaterlod(m.o.x, m.o.y, m.o.z, m.csize, MAT_LAVA) >= (uint)m.csize * 2)
			rendervertwater(m.csize, m.o.x, m.o.y, m.o.z, m.csize, MAT_LAVA);
	}
	else renderflatwater(m.o.x, m.o.y, m.o.z, m.rsize, m.csize, MAT_LAVA);
}

/* reflective/refractive water */

#define MAXREFLECTIONS 16

struct Reflection
{
	GLuint fb, refractfb;
	GLuint tex, refracttex;
	int height, lastupdate, lastused;
	GLfloat tm[16];
	occludequery *query;
	vector<materialsurface *> matsurfs;

	Reflection() : fb(0), refractfb(0), height(-1), lastused(0), query(NULL)
	{}
};
Reflection *findreflection(int height);

VARP(reflectdist, 0, 2000, 10000);
VARW(waterfog, 0, 150, 10000);
VARW(watercolour, 0, 0x103060, 0xFFFFFF);
VARW(waterfallcolour, 0, 0, 0xFFFFFF);
void getwatercolour(uchar *wcol)
{
	uchar gcol[3] = { watercolour>>16, (watercolour>>8)&0xFF, watercolour&0xFF };
	memcpy(wcol, gcol, 3);
}
void getwaterfallcolour(uchar *fcol)
{
    int colour = waterfallcolour;
    if(!colour) colour = watercolour;
    uchar gcol[3] = { colour>>16, (colour>>8)&0xFF, colour&0xFF };
    memcpy(fcol, gcol, 3);
}
VARW(lavafog, 0, 50, 10000);
VARW(lavacolour, 0, 0xFF4400, 0xFFFFFF);
void getlavacolour(uchar *lcol)
{
	uchar gcol[3] = { lavacolour>>16, (lavacolour>>8)&0xFF, lavacolour&0xFF };
	memcpy(lcol, gcol, 3);
}

void setprojtexmatrix(Reflection &ref, bool init = true)
{
	if(init && ref.lastupdate==totalmillis)
	{
		GLfloat pm[16], mm[16];
		glGetFloatv(GL_PROJECTION_MATRIX, pm);
		glGetFloatv(GL_MODELVIEW_MATRIX, mm);

		glLoadIdentity();
        glTranslatef(0.5f, 0.5f, 0);
        glScalef(0.5f, 0.5f, 1);
		glMultMatrixf(pm);
		glMultMatrixf(mm);

		glGetFloatv(GL_TEXTURE_MATRIX, ref.tm);
	}
	else glLoadMatrixf(ref.tm);
}

void setuprefractTMUs()
{
    if(!refractfog) setuptmu(0, "K , T @ Ka");

    if(waterreflect || (waterenvmap && hasCM))
    {
        glActiveTexture_(GL_TEXTURE1_ARB);
        glEnable(waterreflect ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_ARB);
        if(!waterreflect) glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, lookupenvmap(lookuptexture(-MAT_WATER)));

        setuptmu(1, "P , T @ C~a");

        glActiveTexture_(GL_TEXTURE0_ARB);
    }
}

void setupreflectTMUs()
{
	setuptmu(0, "T , K @ Ca", "Ka * P~a");

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_SRC_ALPHA);

    if(!waterreflect)
    {
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, lookupenvmap(lookuptexture(-MAT_WATER)));
    }
}

void cleanupwaterTMUs(bool refract)
{
	resettmu(0);

    if(refract)
    {
        if(waterrefract || (waterenvmap && hasCM))
        {
            glActiveTexture_(GL_TEXTURE1_ARB);
            resettmu(1);
            glLoadIdentity();
            glDisable(waterreflect ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_ARB);
            glActiveTexture_(GL_TEXTURE0_ARB);
        }
    }
    else
    {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
}

VARW(waterspec, 0, 150, 1000);

Reflection reflections[MAXREFLECTIONS];
Reflection waterfallrefraction;
GLuint reflectiondb = 0;

GLuint getwaterfalltex() { return waterfallrefraction.refracttex ? waterfallrefraction.refracttex : notexture->id; }

VAR(oqwater, 0, 1, 1);

extern int oqfrags;

void renderwaterff()
{
	glDisable(GL_CULL_FACE);

    if(!nowater && (waterreflect || waterrefract || (waterenvmap && hasCM)))
    {
        if(waterrefract) setuprefractTMUs();
        else setupreflectTMUs();

        glMatrixMode(GL_TEXTURE);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    float offset = -WATER_OFFSET;

	uchar wcolub[3];
	getwatercolour(wcolub);
	loopi(3) wcol[i] = wcolub[i]/255.0f;

    bool wasbelow = false;
	loopi(MAXREFLECTIONS)
	{
		Reflection &ref = reflections[i];
		if(ref.height<0 || ref.lastused<totalmillis || ref.matsurfs.empty()) continue;

        bool below = camera1->o.z < ref.height + offset;
        if(!nowater && (waterrefract || waterreflect || (waterenvmap && hasCM)))
        {
            if(hasOQ && oqfrags && oqwater && ref.query && ref.query->owner==&ref && checkquery(ref.query)) continue;
            bool projtex = false;
            if(waterreflect || (waterenvmap && hasCM))
            {
                bool tmu1 = waterrefract && (!below || !wasbelow);
                if(tmu1) glActiveTexture_(GL_TEXTURE1_ARB);
                if(!below)
                {
                    if(wasbelow)
                    {
                        wasbelow = false;
                        glEnable(waterreflect ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_ARB);
                        if(!waterrefract) glBlendFunc(GL_ONE, GL_SRC_ALPHA);
                    }
                    if(waterreflect)
                    {
                        glBindTexture(GL_TEXTURE_2D, ref.tex);
                        setprojtexmatrix(ref);
                        projtex = true;
                    }
                }
                else if(!wasbelow)
                {
                    wasbelow = true;
                    glDisable(waterreflect ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_ARB);
                    if(!waterrefract) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                }
                if(tmu1) glActiveTexture_(GL_TEXTURE0_ARB);
            }
            if(waterrefract)
            {
                glBindTexture(GL_TEXTURE_2D, ref.refracttex);
                setprojtexmatrix(ref, !projtex);
            }
        }

        int lastdepth = -1;
        bool begin = false;
        loopvj(ref.matsurfs)
        {
            materialsurface &m = *ref.matsurfs[j];

            if(m.depth!=lastdepth)
            {
                float depth = !waterfog ? 1.0f : min(0.75f*m.depth/waterfog, 0.95f);
                if(nowater || !waterrefract) depth = max(depth, nowater || (!waterreflect && (!waterenvmap || !hasCM)) || below ? 0.6f : 0.3f);
                wcol[3] = depth;
                if(!nowater && (waterrefract || ((waterreflect || (waterenvmap && hasCM)) && !below)))
                {
                    if(begin) { glEnd(); begin = false; }
                    float ec[4] = { wcol[0], wcol[1], wcol[2], depth };
                    if(!waterrefract) { loopk(3) ec[k] *= depth; ec[3] = 1-ec[3]; }
                    colortmu(0, ec[0], ec[1], ec[2], ec[3]);
                }
                lastdepth = m.depth;
            }

            if(!vertwater)
            {
                if(!begin) { glBegin(GL_QUADS); begin = true; }
                renderflatwater(m.o.x, m.o.y, m.o.z, m.rsize, m.csize);
            }
            else if(renderwaterlod(m.o.x, m.o.y, m.o.z, m.csize) >= (uint)m.csize * 2)
                rendervertwater(m.csize, m.o.x, m.o.y, m.o.z, m.csize);
        }
		if(begin) glEnd();
	}

    if(!nowater && (waterrefract || waterreflect || (waterenvmap && hasCM)))
    {
        if(!waterrefract && (wasbelow || !waterreflect))
        {
            if(!waterreflect && !wasbelow) glDisable(GL_TEXTURE_CUBE_MAP_ARB);
            glEnable(GL_TEXTURE_2D);
        }
        cleanupwaterTMUs(waterrefract!=0);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
    }
    else
	{
		glEnable(GL_TEXTURE_2D);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	glEnable(GL_CULL_FACE);
}

VARFP(waterfade, 0, 1, 1, cleanreflections());

void renderwater()
{
	if(editmode && showmat) return;
	if(!rplanes) return;

	if(renderpath==R_FIXEDFUNCTION) { renderwaterff(); return; }

	glDisable(GL_CULL_FACE);

	uchar wcol[3] = { watercolour>>16, (watercolour>>8)&0xFF, watercolour&0xFF };
	glColor3ubv(wcol);

	Slot &s = lookuptexture(-MAT_WATER);

	glActiveTexture_(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s.sts.length() > 2 ? s.sts[2].t->id : notexture->id);
	glActiveTexture_(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s.sts.length() > 3 ? s.sts[3].t->id : notexture->id);

    if(!glaring)
    {
        if(waterrefract)
        {
            glActiveTexture_(GL_TEXTURE3_ARB);
            glEnable(GL_TEXTURE_2D);
            if(waterfade && hasFBO)
            {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
        }
        else
        {
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_SRC_ALPHA);
        }
    }
	glActiveTexture_(GL_TEXTURE0_ARB);

    if(!glaring && waterenvmap && !waterreflect && hasCM)
    {
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
        glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, lookupenvmap(s));
    }

	setenvparamf("camera", SHPARAM_VERTEX, 0, camera1->o.x, camera1->o.y, camera1->o.z);
	setenvparamf("millis", SHPARAM_VERTEX, 1, lastmillis/1000.0f, lastmillis/1000.0f, lastmillis/1000.0f);

    #define SETWATERSHADER(which, name) \
    do { \
        static Shader *name##shader = NULL; \
        if(!name##shader) name##shader = lookupshaderbyname(#name); \
        which##shader = name##shader; \
    } while(0)

    Shader *aboveshader = NULL;
    if(glaring) SETWATERSHADER(above, waterglare);
    else if(waterenvmap && !waterreflect && hasCM)
    {
        if(waterrefract)
        {
            if(waterfade && hasFBO) SETWATERSHADER(above, waterenvfade);
            else SETWATERSHADER(above, waterenvrefract);
        }
        else SETWATERSHADER(above, waterenv);
    }
    else if(waterrefract)
    {
        if(waterfade && hasFBO) SETWATERSHADER(above, waterfade);
        else SETWATERSHADER(above, waterrefract);
    }
    else if(waterreflect) SETWATERSHADER(above, waterreflect);
    else SETWATERSHADER(above, water);

    Shader *belowshader = NULL;
    if(!glaring)
    {
        if(waterrefract)
        {
            if(waterfade && hasFBO) SETWATERSHADER(below, underwaterfade);
            else SETWATERSHADER(below, underwaterrefract);
        }
        else SETWATERSHADER(below, underwater);

        if(waterreflect || waterrefract) glMatrixMode(GL_TEXTURE);
    }

	int sky[3] = { skylight>>16, (skylight>>8)&0xFF, skylight&0xFF };
	vec amb(max(sky[0], ambient), max(sky[1], ambient), max(sky[2], ambient));
    float offset = -WATER_OFFSET;
	loopi(MAXREFLECTIONS)
	{
		Reflection &ref = reflections[i];
		if(ref.height<0 || ref.lastused<totalmillis || ref.matsurfs.empty()) continue;
        if(!glaring && hasOQ && oqfrags && oqwater && ref.query && ref.query->owner==&ref && checkquery(ref.query)) continue;

        bool below = camera1->o.z < ref.height+offset;
        if(below)
        {
            if(!belowshader) continue;
            belowshader->set();
        }
        else aboveshader->set();

        if(!glaring)
        {
            if(waterreflect || waterrefract)
            {
                if(waterreflect || !waterenvmap || !hasCM) glBindTexture(GL_TEXTURE_2D, waterreflect ? ref.tex : ref.refracttex);
                setprojtexmatrix(ref);
            }

            if(waterrefract)
            {
                glActiveTexture_(GL_TEXTURE3_ARB);
                glBindTexture(GL_TEXTURE_2D, ref.refracttex);
                glActiveTexture_(GL_TEXTURE0_ARB);
                if(waterfade)
                {
                    float fadeheight = ref.height+offset+(below ? -2 : 2);
                    setlocalparamf("waterheight", SHPARAM_VERTEX, 7, fadeheight, fadeheight, fadeheight);
                }
            }
        }

        entity *lastlight = (entity *)-1;
        int lastdepth = -1;
		bool begin = false;
		loopvj(ref.matsurfs)
		{
			materialsurface &m = *ref.matsurfs[j];

			entity *light = (m.light && m.light->type==ET_LIGHT ? m.light : NULL);
			if(light!=lastlight)
			{
				if(begin) { glEnd(); begin = false; }
				const vec &lightpos = light ? light->o : vec(hdr.worldsize/2, hdr.worldsize/2, hdr.worldsize);
				float lightrad = light && light->attr1 ? light->attr1 : hdr.worldsize*8.0f;
				const vec &lightcol = (light ? vec(light->attr2, light->attr3, light->attr4) : vec(amb)).div(255.0f).mul(waterspec/100.0f);
				setlocalparamf("lightpos", SHPARAM_VERTEX, 2, lightpos.x, lightpos.y, lightpos.z);
				setlocalparamf("lightcolor", SHPARAM_PIXEL, 3, lightcol.x, lightcol.y, lightcol.z);
				setlocalparamf("lightradius", SHPARAM_PIXEL, 4, lightrad, lightrad, lightrad);
				lastlight = light;
			}

			if(!glaring && !waterrefract && m.depth!=lastdepth)
			{
				if(begin) { glEnd(); begin = false; }
				float depth = !waterfog ? 1.0f : min(0.75f*m.depth/waterfog, 0.95f);
                depth = max(depth, !below && (waterreflect || (waterenvmap && hasCM)) ? 0.3f : 0.6f);
				setlocalparamf("depth", SHPARAM_PIXEL, 5, depth, 1.0f-depth);
				lastdepth = m.depth;
			}

			if(!vertwater)
			{
				if(!begin) { glBegin(GL_QUADS); begin = true; }
				renderflatwater(m.o.x, m.o.y, m.o.z, m.rsize, m.csize);
			}
			else if(renderwaterlod(m.o.x, m.o.y, m.o.z, m.csize) >= (uint)m.csize * 2)
				rendervertwater(m.csize, m.o.x, m.o.y, m.o.z, m.csize);
		}
		if(begin) glEnd();
	}

    if(!glaring)
    {
        if(waterreflect || waterrefract)
        {
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
        }
        if(waterrefract)
        {
            glActiveTexture_(GL_TEXTURE3_ARB);
            glDisable(GL_TEXTURE_2D);
            if(hasFBO && renderpath!=R_FIXEDFUNCTION && waterfade) glDisable(GL_BLEND);
        }
        else
        {
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }

	loopi(2)
	{
		glActiveTexture_(GL_TEXTURE1_ARB+i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture_(GL_TEXTURE0_ARB);

    if(!glaring && waterenvmap && !waterreflect && hasCM)
    {
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        glEnable(GL_TEXTURE_2D);
    }

	glEnable(GL_CULL_FACE);
}

void setupwaterfallrefract(GLenum tmu1, GLenum tmu2)
{
    glActiveTexture_(tmu1);
    glBindTexture(GL_TEXTURE_2D, waterfallrefraction.refracttex ? waterfallrefraction.refracttex : notexture->id);
    glActiveTexture_(tmu2);
    glMatrixMode(GL_TEXTURE);
    setprojtexmatrix(waterfallrefraction);
    glMatrixMode(GL_MODELVIEW);
}

Reflection *findreflection(int height)
{
	loopi(MAXREFLECTIONS)
	{
		if(reflections[i].height==height) return &reflections[i];
	}
	return NULL;
}

void cleanreflection(Reflection &ref)
{
    ref.height = -1;
    ref.lastupdate = 0;
    ref.query = NULL;
    if(ref.fb)
    {
        glDeleteFramebuffers_(1, &ref.fb);
        ref.fb = 0;
    }
    if(ref.tex)
    {
        glDeleteTextures(1, &ref.tex);
        ref.tex = 0;
    }
    if(ref.refractfb)
    {
        glDeleteFramebuffers_(1, &ref.refractfb);
        ref.refractfb = 0;
    }
    if(ref.refracttex)
    {
        glDeleteTextures(1, &ref.refracttex);
        ref.refracttex = 0;
    }
}

void cleanreflections()
{
    loopi(MAXREFLECTIONS) cleanreflection(reflections[i]);
    cleanreflection(waterfallrefraction);
    if(reflectiondb)
    {
        glDeleteRenderbuffers_(1, &reflectiondb);
        reflectiondb = 0;
    }
}

VARFP(reflectsize, 6, 8, 10, cleanreflections());

void invalidatereflections()
{
	if(hasFBO) return;
	loopi(MAXREFLECTIONS) reflections[i].matsurfs.setsizenodelete(0);
    waterfallrefraction.matsurfs.setsizenodelete(0);
}

void genwatertex(GLuint &tex, GLuint &fb, bool refract = false)
{
    static const GLenum colorfmts[] = { GL_RGBA, GL_RGBA8, GL_RGB, GL_RGB8, GL_FALSE },
                        depthfmts[] = { GL_DEPTH_STENCIL_EXT, GL_DEPTH24_STENCIL8_EXT, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT32, GL_FALSE };
    const int stencilfmts = 2;
    static GLenum reflectfmt = GL_FALSE, refractfmt = GL_FALSE, depthfmt = GL_FALSE, stencilfmt = GL_FALSE;
    static bool usingalpha = false;
    bool needsalpha = refract && hasFBO && renderpath!=R_FIXEDFUNCTION && waterrefract && waterfade; 
    if(refract && usingalpha!=needsalpha)
    {
        usingalpha = needsalpha;
        refractfmt = GL_FALSE;
    }
    int size = 1<<reflectsize;
    if(!hasFBO) while(size>screen->w || size>screen->h) size /= 2;
    while(size>hwtexsize) size /= 2;

    GLenum &colorfmt = refract ? refractfmt : reflectfmt;
    int find = needsalpha ? 0 : 2;
    if(hasFBO)
    {
        if(!fb) glGenFramebuffers_(1, &fb);
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, fb);
    }

    glGenTextures(1, &tex);
    char *buf = new char[size*size*4];
    memset(buf, 0, size*size*4);

    do
    {
        createtexture(tex, size, size, buf, 3, false, colorfmt ? colorfmt : colorfmts[find]);
        if(!hasFBO) break;
        else
        {
            glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, tex, 0);
            if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT) break;
        }
    }
    while(!colorfmt && colorfmts[++find]);
    if(!colorfmt) colorfmt = colorfmts[find];

    delete[] buf;

    if(hasFBO)
    {
        if(!reflectiondb) { glGenRenderbuffers_(1, &reflectiondb); depthfmt = stencilfmt = GL_FALSE; }
        if(!depthfmt) glBindRenderbuffer_(GL_RENDERBUFFER_EXT, reflectiondb);
        find = hasstencil && hasDS ? 0 : stencilfmts;
        do
        {
            if(!depthfmt) glRenderbufferStorage_(GL_RENDERBUFFER_EXT, depthfmts[find], size, size);
            glFramebufferRenderbuffer_(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, reflectiondb);
            if(depthfmt ? stencilfmt : find<stencilfmts) glFramebufferRenderbuffer_(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, reflectiondb);
            if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT) break;
        }
        while(!depthfmt && depthfmts[++find]);
        if(!depthfmt)
        {
            glBindRenderbuffer_(GL_RENDERBUFFER_EXT, 0);
            depthfmt = depthfmts[find];
            stencilfmt = find<stencilfmts ? depthfmt : GL_FALSE;
        }

        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
    }
}

void addwaterfallrefraction(materialsurface &m)
{
    Reflection &ref = waterfallrefraction;
    if(ref.lastused!=totalmillis)
    {
        ref.lastused = totalmillis;
        ref.matsurfs.setsizenodelete(0);
        ref.height = INT_MAX;
    }
    ref.matsurfs.add(&m);

    if(!ref.refracttex) genwatertex(ref.refracttex, ref.refractfb);
}

void addreflection(materialsurface &m)
{
	int height = m.o.z;
	Reflection *ref = NULL, *oldest = NULL;
	loopi(MAXREFLECTIONS)
	{
		Reflection &r = reflections[i];
		if(r.height<0)
		{
			if(!ref) ref = &r;
		}
		else if(r.height==height)
		{
			r.matsurfs.add(&m);
			if(r.lastused==totalmillis) return;
			ref = &r;
			break;
		}
		else if(!oldest || r.lastused<oldest->lastused) oldest = &r;
	}
	if(!ref)
	{
		if(!oldest || oldest->lastused==totalmillis) return;
		ref = oldest;
	}
	if(ref->height!=height) ref->height = height;
	rplanes++;
	ref->lastused = totalmillis;
	ref->matsurfs.setsizenodelete(0);
	ref->matsurfs.add(&m);
	if(nowater) return;

    if(waterreflect && !ref->tex) genwatertex(ref->tex, ref->fb);
    if(waterrefract && !ref->refracttex) genwatertex(ref->refracttex, ref->refractfb, true);
}

extern vtxarray *visibleva;
extern void drawreflection(float z, bool refract, bool clear);

int rplanes = 0;

static int lastquery = 0;

void queryreflection(Reflection &ref, bool init)
{
    if(init)
    {
        nocolorshader->set();
        glDepthMask(GL_FALSE);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glDisable(GL_CULL_FACE);
    }
    startquery(ref.query);
    glBegin(GL_QUADS);
    loopvj(ref.matsurfs)
    {
        materialsurface &m = *ref.matsurfs[j];
        float offset = 0.1f;
        if(m.orient==O_TOP)
        {
            offset = WATER_OFFSET +
                (vertwater ? WATER_AMPLITUDE*(camera1->pitch > 0 || m.depth < WATER_AMPLITUDE+0.5f ? -1 : 1) : 0);
            if(fabs(m.o.z-offset - camera1->o.z) < 0.5f && m.depth > WATER_AMPLITUDE+1.5f)
                offset += camera1->pitch > 0 ? -1 : 1;
        }
        drawmaterial(m.orient, m.o.x, m.o.y, m.o.z, m.csize, m.rsize, offset);
    }
    glEnd();
    endquery(ref.query);
}

void queryreflections()
{
	rplanes = 0;

	static int lastsize = 0;
    int size = 1<<reflectsize;
	if(!hasFBO) while(size>screen->w || size>screen->h) size /= 2;
	if(size!=lastsize) { if(lastsize) cleanreflections(); lastsize = size; }

    bool shouldrefract = waterfallrefract && renderpath!=R_FIXEDFUNCTION;
    for(vtxarray *va = visibleva; va; va = va->next)
    {
        if(!va->matsurfs || va->occluded >= OCCLUDE_BB || va->curvfc >= VFC_FOGGED) continue;
        loopi(va->matsurfs)
        {
            materialsurface &m = va->matbuf[i];
            if(m.material==MAT_WATER)
            {
                if(m.orient==O_TOP) addreflection(m);
                else if(m.orient!=O_BOTTOM && shouldrefract) addwaterfallrefraction(m);
            }
        }
    }

	lastquery = totalmillis;

    if((editmode && showmat) || !hasOQ || !oqfrags || !oqwater || nowater) return;

	int refs = 0;
    if(waterreflect || waterrefract) loopi(MAXREFLECTIONS)
    {
        Reflection &ref = reflections[i];
        ref.query = ref.height>=0 && ref.lastused>=totalmillis && ref.matsurfs.length() ? newquery(&ref) : NULL;
        if(ref.query) queryreflection(ref, !refs++);
    }
    if(renderpath!=R_FIXEDFUNCTION && waterfallrefract)
    {
        Reflection &ref = waterfallrefraction;
        ref.query = ref.height>=0 && ref.lastused>=totalmillis && ref.matsurfs.length() ? newquery(&ref) : NULL;
        if(ref.query) queryreflection(ref, !refs++);
    }

	if(refs)
	{
		defaultshader->set();
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glEnable(GL_CULL_FACE);
	}
}

VARP(maxreflect, 1, 1, 8);

int refracting = 0;
bool reflecting = false, fading = false, fogging = false;
float reflectz = 1e16f;

VAR(maskreflect, 0, 2, 16);

void maskreflection(Reflection &ref, float offset, bool reflect)
{
	if(!maskreflect)
	{
		glClear(GL_DEPTH_BUFFER_BIT | (hasstencil && hasDS ? GL_STENCIL_BUFFER_BIT : 0));
		return;
	}
	glClearDepth(0);
	glClear(GL_DEPTH_BUFFER_BIT | (hasstencil && hasDS ? GL_STENCIL_BUFFER_BIT : 0));
	glClearDepth(1);
	glDepthRange(1, 1);
	glDepthFunc(GL_ALWAYS);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	nocolorshader->set();
	if(reflect)
	{
		glPushMatrix();
		glTranslatef(0, 0, 2*(ref.height+offset));
		glScalef(1, 1, -1);
	}
	int border = maskreflect;
	glBegin(GL_QUADS);
	loopv(ref.matsurfs)
	{
		materialsurface &m = *ref.matsurfs[i];
		ivec o(m.o);
		o[R[dimension(m.orient)]] -= border;
		o[C[dimension(m.orient)]] -= border;
		drawmaterial(m.orient, o.x, o.y, o.z, m.csize+2*border, m.rsize+2*border, -offset);
	}
	glEnd();
	if(reflect) glPopMatrix();
	defaultshader->set();
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthFunc(GL_LESS);
	glDepthRange(0, 1);
}

void drawreflections()
{
	if(editmode && showmat) return;
	if(nowater || (!waterreflect && !waterrefract)) return;

	static int lastdrawn = 0;
	int refs = 0, n = lastdrawn;
    float offset = -WATER_OFFSET;
    int size = 1<<reflectsize;
    if(!hasFBO) while(size>screen->w || size>screen->h) size /= 2;
	loopi(MAXREFLECTIONS)
	{
		Reflection &ref = reflections[++n%MAXREFLECTIONS];
		if(ref.height<0 || ref.lastused<lastquery || ref.matsurfs.empty()) continue;
		if(hasOQ && oqfrags && oqwater && ref.query && ref.query->owner==&ref && checkquery(ref.query)) continue;

		bool hasbottom = true;
		loopvj(ref.matsurfs)
		{
			materialsurface &m = *ref.matsurfs[j];
			if(m.depth>=10000) hasbottom = false;
		}

		if(!refs) glViewport(hasFBO ? 0 : screen->w-size, hasFBO ? 0 : screen->h-size, size, size);

		refs++;
		ref.lastupdate = totalmillis;
		lastdrawn = n;

        if(waterreflect && ref.tex && camera1->o.z >= ref.height+offset)
        {
            if(ref.fb) glBindFramebuffer_(GL_FRAMEBUFFER_EXT, ref.fb);
            maskreflection(ref, offset, true);
            drawreflection(ref.height+offset, false, false);
            if(!ref.fb)
            {
                glBindTexture(GL_TEXTURE_2D, ref.tex);
                glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen->w-size, screen->h-size, size, size);
            }
        }

        if(waterrefract && ref.refracttex)
        {
            if(ref.refractfb) glBindFramebuffer_(GL_FRAMEBUFFER_EXT, ref.refractfb);
            maskreflection(ref, offset, false);
            drawreflection(ref.height+offset, true, !hasbottom);
            if(!ref.refractfb)
            {
                glBindTexture(GL_TEXTURE_2D, ref.refracttex);
                glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen->w-size, screen->h-size, size, size);
            }
        }

		if(refs>=maxreflect) break;
	}

    if(renderpath!=R_FIXEDFUNCTION && waterfallrefract && waterfallrefraction.refracttex)
    {
        Reflection &ref = waterfallrefraction;

        if(ref.height<0 || ref.lastused<lastquery || ref.matsurfs.empty()) goto nowaterfall;
        if(hasOQ && oqfrags && oqwater && ref.query && ref.query->owner==&ref && checkquery(ref.query)) goto nowaterfall;

        if(!refs) glViewport(hasFBO ? 0 : screen->w-size, hasFBO ? 0 : screen->h-size, size, size);
        refs++;
        ref.lastupdate = totalmillis;

        if(ref.refractfb) glBindFramebuffer_(GL_FRAMEBUFFER_EXT, ref.refractfb);
        maskreflection(ref, -0.1, false);
        drawreflection(-1, true, false);
        if(!ref.refractfb)
        {
            glBindTexture(GL_TEXTURE_2D, ref.refracttex);
            glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen->w-size, screen->h-size, size, size);
        }
    }
nowaterfall:

	if(!refs) return;
	glViewport(0, 0, screen->w, screen->h);
	if(hasFBO) glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);

	defaultshader->set();
}

