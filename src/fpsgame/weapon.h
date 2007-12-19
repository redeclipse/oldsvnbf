// weapon.h: all shooting and effects code, projectile management

struct weaponstate
{
	fpsclient &cl;
	fpsent *player1;

	static const int OFFSETMILLIS = 500;
	vec sg[SGRAYS];

	weaponstate(fpsclient &_cl) : cl(_cl), player1(_cl.player1)
	{
        CCOMMAND(weapon, "sss", (weaponstate *self, char *a, char *b),
		{
            self->weaponswitch(a[0] ? atoi(a) : -1, b[0] ? atoi(b) : -1);

		});
		CCOMMAND(getgun, "", (weaponstate *self), intret(self->player1->gunselect));
		CCOMMAND(getammo, "s", (weaponstate *self, char *a),
		{
			int n = a[0] ? atoi(a) : -1;
			if (n <= -1 || n >= NUMGUNS) n = self->player1->gunselect;
			intret(self->player1->ammo[n]);
		});
		CCOMMAND(getweapon, "", (weaponstate *self), intret(self->player1->gunselect));
	}

	void weaponswitch(int a = -1, int b = -1)
	{
		if (player1->state != CS_ALIVE || a < -1 || b < -1 || a >= NUMGUNS || b >= NUMGUNS) return;
		int s = player1->gunselect;

		loopi(NUMGUNS) // only loop the amount of times we have guns for
		{
			if (a >= 0) s = a;
			else s += b;

			while (s >= NUMGUNS) s -= NUMGUNS;
			while (s <= -1) s += NUMGUNS;

			if (!player1->canweapon(s, cl.lastmillis))
			{
				if (a >= 0)
				{
					return;
				}
			}
			else break;
		}

		if(s != player1->gunselect)
		{
			cl.cc.addmsg(SV_GUNSELECT, "ri", s);
			cl.playsoundc(S_SWITCH);
		}
		player1->gunselect = s;
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
		int target, lifesequence, info;
		ivec dir;
	};
	vector<hitmsg> hits;

	void hit(fpsent *d, vec &vel, int info = 1)
	{
		hitmsg &h = hits.add();
		h.target = d->clientnum;
		h.lifesequence = d->lifesequence;
		h.info = info;
		h.dir = ivec(int(vel.x*DNF), int(vel.y*DNF), int(vel.z*DNF));
	}

	void hitpush(fpsent *d, vec &from, vec &to, int rays = 1)
	{
		vec v(to);
		v.sub(from);
		v.normalize();
		hit(d, v, rays);
	}

	enum { BNC_SHOT = 0, BNC_GIBS, BNC_DEBRIS };

	struct bouncent : physent
	{
		vec from, to;
		int lifetime, lastbounce;
		float roll;
		bool local;
		fpsent *owner;
		int bnctype;
		float elasticity, relativity, waterfric;
		int gun, schan, id;

		bouncent() : schan(-1), id(-1)
		{
			reset();
		}
		~bouncent() {}

		void init(vec &_f, vec &_t, bool _b, fpsent *_o, int _n, int _i, int _s, int _g, int _l)
		{
			state = CS_ALIVE;

			o = from = _f;
			to = _t;

			local = _b;
			owner = _o;
			bnctype = _n;
			lifetime = _i;
			gun = _g;
			maxspeed = _s;
			id = lastbounce = _l;

			switch (bnctype)
			{
				case BNC_SHOT:
				{
					switch (gun)
					{
						case GUN_GL:
							aboveeye = eyeheight = radius = 0.5f;
							elasticity = 0.33f;
							relativity = 0.5f;
							waterfric = 2.0f;
							break;
						case GUN_RL:
						{
							aboveeye = eyeheight = radius = 1.0f;
							elasticity = 0.0f;
							relativity = 0.25f;
							waterfric = 1.5f;
							break;
						}
						case GUN_PISTOL:
						case GUN_SG:
						case GUN_CG:
						case GUN_RIFLE:
						default:
						{
							aboveeye = eyeheight = radius = 0.25f;
							elasticity = 0.5f;
							relativity = 0.25f;
							waterfric = 1.25f;
							break;
						}
					}
					if (guns[gun].fsound >= 0) schan = playsound(guns[gun].fsound, &o, &vel);
					break;
				}
				case BNC_GIBS:
				{
					aboveeye = eyeheight = radius = 0.66f;
					elasticity = 0.25f;
					relativity = 1.0f;
					waterfric = 2.0f;
					schan = playsound(S_WHIZZ, &o, &vel);
					break;
				}
				case BNC_DEBRIS:
				default:
				{
					aboveeye = eyeheight = radius = 1.25f;
					elasticity = 0.66f;
					relativity = 1.0f;
					waterfric = 1.75f;
					schan = playsound(S_WHIZZ, &o, &vel);
					break;
				}
			}

			vec dir(vec(vec(to).sub(from)).normalize());

			vectoyawpitch(dir, yaw, pitch);
			vel = vec(vec(dir).mul(maxspeed)).add(vec(owner->vel).mul(relativity));

			while (!collide(this, dir) && hitplayer) o.add(dir);
		}

		void reset()
		{
			physent::reset();
			type = ENT_BOUNCE;
			roll = 0.f;
		}

		void check(int time)
		{
			if (sndchans.inrange(schan) && !sndchans[schan].playing()) schan = -1;
            if (bnctype == BNC_SHOT) regular_particle_splash(5, 1, 500, o);
			else if (bnctype == BNC_GIBS) particle_splash(3, 1, 10000, o);
		}

		bool update(int millis, int time, int qtime)
		{
			cube &c = lookupcube(int(o.x), int(o.y), int(o.z));
			bool water = c.ext && isliquid(c.ext->material);
			float secs = float(qtime) / 1000.0f;
			vec old(o);

			if (elasticity > 0.f) vel.sub(vec(0, 0, float(getvar("gravity"))*secs));

			vec dir(vel);
			if (water) dir.div(waterfric);
			dir.mul(secs);
			o.add(dir);

			if (!collide(this, dir) || inside || hitplayer)
			{
				o = old;

				if (bnctype != BNC_SHOT || gun == GUN_GL)
				{
					vec pos(wall);

					if (gun == GUN_GL)
					{
						//conoutf("BNC %d [%.1f,%.1f,%.1f]", qtime, pos.x, pos.y, pos.z);
						//conoutf("GREANDE %.1f,%.1f [%.1f,%.1f,%.1f] %.1f", yaw, pitch, vel.x, vel.y, vel.z, vel.magnitude());
						if (hitplayer)
						{
							pos = vec(vec(o).sub(hitplayer->o)).normalize();
							//conoutf("PLAYER %.1f,%.1f [%.1f,%.1f,%.1f] %.1f", hitplayer->yaw, hitplayer->pitch, hitplayer->vel.x, hitplayer->vel.y, hitplayer->vel.z, hitplayer->vel.magnitude());
							//conoutf("OFFSET [%.1f,%.1f,%.1f]", pos.x, pos.y, pos.z);
						}
					}

					if (vel.magnitude() > 1.f)
					{
						vel.apply(pos, elasticity);
						if (hitplayer) vel.influence(pos, hitplayer->vel, elasticity);

						if (millis-lastbounce > 500)
						{
							if (bnctype == BNC_SHOT && guns[gun].rsound >= 0) playsound(guns[gun].rsound, &o, &vel, vel.magnitude());
							else if (bnctype == BNC_GIBS) playsound(S_SPLAT, &o, &vel, vel.magnitude());
							else if (bnctype == BNC_DEBRIS) playsound(S_DEBRIS, &o, &vel, vel.magnitude());
							lastbounce = millis;
						}
					}
					return true; // stay alive until timeout
				}
				return false; // die on impact
			}

			if (bnctype == BNC_SHOT && gun == GUN_GL) roll += int(vel.magnitude() / time) % 360;

			return true;
		}

		void cleanup()
		{
			if (sndchans.inrange(schan) && sndchans[schan].playing())
				sndchans[schan].stop();

			schan = -1;
		}
	};

	vector<bouncent *> bouncers;

	void newbouncer(vec &from, vec &to, bool local, fpsent *owner, int type, int lifetime, int speed, int gun)
	{
		bouncent &bnc = *(new bouncent());
		bnc.init(from, to, local, owner, type, lifetime, speed, gun, cl.lastmillis);
		bouncers.add(&bnc);
	}

	float middist(fpsent *o, vec &dir, vec &v)
	{
		vec middle = o->o;
		middle.z += (o->aboveeye-o->eyeheight)/2;
		float dist = middle.dist(v, dir);
		dir.div(dist);
		if (dist < 0) dist = 0;
		return dist;
	}

	void radialeffect(fpsent *o, bouncent &bnc)
	{
		vec dir;
		float dist = middist(o, dir, bnc.o);
		if(dist < RL_DAMRAD) hit(o, dir, int(dist*DMF));
	}

	void explode(bouncent &bnc)
	{
		vec dir;
		float dist = middist(cl.player1, dir, bnc.o);
		cl.camerawobble += int(float(guns[bnc.gun].damage)/(dist/RL_DAMRAD/RL_DISTSCALE));

		if (guns[bnc.gun].esound >= 0)
			playsoundv(guns[bnc.gun].esound, bnc.o, dir, RL_DAMRAD);

		particle_splash(0, 200, 300, bnc.o);
		particle_fireball(bnc.o, RL_DAMRAD, bnc.gun == GUN_RL ? 22 : 23);

        adddynlight(bnc.o, 1.15f*RL_DAMRAD, vec(1, 0.75f, 0.5f), 800, 400);

		loopi(rnd(20)+10)
			spawnbouncer(vec(bnc.o).add(vec(bnc.vel)), bnc.vel, bnc.owner, BNC_DEBRIS);

		if (bnc.local)
		{
			hits.setsizenodelete(0);

			loopi(cl.numdynents())
			{
				fpsent *o = (fpsent *)cl.iterdynents(i);
				if (!o || o->state != CS_ALIVE) continue;
				radialeffect(o, bnc);
			}

			cl.cc.addmsg(SV_EXPLODE, "ri3iv", cl.lastmillis-cl.maptime, GUN_GL, bnc.id-cl.maptime,
					hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
		}
	}

	void bounceupdate(int time)
	{
		loopv(bouncers)
		{
			bouncent &bnc = *(bouncers[i]);

			bnc.check(time);
			vec old(bnc.o);
			int rtime = time;

			while (rtime > 0)
			{
				int stime = bnc.bnctype == BNC_SHOT ? 10 : 30, qtime = min(stime, rtime);
				rtime -= qtime;

				if ((bnc.lifetime -= qtime) <= 0 || !bnc.update(cl.lastmillis, time, qtime))
				{
					if (bnc.bnctype == BNC_SHOT && (bnc.gun == GUN_GL || bnc.gun == GUN_RL))
						explode(bnc);
					bnc.state = CS_DEAD;
					break;
				}
			}

			if (bnc.state == CS_DEAD)
			{
				bnc.cleanup();
				delete &bnc;
				bouncers.remove(i--);
			}
		}
	}

	void removebouncers(fpsent *owner)
	{
		loopv(bouncers) if(bouncers[i]->owner==owner) { delete bouncers[i]; bouncers.remove(i--); }
	}

	void bouncereset() { bouncers.deletecontentsp(); bouncers.setsize(0); }

	void damageeffect(int damage, fpsent *d)
	{
		vec p = d->o;
		p.z += 0.6f*(d->eyeheight + d->aboveeye) - d->eyeheight;
		particle_splash(3, damage, 10000, p);
		if(d!=cl.player1)
		{
			s_sprintfd(ds)("@%d", damage);
			particle_text(d->abovehead(), ds, 8);
		}
	}

	void spawnbouncer(vec &p, vec &vel, fpsent *d, int type)
	{
		vec to(rnd(100)-50, rnd(100)-50, rnd(100)-50);
		to.normalize();
		to.add(p);
		newbouncer(p, to, true, d, type, rnd(1000)+1000, rnd(100)+20, -1);
	}

	void superdamageeffect(vec &vel, fpsent *d)
	{
		if(!d->superdamage) return;
		vec from = d->abovehead();
		loopi(rnd(d->superdamage)+1) spawnbouncer(from, vel, d, BNC_GIBS);
	}

	vec hudgunorigin(int gun, const vec &from, const vec &to, fpsent *d)
	{
		vec offset(from);
        if(d!=player1 || cl.gamethirdperson())
        {
            vec front, right;
            vecfromyawpitch(d->yaw, d->pitch, 1, 0, front);
            offset.add(front.mul(d->radius));
			offset.z += (d->aboveeye + d->eyeheight)*0.75f - d->eyeheight;
			vecfromyawpitch(d->yaw, 0, 0, -1, right);
			offset.add(right.mul(0.5f*d->radius));
            return offset;
        }
        offset.add(vec(to).sub(from).normalize().mul(2));
		if(cl.hudgun())
		{
            offset.sub(vec(camup).mul(1.0f));
			offset.add(vec(camright).mul(0.8f));
		}
		return offset;
	}

	void shootv(int gun, vec &from, vec &to, fpsent *d, bool local)	 // create visual effect from a shot
	{
		if (guns[gun].sound >= 0 ) playsound(guns[gun].sound, &d->o, &d->vel);
		switch(gun)
		{
			case GUN_SG:
			{
				loopi(SGRAYS)
				{
					particle_splash(0, 20, 250, sg[i]);
                    particle_flare(hudgunorigin(gun, from, sg[i], d), sg[i], 300, 10);
				}
				break;
			}

			case GUN_CG:
			case GUN_PISTOL:
			{
				particle_splash(0, 200, 250, to);
				//particle_trail(1, 10, from, to);
                particle_flare(hudgunorigin(gun, from, to, d), to, 600, 10);
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
				newbouncer(from, up, local, d, BNC_SHOT, guns[gun].time, guns[gun].speed, gun);
				break;
			}

			case GUN_RIFLE:
				particle_splash(0, 200, 250, to);
				particle_trail(21, 500, hudgunorigin(gun, from, to, d), to);
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
		}
		else if((o = intersectclosest(from, to, d)))
		{
			hitpush(o, from, to);
			shorten(from, o->o, to);
		}
	}

	void reload(fpsent *d)
	{
		if (d->canreload(d->gunselect, cl.lastmillis))
		{
			d->gunlast[d->gunselect] = cl.lastmillis;
			d->gunwait[d->gunselect] = guns[d->gunselect].rdelay;
			cl.cc.addmsg(SV_RELOAD, "ri2", cl.lastmillis-cl.maptime, d->gunselect);
			cl.playsoundc(S_RELOAD);
		}
	}

	void shoot(fpsent *d, vec &targ)
	{
		if (!d->canshoot(d->gunselect, cl.lastmillis)) return;

		d->lastattackgun = d->gunselect;
		d->gunlast[d->gunselect] = cl.lastmillis;
		d->gunwait[d->gunselect] = guns[d->gunselect].adelay;
		d->ammo[d->gunselect]--;
		d->totalshots += guns[d->gunselect].damage*(d->gunselect == GUN_SG ? SGRAYS : 1);

		vec from = d->o;
		vec to = targ;

		vec unitv;
		float dist = to.dist(from, unitv);
		unitv.div(dist);
		vec kickback(unitv);
		kickback.mul(guns[d->gunselect].kick);
		d->vel.add(kickback);
		if (d == player1) cl.camerawobble += guns[d->gunselect].wobble;
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
		if(!guns[d->gunselect].speed) raydamage(from, to, d);

		shootv(d->gunselect, from, to, d, true);

		if(d == player1)
		{
            cl.cc.addmsg(SV_SHOOT, "ri2i6iv", cl.lastmillis-cl.maptime, d->gunselect,
							(int)(from.x*DMF), (int)(from.y*DMF), (int)(from.z*DMF),
							(int)(to.x*DMF), (int)(to.y*DMF), (int)(to.z*DMF),
							hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
		}
	}

	void renderbouncers()
	{
		vec color, dir;
		loopv(bouncers)
		{
			bouncent &bnc = *(bouncers[i]);
			float yaw = bnc.yaw, pitch = bnc.pitch;
			string mname;
			int cull = MDL_CULL_VFC|MDL_DYNSHADOW;

			lightreaching(vec(bnc.o).sub(bnc.vel), color, dir);

            if (bnc.bnctype == BNC_SHOT)
            {
            	if (bnc.gun == GUN_GL)
            	{
            		yaw = pitch = bnc.roll;
            		s_sprintf(mname)("%s", "projectiles/grenade");
            	}
            	else if (bnc.gun == GUN_RL)
            	{
					s_sprintf(mname)("%s", "projectiles/rocket");
            	}
            	else
            	{
					s_sprintf(mname)("%s", "projectiles/frag");
            	}
			}
            else if (bnc.bnctype == BNC_GIBS)
            {
				s_sprintf(mname)("%s", ((int)(size_t)&bnc)&0x40 ? "gibc" : "gibh");
				cull |= MDL_CULL_DIST;
			}
			else if (bnc.bnctype == BNC_DEBRIS)
			{
				s_sprintf(mname)("debris/debris0%d", ((((int)(size_t)&bnc)&0xC0)>>6)+1);
				cull |= MDL_CULL_DIST;
			}
			else continue;

			rendermodel(color, dir, mname, ANIM_MAPMODEL|ANIM_LOOP, 0, 0, bnc.o, yaw+90, pitch, 0, 0, 0, NULL, cull);
		}
	}

	void dynlightbouncers()
	{
		loopv(bouncers)
		{
			bouncent &bnc = *(bouncers[i]);

			switch (bnc.gun)
			{
				case GUN_RL:
					adddynlight(bnc.o, RL_DAMRAD, vec(0.55f, 0.33f, 0.11f), 0, 0);
					break;
				default:
					break;
			}
		}
	}
};
