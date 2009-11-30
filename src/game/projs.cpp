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
	VARA(flamerlength, 50, 100, INT_MAX-1);
	VARP(flamerhint, 0, 1, 1);

	VARP(muzzleflash, 0, 3, 3); // 0 = off, 1 = only other players, 2 = only thirdperson, 3 = all
	VARP(muzzleflare, 0, 3, 3); // 0 = off, 1 = only other players, 2 = only thirdperson, 3 = all
	#define muzzlechk(a,b) (a > 0 && (a == 3 || (a == 2 && game::thirdpersonview(true)) || (a == 1 && b != game::player1)))

	int calcdamage(gameent *actor, gameent *target, int weap, int &flags, int radial, float size, float dist)
	{
		int damage = weaptype[weap].damage[flags&HIT_ALT ? 1 : 0], nodamage = 0; flags &= ~HIT_SFLAGS;
		if((flags&HIT_WAVE || (isweap(weap) && !weaptype[weap].explode[flags&HIT_ALT ? 1 : 0])) && flags&HIT_FULL) flags &= ~HIT_FULL;
		if(radial) damage = int(damage*(1.f-dist/EXPLOSIONSCALE/max(size, 1e-3f)));
		if((actor == target && !selfdamage) || (m_trial(game::gamemode) && !trialdamage)) nodamage++;
		else if(m_team(game::gamemode, game::mutators) && actor->team == target->team)
		{
			if(m_story(game::gamemode)) { if(target->team == TEAM_NEUTRAL) nodamage++; }
			else if(weap == WEAP_MELEE) nodamage++;
			else if(m_fight(game::gamemode)) switch(teamdamage)
			{
				case 2: default: break;
				case 1: if(actor->aitype < 0) break;
				case 0: nodamage++; break;
			}
		}
		if(nodamage || !hithurts(flags)) flags = HIT_WAVE|(flags&HIT_ALT ? HIT_ALT : 0); // so it impacts, but not hurts
		else if((flags&HIT_FULL) && !weaptype[weap].explode[flags&HIT_ALT ? 1 : 0]) flags &= ~HIT_FULL;
		if(hithurts(flags))
		{
			if(flags&HIT_FULL || flags&HIT_HEAD) damage = int(damage*damagescale);
			else if(flags&HIT_TORSO) damage = int(damage*0.5f*damagescale);
			else if(flags&HIT_LEGS) damage = int(damage*0.25f*damagescale);
			else damage = 0;
		}
		else damage = int(damage*damagescale);
		return damage;
	}

	void hitpush(gameent *d, projent &proj, int flags = 0, int radial = 0, float dist = 0)
	{
		vec dir, middle = d->o;
		middle.z += (d->aboveeye-d->height)/2;
        dir = vec(middle).sub(proj.o);
        float dmag = dir.magnitude();
        if(dmag > 1e-3f) dir.div(dmag);
        else dir = vec(0, 0, 1);
        float speed = proj.vel.magnitude();
        if(speed > 1e-6f)
        {
            dir.add(vec(proj.vel).div(speed));
            dmag = dir.magnitude();
            if(dmag > 1e-3f) dir.div(dmag);
            else dir = vec(0, 0, 1);
        }
		if(proj.owner && (proj.owner == game::player1 || proj.owner->ai))
		{
			int hflags = proj.flags|flags, damage = calcdamage(proj.owner, d, proj.weap, hflags, radial, float(radial), dist);
			if(damage > 0) game::hiteffect(proj.weap, hflags, damage, d, proj.owner, dir, false);
		}
		hitmsg &h = hits.add();
		h.flags = flags;
		h.target = d->clientnum;
		h.id = lastmillis-game::maptime;
		h.dist = int(dist*DMF);
		h.dir = ivec(int(dir.x*DNF), int(dir.y*DNF), int(dir.z*DNF));
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
		if(proj.projtype == PRJ_SHOT && (proj.projcollide&COLLIDE_OWNER || d != proj.owner))
		{
			proj.hit = d;
			proj.hitflags = flags;
			proj.norm = norm;
			if(!weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0] && (d->type == ENT_PLAYER || d->type == ENT_AI)) hitproj((gameent *)d, proj);
			switch(proj.weap)
			{
				case WEAP_MELEE: part_create(PART_PLASMA_SOFT, 250, proj.o, 0xFFCC22, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]); break;
				case WEAP_RIFLE: case WEAP_INSTA:
					part_splash(PART_SPARK, 25, 250, proj.o, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.125f, 1, 20, 0, 24);
					part_create(PART_PLASMA, 250, proj.o, 0x6611FF, 2, 1, 0, 0);
					adddynlight(proj.o, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*1.5f, vec(0.4f, 0.05f, 1.f), 250, 10);
					break;
				default: break;
			}
			return (proj.projcollide&COLLIDE_CONT) ? false : true;
		}
		return false;
	}

	void radialeffect(gameent *d, projent &proj, bool explode, int radius)
	{
		float maxdist = proj.weap != WEAP_MELEE && explode ? radius*wavepusharea : radius, dist = 1e16f;
		if(d->type == ENT_PLAYER)
		{
			if(!proj.o.reject(d->legs, maxdist+max(d->lrad.x, d->lrad.y)))
			{
				vec bottom(d->legs), top(d->legs); bottom.z -= d->lrad.z; top.z += d->lrad.z;
				dist = min(dist, closestpointcylinder(proj.o, bottom, top, max(d->lrad.x, d->lrad.y)).dist(proj.o));
			}
			if(!proj.o.reject(d->torso, maxdist+max(d->trad.x, d->trad.y)))
			{
				vec bottom(d->torso), top(d->torso); bottom.z -= d->trad.z; top.z += d->trad.z;
				dist = min(dist, closestpointcylinder(proj.o, bottom, top, max(d->trad.x, d->trad.y)).dist(proj.o));
			}
			if(!proj.o.reject(d->head, maxdist+max(d->hrad.x, d->hrad.y)))
			{
				vec bottom(d->head), top(d->head); bottom.z -= d->hrad.z; top.z += d->hrad.z;
				dist = min(dist, closestpointcylinder(proj.o, bottom, top, max(d->hrad.x, d->hrad.y)).dist(proj.o));
			}
		}
		else
		{
			vec bottom(d->o), top(d->o); bottom.z -= d->height; top.z += d->aboveeye;
			dist = closestpointcylinder(proj.o, bottom, top, d->radius).dist(proj.o);
		}
		if(dist <= radius) hitpush(d, proj, HIT_FULL|(explode ? HIT_EXPLODE : HIT_BURN), radius, dist);
		else if(proj.weap != WEAP_MELEE && explode && dist <= radius*wavepusharea) hitpush(d, proj, HIT_WAVE, radius, dist);
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
		const char *mdls[] = { "gibs/gibc", "gibs/gibh", "debris/debris01", "debris/debris02", "debris/debris03", "debris/debris04", "" };
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
		if(proj.movement > 1.f && (!proj.lastbounce || lastmillis-proj.lastbounce > 500)) switch(proj.projtype)
        {
            case PRJ_SHOT:
            {
                switch(proj.weap)
                {
                    case WEAP_SHOTGUN: case WEAP_SMG:
                    {
                        part_splash(PART_SPARK, 5, 250, proj.o, 0xFFAA22, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f, 1, 20, 0, 16);
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
				if(vol && weaptype[proj.weap].rsound >= 0) playsound(weaptype[proj.weap].rsound, proj.o, NULL, 0, vol);
                break;
            }
            case PRJ_GIBS:
            {
            	if(!kidmode && game::bloodscale > 0 && game::gibscale > 0)
            	{
					adddecal(DECAL_BLOOD, proj.o, proj.norm, proj.radius*clamp(proj.vel.magnitude(), 0.25f, 2.f), bvec(125, 255, 255));
					int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 0, 255);
					if(vol) playsound(S_SPLOSH, proj.o, NULL, 0, vol);
					break;
            	} // otherwise fall through
            }
            case PRJ_DEBRIS:
            {
       	        int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 0, 255);
                if(vol) playsound(S_DEBRIS, proj.o, NULL, 0, vol);
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
				if(proj.owner && (proj.owner != game::player1 || waited)) proj.o = proj.from = proj.owner->muzzlepos(proj.weap);
				proj.aboveeye = proj.height = proj.radius = weaptype[proj.weap].radius[proj.flags&HIT_ALT ? 1 : 0];
				proj.elasticity = weaptype[proj.weap].elasticity[proj.flags&HIT_ALT ? 1 : 0];
				proj.reflectivity = weaptype[proj.weap].reflectivity[proj.flags&HIT_ALT ? 1 : 0];
				proj.relativity = weaptype[proj.weap].relativity[proj.flags&HIT_ALT ? 1 : 0];
				proj.waterfric = weaptype[proj.weap].waterfric[proj.flags&HIT_ALT ? 1 : 0];
				proj.weight = weaptype[proj.weap].weight[proj.flags&HIT_ALT ? 1 : 0];
				proj.projcollide = weaptype[proj.weap].collide[proj.flags&HIT_ALT ? 1 : 0];
				proj.extinguish = weaptype[proj.weap].extinguish[proj.flags&HIT_ALT ? 1 : 0];
				proj.mdl = weaptype[proj.weap].proj;
				proj.escaped = !proj.owner;
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
					proj.mdl = rnd(2) ? "gibs/gibc" : "gibs/gibh";
					proj.aboveeye = 1.0f;
					proj.elasticity = 0.35f;
					proj.reflectivity = 0.f;
					proj.relativity = 1.0f;
					proj.waterfric = 2.0f;
					proj.weight = 125.f;
					proj.vel.add(vec(rnd(20)-11, rnd(20)-11, rnd(20)-11));
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
				proj.weight = 150.f;
				proj.projcollide = BOUNCE_GEOM;
				proj.o.sub(vec(0, 0, proj.owner->height*0.2f));
				proj.vel.add(vec(rnd(50)-26, rnd(50)-26, rnd(25)));
				break;
			}
			default: break;
		}
		if(proj.projtype != PRJ_SHOT)
		{
			if(proj.mdl && *proj.mdl)
			{
				setbbfrommodel(&proj, proj.mdl);
				if(proj.projtype == PRJ_ENT && entities::ents.inrange(proj.id) && entities::ents[proj.id]->type == WEAPON)
					proj.height += 2.5f;
				else proj.height += proj.projtype == PRJ_ENT ? 1.f : 0.5f;
			}
			physics::entinmap(&proj, true);
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
		if(proj.owner && proj.relativity > 0)
		{
			vec r = vec(proj.owner->vel).add(proj.owner->falling);
			if(r.x*rel.x < 0) r.x = 0;
			if(r.y*rel.y < 0) r.y = 0;
			if(r.z*rel.z < 0) r.z = 0;
			rel.add(r.mul(proj.relativity));
		}
		proj.vel = vec(rel).add(vec(dir).mul(physics::movevelocity(&proj)));
		proj.spawntime = lastmillis;
		proj.hit = NULL;
		proj.hitflags = HITFLAG_NONE;
		proj.movement = 1;
		if(proj.projtype == PRJ_SHOT && proj.owner)
		{
			vec eyedir = vec(proj.o).sub(proj.owner->o);
			float eyedist = eyedir.magnitude();
			if(eyedist >= 1e-3f)
			{
				eyedir.div(eyedist);
				float blocked = pltracecollide(&proj, proj.owner->o, eyedir, eyedist);
				if(blocked >= 0) proj.o = vec(eyedir).mul(blocked).add(proj.owner->o);
			}
		}
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
		proj.lifetime = lifetime ? lifetime : lifetime;
		proj.lifemillis = lifemillis ? lifemillis : proj.lifetime;
		proj.waittime = waittime;
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
		if(g != WEAP_MELEE && isweap(g))
		{
			vec from(d->o), to(d->muzzlepos(g));
			if(entities::ents.inrange(n))
			{
				if(!m_noitems(game::gamemode, game::mutators) && itemdropping && !(entities::ents[n]->attrs[1]&WEAP_F_FORCED))
					create(from, to, local, d, PRJ_ENT, itemspawntime, itemspawntime, 1, 1, n);
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

		if(weaptype[weap].sound >= 0)
		{
			if(weap == WEAP_FLAMER && !(flags&HIT_ALT))
			{
				int ends = lastmillis+(d->weapwait[weap]*2);
				if(issound(d->wschan)) sounds[d->wschan].ends = ends;
				else playsound(weaptype[weap].sound, d->o, d, SND_LOOP, -1, -1, -1, &d->wschan, ends);
			}
			else if(!weaptype[weap].time || life)
				playsound(weaptype[weap].sound+(flags&HIT_ALT ? 1 : 0), d->o, d, 0, -1, -1, -1, &d->wschan);
		}

		switch(weap)
		{
			case WEAP_MELEE: break;
			case WEAP_PISTOL:
			{
				part_create(PART_SMOKE_LERP, 200, from, 0xAAAAAA, 1, 1, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_MUZZLE_FLASH, 100, from, 0xFFCC22, 1.5f, 1, 0, 0, d);
				if(muzzlechk(muzzleflare, d))
				{
					vec to; vecfromyawpitch(d->yaw, d->pitch, 1, 0, to);
					part_flare(from, to.mul(3.f).add(from), 100, PART_MUZZLE_FLARE, 0xFFCC22, 1.5f, 1, 0, 0, d);
				}
                adddynlight(from, 32, vec(0.15f, 0.15f, 0.15f), 50, 0, DL_FLASH);
				break;
			}
			case WEAP_SHOTGUN:
			{
				part_create(PART_SMOKE_LERP, 500, from, 0x666666, 4, 1, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_MUZZLE_FLASH, 100, from, 0xFFAA00, 3, 1, 0, 0, d);
				if(muzzlechk(muzzleflare, d))
				{
					vec to; vecfromyawpitch(d->yaw, d->pitch, 1, 0, to);
					part_flare(from, to.mul(16.f).add(from), 100, PART_MUZZLE_FLARE, 0xFFAA00, 4, 1, 0, 0, d);
				}
				adddynlight(from, 48, vec(1.1f, 0.77f, 0.22f), 100, 0, DL_FLASH);
				break;
			}
			case WEAP_SMG:
			{
				part_create(PART_SMOKE_LERP, 50, from, 0x999999, 1.5f, 1, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_MUZZLE_FLASH, 25, from, 0xFF8800, 2.25f, 1, 0, 0, d);
				if(muzzlechk(muzzleflare, d))
				{
					vec to; vecfromyawpitch(d->yaw, d->pitch, 1, 0, to);
					part_flare(from, to.mul(12.f).add(from), 25, PART_MUZZLE_FLARE, 0xFF8800, 3, 1, 0, 0, d);
				}
                adddynlight(from, 32, vec(1.1f, 0.55f, 0.11f), 50, 0, DL_FLASH);
				break;
			}
			case WEAP_FLAMER:
			{
				part_create(PART_SMOKE_LERP, 100, from, 0x333333, 2, 1, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_FIREBALL, 50, from, firecols[rnd(FIRECOLOURS)], 1.5f, 0.75f, -15, 0, d);
				adddynlight(from, 48, vec(1.1f, 0.3f, 0.01f), 100, 0, DL_FLASH);
				break;
			}
			case WEAP_PLASMA:
			{
				part_create(PART_SMOKE, 150, from, 0x88AABB, 0.6f, 1, -20);
				if(muzzlechk(muzzleflash, d)) part_create(PART_PLASMA, 75, from, 0x226688, 1.25f, 1, 0, 0, d);
				adddynlight(from, 24, vec(0.1f, 0.4f, 0.6f), 75, 0, DL_FLASH);
				break;
			}
			case WEAP_RIFLE: case WEAP_INSTA:
			{
				part_create(PART_SMOKE_LERP, 100, from, 0x444444, 0.8f, 1, -40);
				if(muzzlechk(muzzleflash, d)) part_create(PART_PLASMA, 75, from, 0x6611FF, 1.25f, 1, 0, 0, d);
				if(muzzlechk(muzzleflare, d))
				{
					vec to; vecfromyawpitch(d->yaw, d->pitch, 1, 0, to);
					part_flare(from, to.mul(8.f).add(from), 75, PART_MUZZLE_FLARE, 0x6611FF, 2, 1, 0, 0, d);
				}
                adddynlight(from, 32, vec(0.4f, 0.05f, 1.f), 75, 0, DL_FLASH);
				break;
			}
			default: break;
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
		if((!proj.lastradial || d || (lastmillis-proj.lastradial >= 100)) && radius > 0)
		{ // for the flamer this results in at most 50 damage per second
			if(!d)
			{
				bool radiated = false;
				loopi(game::numdynents())
				{
					gameent *f = (gameent *)game::iterdynents(i);
					if(!f || f->state != CS_ALIVE || !physics::issolid(f) || (!(proj.projcollide&COLLIDE_OWNER) && f == proj.owner))
						continue;
					radialeffect(f, proj, false, radius);
					radiated = true;
				}
				if(radiated) proj.lastradial = lastmillis;
			}
			else if(d->state == CS_ALIVE && physics::issolid(d) && (proj.projcollide&COLLIDE_OWNER || d != proj.owner)) radialeffect(d, proj, false, radius);
		}
	}

	VAR(testmelee, 0, 0, 1);
	void effect(projent &proj)
	{
		proj.lifespan = clamp((proj.lifemillis-proj.lifetime)/float(max(proj.lifemillis, 1)), 0.f, 1.f);
		if(proj.projtype == PRJ_SHOT)
		{
			if(weaptype[proj.weap].follows[proj.flags&HIT_ALT ? 1 : 0] && proj.owner) proj.from = proj.owner->muzzlepos(proj.weap);
			if(weaptype[proj.weap].fsound >= 0)
			{
				int vol = 255;
				switch(weaptype[proj.weap].fsound)
				{
					case S_BEEP: case S_BURN: case S_BURNING: vol = 55+int(200*proj.lifespan); break;
					case S_WHIZZ: case S_WHIRR: vol = 55+int(200*(1.f-proj.lifespan)); break;
					default: break;
				}
				if(issound(proj.schan)) sounds[proj.schan].vol = vol;
				else playsound(weaptype[proj.weap].fsound, proj.o, &proj, SND_LOOP, vol, -1, -1, &proj.schan);
			}
			switch(proj.weap)
			{
				case WEAP_MELEE: proj.lifesize = 1.f-proj.lifespan; if(testmelee) part_create(PART_PLASMA_SOFT, 1, proj.o, 0xDD4400, weaptype[proj.weap].radius[proj.flags&HIT_ALT ? 1 : 0]*proj.lifesize); break;
				case WEAP_PISTOL:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= 200;
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0]*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(220*max(1.f-proj.lifesize,0.3f))<<16))|((int(160*max(1.f-proj.lifesize,0.2f)))<<8);
						part_flare(proj.to, proj.o, 1, PART_FLARE, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
						part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f);
						part_create(PART_PLASMA, 1, proj.o, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
					}
					break;
				}
				case WEAP_FLAMER:
				{
					proj.lifesize = clamp(proj.flags&HIT_ALT ? proj.lifespan*proj.lifespan : proj.lifespan, 0.1f, 1.f);
					if(proj.movement > 0.f)
					{
						bool effect = false;
						float size = weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*proj.lifesize;
						if(flamertrail && lastmillis-proj.lasteffect >= flamerdelay) { effect = true; proj.lasteffect = lastmillis; }
						int col = ((int(254*max((1.f-proj.lifespan),proj.flags&HIT_ALT ? 0.6f : 0.4f))<<16)+1)|((int(112*max((1.f-proj.lifespan),0.25f))+1)<<8),
							len = effect ? max(int(flamerlength*(proj.flags&HIT_ALT ? 2 : 1)*max(proj.lifespan, 0.1f)), 1) : 1;
						if(flamerhint) part_create(PART_HINT_SOFT, 1, proj.o, 0x120226, size*1.5f, clamp(1.25f-proj.lifespan, 0.5f, 1.f)*(proj.flags&HIT_ALT ? 0.75f : 1.f));
						part_create(PART_FIREBALL_SOFT, len, proj.o, col, size, clamp(1.25f-proj.lifespan, 0.55f, 0.85f), -15);
					}
					break;
				}
				case WEAP_GRENADE:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					int col = ((int(254*max(1.f-proj.lifespan,0.5f))<<16)+1)|((int(98*max(1.f-proj.lifespan,0.f))+1)<<8), interval = lastmillis%1000;
					float fluc = 1.f+(interval ? (interval <= 500 ? interval/500.f : (1000-interval)/500.f) : 0.f);
					part_create(PART_PLASMA_SOFT, 1, proj.o, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*fluc);
					bool moving = proj.movement > 0.f;
					if(lastmillis-proj.lasteffect >= (moving ? 50 : 100))
					{
						part_create(PART_SMOKE_LERP, 250, proj.o, 0x222222, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*(moving ? 0.5f : 1.f), 1, -20);
						proj.lasteffect = lastmillis;
					}
					break;
				}
				case WEAP_SHOTGUN:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= 200;
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0]*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(224*max(1.f-proj.lifesize,0.3f))<<16)+1)|((int(144*max(1.f-proj.lifesize,0.15f))+1)<<8);
						part_flare(proj.to, proj.o, 1, PART_FLARE, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0], clamp(1.25f-proj.lifespan, 0.5f, 1.f));
						part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f, clamp(1.25f-proj.lifespan, 0.5f, 1.f));
					}
					break;
				}
				case WEAP_SMG:
				{
					proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
					if(proj.movement > 0.f)
					{
						bool iter = proj.lastbounce || proj.lifemillis-proj.lifetime >= 200;
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0]*(1.f-proj.lifesize), 1.f, iter ? min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement) : dist);
						vec dir = iter || dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						int col = ((int(224*max(1.f-proj.lifesize,0.3f))<<16))|((int(80*max(1.f-proj.lifesize,0.1f)))<<8);
						part_flare(proj.to, proj.o, 1, PART_FLARE, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0], clamp(1.25f-proj.lifespan, 0.5f, 1.f));
						part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, col, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.125f, clamp(1.25f-proj.lifespan, 0.5f, 1.f));
					}
					break;
				}
				case WEAP_PLASMA:
				{
					bool taper = proj.lifespan > (proj.flags&HIT_ALT ? 0.25f : 0.0125f);
					if(!proj.stuck || !taper) proj.lifesize = taper ? 1.125f-proj.lifespan*proj.lifespan : proj.lifespan*(proj.flags&HIT_ALT ? 4.f : 80.f);
					if(proj.flags&HIT_ALT) part_fireball(proj.o, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.5f*proj.lifesize, PART_EXPLOSION, 1, 0x225599, 1.f, 0.75f);
					part_create(PART_PLASMA_SOFT, 1, proj.o, proj.flags&HIT_ALT ? 0x4488EE : 0x55AAEE, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*proj.lifesize, proj.flags&HIT_ALT ? 1.f : clamp(1.5f-proj.lifespan, 0.5f, 1.f));
					part_create(PART_ELECTRIC_SOFT, 1, proj.o, proj.flags&HIT_ALT ? 0x4488EE : 0x55AAEE, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.5f*proj.lifesize, proj.flags&HIT_ALT ? 1.f : clamp(1.5f-proj.lifespan, 0.5f, 1.f));
					break;
				}
				case WEAP_RIFLE: case WEAP_INSTA:
				{
					proj.lifesize = clamp(proj.lifespan, 1e-1f, 1.f);
					float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], 1.f, min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement));
					vec dir = dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
					proj.to = vec(proj.o).sub(vec(dir).mul(size));
					part_flare(proj.to, proj.o, 1, PART_FLARE, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
					part_flare(proj.to, proj.o, 1, PART_FLARE_LERP, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f);
					break;
				}
				case WEAP_GIBS:
				{
					proj.lifesize = 1;
					part_create(PART_HINT_SOFT, 1, proj.o, 0x661111, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
					if(lastmillis-proj.lasteffect >= game::bloodfade/10 && proj.lifetime >= min(proj.lifemillis, 1000))
					{
						part_create(PART_BLOOD, game::bloodfade, proj.o, 0x88FFFF, (rnd(20)+1)/10.f, 1, 100, DECAL_BLOOD);
						proj.lasteffect = lastmillis;
					}
					break;
				}
				default: break;
			}
			if(weaptype[proj.weap].radial[proj.flags&HIT_ALT ? 1 : 0] || weaptype[proj.weap].taper[proj.flags&HIT_ALT ? 1 : 0])
				proj.radius = weaptype[proj.weap].radius[proj.flags&HIT_ALT ? 1 : 0]*max(proj.lifesize, 0.1f);
		}
		else
		{
			if(proj.projtype == PRJ_GIBS && !kidmode && game::bloodscale > 0 && game::gibscale > 0)
			{
				proj.lifesize = 1;
				if(lastmillis-proj.lasteffect >= game::bloodfade/10 && proj.lifetime >= min(proj.lifemillis, 1000))
				{
					part_create(PART_BLOOD, game::bloodfade, proj.o, 0x88FFFF, (rnd(20)+1)/10.f, 1, 100, DECAL_BLOOD);
					proj.lasteffect = lastmillis;
				}
			}
			else if(proj.projtype == PRJ_DEBRIS || (proj.projtype == PRJ_GIBS && (kidmode || game::bloodscale <= 0 || game::gibscale <= 0)))
			{
				proj.lifesize = clamp(1.f-proj.lifespan, 0.1f, 1.f); // gets smaller as it gets older
				int steps = clamp(int(proj.vel.magnitude()*proj.lifesize*1.5f), 5, 20);
				if(steps && proj.movement > 0.f)
				{
					vec dir = vec(proj.vel).normalize().neg().mul(proj.radius*0.375f), pos = proj.o;
					loopi(steps)
					{
						float res = float(steps-i)/float(steps), size = clamp(proj.radius*(proj.lifesize+0.1f)*res, 0.1f, proj.radius);
						int col = ((int(224*max(res,0.375f))<<16)+1)|((int(96*max(res,0.125f))+1)<<8);
						part_create(PART_FIREBALL_SOFT, 1, pos, col, size, clamp(1.5f-proj.lifespan, 0.5f, 1.f), -15);
						pos.add(dir);
						if(proj.o.dist(pos) > proj.movement) break;
					}
				}
			}
			if(proj.mdl && *proj.mdl) switch(proj.projtype)
			{
				case PRJ_ENT: if(!entities::ents.inrange(proj.id)) break;
				case PRJ_GIBS: case PRJ_DEBRIS:
					if(proj.lifemillis)
					{
						int interval = min(proj.lifemillis, 1000);
						if(proj.lifetime < interval)
						{
							setbbfrommodel(&proj, proj.mdl, float(proj.lifetime)/float(interval));
							if(proj.projtype == PRJ_ENT && entities::ents.inrange(proj.id) && entities::ents[proj.id]->type == WEAPON)
								proj.height += 2.5f;
							else proj.height += proj.projtype == PRJ_ENT ? 1.f : 0.5f;
						}
					}
					break;
				default: break;
			}
		}
	}

	void destroy(projent &proj)
	{
		proj.lifespan = clamp((proj.lifemillis-proj.lifetime)/float(max(proj.lifemillis, 1)), 0.f, 1.f);
		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				if(weaptype[proj.weap].follows[proj.flags&HIT_ALT ? 1 : 0] && proj.owner) proj.from = proj.owner->muzzlepos(proj.weap);
				int vol = 255;
				switch(proj.weap)
				{
					case WEAP_MELEE: if(testmelee) part_create(PART_PLASMA_SOFT, 300, proj.o, 0x220000, weaptype[proj.weap].radius[proj.flags&HIT_ALT ? 1 : 0]); break;
					case WEAP_PISTOL:
					{
						vol = int(255*(1.f-proj.lifespan));
						part_create(PART_SMOKE_LERP, 200, proj.o, 0x999999, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0], 1, -20);
						adddecal(DECAL_BULLET, proj.o, proj.norm, 2.f);
						break;
					}
					case WEAP_FLAMER:
					case WEAP_GRENADE:
					{ // both basically explosions
						if(!proj.limited)
						{
							if(proj.weap == WEAP_FLAMER)
							{
								part_create(PART_FIREBALL_SOFT, proj.flags&HIT_ALT ? 650 : 150, proj.o, firecols[rnd(FIRECOLOURS)], weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.5f, 0.5f, -15);
								part_create(PART_SMOKE_LERP_SOFT, proj.flags&HIT_ALT ? 750 : 250, proj.o, 0x666666, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.75f, 0.5f, -25);
							}
							else
							{
								int deviation = int(weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.75f);
								loopi(rnd(15)+5)
								{
									vec to(proj.o); loopk(3) to.v[k] += rnd(deviation*2)-deviation;
									part_create(PART_FIREBALL_SOFT, 1500, to, firecols[rnd(FIRECOLOURS)], weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.5f, 0.75f, -15);
								}
								part_create(PART_PLASMA_SOFT, 1000, proj.o, 0xDD4400, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.5f); // corona
								game::quake(proj.o, weaptype[proj.weap].damage[proj.flags&HIT_ALT ? 1 : 0], weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]);
								part_fireball(proj.o, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*1.5f, PART_EXPLOSION, 750, 0xAA4400, 1.f);
								loopi(rnd(21)+20) create(proj.o, vec(proj.o).add(proj.vel), true, proj.owner, PRJ_DEBRIS, rnd(5001)+1500, 0, rnd(501), rnd(101)+50);
								adddecal(DECAL_ENERGY, proj.o, proj.norm, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.7f, bvec(196, 24, 0));
								part_create(PART_SMOKE_LERP_SOFT, 2000, proj.o, 0x333333, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0], 1, -25);
							}
							adddecal(proj.weap == WEAP_FLAMER ? DECAL_SCORCH_SHORT : DECAL_SCORCH, proj.o, proj.norm, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]);
							adddynlight(proj.o, 1.f*weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0], vec(1.1f, 0.22f, 0.02f), proj.weap == WEAP_FLAMER ? 250 : 1000, 10);
						}
						else vol = 0;
						break;
					}
					case WEAP_SHOTGUN: case WEAP_SMG:
					{
						vol = int(255*(1.f-proj.lifespan));
						if(proj.lifetime)
							part_splash(PART_SPARK, proj.weap == WEAP_SHOTGUN ? 10 : 20, 500, proj.o, 0xFFAA22, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f, 1, 20, 16);
						break;
					}
					case WEAP_PLASMA:
					{
						if(!proj.limited)
						{
							part_create(PART_PLASMA_SOFT, proj.flags&HIT_ALT ? 500 : 150, proj.o, 0x55AAEE, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*proj.radius);
							part_create(PART_ELECTRIC_SOFT, proj.flags&HIT_ALT ? 250 : 75, proj.o, 0x55AAEE, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*proj.radius*0.5f);
							part_create(PART_SMOKE, proj.flags&HIT_ALT ? 500 : 250, proj.o, 0x8896A4, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*proj.radius*0.35f, 1, -30);
							game::quake(proj.o, weaptype[proj.weap].damage[proj.flags&HIT_ALT ? 1 : 0], weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]);
							if(proj.flags&HIT_ALT) part_fireball(proj.o, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*proj.radius*0.5f, PART_EXPLOSION, 150, 0x225599, 1.f);
							adddecal(DECAL_ENERGY, proj.o, proj.norm, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*proj.radius*0.75f, bvec(98, 196, 244));
							adddynlight(proj.o, 1.1f*weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*proj.radius, vec(0.1f, 0.4f, 0.6f), proj.flags&HIT_ALT ? 300 : 100, 10);
						}
						else vol = 0;
						break;
					}
					case WEAP_RIFLE: case WEAP_INSTA:
					{
						float dist = proj.o.dist(proj.from), size = clamp(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], 1.f, min(weaptype[proj.weap].partlen[proj.flags&HIT_ALT ? 1 : 0], proj.movement));
						vec dir = dist >= size ? vec(proj.vel).normalize() : vec(proj.o).sub(proj.from).normalize();
						proj.to = vec(proj.o).sub(vec(dir).mul(size));
						part_flare(proj.to, proj.o, proj.flags&HIT_ALT ? 500 : 250, PART_FLARE, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]);
						part_flare(proj.to, proj.o, proj.flags&HIT_ALT ? 500 : 250, PART_FLARE_LERP, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.25f);
						part_splash(PART_SPARK, proj.flags&HIT_ALT ? 25 : 50, proj.flags&HIT_ALT ? 500 : 750, proj.o, 0x6611FF, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*0.125f, 1, 20, 0, 24);
						if(proj.weap == WEAP_RIFLE && !(proj.flags&HIT_ALT))
						{
							part_create(PART_PLASMA_SOFT, 500, proj.o, 0x4408AA, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.5f); // corona
							game::quake(proj.o, weaptype[proj.weap].damage[proj.flags&HIT_ALT ? 1 : 0], weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]);
							part_fireball(proj.o, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0]*0.75f, PART_EXPLOSION, 500, 0x2211FF, 1.f);
							adddynlight(proj.o, weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0], vec(0.4f, 0.05f, 1.f), 500, 10);
							adddecal(DECAL_SCORCH_SHORT, proj.o, proj.norm, 8.f);
							adddecal(DECAL_ENERGY, proj.o, proj.norm, 4.f, bvec(98, 16, 254));
						}
						else
						{
							adddynlight(proj.o, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*1.5f, vec(0.4f, 0.05f, 1.f), 250, 10);
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
					default: break;
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
						int sweap = m_weapon(game::gamemode, game::mutators), attr = entities::ents[proj.id]->type == WEAPON ? w_attr(game::gamemode, entities::ents[proj.id]->attrs[0], sweap) : entities::ents[proj.id]->attrs[0],
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
			if(hitplayer)
			{
				if(!hiteffect(proj, hitplayer, hitflags, vec(hitplayer->o).sub(proj.o).normalize())) return 1;
			}
			else
			{
				if(proj.projcollide&COLLIDE_STICK)
				{
					proj.o.sub(vec(dir).mul(proj.radius*0.125f));
					proj.stuck = true;
					return 1;
				}
				proj.norm = wall;
			}
			bounceeffect(proj);
            if(proj.projcollide&(hitplayer ? BOUNCE_PLAYER : BOUNCE_GEOM))
			{
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
            else
            {
				if(proj.projcollide&COLLIDE_STICK)
				{
					proj.o.sub(vec(dir).mul(proj.radius*0.125f));
					proj.stuck = true;
					return 1;
				}
            	proj.norm = hitsurface;
            }
            bounceeffect(proj);
            if(proj.projcollide&(hitplayer ? BOUNCE_PLAYER : BOUNCE_GEOM))
            {
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

	void checkescaped(projent &proj, const vec &pos, const vec &dir)
	{
		if(lastmillis-proj.spawntime > 350 || proj.lastbounce || proj.stuck) proj.escaped = true;
		else if(proj.projcollide&COLLIDE_TRACE)
		{
			vec to = vec(pos).add(dir);
			float x1 = floor(min(pos.x, to.x)), y1 = floor(min(pos.y, to.y)),
		  		  x2 = ceil(max(pos.x, to.x)), y2 = ceil(max(pos.y, to.y)),
				  maxdist = dir.magnitude(), dist = 1e16f;
			if(physics::xtracecollide(&proj, pos, to, x1, x2, y1, y2, maxdist, dist, proj.owner) || dist > maxdist) proj.escaped = true;
		}
		else if(physics::xcollide(&proj, dir, proj.owner)) proj.escaped = true;
		if(proj.owner == game::player1 && proj.projtype == PRJ_SHOT)
			conoutft(CON_SELF, "%s: %d %d (%s)", proj.escaped ? "escaped" : "waiting", lastmillis-proj.spawntime, proj.lastbounce, proj.stuck ? "stuck" : "free");
	}

	bool move(projent &proj, int qtime)
	{
		int mat = lookupmaterial(vec(proj.o.x, proj.o.y, proj.o.z + (proj.aboveeye - proj.height)/2));
		if(int(mat&MATF_VOLUME) == MAT_LAVA || int(mat&MATF_FLAGS) == MAT_DEATH || proj.o.z < 0) return false; // gets destroyed
		bool water = isliquid(mat&MATF_VOLUME);
		float secs = float(qtime)/1000.f;
		if(proj.weight != 0.f) proj.vel.z -= physics::gravityforce(&proj)*secs;

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

		if(!proj.escaped && proj.owner) checkescaped(proj, pos, dir);

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
            if(((proj.lifetime -= physics::physframetime) <= 0 && proj.lifemillis) || (!proj.stuck && !move(proj, physics::physframetime)))
            {
            	if(proj.lifetime < 0) proj.lifetime = 0;
                alive = false;
                break;
            }
        }
        proj.deltapos = proj.o;
        if(alive && (((proj.lifetime -= physics::physframetime) <= 0 && proj.lifemillis) || (!proj.stuck && !move(proj, physics::physframetime))))
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
		loopvrev(projs) if(projs[i]->ready() || (projs[i]->projtype == PRJ_DEBRIS || projs[i]->projtype == PRJ_GIBS))
			canremove.add(projs[i]);
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
			if(proj.projtype == PRJ_SHOT && weaptype[proj.weap].radial[proj.flags&HIT_ALT ? 1 : 0])
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

					if(((proj.lifetime -= qtime) <= 0 && proj.lifemillis) || (!proj.stuck && !move(proj, qtime)))
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
						radius = weaptype[proj.weap].explode[proj.flags&HIT_ALT ? 1 : 0];
						if(weaptype[proj.weap].taper[proj.flags&HIT_ALT ? 1 : 0]) radius = max(int(radius*proj.radius), 1);
						loopi(game::numdynents())
						{
							gameent *f = (gameent *)game::iterdynents(i);
							if(!f || f->state != CS_ALIVE || !physics::issolid(f) || (!(proj.projcollide&COLLIDE_OWNER) && f == proj.owner)) continue;
							radialeffect(f, proj, true, radius);
						}
					}
				}
				else if(weaptype[proj.weap].radial[proj.flags&HIT_ALT ? 1 : 0])
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
			float trans = 1, size = 1;
			switch(proj.projtype)
			{
				case PRJ_GIBS: case PRJ_DEBRIS: case PRJ_ENT:
					if(proj.lifemillis)
					{
						int interval = min(proj.lifemillis, 1000);
						if(proj.lifetime < interval) size = trans = float(proj.lifetime)/float(interval);
					}
					break;
				default: break;
			}
			rendermodel(&proj.light, proj.mdl, ANIM_MAPMODEL|ANIM_LOOP, proj.o, proj.yaw+90, proj.pitch, proj.roll, MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_DYNSHADOW|MDL_LIGHT|MDL_CULL_DIST, NULL, NULL, 0, 0, trans, size);
		}
	}

	void adddynlights()
	{
		loopv(projs) if(projs[i]->ready() && projs[i]->projtype == PRJ_SHOT)
		{
			projent &proj = *projs[i];
			switch(proj.weap)
			{
				case WEAP_MELEE: default: break;
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
				case WEAP_RIFLE: case WEAP_INSTA:
				{
					vec pos = vec(proj.o).sub(vec(proj.vel).normalize().mul(3.f));
					adddynlight(proj.o, weaptype[proj.weap].partsize[proj.flags&HIT_ALT ? 1 : 0]*1.5f, vec(0.4f, 0.05f, 1.f));
					break;
				}
			}
		}
	}
}

