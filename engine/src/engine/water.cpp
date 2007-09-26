#include "pch.h"
#include "engine.h"

VARFP(waterreflect, 0, 1, 1, cleanreflections());
VARFP(waterrefract, 0, 1, 1, cleanreflections());

/* vertex water */
VARP(watersubdiv, 0, 2, 3);
VARP(waterlod, 0, 1, 3);

static int wx1, wy1, wx2, wy2, wsize;
float wcol[4];

#define VERTW(vertw, body) \
	inline void vertw(float v1, float v2, float v3, float t) \
	{ \
		float angle = (v1-wx1)/wsize*(v2-wy1)/wsize*(v1-wx2)*(v2-wy2)*59/23+t; \
		float s = sinf(angle), h = 0.8f*s-1.1f; \
		body; \
		glVertex3f(v1, v2, v3+h); \
	}
#define VERTWN(vertw, body) \
	inline void vertw(float v1, float v2, float v3) \
	{ \
		float h = -1.1f; \
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
			else if(nowater || (!waterrefract && !waterreflect)) { renderwaterstrips(vertwc, z, t); }
			else if(waterrefract) { renderwaterstrips(vertwmtc, z, t); }
			else { renderwaterstrips(vertwtc, z, t); }
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
			else if(nowater || (!waterrefract && !waterreflect)) { renderwaterquad(vertwcn, z); }
			else if(waterrefract) { renderwaterquad(vertwmtcn, z); }
			else { renderwaterquad(vertwtcn, z); }
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
VAR(waterfog, 0, 150, 10000);

void getwatercolour(uchar *wcol)
{
	static const uchar defaultwcol[3] = { 20, 70, 80};
	if(hdr.watercolour[0] || hdr.watercolour[1] || hdr.watercolour[2]) memcpy(wcol, hdr.watercolour, 3);
	else memcpy(wcol, defaultwcol, 3);
}

void watercolour(int *r, int *g, int *b)
{
	hdr.watercolour[0] = *r;
	hdr.watercolour[1] = *g;
	hdr.watercolour[2] = *b;
}

COMMAND(watercolour, "iii");

VAR(lavafog, 0, 50, 10000);

void getlavacolour(uchar *lcol)
{
	static const uchar defaultlcol[3] = { 255, 64, 0 };
	if(hdr.lavacolour[0] || hdr.lavacolour[1] || hdr.lavacolour[2]) memcpy(lcol, hdr.lavacolour, 3);
	else memcpy(lcol, defaultlcol, 3);
}

void lavacolour(int *r, int *g, int *b)
{
	hdr.lavacolour[0] = *r;
	hdr.lavacolour[1] = *g;
	hdr.lavacolour[2] = *b;
}

COMMAND(lavacolour, "iii");

Shader *watershader = NULL, *waterreflectshader = NULL, *waterrefractshader = NULL;

void setprojtexmatrix(Reflection &ref, bool init = true)
{
	if(init && ref.lastupdate==totalmillis)
	{
		GLfloat pm[16], mm[16];
		glGetFloatv(GL_PROJECTION_MATRIX, pm);
		glGetFloatv(GL_MODELVIEW_MATRIX, mm);

		glLoadIdentity();
		glTranslatef(0.5f, 0.5f, 0.5f);
		glScalef(0.5f, 0.5f, 0.5f);
		glMultMatrixf(pm);
		glMultMatrixf(mm);

		glGetFloatv(GL_TEXTURE_MATRIX, ref.tm);
	}
	else glLoadMatrixf(ref.tm);
}

void setuprefractTMUs()
{
	setuptmu(0, "K , T @ Ka");

	glActiveTexture_(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);

	setuptmu(1, "P , T @ C~a");

	glActiveTexture_(GL_TEXTURE0_ARB);
}

void setupreflectTMUs()
{
	setuptmu(0, "T , K @ Ca", "Ka * P~a");

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_SRC_ALPHA);
}

void cleanupwaterTMUs(bool refract)
{
	resettmu(0);

	if(refract)
	{
		glActiveTexture_(GL_TEXTURE1_ARB);
		resettmu(1);
		glLoadIdentity();
		glDisable(GL_TEXTURE_2D);
		glActiveTexture_(GL_TEXTURE0_ARB);
	}
	else
	{
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}

VAR(waterspec, 0, 150, 1000);

Reflection reflections[MAXREFLECTIONS];
GLuint reflectiondb = 0;

VAR(oqwater, 0, 1, 1);

extern int oqfrags;

void renderwaterff()
{
	glDisable(GL_CULL_FACE);
	
	if(!nowater && (waterreflect || waterrefract))
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

	int lastdepth = -1;
	float offset = -1.1f;

	uchar wcolub[3];
	getwatercolour(wcolub);
	loopi(3) wcol[i] = wcolub[i]/255.0f;

	bool blended = true;
	loopi(MAXREFLECTIONS)
	{
		Reflection &ref = reflections[i];
		if(ref.height<0 || ref.lastused<totalmillis || ref.matsurfs.empty()) continue;

		if(!nowater && (waterreflect || waterrefract))
		{
			if(hasOQ && oqfrags && oqwater && ref.query && ref.query->owner==&ref && checkquery(ref.query)) continue;
			if(waterrefract) glActiveTexture_(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, ref.tex);
			setprojtexmatrix(ref);

			if(waterrefract)
			{
				glActiveTexture_(GL_TEXTURE0_ARB);
				glBindTexture(GL_TEXTURE_2D, camera1->o.z>=ref.height+offset ? ref.refracttex : ref.tex);
				setprojtexmatrix(ref, !waterreflect);
			}
			else
			{
				if(camera1->o.z < ref.height+offset) { if(blended) { glDepthMask(GL_TRUE); glDisable(GL_BLEND); blended = false; } }
				else if(!blended) { glDepthMask(GL_FALSE); glEnable(GL_BLEND); blended = true; }
			}
		}
		bool begin = false;
		loopvj(ref.matsurfs)
		{
			materialsurface &m = *ref.matsurfs[j];
	
			if(m.depth!=lastdepth)
			{
				float depth = !waterfog ? 1.0f : min(0.75f*m.depth/waterfog, 0.95f);
				if(nowater || !waterrefract) depth = max(depth, nowater || !waterreflect ? 0.6f : 0.3f);
				wcol[3] = depth;
				if(!nowater && (waterreflect || waterrefract))
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

	if(!nowater && (waterreflect || waterrefract))
	{
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

void renderwater()
{
	if(editmode && showmat) return;
	if(!rplanes) return;

	if(renderpath==R_FIXEDFUNCTION) { renderwaterff(); return; }

	glDisable(GL_CULL_FACE);

	uchar wcol[3] = { 20, 70, 80 };
	if(hdr.watercolour[0] || hdr.watercolour[1] || hdr.watercolour[2]) memcpy(wcol, hdr.watercolour, 3);
	glColor3ubv(wcol);

	Slot &s = lookuptexture(-MAT_WATER);

	glActiveTexture_(GL_TEXTURE1_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s.sts[2].t->gl);
	glActiveTexture_(GL_TEXTURE2_ARB);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, s.sts[3].t->gl);
	if(waterrefract)
	{
		glActiveTexture_(GL_TEXTURE3_ARB);
		glEnable(GL_TEXTURE_2D);
	}
	else
	{
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_SRC_ALPHA);
	}
	glActiveTexture_(GL_TEXTURE0_ARB);

	setenvparamf("camera", SHPARAM_VERTEX, 0, camera1->o.x, camera1->o.y, camera1->o.z);
	setenvparamf("millis", SHPARAM_VERTEX, 1, lastmillis/1000.0f, lastmillis/1000.0f, lastmillis/1000.0f);

	if(!watershader) watershader = lookupshaderbyname("water");
	if(!waterreflectshader) waterreflectshader = lookupshaderbyname("waterreflect");
	if(!waterrefractshader) waterrefractshader = lookupshaderbyname("waterrefract");
	
	(waterrefract ? waterrefractshader : (waterreflect ? waterreflectshader : watershader))->set();

	if(waterreflect || waterrefract) glMatrixMode(GL_TEXTURE);

	vec ambient(max(hdr.skylight[0], hdr.ambient), max(hdr.skylight[1], hdr.ambient), max(hdr.skylight[2], hdr.ambient));
	entity *lastlight = (entity *)-1;
	int lastdepth = -1;
	float offset = -1.1f;
	bool blended = true;
	loopi(MAXREFLECTIONS)
	{
		Reflection &ref = reflections[i];
		if(ref.height<0 || ref.lastused<totalmillis || ref.matsurfs.empty()) continue;

		if(waterreflect || waterrefract)
		{
			if(hasOQ && oqfrags && oqwater && ref.query && ref.query->owner==&ref && checkquery(ref.query)) continue;
			glBindTexture(GL_TEXTURE_2D, ref.tex);
			setprojtexmatrix(ref);
		}

		if(waterrefract)
		{
			glActiveTexture_(GL_TEXTURE3_ARB);
			glBindTexture(GL_TEXTURE_2D, camera1->o.z>=ref.height+offset ? ref.refracttex : ref.tex);
			glActiveTexture_(GL_TEXTURE0_ARB);
		}
		else if(waterreflect)
		{
			if(camera1->o.z < ref.height+offset) { if(blended) { glDepthMask(GL_TRUE); glDisable(GL_BLEND); blended = false; } }
			else if(!blended) { glDepthMask(GL_FALSE); glEnable(GL_BLEND); blended = true; }
		}

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
				const vec &lightcol = (light ? vec(light->attr2, light->attr3, light->attr4) : vec(ambient)).div(255.0f).mul(waterspec/100.0f);
				setlocalparamf("lightpos", SHPARAM_VERTEX, 2, lightpos.x, lightpos.y, lightpos.z);
				setlocalparamf("lightcolor", SHPARAM_PIXEL, 3, lightcol.x, lightcol.y, lightcol.z);
				setlocalparamf("lightradius", SHPARAM_PIXEL, 4, lightrad, lightrad, lightrad);
				lastlight = light;
			}

			if(!waterrefract && m.depth!=lastdepth)
			{
				if(begin) { glEnd(); begin = false; }
				float depth = !waterfog ? 1.0f : min(0.75f*m.depth/waterfog, 0.95f);
				depth = max(depth, waterreflect ? 0.3f : 0.6f);
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

	if(waterreflect || waterrefract)
	{
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
	}

	if(waterrefract)
	{
		glActiveTexture_(GL_TEXTURE3_ARB);
		glDisable(GL_TEXTURE_2D);
	}
	else
	{
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}

	loopi(2)
	{
		glActiveTexture_(GL_TEXTURE1_ARB+i);
		glDisable(GL_TEXTURE_2D);
	}
	glActiveTexture_(GL_TEXTURE0_ARB);

	glEnable(GL_CULL_FACE);
}

Reflection *findreflection(int height)
{
	loopi(MAXREFLECTIONS)
	{
		if(reflections[i].height==height) return &reflections[i];
	}
	return NULL;
}

void cleanreflections()
{
	loopi(MAXREFLECTIONS)
	{
		Reflection &ref = reflections[i];
		if(ref.fb)
		{
			glDeleteFramebuffers_(1, &ref.fb);
			ref.fb = 0;
		}
		if(ref.tex)
		{
			glDeleteTextures(1, &ref.tex);
			ref.tex = 0;
			ref.height = -1;
			ref.lastupdate = 0;
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

	static const GLenum colorfmts[] = { GL_RGB, GL_RGB8, GL_FALSE },
						depthfmts[] = { GL_DEPTH_STENCIL_EXT, GL_DEPTH24_STENCIL8_EXT, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT32, GL_FALSE };
	const int stencilfmts = 2;
	static GLenum colorfmt = GL_FALSE, depthfmt = GL_FALSE, stencilfmt = GL_FALSE;
	char *buf = NULL;
	int size = 1<<reflectsize;
	if(!hasFBO) while(size>screen->w || size>screen->h) size /= 2;
	while(size>hwtexsize) size /= 2;
	if((waterreflect || waterrefract) && !ref->tex)
	{
		glGenTextures(1, &ref->tex);
		buf = new char[size*size*3];
		memset(buf, 0, size*size*3);
		int find = 0;
		if(hasFBO)
		{
			if(!ref->fb) glGenFramebuffers_(1, &ref->fb);
			glBindFramebuffer_(GL_FRAMEBUFFER_EXT, ref->fb);
		}
		do
		{
			createtexture(ref->tex, size, size, buf, 3, false, colorfmt ? colorfmt : colorfmts[find]);
			if(!hasFBO) break;
			else
			{
				glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, ref->tex, 0);
				if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT) break;
			}
		}
		while(!colorfmt && colorfmts[++find]);
		if(!colorfmt) colorfmt = colorfmts[find];

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
	if(waterrefract && !ref->refracttex)
	{
		glGenTextures(1, &ref->refracttex);
		createtexture(ref->refracttex, size, size, buf, 3, false, colorfmt);
		
		if(hasFBO)
		{
			if(!ref->refractfb) glGenFramebuffers_(1, &ref->refractfb);
			glBindFramebuffer_(GL_FRAMEBUFFER_EXT, ref->refractfb);
			glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, ref->refracttex, 0);
			glFramebufferRenderbuffer_(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, reflectiondb);
			if(stencilfmt) glFramebufferRenderbuffer_(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, reflectiondb);
			glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
		}
	}
	if(buf) delete[] buf;
}

extern vtxarray *visibleva;
extern void drawreflection(float z, bool refract, bool clear);

int rplanes = 0;

static int lastquery = 0;

void queryreflections()
{
	rplanes = 0;

	static int lastsize = 0, size = 1<<reflectsize;
	if(!hasFBO) while(size>screen->w || size>screen->h) size /= 2;
	if(size!=lastsize) { if(lastsize) cleanreflections(); lastsize = size; }

	for(vtxarray *va = visibleva; va; va = va->next)
	{
		lodlevel &lod = va->l0;
		if(!lod.matsurfs && va->occluded >= OCCLUDE_BB) continue;
		loopi(lod.matsurfs)
		{
			materialsurface &m = lod.matbuf[i];
			if(m.material==MAT_WATER && m.orient==O_TOP) addreflection(m);
		}
	}
	
	lastquery = totalmillis;

	if((editmode && showmat) || !hasOQ || !oqfrags || !oqwater || nowater || (!waterreflect && !waterrefract)) return;

	int refs = 0;
	loopi(MAXREFLECTIONS)
	{
		Reflection &ref = reflections[i];
		if(ref.height<0 || ref.lastused<totalmillis || ref.matsurfs.empty())
		{
			ref.query = NULL;
			continue;
		}
		ref.query = newquery(&ref);
		if(!ref.query) continue;

		if(!refs)
		{
			nocolorshader->set();
			glDepthMask(GL_FALSE);
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glDisable(GL_CULL_FACE);
		}
		refs++;
		startquery(ref.query);
		glBegin(GL_QUADS);
		loopvj(ref.matsurfs)
		{
			materialsurface &m = *ref.matsurfs[j];
			drawmaterial(m.orient, m.o.x, m.o.y, m.o.z, m.csize, m.rsize, renderpath==R_FIXEDFUNCTION || vertwater ? 0.1f : 1.1f);
		}
		glEnd();
		endquery(ref.query);
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

float reflecting = 0, refracting = 0;

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
	float offset = -1.1f;
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

		int size = 1<<reflectsize;
		if(!hasFBO) while(size>screen->w || size>screen->h) size /= 2;

		if(!refs) glViewport(hasFBO ? 0 : screen->w-size, hasFBO ? 0 : screen->h-size, size, size);

		refs++;
		ref.lastupdate = totalmillis;
		lastdrawn = n;

		if(waterreflect && ref.tex)
		{
			if(ref.fb) glBindFramebuffer_(GL_FRAMEBUFFER_EXT, ref.fb);
			maskreflection(ref, offset, camera1->o.z >= ref.height+offset);
			drawreflection(ref.height+offset, false, false);
			if(!ref.fb)
			{
				glBindTexture(GL_TEXTURE_2D, ref.tex);
				glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen->w-size, screen->h-size, size, size);
			}
		}

		if(waterrefract && ref.refracttex && camera1->o.z >= ref.height+offset)
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
	
	if(!refs) return;
	glViewport(0, 0, screen->w, screen->h);
	if(hasFBO) glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);

	defaultshader->set();
}

