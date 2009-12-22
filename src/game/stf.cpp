#include "game.h"
namespace stf
{
	stfstate st;

    bool insideflag(const stfstate::flag &b, gameent *d)
    {
        return st.insideflag(b, d->feetpos());
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
            if(!entities::ents.inrange(b.ent)) continue;
			const char *flagname = teamtype[b.owner].flag;
            rendermodel(&entities::ents[b.ent]->light, flagname, ANIM_MAPMODEL|ANIM_LOOP, b.o, entities::ents[b.ent]->attrs[2], entities::ents[b.ent]->attrs[3], 0, MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_OCCLUDED);
			int attack = b.enemy ? b.enemy : b.owner, defend = b.owner ? b.owner : b.enemy;
			if(b.enemy && b.owner)
				formatstring(b.info)("<super>\fs%s%s\fS vs. \fs%s%s\fS", teamtype[b.owner].chat, teamtype[b.owner].name, teamtype[b.enemy].chat, teamtype[b.enemy].name);
			else formatstring(b.info)("<super>\fs%s%s\fS", teamtype[defend].chat, teamtype[defend].name);
			float occupy = attack ? (!b.owner || b.enemy ? clamp(b.converted/float((!stfstyle && b.owner ? 2 : 1)*stfoccupy), 0.f, 1.f) : 1.f) : 0.f;
			vec above = b.o;
			above.z += enttype[FLAG].radius*2/3;
			part_text(above, b.info);
			above.z += 2.5f;
			if(b.enemy)
			{
				part_icon(above, textureload(hud::progresstex, 3), 4, 1, 0, 0, 1, teamtype[b.enemy].colour, 0, occupy);
				part_icon(above, textureload(hud::progresstex, 3), 4, 1, 0, 0, 1, teamtype[b.owner].colour, occupy, 1-occupy);
			}
			else part_icon(above, textureload(hud::progresstex, 3), 4, 1, 0, 0, 1, teamtype[b.owner].colour);
			defformatstring(str)("%d%%", int(occupy*100.f)); part_textcopy(above, str);
		}
	}


    void adddynlights()
    {
        loopv(st.flags)
        {
            stfstate::flag &f = st.flags[i];
            if(!entities::ents.inrange(f.ent)) continue;
			float r = (teamtype[f.owner].colour>>16)/255.f, g = ((teamtype[f.owner].colour>>8)&0xFF)/255.f, b = (teamtype[f.owner].colour&0xFF)/255.f;
            if(f.enemy)
            {
				float r2 = (teamtype[f.enemy].colour>>16)/255.f, g2 = ((teamtype[f.enemy].colour>>8)&0xFF)/255.f, b2 = (teamtype[f.enemy].colour&0xFF)/255.f,
					amt = float(lastmillis%1000)/500.f;
				if(amt > 1.f) amt = 2.f-amt;
            	r += (r2-r)*amt;
            	g += (g2-g)*amt;
            	b += (b2-b)*amt;
            }
			adddynlight(vec(f.o).add(vec(0, 0, enttype[FLAG].radius)), enttype[FLAG].radius*2, vec(r, g, b));
        }
    }

	void drawblips(int w, int h, float blend)
	{
		loopv(st.flags)
		{
			stfstate::flag &f = st.flags[i];
			vec dir(f.o); dir.sub(camera1->o);
			float r = (teamtype[f.owner].colour>>16)/255.f, g = ((teamtype[f.owner].colour>>8)&0xFF)/255.f, b = (teamtype[f.owner].colour&0xFF)/255.f, fade = blend*hud::radarflagblend;
            if(f.enemy)
            {
				float r2 = (teamtype[f.enemy].colour>>16)/255.f, g2 = ((teamtype[f.enemy].colour>>8)&0xFF)/255.f, b2 = (teamtype[f.enemy].colour&0xFF)/255.f,
					amt = float(lastmillis%1000)/500.f;
				if(amt > 1.f) amt = 2.f-amt;
            	r += (r2-r)*amt;
            	g += (g2-g)*amt;
            	b += (b2-b)*amt;
            }
			if(f.owner != game::player1->team && f.enemy != game::player1->team)
			{
				float dist = dir.magnitude(),
					diff = dist <= hud::radarrange() ? clamp(1.f-(dist/hud::radarrange()), 0.f, 1.f) : 0.f;
				fade *= diff*0.5f;
			}
			dir.rotate_around_z(-camera1->yaw*RAD); dir.normalize();
			const char *tex = f.hasflag ? hud::arrowtex : hud::flagtex;
			float size = hud::radarflagsize*(f.hasflag ? 2 : 1);
			if(hud::radarflagnames > (f.hasflag ? 0 : 1))
			{
				float occupy = !f.owner || f.enemy ? clamp(f.converted/float((!stfstyle && f.owner ? 2 : 1) * stfoccupy), 0.f, 1.f) : 1.f;
				bool overthrow = f.owner && f.enemy == game::player1->team;
				if(occupy < 1.f) hud::drawblip(tex, 3, w, h, size, fade, dir, r, g, b, "radar", "%s%d%%", f.hasflag ? (overthrow ? "\fo" : (occupy < 1.f ? "\fy" : "\fg")) : teamtype[f.owner].chat, int(occupy*100.f));
				else hud::drawblip(tex, 3, w, h, size, fade, dir, r, g, b, "radar", "%s%s", f.hasflag ? (overthrow ? "\fo" : (occupy < 1.f ? "\fy" : "\fg")) : teamtype[f.owner].chat, teamtype[f.owner].name);
			}
			else hud::drawblip(tex, 3, w, h, size, fade, dir, r, g, b);
		}
	}

	void drawlast(int w, int h, int &tx, int &ty, float blend)
	{
		if(game::player1->state == CS_ALIVE && hud::shownotices >= 3)
		{
			loopv(st.flags) if(insideflag(st.flags[i], game::player1) && (st.flags[i].owner == game::player1->team || st.flags[i].enemy == game::player1->team))
			{
				stfstate::flag &f = st.flags[i];
				pushfont("super");
				float occupy = !f.owner || f.enemy ? clamp(f.converted/float((!stfstyle && f.owner ? 2 : 1) * stfoccupy), 0.f, 1.f) : 1.f;
				bool overthrow = f.owner && f.enemy == game::player1->team;
				ty += draw_textx("\fzwa%s \fs%s%d%%\fS complete", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1, overthrow ? "Overthrow" : "Secure", overthrow ? "\fo" : (occupy < 1.f ? "\fy" : "\fg"), int(occupy*100.f))*hud::noticescale;
				popfont();
				break;
			}
		}
	}

    int drawinventory(int x, int y, int s, int m, float blend)
    {
		int sy = 0;
		loopv(st.flags)
		{
			if(y-sy-s < m) break;
			stfstate::flag &f = st.flags[i];
			bool hasflag = game::player1->state == CS_ALIVE && insideflag(f, game::player1);
			if(f.hasflag != hasflag) { f.hasflag = hasflag; f.lasthad = lastmillis-max(1000-(lastmillis-f.lasthad), 0); }
			int millis = lastmillis-f.lasthad;
			bool headsup = hud::chkcond(hud::inventorygame, game::player1->state == CS_SPECTATOR || f.owner == game::player1->team || st.flags.length() == 1);
			if(headsup || f.hasflag || millis <= 1000)
			{
				int prevsy = sy; bool skewed = false;
				float skew = headsup ? hud::inventoryskew : 0.f, fade = blend*hud::inventoryblend,
					occupy = f.enemy ? clamp(f.converted/float((!stfstyle && f.owner ? 2 : 1)*stfoccupy), 0.f, 1.f) : (f.owner ? 1.f : 0.f),
					r = (teamtype[f.owner].colour>>16)/255.f, g = ((teamtype[f.owner].colour>>8)&0xFF)/255.f, b = (teamtype[f.owner].colour&0xFF)/255.f,
						r1 = r, g1 = g, b1 = b;
				if(f.enemy)
				{
					float r2 = (teamtype[f.enemy].colour>>16)/255.f, g2 = ((teamtype[f.enemy].colour>>8)&0xFF)/255.f, b2 = (teamtype[f.enemy].colour&0xFF)/255.f,
						amt = float(lastmillis%1000)/500.f;
					if(amt > 1.f) amt = 2.f-amt;
					r += (r2-r)*amt;
					g += (g2-g)*amt;
					b += (b2-b)*amt;
				}
				if(f.hasflag)
				{
					skewed = true;
					skew += (millis <= 1000 ? clamp(float(millis)/1000.f, 0.f, 1.f)*(1.f-skew) : 1.f-skew);
					if(millis > 1000)
					{
						float pc = (millis%1000)/500.f, amt = pc > 1 ? 2.f-pc : pc;
						fade += (1.f-fade)*amt;
					}
				}
				else if(millis <= 1000)
				{
					skewed = true;
					skew += (1.f-skew)-(clamp(float(millis)/1000.f, 0.f, 1.f)*(1.f-skew));
				}
				sy += hud::drawitem(hud::flagtex, x, y-sy, s, false, r, g, b, fade, skew);
				if(f.enemy)
				{
					float r2 = (teamtype[f.enemy].colour>>16)/255.f, g2 = ((teamtype[f.enemy].colour>>8)&0xFF)/255.f, b2 = (teamtype[f.enemy].colour&0xFF)/255.f;
					hud::drawprogress(x, y-prevsy, 0, occupy, int(s*0.5f), false, r2, g2, b2, fade, skew);
					hud::drawprogress(x, y-prevsy, occupy, 1-occupy, int(s*0.5f), false, r1, g1, b1, fade, skew, !skewed && headsup ? "sub" : "radar", "%s%d%%", hasflag ? (f.owner && f.enemy == game::player1->team ? "\fo" : (occupy < 1.f ? "\fy" : "\fg")) : "\fw", int(occupy*100.f));
				}
				else if(f.owner) hud::drawitem(hud::teamtex(f.owner), x, y-prevsy, int(s*0.5f), false, 1.f, 1.f, 1.f, fade, skew);
			}
		}
        return sy;
    }

	void setupflags()
	{
		st.reset();
		loopv(entities::ents)
		{
			extentity *e = entities::ents[i];
			if(e->type != FLAG || !m_check(e->attrs[3], game::gamemode)) continue;
			stfstate::flag &b = st.flags.add();
			b.o = e->o;
			defformatstring(alias)("flag_%d", e->attrs[4]);
			const char *name = getalias(alias);
			if(name[0]) copystring(b.name, name);
			else formatstring(b.name)("flag %d", st.flags.length());
			b.ent = i;
		}
	}

	void sendflags(packetbuf &p)
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
				gameent *d = NULL, *e = NULL;
				loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e->type == ENT_PLAYER && insideflag(b, e))
					if((d = e) == game::player1) break;
				game::announce(S_V_FLAGSECURED, d == game::player1 ? CON_SELF : CON_INFO, d, "\fateam \fs%s%s\fS secured %s", teamtype[owner].chat, teamtype[owner].name, b.name);
				defformatstring(text)("<super>%s\fzReSECURED", teamtype[owner].chat);
				part_textcopy(vec(b.o).add(vec(0, 0, enttype[FLAG].radius)), text, PART_TEXT, game::aboveheadfade, 0xFFFFFF, 3, 1, -10);
				if(game::dynlighteffects) adddynlight(vec(b.o).add(vec(0, 0, enttype[FLAG].radius)), enttype[FLAG].radius*2, vec(teamtype[owner].colour>>16, (teamtype[owner].colour>>8)&0xFF, teamtype[owner].colour&0xFF).mul(2.f/0xFF), 500, 250);
			}
		}
		else if(b.owner)
		{
			gameent *d = NULL, *e = NULL;
			loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && e->type == ENT_PLAYER && insideflag(b, e))
				if((d = e) == game::player1) break;
			game::announce(S_V_FLAGOVERTHROWN, d == game::player1 ? CON_SELF : CON_INFO, d, "\fateam \fs%s%s\fS overthrew %s", teamtype[enemy].chat, teamtype[enemy].name, b.name);
			defformatstring(text)("<super>%s\fzReOVERTHROWN", teamtype[enemy].chat);
			part_textcopy(vec(b.o).add(vec(0, 0, enttype[FLAG].radius)), text, PART_TEXT, game::aboveheadfade, 0xFFFFFF, 3, 1, -10);
			if(game::dynlighteffects) adddynlight(vec(b.o).add(vec(0, 0, enttype[FLAG].radius)), enttype[FLAG].radius*2, vec(teamtype[enemy].colour>>16, (teamtype[enemy].colour>>8)&0xFF, teamtype[enemy].colour&0xFF).mul(2.f/0xFF), 500, 250);
		}
		b.owner = owner;
		b.enemy = enemy;
		b.converted = converted;
	}

	void setscore(int team, int total)
	{
		st.findscore(team).total = total;
	}

	int aiowner(gameent *d)
	{
		loopv(st.flags) if(entities::ents.inrange(st.flags[i].ent) && entities::ents[d->aientity]->links.find(st.flags[i].ent) >= 0)
			return st.flags[i].owner ? st.flags[i].owner : st.flags[i].enemy;
		return d->team;
	}

	bool aicheck(gameent *d, ai::aistate &b)
	{
		return false;
	}

	void aifind(gameent *d, ai::aistate &b, vector<ai::interest> &interests)
	{
		if(d->aitype == AI_BOT)
		{
			vec pos = d->feetpos();
			loopvj(st.flags)
			{
				stfstate::flag &f = st.flags[j];
				static vector<int> targets; // build a list of others who are interested in this
				targets.setsizenodelete(0);
				ai::checkothers(targets, d, ai::AI_S_DEFEND, ai::AI_T_AFFINITY, j, true);
				gameent *e = NULL;
				bool regen = !m_regen(game::gamemode, game::mutators) || d->health >= max(maxhealth, extrahealth);
				loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && !e->ai && ai::owner(d) == ai::owner(e) && ai::targetable(d, e, false))
				{
					vec ep = e->feetpos();
					if(targets.find(e->clientnum) < 0 && ep.squaredist(f.o) <= (enttype[FLAG].radius*enttype[FLAG].radius))
						targets.add(e->clientnum);
				}
				if((!regen && f.owner == ai::owner(d)) || (targets.empty() && (f.owner != ai::owner(d) || f.enemy)))
				{
					ai::interest &n = interests.add();
					n.state = ai::AI_S_DEFEND;
					n.node = entities::closestent(WAYPOINT, f.o, ai::SIGHTMIN, false);
					n.target = j;
					n.targtype = ai::AI_T_AFFINITY;
					n.score = pos.squaredist(f.o)/(!regen ? 100.f : 1.f);
				}
			}
		}
	}

	bool aidefend(gameent *d, ai::aistate &b)
	{
		if(st.flags.inrange(b.target))
		{
			stfstate::flag &f = st.flags[b.target];
			bool regen = d->aitype != AI_BOT || !m_regen(game::gamemode, game::mutators) || d->health >= max(maxhealth, extrahealth);
			int walk = f.enemy && f.enemy != ai::owner(d) ? 1 : 0;
			if(regen && (!f.enemy && ai::owner(d) == f.owner))
			{
				static vector<int> targets; // build a list of others who are interested in this
				targets.setsizenodelete(0);
				ai::checkothers(targets, d, ai::AI_S_DEFEND, ai::AI_T_AFFINITY, b.target, true);
				if(d->aitype == AI_BOT)
				{
					gameent *e = NULL;
					loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && !e->ai && ai::owner(d) == ai::owner(e) && ai::targetable(d, e, false))
					{
						vec ep = e->feetpos();
						if(targets.find(e->clientnum) < 0 && (ep.squaredist(f.o) <= (enttype[FLAG].radius*enttype[FLAG].radius*4)))
							targets.add(e->clientnum);
					}
				}
				if(!targets.empty())
				{
					if(lastmillis-b.millis >= (201-d->skill)*33)
					{
						d->ai->trywipe = true; // re-evaluate so as not to herd
						return true;
					}
					else walk = 2;
				}
				else walk = 1;
			}
			return ai::defend(d, b, f.o, !f.enemy ? ai::CLOSEDIST : float(enttype[FLAG].radius), !f.enemy ? ai::SIGHTMIN : float(enttype[FLAG].radius*(1+walk)), walk);
		}
		return false;
	}

	bool aipursue(gameent *d, ai::aistate &b)
	{
		b.type = ai::AI_S_DEFEND;
		return aidefend(d, b);
	}

    void removeplayer(gameent *d)
    {
    }
}
