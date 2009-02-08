#include "pch.h"
#include "engine.h"
#include "game.h"
namespace ai
{
	entities::avoidset obstacles;
    int avoidmillis = 0, updatemillis = 0;

	VAR(aidebug, 0, 0, 5);

	ICOMMAND(addbot, "s", (char *s), client::addmsg(SV_ADDBOT, "ri", *s ? clamp(atoi(s), 1, 100) : -1));
	ICOMMAND(delbot, "", (), client::addmsg(SV_DELBOT, "r"));

	void create(gameent *d)
	{
		if(!d->ai)
		{
			if((d->ai = new aiinfo()) == NULL)
				fatal("could not create ai");
		}
	}

	void destroy(gameent *d)
	{
		if(d->ai) DELETEP(d->ai);
	}

	void init(gameent *d, int at, int on, int sk, int bn, char *name, int tm)
	{
		gameent *o = world::getclient(on);
		string m;
		if(o) s_strcpy(m, world::colorname(o));
		else s_sprintf(m)("\fs\fwunknown [\fs\fr%d\fS]\fS", on);
		bool resetthisguy = false;
		if(!d->name[0])
		{
			conoutf("\fg%s assigned to %s at skill %d", world::colorname(d, name), m, sk);
			resetthisguy = true;
		}
		else if(d->ownernum != on)
		{
			conoutf("\fg%s reassigned to %s", world::colorname(d, name), m);
			resetthisguy = true;
		}
		else if(d->skill != sk) conoutf("\fg%s changed skill to %d", world::colorname(d, name), sk);
		else if(d->team != tm) conoutf("\fg%s switched to \fs%s%s\fS team", world::colorname(d, name), teamtype[tm].chat, teamtype[tm].name);

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
		if(lastmillis-avoidmillis > 250) avoid();
		bool updating = lastmillis-updatemillis > 40; // about 25fps
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

	bool makeroute(gameent *d, aistate &b, int node, float tolerance, bool retry)
	{
		if(entities::route(d, d->lastnode, node, d->ai->route, obstacles, tolerance, retry))
		{
			b.override = false;
			return true;
		}
		else if(!retry) return makeroute(d, b, node, tolerance, true);
		d->ai->route.setsize(0);
		return false;
	}

	bool makeroute(gameent *d, aistate &b, const vec &pos, float tolerance, bool dist)
	{
		int node = entities::entitynode(pos);
		return makeroute(d, b, node, tolerance);
	}

	bool randomnode(gameent *d, aistate &b, const vec &from, const vec &to, float radius, float wander)
	{
		static vector<int> entities;
		entities.setsizenodelete(0);
		float r = radius*radius, w = wander*wander;
		loopvj(entities::ents) if(entities::ents[j]->type == WAYPOINT && j != d->lastnode && !obstacles.find(j, d))
		{
			float fdist = entities::ents[j]->o.squaredist(from);
			if(fdist <= w)
			{
				float tdist = entities::ents[j]->o.squaredist(to);
				if(tdist > r && fdist > r) entities.add(j);
			}
		}

		while(!entities.empty())
		{
			int w = rnd(entities.length()), n = entities.removeunordered(w);
			if(makeroute(d, b, n)) return true;
		}
		return false;
	}

	bool randomnode(gameent *d, aistate &b, float radius, float wander)
	{
		vec feet = world::feetpos(d);
		return randomnode(d, b, feet, feet, radius, wander);
	}

	bool enemy(gameent *d, aistate &b, const vec &pos, float radius)
	{
		if(world::allowmove(d))
		{
			gameent *t = NULL, *e = NULL;
			vec targ, dp = world::headpos(d), tp = vec(0, 0, 0);
			bool cansee = false;
			loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && e != d && AITARG(d, e, true))
			{
				vec ep = world::headpos(e), ef = world::feetpos(e);
				if(ef.squaredist(pos) <= radius*radius && (!t || ep.squaredist(dp) < tp.squaredist(dp)))
				{
					bool see = AICANSEE(dp, ep, d);
					if(!cansee || see)
					{
						t = e;
						tp = ep;
						cansee = see;
					}
				}
			}
			if(t) return violence(d, b, t, false);
		}
		return false;
	}

	bool patrol(gameent *d, aistate &b, const vec &pos, float radius, float wander, bool walk, bool retry)
	{
		vec feet = world::feetpos(d);
		if(walk || feet.squaredist(pos) <= radius*radius || !makeroute(d, b, pos, radius, wander))
		{ // run away and back to keep ourselves busy
			if(!b.override && randomnode(d, b, feet, pos, radius, wander))
			{
				b.override = true;
				return true;
			}
			else if(!d->ai->route.empty()) return true;
			else if(!retry)
			{
                b.override = false;
				return patrol(d, b, pos, radius, wander, walk, true);
			}
			return false;
		}
		b.override = false;
		return true;
	}

	bool defend(gameent *d, aistate &b, const vec &pos, float radius, float guard, bool walk)
	{
		bool gowalk = enemy(d, b, pos, guard) || walk;
		vec feet = world::feetpos(d);
		if(!gowalk && feet.squaredist(pos) <= radius*radius)
		{
			b.idle = true;
			return true;
		}
		return patrol(d, b, pos, radius, guard, gowalk);
	}

	bool violence(gameent *d, aistate &b, gameent *e, bool pursue)
	{
		if(AITARG(d, e, true))
		{
			if(pursue)
				d->ai->addstate(AI_S_PURSUE, AI_T_PLAYER, e->clientnum);
			d->ai->enemy = e->clientnum;
			d->ai->lastseen = lastmillis;
			return true;
		}
		return false;
	}

	bool target(gameent *d, aistate &b, bool pursue = false, bool force = false)
	{
		gameent *t = NULL, *e = NULL;
		vec targ, dp = world::headpos(d), tp = vec(0, 0, 0);
		loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && e != d && AITARG(d, e, true))
		{
			vec ep = world::headpos(e);
			if((!t || ep.squaredist(dp) < tp.squaredist(dp)) && (force || AICANSEE(dp, ep, d)))
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
		vec dp = world::headpos(d);
		loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && e != d && (all || e->aitype == AI_NONE) && d->team == e->team)
		{
			vec ep = world::headpos(e);
			interest &n = interests.add();
			n.state = AI_S_DEFEND;
			n.node = e->lastnode;
			n.target = e->clientnum;
			n.targtype = AI_T_PLAYER;
			n.tolerance = e->radius*2.f;
			n.score = ep.squaredist(dp)/(force || d->hasweap(d->ai->weappref, m_spawnweapon(world::gamemode, world::mutators)) ? 10.f : 1.f);
		}
	}

	void items(gameent *d, aistate &b, vector<interest> &interests, bool force = false)
	{
		vec pos = world::headpos(d);
		loopvj(entities::ents)
		{
			gameentity &e = *(gameentity *)entities::ents[j];
			if(enttype[e.type].usetype != EU_ITEM) continue;
			int sweap = m_spawnweapon(world::gamemode, world::mutators), attr = weapattr(e.attr[0], sweap);
			switch(e.type)
			{
				case WEAPON:
				{
					if(e.spawned && isweap(attr) && !d->hasweap(attr, sweap))
					{ // go get a weapon upgrade
						interest &n = interests.add();
						n.state = AI_S_INTEREST;
						n.node = entities::entitynode(e.o);
						n.target = j;
						n.targtype = AI_T_ENTITY;
						n.tolerance = enttype[e.type].radius+d->radius;
						n.score = pos.squaredist(e.o)/(force || attr == d->ai->weappref ? 10.f : 1.f);
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
			int sweap = m_spawnweapon(world::gamemode, world::mutators), attr = weapattr(entities::ents[proj.id]->attr[0], sweap);
			switch(entities::ents[proj.id]->type)
			{
				case WEAPON:
				{
					if(isweap(attr) && !d->hasweap(attr, sweap))
					{ // go get a weapon upgrade
						if(proj.owner == d) break;
						interest &n = interests.add();
						n.state = AI_S_INTEREST;
						n.node = entities::entitynode(proj.o);
						n.target = proj.id;
						n.targtype = AI_T_DROP;
						n.tolerance = enttype[WEAPON].radius+d->radius;
						n.score = pos.squaredist(proj.o)/(force || attr == d->ai->weappref ? 10.f : 1.f);
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
		if(m_ctf(world::gamemode)) ctf::aifind(d, b, interests);
		if(m_stf(world::gamemode)) stf::aifind(d, b, interests);
		if(m_team(world::gamemode, world::mutators)) assist(d, b, interests);
		if(!d->hasweap(d->ai->weappref, m_spawnweapon(world::gamemode, world::mutators)))
			items(d, b, interests, false);
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
			if(proceed && makeroute(d, b, n.node, n.tolerance))
			{
				if(override) d->ai->setstate(n.state, n.targtype, n.target);
				else d->ai->addstate(n.state, n.targtype, n.target);
				return true;
			}
		}
		return false;
	}

	void damaged(gameent *d, gameent *e, int weap, int flags, int damage, int health, int millis, vec &dir)
	{
		if(d->ai && world::allowmove(d) && AITARG(d, e, true)) // see if this ai is interested in a grudge
		{
			aistate &b = d->ai->getstate();
			if(violence(d, b, e, false)) return;
		}
		static vector<int> targets; // check if one of our ai is defending them
		targets.setsizenodelete(0);
		if(checkothers(targets, d, AI_S_DEFEND, AI_T_PLAYER, d->clientnum, true))
		{
			gameent *t;
			loopv(targets) if((t = world::getclient(targets[i])) && t->ai && world::allowmove(t) && AITARG(t, e, true))
			{
				aistate &c = t->ai->getstate();
				if(violence(t, c, e, false)) return;
			}
		}
	}

	void spawned(gameent *d)
	{
		if(d->ai) d->ai->reset();
	}

	void killed(gameent *d, gameent *e, int weap, int flags, int damage)
	{
		if(d->ai)
		{
			d->ai->reset();
			aistate &b = d->ai->getstate();
			b.next = lastmillis + 500 + rnd((101-d->skill)*10);
		}
	}

	bool check(gameent *d, aistate &b)
	{
		if(m_ctf(world::gamemode) && ctf::aicheck(d, b)) return true;
		if(m_stf(world::gamemode) && stf::aicheck(d, b)) return true;
		return false;
	}

	bool dowait(gameent *d, aistate &b)
	{
		if(d->state == CS_WAITING) return true; // just wait my precious..
		else if(d->state == CS_DEAD)
		{
			if(d->respawned != d->lifesequence && !d->respawnwait(lastmillis, m_spawndelay(world::gamemode, world::mutators)))
				world::respawn(d);
			return true;
		}
		else if(d->state == CS_ALIVE)
		{
			if(d->timeinair && entities::ents.inrange(d->lastnode))
			{ // we need to make a quick decision to find a landing spot
				int closest = -1;
				vec dp = world::feetpos(d);
				gameentity &e = *(gameentity *)entities::ents[d->lastnode];
				if(!e.links.empty())
				{
					loopv(e.links) if(entities::ents.inrange(e.links[i]))
					{
						gameentity &f = *(gameentity *)entities::ents[e.links[i]];
						if(!entities::ents.inrange(closest) ||
							f.o.squaredist(dp) < entities::ents[closest]->o.squaredist(dp))
								closest = e.links[i];
					}
				}
				if(entities::ents.inrange(closest))
				{
					d->ai->addstate(AI_S_INTEREST, AI_T_NODE, closest);
					return true;
				}
			}
			if(check(d, b)) return true;
			if(find(d, b)) return true;
			if(target(d, b, true, true)) return true;
			if(randomnode(d, b, AIISFAR, 1e16f))
			{
				d->ai->addstate(AI_S_INTEREST, AI_T_NODE, d->ai->route[0]);
				return true;
			}
			world::suicide(d, HIT_LOST); // bail
			b.next = lastmillis + 500 + rnd((101-d->skill)*10);
			return true; // recycle and start from beginning
		}
		return true; // but don't pop the state
	}

	bool dodefend(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			switch(b.targtype)
			{
				case AI_T_NODE:
				case AI_T_ENTITY:
				{
					if(check(d, b)) return true;
					if(entities::ents.inrange(b.target))
					{
						gameentity &e = *(gameentity *)entities::ents[b.target];
						return defend(d, b, e.o, float(enttype[e.type].radius));
					}
					break;
				}
				case AI_T_AFFINITY:
				{
					if(m_ctf(world::gamemode)) return ctf::aidefend(d, b);
					if(m_stf(world::gamemode)) return stf::aidefend(d, b);
					break;
				}
				case AI_T_PLAYER:
				{
					gameent *e = world::getclient(b.target);
					if(e && e->state == CS_ALIVE) return defend(d, b, world::feetpos(e));
					break;
				}
				default: break;
			}
		}
		return false;
	}

	bool dointerest(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			int sweap = m_spawnweapon(world::gamemode, world::mutators);
			switch(b.targtype)
			{
				case AI_T_ENTITY:
				{
					if(d->hasweap(d->ai->weappref, sweap)) return false;
					if(entities::ents.inrange(b.target))
					{
						gameentity &e = *(gameentity *)entities::ents[b.target];
						if(enttype[e.type].usetype != EU_ITEM) return false;
						int attr = weapattr(e.attr[0], sweap);
						switch(e.type)
						{
							case WEAPON:
							{
								if(!e.spawned || d->hasweap(attr, sweap)) return false;
								break;
							}
							default: break;
						}
						return makeroute(d, b, e.o, enttype[e.type].radius);
					}
					break;
				}
				case AI_T_DROP:
				{
					if(d->hasweap(d->ai->weappref, sweap)) return false;
					loopvj(projs::projs) if(projs::projs[j]->projtype == PRJ_ENT && projs::projs[j]->ready() && projs::projs[j]->id == b.target)
					{
						projent &proj = *projs::projs[j];
						if(!entities::ents.inrange(proj.id) || enttype[entities::ents[proj.id]->type].usetype != EU_ITEM) return false;
						int attr = weapattr(entities::ents[proj.id]->attr[0], sweap);
						switch(entities::ents[proj.id]->type)
						{
							case WEAPON:
							{
								if(d->hasweap(attr, sweap) || proj.owner == d) return false;
								break;
							}
							default: break;
						}
						return makeroute(d, b, proj.o, enttype[entities::ents[proj.id]->type].radius);
						break;
					}
					break;
				}
				default: break;
			}
		}
		return false;
	}

	bool dopursue(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			switch(b.targtype)
			{
				case AI_T_AFFINITY:
				{
					if(m_ctf(world::gamemode)) return ctf::aipursue(d, b);
					if(m_stf(world::gamemode)) return stf::aipursue(d, b);
					break;
				}

				case AI_T_PLAYER:
				{
					gameent *e = world::getclient(b.target);
					if(e && e->state == CS_ALIVE) return patrol(d, b, world::feetpos(e));
					break;
				}
				default: break;
			}
		}
		return false;
	}

	int closenode(gameent *d)
	{
		vec pos = world::feetpos(d);
		int node = -1;
		float mindist = 1e16f; //(d->radius+enttype[WAYPOINT].radius);
		loopv(d->ai->route) if(d->ai->route[i] != d->lastnode && entities::ents.inrange(d->ai->route[i]) && !obstacles.find(d->ai->route[i], d))
		{
			gameentity &e = *(gameentity *)entities::ents[d->ai->route[i]];
			float dist = e.o.squaredist(pos);
			if(dist <= mindist)
			{
				node = i;
				mindist = dist;
			}
		}
		return node;
	}

	bool entspot(gameent *d, int n)
	{
		if(entities::ents.inrange(n) && n != d->ai->lastnode && n != d->ai->prevnode && !obstacles.find(n, d))
		{
			gameentity &e = *(gameentity *)entities::ents[n];
			d->ai->spot = e.o;
			vec pos = world::feetpos(d), off = vec(d->ai->spot).sub(pos);
			if(off.z >= AIJUMPHEIGHT && (d->timeinair ? d->vel.z <= 4.f && physics::canimpulse(d) : lastmillis - d->jumptime >= 300))
			{
				d->jumping = true;
				d->jumptime = lastmillis;
				vec dir(off.x, off.y, 0);
				if(dir.magnitude() <= AIJUMPHEIGHT) d->ai->dontmove = true; // going up
			}
			if(((e.attr[0] & WP_CROUCH && !d->crouching) || d->crouching) && (lastmillis-d->crouchtime >= 500))
			{
				d->crouching = !d->crouching;
				d->crouchtime = lastmillis;
			}
			return true;
		}
		return false;
	}

	bool hunt(gameent *d, bool retry = false)
	{
		if(!d->ai->route.empty())
		{
			int n = retry ? closenode(d) : d->ai->route.find(d->lastnode)-1;
			if(n >= 0 && entspot(d, d->ai->route[n])) return true;
			if(!retry) return hunt(d, true);
			d->ai->route.setsize(0); // force the next decision
		}
		if(entities::ents.inrange(d->lastnode)) // brute force our way out
		{
			gameentity &e = *(gameentity *)entities::ents[d->lastnode];
			loopv(e.links) if(entspot(d, e.links[i])) break;
		}
		return false;
	}

	bool request(gameent *d, aistate &b, int busy)
	{
		int sweap = m_spawnweapon(world::gamemode, world::mutators);
		if(!busy && d->reqswitch < 0)
		{
			int weap = -1;
			if(d->hasweap(d->ai->weappref, sweap)) weap = d->ai->weappref; // could be any weap
			else loopi(WEAPON_MAX) if(d->hasweap(i, sweap, 1)) weap = i; // only choose carriables here
			if(weap != d->weapselect && d->canswitch(weap, sweap, lastmillis))
			{
				client::addmsg(SV_WEAPSELECT, "ri3", d->clientnum, lastmillis-world::maptime, weap);
				d->reqswitch = lastmillis;
				return true;
			}
		}

		if(!d->ammo[d->weapselect] && d->canreload(d->weapselect, sweap, lastmillis) && d->reqreload < 0)
		{
			client::addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-world::maptime, d->weapselect);
			d->reqreload = lastmillis;
			return true;
		}

		int weappref = d->ai->weappref;
		if(b.type == AI_S_INTEREST && b.targtype == AI_T_ENTITY &&
			entities::ents.inrange(b.target) && entities::ents[b.target]->type == WEAPON)
				weappref = entities::ents[b.target]->attr[0];

		if(world::allowmove(d) && busy <= 1 && !d->hasweap(weappref, sweap) && !d->useaction && d->requse < 0)
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
							if(d->hasweap(attr, sweap) || attr != weappref) break;
							d->useaction = true;
							d->usetime = lastmillis;
							return true;
							break;
						}
						default: break;
					}
				}
			}
		}

		if(!busy && d->canreload(d->weapselect, sweap, lastmillis) && d->reqreload < 0)
		{
			client::addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-world::maptime, d->weapselect);
			d->reqreload = lastmillis;
			return true;
		}

		if(!busy && !m_noitems(world::gamemode, world::mutators) && d->reqswitch < 0 && b.type == AI_S_DEFEND && b.targtype == AI_T_PLAYER && b.idle)
		{
			loopirev(WEAPON_MAX) if(i != weappref && entities::ents.inrange(d->entid[i]))
			{
				client::addmsg(SV_DROP, "ri2", d->clientnum, i);
				d->reqswitch = lastmillis;
				return true;
			}
		}

		return false;
	}

	bool hastarget(gameent *d, aistate &b, gameent *e)
	{ // add margins of error
		if(!rnd(d->skill*100)) return true; // random margin of error
		gameent *h = world::intersectclosest(d->muzzle, d->ai->target, d);
		if(h && (h == d || (m_team(world::gamemode, world::mutators) && h->team == d->team))) return false;
		vec dir = vec(d->muzzle).sub(d->ai->target).normalize();
		float targyaw, targpitch, mindist = d->radius*d->radius, dist = d->muzzle.squaredist(d->ai->target);
		if(weaptype[d->weapselect].explode) mindist = weaptype[d->weapselect].explode*weaptype[d->weapselect].explode;
		if(mindist <= dist)
		{
			vectoyawpitch(dir, targyaw, targpitch);
			float rtime = (d->skill*weaptype[d->weapselect].rdelay/2000.f)+(d->skill*weaptype[d->weapselect].adelay/200.f),
					skew = clamp(float(lastmillis-b.millis)/float(rtime), 0.f, d->weapselect == WEAPON_GL ? 1.f : 1e16f),
						cyaw = fabs(targyaw-d->yaw), cpitch = fabs(targpitch-d->pitch);
			if(cyaw <= AIFOVX(d->skill)*skew && cpitch <= AIFOVY(d->skill)*skew) return true;
		}
		return false;
	}

	int process(gameent *d, aistate &b)
	{
		int result = 0;
		float frame = float(lastmillis-d->lastupdate)/float((111-d->skill)*(8+rnd(4)));
		gameent *e = world::getclient(d->ai->enemy);
		vec dp = world::headpos(d);
		if(e && e->state == CS_ALIVE && AITARG(d, e, true))
		{
			vec targ, ep = world::headpos(e);
			bool cansee = AICANSEE(dp, ep, d);
			if(cansee || (d->ai->lastseen >= 0 && lastmillis-d->ai->lastseen > d->skill*100))
			{
				if(cansee) d->ai->lastseen = lastmillis;
				float yaw, pitch;
				world::getyawpitch(dp, ep, yaw, pitch);
				world::fixrange(yaw, pitch);
				world::scaleyawpitch(d->yaw, d->pitch, yaw, pitch, frame, 4.f);
				if(d->canshoot(d->weapselect, m_spawnweapon(world::gamemode, world::mutators), lastmillis) && hastarget(d, b, e))
				{
					d->attacking = true;
					d->attacktime = lastmillis;
					result = 2;
				}
				else result = 1;
			}
			else d->ai->enemy = -1;
		}
		if(b.idle)
		{
			if(!b.wasidle)
			{
				d->ai->targyaw += 180.f;
				d->ai->targpitch = 0.f;
			}
			b.wasidle = true;
			b.stuck = 0;
		}
		else
		{
			b.wasidle = false;
			if(hunt(d))
			{
				vec spot = vec(d->ai->spot).add(vec(0, 0, d->height));
				world::getyawpitch(dp, spot, d->ai->targyaw, d->ai->targpitch);
				b.stuck = 0;
			}
			else if(lastmillis-b.stuck >= 3000)
			{
				if(b.stuck) // random walk, when all else fails
				{
					d->ai->targyaw += float(90+rnd(180));
					d->ai->targpitch = 0.f;
                    if(lastmillis-d->jumptime >= 500)
                    {
					    d->jumping = true;
					    d->jumptime = lastmillis;
                    }
				}
				b.stuck = lastmillis;
			}
		}
		world::fixrange(d->ai->targyaw, d->ai->targpitch);
		d->aimyaw = d->ai->targyaw; d->aimpitch = d->ai->targpitch;
		if(!result) world::scaleyawpitch(d->yaw, d->pitch, d->ai->targyaw, d->ai->targpitch, frame, 6.f);
		if(!b.idle && !d->ai->dontmove)
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

	void logic(gameent *d, aistate &b, bool run)
	{
		vec pos = world::headpos(d);
		findorientation(pos, d->yaw, d->pitch, d->ai->target);
		bool allowmove = world::allowmove(d);
		if(d->state != CS_ALIVE || !allowmove) d->stopmoving(true);
		if(d->state == CS_ALIVE)
		{
			if(allowmove && run)
			{
				int busy = process(d, b);
				if(!request(d, b, busy) && busy <= 0) target(d, b);
				weapons::shoot(d, d->ai->target, weaptype[d->weapselect].power); // always use full power
				if(!b.idle && !d->ai->dontmove && d->lastnode == d->ai->lastnode)
				{
					d->ai->timeinnode += curtime;
					if(d->ai->timeinnode >= 10000 && !d->ai->tryreset) d->ai->reset(true); // maybe we've gone insane, wipe our brain
					else if(d->ai->timeinnode >= 20000) world::suicide(d, HIT_LOST); // fine, we're better off doing something than nothing
				}
				else
				{
					d->ai->timeinnode = 0;
					d->ai->prevnode = d->ai->lastnode;
				}
				d->ai->lastnode = d->lastnode;
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
		float guessradius = 1.f; //world::player1->radius;
		obstacles.clear();
		loopi(world::numdynents())
		{
			gameent *d = (gameent *)world::iterdynents(i);
			if(!d || d->state != CS_ALIVE) continue;
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
		avoidmillis = lastmillis;
	}

	void think(gameent *d, bool run)
	{
		// the state stack works like a chain of commands, certain commands simply replace each other
		// others spawn new commands to the stack the ai reads the top command from the stack and executes
		// it or pops the stack and goes back along the history until it finds a suitable command to execute
		if(d->ai->state.empty()) d->ai->reset();
		bool running = run || d->ai->route.empty();
		loopvrev(d->ai->state)
		{
			aistate &c = d->ai->state[i];
			if(running && lastmillis >= c.next)
			{
				bool result = false;
				c.idle = false;
				switch(c.type)
				{
					case AI_S_WAIT: result = dowait(d, c); break;
					case AI_S_DEFEND: result = dodefend(d, c); break;
					case AI_S_PURSUE: result = dopursue(d, c); break;
					case AI_S_INTEREST: result = dointerest(d, c); break;
					default: result = 0; break;
				}
				if(c.type != AI_S_WAIT && !result)
				{
					d->ai->state.remove(i);
					continue; // shouldn't interfere
				}
			}
			logic(d, c, running);
			break;
		}
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
		part_text(vec(d->abovehead()).add(vec(0, 0, above)), s);
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
		if(aidebug > 3)
		{
			vec fr(d->muzzle), dr(d->ai->target), pos = world::headpos(d);
			if(dr.dist(pos) > AILOSDIST(d->skill))
			{
				dr.sub(fr);
				dr.normalize();
				dr.mul(AILOSDIST(d->skill));
			}
			renderline(fr, dr, cr, cg, cb, false);
			rendertris(dr, d->yaw, d->pitch, 2.f, cr, cg, cb, true, false);
		}
		if(aidebug > 4)
		{
			vec pos = world::feetpos(d);
			if(d->ai->spot != vec(0, 0, 0))
			{
				vec spot = vec(d->ai->spot).sub(vec(0, 0, d->height));
				renderline(pos, spot, 1.f, 1.f, 0.f, false);
			}
			if(entities::ents.inrange(d->lastnode))
			{
				renderline(pos, entities::ents[d->lastnode]->o, 0.f, 1.f, 0.f, false);
			}
		}
		renderprimitive(false);
	}

	void render()
	{
		if(aidebug)
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
					if(aidebug > 2 && top && rendernormally && b.type != AI_S_WAIT)
						drawroute(d, b, float(amt[1])/float(amt[0]));
					if(top)
					{
						if(aidebug > 1) top = false;
						else break;
					}
				}
			}
		}
	}
}
