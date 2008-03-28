
struct rendertarget
{
    int texsize;
    GLenum colorfmt, depthfmt;
    GLuint rendertex, renderfb, renderdb, blurtex, blurfb;
    int blursize;
    float blursigma;
    float blurweights[MAXBLURRADIUS+1], bluroffsets[MAXBLURRADIUS+1];
 
    rendertarget() : texsize(0), colorfmt(GL_FALSE), depthfmt(GL_FALSE), rendertex(0), renderfb(0), blurtex(0), blurfb(0), blursize(0), blursigma(0)
    {
    }

    virtual ~rendertarget() {}

    virtual const GLenum *colorformats() const
    {
        static const GLenum colorfmts[] = { GL_RGB, GL_RGB8, GL_FALSE };
        return colorfmts;
    }

    virtual const GLenum *depthformats() const
    {
        static const GLenum depthfmts[] = { GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT32, GL_FALSE };
        return depthfmts;
    }

    void cleanup()
    {
        if(renderfb) { glDeleteFramebuffers_(1, &renderfb); renderfb = 0; }
        if(renderdb) { glDeleteRenderbuffers_(1, &renderdb); renderdb = 0; }
        if(rendertex) { glDeleteTextures(1, &rendertex); rendertex = 0; }
        texsize = 0;
        cleanupblur();
    }

    void cleanupblur()
    {
        if(blurfb) { glDeleteFramebuffers_(1, &blurfb); blurfb = 0; }
        if(blurtex) { glDeleteTextures(1, &blurtex); blurtex = 0; }
        blursize = 0;
        blursigma = 0.0f;
    }

    void setupblur()
    {
        if(!hasFBO) return;
        if(!blurtex) glGenTextures(1, &blurtex);
        createtexture(blurtex, texsize, texsize, NULL, 3, false, colorfmt);

        if(!blurfb) glGenFramebuffers_(1, &blurfb);
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, blurfb);
        glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, blurtex, 0);
        if(swaptexs()) glFramebufferRenderbuffer_(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderdb);
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
    }

    void setup(int size)
    {
        if(hasFBO)
        {
            if(!renderfb) glGenFramebuffers_(1, &renderfb);
            glBindFramebuffer_(GL_FRAMEBUFFER_EXT, renderfb);
        }
        if(!rendertex) glGenTextures(1, &rendertex);

        const GLenum *colorfmts = colorformats();
        int find = 0;
        do
        {
            createtexture(rendertex, size, size, NULL, 3, false, colorfmt ? colorfmt : colorfmts[find]);
            if(!hasFBO) break;
            else
            {
                glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, rendertex, 0);
                if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT) break;
            }
        }
        while(!colorfmt && colorfmts[++find]);
        if(!colorfmt) colorfmt = colorfmts[find];

        if(hasFBO)
        {
            if(!renderdb) { glGenRenderbuffers_(1, &renderdb); depthfmt = GL_FALSE; }
            if(!depthfmt) glBindRenderbuffer_(GL_RENDERBUFFER_EXT, renderdb);
            const GLenum *depthfmts = depthformats();
            find = 0;
            do
            {
                if(!depthfmt) glRenderbufferStorage_(GL_RENDERBUFFER_EXT, depthfmts[find], size, size);
                glFramebufferRenderbuffer_(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderdb);
                if(glCheckFramebufferStatus_(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT) break;
            }
            while(!depthfmt && depthfmts[++find]);
            if(!depthfmt) depthfmt = depthfmts[find];
        
            glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
        }
        texsize = size;
    }
 
    virtual void rendertiles()
    {
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(-1, -1);
        glTexCoord2f(1, 0); glVertex2f(1, -1);
        glTexCoord2f(1, 1); glVertex2f(1, 1);
        glTexCoord2f(0, 1); glVertex2f(-1, 1);
        glEnd();
    }

    void blur(int wantsblursize, float wantsblursigma, int x, int y, int w, int h, bool scissor)
    {
        if(!hasFBO)
        {
            x += screen->w-texsize;
            y += screen->h-texsize;
        }
        else if(!blurtex) setupblur();
        if(blursize!=wantsblursize || (wantsblursize && blursigma!=wantsblursigma))
        {
            setupblurkernel(wantsblursize, wantsblursigma, blurweights, bluroffsets);
            blursize = wantsblursize;
            blursigma = wantsblursigma;
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        if(scissor)
        {
            glScissor(x, y, w, h);
            glEnable(GL_SCISSOR_TEST);
        }

        loopi(2)
        {
            setblurshader(i, texsize, blursize, blurweights, bluroffsets);

            if(hasFBO)
            {
                glBindFramebuffer_(GL_FRAMEBUFFER_EXT, i ? renderfb : blurfb);
                glBindTexture(GL_TEXTURE_2D, i ? blurtex : rendertex);
            }

            rendertiles();

            if(!hasFBO) glCopyTexSubImage2D(GL_TEXTURE_2D, 0, x-(screen->w-texsize), y-(screen->h-texsize), x, y, w, h);
        }

        if(scissor) glDisable(GL_SCISSOR_TEST);
            
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
    }

    virtual bool swaptexs() const { return false; }

    virtual bool dorender() { return true; }

    virtual void doblur(int blursize, float blursigma) 
    { 
        blur(blursize, blursigma, 0, 0, texsize, texsize, false);
    }

    void render(int size, int blursize, float blursigma)
    {
        size = min(size, hwtexsize);
        if(!hasFBO) while(size>screen->w || size>screen->h) size /= 2;
        if(size!=texsize && texsize) cleanup();

        if(!rendertex) setup(size);
        if(blursize && hasFBO && !blurtex) setupblur();
       
        if(hasFBO)
        {
            if(swaptexs() && blursize && blurfb)
            {
                swap(renderfb, blurfb);
                swap(rendertex, blurtex);
            }
            glBindFramebuffer_(GL_FRAMEBUFFER_EXT, renderfb);
            glViewport(0, 0, texsize, texsize);
        }
        else glViewport(screen->w-texsize, screen->h-texsize, texsize, texsize);
        
        bool succeeded = dorender(); 
        if(succeeded)
        {
            if(!hasFBO)
            {
                glBindTexture(GL_TEXTURE_2D, rendertex);
                glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen->w-texsize, screen->h-texsize, texsize, texsize);
            }

            if(blursize) doblur(blursize, blursigma);
        }

        if(hasFBO) glBindFramebuffer_(GL_FRAMEBUFFER_EXT, 0);
        glViewport(0, 0, screen->w, screen->h);
    }

    virtual void dodebug(int w, int h) {}
    virtual bool flipdebug() const { return true; }

    void debug()
    {
        if(!rendertex) return;
        int w = min(screen->w, screen->h)/2, h = w;
        defaultshader->set();
        glColor3f(1, 1, 1);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, rendertex);
        int t1 = 0, t2 = 1;
        if(flipdebug()) swap(t1, t2);
        glBegin(GL_QUADS);
        glTexCoord2f(0, t1); glVertex2i(0, 0);
        glTexCoord2f(1, t1); glVertex2i(w, 0);
        glTexCoord2f(1, t2); glVertex2i(w, h);
        glTexCoord2f(0, t2); glVertex2i(0, h);
        glEnd();
        notextureshader->set();
        glDisable(GL_TEXTURE_2D);
        dodebug(w, h);
    }
};

