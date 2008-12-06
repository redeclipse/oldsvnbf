#include "pch.h"
#include "engine.h"
#include "game.h"
namespace hud
{
	int hudwidth = 0;
	scoreboard sb;

	VARP(showdamage, 0, 1, 1);
	TVAR(damagetex, "textures/damage", 0);
	VARP(showindicator, 0, 1, 1);
	FVARP(indicatorsize, 0, 0.08f, 1);
	FVARP(indicatorblend, 0, 0.5f, 1);
	TVAR(indicatortex, "textures/indicator", 3);
	TVAR(snipetex, "textures/snipe", 0);

	VARP(showcrosshair, 0, 1, 1);
	FVARP(crosshairsize, 0, 0.07f, 1);
	VARP(crosshairhitspeed, 0, 450, INT_MAX-1);
	FVARP(crosshairblend, 0, 0.5f, 1);
	VARP(crosshairhealth, 0, 1, 1);
	FVARP(crosshairskew, -1, 0.4f, 1);
	TVAR(relativecursortex, "textures/cursordot", 3);
	TVAR(guicursortex, "textures/cursor", 3);
	TVAR(editcursortex, "textures/cursordot", 3);
	TVAR(speccursortex, "textures/cursordot", 3);
	TVAR(crosshairtex, "textures/crosshair", 3);
	TVAR(teamcrosshairtex, "textures/teamcrosshair", 3);
	TVAR(hitcrosshairtex, "textures/hitcrosshair", 3);
	TVAR(snipecrosshairtex, "textures/snipecrosshair", 3);
	FVARP(snipecrosshairsize, 0, 0.5f, 1);
	FVARP(cursorsize, 0, 0.05f, 1);
	FVARP(cursorblend, 0, 1.f, 1);

	VARP(showinventory, 0, 1, 1);
	FVARP(inventorysize, 0, 0.075f, 1);
	FVARP(inventoryblend, 0, 0.75f, 1);
	FVARP(inventoryskew, 0, 0.7f, 1);
	FVARP(inventorytextscale, 0, 1.25f, 1);
	FVARP(inventorytextblend, 0, 1.f, 1);
	TVAR(plasmatex, "textures/plasma", 0);
	TVAR(shotguntex, "textures/shotgun", 0);
	TVAR(chainguntex, "textures/chaingun", 0);
	TVAR(grenadestex, "textures/grenades", 0);
	TVAR(flamertex, "textures/flamer", 0);
	TVAR(carbinetex, "textures/carbine", 0);
	TVAR(rifletex, "textures/rifle", 0);
	TVAR(neutralflagtex, "textures/team", 0);
	TVAR(alphaflagtex, "textures/teamalpha", 0);
	TVAR(betaflagtex, "textures/teambeta", 0);
	TVAR(deltaflagtex, "textures/teamdelta", 0);
	TVAR(gammaflagtex, "textures/teamgamma", 0);

	VARP(showclip, 0, 1, 1);
	FVARP(clipsize, 0, 0.06f, 1);
	FVARP(clipblend, 0, 0.25f, 1);
	TVAR(plasmacliptex, "textures/plasmaclip", 3);
	TVAR(shotguncliptex, "textures/shotgunclip", 3);
	TVAR(chainguncliptex, "textures/chaingunclip", 3);
	TVAR(grenadescliptex, "textures/grenadesclip", 3);
	TVAR(flamercliptex, "textures/flamerclip", 3);
	TVAR(carbinecliptex, "textures/carbineclip", 3);
	TVAR(riflecliptex, "textures/rifleclip", 3);

	VARP(showradar, 0, 1, 1);
	TVAR(radartex, "textures/radar", 3);
	FVARP(radarblend, 0, 0.25f, 1);
	FVARP(radarcardblend, 0, 0.75f, 1);
	FVARP(radaritemblend, 0, 0.95f, 1);
	FVARP(radarnameblend, 0, 1.f, 1);
	FVARP(radarblipblend, 0, 1.f, 1);
	FVARP(radarsize, 0, 0.025f, 1);
	VARP(radardist, 0, 256, INT_MAX-1);
	VARP(radarcard, 0, 1, 1);
	VARP(radaritems, 0, 2, 2);
	VARP(radarnames, 0, 1, 1);
	VARP(radarhealth, 0, 1, 1);
	FVARP(radarskew, -1, -0.3f, 1);
	VARP(editradardist, 0, 128, INT_MAX-1);
	VARP(editradarnoisy, 0, 1, 2);

	//VARP(showtips, 0, 2, 3);
	//VARP(showenttips, 0, 1, 2);
	//VARP(showhudents, 0, 10, 100);

	VARP(hudsize, 0, 2000, INT_MAX-1);

	VARP(titlecardtime, 0, 2000, 10000);
	VARP(titlecardfade, 0, 3000, 10000);
	FVARP(titlecardsize, 0, 0.3f, 1);

	VARP(showstats, 0, 0, 1);
	VARP(statrate, 0, 200, 1000);
	VARP(showfps, 0, 2, 2);

	void colourskew(float &r, float &g, float &b, float skew)
	{
		if(skew > 0.75f)
		{ // fade to orange
			float off = (skew-0.75f)*4.f;
			g *= 0.5f+(off*0.5f);
			b *= off;
		}
		else if(skew > 0.25f)
		{ // fade to red
			g *= skew-0.25f;
			b = 0.f;
		}
		else
		{ // fade out
			r *= 0.5f+(skew*2.f);
			g = b = 0.f;
		}
	}

	void healthskew(int &s, float &r, float &g, float &b, float &fade, float ss)
	{
		if(world::player1->lastregen && lastmillis-world::player1->lastregen < REGENTIME)
		{
			float skew = clamp((lastmillis-world::player1->lastregen)/float(REGENTIME), 0.f, 1.f);
			if(skew > 0.5f) skew = 1.f-skew;
			fade += (1.f-fade)*skew;
			s += int(s*ss*skew);
		}

		if(world::player1->health < MAXHEALTH)
			colourskew(r, g, b, clamp(float(world::player1->health)/float(MAXHEALTH), 0.f, 1.f));
	}

	enum
	{
		POINTER_NONE = 0,
		POINTER_RELATIVE,
		POINTER_GUI,
		POINTER_EDIT,
		POINTER_SPEC,
		POINTER_HAIR,
		POINTER_TEAM,
		POINTER_HIT,
		POINTER_SNIPE,
		POINTER_MAX
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

	void drawindicator(int gun, int x, int y, int s)
	{
		Texture *t = textureload(indicatortex, 3);
		if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
		int millis = lastmillis-world::player1->gunlast[gun];
		float r = 1.f, g = 1.f, b = 1.f, amt = 0.f;
		switch(world::player1->gunstate[gun])
		{
			case GNS_POWER:
			{
				if(millis > guntype[gun].power)
				{
					float skew = clamp(float(millis-guntype[gun].power)/float(guntype[gun].time), 0.f, 1.f);
					glBindTexture(GL_TEXTURE_2D, t->getframe(skew));
					glColor4f(r, g, b, indicatorblend);
					if(t->frames.length() > 1) drawsized(x-s*3/4, y-s*3/4, s*3/2);
					else drawslice(0, clamp(skew, 0.f, 1.f), x, y, s*3/2);
					colourskew(r, g, b, 1.f-skew);
					amt = 1.f;
				}
				else amt = clamp(float(millis)/float(guntype[gun].power), 0.f, 1.f);
				break;
			}
			default:
			{
				float skew = clamp(float(millis)/float(world::player1->gunwait[gun]), 0.f, 1.f);
				amt *= skew;
				break;
			}
		}
		glBindTexture(GL_TEXTURE_2D, t->getframe(amt));
		glColor4f(r, g, b, indicatorblend);
		if(t->frames.length() > 1) drawsized(x-s/2, y-s/2, s);
		else drawslice(0, clamp(amt, 0.f, 1.f), x, y, s);
	}

    void drawclip(int gun, int x, int y, int s)
    {
        const char *cliptexs[GUN_MAX] = {
            plasmacliptex, shotguncliptex, chainguncliptex,
            flamercliptex, carbinecliptex, riflecliptex, grenadescliptex,
        };
        Texture *t = textureload(cliptexs[gun], 3);
        int ammo = world::player1->ammo[gun], maxammo = guntype[gun].max;
		if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);

		float fade = clipblend;
		if(lastmillis-world::player1->gunlast[gun] < world::player1->gunwait[gun]) switch(world::player1->gunstate[gun])
		{
			case GNS_RELOAD: case GNS_PICKUP: case GNS_SWITCH:
			{
				fade *= clamp(float(lastmillis-world::player1->gunlast[gun])/float(world::player1->gunwait[gun]), 0.f, 1.f);
				break;
			}
			default: break;
		}
        glColor4f(1.f, 1.f, 1.f, fade);
        glBindTexture(GL_TEXTURE_2D, t->retframe(ammo, maxammo));
        if(t->frames.length() > 1) drawsized(x-s/2, y-s/2, s);
        else switch(gun)
        {
            case GUN_FLAMER:
				drawslice(0, max(ammo-min(maxammo-ammo, 2), 0)/float(maxammo), x, y, s);
				if(world::player1->ammo[gun] < guntype[gun].max)
					drawfadedslice(max(ammo-min(maxammo-ammo, 2), 0)/float(maxammo),
						min(min(maxammo-ammo, ammo), 2) /float(maxammo),
							x, y, s, fade);
                break;

            default:
                drawslice(0.5f/maxammo, ammo/float(maxammo), x, y, s);
                break;
        }
    }

	void drawpointer(int index, int x, int y, int s, float r, float g, float b, float fade)
	{
		Texture *t = textureload(getpointer(index), 3, true);
		if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
		glColor4f(r, g, b, fade);
		glBindTexture(GL_TEXTURE_2D, t->id);
		bool guicursor = index == POINTER_GUI;
		drawsized(guicursor ? x : x-s/2, guicursor ? y : y-s/2, s);
	}

	void drawpointers(int w, int h)
	{
        int index = POINTER_NONE;

		if(UI::hascursor()) index = UI::hascursor(true) ? POINTER_GUI : POINTER_NONE;
        else if(hidehud || !showcrosshair || world::player1->state == CS_DEAD || !connected()) index = POINTER_NONE;
        else if(world::player1->state == CS_EDITING) index = POINTER_EDIT;
        else if(world::player1->state == CS_SPECTATOR || world::player1->state == CS_WAITING) index = POINTER_SPEC;
        else if(world::inzoom() && world::player1->gunselect == GUN_RIFLE) index = POINTER_SNIPE;
        else if(lastmillis-world::lasthit <= crosshairhitspeed) index = POINTER_HIT;
        else if(m_team(world::gamemode, world::mutators))
        {
            vec pos = world::headpos(world::player1, 0.f);
            gameent *d = world::intersectclosest(pos, worldpos, world::player1);
            if(d && d->type == ENT_PLAYER && d->team == world::player1->team)
				index = POINTER_TEAM;
			else index = POINTER_HAIR;
        }
        else index = POINTER_HAIR;

        if(index > POINTER_NONE)
        {
        	bool guicursor = index == POINTER_GUI;
        	int cs = int((guicursor ? cursorsize : crosshairsize)*hudsize);
			float r = 1.f, g = 1.f, b = 1.f, fade = guicursor ? cursorblend : crosshairblend;

        	if(world::player1->state == CS_ALIVE && index >= POINTER_HAIR)
        	{
				if(index == POINTER_SNIPE)
				{
					cs = int(snipecrosshairsize*hudsize);
					if(world::inzoom() && world::player1->gunselect == GUN_RIFLE)
					{
						int frame = lastmillis-world::lastzoom;
						float amt = frame < world::zoomtime() ? clamp(float(frame)/float(world::zoomtime()), 0.f, 1.f) : 1.f;
						if(!world::zooming) amt = 1.f-amt;
						cs = int(cs*amt);
					}
				}
				if(crosshairhealth) healthskew(cs, r, g, b, fade, crosshairskew);
        	}

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glOrtho(0, hudwidth, hudsize, 0, -1, 1);

			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);

			float x = aimx, y = aimy;
			if(index < POINTER_EDIT || world::mousestyle() == 2)
			{
				x = cursorx;
				y = cursory;
			}
			else if(world::isthirdperson() ? world::thirdpersonaim : world::firstpersonaim)
				x = y = 0.5f;

			int cx = int(hudwidth*x), cy = int(hudsize*y);

			drawpointer(index, cx, cy, cs, r, g, b, fade);

			if(index > POINTER_GUI)
			{
				if(world::player1->state == CS_ALIVE)
				{
					if(world::player1->hasgun(world::player1->gunselect))
					{
						if(showclip) drawclip(world::player1->gunselect, cx, cy, int(clipsize*hudsize));
						if(showindicator && guntype[world::player1->gunselect].power && world::player1->gunstate[world::player1->gunselect] == GNS_POWER)
							drawindicator(world::player1->gunselect, cx, cy, int(indicatorsize*hudsize));
					}
				}

				if(world::mousestyle() >= 1) // renders differently
					drawpointer(POINTER_RELATIVE, int(hudwidth*(world::mousestyle()==1?cursorx:0.5f)), int(hudsize*(world::mousestyle()==1?cursory:0.5f)), int(crosshairsize*hudsize), 1.f, 1.f, 1.f, crosshairblend);
			}

			glDisable(GL_BLEND);
			glEnable(GL_DEPTH_TEST);
        }
	}

	void drawquad(int x, int y, int w, int h, float tx1, float ty1, float tx2, float ty2)
	{
		glBegin(GL_QUADS);
		glTexCoord2f(tx1, ty1); glVertex2f(x, y);
		glTexCoord2f(tx2, ty1); glVertex2f(x+w, y);
		glTexCoord2f(tx2, ty2); glVertex2f(x+w, y+h);
		glTexCoord2f(tx1, ty2); glVertex2f(x, y+h);
		glEnd();
	}

	void drawtex(int x, int y, int w, int h, float tx, float ty, float tw, float th)
	{
		drawquad(x, y, w, h, tx, ty, tx+tw, ty+th);
	}

	void drawsized(int x, int y, int s)
	{
		drawtex(x, y, s, s);
	}

	float radarrange()
	{
		float dist = float(radardist);
		if(world::player1->state == CS_EDITING) dist = float(editradardist);
		return dist;
	}

	void drawblip(int w, int h, int s, float blend, int idx, vec &dir, float r, float g, float b, const char *font, float fade, const char *text, ...)
	{
		const struct rddir { bool axis, swap; float x, y, up, down; } rddirs[8] =
		{
			{ true,		false,	 1.f,	0.f,	0.5f,	0.5f	},
			{ false,	true,	 1.f,	1.f,	0.f,	0.5f 	},
			{ false,	true,	 1.f,	2.f,	0.5f,	0.5f 	},
			{ true,		true,	 2.f,	2.f,	1.f,	-0.5f	},
			{ true,		true,	 3.f,	2.f,	0.5f,	-0.5f	},
			{ false,	false,	 3.f,	3.f,	1.f,	-0.5f	},
			{ false,	false,	 3.f,	4.f,	0.5f,	-0.5f	},
			{ true,		false,	 4.f,	4.f,	0.f,	0.5f	}
		};

		int cx = s/2, cy = s/2;
		float yaw = 0.f, pitch = 0.f;
		vectoyawpitch(dir, yaw, pitch);
		float fovsx = curfov*0.5f, fovsy = (360.f-(curfov*2.f))*0.25f;
		int cq = 7;
		for(int cr = 0; cr < 8; cr++) if(yaw < (fovsx*rddirs[cr].x)+(fovsy*rddirs[cr].y))
		{
			cq = cr;
			break;
		}
		const rddir &rd = rddirs[cq];
		float range = rd.axis ? fovsx : fovsy, skew = (yaw-(((fovsx*rd.x)+(fovsy*rd.y))-range))/range;
		if(rd.swap) (rd.axis ? cy : cx) += (rd.axis ? h : w)-s*2;
		(rd.axis ? cx : cy) += int(((rd.axis ? w : h)-s*2)*clamp(rd.up+(rd.down*skew), 0.f, 1.f));

		settexture(radartex, 3);
		glColor4f(r, g, b, blend);
		const float rdblip[4][2] = { // blip dark darker flag
			 { 0.25f, 0.25f }, { 0.25f, 0.5f }, { 0.5f, 0.5f }, { 0.5f, 0.25f }
		};
		drawtex(cx, cy, s, s, rdblip[idx][0], rdblip[idx][1], 0.25f, 0.25f);
		if(text && *text)
		{
			if(font && *font) pushfont(font);
			int tx = rd.axis ? int(cx+s*0.5f) : (rd.swap ? int(cx-s) : int(cx+s*2.f)),
				ty = rd.axis ? (rd.swap ? int(cy-s-FONTH) : int(cy+s*2.f)) : int(cy+s*0.5f-FONTH*0.5f),
				ta = rd.axis ? AL_CENTER : (rd.swap ? AL_RIGHT : AL_LEFT),
				tr = int(r*255.f), tg = int(g*255.f), tb = int(b*255.f),
				tf = int((fade >= 0.f ? fade : blend)*255.f);
			s_sprintfdlv(str, text, text);
			draw_textx("%s", tx, ty, tr, tg, tb, tf, false, ta, -1, -1, str);
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
			int colour = teamtype[d->team].colour;
			float fade = clamp(1.f-(dist/radarrange()), 0.f, 1.f)*blend,
				r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f;
			if(lastmillis-d->lastspawn <= REGENWAIT)
				fade *= clamp(float(lastmillis-d->lastspawn)/float(REGENWAIT), 0.f, 1.f);
			const char *text = radarnames ? world::colorname(d, NULL, "", false) : NULL;
			drawblip(w, h, s, fade*radarblipblend, 0, dir, r, g, b, "radar", fade*radarnameblend, text);
		}
	}

	void drawcardinalblips(int w, int h, int s, float blend)
	{
		loopi(4)
		{
			const char *card = "";
			vec dir(camera1->o);
			switch(i)
			{
				case 0: dir.sub(vec(0, 1, 0)); card = "N"; break;
				case 1: dir.add(vec(1, 0, 0)); card = "E"; break;
				case 2: dir.add(vec(0, 1, 0)); card = "S"; break;
				case 3: dir.sub(vec(1, 0, 0)); card = "W"; break;
				default: break;
			}
			dir.sub(camera1->o);
			dir.rotate_around_z(-camera1->yaw*RAD);
			dir.normalize();
			drawblip(w, h, s, blend*radarblipblend, 2, dir, 1.f, 1.f, 1.f, "hud", blend*radarcardblend, "%s", card);
		}
	}

	void drawentblip(int w, int h, int s, float blend, int n, vec &o, int type, int attr1, int attr2, int attr3, int attr4, int attr5, bool spawned, int lastspawn)
	{
		if(type > NOTUSED && type < MAXENTTYPES && ((enttype[type].usetype == EU_ITEM && spawned) || world::player1->state == CS_EDITING))
		{
			bool insel = world::player1->state == CS_EDITING && entities::ents.inrange(n) && (enthover == n || entgroup.find(n) >= 0);
			float inspawn = spawned && lastspawn && lastmillis-lastspawn <= 1000 ? float(lastmillis-lastspawn)/1000.f : 0.f;
			if(enttype[type].noisy && (world::player1->state != CS_EDITING || !editradarnoisy || (editradarnoisy < 2 && !insel)))
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
			int cp = 1;
			float r = 0.2f, g = 0.1f, b = 0.7f, fade = clamp(1.f-(dist/radarrange()), 0.1f, 1.f)*blend;
			if(insel) { cp = 0; fade = 1.f; r = 0.6f; g = 0.4f; b = 1.f; }
			else if(inspawn > 0.f)
			{
				cp = 0;
				r = 1.f-(inspawn*0.8f);
				g = 1.f-(inspawn*0.9f);
				b = 1.f-(inspawn*0.3f);
				fade = clamp(fade+((1.f-fade)*(1.f-inspawn)), 0.f, 1.f);
			}
			string text;
			if(m_edit(world::gamemode))
				s_sprintf(text)("%s\n%s", enttype[type].name, entities::entinfo(type, attr1, attr2, attr3, attr4, attr5, insel));
			else if(radaritems > 1)
				s_sprintf(text)("%s", entities::entinfo(type, attr1, attr2, attr3, attr4, attr5, false));
			drawblip(w, h, s, fade*radarblipblend, cp, dir, r, g, b, "radar", fade*radaritemblend, "%s", text);
		}
	}

	void drawentblips(int w, int h, int s, float blend)
	{
		loopv(entities::ents)
		{
			gameentity &e = *(gameentity *)entities::ents[i];
			drawentblip(w, h, s, blend, i, e.o, e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, e.spawned, e.lastspawn);
		}

		loopv(projs::projs) if(projs::projs[i]->projtype == PRJ_ENT && projs::projs[i]->ready())
		{
			projent &proj = *projs::projs[i];
			if(entities::ents.inrange(proj.id))
				drawentblip(w, h, s, blend, -1, proj.o, proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4, proj.attr5, true, proj.spawntime);
		}
	}

	void drawtitlecard(int w, int h)
	{
		int ox = hudwidth, oy = hudsize, os = showradar ? int(oy*radarsize*1.5f) : 0;
		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);
		pushfont("emphasis");

		int bs = int(oy*titlecardsize), bp = int(oy*0.01f), bx = ox-bs-bp-os, by = bp+os,
			secs = world::maptime ? lastmillis-world::maptime : 0;
		float fade = hudblend, amt = 1.f;

		if(secs < titlecardtime)
		{
			amt = clamp(float(secs)/float(titlecardtime), 0.f, 1.f);
			fade = clamp(amt*fade, 0.f, 1.f);
		}
		else if(secs < titlecardtime+titlecardfade)
			fade = clamp(fade*(1.f-(float(secs-titlecardtime)/float(titlecardfade))), 0.f, 1.f);

		const char *title = getmaptitle();
		if(!*title) title = getmapname();

		int rs = int(bs*amt), rx = bx+(bs-rs), ry = by;
		glColor4f(1.f, 1.f, 1.f, fade*0.9f);
		if(!rendericon(getmapname(), rx, ry, rs, rs))
			rendericon("textures/emblem", rx, ry, rs, rs);
		glColor4f(1.f, 1.f, 1.f, fade);
		rendericon(guioverlaytex, rx, ry, rs, rs);

		int tx = bx + bs, ty = by + bs + FONTH/2, ts = int(tx*(1.f-amt));
		ty += draw_textx("%s", tx-ts, ty, 255, 255, 255, int(255.f*fade), false, AL_RIGHT, -1, tx-FONTH, server::gamename(world::gamemode, world::mutators));
		ty += draw_textx("%s", tx-ts, ty, 255, 255, 255, int(255.f*fade), false, AL_RIGHT, -1, tx-FONTH, title);
		popfont();
	}

	void drawradar(int w, int h, int s, float blend)
	{
		const struct rdpat {
			int xw, xs, yh, ys, ww, ws, hh, hs; float tx, ty, tw, th;
		} rdpats[8] =
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
		int cs = s;
		if(world::player1->state != CS_DEAD) // damage overlay goes full in this case
		{
			float r = 1.f, g = 1.f, b = 1.f, fade = radarblend*blend;
			if(radarhealth) switch(world::player1->state)
			{
				case CS_ALIVE: healthskew(cs, r, g, b, fade, radarskew); break;
				case CS_SPECTATOR: case CS_WAITING: r = g = b = 0.5f; break;
				default: break;
			}
			glColor4f(r, g, b, fade);
			settexture(radartex, 3);
			loopi(8)
			{
				const rdpat &rd = rdpats[i];
				drawtex(
					(rd.xw*w)+(rd.xs*cs), (rd.yh*h)+(rd.ys*cs),
					(rd.ww*w)+(rd.ws*cs), (rd.hh*h)+(rd.hs*cs),
					rd.tx, rd.ty, rd.tw, rd.th
				);
			}
		}
		if(m_edit(world::gamemode) || radaritems) drawentblips(w, h, cs/2, blend);
		loopv(world::players) if(world::players[i] && world::players[i]->state == CS_ALIVE)
			drawplayerblip(world::players[i], w, h, cs/2, blend);
		if(m_stf(world::gamemode)) stf::drawblips(w, h, cs/2, blend);
		else if(m_ctf(world::gamemode)) ctf::drawblips(w, h, cs/2, blend);
		if(radarcard) drawcardinalblips(w, h, cs/2, blend);
	}

	int drawitem(const char *tex, int x, int y, float size, float fade, float skew, const char *font, float blend, const char *text, ...)
	{
		float f = fade*skew, s = size*skew;
		settexture(tex, 0);
		glColor4f(f, f, f, f);
		drawsized(x-int(s), y-int(s), int(s));
		if(text && *text)
		{
			float off = skew*inventorytextscale;
			glPushMatrix();
			glScalef(off, off, 1);
			int tx = int((x-FONTW/4)*(1.f/off)), ty = int((y-s+FONTW/4)*(1.f/off)),
				tc = int(255.f*skew), ti = int(255.f*inventorytextblend*blend);
			if(font && *font) pushfont(font);
			s_sprintfdlv(str, text, text);
			draw_textx("%s", tx, ty, tc, tc, tc, ti, false, AL_RIGHT, -1, -1, str);
			if(font && *font) popfont();
			glPopMatrix();
		}
		return int(s);
	}

	const char *flagtex(int team)
	{
		const char *flagtexs[TEAM_MAX] = {
			neutralflagtex, alphaflagtex, betaflagtex, deltaflagtex, gammaflagtex
		};
		return flagtexs[team];
	}

	int drawweapons(int x, int y, int s, float blend)
	{
		const char *hudtexs[GUN_MAX] = {
			plasmatex, shotguntex, chainguntex, flamertex, carbinetex, rifletex, grenadestex
		};
		int sy = 0;
		loopi(GUN_MAX) if(world::player1->hasgun(i) || lastmillis-world::player1->gunlast[i] < world::player1->gunwait[i])
		{
			float fade = inventoryblend*blend, size = s, skew = 1.f;
			if(world::player1->gunstate[i] == GNS_SWITCH || world::player1->gunstate[i] == GNS_PICKUP)
			{
				float amt = clamp(float(lastmillis-world::player1->gunlast[i])/float(world::player1->gunwait[i]), 0.f, 1.f);
				skew = (i != world::player1->gunselect ?
					(
						world::player1->hasgun(i) ? 1.f-(amt*(1.f-inventoryskew)) : 1.f-amt
					) : (
						world::player1->gunstate[i] == GNS_PICKUP ? amt : inventoryskew+(amt*(1.f-inventoryskew))
					)
				);
			}
			else if(i != world::player1->gunselect) skew = inventoryskew;
			sy += drawitem(hudtexs[i], x, y-sy, size, fade, skew, "emphasis", blend, "%d", world::player1->ammo[i]);
		}
		return sy;
	}

	void drawinventory(int w, int h, int edge, float blend)
	{
		int cx = int(w-edge*1.5f), cy = int(h-edge*1.5f), cs = int(inventorysize*w),
			cr = cs/4, cc = 0;
		if(world::player1->state == CS_ALIVE && (cc = drawweapons(cx, cy, cs, blend)) > 0) cy -= cc+cr;
		if(m_ctf(world::gamemode) && ((cc = ctf::drawinventory(cx, cy, cs, blend)) > 0)) cy -= cc+cr;
		if(m_stf(world::gamemode) && ((cc = stf::drawinventory(cx, cy, cs, blend)) > 0)) cy -= cc+cr;
	}

	void drawgamehud(int w, int h)
	{
		Texture *t;
		int ox = hudwidth, oy = hudsize, os = showradar ? int(oy*radarsize) : 0,
			secs = world::maptime ? lastmillis-world::maptime : 0;
		float fade = hudblend;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);

		if(secs < titlecardtime+titlecardfade+titlecardfade)
		{
			float amt = clamp(float(secs-titlecardtime-titlecardfade)/float(titlecardfade), 0.f, 1.f);
			fade *= amt;
		}

		if(world::player1->state == CS_ALIVE && world::inzoom() && world::player1->gunselect == GUN_RIFLE)
		{
			t = textureload(snipetex);
			int frame = lastmillis-world::lastzoom;
			float pc = frame < world::zoomtime() ? float(frame)/float(world::zoomtime()) : 1.f;
			if(!world::zooming) pc = 1.f-pc;
			glBindTexture(GL_TEXTURE_2D, t->id);
			glColor4f(1.f, 1.f, 1.f, pc*fade);
			drawtex(0, 0, ox, oy);
		}

		if(showdamage && ((world::player1->state == CS_ALIVE && world::damageresidue > 0) || world::player1->state == CS_DEAD))
		{
			t = textureload(damagetex);
			int dam = world::player1->state == CS_DEAD ? 100 : min(world::damageresidue, 100);
			float pc = float(dam)/100.f;
			glBindTexture(GL_TEXTURE_2D, t->id);
			glColor4f(1.f, 1.f, 1.f, pc*fade);
			drawtex(0, 0, ox, oy);
		}

		if(showradar) drawradar(ox, oy, os, fade);
		if(showinventory) drawinventory(ox, oy, os, fade);
#if 0
		int tp = by + bs + FONTH/2;
		pushfont("emphasis");
		if(world::player1->state == CS_ALIVE)
		{
			if(showtips)
			{
				tp = oy-FONTH;
				if(showtips > 1)
				{
					if(world::player1->hasgun(world::player1->gunselect))
					{
						const char *a = retbindaction("zoom", keym::ACTION_DEFAULT, 0);
						s_sprintfd(actkey)("%s", a && *a ? a : "ZOOM");
						tp -= draw_textx("Press [ \fs\fg%s\fS ] to %s", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey, world::player1->gunselect == GUN_RIFLE ? "zoom" : "prone");
					}
					if(world::player1->canshoot(world::player1->gunselect, lastmillis))
					{
						const char *a = retbindaction("attack", keym::ACTION_DEFAULT, 0);
						s_sprintfd(actkey)("%s", a && *a ? a : "ATTACK");
						tp -= draw_textx("Press [ \fs\fg%s\fS ] to attack", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey);
					}

					if(world::player1->canreload(world::player1->gunselect, lastmillis))
					{
						const char *a = retbindaction("reload", keym::ACTION_DEFAULT, 0);
						s_sprintfd(actkey)("%s", a && *a ? a : "RELOAD");
						tp -= draw_textx("Press [ \fs\fg%s\fS ] to load ammo", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey);
						if(weapons::autoreload > 1 && lastmillis-world::player1->gunlast[world::player1->gunselect] <= 1000)
							tp -= draw_textx("Autoreload in [ \fs\fg%.01f\fS ] second(s)", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, float(1000-(lastmillis-world::player1->gunlast[world::player1->gunselect]))/1000.f);
					}
				}

				vector<actitem> actitems;
				if(entities::collateitems(world::player1, actitems))
				{
					bool found = false;
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
								if(!entities::ents.inrange(proj.id)) break;
								ent = proj.id;
								break;
							}
							default: break;
						}
						if(entities::ents.inrange(ent))
						{
							const char *a = retbindaction("action", keym::ACTION_DEFAULT, 0);
							s_sprintfd(actkey)("%s", a && *a ? a : "ACTION");

							extentity &e = *entities::ents[ent];
							if(enttype[e.type].usetype == EU_ITEM)
							{
								if(!found)
								{
									int drop = -1;
									if(e.type == WEAPON && guntype[world::player1->gunselect].carry &&
										world::player1->ammo[e.attr1] < 0 && guntype[e.attr1].carry &&
											world::player1->carry() >= MAXCARRY) drop = world::player1->drop(e.attr1);
									if(isgun(drop))
									{
										s_sprintfd(dropgun)("%s", entities::entinfo(WEAPON, drop, 0, 0, 0, 0, false));
										tp -= draw_textx("Press [ \fs\fg%s\fS ] to swap [ \fs%s\fS ] for [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey, dropgun, entities::entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, false));
									}
									else tp -= draw_textx("Press [ \fs\fg%s\fS ] to pickup [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey, entities::entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, false));
									if(showtips < 3) break;
									else found = true;
								}
								else tp -= draw_textx("Nearby [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, entities::entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, false));
							}
							else if(e.type == TRIGGER && e.attr3 == TA_ACT)
							{
								if(!found)
								{
									tp -= draw_textx("Press [ \fs\fg%s\fS ] to interact", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey);
									if(showtips < 3) break;
									else found = true;
								}
								else
									tp -= draw_textx("Nearby interactive item", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1);
							}
						}
						actitems.pop();
					}
				}
			}
		}
		else if(world::player1->state == CS_DEAD)
		{
			if(showtips)
			{
				tp = oy-FONTH;
				int wait = respawnwait(world::player1);
				if(wait)
					tp -= draw_textx("Fragged! Respawn available in [ \fs\fg%.01f\fS ] second(s)", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, float(wait)/1000.f);
				else
				{
					const char *a = retbindaction("attack", keym::ACTION_DEFAULT, 0);
					s_sprintfd(actkey)("%s", a && *a ? a : "ACTION");
					tp -= draw_textx("Fragged! Press [ \fs\fg%s\fS ] to respawn", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1, actkey);
				}
			}
		}
		else if(world::player1->state == CS_EDITING)
		{
			tp = oy-FONTH;
			if(showenttips) loopi(clamp(entgroup.length()+1, 0, showhudents))
			{
				int n = i ? entgroup[i-1] : enthover;
				if((!i || n != enthover) && entities::ents.inrange(n))
				{
					gameentity &f = (gameentity &)*entities::ents[n];
					if(showenttips <= 2 && n != enthover) pushfont("hud");
					tp -= draw_textx("entity %d, %s", bx+bs, tp,
						n == enthover ? 255 : 196, 196, 196, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1,
							n, entities::findname(f.type));
					if(showenttips <= 2 && n != enthover) popfont();
					if(showenttips > 1 || n == enthover)
					{
						tp -= draw_textx("%s (%d %d %d %d %d)", bx+bs, tp,
							255, 196, 196, int(255.f*fade*infoblend), false, AL_RIGHT, -1, -1,
								entities::entinfo(f.type, f.attr1, f.attr2, f.attr3, f.attr4, f.attr5, true),
									f.attr1, f.attr2, f.attr3, f.attr4, f.attr5);
					}
				}
			}
		}
		popfont(); // emphasis
#endif
	}

	void drawhudelements(int w, int h)
	{
		int ox = hudwidth, oy = hudsize, os = showradar ? int(oy*radarsize*1.5f) : 0,
			is = showinventory ? int(oy*inventorysize) : 0, bx = os+FONTW/4, by = oy-os-(FONTH/3)*2, bs = ox-bx*2-is;

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
		if(showstats)
		{
			by -= draw_textx("ond:%d va:%d gl:%d(%d) oq:%d lm:%d rp:%d pvs:%d", bx, by, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, bs, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6], lightmaps.length(), curstats[7], getnumviewcells());
			by -= draw_textx("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", bx, by, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, bs, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
		}

		if(showfps) switch(showfps)
		{
			case 2:
				if(autoadjust) by -= draw_textx("fps:%d (%d/%d) +%d-%d [\fs%s%d%%\fS]", bx, by, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, bs, curstats[8], autoadjustfps, maxfps, curstats[9], curstats[10], curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
				else by -= draw_textx("fps:%d (%d) +%d-%d", bx, by, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, bs, curstats[8], maxfps, curstats[9], curstats[10]);
				break;
			case 1:
				if(autoadjust) by -= draw_textx("fps:%d (%d/%d) [\fs%s%d%%\fS]", bx, by, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, bs, curstats[8], autoadjustfps, maxfps, curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
				else by -= draw_textx("fps:%d (%d)", bx, by, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, bs, curstats[8], maxfps);
				break;
			default: break;
		}

		if(connected() && world::maptime)
		{
			if(world::player1->state == CS_EDITING)
			{
				by -= draw_textx("sel:%d,%d,%d %d,%d,%d (%d,%d,%d,%d)", bx, by, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, bs,
						sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z,
							sel.cx, sel.cxs, sel.cy, sel.cys);
				by -= draw_textx("corner:%d orient:%d grid:%d", bx, by, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, bs,
								sel.corner, sel.orient, sel.grid);
				by -= draw_textx("cube:%s%d ents:%d", bx, by, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, bs,
					selchildcount<0 ? "1/" : "", abs(selchildcount), entgroup.length());
			}
		}

		if(getcurcommand()) by -= rendercommand(bx, by, bs);

		popfont(); // emphasis
	}

	void drawhud(int w, int h)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		if(world::maptime && connected())
		{
			if(lastmillis-world::maptime < titlecardtime+titlecardfade)
				drawtitlecard(w, h);
			else drawgamehud(w, h);
		}
		drawhudelements(w, h);
		glDisable(GL_BLEND);
	}

	bool getcolour(vec &colour)
	{
		if(!world::maptime || lastmillis-world::maptime < titlecardtime)
		{
			float fade = world::maptime ? float(lastmillis-world::maptime)/float(titlecardtime) : 0.f;
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
