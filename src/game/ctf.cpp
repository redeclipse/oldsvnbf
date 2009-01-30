#ifdef GAMESERVER
struct ctfservmode : ctfstate, servmode
{
    static const int RESETFLAGTIME = 10000;

    bool notgotflags;

    ctfservmode() : notgotflags(false) {}

    void reset(bool empty)
    {
        ctfstate::reset();
        notgotflags = !empty;
    }

    void dropflag(clientinfo *ci, const vec &o)
    {
        if(notgotflags) return;
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
			ivec p(vec(ci->state.o.dist(o) > enttype[FLAG].radius ? ci->state.o : o).mul(DMF));
            sendf(-1, 1, "ri6", SV_DROPFLAG, ci->clientnum, i, p.x, p.y, p.z);
            ctfstate::dropflag(i, p.tovec().div(DMF), lastmillis);
        }
    }

    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        dropflag(ci, ci->state.o);
    }

    void died(clientinfo *ci, clientinfo *actor)
    {
        dropflag(ci, ci->state.o);
    }

    bool canchangeteam(clientinfo *ci, int oldteam, int newteam)
    {
    	loopi(numteams(gamemode, mutators)) if(newteam == i+TEAM_FIRST) return true;
    	return false;
    }

    void changeteam(clientinfo *ci, int oldteam, int newteam)
    {
        dropflag(ci, ci->state.o);
    }

	int addscore(int team)
	{
		score &cs = findscore(team);
		cs.total++;
		return cs.total;
	}

    void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
    {
        if(notgotflags) return;
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
            loopvk(flags)
            {
				flag &f = flags[k];
				if(isctfhome(f, ci->team) && f.owner<0 && !f.droptime && newpos.dist(f.spawnloc) < enttype[FLAG].radius/2)
				{
					returnflag(i);
					ci->state.flags++;
					int score = addscore(ci->team);
					sendf(-1, 1, "ri5", SV_SCOREFLAG, ci->clientnum, i, k, score);
					if(sv_ctflimit && score >= sv_ctflimit) startintermission();
				}
            }
        }
    }

    void takeflag(clientinfo *ci, int i)
    {
        if(notgotflags || !flags.inrange(i) || ci->state.state!=CS_ALIVE || !ci->team || !(flags[i].base&BASE_FLAG)) return;
		flag &f = flags[i];
		if(f.team == ci->team)
		{
			if(!f.droptime || f.owner >= 0) return;
			ctfstate::returnflag(i);
			sendf(-1, 1, "ri3", SV_RETURNFLAG, ci->clientnum, i);
		}
		else
		{
			if(f.owner >= 0) return;
			loopv(flags) if(flags[i].owner == ci->clientnum) return;
			ctfstate::takeflag(i, ci->clientnum);
			sendf(-1, 1, "ri3", SV_TAKEFLAG, ci->clientnum, i);
		}
    }

    void update()
    {
        if(minremain<0 || notgotflags) return;
        loopv(flags)
        {
            flag &f = flags[i];
            if(f.base&BASE_FLAG && f.owner < 0 && f.droptime && lastmillis-f.droptime >= RESETFLAGTIME)
            {
                returnflag(i);
                sendf(-1, 1, "ri2", SV_RESETFLAG, i);
            }
        }
    }

    void initclient(clientinfo *ci, ucharbuf &p, bool connecting)
    {
		if(connecting)
        {
            loopv(scores)
		    {
			    score &cs = scores[i];
			    putint(p, SV_TEAMSCORE);
			    putint(p, cs.team);
			    putint(p, cs.total);
		    }
        }
        putint(p, SV_INITFLAGS);
        putint(p, flags.length());
        loopv(flags)
        {
            flag &f = flags[i];
            putint(p, f.team);
            putint(p, f.base);
            putint(p, f.owner);
            if(f.owner<0)
            {
                putint(p, f.droptime ? 1 : 0);
                if(f.droptime)
                {
                    putint(p, int(f.droploc.x*DMF));
                    putint(p, int(f.droploc.y*DMF));
                    putint(p, int(f.droploc.z*DMF));
                }
            }
        }
    }

    void parseflags(ucharbuf &p)
    {
    	int numflags = getint(p);
        loopi(numflags)
        {
            int team = getint(p), base = getint(p);
            vec o;
            loopk(3) o[k] = getint(p)/DMF;
            if(notgotflags) addflag(o, team, base, i);
        }
        notgotflags = false;
    }
} ctfmode;
#else
#include "pch.h"
#include "engine.h"
#include "game.h"
namespace ctf
{
	ctfstate st;

	void dropflags()
	{
		vec dir;
		vecfromyawpitch(world::player1->aimyaw, world::player1->aimpitch, -world::player1->move, -world::player1->strafe, dir);
		dir.mul((world::player1->radius*2.f)+enttype[FLAG].radius/2+1.f);
		vec o(vec(world::player1->o).add(dir).mul(DMF));
		client::addmsg(SV_DROPFLAG, "ri4", world::player1->clientnum, o.x, o.y, o.z);
	}
   	ICOMMAND(dropflags, "", (), dropflags());

    void preload()
    {
        loopi(numteams(world::gamemode, world::mutators)+TEAM_FIRST) loadmodel(teamtype[i].flag, -1, true);
    }

    void drawblip(int w, int h, int s, float blend, int i, bool blip)
    {
		ctfstate::flag &f = st.flags[i];
		vec dir;
		int colour = teamtype[f.team].colour;
		float r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f,
			fade = hud::radarblipblend*blend;
        if(blip)
        {
            if(!(f.base&BASE_FLAG) || f.owner == world::player1) return;
			else if(f.owner) { if(lastmillis%500 >= 250) fade = 1.f; }
            else if(f.droptime) { if(lastmillis%500 >= 250) fade = 1.f; }
            else return;
        	dir = f.pos();
        }
        else
        {
        	if(!(f.base&BASE_HOME)) return;
        	dir = f.spawnloc;
        	r *= 0.5f; g *= 0.5f; b *= 0.5f;
        }
		dir.sub(camera1->o);
		if(blip)
		{
			float dist = dir.magnitude(),
				diff = dist <= hud::radarrange() ? clamp(1.f-(dist/hud::radarrange()), 0.f, 1.f) : 0.f;
			fade *= 0.5f+(diff*0.5f);
		}
		dir.rotate_around_z(-camera1->yaw*RAD);
		dir.normalize();
		if(hud::radarflagnames) hud::drawblip(w, h, s, fade, 3, dir, r, g, b,
				"hud", fade*hud::radarnameblend, "%s%s %s", teamtype[f.team].chat, teamtype[f.team].name, blip ? "flag" : "base");
		else hud::drawblip(w, h, s, fade, 3, dir, r, g, b);
    }

	void drawlast(int w, int h, int &tx, int &ty)
	{
		if(world::player1->state == CS_ALIVE)
		{
			loopv(st.flags) if(st.flags[i].owner == world::player1)
			{
				//ctfstate::flag &f = st.flags[i];
				ty += draw_textx("Return the flag!", tx, ty, 255, 255, 255, int(255*hudblend), TEXT_RIGHT_JUSTIFY, -1, -1);
				break;
			}
		}
	}

    void drawblips(int w, int h, int s, float blend)
    {
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent) continue;
            drawblip(w, h, s, blend, i, false);
            drawblip(w, h, s, blend, i, true);
        }
    }

    int drawinventory(int x, int y, int s, float blend)
    {
		int sy = 0, home = -1, team = world::player1->team;
		float bestdist = 1e16f;
		if(world::player1->state == CS_ALIVE)
		{
			loopv(st.flags) if(isctfhome(st.flags[i], world::player1->team))
			{
				ctfstate::flag &f = st.flags[i];
				float dist = world::player1->o.dist(f.spawnloc);
				if(!st.flags.inrange(home) || dist < bestdist)
				{
					home = i;
					bestdist = dist;
					team = f.team;
				}
			}
		}
		loopv(st.flags)
		{
			ctfstate::flag &f = st.flags[i];
			float skew = 0.f, fade = hud::inventoryblend*blend;
			int millis = lastmillis-f.interptime;
			bool hasflag = false;
			if(f.owner == world::player1)
			{
				skew = millis < 500 ? clamp(float(millis)/500.f, 0.f, 1.f) : 1.f;
				hasflag = true;
			}
			else if(f.wasowner == world::player1)
			{
				if(millis < 500)
				{
					skew = clamp(float(millis)/500.f, 0.f, 1.f);
					hasflag = true;
				}
				else f.wasowner = NULL;
			}
			if(hasflag)
			{
				float dist = world::player1->o.dist(f.spawnloc);
				int oldy = y-sy;
				sy += hud::drawitem(hud::flagtex(f.team), x, y-sy, s, fade, skew, "emphasis", blend, "\fs%s+\fS %.1f", teamtype[f.team].chat, dist/16.f);
				hud::drawitemsubtext(x, oldy, s, fade, skew, "default", blend, "\fs%s-\fS %.1f", teamtype[team].chat, bestdist/16.f);
			}
		}
		return sy;
    }


    void render()
    {
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent || f.owner) continue;
            const char *flagname = teamtype[f.team].flag;
            vec above(f.pos());
            rendermodel(NULL, flagname, ANIM_MAPMODEL|ANIM_LOOP, above, 0.f, 0.f, 0.f, MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_OCCLUDED);
            above.z += enttype[FLAG].radius/2;
            s_sprintfd(info)("@%s %s", teamtype[f.team].name, f.base&BASE_HOME || !f.droptime ? "base" : "flag");
			part_text(above, info, PART_TEXT, 1, teamtype[f.team].colour);
        }
    }

    void setupflags()
    {
        st.reset();
        #define setupaddflag(a,b) \
        { \
            index = st.addflag(a->o, a->attr[1], b); \
			if(st.flags.inrange(index)) st.flags[index].ent = a; \
			else continue; \
        }
		#define setupchkflag(a,b) \
		{ \
			if(a->type != FLAG || !isteam(world::gamemode, world::mutators, a->attr[1], TEAM_NEUTRAL)) continue; \
			else \
			{ \
				int already = -1; \
				loopvk(st.flags) if(st.flags[k].ent == a) \
				{ \
					already = k; \
					break; \
				} \
				if(st.flags.inrange(already)) b; \
			} \
		}
		#define setuphomeflag if(!added) { setupaddflag(e, BASE_HOME); added = true; }
		int index = -1;
        loopv(entities::ents)
        {
            extentity *e = entities::ents[i];
            setupchkflag(e, { continue; });
			bool added = false; // check for a linked flag to see if we should use a seperate flag/home assignment
			loopvj(e->links) if(entities::ents.inrange(e->links[j]))
			{
				extentity *g = entities::ents[e->links[j]];
				setupchkflag(g,
				{
					ctfstate::flag &f = st.flags[already];
					if((f.base&BASE_FLAG) && (f.base&BASE_HOME)) // got found earlier, but it is linked!
						f.base &= ~BASE_HOME;
					setuphomeflag;
					continue;
				});
				setupaddflag(g, BASE_FLAG); // add link as flag
				setuphomeflag;
			}
            if(!added && isteam(world::gamemode, world::mutators, e->attr[1], TEAM_FIRST)) // not linked and is a team flag
				setupaddflag(e, BASE_BOTH); // add as both
        }
        vec center(0, 0, 0);
        loopv(st.flags) center.add(st.flags[i].spawnloc);
        center.div(st.flags.length());
    }

    void sendflags(ucharbuf &p)
    {
        putint(p, SV_INITFLAGS);
		putint(p, st.flags.length());
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            putint(p, f.team);
            putint(p, f.base);
            loopk(3) putint(p, int(f.spawnloc[k]*DMF));
        }
    }

	void setscore(int team, int total)
	{
		st.findscore(team).total = total;
	}

    void parseflags(ucharbuf &p, bool commit)
    {
    	int numflags = getint(p);
        loopi(numflags)
        {
            int team = getint(p), base = getint(p), owner = getint(p), dropped = 0;
            vec droploc(0, 0, 0);
            if(owner < 0)
            {
                dropped = getint(p);
                if(dropped) loopk(3) droploc[k] = getint(p)/DMF;
            }
            if(commit && st.flags.inrange(i))
            {
				ctfstate::flag &f = st.flags[i];
				f.team = team;
                f.base = base;
                f.owner = owner >= 0 ? world::newclient(owner) : NULL;
                f.droptime = dropped;
                f.droploc = dropped ? droploc : f.spawnloc;
                f.interptime = 0;

                if(dropped)
                {
                    if(!physics::droptofloor(f.droploc, 2, 0)) f.droploc = vec(-1, -1, -1);
                }
            }
        }
    }

    void dropflag(gameent *d, int i, const vec &droploc)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		f.interptime = lastmillis;
		st.dropflag(i, droploc, 1);
		if(!physics::droptofloor(f.droploc, 2, 0))
		{
			f.droploc = vec(-1, -1, -1);
			f.interptime = 0;
		}
		world::announce(S_V_FLAGDROP, "\fo%s dropped the the \fs%s%s\fS flag", d==world::player1 ? "you" : world::colorname(d), teamtype[f.team].chat, teamtype[f.team].name);
    }

    void flageffect(int i, int team, const vec &from, const vec &to)
    {
		if(from.x >= 0) world::spawneffect(vec(from).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[team].colour, enttype[FLAG].radius/2);
		if(to.x >= 0) world::spawneffect(vec(to).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[team].colour, enttype[FLAG].radius/2);
		if(from.x >= 0 && to.x >= 0) part_trail(PART_ELECTRIC, 250, from, to, teamtype[team].colour, 2.f);
    }

    void returnflag(gameent *d, int i)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		flageffect(i, d->team, f.droploc, f.spawnloc);
		f.interptime = 0;
		st.returnflag(i);
		world::announce(S_V_FLAGRETURN, "\fo%s returned the \fs%s%s\fS flag", d==world::player1 ? "you" : world::colorname(d), teamtype[f.team].chat, teamtype[f.team].name);
    }

    void resetflag(int i)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		flageffect(i, TEAM_NEUTRAL, f.droploc, f.spawnloc);
		f.interptime = 0;
		st.returnflag(i);
		world::announce(S_V_FLAGRESET, "\fothe \fs%s%s\fS flag has been reset", teamtype[f.team].chat, teamtype[f.team].name);
    }

    void scoreflag(gameent *d, int relay, int goal, int score)
    {
        if(!st.flags.inrange(goal) || !st.flags.inrange(relay)) return;
		ctfstate::flag &f = st.flags[goal], &g = st.flags[relay];
		flageffect(goal, d->team, st.flags[goal].spawnloc, st.flags[relay].spawnloc);
		(st.findscore(d->team)).total = score;
		f.interptime = 0;
		st.returnflag(relay);
		if(d!=world::player1)
		{
			s_sprintfd(ds)("@CAPTURED!");
			part_text(d->abovehead(), ds, PART_TEXT_RISE, 5000, teamtype[d->team].colour, 3.f);
		}
		world::announce(S_V_FLAGSCORE, "\fo%s scored the \fs%s%s\fS flag for \fs%s%s\fS team (score: %d)", d==world::player1 ? "you" : world::colorname(d), teamtype[g.team].chat, teamtype[g.team].name, teamtype[d->team].chat, teamtype[d->team].name, score);
    }

    void takeflag(gameent *d, int i)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		world::spawneffect(vec(f.pos()).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[d->team].colour, enttype[FLAG].radius/2);
		f.interptime = lastmillis;
		st.takeflag(i, d);
		world::announce(S_V_FLAGPICKUP, "\fo%s %s the \fs%s%s\fS flag", d==world::player1 ? "you" : world::colorname(d), f.droptime ? "picked up" : "stole", teamtype[f.team].chat, teamtype[f.team].name);
    }

    void checkflags(gameent *d)
    {
        vec o = world::feetpos(d);
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent || f.owner || !(f.base&BASE_FLAG) || (f.droptime ? f.droploc.x<0 : f.team == d->team)) continue;
            if(o.dist(f.pos()) < enttype[FLAG].radius/2)
            {
                if(f.pickup) continue;
                client::addmsg(SV_TAKEFLAG, "ri2", d->clientnum, i);
                f.pickup = true;
            }
            else f.pickup = false;
       }
    }

	bool aihomerun(gameent *d, aistate &b)
	{
		vec pos = world::headpos(d);
		loopk(2)
		{
			int goal = -1;
			loopv(st.flags)
			{
				ctfstate::flag &g = st.flags[i];
				if(isctfhome(g, d->team) && (k || (!g.owner && !g.droptime)) &&
					(!st.flags.inrange(goal) || g.pos().squaredist(pos) < st.flags[goal].pos().squaredist(pos)))
				{
					goal = i;
				}
			}

			if(st.flags.inrange(goal) && ai::makeroute(d, b, st.flags[goal].pos(), enttype[FLAG].radius/2))
			{
				aistate &c = d->ai->setstate(AI_S_PURSUE); // replaces current state!
				c.targtype = AI_T_AFFINITY;
				c.target = goal;
				c.defers = false;
				c.expire = 0; // doesn't until we get it home!
				return true;
			}
		}
		return false;
	}

	bool aicheck(gameent *d, aistate &b)
	{
		vector<int> hasflags, targets;
		loopv(st.flags)
		{
			ctfstate::flag &g = st.flags[i];
			if(g.owner)
			{
				if(g.owner == d) hasflags.add(i);
				else if(g.owner->team == d->team) targets.add(i);
			}
		}
		if(!hasflags.empty() && aihomerun(d, b)) return true;
		return false;
	}

	void aifind(gameent *d, aistate &b, vector<interest> &interests)
	{
		vec pos = world::headpos(d);
		loopvj(st.flags)
		{
			ctfstate::flag &f = st.flags[j];
			vector<int> targets; // build a list of others who are interested in this
			ai::checkothers(targets, d, isctfhome(f, d->team) ? AI_S_DEFEND : AI_S_PURSUE, AI_T_AFFINITY, j, true);
			gameent *e = NULL;
			loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && AITARG(d, e, false) && !e->ai && d->team == e->team)
			{ // try to guess what non ai are doing
				vec ep = world::headpos(e);
				if(targets.find(e->clientnum) < 0 && (ep.squaredist(f.pos()) <= (enttype[FLAG].radius*enttype[FLAG].radius) || f.owner == e))
					targets.add(e->clientnum);
			}
			if(isctfhome(f, d->team))
			{
				bool guard = false;
				if(f.owner || targets.empty()) guard = true;
				else if(d->weapselect == d->ai->weappref)
				{ // see if we can relieve someone who only has a plasma
					gameent *t;
					loopvk(targets) if((t = world::getclient(targets[k])) && t->ai && t->weapselect != t->ai->weappref)
					{
						guard = true;
						break;
					}
				}
				if(guard)
				{ // defend the flag
					interest &n = interests.add();
					n.state = AI_S_DEFEND;
					n.node = entities::entitynode(f.pos());
					n.target = j;
					n.targtype = AI_T_AFFINITY;
					n.tolerance = enttype[FLAG].radius;
					n.score = pos.squaredist(f.pos())/(d->weapselect != d->ai->weappref ? 10.f : 100.f);
					n.defers = false;
					n.expire = 10000;
				}
			}
			else
			{
				if(targets.empty())
				{ // attack the flag
					interest &n = interests.add();
					n.state = AI_S_PURSUE;
					n.node = entities::entitynode(f.pos());
					n.target = j;
					n.targtype = AI_T_AFFINITY;
					n.tolerance = enttype[FLAG].radius;
					n.score = pos.squaredist(f.pos());
					n.defers = false;
					n.expire = 10000;
				}
				else
				{ // help by defending the attacker
					gameent *t;
					loopvk(targets) if((t = world::getclient(targets[k])))
					{
						interest &n = interests.add();
						vec tp = world::headpos(t);
						n.state = AI_S_DEFEND;
						n.node = t->lastnode;
						n.target = t->clientnum;
						n.targtype = AI_T_PLAYER;
						n.tolerance = t->radius*2.f;
						n.score = pos.squaredist(tp);
						n.defers = false;
						n.expire = 10000;
					}
				}
			}
		}
	}

	bool aidefend(gameent *d, aistate &b)
	{
		if(st.flags.inrange(b.target))
		{
			ctfstate::flag &f = st.flags[b.target];
			if(f.owner && ai::violence(d, b, f.owner, true)) return true;
			bool walk = false;
			loopv(st.flags)
			{
				ctfstate::flag &g = st.flags[i];
				if(g.owner && g.owner->team == d->team)
				{
					vec gp = world::feetpos(g.owner);
					if(gp.squaredist(f.pos()) <= ai::AIISNEAR*ai::AIISNEAR)
					{
						walk = true;
						break;
					}
				}
			}
			return ai::defend(d, b, f.pos(), float(enttype[FLAG].radius), ai::AIISNEAR, walk);
		}
		return false;
	}

	bool aipursue(gameent *d, aistate &b)
	{
		if(st.flags.inrange(b.target))
		{
			ctfstate::flag &f = st.flags[b.target];
			if(isctfhome(f, d->team))
			{
				static vector<int> hasflags;
				hasflags.setsizenodelete(0);
				loopv(st.flags)
				{
					ctfstate::flag &g = st.flags[i];
					if(g.owner == d) hasflags.add(i);
				}
				if(hasflags.empty()) return false; // otherwise why are we pursuing home?
				return ai::makeroute(d, b, f.pos(), enttype[FLAG].radius/2);
			}
			else
			{
				if(f.owner == d) return aihomerun(d, b);
				return ai::makeroute(d, b, f.pos(), enttype[FLAG].radius/2);
			}
		}
		return false;
	}
}
#endif
