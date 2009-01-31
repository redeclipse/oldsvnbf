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
	int quakewobble = 0, liquidchan = -1;

	gameent *player1 = new gameent();
	vector<gameent *> players;
	dynent fpsmodel;

	VARW(numplayers, 0, 4, MAXCLIENTS/2);
	VARW(numteamplayers, 0, 3, MAXCLIENTS/2);

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

	VARP(snipetype, 0, 0, 1);
	VARP(snipemouse, 0, 0, 2);
	VARP(snipedeadzone, 0, 25, 100);
	VARP(snipepanspeed, 1, 10, INT_MAX-1);
	VARP(snipecarbinefov, 45, 45, 150);
	VARP(sniperiflefov, 20, 20, 150);
	VARP(snipetime, 1, 300, 10000);

	VARP(pronetype, 0, 0, 1);
	VARP(pronemouse, 0, 0, 2);
	VARP(pronedeadzone, 0, 25, 100);
	VARP(pronepanspeed, 1, 10, INT_MAX-1);
	VARP(pronefov, 70, 70, 150);
	VARP(pronetime, 1, 150, 10000);

	VARP(shownamesabovehead, 0, 1, 2);
	VARP(showdamageabovehead, 0, 0, 1);
	VARP(showobituaries, 0, 1, 4); // 0 = off, 1 = only me, 2 = me & announcements, 3 = all but bots, 4 = all
	VARP(playdamagetones, 0, 2, 2);

	VARP(noblood, 0, 0, 1);

    VARP(ragdoll, 0, 1, 1);
    FVARP(ragdollpush, 0, 1.f, 10000);

	ICOMMAND(gamemode, "", (), intret(gamemode));
	ICOMMAND(mutators, "", (), intret(mutators));

	void start()
	{
		s_strcpy(player1->name, "unnamed");
	}

	char *gametitle() { return server::gamename(gamemode, mutators); }
	char *gametext() { return getmapname(); }

	bool isthirdperson()
	{
		if(!thirdperson) return false;
		if(player1->state == CS_EDITING) return false;
		if(player1->state == CS_SPECTATOR) return false;
		if(inzoom()) return false;
		return true;
	}

	int mousestyle()
	{
		if(player1->state == CS_EDITING) return editmouse;
		if(player1->state == CS_SPECTATOR) return specmouse;
		if(inzoom()) return weaptype[player1->weapselect].snipes ? snipemouse : pronemouse;
		if(isthirdperson()) return thirdpersonmouse;
		return firstpersonmouse;
	}

	int deadzone()
	{
		if(player1->state == CS_EDITING) return editdeadzone;
		if(player1->state == CS_SPECTATOR) return specdeadzone;
		if(inzoom()) return weaptype[player1->weapselect].snipes ? snipedeadzone : pronedeadzone;
		if(isthirdperson()) return thirdpersondeadzone;
		return firstpersondeadzone;
	}

	int panspeed()
	{
		if(inzoom()) return weaptype[player1->weapselect].snipes ? snipepanspeed : pronepanspeed;
		if(player1->state == CS_EDITING) return editpanspeed;
		if(player1->state == CS_SPECTATOR) return specpanspeed;
		if(isthirdperson()) return thirdpersonpanspeed;
		return firstpersonpanspeed;
	}

	int fov()
	{
		if(player1->state == CS_EDITING) return editfov;
		if(player1->state == CS_SPECTATOR) return specfov;
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
	int zoomtime() { return weaptype[player1->weapselect].snipes ? snipetime : pronetime; }

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
			switch(weaptype[player1->weapselect].snipes ? snipetype : pronetype)
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

	void announce(int idx, const char *msg, ...)
	{
		s_sprintfdlv(text, msg, msg);
		conoutf("%s", text);
		playsound(idx, camera1->o, camera1, SND_FORCED);
	}
	ICOMMAND(announce, "is", (int *idx, char *s), announce(*idx, "\fw%s", s));

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
			if(d->state == CS_DEAD || d->state == CS_WAITING) return false;
			else if(d->state == CS_ALIVE)
			{
				if(intermission) return false;
				if(m_paint(gamemode, mutators) && ((gameent *)d)->damageprotect(lastmillis, paintfreezetime*1000))
					return false;
			}
        }
        return true;
    }

	void respawn(gameent *d)
	{
		if(d->state == CS_DEAD)
		{
			int sdelay = m_spawndelay(gamemode, mutators), wait = d->respawnwait(lastmillis, sdelay);
			if(!wait || sdelay-wait > min(sdelay, spawndelaywait*1000))
			{
				client::addmsg(SV_TRYSPAWN, "ri", d->clientnum);
				d->respawned = d->lifesequence;
			}
		}
	}

	void spawneffect(const vec &o, int colour, int radius, float size, int num, int fade, float vel)
	{
		part_create(PART_ELECTRIC_SLENS, fade, o, colour, size);
		regularshape(PART_ELECTRIC, radius*2, colour, 21, num, fade, o, size, vel);
		adddynlight(o, radius*1.1f, vec(colour>>16, (colour>>8)&0xFF, colour&0xFF).mul(2.f/0xFF), fade, fade/3);
	}

	gameent *pointatplayer()
	{
		loopv(players)
		{
			gameent *o = players[i];
			if(!o) continue;
			vec pos = headpos(player1, 0.f);
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
			if(hud::sb.scoreson) hud::sb.showscores(false);
			cleargui();
			lasthit = 0;
		}
		if(types & ST_SPAWNS)
		{
			projs::reset();
			// reset perma-state
			gameent *d;
			loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d->type == ENT_PLAYER)
				d->resetstate(lastmillis, m_maxhealth(gamemode, mutators));
		}
	}

	void heightoffset(gameent *d, bool local)
	{
		d->o.z -= d->height;
		if(d->state == CS_ALIVE)
		{
			if(physics::iscrouching(d))
			{
				bool crouching = d->crouching;
				if(!crouching)
				{
					vec pos(d->o);
					d->o.z += PLAYERHEIGHT;
					if(!collide(d, vec(0, 0, 1), 0.f, false))
					{
						crouching = true;
						if(d->crouchtime >= 0) d->crouchtime = max(CROUCHTIME-(lastmillis-d->crouchtime), 0)-lastmillis;
					}
					else if(d->crouchtime < 0)
						d->crouchtime = lastmillis-max(CROUCHTIME-(lastmillis+d->crouchtime), 0);
					d->o = pos;
				}

				float crouchoff = 1.f-CROUCHHEIGHT;
				if(d->type == ENT_PLAYER)
				{
					int crouchtime = abs(d->crouchtime);
					float amt = lastmillis-crouchtime < CROUCHTIME ?
						clamp(float(lastmillis-crouchtime)/CROUCHTIME, 0.f, 1.f) : 1.f;
					if(!crouching) amt = 1.f-amt;
					crouchoff *= amt;
				}
				d->height = PLAYERHEIGHT-(PLAYERHEIGHT*crouchoff);
			}
			else d->height = PLAYERHEIGHT;
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

		loopi(WEAPON_MAX) if(d->weapstate[i] != WPSTATE_IDLE)
		{
			if(d->state != CS_ALIVE || (d->weapstate[i] != WPSTATE_POWER && lastmillis-d->weaplast[i] >= d->weapwait[i]))
				d->setweapstate(i, WPSTATE_IDLE, 0, lastmillis);
		}

		if(d->reqswitch > 0 && lastmillis-d->reqswitch > WEAPSWITCHDELAY*2) d->reqswitch = -1;
		if(d->reqreload > 0 && lastmillis-d->reqreload > weaptype[d->weapselect].rdelay*2) d->reqreload = -1;
		if(d->requse > 0 && lastmillis-d->requse > WEAPSWITCHDELAY*2) d->requse = -1;
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

	void damaged(int weap, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir)
	{
		if(d->state != CS_ALIVE || intermission) return;
		if(hithurts(flags))
		{
			d->dodamage(millis, health);
			if(actor->type == ENT_PLAYER) actor->totaldamage += damage;

			if(d == player1)
			{
				quakewobble += damage/2;
				hud::damageresidue += damage*2;
				hud::damagecompass(damage, actor->o, actor, weap);
			}

			if(d->type == ENT_PLAYER)
			{
				vec p = headpos(d);
				p.z += 0.6f*(d->height + d->aboveeye) - d->height;
				if(!kidmode && !noblood && weap != WEAPON_PAINT && !m_paint(gamemode, mutators))
					part_splash(PART_BLOOD, clamp(damage/2, 2, 10), 5000, p, 0x66FFFF, 2.f, int(d->radius));
				if(showdamageabovehead)
				{
					s_sprintfd(ds)("@%d", damage);
					part_text(vec(d->abovehead()).sub(vec(0, 0, 3)), ds, PART_TEXT_RISE, 3000, 0xFFFFFF, 3.f);
				}
				if(!issound(d->vschan))
					playsound(S_PAIN1+rnd(5), d->o, d, 0, -1, -1, -1, &d->vschan);
			}

			if(d != actor)
			{
				if(playdamagetones >= (d == player1 || actor == player1 ? 1 : 2))
				{
					int snd = 0;
					if(damage >= 200) snd = 7;
					else if(damage >= 150) snd = 6;
					else if(damage >= 100) snd = 5;
					else if(damage >= 75) snd = 4;
					else if(damage >= 50) snd = 3;
					else if(damage >= 25) snd = 2;
					else if(damage >= 10) snd = 1;
					if(!issound(actor->dschan))
						playsound(S_DAMAGE1+snd, actor->o, actor, 0, -1, -1, -1, &actor->dschan);
				}
				if(actor == player1) lasthit = lastmillis;
			}
			ai::damaged(d, actor, weap, flags, damage, health, millis, dir);
		}
		if(d == player1 || d->ai)
		{
			float force = (float(damage)/float(weaptype[weap].damage))*(100.f/d->weight)*weaptype[weap].hitpush;
			if(flags&HIT_WAVE || !hithurts(flags)) force *= wavepushscale;
			else if(d->health <= 0) force *= ragdollpush*deadpushscale;
			else force *= hitpushscale;
			d->vel.add(vec(dir).mul(force));
		}
	}

	void killed(int weap, int flags, int damage, gameent *d, gameent *actor)
	{
		if(d->type != ENT_PLAYER) return;

        d->lastregen = 0;
        d->lastpain = lastmillis;
		d->state = CS_DEAD;
		d->deaths++;

		int anc = -1, dth = -1;
		bool obliterated = false;
		if(weap == WEAPON_PAINT || m_paint(gamemode, mutators)) dth = S_SPLAT;
		else
		{
			obliterated = flags&HIT_EXPLODE || flags&HIT_MELT || damage > m_maxhealth(gamemode, mutators);
			if(flags&HIT_MELT || flags&HIT_BURN) dth = S_BURN;
			else if(obliterated) dth = S_SPLOSH;
			else dth = S_DIE1+rnd(2);
		}

		if(d == player1)
		{
			anc = S_V_FRAGGED;
			/*
			d->stopmoving();
			d->pitch = 0;
			d->roll = 0;
			*/
		}
		else
        {
        	// d->move = d->strafe = 0;
            d->resetinterp();
        }

		s_strcpy(d->obit, "rests in pieces");
        if(d == actor)
        {
        	if(flags&HIT_MELT) s_strcpy(d->obit, "melted");
			else if(flags&HIT_FALL) s_strcpy(d->obit, "thought they could fly");
			else if(flags&HIT_SPAWN) s_strcpy(d->obit, "tried to spawn inside solid matter");
			else if(flags&HIT_LOST) s_strcpy(d->obit, "got very, very lost");
        	else if(flags && isweap(weap))
        	{
				static const char *suicidenames[WEAPON_MAX] = {
					"found out what their plasma tasted like",
					"discovered buckshot bounces",
					"got caught up in their own crossfire",
					"barbequed themself for dinner",
					"pulled off a seemingly impossible stunt",
					"pulled off a seemingly impossible stunt",
					"decided to kick it, kamikaze style",
					"ate paint"
				};
        		s_strcpy(d->obit, suicidenames[weap]);
        	}
        	else if(m_paint(gamemode, mutators)) s_strcpy(d->obit, "was taken out of the game");
        	else if(flags&HIT_EXPLODE) s_strcpy(d->obit, "was obliterated");
        	else if(flags&HIT_BURN) s_strcpy(d->obit, "burnt up");
        	else s_strcpy(d->obit, "suicided");
        }
		else
		{
			static const char *obitnames[3][WEAPON_MAX] = {
				{
					"was plasmified by",
					"was filled with buckshot by",
					"was riddled with holes by",
					"was char-grilled by",
					"was pierced by",
					"was given laser burn by",
					"was blown to pieces by",
					"was tagged out by"
				},
				{
					"was plasmafied by",
					"was given scrambled brains cooked up by",
					"was air conditioned courtesy of",
					"was char-grilled by",
					"was given an extra orifice by",
					"was expertly sniped by",
					"was blown to pieces by",
					"was tagged out by"
				},
				{
					"was reduced to ooze by",
					"was turned into little chunks by",
					"was swiss-cheesed by",
					"was made the main course by order of chef",
					"was skewered by",
					"had their head laser-scalpeled off by",
					"was obliterated by",
					"was tagged out by"
				}
			};

			int o = obliterated ? 2 : (flags&HIT_HEAD && !weaptype[weap].explode ? 1 : 0);
			const char *oname = isweap(weap) ? obitnames[o][weap] : "was killed by";
			if(m_team(gamemode, mutators) && d->team == actor->team)
				s_sprintf(d->obit)("%s teammate %s", oname, colorname(actor));
			else
			{
				s_sprintf(d->obit)("%s %s", oname, colorname(actor));
				if(!kidmode) switch(actor->spree)
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
						s_strcat(d->obit, noblood || m_paint(gamemode, mutators) ? " creating a paintbath!" : " creating a bloodbath!");
						anc = S_V_SPREE4;
						s_sprintfd(ds)(m_paint(gamemode, mutators) ? "@\fgPAINTBATH" : "@\fgBLOODBATH");
						part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						break;
					}
					default:
					{
						if((flags&HIT_PROJ) && (flags&HIT_HEAD))
						{
							anc = S_V_HEADSHOT;
							s_sprintfd(ds)("@\fgHEADSHOT");
							part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						}
						else if(obliterated || lastmillis-d->lastspawn <= spawnprotecttime*1000+1000) anc = S_V_OWNED;
						break;
					}
				}
			}
		}
		if(dth >= 0)
		{
			if(issound(d->vschan)) removesound(d->vschan);
			playsound(dth, d->o, d, 0, -1, -1, -1, &d->vschan);
		}
		if(showobituaries)
		{
			bool isme = (d == player1 || actor == player1), show = false;
			switch(showobituaries)
			{
				case 1: if(isme) show = true; break;
				case 2: if(isme || anc >= 0) show = true; break;
				case 3: if(d->aitype != AI_NONE || anc >= 0) show = true; break;
				case 4: default: show = true; break;
			}
			if(show)
			{
				if(isme) announce(anc, "\fa%s %s", colorname(d), d->obit);
				else conoutf("\fa%s %s", colorname(d), d->obit);
			}
		}
		if(!kidmode && !noblood && !m_paint(gamemode, mutators))
		{
			vec pos = headpos(d);
			int gdiv = obliterated ? 2 : 4, gibs = clamp((damage+gdiv)/gdiv, 1, 20);
			loopi(rnd(gibs)+1)
				projs::create(pos, vec(pos).add(d->vel), true, d, PRJ_GIBS, rnd(2000)+1000, 0, rnd(100)+1, 50);
		}
		if(m_team(gamemode, mutators) && d->team == actor->team && d != actor && actor == player1)
			hud::teamkills.add(lastmillis);

		ai::killed(d, actor, weap, flags, damage);
	}

	void timeupdate(int timeremain)
	{
		minremain = timeremain;
		if(!timeremain)
		{
			if(!intermission)
			{
				player1->stopmoving(true);
				hud::sb.showscores(true, true);
				intermission = true;
			}
		}
		else if(timeremain > 0)
		{
			conoutf("\fctime remaining: %d %s", timeremain, timeremain==1 ? "minute" : "minutes");
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
		if(*title) conoutf("%s", title);
		intermission = false;
        player1->respawned = player1->suicided = maptime = 0;
        preload();
        entities::mapstart();
		client::mapstart();
        resetstates(ST_ALL);
        // prevent the player from being in the middle of nowhere if he doesn't get spawned
        entities::spawnplayer(player1, -1, true);
	}

	gameent *intersectclosest(vec &from, vec &to, gameent *at)
	{
		gameent *best = NULL, *o;
		float bestdist = 1e16f;
		loopi(numdynents()) if((o = (gameent *)iterdynents(i)))
		{
            if(!o || o==at || o->state!=CS_ALIVE || !physics::issolid(o)) continue;
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
		const char *chat = team && m_team(gamemode, mutators) ? teamtype[d->team].chat : teamtype[TEAM_NEUTRAL].chat;
		s_sprintf(cname)("%s\fs%s%s", *prefix ? prefix : "", chat, name);
		if(!name[0] || d->aitype != AI_NONE || (dupname && duplicatename(d, name)))
		{
			s_sprintfd(s)(" [\fs%s%d\fS]", d->aitype != AI_NONE ? "\fc" : "\fm", d->clientnum);
			s_strcat(cname, s);
		}
		s_strcat(cname, "\fS");
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
	ICOMMAND(kill, "",  (), { suicide(player1, 0); });

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

	void loadworld(gzFile &f, int maptype) {}
	void saveworld(gzFile &f) {}

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
			int frame = lastmillis-lastzoom, f = pronefov,
				t = weaptype[player1->weapselect].snipes ? snipetime : pronetime;
			if(weaptype[player1->weapselect].snipes) switch(player1->weapselect)
			{
				case WEAPON_CARBINE: f = snipecarbinefov; break;
				case WEAPON_RIFLE: f = sniperiflefov; break;
				default: break;
			}
			float diff = float(fov()-f),
				amt = frame < t ? clamp(float(frame)/float(t), 0.f, 1.f) : 1.f;
			if(!zooming) amt = 1.f-amt;
			curfov -= amt*diff;
		}

        aspect = w/float(h);
        fovy = 2*atan2(tan(curfov/2*RAD), aspect)/RAD;
		hud::hudwidth = int(hud::hudsize*aspect);
	}

	bool mousemove(int dx, int dy, int x, int y, int w, int h)
	{
		bool hascursor = UI::hascursor(true);
		#define mousesens(a,b,c) ((float(a)/float(b))*c)
		if(hascursor || (mousestyle() >= 1 && player1->state != CS_WAITING))
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
			if(player1->state == CS_WAITING)
			{
				camera1->yaw += mousesens(dx, w, yawsensitivity*sensitivity);
				camera1->pitch -= mousesens(dy, h, pitchsensitivity*sensitivity*(!hascursor && invmouse ? -1.f : 1.f));
				fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
			}
			else if(allowmove(player1))
			{
				float scale = inzoom() ?
						(weaptype[player1->weapselect].snipes? snipesensitivity : pronesensitivity)
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

	#define scalecameraangle \
		{ \
			if(camera1->yaw < yaw-180.0f) camera1->yaw += 360.0f; \
			if(camera1->yaw > yaw+180.0f) camera1->yaw -= 360.0f; \
			float amt = float(curtime)/1000.f, offyaw = fabs(yaw-camera1->yaw)*amt, offpitch = fabs(pitch-camera1->pitch)*amt*0.25f; \
			if(yaw > camera1->yaw) { \
				camera1->yaw += offyaw; \
				if(yaw < camera1->yaw) camera1->yaw = yaw; \
			} \
			else if(yaw < camera1->yaw) \
			{ \
				camera1->yaw -= offyaw; \
				if(yaw > camera1->yaw) camera1->yaw = yaw; \
			} \
			if(pitch > camera1->pitch) \
			{ \
				camera1->pitch += offpitch; \
				if(pitch < camera1->pitch) camera1->pitch = pitch; \
			} \
			else if(pitch < camera1->pitch) \
			{ \
				camera1->pitch -= offpitch; \
				if(pitch > camera1->pitch) camera1->pitch = pitch; \
			} \
			yaw = camera1->aimyaw = camera1->yaw; \
			pitch = camera1->aimpitch = camera1->pitch; \
		}

	void checkcamera()
	{
		camera1 = &camera;
		if(camera1->type != ENT_CAMERA)
		{
			camera1->reset();
			camera1->type = ENT_CAMERA;
			camera1->state = CS_ALIVE;
			camera1->height = camera1->radius = camera1->xradius = camera1->yradius = 1;
		}
		if(player1->state != CS_WAITING)
		{
			camera1->vel = vec(0, 0, 0);
			camera1->move = camera1->strafe = 0;
		}
	}

	void cameraplayer()
	{
		if(player1->state != CS_WAITING && player1->state != CS_DEAD && !tvmode())
		{
			if(mousestyle() == 2) vectoyawpitch(cursordir, player1->yaw, player1->pitch);
			if(isthirdperson())
			{
				vec dir(worldpos);
				dir.sub(camera1->o);
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

	void cameratv()
	{
		if(cameras.empty()) loopk(2)
		{
			physent d = *player1;
			d.radius = d.height = 2.f;
			loopv(entities::ents) if((k && !enttype[entities::ents[i]->type].noisy) || entities::ents[i]->type == CAMERA)
			{
				gameentity &e = *(gameentity *)entities::ents[i];
				vec pos(e.o);
				if(e.type == MAPMODEL)
				{
					mapmodelinfo &mmi = getmminfo(e.attr[0]);
					vec center, radius;
					mmi.m->collisionbox(0, center, radius);
					if(!mmi.m->ellipsecollide) rotatebb(center, radius, int(e.attr[1]));
					pos.z += ((center.z-radius.z)+radius.z*2*mmi.m->height)*2.f;
				}
				if(enttype[e.type].radius) pos.z += enttype[e.type].radius;
				d.o = pos;
				if(physics::entinmap(&d, false))
				{
					camstate &c = cameras.add();
					c.pos = pos;
					c.ent = i;
					if(!k)
					{
						c.idx = e.attr[0];
						if(e.attr[1]) c.mindist = e.attr[1];
						if(e.attr[2]) c.maxdist = e.attr[2];
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
					loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && (d->state == CS_ALIVE || d->state == CS_DEAD || d->state == CS_WAITING))
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
			float yaw = camera1->yaw, pitch = camera1->pitch;
			vectoyawpitch(dir, yaw, pitch);
			if(cam == oldcam) { scalecameraangle; }
			else
			{
				camera1->yaw = camera1->aimyaw = yaw;
				camera1->pitch = camera1->aimpitch = pitch;
			}
			player1->yaw = player1->aimyaw = camera1->yaw;
			player1->pitch = player1->aimpitch = camera1->pitch;
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
			if(allowmove(player1)) cameraplayer();
			else player1->stopmoving(player1->state != CS_WAITING);

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
			adjustscaled(int, hud::damageresidue);

			if(player1->state == CS_DEAD || player1->state == CS_WAITING)
			{
                if(player1->ragdoll) moveragdoll(player1, true);
				else if(lastmillis-player1->lastpain < 2000)
					physics::move(player1, 10, false);
			}
			else
            {
                if(player1->ragdoll) cleanragdoll(player1);
                if(player1->state == CS_ALIVE)
			    {
				    physics::move(player1, 10, true);
				    addsway(player1);
				    entities::checkitems(player1);
				    weapons::reload(player1);
			    }
			    else if(!tvmode()) physics::move(player1, 10, true);
            }
			checkcamera();
			if(player1->state == CS_DEAD)
			{
				vec dir = vec(ragdollcenter(player1)).sub(camera1->o).normalize();
				float yaw = camera1->yaw, pitch = camera1->pitch;
				vectoyawpitch(dir, yaw, pitch);
				scalecameraangle;
			}
			else if(player1->state == CS_WAITING)
			{
				camera1->move = player1->move;
				camera1->strafe = player1->strafe;
				physics::move(camera1, 10, true);
			}
			else if(tvmode()) cameratv();
            if(hud::sb.canshowscores()) hud::sb.showscores(true);
		}

		if(player1->clientnum >= 0) c2sinfo(40);
	}

	void recomputecamera(int w, int h)
	{
		fixview(w, h);
		checkcamera();
		if(connected() && maptime)
		{
			if(!lastcamera)
			{
				resetcursor();
				cameras.setsize(0);
				lastspec = 0;
				if(mousestyle() == 2 && player1->state != CS_WAITING)
				{
					camera1->yaw = player1->aimyaw = player1->yaw;
					camera1->pitch = player1->aimpitch = player1->pitch;
				}
			}

			if(player1->state == CS_DEAD || player1->state == CS_WAITING || tvmode())
			{
				camera1->aimyaw = camera1->yaw;
				camera1->aimpitch = camera1->pitch;
			}
			else
			{
				camera1->o = headpos(player1, 0.f);
				if(mousestyle() <= 1)
					findorientation(camera1->o, player1->yaw, player1->pitch, worldpos);
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
						camera1->yaw = player1->yaw;
						camera1->pitch = player1->pitch;
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
						break;
					}
				}
				fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
				fixrange(camera1->aimyaw, camera1->aimpitch);
			}

			if(quakewobble > 0)
				camera1->roll = float(rnd(15)-7)*(float(min(quakewobble, 100))/100.f);
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

	void renderclient(gameent *d, bool third, bool trans, int team, modelattach *attachments, bool secondary, int animflags, int animdelay, int lastaction, bool early)
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

		int anim = animflags, basetime = lastaction, basetime2 = 0;
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
					anim |= (((allowmove(d) && (d->move || d->strafe)) || d->vel.z+d->falling.z>0 ? int(ANIM_SWIM) : int(ANIM_SINK))|ANIM_LOOP)<<ANIM_SECONDARY;
				else if(d->timeinair && d->lastimpulse && lastmillis-d->lastimpulse <= 1000) { anim |= ANIM_IMPULSE<<ANIM_SECONDARY; basetime2 = d->lastimpulse; }
				else if(d->timeinair && d->jumptime && lastmillis-d->jumptime <= 1000) { anim |= ANIM_JUMP<<ANIM_SECONDARY; basetime2 = d->jumptime; }
				else if(d->timeinair > 1000) anim |= (ANIM_JUMP|ANIM_END)<<ANIM_SECONDARY;
				else if(d->crouching || d->crouchtime<0)
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
                    anim = (anim>>ANIM_SECONDARY) | ((anim&((1<<ANIM_SECONDARY)-1))<<ANIM_SECONDARY);
                    swap(basetime, basetime2);
					break;
				}
				default: break;
			}
		}

        if(third && testanims && d == player1) yaw = 0;
        else yaw += 90;
        if(anim == ANIM_DYING) pitch *= max(1.0f - (lastmillis-basetime)/1000.0f, 0.0f);

        if(d->ragdoll && (!ragdoll || anim!=ANIM_DYING)) cleanragdoll(d);

		if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX)) anim |= (ANIM_IDLE|ANIM_LOOP)<<ANIM_SECONDARY;

		int flags = MDL_LIGHT;
		if(d != player1 && !(anim&ANIM_RAGDOLL)) flags |= MDL_CULL_VFC | MDL_CULL_OCCLUDED | MDL_CULL_QUERY;
        if(d->type == ENT_PLAYER) flags |= MDL_FULLBRIGHT;
		else flags |= MDL_CULL_DIST;
        if(early) flags |= MDL_NORENDER;
		else if(trans) flags |= MDL_TRANSLUCENT;
		else if(third && (anim&ANIM_INDEX)!=ANIM_DEAD) flags |= MDL_DYNSHADOW;
		dynent *e = third ? (dynent *)d : (dynent *)&fpsmodel;
		rendermodel(NULL, mdl, anim, o, yaw, pitch, roll, flags, e, attachments, basetime, basetime2);
	}

	void renderplayer(gameent *d, bool third, bool trans, bool early = false)
	{
        modelattach a[4];
		int ai = 0, team = m_team(gamemode, mutators) ? d->team : TEAM_NEUTRAL,
			weap = d->weapselect, lastaction = 0,
			animflags = ANIM_IDLE|ANIM_LOOP, animdelay = 0;
		bool secondary = false, showweap = isweap(weap);

		if(d->state == CS_SPECTATOR) return;
		else if(d->state == CS_DEAD || d->state == CS_WAITING)
		{
			showweap = false;
			animflags = ANIM_DYING;
			lastaction = d->lastpain;
            if(ragdoll)
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
		else if(d->state == CS_LAGGED) animflags = ANIM_LAG|ANIM_LOOP;
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
		else if(third && d->lasttaunt && lastmillis-d->lasttaunt <= 1000)
		{
			lastaction = d->lasttaunt;
			animflags = ANIM_TAUNT;
			animdelay = 1000;
		}
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
				int weapstate = WPSTATE_IDLE;
				if(lastmillis-d->weaplast[weap] <= d->weapwait[weap])
				{
					weapstate = d->weapstate[weap];
					lastaction = d->weaplast[weap];
					animdelay = d->weapwait[weap];
				}
				switch(weapstate)
				{
					case WPSTATE_SWITCH:
					case WPSTATE_PICKUP:
					{
						if(lastmillis-d->weaplast[weap] <= d->weapwait[weap]/2)
						{
							if(d->hasweap(d->lastweap, m_spawnweapon(gamemode, mutators))) weap = d->lastweap;
							else showweap = false;
						}
						animflags = ANIM_SWITCH;
						break;
					}
					case WPSTATE_POWER:
					{
						if(!weaptype[weap].power) weapstate = WPSTATE_SHOOT;
						animflags = (weaptype[weap].anim+weapstate);
						break;
					}
					case WPSTATE_SHOOT:
					{
						if(weaptype[weap].power) showweap = false;
						animflags = (weaptype[weap].anim+weapstate);
						break;
					}
					case WPSTATE_RELOAD:
					{
						if(weaptype[weap].power) showweap = false;
						animflags = (weaptype[weap].anim+weapstate);
						break;
					}
					case WPSTATE_IDLE:	default:
					{
						if(!d->hasweap(weap, m_spawnweapon(gamemode, mutators))) showweap = false;
						else animflags = (weaptype[weap].anim+weapstate)|ANIM_LOOP;
						break;
					}
				}
			}
		}

		if(shownamesabovehead && third && d != player1) part_text(d->abovehead(), colorname(d, NULL, "@"));
		if(showweap)
		{ // we could probably animate the vwep too now..
			a[ai].name = weaptype[weap].vwep;
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

        renderclient(d, third, trans, team, a[0].tag ? a : NULL, secondary, animflags, animdelay, lastaction, early);
	}

	bool showtranslucent(gameent *d, bool third = true)
	{
		if(!physics::issolid(d)) return true;
		if(d == player1 && (third ? thirdpersontranslucent : firstpersontranslucent)) return true;
		return false;
	}

	void render()
	{
		if(intermission)
		{
			if(m_team(gamemode, mutators)) { bestteams.setsize(0); hud::sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); hud::sb.bestplayers(bestplayers); }
		}

		startmodelbatches();

		gameent *d;
        loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d != player1)
        {
			if(d->state!=CS_SPECTATOR && d->state!=CS_SPAWNING)
				renderplayer(d, true, showtranslucent(d, true));
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
        if((isthirdperson() || !rendernormally || player1->state == CS_DEAD || player1->state == CS_WAITING) && player1->state != CS_SPECTATOR)
                renderplayer(player1, true, showtranslucent(player1, true), early);
        else if(player1->state == CS_ALIVE)
            renderplayer(player1, false, showtranslucent(player1, false), early);
    }

	bool clientoption(char *arg) { return false; }
}
#undef GAMEWORLD
