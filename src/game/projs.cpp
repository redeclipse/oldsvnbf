#include "game.h"
extern int emitmillis;
namespace projs
{
    struct hitmsg
    {
        int flags, target, id, dist;
        ivec dir;
    };
    vector<hitmsg> hits;
    vector<projent *> projs;

    VAR(IDF_PERSIST, maxprojectiles, 1, 128, INT_MAX-1);

    VAR(IDF_PERSIST, ejectfade, 0, 3500, INT_MAX-1);
    VAR(IDF_PERSIST, ejectspin, 0, 1, 1);
    VAR(IDF_PERSIST, ejecthint, 0, 1, 1);

    VAR(IDF_PERSIST, firetrail, 0, 1, 1);
    VAR(IDF_PERSIST, firedelay, 1, 75, INT_MAX-1);
    VAR(IDF_PERSIST, firelength, 1, 350, INT_MAX-1);
    VAR(IDF_PERSIST, firehint, 0, 1, 1);

    VAR(IDF_PERSIST, muzzleflash, 0, 3, 3); // 0 = off, 1 = only other players, 2 = only thirdperson, 3 = all
    VAR(IDF_PERSIST, muzzleflare, 0, 3, 3); // 0 = off, 1 = only other players, 2 = only thirdperson, 3 = all
    FVAR(IDF_PERSIST, muzzleblend, 0, 1, 1);

    #define muzzlechk(a,b) (a == 3 || (a == 2 && game::thirdpersonview(true)) || (a == 1 && b != game::focus))
    #define notrayspam(a,b,c) (WEAP2(a, rays, b) <= c || !rnd(max(c, 2)))

    int calcdamage(gameent *actor, gameent *target, int weap, int &flags, int radial, float size, float dist)
    {
        int damage = WEAP2(weap, damage, flags&HIT_ALT), nodamage = 0; flags &= ~HIT_SFLAGS;
        if((flags&HIT_WAVE || (isweap(weap) && !WEAPEX(weap, flags&HIT_ALT, game::gamemode, game::mutators, 1.f))) && flags&HIT_FULL) flags &= ~HIT_FULL;
        if(radial) damage = int(ceilf(damage*clamp(1.f-dist/size, 1e-3f, 1.f)));
        else if(WEAP2(weap, taper, flags&HIT_ALT) > 0) damage = int(ceilf(damage*clamp(dist, 0.f, 1.f)));
        if(actor->aitype < AI_START)
        {
            if((actor == target && !selfdamage) || (m_trial(game::gamemode) && !trialdamage)) nodamage++;
            else if(m_play(game::gamemode) && m_team(game::gamemode, game::mutators) && actor->team == target->team && actor != target)
            {
                switch(teamdamage)
                {
                    case 2: default: break;
                    case 1: if(actor->aitype < 0 || target->aitype >= 0) break;
                    case 0: nodamage++; break;
                }
            }
        }
        if(nodamage || !hithurts(flags)) flags = HIT_WAVE|(flags&HIT_ALT ? HIT_ALT : 0); // so it impacts, but not hurts
        if(hithurts(flags))
        {
            if(flags&HIT_FULL || flags&HIT_HEAD) damage = int(ceilf(damage*damagescale));
            else if(flags&HIT_TORSO) damage = int(ceilf(damage*0.5f*damagescale));
            else if(flags&HIT_LEGS) damage = int(ceilf(damage*0.25f*damagescale));
            else damage = 0;
        }
        else damage = int(ceilf(damage*damagescale));
        return damage;
    }

    void hitpush(gameent *d, projent &proj, int flags = 0, int radial = 0, float dist = 0)
    {
        vec dir, middle = d->o;
        middle.z += (d->aboveeye-d->height)/2;
        dir = vec(middle).sub(proj.o);
        float dmag = dir.magnitude();
        if(dmag > 1e-3f) dir.div(dmag);
        else dir = vec(0, 0, 1);
        float speed = proj.vel.magnitude();
        if(speed > 1e-6f)
        {
            dir.add(vec(proj.vel).div(speed));
            dmag = dir.magnitude();
            if(dmag > 1e-3f) dir.div(dmag);
            else dir = vec(0, 0, 1);
        }
        if(proj.owner && (proj.owner == game::player1 || proj.owner->ai))
        {
            int hflags = proj.flags|flags;
            float size = hflags&HIT_WAVE ? radial*WEAP(proj.weap, pusharea) : radial;
            int damage = calcdamage(proj.owner, d, proj.weap, hflags, radial, size, dist);
            game::hiteffect(proj.weap, hflags, damage, d, proj.owner, dir, false);
        }
        hitmsg &h = hits.add();
        h.flags = flags;
        h.target = d->clientnum;
        h.id = lastmillis-game::maptime;
        h.dist = int(dist*DNF);
        h.dir = ivec(int(dir.x*DNF), int(dir.y*DNF), int(dir.z*DNF));
    }

    void hitproj(gameent *d, projent &proj)
    {
        int flags = 0;
        if(proj.hitflags&HITFLAG_LEGS) flags |= HIT_LEGS;
        if(proj.hitflags&HITFLAG_TORSO) flags |= HIT_TORSO;
        if(proj.hitflags&HITFLAG_HEAD) flags |= HIT_HEAD;
        if(flags) hitpush(d, proj, flags|HIT_PROJ, 0, proj.lifesize*proj.scale);
    }

    bool hiteffect(projent &proj, physent *d, int flags, const vec &norm)
    {
        if(proj.projtype == PRJ_SHOT && physics::issolid(d, &proj))
        {
            proj.hit = d;
            proj.hitflags = flags;
            proj.norm = norm;
            if(!WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale) && (d->type == ENT_PLAYER || d->type == ENT_AI)) hitproj((gameent *)d, proj);
            switch(proj.weap)
            {
                case WEAP_RIFLE:
                    part_splash(PART_SPARK, 25, 250, proj.o, 0x6611FF, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.125f, 1, 20, 0, 24);
                    part_create(PART_PLASMA, 250, proj.o, 0x6611FF, 2, 1, 0, 0);
                    adddynlight(proj.o, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*1.5f, vec(0.4f, 0.05f, 1.f), 250, 10);
                    break;
                default:
                    if(weaptype[proj.weap].melee)
                        part_create(PART_PLASMA_SOFT, 250, proj.o, 0xAA8811, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT));
                    break;
            }
            return (proj.projcollide&COLLIDE_CONT) ? false : true;
        }
        return false;
    }

    bool radialeffect(gameent *d, projent &proj, bool explode, int radius)
    {
        bool radiated = false;
        float maxdist = explode ? radius*WEAP(proj.weap, pusharea) : radius, dist = 1e16f;
        if(d->type == ENT_PLAYER || (d->type == ENT_AI && (!isaitype(d->aitype) || aistyle[d->aitype].canmove)))
        {
            if(!proj.o.reject(d->legs, maxdist+max(d->lrad.x, d->lrad.y)))
            {
                vec bottom(d->legs), top(d->legs); bottom.z -= d->lrad.z; top.z += d->lrad.z;
                dist = min(dist, closestpointcylinder(proj.o, bottom, top, max(d->lrad.x, d->lrad.y)).dist(proj.o));
            }
            if(!proj.o.reject(d->torso, maxdist+max(d->trad.x, d->trad.y)))
            {
                vec bottom(d->torso), top(d->torso); bottom.z -= d->trad.z; top.z += d->trad.z;
                dist = min(dist, closestpointcylinder(proj.o, bottom, top, max(d->trad.x, d->trad.y)).dist(proj.o));
            }
            if(!proj.o.reject(d->head, maxdist+max(d->hrad.x, d->hrad.y)))
            {
                vec bottom(d->head), top(d->head); bottom.z -= d->hrad.z; top.z += d->hrad.z;
                dist = min(dist, closestpointcylinder(proj.o, bottom, top, max(d->hrad.x, d->hrad.y)).dist(proj.o));
            }
        }
        else
        {
            vec bottom(d->o), top(d->o); bottom.z -= d->height; top.z += d->aboveeye;
            dist = closestpointcylinder(proj.o, bottom, top, d->radius).dist(proj.o);
        }
        if(explode && dist <= radius*WEAP(proj.weap, pusharea)) { hitpush(d, proj, HIT_WAVE, radius, dist); radiated = true; }
        if(dist <= radius) { hitpush(d, proj, HIT_FULL|(explode ? HIT_EXPLODE : HIT_BURN), radius, dist); radiated = true; }
        return radiated;
    }

    void remove(gameent *owner)
    {
        loopv(projs) if(projs[i]->owner == owner)
        {
            if(projs[i]->projtype == PRJ_SHOT)
            {
                delete projs[i];
                projs.removeunordered(i--);
            }
            else projs[i]->owner = NULL;
        }
    }

    void reset()
    {
        projs.deletecontents();
        projs.shrink(0);
    };

    void preload()
    {
        loopi(WEAP_MAX) if(*weaptype[i].proj) loadmodel(weaptype[i].proj, -1, true);
        const char *mdls[] = { "projs/gibs/gibc", "projs/gibs/gibh", "projs/debris/debris01", "projs/debris/debris02", "projs/debris/debris03", "projs/debris/debris04", "" };
        for(int i = 0; *mdls[i]; i++) loadmodel(mdls[i], -1, true);
    }

    void reflect(projent &proj, vec &pos)
    {
        bool speed = proj.vel.magnitude() > 0.01f;
        float elasticity = speed ? proj.elasticity : 1.f, reflectivity = speed ? proj.reflectivity : 0.f;
        if(elasticity > 0.f)
        {
            vec dir[2]; dir[0] = dir[1] = vec(proj.vel).normalize();
            float mag = proj.vel.magnitude()*elasticity; // conservation of energy
            dir[1].reflect(pos);
            if(!proj.lastbounce && reflectivity > 0.f)
            { // if projectile returns at 180 degrees [+/-]reflectivity, skew the reflection
                float aim[2][2] = { { 0.f, 0.f }, { 0.f, 0.f } };
                loopi(2) vectoyawpitch(dir[i], aim[0][i], aim[1][i]);
                loopi(2)
                {
                    float rmax = 180.f+reflectivity, rmin = 180.f-reflectivity,
                        off = aim[i][1]-aim[i][0];
                    if(fabs(off) <= rmax && fabs(off) >= rmin)
                    {
                        if(off > 0.f ? off > 180.f : off < -180.f)
                            aim[i][1] += rmax-off;
                        else aim[i][1] -= off-rmin;
                    }
                    while(aim[i][1] < 0.f) aim[i][1] += 360.f;
                    while(aim[i][1] >= 360.f) aim[i][1] -= 360.f;
                }
                vecfromyawpitch(aim[0][1], aim[1][1], 1, 0, dir[1]);
            }
            proj.vel = vec(dir[1]).mul(mag);
        }
        else proj.vel = vec(0, 0, 0);
    }

    void bounceeffect(projent &proj)
    {
        if(proj.movement > 1 && (!proj.lastbounce || lastmillis-proj.lastbounce > 500)) switch(proj.projtype)
        {
            case PRJ_SHOT:
            {
                switch(proj.weap)
                {
                    case WEAP_SHOTGUN: case WEAP_SMG:
                    {
                        part_splash(PART_SPARK, 5, 250, proj.o, 0xFFAA22, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.25f, 1, 20, 0, 16);
                        if(notrayspam(proj.weap, proj.flags&HIT_ALT, 5)) adddecal(DECAL_BULLET, proj.o, proj.norm, proj.weap == WEAP_SHOTGUN ? 3.f : 1.5f);
                        break;
                    }
                    case WEAP_FLAMER:
                    {
                        if(notrayspam(proj.weap, proj.flags&HIT_ALT, 0))
                            adddecal(DECAL_SCORCH_SHORT, proj.o, proj.norm, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*proj.lifesize));
                        break;
                    }
                    default: break;
                }
                if(weaptype[proj.weap].rsound >= 0)
                {
                    int vol = int(245*(1.f-proj.lifespan)*proj.scale)+10;
                    playsound(weaptype[proj.weap].rsound, proj.o, NULL, 0, vol);
                }
                break;
            }
            case PRJ_GIBS:
            {
                if(!kidmode && game::bloodscale > 0 && game::debrisscale > 0)
                {
                    adddecal(DECAL_BLOOD, proj.o, proj.norm, proj.radius*clamp(proj.vel.magnitude(), 0.25f, 2.f), bvec(125, 255, 255));
                    int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 10, 255);
                    playsound(S_SPLOSH, proj.o, NULL, 0, vol);
                    break;
                } // otherwise fall through
            }
            case PRJ_DEBRIS:
            {
                int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 10, 255);
                playsound(S_DEBRIS, proj.o, NULL, 0, vol);
                break;
            }
            case PRJ_EJECT:
            {
                int mag = int(proj.vel.magnitude()), vol = clamp(mag*3, 10, 255);
                playsound(S_SHELL1+rnd(6), proj.o, NULL, 0, vol);
                break;
            }
            default: break;
        }
    }

    void updatebb(projent &proj, bool init = false)
    {
        if(proj.mdl && *proj.mdl)
        {
            float size = 1;
            switch(proj.projtype)
            {
                case PRJ_GIBS: case PRJ_DEBRIS: case PRJ_EJECT: size = proj.lifesize;
                case PRJ_ENT:
                    if(init) break;
                    else if(proj.lifemillis && proj.fadetime)
                    {
                        int interval = min(proj.lifemillis, proj.fadetime);
                        if(proj.lifetime < interval)
                        {
                            size *= float(proj.lifetime)/float(interval);
                            break;
                        }
                    } // all falls through to ..
                default: return;
            }
            setbbfrommodel(&proj, proj.mdl, size*proj.scale);
            switch(proj.projtype)
            {
                case PRJ_GIBS: case PRJ_DEBRIS: proj.height += size*proj.scale*0.5f; break;
                case PRJ_EJECT: proj.height += size*proj.scale*0.25f; break;
                case PRJ_ENT: proj.height += size*proj.scale*4.f; break;
            }
        }
        if(init)
        {
            vec orig = proj.o;
            loopi(100)
            {
                if(i) proj.o.add(vec((rnd(21)-10)*i/10.f, (rnd(21)-10)*i/10.f, (rnd(21)-10)*i/10.f));
                if(collide(&proj) && !inside && (!hitplayer || !physics::issolid(hitplayer, &proj))) break;
                proj.o = orig;
            }
        }
    }

    void init(projent &proj, bool waited)
    {
        switch(proj.projtype)
        {
            case PRJ_SHOT:
            {
                if(proj.owner)
                {
                    if(weaptype[proj.weap].traced)
                    {
                        proj.o = proj.from = proj.owner->headpos(proj.owner->height/2);
                        proj.to = proj.owner->muzzlepos(proj.weap);
                    }
                    else
                    {
                        proj.from = proj.weap == WEAP_MELEE && proj.flags&HIT_ALT ? proj.owner->feetpos(1) : proj.owner->muzzlepos(proj.weap);
                        if(waited) proj.o = proj.from;
                    }
                }
                proj.height = proj.radius = proj.xradius = proj.yradius = WEAP2(proj.weap, radius, proj.flags&HIT_ALT);
                proj.elasticity = WEAP2(proj.weap, elasticity, proj.flags&HIT_ALT);
                proj.reflectivity = WEAP2(proj.weap, reflectivity, proj.flags&HIT_ALT);
                proj.relativity = WEAP2(proj.weap, relativity, proj.flags&HIT_ALT);
                proj.waterfric = WEAP2(proj.weap, waterfric, proj.flags&HIT_ALT);
                proj.weight = WEAP2(proj.weap, weight, proj.flags&HIT_ALT);
                proj.projcollide = WEAP2(proj.weap, collide, proj.flags&HIT_ALT);
                proj.extinguish = WEAP2(proj.weap, extinguish, proj.flags&HIT_ALT);
                proj.lifesize = 1;
                proj.mdl = weaptype[proj.weap].proj;
                proj.escaped = !proj.owner || weaptype[proj.weap].traced;
                break;
            }
            case PRJ_GIBS:
            {
                if(!kidmode && game::bloodscale > 0 && game::debrisscale > 0)
                {
                    if(proj.owner)
                    {
                        if(proj.owner->state == CS_DEAD || proj.owner->state == CS_WAITING)
                            proj.o = ragdollcenter(proj.owner);
                        else
                        {
                            proj.lifemillis = proj.lifetime = 1;
                            proj.lifespan = 1.f;
                            proj.state = CS_DEAD;
                            return;
                        }
                    }
                    proj.lifesize = 1.5f-(rnd(100)/100.f);
                    proj.mdl = rnd(2) ? "projs/gibs/gibc" : "projs/gibs/gibh";
                    proj.aboveeye = 1.0f;
                    proj.elasticity = 0.3f;
                    proj.reflectivity = 0.f;
                    proj.relativity = 0.95f;
                    proj.waterfric = 2.0f;
                    proj.weight = 150.f*proj.lifesize;
                    proj.vel.add(vec(rnd(21)-10, rnd(21)-10, rnd(21)-10));
                    proj.projcollide = BOUNCE_GEOM|BOUNCE_PLAYER;
                    proj.escaped = !proj.owner;
                    proj.fadetime = rnd(250)+250;
                    break;
                } // otherwise fall through
            }
            case PRJ_DEBRIS:
            {
                proj.lifesize = 1.5f-(rnd(100)/100.f);
                switch(rnd(4))
                {
                    case 3: proj.mdl = "projs/debris/debris04"; break;
                    case 2: proj.mdl = "projs/debris/debris03"; break;
                    case 1: proj.mdl = "projs/debris/debris02"; break;
                    case 0: default: proj.mdl = "projs/debris/debris01"; break;
                }
                proj.aboveeye = 1.0f;
                proj.elasticity = 0.6f;
                proj.reflectivity = 0.f;
                proj.relativity = 0.0f;
                proj.waterfric = 1.7f;
                proj.weight = 150.f*proj.lifesize;
                proj.vel.add(vec(rnd(101)-50, rnd(101)-50, rnd(151)-50)).mul(2);
                proj.projcollide = BOUNCE_GEOM|BOUNCE_PLAYER|COLLIDE_OWNER;
                proj.escaped = !proj.owner;
                proj.fadetime = rnd(250)+250;
                proj.extinguish = 2;
                break;
            }
            case PRJ_EJECT:
            {
                if(!isweap(proj.weap) && proj.owner) proj.weap = proj.owner->weapselect;
                if(isweap(proj.weap))
                {
                    if(proj.owner) proj.o = proj.from = proj.owner->ejectpos(proj.weap);
                    proj.mdl = weaptype[proj.weap].eject && *weaptype[proj.weap].eprj ? weaptype[proj.weap].eprj : "projs/catridge";
                    proj.lifesize = weaptype[proj.weap].esize;
                }
                else { proj.mdl = "projs/catridge"; proj.lifesize = 1; }
                proj.aboveeye = 1.0f;
                proj.elasticity = 0.3f;
                proj.reflectivity = 0.f;
                proj.relativity = 0.95f;
                proj.waterfric = 1.75f;
                proj.weight = (180+(proj.maxspeed*2))*proj.lifesize; // so they fall better in relation to their speed
                proj.projcollide = BOUNCE_GEOM;
                proj.escaped = true;
                proj.fadetime = rnd(250)+250;
                if(proj.owner)
                {
                    if(proj.owner == game::focus && !game::thirdpersonview())
                        proj.o = proj.from.add(vec(proj.from).sub(camera1->o).normalize().mul(5));
                    vecfromyawpitch(proj.owner->yaw+40+rnd(41), proj.owner->pitch+50-proj.maxspeed+rnd(41), 1, 0, proj.to);
                    proj.to.mul(10).add(proj.from);
                    proj.yaw = proj.owner->yaw;
                    proj.pitch = proj.owner->pitch;
                }
                break;
            }
            case PRJ_ENT:
            {
                proj.lifesize = proj.aboveeye = 1.f;
                proj.elasticity = 0.35f;
                proj.reflectivity = 0.f;
                proj.relativity = 0.95f;
                proj.waterfric = 1.75f;
                proj.weight = 175.f;
                proj.projcollide = BOUNCE_GEOM;
                proj.escaped = true;
                if(proj.owner) proj.o.sub(vec(0, 0, proj.owner->height*0.2f));
                proj.vel.add(vec(rnd(51)-25, rnd(51)-25, rnd(25)));
                proj.fadetime = 500;
                break;
            }
            default: break;
        }
        if(proj.projtype != PRJ_SHOT) updatebb(proj, true);
        if(proj.projtype != PRJ_SHOT || !weaptype[proj.weap].traced)
        {
            vec dir = vec(proj.to).sub(proj.o), orig = proj.o;
            float maxdist = dir.magnitude();
            if(maxdist > 1e-3f)
            {
                dir.mul(1/maxdist);
                if(proj.projtype != PRJ_EJECT) vectoyawpitch(dir, proj.yaw, proj.pitch);
            }
            else if(proj.owner)
            {
                if(proj.projtype != PRJ_EJECT)
                {
                    proj.yaw = proj.owner->yaw;
                    proj.pitch = proj.owner->pitch;
                }
                vecfromyawpitch(proj.yaw, proj.pitch, 1, 0, dir);
            }
            vec rel = vec(proj.vel).add(dir);
            if(proj.owner && proj.relativity > 0)
            {
                vec r = vec(proj.owner->vel).add(proj.owner->falling);
                if(r.x*rel.x < 0) r.x = 0;
                if(r.y*rel.y < 0) r.y = 0;
                if(r.z*rel.z < 0) r.z = 0;
                rel.add(r.mul(proj.relativity));
            }
            proj.vel = vec(rel).add(vec(dir).mul(physics::movevelocity(&proj)));
        }
        proj.spawntime = lastmillis;
        proj.hit = NULL;
        proj.hitflags = HITFLAG_NONE;
        proj.movement = 1;
        if(proj.projtype == PRJ_SHOT && proj.owner && !weaptype[proj.weap].traced)
        {
            vec eyedir = vec(proj.o).sub(proj.owner->o);
            float eyedist = eyedir.magnitude();
            if(eyedist >= 1e-3f)
            {
                eyedir.div(eyedist);
                float blocked = pltracecollide(&proj, proj.owner->o, eyedir, eyedist);
                if(blocked >= 0) proj.o = vec(eyedir).mul(blocked+proj.radius).add(proj.owner->o);
            }
        }
        proj.resetinterp();
    }

    void create(vec &from, vec &to, bool local, gameent *d, int type, int lifetime, int lifemillis, int waittime, int speed, int id, int weap, int flags, float scale)
    {
        if(!d) return;

        projent &proj = *new projent;
        proj.o = proj.from = from;
        proj.to = to;
        proj.local = local;
        proj.projtype = type;
        proj.addtime = lastmillis;
        proj.lifetime = lifetime ? lifetime : lifetime;
        proj.lifemillis = lifemillis ? lifemillis : proj.lifetime;
        proj.waittime = waittime;
        proj.maxspeed = speed;
        proj.id = id ? id : lastmillis;
        proj.weap = weap;
        proj.flags = flags;
        proj.scale = scale;
        proj.movement = 0;
        proj.owner = d;
        if(!proj.waittime) init(proj, false);
        projs.add(&proj);
    }

    void drop(gameent *d, int g, int n, bool local)
    {
        if(g >= WEAP_OFFSET && isweap(g))
        {
            vec from(d->o), to(d->muzzlepos(g));
            if(entities::ents.inrange(n))
            {
                if(!m_noitems(game::gamemode, game::mutators) && itemdropping && !(entities::ents[n]->attrs[1]&WEAP_F_FORCED))
                    create(from, to, local, d, PRJ_ENT, w_spawn(g), w_spawn(g), 1, 1, n);
                d->ammo[g] = -1;
                d->setweapstate(g, WEAP_S_SWITCH, WEAPSWITCHDELAY, lastmillis);
            }
            else if(g == WEAP_GRENADE) create(from, to, local, d, PRJ_SHOT, 1, WEAP2(g, time, false), 1, 1, -1, g);
            else
            {
                d->ammo[g] = -1;
                d->setweapstate(g, WEAP_S_SWITCH, WEAPSWITCHDELAY, lastmillis);
            }
            d->entid[g] = -1;
        }
    }

    void shootv(int weap, int flags, int offset, float scale, vec &from, vector<vec> &locs, gameent *d, bool local)
    {
        int delay = WEAP2(weap, pdelay, flags&HIT_ALT), millis = delay, adelay = WEAP2(weap, adelay, flags&HIT_ALT),
            life = WEAP2(weap, time, flags&HIT_ALT), speed = WEAP2(weap, speed, flags&HIT_ALT);

        if(WEAP2(weap, power, flags&HIT_ALT)) switch(WEAP2(weap, cooked, flags&HIT_ALT))
        {
            case 1: break;
            case 2: case 3: life = int(ceilf(life*(1.f-scale))); if(!(WEAP2(weap, cooked, flags&HIT_ALT)%2)) scale = 1; break; // shorter
            case 4: case 5: life = int(ceilf(life*scale)); if(!(WEAP2(weap, cooked, flags&HIT_ALT)%2)) scale = 1; break; // longer
            default: scale = 1; break;
        }

        if(weaptype[weap].sound >= 0)
        {
            if(weap == WEAP_FLAMER && !(flags&HIT_ALT))
            {
                int ends = lastmillis+(WEAP2(weap, adelay, flags&HIT_ALT)*2);
                if(issound(d->wschan)) sounds[d->wschan].ends = ends;
                else playsound(weaptype[weap].sound+(flags&HIT_ALT ? 1 : 0), d->o, d, SND_LOOP, -1, -1, -1, &d->wschan, ends);
            }
            else if(!WEAP2(weap, time, flags&HIT_ALT) || life)
                playsound(weaptype[weap].sound+(flags&HIT_ALT ? 1 : 0), d->o, d, 0, -1, -1, -1, &d->wschan);
        }
        float muz = muzzleblend; if(d == game::focus) muz *= 0.5f;
        const struct weapfxs
        {
            int smoke, parttype, colour;
            float partsize, flaresize, flarelen;
        } weapfx[WEAP_MAX] = {
            { 0, 0, 0, 0, 0, 0 },
            { 200, PART_MUZZLE_FLASH, 0xFFCC22, 1.5f, 2, 4 },
            { 0, 0, 0, 0, 0, 0 },
            { 350, PART_MUZZLE_FLASH, 0xFFAA00, 3, 5, 16 },
            { 50, PART_MUZZLE_FLASH, 0xFF8800, 2.5f, 4, 12 },
            { 150, PART_MUZZLE_FLASH, -1, 1.5f, 0, 0 },
            { 150, PART_PLASMA, 0x226688, 1.5f, 0, 0 },
            { 150, PART_PLASMA, 0x6611FF, 1.5f, 3, 9 },
            { 0, 0, 0, 0, 0, 0 },
            { 500, PART_MUZZLE_FLASH, -1, 2, 0, 0 },
        };
        if(weapfx[weap].colour && WEAP2(weap, adelay, flags&HIT_ALT) >= 5)
        {
            int colour = weapfx[weap].colour > 0 ? weapfx[weap].colour : firecols[rnd(FIRECOLOURS)];
            if(weapfx[weap].smoke) part_create(PART_SMOKE_LERP, weapfx[weap].smoke, from, 0xAAAAAA, 1, 0.25f, -20);
            if(muzzlechk(muzzleflash, d) && weapfx[weap].partsize > 0)
                part_create(weapfx[weap].parttype, WEAP2(weap, adelay, flags&HIT_ALT)/3, from, colour, weapfx[weap].partsize, muz, 0, 0, d);
            if(muzzlechk(muzzleflare, d) && weapfx[weap].flaresize > 0)
            {
                vec to; vecfromyawpitch(d->yaw, d->pitch, 1, 0, to);
                part_flare(from, to.mul(weapfx[weap].flarelen).add(from), WEAP2(weap, adelay, flags&HIT_ALT)/2, PART_MUZZLE_FLARE, colour, weapfx[weap].flaresize, muz, 0, 0, d);
            }
            adddynlight(from, 32, vec(colour>>16, (colour>>8)&0xFF, colour&0xFF).div(512.f), WEAP2(weap, adelay, flags&HIT_ALT)/4, 0, DL_FLASH);
        }

        loopv(locs)
        {
            int pdelay = delay*i, plife = life;
            if(weaptype[weap].traced && plife > adelay-pdelay) plife = max(adelay-pdelay, 40);
            create(from, locs[i], local, d, PRJ_SHOT, max(plife, 1), WEAP2(weap, time, flags&HIT_ALT), millis+pdelay, speed, 0, weap, flags, scale);
        }
        if(ejectfade && weaptype[weap].eject) loopi(clamp(offset, 1, WEAP2(weap, sub, flags&HIT_ALT)))
            create(from, from, local, d, PRJ_EJECT, rnd(ejectfade)+ejectfade, 0, millis, rnd(weaptype[weap].espeed)+weaptype[weap].espeed, 0, weap, flags);

        if(d->aitype >= AI_BOT && (!WEAP2(weap, fullauto, flags&HIT_ALT) || adelay >= PHYSMILLIS))
            adelay += int(adelay*(1.f/d->skill));
        d->setweapstate(weap, flags&HIT_ALT ? WEAP_S_SECONDARY : WEAP_S_PRIMARY, adelay, lastmillis);
        d->ammo[weap] = max(d->ammo[weap]-offset, 0);
        d->weapshot[weap] = offset;
    }

    void iter(projent &proj)
    {
        proj.lifespan = clamp((proj.lifemillis-proj.lifetime)/float(max(proj.lifemillis, 1)), 0.f, 1.f);
        if(proj.projtype == PRJ_SHOT)
        {
            if(proj.owner)
            {
                if(weaptype[proj.weap].traced)
                {
                    proj.o = proj.from = proj.owner->headpos(proj.owner->height/2);
                    proj.to = proj.owner->muzzlepos(proj.weap);
                }
                else proj.from = proj.weap == WEAP_MELEE && proj.flags&HIT_ALT ? proj.owner->feetpos(1) : proj.owner->muzzlepos(proj.weap);
            }
            if(WEAP2(proj.weap, radial, proj.flags&HIT_ALT))
            {
                if(WEAP2(proj.weap, taper, proj.flags&HIT_ALT) > 0)
                {
                    if(proj.lifespan > 0.125f)
                    {
                        if(!proj.stuck) proj.lifesize = clamp(1.125f-proj.lifespan, 1.f-WEAP2(proj.weap, taper, proj.flags&HIT_ALT), 1.f);
                    }
                    else proj.lifesize = clamp(0.0625f+(proj.lifespan*10), 0.0625f, 1.f);
                }
                else proj.lifesize = proj.lifespan;
            }
            else if(WEAP2(proj.weap, taper, proj.flags&HIT_ALT) > 0) proj.lifesize = clamp(1.f-proj.lifespan, 1.f-WEAP2(proj.weap, taper, proj.flags&HIT_ALT), 1.f);
            else proj.lifesize = 1;
        }
        updatebb(proj);
    }

    void effect(projent &proj)
    {
        if(proj.projtype == PRJ_SHOT)
        {
            if(weaptype[proj.weap].fsound >= 0)
            {
                int vol = 255;
                switch(weaptype[proj.weap].fsound)
                {
                    case S_BEEP: case S_BURN: case S_BURNING: vol = 55+int(200*proj.lifespan*proj.scale); break;
                    case S_WHIZZ: case S_WHIRR: vol = 55+int(200*(1.f-proj.lifespan)*proj.scale); break;
                    default: break;
                }
                if(issound(proj.schan)) sounds[proj.schan].vol = vol;
                else playsound(weaptype[proj.weap].fsound, proj.o, &proj, SND_LOOP, vol, -1, -1, &proj.schan);
            }
            switch(proj.weap)
            {
                case WEAP_PISTOL:
                {
                    if(proj.movement > 0.f)
                    {
                        bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= 200;
                        float dist = proj.o.dist(proj.from), size = clamp(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT)*(1.f-proj.lifespan), 1.f, iter ? min(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT), proj.movement) : dist);
                        vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
                        proj.to = vec(proj.o).sub(vec(dir).mul(size));
                        int col = ((int(220*max(1.f-proj.lifespan,0.3f))<<16))|((int(160*max(1.f-proj.lifespan,0.2f)))<<8);
                        part_flare(proj.to, proj.o, 1, PART_FLARE, col, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT));
                        part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, col, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.25f);
                        part_create(PART_PLASMA, 1, proj.o, col, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT));
                    }
                    break;
                }
                case WEAP_FLAMER:
                {
                    if(proj.movement > 0.f)
                    {
                        bool effect = false;
                        float size = WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*1.25f*proj.lifespan*proj.scale, blend = clamp(1.25f-proj.lifespan, 0.25f, 0.85f)*(0.65f+(rnd(35)/100.f))*proj.scale;
                        if(firetrail && lastmillis-proj.lasteffect >= firedelay) { effect = true; proj.lasteffect = lastmillis; }
                        int len = effect ? max(int(firelength*max(1.f-proj.lifespan, 0.1f)), 1) : 1;
                        if(firehint && effect && notrayspam(proj.weap, proj.flags&HIT_ALT, 1)) part_create(PART_HINT_SOFT, 1, proj.o, 0x120226, size*1.5f, blend*(proj.flags&HIT_ALT ? 0.75f : 1.f));
                        part_create(PART_FIREBALL_SOFT, len, proj.o, firecols[rnd(FIRECOLOURS)], size, blend, -15);
                    }
                    break;
                }
                case WEAP_GRENADE:
                {
                    int col = ((int(254*max(1.f-proj.lifespan,0.5f))<<16)+1)|((int(98*max(1.f-proj.lifespan,0.f))+1)<<8), interval = lastmillis%1000;
                    float fluc = 1.f+(interval ? (interval <= 500 ? interval/500.f : (1000-interval)/500.f) : 0.f);
                    part_create(PART_PLASMA_SOFT, 1, proj.o, col, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*fluc);
                    bool moving = proj.movement > 0.f;
                    if(lastmillis-proj.lasteffect >= (moving ? 50 : 100))
                    {
                        part_create(PART_SMOKE_LERP, 250, proj.o, 0x222222, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*(moving ? 0.5f : 1.f), 0.5f, -20);
                        proj.lasteffect = lastmillis;
                    }
                    break;
                }
                case WEAP_ROCKET:
                {
                    int col = ((int(254*max(1.f-proj.lifespan,0.5f))<<16)+1)|((int(98*max(1.f-proj.lifespan,0.f))+1)<<8), interval = lastmillis%1000;
                    float fluc = 1.f+(interval ? (interval <= 500 ? interval/500.f : (1000-interval)/500.f) : 0.f);
                    part_create(PART_PLASMA_SOFT, 1, proj.o, col, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*fluc);
                    bool moving = proj.movement > 0.f;
                    if(lastmillis-proj.lasteffect >= (moving ? 50 : 100))
                    {
                        part_create(PART_SMOKE_LERP, 150, proj.o, 0x666666, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*(moving ? 0.5f : 1.f), 0.5f, -10);
                        proj.lasteffect = lastmillis;
                    }
                    break;
                }
                case WEAP_SHOTGUN:
                {
                    if(proj.movement > 0.f)
                    {
                        bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= 200;
                        float dist = proj.o.dist(proj.from), size = clamp(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT)*(1.f-proj.lifespan), 1.f, iter ? min(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT), proj.movement) : dist);
                        vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
                        proj.to = vec(proj.o).sub(vec(dir).mul(size));
                        int col = ((int(224*max(1.f-proj.lifespan,0.3f))<<16)+1)|((int(144*max(1.f-proj.lifespan,0.15f))+1)<<8);
                        part_flare(proj.to, proj.o, 1, PART_FLARE, col, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT), clamp(1.25f-proj.lifespan, 0.5f, 1.f));
                        part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, col, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.25f, clamp(1.25f-proj.lifespan, 0.5f, 1.f));
                    }
                    break;
                }
                case WEAP_SMG:
                {
                    if(proj.movement > 0.f)
                    {
                        bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= 200;
                        float dist = proj.o.dist(proj.from), size = clamp(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT)*(1.f-proj.lifespan), 1.f, iter ? min(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT), proj.movement) : dist);
                        vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
                        proj.to = vec(proj.o).sub(vec(dir).mul(size));
                        int col = ((int(224*max(1.f-proj.lifespan,0.3f))<<16))|((int(80*max(1.f-proj.lifespan,0.1f)))<<8);
                        part_flare(proj.to, proj.o, 1, PART_FLARE, col, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT), clamp(1.25f-proj.lifespan, 0.5f, 1.f));
                        part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, col, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.125f, clamp(1.25f-proj.lifespan, 0.5f, 1.f));
                    }
                    break;
                }
                case WEAP_PLASMA:
                {
                    if(proj.flags&HIT_ALT) part_fireball(proj.o, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.5f*proj.lifesize*proj.scale, PART_EXPLOSION, 1, 0x225599, 1.f, 0.5f);
                    part_create(PART_PLASMA_SOFT, 1, proj.o, proj.flags&HIT_ALT ? 0x4488EE : 0x55AAEE, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*proj.lifesize*proj.scale, (proj.flags&HIT_ALT ? 0.75f : clamp(1.5f-proj.lifespan, 0.5f, 1.f))*proj.scale);
                    part_create(PART_ELECTRIC_SOFT, 1, proj.o, proj.flags&HIT_ALT ? 0x4488EE : 0x55AAEE, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.45f*proj.lifesize*proj.scale, (proj.flags&HIT_ALT ? 1.f : clamp(1.5f-proj.lifespan, 0.5f, 1.f))*proj.scale);
                    break;
                }
                case WEAP_RIFLE:
                {
                    float dist = proj.o.dist(proj.from), size = clamp(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT), 1.f, min(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT), proj.movement));
                    vec dir = dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
                    proj.to = vec(proj.o).sub(vec(dir).mul(size));
                    part_flare(proj.to, proj.o, 1, PART_FLARE, 0x6611FF, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT));
                    part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, 0x6611FF, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.25f);
                    break;
                }
                default: break;
            }
        }
        else if(proj.projtype == PRJ_GIBS || proj.projtype == PRJ_DEBRIS)
        {
            if(proj.projtype == PRJ_GIBS && !kidmode && game::bloodscale > 0 && game::debrisscale > 0)
            {
                if(proj.movement > 1 && lastmillis-proj.lasteffect >= 1000 && proj.lifetime >= min(proj.lifemillis, proj.fadetime))
                {
                    float size = ((rnd(game::bloodsize)+1)/10.f)*proj.radius;
                    part_create(PART_BLOOD, game::bloodfade, proj.o, 0x88FFFF, size, 1, 100, DECAL_BLOOD);
                    proj.lasteffect = lastmillis;
                }
            }
            else if(!proj.limited)
            {
                bool effect = false;
                float radius = (proj.radius+0.5f)*(clamp(1.f-proj.lifespan, 0.1f, 1.f)+0.25f), blend = clamp(1.25f-proj.lifespan, 0.25f, 1.f)*(0.75f+(rnd(25)/100.f)); // gets smaller as it gets older
                if(firetrail && lastmillis-proj.lasteffect >= firedelay) { effect = true; proj.lasteffect = lastmillis; }
                int len = effect ? max(int(firelength*max(1.f-proj.lifespan, 0.1f)), 1) : 1;
                part_create(PART_FIREBALL_SOFT, len, proj.o, firecols[rnd(FIRECOLOURS)], radius, blend, -10);
            }
        }
        else switch(proj.projtype)
        {
            case PRJ_EJECT:
            {
                if(isweap(proj.weap) && ejecthint)
                    part_create(PART_HINT, 1, proj.o, weaptype[proj.weap].colour, max(proj.xradius, proj.yradius)*1.75f, clamp(1.f-proj.lifespan, 0.1f, 1.f)*0.2f);
                bool moving = proj.movement > 0.f;
                if(moving && lastmillis-proj.lasteffect >= 100)
                {
                    part_create(PART_SMOKE, 150, proj.o, 0x222222, max(proj.xradius, proj.yradius)*1.75f, clamp(1.f-proj.lifespan, 0.1f, 1.f)*0.5f, -3);
                    proj.lasteffect = lastmillis;
                }
            }
            default: break;
        }
    }

    void quake(const vec &o, int weap, int flags, float scale)
    {
        int q = int(WEAP2(weap, damage, flags&HIT_ALT)/float(WEAP2(weap, rays, flags&HIT_ALT))), r = int(WEAP2(weap, radius, flags&HIT_ALT)*WEAP(weap, pusharea)*scale);
        gameent *d;
        loopi(game::numdynents()) if((d = (gameent *)game::iterdynents(i)))
            d->quake = clamp(d->quake+max(int(q*max(1.f-d->o.dist(o)/r, 1e-3f)), 1), 0, 1000);
    }

    void destroy(projent &proj)
    {
        proj.lifespan = clamp((proj.lifemillis-proj.lifetime)/float(max(proj.lifemillis, 1)), 0.f, 1.f);
        switch(proj.projtype)
        {
            case PRJ_SHOT:
            {
                if(proj.owner)
                {
                    if(weaptype[proj.weap].traced)
                    {
                        proj.o = proj.from = proj.owner->headpos(proj.owner->height/2);
                        proj.to = proj.owner->muzzlepos(proj.weap);
                    }
                    else proj.from = proj.weap == WEAP_MELEE && proj.flags&HIT_ALT ? proj.owner->feetpos(1) : proj.owner->muzzlepos(proj.weap);
                }
                int vol = 255;
                switch(proj.weap)
                {
                    case WEAP_PISTOL:
                    {
                        vol = int(255*(1.f-proj.lifespan)*proj.scale);
                        part_create(PART_SMOKE_LERP, 200, proj.o, 0x999999, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT), 0.25f, -20);
                        if(WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale) > 0)
                        {
                            quake(proj.o, proj.weap, proj.flags, proj.scale);
                            part_fireball(proj.o, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.5f), PART_EXPLOSION, 100, 0x884411, 1.f, 0.5f);
                            if(notrayspam(proj.weap, proj.flags&HIT_ALT, 0))
                            {
                                adddecal(DECAL_SCORCH_SHORT, proj.o, proj.norm, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.5f));
                                adddynlight(proj.o, 1.1f*WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.75f), vec(0.5f, 0.25f, 0.05f), proj.flags&HIT_ALT ? 300 : 100, 10);
                            }
                        }
                        else if(notrayspam(proj.weap, proj.flags&HIT_ALT, 5)) adddecal(DECAL_BULLET, proj.o, proj.norm, 2.f);
                        break;
                    }
                    case WEAP_FLAMER: case WEAP_GRENADE: case WEAP_ROCKET:
                    { // both basically explosions
                        if(!proj.limited)
                        {
                            if(proj.weap == WEAP_FLAMER)
                                part_create(PART_SMOKE_LERP_SOFT, firelength*3, proj.o, 0x666666, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.75f), 0.25f+(rnd(50)/100.f), -15);
                            else
                            {
                                int deviation = max(int(WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.25f)), 1);
                                part_create(PART_PLASMA_SOFT, firelength*(proj.weap == WEAP_GRENADE ? 2 : 3), proj.o, 0xDD4400, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.5f)); // corona
                                quake(proj.o, proj.weap, proj.flags, proj.scale);
                                part_fireball(proj.o, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.75f), PART_EXPLOSION, proj.weap == WEAP_GRENADE ? 750 : 1500, 0xAA4400, 1.f, 0.5f);
                                if(notrayspam(proj.weap, proj.flags&HIT_ALT, 1))
                                {
                                    loopi(proj.weap == WEAP_GRENADE ? 3 : 6)
                                    {
                                        vec to(proj.o); loopk(3) to.v[k] += rnd(deviation*2)-deviation;
                                        part_create(PART_FIREBALL_SOFT, firelength*(proj.weap == WEAP_GRENADE ? 2 : 3), to, firecols[rnd(FIRECOLOURS)], WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.5f), 0.25f+(rnd(50)/100.f), -5);
                                    }
                                    part_create(PART_SMOKE_LERP_SOFT, firelength*(proj.weap == WEAP_GRENADE ? 3 : 4), proj.o, 0x333333, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.75f), 0.5f, -15);
                                    int debris = rnd(proj.weap == WEAP_GRENADE ? 5 : 10)+5, amt = int((rnd(debris)+debris+1)*game::debrisscale);
                                    loopi(amt) create(proj.o, vec(proj.o).add(proj.vel), true, proj.owner, PRJ_DEBRIS, rnd(game::debrisfade)+game::debrisfade, 0, rnd(501), rnd(101)+50);
                                    adddecal(DECAL_ENERGY, proj.o, proj.norm, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.75f), bvec(196, 24, 0));
                                }
                            }
                            if(notrayspam(proj.weap, proj.flags&HIT_ALT, 1))
                            {
                                adddecal(proj.weap == WEAP_FLAMER ? DECAL_SCORCH_SHORT : DECAL_SCORCH, proj.o, proj.norm, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.5f));
                                adddynlight(proj.o, 1.f*WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.75f), vec(1.1f, 0.22f, 0.02f), proj.weap == WEAP_FLAMER ? 250 : 1000, 10);
                            }
                        }
                        else vol = 0;
                        break;
                    }
                    case WEAP_SHOTGUN: case WEAP_SMG:
                    {
                        vol = int(255*(1.f-proj.lifespan));
                        part_splash(PART_SPARK, proj.weap == WEAP_SHOTGUN ? 10 : 20, 500, proj.o, proj.weap == WEAP_SMG ? 0xFF8822 : 0xFFAA22, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.25f, 1, 20, 16);
                        if(WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale) > 0)
                        {
                            quake(proj.o, proj.weap, proj.flags, proj.scale);
                            part_fireball(proj.o, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.5f), PART_EXPLOSION, 100, proj.weap == WEAP_SMG ? 0xAA4400 : 0xAA7711, 1.f, 0.5f);
                            if(notrayspam(proj.weap, proj.flags&HIT_ALT, 0))
                            {
                                adddecal(DECAL_SCORCH_SHORT, proj.o, proj.norm, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.5f));
                                adddynlight(proj.o, 1.1f*WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.75f), vec(0.7f, proj.weap == WEAP_SMG ? 0.2f : 0.4f, 0.f), proj.flags&HIT_ALT ? 300 : 100, 10);
                            }
                        }
                        break;
                    }
                    case WEAP_PLASMA:
                    {
                        if(!proj.limited)
                        {
                            part_create(PART_PLASMA_SOFT, proj.flags&HIT_ALT ? 500 : 150, proj.o, 0x55AAEE, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*proj.lifesize*0.75f));
                            part_create(PART_ELECTRIC_SOFT, proj.flags&HIT_ALT ? 250 : 75, proj.o, 0x55AAEE, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*proj.lifesize*0.375f));
                            part_create(PART_SMOKE, proj.flags&HIT_ALT ? 500 : 250, proj.o, 0x8896A4, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*proj.lifesize*0.35f), 0.35f, -30);
                            quake(proj.o, proj.weap, proj.flags, proj.scale);
                            if(proj.flags&HIT_ALT) part_fireball(proj.o, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*proj.lifesize*0.5f), PART_EXPLOSION, 150, 0x225599, 1.f, 0.5f);
                            if(notrayspam(proj.weap, proj.flags&HIT_ALT, 1))
                            {
                                adddecal(DECAL_ENERGY, proj.o, proj.norm, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*proj.lifesize*0.75f), bvec(98, 196, 244));
                                adddynlight(proj.o, 1.1f*WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*proj.lifesize), vec(0.1f, 0.4f, 0.6f), proj.flags&HIT_ALT ? 300 : 100, 10);
                            }
                        }
                        else vol = 0;
                        break;
                    }
                    case WEAP_RIFLE:
                    {
                        float dist = proj.o.dist(proj.from), size = clamp(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT), 1.f, min(WEAP2(proj.weap, partlen, proj.flags&HIT_ALT), proj.movement));
                        vec dir = dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
                        proj.to = vec(proj.o).sub(vec(dir).mul(size));
                        part_flare(proj.to, proj.o, proj.flags&HIT_ALT ? 500 : 250, PART_FLARE, 0x6611FF, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT));
                        part_flare(proj.to, proj.o, proj.flags&HIT_ALT ? 500 : 250, PART_FLARE_LERP, 0x6611FF, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.25f);
                        part_splash(PART_SPARK, proj.flags&HIT_ALT ? 25 : 50, proj.flags&HIT_ALT ? 500 : 750, proj.o, 0x6611FF, WEAP2(proj.weap, partsize, proj.flags&HIT_ALT)*0.125f, 1, 20, 0, 24);
                        if(WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale) > 0)
                        {
                            part_create(PART_PLASMA_SOFT, 500, proj.o, 0x4408AA, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.5f)); // corona
                            quake(proj.o, proj.weap, proj.flags, proj.scale);
                            part_fireball(proj.o, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.5f), PART_EXPLOSION, 500, 0x2211FF, 1.f, 0.5f);
                            if(notrayspam(proj.weap, proj.flags&HIT_ALT, 1))
                            {
                                adddecal(DECAL_SCORCH, proj.o, proj.norm, 8.f);
                                adddecal(DECAL_ENERGY, proj.o, proj.norm, 4.f, bvec(98, 16, 254));
                                adddynlight(proj.o, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale*0.75f), vec(0.4f, 0.05f, 1.f), 500, 10);
                            }
                        }
                        else if(notrayspam(proj.weap, proj.flags&HIT_ALT, 1))
                        {
                            adddecal(DECAL_SCORCH, proj.o, proj.norm, 5.f);
                            adddecal(DECAL_ENERGY, proj.o, proj.norm, 2.f, bvec(98, 16, 254));
                            adddynlight(proj.o, 16, vec(0.4f, 0.05f, 1.f), 250, 10);
                        }
                        break;
                    }
                    default: break;
                }
                if(vol && weaptype[proj.weap].esound >= 0) playsound(weaptype[proj.weap].esound, proj.o, NULL, 0, vol);
                if(proj.local && proj.owner)
                    client::addmsg(N_DESTROY, "ri7", proj.owner->clientnum, lastmillis-game::maptime, proj.weap, proj.flags, proj.id >= 0 ? proj.id-game::maptime : proj.id, 0, 0);
                break;
            }
            case PRJ_ENT:
            {
                if(proj.beenused <= 1 && proj.local && proj.owner)
                    client::addmsg(N_DESTROY, "ri7", proj.owner->clientnum, lastmillis-game::maptime, -1, 0, proj.id, 0, 0);
                break;
            }
            default: break;
        }
    }

    int checkmaterial(projent &proj)
    {
        int mat = lookupmaterial(vec(proj.o.x, proj.o.y, proj.o.z + (proj.aboveeye - proj.height)/2));
        if(proj.extinguish >= 2 && (int(mat&MATF_VOLUME) == MAT_LAVA || int(mat&MATF_FLAGS) == MAT_DEATH || proj.o.z < 0)) return 2;
        else if(proj.extinguish >= 1 && (isliquid(mat&MATF_VOLUME))) return 1;
        return 0;
    }

    int bounce(projent &proj, const vec &dir)
    {
        int check = checkmaterial(proj);
        if(((!collide(&proj, dir, 0.f, proj.projcollide&COLLIDE_PLAYER) || inside) && (hitplayer ? proj.projcollide&COLLIDE_PLAYER : proj.projcollide&COLLIDE_GEOM)) || check)
        {
            if(hitplayer)
            {
                if(!hiteffect(proj, hitplayer, hitflags, vec(hitplayer->o).sub(proj.o).normalize())) return 1;
            }
            else if(check)
            {
                if(check == 1 && proj.extinguish >= 2) proj.limited = true;
                proj.norm = dir;
            }
            else
            {
                if(proj.projcollide&COLLIDE_STICK)
                {
                    proj.o.sub(vec(dir).mul(proj.radius*0.125f));
                    proj.stuck = true;
                    return 1;
                }
                proj.norm = wall;
            }
            bounceeffect(proj);
            if(check && proj.projtype != PRJ_DEBRIS) return 0;
            else if(proj.projcollide&(hitplayer ? BOUNCE_PLAYER : BOUNCE_GEOM))
            {
                reflect(proj, proj.norm);
                proj.movement = 0;
                proj.lastbounce = lastmillis;
                return 2; // bounce
            }
            else if(proj.projcollide&(hitplayer ? IMPACT_PLAYER : IMPACT_GEOM))
                return 0; // die on impact
        }
        return 1; // live!
    }

    int trace(projent &proj, const vec &dir)
    {
        vec to(proj.o), ray = dir;
        to.add(dir);
        float maxdist = ray.magnitude();
        if(maxdist <= 0) return 1; // not moving anywhere, so assume still alive since it was already alive
        ray.mul(1/maxdist);
        float dist = tracecollide(&proj, proj.o, ray, maxdist, RAY_CLIPMAT | RAY_ALPHAPOLY, proj.projcollide&COLLIDE_PLAYER);
        proj.o.add(vec(ray).mul(dist >= 0 ? dist : maxdist));
        int check = checkmaterial(proj);
        if((dist >= 0 && (hitplayer ? proj.projcollide&COLLIDE_PLAYER : proj.projcollide&COLLIDE_GEOM)) || check)
        {
            if(hitplayer)
            {
                if(!hiteffect(proj, hitplayer, hitflags, vec(hitplayer->o).sub(proj.o).normalize())) return 1;
            }
            else if(check)
            {
                if(check == 1 && proj.extinguish >= 2) proj.limited = true;
                proj.norm = dir;
            }
            else
            {
                if(proj.projcollide&COLLIDE_STICK)
                {
                    proj.o.sub(vec(dir).mul(proj.radius*0.125f));
                    proj.stuck = true;
                    return 1;
                }
                proj.norm = hitsurface;
            }
            bounceeffect(proj);
            if(check && proj.projtype != PRJ_DEBRIS) return 0;
            else if(proj.projcollide&(hitplayer ? BOUNCE_PLAYER : BOUNCE_GEOM))
            {
                reflect(proj, proj.norm);
                proj.o.add(vec(proj.norm).mul(0.1f)); // offset from surface slightly to avoid initial collision
                proj.movement = 0;
                proj.lastbounce = lastmillis;
                return 2; // bounce
            }
            else if(proj.projcollide&(hitplayer ? IMPACT_PLAYER : IMPACT_GEOM))
                return 0; // die on impact
        }
        return 1; // live!
    }

    void checkescaped(projent &proj, const vec &pos, const vec &dir)
    {
        if(!(proj.projcollide&COLLIDE_OWNER) || (proj.spawntime && lastmillis-proj.spawntime >= PHYSMILLIS)) proj.escaped = true;
        else if(proj.projcollide&COLLIDE_TRACE)
        {
            vec to = vec(pos).add(dir);
            float x1 = floor(min(pos.x, to.x)), y1 = floor(min(pos.y, to.y)),
                  x2 = ceil(max(pos.x, to.x)), y2 = ceil(max(pos.y, to.y)),
                  maxdist = dir.magnitude(), dist = 1e16f;
            if(physics::xtracecollide(&proj, pos, to, x1, x2, y1, y2, maxdist, dist, proj.owner) || dist > maxdist) proj.escaped = true;
        }
        else if(physics::xcollide(&proj, dir, proj.owner)) proj.escaped = true;
    }

    bool move(projent &proj, int qtime)
    {
        float secs = float(qtime)/1000.f;
        if(proj.projtype == PRJ_SHOT && proj.escaped && proj.owner)
        {
            if(returningfire && lastmillis-proj.spawntime >= min(WEAP2(proj.weap, time, proj.flags&HIT_ALT)/3, 100))
            {
                if(!proj.stuck) proj.stuck = false;
                vec targ = vec(proj.owner->feetpos(proj.owner->height/2)).sub(proj.o).normalize();
                if(!targ.iszero()) proj.vel = targ.mul(max(proj.vel.magnitude(), physics::movevelocity(&proj)));
            }
            else if(proj.owner->state == CS_ALIVE && WEAP2(proj.weap, guided, proj.flags&HIT_ALT) && lastmillis-proj.spawntime >= WEAP2(proj.weap, gdelay, proj.flags&HIT_ALT))
            {
                bool failed = false;
                vec targ, dir = proj.vel.normalize();
                switch(WEAP2(proj.weap, guided, proj.flags&HIT_ALT))
                {
                    case 5: case 4: case 3: case 2:
                    {
                        switch(WEAP2(proj.weap, guided, proj.flags&HIT_ALT))
                        {
                            case 4: case 5:
                            {
                                if(WEAP2(proj.weap, guided, proj.flags&HIT_ALT)%2 && proj.target) break;
                                float yaw, pitch;
                                vectoyawpitch(dir, yaw, pitch);
                                findorientation(proj.o, yaw, pitch, targ);
                                physent *t = game::intersectclosest(proj.o, targ, proj.owner);
                                if(t) proj.target = t;
                                break;
                            }
                            case 2: case 3: default:
                            {
                                if(WEAP2(proj.weap, guided, proj.flags&HIT_ALT)%2 && proj.target) break;
                                findorientation(proj.owner->o, proj.owner->yaw, proj.owner->pitch, targ);
                                physent *t = game::intersectclosest(proj.owner->o, targ, proj.owner);
                                if(t) proj.target = t;
                                break;
                            }
                        }
                        if(proj.target) targ = proj.target->headpos();
                        else failed = true;
                    }
                    case 1: default: findorientation(proj.owner->o, proj.owner->yaw, proj.owner->pitch, targ); break;
                }
                if(!failed)
                {
                    targ.sub(proj.o).normalize();
                    if(!targ.iszero())
                    {
                        float amt = clamp(WEAP2(proj.weap, delta, proj.flags&HIT_ALT)*secs, 1e-8f, 1.f);
                        dir.mul(1.f-amt).add(targ.mul(amt)).normalize();
                        if(!dir.iszero()) proj.vel = dir.mul(max(proj.vel.magnitude(), physics::movevelocity(&proj)));
                    }
                }
            }
        }
        if(proj.weight != 0.f) proj.vel.z -= physics::gravityforce(&proj)*secs;

        vec dir(proj.vel), pos(proj.o);
        int mat = lookupmaterial(vec(proj.o.x, proj.o.y, proj.o.z + (proj.aboveeye - proj.height)/2));
        if(isliquid(mat&MATF_VOLUME) && proj.waterfric > 0) dir.div(proj.waterfric);
        dir.mul(secs);

        if(!proj.escaped && proj.owner) checkescaped(proj, pos, dir);

        bool blocked = false;
        if(proj.projcollide&COLLIDE_TRACE)
        {
            switch(trace(proj, dir))
            {
                case 2: blocked = true; break;
                case 1: break;
                case 0: return false;
            }
        }
        else
        {
            if(proj.projtype == PRJ_SHOT)
            {
                float stepdist = dir.magnitude();
                vec ray(dir);
                ray.mul(1/stepdist);
                float barrier = raycube(proj.o, ray, stepdist, RAY_CLIPMAT|RAY_POLY);
                if(barrier < stepdist)
                {
                    proj.o.add(ray.mul(barrier-0.15f));
                    switch(bounce(proj, ray))
                    {
                        case 2: proj.o = pos; blocked = true; break;
                        case 1: proj.o = pos; break;
                        case 0: return false;
                    }
                }
            }
            if(!blocked)
            {
                proj.o.add(dir);
                switch(bounce(proj, dir))
                {
                    case 2: proj.o = pos; if(proj.projtype == PRJ_SHOT) blocked = true; break;
                    case 1: default: break;
                    case 0: proj.o = pos; if(proj.projtype == PRJ_SHOT) { dir.rescale(max(dir.magnitude()-0.15f, 0.0f)); proj.o.add(dir); } return false;
                }
            }
        }

        float dist = proj.o.dist(pos), diff = dist/float(4*RAD);
        if(!blocked) proj.movement += dist;
        switch(proj.projtype)
        {
            case PRJ_SHOT:
                if(proj.weap == WEAP_ROCKET)
                {
                    vectoyawpitch(vec(proj.vel).normalize(), proj.yaw, proj.pitch);
                    break;
                }
            case PRJ_DEBRIS: case PRJ_GIBS: case PRJ_EJECT:
            {
                if(!proj.lastbounce || proj.movement >= 1)
                {
                    vec axis(sinf(proj.yaw*RAD), -cosf(proj.yaw*RAD), 0);
                    if(proj.vel.dot2(axis) >= 0) { proj.pitch -= diff; if(proj.pitch < -180) proj.pitch = 180 - fmod(180 - proj.pitch, 360); }
                    else { proj.pitch += diff; if(proj.pitch > 180) proj.pitch = fmod(proj.pitch + 180, 360) - 180; }
                    break;
                }
                if(proj.projtype == PRJ_GIBS) break;
            }
            case PRJ_ENT:
            {
                if(proj.pitch != 0)
                {
                    if(proj.pitch < 0) { proj.pitch += max(diff, !proj.lastbounce || proj.movement >= 1 ? 1.f : 5.f); if(proj.pitch > 0) proj.pitch = 0; }
                    else if(proj.pitch > 0) { proj.pitch -= max(diff, !proj.lastbounce || proj.movement >= 1 ? 1.f : 5.f); if(proj.pitch < 0) proj.pitch = 0; }
                }
                break;
            }
            default: break;
        }
        return true;
    }

    bool moveframe(projent &proj)
    {
        if(((proj.lifetime -= physics::physframetime) <= 0 && proj.lifemillis) || (!proj.stuck && !proj.beenused && !move(proj, physics::physframetime)))
        {
            if(proj.lifetime < 0) proj.lifetime = 0;
            return false;
        }
        return true;
    }

    bool move(projent &proj)
    {
        if(physics::physsteps <= 0)
        {
            physics::interppos(&proj);
            return true;
        }

        bool alive = true;
        proj.o = proj.newpos;
        proj.o.z += proj.height;
        loopi(physics::physsteps-1) if(!(alive = moveframe(proj))) break;
        proj.deltapos = proj.o;
        if(alive) alive = moveframe(proj);
        proj.newpos = proj.o;
        proj.deltapos.sub(proj.newpos);
        proj.newpos.z -= proj.height;
        if(alive) physics::interppos(&proj);
        return alive;
    }

    bool raymove(projent &proj)
    {
        if((proj.lifetime -= physics::physframetime) <= 0 && proj.lifemillis)
        {
            if(proj.lifetime < 0) proj.lifetime = 0;
            return false;
        }
        if(!proj.lastbounce || lastmillis-proj.lastbounce >= 40)
        {
            vec to(proj.to), ray = vec(proj.to).sub(proj.from).mul(weaptype[proj.weap].tracesize);
            float maxdist = ray.magnitude();
            if(maxdist <= 0) return 1; // not moving anywhere, so assume still alive since it was already alive
            ray.mul(1/maxdist);
            float dist = tracecollide(&proj, proj.from, ray, maxdist, RAY_CLIPMAT | RAY_ALPHAPOLY, proj.projcollide&COLLIDE_PLAYER);
            if((dist >= 0 && (hitplayer ? proj.projcollide&COLLIDE_PLAYER : proj.projcollide&COLLIDE_GEOM)))
            {
                proj.o = vec(proj.from).add(vec(ray).mul(dist));
                if(hitplayer)
                {
                    proj.lastbounce = lastmillis;
                    if(!hiteffect(proj, hitplayer, hitflags, vec(hitplayer->o).sub(proj.from).normalize())) return true;
                }
                else proj.norm = hitsurface;
                if(proj.projcollide&(hitplayer ? IMPACT_PLAYER : IMPACT_GEOM))
                    return false; // die on impact
                return true;
            }
        }
        proj.o = proj.to;
        return true;
    }

    void update()
    {
        vector<projent *> canremove;
        loopvrev(projs) if(projs[i]->projtype == PRJ_DEBRIS || projs[i]->projtype == PRJ_GIBS || projs[i]->projtype == PRJ_EJECT)
            canremove.add(projs[i]);
        while(!canremove.empty() && canremove.length() > maxprojectiles)
        {
            int oldest = 0;
            loopv(canremove)
                if(lastmillis-canremove[i]->addtime >= lastmillis-canremove[oldest]->addtime)
                    oldest = i;
            if(canremove.inrange(oldest))
            {
                canremove[oldest]->state = CS_DEAD;
                canremove.removeunordered(oldest);
            }
            else break;
        }

        loopv(projs)
        {
            projent &proj = *projs[i];
            if(proj.projtype == PRJ_SHOT && WEAP2(proj.weap, radial, proj.flags&HIT_ALT))
            {
                proj.hit = NULL;
                proj.hitflags = HITFLAG_NONE;
            }
            hits.setsize(0);
            if((proj.projtype != PRJ_SHOT || proj.owner) && proj.state != CS_DEAD)
            {
                if(proj.projtype == PRJ_ENT && entities::ents.inrange(proj.id) && entities::ents[proj.id]->type == WEAPON) // in case spawnweapon changes
                    proj.mdl = entities::entmdlname(entities::ents[proj.id]->type, entities::ents[proj.id]->attrs);

                if(proj.waittime > 0)
                {
                    if((proj.waittime -= curtime) <= 0)
                    {
                        proj.waittime = 0;
                        init(proj, true);
                    }
                    else continue;
                }
                iter(proj);
                if(proj.projtype == PRJ_SHOT || proj.projtype == PRJ_ENT)
                {
                    if(proj.projtype == PRJ_SHOT && weaptype[proj.weap].traced ? !raymove(proj) : !move(proj)) switch(proj.projtype)
                    {
                        case PRJ_ENT:
                        {
                            if(!proj.beenused)
                            {
                                proj.beenused = 1;
                                proj.lifetime = min(proj.lifetime, proj.fadetime);
                            }
                            if(proj.lifetime > 0) break;
                        }
                        default: proj.state = CS_DEAD; break;
                    }
                }
                else for(int rtime = curtime; proj.state != CS_DEAD && rtime > 0;)
                {
                    int qtime = min(rtime, 30);
                    rtime -= qtime;

                    if(((proj.lifetime -= qtime) <= 0 && proj.lifemillis) || (!proj.stuck && !proj.beenused && !move(proj, qtime)))
                    {
                        if(proj.lifetime < 0) proj.lifetime = 0;
                        proj.state = CS_DEAD;
                        break;
                    }
                }
                effect(proj);
            }
            else proj.state = CS_DEAD;
            if(proj.local && proj.owner && proj.projtype == PRJ_SHOT)
            {
                int radius = 0;
                if(weaptype[proj.weap].traced) proj.o = proj.to;
                if(proj.state == CS_DEAD)
                {
                    if(WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale) > 0)
                    {
                        if(!(proj.projcollide&COLLIDE_CONT)) proj.hit = NULL;
                        radius = int(WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale));
                        if(WEAP2(proj.weap, taper, proj.flags&HIT_ALT) > 0) radius = int(ceilf(radius*proj.lifesize));
                        if(!proj.limited && radius > 0)
                        {
                            loopj(game::numdynents())
                            {
                                gameent *f = (gameent *)game::iterdynents(j);
                                if(!f || f->state != CS_ALIVE || !physics::issolid(f, &proj, false)) continue;
                                radialeffect(f, proj, true, radius);
                            }
                        }
                    }
                }
                else if(WEAP2(proj.weap, radial, proj.flags&HIT_ALT))
                {
                    if(!(proj.projcollide&COLLIDE_CONT)) proj.hit = NULL;
                    radius = int(ceilf(WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale)*proj.lifesize));
                    if(!proj.limited && radius > 0 && (!proj.lastradial || lastmillis-proj.lastradial >= 40))
                    {
                        loopj(game::numdynents())
                        {
                            gameent *f = (gameent *)game::iterdynents(j);
                            if(!f || f->state != CS_ALIVE || !physics::issolid(f, &proj, true)) continue;
                            if(radialeffect(f, proj, false, radius)) proj.lastradial = lastmillis;
                        }
                    }
                }
                if(!hits.empty())
                {
                    client::addmsg(N_DESTROY, "ri6iv", proj.owner->clientnum, lastmillis-game::maptime, proj.weap, proj.flags, proj.id >= 0 ? proj.id-game::maptime : proj.id,
                            int(radius), hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
                }
            }
            if(proj.state == CS_DEAD)
            {
                destroy(proj);
                delete &proj;
                projs.removeunordered(i--);
            }
        }
    }

    void render()
    {
        loopv(projs) if(projs[i]->ready(false) && projs[i]->mdl && *projs[i]->mdl)
        {
            projent &proj = *projs[i];
            if(proj.projtype == PRJ_ENT && !entities::ents.inrange(proj.id)) continue;
            float trans = 1, size = 1;
            switch(proj.projtype)
            {
                case PRJ_GIBS: case PRJ_DEBRIS: case PRJ_EJECT: size = proj.lifesize;
                case PRJ_ENT:
                    if(proj.fadetime && proj.lifemillis)
                    {
                        int interval = min(proj.lifemillis, proj.fadetime);
                        if(proj.lifetime < interval)
                        {
                            float amt = float(proj.lifetime)/float(interval);
                            size *= amt; trans *= amt;
                        }
                        else if(proj.projtype != PRJ_EJECT && proj.lifemillis > interval)
                        {
                            interval = min(proj.lifemillis-interval, proj.fadetime);
                            if(proj.lifemillis-proj.lifetime < interval)
                            {
                                float amt = float(proj.lifemillis-proj.lifetime)/float(interval);
                                size *= amt; trans *= amt;
                            }
                        }
                    }
                    break;
                default: break;
            }
            rendermodel(&proj.light, proj.mdl, ANIM_MAPMODEL|ANIM_LOOP, proj.o, proj.yaw+90, proj.pitch, proj.roll, MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_DYNSHADOW|MDL_LIGHT|MDL_CULL_DIST, NULL, NULL, 0, 0, trans, size);
        }
    }

    void adddynlights()
    {
        loopv(projs) if(projs[i]->ready() && projs[i]->projtype == PRJ_SHOT)
        {
            projent &proj = *projs[i];
            switch(proj.weap)
            {
                case WEAP_SHOTGUN: adddynlight(proj.o, 16, vec(0.5f, 0.35f, 0.1f)); break;
                case WEAP_SMG: adddynlight(proj.o, 8, vec(0.5f, 0.25f, 0.05f)); break;
                case WEAP_FLAMER:
                {
                    vec col(1.1f*max(1.f-proj.lifespan,0.3f)*proj.scale, 0.2f*max(1.f-proj.lifespan,0.15f)*proj.scale, 0.00f);
                    adddynlight(proj.o, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale)*1.1f*proj.lifespan, col);
                    break;
                }
                case WEAP_PLASMA:
                {
                    vec col(0.1f*max(1.f-proj.lifespan,0.1f)*proj.scale, 0.4f*max(1.f-proj.lifespan,0.1f)*proj.scale, 0.6f*max(1.f-proj.lifespan,0.1f)*proj.scale);
                    adddynlight(proj.o, WEAPEX(proj.weap, proj.flags&HIT_ALT, game::gamemode, game::mutators, proj.scale)*1.1f*proj.lifesize, col);
                    break;
                }
                default: break;
            }
        }
    }
}

