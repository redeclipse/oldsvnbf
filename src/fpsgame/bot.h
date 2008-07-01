struct botclient
{
	GAMECLIENT &cl;

	static const int BOTFRAMETIME		= 100;

	static const int BOTISNEAR			= 64;			// is near
	static const int BOTISFAR			= 128;			// too far
	static const int BOTJUMPHEIGHT		= 6;			// decides to jump
	static const int BOTJUMPIMPULSE		= 16;			// impulse to jump
	static const int BOTJUMPMAX			= 32;			// too high
	static const float BOTLOSMIN		= 64.f;			// minimum line of sight
	static const float BOTLOSMAX		= 1024.f;		// maximum line of sight
	static const float BOTFOVMIN		= 90.f;			// minimum field of view
	static const float BOTFOVMAX		= 125.f;		// maximum field of view

	IVAR(botskill, 1, 30, 100);
	IVAR(botstall, 0, 0, 1);
	IVAR(botdebug, 0, 2, 4);

	#define BOTSCALE(x)				clamp(101 - clamp(x, 1, 100), 1, 100)
	#define BOTERROR(x)				BOTSCALE((botskill()+x)/2)
	#define BOTRATE					BOTSCALE(botskill())
	#define BOTCHANCE				rnd(botskill())
	#define BOTAFFINITY				BOTRATE/1000.f

	#define BOTWAIT(x,y)			clamp(rnd(x*BOTRATE) + rnd(y*BOTRATE), y*BOTRATE, (x+y)*BOTRATE)

	#define BOTLOSDIST				clamp(BOTLOSMIN+((BOTLOSMAX-BOTLOSMIN)/float(BOTRATE)), BOTLOSMIN, float(getvar("fog"))+BOTLOSMIN)
	#define BOTFOVX					clamp(BOTFOVMIN+((BOTFOVMAX-BOTFOVMIN)/float(BOTRATE)), BOTFOVMIN, BOTFOVMAX)
	#define BOTFOVY					BOTFOVX*3/4

	botclient(GAMECLIENT &_cl) : cl(_cl)
	{
		CCOMMAND(addbot, "i", (botclient *self, int *n), self->addbot(*n));
		CCOMMAND(delbot, "i", (botclient *self, int *n), self->delbot(*n));
	}

	void addbot(int n)
	{
		loopi(n ? n : 1) cl.cc.addmsg(SV_ADDBOT, "r");
	}

	void delbot(int n)
	{
		int c = n ? n : 1;
		loopvrev(cl.players) if(cl.players[i])
		{
			fpsent *d = cl.players[i];
			if(d->bot)
			{
				cl.cc.addmsg(SV_DELBOT, "ri", d->clientnum);
				c--;
				if(!c) break;
			}
		}
	}

	void create(fpsent *d)
	{
		d->bot = new botinfo();
		if(!d->bot) fatal("could not create bot");
	}

	void update()
	{
		loopv(cl.players) if(cl.players[i])
		{
			fpsent *d = cl.players[i];
			if(d->bot) think(d);
		}
	}

	void reset(fpsent *d, bool full = true)
	{
		if(full) d->bot->reset();
		else d->bot->removestate();
	}

	bool getsight(vec &o, float yaw, float pitch, vec &q, vec &v, float mdist, float fx, float fy)
	{
		float dist = o.dist(q);

		if(dist <= mdist)
		{
			float fovx = fx, fovy = fy;
			if(fovx <= 0.f) fovx = (float)fov;
			if(fovy <= 0.f) fovy = (float)fov*4/3;
			float x = fabs((asin((q.z-o.z)/dist)/RAD)-pitch);
			float y = fabs((-(float)atan2(q.x-o.x, q.y-o.y)/PI*180+180)-yaw);
			if(x <= fovx && y <= fovy) return raycubelos(o, q, v);
		}
		return false;
	}

	bool hastarget(fpsent *d, botstate &b, vec &pos)
	{ // add margins of error
		vec dir(vec(vec(vec(d->o).sub(vec(0, 0, d->height))).sub(pos)).normalize());
		float targyaw, targpitch;
		vectoyawpitch(dir, targyaw, targpitch);
		float margin = float(BOTERROR(b.rate))/100.f,
			cyaw = fabs(targyaw-d->yaw), cpitch = fabs(targpitch-d->pitch);
		return cyaw < margin*BOTFOVX && cpitch < margin*BOTFOVY;
	}

	int checkothers(vector<int> &targets, fpsent *d = NULL, int state = -1, int targtype = -1, int target = -1, bool teams = false)
	{
		int others = 0;
		targets.setsize(0);
		loopv(cl.players) if(cl.players[i] && (!d || cl.players[i] != d) && cl.players[i]->bot && cl.players[i]->state == CS_ALIVE)
		{
			fpsent *e = cl.players[i];
			if(targets.find(e->clientnum) >= 0) continue;
			if(teams && m_team(cl.gamemode, cl.mutators) && d && d->team != e->team) continue;

			botstate &b = e->bot->getstate();
			if(state >= 0 && b.type != state) continue;
			if(target >= 0 && b.target != target) continue;
			if(targtype >=0 && b.targtype != targtype) continue;
			targets.add(e->clientnum);
			others++;
		}
		return others;
	}

	float disttonode(fpsent *d, int node, vector<int> &route, int flags = 0)
	{
		if(cl.et.ents.inrange(node))
		{
			return cl.et.linkroute(d->lastnode, node, route, d->bot->avoid, flags);
		}
		return 1e16f;
	}

	bool violence(fpsent *d, botstate &b, fpsent *e, bool pursue = false)
	{
		botstate &c = d->bot->addstate(pursue ? BS_PURSUE : BS_ATTACK);
		c.rate = BOTCHANCE;
		c.targpos = vec(e->o).sub(vec(0, 0, e->height));
		c.targtype = BTRG_PLAYER;
		c.target = e->clientnum;
		if(pursue)
		{
			c.dist = disttonode(d, e->lastnode, c.route);
			if(!c.route.length())
			{
				reset(d, false);
				return violence(d, b, e, false);
			}
			return true;
		}
		else
		{
			c.dist = b.dist;
			c.route = b.route;
			return true;
		}
		return false;
	}

	bool defer(fpsent *d, botstate &b, bool pursue = false)
	{
		if(d->canshoot(d->gunselect, lastmillis))
		{
			fpsent *t = NULL;
			vec targ;

			#define getplayersight(pl) \
			{ \
				if(pl && pl != d && pl->state == CS_ALIVE && \
					(!m_team(cl.gamemode, cl.mutators) || d->team != pl->team) && \
						(!t || pl->o.dist(d->o) < t->o.dist(d->o)) && \
							getsight(d->o, d->yaw, d->pitch, pl->o, targ, BOTLOSDIST, BOTFOVX, BOTFOVY)) \
								t = pl; \
			}

			getplayersight(cl.player1);
			loopv(cl.players) getplayersight(cl.players[i]);

			if(t) return violence(d, b, t, pursue);
		}
		return false;
	}

	bool ctfhomerun(fpsent *d, botstate &b)
	{
		loopk(2)
		{
			int goal = -1, node = -1;
			loopv(cl.ctf.flags)
			{
				ctfstate::flag &g = cl.ctf.flags[i];
				if(g.team == d->team && (k || (!g.owner && !g.droptime)) &&
					(!cl.ctf.flags.inrange(goal) || g.pos().dist(d->o) < cl.ctf.flags[goal].pos().dist(d->o)))
				{
					int n = cl.et.waypointnode(g.pos(), false);
					if(cl.et.ents.inrange(n))
					{
						goal = i;
						node = n;
					}
				}
			}

			if(cl.et.ents.inrange(node) && cl.ctf.flags.inrange(goal))
			{
				botstate &c = d->bot->setstate(BS_PURSUE); // replaces current state!
				c.targpos = cl.ctf.flags[goal].pos();
				c.dist = disttonode(d, node, c.route);

				if(cl.ctf.flags[goal].owner)
				{ // we resorted to a flag someone has, may as well fight 'em for it
					c.targtype = BTRG_PLAYER;
					c.target = cl.ctf.flags[goal].owner->clientnum;
				}
				else
				{
					c.targtype = BTRG_NODE;
					c.target = goal;
				}
				return true;
			}
		}
		return false;
	}


	void damaged(fpsent *d, fpsent *e, int gun, int flags, int damage, int millis, vec &dir)
	{
		if(d->bot)
		{
			botstate &b = d->bot->getstate();
			bool result = false, pursue = false;
			switch(b.type)
			{
				case BS_PURSUE:
					if(b.targtype == BTRG_PLAYER || (b.targtype == BTRG_FLAG && m_ctf(cl.gamemode) && cl.ctf.flags[b.target].team != d->team))
					{
						result = true;
						if(b.targtype != BTRG_PLAYER) pursue = true;
					}
					break;
				case BS_WAIT:
				case BS_INTEREST:
					result = true;
					pursue = true;
					break;
				case BS_ATTACK:
				case BS_DEFEND: default: break;
			}
			if(result) violence(d, b, e, pursue);
		}
		vector<int> targets;
		int others = checkothers(targets, d, BS_DEFEND, BTRG_PLAYER, d->clientnum, true);
		if(others)
		{
			fpsent *t;
			loopv(targets) if((t = cl.getclient(targets[i])) != NULL && t->bot)
			{
				botstate &c = t->bot->getstate();
				violence(t, c, e, true);
			}
		}
	}

	void killed(fpsent *d, fpsent *e, int gun, int flags, int damage)
	{
		if(d->bot) reset(d, true);
	}

	bool dowait(fpsent *d, botstate &b)
	{
		d->stopmoving();

		if(d->state == CS_DEAD)
		{
			if(d->respawned != d->lifesequence && !cl.respawnwait(d))
				cl.respawnself(d);

			return true;
		}
		else if(d->state == CS_ALIVE)
		{
			if(m_ctf(cl.gamemode))
			{
				vector<int> hasflags;
				hasflags.setsize(0);
				loopv(cl.ctf.flags)
				{
					ctfstate::flag &g = cl.ctf.flags[i];
					if(g.team != d->team && g.owner == d)
						hasflags.add(i);
				}

				if(hasflags.length() && ctfhomerun(d, b))
					return true;
			}

			int state, node;
			// use these as they aren't being used
			#define resetupdatestate \
			{ \
				state = BS_WAIT; \
				b.targtype = b.target = node = -1; \
				b.targpos = vec(d->o).sub(vec(0, 0, d->height)); \
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

				if(m_ctf(cl.gamemode)) loopvj(cl.ctf.flags)
				{
					ctfstate::flag &f = cl.ctf.flags[j];
					int n = cl.et.waypointnode(f.pos(), false);
					if(cl.et.ents.inrange(n) && d->bot->avoid.find(n) < 0)
					{
						vector<int> targets; // build a list of others who are interested in this
						int others = checkothers(targets, d, f.team == d->team ? BS_DEFEND : BS_PURSUE, BTRG_FLAG, j, true);

						#define getotherplayer(pl) \
						{ \
							if(pl && pl != d && !pl->bot && pl->state == CS_ALIVE && pl->team == d->team && \
								targets.find(pl->clientnum) < 0 && pl->o.dist(f.pos()) <= enttype[FLAG].radius*2.f) \
							{ \
								targets.add(pl->clientnum); \
								others++; \
							} \
						}

						getotherplayer(cl.player1);
						loopvk(cl.players) getotherplayer(cl.players[k]);

						if(f.team == d->team)
						{
							if(!others || f.owner) // should we guard the base?
								getclosestnode(BS_DEFEND, BTRG_FLAG, n, j, f.pos(), BOTAFFINITY);
						}
						else
						{
							if(others)
							{ // ok lets help by defending the attacker
								fpsent *t;
								loopvk(targets) if((t = cl.getclient(targets[i])) != NULL)
								{
									getclosestnode(BS_DEFEND, BTRG_PLAYER, t->lastnode, t->clientnum, t->o, BOTAFFINITY);
								}
							}
							else // attack the flag
								getclosestnode(BS_PURSUE, BTRG_FLAG, n, j, f.pos(), BOTAFFINITY);
						}
					}
				}

				loopvj(cl.et.ents)
				{
					fpsentity &e = (fpsentity &)*cl.et.ents[j];
					int n = cl.et.waypointnode(e.o, false);
					if(cl.et.ents.inrange(n) && d->bot->avoid.find(n) < 0)
					{
						switch(e.type)
						{
							case WEAPON:
							{
								if(e.spawned && isgun(e.attr1) && guntype[e.attr1].rdelay > 0 && d->ammo[e.attr1] <= 0 && e.attr1 > d->bestgun(lastmillis) && d->bot->avoid.find(j) < 0)
									getclosestnode(BS_INTEREST, BTRG_ENTITY, n, j, e.o, 1.f);
								break;
							}
							default: break;
						}
					}
				}

				#if 0 // use defer instead?
				#define getinterestinplayer(pl) \
				{ \
					if(pl && pl != d && pl->state == CS_ALIVE) \
					{ \
						int st = !m_team(cl.gamemode, cl.mutators) || d->team != pl->team ? BS_PURSUE : 0; \
						float distmod = 1.f; \
						if(m_ctf(cl.gamemode)) loopvk(cl.ctf.flags) \
						{ \
							ctfstate::flag &f = cl.ctf.flags[k]; \
							if(f.owner && f.owner == pl) \
							{ \
								if(!st && pl->team == d->team) st = BS_DEFEND; \
								distmod *= BOTAFFINITY; \
							} \
						} \
						if(st) getclosestnode(st, BTRG_PLAYER, pl->lastnode, pl->clientnum, pl->o, distmod); \
					} \
				}
				getinterestinplayer(cl.player1);
				loopvj(cl.players) getinterestinplayer(cl.players[j]);
				#endif

				if(!cl.et.ents.inrange(node))
				{ // we failed, go scout to the the other side of the map
					resetupdatestate;
					b.dist = 0.f;

					loopvj(cl.et.ents)
					{
						fpsentity &e = (fpsentity &)*cl.et.ents[j];
						if(e.type == WAYPOINT && e.o.dist(d->o) > b.dist && d->bot->avoid.find(j) < 0)
						{
							state = BS_INTEREST;
							b.targtype = BTRG_NODE;
							b.target = node = j;
							b.dist = e.o.dist(d->o);
							b.targpos = e.o;
						}
					}
				}

				if(cl.et.ents.inrange(node))
				{
					b.dist = disttonode(d, node, b.route, b.targtype != BTRG_PLAYER ? ROUTE_ABS : 0);

					if(b.route.length())
					{
						botstate &c = d->bot->addstate(state);
						c.targtype = b.targtype;
						c.target = b.target;
						c.targpos = b.targpos;
						c.dist = b.dist;
						c.route = b.route;
						return true;
					}
					else
					{ // keep going then
						if(d->bot->avoid.find(node) < 0)
							d->bot->avoid.add(node);
					}
				}
				else break;
			}
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
						defer(d, b, false);
						return true;
					}
					break;
				}
				case BTRG_PLAYER:
				{
					fpsent *e = cl.getclient(b.target);
					if(e && e->state == CS_ALIVE)
					{
						defer(d, b, false);
						return true;
					}
					break;
				}
				default: break;
			}
		}
		return false;
	}

	bool doattack(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			if(!BOTCHANCE || b.cycle >= 10 || hastarget(d, b, b.targpos))
			{
				d->attacking = true;
				d->attacktime = lastmillis;
				return false; // bounce back to the parent
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
				if(defer(d, b, true)) return true;

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
					return false;
				}
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

							return true;
						}
						else
						{
							if(f.owner != d) return ctfhomerun(d, b);
							defer(d, b, false);
							return true;
						}
					}
					break;
				}

				case BTRG_PLAYER:
				{
					fpsent *e = cl.getclient(b.target);
					if(e && e->state == CS_ALIVE)
					{
						defer(d, b, false);
						return true;
					}
					break;
				}
				default: break;
			}
		}
		return false;
	}

	bool hunt(fpsent *d, botstate &b, vec &pos, bool replan = true)
	{
		int node = b.route.find(d->lastnode);
		if(b.route.inrange(node))
		{
			//vec feet(vec(d->o).sub(vec(0, 0, d->height)));
			//do { node++; } while(b.route.inrange(node) &&
			//	cl.et.ents[b.route[node]]->o.dist(feet) <= enttype[WAYPOINT].radius);
			node++;
			if(b.route.inrange(node))
			{
				if(d->bot->avoid.find(b.route[node]) < 0)
				{
					pos = cl.et.ents[b.route[node]]->o;
					return true;
				} // replan
			}
			else return false;
		}
		if(replan)
		{
			int goal = b.goal();
			b.dist = disttonode(d, goal, b.route);
			if(b.route.length()) return hunt(d, b, pos, false);
		}
		return false;
	}

	void aim(fpsent *d, botstate &b, vec &pos, float &yaw, float &pitch, bool aiming = true)
	{
		vec feet(vec(d->o).sub(vec(0, 0, d->height)));
		float targyaw = -(float)atan2(pos.x-feet.x, pos.y-feet.y)/PI*180+180;

		if(yaw < targyaw-180.0f) yaw += 360.0f;
		if(yaw > targyaw+180.0f) yaw -= 360.0f;

		float dist = feet.dist(pos), targpitch = asin((pos.z-feet.z)/dist)/RAD;

		if(aiming)
		{
			float amt = float(lastmillis-d->lastupdate)/float(BOTRATE),
				offyaw = fabs(targyaw-yaw)*amt, offpitch = fabs(targpitch-pitch)*amt;

			if(targyaw > yaw) // slowly turn bot towards target
			{
				yaw += offyaw;
				if(targyaw < yaw) yaw = targyaw;
			}
			else if(targyaw < yaw)
			{
				yaw -= offyaw;
				if(targyaw > yaw) yaw = targyaw;
			}

			if(targpitch > pitch)
			{
				pitch += offpitch;
				if(targpitch < pitch) pitch = targpitch;
			}
			else if(targpitch < pitch)
			{
				pitch -= offpitch;
				if(targpitch > pitch) pitch = targpitch;
			}

			cl.fixrange(yaw, pitch);
		}
		else
		{
			yaw = targyaw;
			pitch = targpitch;
		}
	}

	void process(fpsent *d)
	{
		botstate &b = d->bot->getstate();
		if(d->state == CS_ALIVE)
		{
			cl.et.checkitems(d);
			cl.ws.shoot(d, d->bot->target);

			if(b.type != BS_WAIT)
			{
				int bestgun = d->bestgun(lastmillis);
				if(d->gunselect != bestgun && d->canswitch(bestgun, lastmillis))
				{
					d->setgun(bestgun, lastmillis);
					cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
					cl.playsoundc(S_SWITCH, d);
				}
				else if(d->ammo[d->gunselect] <= 0 && d->canreload(d->gunselect, lastmillis))
				{
					d->gunreload(d->gunselect, guntype[d->gunselect].add, lastmillis);
					cl.cc.addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
					cl.playsoundc(S_RELOAD, d);
				}

				vec movepos;
				if(hunt(d, b, movepos, true))
				{
					aim(d, b, movepos, d->aimyaw, d->aimpitch, false);
					aim(d, b, b.type != BS_ATTACK ? movepos : b.targpos, d->yaw, d->pitch, true);

					if(movepos.z-(d->o.z-d->height) >= BOTJUMPHEIGHT && !d->timeinair)
						d->jumping = true;

					d->move = 1;
					d->strafe = 0;
				}
				else
				{
					if(b.type != BS_ATTACK) reset(d, false);
					else aim(d, b, b.targpos, d->yaw, d->pitch, true);

					d->aimyaw = d->yaw;
					d->aimpitch = d->pitch;
					d->move = d->strafe = 0;
				}
			}
		}
		else d->stopmoving();

		findorientation(d->o, d->yaw, d->pitch, d->bot->target);
		cl.ph.move(d, 10, true);
		d->stopactions();
	}

	void think(fpsent *d)
	{
		// the state stack works like a chain of commands, certain commands simply replace
		// each other, others spawn new commands to the stack the ai reads the top command
		// from the stack and executes it or pops the stack and goes back along the history
		// until it finds a suitable command to execute.

		if(!botstall() && !cl.intermission)
		{
			if(!d->bot->state.length()) reset(d, true);

			botstate &b = d->bot->getstate();

			#define getavoided(pl) \
			{ \
				if(pl && pl != d && pl->state == CS_ALIVE && cl.et.ents.inrange(pl->lastnode) && d->bot->avoid.find(pl->lastnode) < 0) \
					d->bot->avoid.add(pl->lastnode); \
			} \

			d->bot->avoid.setsize(0);
			getavoided(cl.player1);
			loopv(cl.players) getavoided(cl.players[i]);
			loopv(cl.pj.projs)
			{
				if(cl.pj.projs[i] && cl.pj.projs[i]->state == CS_ALIVE)
				{
					int n = cl.et.waypointnode(cl.pj.projs[i]->o, true);
					if(cl.et.ents.inrange(n) && d->bot->avoid.find(n) < 0)
						d->bot->avoid.add(n);
				}
			}

			if(lastmillis-d->bot->lastaction >= BOTFRAMETIME)
			{
				bool result = false;

				d->bot->lastaction += BOTFRAMETIME;
				b.cycle++;

				switch(b.type)
				{
					case BS_WAIT:		result = dowait(d, b);		break;
					case BS_DEFEND:		result = dodefend(d, b);	break;
					case BS_PURSUE:		result = dopursue(d, b);	break;
					case BS_ATTACK:		result = doattack(d, b);	break;
					case BS_INTEREST:	result = dointerest(d, b);	break;
					default: break;
				}
				if(!result) reset(d, false);
			}

			process(d);
		}
		else
		{
			d->stopmoving();
			d->bot->lastaction = lastmillis;
		}

		d->lastupdate = lastmillis;
	}

	void drawstate(fpsent *d, botstate &b, bool top, int above)
	{
		const char *bnames[BS_MAX] = {
			"wait", "defend", "pursue", "attack", "interest"
		}, *btypes[BTRG_MAX+1] = {
			"none", "node", "player", "entity", "flag"
		};
		s_sprintfd(s)("@%s%s [%d] goal:%d[%d] %s:%d",
			top ? "\fy" : "\fw",
			bnames[b.type],
			b.cycle,
			b.goal(), b.route.length(),
			btypes[b.targtype+1], b.target
		);
		particle_text(vec(d->abovehead()).add(vec(0, 0, above)), s, 14, 1);
	}

	void drawroute(fpsent *d, botstate &b, float amt = 1.f)
	{
		renderprimitive(true);

		int colour = teamtype[d->team].colour, last = -1;
		float cr = (colour>>16)/255.f, cg = ((colour>>8)&0xFF)/255.f, cb = (colour&0xFF)/255.f;

		loopv(b.route)
		{
			if(b.route.inrange(last))
			{
				int index = b.route[i], prev = b.route[last];

				if(cl.et.ents.inrange(index) && cl.et.ents.inrange(prev))
				{
					fpsentity &e = (fpsentity &) *cl.et.ents[index], &f = (fpsentity &) *cl.et.ents[prev];
					renderline(vec(f.o).add(vec(0, 0, 1)), vec(e.o).add(vec(0, 0, 1)), cr*amt, cg*amt, cb*amt, false);
				}
			}
			last = i;
		}
		if(botdebug() > 3)
		{
			vec target = cl.ws.gunorigin(d->gunselect, d->o, d->bot->target, d);
			renderline(target, d->bot->target, 0.5f, 0.5f, 0.5f, false);
			target = vec(d->o).sub(vec(0, 0, d->height));
			renderline(target, b.targpos, 0.25f, 0.25f, 0.25f, false);
		}
		renderprimitive(false);
	}

	void render()
	{
		if(botdebug())
		{
			int teams[2][TEAM_MAX] = {{ 0, 0, 0, 0, 0 }, { 0, 0, 0, 0, 0 }};
			loopv(cl.players) if(cl.players[i] && cl.players[i]->bot) teams[0][cl.players[i]->team]++;
			loopv(cl.players) if(cl.players[i] && cl.players[i]->state == CS_ALIVE && cl.players[i]->bot)
			{
				fpsent *d = cl.players[i];
				teams[1][d->team]++;
				bool top = true;
				int above = 0;
				loopvrev(d->bot->state)
				{
					botstate &b = d->bot->state[i];
					drawstate(d, b, top, above += 2);
					if(botdebug() > 2 && top && rendernormally && b.type != BS_WAIT)
						drawroute(d, b, teams[0][d->team] ? float(teams[1][d->team])/float(teams[0][d->team]) : 1.f);
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
