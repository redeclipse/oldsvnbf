// weapon.h: all shooting and effects code, projectile management

struct projectiles
{
	GAMECLIENT &cl;

	projectiles(GAMECLIENT &_cl) : cl(_cl)
	{
	}

	struct projent : physent
	{
		vec from, to;
		int lifetime, lasttime;
		float roll;
		bool local;
		fpsent *owner;
		int bnctype;
		float elasticity, relativity, waterfric;
		int gun, schan, id;

		projent() : schan(-1), id(-1)
		{
			reset();
		}
		~projent() {}

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
			id = lasttime = _l;

			switch (bnctype)
			{
				case PRJ_SHOT:
				{
					switch (gun)
					{
						case GUN_GL:
							aboveeye = height = radius = 0.5f;
							elasticity = 0.33f;
							relativity = 0.5f;
							waterfric = 2.0f;
							break;
						case GUN_RL:
						{
							aboveeye = height = radius = 1.0f;
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
							aboveeye = height = radius = 0.25f;
							elasticity = 0.5f;
							relativity = 0.25f;
							waterfric = 1.25f;
							break;
						}
					}
					if (guntype[gun].fsound >= 0) schan = playsound(guntype[gun].fsound, &o, &vel);
					break;
				}
				case PRJ_GIBS:
				{
					aboveeye = height = radius = 0.66f;
					elasticity = 0.25f;
					relativity = 1.0f;
					waterfric = 2.0f;
					schan = playsound(S_WHIZZ, &o, &vel);
					break;
				}
				case PRJ_DEBRIS:
				default:
				{
					aboveeye = height = radius = 1.25f;
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
            if (bnctype == PRJ_SHOT) regular_particle_splash(5, 1, 500, o);
			else if (bnctype == PRJ_GIBS) particle_splash(3, 1, 10000, o);
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

				if (bnctype != PRJ_SHOT || gun == GUN_GL)
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

						if (millis-lasttime > 500)
						{
							if (bnctype == PRJ_SHOT && guntype[gun].rsound >= 0) playsound(guntype[gun].rsound, &o, &vel, vel.magnitude());
							else if (bnctype == PRJ_GIBS) playsound(S_SPLAT, &o, &vel, vel.magnitude());
							else if (bnctype == PRJ_DEBRIS) playsound(S_DEBRIS, &o, &vel, vel.magnitude());
							lasttime = millis;
						}
					}
					return true; // stay alive until timeout
				}
				return false; // die on impact
			}

			if (bnctype == PRJ_SHOT && gun == GUN_GL) roll += int(vel.magnitude() / time) % 360;

			return true;
		}

		void cleanup()
		{
			if (sndchans.inrange(schan) && sndchans[schan].playing())
				sndchans[schan].stop();

			schan = -1;
		}
	};

	vector<projent *> projs;

	void create(vec &from, vec &to, bool local, fpsent *owner, int type, int lifetime, int speed, int gun)
	{
		projent &bnc = *(new projent());
		bnc.init(from, to, local, owner, type, lifetime, speed, gun, cl.lastmillis);
		projs.add(&bnc);
	}

	void update(int time)
	{
		loopv(projs)
		{
			projent &bnc = *(projs[i]);

			bnc.check(time);
			vec old(bnc.o);
			int rtime = time;

			while (rtime > 0)
			{
				int stime = bnc.bnctype == PRJ_SHOT ? 10 : 30, qtime = min(stime, rtime);
				rtime -= qtime;

				if ((bnc.lifetime -= qtime) <= 0 || !bnc.update(cl.lastmillis, time, qtime))
				{
					if (bnc.bnctype == PRJ_SHOT && (bnc.gun == GUN_GL || bnc.gun == GUN_RL))
						cl.ws.explode(bnc.owner, bnc.o, bnc.vel, bnc.id, bnc.gun, bnc.local);
					bnc.state = CS_DEAD;
					break;
				}
			}

			if (bnc.state == CS_DEAD)
			{
				bnc.cleanup();
				delete &bnc;
				projs.remove(i--);
			}
		}
	}

	void remove(fpsent *owner)
	{
		loopv(projs) if(projs[i]->owner==owner) { delete projs[i]; projs.remove(i--); }
	}

	void reset() { projs.deletecontentsp(); projs.setsize(0); }

	void spawn(vec &p, vec &vel, fpsent *d, int type)
	{
		vec to(rnd(100)-50, rnd(100)-50, rnd(100)-50);
		to.normalize();
		to.add(p);
		create(p, to, true, d, type, rnd(1000)+1000, rnd(100)+20, -1);
	}

	void render()
	{
		vec color, dir;
		loopv(projs)
		{
			projent &bnc = *(projs[i]);
			float yaw = bnc.yaw, pitch = bnc.pitch;
			string mname;
			int cull = MDL_CULL_VFC|MDL_DYNSHADOW;

			lightreaching(vec(bnc.o).sub(bnc.vel), color, dir);

            if (bnc.bnctype == PRJ_SHOT)
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
            else if (bnc.bnctype == PRJ_GIBS)
            {
				s_sprintf(mname)("%s", ((int)(size_t)&bnc)&0x40 ? "gibc" : "gibh");
				cull |= MDL_CULL_DIST;
			}
			else if (bnc.bnctype == PRJ_DEBRIS)
			{
				s_sprintf(mname)("debris/debris0%d", ((((int)(size_t)&bnc)&0xC0)>>6)+1);
				cull |= MDL_CULL_DIST;
			}
			else continue;

			rendermodel(color, dir, mname, ANIM_MAPMODEL|ANIM_LOOP, 0, 0, bnc.o, yaw+90, pitch, 0, 0, 0, NULL, cull);
		}
	}

	void adddynlights()
	{
		loopv(projs)
		{
			projent &bnc = *(projs[i]);

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
} pj;
