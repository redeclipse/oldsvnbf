// renderparticles.cpp

#include "pch.h"
#include "engine.h"
#include "rendertarget.h"

static struct flaretype {
	int type;			 /* flaretex index, 0..5, -1 for 6+random shine */
	float loc;			/* postion on axis */
	float scale;		  /* texture scaling */
	uchar alpha;		  /* color alpha */
} flaretypes[] =
{
	{2,  1.30f, 0.04f, 153}, //flares
	{3,  1.00f, 0.10f, 102},
	{1,  0.50f, 0.20f, 77},
	{3,  0.20f, 0.05f, 77},
	{0,  0.00f, 0.04f, 77},
	{5, -0.25f, 0.07f, 127},
	{5, -0.40f, 0.02f, 153},
	{5, -0.60f, 0.04f, 102},
	{5, -1.00f, 0.03f, 51},
	{-1, 1.00f, 0.30f, 255}, //shine - red, green, blue
	{-2, 1.00f, 0.20f, 255},
	{-3, 1.00f, 0.25f, 255}
};

struct flare
{
	vec o, center;
	float size;
	uchar color[3];
	bool sparkle;
};
#define MAXFLARE 64 //overkill..
static flare flarelist[MAXFLARE];
static int flarecnt;
static unsigned int shinetime = 0; //randomish

VARW(flarelights, 0, 0, 1);
VARP(flarecutoff, 0, 1000, 10000);
VARP(flaresize, 20, 100, 500);

static void newflare(vec &o,  const vec &center, uchar r, uchar g, uchar b, float mod, float size, bool sun, bool sparkle)
{
	if(flarecnt >= MAXFLARE) return;
	vec target; //occlusion check (neccessary as depth testing is turned off)
	if(!raycubelos(o, camera1->o, target)) return;
	flare &f = flarelist[flarecnt++];
	f.o = o;
	f.center = center;
	f.size = size;
	f.color[0] = uchar(r*mod);
	f.color[1] = uchar(g*mod);
	f.color[2] = uchar(b*mod);
	f.sparkle = sparkle;
}

static void makeflare(vec &o, uchar r, uchar g, uchar b, bool sun, bool sparkle)
{
	//frustrum + fog check
	if(isvisiblesphere(0.0f, o) > (sun?VFC_FOGGED:VFC_FULL_VISIBLE)) return;
	//find closest point between camera line of sight and flare pos
	vec viewdir;
	vecfromyawpitch(camera1->yaw, camera1->pitch, 1, 0, viewdir);
	vec flaredir = vec(o).sub(camera1->o);
	vec center = viewdir.mul(flaredir.dot(viewdir)).add(camera1->o);
	float mod, size;
	if(sun) //fixed size
	{
		mod = 1.0;
		size = flaredir.magnitude() * flaresize / 100.0f;
	}
	else
	{
		mod = (flarecutoff-vec(o).sub(center).squaredlen())/flarecutoff;
		if(mod < 0.0f) return;
		size = flaresize / 5.0f;
	}
	newflare(o, center, r, g, b, mod, size, sun, sparkle);
}

static void renderflares(int time)
{
    bool fog = glIsEnabled(GL_FOG)==GL_TRUE;
    if(fog) glDisable(GL_FOG);
	defaultshader->set();
	glDisable(GL_DEPTH_TEST);

	static Texture *flaretex = NULL;
	if(!flaretex) flaretex = textureload("textures/lensflares.png");
	glBindTexture(GL_TEXTURE_2D, flaretex->id);
	glBegin(GL_QUADS);
	loopi(flarecnt)
	{
		flare *f = flarelist+i;
		vec center = f->center;
		vec axis = vec(f->o).sub(center);
		uchar color[4] = {f->color[0], f->color[1], f->color[2], 255};
		loopj(f->sparkle?12:9)
		{
			const flaretype &ft = flaretypes[j];
			vec o = vec(axis).mul(ft.loc).add(center);
			float sz = ft.scale * f->size;
			int tex = ft.type;
			if(ft.type < 0) //sparkles - always done last
			{
				shinetime = (!time ? j : (shinetime + 1)) % 10;
				tex = 6+shinetime;
				color[0] = 0;
				color[1] = 0;
				color[2] = 0;
				color[-ft.type-1] = f->color[-ft.type-1]; //only want a single channel
			}
			color[3] = ft.alpha;
			glColor4ubv(color);
			const float tsz = 0.25; //flares are aranged in 4x4 grid
			float tx = tsz*(tex&0x03);
			float ty = tsz*((tex>>2)&0x03);
			glTexCoord2f(tx,	 ty+tsz); glVertex3f(o.x+(-camright.x+camup.x)*sz, o.y+(-camright.y+camup.y)*sz, o.z+(-camright.z+camup.z)*sz);
			glTexCoord2f(tx+tsz, ty+tsz); glVertex3f(o.x+( camright.x+camup.x)*sz, o.y+( camright.y+camup.y)*sz, o.z+( camright.z+camup.z)*sz);
			glTexCoord2f(tx+tsz, ty);	 glVertex3f(o.x+( camright.x-camup.x)*sz, o.y+( camright.y-camup.y)*sz, o.z+( camright.z-camup.z)*sz);
			glTexCoord2f(tx,	 ty);	 glVertex3f(o.x+(-camright.x-camup.x)*sz, o.y+(-camright.y-camup.y)*sz, o.z+(-camright.z-camup.z)*sz);
		}
	}
	glEnd();
	glEnable(GL_DEPTH_TEST);
    if(fog) glEnable(GL_FOG);
}

static void makelightflares()
{
	flarecnt = 0; //regenerate flarelist each frame
	shinetime += detrnd(lastmillis, 10);

	if(editmode || !flarelights) return;

	const vector<extentity *> &ents = et->getents();
	vec viewdir;
	vecfromyawpitch(camera1->yaw, camera1->pitch, 1, 0, viewdir);
	extern const vector<int> &checklightcache(int x, int y);
	const vector<int> &lights = checklightcache(int(camera1->o.x), int(camera1->o.y));
	loopv(lights)
	{
		entity &e = *ents[lights[i]];
		if(e.type != ET_LIGHT) continue;
		bool sun = (e.attr1==0);
		float radius = float(e.attr1);
		vec flaredir = vec(e.o).sub(camera1->o);
		float len = flaredir.magnitude();
		if(!sun && (len > radius)) continue;
		if(isvisiblesphere(0.0f, e.o) > (sun?VFC_FOGGED:VFC_FULL_VISIBLE)) continue;
		vec center = vec(viewdir).mul(flaredir.dot(viewdir)).add(camera1->o);
		float mod, size;
		if(sun) //fixed size
		{
			mod = 1.0;
			size = len * flaresize / 100.0f;
		}
		else
		{
			mod = (radius-len)/radius;
			size = flaresize / 5.0f;
		}
		newflare(e.o, center, e.attr2, e.attr3, e.attr4, mod, size, sun, sun);
	}
}

// eye space depth texture for soft particles, done at low res then blurred to prevent ugly jaggies
VARP(depthfxfpscale, 1, 1<<12, 1<<16);
VARP(depthfxscale, 1, 1<<6, 1<<8);
VARP(depthfxblend, 1, 16, 64);
VAR(depthfxmargin, 0, 16, 64);
VAR(depthfxbias, 0, 1, 64);

extern void cleanupdepthfx();
VARFP(fpdepthfx, 0, 1, 1, cleanupdepthfx());
VARFP(depthfxprecision, 0, 1, 1, cleanupdepthfx());

#define MAXDFXRANGES 4

void *depthfxowners[MAXDFXRANGES];
float depthfxranges[MAXDFXRANGES];
int numdepthfxranges = 0;
vec depthfxmin(1e16f, 1e16f, 1e16f), depthfxmax(1e16f, 1e16f, 1e16f);

static struct depthfxtexture : rendertarget
{
    const GLenum *colorformats() const
    {
        static const GLenum colorfmts[] = { GL_RGB16F_ARB, GL_RGB16, GL_RGBA, GL_RGBA8, GL_RGB, GL_RGB8, GL_FALSE };
        return &colorfmts[hasTF && hasFBO ? (fpdepthfx ? 0 : (depthfxprecision ? 1 : 2)) : 2];
    }

    bool dorender()
    {
        depthfxing = true;
        refracting = -1;

        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        extern void renderdepthobstacles(const vec &bbmin, const vec &bbmax, float scale, float *ranges, int numranges);
        float scale = depthfxscale;
        float *ranges = depthfxranges;
        int numranges = numdepthfxranges;
        if(colorfmt==GL_RGB16F_ARB || colorfmt==GL_RGB16)
        {
            scale = depthfxfpscale;
            ranges = NULL;
            numranges = 0;
        }
        renderdepthobstacles(depthfxmin, depthfxmax, scale, ranges, numranges);

        refracting = 0;
        depthfxing = false;

        return true;
    }
} depthfxtex;

void cleanupdepthfx()
{
    depthfxtex.cleanup(true);
}

VARFP(depthfxsize, 6, 7, 10, cleanupdepthfx());
VARP(depthfx, 0, 0, 1);
VARP(blurdepthfx, 0, 1, 7);
VARP(blurdepthfxsigma, 1, 50, 200);

VAR(debugdepthfx, 0, 0, 1);

void viewdepthfxtex()
{
    if(!depthfx) return;
    depthfxtex.debug();
}

bool depthfxing = false;

void drawdepthfxtex()
{
    if(!depthfx || renderpath==R_FIXEDFUNCTION) return;

    extern int finddepthfxranges(void **owners, float *ranges, int numranges, vec &bbmin, vec &bbmax);
    depthfxmin = vec(1e16f, 1e16f, 1e16f);
    depthfxmax = vec(0, 0, 0);
    numdepthfxranges = finddepthfxranges(depthfxowners, depthfxranges, MAXDFXRANGES, depthfxmin, depthfxmax);
    loopk(3)
    {
        depthfxmin[k] -= depthfxmargin;
        depthfxmax[k] += depthfxmargin;
    }
    if(!numdepthfxranges && !debugdepthfx) return;

    // Apple/ATI bug - fixed-function fog state can force software fallback even when fragment program is enabled
    glDisable(GL_FOG);
    depthfxtex.render(1<<depthfxsize, blurdepthfx, blurdepthfxsigma/100.0f);
    glEnable(GL_FOG);
}

//cache our unit hemisphere
static GLushort *hemiindices = NULL;
static vec *hemiverts = NULL;
static int heminumverts = 0, heminumindices = 0;
static GLuint hemivbuf = 0, hemiebuf = 0;

static void subdivide(int depth, int face);

static void genface(int depth, int i1, int i2, int i3)
{
	int face = heminumindices; heminumindices += 3;
	hemiindices[face]	= i1;
	hemiindices[face+1] = i2;
	hemiindices[face+2] = i3;
	subdivide(depth, face);
}

static void subdivide(int depth, int face)
{
	if(depth-- <= 0) return;
	int idx[6];
	loopi(3) idx[i] = hemiindices[face+i];
	loopi(3)
	{
		int vert = heminumverts++;
		hemiverts[vert] = vec(hemiverts[idx[i]]).add(hemiverts[idx[(i+1)%3]]).normalize(); //push on to unit sphere
		idx[3+i] = vert;
		hemiindices[face+i] = vert;
	}
	subdivide(depth, face);
	loopi(3) genface(depth, idx[i], idx[3+i], idx[3+(i+2)%3]);
}

//subdiv version wobble much more nicely than a lat/longitude version
static void inithemisphere(int hres, int depth)
{
	const int tris = hres << (2*depth);
	heminumverts = heminumindices = 0;
	DELETEA(hemiverts);
	DELETEA(hemiindices);
	hemiverts = new vec[tris+1];
	hemiindices = new GLushort[tris*3];
	hemiverts[heminumverts++] = vec(0.0f, 0.0f, 1.0f); //build initial 'hres' sided pyramid
	loopi(hres)
	{
		float a = PI2*float(i)/hres;
		hemiverts[heminumverts++] = vec(cosf(a), sinf(a), 0.0f);
	}
	loopi(hres) genface(depth, 0, i+1, 1+(i+1)%hres);

	if(hasVBO)
	{
		if(renderpath!=R_FIXEDFUNCTION)
		{
			if(!hemivbuf) glGenBuffers_(1, &hemivbuf);
			glBindBuffer_(GL_ARRAY_BUFFER_ARB, hemivbuf);
			glBufferData_(GL_ARRAY_BUFFER_ARB, heminumverts*sizeof(vec), hemiverts, GL_STATIC_DRAW_ARB);
			DELETEA(hemiverts);
		}

		if(!hemiebuf) glGenBuffers_(1, &hemiebuf);
		glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, hemiebuf);
		glBufferData_(GL_ELEMENT_ARRAY_BUFFER_ARB, heminumindices*sizeof(GLushort), hemiindices, GL_STATIC_DRAW_ARB);
		DELETEA(hemiindices);
	}
}

static GLuint expmodtex[2] = {0, 0};
static GLuint lastexpmodtex = 0;

static GLuint createexpmodtex(int size, float minval)
{
	uchar *data = new uchar[size*size], *dst = data;
	loop(y, size) loop(x, size)
	{
		float dx = 2*float(x)/(size-1) - 1, dy = 2*float(y)/(size-1) - 1;
        float z = max(0.0f, 1.0f - dx*dx - dy*dy);
		if(minval) z = sqrtf(z);
		else loopk(2) z *= z;
		*dst++ = uchar(max(z, minval)*255);
	}
	GLuint tex = 0;
	glGenTextures(1, &tex);
	createtexture(tex, size, size, data, 3, true, GL_ALPHA);
	delete[] data;
	return tex;
}

static struct expvert
{
	vec pos;
	float u, v, s, t;
} *expverts = NULL;
static GLuint expvbuf = 0;

static void animateexplosion()
{
	static int lastexpmillis = 0;
	if(expverts && lastexpmillis == lastmillis)
	{
		if(hasVBO) glBindBuffer_(GL_ARRAY_BUFFER_ARB, expvbuf);
		return;
	}
	lastexpmillis = lastmillis;
	vec center = vec(13.0f, 2.3f, 7.1f);  //only update once per frame! - so use the same center for all...
	if(!expverts) expverts = new expvert[heminumverts];
	loopi(heminumverts)
	{
		expvert &e = expverts[i];
		vec &v = hemiverts[i];
		//texgen - scrolling billboard
		e.u = v.x*0.5f + 0.0004f*lastmillis;
		e.v = v.y*0.5f + 0.0004f*lastmillis;
		//ensure the mod texture is wobbled
		e.s = v.x*0.5f + 0.5f;
		e.t = v.y*0.5f + 0.5f;
		//wobble - similar to shader code
		float wobble = v.dot(center) + 0.002f*lastmillis;
		wobble -= floor(wobble);
		wobble = 1.0f + fabs(wobble - 0.5f)*0.5f;
		e.pos = vec(v).mul(wobble);
	}

	if(hasVBO)
	{
		if(!expvbuf) glGenBuffers_(1, &expvbuf);
		glBindBuffer_(GL_ARRAY_BUFFER_ARB, expvbuf);
		glBufferData_(GL_ARRAY_BUFFER_ARB, heminumverts*sizeof(expvert), expverts, GL_STREAM_DRAW_ARB);
	}
}

static struct spherevert
{
	vec pos;
	float s, t;
} *sphereverts = NULL;
static GLushort *sphereindices = NULL;
static int spherenumverts = 0, spherenumindices = 0;
static GLuint spherevbuf = 0, sphereebuf = 0;

static void initsphere(int slices, int stacks)
{
	DELETEA(sphereverts);
	spherenumverts = (stacks+1)*(slices+1);
	sphereverts = new spherevert[spherenumverts];
	float ds = 1.0f/slices, dt = 1.0f/stacks, t = 1.0f;
	loopi(stacks+1)
	{
		float rho = M_PI*(1-t), s = 0.0f;
		loopj(slices+1)
		{
			float theta = j==slices ? 0 : 2*M_PI*s;
			spherevert &v = sphereverts[i*(slices+1) + j];
			v.pos = vec(-sin(theta)*sin(rho), cos(theta)*sin(rho), cos(rho));
			v.s = s;
			v.t = t;
			s += ds;
		}
		t -= dt;
	}

	DELETEA(sphereindices);
	spherenumindices = stacks*slices*3*2;
	sphereindices = new ushort[spherenumindices];
	GLushort *curindex = sphereindices;
	loopi(stacks)
	{
		loopk(slices)
		{
			int j = i%2 ? slices-k-1 : k;

			*curindex++ = i*(slices+1)+j;
			*curindex++ = (i+1)*(slices+1)+j;
			*curindex++ = i*(slices+1)+j+1;

			*curindex++ = i*(slices+1)+j+1;
			*curindex++ = (i+1)*(slices+1)+j;
			*curindex++ = (i+1)*(slices+1)+j+1;
		}
	}

	if(hasVBO)
	{
		if(!spherevbuf) glGenBuffers_(1, &spherevbuf);
		glBindBuffer_(GL_ARRAY_BUFFER_ARB, spherevbuf);
		glBufferData_(GL_ARRAY_BUFFER_ARB, spherenumverts*sizeof(spherevert), sphereverts, GL_STATIC_DRAW_ARB);
		DELETEA(sphereverts);

		if(!sphereebuf) glGenBuffers_(1, &sphereebuf);
		glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, sphereebuf);
		glBufferData_(GL_ELEMENT_ARRAY_BUFFER_ARB, spherenumindices*sizeof(GLushort), sphereindices, GL_STATIC_DRAW_ARB);
		DELETEA(sphereindices);
	}
}

VARP(explosion2d, 0, 0, 1);

static void setupexplosion()
{
	if(renderpath!=R_FIXEDFUNCTION || maxtmus>=2)
	{
		if(!expmodtex[0]) expmodtex[0] = createexpmodtex(64, 0);
		if(!expmodtex[1]) expmodtex[1] = createexpmodtex(64, 0.25f);
		lastexpmodtex = 0;
	}

	if(renderpath!=R_FIXEDFUNCTION)
	{
        if(glaring)
        {
            if(explosion2d) SETSHADER(explosion2dglare); else SETSHADER(explosion3dglare);
        }
        else if(!reflecting && !refracting && depthfx && depthfxtex.rendertex && numdepthfxranges>0)
        {
            if(depthfxtex.colorfmt!=GL_RGB16F_ARB && depthfxtex.colorfmt!=GL_RGB16)
            {
                if(explosion2d) SETSHADER(explosion2dsoft8); else SETSHADER(explosion3dsoft8);
            }
            else if(explosion2d) SETSHADER(explosion2dsoft); else SETSHADER(explosion3dsoft);

            glActiveTexture_(GL_TEXTURE2_ARB);
            glBindTexture(GL_TEXTURE_2D, depthfxtex.rendertex);
            glActiveTexture_(GL_TEXTURE0_ARB);
        }
        else if(explosion2d) SETSHADER(explosion2d); else SETSHADER(explosion3d);
	}

	if(renderpath==R_FIXEDFUNCTION || explosion2d)
	{
		if(!hemiverts && !hemivbuf) inithemisphere(5, 2);
		if(renderpath==R_FIXEDFUNCTION) animateexplosion();
		if(hasVBO)
		{
			if(renderpath!=R_FIXEDFUNCTION) glBindBuffer_(GL_ARRAY_BUFFER_ARB, hemivbuf);
			glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, hemiebuf);
		}

		expvert *verts = renderpath==R_FIXEDFUNCTION ? (hasVBO ? 0 : expverts) : (expvert *)hemiverts;

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, renderpath==R_FIXEDFUNCTION ? sizeof(expvert) : sizeof(vec), verts);

		if(renderpath==R_FIXEDFUNCTION)
		{
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glTexCoordPointer(2, GL_FLOAT, sizeof(expvert), &verts->u);

			if(maxtmus>=2)
			{
				setuptmu(0, "C * T", "= Ca");

				glActiveTexture_(GL_TEXTURE1_ARB);
				glClientActiveTexture_(GL_TEXTURE1_ARB);

				glEnable(GL_TEXTURE_2D);
				setuptmu(1, "P * Ta x 4", "Pa * Ta x 4");
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);
				glTexCoordPointer(2, GL_FLOAT, sizeof(expvert), &verts->s);

				glActiveTexture_(GL_TEXTURE0_ARB);
				glClientActiveTexture_(GL_TEXTURE0_ARB);
			}
		}
	}
	else
	{
		if(!sphereverts && !spherevbuf) initsphere(12, 6);

		if(hasVBO)
		{
			glBindBuffer_(GL_ARRAY_BUFFER_ARB, spherevbuf);
			glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, sphereebuf);
		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(spherevert), &sphereverts->pos);
		glTexCoordPointer(2, GL_FLOAT, sizeof(spherevert), &sphereverts->s);
	}
}

static void drawexpverts(int numverts, int numindices, GLushort *indices)
{
	if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, 0, numverts-1, numindices, GL_UNSIGNED_SHORT, indices);
	else glDrawElements(GL_TRIANGLES, numindices, GL_UNSIGNED_SHORT, indices);
	xtraverts += numindices;
	glde++;
}

static void drawexplosion(bool inside, uchar r, uchar g, uchar b, uchar a)
{
	if((renderpath!=R_FIXEDFUNCTION || maxtmus>=2) && lastexpmodtex != expmodtex[inside ? 1 : 0])
	{
		glActiveTexture_(GL_TEXTURE1_ARB);
		lastexpmodtex = expmodtex[inside ? 1 :0];
		glBindTexture(GL_TEXTURE_2D, lastexpmodtex);
		glActiveTexture_(GL_TEXTURE0_ARB);
	}
    int passes = !reflecting && !refracting && inside ? 2 : 1;
    if(renderpath!=R_FIXEDFUNCTION && !explosion2d)
    {
        if(inside) glScalef(1, 1, -1);
        loopi(passes)
        {
            glColor4ub(r, g, b, i ? a/2 : a);
            if(i) glDepthFunc(GL_GEQUAL);
            drawexpverts(spherenumverts, spherenumindices, sphereindices);
            if(i) glDepthFunc(GL_LESS);
        }
        return;
    }
    loopi(passes)
    {
        glColor4ub(r, g, b, i ? a/2 : a);
        if(i)
        {
            glScalef(1, 1, -1);
            glDepthFunc(GL_GEQUAL);
        }
        if(inside)
        {
            if(passes >= 2)
			{
				glCullFace(GL_BACK);
				drawexpverts(heminumverts, heminumindices, hemiindices);
				glCullFace(GL_FRONT);
			}
			glScalef(1, 1, -1);
		}
		drawexpverts(heminumverts, heminumindices, hemiindices);
		if(i) glDepthFunc(GL_LESS);
	}
}

static void cleanupexplosion()
{
	glDisableClientState(GL_VERTEX_ARRAY);
	if(renderpath==R_FIXEDFUNCTION)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		if(maxtmus>=2)
		{
			resettmu(0);

			glActiveTexture_(GL_TEXTURE1_ARB);
			glClientActiveTexture_(GL_TEXTURE1_ARB);

			glDisable(GL_TEXTURE_2D);
			resettmu(1);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

			glActiveTexture_(GL_TEXTURE0_ARB);
			glClientActiveTexture_(GL_TEXTURE0_ARB);
		}
	}
	else
	{
		if(explosion2d) glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        if(fogging) setfogplane(1, refracting);
	}

	if(hasVBO)
	{
		glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
		glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
	}
}

#define MAXPARTYPES 24

struct particle
{
	vec o, d;
	int fade, millis, color;
	particle *next;
	union
	{
        const char *text;         // will call delete[] on this only if it starts with an @
		float val;
        physent *owner;
	};
};

static particle *parlist[MAXPARTYPES], *parempty = NULL;

VARP(particlesize, 20, 100, 500);

// Check emit_particles() to limit the rate that paricles can be emitted for models/sparklies
// Automatically stops particles being emitted when paused or in reflective drawing
VARP(emitfps, 1, 60, 200);
static int lastemitframe = 0;
static bool emit = false;

static bool emit_particles()
{
	if(reflecting || refracting) return false;
	if(emit) return emit;
	int emitmillis = 1000/emitfps;
	emit = (lastmillis-lastemitframe>emitmillis);
	return emit;
}

#define PART_TEXS 11
static Texture *parttexs[PART_TEXS];

void particleinit()
{
    parttexs[0] = textureload("textures/base.png");
    parttexs[1] = textureload("textures/ball1.png");
    parttexs[2] = textureload("textures/smoke.png");
    parttexs[3] = textureload("textures/ball2.png");
    parttexs[4] = textureload("textures/ball3.png");
    parttexs[5] = textureload("textures/flare.jpg");
    parttexs[6] = textureload("textures/spark.png");
    parttexs[7] = textureload("textures/explosion.jpg");
    parttexs[8] = textureload("textures/blood.png");
    parttexs[9] = textureload("textures/lightning.png");
	parttexs[10] = textureload("textures/electricty.png");
	loopi(MAXPARTYPES) parlist[i] = NULL;
}

void cleanupparticles()
{
    loopi(2) if(expmodtex[i]) { glDeleteTextures(1, &expmodtex[i]); expmodtex[i] = 0; }
    if(hemivbuf) { glDeleteBuffers_(1, &hemivbuf); hemivbuf = 0; }
    if(hemiebuf) { glDeleteBuffers_(1, &hemiebuf); hemiebuf = 0; }
    DELETEA(hemiverts);
    DELETEA(hemiindices);
    if(expvbuf) { glDeleteBuffers_(1, &expvbuf); expvbuf = 0; }
    DELETEA(expverts);
    if(spherevbuf) { glDeleteBuffers_(1, &spherevbuf); spherevbuf = 0; }
    if(sphereebuf) { glDeleteBuffers_(1, &sphereebuf); sphereebuf = 0; }
    DELETEA(sphereverts);
    DELETEA(sphereindices);
}

enum
{
	PT_PART = 0,
	PT_FLARE,
	PT_TRAIL,
	PT_TEXT,
	PT_TEXTUP,
	PT_METER,
	PT_METERVS,
	PT_FIREBALL,
	PT_DECAL,
    PT_LIGHTNING,

	PT_ENT  = 1<<8,
	PT_MOD  = 1<<9,
	PT_RND4 = 1<<10,
    PT_LERP  = 1<<11, // Use very sparingly!
    PT_TRACK = 1<<12,
    PT_GLARE = 1<<13
};

// @TODO reorder so as to draw meters & lerps first to reduce visual errors
static struct parttype { int type; int gr, tex; float sz; int collide; } parttypes[MAXPARTYPES] =
{
	{ PT_MOD|PT_RND4,  2,  8, 2.96f, 1 }, // 0 blood spats (note: rgb is inverted)
	{ PT_MOD|PT_RND4|PT_DECAL, 0, 8, 5.92f, 0 }, // 1 blood stain
	{ PT_GLARE, 		2,  6, 0.24f,  0 }, // 2 sparks
	{ 0,			 -20,  2,  0.6f,  0 }, // 3 small slowly rising smoke
	{ PT_GLARE,		  20,  0, 0.32f,  0 }, // 4 edit mode entities
	{ PT_GLARE,		  20,  1,  4.8f,  0 }, // 5 fireball1
	{ 0,			 -20,  2,  2.4f,  0 }, // 6 big  slowly rising smoke
	{ PT_GLARE,		  20,  3,  4.8f,  0 }, // 7 fireball2
	{ PT_GLARE,		  20,  4,  4.8f,  0 }, // 8 big fireball3
	{ PT_TEXTUP,	  -8, -1,  4.0f,  0 }, // 9 TEXT
	{ PT_FLARE|PT_GLARE, 0,  5, 0.28f,  0 }, // 10 flare
	{ PT_TEXT,		 0, -1,  2.0f,  0 }, // 11 TEXT, SMALL, NON-MOVING
	{ PT_GLARE,			  20,  4,  2.0f,  0 }, // 12 fireball3
	{ PT_METER,		0, -1,  2.0f,  0 }, // 13 METER, SMALL, NON-MOVING
	{ PT_METERVS,	  0, -1,  2.0f,  0 }, // 14 METER vs., SMALL, NON-MOVING
	{ 0,			  20,  2,  0.6f,  0 }, // 15 small  slowly sinking smoke trail
	{ PT_FIREBALL|PT_GLARE,	 0,  7,  4.0f,  0 }, // 16 explosion fireball
	{ PT_ENT,		-20,  2,  2.4f,  0 }, // 17 big  slowly rising smoke, entity
	{ 0,			 -15,  2,  2.4f,  0 }, // 18 big  fast rising smoke
	{ PT_ENT|PT_TRAIL|PT_LERP, 2, 0, 0.60f, 0 }, // 19 water, entity
	{ PT_ENT|PT_GLARE,		 20,  1,  4.8f,  0 }, // 20 fireball1, entity
    { PT_LIGHTNING|PT_TRACK|PT_GLARE,    0,  9, 0.28f,  0 }, // 21 lightning
	{ PT_LIGHTNING|PT_TRACK|PT_GLARE,    0,  10, 0.60f,  0 }, // 22 spark
    { PT_FIREBALL,      0,  7, 4.0f,  0 }, // 23 explosion fireball, no glare
};

static particle *newparticle(const vec &o, const vec &d, int fade, int type, int color)
{
	if(!parempty)
	{
		particle *ps = new particle[256];
		loopi(255) ps[i].next = &ps[i+1];
		ps[255].next = parempty;
		parempty = ps;
	}
	particle *p = parempty;
	parempty = p->next;
	p->o = o;
	p->d = d;
	p->fade = fade;
	p->millis = lastmillis;
	p->color = color;
	p->next = parlist[type];
	p->text = NULL;
	parlist[type] = p;
	return p;
}

void clearparticles()
{
	loopi(MAXPARTYPES) if(parlist[i])
	{
		particle *head = parlist[i], *tail = head;
		while(tail->next)
		{
			switch(parttypes[i].type)
			{
				case PT_TEXT:
				case PT_TEXTUP:
					if(tail->text && tail->text[0]=='@') delete[] tail->text;
					break;
			}
			tail = tail->next;
		}
		tail->next = parempty;
		parempty = head;
		parlist[i] = NULL;
	}
}

void removetrackedparticles(physent *owner)
{
    loopi(MAXPARTYPES) if(parttypes[i].type&PT_TRACK)
    {
        for(particle **prev = &parlist[i], *cur = parlist[i]; cur; cur = *prev)
        {
            if(!owner || cur->owner==owner)
            {
                *prev = cur->next;
                cur->next = parempty;
                parempty = cur;
            }
            else prev = &cur->next;
        }
    }
}

VARP(outlinemeters, 0, 0, 1);

#define COLLIDERADIUS 8.0f
#define COLLIDEERROR 1.0f

#define MAXLIGHTNINGSTEPS 64
#define LIGHTNINGSTEP 8
int lnjitterx[MAXLIGHTNINGSTEPS], lnjittery[MAXLIGHTNINGSTEPS];
int lastlnjitter = 0;

VAR(lnjittermillis, 0, 100, 1000);
VAR(lnjitterradius, 0, 2, 100);

static void setuplightning()
{
    if(lastmillis-lastlnjitter > lnjittermillis)
    {
        lastlnjitter = lastmillis - (lastmillis%lnjittermillis);
        loopi(MAXLIGHTNINGSTEPS)
        {
            lnjitterx[i] = -lnjitterradius + rnd(2*lnjitterradius + 1);
            lnjittery[i] = -lnjitterradius + rnd(2*lnjitterradius + 1);
        }
    }
}

static void renderlightning(const vec &o, const vec &d, float sz, float tx, float ty, float tsz)
{
    vec step(d);
    step.sub(o);
    float len = step.magnitude();
    int numsteps = min(int(ceil(len/LIGHTNINGSTEP)), MAXLIGHTNINGSTEPS);
    if(numsteps > 1) step.mul(LIGHTNINGSTEP/len);
    int jitteroffset = detrnd(int(o.x+o.y+o.z), MAXLIGHTNINGSTEPS);
    vec cur(o), up, right;
    up.orthogonal(step);
    up.normalize();
    right.cross(up, step);
    right.normalize();
    glBegin(GL_QUAD_STRIP);
    loopj(numsteps)
    {
        vec next(cur);
        next.add(step);
        if(j+1==numsteps) next = d;
        next.add(vec(right).mul(sz*lnjitterx[(j+jitteroffset)%MAXLIGHTNINGSTEPS]));
        next.add(vec(up).mul(sz*lnjittery[(j+jitteroffset)%MAXLIGHTNINGSTEPS]));
        vec dir1 = next, dir2 = next, across;
        dir1.sub(cur);
        dir2.sub(camera1->o);
        across.cross(dir2, dir1).normalize().mul(sz);
        float tx1 = j&1 ? tx : tx+tsz, tx2 = j&1 ? tx+tsz : tx;
        glTexCoord2f(tx2, ty+tsz); glVertex3f(cur.x-across.x, cur.y-across.y, cur.z-across.z);
        glTexCoord2f(tx2, ty);     glVertex3f(cur.x+across.x, cur.y+across.y, cur.z+across.z);
        if(j+1==numsteps)
        {
            glTexCoord2f(tx1, ty+tsz); glVertex3f(next.x-across.x, next.y-across.y, next.z-across.z);
            glTexCoord2f(tx1, ty);     glVertex3f(next.x+across.x, next.y+across.y, next.z+across.z);
        }
        cur = next;
    }
    glEnd();
}

static const float WOBBLE = 1.25f;

int finddepthfxranges(void **owners, float *ranges, int maxranges, vec &bbmin, vec &bbmax)
{
    GLfloat mm[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mm);

    physent e;
    e.type = ENT_CAMERA;

    int numranges = 0;
    for(particle *p = parlist[16]; p; p = p->next)
    {
        int ts = p->fade <= 5 ? p->fade : lastmillis-p->millis;
        float pmax = p->val,
              size = p->fade ? float(ts)/p->fade : 1,
              psize = (parttypes[16].sz + pmax * size)*WOBBLE;
        if(2*(parttypes[16].sz + pmax)*WOBBLE < depthfxblend || isvisiblesphere(psize, p->o) >= VFC_FOGGED) continue;

        e.o = p->o;
        e.radius = e.height = e.aboveeye = psize;
        if(::collide(&e, vec(0, 0, 0), 0, false)) continue;

        vec dir = camera1->o;
        dir.sub(p->o);
        float dist = dir.magnitude();
        dir.mul(psize/dist).add(p->o);
        float depth = max(-(dir.x*mm[2] + dir.y*mm[6] + dir.z*mm[10] + mm[14]) - depthfxbias, 0.0f);

        loopk(3)
        {
            bbmin[k] = min(bbmin[k], p->o[k] - psize);
            bbmax[k] = max(bbmax[k], p->o[k] + psize);
        }

        int pos = numranges;
        loopi(numranges) if(depth < ranges[i]) { pos = i; break; }
        if(pos >= maxranges) continue;

        if(numranges > pos)
        {
            int moved = min(numranges-pos, maxranges-(pos+1));
            memmove(&ranges[pos+1], &ranges[pos], moved*sizeof(float));
            memmove(&owners[pos+1], &owners[pos], moved*sizeof(void *));
        }
        if(numranges < maxranges) numranges++;

        ranges[pos] = depth;
        owners[pos] = p;
    }

    return numranges;
}

VARP(particleglare, 0, 4, 100);

void render_particles(int time)
{
    if(glaring && !particleglare) return;

    static Shader *particleshader = NULL, *particlenotextureshader = NULL;
    if(!particleshader) particleshader = lookupshaderbyname("particle");
    if(!particlenotextureshader) particlenotextureshader = lookupshaderbyname("particlenotexture");

	static float zerofog[4] = { 0, 0, 0, 1 };
	float oldfogc[4];
	bool rendered = false;
    loopi(MAXPARTYPES) if(parlist[i] && (!glaring || parttypes[i].type&PT_GLARE))
	{
		if(!rendered)
		{
			rendered = true;
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);

            if(glaring) setenvparamf("colorscale", SHPARAM_VERTEX, 4, particleglare, particleglare, particleglare, 1);
            else setenvparamf("colorscale", SHPARAM_VERTEX, 4, 1, 1, 1, 1);

            particleshader->set();
			glGetFloatv(GL_FOG_COLOR, oldfogc);
			glFogfv(GL_FOG_COLOR, zerofog);
		}

		parttype &pt = parttypes[i];
		float sz = pt.type&PT_ENT ? pt.sz : pt.sz*particlesize/100.0f;
		int type = pt.type&0xFF;

		if(pt.type&PT_MOD) glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
		else if(pt.type&PT_LERP)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glFogfv(GL_FOG_COLOR, oldfogc);
		}
		if(pt.tex >= 0) glBindTexture(GL_TEXTURE_2D, parttexs[pt.tex]->id);

        bool quads = false;
        switch(type)
		{
            case PT_PART:
            case PT_FLARE:
            case PT_TRAIL:
            case PT_DECAL:
                quads = true;
                glBegin(GL_QUADS);
                break;

            case PT_LIGHTNING:
                setuplightning();
                quads = true;
                glDisable(GL_CULL_FACE);
                break;

            case PT_FIREBALL:
                setupexplosion();
                break;

            case PT_METER:
            case PT_METERVS:
				glDisable(GL_BLEND);
				glDisable(GL_TEXTURE_2D);
				particlenotextureshader->set();
				glFogfv(GL_FOG_COLOR, oldfogc);
                break;

            default:
                glFogfv(GL_FOG_COLOR, oldfogc);
                break;
		}

		for(particle *p, **pp = &parlist[i]; (p = *pp);)
		{
            int ts = lastmillis-p->millis, blend;
			bool remove = false;
            vec o = p->o, d = p->d;
            if(pt.type&PT_TRACK && p->owner) cl->particletrack(p->owner, o, d);
			uchar color[3] = {p->color>>16, (p->color>>8)&0xFF, p->color&0xFF};
			if(p->fade > 5)
			{	//parametric coordinate generation (but only if its going to show for more than one frame)
				if(pt.gr)
				{
					float t = (float)(ts);
                    vec v = d;
					v.mul(t/5000.0f);
					o.add(v);
					o.z -= t*t/(2.0f * 5000.0f * pt.gr);
				}
				blend = max(255 - (ts<<8)/p->fade, 0);
				if(ts >= p->fade) remove = true;
			}
			else
			{
				blend = 255;
				ts = p->fade;
				remove = true;
			}

			if(quads)
			{
                if(type!=PT_FLARE)
				{
                    blend = min(blend<<2, 255);
                    if(type!=PT_LIGHTNING && renderpath==R_FIXEDFUNCTION && fogging) blend = (uchar)(blend * max(0.0f, min(1.0f, (reflectz - o.z)/waterfog)));
                }
                if(pt.type&PT_MOD) //multiply alpha into color
					glColor3ub((color[0]*blend)>>8, (color[1]*blend)>>8, (color[2]*blend)>>8);
				else
                    glColor4ub(color[0], color[1], color[2], blend);

				float tx = 0, ty = 0, tsz = 1;
				if(pt.type&PT_RND4)
				{
					int i = detrnd((size_t)p, 4);
					tx = 0.5f*(i&1);
					ty = 0.5f*((i>>1)&1);
					tsz = 0.5f;
				}
                if(type==PT_LIGHTNING) renderlightning(o, d, sz, tx, ty, tsz);
                else if(type==PT_FLARE || type==PT_TRAIL)
                {
                    vec e = d;
					if(type==PT_TRAIL)
					{
						if(pt.gr) e.z -= float(ts)/pt.gr;
						e.div(-75.0f);
						e.add(o);
					}
					vec dir1 = e, dir2 = e, c;
					dir1.sub(o);
					dir2.sub(camera1->o);
					c.cross(dir2, dir1).normalize().mul(sz);
					glTexCoord2f(tx,	 ty+tsz); glVertex3f(e.x-c.x, e.y-c.y, e.z-c.z);
					glTexCoord2f(tx+tsz, ty+tsz); glVertex3f(o.x-c.x, o.y-c.y, o.z-c.z);
					glTexCoord2f(tx+tsz, ty);	 glVertex3f(o.x+c.x, o.y+c.y, o.z+c.z);
					glTexCoord2f(tx,	 ty);	 glVertex3f(e.x+c.x, e.y+c.y, e.z+c.z);
				}
				else
				{
					vec v[4] = { o, o, o, o };
					if(type==PT_DECAL)
					{
						vec udir, vdir;
                        udir.orthogonal(d);
						udir.normalize();
						udir.mul(sz);
                        vdir.cross(d, udir);
						v[0].add(udir);
						v[1].add(vdir);
						v[2].sub(udir);
						v[3].sub(vdir);
					}
					else
					{
						v[0].add(vec(camup).sub(camright).mul(sz));
						v[1].add(vec(camup).add(camright).mul(sz));
						v[2].add(vec(camright).sub(camup).mul(sz));
						v[3].sub(vec(camup).add(camright).mul(sz));
					}
					int orient = detrnd((size_t)p, 11);
					glTexCoord2f(tx,	 ty+tsz); glVertex3fv(v[orient%4].v);
					glTexCoord2f(tx+tsz, ty+tsz); glVertex3fv(v[(orient+1)%4].v);
					glTexCoord2f(tx+tsz, ty);	 glVertex3fv(v[(orient+2)%4].v);
					glTexCoord2f(tx,	 ty);	 glVertex3fv(v[(orient+3)%4].v);
				}
			}
			else
			{
				glPushMatrix();
				glTranslatef(o.x, o.y, o.z);

                if(fogging)
                {
                    if(renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - o.z, true);
                    else blend = (uchar)(blend * max(0.0f, min(1.0f, (reflectz - o.z)/waterfog)));
                }
				if(type==PT_FIREBALL)
				{
					float pmax = p->val;
					float size = p->fade ? float(ts)/p->fade : 1;
					float psize = pt.sz + pmax * size;

					bool inside = o.dist(camera1->o) <= psize*WOBBLE;
					vec oc(o);
					oc.sub(camera1->o);
					if(reflecting) oc.z = o.z - reflectz;

					float yaw = inside ? camera1->yaw - 180 : atan2(oc.y, oc.x)/RAD - 90,
						  pitch = (inside ? camera1->pitch : asin(oc.z/oc.magnitude())/RAD) - 90;
					vec rotdir;
					if(renderpath==R_FIXEDFUNCTION || explosion2d)
					{
						glRotatef(yaw, 0, 0, 1);
						glRotatef(pitch, 1, 0, 0);
						rotdir = vec(0, 0, 1);
					}
					else
					{
						vec s(1, 0, 0), t(0, 1, 0);
						s.rotate(pitch*RAD, vec(-1, 0, 0));
						s.rotate(yaw*RAD, vec(0, 0, -1));
						t.rotate(pitch*RAD, vec(-1, 0, 0));
						t.rotate(yaw*RAD, vec(0, 0, -1));

						rotdir = vec(-1, 1, -1).normalize();
						s.rotate(-lastmillis/7.0f*RAD, rotdir);
						t.rotate(-lastmillis/7.0f*RAD, rotdir);

						setlocalparamf("texgenS", SHPARAM_VERTEX, 2, 0.5f*s.x, 0.5f*s.y, 0.5f*s.z, 0.5f);
						setlocalparamf("texgenT", SHPARAM_VERTEX, 3, 0.5f*t.x, 0.5f*t.y, 0.5f*t.z, 0.5f);
					}

					if(renderpath!=R_FIXEDFUNCTION)
					{
						setlocalparamf("center", SHPARAM_VERTEX, 0, o.x, o.y, o.z);
						setlocalparamf("animstate", SHPARAM_VERTEX, 1, size, psize, pmax, float(lastmillis));
                        if(!glaring && !reflecting && !refracting && depthfx && depthfxtex.rendertex && numdepthfxranges>0)
                        {
                            if(depthfxtex.colorfmt!=GL_RGB16F_ARB && depthfxtex.colorfmt!=GL_RGB16)
                            {
                                float select[4] = { 0, 0, 0, 0 }, offset = 2*hdr.worldsize;
                                loopi(numdepthfxranges) if(depthfxowners[i]==p)
                                {
                                    select[i] = float(depthfxscale)/depthfxblend;
                                    offset = depthfxranges[i] - depthfxbias;
                                    break;
                                }
                                setlocalparamf("depthfxrange", SHPARAM_VERTEX, 5, 1.0f/depthfxblend, -offset/depthfxblend, inside ? blend/(2*255.0f) : 0);
                                setlocalparamf("depthfxrange", SHPARAM_PIXEL, 5, 1.0f/depthfxblend, -offset/depthfxblend, inside ? blend/(2*255.0f) : 0);
                                setlocalparamfv("depthfxselect", SHPARAM_PIXEL, 6, select);
                            }
                            else
                            {
                                setlocalparamf("depthfxparams", SHPARAM_VERTEX, 5, float(depthfxfpscale)/depthfxblend, 1.0f/depthfxblend, inside ? blend/(2*255.0f) : 0);
                                setlocalparamf("depthfxparams", SHPARAM_PIXEL, 5, float(depthfxfpscale)/depthfxblend, 1.0f/depthfxblend, inside ? blend/(2*255.0f) : 0);
                            }
                        }
					}

					glRotatef(lastmillis/7.0f, -rotdir.x, rotdir.y, -rotdir.z);
					glScalef(-psize, psize, -psize);
					drawexplosion(inside, color[0], color[1], color[2], blend);
				}
				else
				{
					glRotatef(camera1->yaw-180, 0, 0, 1);
					glRotatef(camera1->pitch-90, 1, 0, 0);

					float scale = pt.sz/80.0f;
					glScalef(-scale, scale, -scale);
					if(type==PT_METER || type==PT_METERVS)
					{
						float right = 8*FONTH, left = p->val*right;
						glTranslatef(-right/2.0f, 0, 0);
						glColor3ubv(color);
						glBegin(GL_TRIANGLE_STRIP);
						loopk(10)
						{
							float c = 0.5f*cosf(M_PI/2 + k/9.0f*M_PI), s = 0.5f + 0.5f*sinf(M_PI/2 + k/9.0f*M_PI);
							glVertex2f(left - c*FONTH, s*FONTH);
							glVertex2f(c*FONTH, s*FONTH);
						}
						glEnd();

						if(type==PT_METERVS) glColor3ub(color[2], color[1], color[0]); //swap r<->b
						else glColor3f(0, 0, 0);
						glBegin(GL_TRIANGLE_STRIP);
						loopk(10)
						{
							float c = -0.5f*cosf(M_PI/2 + k/9.0f*M_PI), s = 0.5f - 0.5f*sinf(M_PI/2 + k/9.0f*M_PI);
							glVertex2f(left + c*FONTH, s*FONTH);
							glVertex2f(right + c*FONTH, s*FONTH);
						}
						glEnd();

						if(outlinemeters)
						{
							glColor3f(0, 0.8f, 0);
							glBegin(GL_LINE_LOOP);
							loopk(10)
							{
								float c = 0.5f*cosf(M_PI/2 + k/9.0f*M_PI), s = 0.5f + 0.5f*sinf(M_PI/2 + k/9.0f*M_PI);
								glVertex2f(c*FONTH, s*FONTH);
							}
							loopk(10)
							{
								float c = -0.5f*cosf(M_PI/2 + k/9.0f*M_PI), s = 0.5f - 0.5f*sinf(M_PI/2 + k/9.0f*M_PI);
								glVertex2f(right + c*FONTH, s*FONTH);
							}
							glEnd();
							glBegin(GL_LINE_STRIP);
							loopk(10)
							{
								float c = -0.5f*cosf(M_PI/2 + k/9.0f*M_PI), s = 0.5f - 0.5f*sinf(M_PI/2 + k/9.0f*M_PI);
								glVertex2f(left + c*FONTH, s*FONTH);
							}
							glEnd();
						}
					}
					else
					{
                        const char *text = p->text+(p->text[0]=='@');
						float xoff = -text_width(text)/2;
						float yoff = 0;
						if(type==PT_TEXTUP) { xoff += detrnd((size_t)p, 100)-50; yoff -= detrnd((size_t)p, 101); } else blend = 255;
						glTranslatef(xoff, yoff, 50);

						draw_text(text, 0, 0, color[0], color[1], color[2], blend);
					}
				}
				glPopMatrix();
			}

			if(pt.collide && o.z < p->val && !refracting && !reflecting)
			{
				vec surface;
                float floorz = rayfloor(vec(o.x, o.y, p->val), surface, RAY_CLIPMAT, COLLIDERADIUS),
                      collidez = floorz<0 ? o.z-COLLIDERADIUS : p->val - rayfloor(vec(o.x, o.y, p->val), surface, RAY_CLIPMAT, COLLIDERADIUS);
				if(o.z >= collidez+COLLIDEERROR) p->val = collidez+COLLIDEERROR;
                else adddecal(pt.collide, vec(o.x, o.y, collidez), vec(p->o).sub(o).normalize(), 2*sz, p->color, pt.type&PT_RND4 ? detrnd((size_t)p, 4) : 0);
                remove = true;
#if 0
				{
					*pp = p->next;
					p->o = vec(o.x, o.y, collidez+0.25f);
					p->d = surface;
					p->millis = lastmillis;
					p->fade = decalfade;
					p->next = parlist[pt.collide];
					parlist[pt.collide] = p;
					continue;
				}
#endif
			}

			if(remove && !reflecting && !refracting)
			{
				*pp = p->next;
				p->next = parempty;
				if((type==PT_TEXT || type==PT_TEXTUP) && p->text && p->text[0]=='@') delete[] p->text;
				parempty = p;
			}
			else
				pp = &p->next;
		}

        switch(type)
				{
            case PT_PART:
            case PT_FLARE:
            case PT_TRAIL:
            case PT_DECAL:
                glEnd();
                break;

            case PT_LIGHTNING:
                glEnable(GL_CULL_FACE);
                break;

            case PT_FIREBALL:
                cleanupexplosion();
                particleshader->set();
                break;

            case PT_METER:
            case PT_METERVS:
				glEnable(GL_BLEND);
				glEnable(GL_TEXTURE_2D);
                if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
				particleshader->set();
				glFogfv(GL_FOG_COLOR, zerofog);
                break;

            default:
                glBlendFunc(GL_SRC_ALPHA, GL_ONE);
                glFogfv(GL_FOG_COLOR, zerofog);
                if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz, true);
                break;
		}

		if(pt.type&PT_MOD) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		else if(pt.type&PT_LERP)
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			glFogfv(GL_FOG_COLOR, zerofog);
		}
	}

	if(flarecnt && !reflecting && !refracting) //the camera is hardcoded into the flares.. reflecting would be nonsense
	{
		if(!rendered)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		}

		renderflares(time);

		if(!rendered) glDisable(GL_BLEND);
	}

	if(rendered)
	{
		glFogfv(GL_FOG_COLOR, oldfogc);
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}

VARP(maxparticledistance, 256, 512, 4096);


//@TODO - extend this to splash in different shapes, e.g. cubes, curtains...
static void splash(int type, int color, int radius, int num, int fade, const vec &p)
{
	if(camera1->o.dist(p) > maxparticledistance) return;
	float collidez = parttypes[type].collide ? p.z - raycube(p, vec(0, 0, -1), COLLIDERADIUS, RAY_CLIPMAT) + COLLIDEERROR : -1;
	loopi(num)
	{
		int x, y, z;
		do
		{
			x = rnd(radius*2)-radius;
			y = rnd(radius*2)-radius;
			z = rnd(radius*2)-radius;
		}
		while(x*x+y*y+z*z>radius*radius);
		vec tmp = vec((float)x, (float)y, (float)z);
		newparticle(p, tmp, rnd(fade*3)+1, type, color)->val = collidez;
	}
}

static void regularsplash(int type, int color, int radius, int num, int fade, const vec &p, int delay=0)
{
	if(!emit_particles() || (delay > 0 && rnd(delay) != 0)) return;
	splash(type, color, radius, num, fade, p);
}



//maps 'classic' particles types to newer types and colors
// @NOTE potentially this and the following public funcs can be tidied up, but lets please defer that for a little bit...
static struct partmap { int type; int color; } partmaps[] =
{
	{  2, 0xB49B4B}, // 0 yellow: sparks
	{  3, 0x897661}, // 1 greyish-brown:	small slowly rising smoke
	{  4, 0x3232FF}, // 2 blue:	edit mode entities
	{  0, 0x60FFFF}, // 3 red:	blood spats (note: rgb is inverted)
	{  5, 0xFFC8C8}, // 4 yellow: fireball1
	{  6, 0x897661}, // 5 greyish-brown:	big  slowly rising smoke
	{  7, 0xFFFFFF}, // 6 blue:	fireball2
	{  8, 0xFFFFFF}, // 7 green:  big fireball3
	{  9, 0xFF4B19}, // 8 TEXT RED
	{  9, 0x32FF64}, // 9 TEXT GREEN
	{ 10, 0xFFC864}, // 10 yellow flare
	{ 11, 0x1EC850}, // 11 TEXT DARKGREEN, SMALL, NON-MOVING
	{ 12, 0xFFFFFF}, // 12 green small fireball3
	{ 11, 0xFF4B19}, // 13 TEXT RED, SMALL, NON-MOVING
	{ 11, 0xB4B4B4}, // 14 TEXT GREY, SMALL, NON-MOVING
	{  9, 0xFFC864}, // 15 TEXT YELLOW
	{ 11, 0x6496FF}, // 16 TEXT BLUE, SMALL, NON-MOVING
	{ 13, 0xFF1932}, // 17 METER RED, SMALL, NON-MOVING
	{ 13, 0x3219FF}, // 18 METER BLUE, SMALL, NON-MOVING
	{ 14, 0xFF1932}, // 19 METER RED vs. BLUE, SMALL, NON-MOVING (note swaps r<->b)
	{ 14, 0x3219FF}, // 20 METER BLUE vs. RED, SMALL, NON-MOVING (note swaps r<->b)
	{ 15, 0x897661}, // 21 greyish-brown:	small  slowly sinking smoke trail
	{ 16, 0xFF8080}, // 22 red explosion fireball
	{ 16, 0xA0C080}, // 23 orange explosion fireball
	{ 17, 0x897661}, // 24 greyish-brown:	big  slowly rising smoke
	{ 18, 0x897661}, // 25 greyish-brown:	big  fast rising smoke
	{ 19, 0x3232FF}, // 26 water
    { 20, 0xFFC8C8}, // 27 yellow: fireball1
    { 21, 0xFFFFFF}, // 28 lightning: yellow
    { 21, 0xFF2222}, // 29 lightning: red
    { 21, 0x2222FF}, // 30 lightning: blue
    { 23, 0x802020}, // 31 fireball: red
    { 23, 0x2020FF}, // 32 fireball: blue
    { 23, 0x208020}, // 33 fireball: green
    {  9, 0x6496FF}, // 34 TEXT BLUE
};

void regular_particle_splash(int type, int num, int fade, const vec &p, int delay)
{
    if(shadowmapping) return;
	int radius = (type==5 || type == 24) ? 50 : 150;
	regularsplash(partmaps[type].type, partmaps[type].color, radius, num, fade, p, delay);
}

void particle_splash(int type, int num, int fade, const vec &p)
{
    if(shadowmapping || renderedgame) return;
	splash(partmaps[type].type, partmaps[type].color, 150, num, fade, p);
}

void particle_trail(int type, int fade, const vec &s, const vec &e)
{
    if(shadowmapping || renderedgame) return;
	vec v;
	float d = e.dist(s, v);
	v.div(d*2);
	vec p = s;
	int ptype = partmaps[type].type;
	int color = partmaps[type].color;
	loopi((int)d*2)
	{
		p.add(v);
		vec tmp = vec(float(rnd(11)-5), float(rnd(11)-5), float(rnd(11)-5));
		newparticle(p, tmp, rnd(fade)+fade, ptype, color);
	}
}

VARP(particletext, 0, 1, 1);

void particle_text(const vec &s, const char *t, int type, int fade)
{
    if(shadowmapping || renderedgame) return;
	if(!particletext || camera1->o.dist(s) > 128) return;
	if(t[0]=='@') t = newstring(t);
	newparticle(s, vec(0, 0, 1), fade, partmaps[type].type, partmaps[type].color)->text = t;
}

void particle_meter(const vec &s, float val, int type, int fade)
{
    if(shadowmapping || renderedgame) return;
	newparticle(s, vec(0, 0, 1), fade, partmaps[type].type, partmaps[type].color)->val = val;
}

void particle_flare(const vec &p, const vec &dest, int fade, int type, physent *owner)
{
    if(shadowmapping || renderedgame) return;
    newparticle(p, dest, fade, partmaps[type].type, partmaps[type].color)->owner = owner;
}

void particle_fireball(const vec &dest, float maxsize, int type, int fade)
{
    if(shadowmapping || renderedgame) return;
    float growth = maxsize - parttypes[partmaps[type].type].sz;
    if(fade < 0) fade = int(growth*25);
    newparticle(dest, vec(0, 0, 1), fade, partmaps[type].type, partmaps[type].color)->val = growth;
}

//dir = 0..6 where 0=up
static inline vec offsetvec(vec o, int dir, int dist)
{
	vec v = vec(o);
	v[(2+dir)%3] += (dir>2)?(-dist):dist;
	return v;
}

//converts a 16bit color to 24bit
static inline int colorfromattr(int attr)
{
	return (((attr&0xF)<<4) | ((attr&0xF0)<<8) | ((attr&0xF00)<<12)) + 0x0F0F0F;
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
void regularshape(int type, int radius, int color, int dir, int num, int fade, const vec &p)
{
	if(!emit_particles()) return;

    int pt = parttypes[type].type&0xFF;
    bool flare = (pt == PT_FLARE) || (pt == PT_LIGHTNING);

	bool inv = (dir >= 32);
	dir = dir&0x1F;
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

        if(flare)
		newparticle(inv?to:from, inv?from:to, rnd(fade*3)+1, type, color);
        else
        {
            vec d(to);
            d.sub(from);
            d.normalize().mul(inv ? -200.0f : 200.0f); //velocity
            newparticle(inv?to:from, d, rnd(fade*3)+1, type, color);
        }
	}
}


static void makeparticles(entity &e)
{
	switch(e.attr1)
	{
		case 0: //fire
			regular_particle_splash(27, 1, 40, e.o);
			regular_particle_splash(24, 1, 200, vec(e.o.x, e.o.y, e.o.z+3.0), 3);
			break;
		case 1: //smoke vent - <dir>
			regular_particle_splash(24, 1, 200, offsetvec(e.o, e.attr2, rnd(10)));
			break;
		case 2: //water fountain - <dir>
		{
			uchar col[3];
			getwatercolour(col);
			int color = (col[0]<<16) | (col[1]<<8) | col[2];
			regularsplash(19, color, 150, 4, 200, offsetvec(e.o, e.attr2, rnd(10)));
			break;
		}
		case 3: //fire ball - <size> <rgb>
			newparticle(e.o, vec(0, 0, 1), 1, 16, colorfromattr(e.attr3))->val = 1+e.attr2;
			break;
        case 4:  //tape - <dir> <length> <rgb>
        case 7:  //lightning
        case 8:  //fire
        case 9:  //smoke
        case 10: //water
        {
            const int typemap[] = { 10, 0, 0, 21, 20, 17, 19 }; //PT_ENT types (dont use TEXT/TEXTUP/DECAL)
            int type = typemap[e.attr1-4];
            if(e.attr2 >= 256) regularshape(type, 1+e.attr3, colorfromattr(e.attr4), e.attr2-256, 5, 200, e.o);
            else newparticle(e.o, offsetvec(e.o, e.attr2, 1+e.attr3), 1, type, colorfromattr(e.attr4));
			break;
        }
		case 5: //meter, metervs - <percent> <rgb>
		case 6:
            newparticle(e.o, vec(0, 0, 1), 1, (e.attr1==5)?13:14, colorfromattr(e.attr3))->val = min(1.0f, float(e.attr2)/100);
			break;
		case 32: //lens flares - plain/sparkle/sun/sparklesun <red> <green> <blue>
		case 33:
		case 34:
		case 35:
			makeflare(e.o, e.attr2, e.attr3, e.attr4, (e.attr1&0x02)!=0, (e.attr1&0x01)!=0);
			break;
		default:
			s_sprintfd(ds)("@particles %d?", e.attr1);
			particle_text(e.o, ds, 16, 1);
	}
}

void entity_particles()
{
	if(emit)
	{
		int emitmillis = 1000/emitfps;
		lastemitframe = lastmillis-(lastmillis%emitmillis);
		emit = false;
	}

	makelightflares();

	const vector<extentity *> &ents = et->getents();
	if (editmode)
	{
		loopv(entgroup)
		{
			entity &e = *ents[entgroup[i]];
			particle_text(e.o, entname(e), 13, 1);
		}
	}
	loopv(ents)
	{
		entity &e = *ents[i];
		if (e.type == ET_EMPTY) continue;

		if (e.type == ET_PARTICLES && e.o.dist(camera1->o) <= maxparticledistance)
			makeparticles(e);

		if (editmode)
		{
			particle_text(e.o, entname(e), 11, 1);
			if (e.type != ET_PARTICLES) regular_particle_splash(2, 2, 40, e.o);
		}
	}
}

void part_textf(const vec &s, char *t, bool moving, int fade, int color, ...)
{
    if(shadowmapping || renderedgame) return;
	s_sprintfdlv(str, color, t);
	s_sprintfd(buf)("@%s", str);
	part_text(s, buf, moving, fade, color);
}

void part_text(const vec &s, char *t, bool moving, int fade, int color)
{
    if(shadowmapping || renderedgame) return;
	if(!particletext || camera1->o.dist(s) > 128) return;
	newparticle(s, vec(0, 0, 1), fade, moving ? 9 : 11, color)->text = newstring(t);
}

void part_splash(int type, int num, int fade, const vec &p, int color)
{
    if(shadowmapping || renderedgame) return;
	if(camera1->o.dist(p) > maxparticledistance) return;
	loopi(num)
	{
		const int radius = (type==6 || type == 17) ? 50 : 150;
		int x, y, z;
		do
		{
			x = rnd(radius*2)-radius;
			y = rnd(radius*2)-radius;
			z = rnd(radius*2)-radius;
		}
		while(x*x+y*y+z*z>radius*radius);
		vec tmp = vec((float)x, (float)y, (float)z);
		newparticle(p, tmp, rnd(fade*3)+1, type, color);
	}
}

void part_trail(int type, int fade, const vec &s, const vec &e, int color)
{
    if(shadowmapping || renderedgame) return;
	vec v;
	float d = e.dist(s, v);
	v.div(d*2);
	vec p = s;
	loopi((int)d*2)
	{
		p.add(v);
		vec tmp = vec(float(rnd(11)-5), float(rnd(11)-5), float(rnd(11)-5));
		newparticle(p, tmp, rnd(fade)+fade, type, color);
	}
}

void part_meter(const vec &s, float val, int type, int fade, int color)
{
    if(shadowmapping || renderedgame) return;
	newparticle(s, vec(0, 0, 1), fade, type, color)->val = val;
}

void part_flare(const vec &p, const vec &dest, int fade, int type, int color, physent *owner)
{
    if(shadowmapping || renderedgame) return;
    newparticle(p, dest, fade, type, color)->owner = owner;
}

void part_fireball(const vec &dest, float max, int type, int color)
{
    if(shadowmapping || renderedgame) return;
	int maxsize = int(max) - 4;
	newparticle(dest, vec(0, 0, 1), maxsize*25, type, color)->val = maxsize;
}

void part_firerad(const vec &dest, float size, int type, int color)
{
    if(shadowmapping || renderedgame) return;
	newparticle(dest, vec(0, 0, 1), int(size), type, color)->val = int(size)+100;
}

void part_spawn(const vec &o, const vec &v, float z, uchar type, int amt, int fade, int color)
{
    if(shadowmapping || renderedgame) return;
	loopi(amt)
	{
		vec w(rnd(int(v.x*2))-int(v.x), rnd(int(v.y*2))-int(v.y), rnd(int(v.z*2))-int(v.z)+z);
		w.add(o);
		part_splash(type, 1, fade, w, color);
	}
}

void part_flares(const vec &o, const vec &v, float z1, const vec &d, const vec &w, float z2, uchar type, int amt, int fade, int color, physent *owner)
{
    if(shadowmapping || renderedgame) return;
	loopi(amt)
	{
		vec from(rnd(int(v.x*2))-int(v.x), rnd(int(v.y*2))-int(v.y), rnd(int(v.z*2))-int(v.z)+z1);
		from.add(o);

		vec to(rnd(int(w.x*2))-int(w.x), rnd(int(w.y*2))-int(w.y), rnd(int(w.z*2))-int(w.z)+z1);
		to.add(d);

		newparticle(from, to, fade, type, color)->owner = owner;
	}
}
