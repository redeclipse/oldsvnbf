// weapon.h: all shooting and effects code

struct weaponstate
{
	GAMECLIENT &cl;

	static const int OFFSETMILLIS = 500;
	vec sg[SGRAYS];

	weaponstate(GAMECLIENT &_cl) : cl(_cl)
	{
        CCOMMAND(weapon, "ss", (weaponstate *self, char *a, char *b),
		{
            self->weaponswitch(a[0] ? atoi(a) : -1, b[0] ? atoi(b) : -1);

		});
		CCOMMAND(gunselect, "", (weaponstate *self), intret(self->cl.player1->gunselect));
		CCOMMAND(ammo, "s", (weaponstate *self, char *a),
		{
			int n = a[0] ? atoi(a) : self->cl.player1->gunselect;
			if (n <= -1 || n >= NUMGUNS) return;
			intret(self->cl.player1->ammo[n]);
		});
	}

	void weaponswitch(int a = -1, int b = -1)
	{
		if (cl.player1->state != CS_ALIVE || a < -1 || b < -1 || a >= NUMGUNS || b >= NUMGUNS) return;
		int s = cl.player1->gunselect;

		loopi(NUMGUNS) // only loop the amount of times we have guns for
		{
			if (a >= 0) s = a;
			else s += b;

			while (s >= NUMGUNS) s -= NUMGUNS;
			while (s <= -1) s += NUMGUNS;

			if (!cl.player1->canweapon(s, lastmillis))
			{
				if (a >= 0)
				{
					return;
				}
			}
			else break;
		}

		if(s != cl.player1->gunselect)
		{
			cl.cc.addmsg(SV_GUNSELECT, "ri", s);
			cl.playsoundc(S_SWITCH);
		}
		cl.player1->gunselect = s;
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
		if (s.z < d->o.z-d->height*0.5f && s.z >= d->o.z-d->height) hits |= HIT_LEGS;
		else if (s.z < d->o.z-d->height*0.2f && s.z >= d->o.z-d->height*0.5f) hits |= HIT_TORSO;
		else if (s.z <= d->o.z+d->aboveeye && s.z >= d->o.z-d->height*0.2f) hits |= HIT_HEAD;
		hit(d, v, hits, rays);
	}


	float middist(fpsent *o, vec &dir, vec &v)
	{
		vec middle = o->o;
		middle.z += (o->aboveeye-o->height)/2;
		float dist = middle.dist(v, dir);
		dir.div(dist);
		if (dist < 0) dist = 0;
		return dist;
	}

	void radialeffect(fpsent *d, vec &o)
	{
		vec dir;
		float dist = middist(d, dir, o);
		if(dist < RL_DAMRAD) hit(d, dir, HIT_TORSO, int(dist*DMF));
	}

	void explode(fpsent *d, vec &o, vec &vel, int id, int gun, bool local)
	{
		vec dir;
		float dist = middist(cl.player1, dir, o);
		cl.camerawobble += int(guntype[gun].damage*(1-dist/RL_DISTSCALE/RL_DAMRAD));

		if (guntype[gun].esound >= 0)
			playsound(guntype[gun].esound, &o, 255, 0, true);

		particle_splash(0, 200, 300, o);
		particle_fireball(o, RL_DAMRAD, gun == GUN_RL ? 22 : 23);

        if(gun==GUN_RL) adddynlight(o, 1.15f*RL_DAMRAD, vec(2, 1.5f, 1), 1100, 100, 0, 0.66f*RL_DAMRAD, vec(1.1f, 0.66f, 0.22f));
        else adddynlight(o, 1.15f*RL_DAMRAD, vec(2, 1.5f, 1), 1100, 100);
        adddecal(DECAL_SCORCH, o, gun==GUN_RL ? vec(vel).neg().normalize() : vec(0, 0, 1), RL_DAMRAD/2);

		loopi(rnd(20)+10)
			cl.pj.spawn(vec(o).add(vec(vel)), vel, d, PRJ_DEBRIS);

		if (local)
		{
			hits.setsizenodelete(0);

			loopi(cl.numdynents())
			{
				fpsent *d = (fpsent *)cl.iterdynents(i);
				if (!d || d->state != CS_ALIVE) continue;
				radialeffect(d, o);
			}

			cl.cc.addmsg(SV_EXPLODE, "ri3iv", lastmillis-cl.maptime, gun, id-cl.maptime,
					hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
		}
	}

	void damageeffect(int damage, fpsent *d)
	{
		vec p = d->o;
		p.z += 0.6f*(d->height + d->aboveeye) - d->height;
		particle_splash(3, min(damage/4, 20), 10000, p);
		if(d!=cl.player1)
		{
			s_sprintfd(ds)("@%d", damage);
			particle_text(d->abovehead(), ds, 8);
		}
	}

	void superdamageeffect(vec &vel, fpsent *d)
	{
		if(!d->superdamage) return;
		vec from = d->abovehead();
		loopi(rnd(d->superdamage)+1) cl.pj.spawn(from, vel, d, PRJ_GIBS);
	}

	vec hudgunorigin(int gun, const vec &from, const vec &to, fpsent *d)
	{
		vec offset(from);
		vec front, right;
		vecfromyawpitch(d->yaw, d->pitch, 1, 0, front);
		front.mul(2);
		front.mul(d->radius);
		offset.add(front);
		offset.z += (d->aboveeye + d->height)*0.8f - d->height;
		vecfromyawpitch(d->yaw, 0, 0, -1, right);
		right.mul(0.35f*d->radius);
		offset.add(right);
		if(d->crouching) offset.z -= (d == cl.player1 ? cl.crouching : 1.0f)*(1-CROUCHHEIGHT)*d->height;
		return offset;
	}

	void shootv(int gun, vec &from, vec &to, fpsent *d, bool local)	 // create visual effect from a shot
	{
		if (guntype[gun].sound >= 0) playsound(guntype[gun].sound, &d->o);
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
			{
				vec up = to;
				if (gun == GUN_GL)
				{
					float dist = from.dist(to);
					up.z += dist/8;
				}
				cl.pj.create(from, up, local, d, PRJ_SHOT, guntype[gun].time, guntype[gun].speed, gun);
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
			hitpush(o, from, to);
			shorten(from, o->o, to);
		}
        else adddecal(DECAL_BULLET, to, vec(from).sub(to).normalize(), 2.0f);
	}

	void reload(fpsent *d)
	{
		if (d->canreload(d->gunselect, lastmillis))
		{
			d->gunlast[d->gunselect] = lastmillis;
			d->gunwait[d->gunselect] = guntype[d->gunselect].rdelay;
			cl.cc.addmsg(SV_RELOAD, "ri2", lastmillis-cl.maptime, d->gunselect);
			cl.playsoundc(S_RELOAD);
		}
	}

	void shoot(fpsent *d, vec &targ)
	{
		if (!d->canshoot(d->gunselect, lastmillis)) return;

		d->lastattackgun = d->gunselect;
		d->gunlast[d->gunselect] = lastmillis;
		d->gunwait[d->gunselect] = guntype[d->gunselect].adelay;
		d->ammo[d->gunselect]--;
		d->totalshots += guntype[d->gunselect].damage*(d->gunselect == GUN_SG ? SGRAYS : 1);

		vec to = targ, from = hudgunorigin(d->gunselect, d->o, to, d), unitv;
		float dist = to.dist(from, unitv);
		unitv.div(dist);
		vec kickback(unitv);
		kickback.mul(guntype[d->gunselect].kick);
		d->vel.add(kickback);
		if (d == cl.player1) cl.camerawobble += guntype[d->gunselect].wobble;
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

		shootv(d->gunselect, from, to, d, true);

		if(d == cl.player1)
		{
            cl.cc.addmsg(SV_SHOOT, "ri2i6iv", lastmillis-cl.maptime, d->gunselect,
							(int)(from.x*DMF), (int)(from.y*DMF), (int)(from.z*DMF),
							(int)(to.x*DMF), (int)(to.y*DMF), (int)(to.z*DMF),
							hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
		}
	}

    void preload()
    {
        loopi(NUMGUNS)
        {
            const char *file = guntype[i].name;
            if(!file) continue;
            s_sprintfd(mdl)("weapons/%s", file);
            loadmodel(mdl, -1, true);
            //s_sprintf(mdl)("hudguns/%s/blue", file);
            //loadmodel(mdl, -1, true);
            s_sprintf(mdl)("weapons/%s/vwep", file);
            loadmodel(mdl, -1, true);
        }
    }
} ws;
