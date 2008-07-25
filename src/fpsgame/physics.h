struct physics
{
	GAMECLIENT &cl;

	IVARW(crawlspeed,		1,			25,			INT_MAX-1);	// crawl speed
	IVARW(gravity,			0,			25,			INT_MAX-1);	// gravity
	IVARW(jumpvel,			0,			60,			INT_MAX-1);	// extra velocity to add when jumping
	IVARW(movespeed,		1,			45,			INT_MAX-1);	// speed
	IVARW(liquidvel,		0,			45,			1024);		// extra liquid velocity

	IVARP(floatspeed,		10,			100,		1000);
	IVARP(physframetime,	5,			5,			20);

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
		CCOMMAND(action, "D", (physics *self, int *down), { self->doaction(*down!=0); });
        CCOMMAND(taunt, "", (physics *self), { self->taunt(self->cl.player1); });

		physicsfraction = physicsrepeat = 0;
		spawncycle = -1;
		fixspawn = 4;
	}

	// inputs
	#define iput(x,y,t,z,a,q) \
		void do##x(bool down) \
		{ \
			if(!q || cl.player1->state == CS_DEAD) \
			{ \
				cl.player1->y = false; \
				if(z && down) cl.respawn(cl.player1); \
			} \
			else \
			{ \
				if((a && cl.player1->y != down) || (!a && down)) \
					cl.player1->t = lastmillis; \
				cl.player1->y = down; \
			} \
		}

	iput(crouch,	crouching,	crouchtime,	false,	true,	cl.allowmove(cl.player1));
	iput(jump,		jumping,	jumptime,	false,	false,	cl.allowmove(cl.player1));
	iput(attack,	attacking,	attacktime,	true,	false,	cl.allowmove(cl.player1));
	iput(reload,	reloading,	reloadtime,	true,	false,	cl.allowmove(cl.player1));
	iput(action,	useaction,	usetime,	true,	false,	cl.allowmove(cl.player1));

	bool iscrouching(physent *d)
	{
		return d->crouching || lastmillis-d->crouchtime <= 200;
	}

	void taunt(fpsent *d)
	{
        if(d->state!=CS_ALIVE || d->physstate<PHYS_SLOPE) return;
		if(lastmillis-d->lasttaunt<1000) return;
		d->lasttaunt = lastmillis;
		cl.cc.addmsg(SV_TAUNT, "ri", d->clientnum);
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
		return (d->inliquid ? float(liquidvel()) : float(jumpvel()))*(float(d->weight)/100.f);
	}
	float gravityforce(physent *d)
	{
		return float(gravity())*(float(d->weight)/100.f);
	}
	float maxspeed(physent *d)
	{
		if(d->type == ENT_PLAYER && d->state != CS_SPECTATOR && d->state != CS_EDITING)
		{
			return d->maxspeed*(float(iscrouching(d) ? crawlspeed() : movespeed())/100.f)*(float(d->weight)/100.f);
		}
		return d->maxspeed;
	}
	float stepspeed(physent *d)
	{
		return 1.0f;
	}
	float liquiddampen(physent *d)
	{
		return 8.f;
	}
	float liquidfric(physent *d)
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


    void recalcdir(physent *d, const vec &oldvel, vec &dir)
    {
        float speed = oldvel.magnitude();
        if(speed > 1e-6f)
        {
            float step = dir.magnitude();
            dir = d->vel;
            dir.add(d->falling);
            dir.mul(step/speed);
        }
    }

    void slideagainst(physent *d, vec &dir, const vec &obstacle, bool foundfloor)
    {
        vec wall(obstacle);
        if(foundfloor && wall.z)
        {
            wall.z = 0;
            if(!wall.iszero()) wall.normalize();
        }
        vec oldvel(d->vel);
        oldvel.add(d->falling);
        d->vel.project(wall);
        d->falling.project(wall);
        recalcdir(d, oldvel, dir);
    }

    void switchfloor(physent *d, vec &dir, const vec &floor)
    {
        if(floor.z >= floorz(d)) d->falling = vec(0, 0, 0);

        vec oldvel(d->vel);
        oldvel.add(d->falling);
        if(dir.dot(floor) >= 0)
        {
            if(d->physstate < PHYS_SLIDE || fabs(dir.dot(d->floor)) > 0.01f*dir.magnitude()) return;
            d->vel.projectxy(floor, 0.0f);
        }
        else d->vel.projectxy(floor);
        d->falling.project(floor);
        recalcdir(d, oldvel, dir);
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
            if(floor.z >= wallz(d)) switchfloor(d, dir, floor);
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
        else if(d->physstate == PHYS_STEP_UP || d->physstate == PHYS_SLIDE)
        {
            if(!collide(d, vec(0, 0, -1)) && wall.z > 0.0f)
            {
                floor = wall;
                if(floor.z >= slopez(d)) found = true;
            }
        }
        else if(d->physstate >= PHYS_SLOPE && d->floor.z < 1.0f)
        {
            d->o.z -= d->radius;
            if(!collide(d, vec(d->floor).neg(), 0.95f) || !collide(d, vec(0, 0, -1)))
            {
                floor = wall;
                if(floor.z >= slopez(d) && floor.z < 1.0f) found = true;
            }
        }
        if(collided && (!found || obstacle.z > floor.z))
        {
            floor = obstacle;
            slide = !found && (floor.z < wallz(d) || floor.z >= slopez(d));
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
		bool collided = false, slidecollide = false;
		vec obstacle;
		d->o.add(dir);
		if(!collide(d, d->type!=ENT_CAMERA ? dir : vec(0, 0, 0)) || (d->type==ENT_AI && !collide(d)))
		{
            obstacle = wall;
            /* check to see if there is an obstacle that would prevent this one from being used as a floor */
            if(d->type==ENT_PLAYER && ((wall.z>=slopez(d) && dir.z<0) || (wall.z<=-slopez(d) && dir.z>0)) && (dir.x || dir.y) && !collide(d, vec(dir.x, dir.y, 0)))
            {
                if(wall.dot(dir) >= 0) slidecollide = true;
                obstacle = wall;
            }

            d->o = old;
            if(d->type == ENT_CAMERA) return false;
            float stepdist = (d->physstate >= PHYS_SLOPE && d->floor.z < 1.0f ? d->radius+0.1f : stairheight(d));
            d->o.z -= stepdist;
            d->zmargin = -stepdist;
            if(d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR || (!collide(d, vec(0, 0, -1), slopez(d)) && (d->physstate==PHYS_STEP_UP || wall.z>=floorz(d))))
            {
                d->o = old;
                d->zmargin = 0;
                float floorz = (d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR ? d->floor.z : wall.z);
                if(trystepup(d, dir, floorz < 1.0f ? d->radius+0.1f : stairheight(d))) return true;
            }
            else
            {
                d->o = old;
                d->zmargin = 0;
            }

			/* can't step over the obstacle, so just slide against it */
			collided = true;
		}
		vec floor(0, 0, 0);
		bool slide = collided,
			 found = findfloor(d, collided, obstacle, slide, floor);
        if(slide || (!collided && floor.z > 0 && floor.z < wallz(d)))
        {
            slideagainst(d, dir, slide ? obstacle : floor, found || slidecollide);
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

	void modifyvelocity(physent *pl, bool local, bool floating, int millis)
	{
		if(floating)
		{
			if(pl->jumping)
			{
				pl->jumping = false;
				pl->vel.z = jumpvelocity(pl);
			}
		}
		else if(pl->physstate >= PHYS_SLOPE || pl->inliquid)
		{
			if(pl->jumping)
			{
				pl->jumping = false;
				pl->vel.z = jumpvelocity(pl);
				if(pl->inliquid) { pl->vel.x /= liquiddampen(pl); pl->vel.y /= liquiddampen(pl); }
				playsound(S_JUMP, 0, 255, pl->o, pl);
			}
		}
        if(pl->physstate == PHYS_FALL) pl->timeinair += curtime;

		vec m(0.0f, 0.0f, 0.0f);
        bool wantsmove = cl.allowmove(pl) && (pl->move || pl->strafe);
		if(m.iszero() && wantsmove)
		{
			vecfromyawpitch(pl->aimyaw, floating || pl->inliquid || movepitch(pl) ? pl->aimpitch : 0, pl->move, pl->strafe, m);

			if(!floating && pl->physstate >= PHYS_SLIDE)
			{
				/* move up or down slopes in air
				 * but only move up slopes in liquid
				 */
				float dz = -(m.x*pl->floor.x + m.y*pl->floor.y)/pl->floor.z;
				if(pl->inliquid) m.z = max(m.z, dz);
				else if(pl->floor.z >= wallz(pl)) m.z = dz;
			}

			m.normalize();
		}

		vec d(m);
		d.mul(maxspeed(pl));
		if(floating) { if(local) d.mul(floatspeed()/100.0f); }
		else if(!pl->inliquid) d.mul((wantsmove ? 1.3f : 1.0f) * (pl->physstate < PHYS_SLOPE ? 1.3f : 1.0f)); // EXPERIMENTAL
		float friction = pl->inliquid && !floating ? liquidfric(pl) : (pl->physstate >= PHYS_SLOPE || floating ? floorfric(pl) : airfric(pl));
		float fpsfric = friction/millis*20.0f;

        pl->vel.mul(fpsfric-1);
        pl->vel.add(d);
        pl->vel.div(fpsfric);
	}

    void modifygravity(physent *pl, int curtime)
    {
        float secs = curtime/1000.0f;
        vec g(0, 0, 0);
        if(pl->physstate == PHYS_FALL) g.z -= gravityforce(pl)*secs;
        else if(pl->floor.z > 0 && pl->floor.z < floorz(pl))
        {
            g.z = -1;
            g.project(pl->floor);
            g.normalize();
            g.mul(gravityforce(pl)*secs);
        }
        if(!pl->inliquid || (!pl->move && !pl->strafe)) pl->falling.add(g);

        if(pl->inliquid || pl->physstate >= PHYS_SLOPE)
        {
            float friction = pl->inliquid ? sinkfric(pl) : floorfric(pl),
                  fpsfric = friction/curtime*20.0f,
                  c = pl->inliquid ? 1.0f : clamp((pl->floor.z - slopez(pl))/(floorz(pl)-slopez(pl)), 0.0f, 1.0f);
            pl->falling.mul(1 - c/fpsfric);
        }
    }

    void updatematerial(physent *pl, bool local, bool floating)
    {
		vec v(cl.feetpos(pl, 1.f));
		int material = lookupmaterial(v);
		if(pl->state == CS_ALIVE && material != pl->inmaterial)
		{
			if(isliquid(material&MATF_VOLUME) || isliquid(pl->inmaterial&MATF_VOLUME))
			{
				uchar col[3] = { 255, 255, 255 };
				#define mattrig(mf,mz,mw) \
				{ \
					mf; \
					int icol = (col[2] + (col[1] << 8) + (col[0] << 16)); \
					regularshape(mz, int(pl->height), icol, 21, rnd(20)+1, 100, v, mz == 4 ? 4.f : 1.f); \
					if(mw>=0) playsound(mw, 0, 255, pl->o, pl); \
				}

				if(int(material&MATF_VOLUME) == MAT_WATER || int(pl->inmaterial&MATF_VOLUME) == MAT_WATER)
				{
					mattrig(getwatercolour(col), 6, int(material&MATF_VOLUME) != MAT_WATER ? S_SPLASH1 : S_SPLASH2);
				}

				if(int(material&MATF_VOLUME) == MAT_LAVA || int(pl->inmaterial&MATF_VOLUME) == MAT_LAVA)
				{
					mattrig(getlavacolour(col), 4, int(material&MATF_VOLUME) != MAT_LAVA ? -1 : S_FLBURNING);
				}
			}

			if(local && pl->type == ENT_PLAYER && (isdeadly(material&MATF_VOLUME)))
				cl.suicide((fpsent *)pl, int(material&MATF_VOLUME) == MAT_LAVA ? HIT_MELT : 0);
		}

		pl->inmaterial = material;

		v = vec(pl->o.x, pl->o.y, pl->o.z + (3*pl->aboveeye - pl->height)/4);
		bool liquid = pl->inliquid;
		pl->inliquid = !floating && isliquid(lookupmaterial(v)&MATF_VOLUME);
		if(!floating && pl->inliquid && liquid != pl->inliquid)
			pl->vel.div(liquiddampen(pl));
    }

	// main physics routine, moves a player/monster for a time step
	// moveres indicated the physics precision (which is lower for monsters and multiplayer prediction)
	// local is false for multiplayer prediction

	bool moveplayer(physent *pl, int moveres, bool local, int millis)
	{
		bool floating = pl->state==CS_EDITING || pl->state==CS_SPECTATOR;
		float secs = millis/1000.f;

		if(pl->type!=ENT_CAMERA) updatematerial(pl, local, floating);

        // apply gravity
        if(!floating && pl->type!=ENT_CAMERA) modifygravity(pl, millis);
		// apply any player generated changes in velocity
		modifyvelocity(pl, local, floating, millis);

		vec d(pl->vel), oldpos(pl->o);
        if(!floating && pl->type!=ENT_CAMERA && pl->inliquid) d.mul(0.5f);
        d.add(pl->falling);
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
                pl->falling = vec(0, 0, 0);
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
				playsound(S_LAND, 0, 255, pl->o, pl);
			}
		}

		if(pl->type!=ENT_CAMERA && pl->state==CS_ALIVE) updatedynentcache(pl);

		if(!pl->timeinair && pl->physstate >= PHYS_FLOOR && pl->vel.squaredlen() < 1e-4f)
			pl->moving = false;

		pl->lastmoveattempt = lastmillis;
		if(pl->o!=oldpos) pl->lastmove = lastmillis;
		return true;
	}

	bool move(physent *d, int moveres = 10, bool local = true, int millis = 0, int repeat = 0)
	{
		if(!millis) millis = physframetime();
		if(!repeat) repeat = physicsrepeat;

		loopi(repeat)
		{
			if(!moveplayer(d, moveres, local, millis)) return false;
			if(local && d->o.z < 0 && d->state == CS_ALIVE)
			{
				cl.suicide((fpsent *)d, HIT_FALL);
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
        conoutf("\frcan't find entity spawn spot! (%.1f, %.1f, %.1f)", d->o.x, d->o.y, d->o.z);
		// leave ent at original pos, possibly stuck
		d->o = orig;
		return false;
	}

    IVARP(smoothmove, -1, -1, 1000);
    IVARP(smoothdist, 0, 64, 1024);

	void smoothplayer(fpsent *d, int res, bool local)
	{
		if(d->state==CS_ALIVE || d->state==CS_EDITING)
		{
			if(smoothmove() && d->smoothmillis>0)
			{
				d->o = d->newpos;

				d->yaw = d->newyaw;
				d->pitch = d->newpitch;

				d->aimyaw = d->newaimyaw;
				d->aimpitch = d->newaimpitch;

				move(d, res, local);

				d->newpos = d->o;

				int s = smoothmove() > 0 ? smoothmove() : cl.player1->ping;
				float k = 1.0f - float(lastmillis - d->smoothmillis)/float(s);
				if(k>0)
				{
					d->o.add(vec(d->deltapos).mul(k));

					d->yaw += d->deltayaw*k;
					if(d->yaw<0) d->yaw += 360;
					else if(d->yaw>=360) d->yaw -= 360;
					d->pitch += d->deltapitch*k;

					d->aimyaw += d->deltaaimyaw*k;
					if(d->aimyaw<0) d->aimyaw += 360;
					else if(d->aimyaw>=360) d->aimyaw -= 360;
					d->aimpitch += d->deltaaimpitch*k;
				}
			}
			else move(d, res, local);
		}
		else if(d->state==CS_DEAD && lastmillis-d->lastpain<2000) move(d, res, local);
	}

	bool droptofloor(vec &o, float radius, float height)
	{
		if(!insideworld(o)) return false;
		vec v(0.0001f, 0.0001f, -1);
		v.normalize();
		if(raycube(o, v, hdr.worldsize) >= hdr.worldsize) return false;
		physent d;
		d.type = ENT_CAMERA;
		d.o = o;
		d.vel = vec(0, 0, -1);
		d.radius = radius;
		d.height = height;
		d.aboveeye = radius;
		loopi(hdr.worldsize) if(!move(&d, v))
		{
			o = d.o;
			return true;
		}
		return false;
	}

	void update()		  // optimally schedule physics frames inside the graphics frames
	{
	    int faketime = curtime+physicsfraction;
		physicsrepeat = faketime/physframetime();
		physicsfraction = faketime%physframetime();
		cleardynentcache();
	}
} ph;

