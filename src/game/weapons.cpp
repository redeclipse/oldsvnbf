#include "game.h"
namespace weapons
{
    VAR(IDF_PERSIST, autoreloading, 0, 2, 4); // 0 = never, 1 = when empty, 2 = weapons that don't add a full clip, 3 = always (+1 zooming weaps too)
    VAR(IDF_PERSIST, autoreloaddelay, 0, 100, INT_MAX-1);

    VAR(IDF_PERSIST, skipspawnweapon, 0, 0, 6); // skip spawnweapon; 0 = never, 1 = if numweaps > 1 (+1), 3 = if carry > 0 (+2), 6 = always
    VAR(IDF_PERSIST, skipmelee, 0, 7, 10); // skip melee; 0 = never, 1 = if numweaps > 1 (+2), 4 = if carry > 0 (+2), 7 = if carry > 0 and is offset (+2), 10 = always
    VAR(IDF_PERSIST, skippistol, 0, 8, 10); // skip pistol; 0 = never, 1 = if numweaps > 1 (+2), 4 = if carry > 0 (+2), 7 = if carry > 0 and is offset (+2), 10 = always
    VAR(IDF_PERSIST, skipgrenade, 0, 0, 10); // skip grenade; 0 = never, 1 = if numweaps > 1 (+2), 4 = if carry > 0 (+2), 7 = if carry > 0 and is offset (+2), 10 = always

    int lastweapselect = 0;
    VAR(IDF_PERSIST, weapselectdelay, 0, 100, INT_MAX-1);

    ICOMMAND(0, weapselect, "", (), intret(game::player1->weapselect));
    ICOMMAND(0, ammo, "i", (int *n), intret(isweap(*n) ? game::player1->ammo[*n] : -1));
    ICOMMAND(0, hasweap, "ii", (int *n, int *o), intret(isweap(*n) && game::player1->hasweap(*n, *o) ? 1 : 0));
    ICOMMAND(0, getweap, "ii", (int *n, int *o), {
        if(isweap(*n))
        {
            switch(*o)
            {
                case 0: result(weaptype[*n].name); break;
                case 1: result(hud::itemtex(WEAPON, *n)); break;
                default: break;
            }
        }
    });

    bool weapselect(gameent *d, int weap, bool local)
    {
        if(!local || d->canswitch(weap, m_weapon(game::gamemode, game::mutators), lastmillis, (1<<WEAP_S_RELOAD)|(1<<WEAP_S_SWITCH)))
        {
            if(local) client::addmsg(N_WEAPSELECT, "ri3", d->clientnum, lastmillis-game::maptime, weap);
            playsound(S_SWITCH, d->o, d);
            d->weapswitch(weap, lastmillis);
            d->action[AC_RELOAD] = false;
            return true;
        }
        return false;
    }

    bool weapreload(gameent *d, int weap, int load, int ammo, bool local)
    {
        if(!local || d->canreload(weap, m_weapon(game::gamemode, game::mutators), lastmillis))
        {
            bool doact = false;
            if(local)
            {
                client::addmsg(N_RELOAD, "ri3", d->clientnum, lastmillis-game::maptime, weap);
                int oldammo = d->ammo[weap];
                ammo = min(max(d->ammo[weap], 0) + WEAP(weap, add), WEAP(weap, max));
                load = ammo-oldammo;
                doact = true;
            }
            else if(d != game::player1 && !d->ai) doact = true;
            else if(load < 0 && d->ammo[weap] < ammo) return false; // because we've already gone ahead..
            d->weapload[weap] = load;
            d->ammo[weap] = min(ammo, WEAP(weap, max));
            if(doact)
            {
                if(issound(d->wschan)) removesound(d->wschan);
                playsound(S_RELOAD, d->o, d, 0, -1, -1, -1, &d->wschan);
                d->setweapstate(weap, WEAP_S_RELOAD, WEAP(weap, rdelay), lastmillis);
            }
            return true;
        }
        return false;
    }

    void weaponswitch(gameent *d, int a = -1, int b = -1)
    {
        if(a < -1 || b < -1 || a >= WEAP_MAX || b >= WEAP_MAX || (weapselectdelay && lastweapselect && lastmillis-lastweapselect < weapselectdelay)) return;
        if(!d->weapwaited(d->weapselect, lastmillis, d->skipwait(d->weapselect, 0, lastmillis, (1<<WEAP_S_RELOAD)|(1<<WEAP_S_SWITCH), true))) return;
        int s = d->weapselect;
        loopi(WEAP_MAX) // only loop the amount of times we have weaps for
        {
            if(a >= 0) s = a;
            else s += b;

            while(s > WEAP_MAX-1) s -= WEAP_MAX;
            while(s < 0) s += WEAP_MAX;

            if(a < 0)
            { // weapon skipping when scrolling
                int p = m_weapon(game::gamemode, game::mutators);
                #define skipweap(q,w) \
                { \
                    if(q && s == w) switch(q) \
                    { \
                        case 10: continue; break; \
                        case 7: case 8: case 9: if(d->carry(p, 5, w) > (q-7)) continue; break; \
                        case 4: case 5: case 6: if(d->carry(p, 1, w) > (q-3)) continue; break; \
                        case 1: case 2: case 3: if(d->carry(p, 0, w) > q) continue; break; \
                        case 0: default: break; \
                    } \
                }
                skipweap(skipspawnweapon, p);
                skipweap(skipmelee, WEAP_MELEE);
                skipweap(skippistol, WEAP_PISTOL);
                skipweap(skipgrenade, WEAP_GRENADE);
            }

            if(weapselect(d, s))
            {
                lastweapselect = lastmillis;
                return;
            }
            else if(a >= 0) break;
        }
        if(d == game::player1) playsound(S_ERROR, d->o, d);
    }
    ICOMMAND(0, weapon, "ss", (char *a, char *b), weaponswitch(game::player1, *a ? atoi(a) : -1, *b ? atoi(b) : -1));

    void drop(gameent *d, int a = -1)
    {
        int weap = isweap(a) ? a : d->weapselect;
        bool found = false;
        if(isweap(weap) && weap != WEAP_MELEE && weap != m_weapon(game::gamemode, game::mutators) && !m_noitems(game::gamemode, game::mutators) && entities::ents.inrange(d->entid[weap]))
        {
            if(d->weapwaited(d->weapselect, lastmillis, d->skipwait(d->weapselect, 0, lastmillis, (1<<WEAP_S_RELOAD)|(1<<WEAP_S_SWITCH), true)))
            {
                client::addmsg(N_DROP, "ri3", d->clientnum, lastmillis-game::maptime, weap);
                d->setweapstate(d->weapselect, WEAP_S_WAIT, WEAPSWITCHDELAY, lastmillis);
                d->action[AC_RELOAD] = false;
                found = true;
            }
        }
        if(!found && d == game::player1) playsound(S_ERROR, d->o, d);
    }
    ICOMMAND(0, drop, "s", (char *n), drop(game::player1, *n ? atoi(n) : -1));

    bool autoreload(gameent *d, int flags = 0)
    {
        if(d == game::player1)
        {
            bool noammo = d->ammo[d->weapselect] < WEAP2(d->weapselect, sub, flags&HIT_ALT);
            if((noammo || (!d->action[AC_ATTACK] && !d->action[AC_ALTERNATE])) && !d->action[AC_USE] && d->weapstate[d->weapselect] == WEAP_S_IDLE && (noammo || lastmillis-d->weaplast[d->weapselect] >= autoreloaddelay))
                return autoreloading >= (noammo ? 1 : (WEAP(d->weapselect, add) < WEAP(d->weapselect, max) ? 2 : (WEAP(d->weapselect, zooms) ? 4 : 3)));
        }
        return false;
    }

    void reload(gameent *d)
    {
        int sweap = m_weapon(game::gamemode, game::mutators);
        if(!d->hasweap(d->weapselect, sweap)) weapselect(d, d->bestweap(sweap, true));
        else if(d->action[AC_RELOAD] || autoreload(d)) weapreload(d, d->weapselect);
    }

    void offsetray(vec &from, vec &to, int spread, int z, vec &dest)
    {
        float f = to.dist(from)*float(spread)/10000.f;
        for(;;)
        {
            #define RNDD rnd(101)-50
            vec v(RNDD, RNDD, RNDD);
            if(v.magnitude() > 50) continue;
            v.mul(f);
            v.z = z > 0 ? v.z/float(z) : 0;
            dest = to;
            dest.add(v);
            vec dir = vec(dest).sub(from).normalize();
            raycubepos(from, dir, dest, 0, RAY_CLIPMAT|RAY_ALPHAPOLY);
            return;
        }
    }

    void shoot(gameent *d, vec &targ, int force)
    {
        if(!game::allowmove(d)) return;
        bool secondary = false, pressed = (d->action[AC_ATTACK] || (d->action[AC_ALTERNATE] && !WEAP(d->weapselect, zooms)));
        if(d == game::player1 && WEAP(d->weapselect, zooms) && game::zooming && game::inzoomswitch()) secondary = true;
        else if(!WEAP(d->weapselect, zooms) && d->action[AC_ALTERNATE] && (!d->action[AC_ATTACK] || d->actiontime[AC_ALTERNATE] > d->actiontime[AC_ATTACK])) secondary = true;
        else if(WEAP(d->weapselect, power) && d->weapstate[d->weapselect] == WEAP_S_POWER && d->actiontime[AC_ALTERNATE] > d->actiontime[AC_ATTACK]) secondary = true;
        int power = clamp(force, 0, WEAP(d->weapselect, power)), flags = secondary ? HIT_ALT : 0, offset = WEAP2(d->weapselect, sub, flags&HIT_ALT), sweap = m_weapon(game::gamemode, game::mutators);
        if(!d->canshoot(d->weapselect, flags, sweap, lastmillis))
        {
            if(!d->canshoot(d->weapselect, flags, sweap, lastmillis, (1<<WEAP_S_RELOAD)))
            {
                // if the problem is not enough ammo, do the reload..
                if(autoreload(d, flags)) weapreload(d, d->weapselect);
                return;
            }
            else offset = -1;
        }
        if(WEAP(d->weapselect, power) && !WEAP(d->weapselect, zooms))
        {
            if(!power)
            {
                if(d->weapstate[d->weapselect] != WEAP_S_POWER)
                {
                    if(pressed)
                    {
                        client::addmsg(N_PHYS, "ri2", d->clientnum, SPHY_POWER);
                        d->setweapstate(d->weapselect, WEAP_S_POWER, 0, lastmillis);
                    }
                    else return;
                }
                power = clamp(lastmillis-d->weaplast[d->weapselect], 0, WEAP(d->weapselect, power));
                if(pressed && power < WEAP(d->weapselect, power)) return;
            }
        }
        else if(!pressed) return;
        if(offset < 0)
        {
            offset = max(d->weapload[d->weapselect], 1)+WEAP2(d->weapselect, sub, flags&HIT_ALT);
            d->weapload[d->weapselect] = -d->weapload[d->weapselect];
        }
        if(!WEAP2(d->weapselect, fullauto, flags&HIT_ALT))
            d->action[secondary && !WEAP(d->weapselect, zooms) ? AC_ALTERNATE : AC_ATTACK] = false;
        d->action[AC_RELOAD] = false;
        vec to = targ, from = d->muzzlepos(d->weapselect), unitv;
        float dist = to.dist(from, unitv);
        if(dist > 0) unitv.div(dist);
        else vecfromyawpitch(d->yaw, d->pitch, 1, 0, unitv);
        if(d->aitype < AI_START || d->maxspeed)
        {
            vec kick = vec(unitv).mul(-WEAP2(d->weapselect, kickpush, flags&HIT_ALT));
            if(d == game::player1)
            {
                if(WEAP(d->weapselect, zooms) && game::inzoom()) kick.mul(0.0125f);
                game::swaypush.add(vec(kick).mul(0.025f));
                if(!physics::iscrouching(d)) d->quake = clamp(d->quake+max(int(WEAP2(d->weapselect, kickpush, flags&HIT_ALT)), 1), 0, 1000);
            }
            if(!physics::iscrouching(d)) d->vel.add(vec(kick).mul(0.5f));
        }

        // move along the eye ray towards the weap origin, stopping when something is hit
        // nudge the target a tiny bit forward in the direction of the target for stability
        vec eyedir(from);
        eyedir.sub(d->o);
        float eyedist = eyedir.magnitude();
        if(eyedist > 0) eyedir.div(eyedist);
        float barrier = eyedist > 0 ? raycube(d->o, eyedir, eyedist, RAY_CLIPMAT) : eyedist;
        if(barrier < eyedist)
        {
            (from = eyedir).mul(barrier).add(d->o);
            (to = targ).sub(from).rescale(1e-3f).add(from);
        }
        else
        {
            barrier = raycube(from, unitv, dist, RAY_CLIPMAT|RAY_ALPHAPOLY);
            if(barrier < dist)
            {
                to = unitv;
                to.mul(barrier);
                to.add(from);
            }
        }

        vector<vec> vshots;
        vector<ivec> shots;
        #define addshot { vshots.add(dest); shots.add(ivec(int(dest.x*DMF), int(dest.y*DMF), int(dest.z*DMF))); }
        loopi(WEAP2(d->weapselect, rays, flags&HIT_ALT))
        {
            vec dest;
            int spread = WEAPSP(d->weapselect, flags&HIT_ALT, game::gamemode, game::mutators);
            if(spread) offsetray(from, to, spread, WEAP2(d->weapselect, zdiv, flags&HIT_ALT), dest);
            else dest = to;
            if(weaptype[d->weapselect].thrown[flags&HIT_ALT ? 1 : 0] > 0)
                dest.z += from.dist(dest)*weaptype[d->weapselect].thrown[flags&HIT_ALT ? 1 : 0];
            addshot;
        }
        projs::shootv(d->weapselect, flags, offset, power, from, vshots, d, true);
        client::addmsg(N_SHOOT, "ri8iv", d->clientnum, lastmillis-game::maptime, d->weapselect, flags, power, int(from.x*DMF), int(from.y*DMF), int(from.z*DMF), shots.length(), shots.length()*sizeof(ivec)/sizeof(int), shots.getbuf());
    }

    void preload(int weap)
    {
        int g = weap < 0 ? m_weapon(game::gamemode, game::mutators) : weap;
        if(g != WEAP_MELEE && isweap(g))
        {
            if(*weaptype[g].item) loadmodel(weaptype[g].item, -1, true);
            if(*weaptype[g].vwep) loadmodel(weaptype[g].vwep, -1, true);
        }
    }
}
