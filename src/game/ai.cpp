#include "game.h"
namespace ai
{
	entities::avoidset obs;
    int updatemillis = 0, updateiters = 0, forcegun = -1;
    vec aitarget(0, 0, 0);

	VAR(aidebug, 0, 0, 6);
    VAR(aiforcegun, -1, -1, WEAP_SUPER-1);
    VARP(showaiinfo, 0, 0, 2); // 0/1 = shows/hides bot join/parts, 2 = show more verbose info

	ICOMMAND(addbot, "s", (char *s), client::addmsg(SV_ADDBOT, "ri", *s ? clamp(atoi(s), 1, 101) : -1));
	ICOMMAND(delbot, "", (), client::addmsg(SV_DELBOT, "r"));

	float viewdist(int x)
	{
		return x <= 100 ? clamp((SIGHTMIN+(SIGHTMAX-SIGHTMIN))/100.f*float(x), float(SIGHTMIN), float(game::fogdist)) : float(game::fogdist);
	}

	float viewfieldx(int x)
	{
		return x <= 100 ? clamp((VIEWMIN+(VIEWMAX-VIEWMIN))/100.f*float(x), float(VIEWMIN), float(VIEWMAX)) : float(VIEWMAX);
	}

	float viewfieldy(int x)
	{
		return viewfieldx(x)*3.f/4.f;
	}

	int owner(gameent *d)
	{
		if(entities::ents.inrange(d->aientity))
		{
			if(m_stf(game::gamemode)) return stf::aiowner(d);
			else if(m_stf(game::gamemode)) return stf::aiowner(d);
		}
		return d->team;
	}

	bool targetable(gameent *d, gameent *e, bool z)
	{
		if(e && d != e && game::allowmove(d) && !m_edit(game::gamemode))
			return e->state == CS_ALIVE && (!z || !m_team(game::gamemode, game::mutators) || owner(d) != owner(e));
		return false;
	}

	bool cansee(gameent *d, vec &x, vec &y, vec &targ)
	{
		if(game::allowmove(d))
			return getsight(x, d->yaw, d->pitch, y, targ, d->ai->views[2], d->ai->views[0], d->ai->views[1]);
		return false;
	}

	bool badhealth(gameent *d)
	{
		if(d->skill <= 100) return d->health <= (111-d->skill)/4;
		return false;
	}

	bool weaprange(gameent *d, int weap, bool alt, float dist)
	{
		if(weaptype[weap].extinguish[alt ? 1 : 0] && d->inliquid) return false;
		float mindist = weaptype[weap].explode[alt ? 1 : 0] ? weaptype[weap].explode[alt ? 1 : 0] : d->radius*3, maxdist = weaptype[weap].maxdist[alt ? 1 : 0] ? weaptype[weap].maxdist[alt ? 1 : 0] : hdr.worldsize;
		return dist >= mindist*mindist && dist <= maxdist*maxdist;
	}

	bool altfire(gameent *d, gameent *e)
	{
		if(e)
		{
			float dist = e->o.dist(d->o);
			if(!weaptype[d->weapselect].zooms && weaprange(d, d->weapselect, true, dist) && d->canshoot(d->weapselect, HIT_ALT, m_spawnweapon(game::gamemode, game::mutators), lastmillis, WEAP_S_RELOAD))
				return true;
		}
		return false;
	}

	bool hastarget(gameent *d, aistate &b, gameent *e, bool alt, float yaw, float pitch, float dist)
	{ // add margins of error
		if(d->skill <= 100 && !rnd(d->skill*10)) return true; // random margin of error
		if(weaprange(d, d->weapselect, alt, dist))
		{
			float skew = clamp(float(lastmillis-d->ai->enemymillis)/float((d->skill*aitype[d->aitype].frame*weaptype[d->weapselect].rdelay/2000.f)+(d->skill*weaptype[d->weapselect].adelay[alt ? 1 : 0]/200.f)), 0.f, d->weapselect == WEAP_GRENADE || d->weapselect == WEAP_GIBS ? 0.25f : 1e16f);
			if(fabs(yaw-d->yaw) <= d->ai->views[0]*skew && fabs(pitch-d->pitch) <= d->ai->views[1]*skew) return true;
		}
		return false;
	}

	vec getaimpos(gameent *d, gameent *e, bool alt)
	{
		vec o = e->headpos();
		if(weaptype[d->weapselect].radial[alt ? 1 : 0]) o.z -= e->height;
		if(d->skill <= 100)
		{
			if(weaptype[d->weapselect].radial[alt ? 1 : 0]) o.z += e->height*(1.f/float(d->skill));
			else o.z -= e->height*(1.f/float(d->skill));
			o.x += (rnd(int(e->radius*(d->weapselect == WEAP_PISTOL ? 8 : 4))+1)-e->radius*(d->weapselect == WEAP_PISTOL ? 4 : 2))*(1.f/float(d->skill));
			o.y += (rnd(int(e->radius*(d->weapselect == WEAP_PISTOL ? 8 : 4))+1)-e->radius*(d->weapselect == WEAP_PISTOL ? 4 : 2))*(1.f/float(d->skill));
		}
		return o;
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

	void init(gameent *d, int at, int et, int on, int sk, int bn, char *name, int tm)
	{
		gameent *o = game::newclient(on);
		string m;
		if(o) copystring(m, game::colorname(o));
		else formatstring(m)("\fs\faunknown [\fs\fr%d\fS]\fS", on);
		bool resetthisguy = false;
		if(!d->name[0])
		{
			if(showaiinfo && game::showplayerinfo)
			{
				if(showaiinfo > 1) conoutft(game::showplayerinfo > 1 ? int(CON_CHAT) : int(CON_INFO), "\fg%s assigned to %s at skill %d", game::colorname(d, name), m, sk);
				else conoutft(game::showplayerinfo > 1 ? int(CON_CHAT) : int(CON_INFO), "\fg%s joined the game", game::colorname(d, name), m, sk);
			}
			resetthisguy = true;
		}
		else
		{
			if(d->ownernum != on)
			{
				if(showaiinfo && game::showplayerinfo)
					conoutft(game::showplayerinfo > 1 ? int(CON_CHAT) : int(CON_INFO), "\fg%s reassigned to %s", game::colorname(d, name), m);
				resetthisguy = true;
			}
			if(d->skill != sk && showaiinfo > 1 && game::showplayerinfo)
				conoutft(game::showplayerinfo > 1 ? int(CON_CHAT) : int(CON_INFO), "\fg%s changed skill to %d", game::colorname(d, name), sk);
		}

		copystring(d->name, name, MAXNAMELEN);
		if((d->aitype = at) >= AI_START)
		{
			d->type = ENT_AI;
			d->aientity = et;
			d->maxspeed = aitype[d->aitype].maxspeed;
			d->weight = aitype[d->aitype].weight;
			d->xradius = aitype[d->aitype].xradius;
			d->yradius = aitype[d->aitype].yradius;
			d->zradius = d->height = aitype[d->aitype].height;
		}
		d->ownernum = on;
		d->skill = sk;
		d->team = tm;

		if(resetthisguy) projs::remove(d);
		if(game::player1->clientnum == d->ownernum) create(d);
		else if(d->ai) destroy(d);
	}

	void update()
	{
		bool updating = lastmillis-updatemillis > m_speedtime(100); // fixed rate logic at 10fps
        if(updating)
        {
        	avoid();
        	forcegun = multiplayer(false) ? -1 : aiforcegun;
        }
		loopv(game::players) if(game::players[i] && game::players[i]->ai)
		{
			bool doupdate = updating && (game::players[i]->aitype >= AI_START ? !updateiters : true);
			if(!game::intermission) think(game::players[i], doupdate);
			else game::players[i]->stopmoving(true);
		}
		if(updating)
		{
			updatemillis = lastmillis;
			if(++updateiters >= 5) updateiters = 0;
		}
	}

	bool checkothers(vector<int> &targets, gameent *d, int state, int targtype, int target, bool teams)
	{ // checks the states of other ai for a match
		targets.setsize(0);
		gameent *e = NULL;
		loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e != d && e->ai && e->state == CS_ALIVE && e->aitype == d->aitype)
		{
			if(targets.find(e->clientnum) >= 0) continue;
			if(teams && m_team(game::gamemode, game::mutators) && d && owner(d) != owner(e)) continue;
			aistate &b = e->ai->getstate();
			if(state >= 0 && b.type != state) continue;
			if(target >= 0 && b.target != target) continue;
			if(targtype >=0 && b.targtype != targtype) continue;
			targets.add(e->clientnum);
		}
		return !targets.empty();
	}

	bool makeroute(gameent *d, aistate &b, int node, bool changed, int retries)
	{
		if(changed && !d->ai->route.empty() && d->ai->route[0] == node) return true;
		if(entities::route(d->lastnode, node, d->ai->route, obs, d, retries <= 1) > 0)
		{
			b.override = false;
			return true;
		}
		d->ai->clear(true);
		if(retries <= 1) return makeroute(d, b, node, false, retries+1);
		return false;
	}

	bool makeroute(gameent *d, aistate &b, const vec &pos, bool changed, int retries)
	{
		int node = entities::closestent(WAYPOINT, pos, NEARDIST, true, d);
		return makeroute(d, b, node, changed, retries);
	}

	bool randomnode(gameent *d, aistate &b, const vec &pos, float guard, float wander)
	{
		static vector<int> candidates;
		candidates.setsizenodelete(0);
        entities::findentswithin(WAYPOINT, pos, guard, wander, candidates);

		while(!candidates.empty())
		{
			int w = rnd(candidates.length()), n = candidates.removeunordered(w);
			if(n != d->lastnode && !d->ai->hasprevnode(n) && !obs.find(n, d) && makeroute(d, b, n)) return true;
		}
		return false;
	}

	bool randomnode(gameent *d, aistate &b, float guard, float wander)
	{
		return randomnode(d, b, d->feetpos(), guard, wander);
	}

	bool enemy(gameent *d, aistate &b, const vec &pos, float guard = NEARDIST, bool pursue = false)
	{
		if(game::allowmove(d))
		{
			gameent *t = NULL, *e = NULL;
			vec dp = d->headpos(), tp(0, 0, 0);
			bool insight = false, tooclose = false;
			float mindist = guard*guard;
			loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e != d && targetable(d, e, true))
			{
				bool alt = altfire(d, e);
				vec ep = getaimpos(d, e, alt);
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

	bool patrol(gameent *d, aistate &b, const vec &pos, float guard, float wander, int walk, bool retry)
	{
		if(aitype[d->aitype].maxspeed)
		{
			vec feet = d->feetpos();
			if(walk == 2 || b.override || (walk && feet.squaredist(pos) <= guard*guard) || !makeroute(d, b, pos))
			{ // run away and back to keep ourselves busy
				if(!b.override && randomnode(d, b, pos, guard, wander))
				{
					b.override = true;
					return true;
				}
				else if(d->ai->route.length() > 1) return true;
				else if(!retry)
				{
					b.override = false;
					return patrol(d, b, pos, guard, wander, walk, true);
				}
				b.override = false;
				return false;
			}
		}
		b.override = false;
		return true;
	}

	bool defend(gameent *d, aistate &b, const vec &pos, float guard, float wander, int walk)
	{
		bool hasenemy = enemy(d, b, pos, wander, false);
		if(!aitype[d->aitype].maxspeed) { b.idle = hasenemy ? 2 : 1; return true; }
		{
			if(!walk && d->feetpos().squaredist(pos) <= guard*guard)
			{
				b.idle = hasenemy ? 2 : 1;
				return true;
			}
			return patrol(d, b, pos, guard, wander, walk);
		}
		return false;
	}

	bool violence(gameent *d, aistate &b, gameent *e, bool pursue)
	{
		if(e && targetable(d, e, true))
		{
			if(pursue) d->ai->switchstate(b, AI_S_PURSUE, AI_T_PLAYER, e->clientnum);
			if(d->ai->enemy != e->clientnum) d->ai->enemymillis = lastmillis;
			d->ai->enemy = e->clientnum;
			d->ai->enemyseen = lastmillis;
			bool alt = altfire(d, e);
			vec dp = d->headpos(), ep = getaimpos(d, e, alt);
			if(!cansee(d, dp, ep)) d->ai->enemyseen -= m_speedtime(((111-d->skill)*10)+10); // so we don't "quick"
			return true;
		}
		return false;
	}

	bool target(gameent *d, aistate &b, bool pursue = false, bool force = false)
	{
		gameent *t = NULL, *e = NULL;
		vec dp = d->headpos(), tp(0, 0, 0);
		loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e != d && targetable(d, e, true))
		{
			bool alt = altfire(d, e);
			vec ep = getaimpos(d, e, alt);
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
		loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e != d && (all || e->aitype < 0) && owner(d) == owner(e))
		{
			interest &n = interests.add();
			n.state = AI_S_DEFEND;
			n.node = e->lastnode;
			n.target = e->clientnum;
			n.targtype = AI_T_PLAYER;
			n.score = e->o.squaredist(d->o)/(force || d->hasweap(d->arenaweap, m_spawnweapon(game::gamemode, game::mutators)) ? 10.f : 1.f);
		}
	}

	void items(gameent *d, aistate &b, vector<interest> &interests, bool force = false)
	{
		vec pos = d->feetpos();
		loopvj(entities::ents)
		{
			gameentity &e = *(gameentity *)entities::ents[j];
			if(enttype[e.type].usetype != EU_ITEM) continue;
			int sweap = m_spawnweapon(game::gamemode, game::mutators);
			switch(e.type)
			{
				case WEAPON:
				{
					int attr = weapattr(e.attrs[0], sweap);
					if(e.spawned && isweap(attr) && !d->hasweap(attr, sweap))
					{ // go get a weapon upgrade
						interest &n = interests.add();
						n.state = AI_S_INTEREST;
						n.node = entities::closestent(WAYPOINT, e.o, NEARDIST, true);
						n.target = j;
						n.targtype = AI_T_ENTITY;
						n.score = pos.squaredist(e.o)/(force || attr == d->arenaweap ? 100.f : 1.f);
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
			gameentity &e = *(gameentity *)entities::ents[proj.id];
			int sweap = m_spawnweapon(game::gamemode, game::mutators);
			switch(e.type)
			{
				case WEAPON:
				{
					int attr = weapattr(e.attrs[0], sweap);
					if(isweap(attr) && !d->hasweap(attr, sweap))
					{ // go get a weapon upgrade
						if(proj.owner == d) break;
						interest &n = interests.add();
						n.state = AI_S_INTEREST;
						n.node = entities::closestent(WAYPOINT, proj.o, NEARDIST, true);
						n.target = proj.id;
						n.targtype = AI_T_DROP;
						n.score = pos.squaredist(proj.o)/(force || attr == d->arenaweap ? 100.f : 1.f);
					}
					break;
				}
				default: break;
			}
		}
	}

	bool find(gameent *d, aistate &b)
	{
		static vector<interest> interests; interests.setsizenodelete(0);
		if(m_fight(game::gamemode))
		{ // don't let bots consume items in story mode (yet?)
			if(d->aitype == AI_BOT && !d->hasweap(d->arenaweap, m_spawnweapon(game::gamemode, game::mutators)))
				items(d, b, interests);
			if(m_stf(game::gamemode)) stf::aifind(d, b, interests);
			else if(m_ctf(game::gamemode)) ctf::aifind(d, b, interests);
		}
		if(d->aitype == AI_BOT)
		{
			if(m_team(game::gamemode, game::mutators)) assist(d, b, interests);
		}
		else if(entities::ents.inrange(d->aientity))
		{
			loopv(entities::ents[d->aientity]->links) if(entities::ents[entities::ents[d->aientity]->links[i]]->type == WAYPOINT)
			{
				interest &n = interests.add();
				n.state = AI_S_DEFEND;
				n.target = n.node = entities::ents[d->aientity]->links[i];
				n.targtype = AI_T_NODE;
				n.score = -1;
			}
		}
		while(!interests.empty())
		{
			int q = interests.length()-1;
			loopi(interests.length()-1) if(interests[i].score < interests[q].score) q = i;
			interest n = interests.removeunordered(q);
			bool proceed = true;
			static vector<int> targets;
			targets.setsizenodelete(0);
			if(m_fight(game::gamemode) && d->aitype == AI_BOT) switch(n.state)
			{
				case AI_S_DEFEND: // don't get into herds
					proceed = !checkothers(targets, d, n.state, n.targtype, n.target, true);
					break;
				default: break;
			}
			if(proceed && (!aitype[d->aitype].maxspeed || makeroute(d, b, n.node, false)))
			{
				d->ai->addstate(n.state, n.targtype, n.target);
				return true;
			}
		}
		return false;
	}

	void damaged(gameent *d, gameent *e)
	{
		if(d->ai && game::allowmove(d) && targetable(d, e, true)) // see if this ai is interested in a grudge
		{
			aistate &b = d->ai->getstate();
			if(violence(d, b, e, false)) return;
		}
		static vector<int> targets; // check if one of our ai is defending them
		targets.setsizenodelete(0);
		if(checkothers(targets, d, AI_S_DEFEND, AI_T_PLAYER, d->clientnum, true))
		{
			gameent *t;
			loopv(targets) if((t = game::getclient(targets[i])) && t->ai && game::allowmove(t) && targetable(t, e, true))
			{
				aistate &c = t->ai->getstate();
				if(violence(t, c, e, false)) return;
			}
		}
	}

	void setup(gameent *d, bool tryreset = false, int ent = -1)
	{
		d->aientity = ent;
		if(d->ai)
		{
			d->ai->reset(tryreset);
			aistate &b = d->ai->getstate();
			b.next = lastmillis+m_speedtime(((111-d->skill)*10)+rnd((111-d->skill)*10));
			if(d->aitype >= AI_START && aitype[d->aitype].weap >= 0) d->arenaweap = aitype[d->aitype].weap;
			else if(m_noitems(game::gamemode, game::mutators) && !m_arena(game::gamemode, game::mutators))
				d->arenaweap = m_spawnweapon(game::gamemode, game::mutators);
			else if(forcegun >= 0 && forcegun < WEAP_SUPER) d->arenaweap = forcegun;
			else while(true)
			{
				d->arenaweap = rnd(WEAP_SUPER);
				if(d->arenaweap != WEAP_PISTOL || !rnd(d->skill)) break;
			}
			if(d->aitype == AI_BOT)
			{
				d->ai->suspended = false;
				if(m_arena(game::gamemode, game::mutators) && d->arenaweap == WEAP_GRENADE)
					d->arenaweap = WEAP_PISTOL;
			}
		}
	}

	void spawned(gameent *d, int ent) { setup(d, false, ent); }
	void killed(gameent *d, gameent *e) { if(d->ai) d->ai->reset(); }

	bool check(gameent *d, aistate &b)
	{
		if(m_stf(game::gamemode) && stf::aicheck(d, b)) return true;
		else if(m_ctf(game::gamemode) && ctf::aicheck(d, b)) return true;
		return false;
	}

	int dowait(gameent *d, aistate &b)
	{
		if(!m_edit(game::gamemode))
		{
			if(check(d, b)) return 1;
			if(find(d, b)) return 1;
			if(target(d, b, true, true)) return 1;
		}
		switch(d->aitype)
		{
			case AI_BOT: if(randomnode(d, b, NEARDIST, 1e16f))
			{
				d->ai->addstate(AI_S_INTEREST, AI_T_NODE, d->ai->route[0]);
				return 1;
			} break;
			case AI_TURRET: if(randomnode(d, b, NEARDIST, FARDIST))
			{
				d->ai->addstate(AI_S_DEFEND, AI_T_NODE, d->ai->route[0]);
				return 1;
			} break;
			default: break;
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
					if(m_stf(game::gamemode)) return stf::aidefend(d, b) ? 1 : 0;
					else if(m_ctf(game::gamemode)) return ctf::aidefend(d, b) ? 1 : 0;
					break;
				}
				case AI_T_PLAYER:
				{
					if(check(d, b)) return 1;
					gameent *e = game::getclient(b.target);
					if(e && e->state == CS_ALIVE) return defend(d, b, e->feetpos()) ? 1 : 0;
					break;
				}
				default: break;
			}
		}
		return 0;
	}

	int dointerest(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE && aitype[d->aitype].maxspeed)
		{
			int sweap = m_spawnweapon(game::gamemode, game::mutators);
			switch(b.targtype)
			{
				case AI_T_NODE:
				{
					if(check(d, b)) return 1;
					if(entities::ents.inrange(b.target))
					{
						gameentity &e = *(gameentity *)entities::ents[b.target];
						if(vec(e.o).sub(d->feetpos()).magnitude() > CLOSEDIST)
							return makeroute(d, b, e.o) ? 1 : 0;
					}
					break;
				}
				case AI_T_ENTITY:
				{
					if(d->hasweap(d->arenaweap, sweap)) return 0;
					if(entities::ents.inrange(b.target))
					{
						gameentity &e = *(gameentity *)entities::ents[b.target];
						if(enttype[e.type].usetype != EU_ITEM) return 0;
						int attr = weapattr(e.attrs[0], sweap);
						switch(e.type)
						{
							case WEAPON:
							{
								if(!e.spawned || d->hasweap(attr, sweap)) return 0;
								float guard = enttype[e.type].radius;
								if(d->feetpos().squaredist(e.o) <= guard*guard) b.idle = enemy(d, b, e.o, guard*4, false) ? 2 : 1;
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
					if(d->hasweap(d->arenaweap, sweap)) return 0;
					loopvj(projs::projs) if(projs::projs[j]->projtype == PRJ_ENT && projs::projs[j]->ready() && projs::projs[j]->id == b.target)
					{
						projent &proj = *projs::projs[j];
						if(!entities::ents.inrange(proj.id) || enttype[entities::ents[proj.id]->type].usetype != EU_ITEM) return 0;
						gameentity &e = *(gameentity *)entities::ents[proj.id];
						int attr = weapattr(e.attrs[0], sweap);
						switch(e.type)
						{
							case WEAPON:
							{
								if(d->hasweap(attr, sweap) || proj.owner == d) return 0;
								float guard = enttype[e.type].radius;
								if(d->feetpos().squaredist(e.o) <= guard*guard) b.idle = enemy(d, b, e.o, guard*4, false) ? 2 : 1;
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
					if(aitype[d->aitype].maxspeed)
					{
						if(m_stf(game::gamemode)) return stf::aipursue(d, b) ? 1 : 0;
						else if(m_ctf(game::gamemode)) return ctf::aipursue(d, b) ? 1 : 0;
					}
					break;
				}

				case AI_T_PLAYER:
				{
					if(check(d, b)) return 1;
					gameent *e = game::getclient(b.target);
					if(e && e->state == CS_ALIVE)
					{
						bool alt = altfire(d, e);
						if(aitype[d->aitype].maxspeed)
						{
							float mindist = weaptype[d->weapselect].explode[alt ? 1 : 0] ? weaptype[d->weapselect].explode[alt ? 1 : 0] : NEARDIST, maxdist = weaptype[d->weapselect].maxdist[alt ? 1 : 0] ? weaptype[d->weapselect].maxdist[alt ? 1 : 0] : FARDIST;
							return patrol(d, b, e->feetpos(), mindist, maxdist) ? 1 : 0;
						}
						else
						{
							vec dp = d->headpos(), ep = getaimpos(d, e, alt);
							return cansee(d, dp, ep) || (e->clientnum == d->ai->enemy && d->ai->enemyseen && lastmillis-d->ai->enemyseen <= m_speedtime((d->skill*50)+1000));
						}
					}
					break;
				}
				default: break;
			}
		}
		return 0;
	}

	int closenode(gameent *d, bool force = false)
	{
		vec pos = d->feetpos();
		int node = -1;
		float mindist = FARDIST*FARDIST;
		loopvrev(d->ai->route) if(entities::ents.inrange(d->ai->route[i]))
		{
			gameentity &e = *(gameentity *)entities::ents[d->ai->route[i]];
			vec epos = e.o;
			int entid = obs.remap(d, d->ai->route[i], epos);
			if(entities::ents.inrange(entid) && (force || entid == d->ai->route[i] || !d->ai->hasprevnode(entid)))
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

	bool wpspot(gameent *d, int n, bool force = false)
	{
		if(entities::ents.inrange(n))
		{
			gameentity &e = *(gameentity *)entities::ents[n];
			vec epos = e.o;
			int entid = obs.remap(d, n, epos);
			if(entities::ents.inrange(entid) && (force || entid == n || !d->ai->hasprevnode(entid)))
			{
				d->ai->spot = epos;
				if(((e.attrs[0] & WP_F_CROUCH && !d->action[AC_CROUCH]) || d->action[AC_CROUCH]) && (lastmillis-d->actiontime[AC_CROUCH] >= 500))
				{
					d->action[AC_CROUCH] = !d->action[AC_CROUCH];
					d->actiontime[AC_CROUCH] = lastmillis;
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
			static vector<int> anyremap; anyremap.setsizenodelete(0);
			if(!e.links.empty())
			{
				loopv(e.links) if(entities::ents.inrange(e.links[i]) && (retry || !d->ai->hasprevnode(e.links[i])))
					anyremap.add(e.links[i]);
			}
			while(!anyremap.empty())
			{
				int r = rnd(anyremap.length()), t = anyremap[r];
				if(wpspot(d, t, retry))
				{
					d->ai->route.add(t);
					d->ai->route.add(d->lastnode);
					return true;
				}
				anyremap.remove(r);
			}
			if(!retry) return anynode(d, b, true);
		}
		return false;
	}

	bool hunt(gameent *d, aistate &b, int retries = 0)
	{
		if(!d->ai->route.empty())
		{
			int n = retries%2 ? d->ai->route.find(d->lastnode) : closenode(d, retries >= 2);
			if(retries%2 && d->ai->route.inrange(n))
			{
				while(d->ai->route.length() > n+1) d->ai->route.pop(); // waka-waka-waka-waka
				if(!n)
				{
					if(wpspot(d, d->ai->route[n], retries >= 2))
					{
						b.next = lastmillis; // reached our goal, update
						d->ai->clear(false);
						return true;
					}
					else if(retries <= 2) return hunt(d, b, retries+1); // try again
				}
				else n--; // otherwise, we want the next in line
			}
			if(d->ai->route.inrange(n) && wpspot(d, d->ai->route[n], retries >= 2)) return true;
			if(retries <= 2) return hunt(d, b, retries+1); // try again
		}
		b.override = false;
		d->ai->clear(false);
		return anynode(d, b);
	}

	void jumpto(gameent *d, aistate &b, const vec &pos)
	{
		vec off = vec(pos).sub(d->feetpos());
		bool offground = d->timeinair && !physics::liquidcheck(d) && !d->onladder,
			jumper = off.z >= JUMPMIN && (!offground || (d->timeinair > 500 && physics::canimpulse(d, impulsecost))),
			jump = (jumper || d->onladder || lastmillis >= d->ai->jumprand) && lastmillis >= d->ai->jumpseed;
		if(jump)
		{
			vec old = d->o;
			d->o = vec(pos).add(vec(0, 0, d->height));
			if(!collide(d, vec(0, 0, 1))) jump = false;
			d->o = old;
			if(jump)
			{
				loopv(entities::ents) if(entities::ents[i]->type == PUSHER)
				{
					gameentity &e = *(gameentity *)entities::ents[i];
					float radius = (e.attrs[3] ? e.attrs[3] : enttype[e.type].radius)*1.5f; radius *= radius;
					if(e.o.squaredist(pos) <= radius) { jump = false; break; }
				}
			}
		}
		if(jump)
		{
			d->action[AC_JUMP] = jump;
			d->actiontime[AC_JUMP] = lastmillis;
			int seed = (111-d->skill)*(d->onladder || d->inliquid ? 1 : 5);
			d->ai->jumpseed = lastmillis+m_speedtime(seed+rnd(seed));
			seed *= b.idle ? 100 : 50;
			d->ai->jumprand = lastmillis+m_speedtime(seed+rnd(seed));
		}
	}

	int process(gameent *d, aistate &b)
	{
		int result = 0, stupify = d->skill <= 30+rnd(20) ? rnd(d->skill*1111) : 0, skmod = (111-d->skill)*10;
		float frame = float(lastmillis-d->ai->lastrun)/float(skmod/2*aitype[d->aitype].frame);
		vec dp = d->headpos();

		bool wasdontmove = d->ai->dontmove;
		d->ai->dontmove = false;
		if(b.idle == 1 || (stupify && stupify <= skmod) || !aitype[d->aitype].maxspeed)
		{
			d->ai->lastaction = d->ai->lasthunt = lastmillis;
			d->ai->dontmove = true;
		}
		else if(hunt(d, b))
		{
			game::getyawpitch(dp, vec(d->ai->spot).add(vec(0, 0, d->height)), d->ai->targyaw, d->ai->targpitch);
			d->ai->lasthunt = lastmillis;
		}
		else d->ai->dontmove = true;

		if(aitype[d->aitype].maxspeed && d->aitype != AI_ZOMBIE)
		{
			if(!d->ai->dontmove) jumpto(d, b, d->ai->spot);
			if(b.idle == 1 && b.type != AI_S_WAIT)
			{
				bool wascrouching = lastmillis-d->actiontime[AC_CROUCH] <= 500, wantscrouch = d->ai->dontmove && !wasdontmove && !d->action[AC_CROUCH];
				if(wascrouching || wantscrouch)
				{
					d->action[AC_CROUCH] = true;
					if(wantscrouch) d->actiontime[AC_CROUCH] = lastmillis;
				}
			}
		}

		gameent *e = game::getclient(d->ai->enemy);
		bool enemyok = e && targetable(d, e, true);
		if(!enemyok || d->skill > 70)
		{
			e = game::intersectclosest(dp, d->ai->target, d);
			if(!targetable(d, e, true))
				e = enemyok ? game::getclient(d->ai->enemy) : NULL;
		}
		if(e && e->state == CS_ALIVE)
		{
			bool alt = altfire(d, e);
			vec ep = getaimpos(d, e, alt);
			bool insight = cansee(d, dp, ep), hasseen = d->ai->enemyseen && lastmillis-d->ai->enemyseen <= m_speedtime((d->skill*50)+1000),
				quick = d->ai->enemyseen && lastmillis-d->ai->enemyseen <= m_speedtime(skmod);
			if(insight) d->ai->enemyseen = lastmillis;
			if(b.idle || insight || hasseen)
			{
				float yaw, pitch;
				game::getyawpitch(dp, ep, yaw, pitch);
				game::fixrange(yaw, pitch);
				float sskew = insight ? 2.f : (hasseen ? 1.f : 0.5f);
				if(b.idle == 1)
				{
					d->ai->targyaw = yaw;
					d->ai->targpitch = pitch;
					if(!insight) frame /= 4.f;
				}
				else if(!insight) frame /= 2.f;
				game::scaleyawpitch(d->yaw, d->pitch, yaw, pitch, frame, sskew);
				if(insight || quick)
				{
					if(!d->ai->becareful && d->canshoot(d->weapselect, alt ? HIT_ALT : 0, m_spawnweapon(game::gamemode, game::mutators), lastmillis, WEAP_S_RELOAD) &&
						hastarget(d, b, e, alt, yaw, pitch, dp.squaredist(ep)))
					{
						d->action[alt ? AC_ALTERNATE : AC_ATTACK] = true;
						d->ai->lastaction = d->actiontime[alt ? AC_ALTERNATE : AC_ATTACK] = lastmillis;
						result = 4;
					}
					else result = insight ? 3 : 2;
				}
				else result = hasseen ? 2 : 1;
			}
			else
			{
				d->ai->enemy = -1;
				d->ai->enemymillis = 0;
				result = 0;
				frame /= 2.f;
			}
		}
		else
		{
			if(!b.idle && !enemyok)
			{
				d->ai->enemy = -1;
				d->ai->enemymillis = 0;
			}
			result = 0;
			frame /= 2.f;
		}

		game::fixrange(d->ai->targyaw, d->ai->targpitch);
		d->aimyaw = d->ai->targyaw; d->aimpitch = d->ai->targpitch;
		if(!result) game::scaleyawpitch(d->yaw, d->pitch, d->ai->targyaw, d->ai->targpitch, frame, 1.f);

		if(d->ai->dontmove || (d->ai->becareful && d->vel.magnitude() >= physics::gravityforce(d))) d->move = d->strafe = 0;
		else
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
			game::fixrange(d->aimyaw, d->aimpitch);
		}
		d->ai->lastrun = lastmillis;
		return result;
	}

	bool request(gameent *d, aistate &b)
	{
		int busy = process(d, b), sweap = m_spawnweapon(game::gamemode, game::mutators);
		if(d->aitype == AI_BOT)
		{
			if(busy <= 1 && !m_noitems(game::gamemode, game::mutators) && d->weapwaited(d->weapselect, lastmillis, d->skipwait(d->weapselect, lastmillis, WEAP_S_RELOAD)) && b.type == AI_S_DEFEND && b.idle)
			{
				loopirev(WEAP_SUPER) if(i != WEAP_GRENADE && i != d->arenaweap && i != d->weapselect && entities::ents.inrange(d->entid[i]))
				{
					client::addmsg(SV_DROP, "ri3", d->clientnum, lastmillis-game::maptime, i);
					d->setweapstate(d->weapselect, WEAP_S_WAIT, WEAPSWITCHDELAY, lastmillis);
					d->ai->lastaction = lastmillis;
					break;
				}
			}
			if(game::allowmove(d) && busy <= 3 && !d->action[AC_USE] && d->weapwaited(d->weapselect, lastmillis, d->skipwait(d->weapselect, lastmillis, WEAP_S_RELOAD)))
			{
				static vector<actitem> actitems;
				actitems.setsizenodelete(0);
				if(entities::collateitems(d, actitems))
				{
					while(!actitems.empty())
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
								if(!busy || (b.type == AI_S_INTEREST && b.targtype == AI_T_ENTITY && b.target == t.target))
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
								if(!busy || (b.type == AI_S_INTEREST && b.targtype == AI_T_DROP && b.target == t.target))
									ent = proj.id;
								break;
							}
							default: break;
						}
						if(entities::ents.inrange(ent))
						{
							extentity &e = *entities::ents[ent];
							if(enttype[e.type].usetype == EU_ITEM)
							{
								if(m_noitems(game::gamemode, game::mutators)) continue;
								int attr = e.type == WEAPON ? weapattr(e.attrs[0], sweap) : e.attrs[0];
								if(d->canuse(e.type, attr, e.attrs, sweap, lastmillis, WEAP_S_RELOAD)) switch(e.type)
								{
									case WEAPON:
									{
										if(d->hasweap(attr, sweap)) break;
										d->action[AC_USE] = true;
										d->ai->lastaction = d->actiontime[AC_USE] = lastmillis;
										return true;
										break;
									}
									default: break;
								}
							}
						}
						actitems.pop();
					}
				}
			}
		}

		if(busy <= 3 && d->weapwaited(d->weapselect, lastmillis, d->skipwait(d->weapselect, lastmillis, WEAP_S_RELOAD)))
		{
			int weap = d->hasweap(d->arenaweap, sweap) ? d->arenaweap : -1;
			vec dp = d->headpos(); float dist = 0;
			gameent *e = game::getclient(d->ai->enemy);
			bool alt = false;
			if(e)
			{
				alt = altfire(d, e);
				dist = dp.squaredist(getaimpos(d, e, alt));
			}
			if(!isweap(weap) || (e && !weaprange(d, weap, alt, dist)))
			{
				loopirev(WEAP_MAX) if(d->hasweap(i, sweap) && (!e || weaprange(d, i, alt, dist)))
				{
					weap = i;
					break;
				}
			}
			if(isweap(weap) && weap != d->weapselect && weapons::weapselect(d, weap))
			{
				d->ai->lastaction = lastmillis;
				return true;
			}
		}

		if(d->hasweap(d->weapselect, sweap) && busy <= (!d->ammo[d->weapselect] ? 3 : 1) && weapons::weapreload(d, d->weapselect))
		{
			d->ai->lastaction = lastmillis;
			return true;
		}

		return busy >= 2;
	}

	void logic(gameent *d, aistate &b, bool run)
	{
		if(!d->ai->suspended)
		{
			vec dp = d->headpos();
			findorientation(dp, d->yaw, d->pitch, d->ai->target);
			bool allowmove = game::allowmove(d);
			if(d->state != CS_ALIVE || !allowmove) d->stopmoving(true);
			if(d->state == CS_ALIVE && allowmove)
			{
				if(!request(d, b)) target(d, b, false, b.idle ? true : false);
				weapons::shoot(d, d->ai->target);
				if(d->ai->lasthunt && aitype[d->aitype].maxspeed)
				{
					int millis = lastmillis-d->ai->lasthunt;
					if(millis < m_speedtime(2500)) d->ai->tryreset = false;
					else if(millis < m_speedtime(5000))
					{
						if(!d->ai->tryreset) setup(d, true, d->aientity);
					}
					else if(d->ai->tryreset)
					{
						game::suicide(d, HIT_LOST); // better off doing something than nothing
						d->ai->reset(false);
					}
				}
			}
		}
        if(d->state == CS_DEAD || d->state == CS_WAITING)
        {
        	if(d->ragdoll) moveragdoll(d, true);
			else if(lastmillis-d->lastpain < 2000)
				physics::move(d, 1, false);
        }
		else
        {
            if(d->ragdoll) cleanragdoll(d);
            if(d->state == CS_ALIVE && !game::intermission && !d->ai->suspended)
            {
            	bool ladder = d->onladder;
				physics::move(d, 1, true);
				if(!ladder && d->onladder) d->ai->jumpseed = lastmillis;
				if(d->ai->becareful && (!d->timeinair || d->vel.magnitude()-d->falling.magnitude() < physics::gravityforce(d)))
					d->ai->becareful = false;
				if(d->aitype == AI_BOT) entities::checkitems(d);
            }
        }
		d->action[AC_ATTACK] = d->action[AC_ALTERNATE] = d->action[AC_RELOAD] = d->action[AC_USE] = false;
	}

	void avoid()
	{
		obs.clear();
		loopi(game::numdynents())
		{
			gameent *d = (gameent *)game::iterdynents(i);
			if(!d || d->state != CS_ALIVE || !physics::issolid(d)) continue;
			vec pos = d->feetpos();
			float limit = enttype[WAYPOINT].radius+d->radius;
            obs.avoidnear(d, pos, limit);
		}
		loopv(projs::projs)
		{
			projent *p = projs::projs[i];
			if(p && p->state == CS_ALIVE && p->projtype == PRJ_SHOT && weaptype[p->weap].explode[p->flags&HIT_ALT ? 1 : 0])
			{
				float limit = enttype[WAYPOINT].radius+(weaptype[p->weap].explode[p->flags&HIT_ALT ? 1 : 0]*p->lifesize);
                obs.avoidnear(p, p->o, limit);
			}
		}
		loopv(entities::ents) if(entities::ents[i]->type == MAPMODEL && entities::ents[i]->lastemit < 0 && !entities::ents[i]->spawned)
		{
			mapmodelinfo &mmi = getmminfo(entities::ents[i]->attrs[0]);
			vec center, radius;
			mmi.m->collisionbox(0, center, radius);
			if(!mmi.m->ellipsecollide) rotatebb(center, radius, int(entities::ents[i]->attrs[1]));
			float limit = enttype[WAYPOINT].radius+(max(radius.x, max(radius.y, radius.z))*mmi.m->height);
			vec pos = entities::ents[i]->o; pos.z += limit*0.5f;
			obs.avoidnear(NULL, pos, limit);
		}
	}

	void think(gameent *d, bool run)
	{
		// the state stack works like a chain of commands, certain commands simply replace each other
		// others spawn new commands to the stack the ai reads the top command from the stack and executes
		// it or pops the stack and goes back along the history until it finds a suitable command to execute
		if(d->ai->state.empty()) setup(d, false, d->aientity);
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
			if(d->state == CS_DEAD && (d->respawned < 0 || lastmillis-d->respawned > 30000) && (!d->lastdeath || (!m_story(game::gamemode) && lastmillis-d->lastdeath > 500)))
			{
				if(m_arena(game::gamemode, game::mutators)) client::addmsg(SV_ARENAWEAP, "ri2", d->clientnum, d->arenaweap);
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
					d->ai->clear(true);
					if(c.type != AI_S_WAIT)
					{
						switch(result)
						{
							case 0: default: d->ai->removestate(i); cleannext = true; break;
							case -1: i = d->ai->state.length()-1; break;
						}
						continue; // shouldn't interfere
					}
					else
					{
						if(d->aitype >= AI_START) d->ai->suspended = true;
						c.next = lastmillis+m_speedtime(1000+rnd(1000));
					}
				}
				else
				{
					d->ai->suspended = false;
					c.next = lastmillis+m_speedtime(500+rnd(500));
				}
			}
			logic(d, c, run);
			break;
		}
		if(d->ai->trywipe) d->ai->wipe();
	}

	void drawstate(gameent *d, aistate &b, bool top, int above)
	{
		const char *bnames[AI_S_MAX] = {
			"wait", "defend", "pursue", "interest"
		}, *btypes[AI_T_MAX+1] = {
			"none", "node", "player", "affinity", "entity", "drop"
		};
		mkstring(s);
		if(top)
		{
			formatstring(s)("@<default>\fg%s (%d[%d]) %s:%d (%d[%d])",
				bnames[b.type],
				lastmillis-b.millis, b.next-lastmillis,
				btypes[b.targtype+1], b.target,
				!d->ai->route.empty() ? d->ai->route[0] : -1,
				d->ai->route.length()
			);
		}
		else
		{
			formatstring(s)("@<sub>\fy%s (%d[%d]) %s:%d",
				bnames[b.type],
				lastmillis-b.millis, b.next-lastmillis,
				btypes[b.targtype+1], b.target
			);
		}
		if(s[0]) part_text(vec(d->abovehead()).add(vec(0, 0, above)), s);
	}

	void drawroute(gameent *d, aistate &b, float amt)
	{
		int colour = teamtype[owner(d)].colour, last = -1;
		loopvrev(d->ai->route)
		{
			if(d->ai->route.inrange(last))
			{
				int index = d->ai->route[i], prev = d->ai->route[last];
				if(entities::ents.inrange(index) && entities::ents.inrange(prev))
				{
					gameentity &e = *(gameentity *)entities::ents[index], &f = *(gameentity *)entities::ents[prev];
					vec fr = f.o, dr = e.o;
					fr.z += amt; dr.z += amt;
					part_trace(fr, dr, 1.f, 1, colour);
				}
			}
			last = i;
		}
		if(aidebug > 4)
		{
			vec fr = vec(d->feetpos()).add(vec(0, 0, amt));
			if(d->ai->spot != vec(0, 0, 0)) part_trace(fr, vec(d->ai->spot).add(vec(0, 0, 0.1f)), 1.f, 1, 0x008888);
			if(entities::ents.inrange(d->lastnode))
			{
				vec dr = vec(entities::ents[d->lastnode]->o).add(vec(0, 0, amt));
				part_trace(fr, dr, 1.f, 1, 0x884400);
				fr = dr;
			}
			loopi(NUMPREVNODES)
			{
				if(entities::ents.inrange(d->ai->prevnodes[i]))
				{
					vec dr = vec(entities::ents[d->ai->prevnodes[i]]->o).add(vec(0, 0, amt));
					part_trace(dr, fr, 1.f, 1, 0x442200);
					fr = dr;
				}
			}
		}
	}

	void render()
	{
		if(aidebug > 1)
		{
			int amt[2] = { 0, 0 };
			loopv(game::players) if(game::players[i] && game::players[i]->ai) amt[0]++;
			loopv(game::players) if(game::players[i] && game::players[i]->state == CS_ALIVE && game::players[i]->ai)
			{
				gameent *d = game::players[i];
				bool top = true;
				int above = 0;
				amt[1]++;
				loopvrev(d->ai->state)
				{
					aistate &b = d->ai->state[i];
					drawstate(d, b, top, above += 2);
					if(aidebug > 3 && top && rendernormally && b.type != AI_S_WAIT && aitype[d->aitype].maxspeed)
						drawroute(d, b, 4.f*(float(amt[1])/float(amt[0])));
					if(top)
					{
						if(aidebug > 2) top = false;
						else break;
					}
				}
			}
			if(aidebug >= 4 && !m_edit(game::gamemode))
			{
				int cur = 0;
				loopv(obs.obstacles)
				{
					const entities::avoidset::obstacle &ob = obs.obstacles[i];
					int next = cur + ob.numentities;
					for(; cur < next; cur++)
					{
						int ent = obs.entities[cur];
						gameentity &e = *(gameentity *)entities::ents[ent];
						part_create(PART_EDIT, 1, e.o, 0xFF6600, 2.f);
					}
					cur = next;
				}
			}
		}
	}
}
