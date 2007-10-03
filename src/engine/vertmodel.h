VARP(lightmodels, 0, 1, 1);
VARP(envmapmodels, 0, 1, 1);
VARP(glowmodels, 0, 1, 1);

struct vertmodel : model
{
    struct anpos
    {
        int fr1, fr2;
        float t;
                
        void setframes(const animstate &as)
        {
            int time = as.anim&ANIM_SETTIME ? as.basetime : lastmillis-as.basetime;
            fr1 = (int)(time/as.speed); // round to full frames
            t = (time-fr1*as.speed)/as.speed; // progress of the frame, value from 0.0f to 1.0f
            if(as.anim&ANIM_LOOP)
            {
                fr1 = fr1%as.range+as.frame;
                fr2 = fr1+1;
                if(fr2>=as.frame+as.range) fr2 = as.frame;
            }
            else
            {
                fr1 = min(fr1, as.range-1)+as.frame;
                fr2 = min(fr1+1, as.frame+as.range-1);
            }
            if(as.anim&ANIM_REVERSE)
            {
                fr1 = (as.frame+as.range-1)-(fr1-as.frame);
                fr2 = (as.frame+as.range-1)-(fr2-as.frame);
            }
        }

        bool operator==(const anpos &a) const { return fr1==a.fr1 && fr2==a.fr2 && (fr1==fr2 || t==a.t); }
        bool operator!=(const anpos &a) const { return fr1!=a.fr1 || fr2!=a.fr2 || (fr1!=fr2 && t!=a.t); }
    };

    struct vert { vec norm, pos; };
    struct vvertff { vec pos; float u, v; };
    struct vvert : vvertff { vec norm; };
    struct tcvert { float u, v; ushort index; };
    struct tri { ushort vert[3]; };

    struct part;

    struct skin
    {
        part *owner;
        Texture *tex, *masks, *envmap;
        int override;
        Shader *shader;
        float spec, ambient, glow, fullbright, envmapmin, envmapmax, translucency, scrollu, scrollv, alphatest;
        bool alphablend;

        skin() : owner(0), tex(notexture), masks(notexture), envmap(NULL), override(0), shader(NULL), spec(1.0f), ambient(0.3f), glow(3.0f), fullbright(0), envmapmin(0), envmapmax(0), translucency(0.5f), scrollu(0), scrollv(0), alphatest(0.9f), alphablend(true) {}

        bool multitextured() { return enableglow; }
        bool envmapped() { return hasCM && envmapmax>0 && envmapmodels && (renderpath!=R_FIXEDFUNCTION || maxtmus>=3); }
        bool normals() { return renderpath!=R_FIXEDFUNCTION || (lightmodels && !fullbright) || envmapped(); }

        void setuptmus(animstate &as, bool masked)
        {
            if(fullbright && enablelighting) { glDisable(GL_LIGHTING); enablelighting = false; }
            else if(lightmodels && !enablelighting) { glEnable(GL_LIGHTING); enablelighting = true; }
            if(masked!=enableglow) lasttex = lastmasks = NULL;
            if(masked)
            {
                if(!enableglow) setuptmu(0, "K , C @ T", as.anim&ANIM_ENVMAP && envmapmax>0 ? "Ca * Ta" : NULL);
                int glowscale = glow>2 ? 4 : (glow > 1 ? 2 : 1);
                float envmap = as.anim&ANIM_ENVMAP && envmapmax>0 ? 0.2f*envmapmax + 0.8f*envmapmin : 1;
                colortmu(0, glow/glowscale, glow/glowscale, glow/glowscale);
                if(fullbright) glColor4f(fullbright/glowscale, fullbright/glowscale, fullbright/glowscale, envmap);
                else if(lightmodels)
                {
                    GLfloat material[4] = { 1.0f/glowscale, 1.0f/glowscale, 1.0f/glowscale, envmap };
                    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material);
                }
                else glColor4f(lightcolor.x/glowscale, lightcolor.y/glowscale, lightcolor.z/glowscale, envmap);

                glActiveTexture_(GL_TEXTURE1_ARB);
                if(!enableglow || (!enableenvmap && as.anim&ANIM_ENVMAP && envmapmax>0) || as.anim&ANIM_TRANSLUCENT)
                {
                    if(!enableglow) glEnable(GL_TEXTURE_2D);
                    if(!(as.anim&ANIM_ENVMAP && envmapmax>0) && as.anim&ANIM_TRANSLUCENT) colortmu(1, 0, 0, 0, translucency);
                    setuptmu(1, "P * T", as.anim&ANIM_ENVMAP && envmapmax>0 ? "= Pa" : (as.anim&ANIM_TRANSLUCENT ? "Ta * Ka" : "= Ta"));
                }
                scaletmu(1, glowscale);

                if(as.anim&ANIM_ENVMAP && envmapmax>0 && as.anim&ANIM_TRANSLUCENT)
                {
                    glActiveTexture_(GL_TEXTURE2_ARB);
                    colortmu(2, 0, 0, 0, translucency);
                }

                glActiveTexture_(GL_TEXTURE0_ARB);

                enableglow = true;
            }
            else
            {
                if(enableglow) disableglow(); 
                if(fullbright) glColor4f(fullbright, fullbright, fullbright, as.anim&ANIM_TRANSLUCENT ? translucency : 1);
                else if(lightmodels) 
                {
                    GLfloat material[4] = { 1, 1, 1, as.anim&ANIM_TRANSLUCENT ? translucency : 1 };
                    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material);
                }
                else glColor4f(lightcolor.x, lightcolor.y, lightcolor.z, as.anim&ANIM_TRANSLUCENT ? translucency : 1);
            }
            if(lightmodels && !fullbright)
            {
                float ambientk = min(ambient*0.75f, 1), 
                      diffusek = 1-ambientk;
                GLfloat ambientcol[4] = { lightcolor.x*ambientk, lightcolor.y*ambientk, lightcolor.z*ambientk, 1 },
                        diffusecol[4] = { lightcolor.x*diffusek, lightcolor.y*diffusek, lightcolor.z*diffusek, 1 };
                float ambientmax = max(ambientcol[0], max(ambientcol[1], ambientcol[2])),
                      diffusemax = max(diffusecol[0], max(diffusecol[1], diffusecol[2]));
                if(ambientmax>1e-3f) loopk(3) ambientcol[k] *= min(1.5f, 1.0f/ambientmax);
                if(diffusemax>1e-3f) loopk(3) diffusecol[k] *= min(1.5f, 1.0f/diffusemax);
                glLightfv(GL_LIGHT0, GL_AMBIENT, ambientcol);
                glLightfv(GL_LIGHT0, GL_DIFFUSE, diffusecol);
            }
        }

        void setshader(animstate &as, bool masked)
        {
            if(renderpath==R_FIXEDFUNCTION) setuptmus(as, masked);
            else
            {
                static Shader *modelshader = NULL, *modelshadernospec = NULL, *modelshadermasks = NULL, *modelshadermasksnospec = NULL, *modelshaderenvmap = NULL, *modelshaderenvmapnospec = NULL;

                if(!modelshader)             modelshader             = lookupshaderbyname("stdmodel");
                if(!modelshadernospec)       modelshadernospec       = lookupshaderbyname("nospecmodel");
                if(!modelshadermasks)        modelshadermasks        = lookupshaderbyname("masksmodel");
                if(!modelshadermasksnospec)  modelshadermasksnospec  = lookupshaderbyname("masksnospecmodel");
                if(!modelshaderenvmap)       modelshaderenvmap       = lookupshaderbyname("envmapmodel");
                if(!modelshaderenvmapnospec) modelshaderenvmapnospec = lookupshaderbyname("envmapnospecmodel");

                if(fullbright) 
                {
                    glColor4f(fullbright/2, fullbright/2, fullbright/2, as.anim&ANIM_TRANSLUCENT ? translucency : 1);
                    setenvparamf("ambient", SHPARAM_VERTEX, 3, 2, 2, 2, 1);
                    setenvparamf("ambient", SHPARAM_PIXEL, 3, 2, 2, 2, 1);
                }
                else 
                {
                    glColor4f(lightcolor.x, lightcolor.y, lightcolor.z, as.anim&ANIM_TRANSLUCENT ? translucency : 1);
                    setenvparamf("specscale", SHPARAM_PIXEL, 2, spec, spec, spec);
                    setenvparamf("ambient", SHPARAM_VERTEX, 3, ambient, ambient, ambient, 1);
                    setenvparamf("ambient", SHPARAM_PIXEL, 3, ambient, ambient, ambient, 1);
                }
                setenvparamf("glowscale", SHPARAM_PIXEL, 4, glow, glow, glow);
                setenvparamf("millis", SHPARAM_VERTEX, 5, lastmillis/1000.0f, lastmillis/1000.0f, lastmillis/1000.0f);

                if(shader) shader->set();
                else if(as.anim&ANIM_ENVMAP && envmapmax>0)
                {
                    if(lightmodels && !fullbright && (masked || spec>=0.01f)) modelshaderenvmap->set();
                    else modelshaderenvmapnospec->set();
                    setlocalparamf("envmapscale", SHPARAM_VERTEX, 5, envmapmin-envmapmax, envmapmax);
                }
                else if(masked && lightmodels && !fullbright) modelshadermasks->set();
                else if(masked && glowmodels) modelshadermasksnospec->set();
                else if(spec>=0.01f && lightmodels && !fullbright) modelshader->set();
                else modelshadernospec->set();
            }
        }

        void bind(animstate &as)
        {
            if(as.anim&ANIM_NOSKIN)
            {
                if(enablealphatest) { glDisable(GL_ALPHA_TEST); enablealphatest = false; }
                if(!(as.anim&ANIM_SHADOW) && enablealphablend) { glDisable(GL_BLEND); enablealphablend = false; }
                if(enableglow) disableglow();
                if(enableenvmap) disableenvmap();
                if(enablelighting) { glDisable(GL_LIGHTING); enablelighting = false; }
                return;
            }
            Texture *s = tex, *m = masks;
            if(override)
            {
                Slot &slot = lookuptexture(override);
                s = slot.sts[0].t;
                if(slot.sts.length() >= 2) m = slot.sts[1].t;
            }
            if((renderpath==R_FIXEDFUNCTION || !lightmodels) && !glowmodels && (!envmapmodels || !(as.anim&ANIM_ENVMAP) || envmapmax<=0)) m = notexture;
            setshader(as, m!=notexture);
            if(s!=lasttex)
            {
                if(enableglow) glActiveTexture_(GL_TEXTURE1_ARB);
                glBindTexture(GL_TEXTURE_2D, s->gl);
                if(enableglow) glActiveTexture_(GL_TEXTURE0_ARB);
                lasttex = s;
            }
            if(s->bpp==32)
            {
                if(alphablend)
                {
                    if(!enablealphablend && !reflecting && !refracting)
                    {
                        glEnable(GL_BLEND);
                        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                        enablealphablend = true;
                    }
                }
                else if(enablealphablend) { glDisable(GL_BLEND); enablealphablend = false; }
                if(alphatest>0)
                {
                    if(!enablealphatest) { glEnable(GL_ALPHA_TEST); enablealphatest = true; }
                    if(lastalphatest!=alphatest)
                    {
                        glAlphaFunc(GL_GREATER, alphatest);
                        lastalphatest = alphatest;
                    }
                }
                else if(enablealphatest) { glDisable(GL_ALPHA_TEST); enablealphatest = false; }
            }
            else
            {
                if(enablealphatest) { glDisable(GL_ALPHA_TEST); enablealphatest = false; }
                if(enablealphablend && !(as.anim&ANIM_TRANSLUCENT)) { glDisable(GL_BLEND); enablealphablend = false; }
            }
            if(m!=lastmasks && m!=notexture)
            {
                if(!enableglow) glActiveTexture_(GL_TEXTURE1_ARB);
                glBindTexture(GL_TEXTURE_2D, m->gl);
                if(!enableglow) glActiveTexture_(GL_TEXTURE0_ARB);
                lastmasks = m;
            }
            if(as.anim&ANIM_ENVMAP && envmapmax>0)
            {
                GLuint emtex = envmap ? envmap->gl : closestenvmaptex;
                if(!enableenvmap || lastenvmaptex!=emtex)
                {
                    glActiveTexture_(GL_TEXTURE2_ARB);
                    if(!enableenvmap)
                    {
                        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
                        if(renderpath==R_FIXEDFUNCTION)
                        {
                            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
                            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
                            glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_ARB);
                            glEnable(GL_TEXTURE_GEN_S);
                            glEnable(GL_TEXTURE_GEN_T);
                            glEnable(GL_TEXTURE_GEN_R);
                        }
                        enableenvmap = true;
                    }
                    if(lastenvmaptex!=emtex) { glBindTexture(GL_TEXTURE_CUBE_MAP_ARB, emtex); lastenvmaptex = emtex; }
                    glActiveTexture_(GL_TEXTURE0_ARB);
                }
            }
            else if(enableenvmap) disableenvmap();
        }
    };

    struct meshgroup;

    struct vbocacheentry
    {
        GLuint vbuf;
        anpos cur, prev;
        float t;
        int millis;

        vbocacheentry() : vbuf(0) { cur.fr1 = prev.fr1 = -1; }
    };

    struct mesh
    {
        meshgroup *group;        
        char *name;
        vert *verts;
        tcvert *tcverts;
        tri *tris;
        int numverts, numtcverts, numtris;

        vert *stripbuf;
        ushort *stripidx;
        int striplen;
        bool stripnorms;

        anpos cachecur, cacheprev;
        float cachet;

        enum { LIST_NOSKIN = 0, LIST_TEX, LIST_TEXNORMS, LIST_MULTITEX, LIST_MULTITEXNORMS, NUMLISTS }; 
        GLuint lists[NUMLISTS];

        int voffset, eoffset, elen;

        mesh() : group(0), name(0), verts(0), tcverts(0), tris(0), stripbuf(0), stripidx(0)
        {
            cachecur.fr1 = cacheprev.fr1 = -1;
            loopi(NUMLISTS) lists[i] = 0;
        }

        ~mesh()
        {
            DELETEA(name);
            DELETEA(verts);
            DELETEA(tcverts);
            DELETEA(tris);
            loopi(NUMLISTS) if(lists[i]) glDeleteLists(lists[i], 1);
            DELETEA(stripidx);
            DELETEA(stripbuf);
        }

        mesh *copy()
        {
            mesh &m = *new mesh;
            m.name = newstring(name);
            m.numverts = numverts;
            m.verts = new vert[numverts*group->numframes];
            memcpy(m.verts, verts, numverts*group->numframes*sizeof(vert));
            m.numtcverts = numtcverts;
            m.tcverts = new tcvert[numtcverts];
            memcpy(m.tcverts, tcverts, numtcverts*sizeof(tcvert));
            m.numtris = numtris;
            m.tris = new tri[numtris];
            memcpy(m.tris, tris, numtris*sizeof(tri));
            return &m;
        }

        void calcbb(int frame, vec &bbmin, vec &bbmax, float m[12])
        {
            vert *fverts = &verts[frame*numverts];
            loopj(numverts)
            {
                vec &v = fverts[j].pos;
                loopi(3)
                {
                    float c = m[i]*v.x + m[i+3]*v.y + m[i+6]*v.z + m[i+9];
                    bbmin[i] = min(bbmin[i], c);
                    bbmax[i] = max(bbmax[i], c);
                }
            }
        }

        void gentris(int frame, Texture *tex, vector<BIH::tri> &out, float m[12])
        {
            vert *fverts = &verts[frame*numverts];
            loopj(numtris)
            {
                BIH::tri &t = out.add();
                t.tex = tex->bpp==32 ? tex : NULL;
                tcvert &av = tcverts[tris[j].vert[0]],
                       &bv = tcverts[tris[j].vert[1]],
                       &cv = tcverts[tris[j].vert[2]];
                vec &a = fverts[av.index].pos,
                    &b = fverts[bv.index].pos,
                    &c = fverts[cv.index].pos;
                loopi(3)
                {
                    t.a[i] = m[i]*a.x + m[i+3]*a.y + m[i+6]*a.z + m[i+9];
                    t.b[i] = m[i]*b.x + m[i+3]*b.y + m[i+6]*b.z + m[i+9];
                    t.c[i] = m[i]*c.x + m[i+3]*c.y + m[i+6]*c.z + m[i+9];
                }
                t.tc[0] = av.u;
                t.tc[1] = av.v;
                t.tc[2] = bv.u;
                t.tc[3] = bv.v;
                t.tc[4] = cv.u;
                t.tc[5] = cv.v;
            }
        }

        void genstripbuf()
        {
            tristrip ts;
            ts.addtriangles((ushort *)tris, numtris);
            vector<ushort> idxs;
            ts.buildstrips(idxs);
            stripbuf = new vert[numverts];
            stripidx = new ushort[idxs.length()];
            memcpy(stripidx, idxs.getbuf(), idxs.length()*sizeof(ushort));
            striplen = idxs.length();
        }

        int genvbo(vector<ushort> &idxs, int offset, vector<vvert> &vverts, bool norms)
        {
            voffset = offset;
            eoffset = idxs.length();
            loopi(numtris)
            {
                tri &t = tris[i];
                if(group->numframes>1) loopj(3) idxs.add(voffset+t.vert[j]);
                else loopj(3) 
                {
                    tcvert &tc = tcverts[t.vert[j]];
                    vert &v = verts[tc.index];
                    loopvk(vverts) // check if it's already added
                    {
                        vvert &w = vverts[k];
                        if(tc.u==w.u && tc.v==w.v && v.pos==w.pos && (!norms || v.norm==w.norm)) { idxs.add((ushort)k); goto found; }
                    }
                    {
                        idxs.add(vverts.length());
                        vvert &w = vverts.add();
                        w.pos = v.pos;
                        w.norm = v.norm;
                        w.u = tc.u;
                        w.v = tc.v;
                    }
                    found:;
                }
            }
            elen = idxs.length()-eoffset;
            return group->numframes>1 ? numtcverts : vverts.length()-voffset;
        }

        void interpverts(anpos &cur, anpos *prev, float ai_t, bool norms, void *vbo, skin &s)
        {
            vert *vert1 = &verts[cur.fr1 * numverts],
                 *vert2 = &verts[cur.fr2 * numverts],
                 *pvert1 = prev ? &verts[prev->fr1 * numverts] : NULL, *pvert2 = prev ? &verts[prev->fr2 * numverts] : NULL;
            #define ip(p1, p2, t)   (p1+t*(p2-p1))
            #define ip_v(p, c, t)   ip(p##1[i].c, p##2[i].c, t)
            #define ip_v_ai(c)      ip(ip_v(pvert, c, prev->t), ip_v(vert, c, cur.t), ai_t)
            #define ip_pos          vec(ip_v(vert, pos.x, cur.t), ip_v(vert, pos.y, cur.t), ip_v(vert, pos.z, cur.t))
            #define ip_pos_ai       vec(ip_v_ai(pos.x), ip_v_ai(pos.y), ip_v_ai(pos.z))
            #define ip_norm         vec(ip_v(vert, norm.x, cur.t), ip_v(vert, norm.y, cur.t), ip_v(vert, norm.z, cur.t))
            #define ip_norm_ai      vec(ip_v_ai(norm.x), ip_v_ai(norm.y), ip_v_ai(norm.z))
            if(!vbo)
            {
                if(prev)
                {
                    if(norms==stripnorms && cacheprev==*prev && cachecur==cur && cachet==ai_t) return;
                    cacheprev = *prev;
                    cachet = ai_t;
                }
                else
                {
                    if(norms==stripnorms && cacheprev.fr1<0 && cachecur==cur) return;
                    cacheprev.fr1 = -1;
                }
                cachecur = cur;
                stripnorms = norms;
                if(!stripbuf) genstripbuf();
                #define iploopnovbo(body) \
                    loopi(numverts) \
                    { \
                        vert &v = stripbuf[i]; \
                        body; \
                    }
                if(norms)
                {
                    if(prev) iploopnovbo({ v.pos = ip_pos_ai; v.norm = ip_norm_ai; })
                    else iploopnovbo({ v.pos = ip_pos; v.norm = ip_norm; })
                }
                else if(prev) iploopnovbo(v.pos = ip_pos_ai)
                else iploopnovbo(v.pos = ip_pos)
                #undef iploopnovbo
            }
            else 
            {
                #define iploopvbo(type, body) \
                    loopj(numtcverts) \
                    { \
                        tcvert &tc = tcverts[j]; \
                        int i = tc.index; \
                        type &v = ((type *)vbo)[j]; \
                        v.u = tc.u; \
                        v.v = tc.v; \
                        body; \
                    }
                if(s.scrollu || s.scrollv)
                {
                    float du = s.scrollu*lastmillis/1000.0f, dv = s.scrollv*lastmillis/1000.0f;    
                    if(norms)
                    {   
                        if(prev) iploopvbo(vvert, { v.u += du; v.v += dv; v.pos = ip_pos_ai; v.norm = ip_norm_ai; })
                        else iploopvbo(vvert, { v.u += du; v.v += dv; v.pos = ip_pos; v.norm = ip_norm; })
                    }   
                    else if(prev) iploopvbo(vvertff, { v.u += du; v.v += dv; v.pos = ip_pos_ai; })
                    else iploopvbo(vvertff, { v.u += du; v.v += dv; v.pos = ip_pos; })
                }
                else if(norms)
                {
                    if(prev) iploopvbo(vvert, { v.pos = ip_pos_ai; v.norm = ip_norm_ai; })
                    else iploopvbo(vvert, { v.pos = ip_pos; v.norm = ip_norm; })
                }
                else if(prev) iploopvbo(vvertff, v.pos = ip_pos_ai)
                else iploopvbo(vvertff, v.pos = ip_pos)
                #undef iploopvbo
            }
            #undef ip
            #undef ip_v
            #undef ip_v_ai
        }

        void render(animstate &as, anpos &cur, anpos *prev, float ai_t, skin &s, vbocacheentry *vc)
        {
            s.bind(as);

            if(vc && vc->vbuf)
            {
                if(s.multitextured())
                {
                    if(!enablemtc)
                    {
                        size_t vertsize = group->vnorms ? sizeof(vvert) : sizeof(vvertff);
                        vvert *vverts = 0;
                        glClientActiveTexture_(GL_TEXTURE1_ARB);
                        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                        glTexCoordPointer(2, GL_FLOAT, vertsize, &vverts->u);
                        glClientActiveTexture_(GL_TEXTURE0_ARB);
                        enablemtc = true;
                    }
                }
                else if(enablemtc) disablemtc();

                if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, group->numframes>1 ? voffset : 0, group->numframes>1 ? voffset+numtcverts-1 : group->vlen-1, elen, GL_UNSIGNED_SHORT, (void *)(eoffset*sizeof(ushort)));
                else glDrawElements(GL_TRIANGLES, elen, GL_UNSIGNED_SHORT, (void *)(eoffset*sizeof(ushort)));
                glde++;

                xtravertsva += numtcverts;
                return;
            }
            GLuint *list = NULL;
            bool isstat = as.frame==0 && as.range==1 && !s.scrollu && !s.scrollv, multitex = s.multitextured(), norms = s.normals();
            if(isstat)
            {
                list = &lists[as.anim&ANIM_NOSKIN ? LIST_NOSKIN : (multitex ? (norms ? LIST_MULTITEXNORMS : LIST_MULTITEX) : (norms ? LIST_TEXNORMS : LIST_TEX))];
                if(*list)
                {
                    glCallList(*list);
                    xtraverts += striplen;
                    return;
                }
                *list = glGenLists(1);
                glNewList(*list, GL_COMPILE);
            } 
            interpverts(cur, prev, ai_t, norms, NULL, s);
            if(stripidx[0]<tristrip::RESTART) glBegin(GL_TRIANGLE_STRIP); 
            #define renderstripverts(b) \
                loopj(striplen) \
                { \
                    ushort index = stripidx[j]; \
                    if(index>=tristrip::RESTART) \
                    { \
                        if(j) glEnd(); \
                        glBegin(index==tristrip::LIST ? GL_TRIANGLES : GL_TRIANGLE_STRIP); \
                        continue; \
                    } \
                    tcvert &tc = tcverts[index]; \
                    vert &v = stripbuf[tc.index]; \
                    b; \
                    glVertex3fv(v.pos.v); \
                }
            if(as.anim&ANIM_NOSKIN) renderstripverts({})
            else if(s.scrollu || s.scrollv)
            {
                float du = s.scrollu*lastmillis/1000.0f, dv = s.scrollv*lastmillis/1000.0f;
                if(norms && multitex) renderstripverts({ glTexCoord2f(tc.u+du, tc.v+dv); glNormal3fv(v.norm.v); glMultiTexCoord2f_(GL_TEXTURE1_ARB, tc.u+du, tc.v+dv); })
                else if(norms) renderstripverts({ glTexCoord2f(tc.u+du, tc.v+dv); glNormal3fv(v.norm.v); })
                else if(multitex) renderstripverts({ glTexCoord2f(tc.u+du, tc.v+dv); glMultiTexCoord2f_(GL_TEXTURE1_ARB, tc.u+du, tc.v+dv); })
            }
            else if(norms && multitex) renderstripverts({ glTexCoord2f(tc.u, tc.v); glNormal3fv(v.norm.v); glMultiTexCoord2f_(GL_TEXTURE1_ARB, tc.u, tc.v); })
            else if(norms) renderstripverts({ glTexCoord2f(tc.u, tc.v); glNormal3fv(v.norm.v); })
            else if(multitex) renderstripverts({ glTexCoord2f(tc.u, tc.v); glMultiTexCoord2f_(GL_TEXTURE1_ARB, tc.u, tc.v); }) 
            glEnd();
            if(isstat)
            {
                glEndList();
                glCallList(*list);
            }
            xtraverts += striplen;
        }
    };

    struct tag
    {
        char *name;
        vec pos;
        float transform[3][3];

        tag() : name(NULL) {}
        ~tag() { DELETEA(name); }
    };

    struct meshgroup
    {
        meshgroup *next;
        int shared;
        char *name;
        vector<mesh *> meshes;
        tag *tags;
        int numtags, numframes;
        float scale;
        vec translate;

        static const int MAXVBOCACHE = 8;
        vbocacheentry vbocache[MAXVBOCACHE];

        GLuint ebuf;
        bool vnorms;
        int vlen;
        vvert *vdata;

        meshgroup() : next(NULL), shared(0), name(NULL), tags(NULL), numtags(0), numframes(0), scale(1), translate(0, 0, 0), ebuf(0), vdata(NULL) 
        {
        }

        virtual ~meshgroup()
        {
            DELETEA(name);
            meshes.deletecontentsp();
            DELETEA(tags);
            loopi(MAXVBOCACHE) if(vbocache[i].vbuf) glDeleteBuffers_(1, &vbocache[i].vbuf);
            if(ebuf) glDeleteBuffers_(1, &ebuf);
            DELETEA(vdata);
            DELETEP(next);
        }

        int findtag(const char *name)
        {
            loopi(numtags) if(!strcmp(tags[i].name, name)) return i;
            return -1;
        }

        vec anyvert(int frame)
        {
            loopv(meshes) if(meshes[i]->numverts) return meshes[i]->verts[frame*meshes[i]->numverts].pos;
            return vec(0, 0, 0);
        }

        void calcbb(int frame, vec &bbmin, vec &bbmax, float m[12])
        {
            loopv(meshes) meshes[i]->calcbb(frame, bbmin, bbmax, m);
        }

        void gentris(int frame, vector<skin> &skins, vector<BIH::tri> &tris, float m[12])
        {
            loopv(meshes) meshes[i]->gentris(frame, skins[i].tex, tris, m);
        }

        bool hasframe(int i) { return i>=0 && i<numframes; }
        bool hasframes(int i, int n) { return i>=0 && i+n<=numframes; }
        int clipframes(int i, int n) { return min(n, numframes - i); }

        meshgroup *copy()
        {
            meshgroup &group = *new meshgroup;
            group.name = newstring(name);
            loopv(meshes) group.meshes.add(meshes[i]->copy())->group = &group;
            group.numtags = numtags;
            group.tags = new tag[numframes*numtags];
            memcpy(group.tags, tags, numframes*numtags*sizeof(tag));
            loopi(numframes*numtags) if(group.tags[i].name) group.tags[i].name = newstring(group.tags[i].name);
            group.numframes = numframes;
            group.scale = scale;
            group.translate = translate;
            return &group;
        }
           
        meshgroup *scaleverts(const float nscale, const vec &ntranslate)
        {
            if(nscale==scale && ntranslate==translate) { shared++; return this; }
            else if(next || shared)
            {
                if(!next) next = copy();
                return next->scaleverts(nscale, ntranslate);
            }
            float scalediff = nscale/scale;
            vec transdiff(ntranslate);
            transdiff.sub(translate);
            transdiff.mul(scale);
            loopv(meshes)
            {
                mesh &m = *meshes[i];
                loopj(numframes*m.numverts) m.verts[j].pos.add(transdiff).mul(scalediff);
            }
            loopi(numframes*numtags) tags[i].pos.add(transdiff).mul(scalediff);
            scale = nscale;
            translate = ntranslate;
            shared++;
            return this;
        }

        void concattagtransform(int frame, int i, float m[12], float n[12])
        {
            tag &t = tags[frame*numtags + i];
            loop(y, 3)
            {
                n[y] = m[y]*t.transform[0][0] + m[y+3]*t.transform[0][1] + m[y+6]*t.transform[0][2];
                n[3+y] = m[y]*t.transform[1][0] + m[y+3]*t.transform[1][1] + m[y+6]*t.transform[1][2];
                n[6+y] = m[y]*t.transform[2][0] + m[y+3]*t.transform[2][1] + m[y+6]*t.transform[2][2];
                n[9+y] = m[y]*t.pos[0] + m[y+3]*t.pos[1] + m[y+6]*t.pos[2] + m[y+9];
            }
        }

        void calctagmatrix(int i, const anpos &cur, const anpos *prev, float ai_t, GLfloat *matrix)
        {
            tag *tag1 = &tags[cur.fr1*numtags + i];
            tag *tag2 = &tags[cur.fr2*numtags + i];
            #define ip(p1, p2, t) (p1+t*(p2-p1))
            #define ip_ai_tag(c) ip( ip( tag1p->c, tag2p->c, prev->t), ip( tag1->c, tag2->c, cur.t), ai_t)
            if(prev)
            {
                tag *tag1p = &tags[prev->fr1*numtags + i];
                tag *tag2p = &tags[prev->fr2*numtags + i];
                loopj(3) matrix[j] = ip_ai_tag(transform[0][j]); // transform
                loopj(3) matrix[4 + j] = ip_ai_tag(transform[1][j]);
                loopj(3) matrix[8 + j] = ip_ai_tag(transform[2][j]);
                loopj(3) matrix[12 + j] = ip_ai_tag(pos[j]); // position      
            }
            else
            {
                loopj(3) matrix[j] = ip(tag1->transform[0][j], tag2->transform[0][j], cur.t); // transform
                loopj(3) matrix[4 + j] = ip(tag1->transform[1][j], tag2->transform[1][j], cur.t);
                loopj(3) matrix[8 + j] = ip(tag1->transform[2][j], tag2->transform[2][j], cur.t);
                loopj(3) matrix[12 + j] = ip(tag1->pos[j], tag2->pos[j], cur.t); // position
            } 
            #undef ip_ai_tag
            #undef ip 
            matrix[3] = matrix[7] = matrix[11] = 0.0f;
            matrix[15] = 1.0f;
        }

        void genvbo(bool norms, vbocacheentry &vc)
        {
            glGenBuffers_(1, &vc.vbuf);

            if(ebuf) return;

            vector<ushort> idxs;
            vector<vvert> vverts;

            vnorms = norms;
            vlen = 0;
            loopv(meshes) vlen += meshes[i]->genvbo(idxs, vlen, vverts, norms);

            glGenBuffers_(1, &vc.vbuf);
            if(numframes>1) { DELETEA(vdata); vdata = new vvert[vlen]; }
            else
            {
                glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc.vbuf);
                if(!norms)
                {
                    vvertff *ff = new vvertff[vverts.length()];
                    loopv(vverts) { vvert &v = vverts[i]; ff[i].pos = v.pos; ff[i].u = v.u; ff[i].v = v.v; }
                    glBufferData_(GL_ARRAY_BUFFER_ARB, vverts.length()*sizeof(vvertff), ff, GL_STATIC_DRAW_ARB);
                    delete[] ff;
                }
                else glBufferData_(GL_ARRAY_BUFFER_ARB, vverts.length()*sizeof(vvert), vverts.getbuf(), GL_STATIC_DRAW_ARB);
            }

            glGenBuffers_(1, &ebuf);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, ebuf);
            glBufferData_(GL_ELEMENT_ARRAY_BUFFER_ARB, idxs.length()*sizeof(ushort), idxs.getbuf(), GL_STATIC_DRAW_ARB);
        }

        void bindvbo(animstate &as, vbocacheentry &vc)
        {
            size_t vertsize = vnorms ? sizeof(vvert) : sizeof(vvertff);
            vvert *vverts = 0;
            if(lastvbo!=vc.vbuf)
            {
                glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc.vbuf);
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, ebuf);
                glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(3, GL_FLOAT, vertsize, &vverts->pos);
                enabletc = false;
            }
            if(as.anim&ANIM_NOSKIN)
            {
                if(enabletc) disabletc();
            }
            else
            {
                if(!enabletc)
                {
                    if(vertsize==sizeof(vvert))
                    {
                        glEnableClientState(GL_NORMAL_ARRAY);
                        glNormalPointer(GL_FLOAT, vertsize, &vverts->norm);
                    }
                    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    glTexCoordPointer(2, GL_FLOAT, vertsize, &vverts->u);
                    enabletc = true;
                    enablemtc = false;
                }
            }
            lastvbo = vc.vbuf;
        }

        void render(animstate &as, anpos &cur, anpos *prev, float ai_t, vector<skin> &skins)
        {
            vbocacheentry *vc = NULL;
            if(hasVBO)
            {
                if(numframes<=1) vc = vbocache;
                else
                {
                    loopi(MAXVBOCACHE)
                    {
                        vbocacheentry &c = vbocache[i];
                        if(!c.vbuf) continue;
                        if(c.cur==cur && (prev ? c.prev==*prev && c.t==ai_t : c.prev.fr1<0)) { vc = &c; break; }
                    }
                    if(!vc) loopi(MAXVBOCACHE) { vc = &vbocache[i]; if(!vc->vbuf || vc->millis < lastmillis) break; }
                }
                bool norms = false;
                loopv(skins) if(skins[i].normals()) { norms = true; break; }
                if(ebuf && norms!=vnorms)
                {
                    loopi(MAXVBOCACHE)
                    {
                        vbocacheentry &c = vbocache[i];
                        if(c.vbuf) { glDeleteBuffers_(1, &c.vbuf); c.vbuf = 0; c.cur.fr1 = -1; }
                    }
                    glDeleteBuffers_(1, &ebuf);
                    ebuf = 0;
                }
                if(!vc->vbuf) genvbo(norms, *vc);
                if(numframes>1)
                {
                    if(vc->cur!=cur || (prev ? vc->prev!=*prev || vc->t!=ai_t : vc->prev.fr1>=0))
                    {
                        vc->cur = cur;
                        if(prev) { vc->prev = *prev; vc->t = ai_t; }
                        else vc->prev.fr1 = -1;
                        vc->millis = lastmillis;
                        loopv(meshes)
                        {
                            mesh &m = *meshes[i];
                            m.interpverts(cur, prev, ai_t, norms, norms ? &vdata[m.voffset] : &((vvertff *)vdata)[m.voffset], skins[i]);
                        }
                        glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc->vbuf);
                        glBufferData_(GL_ARRAY_BUFFER_ARB, vlen * (norms ? sizeof(vvert) : sizeof(vvertff)), vdata, GL_STREAM_DRAW_ARB);
                    }
                    else if(vc->millis!=lastmillis) loopv(meshes)
                    {
                        skin &s = skins[i];
                        if(!s.scrollu && !s.scrollv) continue;
                        if(vc->millis!=lastmillis) { glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc->vbuf); vc->millis = lastmillis; }
                        mesh &m = *meshes[i];
                        void *vbo = norms ? &vdata[m.voffset] : &((vvertff *)vdata)[m.voffset];
                        m.interpverts(cur, prev, ai_t, norms, vbo, s);
                        int sublen = (i+1<meshes.length() ? meshes[i+1]->voffset : vlen) - m.voffset; 
                        glBufferSubData_(GL_ARRAY_BUFFER_ARB, (uchar *)vbo - (uchar *)vdata, sublen * (norms ? sizeof(vvert) : sizeof(vvertff)), vbo);
                    }
                }

                if(vc->vbuf) bindvbo(as, *vc);
                else if(lastvbo) disablevbo();
            }

            loopv(meshes) meshes[i]->render(as, cur, prev, ai_t, skins[i], vc);
        }
    };

    struct animinfo
    {
        int frame, range;
        float speed;
        int priority;
    };

    struct linkedpart
    {
        part *p;
        int anim, basetime;

        linkedpart() : p(NULL), anim(-1), basetime(0) {}
    };

    struct part
    {
        vertmodel *model;
        int index;
        meshgroup *meshes;
        vector<linkedpart> links;
        vector<skin> skins;
        vector<animinfo> *anims;
        float pitchscale, pitchoffset, pitchmin, pitchmax;

        part() : meshes(NULL), anims(NULL), pitchscale(1), pitchoffset(0), pitchmin(0), pitchmax(0) {}
        virtual ~part()
        {
            DELETEA(anims);
        }

        void calcbb(int frame, vec &bbmin, vec &bbmax)
        {
            float m[12] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
            calcbb(frame, bbmin, bbmax, m);
        }

        void calcbb(int frame, vec &bbmin, vec &bbmax, float m[12])
        {
            meshes->calcbb(frame, bbmin, bbmax, m);
            loopv(links) if(links[i].p)
            {
                float n[12];
                meshes->concattagtransform(frame, i, m, n);
                links[i].p->calcbb(frame, bbmin, bbmax, n);
            }
        }

        void gentris(int frame, vector<BIH::tri> &tris)
        {
            float m[12] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
            gentris(frame, tris, m);
        }

        void gentris(int frame, vector<BIH::tri> &tris, float m[12])
        {
            meshes->gentris(frame, skins, tris, m);
            loopv(links) if(links[i].p)
            {
                float n[12];
                meshes->concattagtransform(frame, i, m, n);
                links[i].p->gentris(frame, tris, n);
            }
        }

        bool link(part *link, const char *tag, int anim = -1, int basetime = 0)
        {
            int i = meshes->findtag(tag);
            if(i<0) return false;
            while(i>=links.length()) links.add();
            links[i].p = link;
            links[i].anim = anim;
            links[i].basetime = basetime;
            return true;
        }

        void initskins(Texture *tex = notexture, Texture *masks = notexture)
        {
            if(!meshes) return;
            while(skins.length() < meshes->meshes.length())
            {
                skin &s = skins.add();
                s.owner = this;
                s.tex = tex;
                s.masks = masks;
            }
        }

        virtual void getdefaultanim(animstate &as, int anim, int varseed, dynent *d)
        {
            as.frame = 0;
            as.range = 1;
        }

        void getanimspeed(animstate &as, dynent *d)
        {
            switch(as.anim&ANIM_INDEX)
            {
                case ANIM_FORWARD:
                case ANIM_BACKWARD:
                case ANIM_LEFT:
                case ANIM_RIGHT:
                case ANIM_SWIM:
#ifdef BFRONTIER
					as.speed = 5500.0f/ph->speed(d);
#else
					as.speed = 5500.0f/d->maxspeed;
#endif
					break;

				default:
					as.speed = 100.0f;
					break;
			}
		}
				
		bool calcanimstate(int anim, int varseed, float speed, int basetime, dynent *d, animstate &as)
		{
			as.anim = anim;
			as.speed = speed;
			if(anims)
			{
				vector<animinfo> *primary = &anims[anim&ANIM_INDEX];
				if((anim>>ANIM_SECONDARY)&ANIM_INDEX)
				{
					vector<animinfo> *secondary = &anims[(anim>>ANIM_SECONDARY)&ANIM_INDEX];
					if(secondary->length() && (primary->empty() || (*secondary)[0].priority > (*primary)[0].priority))
					{
						primary = secondary;
						as.anim = anim>>ANIM_SECONDARY;
					}
				}
				if(primary->length())
				{
                    animinfo &ai = (*primary)[uint(varseed)%primary->length()];
					as.frame = ai.frame;
					as.range = ai.range;
					if(ai.speed>0) as.speed = 1000.0f/ai.speed;
				}
				else getdefaultanim(as, anim, varseed, d);
			}
			else getdefaultanim(as, anim, varseed, d);
			if(as.speed<=0) getanimspeed(as, d);

			as.anim &= (1<<ANIM_SECONDARY)-1;
			as.anim |= anim&ANIM_FLAGS;
            as.basetime = basetime;
            if(as.anim&(ANIM_LOOP|ANIM_START|ANIM_END) && (anim>>ANIM_SECONDARY)&ANIM_INDEX)
            {
                as.anim &= ~ANIM_SETTIME;
                as.basetime = -((int)(size_t)d&0xFFF);
            }
			if(as.anim&(ANIM_START|ANIM_END))
			{
				if(as.anim&ANIM_END) as.frame += as.range-1;
				as.range = 1; 
			}

			if(!meshes->hasframes(as.frame, as.range))
			{
				if(!meshes->hasframe(as.frame)) return false;
				as.range = meshes->clipframes(as.frame, as.range);
			}

			if(d && index<2)
			{
				if(d->lastmodel[index]!=this || d->lastanimswitchtime[index]==-1)
				{
					d->current[index] = as;
					d->lastanimswitchtime[index] = lastmillis-animationinterpolationtime*2;
				}
				else if(d->current[index]!=as)
				{
					if(lastmillis-d->lastanimswitchtime[index]>animationinterpolationtime/2) d->prev[index] = d->current[index];
					d->current[index] = as;
					d->lastanimswitchtime[index] = lastmillis;
				}
                else if(as.anim&ANIM_SETTIME) d->current[index].basetime = as.basetime;
				d->lastmodel[index] = this;
			}
			return true;
		}

		void calcnormal(GLfloat *m, vec &dir)
		{
			vec n(dir);
			dir.x = n.x*m[0] + n.y*m[1] + n.z*m[2];
			dir.y = n.x*m[4] + n.y*m[5] + n.z*m[6];
			dir.z = n.x*m[8] + n.y*m[9] + n.z*m[10];
		}

		void calcplane(GLfloat *m, plane &p)
		{
			p.offset += p.x*m[12] + p.y*m[13] + p.z*m[14];
			calcnormal(m, p);
		}

		void calcvertex(GLfloat *m, vec &pos)
		{
			vec p(pos);
				
			p.x -= m[12];
			p.y -= m[13];
			p.z -= m[14];

#if 0
			// This is probably overkill, since just about any transformations this encounters will be orthogonal matrices 
			// where their inverse is simply the transpose.
			int a = fabs(m[0])>fabs(m[1]) && fabs(m[0])>fabs(m[2]) ? 0 : (fabs(m[1])>fabs(m[2]) ? 1 : 2), b = (a+1)%3, c = (a+2)%3;
			float a1 = m[a], a2 = m[a+4], a3 = m[a+8],
				  b1 = m[b], b2 = m[b+4], b3 = m[b+8],
				  c1 = m[c], c2 = m[c+4], c3 = m[c+8];

			pos.z = (p[c] - c1*p[a]/a1 - (c2 - c1*a2/a1)*(p[b] - b1*p[a]/a1)/(b2 - b1*a2/a1)) / (c3 - c1*a3/a1 - (c2 - c1*a2/a1)*(b3 - b1*a3/a1)/(b2 - b1*a2/a1));
			pos.y = (p[b] - b1*p[a]/a1 - (b3 - b1*a3/a1)*pos.z)/(b2 - b1*a2/a1);
			pos.x = (p[a] - a2*pos.y - a3*pos.z)/a1;
#else
			pos.x = p.x*m[0] + p.y*m[1] + p.z*m[2];
			pos.y = p.x*m[4] + p.y*m[5] + p.z*m[6];
			pos.z = p.x*m[8] + p.y*m[9] + p.z*m[10];
#endif
		}

		float calcpitchaxis(int anim, GLfloat pitch, vec &axis, vec &dir, vec &campos, plane &fogplane)
		{
			float angle = pitchscale*pitch + pitchoffset;
			if(pitchmin || pitchmax) angle = max(pitchmin, min(pitchmax, angle));
			if(!angle) return 0;

			float c = cosf(-angle*RAD), s = sinf(-angle*RAD);
			vec d(axis);
			axis.rotate(c, s, d);
			if(!(anim&ANIM_NOSKIN))
			{
				dir.rotate(c, s, d);
				campos.rotate(c, s, d); 
				fogplane.rotate(c, s, d);
			}

			return angle;
		}

		void render(int anim, int varseed, float speed, int basetime, float pitch, const vec &axis, dynent *d, const vec &dir, const vec &campos, const plane &fogplane)
		{
			animstate as;
			if(!calcanimstate(anim, varseed, speed, basetime, d, as)) return;
	
			anpos prev, cur;
			cur.setframes(d && index<2 ? d->current[index] : as);
	
			float ai_t = 0;
			bool doai = d && index<2 && lastmillis-d->lastanimswitchtime[index]<animationinterpolationtime && d->prev[index].range>0;
			if(doai)
			{
				prev.setframes(d->prev[index]);
				ai_t = (lastmillis-d->lastanimswitchtime[index])/(float)animationinterpolationtime;
			}
  
			if(!model->cullface && enablecullface) { glDisable(GL_CULL_FACE); enablecullface = false; }
			else if(model->cullface && !enablecullface) { glEnable(GL_CULL_FACE); enablecullface = true; }

			vec raxis(axis), rdir(dir), rcampos(campos);
			plane rfogplane(fogplane);
			float pitchamount = calcpitchaxis(anim, pitch, raxis, rdir, rcampos, rfogplane);
			if(pitchamount)
			{
				glPushMatrix();
				glRotatef(pitchamount, axis.x, axis.y, axis.z); 
				if(renderpath!=R_FIXEDFUNCTION && anim&ANIM_ENVMAP)
				{
					glMatrixMode(GL_TEXTURE);
					glPushMatrix();
					glRotatef(pitchamount, axis.x, axis.y, axis.z);
					glMatrixMode(GL_MODELVIEW);
				}
			}

			if(!(anim&ANIM_NOSKIN))
			{
				if(renderpath!=R_FIXEDFUNCTION)
				{
					if(refracting) setfogplane(rfogplane);
					setenvparamf("direction", SHPARAM_VERTEX, 0, rdir.x, rdir.y, rdir.z);
					setenvparamf("camera", SHPARAM_VERTEX, 1, rcampos.x, rcampos.y, rcampos.z, 1);
				}
                else if(lightmodels) loopv(skins) if(!skins[i].fullbright)
				{
					GLfloat pos[4] = { rdir.x*1000, rdir.y*1000, rdir.z*1000, 0 };
					glLightfv(GL_LIGHT0, GL_POSITION, pos);
                    break;
				}
			}

			meshes->render(as, cur, doai ? &prev : NULL, ai_t, skins);

			loopv(links) if(links[i].p) // render the linked models - interpolate rotation and position of the 'link-tags'
			{
				part *link = links[i].p;

				GLfloat matrix[16];
				meshes->calctagmatrix(i, cur, doai ? &prev : NULL, ai_t, matrix);

				vec naxis(raxis), ndir(rdir), ncampos(rcampos);
				plane nfogplane(rfogplane);
				calcnormal(matrix, naxis);
				if(!(anim&ANIM_NOSKIN)) 
				{
					calcnormal(matrix, ndir);
					calcvertex(matrix, ncampos);
					calcplane(matrix, nfogplane);
				}

				glPushMatrix();
				glMultMatrixf(matrix);
				if(renderpath!=R_FIXEDFUNCTION && anim&ANIM_ENVMAP) 
				{	
					glMatrixMode(GL_TEXTURE); 
					glPushMatrix(); 
					glMultMatrixf(matrix); 
					glMatrixMode(GL_MODELVIEW); 
				}
				int nanim = anim, nbasetime = basetime;
				if(links[i].anim>=0)
				{
					nanim = links[i].anim | (anim&ANIM_FLAGS);
					nbasetime = links[i].basetime;
				}
				link->render(nanim, varseed, speed, nbasetime, pitch, naxis, d, ndir, ncampos, nfogplane);
				if(renderpath!=R_FIXEDFUNCTION && anim&ANIM_ENVMAP) 
				{ 
					glMatrixMode(GL_TEXTURE); 
					glPopMatrix(); 
					glMatrixMode(GL_MODELVIEW); 
				}
				glPopMatrix();
			}

			if(pitchamount)
			{
				glPopMatrix();
				if(renderpath!=R_FIXEDFUNCTION && anim&ANIM_ENVMAP)
				{
					glMatrixMode(GL_TEXTURE); 
					glPopMatrix(); 
					glMatrixMode(GL_MODELVIEW); 
				}
			}
		}

		void setanim(int num, int frame, int range, float speed, int priority = 0)
		{
			if(frame<0 || range<=0 || !meshes->hasframes(frame, range))
			{ 
				conoutf("invalid frame %d, range %d in model %s", frame, range, model->loadname); 
				return; 
			}
			if(!anims) anims = new vector<animinfo>[NUMANIMS];
			animinfo &ai = anims[num].add();
			ai.frame = frame;
			ai.range = range;
			ai.speed = speed;
			ai.priority = priority;
		}
	};

	bool loaded;
	char *loadname;
	vector<part *> parts;

	vertmodel(const char *name) : loaded(false)
	{
		loadname = newstring(name);
	}

	~vertmodel()
	{
		delete[] loadname;
		parts.deletecontentsp();
	}

	char *name() { return loadname; }

	virtual meshgroup *loadmeshes(char *name) { return NULL; }

	meshgroup *sharemeshes(char *name)
	{
		static hashtable<char *, vertmodel::meshgroup *> meshgroups;
		if(!meshgroups.access(name))
		{
			meshgroup *group = loadmeshes(name);
			if(!group) return NULL;
			meshgroups[group->name] = group;
		}
		return meshgroups[name];
	}

    void gentris(int frame, vector<BIH::tri> &tris)
	{
		if(parts.empty()) return;
		parts[0]->gentris(frame, tris);
	}

    BIH *setBIH()
	{
        if(bih) return bih;
        vector<BIH::tri> tris;
		gentris(0, tris);
        bih = new BIH(tris.length(), tris.getbuf());
        return bih;
	}

	void calcbb(int frame, vec &center, vec &radius)
	{
		if(parts.empty()) return;
		vec bbmin, bbmax;
		bbmin = bbmax = parts[0]->meshes->anyvert(frame);
		parts[0]->calcbb(frame, bbmin, bbmax);
		radius = bbmax;
		radius.sub(bbmin);
		radius.mul(0.5f);
		center = bbmin;
		center.add(radius);
	}

	bool link(part *link, const char *tag, int anim = -1, int basetime = 0)
	{
		loopv(parts) if(parts[i]->link(link, tag, anim, basetime)) return true;
		return false;
	}

	void setskin(int tex = 0)
	{
		loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].override = tex;
	}

	bool envmapped()
	{
		loopv(parts) loopvj(parts[i]->skins) if(parts[i]->skins[j].envmapped()) return true;
		return false;
	}

    virtual bool loaddefaultparts()
    {
        return true;
    }

	void setshader(Shader *shader)
	{
        if(parts.empty()) loaddefaultparts();
		loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].shader = shader;
	}

	void setenvmap(float envmapmin, float envmapmax, Texture *envmap)
	{
        if(parts.empty()) loaddefaultparts();
		loopv(parts) loopvj(parts[i]->skins)
		{
			skin &s = parts[i]->skins[j];
			if(envmapmax)
			{
				s.envmapmin = envmapmin;
				s.envmapmax = envmapmax;
			}
			if(envmap) s.envmap = envmap;
		}
	}

    void setspec(float spec) 
	{
        if(parts.empty()) loaddefaultparts();
		loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].spec = spec;
	}

    void setambient(float ambient)
	{
        if(parts.empty()) loaddefaultparts();
		loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].ambient = ambient;
	}

    void setglow(float glow)
	{
        if(parts.empty()) loaddefaultparts();
		loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].glow = glow;
	}

    void setalphatest(float alphatest)
	{
        if(parts.empty()) loaddefaultparts();
		loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].alphatest = alphatest;
	}

    void setalphablend(bool alphablend)
	{
        if(parts.empty()) loaddefaultparts();
		loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].alphablend = alphablend;
	}

    void settranslucency(float translucency)
	{
        if(parts.empty()) loaddefaultparts();
		loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].translucency = translucency;
	}

    void setfullbright(float fullbright)
    {
        if(parts.empty()) loaddefaultparts();
        loopv(parts) loopvj(parts[i]->skins) parts[i]->skins[j].fullbright = fullbright;
    }

	virtual void render(int anim, int varseed, float speed, int basetime, float pitch, const vec &axis, dynent *d, modelattach *a, const vec &dir, const vec &campos, const plane &fogplane)
	{
	}

	void render(int anim, int varseed, float speed, int basetime, const vec &o, float yaw, float pitch, dynent *d, modelattach *a, const vec &color, const vec &dir)
	{
		vec rdir, campos;
		plane fogplane;

		yaw += spin*lastmillis/1000.0f;

		if(!(anim&ANIM_NOSKIN))
		{
			fogplane = plane(0, 0, 1, o.z-refracting);

			lightcolor = color;
			
			rdir = dir;
			rdir.rotate_around_z((-yaw-180.0f)*RAD);

			campos = camera1->o;
			campos.sub(o);
			campos.rotate_around_z((-yaw-180.0f)*RAD);

			if(envmapped()) anim |= ANIM_ENVMAP;
			else if(a) for(int i = 0; a[i].name; i++) if(a[i].m && a[i].m->envmapped())
			{
				anim |= ANIM_ENVMAP;
				break;
			}
			if(anim&ANIM_ENVMAP) closestenvmaptex = lookupenvmap(closestenvmap(o));
		}

		if(anim&ANIM_ENVMAP)
		{
			if(renderpath==R_FIXEDFUNCTION) glActiveTexture_(GL_TEXTURE2_ARB);
			glMatrixMode(GL_TEXTURE);
			if(renderpath==R_FIXEDFUNCTION)
			{
				setuptmu(2, "T , P @ Pa", anim&ANIM_TRANSLUCENT ? "= Ka" : NULL);

				GLfloat mm[16], mmtrans[16];
				glGetFloatv(GL_MODELVIEW_MATRIX, mm);
				loopi(4) // transpose modelview (mmtrans[4*i+j] = mm[4*j+i]) and convert to (-y, z, x, w)
				{
					GLfloat x = mm[i], y = mm[4+i], z = mm[8+i], w = mm[12+i];
					mmtrans[4*i] = -y;
					mmtrans[4*i+1] = z;
					mmtrans[4*i+2] = x;
					mmtrans[4*i+3] = w;
				}
				glLoadMatrixf(mmtrans);
			}
			else
			{
				glLoadIdentity();
				glTranslatef(o.x, o.y, o.z);
				glRotatef(yaw+180, 0, 0, 1);
			}
			glMatrixMode(GL_MODELVIEW);
			if(renderpath==R_FIXEDFUNCTION) glActiveTexture_(GL_TEXTURE0_ARB);
		}

		glPushMatrix();
		glTranslatef(o.x, o.y, o.z);
		glRotatef(yaw+180, 0, 0, 1);

		if(anim&ANIM_TRANSLUCENT)
		{
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			nocolorshader->set();
			render(anim|ANIM_NOSKIN, varseed, speed, basetime, pitch, vec(0, -1, 0), d, a, rdir, campos, fogplane);
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

			glDepthFunc(GL_LEQUAL);
		}

		if(anim&(ANIM_TRANSLUCENT|ANIM_SHADOW) && !enablealphablend)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			enablealphablend = true;
		}

		render(anim, varseed, speed, basetime, pitch, vec(0, -1, 0), d, a, rdir, campos, fogplane);

		if(anim&ANIM_ENVMAP)
		{
			if(renderpath==R_FIXEDFUNCTION) glActiveTexture_(GL_TEXTURE2_ARB);
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			if(renderpath==R_FIXEDFUNCTION) glActiveTexture_(GL_TEXTURE0_ARB);
		}

		if(anim&ANIM_TRANSLUCENT) glDepthFunc(GL_LESS);

		glPopMatrix();
	}

	static bool enabletc, enablemtc, enablealphatest, enablealphablend, enableenvmap, enableglow, enablelighting, enablecullface;
	static vec lightcolor;
	static float lastalphatest;
	static GLuint lastvbo, lastenvmaptex, closestenvmaptex;
	static Texture *lasttex, *lastmasks;

	void startrender()
	{
		enabletc = enablemtc = enablealphatest = enablealphablend = enableenvmap = enableglow = enablelighting = false;
		enablecullface = true;
		lastalphatest = -1;
		lastvbo = lastenvmaptex = 0;
		lasttex = lastmasks = NULL;

		static bool initlights = false;
		if(renderpath==R_FIXEDFUNCTION && lightmodels && !initlights)
		{
			glEnable(GL_LIGHT0);
			static const GLfloat zero[4] = { 0, 0, 0, 0 };
			glLightModelfv(GL_LIGHT_MODEL_AMBIENT, zero);
			glLightfv(GL_LIGHT0, GL_SPECULAR, zero);
			glMaterialfv(GL_FRONT, GL_SPECULAR, zero);
			glMaterialfv(GL_FRONT, GL_EMISSION, zero);
			initlights = true;
		}
	}

	static void disablemtc()
	{
		glClientActiveTexture_(GL_TEXTURE1_ARB);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glClientActiveTexture_(GL_TEXTURE0_ARB);
		enablemtc = false;
	}

	static void disabletc()
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		if(enablemtc) disablemtc();
		enabletc = false;
	}

	static void disablevbo()
	{
		glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
		glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
		glDisableClientState(GL_VERTEX_ARRAY);
		if(enabletc) disabletc();
		lastvbo = 0;
	}

	static void disableglow()
	{
		resettmu(0);
		glActiveTexture_(GL_TEXTURE1_ARB);
		resettmu(1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture_(GL_TEXTURE0_ARB);
		lasttex = lastmasks = NULL;
		enableglow = false;
	}

	static void disableenvmap()
	{
		glActiveTexture_(GL_TEXTURE2_ARB);
		glDisable(GL_TEXTURE_CUBE_MAP_ARB);
		if(renderpath==R_FIXEDFUNCTION)
		{
			glDisable(GL_TEXTURE_GEN_S);
			glDisable(GL_TEXTURE_GEN_T);
			glDisable(GL_TEXTURE_GEN_R);
		}
		glActiveTexture_(GL_TEXTURE0_ARB);
		enableenvmap = false;
	}

	void endrender()
	{
		if(lastvbo) disablevbo();
		if(enablealphatest) glDisable(GL_ALPHA_TEST);
		if(enablealphablend) glDisable(GL_BLEND);
		if(enableglow) disableglow();
		if(enablelighting) glDisable(GL_LIGHTING);
		if(enableenvmap) disableenvmap();
		if(!enablecullface) glEnable(GL_CULL_FACE);
	}
};

bool vertmodel::enabletc = false, vertmodel::enablemtc = false, vertmodel::enablealphatest = false, vertmodel::enablealphablend = false, 
	 vertmodel::enableenvmap = false, vertmodel::enableglow = false, vertmodel::enablelighting = false, vertmodel::enablecullface = true;
vec vertmodel::lightcolor;
float vertmodel::lastalphatest = -1;
GLuint vertmodel::lastvbo = 0, vertmodel::lastenvmaptex = 0, vertmodel::closestenvmaptex = 0;
Texture *vertmodel::lasttex = NULL, *vertmodel::lastmasks = NULL;

