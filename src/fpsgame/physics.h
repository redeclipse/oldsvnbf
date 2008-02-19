struct physics
{
	GAMECLIENT &cl;

	IVARW(crawlspeed,		1,			25,			INT_MAX-1);	// crawl speed
	IVARW(gravity,			0,			25,			INT_MAX-1);	// gravity
	IVARW(jumpvel,			0,			60,			INT_MAX-1);	// extra velocity to add when jumping
	IVARW(movespeed,		1,			45,			INT_MAX-1);	// speed
	IVARW(watervel,			0,			60,			1024);		// extra water velocity

	IVARP(floatspeed,		10,			100,		1000);
	IVARP(minframetime,		5,			10,			20);

	int spawncycle, fixspawn, physicsfraction, physicsrepeat;

	physics(GAMECLIENT &_cl) : cl(_cl)
	{
		#define movedir(name, v, d, s, os) \
			CCOMMAND(name, "D", (physics *self, int *down), { \
				self->cl.player1->s = *down != 0; \
				self->cl.player1->v = self->cl.player1->s ? d : (self->cl.player1->os ? -(d) : 0); \
			});

		movedir(backward,	move,	-1,		k_down,		k_up);
		movedir(forward,	move,	1,		k_up,		k_down);
		movedir(left,		strafe,	1,		k_left,		k_right);
		movedir(right,		strafe,	-1,		k_right,	k_left);

		CCOMMAND(crouch, "D", (physics *self, int *down), { self->docrouch(*down!=0); });
		CCOMMAND(jump,   "D", (physics *self, int *down), { self->dojump(*down!=0); });
		CCOMMAND(attack, "D", (physics *self, int *down), { self->doattack(*down!=0); });
		CCOMMAND(reload, "D", (physics *self, int *down), { self->doreload(*down!=0); });
		CCOMMAND(use, "D", (physics *self, int *down), { self->douse(*down!=0); });
		CCOMMAND(lean, "D", (physics *self, int *down), { self->dolean(*down!=0); });
        CCOMMAND(taunt, "", (physics *self), { self->taunt(); });

		physicsfraction = physicsrepeat = 0;
		spawncycle = -1;
		fixspawn = 4;
	}

	// inputs
	#define iput(x,y,z) \
		void do##x(bool on) \
		{ \
			bool val = !cl.intermission ? on : false; \
			cl.player1->y = cl.player1->state != CS_DEAD ? val : false; \
			if (z && cl.player1->state == CS_DEAD && val) cl.respawn(); \
		}

	iput(crouch,	crouching,	false);
	iput(jump,		jumping,	false);
	iput(attack,	attacking,	true);
	iput(reload,	reloading,	true);
	iput(use,		usestuff,	true);
	iput(lean,		leaning,	false);

	void taunt()
	{
        if (cl.player1->state!=CS_ALIVE || cl.player1->physstate<PHYS_SLOPE) return;
		if (lastmillis-cl.player1->lasttaunt<1000) return;
		cl.player1->lasttaunt = lastmillis;
		cl.cc.addmsg(SV_TAUNT, "r");
	}


	vec feetpos(physent *d)
	{
		return vec(d->o).sub(vec(0, 0, d->height));
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
			return d->maxspeed * (float(d->crouching ? crawlspeed() : movespeed())/100.f);
		}
		return d->maxspeed;
	}
	float stepspeed(physent *d)
	{
		return 1.0f;
	}
	float waterdampen(physent *d)
	{
		return 8.f;
	}
	float waterfric(physent *d)
	{
		return 20.f;
	}
    float sinkfric(physent *d)
    {
        return 2.0f;
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


	void updatewater(fpsent *d, int waterlevel)
	{
		vec v(d->o.x, d->o.y, d->o.z-d->height);
		int mat = lookupmaterial(v);

		if (waterlevel || mat == MAT_WATER || mat == MAT_LAVA)
		{
			if (waterlevel)
			{
				uchar col[3] = { 255, 255, 255 };

				if (mat == MAT_WATER) getwatercolour(col);
				else if (mat == MAT_LAVA) getlavacolour(col);

				int wcol = (col[2] + (col[1] << 8) + (col[0] << 16));

				part_spawn(v, vec(d->xradius, d->yradius, 4.f), 0, 19, 100, 200, wcol);
			}

			if (waterlevel || d->inwater)
			{
				if (waterlevel < 0 && mat == MAT_WATER)
				{
					playsound(S_SPLASH1, &d->o, 255, 0, true);
				}
				else if (waterlevel > 0 && mat == MAT_WATER)
				{
					playsound(S_SPLASH2, &d->o, 255, 0, true);
				}
				else if (waterlevel < 0 && mat == MAT_LAVA)
				{
					part_spawn(v, vec(d->xradius, d->yradius, 4.f), 0, 5, 200, 500, COL_WHITE);
					if (d == cl.player1) cl.suicide(d);
				}
			}
		}
	}

	void trigger(physent *d, bool local, int floorlevel, int waterlevel)
	{
		if (waterlevel) updatewater((fpsent *)d, waterlevel);

		if (floorlevel > 0)
		{
			playsound(S_JUMP, &d->o);
		}
		else if (floorlevel < 0)
		{
			playsound(S_LAND, &d->o, 255, 0, true);
		}
	}

    void slideagainst(physent *d, vec &dir, const vec &obstacle)
    {
        vec wall(obstacle);
        if(wall.z > 0)
        {
            wall.z = 0;
            if(!wall.iszero()) wall.normalize();
        }
        dir.project(wall);
        d->vel.project(wall);
        if(d->gvel.dot(obstacle) < 0) d->gvel.project(wall);
    }

    void switchfloor(physent *d, vec &dir, const vec &floor)
    {
        if(d->gvel.dot(floor) < 0) d->gvel.projectxy(floor);
        if(d->physstate >= PHYS_SLOPE && fabs(dir.dot(d->floor)/dir.magnitude()) > 0.01f) return;
        dir.projectxy(floor);
        d->vel.projectxy(floor);
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
				switchfloor(d, dir, d->floor);
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
        if(floor.z > 0.0f && floor.z < slopez(d))
        {
            switchfloor(d, dir, floor);
            d->timeinair = 0;
            d->physstate = PHYS_SLIDE;
            d->floor = floor;
        }
		else d->physstate = PHYS_FALL;
	}

    void landing(physent *d, vec &dir, const vec &floor)
	{
    #if 0
        if(d->physstate == PHYS_FALL)
        {
            d->timeinair = 0;
            if(dir.z < 0.0f) dir.z = d->vel.z = 0.0f;
        }
    #endif
        switchfloor(d, dir, floor);
        d->timeinair = 0;

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
		}
        if(collided && (!found || obstacle.z > floor.z))
        {
            floor = obstacle;
            slide = !found && (floor.z <= 0.0f || floor.z >= slopez(d));
        }
		d->o = moved;
		return found;
	}

	bool move(physent *d, vec &dir)
	{
		vec old(d->o);
	#if 0
		if(d->physstate == PHYS_STEP_DOWN && dir.z <= 0.0f && cl.allowmove(d) && (d->move || d->strafe))
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
            obstacle = wall;
            /* check to see if there is an obstacle that would prevent this one from being used as a floor */
            if(d->type==ENT_PLAYER && ((wall.z>=slopez(d) && dir.z<0) || (wall.z<=-slopez(d) && dir.z>0)) && (dir.x || dir.y) && !collide(d, vec(dir.x, dir.y, 0)))
                obstacle = wall;
            d->o = old;
            if(d->type == ENT_CAMERA) return false;
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
		bool slide = collided,
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

	void modifyvelocity(physent *pl, bool local, bool water, bool floating, int millis)
	{
		if (floating)
		{
			if (pl->jumping)
			{
				pl->jumping = pl->crouching = false;
				pl->vel.z = jumpvelocity(pl);
			}
		}
		else if (pl->physstate >= PHYS_SLOPE || water)
		{
			if (water && pl->crouching) pl->crouching = false;
			if (pl->jumping)
			{
				pl->jumping = pl->crouching = false;

				pl->vel.z = jumpvelocity(pl);
				if(water) { pl->vel.x /= waterdampen(pl); pl->vel.y /= waterdampen(pl); }
				trigger(pl, local, 1, 0);
			}
		}
        if (pl->physstate == PHYS_FALL) pl->timeinair += curtime;

		vec m(0.0f, 0.0f, 0.0f);
		if(pl->type==ENT_AI)
		{
			dynent *d = (dynent *)pl;
			if(d->rotspeed && d->yaw!=d->targetyaw)
			{
				float oldyaw = d->yaw, diff = d->rotspeed*millis/1000.0f, maxdiff = fabs(d->targetyaw-d->yaw);
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

        bool wantsmove = cl.allowmove(pl) && (pl->move || pl->strafe);
		if(m.iszero() && wantsmove)
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
		else if(!water) d.mul((wantsmove ? 1.3f : 1.0f) * (pl->physstate < PHYS_SLOPE ? 1.3f : 1.0f)); // EXPERIMENTAL
		float friction = water && !floating ? waterfric(pl) : (pl->physstate >= PHYS_SLOPE || floating ? floorfric(pl) : airfric(pl));
		float fpsfric = friction/millis*20.0f;

		pl->vel.mul(fpsfric-1);
		pl->vel.add(d);
		pl->vel.div(fpsfric);
	}

    void modifygravity(physent *pl, bool water, int curtime)
    {
        float secs = curtime/1000.0f;
        vec g(0, 0, 0);
        if(pl->physstate == PHYS_FALL) g.z -= gravityforce(pl)*secs;
        else if(!pl->floor.iszero() && pl->floor.z < floorz(pl))
        {
            g.z = -1;
            g.project(pl->floor);
            g.normalize();
            g.mul(gravityforce(pl)*secs);
        }
        if(!water || !cl.allowmove(pl) || (!pl->move && !pl->strafe)) pl->gvel.add(g);

        if(!water && pl->physstate < PHYS_SLOPE) return;

        float friction = water ? sinkfric(pl) : floorfric(pl),
              fpsfric = friction/curtime*20.0f,
              c = water ? 1.0f : clamp((pl->floor.z - slopez(pl))/(floorz(pl)-slopez(pl)), 0.0f, 1.0f);
        pl->gvel.mul(1 - c/fpsfric);
    }

	// main physics routine, moves a player/monster for a time step
	// moveres indicated the physics precision (which is lower for monsters and multiplayer prediction)
	// local is false for multiplayer prediction

	bool moveplayer(physent *pl, int moveres, bool local, int millis)
	{
        int material = lookupmaterial(vec(pl->o.x, pl->o.y, pl->o.z + (2*pl->aboveeye - pl->height)/3));
		bool water = isliquid(material);
		bool floating = (editmode && local) || pl->state==CS_EDITING || pl->state==CS_SPECTATOR;
		float secs = millis/1000.f;

		// apply any player generated changes in velocity
		modifyvelocity(pl, local, water, floating, millis);
		// apply gravity
		if(!floating && pl->type!=ENT_CAMERA) modifygravity(pl, water, millis);

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
			int collisions = 0;
			vec vel(pl->vel);

			d.mul(f);
			loopi(moveres) if(!move(pl, d)) { if(pl->type==ENT_CAMERA) return false; if(++collisions<5) i--; } // discrete steps collision detection & sliding
			if(!pl->timeinair && vel.z <= -64) // if we land after long time must have been a high jump, make thud sound
			{
				trigger(pl, local, -1, 0);
			}
		}

		if(pl->type!=ENT_CAMERA && pl->state==CS_ALIVE) updatedynentcache(pl);

		if(!pl->timeinair && pl->physstate >= PHYS_FLOOR && pl->vel.squaredlen() < 1e-4f && pl->gvel.iszero()) pl->moving = false;

		pl->lastmoveattempt = lastmillis;
		if (pl->o!=oldpos) pl->lastmove = lastmillis;

		if (pl->type!=ENT_CAMERA)
		{
            int mat = lookupmaterial(vec(pl->o.x, pl->o.y, pl->o.z + (pl->aboveeye - pl->height)/2));

			bool inwater = isliquid(mat);
			if(!pl->inwater && inwater) trigger(pl, local, 0, -1);
			else if(pl->inwater && !inwater) trigger(pl, local, 0, 1);
			pl->inwater = inwater;
		}

		return true;
	}

	bool move(physent *d, int moveres = 20, bool local = true, int millis = 0, int repeat = 0)
	{
		if (!millis) millis = curtime;
		if (!repeat) repeat = physicsrepeat;

		loopi(repeat)
		{
			if (!moveplayer(d, moveres, local, min(millis, minframetime()))) return false;
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
		bbmin.z -= obstacle->height+d->aboveeye;
		bbmin.sub(space);
		vec bbmax(obstacle->o);
		bbmax.x += rad;
		bbmax.y += rad;
		bbmax.z += obstacle->aboveeye+d->height;
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
		d->o.z += d->height;	 // pos specified is at feet
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
			d->o.z += d->height;
			entinmap(d, false);
		}
	}

    IVARP(smoothmove, 0, 75, 100);
    IVARP(smoothdist, 0, 32, 64);

	void otherplayers()
	{
		loopv(cl.players) if(cl.players[i])
		{
            fpsent *d = cl.players[i];
            const int lagtime = lastmillis-d->lastupdate;
            if(!lagtime || cl.intermission) continue;
            else if(lagtime>1000 && d->state==CS_ALIVE)
			{
                d->state = CS_LAGGED;
				continue;
			}
            if(d->state==CS_ALIVE || d->state==CS_EDITING)
            {
                if(smoothmove() && d->smoothmillis>0)
                {
                    d->o = d->newpos;
                    d->yaw = d->newyaw;
                    d->pitch = d->newpitch;
                    moveplayer(d, 2, false, curtime);
                    d->newpos = d->o;
                    float k = 1.0f - float(lastmillis - d->smoothmillis)/smoothmove();
                    if(k>0)
                    {
                        d->o.add(vec(d->deltapos).mul(k));
                        d->yaw += d->deltayaw*k;
                        if(d->yaw<0) d->yaw += 360;
                        else if(d->yaw>=360) d->yaw -= 360;
                        d->pitch += d->deltapitch*k;
                    }
                }
                else moveplayer(d, 2, false, curtime);
            }
            else if(d->state==CS_DEAD && lastmillis-d->lastpain<2000) moveplayer(d, 2, false, curtime);
		}
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
		otherplayers();
	}
} ph;
