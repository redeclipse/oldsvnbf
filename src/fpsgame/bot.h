struct botclient
{
	GAMECLIENT &cl;

	static const int BOTISNEAR			= 64;			// is near
	static const int BOTISFAR			= 128;			// too far
	static const int BOTJUMPHEIGHT		= 8;			// decides to jump
	static const int BOTJUMPIMPULSE		= 16;			// impulse to jump
	static const int BOTLOSMIN			= 64;			// minimum line of sight
	static const int BOTLOSMAX			= 4096;			// maximum line of sight
	static const int BOTFOVMIN			= 90;			// minimum field of view
	static const int BOTFOVMAX			= 130;			// maximum field of view

	IVAR(botdebug, 0, 0, 5);

	#define BOTLOSDIST(x)			clamp((BOTLOSMIN+(BOTLOSMAX-BOTLOSMIN))/100.f*float(x), float(BOTLOSMIN), float(getvar("fog")+BOTLOSMIN))
	#define BOTFOVX(x)				clamp((BOTFOVMIN+(BOTFOVMAX-BOTFOVMIN))/100.f*float(x), float(BOTFOVMIN), float(BOTFOVMAX))
	#define BOTFOVY(x)				BOTFOVX(x)*3.f/4.f
	#define BOTTARG(x,y,z)			(y != x && y->state == CS_ALIVE && lastmillis-y->lastspawn > REGENWAIT && (!z || !m_team(cl.gamemode, cl.mutators) || y->team != x->team))

	botclient(GAMECLIENT &_cl) : cl(_cl)
	{
		CCOMMAND(addbot, "s", (botclient *self, char *s),
			self->addbot(*s ? clamp(atoi(s), 1, 100) : -1)
		);
		CCOMMAND(delbot, "", (botclient *self), self->delbot());
	}

	void addbot(int s) { cl.cc.addmsg(SV_ADDBOT, "ri", s); }
	void delbot() { cl.cc.addmsg(SV_DELBOT, "r"); }

	void create(fpsent *d)
	{
		if(!d->bot)
		{
			if(!(d->bot = new botinfo()))
				fatal("could not create bot");
		}
	}

	void destroy(fpsent *d)
	{
		if(d->bot)
		{
			DELETEP(d->bot);
			d->bot = NULL;
		}
	}

	void update()
	{
		loopv(cl.players) if(cl.players[i] && cl.players[i]->bot) think(cl.players[i]);
	}

	bool getsight(vec &o, float yaw, float pitch, vec &q, vec &v, float mdist, float fovx, float fovy)
	{
		float dist = o.dist(q);

		if(dist <= mdist)
		{
			float x = fabs((asin((q.z-o.z)/dist)/RAD)-pitch);
			float y = fabs((-(float)atan2(q.x-o.x, q.y-o.y)/PI*180+180)-yaw);
			if(x <= fovx && y <= fovy) return raycubelos(o, q, v);
		}
		return false;
	}

	bool hastarget(fpsent *d, botstate &b, vec &pos)
	{ // add margins of error
		if(!rnd(d->skill*10)) return true;
		else
		{
			vec dir(cl.feetpos(d, 0.f));
			dir.sub(pos);
			dir.normalize();
			float targyaw, targpitch;
			vectoyawpitch(dir, targyaw, targpitch);
			float rtime = d->skill*guntype[d->gunselect].rdelay/1000.f,
				atime = d->skill*guntype[d->gunselect].adelay/100.f,
					skew = float(lastmillis-b.millis)/(rtime+atime),
						cyaw = fabs(targyaw-d->yaw), cpitch = fabs(targpitch-d->pitch);
			if(cyaw <= BOTFOVX(d->skill)*skew && cpitch <= BOTFOVY(d->skill)*skew)
				return true;
		}
		return false;
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
				float tdist = cl.et.ents[j]->o.dist(to), fdist = cl.et.ents[j]->o.dist(from);
				if(tdist > radius && fdist > radius && fdist <= wander)
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

	bool randomnode(fpsent *d, botstate &b, float radius, float wander)
	{
		vec feet(cl.feetpos(d, 0.f));
		return randomnode(d, b, feet, feet, radius, wander);
	}

	bool patrol(fpsent *d, botstate &b, vec &pos, float radius, float wander, bool retry = false)
	{
		vec feet(cl.feetpos(d, 0.f));
		if(b.override || feet.dist(pos) <= radius || !makeroute(d, b, pos, wander))
		{ // run away and back to keep ourselves busy
			if(!b.override && randomnode(d, b, feet, pos, radius, wander))
			{
				b.override = true;
				return true;
			}
			else if(!b.goal) return true;
			else if(!retry)
			{
				b.override = false;
				return patrol(d, b, pos, radius, wander, true);
			}
			return false;
		}
		return true;
	}

	bool follow(fpsent *d, botstate &b, fpsent *e, bool retry = false)
	{
		vec pos(cl.feetpos(e, 0.f)), feet(cl.feetpos(d, 0.f));
		if(b.override || feet.dist(pos) <= BOTISNEAR || !makeroute(d, b, e->lastnode, e->radius*2.f))
		{ // random path if too close
			if(!b.override && randomnode(d, b, feet, pos, BOTISNEAR, BOTISFAR))
			{
				b.override = true;
			}
			else if(!b.goal) return true;
			else if(!retry)
			{
				b.override = false;
				return follow(d, b, e, true);
			}
			else return false;
		}
		d->bot->enemy = e->clientnum;
		return true;
	}

	bool violence(fpsent *d, botstate &b, fpsent *e, bool pursue = false)
	{
		if(BOTTARG(d, e, true))
		{
			if(!pursue || follow(d, b, e))
			{
				botstate &c = d->bot->addstate(pursue ? BS_PURSUE : BS_ATTACK);
				c.targtype = BT_PLAYER;
				d->bot->enemy = c.target = e->clientnum;
				if(pursue) c.expire = 10000;
				return true;
			}
			if(pursue) return violence(d, b, e, false);
		}
		return false;
	}

	bool defer(fpsent *d, botstate &b, bool pursue = false)
	{
		fpsent *t = NULL, *e = NULL;
		vec targ;
		loopi(cl.numdynents()) if((e = (fpsent *)cl.iterdynents(i)) && e != d && BOTTARG(d, e, true))
		{
			vec ep = cl.headpos(e), tp = t ? cl.headpos(t) : vec(0, 0, 0),
				dp = cl.headpos(d);
			if((!t || ep.dist(d->o) < tp.dist(d->o)) &&
				getsight(dp, d->yaw, d->pitch, ep, targ, BOTLOSDIST(d->skill), BOTFOVX(d->skill), BOTFOVY(d->skill)))
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
		vec pos = cl.headpos(d);
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
				vec ep = cl.headpos(e);
				if(targets.find(e->clientnum) < 0 && (ep.dist(f.pos()) <= enttype[FLAG].radius*2.f || f.owner == e))
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
					n.score = pos.dist(f.pos())/(d->gunselect != GUN_PISTOL ? 1.f : 100.f);
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
					n.score = pos.dist(f.pos());
				}
				else
				{ // help by defending the attacker
					fpsent *t;
					loopvk(targets) if((t = cl.getclient(targets[k])))
					{
						interest &n = interests.add();
						vec tp = cl.headpos(t);
						n.state = BS_DEFEND;
						n.node = t->lastnode;
						n.target = t->clientnum;
						n.targtype = BT_PLAYER;
						n.expire = 5000;
						n.tolerance = t->radius*2.f;
						n.score = pos.dist(tp);
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
					if(e.spawned && isgun(e.attr1) && guntype[e.attr1].rdelay > 0 && d->ammo[e.attr1] <= 0 && e.attr1 > d->bestgun())
					{ // go get a weapon upgrade
						interest &n = interests.add();
						n.state = BS_INTEREST;
						n.node = cl.et.waypointnode(e.o, true);
						n.target = j;
						n.targtype = BT_ENTITY;
						n.expire = 10000;
						n.tolerance = enttype[e.type].radius+d->radius;
						n.score = pos.dist(e.o)/(d->gunselect != GUN_PISTOL ? 1.f : 10.f);
					}
					break;
				}
				default: break;
			}
		}

		loopvj(cl.pj.projs) if(cl.pj.projs[j]->projtype == PRJ_ENT && cl.pj.projs[j]->ent == WEAPON && cl.pj.projs[j]->state != CS_DEAD && !cl.pj.projs[j]->beenused && cl.pj.projs[j]->owner)
		{
			if(isgun(cl.pj.projs[j]->attr1) && guntype[cl.pj.projs[j]->attr1].rdelay > 0 && d->ammo[cl.pj.projs[j]->attr1] <= 0 && cl.pj.projs[j]->attr1 > d->bestgun())
			{ // go get a weapon upgrade
				interest &n = interests.add();
				n.state = BS_INTEREST;
				n.node = cl.et.waypointnode(cl.pj.projs[j]->o, true);
				n.target = cl.pj.projs[j]->attr1;
				n.targtype = BT_DROP;
				n.expire = 10000;
				n.tolerance = enttype[cl.pj.projs[j]->ent].radius+d->radius;
				n.score = pos.dist(cl.pj.projs[j]->o)/(d->gunselect != GUN_PISTOL ? 1.f : 10.f);
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
		vec pos = cl.headpos(d);
		loopk(2)
		{
			int goal = -1;
			loopv(cl.ctf.flags)
			{
				ctfstate::flag &g = cl.ctf.flags[i];
				if(g.team == d->team && (k || (!g.owner && !g.droptime)) &&
					(!cl.ctf.flags.inrange(goal) || g.pos().dist(pos) < cl.ctf.flags[goal].pos().dist(pos)))
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
		if(BOTTARG(d, e, true))
		{
			if(d->bot)
			{
				botstate &b = d->bot->getstate();
				bool result = false, pursue = false;
				switch(b.type)
				{
					case BS_PURSUE:
						result = true;
						pursue = false;
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
			if(randomnode(d, b, BOTISFAR, 1e16f))
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
					if(e && e->state == CS_ALIVE && follow(d, b, e))
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
			vec targ, pos = cl.headpos(d);
			fpsent *e = cl.getclient(b.target);
			if(e && BOTTARG(d, e, true))
			{
				vec ep = cl.headpos(e);
				if(e->state == CS_ALIVE && raycubelos(pos, ep, targ))
				{
					d->bot->enemy = e->clientnum;
					if(m_fight(cl.gamemode) && d->canshoot(d->gunselect, lastmillis) && hastarget(d, b, ep))
					{
						d->attacking = true;
						d->attacktime = lastmillis;
						return !b.override && !b.goal;
					}
					if(b.goal) follow(d, b, e);
					return true;
				}
			}
		}
		return false;
	}

	bool dointerest(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			switch(b.targtype)
			{
				case BT_ENTITY:
				{
					if(cl.et.ents.inrange(b.target))
					{
						fpsentity &e = (fpsentity &)*cl.et.ents[b.target];
						if(enttype[e.type].usetype == EU_ITEM)
						{
							switch(e.type)
							{
								case WEAPON:
								{
									if(!e.spawned || d->ammo[e.attr1] > 0 || e.attr1 <= d->bestgun())
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
					break;
				}
				case BT_DROP:
				{
					if(cl.et.ents.inrange(b.goal))
					{
						fpsentity &e = (fpsentity &)*cl.et.ents[b.goal];
						if(lastmillis-d->usetime > 3000 && d->canuse(WEAPON, b.target, 0, lastmillis))
						{
							float eye = d->height*0.5f;
							vec m = d->o;
							m.z -= eye;

							if(insidesphere(m, eye, d->radius-1.f, e.o, enttype[WEAPON].height, enttype[WEAPON].radius))
							{
								d->useaction = true;
								d->usetime = lastmillis;
								return false;
							}
						}
						if(makeroute(d, b, e.o, enttype[WEAPON].radius+d->radius))
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
					if(e)
					{
						if(e->state == CS_ALIVE && follow(d, b, e))
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

	int closenode(fpsent *d, botstate &b)
	{
		int node = -1;
		loopvrev(d->bot->route) if(cl.et.ents.inrange(d->bot->route[i]) && d->bot->route[i] != d->lastnode && d->bot->route[i] != d->oldnode)
		{
			fpsentity &e = (fpsentity &)*cl.et.ents[d->bot->route[i]];

			if(!d->bot->route.inrange(node) ||
				(e.o.dist(cl.feetpos(d, 0.f)) < enttype[WAYPOINT].radius*10.f &&
					e.o.dist(cl.feetpos(d, 0.f)) < cl.et.ents[d->bot->route[node]]->o.dist(cl.feetpos(d, 0.f))))
						node = i;
		}
		return node;
	}

	bool hunt(fpsent *d, botstate &b, bool retry = false)
	{
		if(!d->bot->route.empty())
		{
			int n = d->bot->route.find(d->lastnode), g = d->bot->route[0];
			if(d->bot->route.inrange(n) && --n >= 0) // otherwise got to goal
			{
				if(!d->bot->route.inrange(n)) n = closenode(d, b);
				if(d->bot->route.inrange(n) && d->bot->avoid.find(d->bot->route[n]) < 0)
				{
					b.goal = false;
					vec from = cl.et.ents[d->bot->route[n]]->o, to(vec(from).add(vec(0, 0, PLAYERHEIGHT+d->aboveeye))), unitv;
					float dist = to.dist(from, unitv);
					unitv.div(dist);
					float barrier = raycube(from, unitv, dist, RAY_CLIPMAT);
					if(barrier < dist && dist > PLAYERHEIGHT*CROUCHHEIGHT)
					{
						to = unitv;
						to.mul(barrier);
						to.add(from);
						if(!d->crouching)
						{
							d->crouching = true;
							d->crouchtime = lastmillis;
						}
					}
					else if(d->crouching)
					{
						d->crouching = false;
						d->crouchtime = lastmillis;
					}
					d->bot->spot = to;
					vec pos = cl.feetpos(d);
					if(d->bot->spot.z-PLAYERHEIGHT-pos.z > BOTJUMPHEIGHT && !d->jumping && !d->timeinair && lastmillis-d->jumptime > 1000)
					{
						d->jumping = true;
						d->jumptime = lastmillis;
					}
					return true;
				}
				if(!retry && makeroute(d, b, g)) return hunt(d, b, true);
			}
		}
		b.goal = true;
		return false;
	}

	void aim(fpsent *d, botstate &b, vec &pos, float &yaw, float &pitch, int skew)
	{
		vec dp = cl.headpos(d);
		float targyaw = -(float)atan2(pos.x-dp.x, pos.y-dp.y)/PI*180+180;
		if(yaw < targyaw-180.0f) yaw += 360.0f;
		if(yaw > targyaw+180.0f) yaw -= 360.0f;
		float dist = dp.dist(pos), targpitch = asin((pos.z-dp.z)/dist)/RAD;
		if(skew)
		{
			float amt = float(lastmillis-d->lastupdate)/float((111-d->skill)*skew),
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
			int bestgun = d->bestgun();
			if(d->gunselect != bestgun && d->canswitch(bestgun, lastmillis))
			{
				cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
			}
			else if(d->ammo[d->gunselect] <= 0 && d->canreload(d->gunselect, lastmillis))
			{
				cl.cc.addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
			}

			bool aiming = false;
			fpsent *e = cl.getclient(d->bot->enemy);
			if(e)
			{
				vec enemypos = cl.headpos(e);
				aim(d, b, enemypos, d->yaw, d->pitch, 8);
				aiming = true;
			}

			if(hunt(d, b))
			{
				if(!aiming) aim(d, b, d->bot->spot, d->yaw, d->pitch, 16);
				aim(d, b, d->bot->spot, d->aimyaw, d->aimpitch, 4);
			}
			d->move = 1; // keep on movin'
			d->strafe = 0;
		}
		else d->stopmoving();
	}

	void check(fpsent *d)
	{
		vec pos = cl.headpos(d);
		findorientation(pos, d->yaw, d->pitch, d->bot->target);

		if(!cl.allowmove(d)) d->stopmoving();
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
					if(f.o.dist(cl.feetpos(e, 0.f)) <= e->radius+d->radius)
					{
						d->bot->avoid.add(k);
						found = true;
						break;
					}
				}
				if(!found) loopv(cl.pj.projs) if(cl.pj.projs[i] && cl.pj.projs[i]->state == CS_ALIVE)
				{
					if(cl.pj.projs[i]->projtype == PRJ_SHOT &&
						f.o.dist(cl.pj.projs[i]->o) <= guntype[cl.pj.projs[i]->attr1].explode+d->radius)
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

		if(!cl.intermission)
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
			vec fr(cl.ws.gunorigin(d->o, d->bot->target, d)),
				dr(d->bot->target), pos = cl.headpos(d);
			if(dr.dist(pos) > BOTLOSDIST(d->skill))
			{
				dr.sub(fr);
				dr.normalize();
				dr.mul(BOTLOSDIST(d->skill));
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
