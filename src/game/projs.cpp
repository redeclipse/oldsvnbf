#include "pch.h"
#include "engine.h"
#include "game.h"
namespace projs
{
	struct hitmsg
	{
		int flags, target, lifesequence, info;
		ivec dir;
	};
	vector<hitmsg> hits;
	vector<projent *> projs;

	VARA(maxprojectiles, 0, 200, INT_MAX-1);

	int hitzones(vec &o, vec &pos, float height, float above, float radius = 0.f)
	{
		int hits = 0;
		if(o.z <= (pos.z-height*0.75f)+radius) hits |= HIT_LEGS;
		if(o.z < (pos.z-above)+radius && o.z > (pos.z-height*0.75f)-radius) hits |= HIT_TORSO;
		if(o.z >= (pos.z-above)-radius) hits |= HIT_HEAD;
		return hits;
	}

	void hitpush(gameent *d, projent &proj, int flags = 0, int dist = 0, float radius = 0.f)
	{
		vec pos = world::headpos(d);
		int z = hitzones(proj.o, pos, d->height, d->aboveeye, radius), f = flags|z;
		hitmsg &h = hits.add();
		h.flags = f;
		h.target = d->clientnum;
		h.lifesequence = d->lifesequence;
		h.info = dist;
		vec dir = vec(vec(vec(pos).sub(vec(0, 0, d->height*0.5f))).sub(proj.o)).normalize();
		pos.add(vec(proj.vel).normalize()).normalize();
		h.dir = ivec(int(dir.x*DNF), int(dir.y*DNF), int(dir.z*DNF));
	}

	void radialeffect(gameent *d, projent &proj, int radius)
	{
		vec dir, middle = d->o;
		middle.z += (d->aboveeye-d->height)/2;
		float dist = middle.dist(proj.o, dir);
		dir.div(dist);
		if(dist < 0) dist = 0;
		if(dist < radius)
		{
			int flags = proj.attr1 == GUN_GL ? HIT_EXPLODE : HIT_BURN;
			hitpush(d, proj, flags, int(dist*DMF));
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

	void init(projent &proj, bool waited)
	{
		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				proj.aboveeye = proj.height = proj.radius = guntype[proj.attr1].offset;
				proj.elasticity = guntype[proj.attr1].elasticity;
				proj.reflectivity = guntype[proj.attr1].reflectivity;
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
				proj.reflectivity = 0.f;
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
				proj.elasticity = 0.9f;
				proj.reflectivity = 0.f;
				proj.relativity = 0.0f;
				proj.waterfric = 1.7f;
				proj.weight = 100.f;
				proj.vel.add(vec(rnd(80)-41, rnd(80)-41, rnd(160)-81));
				proj.geomcollide = proj.playercollide = 1; // bounce
				break;
			}
			case PRJ_ENT:
			{
				proj.mdl = entities::entmdlname(proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4, proj.attr5);
				proj.aboveeye = 1.f;
				proj.elasticity = 0.35f;
				proj.reflectivity = 0.f;
				proj.relativity = 0.95f;
				proj.waterfric = 1.75f;
				proj.weight = 100.f;
				proj.geomcollide = 1; // bounce
				proj.playercollide = 0; // don't
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

		vec dir(vec(vec(proj.to).sub(proj.o)).normalize()), orig = proj.o;
		vectoyawpitch(dir, proj.yaw, proj.pitch);
		vec rel = vec(proj.vel).add(dir);
		if(proj.relativity) rel.add(vec(proj.owner->vel).mul(proj.relativity));
		proj.vel = vec(rel).add(vec(dir).mul(proj.maxspeed));
		proj.spawntime = lastmillis;
		proj.movement = 1;

		if(proj.projtype == PRJ_SHOT)
		{
			if(proj.radial) proj.height = proj.radius = guntype[proj.attr1].explode*0.1f;
			if(proj.playercollide || proj.geomcollide)
			{
				vec ray(proj.vel);
				ray.normalize();
				int maxsteps = 25;
				float step = 4,
					  barrier = raycube(proj.o, ray, step*maxsteps, RAY_CLIPMAT|RAY_POLY)-0.1f,
					  dist = 0;
				loopi(maxsteps)
				{
					proj.hit = NULL;
					if(dist < barrier && dist + step > barrier) dist = barrier;
					else dist += step;
					proj.o = vec(ray).mul(dist).add(orig);
					if(!collide(&proj) || inside || (proj.playercollide && hitplayer))
					{
						if((!hitplayer && proj.geomcollide) || (hitplayer && proj.playercollide && hitplayer != proj.owner))
						{
            				if(hitplayer)
            				{
                				proj.hit = hitplayer;
                				proj.norm = vec(hitplayer->o).sub(proj.o).normalize();
            				}
            				else proj.norm = wall;
							proj.state = CS_DEAD;
							break;
						}
					}
					else break;
				}
			}
			if(proj.radial) proj.height = proj.radius = guntype[proj.attr1].offset;
		}
        proj.resetinterp();
	}

	void create(vec &from, vec &to, bool local, gameent *d, int type, int lifetime, int waittime, int speed, int id, int ent, int attr1, int attr2, int attr3, int attr4, int attr5)
	{
		if(!d || !speed) return;

		projent &proj = *new projent;
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

	void drop(gameent *d, int g, int n, int delay)
	{
		if(isgun(g))
		{
			if(n >= 0)
			{
				if(entities::ents.inrange(n) && !(entities::ents[n]->attr2&GNT_FORCED))
					create(d->o, d->o, d == world::player1 || d->ai, d, PRJ_ENT, 0, delay, 20, n,
						entities::ents[n]->type, entities::ents[n]->attr1, entities::ents[n]->attr2, entities::ents[n]->attr3, entities::ents[n]->attr4, entities::ents[n]->attr5);
			}
			else if(g == GUN_GL)
				create(d->o, d->o, d == world::player1 || d->ai, d, PRJ_SHOT, 500, 50, 5, -1, WEAPON, d->gunselect);
			d->ammo[g] = d->entid[g] = -1;
			d->setgunstate(g, GNS_PICKUP, GUNSWITCHDELAY, lastmillis);
		}
	}

	void shootv(int gun, int power, vec &from, vector<vec> &locs, gameent *d, bool local)
	{
		int delay = guntype[gun].delay, millis = delay,
			life = guntype[gun].time, speed = guntype[gun].speed;

		if(guntype[gun].power)
		{
			speed = int(speed*clamp(float(clamp(power, 0, guntype[gun].power))/float(guntype[gun].power), 0.f, 1.f));
			if(power > guntype[gun].power)
			{
				float skew = 1.f-clamp(float(power-guntype[gun].power)/float(guntype[gun].time), 0.f, 1.f);
				life = int(life*skew);
				if(!life) speed = millis = delay = 1;
			}
		}

		if(gun == GUN_FLAMER)
		{
			int ends = lastmillis+(d->gunwait[gun]*2);
			if(issound(d->wschan)) sounds[d->wschan].ends = ends;
			else playsound(guntype[gun].sound, d->o, d, SND_LOOP, -1, -1, -1, &d->wschan, ends);
		}
		else if(!guntype[gun].time || life) playsound(guntype[gun].sound, d->o, d);

		switch(gun)
		{
			case GUN_PLASMA:
			{
				part_create(PART_SMOKE_RISE_SLOW, 250, from, 0x88AABB, 0.8f); // smoke
				part_create(PART_PLASMA, 75, from, 0x226688, 1.0f, d);
				adddynlight(from, 60, vec(0.1f, 0.4f, 0.6f), 75, 0, DL_FLASH);
				break;
			}
			case GUN_SG:
			{
				part_create(PART_SMOKE_RISE_SLOW, 500, from, 0x666666, 4.f); // smoke
				part_create(PART_MUZZLE_FLASH, 100, from, 0xFFAA00, 4.f, d);
				adddynlight(from, 60, vec(1.1f, 0.77f, 0.22f), 100, 0, DL_FLASH);
				break;
			}

			case GUN_CG:
			{
				part_create(PART_SMOKE_RISE_SLOW, 100, from, 0x999999, 1.5f); // smoke
				part_create(PART_MUZZLE_FLASH, 50, from, 0xFFAA00, 3.f, d);
                adddynlight(from, 40, vec(1.1f, 0.55f, 0.11f), 50, 0, DL_FLASH);
				break;
			}
			case GUN_FLAMER:
			{
				part_create(PART_SMOKE_RISE_SLOW, 200, from, 0x444444, 2.f); // smoke
				part_create(PART_FIREBALL, 100, from, 0xFF2200, 2.f, d);
				adddynlight(from, 60, vec(1.1f, 0.33f, 0.01f), 100, 0, DL_FLASH);
				break;
			}
			case GUN_CARBINE:
			case GUN_RIFLE:
			{
				part_create(PART_SMOKE_RISE_SLOW, 500, from, 0xCCCCCC, gun == GUN_RIFLE ? 3.f : 2.f); // smoke
				part_create(PART_SMOKE, 50, from, 0xCCCCCC, gun == GUN_RIFLE ? 2.f : 1.2f, d);
                adddynlight(from, 50, vec(0.15f, 0.15f, 0.15f), 50, 0, DL_FLASH);
				break;
			}
		}
		loopv(locs)
		{
			create(from, locs[i], local, d, PRJ_SHOT, life ? life : 1, millis, speed, 0, WEAPON, gun);
			millis += delay;
		}
	}

	void radiate(projent &proj)
	{
		if(lastmillis-proj.lastradial > 250) // for the flamer this results in at most 20 damage per second
		{
			int radius = int(guntype[proj.attr1].explode*max(proj.lifesize, 0.1f));
			if(radius > 0)
			{
				hits.setsizenodelete(0);
				loopi(world::numdynents())
				{
					gameent *f = (gameent *)world::iterdynents(i);
					if(!f || f->state != CS_ALIVE || lastmillis-f->lastspawn <= REGENWAIT) continue;
					radialeffect(f, proj, radius);
				}
				if(!hits.empty())
				{
					client::addmsg(SV_DESTROY, "ri5iv", proj.owner->clientnum, lastmillis-world::maptime, proj.attr1, proj.id >= 0 ? proj.id-world::maptime : proj.id,
							radius, hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
					proj.lastradial = lastmillis;
				}
			}
		}
	}

	void effect(projent &proj)
	{
		proj.lifespan = clamp((proj.lifemillis-proj.lifetime)/float(proj.lifemillis), 0.f, 1.f);
		if(proj.projtype == PRJ_SHOT)
		{
			if(proj.owner) proj.from = proj.owner->muzzle;
			if(guntype[proj.attr1].fsound >= 0 && !issound(proj.schan))
				playsound(guntype[proj.attr1].fsound, proj.o, &proj, 0, proj.attr1 == GUN_FLAMER ? int(255*proj.lifespan) : -1, -1, -1, &proj.schan);
			switch(proj.attr1)
			{
				case GUN_PLASMA:
				{
					int part = PART_PLASMA;
					if(proj.lifemillis-proj.lifetime < 200) proj.lifesize = clamp((proj.lifemillis-proj.lifetime)/200.f, 0.01f, 1.f);
					else
					{
						part = PART_PLASMA_SOFT;
						proj.lifesize = 1.f;
					}
					part_create(part, 1, proj.o, 0x226688, 5.f*proj.lifesize);
					part_create(part, 1, proj.o, 0x44AADD, 2.5f*proj.lifesize); // brighter center part
					break;
				}
				case GUN_FLAMER:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					proj.lifesize *= proj.lifesize; // increase the size exponentially over time
					int steps = clamp(int(proj.vel.magnitude()*(1.f-proj.lifespan)), 0, 5);
					vec dir = vec(proj.vel).normalize().neg().mul(clamp(32.f*proj.lifesize, 1.f, 16.f)),
						pos = proj.o;
					if(!steps || (proj.lifemillis-proj.lifetime > 200 && proj.movement <= 2.f))
					{
						dir = vec(0, 0, clamp(12.f*proj.lifesize, 0.5f, 8.f));
						steps += 2;
					}
					loopi(steps) // pull some trickery to simulate a stream
					{
						float res = float(steps-i)/float(steps), size = clamp(64.f*proj.lifesize*res, 1.5f, 32.f);
						int col = ((int(200*max((1.f-proj.lifespan)*res,0.3f))<<16)+1)|((int(80*max((1.f-proj.lifespan)*res,0.2f))+1)<<8);
						part_create(PART_FIREBALL_SOFT, 1, pos, col, size);
						if(pos.dist(proj.from) <= size) break;
						pos.add(dir);
						if((proj.movement > 2.f) && proj.o.dist(pos) > proj.movement) break;
					}
					break;
				}
				case GUN_GL:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					int col = ((int(196*max(1.f-proj.lifespan,0.3f))<<16)+1)|((int(96*max(1.f-proj.lifespan,0.2f))+1)<<8);
					part_create(PART_PLASMA_SOFT, 1, proj.o, col, proj.radius*2.5f*proj.lifesize);
					bool moving = proj.movement > 0.f;
					if(lastmillis-proj.lasteffect > (moving ? 250 : 500))
					{
						part_create(PART_SMOKE_RISE_SLOW, moving ? 250 : 750, proj.o, 0x222222, proj.radius*(moving ? 1.f : 2.f));
						proj.lasteffect = lastmillis;
					}
					break;
				}
				case GUN_SG:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.movement > 0.f)
					{
						float size = clamp(48.f*(1.f-proj.lifesize), 1.f, proj.lifemillis-proj.lifetime > 200 ? min(48.f, proj.movement) : proj.o.dist(proj.from));
						vec dir = vec(proj.vel).normalize(), from = vec(proj.o).add(vec(dir).mul(proj.radius));
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(200*max(1.f-proj.lifesize,0.3f))<<16)+1)|((int(120*max(1.f-proj.lifesize,0.1f))+1)<<8);
						part_flare(proj.to, from, 1, PART_STREAK, col, proj.radius*0.3f);
						part_flare(proj.to, from, 1, PART_STREAK_LERP, col, proj.radius*0.1f);
					}
					break;
				}
				case GUN_CG:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.movement > 0.f)
					{
						float size = clamp(24.f*(1.f-proj.lifesize), 1.f, proj.lifemillis-proj.lifetime > 200 ? min(24.f, proj.movement) : proj.o.dist(proj.from));
						vec dir = vec(proj.vel).normalize(), from = vec(proj.o).add(vec(dir).mul(proj.radius));
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(200*max(1.f-proj.lifesize,0.3f))<<16))|((int(100*max(1.f-proj.lifesize,0.1f)))<<8);
						part_flare(proj.to, from, 1, PART_STREAK, col, proj.radius*0.25f);
						part_flare(proj.to, from, 1, PART_STREAK_LERP, col, proj.radius*0.075f);
					}
					break;
				}
				case GUN_CARBINE: case GUN_RIFLE:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.movement > 0.f)
					{
						float adjust = proj.attr1 == GUN_CARBINE ? 256.f : 512.f,
							size = clamp(adjust*(1.f-proj.lifesize), 1.f, proj.lifemillis-proj.lifetime > 200 ? min(adjust, proj.movement) : proj.o.dist(proj.from));
						vec dir = vec(proj.vel).normalize(), from = vec(proj.o).add(vec(dir).mul(proj.radius));
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int c = int(196*max(1.f-proj.lifesize,0.3f)), col = ((c<<16))|(c<<8)|c;
						part_trail(PART_SMOKE_SINK, 1, proj.to, from, col, proj.radius*(proj.attr1 == GUN_CARBINE ? 0.25f : 0.5f));
					}
					break;
				}
				default:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					part_create(PART_PLASMA_SOFT, 1, proj.o, 0xFFFFFF, proj.radius);
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
			proj.lifesize = clamp(1.f-proj.lifespan, 0.1f, 1.f); // gets smaller as it gets older
			int steps = clamp(int(proj.vel.magnitude()*proj.lifesize), 0, 10);
			if(steps && proj.movement > 0.f)
			{
				vec dir = vec(proj.vel).normalize().neg().mul(proj.radius*0.5f), pos = proj.o;
				loopi(steps)
				{
					float res = float(steps-i)/float(steps), size = clamp(proj.radius*proj.lifesize*res, 0.1f, proj.radius);
					int col = ((int(144*max(res,0.3f))<<16)+1)|((int(48*max(res,0.1f))+1)<<8);
					part_create(PART_PLASMA_SOFT, 1, pos, col, size);
					pos.add(dir);
					if(proj.o.dist(pos) > proj.movement) break;
				}
			}
		}
	}

	void destroy(projent &proj)
	{
		proj.state = CS_DEAD;
		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				if(proj.owner) proj.from = proj.owner->muzzle;
				if(guntype[proj.attr1].esound >= 0) playsound(guntype[proj.attr1].esound, proj.o);
				switch(proj.attr1)
				{
					case GUN_PLASMA:
					{
						part_create(PART_PLASMA_SOFT, 1, proj.o, 0x226688, 5.f*proj.lifesize);
						part_create(PART_PLASMA_SOFT, 1, proj.o, 0x44AADD, 2.5f*proj.lifesize); // brighter center part
						part_create(PART_SMOKE_RISE_SLOW, 500, proj.o, 0x88AABB, 2.4f); // smoke
						adddynlight(proj.o, 1.15f*guntype[proj.attr1].explode, vec(0.1f, 0.4f, 0.6f), 200, 10);
						adddecal(DECAL_ENERGY, proj.o, proj.norm, 6.f, bvec(86, 196, 244));
						break;
					}
					case GUN_FLAMER:
					case GUN_GL:
					{ // both basically explosions
						part_create(proj.attr1 == GUN_FLAMER ? PART_FIREBALL_SOFT : PART_PLASMA_SOFT, proj.attr1 == GUN_FLAMER ? 500 : 1000, proj.o, 0x884400, guntype[proj.attr1].explode*0.3f); // corona
						int deviation = int(guntype[proj.attr1].explode*0.5f);
						loopi(rnd(3)+2)
						{
							vec to = vec(proj.o).add(vec(rnd(deviation*2)-deviation, rnd(deviation*2)-deviation, rnd(deviation*2)-deviation));
							part_create(PART_FIREBALL_SOFT, proj.attr1 == GUN_FLAMER ? 500 : 1000, to, 0x882200, guntype[proj.attr1].explode);
						}
						part_create(PART_SMOKE_RISE_SLOW_SOFT, proj.attr1 == GUN_FLAMER ? 500 : 2000, vec(proj.o).sub(vec(0, 0, guntype[proj.attr1].explode*0.25f)), proj.attr1 == GUN_FLAMER ? 0x444444 : 0x222222, guntype[proj.attr1].explode);
						adddynlight(proj.o, 1.15f*guntype[proj.attr1].explode, vec(1.1f, 0.22f, 0.02f), proj.attr1 == GUN_FLAMER ? 250 : 1500, 10);
						if(proj.attr1 == GUN_GL)
						{
							world::quakewobble += max(int(guntype[proj.attr1].damage*(1.f-camera1->o.dist(proj.o)/EXPLOSIONSCALE/guntype[proj.attr1].explode)), 1);
							part_fireball(vec(proj.o).sub(vec(0, 0, 2)), guntype[proj.attr1].explode*0.75f, PART_EXPLOSION, 1000, 0x662200, 4.f); // explosion fireball
							loopi(rnd(20)+10)
								create(proj.o, vec(proj.o).add(proj.vel), true, proj.owner, PRJ_DEBRIS, rnd(1500)+1500, rnd(750), rnd(60)+40);
						}
						adddecal(DECAL_SCORCH, proj.o, proj.norm, guntype[proj.attr1].explode);
						adddecal(DECAL_ENERGY, proj.o, proj.norm, guntype[proj.attr1].explode*0.75f, bvec(196, 24, 0));
						break;
					}
					case GUN_SG: case GUN_CG:
					{
						if(proj.lifetime)
							part_splash(PART_SPARK, proj.attr1 == GUN_SG ? 10 : 20, 500, proj.o, 0xFFAA22, proj.radius*(proj.attr1 == GUN_SG ? 0.25f : 0.1f), 8);
						break;
					}
					case GUN_CARBINE: case GUN_RIFLE:
					{
						proj.from = vec(proj.o).sub(proj.vel);
						part_create(PART_SMOKE_RISE_SLOW, 500, proj.o, 0xFFFFFF, proj.radius*(proj.attr1 == GUN_CARBINE ? 2.f : 3.f));
						adddecal(DECAL_BULLET, proj.o, proj.norm, proj.radius*(proj.attr1 == GUN_CARBINE ? 2.f : 3.f));
						break;
					}
				}
				if(proj.local)
				{
					hits.setsizenodelete(0);
					if(guntype[proj.attr1].explode)
					{
						loopi(world::numdynents())
						{
							gameent *f = (gameent *)world::iterdynents(i);
							if(!f || f->state != CS_ALIVE || lastmillis-f->lastspawn <= REGENWAIT) continue;
							radialeffect(f, proj, guntype[proj.attr1].explode);
						}
					}
					else if(proj.hit && proj.hit->type == ENT_PLAYER)
						hitpush((gameent *)proj.hit, proj);

					client::addmsg(SV_DESTROY, "ri5iv", proj.owner->clientnum, lastmillis-world::maptime, proj.attr1, proj.id >= 0 ? proj.id-world::maptime : proj.id,
							0, hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
				}
				break;
			}
			case PRJ_ENT:
			{
				if(!proj.beenused)
				{
					world::spawneffect(proj.o, 0x221188, int(proj.radius), int(proj.radius));
					if(proj.local)
						client::addmsg(SV_DESTROY, "ri6", proj.owner->clientnum, lastmillis-world::maptime, -1, proj.id, 0, 0);
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

	int bounce(projent &proj, const vec &dir)
	{
		proj.hit = NULL;
		if(!collide(&proj, dir, 0.f, proj.playercollide > 0) || inside || (proj.playercollide && hitplayer))
		{
			if(hitplayer)
			{
				proj.hit = hitplayer;
				proj.norm = vec(hitplayer->o).sub(proj.o).normalize();
			}
			else proj.norm = wall;

			if((!hitplayer && proj.geomcollide == 1) || (hitplayer && proj.playercollide == 1))
			{
				if(proj.movement > 2.f || !proj.lastbounce)
				{
					int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 0, 255);
					switch(proj.projtype)
					{
						case PRJ_SHOT:
						{
							switch(proj.attr1)
							{
								case GUN_SG: case GUN_CG:
								{
									part_splash(PART_SPARK, 5, 250, proj.o, 0xFFAA22, proj.radius*(proj.attr1 == GUN_SG ? 0.25f : 0.1f), 8);
									if(!proj.lastbounce)
										adddecal(DECAL_BULLET, proj.o, proj.norm, proj.radius*(proj.attr1 == GUN_SG ? 3.f : 1.5f));
									break;
								}
								case GUN_FLAMER:
								{
									if(!proj.lastbounce)
									{
										adddecal(DECAL_SCORCH, proj.o, proj.norm, 32.f*proj.lifesize);
										adddecal(DECAL_ENERGY, proj.o, proj.norm, 12.f*proj.lifesize, bvec(92, 12, 0));
									}
									break;
								}
								default: break;
							}
							if(vol && guntype[proj.attr1].rsound >= 0)
								playsound(guntype[proj.attr1].rsound, proj.o, &proj, 0, vol);
							break;
						}
						case PRJ_GIBS:
						{
							if(!proj.lastbounce)
								adddecal(DECAL_BLOOD, proj.o, proj.norm, proj.radius*clamp(proj.vel.magnitude(), 0.5f, 4.f), bvec(100, 255, 255));
							if(vol) playsound(S_SPLAT, proj.o, &proj, 0, vol);
							break;
						}
						case PRJ_DEBRIS:
						{
							if(vol) playsound(S_DEBRIS, proj.o, &proj, 0, vol);
							break;
						}
						default: break;
					}
				}
				reflect(proj, proj.norm);
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
		if(isdeadly(mat&MATF_VOLUME) || proj.o.z < 0) return false; // gets destroyed
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
        if(proj.projtype == PRJ_SHOT)
        {
			float stepdist = dir.magnitude();
            vec ray(dir);
            ray.mul(1/stepdist);
            float barrier = raycube(proj.o, ray, stepdist, RAY_CLIPMAT|RAY_POLY);
            if(barrier < stepdist)
            {
                proj.o.add(ray.mul(barrier-0.1f));
                switch(bounce(proj, ray))
                {
                    case 2: blocked = true; break;
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
			    case 2: proj.o = pos; break;
			    case 1: default: break;
			    case 0: if(proj.projtype != PRJ_SHOT) proj.o = pos; return false;
		    }
        }

		float dist = proj.o.dist(pos), diff = dist/(4*RAD);
		#define adddiff(q) { q += diff; if(q >= 360) q = fmod(q, 360.0f); }
		proj.movement += dist;
		if(proj.projtype == PRJ_SHOT)
		{
			switch(proj.attr1)
			{
				case GUN_GL:
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
			if(proj.pitch < 0.f) { proj.pitch += diff; if(proj.pitch > 0.f) proj.pitch = 0.f; }
			if(proj.pitch > 0.f) { proj.pitch -= diff; if(proj.pitch < 0.f) proj.pitch = 0.f; }
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
                alive = false;
                break;
            }
        }
        proj.deltapos = proj.o;
        if(alive && (((proj.lifetime -= physics::physframetime) <= 0 && proj.lifemillis) || !move(proj, physics::physframetime)))
			alive = false;
        proj.newpos = proj.o;
        proj.deltapos.sub(proj.newpos);
        proj.newpos.z -= proj.height;
        physics::interppos(&proj);
        return alive;
    }

	void update()
	{
		int numprojs = projs.length();
		if(numprojs > maxprojectiles)
		{
			vector<projent *> canremove;
			loopvrev(projs)
				if(projs[i]->ready() && (projs[i]->projtype == PRJ_DEBRIS || projs[i]->projtype == PRJ_GIBS))
					canremove.add(projs[i]);
			while(!canremove.empty() && numprojs > maxprojectiles)
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
			if(!proj.owner || proj.state == CS_DEAD) destroy(proj);
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
                    if(!move(proj)) destroy(proj);
				}
				else for(int rtime = curtime; proj.state != CS_DEAD && rtime > 0;)
				{
					int qtime = min(rtime, 30);
					rtime -= qtime;

					if(((proj.lifetime -= qtime) <= 0 && proj.lifemillis) || !move(proj, qtime))
					{
						destroy(proj);
 						break;
					}
				}
				if(proj.state != CS_DEAD && proj.radial && proj.local) radiate(proj);
			}
			if(proj.state == CS_DEAD)
			{
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
					vec col(0.1f*max(1.f-proj.lifespan,0.1f), 0.4f*max(1.f-proj.lifespan,0.1f), 0.6f*max(1.f-proj.lifespan,0.1f));
					adddynlight(proj.o, guntype[proj.attr1].explode*proj.lifesize, col);
					break;
				}
				case GUN_FLAMER:
				{
					vec col(1.1f*max(1.f-proj.lifespan,0.1f), 0.25f*max(1.f-proj.lifespan,0.05f), 0.00f);
					adddynlight(proj.o, guntype[proj.attr1].explode*proj.lifesize, col);
					break;
				}
				default:
					break;
			}
		}
	}
}
