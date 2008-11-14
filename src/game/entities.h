struct avoidset
{
    struct obstacle
    {
        dynent *ent;
        int numentities;
        bool avoid;

        obstacle(dynent *ent) : ent(ent), numentities(0) {}
    };

    gameclient &cl;
    vector<obstacle> obstacles;
    vector<int> entities;

    avoidset(gameclient &cl) : cl(cl) {}

    void clear()
    {
        obstacles.setsizenodelete(0);
        entities.setsizenodelete(0);
    }

    void add(dynent *ent, int entity)
    {
        if(obstacles.empty() || ent!=obstacles.last().ent) obstacles.add(obstacle(ent));
        obstacles.last().numentities++;
        entities.add(entity);
    }

    #define loopavoid(v, d, body) \
        if(!(v).obstacles.empty()) \
        { \
            int cur = 0; \
            loopv((v).obstacles) \
            { \
                avoidset::obstacle &ob = (v).obstacles[i]; \
                int next = cur + ob.numentities; \
                if(ob.ent && ob.ent != (d)) \
                { \
                    for(; cur < next; cur++) \
                    { \
                        int ent = (v).entities[cur]; \
                        body; \
                    } \
                } \
                cur = next; \
            } \
        }

    bool find(int entity, gameent *d)
    {
        loopavoid(*this, d, { if(ent == entity) return true; });
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
	IVARP(showentnoisy, 0, 0, 2);
	IVARP(showentdir, 0, 1, 3);
	IVARP(showentradius, 0, 1, 3);
	IVARP(showentlinks, 0, 1, 3);
	IVARP(showlighting, 0, 1, 1);

	IVAR(dropentities, 0, 0, 1); // drop entities during play
	bool autodropentities;

	entities(gameclient &_cl) : cl(_cl), autodropentities(false)
	{
        CCOMMAND(announce, "i", (entities *self, int *idx), self->announce(*idx));

        loopi(MAXENTTYPES)
        {
        	entlinks[i].setsize(0);
			switch(i)
			{
				case LIGHT:
					entlinks[i].add(SPOTLIGHT);
					break;
				case MAPMODEL:
					entlinks[i].add(MAPSOUND);
					entlinks[i].add(PARTICLES);
					entlinks[i].add(TRIGGER);
					break;
				case PARTICLES:
					entlinks[i].add(MAPMODEL);
					entlinks[i].add(TELEPORT);
					entlinks[i].add(TRIGGER);
					entlinks[i].add(PUSHER);
					break;
				case MAPSOUND:
					entlinks[i].add(MAPMODEL);
					entlinks[i].add(TELEPORT);
					entlinks[i].add(TRIGGER);
					entlinks[i].add(PUSHER);
					break;
				case TELEPORT:
					entlinks[i].add(MAPSOUND);
					entlinks[i].add(PARTICLES);
					entlinks[i].add(TELEPORT);
					break;
				case RESERVED:
					break;
				case TRIGGER:
					entlinks[i].add(MAPMODEL);
					entlinks[i].add(MAPSOUND);
					entlinks[i].add(PARTICLES);
					break;
				case PUSHER:
					entlinks[i].add(MAPSOUND);
					entlinks[i].add(PARTICLES);
					break;
				case FLAG:
					entlinks[i].add(FLAG);
					break;
				case CHECKPOINT:
					entlinks[i].add(CHECKPOINT);
					break;
				case CAMERA:
					entlinks[i].add(CAMERA);
					entlinks[i].add(CONNECTION);
					break;
				case WAYPOINT:
					entlinks[i].add(WAYPOINT);
					break;
				case CONNECTION:
					entlinks[i].add(CONNECTION);
					break;
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
		if(type == MAPSOUND)
		{
			if(full)
			{
				if(mapsounds.inrange(attr1))
					addentinfo(mapsounds[attr1].sample->name);
				if(attr5 & 0x01) addentinfo("noatten");
				if(attr5 & 0x02) addentinfo("nodelay");
				if(attr5 & 0x04) addentinfo("nocull");
			}
		}
		if(type == TRIGGER)
		{
			if(full)
			{
				const char *trignames[2][4] = {
						{ "toggle", "links", "script", "" },
						{ "(disabled)", "(proximity)", "(action)" }
				};
				int tr = attr2 <= TR_NONE || attr2 >= TR_MAX ? TR_NONE : attr2,
					ta = attr3 <= TA_NONE || attr3 >= TA_MAX ? TA_NONE : attr3;
				s_sprintf(str)("execute %s %s", trignames[0][tr], trignames[1][ta]);
				addentinfo(str);
			}
		}
		if(type == WAYPOINT)
		{
			if(full)
			{
				if(attr1 & WP_CROUCH) addentinfo("crouch");
			}
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
		if(idx > -1 && idx < S_MAX && (force || lastmillis-lastannouncement >= TRIGGERTIME))
		{
			bool announcer = false;
			loopv(ents)
			{
				gameentity &e = *(gameentity *)ents[i];
				if(e.type == ANNOUNCER)
				{
					playsound(idx, e.o, NULL, 0, e.attr3, e.attr1, e.attr2);
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
					playsound(idx, v);
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
		if(ents.inrange(n))
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
				part_text(d->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 3.f);
			}
			playsound(S_ITEMPICKUP, d->o, d);
			if(isgun(g))
			{
				d->ammo[g] = d->entid[g] = -1;
				d->gunselect = g;
			}
			d->useitem(lastmillis, n, e.type, e.attr1, e.attr2);
			if(ents.inrange(r) && ents[r]->type == WEAPON && isgun(ents[r]->attr1))
				cl.pj.drop(d, ents[r]->attr1, r, (d->gunwait[ents[r]->attr1]/2)-50);
			regularshape(PART_PLASMA, enttype[e.type].radius, 0x888822, 53, 50, 200, pos, 2.f);
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
			float radius = (float)enttype[e.type].radius;
			if((e.type == TRIGGER || e.type == TELEPORT || e.type == PUSHER) && e.attr4)
				radius = (float)e.attr4;
			if(!insidesphere(m, eye, d->radius, e.o, radius, radius))
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
			if(!insidesphere(m, eye, d->radius, proj.o, enttype[proj.ent].radius, enttype[proj.ent].radius))
				continue;
			actitem &t = actitems.add();
			t.type = ITEM_PROJ;
			t.target = i;
			t.score = m.squaredist(proj.o);
		}
		return !actitems.empty();
	}

	void execitem(int n, gameent *d)
	{
		gameentity &e = *(gameentity *)ents[n];
		if(d->canuse(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, lastmillis))
		{
			if(enttype[e.type].usetype == EU_ITEM && d->useaction && d->requse < 0)
			{
				cl.cc.addmsg(SV_ITEMUSE, "ri3", d->clientnum, lastmillis-cl.maptime, n);
				d->useaction = false;
				d->requse = lastmillis;
			}
			else if(enttype[e.type].usetype == EU_AUTO)
			{
				if(e.type != TRIGGER || ((e.attr3 == TA_ACT && d->useaction && d == cl.player1) || e.attr3 == TA_AUTO))
				{
					switch(e.type)
					{
						case TELEPORT:
						{
							if(lastmillis-e.lastuse >= TRIGGERTIME)
							{
								e.lastuse = e.lastemit = lastmillis;
								static vector<int> teleports;
								teleports.setsize(0);
								loopv(e.links)
									if(ents.inrange(e.links[i]) && ents[e.links[i]]->type == e.type)
										teleports.add(e.links[i]);

								while(!teleports.empty())
								{
									int r = e.type == TELEPORT ? rnd(teleports.length()) : 0, t = teleports[r];
									gameentity &f = *(gameentity *)ents[t];
									d->timeinair = 0;
									d->falling = vec(0, 0, 0);
									d->o = vec(f.o).sub(vec(0, 0, d->height/2));
									if(cl.ph.entinmap(d, false))
									{
										d->yaw = f.attr1;
										d->pitch = f.attr2;
										float mag = max(d->vel.magnitude(), f.attr3 ? float(f.attr3) : 100.f);
										vecfromyawpitch(d->yaw, d->pitch, 1, 0, d->vel);
										d->vel.mul(mag);
										cl.fixfullrange(d->yaw, d->pitch, d->roll, true);
										f.lastuse = f.lastemit = e.lastemit;
										execlink(d, n, true);
										execlink(d, t, true);
										if(d == cl.player1) cl.resetstates(ST_VIEW);
										break;
									}
									teleports.remove(r); // must've really sucked, try another one
								}
							}
							break;
						}

						case PUSHER:
						{
							vec dir((int)(char)e.attr3*10.f, (int)(char)e.attr2*10.f, e.attr1*10.f);
							d->timeinair = 0;
							d->falling = vec(0, 0, 0);
							loopk(3)
							{
								if((d->vel.v[k] > 0.f && dir.v[k] < 0.f) || (d->vel.v[k] < 0.f && dir.v[k] > 0.f) || (fabs(dir.v[k]) > fabs(d->vel.v[k])))
									d->vel.v[k] = dir.v[k];
							}
							if(lastmillis-e.lastuse >= TRIGGERTIME)
							{
								e.lastuse = e.lastemit = lastmillis;
								execlink(d, n, true);
							}
							break;
						}

						case TRIGGER:
						{
							if(lastmillis-e.lastuse >= TRIGGERTIME)
							{
								e.lastuse = lastmillis;
								switch(e.attr2)
								{
									case TR_NONE: case TR_LINK:
									{ // wait for ack
										cl.cc.addmsg(SV_TRIGGER, "ri2", d->clientnum, n);
										break;
									}
									case TR_SCRIPT:
									{
										if(d == cl.player1)
										{
											s_sprintfd(s)("on_trigger_%d", e.attr1);
											RUNWORLD(s);
										}
										break;
									}
									default: break;
								}
								if(e.attr3 == TA_ACT) d->useaction = false;
							}
							break;
						}
					}
				}
			}
		}
		else if(enttype[e.type].usetype == EU_ITEM && d->useaction)
		{
			d->useaction = false;
			if(d == cl.player1) playsound(S_DENIED, d->o, d);
		}
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
				if(ents.inrange(ent)) execitem(ent, d);
				actitems.removeunordered(closest);
			}
		}
		if(m_ctf(cl.gamemode)) cl.ctf.checkflags(d);
	}

	void putitems(ucharbuf &p)
	{
		loopv(ents) if(enttype[ents[i]->type].usetype == EU_ITEM || ents[i]->type == TRIGGER)
		{
			gameentity &e = *(gameentity *)ents[i];
			putint(p, i);
			putint(p, e.type);
			putint(p, e.attr1);
			putint(p, e.attr2);
			putint(p, e.attr3);
			putint(p, e.attr4);
			putint(p, e.attr5);
			setspawn(i, false);
		}
	}

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
		{
			gameentity &e = *(gameentity *)ents[n];
			if((e.spawned = on)) e.lastspawn = lastmillis;
			if(e.type == TRIGGER && (e.attr2 == TR_NONE || e.attr2 == TR_LINK))
			{
				int millis = lastmillis-e.lastemit;
				if(e.lastemit && millis < TRIGGERTIME) // skew the animation forward
					e.lastemit = lastmillis-(TRIGGERTIME-millis);
				else e.lastemit = lastmillis;
				execlink(NULL, n, false);
			}
		}
	}

	extentity *newent() { return new gameentity(); }

	bool cansee(extentity &e)
	{
		return (showentinfo() || cl.player1->state == CS_EDITING) && (!enttype[e.type].noisy || showentnoisy() >= 2 || (showentnoisy() && cl.player1->state == CS_EDITING));
	}

	void fixentity(extentity &e)
	{
		gameentity &f = (gameentity &)e;

		if(issound(f.schan))
		{
			removesound(f.schan);
			f.schan = -1; // prevent clipping when moving around
			if(f.type == MAPSOUND) f.lastemit = lastmillis;
		}

		switch(e.type)
		{
			case WEAPON:
				while(e.attr1 < 0) e.attr1 += GUN_MAX;
				while(e.attr1 >= GUN_MAX) e.attr1 -= GUN_MAX;
				break;
			case PLAYERSTART:
				while(e.attr1 < 0) e.attr1 += 360;
				while(e.attr1 >= 360) e.attr1 -= 360;
			case FLAG:
				while(e.attr2 < 0) e.attr2 += TEAM_MAX;
				while(e.attr2 >= TEAM_MAX) e.attr2 -= TEAM_MAX;
				break;
			case TELEPORT:
				while(e.attr1 < 0) e.attr1 += 360;
				while(e.attr1 >= 360) e.attr1 -= 360;
				while(e.attr2 < 0) e.attr2 += 360;
				while(e.attr2 >= 360) e.attr2 -= 360;
				break;
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
		if(ents.inrange(index) && maylink(ents[index]->type))
		{
			gameentity &e = *(gameentity *)ents[index];
			bool commit = false;
			loopv(ents)
			{
				gameentity &f = *(gameentity *)ents[i];
				if(f.links.find(index) >= 0)
				{
					bool both = e.links.find(i) >= 0;
					switch(f.type)
					{
						case MAPMODEL:
						{
							if(e.type == TRIGGER)
							{
								f.spawned = e.spawned;
								f.lastemit = e.lastemit;
								commit = false;
							}
							break;
						}
						case PARTICLES:
						{
							f.lastemit = e.lastemit;
							commit = d && local;
							break;
						}
						case MAPSOUND:
						{
							if(mapsounds.inrange(f.attr1) && !issound(f.schan))
							{
								f.lastemit = e.lastemit;
								int flags = SND_MAP;
								if(f.attr5&SND_NOATTEN) flags |= SND_NOATTEN;
								if(f.attr5&SND_NODELAY) flags |= SND_NODELAY;
								if(f.attr5&SND_NOCULL) flags |= SND_NOCULL;
								playsound(f.attr1, both ? f.o : e.o, NULL, flags, f.attr4, f.attr2, f.attr3, &f.schan);
								commit = d && local;
							}
							break;
						}
						default: break;
					}
				}
			}
			if(d && commit) cl.cc.addmsg(SV_EXECLINK, "ri2", d->clientnum, index);
		}
	}

	void findplayerspawn(dynent *d, int forceent = -1, int tag = -1, int retries = 0)   // place at random spawn. also used by monsters!
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
		else if(retries < 2) findplayerspawn(d, -1, retries < 1 ? tag : -1, retries+1);
		else
		{
			d->o.x = d->o.y = d->o.z = 0.5f*getworldsize();
			cl.ph.entinmap(d, false);
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

	bool route(gameent *d, int node, int goal, vector<int> &route, avoidset &obstacles, float tolerance, bool retry = false, float *score = NULL)
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

		if(!retry)
		{
			loopavoid(obstacles, d,
			{
				if(ents.inrange(ent) && ents[ent]->type == ents[node]->type)
				{
					nodes[ent].id = routeid;
					nodes[ent].curscore = -1.f;
					nodes[ent].estscore = 0.f;
				}
			});
		}

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
			int current = int(m-&nodes[0]);
			extentity &ent = *ents[current];
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

	bool entitydrop()
	{
		return (m_fight(cl.gamemode) && autodropentities) || dropentities();
	}

	int entitynode(const vec &v, bool dist = true, int type = WAYPOINT)
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

	void entitylink(int index, int node, bool both = true)
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

	void entitycheck(gameent *d)
	{
		if(d->state == CS_ALIVE)
		{
			vec v(cl.feetpos(d, 0.f));
			int curnode = entitynode(v);
			if(entitydrop() && ((m_fight(cl.gamemode) && d->aitype == AI_NONE) || d == cl.player1))
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
					entitylink(d->lastnode, curnode, !d->timeinair);

				d->lastnode = curnode;
			}
			else
			{
				if(!ents.inrange(curnode)) curnode = entitynode(v, false);
				if(ents.inrange(curnode)) d->lastnode = curnode;
				else d->lastnode = -1;
			}
		}
		else d->lastnode = -1;
	}

	void readent(gzFile &g, int mtype, int mver, char *gid, int gver, int id, entity &e)
	{
		gameentity &f = (gameentity &)e;

		f.mark = false;

		if(mtype == MAP_OCTA)
		{
			// translate into our format
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
				// 13	I_CARTRIDGES	8	WEAPON		GUN_PLASMA
				case 8: case 9: case 10: case 11: case 12: case 13:
				{
					int gun = f.type-8, gunmap[6] = {
						GUN_SG, GUN_CG, GUN_FLAMER, GUN_RIFLE, GUN_GL, GUN_CARBINE
					};

					if(gun >= 0 && gun <= 5)
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
				// 21	MONSTER			10	NOTUSED
				case 21:
				{
					f.type = NOTUSED;
					break;
				}
				// 22	CARROT			11	TRIGGER		0
				case 22:
				{
					f.type = TRIGGER;
					f.attr1 = f.attr2 = f.attr3 = f.attr4 = f.attr5 = 0;
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
						e.attr2 = e.attr4 = 0; // not set in octa
						e.attr3 = 100; // give a push
						e.attr5 = 0x11A; // give it a pretty blueish portal like sauer's
						e.o.z += cl.player1->height/2; // teleport in BF is at middle
						e.mark = false;
						break;
					}
					case WAYPOINT:
					{
						e.attr1 = 0;
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
		int entities = 0;
		loopvj(ents)
		{
			gameentity &e = *(gameentity *)ents[j];
			loopvrev(e.links) if(!canlink(j, e.links[i], true)) e.links.remove(i);

			switch(e.type)
			{
				case WEAPON:
				{
					if(mtype == MAP_BFGZ && gver <= 90)
					{ // move grenades to the end of the weapon array
						if(e.attr1 >= 4) e.attr1--;
						else if(e.attr1 == 3) e.attr1 = GUN_GL;
					}
					if(mtype == MAP_BFGZ && gver <= 97 && e.attr1 >= 4)
						e.attr1++; // add in carbine
					if(e.attr1 == GUN_PLASMA) e.attr1 = GUN_CARBINE; // plasma is permanent
					break;
				}
				case PUSHER:
				{
					if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && gver <= 95))
						e.attr1 = int(e.attr1*1.25f);
					break;
				}
				case WAYPOINT:
				{
					entities++;
					if(mtype == MAP_BFGZ && gver <= 90)
						e.attr1 = e.attr2 = e.attr3 = e.attr4 = e.attr5 = 0;
					break;
				}
				default: break;
			}
		}
		autodropentities = m_fight(cl.gamemode) && !entities;
	}

	void mapstart()
	{
		if(autodropentities)
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
					case FLAG:
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

	void renderlinked(gameentity &e, int idx)
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
					if(ents.inrange(g) && g > idx) // smaller dual links are done already?
					{
						gameentity &h = *(gameentity *)ents[g];
						if(&h == &e)
						{
							both = true;
							break;
						}
					}
				}
#if 0
				part_flare(vec(e.o).add(vec(0, 0, RENDERPUSHZ)),
					vec(f.o).add(vec(0, 0, RENDERPUSHZ)),
						10, PART_LINE, both ? 0xFF8822 : 0xFF2222, 0.2f);
#else
				vec fr(vec(e.o).add(vec(0, 0, RENDERPUSHZ))), dr(vec(f.o).add(vec(0, 0, RENDERPUSHZ)));
				vec col(0.75f, both ? 0.5f : 0.0f, 0.f);
				renderline(fr, dr, col.x, col.y, col.z, false);
				dr.sub(fr);
				dr.normalize();
				float yaw, pitch;
				vectoyawpitch(dr, yaw, pitch);
				dr.mul(RENDERPUSHX);
				dr.add(fr);
				rendertris(dr, yaw, pitch, 2.f, col.x*2.f, col.y*2.f, col.z*2.f, true, false);
#endif
			}
		}
	}

	void renderentshow(gameentity &e, int idx, int level)
	{
		if(level != 1 && e.o.dist(camera1->o) > maxparticledistance) return;
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
					float radius = (float)enttype[e.type].radius;
					if((e.type == TRIGGER || e.type == TELEPORT || e.type == PUSHER) && e.attr4)
						radius = (float)e.attr4;
					if(radius > 0.f) renderradius(e.o, radius, radius, radius, false);
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
			if(!level || showentlinks() >= level || (e.type == WAYPOINT && dropentities()))
				renderlinked(e, idx);
	}

	void renderentlight(gameentity &e)
	{
		vec colour(e.attr2, e.attr3, e.attr4);
		colour.div(255.f);
		adddynlight(vec(e.o), float(e.attr1 ? e.attr1 : hdr.worldsize), colour);
	}

	void adddynlights()
	{
		if(cl.player1->state == CS_EDITING && showlighting())
		{
            loopv(entgroup)
            {
                int n = entgroup[i];
                if(ents.inrange(n) && ents[n]->type == LIGHT && ents[n]->attr1 > 0)
                    renderfocus(n, renderentlight(e));
            }
            if(ents.inrange(enthover) && ents[enthover]->type == LIGHT && ents[enthover]->attr1 > 0)
                renderfocus(enthover, renderentlight(e));
		}
	}

    void preload()
    {
		loopv(ents)
		{
			extentity &e = *ents[i];
			const char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
			if(mdlname && *mdlname) loadmodel(mdlname, -1, true);
			if(e.type == WEAPON && isgun(e.attr1)) cl.ws.preload(e.attr1);
		}
    }

	ITVAR(portaltex, "textures/portal", 0);

 	void update()
	{
		entitycheck(cl.player1);
		loopv(cl.players) if(cl.players[i]) entitycheck(cl.players[i]);
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(e.type == MAPSOUND && !e.links.length() && lastmillis-e.lastemit >= TRIGGERTIME && mapsounds.inrange(e.attr1))
			{
				if(!issound(e.schan))
				{
					int flags = SND_MAP|SND_LOOP; // ambient sounds loop
					if(e.attr5&SND_NOATTEN) flags |= SND_NOATTEN;
					if(e.attr5&SND_NODELAY) flags |= SND_NODELAY;
					if(e.attr5&SND_NOCULL) flags |= SND_NOCULL;
					playsound(e.attr1, e.o, NULL, flags, e.attr4, e.attr2, e.attr3, &e.schan);
					e.lastemit = lastmillis; // prevent clipping when moving around
				}
			}
		}
	}

    void renderteleport(gameentity &e, Texture *t)
    {
		glPushMatrix();
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);

		vec dir;
		vecfromyawpitch((float)e.attr1, (float)e.attr2, 1, 0, dir);
		vec o(vec(e.o).add(dir));
		glTranslatef(o.x, o.y, o.z);
		glRotatef((float)e.attr1-180.f, 0, 0, 1);
		glRotatef((float)e.attr2, 1, 0, 0);
		float radius = (float)(e.attr4 ? e.attr4 : enttype[e.type].radius);
		glScalef(radius, radius, radius);

		glBindTexture(GL_TEXTURE_2D, t->id);

		int attr = int(e.attr5), colour = (((attr&0xF)<<4)|((attr&0xF0)<<8)|((attr&0xF00)<<12))+0x0F0F0F;
		float r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f;
		glColor3f(r, g, b);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.f, 0.f, 1.f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(1.f, 0.f, 1.f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(1.f, 0.f, -1.f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.f, 0.f, -1.f);
		xtraverts += 4;
		glEnd();
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glPopMatrix();
	}

	void render()
	{
		if(rendernormally) // important, don't render lines and stuff otherwise!
		{
			int level = (m_edit(cl.gamemode) ? 2 : ((showentdir()==3 || showentradius()==3 || showentlinks()==3 || (dropentities() && !m_fight(cl.gamemode))) ? 3 : 0));
			if(level)
            {
                renderprimitive(true);
                loopv(ents)
			    {
				    renderfocus(i, renderentshow(e, i, cl.player1->state == CS_EDITING && (entgroup.find(i) >= 0 || enthover == i) ? 1 : level));
			    }
                renderprimitive(false);
            }
		}

		Texture *t = textureload(portaltex(), 0, true);
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(e.type == TELEPORT && e.attr5 && e.o.dist(camera1->o) < maxparticledistance)
				renderteleport(e, t);
		}

		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
			bool active = enttype[e.type].usetype == EU_ITEM && e.spawned;
			if(m_edit(cl.gamemode) || active)
			{
				const char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
				if(mdlname && *mdlname)
				{
					int flags = MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED;
					if(!active) flags |= MDL_TRANSLUCENT;

					rendermodel(&e.light, mdlname, ANIM_MAPMODEL|ANIM_LOOP, e.o, 0.f, 0.f, 0.f, flags);
				}
			}
		}
	}

	void drawparticle(gameentity &e, vec &o, int idx, bool spawned)
	{
		if(e.type == NOTUSED || o.dist(camera1->o) > maxparticledistance) return;
		if(e.type == PARTICLES)
		{
			if(idx < 0 || !e.links.length()) makeparticles((entity &)e);
			else if(lastmillis-e.lastemit < TRIGGERTIME)
				makeparticle(o, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
		}

		if(m_edit(cl.gamemode) && cansee(e))
		{
			bool hasent = idx >= 0 && (entgroup.find(idx) >= 0 || enthover == idx);
			vec off(0, 0, 2.f), pos(o);
			part_create(PART_EDIT, 1, pos, hasent ? 0xFF6600 : 0xFFFF00, hasent ? 2.0f : 1.5f);
			if(showentinfo() >= 2 || cl.player1->state == CS_EDITING)
			{
				s_sprintfd(s)("@%s%s (%d)", hasent ? "\fo" : "\fy", enttype[e.type].name, idx >= 0 ? idx : 0);
				part_text(pos.add(off), s);

				if(showentinfo() >= 3 || hasent)
				{
					s_sprintf(s)("@%s%d %d %d %d %d", hasent ? "\fw" : "\fy", e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
					part_text(pos.add(off), s);
				}
				if(showentinfo() >= 4 || hasent)
				{
					s_sprintf(s)("@%s%s", hasent ? "\fw" : "\fy", entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, showentinfo() >= 5 || hasent));
					part_text(pos.add(off), s);
				}
			}
		}
		else if(showentnames())
		{
			if(e.type == WEAPON && spawned)
			{
				s_sprintfd(s)("@%s", entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, false));
				part_text(vec(o).add(vec(0, 0, 4)), s);
			}
		}
	}

	void drawparticles()
	{
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
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
