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

	VARA(maxprojectiles, 32, 256, INT_MAX-1);
	VARP(flamertrail, 0, 1, 1);
	VARP(flamerdelay, 1, 100, INT_MAX-1);
	VARA(flamerlength, 50, 500, INT_MAX-1);
	VARP(flamerhint, 0, 1, 1);

	VARP(muzzleflash, 0, 3, 3); // 0 = off, 1 = only other players, 2 = only thirdperson, 3 = all
	VARP(muzzleflare, 0, 3, 3); // 0 = off, 1 = only other players, 2 = only thirdperson, 3 = all
	#define muzzlechk(a,b) (a > 0 && (a == 3 || (a == 2 && game::thirdpersonview(true)) || (a == 1 && b != game::player1)))

	int calcdamage(int weap, int &flags, int radial, float size, float dist)
	{
		int damage = weaptype[weap].damage[flags&HIT_ALT ? 1 : 0];
		if(radial) damage = int(damage*(1.f-dist/EXPLOSIONSCALE/max(size, 1e-3f)));
		if(!hithurts(flags)) flags = HIT_WAVE|(flags&HIT_ALT ? HIT_ALT : 0); // so it impacts, but not hurts
		else if((flags&HIT_FULL) && !weaptype[weap].explode[flags&HIT_ALT ? 1 : 0]) flags &= ~HIT_FULL;
		if(hithurts(flags))
		{
			if(flags&HIT_FULL || flags&HIT_HEAD) damage = int(damage*GVAR(damagescale));
			else if(flags&HIT_TORSO) damage = int(damage*0.5f*GVAR(damagescale));
			else if(flags&HIT_LEGS) damage = int(damage*0.25f*GVAR(damagescale));
			else damage = 0;
		}
		else damage = int(damage*GVAR(damagescale));
		return damage;
	}

	void hitpush(gameent *d, projent &proj, int flags = 0, int radial = 0, float dist = 0)
	{
		if(m_play(game::gamemode) && (!m_insta(game::gamemode, game::mutators) || (!(flags&HIT_EXPLODE) && !(flags&HIT_BURN))))
		{
			if(hithurts(flags) && proj.owner && (proj.owner == game::player1 || proj.owner->ai))
			{
				int hflags = proj.flags|flags, damage = calcdamage(proj.weap, hflags, radial, float(radial), dist);
				if(damage > 0 && hithurts(hflags)) game::hiteffect(proj.weap, hflags, damage, d, proj.owner);
			}
			hitmsg &h = hits.add();
			h.flags = flags;
			h.target = d->clientnum;
			h.id = lastmillis-game::maptime;
			h.dist = int(dist*DMF);
			vec dir, middle = d->o;
			middle.z += (d->aboveeye-d->height)/2;
			dir = vec(middle).sub(proj.o).normalize();
			dir.add(vec(proj.vel).normalize()).normalize();
			h.dir = ivec(int(dir.x*DNF), int(dir.y*DNF), int(dir.z*DNF));
		}
	}

	void hitproj(gameent *d, projent &proj)
	{
		int flags = 0;
		if(proj.hitflags&HITFLAG_LEGS) flags |= HIT_LEGS;
		if(proj.hitflags&HITFLAG_TORSO) flags |= HIT_TORSO;
		if(proj.hitflags&HITFLAG_HEAD) flags |= HIT_HEAD;
		if(flags) hitpush(d, proj, flags|HIT_PROJ);
	}

	bool hiteffect(projent &proj, physent *d, int flags, const vec &norm)
	{
		if(d != proj.hit && (proj.projcollide&COLLIDE_OWNER || d != proj.owner))
		{
			proj.hit = d;
			proj.hitflags = flags;
			proj.norm = norm;
			if(!weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0] && (d->type == ENT_PLAYER || d->type == ENT_AI)) hitproj((gameent *)d, proj);
			if(proj.projcollide&COLLIDE_CONT)
			{
				switch(proj.weap)
				{
					case WEAP_RIFLE:
                        part_splash(PART_SPARK, 10, m_speedtime(250), proj.o, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.5f, 10, 0, 24);
						part_create(PART_PLASMA, m_speedtime(250), proj.o, 0x6611FF, 2.f, 0, 0);
						adddynlight(proj.o, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*1.5f, vec(0.4f, 0.05f, 1.f), m_speedtime(250), 10);
						break;
					default: break;
				}
				return false;
			}
			return true;
		}
		return false;
	}

	void radialeffect(gameent *d, projent &proj, bool explode, int radius)
	{
		vec dir, middle = d->o;
		middle.z += (d->aboveeye-d->height)/2;
		float dist = middle.dist(proj.o, dir);
		dir.div(dist); if(dist < 0) dist = 0;
		if(dist <= radius) hitpush(d, proj, HIT_FULL|(explode ? HIT_EXPLODE : HIT_BURN), radius, dist);
		else if(explode && dist <= radius*wavepusharea) hitpush(d, proj, HIT_WAVE, radius, dist);
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
		loopi(WEAP_MAX) if(*weaptype[i].proj) loadmodel(weaptype[i].proj, -1, true);
		const char *mdls[] = { "gibc", "gibh", "debris/debris01", "debris/debris02", "debris/debris03", "debris/debris04", "" };
		for(int i = 0; *mdls[i]; i++) loadmodel(mdls[i], -1, true);
	}

	void reflect(projent &proj, vec &pos)
    {
    	bool speed = proj.vel.magnitude() > 0.01f;
    	float elasticity = speed ? proj.elasticity : 1.f, reflectivity = speed ? proj.reflectivity : 0.f;
    	if(elasticity > 0.f)
    	{
			vec dir[2]; dir[0] = dir[1] = vec(proj.vel).normalize();
    		float mag = proj.vel.magnitude()*elasticity; // conservation of energy
			loopi(3) if((pos[i] > 0.f && dir[1][i] < 0.f) || (pos[i] < 0.f && dir[1][i] > 0.f))
				dir[1][i] = fabs(dir[1][i])*pos[i];
			if(reflectivity > 0.f)
			{ // if projectile returns at 180 degrees [+/-]reflectivity, skew the reflection
				float aim[2][2] = { { 0.f, 0.f }, { 0.f, 0.f } };
				loopi(2) vectoyawpitch(dir[i], aim[0][i], aim[1][i]);
				loopi(2)
				{
					float rmax = 180.f+reflectivity, rmin = 180.f-reflectivity,
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
		if(proj.movement >= 2.f && !proj.lastbounce) switch(proj.projtype)
        {
            case PRJ_SHOT:
            {
                switch(proj.weap)
                {
                    case WEAP_SHOTGUN: case WEAP_SMG:
                    {
                        part_splash(PART_SPARK, 5, m_speedtime(250), proj.o, 0xFFAA22, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.75f, 10, 0, 16);
                        if(!proj.lastbounce)
                            adddecal(DECAL_BULLET, proj.o, proj.norm, proj.weap == WEAP_SHOTGUN ? 3.f : 1.5f);
                        break;
                    }
                    case WEAP_FLAMER:
                    {
                        if(!proj.lastbounce && !rnd(3))
                            adddecal(DECAL_SCORCH_SHORT, proj.o, proj.norm, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*3.f*proj.lifesize);
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
            	if(!kidmode && game::bloodscale > 0 && game::gibscale > 0)
            	{
					if(!proj.lastbounce)
						adddecal(DECAL_BLOOD, proj.o, proj.norm, proj.radius*clamp(proj.vel.magnitude(), 0.25f, 2.f), bvec(125, 255, 255));
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

	void init(projent &proj, bool waited)
	{
		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				if(proj.owner && (proj.owner != game::player1 || waited) && proj.owner->muzzle != vec(-1, -1, -1))
					proj.o = proj.from = proj.owner->muzzle;
				proj.aboveeye = proj.height = proj.radius = 1.f;
				proj.elasticity = weaptype[proj.weap].elasticity[proj.flags&HIT_ALT ? 1 : 0];
				proj.reflectivity = weaptype[proj.weap].reflectivity[proj.flags&HIT_ALT ? 1 : 0];
				proj.relativity = weaptype[proj.weap].relativity[proj.flags&HIT_ALT ? 1 : 0];
				proj.waterfric = weaptype[proj.weap].waterfric[proj.flags&HIT_ALT ? 1 : 0];
				proj.weight = weaptype[proj.weap].weight[proj.flags&HIT_ALT ? 1 : 0];
				proj.projcollide = weaptype[proj.weap].collide[proj.flags&HIT_ALT ? 1 : 0];
				proj.radial = weaptype[proj.weap].radial[proj.flags&HIT_ALT ? 1 : 0];
				proj.extinguish = weaptype[proj.weap].extinguish[proj.flags&HIT_ALT ? 1 : 0];
				proj.mdl = weaptype[proj.weap].proj;
				break;
			}
			case PRJ_GIBS:
			{
				if(!kidmode && game::bloodscale > 0 && game::gibscale > 0)
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
					proj.elasticity = 0.35f;
					proj.reflectivity = 0.f;
					proj.relativity = 1.0f;
					proj.waterfric = 2.0f;
					proj.weight = 75.f;
					proj.vel.add(vec(rnd(40)-21, rnd(40)-21, rnd(40)-21));
					proj.projcollide = BOUNCE_GEOM|BOUNCE_PLAYER|COLLIDE_OWNER;
					break;
				} // otherwise fall through
			}
			case PRJ_DEBRIS:
			{
				switch(rnd(4))
				{
					case 3: proj.mdl = "debris/debris04"; break;
					case 2: proj.mdl = "debris/debris03"; break;
					case 1: proj.mdl = "debris/debris02"; break;
					case 0: default: proj.mdl = "debris/debris01"; break;
				}
				proj.aboveeye = 1.0f;
				proj.elasticity = 0.65f;
				proj.reflectivity = 0.f;
				proj.relativity = 0.0f;
				proj.waterfric = 1.7f;
				proj.weight = 100.f;
				proj.vel.add(vec(rnd(101)-50, rnd(101)-50, rnd(151)-50)).mul(2);
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
				proj.weight = 125.f;
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
			if(proj.projtype == PRJ_ENT)
			{
				proj.radius += 1.f;
				proj.xradius += 1.f;
				proj.yradius += 1.f;
				proj.height += 4.f;
			}
			else
			{
				proj.radius += 0.5f;
				proj.xradius += 0.5f;
				proj.yradius += 0.5f;
				proj.height += 0.5f;
			}
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
		proj.hit = NULL;
		proj.hitflags = HITFLAG_NONE;
		proj.movement = 1;

		if(proj.radial && proj.projtype == PRJ_SHOT) proj.height = proj.radius = weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.25f;
		if(proj.projcollide)
		{
			vec ray = vec(proj.vel).normalize();
			int maxsteps = 25;
			float step = 4,
				  barrier = max(raycube(proj.o, ray, step*maxsteps, RAY_CLIPMAT|(proj.projcollide&COLLIDE_TRACE ? RAY_ALPHAPOLY : RAY_POLY))-0.1f, 1e-3f),
				  dist = 0;
			loopi(maxsteps)
			{
				float olddist = dist;
				if(dist < barrier && dist + step > barrier) dist = barrier;
				else dist += step;
				if(proj.projcollide&COLLIDE_TRACE)
				{
					proj.o = vec(ray).mul(olddist).add(orig);
					float cdist = tracecollide(&proj, proj.o, ray, dist - olddist, RAY_CLIPMAT | RAY_ALPHAPOLY);
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
					if(hitplayer) { if(!hiteffect(proj, hitplayer, hitflags, vec(hitplayer->o).sub(proj.o).normalize())) continue; }
					else proj.norm = proj.projcollide&COLLIDE_TRACE ? hitsurface : wall;

					if(proj.projcollide&(hitplayer ? BOUNCE_PLAYER : BOUNCE_GEOM) && i < maxsteps-1)
					{
						if(proj.movement)
						{
							bounceeffect(proj);
							reflect(proj, proj.norm);
							proj.movement = 0;
							proj.lastbounce = lastmillis;
							ray = vec(proj.vel).normalize();
						}
					}
					else if(proj.lifemillis)
					{ // fastfwd to end
						proj.lifemillis = proj.lifetime = 1;
						proj.lifespan = proj.lifesize = 1.f;
						proj.state = CS_DEAD;
					}
				}
			}
		}
		if(proj.radial && proj.projtype == PRJ_SHOT) proj.height = proj.radius = 1.f;
        proj.resetinterp();
	}

	void create(vec &from, vec &to, bool local, gameent *d, int type, int lifetime, int lifemillis, int waittime, int speed, int id, int weap, int flags)
	{
		if(!d || !speed) return;

		projent &proj = *new projent;
		proj.o = proj.from = from;
		proj.to = to;
		proj.local = local;
		proj.projtype = type;
		proj.addtime = lastmillis;
		proj.lifetime = lifetime ? m_speedtime(lifetime) : lifetime;
		proj.lifemillis = lifemillis ? m_speedtime(lifemillis) : proj.lifetime;
		proj.waittime = waittime ? m_speedtime(waittime) : waittime;
		proj.maxspeed = speed;
		proj.id = id ? id : lastmillis;
        proj.weap = weap;
        proj.flags = flags;
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
				if(!m_noitems(game::gamemode, game::mutators) && itemdropping && !(entities::ents[n]->attrs[1]&WEAP_F_FORCED))
					create(from, to, local, d, PRJ_ENT, 0, 0, 1, 1, n);
				d->ammo[g] = -1;
				d->setweapstate(g, WEAP_S_SWITCH, WEAPSWITCHDELAY, lastmillis);
			}
			else if(g == WEAP_GRENADE)
			{
				create(from, to, local, d, PRJ_SHOT, 1, weaptype[g].time[0], 1, 1, -1, g);
				d->ammo[g] = max(d->ammo[g]-weaptype[g].sub[0], 0);
				d->setweapstate(g, WEAP_S_SHOOT, weaptype[g].adelay[0], lastmillis);
			}
			else
			{
				d->ammo[g] = -1;
				d->setweapstate(g, WEAP_S_SWITCH, WEAPSWITCHDELAY, lastmillis);
			}
			d->entid[g] = -1;
		}
	}

	void shootv(int weap, int flags, int power, vec &from, vector<vec> &locs, gameent *d, bool local)
	{
		int delay = weaptype[weap].delay, millis = delay,
			life = weaptype[weap].time[flags&HIT_ALT ? 1 : 0], speed = weaptype[weap].speed[flags&HIT_ALT ? 1 : 0];

		if(weaptype[weap].power)
		{
			float amt = clamp(float(clamp(power, 0, weaptype[weap].power))/float(weaptype[weap].power), 0.f, 1.f);
			life = int(float(life)*(1.f-amt));
			if(!life) millis = delay = 1;
		}

		if(weap == WEAP_FLAMER)
		{
			int ends = lastmillis+(d->weapwait[weap]*2);
			if(issound(d->wschan)) sounds[d->wschan].ends = ends;
			else playsound(weaptype[weap].sound, d->o, d, SND_LOOP, -1, -1, -1, &d->wschan, ends);
		}
		else if(!weaptype[weap].time || life) playsound(weaptype[weap].sound, d->o, d);

		switch(weap)
		{
			case WEAP_PISTOL:
			{
				part_create(PART_SMOKE_LERP, 200, from, 0xAAAAAA, 1.f, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_MUZZLE_FLASH, 100, from, 0xFFCC22, 1.5f, 0, 0, d);
				if(muzzlechk(muzzleflare, d))
				{
					vec to; vecfromyawpitch(d->yaw, d->pitch, 1, 0, to);
					part_flare(from, to.mul(3.f).add(from), 100, PART_MUZZLE_FLARE, 0xFFCC22, 1.5f, 0, 0, d);
				}
                adddynlight(from, 32, vec(0.15f, 0.15f, 0.15f), 50, 0, DL_FLASH);
				break;
			}
			case WEAP_SHOTGUN:
			{
				part_create(PART_SMOKE_LERP, 500, from, 0x666666, 4.f, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_MUZZLE_FLASH, 100, from, 0xFFAA00, 3.f, 0, 0, d);
				if(muzzlechk(muzzleflare, d))
				{
					vec to; vecfromyawpitch(d->yaw, d->pitch, 1, 0, to);
					part_flare(from, to.mul(16.f).add(from), 100, PART_MUZZLE_FLARE, 0xFFAA00, 4.f, 0, 0, d);
				}
				adddynlight(from, 48, vec(1.1f, 0.77f, 0.22f), 100, 0, DL_FLASH);
				break;
			}
			case WEAP_SMG:
			{
				part_create(PART_SMOKE_LERP, 50, from, 0x999999, 1.5f, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_MUZZLE_FLASH, 25, from, 0xFF8800, 2.25f, 0, 0, d);
				if(muzzlechk(muzzleflare, d))
				{
					vec to; vecfromyawpitch(d->yaw, d->pitch, 1, 0, to);
					part_flare(from, to.mul(12.f).add(from), 25, PART_MUZZLE_FLARE, 0xFF8800, 3.f, 0, 0, d);
				}
                adddynlight(from, 32, vec(1.1f, 0.55f, 0.11f), 50, 0, DL_FLASH);
				break;
			}
			case WEAP_FLAMER:
			{
				part_create(PART_SMOKE_LERP, 100, from, 0x333333, 2.f, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_FIREBALL, 50, from, 0xFF3300, 1.5f, -10, 0, d);
				adddynlight(from, 48, vec(1.1f, 0.3f, 0.01f), 100, 0, DL_FLASH);
				break;
			}
			case WEAP_PLASMA:
			{
				part_create(PART_SMOKE, 150, from, 0x88AABB, 0.6f, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_PLASMA, 75, from, 0x226688, 1.25f, 0, 0, d);
				adddynlight(from, 24, vec(0.1f, 0.4f, 0.6f), 75, 0, DL_FLASH);
				break;
			}
			case WEAP_RIFLE:
			{
				part_create(PART_SMOKE_LERP, 100, from, 0x444444, 0.8f, -40);
				if(muzzlechk(muzzleflash, d)) part_create(PART_PLASMA, 75, from, 0x6611FF, 1.25f, 0, 0, d);
				if(muzzlechk(muzzleflare, d))
				{
					vec to; vecfromyawpitch(d->yaw, d->pitch, 1, 0, to);
					part_flare(from, to.mul(8.f).add(from), 75, PART_MUZZLE_FLARE, 0x6611FF, 2.f, 0, 0, d);
				}
                adddynlight(from, 32, vec(0.4f, 0.05f, 1.f), 75, 0, DL_FLASH);
				break;
			}
		}
		loopv(locs)
		{
			create(from, locs[i], local, d, PRJ_SHOT, life ? life : 1, weaptype[weap].time[flags&HIT_ALT ? 1 : 0], millis, speed, 0, weap, flags);
			millis += delay;
		}
	}

	void radiate(projent &proj, int radius)
	{
		gameent *d = proj.hit && (proj.hit->type == ENT_PLAYER || proj.hit->type == ENT_AI) ? (gameent *)proj.hit : NULL;
		if((!proj.lastradial || d || (lastmillis-proj.lastradial >= m_speedtime(100))) && radius > 0)
		{ // for the flamer this results in at most 40 damage per second
			if(!d)
			{
				loopi(game::numdynents())
				{
					gameent *f = (gameent *)game::iterdynents(i);
					if(!f || f->state != CS_ALIVE || !physics::issolid(f)) continue;
					if(!(proj.projcollide&COLLIDE_OWNER) && f == proj.owner) continue;
					radialeffect(f, proj, false, radius);
				}
				proj.lastradial = lastmillis;
			}
			else if(d->state == CS_ALIVE && physics::issolid(d) && (proj.projcollide&COLLIDE_OWNER || d != proj.owner))
				radialeffect(d, proj, false, radius);
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
				case WEAP_PISTOL:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender && proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= m_speedtime(200);
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0]*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(220*max(1.f-proj.lifesize,0.3f))<<16))|((int(160*max(1.f-proj.lifesize,0.2f)))<<8);
						part_flare(proj.to, proj.o, 1, PART_FLARE, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
						part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f);
						part_create(PART_PLASMA_SOFT, 1, proj.o, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
					}
					break;
				}
				case WEAP_FLAMER:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender)
					{
						bool effect = false;
						float size = weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*proj.lifesize;
						if(flamertrail && lastmillis-proj.lasteffect >= m_speedtime(flamerdelay))
						{
							effect = true;
							proj.lasteffect = lastmillis;
						}
						int col = ((int(254*max((1.f-proj.lifespan),0.3f))<<16)+1)|((int(64*max((1.f-proj.lifespan),0.15f))+1)<<8),
							len = effect ? max(int(m_speedtime(flamerlength)*max(proj.lifespan, 0.1f)), 0) : 0;
						if(!len) { effect = false; len = 1; }
						if(flamerhint) part_create(PART_HINT, max(len/2, 1), proj.o, 0x140434, size*1.25f, -5);
						part_create(PART_FIREBALL_SOFT, len, proj.o, col, size, -5);
					}
					break;
				}
				case WEAP_GRENADE:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender)
					{
						int col = ((int(224*max(1.f-proj.lifespan,0.375f))<<16)+1)|((int(128*max(1.f-proj.lifespan,0.125f))+1)<<8);
						part_create(PART_HINT_SOFT, 1, proj.o, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*proj.lifesize);
						bool moving = proj.movement > 0.f;
						if(lastmillis-proj.lasteffect >= m_speedtime(moving ? 250 : 500))
						{
							part_create(PART_SMOKE_LERP, m_speedtime(moving ? 250 : 750), proj.o, 0x222222, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*(moving ? 0.5f : 1.f), -20);
							proj.lasteffect = lastmillis;
						}
					}
					break;
				}
				case WEAP_SHOTGUN:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender && proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= m_speedtime(200);
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0]*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(224*max(1.f-proj.lifesize,0.3f))<<16)+1)|((int(124*max(1.f-proj.lifesize,0.1f))+1)<<8);
						part_flare(proj.to, proj.o, 1, PART_FLARE, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
						part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f);
					}
					break;
				}
				case WEAP_SMG:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender && proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= m_speedtime(200);
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0]*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(224*max(1.f-proj.lifesize,0.3f))<<16))|((int(100*max(1.f-proj.lifesize,0.1f)))<<8);
						part_flare(proj.to, proj.o, 1, PART_FLARE, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
						part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f);
					}
					break;
				}
				case WEAP_PLASMA:
				{
					proj.lifesize = proj.lifespan > (proj.flags&HIT_ALT ? 0.25f : 0.0625f) ? 1.125f-proj.lifespan*proj.lifespan : proj.lifespan*(proj.flags&HIT_ALT ? 4.f : 16.f);
					if(proj.canrender)
					{
						part_create(PART_PLASMA_SOFT, 1, proj.o, proj.flags&HIT_ALT ? 0x4488EE : 0x55AAEE, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*proj.lifesize);
						part_create(PART_ELECTRIC, 1, proj.o, proj.flags&HIT_ALT ? 0x4488EE : 0x55AAEE, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.7f*proj.lifesize);
					}
					break;
				}
				case WEAP_RIFLE:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.canrender)
					{
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], 1.f, min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement));
						vec dir = dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						part_flare(proj.to, proj.o, 1, PART_FLARE, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
						part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f);
					}
					break;
				}
				case WEAP_GIBS:
				{
					proj.lifesize = 1;
					if(proj.canrender)
					{
						part_create(PART_HINT_SOFT, 1, proj.o, 0x661111, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
						if(lastmillis-proj.lasteffect >= m_speedtime(game::bloodfade/10) && proj.lifetime >= min(proj.lifemillis, 1000))
						{
							part_create(PART_BLOOD, m_speedtime(game::bloodfade), proj.o, 0x88FFFF, 2.f, 50, DECAL_BLOOD);
							proj.lasteffect = lastmillis;
						}
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
			if(weaptype[proj.weap].radial[proj.flags&HIT_ALT ? 1 : 0] || weaptype[proj.weap].taper[proj.flags&HIT_ALT ? 1 : 0]) proj.radius = max(proj.lifesize, 0.1f);
		}
		else if(proj.projtype == PRJ_GIBS && !kidmode && game::bloodscale > 0 && game::gibscale > 0)
		{
			proj.lifesize = 1;
			if(proj.canrender && lastmillis-proj.lasteffect >= m_speedtime(game::bloodfade/10) && proj.lifetime >= min(proj.lifemillis, 1000))
			{
				part_create(PART_BLOOD, m_speedtime(game::bloodfade), proj.o, 0x88FFFF, 2.f, 50, DECAL_BLOOD);
				proj.lasteffect = lastmillis;
			}
		}
		else if(proj.projtype == PRJ_DEBRIS || (proj.projtype == PRJ_GIBS && (kidmode || game::bloodscale <= 0 || game::gibscale <= 0)))
		{
			proj.lifesize = clamp(1.f-proj.lifespan, 0.1f, 1.f); // gets smaller as it gets older
			int steps = clamp(int(proj.vel.magnitude()*proj.lifesize*1.5f), 5, 20);
			if(proj.canrender && steps && proj.movement > 0.f)
			{
				vec dir = vec(proj.vel).normalize().neg().mul(proj.radius*0.375f), pos = proj.o;
				loopi(steps)
				{
					float res = float(steps-i)/float(steps), size = clamp(proj.radius*0.75f*(proj.lifesize+0.1f)*res, 0.01f, proj.radius);
					int col = ((int(224*max(res,0.375f))<<16)+1)|((int(96*max(res,0.125f))+1)<<8);
					part_create(PART_FIREBALL_SOFT, 1, pos, col, size, -10);
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
				if(proj.owner && proj.owner->muzzle != vec(-1, -1, -1)) proj.from = proj.owner->muzzle;
				int vol = 255;
				switch(proj.weap)
				{
					case WEAP_PISTOL:
					{
						vol = int(255*(1.f-proj.lifespan));
						part_create(PART_SMOKE_LERP, m_speedtime(200), proj.o, 0x999999, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0], -20);
						adddecal(DECAL_BULLET, proj.o, proj.norm, 2.f);
						break;
					}
					case WEAP_FLAMER:
					case WEAP_GRENADE:
					{ // both basically explosions
						if(!proj.limited)
						{
							int deviation = int(weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.5f);
							loopi((proj.weap == WEAP_FLAMER ? 1 : rnd(7))+4)
							{
								vec to(proj.o);
								loopk(3) to.v[k] += rnd(deviation*2)-deviation;
								part_create(PART_FIREBALL_SOFT, m_speedtime(proj.weap == WEAP_FLAMER ? 350 : 750), to, proj.weap == WEAP_FLAMER ? 0xBB2600 : 0x882600, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.5f, -10);
							}
							if(proj.weap == WEAP_GRENADE)
							{
								part_create(PART_PLASMA_SOFT, m_speedtime(1000), proj.o, 0xDD4400, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.5f); // corona
								float wobble = weaptype[proj.weap].damage[proj.flags&HIT_ALT ? 1 : 0]*(1.f-camera1->o.dist(proj.o)/EXPLOSIONSCALE/weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0])*0.5f;
								if(proj.weap == m_spawnweapon(game::gamemode, game::mutators)) wobble *= 0.25f;
								hud::quakewobble = clamp(hud::quakewobble + max(int(wobble), 1), 0, 1000);
								part_fireball(proj.o, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*1.5f, PART_EXPLOSION, m_speedtime(750), 0xAA4400, 1.f);
								loopi(rnd(21)+20) create(proj.o, vec(proj.o).add(proj.vel), true, proj.owner, PRJ_DEBRIS, rnd(5001)+1500, 0, rnd(501), rnd(101)+50);
								adddecal(DECAL_ENERGY, proj.o, proj.norm, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.7f, bvec(196, 24, 0));
							}
							part_create(PART_SMOKE_LERP_SOFT, m_speedtime(proj.weap == WEAP_FLAMER ? 250 : 1500), proj.o, proj.weap == WEAP_FLAMER ? 0x666666 : 0x333333, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0], -25);
							adddecal(proj.weap == WEAP_FLAMER ? DECAL_SCORCH_SHORT : DECAL_SCORCH, proj.o, proj.norm, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]);
							adddynlight(proj.o, 1.f*weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0], vec(1.1f, 0.22f, 0.02f), m_speedtime(proj.weap == WEAP_FLAMER ? 250 : 1000), 10);
						}
						else
						{
							part_create(PART_SMOKE_LERP_SOFT, m_speedtime(proj.weap == WEAP_FLAMER ? 50 : 300), proj.o, proj.weap == WEAP_FLAMER ? 0x666666 : 0x333333, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.25f, -25);
							vol = 0;
						}
						break;
					}
					case WEAP_SHOTGUN: case WEAP_SMG:
					{
						vol = int(255*(1.f-proj.lifespan));
						if(proj.lifetime)
							part_splash(PART_SPARK, proj.weap == WEAP_SHOTGUN ? 10 : 20, m_speedtime(500), proj.o, 0xFFAA22, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.75f, 10, 16);
						break;
					}
					case WEAP_PLASMA:
					{
						if(!proj.limited)
						{
							part_create(PART_PLASMA_SOFT, m_speedtime(75), proj.o, 0x55AAEE, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*proj.lifesize*0.8f);
							part_create(PART_ELECTRIC, m_speedtime(50), proj.o, 0x55AAEE, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*proj.lifesize*0.3f);
							part_create(PART_SMOKE, m_speedtime(200), proj.o, 0x8896A4, 2.4f, -20);
							adddecal(DECAL_ENERGY, proj.o, proj.norm, 6.f, bvec(98, 196, 244));
							adddynlight(proj.o, 1.1f*weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0], vec(0.1f, 0.4f, 0.6f), m_speedtime(200), 10);
						}
						else
						{
							part_create(PART_SMOKE, m_speedtime(150), proj.o, 0x8899AA, 1.2f, -20);
							vol = 0;
						}
						break;
					}
					case WEAP_RIFLE:
					{
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], 1.f, min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement));
						vec dir = dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						part_flare(proj.to, proj.o, m_speedtime(proj.flags&HIT_ALT ?  500 : 250), PART_FLARE, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
						part_flare(proj.to, proj.o, m_speedtime(proj.flags&HIT_ALT ? 500 : 250), PART_FLARE_LERP, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f);
						part_splash(PART_SPARK, proj.flags&HIT_ALT ? 10 : 25, m_speedtime(proj.flags&HIT_ALT ? 500 : 750), proj.o, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.5f, 10, 0, 24);
						if(!proj.flags&HIT_ALT)
						{
							part_create(PART_PLASMA_SOFT, m_speedtime(500), proj.o, 0x4408AA, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.5f); // corona
							float wobble = weaptype[proj.weap].damage[proj.flags&HIT_ALT ? 1 : 0]*(1.f-camera1->o.dist(proj.o)/EXPLOSIONSCALE/weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0])*0.5f;
							if(proj.weap == m_spawnweapon(game::gamemode, game::mutators)) wobble *= 0.25f;
							hud::quakewobble = clamp(hud::quakewobble + max(int(wobble), 1), 0, 1000);
							part_fireball(proj.o, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.75f, PART_EXPLOSION, m_speedtime(500), 0x2211FF, 1.f);
							adddynlight(proj.o, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0], vec(0.4f, 0.05f, 1.f), m_speedtime(500), 10);
							adddecal(DECAL_SCORCH_SHORT, proj.o, proj.norm, 8.f);
							adddecal(DECAL_ENERGY, proj.o, proj.norm, 4.f, bvec(98, 16, 254));
						}
						else
						{
							adddynlight(proj.o, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*1.5f, vec(0.4f, 0.05f, 1.f), m_speedtime(250), 10);
							adddecal(DECAL_SCORCH_SHORT, proj.o, proj.norm, 5.f);
							adddecal(DECAL_ENERGY, proj.o, proj.norm, 2.f, bvec(98, 16, 254));
						}
						break;
					}
					case WEAP_GIBS:
					{
						adddecal(DECAL_BLOOD, proj.o, proj.norm, proj.radius*clamp(proj.vel.magnitude(), 0.25f, 2.f), bvec(125, 255, 255));
						break;
					}
				}
				if(vol && weaptype[proj.weap].esound >= 0) playsound(weaptype[proj.weap].esound, proj.o, NULL, 0, vol);
				if(proj.local) client::addmsg(SV_DESTROY, "ri7", proj.owner->clientnum, lastmillis-game::maptime, proj.weap, proj.flags, proj.id >= 0 ? proj.id-game::maptime : proj.id, 0, 0);
				break;
			}
			case PRJ_ENT:
			{
				if(!proj.beenused)
				{
					if(entities::ents.inrange(proj.id))
					{
						int sweap = m_spawnweapon(game::gamemode, game::mutators), attr = entities::ents[proj.id]->type == WEAPON ? weapattr(entities::ents[proj.id]->attrs[0], sweap) : entities::ents[proj.id]->attrs[0],
							colour = entities::ents[proj.id]->type == WEAPON ? weaptype[attr].colour : 0x6666FF;
						game::spawneffect(PART_FIREBALL, proj.o, colour, enttype[entities::ents[proj.id]->type].radius, 5);
					}
					if(proj.local) client::addmsg(SV_DESTROY, "ri7", proj.owner->clientnum, lastmillis-game::maptime, -1, 0, proj.id, 0, 0);
				}
				break;
			}
			default: break;
		}
	}

	int bounce(projent &proj, const vec &dir)
	{
		if((!collide(&proj, dir, 0.f, proj.projcollide&COLLIDE_PLAYER) || inside) && (hitplayer ? proj.projcollide&COLLIDE_PLAYER && hitplayer != proj.owner : proj.projcollide&COLLIDE_GEOM))
		{
			if(hitplayer) { if(!hiteffect(proj, hitplayer, hitflags, vec(hitplayer->o).sub(proj.o).normalize())) return 1; }
			else proj.norm = wall;
            if(proj.projcollide&(hitplayer ? BOUNCE_PLAYER : BOUNCE_GEOM))
			{
                bounceeffect(proj);
				reflect(proj, proj.norm);
				proj.movement = 0;
				proj.lastbounce = lastmillis;
				return 2; // bounce
			}
			else if(proj.projcollide&(hitplayer ? IMPACT_PLAYER : IMPACT_GEOM))
				return 0; // die on impact
		}
		return 1; // live!
	}

    int trace(projent &proj, const vec &dir)
    {
        vec to(proj.o), ray = dir;
        to.add(dir);
        float maxdist = ray.magnitude();
        if(maxdist <= 0) return 1; // not moving anywhere, so assume still alive since it was already alive
        ray.mul(1/maxdist);
        float dist = tracecollide(&proj, proj.o, ray, maxdist, RAY_CLIPMAT | RAY_ALPHAPOLY, proj.projcollide&COLLIDE_PLAYER);
        proj.o.add(vec(ray).mul(dist >= 0 ? dist : maxdist));
        if(dist >= 0)
        {
            if(hitplayer)
            {
				if(!hiteffect(proj, hitplayer, hitflags, vec(hitplayer->o).sub(proj.o).normalize())) return 1;
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
            else if(proj.projcollide&(hitplayer ? IMPACT_PLAYER : IMPACT_GEOM))
				return 0; // die on impact
        }
        return 1; // live!
    }

	bool move(projent &proj, int qtime)
	{
		int mat = lookupmaterial(vec(proj.o.x, proj.o.y, proj.o.z + (proj.aboveeye - proj.height)/2));
		if(int(mat&MATF_VOLUME) == MAT_LAVA || int(mat&MATF_FLAGS) == MAT_DEATH || proj.o.z < 0) return false; // gets destroyed
		bool water = isliquid(mat&MATF_VOLUME);
		float secs = float(qtime)/1000.f;
		if(proj.weight > 0.f) proj.vel.z -=  physics::gravityforce(&proj)*secs;

		vec dir(proj.vel), pos(proj.o);
		if(water)
		{
			if(proj.extinguish)
			{
				proj.limited = true;
				return false; // gets "put out"
			}
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

		float dist = proj.o.dist(pos), diff = dist/float(4*RAD);
		if(!blocked) proj.movement += dist;
		if(proj.projtype == PRJ_SHOT || proj.projtype == PRJ_DEBRIS || proj.projtype == PRJ_GIBS)
		{
			float dummy = 0;
			vectoyawpitch(vec(proj.vel).normalize(), proj.yaw, dummy);
			proj.yaw -= 90; while(proj.yaw < 0) proj.yaw += 360;
			proj.roll += diff; while(proj.roll >= 360) proj.roll -= 360;
		}
		else if(proj.projtype == PRJ_ENT && proj.pitch != 0.f)
		{
			if(proj.pitch < 0.f) { proj.pitch += diff*0.125f; if(proj.pitch > 0.f) proj.pitch = 0.f; }
			else if(proj.pitch > 0.f) { proj.pitch -= diff*0.125f; if(proj.pitch < 0.f) proj.pitch = 0.f; }
			proj.yaw = proj.roll = 0;
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
				if(projs[i]->weap == WEAP_FLAMER)
				{
					if(projs[i]->canrender)
					{
						float asize = weaptype[WEAP_FLAMER].partsize[projs[i]->flags&HIT_ALT ? 1 : 0]*projs[i]->lifesize;
						loopj(i) if(projs[j]->projtype == PRJ_SHOT && projs[j]->weap == WEAP_FLAMER && projs[j]->canrender)
						{
							float bsize = weaptype[WEAP_FLAMER].partsize[projs[j]->flags&HIT_ALT ? 1 : 0]*projs[j]->lifesize;
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
			if(proj.projtype == PRJ_SHOT && proj.radial)
			{
				proj.hit = NULL;
				proj.hitflags = HITFLAG_NONE;
			}
			hits.setsizenodelete(0);
			if(proj.owner && proj.state != CS_DEAD)
			{
				if(proj.projtype == PRJ_ENT && entities::ents.inrange(proj.id) && entities::ents[proj.id]->type == WEAPON) // in case spawnweapon changes
					proj.mdl = entities::entmdlname(entities::ents[proj.id]->type, entities::ents[proj.id]->attrs);

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
			}
			else proj.state = CS_DEAD;
			if(proj.local && proj.projtype == PRJ_SHOT)
			{
				int radius = 0;
				if(proj.state == CS_DEAD)
				{
					if(!proj.limited && weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0])
					{
						bool explode = weaptype[proj.weap].explode > 0;
						radius = weaptype[proj.weap].taper[proj.flags&HIT_ALT ? 1 : 0] ? max(int(weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*proj.radius), 1) : weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0];
						loopi(game::numdynents())
						{
							gameent *f = (gameent *)game::iterdynents(i);
							if(!f || f->state != CS_ALIVE || !physics::issolid(f)) continue;
							if(!(proj.projcollide&COLLIDE_OWNER) && f == proj.owner) continue;
							radialeffect(f, proj, explode, radius);
						}
					}
				}
				else if(proj.radial)
				{
					radius = int(weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*proj.radius);
					if(!proj.limited) radiate(proj, radius);
				}
				if(!hits.empty())
				{
					client::addmsg(SV_DESTROY, "ri6iv", proj.owner->clientnum, lastmillis-game::maptime, proj.weap, proj.flags, proj.id >= 0 ? proj.id-game::maptime : proj.id,
							radius, hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
				}
			}
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
				case WEAP_PISTOL: adddynlight(proj.o, 4, vec(0.075f, 0.075f, 0.075f)); break;
				case WEAP_SHOTGUN: adddynlight(proj.o, 16, vec(0.5f, 0.35f, 0.1f)); break;
				case WEAP_SMG: adddynlight(proj.o, 8, vec(0.5f, 0.25f, 0.05f)); break;
				case WEAP_FLAMER:
				{
					vec col(1.1f*max(1.f-proj.lifespan,0.3f), 0.2f*max(1.f-proj.lifespan,0.15f), 0.00f);
					adddynlight(proj.o, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*1.1f*proj.lifespan, col);
					break;
				}
				case WEAP_PLASMA:
				{
					vec col(0.1f*max(1.f-proj.lifespan,0.1f), 0.4f*max(1.f-proj.lifespan,0.1f), 0.6f*max(1.f-proj.lifespan,0.1f));
					adddynlight(proj.o, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*1.1f*proj.lifesize, col);
					break;
				}
				case WEAP_RIFLE:
				{
					vec pos = vec(proj.o).sub(vec(proj.vel).normalize().mul(3.f));
					adddynlight(proj.o, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*1.5f, vec(0.4f, 0.05f, 1.f));
					break;
				}
				default: break;
			}
		}
	}
}
