#define GAMEWORLD 1
#include "game.h"
namespace game
{
	int nextmode = -1, nextmuts = -1, gamemode = -1, mutators = -1, maptime = 0, minremain = 0,
		lastcamera = 0, lasttvcam = 0, lasttvchg = 0, lastzoom = 0, lastmousetype = 0, liquidchan = -1, fogdist = 0;
	bool intermission = false, prevzoom = false, zooming = false;
	float swayfade = 0, swayspeed = 0, swaydist = 0;
	vec swaydir(0, 0, 0), swaypush(0, 0, 0);
    string clientmap = "";

	gameent *player1 = new gameent(), *focus = player1;
	avatarent avatarmodel;
	vector<gameent *> players;
	vector<camstate> cameras;

	ICOMMANDG(resetvars, "", (), return); // server side

	VARW(numplayers, 0, 0, MAXCLIENTS);
	SVARW(obitwater, "");
	SVARW(obitdeath, "");
	SVARW(mapmusic, "");

	VARP(mouseinvert, 0, 0, 1);
	VARP(mouseabsolute, 0, 0, 1);
	VARP(mousetype, 0, 0, 2);
	VARP(mousedeadzone, 0, 10, 100);
	VARP(mousepanspeed, 1, 30, INT_MAX-1);

	VARP(thirdperson, 0, 0, 1);
	VARP(dynlighteffects, 0, 2, 2);
	FVARP(playerblend, 0, 1, 1);

	VARP(thirdpersonmodel, 0, 1, 1);
	VARP(thirdpersonfov, 90, 120, 150);
	FVARP(thirdpersonblend, 0, 0.5f, 1);
	FVARP(thirdpersondist, -100, 1.f, 100);

	VARP(firstpersonmodel, 0, 1, 1);
	VARP(firstpersonfov, 90, 100, 150);
	VARP(firstpersonsway, 0, 1, 1);
	FVARP(firstpersonswaystep, 1, 18.0f, 100);
	FVARP(firstpersonswayside, 0, 0.06f, 1);
	FVARP(firstpersonswayup, 0, 0.08f, 1);
	FVARP(firstpersonblend, 0, 1, 1);
	FVARP(firstpersondist, -10000, -0.25f, 10000);
	FVARP(firstpersonshift, -10000, 0.3f, 10000);
	FVARP(firstpersonadjust, -10000, -0.07f, 10000);

	VARP(editfov, 1, 120, 179);
	VARP(specfov, 1, 120, 179);

	VARP(follow, -1, 0, INT_MAX-1);
	VARP(specmode, 0, 1, 1); // 0 = float, 1 = tv
	VARP(spectvtime, 1000, 10000, INT_MAX-1);
	FVARP(spectvspeed, 0, 1, 1000);
	FVARP(spectvpitch, 0, 1, 1000);
	VARP(waitmode, 0, 1, 2); // 0 = float, 1 = tv in duel/survivor, 2 = tv always
	VARP(waittvtime, 1000, 5000, INT_MAX-1);
	FVARP(waittvspeed, 0, 1, 1000);
	FVARP(waittvpitch, 0, 1, 1000);
	VARP(deathcamstyle, 0, 1, 2); // 0 = no follow, 1 = follow attacker, 2 = follow self
	FVARP(deathcamspeed, 0, 2.f, 1000);

	FVARP(sensitivity, 1e-3f, 10.0f, 1000);
	FVARP(yawsensitivity, 1e-3f, 10.0f, 1000);
	FVARP(pitchsensitivity, 1e-3f, 7.5f, 1000);
	FVARP(mousesensitivity, 1e-3f, 1.0f, 1000);
	FVARP(zoomsensitivity, 0, 0.75f, 1);

	VARP(zoommousetype, 0, 0, 2);
	VARP(zoommousedeadzone, 0, 25, 100);
	VARP(zoommousepanspeed, 1, 10, INT_MAX-1);
	VARP(zoomfov, 1, 10, 150);
	VARP(zoomtime, 1, 100, 10000);

	VARFP(zoomlevel, 1, 4, 10, checkzoom());
	VARP(zoomlevels, 1, 5, 10);
	VARP(zoomdefault, 0, 0, 10); // 0 = last used, else defines default level
	VARP(zoomscroll, 0, 0, 1); // 0 = stop at min/max, 1 = go to opposite end

	VARP(shownamesabovehead, 0, 2, 2);
	VARP(showstatusabovehead, 0, 2, 2);
	VARP(showteamabovehead, 0, 1, 3);
	VARP(showdamageabovehead, 0, 0, 3);
	FVARP(aboveheadblend, 0.f, 0.75f, 1.f);
	FVAR(aboveheadsmooth, 0, 0.5f, 1);
	VAR(aboveheadsmoothmillis, 1, 200, 10000);
	VARP(aboveheadfade, 500, 5000, INT_MAX-1);

	VARP(showobituaries, 0, 4, 5); // 0 = off, 1 = only me, 2 = 1 + announcements, 3 = 2 + but dying bots, 4 = 3 + but bot vs bot, 5 = all
	VARP(showobitdists, 0, 0, 1);
	VARP(showplayerinfo, 0, 2, 2); // 0 = none, 1 = CON_MESG, 2 = CON_EVENT
	VARP(playdamagetones, 0, 1, 3);

	VARP(quakefade, 0, 100, INT_MAX-1);
    VARP(ragdolls, 0, 1, 1);
	FVARP(bloodscale, 0, 1, 1000);
	VARP(bloodfade, 1, 20000, INT_MAX-1);
	FVARP(gibscale, 0, 1, 1000);
	VARP(gibfade, 1, 20000, INT_MAX-1);
	VARP(fireburnfade, 0, 75, INT_MAX-1);
	FVARP(impulsescale, 0, 1, 1000);
	VARP(impulsefade, 0, 200, INT_MAX-1);

	ICOMMAND(gamemode, "", (), intret(gamemode));
	ICOMMAND(mutators, "", (), intret(mutators));
	ICOMMAND(getintermission, "", (), intret(intermission ? 1 : 0));

	void start() { }

	const char *gametitle() { return connected() ? server::gamename(gamemode, mutators) : "ready"; }
	const char *gametext() { return connected() ? mapname : "not connected"; }

	bool thirdpersonview(bool viewonly)
	{
        if(!viewonly && (focus->state == CS_DEAD || focus->state == CS_WAITING)) return true;
		if(!thirdperson) return false;
		if(player1->state == CS_EDITING) return false;
		if(player1->state == CS_SPECTATOR) return false;
		if(inzoom()) return false;
		return true;
	}
	ICOMMAND(isthirdperson, "i", (int *viewonly), intret(thirdpersonview(*viewonly ? true : false) ? 1 : 0));

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
	ICOMMAND(setzoom, "i", (int *level), setzoomlevel(*level));

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
		if(allowmove(player1) && WPA(player1->weapselect, zooms)) return true;
		zoomset(false, 0);
		return false;
	}

	bool inzoom()
	{
		if(zoomallow() && lastzoom && (zooming || lastmillis-lastzoom <= zoomtime))
			return true;
		return false;
	}
	ICOMMAND(iszooming, "", (), intret(inzoom() ? 1 : 0));

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
			vec vel = vec(d->vel).add(d->falling).mul(FWV(impulsestyle) && d->action[AC_SPRINT] && (d->move || d->strafe) ? 5 : 1);
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
	ICOMMAND(announce, "iis", (int *idx, int *targ, char *s), announce(*idx, *targ, NULL, "\fw%s", s));

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

	ICOMMAND(specmodeswitch, "", (), specmode = specmode ? 0 : 1; hud::sb.showscores(false));
	ICOMMAND(waitmodeswitch, "", (), waitmode = waitmode ? 0 : (m_duke(game::gamemode, game::mutators) ? 1 : 2); hud::sb.showscores(false));
	ICOMMAND(followdelta, "i", (int *n), follow = clamp(follow + *n, -1, INT_MAX-1));

    bool allowmove(physent *d)
    {
		if(d == player1)
		{
			if(UI::hascursor(true)) return false;
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
			else
			{
				loopi(WEAP_SUPER) if(!strcasecmp(weaptype[i].name, s))
				{
					d->loadweap = i;
					break;
				}
			}
			if(d->loadweap < WEAP_OFFSET || d->loadweap >= WEAP_SUPER || d->loadweap == WEAP_GRENADE) d->loadweap = WEAP_MELEE;
			client::addmsg(SV_LOADWEAP, "ri2", d->clientnum, d->loadweap);
			conoutft(CON_SELF, "\fwyou will spawn with: %s%s", weaptype[d->loadweap].text, (d->loadweap >= WEAP_OFFSET ? weaptype[d->loadweap].name : "random weapons"));
		}
		else conoutft(CON_MESG, "\foweapon selection is only available in arena");
	}
	ICOMMAND(loadweap, "s", (char *s), chooseloadweap(player1, s));

	void respawn(gameent *d)
	{
		if(d->state == CS_DEAD && d->respawned < 0 && (!d->lastdeath || lastmillis-d->lastdeath >= 500))
		{
			client::addmsg(SV_TRYSPAWN, "ri", d->clientnum);
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
			if(fireburntime)
			{
				gameent *d = NULL;
				loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d->lastfire && lastmillis-d->lastfire < fireburntime)
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
		if(effect || (FWV(impulsestyle) && d->state == CS_ALIVE && (d->turnside || (d->action[AC_SPRINT] && (!d->ai || d->move || d->strafe)))))
		{
			int num = int((effect ? 25 : 5)*impulsescale), len = effect ? impulsefade : impulsefade/5;
			if(num > 0 && len > 0)
			{
				float intensity = 0.25f+(rnd(75)/100.f);
				if(d->type == ENT_PLAYER || d->type == ENT_AI)
				{
					regularshape(PART_FIREBALL, int(d->radius), firecols[effect ? 0 : rnd(FIRECOLOURS)], 21, num, len, d->lfoot, intensity, 0.5f, -15, 0, 5);
					regularshape(PART_FIREBALL, int(d->radius), firecols[effect ? 0 : rnd(FIRECOLOURS)], 21, num, len, d->rfoot, intensity, 0.5f, -15, 0, 5);
				}
				else regularshape(PART_FIREBALL, int(d->radius)*2, firecols[effect ? 0 : rnd(FIRECOLOURS)], 21, num, len, d->feetpos(), intensity, 0.5f, -15, 0, 5);
			}
		}
	}

	void fireeffect(gameent *d)
	{
		if(fireburntime && d->lastfire && (d != focus || thirdpersonview()) && lastmillis-d->lastfire < fireburntime)
		{
			int millis = lastmillis-d->lastfire; float pc = 1, intensity = 0.25f+(rnd(75)/100.f);
			if(fireburntime-millis < fireburndelay) pc = float(fireburntime-millis)/float(fireburndelay);
			else pc = 0.75f+(float(millis%fireburndelay)/float(fireburndelay*4));
			regular_part_create(PART_FIREBALL_SOFT, max(fireburnfade, 1), d->headpos(-d->height*0.35f), firecols[rnd(FIRECOLOURS)], d->height*deadscale(d, intensity*pc), 0.75f, -15, 0);
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
	ICOMMAND(mode, "ii", (int *val, int *mut), setmode(*val, *mut));

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
	}

	void resetcamera()
	{
		lastcamera = 0;
		zoomset(false, 0);
		resetcursor();
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
		focus = player1;
		hud::sb.showscores(false);
		cleargui();
	}

	void resetstate()
	{
		resetworld();
		resetcamera();
	}

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
				d->setweapstate(i, WEAP_S_IDLE, 0, lastmillis);
		}
		if(d->respawned > 0 && lastmillis-d->respawned >= PHYSMILLIS*4) d->respawned = -1;
		if(d->suicided > 0 && lastmillis-d->suicided >= PHYSMILLIS*4) d->suicided = -1;
		if(d->lastfire > 0)
		{
			if(lastmillis-d->lastfire >= fireburntime-500)
			{
				if(lastmillis-d->lastfire >= fireburntime)
				{
					if(issound(d->fschan)) removesound(d->fschan);
					d->fschan = -1; d->lastfire = 0;
				}
				else if(issound(d->fschan)) sounds[d->fschan].vol = int((d != focus ? 128 : 224)*(1.f-(lastmillis-d->lastfire-(fireburntime-500))/500.f));
			}
		}
		else if(issound(d->fschan))
		{
			removesound(d->fschan);
			d->fschan = -1;
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
        enum { BURN = 1<<0 };

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
            int snd = 0;
            if(flags & BURN) snd = 8;
            else if(damage >= 200) snd = 7;
            else if(damage >= 150) snd = 6;
            else if(damage >= 100) snd = 5;
            else if(damage >= 75) snd = 4;
            else if(damage >= 50) snd = 3;
            else if(damage >= 25) snd = 2;
            else if(damage >= 10) snd = 1;
            playsound(S_DAMAGE1+snd, d->o, d, d == focus ? SND_FORCED : SND_DIRECT, 255-int(camera1->o.dist(d->o)/(getworldsize()/2)*200));
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
        damagetones.setsizenodelete(0);
    }

	static int alarmchan = -1;
	void hiteffect(int weap, int flags, int damage, gameent *d, gameent *actor, vec &dir, bool local)
	{
		bool burning = fireburn(d, weap, flags);
		if(!local || burning)
		{
			if(hithurts(flags))
			{
				if(d == focus) hud::damage(damage, actor->o, actor, weap);
				if(d->type == ENT_PLAYER || d->type == ENT_AI)
				{
					vec p = d->headpos();
					p.z += 0.6f*(d->height + d->aboveeye) - d->height;
					if(!isaitype(d->aitype) || aistyle[d->aitype].maxspeed)
					{
						if(!kidmode && bloodscale > 0)
							part_splash(PART_BLOOD, int(clamp(damage/2, 2, 10)*bloodscale), bloodfade, p, 0x88FFFF, 1.5f, 1, 100, DECAL_BLOOD, int(d->radius*4));
						else part_splash(PART_HINT, int(clamp(damage/2, 2, 10)), bloodfade, p, 0xFFFF88, 1.5f, 1, 50, DECAL_STAIN, int(d->radius*4));
					}
					if(showdamageabovehead > (d != focus ? 0 : 1))
					{
						string ds;
						if(showdamageabovehead > 2) formatstring(ds)("<sub>-%d (%d%%)", damage, flags&HIT_HEAD ? 100 : (flags&HIT_TORSO ? 50 : 25));
						else formatstring(ds)("<sub>-%d", damage);
						part_textcopy(d->abovehead(), ds, PART_TEXT, aboveheadfade, 0x888888, 3, 1, -10, 0, d);
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
					if(vampire)
					{
						float amt = damage/float(m_health(gamemode, mutators));
						part_splash(PART_HINT, clamp(int(amt*30), 1, 30), 100+int(100*amt), d->feetpos(), 0xFF0808, 0.3f+(0.7f*amt), clamp(0.3f+(0.7f*amt), 0.f, 1.f), -1, int(d->radius));
						part_trail(PART_HINT, 100+int(100*amt), d->feetpos(), actor->muzzlepos(weap), 0xFF0808, 0.3f+(0.7f*amt), clamp(0.3f+(0.7f*amt), 0.f, 1.f), -1);
					}
				}
			}
			if(isweap(weap) && !burning && (d == player1 || (d->ai && aistyle[d->aitype].maxspeed)))
			{
				float force = (float(damage)/float(WPB(weap, damage, flags&HIT_ALT)))*(100.f/d->weight)*WPB(weap, hitpush, flags&HIT_ALT);
				if(flags&HIT_WAVE || !hithurts(flags)) force *= wavepushscale;
				else if(d->health <= 0) force *= deadpushscale;
				else force *= hitpushscale;
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
					"pulled off an insta-stunt",
					"was gibbed"
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
			if(d->aitype == AI_TURRET) concatstring(d->obit, "destroyed by");
			else
			{
				static const char *obitnames[4][WEAP_MAX] = {
					{
						"smacked down by",
						"pierced by",
						"sprayed with buckshot by",
						"riddled with holes by",
						"char-grilled by",
						"plasmified by",
						"laser shocked by",
						"blown to pieces by",
						"lasered by",
						"gibbed"
					},
					{
						"smacked down by",
						"pierced by",
						"filled with lead by",
						"spliced apart by",
						"fireballed by",
						"shown the light by",
						"was given laser burn by",
						"blown to pieces by",
						"was given laser burn by",
						"gibbed"
					},
					{
						"smacked down by",
						"capped by",
						"scrambled by",
						"air conditioned courtesy of",
						"char-grilled by",
						"plasmafied by",
						"expertly sniped by",
						"blown to pieces by",
						"expertly sniped by",
						"gibbed"
					},
					{
						"knocked into next week by",
						"skewered by",
						"turned into little chunks by",
						"swiss-cheesed by",
						"barbequed by chef",
						"reduced to ooze by",
						"given laser shock treatment by",
						"obliterated by",
						"lasered in half by",
						"gibbed"
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
					if(actor == player1) d->dominated = false;
					else if(d == player1) actor->dominating = false;
					anc = S_V_REVENGE; override = true;
				}
				else if(style&FRAG_DOMINATE)
				{
					concatstring(d->obit, " \fs\fzoydominating\fS");
					part_text(az, "<super>\fzoyDOMINATING", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, actor); az.z += 4;
					part_text(dz, "<super>\fzoyDOMINATED", PART_TEXT, aboveheadfade, 0xFFFFFF, 4, 1, -10, 0, d); dz.z += 4;
					if(actor == player1) d->dominating = true;
					else if(d == player1) actor->dominated = true;
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

			if(style&FRAG_HEADSHOT)
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
		if(gibscale > 0)
		{
			vec pos = vec(d->o).sub(vec(0, 0, d->height*0.5f));
			int gibs = clamp(max(damage,5)/5, 1, 25), amt = int((rnd(gibs)+gibs+1)*gibscale);
			loopi(amt) projs::create(pos, vec(pos).add(d->vel), true, d, !isaitype(d->aitype) || aistyle[d->aitype].maxspeed ? PRJ_GIBS : PRJ_DEBRIS, rnd(gibfade)+gibfade, 0, rnd(500)+1, rnd(50)+10);
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
		minremain = timeremain;
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

	void clientdisconnected(int cn)
	{
		if(!players.inrange(cn)) return;
		gameent *d = players[cn];
		if(!d) return;
		if(d->name[0] && showplayerinfo && (d->aitype < 0 || ai::showaiinfo))
			conoutft(showplayerinfo > 1 ? int(CON_EVENT) : int(CON_MESG), "\fo%s left the game", colorname(d));
		if(focus == d)
		{
			focus = player1;
			resetcamera();
		}
		cameras.setsize(0);
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

	void startmap(const char *name, const char *reqname, bool empty)	// called just after a map load
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
			defformatstring(s)(" [\fs\fc%s%d\fS]", d->aitype >= 0 ? "\fe" : "", d->clientnum);
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
			client::addmsg(SV_SUICIDE, "ri2", d->clientnum, flags);
			d->suicided = lastmillis;
		}
	}
	ICOMMAND(kill, "",  (), { suicide(player1, 0); });

	void lighteffects(dynent *e, vec &color, vec &dir) { }

    void particletrack(particle *p, uint type, int &ts,  bool lastpass)
    {
        if(!p || !p->owner || (p->owner->type != ENT_PLAYER && p->owner->type != ENT_AI)) return;
		gameent *d = (gameent *)p->owner;
        switch(type&0xFF)
        {
        	case PT_TEXT: case PT_ICON:
        	{
        		vec q = p->owner->abovehead(); q.z = p->o.z;
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

	void newmap(int size) { client::addmsg(SV_NEWMAP, "ri", size); }

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
		bool hascursor = UI::hascursor(true);
		#define mousesens(a,b,c) ((float(a)/float(b))*c)
		if(hascursor || (mousestyle() >= 1 && player1->state != CS_WAITING && player1->state != CS_SPECTATOR))
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
				cursory = clamp(cursory+mousesens(dy, h, mousesensitivity*(!hascursor && mouseinvert ? -1.f : 1.f)), 0.f, 1.f);
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
				target->pitch -= mousesens(dy, h, pitchsensitivity*scale*(!hascursor && mouseinvert ? -1.f : 1.f));
				fixfullrange(target->yaw, target->pitch, target->roll, false);
			}
			return true;
		}
		return false;
	}

	void project(int w, int h)
	{
		int style = UI::hascursor() ? -1 : mousestyle();
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

	void cameraplayer()
	{
		if(player1->state != CS_WAITING && player1->state != CS_SPECTATOR && player1->state != CS_DEAD && !tvmode())
		{
			player1->aimyaw = camera1->yaw;
			player1->aimpitch = camera1->pitch;
			fixrange(player1->aimyaw, player1->aimpitch);
			if(lastcamera && mousestyle() >= 1 && !UI::hascursor())
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
						c.pos = pos; c.ent = i;
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
				c.mindist = 0; c.maxdist = 1e16f;
				vecfromyawpitch(d->yaw, d->pitch, 1, 0, c.dir);
			}
		}
		else loopv(cameras) if(cameras[i].ent < 0 && cameras[i].idx >= 0 && cameras[i].idx < numdynents())
		{
			gameent *d = (gameent *)iterdynents(cameras[i].idx);
			cameras[i].pos = d->headpos();
			vecfromyawpitch(d->yaw, d->pitch, 1, 0, cameras[i].dir);
		}
		#define unsettvmode(q) \
		{ \
			if(q) \
			{ \
				camera1->o.x = camera1->o.y = camera1->o.z = getworldsize(); \
				camera1->o.x *= 0.5f; camera1->o.y *= 0.5f; \
			} \
			camera1->resetinterp(); \
			setvar(isspec ? "specmode" : "waitmode", 0, true); \
			return; \
		}

		if(!cameras.empty())
		{
			camstate *cam = &cameras[0];
			int entidx = cam->ent, len = (isspec ? spectvtime : waittvtime);
			bool alter = cam->alter, renew = !lasttvcam || lastmillis-lasttvcam >= len,
				override = renew || !lasttvcam || lastmillis-lasttvcam >= max(len/3, 1000);
			#define addcamentity(q,p) \
			{ \
				vec trg, pos = p; \
				float dist = c.pos.dist(pos); \
				if(dist >= c.mindist && dist <= min(c.maxdist, float(fogdist)) && raycubelos(c.pos, pos, trg)) \
				{ \
					c.cansee.add(q); \
					avg.add(pos); \
				} \
			}
			#define updatecamorient \
			{ \
				if(!t && (k || j) && c.cansee.length()) \
				{ \
					vec dir = vec(avg).div(c.cansee.length()).sub(c.pos).normalize(); \
					vectoyawpitch(dir, yaw, pitch); \
				} \
			}
			#define dircamentity(q,p) \
			{ \
				vec trg, pos = p; \
				if(getsight(c.pos, yaw, pitch, pos, trg, min(c.maxdist, float(fogdist)), curfov, fovy)) \
				{ \
					c.dir.add(pos); \
					c.score += c.alter ? rnd(32) : c.pos.dist(pos); \
				} \
				else \
				{ \
					avg.sub(pos); \
					c.cansee.remove(q); \
					updatecamorient; \
				} \
			}
			loopk(2)
			{
				int found = 0;
				loopvj(cameras)
				{
					camstate &c = cameras[j]; gameent *t = NULL;
					vec avg(0, 0, 0); c.reset();
					if(c.ent < 0 && c.idx >= 0 && c.idx < numdynents())
					{
						t = (gameent *)iterdynents(c.idx);
						if(t->state == CS_SPECTATOR || (!isspec && waitmode < (m_duke(game::gamemode, game::mutators) ? 1 : 2))) continue;
					}
					switch(k)
					{
						case 0: default:
						{
							gameent *d;
							loopi(numdynents()) if((d = (gameent *)iterdynents(i)))
							{
								if(d == t)
								{
									c.cansee.add(i);
									avg.add(d->feetpos());
								}
								else if(d->aitype < AI_START && (d->state == CS_ALIVE || d->state == CS_DEAD || d->state == CS_WAITING))
									addcamentity(i, d->feetpos());
							}
							break;
						}
						case 1:
						{
							c.alter = true;
							loopv(entities::ents) if(entities::ents[i]->type == WEAPON || entities::ents[i]->type == FLAG)
								addcamentity(i, entities::ents[i]->o);
							break;
						}
					}
					float yaw = camera1->yaw, pitch = camera1->pitch;
					updatecamorient;
					switch(k)
					{
						case 0: default:
						{
							gameent *d;
							loopvrev(c.cansee) if((d = (gameent *)iterdynents(c.cansee[i])))
								dircamentity(i, d->feetpos());
							break;
						}
						case 1:
						{
							loopvrev(c.cansee) if(entities::ents.inrange(c.cansee[i]))
								dircamentity(i, entities::ents[c.cansee[i]]->o);
							break;
						}
					}
					if(!c.cansee.empty())
					{
						float amt = float(c.cansee.length());
						c.dir.div(amt);
						c.score /= amt;
						found++;
					}
					else
					{
						c.score = 0;
						if(override && !k && !j && !alter) renew = true; // quick scotty, get a new cam
					}
					if(!renew || !override) break;
				}
				if(override && !found && (k || !alter))
				{
					if(!k) renew = true;
					else unsettvmode(lasttvcam ? false : true);
				}
				else break;
			}
			if(renew)
			{
				cameras.sort(camstate::camsort);
				cam = &cameras[0];
				lasttvcam = lastmillis;
				if(!lasttvchg || cam->ent != entidx) lasttvchg = lastmillis;
				if(cam->ent < 0 && cam->idx >= 0 && cam->idx < numdynents())
				{
					focus = (gameent *)iterdynents(cam->idx);
					follow = cam->idx;
				}
				else { focus = player1; follow = 0; }
			}
			else if(alter && !cam->cansee.length()) cam->alter = true;
			if(focus != player1)
			{
				camera1->o = focus->headpos();
				camera1->yaw = camera1->aimyaw = focus->yaw;
				camera1->pitch = camera1->aimpitch = focus->pitch;
				camera1->roll = focus->roll;
			}
			else
			{
				camera1->o = cam->pos;
				if(cam->ent != entidx || !cam->alter)
				{
					vec dir = vec(cam->dir).sub(camera1->o).normalize();
					vectoyawpitch(dir, camera1->aimyaw, camera1->aimpitch);
				}
				if(cam->ent != entidx || cam->alter) { camera1->yaw = camera1->aimyaw; camera1->pitch = camera1->aimpitch; }
				else
				{
					float speed = isspec ? spectvspeed : waittvspeed, scale = isspec ? spectvpitch : waittvpitch;
					if(speed > 0) scaleyawpitch(camera1->yaw, camera1->pitch, camera1->aimyaw, camera1->aimpitch, (float(curtime)/1000.f)*speed, scale);
				}
			}
			camera1->resetinterp();
		}
		else unsettvmode(true);
	}

	void updateworld()		// main game update loop
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

       	if(!*player1->name && !menuactive()) showgui("name");
        if(connected())
        {
        	player1->conopen = commandmillis > 0 || UI::hascursor(true);
            // do shooting/projectile update here before network update for greater accuracy with what the player sees
			if(allowmove(player1)) cameraplayer();
			else player1->stopmoving(player1->state != CS_WAITING && player1->state != CS_SPECTATOR);

            gameent *d = NULL; int count = 0;
            loopi(numdynents()) if((d = (gameent *)iterdynents(i)) != NULL && (d->type == ENT_PLAYER || d->type == ENT_AI))
            {
            	if(d != player1 && d->state != CS_SPECTATOR)
            	{
					count++;
					if((player1->state == CS_SPECTATOR || player1->state == CS_WAITING) && !tvmode() && (follow < 0 || follow == count) && focus != d)
					{
						focus = d;
						resetcamera();
					}
            	}
				checkoften(d, d == player1 || d->ai);
				if(d == player1)
				{
					int state = d->weapstate[d->weapselect];
					if(WPA(d->weapselect, zooms))
					{
						if(state == WEAP_S_SHOOT || (state == WEAP_S_RELOAD && lastmillis-d->weaplast[d->weapselect] >= max(d->weapwait[d->weapselect]-zoomtime, 1)))
							state = WEAP_S_IDLE;
					}
					if(zooming && (!WPA(d->weapselect, zooms) || state != WEAP_S_IDLE)) zoomset(false, lastmillis);
					else if(WPA(d->weapselect, zooms) && state == WEAP_S_IDLE && zooming != d->action[AC_ALTERNATE])
						zoomset(d->action[AC_ALTERNATE], lastmillis);
				}
            }
            if(follow < 0) follow = count;
            if((((player1->state != CS_SPECTATOR && player1->state != CS_WAITING) || !follow) && focus != player1) || follow > count)
            {
            	follow = 0;
            	focus = player1;
            	resetcamera();
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
				showgui("loadout");
        }
        else if(!menuactive()) showgui("main");

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
				addsway(focus);
            }
			checkcamera();
			if(focus->state == CS_DEAD)
			{
				gameent *a = deathcamstyle ? (deathcamstyle == 2 ? focus : getclient(focus->lastattacker)) : NULL;
				if(a)
				{
					vec dir = vec(a->headpos(-a->height*0.5f)).sub(camera1->o).normalize();
					float yaw = camera1->yaw, pitch = camera1->pitch;
					vectoyawpitch(dir, yaw, pitch);
					if(deathcamspeed > 0) scaleyawpitch(camera1->yaw, camera1->pitch, yaw, pitch, (float(curtime)/1000.f)*deathcamspeed, 4.f);
					camera1->aimyaw = camera1->yaw;
					camera1->aimpitch = camera1->pitch;
				}
			}
			else if(tvmode()) cameratv();
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
				cameras.setsize(0);
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

	VAR(animoverride, -1, 0, ANIM_MAX-1);
	VAR(testanims, 0, 0, 1);

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
			if(firstpersonsway)
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
				else if(d->physstate == PHYS_FALL && !d->onladder && FWV(impulsestyle) && d->impulse[IM_TYPE] != IM_T_NONE && lastmillis-d->impulse[IM_TIME] <= 1000) { anim |= ANIM_IMPULSE_DASH<<ANIM_SECONDARY; basetime2 = d->impulse[IM_TIME]; }
				else if(d->physstate == PHYS_FALL && !d->onladder && d->actiontime[AC_JUMP] && lastmillis-d->actiontime[AC_JUMP] <= 1000) { anim |= ANIM_JUMP<<ANIM_SECONDARY; basetime2 = d->actiontime[AC_JUMP]; }
				else if(d->physstate == PHYS_FALL && !d->onladder && d->timeinair >= 1000) anim |= (ANIM_JUMP|ANIM_END)<<ANIM_SECONDARY;
				else if(FWV(impulsestyle) && d->action[AC_SPRINT] && (d->move || d->strafe))
				{
					if(d->move>0)		anim |= (ANIM_IMPULSE_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
					else if(d->strafe)	anim |= ((d->strafe>0 ? ANIM_IMPULSE_LEFT : ANIM_IMPULSE_RIGHT)|ANIM_LOOP)<<ANIM_SECONDARY;
					else if(d->move<0)	anim |= (ANIM_IMPULSE_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;
				}
				else if(d->action[AC_CROUCH] || d->actiontime[AC_CROUCH]<0)
				{
					if(d->move>0)		anim |= (ANIM_CRAWL_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
					else if(d->strafe)	anim |= ((d->strafe>0 ? ANIM_CRAWL_LEFT : ANIM_CRAWL_RIGHT)|ANIM_LOOP)<<ANIM_SECONDARY;
					else if(d->move<0)	anim |= (ANIM_CRAWL_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;
					else				anim |= (ANIM_CROUCH|ANIM_LOOP)<<ANIM_SECONDARY;
				}
				else if(d->move>0) anim |= (ANIM_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
				else if(d->strafe) anim |= ((d->strafe>0 ? ANIM_LEFT : ANIM_RIGHT)|ANIM_LOOP)<<ANIM_SECONDARY;
				else if(d->move<0) anim |= (ANIM_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;
			}

			if((anim>>ANIM_SECONDARY)&ANIM_INDEX) switch(anim&ANIM_INDEX)
			{
				case ANIM_IDLE: case ANIM_MELEE: case ANIM_PISTOL: case ANIM_SHOTGUN: case ANIM_SMG:
				case ANIM_GRENADE: case ANIM_FLAMER: case ANIM_PLASMA: case ANIM_RIFLE:
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
#if 0 // breaks linkpos
		if(d != focus && !(anim&ANIM_RAGDOLL)) flags |= MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_CULL_QUERY;
#endif
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
						if(WPA(weap, power)) animflags = weaptype[weap].anim+d->weapstate[weap];
						else animflags = weaptype[weap].anim|ANIM_LOOP;
						break;
					}
					case WEAP_S_SHOOT:
					{
						if(!d->hasweap(weap, m_weapon(gamemode, mutators)) || (!WPA(weap, reloads) && lastmillis-d->weaplast[weap] <= d->weapwait[weap]/3))
							showweap = false;
						animflags = weaptype[weap].anim+d->weapstate[weap];
						break;
					}
					case WEAP_S_RELOAD:
					{
						if(weap != WEAP_MELEE)
						{
							if(!d->hasweap(weap, m_weapon(gamemode, mutators)) || (!WPA(weap, reloads) && lastmillis-d->weaplast[weap] <= d->weapwait[weap]/3))
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
					else if(d->dominating) t = textureload(hud::dominatingtex, 3);
					else if(d->dominated) t = textureload(hud::dominatedtex, 3);
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
