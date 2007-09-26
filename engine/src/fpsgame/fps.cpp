#include "pch.h"

#ifdef BFRONTIER
#ifdef STANDALONE
#include "minimal.h"
#else
#include "engine.h"
#endif
#else
#include "cube.h"

#include "iengine.h"
#include "igame.h"
#endif

#include "game.h"

#include "fpsserver.h"

#ifndef STANDALONE

struct fpsclient : igameclient
{
	// these define classes local to fpsclient
	#include "weapon.h"
	#include "monster.h"
	#include "scoreboard.h"
	#include "fpsrender.h"
	#include "entities.h"
	#include "client.h"
	#include "capture.h"

	int nextmode, gamemode;		 // nextmode becomes gamemode after next map load
	bool intermission;
	int lastmillis;
	string clientmap;
	int maptime, minremain;
	int respawnent;
	int swaymillis;
	vec swaydir;
	int suicided;

#ifdef BFRONTIER
	#include "physics.h"
	physics ph;

	#include "bot.h"
	botcom bc;

	string cptext;
	int cameranum, cameracycled, camerawobble, myrankv, myranks, servermod;
	
	struct sline { string s; };
	struct teamscore
	{
		char *team;
		int score;
		teamscore() {}
		teamscore(char *s, int n) : team(s), score(n) {}
	};

	vector<fpsent *> shplayers;
	vector<teamscore> teamscores;
	
	IVARP(capturespawn, 0, 1, 1);			// auto respawn in capture games
	IVAR(cameracycle, 0, 0, 600);			// cycle camera every N secs
	IVARP(crosshair, 0, 1, 1);				// show the crosshair
	IVARP(damagewobble, 0, 1, 1);			// wobble camera when damaged
	IVARP(hudstyle, 0, HD_RIGHT, HD_MAX-1);	// use new or old hud style
	IVARP(rankhud, 0, 0, 1);				// show ranks on the hud
	IVARP(ranktime, 0, 15000, 600000);		// display unchanged rank no earlier than every N ms
#else
    int following;
    IVARP(followdist, 10, 50, 1000);
    IVARP(followorient, 0, 1, 1);
#endif

	fpsent *player1;				// our client
	vector<fpsent *> players;		// other clients
	fpsent lastplayerstate;

	weaponstate ws;
	monsterset  ms;
	scoreboard  sb;
	fpsrender	fr;
	entities	et;
	clientcom	cc;
	captureclient cpc;

	fpsclient()
		: nextmode(0), gamemode(0), intermission(false), lastmillis(0),
		  maptime(0), minremain(0), respawnent(-1), 
		  swaymillis(0), swaydir(0, 0, 0),
		  suicided(-1),
#ifdef BFRONTIER
		  ph(*this), bc(*this), cameranum(0), cameracycled(0), myrankv(0), myranks(0), servermod(-1),
#else
          following(-1),
#endif
		  player1(spawnstate(new fpsent())),
		  ws(*this), ms(*this), sb(*this), fr(*this), et(*this), cc(*this), cpc(*this)
	{
		CCOMMAND(fpsclient, mode, "s", { self->setmode(atoi(args[0])); });
		CCOMMAND(fpsclient, kill, "",  { self->suicide(self->player1); });
		CCOMMAND(fpsclient, taunt, "", { self->taunt(); });
#ifdef BFRONTIER
		CCOMMAND(fpsclient, cameradir, "ii", self->cameradir(atoi(args[0]), atoi(args[1]) ? true : false));
		CCOMMAND(fpsclient, centerrank, "", self->setcrank());
		CCOMMAND(fpsclient, gotocamera, "i", self->setcamera(atoi(args[0])));

		CCOMMAND(fpsclient, getgamemode, "", intret(self->gamemode));
#else
		CCOMMAND(fpsclient, follow, "s", { self->follow(args[0]); });
#endif
	}

	iclientcom *getcom() { return &cc; }
#ifdef BFRONTIER
	ibotcom *getbot() { return &bc; }
	iphysics *getphysics() { return &ph; }
#endif
	icliententities *getents() { return &et; }

	void setmode(int mode)
	{
		if(multiplayer(false) && !m_mp(mode)) { conoutf("mode %d not supported in multiplayer", mode); return; }
		nextmode = mode;
	}

	void taunt()
	{
		if(player1->state!=CS_ALIVE) return;
		if(lastmillis-player1->lasttaunt<1000) return;
		player1->lasttaunt = lastmillis;
		cc.addmsg(SV_TAUNT, "r");
	}

#ifndef BFRONTIER
	void follow(char *arg)
    {
        if(player1->state!=CS_SPECTATOR && arg[0]) return;
        following = arg[0] ? cc.parseplayer(arg) : -1;
        conoutf("follow %s", following>=0 ? "on" : "off");
	}
#endif

	char *getclientmap() { return clientmap; }

    void rendergame() { fr.rendergame(gamemode); }

	void resetgamestate()
	{
		if(m_classicsp) 
		{
			ms.monsterclear(gamemode);				 // all monsters back at their spawns for editing
			resettriggers();
		}
		ws.projreset();
	}

	fpsent *spawnstate(fpsent *d)			  // reset player state not persistent accross spawns
	{
		d->respawn();
		d->spawnstate(gamemode);
#ifdef BFRONTIER
		playsound(S_RESPAWN, d==player1 ? NULL : &d->o);
#endif
		return d;
	}

	void respawnself()
	{
        if(m_mp(gamemode)) cc.addmsg(SV_TRYSPAWN, "r");
		else
		{
			spawnplayer(player1);
			sb.showscores(false);
		}
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

#ifdef BFRONTIER
    void setupcamera() { return; }
#else
    void followplayer(fpsent *target)
    {
		if(followorient() && target->state!=CS_DEAD) interpolateorientation(target, player1->yaw, player1->pitch);

        physent followcam;
        followcam.o = target->o;
        followcam.yaw = player1->yaw;
        followcam.pitch = player1->pitch;
        followcam.type = ENT_CAMERA;
        followcam.move = -1;
        followcam.eyeheight = 2;
        loopi(10)
        {
            if(!moveplayer(&followcam, 10, true, followdist())) break;
        }

        player1->o = followcam.o;
    }

    void stopfollowing()
    {
        if(following<0) return;
        following = -1;
        conoutf("follow off");
    }

    void setupcamera()
    {
        if(player1->state!=CS_SPECTATOR || following<0) return;
        fpsent *target = getclient(following);
        if(!target || target->state!=CS_ALIVE) return;
        followplayer(target);
    }
#endif

	void otherplayers()
	{
		loopv(players) if(players[i])
		{
            const int lagtime = lastmillis-players[i]->lastupdate;
			if(lagtime>1000 && players[i]->state==CS_ALIVE)
			{
				players[i]->state = CS_LAGGED;
				continue;
			}
            if(lagtime && (players[i]->state==CS_ALIVE || (players[i]->state==CS_DEAD && lastmillis-players[i]->lastpain<2000)) && !intermission) moveplayer(players[i], 2, false);   // use physics to extrapolate player position
#ifndef BFRONTIER
            if(player1->state==CS_SPECTATOR && following>=0 && following==players[i]->clientnum) followplayer(players[i]);
#endif
		}
	}

	void updateworld(vec &pos, int curtime, int lm)		// main game update loop
	{
		lastmillis = lm;
		if(!curtime) return;
		physicsframe();
#ifdef BFRONTIER
		if (g_bf)
		{
			if (player1->state == CS_ALIVE && !intermission)
			{
				if (player1->health < player1->maxhealth && player1->nexthealth <= lastmillis)
				{
					int na = lastmillis-player1->nexthealth, 
						nb = na%300, inc = ((na-nb)+300)/300;
					player1->health = min(player1->health + inc, player1->maxhealth);
					player1->nexthealth = lastmillis + 300 - nb;
				}
				
				if (player1->timeinair)
				{
					if (player1->jumpnext && lastmillis-player1->lastimpulse > 3000)
					{
						vec dir;
						vecfromyawpitch(player1->yaw, player1->pitch, 1, player1->strafe, dir);
						dir.normalize();
						dir.mul(ph.jumpvel(player1));
						player1->vel.add(dir);
						player1->lastimpulse = lastmillis;
						player1->jumpnext = false;
					}
				}
				else player1->lastimpulse = 0;
			}
		}
#endif
		et.checkquad(curtime, player1);
		ws.moveprojectiles(curtime);
		if(player1->clientnum>=0 && player1->state==CS_ALIVE) ws.shoot(player1, pos); // only shoot when connected to server
		ws.bounceupdate(curtime); // need to do this after the player shoots so grenades don't end up inside player's BB next frame
		gets2c();			// do this first, so we have most accurate information when our player moves
		otherplayers();
		ms.monsterthink(curtime, gamemode);
		if(player1->state==CS_DEAD)
		{
			if(lastmillis-player1->lastpain<2000)
			{
				player1->move = player1->strafe = 0;
				moveplayer(player1, 10, false);
			}
#ifdef BFRONTIER
			else if(!intermission && player1->state == CS_DEAD)
			{
				int last = lastmillis-player1->lastpain;
				
				if(m_capture && capturespawn() && last >= cpc.RESPAWNSECS*1000)
				{
					respawnself();
				}
			}
#endif
		}
		else if(!intermission)
		{
			moveplayer(player1, 20, true);
			if(player1->physstate>=PHYS_SLOPE) swaymillis += curtime;
			float k = pow(0.7f, curtime/10.0f);
			swaydir.mul(k); 
#ifdef BFRONTIER
			ph.updatewater(player1, 0);
			swaydir.add(vec(player1->vel).mul((1-k)/(15*max(player1->vel.magnitude(), ph.speed(player1)))));
#else
			swaydir.add(vec(player1->vel).mul((1-k)/(15*max(player1->vel.magnitude(), player1->maxspeed))));
#endif
			et.checkitems(player1);
			if(m_classicsp) checktriggers();
		}
		if(player1->clientnum>=0) c2sinfo(player1);	// do this last, to reduce the effective frame lag
	}

	void spawnplayer(fpsent *d)	// place at random spawn. also used by monsters!
	{
		findplayerspawn(d, m_capture ? cpc.pickspawn(d->team) : (respawnent>=0 ? respawnent : -1));
		spawnstate(d);
		d->state = cc.spectator ? CS_SPECTATOR : (d==player1 && editmode ? CS_EDITING : CS_ALIVE);
	}

	void respawn()
	{
		if(player1->state==CS_DEAD)
		{
			player1->attacking = false;
			if(m_capture && lastmillis-player1->lastpain<cpc.RESPAWNSECS*1000)
			{
                int wait = cpc.RESPAWNSECS-(lastmillis-player1->lastpain)/1000;
#ifdef BFRONTIER
                console("\f2you must wait %d second%s before respawn!", CON_LEFT|CON_CENTER, wait, wait!=1 ? "s" : "");
#else
                conoutf("\f2you must wait %d second%s before respawn!", wait, wait!=1 ? "s" : "");
#endif
				return;
			}
#ifdef BFRONTIER
			if(m_arena) { console("\f2waiting for new round to start...", CON_LEFT|CON_CENTER); return; }
#else
            if(m_arena) { conoutf("\f2waiting for new round to start..."); return; }
			if(m_dmsp) { nextmode = gamemode; cc.changemap(clientmap); return; }	// if we die in SP we try the same map again
#endif
			if(m_classicsp)
			{
				respawnself();
#ifdef BFRONTIER
				console("\f2You wasted another life! The monsters stole your armour and some ammo...", CON_LEFT|CON_CENTER);
#else
                conoutf("\f2You wasted another life! The monsters stole your armour and some ammo...");
#endif
				loopi(NUMGUNS) if(i!=GUN_PISTOL && (player1->ammo[i] = lastplayerstate.ammo[i])>5) player1->ammo[i] = max(player1->ammo[i]/3, 5); 
				return;
			}
			respawnself();
		}
	}

	// inputs

	void doattack(bool on)
	{
		if(intermission) return;
        if(player1->attacking = on) respawn();
	}

	bool canjump() 
	{ 
        if(!intermission) respawn(); 
		return player1->state!=CS_DEAD && !intermission; 
	}

#ifdef BFRONTIER
	void damaged(int damage, fpsent *d, fpsent *actor)
	{
		if(d->state!=CS_ALIVE || intermission) return;
		
		d->dodamage(damage);

		if (d == player1)
		{
			if (g_bf) d->nexthealth = lastmillis + 3000;
			if (damagewobble()) camerawobble = max(camerawobble, damage);
			d->damageroll(damage);
			playsound(S_PAIN6);
		}
		else
		{
			playsound(S_PAIN1+rnd(5), &d->o);
		}
		ws.damageeffect(damage, d);

		if(bc.isbot(d)) bc.damaged(damage, d, actor);
	}
#else
	void damaged(int damage, fpsent *d, fpsent *actor, bool local = true)
	{
		if(d->state!=CS_ALIVE || intermission) return;

		if(local) damage = d->dodamage(damage);
		else if(actor==player1) return;
		if(d==player1)
		{
			damageblend(damage);
			d->damageroll(damage);
		}
		ws.damageeffect(damage, d);

		if(d->health<=0) { if(local) killed(d, actor); }
		else if(d==player1) playsound(S_PAIN6);
		else playsound(S_PAIN1+rnd(5), &d->o);
	}
#endif

	void killed(fpsent *d, fpsent *actor)
	{
		if(d->state!=CS_ALIVE || intermission) return;

		string dname, aname;
		s_strcpy(dname, d==player1 ? "you" : colorname(d));
		s_strcpy(aname, actor==player1 ? "you" : colorname(actor));
#ifdef BFRONTIER
		int cflags = (d==player1 || actor==player1 ? CON_CENTER : 0)|CON_RIGHT;
		if(actor->type==ENT_AI)
			console("\f2%s got killed by %s!", cflags, dname, aname);
		else if(d==actor)
			console("\f2%s suicided%s", cflags, dname, d==player1 ? "!" : "");
		else if(isteam(d->team, actor->team))
		{
			if(d==player1) console("\f2you got fragged by a teammate (%s)", cflags, aname);
			else console("\f2%s fragged a teammate (%s)", cflags, aname, dname);
		}
		else 
		{
			if(d==player1) console("\f2you got fragged by %s", cflags, aname);
			else console("\f2%s fragged %s", cflags, aname, dname);
		}
#else
        if(actor->type==ENT_AI)
            conoutf("\f2%s got killed by %s!", dname, aname);
        else if(d==actor)
            conoutf("\f2%s suicided%s", dname, d==player1 ? "!" : "");
        else if(isteam(d->team, actor->team))
        {
            if(d==player1) conoutf("\f2you got fragged by a teammate (%s)", aname);
            else conoutf("\f2%s fragged a teammate (%s)", aname, dname);
        }
        else 
        {
            if(d==player1) conoutf("\f2you got fragged by %s", aname);
            else conoutf("\f2%s fragged %s", aname, dname);
        }
#endif
		d->state = CS_DEAD;
		d->lastpain = lastmillis;
        d->superdamage = max(-d->health, 0);
		if(d==player1)
		{
			sb.showscores(true);
			lastplayerstate = *player1;
			d->attacking = false;
			d->deaths++;
			d->pitch = 0;
			d->roll = 0;
			playsound(S_DIE1+rnd(2));
		}
		else
		{
            d->move = d->strafe = 0;
			playsound(S_DIE1+rnd(2), &d->o);
			ws.superdamageeffect(d->vel, d);
		}
#ifdef BFRONTIER
		if(bc.isbot(d)) bc.killed(d);
	
		if (d == player1)
			playsound(lastmillis-d->lastspawn < 10000 ? S_V_OWNED : S_V_FRAGGED);
			
		actor->spree++;
		
		switch (actor->spree)
		{
			case 5:  playsound(S_V_SPREE1, actor != player1 ? &actor->o : NULL); break;
			case 10: playsound(S_V_SPREE2, actor != player1 ? &actor->o : NULL); break;
			case 25: playsound(S_V_SPREE3, actor != player1 ? &actor->o : NULL); break;
			case 50: playsound(S_V_SPREE4, actor != player1 ? &actor->o : NULL); break;
			default: if (actor == player1 && d != player1) playsound(S_DAMAGE8); break;
		}
	
		bc.frags(actor, d==actor || isteam(actor->team, d->team) ? -1 : 1);
#endif
	}

	void timeupdate(int timeremain)
	{
		minremain = timeremain;
		if(!timeremain)
		{
			intermission = true;
			player1->attacking = false;
#ifdef BFRONTIER
			if (m_mp(gamemode))
			{
				calcranks();
		
				if ((m_teammode && teamscores.length() && isteam(player1->team, teamscores[0].team)) ||
					(!m_teammode && shplayers.length() && shplayers[0] == player1))
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
#else
			conoutf("\f2intermission:");
			conoutf("\f2game has ended!");
			conoutf("\f2player frags: %d, deaths: %d", player1->frags, player1->deaths);
			int accuracy = player1->totaldamage*100/max(player1->totalshots, 1);
			conoutf("\f2player total damage dealt: %d, damage wasted: %d, accuracy(%%): %d", player1->totaldamage, player1->totalshots-player1->totaldamage, accuracy);				
			if(m_sp)
			{
				conoutf("\f2--- single player time score: ---");
				int pen, score = 0;
                pen = (lastmillis-maptime)/1000; score += pen; if(pen) conoutf("\f2time taken: %d seconds", pen); 
				pen = player1->deaths*60; score += pen; if(pen) conoutf("\f2time penalty for %d deaths (1 minute each): %d seconds", player1->deaths, pen);
				pen = ms.remain*10;		score += pen; if(pen) conoutf("\f2time penalty for %d monsters remaining (10 seconds each): %d seconds", ms.remain, pen);
				pen = (10-ms.skill())*20; score += pen; if(pen) conoutf("\f2time penalty for lower skill level (20 seconds each): %d seconds", pen);
				pen = 100-accuracy;		score += pen; if(pen) conoutf("\f2time penalty for missed shots (1 second each %%): %d seconds", pen);
				s_sprintfd(aname)("bestscore_%s", getclientmap());
				const char *bestsc = getalias(aname);
				int bestscore = *bestsc ? atoi(bestsc) : score;
				if(score<bestscore) bestscore = score;
				s_sprintfd(nscore)("%d", bestscore);
				alias(aname, nscore);
				conoutf("\f2TOTAL SCORE (time + time penalties): %d seconds (best so far: %d seconds)", score, bestscore);
			}
#endif
			sb.showscores(true);
		}
		else if(timeremain > 0)
		{
#ifdef BFRONTIER
			console("\f2time remaining: %d %s", CON_LEFT|CON_CENTER, timeremain, timeremain==1 ? "minute" : "minutes");
#else
            conoutf("\f2time remaining: %d %s", timeremain, timeremain==1 ? "minute" : "minutes");
#endif
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
#ifdef BFRONTIER
		if (cameranum == -cn) cameradir(1, true);
#else
        if(following==cn) stopfollowing();
#endif
		fpsent *d = players[cn];
		if(!d) return; 
		if(d->name[0]) conoutf("player %s disconnected", colorname(d));
		ws.removebouncers(d);
		ws.removeprojectiles(d);
		DELETEP(players[cn]);
		cleardynentcache();
	}

	void initclient()
	{
		clientmap[0] = 0;
		cc.initclientnet();
	}

	void startmap(const char *name)	// called just after a map load
	{
		suicided = -1;
		respawnent = -1;
#ifndef BFRONTIER
		if(multiplayer(false) && m_sp) { gamemode = 0; conoutf("coop sp not supported yet"); }
#endif
		cc.mapstart();
		ms.monsterclear(gamemode);
		ws.projreset();

		// reset perma-state
		player1->frags = 0;
		player1->deaths = 0;
		player1->totaldamage = 0;
		player1->totalshots = 0;
		player1->maxhealth = 100;
		loopv(players) if(players[i])
		{
			players[i]->frags = 0;
			players[i]->deaths = 0;
			players[i]->totaldamage = 0;
			players[i]->totalshots = 0;
			players[i]->maxhealth = 100;
		}

		if(!m_mp(gamemode)) spawnplayer(player1);
		else findplayerspawn(player1, -1);
		et.resetspawns();
		s_strcpy(clientmap, name);
		sb.showscores(false);
		intermission = false;
        maptime = lastmillis;

#ifdef BFRONTIER
		if(m_sp)
		{
			s_sprintfd(aname)("bestscore_%s", getclientmap());
			const char *best = getalias(aname);
			if(*best) conoutf("\f2try to beat your best score so far: %s", best);
		}
		cameranum = 0;
		if (g_sauer && *name)
		{
			s_sprintfd(cfgname)("packages/%s.cfx", mapname);
			path(cfgname);
			execfile(cfgname);
		}
		bc.start(name);
#else
		if(*name) conoutf("\f2game mode is %s", fpsserver::modestr(gamemode));
		if(m_sp)
		{
			s_sprintfd(aname)("bestscore_%s", getclientmap());
			const char *best = getalias(aname);
			if(*best) conoutf("\f2try to beat your best score so far: %s", best);
		}
#endif
	}

#ifndef BFRONTIER
	void physicstrigger(physent *d, bool local, int floorlevel, int waterlevel)
	{
		if	 (waterlevel>0) playsound(S_SPLASH1, d==player1 ? NULL : &d->o);
		else if(waterlevel<0) playsound(S_SPLASH2, d==player1 ? NULL : &d->o);
		if	 (floorlevel>0) { if(local) playsoundc(S_JUMP, (fpsent *)d); else if(d->type==ENT_AI) playsound(S_JUMP, &d->o); }
		else if(floorlevel<0) { if(local) playsoundc(S_LAND, (fpsent *)d); else if(d->type==ENT_AI) playsound(S_LAND, &d->o); }
	}
#endif

	void playsoundc(int n, fpsent *d = NULL) 
	{ 
		if(!d || d==player1)
		{
			cc.addmsg(SV_SOUND, "i", n); 
			playsound(n); 
		}
		else playsound(n, &d->o);
	}

	int numdynents() { return 1+players.length()+ms.monsters.length(); }

	dynent *iterdynents(int i)
	{
		if(!i) return player1;
		i--;
		if(i<players.length()) return players[i];
		i -= players.length();
		if(i<ms.monsters.length()) return ms.monsters[i];
		return NULL;
	}

	bool duplicatename(fpsent *d, char *name = NULL)
	{
		if(!name) name = d->name;
		if(d!=player1 && !strcmp(name, player1->name)) return true;
		loopv(players) if(players[i] && d!=players[i] && !strcmp(name, players[i]->name)) return true;
		return false;
	}

	char *colorname(fpsent *d, char *name = NULL, char *prefix = "")
	{
		if(!name) name = d->name;
		if(name[0] && !duplicatename(d, name)) return name;
		static string cname;
		s_sprintf(cname)("%s%s \fs\f5(%d)\fr", prefix, name, d->clientnum);
		return cname;
	}

	void suicide(physent *d)
	{
		if(d==player1)
		{
			if(d->state!=CS_ALIVE) return;
			if(!m_mp(gamemode)) killed(player1, player1);
			else if(suicided!=player1->lifesequence)
			{
				cc.addmsg(SV_SUICIDE, "r");
				suicided = player1->lifesequence;
			}
		}
#ifdef BFRONTIER
		else if(bc.isbot((fpsent *)d)) killed((fpsent *)d, (fpsent *)d);
#else
		else if(d->type==ENT_AI) ((monsterset::monster *)d)->monsterpain(400, player1);
#endif
	}

	IVARP(hudgun, 0, 1, 1);
	IVARP(hudgunsway, 0, 1, 1);

	void drawhudmodel(int anim, float speed = 0, int base = 0)
	{
#ifdef BFRONTIER
		static char *hudgunnames[] = { "hudguns/fist", "hudguns/shotg", "hudguns/chaing", "hudguns/rocket", "hudguns/rifle", "hudguns/gl", "hudguns/pistol", NULL, NULL, NULL, NULL };
		if(player1->gunselect>GUN_PISTOL) return;
#else
        static char *hudgunnames[] = { "hudguns/fist", "hudguns/shotg", "hudguns/chaing", "hudguns/rocket", "hudguns/rifle", "hudguns/gl", "hudguns/pistol" };
        if(player1->gunselect>GUN_PISTOL) return;
#endif
		vec sway, color, dir;
		vecfromyawpitch(player1->yaw, player1->pitch, 1, 0, sway);
		float swayspeed = min(4.0f, player1->vel.magnitude());
		sway.mul(swayspeed);
		float swayxy = sinf(swaymillis/115.0f)/100.0f,
			  swayz = cosf(swaymillis/115.0f)/100.0f;
		swap(float, sway.x, sway.y);
		sway.x *= -swayxy;
		sway.y *= swayxy;
		sway.z = -fabs(swayspeed*swayz);
		sway.add(swaydir).add(player1->o);
		if(!hudgunsway()) sway = player1->o;
		lightreaching(sway, color, dir);
        dynlightreaching(sway, color, dir);

#if 0
		if(player1->state!=CS_DEAD && player1->quadmillis)
		{
			float t = 0.5f + 0.5f*sinf(2*M_PI*lastmillis/1000.0f);
			color.y = color.y*(1-t) + t;
		}
#endif
		const char *gunname = hudgunnames[player1->gunselect];
		rendermodel(color, dir, gunname, anim, 0, 0, sway, player1->yaw+90, player1->pitch, speed, base, NULL, 0);
	}

	void drawhudgun()
	{
		if(!hudgun() || editmode || player1->state==CS_SPECTATOR) return;

#ifdef BFRONTIER
		int rtime = gunvar(player1->gunwait, player1->gunselect);
		if (lastmillis - gunvar(player1->gunlast, player1->gunselect) > rtime)
		{
			drawhudmodel(ANIM_GUNSHOOT, rtime/17.0f, gunvar(player1->gunlast, player1->gunselect));
		}
		else
		{
			drawhudmodel(ANIM_GUNIDLE|ANIM_LOOP);
		}
#else
		int rtime = ws.reloadtime(player1->gunselect);
		if(player1->lastattackgun==player1->gunselect && lastmillis-player1->lastaction<rtime)
		{
			drawhudmodel(ANIM_GUNSHOOT, rtime/17.0f, player1->lastaction);
		}
		else
		{
			drawhudmodel(ANIM_GUNIDLE|ANIM_LOOP);
		}
#endif
	}

#ifdef BFRONTIER
	void drawicon(float tx, float ty, int x, int y, int s = 64)
	{
		rendericon("packages/icons/overlay.png", x, y, s, s);
		
		settexture("data/items.png");
		
		glBegin(GL_QUADS);
		
		int t = s-(s/8)-2, rx = x+(s/16)+1, ry = y+(s/16)+1;
		
		tx /= 384;
		ty /= 128;
		
		glTexCoord2f(tx,		ty);
		glVertex2i(rx,	ry);
		glTexCoord2f(tx+1/6.0f, ty);
		glVertex2i(rx+t, ry);
		glTexCoord2f(tx+1/6.0f, ty+1/2.0f);
		glVertex2i(rx+t, ry+t);
		glTexCoord2f(tx,		ty+1/2.0f);
		glVertex2i(rx,	ry+t);
		
		glEnd();
	}

	void gameplayhud(int w, int h)
	{
		if (!hidehud)
		{
			int ox = w*900/h, oy = 900;
			
			glLoadIdentity();
			glOrtho(0, ox, oy, 0, -1, 1);
			
			glEnable(GL_BLEND);
			
			if (!titlecard(ox, oy, lastmillis-maptime))
			{
				extern int hudblend;
				float fade = 1.f, amt = hudblend*0.01f;
				fpsent *d = player1;
				
				if (lastmillis-maptime <= CARDTIME+CARDFADE+CARDFADE)
					fade = amt*(float(lastmillis-maptime-CARDTIME-CARDFADE)/float(CARDFADE));
				else
					fade *= amt;
					
				if (player1->state == CS_SPECTATOR)
				{
					if (player1->clientnum == -cameranum)
						d = player1;
					else if (players.inrange(-cameranum) && players[-cameranum])
						d = players[-cameranum];
				}
				
				glColor4f(1.f, 1.f, 1.f, amt);
				rendericon("packages/icons/sauer.jpg", 20, oy-75, 64, 64);
				
				if (d != NULL)
				{
					if (d->health > 0 && d->state == CS_ALIVE)
					{
						int hs[4][HD_MAX][5] = {
							{
								{ 80, 75, 96, 75, AL_LEFT },
								{ 80, 75, 96, 75, AL_LEFT },
								{ 80, 75, 96, 75, AL_RIGHT },
							},
							{
								{ 280, 75, 296, 75, AL_LEFT },
								{ 80, 145, 96, 145, AL_LEFT },
								{ 80, 145, 96, 145, AL_RIGHT },
							},
							{
								{ 480, 75, 496, 75, AL_LEFT },
								{ 80, 215, 96, 215, AL_LEFT },
								{ 80, 215, 96, 215, AL_RIGHT },
							},
							{
								{ 680, 75, 648, 75, AL_CENTER },
								{ 80, 285, 48, 285, AL_CENTER },
								{ 80, 285, 48, 285, AL_CENTER },
							},
						};
						
						#define style(a,b) \
							(hudstyle() == HD_RIGHT && b != 4 ? (b%2 ? oy : ox)-hs[a][hudstyle()][b] : (b%2 ? oy-hs[a][hudstyle()][b] : hs[a][hudstyle()][b]))
						
						glColor4f(1.f, 1.f, 1.f, fade);
						drawicon(192.f, 0.f, style(0,0), style(0,1));
						draw_textx("%d", style(0,2), style(0,3), 255, d->health<=50 ? (d->health<=25 ? 0 : 128) : 255, d->health<=50 ? 0 : 255, int(255.f*fade), style(0,4), d->health);
						
						glColor4f(1.f, 1.f, 1.f, fade);
						drawicon((float)(d->armourtype*64), 0.f, style(1,0), style(1,1));
						draw_textx("%d", style(1,2), style(1,3), 255, d->armour<=50 ? (d->armour<=25 ? 0 : 128) : 255, d->armour<=50 ? 0 : 255, int(255.f*fade), style(1,4), d->armour);
						
						glColor4f(1.f, 1.f, 1.f, fade);
						
						int g = d->gunselect;
						int r = 64;
						if(g==GUN_PISTOL)
						{
							g = 4;
							r = 0;
						}
						int mx[NUMGUNS] =
							{
								0, 30, 60, 15, 15, 30, 120, 0, 0, 0, 0
							};
							
						drawicon((float)(g*64), (float)r, style(2,0), style(2,1));
						
						glColor4f(1.f, 1.f, 1.f, fade);
						draw_textx("%d", style(2,2), style(2,3), 255, d->ammo[d->gunselect]<=mx[d->gunselect]/2 ? (d->ammo[d->gunselect]<=mx[d->gunselect]/4 ? 0 : 128) : 255, mx[d->gunselect]/2 ? 0 : 255, int(255.f*fade), style(2,4), d->ammo[d->gunselect]);
						
						if (d->quadmillis)
						{
							string qs;
							s_sprintf(qs)("%d", d->quadmillis/1000);
							float c = d->quadmillis <= CARDTIME+CARDFADE+CARDFADE ? 1.f-float(d->quadmillis)/float(CARDTIME+CARDFADE+CARDFADE) : 0.f;
							float f = d->quadmillis <= CARDFADE ? fade*(float(d->quadmillis)/float(CARDFADE)) : fade;
							
							glColor4f(1.f, 1.f, 1.f, f);
							rendericon("packages/icons/blank.jpg", style(3,0), style(3,1), 64, 64);
							draw_textx("%s", style(3,2), style(3,3), 255-int(255.f*c), int(255.f*c), 0, int(255.f*f), style(3,4), qs);
						}
					}
					else if (d->state == CS_DEAD)
					{
						int action = lastmillis-d->lastpain;
						
						if (m_capture && action < cpc.RESPAWNSECS*1000)
						{
							float c = float((cpc.RESPAWNSECS*1000)-action)/1000.f;
							draw_textx("Fragged! Down for %.1fs", 100, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT, c);
						}
						else
							draw_textx("Fragged! Press attack to respawn", 100, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT);
					}
				}
				if (!editmode)
				{
					glLoadIdentity();
					glOrtho(0, w*1800/h, 1800, 0, -1, 1);
					
					glDisable(GL_BLEND);
					
					if(m_capture) cpc.capturehud(w, h);
				}
			}
		}
	}

	void crosshaircolor(float &r, float &g, float &b)
	{
		if(player1->state!=CS_ALIVE) return;
		if(gunvar(player1->gunwait,player1->gunselect)) r = g = b = 0.5f;
		else if(!editmode && !m_noitemsrail)
		{
			if(player1->health<=25) { r = 1.0f; g = b = 0; }
			else if(player1->health<=50) { r = 1.0f; g = 0.5f; b = 0; }
		}
	}

	void lighteffects(dynent *e, vec &color, vec &dir)
	{
		fpsent *d = (fpsent *)e;
		if ((d->type == ENT_PLAYER || d->type == ENT_AI) && d->state == CS_ALIVE)
		{
			if (d->type == ENT_PLAYER && d->quadmillis)
			{
				color.x = 1;
			}
		}
	}
#else
	void drawicon(float tx, float ty, int x, int y)
	{
		settexture("data/items.png");
		glBegin(GL_QUADS);
		tx /= 384;
		ty /= 128;
		int s = 120;
		glTexCoord2f(tx,		ty);		glVertex2i(x,	y);
		glTexCoord2f(tx+1/6.0f, ty);		glVertex2i(x+s, y);
		glTexCoord2f(tx+1/6.0f, ty+1/2.0f); glVertex2i(x+s, y+s);
		glTexCoord2f(tx,		ty+1/2.0f); glVertex2i(x,	y+s);
		glEnd();
	}

	void gameplayhud(int w, int h)
	{
		glLoadIdentity();
		glOrtho(0, w*900/h, 900, 0, -1, 1);
		if(player1->state==CS_SPECTATOR)
		{
			draw_text("SPECTATOR", 10, 827);
            if(m_capture)
            {
                glLoadIdentity();
                glOrtho(0, w*1800/h, 1800, 0, -1, 1);
                cpc.capturehud(w, h);
            }
			return;
		}
		draw_textf("%d",  90, 822, player1->state==CS_DEAD ? 0 : player1->health);
		if(player1->state!=CS_DEAD)
		{
			if(player1->armour) draw_textf("%d", 390, 822, player1->armour);
			draw_textf("%d", 690, 822, player1->ammo[player1->gunselect]);		
		}

		glLoadIdentity();
		glOrtho(0, w*1800/h, 1800, 0, -1, 1);

		glDisable(GL_BLEND);

		drawicon(192, 0, 20, 1650);
		if(player1->state!=CS_DEAD)
		{
			if(player1->armour) drawicon((float)(player1->armourtype*64), 0, 620, 1650);
			int g = player1->gunselect;
			int r = 64;
			if(g==GUN_PISTOL) { g = 4; r = 0; }
			drawicon((float)(g*64), (float)r, 1220, 1650);
		}
		if(m_capture) cpc.capturehud(w, h);
	}

	void crosshaircolor(float &r, float &g, float &b)
	{
		if(player1->state!=CS_ALIVE) return;
		if(player1->gunwait) r = g = b = 0.5f;
		else if(!editmode && !m_noitemsrail)
		{
			if(player1->health<=25) { r = 1.0f; g = b = 0; }
			else if(player1->health<=50) { r = 1.0f; g = 0.5f; b = 0; }
		}
	}

	void lighteffects(dynent *e, vec &color, vec &dir)
	{
#if 0
		fpsent *d = (fpsent *)e;
		if(d->state!=CS_DEAD && d->quadmillis)
		{
			float t = 0.5f + 0.5f*sinf(2*M_PI*lastmillis/1000.0f);
			color.y = color.y*(1-t) + t;
		}
#endif
	}
#endif

	void newmap(int size)
	{
		cc.addmsg(SV_NEWMAP, "ri", size);
	}

	void edittrigger(const selinfo &sel, int op, int arg1, int arg2, int arg3)
	{
#ifdef BFRONTIER
        if(multiplayer(false) && gamemode==1) switch(op)
#else
        if(gamemode==1) switch(op)
#endif
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

	// any data written into this vector will get saved with the map data. Must take care to do own versioning, and endianess if applicable. Will not get called when loading maps from other games, so provide defaults.
	void writegamedata(vector<char> &extras) {}
	void readgamedata(vector<char> &extras) {}

#ifdef BFRONTIER
	vec feetpos(physent *d)
	{
		return vec(d->o).sub(vec(0, 0, d->eyeheight));
	}

	void loadworld(const char *name)
	{
	}
	
	void saveworld(const char *name)
	{
	}

	char *savedconfig() { return "config.cfg"; }
	char *defaultconfig() { return "defaults.cfg"; }
	char *autoexec() { return "autoexec.cfg"; }
	char *savedservers() { return "servers.cfg"; }

	bool gethudcolour(vec &colour)
	{
		if (lastmillis-maptime <= CARDTIME)
		{
			float fade = (float(lastmillis-maptime)/float(CARDTIME));
			colour = vec(fade, fade, fade);
			return true;
		}
		else if (player1->state == CS_ALIVE && player1->quadmillis)
		{
			colour = vec((float(player1->quadmillis)/float(getitem(I_QUAD-I_SHELLS).max))+0.1f, 0.5f, 0.5f);
			return true;
		}
		else
		{
			int fogmat = getmatvec(camera1->o);
			if (fogmat == MAT_WATER || fogmat == MAT_LAVA)
			{
				uchar col[3];
				if(fogmat == MAT_WATER) getwatercolour(col);
				else getlavacolour(col);
	
				float maxc = max(col[0], max(col[1], col[2]));
				float blend[3];
	
				loopi(3) blend[i] = col[i] / min(32 + maxc*7/8, 255);
	
				colour = vec(blend[0], blend[1], blend[2]);
	
				return true;
			}
		}
		return false;
	}
	
	void fixcamerarange(physent *d)
	{
		const float MAXPITCH = 90.0f;
		if(d->pitch>MAXPITCH) d->pitch = MAXPITCH;
		if(d->pitch<-MAXPITCH) d->pitch = -MAXPITCH;
		while(d->yaw<0.0f) d->yaw += 360.0f;
		while(d->yaw>=360.0f) d->yaw -= 360.0f;
	}
	
	void fixcamerarange()
	{
		extern physent *camera1;
		physent *d = isthirdperson() && thirdpersonstick ? camera1 : player1;
		fixcamerarange(d);
	}
	
	void mousemove(int dx, int dy)
	{
		const float SENSF = 33.0f;	 // try match quake sens
		extern int sensitivity, sensitivityscale, invmouse;
		extern physent *camera1;
		
		physent *d = isthirdperson() && thirdpersonstick ? camera1 : player1;
		
		d->yaw += (dx/SENSF)*(sensitivity/(float)sensitivityscale);
		d->pitch -= (dy/SENSF)*(sensitivity/(float)sensitivityscale)*(invmouse ? -1 : 1);
		
		fixcamerarange(d);
	}
	
	physent fpscamera;
	
	void findorientation()
	{
		extern physent *camera1;
		physent *d = camera1;
		if (isthirdperson() && !thirdpersonstick && (player1->state != CS_SPECTATOR || player1->clientnum == cameranum))
			d = player1;
			
		vec dir;
		vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
		vecfromyawpitch(d->yaw, 0, 0, -1, camright);
		vecfromyawpitch(d->yaw, d->pitch+90, 1, 0, camup);
		
		if(raycubepos(d->o, dir, worldpos, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
			worldpos = dir.mul(10).add(d->o); //otherwise 3dgui won't work when outside of map
	}
	
	void recomputecamera()
	{
		extern physent *camera1;
		int secs = time(NULL);
		int state, cameratype = 0;
		
		camera1 = &fpscamera;
		
		if (camera1->type != ENT_CAMERA)
		{
			camera1->reset();
			camera1->type = ENT_CAMERA;
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
				camera1->o = players[-cameranum]->o;
				if (!isthirdperson() || !thirdpersonstick)
				{
					//camera1->yaw = players[-cameranum]->yaw;
					//camera1->pitch = players[-cameranum]->pitch;
					extern void interpolateorientation(dynent *d, float &interpyaw, float &interppitch);
					interpolateorientation(players[-cameranum], camera1->yaw, camera1->pitch);
				}
				state = players[-cameranum]->state;
				cameratype = 1;
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
							camera1->o = et.ents[i]->o;
							camera1->yaw = et.ents[i]->attr1;
							camera1->pitch = et.ents[i]->attr2;
							state = CS_ALIVE;
							cameratype = 2;
							break;
						}
						cameras++;
					}
				}
			}
		}
		
		if (cameratype <= 0)
		{
			camera1->o = player1->o;
			if (!isthirdperson() || !thirdpersonstick)
			{
				camera1->yaw = player1->yaw;
				camera1->pitch = player1->pitch;
				camera1->roll = player1->roll;
			}
			state = player1->state;
			cameratype = 0;
		}
		else
		{
			camera1->pitch = player1->pitch;
			camera1->roll = 0.f;
		}
		camera1->eyeheight = 0;
		
		if (isthirdperson() || cameratype > 0)
		{
			if (cameratype != 2)
			{
				if (isthirdperson())
				{
					vec old(camera1->o);
					
					camera1->move = -1;
					camera1->o.z += thirdpersonheight;
					
					if (!thirdpersonstick && !cameratype && thirdpersonscale)
						camera1->pitch = (0.f-fabs(camera1->pitch))*(thirdpersonscale/100.f);
						
					fixcamerarange(camera1);
					
					loopi(10)
					{
						if(!moveplayer(camera1, 10, false, thirdpersondistance))
							break;
					}
					
					if (!thirdpersonstick)
					{
						vec v(cameratype > 0 ? old : worldpos);
						v.sub(camera1->o);
						v.normalize();
						vectoyawpitch(v, camera1->yaw, camera1->pitch);
					}
				}
			}
			
			if (cameratype > 0)
			{
				player1->o = camera1->o;
				player1->yaw = camera1->yaw;
			}
		}
		
		if (camerawobble > 0 && curtime)
		{
			float pc = float(camerawobble)/200.f;
			#define wobble(n) (float(rnd(n*2)-n)*pc)
			camera1->yaw += wobble(20);
			camera1->pitch += wobble(10);
			camera1->roll += wobble(10);
			camerawobble -= max(curtime/10, 1);
		}
		
		fixcamerarange(camera1);
	}
	
	void adddynlights()
	{
		loopi(numdynents())
		{
			fpsent *d = (fpsent *)iterdynents(i);
			
			if (d && d->state == CS_ALIVE)
			{
				int illum = 0;
				vec color(0, 0, 0);
	
				if (d->type == ENT_PLAYER && d->quadmillis)
				{
					int offset = (d->quadmillis%1000)+1;
					float scale = (float(d->quadmillis)/float(getitem(I_QUAD-I_SHELLS).max))+0.1f;
					if (offset > 500) offset = 1000 - offset;
					illum = int((ILLUMINATE-(ILLUMINATE*(offset*0.001f)))*scale);
					color = vec(1, 0, 0);
				}
		
				if (illum)
				{
					adddynlight(vec(d->o).sub(vec(0, 0, d->eyeheight/2)), illum, color, 0, 0);
				}
			}
		}
		
		ws.dynlightprojectiles();
	}
	
	
	bool wantcrosshair()
	{
		return (crosshair() && !(hidehud || player->state == CS_SPECTATOR)) || menuactive();
	}
	
	bool gamethirdperson()
	{
		return !editmode && (thirdperson || player1->state == CS_DEAD);
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
				if (cameranum > 0) console("camera %d selected", CON_LEFT|CON_CENTER, cameranum);
				else if (player1->clientnum == -cameranum) console("spectator camera selected", CON_LEFT|CON_CENTER);
				else console("chasecam %s selected", CON_LEFT|CON_CENTER, players[-cameranum]->name);
			}
		}
		else if (!quiet) console("no other cameras available", CON_LEFT|CON_CENTER);
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
			
		if(m_teammode)
		{
			if(m_capture)
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
							s_sprintf(rinfo)("You've taken %s", myrankv!=-1?"\f1the lead":"\f3first blood");
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
		console("\f2%d%s place", CON_LEFT|CON_CENTER, myrankv+1, myrankv ? myrankv == 1 ? "nd" : myrankv == 2 ? "rd" : "th" : "st");
	}
#else
	char *gameident() { return "fps"; }
	char *defaultmap() { return "metl4"; }
	char *savedconfig() { return "config.cfg"; }
	char *defaultconfig() { return "data/defaults.cfg"; }
	char *autoexec() { return "autoexec.cfg"; }
	char *savedservers() { return "servers.cfg"; }
#endif
};

REGISTERGAME(fpsgame, "fps", new fpsclient(), new fpsserver());

#else

REGISTERGAME(fpsgame, "fps", NULL, new fpsserver());

#endif



