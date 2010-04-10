#define GAMEWORLD 1
#include "game.h"
namespace game
{
    int nextmode = -1, nextmuts = -1, gamemode = -1, mutators = -1, maptime = 0, timeremaining = 0,
        lastcamera = 0, lasttvcam = 0, lasttvchg = 0, lastzoom = 0, lastmousetype = 0, liquidchan = -1, fogdist = 0;
    bool intermission = false, prevzoom = false, zooming = false;
    float swayfade = 0, swayspeed = 0, swaydist = 0;
    vec swaydir(0, 0, 0), swaypush(0, 0, 0);
    string clientmap = "";

    gameent *player1 = new gameent(), *focus = player1;
    avatarent avatarmodel;
    vector<gameent *> players;
    vector<camstate> cameras;

    VAR(IDF_WORLD, numplayers, 0, 0, MAXCLIENTS);
    SVAR(IDF_WORLD, obitwater, "");
    SVAR(IDF_WORLD, obitdeath, "");
    SVAR(IDF_WORLD, mapmusic, "");

    VAR(IDF_PERSIST, mouseinvert, 0, 0, 1);
    VAR(IDF_PERSIST, mouseabsolute, 0, 0, 1);
    VAR(IDF_PERSIST, mousetype, 0, 0, 2);
    VAR(IDF_PERSIST, mousedeadzone, 0, 10, 100);
    VAR(IDF_PERSIST, mousepanspeed, 1, 30, INT_MAX-1);

    VAR(IDF_PERSIST, thirdperson, 0, 0, 1);
    VAR(IDF_PERSIST, thirdpersonfollow, 0, 0, 1);
    VAR(IDF_PERSIST, dynlighteffects, 0, 2, 2);
    FVAR(IDF_PERSIST, playerblend, 0, 1, 1);

    VAR(IDF_PERSIST, thirdpersonmodel, 0, 1, 1);
    VAR(IDF_PERSIST, thirdpersonfov, 90, 120, 150);
    FVAR(IDF_PERSIST, thirdpersonblend, 0, 0.45f, 1);
    FVAR(IDF_PERSIST, thirdpersondist, -1000, 20, 1000);

    VAR(IDF_PERSIST, firstpersonmodel, 0, 1, 1);
    VAR(IDF_PERSIST, firstpersonfov, 90, 100, 150);
    VAR(IDF_PERSIST, firstpersonsway, 0, 1, 1);
    FVAR(IDF_PERSIST, firstpersonswaystep, 1, 18.0f, 100);
    FVAR(IDF_PERSIST, firstpersonswayside, 0, 0.06f, 1);
    FVAR(IDF_PERSIST, firstpersonswayup, 0, 0.08f, 1);
    FVAR(IDF_PERSIST, firstpersonblend, 0, 1, 1);
    FVAR(IDF_PERSIST, firstpersondist, -10000, -0.25f, 10000);
    FVAR(IDF_PERSIST, firstpersonshift, -10000, 0.3f, 10000);
    FVAR(IDF_PERSIST, firstpersonadjust, -10000, -0.07f, 10000);

    VAR(IDF_PERSIST, editfov, 1, 120, 179);
    VAR(IDF_PERSIST, specfov, 1, 120, 179);

    VAR(IDF_PERSIST, follow, 0, 0, INT_MAX-1);
    VARF(IDF_PERSIST, specmode, 0, 1, 1, follow = 0); // 0 = float, 1 = tv
    VARF(IDF_PERSIST, waitmode, 0, 0, 2, follow = 0); // 0 = float, 1 = tv in duel/survivor, 2 = tv always

    VAR(IDF_PERSIST, spectvtime, 1000, 10000, INT_MAX-1);
    FVAR(IDF_PERSIST, spectvspeed, 0, 0.5f, 1000);
    FVAR(IDF_PERSIST, spectvpitch, 0, 0.5f, 1000);
    FVAR(IDF_PERSIST, spectvbias, 0, 2.5f, 1000);

    VAR(IDF_PERSIST, deathcamstyle, 0, 1, 2); // 0 = no follow, 1 = follow attacker, 2 = follow self
    FVAR(IDF_PERSIST, deathcamspeed, 0, 2.f, 1000);

    FVAR(IDF_PERSIST, sensitivity, 1e-3f, 10.0f, 1000);
    FVAR(IDF_PERSIST, yawsensitivity, 1e-3f, 10.0f, 1000);
    FVAR(IDF_PERSIST, pitchsensitivity, 1e-3f, 7.5f, 1000);
    FVAR(IDF_PERSIST, mousesensitivity, 1e-3f, 1.0f, 1000);
    FVAR(IDF_PERSIST, zoomsensitivity, 0, 0.75f, 1);

    VAR(IDF_PERSIST, zoommousetype, 0, 0, 2);
    VAR(IDF_PERSIST, zoommousedeadzone, 0, 25, 100);
    VAR(IDF_PERSIST, zoommousepanspeed, 1, 10, INT_MAX-1);
    VAR(IDF_PERSIST, zoomfov, 1, 10, 150);
    VAR(IDF_PERSIST, zoomtime, 1, 100, 10000);

    VARF(IDF_PERSIST, zoomlevel, 1, 4, 10, checkzoom());
    VAR(IDF_PERSIST, zoomlevels, 1, 5, 10);
    VAR(IDF_PERSIST, zoomdefault, 0, 0, 10); // 0 = last used, else defines default level
    VAR(IDF_PERSIST, zoomscroll, 0, 0, 1); // 0 = stop at min/max, 1 = go to opposite end

    VAR(IDF_PERSIST, shownamesabovehead, 0, 2, 2);
    VAR(IDF_PERSIST, showstatusabovehead, 0, 2, 2);
    VAR(IDF_PERSIST, showteamabovehead, 0, 1, 3);
    VAR(IDF_PERSIST, showdamageabovehead, 0, 0, 2);
    VAR(IDF_PERSIST, showcritabovehead, 0, 1, 2);
    FVAR(IDF_PERSIST, aboveheadblend, 0.f, 0.75f, 1.f);
    FVAR(IDF_PERSIST, aboveheadsmooth, 0, 0.5f, 1);
    VAR(IDF_PERSIST, aboveheadsmoothmillis, 1, 200, 10000);
    VAR(IDF_PERSIST, aboveheadfade, 500, 5000, INT_MAX-1);

    VAR(IDF_PERSIST, showobituaries, 0, 4, 5); // 0 = off, 1 = only me, 2 = 1 + announcements, 3 = 2 + but dying bots, 4 = 3 + but bot vs bot, 5 = all
    VAR(IDF_PERSIST, showobitdists, 0, 0, 1);
    VAR(IDF_PERSIST, showplayerinfo, 0, 2, 2); // 0 = none, 1 = CON_MESG, 2 = CON_EVENT
    VAR(IDF_PERSIST, playdamagetones, 0, 1, 3);
    VAR(IDF_PERSIST, playcrittones, 0, 2, 3);
    VAR(IDF_PERSIST, playreloadnotify, 0, 1, 4);

    VAR(IDF_PERSIST, quakefade, 0, 100, INT_MAX-1);
    VAR(IDF_PERSIST, ragdolls, 0, 1, 1);
    FVAR(IDF_PERSIST, bloodscale, 0, 1, 1000);
    VAR(IDF_PERSIST, bloodfade, 1, 5000, INT_MAX-1);
    VAR(IDF_PERSIST, bloodsize, 1, 15, 1000);
    FVAR(IDF_PERSIST, debrisscale, 0, 1, 1000);
    VAR(IDF_PERSIST, debrisfade, 1, 5000, INT_MAX-1);
    VAR(IDF_PERSIST, fireburning, 0, 2, 2);
    VAR(IDF_PERSIST, fireburnfade, 100, 250, INT_MAX-1);
    FVAR(IDF_PERSIST, fireburnblend, 0, 0.5f, 1);
    FVAR(IDF_PERSIST, impulsescale, 0, 1, 1000);
    VAR(IDF_PERSIST, impulsefade, 0, 200, INT_MAX-1);

    ICOMMAND(0, gamemode, "", (), intret(gamemode));
    ICOMMAND(0, mutators, "", (), intret(mutators));
    ICOMMAND(0, getintermission, "", (), intret(intermission ? 1 : 0));

    void start() { }

    const char *gametitle() { return connected() ? server::gamename(gamemode, mutators) : "ready"; }
    const char *gametext() { return connected() ? mapname : "not connected"; }

    bool thirdpersonview(bool viewonly)
    {
        if(!viewonly && (focus->state == CS_DEAD || focus->state == CS_WAITING)) return true;
        if(!(focus != player1 ? thirdpersonfollow : thirdperson)) return false;
        if(player1->state == CS_EDITING) return false;
        if(player1->state == CS_SPECTATOR && focus == player1) return false;
        if(inzoom()) return false;
        return true;
    }
    ICOMMAND(0, isthirdperson, "i", (int *viewonly), intret(thirdpersonview(*viewonly ? true : false) ? 1 : 0));
    ICOMMAND(0, thirdpersonswitch, "", (), int *n = (focus != player1 ? &thirdpersonfollow : &thirdperson); *n = !*n;);

    int mousestyle()
    {
        if(inzoom()) return zoommousetype;
        return mousetype;
    }

    int deadzone()
    {
        if(inzoom()) return zoommousedeadzone;
        return mousedeadzone;
    }

    int panspeed()
    {
        if(inzoom()) return zoommousepanspeed;
        return mousepanspeed;
    }

    int fov()
    {
        if(player1->state == CS_EDITING) return editfov;
        if(focus == player1 && player1->state == CS_SPECTATOR) return specfov;
        if(thirdpersonview(true)) return thirdpersonfov;
        return firstpersonfov;
    }

    void checkzoom()
    {
        if(zoomdefault > zoomlevels) zoomdefault = zoomlevels;
        if(zoomlevel < 0) zoomlevel = zoomdefault ? zoomdefault : zoomlevels;
        if(zoomlevel > zoomlevels) zoomlevel = zoomlevels;
    }

    void setzoomlevel(int level)
    {
        checkzoom();
        zoomlevel += level;
        if(zoomlevel > zoomlevels) zoomlevel = zoomscroll ? 1 : zoomlevels;
        else if(zoomlevel < 1) zoomlevel = zoomscroll ? zoomlevels : 1;
    }
    ICOMMAND(0, setzoom, "i", (int *level), setzoomlevel(*level));

    void zoomset(bool on, int millis)
    {
        if(on != zooming)
        {
            resetcursor();
            lastzoom = millis-max(zoomtime-(millis-lastzoom), 0);
            prevzoom = zooming;
            if(zoomdefault && on) zoomlevel = zoomdefault;
        }
        checkzoom();
        zooming = on;
    }

    bool zoomallow()
    {
        if(allowmove(player1) && WEAP(player1->weapselect, zooms)) return true;
        zoomset(false, 0);
        return false;
    }

    bool inzoom()
    {
        if(zoomallow() && lastzoom && (zooming || lastmillis-lastzoom <= zoomtime))
            return true;
        return false;
    }
    ICOMMAND(0, iszooming, "", (), intret(inzoom() ? 1 : 0));

    bool inzoomswitch()
    {
        if(zoomallow() && lastzoom && ((zooming && lastmillis-lastzoom >= zoomtime/2) || (!zooming && lastmillis-lastzoom <= zoomtime/2)))
            return true;
        return false;
    }

    void resetsway()
    {
        swaydir = swaypush = vec(0, 0, 0);
        swayfade = swayspeed = swaydist = 0;
    }

    void addsway(gameent *d)
    {
        if(firstpersonsway)
        {
            float maxspeed = physics::movevelocity(d);
            if(d->physstate >= PHYS_SLOPE)
            {
                swayspeed = min(sqrtf(d->vel.x*d->vel.x + d->vel.y*d->vel.y), maxspeed);
                swaydist += swayspeed*curtime/1000.0f;
                swaydist = fmod(swaydist, 2*firstpersonswaystep);
                swayfade = 1;
            }
            else if(swayfade > 0)
            {
                swaydist += swayspeed*swayfade*curtime/1000.0f;
                swaydist = fmod(swaydist, 2*firstpersonswaystep);
                swayfade -= 0.5f*(curtime*maxspeed)/(firstpersonswaystep*1000.0f);
            }

            float k = pow(0.7f, curtime/25.0f);
            swaydir.mul(k);
            vec vel = vec(d->vel).add(d->falling).mul(physics::sprinting(d) ? 5 : 1);
            float speedscale = max(vel.magnitude(), maxspeed);
            if(speedscale > 0) swaydir.add(vec(vel).mul((1-k)/(15*speedscale)));
            swaypush.mul(pow(0.5f, curtime/25.0f));
        }
        else resetsway();
    }

    void announce(int idx, int targ, gameent *d, const char *msg, ...)
    {
        if(targ >= 0 && msg && *msg)
        {
            defvformatstring(text, msg, msg);
            conoutft(targ, "%s", text);
        }
        if(idx >= 0)
        {
            if(d && issound(d->aschan)) removesound(d->aschan);
            physent *t = !d || d == focus ? camera1 : d;
            playsound(idx, t->o, t, t == camera1 ? SND_FORCED : 0, 255, getworldsize()/2, 0, d ? &d->aschan : NULL);
        }
    }
    ICOMMAND(0, announce, "iis", (int *idx, int *targ, char *s), announce(*idx, *targ, NULL, "\fw%s", s));

    bool tvmode()
    {
        if(!m_edit(gamemode)) switch(player1->state)
        {
            case CS_SPECTATOR: if(specmode) return true; break;
            case CS_WAITING: if(waitmode >= (m_duke(game::gamemode, game::mutators) ? 1 : 2) && (!player1->lastdeath || lastmillis-player1->lastdeath >= 500))
                return true; break;
            default: break;
        }
        return false;
    }

    ICOMMAND(0, specmodeswitch, "", (), specmode = specmode ? 0 : 1; hud::sb.showscores(false); follow = 0);
    ICOMMAND(0, waitmodeswitch, "", (), waitmode = waitmode ? 0 : (m_duke(game::gamemode, game::mutators) ? 1 : 2); hud::sb.showscores(false); follow = 0);

    void followswitch(int n)
    {
        follow += n;
        #define checkfollow \
            if(follow >= numdynents()) follow = 0; \
            else if(follow < 0) follow = numdynents()-1;
        checkfollow;
        while(!iterdynents(follow))
        {
            follow += clamp(n, -1, 1);
            checkfollow;
        }
    }
    ICOMMAND(0, followdelta, "i", (int *n), followswitch(*n ? *n : 1));

    bool allowmove(physent *d)
    {
        if(d == player1)
        {
            if(hud::hasinput(true, false)) return false;
            if(tvmode()) return false;
        }
        if(d->type == ENT_PLAYER || d->type == ENT_AI)
        {
            if(d->state == CS_DEAD || d->state == CS_WAITING || d->state == CS_SPECTATOR || intermission)
                return false;
        }
        return true;
    }

    void chooseloadweap(gameent *d, const char *s)
    {
        if(m_arena(gamemode, mutators))
        {
            if(*s >= '0' && *s <= '9') d->loadweap = atoi(s);
            else loopi(WEAP_MAX) if(!strcasecmp(weaptype[i].name, s))
            {
                d->loadweap = i;
                break;
            }
            if(d->loadweap < WEAP_OFFSET || d->loadweap >= WEAP_ITEM) d->loadweap = WEAP_MELEE;
            if(WEAP(d->loadweap, allowed) >= (m_insta(gamemode, mutators) ? 2 : 1))
            {
                client::addmsg(N_LOADWEAP, "ri2", d->clientnum, d->loadweap);
                conoutft(CON_SELF, "you will spawn with: %s%s", weaptype[d->loadweap].text, (d->loadweap >= WEAP_OFFSET ? weaptype[d->loadweap].name : "random weapons"));
            }
            else
            {
                conoutft(CON_SELF, "sorry, the \fs%s%s\fS has been disabled, please select a different weapon", weaptype[d->loadweap].text, (d->loadweap >= WEAP_OFFSET ? weaptype[d->loadweap].name : "random weapons"));
                d->loadweap = -1;
            }
        }
        else conoutft(CON_MESG, "\foweapon selection is only available in arena");
    }
    ICOMMAND(0, loadweap, "s", (char *s), chooseloadweap(player1, s));

    void respawn(gameent *d)
    {
        if(d->state == CS_DEAD && d->respawned < 0 && (!d->lastdeath || lastmillis-d->lastdeath >= 500))
        {
            client::addmsg(N_TRYSPAWN, "ri", d->clientnum);
            d->respawned = lastmillis;
        }
    }

    float deadscale(gameent *d, float amt = 1, bool timechk = false)
    {
        float total = amt;
        if(d->state == CS_DEAD || d->state == CS_WAITING)
        {
            int len = d->aitype >= AI_START ? min(ai::aideadfade, m_campaign(gamemode) ? 60000 : 30000) : m_delay(gamemode, mutators);
            if(len > 0 && (!timechk || len > 1000))
            {
                int interval = min(len/3, 1000), over = max(len-interval, 500), millis = lastmillis-d->lastdeath;
                if(millis <= len) { if(millis >= over) total *= 1.f-(float(millis-over)/float(interval)); }
                else total = 0;
            }
        }
        return total;
    }

    float transscale(gameent *d, bool third = true)
    {
        float total = d == focus ? (third ? thirdpersonblend : firstpersonblend) : playerblend;
        if(d->state == CS_ALIVE)
        {
            int prot = m_protect(gamemode, mutators), millis = d->protect(lastmillis, prot); // protect returns time left
            if(millis > 0) total *= 1.f-(float(millis)/float(prot));
            if(d == player1 && inzoom())
            {
                int frame = lastmillis-lastzoom;
                float pc = frame <= zoomtime ? float(frame)/float(zoomtime) : 1.f;
                total *= zooming ? 1.f-pc : pc;
            }
        }
        else total = deadscale(d, total);
        return total;
    }

    void adddynlights()
    {
        if(dynlighteffects)
        {
            projs::adddynlights();
            entities::adddynlights();
            if(dynlighteffects >= 2)
            {
                if(m_ctf(gamemode)) ctf::adddynlights();
                if(m_stf(gamemode)) stf::adddynlights();
            }
            if(fireburning && fireburntime)
            {
                gameent *d = NULL;
                loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d->onfire(lastmillis, fireburntime))
                {
                    int millis = lastmillis-d->lastfire; float pc = 1, intensity = 0.25f+(rnd(75)/100.f);
                    if(fireburntime-millis < fireburndelay) pc = float(fireburntime-millis)/float(fireburndelay);
                    else pc = 0.5f+(float(millis%fireburndelay)/float(fireburndelay*2));
                    pc = deadscale(d, pc);
                    adddynlight(d->headpos(-d->height*0.5f), d->height*(1.5f+intensity)*pc, vec(1.1f*max(pc,0.5f), 0.45f*max(pc,0.2f), 0.05f*pc), 0, 0, DL_KEEP);
                }
            }
        }
    }

    void impulseeffect(gameent *d, bool effect)
    {
        if(effect || (d->state == CS_ALIVE && physics::sprinting(d, true)))
        {
            int num = int((effect ? 25 : 5)*impulsescale), len = effect ? impulsefade : impulsefade/5;
            if(num > 0 && len > 0)
            {
                float intensity = 0.25f+(rnd(75)/100.f), blend = 0.5f+(rnd(50)/100.f);
                if(d->type == ENT_PLAYER || d->type == ENT_AI)
                {
                    regularshape(PART_FIREBALL, int(d->radius), firecols[effect ? 0 : rnd(FIRECOLOURS)], 21, num, len, d->lfoot, intensity, blend, -15, 0, 5);
                    regularshape(PART_FIREBALL, int(d->radius), firecols[effect ? 0 : rnd(FIRECOLOURS)], 21, num, len, d->rfoot, intensity, blend, -15, 0, 5);
                }
                else regularshape(PART_FIREBALL, int(d->radius)*2, firecols[effect ? 0 : rnd(FIRECOLOURS)], 21, num, len, d->feetpos(), intensity, blend, -15, 0, 5);
            }
        }
    }

    void fireeffect(gameent *d)
    {
        if(fireburning >= (d != focus || thirdpersonview() ? 1 : 2) && fireburntime && d->onfire(lastmillis, fireburntime))
        {
            int millis = lastmillis-d->lastfire; float pc = 1, intensity = 0.25f+(rnd(75)/100.f), blend = 0.5f+(rnd(50)/100.f);
            if(fireburntime-millis < fireburndelay) pc = float(fireburntime-millis)/float(fireburndelay);
            else pc = 0.75f+(float(millis%fireburndelay)/float(fireburndelay*4));
            vec pos = vec(d->headpos(-d->height*0.35f)).add(vec(rnd(9)-4, rnd(9)-4, rnd(5)-2).mul(pc));
            regular_part_create(PART_FIREBALL_SOFT, max(fireburnfade, 100), pos, firecols[rnd(FIRECOLOURS)], d->height*0.75f*deadscale(d, intensity*pc), blend*pc*fireburnblend, -15, 0);
        }
    }

    gameent *pointatplayer()
    {
        loopv(players)
        {
            gameent *o = players[i];
            if(!o) continue;
            vec pos = focus->headpos();
            float dist;
            if(intersect(o, pos, worldpos, dist)) return o;
        }
        return NULL;
    }

    void setmode(int nmode, int nmuts)
    {
        nextmode = nmode; nextmuts = nmuts;
        server::modecheck(&nextmode, &nextmuts);
    }
    ICOMMAND(0, mode, "ii", (int *val, int *mut), setmode(*val, *mut));

    void heightoffset(gameent *d, bool local)
    {
        d->o.z -= d->height;
        if(d->state == CS_ALIVE)
        {
            if(physics::iscrouching(d))
            {
                bool crouching = d->action[AC_CROUCH];
                float crouchoff = 1.f-CROUCHHEIGHT;
                if(!crouching)
                {
                    float z = d->o.z, zoff = d->zradius*crouchoff, zrad = d->zradius-zoff, frac = zoff/10.f;
                    d->o.z += zrad;
                    loopi(10)
                    {
                        d->o.z += frac;
                        if(!collide(d, vec(0, 0, 1), 0.f, false))
                        {
                            crouching = true;
                            break;
                        }
                    }
                    if(crouching)
                    {
                        if(d->actiontime[AC_CROUCH] >= 0) d->actiontime[AC_CROUCH] = max(PHYSMILLIS-(lastmillis-d->actiontime[AC_CROUCH]), 0)-lastmillis;
                    }
                    else if(d->actiontime[AC_CROUCH] < 0)
                        d->actiontime[AC_CROUCH] = lastmillis-max(PHYSMILLIS-(lastmillis+d->actiontime[AC_CROUCH]), 0);
                    d->o.z = z;
                }
                if(d->type == ENT_PLAYER || d->type == ENT_AI)
                {
                    int crouchtime = abs(d->actiontime[AC_CROUCH]);
                    float amt = lastmillis-crouchtime <= PHYSMILLIS ? clamp(float(lastmillis-crouchtime)/PHYSMILLIS, 0.f, 1.f) : 1.f;
                    if(!crouching) amt = 1.f-amt;
                    crouchoff *= amt;
                }
                d->height = d->zradius-(d->zradius*crouchoff);
            }
            else d->height = d->zradius;
        }
        else d->height = d->zradius;
        d->o.z += d->height;
    }

    void checkoften(gameent *d, bool local)
    {
        d->checktags();
        adjustscaled(int, d->quake, quakefade);
        if(d->aitype < AI_START) heightoffset(d, local);
        loopi(WEAP_MAX) if(d->weapstate[i] != WEAP_S_IDLE)
        {
            if(d->state != CS_ALIVE || (d->weapstate[i] != WEAP_S_POWER && lastmillis-d->weaplast[i] >= d->weapwait[i]))
            {
                if(playreloadnotify >= ((d == focus ? 1 : 2)*(WEAP(i, add) < WEAP(i, max) ? 2 : 1)) && i == d->weapselect && d->weapstate[i] == WEAP_S_RELOAD)
                    playsound(S_NOTIFY, d->o, d, d == focus ? SND_FORCED : SND_DIRECT, 255-int(camera1->o.dist(d->o)/(getworldsize()/2)*200));
                d->setweapstate(i, WEAP_S_IDLE, 0, lastmillis);
            }
        }
        if(d->respawned > 0 && lastmillis-d->respawned >= PHYSMILLIS*4) d->respawned = -1;
        if(d->suicided > 0 && lastmillis-d->suicided >= PHYSMILLIS*4) d->suicided = -1;
        if(d->lastfire > 0 && lastmillis-d->lastfire >= fireburntime-500)
        {
            if(lastmillis-d->lastfire >= fireburntime) d->resetfire();
            else if(issound(d->fschan)) sounds[d->fschan].vol = int((d != focus ? 128 : 224)*(1.f-(lastmillis-d->lastfire-(fireburntime-500))/500.f));
        }
    }


    void otherplayers()
    {
        loopv(players) if(players[i])
        {
            gameent *d = players[i];
            const int lagtime = lastmillis-d->lastupdate;
            if(d->ai || !lagtime || intermission) continue;
            //else if(lagtime > 1000) continue;
            physics::smoothplayer(d, 1, false);
        }
    }

    bool fireburn(gameent *d, int weap, int flags)
    {
        if(fireburntime && (doesburn(weap, flags) || flags&HIT_MELT || (weap == -1 && flags&HIT_BURN)))
        {
            if(!issound(d->fschan)) playsound(S_BURNFIRE, d->o, d, SND_LOOP, d != focus ? 128 : 224, -1, -1, &d->fschan);
            if(flags&HIT_FULL) d->lastfire = lastmillis;
            else return true;
        }
        return false;
    }

    struct damagetone
    {
        enum { BURN = 1<<0, CRIT = 1<<1 };

        gameent *d, *actor;
        int damage, flags;

        damagetone() {}
        damagetone(gameent *d, gameent *actor, int damage, int flags) : d(d), actor(actor), damage(damage), flags(flags) {}

        bool merge(const damagetone &m)
        {
            if(d != m.d || actor != m.actor || flags != m.flags) return false;
            damage += m.damage;
            return true;
        }

        void play()
        {
            int snd = S_DAMAGE1;
            if(flags&CRIT) snd = S_CRITDAMAGE;
            else if(flags&BURN) snd = S_BURNDAMAGE;
            else if(damage >= 200) snd += 7;
            else if(damage >= 150) snd += 6;
            else if(damage >= 100) snd += 5;
            else if(damage >= 75) snd += 4;
            else if(damage >= 50) snd += 3;
            else if(damage >= 25) snd += 2;
            else if(damage >= 10) snd += 1;
            playsound(snd, d->o, d, d == focus ? SND_FORCED : SND_DIRECT, 255-int(camera1->o.dist(d->o)/(getworldsize()/2)*200));
        }
    };
    vector<damagetone> damagetones;

    void removedamagetones(gameent *d)
    {
        loopvrev(damagetones) if(damagetones[i].d == d || damagetones[i].actor == d) damagetones.removeunordered(i);
    }

    void mergedamagetone(gameent *d, gameent *actor, int damage, int flags)
    {
        damagetone dt(d, actor, damage, flags);
        loopv(damagetones) if(damagetones[i].merge(dt)) return;
        damagetones.add(dt);
    }

    void flushdamagetones()
    {
        loopv(damagetones) damagetones[i].play();
        damagetones.setsize(0);
    }

    static int alarmchan = -1;
    void hiteffect(int weap, int flags, int damage, gameent *d, gameent *actor, vec &dir, bool local)
    {
        bool burning = fireburn(d, weap, flags);
        if(!local || burning)
        {
            if(hithurts(flags))
            {
                if(d == focus) hud::damage(damage, actor->o, actor, weap, flags);
                if(d->type == ENT_PLAYER || d->type == ENT_AI)
                {
                    vec p = d->headpos();
                    p.z += 0.6f*(d->height + d->aboveeye) - d->height;
                    if(!isaitype(d->aitype) || aistyle[d->aitype].maxspeed)
                    {
                        if(!kidmode && bloodscale > 0)
                            part_splash(PART_BLOOD, int(clamp(damage/2, 2, 10)*bloodscale), bloodfade, p, 0x88FFFF, (rnd(bloodsize)+1)/10.f, 1, 100, DECAL_BLOOD, int(d->radius*4));
                        else part_splash(PART_HINT, int(clamp(damage/2, 2, 10)), bloodfade, p, 0xFFFF88, 1.5f, 1, 50, DECAL_STAIN, int(d->radius*4));
                    }
                    if(d->aitype < AI_START && !issound(d->vschan)) playsound(S_PAIN1+rnd(5), d->o, d, 0, -1, -1, -1, &d->vschan);
                    if(!burning) d->quake = clamp(d->quake+max(damage/2, 1), 0, 1000);
                    d->lastpain = lastmillis;
                }
                if(d != actor)
                {
                    bool sameteam = m_team(gamemode, mutators) && d->team == actor->team;
                    if(sameteam) { if(actor == focus && !burning && !issound(alarmchan)) playsound(S_ALARM, actor->o, actor, 0, -1, -1, -1, &alarmchan); }
                    else if(playdamagetones >= (actor == focus ? 1 : (d == focus ? 2 : 3))) mergedamagetone(d, actor, damage, burning ? damagetone::BURN : 0);
                    if(!burning && !sameteam) actor->lasthit = lastmillis;
                }
            }
            if(isweap(weap) && !burning && (d == player1 || (d->ai && aistyle[d->aitype].maxspeed)))
            {
                float force = (float(damage)/float(WEAP2(weap, damage, flags&HIT_ALT)))*(100.f/d->weight)*WEAP2(weap, hitpush, flags&HIT_ALT);
                if(flags&HIT_WAVE || !hithurts(flags)) force *= wavepushscale;
                else
                {
                    force *= WEAP(weap, pusharea);
                    if(d->health <= 0) force *= deadpushscale;
                    else force *= hitpushscale;
                }
                vec push = dir; push.z += 0.125f; push.mul(force);
                d->vel.add(push);
                if(flags&HIT_WAVE || flags&HIT_EXPLODE || weap == WEAP_MELEE) d->lastpush = lastmillis;
            }
            ai::damaged(d, actor, weap, flags, damage);
        }
    }

    void damaged(int weap, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir)
    {
        if(d->state != CS_ALIVE || intermission) return;
        if(hithurts(flags))
        {
            d->dodamage(health);
            if(actor->type == ENT_PLAYER || actor->type == ENT_AI) actor->totaldamage += damage;
        }
        hiteffect(weap, flags, damage, d, actor, dir, actor == player1 || actor->ai);
        if(hithurts(flags) && showdamageabovehead >= (d != focus ? 1 : 2))
        {
            defformatstring(ds)("<sub>\fr+%d", damage);
            part_textcopy(d->abovehead(), ds, d != focus ? PART_TEXT : PART_TEXT_ONTOP, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, d);
        }
        if(flags&HIT_CRIT)
        {
            if(showcritabovehead >= (d != focus ? 1 : 2))
                part_text(d->abovehead(), "<super>\fzgrCRITICAL", d != focus ? PART_TEXT : PART_TEXT_ONTOP, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, d);
            if(playcrittones >= (actor == focus ? 1 : (d == focus ? 2 : 3))) mergedamagetone(d, actor, damage, damagetone::CRIT);
        }
    }

    void killed(int weap, int flags, int damage, gameent *d, gameent *actor, int style)
    {
        if(d->type != ENT_PLAYER && d->type != ENT_AI) return;
        bool burning = fireburn(d, weap, flags);
        d->lastregen = 0;
        d->lastpain = lastmillis;
        d->state = CS_DEAD;
        d->deaths++;
        int anc = -1, dth = d->aitype >= AI_START || style&FRAG_OBLITERATE ? S_SPLOSH : S_DIE1+rnd(2);
        if(d == focus) anc = !m_duke(gamemode, mutators) && !m_trial(gamemode) ? S_V_FRAGGED : -1;
        else d->resetinterp();
        formatstring(d->obit)("%s ", colorname(d));
        if(d != actor && actor->lastattacker == d->clientnum) actor->lastattacker = -1;
        d->lastattacker = actor->clientnum;
        if(d == actor)
        {
            if(d->aitype == AI_TURRET) concatstring(d->obit, "was destroyed");
            else if(flags&HIT_DEATH) concatstring(d->obit, *obitdeath ? obitdeath : "died");
            else if(flags&HIT_WATER) concatstring(d->obit, *obitwater ? obitwater : "died");
            else if(flags&HIT_MELT) concatstring(d->obit, "melted into a ball of fire");
            else if(flags&HIT_SPAWN) concatstring(d->obit, "tried to spawn inside solid matter");
            else if(flags&HIT_LOST) concatstring(d->obit, "got very, very lost");
            else if(flags && isweap(weap) && !burning)
            {
                static const char *suicidenames[WEAP_MAX] = {
                    "punched themself",
                    "ate a bullet",
                    "discovered buckshot bounces",
                    "got caught in their own crossfire",
                    "spontaneously combusted",
                    "tried to make out with plasma",
                    "got a good shock",
                    "kicked it, kamikaze style",
                    "exploded with style",
                };
                concatstring(d->obit, suicidenames[weap]);
            }
            else if(flags&HIT_BURN || burning) concatstring(d->obit, "burned up");
            else if(style&FRAG_OBLITERATE) concatstring(d->obit, "was obliterated");
            else concatstring(d->obit, "suicided");
        }
        else
        {
            concatstring(d->obit, "was ");
            if(flags&HIT_CRIT) concatstring(d->obit, "\fs\fzgrcritically\fS ");
            if(d->aitype == AI_TURRET) concatstring(d->obit, "destroyed by");
            else
            {
                static const char *obitnames[4][WEAP_MAX] = {
                    {
                        "punched by",
                        "pierced by",
                        "sprayed with buckshot by",
                        "riddled with holes by",
                        "char-grilled by",
                        "plasmified by",
                        "laser shocked by",
                        "blown to pieces by",
                        "exploded by",
                    },
                    {
                        "kicked by",
                        "pierced by",
                        "filled with lead by",
                        "spliced apart by",
                        "fireballed by",
                        "shown the light by",
                        "was given laser burn by",
                        "blown to pieces by",
                        "exploded by",
                    },
                    {
                        "given kung-fu lessons by",
                        "capped by",
                        "scrambled by",
                        "air conditioned courtesy of",
                        "char-grilled by",
                        "plasmafied by",
                        "expertly sniped by",
                        "blown to pieces by",
                        "exploded by",
                    },
                    {
                        "knocked into next week by",
                        "skewered by",
                        "turned into little chunks by",
                        "swiss-cheesed by",
                        "barbequed by chef",
                        "reduced to ooze by",
                        "given laser shock treatment by",
                        "turned into shrapnel by",
                        "obliterated by",
                    }
                };

                int o = style&FRAG_OBLITERATE ? 3 : (style&FRAG_HEADSHOT ? 2 : (flags&HIT_ALT ? 1 : 0));
                concatstring(d->obit, burning ? "set ablaze by" : (isweap(weap) ? obitnames[o][weap] : "killed by"));
            }
            bool override = false;
            vec az = actor->abovehead(), dz = d->abovehead();
            if(!m_fight(gamemode) || actor->aitype >= AI_START)
            {
                concatstring(d->obit, " ");
                concatstring(d->obit, colorname(actor));
            }
            else if(m_team(gamemode, mutators) && d->team == actor->team)
            {
                concatstring(d->obit, " \fs\fzawteam-mate\fS ");
                concatstring(d->obit, colorname(actor));
                if(actor == focus) { anc = S_ALARM; override = true; }
            }
            else
            {
                if(style&FRAG_REVENGE)
                {
                    concatstring(d->obit, " \fs\fzoyvengeful\fS");
                    part_text(az, "<super>\fzoyAVENGED", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                    part_text(dz, "<super>\fzoyREVENGE", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, d); dz.z += 4;
                    d->dominated.removeobj(actor);
                    actor->dominating.removeobj(d);
                    anc = S_V_REVENGE; override = true;
                }
                else if(style&FRAG_DOMINATE)
                {
                    concatstring(d->obit, " \fs\fzoydominating\fS");
                    part_text(az, "<super>\fzoyDOMINATING", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                    part_text(dz, "<super>\fzoyDOMINATED", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, d); dz.z += 4;
                    if(d->dominating.find(actor) < 0) d->dominating.add(actor);
                    if(actor->dominated.find(d) < 0) actor->dominated.add(d);
                    anc = S_V_DOMINATE; override = true;
                }
                concatstring(d->obit, " ");
                concatstring(d->obit, colorname(actor));

                if(style&FRAG_MKILL1)
                {
                    concatstring(d->obit, " \fs\fzRedouble-killing\fS");
                    part_text(az, "<super>\fzvrDOUBLE-KILL", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                    if(actor == focus) { part_text(dz, "<super>\fzvrDOUBLE", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, d); dz.z += 4; }
                    if(!override) anc = S_V_MKILL1;
                }
                else if(style&FRAG_MKILL2)
                {
                    concatstring(d->obit, " \fs\fzRetriple-killing\fS");
                    part_text(az, "<super>\fzvrTRIPLE-KILL", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                    if(actor == focus) { part_text(dz, "<super>\fzvrTRIPLE", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, d); dz.z += 4; }
                    if(!override) anc = S_V_MKILL1;
                }
                else if(style&FRAG_MKILL3)
                {
                    concatstring(d->obit, " \fs\fzRemulti-killing\fS");
                    part_text(az, "<super>\fzvrMULTI-KILL", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                    if(actor == focus) { part_text(dz, "<super>\fzvrMULTI", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, d); dz.z += 4; }
                    if(!override) anc = S_V_MKILL1;
                }
            }

            if(weap != WEAP_MELEE && style&FRAG_HEADSHOT)
            {
                part_text(az, "<super>\fzcwHEADSHOT", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                if(!override) anc = S_V_HEADSHOT;
            }

            if(style&FRAG_SPREE1)
            {
                concatstring(d->obit, " in total \fs\fzcgcarnage\fS");
                part_text(az, "<super>\fzcgCARNAGE", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                if(!override) anc = S_V_SPREE1;
                override = true;
            }
            else if(style&FRAG_SPREE2)
            {
                concatstring(d->obit, " on a \fs\fzcgslaughter\fS");
                part_text(az, "<super>\fzcgSLAUGHTER", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                if(!override) anc = S_V_SPREE2;
                override = true;
            }
            else if(style&FRAG_SPREE3)
            {
                concatstring(d->obit, " on a \fs\fzcgmassacre\fS");
                part_text(az, "<super>\fzcgMASSACRE", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                if(!override) anc = S_V_SPREE3;
                override = true;
            }
            else if(style&FRAG_SPREE4)
            {
                concatstring(d->obit, " in a \fs\fzcgbloodbath\fS");
                part_text(az, "<super>\fzcgBLOODBATH", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                if(!override) anc = S_V_SPREE4;
                override = true;
            }
            else if(style&FRAG_SPREE5)
            {
                concatstring(d->obit," on a \fs\fzcgrampage\fS");
                part_text(az, "<super>\fzcgRAMPAGE", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                if(!override) anc = S_V_SPREE5;
                override = true;
            }
            else if(style&FRAG_SPREE6)
            {
                concatstring(d->obit, " who seems \fs\fzcgunstoppable\fS");
                part_text(az, "<super>\fzcgUNSTOPPABLE", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
                if(!override) anc = S_V_SPREE6;
                override = true;
            }
        }
        if(d != actor)
        {
            if(actor->state == CS_ALIVE) copystring(actor->obit, d->obit);
            actor->lastkill = lastmillis;
        }
        if(dth >= 0)
        {
            if(issound(d->vschan)) removesound(d->vschan);
            playsound(dth, d->o, d, 0, -1, -1, -1, &d->vschan);
        }
        if(showobituaries && (d->aitype < AI_START || actor->aitype < (d->aitype >= AI_START ? AI_BOT : AI_START)))
        {
            bool isme = (d == focus || actor == focus), show = false;
            if(((!m_fight(gamemode) && !isme) || actor->aitype >= AI_START) && anc >= 0) anc = -1;
            if(flags&HIT_LOST) show = true;
            else switch(showobituaries)
            {
                case 1: if(isme || m_duke(gamemode, mutators)) show = true; break;
                case 2: if(isme || anc >= 0 || m_duke(gamemode, mutators)) show = true; break;
                case 3: if(isme || d->aitype < 0 || anc >= 0 || m_duke(gamemode, mutators)) show = true; break;
                case 4: if(isme || d->aitype < 0 || actor->aitype < 0 || anc >= 0 || m_duke(gamemode, mutators)) show = true; break;
                case 5: default: show = true; break;
            }
            int target = show ? (isme ? CON_SELF : CON_INFO) : -1;
            if(showobitdists) announce(anc, target, d, "\fs\fw%s\fS (@\fs\fc%.2f\fSm)", d->obit, actor->o.dist(d->o)/8.f);
            else announce(anc, target, d, "\fw%s", d->obit);
        }
        if(debrisscale > 0)
        {
            vec pos = vec(d->o).sub(vec(0, 0, d->height*0.5f));
            int debris = clamp(max(damage,5)/5, 1, 15), amt = int((rnd(debris)+debris+1)*debrisscale);
            loopi(amt) projs::create(pos, vec(pos).add(d->vel), true, d, !isaitype(d->aitype) || aistyle[d->aitype].maxspeed ? PRJ_GIBS : PRJ_DEBRIS, rnd(debrisfade)+debrisfade, 0, rnd(500)+1, rnd(50)+10);
        }
        if(m_team(gamemode, mutators) && d->team == actor->team && d != actor && actor == player1)
        {
            hud::teamkills.add(lastmillis);
            if(hud::numteamkills() >= hud::teamkillnum) hud::lastteam = lastmillis;
        }
        ai::killed(d, actor);
    }

    void timeupdate(int timeremain)
    {
        timeremaining = timeremain;
        if(!timeremain && !intermission)
        {
            player1->stopmoving(true);
            hud::sb.showscores(true, true);
            intermission = true;
            smartmusic(true, false);
        }
    }

    gameent *newclient(int cn)
    {
        if(cn < 0 || cn >= MAXPLAYERS)
        {
            defformatstring(cnmsg)("clientnum [%d]", cn);
            neterr(cnmsg);
            return NULL;
        }

        if(cn == player1->clientnum) return player1;

        while(cn >= players.length()) players.add(NULL);

        if(!players[cn])
        {
            gameent *d = new gameent();
            d->clientnum = cn;
            players[cn] = d;
        }

        return players[cn];
    }

    gameent *getclient(int cn)
    {
        if(cn == player1->clientnum) return player1;
        if(players.inrange(cn)) return players[cn];
        return NULL;
    }

    void clientdisconnected(int cn, int reason)
    {
        if(!players.inrange(cn)) return;
        gameent *d = players[cn];
        if(!d) return;
        if(d->name[0] && showplayerinfo && (d->aitype < 0 || ai::showaiinfo))
            conoutft(showplayerinfo > 1 ? int(CON_EVENT) : int(CON_MESG), "\fo%s left the game (%s)", colorname(d), reason >= 0 ? disc_reasons[reason] : "normal");
        gameent *e = NULL;
        loopi(numdynents()) if((e = (gameent *)iterdynents(i)))
        {
            e->dominating.removeobj(d);
            e->dominated.removeobj(d);
            if(d == e && follow >= i)
            {
                followswitch(-1);
                focus = (gameent *)iterdynents(follow);
                resetcamera();
            }
        }
        cameras.shrink(0);
        client::clearvotes(d);
        projs::remove(d);
        removedamagetones(d);
        if(m_ctf(gamemode)) ctf::removeplayer(d);
        if(m_stf(gamemode)) stf::removeplayer(d);
        DELETEP(players[cn]);
        players[cn] = NULL;
        cleardynentcache();
    }

    void preload()
    {
        maskpackagedirs(~PACKAGEDIR_OCTA);
        int n = m_fight(gamemode) && m_team(gamemode, mutators) ? numteams(gamemode, mutators)+1 : 1;
        loopi(n)
        {
            loadmodel(teamtype[i].tpmdl, -1, true);
            loadmodel(teamtype[i].fpmdl, -1, true);
        }
        weapons::preload();
        projs::preload();
        entities::preload();
        if(m_edit(gamemode) || m_stf(gamemode)) stf::preload();
        if(m_edit(gamemode) || m_ctf(gamemode)) ctf::preload();
        maskpackagedirs(~0);
    }

    void resetmap(bool empty) // called just before a map load
    {
        if(!empty) smartmusic(true, false);
    }

    void startmap(const char *name, const char *reqname, bool empty)    // called just after a map load
    {
        intermission = false;
        maptime = hud::lastnewgame = 0;
        projs::reset();
        resetworld();
        if(*name)
        {
            conoutft(CON_MESG, "\fs\fw%s by %s [\fa%s\fS]", *maptitle ? maptitle : "Untitled", *mapauthor ? mapauthor : "Unknown", server::gamename(gamemode, mutators));
            preload();
        }
        // reset perma-state
        gameent *d;
        loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && (d->type == ENT_PLAYER || d->type == ENT_AI))
            d->mapchange(lastmillis, m_health(gamemode, mutators));
        entities::spawnplayer(player1, -1, false); // prevent the player from being in the middle of nowhere
        resetcamera();
        if(!empty) client::sendinfo = client::sendcrc = true;
        fogdist = max(float(getvar("fog")), ai::SIGHTMIN);
        copystring(clientmap, reqname ? reqname : (name ? name : ""));
        resetsway();
    }

    gameent *intersectclosest(vec &from, vec &to, gameent *at)
    {
        gameent *best = NULL, *o;
        float bestdist = 1e16f;
        loopi(numdynents()) if((o = (gameent *)iterdynents(i)))
        {
            if(!o || o==at || o->state!=CS_ALIVE || !physics::issolid(o, at)) continue;
            float dist;
            if(intersect(o, from, to, dist) && dist < bestdist)
            {
                best = o;
                bestdist = dist;
            }
        }
        return best;
    }

    int numdynents() { return 1+players.length(); }
    dynent *iterdynents(int i)
    {
        if(!i) return player1;
        i--;
        if(i<players.length()) return players[i];
        i -= players.length();
        return NULL;
    }
    dynent *focusedent(bool force)
    {
        if(force) return player1;
        return focus;
    }

    bool duplicatename(gameent *d, char *name = NULL)
    {
        if(!name) name = d->name;
        if(d!=player1 && !strcmp(name, player1->name)) return true;
        loopv(players) if(players[i] && d!=players[i] && !strcmp(name, players[i]->name)) return true;
        return false;
    }

    char *colorname(gameent *d, char *name, const char *prefix, bool team, bool dupname)
    {
        if(!name) name = d->name;
        static string cname;
        formatstring(cname)("%s\fs%s%s", *prefix ? prefix : "", teamtype[d->team].chat, name);
        if(!name[0] || d->aitype >= 0 || (dupname && duplicatename(d, name)))
        {
            defformatstring(s)(" [\fs%s%d\fS]", d->aitype >= 0 ? "\fc" : "\fw", d->clientnum);
            concatstring(cname, s);
        }
        concatstring(cname, "\fS");
        return cname;
    }

    void suicide(gameent *d, int flags)
    {
        if((d == player1 || d->ai) && d->state == CS_ALIVE && d->suicided < 0)
        {
            fireburn(d, -1, flags);
            client::addmsg(N_SUICIDE, "ri2", d->clientnum, flags);
            d->suicided = lastmillis;
        }
    }
    ICOMMAND(0, kill, "",  (), { suicide(player1, 0); });

    void lighteffects(dynent *e, vec &color, vec &dir) { }

    void particletrack(particle *p, uint type, int &ts,  bool lastpass)
    {
        if(!p || !p->owner || (p->owner->type != ENT_PLAYER && p->owner->type != ENT_AI)) return;
        gameent *d = (gameent *)p->owner;
        switch(type&0xFF)
        {
            case PT_TEXT: case PT_ICON:
            {
                vec q = p->owner->abovehead(p->m.z+4);
                float k = pow(aboveheadsmooth, float(curtime)/float(aboveheadsmoothmillis));
                p->o.mul(k).add(q.mul(1-k));
                break;
            }
            case PT_TAPE: case PT_LIGHTNING:
            {
                float dist = p->o.dist(p->d);
                p->d = p->o = d->muzzlepos(d->weapselect);
                vec dir; vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
                p->d.add(dir.mul(dist));
                break;
            }
            case PT_PART: case PT_FIREBALL: case PT_FLARE:
            {
                p->o = d->muzzlepos(d->weapselect);
                break;
            }
            default: break;
        }
    }

    void dynlighttrack(physent *owner, vec &o) { }

    void newmap(int size) { client::addmsg(N_NEWMAP, "ri", size); }

    void loadworld(stream *f, int maptype) { }
    void saveworld(stream *f) { }

    void fixfullrange(float &yaw, float &pitch, float &roll, bool full)
    {
        if(full)
        {
            while(pitch < -180.0f) pitch += 360.0f;
            while(pitch >= 180.0f) pitch -= 360.0f;
            while(roll < -180.0f) roll += 360.0f;
            while(roll >= 180.0f) roll -= 360.0f;
        }
        else
        {
            if(pitch > 89.9f) pitch = 89.9f;
            if(pitch < -89.9f) pitch = -89.9f;
            if(roll > 89.9f) roll = 89.9f;
            if(roll < -89.9f) roll = -89.9f;
        }
        while(yaw < 0.0f) yaw += 360.0f;
        while(yaw >= 360.0f) yaw -= 360.0f;
    }

    void fixrange(float &yaw, float &pitch)
    {
        float r = 0.f;
        fixfullrange(yaw, pitch, r, false);
    }

    void fixview(int w, int h)
    {
        if(inzoom())
        {
            int frame = lastmillis-lastzoom, f = zoomfov, t = zoomtime;
            checkzoom();
            if(zoomlevels > 1 && zoomlevel < zoomlevels) f = fov()-(((fov()-zoomfov)/zoomlevels)*zoomlevel);
            float diff = float(fov()-f), amt = frame < t ? clamp(float(frame)/float(t), 0.f, 1.f) : 1.f;
            if(!zooming) amt = 1.f-amt;
            curfov = fov()-(amt*diff);
        }
        else curfov = float(fov());
    }

    bool mousemove(int dx, int dy, int x, int y, int w, int h)
    {
        bool hasinput = hud::hasinput(true);
        #define mousesens(a,b,c) ((float(a)/float(b))*c)
        if(hasinput || (mousestyle() >= 1 && player1->state != CS_WAITING && player1->state != CS_SPECTATOR))
        {
            if(mouseabsolute) // absolute positions, unaccelerated
            {
                cursorx = clamp(float(x)/float(w), 0.f, 1.f);
                cursory = clamp(float(y)/float(h), 0.f, 1.f);
                return false;
            }
            else
            {
                cursorx = clamp(cursorx+mousesens(dx, w, mousesensitivity), 0.f, 1.f);
                cursory = clamp(cursory+mousesens(dy, h, mousesensitivity*(!hasinput && mouseinvert ? -1.f : 1.f)), 0.f, 1.f);
                return true;
            }
        }
        else if(!tvmode())
        {
            physent *target = player1->state == CS_WAITING || player1->state == CS_SPECTATOR ? camera1 : (allowmove(player1) ? player1 : NULL);
            if(target)
            {
                float scale = (inzoom() && zoomsensitivity > 0 && zoomsensitivity < 1 ? 1.f-(zoomlevel/float(zoomlevels+1)*zoomsensitivity) : 1.f)*sensitivity;
                target->yaw += mousesens(dx, w, yawsensitivity*scale);
                target->pitch -= mousesens(dy, h, pitchsensitivity*scale*(!hasinput && mouseinvert ? -1.f : 1.f));
                fixfullrange(target->yaw, target->pitch, target->roll, false);
            }
            return true;
        }
        return false;
    }

    void project(int w, int h)
    {
        int style = hud::hasinput() ? -1 : mousestyle();
        if(style != lastmousetype)
        {
            resetcursor();
            lastmousetype = style;
        }
        if(style >= 0) vecfromcursor(cursorx, cursory, 1.f, cursordir);
    }

    void getyawpitch(const vec &from, const vec &pos, float &yaw, float &pitch)
    {
        float dist = from.dist(pos);
        yaw = -(float)atan2(pos.x-from.x, pos.y-from.y)/PI*180+180;
        pitch = asin((pos.z-from.z)/dist)/RAD;
    }

    void scaleyawpitch(float &yaw, float &pitch, float targyaw, float targpitch, float frame, float scale)
    {
        if(yaw < targyaw-180.0f) yaw += 360.0f;
        if(yaw > targyaw+180.0f) yaw -= 360.0f;
        float offyaw = fabs(targyaw-yaw)*frame, offpitch = fabs(targpitch-pitch)*frame*scale;
        if(targyaw > yaw)
        {
            yaw += offyaw;
            if(targyaw < yaw) yaw = targyaw;
        }
        else if(targyaw < yaw)
        {
            yaw -= offyaw;
            if(targyaw > yaw) yaw = targyaw;
        }
        if(targpitch > pitch)
        {
            pitch += offpitch;
            if(targpitch < pitch) pitch = targpitch;
        }
        else if(targpitch < pitch)
        {
            pitch -= offpitch;
            if(targpitch > pitch) pitch = targpitch;
        }
        fixrange(yaw, pitch);
    }

    void deathcamyawpitch(gameent *d, float &yaw, float &pitch)
    {
        if(deathcamstyle)
        {
            gameent *a = deathcamstyle > 1 ? d : getclient(d->lastattacker);
            if(a)
            {
                vec dir = vec(a->headpos(-a->height*0.5f)).sub(camera1->o).normalize();
                vectoyawpitch(dir, camera1->aimyaw, camera1->aimpitch);
                if(deathcamspeed > 0) scaleyawpitch(yaw, pitch, camera1->aimyaw, camera1->aimpitch, (float(curtime)/1000.f)*deathcamspeed, 4.f);
                else { yaw = camera1->aimyaw; pitch = camera1->aimpitch; }
            }
        }
    }

    void cameraplayer()
    {
        if(player1->state != CS_WAITING && player1->state != CS_SPECTATOR && player1->state != CS_DEAD && !tvmode())
        {
            player1->aimyaw = camera1->yaw;
            player1->aimpitch = camera1->pitch;
            fixrange(player1->aimyaw, player1->aimpitch);
            if(lastcamera && mousestyle() >= 1 && !hud::hasinput())
            {
                physent *d = mousestyle() != 2 ? player1 : camera1;
                float amt = clamp(float(lastmillis-lastcamera)/100.f, 0.f, 1.f)*panspeed();
                float zone = float(deadzone())/200.f, cx = cursorx-0.5f, cy = 0.5f-cursory;
                if(cx > zone || cx < -zone) d->yaw += ((cx > zone ? cx-zone : cx+zone)/(1.f-zone))*amt;
                if(cy > zone || cy < -zone) d->pitch += ((cy > zone ? cy-zone : cy+zone)/(1.f-zone))*amt;
                fixfullrange(d->yaw, d->pitch, d->roll, false);
            }
        }
    }

    void cameratv()
    {
        bool isspec = player1->state == CS_SPECTATOR;
        if(cameras.empty())
        {
            loopk(2)
            {
                physent d = *player1;
                d.radius = d.height = 4.f;
                d.state = CS_ALIVE;
                loopv(entities::ents) if(entities::ents[i]->type == CAMERA || (k && !enttype[entities::ents[i]->type].noisy))
                {
                    gameentity &e = *(gameentity *)entities::ents[i];
                    vec pos(e.o);
                    if(e.type == MAPMODEL)
                    {
                        mapmodelinfo &mmi = getmminfo(e.attrs[0]);
                        vec center, radius;
                        mmi.m->collisionbox(0, center, radius);
                        if(e.attrs[4]) { center.mul(e.attrs[4]/100.f); radius.mul(e.attrs[4]/100.f); }
                        if(!mmi.m->ellipsecollide) rotatebb(center, radius, int(e.attrs[1]));
                        pos.z += ((center.z-radius.z)+radius.z*2*mmi.m->height)*3.f;
                    }
                    else if(enttype[e.type].radius) pos.z += enttype[e.type].radius;
                    d.o = pos;
                    if(physics::entinmap(&d, false))
                    {
                        camstate &c = cameras.add();
                        c.pos = d.o; c.ent = i;
                        if(!k)
                        {
                            c.idx = e.attrs[0];
                            if(e.attrs[1]) c.mindist = e.attrs[1];
                            if(e.attrs[2]) c.maxdist = e.attrs[2];
                        }
                    }
                }
                if(!cameras.empty()) break;
            }
            gameent *d = NULL;
            loopi(numdynents()) if((d = (gameent *)iterdynents(i)) != NULL && (d->type == ENT_PLAYER || d->type == ENT_AI))
            {
                camstate &c = cameras.add();
                c.pos = d->headpos();
                c.ent = -1; c.idx = i;
                if(d->state == CS_DEAD || d->state == CS_WAITING) deathcamyawpitch(d, d->yaw, d->pitch);
                vecfromyawpitch(d->yaw, d->pitch, 1, 0, c.dir);
            }
        }
        else loopv(cameras) if(cameras[i].ent < 0 && cameras[i].idx >= 0 && cameras[i].idx < numdynents())
        {
            gameent *d = (gameent *)iterdynents(cameras[i].idx);
            if(!d) { cameras.remove(i--); continue; }
            if(d->state != CS_DEAD && d->state != CS_WAITING) cameras[i].pos = d->headpos();
            else deathcamyawpitch(d, d->yaw, d->pitch);
            vecfromyawpitch(d->yaw, d->pitch, 1, 0, cameras[i].dir);
        }

        if(!cameras.empty())
        {
            camstate *cam = &cameras[0];
            int ent = cam->ent, entidx = cam->ent >= 0 ? cam->ent : cam->idx;
            bool renew = !lasttvcam || lastmillis-lasttvcam >= spectvtime, override = !lasttvcam || lastmillis-lasttvcam >= max(spectvtime/2, 2500);
            loopk(3)
            {
                int found = 0;
                loopvj(cameras)
                {
                    camstate &c = cameras[j]; c.reset();
                    gameent *d, *t = c.ent < 0 && c.idx >= 0 ? (gameent *)iterdynents(c.idx) : NULL;
                    loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d != t && d->aitype < AI_START && (d->state == CS_ALIVE || d->state == CS_DEAD))
                    {
                        vec trg, pos = d->feetpos();
                        float dist = c.pos.dist(d->feetpos());
                        if(dist >= c.mindist && dist <= min(c.maxdist, float(fogdist)) && (raycubelos(c.pos, pos, trg) || raycubelos(c.pos, pos = d->headpos(), trg)))
                        {
                            float yaw = t ? t->yaw : camera1->yaw, pitch = t ? t->pitch : camera1->pitch;
                            if(!t && (k || renew))
                            {
                                vec dir = pos;
                                if(c.cansee.length()) dir.add(vec(c.dir).div(c.cansee.length()));
                                dir.sub(c.pos).normalize();
                                vectoyawpitch(dir, yaw, pitch);
                            }
                            if(k || renew || getsight(c.pos, yaw, pitch, pos, trg, min(c.maxdist, float(fogdist)), curfov, fovy) || getsight(c.pos, yaw, pitch, pos = d->headpos(), trg, min(c.maxdist, float(fogdist)), curfov, fovy))
                            {
                                c.cansee.add(i);
                                c.dir.add(pos);
                                c.score += dist;
                            }
                        }
                    }
                    if(c.cansee.length() > (override && !t ? 1 : 0))
                    {
                        if(c.cansee.length() > 1)
                        {
                            float amt = float(c.cansee.length());
                            c.dir.div(amt);
                            c.score /= amt;
                        }
                        found++;
                    }
                    else { c.dir = c.pos; c.score = t ? 1e14f : 1e16f; }
                    if(t && t->state == CS_ALIVE && spectvbias > 0) c.score /= spectvbias;
                    if(k <= 1) break;
                }
                if(!found)
                {
                    if(k == 1)
                    {
                        if(override) renew = true;
                        else break;
                    }
                }
                else break;
            }
            if(renew)
            {
                cameras.sort(camstate::camsort);
                cam = &cameras[0];
                lasttvcam = lastmillis;
                if(!lasttvchg || (cam->ent >= 0 ? (ent >= 0 && cam->ent != entidx) : (ent < 0 && cam->idx != entidx)))
                {
                    lasttvchg = lastmillis;
                    ent = entidx = -1;
                }
                if(cam->ent < 0 && cam->idx >= 0)
                {
                    if((focus = (gameent *)iterdynents(cam->idx)) != NULL) follow = cam->idx;
                    else { focus = player1; follow = 0; }
                }
                else { focus = player1; follow = 0; }
            }
            if(cam->ent < 0 && focus != player1)
            {
                camera1->o = focus->headpos();
                camera1->yaw = camera1->aimyaw = focus->yaw;
                camera1->pitch = camera1->aimpitch = focus->pitch;
                camera1->roll = focus->roll;
            }
            else
            {
                camera1->o = cam->pos;
                vectoyawpitch(vec(cam->dir).sub(camera1->o).normalize(), camera1->aimyaw, camera1->aimpitch);
                if(spectvspeed > 0) scaleyawpitch(camera1->yaw, camera1->pitch, camera1->aimyaw, camera1->aimpitch, (float(curtime)/1000.f)*spectvspeed, spectvpitch);
                else { camera1->yaw = camera1->aimyaw; camera1->pitch = camera1->aimpitch; }
            }
            camera1->resetinterp();
        }
        else setvar(isspec ? "specmode" : "waitmode", 0, true);
    }

    void checkcamera()
    {
        camera1 = &camera;
        if(camera1->type != ENT_CAMERA)
        {
            camera1->reset();
            camera1->type = ENT_CAMERA;
            camera1->collidetype = COLLIDE_AABB;
            camera1->state = CS_ALIVE;
            camera1->height = camera1->zradius = camera1->radius = camera1->xradius = camera1->yradius = 2;
        }
        if((focus->state != CS_WAITING && focus->state != CS_SPECTATOR) || tvmode())
        {
            camera1->vel = vec(0, 0, 0);
            camera1->move = camera1->strafe = 0;
        }
        if(tvmode()) cameratv();
        else if(focus->state == CS_DEAD)
        {
            deathcamyawpitch(focus, camera1->yaw, camera1->pitch);
            camera1->aimyaw = camera1->yaw;
            camera1->aimpitch = camera1->pitch;
        }
        else if(focus->state == CS_WAITING || focus->state == CS_SPECTATOR)
        {
            camera1->move = player1->move;
            camera1->strafe = player1->strafe;
            physics::move(camera1, 10, true);
        }
        if(focus->state == CS_SPECTATOR)
        {
            player1->aimyaw = player1->yaw = camera1->yaw;
            player1->aimpitch = player1->pitch = camera1->pitch;
            player1->o = camera1->o;
            player1->resetinterp();
        }
    }

    void resetcamera()
    {
        lastcamera = 0;
        zoomset(false, 0);
        if(focus == game::player1) resetcursor();
        checkcamera();
        camera1->o = focus->o;
        camera1->yaw = focus->yaw;
        camera1->pitch = focus->pitch;
        camera1->roll = focus->calcroll(false);
        camera1->resetinterp();
        focus->resetinterp();
    }

    void resetworld()
    {
        follow = 0; focus = player1;
        hud::sb.showscores(false);
        cleargui();
    }

    void resetstate()
    {
        resetworld();
        resetcamera();
    }

    void updateworld()      // main game update loop
    {
        if(connected())
        {
            if(!maptime) { maptime = -1; return; } // skip the first loop
            else if(maptime < 0)
            {
                maptime = lastmillis;
                //if(m_lobby(gamemode)) smartmusic(true, false);
                //else
                if(*mapmusic && (!music || !Mix_PlayingMusic() || strcmp(mapmusic, musicfile))) playmusic(mapmusic, "");
                else musicdone(false);
                RUNWORLD("on_start");
                return;
            }
        }
        if(!curtime) { gets2c(); if(player1->clientnum >= 0) client::c2sinfo(); return; }

        if(!*player1->name && !menuactive()) showgui("name", -1);
        if(connected())
        {
            player1->conopen = commandmillis > 0 || hud::hasinput(true);
            // do shooting/projectile update here before network update for greater accuracy with what the player sees
            if(allowmove(player1)) cameraplayer();
            else player1->stopmoving(player1->state != CS_WAITING && player1->state != CS_SPECTATOR);

            gameent *d = NULL; bool allow = player1->state == CS_SPECTATOR || player1->state == CS_WAITING, found = false;
            loopi(numdynents()) if((d = (gameent *)iterdynents(i)) != NULL)
            {
                if(d->state != CS_SPECTATOR && allow && i == follow)
                {
                    if(focus != d)
                    {
                        focus = d;
                        resetcamera();
                    }
                    follow = i;
                    found = true;
                }
                if(d->type == ENT_PLAYER || d->type == ENT_AI)
                {
                    checkoften(d, d == player1 || d->ai);
                    if(d == player1)
                    {
                        int state = d->weapstate[d->weapselect];
                        if(WEAP(d->weapselect, zooms))
                        {
                            if(state == WEAP_S_SHOOT || (state == WEAP_S_RELOAD && lastmillis-d->weaplast[d->weapselect] >= max(d->weapwait[d->weapselect]-zoomtime, 1)))
                                state = WEAP_S_IDLE;
                        }
                        if(zooming && (!WEAP(d->weapselect, zooms) || state != WEAP_S_IDLE)) zoomset(false, lastmillis);
                        else if(WEAP(d->weapselect, zooms) && state == WEAP_S_IDLE && zooming != d->action[AC_ALTERNATE])
                            zoomset(d->action[AC_ALTERNATE], lastmillis);
                    }
                }
            }
            if(!found || !allow)
            {
                if(focus != player1)
                {
                    focus = player1;
                    resetcamera();
                }
                follow = 0;
            }

            physics::update();
            projs::update();
            ai::update();
            if(!intermission)
            {
                entities::update();
                if(player1->state == CS_ALIVE) weapons::shoot(player1, worldpos);
            }
            otherplayers();
            if(m_arena(gamemode, mutators) && player1->state != CS_SPECTATOR && player1->loadweap < 0 && client::ready() && !menuactive())
                showgui("loadout", -1);
        }
        else if(!menuactive()) showgui("main", -1);

        gets2c();
        adjustscaled(int, hud::damageresidue, hud::damageresiduefade);
        if(connected())
        {
            flushdamagetones();
            if(player1->state == CS_DEAD || player1->state == CS_WAITING)
            {
                if(player1->ragdoll) moveragdoll(player1, true);
                else if(lastmillis-player1->lastpain <= 2000)
                    physics::move(player1, 10, false);
            }
            else
            {
                if(player1->ragdoll) cleanragdoll(player1);
                if(player1->state == CS_EDITING) physics::move(player1, 10, true);
                else if(!intermission && player1->state == CS_ALIVE)
                {
                    physics::move(player1, 10, true);
                    entities::checkitems(player1);
                    weapons::reload(player1);
                }
                if(!intermission) addsway(focus);
            }
            checkcamera();
            if(hud::sb.canshowscores()) hud::sb.showscores(true);
        }

        if(player1->clientnum >= 0) client::c2sinfo();
    }

    void recomputecamera(int w, int h)
    {
        fixview(w, h);
        checkcamera();
        if(client::ready())
        {
            if(!lastcamera)
            {
                resetcursor();
                cameras.shrink(0);
                if(mousestyle() == 2 && focus->state != CS_WAITING && focus->state != CS_SPECTATOR)
                {
                    camera1->yaw = focus->aimyaw = focus->yaw;
                    camera1->pitch = focus->aimpitch = focus->pitch;
                }
            }

            if(focus->state == CS_DEAD || focus->state == CS_WAITING || focus->state == CS_SPECTATOR)
            {
                camera1->aimyaw = camera1->yaw;
                camera1->aimpitch = camera1->pitch;
            }
            else
            {
                camera1->o = focus->headpos();
                if(mousestyle() <= 1)
                    findorientation(camera1->o, focus->yaw, focus->pitch, worldpos);

                camera1->aimyaw = mousestyle() <= 1 ? focus->yaw : focus->aimyaw;
                camera1->aimpitch = mousestyle() <= 1 ? focus->pitch : focus->aimpitch;
                if(thirdpersonview(true) && thirdpersondist)
                {
                    vec dir;
                    vecfromyawpitch(camera1->aimyaw, camera1->aimpitch, thirdpersondist > 0 ? -1 : 1, 0, dir);
                    physics::movecamera(camera1, dir, fabs(thirdpersondist), 1.0f);
                }
                camera1->resetinterp();

                switch(mousestyle())
                {
                    case 0:
                    case 1:
                    {
                        camera1->yaw = focus->yaw;
                        camera1->pitch = focus->pitch;
                        if(mousestyle())
                        {
                            camera1->aimyaw = camera1->yaw;
                            camera1->aimpitch = camera1->pitch;
                        }
                        break;
                    }
                    case 2:
                    {
                        float yaw, pitch;
                        vectoyawpitch(cursordir, yaw, pitch);
                        fixrange(yaw, pitch);
                        findorientation(camera1->o, yaw, pitch, worldpos);
                        if(focus == player1 && allowmove(player1))
                        {
                            player1->yaw = yaw;
                            player1->pitch = pitch;
                        }
                        break;
                    }
                }
                fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
                fixrange(camera1->aimyaw, camera1->aimpitch);
            }
            camera1->roll = focus->calcroll(physics::iscrouching(focus));
            vecfromyawpitch(camera1->yaw, camera1->pitch, 1, 0, camdir);
            vecfromyawpitch(camera1->yaw, 0, 0, -1, camright);
            vecfromyawpitch(camera1->yaw, camera1->pitch+90, 1, 0, camup);

            camera1->inmaterial = lookupmaterial(camera1->o);
            camera1->inliquid = isliquid(camera1->inmaterial&MATF_VOLUME);

            switch(camera1->inmaterial)
            {
                case MAT_WATER:
                {
                    if(!issound(liquidchan))
                        playsound(S_UNDERWATER, camera1->o, camera1, SND_LOOP|SND_NOATTEN|SND_NODELAY|SND_NOCULL, -1, -1, -1, &liquidchan);
                    break;
                }
                default:
                {
                    if(issound(liquidchan)) removesound(liquidchan);
                    liquidchan = -1;
                    break;
                }
            }

            lastcamera = lastmillis;
        }
    }

    VAR(0, animoverride, -1, 0, ANIM_MAX-1);
    VAR(0, testanims, 0, 0, 1);

    int numanims() { return ANIM_MAX; }

    void findanims(const char *pattern, vector<int> &anims)
    {
        loopi(sizeof(animnames)/sizeof(animnames[0]))
            if(*animnames[i] && matchanim(animnames[i], pattern))
                anims.add(i);
    }

    void renderclient(gameent *d, bool third, float trans, float size, int team, modelattach *attachments, bool secondary, int animflags, int animdelay, int lastaction, bool early)
    {
        const char *mdl = "";
        if(d->aitype < AI_START)
        {
            if(third) mdl = teamtype[team].tpmdl;
            else mdl = teamtype[team].fpmdl;
        }
        else if(d->aitype < AI_MAX) mdl = aistyle[d->aitype].mdl;
        else return;

        float yaw = d->yaw, pitch = d->pitch, roll = d->calcroll(physics::iscrouching(d));
        vec o = vec(third ? d->feetpos() : d->headpos());
        if(!third)
        {
            vec dir;
            if(firstpersonsway && !intermission)
            {
                vecfromyawpitch(d->yaw, 0, 0, 1, dir);
                float steps = swaydist/firstpersonswaystep*M_PI;
                dir.mul(firstpersonswayside*cosf(steps));
                dir.z = firstpersonswayup*(fabs(sinf(steps)) - 1);
                o.add(dir).add(swaydir).add(swaypush);
            }
            if(firstpersondist != 0.f)
            {
                vecfromyawpitch(yaw, pitch, 1, 0, dir);
                dir.mul(focus->radius*firstpersondist);
                o.add(dir);
            }
            if(firstpersonshift != 0.f)
            {
                vecfromyawpitch(yaw, pitch, 0, -1, dir);
                dir.mul(focus->radius*firstpersonshift);
                o.add(dir);
            }
            if(firstpersonadjust != 0.f)
            {
                vecfromyawpitch(yaw, pitch+90.f, 1, 0, dir);
                dir.mul(focus->height*firstpersonadjust);
                o.add(dir);
            }
        }

        int anim = animflags, basetime = lastaction, basetime2 = 0;
        if(animoverride)
        {
            anim = (animoverride<0 ? ANIM_ALL : animoverride)|ANIM_LOOP;
            basetime = 0;
        }
        else
        {
            if(secondary && (d->aitype < AI_START || aistyle[d->aitype].maxspeed))
            {
                if(physics::liquidcheck(d) && d->physstate <= PHYS_FALL)
                    anim |= (((allowmove(d) && (d->move || d->strafe)) || d->vel.z+d->falling.z>0 ? int(ANIM_SWIM) : int(ANIM_SINK))|ANIM_LOOP)<<ANIM_SECONDARY;
                else if(d->physstate == PHYS_FALL && !d->onladder && impulsestyle && d->impulse[IM_TYPE] != IM_T_NONE && lastmillis-d->impulse[IM_TIME] <= 1000) { anim |= ANIM_IMPULSE_DASH<<ANIM_SECONDARY; basetime2 = d->impulse[IM_TIME]; }
                else if(d->physstate == PHYS_FALL && !d->onladder && d->actiontime[AC_JUMP] && lastmillis-d->actiontime[AC_JUMP] <= 1000) { anim |= ANIM_JUMP<<ANIM_SECONDARY; basetime2 = d->actiontime[AC_JUMP]; }
                else if(d->physstate == PHYS_FALL && !d->onladder && d->timeinair >= 1000) anim |= (ANIM_JUMP|ANIM_END)<<ANIM_SECONDARY;
                else if(physics::sprinting(d))
                {
                    if(d->move>0)       anim |= (ANIM_IMPULSE_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
                    else if(d->strafe)  anim |= ((d->strafe>0 ? ANIM_IMPULSE_LEFT : ANIM_IMPULSE_RIGHT)|ANIM_LOOP)<<ANIM_SECONDARY;
                    else if(d->move<0)  anim |= (ANIM_IMPULSE_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;
                }
                else if(d->action[AC_CROUCH] || d->actiontime[AC_CROUCH]<0)
                {
                    if(d->move>0)       anim |= (ANIM_CRAWL_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
                    else if(d->strafe)  anim |= ((d->strafe>0 ? ANIM_CRAWL_LEFT : ANIM_CRAWL_RIGHT)|ANIM_LOOP)<<ANIM_SECONDARY;
                    else if(d->move<0)  anim |= (ANIM_CRAWL_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;
                    else                anim |= (ANIM_CROUCH|ANIM_LOOP)<<ANIM_SECONDARY;
                }
                else if(d->move>0) anim |= (ANIM_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
                else if(d->strafe) anim |= ((d->strafe>0 ? ANIM_LEFT : ANIM_RIGHT)|ANIM_LOOP)<<ANIM_SECONDARY;
                else if(d->move<0) anim |= (ANIM_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;
            }

            if((anim>>ANIM_SECONDARY)&ANIM_INDEX) switch(anim&ANIM_INDEX)
            {
                case ANIM_IDLE: case ANIM_MELEE: case ANIM_PISTOL: case ANIM_SHOTGUN: case ANIM_SMG:
                case ANIM_GRENADE: case ANIM_ROCKET: case ANIM_FLAMER: case ANIM_PLASMA: case ANIM_RIFLE:
                {
                    anim = (anim>>ANIM_SECONDARY) | ((anim&((1<<ANIM_SECONDARY)-1))<<ANIM_SECONDARY);
                    swap(basetime, basetime2);
                    break;
                }
                default: break;
            }
        }

        if(third && testanims && d == focus) yaw = 0; else yaw += 90;
        if(anim == ANIM_DYING) pitch *= max(1.f-(lastmillis-basetime)/500.f, 0.f);

        if(d->ragdoll && (!ragdolls || anim!=ANIM_DYING)) cleanragdoll(d);

        if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX)) anim |= (ANIM_IDLE|ANIM_LOOP)<<ANIM_SECONDARY;

        int flags = MDL_LIGHT;
        if(d != focus && !(anim&ANIM_RAGDOLL)) flags |= MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_CULL_QUERY;
        if(d->type == ENT_PLAYER)
        {
            if(!early && third) flags |= MDL_FULLBRIGHT;
        }
        else flags |= MDL_CULL_DIST;
        if(early) flags |= MDL_NORENDER;
        else if(third && (anim&ANIM_INDEX)!=ANIM_DEAD) flags |= MDL_DYNSHADOW;
        dynent *e = third ? (dynent *)d : (dynent *)&avatarmodel;
        rendermodel(NULL, mdl, anim, o, yaw, pitch, roll, flags, e, attachments, basetime, basetime2, trans, size);
    }

    void renderplayer(gameent *d, bool third, float trans, float size, bool early = false)
    {
        if(d->state == CS_SPECTATOR) return;
        if(trans <= 0.f || (d == focus && (third ? thirdpersonmodel : firstpersonmodel) < 1))
        {
            if(d->state == CS_ALIVE && rendernormally && (early || d != focus))
                trans = 1e-16f; // we need tag_muzzle/tag_waist
            else return; // screw it, don't render them
        }

        int team = m_fight(gamemode) && m_team(gamemode, mutators) ? d->team : TEAM_NEUTRAL, weap = d->weapselect, lastaction = 0, animflags = ANIM_IDLE|ANIM_LOOP, animdelay = 0;
        bool secondary = false, showweap = d->aitype < AI_START ? isweap(weap) : aistyle[d->aitype].useweap;

        if(d->state == CS_DEAD || d->state == CS_WAITING)
        {
            showweap = false;
            animflags = ANIM_DYING;
            lastaction = d->lastpain;
            if(ragdolls)
            {
                if(!validragdoll(d, lastaction)) animflags |= ANIM_RAGDOLL;
            }
            else
            {
                int t = lastmillis-lastaction;
                if(t < 0) return;
                if(t > 1000) animflags = ANIM_DEAD|ANIM_LOOP;
            }
        }
        else if(d->state == CS_EDITING)
        {
            animflags = ANIM_EDIT|ANIM_LOOP;
            showweap = false;
        }
#if 0
        else if(intermission)
        {
            lastaction = lastmillis;
            animflags = ANIM_LOSE|ANIM_LOOP;
            animdelay = 1000;
            if(m_fight(gamemode) && m_team(gamemode, mutators))
            {
                loopv(bestteams) if(bestteams[i] == d->team)
                {
                    animflags = ANIM_WIN|ANIM_LOOP;
                    break;
                }
            }
            else if(bestplayers.find(d) >= 0) animflags = ANIM_WIN|ANIM_LOOP;
        }
#endif
        else if(third && lastmillis-d->lastpain <= 300)
        {
            secondary = third;
            lastaction = d->lastpain;
            animflags = ANIM_PAIN;
            animdelay = 300;
        }
        else
        {
            secondary = third;
            if(showweap)
            {
                lastaction = d->weaplast[weap];
                animdelay = d->weapwait[weap];
                switch(d->weapstate[weap])
                {
                    case WEAP_S_SWITCH:
                    case WEAP_S_PICKUP:
                    {
                        if(lastmillis-d->weaplast[weap] <= d->weapwait[weap]/3)
                        {
                            if(!d->hasweap(d->lastweap, m_weapon(gamemode, mutators))) showweap = false;
                            else weap = d->lastweap;
                        }
                        else if(!d->hasweap(weap, m_weapon(gamemode, mutators))) showweap = false;
                        animflags = ANIM_SWITCH+(d->weapstate[weap]-WEAP_S_SWITCH);
                        break;
                    }
                    case WEAP_S_POWER:
                    {
                        if(weap == WEAP_GRENADE) animflags = weaptype[weap].anim+d->weapstate[weap];
                        else animflags = weaptype[weap].anim|ANIM_LOOP;
                        break;
                    }
                    case WEAP_S_SHOOT:
                    {
                        if(weaptype[weap].thrown[0] > 0 && (lastmillis-d->weaplast[weap] <= d->weapwait[weap]/2 || !d->hasweap(weap, m_weapon(gamemode, mutators))))
                            showweap = false;
                        animflags = weaptype[weap].anim+d->weapstate[weap];
                        break;
                    }
                    case WEAP_S_RELOAD:
                    {
                        if(weap != WEAP_MELEE)
                        {
                            if(!d->hasweap(weap, m_weapon(gamemode, mutators)) || (!w_reload(weap, m_weapon(gamemode, mutators)) && lastmillis-d->weaplast[weap] <= d->weapwait[weap]/3))
                                showweap = false;
                            animflags = weaptype[weap].anim+d->weapstate[weap];
                            break;
                        }
                    }
                    case WEAP_S_IDLE: case WEAP_S_WAIT: default:
                    {
                        if(!d->hasweap(weap, m_weapon(gamemode, mutators))) showweap = false;
                        animflags = weaptype[weap].anim|ANIM_LOOP;
                        break;
                    }
                }
            }
        }

        if(third && d->type == ENT_PLAYER && !shadowmapping && !envmapping && trans > 1e-16f && d->o.squaredist(camera1->o) <= maxparticledistance*maxparticledistance)
        {
            vec pos = d->abovehead(2);
            float blend = aboveheadblend*trans;
            if(shownamesabovehead > (d != focus ? 0 : 1))
            {
                const char *name = colorname(d, NULL, d->aitype < 0 ? "<super>" : "<default>");
                if(name && *name)
                {
                    part_textcopy(pos, name, PART_TEXT, 1, 0xFFFFFF, 2, blend);
                    pos.z += 2;
                }
            }
            if(showstatusabovehead > (d != focus ? 0 : 1))
            {
                Texture *t = NULL;
                if(d->state == CS_DEAD || d->state == CS_WAITING) t = textureload(hud::deadtex, 3);
                else if(d->state == CS_ALIVE)
                {
                    if(d->conopen) t = textureload(hud::conopentex, 3);
                    else if(m_team(gamemode, mutators) && showteamabovehead > (d != focus ? (d->team != focus->team ? 1 : 0) : 2))
                        t = textureload(hud::teamtex(d->team), 3);
                    else if(d->dominating.find(game::focus) >= 0) t = textureload(hud::dominatingtex, 3);
                    else if(d->dominated.find(game::focus) >= 0) t = textureload(hud::dominatedtex, 3);
                }
                if(t)
                {
                    part_icon(pos, t, 2, blend);
                    pos.z += 2;
                }
            }
        }

        bool hasweapon = showweap && *weaptype[weap].vwep;
        modelattach a[9]; int ai = 0;
        if(hasweapon) a[ai++] = modelattach("tag_weapon", weaptype[weap].vwep, ANIM_VWEP|ANIM_LOOP, 0); // we could probably animate this too now..
        if(rendernormally && (early || d != focus))
        {
            const char *muzzle = "tag_weapon";
            if(hasweapon)
            {
                muzzle = "tag_muzzle";
                if(weaptype[weap].eject) a[ai++] = modelattach("tag_eject", &d->eject);
            }
            a[ai++] = modelattach(muzzle, &d->muzzle);
            if(third && d->wantshitbox())
            {
                a[ai++] = modelattach("tag_head", &d->head);
                a[ai++] = modelattach("tag_torso", &d->torso);
                a[ai++] = modelattach("tag_waist", &d->waist);
                a[ai++] = modelattach("tag_lfoot", &d->lfoot);
                a[ai++] = modelattach("tag_rfoot", &d->rfoot);
            }
        }
        renderclient(d, third, trans, size, team, a[0].tag ? a : NULL, secondary, animflags, animdelay, lastaction, early);
    }

    void rendercheck(gameent *d)
    {
        d->checktags();
        impulseeffect(d, false);
        fireeffect(d);
    }

    void render()
    {
        startmodelbatches();
        gameent *d;
        loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d != focus) renderplayer(d, true, transscale(d, true), deadscale(d, 1, true));
        entities::render();
        projs::render();
        if(m_stf(gamemode)) stf::render();
        if(m_ctf(gamemode)) ctf::render();
        ai::render();
        if(rendernormally) loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d != focus) d->cleartags();
        endmodelbatches();
        if(rendernormally) loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d != focus) rendercheck(d);
    }

    void renderavatar(bool early)
    {
        if(rendernormally && early) focus->cleartags();
        if((thirdpersonview() || !rendernormally))
            renderplayer(focus, true, transscale(focus, thirdpersonview(true)), deadscale(focus, 1, true), early);
        else if(!thirdpersonview() && focus->state == CS_ALIVE)
            renderplayer(focus, false, transscale(focus, false), deadscale(focus, 1, true), early);
        if(rendernormally && early) rendercheck(focus);
    }

    bool clientoption(char *arg) { return false; }
}
#undef GAMEWORLD
