#include "game.h"
namespace ctf
{
	ctfstate st;

	void dropflag(gameent *d)
	{
		if(m_ctf(game::gamemode))
		{
			loopv(st.flags) if(st.flags[i].owner == d)
			{
				vec dir;
				vecfromyawpitch(d->aimyaw, d->aimpitch, -d->move, -d->strafe, dir);
				dir.mul((d->radius*2.f)+enttype[FLAG].radius);
				vec o(vec(d->o).add(dir));
				client::addmsg(SV_DROPFLAG, "ri4", game::player1->clientnum, int(o.x*DMF), int(o.y*DMF), int(o.z*DMF));
				return;
			}
		}
		if(d == game::player1) playsound(S_DENIED, d->o, d);
	}
   	ICOMMAND(dropflag, "", (), dropflag(game::player1));

    void preload()
    {
        loopi(numteams(game::gamemode, game::mutators)+TEAM_FIRST) loadmodel(teamtype[i].flag, -1, true);
    }

    void drawblip(int w, int h, int s, float blend, int i, bool blip)
    {
		ctfstate::flag &f = st.flags[i];
		vec dir;
		int colour = teamtype[f.team].colour;
		float r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f, fade = blend;
        if(blip)
        {
            if(!(f.base&BASE_FLAG) || f.owner == game::player1 || (!f.owner && !f.droptime) || lastmillis%600 >= 400)
				return;
        	dir = f.pos();
        }
        else
        {
        	if(!(f.base&BASE_HOME)) return;
        	dir = f.spawnloc;
        }
		dir.sub(camera1->o);
		if(!blip)
		{
			if(!(f.base&BASE_FLAG) || f.owner || f.droptime)
			{
				float dist = dir.magnitude(),
					diff = dist <= hud::radarrange() ? clamp(1.f-(dist/hud::radarrange()), 0.f, 1.f) : 0.f;
				fade *= 0.25f+(diff*0.5f);
			}
		}
		dir.rotate_around_z(-camera1->yaw*RAD);
		dir.normalize();
		if(hud::radarflagnames) hud::drawblip(w, h, s, fade, -3, dir, r, g, b, "radar", "%s%s", teamtype[f.team].chat, blip ? "flag" : "base");
		else hud::drawblip(w, h, s, fade, -3, dir, r, g, b);
    }

	void drawlast(int w, int h, int &tx, int &ty, float blend)
	{
		if(game::player1->state == CS_ALIVE)
		{
			int hasflag = -1;
			static vector<int> takenflags, droppedflags;
			takenflags.setsizenodelete(0); droppedflags.setsizenodelete(0);
			loopv(st.flags)
			{
				ctfstate::flag &f = st.flags[i];
				if(f.owner == game::player1) hasflag = f.team;
				if(isctfflag(f, game::player1->team))
				{
					if(f.owner && f.owner->team != game::player1->team) takenflags.add(i);
					else if(f.droptime) droppedflags.add(i);
				}
			}
			if(hasflag >= 0 || !takenflags.empty())
			{
				pushfont("emphasis");
				if(hasflag >= 0) ty += draw_textx("You have the \fs%s%s\fS flag!", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1, teamtype[hasflag].chat, teamtype[hasflag].name);
				if(!takenflags.empty()) ty += draw_textx("Flag has been taken!", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1);
				if(!droppedflags.empty()) ty += draw_textx("Flag has been dropped!", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1);
				popfont();
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
		int sy = 0;
		loopv(st.flags) if(st.flags[i].base&BASE_FLAG)
		{
			ctfstate::flag &f = st.flags[i];
			float skew = 0.75f, fade = blend;
			int millis = lastmillis-f.interptime;
			if(f.owner || f.droptime) skew += (millis < 1000 ? clamp(float(millis)/1000.f, 0.f, 1.f)*0.25f : 0.25f);
			else if(millis < 1000) skew += 0.25f-(clamp(float(millis)/1000.f, 0.f, 1.f)*0.25f);
			int oldy = y-sy;
			sy += hud::drawitem(hud::flagtex(f.team), x, y-sy, s, false, 1.f, 1.f, 1.f, fade, skew, "sub", f.owner ? "\frtaken" : (f.droptime ? "\fydrop" : "\fgsafe"));
			if(f.owner) hud::drawitemsubtext(x, oldy, s, false, skew, "sub", fade, "\fs%s%s\fS", teamtype[f.owner->team].chat, teamtype[f.owner->team].name);
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
			float trans = 1.f;
			int millis = lastmillis-f.interptime;
			if(millis < 1000) trans = float(millis)/1000.f;
            rendermodel(!f.droptime ? &f.ent->light : NULL, flagname, ANIM_MAPMODEL|ANIM_LOOP, above, !f.droptime ? f.ent->attr[2] : f.interptime%360, !f.droptime ? f.ent->attr[3] : 0, 0, MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_OCCLUDED|(f.droptime ? MDL_LIGHT : 0), NULL, NULL, 0, 0, trans);
            above.z += enttype[FLAG].radius/2;
            defformatstring(info)("@%s %s", teamtype[f.team].name, f.base&BASE_HOME && !f.droptime && !f.owner ? "base" : "flag");
			part_text(above, info, PART_TEXT, 1, teamtype[f.team].colour);
        }
    }

    void adddynlights()
    {
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent) continue;
			float trans = 1.f;
			if(!f.owner)
			{
				int millis = lastmillis-f.interptime;
				if(millis < 1000) trans = float(millis)/1000.f;
			}
			adddynlight(vec(f.pos()).add(vec(0, 0, enttype[FLAG].radius)), enttype[FLAG].radius*2,
				vec((teamtype[f.team].colour>>16), ((teamtype[f.team].colour>>8)&0xFF), (teamtype[f.team].colour&0xFF)).div(255.f).mul(trans));
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
			if(a->type != FLAG || !isteam(game::gamemode, game::mutators, a->attr[1], TEAM_NEUTRAL)) continue; \
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
            if(!added && isteam(game::gamemode, game::mutators, e->attr[1], TEAM_FIRST)) // not linked and is a team flag
				setupaddflag(e, BASE_BOTH); // add as both
        }
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
                f.owner = owner >= 0 ? game::newclient(owner) : NULL;
                if(f.owner) { if(!f.taketime) f.taketime = lastmillis; }
                else if(!dropped) f.taketime = 0;
                f.droptime = dropped;
                f.droploc = dropped ? droploc : f.spawnloc;
                if(dropped) physics::droptofloor(f.droploc, 2, 0);
            }
        }
    }

    void dropflag(gameent *d, int i, const vec &droploc)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		bool denied = false;
		loopvk(st.flags) if(isctfhome(st.flags[k], d->team))
		{
            ctfstate::flag &g = st.flags[k];
            if(!g.ent || g.owner || !(g.base&BASE_FLAG) || g.droptime || g.team == d->team) continue;
			if(d->o.dist(g.pos()) <= enttype[FLAG].radius*4)
			{
				denied = true;
				break;
			}
		}
		st.dropflag(i, droploc, 1);
		if(!physics::droptofloor(f.droploc, 2, 0)) f.droploc = vec(-1, -1, -1);
		game::announce(denied ? S_V_DENIED : S_V_FLAGDROP, "\fo%s%s dropped the the \fs%s%s\fS flag", d==game::player1 ? "you" : game::colorname(d), denied ? " was denied a capture and" : "", teamtype[f.team].chat, teamtype[f.team].name);
    }

    void removeplayer(gameent *d)
    {
        loopv(st.flags) if(st.flags[i].owner == d)
        {
            ctfstate::flag &f = st.flags[i];
            st.dropflag(i, f.owner->o, 1);
        }
    }

    void flageffect(int i, int team, const vec &from, const vec &to)
    {
		if(from.x >= 0) game::spawneffect(vec(from).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[team].colour, enttype[FLAG].radius);
		if(to.x >= 0) game::spawneffect(vec(to).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[team].colour, enttype[FLAG].radius);
		if(from.x >= 0 && to.x >= 0) part_trail(PART_ELECTRIC, 250, from, to, teamtype[team].colour, 2.f);
    }

    void returnflag(gameent *d, int i)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		flageffect(i, d->team, f.droploc, f.spawnloc);
		f.interptime = lastmillis;
		st.returnflag(i);
		game::announce(S_V_FLAGRETURN, "\fo%s returned the \fs%s%s\fS flag (time taken: \fs\fc%.2f\fS secs)", d==game::player1 ? "you" : game::colorname(d), teamtype[f.team].chat, teamtype[f.team].name, float(lastmillis-f.taketime)/1000.f);
		f.taketime = 0;
    }

    void resetflag(int i)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		flageffect(i, TEAM_NEUTRAL, f.droploc, f.spawnloc);
		f.interptime = lastmillis;
		f.taketime = 0;
		st.returnflag(i);
		game::announce(S_V_FLAGRESET, "\fothe \fs%s%s\fS flag has been reset", teamtype[f.team].chat, teamtype[f.team].name);
    }

    void scoreflag(gameent *d, int relay, int goal, int score)
    {
        if(!st.flags.inrange(goal) || !st.flags.inrange(relay)) return;
		ctfstate::flag &f = st.flags[relay], &g = st.flags[goal];
		flageffect(goal, d->team, g.spawnloc, f.spawnloc);
		(st.findscore(d->team)).total = score;
		f.interptime = lastmillis;
		st.returnflag(relay);
		if(d!=game::player1)
		{
			defformatstring(ds)("@CAPTURED!");
			part_text(d->abovehead(), ds, PART_TEXT_RISE, 2500, teamtype[d->team].colour, 3.f);
		}
		game::announce(S_V_FLAGSCORE, "\fo%s scored the \fs%s%s\fS flag for \fs%s%s\fS team (score: \fs\fc%d\fS, time taken: \fs\fc%.2f\fS secs)", d==game::player1 ? "you" : game::colorname(d), teamtype[f.team].chat, teamtype[f.team].name, teamtype[d->team].chat, teamtype[d->team].name, score, float(lastmillis-f.taketime)/1000.f);
		g.taketime = 0;
    }

    void takeflag(gameent *d, int i)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		game::spawneffect(vec(f.pos()).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[d->team].colour, enttype[FLAG].radius);
		f.interptime = lastmillis;
		if(!f.droptime) f.taketime = lastmillis;
		st.takeflag(i, d);
		game::announce(S_V_FLAGPICKUP, "\fo%s %s the \fs%s%s\fS flag", d==game::player1 ? "you" : game::colorname(d), f.droptime ? "picked up" : "stole", teamtype[f.team].chat, teamtype[f.team].name);
    }

    void checkflags(gameent *d)
    {
        vec o = d->feetpos();
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent || f.owner || !(f.base&BASE_FLAG) || (!f.droptime && f.team == d->team)) continue;
            if(o.dist(f.pos()) <= enttype[FLAG].radius/2)
            {
                if(f.pickup) continue;
                client::addmsg(SV_TAKEFLAG, "ri2", d->clientnum, i);
                f.pickup = true;
            }
            else f.pickup = false;
       }
    }

	bool aihomerun(gameent *d, ai::aistate &b)
	{
		vec pos = d->feetpos();
		loopk(2)
		{
			int goal = -1;
			loopv(st.flags)
			{
				ctfstate::flag &g = st.flags[i];
				if(isctfhome(g, d->team) && (k || (!g.owner && !g.droptime)) &&
					(!st.flags.inrange(goal) || g.spawnloc.squaredist(pos) < st.flags[goal].spawnloc.squaredist(pos)))
				{
					goal = i;
				}
			}
			if(st.flags.inrange(goal) && ai::makeroute(d, b, st.flags[goal].spawnloc, false))
			{
				d->ai->addstate(ai::AI_S_PURSUE, ai::AI_T_AFFINITY, goal);
				return true;
			}
		}
		return false;
	}

	bool aicheck(gameent *d, ai::aistate &b)
	{
		static vector<int> hasflags, takenflags;
		hasflags.setsizenodelete(0);
		takenflags.setsizenodelete(0);
		loopv(st.flags)
		{
			ctfstate::flag &g = st.flags[i];
			if(g.owner == d) hasflags.add(i);
			else if(isctfflag(g, d->team) && (g.owner || g.droptime))
				takenflags.add(i);
		}
		if(!hasflags.empty())
		{
			aihomerun(d, b);
			return true;
		}
		if(!ai::badhealth(d) && !takenflags.empty())
		{
			int flag = takenflags.length() > 2 ? rnd(takenflags.length()) : 0;
			d->ai->addstate(ai::AI_S_PURSUE, ai::AI_T_AFFINITY, takenflags[flag]);
			return true;
		}
		return false;
	}

	void aifind(gameent *d, ai::aistate &b, vector<ai::interest> &interests)
	{
		vec pos = d->feetpos();
		loopvj(st.flags)
		{
			ctfstate::flag &f = st.flags[j];
			static vector<int> targets; // build a list of others who are interested in this
			targets.setsizenodelete(0);
			bool home = isctfhome(f, d->team), regen = !m_regen(game::gamemode, game::mutators) || !overctfhealth || d->health >= overctfhealth;
			ai::checkothers(targets, d, home ? ai::AI_S_DEFEND : ai::AI_S_PURSUE, ai::AI_T_AFFINITY, j, true);
			gameent *e = NULL;
			loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && ai::targetable(d, e, false) && !e->ai && d->team == e->team)
			{ // try to guess what non ai are doing
				vec ep = e->feetpos();
				if(targets.find(e->clientnum) < 0 && (ep.squaredist(f.pos()) <= (enttype[FLAG].radius*enttype[FLAG].radius*4) || f.owner == e))
					targets.add(e->clientnum);
			}
			if(home)
			{
				bool guard = false;
				if(f.owner || f.droptime || targets.empty()) guard = true;
				else if(d->hasweap(d->ai->weappref, m_spawnweapon(game::gamemode, game::mutators)))
				{ // see if we can relieve someone who only has a piece of crap
					gameent *t;
					loopvk(targets) if((t = game::getclient(targets[k])))
					{
						if((t->ai && !t->hasweap(t->ai->weappref, m_spawnweapon(game::gamemode, game::mutators))) || (!t->ai && t->weapselect == WEAPON_PISTOL))
						{
							guard = true;
							break;
						}
					}
				}
				if(guard)
				{ // defend the flag
					ai::interest &n = interests.add();
					n.state = ai::AI_S_DEFEND;
					n.node = entities::closestent(WAYPOINT, f.pos(), ai::NEARDIST, true);
					n.target = j;
					n.targtype = ai::AI_T_AFFINITY;
					n.score = pos.squaredist(f.pos())/(!regen ? 100.f : 1.f);
				}
			}
			else
			{
				if(targets.empty())
				{ // attack the flag
					ai::interest &n = interests.add();
					n.state = ai::AI_S_PURSUE;
					n.node = entities::closestent(WAYPOINT, f.pos(), ai::NEARDIST, true);
					n.target = j;
					n.targtype = ai::AI_T_AFFINITY;
					n.score = pos.squaredist(f.pos());
				}
				else
				{ // help by defending the attacker
					gameent *t;
					loopvk(targets) if((t = game::getclient(targets[k])))
					{
						ai::interest &n = interests.add();
						n.state = ai::AI_S_DEFEND;
						n.node = t->lastnode;
						n.target = t->clientnum;
						n.targtype = ai::AI_T_PLAYER;
						n.score = d->o.squaredist(t->o);
					}
				}
			}
		}
	}

	bool aidefend(gameent *d, ai::aistate &b)
	{
		if(st.flags.inrange(b.target))
		{
			ctfstate::flag &f = st.flags[b.target];
			static vector<int> hasflags;
			hasflags.setsizenodelete(0);
			loopv(st.flags)
			{
				ctfstate::flag &g = st.flags[i];
				if(g.owner == d) hasflags.add(i);
			}
			if(!hasflags.empty())
			{
				aihomerun(d, b);
				return true;
			}
			if(isctfflag(f, d->team))
			{
				if(f.owner && ai::violence(d, b, f.owner, true)) return true;
				if(f.droptime && ai::makeroute(d, b, f.pos())) return true;
			}
			int walk = 0, regen = !m_regen(game::gamemode, game::mutators) || !overctfhealth || d->health >= overctfhealth;
			if(regen && lastmillis-b.millis >= (201-d->skill)*33)
			{
				static vector<int> targets; // build a list of others who are interested in this
				targets.setsizenodelete(0);
				ai::checkothers(targets, d, ai::AI_S_DEFEND, ai::AI_T_AFFINITY, b.target, true);
				gameent *e = NULL;
				loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && ai::targetable(d, e, false) && !e->ai && d->team == e->team)
				{ // try to guess what non ai are doing
					vec ep = e->feetpos();
					if(targets.find(e->clientnum) < 0 && (ep.squaredist(f.pos()) <= (enttype[FLAG].radius*enttype[FLAG].radius*4) || f.owner == e))
						targets.add(e->clientnum);
				}
				if(!targets.empty())
				{
					d->ai->clear = true; // re-evaluate so as not to herd
					return true;
				}
				else
				{
					walk = 2;
					b.millis = lastmillis;
				}
			}
			vec pos = d->feetpos();
			float mindist = float(enttype[FLAG].radius*enttype[FLAG].radius*8);
			loopv(st.flags)
			{ // get out of the way of the returnee!
				ctfstate::flag &g = st.flags[i];
				if(pos.squaredist(g.pos()) <= mindist)
				{
					if(g.owner && g.owner->team == d->team) walk = 1;
					if(g.droptime && ai::makeroute(d, b, g.pos())) return true;
				}
			}
			return ai::defend(d, b, f.pos(), float(enttype[FLAG].radius*2), float(enttype[FLAG].radius*(2+(walk*2))), walk);
		}
		return false;
	}

	bool aipursue(gameent *d, ai::aistate &b)
	{
		if(st.flags.inrange(b.target))
		{
			ctfstate::flag &f = st.flags[b.target];
			if(f.owner && f.owner == d)
			{
				aihomerun(d, b);
				return true;
			}
			if(isctfhome(f, d->team))
			{
				static vector<int> hasflags;
				hasflags.setsizenodelete(0);
				loopv(st.flags)
				{
					ctfstate::flag &g = st.flags[i];
					if(g.owner == d) hasflags.add(i);
				}
				if(!hasflags.empty())
				{
					ai::makeroute(d, b, f.spawnloc);
					return true;
				}
				else if(!isctfflag(f, d->team)) return false;
			}
			if(isctfflag(f, d->team))
			{
				if(f.owner && ai::violence(d, b, f.owner, true)) return true;
				if(f.droptime && ai::makeroute(d, b, f.pos())) return true;
			}
			else return ai::makeroute(d, b, f.pos());
		}
		return false;
	}
}
