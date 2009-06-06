#include "game.h"
namespace projs
{
	struct hitmsg
	{
		int flags, target, id, dist;
		ivec dir;
	};
	vector<hitmsg> hits;
	vector<projent *> projs;

	VARA(maxprojectiles, 0, 300, INT_MAX-1);
	VARP(flamertrail, 0, 1, 1);
	VARP(flamerdelay, 1, 50, INT_MAX-1);
	VARA(flamerlength, 1, 600, INT_MAX-1);
	VARA(flamersmoke, 1, 200, INT_MAX-1);

	int hitzones(vec &o, vec &pos, float height, float above, int radius = 0)
	{
		int hits = 0;
		if(o.z <= (pos.z-height*0.75f)+float(radius)) hits |= HIT_LEGS;
		if(o.z < (pos.z-above)+float(radius) && o.z > (pos.z-height*0.75f)-float(radius))
			hits |= HIT_TORSO;
		if(o.z >= (pos.z-above)-float(radius)) hits |= HIT_HEAD;
		return hits;
	}

	void hitpush(gameent *d, projent &proj, int flags = 0, int dist = 0)
	{
        if(d != proj.owner && proj.owner == game::player1)
			game::player1->lasthit = lastmillis;
		hitmsg &h = hits.add();
		h.flags = flags;
		h.target = d->clientnum;
		h.id = lastmillis-game::maptime;
		h.dist = dist;
		vec dir, middle = d->o;
		middle.z += (d->aboveeye-d->height)/2;
		dir = vec(middle).sub(proj.o).normalize();
		dir.add(vec(proj.vel).normalize()).normalize();
		h.dir = ivec(int(dir.x*DNF), int(dir.y*DNF), int(dir.z*DNF));
	}

	void hitproj(gameent *d, projent &proj)
	{
		int flags = HIT_PROJ;
		flags |= proj.weap != WEAPON_PAINT ? hitzones(proj.o, d->o, d->height, d->aboveeye) : HIT_FULL;
		hitpush(d, proj, flags);
	}

	void radialeffect(gameent *d, projent &proj, bool explode, int radius)
	{
		vec dir, middle = d->o;
		middle.z += (d->aboveeye-d->height)/2;
		float dist = middle.dist(proj.o, dir);
		dir.div(dist);
		if(dist < 0) dist = 0;
		if(dist <= radius)
		{
			int flags = (explode ? HIT_EXPLODE : HIT_BURN)|hitzones(proj.o, d->o, d->height, d->aboveeye, radius);
			hitpush(d, proj, flags, int(dist*DMF));
		}
		else if(explode && dist <= radius*4.f) hitpush(d, proj, HIT_WAVE, int(dist*DMF));
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
    };

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

	void init(projent &proj, bool waited)
	{
		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				if(proj.owner && (proj.owner != game::player1 || waited) && proj.owner->muzzle != vec(-1, -1, -1))
					proj.o = proj.from = proj.owner->muzzle;
				proj.aboveeye = proj.height = proj.radius = 0.1f;
				proj.elasticity = weaptype[proj.weap].elasticity;
				proj.reflectivity = weaptype[proj.weap].reflectivity;
				proj.relativity = weaptype[proj.weap].relativity;
				proj.waterfric = weaptype[proj.weap].waterfric;
				proj.weight = weaptype[proj.weap].weight;
				proj.projcollide = weaptype[proj.weap].collide;
				proj.radial = weaptype[proj.weap].radial;
				proj.extinguish = weaptype[proj.weap].extinguish;
				proj.mdl = weaptype[proj.weap].proj;
				if(proj.weap == WEAPON_PAINT)
				{
					if(m_team(game::gamemode, game::mutators) && proj.owner)
						proj.colour = paintcolours[proj.owner->team];
					else proj.colour = paintcolours[rnd(10)];
				}
				break;
			}
			case PRJ_GIBS:
			{
				if(!kidmode && !game::noblood && !m_paint(game::gamemode, game::mutators))
				{
					if(proj.owner)
					{
						if(proj.owner->state == CS_DEAD || proj.owner->state == CS_WAITING)
							proj.o = ragdollcenter(proj.owner);
						else
						{
							proj.lifemillis = proj.lifetime = 1;
							proj.lifespan = proj.lifesize = 1.f;
							proj.state = CS_DEAD;
							return;
						}
					}
					proj.mdl = rnd(2) ? "gibc" : "gibh";
					proj.aboveeye = 1.0f;
					proj.elasticity = 0.3f;
					proj.reflectivity = 0.f;
					proj.relativity = 1.0f;
					proj.waterfric = 2.0f;
					proj.weight = 100.f;
					proj.vel.add(vec(rnd(40)-21, rnd(40)-21, rnd(40)-21));
					proj.projcollide = BOUNCE_GEOM|BOUNCE_PLAYER|COLLIDE_OWNER;
					break;
				} // otherwise fall through
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
				proj.elasticity = 0.5f;
				proj.reflectivity = 0.f;
				proj.relativity = 0.0f;
				proj.waterfric = 1.7f;
				proj.weight = 120.f;
				proj.vel.add(vec(rnd(80)-41, rnd(80)-41, rnd(160)-81));
				proj.projcollide = BOUNCE_GEOM|BOUNCE_PLAYER|COLLIDE_OWNER;
				break;
			}
			case PRJ_ENT:
			{
				proj.aboveeye = 1.f;
				proj.elasticity = 0.35f;
				proj.reflectivity = 0.f;
				proj.relativity = 0.95f;
				proj.waterfric = 1.75f;
				proj.weight = 120.f;
				proj.projcollide = BOUNCE_GEOM;
				proj.o.sub(vec(0, 0, proj.owner->height*0.2f));
				proj.vel.add(vec(rnd(40)-21, rnd(40)-21, rnd(40)-11));
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

		vec dir = vec(proj.to).sub(proj.o), orig = proj.o;
        float maxdist = dir.magnitude();
        if(maxdist > 1e-3f)
        {
            dir.mul(1/maxdist);
		    vectoyawpitch(dir, proj.yaw, proj.pitch);
        }
        else if(proj.owner)
        {
            proj.yaw = proj.owner->yaw;
            proj.pitch = proj.owner->pitch;
            vecfromyawpitch(proj.yaw, proj.pitch, 1, 0, dir);
        }
		vec rel = vec(proj.vel).add(dir);
		if(proj.owner && proj.relativity) rel.add(vec(proj.owner->vel).add(proj.owner->falling).mul(proj.relativity));
		proj.vel = vec(rel).add(vec(dir).mul(physics::movevelocity(&proj)));
		proj.spawntime = lastmillis;
		proj.movement = 1;

		if(proj.radial && proj.projtype == PRJ_SHOT) proj.height = proj.radius = weaptype[proj.weap].explode*0.1f;
		if(proj.projcollide)
		{
			vec ray(proj.vel);
			ray.normalize();
			int maxsteps = 25;
			float step = 4,
				  barrier = max(raycube(proj.o, ray, step*maxsteps, RAY_CLIPMAT|(proj.projcollide&COLLIDE_TRACE ? RAY_ALPHAPOLY : RAY_POLY))-0.1f, 1e-3f),
				  dist = 0;
			loopi(maxsteps)
			{
				proj.hit = NULL;
				float olddist = dist;
				if(dist < barrier && dist + step > barrier) dist = barrier;
				else dist += step;
				if(proj.projcollide&COLLIDE_TRACE)
				{
					proj.o = vec(ray).mul(olddist).add(orig);
					float cdist = tracecollide(proj.o, ray, dist - olddist, RAY_CLIPMAT | RAY_ALPHAPOLY);
					proj.o.add(vec(ray).mul(dist - olddist));
					if(cdist < 0 || dist >= barrier) break;
				}
				else
				{
					proj.o = vec(ray).mul(dist).add(orig);
					if(collide(&proj) && !inside) break;
				}
				if(hitplayer ? proj.projcollide&COLLIDE_PLAYER && hitplayer != proj.owner : proj.projcollide&COLLIDE_GEOM)
				{
					if(hitplayer)
					{
						proj.hit = hitplayer;
						proj.norm = vec(hitplayer->o).sub(proj.o).normalize();
					}
					else proj.norm = proj.projcollide&COLLIDE_TRACE ? hitsurface : wall;
					if(proj.lifemillis)
					{ // fastfwd to end
						proj.lifemillis = proj.lifetime = 1;
						proj.lifespan = proj.lifesize = 1.f;
						proj.state = CS_DEAD;
					}
					break;
				}
			}
		}
		if(proj.radial && proj.projtype == PRJ_SHOT) proj.height = proj.radius = 1.f;
        proj.resetinterp();
	}

	void create(vec &from, vec &to, bool local, gameent *d, int type, int lifetime, int lifemillis, int waittime, int speed, int id, int weap)
	{
		if(!d || !speed) return;

		projent &proj = *new projent;
		proj.o = proj.from = from;
		proj.to = to;
		proj.local = local;
		proj.projtype = type;
		proj.addtime = lastmillis;
		proj.lifetime = lifetime ? m_speedtimex(lifetime) : lifetime;
		proj.lifemillis = lifemillis ? m_speedtimex(lifemillis) : proj.lifetime;
		proj.waittime = waittime ? m_speedtimex(waittime) : waittime;
		proj.maxspeed = speed;
		proj.id = id ? id : lastmillis;
        proj.weap = weap;
		proj.lastradial = lastmillis;
		proj.movement = 0;
		proj.owner = d;
		if(!proj.waittime) init(proj, false);
		projs.add(&proj);
	}

	void drop(gameent *d, int g, int n, bool local)
	{
		if(isweap(g))
		{
			vec from(d->o), to(d->muzzle);
			if(entities::ents.inrange(n))
			{
				if(!m_noitems(game::gamemode, game::mutators) && itemdropping && !(entities::ents[n]->attr[1]&WEAPFLAG_FORCED))
					create(from, to, local, d, PRJ_ENT, 0, 0, 1, 1, n);
				d->ammo[g] = -1;
				d->setweapstate(g, WPSTATE_SWITCH, WEAPSWITCHDELAY, lastmillis);
			}
			else if(g == WEAPON_GRENADE)
			{
				create(from, to, local, d, PRJ_SHOT, 1, weaptype[g].time, 1, 1, -1, g);
				d->ammo[g] = max(d->ammo[g]-1, 0);
				d->setweapstate(g, WPSTATE_SHOOT, weaptype[g].adelay, lastmillis);
			}
			else
			{
				d->ammo[g] = -1;
				d->setweapstate(g, WPSTATE_SWITCH, WEAPSWITCHDELAY, lastmillis);
			}
			d->entid[g] = -1;
		}
	}

	void shootv(int weap, int power, vec &from, vector<vec> &locs, gameent *d, bool local)
	{
		int delay = weaptype[weap].delay, millis = delay,
			life = weaptype[weap].time, speed = weaptype[weap].speed;

		if(weaptype[weap].power)
		{
			float amt = clamp(float(clamp(power, 0, weaptype[weap].power))/float(weaptype[weap].power), 0.f, 1.f);
			life = int(float(life)*(1.f-amt));
			if(!life) millis = delay = 1;
		}

		if(weap == WEAPON_FLAMER)
		{
			int ends = lastmillis+(d->weapwait[weap]*2);
			if(issound(d->wschan)) sounds[d->wschan].ends = ends;
			else playsound(weaptype[weap].sound, d->o, d, SND_LOOP, -1, -1, -1, &d->wschan, ends);
		}
		else if(!weaptype[weap].time || life) playsound(weaptype[weap].sound, d->o, d);

		switch(weap)
		{
			case WEAPON_PISTOL:
			{
				part_create(PART_SMOKE_LERP, 200, from, 0xAAAAAA, 1.f, -20);
				part_create(PART_MUZZLE_FLASH, 100, from, 0xFFCC22, 2.f, 0, 0, d);
                adddynlight(from, 32, vec(0.15f, 0.15f, 0.15f), 50, 0, DL_FLASH);
				break;
			}
			case WEAPON_SG:
			{
				part_create(PART_SMOKE_LERP, 500, from, 0x666666, 4.f, -20);
				part_create(PART_MUZZLE_FLASH, 100, from, 0xFFAA00, 4.f, 0, 0, d);
				adddynlight(from, 48, vec(1.1f, 0.77f, 0.22f), 100, 0, DL_FLASH);
				break;
			}
			case WEAPON_SMG:
			{
				part_create(PART_SMOKE_LERP, 100, from, 0x999999, 1.5f, -20);
				part_create(PART_MUZZLE_FLASH, 25, from, 0xFFAA00, 3.f, 0, 0, d);
                adddynlight(from, 32, vec(1.1f, 0.55f, 0.11f), 50, 0, DL_FLASH);
				break;
			}
			case WEAPON_FLAMER:
			{
				part_create(PART_SMOKE_LERP, 250, from, 0x333333, 2.f, -20);
				part_create(PART_FIREBALL, 75, from, 0xFF2200, 2.f, -10, 0, d);
				adddynlight(from, 48, vec(1.1f, 0.33f, 0.01f), 100, 0, DL_FLASH);
				break;
			}
			case WEAPON_PLASMA:
			{
				part_create(PART_SMOKE, 250, from, 0x88AABB, 0.6f, -20);
				part_create(PART_PLASMA, 75, from, 0x226688, 1.2f, 0, 0, d);
				adddynlight(from, 24, vec(0.1f, 0.4f, 0.6f), 75, 0, DL_FLASH);
				break;
			}
			case WEAPON_RIFLE:
			{
				part_create(PART_SMOKE_LERP, 200, from, 0x444444, 0.8f, -40);
				part_create(PART_PLASMA, 75, from, 0x6611FF, 1.5f, 0, 0, d);
                adddynlight(from, 32, vec(0.4f, 0.05f, 1.f), 75, 0, DL_FLASH);
				break;
			}
			case WEAPON_PAINT:
			{
				part_create(PART_SMOKE, 250, from, 0x999999, 0.5f, -20);
				break;
			}
		}
		loopv(locs)
		{
			create(from, locs[i], local, d, PRJ_SHOT, life ? life : 1, weaptype[weap].time, millis, speed, 0, weap);
			millis += delay;
		}
	}

	void radiate(projent &proj, gameent *d)
	{
		if(!proj.lastradial || d || (lastmillis-proj.lastradial >= m_speedtimex(250)))
		{ // for the flamer this results in at most 40 damage per second
			int radius = int(weaptype[proj.weap].explode*proj.radius);
			if(radius > 0)
			{
				hits.setsizenodelete(0);
				if(!d)
				{
					loopi(game::numdynents())
					{
						gameent *f = (gameent *)game::iterdynents(i);
						if(!f || f->state != CS_ALIVE || !physics::issolid(f)) continue;
						radialeffect(f, proj, false, radius);
					}
				}
				else if(d->state == CS_ALIVE && physics::issolid(d)) radialeffect(d, proj, false, radius);
				if(!hits.empty())
				{
					client::addmsg(SV_DESTROY, "ri5iv", proj.owner->clientnum, lastmillis-game::maptime, proj.weap, proj.id >= 0 ? proj.id-game::maptime : proj.id,
							radius, hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
					if(!d) proj.lastradial = lastmillis;
				}
			}
		}
	}

	void effect(projent &proj)
	{
		proj.lifespan = clamp((proj.lifemillis-proj.lifetime)/float(proj.lifemillis), 0.f, 1.f);
		if(proj.projtype == PRJ_SHOT)
		{
			if(proj.owner && proj.owner->muzzle != vec(-1, -1, -1)) proj.from = proj.owner->muzzle;
			if(weaptype[proj.weap].fsound >= 0)
			{
				int vol = int(255*(1.f-proj.lifespan));
				if(issound(proj.schan)) sounds[proj.schan].vol = vol;
				else playsound(weaptype[proj.weap].fsound, proj.o, &proj, SND_LOOP, vol, -1, -1, &proj.schan);
			}
			switch(proj.weap)
			{
				case WEAPON_PISTOL:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender && proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= m_speedtimex(200);
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen, proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(220*max(1.f-proj.lifesize,0.3f))<<16))|((int(160*max(1.f-proj.lifesize,0.2f)))<<8);
						part_flare(proj.to, proj.o, 1, PART_SFLARE, col, weaptype[proj.weap].partsize);
						part_flare(proj.to, proj.o, 1, PART_FFLARE_LERP, col, weaptype[proj.weap].partsize*0.5f);
						part_create(PART_PLASMA_SOFT, 1, proj.o, col, 0.5f);
					}
					break;
				}
				case WEAPON_FLAMER:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender)
					{
						bool effect = false;
						float size = weaptype[proj.weap].partsize*proj.lifesize;
						if(flamertrail && lastmillis-proj.lasteffect >= m_speedtimex(flamerdelay))
						{
							effect = true;
							proj.lasteffect = lastmillis;
						}
						int col = ((int(254*max((1.f-proj.lifespan),0.3f))<<16)+1)|((int(90*max((1.f-proj.lifespan),0.2f))+1)<<8),
							len = effect ? max(int(m_speedtimex(flamerlength)*(1.1f-proj.lifespan)), 1) : 1;
						if(len <= 1) effect = false;
						part_create(effect ? PART_FIREBALL_SOFT : PART_FIREBALL_SOFT, len, proj.o, col, size, -10);
						if(effect && flamersmoke)
							part_create(PART_SMOKE, m_speedtimex(len*flamersmoke/100), vec(proj.o).add(vec(0, 0, size*0.375f)), 0x111111, size*0.75f, -40);
					}
					break;
				}
				case WEAPON_GRENADE:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender)
					{
						int col = ((int(196*max(1.f-proj.lifespan,0.3f))<<16)+1)|((int(96*max(1.f-proj.lifespan,0.2f))+1)<<8);
						part_create(PART_PLASMA_SOFT, 1, proj.o, col, weaptype[proj.weap].partsize*proj.lifesize);
						bool moving = proj.movement > 0.f;
						if(lastmillis-proj.lasteffect >= m_speedtimex(moving ? 250 : 500))
						{
							part_create(PART_SMOKE_LERP, m_speedtimex(moving ? 250 : 750), proj.o, 0x222222, weaptype[proj.weap].partsize*(moving ? 0.5f : 1.f), -20);
							proj.lasteffect = lastmillis;
						}
					}
					break;
				}
				case WEAPON_SG:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender && proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= m_speedtimex(200);
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen, proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(200*max(1.f-proj.lifesize,0.3f))<<16)+1)|((int(120*max(1.f-proj.lifesize,0.1f))+1)<<8);
						part_flare(proj.to, proj.o, 1, PART_SFLARE, col, weaptype[proj.weap].partsize);
						part_flare(proj.to, proj.o, 1, PART_SFLARE_LERP, col, weaptype[proj.weap].partsize*0.5f);
					}
					break;
				}
				case WEAPON_SMG:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender && proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= m_speedtimex(200);
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen, proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(200*max(1.f-proj.lifesize,0.3f))<<16))|((int(100*max(1.f-proj.lifesize,0.1f)))<<8);
						part_flare(proj.to, proj.o, 1, PART_SFLARE, col, weaptype[proj.weap].partsize);
						part_flare(proj.to, proj.o, 1, PART_SFLARE_LERP, col, weaptype[proj.weap].partsize*0.5f);
					}
					break;
				}
				case WEAPON_PLASMA:
				{
					int part = PART_PLASMA_SOFT;
					float resize = 1.f;
					resize = proj.lifesize = 1.f-clamp((proj.lifemillis-proj.lifetime)/float(proj.lifemillis), 0.05f, 1.f);
					if(proj.lifemillis-proj.lifetime < m_speedtimex(100))
					{
						resize = (clamp((proj.lifemillis-proj.lifetime)/float(m_speedtimex(100)), 0.f, 1.f)*0.75f)+0.25f;
						part = PART_PLASMA;
					}
					if(proj.canrender)
					{
						part_create(part, 1, proj.o, 0x226688, weaptype[proj.weap].partsize*resize);
						part_create(PART_ELECTRIC, 1, proj.o, 0x55AAEE, weaptype[proj.weap].partsize*0.7f*resize); // electrifying!
						part_create(PART_PLASMA, 1, proj.o, 0x55AAEE, weaptype[proj.weap].partsize*0.3f*resize); // brighter center part
					}
					break;
				}
				case WEAPON_RIFLE:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender && proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= m_speedtimex(200);
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen, proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(96*max(1.f-proj.lifesize,0.2f))<<16))|((int(16*max(1.f-proj.lifesize,0.2f)))<<8)|(int(254*max(1.f-proj.lifesize,0.2f)));
						part_flare(proj.to, proj.o, 1, PART_FFLARE, col, weaptype[proj.weap].partsize);
						col = ((int(64*max(1.f-proj.lifesize,0.1f))<<16))|((int(8*max(1.f-proj.lifesize,0.1f)))<<8)|(int(196*max(1.f-proj.lifesize,0.1f)));
						part_flare(proj.to, proj.o, 1, PART_SFLARE_LERP, col, weaptype[proj.weap].partsize*0.5f);
					}
					break;
				}
				case WEAPON_PAINT:
				{
					int part = PART_PLASMA;
					if(proj.lifemillis-proj.lifetime < m_speedtimex(250)) proj.lifesize = clamp((proj.lifemillis-proj.lifetime)/float(m_speedtimex(250)), 0.25f, 1.f);
					else
					{
						part = PART_PLASMA_SOFT;
						proj.lifesize = 1.f;
					}
					if(proj.canrender)
					{
						part_create(part, 1, proj.o, proj.colour, weaptype[proj.weap].partsize*proj.lifesize);
						part_create(PART_PLASMA_LERP, 1, proj.o, proj.colour, weaptype[proj.weap].partsize*proj.lifesize*0.5f);
					}
					break;
				}
				default:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender) part_create(PART_PLASMA_SOFT, 1, proj.o, proj.colour, 1.f);
					break;
				}
			}
			if(weaptype[proj.weap].radial || weaptype[proj.weap].taper) proj.radius = max(proj.lifesize, 0.1f);
		}
		else if(proj.projtype == PRJ_GIBS && !kidmode && !game::noblood && !m_paint(game::gamemode, game::mutators))
		{
			proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
			if(proj.canrender && lastmillis-proj.lasteffect >= m_speedtimex(500) && proj.lifetime >= min(proj.lifemillis, 1000))
			{
				if(!kidmode && !game::noblood) part_create(PART_BLOOD, m_speedtimex(5000), proj.o, 0x66FFFF, 2.f, 50, DECAL_BLOOD);
				proj.lasteffect = lastmillis;
			}
		}
		else if(proj.projtype == PRJ_DEBRIS || (proj.projtype == PRJ_GIBS && (kidmode || game::noblood || m_paint(game::gamemode, game::mutators))))
		{
			proj.lifesize = clamp(1.f-proj.lifespan, 0.1f, 1.f); // gets smaller as it gets older
			int steps = clamp(int(proj.vel.magnitude()*proj.lifesize), 5, 15);
			if(proj.canrender && steps && proj.movement > 0.f)
			{
				vec dir = vec(proj.vel).normalize().neg().mul(proj.radius*0.35f), pos = proj.o;
				loopi(steps)
				{
					float res = float(steps-i)/float(steps), size = clamp(proj.radius*0.75f*(proj.lifesize+0.1f)*res, 0.01f, proj.radius);
					int col = ((int(196*max(res,0.3f))<<16)+1)|((int(64*max(res,0.2f))+1)<<8);
					part_create(i ? PART_FIREBALL_SOFT : PART_FIREBALL_SOFT, 1, pos, col, size, -10);
					pos.add(dir);
					if(proj.o.dist(pos) > proj.movement) break;
				}
			}
		}
		proj.canrender = true; // until next frame my friend
	}

	void destroy(projent &proj)
	{
		proj.lifespan = clamp((proj.lifemillis-proj.lifetime)/float(proj.lifemillis), 0.f, 1.f);
		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				if(proj.owner) proj.from = proj.owner->muzzle;
				int vol = 255;
				switch(proj.weap)
				{
					case WEAPON_PISTOL:
					{
						vol = int(255*(1.f-proj.lifespan));
						proj.from = vec(proj.o).sub(proj.vel);
						part_create(PART_SMOKE_LERP, m_speedtimex(200), proj.o, 0x999999, weaptype[proj.weap].partsize, -20);
						adddecal(DECAL_BULLET, proj.o, proj.norm, 2.f);
						break;
					}
					case WEAPON_FLAMER:
					case WEAPON_GRENADE:
					{ // both basically explosions
						int deviation = int(weaptype[proj.weap].explode*(proj.weap == WEAPON_FLAMER ? 0.5f : 0.75f));
						loopi(rnd(7)+4)
						{
							vec to = vec(vec(proj.o).sub(vec(0, 0, weaptype[proj.weap].explode*0.25f))).add(vec(rnd(deviation*2)-deviation, rnd(deviation*2)-deviation, rnd(deviation*2)-deviation));
							part_create(PART_FIREBALL_SOFT, m_speedtimex(proj.weap == WEAPON_FLAMER ? 350 : 500), to, 0x660600, weaptype[proj.weap].explode*(proj.weap == WEAPON_FLAMER ? 0.5f : 1.f), proj.weap == WEAPON_FLAMER ? -10 : 0);
						}
						part_create(PART_SMOKE_LERP_SOFT, m_speedtimex(proj.weap == WEAPON_FLAMER ? 750 : 1500), vec(proj.o).sub(vec(0, 0, weaptype[proj.weap].explode*0.25f)), proj.weap == WEAPON_FLAMER ? 0x303030 : 0x222222, weaptype[proj.weap].explode, -20);
						if(proj.weap == WEAPON_GRENADE)
						{
							part_create(PART_PLASMA_SOFT, m_speedtimex(1000), vec(proj.o).sub(vec(0, 0, weaptype[proj.weap].explode*0.15f)), 0x992200, weaptype[proj.weap].explode*0.5f); // corona
							float wobble = weaptype[proj.weap].damage*(1.f-camera1->o.dist(proj.o)/EXPLOSIONSCALE/weaptype[proj.weap].explode)*0.5f;
							if(proj.weap == m_spawnweapon(game::gamemode, game::mutators)) wobble *= 0.25f;
							game::quakewobble = clamp(game::quakewobble + max(int(wobble), 1), 0, 1000);
							part_fireball(vec(proj.o).sub(vec(0, 0, weaptype[proj.weap].explode*0.25f)), float(weaptype[proj.weap].explode*1.5f), PART_EXPLOSION, m_speedtimex(750), 0xAA3300, 1.f);
							loopi(rnd(26)+10) create(proj.o, vec(proj.o).add(proj.vel), true, proj.owner, PRJ_DEBRIS, rnd(30001)+1500, 0, rnd(1001), rnd(126)+25);
							adddecal(DECAL_ENERGY, proj.o, proj.norm, weaptype[proj.weap].explode*0.7f, bvec(196, 24, 0));
						}
						adddecal(DECAL_SCORCH, proj.o, proj.norm, weaptype[proj.weap].explode);
						adddynlight(proj.o, 1.f*weaptype[proj.weap].explode, vec(1.1f, 0.22f, 0.02f), m_speedtimex(proj.weap == WEAPON_FLAMER ? 250 : 1250), 10);
						break;
					}
					case WEAPON_SG: case WEAPON_SMG:
					{
						vol = int(255*(1.f-proj.lifespan));
						if(proj.lifetime)
							part_splash(PART_SPARK, proj.weap == WEAPON_SG ? 10 : 20, m_speedtimex(500), proj.o, 0xFFAA22, weaptype[proj.weap].partsize*0.75f, 10, 8);
						break;
					}
					case WEAPON_PLASMA:
					{
						part_create(PART_PLASMA_SOFT, m_speedtimex(100), proj.o, 0x226688, weaptype[proj.weap].partsize*proj.lifesize);
						part_create(PART_PLASMA, m_speedtimex(100), proj.o, 0x44AADD, weaptype[proj.weap].partsize*0.5f*proj.lifesize); // brighter center part
						part_create(PART_SMOKE, m_speedtimex(250), proj.o, 0x8899AA, 2.4f, -20);
						adddynlight(proj.o, 1.1f*weaptype[proj.weap].explode, vec(0.1f, 0.4f, 0.6f), m_speedtimex(200), 10);
						adddecal(DECAL_ENERGY, proj.o, proj.norm, 6.f, bvec(86, 196, 244));
						break;
					}
					case WEAPON_RIFLE:
					{
						proj.from = vec(proj.o).sub(proj.vel);
						part_create(PART_SMOKE_LERP, m_speedtimex(100), proj.o, 0x444444, weaptype[proj.weap].partsize, -20);
						adddynlight(proj.o, weaptype[proj.weap].partsize*1.5f, vec(0.4f, 0.05f, 1.f), m_speedtimex(200), 10);
						adddecal(DECAL_SCORCH, proj.o, proj.norm, 4.f);
                        adddecal(DECAL_ENERGY, proj.o, proj.norm, 3.f, bvec(96, 16, 254));
						break;
					}
					case WEAPON_PAINT:
					{
						vol = int(255*(1.f-proj.lifespan));
						proj.from = vec(proj.o).sub(proj.vel);
						int r = 255-(proj.colour>>16), g = 255-((proj.colour>>8)&0xFF), b = 255-(proj.colour&0xFF),
							colour = (r<<16)|(g<<8)|b;
						part_splash(PART_BLOOD, 5, m_speedtimex(5000), proj.o, colour, 2.f, 25, DECAL_BLOOD, 4);
                        adddecal(DECAL_BLOOD, proj.o, proj.norm, rnd(4)+4.f, bvec(r, g, b));
						break;
					}
				}
				if(vol && weaptype[proj.weap].esound >= 0) playsound(weaptype[proj.weap].esound, proj.o, NULL, 0, vol);
				if(proj.local)
				{
					int radius = weaptype[proj.weap].taper ? max(int(weaptype[proj.weap].explode*proj.radius), 1) : weaptype[proj.weap].explode;
					hits.setsizenodelete(0);
					if(weaptype[proj.weap].explode)
					{
						loopi(game::numdynents())
						{
							gameent *f = (gameent *)game::iterdynents(i);
							if(!f || f->state != CS_ALIVE || !physics::issolid(f)) continue;
							radialeffect(f, proj, proj.weap == WEAPON_GRENADE, radius);
						}
					}
					else if(proj.hit && proj.hit->type == ENT_PLAYER)
						hitproj((gameent *)proj.hit, proj);

					client::addmsg(SV_DESTROY, "ri5iv", proj.owner->clientnum, lastmillis-game::maptime, proj.weap, proj.id >= 0 ? proj.id-game::maptime : proj.id,
							-radius, hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
				}
				break;
			}
			case PRJ_ENT:
			{
				if(!proj.beenused)
				{
					if(entities::ents.inrange(proj.id)) game::spawneffect(proj.o, 0x6666FF, enttype[entities::ents[proj.id]->type].radius);
					if(proj.local) client::addmsg(SV_DESTROY, "ri6", proj.owner->clientnum, lastmillis-game::maptime, -1, proj.id, 0, 0);
				}
				break;
			}
			default: break;
		}
	}

	void reflect(projent &proj, vec &pos)
    {
    	if(proj.elasticity > 0.f)
    	{
			vec dir[2]; dir[0] = dir[1] = vec(proj.vel).normalize();
    		float mag = proj.vel.magnitude()*proj.elasticity; // conservation of energy
			loopi(3) if((pos[i] > 0.f && dir[1][i] < 0.f) || (pos[i] < 0.f && dir[1][i] > 0.f))
				dir[1][i] = fabs(dir[1][i])*pos[i];
			if(proj.reflectivity > 0.f)
			{ // if projectile returns at 180 degrees [+/-]reflectivity, skew the reflection
				float aim[2][2] = { { 0.f, 0.f }, { 0.f, 0.f } };
				loopi(2) vectoyawpitch(dir[i], aim[0][i], aim[1][i]);
				loopi(2)
				{
					float rmax = 180.f+proj.reflectivity, rmin = 180.f-proj.reflectivity,
						off = aim[i][1]-aim[i][0];
					if(fabs(off) <= rmax && fabs(off) >= rmin)
					{
						if(off > 0.f ? off > 180.f : off < -180.f)
							aim[i][1] += rmax-off;
						else aim[i][1] -= off-rmin;
					}
					while(aim[i][1] < 0.f) aim[i][1] += 360.f;
					while(aim[i][1] >= 360.f) aim[i][1] -= 360.f;
				}
				vecfromyawpitch(aim[0][1], aim[1][1], 1, 0, dir[1]);
			}
			proj.vel = vec(dir[1]).mul(mag);
    	}
    	else proj.vel = vec(0, 0, 0);
    }

    void bounceeffect(projent &proj)
    {
        if(proj.movement < 2.f && proj.lastbounce) return;
        switch(proj.projtype)
        {
            case PRJ_SHOT:
            {
                switch(proj.weap)
                {
                    case WEAPON_SG: case WEAPON_SMG:
                    {
                        part_splash(PART_SPARK, 5, m_speedtimex(250), proj.o, 0xFFAA22, weaptype[proj.weap].partsize*0.75f, 10, 0, 8);
                        if(!proj.lastbounce)
                            adddecal(DECAL_BULLET, proj.o, proj.norm, proj.weap == WEAPON_SG ? 3.f : 1.5f);
                        break;
                    }
                    case WEAPON_FLAMER:
                    {
                        if(!proj.lastbounce)
                        {
                            adddecal(DECAL_SCORCH, proj.o, proj.norm, weaptype[proj.weap].explode*3.f*proj.lifesize);
                            //adddecal(DECAL_ENERGY, proj.o, proj.norm, weaptype[proj.weap].explode*4.f*proj.lifesize, bvec(184, 88, 0));
                        }
                        break;
                    }
                    default: break;
                }
				int vol = int(255*(1.f-proj.lifespan));
                if(vol && weaptype[proj.weap].rsound >= 0) playsound(weaptype[proj.weap].rsound, proj.o, &proj, 0, vol);
                break;
            }
            case PRJ_GIBS:
            {
            	if(!kidmode && !game::noblood && !m_paint(game::gamemode, game::mutators))
            	{
					if(!proj.lastbounce)
						adddecal(DECAL_BLOOD, proj.o, proj.norm, proj.radius*clamp(proj.vel.magnitude(), 0.5f, 4.f), bvec(100, 255, 255));
					int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 0, 255);
					if(vol) playsound(S_SPLOSH, proj.o, &proj, 0, vol);
					break;
            	} // otherwise fall through
            }
            case PRJ_DEBRIS:
            {
       	        int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 0, 255);
                if(vol) playsound(S_DEBRIS, proj.o, &proj, 0, vol);
                break;
            }
            default: break;
        }
    }

	int bounce(projent &proj, const vec &dir)
	{
		proj.hit = NULL;
		if(!collide(&proj, dir, 0.f, proj.projcollide&COLLIDE_PLAYER) || inside)
		{
			if(hitplayer)
			{
				if((proj.projcollide&COLLIDE_OWNER && (!proj.lifemillis || proj.lastbounce || proj.lifemillis-proj.lifetime >= m_speedtimex(1000))) || hitplayer != proj.owner)
				{
					proj.hit = hitplayer;
					proj.norm = vec(hitplayer->o).sub(proj.o).normalize();
				}
				else return 1;
			}
			else proj.norm = wall;

            if(proj.projcollide&(hitplayer ? BOUNCE_PLAYER : BOUNCE_GEOM))
			{
                bounceeffect(proj);
				reflect(proj, proj.norm);
				proj.movement = 0;
				proj.lastbounce = lastmillis;
				return 2; // bounce
			}
			return 0; // die on impact
		}
		return 1; // live!
	}

    int trace(projent &proj, const vec &dir)
    {
        proj.hit = NULL;
        vec to(proj.o), ray = dir;
        to.add(dir);
        float maxdist = ray.magnitude();
        if(maxdist <= 0) return 1; // not moving anywhere, so assume still alive since it was already alive
        ray.mul(1/maxdist);
        float dist = tracecollide(proj.o, ray, maxdist, RAY_CLIPMAT | RAY_ALPHAPOLY, proj.projcollide&COLLIDE_PLAYER);
        proj.o.add(vec(ray).mul(dist >= 0 ? dist : maxdist));
        if(dist >= 0)
        {
            if(hitplayer)
            {
            	if((proj.projcollide&COLLIDE_OWNER && (!proj.lifemillis || proj.lastbounce || proj.lifemillis-proj.lifetime >= m_speedtimex(1000))) || hitplayer != proj.owner)
            	{
					proj.hit = hitplayer;
					proj.norm = vec(hitplayer->o).sub(proj.o).normalize();
            	}
            	else return 1;
            }
            else proj.norm = hitsurface;

            if(proj.projcollide&(hitplayer ? BOUNCE_PLAYER : BOUNCE_GEOM))
            {
                bounceeffect(proj);
                reflect(proj, proj.norm);
                proj.o.add(vec(proj.norm).mul(0.1f)); // offset from surface slightly to avoid initial collision
                proj.movement = 0;
                proj.lastbounce = lastmillis;
                return 2; // bounce
            }
            return 0; // die on impact
        }
        return 1; // live!
    }

	bool move(projent &proj, int qtime)
	{
		int mat = lookupmaterial(vec(proj.o.x, proj.o.y, proj.o.z + (proj.aboveeye - proj.height)/2));
		if(int(mat&MATF_VOLUME) == MAT_LAVA || int(mat&MATF_FLAGS) == MAT_DEATH || proj.o.z < 0) return false; // gets destroyed
		bool water = isliquid(mat&MATF_VOLUME);
		float secs = float(qtime) / 1000.0f;
		if(proj.weight > 0.f) proj.vel.z -=  physics::gravityforce(&proj)*secs;

		vec dir(proj.vel), pos(proj.o);
		if(water)
		{
			if(proj.extinguish) return false; // gets "put out"
			dir.div(proj.waterfric);
		}
		dir.mul(secs);

        bool blocked = false;
        if(proj.projcollide&COLLIDE_TRACE)
        {
            switch(trace(proj, dir))
            {
                case 2: blocked = true; break;
                case 1: break;
                case 0: return false;
            }
        }
        else
        {
            if(proj.projtype == PRJ_SHOT)
            {
			    float stepdist = dir.magnitude();
                vec ray(dir);
                ray.mul(1/stepdist);
                float barrier = raycube(proj.o, ray, stepdist, RAY_CLIPMAT|RAY_POLY);
                if(barrier < stepdist)
                {
                    proj.o.add(ray.mul(barrier-0.15f));
                    switch(bounce(proj, ray))
                    {
                        case 2: proj.o = pos; blocked = true; break;
                        case 1: proj.o = pos; break;
                        case 0: return false;
                    }
                }
            }
		    if(!blocked)
            {
                proj.o.add(dir);
                switch(bounce(proj, dir))
		        {
			        case 2: proj.o = pos; if(proj.projtype == PRJ_SHOT) blocked = true; break;
			        case 1: default: break;
			        case 0: proj.o = pos; if(proj.projtype == PRJ_SHOT) { dir.rescale(max(dir.magnitude()-0.15f, 0.0f)); proj.o.add(dir); } return false;
		        }
            }
        }

		float dist = proj.o.dist(pos), diff = dist/(4*RAD);
		#define adddiff(q) { q += diff; if(q >= 360) q = fmod(q, 360.0f); }
		if(!blocked) proj.movement += dist;
		if(proj.projtype == PRJ_SHOT)
		{
			switch(proj.weap)
			{
				case WEAPON_GRENADE:
				{
					adddiff(proj.yaw);
					adddiff(proj.pitch);
					adddiff(proj.roll);
					break;
				}
				default:
				{
					vectoyawpitch(vec(proj.vel).normalize(), proj.yaw, proj.pitch);
					adddiff(proj.roll);
					break;
				}
			}
		}
		else if(proj.projtype == PRJ_ENT && proj.pitch != 0.f)
		{
			if(proj.pitch < 0.f) { proj.pitch += diff*0.5f; if(proj.pitch > 0.f) proj.pitch = 0.f; }
			if(proj.pitch > 0.f) { proj.pitch -= diff*0.5f; if(proj.pitch < 0.f) proj.pitch = 0.f; }
			proj.yaw = proj.roll = 0.f;
		}

		return true;
	}

    bool move(projent &proj)
    {
        if(physics::physsteps <= 0)
        {
            physics::interppos(&proj);
            return true;
        }

        bool alive = true;
        proj.o = proj.newpos;
        proj.o.z += proj.height;
        loopi(physics::physsteps-1)
        {
            if(((proj.lifetime -= physics::physframetime) <= 0 && proj.lifemillis) || !move(proj, physics::physframetime))
            {
            	if(proj.lifetime < 0) proj.lifetime = 0;
                alive = false;
                break;
            }
        }
        proj.deltapos = proj.o;
        if(alive && (((proj.lifetime -= physics::physframetime) <= 0 && proj.lifemillis) || !move(proj, physics::physframetime)))
        {
           	if(proj.lifetime < 0) proj.lifetime = 0;
			alive = false;
        }
        proj.newpos = proj.o;
        proj.deltapos.sub(proj.newpos);
        proj.newpos.z -= proj.height;
        if(alive) physics::interppos(&proj);
        return alive;
    }

	void update()
	{
		vector<projent *> canremove;
		loopvrev(projs) if(projs[i]->ready())
		{
			if(projs[i]->projtype == PRJ_DEBRIS || projs[i]->projtype == PRJ_GIBS) canremove.add(projs[i]);
			else if(projs[i]->projtype == PRJ_SHOT)
			{
				if(projs[i]->weap == WEAPON_FLAMER)
				{
					if(projs[i]->canrender)
					{
						float asize = weaptype[WEAPON_FLAMER].partsize*projs[i]->lifesize;
						loopj(i) if(projs[j]->projtype == PRJ_SHOT && projs[j]->weap == WEAPON_FLAMER && projs[j]->canrender)
						{
							float bsize = weaptype[WEAPON_FLAMER].partsize*projs[j]->lifesize;
							if(projs[i]->o.squaredist(projs[j]->o) <= asize*bsize) // intentional cross multiply
								projs[j]->canrender = false;
						}
					}
				}
				else projs[i]->canrender = true;
			}
		}
		while(!canremove.empty() && canremove.length() > maxprojectiles)
		{
			int oldest = 0;
			loopv(canremove)
				if(lastmillis-canremove[i]->addtime >= lastmillis-canremove[oldest]->addtime)
					oldest = i;
			if(canremove.inrange(oldest))
			{
				canremove[oldest]->state = CS_DEAD;
				canremove.removeunordered(oldest);
			}
			else break;
		}

		loopv(projs)
		{
			projent &proj = *projs[i];
			if(proj.owner && proj.state != CS_DEAD)
			{
				if(proj.projtype == PRJ_ENT && entities::ents.inrange(proj.id) && entities::ents[proj.id]->type == WEAPON) // in case spawnweapon changes
					proj.mdl = entities::entmdlname(entities::ents[proj.id]->type, entities::ents[proj.id]->attr[0], entities::ents[proj.id]->attr[1], entities::ents[proj.id]->attr[2], entities::ents[proj.id]->attr[3], entities::ents[proj.id]->attr[4]);

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
                    if(!move(proj)) proj.state = CS_DEAD;
				}
				else for(int rtime = curtime; proj.state != CS_DEAD && rtime > 0;)
				{
					int qtime = min(rtime, 30);
					rtime -= qtime;

					if(((proj.lifetime -= qtime) <= 0 && proj.lifemillis) || !move(proj, qtime))
					{
						if(proj.lifetime < 0) proj.lifetime = 0;
						proj.state = CS_DEAD;
 						break;
					}
				}
				if(proj.state != CS_DEAD && proj.radial && proj.local)
					radiate(proj, proj.hit && proj.hit->type == ENT_PLAYER ? (gameent *)proj.hit : NULL);
			}
			else proj.state = CS_DEAD;

			if(proj.state == CS_DEAD)
			{
				destroy(proj);
				delete &proj;
				projs.removeunordered(i--);
			}
		}
	}

	void render()
	{
		loopv(projs) if(projs[i]->ready() && projs[i]->mdl && *projs[i]->mdl)
		{
			projent &proj = *projs[i];
            if(proj.projtype == PRJ_ENT && !entities::ents.inrange(proj.id)) continue;
			float trans = 1.f;
			switch(proj.projtype)
			{
				case PRJ_GIBS: case PRJ_DEBRIS:
					if(proj.lifemillis)
					{
						int interval = min(proj.lifemillis, 1000);
						if(proj.lifetime < interval) trans = float(proj.lifetime)/float(interval);
					}
					break;
				default: break;
			}
			rendermodel(&proj.light, proj.mdl, ANIM_MAPMODEL|ANIM_LOOP, proj.o, proj.yaw+90, proj.pitch, proj.roll, MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_DYNSHADOW|MDL_LIGHT|MDL_CULL_DIST, NULL, NULL, 0, 0, trans);
		}
	}

	void adddynlights()
	{
		loopv(projs) if(projs[i]->ready() && projs[i]->projtype == PRJ_SHOT)
		{
			projent &proj = *projs[i];
			switch(proj.weap)
			{
				case WEAPON_PISTOL: adddynlight(proj.o, 4, vec(0.075f, 0.075f, 0.075f)); break;
				case WEAPON_SG: adddynlight(proj.o, 16, vec(0.5f, 0.35f, 0.1f)); break;
				case WEAPON_SMG: adddynlight(proj.o, 8, vec(0.5f, 0.25f, 0.05f)); break;
				case WEAPON_FLAMER:
				{
					vec col(1.1f*max(1.f-proj.lifespan,0.1f), 0.25f*max(1.f-proj.lifespan,0.05f), 0.00f);
					adddynlight(proj.o, weaptype[proj.weap].explode*1.1f*proj.lifespan, col);
					break;
				}
				case WEAPON_PLASMA:
				{
					vec col(0.1f*max(1.f-proj.lifespan,0.1f), 0.4f*max(1.f-proj.lifespan,0.1f), 0.6f*max(1.f-proj.lifespan,0.1f));
					adddynlight(proj.o, weaptype[proj.weap].explode*1.1f*proj.lifesize, col);
					break;
				}
				case WEAPON_RIFLE:
				{
					vec pos = vec(proj.o).sub(vec(proj.vel).normalize().mul(3.f));
					adddynlight(proj.o, weaptype[proj.weap].partsize*1.5f, vec(0.4f, 0.05f, 1.f));
					break;
				}
				default: break;
			}
		}
	}
}
