#include "game.h"
namespace hud
{
	const int NUMSTATS = 12;
	int damageresidue = 0, hudwidth = 0, laststats = 0, prevstats[NUMSTATS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, curstats[NUMSTATS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	vector<int> teamkills;
	scoreboard sb;

	VARP(hudsize, 0, 2048, INT_MAX-1);
	FVARP(gapsize, 0, 0.01f, 1000);

	VARP(showconsole, 0, 2, 2);
	VARP(shownotices, 0, 4, 4);

	VARP(showstats, 0, 1, 2);
	VARP(statrate, 0, 200, 1000);
	FVARP(statblend, 0, 0.75f, 1);

	VARP(showfps, 0, 1, 3);

	bool fullconsole = false;
	void toggleconsole() { fullconsole = !fullconsole; }
	COMMAND(toggleconsole, "");

	int conskip = 0;
	void setconskip(int *n)
	{
		conskip += *n;
		if(conskip<0) conskip = 0;
	}
	COMMANDN(conskip, setconskip, "i");

	VARP(consize, 0, 5, 100);
	VARP(contime, 0, 15000, INT_MAX-1);
	FVARP(conblend, 0, 0.85f, 1);
	VARP(chatconsize, 0, 5, 100);
	VARP(chatcontime, 0, 30000, INT_MAX-1);
	FVARP(chatconblend, 0, 0.85f, 1);
	FVARP(fullconblend, 0, 1.f, 1);
	VARP(fullconsize, 0, 15, 100);

	FVARP(commandoffset, 0.f, 0.35f, 1.f);

	FVARP(noticeoffset, 0.f, 0.33f, 1.f);
	FVARP(noticeblend, 0.f, 0.75f, 1.f);
	VARP(obitnotices, 0, 2, 2);
	VARP(obitnoticetime, 0, 5000, INT_MAX-1);
	TVAR(inputtex, "textures/menu", 3);

	VARP(teamwidgets, 0, 1, 3); // colour based on team
	VARP(teamclips, 0, 1, 2);
	VARP(teamcrosshair, 0, 1, 2);
	VARP(teamnotices, 0, 0, 1);
	VARP(teamkillnum, 0, 5, INT_MAX-1);
	VARP(teamkilltime, 0, 30, INT_MAX-1);

	TVAR(underlaytex, "", 3);
	VARP(underlaydisplay, 0, 0, 2); // 0 = only firstperson and alive, 1 = only when alive, 2 = always
	FVARP(underlayblend, 0, 0.5f, 1);
	TVAR(overlaytex, "", 3);
	VARP(overlaydisplay, 0, 0, 2); // 0 = only firstperson and alive, 1 = only when alive, 2 = always
	FVARP(overlayblend, 0, 0.5f, 1);

	VARP(titlecard, 0, 5000, 10000);
	VARP(showdamage, 0, 1, 2); // 1 shows just damage, 2 includes regen
	TVAR(damagetex, "textures/damage", 3);
	FVARP(damageblend, 0, 0.75f, 1);

	VARP(showdamagecompass, 0, 1, 1);
	VARP(damagecompassfade, 1, 1000, 10000);
	FVARP(damagecompasssize, 0, 0.25f, 1000);
	FVARP(damagecompassblend, 0, 0.5f, 1);
	VARP(damagecompassmin, 1, 25, 1000);
	VARP(damagecompassmax, 1, 200, 1000);

	VARP(showindicator, 0, 1, 1);
	FVARP(indicatorsize, 0, 0.021f, 1000);
	FVARP(indicatorblend, 0, 0.75f, 1);
	VARP(teamindicator, 0, 1, 1);
	FVARP(teamindicatorsize, 0, 0.0575f, 1000);
	FVARP(teamindicatorblend, 0, 0.5f, 1);
	TVAR(indicatortex, "textures/indicator", 3);
	TVAR(snipetex, "textures/snipe", 3);

	VARP(showcrosshair, 0, 1, 1);
	FVARP(crosshairsize, 0, 0.05f, 1000);
	VARP(crosshairhitspeed, 0, 500, INT_MAX-1);
	FVARP(crosshairblend, 0, 0.75f, 1);
	VARP(crosshairhealth, 0, 2, 2);
	FVARP(crosshairskew, -1, 0.3f, 1);
	TVAR(relativecursortex, "textures/dotcrosshair", 3);
	TVAR(guicursortex, "textures/cursor", 3);
	TVAR(editcursortex, "textures/dotcrosshair", 3);
	TVAR(speccursortex, "textures/dotcrosshair", 3);
	TVAR(crosshairtex, "textures/crosshair", 3);
	TVAR(teamcrosshairtex, "textures/teamcrosshair", 3);
	TVAR(hitcrosshairtex, "textures/hitcrosshair", 3);
	TVAR(snipecrosshairtex, "textures/snipecrosshair", 3);
	FVARP(snipecrosshairsize, 0, 0.575f, 1000);
	FVARP(cursorsize, 0, 0.05f, 1000);
	FVARP(cursorblend, 0, 1.f, 1);

	VARP(showinventory, 0, 1, 1);
	VARP(inventoryammo, 0, 1, 2);
	VARP(inventoryedit, 0, 1, 1);
	VARP(inventoryscores, 0, 1, 1);
	VARP(inventoryweapids, 0, 1, 2);
	VARP(inventoryweapents, 0, 0, 1);
	VARP(inventoryhealth, 0, 2, 2);
	VARP(inventorythrob, 0, 1, 1);
	VARP(inventorycolour, 0, 1, 2);
	FVARP(inventorysize, 0, 0.05f, 1000);
	FVARP(inventoryblend, 0, 0.5f, 1);
	FVARP(inventoryglow, 0, 0.055f, 1);
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
	TVAR(inventoryenttex, "textures/progress", 3);
	TVAR(inventoryedittex, "textures/progress", 3);
	TVAR(inventorywaittex, "textures/wait", 3);
	TVAR(inventorydeadtex, "textures/exit", 3);
	TVAR(inventorychattex, "textures/conopen", 3);

	VARP(showclip, 0, 1, 1);
	FVARP(clipsize, 0, 0.05f, 1000);
	FVARP(clipblend, 0, 0.5f, 1000);
	FVARP(clipcolour, 0.f, 1.f, 1.f);
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
	FVARP(radarcardblend, 0, 0.75f, 1);
	FVARP(radarplayerblend, 0, 0.5f, 1);
	FVARP(radarblipblend, 0, 0.75f, 1);
	FVARP(radarflagblend, 0, 1.f, 1);
	FVARP(radaritemblend, 0, 0.75f, 1);
	FVARP(radarsize, 0, 0.025f, 1000);
	FVARP(radaroffset, 0, 0.075f, 1000);
	VARP(radardist, 0, 256, INT_MAX-1);
	VARP(radarcard, 0, 0, 2);
	VARP(radaritems, 0, 2, 2);
	VARP(radaritemspawn, 0, 1, 1);
	VARP(radaritemtime, 0, 3000, INT_MAX-1);
	VARP(radaritemnames, 0, 0, 2);
	VARP(radarplayers, 0, 1, 2);
	VARP(radarplayernames, 0, 0, 2);
	VARP(radarflags, 0, 2, 2);
	VARP(radarflagnames, 0, 0, 1);
	VARP(showeditradar, 0, 1, 1);
	VARP(editradarcard, 0, 0, 1);
	VARP(editradardist, 0, 32, INT_MAX-1);
	VARP(editradarnoisy, 0, 1, 2);

	bool hastv(int val)
	{
		if(val == 2 || (val && game::tvmode())) return true;
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

	void drawblend(int x, int y, int w, int h, float r, float g, float b)
	{
        glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		glColor3f(r, g, b);
		glBegin(GL_QUADS);
		glVertex2f(x, y); glVertex2f(w, y);
		glVertex2f(w, h); glVertex2f(x, h);
		glEnd();
        glDisable(GL_BLEND);
	}

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
		if(throb && regentime && game::player1->lastregen && lastmillis-game::player1->lastregen < regentime*1000)
		{
			float skew = clamp((lastmillis-game::player1->lastregen)/float(regentime*1000), 0.f, 1.f);
			if(skew > 0.5f) skew = 1.f-skew;
			fade *= 1.f-skew;
			s += int(s*ss*skew);
		}
		int total = m_maxhealth(game::gamemode, game::mutators);
		bool full = game::player1->health >= total;
		if(full)
		{
			int over = total;
			if(m_ctf(game::gamemode)) over = max(overctfhealth, total);
			if(m_stf(game::gamemode)) over = max(overstfhealth, total);
			if(over > total)
				colourskew(r, g, b, 1.f+clamp(float(game::player1->health-total)/float(over-total), 0.f, 1.f));
		}
		else colourskew(r, g, b, clamp(float(game::player1->health)/float(total), 0.f, 1.f));
	}

	void skewcolour(float &r, float &g, float &b, bool t)
	{
		int team = game::player1->team;
		r *= (teamtype[team].colour>>16)/255.f;
		g *= ((teamtype[team].colour>>8)&0xFF)/255.f;
		b *= (teamtype[team].colour&0xFF)/255.f;
		if(!team && t)
		{
			float f = game::player1->state == CS_SPECTATOR || game::player1->state == CS_EDITING ? 0.25f : 0.375f;
			r *= f;
			g *= f;
			b *= f;
		}
	}

	void skewcolour(int &r, int &g, int &b, bool t)
	{
		int team = game::player1->team;
		r = int(r*((teamtype[team].colour>>16)/255.f));
		g = int(g*(((teamtype[team].colour>>8)&0xFF)/255.f));
		b = int(b*((teamtype[team].colour&0xFF)/255.f));
		if(!team && t)
		{
			float f = game::player1->state == CS_SPECTATOR || game::player1->state == CS_EDITING ? 0.25f : 0.375f;
			r = int(r*f);
			g = int(g*f);
			b = int(b*f);
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
            case POINTER_GUI:
            {
            	if(commandmillis >= 0) return commandicon ? commandicon : inputtex;
            	else return guicursortex;
            	break;
            }
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
		int millis = lastmillis-game::player1->weaplast[weap];
		float r = 1.f, g = 1.f, b = 1.f, amt = 0.f;
		switch(game::player1->weapstate[weap])
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
		glColor4f(r, g, b, indicatorblend*hudblend);
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
        int ammo = game::player1->ammo[weap], maxammo = weaptype[weap].max;
		if(t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);

		float fade = clipblend*hudblend;
		if(lastmillis-game::player1->weaplast[weap] < game::player1->weapwait[weap]) switch(game::player1->weapstate[weap])
		{
			case WPSTATE_RELOAD: case WPSTATE_PICKUP: case WPSTATE_SWITCH:
			{
				fade *= clamp(float(lastmillis-game::player1->weaplast[weap])/float(game::player1->weapwait[weap]), 0.f, 1.f);
				break;
			}
			default: break;
		}
        switch(weap)
        {
            case WEAPON_PLASMA: case WEAPON_FLAMER: case WEAPON_CG: s *= 0.8f; break;
            default: break;
        }
        float r = clipcolour, g = clipcolour, b = clipcolour;
		if(teamclips >= (clipcolour > 0 ? 2 : 1)) skewcolour(r, g, b);
		else if(clipcolour > 0)
		{
			r = ((weaptype[weap].colour>>16)/255.f)*clipcolour;
			g = (((weaptype[weap].colour>>8)&0xFF)/255.f)*clipcolour;
			b = ((weaptype[weap].colour&0xFF)/255.f)*clipcolour;
		}
        glColor4f(r, g, b, fade);
        glBindTexture(GL_TEXTURE_2D, t->retframe(ammo, maxammo));
        if(t->frames.length() > 1) drawsized(x-s/2, y-s/2, s);
        else switch(weap)
        {
            case WEAPON_FLAMER:
				drawslice(0, max(ammo-min(maxammo-ammo, 2), 0)/float(maxammo), x, y, s);
				if(game::player1->ammo[weap] < weaptype[weap].max)
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
		bool guicursor = index == POINTER_GUI && commandmillis < 0;
		drawsized(guicursor ? x : x-s/2, guicursor ? y : y-s/2, s);
	}

	void drawpointer(int w, int h, int index)
	{
		bool guicursor = index == POINTER_GUI;
		int cs = int((guicursor ? cursorsize : crosshairsize)*hudsize);
		float r = 1.f, g = 1.f, b = 1.f, fade = (guicursor ? cursorblend : crosshairblend)*hudblend;
		if(guicursor && commandmillis >= 0)
		{
			fade = float(lastmillis%1000)/1000.f;
			if(fade < 0.5f) fade = 1.f-fade;
			cs = int(max(crosshairsize, cursorsize)*hudsize);
		}
		else if(teamcrosshair >= (crosshairhealth ? 2 : 1)) skewcolour(r, g, b);
		if(game::player1->state == CS_ALIVE && index >= POINTER_HAIR)
		{
			if(index == POINTER_SNIPE)
			{
				cs = int(snipecrosshairsize*hudsize);
				if(game::inzoom() && weaptype[game::player1->weapselect].snipes)
				{
					int frame = lastmillis-game::lastzoom;
					float amt = frame < game::zoomtime() ? clamp(float(frame)/float(game::zoomtime()), 0.f, 1.f) : 1.f;
					if(!game::zooming) amt = 1.f-amt;
					cs = int(cs*amt);
				}
			}
			if(crosshairhealth) healthskew(cs, r, g, b, fade, crosshairskew, crosshairhealth > 1);
		}
		int cx = int(hudwidth*cursorx), cy = int(hudsize*cursory), nx = int(hudwidth*0.5f), ny = int(hudsize*0.5f);
		if(index > POINTER_GUI && teamindicator && game::player1->team)
		{
			Texture *t = textureload(indicatortex, 3);
			if(t != notexture)
			{
				if(t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				else glBlendFunc(GL_ONE, GL_ONE);
				float tr = 1.f, tg = 1.f, tb = 1.f;
				skewcolour(tr, tg, tb);
				glColor4f(tr, tg, tb, teamindicatorblend*hudblend);
				glBindTexture(GL_TEXTURE_2D, t->id);
				int size = int(teamindicatorsize*hudsize);
				drawsized(nx-size/2, ny-size/2, size);
			}
		}
		drawpointerindex(index, game::mousestyle() != 1 ? cx : nx, game::mousestyle() != 1 ? cy : ny, cs, r, g, b, fade);
		if(index > POINTER_GUI)
		{
			if(game::player1->state == CS_ALIVE && game::player1->hasweap(game::player1->weapselect, m_spawnweapon(game::gamemode, game::mutators)))
			{
				if(showclip) drawclip(game::player1->weapselect, nx, ny, clipsize*hudsize);
				if(showindicator && weaptype[game::player1->weapselect].power && game::player1->weapstate[game::player1->weapselect] == WPSTATE_POWER)
					drawindicator(game::player1->weapselect, nx, ny, int(indicatorsize*hudsize));
			}
			if(game::mousestyle() >= 1) // renders differently
				drawpointerindex(POINTER_RELATIVE, game::mousestyle() != 1 ? nx : cx, game::mousestyle() != 1 ? ny : cy, int(crosshairsize*hudsize), 1.f, 1.f, 1.f, crosshairblend*hudblend);
		}
	}

	void drawpointers(int w, int h)
	{
        int index = POINTER_NONE;
		if(UI::hascursor()) index = UI::hascursor(true) ? POINTER_GUI : POINTER_NONE;
        else if(!showcrosshair || game::player1->state == CS_DEAD || !connected() || !client::ready()) index = POINTER_NONE;
        else if(game::player1->state == CS_EDITING) index = POINTER_EDIT;
        else if(game::player1->state == CS_SPECTATOR || game::player1->state == CS_WAITING) index = POINTER_SPEC;
        else if(game::inzoom() && weaptype[game::player1->weapselect].snipes) index = POINTER_SNIPE;
        else if(lastmillis-game::player1->lasthit <= crosshairhitspeed) index = POINTER_HIT;
        else if(m_team(game::gamemode, game::mutators))
        {
            vec pos = game::player1->headpos();
            gameent *d = game::intersectclosest(pos, worldpos, game::player1);
            if(d && d->type == ENT_PLAYER && d->team == game::player1->team)
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
		pushfont("super");
		int ty = (hudsize/2)-FONTH, tx = hudwidth/2, tf = int(255*hudblend), tr = 255, tg = 255, tb = 255;
		if(commandmillis >= 0)
			ty += draw_textx(commandbuf, tx-FONTW/2, ty+int(hudsize/2*commandoffset), 255, 255, 255, tf, TEXT_CENTERED, commandpos >= 0 ? commandpos : strlen(commandbuf), hudwidth/3*2);
		else if(shownotices && game::maptime && !UI::hascursor(false) && !texpaneltimer)
		{
			ty += int(hudsize/2*noticeoffset);
			tf = int(tf*noticeblend);
			if(teamnotices) skewcolour(tr, tg, tb);
			if(lastmillis-game::maptime <= titlecard)
			{
				const char *title = getmaptitle();
				if(!*title) title = getmapname();
				ty += draw_textx("%s", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, -1, title);
				pushfont("emphasis");
				ty += draw_textx("[ %s ]", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, -1, server::gamename(game::gamemode, game::mutators));
				popfont();
			}

			if(m_ctf(game::gamemode)) ctf::drawlast(w, h, tx, ty, tf/255.f);
			else if(m_stf(game::gamemode)) stf::drawlast(w, h, tx, ty, tf/255.f);

			if(game::player1->state == CS_DEAD || game::player1->state == CS_WAITING)
			{
				int sdelay = m_spawndelay(game::gamemode, game::mutators), delay = game::player1->lastdeath ? game::player1->respawnwait(lastmillis, sdelay) : 0;
				const char *msg = game::player1->state != CS_WAITING && game::player1->lastdeath ? (m_paint(game::gamemode, game::mutators) ? "Tagged!" : "Fragged!") : "Please Wait";
				ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, msg);
				if(obitnotices && game::player1->lastdeath && *game::player1->obit)
				{
					pushfont("default");
					ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, game::player1->obit);
					popfont();
				}
				if(shownotices >= 2)
				{
					SEARCHBINDCACHE(attackkey)("attack", 0);
					if(delay || m_duke(game::gamemode, game::mutators))
					{
						pushfont("emphasis");
						if(m_duke(game::gamemode, game::mutators) || !game::player1->lastdeath)
							ty += draw_textx("Waiting for new round", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1);
						else if(delay) ty += draw_textx("Down for \fs\fy%.1f\fS second(s)", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, delay/1000.f);
						popfont();
						if(game::player1->state != CS_WAITING && shownotices >= 3 && lastmillis-game::player1->lastdeath > 500)
						{
							pushfont("default");
							ty += draw_textx("Press \fs\fc%s\fS to look around", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, attackkey);
							popfont();
						}
					}
					else
					{
						pushfont("emphasis");
						ty += draw_textx("Ready to respawn", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1);
						popfont();
						if(game::player1->state != CS_WAITING && shownotices >= 3)
						{
							pushfont("default");
							ty += draw_textx("Press \fs\fc%s\fS to respawn", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, attackkey);
							popfont();
						}
					}
				}
			}
			else if(game::player1->state == CS_ALIVE)
			{
				if(teamkillnum && m_team(game::gamemode, game::mutators) && numteamkills() >= teamkillnum)
				{
					ty += draw_textx("%sDon't shoot team mates!", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, lastmillis%500 >= 250 ? "\fo" : "\fy");
					if(shownotices >= 2)
					{
						pushfont("emphasis");
						ty += draw_textx("You are on team \fs%s%s\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, teamtype[game::player1->team].chat, teamtype[game::player1->team].name);
						popfont();
						pushfont("default");
						ty += draw_textx("Shoot anyone not the \fs%ssame colour\fS!", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, teamtype[game::player1->team].chat);
						popfont();
					}
				}
				if(obitnotices && lastmillis-game::player1->lastkill <= obitnoticetime && *game::player1->obit)
				{
					pushfont("default");
					ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, game::player1->obit);
					popfont();
				}
				if(shownotices >= 3 && game::allowmove(game::player1))
				{
					pushfont("default");
					if(game::player1->requse < 0)
					{
						static vector<actitem> actitems;
						actitems.setsizenodelete(0);
						if(entities::collateitems(game::player1, actitems))
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
										int drop = -1, sweap = m_spawnweapon(game::gamemode, game::mutators), attr = e.type == WEAPON ? weapattr(e.attr[0], sweap) : e.attr[0];
										if(game::player1->canuse(e.type, attr, e.attr[1], e.attr[2], e.attr[3], e.attr[4], sweap, lastmillis))
										{
											if(e.type == WEAPON && weapcarry(game::player1->weapselect, sweap) && game::player1->ammo[attr] < 0 &&
												weapcarry(attr, sweap) && game::player1->carry(sweap) >= maxcarry) drop = game::player1->drop(sweap, attr);
											if(isweap(drop))
											{
												defformatstring(dropweap)("%s", entities::entinfo(WEAPON, drop, 0, 0, 0, 0, false));
												ty += draw_textx("Press \fs\fc%s\fS to swap \fs%s\fS for \fs%s\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, actionkey, dropweap, entities::entinfo(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], false));
											}
											else ty += draw_textx("Press \fs\fc%s\fS to pickup \fs%s\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, actionkey, entities::entinfo(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], false));
											break;
										}
									}
									else if(e.type == TRIGGER && e.attr[2] == TA_ACT)
									{
										ty += draw_textx("Press \fs\fc%s\fS to interact", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, actionkey);
										break;
									}
								}
								actitems.pop();
							}
						}
					}
					if(shownotices >= 4)
					{
						if(game::player1->hasweap(game::player1->weapselect, m_spawnweapon(game::gamemode, game::mutators)))
						{
							SEARCHBINDCACHE(zoomkey)("zoom", 0);
							ty += draw_textx("Press \fs\fc%s\fS to %s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, zoomkey, weaptype[game::player1->weapselect].snipes ? "zoom" : "prone");
						}
						if(game::player1->canshoot(game::player1->weapselect, m_spawnweapon(game::gamemode, game::mutators), lastmillis))
						{
							SEARCHBINDCACHE(attackkey)("attack", 0);
							ty += draw_textx("Press \fs\fc%s\fS to attack", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, attackkey);
						}
						if(game::player1->reqreload < 0 && game::player1->canreload(game::player1->weapselect, m_spawnweapon(game::gamemode, game::mutators), lastmillis))
						{
							SEARCHBINDCACHE(reloadkey)("reload", 0);
							ty += draw_textx("Press \fs\fc%s\fS to reload ammo", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, reloadkey);
							if(weapons::autoreload > 1 && lastmillis-game::player1->weaplast[game::player1->weapselect] <= 1000)
								ty += draw_textx("Automatic reload in \fs\fy%.01f\fS second(s)", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1, float(1000-(lastmillis-game::player1->weaplast[game::player1->weapselect]))/1000.f);
						}
					}
					popfont();
				}
			}
			else if(game::player1->state == CS_SPECTATOR)
			{
				ty += draw_textx("%s", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, -1, game::tvmode() ? "SpecTV" : "Spectating");
				if(shownotices >= 2)
				{
					SEARCHBINDCACHE(speconkey)("spectator 0", 1);
					pushfont("default");
					ty += draw_textx("Press \fs\fc%s\fS to play", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, -1, speconkey);
					if(shownotices >= 3)
					{
						SEARCHBINDCACHE(specmodekey)("specmodeswitch", 1);
						ty += draw_textx("Press \fs\fc%s\fS to %s", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, -1, specmodekey, game::tvmode() ? "look around" : "observe");
					}
					popfont();
				}
			}
		}
		popfont();

		if(overlaydisplay >= 2 || (game::player1->state == CS_ALIVE && (overlaydisplay || !game::isthirdperson())))
		{
			Texture *t = *overlaytex ? textureload(overlaytex, 3) : notexture;
			if(t != notexture)
			{
				glBindTexture(GL_TEXTURE_2D, t->id);
				glColor4f(1.f, 1.f, 1.f, overlayblend*hudblend);
				drawtex(0, 0, hudwidth, hudsize);
			}
		}
		drawpointers(w, h); // do this last, as it has to interact with the lower levels unhindered

		glDisable(GL_BLEND);
	}

	void drawconsole(int type, int w, int h, int x, int y, int s)
	{
		static vector<char *> refs; refs.setsizenodelete(0);
		int numl = 0;
		bool full = fullconsole || commandmillis >= 0;
		if(full) numl = fullconsize;
		else numl = consize;
		if(type == CON_CHAT)
		{
			numl = chatconsize;
			if(numl) loopv(conlines) if(conlines[i].type == CON_CHAT)
			{
				if(full || lastmillis-conlines[i].outtime < chatcontime)
				{
					refs.add(conlines[i].cref);
					if(refs.length() >= numl) break;
				}
			}
			pushfont("hud");
			int r = x+FONTW, z = y;
			loopv(refs) z -= draw_textx("%s", r, z, 255, 255, 255, int(255*(full ? fullconblend : chatconblend)*hudblend), TEXT_LEFT_UP, -1, s, refs[i]);
			popfont();
		}
		else
		{
			if(numl) loopv(conlines) if(conlines[i].type == CON_INFO || type < 0)
			{
				if(conskip ? i>=conskip-1 || i>=conlines.length()-numl : full || lastmillis-conlines[i].outtime < contime)
				{
					refs.add(conlines[i].cref);
					if(refs.length() >= numl) break;
				}
			}
			pushfont("sub");
			int z = y;
			loopvrev(refs) z += draw_textx("%s", x, z, 255, 255, 255, int(255*(full ? fullconblend : conblend)*hudblend), TEXT_CENTERED, -1, s, refs[i]);
			popfont();
		}
	}

	float radarrange()
	{
		float dist = float(radardist);
		if(game::player1->state == CS_EDITING) dist = float(editradardist);
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
				 { 0.f, 0.f }, { 0.f, 0.5f }, { 0.5f, 0.f }, { 0.5f, 0.5f }
			};
			fx = rdblip[blip][0];
			fy = rdblip[blip][1];
			fw = fh = 0.5f;
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
			defvformatstring(str, text, text);
			draw_textx("%s", cx+tq, cy+(idx > -4 ? ts : ts/2-FONTH/2), 255, 255, 255, int(blend*255.f), TEXT_CENTERED|TEXT_NO_INDENT, -1, -1, str);
			if(font && *font) popfont();
		}
	}

	void drawplayerblip(gameent *d, int w, int h, int s, float blend)
	{
		vec dir = d->headpos();
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
			if(hastv(radarplayernames)) drawblip(w, h, s, fade, -1, dir, r, g, b, "radar", "%s", game::colorname(d, NULL, "", false));
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
		if(type > NOTUSED && type < MAXENTTYPES && ((enttype[type].usetype == EU_ITEM && spawned) || game::player1->state == CS_EDITING))
		{
			float inspawn = radaritemtime && spawned && lastspawn && lastmillis-lastspawn <= radaritemtime ? float(lastmillis-lastspawn)/float(radaritemtime) : 0.f;
			if(enttype[type].noisy && (game::player1->state != CS_EDITING || !editradarnoisy || (editradarnoisy < 2 && !insel)))
				return;
			if(game::player1->state != CS_EDITING && radaritemspawn && (enttype[type].usetype != EU_ITEM || inspawn <= 0.f))
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
			int cp = type == WEAPON ? weapattr(attr1, m_spawnweapon(game::gamemode, game::mutators)) : -2;
			float r = 1.f, g = 1.f, b = 1.f, fade = insel ? 1.f : clamp(1.f-(dist/radarrange()), 0.1f, 1.f)*blend;
			if(cp >= 0)
			{
				r = (weaptype[cp].colour>>16)/255.f;
				g = ((weaptype[cp].colour>>8)&0xFF)/255.f;
				b = (weaptype[cp].colour&0xFF)/255.f;
			}
			if(game::player1->state != CS_EDITING && !insel && inspawn > 0.f)
				fade = radaritemspawn ? 1.f-inspawn : fade+((1.f-fade)*(1.f-inspawn));
			if(insel) drawblip(w, h, s, fade, cp, dir, r, g, b, "radar", "%s\n%s", enttype[type].name, entities::entinfo(type, attr1, attr2, attr3, attr4, attr5, insel));
			else if(hastv(radaritemnames)) drawblip(w, h, s, fade, cp, dir, r, g, b, "radar", "%s", entities::entinfo(type, attr1, attr2, attr3, attr4, attr5, false));
			else drawblip(w, h, s, fade, cp, dir, r, g, b);
		}
	}

	void drawentblips(int w, int h, int s, float blend)
	{
		if(m_edit(game::gamemode) && game::player1->state == CS_EDITING && (entities::ents.inrange(enthover) || !entgroup.empty()))
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

	void drawradar(int w, int h, float blend)
	{
		int s = max(w, h)/2;
		if(hastv(radarcard) || (editradarcard && m_edit(game::gamemode))) drawcardinalblips(w, h, s, blend*radarblend, m_edit(game::gamemode));
		if(hastv(radarplayers) || m_edit(game::gamemode))
		{
			loopv(game::players) if(game::players[i] && game::players[i]->state == CS_ALIVE)
				drawplayerblip(game::players[i], w, h, s, blend*radarblend);
		}
		if(hastv(radaritems) || m_edit(game::gamemode)) drawentblips(w, h, s, blend*radarblend);
		if(hastv(radarflags))
		{
			if(m_stf(game::gamemode)) stf::drawblips(w, h, s, blend);
			else if(m_ctf(game::gamemode)) ctf::drawblips(w, h, s, blend*radarblend);
		}
	}

	int drawitem(const char *tex, int x, int y, float size, bool left, float r, float g, float b, float fade, float skew, const char *font, const char *text, ...)
	{
		if(skew <= 0.f) return 0;
		float f = fade*inventoryblend*skew, cr = r*skew, cg = g*skew, cb = b*skew, s = size*skew;
		settexture(tex, 3);
		glColor4f(cr, cg, cb, f);
		drawsized(left ? x : x-int(s), y-int(s), int(s));
		if(text && *text)
		{
			glPushMatrix();
			glScalef(skew, skew, 1);
			if(font && *font) pushfont(font);
			int tx = int((left ? (x+s) : x)*(1.f/skew)), ty = int((y-s)*(1.f/skew)), ti = int(255.f*f);
			defvformatstring(str, text, text);
			draw_textx("%s", tx, ty, 255, 255, 255, ti, TEXT_RIGHT_JUSTIFY, -1, -1, str);
			if(font && *font) popfont();
			glPopMatrix();
		}
		return int(s);
	}

	void drawitemsubtext(int x, int y, float size, bool left, float skew, const char *font, float blend, const char *text, ...)
	{
		if(skew <= 0.f) return;
		glPushMatrix();
		glScalef(skew, skew, 1);
		if(font && *font) pushfont(font);
		int tx = int(x*(1.f/skew)), ty = int(y*(1.f/skew))-FONTH, ti = int(255.f*inventoryblend*blend*skew);
		defvformatstring(str, text, text);
		draw_textx("%s", tx, ty, 255, 255, 255, ti, left ? TEXT_LEFT_JUSTIFY : TEXT_RIGHT_JUSTIFY, -1, -1, str);
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

	int drawentitem(int n, int x, int y, int s, float skew, float fade)
	{
		if(entities::ents.inrange(n))
		{
			gameentity &e = *(gameentity *)entities::ents[n];
			const char *itext = itemtex(e.type, e.attr[0]);
			int ty = drawitem(itext && *itext ? itext : inventoryenttex, x, y, s, false, 1.f, 1.f, 1.f, fade, skew, "default", "%s (%d)", enttype[e.type].name, n);
			drawitemsubtext(x, y, s, false, skew, "sub", fade, "%s", entities::entinfo(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], true));
			return ty;
		}
		return 0;
	}

	int drawselection(int x, int y, int s, float blend)
	{
		int sy = showfps ? s/2+s/4 : s/16;
		if(game::player1->state == CS_ALIVE)
		{
			if(game::player1->team)
				sy += drawitem(flagtex(game::player1->team), x, y-sy, s, false, 1.f, 1.f, 1.f, blend, 1.f, "sub", "%s%s", teamtype[game::player1->team].chat, teamtype[game::player1->team].name) + s/8;
			else sy += drawitem(flagtex(game::player1->team), x, y-sy, s, false, 1.f, 1.f, 1.f, blend, 1.f) + s/8;
			if(inventoryammo)
			{
				const char *hudtexs[WEAPON_MAX] = {
					plasmatex, shotguntex, chainguntex, flamertex, carbinetex, rifletex, grenadestex,
					paintguntex
				};
				int sweap = m_spawnweapon(game::gamemode, game::mutators);
				loopi(WEAPON_MAX) if(game::player1->hasweap(i, sweap) || lastmillis-game::player1->weaplast[i] < game::player1->weapwait[i])
				{
					float fade = blend, size = s, skew = 1.f;
					if(game::player1->weapstate[i] == WPSTATE_SWITCH || game::player1->weapstate[i] == WPSTATE_PICKUP)
					{
						float amt = clamp(float(lastmillis-game::player1->weaplast[i])/float(game::player1->weapwait[i]), 0.f, 1.f);
						skew = (i != game::player1->weapselect ?
							(
								game::player1->hasweap(i, sweap) ? 1.f-(amt*(0.25f)) : 1.f-amt
							) : (
								game::player1->weapstate[i] == WPSTATE_PICKUP ? amt : 0.75f+(amt*(0.25f))
							)
						);
					}
					else if(i != game::player1->weapselect) skew = 0.75f;
					bool instate = (i == game::player1->weapselect || game::player1->weapstate[i] != WPSTATE_PICKUP);
					int oldy = y-sy, delay = lastmillis-game::player1->lastspawn;
					if(delay < 1000) skew *= delay/1000.f;
					float r = 1.f, g = 1.f, b = 1.f;
					if(teamwidgets >= (inventorycolour ? 2 : 1)) skewcolour(r, g, b);
					else if(inventorycolour)
					{
						r = (weaptype[i].colour>>16)/255.f;
						g = ((weaptype[i].colour>>8)&0xFF)/255.f;
						b = (weaptype[i].colour&0xFF)/255.f;
					}
					if(inventoryammo && (instate || inventoryammo > 1) && game::player1->hasweap(i, sweap))
						sy += drawitem(hudtexs[i], x, y-sy, size, false, r, g, b, fade, skew, "default", "%d", game::player1->ammo[i]);
					else sy += drawitem(hudtexs[i], x, y-sy, size, false, r, g, b, fade, skew);
					if(inventoryweapids && (instate || inventoryweapids > 1))
					{
						static string weapids[WEAPON_MAX];
						static int lastweapids = -1;
						if(lastweapids != changedkeys)
						{
							loopj(WEAPON_MAX)
							{
								defformatstring(action)("weapon %d", j);
								const char *actkey = searchbind(action, 0);
								if(actkey && *actkey) copystring(weapids[j], actkey);
								else formatstring(weapids[j])("%d", j+1);
							}
							lastweapids = changedkeys;
						}
						if(inventoryweapents && entities::ents.inrange(game::player1->entid[i]) && game::player1->hasweap(i, sweap))
							drawitemsubtext(x, oldy, size, false, skew, "sub", fade, "[\fs\fa%d\fS] \fs%s%s\fS", game::player1->entid[i], inventorycolour >= 2 ? weaptype[i].text : "\fa", weapids[i]);
						else drawitemsubtext(x, oldy, size, false, skew, "sub", fade, "\fs%s%s\fS", inventorycolour >= 2 ? weaptype[i].text : "\fa", weapids[i]);
					}
				}
			}
		}
		else
		{
			if(game::player1->state == CS_EDITING)
			{
				sy += drawitem(inventoryedittex, x, y-sy, s, false, 1.f, 1.f, 1.f, blend, 1.f) + s/8;
				if(inventoryedit)
				{
					int stop = hudsize-s*3;
					sy += drawentitem(enthover, x, y-sy, s, 1.f, blend);
					loopv(entgroup) if(entgroup[i] != enthover && (sy += drawentitem(entgroup[i], x, y-sy, s, 0.65f, blend)) >= stop) break;
				}
			}
			else if(game::player1->state == CS_WAITING) sy += drawitem(inventorywaittex, x, y-sy, s, false, 1.f, 1.f, 1.f, blend, 1.f);
			else if(game::player1->state == CS_DEAD) sy += drawitem(inventorydeadtex, x, y-sy, s, false,1.f, 1.f, 1.f, blend, 1.f);
			else sy += drawitem(inventorychattex, x, y-sy, s, false, 1.f, 1.f, 1.f, blend, 1.f);
		}
		return sy;
	}

	int drawhealth(int x, int y, int s, float blend)
	{
        int size = s*2, width = s, glow = int(width*inventoryglow);
		float fade = inventoryblend*blend, r = 1.f, g = 1.f, b = 1.f;
		if(teamwidgets) skewcolour(r, g, b);
        settexture(healthtex, 3);
        glColor4f(r, g, b, fade*(game::player1->state == CS_ALIVE ? 0.5f : 1.f));
        drawtex(x, y-size, width, size);
		if(inventoryhealth && game::player1->state == CS_ALIVE)
		{
			if(game::player1->lastspawn && lastmillis-game::player1->lastspawn < 1000) fade *= (lastmillis-game::player1->lastspawn)/1000.f;
			else if(inventorythrob && regentime && game::player1->lastregen && lastmillis-game::player1->lastregen < regentime*1000)
			{
				float amt = clamp((lastmillis-game::player1->lastregen)/float(regentime*1000), 0.f, 1.f);
				if(amt < 0.5f) amt = 1.f-amt;
				fade *= amt;
			}
			const struct healthbarstep
			{
				float health, r, g, b;
			} steps[] = { { 0, 0.5f, 0, 0 }, { 0.25f, 1, 0, 0 }, { 0.75f, 1, 0.5f, 0 }, { 1, 0, 1, 0 } };
			glBegin(GL_QUAD_STRIP);
			float health = clamp(game::player1->health/float(m_maxhealth(game::gamemode, game::mutators)), 0.0f, 1.0f);
			const float margin = 0.1f;
			int sx = x+glow, sy = y-size+glow, sw = width-glow*2, sh = size-glow*2;
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
						glColor4f(r, g, b, fade); glTexCoord2f(0, hoff); glVertex2f(sx, sy + hoff*sh);
						glColor4f(r, g, b, fade); glTexCoord2f(1, hoff); glVertex2f(sx + sw, sy + hoff*sh);
					}
					if(step.health > health + margin)
					{
						float hoff = 1 - (health + margin), hlerp = (health + margin - steps[i-1].health) / (step.health - steps[i-1].health),
							  r = step.r*hlerp + steps[i-1].r*(1-hlerp),
							  g = step.g*hlerp + steps[i-1].g*(1-hlerp),
							  b = step.b*hlerp + steps[i-1].b*(1-hlerp);
						glColor4f(r, g, b, 0); glTexCoord2f(0, hoff); glVertex2f(sx, sy + hoff*sh);
						glColor4f(r, g, b, 0); glTexCoord2f(1, hoff); glVertex2f(sx + sw, sy + hoff*sh);
						break;
					}
				}
				float off = 1 - step.health, hfade = fade, r = step.r, g = step.g, b = step.b;
				if(step.health > health) hfade *= 1 - (step.health - health)/margin;
				glColor4f(r, g, b, hfade); glTexCoord2f(0, off); glVertex2f(sx, sy + off*sh);
				glColor4f(r, g, b, hfade); glTexCoord2f(1, off); glVertex2f(sx + sw, sy + off*sh);
			}
			glEnd();
			if(inventoryhealth > 1) drawitemsubtext(x, y, width, true, 1.f, "default", fade, "%d", max(game::player1->health, 0));
		}
		else
		{
			const char *state = "\fgALIVE";
			switch(game::player1->state)
			{
				case CS_EDITING: state = "\fcEDIT"; break;
				case CS_WAITING: state = "\fyWAIT"; break;
				case CS_SPECTATOR: state = "\faSPEC"; break;
				case CS_DEAD: default: state = "\frDEAD"; break;
			}
			drawitemsubtext(x, y, width, true, 1.f, "sub", fade, "%s", state);
		}
		return size;
	}

	void drawinventory(int w, int h, int edge, float blend)
	{
		int cx[2] = { edge, w-edge }, cy[2] = { h-edge, h-edge }, cs = int(inventorysize*w), cr = cs/8, cc = 0;
		loopi(2) switch(i)
		{
			case 0: default:
			{
				if((cc = drawhealth(cx[i], cy[i], cs, blend)) > 0) cy[i] -= cc+cr;
				if(!m_edit(game::gamemode) && inventoryscores && ((cc = sb.drawinventory(cx[i], cy[i], cs, blend)) > 0)) cy[i] -= cc+cr;
				break;
			}
			case 1:
			{
				if(!texpaneltimer)
				{
					if((cc = drawselection(cx[i], cy[i], cs, blend)) > 0) cy[i] -= cc+cr;
					if(m_ctf(game::gamemode) && ((cc = ctf::drawinventory(cx[i], cy[i], cs, blend)) > 0)) cy[i] -= cc+cr;
					if(m_stf(game::gamemode) && ((cc = stf::drawinventory(cx[i], cy[i], cs, blend)) > 0)) cy[i] -= cc+cr;
				}
				break;
			}
		}
	}

	void drawdamage(int w, int h, int s, float blend)
	{
		float pc = 0.f;
		if((game::player1->state == CS_ALIVE && hud::damageresidue > 0) || game::player1->state == CS_DEAD)
			pc = float(game::player1->state == CS_DEAD ? 100 : min(hud::damageresidue, 100))/100.f;

		if(showdamage > 1 && game::player1->state == CS_ALIVE && regentime && game::player1->lastregen && lastmillis-game::player1->lastregen < regentime*1000)
		{
			float skew = clamp((lastmillis-game::player1->lastregen)/float(regentime*1000), 0.f, 1.f);
			if(skew > 0.5f) skew = 1.f-skew;
			pc += (1.f-pc)*skew;
		}

		if(pc > 0.f)
		{
			Texture *t = *damagetex ? textureload(damagetex, 3) : notexture;
			if(t != notexture)
			{
				glBindTexture(GL_TEXTURE_2D, t->id);
				glColor4f(1.f, 1.f, 1.f, pc*blend*damageblend);
				drawtex(0, 0, w, h);
			}
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
            int col = paintcolours[actor->type == ENT_PLAYER && m_team(game::gamemode, game::mutators) ? actor->team : rnd(10)];
            dir.color = vec((col>>16)&0xFF, (col>>8)&0xFF, col&0xFF).div(0xFF);
        }
        else if(kidmode || game::noblood) dir.color = vec(1, 0.25f, 1);
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
		int frame = lastmillis-game::lastzoom;
		float pc = frame < game::zoomtime() ? float(frame)/float(game::zoomtime()) : 1.f;
		if(!game::zooming) pc = 1.f-pc;
		glBindTexture(GL_TEXTURE_2D, t->id);
		glColor4f(1.f, 1.f, 1.f, pc*blend);
		drawtex(0, 0, w, h);
	}

	void drawhud(int w, int h)
	{
		float fade = hudblend;
		bool texturing = true;
		if(!game::maptime || lastmillis-game::maptime < titlecard)
		{
			float amt = game::maptime ? float(lastmillis-game::maptime)/float(titlecard) : 0.f;
			if(amt < 1.f)
			{
				usetexturing(false); texturing = false;
				drawblend(0, 0, w, h, amt, amt, amt);
				fade *= amt;
			}
		}
		else if(game::tvmode())
		{
			float amt = 1.f;
			int millis = game::spectvtime ? min(game::spectvtime/10, 500) : 500, interval = lastmillis-game::lastspec;
			if(!game::lastspec || interval < millis) amt = game::lastspec ? float(interval)/float(millis) : 0.f;
			else if(game::spectvtime && interval > game::spectvtime-millis) amt = float(game::spectvtime-interval)/float(millis);
			if(amt < 1.f)
			{
				usetexturing(false); texturing = false;
				drawblend(0, 0, w, h, amt, amt, amt);
				fade *= amt;
			}
		}
		else if(game::player1->state == CS_ALIVE && game::player1->lastspawn && lastmillis-game::player1->lastspawn < 1000)
		{
			float amt = (lastmillis-game::player1->lastspawn)/500.f;
			if(amt < 2.f)
			{
				float r = 1.f, g = 1.f, b = 1.f;
				skewcolour(r, g, b, true);
				fade *= amt*0.5f;
				if(amt < 1.f)
				{
					r *= amt;
					g *= amt;
					b *= amt;
				}
				else
				{
					amt -= 1.f;
					r += (1.f-r)*amt;
					g += (1.f-g)*amt;
					b += (1.f-b)*amt;
				}
				usetexturing(false); texturing = false;
				drawblend(0, 0, w, h, r, g, b);
			}
		}
		if(!texturing) usetexturing(true);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		int ox = hudwidth, oy = hudsize, os = int(oy*gapsize), is = int(oy*inventorysize);
		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);
		if(game::maptime && connected() && client::ready())
		{
			if(underlaydisplay >= 2 || (game::player1->state == CS_ALIVE && (underlaydisplay || !game::isthirdperson())))
			{
				Texture *t = *underlaytex ? textureload(underlaytex, 3) : notexture;
				if(t != notexture)
				{
					glBindTexture(GL_TEXTURE_2D, t->id);
					glColor4f(1.f, 1.f, 1.f, underlayblend*hudblend);
					drawtex(0, 0, ox, oy);
				}
			}
			if(game::player1->state == CS_ALIVE && game::inzoom() && weaptype[game::player1->weapselect].snipes)
				drawsniper(ox, oy, os, fade);
			if(showdamage && !kidmode && !game::noblood) drawdamage(ox, oy, os, fade);
			if(!UI::hascursor())
			{
				if(showdamagecompass) drawdamagecompass(ox, oy, os, fade);
				if(game::player1->state == CS_EDITING ? showeditradar > 0 : hastv(showradar)) drawradar(ox, oy, fade);
			}
			if(showinventory) drawinventory(ox, oy, os, fade);
		}

		int br = is+os*4, bs = (ox-br*2)/2, bx = ox-br, by = oy-os, bf = int(255*fade*statblend);
		if(showconsole)
		{
			drawconsole(showconsole >= 2 ? CON_INFO : -1, ox, oy, ox/2, os, bs*2);
			if(showconsole >= 2) drawconsole(CON_CHAT, ox, oy, br, by, bs);
		}

		if(!texpaneltimer)
		{
			pushfont("sub");
			bx -= FONTW; by -= FONTH;
			if(totalmillis-laststats >= statrate)
			{
				memcpy(prevstats, curstats, sizeof(prevstats));
				laststats = totalmillis-(totalmillis%statrate);
			}
			int nextstats[NUMSTATS] = {
				vtris*100/max(wtris, 1), vverts*100/max(wverts, 1), xtraverts/1024, xtravertsva/1024, glde, gbatches, getnumqueries(), rplanes, curfps, bestfpsdiff, worstfpsdiff, autoadjustlevel
			};
			loopi(NUMSTATS) if(prevstats[i] == curstats[i]) curstats[i] = nextstats[i];
			if(showfps)
			{
				draw_textx("%d", ox-os*2-is/2, by-FONTH, 255, 255, 255, bf, TEXT_CENTERED, -1, bs, curstats[8]);
				draw_textx("fps", ox-os*2-is/2, by, 255, 255, 255, bf, TEXT_CENTERED, -1, -1);
				switch(showfps)
				{
					case 3:
						if(autoadjust) by -= draw_textx("min:%d max:%d range:+%d-%d bal:\fs%s%d\fS%%", bx, by, 255, 255, 255, bf, TEXT_RIGHT_JUSTIFY, -1, bs, minfps, maxfps, curstats[9], curstats[10], curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
						else by -= draw_textx("max:%d range:+%d-%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_JUSTIFY, -1, bs, maxfps, curstats[9], curstats[10]);
						break;
					case 2:
						if(autoadjust) by -= draw_textx("min:%d max:%d, bal:\fs%s%d\fS%% %dfps", bx, by, 255, 255, 255, bf, TEXT_RIGHT_JUSTIFY, -1, bs, minfps, maxfps, curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
						else by -= draw_textx("max:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_JUSTIFY, -1, bs, maxfps);
						break;
					default: break;
				}
			}
			if(showstats > (m_edit(game::gamemode) ? 0 : 1))
			{
				by -= draw_textx("ond:%d va:%d gl:%d(%d) oq:%d lm:%d rp:%d pvs:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_JUSTIFY, -1, bs, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6], lightmaps.length(), curstats[7], getnumviewcells());
				by -= draw_textx("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", bx, by, 255, 255, 255, bf, TEXT_RIGHT_JUSTIFY, -1, bs, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
			}
			if(connected() && client::ready() && game::maptime)
			{
				if(game::player1->state == CS_EDITING)
				{
					by -= draw_textx("cube:%s%d ents:%d[%d] corner:%d orient:%d grid:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_JUSTIFY, -1, bs,
							selchildcount<0 ? "1/" : "", abs(selchildcount), entities::ents.length(), entgroup.length(),
									sel.corner, sel.orient, sel.grid);
					by -= draw_textx("sel:%d,%d,%d %d,%d,%d (%d,%d,%d,%d)", bx, by, 255, 255, 255, bf, TEXT_RIGHT_JUSTIFY, -1, bs,
							sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z,
								sel.cx, sel.cxs, sel.cy, sel.cys);
					by -= draw_textx("pos:%d,%d,%d yaw:%d pitch:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_JUSTIFY, -1, bs,
							(int)game::player1->o.x, (int)game::player1->o.y, (int)game::player1->o.z,
							(int)game::player1->yaw, (int)game::player1->pitch);
				}
			}
			popfont();
		}
		glDisable(GL_BLEND);
	}

	void gamemenus() { sb.show(); }
}
