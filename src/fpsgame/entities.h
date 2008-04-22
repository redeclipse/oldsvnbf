struct entities : icliententities
{
	GAMECLIENT &cl;

	vector<extentity *> ents;

	IVARP(showentdir, 0, 1, 2);
	IVARP(showentradius, 0, 1, 2);

	IVARP(showbaselinks, 0, 0, 2);
	IVARP(showcheckpointlinks, 0, 0, 2);
	IVARP(showcameralinks, 0, 0, 2);
	IVARP(showteleportlinks, 0, 0, 2);
	IVARP(showwaypointlinks, 0, 0, 2);

	IVAR(dropwaypoints, 0, 0, 1); // drop waypoints during play

	entities(GAMECLIENT &_cl) : cl(_cl)
	{
		CCOMMAND(entlink, "", (entities *self), self->linkent());
	}

	vector<extentity *> &getents() { return ents; }

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
	}

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
			default:
				break;
		}
		return emdl[0] ? emdl : NULL;
	}

	void rumble(extentity &e) { playsound(S_RUMBLE, &e.o, 255, 0, true); }

	// these two functions are called when the server acknowledges that you really
	// picked up the item (in multiplayer someone may grab it before you).

	void useeffects(fpsent *d, int m, int n)
	{
        if(d && ents.inrange(n))
		{
			particle_text(d->abovehead(), itemname(n), 15);
			playsound(S_ITEMPICKUP, &d->o);
			if(d==cl.player1 || d->ownernum==cl.player1->clientnum)
				d->useitem(m, ents[n]->type, ents[n]->attr1, ents[n]->attr2);
		}
		ents[n]->spawned = false; // if the server sent this, then the ent is gone
	}

	// these functions are called when the client touches the item

	void teleport(int n, fpsent *d)	 // also used by monsters
	{
		fpsentity &e = (fpsentity &)*ents[n];

		while (e.links.length())
		{
			int link = rnd(e.links.length()), targ = e.links[link];

			if(ents.inrange(targ) && ents[targ]->type == TELEPORT)
			{
				d->o = ents[targ]->o;
				d->yaw = clamp((int)ents[targ]->attr1, 0, 359);
				d->pitch = clamp((int)ents[targ]->attr2, -89, 89);
				float mag = max(48.f, (float)ents[targ]->attr3+d->vel.magnitude());
				d->vel = vec(0, 0, 0);
				vecfromyawpitch(d->yaw, d->pitch, 1, 0, d->vel);
				d->o.add(d->vel);
				d->vel.mul(mag);
				cl.ph.entinmap(d, false);
				cl.playsoundc(S_TELEPORT, d);
				return;
			}
			else e.links.remove(link); // something wrong with it..
		}
		conoutf("unable to find a linking teleport for %d", n);
	}

	void findplayerspawn(dynent *d, int forceent = -1, int tag = -1)   // place at random spawn. also used by monsters!
	{
		int pick = forceent;
		if(pick<0)
		{
			int r = cl.ph.fixspawn-->0 ? 7 : rnd(10)+1;
			loopi(r) cl.ph.spawncycle = findentity(ET_PLAYERSTART, cl.ph.spawncycle+1, -1, tag);
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
				attempt = findentity(ET_PLAYERSTART, attempt+1, -1, tag);
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

	void gotocamera(int n, fpsent *d, int &delta)
	{
		int e = -1, tag = n, beenhere = -1;
		for (;;)
		{
			e = findentity(CAMERA, e + 1);
			if(e == beenhere || e < 0)
			{
				conoutf("no camera destination for tag %d", tag);
				return ;
			};
			if(beenhere < 0)
				beenhere = e;
			if(ents[e]->attr4 == tag)
			{
				d->o = ents[e]->o;
				d->yaw = ents[e]->attr1;
				d->pitch = ents[e]->attr2;
				delta = ents[e]->attr3;
				if(cl.player1->state == CS_EDITING)
				{
					vec dirv;
					vecfromyawpitch(d->yaw, d->pitch, 1, 0, dirv);
					vec tinyv = dirv.normalize().mul(10.0f);
					d->vel = tinyv;
				}
				else
				{
					d->vel = vec(0, 0, 0);
				}
				s_sprintfd(camnamalias)("camera_name_%d", ents[e]->attr4);
				break;
			}
		}
	}

	void reaction(int n, fpsent *d)
	{
		switch(ents[n]->type)
		{
			case TELEPORT:
			{
				if(d->lastuse == ents[n]->type && lastmillis-d->lastusemillis<500) break;
				d->lastuse = ents[n]->type;
				d->lastusemillis = lastmillis;
				teleport(n, d);
				break;
			}

			case JUMPPAD:
			{
				if(d->lastuse==ents[n]->type && lastmillis-d->lastusemillis<500) break;
				d->lastuse = ents[n]->type;
				d->lastusemillis = lastmillis;
				vec v((int)(char)ents[n]->attr3*10.0f, (int)(char)ents[n]->attr2*10.0f, ents[n]->attr1*12.5f);
				d->timeinair = 0;
                d->falling = vec(0, 0, 0);
				d->vel = v;
				cl.playsoundc(S_JUMPPAD, d);
				break;
			}
		}
	}

	void checkitems(fpsent *d)
	{
		if(d->state != CS_EDITING && d->state != CS_SPECTATOR)
		{
			float eye = d->height*0.5f;
			vec m = d->o;
			m.z -= eye;

			loopv(ents)
			{
				extentity &e = *ents[i];
				if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
				enttypes &t = enttype[e.type];
				if(t.usetype == ETU_AUTO && insidesphere(m, eye, d->radius, e.o, t.height, t.radius))
					reaction(i, d);
			}

			if(d->useaction)
			{ // difference here is the client requests it and gets the closest one
				int n = -1;
				loopv(ents)
				{
					extentity &e = *ents[i];
					if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
					enttypes &t = enttype[e.type];
					if(e.spawned && t.usetype == ETU_ITEM
						&& insidesphere(m, eye, d->radius, e.o, t.height, t.radius)
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
					ents[i]->spawned = true;
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
		switch(e.type)
		{
			case WEAPON:
				while (e.attr1 < 0) e.attr1 += NUMGUNS;
				while (e.attr1 >= NUMGUNS) e.attr1 -= NUMGUNS;
				if(e.attr2 < 0) e.attr2 = 0;
				break;
			default:
				break;
		}
	}

	const char *entnameinfo(entity &e) { return ""; }
	const char *entname(int i) { return i >= NOTUSED && i < MAXENTTYPES ? enttype[i].name : ""; }

	void editent(int i)
	{
		extentity &e = *ents[i];
		if(e.type == ET_EMPTY) linkclear(i);
		fixentity(e);
		if(multiplayer(false) && m_edit(cl.gamemode))
			cl.cc.addmsg(SV_EDITENT, "ri9", i, (int)(e.o.x*DMF), (int)(e.o.y*DMF), (int)(e.o.z*DMF), e.type, e.attr1, e.attr2, e.attr3, e.attr4); // FIXME
	}

	float dropheight(entity &e)
	{
		if(e.type==MAPMODEL || e.type==BASE) return 0.0f;
		return 4.0f;
	}

	bool linkents(int index, int node, bool add, bool local)
	{
		if (ents.inrange(index) && ents.inrange(node) && ents[index]->type == ents[node]->type && enttype[ents[index]->type].links)
		{
			fpsentity &e = (fpsentity &)*ents[index];
			fpsentity &f = (fpsentity &)*ents[node];
			int g;

			// works differently for the local client, using toggles instead of absolute values

			if((g = e.links.find(node)) >= 0 && (local || !add))
			{
				int h;
				if((h = f.links.find(index)) >= 0 && (local || !add))
				{
					e.links.remove(g);
					if (local && multiplayer(false) && m_edit(cl.gamemode))
						cl.cc.addmsg(SV_EDITLINK, "ri3", 0, index, node);
					return true;
				}
				else if(local)
				{
					f.links.add(index);
					if (local && multiplayer(false) && m_edit(cl.gamemode))
						cl.cc.addmsg(SV_EDITLINK, "ri3", 1, node, index);
					return true;
				}
			}
			else if((g = f.links.find(index)) >= 0 && local)
			{
				f.links.remove(g);
				if (local && multiplayer(false) && m_edit(cl.gamemode))
					cl.cc.addmsg(SV_EDITLINK, "ri3", 0, node, index);
				return true;
			}
			else if(local || add)
			{
				e.links.add(node);
				if (local && multiplayer(false) && m_edit(cl.gamemode))
					cl.cc.addmsg(SV_EDITLINK, "ri3", 1, index, node);

				return true;
			}
		}
		return false;
	}

	void linkent()
	{
		if(entgroup.length())
		{
			int index = entgroup[0];

			if(ents.inrange(index))
			{
				int type = ents[index]->type;

				if(enttype[type].links)
				{
					int last = -1;

					loopv(entgroup)
					{
						index = entgroup[i];

						if(ents[index]->type == type)
						{
							if(ents.inrange(last)) linkents(last, index, false, true);
							last = index;
						}
						else conoutf("entity %s is not linkable to a %s", enttype[type].name, enttype[index].name);
					}
				}
				else conoutf("entity %s is not linkable", enttype[type].name);
			}
		}
	}

	void linkclear(int n)
	{
		loopv(ents) if(enttype[ents[i]->type].links)
		{
			fpsentity &e = (fpsentity &)*ents[i];

			loopvj(e.links)
			{
				if(e.links[j] == n)
				{
					e.links.remove(j);
					break;
				}
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

	bool linkroute(int node, int goal, vector<int> &route, vector<int> &avoid, int flags = 0)
	{
		bool result = false;

		route.setsize(0);

		if (ents.inrange(node) && ents.inrange(goal) && ents[goal]->type == ents[node]->type && enttype[ents[node]->type].links)
		{
			struct fpsentity &f = (fpsentity &) *ents[node], &g = (fpsentity &) *ents[goal];
			vector<linkq *> queue;
			int q = 0;

			queue.add(new linkq());
			queue[q]->nodes.add(node);
			queue[q]->goal = f.o.dist(g.o);

			while (queue.inrange(q) && queue[q]->nodes.last() != goal)
			{
				struct fpsentity &e = (fpsentity &) *ents[queue[q]->nodes.last()];

				float w = queue[q]->weight;
				vector<int> s = queue[q]->nodes;
				int a = 0;

				loopvj(e.links)
				{
					int v = e.links[j];

					if (ents.inrange(v) && ents[v]->type == f.type)
					{
						bool skip = false;

						loopvk(queue)
						{
							if (queue[k]->nodes.find(v) >= 0 ||
									((flags & ROUTE_AVOID) && v != goal && avoid.find(v) >= 0) ||
									((flags & ROUTE_GTONE) && v == goal && queue.length() == 1))
							{
								skip = true;
								break;
							}
						} // don't revisit shorter noded paths
						if (!skip)
						{
							struct fpsentity &h = (fpsentity &) *ents[v];

							int r = q; // continue this line for the first one

							if (a)
							{
								r = queue.length();
								queue.add(new linkq());
								queue[r]->nodes = s;
								queue[r]->weight = w;
							}
							queue[r]->nodes.add(v);
							queue[r]->weight += e.o.dist(h.o);
							queue[r]->goal = h.o.dist(g.o);
							a++;
						}
					}
				}
				if (!a)
				{
					queue[q]->dead = true;
				} // this one ain't going anywhere..

				q = -1; // get shortest path
				loopvj(queue)
				{
					if (!queue[j]->dead && (!queue.inrange(q) || queue[j]->weight + queue[j]->goal < queue[q]->weight + queue[q]->goal))
						q = j;
				}
			}

			if (!queue.inrange(q) && !(flags & ROUTE_ABS)) // didn't get there, resort to failsafe proximity match
			{
				q = -1;

				loopvj(queue)
				{
					int u = -1;

					loopvrev(queue[j]->nodes) // find the closest node in this branch
					{
						if (!queue[j]->nodes.inrange(u) || ents[queue[j]->nodes[i]]->o.dist(g.o) < ents[queue[j]->nodes[u]]->o.dist(g.o))
							u = i;
					}

					if (queue[j]->nodes.inrange(u))
					{
						loopvrev(queue[j]->nodes) // trim the node list to the end at the shortest
						{
							if (i <= u)
								break;
							queue[j]->nodes.remove(i);
						}

						if (!queue.inrange(q) || ents[queue[j]->nodes[u]]->o.dist(g.o) < ents[queue[q]->nodes.last()]->o.dist(g.o))
							q = j;
					}
				}
			}

			if (queue.inrange(q))
			{
				route = queue[q]->nodes;
				result = true;
			}

			loopv(queue) DELETEP(queue[i]); // purge

			if (!result && !(flags & ROUTE_ABS)) // random search
			{
				for (int c = node; ents.inrange(c) && ents[c]->type == f.type; )
				{
					fpsentity &e = (fpsentity &) *ents[c];
					int b = -1;

					loopv(e.links)
					{
						int n = e.links[i];

						if (route.find(n) < 0 && (!(flags & ROUTE_AVOID) || avoid.find(n) >= 0))
							if (!ents.inrange(b) ||
									(ents.inrange(n) && ents[n]->o.dist(g.o) < ents[b]->o.dist(g.o)))
								b = n;
					}
					if (ents.inrange(b))
					{
						route.add(b);
					}
					c = b;
				}
				if (route.length())
					result = true;
			}
		}
		return result;
	}

	int waypointnode(vec &v, bool restrict = true, float d = 0.f, int n = -1)
	{
		int w = -1;
		loopv(ents) if(ents[i]->type == WAYPOINT && i != n)
		{
			if ((!restrict || ents[i]->o.dist(v) <= ents[i]->attr1+d) && (!ents.inrange(w) || ents[i]->o.dist(v) < ents[w]->o.dist(v)))
				w = i;
		}
		return w;
	}

	void waypointcheck(fpsent *d)
	{
		if(d->state == CS_ALIVE)
		{
			vec v(vec(d->o).sub(vec(0, 0, d->height-1)));

			if(dropwaypoints() && d == cl.player1)
			{
				int oldnode = d->lastnode;
				d->lastnode = waypointnode(v);
				if(!ents.inrange(d->lastnode)) d->lastnode = waypointnode(v, true, enttype[WAYPOINT].radius);
				if(!ents.inrange(d->lastnode))
				{
					conoutf("dropping waypoint.. (%.1f, %.1f, %1f)", v.x, v.y, v.z);
					d->lastnode = ents.length();
					newentity(v, WAYPOINT, enttype[WAYPOINT].radius, 0, 0, 0);
				}

				if(d->lastnode != oldnode && ents.inrange(oldnode) && ents.inrange(d->lastnode))
				{
					conoutf("linking waypoints %d and %d..", oldnode, d->lastnode);
					fpsentity &e = (fpsentity &)*ents[oldnode], &f = (fpsentity &)*ents[d->lastnode];
					if(e.links.find(d->lastnode) < 0)
						linkents(oldnode, d->lastnode, true, false);
					if(!d->timeinair && f.links.find(oldnode) < 0)
						linkents(d->lastnode, oldnode, true, false);
				}
			}
			else d->lastnode = waypointnode(v, false);
		}
		else d->lastnode = -1;
	}

	void readent(gzFile &g, int mtype, int mver, char *gid, int gver, int id, entity &e)
	{
		fpsentity &f = (fpsentity &)e;
		f.links.setsize(0);

		if(mtype == MAP_BFGZ)
		{
			int type = f.type;
			if(gver <= 49 && type >= 10) type--; // translation for these are done later..

			if(enttype[type].links && enttype[type].links <= gver)
			{
				int links = gzgetint(g);
				loopi(links) f.links.add(gzgetint(g));
				if(verbose >= 2) conoutf("entity %d loaded %d link(s)", id, links);
			}
		}
		else if(mtype == MAP_OCTA)
		{
			// sauerbraten version increments
			if(mver <= 10) if(f.type >= 7) f.type++;
			if(mver <= 12) if(f.type >= 8) f.type++;

			// now translate into our format
			if((f.type >= 14 && f.type <= 18) || f.type >= 26) f.type = NOTUSED;
			else if(f.type >= 8 && f.type <= 13)
			{
				int gun = f.type-8, gunmap[NUMGUNS] = {
					GUN_SG, GUN_CG, GUN_RL, GUN_RIFLE, GUN_GL, GUN_PISTOL
				};
				f.type -= gun;
				f.attr1 = gunmap[gun];
				f.attr2 = 0;
			}
			else if(f.type >= 19) f.type -= 10;
		}
	}

	void writeent(gzFile &g, int id, entity &e)
	{
		fpsentity &d = (fpsentity &)e;

		if(enttype[d.type].links)
		{
			vector<int> links;
			int n = 0;

			loopv(ents)
			{
				fpsentity &f = (fpsentity &)e;

				if(f.type != ET_EMPTY)
				{
					if(enttype[f.type].links)
						if(d.links.find(i) >= 0) links.add(n); // align to indices

					n++;
				}
			}

			gzputint(g, links.length());
			loopv(links) gzputint(g, links[i]); // aligned index
		}
	}

	void initents(gzFile &g, int mtype, int mver, char *gid, int gver)
	{
		if(gver <= 49 || mtype == MAP_OCTA)
		{
			vector<short> teleyaw;
			loopv(ents) teleyaw.add(0);

			loopv(ents)
			{
				fpsentity &e = (fpsentity &)*ents[i];

				if(e.type == 10) // translate teledest to teleport and link them appropriately
				{
					int dest = -1;

					loopvj(ents) // see if this guy is sitting on top of a teleport already
					{
						fpsentity &f = (fpsentity &)*ents[j];

						if(f.type == 9 && e.o.dist(f.o) <= enttype[TELEPORT].radius*2.f &&
							(!ents.inrange(dest) || e.o.dist(f.o) < ents[dest]->o.dist(f.o)))
								dest = j;
					}

					if(ents.inrange(dest))
					{
						e.type = NOTUSED; // get rid of this guy then
						conoutf("WARNING: replaced teledest %d [%d] with closest teleport", dest, i);
					}
					else
					{
						dest = i;
						e.type--; // no teleport nearby, make this guy one
						conoutf("WARNING: modified teledest %d [%d] to a teleport", dest, i);
					}

					teleyaw[dest] = e.attr1; // store the yaw for later

					loopvj(ents) // find linked teleport(s)
					{
						fpsentity &f = (fpsentity &)*ents[j];

						if(f.type == 9 && f.attr1 == e.attr2)
						{
							f.links.add(dest);
							conoutf("WARNING: teleports %d and %d linked automatically", dest, j);
						}
					}
				}
			}

			int fcnt = 0;

			loopv(ents)
			{
				fpsentity &e = (fpsentity &)*ents[i];
				if(e.type == 9)
				{
					e.attr1 = teleyaw[i]; // grab what we stored earlier
					e.attr2 = 0;
				}
				else if(e.type == 10) e.type = NOTUSED; // unused teledest?
				else if(e.type >= 11) e.type--;

				if(e.type == MAPSOUND)
				{
					e.attr2 = 255;
					e.attr3 = 0;
				}

				if(e.type == WAYPOINT) e.attr1 = enttype[WAYPOINT].radius;

				if(mtype == MAP_OCTA)
				{
					if(e.type == 20) e.attr1 = ++fcnt; // number them instead
					else if(e.type >= MAXENTTYPES) e.type = NOTUSED; // sanity check
				}
			}
		}
	}

	void renderlinked(extentity &e)
	{
		fpsentity &d = (fpsentity &)e;

		loopv(d.links)
		{
			int index = d.links[i];

			if(ents.inrange(index))
			{
				fpsentity &f = (fpsentity &)*ents[index];
				bool both = false,
					hassel = editmode && (entgroup.find(efocus) >= 0 || efocus == enthover);

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
				renderline(fr, to, col.x, col.y, col.z, hassel);

				if(hassel)
				{
					vec dr = to;
					float yaw, pitch;

					dr.sub(fr);
					dr.normalize();

					vectoyawpitch(dr, yaw, pitch);

					dr.mul(RENDERPUSHX);
					dr.add(fr);

					rendertris(dr, yaw, pitch, 2.f, col.x*2.f, col.y*2.f, col.z*2.f, true, hassel);
				}
			}
		}
	}

	void renderentshow(extentity &e, int level)
	{
		if(!level && showentradius() >= level)
		{
			int h = enttype[e.type].height, r = enttype[e.type].radius;
			switch(e.type)
			{
				case WAYPOINT:
					h = r = e.attr1;
					break;
				default:
					break;
			}
			if(h > 0.f || r > 0.f) renderradius(e.o, h, r);
		}

		switch(e.type)
		{
			case PLAYERSTART:
			case MAPMODEL:
			{
				if(!level || showentdir() >= level) renderdir(e.o, e.attr1, 0);
				break;
			}
			case TELEPORT:
			{
				if(!level || showteleportlinks() >= level) renderlinked(e);
				if(!level || showentdir() >= level) renderdir(e.o, e.attr1, 0);
				break;
			}
			case BASE:
			{
				if(!level || showbaselinks() >= level) renderlinked(e);
				break;
			}
			case CHECKPOINT:
			{
				if(!level || showcheckpointlinks() >= level) renderlinked(e);
				break;
			}
			case CAMERA:
			{
				if(!level || showcameralinks() >= level) renderlinked(e);
				if(!level || showentdir() >= level) renderdir(e.o, e.attr1, e.attr2);
				break;
			}
			case WAYPOINT:
			{
				if(!level || showwaypointlinks() >= level || dropwaypoints()) renderlinked(e);
				break;
			}
			default:
				break;
		}
	}

	void render()
	{
		if(rendernormally) // important, don't render lines and stuff otherwise!
		{
			#define entfocus(i, f) { int n = efocus = (i); if(n >= 0) { extentity &e = *ents[n]; f; } }
			int level = (editmode ? 1 : ((showentdir()==2 || showentradius()==2 || showbaselinks()==2 || showcheckpointlinks()==2 || showcameralinks()==2 || showteleportlinks()==2 || showwaypointlinks()==2 || dropwaypoints()) ? 2 : 0));
			if(level)
			{
				renderprimitive(true);

				if(editmode)
				{
					loopv(entgroup) entfocus(entgroup[i], renderentshow(e, 0));
					if(enthover >= 0) entfocus(enthover, renderentshow(e, 0));
				}

				loopv(ents)
				{
					entfocus(i, { renderentshow(e, level); });
				}

				renderprimitive(false);
			}

			loopv(ents) // sounds are here so they only execute once per frame
			{
				fpsentity &e = (fpsentity &)*ents[i];

				if(e.type == MAPSOUND && mapsounds.inrange(e.attr1))
				{
					if(!sounds.inrange(e.schan) || !sounds[e.schan].inuse)
					{
						int vol = e.attr2 >= 1 && e.attr2 <= 255 ? e.attr2 : 255;
						e.schan = playsound(e.attr1, &e.o, vol, -1, true, true);
					}
				}
			}
		}

		loopv(ents)
		{
			extentity &e = *ents[i];
			if(e.type == TELEPORT || (e.type == WEAPON && (e.spawned || editmode)))
			{
				const char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);

				if(mdlname)
				{
					rendermodel(&e.light, mdlname, ANIM_MAPMODEL|ANIM_LOOP,
						e.o, 0.f, 0.f, 0.f,
						MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED);
				}
			}
		}
	}
} et;
