#include "game.h"
namespace ctf
{
	ctfstate st;

	void dropflag(gameent *d)
	{
		if(m_ctf(game::gamemode))
		{
			vec dir;
			vecfromyawpitch(d->yaw, d->pitch, -d->move, -d->strafe, dir);
			dir.mul((d->radius*2.f)+enttype[FLAG].radius);
			vec o(vec(d->o).add(dir));
			client::addmsg(SV_DROPFLAG, "ri4", game::player1->clientnum, int(o.x*DMF), int(o.y*DMF), int(o.z*DMF));
		}
		else if(d == game::player1) playsound(S_DENIED, d->o, d);
	}
   	ICOMMAND(dropflag, "", (), dropflag(game::player1));

   	ICOMMAND(ctfdebug, "", (), {
		loopv(st.flags)
		{
			ctfstate::flag &f = st.flags[i];
			conoutf("flag %2d: %d [%d] %.1f,%.1f,%.1f", i, f.team, f.base, f.spawnloc.x, f.spawnloc.y, f.spawnloc.z);
			if(f.droptime) conoutf("    dropped [%d] %.1f,%.1f,%1.f", f.droptime, f.droploc.x, f.droploc.y, f.droploc.z);
			if(f.owner) conoutf("    taken [%s] %.1f,%.1f,%1.f", game::colorname(f.owner), f.owner->o.x, f.owner->o.y, f.owner->o.z);
		}
	});

    void preload()
    {
        loopi(numteams(game::gamemode, game::mutators)+TEAM_FIRST) loadmodel(teamtype[i].flag, -1, true);
    }

    void drawblips(int w, int h, float blend)
    {
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent) continue;
            loopk(2)
            {
				vec dir;
				int colour = teamtype[f.team].colour;
				float r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f, fade = blend*hud::radarflagblend;
				if(k)
				{
					if(!(f.base&BASE_FLAG) || f.owner == game::player1 || (!f.owner && !f.droptime) || lastmillis%600 >= 400) break;
					dir = f.pos();
				}
				else dir = f.spawnloc;
				dir.sub(camera1->o);
				if(!k && (!(f.base&BASE_FLAG) || f.owner || f.droptime))
				{
					float dist = dir.magnitude(),
						diff = dist <= hud::radarrange() ? clamp(1.f-(dist/hud::radarrange()), 0.f, 1.f) : 0.f;
					fade *= diff*0.5f;
				}
				dir.rotate_around_z(-camera1->yaw*RAD); dir.normalize();
				if(hud::radarflagnames) hud::drawblip(hud::flagtex, 3, w, h, hud::radarflagsize, fade, dir, r, g, b, "radar", "%s%s", teamtype[f.team].chat, k ? "flag" : "base");
				else hud::drawblip(hud::flagtex, 3, w, h, hud::radarflagsize, fade, dir, r, g, b);
            }
        }
    }

	void drawlast(int w, int h, int &tx, int &ty, float blend)
	{
		if(game::player1->state == CS_ALIVE)
		{
			static vector<int> hasflags, takenflags, droppedflags;
			hasflags.setsizenodelete(0); takenflags.setsizenodelete(0); droppedflags.setsizenodelete(0);
			loopv(st.flags)
			{
				ctfstate::flag &f = st.flags[i];
				if(f.owner == game::player1) hasflags.add(i);
				else if(isctfflag(f, game::player1->team))
				{
					if(f.owner && f.owner->team != game::player1->team) takenflags.add(i);
					else if(f.droptime) droppedflags.add(i);
				}
			}
			pushfont("super");
			if(!hasflags.empty()) ty += draw_textx("%sYou have \fs\fc%d\fS %s, return to base!", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1, lastmillis%500 >= 250 ? "\fo" : "\fy", hasflags.length(), hasflags.length() > 1 ? "flags" : "flag");
			if(!takenflags.empty()) ty += draw_textx("Flag has been taken, go get it!", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1);
			if(!droppedflags.empty()) ty += draw_textx("Flag has been dropped, go get it!", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1);
			popfont();
		}
	}

    int drawinventory(int x, int y, int s, float blend)
    {
		int sy = 0;
		loopv(st.flags) if(st.flags[i].base&BASE_FLAG && (game::player1->state == CS_SPECTATOR || hud::inventorygame >= 2 || st.flags[i].lastowner == game::player1))
		{
			ctfstate::flag &f = st.flags[i];
			int millis = lastmillis-f.interptime, oldy = y-sy, colour = teamtype[f.team].colour;
			float skew = game::player1->state == CS_SPECTATOR || hud::inventorygame >= 2 ? hud::inventoryskew : 0.f, fade = blend*hud::inventoryblend,
				r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f;
			if(f.owner || f.droptime) skew += (millis < 1000 ? clamp(float(millis)/1000.f, 0.f, 1.f)*(1.f-skew) : 1.f-skew);
			else if(millis < 1000) skew += (1.f-skew)-(clamp(float(millis)/1000.f, 0.f, 1.f)*(1.f-skew));
			sy += hud::drawitem(hud::flagtex, x, y-sy, s, false, r, g, b, fade, skew, "sub", f.owner ? (f.team == f.owner->team ? "\fysecured" : "\frtaken") : (f.droptime ? "\fodropped" : "\fgsafe"));
			if(f.owner) hud::drawitemsubtext(x, oldy, s, false, skew, "sub", fade, "\fs%s\fS", game::colorname(f.owner));
		}
		return sy;
    }

    void render()
    {
        loopv(st.flags) // flags/bases
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent) continue;
            const char *flagname = teamtype[f.team].flag;
            vec above(f.spawnloc);
            float trans = 0.25f;
            if((f.base&BASE_FLAG) && !f.owner && !f.droptime)
            {
				int millis = lastmillis-f.interptime;
				if(millis < 1000)
				{
					if(millis > 250) trans += float(millis-250)/1000.f;
				}
				else trans = 1.f;
            }
            rendermodel(&f.ent->light, flagname, ANIM_MAPMODEL|ANIM_LOOP, above, f.ent->attr[1], f.ent->attr[2], 0, MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_OCCLUDED, NULL, NULL, 0, 0, trans);
			above.z += enttype[FLAG].radius*2/3;
            if((f.base&BASE_HOME) || (!f.owner && !f.droptime))
            {
				defformatstring(info)("@%s %s", teamtype[f.team].name, f.base&BASE_HOME ? "base" : "flag");
				part_text(above, info, PART_TEXT, 1, teamtype[f.team].colour);
				above.z += 2.5f;
            }
			if((f.base&BASE_FLAG) && f.droptime)
			{
				float wait = clamp((lastmillis-f.droptime)/float(st.RESETFLAGTIME), 0.f, 1.f);
				part_icon(above, textureload("textures/progress", 3), 0.25f, 4, 0, 0, 1, teamtype[f.team].colour);
				part_icon(above, textureload("textures/progress", 3), 1, 4, 0, 0, 1, teamtype[f.team].colour, 0, wait);
				defformatstring(str)("@%d%%", int(wait*100.f)); part_text(above, str);
				above.z += 2.5f;
			}
            if(f.base&BASE_FLAG && (f.owner || f.droptime))
            {
				if(f.owner)
				{
					defformatstring(info)("@%s", game::colorname(f.owner));
					part_text(above, info, PART_TEXT, 1);
					above.z += 1.5f;
				}
				defformatstring(info)("@%s", f.owner ? (f.team == f.owner->team ? "\fysecured" : "\frtaken") : "\fodropped");
				part_text(above, info, PART_TEXT, 1, teamtype[f.team].colour);
            }
        }
        static vector<int> numflags, iterflags; // dropped/owned
        loopv(numflags) numflags[i] = iterflags[i] = 0;
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent || !(f.base&BASE_FLAG) || !f.owner) continue;
            while(numflags.length() <= f.owner->clientnum) { numflags.add(0); iterflags.add(0); }
            numflags[f.owner->clientnum]++;
        }
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent || !(f.base&BASE_FLAG) || (!f.owner && !f.droptime)) continue;
            const char *flagname = teamtype[f.team].flag;
            vec above(f.pos());
			float trans = 1.f, yaw = 90;
			if(f.owner)
			{
				yaw += f.owner->yaw-45.f+(90/float(numflags[f.owner->clientnum]+1)*(iterflags[f.owner->clientnum]+1));
				while(yaw >= 360.f) yaw -= 360.f;
			}
			else yaw += (f.interptime+(360/st.flags.length()*i))%360;
			int millis = lastmillis-f.interptime;
			if(millis < 1000) trans = float(millis)/1000.f;
            rendermodel(NULL, flagname, ANIM_MAPMODEL|ANIM_LOOP, above, yaw, 0, 0, MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_LIGHT, NULL, NULL, 0, 0, trans);
			above.z += enttype[FLAG].radius*2/3;
			if(f.owner) { above.z += iterflags[f.owner->clientnum]*2; iterflags[f.owner->clientnum]++; }
            defformatstring(info)("@%s flag", teamtype[f.team].name);
			part_text(above, info, PART_TEXT, 1, teamtype[f.team].colour);
			above.z += 2.5f;
			if((f.base&BASE_FLAG) && f.droptime)
			{
				float wait = clamp((lastmillis-f.droptime)/float(st.RESETFLAGTIME), 0.f, 1.f);
				part_icon(above, textureload("textures/progress", 3), 0.25f, 4, 0, 0, 1, teamtype[f.team].colour);
				part_icon(above, textureload("textures/progress", 3), 1, 4, 0, 0, 1, teamtype[f.team].colour, 0, wait);
				defformatstring(str)("@%d%%", int(wait*100.f)); part_text(above, str);
			}
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
			adddynlight(vec(f.pos()).add(vec(0, 0, enttype[FLAG].radius*2/3)), enttype[FLAG].radius*2,
				vec((teamtype[f.team].colour>>16), ((teamtype[f.team].colour>>8)&0xFF), (teamtype[f.team].colour&0xFF)).div(255.f).mul(trans));
        }
    }

    void setupflags()
    {
        st.reset();
        #define setupaddflag(a,b) \
        { \
            index = st.addflag(a->o, a->attr[0], b); \
			if(st.flags.inrange(index)) st.flags[index].ent = a; \
			else continue; \
        }
		#define setupchkflag(a,b) \
		{ \
			if(a->type != FLAG || !chkflagmode(game::gamemode, game::mutators, a->attr[3]) || !isteam(game::gamemode, game::mutators, a->attr[0], TEAM_NEUTRAL)) \
				continue; \
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
            if(!added && isteam(game::gamemode, game::mutators, e->attr[0], TEAM_FIRST)) // not linked and is a team flag
				setupaddflag(e, BASE_BOTH); // add as both
        }
        if(!st.flags.empty()) loopi(numteams(game::gamemode, game::mutators))
        {
        	bool found = false;
        	loopvj(st.flags) if(st.flags[j].team == i+TEAM_FIRST) { found = true; break; }
        	if(!found)
        	{
        		loopvj(st.flags) if(st.flags[j].team == TEAM_NEUTRAL) { found = true; break; }
       			loopvj(st.flags) st.flags[j].team = TEAM_NEUTRAL;
        		if(!found)
        		{
        			loopvj(st.flags) st.flags[j].base &= ~BASE_FLAG;
        			int best = -1;
        			float margin = 1e16f, mindist = 1e16f;
					vector<int> route;
					entities::avoidset obstacles;
   			        for(int q = 0; q < 2; q++) loopvj(entities::ents)
   			        {
						extentity *e = entities::ents[j];
						setupchkflag(e, { continue; });
						vector<float> dists;
						float v = 0;
						if(!q)
						{
							int node = entities::closestent(WAYPOINT, e->o, 1e16f, true);
							bool found = true;
							loopvk(st.flags)
							{
								int goal = entities::closestent(WAYPOINT, st.flags[k].spawnloc, 1e16f, true);
								float u[2] = { entities::route(node, goal, route, obstacles), entities::route(goal, node, route, obstacles) };
								if(u[0] > 0 && u[1] > 0) v += dists.add((u[0]+u[1])*0.5f);
								else
								{
									found = false;
									break;
								}
							}
							if(!found)
							{
								best = -1;
								margin = mindist = 1e16f;
								break;
							}
						}
						else loopvk(st.flags) v += dists.add(st.flags[k].spawnloc.dist(e->o));
						v /= st.flags.length();
						if(v <= mindist)
						{
							float m = 0;
							loopvk(dists) if(k) m += fabs(dists[k]-dists[k-1]);
							if(m <= margin) { best = j; margin = m; mindist = v; }
						}
   			        }
					if(entities::ents.inrange(best)) setupaddflag(entities::ents[best], BASE_FLAG);
        		}
        		break;
        	}
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
		loopvk(st.flags)
		{
            ctfstate::flag &g = st.flags[k];
            if(!g.ent || g.owner || g.droptime || !isctfhome(st.flags[k], d->team)) continue;
			if(d->o.dist(g.pos()) <= enttype[FLAG].radius*4)
			{
				denied = true;
				break;
			}
		}
		int idx = !game::announcefilter || game::player1->state == CS_SPECTATOR || d->team == game::player1->team || isctfflag(f, game::player1->team) ? (denied ? S_V_DENIED : S_V_FLAGDROP) : -1;
		game::announce(idx, CON_INFO, "\fo%s%s dropped the the \fs%s%s\fS flag", game::colorname(d), denied ? " was denied a capture and" : "", teamtype[f.team].chat, teamtype[f.team].name);
		st.dropflag(i, droploc, lastmillis);
		physics::droptofloor(f.droploc, 2, 0);
    }

    void removeplayer(gameent *d)
    {
        loopv(st.flags) if(st.flags[i].owner == d)
        {
            ctfstate::flag &f = st.flags[i];
            st.dropflag(i, f.owner->o, lastmillis);
        }
    }

    void flageffect(int i, int team, const vec &from, const vec &to)
    {
		if(from.x >= 0) game::spawneffect(PART_FIREBALL, vec(from).add(vec(0, 0, enttype[FLAG].radius*2/3)), teamtype[team].colour, enttype[FLAG].radius);
		if(to.x >= 0) game::spawneffect(PART_FIREBALL, vec(to).add(vec(0, 0, enttype[FLAG].radius*2/3)), teamtype[team].colour, enttype[FLAG].radius);
		if(from.x >= 0 && to.x >= 0) part_trail(PART_FIREBALL, 250, from, to, teamtype[team].colour, 2.f, -5);
    }

    void returnflag(gameent *d, int i)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		flageffect(i, d->team, d->feetpos(), f.spawnloc);
		int idx = !game::announcefilter || game::player1->state == CS_SPECTATOR || isctfflag(f, game::player1->team) ? S_V_FLAGRETURN : -1;
		game::announce(idx, CON_INFO, "\fo%s returned the \fs%s%s\fS flag (time taken: \fs\fc%.2f\fS secs)", game::colorname(d), teamtype[f.team].chat, teamtype[f.team].name, float(lastmillis-f.taketime)/1000.f);
		st.returnflag(i);
		f.interptime = lastmillis;
		f.taketime = 0;
    }

    void resetflag(int i)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		flageffect(i, TEAM_NEUTRAL, f.droploc, f.spawnloc);
		int idx = !game::announcefilter || game::player1->state == CS_SPECTATOR || isctfflag(f, game::player1->team) ? S_V_FLAGRESET : -1;
		game::announce(idx, CON_INFO, "\fothe \fs%s%s\fS flag has been reset", teamtype[f.team].chat, teamtype[f.team].name);
		st.returnflag(i);
		f.interptime = lastmillis;
		f.taketime = 0;
    }

    void scoreflag(gameent *d, int relay, int goal, int score)
    {
        if(!st.flags.inrange(goal) || !st.flags.inrange(relay)) return;
		ctfstate::flag &f = st.flags[relay], &g = st.flags[goal];
		flageffect(goal, d->team, g.spawnloc, f.spawnloc);
		(st.findscore(d->team)).total = score;
		part_text(d->abovehead(), "@CAPTURED!", PART_TEXT, 2500, teamtype[d->team].colour, 3.f, -10);
		int idx = !game::announcefilter || game::player1->state == CS_SPECTATOR || d->team == game::player1->team || isctfflag(f, game::player1->team) ? S_V_FLAGSCORE : -1;
		game::announce(idx, CON_INFO, "\fo%s scored the \fs%s%s\fS flag for \fs%s%s\fS team (score: \fs\fc%d\fS, time taken: \fs\fc%.2f\fS secs)", game::colorname(d), teamtype[f.team].chat, teamtype[f.team].name, teamtype[d->team].chat, teamtype[d->team].name, score, float(lastmillis-f.taketime)/1000.f);
		st.returnflag(relay);
		f.interptime = lastmillis;
		g.taketime = 0;
    }

    void takeflag(gameent *d, int i)
    {
        if(!st.flags.inrange(i)) return;
		ctfstate::flag &f = st.flags[i];
		flageffect(i, d->team, d->feetpos(), f.pos());
		int idx = !game::announcefilter || game::player1->state == CS_SPECTATOR || d->team == game::player1->team || isctfflag(f, game::player1->team) ? (f.team == d->team ? S_V_FLAGSECURED : S_V_FLAGPICKUP) : -1;
		game::announce(idx, CON_INFO, "\fo%s %s the \fs%s%s\fS flag", game::colorname(d), f.droptime ? (f.team == d->team ? "secured" : "picked up") : "stole", teamtype[f.team].chat, teamtype[f.team].name);
		if(!f.droptime) f.taketime = lastmillis;
		st.takeflag(i, d);
		f.interptime = lastmillis;
    }

    void checkflags(gameent *d)
    {
        vec o = d->feetpos();
        loopv(st.flags)
        {
            ctfstate::flag &f = st.flags[i];
            if(!f.ent || !(f.base&BASE_FLAG) || f.owner || (f.team == d->team && !f.droptime)) continue;
            if(o.dist(f.pos()) <= enttype[FLAG].radius*2/3)
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
			if(st.flags.inrange(goal) && ai::makeroute(d, b, st.flags[goal].spawnloc))
			{
				d->ai->switchstate(b, ai::AI_S_PURSUE, ai::AI_T_AFFINITY, goal);
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
		if(!ai::badhealth(d))
		{
			while(!takenflags.empty())
			{
				int flag = takenflags.length() > 2 ? rnd(takenflags.length()) : 0;
				if(ai::makeroute(d, b, st.flags[takenflags[flag]].pos()))
				{
					d->ai->switchstate(b, ai::AI_S_PURSUE, ai::AI_T_AFFINITY, takenflags[flag]);
					return true;
				}
				else takenflags.remove(flag);
			}
		}
		return false;
	}

	void aifind(gameent *d, ai::aistate &b, vector<ai::interest> &interests)
	{
		vec pos = d->feetpos();
		loopvj(st.flags)
		{
			ctfstate::flag &f = st.flags[j];
			bool home = isctfhome(f, d->team);
			if(!home && !(f.base&BASE_FLAG)) continue; // don't bother with other bases
			static vector<int> targets; // build a list of others who are interested in this
			targets.setsizenodelete(0);
			bool regen = f.team == TEAM_NEUTRAL || !m_regen(game::gamemode, game::mutators) || !overctfhealth || d->health >= overctfhealth;
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
				else if(d->hasweap(d->arenaweap, m_spawnweapon(game::gamemode, game::mutators)))
				{ // see if we can relieve someone who only has a piece of crap
					gameent *t;
					loopvk(targets) if((t = game::getclient(targets[k])))
					{
						if((t->ai && !t->hasweap(t->arenaweap, m_spawnweapon(game::gamemode, game::mutators))) || (!t->ai && t->weapselect == WEAPON_PISTOL))
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
		if(st.flags.inrange(b.target))
		{
			ctfstate::flag &f = st.flags[b.target];
			if(isctfflag(f, d->team))
			{
				if(ai::makeroute(d, b, f.pos()))
					return f.owner ? ai::violence(d, b, f.owner, false) : true;
			}
			int walk = 0, regen = !m_regen(game::gamemode, game::mutators) || !overctfhealth || d->health >= overctfhealth;
			if(regen && lastmillis-b.millis >= m_speedtime((201-d->skill)*33))
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
					d->ai->trywipe = true; // re-evaluate so as not to herd
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
			if(isctfhome(f, d->team))
			{
				static vector<int> hasflags; hasflags.setsizenodelete(0);
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
				if(ai::makeroute(d, b, f.pos()))
					return f.owner ? ai::violence(d, b, f.owner, false) : true;
			}
			else return ai::makeroute(d, b, f.pos());
		}
		return false;
	}
}
