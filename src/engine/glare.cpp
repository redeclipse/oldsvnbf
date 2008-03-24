#include "pch.h"
#include "engine.h"

static GLuint glaretex = 0, glarefb = 0, glaredb = 0, blurtex = 0, blurfb = 0;

void cleanupglare()
{
    if(glarefb) { glDeleteFramebuffers_(1, &glarefb); glarefb = 0; }
    if(glaredb) { glDeleteRenderbuffers_(1, &glaredb); glaredb = 0; }
    if(glaretex) { glDeleteTextures(1, &glaretex); glaretex = 0; }
    if(blurfb) { glDeleteFramebuffers_(1, &blurfb); blurfb = 0; }
    if(blurtex) { glDeleteTextures(1, &blurtex); blurtex = 0; }
}

void setupglare(int size)
{
    static const GLenum colorfmts[] = { GL_RGBA, GL_RGBA8, GL_RGB, GL_RGB8, GL_FALSE },
                        depthfmts[] = { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT32, GL_FALSE };
    static GLenum colorfmt = GL_FALSE, depthfmt = GL_FALSE;

    if(hasFBO)
    {
        if(!glarefb) glGenFramebuffers_(1, &glarefb);
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, glarefb);
    }   

    if(!glaretex) glGenTextures(1, &glaretex);

    int find = 0;
    do
    {
        createtexture(glaretex, size, size, NULL, 3, false, colorfmt ? colorfmt : colorfmts[find]);
        if(!hasFBO) break;
        else
        {
            glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, glaretex, 0);
            if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT) break;
        }
    }
    while(!colorfmt && colorfmts[++find]);
    if(!colorfmt) colorfmt = colorfmts[find];

    if(hasFBO)
    {
        if(!glaredb) { glGenRenderbuffers_(1, &glaredb); depthfmt = GL_FALSE; }
        if(!depthfmt) glBindRenderbuffer_(GL_RENDERBUFFER_EXT, glaredb);
        find = 0;
        do
        {
            if(!depthfmt) glRenderbufferStorage_(GL_RENDERBUFFER_EXT, depthfmts[find], size, size);
            glFramebufferRenderbuffer_(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, glaredb);
            if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT) break;
        }
        while(!depthfmt && depthfmts[++find]);
        if(!depthfmt) depthfmt = depthfmts[find];

        glGenTextures(1, &blurtex);
        createtexture(blurtex, size, size, NULL, 3, false, colorfmt);

        glGenFramebuffers_(1, &blurfb);
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, blurfb);
        glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, blurtex, 0);

        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
    }
}

VARFP(glaresize, 6, 8, 10, cleanreflections());
VARP(glare, 0, 0, 1);

VAR(debugglare, 0, 0, 1);

void viewglaretex()
{
    if(!glare || !glaretex) return;
    int w = min(screen->w, screen->h)/2, h = w;
    defaultshader->set();
    glColor3f(1, 1, 1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, glaretex);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2i(0, 0);
    glTexCoord2f(1, 1); glVertex2i(w, 0);
    glTexCoord2f(1, 0); glVertex2i(w, h);
    glTexCoord2f(0, 0); glVertex2i(0, h);
    glEnd();
    notextureshader->set();
    glDisable(GL_TEXTURE_2D);
}

static float blurweights[MAXBLURRADIUS+1] = { 0 }, bluroffsets[MAXBLURRADIUS+1] = { 0 };

extern int blurglare, blurglaresigma;
VARFP(blurglare, 0, 3, 7, setupblurkernel(blurglare, blurglaresigma/100.0f, blurweights, bluroffsets));
VARFP(blurglaresigma, 1, 50, 200, setupblurkernel(blurglare, blurglaresigma/100.0f, blurweights, bluroffsets));

void blurglaretex(int size)
{
    if(blurglare<=0) return;

    if(!blurweights[0]) setupblurkernel(blurglare, blurglaresigma/100.0f, blurweights, bluroffsets);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    loopi(2)
    {
        setblurshader(i, size, blurglare, blurweights, bluroffsets); 

        if(hasFBO)
        {
            glBindFramebuffer_(GL_FRAMEBUFFER_EXT, i ? glarefb : blurfb);
            glBindTexture(GL_TEXTURE_2D, i ? blurtex : glaretex);
        }

        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-1, -1);
        glTexCoord2f(1, 0); glVertex2f(1, -1);
        glTexCoord2f(1, 1); glVertex2f(1, 1);
        glTexCoord2f(0, 1); glVertex2f(-1, 1);
        glEnd();

        if(!hasFBO) glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen->w-size, screen->h-size, size, size);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

bool glaring = false;

void drawglaretex()
{
    if(!glare || renderpath==R_FIXEDFUNCTION) return;

    static int lastsize = 0;
    int size = 1<<glaresize;
    if(!hasFBO) while(size>screen->w || size>screen->h) size /= 2;
    if(size!=lastsize) { if(lastsize) cleanupglare(); lastsize = size; }

    if(!glaretex)
    {
        setupglare(size);
        if(!glaretex) return;
    }

    glViewport(hasFBO ? 0 : screen->w-size, hasFBO ? 0 : screen->h-size, size, size);
    if(hasFBO) glBindFramebuffer_(GL_FRAMEBUFFER_EXT, glarefb);

    extern void drawglare();
    drawglare();

    if(!hasFBO)
    {
        glBindTexture(GL_TEXTURE_2D, glaretex);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen->w-size, screen->h-size, size, size);
    }

    blurglaretex(size);

    glViewport(0, 0, screen->w, screen->h);
    if(hasFBO) glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
}

FVARP(glarescale, 2);

void addglare()
{
    if(!glare || renderpath==R_FIXEDFUNCTION) return;

    glDisable(GL_DEPTH_TEST);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    static Shader *glareshader = NULL;
    if(!glareshader) glareshader = lookupshaderbyname("glare");
    glareshader->set();

    glBindTexture(GL_TEXTURE_2D, glaretex);

    setlocalparamf("glarescale", SHPARAM_PIXEL, 0, glarescale, glarescale, glarescale);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-1, -1, 0);
    glTexCoord2f(1, 0); glVertex3f( 1, -1, 0);
    glTexCoord2f(1, 1); glVertex3f( 1,  1, 0);
    glTexCoord2f(0, 1); glVertex3f(-1,  1, 0);
    glEnd();

    glDisable(GL_BLEND);

    glEnable(GL_DEPTH_TEST);
}
     
