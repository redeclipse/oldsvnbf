struct entities : icliententities
{
	GAMECLIENT &cl;

	vector<extentity *> ents;
	vector<int> entlinks[MAXENTTYPES];

	IVARP(showentdir, 0, 1, 3);
	IVARP(showentradius, 0, 1, 3);
	IVARP(showentlinks, 0, 1, 3);
	IVARP(showlighting, 0, 1, 1);

	IVAR(dropwaypoints, 0, 0, 1); // drop waypoints during play

	entities(GAMECLIENT &_cl) : cl(_cl)
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
				//	MAPMODEL = ET_MAPMODEL,			// 2  angle, idx
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
				//	MONSTER,						// 10 [angle], [type]
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
				//	WAYPOINT,						// 16 radius, weight
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

	const char *itemname(int type, int attr1 = 0, int attr2 = 0)
	{
		if(type == WEAPON)
		{
			int gun = attr1;
			if(!isgun(gun)) gun = GUN_PISTOL;
			return guntype[gun].name;
		}
		return NULL;
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
		if(sounds.inrange(idx))
		{
			static int lastannouncement;
			if(force || lastmillis-lastannouncement > 1000)
			{
				bool announcer = false;
				loopv(ents)
				{
					fpsentity &e = (fpsentity &)*ents[i];
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
		}
		if(*msg) console("\fg%s", (force ? CON_CENTER : 0)|CON_NORMAL, msg);
	}

	// these two functions are called when the server acknowledges that you really
	// picked up the item (in multiplayer someone may grab it before you).

	void useeffects(fpsent *d, int n, int g, int r)
	{
		if(d && ents.inrange(n))
		{
			fpsentity &e = (fpsentity &)*ents[n];
			vec pos = e.o;
			loopv(cl.pj.projs)
			{
				projent &proj = *cl.pj.projs[i];
				if(proj.projtype != PRJ_ENT || proj.id != n) continue;
				pos = proj.o;
				proj.beenused = true;
				proj.state = CS_DEAD;
			}
			const char *item = itemname(e.type, e.attr1, e.attr2);
			if(item && (d != cl.player1 || cl.isthirdperson()))
				particle_text(d->abovehead(), item, 15);
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

	bool collateitems(fpsent *d, bool use, bool can, vector<actitem> &actitems)
	{
		float eye = d->height*0.5f;
		vec m = d->o;
		m.z -= eye;

		loopv(ents)
		{
			extentity &e = *ents[i];
			if(e.type <= NOTUSED || e.type >= MAXENTTYPES || enttype[e.type].usetype == EU_NONE)
				continue;
			if(!insidesphere(m, eye, d->radius, e.o, enttype[e.type].height, enttype[e.type].radius))
				continue;

			switch(enttype[e.type].usetype)
			{
				case EU_AUTO:
				{
					if(e.type != TRIGGER)
					{
						if(use) reaction(i, d);
					}
					else if(e.spawned || !e.attr4)
					{
						switch(e.attr3)
						{
							case TA_AUTO:
								if(use) reaction(i, d);
								break;
							case TA_ACT:
							{
								actitem &t = actitems.add();
								t.type = ITEM_ENT;
								t.target = i;
								t.score = m.dist(e.o);
								break;
							}
							default: break;
						}
					}
					break;
				}
				case EU_ITEM:
				{
					if(!e.spawned) break;
					if(can && !d->canuse(e.type, e.attr1, e.attr2, lastmillis))
						break;
					actitem &t = actitems.add();
					t.type = ITEM_ENT;
					t.target = i;
					t.score = m.dist(e.o);
					break;
				}
				default: break;
			}
		}
		loopv(cl.pj.projs)
		{
			projent &proj = *cl.pj.projs[i];
			if(proj.projtype != PRJ_ENT || !proj.ready()) continue;
			if(enttype[proj.ent].usetype != EU_ITEM || !ents.inrange(proj.id))
				continue;
			if(can && !d->canuse(proj.ent, proj.attr1, proj.attr2, lastmillis)) continue;
			if(!insidesphere(m, eye, d->radius, proj.o, enttype[proj.ent].height, enttype[proj.ent].radius))
				continue;
			actitem &t = actitems.add();
			t.type = ITEM_PROJ;
			t.target = i;
			t.score = m.dist(proj.o);
		}
		return !actitems.empty();
	}

	void checkitems(fpsent *d)
	{
		vector<actitem> actitems;
		if(collateitems(d, true, true, actitems))
		{
			if(d->useaction)
			{
				while(!actitems.empty())
				{
					int closest = actitems.length()-1;
					loopv(actitems)
						if(actitems[i].score < actitems[closest].score)
							closest = i;

					actitem &t = actitems[closest];
					switch(t.type)
					{
						case ITEM_ENT:
						{
							if(!ents.inrange(t.target)) break;
							extentity &e = *ents[t.target];
							if(enttype[e.type].usetype == EU_ITEM)
							{
								cl.cc.addmsg(SV_ITEMUSE, "ri3", d->clientnum, lastmillis-cl.maptime, t.target);
								d->useaction = false;
							}
							else if(enttype[e.type].usetype == EU_AUTO)
							{
								reaction(t.target, d);
								d->useaction = false;
							}
							break;
						}
						case ITEM_PROJ:
						{
							if(!cl.pj.projs.inrange(t.target)) break;
							projent &proj = *cl.pj.projs[t.target];
							cl.cc.addmsg(SV_ITEMUSE, "ri3", d->clientnum, lastmillis-cl.maptime, proj.id);
							d->useaction = false;
							break;
						}
						default: break;
					}
					if(!d->useaction) break;
					else actitems.remove(closest);
				}
				if(d->useaction)
				{
					d->useaction = false;
					playsound(S_DENIED, 0, 255, d->o, d);
				}
			}
		}
		if(m_ctf(cl.gamemode)) cl.ctf.checkflags(d);
	}

	void putitems(ucharbuf &p)
	{
		loopv(ents)
		{
			fpsentity &e = (fpsentity &)*ents[i];
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
		if(ents.inrange(n)) ents[n]->spawned = on;
	}

	extentity *newent() { return new fpsentity(); }

	void fixentity(extentity &e)
	{
		fpsentity &f = (fpsentity &)e;

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
				while (e.attr1 < 0) e.attr1 += GUN_MAX;
				while (e.attr1 >= GUN_MAX) e.attr1 -= GUN_MAX;
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
	void execlink(fpsent *d, int index, bool local)
	{
		if(d && ents.inrange(index) && maylink(ents[index]->type))
		{
			if(local)
			{
				cl.cc.addmsg(SV_EXECLINK, "ri2", d->clientnum, index);
				if(d->bot) return;
			}

			fpsentity &e = (fpsentity &)*ents[index];

			loopv(ents)
			{
				fpsentity &f = (fpsentity &)*ents[i];
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


	void teleport(int n, fpsent *d)
	{
		fpsentity &e = (fpsentity &)*ents[n];
		vector<int> teleports;
		loopv(e.links)
			if(ents.inrange(e.links[i]) && ents[e.links[i]]->type == TELEPORT)
				teleports.add(i);

		if(!teleports.empty())
		{
			int r = rnd(teleports.length()), t = teleports[r];
			fpsentity &f = (fpsentity &)*ents[t];
			d->o = f.o;
			d->yaw = clamp((int)f.attr1, 0, 359);
			d->pitch = clamp((int)f.attr2, -89, 89);
			float mag = max((float)f.attr3+d->vel.magnitude(), 64.f);
			d->vel = vec(0, 0, 0);
			vecfromyawpitch(d->yaw, d->pitch, 1, 0, d->vel);
			d->o.add(d->vel);
			d->vel.mul(mag);
			cl.ph.entinmap(d, false);

			execlink(d, n, true);
			execlink(d, t, true);

			if(d == cl.player1) cl.resetstates(ST_VIEW);
			return;
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

	void reaction(int n, fpsent *d)
	{
		fpsentity &e = (fpsentity &)*ents[n];
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
			cl.cc.addmsg(SV_EDITENT, "ri9", i, (int)(e.o.x*DMF), (int)(e.o.y*DMF), (int)(e.o.z*DMF), e.type, e.attr1, e.attr2, e.attr3, e.attr4); // FIXME
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
			fpsentity &e = (fpsentity &)*ents[index];
			fpsentity &f = (fpsentity &)*ents[node];
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
			fpsentity &e = (fpsentity &)*ents[j];

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

	bool route(int node, int goal, vector<int> &route, vector<int> &avoid, float tolerance, float *score = NULL)
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

		loopv(avoid)
		{
			int ent = avoid[i];
			if(ents[ent]->type == ents[node]->type)
			{
				nodes[ent].id = routeid;
				nodes[ent].curscore = -1.f;
				nodes[ent].estscore = 0.f;
			}
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

	int waypointnode(vec &v, bool dist = true, int type = WAYPOINT)
	{
		int w = -1, t = entlinks[WAYPOINT].find(type) >= 0 ? type : WAYPOINT;
		loopv(ents) if(ents[i]->type == t)
		{
			if((!dist || ents[i]->o.dist(v) <= enttype[t].radius) &&
				(!ents.inrange(w) || ents[i]->o.dist(v) < ents[w]->o.dist(v)))
					w = i;
		}
		return w;
	}

	void waypointlink(int index, int node, bool both = true)
	{
		if(ents.inrange(index) && ents.inrange(node))
		{
			fpsentity &e = (fpsentity &)*ents[index], &f = (fpsentity &)*ents[node];

			if(e.links.find(node) < 0)
				linkents(index, node, true, true, false);
			if(both && f.links.find(index) < 0)
				linkents(node, index, true, true, false);
		}
	}

	void waypointcheck(fpsent *d)
	{
		if(d->state == CS_ALIVE)
		{
			vec v(cl.feetpos(d, 0.f));
			int curnode = waypointnode(v);

			if(m_edit(cl.gamemode) && dropwaypoints() && d == cl.player1)
			{
				if(!ents.inrange(curnode) && ents.inrange(d->lastnode) && ents[d->lastnode]->o.dist(v) <= enttype[WAYPOINT].radius*2.f)
					curnode = d->lastnode;

				if(!ents.inrange(curnode))
				{
					curnode = ents.length();
					newentity(v, WAYPOINT, 0, 0, 0, 0);
				}

				if(ents.inrange(d->lastnode) && d->lastnode != curnode)
					waypointlink(d->lastnode, curnode, !d->timeinair);

				d->lastnode = curnode;
			}
			else
			{
				if(!ents.inrange(curnode))
					curnode = waypointnode(v, false);

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
		fpsentity &f = (fpsentity &)e;

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
				// 21	MONSTER			10	MONSTER
				case 21:
				{
					f.type = MONSTER;
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
				fpsentity &e = (fpsentity &)*ents[i];

				if(e.type == TELEPORT && e.mark) // translate teledest to teleport and link them appropriately
				{
					int dest = -1;

					loopvj(ents) // see if this guy is sitting on top of a teleport already
					{
						fpsentity &f = (fpsentity &)*ents[j];

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
						fpsentity &f = (fpsentity &)*ents[j];

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
							fpsentity &f = (fpsentity &)*ents[j];

							if(f.type == FLAG && f.attr2 != TEAM_NEUTRAL &&
								(!ents.inrange(dest) || e.o.dist(f.o) < ents[dest]->o.dist(f.o)) &&
									e.o.dist(f.o) <= enttype[FLAG].radius*4.f)
										dest = j;
						}

						if(ents.inrange(dest))
						{
							fpsentity &f = (fpsentity &)*ents[dest];
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
				fpsentity &e = (fpsentity &)*ents[i];

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
						e.attr1 = enttype[WAYPOINT].radius;
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

		loopvj(ents)
		{
			fpsentity &e = (fpsentity &)*ents[j];
			loopvrev(e.links) if(!canlink(j, e.links[i], true)) e.links.remove(i);
		}
	}

	#define renderfocus(i,f) { extentity &e = *ents[i]; f; }

	void renderlinked(extentity &e)
	{
		fpsentity &d = (fpsentity &)e;

		loopv(d.links)
		{
			int index = d.links[i];

			if(ents.inrange(index))
			{
				fpsentity &f = (fpsentity &)*ents[index];
				bool both = false;

				loopvj(f.links)
				{
					int g = f.links[j];
					if(ents.inrange(g))
					{
						fpsentity &h = (fpsentity &)*ents[g];
						if(&h == &d)
						{
							both = true;
							break;
						}
					}
				}

				vec fr(vec(d.o).add(vec(0, 0, RENDERPUSHZ))),
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

	void renderentshow(extentity &e, int level)
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
			case MAPMODEL:
			{
				if(!level || showentdir() >= level) renderdir(e.o, e.attr1, 0, false);
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

	void renderentlight(extentity &e)
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
        loopv(ents)
        {
        	extentity &e = *ents[i];
			const char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
            if(mdlname && *mdlname) loadmodel(mdlname, -1, true);
        }
    }

 	void update()
	{
		waypointcheck(cl.player1);
		loopv(cl.players) if(cl.players[i]) waypointcheck(cl.players[i]);
		loopv(ents)
		{
			fpsentity &e = (fpsentity &)*ents[i];
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
			extentity &e = *ents[i];
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

	void drawparticles()
	{
		loopv(ents)
		{
			fpsentity &e = (fpsentity &)*ents[i];

			if(e.type == NOTUSED) continue;

			if(e.type == PARTICLES && e.o.dist(camera1->o) <= maxparticledistance)
			{
				if(!e.links.length()) makeparticles((entity &)e);
				else if(lastmillis-e.lastemit < 500)
				{
					bool both = false;

					loopvk(e.links) if(ents.inrange(e.links[k]))
					{
						fpsentity &f = (fpsentity &)*ents[e.links[k]];
						if(f.links.find(i) >= 0 && lastmillis-f.lastemit < 500)
						{
							makeparticle(f.o, e.attr1, e.attr2, e.attr3, e.attr4);
							both = true;
						}
					}

					if(!both) // hasn't got an active reciprocal link (fallback)
						makeparticle(e.o, e.attr1, e.attr2, e.attr3, e.attr4);
				}
			}

			if(m_edit(cl.gamemode))
			{
				bool hasent = (entgroup.find(i) >= 0 || enthover == i);
				if(e.type == PLAYERSTART || e.type == FLAG)
					particle_text(vec(e.o).add(vec(0, 0, 4)),
						e.attr2 >= TEAM_NEUTRAL && e.attr2 < TEAM_MAX ? teamtype[e.attr2].name : "unknown",
							hasent ? 13 : 11, 1);

				particle_text(vec(e.o).add(vec(0, 0, 2)), findname(e.type), hasent ? 13 : 11, 1);
				if(e.type != PARTICLES)
					part_create(3, 1, e.o, hasent ? 0xFF8822 : 0x888800, 1.f);
			}
		}
	}
} et;
