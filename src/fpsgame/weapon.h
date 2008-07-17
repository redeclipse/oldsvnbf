// weapon.h: all shooting and effects code

struct weaponstate
{
	GAMECLIENT &cl;

	static const int OFFSETMILLIS = 500;
	vec sg[SGRAYS];

	IVARP(autoreload, 0, 1, 1);// auto reload when empty

	weaponstate(GAMECLIENT &_cl) : cl(_cl)
	{
        CCOMMAND(weapon, "ss", (weaponstate *self, char *a, char *b),
		{
            self->weaponswitch(self->cl.player1, a[0] ? atoi(a) : -1, b[0] ? atoi(b) : -1);

		});
		CCOMMAND(gunselect, "", (weaponstate *self), intret(self->cl.player1->gunselect));
		CCOMMAND(ammo, "s", (weaponstate *self, char *a),
		{
			int n = a[0] ? atoi(a) : self->cl.player1->gunselect;
			if (n <= -1 || n >= NUMGUNS) return;
			intret(self->cl.player1->ammo[n]);
		});
	}

	void weaponswitch(fpsent *d, int a = -1, int b = -1)
	{
		if (!cl.allowmove(cl.player1) || cl.inzoom() || a < -1 || b < -1 || a >= NUMGUNS || b >= NUMGUNS) return;
		int s = d->gunselect;

		loopi(NUMGUNS) // only loop the amount of times we have guns for
		{
			if (a >= 0) s = a;
			else s += b;

			while (s >= NUMGUNS) s -= NUMGUNS;
			while (s <= -1) s += NUMGUNS;

			if (!d->canswitch(s, lastmillis))
			{
				if (a >= 0) return;
			}
			else break;
		}

		if(s != d->gunselect)
		{
			d->setgun(s, lastmillis);
			cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
		}
	}

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
            dir.normalize();
			raycubepos(from, dir, dest, 0, RAY_CLIPMAT|RAY_POLY);
			return;
		}
	}

	void createrays(vec &from, vec &to)			 // create random spread of rays for the shotgun
	{
		loopi(SGRAYS) offsetray(from, to, SGSPREAD, sg[i]);
	}

	struct hitmsg
	{
		int flags, target, lifesequence, info;
		ivec dir;
	};
	vector<hitmsg> hits;

	void hit(fpsent *d, vec &vel, int flags = 0, int info = 1)
	{
		hitmsg &h = hits.add();
		h.flags = flags;
		h.target = d->clientnum;
		h.lifesequence = d->lifesequence;
		h.info = info;
		h.dir = ivec(int(vel.x*DNF), int(vel.y*DNF), int(vel.z*DNF));
	}

	void hitpush(fpsent *d, vec &from, vec &to, int rays = 1)
	{
		vec v(to), s(to);
		v.sub(from);
		v.normalize();
		shorten(from, d->o, s);
		int hits = 0;
		if (s.z < d->o.z-d->height*0.75f && s.z >= d->o.z-d->height) hits |= HIT_LEGS;
		else if (s.z < d->o.z-d->aboveeye && s.z >= d->o.z-d->height*0.75f) hits |= HIT_TORSO;
		else if (s.z <= d->o.z+d->aboveeye && s.z >= d->o.z-d->aboveeye) hits |= HIT_HEAD;
		hit(d, v, hits, rays);
	}

	float middist(physent *o, vec &dir, vec &v)
	{
		vec middle = o->o;
		middle.z += (o->aboveeye-o->height)/2;
		float dist = middle.dist(v, dir);
		dir.div(dist);
		if (dist < 0) dist = 0;
		return dist;
	}

	void radialeffect(fpsent *d, vec &o, int radius, int flags)
	{
		vec dir;
		float dist = middist(d, dir, o);
		if(dist < radius) hit(d, dir, flags, int(dist*DMF));
	}

	void explode(fpsent *d, vec &o, vec &vel, int id, int gun, bool local)
	{
		vec dir;
		float dist = middist(camera1, dir, o);

		if (guntype[gun].esound >= 0)
			playsound(guntype[gun].esound, &o, 255, 0, 0, SND_COPY);

		if(gun != GUN_FLAMER)
		{
			cl.quakewobble += int(guntype[gun].damage*(1-dist/guntype[gun].scale/guntype[gun].radius));

			particle_splash(0, 200, 300, o);
			particle_fireball(o, guntype[gun].radius, gun == GUN_GL ? 23 : 22);
			if(gun==GUN_RL) adddynlight(o, 1.15f*guntype[gun].radius, vec(2, 1.5f, 1), 1100, 100, 0, 0.66f*guntype[gun].radius, vec(1.1f, 0.66f, 0.22f));
			else adddynlight(o, 1.15f*guntype[gun].radius, vec(2, 1.5f, 1), 1100, 100);

			loopi(rnd(20)+10)
				cl.pj.spawn(vec(o).add(vec(vel)), vel, d, PRJ_DEBRIS);
		}
        adddecal(DECAL_SCORCH, o, gun==GUN_GL ? vec(0, 0, 1) : vec(vel).neg().normalize(), guntype[gun].radius);

		if(local)
		{
			hits.setsizenodelete(0);

			loopi(cl.numdynents())
			{
				fpsent *f = (fpsent *)cl.iterdynents(i);
				if (!f || f->state != CS_ALIVE) continue;
				radialeffect(f, o, guntype[gun].radius, gun != GUN_FLAMER ? HIT_EXPLODE : HIT_BURN);
			}

			cl.cc.addmsg(SV_EXPLODE, "ri4iv", d->clientnum, lastmillis-cl.maptime, gun, id-cl.maptime,
					hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
		}
	}

	vec gunorigin(int gun, const vec &from, const vec &to, fpsent *d)
	{
		vec offset(from);
		vec front, right;
		vecfromyawpitch(d->yaw, d->pitch, 1, 0, front);
		front.mul(2);
		front.mul(d->radius);
		offset.add(front);
		offset.z += (d->aboveeye + d->height)*0.835f - d->height;
		vecfromyawpitch(d->yaw, 0, 0, -1, right);
		right.mul(0.35f*d->radius);
		offset.add(right);
		if(d->crouching)
			offset.z -= (d == cl.player1 ? min(1.0f, (lastmillis-d->crouchtime)/200.f) : 1.0f)*(1-CROUCHHEIGHT)*d->height;
		return offset;
	}

	void shootv(int gun, int power, vec &from, vec &to, fpsent *d, bool local)	 // create visual effect from a shot
	{
		int pow = guntype[gun].power ? power : 100;

		if (guntype[gun].sound >= 0)
		{
			if (gun == GUN_FLAMER)
			{
				if (!issound(d->wschan))
					d->wschan = playsound(guntype[gun].sound, &from, 255, 0, 0, SND_COPY);
			}
			else
			{
				playsound(guntype[gun].sound, &from, 255, 0, 0, SND_COPY);
				d->wschan = -1;
			}
		}
		if(gun != GUN_GL)
			adddynlight(from, 40, vec(1.1f, 0.66f, 0.22f), 40, 0, DL_FLASH);

		switch(gun)
		{
			case GUN_SG:
			{
				loopi(SGRAYS)
				{
					particle_splash(0, 20, 250, sg[i]);
                    particle_flare(from, sg[i], 300, 10, d);
                    if(!local) adddecal(DECAL_BULLET, sg[i], vec(from).sub(sg[i]).normalize(), 2.0f);
				}
				break;
			}

			case GUN_CG:
			case GUN_PISTOL:
			{
				particle_splash(0, 200, 250, to);
                particle_flare(from, to, 600, 10, d);
                if(!local) adddecal(DECAL_BULLET, to, vec(from).sub(to).normalize(), 2.0f);
				break;
			}

			case GUN_RL:
			case GUN_GL:
			case GUN_FLAMER:
			{
				vec up = to;
				if (gun == GUN_GL)
				{
					float dist = from.dist(to);
					up.z += dist/8;
				}
				int spd = clamp(int(float(guntype[gun].speed)/100.f*pow), 1, guntype[gun].speed);
				cl.pj.create(from, up, local, d, PRJ_SHOT, guntype[gun].time, spd, gun);
				break;
			}

			case GUN_RIFLE:
				particle_splash(0, 200, 250, to);
				particle_trail(21, 500, from, to);
                if(!local) adddecal(DECAL_BULLET, to, vec(from).sub(to).normalize(), 3.0f);
				break;
		}
	}

	fpsent *intersectclosest(vec &from, vec &to, fpsent *at)
	{
		fpsent *best = NULL;
		float bestdist = 1e16f;
		loopi(cl.numdynents())
		{
			fpsent *o = (fpsent *)cl.iterdynents(i);
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

	void raydamage(vec &from, vec &to, fpsent *d)
	{
		fpsent *o, *cl;
		if(d->gunselect==GUN_SG)
		{
			bool done[SGRAYS];
			loopj(SGRAYS) done[j] = false;
			for(;;)
			{
				bool raysleft = false;
				int hitrays = 0;
				o = NULL;
				loop(r, SGRAYS) if(!done[r] && (cl = intersectclosest(from, sg[r], d)))
				{
					if((!o || o==cl) /*&& (damage<cl->health+cl->armour || cl->type!=ENT_AI)*/)
					{
						hitrays++;
						o = cl;
						done[r] = true;
						shorten(from, o->o, sg[r]);
					}
					else raysleft = true;
				}
				if(hitrays) hitpush(o, from, to, hitrays);
				if(!raysleft) break;
			}
            loopj(SGRAYS) if(!done[j]) adddecal(DECAL_BULLET, sg[j], vec(from).sub(sg[j]).normalize(), 2.0f);
		}
		else if((o = intersectclosest(from, to, d)))
		{
			shorten(from, o->o, to);
			hitpush(o, from, to);
		}
        else adddecal(DECAL_BULLET, to, vec(from).sub(to).normalize(), 2.0f);
	}

	bool doautoreload(fpsent *d)
	{
		return autoreload() && isgun(d->gunselect) && d->ammo[d->gunselect] <= 0 && d->canreload(d->gunselect, lastmillis);
	}

	void checkweapons(fpsent *d)
	{
		loopi(NUMGUNS) if(d->gunstate[i] != GUNSTATE_NONE)
		{
			if(d->state != CS_ALIVE || (d->gunstate[i] != GUNSTATE_POWER && lastmillis-d->gunlast[i] >= d->gunwait[i]))
				d->setgunstate(i, GUNSTATE_NONE, 0, lastmillis);
		}
	}

	void update()
	{
		checkweapons(cl.player1);
		loopv(cl.players) if(cl.players[i]) checkweapons(cl.players[i]);
	}

	void reload(fpsent *d)
	{
		if(guntype[d->gunselect].rdelay <= 0)
		{
			int bestgun = d->bestgun();
			if(d->ammo[d->gunselect] <= 0 && d->canswitch(bestgun, lastmillis))
			{
				d->setgun(bestgun, lastmillis);
				cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
			}
		}
		else if(d->reloading || doautoreload(d))
		{
			if(d->canreload(d->gunselect, lastmillis))
			{
				d->gunreload(d->gunselect, guntype[d->gunselect].add, lastmillis);
				cl.cc.addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
			}
			else if(d->reloading) cl.playsoundc(S_DENIED, d);
		}
		d->reloading = false;
	}

	void shoot(fpsent *d, vec &targ)
	{
		if(!d->canshoot(d->gunselect, lastmillis)) return;

		int power = 100;
		if(guntype[d->gunselect].power)
		{
			if(d->gunstate[d->gunselect] != GUNSTATE_POWER) // FIXME: not synched in MP yet!!
			{
				if(d->attacking) d->setgunstate(d->gunselect, GUNSTATE_POWER, 0, lastmillis);
				else return;
			}

			int secs = lastmillis-d->gunlast[d->gunselect];
			if(d->attacking && secs < guntype[d->gunselect].power) return;

			power = clamp(int(float(secs)/float(guntype[d->gunselect].power)*100.f), 0, 100);
			d->attacking = false;
		}
		else if(!d->attacking) return;

		if(guntype[d->gunselect].max) d->ammo[d->gunselect]--;
		d->setgunstate(d->gunselect, GUNSTATE_SHOOT, guntype[d->gunselect].adelay, lastmillis);
		d->totalshots += guntype[d->gunselect].damage*(d->gunselect == GUN_SG ? SGRAYS : 1);

		vec to = targ, from = gunorigin(d->gunselect, d->o, to, d), unitv;
		float dist = to.dist(from, unitv);
		unitv.div(dist);
		vec kickback(unitv);
		kickback.mul(guntype[d->gunselect].kick);
		d->vel.add(kickback);
		if(d == cl.player1) cl.quakewobble += guntype[d->gunselect].wobble;
		float barrier = raycube(d->o, unitv, dist, RAY_CLIPMAT|RAY_POLY);
		if(barrier < dist)
		{
			to = unitv;
			to.mul(barrier);
			to.add(from);
		}

		if(d->gunselect==GUN_SG) createrays(from, to);
		else if(d->gunselect==GUN_CG) offsetray(from, to, 1, to);

		hits.setsizenodelete(0);
		if(!guntype[d->gunselect].speed) raydamage(from, to, d);

		shootv(d->gunselect, power, from, to, d, true);

		cl.cc.addmsg(SV_SHOOT, "ri4i6iv", d->clientnum, lastmillis-cl.maptime, d->gunselect, power,
						(int)(from.x*DMF), (int)(from.y*DMF), (int)(from.z*DMF),
						(int)(to.x*DMF), (int)(to.y*DMF), (int)(to.z*DMF),
						hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
	}

    void preload()
    {
        loopi(NUMGUNS)
        {
            const char *file = guntype[i].name;
            if(!file) continue;
            s_sprintfd(mdl)("weapons/%s", file);
            loadmodel(mdl, -1, true);
            s_sprintf(mdl)("weapons/%s/vwep", file);
            loadmodel(mdl, -1, true);
        }
    }
} ws;
