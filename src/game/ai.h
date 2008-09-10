#ifdef AISERV
struct aiserv
{
	gameserver &sv;
	aiserv(gameserver &_sv) : sv(_sv) {}

	void reqadd(clientinfo *ci, int skill)
	{
		if(sv.haspriv(ci, PRIV_MASTER, true))
		{
			if(m_lobby(sv.gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else if(m_fight(sv.gamemode) && sv_botbalance)
			{
				if(sv_botbalance < MAXCLIENTS-1)
				{
					setvar("sv_botbalance", sv_botbalance+1, true);
					s_sprintfd(val)("%d", sv_botbalance);
					sendf(-1, 1, "ri2ss", SV_COMMAND, ci->clientnum, "botbalance", val);
				}
				else sv.srvoutf(ci->clientnum, "botbalance is at its highest");
			}
			else if(!addai(AI_BOT, skill))
				sv.srvoutf(ci->clientnum, "failed to create or assign bot");
		}
	}

	void reqdel(clientinfo *ci)
	{
		if(sv.haspriv(ci, PRIV_MASTER, true))
		{
			if(m_lobby(sv.gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else if(m_fight(sv.gamemode) && sv_botbalance)
			{
				if(sv_botbalance > 1)
				{
					setvar("sv_botbalance", sv_botbalance-1, true);
					s_sprintfd(val)("%d", sv_botbalance);
					sendf(-1, 1, "ri2ss", SV_COMMAND, ci->clientnum, "botbalance", val);
				}
				else sv.srvoutf(ci->clientnum, "botbalance is at its lowest");
			}
			else if(!delai(AI_BOT))
				sv.srvoutf(ci->clientnum, "failed to remove any bots");
		}
	}

	int findaiclient()
	{
		vector<int> siblings;
		while(siblings.length() < sv.clients.length()) siblings.add(-1);
		loopv(sv.clients)
		{
			clientinfo *ci = sv.clients[i];
			if(ci->state.aitype != AI_NONE || !ci->name[0] || ci->state.state == CS_SPECTATOR)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(sv.clients) if(sv.clients[j]->state.aitype != AI_NONE && sv.clients[j]->state.ownernum == ci->clientnum)
					siblings[i]++;
			}
		}
		while(!siblings.empty())
		{
			int q = -1;
			loopv(siblings)
				if(siblings[i] >= 0 && (!siblings.inrange(q) || siblings[i] < siblings[q]))
					q = i;
			if(siblings.inrange(q)) return sv.clients[q]->clientnum;
			else if(siblings.inrange(q)) siblings.remove(q);
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
				if(ci->state.ownernum >= 0 && ((cp = (clientinfo *)getinfo(cn))))
				{
					int s = skill, m = sv_botmaxskill > sv_botminskill ? sv_botmaxskill : sv_botminskill,
						n = sv_botminskill < sv_botmaxskill ? sv_botminskill : sv_botmaxskill;
					if(skill > m || skill < n) s = (m != n ? rnd(m-n) + n + 1 : m);
					ci->state.skill = clamp(s, 1, 100);
					ci->state.state = CS_DEAD;
					sv.clients.add(ci);
					ci->state.lasttimeplayed = lastmillis;
					s_strncpy(ci->name, aitype[ci->state.aitype].name, MAXNAMELEN);

					if(m_team(sv.gamemode, sv.mutators)) ci->team = sv.chooseworstteam(ci);
					else ci->team = TEAM_NEUTRAL;

					sendf(-1, 1, "ri5si", SV_INITAI, ci->state.aitype, ci->state.ownernum, ci->state.skill, ci->clientnum, ci->name, ci->team);
					s_sprintfd(o)("%s", sv.colorname(cp));
					sv.relayf("\fg* %s assigned to %s at skill %d", sv.colorname(ci), o, ci->state.skill);

					if(ci->state.state != CS_SPECTATOR)
					{
						int nospawn = 0;
						if(sv.smode && !sv.smode->canspawn(ci, true)) { nospawn++; }
						mutate(sv.smuts, if (!mut->canspawn(ci, true)) { nospawn++; });

						if(nospawn)
						{
							ci->state.state = CS_DEAD;
							sendf(-1, 1, "ri2", SV_FORCEDEATH, ci->clientnum);
						}
						else sv.sendspawn(ci);
					}
					return true;
				}
				sv.clients.removeobj(ci);
			}
			delclient(cn);
		}
		return false;
	}

	void deleteai(clientinfo *ci)
	{
		int cn = ci->clientnum;
		if(sv.smode) sv.smode->leavegame(ci, true);
		mutate(sv.smuts, mut->leavegame(ci));
		ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		sv.savescore(ci);
		sendf(-1, 1, "ri2", SV_CDIS, cn);
		sv.relayf("\fo* %s has left the game", sv.colorname(ci));
		sv.clients.removeobj(ci);
		delclient(cn);
	}

	bool delai(int type)
	{
		loopvrev(sv.clients) if(sv.clients[i]->state.aitype == type && sv.clients[i]->state.ownernum >= 0)
		{
			deleteai(sv.clients[i]);
			return true;
		}
		return false;
	}

	void removeai(clientinfo *ci)
	{
		loopvrev(sv.clients) if(sv.clients[i]->state.ownernum == ci->clientnum)
			deleteai(sv.clients[i]);
	}

	bool reassignai()
	{
		vector<int> siblings;
		while(siblings.length() < sv.clients.length()) siblings.add(-1);
		int hi = -1, lo = -1;
		loopv(sv.clients)
		{
			clientinfo *ci = sv.clients[i];
			if(ci->state.aitype != AI_NONE || !ci->name[0] || ci->state.state == CS_SPECTATOR)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(sv.clients) if(sv.clients[j]->state.aitype != AI_NONE && sv.clients[j]->state.ownernum == ci->clientnum)
					siblings[i]++;
				if(!siblings.inrange(hi) || siblings[i] > siblings[hi])
					hi = i;
				if(!siblings.inrange(lo) || siblings[i] < siblings[lo])
					lo = i;
			}
		}
		if(siblings.inrange(hi) && siblings.inrange(lo) && (siblings[hi]-siblings[lo]) > 1)
		{
			clientinfo *ci = sv.clients[hi];
			loopvrev(sv.clients) if(sv.clients[i]->state.aitype != AI_NONE && sv.clients[i]->state.ownernum == ci->clientnum)
			{
				clientinfo *cp = sv.clients[i];
				cp->state.ownernum = sv.clients[lo]->clientnum;
				if(cp->state.ownernum >= 0)
				{
					sendf(-1, 1, "ri5si", SV_INITAI, cp->state.aitype, cp->state.ownernum, cp->state.skill, cp->clientnum, cp->name, cp->team);
					s_sprintfd(o)("%s", sv.colorname(ci));
					sv.relayf("\fg* %s reassigned to %s", sv.colorname(cp), o);
					return true;
				}
				else deleteai(cp);
			}
		}
		return false;
	}

	void checkskills()
	{
		int m = sv_botmaxskill > sv_botminskill ? sv_botmaxskill : sv_botminskill,
			n = sv_botminskill < sv_botmaxskill ? sv_botminskill : sv_botmaxskill;
		loopv(sv.clients) if(sv.clients[i]->state.aitype == AI_BOT && (sv.clients[i]->state.skill > m || sv.clients[i]->state.skill < n))
		{
			clientinfo *cp = sv.clients[i];
			cp->state.skill = (m != n ? rnd(m-n) + n + 1 : m);
			sendf(-1, 1, "ri5si", SV_INITAI, cp->state.aitype, cp->state.ownernum, cp->state.skill, cp->clientnum, cp->name, cp->team);
			sv.relayf("\fg* %s changed skill to %d", sv.colorname(cp), cp->state.skill);
		}
	}

	void checkai(bool renew = false)
	{
		if(m_lobby(sv.gamemode))
		{
			loopvrev(sv.clients) if(sv.clients[i]->state.aitype != AI_NONE)
				deleteai(sv.clients[i]);
		}
		else if(sv.nonspectators())
		{
			checkskills();
			if(m_fight(sv.gamemode))
			{
				if(sv_botbalance)
				{
					while(sv.nonspectators() < sv_botbalance && addai(AI_BOT, -1)) ;
					while(sv.nonspectators() > sv_botbalance && delai(AI_BOT)) ;
				}
			}
			while(reassignai()) ;
		}
	}
} ai;
#else
struct aiclient
{
	gameclient &cl;
    avoidset obstacles;
    int avoidmillis;

	static const int AIISNEAR			= 64;			// is near
	static const int AIISFAR			= 128;			// too far
	static const int AIJUMPHEIGHT		= 8;			// decides to jump
	static const int AIJUMPIMPULSE		= 16;			// impulse to jump
	static const int AILOSMIN			= 64;			// minimum line of sight
	static const int AILOSMAX			= 4096;			// maximum line of sight
	static const int AIFOVMIN			= 90;			// minimum field of view
	static const int AIFOVMAX			= 130;			// maximum field of view

	IVAR(aidebug, 0, 0, 5);

	#define AILOSDIST(x)			clamp((AILOSMIN+(AILOSMAX-AILOSMIN))/100.f*float(x), float(AILOSMIN), float(getvar("fog")+AILOSMIN))
	#define AIFOVX(x)				clamp((AIFOVMIN+(AIFOVMAX-AIFOVMIN))/100.f*float(x), float(AIFOVMIN), float(AIFOVMAX))
	#define AIFOVY(x)				AIFOVX(x)*3.f/4.f
    #define AIMAYTARG(y)           ((y)->state == CS_ALIVE && lastmillis-(y)->lastspawn > REGENWAIT)
	#define AITARG(x,y,z)			((y) != (x) && AIMAYTARG(y) && (!z || AVOIDENEMY(x, y)))

	aiclient(gameclient &_cl) : cl(_cl), obstacles(_cl), avoidmillis(0)
	{
		CCOMMAND(addbot, "s", (aiclient *self, char *s),
			self->addbot(*s ? clamp(atoi(s), 1, 100) : -1)
		);
		CCOMMAND(delbot, "", (aiclient *self), self->delbot());
	}

	void addbot(int s) { cl.cc.addmsg(SV_ADDBOT, "ri", s); }
	void delbot() { cl.cc.addmsg(SV_DELBOT, "r"); }

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
		gameent *o = cl.getclient(on);
		s_sprintfd(m)("%s", o ? cl.colorname(o) : "unknown");
		string r; r[0] = 0;

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
		else if(d->team != tm)
		{
			s_sprintf(r)("balanced to \fs%s%s\fS team", teamtype[tm].chat, teamtype[tm].name);
		}

		s_strncpy(d->name, name, MAXNAMELEN);
		d->aitype = at;
		d->ownernum = on;
		d->skill = sk;
		d->team = tm;

		if(r[0]) conoutf("\fg* %s %s", cl.colorname(d), r);

		if(rst) // only if connecting or changing owners
		{
			if(cl.player1->clientnum == d->ownernum) create(d);
			else if(d->ai) destroy(d);
		}
	}

	void update()
	{
		bool avoided = false;
		loopv(cl.players) if(cl.players[i] && cl.players[i]->ai)
		{
			if(!avoided)
			{
				avoid();
				avoided = true;
			}
			think(cl.players[i]);
		}
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

	bool hastarget(gameent *d, aistate &b, vec &pos)
	{ // add margins of error
		if(!rnd(d->skill*10)) return true;
		else
		{
			vec dir(cl.feetpos(d, 0.f));
			dir.sub(pos);
			dir.normalize();
			float targyaw, targpitch;
			vectoyawpitch(dir, targyaw, targpitch);
			int rdelay = guntype[d->gunselect].rdelay > 0 ? guntype[d->gunselect].rdelay : guntype[d->gunselect].adelay*10;
			float rtime = d->skill*rdelay/1000.f, atime = d->skill*guntype[d->gunselect].adelay/100.f,
					skew = float(lastmillis-b.millis)/(rtime+atime), cyaw = fabs(targyaw-d->yaw), cpitch = fabs(targpitch-d->pitch);
			if(cyaw <= AIFOVX(d->skill)*skew && cpitch <= AIFOVY(d->skill)*skew)
				return true;
		}
		return false;
	}

	bool checkothers(vector<int> &targets, gameent *d = NULL, int state = -1, int targtype = -1, int target = -1, bool teams = false)
	{ // checks the states of other ai for a match
		targets.setsize(0);
		gameent *e = NULL;
		loopi(cl.numdynents()) if((e = (gameent *)cl.iterdynents(i)) && e != d && e->ai && e->state == CS_ALIVE)
		{
			if(targets.find(e->clientnum) >= 0) continue;
			if(teams && m_team(cl.gamemode, cl.mutators) && d && d->team != e->team) continue;

			aistate &b = e->ai->getstate();
			if(state >= 0 && b.type != state) continue;
			if(target >= 0 && b.target != target) continue;
			if(targtype >=0 && b.targtype != targtype) continue;
			targets.add(e->clientnum);
		}
		return !targets.empty();
	}

	bool makeroute(gameent *d, aistate &b, int node, float tolerance = 0.f)
	{
		if(cl.et.route(d, d->lastnode, node, d->ai->route, obstacles, tolerance))
		{
			b.goal = false;
			b.override = false;
			return true;
		}
		return false;
	}

	bool makeroute(gameent *d, aistate &b, vec &pos, float tolerance = 0.f, bool dist = false)
	{
		int node = cl.et.entitynode(pos, dist);
		return makeroute(d, b, node, tolerance);
	}

	bool randomnode(gameent *d, aistate &b, vec &from, vec &to, float radius, float wander)
	{
		static vector<int> entities;
		entities.setsizenodelete(0);
		float r = radius*radius, w = wander*wander;
		loopvj(cl.et.ents) if(cl.et.ents[j]->type == WAYPOINT && j != d->lastnode)
		{
			float fdist = cl.et.ents[j]->o.squaredist(from);
			if(fdist <= w)
			{
				float tdist = cl.et.ents[j]->o.squaredist(to);
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
		vec feet(cl.feetpos(d, 0.f));
		return randomnode(d, b, feet, feet, radius, wander);
	}

	bool patrol(gameent *d, aistate &b, vec &pos, float radius, float wander, bool retry = false)
	{
		vec feet(cl.feetpos(d, 0.f));
		if(b.override || feet.squaredist(pos) <= radius*radius || !makeroute(d, b, pos))
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

	bool violence(gameent *d, aistate &b, gameent *e, bool pursue = false)
	{
		if(AITARG(d, e, true))
		{
			vec epos(cl.feetpos(e, 0.f));
			if(!pursue || patrol(d, b, epos, AIISNEAR, AIISFAR))
			{
				aistate &c = d->ai->addstate(pursue ? AI_S_PURSUE : AI_S_ATTACK);
				c.targtype = AI_T_PLAYER;
				c.defers = b.defers;
				d->ai->enemy = c.target = e->clientnum;
				if(pursue) c.expire = 10000;
				return true;
			}
			if(pursue) return violence(d, b, e, false);
		}
		return false;
	}

	bool defer(gameent *d, aistate &b, bool pursue = false)
	{
		gameent *t = NULL, *e = NULL;
		vec targ;
		loopi(cl.numdynents()) if((e = (gameent *)cl.iterdynents(i)) && e != d && AITARG(d, e, true))
		{
			vec ep = cl.headpos(e), tp = t ? cl.headpos(t) : vec(0, 0, 0),
				dp = cl.headpos(d);
			if((!t || ep.squaredist(d->o) < tp.squaredist(d->o)) &&
				getsight(dp, d->yaw, d->pitch, ep, targ, AILOSDIST(d->skill), AIFOVX(d->skill), AIFOVY(d->skill)))
					t = e;
		}
		if(t) return violence(d, b, t, pursue);
		return false;
	}

	struct interest
	{
		int state, node, target, targtype, expire;
		float tolerance, score;
		bool defers;
		interest() : state(-1), node(-1), target(-1), targtype(-1), expire(0), tolerance(0.f), score(0.f) {}
		~interest() {}
	};

	bool find(gameent *d, aistate &b, bool override = true)
	{
		vec pos = cl.headpos(d);
		static vector<interest> interests;
		interests.setsizenodelete(0);

		if(m_ctf(cl.gamemode))
        {
			loopvj(cl.ctf.flags)
			{
				ctfstate::flag &f = cl.ctf.flags[j];

				vector<int> targets; // build a list of others who are interested in this
				checkothers(targets, d, f.team == d->team ? AI_S_DEFEND : AI_S_PURSUE, AI_T_FLAG, j, true);

				gameent *e = NULL;
				loopi(cl.numdynents()) if((e = (gameent *)cl.iterdynents(i)) && AITARG(d, e, false) && !e->ai && d->team == e->team)
				{ // try to guess what non ai are doing
					vec ep = cl.headpos(e);
					if(targets.find(e->clientnum) < 0 && (ep.squaredist(f.pos()) <= ((enttype[FLAG].radius*enttype[FLAG].radius)*2.f) || f.owner == e))
						targets.add(e->clientnum);
				}

				if(f.team == d->team)
				{
					bool guard = false;
					if(f.owner || targets.empty()) guard = true;
					else if(d->gunselect != GUN_PISTOL)
					{ // see if we can relieve someone who only has a pistol
						gameent *t;
						loopvk(targets) if((t = cl.getclient(targets[k])) && t->gunselect == GUN_PISTOL)
						{
							guard = true;
							break;
						}
					}

					if(guard)
					{ // defend the flag
						interest &n = interests.add();
						n.state = AI_S_DEFEND;
						n.node = cl.et.entitynode(f.pos(), false);
						n.target = j;
						n.targtype = AI_T_FLAG;
						n.expire = 10000;
						n.tolerance = enttype[FLAG].radius*2.f;
						n.score = pos.squaredist(f.pos())/(d->gunselect != GUN_PISTOL ? 100.f : 1.f);
						n.defers = false;
					}
				}
				else
				{
					if(targets.empty())
					{ // attack the flag
						interest &n = interests.add();
						n.state = AI_S_PURSUE;
						n.node = cl.et.entitynode(f.pos(), false);
						n.target = j;
						n.targtype = AI_T_FLAG;
						n.expire = 10000;
						n.tolerance = enttype[FLAG].radius*2.f;
						n.score = pos.squaredist(f.pos());
						n.defers = false;
					}
					else
					{ // help by defending the attacker
						gameent *t;
						loopvk(targets) if((t = cl.getclient(targets[k])))
						{
							interest &n = interests.add();
							vec tp = cl.headpos(t);
							n.state = AI_S_DEFEND;
							n.node = t->lastnode;
							n.target = t->clientnum;
							n.targtype = AI_T_PLAYER;
							n.expire = 5000;
							n.tolerance = t->radius*2.f;
							n.score = pos.squaredist(tp);
							n.defers = false;
						}
					}
				}
			}
		}

		if(!d->hasgun(d->ai->gunpref))
		{
			loopvj(cl.et.ents)
			{
				gameentity &e = *(gameentity *)cl.et.ents[j];
				if(enttype[e.type].usetype != EU_ITEM) continue;
				switch(e.type)
				{
					case WEAPON:
					{
						if(e.spawned && isgun(e.attr1) && !d->hasgun(e.attr1))
						{ // go get a weapon upgrade
							interest &n = interests.add();
							n.state = AI_S_INTEREST;
							n.node = cl.et.entitynode(e.o, true);
							n.target = j;
							n.targtype = AI_T_ENTITY;
							n.expire = 10000;
							n.tolerance = enttype[e.type].radius+d->radius;
							n.score = pos.squaredist(e.o)/(e.attr1 != d->ai->gunpref ? 1.f : 10.f);
							n.defers = true;
						}
						break;
					}
					default: break;
				}
			}

			loopvj(cl.pj.projs) if(cl.pj.projs[j]->projtype == PRJ_ENT && cl.pj.projs[j]->ready())
			{
				projent &proj = *cl.pj.projs[j];
				if(enttype[proj.ent].usetype != EU_ITEM || !cl.et.ents.inrange(proj.id)) continue;
				switch(proj.ent)
				{
					case WEAPON:
					{
						if(isgun(proj.attr1) && !d->hasgun(proj.attr1))
						{ // go get a weapon upgrade
							if(proj.owner == d && d->gunselect != GUN_PISTOL) break;
							interest &n = interests.add();
							n.state = AI_S_INTEREST;
							n.node = cl.et.entitynode(proj.o, true);
							n.target = proj.id;
							n.targtype = AI_T_DROP;
							n.expire = 5000;
							n.tolerance = enttype[proj.ent].radius+d->radius;
							n.score = pos.squaredist(proj.o)/(proj.attr1 != d->ai->gunpref ? 1.f : 10.f);
							n.defers = true;
						}
						break;
					}
					default: break;
				}
			}
		}

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

	bool ctfhomerun(gameent *d, aistate &b)
	{
		vec pos = cl.headpos(d);
		loopk(2)
		{
			int goal = -1;
			loopv(cl.ctf.flags)
			{
				ctfstate::flag &g = cl.ctf.flags[i];
				if(g.team == d->team && (k || (!g.owner && !g.droptime)) &&
					(!cl.ctf.flags.inrange(goal) || g.pos().squaredist(pos) < cl.ctf.flags[goal].pos().squaredist(pos)))
				{
					goal = i;
				}
			}

			if(cl.ctf.flags.inrange(goal) && makeroute(d, b, cl.ctf.flags[goal].pos(), enttype[FLAG].radius))
			{
				aistate &c = d->ai->setstate(AI_S_PURSUE); // replaces current state!
				c.targtype = AI_T_FLAG;
				c.target = goal;
				c.defers = false;
				return true;
			}
		}
		return false;
	}

	void damaged(gameent *d, gameent *e, int gun, int flags, int damage, int health, int millis, vec &dir)
	{
		if(AITARG(d, e, true))
		{
			if(d->ai)
			{
				aistate &b = d->ai->getstate();
				bool result = false, pursue = false;
				switch(b.type)
				{
					case AI_S_PURSUE:
						result = true;
						pursue = false;
						break;
					case AI_S_WAIT:
					case AI_S_INTEREST:
						result = true;
						pursue = true;
						break;
					case AI_S_ATTACK:
					case AI_S_DEFEND: default: break;
				}
				if(result) violence(d, b, e, pursue);
			}
			vector<int> targets;
			if(checkothers(targets, d, AI_S_DEFEND, AI_T_PLAYER, d->clientnum, true))
			{
				gameent *t;
				loopv(targets) if((t = cl.getclient(targets[i])) && t->ai)
				{
					aistate &c = t->ai->getstate();
					violence(t, c, e, false);
				}
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
			if(d->respawned != d->lifesequence && !cl.respawnwait(d))
				cl.respawnself(d);
			return true;
		}
		else if(d->state == CS_ALIVE)
		{
			if(d->timeinair && cl.et.ents.inrange(d->lastnode))
			{ // we need to make a quick decision to find a landing spot
				int closest = -1;
				gameentity &e = *(gameentity *)cl.et.ents[d->lastnode];
				if(!e.links.empty())
				{
					loopv(e.links) if(cl.et.ents.inrange(e.links[i]))
					{
						gameentity &f = *(gameentity *)cl.et.ents[e.links[i]];
						if(!cl.et.ents.inrange(closest) ||
							f.o.squaredist(d->o) < cl.et.ents[closest]->o.squaredist(d->o))
								closest = e.links[i];
					}
				}
				if(cl.et.ents.inrange(closest))
				{
					aistate &c = d->ai->setstate(AI_S_INTEREST);
					c.targtype = AI_T_NODE;
					c.target = closest;
					c.expire = 1000;
					c.defers = false;
					return true;
				}
			}
			if(m_ctf(cl.gamemode))
			{
				static vector<int> hasflags;
				hasflags.setsizenodelete(0);
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
			if(randomnode(d, b, AIISFAR, 1e16f))
			{
				aistate &c = d->ai->setstate(AI_S_INTEREST);
				c.targtype = AI_T_NODE;
				c.target = d->ai->route[0];
				c.expire = 10000;
				c.defers = true;
				return true;
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
				case AI_T_FLAG:
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
				case AI_T_PLAYER:
				{
					gameent *e = cl.getclient(b.target);
					if(e)
					{
						vec epos(cl.feetpos(e, 0.f));
						if(e->state == CS_ALIVE && patrol(d, b, epos, AIISNEAR, AIISFAR))
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

	bool doattack(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			vec targ, pos = cl.headpos(d);
			gameent *e = cl.getclient(b.target);
			if(e && AITARG(d, e, true))
			{
				vec ep = cl.headpos(e);
				if(e->state == CS_ALIVE && raycubelos(pos, ep, targ))
				{
					d->ai->enemy = e->clientnum;
					if(m_fight(cl.gamemode) && d->canshoot(d->gunselect, lastmillis) && hastarget(d, b, ep))
					{
						d->attacking = true;
						d->attacktime = lastmillis;
						return !b.override && !b.goal;
					}
					if(b.defers && b.goal)
					{
						vec epos(cl.feetpos(e, 0.f));
						return patrol(d, b, epos, AIISNEAR, AIISFAR);
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
			switch(b.targtype)
			{
				case AI_T_ENTITY:
				{
					if(cl.et.ents.inrange(b.target))
					{
						gameentity &e = *(gameentity *)cl.et.ents[b.target];
						if(enttype[e.type].usetype != EU_ITEM) return false;
						switch(e.type)
						{
							case WEAPON:
							{
								if(d->hasgun(d->ai->gunpref))
									return false;
								if(!e.spawned || d->hasgun(e.attr1))
									return false;
								break;
							}
							default: break;
						}
						if(makeroute(d, b, e.o, enttype[e.type].radius))
						{
							defer(d, b, b.targtype == AI_T_NODE);
							return true;
						}
					}
					break;
				}
				case AI_T_DROP:
				{
					loopvj(cl.pj.projs) if(cl.pj.projs[j]->projtype == PRJ_ENT && cl.pj.projs[j]->ready() && cl.pj.projs[j]->id == b.target)
					{
						projent &proj = *cl.pj.projs[j];
						if(enttype[proj.ent].usetype != EU_ITEM || !cl.et.ents.inrange(proj.id)) return false;
						switch(proj.ent)
						{
							case WEAPON:
							{
								if(d->hasgun(d->ai->gunpref))
									return false;
								if(d->hasgun(proj.attr1))
									return false;
								if(proj.owner == d && d->gunselect != GUN_PISTOL)
									return false;
								break;
							}
							default: break;
						}
						if(makeroute(d, b, proj.o, enttype[proj.ent].radius))
						{
							defer(d, b, false);
							return true;
						}
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
				case AI_T_FLAG:
				{
					if(m_ctf(cl.gamemode) && cl.ctf.flags.inrange(b.target))
					{
						ctfstate::flag &f = cl.ctf.flags[b.target];
						if(f.team == d->team)
						{
							static vector<int> hasflags;
							hasflags.setsizenodelete(0);
							loopv(cl.ctf.flags)
							{
								ctfstate::flag &g = cl.ctf.flags[i];
								if(g.team != d->team && g.owner == d)
									hasflags.add(i);
							}

							if(hasflags.empty())
								return false; // otherwise why are we pursuing home?

							if(makeroute(d, b, f.pos(), enttype[FLAG].radius))
							{
								defer(d, b, false);
								return true;
							}
						}
						else
						{
							if(f.owner == d) return ctfhomerun(d, b);
							if(makeroute(d, b, f.pos(), enttype[FLAG].radius))
							{
								defer(d, b, false);
								return true;
							}
						}
					}
					break;
				}

				case AI_T_PLAYER:
				{
					gameent *e = cl.getclient(b.target);
					if(e)
					{
						vec epos(cl.feetpos(e, 0.f));
						if(e->state == CS_ALIVE && patrol(d, b, epos, AIISNEAR, AIISFAR))
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

	int closenode(gameent *d, aistate &b)
	{
		vec pos(cl.feetpos(d, 0.f));
		int node = -1;
		float mindist = 1e16f;
		loopvrev(d->ai->route) if(cl.et.ents.inrange(d->ai->route[i]) && d->ai->route[i] != d->lastnode && d->ai->route[i] != d->oldnode)
		{
			gameentity &e = *(gameentity *)cl.et.ents[d->ai->route[i]];

			float dist = e.o.squaredist(pos);
			if(dist < mindist)
			{
				node = i;
				mindist = min(dist, (enttype[WAYPOINT].radius*enttype[WAYPOINT].radius)*10.0f);
			}
		}
		return node;
	}

	bool hunt(gameent *d, aistate &b, bool retry = false)
	{
		if(!d->ai->route.empty())
		{
			int n = d->ai->route.find(d->lastnode), g = d->ai->route[0];
			if(d->ai->route.inrange(n) && --n >= 0) // otherwise got to goal
			{
				if(!d->ai->route.inrange(n)) n = closenode(d, b);
				if(d->ai->route.inrange(n) && !obstacles.find(d->ai->route[n], d))
				{
					gameentity &e = *(gameentity *)cl.et.ents[d->ai->route[n]];
					b.goal = false;
					d->ai->spot = vec(e.o).add(vec(0, 1, d->height));
					vec pos = cl.feetpos(d);
					if(d->ai->spot.z-PLAYERHEIGHT-pos.z > AIJUMPHEIGHT && !d->jumping && !d->timeinair && lastmillis-d->jumptime > 1000)
					{
						d->jumping = true;
						d->jumptime = lastmillis;
					}
					if(((e.attr1 & WP_CROUCH && !d->crouching) || d->crouching) && (lastmillis-d->crouchtime > 250))
					{
						d->crouching = !d->crouching;
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

	void aim(gameent *d, aistate &b, vec &pos, float &yaw, float &pitch, int skew)
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
			cl.fixrange(yaw, pitch);
		}
		else
		{
			yaw = targyaw;
			pitch = targpitch;
		}
	}

	void process(gameent *d, aistate &b)
	{
		if(d->state == CS_ALIVE)
		{
			if(lastmillis-d->ai->lastreq > 500)
			{
				int gun = -1;
				if(d->hasgun(d->ai->gunpref)) gun = d->ai->gunpref; // could be any gun
				else loopi(GUN_MAX) if(d->hasgun(i, 1)) gun = i; // only choose carriables here
				if(gun != d->gunselect && d->canswitch(gun, lastmillis))
				{
					cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, gun);
					d->ai->lastreq = lastmillis;
				}
				else if(!d->ammo[d->gunselect] && d->canreload(d->gunselect, lastmillis))
				{
					cl.cc.addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
					d->ai->lastreq = lastmillis;
				}
				else if(!d->hasgun(d->ai->gunpref) && !d->useaction)
				{
					static vector<actitem> actitems;
                    actitems.setsizenodelete(0);
					if(cl.et.collateitems(d, false, actitems))
					{
						int closest = actitems.length()-1;
						loopv(actitems)
							if(actitems[i].score < actitems[closest].score)
								closest = i;

						actitem &t = actitems[closest];
						int ent = -1;
						switch(t.type)
						{
							case ITEM_ENT:
							{
								if(!cl.et.ents.inrange(t.target)) break;
								extentity &e = *cl.et.ents[t.target];
								if(enttype[e.type].usetype != EU_ITEM) break;
								ent = t.target;
								break;
							}
							case ITEM_PROJ:
							{
								if(!cl.pj.projs.inrange(t.target)) break;
								projent &proj = *cl.pj.projs[t.target];
								if(!cl.et.ents.inrange(proj.id)) break;
								extentity &e = *cl.et.ents[proj.id];
								if(enttype[e.type].usetype != EU_ITEM) break;
								if(proj.owner == d && d->gunselect != GUN_PISTOL) break;
								ent = proj.id;
								break;
							}
							default: break;
						}
						if(cl.et.ents.inrange(ent))
						{
							extentity &e = *cl.et.ents[ent];
							if(d->canuse(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, lastmillis)) switch(e.type)
							{
								case WEAPON:
								{
									if(d->hasgun(e.attr1)) break;
									if(d->gunselect != GUN_PISTOL && e.attr1 != d->ai->gunpref)
										break;
									d->useaction = true;
									d->usetime = d->ai->lastreq = lastmillis;
									break;
								}
								default: break;
							}
						}
					}
				}
			}
			bool aiming = false;
			gameent *e = cl.getclient(d->ai->enemy);
			if(e)
			{
				vec enemypos = cl.headpos(e);
				aim(d, b, enemypos, d->yaw, d->pitch, 9);
				aiming = true;
			}
			if(hunt(d, b))
			{
				if(!aiming) aim(d, b, d->ai->spot, d->yaw, d->pitch, 6);
				aim(d, b, d->ai->spot, d->aimyaw, d->aimpitch, 3);
			}

            const struct aimdir { int move, strafe, offset; } aimdirs[8] =
            {
                {  1,  0,   0 },
                {  1,  1,  45 },
                {  0,  1,  90 },
                { -1,  1, 135 },
                { -1,  0, 180 },
                { -1, -1, 225 },
                {  0, -1, 270 },
                {  1, -1, 315 }
            };
            const aimdir &ad = aimdirs[(int)floor((d->aimyaw - d->yaw + 22.5f)/45.0f) & 7];
            d->move = ad.move;
            d->strafe = ad.strafe;
            d->aimyaw -= ad.offset;
			cl.fixrange(d->aimyaw, d->aimpitch);
		}
		else d->stopmoving();
	}

	void check(gameent *d)
	{
		vec pos = cl.headpos(d);
		findorientation(pos, d->yaw, d->pitch, d->ai->target);

		if(!cl.allowmove(d)) d->stopmoving();
		if(d->state == CS_ALIVE)
		{
			cl.et.checkitems(d);
			cl.ws.shoot(d, d->ai->target, 100); // always use full power
		}

		cl.ph.move(d, 10, true);
		d->attacking = d->jumping = d->reloading = d->useaction = false;
	}

	void avoid()
	{
		if(lastmillis-avoidmillis > 500) // only generate twice a second
		{
			// guess as to the radius of ai and other critters relying on the avoid set for now
			float guessradius = cl.player1->radius;

			obstacles.clear();
			loopi(cl.numdynents())
			{
				gameent *d = (gameent *)cl.iterdynents(i);
				if(!d || !AIMAYTARG(d)) continue;
				vec pos(cl.feetpos(d, 0.f));
				float limit = guessradius + d->radius;
				limit *= limit; // square it to avoid expensive square roots
				loopvk(cl.et.ents)
				{
					gameentity &e = *(gameentity *)cl.et.ents[k];
					if(e.type == WAYPOINT && e.o.squaredist(pos) <= limit)
						obstacles.add(d, k);
				}
			}
			loopv(cl.pj.projs)
			{
				projent *p = cl.pj.projs[i];
				if(p && p->state == CS_ALIVE && p->projtype == PRJ_SHOT)
				{
					float limit = guntype[p->attr1].explode + guessradius;
					limit *= limit; // square it to avoid expensive square roots
					loopvk(cl.et.ents)
					{
						gameentity &e = *(gameentity *)cl.et.ents[k];
						if(e.type == WAYPOINT && e.o.squaredist(p->o) <= limit)
							obstacles.add(p, k);
					}
				}
			}
			avoidmillis = lastmillis;
		}
	}

	void think(gameent *d)
	{
		if(d->ai->state.empty()) d->ai->reset();

		// the state stack works like a chain of commands, certain commands simply replace
		// each other, others spawn new commands to the stack the ai reads the top command
		// from the stack and executes it or pops the stack and goes back along the history
		// until it finds a suitable command to execute.

		if(!cl.intermission)
		{
			aistate &b = d->ai->getstate();
			process(d, b);
			bool override = d->state == CS_ALIVE && (b.goal || d->ai->route.empty());
			if(override || lastmillis >= b.next)
			{
				bool result = true;
				int frame = aiframetimes[b.type] - d->skill;
				b.next = lastmillis + frame;
				b.cycle++;
				switch(b.type)
				{
					case AI_S_WAIT:		result = dowait(d, b);		break;
					case AI_S_DEFEND:		result = dodefend(d, b);	break;
					case AI_S_PURSUE:		result = dopursue(d, b);	break;
					case AI_S_ATTACK:		result = doattack(d, b);	break;
					case AI_S_INTEREST:	result = dointerest(d, b);	break;
					default:			result = false;				break;
				}
				if((b.expire && (b.expire -= frame) <= 0) || !result)
					d->ai->removestate();
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
			"none", "node", "player", "entity", "flag"
		};
		s_sprintfd(s)("@%s%s [%d:%d:%d] goal:%d[%d] %s:%d",
			top ? "\fy" : "\fw",
			bnames[b.type],
			b.cycle, b.expire, b.next-lastmillis,
			!d->ai->route.empty() ? d->ai->route[0] : -1,
			d->ai->route.length(),
			btypes[b.targtype+1], b.target
		);
		particle_text(vec(d->abovehead()).add(vec(0, 0, above)), s, 14, 1);
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
				if(cl.et.ents.inrange(index) && cl.et.ents.inrange(prev))
				{
					gameentity &e = *(gameentity *)cl.et.ents[index],
						&f = *(gameentity *)cl.et.ents[prev];
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
		if(aidebug() > 3)
		{
			vec fr(cl.ws.gunorigin(d->o, d->ai->target, d, true)),
				dr(d->ai->target), pos = cl.headpos(d);
			if(dr.dist(pos) > AILOSDIST(d->skill))
			{
				dr.sub(fr);
				dr.normalize();
				dr.mul(AILOSDIST(d->skill));
			}
			renderline(fr, dr, cr, cg, cb, false);
			rendertris(dr, d->yaw, d->pitch, 2.f, cr, cg, cb, true, false);
		}
		renderprimitive(false);
	}

	void render()
	{
		if(aidebug())
		{
			int amt[2] = { 0, 0 };
			loopv(cl.players) if(cl.players[i] && cl.players[i]->ai) amt[0]++;
			loopv(cl.players) if(cl.players[i] && cl.players[i]->state == CS_ALIVE && cl.players[i]->ai)
			{
				gameent *d = cl.players[i];
				bool top = true;
				int above = 0;
				amt[1]++;
				loopvrev(d->ai->state)
				{
					aistate &b = d->ai->state[i];
					drawstate(d, b, top, above += 2);
					if(aidebug() > 2 && top && rendernormally && b.type != AI_S_WAIT)
						drawroute(d, b, float(amt[1])/float(amt[0]));
					if(top)
					{
						if(aidebug() > 1) top = false;
						else break;
					}
				}
			}
		}
	}
} ai;
#endif
