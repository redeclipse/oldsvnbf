// weapon.h: all shooting and effects code

struct weaponstate
{
	gameclient &cl;

	static const int OFFSETMILLIS = 500;
	vec sg[SGRAYS];

	int requestswitch, requestreload;

	IVARP(autoreload, 0, 1, 1);// auto reload when empty
	IVARP(switchgl, 0, 0, 1);

	weaponstate(gameclient &_cl) : cl(_cl), requestswitch(0), requestreload(0)
	{
        CCOMMAND(weapon, "ss", (weaponstate *self, char *a, char *b),
		{
            self->weaponswitch(self->cl.player1, a[0] ? atoi(a) : -1, b[0] ? atoi(b) : -1);

		});
		CCOMMAND(gunselect, "", (weaponstate *self), intret(self->cl.player1->gunselect));
		CCOMMAND(ammo, "s", (weaponstate *self, char *a),
		{
			int n = a[0] ? atoi(a) : self->cl.player1->gunselect;
			if(n <= -1 || n >= GUN_MAX) return;
			intret(self->cl.player1->ammo[n]);
		});
	}

	void weaponswitch(gameent *d, int a = -1, int b = -1)
	{
		if(a < -1 || b < -1 || a >= GUN_MAX || b >= GUN_MAX) return;
		if(lastmillis-requestswitch <= 1000) return;
		if (!cl.allowmove(cl.player1) || cl.inzoom()) return;
		int s = d->gunselect;

		loopi(GUN_MAX) // only loop the amount of times we have guns for
		{
			if(a >= 0) s = a;
			else s += b;

			while(s > GUN_MAX-1) s -= GUN_MAX;
			while(s < 0) s += GUN_MAX;
			if(a < 0 && !switchgl() && s == GUN_GL)
				continue;

			if(d->canswitch(s, lastmillis))
			{
				cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, s);
				requestswitch = lastmillis;
				return;
			}
			else if(a >= 0) break;
		}
		playsound(S_DENIED, 0, 255, d->o, d);
	}

	void offsetray(vec &from, vec &to, int spread, vec &dest)
	{
		float f = to.dist(from)*float(spread)/10000.f;
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

	void hit(gameent *d, vec &vel, int flags = 0, int info = 1)
	{
		hitmsg &h = hits.add();
		h.flags = flags;
		h.target = d->clientnum;
		h.lifesequence = d->lifesequence;
		h.info = info;
		h.dir = ivec(int(vel.x*DNF), int(vel.y*DNF), int(vel.z*DNF));
	}

	void hitpush(gameent *d, vec &from, vec &to, int rays = 1)
	{
		vec v(to), s(to), pos = cl.headpos(d);
		v.sub(from);
		v.normalize();
		shorten(from, pos, s);
		int hits = 0;
		if(s.z < pos.z-d->height*0.75f && s.z >= pos.z-d->height) hits |= HIT_LEGS;
		else if(s.z < pos.z-d->aboveeye && s.z >= pos.z-d->height*0.75f) hits |= HIT_TORSO;
		else if(s.z <= pos.z+d->aboveeye && s.z >= pos.z-d->aboveeye) hits |= HIT_HEAD;
		hit(d, v, hits, rays);
	}

	float middist(physent *o, vec &dir, vec &v)
	{
		vec middle = o->o;
		middle.z += (o->aboveeye-o->height)/2;
		float dist = middle.dist(v, dir);
		dir.div(dist);
		if(dist < 0) dist = 0;
		return dist;
	}

	void radialeffect(gameent *d, vec &o, int radius, int flags)
	{
		vec dir;
		float dist = middist(d, dir, o);
		if(dist < radius) hit(d, dir, flags, int(dist*DMF));
	}

	void explode(gameent *d, vec &o, vec &vel, int id, int gun, bool local)
	{
		vec dir;
		float dist = middist(camera1, dir, o);

		if(guntype[gun].esound >= 0)
			playsound(guntype[gun].esound, 0, 128, o);

		if(gun == GUN_PISTOL)
		{
			regular_part_splash(7, rnd(2)+1, 50, o, teamtype[d->team].colour, guntype[gun].explode*0.75f, 5);
			adddynlight(o, 1.15f*guntype[gun].explode, vec((teamtype[d->team].colour>>16)/255.f, ((teamtype[d->team].colour>>8)&0xFF)/255.f, (teamtype[d->team].colour&0xFF)/255.f), 100, 10);
		}
		else if(gun == GUN_FLAMER)
		{
			regular_part_splash(7, rnd(2)+1, 200, o, 0xCC6600, guntype[gun].explode, 5);
			adddynlight(o, 1.15f*guntype[gun].explode, vec(1.1f, 0.22f, 0.02f), 100, 10);
		}
		else
		{
			cl.quakewobble += int(guntype[gun].damage*(1.f-dist/EXPLOSIONSCALE/guntype[gun].explode/10.f));
			particle_splash(0, 200, 300, o);
			particle_fireball(o, guntype[gun].explode, gun == GUN_GL ? 23 : 22);
#if 0
			if(gun==GUN_RL) adddynlight(o, 1.15f*guntype[gun].explode, vec(2, 1.5f, 1), 1100, 100, 0, 0.66f*guntype[gun].explode, vec(1.1f, 0.66f, 0.22f));
			else
#endif
				adddynlight(o, 1.15f*guntype[gun].explode, vec(2, 1.5f, 1), 1100, 100);

			loopi(rnd(20)+10)
				cl.pj.spawn(vec(o).add(vec(vel)), vel, d, PRJ_DEBRIS);
		}
        adddecal(DECAL_SCORCH, o, gun == GUN_GL ? vec(0, 0, 1) : vec(vel).neg().normalize(), guntype[gun].explode);
        adddecal(DECAL_ENERGY, o, gun == GUN_GL ? vec(0, 0, 1) : vec(vel).neg().normalize(), guntype[gun].explode*0.5f, gun == GUN_PISTOL ? bvec((teamtype[d->team].colour>>16)/2, ((teamtype[d->team].colour>>8)&0xFF)/2, (teamtype[d->team].colour&0xFF)/3) : bvec(96, 48, 16));

		if(local)
		{
			hits.setsizenodelete(0);

			loopi(cl.numdynents())
			{
				gameent *f = (gameent *)cl.iterdynents(i);
				if(!f || f->state != CS_ALIVE || lastmillis-f->lastspawn <= REGENWAIT) continue;
				radialeffect(f, o, guntype[gun].explode, gun == GUN_FLAMER || gun == GUN_PISTOL ? HIT_BURN : HIT_EXPLODE);
			}

			cl.cc.addmsg(SV_EXPLODE, "ri4iv", d->clientnum, lastmillis-cl.maptime, gun, id >= 0 ? id-cl.maptime : id,
					hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
		}
	}

	vec gunorigin(const vec &from, const vec &to, gameent *d, bool third)
	{
		vec origin;
		if(d->muzzle.x >= 0) origin = d->muzzle;
		else
		{
			bool oversized = d->gunselect == GUN_SG || d->gunselect == GUN_FLAMER || d->gunselect == GUN_RIFLE;
			float hoff = 0.f, roff = 0.f, dmul = oversized ? 3.f : 2.f;
			if(third)
			{
				hoff = oversized ? 0.88f : 0.86f;
				roff = oversized ? 0.33f : 0.35f;
			}
			else
			{
				hoff = oversized ? 0.92f : 0.89f;
				roff = oversized ? 0.35f : 0.33f;
			}
			vec front, right;
			vecfromyawpitch(d->yaw, d->pitch, 1, 0, front);
			front.mul(d->radius*dmul);
			origin = vec(from).add(front);
			origin.z += (d->aboveeye + d->height)*hoff - d->height;
			vecfromyawpitch(d->yaw, 0, 0, -1, right);
			right.mul(d->radius*roff);
			origin.add(right);
		}
		return origin;
	}

	void shootv(int gun, int power, vec &from, vec &to, gameent *d, bool local)	 // create visual effect from a shot
	{
		int pow = guntype[gun].power ? power : 100;

		if(gun == GUN_FLAMER)
		{
			int ends = lastmillis+(d->gunwait[gun]*2);
			if(issound(d->wschan)) sounds[d->wschan].ends = ends;
			else playsound(guntype[gun].sound, SND_LOOP, 255, d->o, d, &d->wschan, ends);
		}
		else playsound(guntype[gun].sound, 0, 255, d->o, d);

		switch(gun)
		{
			case GUN_SG:
			{
				loopi(SGRAYS)
				{
					particle_splash(0, 20, 250, sg[i]);
                    particle_flare(from, sg[i], 250, 10, d);
                    if(!local) adddecal(DECAL_BULLET, sg[i], vec(from).sub(sg[i]).normalize(), 2.0f);
				}
				adddynlight(from, 50, vec(1.1f, 0.66f, 0.22f), 50, 0, DL_FLASH);
				part_create(20, 50, from, 0xFFAA00, 4.f, d);
				regularshape(6, 2, 0x333333, 21, rnd(20)+1, 25, from, 1.5f);
				break;
			}

			case GUN_CG:
			{
				particle_splash(0, 200, gun == GUN_CG ? 100 : 200, to);
                particle_flare(from, to, 250, 10, d);
                if(!local) adddecal(DECAL_BULLET, to, vec(from).sub(to).normalize(), 2.0f);
                adddynlight(from, 40, vec(1.1f, 0.66f, 0.22f), 50, 0, DL_FLASH);
				part_create(20, 50, from, 0xFFAA00, 3.f, d);
				regularshape(6, 1, 0x333333, 21, rnd(10)+1, 25, from, 0.5f);
				break;
			}

#if 0
			case GUN_RL:
#endif
			case GUN_PISTOL:
			case GUN_FLAMER:
			case GUN_GL:
			{
				int spd = clamp(int(float(guntype[gun].speed)/100.f*pow), 1, guntype[gun].speed);
				cl.pj.create(from, to, local, d, PRJ_SHOT, guntype[gun].time, gun != GUN_GL ? 0 : 150, spd, 0, WEAPON, gun);
				if(gun == GUN_PISTOL)
				{
					adddynlight(from, 25, vec((teamtype[d->team].colour>>16)/255.f, ((teamtype[d->team].colour>>8)&0xFF)/255.f, (teamtype[d->team].colour&0xFF)/255.f), 20, 0, DL_FLASH);
					part_create(7, 50, from, teamtype[d->team].colour, 1.2f, d);
				}
				else if(gun == GUN_FLAMER)
				{
					adddynlight(from, 50, vec(1.1f, 0.33f, 0.01f), 50, 0, DL_FLASH);
					part_create(5, 50, from, 0xFF2200, 1.5f, d);
				}
				break;
			}

			case GUN_RIFLE:
			{
				part_splash(14, 100, 250, to, teamtype[d->team].colour, 1.2f);
				part_trail(14, 1500, from, to, teamtype[d->team].colour, 1.2f);
                if(!local) adddecal(DECAL_BULLET, to, vec(from).sub(to).normalize(), 3.0f);
                adddynlight(from, 50, vec((teamtype[d->team].colour>>16)/255.f, ((teamtype[d->team].colour>>8)&0xFF)/255.f, (teamtype[d->team].colour&0xFF)/255.f), 50, 0, DL_FLASH);
				part_create(2, 100, from, teamtype[d->team].colour, 2.f, d);
				break;
			}
		}
	}

	gameent *intersectclosest(vec &from, vec &to, gameent *at)
	{
		gameent *best = NULL;
		float bestdist = 1e16f;
		loopi(cl.numdynents())
		{
			gameent *o = (gameent *)cl.iterdynents(i);
            if(!o || o==at || o->state!=CS_ALIVE || lastmillis-o->lastspawn <= REGENWAIT) continue;
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

	void raydamage(vec &from, vec &to, gameent *d)
	{
		gameent *o, *cl;
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

	bool doautoreload(gameent *d)
	{
		return autoreload() && d->hasgun(d->gunselect) && !d->ammo[d->gunselect] && d->canreload(d->gunselect, lastmillis);
	}

	void reload(gameent *d)
	{
		if(guntype[d->gunselect].rdelay <= 0)
		{
			int bestgun = d->bestgun();
			if(d->ammo[d->gunselect] <= 0 && d->canswitch(bestgun, lastmillis))
			{
				if(lastmillis-requestswitch > 1000)
				{
					cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, bestgun);
					requestswitch = lastmillis;
				}
			}
		}
		else if((d->reloading || doautoreload(d)) && d->canreload(d->gunselect, lastmillis))
		{
			if(lastmillis-requestreload > 1000)
			{
				cl.cc.addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
				requestreload = lastmillis;
			}
		}
	}

	void shoot(gameent *d, vec &targ, int pow = 0)
	{
		if(!d->canshoot(d->gunselect, lastmillis)) return;

		int power = pow ? pow : 100;
		if(guntype[d->gunselect].power)
		{
			if(!pow)
			{
				if(d->gunstate[d->gunselect] != GUNSTATE_POWER) // FIXME: not synched in MP yet!!
				{
					if(d->attacking) d->setgunstate(d->gunselect, GUNSTATE_POWER, 0, lastmillis);
					else return;
				}

				int secs = lastmillis-d->gunlast[d->gunselect];
				if(d->attacking && secs < guntype[d->gunselect].power) return;

				power = clamp(int(float(secs)/float(guntype[d->gunselect].power)*100.f), 0, 100);
			}
			d->attacking = false;
		}
		else if(!d->attacking) return;

		if(guntype[d->gunselect].max) d->ammo[d->gunselect] = max(d->ammo[d->gunselect]-1, 0);
		d->setgunstate(d->gunselect, GUNSTATE_SHOOT, guntype[d->gunselect].adelay, lastmillis);
		d->totalshots += guntype[d->gunselect].damage*(d->gunselect == GUN_SG ? SGRAYS : 1);

		vec to = targ, from = gunorigin(d->o, to, d, d != cl.player1 || cl.isthirdperson()), unitv;
		float dist = to.dist(from, unitv);
		unitv.div(dist);
		vec kickback(unitv);
		kickback.mul(guntype[d->gunselect].kick*(cl.ph.iscrouching(d) ? 0.1f : 1.f));
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
		else if(d->gunselect==GUN_CG) offsetray(from, to, 5, to);

		hits.setsizenodelete(0);
		if(!guntype[d->gunselect].speed) raydamage(from, to, d);

		shootv(d->gunselect, power, from, to, d, true);

		cl.cc.addmsg(SV_SHOOT, "ri4i6iv", d->clientnum, lastmillis-cl.maptime, d->gunselect, power,
						(int)(from.x*DMF), (int)(from.y*DMF), (int)(from.z*DMF),
						(int)(to.x*DMF), (int)(to.y*DMF), (int)(to.z*DMF),
						hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
	}

    void preload(int gun = -1)
    {
    	int g = gun < 0 ? (m_insta(cl.gamemode, cl.mutators) ? instaspawngun : spawngun) : gun;
    	if(isgun(g))
        {
        	string mdl;
            s_sprintf(mdl)("weapons/%s", guntype[g].name);
            loadmodel(mdl, -1, true);
			s_sprintf(mdl)("weapons/%s/vwep", guntype[g].name);
			loadmodel(mdl, -1, true);
        }
    }
} ws;
