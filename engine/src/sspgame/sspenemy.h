// Blood Frontier - SSPGAME - Side Scrolling Platformer by Quinton Reeves
// This is the SSP enemies

struct sspenemy
{
	sspclient &cl;

	vector<enemyent *> enemies;
	enemyent *enemy;

	sspenemy(sspclient &_cl) : cl(_cl), enemy(NULL)
	{
		// Variables
		#define enemyvariable(a,b,c,d) \
			CVARF(sspenemy, a, b, 0, c, if (self->enemy != NULL) { d; });

		enemyvariable(ev_aboveeye,	0,			INT_MAX-1,	self->enemy->aboveeye = (float)*_storage);
		enemyvariable(ev_attack,	0,			1,			self->enemy->attacking = *_storage ? true : false);
		enemyvariable(ev_action,	0,			INT_MAX-1,	self->enemy->lastaction = *_storage);
		enemyvariable(ev_blocked,	0,			1,			self->enemy->blocked = *_storage ? true : false);
		enemyvariable(ev_coins,		0,			INT_MAX-1,	self->enemy->sd.coins = *_storage);
		enemyvariable(ev_depth,		-1,			1,			self->enemy->depth = *_storage);
		enemyvariable(ev_dir,		DR_LEFT,	DR_DOWN,	self->enemy->setdir(*_storage, false));
		enemyvariable(ev_eyeheight,	0,			INT_MAX-1,	self->enemy->eyeheight = (float)*_storage);
		enemyvariable(ev_gun,		-1,			GN_MAX-1,	self->enemy->sd.gun = *_storage);
		enemyvariable(ev_gunwait,	0,			INT_MAX-1,	self->enemy->gunwait = *_storage);
		enemyvariable(ev_hits,		0,			INT_MAX-1,	self->enemy->hits = *_storage);
		enemyvariable(ev_id,		0,			INT_MAX-1,
			s_sprintfd(s)("en_%d_%d_init", self->enemy->enemytype, *_storage);
			if (identexists(s)) self->enemy->enemyid = *_storage;
		);
		enemyvariable(ev_inair,		0,			INT_MAX-1,	self->enemy->timeinair = *_storage);
		enemyvariable(ev_index,		0,			-1,			return); // read-only
		enemyvariable(ev_invincible,0,			INT_MAX-1,	self->enemy->invincible = *_storage);
		enemyvariable(ev_inwater,	0,			INT_MAX-1,	self->enemy->timeinwater = *_storage);
		enemyvariable(ev_jump,		0,			INT_MAX-1,	self->enemy->jumpnext = *_storage ? true : false);
		enemyvariable(ev_lives,		-1,			INT_MAX-1,	self->enemy->sd.lives = *_storage);
		enemyvariable(ev_move,		-1,			1,			self->enemy->move = *_storage);
		enemyvariable(ev_pain,		0,			INT_MAX-1,	self->enemy->lastpain = *_storage);
		enemyvariable(ev_pitch,		0,			360,		self->enemy->pitch = (float)*_storage);
		enemyvariable(ev_power,		0,			INT_MAX-1,	self->enemy->setpower(*_storage));
		enemyvariable(ev_radius,	0,			INT_MAX-1,	self->enemy->radius = self->enemy->xradius = self->enemy->yradius = (float)*_storage);
		enemyvariable(ev_roll,		0,			360,		self->enemy->roll = (float)*_storage);
		enemyvariable(ev_shield,	SH_NONE,	SH_MAX-1,	self->enemy->sd.shield = *_storage);
		enemyvariable(ev_speed,		0,			INT_MAX-1,	self->enemy->maxspeed = (float)*_storage);
		enemyvariable(ev_state,		CS_ALIVE,	CS_LAGGED,	self->enemy->state = *_storage);
		enemyvariable(ev_strafe,	-1,			1,			self->enemy->strafe = *_storage);
		enemyvariable(ev_type,		0,			INT_MAX-1,
			s_sprintfd(s)("en_%d_%d_init", *_storage, self->enemy->enemyid);
			if (identexists(s)) self->enemy->enemytype = *_storage;
		);
		enemyvariable(ev_weight,	0,			INT_MAX-1,	self->enemy->weight = *_storage);
		enemyvariable(ev_xradius,	0,			INT_MAX-1,	self->enemy->xradius = (float)*_storage);
		enemyvariable(ev_yaw,		0,			360,		self->enemy->yaw = (float)*_storage);
		enemyvariable(ev_yradius,	0,			INT_MAX-1,	self->enemy->yradius = (float)*_storage);

		// Commands
		#define enemyscript(a,b,c) \
			CCOMMAND(sspenemy, a, b, if (self->enemy != NULL) { c; });

		enemyscript(ec_name,	"s",	s_strcpy(self->enemy->enemyname, args[0]));
		enemyscript(ec_model,	"s",	s_strcpy(self->enemy->enemymodel, args[0]); self->enemy->setbounds(self->enemy->enemymodel));
		enemyscript(ec_sound,	"si",	playsoundname(args[0], &self->enemy->o, atoi(args[1])));
		enemyscript(ec_vwep,	"s",	s_strcpy(self->enemy->enemyvwep, args[0]));

		enemyscript(ec_face,	"i",	{ self->faceenemy(self->enemy, atoi(args[0])); setvar("ev_dir", self->enemy->direction); });

		CCOMMAND(sspenemy, ec_context, "is", self->contextenemy(atoi(args[0]), args[1]));
	}
	~sspenemy() { }

	void varenemy(enemyent *d)
	{
		setvar("ev_aboveeye",		(int)d->aboveeye);
		setvar("ev_action",			d->lastaction);
		setvar("ev_attack",			d->attacking ? 1 : 0);
		setvar("ev_blocked",		d->blocked ? 1 : 0);
		setvar("ev_coins",			d->sd.coins);
		setvar("ev_depth",			d->depth);
		setvar("ev_dir",			d->direction);
		setvar("ev_eyeheight",		(int)d->eyeheight);
		setvar("ev_gun",			d->sd.gun);
		setvar("ev_gunwait",		d->gunwait);
		setvar("ev_hits",			d->hits);
		setvar("ev_id",				d->enemyid);
		setvar("ev_inair",			d->timeinair);
		setvar("ev_index",			d->enemyindex);
		setvar("ev_invincible",		d->invincible);
		setvar("ev_inwater",		d->timeinwater);
		setvar("ev_jump",			d->jumpnext ? 1 : 0);
		setvar("ev_lives",			d->sd.lives);
		setvar("ev_move",			d->move);
		setvar("ev_pain",			d->lastpain);
		setvar("ev_pitch",			(int)d->pitch);
		setvar("ev_power",			d->sd.power);
		setvar("ev_radius",			(int)d->radius);
		setvar("ev_roll",			(int)d->roll);
		setvar("ev_shield",			d->sd.shield);
		setvar("ev_speed",			(int)d->maxspeed);
		setvar("ev_state",			d->state);
		setvar("ev_strafe",			d->strafe);
		setvar("ev_type",			d->enemytype);
		setvar("ev_weight",			d->weight);
		setvar("ev_xradius",		(int)d->xradius);
		setvar("ev_yaw",			(int)d->yaw);
		setvar("ev_yradius",		(int)d->yradius);
	}

	void runenemy(enemyent *d, char *script)
	{
		if ((enemy = d) != NULL)
		{
			s_sprintfd(s)("en_%d_%d_%s", enemy->enemytype, enemy->enemyid, script);
			if (identexists(s))
			{
				varenemy(enemy);
				execute(s);
			}
			enemy = NULL;
		}
	}

	void contextenemy(int a, char *b)
	{
		if (enemies.inrange(a) && ((enemy = enemies[a]) != NULL))
		{
			varenemy(enemy);
			execute(b);
			enemy = NULL;
		}
		else conoutf("invalid enemy index %d", a);
	}

	void addenemy(sspentity &e)
	{
		enemyent *d = new enemyent;

		d->o = d->enemypos = e.o; // for respawning
		d->enemytype = e.attr1;
		d->enemyid = e.attr2;
		d->direction = d->enemydir = e.attr3;
		d->depth = d->move = d->strafe = d->sd.coins = d->sd.lives = 0;
		d->sd.lives = d->sd.gun = -1;
		d->rotspeed = 0;

		d->enemyindex = enemies.length();
		enemies.add(d);

		d->o.z += d->eyeheight;
		d->enemyreset();
		d->state = CS_DEAD;

		runenemy(d, "init");
	}

	void faceenemy(enemyent *d, int type)
	{
		switch (type)
		{
			case 1: // player
			{
				if (cl.player1->state == CS_ALIVE)
				{
					vec v(cl.player1->o);
					v.sub(d->o);
					
					if (fabs(v.x) > fabs(v.y))
					{
						if (v.x > 0) d->setdir(DR_UP, false);
						else d->setdir(DR_DOWN, false);
					}
					else
					{
						if (v.y > 0) d->setdir(DR_RIGHT, false);
						else d->setdir(DR_LEFT, false);
					}
				}
				break;
			}
			default:
				break;
		}
	}

	void damageenemy(sspent *d, int damage)
	{
		if (d->state == CS_ALIVE)
		{
			int max = (d->sd.shield > SH_NONE ? 4 : 3);
			int mx = (d->sd.shield > SH_NONE ? d->sd.power+1 : d->sd.power);
	        int dam = (damage > max ? mx : damage);
	        
	        if (dam > 0 && d->sd.shield > SH_NONE)
	        {
	        	d->sd.shield = SH_NONE;
	        	dam--;
	        }

			if (dam > 0) d->setpower(d->sd.power - dam);

			d->lastpain = lastmillis;

			if (d->sd.power <= 0)
			{
				if (d->sd.coins > 0) // enemies only when dead
				{
					cl.et.spawnprojents(d, d->o, d->yradius/4.f, d->eyeheight, ENTPART, SE_COIN, 1, 0, 0, 0, 0, d->sd.coins);
				}

				d->attacking = false;
				((enemyent *)d)->enemyawake = false;
				d->state = CS_DEAD;
				d->sd.lives--;
				d->pitch = d->roll = 0;

				vec vel = d->vel;
				d->reset();
				d->vel = vel;
				d->lastaction = lastmillis;

				runenemy((enemyent *)d, "dead");
			}
			else
			{
				runenemy((enemyent *)d, "hurt");
			}
		}
	}

	void clearenemies()
	{
		enemies.deletecontentsp();
		enemies.setsize(0);
	}

	bool edgeenemy(enemyent *d)
	{
		vec q(dirvec[d->direction]), v(d->o);
		bool ans = false;

		q.mul(d->radius);
		q.add(d->o);
		q.z -= 1.f;
		
		d->o = q;
		
		if (collide(d)) ans = true;

		d->o = v;
		
		return ans;
	}

	void updateenemy(enemyent *d)
	{
		sspent *at = cl.player1;
		
		if (at->state == CS_ALIVE)
		{
			bool inair = d->timeinair > 0;//, jump = d->jumpnext;
		
			if (d->jumpnext && !d->timeinair)
			{
				d->enemyjump = lastmillis;
				d->enemyjumped = true;
				runenemy(d, "jump");
			}
		
			cl.wp.shoot(d);
			if (at->state == CS_ALIVE) cl.wp.clobber(d, at);
			cl.updatedynent(d);
		
			if (d->blocked) { d->blocked = false; runenemy(d, "blck"); }
			if (!inair && !d->timeinair && edgeenemy(d)) { runenemy(d, "edge"); }
			if (inair && !d->timeinair)
			{
				if (d->enemyjumped)
				{
					d->enemyjumped = false;
					runenemy(d, "land");
				}
				else runenemy(d, "fell");
			}
		
			runenemy(d, "thnk");
		}
	}

	void deadenemy(enemyent *d)
	{
		if (d->state == CS_DEAD && lastmillis-d->lastaction <= getvar("enttime"))
		{
			d->move = d->strafe = 0;

			d->o.z -= (lastmillis-d->lastaction)*0.00005f;
			d->o.x += (lastmillis-d->lastaction)*0.001f;

			d->yaw += (lastmillis-d->lastaction)*0.005f;
			if (d->yaw > 360.f) d->yaw -= 360.f;

			cl.updatedynent(d);
		}
		else if (d->sd.lives > 0 || d->sd.lives < 0) // less than 0 will respawn forever
		{
			sspent *at = cl.player1;
			
			if (d->state == CS_DEAD)
			{
				d->enemyreset();
				d->setposition(d->enemydir, d->enemypos.x, d->enemypos.y, d->enemypos.z);
			}

			if (at->state == CS_ALIVE)
			{
				vec target, p(at->o.x, at->o.y, at->o.z);

				//if (p.dist(d->o) <= getvar("enemylos") && getsight((physent *)d, p, target, getvar("enemylos"), 96, 128))
				if (getsight((physent *)d, p, target, float(getvar("enemylos")), 90.f, 90.f))
				{
					d->enemyawake = true;
					runenemy(d, "wake");
				}
			}
			
			cl.updatedynent(d);
		}
	}

	void checkenemies()
	{
		loopv(enemies)
		{
			enemyent *d = enemies[i];

			if (d != NULL)
			{
				if (d->state == CS_ALIVE && d->enemyawake)
				{
					updateenemy(d);
				}
				else if (d->state == CS_DEAD || !d->enemyawake)
				{
					deadenemy(d);
				}
			}
		}
	}

	void renderenemies()
	{
		loopv(enemies)
		{
			enemyent *d = enemies[i];

			if (d != NULL && d->enemymodel[0] && (cl.iscamera() || d->state != CS_DEAD || lastmillis-d->lastaction < getvar("enttime")))
			{
				if (d->sd.gun >= 0) cl.rendersspact(d, d->enemymodel, d->enemyvwep[0] ? d->enemyvwep : NULL, NULL, d->invincible ? "quadspheres" : NULL, lastmillis-d->lastaction < cl.wp.reloadtime(d->sd.gun) ? -ANIM_SHOOT : 0, cl.wp.reloadtime(d->sd.gun));
				else cl.rendersspact(d, d->enemymodel, NULL, NULL, d->invincible ? "quadspheres" : NULL, 0, 0);

				if (editmode)
				{
					part_textf(d->abovehead(), "%s", false, 0, COL_LBLUE, d->enemyname);
				}
			}
		}
	}
};
