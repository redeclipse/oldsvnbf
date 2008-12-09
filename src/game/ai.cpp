#ifdef GAMESERVER
namespace ai
{
	int findaiclient(int exclude)
	{
		vector<int> siblings;
		while(siblings.length() < clients.length()) siblings.add(-1);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->clientnum < 0 || ci->state.aitype != AI_NONE || !ci->name[0] || ci->clientnum == exclude)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(clients) if(clients[j]->state.aitype != AI_NONE && clients[j]->state.ownernum == ci->clientnum)
					siblings[i]++;
			}
		}
		while(!siblings.empty())
		{
			int q = -1;
			loopv(siblings)
				if(siblings[i] >= 0 && (!siblings.inrange(q) || siblings[i] < siblings[q]))
					q = i;
			if(siblings.inrange(q))
			{
				if(clients.inrange(q)) return clients[q]->clientnum;
				else siblings.removeunordered(q);
			}
			else break;
		}
		return -1;
	}

	bool addai(int type, int skill)
	{
		int cn = addclient(ST_REMOTE);
		if(cn >= 0 && isaitype(type))
		{
			clientinfo *ci = (clientinfo *)getinfo(cn), *cp = NULL;
			if(ci)
			{
				ci->clientnum = cn;
				ci->state.aitype = type;
				ci->state.ownernum = findaiclient();
				if(ci->state.ownernum >= 0 && ((cp = (clientinfo *)getinfo(ci->state.ownernum))))
				{
					int s = skill, m = sv_botmaxskill > sv_botminskill ? sv_botmaxskill : sv_botminskill,
						n = sv_botminskill < sv_botmaxskill ? sv_botminskill : sv_botmaxskill;
					if(skill > m || skill < n) s = (m != n ? rnd(m-n) + n + 1 : m);
					ci->state.skill = clamp(s, 1, 100);
					ci->state.state = CS_DEAD;
					clients.add(ci);
					ci->state.lasttimeplayed = lastmillis;
					s_strncpy(ci->name, aitype[ci->state.aitype].name, MAXNAMELEN);

					if(m_team(gamemode, mutators)) ci->team = chooseworstteam(ci);
					else ci->team = TEAM_NEUTRAL;

					sendf(-1, 1, "ri5si", SV_INITAI, ci->clientnum, ci->state.ownernum, ci->state.aitype, ci->state.skill, ci->name, ci->team);

					int nospawn = 0;
					if(smode && !smode->canspawn(ci, true)) { nospawn++; }
					mutate(smuts, if (!mut->canspawn(ci, true)) { nospawn++; });
					if(nospawn)
					{
						ci->state.state = CS_DEAD;
						sendf(-1, 1, "ri2", SV_FORCEDEATH, ci->clientnum);
					}
					else sendspawn(ci);

					ci->online = true;
					return true;
				}
			}
			delclient(cn);
		}
		return false;
	}

	void deleteai(clientinfo *ci)
	{
		int cn = ci->clientnum;
		if(smode) smode->leavegame(ci, true);
		mutate(smuts, mut->leavegame(ci));
		ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		savescore(ci);
		sendf(-1, 1, "ri2", SV_CDIS, cn);
		clients.removeobj(ci);
		delclient(cn);
	}

	bool delai(int type)
	{
		loopvrev(clients) if(clients[i]->state.aitype == type && clients[i]->state.ownernum >= 0)
		{
			deleteai(clients[i]);
			return true;
		}
		return false;
	}

	void reinitai(clientinfo *ci, int cn = -1, bool init = false)
	{
		if(cn >= 0) ci->state.ownernum = cn;
		if(ci->state.ownernum >= 0)
		{
			if(init)
			{
				if(smode) smode->leavegame(ci);
				mutate(smuts, mut->leavegame(ci));
			}

			if(m_team(gamemode, mutators)) ci->team = chooseworstteam(ci);
			else ci->team = TEAM_NEUTRAL;
			sendf(-1, 1, "ri5si", SV_INITAI, ci->clientnum, ci->state.ownernum, ci->state.aitype, ci->state.skill, ci->name, ci->team);

			if(init)
			{
				if(smode) smode->entergame(ci);
				mutate(smuts, mut->entergame(ci));
			}
		}
		else deleteai(ci);
	}

	void removeai(clientinfo *ci)
	{
		bool remove = !numclients(ci->clientnum, false, true);
		loopvrev(clients) if(clients[i]->state.ownernum == ci->clientnum)
		{
			if(remove) deleteai(clients[i]);
			else reinitai(clients[i], findaiclient(ci->clientnum), true); // try to reassign the ai to someone else
		}
	}

	bool reassignai(int exclude)
	{
		vector<int> siblings;
		while(siblings.length() < clients.length()) siblings.add(-1);
		int hi = -1, lo = -1;
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->clientnum < 0 || ci->state.aitype != AI_NONE || !ci->name[0] || ci->clientnum == exclude)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(clients) if(clients[j]->state.aitype != AI_NONE && clients[j]->state.ownernum == ci->clientnum)
					siblings[i]++;
				if(!siblings.inrange(hi) || siblings[i] > siblings[hi])
					hi = i;
				if(!siblings.inrange(lo) || siblings[i] < siblings[lo])
					lo = i;
			}
		}
		if(siblings.inrange(hi) && siblings.inrange(lo) && (siblings[hi]-siblings[lo]) > 1)
		{
			clientinfo *ci = clients[hi];
			loopvrev(clients) if(clients[i]->state.aitype != AI_NONE && clients[i]->state.ownernum == ci->clientnum)
			{
				reinitai(clients[i], clients[lo]->clientnum, true);
				return true;
			}
		}
		return false;
	}

	void checksetup()
	{
		int m = sv_botmaxskill > sv_botminskill ? sv_botmaxskill : sv_botminskill,
			n = sv_botminskill < sv_botmaxskill ? sv_botminskill : sv_botmaxskill;
		loopv(clients) if(clients[i]->state.aitype == AI_BOT)
		{
			clientinfo *cp = clients[i];
			bool reinit = false;
			if(cp->state.skill > m || cp->state.skill < n)
			{
				cp->state.skill = (m != n ? rnd(m-n) + n + 1 : m);
				reinit = true;
			}
			if(reinit) reinitai(cp);
		}
	}

	void clearbots()
	{
		loopvrev(clients) if(clients[i]->state.aitype != AI_NONE)
			deleteai(clients[i]);
	}

	void checkai()
	{
		if(m_demo(gamemode) || m_lobby(gamemode)) clearbots();
		else if(numclients(-1, false, true))
		{
			if(m_play(gamemode) && sv_botbalance)
			{
				int balance = clamp(sv_botbalance * (m_team(gamemode, mutators) ? numteams(gamemode, mutators) : 1), 0, 128);
				while(numclients(-1, true, false) < balance && addai(AI_BOT, -1)) ;
				while(numclients(-1, true, false) > balance && delai(AI_BOT)) ;
			}
			while(reassignai()) ;
			checksetup();
		}
	}

	void reqadd(clientinfo *ci, int skill)
	{
		if(haspriv(ci, PRIV_MASTER, true))
		{
			if(m_lobby(gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else if(m_play(gamemode) && sv_botbalance)
			{
				if(sv_botbalance < 32)
				{
					setvar("sv_botbalance", sv_botbalance+1, true);
					s_sprintfd(val)("%d", sv_botbalance);
					sendf(-1, 1, "ri2ss", SV_COMMAND, ci->clientnum, "botbalance", val);
				}
				else srvmsgf(ci->clientnum, "botbalance is at its highest");
			}
			else if(!addai(AI_BOT, skill))
				srvmsgf(ci->clientnum, "failed to create or assign bot");
		}
	}

	void reqdel(clientinfo *ci)
	{
		if(haspriv(ci, PRIV_MASTER, true))
		{
			if(m_lobby(gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else if(m_play(gamemode) && sv_botbalance)
			{
				if(sv_botbalance > 0)
				{
					setvar("sv_botbalance", sv_botbalance-1, true);
					s_sprintfd(val)("%d", sv_botbalance);
					sendf(-1, 1, "ri2ss", SV_COMMAND, ci->clientnum, "botbalance", val);
				}
				else srvmsgf(ci->clientnum, "botbalance is at its lowest");
			}
			else if(!delai(AI_BOT))
				srvmsgf(ci->clientnum, "failed to remove any bots");
		}
	}
}
#else
#include "pch.h"
#include "engine.h"
#include "game.h"
namespace ai
{
	entities::avoidset obstacles;
    int avoidmillis = 0, currentai = 0;

	VAR(aidebug, 0, 0, 5);

	ICOMMAND(addbot, "s", (char *s), client::addmsg(SV_ADDBOT, "ri", *s ? clamp(atoi(s), 1, 100) : -1));
	ICOMMAND(delbot, "", (), client::addmsg(SV_DELBOT, "r"));

	void create(gameent *d)
	{
		if(!d->ai && ((d->ai = new aiinfo()) == NULL))
			fatal("could not create ai");
	}

	void destroy(gameent *d)
	{
		if(d->ai) DELETEP(d->ai);
	}

	void init(gameent *d, int at, int on, int sk, int bn, char *name, int tm)
	{
		bool rst = false;
		gameent *o = world::getclient(on);
		string m, r; r[0] = 0;
		if(o) s_strcpy(m, world::colorname(o));
		else s_sprintf(m)("client %d", on);

		if(!d->name[0])
		{
			s_sprintf(r)("assigned to %s at skill %d", m, sk);
			rst = true;
		}
		else if(d->ownernum != on)
		{
			s_sprintf(r)("reassigned to %s", m);
			rst = true;
		}
		else if(d->skill != sk)
		{
			s_sprintf(r)("changed skill to %d", sk);
		}

		s_strncpy(d->name, name, MAXNAMELEN);
		d->aitype = at;
		d->ownernum = on;
		d->skill = sk;
		d->team = tm;

		if(r[0]) conoutf("\fg%s %s", world::colorname(d), r);

		if(rst) // only if connecting or changing owners
		{
			if(world::player1->clientnum == d->ownernum) create(d);
			else if(d->ai) destroy(d);
		}
	}

	void update()
	{
		int numai = 0;
		loopv(world::players) if(world::players[i] && world::players[i]->ai)
			numai++;
		if(numai)
		{
			if(lastmillis-avoidmillis > 100)
			{
				avoid();
				avoidmillis = lastmillis;
			}
			int idx = 0;
			if(currentai >= numai || currentai < 0) currentai = 0;
			loopv(world::players) if(world::players[i] && world::players[i]->ai)
			{
				think(world::players[i], idx);
				idx++;
			}
		}
	}

	bool hastarget(gameent *d, aistate &b, vec &pos)
	{ // add margins of error
		if(!rnd(d->skill*10)) return true;
		else
		{
			vec dir = world::feetpos(d, 0.f);
			dir.sub(pos);
			dir.normalize();
			float targyaw, targpitch;
			vectoyawpitch(dir, targyaw, targpitch);
			float rtime = d->skill*guntype[d->gunselect].rdelay/1000.f, atime = d->skill*guntype[d->gunselect].adelay/100.f,
					skew = float(lastmillis-b.millis)/(rtime+atime), cyaw = fabs(targyaw-d->yaw), cpitch = fabs(targpitch-d->pitch);
			if(cyaw <= AIFOVX(d->skill)*skew && cpitch <= AIFOVY(d->skill)*skew)
				return true;
		}
		return false;
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

	bool makeroute(gameent *d, aistate &b, vec &pos, float tolerance, bool dist)
	{
		int node = entities::entitynode(pos, dist);
		return makeroute(d, b, node, tolerance);
	}

	bool randomnode(gameent *d, aistate &b, vec &from, vec &to, float radius, float wander)
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
		vec feet = world::feetpos(d, 0.f);
		return randomnode(d, b, feet, feet, radius, wander);
	}

	bool patrol(gameent *d, aistate &b, vec &pos, float radius, float wander, bool retry)
	{
		vec feet = world::feetpos(d, 0.f);
		if(feet.squaredist(pos) <= radius*radius || !makeroute(d, b, pos, radius, wander))
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
				return patrol(d, b, pos, radius, wander, true);
			}
			return false;
		}
		return true;
	}

	bool violence(gameent *d, aistate &b, gameent *e, bool pursue)
	{
		if(AITARG(d, e, true))
		{
			vec epos(world::feetpos(e, 0.f));
			aistate &c = d->ai->addstate(pursue ? AI_S_PURSUE : AI_S_ATTACK);
			c.targtype = AI_T_PLAYER;
			c.defers = pursue;
			d->ai->enemy = c.target = e->clientnum;
			d->ai->lastseen = lastmillis;
			if(pursue) c.expire = clamp(d->skill*100, 1000, 10000);
			return true;
		}
		return false;
	}

	bool defer(gameent *d, aistate &b, bool pursue)
	{
		gameent *t = NULL, *e = NULL;
		vec targ, dp = world::headpos(d), tp = vec(0, 0, 0);
		loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && e != d && AITARG(d, e, true))
		{
			vec ep = world::headpos(e);
			if((!t || ep.squaredist(d->o) < tp.squaredist(d->o)) && (pursue || AICANSEE(dp, ep, d)))
			{
				t = e;
				tp = ep;
			}
		}
		if(t) return violence(d, b, t, pursue);
		return false;
	}

	void gunfind(gameent *d, aistate &b, vector<interest> &interests)
	{
		vec pos = world::headpos(d);
		loopvj(entities::ents)
		{
			gameentity &e = *(gameentity *)entities::ents[j];
			if(enttype[e.type].usetype != EU_ITEM) continue;
			int sgun = m_spawngun(world::gamemode, world::mutators), attr = gunattr(e.attr1, sgun);
			switch(e.type)
			{
				case WEAPON:
				{
					if(e.spawned && isgun(attr) && !d->hasgun(attr, sgun))
					{ // go get a weapon upgrade
						interest &n = interests.add();
						n.state = AI_S_INTEREST;
						n.node = entities::entitynode(e.o, true);
						n.target = j;
						n.targtype = AI_T_ENTITY;
						n.tolerance = enttype[e.type].radius+d->radius;
						n.score = pos.squaredist(e.o)/(attr != d->ai->gunpref ? 1.f : 10.f);
						n.defers = (d->gunselect != GUN_PLASMA);
					}
					break;
				}
				default: break;
			}
		}

		loopvj(projs::projs) if(projs::projs[j]->projtype == PRJ_ENT && projs::projs[j]->ready())
		{
			projent &proj = *projs::projs[j];
			if(enttype[proj.ent].usetype != EU_ITEM || !entities::ents.inrange(proj.id)) continue;
			int sgun = m_spawngun(world::gamemode, world::mutators), attr = gunattr(proj.attr1, sgun);
			switch(proj.ent)
			{
				case WEAPON:
				{
					if(isgun(attr) && !d->hasgun(attr, sgun))
					{ // go get a weapon upgrade
						if(proj.owner == d && d->gunselect != GUN_PLASMA) break;
						interest &n = interests.add();
						n.state = AI_S_INTEREST;
						n.node = entities::entitynode(proj.o, true);
						n.target = proj.id;
						n.targtype = AI_T_DROP;
						n.tolerance = enttype[proj.ent].radius+d->radius;
						n.score = pos.squaredist(proj.o)/(attr != d->ai->gunpref ? 1.f : 10.f);
						n.defers = (d->gunselect != GUN_PLASMA);
					}
					break;
				}
				default: break;
			}
		}
	}

	bool find(gameent *d, aistate &b, bool override = true)
	{
		static vector<interest> interests;
		interests.setsizenodelete(0);

		if(m_ctf(world::gamemode)) ctf::aifind(d, b, interests);
		if(m_stf(world::gamemode)) stf::aifind(d, b, interests);
		if(!d->hasgun(d->ai->gunpref, m_spawngun(world::gamemode, world::mutators))) gunfind(d, b, interests);
		while(!interests.empty())
		{
			int q = interests.length()-1;
			loopi(interests.length()-1) if(interests[i].score < interests[q].score) q = i;
			interest n = interests.removeunordered(q);
			if(makeroute(d, b, n.node, n.tolerance))
			{
				aistate &c = override ? d->ai->setstate(n.state) : d->ai->addstate(n.state);
				c.targtype = n.targtype;
				c.target = n.target;
				c.expire = n.expire;
				c.defers = n.defers;
				return true;
			}
		}
		return false;
	}

	bool decision(gameent *d, bool *result, bool *pursue, bool defend = true)
	{
		if(d->attacking) return false;
		aistate &b = d->ai->getstate();
		switch(b.type)
		{
			case AI_S_PURSUE:
			{
				*result = true;
				*pursue = false;
				return true;
			}
			case AI_S_WAIT:
			case AI_S_INTEREST:
			{
				*result = true;
				*pursue = true;
				return true;
			}
			case AI_S_DEFEND:
			{
				if(defend)
				{
					*result = true;
					*pursue = false;
				}
				else
				{
					*result = true;
					*pursue = true;
				}
				return true;
			}
			case AI_S_ATTACK: default: break;
		}
		return false;
	}

	void damaged(gameent *d, gameent *e, int gun, int flags, int damage, int health, int millis, vec &dir)
	{
		if(d->ai)
		{
			if(AITARG(d, e, true)) // see if this ai is interested in a grudge
			{
				bool r = false, p = false;
				if(decision(d, &r, &p, true) && r)
				{
					aistate &b = d->ai->getstate();
					violence(d, b, e, p);
				}
			}
		}

		vector<int> targets; // check if one of our ai is defending them
		if(checkothers(targets, d, AI_S_DEFEND, AI_T_PLAYER, d->clientnum, true))
		{
			gameent *t;
			loopv(targets) if((t = world::getclient(targets[i])) && t->ai && AITARG(t, e, true))
			{
				aistate &c = t->ai->getstate();
				violence(t, c, e, false);
			}
		}
	}

	void spawned(gameent *d)
	{
		if(d->ai) d->ai->reset();
	}

	void killed(gameent *d, gameent *e, int gun, int flags, int damage)
	{
		if(d->ai) d->ai->reset();
	}

	bool dowait(gameent *d, aistate &b)
	{
		if(d->state == CS_DEAD)
		{
			if(d->respawned != d->lifesequence && !world::respawnwait(d))
				world::respawnself(d);
			return true;
		}
		else if(d->state == CS_ALIVE)
		{
			if(d->timeinair && entities::ents.inrange(d->lastnode))
			{ // we need to make a quick decision to find a landing spot
				int closest = -1;
				gameentity &e = *(gameentity *)entities::ents[d->lastnode];
				if(!e.links.empty())
				{
					loopv(e.links) if(entities::ents.inrange(e.links[i]))
					{
						gameentity &f = *(gameentity *)entities::ents[e.links[i]];
						if(!entities::ents.inrange(closest) ||
							f.o.squaredist(d->o) < entities::ents[closest]->o.squaredist(d->o))
								closest = e.links[i];
					}
				}
				if(entities::ents.inrange(closest))
				{
					aistate &c = d->ai->setstate(AI_S_INTEREST);
					c.targtype = AI_T_NODE;
					c.target = closest;
					c.expire = 1000;
					c.defers = false;
					return true;
				}
			}
			if(m_ctf(world::gamemode) && ctf::aicheck(d, b)) return true;
			if(m_stf(world::gamemode) && stf::aicheck(d, b)) return true;
			if(find(d, b, true)) return true;
			if(defer(d, b, true)) return true;
			if(randomnode(d, b, AIISFAR, 1e16f))
			{
				aistate &c = d->ai->setstate(AI_S_INTEREST);
				c.targtype = AI_T_NODE;
				c.target = d->ai->route[0];
				c.expire = 10000;
				c.defers = true;
				return true;
			}
			if(b.cycle >= 10)
			{
				world::suicide(d, 0); // bail
				return true; // recycle and start from beginning
			}
		}
		return true; // but don't pop the state
	}

	bool dodefend(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			switch(b.targtype)
			{
				case AI_T_AFFINITY:
				{
					if(m_ctf(world::gamemode)) return ctf::aidefend(d, b);
					if(m_stf(world::gamemode)) return stf::aidefend(d, b);
					break;
				}
				case AI_T_PLAYER:
				{
					gameent *e = world::getclient(b.target);
					if(e && e->state == CS_ALIVE)
					{
						vec epos(world::feetpos(e, 0.f));
						return patrol(d, b, epos);
					}
					break;
				}
				default: break;
			}
		}
		return false;
	}

	bool doattack(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			vec targ, pos = world::headpos(d);
			gameent *e = world::getclient(b.target);
			if(e && e->state == CS_ALIVE && AITARG(d, e, true))
			{
				vec ep = world::headpos(e);
				bool cansee = AICANSEE(pos, ep, d);
				if(cansee || (d->ai->lastseen >= 0 && lastmillis-d->ai->lastseen > d->skill*25))
				{
					d->ai->enemy = e->clientnum;
					d->ai->lastseen = lastmillis;
					if(d->canshoot(d->gunselect, m_spawngun(world::gamemode, world::mutators), lastmillis) && hastarget(d, b, ep))
					{
						d->attacking = true;
						d->attacktime = lastmillis;
						return !b.override && !d->ai->route.empty();
					}
					if(b.defers || d->ai->route.empty())
					{
						vec epos(world::feetpos(e, 0.f));
						return patrol(d, b, epos);
					}
					return true;
				}
			}
		}
		return false;
	}

	bool dointerest(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			int sgun = m_spawngun(world::gamemode, world::mutators);
			switch(b.targtype)
			{
				case AI_T_ENTITY:
				{
					if(d->hasgun(d->ai->gunpref, sgun)) return false;
					if(entities::ents.inrange(b.target))
					{
						gameentity &e = *(gameentity *)entities::ents[b.target];
						if(enttype[e.type].usetype != EU_ITEM) return false;
						int attr = gunattr(e.attr1, sgun);
						switch(e.type)
						{
							case WEAPON:
							{
								if(!e.spawned || d->hasgun(attr, sgun)) return false;
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
					if(d->hasgun(d->ai->gunpref, sgun)) return false;
					loopvj(projs::projs) if(projs::projs[j]->projtype == PRJ_ENT && projs::projs[j]->ready() && projs::projs[j]->id == b.target)
					{
						projent &proj = *projs::projs[j];
						if(enttype[proj.ent].usetype != EU_ITEM || !entities::ents.inrange(proj.id)) return false;
						int attr = gunattr(proj.attr1, sgun);
						switch(proj.ent)
						{
							case WEAPON:
							{
								if(d->hasgun(attr, sgun)) return false;
								if(proj.owner == d && d->gunselect != GUN_PLASMA)
									return false;
								break;
							}
							default: break;
						}
						return makeroute(d, b, proj.o, enttype[proj.ent].radius);
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
					if(e && e->state == CS_ALIVE)
					{
						vec epos(world::feetpos(e, 0.f));
						return patrol(d, b, epos);
					}
					break;
				}
				default: break;
			}
		}
		return false;
	}

	int closenode(gameent *d)
	{
		vec pos = world::feetpos(d, 0.f);
		int node = -1;
		float mindist = 1e16f;
		loopvrev(d->ai->route) if(entities::ents.inrange(d->ai->route[i]) && !obstacles.find(d->ai->route[i], d))
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

	bool hunt(gameent *d, bool retry = false)
	{
		if(!d->ai->route.empty())
		{
			int n = retry ? closenode(d) : d->ai->route.find(d->lastnode);
			if(n > 0 && entities::ents.inrange(d->ai->route[n-1]) && !obstacles.find(d->ai->route[n-1], d))
			{
				gameentity &e = *(gameentity *)entities::ents[d->ai->route[n-1]];
				vec pos = world::feetpos(d);
				d->ai->spot = e.o;
				if((!d->timeinair && d->ai->spot.z-pos.z > AIJUMPHEIGHT) ||
					(d->timeinair && d->vel.z <= 4.f && physics::canimpulse(d))) // try to impulse at height of a jump
				{
					d->jumping = true;
					d->jumptime = lastmillis;
				}
				if(((e.attr1 & WP_CROUCH && !d->crouching) || d->crouching) && (lastmillis-d->crouchtime > 250))
				{
					d->crouching = !d->crouching;
					d->crouchtime = lastmillis;
				}
				d->ai->spot.z += d->height;
				return true;
			}
			if(n != 0 && !retry) return hunt(d, true);
			d->ai->route.setsize(0); // bail out
		}
		return false;
	}

	void aim(gameent *d, vec &pos, float &yaw, float &pitch, int skew = 0)
	{
		vec dp = world::headpos(d);
		float targyaw = -(float)atan2(pos.x-dp.x, pos.y-dp.y)/PI*180+180;
		if(yaw < targyaw-180.0f) yaw += 360.0f;
		if(yaw > targyaw+180.0f) yaw -= 360.0f;
		float dist = dp.dist(pos), targpitch = asin((pos.z-dp.z)/dist)/RAD;
		if(skew)
		{
			float amt = float(lastmillis-d->lastupdate)/float((111-d->skill)*(skew+rnd(skew))),
				offyaw = fabs(targyaw-yaw)*amt, offpitch = fabs(targpitch-pitch)*amt*0.25f;

			if(targyaw > yaw) // slowly turn ai towards target
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
			world::fixrange(yaw, pitch);
		}
		else
		{
			yaw = targyaw;
			pitch = targpitch;
		}
	}

	bool request(gameent *d, int busy)
	{
		int sgun = m_spawngun(world::gamemode, world::mutators);
		if(!busy && d->reqswitch < 0)
		{
			int gun = -1;
			if(d->hasgun(d->ai->gunpref, sgun)) gun = d->ai->gunpref; // could be any gun
			else loopi(GUN_MAX) if(d->hasgun(i, sgun, 1)) gun = i; // only choose carriables here
			if(gun != d->gunselect && d->canswitch(gun, sgun, lastmillis))
			{
				client::addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-world::maptime, gun);
				d->reqswitch = lastmillis;
				return true;
			}
		}

		if(!d->ammo[d->gunselect] && d->canreload(d->gunselect, sgun, lastmillis) && d->reqreload < 0)
		{
			client::addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-world::maptime, d->gunselect);
			d->reqreload = lastmillis;
			return true;
		}

		if(busy <= 1 && !d->hasgun(d->ai->gunpref, sgun) && !d->useaction && d->requse < 0)
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
						if(enttype[e.type].usetype != EU_ITEM) break;
						if(proj.owner == d && d->gunselect != GUN_PLASMA) break;
						ent = proj.id;
						break;
					}
					default: break;
				}
				if(entities::ents.inrange(ent))
				{
					extentity &e = *entities::ents[ent];
					if(d->canuse(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, sgun, lastmillis)) switch(e.type)
					{
						case WEAPON:
						{
							int attr = gunattr(e.attr1, sgun);
							if(d->hasgun(attr, sgun)) break;
							if(d->gunselect != GUN_PLASMA && attr != d->ai->gunpref)
								break;
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

		if(!busy && d->canreload(d->gunselect, sgun, lastmillis) && d->reqreload < 0)
		{
			client::addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-world::maptime, d->gunselect);
			d->reqreload = lastmillis;
			return true;
		}

		return false;
	}

	bool process(gameent *d)
	{
		bool aiming = false;
		gameent *e = world::getclient(d->ai->enemy);
		if(e && e->state == CS_ALIVE)
		{
			vec targ, dp = world::headpos(d), ep = world::headpos(e);
			bool cansee = AICANSEE(dp, ep, d);
			if(cansee || (d->ai->lastseen >= 0 && lastmillis-d->ai->lastseen > d->skill*25))
			{
				if(cansee) d->ai->lastseen = lastmillis;
				aim(d, ep, d->yaw, d->pitch, 5);
				aiming = true;
			}
		}

		if(hunt(d))
		{
			if(!aiming) aim(d, d->ai->spot, d->yaw, d->pitch, 10);
			aim(d, d->ai->spot, d->aimyaw, d->aimpitch);

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
			const aimdir &ad = aimdirs[(int)floor((d->aimyaw - d->yaw + 22.5f)/45.0f) & 7];
			d->move = ad.move;
			d->strafe = ad.strafe;
			d->aimyaw -= ad.offset;
		}
		else
		{
			d->stopmoving();
			d->move = 1; // walk forward
		}
		world::fixrange(d->aimyaw, d->aimpitch);
		return aiming;
	}

	void check(gameent *d)
	{
		vec pos = world::headpos(d);
		findorientation(pos, d->yaw, d->pitch, d->ai->target);

		if(world::allowmove(d) && d->state == CS_ALIVE)
		{
			bool r = false, p = false;
			int busy = process(d) ? 1 : 0;
			if(!decision(d, &r, &p, false) || !r) busy = 2;
			if(!request(d, busy) && !busy)
			{
				aistate &b = d->ai->getstate();
				defer(d, b, false);
			}
			entities::checkitems(d);
			weapons::shoot(d, d->ai->target, guntype[d->gunselect].power); // always use full power
		}
		else d->stopmoving();

		physics::move(d, 10, true);
		d->attacking = d->jumping = d->reloading = d->useaction = false;
	}

	void avoid()
	{
		// guess as to the radius of ai and other critters relying on the avoid set for now
		float guessradius = world::player1->radius;

		obstacles.clear();
		loopi(world::numdynents())
		{
			gameent *d = (gameent *)world::iterdynents(i);
			if(!d || d->state != CS_ALIVE) continue;
			vec pos = world::feetpos(d, 0.f);
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
			if(p && p->state == CS_ALIVE && p->projtype == PRJ_SHOT && guntype[p->attr1].explode)
			{
				float limit = guessradius+(guntype[p->attr1].explode*p->lifesize);
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

	void think(gameent *d, int idx)
	{
		if(d->ai->state.empty()) d->ai->reset();

		// the state stack works like a chain of commands, certain commands simply replace
		// each other, others spawn new commands to the stack the ai reads the top command
		// from the stack and executes it or pops the stack and goes back along the history
		// until it finds a suitable command to execute.

		if(!world::intermission)
		{
			aistate &b = d->ai->getstate();
			if(idx == currentai)
			{
				bool override = d->state == CS_ALIVE && d->ai->route.empty(),
					expired = lastmillis >= b.next;
				if(override || expired)
				{
					bool result = true;
					int frame = 0;
					if(expired)
					{
						frame = aiframetimes[b.type]-d->skill;
						//extern int autoadjust, autoadjustlevel;
						//if(autoadjust) frame = frame*102/(autoadjustlevel+2);
						b.next = lastmillis + frame;
						b.cycle++;
					}
					if(override) b.override = false;
					switch(b.type)
					{
						case AI_S_WAIT:			result = dowait(d, b);		break;
						case AI_S_DEFEND:		result = dodefend(d, b);	break;
						case AI_S_PURSUE:		result = dopursue(d, b);	break;
						case AI_S_ATTACK:		result = doattack(d, b);	break;
						case AI_S_INTEREST:		result = dointerest(d, b);	break;
						default:				result = false;				break;
					}
					if(b.type != AI_S_WAIT)
					{
						if((expired && b.expire > 0 && (b.expire -= frame) <= 0) || !result)
						{
							d->ai->removestate();
						}
					}
					else if(result)
					{
						b.next = lastmillis;
						b.cycle = 0; // recycle the root of the command tree
					}
				}
				currentai++;
			}
			check(d);
		}
		else d->stopmoving();
		d->lastupdate = lastmillis;
	}

	void drawstate(gameent *d, aistate &b, bool top, int above)
	{
		const char *bnames[AI_S_MAX] = {
			"wait", "defend", "pursue", "attack", "interest"
		}, *btypes[AI_T_MAX+1] = {
			"none", "node", "player", "affinity", "entity", "drop"
		};
		s_sprintfd(s)("@%s%s [%d:%d:%d] goal:%d[%d] %s:%d",
			top ? "\fy" : "\fw",
			bnames[b.type],
			b.cycle, b.expire, b.next-lastmillis,
			!d->ai->route.empty() ? d->ai->route[0] : -1,
			d->ai->route.length(),
			btypes[b.targtype+1], b.target
		);
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
			vec pos = world::feetpos(d, 0.f);
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
#endif
