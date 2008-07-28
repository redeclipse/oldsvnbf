// weapon.h: all shooting and effects code, projectile management

struct projectiles
{
	GAMECLIENT &cl;

	projectiles(GAMECLIENT &_cl) : cl(_cl)
	{
	}

	vector<projent *> projs;

	void init(projent &proj, bool waited)
	{
		if(waited)
		{
			findorientation(proj.owner->o, proj.owner->aimyaw, proj.owner->aimpitch, proj.to);
			proj.o = cl.ws.gunorigin(proj.owner->o, proj.to, proj.owner, proj.owner != cl.player1 || cl.isthirdperson());
		}

		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				switch(proj.attr1)
				{
					case GUN_GL:
					{
						proj.mdl = "projectiles/grenade";
						proj.aboveeye = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						float dist = proj.o.dist(proj.to);
						proj.to.z += dist/8;
						break;
					}
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
						proj.height = proj.radius = guntype[proj.attr1].size*0.05f;
						proj.aboveeye = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						vec v(rnd(81)-40, rnd(81)-40, rnd(81)-50);
						if(v.magnitude()>40) v.div(40);
						v.z /= 2.f;
						proj.to.add(v);
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
				proj.elasticity = 0.15f;
				proj.relativity = 1.0f;
				proj.waterfric = 2.0f;
				proj.weight = 100.f;
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
				proj.weight = 150.f;
				break;
			}
			case PRJ_ENT:
			{
				proj.mdl = cl.et.entmdlname(proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4);
				proj.aboveeye = 1.f;
				proj.elasticity = 0.35f;
				proj.relativity = 0.85f;
				proj.waterfric = 1.75f;
				proj.weight = 100.f;
				proj.o.sub(vec(0, 0, proj.owner->height*0.2f));
				vec v(rnd(101)-50, rnd(101)-50, rnd(101)-50);
				if(v.magnitude()>50) v.div(50);
				v.z /= 2.f;
				proj.to.add(v);
				break;
			}
			default: break;
		}
		if(proj.mdl && *proj.mdl)
		{
			setbbfrommodel(&proj, proj.mdl);
			proj.radius += 1.f;
			proj.xradius += 1.f;
			proj.yradius += 1.f;
			proj.height += proj.projtype == PRJ_ENT ? 4.f : 1.f;
		}

		vec dir(vec(vec(proj.to).sub(proj.o)).normalize());
		vectoyawpitch(dir, proj.yaw, proj.pitch);
		proj.vel = vec(vec(dir).mul(cl.ph.maxspeed(&proj))).add(vec(proj.owner->vel).mul(proj.relativity));

		vec orig = proj.o;
		bool found = false;
		if(proj.elasticity > 0.f)
		{
			loopi(1000)
			{
				if(!collide(&proj) || inside || (proj.projtype != PRJ_ENT && hitplayer))
				{
					if(!hitplayer || hitplayer != proj.owner)
					{
						proj.o = orig;
						vec pos;
						if(hitplayer)
							pos = vec(vec(proj.o).sub(hitplayer->o)).neg().normalize();
						else pos = wall;
						proj.vel.apply(pos, proj.elasticity);
						if(hitplayer) proj.vel.apply(hitplayer->vel, proj.elasticity);
					}
					else proj.o.add(vec(proj.vel).mul(0.001f));
				}
				else
				{
					found = true;
					break;
				}
			}
		}

		if(proj.projtype == PRJ_SHOT && proj.attr1 == GUN_FLAMER)
			proj.radius = proj.height = guntype[proj.attr1].offset;
		else if(!found)
		{
			proj.o = orig;
			cl.ph.entinmap(&proj, false); // failsafe
		}
	}

	bool check(projent &proj)
	{
		if(proj.state == CS_ALIVE && !proj.beenused && proj.waittime <= 0)
		{
			if(proj.projtype == PRJ_SHOT)
			{
				float life = clamp((guntype[proj.attr1].time-proj.lifetime)/float(guntype[proj.attr1].time), 0.05f, 1.0f),
						size = guntype[proj.attr1].size*min(life*2.f,1.f);

				if(guntype[proj.attr1].fsound >= 0 && !issound(proj.schan))
					playsound(guntype[proj.attr1].fsound, 0, proj.attr1 == GUN_FLAMER ? int(255*life) : 255, proj.o, &proj, &proj.schan);

				if(proj.attr1 == GUN_FLAMER)
				{
					int col = ((int(254*max(1.0f-life,0.1f))<<16)+1)|((int(64*max(1.0f-life,0.05f))+1)<<8);
					regular_part_splash(4, rnd(5)+1, 50, proj.o, col, size, int(proj.radius));
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
				regularshape(5, int(proj.radius), 0x443322, 21, rnd(5)+1, 100, proj.o, 2.f);
			}
			else if(proj.projtype == PRJ_GIBS)
				regular_part_splash(0, 1, 5000, proj.o, 0x60FFFF, proj.radius, int(proj.radius), 3);
		}
		return true;
	}

	bool move(projent &proj, int qtime)
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
		if(proj.weight > 0.f)
			proj.vel.z -=  cl.ph.gravityforce(&proj)*secs;
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

		bool playercol = proj.projtype != PRJ_ENT;
		if(!collide(&proj, dir, 0.f, playercol) || inside || (playercol && hitplayer))
		{
			proj.o = old;
			vec pos;
			if(playercol && hitplayer)
				pos = vec(vec(hitplayer->o).sub(proj.o)).normalize();
			else pos = wall;

			if(proj.projtype == PRJ_GIBS || proj.projtype == PRJ_DEBRIS || proj.projtype == PRJ_ENT ||
				(proj.projtype == PRJ_SHOT && (proj.attr1 == GUN_GL || (proj.attr1 == GUN_FLAMER && !hitplayer))))
			{
				if(proj.movement > 2.f)
				{
					int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 1, 255);

					if(proj.projtype == PRJ_GIBS)
						part_splash(0, 1, 10000, proj.o, 0x60FFFF, proj.radius);
					else if(!hitplayer && proj.projtype == PRJ_SHOT && proj.attr1 == GUN_FLAMER)
						adddecal(DECAL_SCORCH, proj.o, wall, guntype[proj.attr1].explode);

					if(vol)
					{
						if(proj.projtype == PRJ_SHOT && guntype[proj.attr1].rsound >= 0)
							playsound(guntype[proj.attr1].rsound, 0, vol, proj.o, &proj);
						else if(proj.projtype == PRJ_GIBS)
							playsound(S_SPLAT, 0, vol, proj.o, &proj);
						else if(proj.projtype == PRJ_DEBRIS)
							playsound(S_DEBRIS, 0, vol, proj.o, &proj);
					}
				}
				if(proj.elasticity > 0.f)
				{
					proj.vel.apply(pos, proj.elasticity);
					if(playercol && hitplayer)
						proj.vel.apply(hitplayer->vel, proj.elasticity*proj.relativity*0.1f);
				}
				else proj.vel = vec(0, 0, 0);
				proj.movement = 0;
				return true; // stay alive until timeout
			}
			proj.vel = vec(pos).mul(proj.radius);
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

	void create(vec &from, vec &to, bool local, fpsent *d, int type, int lifetime, int waittime, int speed, int ent = 0, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0)
	{
		if(!d || !lifetime || !speed) return;

		projent &proj = *(new projent());
		proj.o = proj.from = from;
		proj.to = to;
		proj.local = local;
		proj.projtype = type;
		proj.lifetime = lifetime;
		proj.waittime = waittime;
		proj.ent = ent;
		proj.attr1 = attr1;
		proj.attr2 = attr2;
		proj.attr3 = attr3;
		proj.attr4 = attr4;
		proj.maxspeed = speed;
		proj.id = lastmillis;
		proj.movement = 0;
		proj.owner = d;
		if(!waittime) init(proj, false);
		projs.add(&proj);
	}

	void dropgun(fpsent *d, int gun, int delay)
	{
		if(isgun(gun) && !m_noitems(cl.gamemode, cl.mutators))
		{
			loopvrev(projs)
			{
				projent &proj = *projs[i];
				if(proj.owner == d && proj.projtype == PRJ_ENT && proj.ent == WEAPON)
				{
					proj.beenused = true;
					proj.state = CS_DEAD;
				}
			}
			bool local = d == cl.player1 || d->bot;
			create(d->o, d->o, local, d, PRJ_ENT, INT_MAX-1, delay, 20, WEAPON, gun, guntype[gun].add);
		}
	}

	void update()
	{
		loopv(projs)
		{
			projent &proj = *projs[i];
			if(!proj.owner || proj.state == CS_DEAD || !check(proj))
			{
				proj.state = CS_DEAD;
				if(proj.projtype == PRJ_ENT)
					regularshape(7, int(proj.radius), 0x888822, 21, 50, 100, proj.o, 1.f);
				else if(proj.projtype == PRJ_SHOT && guntype[proj.attr1].explode)
					cl.ws.explode(proj.owner, proj.o, proj.vel, proj.id, proj.attr1, proj.local);
			}
			else
			{
				if(proj.waittime > 0)
				{
					if((proj.waittime -= curtime) <= 0)
					{
						proj.waittime = 0;
						init(proj, true);
					}
					else continue;
				}
				if(proj.projtype == PRJ_SHOT)
				{
					loopj(cl.ph.physicsrepeat)
					{
						if((proj.lifetime -= cl.ph.physframetime()) <= 0 || !move(proj, cl.ph.physframetime()))
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

					if((proj.lifetime -= qtime) <= 0 || !move(proj, qtime))
					{
						if(proj.projtype == PRJ_ENT)
							regularshape(7, int(proj.radius), 0x888822, 21, 50, 100, proj.o, 1.f);
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
		create(p, to, true, d, type, rnd(3000)+1000, 0, rnd(30)+10, -1);
	}

	void render()
	{
		loopv(projs) if(projs[i]->owner && !projs[i]->beenused && projs[i]->waittime <= 0 && projs[i]->state != CS_DEAD && projs[i]->mdl && *projs[i]->mdl)
		{
			projent &proj = *projs[i];
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
			projent &proj = *projs[i];

			switch(proj.attr1)
			{
#if 0
				case GUN_RL:
					adddynlight(proj.o, 1.15f*proj.radius, vec(1.1f, 0.66f, 0.22f));
					break;
#endif
				case GUN_FLAMER:
				{
					float life = clamp((guntype[proj.attr1].time-proj.lifetime)/float(guntype[proj.attr1].time), 0.05f, 1.0f),
							size = guntype[proj.attr1].size*min(life*2.f,1.f)*1.15f;
					vec col(1.1f*max(1.0f-life,0.1f), 0.25f*max(1.0f-life,0.05f), 0.00f);
					adddynlight(proj.o, size, col);
					break;
				}
				default:
					break;
			}
		}
	}
} pj;
