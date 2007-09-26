// Blood Frontier - SSPGAME - Side Scrolling Platformer by Quinton Reeves
// This is the SSP entities.

struct sspentities : icliententities
{
	sspclient &cl;
	vector<sspentity *> ents;

	struct sspitem
	{
		char *name;								// name
		bool touch, pickup, detach;				// touchable / collectable / detachable
		int render, wait;						// render type (0: don't, 1: bobbing/rotating, 2: static) / wait time
		bool enemy, proj;						// usable by enemy / projectile
		float yaw, zoff, height, radius, elast;	// yaw / z offset / height / radius / bounciness
	} *sspitems;

	sspentities(sspclient &_cl) : cl(_cl)
	{
		//	name				touch	pick	detach	r	wait	enemy	proj	yaw		zoff	height, radius,	elast
		static sspitem _sspitems[] = {
			{ "none?",			false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "light",			false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "mapmodel",		false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "playerstart",	false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "envmap",			false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "particles",		false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "sound",			false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "spotlight",		false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "coin",			true,	true,	true,	1,	0, 		false,	false,	0.f,	-4.f,	4.f,	4.f,	0.8f },
			{ "powerup",		true,	true,	true,	1,	0, 		false,	false,	0.f,	-3.f,	4.f,	4.f,	0.7f },
			{ "invincible",		true,	true,	true,	1,	0, 		false,	false,	0.f,	-4.5f,	4.f,	4.f,	0.6f },
			{ "life",			true,	true,	true,	1,	0, 		false,	false,	0.f,	-5.f,	4.5f,	4.5f,	0.6f },
			{ "enemy",			false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "goal",			true,	false,	false,	2,	1000, 	false,	false,	270.f,	-8.f,	16.f,	4.f,	0.0f },
			{ "jumppad",		true,	false,	false,	2,	500,	true,	true,	270.f,	0.f,	8.f,	8.f,	0.0f },
			{ "teleport",		true,	false,	false,	1,	750,	true,	true,	0.f,	-9.f,	12.f,	8.f,	0.0f },
			{ "teledest",		false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "path",			false,	false,	false,	0,	0, 		true,	true,	0.f,	0.f,	0.f,	32.f,	0.0f },
			{ "block",			false,	false,	false,	0,	0, 		false,	false,	0.f,	0.f,	0.f,	0.f,	0.3f },
			{ "axis",			true,	true,	false,	0,	1000, 	true,	true,	0.f,	0.f,	16.f,	4.f,	0.0f },
			{ "script",			true,	true,	false,	0,	1000, 	false,	false,	0.f,	0.f,	16.f,	4.f,	0.0f },
			{ "shield",			true,	true,	true,	1,	0, 		false,	false,	0.f,	-3.f,	4.f,	4.f,	0.7f },
		};
		sspitems = _sspitems;
	}
	~sspentities() { }

	vector<extentity *> &getents() { return (vector<extentity *> &)ents; }

	void renderent(sspentity &e, const char *mdlname, int tex, float z, float yaw, float pitch, int frame = 0, int anim = ANIM_MAPMODEL|ANIM_LOOP, int basetime = 0, float speed = 10.0f)
	{
		rendermodel(e.color, e.dir, mdlname, anim, 0, tex, vec(e.o).add(vec(0, 0, z)), yaw, pitch, speed, basetime, NULL, MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED);
	}

	char *renderentmdl(uchar type, short attr1, short attr2, short attr3, short attr4, short attr5)
	{
		char *imdl = NULL;

		switch (type)
		{
			case SE_COIN:
				if (attr1 > 1) imdl = "tentus/moneybag";
				else imdl = "coin";
				break;
			case SE_POWERUP:
				{
					switch (attr1)
					{
						case PW_HIT:
							imdl = "ammo/life";
							break;
						case PW_GRENADE:
							imdl = "ammo/grenades";
							break;
						case PW_ROCKET:
							imdl = "ammo/rockets";
							break;
						case PW_BOMB:
							imdl = "ammo/bombs";
							break;
						default:
							break;
					}
					break;
				}
			case SE_INVINCIBLE:
				imdl = "quad";
				break;
			case SE_LIFE:
				imdl = "boost";
				break;
			case SE_GOAL:
				if ((attr1 > 0 || attr2 == 0) && (editmode || cl.isfinished())) { imdl = "flags/red"; }
				else if ((attr1 == 0 && attr2 > 0) && (editmode || (ents.inrange(cl.player1->sd.chkpoint) &&
					ents[cl.player1->sd.chkpoint]->type == SE_GOAL &&
					ents[cl.player1->sd.chkpoint]->attr2 >= attr2)))
				{
					imdl = "flags/blue";
				}
				else { imdl = "flags/neutral"; }
				break;
			case SE_JUMPPAD:
				imdl = "dcp/jumppad2";
				break;
			case SE_TELEPORT:
				if (attr2 == 0) imdl = "teleporter";
				break;
			case SE_SHIELD:
				{
					imdl = shields[attr1];
					break;
				}
			default:
				break;
		}
		return imdl;
	}

	float renderentpitch(uchar type, short attr1, short attr2, short attr3, short attr4, short attr5)
	{
		switch (type)
		{
			case SE_JUMPPAD:
				return (float)attr3;
				break;
			default:
				break;
		}
		return 0.0f;
	}


	void renderentforce(sspentity &e)
	{
		//sspitem &t = sspitems[e.type];

		switch (e.type)
		{
			case SE_PATH:
				{
					if (showentdir && ents.inrange(e.link))
					{
						sspentity &f = *ents[e.link];

						if (e.link > f.link)
						{
							vec fr = e.o, to = f.o;
							fr.z += RENDERPUSHZ;
							to.z += RENDERPUSHZ;
							renderline(fr, to, e.attr2 ? 0.3f : 0.0f, 0.0f, 1.f);
						}
					}
					break;
				}
			default:
				break;
		}
	}

	void renderentshow(sspentity &e)
	{
		sspitem &t = sspitems[e.type];

		if (showentradius && (t.height > 0.f || t.radius > 0.f)) renderentradius(e.o, t.height, t.radius);

		switch (e.type)
		{
			case ET_PLAYERSTART:
			case SE_AXIS:
				if (showentdir)
				{
					if (e.attr1 != DR_NONE) renderentdir(e.o, diryaw[e.attr1], 0.f);
					if (e.attr2 != DR_NONE) renderentdir(e.o, diryaw[e.attr2], 0.f);
				}
				break;
			case ET_MAPMODEL:
				if (showentdir && e.attr1 >= 0.f && e.attr1 <= 360.f) renderentdir(e.o, e.attr1, 0.0f);
				break;
			case SE_ENEMY:
			case SE_SCRIPT:
				if (showentdir && e.attr3 != DR_NONE) renderentdir(e.o, diryaw[e.attr3], 0.f);
				break;
			case SE_TELEDEST:
				if (showentdir && e.attr2 != DR_NONE) renderentdir(e.o, diryaw[e.attr2], 0.f);
				break;
			case SE_JUMPPAD:
				{
					if (showentdir)
					{
						float yaw, pitch;
						vec dir;

						dir = vec(0, e.attr2, e.attr1);
						dir.normalize();
						vectoyawpitch(dir, yaw, pitch);
						renderentdir(e.o, yaw, pitch);

						dir = vec(e.attr2, 0, e.attr1);
						dir.normalize();
						vectoyawpitch(dir, yaw, pitch);
						renderentdir(e.o, yaw, pitch);
					}
					break;
				}
			default:
				break;
		}
	}

	void renderentities()
	{
		if (editmode)
		{
			#define entfocus(i, f) \
				{ int n = efocus = (i); if(n>=0) { sspentity &e = *ents[n]; f; } }
	
			renderprimitive(true);
			if(enthover>=0) { entfocus(enthover, renderentshow(e)); }
			else { loopv(entgroup) entfocus(entgroup[i], renderentshow(e)); }
			loopv(ents) entfocus(i, renderentforce(e));
			renderprimitive(false);
		}

		loopv(ents)
		{
			sspentity &e = *ents[i];
			sspitem &t = sspitems[e.type];

			if(!e.spawned || !t.render) continue;

			char *imdl = renderentmdl(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);

			if (imdl)
			{
				float z = (t.render == 1 ? (float)(1+sin(lastmillis/100.0+e.o.x+e.o.y)/20.f) : 0.f) + t.zoff;
				float yaw = (t.render == 1 ? lastmillis/10.f : 0.f) + t.yaw;
				float pitch = renderentpitch(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
				
				renderent(e, imdl, 0, z, yaw, pitch);
			}
		}
	}

	void fixent(uchar *type, short *attr1, short *attr2, short *attr3, short *attr4, short *attr5)
	{
		string s;

		switch(*type)
		{
			case ET_PLAYERSTART:
				*attr1 = *attr1 > (short)DR_NONE && *attr1 < (short)DR_MAX ? *attr1 : (*attr1 >= (short)DR_MAX ? (short)DR_LEFT : (short)DR_DOWN);
				break;
			case SE_COIN:
			case SE_LIFE:
				*attr1 = *attr1 > 0 ? *attr1 : 1;
				break;
			case SE_POWERUP:
				*attr1 = *attr1 >= PW_HIT && *attr1 < (short)PW_MAX ? *attr1 : (*attr1 >= (short)PW_MAX ? (short)PW_HIT : (short)PW_MAX-1);
				*attr2 = *attr2 >= 0 && *attr2 <= 1 ? *attr2 : (*attr2 > 1 ? 0 : 1);
				break;
			case SE_ENEMY:
				s_sprintf(s)("en_%d_%d_init", *attr1, *attr2);
				if (!identexists(s)) *attr2 = 0;
				s_sprintf(s)("en_%d_%d_init", *attr1, *attr2);
				if (!identexists(s)) *attr1 = 0;
				*attr3 = *attr3 > (short)DR_NONE && *attr3 < (short)DR_MAX ? *attr3 : (*attr3 >= (short)DR_MAX ? (short)DR_LEFT : (short)DR_DOWN);
				break;
			case SE_GOAL:
				if (*attr1 > 0)
				{
					*attr2 = *attr2 >= 0 ? *attr2 : getvar("world");
					s_sprintf(s)("map_%d_%d", *attr2, *attr1);
					
					if (!identexists(s))
					{
						*attr1 = 0;
						*attr2 = 0;
					}
				}
				else
				{
					*attr1 = 0;
					*attr2 = *attr2 >= 0 ? *attr2 : 0;
				}
				break;
			case SE_JUMPPAD:
				*attr3 = *attr3 >= 0 && *attr3 <= 360 ? *attr3 : (*attr3 > 360 ? 0 : 360);
				break;
			case SE_TELEPORT:
				*attr2 = *attr2 >= 0 ? *attr2 : 0;
				break;
			case SE_TELEDEST:
				*attr2 = *attr2 >= (short)DR_NONE && *attr2 < (short)DR_MAX ? *attr2 : (*attr2 >= (short)DR_MAX ? (short)DR_LEFT : (short)DR_DOWN);
				break;
			case SE_PATH:
				*attr2 = *attr2 >= 0 && *attr2 <= 1 ? *attr2 : (*attr2 > 1 ? 0 : 1);
				break;
			case SE_BLOCK:
				s_sprintf(s)("bl_%d_init", *attr1);
				if (!identexists(s)) *attr1 = 0;
				*attr2 = *attr2 >= 0 && *attr2 < curtexnum ? *attr2 : (*attr2 >= curtexnum ? 0 : curtexnum-1);
				break;
			case SE_AXIS:
				*attr1 = *attr1 > (short)DR_NONE && *attr1 < (short)DR_MAX ? *attr1 : (*attr1 >= (short)DR_MAX ? (short)DR_LEFT : (short)DR_DOWN);
				*attr2 = *attr2 > (short)DR_NONE && *attr2 < (short)DR_MAX ? *attr2 : (*attr2 >= (short)DR_MAX ? (short)DR_LEFT : (short)DR_DOWN);
				*attr3 = *attr3 >= 0 && *attr3 <= 1 ? *attr3 : (*attr3 > 1 ? 0 : 1);
				break;
			case SE_SCRIPT:
				s_sprintf(s)("script_%d", *attr1);
				if (!identexists(s))
				{
					*attr1 = 0;
				}
				*attr2 = *attr2 >= 0 && *attr2 <= 1 ? *attr2 : (*attr2 > 1 ? 0 : 1);
				*attr3 = *attr3 > (short)DR_NONE && *attr3 < (short)DR_MAX ? *attr3 : (*attr3 >= (short)DR_MAX ? (short)DR_LEFT : (short)DR_DOWN);
				break;
			case SE_SHIELD:
				*attr1 = *attr1 >= SH_NONE+1 && *attr1 < (short)SH_MAX ? *attr1 : (*attr1 >= (short)SH_MAX ? (short)SH_NONE+1 : (short)SH_MAX-1);
				break;
			default:
				break;
		}
	}

	void fixentity(extentity &d)
	{
		sspentity &e = (sspentity &)d;
		
		switch (e.type)
		{
			case SE_PATH:
				linkpath(e);
			default:
				fixent(&e.type, &e.attr1, &e.attr2, &e.attr3, &e.attr4, &e.attr5);
				break;
		}
	}

	void editent(int i)
	{
		//if (ents.inrange(i)) fixentity(*ents[i]);
		cl.resetgame(true); // fixes all
	}

	const char *entnameinfo(entity &e) { return ""; }
	const char *entname(int i) { return i >= 0 && i < SE_MAX ? sspitems[i].name : ""; }

    int extraentinfosize() { return 0; } //EXTRAENTSIZE; }
    void writeent(entity &e, char *buf)
    {
        //endianswap(&((sspentity &)e).extr1, sizeof(char), EXTRAENTSIZE);
    	//memcpy(buf, &((sspentity &)e).extr1, sizeof(char)*EXTRAENTSIZE);
	}
    void readent (entity &e, char *buf)
    {
    	//memcpy(&((sspentity &)e).extr1, buf, sizeof(char)*EXTRAENTSIZE);
        //endianswap(&((sspentity &)e).extr1, sizeof(char), EXTRAENTSIZE);
	}

	float dropheight(entity &e)
	{
		sspitem &t = sspitems[e.type];
		return t.height;
	}

	void rumble(const extentity &e) { cl.sfx(SF_RUMBLE, &e.o); }
	void trigger(extentity &e) {}

	extentity *newentity() { return new sspentity; }

	void pickuptext(actent *d, char *fmt, ...)
	{
		vec v(camera1->o);
		v.sub(d->o);
		v.mul(0.25f);
		v.add(d->o);
		s_sprintfdlv(str, fmt, fmt);
		part_textf(v, "%s", true, CARDFADE, COL_WHITE, str);
		console("%s", CON_CENTER, str);
	}

	bool reachscript(actent *d, vec &o, short attr1, short attr2, short attr3, short attr4)
	{
		string s;
		s_sprintf(s)("script_%d", attr1);
		if (identexists(s))
		{
			s_sprintf(s)("script_%d %d %d", attr1, d->direction == attr3 ? 1 : 0, attr4);
			execute(s);
		}
		return true;
	}

	bool reachaxis(actent *d, vec &o, short attr1, short attr2, short attr3)
	{
		if (d->direction == DR_LEFT || d->direction == DR_RIGHT)
		{
			if (attr1 == DR_UP || attr1 == DR_DOWN)
				d->setdir(attr1, true);
			else if (attr2 == DR_UP || attr2 == DR_DOWN)
				d->setdir(attr2, true);
		}
		else if (d->direction == DR_UP || d->direction == DR_DOWN)
		{
			if (attr1 == DR_LEFT || attr1 == DR_RIGHT)
				d->setdir(attr1, true);
			else if (attr2 == DR_LEFT || attr2 == DR_RIGHT)
				d->setdir(attr2, true);
		}

		if (isxaxis(d->direction)) d->setpath(o.x, true);
		else d->setpath(o.y, true);
		
		return true;
	}

	bool reachgoal(sspent *d, int idx, vec &o, short attr1, short attr2)
	{
		if (!cl.isfinished() && cl.player1->sd.chkpoint != idx)
		{
			if (attr1 > 0 || attr2 == 0) // end goal
			{
				cl.sfx(SF_GOAL, d != cl.player1 ? &d->o : NULL);
		
				cl.player1->sd.chkpoint = idx;
				
				setvar("level", attr1 > 0 ? attr1 : getvar("level") + 1);

				if (getvar("level") > 8)
				{
					setvar("level", 1);
					setvar("world", getvar("world") + 1);
				}
				else if (attr2 > 0)
				{
					setvar("world", attr2);
				}
				
				if (getvar("cuttime") <= 0)
				{
					setvar("cuttime", CARDTIME);
					setvar("cuthold", 1);

					d->setdir(DR_NONE, false);
					
					d->o.x = o.x;
					d->o.y = o.y;
					
					if (isxaxis(d->direction)) d->setpath(o.x, true);
					else d->setpath(o.y, true);
				}
				part_spawn(o, vec(sspitems[SE_GOAL].radius, sspitems[SE_GOAL].radius, sspitems[SE_GOAL].height), 0, 6, 100, 500, COL_LRED);
			}
			else if (!ents.inrange(cl.player1->sd.chkpoint) || ents[cl.player1->sd.chkpoint]->type != SE_GOAL || ents[cl.player1->sd.chkpoint]->attr2 < attr2)
			{
				cl.sfx(SF_CHKPOINT, d != cl.player1 ? &d->o : NULL);
				
				cl.player1->sd.chkpoint = idx;
				
				part_spawn(o, vec(sspitems[SE_GOAL].radius, sspitems[SE_GOAL].radius, sspitems[SE_GOAL].height), 0, 6, 100, 500, COL_LBLUE);

				if (getvar("level") == cl.player1->sd.level && getvar("world") == cl.player1->sd.world && getvar("saveslot") > 0)
						cl.writesave(getvar("saveslot"), false);
			}
			return true;
		}
		return false;
	}

	bool pickupcoin(sspent *d, vec &o, short attr1)
	{
		cl.sfx(SF_COIN, d != cl.player1 ? &d->o : NULL);

		d->sd.coins += attr1;

		if (d->sd.coins >= 100)
		{
			int lv = (d->sd.coins-(d->sd.coins%100));
			pickuplife(d, o, lv/100);
			d->sd.coins -= lv;
		}

		if (attr1 > 1) pickuptext(d, "\f2+%d coins", attr1);
		part_spawn(o, vec(sspitems[SE_COIN].radius, sspitems[SE_COIN].radius, sspitems[SE_COIN].height), 0, 2, 100, 250, COL_YELLOW);
		
		return true;
	}

	bool pickuppowerup(sspent *d, vec &o, short attr1, short attr2)
	{
		short attr = attr2 || d->sd.power > 1 ? attr1 : (short)PW_HIT;
		
		if (attr >= PW_HIT && attr < PW_MAX)
		{
			int r = attr == PW_HIT ? 2 : 3, g = pgun[attr];
			
			cl.sfx(SF_POWERUP, d != cl.player1 ? &d->o : NULL);

			if (d->sd.power >= r)
			{
				if (r > 2 && d->sd.gun != g)
				{
					d->setinventory(pw[d->sd.gun]);
					d->sd.gun = g;
					d->setpower(r);
				}
				else d->setinventory(attr);
			}
			else
			{
				d->sd.gun = g;
				d->setpower(r);
			}

			pickuptext(d, "\f0%s powerup", pname[attr]);
			part_spawn(o, vec(sspitems[SE_POWERUP].radius, sspitems[SE_POWERUP].radius, sspitems[SE_POWERUP].height), 0, 6, 100, 500, pcol[attr1]);

			return true;
		}
		return false;
	}

	bool pickupshield(sspent *d, vec &o, short attr1)
	{
		cl.sfx(SF_SHIELD, d != cl.player1 ? &d->o : NULL);

		d->sd.shield = attr1;
		d->special = false;

		pickuptext(d, "\f6%s shield", sname[attr1]);
		part_spawn(o, vec(sspitems[SE_SHIELD].radius, sspitems[SE_SHIELD].radius, sspitems[SE_SHIELD].height), 0, 6, 100, 500, scol[attr1]);
		
		return true;
	}
	
	bool pickupinvincible(sspent *d, vec &o)
	{
		cl.sfx(SF_INVINCIBLE, d != cl.player1 ? &d->o : NULL);
		d->invincible = getvar("invincible");
		
		pickuptext(d, "\f3invincibile");
		part_spawn(o, vec(sspitems[SE_INVINCIBLE].radius, sspitems[SE_INVINCIBLE].radius, sspitems[SE_INVINCIBLE].height), 0, 6, 100, 500, COL_RED);
		
		return true;
	}

	bool pickuplife(sspent *d, vec &o, short attr1)
	{
		cl.sfx(SF_LIFE, d != cl.player1 ? &d->o : NULL);
		d->sd.lives += attr1;
		
		pickuptext(d, "\f4+%d %s", attr1, attr1 > 1 ? "lives" : "life");
		part_spawn(o, vec(sspitems[SE_LIFE].radius, sspitems[SE_LIFE].radius, sspitems[SE_LIFE].height), 0, 6, 100, 500, COL_WHITE);
		
		return true;
	}

	bool jumppad(actent *d, vec &o, short attr1, short attr2)
	{
		cl.sfx(SF_JUMPPAD, d != cl.player1 ? &d->o : NULL);

		if (d->type == ENT_PLAYER || d->type == ENT_AI) d->timeinair = 0;

		d->gravity = vec(0, 0, 0);
		d->vel.z = attr1;

		if (isxaxis(d->direction))
		{
			if (attr2 != 0) d->setdir(attr2 > 0 ? DR_RIGHT : DR_LEFT, false);
			d->vel.y = attr2;
		}
		else
		{
			if (attr2 != 0) d->setdir(attr2 > 0 ? DR_UP : DR_DOWN, false);
			d->vel.x = attr2;
		}

		part_spawn(o, vec(sspitems[SE_JUMPPAD].radius, sspitems[SE_JUMPPAD].radius, sspitems[SE_JUMPPAD].height), 0, 2, 100, 500, COL_WATER);
		
		return true;
	}

	bool teleport(actent *d, vec &o, short attr1, short attr2)
	{
		bool ans = false;
		loopv(ents)
		{
			sspentity &f = *ents[i];

			if (f.type == SE_TELEDEST && f.attr1 == attr1)
			{
				d->setposition(f.attr2 != DR_NONE ? f.attr2 : d->direction, f.o.x, f.o.y, f.o.z);
				d->vel = vec(dirvec[d->direction]).mul(getvar("teledvel"));

				switch (attr2)
				{
					case 0:
					{
						cl.sfx(SF_TELEPORT, &o);
						cl.sfx(SF_TELEPORT, d->type != ENT_PLAYER ? &f.o : NULL);
	
						part_spawn(o, vec(sspitems[SE_TELEPORT].radius, sspitems[SE_TELEPORT].radius, sspitems[SE_TELEPORT].height), 0, 6, 100, 500, COL_WATER);
						if (d->type == ENT_PLAYER) cl.recomputecamera(); // otherwise maxparticledistance interferes
						part_spawn(f.o, vec(sspitems[SE_TELEPORT].radius, sspitems[SE_TELEPORT].radius, sspitems[SE_TELEPORT].height), 0, 6, 100, 500, COL_WATER);
				
						break;
					}
					default:
						break;
				}

				ans = true;
				break;
			}
		}

		if (!ans)
		{
			s_sprintfd(s)("teleport_%d", attr1);
		
			if (identexists(s))
			{
				execute(s);
				ans = true;
			}
		}
		
		if (!ans) conoutf("no teleport destination for tag %d", attr1);
		
		return ans;
	}

	bool pickup(actent *d, int idx, vec &o, uchar type, short attr1, short attr2, short attr3, short attr4, short attr5)
	{
		switch(type)
		{
			case SE_COIN:
				return pickupcoin((sspent *)d, o, attr1);
				break;
			case SE_POWERUP:
				return pickuppowerup((sspent *)d, o, attr1, attr2);
				break;
			case SE_INVINCIBLE:
				return pickupinvincible((sspent *)d, o);
				break;
			case SE_LIFE:
				return pickuplife((sspent *)d, o, attr1);
				break;
			case SE_GOAL:
				return reachgoal((sspent *)d, idx, o, attr1, attr2);
				break;
			case SE_JUMPPAD:
				return jumppad(d, o, attr1, attr2);
				break;
			case SE_TELEPORT:
				return teleport(d, o, attr1, attr2);
				break;
			case SE_AXIS:
				return reachaxis(d, o, attr1, attr2, attr3);
				break;
			case SE_SCRIPT:
				return reachscript(d, o, attr1, attr2, attr3, attr4);
				break;
			case SE_SHIELD:
				return pickupshield((sspent *)d, o, attr1);
				break;
			default:
				break;
		}
		return false;
	}

	void closestclamp(sspent *d)
	{
		int best = d->sd.chkpoint;

		loopv(ents)
		{
			sspentity &e = *ents[i];

			if (e.type == ET_PLAYERSTART || e.type == SE_TELEDEST ||
				e.type == SE_AXIS || e.type == SE_GOAL)
			{
				if (!ents.inrange(best) || d->o.dist(e.o) < d->o.dist(ents[best]->o))
					best = i;
			}
		}

		if (ents.inrange(best))
		{
			sspentity &e = *ents[best];
			int dir;
			if (e.type == SE_TELEDEST) dir = e.attr2;
			else if (e.type == SE_GOAL || e.type == SE_PATH) dir = d->direction;
			else dir = e.attr1;
			
			d->setposition(dir,
				dir == DR_UP || dir == DR_DOWN ? d->o.x : e.o.x,
					dir == DR_LEFT || dir == DR_RIGHT ? d->o.y : e.o.y, e.o.z);
		}
	}

	void linkpath(sspentity &e)
	{
		e.link = -1;
		loopvj(ents)
		{
			sspentity &f = *ents[j];

			if ((&e != &f) && (f.type == SE_PATH) &&
				(e.attr1 == f.attr1) && (e.o.x == f.o.x || e.o.y == f.o.y) &&
					(!ents.inrange(e.link) || e.o.dist(f.o) < e.o.dist(ents[e.link]->o)))
			{
				e.link = j;
			}
		}
	}

	void checkpath(actent *d)
	{
		float p = d->clamp, q = 0.f;

		loopv(ents)
		{
			sspentity &e = *ents[i];
			
			if (e.type == SE_PATH)
			{
				if (ents.inrange(e.link) && e.link > i)
				{
					sspitem &t = sspitems[e.type];
					float d1 = isxaxis(d->direction) ? d->o.x : d->o.y;
					float d2 = isxaxis(d->direction) ? e.o.x : e.o.y;

					sspentity &f = *ents[e.link];

					if (fabs(d2-d1) <= t.radius && ((isxaxis(d->direction) && (e.o.x == f.o.x)) || (!isxaxis(d->direction) && (e.o.y == f.o.y))))
					{
						float a = isxaxis(d->direction) ? e.o.y-d->o.y : e.o.x-d->o.x;
						float b = isxaxis(d->direction) ? f.o.y-d->o.y : f.o.x-d->o.x;
						float z = d->o.z-d->eyeheight-e.o.z-(!e.attr2 && !f.attr2 && d->depth ? d->o.z-e.o.z-1.f : 0.f);
	
						if (((a > 0 && b < 0) || (a < 0 && b > 0)) && ((z >= 0) && (e.o.z > q)))
						{
							p = d2;
							q = e.o.z;
						}
					}
				}
			}
		}
		d->setpath(p, false);
		d->offz = q;
	}

	void checkinvincible(sspent *d)
	{
		if (d->invincible && (d->invincible -= curtime) <= 0)
		{
			cl.sfx(SF_INVOVER, d != cl.player1 ? &d->o : NULL);
			d->invincible = 0;
		}
	}
	
	bool checkentity(actent *d, projent *j, int idx, vec &o, uchar type, short attr1, short attr2, short attr3, short attr4, short attr5)
	{
		sspitem &t = sspitems[type];

		if (!t.touch || d->type == ENT_BLOCK) return false;
		if ((d->type == ENT_AI && !t.enemy) || (d->type == ENT_BOUNCE && !t.proj)) return false;
		if (t.wait > 0 && (d->lastent == type && lastmillis-d->lastenttime < (getvar("cuttime") > 0 ? t.wait/5 : t.wait))) return false;

		float eye = d->eyeheight*0.5f;
		vec m = d->o;
		m.z -= eye;

		if (inbounds(m, eye, d->radius, o, t.height, t.radius))
		{
			bool pick = pickup(d, idx, o, type, attr1, attr2, attr3, attr4, attr5);
			if (t.wait > 0 && pick) { d->lastenttime = lastmillis; d->lastent = type; }
			if (t.pickup && d->type == ENT_PLAYER && (type != SE_AXIS || !attr3) && (type != SE_SCRIPT || !attr2)) return pick;
		}
		else if (d->type == ENT_PLAYER && t.detach)
		{
			sspent *p = (sspent *)d;
			
			if (p->sd.shield == SH_ELEC)
			{
				if (inbounds(m, getvar("elecsize")/2, getvar("elecsize"), o, t.height, t.radius))
				{
					part_flare(vec(m).add(vec(rnd(int(d->xradius*2))-d->xradius, rnd(int(d->yradius*2))-d->yradius, rnd(int(eye*2))-eye)),
						vec(vec(o).add(vec(0, 0, t.height))).add(vec(rnd(int(t.radius*2))-t.radius, rnd(int(t.radius*2))-t.radius, rnd(int(t.height*2))-t.height)),
						100, 21, scol[SH_ELEC]);

					if (idx >= 0)
					{
						cl.pr.newprojectile(o, vec(o).sub(vec(0, 0, 1)), p, t.height, t.radius, PR_ENT, type, attr1, attr2, attr3, attr4, attr5, INT_MAX-1, getvar("entspeed"), true, false);
						return true;
					}
					else if (idx == -1 && j != NULL)
					{
						j->vel = vec(vec(vec(d->o).sub(j->o)).normalize()).mul(cl.ph.speed(j));
						j->gravity = vec(0, 0, 0);
						return false;
					}
				}
			}
		}
		return false;
	}

	void checkentities(actent *d)
	{
		if (d->type == ENT_PLAYER) { checktriggers(); }
		if (d->type == ENT_PLAYER || d->type == ENT_AI) { checkinvincible((sspent *)d); }
		if (d->doclamp) { checkpath(d); cl.update2d(d); }
		if (d->type == ENT_BLOCK) return;
		
		loopv(ents)
		{
			sspentity &e = *ents[i];
			
			if (e.spawned)
			{
				if (checkentity(d, NULL, i, e.o, e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5))
					e.spawned = false;
			}
		}
	}

	void spawnprojents(sspent *d, vec &o, float y, float z, float zpush, uchar type, short attr1, short attr2, short attr3, short attr4, short attr5, int amt)
	{
		loopi(amt)
		{
			vec w(0, rnd(int(y*2))-int(y), rnd(int(z*2))-int(z)+zpush);
			w.add(o);
			cl.pr.newprojectile(o, w, d, cl.et.sspitems[type].height, cl.et.sspitems[type].radius, PR_ENT, type, attr1, attr2, attr3, attr4, attr5, getvar("enttime"), getvar("entspeed"), true, true);
		}
	}

	void resetents()
	{
		resettriggers();

		cl.bl.clearblocks();
		cl.en.clearenemies();
		cl.pr.projectileeset();

		cleardynentcache();
	}

    void spawnents()
    {
		show_out_of_renderloop_progress(0.f, "spawning entities..");
		resetents();

        loopv(ents)
        {
			sspentity &e = *ents[i];
			sspitem &t = sspitems[e.type];
			string entnam;
			s_sprintf(entnam)("spawning entities.. (%s)", t.name);
			show_out_of_renderloop_progress(float(i)/float(ents.length()), entnam);
	
			fixentity(e);

			switch(e.type)
			{
				case SE_ENEMY:
					cl.en.addenemy(e);
					break;
				case SE_PATH:
					linkpath(e);
					break;
				case SE_BLOCK:
		            cl.bl.addblock(e);
		            break;
				default:
		            break;
			}
            e.spawned = true;
        }
		show_out_of_renderloop_progress(1.f, "spawning entities.. done.");
    }

	void spawnstart()
	{
		if (!ents.inrange(cl.player1->sd.chkpoint))
		{ // find a playerstart
			loopv(ents)
			{
				if (ents[i]->type == ET_PLAYERSTART)
				{
					cl.player1->sd.chkpoint = i;
					break;
				}
			}
		}
		
		if (ents.inrange(cl.player1->sd.chkpoint))
			cl.player1->setposition(ents[cl.player1->sd.chkpoint]->attr1, ents[cl.player1->sd.chkpoint]->o.x, ents[cl.player1->sd.chkpoint]->o.y, ents[cl.player1->sd.chkpoint]->o.z);
		else
			cl.player1->setposition(DR_RIGHT, 128.f, 128.f, 0.5f*getworldsize());
	}

	void spawnplayer(bool respawn)
	{
		if (!respawn || cl.player1->sd.lives > 0)
		{
			if (identexists("map_spawn")) execute("map_spawn");
			if (ents.inrange(cl.nextspawn)) { cl.player1->sd.chkpoint = cl.nextspawn; cl.nextspawn = -1; }

			cl.leveltime = 0;
			spawnstart();
			spawnents();
			cl.player1->spawn(respawn);

			if (getvar("level") == cl.player1->sd.level && getvar("world") == cl.player1->sd.world && getvar("saveslot") > 0) cl.writesave(getvar("saveslot"), false);
		}
		else
		{ // outta lives, go back to the start
			load_world(getdefaultmap());
		}
	}

	void entprerender()
	{
		show_out_of_renderloop_progress(0.f, "pre-rendering.. entities");
		loopv(ents)
		{
			sspentity &e = *ents[i];
			sspitem &t = sspitems[e.type];

			show_out_of_renderloop_progress(float(i)/float(ents.length()), "pre-rendering.. entities (models)");

			if (t.render)
			{
				vec v(1, 1, 1);
				char *mdlname = renderentmdl(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
				rendermodel(v, v, mdlname, ANIM_MAPMODEL|ANIM_LOOP, 0, 0, vec(cl.player1->o).add(vec(0, 0, 8.f)), 0, 0, 10.f, 0, NULL, MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED);
			}
		}
		show_out_of_renderloop_progress(1.f, "pre-rendering.. entities done.");
	}
};
