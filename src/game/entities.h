#define AVOIDENEMY(x, y) (!m_team(cl.gamemode, cl.mutators) || (x)->team != (y)->team)

struct avoidset
{
    struct obstacle
    {
        dynent *ent;
        int numwaypoints;
        bool avoid;

        obstacle(dynent *ent) : ent(ent), numwaypoints(0) {}
    };

    gameclient &cl;
    vector<obstacle> obstacles;
    vector<int> waypoints;

    avoidset(gameclient &cl) : cl(cl) {}

    void clear()
    {
        obstacles.setsizenodelete(0);
        waypoints.setsizenodelete(0);
    }

    void add(dynent *ent, int waypoint)
    {
        if(obstacles.empty() || ent!=obstacles.last().ent) obstacles.add(obstacle(ent));
        obstacles.last().numwaypoints++;
        waypoints.add(waypoint);
    }

    #define loopavoid(v, d, body) \
        do { \
            int cur = 0; \
            loopv((v).obstacles) \
            { \
                avoidset::obstacle &ob = (v).obstacles[i]; \
                int next = cur + ob.numwaypoints; \
                if(ob.ent != (d) && (ob.ent->type != ENT_PLAYER || AVOIDENEMY((d), (gameent *)ob.ent))) \
                { \
                    for(; cur < next; cur++) \
                    { \
                        int ent = (v).waypoints[cur]; \
                        body; \
                    } \
                } \
                cur = next; \
            } \
        } while(0)

    bool find(int waypoint, gameent *d)
    {
        loopavoid(*this, d, { if(ent == waypoint) return true; });
        return false;
    }
};

struct entities : icliententities
{
	gameclient &cl;

	vector<extentity *> ents;
	vector<int> entlinks[MAXENTTYPES];

	IVARP(showentnames, 0, 1, 2);
	IVARP(showentinfo, 0, 1, 5);
	IVARP(showentnoisy, 0, 1, 2);
	IVARP(showentdir, 0, 1, 3);
	IVARP(showentradius, 0, 1, 3);
	IVARP(showentlinks, 0, 1, 3);
	IVARP(showlighting, 0, 1, 1);

	IVAR(dropwaypoints, 0, 0, 1); // drop waypoints during play
	bool autodropwaypoints;

	entities(gameclient &_cl) : cl(_cl), autodropwaypoints(false)
	{
        CCOMMAND(announce, "i", (entities *self, int *idx), self->announce(*idx));

        loopi(MAXENTTYPES)
        {
        	entlinks[i].setsize(0);
			switch(i)
			{
				//	NOTUSED = ET_EMPTY,				// 0  entity slot not in use in map
				//	LIGHT = ET_LIGHT,				// 1  radius, intensity
				case LIGHT:
					entlinks[i].add(SPOTLIGHT);
					break;
				//	MAPMODEL = ET_MAPMODEL,			// 2  idx, yaw, pitch, roll
				case MAPMODEL:
					entlinks[i].add(TRIGGER);
					break;
				//	PLAYERSTART = ET_PLAYERSTART,	// 3  angle, [team]
				//	ENVMAP = ET_ENVMAP,				// 4  radius
				//	PARTICLES = ET_PARTICLES,		// 5  type, [others]
				case PARTICLES:
				//	MAPSOUND = ET_SOUND,			// 6  idx, maxrad, minrad, volume
				case MAPSOUND:
					entlinks[i].add(TELEPORT);
					entlinks[i].add(TRIGGER);
					entlinks[i].add(PUSHER);
					break;
				//	SPOTLIGHT = ET_SPOTLIGHT,		// 7  radius
				//	WEAPON = ET_GAMESPECIFIC,		// 8  gun, ammo
				//	TELEPORT,						// 9  yaw, pitch, roll, push
				case TELEPORT:
					entlinks[i].add(MAPSOUND);
					entlinks[i].add(PARTICLES);
					entlinks[i].add(TELEPORT);
					break;
				//	OBSOLETED,						// 10
				//	TRIGGER,						// 11
				case TRIGGER:
					entlinks[i].add(MAPSOUND);
					entlinks[i].add(PARTICLES);
					break;
				//	PUSHER,							// 12 zpush, ypush, xpush
				case PUSHER:
					entlinks[i].add(MAPSOUND);
					entlinks[i].add(PARTICLES);
					break;
				//	FLAG,							// 13 idx, team
				case FLAG:
					entlinks[i].add(FLAG);
					break;
				//	CHECKPOINT,						// 14 idx
				case CHECKPOINT:
					entlinks[i].add(CHECKPOINT);
					break;
				//	CAMERA,							// 15 type, radius, weight
				case CAMERA:
					entlinks[i].add(CAMERA);
					entlinks[i].add(CONNECTION);
					break;
				//	WAYPOINT,						// 16 cmd
				case WAYPOINT:
					entlinks[i].add(WAYPOINT);
					break;
				//	ANNOUNCER,						// 17 maxrad, minrad, volume
				//	CONNECTION,						// 18
				case CONNECTION:
					entlinks[i].add(CONNECTION);
					break;
				//	MAXENTTYPES						// 19
				default: break;
			}
        }
	}

	vector<extentity *> &getents() { return ents; }

	const char *entinfo(int type, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0, bool full = false)
	{
		static string entinfostr;
		string str;
		entinfostr[0] = 0;
		#define addentinfo(s) { \
			if(entinfostr[0]) s_strcat(entinfostr, ", "); \
			s_strcat(entinfostr, s); \
		}
		if(type == PLAYERSTART || type == FLAG)
		{
			if(isteam(attr2, TEAM_ALPHA))
			{
				s_sprintf(str)("team %s", teamtype[attr2].name);
				addentinfo(str);
			}
			else if(attr2 == TEAM_NEUTRAL || attr2 == TEAM_ENEMY)
			{
				s_sprintf(str)("%s", teamtype[attr2].name);
				addentinfo(str);
			}
		}
		if(type == WEAPON)
		{
			if(isgun(attr1))
			{
				int a = clamp(attr2 > 0 ? attr2 : guntype[attr1].add, 1, guntype[attr1].max);
				if(full) s_sprintf(str)("\fs%s%s\fS, %d ammo", guntype[attr1].text, guntype[attr1].name, a);
				else s_sprintf(str)("\fs%s%s\fS", guntype[attr1].text, guntype[attr1].name);
				addentinfo(str);
			}
		}
		if(type == TRIGGER)
		{
			if(full)
			{
				const char *trignames[2][4] = {
						{ "unknown", "mapmodel", "script", "" },
						{ "unknown", "automatically", "by action", "by link" }
				};
				int tr = attr2 <= TR_NONE || attr2 >= TR_MAX ? TR_NONE : attr2,
					ta = attr3 <= TA_NONE || attr3 >= TA_MAX ? TA_NONE : attr3;
				s_sprintf(str)("%s activated %s", trignames[0][tr], trignames[1][ta]);
				addentinfo(str);
			}
		}
		if(type == WAYPOINT)
		{
			if(attr1 & WP_CROUCH) addentinfo("crouch");
		}
		return entinfostr;
	}

	const char *entmdlname(int type, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0)
	{
		switch(type)
		{
			case WEAPON: return guntype[attr1].item;
			case FLAG: return teamtype[attr2].flag;
			default: break;
		}
		return "";
	}

	void announce(int idx, const char *msg = "", bool force = false)
	{
		static int lastannouncement;
		if(idx > -1 && idx < S_MAX && (force || lastmillis-lastannouncement > 1000))
		{
			bool announcer = false;
			loopv(ents)
			{
				gameentity &e = *(gameentity *)ents[i];
				if(e.type == ANNOUNCER)
				{
					playsound(idx, 0, e.attr3, e.o, NULL, NULL, 0, e.attr1, e.attr2);
					announcer = true;
				}
			}
			if(!announcer)
			{ // if there's no announcer entities, just encompass the level
				loopi(13)
				{
					vec v;
					switch(i)
					{
						case 1:		v = vec(0, 0, 0.5f*getworldsize()); break;
						case 2:		v = vec(0, getworldsize(), 0.5f*getworldsize()); break;
						case 3:		v = vec(getworldsize(), getworldsize(), 0.5f*getworldsize()); break;
						case 4:		v = vec(getworldsize(), 0, 0.5f*getworldsize()); break;
						case 5:		v = vec(0, 0, getworldsize()); break;
						case 6:		v = vec(0, getworldsize(), getworldsize()); break;
						case 7:		v = vec(getworldsize(), getworldsize(), getworldsize()); break;
						case 8:		v = vec(getworldsize(), 0, getworldsize()); break;
						case 9:		v = vec(0, 0, 0.5f*getworldsize()); break;
						case 10:	v = vec(0, getworldsize(), 0); break;
						case 11:	v = vec(getworldsize(), getworldsize(), 0); break;
						case 12:	v = vec(getworldsize(), 0, 0); break;
						default:	v = vec(0.5f*getworldsize(), 0.5f*getworldsize(), 0.5f*getworldsize()); break;
					}
					playsound(idx, 0, 255, v, NULL, NULL, 0, getworldsize()*5/4, 0);
				}
			}
			lastannouncement = lastmillis;
		}
		if(*msg) console("\fg%s", (force ? CON_CENTER : 0)|CON_NORMAL, msg);
	}

	// these two functions are called when the server acknowledges that you really
	// picked up the item (in multiplayer someone may grab it before you).

	void useeffects(gameent *d, int n, int g, int r)
	{
		if(d && ents.inrange(n))
		{
			gameentity &e = *(gameentity *)ents[n];
			vec pos = e.o;
			loopv(cl.pj.projs)
			{
				projent &proj = *cl.pj.projs[i];
				if(proj.projtype != PRJ_ENT || proj.id != n) continue;
				pos = proj.o;
				proj.beenused = true;
				proj.state = CS_DEAD;
			}
			const char *item = entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
			if(item && (d != cl.player1 || cl.isthirdperson()))
			{
				s_sprintfd(ds)("@%s", item);
				particle_text(d->abovehead(), ds, 15);
			}
			playsound(S_ITEMPICKUP, 0, 255, d->o, d);
			if(isgun(g))
			{
				d->ammo[g] = d->entid[g] = -1;
				d->gunselect = g;
			}
			d->useitem(lastmillis, n, e.type, e.attr1, e.attr2);
			if(ents.inrange(r) && ents[r]->type == WEAPON && isgun(ents[r]->attr1))
				cl.pj.drop(d, ents[r]->attr1, r, (d->gunwait[ents[r]->attr1]/2)-50);
			regularshape(7, enttype[e.type].radius, 0x888822, 21, 50, 250, pos, 1.f);
			e.spawned = false;
		}
	}

	bool collateitems(gameent *d, bool use, vector<actitem> &actitems)
	{
		float eye = d->height*0.5f;
		vec m = d->o;
		m.z -= eye;

		loopv(ents)
		{
			extentity &e = *ents[i];
			if(e.type <= NOTUSED || e.type >= MAXENTTYPES || enttype[e.type].usetype == EU_NONE)
				continue;
			if(enttype[e.type].usetype == EU_ITEM && !e.spawned) continue;
			if(!insidesphere(m, eye, d->radius, e.o, enttype[e.type].height, enttype[e.type].radius))
				continue;
			actitem &t = actitems.add();
			t.type = ITEM_ENT;
			t.target = i;
			t.score = m.squaredist(e.o);
			break;
		}
		loopv(cl.pj.projs)
		{
			projent &proj = *cl.pj.projs[i];
			if(proj.projtype != PRJ_ENT || !proj.ready()) continue;
			if(enttype[proj.ent].usetype != EU_ITEM || !ents.inrange(proj.id))
				continue;
			if(!insidesphere(m, eye, d->radius, proj.o, enttype[proj.ent].height, enttype[proj.ent].radius))
				continue;
			actitem &t = actitems.add();
			t.type = ITEM_PROJ;
			t.target = i;
			t.score = m.squaredist(proj.o);
		}
		return !actitems.empty();
	}

	void checkitems(gameent *d)
	{
		static vector<actitem> actitems;
        actitems.setsizenodelete(0);
		if(collateitems(d, true, actitems))
		{
			while(!actitems.empty())
			{
				int closest = actitems.length()-1;
				loopv(actitems)
					if(actitems[i].score < actitems[closest].score)
						closest = i;

				actitem &t = actitems[closest];
				int ent = -1;
				switch(t.type)
				{
					case ITEM_ENT:
					{
						if(!ents.inrange(t.target)) break;
						ent = t.target;
						break;
					}
					case ITEM_PROJ:
					{
						if(!cl.pj.projs.inrange(t.target)) break;
						projent &proj = *cl.pj.projs[t.target];
						ent = proj.id;
						break;
					}
					default: break;
				}
				if(cl.et.ents.inrange(ent))
				{
					extentity &e = *ents[ent];
					if(d->canuse(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, lastmillis))
					{
						if(enttype[e.type].usetype == EU_ITEM && d->useaction)
						{
							cl.cc.addmsg(SV_ITEMUSE, "ri3", d->clientnum, lastmillis-cl.maptime, ent);
							d->useaction = false;
						}
						else if(enttype[e.type].usetype == EU_AUTO)
						{
							if(e.type != TRIGGER ||
								((e.spawned || !e.attr4) && ((e.attr3 == TA_ACT && d->useaction) || e.attr3 == TA_AUTO)))
							{
								reaction(t.target, d);
								if(e.type == TRIGGER && e.attr3 == TA_ACT)
									d->useaction = false;
							}
						}
					}
					else if(enttype[e.type].usetype == EU_ITEM && d->useaction)
					{
						d->useaction = false;
						playsound(S_DENIED, 0, 255, d->o, d);
					}
				}
				actitems.removeunordered(closest);
			}
		}
		if(m_ctf(cl.gamemode)) cl.ctf.checkflags(d);
	}

	void putitems(ucharbuf &p)
	{
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(enttype[e.type].usetype == EU_ITEM || e.type == TRIGGER)
			{
				putint(p, i);
				putint(p, e.type);
				putint(p, e.attr1);
				putint(p, e.attr2);
				putint(p, e.attr3);
				putint(p, e.attr4);
				putint(p, e.attr5);
				setspawn(i, m_noitems(cl.gamemode, cl.mutators) ? false : true);
			}
		}
	}

	void resetspawns() { loopv(ents) ents[i]->spawned = false; }
	void setspawn(int n, bool on)
	{
		loopv(cl.pj.projs)
		{
			projent &proj = *cl.pj.projs[i];
			if(proj.projtype != PRJ_ENT || proj.id != n)
				continue;
			proj.beenused = true;
			proj.state = CS_DEAD;
		}
		if(ents.inrange(n))
			if((ents[n]->spawned = on))
				((gameentity *)ents[n])->lastspawn = lastmillis;
	}

	extentity *newent() { return new gameentity(); }

	void fixentity(extentity &e)
	{
		gameentity &f = (gameentity &)e;

		if(issound(f.schan))
		{
			removesound(f.schan);
			f.schan = -1;
			if(f.type == MAPSOUND)
				f.lastemit = lastmillis; // prevent clipping when moving around
		}

		switch(e.type)
		{
			case WEAPON:
				while(e.attr1 < 0) e.attr1 += GUN_MAX;
				while(e.attr1 >= GUN_MAX) e.attr1 -= GUN_MAX;
				if(e.attr2 < 0) e.attr2 = 0;
				break;
			case PLAYERSTART:
			case FLAG:
				while(e.attr2 < 0) e.attr2 += TEAM_MAX;
				while(e.attr2 >= TEAM_MAX) e.attr2 -= TEAM_MAX;
				if(e.attr2 < 0) e.attr2 = TEAM_NEUTRAL;
			default:
				break;
		}
	}

	const char *findname(int type)
	{
		if(type >= NOTUSED && type < MAXENTTYPES) return enttype[type].name;
		return "";
	}

	int findtype(char *type)
	{
		loopi(MAXENTTYPES) if(!strcmp(type, enttype[i].name)) return i;
		return NOTUSED;
	}

	// these functions are called when the client touches the item
	void execlink(gameent *d, int index, bool local)
	{
		if(d && ents.inrange(index) && maylink(ents[index]->type))
		{
			if(local)
			{
				cl.cc.addmsg(SV_EXECLINK, "ri2", d->clientnum, index);
				if(d->ai) return;
			}

			gameentity &e = *(gameentity *)ents[index];

			loopv(ents)
			{
				gameentity &f = *(gameentity *)ents[i];
				if(f.links.find(index) >= 0)
				{
					bool both = e.links.find(i) >= 0;

					switch(f.type)
					{
						case MAPSOUND:
						{
							if((e.type == TRIGGER || e.type == TELEPORT || e.type == PUSHER) && mapsounds.inrange(f.attr1))
							{
								if(!issound(f.schan))
								{
									playsound(f.attr1, SND_MAP, f.attr4, both ? f.o : e.o, NULL, &f.schan, 0, f.attr2, f.attr3);
									f.lastemit = lastmillis;
									if(both) e.lastemit = lastmillis;
								}
							}
							break;
						}
						case PARTICLES:
						{
							if(e.type == TRIGGER || e.type == TELEPORT || e.type == PUSHER)
							{
								f.lastemit = lastmillis;
								if(both) e.lastemit = lastmillis;
							}
							break;
						}
						default: break;
					}
				}
			}
		}
	}


	void teleport(int n, gameent *d)
	{
		gameentity &e = *(gameentity *)ents[n];
		vector<int> teleports;
		loopv(e.links)
			if(ents.inrange(e.links[i]) && ents[e.links[i]]->type == TELEPORT)
				teleports.add(e.links[i]);

		while(!teleports.empty())
		{
			int r = rnd(teleports.length()), t = teleports[r];
			gameentity &f = *(gameentity *)ents[t];
			d->o = f.o;
			d->yaw = clamp((int)f.attr1, 0, 359);
			d->pitch = clamp((int)f.attr2, -89, 89);
			float mag = max((float)f.attr3+d->vel.magnitude(), 64.f);
			d->vel = vec(0, 0, 0);
			vecfromyawpitch(d->yaw, d->pitch, 1, 0, d->vel);
			d->o.add(d->vel);
			d->vel.mul(mag);
			if(cl.ph.entinmap(d, false))
			{
				execlink(d, n, true);
				execlink(d, t, true);
				if(d == cl.player1) cl.resetstates(ST_VIEW);
				break;
			}
			teleports.remove(r);
		}
	}

	void findplayerspawn(dynent *d, int forceent = -1, int tag = -1)   // place at random spawn. also used by monsters!
	{
		int pick = forceent;
		if(pick<0)
		{
			int r = cl.ph.fixspawn-->0 ? 7 : rnd(10)+1;
			loopi(r) cl.ph.spawncycle = findentity(PLAYERSTART, cl.ph.spawncycle+1, -1, tag);
			pick = cl.ph.spawncycle;
		}
		if(pick!=-1)
		{
			d->pitch = 0;
			d->roll = 0;
			for(int attempt = pick;;)
			{
				d->o = ents[attempt]->o;
				d->yaw = ents[attempt]->attr1;
				if(cl.ph.entinmap(d, true)) break;
				attempt = findentity(PLAYERSTART, attempt+1, -1, tag);
				if(attempt<0 || attempt==pick)
				{
					d->o = ents[attempt]->o;
					d->yaw = ents[attempt]->attr1;
					cl.ph.entinmap(d, false);
					break;
				}
			}
		}
		else
		{
			d->o.x = d->o.y = d->o.z = 0.5f*getworldsize();
			cl.ph.entinmap(d, false);
		}
	}

	void reaction(int n, gameent *d)
	{
		gameentity &e = *(gameentity *)ents[n];
		switch(e.type)
		{
			case TELEPORT:
			{
				if(d->lastuse == e.type && lastmillis-d->lastusemillis<1000) break;
				d->lastuse = e.type;
				d->lastusemillis = lastmillis;
				teleport(n, d);
				break;
			}

			case PUSHER:
			{
				if(d->lastuse==e.type && lastmillis-d->lastusemillis<1000) break;
				d->lastuse = e.type;
				d->lastusemillis = lastmillis;
				vec v((int)(char)e.attr3*10.0f, (int)(char)e.attr2*10.0f, e.attr1*12.5f);
				d->timeinair = 0;
                d->falling = vec(0, 0, 0);
				d->vel = v;
				execlink(d, n, true);
				break;
			}

			case TRIGGER:
			{
				if(d->lastuse==e.type && lastmillis-d->lastusemillis<1000)
					break;
				d->lastuse = e.type;
				d->lastusemillis = lastmillis;
				switch(e.type)
				{
					case TRIGGER:
					{
						switch(e.attr2)
						{
							case TR_SCRIPT:
							{
								s_sprintfd(s)("on_trigger_%d", e.attr1);
								RUNWORLD(s);
								break;
							}
							default: break;
						}
					}
					default: break;
				}
				execlink(d, n, true);
				break;
			}
		}
	}

	void editent(int i)
	{
		extentity &e = *ents[i];
		if(e.type == NOTUSED) linkclear(i);
		fixentity(e);
		if(multiplayer(false) && m_edit(cl.gamemode))
			cl.cc.addmsg(SV_EDITENT, "ri9i", i, (int)(e.o.x*DMF), (int)(e.o.y*DMF), (int)(e.o.z*DMF), e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5); // FIXME
	}

	float dropheight(entity &e)
	{
		if(e.type==MAPMODEL || e.type==FLAG) return 0.0f;
		return 4.0f;
	}

	bool maylink(int type, int ver = 0)
	{
		if(enttype[type].links && enttype[type].links <= (ver ? ver : GAMEVERSION))
				return true;
		return false;
	}

	bool canlink(int index, int node, bool msg = false)
	{
		if(ents.inrange(index) && ents.inrange(node))
		{
			if(index != node && maylink(ents[index]->type) && maylink(ents[node]->type) &&
					entlinks[ents[index]->type].find(ents[node]->type) >= 0)
						return true;
			if(msg)
				conoutf("\frentity %s (%d) and %s (%d) are not linkable", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);

			return false;
		}
		if(msg) conoutf("\frentity %d and %d are unable to be linked as one does not seem to exist", index, node);
		return false;
	}

	bool linkents(int index, int node, bool add, bool local, bool toggle)
	{
		if(ents.inrange(index) && ents.inrange(node) && index != node && canlink(index, node, local))
		{
			gameentity &e = *(gameentity *)ents[index], &f = *(gameentity *)ents[node];
			int g;

			if((toggle || !add) && (g = e.links.find(node)) >= 0)
			{
				int h;
				if(!add || (toggle && (!canlink(node, index) || (h = f.links.find(index)) >= 0)))
				{
					e.links.remove(g);
					if(local && m_edit(cl.gamemode))
						cl.cc.addmsg(SV_EDITLINK, "ri3", 0, index, node);

					if(verbose >= 3)
						conoutf("\fwentity %s (%d) and %s (%d) delinked", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
					return true;
				}
				else if(toggle && canlink(node, index))
				{
					f.links.add(index);
					if(local && m_edit(cl.gamemode))
						cl.cc.addmsg(SV_EDITLINK, "ri3", 1, node, index);

					if(verbose >= 3)
						conoutf("\fwentity %s (%d) and %s (%d) linked", enttype[ents[node]->type].name, node, enttype[ents[index]->type].name, index);
					return true;
				}
			}
			else if(toggle && canlink(node, index) && (g = f.links.find(index)) >= 0)
			{
				f.links.remove(g);
				if(local && m_edit(cl.gamemode))
					cl.cc.addmsg(SV_EDITLINK, "ri3", 0, node, index);

				if(verbose >= 3)
					conoutf("\fwentity %s (%d) and %s (%d) delinked", enttype[ents[node]->type].name, node, enttype[ents[index]->type].name, index);
				return true;
			}
			else if(toggle || add)
			{
				e.links.add(node);
				if(local && m_edit(cl.gamemode))
					cl.cc.addmsg(SV_EDITLINK, "ri3", 1, index, node);

				if(verbose >= 3)
					conoutf("\fwentity %s (%d) and %s (%d) linked", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
				return true;
			}
		}
		if(verbose >= 3)
			conoutf("\frentity %s (%d) and %s (%d) failed linking", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
		return false;
	}

	void linkclear(int n)
	{
		loopvj(ents) if(maylink(ents[j]->type))
		{
			gameentity &e = *(gameentity *)ents[j];

			loopvrev(e.links) if(e.links[i] == n)
			{
				e.links.remove(i);
				break;
			}
		}
	}

	struct linkq
	{
		uint id;
		float curscore, estscore;
		linkq *prev;

		linkq() : id(0), curscore(0.f), estscore(0.f), prev(NULL) {}

        float score() const { return curscore + estscore; }
	};

	bool route(gameent *d, int node, int goal, vector<int> &route, avoidset &obstacles, float tolerance, float *score = NULL)
	{
        if(!ents.inrange(node) || !ents.inrange(goal) || ents[goal]->type != ents[node]->type || goal == node) return false;

		static uint routeid = 1;
		static vector<linkq> nodes;
		static vector<linkq *> queue;

		int routestart = verbose > 3 ? SDL_GetTicks() : 0;

		if(!routeid)
		{
			loopv(nodes) nodes[i].id = 0;
			routeid = 1;
		}
		while(nodes.length() < ents.length()) nodes.add();

		loopavoid(obstacles, d,
		{
			if(ents[ent]->type == ents[node]->type)
			{
				nodes[ent].id = routeid;
				nodes[ent].curscore = -1.f;
				nodes[ent].estscore = 0.f;
			}
		});

		nodes[node].id = routeid;
		nodes[goal].curscore = nodes[node].curscore = 0.f;
		nodes[goal].estscore = nodes[node].estscore = 0.f;
		nodes[goal].prev = nodes[node].prev = NULL;
		queue.setsizenodelete(0);
		queue.add(&nodes[node]);
		route.setsizenodelete(0);

		int lowest = -1;
		while(!queue.empty())
		{
			int q = queue.length()-1;
			loopi(queue.length()-1) if(queue[i]->score() < queue[q]->score()) q = i;
			linkq *m = queue.removeunordered(q);
			float prevscore = m->curscore;
			m->curscore = -1.f;
			extentity &ent = *ents[m - &nodes[0]];
			vector<int> &links = ent.links;
			loopv(links)
			{
				int link = links[i];
				if(ents.inrange(link) && ents[link]->type == ents[node]->type)
				{
					linkq &n = nodes[link];
					float curscore = prevscore + ents[link]->o.dist(ent.o);
					if(n.id == routeid && curscore >= n.curscore) continue;
					n.curscore = curscore;
					n.prev = m;
					if(n.id != routeid)
					{
						n.estscore = ents[link]->o.dist(ents[goal]->o);
						if(n.estscore <= tolerance && (lowest < 0 || n.estscore < nodes[lowest].estscore))
							lowest = link;
						if(link != goal) queue.add(&n);
						else queue.setsizenodelete(0);
						n.id = routeid;
					}
				}
			}
		}

        routeid++;

		if(lowest >= 0) // otherwise nothing got there
		{
			for(linkq *m = &nodes[lowest]; m != NULL; m = m->prev)
				route.add(m - &nodes[0]); // just keep it stored backward
			if(!route.empty() && score) *score = nodes[lowest].score();
		}

		if(verbose >= 4)
			conoutf("\fwroute %d to %d (%d) generated %d nodes in %fs", node, goal, lowest, route.length(), (SDL_GetTicks()-routestart)/1000.0f);

		return !route.empty();
	}

	bool waypointdrop()
	{
		return (m_fight(cl.gamemode) && autodropwaypoints) || dropwaypoints();
	}

	int waypointnode(const vec &v, bool dist = true, int type = WAYPOINT)
	{
		int w = -1, t = entlinks[WAYPOINT].find(type) >= 0 ? type : WAYPOINT;
        float mindist = dist ? enttype[t].radius * enttype[t].radius : 1e16f; // avoid square roots
        loopv(ents) if(ents[i]->type == t)
        {
            float u = ents[i]->o.squaredist(v);
            if(u <= mindist)
            {
                w = i;
                mindist = u;
            }
        }
		return w;
	}

	void waypointlink(int index, int node, bool both = true)
	{
		if(ents.inrange(index) && ents.inrange(node))
		{
			gameentity &e = *(gameentity *)ents[index], &f = *(gameentity *)ents[node];

			if(e.links.find(node) < 0)
				linkents(index, node, true, true, false);
			if(both && f.links.find(index) < 0)
				linkents(node, index, true, true, false);
		}
	}

	void waypointcheck(gameent *d)
	{
		if(d->state == CS_ALIVE)
		{
			vec v(cl.feetpos(d, 0.f));
			int curnode = waypointnode(v);
			if(waypointdrop() && ((m_fight(cl.gamemode) && d->aitype == AI_NONE) || d == cl.player1))
			{
				if(!ents.inrange(curnode) && ents.inrange(d->lastnode) && ents[d->lastnode]->o.dist(v) <= d->radius+enttype[WAYPOINT].radius)
					curnode = d->lastnode;

				if(!ents.inrange(curnode))
				{
					int cmds = WP_NONE;
					if(d->crouching) cmds |= WP_CROUCH;
					curnode = ents.length();
					newentity(v, WAYPOINT, cmds, 0, 0, 0, 0);
				}
				if(ents.inrange(d->lastnode) && d->lastnode != curnode)
					waypointlink(d->lastnode, curnode, !d->timeinair);

				d->lastnode = curnode;
			}
			else
			{
				if(!ents.inrange(curnode)) curnode = waypointnode(v, false);
				if(ents.inrange(curnode))
				{
					if(d->lastnode != curnode) d->oldnode = d->lastnode;
					d->lastnode = curnode;
				}
				else if(ents.inrange(d->oldnode)) d->lastnode = d->oldnode;
				else d->lastnode = d->oldnode = -1;
			}
		}
		else d->lastnode = d->oldnode = -1;
	}

	void readent(gzFile &g, int mtype, int mver, char *gid, int gver, int id, entity &e)
	{
		gameentity &f = (gameentity &)e;

		f.mark = false;

		if(mtype == MAP_OCTA)
		{
			// sauerbraten version increments
			if(mver <= 10) if(f.type >= 7) f.type++;
			if(mver <= 12) if(f.type >= 8) f.type++;

			// now translate into our format
			switch(f.type)
			{
				// 1	LIGHT			1	LIGHT
				// 2	MAPMODEL		2	MAPMODEL
				// 3	PLAYERSTART		3	PLAYERSTART
				// 4	ENVMAP			4	ENVMAP
				// 5	PARTICLES		5	PARTICLES
				// 6	MAPSOUND		6	MAPSOUND
				// 7	SPOTLIGHT		7	SPOTLIGHT
				case 1: case 2: case 3: case 4: case 5: case 6: case 7:
				{
					break;
				}

				// 8	I_SHELLS		8	WEAPON		GUN_SG
				// 9	I_BULLETS		8	WEAPON		GUN_CG
				// 10	I_ROCKETS		8	WEAPON		GUN_FLAMER
				// 11	I_ROUNDS		8	WEAPON		GUN_RIFLE
				// 12	I_GRENADES		8	WEAPON		GUN_GL
				// 13	I_CARTRIDGES	8	WEAPON		GUN_PISTOL
				case 8: case 9: case 10: case 11: case 12: case 13:
				{
					int gun = f.type-8, gunmap[6] = {
						GUN_SG, GUN_CG, GUN_FLAMER, GUN_RIFLE, GUN_GL, GUN_PISTOL
					};

					if(gun <= 5 && gun >= 0 && gunmap[gun])
					{
						f.type = WEAPON;
						f.attr1 = gunmap[gun];
						f.attr2 = 0;
					}
					else f.type = NOTUSED;
					break;
				}
				// 19	TELEPORT		9	TELEPORT
				// 20	TELEDEST		9	TELEPORT (linked)
				case 19: case 20:
				{
					if(f.type == 20) f.mark = true; // needs translating later
					f.type = TELEPORT;
					break;
				}
				// 21	MONSTER			10	OBSOLETED
				case 21:
				{
					f.type = OBSOLETED;
					break;
				}
				// 22	CARROT			11	TRIGGER		0
				case 22:
				{
					f.type = TRIGGER;
					f.attr1 = f.attr2 = 0;
					break;
				}
				// 23	JUMPPAD			12	PUSHER
				case 23:
				{
					f.type = PUSHER;
					break;
				}
				// 24	BASE			13	FLAG		1:idx		TEAM_NEUTRAL
				case 24:
				{
					f.type = FLAG;
					if(f.attr1 < 0) f.attr1 = 0;
					f.attr2 = TEAM_NEUTRAL; // spawn as neutrals
					break;
				}
				// 25	RESPAWNPOINT	14	CHECKPOINT
				case 25:
				{
					f.type = CHECKPOINT;
					break;
				}
				// 30	FLAG			13	FLAG		#			2:team
				case 30:
				{
					f.type = FLAG;
					f.attr1 = 0;
					if(f.attr2 <= 0) f.attr2 = -1; // needs a team
					break;
				}

				// 14	I_HEALTH		-	NOTUSED
				// 15	I_BOOST			-	NOTUSED
				// 16	I_GREENARMOUR	-	NOTUSED
				// 17	I_YELLOWARMOUR	-	NOTUSED
				// 18	I_QUAD			-	NOTUSED
				// 26	BOX				-	NOTUSED
				// 27	BARREL			-	NOTUSED
				// 28	PLATFORM		-	NOTUSED
				// 29	ELEVATOR		-	NOTUSED
				default:
				{
					conoutf("\frWARNING: ignoring entity %d type %d", id, f.type);
					f.type = NOTUSED;
					break;
				}
			}
		}
	}

	void writeent(gzFile &g, int id, entity &e)
	{
	}

	void initents(gzFile &g, int mtype, int mver, char *gid, int gver)
	{
		if(gver <= 49 || mtype == MAP_OCTA)
		{
			int flag = 0, teams[TEAM_MAX-TEAM_ALPHA] = { 0, 0, 0, 0 };
			vector<short> teleyaw;
			loopv(ents) teleyaw.add(0);

			loopv(ents)
			{
				gameentity &e = *(gameentity *)ents[i];

				if(e.type == TELEPORT && e.mark) // translate teledest to teleport and link them appropriately
				{
					int dest = -1;

					loopvj(ents) // see if this guy is sitting on top of a teleport already
					{
						gameentity &f = *(gameentity *)ents[j];

						if(f.type == TELEPORT && !f.mark &&
							(!ents.inrange(dest) || e.o.dist(f.o) < ents[dest]->o.dist(f.o)) &&
								e.o.dist(f.o) <= enttype[TELEPORT].radius*4.f)
									dest = j;
					}

					if(ents.inrange(dest))
					{
						conoutf("\frWARNING: replaced teledest %d with closest teleport %d", i, dest);
						e.type = NOTUSED; // get rid of this guy then
					}
					else
					{
						conoutf("\frWARNING: modified teledest %d to a teleport", i);
						dest = i;
					}

					teleyaw[dest] = e.attr1; // store the yaw for later

					loopvj(ents) // find linked teleport(s)
					{
						gameentity &f = *(gameentity *)ents[j];

						if(f.type == TELEPORT && !f.mark && f.attr1 == e.attr2)
						{
							if(verbose) conoutf("\frimported teleports %d and %d linked automatically", dest, j);
							f.links.add(dest);
						}
					}
				}

				if(e.type == FLAG) // replace bases/neutral flags near team flags
				{
					if(isteam(e.attr2, TEAM_ALPHA)) teams[e.attr2-TEAM_ALPHA]++;
					else if(e.attr2 == TEAM_NEUTRAL)
					{
						int dest = -1;

						loopvj(ents)
						{
							gameentity &f = *(gameentity *)ents[j];

							if(f.type == FLAG && f.attr2 != TEAM_NEUTRAL &&
								(!ents.inrange(dest) || e.o.dist(f.o) < ents[dest]->o.dist(f.o)) &&
									e.o.dist(f.o) <= enttype[FLAG].radius*4.f)
										dest = j;
						}

						if(ents.inrange(dest))
						{
							gameentity &f = *(gameentity *)ents[dest];
							conoutf("\frWARNING: old base %d (%d, %d) replaced with flag %d (%d, %d)", i, e.attr1, e.attr2, dest, f.attr1, f.attr2);
							if(!f.attr1) f.attr1 = e.attr1; // give it the old base idx
							e.type = NOTUSED;
						}
						else if(e.attr1 > flag) flag = e.attr1; // find the highest idx
					}
				}
			}

			loopv(ents)
			{
				gameentity &e = *(gameentity *)ents[i];

				switch(e.type)
				{
					case TELEPORT:
					{
						e.attr1 = teleyaw[i]; // grab what we stored earlier
						e.attr2 = 0;
						e.mark = false;
						break;
					}
					case WAYPOINT:
					{
						e.attr1 = WP_NONE;
						break;
					}
					case FLAG:
					{
						if(!e.attr1) e.attr1 = ++flag; // assign a sane idx
						if(!isteam(e.attr2, TEAM_NEUTRAL)) // assign a team
						{
							int lowest = -1;
							loopk(TEAM_MAX-TEAM_ALPHA)
								if(lowest<0 || teams[k] < teams[lowest])
									lowest = i;
							e.attr2 = lowest+TEAM_ALPHA;
							teams[lowest]++;
						}
						break;
					}
				}
			}
		}

		physent dummyent;
		dummyent.height = dummyent.radius = dummyent.xradius = dummyent.yradius = 1;
		int waypoints = 0;
		loopvj(ents)
		{
			gameentity &e = *(gameentity *)ents[j];
			loopvrev(e.links) if(!canlink(j, e.links[i], true)) e.links.remove(i);

			switch(e.type)
			{
				case WEAPON:
				{
					if(gver <= 90)
					{
						if(e.attr1 > 3) e.attr1--;
						else if(e.attr1 == 3) e.attr1 = GUN_GL;
					}
					break;
				}
				case WAYPOINT:
				{
					waypoints++;
					if(gver <= 90)
						e.attr1 = e.attr2 = e.attr3 = e.attr4 = e.attr5 = 0;
					break;
				}
				default: break;
			}
		}
		autodropwaypoints = m_fight(cl.gamemode) && !waypoints;
	}

	void mapstart()
	{
		if(autodropwaypoints)
		{
			loopv(ents)
			{
				gameentity &e = *(gameentity *)ents[i];
				vec v(e.o);
				v.z -= dropheight(e);
				switch(e.type)
				{
					case PLAYERSTART:
					case WEAPON:
					{
						newentity(v, WAYPOINT, 0, 0, 0, 0, 0);
						break;
					}
					default: break;
				}
			}
		}
	}

	#define renderfocus(i,f) { gameentity &e = *(gameentity *)ents[i]; f; }

	void renderlinked(gameentity &e)
	{
		loopv(e.links)
		{
			int index = e.links[i];

			if(ents.inrange(index))
			{
				gameentity &f = *(gameentity *)ents[index];
				bool both = false;

				loopvj(f.links)
				{
					int g = f.links[j];
					if(ents.inrange(g))
					{
						gameentity &h = *(gameentity *)ents[g];
						if(&h == &e)
						{
							both = true;
							break;
						}
					}
				}

				vec fr(vec(e.o).add(vec(0, 0, RENDERPUSHZ))),
					dr(vec(f.o).add(vec(0, 0, RENDERPUSHZ)));
				vec col(0.5f, both ? 0.25f : 0.0f, 0.f);
				renderline(fr, dr, col.x, col.y, col.z, false);
				dr.sub(fr);
				dr.normalize();
				float yaw, pitch;
				vectoyawpitch(dr, yaw, pitch);
				dr.mul(RENDERPUSHX);
				dr.add(fr);
				rendertris(dr, yaw, pitch, 2.f, col.x*2.f, col.y*2.f, col.z*2.f, true, false);
			}
		}
	}

	void renderentshow(gameentity &e, int level)
	{
		if(!level || showentradius() >= level)
		{
			switch(e.type)
			{
				case MAPSOUND:
					renderradius(e.o, e.attr2, e.attr2, e.attr2, false);
					renderradius(e.o, e.attr3, e.attr3, e.attr3, false);
					break;
				case LIGHT:
				{
					int s = e.attr1 ? e.attr1 : hdr.worldsize;
					renderradius(e.o, s, s, s, false);
					break;
				}
				case ANNOUNCER:
					renderradius(e.o, e.attr1, e.attr1, e.attr1, false);
					renderradius(e.o, e.attr2, e.attr2, e.attr2, false);
					break;
				default:
					if(enttype[e.type].radius || enttype[e.type].height)
						renderradius(e.o, enttype[e.type].radius, enttype[e.type].radius, enttype[e.type].height, false);
					break;
			}
		}

		switch(e.type)
		{
			case PLAYERSTART:
			{
				if(!level || showentdir() >= level) renderdir(e.o, e.attr1, 0.f, false);
				break;
			}
			case MAPMODEL:
			{
				if(!level || showentdir() >= level) renderdir(e.o, e.attr2, e.attr3, false);
				break;
			}
			case TELEPORT:
			{
				if(!level || showentdir() >= level) renderdir(e.o, e.attr1, e.attr2, false);
				break;
			}
			case CAMERA:
			{
				if(!level || showentdir() >= level) renderdir(e.o, e.attr1, e.attr2, false);
				break;
			}
			default:
				break;
		}

		if(enttype[e.type].links)
			if(!level || showentlinks() >= level || (e.type == WAYPOINT && dropwaypoints()))
				renderlinked(e);
	}

	void renderentlight(gameentity &e)
	{
		vec color(e.attr2, e.attr3, e.attr4);
		color.div(255.f);
		adddynlight(vec(e.o), float(e.attr1 ? e.attr1 : hdr.worldsize), color);
	}

	void adddynlights()
	{
		if(editmode && showlighting())
		{
			loopv(ents)
			{
				if((entgroup.find(i) >= 0 || enthover == i) && ents[i]->type == LIGHT && ents[i]->attr1 > 0)
				{
					renderfocus(i, renderentlight(e));
				}
			}
		}
	}

    void preload()
    {
        if(!m_noitems(cl.gamemode, cl.mutators))
		{
			loopv(ents)
        	{
        		extentity &e = *ents[i];
				const char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
            	if(mdlname && *mdlname) loadmodel(mdlname, -1, true);
            	if(e.type == WEAPON && isgun(e.attr1)) cl.ws.preload(e.attr1);
			}
        }
    }

 	void update()
	{
		waypointcheck(cl.player1);
		loopv(cl.players) if(cl.players[i]) waypointcheck(cl.players[i]);
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(e.type == MAPSOUND && !e.links.length() && lastmillis-e.lastemit > 500 && mapsounds.inrange(e.attr1))
			{
				if(!issound(e.schan))
				{
					playsound(e.attr1, SND_MAP|SND_LOOP, e.attr4, e.o, NULL, &e.schan, 0, e.attr2, e.attr3);
					e.lastemit = lastmillis; // prevent clipping when moving around
				}
			}
		}
	}

	void render()
	{
		if(rendernormally) // important, don't render lines and stuff otherwise!
		{
			int level = (m_edit(cl.gamemode) ? 2 : ((showentdir()==3 || showentradius()==3 || showentlinks()==3 || dropwaypoints()) ? 3 : 0));
			if(level)
			{
				renderprimitive(true);
				loopv(ents)
				{
					renderfocus(i, renderentshow(e, editmode && (entgroup.find(i) >= 0 || enthover == i) ? 1 : level));
				}
				renderprimitive(false);
			}
		}

		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
			bool active = (enttype[e.type].usetype == EU_ITEM && e.spawned);
			if(m_edit(cl.gamemode) || active)
			{
				const char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
				if(mdlname && *mdlname)
				{
					int flags = MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED;
					if(!active) flags |= MDL_TRANSLUCENT;

					rendermodel(&e.light, mdlname, ANIM_MAPMODEL|ANIM_LOOP,
							e.o, 0.f, 0.f, 0.f, flags);
				}
			}
		}
	}

	void drawparticle(gameentity &e, vec &o, int idx, bool spawned)
	{
		if(e.type == NOTUSED) return;
		string s;
		if(e.type == PARTICLES && o.dist(camera1->o) <= maxparticledistance)
		{
			if(idx >= 0 || !e.links.length()) makeparticles((entity &)e);
			else if(lastmillis-e.lastemit < 500)
			{
				bool both = false;

				loopvk(e.links) if(ents.inrange(e.links[k]))
				{
					gameentity &f = *(gameentity *)ents[e.links[k]];
					if(f.links.find(idx) >= 0 && lastmillis-f.lastemit < 500)
					{
						makeparticle(f.o, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
						both = true;
					}
				}

				if(!both) // hasn't got an active reciprocal link (fallback)
					makeparticle(o, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
			}
		}

		if(m_edit(cl.gamemode))
		{
			if((showentinfo() || editmode) && (!enttype[e.type].noisy || showentnoisy() >= 2 || (showentnoisy() && editmode)))
			{
				bool hasent = idx >= 0 && (entgroup.find(idx) >= 0 || enthover == idx);
				vec off(0, 0, 2.f), pos(o);
				part_create(3, 1, pos, hasent ? 0xFF6600 : 0xFFFF00, hasent ? 2.0f : 1.5f);
				if(showentinfo() >= 2 || editmode)
				{
					s_sprintf(s)("@%s%s (%d)", hasent ? "\fo" : "\fy", enttype[e.type].name, idx >= 0 ? idx : 0);
					particle_text(pos.add(off), s, hasent ? 26 : 27, 1);

					if(showentinfo() >= 3 || hasent)
					{
						s_sprintf(s)("@%s%d %d %d %d %d", hasent ? "\fw" : "\fy", e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
						particle_text(pos.add(off), s, hasent ? 27 : 24, 1);
					}
					if(showentinfo() >= 4 || hasent)
					{
						s_sprintf(s)("@%s%s", hasent ? "\fw" : "\fy", entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, showentinfo() >= 5 || hasent));
						particle_text(pos.add(off), s, hasent ? 27 : 24, 1);
					}
				}
			}
		}
		else if(showentnames())
		{
			if(e.type == WEAPON && spawned)
			{
				s_sprintf(s)("@%s", entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, false));
				particle_text(vec(o).add(vec(0, 0, 4)), s, 26, 1);
			}
		}
	}

	void drawparticles()
	{
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(e.type == NOTUSED) continue;
			drawparticle(e, e.o, i, e.spawned);
		}
		loopv(cl.pj.projs)
		{
			projent &proj = *cl.pj.projs[i];
			if(proj.projtype != PRJ_ENT || !ents.inrange(proj.id))
				continue;
			gameentity &e = *(gameentity *)ents[proj.id];
			drawparticle(e, proj.o, -1, proj.ready());
		}
	}
} et;
