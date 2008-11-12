// projs.h: projectiles 'n stuff

struct projectiles
{
	gameclient &cl;

	IVARA(maxprojectiles, 0, 200, INT_MAX-1);

	projectiles(gameclient &_cl) : cl(_cl)
	{
	}

	vector<projent *> projs;

	void init(projent &proj, bool waited)
	{
		if(proj.projtype == PRJ_SHOT)
			proj.from = proj.o = cl.ws.gunorigin(proj.owner->o, proj.to, proj.owner, proj.owner != cl.player1 || cl.isthirdperson());

		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				proj.aboveeye = proj.height = proj.radius = guntype[proj.attr1].offset;
				proj.elasticity = guntype[proj.attr1].elasticity;
				proj.relativity = guntype[proj.attr1].relativity;
				proj.waterfric = guntype[proj.attr1].waterfric;
				proj.weight = guntype[proj.attr1].weight;
				proj.geomcollide = guntype[proj.attr1].geomcollide;
				proj.playercollide = guntype[proj.attr1].playercollide;
				proj.radial = guntype[proj.attr1].radial;
				proj.extinguish = guntype[proj.attr1].extinguish;
				proj.mdl = guntype[proj.attr1].proj;
				break;
			}
			case PRJ_GIBS:
			{
				proj.mdl = ((int)(size_t)&proj)&0x40 ? "gibc" : "gibh";
				proj.aboveeye = 1.0f;
				proj.elasticity = 0.15f;
				proj.relativity = 1.0f;
				proj.waterfric = 2.0f;
				proj.weight = 50.f;
				proj.vel.add(vec(rnd(40)-21, rnd(40)-21, rnd(40)-21));
				proj.geomcollide = proj.playercollide = 1; // bounce
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
				proj.elasticity = 0.7f;
				proj.relativity = 0.0f;
				proj.waterfric = 1.7f;
				proj.weight = 125.f;
				proj.vel.add(vec(rnd(80)-41, rnd(80)-41, rnd(80)-41));
				proj.geomcollide = proj.playercollide = 1; // bounce
				break;
			}
			case PRJ_ENT:
			{
				proj.mdl = cl.et.entmdlname(proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4, proj.attr5);
				proj.aboveeye = 1.f;
				proj.elasticity = 0.35f;
				proj.relativity = 0.95f;
				proj.waterfric = 1.75f;
				proj.weight = 90.f;
				proj.geomcollide = 1; // bounce
				proj.playercollide = 0; // don't
				proj.o.sub(vec(0, 0, proj.owner->height*0.2f));
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

		vec dir(vec(vec(proj.to).sub(proj.o)).normalize()), orig = proj.o;
		vectoyawpitch(dir, proj.yaw, proj.pitch);
		vec rel = vec(proj.vel).add(dir);
		if(proj.relativity) rel.add(vec(proj.owner->vel).mul(proj.relativity));
		proj.vel = vec(rel).add(vec(dir).mul(proj.maxspeed));
		proj.spawntime = lastmillis;
		proj.to = vec(proj.o).add(proj.vel);

		if(proj.owner)
		{
			if(proj.projtype == PRJ_SHOT && proj.radial)
				proj.height = proj.radius = guntype[proj.attr1].explode;

			for(hitplayer = NULL; !plcollide(&proj) && hitplayer == proj.owner; hitplayer = NULL)
				proj.o.add(vec(proj.vel).mul(0.01f)); // get out of the player

			if(proj.projtype == PRJ_SHOT && proj.radial)
				proj.height = proj.radius = guntype[proj.attr1].offset;
		}

		bool found = false;
		loopi(100)
		{
			if(!collide(&proj) || inside || hitplayer)
			{
				if(hitplayer)
				{
					if(proj.playercollide == 2) break;
					dir = vec(vec(hitplayer->o).sub(proj.o)).normalize();
				}
				else
				{
					if(proj.geomcollide == 2) break;
					dir = wall;
				}
				float amt = proj.elasticity > 0.f ? proj.elasticity : 1.f;
				proj.vel.apply(dir, amt);
				if(hitplayer) proj.vel.apply(hitplayer->vel, amt);
				proj.o.add(vec(proj.vel).mul(0.01f));
			}
			else
			{
				found = true;
				break;
			}
		}
		if(!found)
		{
			proj.o = orig;
			if(proj.playercollide == 2 || proj.geomcollide == 2)
			{
				proj.lifemillis = proj.lifetime = 1;
				proj.state = CS_DEAD; // aww, poor proj
			}
		}
        proj.resetinterp();
	}

	void effect(projent &proj)
	{
		proj.lifespan = clamp((proj.lifemillis-proj.lifetime)/float(proj.lifemillis), 0.f, 1.f);

		if(proj.projtype == PRJ_SHOT)
		{
			if(proj.owner)
				proj.from = cl.ws.gunorigin(proj.owner->o, proj.to, proj.owner, proj.owner != cl.player1 || cl.isthirdperson());

			if(guntype[proj.attr1].fsound >= 0 && !issound(proj.schan))
				playsound(guntype[proj.attr1].fsound, proj.o, &proj, 0, proj.attr1 == GUN_FLAMER ? int(255*proj.lifespan) : -1, -1, -1, &proj.schan);
			switch(proj.attr1)
			{
				case GUN_PLASMA:
				{
					int part = PART_PLASMA;
					if(proj.lifemillis-proj.lifetime < 100) proj.lifesize = clamp((proj.lifemillis-proj.lifetime)/100.f, 0.01f, 1.f);
					else
					{
						part = PART_PLASMA_SOFT;
						proj.lifesize = 1.f;
					}
					part_create(part, 1, proj.o, 0x226688, guntype[proj.attr1].size*proj.lifesize);
					part_create(part, 1, proj.o, 0x44AADD, guntype[proj.attr1].size*proj.lifesize*0.5f); // brighter center part
					break;
				}
				case GUN_FLAMER:
				{
					proj.lifesize = clamp(proj.lifespan, 0.01f, 1.f);
					int col = ((int(254*max(1.0f-proj.lifespan,0.3f))<<16)+1)|((int(86*max(1.0f-proj.lifespan,0.15f))+1)<<8);
					bool moving = (proj.movement > 2.f);
					if(lastmillis-proj.lasteffect > (moving ? 100 : 250))
					{
						part_create(PART_FIREBALL_SOFT, moving ? 100 : 250, proj.o, col, guntype[proj.attr1].size*proj.lifesize);
						proj.lasteffect = lastmillis;
					}
					else part_create(PART_FIREBALL_SOFT, 1, proj.o, col, guntype[proj.attr1].size*proj.lifesize);
					break;
				}
				case GUN_GL:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					int col = ((int(144*max(1.0f-proj.lifespan,0.3f))<<16)+1)|((int(96*max(1.0f-proj.lifespan,0.2f))+1)<<8);
					part_create(PART_PLASMA_SOFT, 1, proj.o, col, proj.radius*2.f*proj.lifesize);
					bool moving = (proj.movement > 4.f);
					if(lastmillis-proj.lasteffect > (moving ? 250 : 500))
					{
						part_create(PART_SMOKE_RISE_SLOW, moving ? 250 : 750, proj.o, 0x666666, proj.radius*(moving ? 1.f : 2.f));
						proj.lasteffect = lastmillis;
					}
					break;
				}
				case GUN_SG: case GUN_CG:
				{
					proj.lifesize = proj.attr1 == GUN_SG ? clamp(proj.lifespan, 0.5f, 1.f) : 1.f;
					float size = proj.attr1 == GUN_SG ? 50.f*proj.lifesize : 20.f;
					vec from = proj.from;
					if(proj.lifemillis-proj.lifetime > 200 || proj.o.dist(proj.from) > size)
						from = vec(proj.o).sub(vec(proj.vel).normalize().mul(size));
					int col = ((int(254*max(1.0f-proj.lifesize,0.3f))<<16)+1)|((int(196*max(1.0f-proj.lifesize,0.15f))+1)<<8);
					part_flare(from, proj.o, 1, PART_STREAK, col, proj.radius*(proj.attr1 == GUN_SG ? 1.5f : 0.5f));
					break;
				}
				case GUN_CARBINE: case GUN_RIFLE:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					part_trail(PART_SMOKE_SINK, 1, proj.o, proj.from, 0xFFFFFF, proj.radius*(proj.attr1 == GUN_CARBINE ? 0.25f : 0.5f));
					break;
				}
				default:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					part_create(PART_PLASMA_SOFT, 1, proj.o, 0xFF9922, proj.radius*0.5f);
					break;
				}
			}
		}
		else if(proj.projtype == PRJ_GIBS)
		{
			proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
			if(lastmillis-proj.lasteffect > 500)
			{
				part_create(PART_BLOOD, 3000, proj.o, 0x66FFFF, 1.2f);
				proj.lasteffect = lastmillis;
			}
		}
		else if(proj.projtype == PRJ_DEBRIS)
		{
			proj.lifesize = clamp(1.f-proj.lifespan, 0.1f, 1.f); // rather, this gets smaller as it gets older
			int steps = int(proj.vel.magnitude()*0.5f*proj.lifesize);
			if(steps)
			{
				vec dir = vec(proj.vel).normalize().neg().mul(proj.radius*0.75f), pos = proj.o;
				loopi(steps)
				{
					float res = float(steps-i)/float(steps);
					int col = ((int(144*max(res,0.3f))<<16)+1)|((int(78*max(res,0.2f))+1)<<8);
					part_create(PART_PLASMA_SOFT, 1, pos, col, proj.radius*proj.lifesize*res);
					pos.add(dir);
				}
			}
			if(lastmillis-proj.lasteffect > 300)
			{
				part_create(PART_SMOKE_RISE_SLOW, 500, vec(proj.o).sub(vec(0, 0, 1)), 0x222222, proj.radius*2.f*proj.lifesize); // smoke
				proj.lasteffect = lastmillis;
			}
		}
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
			if(proj.extinguish)
			{
				proj.movement = 0;
				return false; // gets "put out"
			}
			dir.div(proj.waterfric);
		}
		dir.mul(secs);
		proj.o.add(dir);

		if(!collide(&proj, dir, 0.f, proj.playercollide > 0) || inside || (proj.playercollide && hitplayer))
		{
			proj.o = old;
			vec pos;
			if(proj.playercollide && hitplayer)
			{
				proj.hit = hitplayer;
				pos = vec(vec(hitplayer->o).sub(proj.o)).normalize();
			}
			else pos = wall;

			if((!hitplayer && proj.geomcollide == 1) || (hitplayer && proj.playercollide == 1))
			{
				if(proj.movement > 2.f)
				{
					int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 1, 255);
					if(!hitplayer)
					{
						switch(proj.projtype)
						{
							case PRJ_SHOT:
							{
								switch(proj.attr1)
								{
									case GUN_SG:
									{
										part_splash(PART_SPARK, 10, 500, proj.o, 0xFF9922, proj.radius*(proj.attr1 == GUN_SG ? 0.5f : 0.25f));
										adddecal(DECAL_BULLET, proj.o, vec(proj.vel).neg().normalize(), proj.radius*(proj.attr1 == GUN_SG ? 1.f : 0.5f));
										break;
									}
									case GUN_FLAMER:
									{
										adddecal(DECAL_SCORCH, proj.o, wall, guntype[proj.attr1].size*proj.lifesize);
										adddecal(DECAL_ENERGY, proj.o, wall, guntype[proj.attr1].size*proj.lifesize*0.5f, bvec(92, 24, 8));
										break;
									}
									default: break;
								}
								break;
							}
							case PRJ_GIBS:
							{
								adddecal(DECAL_BLOOD, proj.o, wall, proj.radius*clamp(proj.vel.magnitude(), 0.5f, 4.f), bvec(100, 255, 255));
								break;
							}
							default: break;
						}
					}

					if(vol)
					{
						if(proj.projtype == PRJ_SHOT && guntype[proj.attr1].rsound >= 0)
							playsound(guntype[proj.attr1].rsound, proj.o, &proj, 0, vol);
						else if(proj.projtype == PRJ_GIBS) playsound(S_SPLAT, proj.o, &proj, 0, vol);
						else if(proj.projtype == PRJ_DEBRIS) playsound(S_DEBRIS, proj.o, &proj, 0, vol);
					}
				}
				if(proj.elasticity > 0.f)
				{
					proj.vel.apply(pos, proj.elasticity);
					if(hitplayer)
						proj.vel.apply(hitplayer->vel, proj.elasticity*proj.relativity*0.1f);
				}
				else proj.vel = vec(0, 0, 0);
				proj.movement = 0;
				return true; // stay alive until timeout
			}
			proj.vel = vec(pos).mul(proj.radius).neg();
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
			proj.roll = 0.f;
			if(proj.pitch < 0.f) { proj.pitch += diff; if(proj.pitch > 0.f) proj.pitch = 0.f; }
			if(proj.pitch > 0.f) { proj.pitch -= diff; if(proj.pitch < 0.f) proj.pitch = 0.f; }
		}

		return true;
	}

    bool move(projent &proj)
    {
        if(cl.ph.physsteps <= 0)
        {
            cl.ph.interppos(&proj);
            return true;
        }

        bool alive = true;
        proj.o = proj.newpos;
        proj.o.z += proj.height;
        loopi(cl.ph.physsteps-1)
        {
            if(((proj.lifetime -= cl.ph.physframetime()) <= 0 && proj.lifemillis) || !move(proj, cl.ph.physframetime()))
            {
                alive = false;
                break;
            }
        }
        proj.deltapos = proj.o;
        if(alive && (((proj.lifetime -= cl.ph.physframetime()) <= 0 && proj.lifemillis) || !move(proj, cl.ph.physframetime())))
			alive = false;
        proj.newpos = proj.o;
        proj.deltapos.sub(proj.newpos);
        proj.newpos.z -= proj.height;
        cl.ph.interppos(&proj);
        return alive;
    }

	void create(vec &from, vec &to, bool local, gameent *d, int type, int lifetime, int waittime, int speed, int id = 0, int ent = 0, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0)
	{
		if(!d || !speed) return;

		projent &proj = *(new projent());
		proj.o = proj.from = from;
		proj.to = to;
		proj.local = local;
		proj.projtype = type;
		proj.addtime = lastmillis;
		proj.lifetime = proj.lifemillis = lifetime;
		proj.waittime = waittime;
		proj.ent = ent;
		proj.attr1 = attr1;
		proj.attr2 = attr2;
		proj.attr3 = attr3;
		proj.attr4 = attr4;
		proj.attr5 = attr5;
		proj.maxspeed = speed;
		if(id) proj.id = id;
		else proj.id = lastmillis;
		proj.lastradial = lastmillis;
		proj.movement = 0;
		proj.owner = d;
		if(!waittime) init(proj, false);
		projs.add(&proj);
	}

	void drop(gameent *d, int g, int n, int delay = 0)
	{
		if(n >= 0)
		{
			if(cl.et.ents.inrange(n))
			{
				gameentity &e = (gameentity &)*cl.et.ents[n];
				create(d->o, d->o, d == cl.player1 || d->ai, d, PRJ_ENT, 0, delay, 20, n, e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
			}
		}
		else if(g == GUN_GL)
			create(d->o, d->o, d == cl.player1 || d->ai, d, PRJ_SHOT, 500, 50, 5, -1, WEAPON, d->gunselect);
	}

	struct hitmsg
	{
		int flags, target, lifesequence, info;
		ivec dir;
	};
	vector<hitmsg> hits;

	int hitzones(vec &o, vec &pos, float height, float above, int radius = 0)
	{
		int hits = 0;
		float zones[3][2] = {
			{ pos.z-height*0.75f, pos.z-height },
			{ pos.z-above, pos.z-height*0.75f },
			{ pos.z+above, pos.z-above }
		};
		if(o.z <= zones[0][0]+radius && o.z >= zones[0][1]-radius) hits |= HIT_LEGS;
		if(o.z <= zones[1][0]+radius && o.z >= zones[1][1]-radius) hits |= HIT_TORSO;
		if(o.z <= zones[2][0]+radius && o.z >= zones[2][1]-radius) hits |= HIT_HEAD;
		return hits;
	}

	void hitpush(gameent *d, projent &proj, int flags = 0, int dist = 0, int radius = 0)
	{
		vec pos = cl.headpos(d);
		int f = flags|(hitzones(proj.o, pos, d->height, d->aboveeye, radius));
		hitmsg &h = hits.add();
		h.flags = f;
		h.target = d->clientnum;
		h.lifesequence = d->lifesequence;
		h.info = dist;
		h.dir = ivec(int(proj.vel.x*DNF), int(proj.vel.y*DNF), int(proj.vel.z*DNF));
	}

	void radialeffect(gameent *d, projent &proj, int flags, int radius)
	{
		vec dir, middle = d->o;
		middle.z += (d->aboveeye-d->height)/2;
		float dist = middle.dist(proj.o, dir);
		dir.div(dist);
		if(dist < 0) dist = 0;
		if(dist < radius) hitpush(d, proj, flags, int(dist*DMF), radius);
	}

	void radiate(projent &proj)
	{
		if(lastmillis-proj.lastradial > 250) // for the flamer this results in at most 20 damage per second
		{
			hits.setsizenodelete(0);

			int radius = int(guntype[proj.attr1].explode*proj.lifesize);

			loopi(cl.numdynents())
			{
				gameent *f = (gameent *)cl.iterdynents(i);
				if(!f || f->state != CS_ALIVE || lastmillis-f->lastspawn <= REGENWAIT) continue;
				radialeffect(f, proj, HIT_BURN, radius);
			}

			if(hits.length() > 0)
			{
				cl.cc.addmsg(SV_DESTROY, "ri5iv", proj.owner->clientnum, lastmillis-cl.maptime, proj.attr1, proj.id >= 0 ? proj.id-cl.maptime : proj.id,
						radius, hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
				proj.lastradial = lastmillis;
			}
		}
	}

	void destroy(projent &proj)
	{
		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				if(proj.owner)
					proj.from = cl.ws.gunorigin(proj.owner->o, proj.to, proj.owner, proj.owner != cl.player1 || cl.isthirdperson());
				if(guntype[proj.attr1].esound >= 0) playsound(guntype[proj.attr1].esound, proj.o);
				switch(proj.attr1)
				{
					case GUN_PLASMA:
					{
						part_create(PART_PLASMA_SOFT, 200, proj.o, 0x226688, guntype[proj.attr1].size*0.8f); // plasma explosion
						part_create(PART_SMOKE_RISE_SLOW, 500, proj.o, 0x88AABB, guntype[proj.attr1].explode*0.4f); // smoke
						adddynlight(proj.o, 1.15f*guntype[proj.attr1].explode, vec(0.1f, 0.4f, 0.6f), 200, 10);
						adddecal(DECAL_ENERGY, proj.o, vec(proj.vel).neg().normalize(), guntype[proj.attr1].size, bvec(86, 196, 244));
						break;
					}
					case GUN_FLAMER:
					case GUN_GL:
					{
						part_create(proj.attr1 == GUN_FLAMER ? PART_FIREBALL_SOFT : PART_PLASMA_SOFT, proj.attr1 == GUN_FLAMER ? 500 : 750, proj.o, 0x663603, guntype[proj.attr1].explode*0.4f); // corona
						int deviation = int(guntype[proj.attr1].explode*0.35f);
						loopi(rnd(3)+1)
						{
							vec to = vec(proj.o).add(vec(rnd(deviation*2)-deviation, rnd(deviation*2)-deviation, rnd(deviation*2)-deviation));
							part_splash(PART_FIREBALL_SOFT, proj.attr1 == GUN_FLAMER ? 1 : 2, proj.attr1 == GUN_FLAMER ? 250 : 750, to, 0x441404, guntype[proj.attr1].explode*0.9f); // fireball
							part_splash(PART_SMOKE_RISE_SLOW_SOFT, proj.attr1 == GUN_FLAMER ? 1 : 2, proj.attr1 == GUN_FLAMER ? 500 : 1500, vec(to).sub(vec(0, 0, 2)), proj.attr1 == GUN_FLAMER ? 0x555555 : 0x111111, guntype[proj.attr1].explode); // smoke
						}
						adddynlight(proj.o, 1.15f*guntype[proj.attr1].explode, vec(1.1f, 0.22f, 0.02f), proj.attr1 == GUN_FLAMER ? 250 : 1500, 10);
						if(proj.attr1 == GUN_GL)
						{
							cl.quakewobble += max(int(guntype[proj.attr1].damage*(1.f-camera1->o.dist(proj.o)/EXPLOSIONSCALE/guntype[proj.attr1].explode)), 1);
							part_fireball(vec(proj.o).sub(vec(0, 0, 2)), guntype[proj.attr1].explode*0.75f, PART_EXPLOSION, 1000, 0x642404, 4.f); // explosion fireball
							loopi(rnd(20)+10)
								create(proj.o, vec(proj.o).add(proj.vel), true, proj.owner, PRJ_DEBRIS, rnd(1500)+1500, rnd(750), rnd(60)+40);
						}
						adddecal(DECAL_SCORCH, proj.o, proj.attr1 == GUN_GL ? vec(0, 0, 1) : vec(proj.vel).neg().normalize(), guntype[proj.attr1].explode);
						adddecal(DECAL_ENERGY, proj.o, proj.attr1 == GUN_GL ? vec(0, 0, 1) : vec(proj.vel).neg().normalize(), guntype[proj.attr1].explode*0.75f, bvec(196, 78, 24));
						break;
					}
					case GUN_SG: case GUN_CG:
					{
						if(proj.lifetime)
							part_splash(PART_SPARK, 10, 500, proj.o, 0xFF9922, proj.radius*(proj.attr1 == GUN_SG ? 0.5f : 0.25f));
						adddecal(DECAL_BULLET, proj.o, vec(proj.vel).neg().normalize(), proj.radius*(proj.attr1 == GUN_SG ? 1.f : 0.5f));
						break;
					}
					case GUN_CARBINE: case GUN_RIFLE:
					{
						part_splash(PART_SMOKE_RISE_SLOW, 5, 500, proj.o, 0xFFFFFF, proj.radius*(proj.attr1 == GUN_CARBINE ? 0.25f : 0.5f));
						part_trail(PART_SMOKE_SINK, 500, proj.o, proj.from, 0xFFFFFF, proj.radius*(proj.attr1 == GUN_CARBINE ? 0.25f : 0.25f));
						adddecal(DECAL_BULLET, proj.o, vec(proj.vel).neg().normalize(), proj.radius*(proj.attr1 == GUN_CARBINE ? 1.f : 2.f));
						break;
					}
				}
				if(proj.local)
				{
					hits.setsizenodelete(0);

					if(proj.radial)
					{
						loopi(cl.numdynents())
						{
							gameent *f = (gameent *)cl.iterdynents(i);
							if(!f || f->state != CS_ALIVE || lastmillis-f->lastspawn <= REGENWAIT) continue;
							radialeffect(f, proj, proj.attr1 == GUN_GL ? HIT_EXPLODE : HIT_BURN, guntype[proj.attr1].explode);
						}
					}
					else if(proj.hit && proj.hit->type == ENT_PLAYER)
						hitpush((gameent *)proj.hit, proj);

					cl.cc.addmsg(SV_DESTROY, "ri5iv", proj.owner->clientnum, lastmillis-cl.maptime, proj.attr1, proj.id >= 0 ? proj.id-cl.maptime : proj.id,
							0, hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
				}
				break;
			}
			case PRJ_ENT:
			{
				if(!proj.beenused)
				{
					regularshape(PART_PLASMA_SOFT, int(proj.radius), 0x888822, 53, 50, 200, proj.o, 2.f);
					if(proj.local)
						cl.cc.addmsg(SV_DESTROY, "ri6", proj.owner->clientnum, lastmillis-cl.maptime, -1, proj.id, 0, 0);
				}
				break;
			}
		}
	}

	void update()
	{
		int numprojs = projs.length();
		if(numprojs > maxprojectiles())
		{
			vector<projent *> canremove;
			loopvrev(projs)
				if(projs[i]->ready() && (projs[i]->projtype == PRJ_DEBRIS || projs[i]->projtype == PRJ_GIBS))
					canremove.add(projs[i]);

			while(!canremove.empty() && numprojs > maxprojectiles())
			{
				int oldest = 0;
				loopv(canremove)
					if(lastmillis-canremove[i]->addtime > lastmillis-canremove[oldest]->addtime)
						oldest = i;
				if(canremove.inrange(oldest))
				{
					canremove[oldest]->state = CS_DEAD;
					canremove.removeunordered(oldest);
					numprojs--;
				}
				else break;
			}
		}

		loopv(projs)
		{
			projent &proj = *projs[i];
			if(!proj.owner || proj.state == CS_DEAD)
			{
				proj.state = CS_DEAD;
				destroy(proj);
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

				effect(proj);

				if(proj.projtype == PRJ_SHOT || proj.projtype == PRJ_ENT)
				{
                    if(!move(proj))
                    {
						proj.state = CS_DEAD;
						destroy(proj);
					}
				}
				else for(int rtime = curtime; proj.state != CS_DEAD && rtime > 0;)
				{
					int qtime = min(rtime, 30);
					rtime -= qtime;

					if(((proj.lifetime -= qtime) <= 0 && proj.lifemillis) || !move(proj, qtime))
					{
						proj.state = CS_DEAD;
						break;
					}
				}
			}

			if(proj.state == CS_DEAD)
			{
				delete &proj;
				projs.removeunordered(i--);
			}
			else if(proj.radial && proj.local && proj.attr1 != GUN_GL) radiate(proj);
		}
	}

	void remove(gameent *owner)
	{
		loopv(projs) if(projs[i]->owner==owner)
        {
            delete projs[i];
            projs.removeunordered(i--);
        }
	}

	void reset()
    {
        projs.deletecontentsp();
        projs.setsize(0);
    }

	void preload()
	{
		const char *mdls[] = {
			"projectiles/grenade",
			"gibc", "gibh",
			"debris/debris01", "debris/debris02",
			"debris/debris03", "debris/debris04",
			""
		};
		for(int i = 0; *mdls[i]; i++) loadmodel(mdls[i], -1, true);
	}

	void render()
	{
		loopv(projs) if(projs[i]->ready() && projs[i]->mdl && *projs[i]->mdl)
		{
			projent &proj = *projs[i];
            if(proj.projtype == PRJ_ENT && !cl.et.ents.inrange(proj.id)) continue;
			rendermodel(&proj.light, proj.mdl, ANIM_MAPMODEL|ANIM_LOOP, proj.o, proj.yaw+90, proj.pitch, proj.roll, MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_DYNSHADOW|MDL_LIGHT|MDL_CULL_DIST);
		}
	}

	void adddynlights()
	{
		loopv(projs) if(projs[i]->ready() && projs[i]->projtype == PRJ_SHOT)
		{
			projent &proj = *projs[i];

			switch(proj.attr1)
			{
				case GUN_PLASMA:
				{
					vec col(0.1f*max(1.0f-proj.lifespan,0.1f), 0.4f*max(1.0f-proj.lifespan,0.1f), 0.6f*max(1.0f-proj.lifespan,0.1f));
					adddynlight(proj.o, guntype[proj.attr1].explode*proj.lifesize, col);
					break;
				}
				case GUN_FLAMER:
				{
					vec col(1.1f*max(1.0f-proj.lifespan,0.1f), 0.25f*max(1.0f-proj.lifespan,0.05f), 0.00f);
					adddynlight(proj.o, guntype[proj.attr1].explode*proj.lifesize, col);
					break;
				}
				default:
					break;
			}
		}
	}
} pj;
