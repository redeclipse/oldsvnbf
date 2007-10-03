// Blood Frontier - BOT - Offline test and practice AI by Quinton Reeves
// Included by: fpsgame/bot/bot.h
// This is a set of helpful macros used throughout the bot code.

bool isbot(fpsent *d)
{ 
	return d != NULL && d->botstate != M_NONE;
}

void reset(fpsent *d, bool soft = false)
{
	d->bottarg = -1;
	d->botvec = vec(0, 0, 0);
	d->botroute.setsize(0);
	
	if (!soft)
	{
		d->botnode = -1;
		d->botjump = 0;
		d->botenemy = NULL;
	}
}

void spawn(fpsent *d)
{ 
	if (isbot(d))
	{
		int gamemode = cl.gamemode;

		reset(d);
		d->botupdate = d->botrate*10;
		
		if (d->botflags & BOT_PLAYER)
		{
			if (d->state != CS_ALIVE)
			{
				if (m_classicsp) d->o = cl.player1->o;
				else findplayerspawn(d, -1); // TODO: pick properly

				cl.spawnstate(d);
				d->state = CS_ALIVE;
				entinmap(d);
				wayposition(d);

				playsound(S_RESPAWN, &d->o);

				d->botstate = M_SLEEP;
			}
		}
		else if (d->botflags & BOT_MONSTER)
		{
			if (d->state == CS_ALIVE)
			{
				bool hurt = d->health < d->maxhealth;
				float dist = 1e16f, angle = 1e16f;
				
				if (!hurt) find(d, true); // let botfind take care of botenemy

				if (d->botenemy)
				{
					dist = d->botenemy->o.dist(d->o);
					float enemyyaw = -(float)atan2(d->botenemy->o.x - d->o.x, d->botenemy->o.y - d->o.y) / RAD + 180;
					d->normalize_yaw(enemyyaw);
					angle = (float)fabs(enemyyaw - d->yaw);
				}

				if (hurt || (d->botenemy && (dist < 32 || (dist < 64 && angle < 135) || (dist < 128 && angle < 90) || (dist < 256 && angle < 45) || angle < 10)))
				{
					vec target;

					if (hurt || raycubelos(d->o, d->botenemy->o, target))
					{
						d->botstate = M_SLEEP;
						playsound(S_GRUNT1 + rnd(2), &d->o);
						wayposition(d);
					}
				}
			}
		}
	}
}

void frags(fpsent *d, int incr) // this is used to fake frag counts
{ 
	s_sprintfd(ds)("@%d", d->frags += incr);
	particle_text(d->abovehead(), ds, 9);

	if (d->botflags & BOT_PLAYER)
	{
		fpsserver::clientinfo *c = (fpsserver::clientinfo *)getinfo(d->clientnum);
		if (c) c->state.frags = d->frags;
		cl.calcranks(); // update ranks
	}
}

void killed(fpsent *d)
{ 
	if (isbot(d))
	{
		d->attacking = false;

		d->deaths++;
		d->pitch = 0;
		d->roll = 0;
		cl.ws.superdamageeffect(d->vel, d);

		if (d->botflags & BOT_MONSTER)
			cl.ms.monsterkilled();

		trans(d, M_PAIN, 0, 0, false, 2500); // TODO: canrespawn
		
		reset(d);
	}
}


void damaged(int damage, fpsent *d, fpsent *act)
{ 
	if (isbot(d))
	{
		d->damageroll(damage);
		if (d != act && act != d->botenemy)
			enemy(d, act); // keep going about their business
	}
}

bool aimat(fpsent *d, bool force = false)
{ 
	bool result = false;

	if ((d->botstate != M_SEARCH && d->botstate != M_PAIN) || force)
	{
		extern int curtime;
		float bk = (curtime/10.f)*float(101-d->botrate);
		bool aim = d->botenemy && (d->botstate == M_AIMING || d->botstate == M_ATTACKING);
		vec h(vec(0, 0, aim ? d->botenemy->eyeheight / 2 : d->eyeheight));
		vec v(aim ? vec(d->botenemy->o).sub(h) : vec(d->botvec).add(h));

		d->botrot.y = -(float)atan2(v.x - d->o.x, v.y - d->o.y) / PI * 180 + 180;

		if (d->yaw < d->botrot.y - 180.0f) d->yaw += 360.0f;
		if (d->yaw > d->botrot.y + 180.0f) d->yaw -= 360.0f;

		float dist = d->o.dist(v);

		d->botrot.x = asin((v.z - d->o.z) / dist) / RAD;

		if (d->botrot.x > 90.f) d->botrot.x = 90.f;
		if (d->botrot.x < -90.f) d->botrot.x = -90.f;

		if (force)
		{
			d->yaw = d->botrot.y;
			d->pitch = d->botrot.x;
		}
		else
		{
			#define bkcoord(a,b) \
				if (a > b) \
				{ \
					b += bk; \
					if (a < b) b = a; \
					else result = true; \
				} \
				else if (a < b) \
				{ \
					b -= bk; \
					if (a > b) b = a; \
					else result = true; \
				}
			
			bkcoord(d->botrot.y, d->yaw);
			bkcoord(d->botrot.x, d->pitch);
		}
	}
	vec dir(0, 0, 0);
	vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);

	if (raycubepos(d->o, dir, d->botpos, 0, RAY_CLIPMAT | RAY_SKIPFIRST) == -1)
		d->botpos = dir.mul(10).add(d->o);

	return result;
}

void target(fpsent *d)
{ 
	vec ray = d->botpos;
	ray.sub(d->o);
	float mag = ray.magnitude();
	raycubepos(d->o, ray, d->botpos, mag, RAY_CLIPMAT | RAY_POLY);
}

void weapon(fpsent *d, bool push = false)
{ 
	if (isbot(d))
	{
		int s = d->gunselect;

		if (d->botflags & BOT_PLAYER)
		{
			int *ammo = d->ammo;

			if (push)
			{
				if (ammo[GUN_RL])			s = GUN_RL;
				else if (ammo[GUN_GL])		s = GUN_GL;
				if (ammo[GUN_RIFLE])		s = GUN_RIFLE;
				else if (ammo[GUN_SG])		s = GUN_SG;
				else if (ammo[GUN_CG])		s = GUN_CG;
				else if (ammo[GUN_PISTOL])	s = GUN_PISTOL;
				else						s = GUN_FIST;
			}
			else
			{
				if (ammo[GUN_RL] && d->botvec.dist(d->o) >= BOTRADIALDIST)			s = GUN_RL;
				else if (ammo[GUN_GL] && d->botvec.dist(d->o) >= BOTRADIALDIST)		s = GUN_GL;
				else if (ammo[GUN_CG] && d->botvec.dist(d->o) >= BOTMELEEDIST)		s = GUN_CG;
				else if (ammo[GUN_SG] && d->botvec.dist(d->o) >= BOTMELEEDIST)		s = GUN_SG;
				else if (ammo[GUN_RIFLE] && d->botvec.dist(d->o) >= BOTMELEEDIST)	s = GUN_RIFLE;
				else if (ammo[GUN_PISTOL] && d->botvec.dist(d->o) >= BOTMELEEDIST)	s = GUN_PISTOL;
				else																s = GUN_FIST;
			}

			if (s != d->gunselect)
			{
				playsound(S_WEAPLOAD, &d->o);
				d->gunselect = s;
			}
		}
		else if (d->botflags & BOT_MONSTER)
		{
			fpsclient::monsterset::monster *e = (fpsclient::monsterset::monster *)d;

			if (push || d->botvec.dist(d->o) >= BOTMELEEDIST) s = cl.ms.monstertypes[e->mtype].gun;
			else if (e->gunselect != GUN_BITE) s = GUN_FIST;

			if (s != d->gunselect) d->gunselect = s;
		}
	}
}

void shoot(fpsent *d, bool push = false)
{
	d->attacking = true;
	cl.ws.shoot(d, d->botpos);
	d->attacking = false;
	
	if (!push)
	{
		if (d->botflags & BOT_MONSTER) gunvar(d->gunwait,d->gunselect) += d->botrate * 10;
		else if (d->gunselect != GUN_CG) gunvar(d->gunwait,d->gunselect) += d->botrate;
	}
}

void checkents(fpsent *d)
{ 
	loopv(ents) // equivalent of player entity touch, but only teleports are used
	{
		entity &e = *ents[i];
		if (e.type != TELEPORT && e.type != JUMPPAD) continue;
		vec v = e.o;
		v.z += d->eyeheight;
		float dist = v.dist(d->o);
		v.z -= d->eyeheight;
		if (dist < (e.type == TELEPORT ? 16 : 12)) cl.et.trypickup(i, d);
	}
}

void pickup(int n, fpsent *d)
{ 
	if (d->botflags & BOT_PLAYER)
	{
		int type = ents[n]->type;
		if (type < I_SHELLS || type > I_QUAD)
			return ;

		cl.et.pickupeffects(n, d);
		d->pickup(type);
		// TODO: send message to server
	}
}

void renderstate(fpsent *d)
{ 
	vec v(d->abovehead());
	string s;

	if (d->botstate >= M_NONE && d->botstate <= M_AIMING)
	{
		char *bstate[M_AIMING + 1] =
			{ "none", "searching", "homing", "attacking", "pain", "sleep", "aiming" };

		v.z += 2.f;

		s_sprintf(s)("@%s: %dms", bstate[d->botstate], d->botupdate);
		particle_text(v, s, 14, 1);

		if (d->botenemy != NULL && *d->botenemy->name)
		{
			v.z += 2.f;

			s_sprintf(s)("@[%s]", d->botenemy->name);
			particle_text(v, s, 14, 1);
		}
	}
}

void rendercoord(fpsent *d)
{ 
	vec v(d->o);
	v.z -= d->eyeheight;

	if (d->botvec != vec(0, 0, 0))
		renderline(d->o, d->botvec, 1.f, 1.f, 1.f);

	if (cl.et.ents.inrange(d->bottarg))
		renderline(v, cl.et.ents[d->bottarg]->o, 0.0f, 1.f, 0.0f);
	if (cl.et.ents.inrange(d->botnode))
		renderline(v, cl.et.ents[d->botnode]->o, 1.f, 1.f, 0.0f);

	loopvj(d->botroute)
	{
		if (j > 0)
			renderline(vec(cl.et.ents[d->botroute[j - 1]]->o).add(vec(0, 0, 1.f)), vec(cl.et.ents[d->botroute[j]]->o).add(vec(0, 0, 1.f)), 0.5f, 0.5f, 0.5f, true);
	}
}

void render()
{ 
#if 0
	if (editmode || cl.et.showallwp())
	{
		loopv(cl.players)
		{
			if (cl.players.inrange(i) && isbot(cl.players[i]) && cl.players[i]->state == CS_ALIVE)
			{
				renderstate(cl.players[i]);
			}
		}
		loopv(cl.ms.monsters)
		{
			if (cl.ms.monsters.inrange(i) && isbot((fpsent *)cl.ms.monsters[i]) && cl.ms.monsters[i]->state == CS_ALIVE)
			{
				renderstate((fpsent *)cl.ms.monsters[i]);
			}
		}
	}
#endif
	if (editmode || cl.et.showallwp())
	{
		renderprimitive(true);

		loopv(cl.players)
		{
			if (cl.players.inrange(i) && isbot(cl.players[i]) && cl.players[i]->state == CS_ALIVE)
			{
				rendercoord(cl.players[i]);
			}
		}

		loopv(cl.ms.monsters)
		{
			if (cl.ms.monsters.inrange(i) && isbot((fpsent *)cl.ms.monsters[i]) && cl.ms.monsters[i]->state == CS_ALIVE)
			{
				rendercoord((fpsent *)cl.ms.monsters[i]);
			}
		}
		renderprimitive(false);
	}
}
