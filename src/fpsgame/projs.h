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
		bool local, beenused;
		int projtype;
		float elasticity, relativity, waterfric;
		int ent, attr1, attr2, attr3, attr4;
		int schan, id;
        entitylight light;
        fpsent *owner;
		const char *mdl;

		projent() : id(-1), mdl(NULL)
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

		void reset()
		{
			type = ENT_BOUNCE;
			state = CS_ALIVE;
			lifetime = ent = attr1 = attr2 = attr3 = attr4 = 0;
			schan = id = -1;
			movement = roll = 0.f;
			beenused = false;
			physent::reset();
		}
	};
	vector<projent *> projs;

	void init(projent &proj)
	{
		vec dir(vec(vec(proj.to).sub(proj.from)).normalize());

		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				switch(proj.attr1)
				{
					case GUN_GL:
						proj.mdl = "projectiles/grenade";
						proj.aboveeye = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						break;
#if 0
					case GUN_RL:
					{
						proj.aboveeye = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						break;
					}
#endif
					case GUN_FLAMER:
					{
						proj.height = proj.radius = guntype[proj.attr1].offset;
						proj.aboveeye = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						vec v(rnd(41)-20, rnd(41)-20, rnd(41)-20);
						if(v.magnitude()>20) v.div(20);
						v.z /= 2.5f;
						dir = proj.to;
						dir.add(v);
						dir.sub(proj.from);
						dir.normalize();
						break;
					}
					case GUN_PISTOL:
					case GUN_SG:
					case GUN_CG:
					case GUN_RIFLE:
					default:
					{
						proj.aboveeye = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						break;
					}
				}
				break;
			}
			case PRJ_GIBS:
			{
				proj.mdl = ((int)(size_t)&proj)&0x40 ? "gibc" : "gibh";
				proj.aboveeye = 1.0f;
				proj.elasticity = 0.25f;
				proj.relativity = 1.0f;
				proj.waterfric = 2.0f;
				proj.weight = 10.f;
				break;
			}
			case PRJ_DEBRIS:
			{
				switch(((((int)(size_t)&proj)&0xC0)>>6)+1)
				{
					case 4: proj.mdl = "debris/debris04"; break;
					case 3: proj.mdl = "debris/debris03"; break;
					case 2: proj.mdl = "debris/debris02"; break;
					case 1: default: proj.mdl = "debris/debris01"; break;
				}
				proj.aboveeye = 1.0f;
				proj.elasticity = 0.66f;
				proj.relativity = 1.0f;
				proj.waterfric = 1.75f;
				proj.weight = 25.f;
				break;
			}
			case PRJ_ENT:
			{
				proj.mdl = cl.et.entmdlname(proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4);
				proj.aboveeye = 1.f;
				proj.elasticity = 0.15f;
				proj.relativity = 1.0f;
				proj.waterfric = 1.75f;
				proj.weight = 75.f;
				break;
			}
			default: break;
		}
		if(proj.mdl && *proj.mdl) setbbfrommodel(&proj, proj.mdl);
		proj.height += 1.f;
		proj.radius += 1.f;
		proj.xradius += 1.f;
		proj.yradius += 1.f;
		vectoyawpitch(dir, proj.yaw, proj.pitch);
		proj.vel = vec(vec(dir).mul(cl.ph.maxspeed(&proj))).add(vec(proj.owner->vel).mul(proj.relativity));
		dir.mul(2);
		proj.o.add(vec(dir).mul(proj.owner->radius)); // a push to get out of the way
	}

	bool check(projent &proj)
	{
		if(proj.projtype == PRJ_SHOT)
		{
			float life = clamp(((guntype[proj.attr1].time-proj.lifetime)*2.f)/float(guntype[proj.attr1].time), 0.25f, 1.0f),
					size = guntype[proj.attr1].size*life;

			if(guntype[proj.attr1].fsound >= 0 && !issound(proj.schan))
			{
				proj.schan = playsound(guntype[proj.attr1].fsound, 0, proj.attr1 == GUN_FLAMER ? int(128*life) : 255, proj.o, &proj);
			}

			if(proj.attr1 == GUN_FLAMER)
			{
				int col = (int(254*(1.25f-life))<<16)|(int(96*(1.2f-life))<<8);
				//regular_part_fireball(proj.o, size, 14, 1, col, size);
				loopi(rnd(5)+1)
					regular_part_splash(4, 1, 50, proj.o, col, size, int(proj.radius), 2);
				loopi(rnd(5)+1)
					regular_part_splash(5, 1, 25, proj.o, 0x231008, 4.f, int(size), 2);

				loopi(cl.numdynents())
				{
					fpsent *f = (fpsent *)cl.iterdynents(i);
					if(!f || f->state != CS_ALIVE || lastmillis-f->lastspawn <= REGENWAIT)
						continue;
					vec dir;
					float dist = cl.ws.middist(f, dir, proj.o);
					if(dist < size) return false;
				}
			}
			else
			{
				loopi(rnd(5)+1)
					regular_part_splash(5, 1, 100, proj.o, 0x231008, size, int(proj.radius), 3);
			}
		}
		else if(proj.projtype == PRJ_GIBS)
			regular_part_splash(0, 1, 10000, proj.o, 0x60FFFF, proj.radius, int(proj.radius), 3);

		return true;
	}

	bool update(projent &proj, int qtime)
	{
		int mat = lookupmaterial(vec(proj.o.x, proj.o.y, proj.o.z + (proj.aboveeye - proj.height)/2));

		if(isdeadly(mat&MATF_VOLUME) || proj.o.z < 0)
		{
			proj.movement = 0;
			return false; // gets destroyed
		}

		bool water = isliquid(mat&MATF_VOLUME);
		float secs = float(qtime) / 1000.0f;
		vec old(proj.o);
		if(proj.weight > 0.f) proj.vel.sub(vec(0, 0, cl.ph.gravityforce(&proj)*secs));
		vec dir(proj.vel);

		if(water)
		{
			if(proj.projtype == PRJ_SHOT && proj.attr1 == GUN_FLAMER)
			{
				proj.movement = 0;
				return false; // gets "put out"
			}
			dir.div(proj.waterfric);
		}
		dir.mul(secs);
		proj.o.add(dir);

		if(!collide(&proj, dir) || inside || hitplayer)
		{
			proj.o = old;

			if(proj.projtype == PRJ_GIBS || proj.projtype == PRJ_DEBRIS || proj.projtype == PRJ_ENT ||
				(proj.projtype == PRJ_SHOT && (proj.attr1 == GUN_GL || (proj.attr1 == GUN_FLAMER && !hitplayer))))
			{
				if(proj.movement > 2.0f)
				{
					int mag = int(proj.vel.magnitude()), vol = clamp(mag*10, 1, 255);

					if(proj.projtype == PRJ_GIBS)
						part_splash(0, 1, 10000, proj.o, 0x60FFFF, proj.radius);
					else if(!hitplayer && proj.projtype == PRJ_SHOT && proj.attr1 == GUN_FLAMER)
						adddecal(DECAL_SCORCH, proj.o, wall, guntype[proj.attr1].explode);

					if(vol)
					{
						if(proj.projtype == PRJ_SHOT && guntype[proj.attr1].rsound >= 0) playsound(guntype[proj.attr1].rsound, 0, vol, proj.o, &proj);
						else if(proj.projtype == PRJ_GIBS) playsound(S_SPLAT, 0, vol, proj.o, &proj);
						else if(proj.projtype == PRJ_DEBRIS) playsound(S_DEBRIS, 0, vol, proj.o, &proj);
					}
				}
				if(proj.elasticity > 0.f)
				{
					vec pos;
					if(hitplayer)
						pos = vec(vec(proj.o).sub(hitplayer->o)).neg().normalize();
					else pos = wall;
					proj.vel.apply(pos, proj.elasticity);
					if(hitplayer) proj.vel.apply(hitplayer->vel, proj.elasticity);
				}
				proj.movement = 0;
				return true; // stay alive until timeout
			}
			proj.vel = vec(vec(proj.o).sub(wall)).normalize().mul(proj.radius);
			proj.movement = 0;
			return false; // die on impact
		}

		float dist = proj.o.dist(old), diff = dist/(4*RAD);
		proj.movement += dist;
		if(proj.projtype == PRJ_SHOT && proj.attr1 == GUN_GL)
		{
			proj.roll += diff;
			if(proj.roll >= 360) proj.roll = fmod(proj.roll, 360.0f);
		}
		else if(proj.projtype == PRJ_ENT && proj.pitch != 0.f)
		{
			if(proj.pitch < 0.f) { proj.pitch += diff; if(proj.pitch > 0.f) proj.pitch = 0.f; }
			if(proj.pitch > 0.f) { proj.pitch -= diff; if(proj.pitch < 0.f) proj.pitch = 0.f; }
		}

		return true;
	}

	void create(vec &from, vec &to, bool local, fpsent *owner, int type, int lifetime, int speed, int ent = 0, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0)
	{
		if(!owner || !lifetime || !speed) return;

		if(type == PRJ_ENT && ent == WEAPON) loopvrev(projs)
		{
			projent &proj = *(projs[i]);
			if(proj.owner == owner && proj.projtype == PRJ_ENT && proj.ent == WEAPON)
			{
				proj.beenused = true;
				proj.state = CS_DEAD;
			}
		}

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
		init(proj);
		projs.add(&proj);
	}

	void update()
	{
		loopv(projs)
		{
			projent &proj = *(projs[i]);

			if(!proj.owner || proj.state == CS_DEAD || !check(proj))
			{
				proj.state = CS_DEAD;
				if(proj.projtype == PRJ_SHOT && guntype[proj.attr1].explode)
					cl.ws.explode(proj.owner, proj.o, proj.vel, proj.id, proj.attr1, proj.local);
			}
			else if(proj.state != CS_DEAD)
			{
				if(proj.projtype == PRJ_SHOT)
				{
					loopj(cl.ph.physicsrepeat)
					{
						if((proj.lifetime -= cl.ph.physframetime()) <= 0 || !update(proj, cl.ph.physframetime()))
						{
							if(guntype[proj.attr1].explode)
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

					if((proj.lifetime -= qtime) <= 0 || !update(proj, qtime))
					{
						if(proj.projtype == PRJ_SHOT && guntype[proj.attr1].explode)
							cl.ws.explode(proj.owner, proj.o, proj.vel, proj.id, proj.attr1, proj.local);
						proj.state = CS_DEAD;
						break;
					}
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

	void useeffects(fpsent *d, fpsent *f)
	{
		loopv(projs) if(projs[i]->projtype == PRJ_ENT && projs[i]->ent == WEAPON && projs[i]->owner && projs[i]->owner == f)
		{
			projent &proj = *projs[i];
			const char *item = cl.et.itemname(proj.ent, proj.attr1, proj.attr2);
			if(item && (d != cl.player1 || cl.isthirdperson()))
				particle_text(d->abovehead(), item, 15);
			playsound(S_ITEMPICKUP, 0, 255, d->o, d);
			d->useitem(lastmillis, proj.ent, proj.attr1, proj.attr2);
			proj.beenused = true;
			proj.state = CS_DEAD;
			return;
		}
	}

	bool usecheck(fpsent *d, vec &m, float eye)
	{
		int n = -1;
		loopv(projs) if(projs[i]->projtype == PRJ_ENT && projs[i]->ent == WEAPON && projs[i]->state != CS_DEAD && !projs[i]->beenused && projs[i]->owner)
		{
			projent &proj = *(projs[i]);
			if(insidesphere(m, eye, d->radius, proj.o, enttype[proj.ent].height, enttype[proj.ent].radius)
				&& d->canuse(proj.ent, proj.attr1, proj.attr2, lastmillis)
				&& (!projs.inrange(n) || proj.o.dist(m) < projs[n]->o.dist(m)))
					n = i;
		}
		if(projs.inrange(n))
		{
			projs[n]->beenused = true;
			cl.cc.addmsg(SV_ITEMUSE, "ri4", d->clientnum, lastmillis-cl.maptime, -1, projs[n]->owner->clientnum);
			return true;
		}
		return false;
	}

	void render()
	{
		loopv(projs) if(projs[i]->owner && !projs[i]->beenused && projs[i]->state != CS_DEAD && projs[i]->mdl && *projs[i]->mdl)
		{
			projent &proj = *(projs[i]);
			float yaw = proj.yaw, pitch = proj.pitch;
            int flags = MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_DYNSHADOW|MDL_LIGHT;

            if(proj.projtype == PRJ_SHOT)
            {
            	if(proj.attr1 == GUN_GL)
            	{
            		yaw = pitch = proj.roll;
            	}
            	else continue;
			}
            else if(proj.projtype == PRJ_GIBS)
            {
				flags |= MDL_CULL_DIST;
			}
			else if(proj.projtype == PRJ_DEBRIS)
			{
				flags |= MDL_CULL_DIST;
			}
			else if(proj.projtype == PRJ_ENT)
			{
				flags |= MDL_CULL_DIST;
			}
			else continue;

			rendermodel(&proj.light, proj.mdl, ANIM_MAPMODEL|ANIM_LOOP, proj.o, yaw+90, pitch, 0, flags);
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
					adddynlight(proj.o, 1.15f*proj.radius, vec(1.1f, 0.66f, 0.22f));
					break;
#endif
				case GUN_FLAMER:
					adddynlight(proj.o, 1.15f*proj.radius, vec(1.1f, 0.22f, 0.02f));
					break;
				default:
					break;
			}
		}
	}
} pj;
