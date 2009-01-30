#ifdef GAMESERVER
struct stfservmode : stfstate, servmode
{
	int scoresec;
	bool notgotflags;

	stfservmode() : scoresec(0), notgotflags(false) {}

	void reset(bool empty)
	{
		stfstate::reset();
		scoresec = 0;
		notgotflags = !empty;
	}

	void stealflag(int n, int team)
	{
		flag &b = flags[n];
		loopv(clients)
		{
			server::clientinfo *ci = clients[i];
			if(!ci->spectator && ci->state.state==CS_ALIVE && ci->team && ci->team == team && insideflag(b, ci->state.o))
				b.enter(ci->team);
		}
		sendflag(n);
	}

	void moveflags(int team, const vec &oldpos, const vec &newpos)
	{
		if(!team || minremain<0) return;
		loopv(flags)
		{
			flag &b = flags[i];
			bool leave = insideflag(b, oldpos),
				 enter = insideflag(b, newpos);
			if(leave && !enter && b.leave(team)) sendflag(i);
			else if(enter && !leave && b.enter(team)) sendflag(i);
			else if(leave && enter && b.steal(team)) stealflag(i, team);
		}
	}

	void leaveflags(int team, const vec &o)
	{
		moveflags(team, o, vec(-1e10f, -1e10f, -1e10f));
	}

	void enterflags(int team, const vec &o)
	{
		moveflags(team, vec(-1e10f, -1e10f, -1e10f), o);
	}

	void addscore(int team, int n)
	{
		if(!n) return;
		score &cs = findscore(team);
		cs.total += n;
		sendf(-1, 1, "ri3", SV_TEAMSCORE, team, cs.total);
	}

	void update()
	{
		if(minremain < 0) return;
		endcheck();
		int t = (gamemillis/1000)-((gamemillis-curtime)/1000);
		if(t < 1) return;
		loopv(flags)
		{
			flag &b = flags[i];
			if(b.enemy)
			{
                if((!b.owners || !b.enemies) && b.occupy(b.enemy, (m_insta(gamemode, mutators) ? OCCUPYPOINTS*2 : OCCUPYPOINTS)*(b.enemies ? b.enemies : -(1+b.owners))*t)==1)
					addscore(b.owner, SECURESCORE);
				sendflag(i);
			}
			else if(b.owner)
			{
				b.securetime += t;
				int score = b.securetime/SCORESECS - (b.securetime-t)/SCORESECS;
				if(score) addscore(b.owner, score);
				sendflag(i);
			}
		}
	}

	void sendflag(int i)
	{
		flag &b = flags[i];
		sendf(-1, 1, "ri5", SV_FLAGINFO, i, b.enemy ? b.converted : 0, b.owner, b.enemy);
	}

	void sendflags()
	{
		ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
		ucharbuf p(packet->data, packet->dataLength);
		initclient(NULL, p, false);
		enet_packet_resize(packet, p.length());
		sendpacket(-1, 1, packet);
		if(!packet->referenceCount) enet_packet_destroy(packet);
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
		putint(p, SV_FLAGS);
		loopv(flags)
		{
			flag &b = flags[i];
			putint(p, b.converted);
			putint(p, b.owner);
			putint(p, b.enemy);
		}
		putint(p, -1);
	}

	void winner(int team, int score)
	{
		sendf(-1, 1, "ri3", SV_TEAMSCORE, team, score);
		startintermission();
	}

	void endcheck()
	{
		int maxscore = sv_stflimit ? sv_stflimit : INT_MAX-1;
		loopi(numteams(gamemode, mutators))
		{
			int lastteam = i+TEAM_FIRST;
			if(findscore(lastteam).total >= maxscore)
			{
				findscore(lastteam).total = maxscore;
				winner(lastteam, maxscore);
				return;
			}
		}
		if(sv_stffinish)
		{
			int lastteam = TEAM_NEUTRAL;
			loopv(flags)
			{
				flag &b = flags[i];
				if(b.owner)
				{
					if(!lastteam) lastteam = b.owner;
					else if(lastteam != b.owner)
					{
						lastteam = TEAM_NEUTRAL;
						break;
					}
				}
				else
				{
					lastteam = TEAM_NEUTRAL;
					break;
				}
			}
			if(lastteam)
			{
				findscore(lastteam).total = maxscore;
				winner(lastteam, maxscore);
				return;
			}
		}
	}

	void entergame(clientinfo *ci)
	{
        if(notgotflags || ci->state.state!=CS_ALIVE) return;
		enterflags(ci->team, ci->state.o);
	}

	void spawned(clientinfo *ci)
	{
		if(notgotflags) return;
		enterflags(ci->team, ci->state.o);
	}

    void leavegame(clientinfo *ci, bool disconnecting = false)
	{
        if(notgotflags || ci->state.state!=CS_ALIVE) return;
		leaveflags(ci->team, ci->state.o);
	}

	void died(clientinfo *ci, clientinfo *actor)
	{
		if(notgotflags) return;
		leaveflags(ci->team, ci->state.o);
	}

	void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
	{
		if(notgotflags) return;
		moveflags(ci->team, oldpos, newpos);
	}

	void changeteam(clientinfo *ci, int oldteam, int newteam)
	{
		if(notgotflags) return;
		leaveflags(oldteam, ci->state.o);
		enterflags(newteam, ci->state.o);
	}

	void parseflags(ucharbuf &p)
	{
		int x = 0;
		while((x = getint(p))>=0)
		{
			vec o;
			o.x = x/DMF;
			o.y = getint(p)/DMF;
			o.z = getint(p)/DMF;
			if(notgotflags) addflag(o);
		}
		if(notgotflags)
		{
			notgotflags = false;
			sendflags();
			loopv(clients) if(clients[i]->state.state==CS_ALIVE) entergame(clients[i]);
		}
	}
} stfmode;
#else
#include "pch.h"
#include "engine.h"
#include "game.h"
namespace stf
{
	stfstate st;

    bool insideflag(const stfstate::flag &b, gameent *d)
    {
        return st.insideflag(b, world::feetpos(d));
    }

    void preload()
    {
        loopi(TEAM_MAX) loadmodel(teamtype[i].flag, -1, true);
    }

	void render()
	{
		loopv(st.flags)
		{
			stfstate::flag &b = st.flags[i];
            if(!b.ent) continue;
			const char *flagname = teamtype[b.owner].flag;
            rendermodel(&b.ent->light, flagname, ANIM_MAPMODEL|ANIM_LOOP, b.o, 0, 0, 0, MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_OCCLUDED);
			int attack = b.enemy ? b.enemy : b.owner, defend = b.owner ? b.owner : b.enemy;
			if(b.enemy && b.owner)
				s_sprintf(b.info)("\fs%s%s\fS vs. \fs%s%s\fS", teamtype[b.owner].chat, teamtype[b.owner].name, teamtype[b.enemy].chat, teamtype[b.enemy].name);
			if(defend) s_sprintf(b.info)("\fs%s%s\fS", teamtype[defend].chat, teamtype[defend].name);
			else b.info[0] = '\0';
			float occupy = attack ? (!b.owner || b.enemy ? clamp(b.converted/float((b.owner?2:1) * st.OCCUPYLIMIT), 0.f, 1.f) : 1.f) : 0.f;
			vec p = vec(b.pos).add(vec(0, 0, enttype[FLAG].radius*0.75f));
			part_meter(p, occupy, b.enemy && b.owner ? PART_METER_VS : PART_METER, 1, teamtype[attack].colour, teamtype[defend].colour); p.z += 2.f;
			part_text(p, b.info); p.z += 2.f;
			s_sprintfd(str)("@%d%%", int(occupy*100.f)); part_text(p, str); p.z += 2.f;
		}
	}

	void drawblip(int w, int h, int s, float blend, int type, bool skipenemy = false)
	{
		loopv(st.flags)
		{
			stfstate::flag &f = st.flags[i];
			if(skipenemy && f.enemy) continue;
			if(type == 0 && (!f.owner || f.owner != world::player1->team)) continue;
			if(type == 1 && f.owner) continue;
			if(type == 2 && (!f.owner || f.owner == world::player1->team)) continue;
			if(type == 3 && (!f.enemy || f.enemy == world::player1->team)) continue;
			bool blip = f.owner != world::player1->team && f.enemy != world::player1->team;
			vec dir(f.o);
			dir.sub(camera1->o);
			int colour = teamtype[f.owner].colour;
			float r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f,
				fade = hud::radarblipblend*blend;
			if(blip)
			{
				float dist = dir.magnitude(),
					diff = dist <= hud::radarrange() ? clamp(1.f-(dist/hud::radarrange()), 0.f, 1.f) : 0.f;
				fade *= 0.5f+(diff*0.5f);
			}
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();
			hud::drawblip(w, h, s, fade, 3, dir, r, g, b);
		}
	}

	void drawlast(int w, int h, int &tx, int &ty)
	{
		if(world::player1->state == CS_ALIVE)
		{
			loopv(st.flags) if(insideflag(st.flags[i], world::player1) && (st.flags[i].owner == world::player1->team || st.flags[i].enemy == world::player1->team))
			{
				ty += draw_textx("Securing..", tx, ty, 255, 255, 255, int(255*hudblend), TEXT_RIGHT_JUSTIFY, -1, -1);
				break;
			}
		}
	}

	void drawblips(int w, int h, int s, float blend)
	{
		bool showenemies = lastmillis%1000 >= 500;
		drawblip(w, h, s, blend, 0, showenemies);
		drawblip(w, h, s, blend, 1, showenemies);
		drawblip(w, h, s, blend, 2, showenemies);
		if(showenemies) drawblip(w, h, s, blend, 3);
	}

    int drawinventory(int x, int y, int s, float blend)
    {
		int sy = 0;
		loopv(st.flags)
		{
			stfstate::flag &f = st.flags[i];
			bool hasflag = world::player1->state == CS_ALIVE &&
				insideflag(f, world::player1) && (f.owner == world::player1->team || f.enemy == world::player1->team);
			if(f.hasflag != hasflag) { f.hasflag = hasflag; f.lasthad = lastmillis-max(500-(lastmillis-f.lasthad), 0); }
			float skew = f.hasflag ? 1.f : hud::inventoryskew, fade = hud::inventoryblend*blend,
				occupy = f.enemy ? clamp(f.converted/float((f.owner ? 2 : 1)*st.OCCUPYLIMIT), 0.f, 1.f) : (f.owner ? 1.f : 0.f);
			int size = s, millis = lastmillis-f.lasthad, prevsy = sy, delay = lastmillis-world::player1->lastspawn;
			if(millis < 500)
			{
				float amt = clamp(float(millis)/500.f, 0.f, 1.f);
				if(f.hasflag) skew = hud::inventoryskew+(amt*(1.f-hud::inventoryskew));
				else skew = 1.f-(amt*(1.f-hud::inventoryskew));
			}
			if(delay < 1000) skew *= delay/1000.f;
			sy += hud::drawitem(hud::flagtex(f.owner), x, y-sy, size, fade, skew, "emphasis", blend, "%d%%", int(occupy*100.f));
			if(f.enemy) hud::drawitem(hud::flagtex(f.enemy), x, y-prevsy, int(size*0.5f), fade, skew);
		}
        return sy;
    }

	void setupflags()
	{
		st.reset();
		loopv(entities::ents)
		{
			extentity *e = entities::ents[i];
			if(e->type!=FLAG) continue;
			stfstate::flag &b = st.flags.add();
			b.o = e->o;
            b.pos = b.o;
			s_sprintfd(alias)("flag_%d", e->attr[0]);
			const char *name = getalias(alias);
			if(name[0]) s_strcpy(b.name, name);
			else s_sprintf(b.name)("flag %d", st.flags.length());
			b.ent = e;
		}
		vec center(0, 0, 0);
		loopv(st.flags) center.add(st.flags[i].o);
		center.div(st.flags.length());
	}

	void sendflags(ucharbuf &p)
	{
		putint(p, SV_FLAGS);
		loopv(st.flags)
		{
			stfstate::flag &b = st.flags[i];
			putint(p, int(b.o.x*DMF));
			putint(p, int(b.o.y*DMF));
			putint(p, int(b.o.z*DMF));
		}
		putint(p, -1);
	}

	void updateflag(int i, int owner, int enemy, int converted)
	{
		if(!st.flags.inrange(i)) return;
		stfstate::flag &b = st.flags[i];
		if(owner)
		{
			if(b.owner != owner)
			{
				world::announce(S_V_FLAGSECURED, "\foteam \fs%s%s\fS secured %s", teamtype[owner].chat, teamtype[owner].name, b.name);
				world::spawneffect(vec(b.pos).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[owner].colour, enttype[FLAG].radius/2);
			}
		}
		else if(b.owner)
		{
			world::announce(S_V_FLAGOVERTHROWN, "\foteam \fs%s%s\fS overthrew %s", teamtype[b.owner].chat, teamtype[b.owner].name, b.name);
			world::spawneffect(vec(b.pos).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[b.owner].colour, enttype[FLAG].radius/2);
		}
		b.owner = owner;
		b.enemy = enemy;
		b.converted = converted;
	}

	void setscore(int team, int total)
	{
		st.findscore(team).total = total;
	}

	bool aicheck(gameent *d, aistate &b)
	{
		return false;
	}

	void aifind(gameent *d, aistate &b, vector<interest> &interests)
	{
		vec pos = world::headpos(d);
		loopvj(st.flags)
		{
			stfstate::flag &f = st.flags[j];
			vector<int> targets; // build a list of others who are interested in this
			ai::checkothers(targets, d, AI_S_DEFEND, AI_T_AFFINITY, j, true);
			gameent *e = NULL;
			loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && AITARG(d, e, false) && !e->ai && d->team == e->team)
			{ // try to guess what non ai are doing
				vec ep = world::headpos(e);
				if(targets.find(e->clientnum) < 0 && ep.squaredist(f.pos) <= ((enttype[FLAG].radius*enttype[FLAG].radius)*2.f))
					targets.add(e->clientnum);
			}
			if(targets.empty() && (f.owner != d->team || f.enemy))
			{
				interest &n = interests.add();
				n.state = AI_S_DEFEND;
				n.node = entities::entitynode(f.pos);
				n.target = j;
				n.targtype = AI_T_AFFINITY;
				n.tolerance = enttype[FLAG].radius*2.f;
				n.score = pos.squaredist(f.pos)/(d->weapselect != d->ai->weappref ? 10.f : 100.f);
				n.defers = false;
				n.expire = 10000;
			}
		}
	}

	bool aidefend(gameent *d, aistate &b)
	{
		if(st.flags.inrange(b.target))
		{
			stfstate::flag &f = st.flags[b.target];
			return ai::defend(d, b, f.pos, float(enttype[FLAG].radius/2));
		}
		return false;
	}

	bool aipursue(gameent *d, aistate &b)
	{
		return aidefend(d, b);
	}
}
#endif
