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
		int ent, attr1, attr2, attr3, attr4;
		int schan, id;
        entitylight light;
        fpsent *owner;

		projent() : id(-1)
		{
			schan = -1;
			reset();
		}
		~projent()
		{
			removetrackedparticles(this);
			removetrackedsounds(this);
			if(issound(schan)) removesound(schan);
			schan = -1;
		}

		void init()
		{
			state = CS_ALIVE;

			vec dir(vec(vec(to).sub(from)).normalize());

			switch (projtype)
			{
				case PRJ_SHOT:
				{
					switch(attr1)
					{
						case GUN_GL:
							aboveeye = height = radius = 1.0f;
							elasticity = 0.33f;
							relativity = 0.5f;
							waterfric = 2.0f;
							weight = 50.f;
							break;
#if 0
						case GUN_RL:
						{
							aboveeye = height = radius = 2.0f;
							elasticity = 0.0f;
							relativity = 0.25f;
							waterfric = 1.5f;
							weight = 0.f;
							break;
						}
#endif
						case GUN_FLAMER:
						{
							aboveeye = height = radius = 2.0f;
							elasticity = 0.01f;
							relativity = 0.25f;
							waterfric = 1.5f;
							weight = 10.f;
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
							weight = 1.f;
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
					weight = 20.f;
					break;
				}
				case PRJ_ENT:
				{
					aboveeye = 1.f;
					height = enttype[ent].height;
					radius = enttype[ent].radius;
					elasticity = 0.25f;
					relativity = 1.0f;
					waterfric = 1.75f;
					weight = 100.f;
					break;
				}
				case PRJ_DEBRIS:
				default:
				{
					aboveeye = height = radius = 1.0f;
					elasticity = 0.66f;
					relativity = 1.0f;
					waterfric = 1.75f;
					weight = 25.f;
					break;
				}
			}

			vectoyawpitch(dir, yaw, pitch);
			vel = vec(vec(dir).mul(maxspeed)).add(vec(owner->vel).mul(relativity));
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
            if(projtype == PRJ_SHOT)
            {
				if(guntype[attr1].fsound >= 0 && !issound(schan))
				{
					schan = playsound(guntype[attr1].fsound, 0, 255, o, this);
				}

				regular_particle_splash(5, 1, 500, o);
				if(attr1 == GUN_FLAMER)
				{
					float life = (guntype[attr1].time-lifetime)/float(guntype[attr1].time);
					int col = (int(255*max(1.f-life,0.f))<<16)|(int(127*max(1.f-life,0.f))<<8),
						fade = int(500*life)+1;
					regular_part_splash(4, rnd(2)+1, fade, o, col, 8.0f);
				}
            }
			else if(projtype == PRJ_GIBS) regular_particle_splash(3, 1, 10000, o);
		}

		bool update(int qtime)
		{
            int mat = lookupmaterial(vec(o.x, o.y, o.z + (aboveeye - height)/2));
            bool water = isliquid(mat&MATF_VOLUME);
			float secs = float(qtime) / 1000.0f;
		    vec old(o);

			if(elasticity > 0.f) vel.sub(vec(0, 0, float(getvar("gravity"))*secs));

			vec dir(vel);
			if(water)
			{
				if(projtype == PRJ_SHOT && attr1 == GUN_FLAMER)
				{
					movement = 0;
					return false; // gets "put out"
				}
				dir.div(waterfric);
			}
			if(isdeadly(mat&MATF_VOLUME))
			{
				movement = 0;
				return false; // gets "put out"
			}
			dir.mul(secs);
			o.add(dir);

			if(!collide(this, dir) || inside || hitplayer)
			{
				o = old;

				if(projtype != PRJ_SHOT || attr1 == GUN_GL)
				{
					vec pos(wall);

					if(hitplayer) pos = vec(vec(o).sub(hitplayer->o)).normalize();
					vel.apply(pos, elasticity);
					if(hitplayer) vel.influence(pos, hitplayer->vel, elasticity);

					if(movement > 8.0f)
					{
						int mag = int(vel.magnitude()), vol = clamp(mag*10, 1, 255);

						if(vol)
						{
							if(projtype == PRJ_SHOT && guntype[attr1].rsound >= 0) playsound(guntype[attr1].rsound, 0, vol, o, this);
							else if(projtype == PRJ_GIBS) playsound(S_SPLAT, 0, vol, o, this);
							else if(projtype == PRJ_DEBRIS) playsound(S_DEBRIS, 0, vol, o, this);
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
			if(projtype == PRJ_SHOT && attr1 == GUN_GL)
            {
                roll += diff / (4*RAD);
                if(roll >= 360) roll = fmod(roll, 360.0f);
            }

			return true;
		}
	};

	vector<projent *> projs;

	void create(vec &from, vec &to, bool local, fpsent *owner, int type, int lifetime, int speed, int ent = 0, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0)
	{
		projent &proj = *(new projent());
		proj.o = proj.from = from;
		proj.to = to;
		proj.local = local;
		proj.projtype = type;
		proj.lifetime = lifetime;
		proj.ent = ent;
		proj.attr1 = attr1;
		proj.attr2 = attr2;
		proj.attr3 = attr3;
		proj.attr4 = attr4;
		proj.maxspeed = speed;
		proj.id = lastmillis;
		proj.movement = 0;
		proj.owner = owner;
		proj.init();
		projs.add(&proj);
	}

	void update()
	{
		loopv(projs)
		{
			projent &proj = *(projs[i]);

			proj.check();

			if(!proj.owner) proj.state = CS_DEAD;

            if(proj.projtype == PRJ_SHOT && proj.state != CS_DEAD)
            {
                loopj(cl.ph.physicsrepeat)
                {
                    if((proj.lifetime -= cl.ph.physframetime()) <= 0 || !proj.update(cl.ph.physframetime()))
                    {
                        if(guntype[proj.attr1].radius)
                            cl.ws.explode(proj.owner, proj.o, proj.vel, proj.id, proj.attr1, proj.local);
                        proj.state = CS_DEAD;
                        break;
                    }
                }
            }
            else for(int rtime = curtime; proj.state != CS_DEAD && rtime > 0;)
			{
				int qtime = min(rtime, 30);
				rtime -= qtime;

				if((proj.lifetime -= qtime) <= 0 || !proj.update(qtime))
				{
					if(proj.projtype == PRJ_SHOT && guntype[proj.attr1].radius)
						cl.ws.explode(proj.owner, proj.o, proj.vel, proj.id, proj.attr1, proj.local);
					proj.state = CS_DEAD;
					break;
				}
			}

			if(proj.state == CS_DEAD)
			{
				delete &proj;
				projs.remove(i--);
			}
		}
	}

	void remove(fpsent *owner)
	{
		loopv(projs) if(projs[i]->owner==owner)
        {
            delete projs[i];
            projs.remove(i--);
        }
	}

	void reset()
    {
        projs.deletecontentsp();
        projs.setsize(0);
    }

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

            if(proj.projtype == PRJ_SHOT)
            {
            	if(proj.attr1 == GUN_GL)
            	{
            		yaw = pitch = proj.roll;
            		s_sprintf(mname)("%s", "projectiles/grenade");
            	}
#if 0
            	else if(proj.attr1 == GUN_RL)
            	{
					s_sprintf(mname)("%s", "projectiles/rocket");
            	}
#endif
            	else continue;
			}
            else if(proj.projtype == PRJ_GIBS)
            {
				s_sprintf(mname)("%s", ((int)(size_t)&proj)&0x40 ? "gibc" : "gibh");
				cull |= MDL_CULL_DIST;
			}
			else if(proj.projtype == PRJ_DEBRIS)
			{
				s_sprintf(mname)("debris/debris0%d", ((((int)(size_t)&proj)&0xC0)>>6)+1);
				cull |= MDL_CULL_DIST;
			}
			else if(proj.projtype == PRJ_ENT)
			{
				s_sprintf(mname)("%s", cl.et.entmdlname(proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4));
				cull |= MDL_CULL_DIST;
			}
			else continue;

			rendermodel(&proj.light, mname, ANIM_MAPMODEL|ANIM_LOOP, proj.o, yaw+90, pitch, 0, cull);
		}
	}

	void adddynlights()
	{
		loopv(projs) if(projs[i]->projtype == PRJ_SHOT)
		{
			projent &proj = *(projs[i]);

			switch(proj.attr1)
			{
#if 0
				case GUN_RL:
					adddynlight(proj.o, 0.66f*guntype[proj.attr1].radius, vec(1.1f, 0.66f, 0.22f));
					break;
#endif
				case GUN_FLAMER:
					adddynlight(proj.o, 0.66f*guntype[proj.attr1].radius, vec(1.1f, 0.22f, 0.02f));
					break;
				default:
					break;
			}
		}
	}
} pj;
