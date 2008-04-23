struct GAMECLIENT : igameclient
{
	#include "physics.h"
	#include "projs.h"
	#include "weapon.h"
	#include "scoreboard.h"
	#include "fpsrender.h"
	#include "entities.h"
	#include "client.h"
	#include "bot.h"
	#include "capture.h"
    #include "ctf.h"

	int nextmode, nextmuts, gamemode, mutators;
	bool intermission;
	int maptime, minremain;
	int respawnent;
	int swaymillis;
	vec swaydir;
    dynent guninterp;
    int lasthit;

	string cptext;
	int cameratype, cameranum, cameracycled, camerawobble, damageresidue, myrankv, myranks;
    float crouching;

	struct sline { string s; };
	struct teamscore
	{
		const char *team;
		int score;
		teamscore() {}
		teamscore(const char *s, int n) : team(s), score(n) {}
	};

	vector<fpsent *> shplayers;
	vector<teamscore> teamscores;

	fpsent *player1;				// our client
	vector<fpsent *> players;		// other clients
	fpsent lastplayerstate;

	IVAR(cameracycle, 0, 0, 600);// cycle camera every N secs

	IVARP(invmouse, 0, 0, 1);
	IVARP(thirdperson, 0, 0, 1);
	IVARP(thirdpersondist, 2, 16, 128);
	IVARP(thirdpersonheight, 2, 12, 128);

	IVARP(yawsensitivity, 0, 10, 1000);
	IVARP(pitchsensitivity, 0, 7, 1000);
	IVARP(rollsensitivity, 0, 3, 1000);
	IVARP(sensitivityscale, 1, 1, 100);

	IVARP(autoreload, 0, 1, 1);// auto reload when empty

	IVARP(crosshair, 0, 1, 1);// show the crosshair
	IVARP(teamcrosshair, 0, 1, 1);
	IVARP(hitcrosshair, 0, 425, 1000);

	IVARP(rankhud, 0, 0, 1);// show ranks on the hud
	IVARP(ranktime, 0, 15000, 600000);// display unchanged rank no earlier than every N ms

    GAMECLIENT()
		: ph(*this), pj(*this), ws(*this), sb(*this), fr(*this), et(*this), cc(*this), bot(*this), cpc(*this), ctf(*this),
			nextmode(sv->defaultmode()), nextmuts(0), gamemode(sv->defaultmode()), mutators(0), intermission(false),
			maptime(0), minremain(0), respawnent(-1),
			swaymillis(0), swaydir(0, 0, 0),
			cameratype(CAMERA_NONE), cameranum(0), cameracycled(0), myrankv(0), myranks(0), crouching(0),
			player1(spawnstate(new fpsent()))
	{
		CCOMMAND(centerrank, "", (GAMECLIENT *self), self->setcrank());
		CCOMMAND(cameradir, "ii", (GAMECLIENT *self, int *a, int *b), self->cameradir(*a, *b!=0));
		CCOMMAND(gotocamera, "i", (GAMECLIENT *self, int *a), self->setcamera(*a));
        CCOMMAND(kill, "",  (GAMECLIENT *self), { self->suicide(self->player1); });
		CCOMMAND(mode, "ii", (GAMECLIENT *self, int *val, int *mut), { self->setmode(*val, *mut); });
		CCOMMAND(gamemode, "", (GAMECLIENT *self), intret(self->gamemode));
		CCOMMAND(mutators, "", (GAMECLIENT *self), intret(self->mutators));

		CCOMMAND(sensitivity, "s", (GAMECLIENT *self, char *s), {
				if (*s)
				{
					int x = atoi(s);
					if (x)
					{
						setvar("yawsensitivity", x);
						setvar("pitchsensitivity", x/2);
						setvar("rollsensitivity", x/3);
					}
				}
				intret(self->yawsensitivity());
			});
	}

	iclientcom *getcom() { return &cc; }
	icliententities *getents() { return &et; }

	fpsent *spawnstate(fpsent *d)			  // reset player state not persistent accross spawns
	{
		d->respawn();
		playsound(S_RESPAWN, &d->o, 255, 0, true);
		d->spawnstate(gamemode, mutators);
		return d;
	}

	void spawnplayer(fpsent *d)	// place at random spawn. also used by monsters!
	{
		et.findplayerspawn(d, m_capture(gamemode) ? cpc.pickspawn(d->team) : (respawnent>=0 ? respawnent : -1), m_ctf(gamemode) ? ctf.teamflag(player1->team, m_ttwo(gamemode, mutators))+1 : -1);
		spawnstate(d);
		d->state = cc.spectator ? CS_SPECTATOR : (d==player1 && editmode ? CS_EDITING : CS_ALIVE);
	}

	int respawnwait(fpsent *d)
	{
		int wait = 0;
		if (m_capture(gamemode)) wait = cpc.respawnwait(d);
		else if (m_ctf(gamemode)) wait = ctf.respawnwait(d);
		return wait;
	}

	void respawn(fpsent *d)
	{
		if(d->state == CS_DEAD)
		{
			int wait = respawnwait(d);

			if (wait)
			{
				if(d==player1) console("\f2you must wait %d second%s before respawn!", CON_NORMAL|CON_CENTER, wait, wait!=1 ? "s" : "");
				return;
			}

			respawnself(d);
		}
	}

	bool canjump()
	{
		return player1->state != CS_DEAD && !intermission;
	}

    bool allowmove(physent *d)
    {
        if (d->type != ENT_PLAYER) return true;
        fpsent *e = (fpsent *)d;
        return !intermission && lastmillis-e->lasttaunt >= 1000;
    }

	void respawnself(fpsent *d)
	{
        if (d==player1) crouching = 0;

		d->stopmoving();

        if(m_mp(gamemode))
        {
            if (d->respawned != d->lifesequence)
            {
                cc.addmsg(SV_TRYSPAWN, "ri", d->clientnum);
                d->respawned = d->lifesequence;
            }
        }
        else
        {
            spawnplayer(d);
            if (d==player1)
            {
            	sb.showscores(false);
				lasthit = 0;
            }
        }
	}

	bool doautoreload()
	{
		return autoreload() && player1->gunselect >= 0 &&
			!player1->ammo[player1->gunselect] && guntype[player1->gunselect].rdelay > 0;
	}

	fpsent *pointatplayer()
	{
		loopv(players)
		{
			fpsent *o = players[i];
			if(!o) continue;
			if(intersect(o, player1->o, worldpos)) return o;
		}
		return NULL;
	}

	void setmode(int mode, int muts)
	{
		if (!m_game(mode)) { conoutf("mode %d is not a valid gameplay type", mode); return; }
		if (m_sp(mode)) { conoutf("playing %s is not yet supported", gametype[mode]); return; }
		if (multiplayer(false) && !m_mp(mode)) { conoutf("playing %s is not supported in multiplayer", gametype[mode]); return; }
		nextmode = mode;
		nextmuts = muts;
	}

    void render() { fr.render(); }

	void resetgamestate()
	{
		pj.reset();
	}

	void updateworld()		// main game update loop
	{
        if (!maptime) { maptime = lastmillis + curtime; return; }
		if (!curtime) return;

		ph.update();
		pj.update();
		et.update();
		bot.update();
		gets2c();

		if (cc.ready())
		{
			if (!allowmove(player1) || saycommandon) player1->stopmoving();

			#define adjust(t,n,m) \
				if (n != 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/float(m))); }

			adjust(int, camerawobble, 100);
			adjust(int, damageresidue, 200);
			if (!player1->leaning) adjust(float, player1->roll, 50);

			if (player1->state == CS_DEAD)
			{
				if (lastmillis-player1->lastpain < 2000)
				{
					player1->stopmoving();
					ph.move(player1, 10, false);
				}
			}
			else
			{
				if (player1->timeinair)
				{
					if (player1->jumping && lastmillis-player1->lastimpulse > ph.gravityforce(player1)*100)
					{
						vec dir;
						vecfromyawpitch(player1->yaw, player1->move || player1->strafe ? player1->pitch : 90.f, player1->move || player1->strafe ? player1->move : 1, player1->strafe, dir);
						dir.normalize();
						dir.mul(ph.jumpvelocity(player1));
						player1->vel.add(dir);
						player1->lastimpulse = lastmillis;
						player1->jumping = false;
					}
				}
				else player1->lastimpulse = 0;

				ph.move(player1, 20, true);
				ph.updatewater(player1, 0);

				if (player1->physstate >= PHYS_SLOPE) swaymillis += curtime;

                if (player1->crouching)
                {
                    crouching = min(1.0f, crouching + curtime/300.0f);
                }
                else crouching = max(0.0f, crouching - curtime/300.0f);

				float k = pow(0.7f, curtime/10.0f);
				swaydir.mul(k);
                vec vel(player1->vel);
                vel.add(player1->falling);
                swaydir.add(vec(vel).mul((1-k)/(15*max(vel.magnitude(), ph.maxspeed(player1)))));

				et.checkitems(player1);
				if (player1->attacking) ws.shoot(player1, worldpos);
				if (player1->reloading || doautoreload()) ws.reload(player1);
			}
		}
		if (player1->clientnum >= 0) c2sinfo();
	}

	void damaged(int gun, int flags, int damage, fpsent *d, fpsent *actor, int millis, vec &dir)
	{
		if(d->state != CS_ALIVE || intermission) return;

		d->dodamage(damage, millis);
		d->superdamage = 0;

		if (actor->type == ENT_PLAYER) actor->totaldamage += damage;

		if (actor == player1)
		{
			int snd;
			if (damage > 200) snd = 0;
			else if (damage > 175) snd = 1;
			else if (damage > 150) snd = 2;
			else if (damage > 125) snd = 3;
			else if (damage > 100) snd = 4;
			else if (damage > 50) snd = 5;
			else snd = 6;
			playsound(S_DAMAGE1+snd);
			lasthit = lastmillis;
		}

		if (d == player1)
		{
			camerawobble += damage;
			damageresidue += damage;
			d->hitpush(damage, dir, actor, gun);
			d->damageroll(damage);
		}

		if (d->type == ENT_PLAYER)
		{
			playsound(S_PAIN1+rnd(5), &d->o);
			ws.damageeffect(damage, d);
		}
	}

	void killed(int gun, int flags, int damage, fpsent *d, fpsent *actor)
	{
		if(d->state!=CS_ALIVE || intermission) return;

		static const char *obitnames[NUMGUNS] = {
			"ate a bullet from", "got filled with buckshot by", "was gunned down by",
			"was blown to pieces by", "rode the wrong end of a rocket from",
			"was sniped by"
		};
		string dname, aname, oname;
		s_strcpy(dname, colorname(d));
		s_strcpy(aname, actor->type!=ENT_INANIMATE ? colorname(actor) : "");
		s_strcpy(oname, flags&HIT_HEAD ? "was shot in the head by" : (gun >= 0 && gun < NUMGUNS ? obitnames[gun] : "was killed by"));
		int cflags = (d==player1 || actor==player1 ? CON_CENTER : 0)|CON_NORMAL;
        if(d==actor || actor->type==ENT_INANIMATE) console("\f2%s killed themself", cflags, dname);
		else if(actor->type==ENT_AI) console("\f2%s %s %s", cflags, aname, oname, dname);
		else if(m_team(gamemode, mutators) && isteam(d->team, actor->team)) console("\f2%s %s teammate %s", cflags, dname, oname, aname);
		else console("\f2%s %s %s", cflags, dname, oname, aname);

		d->state = CS_DEAD;
        d->superdamage = max(-d->health, 0);

		if (d == player1)
		{
			sb.showscores(true);
			lastplayerstate = *player1;
			d->attacking = d->reloading = d->useaction = d->leaning = false;
			d->deaths++;
			d->pitch = 0;
			d->roll = 0;
			playsound(lastmillis-d->lastspawn < 10000 ? S_V_OWNED : S_V_FRAGGED);
		}
		else
		{
            d->move = d->strafe = 0;
		}

		playsound(S_DIE1+rnd(2), &d->o);
		ws.superdamageeffect(d->vel, d);

		if (d != actor)
		{
			actor->spree++;
			switch (actor->spree)
			{
				case 5:  playsound(S_V_SPREE1, &actor->o); break;
				case 10: playsound(S_V_SPREE2, &actor->o); break;
				case 25: playsound(S_V_SPREE3, &actor->o); break;
				case 50: playsound(S_V_SPREE4, &actor->o); break;
				default:
				{
					if (flags&HIT_HEAD) playsound(S_V_HEADSHOT, &actor->o);
					else playsound(S_DAMAGE8, &actor->o);
					break;
				}
			}
		}
	}

	void timeupdate(int timeremain)
	{
		minremain = timeremain;
		if(!timeremain)
		{
			intermission = true;
			player1->attacking = player1->reloading = player1->useaction = player1->leaning = false;
			if (m_mp(gamemode))
			{
				calcranks();

				if ((m_team(gamemode, mutators) && isteam(player1->team, teamscores[0].team)) ||
					(!m_team(gamemode, mutators) && shplayers.length() && shplayers[0] == player1))
				{
					conoutf("\f2intermission: you win!");
					playsound(S_V_YOUWIN);
				}
				else
				{
					conoutf("\f2intermission: you lose!");
					playsound(S_V_YOULOSE);
				}
			}
			else
			{
				conoutf("\f2intermission: the game has ended!");
			}
			sb.showscores(true);
		}
		else if(timeremain > 0)
		{
			console("\f2time remaining: %d %s", CON_NORMAL|CON_CENTER, timeremain, timeremain==1 ? "minute" : "minutes");
		}
	}

	fpsent *newclient(int cn)	// ensure valid entity
	{
		if(cn<0 || cn>=MAXCLIENTS)
		{
			neterr("clientnum");
			return NULL;
		}
		while(cn>=players.length()) players.add(NULL);
		if(!players[cn])
		{
			fpsent *d = new fpsent();
			d->clientnum = cn;
			players[cn] = d;
		}
		return players[cn];
	}

	fpsent *getclient(int cn)	// ensure valid entity
	{
		return players.inrange(cn) ? players[cn] : NULL;
	}

	void clientdisconnected(int cn)
	{
		if(!players.inrange(cn)) return;
		if (cameranum == -cn) cameradir(1, true);
		fpsent *d = players[cn];
		if(!d) return;
		if(d->name[0]) conoutf("player %s disconnected", colorname(d));
		pj.remove(d);
        removetrackedparticles(d);
		DELETEP(players[cn]);
		cleardynentcache();
	}

	void initclient()
	{
		if (serverhost) connects("localhost");
		else lanconnect();
	}

    void preload()
    {
        ws.preload();
        fr.preload();
        et.preload();
		if (m_capture(gamemode)) cpc.preload();
        else if(m_ctf(gamemode)) ctf.preload();
    }

	void startmap(const char *name)	// called just after a map load
	{
        player1->respawned = player1->suicided = 0;
		respawnent = -1;
        lasthit = 0;
		cc.mapstart();
		pj.reset();

		// reset perma-state
		player1->frags = 0;
		player1->deaths = 0;
		player1->totaldamage = 0;
		player1->totalshots = 0;
		loopv(players) if(players[i])
		{
			players[i]->frags = 0;
			players[i]->deaths = 0;
			players[i]->totaldamage = 0;
			players[i]->totalshots = 0;
		}

		et.findplayerspawn(player1, -1, -1);
		et.resetspawns();
		sb.showscores(false);
		intermission = false;
        maptime = 0;
		if(m_sp(gamemode))
		{
			s_sprintfd(aname)("bestscore_%s", getmapname());
			const char *best = getalias(aname);
			if(*best) conoutf("\f2try to beat your best score so far: %s", best);
		}
		cameranum = 0;
	}

	void playsoundc(int n, fpsent *d = NULL)
	{
		fpsent *c = d ? d : player1;
		if (c == player1 || c->ownernum == player1->clientnum)
			cc.addmsg(SV_SOUND, "i2", c->clientnum, n);
		playsound(n, &c->o);
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

	bool duplicatename(fpsent *d, char *name = NULL)
	{
		if(!name) name = d->name;
		if(d!=player1 && !strcmp(name, player1->name)) return true;
		loopv(players) if(players[i] && d!=players[i] && !strcmp(name, players[i]->name)) return true;
		return false;
	}

	char *colorname(fpsent *d, char *name = NULL, const char *prefix = "")
	{
		if(!name) name = d->name;
		if(name[0] && !duplicatename(d, name)) return name;
		static string cname;
		s_sprintf(cname)("%s%s \fs\f5(%d)\fS", prefix, name, d->clientnum);
		return cname;
	}

	void suicide(fpsent *d)
	{
		if(d == player1 || d->ownernum == player1->clientnum)
		{
			if(d->state!=CS_ALIVE) return;
			if(d->suicided!=d->lifesequence)
			{
				cc.addmsg(SV_SUICIDE, "ri", d->clientnum);
				d->suicided = d->lifesequence;
			}
		}
	}

	void drawradar(float x, float y, float s)
	{
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x,	y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x+s, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x+s, y+s);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x,	y+s);
	}


	void drawblips(int x, int y, int s)
	{
		physent *d = player1->state == CS_SPECTATOR || player1->state == CS_EDITING ? camera1 : player1;

		loopv(players) if (players[i] && players[i]->state == CS_ALIVE)
		{
			fpsent *f = players[i];
			vec dir(f->o);
			dir.sub(d->o);
			dir.z = 0.0f;
			float dist = dir.magnitude();
			if(dist >= MAXRADAR) continue;
			dir.rotate_around_z(-d->yaw*RAD);
			settexture(m_team(gamemode, mutators) && isteam(player1->team, f->team) ? "textures/blip_blue.png" : "textures/blip_red.png");
			glBegin(GL_QUADS);
			drawradar(x + s*0.5f*0.95f*(1.0f+dir.x/MAXRADAR), y + s*0.5f*0.95f*(1.0f+dir.y/MAXRADAR), (f->crouching ? 0.05f : 0.025f)*s);
			glEnd();
		}
		loopv(et.ents)
		{
			extentity &e = *et.ents[i];
			if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
			enttypes &t = enttype[e.type];
			if((t.usetype == ETU_ITEM && e.spawned) || player1->state == CS_EDITING)
			{
				vec dir(e.o);
				dir.sub(d->o);
				dir.z = 0.0f;
				float dist = dir.magnitude();
				if(dist >= MAXRADAR) continue;
				dir.rotate_around_z(-d->yaw*RAD);
				settexture("textures/blip_green.png");
				glBegin(GL_QUADS);
				drawradar(x + s*0.5f*0.95f*(1.0f+dir.x/MAXRADAR), y + s*0.5f*0.95f*(1.0f+dir.y/MAXRADAR), 0.025f*s);
				glEnd();
			}
		}
	}

	void drawhud(int w, int h)
	{
		if (maptime || !cc.ready())
		{
			if (!hidehud && !editmode)
			{
				int ox = w*900/h, oy = 900;

				glLoadIdentity();
				glOrtho(0, ox, oy, 0, -1, 1);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				int secs = lastmillis-maptime;
				float fade = 1.f, amt = hudblend*0.01f;

				if (secs <= CARDTIME+CARDFADE)
				{
					int x = ox;

					if (secs <= CARDTIME) x = int((float(secs)/float(CARDTIME))*(float)ox);
					else if (secs <= CARDTIME+CARDFADE) fade -= (float(secs-CARDTIME)/float(CARDFADE));

					const char *title = getmaptitle();
					if (!*title) title = "Untitled by Unknown";

					glColor4f(1.f, 1.f, 1.f, amt);

					rendericon("textures/logo.jpg", ox+20-x, oy-75, 64, 64);

					draw_textx("%s", ox+100-x, oy-75, 255, 255, 255, int(255.f*fade), false, AL_LEFT, title);

					glColor4f(1.f, 1.f, 1.f, fade);
					rendericon("textures/overlay.png", ox+20-x, oy-260, 144, 144);
					if(!rendericon(picname, ox+28-x, oy-252, 128, 128))
						rendericon("textures/logo.jpg", ox+20-x, oy-260, 144, 144);

					draw_textx("%s", ox+180-x, oy-180, 255, 255, 255, int(255.f*fade), false, AL_LEFT, sv->gamename(gamemode, mutators));
				}
				else
				{
					fpsent *d = player1;

					if (lastmillis-maptime <= CARDTIME+CARDFADE+CARDFADE)
						fade = amt*(float(lastmillis-maptime-CARDTIME-CARDFADE)/float(CARDFADE));
					else fade *= amt;

					if (player1->state == CS_SPECTATOR)
					{
						if (player1->clientnum == -cameranum)
							d = player1;
						else if (players.inrange(-cameranum) && players[-cameranum])
							d = players[-cameranum];
					}

					if (fov < 90 && d->gunselect == GUN_RIFLE && d->state == CS_ALIVE)
					{
						settexture("textures/overlay_zoom.png");

						glColor4f(1.f, 1.f, 1.f, 1.f);

						glBegin(GL_QUADS);

						glTexCoord2f(0, 0); glVertex2i(0, 0);
						glTexCoord2f(1, 0); glVertex2i(ox, 0);
						glTexCoord2f(1, 1); glVertex2i(ox, oy);
						glTexCoord2f(0, 1); glVertex2i(0, oy);

						glEnd();
					}

					if (damageresidue > 0 || d->state == CS_DEAD)
					{
						int dam = d->state == CS_DEAD ? 100 : min(damageresidue, 100);
						float pc = float(dam)/100.f;
						settexture("textures/overlay_damage.png");

						glColor4f(1.f, 1.f, 1.f, pc);

						glBegin(GL_QUADS);

						glTexCoord2f(0, 0); glVertex2i(0, 0);
						glTexCoord2f(1, 0); glVertex2i(ox, 0);
						glTexCoord2f(1, 1); glVertex2i(ox, oy);
						glTexCoord2f(0, 1); glVertex2i(0, oy);

						glEnd();
					}

					if (d != NULL)
					{
						if (d->state == CS_ALIVE)
						{
							float hlt = d->health/float(MAXHEALTH), glow = min((hlt*0.5f)+0.5f, 1.f), pulse = fade;
							if (lastmillis < d->lastregen+500) pulse *= (lastmillis-d->lastregen)/500.f;
							glColor4f(glow, 0.f, 0.f, pulse);

							int rw = oy/5/4, rx = oy/5+8, rh = oy/5, ry = oy-rh-4, ro = int(((oy/5)-(oy/30))*hlt), off = rh-ro-(oy/30);
							settexture("textures/barv.png");
							glBegin(GL_QUADS);
							glTexCoord2f(0, 0); glVertex2i(rx, ry+off);
							glTexCoord2f(1, 0); glVertex2i(rx+rw, ry+off);
							glTexCoord2f(1, 1); glVertex2i(rx+rw, ry+rh);
							glTexCoord2f(0, 1); glVertex2i(rx, ry+rh);
							glEnd();

							if(isgun(d->gunselect) && d->ammo[d->gunselect] > 0)
							{
								draw_textx("%d", oy/5+rw+16, oy-75, 255, 255, 255, int(255.f*fade), false, AL_LEFT, d->ammo[d->gunselect]);
							}
							else
							{
								draw_textx("Out of ammo", oy/5+rw+16, oy-75, 255, 255, 255, int(255.f*fade), false, AL_LEFT);
							}
						}
						else if (d->state == CS_DEAD)
						{
							int wait = respawnwait(d);

							if (wait)
							{
								float c = float(wait)/1000.f;
								draw_textx("Fragged! Down for %.1fs", oy/5+16, oy-75, 255, 255, 255, int(255.f*fade), false, AL_LEFT, c);
							}
							else
								draw_textx("Fragged! Press attack to respawn", oy/5+16, oy-75, 255, 255, 255, int(255.f*fade), false, AL_LEFT);
						}
					}

					glColor4f(1.f, 1.f, 1.f, fade);

					int rx = 4, rs = oy/5, ry = oy-rs-4;
					settexture("textures/radar.png");
					glBegin(GL_QUADS);
					drawradar(float(rx), float(ry), float(rs));
					glEnd();
					drawblips(rx, ry, rs);

					if(m_capture(gamemode)) cpc.drawhud(ox, oy);
					else if(m_ctf(gamemode)) ctf.drawhud(ox, oy);
				}
				glDisable(GL_BLEND);
			}
		}
		else
		{
			glLoadIdentity();
			glOrtho(0, w, h, 0, -1, 1);
			glColor3f(1, 1, 1);

			settexture("textures/loadback.jpg");

			glBegin(GL_QUADS);

			glTexCoord2f(0, 0); glVertex2i(0, 0);
			glTexCoord2f(1, 0); glVertex2i(w, 0);
			glTexCoord2f(1, 1); glVertex2i(w, h);
			glTexCoord2f(0, 1); glVertex2i(0, h);

			glEnd();
		}
	}

    const char *defaultcrosshair(int index)
    {
        switch(index)
        {
            case 1: return "textures/crosshair_team.png";
            case 2: return "textures/crosshair_hit.png";
            case 3: return "textures/crosshair_zoom.png";
            default: return "textures/crosshair.png";
        }
    }
    int selectcrosshair(float &r, float &g, float &b)
    {
        if(player1->state!=CS_ALIVE) return 0;

        int crosshair = 0;
        if(fov < 90) crosshair = 3;
        else if(lastmillis - lasthit < hitcrosshair()) crosshair = 2;
        else if(m_team(gamemode, mutators) && teamcrosshair())
        {
            dynent *d = ws.intersectclosest(player1->o, worldpos, player1);
            if(d && d->type==ENT_PLAYER && isteam(((fpsent *)d)->team, player1->team))
            {
                crosshair = 1;
                r = g = 0;
            }
        }

        if(player1->gunwait) { r *= 0.5f; g *= 0.5f; b *= 0.5f; }
        else if(!crosshair && r && g && b && !editmode && !m_insta(gamemode, mutators))
        {
            if(player1->health<=25) { r = 1.0f; g = b = 0; }
            else if(player1->health<=50) { r = 1.0f; g = 0.5f; b = 0; }
        }
        return crosshair;
    }

	void lighteffects(dynent *e, vec &color, vec &dir)
	{
	}

    void particletrack(physent *owner, vec &o, vec &d)
    {
        if(owner->type!=ENT_PLAYER && owner->type!=ENT_AI) return;
        float dist = o.dist(d);
        vecfromyawpitch(owner->yaw, owner->pitch, 1, 0, d);
        float newdist = raycube(owner->o, d, dist, RAY_CLIPMAT|RAY_POLY);
        d.mul(min(newdist, dist)).add(owner->o);
        o = ws.gunorigin(GUN_PISTOL, owner->o, d, (fpsent *)owner);
    }

	void newmap(int size)
	{
		cc.addmsg(SV_NEWMAP, "ri", size);
	}

	void editvariable(const char *name, int value)
	{
        if(m_edit(gamemode)) cc.addmsg(SV_EDITVAR, "rsi", name, value);
	}

	void edittrigger(const selinfo &sel, int op, int arg1, int arg2, int arg3)
	{
        if(m_edit(gamemode)) switch(op)
		{
			case EDIT_FLIP:
			case EDIT_COPY:
			case EDIT_PASTE:
			case EDIT_DELCUBE:
			{
				cc.addmsg(SV_EDITF + op, "ri9i4",
					sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
					sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner);
				break;
			}
			case EDIT_MAT:
			case EDIT_ROTATE:
			{
				cc.addmsg(SV_EDITF + op, "ri9i5",
					sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
					sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
					arg1);
				break;
			}
			case EDIT_FACE:
			case EDIT_TEX:
			case EDIT_REPLACE:
			{
				cc.addmsg(SV_EDITF + op, "ri9i6",
					sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
					sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
					arg1, arg2);
				break;
			}
            case EDIT_REMIP:
            {
                cc.addmsg(SV_EDITF + op, "r");
                break;
            }
		}
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
		if (!maptime && lastmillis-maptime <= CARDTIME)
		{
			float fade = maptime ? float(lastmillis-maptime)/float(CARDTIME) : 0.f;
			colour = vec(fade, fade, fade);
			return true;
		}
		return false;
	}

	void fixrange(physent *d)
	{
		const float MAXPITCH = 89.9f;
		const float MAXROLL = 35.0f;

		if (d->pitch > MAXPITCH) d->pitch = MAXPITCH;
		if (d->pitch < -MAXPITCH) d->pitch = -MAXPITCH;

		if (d->roll > MAXROLL) d->roll = MAXROLL;
		if (d->roll < -MAXROLL) d->roll = -MAXROLL;

		while (d->yaw < 0.0f) d->yaw += 360.0f;
		while (d->yaw > 360.0f) d->yaw -= 360.0f;
	}

	void fixview()
	{
		if (fov > MAXFOV) fov = MAXFOV;
		if (fov < MINFOV) fov = MINFOV;

		fixrange(player1);
	}

	void mousemove(int dx, int dy)
	{
		if (player1->state != CS_DEAD)
		{
			const float SENSF = 33.0f;	 // try match quake sens

			if (player1->leaning) player1->roll += (dx/SENSF)*(rollsensitivity()/(float)sensitivityscale());
			else player1->yaw += (dx/SENSF)*(yawsensitivity()/(float)sensitivityscale());
			player1->pitch -= (dy/SENSF)*(pitchsensitivity()/(float)sensitivityscale())*(invmouse() ? -1 : 1);

			fixrange(player1);
		}
	}

	physent fpscamera;

	void findorientation(physent *d, vec &pos)
	{
		vec dir;
		vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
		vecfromyawpitch(d->yaw, 0, 0, -1, camright);
		vecfromyawpitch(d->yaw, d->pitch+90, 1, 0, camup);

		if(raycubepos(d->o, dir, pos, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
			pos = dir.mul(2*hdr.worldsize).add(d->o); //otherwise 3dgui won't work when outside of map
	}

	void recomputecamera()
	{
		int secs = time(NULL), lastcam = cameratype;
		camera1 = &fpscamera;
		cameratype = CAMERA_NONE;

		fixview();

		if (camera1->type != ENT_CAMERA)
		{
			camera1->reset();
			camera1->type = ENT_CAMERA;
			camera1->state = CS_ALIVE;
		}

		if (player1->state == CS_SPECTATOR)
		{
			if (cameracycle() > 0 && secs-cameracycled > cameracycle())
			{
				cameradir(1, true);
				cameracycled = secs;
			}

			if (players.inrange(-cameranum) && players[-cameranum])
			{
				cameratype = CAMERA_FOLLOW;
				camera1->o = players[-cameranum]->o;
				camera1->vel = players[-cameranum]->vel;
				camera1->yaw = players[-cameranum]->yaw + player1->yaw;
				camera1->pitch = players[-cameranum]->pitch + player1->pitch;
				camera1->roll = players[-cameranum]->roll;
				camera1->radius = players[-cameranum]->radius;
				camera1->height = players[-cameranum]->height;

				if (!gamethirdperson())
				{
					if (players[-cameranum]->crouching)
						camera1->o.z -= crouching*(1-CROUCHHEIGHT)*players[-cameranum]->height;
				}
			}
			else if (player1->clientnum != -cameranum)
			{
				int cameras = 1;

				loopv (et.ents)
				{
					if (et.ents[i]->type == CAMERA)
					{
						if (cameras == cameranum)
						{
							cameratype = CAMERA_ENTITY;
							camera1->o = et.ents[i]->o;
							camera1->vel = vec(0, 0, 0);
							camera1->yaw = et.ents[i]->attr1 + player1->yaw;
							camera1->pitch = et.ents[i]->attr2 + player1->pitch;
							camera1->roll = player1->roll;
							camera1->radius = player1->radius;
							camera1->height = player1->height;
							break;
						}
						cameras++;
					}
				}
			}
		}
		if (cameratype <= CAMERA_NONE)
		{
			cameratype = CAMERA_PLAYER;
			camera1->o = player1->o;
			camera1->vel = player1->vel;
			camera1->yaw = player1->yaw;
			camera1->pitch = player1->pitch;
			camera1->roll = player1->roll;
			camera1->radius = player1->radius;
			camera1->height = player1->height;

			if (!gamethirdperson())
			{
				if (player1->crouching)
					camera1->o.z -= crouching*(1-CROUCHHEIGHT)*player1->height;

				if (player1->state == CS_DEAD)
				{
					camera1->o.z -= (player1->height/2000.f)*float(min(lastmillis-player1->lastpain, 2000));
					camera1->o.z = max(player1->o.z - player1->height + 1.0f, camera1->o.z);
				}
			}
		}

		if (gamethirdperson())
		{
			vec pos;

			vecfromyawpitch(camera1->yaw, camera1->pitch, -1, 0, pos);
			camera1->o.x += pos.x*thirdpersondist();
			camera1->o.y += pos.y*thirdpersondist();
			camera1->o.z += (pos.z*thirdpersondist())+thirdpersonheight();

			findorientation(player1, pos);
			vec rot(pos);
			rot.sub(camera1->o);
			rot.normalize();
			vectoyawpitch(rot, camera1->yaw, camera1->pitch);
		}
		else
		{
			vec off;
			vecfromyawpitch(camera1->yaw, min(camera1->pitch,80.f)+60.f, 1, 0, off);
			off.x *= camera1->radius+1.f;
			off.y *= camera1->radius+1.f;
			off.z = max(off.z, 0.f);
			off.z *= camera1->height*0.5f;
			off.sub(vec(0, 0, camera1->height*0.5f));
			camera1->o.add(off);

			vec sway;
			vecfromyawpitch(camera1->yaw, camera1->pitch, 1, 0, sway);
            float swayspeed = sqrtf(player1->vel.x*player1->vel.x + player1->vel.y*player1->vel.y);
            swayspeed = min(4.0f, swayspeed);
			sway.mul(swayspeed);
			float swayxy = sinf(swaymillis/115.0f)/100.0f, swayz = cosf(swaymillis/115.0f)/100.0f;
			swap(sway.x, sway.y);
			sway.x *= -swayxy;
			sway.y *= swayxy;
			sway.z = -fabs(swayspeed*swayz);
			sway.add(swaydir);
			camera1->o.add(sway);
		}

		if (camerawobble > 0 && cameratype == lastcam)
		{
			float pc = float(min(camerawobble, 100))/100.f;
			#define wobble (float(rnd(8)-4)*pc)
			camera1->yaw += wobble;
			camera1->pitch += wobble;
			camera1->roll += wobble;
		}
		else camerawobble = 0;

		fixview();
		fixrange(camera1);
		findorientation(camera1, worldpos);
	}

	void adddynlights()
	{
		pj.adddynlights();
	}

	bool wantcrosshair()
	{
		return (crosshair() &&
			!(hidehud || player1->state == CS_SPECTATOR || player1->state == CS_DEAD)) ||
			menuactive();
	}

	bool gamethirdperson()
	{
		return cameratype <= CAMERA_FOLLOW && thirdperson() &&
			player1->state != CS_EDITING && player1->state != CS_SPECTATOR &&
			(cameratype != CAMERA_PLAYER || player1->gunselect != GUN_RIFLE || fov >= 90);
	}

	void menuevent(int event)
	{
		int s = -1;
		switch (event)
		{
			case MN_BACK: s = S_MENUBACK; break;
			case MN_INPUT: s = S_MENUPRESS; break;
			default: break;
		}
		if (s >= 0) playsound(s);
	}

	void cameradir(int dir, bool quiet = false)
	{
		int cams = 0, cam_lo = 0, cam_hi = 0;

		cam_lo = players.length();

		loopv(et.ents) if (et.ents[i]->type == CAMERA) cam_hi++;

		if ((cams = cam_lo + cam_hi) > 1)
		{
			while (true)
			{
				cameranum += dir;

				while (cameranum < -cam_lo) cameranum += cams;
				while (cameranum > cam_hi) cameranum -= cams;

				if (cameranum > 0 || player1->clientnum == -cameranum ||
					(players.inrange(-cameranum) && players[-cameranum])) break;
			}
			if (!quiet)
			{
				if (cameranum > 0) console("camera %d selected", CON_NORMAL|CON_CENTER, cameranum);
				else if (player1->clientnum == -cameranum) console("spectator camera selected", CON_NORMAL|CON_CENTER);
				else console("chasecam %s selected", CON_NORMAL|CON_CENTER, players[-cameranum]->name);
			}
		}
		else if (!quiet) console("no other cameras available", CON_NORMAL|CON_CENTER);
	}

	void setcamera(int idx)
	{
		if (idx>=0)
		{
			if ((player1->state == CS_SPECTATOR) || (player1->state == CS_EDITING))
			{
				int foo_delta; // we will ignore the delta attribute (it's for AwayCAM (still to come))
				et.gotocamera(idx, player1, foo_delta);
			}
			else
			{
				conoutf ("need to be editing or spectator");
			}
		}
	}

	static int mteamscorecmp(const teamscore *x, const teamscore *y)
	{
		if(x->score > y->score)
			return -1;
		if(x->score < y->score)
			return 1;
		return 0;
	}

	static int mplayersort(const fpsent **a, const fpsent **b)
	{
		return (int)((*a)->frags<(*b)->frags)*2-1;
	}

	void calcranks()
	{
		bool hold = false;

		shplayers.setsize(0);

		loopi(numdynents())
		{
			fpsent *o = (fpsent *)iterdynents(i);
			if(o && (o->type != ENT_AI))
				shplayers.add(o);
		}

		shplayers.sort(mplayersort);

		if(teamscores.length())
			teamscores.setsize(0);

		if(m_team(gamemode, mutators))
		{
			if(m_capture(gamemode))
			{
				loopv(cpc.scores) if(cpc.scores[i].total)
					teamscores.add(teamscore(cpc.scores[i].team, cpc.scores[i].total));
			}
			else
				loopi(numdynents())
			{
				fpsent *o = (fpsent *)iterdynents(i);
				if(o && o->type!=ENT_AI && o->frags)
				{
					teamscore *ts = NULL;
					loopv(teamscores) if(!strcmp(teamscores[i].team, o->team))
					{
						ts = &teamscores[i];
						break;
					}
					if(!ts)
						teamscores.add(teamscore(o->team, o->frags));
					else
						ts->score += o->frags;
				}
			}
			teamscores.sort(mteamscorecmp);
		}

		string rinfo;
		rinfo[0] = 0;

		loopv(shplayers)
		{
			if (shplayers[i]->clientnum == player1->clientnum)
			{
				if (i != myrankv)
				{
					if (i==0)
					{
						if (player1->state != CS_SPECTATOR)
						{
							hold = true;
							s_sprintf(rinfo)("You've taken %s", myrankv!=-1?"\fythe lead":"\frfirst blood");
						}
					}
					else
					{
						if (myrankv==0)
						{
							hold = true;
							s_sprintf(rinfo)("\f2%s \fftakes \f2the lead", colorname(shplayers[0]));
						}
						int df = shplayers[0]->frags - shplayers[i]->frags;
						string cmbN;
						if (player1->state == CS_SPECTATOR)
						{
							df = shplayers[0]->frags - shplayers[i==1?2:1]->frags;
							string dfs;
							if (df)
								s_sprintf(dfs)("+%d frags", df);
							else
								s_sprintf(dfs)("%s","tied for the lead");
							s_sprintf(cmbN)("%s%s%s \f3VS\ff %s\n%s", rinfo[0]?rinfo:"", rinfo[0]?"\n":"", colorname(shplayers[0]), colorname(shplayers[i==1?2:1]), dfs);
						}
						else
						{
							if(df)
								s_sprintf(cmbN)("%s%s\f%d%d\ff %s", rinfo[0]?rinfo:"", rinfo[0]?"\n":"", df>0?3:1, abs(df), colorname(shplayers[0]));
							else
								s_sprintf(cmbN)("%s%s%s \f3VS\ff %s", rinfo[0]?rinfo:"", rinfo[0]?"\n":"", colorname(shplayers[i]), colorname(shplayers[i?0:1]));
						}
						s_sprintf(rinfo)("%s", cmbN);
					}
					myrankv = i;
				}
				else if (myranks)
				{
					if (player1->state == CS_SPECTATOR)
					{
						if (lastmillis > myranks + ranktime())
						{
							if (shplayers.length()>2)
							{
								int df = shplayers[0]->frags - shplayers[i==1?2:1]->frags;
								string dfs;
								if (df)
									s_sprintf(dfs)("+%d frags", df);
								else
									s_sprintf(dfs)("%s","tied for the lead");
								s_sprintf(rinfo)("%s \f3VS\ff %s\n%s", colorname(shplayers[0]), colorname(shplayers[i==1?2:1]), dfs);
							}
						}
					}
					else
					{
						if (lastmillis > myranks + ranktime())
						{
							int df;
							if (shplayers.length()>1)
							{
								if(i)
									df= shplayers[0]->frags - shplayers[i]->frags;
								else
									df = shplayers[1]->frags - shplayers[0]->frags;
								if(df)
									s_sprintf(rinfo)("\f%d%d\ff %s", df>0?3:1, abs(df), colorname(shplayers[df>0?0:1]));
								else
									s_sprintf(rinfo)("%s \f3VS\ff %s", colorname(shplayers[i]), colorname(shplayers[i?0:1]));
							}
						}
					}

				}
				myranks = lastmillis;
			}
		}
		if (rankhud() && rinfo[0]) { console("%s", CON_CENTER, rinfo); }
	}

	void setcrank()
	{
		console("\f2%d%s place", CON_NORMAL|CON_CENTER, myrankv+1, myrankv ? myrankv == 1 ? "nd" : myrankv == 2 ? "rd" : "th" : "st");
	}

	char *gametitle()
	{
		return sv->gamename(gamemode, mutators);
	}

	char *gametext()
	{
		return getmapname();
	}
};
