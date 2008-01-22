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
		int lifetime;
		float movement, roll;
		bool local;
		fpsent *owner;
		int projtype;
		float elasticity, relativity, waterfric;
		int gun, schan, id;
        entitylight light;

		projent() : id(-1)
		{
			schan = -1;
			reset();
		}
		~projent()
		{
			if (sounds.inrange(schan) && sounds[schan].inuse) removesound(schan);
			schan = -1;
		}

		void init(vec &_f, vec &_t, bool _b, fpsent *_o, int _n, int _i, int _s, int _g, int _l)
		{
			state = CS_ALIVE;

			o = from = _f;
			to = _t;

			local = _b;
			owner = _o;
			projtype = _n;
			lifetime = _i;
			gun = _g;
			maxspeed = _s;
			id = _l;
            movement = 0;

			switch (projtype)
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
					if (guntype[gun].fsound >= 0)
						schan = playsound(guntype[gun].fsound, &o);
					break;
				}
				case PRJ_GIBS:
				{
					aboveeye = height = radius = 0.66f;
					elasticity = 0.25f;
					relativity = 1.0f;
					waterfric = 2.0f;
					break;
				}
				case PRJ_DEBRIS:
				default:
				{
					aboveeye = height = radius = 1.25f;
					elasticity = 0.66f;
					relativity = 1.0f;
					waterfric = 1.75f;
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

		void check()
		{
            if (projtype == PRJ_SHOT)
            {
				if (guntype[gun].fsound >= 0 && (!sounds.inrange(schan) || !sounds[schan].inuse))
				{
					schan = playsound(guntype[gun].fsound, &o);
				}
            	regular_particle_splash(5, 1, 500, o);
            }
			else if (projtype == PRJ_GIBS) particle_splash(3, 1, 10000, o);
		}

		bool update(int qtime)
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

				if (projtype != PRJ_SHOT || gun == GUN_GL)
				{
					vec pos(wall);

					if (gun == GUN_GL)
					{
						if (hitplayer) pos = vec(vec(o).sub(hitplayer->o)).normalize();
					}

					if (vel.magnitude() > 2.f)
					{
						vel.apply(pos, elasticity);
						if (hitplayer) vel.influence(pos, hitplayer->vel, elasticity);

						if (movement > 2.f)
						{
							if (projtype == PRJ_SHOT && guntype[gun].rsound >= 0) playsound(guntype[gun].rsound, &o, true);
							else if (projtype == PRJ_GIBS) playsound(S_SPLAT, &o, true);
							else if (projtype == PRJ_DEBRIS) playsound(S_DEBRIS, &o, true);
						}
					}
                    movement = 0;
					return true; // stay alive until timeout
				}
                movement = 0;
				return false; // die on impact
			}

            movement += o.dist(old);
			if (projtype == PRJ_SHOT && gun == GUN_GL) roll += int(vel.magnitude() / curtime) % 360;

			return true;
		}
	};

	vector<projent *> projs;

	void create(vec &from, vec &to, bool local, fpsent *owner, int type, int lifetime, int speed, int gun)
	{
		projent &proj = *(new projent());
		proj.init(from, to, local, owner, type, lifetime, speed, gun, lastmillis);
		projs.add(&proj);
	}

	void update()
	{
		loopv(projs)
		{
			projent &proj = *(projs[i]);

			proj.check();
			int rtime = curtime;

			while (rtime > 0)
			{
				int stime = proj.projtype == PRJ_SHOT ? 10 : 30, qtime = min(stime, rtime);
				rtime -= qtime;

				if ((proj.lifetime -= qtime) <= 0 || !proj.update(qtime))
				{
					if (proj.projtype == PRJ_SHOT && (proj.gun == GUN_GL || proj.gun == GUN_RL))
						cl.ws.explode(proj.owner, proj.o, proj.vel, proj.id, proj.gun, proj.local);
					proj.state = CS_DEAD;
					break;
				}
			}

			if (proj.state == CS_DEAD)
			{
				delete &proj;
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
		loopv(projs)
		{
			projent &proj = *(projs[i]);
			float yaw = proj.yaw, pitch = proj.pitch;
			string mname;
            int cull = MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_DYNSHADOW|MDL_LIGHT;

            if (proj.projtype == PRJ_SHOT)
            {
            	if (proj.gun == GUN_GL)
            	{
            		yaw = pitch = proj.roll;
            		s_sprintf(mname)("%s", "projectiles/grenade");
            	}
            	else if (proj.gun == GUN_RL)
            	{
					s_sprintf(mname)("%s", "projectiles/rocket");
            	}
            	else
            	{
					s_sprintf(mname)("%s", "projectiles/frag");
            	}
			}
            else if (proj.projtype == PRJ_GIBS)
            {
				s_sprintf(mname)("%s", ((int)(size_t)&proj)&0x40 ? "gibc" : "gibh");
				cull |= MDL_CULL_DIST;
			}
			else if (proj.projtype == PRJ_DEBRIS)
			{
				s_sprintf(mname)("debris/debris0%d", ((((int)(size_t)&proj)&0xC0)>>6)+1);
				cull |= MDL_CULL_DIST;
			}
			else continue;

			rendermodel(&proj.light, mname, ANIM_MAPMODEL|ANIM_LOOP, proj.o, yaw+90, pitch, 0, cull);
		}
	}

	void adddynlights()
	{
		loopv(projs)
		{
			projent &proj = *(projs[i]);

			switch (proj.gun)
			{
				case GUN_RL:
					adddynlight(proj.o, RL_DAMRAD, vec(0.55f, 0.33f, 0.11f), 0, 0);
					break;
				default:
					break;
			}
		}
	}
} pj;
