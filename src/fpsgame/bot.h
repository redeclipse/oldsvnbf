struct botclient
{
	GAMECLIENT &cl;

	static const int BOTISNEAR		= 64;			// is near
	static const int BOTISFAR		= 128;			// too far
	static const int BOTJUMPHEIGHT	= 8;			// decides to jump
	static const int BOTJUMPIMPULSE	= 16;			// impulse to jump
	static const int BOTJUMPMAX		= 32;			// too high
	static const int BOTLOSRANGE	= 514;			// line of sight range
	static const int BOTFOVRANGE	= 128;			// field of view range

	IVAR(botskill, 1, 30, 100);
	IVAR(botdebug, 0, 2, 4);

	#define BOTRATE					(101-botskill())
	#define BOTLOSDIST				(BOTLOSRANGE-(BOTRATE*2.0f))
	#define BOTFOVX					(BOTFOVRANGE-(BOTRATE*0.5f))
	#define BOTFOVY					((BOTFOVRANGE*3/4)-(BOTRATE*0.5f))
	#define BOTAFFINITY				(BOTRATE/1000.f)
	#define BOTWAIT(x,y)			(clamp(rnd(x*BOTRATE)+rnd(y*BOTRATE), y*BOTRATE, (x+y)*BOTRATE))

	botclient(GAMECLIENT &_cl) : cl(_cl)
	{
		CCOMMAND(addbot, "", (botclient *self), self->addbot());
		CCOMMAND(delbot, "", (botclient *self), self->delbot());
	}

	void addbot()
	{
		cl.cc.addmsg(SV_ADDBOT, "r");
	}

	void delbot()
	{
		loopvrev(cl.players) if(cl.players[i])
		{
			fpsent *d = cl.players[i];
			if (d->bot)
			{
				cl.cc.addmsg(SV_DELBOT, "ri", d->clientnum);
				break;
			}
		}
	}

	void update()
	{
		loopv(cl.players) if(cl.players[i])
		{
			fpsent *d = cl.players[i];
			if(d->bot) think(d);
		}
	}

	bool getsight(vec &o, float yaw, float pitch, vec &q, vec &v, float mdist, float fx, float fy)
	{
		float dist = o.dist(q);

		if(dist <= mdist)
		{
			float fovx = fx, fovy = fy;

			if (fovx <= 0.f) fovx = (float)fov;
			if (fovy <= 0.f) fovy = (float)fov*4/3;

			float x = fabs((asin((q.z-o.z)/dist)/RAD)-pitch);
			float y = fabs((-(float)atan2(q.x-o.x, q.y-o.y)/PI*180+180)-yaw);

			if(x <= fovx && y <= fovy)
				return raycubelos(o, q, v);
		}
		return false;
	}

	bool hastarget(fpsent *d)
	{
		float cyaw = fabs(fabs(d->bot->targyaw)-fabs(d->yaw)),
			cpitch = fabs(fabs(d->bot->targpitch)-fabs(d->pitch)),
			amt = BOTRATE*0.25f;
		if (cyaw < amt && cpitch < amt) return true;
		return false;
	}

	float disttonode(fpsent *d, int node, vector<int> &route, int flags = 0)
	{
		if(cl.et.ents.inrange(node))
		{
			vector<int> avoid;
			return cl.et.linkroute(d->lastnode, node, route, avoid, flags);
		}
		return 1e16f;
	}

	void doreset(fpsent *d, bool full = true)
	{
		if(full)
		{
			d->stopmoving();
			d->bot->reset();
		}
		else d->bot->removestate();
	}

	bool dodefer(fpsent *d, botstate &b, bool pursue = false)
	{
		if(d->canshoot(d->gunselect, lastmillis))
		{
			fpsent *targpl = NULL;
			int target = -1;
			float dist = 1e16f;
			vec targpos, targ;

			if(cl.player1->state == CS_ALIVE && (!m_team(cl.gamemode, cl.mutators) || d->team != cl.player1->team) && cl.player1->o.dist(d->o) < dist && getsight(d->o, d->yaw, d->pitch, cl.player1->o, targ, BOTLOSDIST, BOTFOVX, BOTFOVY))
			{
				target = cl.player1->clientnum;
				targpos = cl.player1->o;
				targpl = cl.player1;
			}
			loopv(cl.players)
			{
				if(cl.players[i] && cl.players[i] != d && cl.players[i]->state == CS_ALIVE && (!m_team(cl.gamemode, cl.mutators) || d->team != cl.players[i]->team) && cl.players[i]->o.dist(d->o) < dist && getsight(d->o, d->yaw, d->pitch, cl.players[i]->o, targ, BOTLOSDIST, BOTFOVX, BOTFOVY))
				{
					target = cl.players[i]->clientnum;
					targpos = cl.players[i]->o;
					targpl = cl.players[i];
				}
			}

			if(target >= 0)
			{
				botstate &c = d->bot->addstate(pursue ? BS_PURSUE : BS_ATTACK);
				c.waittime = BOTWAIT(3, 2);
				c.airtime = BOTWAIT(1, 1);
				c.cycles = 100;
				c.targpos = targpos;
				c.targtype = BTRG_PLAYER;
				c.target = target;
				if(pursue)
				{
					c.dist = disttonode(d, targpl->lastnode, c.route, ROUTE_ABS|ROUTE_GTONE);
					if(!c.route.length())
					{
						d->bot->removestate();
						return dodefer(d, b, false);
					}
				}
				else
				{
					c.route = b.route;
					c.dist = b.dist;
				}
				return true;
			}
		}
		return false;
	}

	int dohunt(fpsent *d, botstate &b, bool replan)
	{
		int node = b.route.find(d->lastnode);
		if(b.route.inrange(node))
		{
			node++;
			if(b.route.inrange(node))
			{
				botstate &c = d->bot->addstate(BS_MOVE);
				b.targpos = c.targpos = vec(cl.et.ents[b.route[node]]->o).add(vec(0, 0, d->height));
				c.targtype = BTRG_NODE;
				c.target = b.route[node];
				c.route = b.route;
				c.dist = b.dist;
				return 2;
			}
			return 1;
		}
		else if(replan)
		{
			int goal = b.goal();
			if(goal != d->lastnode)
			{
				b.dist = disttonode(d, goal, b.route);
				if(b.route.length()) return dohunt(d, b, false);
			}
		}
		return 0;
	}

	void dowait(fpsent *d, botstate &b)
	{
		d->stopmoving();

		if(d->state == CS_DEAD)
		{
			if(d->respawned != d->lifesequence && !cl.respawnwait(d))
				cl.respawnself(d);
		}
		else if(d->state == CS_ALIVE)
		{
			botstate &c = d->bot->addstate(BS_UPDATE);
			c.waittime = BOTWAIT(10, 10);
			c.airtime = BOTWAIT(5, 5);
			d->bot->targyaw = d->yaw;
			d->bot->targpitch = d->pitch;
			vec dir;
			vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
			dir.mul(d->radius*2);
			dir.add(d->o);
			c.targpos = dir;
		}
	}

	bool doupdate(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			int state, node;
			vector<int> ignore[BTRG_MAX];
			loopi(BTRG_MAX) ignore[i].setsize(0);
			d->stopmoving();

			// use these as they aren't being used
			#define resetupdatestate \
			{ \
				state = BS_WAIT; \
				b.targtype = b.target = node = -1; \
				b.targpos = d->o; \
				b.dist = 1e16f; \
			}

			#define getclosestnode(st, tt, nd, tr, ps, dm) \
			{ \
				vec targ; \
				float pdist = d->o.dist(ps); \
				if(dm > 0.f) pdist *= dm; \
				if(getsight(d->o, d->yaw, d->pitch, ps, targ, BOTLOSDIST, BOTFOVX, BOTFOVY)) \
					pdist *= BOTAFFINITY; \
				if(pdist < b.dist) \
				{ \
					state = st; \
					node = nd; \
					b.targtype = tt; \
					b.target = tr; \
					b.targpos = ps; \
					b.dist = pdist; \
				} \
			}

			loopi(10)
			{ // try hard to get a route, story of my life :P
				resetupdatestate;

				if(m_ctf(cl.gamemode)) loopvj(cl.ctf.flags) if(!cl.ctf.flags[j].owner && ignore[BTRG_FLAG].find(j) < 0)
				{
					ctfstate::flag &f = cl.ctf.flags[j];
					int n = cl.et.waypointnode(f.pos(), false);

					if(cl.et.ents.inrange(n) && ignore[BTRG_NODE].find(n) < 0)
					{
						int defending = 0;
						#define getdefendingplayer(pl) \
						{ \
							if(pl && pl != d && pl->state == CS_ALIVE && pl->team == d->team && pl->o.dist(f.pos()) <= enttype[FLAG].radius*2.f) \
								defending++; \
						}

						getdefendingplayer(cl.player1);
						loopvk(cl.players) getdefendingplayer(cl.players[k]);

						if(!defending)
						{
							if(f.team == d->team)
							{
								getclosestnode(BS_DEFEND, BTRG_FLAG, n, j, vec(f.pos()).add(vec(0, 0, d->height)), BOTAFFINITY);
							}
							else
							{
								getclosestnode(BS_PURSUE, BTRG_FLAG, n, j, vec(f.pos()).add(vec(0, 0, d->height)), BOTAFFINITY);
							}
						}
					}
					else if(botdebug() >= 3) conoutf("ignoring flag %d (node %d)", j, n);
				}

				loopvj(cl.et.ents)
				{
					fpsentity &e = (fpsentity &)*cl.et.ents[j];
					switch(e.type)
					{
						case WEAPON:
						{
							if(e.spawned && e.attr1 != GUN_GL && d->ammo[e.attr1] <= 0 && e.attr1 > d->bestgun(lastmillis) && ignore[BTRG_ENTITY].find(j) < 0)
								getclosestnode(BS_INTEREST, BTRG_ENTITY, j, j, vec(e.o).add(vec(0, 0, d->height)), 1.f);
							break;
						}
						default: break;
					}
				}

				#define getinterestinplayer(pl) \
				{ \
					if(pl && pl != d && pl->state == CS_ALIVE && (!m_team(cl.gamemode, cl.mutators) || d->team != pl->team) && ignore[BTRG_NODE].find(pl->lastnode) < 0) \
					{ \
						float distmod = 1.f; \
						if(m_ctf(cl.gamemode)) loopvk(cl.ctf.flags) \
						{ \
							ctfstate::flag &f = cl.ctf.flags[k]; \
							if(f.team == d->team && f.owner && f.owner == pl) \
								distmod *= BOTAFFINITY; \
						} \
						getclosestnode(BS_PURSUE, BTRG_PLAYER, pl->lastnode, pl->clientnum, pl->o, distmod); \
					} \
				}


				getinterestinplayer(cl.player1);
				loopvj(cl.players) getinterestinplayer(cl.players[j]);

				if(!cl.et.ents.inrange(node))
				{ // we failed, go scout to the the other side of the map
					resetupdatestate;

					loopvj(cl.et.ents)
					{
						fpsentity &e = (fpsentity &)*cl.et.ents[j];
						if(e.type == WAYPOINT && e.o.dist(d->o) > b.dist && ignore[BTRG_NODE].find(j) < 0)
						{
							state = BS_INTEREST;
							b.targtype = BTRG_NODE;
							b.target = node = j;
							b.dist = e.o.dist(d->o);
							b.targpos = vec(e.o).add(vec(0, 0, d->height));
						}
					}
				}

				if(cl.et.ents.inrange(node))
				{
					b.dist = disttonode(d, node, b.route, ROUTE_ABS|ROUTE_GTONE);

					if(b.route.length())
					{
						botstate &c = d->bot->addstate(state);
						c.waittime = BOTWAIT(2, 1);
						c.airtime = BOTWAIT(1, 1);
						c.cycles = 100;
						c.targtype = b.targtype;
						c.target = b.target;
						c.targpos = b.targpos;
						c.dist = b.dist;
						c.route = b.route;
						return true;
					}
					else
					{ // keep going then
						if(botdebug() >= 4)
							conoutf("%s failed to home in on %d (%d), ignoring", cl.colorname(d), node, b.target);
						ignore[BTRG_NODE].add(node);
						if(b.targtype > 0 && ignore[b.targtype].find(b.target) < 0)
							ignore[b.targtype].add(b.target);
					}
				}
				else
				{
					if(botdebug() >= 3)
						conoutf("%s failed to get any sort of node", cl.colorname(d));
					break;
				}
			}
			if(botdebug() >= 3)
				conoutf("%s failed to get a route", cl.colorname(d));
		}
		return false;
	}

	bool domove(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			d->stopmoving();
			vec off(vec(b.targpos).sub(d->o));

			if(off.z > BOTJUMPHEIGHT && !d->timeinair)
				d->jumping = true;

			d->move = 1;
			d->bot->removestate(); // bounce back to the parent
			return true;
		}
		return false;
	}

	bool dodefend(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			switch(b.targtype)
			{
				case BTRG_FLAG:
				{
					if(m_ctf(cl.gamemode) && cl.ctf.flags.inrange(b.target))
					{
						if(dodefer(d, b, false)) return true;
						switch(dohunt(d, b, true))
						{
							case 2: return true; break;
							case 1:
							{
								d->stopmoving();
								d->move = rnd(2)-1;
								d->strafe = rnd(3)-1;
							}
							case 0: default: break;
						}
					}
					break;
				}
			}
		}
		return false;
	}

	bool doattack(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			d->stopmoving();

			if(hastarget(d))
			{
				d->attacking = true;
				d->attacktime = lastmillis;
				d->bot->removestate(); // bounce back to the parent
			}
			return true;
		}
		return false;
	}

	bool dointerest(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			if(cl.et.ents.inrange(b.target))
			{
				if(dodefer(d, b, true)) return true;

				fpsentity &e = (fpsentity &)*cl.et.ents[b.target];
				float eye = d->height*0.5f;
				vec m = d->o;
				m.z -= eye;

				switch(e.type)
				{
					case WEAPON:
					{
						if(!e.spawned || d->ammo[e.attr1] > 0 || e.attr1 <= d->bestgun(lastmillis))
							return false;
						break;
					}
					default: break;
				}

				if(insidesphere(m, eye, d->radius, e.o, enttype[e.type].height, enttype[e.type].radius))
				{
					if(enttype[e.type].usetype == ETU_ITEM && d->canuse(e.type, e.attr1, e.attr2, lastmillis))
					{
						d->useaction = true;
						d->usetime = lastmillis;
					}
					d->bot->removestate();
					return true;
				}

				if(dohunt(d, b, true) == 2) return true;
			}
		}
		return false;
	}

	bool doctfhomerun(fpsent *d, botstate &b)
	{
		loopk(2)
		{
			int goal = -1;
			loopv(cl.ctf.flags)
			{
				ctfstate::flag &g = cl.ctf.flags[i];
				if(g.team == d->team && (k || (!g.owner && !g.droptime)) &&
					(!cl.ctf.flags.inrange(goal) || g.pos().dist(d->o) < cl.ctf.flags[goal].spawnloc.dist(d->o)))
				{
					int node = cl.et.waypointnode(g.pos(), false);
					if(!cl.et.ents.inrange(node)) node = cl.et.waypointnode(g.pos(), false);
					if(cl.et.ents.inrange(node)) goal = node;
				}
			}

			if(cl.et.ents.inrange(goal))
			{
				botstate &c = d->bot->setstate(BS_PURSUE); // replaces!
				c.waittime = BOTWAIT(2, 1);
				c.airtime = BOTWAIT(1, 1);
				c.targpos = vec(cl.et.ents[goal]->o).add(vec(0, 0, d->height));
				c.dist = disttonode(d, goal, c.route, ROUTE_ABS|ROUTE_GTONE);
				c.targtype = BTRG_NODE;
				c.target = goal;
				return true;
			}
		}
		return false;
	}

	bool dopursue(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			switch(b.targtype)
			{
				case BTRG_FLAG:
				{
					if(m_ctf(cl.gamemode) && cl.ctf.flags.inrange(b.target))
					{
						ctfstate::flag &f = cl.ctf.flags[b.target];
						if(f.team == d->team)
						{
							vector<int> hasflags;
							hasflags.setsize(0);
							loopv(cl.ctf.flags)
							{
								ctfstate::flag &g = cl.ctf.flags[i];
								if(g.team != d->team && g.owner == d)
									hasflags.add(i);
							}

							if(!hasflags.length())
								return false; // otherwise why are we pursuing home?

							if(dodefer(d, b, false)) return true;
							if(dohunt(d, b, true) == 2) return true;
						}
						else
						{
							if(f.owner == d) return doctfhomerun(d, b);
							if(dodefer(d, b, false)) return true;
							if(dohunt(d, b, true) == 2) return true;
						}
					}
					break;
				}

				case BTRG_PLAYER:
				{
					if(dodefer(d, b, false)) return true;

					fpsent *e = cl.getclient(b.target);

					if(e && e->state == CS_ALIVE)
					{ // if we're here, we haven't been able to shoot
						vec target;
						if(e->o.dist(d->o) < BOTISNEAR && cl.et.ents.inrange(d->lastnode) &&
							getsight(d->o, d->yaw, d->pitch, e->o, target, BOTLOSDIST, BOTFOVX, BOTFOVY))
						{
							#if 0
							int node = -1;
							float dist = 0.f;
							loopv(cl.et.ents[d->lastnode]->links)
							{ // find a random node to go to, we're too close
								if(cl.et.ents.inrange(cl.et.ents[d->lastnode]->links[i]))
								{
									fpsentity &f = (fpsentity &)*cl.et.ents[cl.et.ents[d->lastnode]->links[i]];
									if(f.type == WAYPOINT && f.o.dist(e->o) > dist)
									{ // move away from our target for a bit
										node = cl.et.ents[d->lastnode]->links[i];
										dist = f.o.dist(e->o);
									}
								}
							}

							if(cl.et.ents.inrange(node))
							{
								botstate &c = d->bot->addstate(BS_MOVE);
								b.targpos = c.targpos = vec(cl.et.ents[node]->o).add(vec(0, 0, d->height));
								c.targtype = BTRG_NODE;
								c.target = node;
								c.route = b.route;
								c.dist = dist;
								return true;
							}
							#else
							d->stopmoving();
							d->move = rnd(2)-1;
							d->strafe = rnd(3)-1;
							return true;
							#endif
						}

						if(dohunt(d, b, true) == 2) return true;
					}
					break;
				}
				default: break;
			}
		}
		return false;
	}

	void doaim(fpsent *d, botstate &b)
	{
		float amt = float(lastmillis-d->lastupdate)/float(BOTRATE);

		d->bot->targyaw = -(float)atan2(b.targpos.x-d->o.x, b.targpos.y-d->o.y)/PI*180+180;

		if (d->yaw < d->bot->targyaw-180.0f) d->yaw += 360.0f;
		if (d->yaw > d->bot->targyaw+180.0f) d->yaw -= 360.0f;

		float dist = d->o.dist(b.targpos);
		d->bot->targpitch = asin((b.targpos.z-d->o.z)/dist)/RAD;

		if (d->bot->targpitch > 90.f) d->bot->targpitch = 90.f;
		if (d->bot->targpitch < -90.f) d->bot->targpitch = -90.f;

		float offyaw = fabs(d->bot->targyaw-d->yaw), offpitch = fabs(d->bot->targpitch-d->pitch);

		if (d->bot->targyaw > d->yaw) // slowly turn bot towards target
		{
			d->yaw += amt*offyaw;
			if (d->bot->targyaw < d->yaw) d->yaw = d->bot->targyaw;
		}
		else if (d->bot->targyaw < d->yaw)
		{
			d->yaw -= amt*offyaw;
			if (d->bot->targyaw > d->yaw) d->yaw = d->bot->targyaw;
		}
		if (d->bot->targpitch > d->pitch)
		{
			d->pitch += amt*offpitch;
			if (d->bot->targpitch < d->pitch) d->pitch = d->bot->targpitch;
		}
		else if (d->bot->targpitch < d->pitch)
		{
			d->pitch -= amt*offpitch;
			if (d->bot->targpitch > d->pitch) d->pitch = d->bot->targpitch;
		}

		cl.fixrange(d->yaw, d->pitch);
		findorientation(d->o, d->yaw, d->pitch, d->bot->targpos);
		d->aimyaw = d->yaw;
		d->aimpitch = d->pitch;
	}

	void think(fpsent *d)
	{
		if(!d->bot->state.length()) doreset(d, true);

		// the state stack works like a chain of commands, certain commands simply replace
		// each other, others spawn new commands to the stack the ai reads the top command
		// from the stack and executes it or pops the stack and goes back along the history
		// until it finds a suitable command to execute. states can have wait times and a
		// limit can be specified on the number cycles it may try for until removed.

		botstate &b = d->bot->getstate();
		int secs = lastmillis - b.millis;

		b.cycle++;

		if(d->state == CS_ALIVE && b.type != BS_WAIT)
		{
			cl.et.checkitems(d);
			cl.ws.shoot(d, d->bot->targpos);
			if(d->gunselect != d->bestgun(lastmillis) && d->canswitch(d->bestgun(lastmillis), lastmillis))
			{
				d->setgun(d->bestgun(lastmillis), lastmillis);
				cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
				cl.playsoundc(S_SWITCH, d);
			}
			else if(d->ammo[d->gunselect] <= 0 && d->canreload(d->gunselect, lastmillis))
			{
				d->gunreload(d->gunselect, guntype[d->gunselect].add, lastmillis);
				cl.cc.addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
				cl.playsoundc(S_RELOAD, d);
			}
		}

		if(d->state == CS_ALIVE && b.type != BS_WAIT)
			doaim(d, b);

		if(b.cycles && b.cycle >= b.cycles) doreset(d, false);
		else if(secs >= (d->timeinair ? b.airtime : b.waittime))
		{
			bool result = false;
			switch(b.type)
			{
				case BS_WAIT:		result = true; dowait(d, b);	break;
				case BS_UPDATE:		result = doupdate(d, b);		break;
				case BS_MOVE:		result = domove(d, b);			break;
				case BS_DEFEND:		result = dodefend(d, b);		break;
				case BS_PURSUE:		result = dopursue(d, b);		break;
				case BS_ATTACK:		result = doattack(d, b);		break;
				case BS_INTEREST:	result = dointerest(d, b);		break;
				default: break;
			}
			if(!result) doreset(d, false);
			else b.millis = lastmillis;
		}
		cl.ph.move(d, 10, true);
		d->lastupdate = lastmillis;
	}

	void drawstate(fpsent *d, botstate &b, bool top, int above)
	{
		const char *bnames[BS_MAX] = {
			"wait", "update", "move", "defend", "pursue", "attack", "interest"
		}, *btypes[BTRG_MAX+1] = {
			"none", "node", "player", "entity", "flag"
		};
		s_sprintfd(s)("@%s%s (%.2fs) [%d/%d] goal:%d[%d] %s:%d",
			top ? "\fy" : "\fw",
			bnames[b.type],
			max((b.millis+b.waittime-lastmillis)/1000.f, 0.f),
			b.cycle, b.cycles,
			b.goal(), b.route.length(),
			btypes[b.targtype+1], b.target
		);
		particle_text(vec(d->abovehead()).add(vec(0, 0, above)), s, 14, 1);
	}

	void drawroute(fpsent *d, botstate &b)
	{
		renderprimitive(true);
		int last = -1;
		loopv(b.route)
		{
			if (b.route.inrange(last))
			{
				int index = b.route[i], prev = b.route[last];

				if (cl.et.ents.inrange(index) && cl.et.ents.inrange(prev))
				{
					fpsentity &e = (fpsentity &) *cl.et.ents[index], &f = (fpsentity &) *cl.et.ents[prev];
					renderline(vec(f.o).add(vec(0, 0, 1)), vec(e.o).add(vec(0, 0, 1)), 128.f, 64.f, 64.f, false);
				}
			}
			last = i;
		}
		if(botdebug() > 3)
			renderline(vec(d->o).sub(vec(0, 0, d->height*0.5f)), b.targpos, 64.f, 128.f, 64.f, false);
		renderprimitive(false);
	}

	void render()
	{
		if(botdebug())
		{
			loopv(cl.players) if(cl.players[i] && cl.players[i]->state == CS_ALIVE && cl.players[i]->bot)
			{
				fpsent *d = cl.players[i];
				bool top = true;
				int above = 0;
				loopvrev(d->bot->state)
				{
					botstate &b = d->bot->state[i];
					drawstate(d, b, top, above += 2);
					if(botdebug() > 2 && top && rendernormally && b.type != BS_WAIT)
						drawroute(d, b);
					if(top)
					{
						if(botdebug() > 1) top = false;
						else break;
					}
				}
			}
		}
	}
} bot;
