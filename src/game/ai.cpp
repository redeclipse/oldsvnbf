#include "cube.h"
#include "engine.h"
#include "game.h"
namespace ai
{
	entities::avoidset obstacles;
    int updatemillis = 0;
    vec aitarget(0, 0, 0);

	VAR(aidebug, 0, 0, 5);

	ICOMMAND(addbot, "s", (char *s), client::addmsg(SV_ADDBOT, "ri", *s ? clamp(atoi(s), 1, 101) : -1));
	ICOMMAND(delbot, "", (), client::addmsg(SV_DELBOT, "r"));

	float viewdist(int x)
	{
		return x <= 100 ? clamp((SIGHTMIN+(SIGHTMAX-SIGHTMIN))/100.f*float(x), float(SIGHTMIN), float(world::fogdist)) : float(world::fogdist);
	}

	float viewfieldx(int x)
	{
		return x <= 100 ? clamp((VIEWMIN+(VIEWMAX-VIEWMIN))/100.f*float(x), float(VIEWMIN), float(VIEWMAX)) : float(VIEWMAX);
	}

	float viewfieldy(int x)
	{
		return viewfieldx(x)*3.f/4.f;
	}

	float targetable(gameent *d, gameent *e, bool z)
	{
		aistate &b = d->ai->getstate();
		if(d != e && world::allowmove(d) && b.type != AI_S_WAIT)
			return e->state == CS_ALIVE && (!z || !m_team(world::gamemode, world::mutators) || (d)->team != (e)->team);
		return false;
	}

	float cansee(gameent *d, vec &x, vec &y, vec &targ)
	{
		aistate &b = d->ai->getstate();
		if(world::allowmove(d) && b.type != AI_S_WAIT)
			return getsight(x, d->yaw, d->pitch, y, targ, d->ai->views[2], d->ai->views[0], d->ai->views[1]);
		return false;
	}


	vec getaimpos(gameent *d, gameent *e)
	{
		if(d->skill <= 100)
		{
			static vec apos;
			apos = world::headpos(e);
			apos.z -= e->height*(1.f/float(d->skill));
			return apos;
		}
		return world::headpos(e);
	}

	void create(gameent *d)
	{
		if(!d->ai)
		{
			if((d->ai = new aiinfo()) == NULL)
			{
				fatal("could not create ai");
				return;
			}
		}
		if(d->ai)
		{
			d->ai->views[0] = viewfieldx(d->skill);
			d->ai->views[1] = viewfieldy(d->skill);
			d->ai->views[2] = viewdist(d->skill);
		}
	}

	void destroy(gameent *d)
	{
		if(d->ai) DELETEP(d->ai);
	}

	void init(gameent *d, int at, int on, int sk, int bn, char *name, int tm)
	{
		gameent *o = world::newclient(on);
		string m;
		if(o) s_strcpy(m, world::colorname(o));
		else s_sprintf(m)("\fs\fwunknown [\fs\fr%d\fS]\fS", on);
		bool resetthisguy = false;
		if(!d->name[0])
		{
			if(aidebug) conoutf("\fg%s assigned to %s at skill %d", world::colorname(d, name), m, sk);
			else conoutf("\fg%s joined the game", world::colorname(d, name), m, sk);
			resetthisguy = true;
		}
		else
		{
			if(d->ownernum != on)
			{
				if(aidebug) conoutf("\fg%s reassigned to %s", world::colorname(d, name), m);
				resetthisguy = true;
			}
			if(d->skill != sk && aidebug) conoutf("\fg%s changed skill to %d", world::colorname(d, name), sk);
		}
		//else if(d->team != tm) conoutf("\fg%s switched to \fs%s%s\fS team", world::colorname(d, name), teamtype[tm].chat, teamtype[tm].name);

		s_strncpy(d->name, name, MAXNAMELEN);
		d->aitype = at;
		d->ownernum = on;
		d->skill = sk;
		d->team = tm;

		if(resetthisguy) projs::remove(d);
		if(world::player1->clientnum == d->ownernum) create(d);
		else if(d->ai) destroy(d);
	}

	void update()
	{
		bool updating = lastmillis-updatemillis > 100; // fixed rate logic at 10fps
		if(updating) avoid();
		loopv(world::players) if(world::players[i] && world::players[i]->ai)
		{
			if(!world::intermission) think(world::players[i], updating);
			else world::players[i]->stopmoving(true);
		}
		if(updating) updatemillis = lastmillis;
	}

	bool checkothers(vector<int> &targets, gameent *d, int state, int targtype, int target, bool teams)
	{ // checks the states of other ai for a match
		targets.setsize(0);
		gameent *e = NULL;
		loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && e != d && e->ai && e->state == CS_ALIVE)
		{
			if(targets.find(e->clientnum) >= 0) continue;
			if(teams && m_team(world::gamemode, world::mutators) && d && d->team != e->team) continue;
			aistate &b = e->ai->getstate();
			if(state >= 0 && b.type != state) continue;
			if(target >= 0 && b.target != target) continue;
			if(targtype >=0 && b.targtype != targtype) continue;
			targets.add(e->clientnum);
		}
		return !targets.empty();
	}

	bool makeroute(gameent *d, aistate &b, int node, bool changed, float obdist)
	{
		int n = node;
		if((n == d->lastnode || n == d->ai->lastnode || n == d->ai->prevnode) && entities::ents.inrange(d->lastnode))
		{
			gameentity &e = *(gameentity *)entities::ents[d->lastnode];
			static vector<int> noderemap; noderemap.setsizenodelete(0);
			if(!e.links.empty())
			{
				loopv(e.links) if(e.links[i] != d->lastnode && e.links[i] != d->ai->lastnode && e.links[i] != d->ai->prevnode)
					noderemap.add(e.links[i]);
			}
			if(!noderemap.empty()) n = noderemap[rnd(noderemap.length())];
		}
		if(n != d->lastnode)
		{
			if(changed && !d->ai->route.empty() && d->ai->route[0] == n) return true;
			if(entities::route(d, d->lastnode, n, d->ai->route, obstacles, obdist))
			{
				b.override = false;
				return true;
			}
			if(obdist >= 0.f) return makeroute(d, b, n, true, obdist != 0.f ? 0.f : -1.f);
		}
		d->ai->route.setsize(0);
		b.override = false;
		return false;
	}

	bool makeroute(gameent *d, aistate &b, const vec &pos, bool changed, float obdist)
	{
		int node = entities::closestent(WAYPOINT, pos, NEARDIST, true);
		return makeroute(d, b, node, changed, obdist);
	}

	bool randomnode(gameent *d, aistate &b, const vec &pos, float guard, float wander)
	{
		static vector<int> candidates;
		candidates.setsizenodelete(0);
        entities::findentswithin(WAYPOINT, pos, guard, wander, candidates);

		while(!candidates.empty())
		{
			int w = rnd(candidates.length()), n = candidates.removeunordered(w);
			if(n != d->lastnode && n != d->ai->lastnode && n != d->ai->prevnode && !obstacles.find(n, d) && makeroute(d, b, n)) return true;
		}
		return false;
	}

	bool randomnode(gameent *d, aistate &b, float guard, float wander)
	{
		vec feet = world::feetpos(d);
		return randomnode(d, b, feet, guard, wander);
	}

	bool badhealth(gameent *d)
	{
		if(d->skill <= 100) return d->health <= (111-d->skill)/4;
		return false;
	}

	bool enemy(gameent *d, aistate &b, const vec &pos, float guard = NEARDIST, bool pursue = false)
	{
		if(world::allowmove(d))
		{
			gameent *t = NULL, *e = NULL;
			vec dp = world::headpos(d), tp(0, 0, 0);
			bool insight = false, tooclose = false;
			float mindist = guard*guard;
			loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && e != d && targetable(d, e, true))
			{
				vec ep = getaimpos(d, e);
				bool close = ep.squaredist(pos) < mindist;
				if(!t || ep.squaredist(dp) < tp.squaredist(dp) || close)
				{
					bool see = cansee(d, dp, ep);
					if(!insight || see || close)
					{
						t = e; tp = ep;
						if(!insight && see) insight = see;
						if(!tooclose && close) tooclose = close;
					}
				}
			}
			if(t && violence(d, b, t, pursue && (tooclose || insight))) return insight || tooclose;
		}
		return false;
	}

	void noenemy(gameent *d)
	{
		d->ai->enemy = -1;
		d->ai->enemymillis = 0;
	}

	bool patrol(gameent *d, aistate &b, const vec &pos, float guard, float wander, int walk, bool retry)
	{
		vec feet = world::feetpos(d);
		if(walk == 2 || b.override || (walk && feet.squaredist(pos) <= guard*guard) || !makeroute(d, b, pos))
		{ // run away and back to keep ourselves busy
			if(!b.override && randomnode(d, b, pos, guard, wander))
			{
				b.override = true;
				return true;
			}
			else if(!d->ai->route.empty()) return true;
			else if(!retry)
			{
                b.override = false;
				return patrol(d, b, pos, guard, wander, walk, true);
			}
			b.override = false;
			return false;
		}
		b.override = false;
		return true;
	}

	bool defend(gameent *d, aistate &b, const vec &pos, float guard, float wander, int walk)
	{
		if(!b.override)
		{
			vec feet = world::feetpos(d);
			if(walk < 2 && feet.squaredist(pos) <= guard*guard)
			{
				bool hasenemy = enemy(d, b, pos, wander, false);
				if((walk || badhealth(d)) && hasenemy && patrol(d, b, pos, guard, wander, 2)) return true;
				d->ai->route.setsize(0);
				d->ai->spot = pos;
				b.idle = hasenemy ? 2 : 1;
				return true;
			}
		}
		return patrol(d, b, pos, guard, wander, walk);
	}

	bool violence(gameent *d, aistate &b, gameent *e, bool pursue)
	{
		if(targetable(d, e, true))
		{
			if(pursue)
				d->ai->addstate(AI_S_PURSUE, AI_T_PLAYER, e->clientnum);
			if(d->ai->enemy != e->clientnum) d->ai->enemymillis = lastmillis;
			d->ai->enemy = e->clientnum;
			d->ai->enemyseen = lastmillis;
			vec dp = world::headpos(d), ep = getaimpos(d, e);
			if(!cansee(d, dp, ep)) d->ai->enemyseen -= ((111-d->skill)*10)+10; // so we don't "quick"
			return true;
		}
		return false;
	}

	bool target(gameent *d, aistate &b, bool pursue = false, bool force = false)
	{
		gameent *t = NULL, *e = NULL;
		vec dp = world::headpos(d), tp(0, 0, 0);
		loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && e != d && targetable(d, e, true))
		{
			vec ep = getaimpos(d, e);
			if((!t || ep.squaredist(dp) < tp.squaredist(dp)) && (force || cansee(d, dp, ep)))
			{
				t = e;
				tp = ep;
			}
		}
		if(t) return violence(d, b, t, pursue);
		return false;
	}

	void assist(gameent *d, aistate &b, vector<interest> &interests, bool all = false, bool force = false)
	{
		gameent *e = NULL;
		loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && e != d && (all || e->aitype == AI_NONE) && d->team == e->team)
		{
			interest &n = interests.add();
			n.state = AI_S_DEFEND;
			n.node = e->lastnode;
			n.target = e->clientnum;
			n.targtype = AI_T_PLAYER;
			n.score = e->o.squaredist(d->o)/(force || d->hasweap(d->ai->weappref, m_spawnweapon(world::gamemode, world::mutators)) ? 10.f : 1.f);
		}
	}

	void items(gameent *d, aistate &b, vector<interest> &interests, bool force = false)
	{
		vec pos = world::feetpos(d);
		loopvj(entities::ents)
		{
			gameentity &e = *(gameentity *)entities::ents[j];
			if(enttype[e.type].usetype != EU_ITEM) continue;
			int sweap = m_spawnweapon(world::gamemode, world::mutators);
			switch(e.type)
			{
				case WEAPON:
				{
					int attr = weapattr(e.attr[0], sweap);
					if(e.spawned && isweap(attr) && !d->hasweap(attr, sweap))
					{ // go get a weapon upgrade
						interest &n = interests.add();
						n.state = AI_S_INTEREST;
						n.node = entities::closestent(WAYPOINT, e.o, NEARDIST, true);
						n.target = j;
						n.targtype = AI_T_ENTITY;
						n.score = pos.squaredist(e.o)/(force || attr == d->ai->weappref ? 100.f : 1.f);
					}
					break;
				}
				default: break;
			}
		}

		loopvj(projs::projs) if(projs::projs[j]->projtype == PRJ_ENT && projs::projs[j]->ready())
		{
			projent &proj = *projs::projs[j];
			if(!entities::ents.inrange(proj.id) || enttype[entities::ents[proj.id]->type].usetype != EU_ITEM) continue;
			int sweap = m_spawnweapon(world::gamemode, world::mutators);
			switch(entities::ents[proj.id]->type)
			{
				case WEAPON:
				{
					int attr = weapattr(entities::ents[proj.id]->attr[0], sweap);
					if(isweap(attr) && !d->hasweap(attr, sweap))
					{ // go get a weapon upgrade
						if(proj.owner == d) break;
						interest &n = interests.add();
						n.state = AI_S_INTEREST;
						n.node = entities::closestent(WAYPOINT, proj.o, NEARDIST, true);
						n.target = proj.id;
						n.targtype = AI_T_DROP;
						n.score = pos.squaredist(proj.o)/(force || attr == d->ai->weappref ? 100.f : 1.f);
					}
					break;
				}
				default: break;
			}
		}
	}

	bool find(gameent *d, aistate &b, bool override = false)
	{
		static vector<interest> interests;
		interests.setsizenodelete(0);
		if(!d->hasweap(d->ai->weappref, m_spawnweapon(world::gamemode, world::mutators)))
			items(d, b, interests);
		if(m_ctf(world::gamemode)) ctf::aifind(d, b, interests);
		if(m_stf(world::gamemode)) stf::aifind(d, b, interests);
		if(m_team(world::gamemode, world::mutators)) assist(d, b, interests);
		while(!interests.empty())
		{
			int q = interests.length()-1;
			loopi(interests.length()-1) if(interests[i].score < interests[q].score) q = i;
			interest n = interests.removeunordered(q);
			bool proceed = true;
			static vector<int> targets;
			targets.setsizenodelete(0);
			switch(n.state)
			{
				case AI_S_DEFEND: // don't get into herds
					proceed = !checkothers(targets, d, n.state, n.targtype, n.target, true);
					break;
				default: break;
			}
			if(proceed && makeroute(d, b, n.node, false))
			{
				d->ai->setstate(n.state, n.targtype, n.target, override);
				return true;
			}
		}
		return false;
	}

	void damaged(gameent *d, gameent *e, int weap, int flags, int damage, int health, int millis, vec &dir)
	{
		if(d->ai && world::allowmove(d) && targetable(d, e, true)) // see if this ai is interested in a grudge
		{
			aistate &b = d->ai->getstate();
			if(violence(d, b, e, false)) return;
		}
		static vector<int> targets; // check if one of our ai is defending them
		targets.setsizenodelete(0);
		if(checkothers(targets, d, AI_S_DEFEND, AI_T_PLAYER, d->clientnum, true))
		{
			gameent *t;
			loopv(targets) if((t = world::getclient(targets[i])) && t->ai && world::allowmove(t) && targetable(t, e, true))
			{
				aistate &c = t->ai->getstate();
				if(violence(t, c, e, false)) return;
			}
		}
	}

	void setup(gameent *d, bool tryreset = false)
	{
		d->ai->reset(tryreset);
		aistate &b = d->ai->getstate();
		b.next = lastmillis+((111-d->skill)*10)+rnd((111-d->skill)*10);
		if(m_noitems(world::gamemode, world::mutators))
			d->ai->weappref = m_spawnweapon(world::gamemode, world::mutators);
		else while(true)
		{
			d->ai->weappref = rnd(WEAPON_TOTAL);
			if(d->ai->weappref != WEAPON_PLASMA || !rnd(d->skill)) break;
		}
	}

	void spawned(gameent *d)
	{
		if(d->ai) setup(d, false);
	}

	void killed(gameent *d, gameent *e, int weap, int flags, int damage)
	{
		if(d->ai) d->ai->reset();
	}

	bool check(gameent *d, aistate &b)
	{
		if(m_ctf(world::gamemode) && ctf::aicheck(d, b)) return true;
		if(m_stf(world::gamemode) && stf::aicheck(d, b)) return true;
		return false;
	}

	int dowait(gameent *d, aistate &b)
	{
		if(check(d, b)) return 1;
		if(find(d, b)) return 1;
		if(target(d, b, true, true)) return 1;
		if(randomnode(d, b, NEARDIST, 1e16f))
		{
			d->ai->addstate(AI_S_INTEREST, AI_T_NODE, d->ai->route[0]);
			return 1;
		}
		return 0; // but don't pop the state
	}

	int dodefend(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			switch(b.targtype)
			{
				case AI_T_NODE:
				case AI_T_ENTITY:
				{
					if(check(d, b)) return 1;
					if(entities::ents.inrange(b.target))
					{
						gameentity &e = *(gameentity *)entities::ents[b.target];
						return defend(d, b, e.o) ? 1 : 0;
					}
					break;
				}
				case AI_T_AFFINITY:
				{
					if(m_ctf(world::gamemode)) return ctf::aidefend(d, b) ? 1 : 0;
					if(m_stf(world::gamemode)) return stf::aidefend(d, b) ? 1 : 0;
					break;
				}
				case AI_T_PLAYER:
				{
					gameent *e = world::getclient(b.target);
					if(e && e->state == CS_ALIVE) return defend(d, b, world::feetpos(e)) ? 1 : 0;
					break;
				}
				default: break;
			}
		}
		return 0;
	}

	int dointerest(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			int sweap = m_spawnweapon(world::gamemode, world::mutators);
			switch(b.targtype)
			{
				case AI_T_NODE:
				{
					return !d->ai->route.empty() ? 1 : 0;
					break;
				}
				case AI_T_ENTITY:
				{
					if(d->hasweap(d->ai->weappref, sweap)) return 0;
					if(entities::ents.inrange(b.target))
					{
						gameentity &e = *(gameentity *)entities::ents[b.target];
						if(enttype[e.type].usetype != EU_ITEM) return 0;
						int attr = weapattr(e.attr[0], sweap);
						switch(e.type)
						{
							case WEAPON:
							{
								if(!e.spawned || d->hasweap(attr, sweap)) return 0;
								break;
							}
							default: break;
						}
						return makeroute(d, b, e.o) ? 1 : 0;
					}
					break;
				}
				case AI_T_DROP:
				{
					if(d->hasweap(d->ai->weappref, sweap)) return 0;
					loopvj(projs::projs) if(projs::projs[j]->projtype == PRJ_ENT && projs::projs[j]->ready() && projs::projs[j]->id == b.target)
					{
						projent &proj = *projs::projs[j];
						if(!entities::ents.inrange(proj.id) || enttype[entities::ents[proj.id]->type].usetype != EU_ITEM) return 0;
						int attr = weapattr(entities::ents[proj.id]->attr[0], sweap);
						switch(entities::ents[proj.id]->type)
						{
							case WEAPON:
							{
								if(d->hasweap(attr, sweap) || proj.owner == d) return 0;
								break;
							}
							default: break;
						}
						return makeroute(d, b, proj.o) ? 1 : 0;
						break;
					}
					break;
				}
				default: break;
			}
		}
		return 0;
	}

	int dopursue(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			switch(b.targtype)
			{
				case AI_T_AFFINITY:
				{
					if(m_ctf(world::gamemode)) return ctf::aipursue(d, b) ? 1 : 0;
					if(m_stf(world::gamemode)) return stf::aipursue(d, b) ? 1 : 0;
					break;
				}

				case AI_T_PLAYER:
				{
					gameent *e = world::getclient(b.target);
					if(e && e->state == CS_ALIVE) return patrol(d, b, world::feetpos(e)) ? 1 : 0;
					break;
				}
				default: break;
			}
		}
		return 0;
	}

	int closenode(gameent *d, bool force = false)
	{
		vec pos = world::feetpos(d);
		int node = -1;
		float mindist = NEARDISTSQ;
		loopv(d->ai->route) if(entities::ents.inrange(d->ai->route[i]) && (force || (d->ai->route[i] != d->lastnode && d->ai->route[i] != d->ai->lastnode && d->ai->route[i] != d->ai->prevnode)))
		{
			gameentity &e = *(gameentity *)entities::ents[d->ai->route[i]];
			vec epos = e.o;
			int entid = obstacles.remap(d, d->ai->route[i], epos);
			if(entities::ents.inrange(entid) && (force || entid == d->ai->route[i] || (entid != d->ai->lastnode && entid != d->ai->prevnode)))
			{
				float dist = epos.squaredist(pos);
				if(dist < mindist)
				{
					node = entid;
					mindist = dist;
				}
			}
		}
		return node;
	}

	bool entspot(gameent *d, int n, bool force = false)
	{
		if(entities::ents.inrange(n))
		{
			gameentity &e = *(gameentity *)entities::ents[n];
			vec epos = e.o;
			int entid = obstacles.remap(d, n, epos);
			if(entities::ents.inrange(entid) && (force || entid == n || (entid != d->ai->lastnode && entid != d->ai->prevnode)))
			{
				d->ai->spot = epos;
				if(((e.attr[0] & WP_CROUCH && !d->crouching) || d->crouching) && (lastmillis-d->crouchtime >= 500))
				{
					d->crouching = !d->crouching;
					d->crouchtime = lastmillis;
				}
				return true;
			}
		}
		return false;
	}

	bool anynode(gameent *d, aistate &b, bool retry = false)
	{
		if(entities::ents.inrange(d->lastnode))
		{
			gameentity &e = *(gameentity *)entities::ents[d->lastnode];
			vec dir = vec(e.o).sub(world::feetpos(d));
			if(d->timeinair || dir.magnitude() <= CLOSEDIST || retry)
			{
				static vector<int> anyremap; anyremap.setsizenodelete(0);
				gameentity &e = *(gameentity *)entities::ents[d->lastnode];
				if(!e.links.empty())
				{
					loopv(e.links) if(entities::ents.inrange(e.links[i]) && e.links[i] != d->lastnode && (retry || (e.links[i] != d->ai->lastnode && e.links[i] != d->ai->prevnode)))
						anyremap.add(e.links[i]);
				}
				while(!anyremap.empty())
				{
					int r = rnd(anyremap.length()), t = anyremap[r];
					if(entspot(d, t, retry)) return true;
					anyremap.remove(r);
				}
			}
			if(!retry)
			{
				if(entspot(d, d->lastnode, true)) return true;
				return anynode(d, b, true);
			}
		}
		return false;
	}

	bool hunt(gameent *d, aistate &b, int retries = 0)
	{
		if(!d->ai->route.empty())
		{
			bool alternate = retries%2 ? true : false;
			int n = alternate ? closenode(d, retries == 3) : d->ai->route.find(d->lastnode);
			if(!alternate && d->ai->route.inrange(n))
			{ // waka-waka-waka-waka
				while(d->ai->route.length() > n+2) d->ai->route.pop(); // all but this and the last node
				if(!n)
				{
					if(entspot(d, d->ai->route[n], retries > 1))
					{
						if(vec(d->ai->spot).sub(world::feetpos(d)).magnitude() <= CLOSEDIST)
						{
							d->ai->dontmove = true;
							d->ai->route.setsize(0);
						}
						return true; // this is our goal?
					}
					else return anynode(d, b);
				}
				else n--; // otherwise, we want the next in line
			}
			if(d->ai->route.inrange(n) && entspot(d, d->ai->route[n], retries > 1)) return true;
			if(retries < 3) return hunt(d, b, retries+1); // try again
			d->ai->route.setsize(0);
		}
		b.override = false;
		return anynode(d, b);
	}

	bool hastarget(gameent *d, aistate &b, gameent *e)
	{ // add margins of error
		if(d->skill <= 100 && !rnd(d->skill*10)) return true; // random margin of error
		vec dp = world::headpos(d), ep = getaimpos(d, e);
		gameent *h = world::intersectclosest(dp, d->ai->target, d);
		if(h && !targetable(d, h, true)) return false;
		float targyaw, targpitch, mindist = d->radius*d->radius, dist = dp.squaredist(ep);
		if(weaptype[d->weapselect].explode) mindist = weaptype[d->weapselect].explode*weaptype[d->weapselect].explode;
		if(mindist <= dist)
		{
			if(d->skill > 100 && h) return true;
			vec dir = vec(dp).sub(ep).normalize();
			vectoyawpitch(dir, targyaw, targpitch);
			float rtime = (d->skill*weaptype[d->weapselect].rdelay/2000.f)+(d->skill*weaptype[d->weapselect].adelay/200.f),
					skew = clamp(float(lastmillis-d->ai->enemymillis)/float(rtime), 0.f, d->weapselect == WEAPON_GL ? 1.f : 1e16f),
						cyaw = fabs(targyaw-d->yaw), cpitch = fabs(targpitch-d->pitch);
			if(cyaw <= d->ai->views[0]*skew && cpitch <= d->ai->views[1]*skew) return true;
		}
		return false;
	}

	void jumpto(gameent *d, aistate &b, const vec &pos, bool cando = true)
	{
		vec off = vec(pos).sub(world::feetpos(d)), dir(off.x, off.y, 0);
		bool offground = (d->timeinair && !d->inliquid && !d->onladder), jumper = off.z >= JUMPMIN,
			jump = jumper || d->onladder || lastmillis >= d->ai->jumprand,
			propeller = dir.magnitude() >= JUMPMIN, propel = jumper || propeller;
		if(propel && (!offground || !cando || lastmillis < d->ai->propelseed || !physics::canimpulse(d)))
			propel = false;
		if(jump)
		{
			if(offground || lastmillis < d->ai->jumpseed) jump = false;
			else
			{
				vec old = d->o;
				d->o = vec(pos).add(vec(0, 0, d->height));
				if(!collide(d, vec(0, 0, 1))) jump = false;
				d->o = old;
			}
		}
		if(jump || propel)
		{
			d->jumping = true;
			d->jumptime = lastmillis;
			if(jump && !d->onladder && !propeller) d->ai->dontmove = true; // going up
			int seed = 111-d->skill;
			if(!d->onladder) seed *= 10;
			d->ai->propelseed = lastmillis+seed+rnd(seed);
			if(jump) d->ai->jumpseed = d->ai->propelseed;
			if(!d->onladder) seed *= 10;
			d->ai->jumprand = lastmillis+seed+rnd(seed*10);
		}
	}

	int process(gameent *d, aistate &b)
	{
		int result = 0, stupify = d->skill <= 30+rnd(20) ? rnd(d->skill*1111) : 0, skmod = (111-d->skill)*10;
		float frame = float(lastmillis-d->lastupdate)/float(skmod);
		vec dp = world::headpos(d);
		if(b.idle || (stupify && stupify <= skmod))
		{
			d->ai->lastaction = d->ai->lasthunt = lastmillis;
			d->ai->dontmove = b.idle || (stupify && rnd(stupify) <= stupify/4);
			if(b.idle == 2 || (stupify && stupify <= skmod/8))
				jumpto(d, b, dp, !rnd(d->skill*10)); // jump up and down
		}
		else if(hunt(d, b))
		{
			jumpto(d, b, d->ai->spot);
			world::getyawpitch(dp, vec(d->ai->spot).add(vec(0, 0, d->height)), d->ai->targyaw, d->ai->targpitch);
			d->ai->lasthunt = lastmillis;
		}
		else if(d->ai->route.empty()) d->ai->dontmove = true;

		gameent *e = world::getclient(d->ai->enemy);
		if(d->skill > 90 && (!e || !targetable(d, e, true))) e = world::intersectclosest(dp, d->ai->target, d);
		if(e && targetable(d, e, true))
		{
			vec ep = getaimpos(d, e);
			bool insight = cansee(d, dp, ep), hasseen = d->ai->enemyseen && lastmillis-d->ai->enemyseen <= (d->skill*50)+1000,
				quick = d->ai->enemyseen && lastmillis-d->ai->enemyseen <= skmod;
			if(insight) d->ai->enemyseen = lastmillis;
			if(b.idle || insight || hasseen)
			{
				float yaw, pitch;
				world::getyawpitch(dp, ep, yaw, pitch);
				world::fixrange(yaw, pitch);
				float sskew = (insight ? 1.25f : (hasseen ? 0.75f : 0.5f))*((insight || hasseen) && (d->jumping || d->timeinair) ? 1.25f : 1.f);
				if(b.idle)
				{
					d->ai->targyaw = yaw;
					d->ai->targpitch = pitch;
					if(!insight) frame /= 3.f;
				}
				else if(!insight) frame /= 2.f;
				world::scaleyawpitch(d->yaw, d->pitch, yaw, pitch, frame, sskew);
				if(insight || quick)
				{
					if(physics::issolid(e) && d->canshoot(d->weapselect, m_spawnweapon(world::gamemode, world::mutators), lastmillis) && hastarget(d, b, e))
					{
						d->attacking = true;
						d->ai->lastaction = d->attacktime = lastmillis;
						result = 4;
					}
					else result = insight ? 3 : 2;
				}
				else
				{
					if(!b.idle && !hasseen) noenemy(d);
					result = hasseen ? 2 : 1;
				}
			}
			else
			{
				noenemy(d);
				result = 0;
			}
		}
		else
		{
			noenemy(d);
			result = 0;
		}

		world::fixrange(d->ai->targyaw, d->ai->targpitch);
		d->aimyaw = d->ai->targyaw; d->aimpitch = d->ai->targpitch;
		if(!result) world::scaleyawpitch(d->yaw, d->pitch, d->ai->targyaw, d->ai->targpitch, frame, 0.5f);

		if(!d->ai->dontmove)
		{ // our guys move one way.. but turn another?! :)
			const struct aimdir { int move, strafe, offset; } aimdirs[8] =
			{
				{  1,  0,   0 },
				{  1,  -1,  45 },
				{  0,  -1,  90 },
				{ -1,  -1, 135 },
				{ -1,  0, 180 },
				{ -1, 1, 225 },
				{  0, 1, 270 },
				{  1, 1, 315 }
			};
			float yaw = d->aimyaw-d->yaw;
			while(yaw < 0.0f) yaw += 360.0f;
			while(yaw >= 360.0f) yaw -= 360.0f;
			int r = clamp(((int)floor((yaw+22.5f)/45.0f))&7, 0, 7);
			const aimdir &ad = aimdirs[r];
			d->move = ad.move;
			d->strafe = ad.strafe;
			d->aimyaw -= ad.offset;
			world::fixrange(d->aimyaw, d->aimpitch);
		}
		else d->move = d->strafe = 0;
		d->ai->dontmove = false;
		return result;
	}

	bool request(gameent *d, aistate &b)
	{
		int busy = process(d, b), sweap = m_spawnweapon(world::gamemode, world::mutators);
		if(busy <= 1 && !m_noitems(world::gamemode, world::mutators) && d->reqswitch < 0 && b.type == AI_S_DEFEND && b.idle)
		{
			loopirev(WEAPON_MAX) if(i != WEAPON_GL && i != d->ai->weappref && i != d->weapselect && entities::ents.inrange(d->entid[i]))
			{
				client::addmsg(SV_DROP, "ri3", d->clientnum, lastmillis-world::maptime, i);
				d->ai->lastaction = d->reqswitch = lastmillis;
				break;
			}
		}
		if(world::allowmove(d) && busy <= (d->hasweap(d->ai->weappref, sweap) ? 0 : 2) && !d->useaction && d->requse < 0)
		{
			static vector<actitem> actitems;
			actitems.setsizenodelete(0);
			if(entities::collateitems(d, actitems))
			{
				actitem &t = actitems.last();
				int ent = -1;
				switch(t.type)
				{
					case ITEM_ENT:
					{
						if(!entities::ents.inrange(t.target)) break;
						extentity &e = *entities::ents[t.target];
						if(enttype[e.type].usetype != EU_ITEM) break;
						ent = t.target;
						break;
					}
					case ITEM_PROJ:
					{
						if(!projs::projs.inrange(t.target)) break;
						projent &proj = *projs::projs[t.target];
						if(!entities::ents.inrange(proj.id)) break;
						extentity &e = *entities::ents[proj.id];
						if(enttype[e.type].usetype != EU_ITEM || proj.owner == d) break;
						ent = proj.id;
						break;
					}
					default: break;
				}
				if(entities::ents.inrange(ent))
				{
					extentity &e = *entities::ents[ent];
					if(d->canuse(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], sweap, lastmillis)) switch(e.type)
					{
						case WEAPON:
						{
							int attr = weapattr(e.attr[0], sweap);
							if(d->hasweap(attr, sweap)) break;
							d->useaction = true;
							d->ai->lastaction = d->usetime = lastmillis;
							return true;
							break;
						}
						default: break;
					}
				}
			}
		}

		if(busy <= (!d->hasweap(d->weapselect, sweap) ? 2 : 1) && d->reqswitch < 0)
		{
			int weap = -1;
			if(d->hasweap(d->ai->weappref, sweap)) weap = d->ai->weappref; // could be any weap
			else loopi(WEAPON_MAX) if(d->hasweap(i, sweap, 1)) weap = i; // only choose carriables here
			if(weap != d->weapselect && d->canswitch(weap, sweap, lastmillis))
			{
				client::addmsg(SV_WEAPSELECT, "ri3", d->clientnum, lastmillis-world::maptime, weap);
				d->ai->lastaction = d->reqswitch = lastmillis;
				return true;
			}
		}

		if(d->hasweap(d->weapselect, sweap) && busy <= (!d->ammo[d->weapselect] ? 3 : 1) && d->canreload(d->weapselect, sweap, lastmillis) && d->reqreload < 0)
		{
			client::addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-world::maptime, d->weapselect);
			d->ai->lastaction = d->reqreload = lastmillis;
			return true;
		}

		return busy >= 2;
	}

	void logic(gameent *d, aistate &b, bool run)
	{
		vec dp = world::headpos(d);
		findorientation(dp, d->yaw, d->pitch, d->ai->target);
		bool allowmove = world::allowmove(d) && b.type != AI_S_WAIT;
		if(d->state != CS_ALIVE || !allowmove) d->stopmoving(true);
		if(d->state == CS_ALIVE)
		{
			if(allowmove)
			{
				if(!request(d, b)) target(d, b, false, b.idle ? true : false);
				weapons::shoot(d, d->ai->target);
				if(d->ai->lasthunt)
				{
					int millis = lastmillis-d->ai->lasthunt;
					if(millis < 5000) d->ai->tryreset = false;
					else if(millis < 10000)
					{
						if(!d->ai->tryreset) setup(d, true);
					}
					else
					{
						if(d->ai->tryreset)
						{
							world::suicide(d, HIT_LOST); // better off doing something than nothing
							d->ai->reset(false);
						}
					}
				}
				if(d->ai->prevnode != d->lastnode)
				{
					if(d->ai->lastnode != d->ai->prevnode)
						d->ai->lastnode = d->ai->prevnode;
					d->ai->prevnode = d->lastnode;
				}
			}
			entities::checkitems(d);
		}
        if((d->state == CS_DEAD || d->state == CS_WAITING) && d->ragdoll) moveragdoll(d, false);
		else
        {
            if(d->ragdoll) cleanragdoll(d);
            physics::move(d, 1, true);
        }
		d->attacking = d->jumping = d->reloading = d->useaction = false;
		if(run) d->lastupdate = lastmillis;
	}

	void avoid()
	{
		// guess as to the radius of ai and other critters relying on the avoid set for now
		float guessradius = world::player1->radius;
		obstacles.clear();
		loopi(world::numdynents())
		{
			gameent *d = (gameent *)world::iterdynents(i);
			if(!d || d->state != CS_ALIVE || !physics::issolid(d)) continue;
			vec pos = world::feetpos(d);
			float limit = guessradius+d->radius;
			limit *= limit; // square it to avoid expensive square roots
			loopvk(entities::ents)
			{
				gameentity &e = *(gameentity *)entities::ents[k];
				if(e.type == WAYPOINT && e.o.squaredist(pos) <= limit)
					obstacles.add(d, k);
			}
		}
		loopv(projs::projs)
		{
			projent *p = projs::projs[i];
			if(p && p->state == CS_ALIVE && p->projtype == PRJ_SHOT && weaptype[p->weap].explode)
			{
				float limit = guessradius+(weaptype[p->weap].explode*p->lifesize);
				limit *= limit; // square it to avoid expensive square roots
				loopvk(entities::ents)
				{
					gameentity &e = *(gameentity *)entities::ents[k];
					if(e.type == WAYPOINT && e.o.squaredist(p->o) <= limit)
						obstacles.add(p, k);
				}
			}
		}
	}

	void think(gameent *d, bool run)
	{
		// the state stack works like a chain of commands, certain commands simply replace each other
		// others spawn new commands to the stack the ai reads the top command from the stack and executes
		// it or pops the stack and goes back along the history until it finds a suitable command to execute
		if(d->ai->state.empty()) setup(d);
		bool cleannext = false;
		loopvrev(d->ai->state)
		{
			aistate &c = d->ai->state[i];
			if(cleannext)
			{
				c.millis = lastmillis;
				c.override = false;
				cleannext = false;
			}
			if(d->state == CS_DEAD && (d->respawned < 0 || lastmillis-d->respawned > 5000) && (!d->lastdeath || lastmillis-d->lastdeath > 500))
			{
				client::addmsg(SV_TRYSPAWN, "ri", d->clientnum);
				d->respawned = lastmillis;
			}
			else if(d->state == CS_ALIVE && run && lastmillis >= c.next)
			{
				int result = 0;
				c.idle = 0;
				switch(c.type)
				{
					case AI_S_WAIT: result = dowait(d, c); break;
					case AI_S_DEFEND: result = dodefend(d, c); break;
					case AI_S_PURSUE: result = dopursue(d, c); break;
					case AI_S_INTEREST: result = dointerest(d, c); break;
					default: result = 0; break;
				}
				if(result <= 0)
				{
					d->ai->route.setsize(0);
					if(c.type != AI_S_WAIT)
					{
						d->ai->removestate(i);
						switch(result)
						{
							case 0: default: cleannext = true; break;
							case -1: i = d->ai->state.length()-1; break;
						}
					}
					else
					{
						c.next = lastmillis+1000;
						d->ai->dontmove = true;
					}
					continue; // shouldn't interfere
				}
			}
			logic(d, c, run);
			break;
		}
		if(d->ai->clear) d->ai->wipe();
	}

	void drawstate(gameent *d, aistate &b, bool top, int above)
	{
		const char *bnames[AI_S_MAX] = {
			"wait", "defend", "pursue", "interest"
		}, *btypes[AI_T_MAX+1] = {
			"none", "node", "player", "affinity", "entity", "drop"
		};
		string s;
		if(top)
		{
			s_sprintf(s)("@\fg%s (%d[%d]) goal: %s:%d (%d[%d])",
				bnames[b.type],
				lastmillis-b.millis, b.next-lastmillis,
				btypes[b.targtype+1], b.target,
				!d->ai->route.empty() ? d->ai->route[0] : -1,
				d->ai->route.length()
			);
		}
		else
		{
			s_sprintf(s)("@\fy%s (%d[%d]) goal: %s:%d",
				bnames[b.type],
				lastmillis-b.millis, b.next-lastmillis,
				btypes[b.targtype+1], b.target
			);
		}
		part_text(vec(world::abovehead(d)).add(vec(0, 0, above)), s);
	}

	void drawroute(gameent *d, aistate &b, float amt = 1.f)
	{
		renderprimitive(true);
		int colour = teamtype[d->team].colour, last = -1;
		float cr = (colour>>16)/255.f, cg = ((colour>>8)&0xFF)/255.f, cb = (colour&0xFF)/255.f;

		loopvrev(d->ai->route)
		{
			if(d->ai->route.inrange(last))
			{
				int index = d->ai->route[i], prev = d->ai->route[last];
				if(entities::ents.inrange(index) && entities::ents.inrange(prev))
				{
					gameentity &e = *(gameentity *)entities::ents[index],
						&f = *(gameentity *)entities::ents[prev];
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
		if(aidebug > 4)
		{
			vec pos = world::feetpos(d);
			if(d->ai->spot != vec(0, 0, 0)) renderline(pos, d->ai->spot, 1.f, 1.f, 1.f, false);
			if(entities::ents.inrange(d->lastnode))
				renderline(pos, entities::ents[d->lastnode]->o, 0.f, 1.f, 1.f, false);
			if(entities::ents.inrange(d->ai->lastnode))
				renderline(pos, entities::ents[d->ai->lastnode]->o, 1.f, 0.f, 1.f, false);
		}
		renderprimitive(false);
	}

	void render()
	{
		if(aidebug > 1)
		{
			int amt[2] = { 0, 0 };
			loopv(world::players) if(world::players[i] && world::players[i]->ai) amt[0]++;
			loopv(world::players) if(world::players[i] && world::players[i]->state == CS_ALIVE && world::players[i]->ai)
			{
				gameent *d = world::players[i];
				bool top = true;
				int above = 0;
				amt[1]++;
				loopvrev(d->ai->state)
				{
					aistate &b = d->ai->state[i];
					drawstate(d, b, top, above += 2);
					if(aidebug > 3 && top && rendernormally && b.type != AI_S_WAIT)
						drawroute(d, b, float(amt[1])/float(amt[0]));
					if(top)
					{
						if(aidebug > 2) top = false;
						else break;
					}
				}
			}
		}
	}
}
