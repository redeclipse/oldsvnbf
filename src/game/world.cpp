#define GAMEWORLD 1
#include "pch.h"
#include "engine.h"
#include "game.h"
namespace world
{
	int nextmode = G_LOBBY, nextmuts = 0, gamemode = G_LOBBY, mutators = 0;
	bool intermission = false;
	int maptime = 0, minremain = 0, swaymillis = 0;
	vec swaydir(0, 0, 0);
    int lasthit = 0, lastcamera = 0, lastspec = 0, lastzoom = 0, lastmousetype = 0;
    bool prevzoom = false, zooming = false;
	int quakewobble = 0, damageresidue = 0;
    int liquidchan = -1, hudwidth = 0;

	gameent *player1 = new gameent();
	vector<gameent *> players;
	gameent lastplayerstate;
	dynent fpsmodel;

	scoreboard sb;

	VARP(invmouse, 0, 0, 1);
	VARP(absmouse, 0, 0, 1);

	VARP(thirdperson, 0, 0, 1);

	VARP(thirdpersonmouse, 0, 0, 2);
	VARP(thirdpersondeadzone, 0, 10, 100);
	VARP(thirdpersonpanspeed, 1, 30, INT_MAX-1);
	VARP(thirdpersonaim, 0, 250, INT_MAX-1);
	VARP(thirdpersonfov, 90, 120, 150);
	VARP(thirdpersontranslucent, 0, 0, 1);
	VARP(thirdpersondist, -100, 1, 100);
	VARP(thirdpersonshift, -100, 4, 100);
	VARP(thirdpersonangle, 0, 40, 360);

	VARP(firstpersonmouse, 0, 0, 2);
	VARP(firstpersondeadzone, 0, 10, 100);
	VARP(firstpersonpanspeed, 1, 30, INT_MAX-1);
	VARP(firstpersonfov, 90, 100, 150);
	VARP(firstpersonaim, 0, 0, INT_MAX-1);
	VARP(firstpersonsway, 0, 100, INT_MAX-1);
	VARP(firstpersontranslucent, 0, 0, 1);
	FVARP(firstpersondist, -10000, -0.25f, 10000);
	FVARP(firstpersonshift, -10000, 0.25f, 10000);
	FVARP(firstpersonadjust, -10000, 0.f, 10000);

	VARP(editmouse, 0, 0, 2);
	VARP(editfov, 1, 120, 179);
	VARP(editdeadzone, 0, 10, 100);
	VARP(editpanspeed, 1, 20, INT_MAX-1);

	VARP(spectv, 0, 1, 1); // 0 = float, 1 = tv
	VARP(spectvtime, 0, 30000, INT_MAX-1);
	VARP(specmouse, 0, 0, 2);
	VARP(specfov, 1, 120, 179);
	VARP(specdeadzone, 0, 10, 100);
	VARP(specpanspeed, 1, 20, INT_MAX-1);

	FVARP(sensitivity, 1e-3f, 10.0f, 1000);
	FVARP(yawsensitivity, 1e-3f, 10.0f, 1000);
	FVARP(pitchsensitivity, 1e-3f, 7.5f, 1000);
	FVARP(mousesensitivity, 1e-3f, 1.0f, 1000);
	FVARP(snipesensitivity, 1e-3f, 3.0f, 1000);
	FVARP(pronesensitivity, 1e-3f, 5.0f, 1000);

	VARP(showdamage, 0, 1, 1);
	TVAR(damagetex, "textures/damage", 0);
	VARP(showindicator, 0, 1, 1);
	TVAR(indicatortex, "textures/indicator", 3);
	FVARP(indicatorblend, 0, 0.5f, 1);
	TVAR(snipetex, "textures/snipe", 0);

	VARP(showcrosshair, 0, 1, 1);
	TVAR(relativecursortex, "textures/cursordot", 3);
	TVAR(guicursortex, "textures/cursor", 3);
	TVAR(editcursortex, "textures/cursordot", 3);
	TVAR(speccursortex, "textures/cursordot", 3);
	TVAR(crosshairtex, "textures/crosshair", 3);
	TVAR(teamcrosshairtex, "textures/teamcrosshair", 3);
	TVAR(hitcrosshairtex, "textures/hitcrosshair", 3);
	TVAR(snipecrosshairtex, "textures/snipecrosshair", 3);
	FVARP(crosshairsize, 0, 0.05f, 1);
	VARP(crosshairhitspeed, 0, 1000, INT_MAX-1);
	FVARP(crosshairblend, 0, 0.5f, 1);

	FVARP(cursorsize, 0, 0.05f, 1);
	FVARP(cursorblend, 0, 1.f, 1);

	VARP(snipetype, 0, 0, 1);
	VARP(snipemouse, 0, 0, 2);
	VARP(snipedeadzone, 0, 25, 100);
	VARP(snipepanspeed, 1, 10, INT_MAX-1);
	VARP(snipefov, 1, 35, 150);
	VARP(snipetime, 1, 300, 10000);
	FVARP(snipecrosshairsize, 0, 0.5f, 1);

	VARP(pronetype, 0, 0, 1);
	VARP(pronemouse, 0, 0, 2);
	VARP(pronedeadzone, 0, 25, 100);
	VARP(pronepanspeed, 1, 10, INT_MAX-1);
	VARP(pronefov, 70, 70, 150);
	VARP(pronetime, 1, 150, 10000);

	VARP(showclip, 0, 1, 1);
	FVARP(clipblend, 0, 0.2f, 1);
	TVAR(plasmacliptex, "textures/plasmaclip", 3);
	TVAR(shotguncliptex, "textures/shotgunclip", 3);
	TVAR(chainguncliptex, "textures/chaingunclip", 3);
	TVAR(grenadescliptex, "textures/grenadesclip", 3);
	TVAR(flamercliptex, "textures/flamerclip", 3);
	TVAR(carbinecliptex, "textures/carbineclip", 3);
	TVAR(riflecliptex, "textures/rifleclip", 3);

	//FVARP(barblend, 0, 1.0f, 1);
	//FVARP(infoblend, 0, 1.f, 1);
	//FVARP(ammosize, 0, 0.07f, 1);
	//FVARP(ammoblend, 0, 0.8f, 1);
	//FVARP(ammoblendinactive, 0, 0.3f, 1);

	VARP(showradar, 0, 1, 1);
	TVAR(radartex, "textures/radar", 3);
	FVARP(radarsize, 0, 0.02f, 1);
	VARP(radardist, 0, 512, 512);
	VARP(radarnames, 0, 1, 1);
	FVARP(radarblend, 0, 0.3f, 1);
	FVARP(radarblipblend, 0, 0.5f, 1);
	FVARP(radarcardblend, 0, 0.3f, 1);
	VARP(editradardist, 0, 512, INT_MAX-1);
	VARP(editradarnoisy, 0, 1, 2);

	//VARP(showtips, 0, 2, 3);
	//VARP(showguns, 0, 2, 2);
	//VARP(showenttips, 0, 1, 2);
	//VARP(showhudents, 0, 10, 100);

	VARP(hudsize, 0, 2000, INT_MAX-1);

	VARP(titlecardtime, 0, 2000, 10000);
	VARP(titlecardfade, 0, 3000, 10000);
	FVARP(titlecardsize, 0, 0.3f, 1);

	VARP(showstats, 0, 0, 1);
	VARP(statrate, 0, 200, 1000);
	VARP(showfps, 0, 2, 2);
	VARP(shownamesabovehead, 0, 1, 2);

	//TVAR(healthbartex, "textures/healthbar", 0);
	//TVAR(plasmatex, "textures/plasma", 0);
	//TVAR(shotguntex, "textures/shotgun", 0);
	//TVAR(chainguntex, "textures/chaingun", 0);
	//TVAR(grenadestex, "textures/grenades", 0);
	//TVAR(flamertex, "textures/flamer", 0);
	//TVAR(carbinetex, "textures/carbine", 0);
	//TVAR(rifletex, "textures/rifle", 0);

	ICOMMAND(kill, "",  (), { suicide(player1, 0); });
	ICOMMAND(gamemode, "", (), intret(gamemode));
	ICOMMAND(mutators, "", (), intret(mutators));

	void start()
	{
		s_strcpy(player1->name, "unnamed");
		entities::start();
	}

	char *gametitle() { return server::gamename(gamemode, mutators); }
	char *gametext() { return getmapname(); }

	float radarrange()
	{
		float dist = float(radardist);
		if(player1->state == CS_EDITING) dist = float(editradardist);
		return dist;
	}

	bool isthirdperson()
	{
		if(!thirdperson) return false;
		if(player1->state == CS_EDITING) return false;
		if(player1->state == CS_SPECTATOR) return false;
		if(player1->state == CS_WAITING) return false;
		return true;
	}

	int mousestyle()
	{
		if(player1->state == CS_EDITING) return editmouse;
		if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) return specmouse;
		if(inzoom()) return player1->gunselect == GUN_RIFLE ? snipemouse : pronemouse;
		if(isthirdperson()) return thirdpersonmouse;
		return firstpersonmouse;
	}

	int deadzone()
	{
		if(player1->state == CS_EDITING) return editdeadzone;
		if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) return specdeadzone;
		if(inzoom()) return player1->gunselect == GUN_RIFLE ? snipedeadzone : pronedeadzone;
		if(isthirdperson()) return thirdpersondeadzone;
		return firstpersondeadzone;
	}

	int panspeed()
	{
		if(inzoom()) return player1->gunselect == GUN_RIFLE ? snipepanspeed : pronepanspeed;
		if(player1->state == CS_EDITING) return editpanspeed;
		if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) return specpanspeed;
		if(isthirdperson()) return thirdpersonpanspeed;
		return firstpersonpanspeed;
	}

	int fov()
	{
		if(player1->state == CS_EDITING) return editfov;
		if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) return specfov;
		if(isthirdperson()) return thirdpersonfov;
		return firstpersonfov;
	}

	void zoomset(bool on, int millis)
	{
		if(on != zooming)
		{
			resetcursor();
			lastzoom = millis;
			prevzoom = zooming;
		}
		zooming = on;
	}

	bool zoomallow()
	{
		if(allowmove(player1)) return true;
		zoomset(false, 0);
		return false;
	}

	int zoomtime()
	{
		return player1->gunselect == GUN_RIFLE ? snipetime : pronetime;
	}

	bool inzoom()
	{
		if(zoomallow() && (zooming || lastmillis-lastzoom < zoomtime()))
			return true;
		return false;
	}

	bool inzoomswitch()
	{
		if(zoomallow() && ((zooming && lastmillis-lastzoom > zoomtime()/2) || (!zooming && lastmillis-lastzoom < zoomtime()/2)))
			return true;
		return false;
	}

	void dozoom(bool down)
	{
		if(zoomallow())
		{
			bool on = false;
			switch(player1->gunselect == GUN_RIFLE ? snipetype : pronetype)
			{
				case 1: on = down; break;
				case 0: default:
					if(down) on = !zooming;
					else on = zooming;
					break;
			}
			zoomset(on, lastmillis);
		}
	}
	ICOMMAND(zoom, "D", (int *down), { dozoom(*down!=0); });

	void addsway(gameent *d)
	{
		if(firstpersonsway)
		{
			if(d->physstate >= PHYS_SLOPE) swaymillis += curtime;

			float k = pow(0.7f, curtime/10.0f);
			swaydir.mul(k);
			vec vel(d->vel);
			vel.add(d->falling);
			swaydir.add(vec(vel).mul((1-k)/(15*max(vel.magnitude(), physics::maxspeed(d)))));
		}
		else swaydir = vec(0, 0, 0);
	}

	int respawnwait(gameent *d)
	{
		int wait = 0;
		if(m_stf(gamemode)) wait = stf::respawnwait(d);
		else if(m_ctf(gamemode)) wait = ctf::respawnwait(d);
		return wait;
	}

	void respawn(gameent *d)
	{
		if(d->state == CS_DEAD && !respawnwait(d))
			respawnself(d);
	}

	bool tvmode()
	{
		return player1->state == CS_SPECTATOR && spectv;
	}

    bool allowmove(physent *d)
    {
        if(d->type == ENT_PLAYER)
        {
        	if(d == player1)
        	{
        		if(UI::hascursor(true)) return false;
				if(tvmode()) return false;
        	}
			if(d->state == CS_DEAD) return false;
			if(intermission) return false;
        }
        return true;
    }

	void respawnself(gameent *d)
	{
		d->stopmoving();

		if(d->respawned != d->lifesequence)
		{
			client::addmsg(SV_TRYSPAWN, "ri", d->clientnum);
			d->respawned = d->lifesequence;
		}
	}

	gameent *pointatplayer()
	{
		loopv(players)
		{
			gameent *o = players[i];
			if(!o) continue;
			vec pos = headpos(player1, 0.f);
			if(intersect(o, pos, worldpos)) return o;
		}
		return NULL;
	}

	void setmode(int nmode, int nmuts)
	{
		nextmode = nmode; nextmuts = nmuts;
		server::modecheck(&nextmode, &nextmuts);
	}
	ICOMMAND(mode, "ii", (int *val, int *mut), setmode(*val, *mut));

	void resetstates(int types)
	{
		if(types & ST_CAMERA)
		{
			lastcamera = 0;
			zoomset(false, 0);
		}
		if(types & ST_CURSOR) resetcursor();
		if(types & ST_GAME)
		{
			sb.showscores(false);
			lasthit = 0;
		}
		if(types & ST_SPAWNS)
		{
			projs::reset();
			// reset perma-state
			gameent *d;
			loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d->type == ENT_PLAYER)
				d->resetstate(lastmillis);
		}
	}

	void heightoffset(gameent *d, bool local)
	{
		d->o.z -= d->height;
		if(d->state == CS_ALIVE)
		{
			if(physics::iscrouching(d))
			{
				float crouchoff = 1.f-CROUCHHEIGHT;
				if(d->type == ENT_PLAYER)
				{
					float amt = clamp(float(lastmillis-d->crouchtime)/200.f, 0.f, 1.f);
					if(!d->crouching) amt = 1.f-amt;
					crouchoff *= amt;
				}
				d->height = PLAYERHEIGHT-(PLAYERHEIGHT*crouchoff);
			}
			else d->height = PLAYERHEIGHT;
		}
		else if(d->state == CS_DEAD)
		{
			if(d->obliterated) d->height = PLAYERHEIGHT;
			else
			{
				int t = lastmillis-d->lastpain;
				if(t < 0) d->height = PLAYERHEIGHT;
				float amt = t > 1000 ? 0.9f : clamp(float(lastmillis-d->crouchtime)/1000.f, 0.f, 0.9f);
				d->height = PLAYERHEIGHT-(PLAYERHEIGHT*amt);
			}
		}
		else d->height = PLAYERHEIGHT;
		d->o.z += d->height;
	}

	void checkoften(gameent *d, bool local)
	{
		heightoffset(d, local);

        if(d->muzzle == vec(-1, -1, -1))
        { // ensure valid projection "position"
        	vec dir, right;
        	vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
        	vecfromyawpitch(d->yaw, d->pitch, 0, -1, right);
        	dir.mul(d->radius*1.25f);
        	right.mul(d->radius);
        	dir.z -= d->height*0.1f;
			d->muzzle = vec(d->o).add(dir).add(right);
        }

		loopi(GUN_MAX) if(d->gunstate[i] != GNS_IDLE)
		{
			if(d->state != CS_ALIVE || (d->gunstate[i] != GNS_POWER && lastmillis-d->gunlast[i] >= d->gunwait[i]))
				d->setgunstate(i, GNS_IDLE, 0, lastmillis);
		}

		if(d->reqswitch > 0 && lastmillis-d->reqswitch > GUNSWITCHDELAY*2)
			d->reqswitch = -1;
		if(d->reqreload > 0 && lastmillis-d->reqreload > guntype[d->gunselect].rdelay*2)
			d->reqreload = -1;
		if(d->requse > 0 && lastmillis-d->requse > GUNSWITCHDELAY*2)
			d->requse = -1;
	}


	void otherplayers()
	{
		loopv(players) if(players[i])
		{
            gameent *d = players[i];
            const int lagtime = lastmillis-d->lastupdate;
            if(d->ai || !lagtime || intermission) continue;
            else if(lagtime>1000 && d->state==CS_ALIVE)
			{
                d->state = CS_LAGGED;
				continue;
			}
			physics::smoothplayer(d, 1, false);
		}
	}

	void updateworld()		// main game update loop
	{
		if(!curtime) return;
        if(!maptime)
        {
        	maptime = lastmillis;
        	return;
        }

        if(connected())
        {
            // do shooting/projectile update here before network update for greater accuracy with what the player sees
			if(!allowmove(player1)) player1->stopmoving();

            gameent *d = NULL;
            loopi(numdynents()) if((d = (gameent *)iterdynents(i)) != NULL && d->type == ENT_PLAYER)
				checkoften(d, d == player1 || d->ai);

            physics::update();
            projs::update();
            entities::update();
            ai::update();

            if(player1->state == CS_ALIVE) weapons::shoot(player1, worldpos);

            otherplayers();
        }

		gets2c();

		if(connected())
		{
			#define adjustscaled(t,n) \
				if(n > 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/100.f)); if(n <= 0) n = (t)0; }

			adjustscaled(float, player1->roll);
			adjustscaled(int, quakewobble);
			adjustscaled(int, damageresidue);

			if(player1->state == CS_DEAD)
			{
				if(lastmillis-player1->lastpain < 2000)
					physics::move(player1, 10, false);
			}
			else if(player1->state == CS_ALIVE)
			{
				physics::move(player1, 10, true);
				addsway(player1);
				entities::checkitems(player1);
				weapons::reload(player1);
			}
			else physics::move(player1, 10, true);
		}

		if(player1->clientnum >= 0) c2sinfo(40);
	}

	void damaged(int gun, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir)
	{
		if(d->state != CS_ALIVE || intermission) return;

        d->lastregen = d->lastpain = lastmillis;
        d->health = health;

		if(actor->type == ENT_PLAYER) actor->totaldamage += damage;

		if(d == player1)
		{
			quakewobble += damage/2;
			damageresidue += damage*2;
		}
		if(d == player1 || d->ai) d->hitpush(damage, dir);

		if(d->type == ENT_PLAYER)
		{
			vec p = headpos(d);
			p.z += 0.6f*(d->height + d->aboveeye) - d->height;
			part_splash(PART_BLOOD, max(damage/3, 3), REGENWAIT, p, 0x66FFFF, 3.f);
			s_sprintfd(ds)("@%d", damage);
			part_text(vec(d->abovehead()).sub(vec(0, 0, 3)), ds, PART_TEXT_RISE, 3000, 0xFFFFFF, 3.f);
			playsound(S_PAIN1+rnd(5), d->o, d);
		}

		if(d != actor)
		{
			int snd = 0;
			if(damage >= 200) snd = 7;
			else if(damage >= 150) snd = 6;
			else if(damage >= 100) snd = 5;
			else if(damage >= 75) snd = 4;
			else if(damage >= 50) snd = 3;
			else if(damage >= 25) snd = 2;
			else if(damage >= 10) snd = 1;
			playsound(S_DAMAGE1+snd, actor->o, actor);
			if(actor == player1) lasthit = lastmillis;
		}

		ai::damaged(d, actor, gun, flags, damage, health, millis, dir);
	}

	void killed(int gun, int flags, int damage, gameent *d, gameent *actor)
	{
		if(d->type != ENT_PLAYER) return;

		d->obliterated = d == actor || flags&HIT_EXPLODE || flags&HIT_MELT || damage > MAXHEALTH;
        d->lastregen = d->lastpain = lastmillis;
		d->state = CS_DEAD;
		d->deaths++;

		int anc = -1, dth = S_DIE1+rnd(2);
		if(flags&HIT_MELT || flags&HIT_BURN) dth = S_BURN;
		else if(d->obliterated) dth = S_SPLAT;

		if(d == player1)
		{
			anc = S_V_FRAGGED;
			sb.showscores(true);
			lastplayerstate = *player1;
			d->stopmoving();
			d->pitch = 0;
			d->roll = 0;
		}
		else
        {
            d->move = d->strafe = 0;
            d->resetinterp();
        }

		s_strcpy(d->obit, "rests in pieces");
        if(d == actor)
        {
        	if(flags&HIT_MELT) s_strcpy(d->obit, "melted");
			else if(flags&HIT_FALL) s_strcpy(d->obit, "thought they could fly");
        	else if(flags && isgun(gun))
        	{
				static const char *suicidenames[GUN_MAX] = {
					"found out what their plasma tasted like",
					"discovered buckshot bounces",
					"got caught up in their own crossfire",
					"barbequed themselves for dinner",
					"pulled off a seemingly impossible stunt",
					"pulled off a seemingly impossible stunt",
					"decided to kick it, kamakaze style",
				};
        		s_strcpy(d->obit, suicidenames[gun]);
        	}
        	else if(flags&HIT_EXPLODE) s_strcpy(d->obit, "was obliterated");
        	else if(flags&HIT_BURN) s_strcpy(d->obit, "burnt up");
        	else s_strcpy(d->obit, "suicided");
        }
		else
		{
			static const char *obitnames[3][GUN_MAX] = {
				{
					"was plasmified by",
					"was filled with buckshot by",
					"was riddled with holes by",
					"was char-grilled by",
					"was skewered by",
					"was pierced by",
					"was blown to pieces by",
				},
				{
					"was plasmafied by",
					"was given scrambled brains cooked up by",
					"was air conditioned courtesy of",
					"was char-grilled by",
					"was given an extra orifice by",
					"was expertly sniped by",
					"was blown to pieces by",
				},
				{
					"was reduced to ooze by",
					"was turned into little chunks by",
					"was swiss-cheesed by",
					"was made the main course by order of chef",
					"was spliced by",
					"had their head blown clean off by",
					"was obliterated by",
				}
			};

			int o = d->obliterated ? 2 : (flags&HIT_HEAD && !guntype[gun].explode ? 1 : 0);
			const char *oname = isgun(gun) ? obitnames[o][gun] : "was killed by";
			if(m_team(gamemode, mutators) && d->team == actor->team)
				s_sprintf(d->obit)("%s teammate %s", oname, colorname(actor));
			else
			{
				s_sprintf(d->obit)("%s %s", oname, colorname(actor));
				switch(actor->spree)
				{
					case 5:
					{
						s_strcat(d->obit, " in total carnage!");
						anc = S_V_SPREE1;
						s_sprintfd(ds)("@\fgCARNAGE");
						part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						break;
					}
					case 10:
					{
						s_strcat(d->obit, " who is slaughtering!");
						anc = S_V_SPREE2;
						s_sprintfd(ds)("@\fgSLAUGHTER");
						part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						break;
					}
					case 25:
					{
						s_strcat(d->obit, " going on a massacre!");
						anc = S_V_SPREE3;
						s_sprintfd(ds)("@\fgMASSACRE");
						part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						break;
					}
					case 50:
					{
						s_strcat(d->obit, " creating a bloodbath!");
						anc = S_V_SPREE4;
						s_sprintfd(ds)("@\fgBLOODBATH");
						part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						break;
					}
					default:
					{
						if(flags&HIT_HEAD)
						{
							anc = S_V_HEADSHOT;
							s_sprintfd(ds)("@\fgHEADSHOT");
							part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						}
						else if(d->obliterated || lastmillis-d->lastspawn <= REGENWAIT*3)
						{
							anc = S_V_OWNED;
						}
						break;
					}
				}
			}
		}
		bool af = (d == player1 || actor == player1);
		if(dth >= 0) playsound(dth, d->o, d);
		s_sprintfd(a)("\fy%s %s", colorname(d), d->obit);
		entities::announce(anc, a, af);
		s_sprintfd(da)("@%s", a);
		part_text(vec(d->abovehead()).add(vec(0, 0, 4)), da, PART_TEXT_RISE, 5000, 0xFFFFFF, 3.f);

		vec pos = headpos(d);
		int gdiv = d->obliterated ? 2 : 4, gibs = clamp((damage+gdiv)/gdiv, 1, 20);
		loopi(rnd(gibs)+1)
			projs::create(pos, vec(pos).add(d->vel), true, d, PRJ_GIBS, rnd(2000)+1000, rnd(100)+1, 50);

		ai::killed(d, actor, gun, flags, damage);
	}

	void timeupdate(int timeremain)
	{
		minremain = timeremain;
		if(!timeremain)
		{
			if(!intermission)
			{
				player1->stopmoving();
				sb.showscores(true, true);
				intermission = true;
			}
		}
		else if(timeremain > 0)
		{
			console("\f2time remaining: %d %s", CON_NORMAL|CON_CENTER, timeremain, timeremain==1 ? "minute" : "minutes");
		}
	}

	gameent *newclient(int cn)
	{
		if(cn < 0 || cn >= MAXCLIENTS)
		{
			neterr("clientnum");
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
		if(d->name[0]) conoutf("\fo%s left the game", colorname(d));
		projs::remove(d);
		DELETEP(players[cn]);
		players[cn] = NULL;
		cleardynentcache();
	}

    void preload()
    {
    	int n = m_team(gamemode, mutators) ? numteams(gamemode, mutators)+1 : 1;
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
    }

	void startmap(const char *name)	// called just after a map load
	{
		const char *title = getmaptitle();
		if(*title) console("%s", CON_CENTER|CON_NORMAL, title);
		intermission = false;
        player1->respawned = player1->suicided = maptime = 0;
        preload();
        entities::mapstart();
		client::mapstart();
        resetstates(ST_ALL);

        // prevent the player from being in the middle of nowhere if he doesn't get spawned
        entities::findplayerspawn(player1);
	}

	void playsoundc(int n, gameent *d = NULL)
	{
		if(n < 0 || n >= S_MAX) return;
		gameent *c = d ? d : player1;
		if(c == player1 || c->ai) client::addmsg(SV_SOUND, "i2", c->clientnum, n);
		playsound(n, c->o, c);
	}

	gameent *intersectclosest(vec &from, vec &to, gameent *at)
	{
		gameent *best = NULL, *o;
		float bestdist = 1e16f;
		loopi(numdynents()) if((o = (gameent *)iterdynents(i)))
		{
            if(!o || o==at || o->state!=CS_ALIVE || lastmillis-o->lastspawn <= REGENWAIT) continue;
			if(!intersect(o, from, to)) continue;
			float dist = at->o.dist(o->o);
			if(dist<bestdist)
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
		s_sprintf(cname)("%s\fs%s\fS", *prefix ? prefix : "", name);
		if(!name[0] || d->aitype != AI_NONE || (dupname && duplicatename(d, name)))
		{
			s_sprintfd(s)(" [\fs%s%d\fS]", d->aitype != AI_NONE ? "\fc" : "\fm", d->clientnum);
			s_strcat(cname, s);
		}
		if(team && m_team(gamemode, mutators))
		{
			s_sprintfd(s)(" (\fs%s%s\fS)", teamtype[d->team].chat, teamtype[d->team].name);
			s_strcat(cname, s);
		}
		return cname;
	}

	void suicide(gameent *d, int flags)
	{
		if(d == player1 || d->ai)
		{
			if(d->state!=CS_ALIVE) return;
			if(d->suicided!=d->lifesequence)
			{
				client::addmsg(SV_SUICIDE, "ri2", d->clientnum, flags);
				d->suicided = d->lifesequence;
			}
		}
	}

	enum
	{
		POINTER_NONE = 0,
		POINTER_RELATIVE,
		POINTER_GUI,
		POINTER_EDIT,
		POINTER_SPEC,
		POINTER_HAIR,
		POINTER_TEAM,
		POINTER_HIT,
		POINTER_SNIPE,
		POINTER_MAX
	};

    const char *getpointer(int index)
    {
        switch(index)
        {
            case POINTER_RELATIVE: default: return relativecursortex; break;
            case POINTER_GUI: return guicursortex; break;
            case POINTER_EDIT: return editcursortex; break;
            case POINTER_SPEC: return speccursortex; break;
            case POINTER_HAIR: return crosshairtex; break;
            case POINTER_TEAM: return teamcrosshairtex; break;
            case POINTER_HIT: return hitcrosshairtex; break;
            case POINTER_SNIPE: return snipecrosshairtex; break;
        }
        return NULL;
    }

	void drawindicator(int gun, float x, float y, float s)
	{
		Texture *t = textureload(indicatortex, 3);
		if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
		float amt = clamp(float(lastmillis-player1->gunlast[gun])/float(guntype[gun].power), 0.f, 2.f),
			r = 1.f, g = amt < 1.f ? 1.f : clamp(2.f-amt, 0.f, 1.f), b = amt < 1.f ? clamp(1.f-amt, 0.f, 1.f) : 0.f;
		glColor4f(r, g, b, indicatorblend);
		glBindTexture(GL_TEXTURE_2D, t->retframe(lastmillis-player1->gunlast[gun], guntype[gun].power));
		float cx = x*hudwidth, cy = y*hudsize;
		if(t->frames.length() > 1) drawsized(cx-s/2.f, cy-s/2.f, s);
		else drawslice(0, clamp(amt, 0.f, 1.f), cx, cy, s);
	}

    void drawclip(int gun, float x, float y, float s, float fade)
    {
        const char *cliptexs[GUN_MAX] = {
            plasmacliptex, shotguncliptex, chainguncliptex,
            flamercliptex, carbinecliptex, riflecliptex, grenadescliptex,
        };
        Texture *t = textureload(cliptexs[gun], 3);
        int ammo = player1->ammo[gun], maxammo = guntype[gun].max;
		if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
        glColor4f(1.f, 1.f, 1.f, fade);

        glBindTexture(GL_TEXTURE_2D, t->retframe(ammo, maxammo));
		float cx = x*hudwidth, cy = y*hudsize;
        if(t->frames.length() > 1) drawsized(x-s/2.f, cy-s/2.f, s);
        else switch(gun)
        {
            case GUN_FLAMER:
				drawslice(0, max(ammo-min(maxammo-ammo, 2), 0)/float(maxammo), cx, cy, s);
				if(player1->ammo[gun] < guntype[gun].max)
					drawfadedslice(max(ammo-min(maxammo-ammo, 2), 0)/float(maxammo),
						min(min(maxammo-ammo, ammo), 2) /float(maxammo),
							cx-s/8.f, cy-s/8.f, s, fade);
                break;

            default:
                drawslice(0.5f/maxammo, ammo/float(maxammo), cx, cy, s);
                break;
        }
    }

	void drawpointer(int index, float x, float y, float s, float r, float g, float b, float fade)
	{
		Texture *t = textureload(getpointer(index), 3, true);
		if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
		glColor4f(r, g, b, fade);
		float cx = x*hudwidth, cy = y*hudsize;
		if(index != POINTER_GUI)
		{
			cx -= s/2.0f;
			cy -= s/2.0f;
		}
		glBindTexture(GL_TEXTURE_2D, t->id);
		drawsized(cx, cy, s);
	}

	void drawpointers(int w, int h)
	{
        int index = POINTER_NONE;

		if(UI::hascursor()) index = UI::hascursor(true) ? POINTER_GUI : POINTER_NONE;
        else if(hidehud || !showcrosshair || player1->state == CS_DEAD || !connected()) index = POINTER_NONE;
        else if(player1->state == CS_EDITING) index = POINTER_EDIT;
        else if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) index = POINTER_SPEC;
        else if(inzoom() && player1->gunselect == GUN_RIFLE) index = POINTER_SNIPE;
        else if(lastmillis-lasthit <= crosshairhitspeed) index = POINTER_HIT;
        else if(m_team(gamemode, mutators))
        {
            vec pos = headpos(player1, 0.f);
            dynent *d = intersectclosest(pos, worldpos, player1);
            if(d && d->type == ENT_PLAYER && ((gameent *)d)->team == player1->team)
				index = POINTER_TEAM;
			else index = POINTER_HAIR;
        }
        else index = POINTER_HAIR;

        if(index > POINTER_NONE)
        {
        	bool guicursor = index == POINTER_GUI;
			float s = (guicursor ? cursorsize : crosshairsize)*hudsize,
				r = 1.f, g = 1.f, b = 1.f, fade = guicursor ? cursorblend : crosshairblend;

        	if(player1->state == CS_ALIVE && index >= POINTER_HAIR)
        	{
				if(index == POINTER_SNIPE)
				{
					s = snipecrosshairsize*hudsize;
					if(inzoom() && player1->gunselect == GUN_RIFLE)
					{
						int frame = lastmillis-lastzoom;
						float amt = frame < zoomtime() ? clamp(float(frame)/float(zoomtime()), 0.f, 1.f) : 1.f;
						if(!zooming) amt = 1.f-amt;
						s *= amt;
					}
				}
				if(player1->health < MAXHEALTH/2)
				{
					r = 1.f;
					g = clamp(float(player1->health)/float(MAXHEALTH/2), 0.f, 1.f);
					b = 0.f;
				}
				else if(player1->health < MAXHEALTH)
				{
					r = 1.f;
					g = 1.f;
					b = clamp(float(player1->health-MAXHEALTH/2)/float(MAXHEALTH/2), 0.f, 1.f);
				}
				if(lastmillis-player1->lastregen < 500)
					fade *= clamp((lastmillis-player1->lastregen)/500.f, 0.1f, 1.f);
        	}

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, hudwidth, hudsize, 0, -1, 1);

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);

			float x = aimx, y = aimy;
			if(index < POINTER_EDIT || mousestyle() == 2)
			{
				x = cursorx;
				y = cursory;
			}
			else if(isthirdperson() ? thirdpersonaim : firstpersonaim)
				x = y = 0.5f;

			drawpointer(index, x, y, s, r, g, b, fade);

			if(index > POINTER_GUI)
			{
				if(player1->state == CS_ALIVE && player1->hasgun(player1->gunselect))
				{
					int gun = player1->gunselect;
					if(showclip)
					{
						float blend = clipblend;
						if(!player1->canshoot(gun, lastmillis) && player1->ammo[gun] > 0)
							blend *= clamp(float(lastmillis-player1->gunlast[gun])/float(player1->gunwait[gun]), 0.f, 1.f);

						drawclip(gun, x, y, s, blend);
					}
					if(showindicator && guntype[gun].power && player1->gunstate[gun] == GNS_POWER)
						drawindicator(gun, x, y, s);
				}

				if(mousestyle() >= 1)
					drawpointer(POINTER_RELATIVE, mousestyle()==1 ? cursorx : 0.5f, mousestyle()==1 ? cursory : 0.5f, s, r, g, b, fade);
			}

			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
        }
	}

	void drawquad(float x, float y, float w, float h, float tx1, float ty1, float tx2, float ty2)
	{
		glBegin(GL_QUADS);
		glTexCoord2f(tx1, ty1); glVertex2f(x, y);
		glTexCoord2f(tx2, ty1); glVertex2f(x+w, y);
		glTexCoord2f(tx2, ty2); glVertex2f(x+w, y+h);
		glTexCoord2f(tx1, ty2); glVertex2f(x, y+h);
		glEnd();
	}
	void drawtex(float x, float y, float w, float h, float tx, float ty, float tw, float th)
	{
		drawquad(x, y, w, h, tx, ty, tx+tw, ty+th);
	}
	void drawsized(float x, float y, float s)
	{
		drawtex(x, y, s, s);
	}

	void drawplayerblip(gameent *d, int w, int h, int s)
	{
		vec dir = headpos(d);
		dir.sub(camera1->o);
		float dist = dir.magnitude();
		if(dist < radarrange())
		{
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();

			int colour = teamtype[d->team].colour, r = colour>>16, g = (colour>>8)&0xFF, b = colour&0xFF;
			float fade = clamp(1.f-(dist/radarrange()), 0.f, 1.f)*radarblipblend;
			if(lastmillis-d->lastspawn <= REGENWAIT)
				fade *= clamp(float(lastmillis-d->lastspawn)/float(REGENWAIT), 0.f, 1.f);

			getradardir;
			settexture(radartex, 3);
			glColor4f(r/255.f, g/255.f, b/255.f, fade);
			drawtex(cx, cy, s, s, 0.25f, 0.25f, 0.25f, 0.25f);
			if(radarnames)
			{
				pushfont("radar");
				int tx = rd.axis ? int(cx+s*0.5f) : (rd.swap ? int(cx-s) : int(cx+s*2.f)),
					ty = rd.axis ? (rd.swap ? int(cy-s-FONTH) : int(cy+s*2.f)) : int(cy+s*0.5f-FONTH*0.5f),
					ta = rd.axis ? AL_CENTER : (rd.swap ? AL_RIGHT : AL_LEFT);
				draw_textx("%s", tx, ty, r, g, b, int(fade*255.f), false, ta, -1, -1, colorname(d, NULL, "", false));
				popfont();
			}
		}
	}

	void drawcardinalblips(int w, int h, int s)
	{
		pushfont("emphasis");
		loopi(4)
		{
			const char *card = "";
			vec dir(camera1->o);
			switch(i)
			{
				case 0: dir.sub(vec(0, 1, 0)); card = "N"; break;
				case 1: dir.add(vec(1, 0, 0)); card = "E"; break;
				case 2: dir.add(vec(0, 1, 0)); card = "S"; break;
				case 3: dir.sub(vec(1, 0, 0)); card = "W"; break;
				default: break;
			}
			dir.sub(camera1->o);
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();

			float cx = 0.f, cy = 0.f, yaw = 0.f, pitch = 0.f;
			findradardir;
			float cu = rd.axis ? fovsx : fovsy, cv = (fovsx*rd.x)+(fovsy*rd.y), ct = cv-cu, cw = (yaw-ct)/cu;
			if(rd.swap) (rd.axis ? cy : cx) += (rd.axis ? h-FONTH : w-FONTW);
			(rd.axis ? cx : cy) += (rd.axis ? w-FONTW : h-FONTH)*clamp(rd.up+(rd.down*cw), 0.f, 1.f);

			draw_textx("%s", int(cx), int(cy), 255, 255, 255, int(255*radarcardblend), true, AL_LEFT, -1, -1, card);
		}
		popfont();
	}

	void drawentblip(int w, int h, int s, int n, vec &o, int type, int attr1, int attr2, int attr3, int attr4, int attr5, bool spawned, int lastspawn)
	{
		if(type > NOTUSED && type < MAXENTTYPES && ((enttype[type].usetype == EU_ITEM && spawned) || player1->state == CS_EDITING))
		{
			bool insel = player1->state == CS_EDITING && entities::ents.inrange(n) && (enthover == n || entgroup.find(n) >= 0);
			float inspawn = spawned && lastspawn && lastmillis-lastspawn <= 1000 ? float(lastmillis-lastspawn)/1000.f : 0.f;
			if(enttype[type].noisy && (player1->state != CS_EDITING || !editradarnoisy || (editradarnoisy < 2 && !insel)))
				return;
			vec dir(o);
			dir.sub(camera1->o);
			float dist = dir.magnitude();
			if(dist >= radarrange())
			{
				if(insel || inspawn > 0.f) dir.mul(radarrange()/dist);
				else return;
			}
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();
			float range = (inspawn > 0.f ? 2.f-inspawn : 1.f)-(insel ? 1.f : (dist/radarrange())),
					fade = clamp(range, 0.1f, 1.f)*radarblipblend;

			getradardir;
			settexture(radartex, 3);
			if(inspawn > 0.f)
			{
				glColor4f(1.f, 0.5f+(inspawn*0.5f), 0.f, fade*(1.f-inspawn));
				drawtex(cx-(inspawn*s), cy-(inspawn*s), s+(inspawn*s*2.f), s+(inspawn*s*2.f), 0.25f, 0.25f, 0.25f, 0.25f);
			}
			glColor4f(1.f, insel ? 0.5f : 1.f, 0.f, fade);
			drawtex(cx-(insel ? s : 0), cy-(insel ? s : 0), s+(insel ? s*2 : 0), s+(insel ? s*2 : 0), 0.25f, 0.5f, 0.25f, 0.25f);
		}
	}

	void drawentblips(int w, int h, int s)
	{
		loopv(entities::ents)
		{
			gameentity &e = *(gameentity *)entities::ents[i];
			drawentblip(w, h, s, i, e.o, e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, e.spawned, e.lastspawn);
		}

		loopv(projs::projs) if(projs::projs[i]->projtype == PRJ_ENT && projs::projs[i]->ready())
		{
			projent &proj = *projs::projs[i];
			if(entities::ents.inrange(proj.id))
				drawentblip(w, h, s, -1, proj.o, proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4, proj.attr5, true, proj.spawntime);
		}
	}

	void drawtitlecard(int w, int h)
	{
		int ox = hudwidth, oy = hudsize;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);
		pushfont("emphasis");

		int bs = int(oy*titlecardsize), bp = int(oy*0.01f), bx = ox-bs-bp, by = bp,
			secs = maptime ? lastmillis-maptime : 0;
		float fade = hudblend, amt = 1.f;

		if(secs < titlecardtime)
		{
			amt = clamp(float(secs)/float(titlecardtime), 0.f, 1.f);
			fade = clamp(amt*fade, 0.f, 1.f);
		}
		else if(secs < titlecardtime+titlecardfade)
			fade = clamp(fade*(1.f-(float(secs-titlecardtime)/float(titlecardfade))), 0.f, 1.f);

		const char *title = getmaptitle();
		if(!*title) title = getmapname();

		int rs = int(bs*amt), rx = bx+(bs-rs), ry = by;
		glColor4f(1.f, 1.f, 1.f, fade*0.9f);
		if(!rendericon(getmapname(), rx, ry, rs, rs))
			rendericon("textures/emblem", rx, ry, rs, rs);
		glColor4f(1.f, 1.f, 1.f, fade);
		rendericon(guioverlaytex, rx, ry, rs, rs);

		int tx = bx + bs, ty = by + bs + FONTH/2, ts = int(tx*(1.f-amt));
		ty += draw_textx("%s", tx-ts, ty, 255, 255, 255, int(255.f*fade), false, AL_RIGHT, -1, tx-FONTH, server::gamename(gamemode, mutators));
		ty += draw_textx("%s", tx-ts, ty, 255, 255, 255, int(255.f*fade), false, AL_RIGHT, -1, tx-FONTH, title);
		popfont();
	}

	void drawgamehud(int w, int h)
	{
		Texture *t;
		int ox = hudwidth, oy = hudsize;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);

		int secs = maptime ? lastmillis-maptime : 0;
		float fade = hudblend;

		if(secs < titlecardtime+titlecardfade+titlecardfade)
		{
			float amt = clamp(float(secs-titlecardtime-titlecardfade)/float(titlecardfade), 0.f, 1.f);
			fade = clamp(fade*amt, 0.f, 1.f);
		}

		if(player1->state == CS_ALIVE && inzoom() && player1->gunselect == GUN_RIFLE)
		{
			t = textureload(snipetex);
			int frame = lastmillis-lastzoom;
			float pc = frame < zoomtime() ? float(frame)/float(zoomtime()) : 1.f;
			if(!zooming) pc = 1.f-pc;
			glBindTexture(GL_TEXTURE_2D, t->id);
			glColor4f(1.f, 1.f, 1.f, pc);
			drawtex(0, 0, ox, oy);
		}

		if(showdamage && ((player1->state == CS_ALIVE && damageresidue > 0) || player1->state == CS_DEAD))
		{
			t = textureload(damagetex);
			int dam = player1->state == CS_DEAD ? 100 : min(damageresidue, 100);
			float pc = float(dam)/100.f;
			glBindTexture(GL_TEXTURE_2D, t->id);
			glColor4f(1.f, 1.f, 1.f, pc);
			drawtex(0, 0, ox, oy);
		}

		if(showradar)
		{
			int os = int(oy*radarsize), qs = os/2, colour = teamtype[player1->team].colour,
				r = (colour>>16), g = ((colour>>8)&0xFF), b = (colour&0xFF);;

			glColor4f((r/255.f), (g/255.f), (b/255.f), fade*radarblend);
			settexture(radartex, 3);
			drawtex(os, os, os, os,					0.f, 0.f, 0.25f, 0.25f);		// TL
			drawtex(ox-(os*2), os, os, os,			0.75f, 0.f, 0.25f, 0.25f);		// TR
			drawtex(os, oy-(os*2), os, os,			0.f, 0.75f, 0.25f, 0.25f);		// BL
			drawtex(ox-(os*2), oy-(os*2), os, os,	0.75f, 0.75f, 0.25f, 0.25f);	// BR
			drawtex(os*2, os, ox-(os*4), os,		0.25f, 0.f, 0.5f, 0.25f);		// T
			drawtex(os*2, oy-(os*2), ox-(os*4), os,	0.25f, 0.75f, 0.5f, 0.25f);		// B
			drawtex(os, os*2, os, oy-(os*4),		0.f, 0.25f, 0.25f, 0.5f);		// L
			drawtex(ox-(os*2), os*2, os, oy-(os*4),	0.75f, 0.25f, 0.25f, 0.5f);		// R

			drawentblips(ox, oy, qs);
			loopv(players)
				if(players[i] && players[i]->state == CS_ALIVE)
					drawplayerblip(players[i], ox, oy, qs);
			if(m_stf(gamemode)) stf::drawblips(ox, oy, qs);
			else if(m_ctf(gamemode)) ctf::drawblips(ox, oy, qs);
			drawcardinalblips(ox, oy, qs);
		}
#if 0
		int tp = by + bs + FONTH/2;
		pushfont("emphasis");
		if(player1->state == CS_ALIVE)
		{
			if(showguns)
			{
				int ta = int(oy*ammosize), tb = ta*3, tv = bx + bs - tb,
					to = ta/16, tr = ta/2, tq = tr - FONTH/2;
				const char *hudtexs[GUN_MAX] = {
					plasmatex, shotguntex, chainguntex,
					flamertex, carbinetex, rifletex, grenadestex,
				};
				loopi(GUN_MAX) if(player1->hasgun(i) && (i == player1->gunselect || showguns > 1))
				{
					float blend = fade * (i == player1->gunselect ? ammoblend : ammoblendinactive);
					settexture(hudtexs[i], 0);
					glColor4f(1.f, 1.f, 1.f, blend);
					drawtex(float(tv), float(tp), float(tb), float(ta));
                    drawclip(i, tv+to/2, tp+to/2, ta-to, blend);
					if(i != player1->gunselect) pushfont("hud");
					int ts = tv + tr, tt = tp + tq;
					draw_textx("%s%d", ts, tt, 255, 255, 255, int(255.f*blend), false, AL_CENTER, -1, -1, player1->canshoot(i, lastmillis) ? "\fw" : "\fr", player1->ammo[i]);
					if(i != player1->gunselect) popfont();
					tp += ta;
				}
				tp += FONTH/2;
			}
			if(showtips)
			{
				tp = oy-FONTH;
				if(showtips > 1)
				{
					if(player1->hasgun(player1->gunselect))
					{
						const char *a = retbindaction("zoom", keym::ACTION_DEFAULT, 0);
						s_sprintfd(actkey)("%s", a && *a ? a : "ZOOM");
						tp -= draw_textx("Press [ \fs\fg%s\fS ] to %s", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey, player1->gunselect == GUN_RIFLE ? "zoom" : "prone");
					}
					if(player1->canshoot(player1->gunselect, lastmillis))
					{
						const char *a = retbindaction("attack", keym::ACTION_DEFAULT, 0);
						s_sprintfd(actkey)("%s", a && *a ? a : "ATTACK");
						tp -= draw_textx("Press [ \fs\fg%s\fS ] to attack", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey);
					}

					if(player1->canreload(player1->gunselect, lastmillis))
					{
						const char *a = retbindaction("reload", keym::ACTION_DEFAULT, 0);
						s_sprintfd(actkey)("%s", a && *a ? a : "RELOAD");
						tp -= draw_textx("Press [ \fs\fg%s\fS ] to load ammo", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey);
						if(weapons::autoreload > 1 && lastmillis-player1->gunlast[player1->gunselect] <= 1000)
							tp -= draw_textx("Autoreload in [ \fs\fg%.01f\fS ] second(s)", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, float(1000-(lastmillis-player1->gunlast[player1->gunselect]))/1000.f);
					}
				}

				vector<actitem> actitems;
				if(entities::collateitems(player1, actitems))
				{
					bool found = false;
					while(!actitems.empty())
					{
						actitem &t = actitems.last();
						int ent = -1;
						switch(t.type)
						{
							case ITEM_ENT:
							{
								if(!entities::ents.inrange(t.target)) break;
								ent = t.target;
								break;
							}
							case ITEM_PROJ:
							{
								if(!projs::projs.inrange(t.target)) break;
								projent &proj = *projs::projs[t.target];
								if(!entities::ents.inrange(proj.id)) break;
								ent = proj.id;
								break;
							}
							default: break;
						}
						if(entities::ents.inrange(ent))
						{
							const char *a = retbindaction("action", keym::ACTION_DEFAULT, 0);
							s_sprintfd(actkey)("%s", a && *a ? a : "ACTION");

							extentity &e = *entities::ents[ent];
							if(enttype[e.type].usetype == EU_ITEM)
							{
								if(!found)
								{
									int drop = -1;
									if(e.type == WEAPON && guntype[player1->gunselect].carry &&
										player1->ammo[e.attr1] < 0 && guntype[e.attr1].carry &&
											player1->carry() >= MAXCARRY) drop = player1->drop(e.attr1);
									if(isgun(drop))
									{
										s_sprintfd(dropgun)("%s", entities::entinfo(WEAPON, drop, player1->ammo[drop], 0, 0, 0, true));
										tp -= draw_textx("Press [ \fs\fg%s\fS ] to swap [ \fs%s\fS ] for [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey, dropgun, entities::entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, true));
									}
									else tp -= draw_textx("Press [ \fs\fg%s\fS ] to pickup [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey, entities::entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, true));
									if(showtips < 3) break;
									else found = true;
								}
								else tp -= draw_textx("Nearby [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, entities::entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, true));
							}
							else if(e.type == TRIGGER && e.attr3 == TA_ACT)
							{
								if(!found)
								{
									tp -= draw_textx("Press [ \fs\fg%s\fS ] to interact", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey);
									if(showtips < 3) break;
									else found = true;
								}
								else
									tp -= draw_textx("Nearby interactive item", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1);
							}
						}
						actitems.pop();
					}
				}
			}
		}
		else if(player1->state == CS_DEAD)
		{
			if(showtips)
			{
				tp = oy-FONTH;
				int wait = respawnwait(player1);
				if(wait)
					tp -= draw_textx("Fragged! Respawn available in [ \fs\fg%.01f\fS ] second(s)", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, float(wait)/1000.f);
				else
				{
					const char *a = retbindaction("attack", keym::ACTION_DEFAULT, 0);
					s_sprintfd(actkey)("%s", a && *a ? a : "ACTION");
					tp -= draw_textx("Fragged! Press [ \fs\fg%s\fS ] to respawn", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey);
				}
			}
		}
		else if(player1->state == CS_EDITING)
		{
			tp = oy-FONTH;
			if(showenttips) loopi(clamp(entgroup.length()+1, 0, showhudents))
			{
				int n = i ? entgroup[i-1] : enthover;
				if((!i || n != enthover) && entities::ents.inrange(n))
				{
					gameentity &f = (gameentity &)*entities::ents[n];
					if(showenttips <= 2 && n != enthover) pushfont("hud");
					tp -= draw_textx("entity %d, %s", bx+bs, tp,
						n == enthover ? 255 : 196, 196, 196, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1,
							n, entities::findname(f.type));
					if(showenttips <= 2 && n != enthover) popfont();
					if(showenttips > 1 || n == enthover)
					{
						tp -= draw_textx("%s (%d %d %d %d %d)", bx+bs, tp,
							255, 196, 196, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1,
								entities::entinfo(f.type, f.attr1, f.attr2, f.attr3, f.attr4, f.attr5, true),
									f.attr1, f.attr2, f.attr3, f.attr4, f.attr5);
					}
				}
			}
		}
		popfont(); // emphasis
#endif
	}

	void drawhudelements(int w, int h)
	{
		int ox = hudwidth, oy = hudsize, bx = showradar ? int(oy*radarsize) : 0, by = oy-bx-FONTH/4;
		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);

		pushfont("hud");
		renderconsole(ox, oy, bx);

		static int laststats = 0, prevstats[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, curstats[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		if(totalmillis-laststats >= statrate)
		{
			memcpy(prevstats, curstats, sizeof(prevstats));
			laststats = totalmillis-(totalmillis%statrate);
		}

		int nextstats[12] =
		{
			vtris*100/max(wtris, 1),
			vverts*100/max(wverts, 1),
			xtraverts/1024,
			xtravertsva/1024,
			glde,
			gbatches,
			getnumqueries(),
			rplanes,
			curfps,
			bestfpsdiff,
			worstfpsdiff,
			autoadjustlevel
		};

		loopi(12) if(prevstats[i] == curstats[i]) curstats[i] = nextstats[i];

		if(showstats)
		{
			by -= draw_textx("ond:%d va:%d gl:%d(%d) oq:%d lm:%d rp:%d pvs:%d", bx+FONTW/2, by-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6], lightmaps.length(), curstats[7], getnumviewcells());
			by -= draw_textx("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", bx+FONTW/2, by-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
		}

		if(showfps) switch(showfps)
		{
			case 2:
				if(autoadjust) by -= draw_textx("fps:%d (%d/%d) +%d-%d [\fs%s%d%%\fS]", bx+FONTW/2, by-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], autoadjustfps, maxfps, curstats[9], curstats[10], curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
				else by -= draw_textx("fps:%d (%d) +%d-%d", bx+FONTW/2, by-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], maxfps, curstats[9], curstats[10]);
				break;
			case 1:
				if(autoadjust) by -= draw_textx("fps:%d (%d/%d) [\fs%s%d%%\fS]", bx+FONTW/2, by-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], autoadjustfps, maxfps, curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
				else by -= draw_textx("fps:%d (%d)", bx+FONTW/2, by-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], maxfps);
				break;
			default: break;
		}

		if(getcurcommand())
			by -= rendercommand(bx+FONTW/2, by-FONTH, hudwidth-FONTW-bx*2);

		if(connected() && maptime)
		{
			if(player1->state == CS_EDITING)
			{
				by -= draw_textx("sel:%d,%d,%d %d,%d,%d (%d,%d,%d,%d)", bx+FONTW/2, by-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1,
						sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z,
							sel.cx, sel.cxs, sel.cy, sel.cys);
				by -= draw_textx("corner:%d orient:%d grid:%d", bx+FONTW/2, by-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1,
								sel.corner, sel.orient, sel.grid);
				by -= draw_textx("cube:%s%d ents:%d", bx+FONTW/2, by-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1,
					selchildcount<0 ? "1/" : "", abs(selchildcount), entgroup.length());
			}

			render_texture_panel(w, h);
		}
		popfont(); // emphasis
	}

	void drawhud(int w, int h)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if(maptime && connected())
		{
			if(lastmillis-maptime < titlecardtime+titlecardfade)
				drawtitlecard(w, h);
			else drawgamehud(w, h);
		}
		drawhudelements(w, h);
		glDisable(GL_BLEND);
	}

	void lighteffects(dynent *e, vec &color, vec &dir)
	{
	}

    void particletrack(particle *p, uint type, int &ts, vec &o, vec &d, bool lastpass)
    {
        if(!p->owner || p->owner->type != ENT_PLAYER) return;

        switch(type&0xFF)
        {
        	case PT_PART: case PT_TAPE: case PT_FIREBALL: case PT_LIGHTNING: case PT_FLARE:
        	{
				o = ((gameent *)p->owner)->muzzle;
				break;
        	}
        	default: break;
        }
    }

	void newmap(int size)
	{
		client::addmsg(SV_NEWMAP, "ri", size);
	}

	void gamemenus()
	{
		sb.show();
	}

	void loadworld(gzFile &f, int maptype)
	{
	}

	void saveworld(gzFile &f)
	{
	}

	bool gethudcolour(vec &colour)
	{
		if(!maptime || lastmillis-maptime < titlecardtime)
		{
			float fade = maptime ? float(lastmillis-maptime)/float(titlecardtime) : 0.f;
			if(fade < 1.f)
			{
				colour = vec(fade, fade, fade);
				return true;
			}
		}
		if(tvmode())
		{
			float fade = 1.f;
			int millis = spectvtime ? min(spectvtime/10, 500) : 500, interval = lastmillis-lastspec;
			if(!lastspec || interval < millis)
				fade = lastspec ? float(interval)/float(millis) : 0.f;
			else if(spectvtime && interval > spectvtime-millis)
				fade = float(spectvtime-interval)/float(millis);
			if(fade < 1.f)
			{
				colour = vec(fade, fade, fade);
				return true;
			}
		}
		return false;
	}

	vec headpos(physent *d, float off)
	{
		vec pos(d->o);
		pos.z -= off;
		return pos;
	}

	vec feetpos(physent *d, float off)
	{
 		vec pos(d->o);
		if(d->type == ENT_PLAYER) pos.z += off-d->height;
		return pos;
	}

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
		curfov = float(fov());

		if(inzoom())
		{
			int frame = lastmillis-lastzoom,
				t = player1->gunselect == GUN_RIFLE ? snipetime : pronetime,
				f = player1->gunselect == GUN_RIFLE ? snipefov : pronefov;
			float diff = float(fov()-f),
				amt = frame < t ? clamp(float(frame)/float(t), 0.f, 1.f) : 1.f;
			if(!zooming) amt = 1.f-amt;
			curfov -= amt*diff;
		}

        aspect = w/float(h);
        fovy = 2*atan2(tan(curfov/2*RAD), aspect)/RAD;
        hudwidth = int(hudsize*aspect);
	}

	bool mousemove(int dx, int dy, int x, int y, int w, int h)
	{
		bool hascursor = UI::hascursor();
		#define mousesens(a,b,c) ((float(a)/float(b))*c)
		if(hascursor || mousestyle() >= 1)
		{
			if(absmouse) // absolute positions, unaccelerated
			{
				cursorx = clamp(float(x)/float(w), 0.f, 1.f);
				cursory = clamp(float(y)/float(h), 0.f, 1.f);
				return false;
			}
			else
			{
				cursorx = clamp(cursorx+mousesens(dx, w, mousesensitivity), 0.f, 1.f);
				cursory = clamp(cursory+mousesens(dy, h, mousesensitivity*(!hascursor && invmouse ? -1.f : 1.f)), 0.f, 1.f);
				return true;
			}
		}
		else
		{
			if(allowmove(player1))
			{
				float scale = inzoom() ?
						(player1->gunselect == GUN_RIFLE ? snipesensitivity : pronesensitivity)
					: sensitivity;
				player1->yaw += mousesens(dx, w, yawsensitivity*scale);
				player1->pitch -= mousesens(dy, h, pitchsensitivity*scale*(!hascursor && invmouse ? -1.f : 1.f));
				fixfullrange(player1->yaw, player1->pitch, player1->roll, false);
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
		if(style >= 0)
		{
			int aim = isthirdperson() ? thirdpersonaim : firstpersonaim;
			if(aim)
			{
				float ax, ay, az;
				vectocursor(worldpos, ax, ay, az);
				float amt = float(curtime)/float(aim),
					  offx = ax-aimx, offy = ay-aimy;
				aimx += offx*amt;
				aimy += offy*amt;
			}
			else
			{
				aimx = cursorx;
				aimy = cursory;
			}
			float vx = mousestyle() <= 1 ? aimx : cursorx,
				vy = mousestyle() <= 1 ? aimy : cursory;
			vecfromcursor(vx, vy, 1.f, cursordir);
		}
	}

	void scaleyawpitch(float &yaw, float &pitch, float newyaw, float newpitch, int frame, int speed)
	{
		if(speed && frame)
		{
			float amt = float(lastmillis-frame)/float(speed),
				offyaw = newyaw-yaw, offpitch = newpitch-pitch;
			if(offyaw > 180.f) offyaw -= 360.f;
			if(offyaw < -180.f) offyaw += 360.f;
			yaw += offyaw*amt;
			pitch += offpitch*amt;
		}
		else
		{
			yaw = newyaw;
			pitch = newpitch;
		}
		fixrange(yaw, pitch);
	}

	struct camstate
	{
		int ent, idx;
		vec pos, dir;
		vector<int> cansee;
		float mindist, maxdist, score;

		camstate() : idx(-1), mindist(32.f), maxdist(512.f) { reset(); }
		~camstate() {}

		void reset()
		{
			cansee.setsize(0);
			dir = vec(0, 0, 0);
			score = 0.f;
		}
	};
	vector<camstate> cameras;

	static int camerasort(const camstate *a, const camstate *b)
	{
		int asee = a->cansee.length(), bsee = b->cansee.length();
		if(asee > bsee) return -1;
		if(asee < bsee) return 1;
		if(a->score < b->score) return -1;
		if(a->score > b->score) return 1;
		return 0;
	}

	void recomputecamera(int w, int h)
	{
		fixview(w, h);

		camera1 = &camera;

		if(camera1->type != ENT_CAMERA)
		{
			camera1->reset();
			camera1->type = ENT_CAMERA;
			camera1->state = CS_ALIVE;
			camera1->height = camera1->radius = camera1->xradius = camera1->yradius = 1;
		}

		if(connected() && maptime)
		{
			if(!lastcamera)
			{
				resetcursor();
				cameras.setsize(0);
				lastspec = 0;
				if(mousestyle() == 2)
				{
					camera1->yaw = player1->aimyaw = player1->yaw;
					camera1->pitch = player1->aimpitch = player1->pitch;
				}
			}
			if(tvmode())
			{
				if(cameras.empty()) loopk(2)
				{
					physent d = *camera1;
					loopv(entities::ents) if(entities::ents[i]->type == (k ? LIGHT : CAMERA))
					{
						d.o = entities::ents[i]->o;
						if(physics::entinmap(&d, false))
						{
							camstate &c = cameras.add();
							c.pos = entities::ents[i]->o;
							c.ent = i;
							if(!k)
							{
								c.idx = entities::ents[i]->attr1;
								if(entities::ents[i]->attr2) c.mindist = entities::ents[i]->attr2;
								if(entities::ents[i]->attr3) c.maxdist = entities::ents[i]->attr3;
							}
						}
					}
					lastspec = 0;
					if(!cameras.empty()) break;
				}
				if(!cameras.empty())
				{
					bool renew = !lastspec || (spectvtime && lastmillis-lastspec >= spectvtime);
					loopvj(cameras)
					{
						camstate &c = cameras[j];
						loopk(2)
						{
							vec avg(0, 0, 0);
							gameent *d;
							c.reset();
							loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && (d->state == CS_ALIVE || d->state == CS_DEAD))
							{
								vec trg, pos = feetpos(d);
								float dist = c.pos.dist(pos);
								if((k || dist >= c.mindist) && dist <= c.maxdist && raycubelos(c.pos, pos, trg))
								{
									c.cansee.add(i);
									avg.add(pos);
								}
							}
							float yaw = camera1->yaw, pitch = camera1->pitch;
							#define updatecamorient \
							{ \
								if((k || j) && c.cansee.length()) \
								{ \
									vec dir = vec(avg).div(c.cansee.length()).sub(c.pos).normalize(); \
									vectoyawpitch(dir, yaw, pitch); \
								} \
							}
							updatecamorient;
							loopvrev(c.cansee) if((d = (gameent *)iterdynents(c.cansee[i])))
							{
								vec trg, pos = feetpos(d);
								if(getsight(c.pos, yaw, pitch, pos, trg, c.maxdist, curfov, fovy))
								{
									c.dir.add(pos);
									c.score += c.pos.dist(pos);
								}
								else
								{
									avg.sub(pos);
									c.cansee.removeunordered(i);
									updatecamorient;
								}
							}
							if(!j || !c.cansee.empty()) break;
						}
						if(!c.cansee.empty())
						{
							float amt = float(c.cansee.length());
							c.dir.div(amt);
							c.score /= amt;
						}
						else if(!j) renew = true; // quick scotty, get a new cam
						if(!renew) break; // only update first camera then
					}
					camstate *cam = &cameras[0], *oldcam = cam;
					if(renew)
					{
						cameras.sort(camerasort);
						lastspec = lastmillis;
						cam = &cameras[0];
					}
					player1->o = camera1->o = cam->pos;
					vec dir = vec(cam->dir).sub(camera1->o).normalize();
					vectoyawpitch(dir, camera1->yaw, camera1->pitch);
					if(cam == oldcam)
					{
						float amt = float(curtime)/1000.f,
							offyaw = fabs(camera1->yaw-camera1->aimyaw)*amt, offpitch = fabs(camera1->pitch-camera1->aimpitch)*amt*0.25f;

						if(camera1->yaw > camera1->aimyaw)
						{
							camera1->aimyaw += offyaw;
							if(camera1->yaw < camera1->aimyaw) camera1->aimyaw = camera1->yaw;
						}
						else if(camera1->yaw < camera1->aimyaw)
						{
							camera1->aimyaw -= offyaw;
							if(camera1->yaw > camera1->aimyaw) camera1->aimyaw = camera1->yaw;
						}

						if(camera1->pitch > camera1->aimpitch)
						{
							camera1->aimpitch += offpitch;
							if(camera1->pitch < camera1->aimpitch) camera1->aimpitch = camera1->pitch;
						}
						else if(camera1->pitch < camera1->aimpitch)
						{
							camera1->aimpitch -= offpitch;
							if(camera1->pitch > camera1->aimpitch) camera1->aimpitch = camera1->pitch;
						}
						camera1->yaw = camera1->aimyaw;
						camera1->pitch = camera1->aimpitch;
					}
					player1->yaw = player1->aimyaw = camera1->aimyaw = camera1->yaw;
					player1->pitch = player1->aimpitch = camera1->aimpitch = camera1->pitch;
				}
			}
			else
			{
				vec pos = headpos(player1, 0.f);

				if(mousestyle() <= 1)
					findorientation(pos, player1->yaw, player1->pitch, worldpos);

				camera1->o = pos;

				if(isthirdperson())
				{
					float angle = thirdpersonangle ? 0-thirdpersonangle : player1->pitch;
					camera1->aimyaw = mousestyle() <= 1 ? player1->yaw : player1->aimyaw;
					camera1->aimpitch = mousestyle() <= 1 ? angle : player1->aimpitch;

					#define cameramove(d,s) \
						if(d) \
						{ \
							camera1->move = !s ? (d > 0 ? -1 : 1) : 0; \
							camera1->strafe = s ? (d > 0 ? -1 : 1) : 0; \
							loopi(10) if(!physics::moveplayer(camera1, 10, true, abs(d))) break; \
						}
					cameramove(thirdpersondist, false);
					cameramove(thirdpersonshift, true);
				}
				else
				{
					camera1->aimyaw = mousestyle() <= 1 ? player1->yaw : player1->aimyaw;
					camera1->aimpitch = mousestyle() <= 1 ? player1->pitch : player1->aimpitch;
				}

				switch(mousestyle())
				{
					case 0:
					case 1:
					{
						if(!thirdpersonaim && isthirdperson())
						{
							vec dir(worldpos);
							dir.sub(camera1->o);
							dir.normalize();
							vectoyawpitch(dir, camera1->yaw, camera1->pitch);
							fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
						}
						else
						{
							camera1->yaw = player1->yaw;
							camera1->pitch = player1->pitch;
						}
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
						findorientation(isthirdperson() ? camera1->o : pos, yaw, pitch, worldpos);
						if(allowmove(player1))
						{
							if(isthirdperson())
							{
								vec dir(worldpos);
								dir.sub(camera1->o);
								dir.normalize();
								vectoyawpitch(dir, player1->yaw, player1->pitch);
							}
							else
							{
								player1->yaw = yaw;
								player1->pitch = pitch;
							}
						}
						break;
					}
				}

				fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
				fixrange(camera1->aimyaw, camera1->aimpitch);

				if(allowmove(player1))
				{
					if(isthirdperson())
					{
						vec dir(worldpos);
						dir.sub(pos);
						dir.normalize();
						vectoyawpitch(dir, player1->aimyaw, player1->aimpitch);
					}
					else
					{
						player1->aimyaw = camera1->yaw;
						player1->aimpitch = camera1->pitch;
					}
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

			if(quakewobble > 0)
				camera1->roll = float(rnd(21)-10)*(float(min(quakewobble, 100))/100.f);
			else camera1->roll = 0;

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

	void adddynlights()
	{
		projs::adddynlights();
		entities::adddynlights();
	}

	vector<gameent *> bestplayers;
    vector<int> bestteams;

	VAR(animoverride, -1, 0, ANIM_MAX-1);
	VAR(testanims, 0, 0, 1);

	int numanims() { return ANIM_MAX; }

	void findanims(const char *pattern, vector<int> &anims)
	{
		loopi(sizeof(animnames)/sizeof(animnames[0]))
			if(*animnames[i] && matchanim(animnames[i], pattern))
				anims.add(i);
	}

	void renderclient(gameent *d, bool third, bool trans, int team, modelattach *attachments, bool secondary, int animflags, int animdelay, int lastaction, float speed, bool early)
	{
		string mdl;
		if(third) s_strcpy(mdl, teamtype[team].tpmdl);
		else s_strcpy(mdl, teamtype[team].fpmdl);

		float yaw = d->yaw, pitch = d->pitch, roll = d->roll;
		vec o = vec(third ? feetpos(d) : headpos(d));
		if(!third)
		{
			vec dir;
			if(firstpersonsway != 0.f)
			{
				vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
				float swayspeed = min(4.f, sqrtf(d->vel.x*d->vel.x + d->vel.y*d->vel.y));
				dir.mul(swayspeed);
				float swayxy = sinf(swaymillis/115.0f)/float(firstpersonsway),
					  swayz = cosf(swaymillis/115.0f)/float(firstpersonsway);
				swap(dir.x, dir.y);
				dir.x *= -swayxy;
				dir.y *= swayxy;
				dir.z = -fabs(swayspeed*swayz);
				dir.add(swaydir);
				o.add(dir);
			}
			if(firstpersondist != 0.f)
			{
				vecfromyawpitch(yaw, pitch, 1, 0, dir);
				dir.mul(player1->radius*firstpersondist);
				o.add(dir);
			}
			if(firstpersonshift != 0.f)
			{
				vecfromyawpitch(yaw, pitch, 0, -1, dir);
				dir.mul(player1->radius*firstpersonshift);
				o.add(dir);
			}
			if(firstpersonadjust != 0.f)
			{
				vecfromyawpitch(yaw, pitch+90.f, 1, 0, dir);
				dir.mul(player1->height*firstpersonadjust);
				o.add(dir);
			}
		}

		int anim = animflags, basetime = lastaction;
		if(animoverride)
		{
			anim = (animoverride<0 ? ANIM_ALL : animoverride)|ANIM_LOOP;
			basetime = 0;
		}
		else
		{
			if(secondary)
			{
				if(d->inliquid && d->physstate <= PHYS_FALL)
					anim |= (((allowmove(d) && (d->move || d->strafe)) || d->vel.z+d->falling.z>0 ? ANIM_SWIM : ANIM_SINK)|ANIM_LOOP)<<ANIM_SECONDARY;
				else if(d->timeinair > 1000)
					anim |= (ANIM_JUMP|ANIM_END)<<ANIM_SECONDARY;
				else if(d->crouching)
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
				case ANIM_IDLE: case ANIM_PLASMA: case ANIM_SHOTGUN: case ANIM_CHAINGUN:
				case ANIM_GRENADES: case ANIM_FLAMER: case ANIM_CARBINE: case ANIM_RIFLE:
				{
					anim >>= ANIM_SECONDARY;
					break;
				}
				default: break;
			}
		}

		if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX)) switch(anim&ANIM_INDEX)
		{
			case ANIM_IDLE: case ANIM_PLASMA: case ANIM_SHOTGUN: case ANIM_CHAINGUN:
			case ANIM_GRENADES: case ANIM_FLAMER: case ANIM_CARBINE: case ANIM_RIFLE:
			{
				anim |= ((anim&ANIM_INDEX)|ANIM_LOOP)<<ANIM_SECONDARY;
				break;
			}
			default:
			{
				anim |= (ANIM_IDLE|ANIM_LOOP)<<ANIM_SECONDARY;
				break;
			}
		}

		int flags = MDL_LIGHT;
		if(d != player1) flags |= MDL_CULL_VFC | MDL_CULL_OCCLUDED | MDL_CULL_QUERY;
		if(d->type != ENT_PLAYER) flags |= MDL_CULL_DIST;
        if(early) flags |= MDL_NORENDER;
		else if(trans) flags |= MDL_TRANSLUCENT;
		else if(third && (anim&ANIM_INDEX)!=ANIM_DEAD) flags |= MDL_DYNSHADOW;
		dynent *e = third ? (dynent *)d : (dynent *)&fpsmodel;
		rendermodel(NULL, mdl, anim, o, !third && testanims && d == player1 ? 0 : yaw+90, pitch, roll, flags, e, attachments, basetime, speed);
	}

	void renderplayer(gameent *d, bool third, bool trans, bool early = false)
	{
        modelattach a[4];
		int ai = 0, team = m_team(gamemode, mutators) ? d->team : TEAM_NEUTRAL,
			gun = d->gunselect, lastaction = lastmillis,
			animflags = ANIM_IDLE|ANIM_LOOP, animdelay = 0;
		bool secondary = false, showgun = isgun(gun);

		if(d->state == CS_SPECTATOR || d->state == CS_WAITING) return;
		else if(d->state == CS_DEAD)
		{
			if(d->obliterated) return; // not shown at all
			showgun = false;
			animflags = ANIM_DYING;
			lastaction = d->lastpain;
			int t = lastmillis-lastaction;
			if(t < 0) return;
			if(t > 1000) animflags = ANIM_DEAD|ANIM_LOOP;
        }
		else if(d->state == CS_EDITING)
		{
			animflags = ANIM_EDIT|ANIM_LOOP;
		}
		else if(d->state == CS_LAGGED)
		{
			animflags = ANIM_LAG|ANIM_LOOP;
		}
		else if(intermission)
		{
			lastaction = lastmillis;
			animflags = ANIM_LOSE|ANIM_LOOP;
			animdelay = 1000;
			if(m_team(gamemode, mutators))
			{
				loopv(bestteams) if(bestteams[i] == d->team)
				{
					animflags = ANIM_WIN|ANIM_LOOP;
					break;
				}
			}
			else if(bestplayers.find(d) >= 0) animflags = ANIM_WIN|ANIM_LOOP;
		}
		else if(d->lasttaunt && lastmillis-d->lasttaunt <= 1000)
		{
			lastaction = d->lasttaunt;
			animflags = ANIM_TAUNT;
			animdelay = 1000;
		}
		else if(lastmillis-d->lastpain <= 300)
		{
			secondary = third && allowmove(d);
			lastaction = d->lastpain;
			animflags = ANIM_PAIN;
			animdelay = 300;
		}
		else
		{
			secondary = third && allowmove(d);
			if(showgun)
			{
				int gunstate = GNS_IDLE;
				if(lastmillis-d->gunlast[gun] <= d->gunwait[gun])
				{
					gunstate = d->gunstate[gun];
					lastaction = d->gunlast[gun];
					animdelay = d->gunwait[gun];
				}
				switch(gunstate)
				{
					case GNS_SWITCH:
					{
						if(lastmillis-d->gunlast[gun] <= d->gunwait[gun]/2)
						{
							if(d->hasgun(d->lastgun)) gun = d->lastgun;
							else showgun = false;
						}
						animflags = ANIM_SWITCH;
						break;
					}
					case GNS_POWER:
					{
						if(!guntype[gun].power) gunstate = GNS_SHOOT;
						animflags = (guntype[gun].anim+gunstate);
						break;
					}
					case GNS_SHOOT:
					{
						if(guntype[gun].power) showgun = false;
						animflags = (guntype[gun].anim+gunstate);
						break;
					}
					case GNS_RELOAD:
					{
						if(guntype[gun].power) showgun = false;
						animflags = (guntype[gun].anim+gunstate);
						break;
					}
					case GNS_IDLE:	default:
					{
						if(!d->hasgun(gun)) showgun = false;
						else animflags = (guntype[gun].anim+gunstate)|ANIM_LOOP;
						break;
					}
				}
			}
		}

		if(shownamesabovehead && third && d != player1)
		{
			s_sprintfd(s)("@%s", colorname(d));
			part_text(d->abovehead(), s);
		}


		if(showgun)
		{ // we could probably animate the vwep too now..
			a[ai].name = guntype[gun].vwep;
			a[ai].tag = "tag_weapon";
			a[ai].anim = ANIM_VWEP|ANIM_LOOP;
			a[ai].basetime = 0;
			ai++;
		}
		if(third)
		{
			if(m_ctf(gamemode))
			{
				loopv(ctf::st.flags) if(ctf::st.flags[i].owner == d && !ctf::st.flags[i].droptime)
				{
					a[ai].name = teamtype[ctf::st.flags[i].team].flag;
					a[ai].tag = "tag_flag";
					a[ai].anim = ANIM_MAPMODEL|ANIM_LOOP;
					a[ai].basetime = 0;
					ai++;
				}
			}
		}

        if(rendernormally && (early || d != player1))
        {
            d->muzzle = vec(-1, -1, -1);
            a[ai].tag = "tag_muzzle";
            a[ai].pos = &d->muzzle;
            ai++;
        }

        renderclient(d, third, trans, team, a[0].name ? a : NULL, secondary, animflags, animdelay, lastaction, 0.f, early);
	}

	void render()
	{
		if(intermission)
		{
			if(m_team(gamemode, mutators)) { bestteams.setsize(0); sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); sb.bestplayers(bestplayers); }
		}

		startmodelbatches();

		gameent *d;
        loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d != player1)
        {
			if(d->state!=CS_SPECTATOR && d->state!=CS_WAITING && d->state!=CS_SPAWNING && (d->state!=CS_DEAD || !d->obliterated))
				renderplayer(d, true, d->state == CS_LAGGED || (d->state == CS_ALIVE && lastmillis-d->lastspawn <= REGENWAIT));
        }

		entities::render();
		projs::render();
		if(m_stf(gamemode)) stf::render();
        else if(m_ctf(gamemode)) ctf::render();
        ai::render();

		endmodelbatches();
	}

    void renderavatar(bool early)
    {
        if(inzoomswitch() && player1->gunselect == GUN_RIFLE) return;
        if(isthirdperson() || !rendernormally)
        {
            if(player1->state!=CS_SPECTATOR && player1->state!=CS_WAITING && (player1->state!=CS_DEAD || !player1->obliterated))
                renderplayer(player1, true, (player1->state == CS_ALIVE && lastmillis-player1->lastspawn <= REGENWAIT) || thirdpersontranslucent, early);
        }
        else if(player1->state == CS_ALIVE)
        {
            renderplayer(player1, false, (lastmillis-player1->lastspawn <= REGENWAIT) || firstpersontranslucent, early);
        }
    }

	bool clientoption(char *arg) { return false; }
}
#undef GAMEWORLD
