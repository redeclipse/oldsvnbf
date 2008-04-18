extern int rtsharefb, rtscissor, blurtile;

struct rendertarget
{
    int texsize;
    GLenum colorfmt, depthfmt;
    GLuint rendertex, renderfb, renderdb, blurtex, blurfb, blurdb;
    int blursize;
    float blursigma;
    float blurweights[MAXBLURRADIUS+1], bluroffsets[MAXBLURRADIUS+1];

    float scissorx1, scissory1, scissorx2, scissory2;
#define BLURTILES 32
#define BLURTILEMASK (0xFFFFFFFFU>>(32-BLURTILES))
    uint blurtiles[BLURTILES+1];

    rendertarget() : texsize(0), colorfmt(GL_FALSE), depthfmt(GL_FALSE), rendertex(0), renderfb(0), renderdb(0), blurtex(0), blurfb(0), blurdb(0), blursize(0), blursigma(0)
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

    void cleanup(bool fullclean = false)
    {
        if(renderfb) { glDeleteFramebuffers_(1, &renderfb); renderfb = 0; }
        if(renderdb) { glDeleteRenderbuffers_(1, &renderdb); renderdb = 0; }
        if(rendertex) { glDeleteTextures(1, &rendertex); rendertex = 0; }
        texsize = 0;
        cleanupblur();

        if(fullclean) colorfmt = depthfmt = GL_FALSE;
    }

    void cleanupblur()
    {
        if(blurfb) { glDeleteFramebuffers_(1, &blurfb); blurfb = 0; }
        if(blurtex) { glDeleteTextures(1, &blurtex); blurtex = 0; }
        if(blurdb) { glDeleteRenderbuffers_(1, &blurdb); blurdb = 0; }
        blursize = 0;
        blursigma = 0.0f;
    }

    void setupblur()
    {
        if(!hasFBO) return;
        if(!blurtex) glGenTextures(1, &blurtex);
        createtexture(blurtex, texsize, texsize, NULL, 3, false, colorfmt);

        if(!swaptexs() || rtsharefb) return;
        if(!blurfb) glGenFramebuffers_(1, &blurfb);
        glBindFramebuffer_(GL_FRAMEBUFFER_EXT, blurfb);
        glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, blurtex, 0);
        if(!blurdb) glGenRenderbuffers_(1, &blurdb);
        glGenRenderbuffers_(1, &blurdb);
        glBindRenderbuffer_(GL_RENDERBUFFER_EXT, blurdb);
        glRenderbufferStorage_(GL_RENDERBUFFER_EXT, depthfmt, texsize, texsize);
        glFramebufferRenderbuffer_(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, blurdb);
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

    bool addblurtiles(float x1, float y1, float x2, float y2, float blurmargin = 0)
    {
        if(x1 >= 1 || y1 >= 1 || x2 <= -1 || y2 <= -1) return false;

        scissorx1 = min(scissorx1, max(x1, -1.0f));
        scissory1 = min(scissory1, max(y1, -1.0f));
        scissorx2 = max(scissorx2, min(x2, 1.0f));
        scissory2 = max(scissory2, min(y2, 1.0f));

        float blurerror = 2.0f*float(2*blursize + blurmargin) / texsize;
        int tx1 = max(0, min(BLURTILES - 1, int((x1-blurerror + 1)/2 * BLURTILES))),
            ty1 = max(0, min(BLURTILES - 1, int((y1-blurerror + 1)/2 * BLURTILES))),
            tx2 = max(0, min(BLURTILES - 1, int((x2+blurerror + 1)/2 * BLURTILES))),
            ty2 = max(0, min(BLURTILES - 1, int((y2+blurerror + 1)/2 * BLURTILES)));

        uint mask = (BLURTILEMASK>>(BLURTILES - (tx2+1))) & (BLURTILEMASK<<tx1);
        for(int y = ty1; y <= ty2; y++) blurtiles[y] |= mask;
        return true;
    }

    bool checkblurtiles(float x1, float y1, float x2, float y2, float blurmargin = 0)
    {
        float blurerror = 2.0f*float(2*blursize + blurmargin) / texsize;
        if(x2+blurerror < scissorx1 || y2+blurerror < scissory1 || 
           x1-blurerror > scissorx2 || y1-blurerror > scissory2) 
            return false;

        if(!blurtile) return true;

        int tx1 = max(0, min(BLURTILES - 1, int((x1 + 1)/2 * BLURTILES))),
            ty1 = max(0, min(BLURTILES - 1, int((y1 + 1)/2 * BLURTILES))),
            tx2 = max(0, min(BLURTILES - 1, int((x2 + 1)/2 * BLURTILES))),
            ty2 = max(0, min(BLURTILES - 1, int((y2 + 1)/2 * BLURTILES)));

        uint mask = (BLURTILEMASK>>(BLURTILES - (tx2+1))) & (BLURTILEMASK<<tx1);
        for(int y = ty1; y <= ty2; y++) if(blurtiles[y] & mask) return true;

        return false;
    }

    virtual void rendertiles()
    {
        glBegin(GL_QUADS);
        if(blurtile && scissorx1 < scissorx2 && scissory1 < scissory2)
        {
            uint tiles[sizeof(blurtiles)/sizeof(uint)];
            memcpy(tiles, blurtiles, sizeof(blurtiles));

            float tsz = 1.0f/BLURTILES;
            loop(y, BLURTILES+1)
            {
                uint mask = tiles[y];
                int x = 0;
                while(mask)
                {
                    while(!(mask&0xFF)) { mask >>= 8; x += 8; }
                    while(!(mask&1)) { mask >>= 1; x++; }
                    int xstart = x;
                    do { mask >>= 1; x++; } while(mask&1);
                    uint strip = (BLURTILEMASK>>(BLURTILES - x)) & (BLURTILEMASK<<xstart);
                    int yend = y;
                    do { tiles[yend] &= ~strip; yend++; } while((tiles[yend] & strip) == strip);
                    float tx = xstart*tsz,
                          ty = y*tsz,
                          tw = (x-xstart)*tsz,
                          th = (yend-y)*tsz,
                          vx = 2*tx - 1, vy = 2*ty - 1, vw = tw*2, vh = th*2;
                    glTexCoord2f(tx,    ty);    glVertex2f(vx,    vy);
                    glTexCoord2f(tx+tw, ty);    glVertex2f(vx+vw, vy);
                    glTexCoord2f(tx+tw, ty+th); glVertex2f(vx+vw, vy+vh);
                    glTexCoord2f(tx,    ty+th); glVertex2f(vx,    vy+vh);
                }
            }
        }
        else
        {
            glTexCoord2f(0, 0); glVertex2f(-1, -1);
            glTexCoord2f(1, 0); glVertex2f(1, -1);
            glTexCoord2f(1, 1); glVertex2f(1, 1);
            glTexCoord2f(0, 1); glVertex2f(-1, 1);
        }
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
                if(!swaptexs() || rtsharefb) glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, i ? rendertex : blurtex, 0);
                else glBindFramebuffer_(GL_FRAMEBUFFER_EXT, i ? renderfb : blurfb);
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

    virtual bool shouldrender() { return true; }

    virtual void doblur(int blursize, float blursigma) 
    { 
        int sx, sy, sw, sh;
        bool scissoring = rtscissor && scissorblur(sx, sy, sw, sh) && sw > 0 && sh > 0;
        if(!scissoring) { sx = sy = 0; sw = sh = texsize; }
        blur(blursize, blursigma, sx, sy, sw, sh, scissoring);
    }

    virtual bool scissorrender(int &x, int &y, int &w, int &h)
    {
        if(scissorx1 >= scissorx2 || scissory1 >= scissory2) return false;
        x = max(int(floor((scissorx1+1)/2*texsize)) - 2*blursize, 0);
        y = max(int(floor((scissory1+1)/2*texsize)) - 2*blursize, 0);
        w = min(int(ceil((scissorx2+1)/2*texsize)) + 2*blursize, texsize) - x;
        h = min(int(ceil((scissory2+1)/2*texsize)) + 2*blursize, texsize) - y;
        return true;
    }

    virtual bool scissorblur(int &x, int &y, int &w, int &h)
    {
        if(scissorx1 >= scissorx2 || scissory1 >= scissory2) return false;
        x = max(int(floor((scissorx1+1)/2*texsize)), 0);
        y = max(int(floor((scissory1+1)/2*texsize)), 0);
        w = min(int(ceil((scissorx2+1)/2*texsize)), texsize) - x;
        h = min(int(ceil((scissory2+1)/2*texsize)), texsize) - y;
        return true;
    }

    virtual void doclear() {}

    void render(int size, int blursize, float blursigma)
    {
        size = min(size, hwtexsize);
        if(!hasFBO) while(size>screen->w || size>screen->h) size /= 2;
        if(size!=texsize || (hasFBO && (swaptexs() && !rtsharefb ? !blurfb : blurfb))) cleanup();
            
        if(!rendertex) setup(size);
    
        scissorx2 = scissory2 = -1;
        scissorx1 = scissory1 = 1;
        memset(blurtiles, 0, sizeof(blurtiles));
 
        if(!shouldrender()) return;

        if(hasFBO)
        {
            if(blursize && !blurtex) setupblur();
            if(swaptexs() && blursize)
            {
                swap(rendertex, blurtex);
                if(!rtsharefb)
                {
                    swap(renderfb, blurfb);
                    swap(renderdb, blurdb);
                }
            }
            glBindFramebuffer_(GL_FRAMEBUFFER_EXT, renderfb);
            if(swaptexs() && blursize && rtsharefb)
                glFramebufferTexture2D_(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, rendertex, 0);
            glViewport(0, 0, texsize, texsize);
        }
        else glViewport(screen->w-texsize, screen->h-texsize, texsize, texsize);

        doclear();

        int sx, sy, sw, sh;
        bool scissoring = rtscissor && scissorrender(sx, sy, sw, sh) && sw > 0 && sh > 0;
        if(scissoring)
        {
            glScissor((hasFBO ? 0 : screen->w-texsize) + sx, (hasFBO ? 0 : screen->h-texsize) + sy, sw, sh);
            glEnable(GL_SCISSOR_TEST);
        }

        bool succeeded = dorender(); 

        if(scissoring) glDisable(GL_SCISSOR_TEST);

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

    void debugscissor(int w, int h, bool lines = false)
    {
        if(!rtscissor || scissorx1 >= scissorx2 || scissory1 >= scissory2) return;
        int sx = int(0.5f*(scissorx1 + 1)*w),
            sy = int(0.5f*(scissory1 + 1)*h),
            sw = int(0.5f*(scissorx2 - scissorx1)*w),
            sh = int(0.5f*(scissory2 - scissory1)*h);
        if(flipdebug()) { sy = h - sy; sh = -sh; }
        glBegin(lines ? GL_LINE_LOOP : GL_QUADS);
        glVertex2i(sx,      sy);
        glVertex2i(sx + sw, sy);
        glVertex2i(sx + sw, sy + sh);
        glVertex2i(sx,      sy + sh);
        glEnd();
    }

    void debugblurtiles(int w, int h, bool lines = false)
    {
        if(!blurtile) return;
        float vxsz = float(w)/BLURTILES, vysz = float(h)/BLURTILES;
        loop(y, BLURTILES+1)
        {
            uint mask = blurtiles[y];
            int x = 0;
            while(mask)
            {
                while(!(mask&0xFF)) { mask >>= 8; x += 8; }
                while(!(mask&1)) { mask >>= 1; x++; }
                    int xstart = x;
                do { mask >>= 1; x++; } while(mask&1);
                uint strip = (BLURTILEMASK>>(BLURTILES - x)) & (BLURTILEMASK<<xstart);
                int yend = y;
                do { blurtiles[yend] &= ~strip; yend++; } while((blurtiles[yend] & strip) == strip);
                float vx = xstart*vxsz,
                      vy = y*vysz,
                      vw = (x-xstart)*vxsz,
                      vh = (yend-y)*vysz;
                if(flipdebug()) { vy = h - vy; vh = -vh; }
                loopi(lines ? 1 : 2)
                {
                    if(!lines) glColor3f(1, 1, i ? 1.0f : 0.5f);
                    glBegin(lines || i ? GL_LINE_LOOP : GL_QUADS);
                    glVertex2f(vx,    vy);
                    glVertex2f(vx+vw, vy);
                    glVertex2f(vx+vw, vy+vh);
                    glVertex2f(vx,    vy+vh);
                    glEnd();
                }
            }
        }
    }

    void debug()
    {
        if(!rendertex) return;
        int w = min(screen->w, screen->h)/2, h = (w*screen->h)/screen->w;
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

