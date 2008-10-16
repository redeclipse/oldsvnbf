#include "pch.h"
#include "engine.h"
#include "game.h"
#include "server.h"
#ifndef STANDALONE
struct gameclient : igameclient
{
	#include "physics.h"
	#include "projs.h"
	#include "weapon.h"
	#include "scoreboard.h"
	#include "entities.h"
	#include "client.h"
	#include "ai.h"
	#include "stf.h"
    #include "ctf.h"

	int nextmode, nextmuts, nextstyle, gamemode, mutators, gamestyle;
	bool intermission;
	int maptime, minremain, swaymillis;
	dynent fpsmodel;
	vec swaydir;
    int lasthit, lastcamera, lastzoom, lastmousetype;
    bool prevzoom, zooming;
	int quakewobble, damageresidue;
    int liquidchan;

	gameent *player1;				// our client
	vector<gameent *> players;		// other clients
	gameent lastplayerstate;

	IVARP(titlecardtime, 0, 2000, 10000);
	IVARP(titlecardfade, 0, 3000, 10000);
	IFVARP(titlecardsize, 0.3f);

	IVARP(invmouse, 0, 0, 1);
	IVARP(absmouse, 0, 0, 1);

	IVARP(thirdperson, 0, 0, 1);

	IVARP(thirdpersonmouse, 0, 0, 2);
	IVARP(thirdpersondeadzone, 0, 10, 100);
	IVARP(thirdpersonpanspeed, 1, 30, INT_MAX-1);
	IVARP(thirdpersonaim, 0, 250, INT_MAX-1);
	IVARP(thirdpersonfov, 90, 120, 150);
	IVARP(thirdpersontranslucent, 0, 0, 1);
	IVARP(thirdpersondist, -100, 1, 100);
	IVARP(thirdpersonshift, -100, 4, 100);
	IVARP(thirdpersonangle, 0, 40, 360);

	IVARP(firstpersonmouse, 0, 0, 2);
	IVARP(firstpersondeadzone, 0, 10, 100);
	IVARP(firstpersonpanspeed, 1, 30, INT_MAX-1);
	IVARP(firstpersonfov, 90, 100, 150);
	IVARP(firstpersonaim, 0, 0, INT_MAX-1);
	IVARP(firstpersonsway, 0, 100, INT_MAX-1);
	IVARP(firstpersontranslucent, 0, 0, 1);
	IFVARP(firstpersondist, -0.25f);
	IFVARP(firstpersonshift, 0.25f);
	IFVARP(firstpersonadjust, 0.f);

	IVARP(sidescrollfov, 90, 100, 150);
	IVARW(sspcameradist, 16, 64, INT_MAX-1);

	IVARP(editmouse, 0, 0, 2);
	IVARP(editfov, 1, 120, 360);
	IVARP(editdeadzone, 0, 10, 100);
	IVARP(editpanspeed, 1, 20, INT_MAX-1);

	IVARP(specmouse, 0, 0, 2);
	IVARP(specfov, 1, 120, 360);
	IVARP(specdeadzone, 0, 10, 100);
	IVARP(specpanspeed, 1, 20, INT_MAX-1);

	IFVARP(sensitivity, 10.0f);
	IFVARP(yawsensitivity, 10.0f);
	IFVARP(pitchsensitivity, 7.5f);
	IFVARP(mousesensitivity, 1.0f);
	IFVARP(snipesensitivity, 3.0f);
	IFVARP(pronesensitivity, 5.0f);

	IVARP(crosshairclip, 0, 1, 1);
	IVARP(crosshairhitspeed, 0, 1000, INT_MAX-1);
	IFVARP(crosshairsize, 0.05f);
	IFVARP(cursorsize, 0.05f);
	IFVARP(cursorblend, 1.f);

	IFVARP(crosshairblend, 0.5f);
	IFVARP(indicatorblend, 1.f);
	IFVARP(clipbarblend, 1.f);
	IFVARP(radarblend, 0.75f);
	IFVARP(blipblend, 1.0f);
	IFVARP(barblend, 1.0f);
	IFVARP(candinalblend, 1.0f);
	IFVARP(ammoblend, 1.f);
	IFVARP(ammoblendinactive, 0.5f);
	IFVARP(infoblend, 1.f);

	IVARP(radardist, 0, 512, 512);
	IVARP(radarnames, 0, 1, 2);
	IFVARP(radarsize, 0.33f);
	IFVARP(ammosize, 0.07f);
	IVARP(editradardist, 0, 512, INT_MAX-1);
	IVARP(editradarnoisy, 0, 1, 2);

	IVARP(showcrosshair, 0, 1, 1);
	IVARP(showdamage, 0, 1, 1);
	IVARP(showhudinfo, 0, 1, 2);
	IVARP(showhudammo, 0, 2, 2);
	IVARP(shownameinfo, 0, 1, 2);
	IVARP(showindicator, 0, 1, 1);

	IVARP(showstats, 0, 0, 1);
	IVARP(showhudentinfo, 0, 1, 2);
	IVARP(showhudents, 0, 10, 100);
	IVARP(showfps, 0, 2, 2);
	IVARP(statrate, 0, 200, 1000);

	IVARP(snipetype, 0, 0, 1);
	IVARP(snipemouse, 0, 0, 2);
	IVARP(snipedeadzone, 0, 25, 100);
	IVARP(snipepanspeed, 1, 10, INT_MAX-1);
	IVARP(snipefov, 1, 35, 150);
	IVARP(snipetime, 1, 300, 10000);
	IFVARP(snipecrosshairsize, 0.5f);

	IVARP(pronetype, 0, 0, 1);
	IVARP(pronemouse, 0, 0, 2);
	IVARP(pronedeadzone, 0, 25, 100);
	IVARP(pronepanspeed, 1, 10, INT_MAX-1);
	IVARP(pronefov, 70, 70, 150);
	IVARP(pronetime, 1, 150, 10000);

	ITVAR(relativecursortex, "textures/relativecursor", 3);
	ITVAR(guicursortex, "textures/guicursor", 3);
	ITVAR(editcursortex, "textures/editcursor", 3);
	ITVAR(crosshairtex, "textures/crosshair", 3);
	ITVAR(teamcrosshairtex, "textures/teamcrosshair", 3);
	ITVAR(hitcrosshairtex, "textures/hitcrosshair", 3);
	ITVAR(snipecrosshairtex, "textures/snipecrosshair", 3);

	ITVAR(bliptex, "textures/blip", 3);
	ITVAR(flagbliptex, "textures/flagblip", 3);
	ITVAR(radartex, "textures/radar", 0);
	ITVAR(radarpingtex, "<anim:75>textures/radarping", 0);
	ITVAR(healthbartex, "<anim>textures/healthbar", 0);
	ITVAR(goalbartex, "<anim>textures/goalbar", 0);
	ITVAR(teambartex, "<anim>textures/teambar", 0);

	ITVAR(indicatortex, "<anim>textures/indicator", 3);
	ITVAR(pistolhudtex, "textures/pistolhud", 0);
	ITVAR(shotgunhudtex, "textures/shotgunhud", 0);
	ITVAR(chaingunhudtex, "textures/chaingunhud", 0);
	ITVAR(grenadeshudtex, "textures/grenadeshud", 0);
	ITVAR(flamerhudtex, "textures/flamerhud", 0);
	ITVAR(riflehudtex, "textures/riflehud", 0);
	ITVAR(pistolcliptex, "<anim>textures/pistolclip", 3);
	ITVAR(shotguncliptex, "<anim>textures/shotgunclip", 3);
	ITVAR(chainguncliptex, "<anim>textures/chaingunclip", 3);
	ITVAR(grenadescliptex, "<anim>textures/grenadesclip", 3);
	ITVAR(flamercliptex, "<anim>textures/flamerclip", 3);
	ITVAR(riflecliptex, "<anim>textures/rifleclip", 3);
	ITVAR(damagetex, "textures/damage", 0);
	ITVAR(snipetex, "textures/snipe", 0);

	gameclient()
		: ph(*this), pj(*this), ws(*this), sb(*this), et(*this), cc(*this), ai(*this), stf(*this), ctf(*this),
			nextmode(G_LOBBY), nextmuts(0), nextstyle(G_S_PVS), gamemode(G_LOBBY), mutators(0), gamestyle(G_S_PVS), intermission(false),
			maptime(0), minremain(0), swaymillis(0), swaydir(0, 0, 0),
			lasthit(0), lastcamera(0), lastzoom(0), lastmousetype(0),
			prevzoom(false), zooming(false),
			quakewobble(0), damageresidue(0),
			liquidchan(-1),
			player1(new gameent())
	{
        CCOMMAND(kill, "",  (gameclient *self), { self->suicide(self->player1, 0); });
		CCOMMAND(mode, "iii", (gameclient *self, int *val, int *mut, int *style), { self->setmode(*val, *mut, *style); });
		CCOMMAND(gamemode, "", (gameclient *self), intret(self->gamemode));
		CCOMMAND(mutators, "", (gameclient *self), intret(self->mutators));
		CCOMMAND(gamestyle, "", (gameclient *self), intret(self->gamestyle));
		CCOMMAND(zoom, "D", (gameclient *self, int *down), { self->dozoom(*down!=0); });
		s_strcpy(player1->name, "unnamed");
	}

	iclientcom *getcom() { return &cc; }
	icliententities *getents() { return &et; }

	char *gametitle() { return sv->gamename(gamemode, mutators, gamestyle); }
	char *gametext() { return getmapname(); }

	float radarrange()
	{
		float dist = float(radardist());
		if(editmode) dist = float(editradardist());
		return dist;
	}

	bool isthirdperson()
	{
		return (m_ssp(gamestyle) || thirdperson()) && !editmode && !cc.spectator;
	}

	int mousestyle()
	{
		if(editmode) return editmouse();
		if(cc.spectator) return specmouse();
		if(m_ssp(gamestyle)) return 2;
		if(inzoom()) return player1->gunselect == GUN_RIFLE ? snipemouse() : pronemouse();
		if(isthirdperson()) return thirdpersonmouse();
		return firstpersonmouse();
	}

	int deadzone()
	{
		if(editmode) return editdeadzone();
		if(cc.spectator) return specdeadzone();
		if(inzoom()) return player1->gunselect == GUN_RIFLE ? snipedeadzone() : pronedeadzone();
		if(isthirdperson()) return thirdpersondeadzone();
		return firstpersondeadzone();
	}

	int panspeed()
	{
		if(inzoom()) return player1->gunselect == GUN_RIFLE ? snipepanspeed() : pronepanspeed();
		if(editmode) return editpanspeed();
		if(cc.spectator) return specpanspeed();
		if(isthirdperson()) return thirdpersonpanspeed();
		return firstpersonpanspeed();
	}

	int fov()
	{
		if(editmode) return editfov();
		if(cc.spectator) return specfov();
		if(m_ssp(gamestyle)) return sidescrollfov();
		if(isthirdperson()) return thirdpersonfov();
		return firstpersonfov();
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
		if(m_pvs(gamestyle) && allowmove(player1)) return true;
		zoomset(false, 0);
		return false;
	}

	int zoomtime()
	{
		return player1->gunselect == GUN_RIFLE ? snipetime() : pronetime();
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
			switch(player1->gunselect == GUN_RIFLE ? snipetype() : pronetype())
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

	void addsway(gameent *d)
	{
		if(firstpersonsway())
		{
			if(d->physstate >= PHYS_SLOPE) swaymillis += curtime;

			float k = pow(0.7f, curtime/10.0f);
			swaydir.mul(k);
			vec vel(d->vel);
			vel.add(d->falling);
			swaydir.add(vec(vel).mul((1-k)/(15*max(vel.magnitude(), ph.maxspeed(d)))));
		}
		else swaydir = vec(0, 0, 0);
	}

	int respawnwait(gameent *d)
	{
		int wait = 0;
		if(m_stf(gamemode)) wait = stf.respawnwait(d);
		else if(m_ctf(gamemode)) wait = ctf.respawnwait(d);
		return wait;
	}

	void respawn(gameent *d)
	{
		if(d->state == CS_DEAD && !respawnwait(d))
			respawnself(d);
	}

    bool allowmove(physent *d)
    {
        if(d->type == ENT_PLAYER)
        {
        	if(d == player1 && g3d_active(true, true)) return false;
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
			cc.addmsg(SV_TRYSPAWN, "ri", d->clientnum);
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

	void setmode(int mode, int muts, int style)
	{
		nextmode = mode; nextmuts = muts; nextstyle = style;
		sv->modecheck(&nextmode, &nextmuts, &nextstyle);
	}

	void resetstates(int types)
	{
		if(types & ST_REQS) ws.requestswitch = ws.requestreload = 0;
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
			et.resetspawns();
			pj.reset();

			// reset perma-state
			dynent *d;
			loopi(numdynents()) if((d = iterdynents(i)) && d->type == ENT_PLAYER)
				((gameent *)d)->resetstate(lastmillis);
		}
	}

	void checkoften(gameent *d)
	{
		heightoffset(d, d == player1 || d->ai);
		loopi(GUN_MAX) if(d->gunstate[i] != GUNSTATE_IDLE)
		{
			if(d->state != CS_ALIVE || (d->gunstate[i] != GUNSTATE_POWER && lastmillis-d->gunlast[i] >= d->gunwait[i]))
				d->setgunstate(i, GUNSTATE_IDLE, 0, lastmillis);
		}
	}


	void otherplayers()
	{
		loopv(players) if(players[i])
		{
            gameent *d = players[i];
            const int lagtime = lastmillis-d->lastupdate;
            checkoften(d);
            if(d->ai || !lagtime || intermission) continue;
            else if(lagtime>1000 && d->state==CS_ALIVE)
			{
                d->state = CS_LAGGED;
				continue;
			}
			ph.smoothplayer(d, 1, false);
		}
	}

	void updateworld()		// main game update loop
	{
		if(!curtime) return;
        if(!maptime)
        {
        	maptime = lastmillis + curtime;
        	return;
        }

		gets2c();

		if(cc.ready())
		{
			ph.update();
			pj.update();
			et.update();
			ai.update();

			otherplayers();
			if(!allowmove(player1)) player1->stopmoving();
            checkoften(player1);

			#define adjustscaled(t,n,m) \
				if(n > 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/m)); if(n <= 0) n = (t)0; }

			adjustscaled(float, player1->roll, 100.f);
			adjustscaled(int, quakewobble, 150.f);
			adjustscaled(int, damageresidue, 200.f);

			if(player1->state == CS_DEAD)
			{
				if(lastmillis-player1->lastpain < 2000)
					ph.move(player1, 10, false);
			}
			else if(player1->state == CS_ALIVE)
			{
				ph.move(player1, 20, true);
				addsway(player1);
				et.checkitems(player1);
				ws.shoot(player1, worldpos);
				ws.reload(player1);
			}
			else ph.move(player1, 20, true);
		}

		if(player1->clientnum >= 0) c2sinfo(33);
	}

	void damaged(int gun, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir)
	{
		if(d->state != CS_ALIVE || intermission) return;

        d->lastregen = d->lastpain = lastmillis;
        d->health = health;

		if(actor->type == ENT_PLAYER) actor->totaldamage += damage;

		if(d == player1)
		{
			quakewobble += damage;
			damageresidue += damage;
			d->hitpush(damage, dir, actor, gun);
		}

		if(d->type == ENT_PLAYER)
		{
			vec p = headpos(d);
			p.z += 0.6f*(d->height + d->aboveeye) - d->height;
			particle_splash(3, min(damage/4, 10), 10000, p);
			if(d!=player1)
			{
				s_sprintfd(ds)("@%d", damage);
				particle_text(d->abovehead(), ds, 8);
			}
			playsound(S_PAIN1+rnd(5), 0, 255, d->o, d);
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
			playsound(S_DAMAGE1+snd, 0, 255, actor->o, actor);
			if(actor == player1) lasthit = lastmillis;
		}

		ai.damaged(d, actor, gun, flags, damage, health, millis, dir);
	}

	void killed(int gun, int flags, int damage, gameent *d, gameent *actor)
	{
		if(d->type != ENT_PLAYER) return;

		d->obliterated = (d == actor) || (flags & HIT_EXPLODE) || (flags & HIT_MELT) || (damage >= MAXHEALTH*2);
        d->lastregen = d->lastpain = lastmillis;
		d->state = CS_DEAD;
		d->deaths++;

		int anc = -1, dth = S_DIE1+rnd(2);
		if((flags & HIT_MELT) || (flags & HIT_BURN)) dth = S_FLBURN;
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
		else d->move = d->strafe = 0;

		s_strcpy(d->obit, "rests in pieces");
        if(d == actor)
        {
        	if(flags & HIT_MELT) s_strcpy(d->obit, "melted");
        	else if(flags & HIT_FALL) s_strcpy(d->obit, "thought they could fly");
        	else if(flags & HIT_EXPLODE) s_strcpy(d->obit, "decided to go out kamikaze style");
        	else if(flags & HIT_BURN) s_strcpy(d->obit, "was like a moth attracted to a flame");
        	else s_strcpy(d->obit, "suicided");
        }
		else
		{
			static const char *obitnames[3][GUN_MAX] = {
				{
					"ate a bullet from",
					"was filled with buckshot by",
					"was riddled with holes by",
					"was char-grilled by",
					"was pierced by",
					"was blown to pieces by",
				},
				{
					"received a bullet shaped brain implant from",
					"was given scrambled brains cooked up by",
					"was air conditioned courtesy of",
					"was char-grilled by",
					"was expertly sniped by",
					"was blown to pieces by",
				},
				{
					"exploded from a measily bullet shot by",
					"was turned into little chunks by",
					"was swiss-cheesed by",
					"was made the main course by order of chef",
					"had their head blown clean off by",
					"was obliterated by",
				}
			};

			int o = (flags & HIT_HEAD) ? 1 : (d->obliterated ? 2 : 0);
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
						particle_text(actor->abovehead(), ds, 8);
						break;
					}
					case 10:
					{
						s_strcat(d->obit, " who is slaughtering!");
						anc = S_V_SPREE2;
						s_sprintfd(ds)("@\fgSLAUGHTER");
						particle_text(actor->abovehead(), ds, 8);
						break;
					}
					case 25:
					{
						s_strcat(d->obit, " going on a massacre!");
						anc = S_V_SPREE3;
						s_sprintfd(ds)("@\fgMASSACRE");
						particle_text(actor->abovehead(), ds, 8);
						break;
					}
					case 50:
					{
						s_strcat(d->obit, " creating a bloodbath!");
						anc = S_V_SPREE4;
						s_sprintfd(ds)("@\fgBLOODBATH");
						particle_text(actor->abovehead(), ds, 8);
						break;
					}
					default:
					{
						if(flags & HIT_HEAD)
						{
							anc = S_V_HEADSHOT;
							s_sprintfd(ds)("@\fgHEADSHOT");
							particle_text(actor->abovehead(), ds, 8);
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
		if(dth >= 0) playsound(dth, 0, 255, d->o, d);
		s_sprintfd(a)("\fy%s %s", colorname(d), d->obit);
		et.announce(anc, a, af);
		s_sprintfd(da)("@%s", a);
		particle_text(d->abovehead(), da, 8);

		vec pos = headpos(d);
		int gdiv = d->obliterated ? 2 : 4, gibs = clamp((damage+gdiv)/gdiv, 1, 20);
		loopi(rnd(gibs)+1) pj.spawn(pos, d->vel, d, PRJ_GIBS);

		ai.killed(d, actor, gun, flags, damage);
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

	gameent *newclient(int cn)	// ensure valid entity
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

	gameent *getclient(int cn)	// ensure valid entity
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
		if(d->name[0]) conoutf("\fo* %s left the game", colorname(d));
		pj.remove(d);
        removetrackedparticles(d);
		removetrackedsounds(d);
		DELETEP(players[cn]);
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
    	ws.preload();
		pj.preload();
        et.preload();
		if(m_edit(gamemode) || m_stf(gamemode)) stf.preload();
        if(m_edit(gamemode) || m_ctf(gamemode)) ctf.preload();
    }

	void startmap(const char *name)	// called just after a map load
	{
		const char *title = getmaptitle();
		if(*title) console("%s", CON_CENTER|CON_NORMAL, title);
		intermission = false;
        player1->respawned = player1->suicided = maptime = 0;
        preload();
        et.mapstart();
		cc.mapstart();
        resetstates(ST_ALL);
	}

	void playsoundc(int n, gameent *d = NULL)
	{
		if(n < 0 || n >= S_MAX) return;
		gameent *c = d ? d : player1;
		if(c == player1 || c->ai) cc.addmsg(SV_SOUND, "i2", c->clientnum, n);
		playsound(n, 0, 255, c->o, c);
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

	char *colorname(gameent *d, char *name = NULL, const char *prefix = "", bool team = true, bool dupname = true)
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
				cc.addmsg(SV_SUICIDE, "ri2", d->clientnum, flags);
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
            case POINTER_RELATIVE: default: return relativecursortex(); break;
            case POINTER_GUI: return guicursortex(); break;
            case POINTER_EDIT: return editcursortex(); break;
            case POINTER_HAIR: return crosshairtex(); break;
            case POINTER_TEAM: return teamcrosshairtex(); break;
            case POINTER_HIT: return hitcrosshairtex(); break;
            case POINTER_SNIPE: return snipecrosshairtex(); break;
        }
        return NULL;
    }

	void drawpointer(int w, int h, int index, float x, float y, float r, float g, float b)
	{
		Texture *pointer = textureload(getpointer(index), 3, true);
		if(pointer)
		{
			float chsize = crosshairsize()*h*3.f, blend = crosshairblend();
			if(index == POINTER_GUI)
			{
				chsize = cursorsize()*h*3.f;
				blend = cursorblend();
			}
			else if(index == POINTER_SNIPE)
			{
				chsize = snipecrosshairsize()*h*3.f;
				if(inzoom() && player1->gunselect == GUN_RIFLE)
				{
					int frame = lastmillis-lastzoom;
					float amt = frame < zoomtime() ? clamp(float(frame)/float(zoomtime()), 0.f, 1.f) : 1.f;
					if(!zooming) amt = 1.f-amt;
					chsize *= amt;
				}
			}

			if(pointer->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else glBlendFunc(GL_ONE, GL_ONE);
			glColor4f(r, g, b, blend);

			float ox = x*w*3.f, oy = y*h*3.f, os = index != POINTER_GUI ? chsize/2.0f : 0,
				cx = ox-os, cy = oy-os;
			glBindTexture(GL_TEXTURE_2D, pointer->id);
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(cx, cy);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(cx + chsize, cy);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(cx + chsize, cy + chsize);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(cx, cy + chsize);
			glEnd();

			if(index > POINTER_GUI && m_pvs(gamestyle) && player1->state == CS_ALIVE && isgun(player1->gunselect) && player1->hasgun(player1->gunselect))
			{
				Texture *t;
				if(showindicator() && guntype[player1->gunselect].power && player1->gunstate[player1->gunselect] == GUNSTATE_POWER)
				{
					t = textureload(indicatortex(), 3);
					if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					else glBlendFunc(GL_ONE, GL_ONE);
					float nsize = chsize*(float(t->w)/float(pointer->w));
					cx = ox-nsize/2.0f;
					cy = oy-nsize/2.0f;
					glBindTexture(GL_TEXTURE_2D, t->retframe(lastmillis-player1->gunlast[player1->gunselect], guntype[player1->gunselect].power));
					glColor4f(1.f, 1.f, 0.f, indicatorblend());
					glBegin(GL_QUADS);
					glTexCoord2f(0.0f, 0.0f); glVertex2f(cx, cy);
					glTexCoord2f(1.0f, 0.0f); glVertex2f(cx + nsize, cy);
					glTexCoord2f(1.0f, 1.0f); glVertex2f(cx + nsize, cy + nsize);
					glTexCoord2f(0.0f, 1.0f); glVertex2f(cx, cy + nsize);
					glEnd();
				}

				if(crosshairclip())
				{
					const char *cliptexs[GUN_MAX] = {
						pistolcliptex(), shotguncliptex(), chainguncliptex(),
						flamercliptex(), riflecliptex(), grenadescliptex(),
					};
					t = textureload(cliptexs[player1->gunselect], 3);
					if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					else glBlendFunc(GL_ONE, GL_ONE);
					float nsize = chsize*(float(t->w)/float(pointer->w));
					cx = ox-nsize/2.0f;
					cy = oy-nsize/2.0f;
					glBindTexture(GL_TEXTURE_2D, t->retframe(player1->ammo[player1->gunselect], guntype[player1->gunselect].max));
					glColor4f(1.f, 1.f, 1.f, clipbarblend());
					glBegin(GL_QUADS);
					glTexCoord2f(0.0f, 0.0f); glVertex2f(cx, cy);
					glTexCoord2f(1.0f, 0.0f); glVertex2f(cx + nsize, cy);
					glTexCoord2f(1.0f, 1.0f); glVertex2f(cx + nsize, cy + nsize);
					glTexCoord2f(0.0f, 1.0f); glVertex2f(cx, cy + nsize);
					glEnd();
				}
			}
		}
	}

	void drawpointers(int w, int h)
	{
        int index = POINTER_NONE;

		if(g3d_active(true, false))
		{
			if(g3d_active()) index = POINTER_GUI;
			else return;
		}
        else if(hidehud || !showcrosshair() || player1->state == CS_DEAD) return;
        else if(editmode) index = POINTER_EDIT;
        else if(inzoom() && player1->gunselect == GUN_RIFLE) index = POINTER_SNIPE;
        else if(lastmillis-lasthit <= crosshairhitspeed()) index = POINTER_HIT;
        else if(m_team(gamemode, mutators))
        {
            vec pos = headpos(player1, 0.f);
            dynent *d = ws.intersectclosest(pos, worldpos, player1);
            if(d && d->type == ENT_PLAYER && ((gameent *)d)->team == player1->team)
				index = POINTER_TEAM;
			else index = POINTER_HAIR;
        }
        else index = POINTER_HAIR;

		float r = 1.f, g = 1.f, b = 1.f;
		if(index >= POINTER_HAIR)
		{
			if(!player1->canshoot(player1->gunselect, lastmillis)) { r *= 0.5f; g *= 0.5f; b *= 0.5f; }
			else if(r && g && b && !editmode)
			{
				if(player1->health<=25) { r = 1; g = b = 0; }
				else if(player1->health<=50) { r = 1; g = 0.5f; b = 0; }
			}
		}

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w*3, h*3, 0, -1, 1);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		float curx = index < POINTER_EDIT || mousestyle() == 2 ? cursorx : aimx,
			cury = index < POINTER_EDIT || mousestyle() == 2 ? cursory : aimy;

		drawpointer(w, h, index, curx, cury, r, g, b);

		if(index > POINTER_GUI && mousestyle() >= 1 && m_pvs(gamestyle))
		{
			curx = mousestyle() == 1 ? cursorx : 0.5f;
			cury = mousestyle() == 1 ? cursory : 0.5f;
			drawpointer(w, h, POINTER_RELATIVE, curx, cury, r, g, b);
		}
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}

	void drawtex(float x, float y, float w, float h)
	{
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x,	y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x+w, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x+w, y+h);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x,	y+h);
		glEnd();
	}
	void drawsized(float x, float y, float s) { drawtex(x, y, s, s); }

	void drawplayerblip(gameent *d, int x, int y, int s)
	{
		vec dir = headpos(d);
		dir.sub(camera1->o);
		float dist = dir.magnitude();
		if(dist < radarrange())
		{
			dir.rotate_around_z(-camera1->yaw*RAD);
			int colour = teamtype[d->team].colour;
			float cx = x + s*0.5f*(1.0f+dir.x/radarrange()),
				cy = y + s*0.5f*(1.0f+dir.y/radarrange()),
				cs = (d->crouching ? 0.005f : 0.025f)*s,
				r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f,
				fmag = clamp(d->vel.magnitude()/ph.maxspeed(d), 0.f, 1.f),
				fade = clamp(1.f-(dist/radarrange()), 0.f, 1.f)*blipblend()*fmag;
			if(lastmillis-d->lastspawn <= REGENWAIT)
				fade *= clamp(float(lastmillis-d->lastspawn)/float(REGENWAIT), 0.f, 1.f);
			settexture(bliptex(), 3);
			glColor4f(r, g, b, fade);
			drawsized(cx-cs*0.5f, cy-cs*0.5f, cs);
			int ty = int(cy+cs);
			if(radarnames())
				ty += draw_textx("%s", int(cx), ty, 255, 255, 255, int(fade*255.f), false, AL_CENTER, -1, -1, colorname(d, NULL, "", false));
			if(radarnames() == 2 && m_team(gamemode, mutators))
				ty += draw_textx("(\fs%s%s\fS)", int(cx), ty, 255, 255, 255, int(fade*255.f), false, AL_CENTER, -1, -1, teamtype[d->team].chat, teamtype[d->team].name);
		}
	}

	void drawcardinalblips(int x, int y, int s)
	{
		pushfont("radar");
		loopi(4)
		{
			const char *card = "";
			vec dir(camera1->o);
			switch(i)
			{
				case 0: dir.sub(vec(0, radarrange(), 0)); card = "N"; break;
				case 1: dir.add(vec(radarrange(), 0, 0)); card = "E"; break;
				case 2: dir.add(vec(0, radarrange(), 0)); card = "S"; break;
				case 3: dir.sub(vec(radarrange(), 0, 0)); card = "W"; break;
				default: break;
			}
			dir.sub(camera1->o);
			dir.rotate_around_z(-camera1->yaw*RAD);

			float cx = x + (s-FONTW)*0.5f*(1.0f+dir.x/radarrange()),
				cy = y + (s-FONTH)*0.5f*(1.0f+dir.y/radarrange());

			draw_textx("%s", int(cx), int(cy), 255, 255, 255, int(255*candinalblend()), true, AL_LEFT, -1, -1, card);
		}
		popfont();
	}

	void drawentblip(int x, int y, int s, int n, vec &o, int type, int attr1, int attr2, int attr3, int attr4, int attr5, bool spawned, int lastspawn)
	{
		if(type > NOTUSED && type < MAXENTTYPES && ((enttype[type].usetype == EU_ITEM && spawned) || editmode))
		{
			bool insel = editmode && et.ents.inrange(n) && (enthover == n || entgroup.find(n) >= 0);
			float inspawn = spawned && lastspawn && lastmillis-lastspawn <= 1000 ? float(lastmillis-lastspawn)/1000.f : 0.f;
			if(enttype[type].noisy && (!editmode || !editradarnoisy() || (editradarnoisy() < 2 && !insel)))
				return;
			vec dir(o);
			dir.sub(camera1->o);
			float dist = dir.magnitude();
			if(dist >= radarrange())
			{
				if(insel || inspawn) dir.mul(radarrange()/dist);
				else return;
			}
			dir.rotate_around_z(-camera1->yaw*RAD);
			float cx = x + s*0.5f*0.95f*(1.0f+dir.x/radarrange()), cy = y + s*0.5f*0.95f*(1.0f+dir.y/radarrange()),
				cs = (inspawn > 0.f ? (2.0f-inspawn)*0.025f : (insel ? 0.033f : 0.025f))*s,
					range = (inspawn > 0.f ? 2.f-inspawn : 1.f)-(insel ? 1.f : (dist/radarrange())),
						fade = clamp(range, 0.f, 1.f)*blipblend();
			settexture(bliptex(), 3);
			glColor4f(1.f, inspawn > 0.f ? inspawn : (insel ? 0.5f : 1.f), 0.f, fade);
			drawsized(cx-(cs*0.5f), cy-(cs*0.5f), cs);
		}
	}

	void drawentblips(int x, int y, int s)
	{
		loopv(et.ents)
		{
			gameentity &e = *(gameentity *)et.ents[i];
			drawentblip(x, y, s, i, e.o, e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, e.spawned, e.lastspawn);
		}

		loopv(pj.projs) if(pj.projs[i]->projtype == PRJ_ENT && pj.projs[i]->ready())
		{
			projent &proj = *pj.projs[i];
			if(et.ents.inrange(proj.id))
				drawentblip(x, y, s, -1, proj.o, proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4, proj.attr5, true, proj.spawntime);
		}
	}

	void drawtitlecard(int w, int h)
	{
		int ox = w*3, oy = h*3;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);
		pushfont("emphasis");

		int bs = int(oy*titlecardsize()), bp = int(oy*0.01f), bx = ox-bs-bp, by = bp,
			secs = maptime ? lastmillis-maptime : 0;
		float fade = hudblend, amt = 1.f;

		if(secs < titlecardtime())
		{
			amt = clamp(float(secs)/float(titlecardtime()), 0.f, 1.f);
			fade = clamp(amt*fade, 0.f, 1.f);
		}
		else if(secs < titlecardtime()+titlecardfade())
			fade = clamp(fade*(1.f-(float(secs-titlecardtime())/float(titlecardfade()))), 0.f, 1.f);

		const char *title = getmaptitle();
		if(!*title) title = getmapname();

		int rs = int(bs*amt), rx = bx+(bs-rs), ry = by;
		glColor4f(1.f, 1.f, 1.f, fade*0.9f);
		if(!rendericon(getmapname(), rx, ry, rs, rs))
			rendericon("textures/emblem", rx, ry, rs, rs);
		glColor4f(1.f, 1.f, 1.f, fade);
		rendericon(guioverlaytex, rx, ry, rs, rs);

		int tx = bx + bs, ty = by + bs + FONTH/2, ts = int(tx*(1.f-amt));
		ty += draw_textx("%s", tx-ts, ty, 255, 255, 255, int(255.f*fade), false, AL_RIGHT, -1, tx-FONTH, sv->gamename(gamemode, mutators, gamestyle));
		ty += draw_textx("%s", tx-ts, ty, 255, 255, 255, int(255.f*fade), false, AL_RIGHT, -1, tx-FONTH, title);
		popfont();
	}

	void drawgamehud(int w, int h)
	{
		Texture *t;
		int ox = w*3, oy = h*3;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);
		pushfont("hud");

		int secs = maptime ? lastmillis-maptime : 0;
		float fade = hudblend;

		if(secs < titlecardtime()+titlecardfade()+titlecardfade())
		{
			float amt = clamp(float(secs-titlecardtime()-titlecardfade())/float(titlecardfade()), 0.f, 1.f);
			fade = clamp(fade*amt, 0.f, 1.f);
		}

		if(player1->state == CS_ALIVE && inzoom() && player1->gunselect == GUN_RIFLE)
		{
			t = textureload(snipetex());
			int frame = lastmillis-lastzoom;
			float pc = frame < zoomtime() ? float(frame)/float(zoomtime()) : 1.f;
			if(!zooming) pc = 1.f-pc;

			glBindTexture(GL_TEXTURE_2D, t->id);
			glColor4f(1.f, 1.f, 1.f, pc);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(0, 0);
			glTexCoord2f(1, 0); glVertex2f(ox, 0);
			glTexCoord2f(1, 1); glVertex2f(ox, oy);
			glTexCoord2f(0, 1); glVertex2f(0, oy);
			glEnd();
		}

		if(showdamage() && ((player1->state == CS_ALIVE && damageresidue > 0) || player1->state == CS_DEAD))
		{
			t = textureload(damagetex());
			int dam = player1->state == CS_DEAD ? 100 : min(damageresidue, 100);
			float pc = float(dam)/100.f;

			glBindTexture(GL_TEXTURE_2D, t->id);
			glColor4f(1.f, 1.f, 1.f, pc);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(0, 0);
			glTexCoord2f(1, 0); glVertex2f(ox, 0);
			glTexCoord2f(1, 1); glVertex2f(ox, oy);
			glTexCoord2f(0, 1); glVertex2f(0, oy);
			glEnd();
		}

		int bs = int(oy*radarsize()), bo = int(bs/16.f), bp = int(oy*0.01f), bx = ox-bs-bp, by = bp,
			colour = teamtype[player1->team].colour,
				r = (colour>>16), g = ((colour>>8)&0xFF), b = (colour&0xFF);

		pushfont("radar");
		settexture(radartex());
		glColor4f(1.f, 1.f, 1.f, fade*radarblend());
		drawsized(float(bx), float(by), float(bs));

		drawentblips(bx+bo, by+bo, bs-(bo*2));

		if(m_stf(gamemode)) stf.drawblips(ox, oy, bx+bo, by+bo, bs-(bo*2));
		else if(m_ctf(gamemode)) ctf.drawblips(ox, oy, bx+bo, by+bo, bs-(bo*2));

		loopv(players)
			if(players[i] && players[i]->state == CS_ALIVE)
				drawplayerblip(players[i], bx+bo, by+bo, bs-(bo*2));

		settexture(radarpingtex());
		glColor4f(1.f, 1.f, 1.f, fade*radarblend());
		drawsized(float(bx), float(by), float(bs));

		settexture(goalbartex());
		glColor4f(1.f, 1.f, 1.f, fade*barblend());
		drawsized(float(bx), float(by), float(bs));

		settexture(teambartex());
		glColor4f(r/255.f, g/255.f, b/255.f, fade*barblend());
		drawsized(float(bx), float(by), float(bs));
		popfont();

		int tp = by + bs + FONTH/2;
		if(player1->state == CS_ALIVE)
		{
			t = textureload(healthbartex());
			float glow = 1.f, pulse = fade;

			if(lastmillis <= player1->lastregen+500)
			{
				float regen = (lastmillis-player1->lastregen)/500.f;
				pulse = clamp(pulse*regen, 0.3f, max(fade, 0.33f))*barblend();
				glow = clamp(glow*regen, 0.3f, 1.f);
			}

			glBindTexture(GL_TEXTURE_2D, t->retframe(player1->health, MAXHEALTH));
			glColor4f(glow, glow*0.3f, 0.f, pulse);
			drawsized(float(bx), float(by), float(bs));

			if(showhudammo())
			{
				int ta = int(oy*ammosize()), tb = ta*3, tv = bx + bs - tb,
					to = ta/16, tr = ta/2, tq = tr - FONTH/2;
				const char *cliptexs[GUN_MAX] = {
					pistolcliptex(), shotguncliptex(), chainguncliptex(),
					flamercliptex(), riflecliptex(), grenadescliptex(),
				}, *hudtexs[GUN_MAX] = {
					pistolhudtex(), shotgunhudtex(), chaingunhudtex(),
					flamerhudtex(), riflehudtex(), grenadeshudtex(),
				};
				loopi(GUN_MAX) if(player1->hasgun(i) && (i == player1->gunselect || showhudammo() > 1))
				{
					float blend = fade * (i == player1->gunselect ? ammoblend() : ammoblendinactive());
					settexture(hudtexs[i], 0);
					glColor4f(1.f, 1.f, 1.f, blend);
					drawtex(float(tv), float(tp), float(tb), float(ta));
					t = textureload(cliptexs[i], 3);
					glBindTexture(GL_TEXTURE_2D, t->retframe(player1->ammo[i], guntype[i].max));
					glColor4f(1.f, 1.f, 1.f, blend);
					drawtex(float(tv+to/2), float(tp+to/2), float(ta-to), float(ta-to));
					if(i == player1->gunselect) pushfont("emphasis");
					int ts = tv + tr, tt = tp + tq;
					draw_textx("%s%d", ts, tt, 255, 255, 255, int(255.f*blend), false, AL_CENTER, -1, -1, player1->canshoot(i, lastmillis) ? "\fw" : "\fr", player1->ammo[i]);
					if(i == player1->gunselect) popfont();
					tp += ta;
				}
				tp += FONTH/2;
			}
			if(showhudinfo())
			{
				vector<actitem> actitems;
				if(et.collateitems(player1, false, actitems))
				{
					bool found = false;
					while(!actitems.empty())
					{
						int closest = actitems.length()-1;
						loopv(actitems)
							if(actitems[i].score < actitems[closest].score)
								closest = i;

						actitem &t = actitems[closest];
						int ent = -1;
						switch(t.type)
						{
							case ITEM_ENT:
							{
								if(!et.ents.inrange(t.target)) break;
								ent = t.target;
								break;
							}
							case ITEM_PROJ:
							{
								if(!pj.projs.inrange(t.target)) break;
								projent &proj = *pj.projs[t.target];
								if(!et.ents.inrange(proj.id)) break;
								ent = proj.id;
								break;
							}
							default: break;
						}
						if(et.ents.inrange(ent))
						{
							const char *a = retbindaction("action", keym::ACTION_DEFAULT, 0);
							s_sprintfd(actkey)("%s", a && *a ? a : "ACTION");

							extentity &e = *et.ents[ent];
							if(enttype[e.type].usetype == EU_ITEM)
							{
								if(!found)
								{
									pushfont("emphasis");
									int drop = -1;
									if(e.type == WEAPON && guntype[player1->gunselect].rdelay > 0 &&
										player1->ammo[e.attr1] < 0 && guntype[e.attr1].rdelay > 0 &&
											player1->carry() >= MAXCARRY) drop = player1->drop(e.attr1);
									if(isgun(drop))
									{
										s_sprintfd(dropgun)("%s", et.entinfo(WEAPON, drop, player1->ammo[drop], 0, 0, 0, true));
										tp += draw_textx("Press [ \fs\fg%s\fS ] to swap", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey);
										tp += draw_textx("[ \fs%s\fS ] for [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1,
											dropgun, et.entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, true));
									}
									else
									{
										tp += draw_textx("Press [ \fs\fg%s\fS ] to pickup", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey);
										tp += draw_textx("[ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1,
											et.entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, true));
									}
									popfont();
									tp += FONTH/2;
									if(showhudinfo() < 2) break;
									else found = true;
								}
								else
									tp += draw_textx("[ \fs\fa%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, et.entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, true));
							}
							else if(e.type == TRIGGER && (e.spawned || !e.attr4) && e.attr3 == TA_ACT)
							{
								if(!found)
								{
									pushfont("emphasis");
									tp += draw_textx("Press [ \fs\fg%s\fS ] to interact", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey);
									tp += draw_textx("with [ \fs\fc%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, enttype[e.type].name);
									popfont();
									tp += FONTH/2;
									if(showhudinfo() < 2) break;
									else found = true;
								}
								else
									tp += draw_textx("[ \fs\fa%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, enttype[e.type].name);
							}
						}
						actitems.removeunordered(closest);
					}
					popfont();
				}
			}
		}
		else if(player1->state == CS_DEAD)
		{
			pushfont("emphasis");
			tp += draw_textx("Fragged!", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1);
			int wait = respawnwait(player1);
			if(wait)
				tp += draw_textx("Down for %.01fs", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, float(wait)/1000.f);
			else
			{
				const char *a = retbindaction("attack", keym::ACTION_DEFAULT, 0);
				s_sprintfd(actkey)("%s", a && *a ? a : "ACTION");
				tp += draw_textx("Press [ \fs\fg%s\fS ] to respawn", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey);
			}
			popfont();
		}
		else if(player1->state == CS_EDITING)
		{
			if(showhudentinfo()) loopi(clamp(entgroup.length()+1, 0, showhudents()))
			{
				int n = i ? entgroup[i-1] : enthover;
				if((!i || n != enthover) && et.ents.inrange(n))
				{
					gameentity &f = (gameentity &)*et.ents[n];
					if(showhudentinfo() > 2 || n == enthover) pushfont("emphasis");
					tp += draw_textx("entity %d, %s", bx+bs, tp,
						n == enthover ? 255 : 196, 196, 196, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1,
							n, et.findname(f.type));
					if(showhudentinfo() > 2 || n == enthover) popfont();
					if(showhudentinfo() > 1 || n == enthover)
					{
						tp += draw_textx("%s (%d %d %d %d %d)", bx+bs, tp,
							255, 196, 196, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1,
								et.entinfo(f.type, f.attr1, f.attr2, f.attr3, f.attr4, f.attr5, true),
									f.attr1, f.attr2, f.attr3, f.attr4, f.attr5);
					}
				}
			}
		}

		drawcardinalblips(bx+bo, by+bo, bs-(bo*2));
		popfont();
	}

	void drawhudelements(int w, int h)
	{
		int ox = w*3, oy = h*3, hoff = oy;
		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);

		renderconsole(ox, oy);

		static int laststats = 0, prevstats[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, curstats[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		if(lastmillis-laststats >= statrate())
		{
			memcpy(prevstats, curstats, sizeof(prevstats));
			laststats = lastmillis-(lastmillis%statrate());
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

		if(showstats())
		{
			hoff -= draw_textx("ond:%d va:%d gl:%d(%d) oq:%d lm:%d rp:%d pvs:%d", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6], lightmaps.length(), curstats[7], getnumviewcells());
			hoff -= draw_textx("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
		}

		if(showfps()) switch(showfps())
		{
			case 2:
				if(autoadjust) hoff -= draw_textx("fps:%d (%d/%d) +%d-%d [%d%%]", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], autoadjustfps, maxfps, curstats[9], curstats[10], curstats[11]);
				else hoff -= draw_textx("fps:%d (%d) +%d-%d", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], maxfps, curstats[9], curstats[10]);
				break;
			case 1:
				if(autoadjust) hoff -= draw_textx("fps:%d (%d/%d) [%d%%]", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], autoadjustfps, maxfps, curstats[11]);
				else hoff -= draw_textx("fps:%d (%d)", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], maxfps);
				break;
			default: break;
		}

		if(getcurcommand())
			hoff -= rendercommand(FONTH/4, hoff-FONTH, w*3-FONTH);

		if(cc.ready() && maptime)
		{
			if(editmode)
			{
				hoff -= draw_textx("sel:%d,%d,%d %d,%d,%d (%d,%d,%d,%d)", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1,
						sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z,
							sel.cx, sel.cxs, sel.cy, sel.cys);
				hoff -= draw_textx("corner:%d orient:%d grid:%d", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1,
								sel.corner, sel.orient, sel.grid);
				hoff -= draw_textx("cube:%s%d ents:%d", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1,
					selchildcount<0 ? "1/" : "", abs(selchildcount), entgroup.length());
			}

			render_texture_panel(w, h);
		}
	}

	void drawhud(int w, int h)
	{
		if(!hidehud)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			if(maptime && cc.ready())
			{
				if(lastmillis-maptime < titlecardtime()+titlecardfade())
					drawtitlecard(w, h);
				else drawgamehud(w, h);
			}
			drawhudelements(w, h);
			glDisable(GL_BLEND);
		}
		g3d_render();
		drawpointers(w, h); // gets done last!
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
				o = ws.gunorigin(p->owner->o, d, (gameent *)p->owner, p->owner != player1 || isthirdperson());
				break;
        	}
        	default: break;
        }
    }

	void newmap(int size)
	{
		cc.addmsg(SV_NEWMAP, "ri", size);
	}

	void g3d_gamemenus() { sb.show(); }

	void loadworld(gzFile &f, int maptype)
	{
	}

	void saveworld(gzFile &f)
	{
	}

	bool gethudcolour(vec &colour)
	{
		if(!maptime || lastmillis-maptime < titlecardtime())
		{
			float fade = maptime ? float(lastmillis-maptime)/float(titlecardtime()) : 0.f;
			colour = vec(fade, fade, fade);
			return true;
		}
		return false;
	}

	void heightoffset(physent *d, bool local)
	{
		if(local) d->o.z -= d->height;
		if(ph.iscrouching(d))
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
		if(local) d->o.z += d->height;
	}

	vec headpos(physent *d, float off = 0.f)
	{
		vec pos(d->o);
		pos.z -= off;
		return pos;
	}

	vec feetpos(physent *d, float off = 0.f)
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
				t = player1->gunselect == GUN_RIFLE ? snipetime() : pronetime(),
				f = player1->gunselect == GUN_RIFLE ? snipefov() : pronefov();
			float diff = float(fov()-f),
				amt = frame < t ? clamp(float(frame)/float(t), 0.f, 1.f) : 1.f;
			if(!zooming) amt = 1.f-amt;
			curfov -= amt*diff;
		}

        aspect = w/float(h);
        fovy = 2*atan2(tan(curfov/2*RAD), aspect)/RAD;
	}

	bool mousemove(int dx, int dy, int x, int y, int w, int h)
	{
		bool hit = g3d_active();

		#define mousesens(a,b,c) ((float(a)/float(b))*c)

		if(hit || mousestyle() >= 1)
		{
			if(absmouse()) // absolute positions, unaccelerated
			{
				cursorx = clamp(float(x)/float(w), 0.f, 1.f);
				cursory = clamp(float(y)/float(h), 0.f, 1.f);
				return false;
			}
			else
			{
				cursorx = clamp(cursorx+mousesens(dx, w, mousesensitivity()), 0.f, 1.f);
				cursory = clamp(cursory+mousesens(dy, h, mousesensitivity()*(!hit && invmouse() ? -1.f : 1.f)), 0.f, 1.f);
				return true;
			}
		}
		else
		{
			if(allowmove(player1))
			{
				float scale = inzoom() ?
						(player1->gunselect == GUN_RIFLE ? snipesensitivity() : pronesensitivity())
					: sensitivity();
				player1->yaw += mousesens(dx, w, yawsensitivity()*scale);
				player1->pitch -= mousesens(dy, h, pitchsensitivity()*scale*(!hit && invmouse() ? -1.f : 1.f));
				fixfullrange(player1->yaw, player1->pitch, player1->roll, false);
			}
			return true;
		}
		return false;
	}

	void project(int w, int h)
	{
		int style = g3d_active() ? -1 : mousestyle();
		if(style != lastmousetype)
		{
			resetcursor();
			lastmousetype = style;
		}
		if(!g3d_active())
		{
			int aim = m_pvs(gamestyle) ? (isthirdperson() ? thirdpersonaim() : firstpersonaim()) : 0;
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

		if(cc.ready() && maptime)
		{
			if(cc.spectator || editmode || m_pvs(gamestyle))
			{
				if(!lastcamera)
				{
					resetcursor();
					if(mousestyle() == 2)
					{
						camera1->yaw = player1->aimyaw = player1->yaw;
						camera1->pitch = player1->aimpitch = player1->pitch;
					}
				}

				vec pos = headpos(player1, 0.f);

				if(mousestyle() <= 1)
					findorientation(pos, player1->yaw, player1->pitch, worldpos);

				camera1->o = pos;

				if(isthirdperson())
				{
					float angle = thirdpersonangle() ? 0-thirdpersonangle() : player1->pitch;
					camera1->aimyaw = mousestyle() <= 1 ? player1->yaw : player1->aimyaw;
					camera1->aimpitch = mousestyle() <= 1 ? angle : player1->aimpitch;

					#define cameramove(d,s) \
						if(d) \
						{ \
							camera1->move = !s ? (d > 0 ? -1 : 1) : 0; \
							camera1->strafe = s ? (d > 0 ? -1 : 1) : 0; \
							loopi(10) if(!ph.moveplayer(camera1, 10, true, abs(d))) break; \
						}
					cameramove(thirdpersondist(), false);
					cameramove(thirdpersonshift(), true);
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
						if(!thirdpersonaim() && isthirdperson() && (!inzoom() || player1->gunselect != GUN_RIFLE))
						{ // if they don't like it they just suffer a bit of jerking
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
						findorientation(isthirdperson() && (!inzoom() || player1->gunselect != GUN_RIFLE) ? camera1->o : pos, yaw, pitch, worldpos);
						if(allowmove(player1))
						{
							if(isthirdperson() && (!inzoom() || player1->gunselect != GUN_RIFLE))
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

				if(quakewobble > 0)
					camera1->roll = float(rnd(21)-10)*(float(min(quakewobble, 100))/100.f);
				else camera1->roll = 0;

				if(!isthirdperson())
				{
					float yaw = camera1->aimyaw, pitch = camera1->aimpitch;
					if(mousestyle() == 2)
					{
						yaw = player1->yaw;
						pitch = player1->pitch;
					}
				}

				if(inzoom() && player1->gunselect == GUN_RIFLE)
				{
					float amt = lastmillis-lastzoom < zoomtime() ? clamp(float(lastmillis-lastzoom)/float(zoomtime()), 0.f, 1.f) : 1.f;
					if(!zooming) amt = 1.f-amt;
					vec off(vec(vec(pos).sub(camera1->o)).mul(amt));
					camera1->o.add(off);
				}

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

					if(lastcamera && mousestyle() >= 1 && !g3d_active())
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
			else if(m_ssp(gamestyle))
			{
				vec dir;
				vecfromyawpitch(player1->aimyaw, player1->aimpitch, 0, -1, dir);
				camera1->o = vec(player1->o).add(dir.mul(sspcameradist()));
				dir = vec(player1->o).sub(camera1->o).normalize();
				vectoyawpitch(dir, camera1->yaw, camera1->pitch);
				fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
				if(allowmove(player1))
				{
					dir = vec(cursorx*2.f-1.f, 0.f, (1.f-cursory)*2.f-1.f);
					player1->pitch = asin(dir.z/dir.magnitude())/RAD;
					player1->yaw = player1->aimyaw;
					if(dir.x < 0.f) player1->yaw -= 180.f;
					fixfullrange(player1->yaw, player1->pitch, player1->roll, false);
				}
				findorientation(player1->o, player1->yaw, player1->pitch, worldpos);
			}

			vecfromyawpitch(camera1->yaw, camera1->pitch, 1, 0, camdir);
			vecfromyawpitch(camera1->yaw, 0, 0, -1, camright);
			vecfromyawpitch(camera1->yaw, camera1->pitch+90, 1, 0, camup);

			ph.updatematerial(camera1, true, true);

			switch(camera1->inmaterial)
			{
				case MAT_WATER:
				{
					if(!issound(liquidchan))
						playsound(S_UNDERWATER, SND_LOOP|SND_NOATTEN|SND_NODELAY|SND_NOCULL, 255, camera1->o, camera1, &liquidchan);
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
		pj.adddynlights();
		et.adddynlights();
	}

	vector<gameent *> bestplayers;
    vector<int> bestteams;

	IVAR(animoverride, -1, 0, ANIM_MAX-1);
	IVAR(testanims, 0, 0, 1);

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
			if(firstpersonsway() != 0.f)
			{
				vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
				float swayspeed = min(4.f, sqrtf(d->vel.x*d->vel.x + d->vel.y*d->vel.y));
				dir.mul(swayspeed);
				float swayxy = sinf(swaymillis/115.0f)/float(firstpersonsway()),
					  swayz = cosf(swaymillis/115.0f)/float(firstpersonsway());
				swap(dir.x, dir.y);
				dir.x *= -swayxy;
				dir.y *= swayxy;
				dir.z = -fabs(swayspeed*swayz);
				dir.add(swaydir);
				o.add(dir);
			}
			if(firstpersondist() != 0.f)
			{
				vecfromyawpitch(yaw, pitch, 1, 0, dir);
				dir.mul(player1->radius*firstpersondist());
				o.add(dir);
			}
			if(firstpersonshift() != 0.f)
			{
				vecfromyawpitch(yaw, pitch, 0, -1, dir);
				dir.mul(player1->radius*firstpersonshift());
				o.add(dir);
			}
			if(firstpersonadjust() != 0.f)
			{
				vecfromyawpitch(yaw, pitch+90.f, 1, 0, dir);
				dir.mul(player1->height*firstpersonadjust());
				o.add(dir);
			}
		}

		int anim = animflags, basetime = lastaction;
		if(animoverride())
		{
			anim = (animoverride()<0 ? ANIM_ALL : animoverride())|ANIM_LOOP;
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
				case ANIM_IDLE: case ANIM_PISTOL: case ANIM_SHOTGUN: case ANIM_CHAINGUN:
				case ANIM_GRENADES: case ANIM_FLAMER: case ANIM_RIFLE:
				{
					anim >>= ANIM_SECONDARY;
					break;
				}
				default: break;
			}
		}

		if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX)) switch(anim&ANIM_INDEX)
		{
			case ANIM_IDLE: case ANIM_PISTOL: case ANIM_SHOTGUN: case ANIM_CHAINGUN:
			case ANIM_GRENADES: case ANIM_FLAMER: case ANIM_RIFLE:
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
		rendermodel(NULL, mdl, anim, o, !third && testanims() && d == player1 ? 0 : yaw+90, pitch, roll, flags, e, attachments, basetime, speed);
	}

	void renderplayer(gameent *d, bool third, bool trans, bool early = false)
	{
        modelattach a[4];
		int ai = 0, team = m_team(gamemode, mutators) ? d->team : TEAM_NEUTRAL,
			gun = d->gunselect, lastaction = lastmillis,
			animflags = ANIM_IDLE|ANIM_LOOP, animdelay = 0;
		bool secondary = false, showgun = isgun(gun);

		if(shownameinfo() && third && d != player1 && d->state != CS_SPECTATOR)
		{
			s_sprintfd(s)("@%s", colorname(d));
			part_text(d->abovehead(), s, 11, 1, 0xFFFFFF);
		}

		if(d->state == CS_DEAD)
		{
			if(d->obliterated) return; // not shown at all
			showgun = false;
			animflags = ANIM_DYING;
			lastaction = d->lastpain;
			int t = lastmillis-lastaction;
			if(t < 0 || t > 20000) return;
			if(t > 1000) { animflags = ANIM_DEAD|ANIM_LOOP; }
        }
		else if(d->state == CS_EDITING || d->state == CS_SPECTATOR)
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
			if(m_team(gamemode, mutators)) loopv(bestteams) { if(bestteams[i] == d->team) { animflags = ANIM_WIN|ANIM_LOOP; break; } }
			else if(bestplayers.find(d)>=0) animflags = ANIM_WIN|ANIM_LOOP;
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
				int gunstate = GUNSTATE_IDLE;
				if(lastmillis-d->gunlast[gun] <= d->gunwait[gun])
				{
					gunstate = d->gunstate[gun];
					lastaction = d->gunlast[gun];
					animdelay = d->gunwait[gun];
				}
				switch(gunstate)
				{
					case GUNSTATE_SWITCH:
					{
						if(lastmillis-d->gunlast[gun] <= d->gunwait[gun]/2)
						{
							if(d->hasgun(d->lastgun)) gun = d->lastgun;
							else showgun = false;
						}
						animflags = ANIM_SWITCH;
						break;
					}
					case GUNSTATE_POWER:
					{
						if(!guntype[gun].power) gunstate = GUNSTATE_SHOOT;
						animflags = (guntype[gun].anim+gunstate);
						break;
					}
					case GUNSTATE_SHOOT:
					{
						if(guntype[gun].power) showgun = false;
						animflags = (guntype[gun].anim+gunstate);
						break;
					}
					case GUNSTATE_RELOAD:
					{
						if(guntype[gun].power) showgun = false;
						animflags = (guntype[gun].anim+gunstate);
						break;
					}
					case GUNSTATE_IDLE:	default:
					{
						if(!d->hasgun(gun)) showgun = false;
						else animflags = (guntype[gun].anim+gunstate)|ANIM_LOOP;
						break;
					}
				}
			}
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
				loopv(ctf.flags) if(ctf.flags[i].owner == d && !ctf.flags[i].droptime)
				{
					a[ai].name = teamtype[ctf.flags[i].team].flag;
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
			if(d->state!=CS_SPECTATOR && d->state!=CS_SPAWNING && (d->state!=CS_DEAD || !d->obliterated))
				renderplayer(d, true, d->state == CS_LAGGED || (d->state == CS_ALIVE && lastmillis-d->lastspawn <= REGENWAIT));
        }

		et.render();
		pj.render();
		if(m_stf(gamemode)) stf.render();
        else if(m_ctf(gamemode)) ctf.render();
        ai.render();

		endmodelbatches();
	}

    void renderavatar(bool early)
    {
        if(inzoomswitch() && player1->gunselect == GUN_RIFLE) return;
        if(isthirdperson())
        {
            if(player1->state!=CS_SPECTATOR && (player1->state!=CS_DEAD || !player1->obliterated))
                renderplayer(player1, true, (player1->state == CS_ALIVE && lastmillis-player1->lastspawn <= REGENWAIT) || thirdpersontranslucent(), early);
        }
        else if(player1->state == CS_ALIVE)
        {
            renderplayer(player1, false, (lastmillis-player1->lastspawn <= REGENWAIT) || firstpersontranslucent(), early);
        }
    }
};
REGISTERGAME(GAMEID, new gameclient(), new gameserver());
#else
REGISTERGAME(GAMEID, NULL, new gameserver());
#endif
