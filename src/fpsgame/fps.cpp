#include "pch.h"
#include "engine.h"
#include "bfa.h"
#include "game.h"
#include "fpsserver.h"

#ifndef STANDALONE
struct GAMECLIENT : igameclient
{
	#include "physics.h"
	#include "projs.h"
	#include "weapon.h"
	#include "scoreboard.h"
	#include "entities.h"
	#include "client.h"
	#include "bot.h"
	#include "stf.h"
    #include "ctf.h"

	int nextmode, nextmuts, gamemode, mutators;
	bool intermission;
	int maptime, minremain;
	int respawnent, swaymillis;
	dynent fpsmodel;
	vec swaydir;
    int lasthit, lastcamera, lastzoom;
    bool prevzoom, zooming;
	int quakewobble, damageresidue;
    int liquidchan;

	struct sline { string s; };
	struct teamscore
	{
		int team, score;
		teamscore() {}
		teamscore(int s, int n) : team(s), score(n) {}
	};

	vector<fpsent *> shplayers;
	vector<teamscore> teamscores;

	fpsent *player1;				// our client
	vector<fpsent *> players;		// other clients
	fpsent lastplayerstate;

	IVARP(cardtime, 0, 2000, 10000);
	IVARP(cardfade, 0, 3000, 10000);

	IVARP(thirdperson, 0, 1, 1);
	IVARP(thirdpersonfov, 90, 120, 150);
	IVARP(thirdpersondist, -100, 1, 100);
	IVARP(thirdpersonshift, -100, 4, 100);
	IVARP(thirdpersonangle, 0, 40, 360);
	IVARP(thirdpersontranslucent, 0, 0, 1);

	IVARP(firstpersonfov, 90, 100, 150);
	IVARP(firstpersondist, -100, 50, 100);
	IVARP(firstpersonshift, -100, 25, 100);
	IVARP(firstpersonsway, 0, 100, INT_MAX-1);
	IVARP(firstpersontranslucent, 0, 0, 1);

	IVARP(invmouse, 0, 0, 1);
	IVARP(absmouse, 0, 1, 1);

	IVARP(mousetype, 0, 0, 2);
	IVARP(mousedeadzone, 0, 5, 100);
	IVARP(mousepanspeed, 1, 30, INT_MAX-1);

	IVARP(zoommousetype, 0, 2, 2);
	IVARP(zoomdeadzone, 0, 30, 100);
	IVARP(zoompanspeed, 1, 10, INT_MAX-1);

	IVARP(editmousetype, 0, 0, 2);
	IVARP(editfov, 1, 120, 360);
	IVARP(editdeadzone, 0, 50, 100);
	IVARP(editpanspeed, 1, 20, INT_MAX-1);

	IVARP(specmousetype, 0, 0, 2);
	IVARP(specfov, 1, 120, 360);
	IVARP(specdeadzone, 0, 5, 100);
	IVARP(specpanspeed, 1, 20, INT_MAX-1);

	IFVARP(sensitivity, 10.0f);
	IFVARP(yawsensitivity, 10.0f);
	IFVARP(pitchsensitivity, 7.5f);
	IFVARP(zoomsensitivity, 1.0f);
	IFVARP(mousesensitivity, 1.0f);

	IVARP(crosshair, 0, 1, 1);
	IVARP(teamcrosshair, 0, 1, 1);
	IVARP(hitcrosshair, 0, 425, 1000);

	IVARP(crosshairsize, 1, 30, 1000);
	IVARP(crosshairblend, 0, 30, 100);
	IVARP(crosshairspeed, 0, 250, INT_MAX-1);

	IVARP(cursorsize, 1, 30, 1000);
	IVARP(cursorblend, 0, 100, 100);

	IVARP(zoomtype, 0, 0, 1);
	IVARP(zoomfov, 20, 20, 150);
	IVARP(zoomtime, 1, 500, 10000);
	IVARP(zoomcrosshairsize, 1, 200, 1000);

	ITVAR(relativecursortex, "textures/relativecursor");
	ITVAR(guicursortex, "textures/guicursor");
	ITVAR(editcursortex, "textures/editcursor");
	ITVAR(crosshairtex, "textures/crosshair");
	ITVAR(teamcrosshairtex, "textures/teamcrosshair");
	ITVAR(hitcrosshairtex, "textures/hitcrosshair");
	ITVAR(zoomcrosshairtex, "textures/zoomcrosshair");

	IVARP(radardist, 0, 512, 512);
	IVARP(radarnames, 0, 1, 2);

	IVARP(editradardist, 0, 512, INT_MAX-1);
	IVARP(editradarentnames, 0, 1, 2);

	ITVAR(bliptex, "textures/blip");
	ITVAR(flagbliptex, "textures/flagblip");
	ITVAR(radartex, "textures/radar");
	ITVAR(radarpingtex, "<anim:75>textures/radarping");
	ITVAR(healthbartex, "<anim>textures/healthbar");
	ITVAR(goalbartex, "<anim>textures/goalbar");
	ITVAR(teambartex, "<anim>textures/teambar");
	ITVAR(indicatortex, "<anim>textures/indicator");

	ITVAR(damagetex, "textures/damage");
	ITVAR(zoomtex, "textures/zoom");

	IVARP(showstats, 0, 1, 1);
	IVARP(showhudents, 0, 10, 100);
	IVARP(showfps, 0, 2, 2);
	IVARP(statrate, 0, 200, 1000);

    GAMECLIENT()
		: ph(*this), pj(*this), ws(*this), sb(*this), et(*this), cc(*this), bot(*this), stf(*this), ctf(*this),
			nextmode(sv->defaultmode()), nextmuts(0), gamemode(sv->defaultmode()), mutators(0), intermission(false),
			maptime(0), minremain(0), respawnent(-1),
			swaymillis(0), swaydir(0, 0, 0),
			lasthit(0), lastcamera(0), lastzoom(0), prevzoom(false), zooming(false),
			quakewobble(0), damageresidue(0),
			liquidchan(-1),
			player1(new fpsent())
	{
        CCOMMAND(kill, "",  (GAMECLIENT *self), { self->suicide(self->player1); });
		CCOMMAND(mode, "ii", (GAMECLIENT *self, int *val, int *mut), { self->setmode(*val, *mut); });
		CCOMMAND(gamemode, "", (GAMECLIENT *self), intret(self->gamemode));
		CCOMMAND(mutators, "", (GAMECLIENT *self), intret(self->mutators));
		CCOMMAND(zoom, "D", (GAMECLIENT *self, int *down), { self->dozoom(*down!=0); });
	}

	iclientcom *getcom() { return &cc; }
	icliententities *getents() { return &et; }

	char *gametitle() { return sv->gamename(gamemode, mutators); }
	char *gametext() { return getmapname(); }

	float radarrange()
	{
		float dist = float(radardist());
		if(editmode) dist = float(editradardist());
		return dist;

	}

	bool isthirdperson()
	{
		return thirdperson() && !editmode && !cc.spectator;
	}

	int mousestyle()
	{
		if(inzoom()) return zoommousetype();
		if(editmode) return editmousetype();
		if(cc.spectator) return specmousetype();
		return mousetype();
	}

	int deadzone()
	{
		if(inzoom()) return zoomdeadzone();
		if(editmode) return editdeadzone();
		if(cc.spectator) return specdeadzone();
		return mousedeadzone();
	}

	int panspeed()
	{
		if(inzoom()) return zoompanspeed();
		if(editmode) return editpanspeed();
		if(cc.spectator) return specpanspeed();
		return mousepanspeed();
	}

	int fov()
	{
		if(editmode) return editfov();
		if(cc.spectator) return specfov();
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
		if(allowmove(player1) && player1->gunselect == GUN_RIFLE) return true;
		zoomset(false, 0);
		return false;
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
			switch(zoomtype())
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

	void addsway(fpsent *d)
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

	int respawnwait(fpsent *d)
	{
		int wait = 0;
		if(m_stf(gamemode)) wait = stf.respawnwait(d);
		else if(m_ctf(gamemode)) wait = ctf.respawnwait(d);
		return wait;
	}

	void respawn(fpsent *d)
	{
		if(d->state == CS_DEAD && !respawnwait(d))
			respawnself(d);
	}

    bool allowmove(physent *d)
    {
        if(d->type == ENT_PLAYER)
        {
			if(d->state == CS_DEAD) return false;
			if(intermission) return false;
        }
        return true;
    }

	void respawnself(fpsent *d)
	{
		d->stopmoving();

		if(d->respawned != d->lifesequence)
		{
			cc.addmsg(SV_TRYSPAWN, "ri", d->clientnum);
			d->respawned = d->lifesequence;
		}
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

	fpsent *pointatplayer()
	{
		loopv(players)
		{
			fpsent *o = players[i];
			if(!o) continue;
			vec pos = ph.headpos(player1, 0.f);
			if(intersect(o, pos, worldpos)) return o;
		}
		return NULL;
	}

	void setmode(int mode, int muts)
	{
		nextmode = mode; nextmuts = muts;
		sv->modecheck(&nextmode, &nextmuts);
	}

	void resetgamestate()
	{
		pj.reset();
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
			ws.update();
			bot.update();

			if(!allowmove(player1)) player1->stopmoving();

			#define adjustscaled(t,n,m) \
				if(n > 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/m)); if(n <= 0) n = (t)0; }

			adjustscaled(float, player1->roll, 100.f);
			adjustscaled(int, quakewobble, 100.f);
			adjustscaled(int, damageresidue, 100.f);

			if(player1->state == CS_DEAD)
			{
				if(lastmillis-player1->lastpain < 2000)
					ph.move(player1, 10, false);
			}
			else if(player1->state == CS_ALIVE)
			{
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
				addsway(player1);
				et.checkitems(player1);
				ws.shoot(player1, worldpos);
				ws.reload(player1);
			}
			else ph.move(player1, 20, true);
		}
		if(player1->clientnum >= 0) c2sinfo();
	}

	void damaged(int gun, int flags, int damage, fpsent *d, fpsent *actor, int millis, vec &dir)
	{
		if(d->state != CS_ALIVE || intermission) return;

		d->dodamage(damage, millis);

		if(actor->type == ENT_PLAYER) actor->totaldamage += damage;

		if(d == player1)
		{
			quakewobble += damage;
			damageresidue += damage;
			d->hitpush(damage, dir, actor, gun);
		}

		if(d->type == ENT_PLAYER)
		{
			vec p = ph.headpos(d);
			p.z += 0.6f*(d->height + d->aboveeye) - d->height;
			particle_splash(3, min(damage/4, 20), 10000, p);
			if(d!=player1)
			{
				s_sprintfd(ds)("@%d", damage);
				particle_text(d->abovehead(), ds, 8);
			}
			playsound(S_PAIN1+rnd(5), &d->o);
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
			playsound(S_DAMAGE1+snd, &actor->o);
			if(actor == player1) lasthit = lastmillis;
		}
		bot.damaged(d, actor, gun, flags, damage, millis, dir);
	}

	void killed(int gun, int flags, int damage, fpsent *d, fpsent *actor)
	{
		if(d->state!=CS_ALIVE || intermission) return;

		static const char *obitnames[] = {
			"ate a bullet from",
			"was filled with buckshot by",
			"was riddled with holes by",
			"was blown to pieces by",
			"was char-grilled by",
			"was pierced by",
#if 0
			"rode the wrong end of a rocket from",
#endif
			""
		};
		string dname, aname, oname;
		int cflags = (d==player1 || actor==player1 ? CON_CENTER : 0)|CON_NORMAL;
		s_strcpy(dname, colorname(d));
		s_strcpy(aname, actor->type!=ENT_INANIMATE ? colorname(actor) : "");
		s_strcpy(oname, gun >= 0 && gun < NUMGUNS ? obitnames[gun] : "was killed by");
        if(d==actor || actor->type==ENT_INANIMATE) console("\f2%s killed themself", cflags, dname);
		else if(actor->type==ENT_AI) console("\f2%s %s %s", cflags, aname, oname, dname);
		else if(m_team(gamemode, mutators) && d->team == actor->team) console("\f2%s %s teammate %s", cflags, dname, oname, aname);
		else console("\f2%s %s %s", cflags, dname, oname, aname);

		d->state = CS_DEAD;
		d->obliterated = !(flags & HIT_BURN) && (damage >= MAXHEALTH || (flags & HIT_EXPLODE));

		if(d == player1)
		{
			sb.showscores(true);
			lastplayerstate = *player1;
			d->stopmoving();
			d->deaths++;
			d->pitch = 0;
			d->roll = 0;
			et.announce(lastmillis-d->lastspawn < 10000 ? S_V_OWNED : S_V_FRAGGED);
		}
		else
		{
            d->move = d->strafe = 0;
		}

		vec pos = ph.headpos(d);
		loopi(rnd((damage+2)/2)+1) pj.spawn(pos, d->vel, d, PRJ_GIBS);
		playsound(S_DIE1+rnd(2), &d->o);

		bot.killed(d, actor, gun, flags, damage);
	}

	void timeupdate(int timeremain)
	{
		minremain = timeremain;
		if(!timeremain)
		{
			if(!intermission)
			{
				player1->stopmoving();
				sb.showscores(true);

				if(m_mission(gamemode))
				{
					et.announce(S_V_MCOMPLETE, "intermission: mission complete!");
				}
				else
				{
					bool won = m_team(gamemode, mutators) ?
						(teamscores.length() && player1->team == teamscores[0].team) :
							(shplayers.length() && shplayers[0] == player1);
					if(won)
					{
						et.announce(S_V_YOUWIN, "intermission: you win!");
					}
					else
					{
						et.announce(S_V_YOULOSE, "intermission: you lose!");
					}
				}
				intermission = true;
			}
		}
		else if(timeremain > 0)
		{
			console("\f2time remaining: %d %s", CON_NORMAL|CON_CENTER, timeremain, timeremain==1 ? "minute" : "minutes");
		}
	}

	fpsent *newclient(int cn)	// ensure valid entity
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
			fpsent *d = new fpsent();
			d->clientnum = cn;
			players[cn] = d;
		}

		return players[cn];
	}

	fpsent *getclient(int cn)	// ensure valid entity
	{
		if(cn == player1->clientnum) return player1;
		if(players.inrange(cn)) return players[cn];
		return NULL;
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

    void preload()
    {
    	loopi(TEAM_MAX)
    	{
			loadmodel(teamtype[i].tpmdl, -1, true);
			loadmodel(teamtype[i].fpmdl, -1, true);
    	}
        ws.preload();
        et.preload();
		stf.preload();
        ctf.preload();
    }

	void startmap(const char *name)	// called just after a map load
	{
		const char *title = getmaptitle();
		if(*title) console("%s", CON_CENTER|CON_NORMAL, title);

		intermission = false;
        player1->respawned = player1->suicided = 0;
		respawnent = -1;
        maptime = lasthit = lastcamera = 0;
		cc.mapstart();
		pj.reset();

		// reset perma-state
		player1->oldnode = player1->lastnode = -1;
		player1->frags = 0;
		player1->deaths = 0;
		player1->totaldamage = 0;
		player1->totalshots = 0;
		loopv(players) if(players[i])
		{
			players[i]->oldnode = players[i]->lastnode = -1;
			players[i]->frags = 0;
			players[i]->deaths = 0;
			players[i]->totaldamage = 0;
			players[i]->totalshots = 0;
		}

		et.findplayerspawn(player1, -1, -1);
		et.resetspawns();
		sb.showscores(false);
	}

	void playsoundc(int n, fpsent *d = NULL)
	{
		if(n < 0 || n >= S_MAX) return;
		fpsent *c = d ? d : player1;
		if(c == player1 || c->bot)
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

	char *colorname(fpsent *d, char *name = NULL, const char *prefix = "", bool team = true, bool dupname = true)
	{
		if(!name) name = d->name;
		static string cname;
		s_sprintf(cname)("%s\fs%s\fS", prefix, name);
		if(!name[0] || d->ownernum >= 0 || (dupname && duplicatename(d, name)))
		{
			s_sprintfd(s)(" [\fs\f5%d\fS]", d->clientnum);
			s_strcat(cname, s);
		}
		if(team && m_team(gamemode, mutators))
		{
			s_sprintfd(s)(" (\fs%s%s\fS)", teamtype[d->team].chat, teamtype[d->team].name);
			s_strcat(cname, s);
		}
		return cname;
	}

	void suicide(fpsent *d)
	{
		if(d == player1 || d->bot)
		{
			if(d->state!=CS_ALIVE) return;
			if(d->suicided!=d->lifesequence)
			{
				cc.addmsg(SV_SUICIDE, "ri", d->clientnum);
				d->suicided = d->lifesequence;
			}
		}
	}

	void drawsized(float x, float y, float s)
	{
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x,	y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x+s, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x+s, y+s);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x,	y+s);
	}


	void drawplayerblip(fpsent *d, int x, int y, int s)
	{
		vec dir = ph.headpos(d);
		dir.sub(camera1->o);
		float dist = dir.magnitude();
		if(dist < radarrange())
		{
			dir.rotate_around_z(-camera1->yaw*RAD);

			int colour = teamtype[d->team].colour;
			float cx = x + s*0.5f*0.95f*(1.0f+dir.x/radarrange()),
				cy = y + s*0.5f*0.95f*(1.0f+dir.y/radarrange()),
				cs = (d->crouching ? 0.005f : 0.025f)*s,
				r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f,
				fade = clamp(1.f-(dist/radarrange()), 0.f, 1.f);

			if(lastmillis-d->lastspawn <= REGENWAIT)
				fade *= clamp(float(lastmillis-d->lastspawn)/float(REGENWAIT), 0.f, 1.f);

			settexture(bliptex());
			glColor4f(r, g, b, fade);
			glBegin(GL_QUADS);
			drawsized(cx-cs*0.5f, cy-cs*0.5f, cs);
			glEnd();
			int ty = int(cy+cs);
			if(radarnames())
				ty += draw_textx("%s", int(cx), ty, 255, 255, 255, int(fade*255.f), false, AL_CENTER, -1, -1, colorname(d, NULL, "", false));
			if(radarnames() == 2 && m_team(gamemode, mutators))
				draw_textx("(\fs%s%s\fS)", int(cx), ty, 255, 255, 255, int(fade*255.f), false, AL_CENTER, -1, -1, teamtype[d->team].chat, teamtype[d->team].name);
		}
	}

	void drawblips(int x, int y, int s)
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

			float cx = x + (s-FONTH)*0.5f*(1.0f+dir.x/radarrange()),
				cy = y + (s-FONTH)*0.5f*(1.0f+dir.y/radarrange());

			draw_textx("%s", int(cx), int(cy), 255, 255, 255, hudblend*255/100, true, AL_LEFT, -1, -1, card);
		}

		loopv(players)
			if(players[i] && players[i]->state == CS_ALIVE)
				drawplayerblip(players[i], x, y, s);

		loopv(et.ents)
		{
			extentity &e = *et.ents[i];
			if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
			enttypes &t = enttype[e.type];
			if((t.usetype == EU_ITEM && e.spawned) || editmode)
			{
				bool insel = editmode && (enthover == i || entgroup.find(i) >= 0);
				vec dir(e.o);
				dir.sub(camera1->o);
				float dist = dir.magnitude();
				if(!insel && dist >= radarrange()) continue;
				if(dist >= radarrange()) dir.mul(radarrange()/dist);
				dir.rotate_around_z(-camera1->yaw*RAD);
				float cx = x + s*0.5f*0.95f*(1.0f+dir.x/radarrange()),
					cy = y + s*0.5f*0.95f*(1.0f+dir.y/radarrange()),
						cs = (insel ? 0.033f : 0.025f)*s,
							fade = clamp(insel ? 1.f : 1.f-(dist/radarrange()), 0.f, 1.f);
				settexture(bliptex());
				glColor4f(1.f, insel ? 0.5f : 1.f, 0.f, fade);
				glBegin(GL_QUADS);
				drawsized(cx-(cs*0.5f), cy-(cs*0.5f), cs);
				glEnd();
				int ty = int(cy+cs);
				if(editradarentnames() == 2 && editmode)
					ty += draw_textx("%s [%d]", int(cx), ty, 255, 255, 255, int(fade*255.f), false, AL_CENTER, -1, -1, t.name, i);
				if(editradarentnames() && insel)
					draw_textx("(%d %d %d %d)", int(cx), ty, 255, 255, 255, int(fade*255.f), false, AL_CENTER, -1, -1, e.attr1, e.attr2, e.attr3, e.attr4);
			}
		}
		popfont();
	}

	int drawtitlecard(int w, int h, int y)
	{
		int ox = w*3, oy = h*3, hoff = y;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);

		int bs = oy/4, bx = ox-bs-FONTH/4, by = FONTH/4, secs = maptime ? lastmillis-maptime : 0;
		float fade = hudblend/100.f, amt = 1.f;

		if(secs < cardtime())
		{
			amt = clamp(float(secs)/float(cardtime()), 0.f, 1.f);
			fade = clamp(amt*fade, 0.f, 1.f);
		}
		else if(secs < cardtime()+cardfade())
			fade = clamp(fade*(1.f-(float(secs-cardtime())/float(cardfade()))), 0.f, 1.f);

		const char *title = getmaptitle();
		if(!*title) title = "Untitled by Unknown";

		glColor4f(1.f, 1.f, 1.f, fade*0.9f);
		if(!rendericon(getmapname(), bx, by, bs, bs))
			rendericon("textures/emblem", bx, by, bs, bs);
		glColor4f(1.f, 1.f, 1.f, fade);
		rendericon("textures/guioverlay", bx, by, bs, bs);

		int ty = by + bs;
		ty = draw_textx("%s", int(ox*amt)-FONTH/4, ty, 255, 255, 255, int(255.f*fade), false, AL_RIGHT, -1, ox-FONTH, sv->gamename(gamemode, mutators));
		ty = draw_textx("%s", int(ox*(1.f-amt))+FONTH/4, ty, 255, 255, 255, int(255.f*fade), false, AL_LEFT, -1, ox-FONTH, title);

		return hoff;
	}

	int drawgamehud(int w, int h, int y)
	{
		Texture *t;
		int ox = w*3, oy = h*3, hoff = y;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);

		int bs = oy/4, bo = bs/16, bx = ox-bs-FONTH/4, by = FONTH/4,
			secs = maptime ? lastmillis-maptime : 0;
		float fade = hudblend/100.f;

		if(secs < cardtime()+cardfade()+cardfade())
		{
			float amt = clamp(float(secs-cardtime()-cardfade())/float(cardfade()), 0.f, 1.f);
			fade = clamp(fade*amt, 0.f, 1.f);
		}

		if(player1->state == CS_ALIVE && inzoom())
		{
			if((t = textureload(damagetex())) != notexture)
			{
				int frame = lastmillis-lastzoom;
				float pc = frame < zoomtime() ? float(frame)/float(zoomtime()) : 1.f;
				if(!zooming) pc = 1.f-pc;
				settexture("textures/zoom");

				glColor4f(1.f, 1.f, 1.f, pc);

				glBegin(GL_QUADS);
				glTexCoord2f(0, 0); glVertex2f(0, 0);
				glTexCoord2f(1, 0); glVertex2f(ox, 0);
				glTexCoord2f(1, 1); glVertex2f(ox, oy);
				glTexCoord2f(0, 1); glVertex2f(0, oy);
				glEnd();
			}
		}

		if((player1->state == CS_ALIVE && damageresidue > 0) || player1->state == CS_DEAD)
		{
			if((t = textureload(damagetex())) != notexture)
			{
				int dam = player1->state == CS_DEAD ? 100 : min(damageresidue, 100);
				float pc = float(dam)/100.f;
				settexture("textures/damage");

				glColor4f(1.f, 1.f, 1.f, pc);

				glBegin(GL_QUADS);
				glTexCoord2f(0, 0); glVertex2f(0, 0);
				glTexCoord2f(1, 0); glVertex2f(ox, 0);
				glTexCoord2f(1, 1); glVertex2f(ox, oy);
				glTexCoord2f(0, 1); glVertex2f(0, oy);
				glEnd();
			}
		}

		int colour = teamtype[player1->team].colour,
			r = (colour>>16), g = ((colour>>8)&0xFF), b = (colour&0xFF);

		settexture(radartex());
		//glColor4f(r/255.f, g/255.f, b/255.f, fade);
		glColor4f(1.f, 1.f, 1.f, fade);
		glBegin(GL_QUADS);
		drawsized(float(bx), float(by), float(bs));
		glEnd();

		drawblips(bx+bo, by+bo, bs-(bo*2));
		if(m_stf(gamemode)) stf.drawblips(ox, oy, bx+bo, by+bo, bs-(bo*2));
		else if(m_ctf(gamemode)) ctf.drawblips(ox, oy, bx+bo, by+bo, bs-(bo*2));

		settexture(radarpingtex());
		glColor4f(1.f, 1.f, 1.f, fade);
		glBegin(GL_QUADS);
		drawsized(float(bx), float(by), float(bs));
		glEnd();

		settexture(goalbartex());
		glColor4f(1.f, 1.f, 1.f, fade);
		glBegin(GL_QUADS);
		drawsized(float(bx), float(by), float(bs));
		glEnd();

		settexture(teambartex());
		glColor4f(r/255.f, g/255.f, b/255.f, fade);
		glBegin(GL_QUADS);
		drawsized(float(bx), float(by), float(bs));
		glEnd();

		if(player1->state == CS_ALIVE)
		{
			if((t = textureload(healthbartex())) != notexture)
			{
				float amt = clamp(float(player1->health)/float(MAXHEALTH), 0.f, 1.f);
				float glow = 1.f, pulse = fade;

				if(lastmillis <= player1->lastregen+500)
				{
					float regen = (lastmillis-player1->lastregen)/500.f;
					pulse = clamp(pulse*regen, 0.3f, max(fade, 0.33f));
					glow = clamp(glow*regen, 0.3f, 1.f);
				}

				glBindTexture(GL_TEXTURE_2D, t->getframe(amt));
				glColor4f(glow, glow*0.3f, 0.f, pulse);
				glBegin(GL_QUADS);
				drawsized(float(bx), float(by), float(bs));
				glEnd();
			}

			if(isgun(player1->gunselect) && player1->ammo[player1->gunselect] > 0)
			{
				bool canshoot = player1->canshoot(player1->gunselect, lastmillis);
				draw_textx("%d", ox-FONTH/4, by+bs, canshoot ? 255 : 128, canshoot ? 255 : 128, canshoot ? 255 : 128, int(255.f*fade), false, AL_RIGHT, -1, -1, player1->ammo[player1->gunselect]);
			}
		}
		else if(player1->state == CS_DEAD)
		{
			int wait = respawnwait(player1);

			if(wait)
				draw_textx("Fragged! Down for %.01fs", FONTH/4, hoff-FONTH, 255, 255, 255, hudblend*255/100, false, AL_LEFT, -1, -1, float(wait)/1000.f);
			else
				draw_textx("Fragged! Press attack to respawn", FONTH/4, hoff-FONTH, 255, 255, 255, hudblend*255/100, false, AL_LEFT);

			hoff -= FONTH;
		}

		return hoff;
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
		POINTER_ZOOM,
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
            case POINTER_ZOOM: return zoomcrosshairtex(); break;
        }
        return NULL;
    }

	void drawpointer(int w, int h, int index, float x, float y, float r, float g, float b)
	{
		Texture *pointer = textureload(getpointer(index));
		if(pointer)
		{
			float chsize = crosshairsize()*w/300.0f, blend = crosshairblend()/100.f;

			glEnable(GL_BLEND);
			if(index == POINTER_GUI)
			{
				chsize = cursorsize()*w/300.0f;
				blend = cursorblend()/100.f;
			}
			else if(index == POINTER_ZOOM)
			{
				chsize = zoomcrosshairsize()*w/300.0f;
				if(inzoom())
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

			float cx = x*w*3.0f - (index != POINTER_GUI ? chsize/2.0f : 0);
			float cy = y*h*3.0f - (index != POINTER_GUI ? chsize/2.0f : 0);
			glBindTexture(GL_TEXTURE_2D, pointer->id);
			glBegin(GL_QUADS);
			glTexCoord2d(0.0, 0.0); glVertex2f(cx, cy);
			glTexCoord2d(1.0, 0.0); glVertex2f(cx + chsize, cy);
			glTexCoord2d(1.0, 1.0); glVertex2f(cx + chsize, cy + chsize);
			glTexCoord2d(0.0, 1.0); glVertex2f(cx, cy + chsize);
			glEnd();


			if(index > POINTER_GUI && player1->state == CS_ALIVE &&
				isgun(player1->gunselect) && player1->ammo[player1->gunselect] > 0)
			{
				Texture *t;
				if(((t = textureload(indicatortex())) != notexture) &&
					guntype[player1->gunselect].power && player1->gunstate[player1->gunselect] == GUNSTATE_POWER)
				{
					float amt = clamp(float(lastmillis-player1->gunlast[player1->gunselect])/float(guntype[player1->gunselect].power), 0.f, 1.f);
					if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					else glBlendFunc(GL_ONE, GL_ONE);
					chsize *= 1.5f;
					cx = x*w*3.0f - (index != POINTER_GUI ? chsize/2.0f : 0);
					cy = y*h*3.0f - (index != POINTER_GUI ? chsize/2.0f : 0);
					glBindTexture(GL_TEXTURE_2D, t->getframe(amt));
					glColor4f(1.f, 1.f, 0.f, blend);
					glBegin(GL_QUADS);
					glTexCoord2d(0.0, 0.0); glVertex2f(cx, cy);
					glTexCoord2d(1.0, 0.0); glVertex2f(cx + chsize, cy);
					glTexCoord2d(1.0, 1.0); glVertex2f(cx + chsize, cy + chsize);
					glTexCoord2d(0.0, 1.0); glVertex2f(cx, cy + chsize);
					glEnd();
				}
			}

			glDisable(GL_BLEND);
		}
	}

	void drawpointers(int w, int h)
	{
		static bool wasingui;
		float r = 1.f, g = 1.f, b = 1.f;
        int index = POINTER_NONE;

		if(g3d_windowhit(true, false)) { index = POINTER_GUI; wasingui = true; }
		else if(wasingui) { index = POINTER_GUI; resetcursor(); wasingui = false; }
        else if(!crosshair() || hidehud || player1->state == CS_DEAD) index = POINTER_NONE;
        else if(editmode) index = POINTER_EDIT;
        else if(inzoom()) index = POINTER_ZOOM;
        else if(lastmillis-lasthit < hitcrosshair()) index = POINTER_HIT;
        else if(m_team(gamemode, mutators) && teamcrosshair())
        {
            vec pos = ph.headpos(player1, 0.f);
            dynent *d = ws.intersectclosest(pos, worldpos, player1);
            if(d && d->type==ENT_PLAYER && ((fpsent *)d)->team == player1->team)
				index = POINTER_TEAM;
			else index = POINTER_HAIR;
        }
        else index = POINTER_HAIR;

		if(index >= POINTER_HAIR)
		{
			if(!player1->canshoot(player1->gunselect, lastmillis)) { r *= 0.5f; g *= 0.5f; b *= 0.5f; }
			else if(r && g && b && !editmode && !m_insta(gamemode, mutators))
			{
				if(player1->health<=25) { r = 1; g = b = 0; }
				else if(player1->health<=50) { r = 1; g = 0.5f; b = 0; }
			}
		}

		glLoadIdentity();
		glOrtho(0, w*3, h*3, 0, -1, 1);

		if(index > POINTER_NONE)
		{
			float curx = index < POINTER_EDIT ? cursorx : aimx,
				cury = index < POINTER_EDIT ? cursory : aimy;
			drawpointer(w, h, index, curx, cury, r, g, b);
		}

		if(index > POINTER_GUI && mousestyle() >= 1)
		{
			float curx = mousestyle() == 1 ? cursorx : 0.5f,
				cury = mousestyle() == 1 ? cursorx : 0.5f;
			drawpointer(w, h, POINTER_RELATIVE, curx, cury, r, g, b);
		}
	}

	int drawhudelements(int w, int h, int y)
	{
		int hoff = y;

		glLoadIdentity();
		glOrtho(0, w*3, h*3, 0, -1, 1);

		renderconsole(w, h);

		static int laststats = 0, prevstats[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, curstats[11] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		int fps, bestdiff, worstdiff;
		getfps(fps, bestdiff, worstdiff);

		if(lastmillis-laststats >= statrate())
		{
			memcpy(prevstats, curstats, sizeof(prevstats));
			laststats = lastmillis - (lastmillis%statrate());
		}

		int nextstats[11] =
		{
			vtris*100/max(wtris, 1),
			vverts*100/max(wverts, 1),
			xtraverts/1024,
			xtravertsva/1024,
			glde,
			gbatches,
			getnumqueries(),
			rplanes,
			fps,
			bestdiff,
			worstdiff
		};

		loopi(11) if(prevstats[i]==curstats[i]) curstats[i] = nextstats[i];

		if(showstats())
		{
			draw_textx("ond:%d va:%d gl:%d(%d) oq:%d lm:%d rp:%d pvs:%d", FONTH/4, hoff-FONTH, 255, 255, 255, hudblend*255/100, false, AL_LEFT, -1, -1, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6], lightmaps.length(), curstats[7], getnumviewcells());
			hoff -= FONTH;
			draw_textx("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", FONTH/4, hoff-FONTH, 255, 255, 255, hudblend*255/100, false, AL_LEFT, -1, -1, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
			hoff -= FONTH;
		}

		switch(showfps())
		{
			case 2: draw_textx("fps:%d +%d-%d", FONTH/4, hoff-FONTH, 255, 255, 255, hudblend*255/100, false, AL_LEFT, -1, -1, curstats[8], curstats[9], curstats[10]); hoff -= FONTH; break;
			case 1: draw_textx("fps:%d", FONTH/4, hoff-FONTH, 255, 255, 255, hudblend*255/100, false, AL_LEFT, -1, -1, curstats[8]); hoff -= FONTH; break;
			default: break;
		}

		if(getcurcommand())
		{
			rendercommand(FONTH/4, hoff-FONTH, h*3-FONTH);
			hoff -= FONTH;
		}

		if(cc.ready() && maptime)
		{
			if(editmode)
			{
				draw_textx("sel:%d,%d,%d %d,%d,%d (%d,%d,%d,%d)", FONTH/4, hoff-FONTH, 255, 255, 255, hudblend*255/100, false, AL_LEFT, -1, -1,
						sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z,
							sel.cx, sel.cxs, sel.cy, sel.cys);
				hoff -= FONTH;
				draw_textx("corner:%d orient:%d grid:%d", FONTH/4, hoff-FONTH, 255, 255, 255, hudblend*255/100, false, AL_LEFT, -1, -1,
								sel.corner, sel.orient, sel.grid);
				hoff -= FONTH;
				draw_textx("cube:%s%d ents:%d", FONTH/4, hoff-FONTH, 255, 255, 255, hudblend*255/100, false, AL_LEFT, -1, -1,
					selchildcount<0 ? "1/" : "", abs(selchildcount), entgroup.length());
				hoff -= FONTH;

				#define enthudtext(n, r, g, b) \
					if(et.ents.inrange(n)) \
					{ \
						fpsentity &f = (fpsentity &)*et.ents[n]; \
						draw_textx("entity:%d %s (%d %d %d %d)", FONTH/4, hoff-FONTH, r, g, b, hudblend*255/100, false, AL_LEFT, -1, -1, \
							n, et.findname(f.type), f.attr1, f.attr2, f.attr3, f.attr4); \
						hoff -= FONTH; \
					}

				enthudtext(enthover, 255, 255, 196);
				loopi(clamp(entgroup.length(), 0, showhudents()))
					enthudtext(entgroup[i], 196, 196, 196);
			}

			render_texture_panel(w, h);
		}
		return hoff;
	}

	void drawhud(int w, int h)
	{
		if(!hidehud)
		{
			int hoff = h*3;

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if(maptime && cc.ready())
			{
				pushfont("hud");
				if(lastmillis-maptime < cardtime()+cardfade())
					hoff = drawtitlecard(w, h, hoff);
				else hoff = drawgamehud(w, h, hoff);
				popfont();
			}

			hoff = drawhudelements(w, h, hoff);
			glDisable(GL_BLEND);
			drawpointers(w, h);
		}
	}

	void lighteffects(dynent *e, vec &color, vec &dir)
	{
	}

    void particletrack(physent *owner, vec &o, vec &d)
    {
        if(owner->type != ENT_PLAYER) return;
        float dist = o.dist(d);
        vecfromyawpitch(owner->yaw, owner->pitch, 1, 0, d);
        vec pos = ph.headpos(owner);
        float newdist = raycube(pos, d, dist, RAY_CLIPMAT|RAY_POLY);
        d.mul(min(newdist, dist)).add(pos);
        o = ws.gunorigin(GUN_PISTOL, owner->o, d, (fpsent *)owner);
    }

	void newmap(int size)
	{
		cc.addmsg(SV_NEWMAP, "ri", size);
	}

	void editvar(ident *id, bool local)
	{
        if(id && id->flags&IDF_WORLD && local && m_edit(gamemode))
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
		if(!maptime || lastmillis-maptime < cardtime())
		{
			float fade = maptime ? float(lastmillis-maptime)/float(cardtime()) : 0.f;
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
		while(yaw >= 360.0f) yaw -= 360.0f;
	}

	void fixview(int w, int h)
	{
		curfov = float(fov());

		if(inzoom())
		{
			int frame = lastmillis-lastzoom;
			float diff = float(fov()-zoomfov()),
				amt = frame < zoomtime() ? clamp(float(frame)/float(zoomtime()), 0.f, 1.f) : 1.f;
			if(!zooming) amt = 1.f-amt;
			curfov -= amt*diff;
		}

        aspect = w/float(h);
        fovy = 2*atan2(tan(curfov/2*RAD), aspect)/RAD;
	}

	bool mousemove(int dx, int dy, int x, int y, int w, int h)
	{
		bool windowhit = g3d_windowhit(true, false);

		#define mousesens(a,b,c) ((float(a)/float(b))*c)

		if(windowhit || mousestyle() >= 1)
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
				cursory = clamp(cursory+mousesens(dy, h, mousesensitivity()*(!windowhit && invmouse() ? -1.f : 1.f)), 0.f, 1.f);
				return true;
			}
		}
		else
		{
			if(allowmove(player1))
			{
				float scale = inzoom() ? zoomsensitivity() : sensitivity();
				player1->yaw += mousesens(dx, w, yawsensitivity()*scale);
				player1->pitch -= mousesens(dy, h, pitchsensitivity()*scale*(!windowhit && invmouse() ? -1.f : 1.f));
				fixrange(player1->yaw, player1->pitch);
			}
			return true;
		}
		return false;
	}

	void project(int w, int h)
	{
		if(!g3d_windowhit(true, false))
		{
			if(mousestyle() <= 1 && crosshairspeed())
			{
				float cx, cy, cz;
				vectocursor(worldpos, cx, cy, cz);
				float ax = float(cx)/float(w), ay = float(cy)/float(h),
					amt = float(curtime)/float(crosshairspeed()), offx = ax-aimx, offy = ay-aimy;
				aimx += offx*amt;
				aimy += offy*amt;
			}
			else
			{
				aimx = cursorx;
				aimy = cursory;
			}
			vecfromcursor(aimx, aimy, 1.f, cursordir);
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
			if(!lastcamera)
			{
				resetcursor();
				if(mousestyle() == 2)
				{
					camera1->yaw = player1->aimyaw = player1->yaw;
					camera1->pitch = player1->aimpitch = player1->pitch;
				}
			}

			vec pos = ph.headpos(player1, 0.f);

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
					if(!crosshairspeed() && isthirdperson() && !inzoom())
					{ // if they don't like it they just suffer a bit of jerking
						vec dir(worldpos);
						dir.sub(camera1->o);
						dir.normalize();
						vectoyawpitch(dir, camera1->yaw, camera1->pitch);
						fixrange(camera1->yaw, camera1->pitch);
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
					findorientation(isthirdperson() && !inzoom() ? camera1->o : pos, yaw, pitch, worldpos);
					if(allowmove(player1))
					{
						if(isthirdperson() && !inzoom())
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

			fixrange(camera1->yaw, camera1->pitch);
			fixrange(camera1->aimyaw, camera1->aimpitch);

			if(quakewobble > 0)
				camera1->roll = float(rnd(25)-12)*(float(min(quakewobble, 100))/100.f);
			else camera1->roll = 0;

			if(!isthirdperson() && (firstpersondist() || firstpersonshift()))
			{
				float yaw = camera1->aimyaw, pitch = camera1->aimpitch;
				if(mousestyle() == 2)
				{
					yaw = player1->yaw;
					pitch = player1->pitch;
				}
				if(firstpersondist())
				{
					vec dir;
					vecfromyawpitch(yaw, pitch, 1, 0, dir);
					dir.mul(player1->radius*firstpersondist()/100.f);
					camera1->o.add(dir);
				}
				if(firstpersonshift())
				{
					vec dir;
					vecfromyawpitch(yaw, pitch, 0, 1, dir);
					dir.mul(player1->radius*firstpersonshift()/100.f);
					camera1->o.add(dir);
				}
			}

			if(inzoom())
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

				if(lastcamera && mousestyle() >= 1 && !g3d_windowhit(true, false))
				{
					physent *d = mousestyle() != 2 ? player1 : camera1;
					float amt = clamp(float(lastmillis-lastcamera)/100.f, 0.f, 1.f)*panspeed();
					float zone = float(deadzone())/100.f, cx = cursorx-0.5f, cy = 0.5f-cursory;
					if(cx > zone || cx < -zone) d->yaw += ((cx > zone ? cx-zone : cx+zone)/(1.f-zone))*amt;
					if(cy > zone || cy < -zone) d->pitch += ((cy > zone ? cy-zone : cy+zone)/(1.f-zone))*amt;
					fixrange(d->yaw, d->pitch);
				}
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
						liquidchan = playsound(S_UNDERWATER, &camera1->o, 255, 0, 0, SND_LOOP|SND_NOATTEN|SND_NODELAY);
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

	vector<fpsent *> bestplayers;
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

	void renderclient(fpsent *d, bool third, bool trans, int team, modelattach *attachments, int animflags, int animdelay, int lastaction, float speed)
	{
		string mdl;
		if(third) s_strcpy(mdl, teamtype[team].tpmdl);
		else s_strcpy(mdl, teamtype[team].fpmdl);

		int anim = ANIM_IDLE|ANIM_LOOP;
		float yaw = d->yaw, pitch = d->pitch, roll = d->roll;
		vec o = vec(third ? vec(d->o).sub(vec(0, 0, d->height)) : ph.headpos(d));
		if(!third && firstpersonsway())
		{
			vec sway;
			vecfromyawpitch(d->yaw, d->pitch, 1, 0, sway);
			float swayspeed = sqrtf(d->vel.x*d->vel.x + d->vel.y*d->vel.y);
			swayspeed = min(4.0f, swayspeed);
			sway.mul(swayspeed);
			float swayxy = sinf(swaymillis/115.0f)/float(firstpersonsway()),
				  swayz = cosf(swaymillis/115.0f)/float(firstpersonsway());
			swap(sway.x, sway.y);
			sway.x *= -swayxy;
			sway.y *= swayxy;
			sway.z = -fabs(swayspeed*swayz);
			sway.add(swaydir);
			o.add(sway);
		}
		int basetime = 0;
		if(animoverride()) anim = (animoverride()<0 ? ANIM_ALL : animoverride())|ANIM_LOOP;
		else
		{
			anim = animflags;
			basetime = lastaction;

			if(d->inliquid && d->physstate<=PHYS_FALL)
				anim |= (((allowmove(d) && (d->move || d->strafe)) || d->vel.z+d->falling.z>0 ? ANIM_SWIM : ANIM_SINK)|ANIM_LOOP)<<ANIM_SECONDARY;
			else if(d->timeinair > 100)
				anim |= (ANIM_JUMP|ANIM_END)<<ANIM_SECONDARY;
			else if(allowmove(d))
			{
				if(d->crouching)
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

			if((anim&ANIM_INDEX)==ANIM_IDLE && (anim>>ANIM_SECONDARY)&ANIM_INDEX)
				anim >>= ANIM_SECONDARY;
		}

		if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX))
			anim |= (ANIM_IDLE|ANIM_LOOP)<<ANIM_SECONDARY;

		int flags = MDL_LIGHT;
		if(d->type==ENT_PLAYER) flags |= MDL_FULLBRIGHT;
		else flags |= MDL_CULL_DIST | MDL_CULL_VFC | MDL_CULL_OCCLUDED | MDL_CULL_QUERY;
		if(trans) flags |= MDL_TRANSLUCENT;
		else if(third && (anim&ANIM_INDEX)!=ANIM_DEAD) flags |= MDL_DYNSHADOW;
		dynent *e = third ? (dynent *)d : (dynent *)&fpsmodel;
		rendermodel(NULL, mdl, anim, o, !third && testanims() && d == player1 ? 0 : yaw+90, pitch, roll, flags, e, attachments, basetime, speed);
	}

	void renderplayer(fpsent *d, bool third, bool trans)
	{
        modelattach a[4] = { { NULL }, { NULL }, { NULL }, { NULL } };
		int ai = 0, team = m_team(gamemode, mutators) ? d->team : TEAM_NEUTRAL,
			gun = d->gunselect, lastaction = lastmillis,
			animflags = ANIM_IDLE|ANIM_LOOP, animdelay = 0;

		s_sprintf(d->info)("%s", colorname(d, NULL, "@"));

		if(d->state == CS_DEAD)
		{
			if(d->obliterated) return;
			animflags = ANIM_DYING;
			lastaction = d->lastpain;
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
			lastaction = d->lastpain;
			animflags = ANIM_PAIN;
			animdelay = 300;
		}
		else
		{
			bool showgun = isgun(gun);
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
							if(isgun(d->lastgun) && (d->ammo[gun] > 0 || guntype[gun].rdelay > 0))
								gun = d->lastgun;
							else showgun = false;
						}
						break;
					}
					case GUNSTATE_POWER:
					{
						if(!guntype[gun].power) gunstate = GUNSTATE_SHOOT;
						break;
					}
					case GUNSTATE_SHOOT:
					{
						if(guntype[gun].power) showgun = false;
						break;
					}
					case GUNSTATE_RELOAD:
					{
						if(guntype[gun].power) showgun = false;
						break;
					}
					case GUNSTATE_IDLE:	default:
					{
						if(d->ammo[gun] <= 0 && guntype[gun].rdelay <= 0)
							showgun = false;
						break;
					}
				}
				animflags = guntype[gun].anim + gunstate;
				if(gunstate == GUNSTATE_IDLE) animflags |= ANIM_LOOP;

				if(showgun)
				{ // we could probably animate the vwep too now..
					a[ai].name = guntype[gun].vwep;
					a[ai].tag = "tag_weapon";
					a[ai].anim = ANIM_VWEP|ANIM_LOOP;
					a[ai].basetime = 0;
					ai++;
				}
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
		}

		if(third && d != player1 && d->state != CS_DEAD && d->state != CS_SPECTATOR)
			part_text(d->abovehead(), d->info, 10, 1, 0xFFFFFF);

        renderclient(d, third, trans, team, a[0].name ? a : NULL, animflags, animdelay, lastaction, 0.f);
	}

	IVARP(lasersight, 0, 0, 1);

	void render()
	{
		if(intermission)
		{
			if(m_team(gamemode, mutators)) { bestteams.setsize(0); sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); sb.bestplayers(bestplayers); }
		}

		startmodelbatches();

		fpsent *d;
        loopi(numdynents()) if((d = (fpsent *)iterdynents(i)) && (!rendernormally || d != player1 || (isthirdperson() && !inzoomswitch())))
			if(d->state!=CS_SPECTATOR && d->state!=CS_SPAWNING && (d->state!=CS_DEAD || !d->obliterated))
				renderplayer(d, true, (d->state == CS_LAGGED || (d->state == CS_ALIVE && lastmillis-d->lastspawn <= REGENWAIT) || (d == player1 && thirdpersontranslucent())));

		if(player1->state == CS_ALIVE && !isthirdperson() && !inzoomswitch())
			renderplayer(player1, false, (player1->state == CS_LAGGED || (player1->state == CS_ALIVE && lastmillis-player1->lastspawn <= REGENWAIT) || firstpersontranslucent()));

		et.render();
		pj.render();
		if(m_stf(gamemode)) stf.render();
        else if(m_ctf(gamemode)) ctf.render();
        bot.render();

		endmodelbatches();

		if(lasersight() && rendernormally)
		{
			renderprimitive(true);
			vec v(vec(ws.gunorigin(player1->gunselect, player1->o, worldpos, player1)).add(vec(0, 0, 1)));
			renderline(v, worldpos, 0.2f, 0.0f, 0.0f, false);
			renderprimitive(false);
		}
	}
};
REGISTERGAME(GAMENAME, GAMEID, new GAMECLIENT(), new GAMESERVER());
#else
REGISTERGAME(GAMENAME, GAMEID, NULL, new GAMESERVER());
#endif
