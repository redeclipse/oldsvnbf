struct botclient
{
	GAMECLIENT &cl;

	static const int BOTISNEAR			= 64;			// is near
	static const int BOTISFAR			= 128;			// too far
	static const int BOTJUMPHEIGHT		= 8;			// decides to jump
	static const int BOTJUMPIMPULSE		= 16;			// impulse to jump
	static const float BOTLOSMIN		= 64.f;			// minimum line of sight
	static const float BOTLOSMAX		= 4096.f;		// maximum line of sight
	static const float BOTFOVMIN		= 80.f;			// minimum field of view
	static const float BOTFOVMAX		= 130.f;		// maximum field of view

	IVAR(botskill, 1, 30, 100);
	IVAR(botstall, 0, 0, 1);
	IVAR(botdebug, 0, 2, 5);

	#define BOTSCALE(x)				clamp(101 - clamp(x, 1, 100), 1, 100)
	#define BOTRATE					BOTSCALE(botskill())
	#define BOTCHANCE				rnd(botskill())
	#define BOTLOSDIST				clamp(BOTLOSMIN+((BOTLOSMAX-BOTLOSMIN)/float(BOTRATE)), BOTLOSMIN, float(getvar("fog"))+BOTLOSMIN)
	#define BOTFOVX					clamp(BOTFOVMIN+((BOTFOVMAX-BOTFOVMIN)/float(BOTRATE)), BOTFOVMIN, BOTFOVMAX)
	#define BOTFOVY					BOTFOVX*3/4
	#define BOTTARG(x,y,z)			(y != x && y->state == CS_ALIVE && (!z || !m_team(cl.gamemode, cl.mutators) || y->team != x->team))

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
		loopvrev(cl.players) if(cl.players[i] && cl.players[i]->bot)
		{
			cl.cc.addmsg(SV_DELBOT, "ri", cl.players[i]->clientnum);
			if(--c <= 0) break;
		}
	}

	void create(fpsent *d)
	{
		if(d->bot) return;
		d->bot = new botinfo();
		if(!d->bot) fatal("could not create bot");
	}

	void update()
	{
		loopv(cl.players) if(cl.players[i] && cl.players[i]->bot) think(cl.players[i]);
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
		vec dir(d->o);
		dir.sub(vec(0, 0, d->height));
		dir.sub(pos);
		dir.normalize();
		float targyaw, targpitch;
		vectoyawpitch(dir, targyaw, targpitch);
		float margin = (float(BOTRATE/4)/100.f)+0.05f,
			cyaw = fabs(targyaw-d->yaw), cpitch = fabs(targpitch-d->pitch);
		return cyaw < margin*BOTFOVX && cpitch < margin*BOTFOVY;
	}

	bool checkothers(vector<int> &targets, fpsent *d = NULL, int state = -1, int targtype = -1, int target = -1, bool teams = false)
	{ // checks the states of other bots for a match
		targets.setsize(0);
		fpsent *e = NULL;
		loopi(cl.numdynents()) if((e = (fpsent *)cl.iterdynents(i)) && e != d && e->bot && e->state == CS_ALIVE)
		{
			if(targets.find(e->clientnum) >= 0) continue;
			if(teams && m_team(cl.gamemode, cl.mutators) && d && d->team != e->team) continue;

			botstate &b = e->bot->getstate();
			if(state >= 0 && b.type != state) continue;
			if(target >= 0 && b.target != target) continue;
			if(targtype >=0 && b.targtype != targtype) continue;
			targets.add(e->clientnum);
		}
		return !targets.empty();
	}

	bool makeroute(fpsent *d, botstate &b, int node, float tolerance = 0.f)
	{
		if(cl.et.route(d->lastnode, node, d->bot->route, d->bot->avoid, tolerance))
		{
			b.goal = false;
			b.override = false;
			return true;
		}
		return false;
	}

	bool makeroute(fpsent *d, botstate &b, vec &pos, float tolerance = 0.f, bool dist = false)
	{
		int node = cl.et.waypointnode(pos, dist);
		return makeroute(d, b, node, tolerance);
	}

	bool randomnode(fpsent *d, botstate &b, vec &from, vec &to, float radius, float wander)
	{
		vector<int> waypoints;
		waypoints.setsize(0);
		loopvj(cl.et.ents)
		{
			if(cl.et.ents[j]->type == WAYPOINT && j != d->lastnode)
			{
				float fdist = cl.et.ents[j]->o.dist(to), dist = cl.et.ents[j]->o.dist(from);
				if(dist > radius && fdist > radius && fdist < wander)
					waypoints.add(j);
			}
		}

		while(!waypoints.empty())
		{
			int w = rnd(waypoints.length()), n = waypoints.remove(w);
			if(makeroute(d, b, n)) return true;
		}
		return false;
	}

	bool patrol(fpsent *d, botstate &b, vec &pos, float radius, float wander)
	{
		vec feet(vec(d->o).sub(vec(0, 0, d->height)));
		if(feet.dist(pos) <= radius || b.override)
		{ // run away and back to keep ourselves busy
			if((!b.override || b.goal) && randomnode(d, b, feet, pos, radius, wander))
				b.override = true;
			return true;
		}
		return makeroute(d, b, pos, radius);
	}

	bool follow(fpsent *d, botstate &b, fpsent *e)
	{
		vec pos(vec(e->o).sub(vec(0, 0, e->height))), feet(vec(d->o).sub(vec(0, 0, d->height)));
		if(feet.dist(pos) <= BOTISNEAR || b.override)
		{ // random path if too close
			if((!b.override || b.goal) && randomnode(d, b, feet, pos, BOTISNEAR, BOTISFAR))
				b.override = true;
			return true;
		}
		return makeroute(d, b, e->lastnode, e->radius*2.f);
	}

	bool violence(fpsent *d, botstate &b, fpsent *e, bool pursue = false)
	{
		if((!pursue && !b.goal) || (pursue && follow(d, b, e)))
		{
			botstate &c = d->bot->addstate(pursue ? BS_PURSUE : BS_ATTACK);
			c.targtype = BT_PLAYER;
			d->bot->enemy = c.target = e->clientnum;
			if(pursue) c.expire = 10000;
			return true;
		}
		if(pursue) return violence(d, b, e, false);
		return false;
	}

	bool defer(fpsent *d, botstate &b, bool pursue = false)
	{
		fpsent *t = NULL, *e = NULL;
		vec targ;
		loopi(cl.numdynents()) if((e = (fpsent *)cl.iterdynents(i)) && e != d && BOTTARG(d, e, true))
		{
			if((!t || e->o.dist(d->o) < t->o.dist(d->o)) &&
				getsight(d->o, d->yaw, d->pitch, e->o, targ, BOTLOSDIST, BOTFOVX, BOTFOVY))
					t = e;
		}
		if(t) return violence(d, b, t, pursue);
		return false;
	}

	struct interest
	{
		int state, node, target, targtype, expire;
		float tolerance, score;

		interest() :
			state(-1), node(-1), target(-1), targtype(-1), expire(0), tolerance(0.f), score(0.f) {}
		~interest() {}
	};

	bool find(fpsent *d, botstate &b, bool override = true)
	{
		vector<interest> interests;
		interests.setsize(0);

		if(m_ctf(cl.gamemode)) loopvj(cl.ctf.flags)
		{
			ctfstate::flag &f = cl.ctf.flags[j];

			vector<int> targets; // build a list of others who are interested in this
			checkothers(targets, d, f.team == d->team ? BS_DEFEND : BS_PURSUE, BT_FLAG, j, true);

			fpsent *e = NULL;
			loopi(cl.numdynents()) if((e = (fpsent *)cl.iterdynents(i)) && BOTTARG(d, e, false) && !e->bot && d->team == e->team)
			{ // try to guess what non bots are doing
				if(targets.find(e->clientnum) < 0 && (e->o.dist(f.pos()) <= enttype[FLAG].radius*2.f || f.owner == e))
					targets.add(e->clientnum);
			}

			if(f.team == d->team)
			{
				bool guard = false;
				if(f.owner || targets.empty()) guard = true;
				else if(d->gunselect != GUN_PISTOL)
				{ // see if we can relieve someone who only has a pistol
					fpsent *t;
					loopvk(targets) if((t = cl.getclient(targets[k])) && t->gunselect == GUN_PISTOL)
					{
						guard = true;
						break;
					}
				}

				if(guard)
				{ // defend the flag
					interest &n = interests.add();
					n.state = BS_DEFEND;
					n.node = cl.et.waypointnode(f.pos(), false);
					n.target = j;
					n.targtype = BT_FLAG;
					n.expire = 10000;
					n.tolerance = enttype[FLAG].radius*2.f;
					n.score = d->o.dist(f.pos())/(d->gunselect != GUN_PISTOL ? 1.f : 100.f);
				}
			}
			else
			{
				if(targets.empty())
				{ // attack the flag
					interest &n = interests.add();
					n.state = BS_PURSUE;
					n.node = cl.et.waypointnode(f.pos(), false);
					n.target = j;
					n.targtype = BT_FLAG;
					n.expire = 10000;
					n.tolerance = enttype[FLAG].radius*2.f;
					n.score = d->o.dist(f.pos());
				}
				else
				{ // help by defending the attacker
					fpsent *t;
					loopvk(targets) if((t = cl.getclient(targets[k])))
					{
						interest &n = interests.add();
						n.state = BS_DEFEND;
						n.node = t->lastnode;
						n.target = t->clientnum;
						n.targtype = BT_PLAYER;
						n.expire = 5000;
						n.tolerance = t->radius*2.f;
						n.score = d->o.dist(t->o);
					}
				}
			}
		}

		loopvj(cl.et.ents)
		{
			fpsentity &e = (fpsentity &)*cl.et.ents[j];
			switch(e.type)
			{
				case WEAPON:
				{
					if(e.spawned && isgun(e.attr1) && guntype[e.attr1].rdelay > 0 && d->ammo[e.attr1] <= 0 && e.attr1 > d->bestgun(lastmillis))
					{ // go get a weapon upgrade
						interest &n = interests.add();
						n.state = BS_INTEREST;
						n.node = cl.et.waypointnode(e.o, true);
						n.target = j;
						n.targtype = BT_ENTITY;
						n.expire = 10000;
						n.tolerance = enttype[e.type].radius+d->radius;
						n.score = d->o.dist(e.o)/(d->gunselect != GUN_PISTOL ? 1.f : 10.f);
					}
					break;
				}
				default: break;
			}
		}

		while(!interests.empty())
		{
			int q = interests.length()-1;
			loopi(interests.length()-1) if(interests[i].score < interests[q].score) q = i;
			interest n = interests.removeunordered(q);
			if(makeroute(d, b, n.node, n.tolerance))
			{
				botstate &c = override ? d->bot->setstate(n.state) : d->bot->addstate(n.state);
				c.targtype = n.targtype;
				c.target = n.target;
				c.expire = n.expire;
				return true;
			}
		}
		return false;
	}

	bool ctfhomerun(fpsent *d, botstate &b)
	{
		loopk(2)
		{
			int goal = -1;
			loopv(cl.ctf.flags)
			{
				ctfstate::flag &g = cl.ctf.flags[i];
				if(g.team == d->team && (k || (!g.owner && !g.droptime)) &&
					(!cl.ctf.flags.inrange(goal) || g.pos().dist(d->o) < cl.ctf.flags[goal].pos().dist(d->o)))
				{
					goal = i;
				}
			}

			if(cl.ctf.flags.inrange(goal) && makeroute(d, b, cl.ctf.flags[goal].pos(), enttype[FLAG].radius*2.f))
			{
				botstate &c = d->bot->setstate(BS_PURSUE); // replaces current state!
				c.targtype = BT_FLAG;
				c.target = goal;
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
					if(b.targtype == BT_PLAYER || (b.targtype == BT_FLAG && m_ctf(cl.gamemode) && cl.ctf.flags[b.target].team != d->team))
					{
						result = true;
						if(b.targtype != BT_PLAYER) pursue = true;
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
		if(checkothers(targets, d, BS_DEFEND, BT_PLAYER, d->clientnum, true))
		{
			fpsent *t;
			loopv(targets) if((t = cl.getclient(targets[i])) && t->bot)
			{
				botstate &c = t->bot->getstate();
				violence(t, c, e, true);
			}
		}
	}

	void spawned(fpsent *d)
	{
		if(d->bot) d->bot->reset();
	}

	void killed(fpsent *d, fpsent *e, int gun, int flags, int damage)
	{
		if(d->bot) d->bot->reset();
	}

	bool dowait(fpsent *d, botstate &b)
	{
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

				if(!hasflags.empty() && ctfhomerun(d, b))
					return true;
			}

			if(find(d, b, true)) return true;
			if(defer(d, b, true)) return true;

			vec feet(vec(d->o).sub(vec(0, 0, d->height)));
			if(randomnode(d, b, feet, feet, BOTISNEAR, 1e16f))
			{
				botstate &c = d->bot->setstate(BS_INTEREST);
				c.targtype = BT_NODE;
				c.target = d->bot->route[0];
				c.expire = 10000;
				return true;
			}
		}
		return true; // but don't pop the state
	}

	bool dodefend(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			switch(b.targtype)
			{
				case BT_FLAG:
				{
					if(m_ctf(cl.gamemode) && cl.ctf.flags.inrange(b.target))
					{
						ctfstate::flag &f = cl.ctf.flags[b.target];
						if(f.owner && violence(d, b, f.owner, true)) return true;
						if(patrol(d, b, f.pos(), enttype[FLAG].radius, enttype[FLAG].radius*4.f))
						{
							defer(d, b, false);
							return true;
						}
					}
					break;
				}
				case BT_PLAYER:
				{
					fpsent *e = cl.getclient(b.target);
					if(e && e->state == CS_ALIVE)
					{
						if(patrol(d, b, vec(e->o).sub(vec(0, 0, e->height)), d->radius*2.f, float(BOTISNEAR)))
						{
							defer(d, b, false);
							return true;
						}
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
			vec targ;
			fpsent *e = cl.getclient(b.target);
			if(e && e->state == CS_ALIVE && raycubelos(d->o, e->o, targ))
			{
				d->bot->enemy = e->clientnum;
				if(d->canshoot(d->gunselect, lastmillis) && (!BOTCHANCE || hastarget(d, b, e->o)))
				{
					d->attacking = true;
					d->attacktime = lastmillis;
					return false;
				}
				if(b.goal) follow(d, b, e);
				return true;
			}
		}
		return false;
	}

	bool dointerest(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			if(cl.et.ents.inrange(b.target))
			{
				fpsentity &e = (fpsentity &)*cl.et.ents[b.target];
				if(enttype[e.type].usetype == ETU_ITEM)
				{
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
					if(lastmillis-d->usetime > 3000 && d->canuse(e.type, e.attr1, e.attr2, lastmillis))
					{
						float eye = d->height*0.5f;
						vec m = d->o;
						m.z -= eye;

						if(insidesphere(m, eye, d->radius-1.f, e.o, enttype[e.type].height, enttype[e.type].radius))
						{
							d->useaction = true;
							d->usetime = lastmillis;
							return false;
						}
					}
				}
				if(makeroute(d, b, e.o, enttype[e.type].radius+d->radius))
				{
					defer(d, b, b.targtype == BT_NODE);
					return true;
				}
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
				case BT_FLAG:
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

							if(hasflags.empty())
								return false; // otherwise why are we pursuing home?

							if(makeroute(d, b, f.pos(), enttype[FLAG].radius*2.f))
							{
								defer(d, b, false);
								return true;
							}
						}
						else
						{
							if(f.owner == d) return ctfhomerun(d, b);
							if(makeroute(d, b, f.pos(), enttype[FLAG].radius*2.f))
							{
								defer(d, b, false);
								return true;
							}
						}
					}
					break;
				}

				case BT_PLAYER:
				{
					fpsent *e = cl.getclient(b.target);
					if(e && e->state == CS_ALIVE && follow(d, b, e))
					{
						d->bot->enemy = e->clientnum;
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

	int closenode(fpsent *d, botstate &b)
	{
		int node = -1;
		loopvrev(d->bot->route) if(cl.et.ents.inrange(d->bot->route[i]) && d->bot->route[i] != d->lastnode && d->bot->route[i] != d->oldnode)
		{
			fpsentity &e = (fpsentity &)*cl.et.ents[d->bot->route[i]];

			if(!d->bot->route.inrange(node) ||
				(e.o.dist(cl.ph.feetpos(d)) < enttype[WAYPOINT].radius*10.f &&
					e.o.dist(cl.ph.feetpos(d)) < cl.et.ents[d->bot->route[node]]->o.dist(cl.ph.feetpos(d))))
						node = i;
		}
		return node;
	}

	bool hunt(fpsent *d, botstate &b)
	{
		if(!d->bot->route.empty() && !b.goal)
		{
			int n = d->bot->route.find(d->lastnode);
			if(d->bot->route.inrange(n) && --n >= 0) // otherwise got to goal
			{
				if(!d->bot->route.inrange(n)) n = closenode(d, b);
				if(d->bot->route.inrange(n) && d->bot->avoid.find(d->bot->route[n]) < 0)
				{
					d->bot->spot = vec(cl.et.ents[d->bot->route[n]]->o).add(vec(0, 0, d->height));
					return true;
				}
			}
		}
		b.goal = true;
		return false;
	}

	void aim(fpsent *d, botstate &b, vec &pos, float &yaw, float &pitch, bool aiming = true)
	{
		float targyaw = -(float)atan2(pos.x-d->o.x, pos.y-d->o.y)/PI*180+180;
		if(yaw < targyaw-180.0f) yaw += 360.0f;
		if(yaw > targyaw+180.0f) yaw -= 360.0f;
		float dist = d->o.dist(pos), targpitch = asin((pos.z-d->o.z)/dist)/RAD;
		if(aiming)
		{
			float amt = float(lastmillis-d->lastupdate)/float(BOTRATE)/10.f,
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

	void process(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			int bestgun = d->bestgun(lastmillis);
			if(d->gunselect != bestgun && d->canswitch(bestgun, lastmillis))
			{
				d->setgun(bestgun, lastmillis);
				cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
			}
			else if(d->ammo[d->gunselect] <= 0 && d->canreload(d->gunselect, lastmillis))
			{
				d->gunreload(d->gunselect, guntype[d->gunselect].add, lastmillis);
				cl.cc.addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
			}

			if(hunt(d, b))
			{
				if(d->bot->spot.z-d->o.z > BOTJUMPHEIGHT && !d->timeinair && lastmillis-d->jumptime > 1000)
				{
					d->jumping = true;
					d->jumptime = lastmillis;
				}

				d->move = 1;
				d->strafe = 0;
			}
			else d->move = d->strafe = 0;

			vec aimpos;
			fpsent *e;
			if((e = cl.getclient(d->bot->enemy)) && e->state == CS_ALIVE &&
				raycubelos(d->o, e->o, aimpos)) aimpos = e->o;
			else
			{
				d->bot->enemy = -1;
				aimpos = d->bot->spot;
			}

			aim(d, b, aimpos, d->yaw, d->pitch, true);
			aim(d, b, d->bot->spot, d->aimyaw, d->aimpitch, false);
		}
		else d->move = d->strafe = 0;
	}

	void check(fpsent *d)
	{
		findorientation(d->o, d->yaw, d->pitch, d->bot->target);

		if(d->state == CS_ALIVE)
		{
			cl.et.checkitems(d);
			cl.ws.shoot(d, d->bot->target);
		}

		cl.ph.move(d, 10, true);
		d->attacking = d->jumping = d->reloading = d->useaction = false;
	}

	void avoid(fpsent *d)
	{
		d->bot->avoid.setsize(0);
		if(d->state == CS_ALIVE)
		{
			loopvk(cl.et.ents) if(cl.et.ents[k]->type == WAYPOINT && d->bot->avoid.find(k) < 0)
			{
				bool found = false;
				fpsentity &f = (fpsentity &)*cl.et.ents[k];
				fpsent *e = NULL;
				loopi(cl.numdynents()) if((e = (fpsent *)cl.iterdynents(i)) && BOTTARG(d, e, true))
				{
					if(f.o.dist(vec(e->o).sub(vec(0, 0, e->height))) <= e->radius+d->radius)
					{
						d->bot->avoid.add(k);
						found = true;
						break;
					}
				}
				if(!found) loopv(cl.pj.projs) if(cl.pj.projs[i] && cl.pj.projs[i]->state == CS_ALIVE)
				{
					if(cl.pj.projs[i]->projtype == PRJ_SHOT &&
						f.o.dist(cl.pj.projs[i]->o) <= guntype[cl.pj.projs[i]->gun].radius+d->radius)
						{
							d->bot->avoid.add(k);
							found = true;
							break;
						}
				}
			}
		}
	}

	void think(fpsent *d)
	{
		if(d->bot->state.empty()) d->bot->reset();

		// the state stack works like a chain of commands, certain commands simply replace
		// each other, others spawn new commands to the stack the ai reads the top command
		// from the stack and executes it or pops the stack and goes back along the history
		// until it finds a suitable command to execute.

		if(!botstall() && !cl.intermission)
		{
			botstate &b = d->bot->getstate();
			process(d, b);
			if(lastmillis >= b.next)
			{
				int cmdstart = botdebug() > 4 ? SDL_GetTicks() : 0;
				bool result = false;

				b.next = lastmillis + botframetimes[b.type];
				b.cycle++;

				avoid(d);

				switch(b.type)
				{
					case BS_WAIT:		result = dowait(d, b);		break;
					case BS_DEFEND:		result = dodefend(d, b);	break;
					case BS_PURSUE:		result = dopursue(d, b);	break;
					case BS_ATTACK:		result = doattack(d, b);	break;
					case BS_INTEREST:	result = dointerest(d, b);	break;
					default: break;
				}

				if((b.expire && (b.expire -= botframetimes[b.type]) <= 0) || !result)
					d->bot->removestate();

				if(botdebug() > 4)
					conoutf("%s processed command (%s) in %fs", cl.colorname(d), result ? "ok" : "fail", (SDL_GetTicks()-cmdstart)/1000.0f);
			}
			check(d);
		}
		else d->stopmoving();
		d->lastupdate = lastmillis;
	}

	void drawstate(fpsent *d, botstate &b, bool top, int above)
	{
		const char *bnames[BS_MAX] = {
			"wait", "defend", "pursue", "attack", "interest"
		}, *btypes[BT_MAX+1] = {
			"none", "node", "player", "entity", "flag"
		};
		s_sprintfd(s)("@%s%s [%d:%d:%d] goal:%d[%d] %s:%d",
			top ? "\fy" : "\fw",
			bnames[b.type],
			b.cycle, b.expire, b.next-lastmillis,
			!d->bot->route.empty() ? d->bot->route[0] : -1,
			d->bot->route.length(),
			btypes[b.targtype+1], b.target
		);
		particle_text(vec(d->abovehead()).add(vec(0, 0, above)), s, 14, 1);
	}

	void drawroute(fpsent *d, botstate &b, float amt = 1.f)
	{
		renderprimitive(true);
		int colour = teamtype[d->team].colour, last = -1;
		float cr = (colour>>16)/255.f, cg = ((colour>>8)&0xFF)/255.f, cb = (colour&0xFF)/255.f;

		loopvrev(d->bot->route)
		{
			if(d->bot->route.inrange(last))
			{
				int index = d->bot->route[i], prev = d->bot->route[last];
				if(cl.et.ents.inrange(index) && cl.et.ents.inrange(prev))
				{
					fpsentity &e = (fpsentity &) *cl.et.ents[index],
						&f = (fpsentity &) *cl.et.ents[prev];
					vec fr(vec(f.o).add(vec(0, 0, 4.f*amt))),
						dr(vec(e.o).add(vec(0, 0, 4.f*amt)));
					renderline(fr, dr, cr, cg, cb, false);
					dr.sub(fr);
					dr.normalize();
					float yaw, pitch;
					vectoyawpitch(dr, yaw, pitch);
					dr.mul(RENDERPUSHX);
					dr.add(fr);
					rendertris(dr, yaw, pitch, 2.f, cr, cg, cb, true, false);
				}
			}
			last = i;
		}
		if(botdebug() > 3)
		{
			vec fr(cl.ws.gunorigin(d->gunselect, d->o, d->bot->target, d)),
				dr(d->bot->target);
			if(dr.dist(d->o) > BOTLOSDIST)
			{
				dr.sub(fr);
				dr.normalize();
				dr.mul(BOTLOSDIST);
			}
			renderline(fr, dr, cr, cg, cb, false);
			rendertris(dr, d->yaw, d->pitch, 2.f, cr, cg, cb, true, false);
		}
		renderprimitive(false);
	}

	void render()
	{
		if(botdebug())
		{
			int amt[2] = { 0, 0 };
			loopv(cl.players) if(cl.players[i] && cl.players[i]->bot) amt[0]++;
			loopv(cl.players) if(cl.players[i] && cl.players[i]->state == CS_ALIVE && cl.players[i]->bot)
			{
				fpsent *d = cl.players[i];
				bool top = true;
				int above = 0;
				amt[1]++;
				loopvrev(d->bot->state)
				{
					botstate &b = d->bot->state[i];
					drawstate(d, b, top, above += 2);
					if(botdebug() > 2 && top && rendernormally && b.type != BS_WAIT)
						drawroute(d, b, float(amt[1])/float(amt[0]));
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
