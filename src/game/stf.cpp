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
			vec p = vec(b.o).add(vec(0, 0, enttype[FLAG].radius*0.75f));
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
			float r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f, fade = blend;
			if(blip)
			{
				float dist = dir.magnitude(),
					diff = dist <= hud::radarrange() ? clamp(1.f-(dist/hud::radarrange()), 0.f, 1.f) : 0.f;
				fade *= 0.25f+(diff*0.5f);
			}
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();
			if(hud::radarflagnames)
			{
				float occupy = !f.owner || f.enemy ? clamp(f.converted/float((f.owner?2:1) * st.OCCUPYLIMIT), 0.f, 1.f) : 1.f;
				if(occupy < 1.f)
					hud::drawblip(w, h, s, fade, -3, dir, r, g, b, "radar", "%s%d%%", teamtype[f.owner].chat, int(occupy*100.f));
				else hud::drawblip(w, h, s, fade, -3, dir, r, g, b, "radar", "%s%s", teamtype[f.owner].chat, teamtype[f.owner].name);
			}
			else hud::drawblip(w, h, s, fade, -3, dir, r, g, b);
		}
	}

	void drawlast(int w, int h, int &tx, int &ty)
	{
		if(world::player1->state == CS_ALIVE)
		{
			loopv(st.flags) if(insideflag(st.flags[i], world::player1) && (st.flags[i].owner == world::player1->team || st.flags[i].enemy == world::player1->team))
			{
				pushfont("emphasis");
				ty -= draw_textx("Team [ \fs%s%s\fS ]", tx-FONTH-FONTH/2, ty, 255, 255, 255, int(255*hudblend), TEXT_RIGHT_UP, -1, -1, teamtype[world::player1->team].chat, teamtype[world::player1->team].name);
				settexture(hud::flagtex(world::player1->team), 3);
				glColor4f(1.f, 1.f, 1.f, int(255*hudblend));
				hud::drawsized(tx-FONTH, ty, FONTH);
				popfont();
				pushfont("default");
				int occupy = int((st.flags[i].enemy ? clamp(st.flags[i].converted/float((st.flags[i].owner ? 2 : 1)*st.OCCUPYLIMIT), 0.f, 1.f) : (st.flags[i].owner ? 1.f : 0.f))*50.f);
				bool overthrow = st.flags[i].owner != world::player1->team;
				ty -= draw_textx("%s [ \fs%s%d%%\fS ] complete", tx, ty, 255, 255, 255, int(255*hudblend), TEXT_RIGHT_UP, -1, -1, overthrow ? "Overthrow" : "Secure", overthrow ? "\fo" : (occupy < 50.f ? "\fy" : "\fg"), occupy+(overthrow ? 0.f : 50.f));
				popfont();
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
			float skew = f.hasflag ? 1.f : 0.5f, fade = blend,
				occupy = f.enemy ? clamp(f.converted/float((f.owner ? 2 : 1)*st.OCCUPYLIMIT), 0.f, 1.f) : (f.owner ? 1.f : 0.f);
			int size = s, millis = lastmillis-f.lasthad, prevsy = sy, delay = lastmillis-world::player1->lastspawn;
			if(millis < 500)
			{
				float amt = clamp(float(millis)/500.f, 0.f, 1.f);
				if(f.hasflag) skew = 0.5f+(amt*0.5f);
				else skew = 1.f-(amt*0.5f);
			}
			if(delay < 1000) skew *= delay/1000.f;
			sy += hud::drawitem(hud::flagtex(f.owner), x, y-sy, size, 1.f, 1.f, 1.f, fade, skew, "default", "%d%%", int(occupy*100.f));
			if(f.enemy) hud::drawitem(hud::flagtex(f.enemy), x, y-prevsy, int(size*0.5f), 1.f, 1.f, 1.f, fade, skew);
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
			s_sprintfd(alias)("flag_%d", e->attr[0]);
			const char *name = getalias(alias);
			if(name[0]) s_strcpy(b.name, name);
			else s_sprintf(b.name)("flag %d", st.flags.length());
			b.ent = e;
		}
	}

	void sendflags(ucharbuf &p)
	{
		putint(p, SV_FLAGS);
		putint(p, st.flags.length());
		loopv(st.flags)
		{
			stfstate::flag &b = st.flags[i];
			putint(p, int(b.o.x*DMF));
			putint(p, int(b.o.y*DMF));
			putint(p, int(b.o.z*DMF));
		}
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
				world::spawneffect(vec(b.o).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[owner].colour, enttype[FLAG].radius);
			}
		}
		else if(b.owner)
		{
			world::announce(S_V_FLAGOVERTHROWN, "\foteam \fs%s%s\fS overthrew %s", teamtype[enemy].chat, teamtype[enemy].name, b.name);
			world::spawneffect(vec(b.o).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[enemy].colour, enttype[FLAG].radius);
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
		vec pos = world::feetpos(d);
		loopvj(st.flags)
		{
			stfstate::flag &f = st.flags[j];
			static vector<int> targets; // build a list of others who are interested in this
			targets.setsizenodelete(0);
			ai::checkothers(targets, d, AI_S_DEFEND, AI_T_AFFINITY, j, true);
			gameent *e = NULL;
			bool regen = !m_regen(world::gamemode, world::mutators) || !overctfhealth || d->health >= overctfhealth;
			loopi(world::numdynents()) if((e = (gameent *)world::iterdynents(i)) && ai::targetable(d, e, false) && !e->ai && d->team == e->team)
			{ // try to guess what non ai are doing
				vec ep = world::feetpos(e);
				if(targets.find(e->clientnum) < 0 && ep.squaredist(f.o) <= (enttype[FLAG].radius*enttype[FLAG].radius))
					targets.add(e->clientnum);
			}
			if((!regen && f.owner == d->team) || (targets.empty() && (f.owner != d->team || f.enemy)))
			{
				interest &n = interests.add();
				n.state = AI_S_DEFEND;
				n.node = entities::closestent(WAYPOINT, f.o, ai::NEARDIST, false);
				n.target = j;
				n.targtype = AI_T_AFFINITY;
				n.score = pos.squaredist(f.o)/(!regen ? 100.f : 1.f);
			}
		}
	}

	bool aidefend(gameent *d, aistate &b)
	{
		if(st.flags.inrange(b.target))
		{
			stfstate::flag &f = st.flags[b.target];
			bool regen = !m_regen(world::gamemode, world::mutators) || !overctfhealth || d->health >= overctfhealth;
			if(regen && !f.enemy && f.owner == d->team)
			{
				d->ai->clear = true; // re-evaluate
				return true;
			}
			return ai::defend(d, b, f.o, float(enttype[FLAG].radius), float(enttype[FLAG].radius*2), 0); // less wander than ctf
		}
		return false;
	}

	bool aipursue(gameent *d, aistate &b)
	{
		b.type = AI_S_DEFEND;
		return aidefend(d, b);
	}

    void removeplayer(gameent *d)
    {
    }
}
