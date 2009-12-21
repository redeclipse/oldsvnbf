#include "game.h"
extern char *progresstitle, *progresstext;
extern float progresspart;

namespace hud
{
	const int NUMSTATS = 12;
	int damageresidue = 0, hudwidth = 0, lastteam = 0, lastnewgame = 0,
		laststats = 0, prevstats[NUMSTATS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, curstats[NUMSTATS] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	vector<int> teamkills;
	scoreboard sb;
    struct damageloc
    {
        int attacker, outtime, damage; vec dir, colour;
        damageloc(int a, int t, int d, const vec &p, const vec &c) : attacker(a), outtime(t), damage(d), dir(p), colour(c) {}
    };
    vector<damageloc> damagelocs;
	VARP(damageresiduefade, 0, 500, INT_MAX-1);

	VARP(showhud, 0, 1, 1);
	VARP(huduioverride, 0, 1, 2); // 0=off, 1=except intermission, 2=interactive ui only
	VARP(hudsize, 0, 2048, INT_MAX-1);
	FVARP(hudblend, 0, 1, 1);
	FVARP(gapsize, 0, 0.01f, 1000);

	VARP(showconsole, 0, 2, 2);
	VARP(shownotices, 0, 3, 4);

	VARP(showfps, 0, 1, 3);
	VARP(showstats, 0, 1, 2);
	VARP(statrate, 0, 200, 1000);
	FVARP(statblend, 0, 0.65f, 1);

	bool fullconsole = false;
	void toggleconsole() { fullconsole = !fullconsole; }
	COMMAND(toggleconsole, "");

	VARP(titlefade, 0, 3000, 10000);
	VARP(tvmodefade, 0, 1000, INT_MAX-1);
	VARP(spawnfade, 0, 3000, INT_MAX-1);

	VARP(commandfade, 0, 250, INT_MAX-1);
	FVARP(commandfadeamt, 0, 0.75f, 1);
	VARP(uifade, 0, 250, INT_MAX-1);
	FVARP(uifadeamt, 0, 0.5f, 1);

	int conskip = 0;
	void setconskip(int *n)
	{
		conskip += *n;
		if(conskip<0) conskip = 0;
	}
	COMMANDN(conskip, setconskip, "i");

	VARP(consize, 0, 6, 100);
	VARP(contime, 0, 30000, INT_MAX-1);
	VARP(confade, 0, 1000, INT_MAX-1);
	VARP(conoverflow, 0, 9, INT_MAX-1);
	VARP(concenter, 0, 0, 1);
	VARP(confilter, 0, 1, 1);
	FVARP(conblend, 0, 0.6f, 1);
	VARP(chatconsize, 0, 5, 100);
	VARP(chatcontime, 0, 30000, INT_MAX-1);
	VARP(chatconfade, 0, 1000, INT_MAX-1);
	VARP(chatconoverflow, 0, 5, INT_MAX-1);
	FVARP(chatconblend, 0, 0.75f, 1);
	FVARP(fullconblend, 0, 0.9f, 1);

	FVARP(noticeoffset, -1, 0.4f, 1);
	FVARP(noticeblend, 0, 0.6f, 1);
	FVARP(noticescale, 1e-3f, 1, 1);
	VARP(noticetime, 0, 5000, INT_MAX-1);
	VARP(obitnotices, 0, 2, 2);

	TVAR(conopentex, "textures/conopen", 3);
	TVAR(playertex, "textures/player", 3);
	TVAR(deadtex, "textures/dead", 3);
	TVAR(dominatingtex, "textures/dominating", 3);
	TVAR(dominatedtex, "textures/dominated", 3);
	TVAR(inputtex, "textures/menu", 3);

	VARP(teamwidgets, 0, 1, 3); // colour based on team
	VARP(teamclips, 0, 1, 2);
	VARP(teamcrosshair, 0, 1, 2);
	VARP(teamnotices, 0, 0, 1);
	VARP(teamkillnum, 0, 3, INT_MAX-1);
	VARP(teamkilltime, 0, 60000, INT_MAX-1);

	TVAR(underlaytex, "", 3);
	VARP(underlaydisplay, 0, 0, 2); // 0 = only firstperson and alive, 1 = only when alive, 2 = always
	FVARP(underlayblend, 0, 0.5f, 1);
	TVAR(overlaytex, "", 3);
	VARP(overlaydisplay, 0, 0, 2); // 0 = only firstperson and alive, 1 = only when alive, 2 = always
	FVARP(overlayblend, 0, 0.5f, 1);

	VARP(showdamage, 0, 1, 2); // 1 shows just damage texture, 2 blends as well
	VARP(damagefade, 0, 0, 1);
	TVAR(damagetex, "textures/damage", 3);
	FVARP(damageblend, 0, 0.75f, 1);
	TVAR(burntex, "textures/burn", 3);
	FVARP(burnblend, 0, 0.65f, 1);

	VARP(showindicator, 0, 1, 1);
	FVARP(indicatorsize, 0, 0.03f, 1000);
	FVARP(indicatorblend, 0, 0.85f, 1);
	TVAR(indicatortex, "textures/indicator", 3);
	TVAR(zoomtex, "textures/zoom", 3);

	VARP(showcrosshair, 0, 1, 1);
	FVARP(crosshairsize, 0, 0.04f, 1000);
	VARP(crosshairhitspeed, 0, 500, INT_MAX-1);
	FVARP(crosshairblend, 0, 0.95f, 1);
	VARP(crosshairhealth, 0, 2, 2);
	FVARP(crosshairskew, 1e-3f, 0.3f, 1);
	TVAR(relativecursortex, "textures/crosshair", 3);
	TVAR(guicursortex, "textures/cursor", 3);
	TVAR(editcursortex, "textures/crosshair", 3);
	TVAR(speccursortex, "textures/crosshair", 3);
	TVAR(crosshairtex, "textures/crosshair", 3);
	TVAR(teamcrosshairtex, "", 3);
	TVAR(hitcrosshairtex, "textures/hitcrosshair", 3);
	FVARP(cursorsize, 0, 0.025f, 1000);
	FVARP(cursorblend, 0, 1, 1);

	TVAR(zoomcrosshairtex, "", 3);
	FVARP(zoomcrosshairsize, 0, 0.575f, 1000);

	VARP(showinventory, 0, 1, 1);
	VARP(inventoryammo, 0, 1, 2);
	VARP(inventorygame, 0, 1, 2);
	VARP(inventoryteams, 0, 4000, INT_MAX-1);
	VARP(inventorystatus, 0, 2, 2);
	VARP(inventoryscore, 0, 0, 1);
	VARP(inventoryweapids, 0, 1, 2);
	VARP(inventorycolour, 0, 2, 2);
	FVARP(inventorysize, 0, 0.07f, 1000);
	FVARP(inventoryskew, 1e-3f, 0.6f, 1);
	FVARP(inventorygrow, 1e-3f, 0.75f, 1);
	FVARP(inventoryblend, 0, 0.6f, 1);

	VARP(inventoryedit, 0, 1, 1);
	FVARP(inventoryeditblend, 0, 1, 1);
	FVARP(inventoryeditskew, 1e-3f, 0.65f, 1);

	VARP(inventoryhealth, 0, 3, 3);
	FVARP(inventoryhealththrob, 0, 1, 1);
	FVARP(inventoryhealthblend, 0, 0.95f, 1);
	FVARP(inventoryhealthglow, 0, 0.125f, 1);
	FVARP(inventoryhealthflash, 0, 1, 1);
	VARP(inventoryimpulse, 0, 2, 2);
	VARP(inventorytrial, 0, 2, 2);

	TVAR(meleetex, "textures/melee", 3);
	TVAR(pistoltex, "textures/pistol", 3);
	TVAR(shotguntex, "textures/shotgun", 3);
	TVAR(smgtex, "textures/smg", 3);
	TVAR(grenadetex, "textures/grenade", 3);
	TVAR(flamertex, "textures/flamer", 3);
	TVAR(plasmatex, "textures/plasma", 3);
	TVAR(rifletex, "textures/rifle", 3);
	TVAR(healthtex, "textures/health", 3);
	TVAR(progresstex, "textures/progress", 3);
	TVAR(inventoryenttex, "textures/progress", 3);
	TVAR(inventoryedittex, "textures/arrow", 3);
	TVAR(inventorywaittex, "textures/wait", 3);
	TVAR(inventorydeadtex, "textures/dead", 3);
	TVAR(inventorychattex, "textures/conopen", 3);

	TVAR(neutraltex, "textures/team", 3);
	TVAR(alphatex, "textures/teamalpha", 3);
	TVAR(betatex, "textures/teambeta", 3);
	TVAR(gammatex, "textures/teamgamma", 3);
	TVAR(deltatex, "textures/teamdelta", 3);

	VARP(showclips, 0, 2, 2);
	FVARP(clipsize, 0, 0.045f, 1000);
	FVARP(clipblend, 0, 0.45f, 1000);
	FVARP(clipcolour, 0, 1, 1);
	TVAR(pistolcliptex, "textures/pistolclip", 3);
	TVAR(shotguncliptex, "textures/shotgunclip", 3);
	TVAR(smgcliptex, "textures/smgclip", 3);
	TVAR(grenadecliptex, "textures/grenadeclip", 3);
	TVAR(flamercliptex, "textures/flamerclip", 3);
	TVAR(plasmacliptex, "textures/plasmaclip", 3);
	TVAR(riflecliptex, "textures/rifleclip", 3);
	FVARP(pistolclipskew, 0, 0.85f, 1);
	FVARP(shotgunclipskew, 0, 1, 1);
	FVARP(smgclipskew, 0, 0.85f, 1);
	FVARP(grenadeclipskew, 0, 1.25f, 1);
	FVARP(flamerclipskew, 0, 0.85f, 1);
	FVARP(plasmaclipskew, 0, 0.85f, 1);
	FVARP(rifleclipskew, 0, 1, 1);

	VARP(showradar, 0, 2, 2);
	TVAR(bliptex, "textures/blip", 3);
	TVAR(cardtex, "textures/card", 3);
	TVAR(flagtex, "textures/flag", 3);
	TVAR(arrowtex, "textures/arrow", 3);
	FVARP(radarblend, 0, 1, 1);
	FVARP(radarcardsize, 0, 0.5f, 1000);
	FVARP(radarcardblend, 0, 0.75f, 1);
	FVARP(radarplayerblend, 0, 0.5f, 1);
	FVARP(radarplayersize, 0, 0.5f, 1000);
	FVARP(radarblipblend, 0, 0.5f, 1);
	FVARP(radarblipsize, 0, 0.5f, 1000);
	FVARP(radarflagblend, 0, 1, 1);
	FVARP(radarflagsize, 0, 1, 1000);
	FVARP(radaritemblend, 0, 0.75f, 1);
	FVARP(radaritemsize, 0, 0.9f, 1000);
	FVARP(radarsize, 0, 0.035f, 1000);
	FVARP(radaroffset, 0, 0.035f, 1000);
	VARP(radardist, 0, 0, INT_MAX-1); // 0 = use world size
	VARP(radarcard, 0, 0, 2);
	VARP(radaritems, 0, 2, 2);
	VARP(radaritemspawn, 0, 1, 1);
	VARP(radaritemtime, 0, 5000, INT_MAX-1);
	VARP(radaritemnames, 0, 0, 2);
	VARP(radarplayers, 0, 1, 2);
	VARP(radarplayernames, 0, 0, 2);
	VARP(radarflags, 0, 2, 2);
	VARP(radarflagnames, 0, 1, 2);

	VARP(radardamage, 0, 2, 5); // 0 = off, 1 = basic damage, 2 = with killer announce (+1 killer track, +2 and bots), 5 = verbose
	VARP(radardamagetime, 1, 500, INT_MAX-1);
	VARP(radardamagefade, 1, 2000, INT_MAX-1);
	FVARP(radardamagesize, 0, 4, 1000);
	FVARP(radardamageblend, 0, 1, 1);
	FVARP(radardamagetrack, 0, 1, 1000);
	VARP(radardamagemin, 1, 25, INT_MAX-1);
	VARP(radardamagemax, 1, 100, INT_MAX-1);

	VARP(showeditradar, 0, 0, 1);
	VARP(editradarcard, 0, 0, 1);
	VARP(editradardist, 0, 64, INT_MAX-1); // 0 = use radardist
	VARP(editradarnoisy, 0, 1, 2);

	VARP(motionblurfx, 0, 1, 2); // 0 = off, 1 = on, 2 = override
	FVARP(motionblurmin, 0, 0.0f, 1); // minimum
	FVARP(motionblurmax, 0, 0.75f, 1); // maximum
	FVARP(motionbluramt, 0, 0.5f, 1); // used for override

	bool chkcond(int val, bool cond)
	{
		if(val == 2 || (val && cond)) return true;
		return false;
	}

	char *timetostr(int millis, bool limited)
	{
		static string timestr; timestr[0] = 0;
		int tm = millis, ms = 0, ss = 0, mn = 0;
		if(tm > 0)
		{
			ms = tm%1000;
			tm = (tm-ms)/1000;
			if(tm > 0)
			{
				ss = tm%60;
				tm = (tm-ss)/60;
				if(tm > 0) mn = tm;
			}
		}
		formatstring(timestr)(limited ? "%d:%02d.%d" : "%d:%02d.%03d", mn, ss, limited ? ms/100 : ms);
		return timestr;
	}

	float motionblur(float scale)
	{
		float amt = 0;
		switch(motionblurfx)
		{
			case 1:
			{
				if(game::player1->state == CS_WAITING || game::player1->state == CS_SPECTATOR || game::player1->state == CS_EDITING) break;
				float damage = game::player1->state == CS_ALIVE ? min(damageresidue, 100)/100.f : 1.f,
					  healthscale = float(m_health(game::gamemode, game::mutators));
				if(healthscale > 0) damage = max(damage, 1.f-max(game::player1->health, 0)/healthscale);
				amt += damage*0.65f;
				if(fireburntime && game::player1->lastfire && lastmillis-game::player1->lastfire < fireburntime)
					amt += 0.25f+(float((lastmillis-game::player1->lastfire)%fireburndelay)/float(fireburndelay))*0.35f;
				if(FWV(impulsestyle) && (game::player1->turnside || (game::player1->action[AC_SPRINT] && (game::player1->move || game::player1->strafe))))
					amt += game::player1->turnside ? 0.125f : 0.25f;
				break;
			}
			case 2: amt += motionbluramt; break;
			default: break;
		}
		return clamp(amt, motionblurmin, motionblurmax)*scale;
	}

	void damage(int n, const vec &loc, gameent *actor, int weap)
	{
		damageresidue = clamp(damageresidue+n, 0, 200);
		vec colour = weap == WEAP_FLAMER || weap == WEAP_GRENADE ? vec(1.f, 0.35f, 0.0625f) : (kidmode || game::bloodscale <= 0 ? vec(1, 0.25f, 1) : vec(1.f, 0, 0));
        damagelocs.add(damageloc(actor->clientnum, lastmillis, n, vec(loc).sub(camera1->o).normalize(), colour));
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

	void drawblend(int x, int y, int w, int h, float r, float g, float b, bool blend = false)
	{
        if(!blend) glEnable(GL_BLEND);
        glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		glColor3f(r, g, b);
		glBegin(GL_QUADS);
		glVertex2f(x, y); glVertex2f(x+w, y);
		glVertex2f(x+w, y+h); glVertex2f(x, y+h);
		glEnd();
        if(!blend) glDisable(GL_BLEND);
        else glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void colourskew(float &r, float &g, float &b, float skew)
	{
		if(skew >= 2.f)
		{ // fully overcharged to green
			r = b = 0;
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
			b = 0;
		}
	}

	void healthskew(int &s, float &r, float &g, float &b, float &fade, float ss, bool throb)
	{
		if(throb && regentime && game::player1->lastregen && lastmillis-game::player1->lastregen <= regentime)
		{
			float skew = clamp((lastmillis-game::player1->lastregen)/float(regentime/2), 0.f, 1.f);
			if(skew > 0.5f) skew = 1.f-skew;
			fade *= 1.f-skew;
			s += int(s*ss*skew);
		}
		int total = m_health(game::gamemode, game::mutators);
		bool full = game::player1->health >= total;
		if(full)
		{
			int over = total;
			if(m_ctf(game::gamemode) || m_stf(game::gamemode)) over = max(extrahealth, total);
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
		POINTER_HAIR, POINTER_TEAM, POINTER_HIT, POINTER_ZOOM, POINTER_MAX
	};

    const char *getpointer(int index)
    {
        switch(index)
        {
            case POINTER_RELATIVE: return relativecursortex;
            case POINTER_GUI: return guicursortex;
            case POINTER_EDIT: return editcursortex;
            case POINTER_SPEC: return speccursortex;
            case POINTER_HAIR: return crosshairtex;
            case POINTER_TEAM: return teamcrosshairtex;
            case POINTER_HIT: return *hitcrosshairtex ? hitcrosshairtex : crosshairtex;
            case POINTER_ZOOM: return *zoomcrosshairtex ? zoomcrosshairtex : crosshairtex;
            default: break;
        }
        return NULL;
    }

	void drawindicator(int weap, int x, int y, int s)
	{
		Texture *t = textureload(indicatortex, 3);
		if(t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
		int millis = lastmillis-game::player1->weaplast[weap];
		float r = 1, g = 1, b = 1, amt = 0;
		switch(game::player1->weapstate[weap])
		{
			case WEAP_S_POWER:
			{
				amt = clamp(float(millis)/float(weaptype[weap].power), 0.f, 1.f);
				colourskew(r, g, b, 1.f-amt);
				break;
			}
			default: amt = 0; break;
		}
		glBindTexture(GL_TEXTURE_2D, t->getframe(amt));
		glColor4f(r, g, b, indicatorblend*hudblend);
		if(t->frames.length() > 1) drawsized(x-s/2, y-s/2, s);
		else drawslice(0, clamp(amt, 0.f, 1.f), x, y, s);
	}

    void drawclip(int weap, int x, int y, float s)
    {
    	if(!isweap(weap) || weap == WEAP_MELEE) return;
		const char *cliptexs[WEAP_MAX] = {
			"", pistolcliptex, shotguncliptex, smgcliptex,
			flamercliptex, plasmacliptex, riflecliptex, grenadecliptex, // end of regular weapons
			riflecliptex, ""
		};
		Texture *t = textureload(cliptexs[weap], 3);
		int ammo = game::player1->ammo[weap], maxammo = weaptype[weap].max;
		if(t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);

		const float clipskew[WEAP_MAX] = {
			0, pistolclipskew, shotgunclipskew, smgclipskew,
			flamerclipskew, plasmaclipskew, rifleclipskew, grenadeclipskew, // end of regular weapons
			rifleclipskew, 0
		};
		float fade = clipblend*hudblend, size = s*clipskew[weap];
		int interval = lastmillis-game::player1->weaplast[weap];
		if(interval <= game::player1->weapwait[weap]) switch(game::player1->weapstate[weap])
		{
			case WEAP_S_SHOOT:
			{
				float amt = 1.f-clamp(float(interval)/float(game::player1->weapwait[weap]), 0.f, 1.f); fade *= amt;
				if(showclips >= 2) size *= amt;
				break;
			}
			case WEAP_S_RELOAD:
			{
				if(game::player1->weapload[weap] > 0)
				{
					int check = game::player1->weapwait[weap]/2;
					if(interval >= check)
					{
						float amt = clamp(float(interval-check)/float(check), 0.f, 1.f); fade *= amt;
						if(showclips >= 2) size *= amt;
					}
					else
					{
						fade = 0.f;
						if(showclips >= 2) size = 0;
					}
					break;
				}
				// falls through
			}
			case WEAP_S_PICKUP: case WEAP_S_SWITCH:
			{
				float amt = clamp(float(interval)/float(game::player1->weapwait[weap]), 0.f, 1.f); fade *= amt;
				if(showclips >= 2 && game::player1->weapstate[weap] != WEAP_S_RELOAD) size *= amt;
				break;
			}
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
		glBindTexture(GL_TEXTURE_2D, t->id);
		if(interval <= game::player1->weapwait[weap]) switch(game::player1->weapstate[weap])
		{
			case WEAP_S_SHOOT:
			{
				int shot = game::player1->weapshot[weap] ? game::player1->weapshot[weap] : weaptype[weap].sub[0];
				if(shot) switch(weap)
				{
					case WEAP_FLAMER:
						drawslice(ammo/float(maxammo), shot/float(maxammo), x, y, size);
						break;
					case WEAP_GRENADE:
						drawslice(0.25f/maxammo+ammo/float(maxammo), shot/float(maxammo), x, y, size);
						break;
					default:
						drawslice(0.5f/maxammo+ammo/float(maxammo), shot/float(maxammo), x, y, size);
						break;
				}
				glColor4f(r, g, b, clipblend*hudblend);
				size = s*clipskew[weap];
				break;
			}
			case WEAP_S_RELOAD:
			{
				if(game::player1->weapload[weap] > 0)
				{
					ammo -= game::player1->weapload[weap];
					switch(weap)
					{
						case WEAP_FLAMER:
							drawslice(ammo/float(maxammo), game::player1->weapload[weap]/float(maxammo), x, y, size);
							break;
						case WEAP_GRENADE:
							drawslice(0.25f/maxammo+ammo/float(maxammo), game::player1->weapload[weap]/float(maxammo), x, y, size);
							break;
						default:
							drawslice(0.5f/maxammo+ammo/float(maxammo), game::player1->weapload[weap]/float(maxammo), x, y, size);
							break;
					}
					glColor4f(r, g, b, clipblend*hudblend);
					size = s*clipskew[weap];
				}
				break;
			}
			default: break;
		}
		if(ammo > 0) switch(weap)
		{
			case WEAP_FLAMER:
				drawslice(0, ammo/float(maxammo), x, y, size);
				break;
			case WEAP_GRENADE:
				drawslice(0.25f/maxammo, ammo/float(maxammo), x, y, size);
				break;
			default:
				drawslice(0.5f/maxammo, ammo/float(maxammo), x, y, size);
				break;
		}
    }

	void drawpointerindex(int index, int x, int y, int s, float r, float g, float b, float fade)
	{
		const char *tex = getpointer(index);
		Texture *t = tex && *tex ? textureload(tex, 3) : NULL;
		if(t && t != notexture)
		{
			if(t->bpp == 4) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else glBlendFunc(GL_ONE, GL_ONE);
			glColor4f(r, g, b, fade);
			glBindTexture(GL_TEXTURE_2D, t->id);
			drawsized(index == POINTER_GUI ? x : x-s/2, index == POINTER_GUI ? y : y-s/2, s);
		}
	}

	void drawpointer(int w, int h, int index)
	{
		int cs = int((index == POINTER_GUI ? cursorsize : crosshairsize)*hudsize);
		float r = 1, g = 1, b = 1, fade = (index == POINTER_GUI ? cursorblend : crosshairblend)*hudblend;
		if(index != POINTER_GUI && teamcrosshair >= (crosshairhealth ? 2 : 1)) skewcolour(r, g, b);
		if(game::player1->state == CS_ALIVE && index >= POINTER_HAIR)
		{
			if(index == POINTER_ZOOM && *zoomcrosshairtex && game::inzoom() && weaptype[game::player1->weapselect].zooms)
			{
				int frame = lastmillis-game::lastzoom, off = int(zoomcrosshairsize*hudsize)-cs;
				float amt = frame <= game::zoomtime ? clamp(float(frame)/float(game::zoomtime), 0.f, 1.f) : 1.f;
				if(!game::zooming) amt = 1.f-amt;
				cs += int(off*amt);
			}
			else if(crosshairhealth) healthskew(cs, r, g, b, fade, crosshairskew, crosshairhealth > 1);
		}
		int cx = int(hudwidth*cursorx), cy = int(hudsize*cursory), nx = int(hudwidth*0.5f), ny = int(hudsize*0.5f);
		drawpointerindex(index, game::mousestyle() != 1 ? cx : nx, game::mousestyle() != 1 ? cy : ny, cs, r, g, b, fade);
		if(index > POINTER_GUI)
		{
			if(game::player1->state == CS_ALIVE && game::player1->hasweap(game::player1->weapselect, m_weapon(game::gamemode, game::mutators)))
			{
				if(showclips) drawclip(game::player1->weapselect, nx, ny, clipsize*hudsize);
				if(showindicator && weaptype[game::player1->weapselect].power && game::player1->weapstate[game::player1->weapselect] == WEAP_S_POWER)
					drawindicator(game::player1->weapselect, nx, ny, int(indicatorsize*hudsize));
			}
			if(game::mousestyle() >= 1) // renders differently
				drawpointerindex(POINTER_RELATIVE, game::mousestyle() != 1 ? nx : cx, game::mousestyle() != 1 ? ny : cy, int(crosshairsize*hudsize), 1, 1, 1, crosshairblend*hudblend);
		}
	}

	void drawpointers(int w, int h)
	{
        int index = POINTER_NONE;
		if(UI::hascursor()) index = !UI::hascursor(true) || commandmillis > 0 ? POINTER_NONE : POINTER_GUI;
        else if(!showcrosshair || game::player1->state == CS_DEAD || !client::ready()) index = POINTER_NONE;
        else if(game::player1->state == CS_EDITING) index = POINTER_EDIT;
        else if(game::player1->state == CS_SPECTATOR || game::player1->state == CS_WAITING) index = POINTER_SPEC;
        else if(game::inzoom() && weaptype[game::player1->weapselect].zooms) index = POINTER_ZOOM;
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
			if(lastmillis-teamkills[i] <= teamkilltime) numkilled++;
			else teamkills.remove(i);
		}
		return numkilled;
	}

	void drawlast()
	{
		if(showhud)
		{
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, hudwidth, hudsize, 0, -1, 1);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			drawpointers(hudwidth, hudsize);
			if(shownotices && client::ready() && !UI::hascursor(false) && !texpaneltimer)
			{
				pushfont("super");
				int ty = (hudsize/2)-FONTH+int(hudsize/2*noticeoffset), tx = hudwidth/2, tf = int(255*hudblend*noticeblend), tr = 255, tg = 255, tb = 255,
					tw = hudwidth-(int(hudsize*gapsize)*2+int(hudsize*inventorysize)*2);
				if(noticescale < 1)
				{
					glPushMatrix();
					glScalef(noticescale, noticescale, 1);
					ty = int(ty*(1.f/noticescale));
					tx = int(tx*(1.f/noticescale));
				}
				if(teamnotices) skewcolour(tr, tg, tb);
				if(lastmillis-game::maptime <= titlefade*3)
				{

					ty += draw_textx("%s", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, tw, *maptitle ? maptitle : mapname)*noticescale;
					pushfont("default");
					if(*mapauthor) ty += draw_textx("by %s", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, tw, mapauthor)*noticescale;
					popfont();
					pushfont("sub");
					ty += draw_textx("[ \fs\fa%s\fS ]", tx, ty, 255, 255, 255, tf, TEXT_CENTERED, -1, tw, server::gamename(game::gamemode, game::mutators))*noticescale;
					popfont();
				}

				if(m_stf(game::gamemode)) stf::drawlast(hudwidth, hudsize, tx, ty, tf/255.f);
				else if(m_ctf(game::gamemode)) ctf::drawlast(hudwidth, hudsize, tx, ty, tf/255.f);

				if(game::player1->state == CS_DEAD || game::player1->state == CS_WAITING)
				{
					int sdelay = m_delay(game::gamemode, game::mutators), delay = game::player1->lastdeath ? game::player1->respawnwait(lastmillis, sdelay) : 0;
					const char *msg = game::player1->state != CS_WAITING && game::player1->lastdeath ? "Fragged" : "Please Wait";
					ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, msg)*noticescale;
					if(obitnotices && game::player1->lastdeath && (delay || game::player1->state == CS_DEAD) && *game::player1->obit)
					{
						pushfont("default");
						ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, game::player1->obit)*noticescale;
						popfont();
					}
					if(shownotices >= 2)
					{
						SEARCHBINDCACHE(attackkey)("action 0", 0);
						if(delay || m_campaign(game::gamemode) || (m_trial(game::gamemode) && !game::player1->lastdeath) || m_duke(game::gamemode, game::mutators))
						{
							if(!m_campaign(game::gamemode))
							{
								pushfont("emphasis");
								if(m_duke(game::gamemode, game::mutators))
									ty += draw_textx("Queued for new round", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1)*noticescale;
								else if(delay) ty += draw_textx("Down for \fs\fy%.1f\fS second(s)", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, delay/1000.f)*noticescale;
								popfont();
							}
							if(game::player1->state != CS_WAITING && shownotices >= 3 && lastmillis-game::player1->lastdeath >= 500)
							{
								pushfont("default");
								ty += draw_textx("Press \fs\fc%s\fS to look around", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, attackkey)*noticescale;
								popfont();
							}
						}
						else
						{
							pushfont("emphasis");
							ty += draw_textx("Ready to respawn", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1)*noticescale;
							popfont();
							if(game::player1->state != CS_WAITING && shownotices >= 3)
							{
								pushfont("default");
								ty += draw_textx("Press \fs\fc%s\fS to respawn", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, attackkey)*noticescale;
								popfont();
							}
						}
						if(game::player1->state == CS_WAITING && shownotices >= 3)
						{
							SEARCHBINDCACHE(waitmodekey)("waitmodeswitch", 3);
							pushfont("default");
							ty += draw_textx("Press \fs\fc%s\fS to %s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, waitmodekey, game::tvmode() ? "look around" : "observe")*noticescale;
							popfont();
						}
						if(m_arena(game::gamemode, game::mutators))
						{
							SEARCHBINDCACHE(arenakey)("showgui arena", 0);
							pushfont("default");
							ty += draw_textx("Press \fs\fc%s\fS to change arena weapon", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, arenakey)*noticescale;
							popfont();
						}
					}
				}
				else if(game::player1->state == CS_ALIVE)
				{
					if(teamkillnum && m_team(game::gamemode, game::mutators) && numteamkills() >= teamkillnum) ty += draw_textx("\fzryDon't shoot team mates", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1)*noticescale;
					if(inventoryteams)
					{
						if(game::player1->state == CS_ALIVE && !lastteam) lastteam = lastmillis;
						if(lastmillis-lastteam <= inventoryteams)
						{
							if(game::player1->team == TEAM_NEUTRAL)
							{
								if(m_campaign(game::gamemode)) ty += draw_textx("Campaign Mission", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1)*noticescale;
								else ty += draw_textx("\fzReFree-for-all Deathmatch", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, -1)*noticescale;
							}
							else ty += draw_textx("\fzReTeam \fs%s%s\fS \fs\fw(\fS\fs%s%s\fS\fs\fw)\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, teamtype[game::player1->team].chat, teamtype[game::player1->team].name, teamtype[game::player1->team].chat, teamtype[game::player1->team].colname)*noticescale;
						}
					}
					if(obitnotices && lastmillis-game::player1->lastkill <= noticetime && *game::player1->obit)
					{
						pushfont("default");
						ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, game::player1->obit)*noticescale;
						popfont();
					}
					if(shownotices >= 3 && game::allowmove(game::player1))
					{
						pushfont("default");
						static vector<actitem> actitems;
						actitems.setsizenodelete(0);
						if(entities::collateitems(game::player1, actitems))
						{
							SEARCHBINDCACHE(actionkey)("action 3", 0);
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
										int drop = -1, sweap = m_weapon(game::gamemode, game::mutators), attr = e.type == WEAPON ? w_attr(game::gamemode, e.attrs[0], sweap) : e.attrs[0];
										if(game::player1->canuse(e.type, attr, e.attrs, sweap, lastmillis, (1<<WEAP_S_RELOAD)|(1<<WEAP_S_SWITCH)))
										{
											if(e.type == WEAPON && w_carry(game::player1->weapselect, sweap) && game::player1->ammo[attr] < 0 &&
												w_carry(attr, sweap) && game::player1->carry(sweap) >= maxcarry) drop = game::player1->drop(sweap, attr);
											if(isweap(drop))
											{
												static vector<int> attrs; attrs.setsizenodelete(0); loopk(5) attrs.add(k ? 0 : drop);
												defformatstring(dropweap)("%s", entities::entinfo(WEAPON, attrs, false));
												ty += draw_textx("Press \fs\fc%s\fS to swap \fs%s\fS for \fs%s\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, actionkey, dropweap, entities::entinfo(e.type, e.attrs, false))*noticescale;
											}
											else ty += draw_textx("Press \fs\fc%s\fS to pickup \fs%s\fS", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, actionkey, entities::entinfo(e.type, e.attrs, false))*noticescale;
											break;
										}
									}
									else if(e.type == TRIGGER && e.attrs[2] == TA_ACTION)
									{
										ty += draw_textx("Press \fs\fc%s\fS to interact", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, actionkey)*noticescale;
										break;
									}
								}
								actitems.pop();
							}
						}
						if(shownotices >= 4)
						{
							if(game::player1->canshoot(game::player1->weapselect, 0, m_weapon(game::gamemode, game::mutators), lastmillis, (1<<WEAP_S_RELOAD)))
							{
								SEARCHBINDCACHE(attackkey)("action 0", 0);
								ty += draw_textx("Press \fs\fc%s\fS to attack", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, attackkey)*noticescale;
								SEARCHBINDCACHE(altkey)("action 1", 0);
								ty += draw_textx("Press \fs\fc%s\fS to %s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, altkey, weaptype[game::player1->weapselect].zooms ? "zoom" : "alt-attack")*noticescale;
							}
							if(game::player1->canreload(game::player1->weapselect, m_weapon(game::gamemode, game::mutators), lastmillis))
							{
								SEARCHBINDCACHE(reloadkey)("action 2", 0);
								ty += draw_textx("Press \fs\fc%s\fS to reload ammo", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, reloadkey)*noticescale;
							}
						}
						popfont();
					}
				}
				else if(game::player1->state == CS_SPECTATOR)
				{
					ty += draw_textx("%s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, game::tvmode() ? "SpecTV" : "Spectating")*noticescale;
					if(shownotices >= 2)
					{
						SEARCHBINDCACHE(speconkey)("spectator 0", 1);
						pushfont("default");
						ty += draw_textx("Press \fs\fc%s\fS to play", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, speconkey)*noticescale;
						if(shownotices >= 3)
						{
							SEARCHBINDCACHE(specmodekey)("specmodeswitch", 1);
							ty += draw_textx("Press \fs\fc%s\fS to %s", tx, ty, tr, tg, tb, tf, TEXT_CENTERED, -1, tw, specmodekey, game::tvmode() ? "look around" : "observe")*noticescale;
						}
						popfont();
					}
				}
				if(noticescale < 1) glPopMatrix();
				popfont();
			}
			if(client::ready() && (overlaydisplay >= 2 || (game::player1->state == CS_ALIVE && (overlaydisplay || !game::thirdpersonview(true)))))
			{
				Texture *t = *overlaytex ? textureload(overlaytex, 3) : notexture;
				if(t != notexture)
				{
					glBindTexture(GL_TEXTURE_2D, t->id);
					glColor4f(1.f, 1.f, 1.f, overlayblend*hudblend);
					drawtex(0, 0, hudwidth, hudsize);
				}
			}

			glDisable(GL_BLEND);
		}
	}

	void drawconsole(int type, int w, int h, int x, int y, int s)
	{
		static vector<int> refs; refs.setsizenodelete(0);
		bool full = fullconsole || commandmillis > 0;
		pushfont("console");
		if(type >= 2)
		{
			int numl = chatconsize, numo = chatconsize+chatconoverflow;
			if(numl)
			{
				loopvj(conlines) if(conlines[j].type >= CON_CHAT)
				{
					int len = !full && conlines[j].type > CON_CHAT ? chatcontime/2 : chatcontime;
					if(full || lastmillis-conlines[j].reftime <= len+chatconfade)
					{
						if(refs.length() >= numl)
						{
							if(!full)
							{
								bool found = false;
								loopvrev(refs) if(i > 0 && conlines[refs[i]].type > CON_CHAT)
								{
									if(refs.length() >= numo) refs.remove(i);
									else
									{
										int dur = conlines[refs[i]].type > CON_CHAT ? chatcontime/2 : chatcontime;
										if(lastmillis-conlines[refs[i]].reftime <= dur) conlines[refs[i]].reftime = lastmillis-dur;
									}
									found = true;
									break;
								}
								if(found)
								{
									refs.add(j);
									continue;
								}
							}
							if(refs.length() >= numo)
							{
								if(lastmillis-conlines[j].reftime <= len+chatconfade) conlines[j].reftime = lastmillis-(len+chatconfade);
								continue;
							}
							if(lastmillis-conlines[j].reftime <= len) conlines[j].reftime = lastmillis-len;
						}
						refs.add(j);
					}
				}
				int r = x+FONTW, z = y;
				loopvj(refs)
				{
					int len = !full && conlines[refs[j]].type > CON_CHAT ? chatcontime/2 : chatcontime;
					float f = full || !chatconfade ? 1.f : clamp(((len+chatconfade)-(lastmillis-conlines[refs[j]].reftime))/float(chatconfade), 0.f, 1.f),
						g = conlines[refs[j]].type > CON_CHAT ? conblend : chatconblend;
					z -= draw_textx("%s", r, z, 255, 255, 255, int(255*hudblend*f*g), TEXT_LEFT_UP, -1, s, conlines[refs[j]].cref)*f;
				}
			}
		}
		else
		{
			int numl = consize, numo = consize+conoverflow, z = y;
			if(numl)
			{
				loopvj(conlines) if(type ? conlines[j].type >= (confilter && !full ? CON_LO : 0) && conlines[j].type <= CON_HI : conlines[j].type >= (confilter && !full ? CON_LO : 0))
				{
					int len = !full && conlines[j].type < CON_IMPORTANT ? contime/2 : contime;
					if(conskip ? j>=conskip-1 || j>=conlines.length()-numl : full || lastmillis-conlines[j].reftime <= len+confade)
					{
						if(refs.length() >= numl)
						{
							if(!full && conlines[j].type >= CON_IMPORTANT)
							{
								bool found = false;
								loopvrev(refs) if(i > 0 && conlines[refs[i]].type < CON_IMPORTANT)
								{
									if(refs.length() >= numo) refs.remove(i);
									else
									{
										int dur = conlines[refs[i]].type < CON_IMPORTANT ? contime/2 : contime;
										if(lastmillis-conlines[refs[i]].reftime <= dur) conlines[refs[i]].reftime = lastmillis-dur;
									}
									found = true;
									break;
								}
								if(found)
								{
									refs.add(j);
									continue;
								}
							}
							if(refs.length() >= numo)
							{
								if(lastmillis-conlines[j].reftime <= len+confade) conlines[j].reftime = lastmillis-(len+confade);
								continue;
							}
							if(lastmillis-conlines[j].reftime <= len) conlines[j].reftime = lastmillis-len;
						}
						refs.add(j);
					}
				}
				loopvrev(refs)
				{
					int len = !full && conlines[refs[i]].type < CON_IMPORTANT ? contime/2 : contime;
					float f = full || !confade ? 1.f : clamp(((len+confade)-(lastmillis-conlines[refs[i]].reftime))/float(confade), 0.f, 1.f),
						g = full || conlines[refs[i]].type >= CON_IMPORTANT ? fullconblend : conblend;
					z += draw_textx("%s", concenter ? x+s/2 : x, z, 255, 255, 255, int(255*hudblend*f*g), concenter ? TEXT_CENTERED : TEXT_LEFT_JUSTIFY, -1, s, conlines[refs[i]].cref)*f;
				}
			}
			if(commandmillis > 0)
			{
				pushfont("command");
				Texture *t = textureload(commandicon ? commandicon : inputtex, 3);
				float f = float(lastmillis%1000)/1000.f;
				if(f < 0.5f) f = 1.f-f;
				glBindTexture(GL_TEXTURE_2D, t->id);
				glColor4f(1.f, 1.f, 1.f, fullconblend*hudblend*f);
				drawtex(x, z, FONTH, FONTH);
				z += draw_textx("%s", (concenter ? x+s/2-FONTW*3 : x)+(FONTH+FONTW), z, 255, 255, 255, int(255*fullconblend*hudblend), concenter ? TEXT_CENTERED : TEXT_LEFT_JUSTIFY, commandpos >= 0 ? commandpos : strlen(commandbuf), s-(FONTH+FONTW), commandbuf);
				popfont();
			}
		}
		popfont();
	}

	float radarrange()
	{
		float dist = radardist ? radardist : getworldsize();
		if(game::player1->state == CS_EDITING && editradardist) dist = editradardist;
		return dist;
	}

	void drawblip(const char *tex, float area, int w, int h, float s, float blend, vec &dir, float r, float g, float b, const char *font, const char *text, ...)
	{
		float yaw = -(float)atan2(dir.x, dir.y)/RAD + 180, x = sinf(RAD*yaw), y = -cosf(RAD*yaw),
			size = max(w, h)/2, tx = w/2, ty = h/2, ts = size*radarsize, tp = ts*s, tq = tp/2, tr = (size*radaroffset)+(ts*area);
		vec pos = vec(tx+(tr*x), ty+(tr*y), 0);
		settexture(tex, 3);
		glColor4f(r, g, b, blend);
		glBegin(GL_QUADS);
		loopk(4)
		{
			vec norm;
			switch(k)
			{
				case 0: vecfromyawpitch(yaw, 0, 1, -1, norm);	glTexCoord2f(1, 0); break;
				case 1: vecfromyawpitch(yaw, 0, 1, 1, norm);	glTexCoord2f(0, 0); break;
				case 2: vecfromyawpitch(yaw, 0, -1, 1, norm);	glTexCoord2f(0, 1); break;
				case 3: vecfromyawpitch(yaw, 0, -1, -1, norm);	glTexCoord2f(1, 1); break;
			}
			norm.z = 0; norm.normalize().mul(tq).add(pos);
			glVertex2f(norm.x, norm.y);
		}
		glEnd();
		if(text && *text)
		{
			if(font && *font) pushfont(font);
			defvformatstring(str, text, text);
			pos.x -= x*tq; pos.y -= y*tq*0.75f;
			if(y > 0)
			{
				int width, height; text_bounds(str, width, height, -1, TEXT_CENTERED|TEXT_NO_INDENT);
				pos.y -= y*height;
			}
			draw_textx("%s", int(pos.x), int(pos.y), 255, 255, 255, int(blend*255.f), TEXT_CENTERED|TEXT_NO_INDENT, -1, -1, str);
			if(font && *font) popfont();
		}
	}

	void drawplayerblip(gameent *d, int w, int h, float blend)
	{
		vec dir = d->headpos();
		dir.sub(camera1->o);
		float dist = dir.magnitude();
		if(dist <= radarrange())
		{
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();
			int colour = teamtype[d->team].colour;
			float fade = clamp(1.f-(dist/radarrange()), 0.f, 1.f), r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f;
			if(d->state == CS_DEAD || d->state == CS_WAITING)
			{
				int millis = d->lastdeath ? lastmillis-d->lastdeath : 0;
				if(millis > 0)
				{
					int len = m_delay(game::gamemode, game::mutators);
					if(!len && d->aitype >= AI_START) len = ai::aideadfade;
					if(len > 0) fade *= clamp(float(len-millis)/float(len), 0.f, 1.f);
					else return;
				}
				else return;
			}
			else
			{
				int len = m_protect(game::gamemode, game::mutators), millis = d->protect(lastmillis, len);
				if(millis > 0) fade *= clamp(float(len-millis)/float(len), 0.f, 1.f);
			}
			if(chkcond(radarplayernames, game::tvmode()))
				drawblip(bliptex, 4, w, h, radarplayersize*fade, fade*blend*radarplayerblend, dir, r, g, b, "radar", "%s", game::colorname(d, NULL, "", false));
			else drawblip(bliptex, 4, w, h, radarplayersize, fade, dir, r, g, b);
		}
	}

	void drawcardinalblips(int w, int h, float blend, bool altcard)
	{
		loopi(4)
		{
			const char *card = "";
			vec dir(0, 0, 0);
			switch(i)
			{
				case 0: dir = vec(0, -1, 0); card = altcard ? "0" : "N"; break;
				case 1: dir = vec(1, 0, 0); card = altcard ? "90" : "E"; break;
				case 2: dir = vec(0, 1, 0); card = altcard ? "180" : "S"; break;
				case 3: dir = vec(-1, 0, 0); card = altcard ? "270" : "W"; break;
				default: break;
			}
			if(!dir.iszero())
			{
				dir.rotate_around_z(-camera1->yaw*RAD);
				dir.normalize();
				float yaw = -(float)atan2(dir.x, dir.y)/RAD + 180, x = sinf(RAD*yaw), y = -cosf(RAD*yaw),
					size = max(w, h)/2, tx = w/2, ty = h/2, ts = size*radarsize, tp = ts*radarcardsize, tr = (size*radaroffset)+(ts*4);
				pushfont("radar");
				draw_textx("%s", int(tx+(tr*x)+(tp*0.5f)), int(ty+(tr*y)-(FONTH*y*0.5f)), 255, 255, 255, int(blend*radarcardblend*255.f), TEXT_CENTERED|TEXT_NO_INDENT, -1, -1, card);
				popfont();
			}
		}
	}

	void drawentblip(int w, int h, float blend, int n, vec &o, int type, vector<int> &attr, bool spawned, int lastspawn, bool insel)
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
			const char *tex = bliptex;
			float r = 1.f, g = 1.f, b = 1.f, fade = insel ? 1.f : clamp(1.f-(dist/radarrange()), 0.1f, 1.f), size = radarblipsize;
			if(type == WEAPON)
			{
				int attr1 = w_attr(game::gamemode, attr[0], m_weapon(game::gamemode, game::mutators));
				tex = itemtex(WEAPON, attr1);
				r = (weaptype[attr1].colour>>16)/255.f;
				g = ((weaptype[attr1].colour>>8)&0xFF)/255.f;
				b = (weaptype[attr1].colour&0xFF)/255.f;
				fade *= radaritemblend;
				size = radaritemsize;
			}
			else fade *= radarblipblend;
			if(game::player1->state != CS_EDITING && !insel && inspawn > 0.f)
				fade = radaritemspawn ? 1.f-inspawn : fade+((1.f-fade)*(1.f-inspawn));
			if(insel) drawblip(tex, 2, w, h, size, fade*blend, dir, r, g, b, "radar", "%s %s", enttype[type].name, entities::entinfo(type, attr, insel));
			else if(chkcond(radaritemnames, game::tvmode())) drawblip(tex, 2, w, h, size, fade*blend, dir, r, g, b, "radar", "%s", entities::entinfo(type, attr, false));
			else drawblip(tex, 2, w, h, size, fade*blend, dir, r, g, b);
		}
	}

	void drawentblips(int w, int h, float blend)
	{
		if(m_edit(game::gamemode) && game::player1->state == CS_EDITING)
		{
			int hover = !entities::ents.inrange(enthover) && !entgroup.empty() ? entgroup[0] : -1;
			loopv(entgroup) if(entities::ents.inrange(entgroup[i]) && entgroup[i] != hover)
			{
				gameentity &e = *(gameentity *)entities::ents[entgroup[i]];
				drawentblip(w, h, blend, entgroup[i], e.o, e.type, e.attrs, e.spawned, e.lastspawn, true);
			}
			if(entities::ents.inrange(hover))
			{
				gameentity &e = *(gameentity *)entities::ents[hover];
				drawentblip(w, h, blend, hover, e.o, e.type, e.attrs, e.spawned, e.lastspawn, true);
			}
		}
		else
		{
			loopi(entities::lastusetype[EU_ITEM])
			{
				gameentity &e = *(gameentity *)entities::ents[i];
				drawentblip(w, h, blend, i, e.o, e.type, e.attrs, e.spawned, e.lastspawn, false);
			}
			loopv(projs::projs) if(projs::projs[i]->projtype == PRJ_ENT && projs::projs[i]->ready())
			{
				projent &proj = *projs::projs[i];
				if(entities::ents.inrange(proj.id))
					drawentblip(w, h, blend, -1, proj.o, entities::ents[proj.id]->type, entities::ents[proj.id]->attrs, true, proj.spawntime, false);
			}
		}
	}

	void drawdamageblips(int w, int h, float blend)
	{
		loopv(damagelocs)
		{
			damageloc &d = damagelocs[i];
			int millis = lastmillis-d.outtime;
			if(millis >= radardamagetime+radardamagefade) { damagelocs.remove(i--); continue; }
			if(game::player1->state == CS_ALIVE || (game::player1->state == CS_DEAD && game::player1->lastdeath))
			{
				float fade = clamp(max(d.damage, radardamagemin)/float(max(radardamagemax-radardamagemin, 1)), radardamagemin/100.f, 1.f),
					size = clamp(fade*radardamagesize, min(radardamagesize*radardamagemin/100.f, 1.f), radardamagesize);
				if(millis >= radardamagetime) fade *= 1.f-(float(millis-radardamagetime)/float(radardamagefade));
				else fade *= float(millis)/float(radardamagetime);
				vec dir = vec(d.dir).normalize().rotate_around_z(-camera1->yaw*RAD);
				if(radardamage >= 5)
				{
					gameent *a = game::getclient(d.attacker);
					drawblip(arrowtex, 4+size, w, h, size, blend*radardamageblend*fade, dir, d.colour.x, d.colour.y, d.colour.z, "radar", "%s +%d", a ? game::colorname(a) : "?", d.damage);
				}
				else drawblip(arrowtex, 4+size, w, h, size, blend*radardamageblend*fade, dir, d.colour.x, d.colour.y, d.colour.z);
			}
		}
		if(radardamage >= 2)
		{
			bool dead = (game::player1->state == CS_DEAD || game::player1->state == CS_WAITING) && game::player1->lastdeath;
			if(dead && lastmillis-game::player1->lastdeath <= m_delay(game::gamemode, game::mutators))
			{
				vec dir = vec(game::player1->o).sub(camera1->o).normalize().rotate_around_z(-camera1->yaw*RAD);
				float r = (teamtype[game::player1->team].colour>>16)/255.f, g = ((teamtype[game::player1->team].colour>>8)&0xFF)/255.f, b = (teamtype[game::player1->team].colour&0xFF)/255.f;
				drawblip(arrowtex, 4+radardamagetrack, w, h, radardamagetrack, blend*radardamageblend, dir, r, g, b, "radar", "you");
			}
			gameent *a = game::getclient(game::player1->lastattacker);
			if(a && a != game::player1 && (dead || (radardamage >= 3 && (a->aitype < 0 || radardamage >= 4))))
			{
				vec pos = vec(a->o).sub(camera1->o).normalize(), dir = vec(pos).rotate_around_z(-camera1->yaw*RAD);
				float r = (teamtype[a->team].colour>>16)/255.f, g = ((teamtype[a->team].colour>>8)&0xFF)/255.f, b = (teamtype[a->team].colour&0xFF)/255.f;
				if(dead && (a->state == CS_ALIVE || a->state == CS_DEAD || a->state == CS_WAITING))
				{
					if(a->state == CS_ALIVE) drawblip(arrowtex, 4+radardamagetrack, w, h, radardamagetrack, blend*radardamageblend, dir, r, g, b, "radar", "%s (%d)", game::colorname(a), a->health);
					else drawblip(arrowtex, 4+radardamagetrack, w, h, radardamagetrack, blend*radardamageblend, dir, r, g, b, "radar", "%s", game::colorname(a));
				}
			}
		}
	}

	void drawradar(int w, int h, float blend)
	{
		if(chkcond(radaritems, game::tvmode()) || m_edit(game::gamemode)) drawentblips(w, h, blend*radarblend); // 2
		if(chkcond(radarflags, game::tvmode())) // 3
		{
			if(m_stf(game::gamemode)) stf::drawblips(w, h, blend);
			else if(m_ctf(game::gamemode)) ctf::drawblips(w, h, blend*radarblend);
		}
		if(chkcond(radarplayers, m_campaign(game::gamemode) || m_duke(game::gamemode, game::mutators) || m_edit(game::gamemode) || game::tvmode())) // 4
		{
			loopv(game::players) if(game::players[i] && game::players[i]->state != CS_SPECTATOR && game::players[i]->aitype <= AI_BOT && (!m_campaign(game::gamemode) || game::players[i]->team == game::player1->team))
				drawplayerblip(game::players[i], w, h, blend*radarblend);
		}
		if(chkcond(radarcard, game::tvmode()) || (editradarcard && m_edit(game::gamemode))) drawcardinalblips(w, h, blend*radarblend, m_edit(game::gamemode)); // 4
		if(radardamage) drawdamageblips(w, h, blend*radarblend); // 5+
	}

	int drawprogress(int x, int y, float start, float length, float size, bool left, float r, float g, float b, float fade, float skew, const char *font, const char *text, ...)
	{
		if(skew <= 0.f) return 0;
		float q = clamp(skew, 0.f, 1.f), f = fade*q, cr = r*q, cg = g*q, cb = b*q, s = size*skew, cs = int(s)/2, cx = left ? x+cs : x-cs, cy = y-cs;
		settexture(progresstex, 3);
		glColor4f(cr, cg, cb, f);
		if(length < 1) drawslice(start, length, cx, cy, cs);
		else drawsized(left ? x : x-int(s), y-int(s), int(s));
		if(text && *text)
		{
			glPushMatrix();
			glScalef(skew, skew, 1);
			if(font && *font) pushfont(font);
			int tx = int(cx*(1.f/skew)), ty = int((cy-FONTH/2)*(1.f/skew)), ti = int(255.f*f);
			defvformatstring(str, text, text);
			draw_textx("%s", tx, ty, 255, 255, 255, ti, TEXT_CENTERED, -1, -1, str);
			if(font && *font) popfont();
			glPopMatrix();
		}
		return int(s);
	}

	int drawitem(const char *tex, int x, int y, float size, bool left, float r, float g, float b, float fade, float skew, const char *font, const char *text, ...)
	{
		if(skew <= 0.f) return 0;
		float q = clamp(skew, 0.f, 1.f), f = fade*q, cr = r*q, cg = g*q, cb = b*q, s = size*skew;
		settexture(tex, 3);
		glColor4f(cr, cg, cb, f);
		drawsized(left ? x : x-int(s), y-int(s), int(s));
		if(text && *text)
		{
			glPushMatrix();
			glScalef(skew, skew, 1);
			if(font && *font) pushfont(font);
			int tx = int((left ? (x+s) : x)*(1.f/skew)), ty = int((y-s+s/32)*(1.f/skew)), ti = int(255.f*f);
			defvformatstring(str, text, text);
			draw_textx("%s", tx, ty, 255, 255, 255, ti, TEXT_RIGHT_JUSTIFY, -1, -1, str);
			if(font && *font) popfont();
			glPopMatrix();
		}
		return int(s);
	}

	int drawitemsubtext(int x, int y, float size, int align, float skew, const char *font, float blend, const char *text, ...)
	{
		if(skew <= 0.f) return 0;
		glPushMatrix();
		glScalef(skew, skew, 1);
		if(font && *font) pushfont(font);
		int sy = int(FONTH*skew), tx = int(x*(1.f/skew)), ty = int((y-size/32)*(1.f/skew)), ti = int(255.f*blend*clamp(skew, 0.f, 1.f));
		defvformatstring(str, text, text);
		draw_textx("%s", tx, ty, 255, 255, 255, ti, align, -1, -1, str);
		if(font && *font) popfont();
		glPopMatrix();
		return sy;
	}

	const char *teamtex(int team)
	{
		const char *teamtexs[TEAM_MAX] = {
			neutraltex, alphatex, betatex, gammatex, deltatex, neutraltex
		};
		return teamtexs[team];
	}

	const char *itemtex(int type, int stype)
	{
		switch(type)
		{
			case WEAPON:
			{
				const char *weaptexs[WEAP_MAX] = {
					meleetex, pistoltex, shotguntex, smgtex, flamertex, plasmatex, rifletex, grenadetex, rifletex, ""
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
			const char *itext = itemtex(e.type, e.attrs[0]);
			int ty = drawitem(itext && *itext ? itext : inventoryenttex, x, y, s, false, 1.f, 1.f, 1.f, fade*inventoryblend, skew, "default", "%s (%d)", enttype[e.type].name, n);
			mkstring(attrstr);
			loopi(enttype[e.type].numattrs)
			{
				defformatstring(s)("%s%d", i ? " " : "", e.attrs[i]);
				concatstring(attrstr, s);
			}
			if(attrstr[0]) drawitemsubtext(x, y-int(s/3*skew), s, TEXT_RIGHT_UP, skew, "default", fade*inventoryblend, "%s", attrstr);
			drawitemsubtext(x, y, s, TEXT_RIGHT_UP, skew, "default", fade*inventoryblend, "%s", entities::entinfo(e.type, e.attrs, true));
			return ty;
		}
		return 0;
	}

	int drawselection(int x, int y, int s, int m, float blend)
	{
		int sy = 0;
		if(game::player1->state == CS_ALIVE)
		{
			if(inventoryammo)
			{
				const char *hudtexs[WEAP_MAX] = {
					meleetex, pistoltex, shotguntex, smgtex, flamertex, plasmatex, rifletex, grenadetex, rifletex, ""
				};
				int sweap = m_weapon(game::gamemode, game::mutators);
				loopi(WEAP_MAX) if(game::player1->hasweap(i, sweap) || lastmillis-game::player1->weaplast[i] <= game::player1->weapwait[i])
				{
					if(y-sy-s < m) break;
					float fade = blend*inventoryblend, size = s, skew = 0.f;
					if(game::player1->weapstate[i] == WEAP_S_SWITCH || game::player1->weapstate[i] == WEAP_S_PICKUP)
					{
						float amt = clamp(float(lastmillis-game::player1->weaplast[i])/float(game::player1->weapwait[i]), 0.f, 1.f);
						if(i != game::player1->weapselect) skew = game::player1->hasweap(i, sweap) ? 1.f-(amt*(1.f-inventoryskew)) : 1.f-amt;
						else skew = game::player1->weapstate[i] == WEAP_S_PICKUP ? amt : inventoryskew+(amt*(1.f-inventoryskew));
					}
					else if(game::player1->hasweap(i, sweap)) skew = i != game::player1->weapselect ? inventoryskew : 1.f;
					else continue;
					bool instate = (i == game::player1->weapselect || game::player1->weapstate[i] != WEAP_S_PICKUP);
					float r = 1.f, g = 1.f, b = 1.f;
					if(teamwidgets >= (inventorycolour ? 2 : 1)) skewcolour(r, g, b);
					else if(inventorycolour)
					{
						r = (weaptype[i].colour>>16)/255.f;
						g = ((weaptype[i].colour>>8)&0xFF)/255.f;
						b = (weaptype[i].colour&0xFF)/255.f;
					}
					int oldy = y-sy;
					if(inventoryammo && (instate || inventoryammo > 1) && i != WEAP_MELEE && game::player1->hasweap(i, sweap))
						sy += drawitem(hudtexs[i], x, y-sy, size, false, r, g, b, fade, skew, "super", "%d", game::player1->ammo[i]);
					else sy += drawitem(hudtexs[i], x, y-sy, size, false, r, g, b, fade, skew);
					if(inventoryweapids && (instate || inventoryweapids > 1))
					{
						static string weapids[WEAP_MAX];
						static int lastweapids = -1;
						if(lastweapids != changedkeys)
						{
							loopj(WEAP_MAX)
							{
								defformatstring(action)("weapon %d", j);
								const char *actkey = searchbind(action, 0);
								if(actkey && *actkey) copystring(weapids[j], actkey);
								else formatstring(weapids[j])("%d", j+1);
							}
							lastweapids = changedkeys;
						}
						drawitemsubtext(x, oldy, size, TEXT_RIGHT_UP, skew, "sub", fade, "%s%s", inventorycolour >= 2 ? weaptype[i].text : "\fa", weapids[i]);
					}
				}
			}
		}
		else if(game::player1->state == CS_EDITING && inventoryedit)
		{
			int hover = entities::ents.inrange(enthover) ? enthover : (!entgroup.empty() ? entgroup[0] : -1);
			if(y-sy-s >= m) sy += drawitem(inventoryedittex, x, y-sy, s-s/4, false, 1.f, 1.f, 1.f, blend*inventoryblend*0.25f, 1.f);
			if(y-sy-s >= m) sy += drawentitem(hover, x, y-sy, s, 1.f, blend*inventoryeditblend);
			loopv(entgroup) if(entgroup[i] != hover)
			{
				if(y-sy-s < m) break;
				sy += drawentitem(entgroup[i], x, y-sy, s, inventoryeditskew, blend*inventoryeditblend);
			}
		}
		return sy;
	}

	int drawhealth(int x, int y, int s, float blend)
	{
        int size = s+s/2, width = s-s/4, glow = int(width*inventoryhealthglow), sy = 0, sw = width+s/16, heal = m_health(game::gamemode, game::mutators);
		float fade = inventoryhealthblend*blend;
		settexture(healthtex, 3);
		if(game::player1->state == CS_ALIVE)
		{
			bool pulse = inventoryhealthflash > 0 && game::player1->health < heal;
			if(inventoryhealth && (glow || pulse))
			{
				int gap = 0;
				float r = 1.f, g = 1.f, b = 1.f, bgfade = game::player1->lastspawn && lastmillis-game::player1->lastspawn <= 1000 ? (lastmillis-game::player1->lastspawn)/2000.f : 0.5f;
				if(teamwidgets) skewcolour(r, g, b);
				if(pulse)
				{
					int timestep = lastmillis%1000;
					float skew = clamp((timestep <= 500 ? timestep/500.f : (1000-timestep)/500.f)*(float(heal-game::player1->health)/float(heal))*inventoryhealthflash, 0.f, 1.f);
					r += (1.f-r)*skew;
					g -= g*skew;
					b -= b*skew;
					bgfade += (1.f-bgfade)*skew;
					gap += int(x*skew);
				}
				glColor4f(r, g, b, fade*bgfade);
				drawtex(x-gap, y-size-gap, width+gap*2, size+gap*2);
				sy += size;
			}
			if(game::player1->lastspawn && lastmillis-game::player1->lastspawn <= 1000) fade *= (lastmillis-game::player1->lastspawn)/1000.f;
			else if(inventoryhealththrob > 0 && regentime && game::player1->lastregen && lastmillis-game::player1->lastregen <= regentime)
			{
				float amt = clamp((lastmillis-game::player1->lastregen)/float(regentime/2), 0.f, 2.f);
				glow = int(glow*(amt > 1.f ? amt-1.f : 1.f-amt)*inventoryhealththrob);
			}
			if(inventoryhealth >= 2)
			{
				const struct healthbarstep
				{
					float health, r, g, b;
				} steps[] = { { 0, 0.75f, 0, 0 }, { 0.35f, 1, 0.5f, 0 }, { 0.65f, 1, 1, 0 }, { 1, 0, 1, 0 } };
				glBegin(GL_QUAD_STRIP);
				int cx = x+glow, cy = y-size+glow, cw = width-glow*2, ch = size-glow*2;
				float health = clamp(game::player1->health/float(heal), 0.0f, 1.0f);
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
							glColor4f(r, g, b, fade); glTexCoord2f(0, hoff); glVertex2f(cx, cy + hoff*ch);
							glColor4f(r, g, b, fade); glTexCoord2f(1, hoff); glVertex2f(cx + cw, cy + hoff*ch);
						}
						if(step.health > health + margin)
						{
							float hoff = 1 - (health + margin), hlerp = (health + margin - steps[i-1].health) / (step.health - steps[i-1].health),
								  r = step.r*hlerp + steps[i-1].r*(1-hlerp),
								  g = step.g*hlerp + steps[i-1].g*(1-hlerp),
								  b = step.b*hlerp + steps[i-1].b*(1-hlerp);
							glColor4f(r, g, b, 0); glTexCoord2f(0, hoff); glVertex2f(cx, cy + hoff*ch);
							glColor4f(r, g, b, 0); glTexCoord2f(1, hoff); glVertex2f(cx + cw, cy + hoff*ch);
							break;
						}
					}
					float off = 1 - step.health, hfade = fade, r = step.r, g = step.g, b = step.b;
					if(step.health > health) hfade *= 1 - (step.health - health)/margin;
					glColor4f(r, g, b, hfade); glTexCoord2f(0, off); glVertex2f(cx, cy + off*ch);
					glColor4f(r, g, b, hfade); glTexCoord2f(1, off); glVertex2f(cx + cw, cy + off*ch);
				}
				glEnd();
				if(!sy) sy += size;
			}
			if(inventoryhealth)
			{
				pushfont("super");
				int dt = draw_textx("%d", x+width/2, y-sy, 255, 255, 255, int(fade*255), TEXT_CENTERED, -1, -1, max(game::player1->health, 0));
				if(!sy) sy += dt;
				popfont();
			}
			if(FWV(impulsestyle) && FWV(impulsemeter) && inventoryimpulse)
			{
				float len = 1.f-clamp(game::player1->impulse[IM_METER]/float(FWV(impulsemeter)), 0.f, 1.f);
				settexture(progresstex, 3);
				float r = 1.f, g = 1.f, b = 1.f;
				if(teamwidgets) skewcolour(r, g, b);
				glColor4f(r, g, b, fade*0.25f);
				drawsized(x, y-sy-sw, sw);
				glColor4f(r, g, b, fade);
				drawslice(0, len, x+sw/2, y-sy-sw/2, sw/2);
				if(inventoryimpulse >= 2)
				{
					pushfont("sub");
					draw_textx("%s%d%%", x+sw/2, y-sy-sw/2-FONTH/2, 255, 255, 255, int(fade*255), TEXT_CENTERED, -1, -1,
						game::player1->impulse[IM_METER] > 0 ? (FWV(impulsemeter)-game::player1->impulse[IM_METER] > FWV(impulsecost) ? "\fy" : "\fw") : "\fg",
							int(len*100));
					popfont();
				}
				sy += sw;
			}
			else if(inventorystatus >= 2) sy += drawitem(teamtex(game::player1->team), x, y-sy, sw, true, 1.f, 1.f, 1.f, blend*inventoryblend, 1.f);
		}
		else
		{
			fade *= 0.5f;
			const char *state = "", *tex = "";
			switch(game::player1->state)
			{
				case CS_EDITING: state = "EDIT"; tex = inventorychattex; break;
				case CS_SPECTATOR: state = "SPEC"; tex = inventorychattex; break;
				case CS_WAITING: state = "WAIT"; tex = inventorywaittex; break;
				case CS_DEAD: state = "DEAD"; tex = inventorydeadtex; break;
			}
			if(inventoryhealth >= 3 && *state)
			{
				pushfont("super");
				sy += draw_textx("%s", x+width/2, y-sy, 255, 255, 255, int(fade*255)/2, TEXT_CENTER_UP, -1, -1, state);
				popfont();
			}
			if(inventorystatus && *tex) sy += drawitem(tex, x, y-sy, sw, true, 1.f, 1.f, 1.f, fade, 1.f);
		}
		if(inventorytrial && m_trial(game::gamemode) && game::player1->state != CS_EDITING && game::player1->state != CS_SPECTATOR)
		{
			if((game::player1->cpmillis > 0 || game::player1->cptime) && (game::player1->state == CS_ALIVE || game::player1->state == CS_DEAD || game::player1->state == CS_WAITING))
			{
				pushfont("default");
				if(game::player1->cpmillis > 0)
					sy += draw_textx("%s", x, y-sy, 255, 255, 255, int(fade*255), TEXT_LEFT_UP, -1, -1, timetostr(lastmillis-game::player1->cpmillis, true));
				else if(game::player1->cplast)
					sy += draw_textx("\fzwe%s", x, y-sy, 255, 255, 255, int(fade*255), TEXT_LEFT_UP, -1, -1, timetostr(game::player1->cplast));
				popfont();
				if(game::player1->cptime)
				{
					pushfont("sub");
					sy += draw_textx("\fy%s", x, y-sy, 255, 255, 255, int(fade*255), TEXT_LEFT_UP, -1, -1, timetostr(game::player1->cptime));
					popfont();
				}
			}
			sy += sb.trialinventory(x, y-sy, sw, fade);
		}
		return sy;
	}

	void drawinventory(int w, int h, int edge, float blend)
	{
		int cx[2] = { edge, w-edge }, cy[2] = { h-edge, h-edge }, cs = int(inventorysize*w), cr = cs/8, cc = 0;
		loopi(2) switch(i)
		{
			case 0: default:
			{
				if((cc = drawhealth(cx[i], cy[i], cs, blend)) > 0) cy[i] -= cc+cr;
				break;
			}
			case 1:
			{
				int cm = edge;
				if(!texpaneltimer)
				{
					cy[i] -= showfps || showstats > (m_edit(game::gamemode) ? 0 : 1) ? cs/2 : cs/16;
					if(inventoryteams && game::player1->state != CS_EDITING && game::player1->state != CS_SPECTATOR)
					{
						if(game::player1->state == CS_ALIVE && !lastteam) lastteam = lastmillis;
						if(lastnewgame)
						{
							if(!game::intermission) lastnewgame = 0;
							else
							{
								int millis = votelimit-(lastmillis-lastnewgame);
								float amt = float(millis)/float(votelimit);
								const char *col = "\fw";
								if(amt > 0.75f) col = "\fg";
								else if(amt > 0.5f) col = "\fy";
								else if(amt > 0.25f) col = "\fo";
								else col = "\fr";
								hud::drawprogress(cx[i], cm+cs, 0, 1, cs, false, 1, 1, 1, blend*inventoryblend*0.25f, 1);
								cm += hud::drawprogress(cx[i], cm+cs, 0, amt, cs, false, 1, 1, 1, blend*inventoryblend, 1, "default", "%s%.1f", col, millis/1000.f);
							}
						}
						if(!lastnewgame && lastteam)
						{
							const char *pre = "";
							float skew = inventoryskew, fade = blend*inventoryblend, rescale = 1;
							int millis = lastmillis-lastteam, pos[2] = { cx[i], cm+int(cs*inventoryskew) };
							if(millis <= inventoryteams)
							{
								pre = "\fzRw";
								int off[2] = { hudwidth/2, hudsize/4 };
								if(millis <= inventoryteams/2)
								{
									float tweak = millis <= inventoryteams/4 ? clamp(float(millis)/float(inventoryteams/4), 0.f, 1.f) : 1.f;
									skew += tweak*inventorygrow;
									loopk(2) pos[k] = off[k]+(cs/2*tweak*skew);
									skew *= tweak; fade *= tweak; rescale = 0;
								}
								else
								{
									float tweak = clamp(float(millis-(inventoryteams/2))/float(inventoryteams/2), 0.f, 1.f);
									skew += (1.f-tweak)*inventorygrow;
									loopk(2) pos[k] -= int((pos[k]-(off[k]+cs/2*skew))*(1.f-tweak));
									rescale = tweak;
								}
							}
							cm += int(drawitem(teamtex(game::player1->team), pos[0], pos[1], cs, false, 1, 1, 1, fade, skew)*rescale);
							if(game::player1->team == TEAM_NEUTRAL)
							{
								if(m_campaign(game::gamemode)) cm += int(drawitemsubtext(pos[0]-int(cs*skew/2), pos[1], cs, TEXT_CENTERED, skew, "default", fade, "%s%scampaign", teamtype[game::player1->team].chat, pre)*rescale);
								else cm += int(drawitemsubtext(pos[0]-int(cs*skew/2), pos[1], cs, TEXT_CENTERED, skew, "default", fade, "%s%sffa", teamtype[game::player1->team].chat, pre)*rescale);
							}
							else cm += int(drawitemsubtext(pos[0]-int(cs*skew/2), pos[1], cs, TEXT_CENTERED, skew, "default", fade, "%s%s%s", teamtype[game::player1->team].chat, pre, teamtype[game::player1->team].name)*rescale);
						}
					}
					if((cc = drawselection(cx[i], cy[i], cs, cm, blend)) > 0) cy[i] -= cc+cr;
					if(!m_edit(game::gamemode) && inventoryscore && ((cc = sb.drawinventory(cx[i], cy[i], cs, cm, blend)) > 0)) cy[i] -= cc+cr;
					if(inventorygame)
					{
						if(m_stf(game::gamemode) && ((cc = stf::drawinventory(cx[i], cy[i], cs, cm, blend)) > 0)) cy[i] -= cc+cr;
						else if(m_ctf(game::gamemode) && ((cc = ctf::drawinventory(cx[i], cy[i], cs, cm, blend)) > 0)) cy[i] -= cc+cr;
					}
				}
				break;
			}
		}
	}

	void drawdamage(int w, int h, int s, float blend)
	{
		float pc = game::player1->state == CS_DEAD ? 0.5f : (game::player1->state == CS_ALIVE ? min(damageresidue, 100)/100.f : 0.f);
		if(pc > 0)
		{
			Texture *t = *damagetex ? textureload(damagetex, 3) : notexture;
			if(t != notexture)
			{
				glBindTexture(GL_TEXTURE_2D, t->id);
				glColor4f(0.85f, 0.09f, 0.09f, pc*blend*damageblend);
				drawtex(0, 0, w, h);
			}
		}
	}

	void drawfire(int w, int h, int s, float blend)
	{
		int interval = game::player1->lastfire ? lastmillis-game::player1->lastfire : 0;
		if(interval && interval < fireburntime)
		{
			Texture *t = *burntex ? textureload(burntex, 3) : notexture;
			if(t != notexture)
			{
				float pc = interval >= fireburntime-500 ? 1.f+(interval-(fireburntime-500))/500.f : (interval%fireburndelay)/float(fireburndelay/2); if(pc > 1.f) pc = 2.f-pc;
				glBindTexture(GL_TEXTURE_2D, t->id);
				glColor4f(0.9f*max(pc,0.5f), 0.3f*pc, 0.0625f*max(pc,0.25f), blend*burnblend*(interval >= fireburntime-(fireburndelay/2) ? pc : min(pc+0.5f, 1.f)));
				drawtex(0, 0, w, h);
			}
		}
	}

	void drawzoom(int w, int h)
	{
		Texture *t = textureload(zoomtex, 3);
		int frame = lastmillis-game::lastzoom;
		float pc = frame <= game::zoomtime ? float(frame)/float(game::zoomtime) : 1.f;
		if(!game::zooming) pc = 1.f-pc;
		int x = 0, y = 0, c = 0;
		if(w > h)
		{
			float rc = 1.f-pc;
			c = h; x += (w-h)/2;
			usetexturing(false);
			drawblend(0, 0, x, c, rc, rc, rc, true);
			drawblend(x+c, 0, x+1, c, rc, rc, rc, true);
			usetexturing(true);
		}
		else if(h > w)
		{
			float rc = 1.f-pc;
			c = w; y += (h-w)/2;
			usetexturing(false);
			drawblend(0, 0, c, y, rc, rc, rc, true);
			drawblend(0, y+c, c, y, rc, rc, rc, true);
			usetexturing(true);
		}
		else c = h;
		glBindTexture(GL_TEXTURE_2D, t->id);
		glColor4f(1.f, 1.f, 1.f, pc);
		drawtex(x, y, c, c);
	}

	void drawbackground(int w, int h)
	{
		settexture("textures/logo", 3);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(w-512, 0);
		glTexCoord2f(1, 0); glVertex2f(w, 0);
		glTexCoord2f(1, 1); glVertex2f(w, 128);
		glTexCoord2f(0, 1); glVertex2f(w-512, 128);
		glEnd();

		settexture("textures/cube2badge", 3);
		glBegin(GL_QUADS); // goes off the edge on purpose
		glTexCoord2f(0, 0); glVertex2f(w-208, 96);
		glTexCoord2f(1, 0); glVertex2f(w-16, 96);
		glTexCoord2f(1, 1); glVertex2f(w-16, 192);
		glTexCoord2f(0, 1); glVertex2f(w-208, 192);
		glEnd();

		pushfont("sub");
		int y = h-FONTH/2;
		if(loadbackinfo && *loadbackinfo) y -= draw_textx("%s", FONTH/2, y, 255, 255, 255, 255, TEXT_LEFT_UP, -1, w*2, loadbackinfo);
		if(progressing)
		{
			if(*progresstext) y -= draw_textx("%s %s [\fs\fa%d%%\fS]", FONTH/2, y, 255, 255, 255, 255, TEXT_LEFT_UP, -1, w*2, *progresstitle ? progresstitle : "please wait...", progresstext, int(progresspart*100));
			else y -= draw_textx("%s", FONTH/2, y, 255, 255, 255, 255, TEXT_LEFT_UP, -1, w*2, *progresstitle ? progresstitle : "please wait...");
		}
		y = h-FONTH/2;
		y -= draw_textx("%s", w-FONTH/2, y, 255, 255, 255, 255, TEXT_RIGHT_UP, -1, w, ENG_URL);
		y -= draw_textx("v%.2f (%s)", w-FONTH, y, 255, 255, 255, 255, TEXT_RIGHT_UP, -1, w, float(ENG_VERSION)/100.f, ENG_RELEASE);
		popfont();
	}

	void drawheadsup(int w, int h, float fade, int os, int is, int br, int bs, int bx, int by)
	{
		if(underlaydisplay >= 2 || (game::player1->state == CS_ALIVE && (underlaydisplay || !game::thirdpersonview(true))))
		{
			Texture *t = *underlaytex ? textureload(underlaytex, 3) : notexture;
			if(t != notexture)
			{
				glBindTexture(GL_TEXTURE_2D, t->id);
				glColor4f(1.f, 1.f, 1.f, underlayblend*hudblend);
				drawtex(0, 0, w, h);
			}
		}
		if(game::player1->state == CS_ALIVE && game::inzoom() && weaptype[game::player1->weapselect].zooms) drawzoom(w, h);
		if(showdamage)
		{
			if(fireburntime && game::player1->state == CS_ALIVE) drawfire(w, h, os, fade);
			if(!kidmode && game::bloodscale > 0) drawdamage(w, h, os, fade);
		}
		if(!UI::hascursor() && (game::player1->state == CS_EDITING ? showeditradar > 0 : chkcond(showradar, game::tvmode()))) drawradar(w, h, fade);
		if(showinventory) drawinventory(w, h, os, fade);

		if(!texpaneltimer)
		{
			int bf = int(255*fade*statblend);
			pushfont("sub");
			bx -= FONTW;
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
				draw_textx("%d", w-(br+FONTW)/2, by-FONTH*2, 255, 255, 255, bf, TEXT_CENTERED, -1, bs, curstats[8]);
				draw_textx("fps", w-(br+FONTW)/2, by-FONTH, 255, 255, 255, bf, TEXT_CENTERED, -1, -1);
				switch(showfps)
				{
					case 3:
						if(autoadjust) by -= draw_textx("min:%d max:%d range:+%d-%d bal:\fs%s%d\fS%%", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, minfps, maxfps, curstats[9], curstats[10], curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
						else by -= draw_textx("max:%d range:+%d-%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, maxfps, curstats[9], curstats[10]);
						break;
					case 2:
						if(autoadjust) by -= draw_textx("min:%d max:%d, bal:\fs%s%d\fS%% %dfps", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, minfps, maxfps, curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
						else by -= draw_textx("max:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, maxfps);
						break;
					default: break;
				}
			}
			if(showstats > (m_edit(game::gamemode) ? 0 : 1))
			{
				by -= draw_textx("ond:%d va:%d gl:%d(%d) oq:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6]);
				by -= draw_textx("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
				by -= draw_textx("ents:%d(%d) lm:%d rp:%d pvs:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs, entities::ents.length(), entgroup.length(), lightmaps.length(), curstats[7], getnumviewcells());
				if(game::player1->state == CS_EDITING)
				{
					by -= draw_textx("cube:%s%d corner:%d orient:%d grid:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs,
							selchildcount<0 ? "1/" : "", abs(selchildcount), sel.corner, sel.orient, sel.grid);
					by -= draw_textx("sel:%d,%d,%d %d,%d,%d (%d,%d,%d,%d)", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs,
							sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z,
								sel.cx, sel.cxs, sel.cy, sel.cys);
					by -= draw_textx("pos:%d,%d,%d yaw:%d pitch:%d", bx, by, 255, 255, 255, bf, TEXT_RIGHT_UP, -1, bs,
							(int)game::player1->o.x, (int)game::player1->o.y, (int)game::player1->o.z,
							(int)game::player1->yaw, (int)game::player1->pitch);
				}
			}
			popfont();
		}
	}

	void drawhud(bool noview)
	{
		float fade = hudblend;
		if(!progressing)
		{
			vec colour = vec(1, 1, 1);
			if(commandfade && (commandmillis > 0 || lastmillis-(commandmillis > 0 ? commandmillis : -commandmillis) <= commandfade))
			{
				float a = min(float(lastmillis-(commandmillis > 0 ? commandmillis : -commandmillis))/float(commandfade), 1.f)*commandfadeamt;
				if(commandmillis > 0) a = 1.f-a;
				else a += (1.f-commandfadeamt);
				loopi(3) if(a < colour[i]) colour[i] *= a;
			}
			if(huduioverride >= (game::intermission ? 2 : 1) && uifade && (uimillis > 0 || lastmillis-(uimillis > 0 ? uimillis : -uimillis) <= uifade))
			{
				float n = min(float(lastmillis-(uimillis > 0 ? uimillis : -uimillis))/float(uifade), 1.f), a = n*uifadeamt;
				if(uimillis > 0) a = 1.f-a;
				else a += (1.f-uifadeamt);
				loopi(3) if(a < colour[i]) colour[i] *= a;
				//if(UI::hascursor(true)) fade *= uimillis > 0 ? 1.f-n : n;
			}
			if(!noview)
			{
				if(titlefade && (!client::ready() || game::maptime <= 0 || lastmillis-game::maptime <= titlefade))
				{
					float a = client::ready() && game::maptime > 0 ? float(lastmillis-game::maptime)/float(titlefade) : 0.f;
					loopi(3) if(a < colour[i]) colour[i] *= a;
				}
				if(tvmodefade && game::tvmode())
				{
					float a = game::lasttvchg ? (lastmillis-game::lasttvchg <= tvmodefade ? float(lastmillis-game::lasttvchg)/float(tvmodefade) : 1.f) : 0.f;
					loopi(3) if(a < colour[i]) colour[i] *= a;
				}
				if(spawnfade && game::player1->state == CS_ALIVE && game::player1->lastspawn && lastmillis-game::player1->lastspawn <= spawnfade)
				{
					float a = (lastmillis-game::player1->lastspawn)/float(spawnfade/3);
					if(a < 3.f)
					{
						vec col = vec(1, 1, 1); skewcolour(col.x, col.y, col.z, true);
						if(a < 1.f) { loopi(3) col[i] *= a; }
						else { a = (a-1.f)*0.5f; loopi(3) col[i] += (1.f-col[i])*a; }
						loopi(3) if(col[i] < colour[i]) colour[i] *= col[i];
					}
				}
				if(showdamage >= 2 && damageresidue > 0)
				{
					float pc = min(damageresidue, 100)/100.f;
					loopi(2) if(colour[i+1] > 0) colour[i+1] -= colour[i+1]*pc;
				}
			}
			if(colour.x < 1 || colour.y < 1 || colour.z < 1)
			{
				usetexturing(false);
				drawblend(0, 0, screen->w, screen->h, colour.x, colour.y, colour.z);
				usetexturing(true);
				fade *= min(colour.x, min(colour.y, colour.z));
			}
		}

		int gap = int(hudsize*gapsize), inv = int(hudsize*inventorysize), br = inv+gap*2, bs = (hudwidth-br*2)/2, bx = hudwidth-br, by = hudsize-gap;
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glLoadIdentity();
		glOrtho(0, hudwidth, hudsize, 0, -1, 1);
		glColor3f(1, 1, 1);

		if(noview) drawbackground(hudwidth, hudsize);
		else if(showhud && client::ready() && fade > 0) drawheadsup(hudwidth, hudsize, fade, gap, inv, br, bs, bx, by);
		if(UI::ready && showconsole)
		{
			drawconsole(showconsole >= 2 ? 1 : 0, hudwidth, hudsize, gap, gap, hudwidth-gap*2);
			if(showconsole >= 2 && !noview && !progressing)
				drawconsole(2, hudwidth, hudsize, br, by, showfps > 1 || showstats > (m_edit(game::gamemode) ? 0 : 1) ? bs-gap : (bs-gap)*2);
		}

		glDisable(GL_BLEND);
	}

	void gamemenus() { sb.show(); }

	void update(int w, int h)
	{
        aspect = w/float(h);
        fovy = 2*atan2(tan(curfov/2*RAD), aspect)/RAD;
		hudwidth = int(ceil(hudsize*aspect));
	}
}
