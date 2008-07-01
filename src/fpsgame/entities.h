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
					entlinks[i].add(PLAYERSTART);
					entlinks[i].add(WEAPON);
					entlinks[i].add(PUSHER);
					entlinks[i].add(FLAG);
					entlinks[i].add(TELEPORT);
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

   const char *itemname(int i)
	{
		if(ents[i]->type == WEAPON)
		{
			int gun = ents[i]->attr1;
			if(gun <= -1 || gun >= NUMGUNS) gun = 0;
			return guntype[gun].name;
		}
		return NULL;
	}

	const char *entmdlname(int type, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0)
	{
		static string emdl;
		emdl[0] = 0;

		switch (type)
		{
			case WEAPON:
				s_sprintf(emdl)("weapons/%s/item", guntype[attr1].name);
				break;
			case FLAG:
				if(m_edit(cl.gamemode)) s_sprintf(emdl)("%s", teamtype[attr2].flag);
				break;
			default:
				break;
		}
		return emdl[0] ? emdl : NULL;
	}

	void announce(int idx, const char *msg = "")
	{
		bool announcer = false;
		loopv(ents)
		{
			fpsentity &e = (fpsentity &)*ents[i];
			if(e.type == ANNOUNCER)
			{
				playsound(idx, &e.o, e.attr3, e.attr1, e.attr2, SND_COPY);
				announcer = true;
			}
		}
		if(!announcer)
		{
			loopi(13)
			{
				vec v;
				switch(i)
				{
					case 1:
						v = vec(0, 0, 0.5f*getworldsize());
						break;
					case 2:
						v = vec(0, getworldsize(), 0.5f*getworldsize());
						break;
					case 3:
						v = vec(getworldsize(), getworldsize(), 0.5f*getworldsize());
						break;
					case 4:
						v = vec(getworldsize(), 0, 0.5f*getworldsize());
						break;

					case 5:
						v = vec(0, 0, getworldsize());
						break;
					case 6:
						v = vec(0, getworldsize(), getworldsize());
						break;
					case 7:
						v = vec(getworldsize(), getworldsize(), getworldsize());
						break;
					case 8:
						v = vec(getworldsize(), 0, getworldsize());
						break;

					case 9:
						v = vec(0, 0, 0.5f*getworldsize());
						break;
					case 10:
						v = vec(0, getworldsize(), 0);
						break;
					case 11:
						v = vec(getworldsize(), getworldsize(), 0);
						break;
					case 12:
						v = vec(getworldsize(), 0, 0);
						break;

					case 0:
					default:
						v = vec(0.5f*getworldsize(), 0.5f*getworldsize(), 0.5f*getworldsize());
						break;
				}
				playsound(idx, &v, 255, getworldsize()/2, 0, SND_COPY);
			}
		}
		if(*msg) conoutf("\fr%s", msg);
	}

	// these two functions are called when the server acknowledges that you really
	// picked up the item (in multiplayer someone may grab it before you).

	void useeffects(fpsent *d, int m, int n)
	{
        if(d && ents.inrange(n))
		{
			particle_text(d->abovehead(), itemname(n), 15);
			playsound(S_ITEMPICKUP, &d->o);
			d->useitem(m, ents[n]->type, ents[n]->attr1, ents[n]->attr2);
			ents[n]->spawned = false;
		}
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
							if((e.type == TRIGGER || e.type == TELEPORT || e.type == PUSHER) && mapsounds.inrange(f.attr1) && !issound(f.schan))
							{
								playsound(f.attr1, both ? &f.o : &e.o, f.attr4, f.attr2, f.attr3, SND_MAP);
								f.lastemit = lastmillis;
								if(both) e.lastemit = lastmillis;
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


	void teleport(int n, fpsent *d)	 // also used by monsters
	{
		fpsentity &e = (fpsentity &)*ents[n];
		for (int off = 0; off < e.links.length(); off++)
		{
			loopi(off+1)
			{
				int link = rnd(e.links.length()-off)+i;

				if(e.links.inrange(link))
				{
					int targ = e.links[link];

					if(ents.inrange(targ) && ents[targ]->type == TELEPORT)
					{
						d->o = ents[targ]->o;
						d->yaw = clamp((int)ents[targ]->attr1, 0, 359);
						d->pitch = clamp((int)ents[targ]->attr2, -89, 89);
						float mag = max((float)ents[targ]->attr3+d->vel.magnitude(), 64.f);
						d->vel = vec(0, 0, 0);
						vecfromyawpitch(d->yaw, d->pitch, 1, 0, d->vel);
						d->o.add(d->vel);
						d->vel.mul(mag);
						cl.ph.entinmap(d, false);
						execlink(d, n, true);
						execlink(d, targ, true);
						if(d == cl.player1) cl.lastmouse = cl.lastcamera = 0;
						return;
					}
				}
			}
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
		if(d == cl.player1) cl.lastmouse = cl.lastcamera = 0;
	}

	void reaction(int n, fpsent *d)
	{
		switch(ents[n]->type)
		{
			case TELEPORT:
			{
				if(d->lastuse == ents[n]->type && lastmillis-d->lastusemillis<1000) break;
				d->lastuse = ents[n]->type;
				d->lastusemillis = lastmillis;
				teleport(n, d);
				break;
			}

			case PUSHER:
			{
				if(d->lastuse==ents[n]->type && lastmillis-d->lastusemillis<1000) break;
				d->lastuse = ents[n]->type;
				d->lastusemillis = lastmillis;
				vec v((int)(char)ents[n]->attr3*10.0f, (int)(char)ents[n]->attr2*10.0f, ents[n]->attr1*12.5f);
				d->timeinair = 0;
                d->falling = vec(0, 0, 0);
				d->vel = v;
				execlink(d, n, true);
				break;
			}
		}
	}

	void checkitems(fpsent *d)
	{
		float eye = d->height*0.5f;
		vec m = d->o;
		m.z -= eye;

		loopv(ents)
		{
			extentity &e = *ents[i];
			if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
			if(enttype[e.type].usetype == ETU_AUTO && insidesphere(m, eye, d->radius, e.o, enttype[e.type].height, enttype[e.type].radius))
				reaction(i, d);
		}

		if(d->useaction)
		{ // difference here is the client requests it and gets the closest one
			int n = -1;
			loopv(ents)
			{
				extentity &e = *ents[i];
				if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
				if(e.spawned && enttype[e.type].usetype == ETU_ITEM
					&& insidesphere(m, eye, d->radius, e.o, enttype[e.type].height, enttype[e.type].radius)
					&& d->canuse(e.type, e.attr1, e.attr2, lastmillis)
					&& (!ents.inrange(n) || e.o.dist(m) < ents[n]->o.dist(m)))
						n = i;
			}
			if(ents.inrange(n)) cl.cc.addmsg(SV_ITEMUSE, "ri3", d->clientnum, lastmillis-cl.maptime, n);
			else playsound(S_DENIED);
			d->useaction = false;
		}
		if(m_ctf(cl.gamemode)) cl.ctf.checkflags(d);
	}

	void putitems(ucharbuf &p)
	{
		loopv(ents)
		{
			switch (ents[i]->type)
			{
				case WEAPON:
				{
					putint(p, i);
					putint(p, ents[i]->type);
					putint(p, ents[i]->attr1);
					putint(p, ents[i]->attr2);
					putint(p, ents[i]->attr3);
					putint(p, ents[i]->attr4);
					putint(p, ents[i]->attr5);
					ents[i]->spawned = m_insta(cl.gamemode, cl.mutators) ? false : true;
					break;
				}
				default: break;
			}
		}
	}

	void resetspawns() { loopv(ents) ents[i]->spawned = false; }
	void setspawn(int i, bool on) { if(ents.inrange(i)) ents[i]->spawned = on; }

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
				while (e.attr1 < 0) e.attr1 += NUMGUNS;
				while (e.attr1 >= NUMGUNS) e.attr1 -= NUMGUNS;
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
			if(maylink(ents[index]->type) &&
				maylink(ents[node]->type) &&
					entlinks[ents[index]->type].find(ents[node]->type) >= 0)
						return true;
			if(msg)
				conoutf("entity %s (%d) and %s (%d) are not linkable", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);

			return false;
		}
		if(msg) conoutf("entity %d and %d are unable to be linked as one does not seem to exist", index, node);
		return false;
	}

	bool linkents(int index, int node, bool add, bool local, bool toggle)
	{
		if(ents.inrange(index) && ents.inrange(node) && canlink(index, node, local))
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
						conoutf("entity %s (%d) and %s (%d) delinked", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
					return true;
				}
				else if(toggle && canlink(node, index))
				{
					f.links.add(index);
					if(local && m_edit(cl.gamemode))
						cl.cc.addmsg(SV_EDITLINK, "ri3", 1, node, index);

					if(verbose >= 3)
						conoutf("entity %s (%d) and %s (%d) linked", enttype[ents[node]->type].name, node, enttype[ents[index]->type].name, index);
					return true;
				}
			}
			else if(toggle && canlink(node, index) && (g = f.links.find(index)) >= 0)
			{
				f.links.remove(g);
				if(local && m_edit(cl.gamemode))
					cl.cc.addmsg(SV_EDITLINK, "ri3", 0, node, index);

				if(verbose >= 3)
					conoutf("entity %s (%d) and %s (%d) delinked", enttype[ents[node]->type].name, node, enttype[ents[index]->type].name, index);
				return true;
			}
			else if(toggle || add)
			{
				e.links.add(node);
				if(local && m_edit(cl.gamemode))
					cl.cc.addmsg(SV_EDITLINK, "ri3", 1, index, node);

				if(verbose >= 3)
					conoutf("entity %s (%d) and %s (%d) linked", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
				return true;
			}
		}
		if(verbose >= 3)
			conoutf("entity %s (%d) and %s (%d) failed linking", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
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
		float weight, goal;
		bool dead;
		vector<int> nodes;

		linkq() : weight(0.f), goal(0.f), dead(false) {}
		~linkq() {}
	};

	float route(int node, int goal, vector<int> &route, vector<int> &avoid, int flags)
	{
		float result = 1e16f;

		route.setsize(0);

		if(ents.inrange(node) && ents.inrange(goal))
		{
			struct fpsentity &f = (fpsentity &) *ents[node], &g = (fpsentity &) *ents[goal];
			vector<linkq> queue;
			int q = 0;
			queue.setsize(0);
			queue.add(linkq());
			queue[q].nodes.add(node);
			queue[q].goal = f.o.dist(g.o);

			while (queue.inrange(q) && queue[q].nodes.last() != goal)
			{
				struct fpsentity &e = (fpsentity &) *ents[queue[q].nodes.last()];

				float w = queue[q].weight;
				vector<int> s = queue[q].nodes;
				int a = 0;

				loopvj(e.links)
				{
					int v = e.links[j];

					if(ents.inrange(v) && ents[v]->type == f.type)
					{
						bool skip = false;

						loopvk(queue)
						{
							if(queue[k].nodes.find(v) >= 0 ||
									((flags & ROUTE_AVOID) && v != goal && avoid.find(v) >= 0) ||
									((flags & ROUTE_GTONE) && v == goal && queue[k].nodes.length() == 1))
							{
								skip = true;
								break;
							}
						} // don't revisit shorter noded paths
						if(!skip)
						{
							struct fpsentity &h = (fpsentity &) *ents[v];

							int r = q; // continue this line for the first one

							if(a)
							{
								r = queue.length();
								queue.add(linkq());
								queue[r].nodes = s;
								queue[r].weight = w;
							}
							queue[r].nodes.add(v);
							queue[r].weight += e.o.dist(h.o);
							queue[r].goal = h.o.dist(g.o);
							a++;
						}
					}
				}
				if(!a)
				{
					queue[q].dead = true;
				} // this one ain't going anywhere..

				q = -1; // get shortest path
				loopvj(queue)
				{
					if(!queue[j].dead && (!queue.inrange(q) || queue[j].weight+queue[j].goal < queue[q].weight+queue[q].goal))
						q = j;
				}
			}

			if(!queue.inrange(q) && !(flags & ROUTE_ABSOLUTE)) // didn't get there, resort to failsafe proximity match
			{
				q = -1;

				loopvj(queue)
				{
					int u = -1;

					loopvrev(queue[j].nodes) // find the closest node in this branch
					{
						if(!queue[j].nodes.inrange(u) || ents[queue[j].nodes[i]]->o.dist(g.o) < ents[queue[j].nodes[u]]->o.dist(g.o))
							u = i;
					}

					if(queue[j].nodes.inrange(u))
					{
						loopvrev(queue[j].nodes) // trim the node list to the end at the shortest
						{
							if(i <= u)
								break;
							queue[j].nodes.remove(i);
						}

						if(!queue.inrange(q) || ents[queue[j].nodes[u]]->o.dist(g.o) < ents[queue[q].nodes.last()]->o.dist(g.o))
							q = j;
					}
				}
			}

			if(queue.inrange(q))
			{
				route = queue[q].nodes;
				result = queue[q].weight;
			}
			else if(flags & ROUTE_FAILSAFE) // random search
			{
				float dist = 0.f;
				for (int c = node; ents.inrange(c); )
				{
					fpsentity &e = (fpsentity &) *ents[c];
					int b = -1;

					loopv(e.links)
					{
						int n = e.links[i];

						if(ents[n]->type == f.type &&
							route.find(n) < 0 && (!(flags & ROUTE_AVOID) || avoid.find(n) < 0) &&
								(!ents.inrange(b) || (ents.inrange(n) && ents[n]->o.dist(g.o) < ents[b]->o.dist(g.o))))
									b = n;
					}
					if(ents.inrange(b))
					{
						route.add(b);
						dist += ents[b]->o.dist(e.o);
					}
					c = b;
				}
				if(dist > 0.f) result = dist;
			}
		}
		return result;
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
			vec v(vec(d->o).sub(vec(0, 0, d->height)));
			int curnode = waypointnode(v, true);

			if(m_edit(cl.gamemode) && dropwaypoints() && d == cl.player1)
			{
				if(!ents.inrange(curnode) && ents.inrange(d->oldnode) && ents[d->oldnode]->o.dist(v) <= enttype[WAYPOINT].radius*2.f)
					curnode = d->oldnode;

				if(!ents.inrange(curnode))
				{
					curnode = ents.length();
					vec o(d->o);
					if(d->timeinair || !cl.ph.droptofloor(o, enttype[WAYPOINT].radius, 0))
						o = v;
					newentity(o, WAYPOINT, 0, 0, 0, 0);
				}

				if(ents.inrange(d->oldnode) && d->oldnode != curnode)
				{
					waypointlink(d->oldnode, curnode, !d->timeinair);

					loopv(ents)
						if(ents[i]->type != WAYPOINT && canlink(curnode, i) &&
							ents[i]->o.dist(v) <= enttype[WAYPOINT].radius)
								waypointlink(curnode, i, false);
				}
			}
			else if(!ents.inrange(curnode))
				curnode = waypointnode(v, false);

			if(ents.inrange(curnode))
			{
				if(d->lastnode != curnode) d->oldnode = d->lastnode;
				d->lastnode = curnode;
			}
			else if(ents.inrange(d->oldnode)) d->lastnode = d->oldnode;
			else d->lastnode = d->oldnode = -1;
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
					conoutf("WARNING: ignoring entity %d type %d", id, f.type);
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
						conoutf("WARNING: replaced teledest %d with closest teleport %d", i, dest);
						e.type = NOTUSED; // get rid of this guy then
					}
					else
					{
						conoutf("WARNING: modified teledest %d to a teleport", i);
						dest = i;
					}

					teleyaw[dest] = e.attr1; // store the yaw for later

					loopvj(ents) // find linked teleport(s)
					{
						fpsentity &f = (fpsentity &)*ents[j];

						if(f.type == TELEPORT && !f.mark && f.attr1 == e.attr2)
						{
							if(verbose >= 2)
								conoutf("WARNING: teleports %d and %d linked automatically", dest, j);
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
							conoutf("WARNING: old base %d (%d, %d) replaced with flag %d (%d, %d)", i, e.attr1, e.attr2, dest, f.attr1, f.attr2);
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

				vec fr = d.o, to = f.o;

				fr.z += RENDERPUSHZ;
				to.z += RENDERPUSHZ;

				vec col(0.5f, both ? 0.25f : 0.0f, 0.f);
				renderline(fr, to, col.x, col.y, col.z, false);

				vec dr = to;
				float yaw, pitch;

				dr.sub(fr);
				dr.normalize();

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
					renderradius(e.o, e.attr2, e.attr2, false);
					renderradius(e.o, e.attr3, e.attr3, false);
					break;
				case LIGHT:
					renderradius(e.o, e.attr1 ? e.attr1 : hdr.worldsize, e.attr1 ? e.attr1 : hdr.worldsize, false);
					break;
				case ANNOUNCER:
					renderradius(e.o, e.attr1, e.attr1, false);
					renderradius(e.o, e.attr2, e.attr2, false);
					break;
				default:
					if(enttype[e.type].height || enttype[e.type].radius)
						renderradius(e.o, enttype[e.type].height, enttype[e.type].radius, false);
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
            if(!mdlname) continue;
            loadmodel(mdlname, -1, true);
        }
    }

 	void update()
	{
		waypointcheck(cl.player1);
		loopv(cl.players) if(cl.players[i]) waypointcheck(cl.players[i]);
		loopv(ents)
		{
			fpsentity &e = (fpsentity &)*ents[i];
			if(e.type == MAPSOUND && !e.links.length() && lastmillis-e.lastemit > 500 && mapsounds.inrange(e.attr1) && !issound(e.schan))
			{
				e.schan = playsound(e.attr1, &e.o, e.attr4, e.attr2, e.attr3, SND_MAP|SND_LOOP);
				e.lastemit = lastmillis; // prevent clipping when moving around
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
			bool active = (enttype[e.type].usetype == ETU_ITEM && e.spawned);
			if(m_edit(cl.gamemode) || active)
			{
				const char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);

				if(mdlname)
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
				if(e.type == PLAYERSTART || e.type == FLAG)
					particle_text(vec(e.o).add(vec(0, 0, 4)),
						e.attr2 >= TEAM_NEUTRAL && e.attr2 < TEAM_MAX ? teamtype[e.attr2].name : "unknown",
							entgroup.find(i) >= 0 || enthover == i ? 13 : 11, 1);

				particle_text(vec(e.o).add(vec(0, 0, 2)),
					findname(e.type), entgroup.find(i) >= 0 || enthover == i ? 13 : 11, 1);
				if(e.type != PARTICLES) regular_particle_splash(2, 2, 40, e.o);
			}
		}
	}
} et;
