#include "cube.h"
#include "engine.h"
#include "game.h"
namespace hud
{
	int damageresidue = 0, hudwidth = 0;
	bool hasinventory = false;
	vector<int> teamkills;
	scoreboard sb;

	VARP(hudsize, 0, 2048, INT_MAX-1);

	VARP(shownotices, 0, 4, 4);
	VARP(showstats, 0, 1, 2);
	VARP(statrate, 0, 200, 1000);
	VARP(showfps, 0, 1, 3);

	VARP(obitnotices, 0, 2, 2);
	VARP(obitnoticetime, 0, 5000, INT_MAX-1);

	VARP(teamwidgets, 0, 1, 3); // colour based on team
	VARP(teamclips, 0, 1, 2);
	VARP(teamcrosshair, 0, 1, 2);
	VARP(teamnotices, 0, 0, 1);
	VARP(teamkillnum, 0, 5, INT_MAX-1);
	VARP(teamkilltime, 0, 30, INT_MAX-1);

	VARP(titlecard, 0, 5000, 10000);
	VARP(showdamage, 0, 1, 2); // 1 shows just damage, 2 includes regen
	TVAR(damagetex, "textures/damage", 3);
	FVARP(damageblend, 0, 0.5f, 1);

	VARP(showdamagecompass, 0, 1, 1);
	VARP(damagecompassfade, 1, 1000, 10000);
	FVARP(damagecompasssize, 0, 0.25f, 1000);
	FVARP(damagecompassblend, 0, 0.5f, 1);
	VARP(damagecompassmin, 1, 25, 1000);
	VARP(damagecompassmax, 1, 200, 1000);

	VARP(showindicator, 0, 1, 1);
	FVARP(indicatorsize, 0, 0.025f, 1000);
	FVARP(indicatorblend, 0, 1.f, 1);
	TVAR(indicatortex, "textures/indicator", 3);
	TVAR(snipetex, "textures/snipe", 3);

	VARP(showcrosshair, 0, 1, 1);
	FVARP(crosshairsize, 0, 0.05f, 1000);
	VARP(crosshairhitspeed, 0, 500, INT_MAX-1);
	FVARP(crosshairblend, 0, 0.5f, 1);
	VARP(crosshairhealth, 0, 2, 2);
	FVARP(crosshairskew, -1, 0.3f, 1);
	TVAR(relativecursortex, "textures/cursordot", 3);
	TVAR(guicursortex, "textures/cursor", 3);
	TVAR(editcursortex, "textures/cursordot", 3);
	TVAR(speccursortex, "textures/cursordot", 3);
	TVAR(crosshairtex, "textures/crosshair", 3);
	TVAR(teamcrosshairtex, "textures/teamcrosshair", 3);
	TVAR(hitcrosshairtex, "textures/hitcrosshair", 3);
	TVAR(snipecrosshairtex, "textures/snipecrosshair", 3);
	FVARP(snipecrosshairsize, 0, 0.6f, 1000);
	FVARP(cursorsize, 0, 0.05f, 1000);
	FVARP(cursorblend, 0, 1.f, 1);

	VARP(showinventory, 0, 1, 1);
	VARP(inventoryammo, 0, 1, 2);
	VARP(inventoryweapids, 0, 1, 2);
	VARP(inventoryweapents, 0, 0, 1);
	VARP(inventoryhealth, 0, 2, 2);
	VARP(inventorythrob, 0, 1, 2);
	VARP(inventorycolour, 0, 1, 2);
	FVARP(inventorysize, 0, 0.05f, 1000);
	FVARP(inventoryblend, 0, 0.5f, 1);
	TVAR(plasmatex, "textures/plasma", 3);
	TVAR(shotguntex, "textures/shotgun", 3);
	TVAR(chainguntex, "textures/chaingun", 3);
	TVAR(grenadestex, "textures/grenades", 3);
	TVAR(flamertex, "textures/flamer", 3);
	TVAR(carbinetex, "textures/carbine", 3);
	TVAR(rifletex, "textures/rifle", 3);
	TVAR(paintguntex, "textures/paintgun", 3);
	TVAR(healthtex, "textures/health", 3);
	TVAR(neutralflagtex, "textures/team", 3);
	TVAR(alphaflagtex, "textures/teamalpha", 3);
	TVAR(betaflagtex, "textures/teambeta", 3);
	TVAR(deltaflagtex, "textures/teamdelta", 3);
	TVAR(gammaflagtex, "textures/teamgamma", 3);

	VARP(showclip, 0, 1, 1);
	FVARP(clipsize, 0, 0.05f, 1000);
	FVARP(clipblend, 0, 0.5f, 1000);
	VARP(clipcolour, 0, 1, 1);
	TVAR(plasmacliptex, "textures/plasmaclip", 3);
	TVAR(shotguncliptex, "textures/shotgunclip", 3);
	TVAR(chainguncliptex, "textures/chaingunclip", 3);
	TVAR(grenadescliptex, "textures/grenadesclip", 3);
	TVAR(flamercliptex, "textures/flamerclip", 3);
	TVAR(carbinecliptex, "textures/carbineclip", 3);
	TVAR(riflecliptex, "textures/rifleclip", 3);
	TVAR(paintguncliptex, "textures/paintgunclip", 3);

	VARP(showradar, 0, 2, 2);
	TVAR(radartex, "textures/radar", 3);
	FVARP(radarblend, 0, 1.f, 1);
	FVARP(radarcardblend, 0, 1.f, 1);
	FVARP(radarplayerblend, 0, 0.5f, 1);
	FVARP(radarblipblend, 0, 0.5f, 1);
	FVARP(radarflagblend, 0, 1.f, 1);
	FVARP(radaritemblend, 0, 0.5f, 1);
	FVARP(radarsize, 0, 0.02f, 1000);
	FVARP(radaroffset, 0, 0.0515f, 1000);
	VARP(radardist, 0, 256, INT_MAX-1);
	VARP(radarcard, 0, 1, 2);
	VARP(radaritems, 0, 2, 2);
	VARP(radaritemspawn, 0, 1, 1);
	VARP(radaritemtime, 0, 3000, INT_MAX-1);
	VARP(radaritemnames, 0, 0, 2);
	VARP(radarplayers, 0, 1, 2);
	VARP(radarplayernames, 0, 0, 2);
	VARP(radarflags, 0, 2, 2);
	VARP(radarflagnames, 0, 0, 1);
	VARP(showeditradar, 0, 1, 1);
	VARP(editradardist, 0, 32, INT_MAX-1);
	VARP(editradarnoisy, 0, 1, 2);

    VARP(showborder, 0, 1, 2);
	VARP(borderhealth, 0, 1, 2);
	FVARP(bordersize, 0, 0.025f, 1000);
	FVARP(borderblend, 0, 0.5f, 1);
	FVARP(borderskew, -1, -0.3f, 1);

	bool hastv(int val)
	{
		if(val == 2 || (val && world::tvmode())) return true;
		return false;
	}

	void drawquad(float x, float y, float w, float h, float tx1, float ty1, float tx2, float ty2)
	{
		glBegin(GL_QUADS);
		glTexCoord2f(tx1, ty1); glVertex2f(x, y);
		glTexCoord2f(tx2, ty1); glVertex2f(x+w, y);
		glTexCoord2f(tx2, ty2); glVertex2f(x+w, y+h);
		glTexCoord2f(tx1, ty2); glVertex2f(x, y+h);
		glEnd();
	}
	void drawtex(float x, float y, float w, float h, float tx, float ty, float tw, float th) { drawquad(x, y, w, h, tx, ty, tx+tw, ty+th); }
	void drawsized(float x, float y, float s) { drawtex(x, y, s, s); }

	void colourskew(float &r, float &g, float &b, float skew)
	{
		if(skew >= 2.f)
		{ // fully overcharged to green
			r = b = 0.f;
		}
		else if(skew >= 1.f)
		{ // overcharge to yellow
			b *= 1.f-(skew-1.f);
		}
		else if(skew >= 0.5f)
		{ // fade to orange
			float off = skew-0.5f;
			g *= 0.5f+off;
			b *= off*2.f;
		}
		else
		{ // fade to red
			g *= skew;
			b = 0.f;
		}
	}

	void healthskew(int &s, float &r, float &g, float &b, float &fade, float ss, bool throb)
	{
		if(throb && regentime && world::player1->lastregen && lastmillis-world::player1->lastregen < regentime*1000)
		{
			float skew = clamp((lastmillis-world::player1->lastregen)/float(regentime*1000), 0.f, 1.f);
			if(skew > 0.5f) skew = 1.f-skew;
			fade *= 1.f-skew;
			s += int(s*ss*skew);
		}
		int total = m_maxhealth(world::gamemode, world::mutators);
		bool full = world::player1->health >= total;
		if(full)
		{
			int over = total;
			if(m_ctf(world::gamemode)) over = max(overctfhealth, total);
			if(m_stf(world::gamemode)) over = max(overstfhealth, total);
			if(over > total)
				colourskew(r, g, b, 1.f+clamp(float(world::player1->health-total)/float(over-total), 0.f, 1.f));
		}
		else colourskew(r, g, b, clamp(float(world::player1->health)/float(total), 0.f, 1.f));
	}

	void skewcolour(float &r, float &g, float &b)
	{
		if(m_team(world::gamemode, world::mutators) && world::player1->state != CS_SPECTATOR && world::player1->state != CS_EDITING)
		{
			r *= (teamtype[world::player1->team].colour>>16)/255.f;
			g *= ((teamtype[world::player1->team].colour>>8)&0xFF)/255.f;
			b *= (teamtype[world::player1->team].colour&0xFF)/255.f;
		}
	}

	void skewcolour(int &r, int &g, int &b)
	{
		if(m_team(world::gamemode, world::mutators) && world::player1->state != CS_SPECTATOR && world::player1->state != CS_EDITING)
		{
			r = int(r*((teamtype[world::player1->team].colour>>16)/255.f));
			g = int(g*(((teamtype[world::player1->team].colour>>8)&0xFF)/255.f));
			b = int(b*((teamtype[world::player1->team].colour&0xFF)/255.f));
		}
	}

	enum
	{
		POINTER_NONE = 0, POINTER_RELATIVE, POINTER_GUI, POINTER_EDIT, POINTER_SPEC,
		POINTER_HAIR, POINTER_TEAM, POINTER_HIT, POINTER_SNIPE, POINTER_MAX
	};

    const char *getpointer(int index)
    {
        switch(index)
        {
            case POINTER_RELATIVE: default: return relativecursortex; break;
            case POINTER_GUI: return guicursortex; break;
            case POINTER_EDIT: return editcursortex; break;
            case POINTER_SPEC: return speccursortex; break;
            case POINTER_HAIR: return crosshairtex; break;
            case POINTER_TEAM: return teamcrosshairtex; break;
            case POINTER_HIT: return hitcrosshairtex; break;
            case POINTER_SNIPE: return snipecrosshairtex; break;
        }
        return NULL;
    }

	void drawindicator(int weap, int x, int y, int s)
	{
		Texture *t = textureload(indicatortex, 3);
		if(t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
		int millis = lastmillis-world::player1->weaplast[weap];
		float r = 1.f, g = 1.f, b = 1.f, amt = 0.f;
		switch(world::player1->weapstate[weap])
		{
			case WPSTATE_POWER:
			{
				amt = clamp(float(millis)/float(weaptype[weap].power), 0.f, 1.f);
				colourskew(r, g, b, 1.f-amt);
				break;
			}
			default: amt = 0.f; break;
		}
		glBindTexture(GL_TEXTURE_2D, t->getframe(amt));
		glColor4f(r, g, b, indicatorblend);
		if(t->frames.length() > 1) drawsized(x-s/2, y-s/2, s);
		else drawslice(0, clamp(amt, 0.f, 1.f), x, y, s);
	}

    void drawclip(int weap, int x, int y, float s)
    {
        const char *cliptexs[WEAPON_MAX] = {
            plasmacliptex, shotguncliptex, chainguncliptex,
            flamercliptex, carbinecliptex, riflecliptex, grenadescliptex, // end of regular weapons
			paintguncliptex
        };
        Texture *t = textureload(cliptexs[weap], 3);
        int ammo = world::player1->ammo[weap], maxammo = weaptype[weap].max;
		if(t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);

		float fade = clipblend;
		if(lastmillis-world::player1->weaplast[weap] < world::player1->weapwait[weap]) switch(world::player1->weapstate[weap])
		{
			case WPSTATE_RELOAD: case WPSTATE_PICKUP: case WPSTATE_SWITCH:
			{
				fade *= clamp(float(lastmillis-world::player1->weaplast[weap])/float(world::player1->weapwait[weap]), 0.f, 1.f);
				break;
			}
			default: break;
		}
        switch(weap)
        {
            case WEAPON_PLASMA: case WEAPON_FLAMER: case WEAPON_CG: s *= 0.8f; break;
            default: break;
        }
        float r = 1.f, g = 1.f, b = 1.f;
		if(teamclips >= (clipcolour ? 2 : 1)) skewcolour(r, g, b);
		else if(clipcolour)
		{
			r = (weaptype[weap].colour>>16)/255.f;
			g = ((weaptype[weap].colour>>8)&0xFF)/255.f;
			b = (weaptype[weap].colour&0xFF)/255.f;
		}
        glColor4f(r, g, b, fade);
        glBindTexture(GL_TEXTURE_2D, t->retframe(ammo, maxammo));
        if(t->frames.length() > 1) drawsized(x-s/2, y-s/2, s);
        else switch(weap)
        {
            case WEAPON_FLAMER:
				drawslice(0, max(ammo-min(maxammo-ammo, 2), 0)/float(maxammo), x, y, s);
				if(world::player1->ammo[weap] < weaptype[weap].max)
					drawfadedslice(max(ammo-min(maxammo-ammo, 2), 0)/float(maxammo),
						min(min(maxammo-ammo, ammo), 2) /float(maxammo),
							x, y, s, fade, r, g, b);
                break;

            default:
                drawslice(0.5f/maxammo, ammo/float(maxammo), x, y, s);
                break;
        }
    }

	void drawpointerindex(int index, int x, int y, int s, float r, float g, float b, float fade)
	{
		Texture *t = textureload(getpointer(index), 3);
		if(t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
		glColor4f(r, g, b, fade);
		glBindTexture(GL_TEXTURE_2D, t->id);
		bool guicursor = index == POINTER_GUI;
		drawsized(guicursor ? x : x-s/2, guicursor ? y : y-s/2, s);
	}

	void drawpointer(int w, int h, int index)
	{
		bool guicursor = index == POINTER_GUI;
		int cs = int((guicursor ? cursorsize : crosshairsize)*hudsize);
		float r = 1.f, g = 1.f, b = 1.f, fade = (guicursor ? cursorblend : crosshairblend)*hudblend;
		if(teamcrosshair >= (crosshairhealth ? 2 : 1)) skewcolour(r, g, b);
		if(world::player1->state == CS_ALIVE && index >= POINTER_HAIR)
		{
			if(index == POINTER_SNIPE)
			{
				cs = int(snipecrosshairsize*hudsize);
				if(world::inzoom() && weaptype[world::player1->weapselect].snipes)
				{
					int frame = lastmillis-world::lastzoom;
					float amt = frame < world::zoomtime() ? clamp(float(frame)/float(world::zoomtime()), 0.f, 1.f) : 1.f;
					if(!world::zooming) amt = 1.f-amt;
					cs = int(cs*amt);
				}
			}
			if(crosshairhealth) healthskew(cs, r, g, b, fade, crosshairskew, crosshairhealth > 1);
		}
		int cx = int(hudwidth*cursorx), cy = int(hudsize*cursory), nx = int(hudwidth*0.5f), ny = int(hudsize*0.5f);
		drawpointerindex(index, world::mousestyle() != 1 ? cx : nx, world::mousestyle() != 1 ? cy : ny, cs, r, g, b, fade);
		if(index > POINTER_GUI)
		{
			if(world::player1->state == CS_ALIVE)
			{
				if(world::player1->hasweap(world::player1->weapselect, m_spawnweapon(world::gamemode, world::mutators)))
				{
					if(showclip) drawclip(world::player1->weapselect, nx, ny, clipsize*hudsize);
					if(showindicator && weaptype[world::player1->weapselect].power && world::player1->weapstate[world::player1->weapselect] == WPSTATE_POWER)
						drawindicator(world::player1->weapselect, nx, ny, int(indicatorsize*hudsize));
				}
			}
			if(world::mousestyle() >= 1) // renders differently
				drawpointerindex(POINTER_RELATIVE, world::mousestyle() != 1 ? nx : cx, world::mousestyle() != 1 ? ny : cy, int(crosshairsize*hudsize), 1.f, 1.f, 1.f, crosshairblend);
		}
	}

	void drawpointers(int w, int h)
	{
        int index = POINTER_NONE;
		if(UI::hascursor()) index = UI::hascursor(true) ? POINTER_GUI : POINTER_NONE;
        else if(!showcrosshair || world::player1->state == CS_DEAD || !connected() || !client::ready()) index = POINTER_NONE;
        else if(world::player1->state == CS_EDITING) index = POINTER_EDIT;
        else if(world::player1->state == CS_SPECTATOR || world::player1->state == CS_WAITING) index = POINTER_SPEC;
        else if(world::inzoom() && weaptype[world::player1->weapselect].snipes) index = POINTER_SNIPE;
        else if(lastmillis-world::player1->lasthit <= crosshairhitspeed) index = POINTER_HIT;
        else if(m_team(world::gamemode, world::mutators))
        {
            vec pos = world::headpos(world::player1, 0.f);
            gameent *d = world::intersectclosest(pos, worldpos, world::player1);
            if(d && d->type == ENT_PLAYER && d->team == world::player1->team)
				index = POINTER_TEAM;
			else index = POINTER_HAIR;
        }
        else index = POINTER_HAIR;
		if(index > POINTER_NONE) drawpointer(w, h, index);
	}

	int numteamkills()
	{
		int numkilled = 0;
		loopvrev(teamkills)
		{
			if(lastmillis-teamkills[i] <= teamkilltime*1000) numkilled++;
			else teamkills.remove(i);
		}
		return numkilled;
	}

	void drawlast(int w, int h)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, hudwidth, hudsize, 0, -1, 1);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// superhud!
		if(shownotices && world::maptime && !world::player1->conopen)
		{
			pushfont("super");
			int ofy = int(hudsize*bordersize*(hastv(showborder) ? 1.5f : 1.f)), ofx = int(ofy*(hastv(showborder) ? 1.5f : 1.f)),
				inv = hasinventory ? int(hudsize*inventorysize*1.5f) : 0,
				ty = hudsize-ofy, tx = hudwidth-ofx-inv, tf = int(255*hudblend),
				tr = 255, tg = 255, tb = 255;
			if(teamnotices) skewcolour(tr, tg, tb);
			if(lastmillis-world::maptime <= titlecard)
			{
				const char *title = getmaptitle();
				if(!*title) title = getmapname();
				ty -= draw_textx("%s", tx, ty, 255, 255, 255, tf, TEXT_RIGHT_UP, -1, -1, title);
				pushfont("emphasis");
				ty -= draw_textx("Playing: %s", tx, ty, 255, 255, 255, tf, TEXT_RIGHT_UP, -1, -1, server::gamename(world::gamemode, world::mutators));
				popfont();
			}

			if(m_ctf(world::gamemode)) ctf::drawlast(w, h, tx, ty);
			else if(m_stf(world::gamemode)) stf::drawlast(w, h, tx, ty);

			if(world::player1->state == CS_DEAD || world::player1->state == CS_WAITING)
			{
				int sdelay = m_spawndelay(world::gamemode, world::mutators), delay = world::player1->lastdeath ? world::player1->respawnwait(lastmillis, sdelay) : 0;
				const char *msg = world::player1->state != CS_WAITING && world::player1->lastdeath ? (m_paint(world::gamemode, world::mutators) ? "Tagged!" : "Fragged!") : "Please Wait";
				ty -= draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, msg);
				if(obitnotices && world::player1->lastdeath && *world::player1->obit)
				{
					pushfont("default");
					ty -= draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, world::player1->obit);
					popfont();
				}
				if(shownotices >= 2)
				{
					SEARCHBINDCACHE(attackkey)("attack", 0);
					if(delay || m_duke(world::gamemode, world::mutators))
					{
						pushfont("emphasis");
						if(m_duke(world::gamemode, world::mutators) || !world::player1->lastdeath)
							ty -= draw_textx("Waiting for new round", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1);
						else if(delay) ty -= draw_textx("Down for [ \fs\fy%.1f\fS ] second(s)", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, delay/1000.f);
						popfont();
						if(world::player1->state != CS_WAITING && shownotices >= 3 && lastmillis-world::player1->lastdeath > 500)
						{
							pushfont("default");
							ty -= draw_textx("Press [ \fs\fa%s\fS ] to look around", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, attackkey);
							popfont();
						}
					}
					else
					{
						pushfont("emphasis");
						ty -= draw_textx("Ready to respawn", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1);
						popfont();
						if(world::player1->state != CS_WAITING && shownotices >= 3)
						{
							pushfont("default");
							ty -= draw_textx("Press [ \fs\fa%s\fS ] to respawn", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, attackkey);
							popfont();
						}
					}
				}
			}
			else if(world::player1->state == CS_ALIVE)
			{
				if(teamkillnum && m_team(world::gamemode, world::mutators) && numteamkills() >= teamkillnum)
				{
					ty -= draw_textx("%sDon't shoot team mates!", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, lastmillis%500 >= 250 ? "\fo" : "\fy");
					if(shownotices >= 2)
					{
						pushfont("emphasis");
						ty -= draw_textx("You are on team [ \fs%s%s\fS ]", tx-FONTH-FONTH/2, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, teamtype[world::player1->team].chat, teamtype[world::player1->team].name);
						settexture(flagtex(world::player1->team), 3);
						glColor4f(1.f, 1.f, 1.f, tf);
						drawsized(tx-FONTH, ty, FONTH);
						popfont();
						pushfont("default");
						ty -= draw_textx("Shoot anyone not the \fs%ssame colour\fS!", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, teamtype[world::player1->team].chat);
						popfont();
					}
				}
				if(obitnotices && lastmillis-world::player1->lastkill <= obitnoticetime && *world::player1->obit)
				{
					pushfont("default");
					ty -= draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, world::player1->obit);
					popfont();
				}
				if(shownotices >= 3)
				{
					pushfont("default");
					if(world::player1->requse < 0)
					{
						static vector<actitem> actitems;
						actitems.setsizenodelete(0);
						if(entities::collateitems(world::player1, actitems))
						{
							SEARCHBINDCACHE(actionkey)("action", 0);
							while(!actitems.empty())
							{
								actitem &t = actitems.last();
								int ent = -1;
								switch(t.type)
								{
									case ITEM_ENT:
									{
										if(!entities::ents.inrange(t.target)) break;
										ent = t.target;
										break;
									}
									case ITEM_PROJ:
									{
										if(!projs::projs.inrange(t.target)) break;
										projent &proj = *projs::projs[t.target];
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
										int drop = -1, sweap = m_spawnweapon(world::gamemode, world::mutators), attr = e.type == WEAPON ? weapattr(e.attr[0], sweap) : e.attr[0];
										if(e.type == WEAPON && weapcarry(world::player1->weapselect, sweap) && world::player1->ammo[attr] < 0 &&
											weapcarry(attr, sweap) && world::player1->carry(sweap) >= maxcarry) drop = world::player1->drop(sweap, attr);
										if(isweap(drop))
										{
											s_sprintfd(dropweap)("%s", entities::entinfo(WEAPON, drop, 0, 0, 0, 0, false));
											ty -= draw_textx("Press [ \fs\fa%s\fS ] to swap [ \fs%s\fS ] for [ \fs%s\fS ]", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, actionkey, dropweap, entities::entinfo(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], false));
										}
										else ty -= draw_textx("Press [ \fs\fa%s\fS ] to pickup [ \fs%s\fS ]", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, actionkey, entities::entinfo(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], false));
										break;
									}
									else if(e.type == TRIGGER && e.attr[2] == TA_ACT)
									{
										ty -= draw_textx("Press [ \fs\fa%s\fS ] to interact", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, actionkey);
										break;
									}
								}
								actitems.pop();
							}
						}
					}
					if(shownotices >= 4)
					{
						if(world::player1->hasweap(world::player1->weapselect, m_spawnweapon(world::gamemode, world::mutators)))
						{
							SEARCHBINDCACHE(zoomkey)("zoom", 0);
							ty -= draw_textx("Press [ \fs\fa%s\fS ] to %s", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, zoomkey, weaptype[world::player1->weapselect].snipes ? "zoom" : "prone");
						}
						if(world::player1->canshoot(world::player1->weapselect, m_spawnweapon(world::gamemode, world::mutators), lastmillis))
						{
							SEARCHBINDCACHE(attackkey)("attack", 0);
							ty -= draw_textx("Press [ \fs\fa%s\fS ] to attack", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, attackkey);
						}
						if(world::player1->reqreload < 0 && world::player1->canreload(world::player1->weapselect, m_spawnweapon(world::gamemode, world::mutators), lastmillis))
						{
							SEARCHBINDCACHE(reloadkey)("reload", 0);
							ty -= draw_textx("Press [ \fs\fa%s\fS ] to reload ammo", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, reloadkey);
							if(weapons::autoreload > 1 && lastmillis-world::player1->weaplast[world::player1->weapselect] <= 1000)
								ty -= draw_textx("Automatic reload in [ \fs\fy%.01f\fS ] second(s)", tx, ty, tr, tg, tb, tf, TEXT_RIGHT_UP, -1, -1, float(1000-(lastmillis-world::player1->weaplast[world::player1->weapselect]))/1000.f);
						}
					}
					popfont();
				}
			}
			else if(world::player1->state == CS_EDITING)
			{
				int gotit[MAXENTTYPES], numgot = 0;
				loopi(MAXENTTYPES) gotit[i] = 0;
				loopv(entities::ents) if(entgroup.find(i) >= 0 || enthover == i)
				{
					gameentity &e = *(gameentity *)entities::ents[i];
					if(gotit[e.type] < 3 && entities::cansee(e))
					{
						pushfont("default");
						ty -= draw_textx("%s (%d)", tx, ty, 255, 255, 255, tf, TEXT_RIGHT_UP, -1, -1, enttype[e.type].name, i);
						popfont();
						pushfont("sub");
						const char *info = entities::entinfo(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], true);
						if(info && *info) ty -= draw_textx("%s", tx, ty, 255, 255, 255, tf, TEXT_RIGHT_UP, -1, -1, info);
						loopk(5)
						{
							if(*enttype[e.type].attrs[k])
								ty -= draw_textx("%s:%d", tx, ty, 255, 255, 255, tf, TEXT_RIGHT_UP, -1, -1, enttype[e.type].attrs[k], e.attr[k]);
							else break;
						}
						popfont();
						gotit[e.type]++;
						numgot++;
					}
				}
			}
			else if(world::player1->state == CS_SPECTATOR)
			{
				ty -= draw_textx("%s", tx, ty, 255, 255, 255, tf, TEXT_RIGHT_UP, -1, -1, world::tvmode() ? "SpecTV" : "Spectating");
				if(shownotices >= 2)
				{
					SEARCHBINDCACHE(speconkey)("spectator 0", 1);
					pushfont("default");
					ty -= draw_textx("Press [ \fs\fa%s\fS ] to play", tx, ty, 255, 255, 255, tf, TEXT_RIGHT_UP, -1, -1, speconkey);
					if(shownotices >= 3)
					{
						SEARCHBINDCACHE(specmodekey)("specmodeswitch", 1);
						ty -= draw_textx("Press [ \fs\fa%s\fS ] to %s", tx, ty, 255, 255, 255, tf, TEXT_RIGHT_UP, -1, -1, specmodekey, world::tvmode() ? "look around" : "observe");
					}
					popfont();
				}
			}

			popfont();
		}

		drawpointers(w, h); // do this last, as it has to interact with the lower levels unhindered

		glDisable(GL_BLEND);
	}

	float radarrange()
	{
		float dist = float(radardist);
		if(world::player1->state == CS_EDITING) dist = float(editradardist);
		return dist;
	}

	void drawblip(int w, int h, int s, float blend, int idx, vec &dir, float r, float g, float b, const char *font, const char *text, ...)
	{
		float fx = 0.f, fy = 0.f, fw = 1.f, fh = 1.f, fade = blend;
		int tx = w/2, ty = h/2, ts = int(s*radarsize), tr = int(s*radaroffset);
		if(idx < 0)
		{
			int blip = clamp(-idx-1, 0, 3);
			const float rdblip[4][2] = { // player entity flag card
				 { 0.25f, 0.25f }, { 0.25f, 0.5f }, { 0.5f, 0.25f }, { 0.5f, 0.5f }
			};
			fx = rdblip[blip][0];
			fy = rdblip[blip][1];
			fw = fh = 0.25f;
			settexture(radartex, 3);
			if(blip) tr += ts*blip;
			if(blip < 2) ts = ts*2/3;
			switch(blip)
			{
				case 0: fade *= radarplayerblend; break;
				case 1: default: fade *= radarblipblend; break;
				case 2: fade *= radarflagblend; break;
				case 3: fade *= radarcardblend; break;
			}
		}
		else if(isweap(idx))
		{
			settexture(hud::itemtex(WEAPON, idx), 3);
			tr += ts; // sit in the entity zone
			fade *= radaritemblend;
		}
		else return;
		float yaw = -(float)atan2(dir.x, dir.y)/RAD + 180, x = sinf(RAD*yaw), y = -cosf(RAD*yaw);
		int tq = ts/2, cx = int(tx+(tr*x)-tq), cy = int(ty+(tr*y)-tq);
		glColor4f(r, g, b, blend);
		drawtex(cx, cy, ts, ts, fx, fy, fw, fh);
		if(text && *text)
		{
			if(font && *font) pushfont(font);
			s_sprintfdlv(str, text, text);
			draw_textx("%s", cx+tq, cy+(idx > -4 ? ts : ts/2-FONTH/2), 255, 255, 255, int(blend*255.f), TEXT_CENTERED|TEXT_NO_INDENT, -1, -1, str);
			if(font && *font) popfont();
		}
	}

	void drawplayerblip(gameent *d, int w, int h, int s, float blend)
	{
		vec dir = world::headpos(d);
		dir.sub(camera1->o);
		float dist = dir.magnitude();
		if(dist < radarrange())
		{
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();
			int colour = teamtype[d->team].colour, delay = d->protect(lastmillis, spawnprotecttime*1000);
			float fade = clamp(1.f-(dist/radarrange()), 0.f, 1.f)*blend,
				r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f;
			if(delay > 0) fade *= clamp(float(delay)/float(spawnprotecttime*1000), 0.f, 1.f);
			if(hastv(radarplayernames)) drawblip(w, h, s, fade, -1, dir, r, g, b, "radar", "%s", world::colorname(d, NULL, "", false));
			else drawblip(w, h, s, fade, -1, dir, r, g, b);
		}
	}

	void drawcardinalblips(int w, int h, int s, float blend, bool altcard)
	{
		loopi(4)
		{
			const char *card = "";
			vec dir(camera1->o);
			switch(i)
			{
				case 0: dir.sub(vec(0, 1, 0)); card = altcard ? "0" : "N"; break;
				case 1: dir.add(vec(1, 0, 0)); card = altcard ? "90" : "E"; break;
				case 2: dir.add(vec(0, 1, 0)); card = altcard ? "180" : "S"; break;
				case 3: dir.sub(vec(1, 0, 0)); card = altcard ? "270" : "W"; break;
				default: break;
			}
			dir.sub(camera1->o);
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();
			drawblip(w, h, s, blend, -4, dir, 1.f, 1.f, 1.f, "radar", "%s", card);
		}
	}

	void drawentblip(int w, int h, int s, float blend, int n, vec &o, int type, int attr1, int attr2, int attr3, int attr4, int attr5, bool spawned, int lastspawn, bool insel)
	{
		if(type > NOTUSED && type < MAXENTTYPES && ((enttype[type].usetype == EU_ITEM && spawned) || world::player1->state == CS_EDITING))
		{
			float inspawn = radaritemtime && spawned && lastspawn && lastmillis-lastspawn <= radaritemtime ? float(lastmillis-lastspawn)/float(radaritemtime) : 0.f;
			if(enttype[type].noisy && (world::player1->state != CS_EDITING || !editradarnoisy || (editradarnoisy < 2 && !insel)))
				return;
			if(world::player1->state != CS_EDITING && radaritemspawn && (enttype[type].usetype != EU_ITEM || inspawn <= 0.f))
				return;
			vec dir(o);
			dir.sub(camera1->o);
			float dist = dir.magnitude();
			if(dist >= radarrange())
			{
				if(insel || inspawn > 0.f) dir.mul(radarrange()/dist);
				else return;
			}
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();
			int cp = type == WEAPON ? weapattr(attr1, m_spawnweapon(world::gamemode, world::mutators)) : -2;
			float r = 1.f, g = 1.f, b = 1.f, fade = insel ? 1.f : clamp(1.f-(dist/radarrange()), 0.1f, 1.f)*blend;
			if(cp >= 0)
			{
				r = (weaptype[cp].colour>>16)/255.f;
				g = ((weaptype[cp].colour>>8)&0xFF)/255.f;
				b = (weaptype[cp].colour&0xFF)/255.f;
			}
			if(world::player1->state != CS_EDITING && !insel && inspawn > 0.f)
				fade = radaritemspawn ? 1.f-inspawn : fade+((1.f-fade)*(1.f-inspawn));
			if(insel) drawblip(w, h, s, fade, cp, dir, r, g, b, "radar", "%s\n%s", enttype[type].name, entities::entinfo(type, attr1, attr2, attr3, attr4, attr5, insel));
			else if(hastv(radaritemnames)) drawblip(w, h, s, fade, cp, dir, r, g, b, "radar", "%s", entities::entinfo(type, attr1, attr2, attr3, attr4, attr5, false));
			else drawblip(w, h, s, fade, cp, dir, r, g, b);
		}
	}

	void drawentblips(int w, int h, int s, float blend)
	{
		if(m_edit(world::gamemode) && world::player1->state == CS_EDITING && (entities::ents.inrange(enthover) || !entgroup.empty()))
		{
			loopv(entgroup) if(entities::ents.inrange(entgroup[i]) && entgroup[i] != enthover)
			{
				gameentity &e = *(gameentity *)entities::ents[entgroup[i]];
				drawentblip(w, h, s, blend, entgroup[i], e.o, e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], e.spawned, e.lastspawn, true);
			}
			if(entities::ents.inrange(enthover))
			{
				gameentity &e = *(gameentity *)entities::ents[enthover];
				drawentblip(w, h, s, blend, enthover, e.o, e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], e.spawned, e.lastspawn, true);
			}
		}
		else
		{
			loopv(entities::ents)
			{
				gameentity &e = *(gameentity *)entities::ents[i];
				drawentblip(w, h, s, blend, i, e.o, e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], e.spawned, e.lastspawn, false);
			}
			loopv(projs::projs) if(projs::projs[i]->projtype == PRJ_ENT && projs::projs[i]->ready())
			{
				projent &proj = *projs::projs[i];
				if(entities::ents.inrange(proj.id))
					drawentblip(w, h, s, blend, -1, proj.o, entities::ents[proj.id]->type, entities::ents[proj.id]->attr[0], entities::ents[proj.id]->attr[1], entities::ents[proj.id]->attr[2], entities::ents[proj.id]->attr[3], entities::ents[proj.id]->attr[4], true, proj.spawntime, false);
			}
		}
	}

	const struct borderpatch {
		int xw, xs, yh, ys, ww, ws, hh, hs; float tx, ty, tw, th;
	} borderpatches[8] =
	{
		{	0,	1,	0,	1,	0,	1,	0,	1,	0.f,	0.f,	0.2f,	0.2f	},
		{	1,	-2,	0,	1,	0,	1,	0,	1,	0.8f,	0.f,	0.2f,	0.2f	},
		{	0,	1,	1,	-2,	0,	1,	0,	1,	0.f,	0.8f,	0.2f,	0.2f	},
		{	1,	-2,	1,	-2,	0,	1,	0,	1,	0.8f,	0.8f,	0.2f,	0.2f	},
		{	0,	2,	0,	1,	1,	-4,	0,	1,	0.2f,	0.f,	0.6f,	0.2f	},
		{	0,	2,	1,	-2,	1,	-4,	0,	1,	0.2f,	0.8f,	0.6f,	0.2f	},
		{	0,	1,	0,	2,	0,	1,	1,	-4,	0.f,	0.2f,	0.2f,	0.6f	},
		{	1,	-2,	0,	2,	0,	1,	1,	-4,	0.8f,	0.2f,	0.2f,	0.6f	}
	};

	void drawborder(int w, int h, int s, float blend)
	{
		if(hastv(showborder) && world::player1->state != CS_DEAD) // damage overlay goes full in this case
		{
			float r = 1.f, g = 1.f, b = 1.f, fade = borderblend*blend;
			if(teamwidgets >= (borderhealth ? 3 : 1)) skewcolour(r, g, b);
			if(borderhealth) switch(world::player1->state)
			{
				case CS_ALIVE: healthskew(s, r, g, b, fade, borderskew, borderhealth > 1); break;
				default: break;
			}
			glColor4f(r, g, b, fade);
			settexture(radartex, 3);
			loopi(8)
			{
				const borderpatch &rd = borderpatches[i];
				drawtex(
					(rd.xw*w)+(rd.xs*s), (rd.yh*h)+(rd.ys*s),
					(rd.ww*w)+(rd.ws*s), (rd.hh*h)+(rd.hs*s),
					rd.tx, rd.ty, rd.tw, rd.th
				);
			}
		}
	}

	void drawradar(int w, int h, float blend)
	{
		int s = max(w, h)/2;
		if(hastv(radarcard) || m_edit(world::gamemode)) drawcardinalblips(w, h, s, blend*radarblend, m_edit(world::gamemode));
		if(hastv(radarplayers) || m_edit(world::gamemode))
		{
			loopv(world::players) if(world::players[i] && world::players[i]->state == CS_ALIVE)
				drawplayerblip(world::players[i], w, h, s, blend*radarblend);
		}
		if(hastv(radaritems) || m_edit(world::gamemode)) drawentblips(w, h, s, blend*radarblend);
		if(hastv(radarflags))
		{
			if(m_stf(world::gamemode)) stf::drawblips(w, h, s, blend);
			else if(m_ctf(world::gamemode)) ctf::drawblips(w, h, s, blend*radarblend);
		}
	}

	int drawitem(const char *tex, int x, int y, float size, float r, float g, float b, float fade, float skew, const char *font, const char *text, ...)
	{
		if(skew <= 0.f) return 0;
		float f = fade*inventoryblend*skew, cr = r*skew, cg = g*skew, cb = b*skew, s = size*skew;
		settexture(tex, 3);
		glColor4f(cr, cg, cb, f);
		drawsized(x-int(s), y-int(s), int(s));
		if(text && *text)
		{
			glPushMatrix();
			glScalef(skew, skew, 1);
			if(font && *font) pushfont(font);
			int tx = int(float(x)*(1.f/skew))-FONTW/4, ty = int(float(y-s)*(1.f/skew))+FONTH/4, ti = int(255.f*f);
			s_sprintfdlv(str, text, text);
			draw_textx("%s", tx, ty, 255, 255, 255, ti, TEXT_RIGHT_JUSTIFY, -1, -1, str);
			if(font && *font) popfont();
			glPopMatrix();
		}
		return int(s);
	}

	void drawitemsubtext(int x, int y, float skew, const char *font, float blend, const char *text, ...)
	{
		if(skew <= 0.f) return;
		glPushMatrix();
		glScalef(skew, skew, 1);
		if(font && *font) pushfont(font);
		int tx = int(float(x)*(1.f/skew))-FONTW/4, ty = int(float(y)*(1.f/skew))-FONTH-FONTH/4, ti = int(255.f*inventoryblend*blend*skew);
		s_sprintfdlv(str, text, text);
		draw_textx("%s", tx, ty, 255, 255, 255, ti, TEXT_RIGHT_JUSTIFY, -1, -1, str);
		if(font && *font) popfont();
		glPopMatrix();
	}

	const char *flagtex(int team)
	{
		const char *flagtexs[TEAM_MAX] = {
			neutralflagtex, alphaflagtex, betaflagtex, deltaflagtex, gammaflagtex, neutralflagtex
		};
		return flagtexs[team];
	}

	const char *itemtex(int type, int stype)
	{
		switch(type)
		{
			case WEAPON:
			{
				const char *weaptexs[WEAPON_MAX] = {
					plasmatex, shotguntex, chainguntex, flamertex, carbinetex, rifletex, grenadestex,
					paintguntex
				};
				if(isweap(stype)) return weaptexs[stype];
				break;
			}
			default: break;
		}
		return "";
	}

	int drawweapons(int x, int y, int s, float blend)
	{
		const char *hudtexs[WEAPON_MAX] = {
			plasmatex, shotguntex, chainguntex, flamertex, carbinetex, rifletex, grenadestex,
			paintguntex
		};
		int sy = 0, sweap = m_spawnweapon(world::gamemode, world::mutators);
		loopi(WEAPON_MAX) if(world::player1->hasweap(i, sweap) || lastmillis-world::player1->weaplast[i] < world::player1->weapwait[i])
		{
			float fade = blend, size = s, skew = 1.f;
			if(world::player1->weapstate[i] == WPSTATE_SWITCH || world::player1->weapstate[i] == WPSTATE_PICKUP)
			{
				float amt = clamp(float(lastmillis-world::player1->weaplast[i])/float(world::player1->weapwait[i]), 0.f, 1.f);
				skew = (i != world::player1->weapselect ?
					(
						world::player1->hasweap(i, sweap) ? 1.f-(amt*(0.3f)) : 1.f-amt
					) : (
						world::player1->weapstate[i] == WPSTATE_PICKUP ? amt : 0.7f+(amt*(0.3f))
					)
				);
			}
			else if(i != world::player1->weapselect) skew = 0.7f;
			bool instate = (i == world::player1->weapselect || world::player1->weapstate[i] != WPSTATE_PICKUP);
			int oldy = y-sy, delay = lastmillis-world::player1->lastspawn;
			if(delay < 1000) skew *= delay/1000.f;
			float r = 1.f, g = 1.f, b = 1.f;
			if(teamwidgets >= (inventorycolour ? 2 : 1)) skewcolour(r, g, b);
			else if(inventorycolour)
			{
				r = (weaptype[i].colour>>16)/255.f;
				g = ((weaptype[i].colour>>8)&0xFF)/255.f;
				b = (weaptype[i].colour&0xFF)/255.f;
			}
			if(inventoryammo && (instate || inventoryammo > 1) && world::player1->hasweap(i, sweap))
				sy += drawitem(hudtexs[i], x, y-sy, size, r, g, b, fade, skew, "default", "%d", world::player1->ammo[i]);
			else sy += drawitem(hudtexs[i], x, y-sy, size, r, g, b, fade, skew);
            if(inventoryweapids && (instate || inventoryweapids > 1))
            {
				static string weapids[WEAPON_MAX];
				static int lastweapids = -1;
				if(lastweapids != changedkeys)
				{
					loopj(WEAPON_MAX)
					{
						s_sprintfd(action)("weapon %d", j);
						const char *actkey = searchbind(action, 0);
						if(actkey && *actkey) s_strcpy(weapids[j], actkey);
						else s_sprintf(weapids[j])("%d", j+1);
					}
					lastweapids = changedkeys;
				}
                if(inventoryweapents && entities::ents.inrange(world::player1->entid[i]) && world::player1->hasweap(i, sweap))
					drawitemsubtext(x, oldy, skew, "sub", fade, "[\fs\fa%d\fS] \fs%s%s\fS", world::player1->entid[i], inventorycolour >= 2 ? weaptype[i].text : "\fa", weapids[i]);
                else drawitemsubtext(x, oldy, skew, "sub", fade, "\fs%s%s\fS", inventorycolour >= 2 ? weaptype[i].text : "\fa", weapids[i]);
            }
		}
		return sy;
	}

	int drawhealth(int x, int y, int s, float blend)
	{
		float fade = inventoryblend*blend, skew = 1.f;
		if(world::player1->state == CS_ALIVE)
		{
			int delay = lastmillis-world::player1->lastspawn;
			if(delay < 1000) skew *= delay/1000.f;
			if(inventorythrob && regentime && world::player1->lastregen && lastmillis-world::player1->lastregen < regentime*1000)
			{
				float amt = clamp((lastmillis-world::player1->lastregen)/float(regentime*1000), 0.f, 1.f);
				if(amt < 0.5f) amt = 1.f-amt;
				fade *= amt;
				if(inventorythrob > 1) skew *= amt;
			}
			if(skew <= 0.f) return 0;
		}
		else if(world::player1->lastdeath && (world::player1->state == CS_DEAD || world::player1->state == CS_WAITING))
		{
			int delay = lastmillis-world::player1->lastdeath;
			if(delay < 1000)
			{
				skew *= 1.f-(delay/1000.f);
				if(skew <= 0.f) return 0;
			}
			else return 0;
		}
		else return 0;

        const struct healthbarstep
        {
            float health, r, g, b;
        } steps[] = { { 0, 0.5f, 0, 0 }, { 0.25f, 1, 0, 0 }, { 0.75f, 1, 0.5f, 0 }, { 1, 0, 1, 0 } };
        settexture(healthtex, 3);
        glBegin(GL_QUAD_STRIP);
        int size = int(1.5f*s*skew);
        float health = clamp(world::player1->health/float(m_maxhealth(world::gamemode, world::mutators)), 0.0f, 1.0f);
        const float margin = 0.1f;
        loopi(4)
        {
            const healthbarstep &step = steps[i];
            if(i > 0)
            {
				if(step.health > health && steps[i-1].health <= health)
				{
					float hoff = 1 - health, hlerp = (health - steps[i-1].health) / (step.health - steps[i-1].health),
						  r = step.r*hlerp + steps[i-1].r*(1-hlerp),
						  g = step.g*hlerp + steps[i-1].g*(1-hlerp),
						  b = step.b*hlerp + steps[i-1].b*(1-hlerp);
					glColor4f(r, g, b, fade); glTexCoord2f(0, hoff); glVertex2f(x - size/2, y - size + hoff*size);
					glColor4f(r, g, b, fade); glTexCoord2f(1, hoff); glVertex2f(x, y - size + hoff*size);
				}
				if(step.health > health + margin)
				{
					float hoff = 1 - (health + margin), hlerp = (health + margin - steps[i-1].health) / (step.health - steps[i-1].health),
						  r = step.r*hlerp + steps[i-1].r*(1-hlerp),
						  g = step.g*hlerp + steps[i-1].g*(1-hlerp),
						  b = step.b*hlerp + steps[i-1].b*(1-hlerp);
					glColor4f(r, g, b, 0); glTexCoord2f(0, hoff); glVertex2f(x - size/2, y - size + hoff*size);
					glColor4f(r, g, b, 0); glTexCoord2f(1, hoff); glVertex2f(x, y - size + hoff*size);
					break;
				}
            }
            float off = 1 - step.health, hfade = fade, r = step.r, g = step.g, b = step.b;
            if(step.health > health) hfade *= 1 - (step.health - health)/margin;
            glColor4f(r, g, b, hfade); glTexCoord2f(0, off); glVertex2f(x - size/2, y - size + off*size);
            glColor4f(r, g, b, hfade); glTexCoord2f(1, off); glVertex2f(x, y - size + off*size);
        }
        glEnd();
		if(inventoryhealth > 1) drawitemsubtext(x, y, skew, "sub", blend, "%d", max(world::player1->health, 0));
		return size;
	}

	void drawinventory(int w, int h, int edge, float blend)
	{
		int cx = int(w-edge*1.5f), cy = int(h-edge*1.5f), cs = int(inventorysize*w),
			cr = cs/4, cc = 0, oldpos = cy;
		if(inventoryhealth && (cc = drawhealth(cx, cy, cs, blend)) > 0) cy -= cc+cr;
		if((inventoryammo || inventoryweapids) && world::player1->state == CS_ALIVE && (cc = drawweapons(cx, cy, cs, blend)) > 0) cy -= cc+cr;
		if(m_ctf(world::gamemode) && ((cc = ctf::drawinventory(cx, cy, cs, blend)) > 0)) cy -= cc+cr;
		if(m_stf(world::gamemode) && ((cc = stf::drawinventory(cx, cy, cs, blend)) > 0)) cy -= cc+cr;
		hasinventory = oldpos != cy;
	}

	void drawdamage(int w, int h, int s, float blend)
	{
		float pc = 0.f;
		if((world::player1->state == CS_ALIVE && hud::damageresidue > 0) || world::player1->state == CS_DEAD)
			pc = float(world::player1->state == CS_DEAD ? 100 : min(hud::damageresidue, 100))/100.f;

		if(showdamage > 1 && world::player1->state == CS_ALIVE && regentime && world::player1->lastregen && lastmillis-world::player1->lastregen < regentime*1000)
		{
			float skew = clamp((lastmillis-world::player1->lastregen)/float(regentime*1000), 0.f, 1.f);
			if(skew > 0.5f) skew = 1.f-skew;
			pc += (1.f-pc)*skew;
		}

		if(pc > 0.f)
		{
			Texture *t = textureload(damagetex, 3);
			glBindTexture(GL_TEXTURE_2D, t->id);
			glColor4f(1.f, 1.f, 1.f, pc*blend*damageblend);
			drawtex(0, 0, w, h);
		}
	}

    struct damagecompassdir
    {
        float damage;
        vec color;

        damagecompassdir() : damage(0), color(1, 0, 0) {}
    } damagecompassdirs[8];

	void damagecompass(int n, const vec &loc, gameent *actor, int weap)
	{
		if(!showdamagecompass) return;
		vec delta = vec(loc).sub(camera1->o).normalize();
		float yaw, pitch;
		vectoyawpitch(delta, yaw, pitch);
		yaw -= camera1->yaw;
		while(yaw < 0.0f) yaw += 360.0f;
		while(yaw >= 360.0f) yaw -= 360.0f;
		int d = clamp(((int)floor((yaw+22.5f)/45.0f))&7, 0, 7);
        damagecompassdir &dir = damagecompassdirs[d];
        dir.damage += max(n, damagecompassmin)/float(damagecompassmax);
        if(dir.damage > 1) dir.damage = 1;
        if(weap == WEAPON_PAINT)
        {
            int col = paintcolours[actor->type == ENT_PLAYER && m_team(world::gamemode, world::mutators) ? actor->team : rnd(10)];
            dir.color = vec((col>>16)&0xFF, (col>>8)&0xFF, col&0xFF).div(0xFF);
        }
        else if(kidmode || world::noblood) dir.color = vec(1, 0.25f, 1);
        else dir.color = vec(0.75f, 0, 0);
	}

	void drawdamagecompass(int w, int h, int s, float blend)
	{
		int dirs = 0;
		float size = damagecompasssize*min(h, w)/2.0f;
		loopi(8) if(damagecompassdirs[i].damage > 0)
		{
			if(!dirs)
			{
                usetexturing(false);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			}
			dirs++;

            damagecompassdir &dir = damagecompassdirs[i];

			glPushMatrix();
			glTranslatef(w/2, h/2, 0);
			glRotatef(i*45, 0, 0, 1);
			glTranslatef(0, -size/2.0f-min(h, w)/4.0f, 0);
			float logscale = 32, scale = log(1 + (logscale - 1)*dir.damage) / log(logscale);
			glScalef(size*scale, size*scale, 0);

            glColor4f(dir.color.x, dir.color.y, dir.color.z, damagecompassblend*blend);

			glBegin(GL_TRIANGLES);
			glVertex3f(1, 1, 0);
			glVertex3f(-1, 1, 0);
			glVertex3f(0, 0, 0);
			glEnd();
			glPopMatrix();

			// fade in log space so short blips don't disappear too quickly
			scale -= float(curtime)/damagecompassfade;
			dir.damage = scale > 0 ? (pow(logscale, scale) - 1) / (logscale - 1) : 0;
		}
        if(dirs) usetexturing(true);
	}

	void drawsniper(int w, int h, int s, float blend)
	{
		Texture *t = textureload(snipetex, 3);
		int frame = lastmillis-world::lastzoom;
		float pc = frame < world::zoomtime() ? float(frame)/float(world::zoomtime()) : 1.f;
		if(!world::zooming) pc = 1.f-pc;
		glBindTexture(GL_TEXTURE_2D, t->id);
		glColor4f(1.f, 1.f, 1.f, pc*blend);
		drawtex(0, 0, w, h);
	}

	void drawgamehud(int w, int h)
	{
		float fade = hudblend;
		int ox = hudwidth, oy = hudsize, os = int(oy*bordersize*(hastv(showborder) ? 1.f : 0.5f)),
			secs = world::maptime ? lastmillis-world::maptime : 0;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);

		if(secs < titlecard)
		{
			float amt = clamp(float(secs)/float(titlecard), 0.f, 1.f);
			fade *= amt;
		}

		if(world::player1->state == CS_ALIVE && world::inzoom() && weaptype[world::player1->weapselect].snipes)
			drawsniper(ox, oy, os, fade);
		if(hastv(showborder)) drawborder(ox, oy, os, fade);
		if(showdamage && !kidmode && !world::noblood) drawdamage(ox, oy, os, fade);
        if(showdamagecompass) drawdamagecompass(ox, oy, os, fade);
        bool hasradar = world::player1->state == CS_EDITING ? showeditradar > 0 : hastv(showradar);
		if(hasradar) drawradar(ox, oy, fade);
		if(showinventory) drawinventory(ox, oy, os, fade);
		else hasinventory = false;
	}

	void drawhudelements(int w, int h)
	{
		int ox = hudwidth, oy = hudsize, os = int(oy*bordersize*(hastv(showborder) ? 1.f : 0.5f)*1.5f),
			is = hasinventory ? int(oy*inventorysize) : 0, bx = os+FONTW/4, by = oy-os-(FONTH/3)*2, bs = ox-bx*2-is;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);
		pushfont("hud");

		renderconsole(ox, oy, bx, os, bs);

		static int laststats = 0, prevstats[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, curstats[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		if(totalmillis-laststats >= statrate)
		{
			memcpy(prevstats, curstats, sizeof(prevstats));
			laststats = totalmillis-(totalmillis%statrate);
		}
		int nextstats[12] =
		{
			vtris*100/max(wtris, 1),
			vverts*100/max(wverts, 1),
			xtraverts/1024,
			xtravertsva/1024,
			glde,
			gbatches,
			getnumqueries(),
			rplanes,
			curfps,
			bestfpsdiff,
			worstfpsdiff,
			autoadjustlevel
		};
		loopi(12) if(prevstats[i] == curstats[i]) curstats[i] = nextstats[i];
		if(showstats > (m_edit(world::gamemode) ? 0 : 1))
		{
			by -= draw_textx("ond:%d va:%d gl:%d(%d) oq:%d lm:%d rp:%d pvs:%d", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6], lightmaps.length(), curstats[7], getnumviewcells());
			by -= draw_textx("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
		}
		if(showfps) switch(showfps)
		{
			case 3:
				if(autoadjust) by -= draw_textx("fps:%d (%d/%d) +%d-%d [\fs%s%d%%\fS]", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs, curstats[8], minfps, maxfps, curstats[9], curstats[10], curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
				else by -= draw_textx("fps:%d (%d) +%d-%d", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs, curstats[8], maxfps, curstats[9], curstats[10]);
				break;
			case 2:
				if(autoadjust) by -= draw_textx("fps:%d (%d/%d) [\fs%s%d%%\fS]", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs, curstats[8], minfps, maxfps, curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
				else by -= draw_textx("fps:%d (%d)", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs, curstats[8], maxfps);
				break;
            case 1:
                by -= draw_textx("fps:%d", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs, curstats[8]);
                break;
			default: break;
		}
		if(connected() && client::ready() && world::maptime)
		{
			if(world::player1->state == CS_EDITING)
			{
				by -= draw_textx("pos:%d,%d,%d yaw:%d pitch:%d", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs,
						(int)world::player1->o.x, (int)world::player1->o.y, (int)world::player1->o.z,
						(int)world::player1->yaw, (int)world::player1->pitch);
				by -= draw_textx("sel:%d,%d,%d %d,%d,%d (%d,%d,%d,%d)", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs,
						sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z,
							sel.cx, sel.cxs, sel.cy, sel.cys);
				by -= draw_textx("corner:%d orient:%d grid:%d", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs,
								sel.corner, sel.orient, sel.grid);
				by -= draw_textx("cube:%s%d ents:%d[%d]", bx, by, 255, 255, 255, int(255*hudblend), TEXT_LEFT_JUSTIFY, -1, bs,
					selchildcount<0 ? "1/" : "", abs(selchildcount), entities::ents.length(), entgroup.length());
			}
		}
		if(getcurcommand()) by -= rendercommand(bx, by, bs);
		popfont(); // emphasis
	}

	void drawhud(int w, int h)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if(world::maptime && connected() && client::ready()) drawgamehud(w, h);
		drawhudelements(w, h);
		glDisable(GL_BLEND);
	}

	bool getcolour(vec &colour)
	{
		if(!world::maptime || lastmillis-world::maptime < titlecard)
		{
			float fade = world::maptime ? float(lastmillis-world::maptime)/float(titlecard) : 0.f;
			if(fade < 1.f)
			{
				colour = vec(fade, fade, fade);
				return true;
			}
		}
		if(world::tvmode())
		{
			float fade = 1.f;
			int millis = world::spectvtime ? min(world::spectvtime/10, 500) : 500, interval = lastmillis-world::lastspec;
			if(!world::lastspec || interval < millis)
				fade = world::lastspec ? float(interval)/float(millis) : 0.f;
			else if(world::spectvtime && interval > world::spectvtime-millis)
				fade = float(world::spectvtime-interval)/float(millis);
			if(fade < 1.f)
			{
				colour = vec(fade, fade, fade);
				return true;
			}
		}
		return false;
	}
	void gamemenus() { sb.show(); }
}
