// Blood Frontier - SSPGAME - Side Scrolling Platformer by Quinton Reeves
// This is the SSP weapon states.

struct sspweapons
{
	sspclient &cl;

	struct guninfo
	{
		char *name;
		int sound, attackdelay, projspeed, part, col, kickamount;
	};

	vec sg[SGRAYS];

	guninfo *guns;

	sspweapons(sspclient &_cl) : cl(_cl)
	{
		static guninfo _guns[GN_MAX] =
		{
			{ "fist",		SF_PUNCH,		500,	0,		-1,	0,				1 },
			{ "shotgun",	SF_SG,			1500,	0,		-1,	0,				20 },
			{ "chaingun",	SF_CG,			250,	0,		-1,	0,				5 },
			{ "rockets",	SF_RL,			1000,	160,	-1,	0,				15 },
			{ "rifle",		SF_RIFLE,		1500,	0,		-1,	0,				25 },
			{ "grenades",	SF_GL,			1000,	140,	-1,	0,				10 },
			{ "bombs",		SF_BOMB,		1500,	100,	-1,	0,				5 },
			{ "fireball",	SF_FIRE,		500,	120,	5,	COL_RED,		5 },
			{ "iceball",	SF_ICE,			500,	100,	7,	COL_WHITE,		5 },
			{ "slimeball",	SF_SLIME,		500,	80,		8,	COL_WHITE,		5 },
		};
		guns = _guns;
	}
	~sspweapons() { }

    int reloadtime(int gun) { return guns[gun].attackdelay; }

	void offsetray(vec &from, vec &to, int spread, vec &dest)
	{
		float f = to.dist(from)*spread/1000;
		for(;;)
		{
			#define RNDD rnd(101)-50
			vec v(RNDD, RNDD, RNDD);
			if(v.magnitude()>50) continue;
			v.mul(f);
			v.z /= 2;
			dest = to;
			dest.add(v);
			vec dir = dest;
			dir.sub(from);
			raycubepos(from, dir, dest, 0, RAY_CLIPMAT|RAY_POLY);
			return;
		}
	}

	void createrays(vec &from, vec &to)			 // create random spread of rays for the shotgun
	{
		loopi(SGRAYS)
		{
			offsetray(from, to, SGSPREAD, sg[i]);
		}
	}

	void damageeffect(int damage, sspent *d)
	{
		vec p = d->o;
		p.z += 0.6f*(d->eyeheight + d->aboveeye) - d->eyeheight;
		part_splash(4, damage*5, 1000, p, COL_BLOOD);
	}

	void hit(int damage, sspent *d, sspent *at, vec &vel)
	{
		if (d->state == CS_ALIVE && d->invincible <= 0)
		{
			vec v(vel);
			v.mul(damage*10);

			if (d->type == ENT_PLAYER)
			{
				if (lastmillis-d->lastpain <= getvar("invulnerable")) return;

				d->vel.add(v);
				cl.damageplayer(damage);
			}
			else if (d->type == ENT_AI)
			{
				d->vel.add(v);
				cl.en.damageenemy(d, damage);

				if (at->type == ENT_PLAYER && d != at)
				{
					cl.sfx(SF_HIT1+(at->hits <= 6 && at->hits >= 0 ? at->hits : 7), at != cl.player1 ? &at->o : NULL);
					at->hits++;
					if (at->hits >= 8) cl.et.pickuplife(at, at->o, 1);
				}
			}
			else if (d->type == ENT_BLOCK)
			{
				cl.bl.damageblock((blockent *)d, at, vel);
			}
			
			if (d->type != ENT_BLOCK)
			{
				damageeffect(damage, d);
				if(d->state == CS_DEAD) cl.pr.superdamageeffect(d->vel, d);
			}
		}
	}

	void hitpush(int damage, sspent *d, sspent *at, vec &from, vec &to)
	{
		vec v(to);
		v.sub(from);
		v.normalize();
		hit(damage, d, at, v);
	}

	void clobber(sspent *d, sspent *at)
	{
		if (d->state == CS_ALIVE && at->state == CS_ALIVE)
		{
			if (!d->o.reject(at->o, d->radius+at->radius))
			{
				float z = at->o.z-at->eyeheight;

				if (z >= d->o.z-HITCLOB && z <= d->o.z+HITCLOB)
				{
					hitpush(1, d, at, at->o, d->o);
					at->vel.z = getvar("clobberzvel")*(at->jumpnext ? 2.f : 1.f); // proj higher
					at->lastaction = lastmillis;
					at->jumpnext = false;
					at->gravity.z = 0.f;
				}
				else if (at->o.z >= d->o.z-d->eyeheight-HITCLOB && z <= d->o.z-HITCLOB)
				{
					if (at->invincible > 0)
					{
						hitpush(1, d, at, at->o, d->o);
						at->lastaction = lastmillis;
						d->blocked = true;
					}
					else
					{
						hitpush(1, at, d, d->o, at->o);
						d->lastaction = lastmillis;
						d->blocked = true;
					}
				}
			}
		}
	}

	vec gunorigin(int gun, const vec &from, const vec &to, sspent *d)
	{
		if(d != cl.player1) return from;
		vec offset(from);
		offset.add(vec(to).sub(from).normalize().mul(6));
		return offset;
	}

	void shootv(int gun, vec &from, vec &to, sspent *d)	 // create visual effect from a shot
	{
		cl.sfx(guns[gun].sound, d != cl.player1 ? &d->o : NULL);
		vec behind = vec(from).sub(to).normalize().mul(4).add(from);
		switch(gun)
		{
			case GN_FIST:
				break;

			case GN_SG:
			{
				loopi(SGRAYS)
				{
					part_splash(2, 20, 250, sg[i], COL_YELLOW);
					part_flare(gunorigin(gun, behind, sg[i], d), sg[i], 300, 10, COL_YELLOW);
				}
				break;
			}

			case GN_CG:
			{
				part_splash(2, 100, 250, to, COL_YELLOW);
				part_flare(gunorigin(gun, behind, to, d), to, 600, 10, COL_YELLOW);
				break;
			}

			case GN_RL:
			case GN_FIREBALL:
			case GN_ICEBALL:
			case GN_SLIMEBALL:
				cl.pr.newprojectile(from, to, d, 0.5f, 0.5f, PR_PROJ, -1, 0, 0, 0, 0, 0, getvar("projtime"), guns[gun].projspeed, true, true, gun);
				break;

			case GN_GL:
			case GN_BOMB:
			{
				float dist = from.dist(to);
				vec up = to;
				up.z += dist/8;
				cl.pr.newprojectile(from, up, d, 1.5f, 1.5f, PR_BNC, -1, 0, 0, 0, 0, 0, getvar("projtime"), guns[gun].projspeed, true, true, gun);
				break;
			}

			case GN_RIFLE:
				part_splash(2, 100, 250, to, COL_YELLOW);
				part_trail(15, 500, gunorigin(gun, from, to, d), to, COL_GREY);
				break;
		}
	}

	sspent *intersectclosest(vec &from, vec &to, sspent *at)
	{
		sspent *best = NULL;
		float bestdist = 1e16f;
		loopi(cl.numdynents())
		{
			sspent *o = (sspent *)cl.iterdynents(i);
			if(!o || o==at || o->state!=CS_ALIVE) continue;
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

	void shorten(vec &from, vec &to, vec &target)
	{
		target.sub(from).normalize().mul(from.dist(to)).add(from);
	}

	void raydamage(vec &from, vec &to, sspent *d)
	{
		sspent *o, *cl;
		if(d->sd.gun==GN_SG)
		{
			bool done[SGRAYS];
			loopj(SGRAYS) done[j] = false;
			for(;;)
			{
				bool raysleft = false;
				int damage = 0;
				o = NULL;
				loop(r, SGRAYS) if(!done[r] && (cl = intersectclosest(from, sg[r], d)))
				{
					if((!o || o==cl) /*&& (damage<cl->health+cl->armour || cl->type!=ENT_AI)*/)
					{
						damage += 1;
						o = cl;
						done[r] = true;
						shorten(from, o->o, sg[r]);
					}
					else raysleft = true;
				}
				if(damage) hitpush(damage, o, d, from, to);
				if(!raysleft) break;
			}
		}
		else if((o = intersectclosest(from, to, d)))
		{
			hitpush(1, o, d, from, to);
			shorten(from, o->o, to);
		}
	}

	void shoot(sspent *d)
	{
		if(!d->attacking) return;
		int attacktime = lastmillis-d->lastaction;
		if(attacktime<d->gunwait) return;

		d->gunwait = 0;

		vec targ, dir, from = d->o;

		float pitch = d->pitch;

		if (d->type == ENT_PLAYER)
		{
			switch (d->sd.gun)
			{
				case GN_GL: pitch = -25.f; break;
				case GN_BOMB: pitch = 75.f; break;
				default: break;
			}
		}
		
		vecfromyawpitch(d->yaw, pitch, 1, 0, dir);
		from.x += dir.x*(d->xradius*0.5f);
		from.y += dir.y*(d->yradius*0.5f);
		from.z -= d->eyeheight*0.5f;
		if(raycubepos(from, dir, targ, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
			targ = dir.mul(10).add(d->o);

		d->lastaction = lastmillis;

		vec unitv;
		float dist = targ.dist(from, unitv);
		unitv.div(dist);
		vec kickback(unitv);
		kickback.mul(guns[d->sd.gun].kickamount*-2.5f);
		d->vel.add(kickback);
		float shorten = 0.0f;

		if(dist > 1024) shorten = 1024;
		if(d->sd.gun == GN_FIST) shorten = d->radius*3.f;
		float barrier = raycube(d->o, unitv, dist, RAY_CLIPMAT|RAY_POLY);
		if(barrier < dist && (!shorten || barrier < shorten))
			shorten = barrier;
		if(shorten)
		{
			targ = unitv;
			targ.mul(shorten);
			targ.add(from);
		}

		if(d->sd.gun==GN_SG) createrays(from, targ);
		else if(d->sd.gun==GN_CG) offsetray(from, targ, 1, targ);

		if(!guns[d->sd.gun].projspeed) raydamage(from, targ, d);

		shootv(d->sd.gun, from, targ, d);
		d->gunwait = guns[d->sd.gun].attackdelay;
	}
};
