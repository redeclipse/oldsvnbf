// Blood Frontier - SSPGAME - Side Scrolling Platformer by Quinton Reeves
// This is the primary SSP game module.

#ifdef BFRONTIER
#include "pch.h"
#include "cube.h"

#include "iengine.h"
#include "igame.h"

#include "sspgame.h"

struct sspclient : igameclient
{
	#include "ssprender.h"
    #include "sspents.h"
    #include "sspprojs.h"
    #include "sspweap.h"
    #include "sspblock.h"
    #include "sspenemy.h"

	struct sspdummycom : iclientcom
	{
		~sspdummycom() { }
	
		void gamedisconnect() {}
		void parsepacketclient(int chan, ucharbuf &p) {}
		int sendpacketclient(ucharbuf &p, bool &reliable, dynent *d) { return -1; }
		void gameconnect(bool _remote) {}
		bool allowedittoggle() { return true; }
		void writeclientinfo(FILE *f) {}
		bool ready() { return true; }
		void toserver(char *text) {}
		void changemap(const char *name) { load_world(name); }
	};
	
	struct sspdummybot : ibotcom
	{
		~sspdummybot() { }
	};
	
	struct sspdummyphysics : iphysics
	{
		sspclient &cl;
	
		sspdummyphysics(sspclient &_cl) : cl(_cl)
		{
			CVAR(sspdummyphysics, gravity,		1-INT_MAX,	200,		INT_MAX-1);	// gravity
			CVAR(sspdummyphysics, jumpvel,		0,			200,		1024);		// extra velocity to add when jumping
			CVAR(sspdummyphysics, speed,		0,			70,			INT_MAX-1);		// maximum speed
			CVAR(sspdummyphysics, watertype,	WT_WATER,	WT_WATER,	WT_MAX-1);	// water type (0: normal, 1: hurt, 2: kill)
			CVAR(sspdummyphysics, watervel,		1-INT_MAX,	200,		INT_MAX-1);	// extra water velocity
		}
	
		float stairheight(physent *d) { return 4.1f; }
		float floorz(physent *d) { return 0.867f; }
		float slopez(physent *d) { return 0.5f; }
		float wallz(physent *d) { return 0.2f; }
		float jumpvel(physent *d) { return d->inwater ? float(getvar("watervel")) : float(getvar("jumpvel")); }
		float gravity(physent *d) { return float(getvar("gravity")); }
		float speed(physent *d)
		{
			if (d->state != CS_SPECTATOR && d->state != CS_EDITING)
			{
				return d->maxspeed * (float(getvar("speed"))/100.f);
			}
			return d->maxspeed;
		}
		float stepspeed(physent *d) { return 1.0f; }
		float watergravscale(physent *d) { return 6.0f; }
		float waterfric(physent *d) { return 20.f; }
		float waterdampen(physent *d) { return 2.f; }
		float floorfric(physent *d) { return 6.f; }
		float airfric(physent *d) { return (d->type == ENT_PLAYER || d->type == ENT_AI) && d->state == CS_ALIVE && ((sspent *)d)->special ? 100.f : 30.f; }
	
		bool movepitch(physent *d)
		{
			if (getvar("slope") || d->type == ENT_CAMERA) return true;
			return false;
		}
	
		void updateroll(physent *d) {}
	
		void trigger(physent *d, bool local, int floorlevel, int waterlevel)
		{
			if (waterlevel) cl.updatewater((sspent *)d, waterlevel);
	
			if (d->type == ENT_PLAYER)
			{
				if (floorlevel > 0) cl.sfx(SF_JUMP, d != cl.player1 ? &d->o : NULL);
				else if (floorlevel < 0) cl.sfx(SF_LAND, d != cl.player1 ? &d->o : NULL);
			}
		}
	};
	
	sspdummyphysics ph;
    sspentities		et;
    sspprojectiles	pr;
    sspweapons		wp;
	sspblock		bl;
	sspenemy		en;
	sspdummybot		bc;
    sspdummycom		cc;
    
    cament *camera;
    sspent *player1;
	sspsave saves[MAXSAVES];

    int cheat, leveltime, mapmove, maptime, endtime, nextspawn, nextdynlight;
    bool wascamera;
	
    string mapnext, maptext;
    
    sspclient() : 
		ph(*this), et(*this), pr(*this), wp(*this), bl(*this), en(*this),
		camera(new cament()), player1(new sspent()),
		nextspawn(-1), nextdynlight(0)
    {
        CVAR(sspclient, gamemillis,		0,			0,			-1);		// current update
        CVAR(sspclient, gametime,		0,			0,			-1);		// current level time
        CVAR(sspclient, world,			0,			0,			-1);		// current map world
        CVAR(sspclient, level,			0,			0,			-1);		// current map index

        CVAR(sspclient, camerafov,		10,			70,			150);		// camera forced fov
        CVAR(sspclient, camerarot,		0,			1,			1);			// camera rotation
        CVAR(sspclient, cameradist,		1-INT_MAX,	128,		INT_MAX-1);	// camera distance
        CVAR(sspclient, cameradoff,		1-INT_MAX,	0,			INT_MAX-1);	// camera direction offset
        CVAR(sspclient, camerazoff,		1-INT_MAX,	16,			INT_MAX-1);	// camera z offset
        CVAR(sspclient, cameradvel,		-100,		-10,		100);		// camera direction velocity percentage
        CVAR(sspclient, camerazvel,		-100,		0,			100);		// camera z velocity percentage

        CVAR(sspclient, clipdsize,		1-INT_MAX,	128,		INT_MAX-1);	// world direction clip
        CVAR(sspclient, clipzsize,		1-INT_MAX,	128,		INT_MAX-1);	// world z clip

        CVAR(sspclient, clobberzvel,	1-INT_MAX,	125,		INT_MAX-1);	// extra velocity to add when clobbering an enemy

        CVAR(sspclient, cuttime,		0,			0,			INT_MAX-1);	// current cut time
        CVAR(sspclient, cuthold,		0,			0,			1);			// current cut hold

        CVAR(sspclient, debrisamt,		0,			25,			INT_MAX-1);	// number of debris to throw
        CVAR(sspclient, elecsize,		0,			64,			INT_MAX-1);	// distance to items for electric attraction
        CVAR(sspclient, enemylos,		0,			148,		INT_MAX-1);	// distance to enemy before they wake
        CVAR(sspclient, entspeed,		0,			96,			INT_MAX-1);	// entity speed when projectile
        CVAR(sspclient, enttime,		0,			10000,		INT_MAX-1);	// entity time when projectile
        CVAR(sspclient, gibamt,			0,			25,			INT_MAX-1);	// number of gibs to throw
        CVAR(sspclient, idletime,		0,			10000,		INT_MAX-1);	// idle taunt timeout
        CVAR(sspclient, invulnerable,	0,			3000,		INT_MAX-1);	// invulnerable time when being hurt
        CVAR(sspclient, invincible,		0,			10000,		INT_MAX-1);	// invincible time
        CVAR(sspclient, projtime,		0,			3000,		INT_MAX-1);	// weapon projectile time
        CVAR(sspclient, slope,			0,			1,			1);			// slopes
        CVAR(sspclient, teledvel,		1-INT_MAX,	75,			INT_MAX-1);	// extra push when exiting a teleport
        CVAR(sspclient, timelimit,		0,			300000,		INT_MAX-1);	// timelimit of each level
        CVAR(sspclient, worldamt,		0,			0,			INT_MAX-1);

        #ifdef SSPDEBUG
		CCOMMAND(sspclient, cheattoggle, "i", self->cheattoggle(atoi(args[0])));
		CCOMMAND(sspclient, cheatset, "i", self->cheatset(atoi(args[0])));
		#endif

		CCOMMAND(sspclient, map, "s", self->loadmap(args[0]));
		CCOMMAND(sspclient, map_text, "s", s_strcpy(self->maptext, args[0]));
        
		CCOMMAND(sspclient, inventory, "", self->inventory());
        CCOMMAND(sspclient, resetgame, "i", self->resetgame(atoi(args[0]) ? true : false););

        CVARF(sspclient, skill, 1, 1, 10, self->reset();); // skill level

        CVARF(sspclient, saveslot, 0, 0, MAXSAVES, self->loadsave(*_storage, true);); // current save state (0 = don't save)
        CCOMMAND(sspclient, savereset, "i", self->writesave(atoi(args[0]), true););
        CCOMMAND(sspclient, savelist, "i", result(self->listsave(atoi(args[0]))););

		CVARF(sspclient, spectator, 0, 0, 1, {
			if (self->player1->state != CS_DEAD) self->player1->state = (*_storage ? CS_SPECTATOR : (editmode ? CS_EDITING : CS_ALIVE));
		});
    }
    ~sspclient() { }
    
    char *getclientmap() { return mapname; }
    icliententities *getents() { return &et; }
    iphysics *getphysics() { return &ph; }
	ibotcom *getbot() { return &bc; }
    iclientcom *getcom() { return &cc; }

	char *savedconfig() { return "config.cfg"; }
	char *defaultconfig() { return "defaults.cfg"; }
	char *autoexec() { return "autoexec.cfg"; }
	char *savedservers() { return "servers.cfg"; }

	bool gui3d() { return false; }

	vec feetpos(physent *d)
	{
		if (d->type == ENT_PLAYER || d->type == ENT_AI || d->type == ENT_BLOCK)
			return vec(d->o).sub(vec(0, 0, d->eyeheight-1));
		
		return vec(d->o);
	}

	bool iscamera() { return getvar("spectator") || editmode; }
	
	bool isfinished()
	{
		return (endtime > 0 || (et.ents.inrange(player1->sd.chkpoint) &&
			et.ents[player1->sd.chkpoint]->type == SE_GOAL &&
			(et.ents[player1->sd.chkpoint]->attr1 > 0 || et.ents[player1->sd.chkpoint]->attr2 == 0)));
	}
	
    bool canjump() { return !menuactive() && endtime <= 0; }

	#ifdef SSPDEBUG
	void cheattoggle(int a)
	{
		cheat += a;
		if (cheat < CH_POWER) cheat = CH_MAX-1;
		else if (cheat >= CH_MAX) cheat = CH_POWER;		
		conoutf("cheat '%s' selected", cheats[cheat]);
	}

	void cheatset(int a)
	{
		switch (cheat)
		{
			case CH_POWER:
				player1->sd.power += a;
				if (player1->sd.power < 1) player1->sd.power = MAXPOWER;
				else if (player1->sd.power > MAXPOWER) player1->sd.power = 1;
				player1->setpower(player1->sd.power);
				conoutf("cheat: power level set to '%d'", player1->sd.power);
				break;
			case CH_WEAPON:
				player1->sd.gun += a;
				if (player1->sd.gun < GN_FIST) player1->sd.gun = GN_MAX-1;
				else if (player1->sd.gun >= GN_MAX) player1->sd.gun = GN_FIST;
				player1->setpower(player1->sd.power);
				conoutf("cheat: gun set to '%s'", wp.guns[player1->sd.gun].name);
				break;
			case CH_SHIELD:
				player1->sd.shield += a;
				if (player1->sd.shield < SH_NONE) player1->sd.shield = SH_MAX-1;
				else if (player1->sd.shield >= SH_MAX) player1->sd.shield = SH_NONE;
				conoutf("cheat: shield set to '%s'", sname[player1->sd.shield]);
				break;
			default:
				break;
		}
	}
	#endif

	// map operations

	void loadmap(const char *name)
	{
		vec v(player1->o);
		v.z -= player1->eyeheight;
		//part_spawn(v, vec(player1->xradius, player1->yradius, player1->eyeheight), 0, 2, 100, 500, COL_WHITE);
		s_sprintf(mapnext)("%s", name);
		endtime = lastmillis;
	}

	void nextmap()
	{
		string nextmapalias;
		s_sprintf(nextmapalias)("map_%d_%d", getvar("world"), getvar("level"));

		const char *map = getalias(nextmapalias);
		if (!*map) map = getdefaultmap();
		loadmap(map);
	}

	// save operations

    string _listsave;
    char *listsave(int idx)
    {
		if (idx > 0 && idx <= MAXSAVES)
		{
			sspsave z = saves[idx-1];
			s_sprintf(_listsave)("%d %d %d %d %d %d %d %d %d", z.coins, z.lives, z.power, z.gun, z.shield, z.inventory, z.chkpoint, z.world, z.level);
			return _listsave;
		}
		return "";
    }

	void writesave(int idx, bool reset)
	{
		if (idx > 0 && idx <= MAXSAVES)
		{
			string savename;
			s_sprintf(savename)("data/ssp/ssp%d.sav", idx);
			path(savename);
		
			gzFile f = opengzfile(savename, "wb9");
			
			if (f)
			{
				char head[4];
				
				strncpy(head, "SSPS", 4);
				gzwrite(f, &head, 4);
				gzputint(f, SSPSAVEVER);
				
				if (reset) memcpy(&saves[idx-1], &dsave, sizeof(sspsave));
				else memcpy(&saves[idx-1], &player1->sd, sizeof(sspsave));
				
				sspsave tmp = (sspsave &)saves[idx-1];
				endianswap(&tmp.coins, sizeof(int), NUMSAVE);
				gzwrite(f, &tmp, sizeof(sspsave));
				
				gzclose(f);
				
				conoutf(reset ? "save %s reset" : "game saved to %s", savename);
			}
			else
				conoutf("cannot open %s", savename);
		}
	}

	void loadsave(int idx, bool load)
	{
		nextspawn = -1;
		if (idx > 0 && idx <= MAXSAVES)
		{
			memcpy(&player1->sd, &saves[idx-1], sizeof(sspsave));
			setvar("world", player1->sd.world);
			setvar("level", player1->sd.level);
		}
		else
		{
			memcpy(&player1->sd, &dsave, sizeof(sspsave));

			setvar("world", 0);
			setvar("level", 0);

			if (!load) loadsaves();
		}
		nextspawn = player1->sd.chkpoint;
		if (load) nextmap();
	}

	void loadsaves()
	{
		loopi(MAXSAVES)
		{
			bool ans = false;
			string savename;
			s_sprintf(savename)("data/ssp/ssp%d.sav", i+1);
			path(savename);
			
			gzFile f = opengzfile(savename, "rb9");
		    
		    if (f)
			{
				char head[4];
				int version;
		
				gzread(f, &head, 4);
				version = gzgetint(f);
		
				if (version >= 2 && !strncmp(head, "SSPS", 4))
				{
					if (version <= SSPSAVEVER)
					{
				        gzread(f, &saves[i], sizeof(sspsave));
				        endianswap(&saves[i].coins, sizeof(int), NUMSAVE);
						//conoutf("loaded save %s", savename);
				        ans = true;
					}
					else
						conoutf("save file %s made with incompatible version", savename);
				}
				else
					conoutf("save file %s is corrupt", savename);

				gzclose(f);
			}
			else
				conoutf("cannot open %s", savename);
			
			if (!ans) writesave(i+1, true);
		}
	}

	// input
	void inventory()
	{
		if (getvar("cuttime") <= 0 && player1->state == CS_ALIVE && player1->sd.inventory >= PW_HIT)
		{
			vec w(player1->abovehead());
			vec v(w);
			v.z += player1->eyeheight;

			pr.newprojectile(w, v, player1, et.sspitems[SE_POWERUP].height, et.sspitems[SE_POWERUP].radius, PR_ENT, SE_POWERUP, player1->sd.inventory, 1, 0, 0, 0, INT_MAX-1, getvar("entspeed"), true, true);
			part_spawn(w, vec(et.sspitems[SE_POWERUP].radius, et.sspitems[SE_POWERUP].radius, et.sspitems[SE_POWERUP].height), 0, 2, 100, 500, COL_WHITE);
			
			player1->sd.inventory = -1;
		}
	}

	// world update
	void updatevars()
	{
		setvar("gamemillis",	lastmillis);
		setvar("gametime",		leveltime);
	}

	void updatecamera()
	{
		if (iscamera() != wascamera)
		{
			player1->move = player1->strafe = 0;
			player1->vel = player1->gravity = vec(0, 0, 0);
			
			endtime = 0;
			mapnext[0] = 0;
			
			if (!iscamera() && wascamera)
			{
				et.closestclamp(player1);
				resetgamestate();
			}
			else if (iscamera() && !wascamera)
			{
				player1->o = camera->o; // put player at camera pos
				player1->yaw = camera->yaw;
				player1->pitch = camera->pitch;
				fov = 110; // fix overridden fov
			}
		}
		wascamera = iscamera();
	}

	void update2d(actent *d)
	{
		// force 2d direction movement
		if (isxaxis(d->direction))
		{
			d->o.x = d->path;
			d->vel.x = d->gravity.x = 0;
		}
		else
		{
			d->o.y = d->path;
			d->vel.y = d->gravity.y = 0;
		}
	}
	
	void updatewater(sspent *d, int waterlevel)
	{
		vec v(feetpos(d));
		int mat = getmatvec(v);
		
		if (waterlevel || mat == MAT_WATER || mat == MAT_LAVA)
		{
			if (d->inwater) d->timeinwater += curtime;
			else d->timeinwater = 0;
	
			if (waterlevel)
			{
				uchar col[3] = { 255, 255, 255 };
		
				if (mat == MAT_WATER) getwatercolour(col);
				else if (mat == MAT_LAVA) getlavacolour(col);
		
				int wcol = (col[2] + (col[1] << 8) + (col[0] << 16));
		
				part_spawn(v, vec(d->xradius, d->yradius, ENTPART), 0, 19, 100, 200, wcol);
			}

			if (waterlevel || d->inwater)
			{
				int water = getvar("watertype");
			
				if (waterlevel < 0 && (mat == MAT_WATER && water == WT_WATER))
				{
					sfx(SF_SPLASH1, d != player1 ? &d->o : NULL);
				}
				else if (waterlevel > 0 && (mat == MAT_WATER && water == WT_WATER))
				{
					sfx(SF_SPLASH2, d != player1 ? &d->o : NULL);
				}
				else if ((waterlevel < 0 || d->inwater) && (mat == MAT_WATER && water == WT_HURT))
				{
					if (lastmillis-d->lastpain > getvar("invulnerable"))
					{
						part_spawn(v, vec(d->xradius, d->yradius, ENTPART), 0, 6, 100, 500, COL_WHITE);
						damaged(1, d, d);
					}
				}
				else if (waterlevel < 0 && ((mat == MAT_WATER && water == WT_KILL) || mat == MAT_LAVA))
				{
					part_spawn(v, vec(d->xradius, d->yradius, ENTPART), 0, 6, 100, 500, COL_WHITE);
					//suicide(d);
				}
			}
		}
	}

	void updatejump(sspent *d)
	{
		if (d->special)
		{
			switch (d->sd.shield)
			{
				case SH_ELEC:
				{
					if (d->timeinair <= 0)
						d->special = false;
					break;
				}
				case SH_FIRE:
				{
					if (lastmillis-d->lastspecial > stimes[d->sd.shield])
						d->special = false;
					break;
				}
				default:
				{
					d->special = false;
					break;
				}
			}
		}

		if (d->sd.shield > SH_NONE)
		{
			float eye = d->eyeheight-4.f;
			
			if (d->special)
			{
				switch (d->sd.shield)
				{
					case SH_FIRE:
					{
						part_spawn(d->o, vec(d->xradius, d->yradius, eye), 0, 2, 100, 1000, scol[d->sd.shield]);
						break;
					}
					case SH_ELEC:
					{
						part_spawn(d->o, vec(d->xradius, d->yradius, eye), 0, 2, 20, 200, scol[d->sd.shield]);
						break;
					}
					default: break;
				}
			}
			else if (d->timeinair > 0 && d->jumpnext)
			{
				d->jumpnext = false;
				d->special = true;
				d->lastspecial = lastmillis;
				
				switch (d->sd.shield)
				{
					case SH_ELEC:
					{
						vec v(0, 0, 1);
						v.mul(getvar("jumpvel"));
						v.mul(0.25f);
						d->vel.add(v);
						d->gravity = vec(0, 0, 0);
						part_spawn(d->o, vec(d->xradius, d->yradius, eye), 0, 2, 100, 500, scol[d->sd.shield]);
						break;
					}
					case SH_FIRE:
					{
						if (d->invincible <= 0) d->invincible = stimes[d->sd.shield];
						vec v;
						vecfromyawpitch(diryaw[d->direction], 0, 1, 0, v);
						v.normalize();
						v.mul(getvar("jumpvel"));
						v.mul(0.25f);
						d->vel.add(v);
						d->gravity = vec(0, 0, 0);
						part_spawn(d->o, vec(d->xradius, d->yradius, eye), 0, 2, 100, 500, scol[d->sd.shield]);
						break;
					}
					default: break;
				}
			}
			/*
			else
			{
				switch (d->shield)
				{
					case SH_ELEC:
					{
						loopi(5)
						{
							part_flare(vec(m).add(vec(rnd(int(d->xradius*2))-d->xradius, rnd(int(d->yradius*2))-d->yradius, rnd(int(eye*2))-eye)),
								vec(vec(o).add(vec(0, 0, t.height))).add(vec(rnd(int(t.radius*2))-t.radius, rnd(int(t.radius*2))-t.radius, rnd(int(t.height*2))-t.height)),
								10, 21, scol[SH_ELEC]);
						}
					}
					default:
					{
						part_spawn(d->o, vec(d->xradius, d->yradius, eye), 0, 2, 20, 200, scol[d->sd.shield]);
						break;
					}
				}
			}
			*/
		}
	}

	void updatedynent(sspent *d)
	{
		if (d->state == CS_ALIVE)
		{
			updatewater(d, 0);
			updatejump(d);
            et.checkentities(d);

			// consecutive hits
			if (!d->invincible && !d->timeinair) d->hits = 0;

			d->yaw = diryaw[d->direction];
	       	d->pitch = 0.f;
	       	
	       	if (!d->jumpnext && !d->attacking && !d->move && !d->strafe) d->idle += curtime;
	       	else d->idle = 0;
		}

		if (d->type == ENT_PLAYER) moveplayer((physent *)d, 20, true);
		else moveplayer((physent *)d, 10, false);
	}
	
	bool updatetime()
	{
		if (!iscamera() && !isfinished() &&
			getvar("cuttime") <= 0 && lastmillis-maptime > CARDTIME &&
			player1->state != CS_DEAD && (leveltime += curtime) > 0)
		{
			if (getvar("timelimit") > 0 && leveltime >= getvar("timelimit"))
			{
				damageplayer(player1->sd.power+(player1->sd.shield > SH_NONE ? 1 : 0));
				return false;
			}
		}

		return true;
	}
	
	void updategame()
	{
		if (!updatetime()) resetcut();

		if (getvar("cuttime") > 0) setvar("cuttime", getvar("cuttime") - curtime);
		if (getvar("cuttime") < 0) setvar("cuttime", 0);
	
		if (!iscamera() && getvar("cuttime") > 0)
		{
			player1->jumpnext = false;
			player1->strafe = player1->depth = 0;

			player1->move = !player1->timeinair && player1->direction != DR_NONE ? 1 : 0;

			updatedynent(player1);
		}
		else if (!iscamera() && lastmillis-maptime <= CARDTIME)
		{
			player1->jumpnext = false;
			player1->move = player1->strafe = player1->depth = 0;
			updatedynent(player1);
		}
		else if (isfinished())
		{
			player1->jumpnext = false;
			player1->move = player1->strafe = player1->depth = 0;

			if (endtime > 0)
			{
				if (lastmillis-endtime >= CARDTIME)
				{
					if (mapnext[0]) { load_world(mapnext); mapnext[0] = 0; }
					else { load_world(getdefaultmap()); }
					return;
				}
			}
			else nextmap();
		}
		else if (!iscamera() && player1->state == CS_DEAD)
		{
			if (getvar("cuttime") > 0) resetcut();

			player1->jumpnext = false;
			player1->move = player1->strafe = player1->depth = 0;

			updatedynent(player1);
		}
		else 
		{
			char strafe = player1->strafe, move = player1->move;

			if (!iscamera())
			{
				if (menuactive()) 
				{
					player1->gravity.x = player1->gravity.y = player1->vel.x = player1->vel.y = 0.f;
					player1->move = player1->strafe = player1->depth = 0;
				}
				else
				{
					player1->strafe = 0; // no strafing in ssp

					wp.shoot(player1);
		
					if (isxaxis(player1->direction))
					{
						if (getvar("cameradist") >= 0)
							player1->setdir(strafe ? (strafe > 0 ? DR_LEFT : DR_RIGHT) : player1->direction, false);
						else
							player1->setdir(strafe ? (strafe > 0 ? DR_RIGHT : DR_LEFT) : player1->direction, false);

						player1->depth = move;
						player1->move = (strafe ? 1 : 0);
					}
					else
					{
						//if (getvar("cameradoff") == 0)
						//{
							if (getvar("cameradist") >= 0)
								player1->setdir(move ? (move > 0 ? DR_UP : DR_DOWN) : player1->direction, false);
							else
								player1->setdir(move ? (move > 0 ? DR_DOWN : DR_UP) : player1->direction, false);
	
							player1->depth = strafe;
							player1->move = (move ? 1 : 0);
						//}
						//else
						//{
						//	if (getvar("cameradoff") < 0)
						//		player1->setdir(strafe ? (strafe > 0 ? DR_LEFT : DR_RIGHT) : player1->direction, false);
						//	else
						//		player1->setdir(strafe ? (strafe > 0 ? DR_RIGHT : DR_LEFT) : player1->direction, false);
						//	player1->depth = move;
						//	player1->move = (strafe ? 1 : 0);
						//}
					}
				}
			}
			
			updatedynent(player1);
			
			if (!iscamera())
			{
				player1->move = move;
				player1->strafe = strafe;
			}
		}

		pr.checkprojectiles();

		//if (!iscamera())
		//{
			bl.checkblocks();
			en.checkenemies();
		//}
	}

    void updateworld(vec &pos, int ct, int lm)
    {
		if(!curtime) return;
		
		updatevars();
		updatecamera();
		physicsframe();
		updategame();
    }
    
    void initclient() {}
	
    void edittrigger(const selinfo &sel, int op, int arg1 = 0, int arg2 = 0, int arg3 = 0) {}

	void damageplayer(int damage)
	{
		if (player1->state == CS_ALIVE)
		{
			int max = (player1->sd.shield > SH_NONE ? 4 : 3);
			int mx = (player1->sd.shield > SH_NONE ? player1->sd.power+1 : player1->sd.power);
	        int dam = (damage > max ? mx : damage);
	        
	        if (dam > 0)
	        {
				int coins = player1->sd.coins > 10 ? rnd(player1->sd.coins) : player1->sd.coins; // throw out coins, kinda like sonic
				if (coins > 0)
				{
					et.spawnprojents(player1, player1->o, player1->yradius/4.f, player1->eyeheight, ENTPART, SE_COIN, 1, 0, 0, 0, 0, coins);
					player1->sd.coins -= coins;
				}
	        }
	        
	        if (dam > 0 && player1->sd.shield > SH_NONE)
	        {
	        	player1->sd.shield = SH_NONE;
	        	dam--;
	        }

			if (dam > 0) player1->setpower(player1->sd.power - dam);

			player1->lastpain = lastmillis;
			
	        if (player1->sd.power <= 0)
	        {
				sfx(SF_DIE1+rnd(SF_DIE2-SF_DIE1+1));
	            
	            player1->sd.power = 0;
	            player1->attacking = false;
	            player1->state = CS_DEAD;
	            player1->sd.lives--;
	            player1->pitch = player1->roll = 0;
	            
				vec vel = player1->vel;
	            player1->reset();
	            player1->vel = vel;
	            player1->lastaction = lastmillis;
			}
	        else
	        {
	            sfx(SF_PAIN1+rnd(SF_PAIN6-SF_PAIN1+1));
	        }
		}
	}
	
	void damaged(int damage, sspent *d, sspent *actor)
	{
		sspent *e = (sspent *)d;
        if (e == player1) damageplayer(damage);
        else if (e->type == ENT_AI) en.damageenemy(e, damage);
        else if (e->type == ENT_BOUNCE) ((projent *)d)->lifetime = 0;
	}

	void suicide(physent *d)
	{
		sspent *e = (sspent *)d;
        if (e == player1) damageplayer(e->sd.power+1);
        else if (e->type == ENT_AI) en.damageenemy(e, e->sd.power+1);
        else if (e->type == ENT_BOUNCE) ((projent *)d)->lifetime = 0;
	}
    
	void newmap(int size) {}

	void sfx(int idx, const vec *o = NULL)
	{
		playsound(idx, o);
	}
	
    // those of class sspent only
    int numdynents() { return 1+bl.blocks.length()+en.enemies.length(); }
    dynent *iterdynents(int i)
	{
		if (!i) { return player1; } i--;
		if (i < bl.blocks.length()) { return bl.blocks[i]; } i -= bl.blocks.length();
		if (i < en.enemies.length()) { return en.enemies[i]; }// i -= en.enemies.length();
		return NULL;
	}

	// world operations

	void resetgame(bool restart)
	{
	    leveltime = endtime = 0;
	    mapnext[0] = 0;
	    resetcut();

		if (restart) et.spawnents();
	}

    void resetgamestate()
	{
		resetgame(true);
		player1->setposition(player1->direction, player1->clamp, player1->o.y, player1->o.z);
	}
    
	void resetcut()
	{
		setvar("cuttime", 0);
		setvar("cuthold", 0);
	}

	void reset()
	{
		maptime = lastmillis;
		wascamera = iscamera();

	    resetgame(false);
		et.spawnplayer(false);
	}

	void startmap(const char *name)
    {
		reset();
		if (identexists("map_music")) execute("map_music");
		prerender(); // prerender to avoid stutter
		show_out_of_renderloop_progress(0.f, "starting level..");
    }
    
    void doattack(bool on)
	{
        if (isfinished() || getvar("cuttime") > 0) return;

		if (player1->attacking = on)
		{
			if (player1->state == CS_DEAD)
			{
				player1->attacking = false;
				et.spawnplayer(true);
			}
			else if (player1->sd.power <= 1)
			{
				player1->attacking = false;
			}
		}
	}

    void writegamedata(vector<char> &extras) {}
    void readgamedata(vector<char> &extras) {}

	void getworld(const char *name)
	{
		loopi(getvar("worldamt") + 1)
		{
			string sa;
			s_sprintf(sa)("map_%d_num", i);
		
			const char *maps = getalias(sa);
			if (*maps)
			{
				loopj(atoi(maps))
				{
					string sb;
					s_sprintf(sb)("map_%d_%d", i, j+1);
					const char *map = getalias(sb);
					if (*map && !strcmp(map, name))
					{
						setvar("world", i);
						setvar("level", j+1);
						return;
					}
				}
			}
		}
	}

	void loadworld(const char *name)
	{
		show_out_of_renderloop_progress(0.f, "loading world");

		player1->sd.chkpoint = -1;

		setvar("world", 0);
		setvar("level", 0);

		maptext[0] = 0;

		if (*name)
		{
			getworld(name);

			char *defname = getdefaultmap();
			if (!strcmp(mapname, defname))
			{
				loadsave(0, false);
				player1->sd.power = 2; // hack
			}
			else
			{
				if ((getvar("level") > player1->sd.level) || (getvar("world") > player1->sd.world))
				{
					player1->sd.level = getvar("level");
					player1->sd.world = getvar("world");
				}
			}
		}
		else loadsave(0, false);
	}
	void saveworld(const char *name) {}
};

REGISTERGAME(sspgame, "ssp", new sspclient(), new sspdummyserver());
#endif

