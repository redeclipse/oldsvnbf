struct physics
{
	fpsclient &cl;
	
	IVARW(gravity,		0,			25,			INT_MAX-1);	// gravity
	IVARW(movespeed,	1,			45,			INT_MAX-1);	// speed
	IVARW(crawlspeed,	1,			25,			INT_MAX-1);	// crawl speed
	IVARW(jumpvel,		0,			60,			INT_MAX-1);	// extra velocity to add when jumping
	IVARW(watertype,	WT_WATER,	WT_WATER,	WT_MAX-1);	// water type (0: water, 1: slime, 2: lava)
	IVARW(watervel,		0,			60,			1024);		// extra water velocity

	IVARP(floatspeed,	10,			100,		1000);
	IVARP(minframetime,	5,			10,			20);

	int spawncycle, fixspawn, physicsfraction, physicsrepeat;

	physics(fpsclient &_cl) : cl(_cl)
	{
		physicsfraction = physicsrepeat = 0;
		spawncycle = -1;
		fixspawn = 4;
	}

	float stairheight(physent *d)
	{
		return 4.1f;
	}
	float floorz(physent *d)
	{
		return 0.867f;
	}
	float slopez(physent *d)
	{
		return 0.5f;
	}
	float wallz(physent *d)
	{
		return 0.2f;
	}
	float jumpvelocity(physent *d)
	{
		return d->inwater ? float(watervel()) : float(jumpvel());
	}
	float gravityforce(physent *d)
	{
		return float(gravity());
	}
	float maxspeed(physent *d)
	{
		if (d->state != CS_SPECTATOR && d->state != CS_EDITING)
		{
			return d->maxspeed * (float(d->crouch ? crawlspeed() : movespeed())/100.f);
		}
		return d->maxspeed;
	}
	float stepspeed(physent *d)
	{
		return 1.0f;
	}
	float watergravscale(physent *d)
	{
		return d->move || d->strafe ? 0.f : 4.f;
	}
	float waterdampen(physent *d)
	{
		return 8.f;
	}
	float waterfric(physent *d)
	{
		return 20.f;
	}
	float floorfric(physent *d)
	{
		return 6.f;
	}
	float airfric(physent *d)
	{
		return 30.f;
	}

	bool movepitch(physent *d)
	{
		return d->type == ENT_CAMERA || d->state == CS_SPECTATOR || d->state == CS_EDITING;
	}


	void updateroll(physent *d)
	{
		d->roll = d->roll/(1+(float)sqrtf((float)curtime)/25);
	}

	void updatewater(fpsent *d, int waterlevel)
	{
		vec v(d->o.x, d->o.y, d->o.z-d->eyeheight);
		int mat = lookupmaterial(v);
			
		if (waterlevel || mat == MAT_WATER || mat == MAT_LAVA)
		{
			if (waterlevel)
			{
				uchar col[3] = { 255, 255, 255 };

				if (mat == MAT_WATER) getwatercolour(col);
				else if (mat == MAT_LAVA) getlavacolour(col);
					
				int wcol = (col[2] + (col[1] << 8) + (col[0] << 16));
				
				part_spawn(v, vec(d->xradius, d->yradius, ENTPART), 0, 19, 100, 200, wcol);
			}
			
			if (waterlevel || d->inwater)
			{
				int water = watertype();
				
				if (waterlevel < 0 && (mat == MAT_WATER && water == WT_WATER))
				{
					playsound(S_SPLASH1, &d->o, &d->vel);
				}
				else if (waterlevel > 0 && (mat == MAT_WATER && water == WT_WATER))
				{
					playsound(S_SPLASH2, &d->o, &d->vel);
				}
				else if (waterlevel < 0 && ((mat == MAT_WATER && (water == WT_KILL || water == WT_HURT)) || mat == MAT_LAVA))
				{
					part_spawn(v, vec(d->xradius, d->yradius, ENTPART), 0, 5, 200, 500, COL_WHITE);
					if (mat != MAT_LAVA && d == cl.player1) cl.suicide(d);
				}
			}
		}
	}
	
	void trigger(physent *d, bool local, int floorlevel, int waterlevel)
	{
		if (waterlevel) updatewater((fpsent *)d, waterlevel);
			
		if (floorlevel > 0)
		{
			playsound(S_JUMP, &d->o, &d->vel);
		}
		else if (floorlevel < 0)
		{
			playsound(S_LAND, &d->o, &d->vel);
		}
	}
	
	void slopegravity(float g, const vec &slope, vec &gvec)
	{
		float k = slope.z*g/(slope.x*slope.x + slope.y*slope.y);
		gvec.x = slope.x*k;
		gvec.y = slope.y*k;
		gvec.z = -g;
	}
	
	void slideagainst(physent *d, vec &dir, const vec &obstacle)
	{
		vec wdir(obstacle), wvel(obstacle);
		wdir.z = wvel.z = d->physstate >= PHYS_SLOPE ? 0.0f : min(obstacle.z, 0.0f);
	//	if(obstacle.z < 0.0f && dir.z > 0.0f) dir.z = d->vel.z = 0.0f;
		wdir.mul(obstacle.dot(dir));
		wvel.mul(obstacle.dot(d->vel));
		dir.sub(wdir);
		d->vel.sub(wvel);
	}
	
	void switchfloor(physent *d, vec &dir, bool landing, const vec &floor)
	{
		if(d->physstate == PHYS_FALL || d->floor.z < floorz(d))
		{
			if(landing)
			{
				if(floor.z >= floorz(d))
				{
					if(d->vel.z + d->gvel.z > 0) d->vel.add(d->gvel);
					else if(d->vel.z > 0) d->vel.z = 0.0f;
					d->gvel = vec(0, 0, 0);
				}
				else
				{
					float gmag = d->gvel.magnitude();
					if(gmag > 1e-4f)
					{
						vec g;
						slopegravity(-d->gvel.z, floor, g);
						if(d->physstate == PHYS_FALL || d->floor != floor)
						{
							float c = d->gvel.dot(floor)/gmag;
							g.normalize();
							g.mul(min(1.0f+c, 1.0f)*gmag);
						}
						d->gvel = g;
					}
				}
			}
		}
	
		if(((d->physstate == PHYS_SLIDE || (d->physstate == PHYS_FALL && floor.z < 1.0f)) && landing) ||
			(d->physstate >= PHYS_SLOPE && fabs(dir.dot(d->floor)/dir.magnitude()) < 0.01f))
		{
			if(floor.z > 0 && floor.z < wallz(d)) { slideagainst(d, dir, floor); return; }
			float dmag = dir.magnitude(), dz = -(dir.x*floor.x + dir.y*floor.y)/floor.z;
			dir.z = dz;
			float dfmag = dir.magnitude();
			if(dfmag > 0) dir.mul(dmag/dfmag);
	
			float vmag = d->vel.magnitude(), vz = -(d->vel.x*floor.x + d->vel.y*floor.y)/floor.z;
			d->vel.z = vz;
			float vfmag = d->vel.magnitude();
			if(vfmag > 0) d->vel.mul(vmag/vfmag);
		}
	}
	
	bool trystepup(physent *d, vec &dir, float maxstep)
	{
		vec old(d->o);
		/* check if there is space atop the stair to move to */
		if(d->physstate != PHYS_STEP_UP)
		{
			d->o.add(dir);
			d->o.z += maxstep + 0.1f;
			if(!collide(d))
			{
				d->o = old;
				return false;
			}
		}
		/* try stepping up */
		d->o = old;
		d->o.z += dir.magnitude()*stepspeed(d);
		if(collide(d, vec(0, 0, 1)))
		{
			if(d->physstate == PHYS_FALL)
			{
				d->timeinair = 0;
				d->floor = vec(0, 0, 1);
				switchfloor(d, dir, true, d->floor);
			}
			d->physstate = PHYS_STEP_UP;
			return true;
		}
		d->o = old;
		return false;
	}
	
	#if 0
	bool trystepdown(physent *d, vec &dir, float step, float a, float b)
	{
		vec old(d->o);
		vec dv(dir.x*a, dir.y*a, -step*b), v(dv);
		v.mul(stairheight(d)/(step*b));
		d->o.add(v);
		if(!collide(d, vec(0, 0, -1), slopez(d)))
		{
			d->o = old;
			d->o.add(dv);
			if(collide(d, vec(0, 0, -1))) return true;
		}
		d->o = old;
		return false;
	}
	#endif
	
	void falling(physent *d, vec &dir, const vec &floor)
	{
	#if 0
		if(d->physstate >= PHYS_FLOOR && (floor.z == 0.0f || floor.z == 1.0f))
		{
			vec moved(d->o);
			d->o.z -= stairheight(d) + 0.1f;
			if(!collide(d, vec(0, 0, -1), slopez(d)))
			{
				d->o = moved;
				d->physstate = PHYS_STEP_DOWN;
				return;
			}
			else d->o = moved;
		}
	#endif
		bool sliding = floor.z > 0.0f && floor.z < slopez(d);
		switchfloor(d, dir, sliding, sliding ? floor : vec(0, 0, 1));
		if(sliding)
		{
			if(d->timeinair > 0) d->timeinair = 0;
			d->physstate = PHYS_SLIDE;
			d->floor = floor;
		}
		else d->physstate = PHYS_FALL;
	}
	
	void landing(physent *d, vec &dir, const vec &floor)
	{
		if(d->physstate == PHYS_FALL)
		{
			d->timeinair = 0;
			if(dir.z < 0.0f) dir.z = d->vel.z = 0.0f;
		}
		switchfloor(d, dir, true, floor);
		if(floor.z >= floorz(d)) d->physstate = PHYS_FLOOR;
		else d->physstate = PHYS_SLOPE;
		d->floor = floor;
	}
	
	bool findfloor(physent *d, bool collided, const vec &obstacle, bool &slide, vec &floor)
	{
		bool found = false;
		vec moved(d->o);
		d->o.z -= 0.1f;
		if(!collide(d, vec(0, 0, -1), d->physstate == PHYS_SLOPE ? slopez(d) : floorz(d)))
		{
			floor = wall;
			found = true;
		}
		else if(collided && obstacle.z >= slopez(d))
		{
			floor = obstacle;
			found = true;
			slide = false;
		}
		else
		{
			if(d->physstate == PHYS_STEP_UP || d->physstate == PHYS_SLIDE)
			{
				if(!collide(d, vec(0, 0, -1)) && wall.z > 0.0f)
				{
					floor = wall;
					if(floor.z > slopez(d)) found = true;
				}
			}
			else
			{
				d->o.z -= d->radius;
				if(d->physstate >= PHYS_SLOPE && d->floor.z < 1.0f && !collide(d, vec(0, 0, -1)))
				{
					floor = wall;
					if(floor.z >= slopez(d) && floor.z < 1.0f) found = true;
				}
			}
			if(collided && (!found || obstacle.z > floor.z)) floor = obstacle;
		}
		d->o = moved;
		return found;
	}
	
	bool move(physent *d, vec &dir)
	{
		vec old(d->o);
	#if 0
		if(d->physstate == PHYS_STEP_DOWN && dir.z <= 0.0f && cl.allowmove(pl) && (d->move || d->strafe))
		{
			float step = dir.magnitude()*stepspeed(d);
			if(trystepdown(d, dir, step, 0.75f, 0.25f)) return true;
			if(trystepdown(d, dir, step, 0.5f, 0.5f)) return true;
			if(trystepdown(d, dir, step, 0.25f, 0.75f)) return true;
			d->o.z -= step;
			if(collide(d, vec(0, 0, -1))) return true;
			d->o = old;
		}
	#endif
		bool collided = false;
		vec obstacle;
		d->o.add(dir);
		if(!collide(d, d->type!=ENT_CAMERA ? dir : vec(0, 0, 0)) || (d->type==ENT_AI && !collide(d)))
		{
			d->o = old;
			if(d->type == ENT_CAMERA) return false;
			obstacle = wall;
			d->o.z -= (d->physstate >= PHYS_SLOPE && d->floor.z < 1.0f ? d->radius+0.1f : stairheight(d));
			if((d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR) || (!collide(d, vec(0, 0, -1), slopez(d)) && (d->physstate == PHYS_STEP_UP || wall.z == 1.0f)))
			{
				d->o = old;
				float floorz = (d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR ? d->floor.z : wall.z);
				if(trystepup(d, dir, floorz < 1.0f ? d->radius+0.1f : stairheight(d))) return true;
			}
			else d->o = old;
			/* can't step over the obstacle, so just slide against it */
			collided = true;
		}
		vec floor(0, 0, 0);
		bool slide = collided && obstacle.z < 1.0f,
			 found = findfloor(d, collided, obstacle, slide, floor);
		if(slide)
		{
			slideagainst(d, dir, obstacle);
			if(d->type == ENT_AI || d->type == ENT_INANIMATE) d->blocked = true;
		}
		if(found)
		{
			if(d->type == ENT_CAMERA) return false;
			landing(d, dir, floor);
		}
		else falling(d, dir, floor);
		return !collided;
	}

	void modifyvelocity(physent *pl, bool local, bool water, bool floating, int curtime)
	{
		if (floating)
		{
			if (pl->crouch) pl->crouch = false;
			if (pl->jumpnext)
			{
				pl->jumpnext = false;
				pl->vel.z = jumpvelocity(pl);
			}
		}
		else if (pl->physstate >= PHYS_SLOPE || water)
		{
			if (water)
			{
				if( pl->crouch) pl->crouch = false;
				if (pl->timeinair > 0)
				{
					pl->timeinair = 0;
					pl->vel.z = 0;
				}
			}
			if(pl->jumpnext)
			{
				pl->jumpnext = false;
	
				pl->vel.z = jumpvelocity(pl);
				if(water) { pl->vel.x /= waterdampen(pl); pl->vel.y /= waterdampen(pl); }
				trigger(pl, local, 1, 0);
			}
		}
		else
		{
			if (pl->crouch) pl->crouch = false;
			pl->timeinair += curtime;
		}
	
		vec m(0.0f, 0.0f, 0.0f);
		if(pl->type==ENT_AI)
		{
			dynent *d = (dynent *)pl;
			if(d->rotspeed && d->yaw!=d->targetyaw)
			{
				float oldyaw = d->yaw, diff = d->rotspeed*curtime/1000.0f, maxdiff = fabs(d->targetyaw-d->yaw);
				if(diff >= maxdiff)
				{
					d->yaw = d->targetyaw;
					d->rotspeed = 0;
				}
				else d->yaw += (d->targetyaw>d->yaw ? 1 : -1) * min(diff, maxdiff);
				d->normalize_yaw(d->targetyaw);
				if(!plcollide(d, vec(0, 0, 0)))
				{
					d->yaw = oldyaw;
					m.x = d->o.x - hitplayer->o.x;
					m.y = d->o.y - hitplayer->o.y;
					if(!m.iszero()) m.normalize();
				}
			}
		}
	
		if(m.iszero() && cl.allowmove(pl) && (pl->move || pl->strafe))
		{
			vecfromyawpitch(pl->yaw, floating || water || movepitch(pl) ? pl->pitch : 0, pl->move, pl->strafe, m);
	
			if(!floating && pl->physstate >= PHYS_SLIDE)
			{
				/* move up or down slopes in air
				 * but only move up slopes in water
				 */
				float dz = -(m.x*pl->floor.x + m.y*pl->floor.y)/pl->floor.z;
				if(water) m.z = max(m.z, dz);
				else if(pl->floor.z >= wallz(pl)) m.z = dz;
			}
	
			m.normalize();
		}
	
		vec d(m);
		d.mul(maxspeed(pl));
		if(floating) { if (local) d.mul(floatspeed()/100.0f); }
		else if(!water && cl.allowmove(pl)) d.mul((pl->move && !pl->strafe ? 1.3f : 1.0f) * (pl->physstate < PHYS_SLOPE ? 1.3f : 1.0f)); // EXPERIMENTAL
		float friction = water && !floating ? waterfric(pl) : (pl->physstate >= PHYS_SLOPE || floating ? floorfric(pl) : airfric(pl));
		float fpsfric = friction/curtime*20.0f;
	
		pl->vel.mul(fpsfric-1);
		pl->vel.add(d);
		pl->vel.div(fpsfric);
	}
	
	void modifygravity(physent *pl, bool water, float secs)
	{
		vec g(0, 0, 0);
		if(pl->physstate == PHYS_FALL) g.z -= gravityforce(pl)*secs;
		else if(!pl->floor.iszero() && pl->floor.z < floorz(pl))
		{
			float c = min(floorz(pl) - pl->floor.z, floorz(pl)-slopez(pl))/(floorz(pl)-slopez(pl));
			slopegravity(gravityforce(pl)*secs*c, pl->floor, g);
		}
		if(water) pl->gvel = g.mul(watergravscale(pl));
		else pl->gvel.add(g);
	}
	
	// main physics routine, moves a player/monster for a curtime step
	// moveres indicated the physics precision (which is lower for monsters and multiplayer prediction)
	// local is false for multiplayer prediction
	
	bool moveplayer(physent *pl, int moveres, bool local, int curtime)
	{
		int material = lookupmaterial(pl->o);
		bool water = isliquid(material);
		bool floating = (editmode && local) || pl->state==CS_EDITING || pl->state==CS_SPECTATOR;
		float secs = curtime/1000.f;
	
		// apply any player generated changes in velocity
		modifyvelocity(pl, local, water, floating, curtime);
		// apply gravity
		if(!floating && pl->type!=ENT_CAMERA) modifygravity(pl, water, secs);
	
		vec d(pl->vel), oldpos(pl->o);
		if(!floating && pl->type!=ENT_CAMERA && water) d.mul(0.5f);
		d.add(pl->gvel);
		d.mul(secs);
	
		pl->blocked = false;
		pl->moving = true;
		pl->onplayer = NULL;
	
		if(floating)				// just apply velocity
		{
			if(pl->physstate != PHYS_FLOAT)
			{
				pl->physstate = PHYS_FLOAT;
				pl->timeinair = 0;
				pl->gvel = vec(0, 0, 0);
			}
			pl->o.add(d);
		}
		else						// apply velocity with collision
		{
			const float f = 1.0f/moveres;
			const int timeinair = pl->timeinair;
			int collisions = 0;
	
			d.mul(f);
			loopi(moveres) if(!move(pl, d)) { if(pl->type==ENT_CAMERA) return false; if(++collisions<5) i--; } // discrete steps collision detection & sliding
			if(timeinair > 800 && !pl->timeinair) // if we land after long time must have been a high jump, make thud sound
			{
				trigger(pl, local, -1, 0);
			}
		}
	
		if(pl->type!=ENT_CAMERA && pl->state==CS_ALIVE) updatedynentcache(pl);
	
		if(!pl->timeinair && pl->physstate >= PHYS_FLOOR && pl->vel.squaredlen() < 1e-4f && pl->gvel.iszero()) pl->moving = false;
	
		pl->lastmoveattempt = cl.lastmillis;
		if(pl->o!=oldpos) pl->lastmove = cl.lastmillis;
	
		updateroll(pl);
	
		if(pl->type!=ENT_CAMERA)
		{
			int mat = lookupmaterial(vec(pl->o.x, pl->o.y, pl->o.z+1));
			bool inwater = isliquid(mat);
			if(!pl->inwater && inwater) trigger(pl, local, 0, -1);
			else if(pl->inwater && !inwater) trigger(pl, local, 0, 1);
			pl->inwater = inwater;
	
			if(pl->state==CS_ALIVE && material==MAT_LAVA) cl.suicide(pl);
		}
	
		return true;
	}
	
	bool move(physent *d, int moveres = 20, bool local = true, int secs = 0, int repeat = 0)
	{
		if (!secs) secs = curtime;
		if (!repeat) repeat = physicsrepeat;
		
		loopi(repeat)
		{
			if (!moveplayer(d, moveres, local, min(secs, minframetime()))) return false;
			if (d->o.z < 0 && d->state == CS_ALIVE)
			{
				cl.suicide(d);
				return false;
			}
		}
		return true;
	}

	void avoidcollision(physent *d, const vec &dir, physent *obstacle, float space)
	{
		float rad = obstacle->radius+d->radius;
		vec bbmin(obstacle->o);
		bbmin.x -= rad;
		bbmin.y -= rad;
		bbmin.z -= obstacle->eyeheight+d->aboveeye;
		bbmin.sub(space);
		vec bbmax(obstacle->o);
		bbmax.x += rad;
		bbmax.y += rad;
		bbmax.z += obstacle->aboveeye+d->eyeheight;
		bbmax.add(space);
	
		loopi(3) if(d->o[i] <= bbmin[i] || d->o[i] >= bbmax[i]) return;
	
		float mindist = 1e16f;
		loopi(3) if(dir[i] != 0)
		{
			float dist = ((dir[i] > 0 ? bbmax[i] : bbmin[i]) - d->o[i]) / dir[i];
			mindist = min(mindist, dist);
		}
		if(mindist >= 0.0f && mindist < 1e15f) d->o.add(vec(dir).mul(mindist));
	}

	void updatephysstate(physent *d)
	{
		if(d->physstate == PHYS_FALL) return;
		d->timeinair = 0;
		vec old(d->o);
		/* Attempt to reconstruct the floor state.
		 * May be inaccurate since movement collisions are not considered.
		 * If good floor is not found, just keep the old floor and hope it's correct enough.
		 */
		switch(d->physstate)
		{
			case PHYS_SLOPE:
			case PHYS_FLOOR:
				d->o.z -= 0.1f;
				if(!collide(d, vec(0, 0, -1), d->physstate == PHYS_SLOPE ? slopez(d) : floorz(d)))
					d->floor = wall;
				else if(d->physstate == PHYS_SLOPE)
				{
					d->o.z -= d->radius;
					if(!collide(d, vec(0, 0, -1), slopez(d)))
						d->floor = wall;
				}
				break;
	
			case PHYS_STEP_UP:
				d->o.z -= stairheight(d)+0.1f;
				if(!collide(d, vec(0, 0, -1), slopez(d)))
					d->floor = wall;
				break;
	
			case PHYS_SLIDE:
				d->o.z -= d->radius+0.1f;
				if(!collide(d, vec(0, 0, -1)) && wall.z < slopez(d))
					d->floor = wall;
				break;
		}
		if(d->physstate > PHYS_FALL && d->floor.z <= 0) d->floor = vec(0, 0, 1);
		d->o = old;
	}
	
	bool entinmap(dynent *d, bool avoidplayers)		// brute force but effective way to find a free spawn spot in the map
	{
		d->o.z += d->eyeheight;	 // pos specified is at feet
		vec orig = d->o;
		loopi(100)				  // try max 100 times
		{
			if(collide(d) && !inside) return true;
			if(hitplayer && avoidplayers)
			{
				d->o = orig;
				return false;
			}
			d->o = orig;
			d->o.x += (rnd(21)-10)*i/5;  // increasing distance
			d->o.y += (rnd(21)-10)*i/5;
			d->o.z += (rnd(21)-10)*i/5;
		}
		conoutf("can't find entity spawn spot! (%d, %d)", d->o.x, d->o.y);
		// leave ent at original pos, possibly stuck
		d->o = orig;
		return false;
	}

	void findplayerspawn(dynent *d, int forceent)	// place at random spawn. also used by monsters!
	{
		int pick = forceent;
		if(pick<0)
		{
			int r = fixspawn-->0 ? 7 : rnd(10)+1;
			loopi(r) spawncycle = findentity(ET_PLAYERSTART, spawncycle+1);
			pick = spawncycle;
		}
		if(pick!=-1)
		{
			d->pitch = 0;
			d->roll = 0;
			for(int attempt = pick;;)
			{
				d->o = cl.et.ents[attempt]->o;
				d->yaw = cl.et.ents[attempt]->attr1;
				if(entinmap(d, true)) break;
				attempt = findentity(ET_PLAYERSTART, attempt+1);
				if(attempt<0 || attempt==pick)
				{
					d->o = cl.et.ents[attempt]->o;
					d->yaw = cl.et.ents[attempt]->attr1;
					entinmap(d, false);
					break;
				}
			}
		}
		else
		{
			d->o.x = d->o.y = d->o.z = 0.5f*getworldsize();
			entinmap(d, false);
		}
	}

	struct platformcollision
	{
		physent *d;
		int next;
		float margin, border;
	
		platformcollision() {}
		platformcollision(physent *d, int next) : d(d), next(next), margin(0.2f), border(10.0f) {}
	};
	
	bool platformcollide(physent *d, physent *o, const vec &dir, float margin = 0)
	{
		if(d->collidetype!=COLLIDE_ELLIPSE || o->collidetype!=COLLIDE_ELLIPSE)
		{
			if(rectcollide(d, dir, o->o,
				o->collidetype==COLLIDE_ELLIPSE ? o->radius : o->xradius,
				o->collidetype==COLLIDE_ELLIPSE ? o->radius : o->yradius, o->aboveeye,
				o->eyeheight + margin))
				return true;
		}
		else if(ellipsecollide(d, dir, o->o, o->yaw, o->xradius, o->yradius, o->aboveeye, o->eyeheight + margin))
			return true;
		return false;
	}
	
	bool moveplatform(physent *p, const vec &dir)
	{   
		vec oldpos(p->o);
		p->o.add(dir);
		if(!collide(p, dir, 0, dir.z<=0))
		{
			p->o = oldpos; 
			return false;
		}
		p->o = oldpos; 
	
		static vector<physent *> candidates;
		candidates.setsizenodelete(0);
		for(int x = int(max(p->o.x-p->radius-PLATFORMBORDER, 0))>>dynentsize, ex = int(min(p->o.x+p->radius+PLATFORMBORDER, hdr.worldsize-1))>>dynentsize; x <= ex; x++)
		for(int y = int(max(p->o.y-p->radius-PLATFORMBORDER, 0))>>dynentsize, ey = int(min(p->o.y+p->radius+PLATFORMBORDER, hdr.worldsize-1))>>dynentsize; y <= ey; y++)
		{
			const vector<physent *> &dynents = checkdynentcache(x, y);
			loopv(dynents)
			{
				physent *d = dynents[i];
				if(p==d || d->o.z-d->eyeheight < p->o.z+p->aboveeye || p->o.reject(d->o, p->radius+PLATFORMBORDER+d->radius) || candidates.find(d) >= 0) continue;
				candidates.add(d);
				d->stacks = d->collisions = -1;
			}
		}
		static vector<physent *> passengers, colliders;
		passengers.setsizenodelete(0);
		colliders.setsizenodelete(0);
		static vector<platformcollision> collisions;
		collisions.setsizenodelete(0);
		// build up collision DAG of colliders to be pushed off, and DAG of stacked passengers
		loopv(candidates)
		{
			physent *d = candidates[i];
			// check if the dynent is on top of the platform
			if(!platformcollide(p, d, vec(0, 0, 1), PLATFORMMARGIN)) passengers.add(d);
			vec doldpos(d->o);
			d->o.add(dir);
			if(!collide(d, dir, 0, false)) colliders.add(d);
			d->o = doldpos;
			loopvj(candidates)
			{
				physent *o = candidates[j];
				if(!platformcollide(d, o, dir))
				{
					collisions.add(platformcollision(d, o->collisions));
					o->collisions = collisions.length() - 1;
				}
				if(d->o.z < o->o.z && !platformcollide(d, o, vec(0, 0, 1), PLATFORMMARGIN))
				{
					collisions.add(platformcollision(o, d->stacks));
					d->stacks = collisions.length() - 1;
				}
			}
		}
		loopv(colliders) // propagate collisions
		{
			physent *d = colliders[i];
			for(int n = d->collisions; n>=0; n = collisions[n].next)
			{
				physent *o = collisions[n].d;
				if(colliders.find(o)<0) colliders.add(o);
			}
		}
		if(dir.z>0)
		{
			loopv(passengers) // if any stacked passengers collide, stop the platform
			{
				physent *d = passengers[i];
				if(colliders.find(d)>=0) return false;
				for(int n = d->stacks; n>=0; n = collisions[n].next)
				{
					physent *o = collisions[n].d;
					if(passengers.find(o)<0) passengers.add(o);
				}
			}
			loopv(passengers)
			{
				physent *d = passengers[i];
				d->o.add(dir);
				d->lastmove = cl.lastmillis;
				if(dir.x || dir.y) updatedynentcache(d);
			}
		}
		else loopv(passengers) // move any stacked passengers who aren't colliding with non-passengers
		{
			physent *d = passengers[i];
			if(colliders.find(d)>=0) continue;
	
			d->o.add(dir);
			d->lastmove = cl.lastmillis;
			if(dir.x || dir.y) updatedynentcache(d);
	
			for(int n = d->stacks; n>=0; n = collisions[n].next)
			{
				physent *o = collisions[n].d;
				if(passengers.find(o)<0) passengers.add(o);
			}
		}
	
		p->o.add(dir);
		p->lastmove = cl.lastmillis;
		if(dir.x || dir.y) updatedynentcache(p);
	
		return true;
	}

	
	void update()		  // optimally schedule physics frames inside the graphics frames
	{
		if(curtime>=minframetime())
		{
			int faketime = curtime+physicsfraction;
			physicsrepeat = faketime/minframetime();
			physicsfraction = faketime%minframetime();
		}
		else
		{
			physicsrepeat = curtime>0 ? 1 : 0;
		}
		cleardynentcache();
	}
};
