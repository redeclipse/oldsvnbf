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
	int quakewobble, damageresidue;

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

	IVARP(mousedeadzone, 0, 10, 100);
	IVARP(mousepanspeed, 0, 30, 1000);
	IVARP(mousesensitivity, 0, 10, 100);

	IVARP(yawsensitivity, 0, 10, 1000);
	IVARP(pitchsensitivity, 0, 7, 1000);
	IVARP(rollsensitivity, 0, 3, 1000);
	IVARP(sensitivityscale, 1, 1, 100);

	IVARP(autoreload, 0, 1, 1);// auto reload when empty

	IVARP(crosshair, 0, 1, 1);// show the crosshair
	IVARP(teamcrosshair, 0, 1, 1);
	IVARP(hitcrosshair, 0, 425, 1000);

    GAMECLIENT()
		: ph(*this), pj(*this), ws(*this), sb(*this), fr(*this), et(*this), cc(*this), bot(*this), cpc(*this), ctf(*this),
			nextmode(sv->defaultmode()), nextmuts(0), gamemode(sv->defaultmode()), mutators(0), intermission(false),
			maptime(0), minremain(0), respawnent(-1),
			swaymillis(0), swaydir(0, 0, 0),
			player1(spawnstate(new fpsent()))
	{
        CCOMMAND(kill, "",  (GAMECLIENT *self), { self->suicide(self->player1); });
		CCOMMAND(mode, "ii", (GAMECLIENT *self, int *val, int *mut), { self->setmode(*val, *mut); });
		CCOMMAND(gamemode, "", (GAMECLIENT *self), intret(self->gamemode));
		CCOMMAND(mutators, "", (GAMECLIENT *self), intret(self->mutators));

		CCOMMAND(sensitivity, "s", (GAMECLIENT *self, char *s), {
				if(*s)
				{
					int x = atoi(s);
					if(x)
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
		playsound(S_RESPAWN, &d->o);
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
		if(m_capture(gamemode)) wait = cpc.respawnwait(d);
		else if(m_ctf(gamemode)) wait = ctf.respawnwait(d);
		return wait;
	}

	void respawn(fpsent *d)
	{
		if(d->state == CS_DEAD)
		{
			int wait = respawnwait(d);

			if(wait)
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
        if(d->type != ENT_PLAYER) return true;
        fpsent *e = (fpsent *)d;
        return !intermission && lastmillis-e->lasttaunt >= 1000;
    }

	void respawnself(fpsent *d)
	{
		d->stopmoving();

        if(m_mp(gamemode))
        {
            if(d->respawned != d->lifesequence)
            {
                cc.addmsg(SV_TRYSPAWN, "ri", d->clientnum);
                d->respawned = d->lifesequence;
            }
        }
        else
        {
            spawnplayer(d);
            if(d==player1)
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
		if(!m_game(mode)) { conoutf("mode %d is not a valid gameplay type", mode); return; }
		if(m_sp(mode)) { conoutf("playing %s is not yet supported", gametype[mode]); return; }
		if(multiplayer(false) && !m_mp(mode)) { conoutf("playing %s is not supported in multiplayer", gametype[mode]); return; }
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
        if(!maptime) { maptime = lastmillis + curtime; return; }
		if(!curtime) return;

		ph.update();
		pj.update();
		et.update();
		bot.update();
		gets2c();

		if(cc.ready())
		{
			if(!allowmove(player1) || saycommandon) player1->stopmoving();

			#define adjust(t,n,m) \
				if(n != 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/float(m))); }

			adjust(float, player1->roll, 100);
			adjust(int, quakewobble, 100);
			adjust(int, damageresidue, 200);

			if(player1->state == CS_ALIVE || player1->state == CS_DEAD)
			{
				float fx, fy;
				vectoyawpitch(cursordir, fx, fy);
				findorientation(camera1->o, fx, fy, worldpos, true);
			}

			if(player1->state == CS_DEAD)
			{
				if(lastmillis-player1->lastpain < 2000)
				{
					player1->stopmoving();
					ph.move(player1, 10, false);
				}
			}
			else if(player1->state == CS_ALIVE)
			{
				vec v(worldpos);
				v.sub(player1->o);
				v.normalize();
				vectoyawpitch(v, player1->yaw, player1->pitch);
				//fixrange(player1->yaw, player1->pitch);

				if(player1->timeinair)
				{
					if(player1->jumping && lastmillis-player1->lastimpulse > ph.gravityforce(player1)*100)
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

				if(player1->physstate >= PHYS_SLOPE) swaymillis += curtime;

				float k = pow(0.7f, curtime/10.0f);
				swaydir.mul(k);
				vec vel(player1->vel);
				vel.add(player1->falling);
				swaydir.add(vec(vel).mul((1-k)/(15*max(vel.magnitude(), ph.maxspeed(player1)))));

				et.checkitems(player1);
				if(player1->attacking) ws.shoot(player1, worldpos);
				if(player1->reloading || doautoreload()) ws.reload(player1);
			}
			else ph.move(player1, 20, true);
		}
		if(player1->clientnum >= 0) c2sinfo();
	}

	void damaged(int gun, int flags, int damage, fpsent *d, fpsent *actor, int millis, vec &dir)
	{
		if(d->state != CS_ALIVE || intermission) return;

		d->dodamage(damage, millis);
		d->superdamage = 0;

		if(actor->type == ENT_PLAYER) actor->totaldamage += damage;

		if(actor == player1)
		{
			int snd;
			if(damage > 200) snd = 0;
			else if(damage > 175) snd = 1;
			else if(damage > 150) snd = 2;
			else if(damage > 125) snd = 3;
			else if(damage > 100) snd = 4;
			else if(damage > 50) snd = 5;
			else snd = 6;
			playsound(S_DAMAGE1+snd);
			lasthit = lastmillis;
		}

		if(d == player1)
		{
			quakewobble += damage;
			damageresidue += damage;
			d->hitpush(damage, dir, actor, gun);
		}

		if(d->type == ENT_PLAYER)
		{
			vec p = d->o;
			p.z += 0.6f*(d->height + d->aboveeye) - d->height;
			particle_splash(3, min(damage/4, 20), 10000, p);
			if(d!=player1)
			{
				s_sprintfd(ds)("@%d", damage);
				particle_text(d->abovehead(), ds, 8);
			}
			playsound(S_PAIN1+rnd(5), &d->o);
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

		if(d == player1)
		{
			sb.showscores(true);
			lastplayerstate = *player1;
			d->attacking = d->reloading = d->useaction = false;
			d->deaths++;
			d->pitch = 0;
			d->roll = 0;
			playsound(lastmillis-d->lastspawn < 10000 ? S_V_OWNED : S_V_FRAGGED);
		}
		else
		{
            d->move = d->strafe = 0;
		}

		if(d->superdamage)
		{
			vec from = d->abovehead();
			loopi(rnd(d->superdamage)+1) pj.spawn(from, d->vel, d, PRJ_GIBS);
		}
		playsound(S_DIE1+rnd(2), &d->o);

		if(d != actor)
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
					if(flags&HIT_HEAD) playsound(S_V_HEADSHOT, &actor->o);
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
			player1->attacking = player1->reloading = player1->useaction = false;
			if(m_mp(gamemode))
			{
				if((m_team(gamemode, mutators) && isteam(player1->team, teamscores[0].team)) ||
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
		if(serverhost) connects("localhost");
		else lanconnect();
	}

    void preload()
    {
        ws.preload();
        fr.preload();
        et.preload();
		if(m_capture(gamemode)) cpc.preload();
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
	}

	void playsoundc(int n, fpsent *d = NULL)
	{
		fpsent *c = d ? d : player1;
		if(c == player1 || c->ownernum == player1->clientnum)
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

    const char *defaultcrosshair(int index)
    {
        switch(index)
        {
            case 0: return "textures/guicursor.png";
            case 1: return "textures/editcursor.png";
            case 2: return "textures/crosshair.png";
            case 3: return "textures/crosshair_team.png";
            case 4: return "textures/crosshair_hit.png";
            default: return "";
        }
    }

    int selectcrosshair(float &r, float &g, float &b)
    {
        int c = 2;
        if(menuactive()) c = 0;
        else if(!crosshair() || hidehud || player1->state == CS_DEAD) c = -1;
        else if(editmode) c = 1;
        else if(lastmillis - lasthit < hitcrosshair())
        {
        	c = 3;
        	r = 1;
        	g = b = 0;
        }
        else if(m_team(gamemode, mutators) && teamcrosshair())
        {
            dynent *d = ws.intersectclosest(player1->o, worldpos, player1);
            if(d && d->type==ENT_PLAYER && isteam(((fpsent *)d)->team, player1->team))
            {
                c = 2;
                r = g = 0;
            }
        }
		if(c > 1)
		{
			if(!player1->canshoot(player1->gunselect, lastmillis)) { r *= 0.5f; g *= 0.5f; b *= 0.5f; }
			else if(!c && r && g && b && !editmode && !m_insta(gamemode, mutators))
			{
				if(player1->health<=25) { r = 1; g = b = 0; }
				else if(player1->health<=50) { r = 1; g = 0.5f; b = 0; }
			}
		}
        return c;
    }

	IVARP(radardist, 0, 256, 256);
	IVARP(editradardist, 0, 64, 1024);

	float radarrange()
	{
		float dist = float(radardist());
		if(editmode) dist = float(editradardist());
		return dist;

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
		physent *d = cc.spectator || editmode ? camera1 : player1;
		settexture("textures/blip.png");
		glBegin(GL_QUADS);
		loopv(players) if(players[i] && players[i]->state == CS_ALIVE)
		{
			fpsent *f = players[i];
			vec dir(f->o);
			dir.sub(d->o);
			float dist = dir.magnitude();
			if(dist >= radarrange()) continue;
			dir.rotate_around_z(-d->yaw*RAD);
			glColor4f(0.f, 1.f, 0.f, 1.f-(dist/radarrange()));
			drawradar(x + s*0.5f*0.95f*(1.0f+dir.x/radarrange()), y + s*0.5f*0.95f*(1.0f+dir.y/radarrange()), (f->crouching ? 0.05f : 0.025f)*s);
		}
		loopv(et.ents)
		{
			extentity &e = *et.ents[i];
			if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
			enttypes &t = enttype[e.type];
			if((t.usetype == ETU_ITEM && e.spawned) || editmode)
			{
				bool insel = (editmode && (enthover == i || entgroup.find(i) >= 0));
				vec dir(e.o);
				dir.sub(d->o);
				float dist = dir.magnitude();
				if(!insel && dist >= radarrange()) continue;
				if(dist >= radarrange()*(1 - 0.05f)) dir.mul(radarrange()*(1 - 0.05f)/dist);
				dir.rotate_around_z(-d->yaw*RAD);
				settexture("textures/blip.png");
				glColor4f(1.f, 1.f, insel ? 1.0f : 0.f, insel ? 1.f : 1.f-(dist/radarrange()));
				drawradar(x + s*0.5f*0.95f*(1.0f+dir.x/radarrange()), y + s*0.5f*0.95f*(1.0f+dir.y/radarrange()), (insel ? 0.075f : 0.025f)*s);
			}
		}
		glEnd();
	}

	IVARP(hidestats, 0, 0, 1);
	IVARP(showfpsrange, 0, 0, 1);
	IVAR(showeditstats, 0, 0, 1);
	IVAR(statrate, 0, 200, 1000);

	void drawhudelements(int w, int h)
	{
		glLoadIdentity();
		glOrtho(0, w*3, h*3, 0, -1, 1);
		int hoff = h*3-h*3/4-FONTH/2;

		char *command = getcurcommand();
		if(command) rendercommand(FONTH/2, hoff, h*3-FONTH);
		hoff += FONTH;

		drawcrosshair(w, h);
		renderconsole(w, h);

		if(!hidestats())
		{
			extern void getfps(int &fps, int &bestdiff, int &worstdiff);
			int fps, bestdiff, worstdiff;
			getfps(fps, bestdiff, worstdiff);
			#if 0
			if(showfpsrange()) draw_textx("%d+%d-%d:%d", w*3-(FONTH/4), 4, 255, 255, 255, 255, false, AL_RIGHT, -1, -1, fps, bestdiff, worstdiff, perflevel);
			else draw_textx("%d:%d", w*3-(FONTH/4), 4, 255, 255, 255, 255, false, AL_RIGHT, -1, -1, fps, perflevel);
			#else
			if(showfpsrange()) draw_textx("%d+%d-%d", w*3-(FONTH/4), 4, 255, 255, 255, 255, false, AL_RIGHT, -1, -1, fps, bestdiff, worstdiff);
			else draw_textx("%d", w*3-(FONTH/4), 4, 255, 255, 255, 255, false, AL_RIGHT, -1, -1, fps);
			#endif

			if((editmode || showeditstats()) && lastmillis-maptime > CARDTIME+CARDFADE)
			{
				static int laststats = 0, prevstats[8] = { 0, 0, 0, 0, 0, 0, 0 }, curstats[8] = { 0, 0, 0, 0, 0, 0, 0 };
				if(lastmillis - laststats >= statrate())
				{
					memcpy(prevstats, curstats, sizeof(prevstats));
					laststats = lastmillis - (lastmillis%statrate());
				}
				int nextstats[8] =
				{
					vtris*100/max(wtris, 1),
					vverts*100/max(wverts, 1),
					xtraverts/1024,
					xtravertsva/1024,
					glde,
					gbatches,
					getnumqueries(),
					rplanes
				};

				loopi(8) if(prevstats[i]==curstats[i]) curstats[i] = nextstats[i];

				draw_textf("ond:%d va:%d gl:%d(%d) oq:%d lm:%d rp:%d pvs:%d", h*3/5+FONTH/2, hoff, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6], lightmaps.length(), curstats[7], getnumviewcells()); hoff += FONTH;
				draw_textf("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", h*3/5+FONTH/2, hoff, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]); hoff += FONTH;
				draw_textf("cube %s%d", h*3/5+FONTH/2, hoff, selchildcount<0 ? "1/" : "", abs(selchildcount)); hoff += FONTH;
			}
		}

		if(editmode)
		{
			char *editinfo = executeret("edithud");
			if(editinfo)
			{
				draw_text(editinfo, h*3/5+FONTH/2, hoff); hoff += FONTH;
				DELETEA(editinfo);
			}
		}

		render_texture_panel(w, h);
	}

	void drawhud(int w, int h)
	{
		if(maptime || !cc.ready())
		{
			if(!hidehud && !menuactive())
			{
				int ox = w*900/h, oy = 900;

				glLoadIdentity();
				glOrtho(0, ox, oy, 0, -1, 1);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				int secs = lastmillis-maptime;
				float fade = 1.f, amt = hudblend*0.01f;

				if(secs <= CARDTIME+CARDFADE)
				{
					int x = ox;

					if(secs <= CARDTIME) x = int((float(secs)/float(CARDTIME))*(float)ox);
					else if(secs <= CARDTIME+CARDFADE) fade -= (float(secs-CARDTIME)/float(CARDFADE));

					const char *title = getmaptitle();
					if(!*title) title = "Untitled by Unknown";

					glColor4f(1.f, 1.f, 1.f, amt);

					rendericon("textures/logo.jpg", ox+20-x, oy-75, 64, 64);

					draw_textx("%s", ox+100-x, oy-75, 255, 255, 255, int(255.f*fade), false, AL_LEFT, -1, -1, title);

					glColor4f(1.f, 1.f, 1.f, fade);
					rendericon("textures/guioverlay.png", ox+20-x, oy-260, 144, 144);
					if(!rendericon(picname, ox+28-x, oy-252, 128, 128))
						rendericon("textures/logo.jpg", ox+20-x, oy-260, 144, 144);

					draw_textx("%s", ox+180-x, oy-180, 255, 255, 255, int(255.f*fade), false, AL_LEFT, -1, -1, sv->gamename(gamemode, mutators));
				}
				else
				{
					fpsent *d = player1;

					if(lastmillis-maptime <= CARDTIME+CARDFADE+CARDFADE)
						fade = amt*(float(lastmillis-maptime-CARDTIME-CARDFADE)/float(CARDFADE));
					else fade *= amt;

					if(player1->state == CS_ALIVE)
					{
						if(damageresidue > 0 || d->state == CS_DEAD)
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
					}

					if(d->state == CS_ALIVE)
					{
						float hlt = d->health/float(MAXHEALTH), glow = min((hlt*0.5f)+0.5f, 1.f), pulse = fade;
						if(lastmillis < d->lastregen+500) pulse *= (lastmillis-d->lastregen)/500.f;
						settexture("textures/barv.png");
						glColor4f(glow, 0.f, 0.f, pulse);

						int rw = oy/5/4, rx = oy/5+FONTH/2, rh = oy/5, ry = oy-rh-(FONTH/4), ro = int(((oy/5)-(oy/30))*hlt), off = rh-ro-(oy/30);
						glBegin(GL_QUADS);
						glTexCoord2f(0, 0); glVertex2i(rx, ry+off);
						glTexCoord2f(1, 0); glVertex2i(rx+rw, ry+off);
						glTexCoord2f(1, 1); glVertex2i(rx+rw, ry+rh);
						glTexCoord2f(0, 1); glVertex2i(rx, ry+rh);
						glEnd();

						if(isgun(d->gunselect) && d->ammo[d->gunselect] > 0)
						{
							bool canshoot = d->canshoot(d->gunselect, lastmillis);
							draw_textx("%d", oy/5+rw+(FONTH/4*3), oy-75, canshoot ? 255 : 128, canshoot ? 255 : 128, canshoot ? 255 : 128, int(255.f*fade), false, AL_LEFT, -1, -1, d->ammo[d->gunselect]);
						}
						else
						{
							draw_textx("Out of ammo", oy/5+rw+(FONTH/4*3), oy-75, 255, 255, 255, int(255.f*fade), false, AL_LEFT);
						}
					}
					else if(d->state == CS_DEAD)
					{
						int wait = respawnwait(d);

						if(wait)
						{
							float c = float(wait)/1000.f;
							draw_textx("Fragged! Down for %.1fs", oy/5+(FONTH/4*3), oy-75, 255, 255, 255, int(255.f*fade), false, AL_LEFT, -1, -1, c);
						}
						else
							draw_textx("Fragged! Press attack to respawn", oy/5+(FONTH/4*3), oy-75, 255, 255, 255, int(255.f*fade), false, AL_LEFT);
					}

					int rx = FONTH/4, rs = oy/5, ry = oy-rs-(FONTH/4);
					settexture("textures/radar.png");
					if(m_team(gamemode, mutators)) glColor4f(0.f, 0.f, 1.f, fade);
					else glColor4f(0.f, 1.0f, 0.f, fade);

					glBegin(GL_QUADS);
					drawradar(float(rx), float(ry), float(rs));
					glEnd();

					if(m_capture(gamemode)) cpc.drawhud(ox, oy);
					else if(m_ctf(gamemode)) ctf.drawhud(ox, oy);
					drawblips(rx, ry, rs);
				}

				drawhudelements(w, h);
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

	void editvar(ident *id, bool local)
	{
        if(id && id->world && local && m_edit(gamemode))
        {
        	switch(id->type)
        	{
        		case ID_VAR:
					cc.addmsg(SV_EDITVAR, "risi", id->type, id->name, *id->storage.i);
					break;
        		case ID_FVAR:
					cc.addmsg(SV_EDITVAR, "risf", id->type, id->name, *id->storage.f);
					break;
        		case ID_SVAR:
					cc.addmsg(SV_EDITVAR, "riss", id->type, id->name, *id->storage.s);
					break;
        		case ID_ALIAS:
					cc.addmsg(SV_EDITVAR, "riss", id->type, id->name, id->action);
					break;
				default: break;
        	}
        }
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
		if(!maptime && lastmillis-maptime <= CARDTIME)
		{
			float fade = maptime ? float(lastmillis-maptime)/float(CARDTIME) : 0.f;
			colour = vec(fade, fade, fade);
			return true;
		}
		return false;
	}

	void fixrange(float &yaw, float &pitch)
	{
		const float MAXPITCH = 89.9f;

		if(pitch > MAXPITCH) pitch = MAXPITCH;
		if(pitch < -MAXPITCH) pitch = -MAXPITCH;
		while(yaw < 0.0f) yaw += 360.0f;
		while(yaw > 360.0f) yaw -= 360.0f;
	}

	void fixview(int w, int h)
	{
		if(fov > MAXFOV) fov = MAXFOV;
		if(fov < MINFOV) fov = MINFOV;
		curfov = float(fov);
		fovy = float(curfov*h)/w;
		aspect = w/float(h);
	}

	void mousemove(int dx, int dy)
	{
		if(!menuactive() && (player1->state != CS_ALIVE && player1->state != CS_DEAD))
		{
			player1->yaw += (dx/SENSF)*(yawsensitivity()/(float)sensitivityscale());
			player1->pitch -= (dy/SENSF)*(pitchsensitivity()/(float)sensitivityscale())*(invmouse() ? -1 : 1);
			fixrange(player1->yaw, player1->pitch);
			cursorx = cursory = 0.5f;
			return;
		}
		cursorx = max(0.0f, min(1.0f, cursorx+(float(dx*mousesensitivity())/10000.f)));
		cursory = max(0.0f, min(1.0f, cursory+(float(dy*mousesensitivity())/10000.f)));
	}

	int lastcamera;
	physent gamecamera;

	void recomputecamera(int w, int h)
	{
		fixview(w, h);

		if(!menuactive())
		{
			if(player1->state != CS_ALIVE && player1->state != CS_DEAD)
			{
				camera1 = player1;
				fixrange(camera1->yaw, camera1->pitch);
				findorientation(camera1->o, camera1->yaw, camera1->pitch, worldpos, true);
				lastcamera = 0;
			}
			else
			{
				camera1 = &gamecamera;

				if(!lastcamera)
				{
					camera1->reset();
					camera1->type = ENT_CAMERA;
					camera1->state = CS_ALIVE;
					camera1->o = vec(player1->o).add(vec(0, 0, player1->height));
					camera1->roll = 0.f;

					cursorx = cursory = 0.5f;

					fixrange(player1->yaw, player1->pitch);
					findorientation(player1->o, player1->yaw, player1->pitch, worldpos, true);

					vec v(worldpos);
					v.sub(camera1->o);
					v.normalize();
					vectoyawpitch(v, camera1->yaw, camera1->pitch);
				}
				else
				{
					int frame = lastcamera-lastmillis;
					float deadzone = (mousedeadzone()/100.f);
					float cx = (cursorx-0.5f), cy = (0.5f-cursory);
					camera1->o = vec(player1->o).add(vec(0, 0, 2));

					if(cx > deadzone || cx < -deadzone)
						camera1->yaw -= ((cx > deadzone ? cx-deadzone : cx+deadzone)/(1.f-deadzone))*frame*mousepanspeed()/100.f;

					if(cy > deadzone || cy < -deadzone)
						camera1->pitch -= ((cy > deadzone ? cy-deadzone : cy+deadzone)/(1.f-deadzone))*frame*mousepanspeed()/100.f;
					camera1->roll = 0.f;
					fixrange(camera1->yaw, camera1->pitch);

					vec dir;
					vecfromyawpitch(camera1->yaw, camera1->pitch, -1, 0, dir);
					camera1->o.add(vec(dir).mul(5));
				}

				if(lastcamera && quakewobble > 0)
				{
					float pc = float(min(quakewobble, 100))/100.f;
					#define wobble (float(rnd(24)-12)*pc)
					camera1->roll = wobble;
				}
				else quakewobble = 0;

				lastcamera = lastmillis;
			}
		}
	}

	void adddynlights()
	{
		pj.adddynlights();
		et.adddynlights();
	}

	bool gamethirdperson()
	{
		return true;
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
		if(s >= 0) playsound(s);
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
