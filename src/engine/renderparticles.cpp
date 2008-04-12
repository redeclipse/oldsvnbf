// renderparticles.cpp

#include "pch.h"
#include "engine.h"
#include "rendertarget.h"

// eye space depth texture for soft particles, done at low res then blurred to prevent ugly jaggies
VARP(depthfxfpscale, 1, 1<<12, 1<<16);
VARP(depthfxscale, 1, 1<<6, 1<<8);
VARP(depthfxblend, 1, 16, 64);
VAR(depthfxmargin, 0, 16, 64);
VAR(depthfxbias, 0, 1, 64);

extern void cleanupdepthfx();
VARFP(fpdepthfx, 0, 1, 1, cleanupdepthfx());
VARFP(depthfxprecision, 0, 0, 1, cleanupdepthfx());
VARFP(depthfxsize, 6, 7, 10, cleanupdepthfx());
VARP(depthfx, 0, 1, 1);
VARP(blurdepthfx, 0, 1, 7);
VARP(blurdepthfxsigma, 1, 50, 200);
VAR(depthfxscissor, 0, 2, 2);
VAR(debugdepthfx, 0, 0, 1);

#define MAXDFXRANGES 4

void *depthfxowners[MAXDFXRANGES];
float depthfxranges[MAXDFXRANGES];
int numdepthfxranges = 0;
vec depthfxmin(1e16f, 1e16f, 1e16f), depthfxmax(1e16f, 1e16f, 1e16f);

static struct depthfxtexture : rendertarget
{    
    GLfloat mvpmatrix[16];

    const GLenum *colorformats() const
    {
        static const GLenum colorfmts[] = { GL_RGB16F_ARB, GL_RGB16, GL_RGBA, GL_RGBA8, GL_RGB, GL_RGB8, GL_FALSE };
        return &colorfmts[hasTF && hasFBO ? (fpdepthfx ? 0 : (depthfxprecision ? 1 : 2)) : 2];
    }

    void getmvpmatrix()
    {
        GLfloat mm[16], pm[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, mm);
        glGetFloatv(GL_PROJECTION_MATRIX, pm);
        loopi(4) loopj(4)
        {
            float c = 0;
            loopk(4) c += pm[k*4 + j] * mm[i*4 + k];
            mvpmatrix[i*4 + j] = c;
        }
    }

    void addscissorvert(const vec &v, float &sx1, float &sy1, float &sx2, float &sy2)
    {
        float w = v.x*mvpmatrix[3] + v.y*mvpmatrix[7] + v.z*mvpmatrix[11] + mvpmatrix[15],
              x = (v.x*mvpmatrix[0] + v.y*mvpmatrix[4] + v.z*mvpmatrix[8] + mvpmatrix[12]) / w,
              y = (v.x*mvpmatrix[1] + v.y*mvpmatrix[5] + v.z*mvpmatrix[9] + mvpmatrix[13]) / w;
        sx1 = min(sx1, x);
        sy1 = min(sy1, y);
        sx2 = max(sx2, x);
        sy2 = max(sy2, y);
    }

    bool addscissorbox(const vec &center, float size)
    {
        float sx1 = 1, sy1 = 1, sx2 = -1, sy2 = -1;
        loopi(4)
        {
            vec v(center);
            v.add(vec(i < 2 ? camright : camup).mul(i&1 ? size : -size));
            addscissorvert(v, sx1, sy1, sx2, sy2);
        }
        return addblurtiles(sx1, sy1, sx2, sy2);
    }

    bool addscissorbox(const vec &bbmin, const vec &bbmax)
    {
        float sx1 = 1, sy1 = 1, sx2 = -1, sy2 = -1;
        loopi(8)
        {
            vec v(i&1 ? bbmax.x : bbmin.x, i&2 ? bbmax.y : bbmin.y, i&4 ? bbmax.z : bbmin.z);
            addscissorvert(v, sx1, sy1, sx2, sy2);
        }
        return addblurtiles(sx1, sy1, sx2, sy2);
    }

    bool shouldrender()
    {
        extern void finddepthfxranges();
        finddepthfxranges();
        return (numdepthfxranges && scissorx1 < scissorx2 && scissory1 < scissory2) || debugdepthfx;
    }

    bool dorender()
    {
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        depthfxing = true;
        refracting = -1;
        
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

        return numdepthfxranges > 0;
    }

    void dodebug(int w, int h)
    {
        if(numdepthfxranges > 0)
        {
            glColor3f(0, 1, 0);
            debugscissor(w, h, true);
            glColor3f(0, 0, 1);
            debugblurtiles(w, h, true);
            glColor3f(1, 1, 1);
        }
    }
} depthfxtex;

void cleanupdepthfx()
{
    depthfxtex.cleanup(true);
}

void viewdepthfxtex()
{
    if(!depthfx) return;
    depthfxtex.debug();
}

bool depthfxing = false;

void drawdepthfxtex()
{
    if(!depthfx || renderpath==R_FIXEDFUNCTION) return;

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
    hemiindices[face]   = i1;
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

        if(fogging) setfogplane(1, reflectz);
    }

    if(hasVBO) 
    {
        glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
        glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
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

Shader *particleshader = NULL, *particlenotextureshader = NULL;

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

enum
{
    PT_PART = 0,
    PT_TAPE,
    PT_TRAIL,
    PT_TEXT,
    PT_TEXTUP,
    PT_METER,
    PT_METERVS,
    PT_FIREBALL,
    PT_LIGHTNING,
    PT_FLARE,

    PT_MOD   = 1<<8,
    PT_RND4  = 1<<9,
    PT_LERP  = 1<<10, // use very sparingly - order of blending issues
    PT_TRACK = 1<<11,
    PT_GLARE = 1<<12,
};

const char *partnames[] = { "part", "tape", "trail", "text", "textup", "meter", "metervs", "fireball", "lightning", "flare" };

struct particle
{
    vec o, d;
    int fade, millis;
    bvec color;
    uchar flags;
    float size;
    union
    {
        const char *text;         // will call delete[] on this only if it starts with an @
        float val;
        physent *owner;
    }; 
};

struct partvert
{
    vec pos;
    float u, v;
    bvec color;
    uchar alpha;
};

#define COLLIDERADIUS 8.0f
#define COLLIDEERROR 1.0f

struct partrenderer
{
    Texture *tex;
    const char *texname;
    uint type;
    int grav, collide;
    
    partrenderer(const char *texname, int type, int grav, int collide) 
        : tex(NULL), texname(texname), type(type), grav(grav), collide(collide)
    {
    }
    virtual ~partrenderer()
    {
    }

    virtual void init(int n) { }
    virtual void reset() = NULL;
    virtual void resettracked(physent *owner) { }   
    virtual particle *addpart(const vec &o, const vec &d, int fade, int color, float size) = NULL;    
    virtual void update() { }
    virtual void render() = NULL;
    virtual bool haswork() = NULL;
    virtual int count() = NULL; //for debug
    virtual bool usesvertexarray() { return false; } 

    //blend = 0 => remove it
    void calc(particle *p, int &blend, int &ts, vec &o, vec &d, bool lastpass = true)
    {
        o = p->o;
        d = p->d;
        if(type&PT_TRACK && p->owner) cl->particletrack(p->owner, o, d);
        if(p->fade <= 5) 
        {
            ts = 1;
            blend = 255;
        }
        else
        {
            ts = lastmillis-p->millis;
            blend = max(255 - (ts<<8)/p->fade, 0);
            if(grav)
            {
                if(ts > p->fade) ts = p->fade;
                float t = (float)(ts);
                vec v = d;
                v.mul(t/5000.0f);
                o.add(v);
                o.z -= t*t/(2.0f * 5000.0f * grav);
            }
            if(collide && o.z < p->val && lastpass)
            {
                vec surface;
                float floorz = rayfloor(vec(o.x, o.y, p->val), surface, RAY_CLIPMAT, COLLIDERADIUS);
                float collidez = floorz<0 ? o.z-COLLIDERADIUS : p->val - rayfloor(vec(o.x, o.y, p->val), surface, RAY_CLIPMAT, COLLIDERADIUS);
                if(o.z >= collidez+COLLIDEERROR) 
                    p->val = collidez+COLLIDEERROR;
                else 
                {
                    adddecal(collide, vec(o.x, o.y, collidez), vec(p->o).sub(o).normalize(), 2*p->size, p->color, type&PT_RND4 ? detrnd((size_t)p, 4) : 0);
                    blend = 0;
                }
            }
        }
    }
};

struct listparticle : particle
{   
    listparticle *next;
};

static listparticle *parempty = NULL;

VARP(outlinemeters, 0, 0, 1);

struct listrenderer : partrenderer
{
    listparticle *list;

    listrenderer(const char *texname, int type, int grav, int collide) 
        : partrenderer(texname, type, grav, collide), list(NULL)
    {
    }

    virtual ~listrenderer()
    {
    }

    virtual void cleanup(listparticle *p)
    {
    }

    void reset()  
    {
        if(!list) return;
        listparticle *p = list;
        for(;;)
        {
            cleanup(p);
            if(p->next) p = p->next;
            else break;
        }
        p->next = parempty;
        parempty = list;
        list = NULL;
    }
    
    void resettracked(physent *owner) 
    {
        if(!(type&PT_TRACK)) return;
        for(listparticle **prev = &list, *cur = list; cur; cur = *prev)
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
    
    particle *addpart(const vec &o, const vec &d, int fade, int color, float size) 
    {
        if(!parempty)
        {
            listparticle *ps = new listparticle[256];
            loopi(255) ps[i].next = &ps[i+1];
            ps[255].next = parempty;
            parempty = ps;
        }
        listparticle *p = parempty;
        parempty = p->next;
        p->next = list;
        list = p;
        p->o = o;
        p->d = d;
        p->fade = fade;
        p->millis = lastmillis;
        p->color = bvec(color>>16, (color>>8)&0xFF, color&0xFF);
        p->size = size;
        p->owner = NULL;
        return p;
    }
    
    int count() 
    {
        int num = 0;
        listparticle *lp;
        for(lp = list; lp; lp = lp->next) num++;
        return num;
    }
    
    bool haswork() 
    {
        return (list != NULL);
    }
    
    virtual void startrender() = 0;
    virtual void endrender() = 0;
    virtual void renderpart(listparticle *p, const vec &o, const vec &d, int blend, int ts, uchar *color) = 0;

    void render() 
    {
        startrender();
        if(texname)
        {
            if(!tex) tex = textureload(texname);
            glBindTexture(GL_TEXTURE_2D, tex->id);
        }
        
        bool lastpass = !reflecting && !refracting;
        
        for(listparticle **prev = &list, *p = list; p; p = *prev)
        {   
            vec o, d;
            int blend, ts;
            calc(p, blend, ts, o, d, lastpass);
            if(blend > 0) 
            {
                renderpart(p, o, d, blend, ts, p->color.v);

                if(p->fade > 5 || !lastpass) 
                {
                    prev = &p->next;
                    continue;
                }
            }
            //remove
            *prev = p->next;
            p->next = parempty;
            cleanup(p);
            parempty = p;
        }
       
        endrender();
    }
};

struct lightningrenderer : listrenderer
{
    lightningrenderer()
        : listrenderer("textures/lightning.jpg", PT_LIGHTNING|PT_TRACK|PT_GLARE, 0, 0)
    {}

    void startrender()
    {
        glDisable(GL_CULL_FACE);
    }

    void endrender()
    {
        glEnable(GL_CULL_FACE); 
    }

    void update()
    {
        setuplightning();
    }

    void renderpart(listparticle *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
    {
        blend = min(blend<<2, 255);
        if(type&PT_MOD) //multiply alpha into color
            glColor3ub((color[0]*blend)>>8, (color[1]*blend)>>8, (color[2]*blend)>>8);
        else
            glColor4ub(color[0], color[1], color[2], blend);
        float tx = 0, ty = 0, tsz = 1;
        if(type&PT_RND4)
        {
            int i = detrnd((size_t)p, 4);
            tx = 0.5f*(i&1);
            ty = 0.5f*((i>>1)&1);
            tsz = 0.5f;
        }
        renderlightning(o, d, p->size, tx, ty, tsz);
    }
};
static lightningrenderer lightnings;

struct meterrenderer : listrenderer
{
    meterrenderer(int type)
        : listrenderer(NULL, type, 0, 0)
    {}

    void startrender()
    {
         glDisable(GL_BLEND);
         glDisable(GL_TEXTURE_2D);
         particlenotextureshader->set();
    }

    void endrender()
    {
         glEnable(GL_BLEND);
         glEnable(GL_TEXTURE_2D);
         if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
         particleshader->set();
    }

    void renderpart(listparticle *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
    {
        int basetype = type&0xFF;

        glPushMatrix();
        glTranslatef(o.x, o.y, o.z);
        if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - o.z, true);
        glRotatef(camera1->yaw-180, 0, 0, 1);
        glRotatef(camera1->pitch-90, 1, 0, 0);

        float scale = p->size/80.0f;
        glScalef(-scale, scale, -scale);

        float right = 8*FONTH, left = p->val*right;
        glTranslatef(-right/2.0f, 0, 0);
        glColor3ubv(color);
        glBegin(GL_TRIANGLE_STRIP);
        loopk(10)
        {
            float c = -0.5f*sinf(k/9.0f*M_PI), s = 0.5f + 0.5f*cosf(k/9.0f*M_PI);
            glVertex2f(left - c*FONTH, s*FONTH);
            glVertex2f(c*FONTH, s*FONTH);
        }
        glEnd();

        if(basetype==PT_METERVS) glColor3ub(color[2], color[1], color[0]); //swap r<->b                    
        else glColor3f(0, 0, 0);
        glBegin(GL_TRIANGLE_STRIP);
        loopk(10)
        {
            float c = 0.5f*sinf(k/9.0f*M_PI), s = 0.5f - 0.5f*cosf(k/9.0f*M_PI);
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
                float c = -0.5f*sinf(k/9.0f*M_PI), s = 0.5f + 0.5f*cosf(k/9.0f*M_PI);
                glVertex2f(c*FONTH, s*FONTH);
            }
            loopk(10)
            {
                float c = 0.5f*sinf(k/9.0f*M_PI), s = 0.5f - 0.5f*cosf(k/9.0f*M_PI);
                glVertex2f(right + c*FONTH, s*FONTH);
            }
            glEnd();
           
            glBegin(GL_LINE_STRIP);
            loopk(10)
            {
                float c = 0.5f*sinf(k/9.0f*M_PI), s = 0.5f - 0.5f*cosf(k/9.0f*M_PI);
                glVertex2f(left + c*FONTH, s*FONTH);
            }
            glEnd();
        }

        glPopMatrix();
    }
};
static meterrenderer meters(PT_METER|PT_LERP), metervs(PT_METERVS|PT_LERP);

static const float WOBBLE = 1.25f;

struct fireballrenderer : listrenderer
{
    fireballrenderer(int type)
        : listrenderer("textures/explosion.jpg", type, 0, 0)
    {}

    void startrender()
    {
        setupexplosion();
    }

    void endrender()
    {
        cleanupexplosion();
        particleshader->set();
    }

    int finddepthfxranges(void **owners, float *ranges, int maxranges, vec &bbmin, vec &bbmax)
    {
        GLfloat mm[16];
        glGetFloatv(GL_MODELVIEW_MATRIX, mm);

        physent e;
        e.type = ENT_CAMERA;

        int numranges = 0;
        for(listparticle *p = list; p; p = p->next)
        {
            int ts = p->fade <= 5 ? 1 : lastmillis-p->millis;
            float pmax = p->val, 
                  size = p->fade ? float(ts)/p->fade : 1,
                  psize = (p->size + pmax * size)*WOBBLE;
            if(2*(p->size + pmax)*WOBBLE < depthfxblend || isvisiblesphere(psize, p->o) >= VFC_FOGGED) continue;

            e.o = p->o;
            e.radius = e.height = e.aboveeye = psize;
            if(::collide(&e, vec(0, 0, 0), 0, false)) continue;
  
            if(depthfxscissor==2 && !depthfxtex.addscissorbox(p->o, psize)) continue;

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

    void renderpart(listparticle *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
    {
        float pmax = p->val,
              size = p->fade ? float(ts)/p->fade : 1,
              psize = p->size + pmax * size;

        if(isvisiblesphere(psize*WOBBLE, p->o) >= VFC_FOGGED) return;

        glPushMatrix();
        glTranslatef(o.x, o.y, o.z);
        if(fogging)
        {
            if(renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - o.z, true);
            else blend = (uchar)(blend * max(0.0f, min(1.0f, (reflectz - o.z)/waterfog)));
        }

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

        glPopMatrix();
    }
};
static fireballrenderer fireballs(PT_FIREBALL|PT_GLARE), noglarefireballs(PT_FIREBALL);

void finddepthfxranges()
{
    depthfxtex.getmvpmatrix();

    depthfxmin = vec(1e16f, 1e16f, 1e16f);
    depthfxmax = vec(0, 0, 0);
    numdepthfxranges = fireballs.finddepthfxranges(depthfxowners, depthfxranges, MAXDFXRANGES, depthfxmin, depthfxmax);
    loopk(3)
    {
        depthfxmin[k] -= depthfxmargin;
        depthfxmax[k] += depthfxmargin;
    }

    if(depthfxscissor<2 && numdepthfxranges>0) depthfxtex.addscissorbox(depthfxmin, depthfxmax);
}

struct textrenderer : listrenderer
{
    textrenderer(int type, int grav = 0)
        : listrenderer(NULL, type, grav, 0)
    {}

    void startrender()
    {
    }

    void endrender()
    {
        if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
    }

    void cleanup(listparticle *p)
    {
        if(p->text && p->text[0]=='@') delete[] p->text;
    }

    void renderpart(listparticle *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
    {
        glPushMatrix();
        glTranslatef(o.x, o.y, o.z);
        if(fogging)
        {
            if(renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - o.z, true);
            else blend = (uchar)(blend * max(0.0f, min(1.0f, (reflectz - o.z)/waterfog)));
        }

        glRotatef(camera1->yaw-180, 0, 0, 1);
        glRotatef(camera1->pitch-90, 1, 0, 0);

        float scale = p->size/80.0f;
        glScalef(-scale, scale, -scale);

        const char *text = p->text+(p->text[0]=='@' ? 1 : 0);
        float xoff = -text_width(text)/2;
        float yoff = 0;
        if((type&0xFF)==PT_TEXTUP) { xoff += detrnd((size_t)p, 100)-50; yoff -= detrnd((size_t)p, 101); } //@TODO instead in worldspace beforehand?
        glTranslatef(xoff, yoff, 50);

        draw_text(text, 0, 0, color[0], color[1], color[2], blend);

        glPopMatrix();
    } 
};
static textrenderer texts(PT_TEXT|PT_LERP), textups(PT_TEXTUP|PT_LERP, -8);

template<int T>
static inline void modifyblend(const vec &o, int &blend)
{
    blend = min(blend<<2, 255);
    if(renderpath==R_FIXEDFUNCTION && fogging) blend = (uchar)(blend * max(0.0f, min(1.0f, (reflectz - o.z)/waterfog)));
}

template<>
inline void modifyblend<PT_TAPE>(const vec &o, int &blend)
{
}

template<int T>
static inline void genpos(const vec &o, const vec &d, float size, int grav, int ts, partvert *vs)
{
    vec udir = vec(camup).sub(camright).mul(size);
    vec vdir = vec(camup).add(camright).mul(size);
    vs[0].pos = vec(o.x + udir.x, o.y + udir.y, o.z + udir.z);
    vs[1].pos = vec(o.x + vdir.x, o.y + vdir.y, o.z + vdir.z);
    vs[2].pos = vec(o.x - udir.x, o.y - udir.y, o.z - udir.z);
    vs[3].pos = vec(o.x - vdir.x, o.y - vdir.y, o.z - vdir.z);
}

template<>
inline void genpos<PT_TAPE>(const vec &o, const vec &d, float size, int ts, int grav, partvert *vs)
{
    vec dir1 = d, dir2 = d, c;
    dir1.sub(o);
    dir2.sub(camera1->o);
    c.cross(dir2, dir1).normalize().mul(size);
    vs[0].pos = vec(d.x-c.x, d.y-c.y, d.z-c.z);
    vs[1].pos = vec(o.x-c.x, o.y-c.y, o.z-c.z);
    vs[2].pos = vec(o.x+c.x, o.y+c.y, o.z+c.z);
    vs[3].pos = vec(d.x+c.x, d.y+c.y, d.z+c.z);
}

template<>
inline void genpos<PT_TRAIL>(const vec &o, const vec &d, float size, int ts, int grav, partvert *vs)
{
    vec e = d;
    if(grav) e.z -= float(ts)/grav;
    e.div(-75.0f);
    e.add(o);
    genpos<PT_TAPE>(o, e, size, ts, grav, vs);
}

template<int T>
struct varenderer : partrenderer
{
    partvert *verts;
    particle *parts;
    int maxparts, numparts, lastupdate;

    varenderer(const char *texname, int type, int grav, int collide) 
        : partrenderer(texname, type, grav, collide),
          verts(NULL), parts(NULL), maxparts(0), numparts(0), lastupdate(-1)
    {
    }
    
    void init(int n)
    {
        DELETEA(parts);
        DELETEA(verts);
        parts = new particle[n];
        verts = new partvert[n*4];
        maxparts = n;
        numparts = 0;
        lastupdate = -1;
    }
        
    void reset() 
    {
        numparts = 0;
        lastupdate = -1;
    }
    
    void resettracked(physent *owner) 
    {
        if(!(type&PT_TRACK)) return;
        loopi(numparts)
        {
            particle *p = parts+i;
            if(!owner || (p->owner == owner)) p->fade = -1;
        }
        lastupdate = -1;
    }
    
    int count() 
    {
        return numparts;
    }
    
    bool haswork() 
    {
        return (numparts > 0);
    }

    bool usesvertexarray() { return true; }

    particle *addpart(const vec &o, const vec &d, int fade, int color, float size) 
    {
        particle *p = parts + (numparts < maxparts ? numparts++ : rnd(maxparts)); //next free slot, or kill a random kitten
        p->o = o;
        p->d = d;
        p->fade = fade;
        p->millis = lastmillis;
        p->color = bvec(color>>16, (color>>8)&0xFF, color&0xFF);
        p->size = size;
        p->owner = NULL;
        p->flags = 0x80;
        int offset = p-parts;
        if(type&PT_RND4) p->flags |= detrnd(offset, 4)<<2;
        if((type&0xFF)==PT_PART) p->flags |= detrnd(offset*offset+37, 4);
        lastupdate = -1;
        return p;
    }
  
    void genverts(particle *p, partvert *vs, bool regen)
    {
        vec o, d;
        int blend, ts;

        calc(p, blend, ts, o, d);
        if(blend <= 1 || p->fade <= 5) p->fade = -1; //mark to remove on next pass (i.e. after render)

        modifyblend<T>(o, blend);

        if(regen)
        {
            p->flags &= ~0x80;

            int orient = p->flags&3;
            #define SETTEXCOORDS(u1, u2, v1, v2) \
            do { \
                vs[orient].u       = u1; \
                vs[orient].v       = v2; \
                vs[(orient+1)&3].u = u2; \
                vs[(orient+1)&3].v = v2; \
                vs[(orient+2)&3].u = u2; \
                vs[(orient+2)&3].v = v1; \
                vs[(orient+3)&3].u = u1; \
                vs[(orient+3)&3].v = v1; \
            } while(0)    
            if(type&PT_RND4)
            {
                float tx = 0.5f*((p->flags>>2)&1), ty = 0.5f*((p->flags>>3)&1);
                SETTEXCOORDS(tx, tx + 0.5f, ty, ty + 0.5f);
            } 
            else SETTEXCOORDS(0, 1, 0, 1);

            #define SETCOLOR(r, g, b, a) \
            do { \
                uchar col[4] = { r, g, b, a }; \
                loopi(4) memcpy(vs[i].color.v, col, sizeof(col)); \
            } while(0) 
            #define SETMODCOLOR SETCOLOR((p->color[0]*blend)>>8, (p->color[1]*blend)>>8, (p->color[2]*blend)>>8, 255)
            if(type&PT_MOD) SETMODCOLOR;
            else SETCOLOR(p->color[0], p->color[1], p->color[2], blend);
        }
        else if(type&PT_MOD) SETMODCOLOR;
        else loopi(4) vs[i].alpha = blend;

        genpos<T>(o, d, p->size, ts, grav, vs); 
    }

    void update()
    {
        if(lastmillis == lastupdate) return;
        lastupdate = lastmillis;
      
        loopi(numparts)
        {
            particle *p = &parts[i];
            partvert *vs = &verts[i*4];
            if(p->fade < 0)
            {
                do 
                {
                    --numparts; 
                    if(numparts <= i) return;
                }
                while(parts[numparts].fade < 0);
                *p = parts[numparts];
                genverts(p, vs, true);
            }
            else genverts(p, vs, (p->flags&0x80)!=0);
        }
    }
    
    void render()
    {   
        if(!tex) tex = textureload(texname);
        glBindTexture(GL_TEXTURE_2D, tex->id);
        glVertexPointer(3, GL_FLOAT, sizeof(partvert), &verts->pos);
        glTexCoordPointer(2, GL_FLOAT, sizeof(partvert), &verts->u);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(partvert), &verts->color);
        glDrawArrays(GL_QUADS, 0, numparts*4);
    }
};
typedef varenderer<PT_PART> quadrenderer;
typedef varenderer<PT_TAPE> taperenderer;
typedef varenderer<PT_TRAIL> trailrenderer;

static struct flaretype 
{
    int type;             /* flaretex index, 0..5, -1 for 6+random shine */
    float loc;            /* postion on axis */
    float scale;          /* texture scaling */
    uchar alpha;          /* color alpha */
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
                 
VAR(flarelights, 0, 0, 1);
VARP(flarecutoff, 0, 1000, 10000);
VARP(flaresize, 20, 100, 500);

struct flarerenderer : partrenderer
{
    int maxflares, numflares;
    unsigned int shinetime;
    flare *flares;
    
    flarerenderer(const char *texname, int maxflares) 
        : partrenderer(texname, PT_FLARE, 0, 0), maxflares(maxflares), shinetime(0)
    {
        flares = new flare[maxflares];
    }
    void reset() 
    {
        numflares = 0;
    }

    void newflare(vec &o,  const vec &center, uchar r, uchar g, uchar b, float mod, float size, bool sun, bool sparkle)
    {
        if(numflares >= maxflares) return;
        vec target; //occlusion check (neccessary as depth testing is turned off)
        if(!raycubelos(o, camera1->o, target)) return;
        flare &f = flares[numflares++];
        f.o = o;
        f.center = center;
        f.size = size;
        f.color[0] = uchar(r*mod);
        f.color[1] = uchar(g*mod);
        f.color[2] = uchar(b*mod);
        f.sparkle = sparkle;
    }
    
    void addflare(vec &o, uchar r, uchar g, uchar b, bool sun, bool sparkle) 
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
    
    void makelightflares()
    {
        numflares = 0; //regenerate flarelist each frame
        shinetime = lastmillis/10;
        
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
    
    int count()
    {
        return numflares;
    }
    
    bool haswork() 
    {
        return (numflares != 0) && !glaring && !reflecting  && !refracting;
    }
    
    void render()
    {
        bool fog = glIsEnabled(GL_FOG)==GL_TRUE;
        if(fog) glDisable(GL_FOG);
        defaultshader->set();
        glDisable(GL_DEPTH_TEST);
        if(!tex) tex = textureload(texname);
        glBindTexture(GL_TEXTURE_2D, tex->id);
        glBegin(GL_QUADS);
        loopi(numflares)
        {
            flare *f = flares+i;
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
                    shinetime = (shinetime + 1) % 10;
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
                glTexCoord2f(tx,     ty+tsz); glVertex3f(o.x+(-camright.x+camup.x)*sz, o.y+(-camright.y+camup.y)*sz, o.z+(-camright.z+camup.z)*sz);
                glTexCoord2f(tx+tsz, ty+tsz); glVertex3f(o.x+( camright.x+camup.x)*sz, o.y+( camright.y+camup.y)*sz, o.z+( camright.z+camup.z)*sz);
                glTexCoord2f(tx+tsz, ty);     glVertex3f(o.x+( camright.x-camup.x)*sz, o.y+( camright.y-camup.y)*sz, o.z+( camright.z-camup.z)*sz);
                glTexCoord2f(tx,     ty);     glVertex3f(o.x+(-camright.x-camup.x)*sz, o.y+(-camright.y-camup.y)*sz, o.z+(-camright.z-camup.z)*sz);
            }
        }
        glEnd();
        glEnable(GL_DEPTH_TEST);
        if(fog) glEnable(GL_FOG);
    }
    
    //square per round hole - use addflare(..) instead
    particle *addpart(const vec &o, const vec &d, int fade, int color, float size) { return NULL; }
};
static flarerenderer flares("textures/lensflares.png", 64);

static partrenderer *parts[] = 
{
    new quadrenderer("textures/blood.png", PT_PART|PT_MOD|PT_RND4, 2, 1), // 0 blood spats (note: rgb is inverted) 
    new quadrenderer("textures/spark.png", PT_PART|PT_GLARE,   2, 0),     // 1 sparks
    new quadrenderer("textures/smoke.png", PT_PART,          -20, 0),     // 2 small slowly rising smoke
    new quadrenderer("textures/base.png",  PT_PART|PT_GLARE,  20, 0),     // 3 edit mode entities
    new quadrenderer("textures/ball1.png", PT_PART|PT_GLARE,  20, 0),     // 4 fireball1
    new quadrenderer("textures/smoke.png", PT_PART,          -20, 0),     // 5 big  slowly rising smoke   
    new quadrenderer("textures/ball2.png", PT_PART|PT_GLARE,  20, 0),     // 6 fireball2
    new quadrenderer("textures/ball3.png", PT_PART|PT_GLARE,  20, 0),     // 7 big fireball3
    &textups,                                                             // 8 TEXT, floats up
    new taperenderer("textures/flare.jpg", PT_TAPE|PT_GLARE,   0, 0),     // 9 streak
    &texts,                                                               // 10 TEXT, SMALL, NON-MOVING
    &meters,                                                              // 11 METER, SMALL, NON-MOVING
    &metervs,                                                             // 12 METER vs., SMALL, NON-MOVING
    new quadrenderer("textures/smoke.png", PT_PART,           20, 0),     // 13 small  slowly sinking smoke trail
    &fireballs,                                                           // 14 explosion fireball
    &lightnings,                                                          // 15 lightning
    new quadrenderer("textures/smoke.png", PT_PART,          -15, 0),     // 16 big  fast rising smoke          
    new trailrenderer("textures/base.png", PT_TRAIL|PT_LERP,   2, 0),     // 17 water, entity
    &noglarefireballs,                                                    // 18 explosion fireball no glare
    &flares // must be done last
};

VARFP(maxparticles, 10, 4000, 40000, particleinit());

void particleinit() 
{
    if(!particleshader) particleshader = lookupshaderbyname("particle");
    if(!particlenotextureshader) particlenotextureshader = lookupshaderbyname("particlenotexture");
    loopi(sizeof(parts)/sizeof(parts[0])) parts[i]->init(maxparticles);
}

void clearparticles()
{   
    loopi(sizeof(parts)/sizeof(parts[0])) parts[i]->reset();
}   

void removetrackedparticles(physent *owner)
{
    loopi(sizeof(parts)/sizeof(parts[0])) parts[i]->resettracked(owner);
}

VARP(particleglare, 0, 4, 100);

VAR(debugparticles, 0, 0, 1);

void render_particles(int time)
{
    //want to debug BEFORE the lastpass render (that would delete particles)
    if(debugparticles && !glaring && !reflecting && !refracting) 
    {
        int n = sizeof(parts)/sizeof(parts[0]);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, FONTH*n*2, FONTH*n*2, 0, -1, 1); //squeeze into top-left corner        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        defaultshader->set();
        loopi(n) 
        {
            int type = parts[i]->type;
            const char *title = parts[i]->texname ? strrchr(parts[i]->texname, '/')+1 : NULL;
            string info = "";
            if(type&PT_GLARE) s_strcat(info, "g,");
            if(type&PT_LERP) s_strcat(info, "l,");
            if(type&PT_MOD) s_strcat(info, "m,");
            if(type&PT_RND4) s_strcat(info, "r,");
            if(type&PT_TRACK) s_strcat(info, "t,");
            if(parts[i]->collide) s_strcat(info, "c,");
            s_sprintfd(ds)("%d\t%s(%s%d) %s", parts[i]->count(), partnames[type&0xFF], info, parts[i]->grav, (title?title:""));
            draw_text(ds, FONTH, (i+n/2)*FONTH);
        }
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }

    if(glaring && !particleglare) return;
   
    loopi(sizeof(parts)/sizeof(parts[0]))
    {
        if(glaring && !(parts[i]->type&PT_GLARE)) continue;
        parts[i]->update();
    }
    
    static float zerofog[4] = { 0, 0, 0, 1 };
    float oldfogc[4];
    bool rendered = false;
    uint lastflags = PT_LERP;
    
    loopi(sizeof(parts)/sizeof(parts[0]))
    {
        partrenderer *p = parts[i];
        if(glaring && !(p->type&PT_GLARE)) continue;
        if(!p->haswork()) continue;
    
        if(!rendered)
        {
            rendered = true;
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);             

            if(glaring) setenvparamf("colorscale", SHPARAM_VERTEX, 4, particleglare, particleglare, particleglare, 1);
            else setenvparamf("colorscale", SHPARAM_VERTEX, 4, 1, 1, 1, 1);

            particleshader->set();
            glGetFloatv(GL_FOG_COLOR, oldfogc);
        }
        
        uint flags = p->type & (PT_LERP|PT_MOD);
        if(p->usesvertexarray()) flags |= 0x01; //0x01 = VA marker
        uint changedbits = (flags ^ lastflags);
        if(changedbits != 0x0000)
        {
            if(changedbits&0x01)
            {
                if(flags&0x01)
                {
                    glEnableClientState(GL_VERTEX_ARRAY);
                    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    glEnableClientState(GL_COLOR_ARRAY);
                } 
                else
                {
                    glDisableClientState(GL_VERTEX_ARRAY);
                    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                    glDisableClientState(GL_COLOR_ARRAY);
                }
            }
            if(changedbits&PT_LERP) glFogfv(GL_FOG_COLOR, (flags&PT_LERP) ? oldfogc : zerofog);
            if(changedbits&(PT_LERP|PT_MOD))
            {
                if(flags&PT_LERP) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                else if(flags&PT_MOD) glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
                else glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            }
            lastflags = flags;        
        }
        p->render();
    }

    if(rendered)
    {
        if(lastflags&(PT_LERP|PT_MOD)) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        if(!(lastflags&PT_LERP)) glFogfv(GL_FOG_COLOR, oldfogc);
        if(lastflags&0x01)
        {
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
        }
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }
}

static inline particle *newparticle(const vec &o, const vec &d, int fade, int type, int color, float size)
{
    return parts[type]->addpart(o, d, fade, color, size);
}

VARP(maxparticledistance, 256, 512, 4096);

static void splash(int type, int color, int radius, int num, int fade, const vec &p, float size)
{
    if(camera1->o.dist(p) > maxparticledistance) return;
    float collidez = parts[type]->collide ? p.z - raycube(p, vec(0, 0, -1), COLLIDERADIUS, RAY_CLIPMAT) + COLLIDEERROR : -1; 
    int fmin = 1;
    int fmax = fade*3;
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
        int f = (num < 10) ? (fmin + rnd(fmax)) : (fmax - (i*(fmax-fmin))/(num-1)); //help deallocater by using fade distribution rather than random
        newparticle(p, tmp, f, type, color, size)->val = collidez;
    }
}

static void regularsplash(int type, int color, int radius, int num, int fade, const vec &p, float size, int delay=0) 
{
    if(!emit_particles() || (delay > 0 && rnd(delay) != 0)) return;
    splash(type, color, radius, num, fade, p, size);
}

//maps 'classic' particles types to newer types and colors
// @NOTE potentially this and the following public funcs can be tidied up, but lets please defer that for a little bit...
static struct partmap { int type; int color; float size; } partmaps[] = 
{
    {  1, 0xB49B4B, 0.24f}, // 0 yellow: sparks 
    {  2, 0x897661, 0.6f }, // 1 greyish-brown:   small slowly rising smoke
    {  3, 0x3232FF, 0.32f}, // 2 blue:   edit mode entities
    {  0, 0x60FFFF, 2.96f}, // 3 red:    blood spats (note: rgb is inverted)
    {  4, 0xFFC8C8, 4.8f }, // 4 yellow: fireball1
    {  5, 0x897661, 2.4f }, // 5 greyish-brown:   big  slowly rising smoke   
    {  6, 0xFFFFFF, 4.8f }, // 6 blue:   fireball2
    {  7, 0xFFFFFF, 4.8f }, // 7 green:  big fireball3
    {  8, 0xFF4B19, 4.0f }, // 8 TEXT RED
    {  8, 0x32FF64, 4.0f }, // 9 TEXT GREEN
    {  9, 0xFFC864, 0.28f}, // 10 yellow flare
    { 10, 0x1EC850, 2.0f }, // 11 TEXT DARKGREEN, SMALL, NON-MOVING
    {  7, 0xFFFFFF, 2.0f }, // 12 green small fireball3
    { 10, 0xFF4B19, 2.0f }, // 13 TEXT RED, SMALL, NON-MOVING
    { 10, 0xB4B4B4, 2.0f }, // 14 TEXT GREY, SMALL, NON-MOVING
    {  8, 0xFFC864, 4.0f }, // 15 TEXT YELLOW
    { 10, 0x6496FF, 2.0f }, // 16 TEXT BLUE, SMALL, NON-MOVING
    { 11, 0xFF1932, 2.0f }, // 17 METER RED, SMALL, NON-MOVING
    { 11, 0x3219FF, 2.0f }, // 18 METER BLUE, SMALL, NON-MOVING
    { 12, 0xFF1932, 2.0f }, // 19 METER RED vs. BLUE, SMALL, NON-MOVING (note swaps r<->b)
    { 12, 0x3219FF, 2.0f }, // 20 METER BLUE vs. RED, SMALL, NON-MOVING (note swaps r<->b)
    { 13, 0x897661, 0.6f }, // 21 greyish-brown:   small  slowly sinking smoke trail
    { 14, 0xFF8080, 4.0f }, // 22 red explosion fireball
    { 14, 0xA0C080, 4.0f }, // 23 orange explosion fireball
    /* @UNUSED */ { -1, 0, 0.0f}, // 24
    { 16, 0x897661, 2.4f }, // 25 greyish-brown:   big  fast rising smoke          
    /* @UNUSED */ { -1, 0, 0.0f}, // 26  
    /* @UNUSED */ { -1, 0, 0.0f}, // 27
    { 15, 0xFFFFFF, 0.28f}, // 28 lightning
    { 15, 0xFF2222, 0.28f}, // 29 lightning: red
    { 15, 0x2222FF, 0.28f}, // 30 lightning: blue
    { 18, 0x802020, 4.8f }, // 31 fireball: red, no glare
    { 18, 0x2020FF, 4.8f }, // 32 fireball: blue, no glare
    { 18, 0x208020, 4.8f }, // 33 fireball: green, no glare
    {  8, 0x6496FF, 4.0f }, // 34 TEXT BLUE
    { 14, 0x802020, 4.8f }, // 35 fireball: red
    { 14, 0x2020FF, 4.8f }, // 36 fireball: blue
    // fill the above @UNUSED slots first!
};

static inline float partsize(int type)
{
    return partmaps[type].size * particlesize/100.0f;
}

void regular_particle_splash(int type, int num, int fade, const vec &p, int delay) 
{
    if(shadowmapping) return;
    int radius = (type==5 || type == 25) ? 50 : 150;
    regularsplash(partmaps[type].type, partmaps[type].color, radius, num, fade, p, partsize(type), delay);
}

void part_splash(int type, int num, int fade, const vec &p, int color, float size)
{
    if(shadowmapping || renderedgame) return;
    splash(type, color, 150, num, fade, p, size);
}

void particle_splash(int type, int num, int fade, const vec &p) 
{
    part_splash(partmaps[type].type, num, fade, p, partmaps[type].color, partsize(type));
}

VARP(maxtrail, 1, 500, 10000);

void part_trail(int ptype, int fade, const vec &s, const vec &e, int color, float size)
{
    if(shadowmapping || renderedgame) return;
    vec v;
    float d = e.dist(s, v);
    int steps = clamp(int(d*2), 1, maxtrail);
    v.div(steps);
    vec p = s;
    loopi(steps)
    {
        p.add(v);
        vec tmp = vec(float(rnd(11)-5), float(rnd(11)-5), float(rnd(11)-5));
        newparticle(p, tmp, rnd(fade)+fade, ptype, color, size);
    }
}

void particle_trail(int type, int fade, const vec &s, const vec &e)
{
    part_trail(partmaps[type].type, fade, s, e, partmaps[type].color, partsize(type));
}

VARP(particletext, 0, 1, 1);

void part_text(const vec &s, const char *t, int type, int fade, int color, float size)
{
    if(shadowmapping || renderedgame) return;
    if(!particletext || camera1->o.dist(s) > 128) return;
    if(t[0]=='@') t = newstring(t);
    newparticle(s, vec(0, 0, 1), fade, type, color, size)->text = t;
}

void particle_text(const vec &s, const char *t, int type, int fade)
{
    part_text(s, t, partmaps[type].type, fade, partmaps[type].color, partmaps[type].size);
}

void part_meter(const vec &s, float val, int type, int fade, int color, float size)
{
    if(shadowmapping || renderedgame) return;
    newparticle(s, vec(0, 0, 1), fade, type, color, size)->val = val;
}

void particle_meter(const vec &s, float val, int type, int fade)
{
    part_meter(s, val, partmaps[type].type, fade, partmaps[type].color, partmaps[type].size);
}

void part_flare(const vec &p, const vec &dest, int fade, int type, int color, float size, physent *owner)
{
    if(shadowmapping || renderedgame) return;
    newparticle(p, dest, fade, type, color, size)->owner = owner;
}

void particle_flare(const vec &p, const vec &dest, int fade, int type, physent *owner)
{
    part_flare(p, dest, fade, partmaps[type].type, partmaps[type].color, partsize(type), owner);
}

void part_fireball(const vec &dest, float maxsize, int type, int fade, int color, float size)
{
    if(shadowmapping || renderedgame) return;
    float growth = maxsize - size;
    if(fade < 0) fade = int(growth*25);
    newparticle(dest, vec(0, 0, 1), fade, type, color, size)->val = growth;
}

void particle_fireball(const vec &dest, float maxsize, int type, int fade)
{
    part_fireball(dest, maxsize, partmaps[type].type, fade, partmaps[type].color, partsize(type));
}

void part_spawn(const vec &o, const vec &v, float z, uchar type, int amt, int fade, int color, float size)
{
    if(shadowmapping || renderedgame) return;
    loopi(amt)
    {
        vec w(rnd(int(v.x*2))-int(v.x), rnd(int(v.y*2))-int(v.y), rnd(int(v.z*2))-int(v.z)+z);
        w.add(o);
        part_splash(type, 1, fade, w, color, size);
    }
}

void part_flares(const vec &o, const vec &v, float z1, const vec &d, const vec &w, float z2, uchar type, int amt, int fade, int color, float size, physent *owner)
{
    if(shadowmapping || renderedgame) return;
    loopi(amt)
    {
        vec from(rnd(int(v.x*2))-int(v.x), rnd(int(v.y*2))-int(v.y), rnd(int(v.z*2))-int(v.z)+z1);
        from.add(o);

        vec to(rnd(int(w.x*2))-int(w.x), rnd(int(w.y*2))-int(w.y), rnd(int(w.z*2))-int(w.z)+z1);
        to.add(d);

        newparticle(from, to, fade, type, color, size)->owner = owner;
    }
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
void regularshape(int type, int radius, int color, int dir, int num, int fade, const vec &p, float size)
{
    if(!emit_particles()) return;
    
    int basetype = parts[type]->type&0xFF;
    bool flare = (basetype == PT_TAPE) || (basetype == PT_LIGHTNING);
    
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
            newparticle(inv?to:from, inv?from:to, rnd(fade*3)+1, type, color, size);
        else 
        {  
            vec d(to);
            d.sub(from);
            d.normalize().mul(inv ? -200.0f : 200.0f); //velocity
            newparticle(inv?to:from, d, rnd(fade*3)+1, type, color, size);
        }
    }
}

static void makeparticles(entity &e) 
{
    switch(e.attr1)
    {
        case 0: //fire
            regularsplash(4, 0xFFC8C8, 150, 1, 40, e.o, 4.8);
            regularsplash(5, 0x897661, 50, 1, 200,  vec(e.o.x, e.o.y, e.o.z+3.0), 2.4, 3);
            break;
        case 1: //smoke vent - <dir>
            regularsplash(5, 0x897661, 50, 1, 200,  offsetvec(e.o, e.attr2, rnd(10)), 2.4);
            break;
        case 2: //water fountain - <dir>
        {
            uchar col[3];
            getwatercolour(col);
            int color = (col[0]<<16) | (col[1]<<8) | col[2];
            regularsplash(17, color, 150, 4, 200, offsetvec(e.o, e.attr2, rnd(10)), 0.6);
            break;
        }
        case 3: //fire ball - <size> <rgb>
            newparticle(e.o, vec(0, 0, 1), 1, 14, colorfromattr(e.attr3), 4.0)->val = 1+e.attr2;
            break;
        case 4:  //tape - <dir> <length> <rgb>
        case 7:  //lightning 
        case 8:  //fire
        case 9:  //smoke
        case 10: //water
        {
            const int typemap[]   = {    9,  -1,  -1,   15,   4,   5,   17 };
            const float sizemap[] = { 0.28, 0.0, 0.0, 0.28, 4.8, 2.4, 0.60 };
            int type = typemap[e.attr1-4];
            float size = sizemap[e.attr1-4];
            if(e.attr2 >= 256) regularshape(type, 1+e.attr3, colorfromattr(e.attr4), e.attr2-256, 5, 200, e.o, size);
            else newparticle(e.o, offsetvec(e.o, e.attr2, 1+e.attr3), 1, type, colorfromattr(e.attr4), size);
            break;
        }
        case 5: //meter, metervs - <percent> <rgb>
        case 6:
            newparticle(e.o, vec(0, 0, 1), 1, (e.attr1==5)?11:12, colorfromattr(e.attr3), 2.0)->val = min(1.0f, float(e.attr2)/100);
            break;
        case 32: //lens flares - plain/sparkle/sun/sparklesun <red> <green> <blue>
        case 33:
        case 34:
        case 35:
            flares.addflare(e.o, e.attr2, e.attr3, e.attr4, (e.attr1&0x02)!=0, (e.attr1&0x01)!=0);
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
   
    flares.makelightflares();

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

