// weapon.h: all shooting and effects code, projectile management

struct projectiles
{
	GAMECLIENT &cl;

	projectiles(GAMECLIENT &_cl) : cl(_cl)
	{
	}

	struct projent : dynent
	{
		vec from, to;
		int lifetime;
		float movement, roll;
		bool local;
		int projtype;
		float elasticity, relativity, waterfric;
		int gun, schan, id;
        entitylight light;
        fpsent *owner;

		projent() : id(-1)
		{
			schan = -1;
			reset();
		}
		~projent()
		{
			if (issound(schan)) removesound(schan);
			schan = -1;
		}

		void init(vec &_f, vec &_t, bool _b, fpsent *_o, int _n, int _i, int _s, int _g, int _l)
		{
			state = CS_ALIVE;

			o = from = _f;
			to = _t;

			local = _b;
			projtype = _n;
			lifetime = _i;
			gun = _g;
			maxspeed = _s;
			id = _l;
            movement = 0;
            owner = _o;

			vec dir(vec(vec(to).sub(from)).normalize());

			switch (projtype)
			{
				case PRJ_SHOT:
				{
					switch (gun)
					{
						case GUN_GL:
							aboveeye = height = radius = 1.0f;
							elasticity = 0.33f;
							relativity = 0.5f;
							waterfric = 2.0f;
							break;
						case GUN_RL:
						{
							aboveeye = height = radius = 2.0f;
							elasticity = 0.0f;
							relativity = 0.25f;
							waterfric = 1.5f;
							break;
						}
						case GUN_FLAMER:
						{
							aboveeye = height = radius = 2.0f;
							elasticity = 0.01f;
							relativity = 0.25f;
							waterfric = 1.5f;
							vec v(rnd(101)-50, rnd(101)-50, rnd(101)-50);
							if(v.magnitude()>50) v.div(50);
							v.mul(to.dist(from)*0.005f);
							v.z /= 2;
							dir = to;
							dir.add(v);
							dir.sub(from);
							dir.normalize();
							break;
						}
						case GUN_PISTOL:
						case GUN_SG:
						case GUN_CG:
						case GUN_RIFLE:
						default:
						{
							aboveeye = height = radius = 1.0f;
							elasticity = 0.5f;
							relativity = 0.25f;
							waterfric = 1.25f;
							break;
						}
					}
					break;
				}
				case PRJ_GIBS:
				{
					aboveeye = height = radius = 1.0f;
					elasticity = 0.25f;
					relativity = 1.0f;
					waterfric = 2.0f;
					break;
				}
				case PRJ_DEBRIS:
				default:
				{
					aboveeye = height = radius = 1.0f;
					elasticity = 0.66f;
					relativity = 1.0f;
					waterfric = 1.75f;
					break;
				}
			}

			vectoyawpitch(dir, yaw, pitch);
			vel = vec(vec(dir).mul(maxspeed)).add(vec(_o->vel).mul(relativity));
			dir.mul(2);
			o.add(vec(dir).mul(radius)); // a push to get out of the way
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
				if (guntype[gun].fsound >= 0 && !issound(schan))
				{
					schan = playsound(guntype[gun].fsound, &o, 255, 0, 0, SND_LOOP);
				}

				regular_particle_splash(5, 1, 500, o);
				if(gun == GUN_FLAMER)
				{
					float life = (guntype[gun].time-lifetime)/float(guntype[gun].time);
					int col = (int(255*max(1.f-life,0.f))<<16)|(int(127*max(1.f-life,0.f))<<8),
						fade = int(500*life)+1;
					part_splash(4, rnd(2)+1, fade, o, col, 8.0f);
				}
            }
			else if (projtype == PRJ_GIBS) particle_splash(3, 1, 10000, o);
		}

		bool update(int qtime)
		{
            int mat = lookupmaterial(vec(o.x, o.y, o.z + (aboveeye - height)/2));
            bool water = isliquid(mat);
			float secs = float(qtime) / 1000.0f;
		    vec old(o);

			if (elasticity > 0.f) vel.sub(vec(0, 0, float(getvar("gravity"))*secs));

			vec dir(vel);
			if (water)
			{
				if(gun == GUN_FLAMER)
				{
					movement = 0;
					return false; // gets "put out"
				}
				dir.div(waterfric);
			}
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

					vel.apply(pos, elasticity);
					if (hitplayer) vel.influence(pos, hitplayer->vel, elasticity);

					if (movement > 8.0f)
					{
						int mag = int(vel.magnitude()), vol = clamp(mag*10, 1, 255);

						if (vol)
						{
							if (projtype == PRJ_SHOT && guntype[gun].rsound >= 0) playsound(guntype[gun].rsound, &o, vol);
							else if (projtype == PRJ_GIBS) playsound(S_SPLAT, &o, vol);
							else if (projtype == PRJ_DEBRIS) playsound(S_DEBRIS, &o, vol);
						}
					}
                    movement = 0;
					return true; // stay alive until timeout
				}
                movement = 0;
				return false; // die on impact
			}

            float diff = o.dist(old);
            movement += diff;
			if (projtype == PRJ_SHOT && gun == GUN_GL)
            {
                roll += diff / (4*RAD);
                if(roll >= 360) roll = fmod(roll, 360.0f);
            }

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

			if (!proj.owner) proj.state = CS_DEAD;

            if(proj.projtype == PRJ_SHOT && proj.state != CS_DEAD)
            {
                loopj(cl.ph.physicsrepeat)
                {
                    if((proj.lifetime -= cl.ph.physframetime()) <= 0 || !proj.update(cl.ph.physframetime()))
                    {
                        if(guntype[proj.gun].radius)
                            cl.ws.explode(proj.owner, proj.o, proj.vel, proj.id, proj.gun, proj.local);
                        proj.state = CS_DEAD;
                        break;
                    }
                }
            }
            else for(int rtime = curtime; proj.state != CS_DEAD && rtime > 0;)
			{
				int qtime = min(rtime, 30);
				rtime -= qtime;

				if ((proj.lifetime -= qtime) <= 0 || !proj.update(qtime))
				{
					if (proj.projtype == PRJ_SHOT && guntype[proj.gun].radius)
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
            	else continue;
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
					adddynlight(proj.o, 0.66f*guntype[proj.gun].radius, vec(1.1f, 0.66f, 0.22f));
					break;
				case GUN_FLAMER:
					adddynlight(proj.o, 0.66f*guntype[proj.gun].radius, vec(1.1f, 0.22f, 0.02f));
					break;
				default:
					break;
			}
		}
	}
} pj;
