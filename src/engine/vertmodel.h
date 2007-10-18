VARP(lightmodels, 0, 1, 1);
VARP(envmapmodels, 0, 1, 1);
VARP(glowmodels, 0, 1, 1);
VARP(bumpmodels, 0, 1, 1);

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
    struct vvertbump : vvert { vec tangent; float bitangent; };
    struct tcvert { float u, v; };
    struct bumpvert { vec tangent; float bitangent; };
    struct tri { ushort vert[3]; };

    struct part;

    struct skin
    {
        part *owner;
        Texture *tex, *masks, *envmap, *unlittex, *normalmap;
        int override;
        Shader *shader;
        float spec, ambient, glow, fullbright, envmapmin, envmapmax, translucency, scrollu, scrollv, alphatest;
        bool alphablend;

        skin() : owner(0), tex(notexture), masks(notexture), envmap(NULL), unlittex(NULL), normalmap(NULL), override(0), shader(NULL), spec(1.0f), ambient(0.3f), glow(3.0f), fullbright(0), envmapmin(0), envmapmax(0), translucency(0.5f), scrollu(0), scrollv(0), alphatest(0.9f), alphablend(true) {}

        bool multitextured() { return enableglow; }
        bool envmapped() { return hasCM && envmapmax>0 && envmapmodels && (renderpath!=R_FIXEDFUNCTION || maxtmus >= (refracting && refractfog ? 4 : 3)); }
        bool bumpmapped() { return renderpath!=R_FIXEDFUNCTION && normalmap && bumpmodels; }
        bool normals() { return renderpath!=R_FIXEDFUNCTION || (lightmodels && !fullbright) || envmapped() || bumpmapped(); }
        bool tangents() { return bumpmapped(); }

        void setuptmus(animstate &as, bool masked)
        {
            if(fullbright && enablelighting) { glDisable(GL_LIGHTING); enablelighting = false; }
            else if(lightmodels && !enablelighting) { glEnable(GL_LIGHTING); enablelighting = true; }
            int needsfog = -1;
            if(refracting && refractfog)
            {
                needsfog = masked ? 2 : 1;
                if(fogtmu!=needsfog && fogtmu>=0) disablefog(true);
            }
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
                    glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
                    colortmu(envmaptmu, 0, 0, 0, translucency);
                }

                if(needsfog<0) glActiveTexture_(GL_TEXTURE0_ARB);

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
            if(needsfog>=0)
            {
                if(needsfog!=fogtmu)
                {
                    fogtmu = needsfog;
                    glActiveTexture_(GL_TEXTURE0_ARB+fogtmu);
                    glEnable(GL_TEXTURE_1D);
                    glEnable(GL_TEXTURE_GEN_S);
                    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
                    setuptmu(fogtmu, "K , P @ Ta", masked && as.anim&ANIM_ENVMAP && envmapmax>0 ? "Ka , Pa @ Ta" : "= Pa");
                    uchar wcol[3];
                    getwatercolour(wcol);
                    colortmu(fogtmu, wcol[0]/255.0f, wcol[1]/255.0f, wcol[2]/255.0f, 0);
                    if(!fogtex) createfogtex();
                    glBindTexture(GL_TEXTURE_1D, fogtex);
                }
                else glActiveTexture_(GL_TEXTURE0_ARB+fogtmu);
                if(!enablefog) { glEnable(GL_TEXTURE_1D); enablefog = true; }
                GLfloat s[4] = { -refractfogplane.x/waterfog, -refractfogplane.y/waterfog, -refractfogplane.z/waterfog, -refractfogplane.offset/waterfog };
                glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
                glActiveTexture_(GL_TEXTURE0_ARB);
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

                #define SETMODELSHADER(name) \
                    do \
                    { \
                        static Shader *name##shader = NULL; \
                        if(!name##shader) name##shader = lookupshaderbyname(#name); \
                        name##shader->set(); \
                    } \
                    while(0)
                if(shader) shader->set();
                else if(bumpmapped())
                {
                    if(as.anim&ANIM_ENVMAP && envmapmax>0)
                    {
                        if(lightmodels && !fullbright && (masked || spec>=0.01f)) SETMODELSHADER(bumpenvmapmodel);
                        else SETMODELSHADER(bumpenvmapnospecmodel);
                        setlocalparamf("envmapscale", SHPARAM_PIXEL, 6, envmapmin-envmapmax, envmapmax);
                    }
                    else if(masked && lightmodels && !fullbright) SETMODELSHADER(bumpmasksmodel);
                    else if(masked && glowmodels) SETMODELSHADER(bumpmasksnospecmodel);
                    else if(spec>=0.01f && lightmodels && !fullbright) SETMODELSHADER(bumpmodel);
                    else SETMODELSHADER(bumpnospecmodel);
                }
                else if(as.anim&ANIM_ENVMAP && envmapmax>0)
                {
                    if(lightmodels && !fullbright && (masked || spec>=0.01f)) SETMODELSHADER(envmapmodel);
                    else SETMODELSHADER(envmapnospecmodel);
                    setlocalparamf("envmapscale", SHPARAM_VERTEX, 6, envmapmin-envmapmax, envmapmax);
                }
                else if(masked && lightmodels && !fullbright) SETMODELSHADER(masksmodel);
                else if(masked && glowmodels) SETMODELSHADER(masksnospecmodel);
                else if(spec>=0.01f && lightmodels && !fullbright) SETMODELSHADER(stdmodel);
                else SETMODELSHADER(nospecmodel);
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
                if(enablefog) disablefog(true);
                return;
            }
            Texture *s = bumpmapped() && unlittex ? unlittex : tex, *m = masks, *n = bumpmapped() ? normalmap : NULL;
            if(override)
            {
                Slot &slot = lookuptexture(override);
                s = slot.sts[0].t;
                if(slot.sts.length() >= 2) 
                {
                    m = slot.sts[1].t;
                    if(n && slot.sts.length() >= 3) n = slot.sts[2].t;
                }
            }
            if((renderpath==R_FIXEDFUNCTION || !lightmodels) && 
               (!glowmodels || (renderpath==R_FIXEDFUNCTION && refracting && refractfog && maxtmus<=2)) &&
               (!envmapmodels || !(as.anim&ANIM_ENVMAP) || envmapmax<=0)) 
                m = notexture;
            setshader(as, m!=notexture);
            if(s!=lasttex)
            {
                if(enableglow) glActiveTexture_(GL_TEXTURE1_ARB);
                glBindTexture(GL_TEXTURE_2D, s->gl);
                if(enableglow) glActiveTexture_(GL_TEXTURE0_ARB);
                lasttex = s;
            }
            if(n && n!=lastnormalmap)
            {
                glActiveTexture_(GL_TEXTURE3_ARB);
                glBindTexture(GL_TEXTURE_2D, n->gl);
                glActiveTexture_(GL_TEXTURE0_ARB);
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
            if((renderpath!=R_FIXEDFUNCTION || m!=notexture) && as.anim&ANIM_ENVMAP && envmapmax>0)
            {
                GLuint emtex = envmap ? envmap->gl : closestenvmaptex;
                if(!enableenvmap || lastenvmaptex!=emtex)
                {
                    glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
                    if(!enableenvmap)
                    {
                        glEnable(GL_TEXTURE_CUBE_MAP_ARB);
                        if(!lastenvmaptex && renderpath==R_FIXEDFUNCTION)
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
        uchar *vdata;
        GLuint vbuf;
        anpos cur, prev;
        float t;
        int millis;
 
        vbocacheentry() : vdata(NULL), vbuf(0) { cur.fr1 = prev.fr1 = -1; }
    };

    struct mesh
    {
        meshgroup *group;        
        char *name;
        vert *verts;
        tcvert *tcverts;
        bumpvert *bumpverts;
        tri *tris;
        int numverts, numtris;

        int voffset, eoffset, elen;
        ushort minvert, maxvert;

        mesh() : group(0), name(0), verts(0), tcverts(0), bumpverts(0), tris(0)
        {
        }

        ~mesh()
        {
            DELETEA(name);
            DELETEA(verts);
            DELETEA(tcverts);
            DELETEA(bumpverts);
            DELETEA(tris);
        }

        mesh *copy()
        {
            mesh &m = *new mesh;
            m.name = newstring(name);
            m.numverts = numverts;
            m.verts = new vert[numverts*group->numframes];
            memcpy(m.verts, verts, numverts*group->numframes*sizeof(vert));
            m.tcverts = new tcvert[numverts];
            memcpy(m.tcverts, tcverts, numverts*sizeof(tcvert));
            m.numtris = numtris;
            m.tris = new tri[numtris];
            memcpy(m.tris, tris, numtris*sizeof(tri));
            if(bumpverts)
            {
                m.bumpverts = new bumpvert[numverts];
                memcpy(m.bumpverts, bumpverts, numverts*sizeof(bumpvert));
            }
            else m.bumpverts = NULL;
            return &m;
        }

        void calctangents()
        {
            if(bumpverts) return;
            vec *tangent = new vec[2*numverts], *bitangent = tangent+numverts;
            memset(tangent, 0, 2*numverts*sizeof(vec));
            bumpverts = new bumpvert[group->numframes*numverts];
            loopk(group->numframes)
            {
                vert *fverts = &verts[k*numverts];
                loopi(numtris)
                {
                    const tri &t = tris[i];
                    const tcvert &tc0 = tcverts[t.vert[0]],
                                 &tc1 = tcverts[t.vert[1]],
                                 &tc2 = tcverts[t.vert[2]];
 
                    vec v0(fverts[t.vert[0]].pos),
                        e1(fverts[t.vert[1]].pos), 
                        e2(fverts[t.vert[2]].pos);
                    e1.sub(v0);
                    e2.sub(v0);
 
                    float u1 = tc1.u - tc0.u, v1 = tc1.v - tc0.v, 
                          u2 = tc2.u - tc0.u, v2 = tc2.v - tc0.v,
                          scale = u1*v2 - u2*v1;
                    if(scale!=0) scale = 1.0f / scale;
                    vec u(e1), v(e2);
                    u.mul(v2).sub(vec(e2).mul(v1)).mul(scale);
                    v.mul(u1).sub(vec(e1).mul(u2)).mul(scale);
 
                    loopj(3)
                    {
                        tangent[t.vert[j]].add(u);
                        bitangent[t.vert[j]].add(v);
                    }
                }
                bumpvert *fbumpverts = &bumpverts[k*numverts];
                loopi(numverts)
                {
                    const vec &n = fverts[i].norm,
                              &t = tangent[i],
                              &bt = bitangent[i];
                    bumpvert &bv = fbumpverts[i];
                    (bv.tangent = t).sub(vec(n).mul(n.dot(t))).normalize();
                    bv.bitangent = vec().cross(n, t).dot(bt) < 0 ? -1 : 1;
                }
            }
            delete[] tangent;
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
                vec &a = fverts[tris[j].vert[0]].pos,
                    &b = fverts[tris[j].vert[1]].pos,
                    &c = fverts[tris[j].vert[2]].pos;
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

        static inline bool comparevert(vvertff &w, int j, tcvert &tc, vert &v)
        {
            return tc.u==w.u && tc.v==w.v && v.pos==w.pos;
        }

        static inline bool comparevert(vvert &w, int j, tcvert &tc, vert &v)
        {
            return tc.u==w.u && tc.v==w.v && v.pos==w.pos && v.norm==w.norm;
        }

        inline bool comparevert(vvertbump &w, int j, tcvert &tc, vert &v)
        {
            return tc.u==w.u && tc.v==w.v && v.pos==w.pos && v.norm==w.norm && bumpverts[j].tangent==w.tangent && bumpverts[j].bitangent==w.bitangent;
        }

        static inline void assignvert(vvertff &vv, int j, tcvert &tc, vert &v)
        {
            vv.pos = v.pos;
            vv.u = tc.u;
            vv.v = tc.v;
        }

        static inline void assignvert(vvert &vv, int j, tcvert &tc, vert &v)
        {
            vv.pos = v.pos;
            vv.norm = v.norm;
            vv.u = tc.u;
            vv.v = tc.v;
        }

        inline void assignvert(vvertbump &vv, int j, tcvert &tc, vert &v)
        {
            vv.pos = v.pos;
            vv.norm = v.norm;
            vv.u = tc.u;
            vv.v = tc.v;
            vv.tangent = bumpverts[j].tangent;
            vv.bitangent = bumpverts[j].bitangent;
        }

        template<class T>
        int genvbo(vector<ushort> &idxs, int offset, vector<T> &vverts)
        {
            voffset = offset;
            eoffset = idxs.length();
            minvert = 0xFFFF;
            loopi(numtris)
            {
                tri &t = tris[i];
                loopj(3) 
                {
                    tcvert &tc = tcverts[t.vert[j]];
                    vert &v = verts[t.vert[j]];
                    loopvk(vverts)
                    {
                        if(comparevert(vverts[k], j, tc, v)) { minvert = min(minvert, (ushort)k); idxs.add((ushort)k); goto found; }
                    }
                    idxs.add(vverts.length());
                    assignvert(vverts.add(), j, tc, v);
                    found:;
                }
            }
            minvert = min(minvert, vverts.length()-1);
            maxvert = max(minvert, vverts.length()-1);
            elen = idxs.length()-eoffset;
            return vverts.length()-voffset;
        }

        int genvbo(vector<ushort> &idxs, int offset)
        {
            voffset = offset;
            eoffset = idxs.length();
            loopi(numtris)
            {
                tri &t = tris[i];
                loopj(3) idxs.add(voffset+t.vert[j]);
            }
            minvert = voffset;
            maxvert = voffset + numverts-1;
            elen = idxs.length()-eoffset;
            return numverts;
        }

        void filltc(uchar *vdata, size_t stride)
        {
            vdata = (uchar *)&((vvertff *)&vdata[voffset*stride])->u;
            loopi(numverts)
            {
                *(tcvert *)vdata = tcverts[i];
                vdata += stride;
            }
        }

        void interpverts(anpos &cur, anpos *prev, float ai_t, bool norms, bool tangents, void *vdata, skin &s)
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
            #define ip_b_ai(c)      ip(ip_v(bpvert, c, prev->t), ip_v(bvert, c, cur.t), ai_t)
            #define ip_tangent      vec(ip_v(bvert, tangent.x, cur.t), ip_v(bvert, tangent.y, cur.t), ip_v(bvert, tangent.z, cur.t))
            #define ip_tangent_ai   vec(ip_b_ai(tangent.x), ip_b_ai(tangent.y), ip_b_ai(tangent.z))
            #define iploop(type, body) \
                loopi(numverts) \
                { \
                    type &v = ((type *)vdata)[i]; \
                    body; \
                }
            if(tangents)
            {
                bumpvert *bvert1 = &bumpverts[cur.fr1 * numverts],
                         *bvert2 = &bumpverts[cur.fr2 * numverts],
                         *bpvert1 = prev ? &bumpverts[prev->fr1 * numverts] : NULL, *bpvert2 = prev ? &bumpverts[prev->fr2 * numverts] : NULL;
                if(prev) iploop(vvertbump, { v.pos = ip_pos_ai; v.norm = ip_norm_ai; v.tangent = ip_tangent_ai; v.bitangent = bvert1[i].bitangent; })
                else iploop(vvertbump, { v.pos = ip_pos; v.norm = ip_norm; v.tangent = ip_tangent; v.bitangent = bvert1[i].bitangent; })
            }
            else if(norms)
            {
                if(prev) iploop(vvert, { v.pos = ip_pos_ai; v.norm = ip_norm_ai; })
                else iploop(vvert, { v.pos = ip_pos; v.norm = ip_norm; })
            }
            else if(prev) iploop(vvertff, v.pos = ip_pos_ai)
            else iploop(vvertff, v.pos = ip_pos)
            #undef iploop
            #undef ip
            #undef ip_v
            #undef ip_v_ai
        }

        void render(animstate &as, anpos &cur, anpos *prev, float ai_t, skin &s, vbocacheentry &vc)
        {
            s.bind(as);

            if(s.multitextured() || s.tangents())
            {
                if(!enablemtc || lastmtcbuf!=lastvbuf)
                {
                    glClientActiveTexture_(GL_TEXTURE1_ARB);
                    if(!enablemtc) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                    if(lastmtcbuf!=lastvbuf)
                    {
                        size_t vertsize = group->vtangents ? sizeof(vvertbump) : (group->vnorms ? sizeof(vvert) : sizeof(vvertff));
                        vvertbump *vverts = hasVBO ? 0 : (vvertbump *)vc.vdata;
                        glTexCoordPointer(s.tangents() ? 4 : 2, GL_FLOAT, vertsize, s.tangents() ? &vverts->tangent.x : &vverts->u);
                    }
                    glClientActiveTexture_(GL_TEXTURE0_ARB);
                    lastmtcbuf = lastvbuf;
                    enablemtc = true;
                }
            }
            else if(enablemtc) disablemtc();

            if(renderpath==R_FIXEDFUNCTION && (s.scrollu || s.scrollv))
            {
                glMatrixMode(GL_TEXTURE);
                glPushMatrix();
                glTranslatef(s.scrollu*lastmillis/1000.0f, s.scrollv*lastmillis/1000.0f, 0);

                if(s.multitextured())
                {
                    glActiveTexture_(GL_TEXTURE1_ARB);
                    glPushMatrix();
                    glTranslatef(s.scrollu*lastmillis/1000.0f, s.scrollv*lastmillis/1000.0f, 0);
                }
            }

            if(hasDRE) glDrawRangeElements_(GL_TRIANGLES, minvert, maxvert, elen, GL_UNSIGNED_SHORT, &group->edata[eoffset]);
            else glDrawElements(GL_TRIANGLES, elen, GL_UNSIGNED_SHORT, &group->edata[eoffset]);
            glde++;
            xtravertsva += numverts;

            if(renderpath==R_FIXEDFUNCTION && (s.scrollu || s.scrollv))
            {
                if(s.multitextured())
                {
                    glPopMatrix();
                    glActiveTexture_(GL_TEXTURE0_ARB);
                }

                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
            }

            return;
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

        ushort *edata;
        GLuint ebuf;
        bool vnorms, vtangents;
        int vlen;
        uchar *vdata;

        meshgroup() : next(NULL), shared(0), name(NULL), tags(NULL), numtags(0), numframes(0), scale(1), translate(0, 0, 0), edata(NULL), ebuf(0), vdata(NULL) 
        {
        }

        virtual ~meshgroup()
        {
            DELETEA(name);
            meshes.deletecontentsp();
            DELETEA(tags);
            if(ebuf) glDeleteBuffers_(1, &ebuf);
            loopi(MAXVBOCACHE) 
            {
                DELETEA(vbocache[i].vdata);
                if(vbocache[i].vbuf) glDeleteBuffers_(1, &vbocache[i].vbuf);
            }
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

        void genvbo(bool norms, bool tangents, vbocacheentry &vc)
        {
            if(hasVBO) 
            {
                if(!vc.vbuf) glGenBuffers_(1, &vc.vbuf);
                if(ebuf) return;
            }
            else if(edata) 
            {
                #define ALLOCVDATA(vdata) \
                    do \
                    { \
                        DELETEA(vdata); \
                        size_t vertsize = tangents ? sizeof(vvertbump) : (norms ? sizeof(vvert) : sizeof(vvertff)); \
                        vdata = new uchar[vlen*vertsize]; \
                        loopv(meshes) meshes[i]->filltc(vdata, vertsize); \
                    } while(0)
                if(!vc.vdata) ALLOCVDATA(vc.vdata);
                return;
            }

            vector<ushort> idxs;

            vnorms = norms;
            vtangents = tangents;
            vlen = 0;
            if(numframes>1)
            {
                loopv(meshes) vlen += meshes[i]->genvbo(idxs, vlen);
                DELETEA(vdata);
                if(hasVBO) ALLOCVDATA(vdata); 
                else ALLOCVDATA(vc.vdata);
            } 
            else 
            {
                if(hasVBO) glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc.vbuf);
                #define GENVBO(type) \
                    do \
                    { \
                        vector<type> vverts; \
                        loopv(meshes) vlen += meshes[i]->genvbo(idxs, vlen, vverts); \
                        if(hasVBO) glBufferData_(GL_ARRAY_BUFFER_ARB, vverts.length()*sizeof(type), vverts.getbuf(), GL_STATIC_DRAW_ARB); \
                        else \
                        { \
                            DELETEA(vc.vdata); \
                            vc.vdata = new uchar[vverts.length()*sizeof(type)]; \
                            memcpy(vc.vdata, vverts.getbuf(), vverts.length()*sizeof(type)); \
                        } \
                    } while(0)
                if(tangents) GENVBO(vvertbump);
                else if(norms) GENVBO(vvert);
                else GENVBO(vvertff);
            }

            if(hasVBO)
            {
                glGenBuffers_(1, &ebuf);
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, ebuf);
                glBufferData_(GL_ELEMENT_ARRAY_BUFFER_ARB, idxs.length()*sizeof(ushort), idxs.getbuf(), GL_STATIC_DRAW_ARB);
            }
            else
            {
                edata = new ushort[idxs.length()];
                memcpy(edata, idxs.getbuf(), idxs.length()*sizeof(ushort));
            }
        }

        void bindvbo(animstate &as, vbocacheentry &vc)
        {
            size_t vertsize = vtangents ? sizeof(vvertbump) : (vnorms ? sizeof(vvert) : sizeof(vvertff));
            vvert *vverts = hasVBO ? 0 : (vvert *)vc.vdata;
            if(hasVBO && lastebuf!=ebuf)
            {
                glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, ebuf);
                lastebuf = ebuf;
            }
            if(lastvbuf != (hasVBO ? (void *)(size_t)vc.vbuf : vc.vdata))
            {
                if(hasVBO) glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc.vbuf);
                if(!lastvbuf) glEnableClientState(GL_VERTEX_ARRAY);
                glVertexPointer(3, GL_FLOAT, vertsize, &vverts->pos);
            }
            lastvbuf = hasVBO ? (void *)(size_t)vc.vbuf : vc.vdata;
            if(as.anim&ANIM_NOSKIN)
            {
                if(enabletc) disabletc();
            }
            else if(!enabletc || lasttcbuf!=lastvbuf)
            {
                if(vnorms || vtangents)
                {
                    if(!enabletc) glEnableClientState(GL_NORMAL_ARRAY);
                    if(lasttcbuf!=lastvbuf) glNormalPointer(GL_FLOAT, vertsize, &vverts->norm);
                }
                if(!enabletc) glEnableClientState(GL_TEXTURE_COORD_ARRAY);
                if(lasttcbuf!=lastvbuf) glTexCoordPointer(2, GL_FLOAT, vertsize, &vverts->u);
                lasttcbuf = lastvbuf;
                enabletc = true;
            }
        }

        void render(animstate &as, anpos &cur, anpos *prev, float ai_t, vector<skin> &skins)
        {
            bool norms = false, tangents = false;
            loopv(skins) 
            {
                if(skins[i].normals()) norms = true;
                if(skins[i].tangents()) tangents = true;
            }
            if(norms!=vnorms || tangents!=vtangents)
            {
                loopi(MAXVBOCACHE) 
                {
                    vbocacheentry &c = vbocache[i];
                    if(c.vbuf) { glDeleteBuffers_(1, &c.vbuf); c.vbuf = 0; }
                    DELETEA(c.vdata);
                    c.cur.fr1 = -1;
                }
                if(hasVBO) { if(ebuf) { glDeleteBuffers_(1, &ebuf); ebuf = 0; } }
                else DELETEA(vdata);
                lastvbuf = lasttcbuf = lastmtcbuf = NULL;
                lastebuf = 0;
            }
            vbocacheentry *vc = NULL;
            if(numframes<=1) vc = vbocache;
            else
            {
                loopi(MAXVBOCACHE)
                {
                    vbocacheentry &c = vbocache[i];
                    if(hasVBO ? !c.vbuf : !c.vdata) continue;
                    if(c.cur==cur && (prev ? c.prev==*prev && c.t==ai_t : c.prev.fr1<0)) { vc = &c; break; }
                }
                if(!vc) loopi(MAXVBOCACHE) { vc = &vbocache[i]; if((hasVBO ? !vc->vbuf : !vc->vdata) || vc->millis < lastmillis) break; }
            }
            if(hasVBO ? !vc->vbuf : !vc->vdata) genvbo(norms, tangents, *vc);
            if(numframes>1)
            {
                if(vc->cur!=cur || (prev ? vc->prev!=*prev || vc->t!=ai_t : vc->prev.fr1>=0))
                {
                    vc->cur = cur;
                    if(prev) { vc->prev = *prev; vc->t = ai_t; }
                    else vc->prev.fr1 = -1;
                    vc->millis = lastmillis;
                    size_t vertsize = tangents ? sizeof(vvertbump) : (norms ? sizeof(vvert) : sizeof(vvertff));
                    loopv(meshes) 
                    {
                        mesh &m = *meshes[i];
                        m.interpverts(cur, prev, ai_t, norms, tangents, (hasVBO ? vdata : vc->vdata) + m.voffset*vertsize, skins[i]);
                    }
                    if(hasVBO)
                    {
                        glBindBuffer_(GL_ARRAY_BUFFER_ARB, vc->vbuf);
                        glBufferData_(GL_ARRAY_BUFFER_ARB, vlen*vertsize, vdata, GL_STREAM_DRAW_ARB);    
                    }
                }
            }
        
            bindvbo(as, *vc);
            loopv(meshes) meshes[i]->render(as, cur, prev, ai_t, skins[i], *vc);
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
#ifdef BFRONTIER // external physics support
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
                if(d->lastmodel[index]!=this || d->lastanimswitchtime[index]==-1 || lastmillis-d->lastrendered>animationinterpolationtime)
                {
                    d->prev[index] = d->current[index] = as;
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
                else 
                {
                    if(refracting && refractfog) refractfogplane = rfogplane;
                    if(lightmodels) loopv(skins) if(!skins[i].fullbright)
                	{
                    	GLfloat pos[4] = { rdir.x*1000, rdir.y*1000, rdir.z*1000, 0 };
                   		glLightfv(GL_LIGHT0, GL_POSITION, pos);
                    	break;
                	}
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
            envmaptmu = 2;
            if(renderpath==R_FIXEDFUNCTION) 
            {
                if(refracting && refractfog) envmaptmu = 3;
                glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
            }
            glMatrixMode(GL_TEXTURE);
            if(renderpath==R_FIXEDFUNCTION)
            {
                setuptmu(envmaptmu, "T , P @ Pa", anim&ANIM_TRANSLUCENT ? "= Ka" : NULL);

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
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, refracting && renderpath!=R_FIXEDFUNCTION ? GL_FALSE : GL_TRUE);

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
            if(renderpath==R_FIXEDFUNCTION) glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            glMatrixMode(GL_MODELVIEW);
            if(renderpath==R_FIXEDFUNCTION) glActiveTexture_(GL_TEXTURE0_ARB);
        }

        if(anim&ANIM_TRANSLUCENT) glDepthFunc(GL_LESS);

        glPopMatrix();

        if(d) d->lastrendered = lastmillis;
    }

    static bool enabletc, enablemtc, enablealphatest, enablealphablend, enableenvmap, enableglow, enablelighting, enablecullface, enablefog;
    static vec lightcolor;
    static plane refractfogplane;
    static float lastalphatest;
    static void *lastvbuf, *lasttcbuf, *lastmtcbuf;
    static GLuint lastebuf, lastenvmaptex, closestenvmaptex;
    static Texture *lasttex, *lastmasks, *lastnormalmap;
    static int envmaptmu, fogtmu;

    void startrender()
    {
        enabletc = enablemtc = enablealphatest = enablealphablend = enableenvmap = enableglow = enablelighting = enablefog = false;
        enablecullface = true;
        lastalphatest = -1;
        lastvbuf = lasttcbuf = lastmtcbuf = NULL;
        lastebuf = lastenvmaptex = closestenvmaptex = 0;
        lasttex = lastmasks = lastnormalmap = NULL;
        envmaptmu = fogtmu = -1;

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
        if(hasVBO)
        {
            glBindBuffer_(GL_ARRAY_BUFFER_ARB, 0);
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
        }
        glDisableClientState(GL_VERTEX_ARRAY);
        if(enabletc) disabletc();
        lastvbuf = lasttcbuf = lastmtcbuf = NULL;
        lastebuf = 0;
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

    static void disablefog(bool cleanup = false)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+fogtmu);
        if(enablefog) glDisable(GL_TEXTURE_1D);
        if(cleanup)
    {
            resettmu(fogtmu);
            glDisable(GL_TEXTURE_GEN_S);
            fogtmu = -1;
        }
        glActiveTexture_(GL_TEXTURE0_ARB);
        enablefog = false;
    }

    static void disableenvmap(bool cleanup = false)
    {
        glActiveTexture_(GL_TEXTURE0_ARB+envmaptmu);
        if(enableenvmap) glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        if(cleanup && renderpath==R_FIXEDFUNCTION)
        {
            resettmu(envmaptmu);
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_GEN_R);
        }
        glActiveTexture_(GL_TEXTURE0_ARB);
        enableenvmap = false;
    }

    void endrender()
    {
        if(lastvbuf || lastebuf) disablevbo();
        if(enablealphatest) glDisable(GL_ALPHA_TEST);
        if(enablealphablend) glDisable(GL_BLEND);
        if(enableglow) disableglow();
        if(enablelighting) glDisable(GL_LIGHTING);
        if(lastenvmaptex) disableenvmap(true);
        if(!enablecullface) glEnable(GL_CULL_FACE);
        if(fogtmu>=0) disablefog(true);
    }
};

bool vertmodel::enabletc = false, vertmodel::enablemtc = false, vertmodel::enablealphatest = false, vertmodel::enablealphablend = false, 
     vertmodel::enableenvmap = false, vertmodel::enableglow = false, vertmodel::enablelighting = false, vertmodel::enablecullface = true,
     vertmodel::enablefog = false;
vec vertmodel::lightcolor;
plane vertmodel::refractfogplane;
float vertmodel::lastalphatest = -1;
void *vertmodel::lastvbuf = NULL, *vertmodel::lasttcbuf = NULL, *vertmodel::lastmtcbuf = NULL;
GLuint vertmodel::lastebuf = 0, vertmodel::lastenvmaptex = 0, vertmodel::closestenvmaptex = 0;
Texture *vertmodel::lasttex = NULL, *vertmodel::lastmasks = NULL, *vertmodel::lastnormalmap = NULL;
int vertmodel::envmaptmu = -1, vertmodel::fogtmu = -1;

