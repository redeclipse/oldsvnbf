struct entities : icliententities
{
	GAMECLIENT &cl;

	vector<extentity *> ents;

	IVARP(showentdir, 0, 1, 1);
	IVARP(showentradius, 0, 1, 1);
	
	IVARP(showbaselinks, 0, 0, 1);
	IVARP(showcheckpointlinks, 0, 0, 1);
	IVARP(showcameralinks, 0, 0, 1);
	IVARP(showteleportlinks, 0, 0, 1);
	IVARP(showwaypointlinks, 0, 0, 1);

	entities(GAMECLIENT &_cl) : cl(_cl)
	{
		CCOMMAND(entlink, "", (entities *self), self->entlink());
	}

	vector<extentity *> &getents() { return ents; }

    const char *itemname(int i)
	{
		int t = ents[i]->type;
		if(t == WEAPON)
		{
			int gun = ents[i]->attr1;
			if (gun <= -1 || gun >= NUMGUNS) gun = 0;
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
			case TELEPORT:
				s_sprintf(emdl)("%s", enttype[type].name);
				break;
			default:
				break;
		}
		return emdl[0] ? emdl : NULL;
	}

    void preloadentities()
    {
        loopv(ents)
        {
        	extentity &e = *ents[i];
			const char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
            if(!mdlname) continue;
            loadmodel(mdlname, -1, true);
        }
    }

	void rumble(extentity &e) { playsound(S_RUMBLE, &e.o, true); }

	// these two functions are called when the server acknowledges that you really
	// picked up the item (in multiplayer someone may grab it before you).

	void useeffects(int n, fpsent *d)
	{
        if(!ents.inrange(n)) return;
		if (ents[n]->type == WEAPON && ents[n]->attr1 > -1 && ents[n]->attr1 < NUMGUNS)
		{
			ents[n]->spawned = false;
			if(!d) return;
			guntypes &g = guntype[ents[n]->attr1];
			if(d!=cl.player1 || isthirdperson()) particle_text(d->abovehead(), g.name, 15);
			playsound(S_ITEMAMMO, &d->o);
			if(d!=cl.player1) return;
			d->useitem(lastmillis, ents[n]->type, ents[n]->attr1, ents[n]->attr2);
		}
		else return;
	}

	// these functions are called when the client touches the item

	void teleport(int n, fpsent *d)	 // also used by monsters
	{
		fpsentity &e = (fpsentity &)*ents[n];
		
		while (e.links.length())
		{
			int link = rnd(e.links.length()), targ = e.links[link];
			
			if (ents.inrange(targ) && ents[targ]->type == TELEPORT)
			{
				d->o = ents[targ]->o;
				d->yaw = min(359, max(0, (int)ents[targ]->attr1));
				d->pitch = min(89, max(-89, (int)ents[targ]->attr2));
				float mag = max(48.f, d->vel.magnitude());
				d->vel = vec(0, 0, 0);
				vecfromyawpitch(d->yaw, 0, 1, 0, d->vel);
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

	void tryuse(int n, fpsent *d)
	{
		switch(ents[n]->type)
		{
			default:
				if (d->canuse(ents[n]->type, ents[n]->attr1, ents[n]->attr2, lastmillis))
				{
					if (d == cl.player1)
					{
						if (!cl.player1->usestuff) return;
						cl.cc.addmsg(SV_ITEMUSE, "ri", n);
					}
					ents[n]->spawned = false; // even if someone else gets it first
				}
				break;

			case TELEPORT:
			{
				if(d->lastuse == ents[n]->type && lastmillis-d->lastusemillis<500) break;
				d->lastuse = ents[n]->type;
				d->lastusemillis = lastmillis;
				teleport(n, d);
				break;
			}

			case CHECKPOINT:
				if(d!=cl.player1) break;
				if(n==cl.respawnent) break;
				cl.respawnent = n;
				conoutf("\f2respawn point set!");
				playsound(S_V_CHECKPOINT);
				break;

			case JUMPPAD:
			{
				if(d->lastuse==ents[n]->type && lastmillis-d->lastusemillis<500) break;
				d->lastuse = ents[n]->type;
				d->lastusemillis = lastmillis;
				vec v((int)(char)ents[n]->attr3*10.0f, (int)(char)ents[n]->attr2*10.0f, ents[n]->attr1*12.5f);
				d->timeinair = 0;
				d->gvel = vec(0, 0, 0);
				d->vel = v;
				cl.playsoundc(S_JUMPPAD, d);
				break;
			}
		}
	}

	void checkitems(fpsent *d)
	{
		if(d==cl.player1 && (editmode || cl.cc.spectator)) return;
		vec o = d->o;
		o.z -= d->height;
		loopv(ents)
		{
			extentity &e = *ents[i];
			if (e.type == NOTUSED) continue;
			if (!e.spawned && e.type!=TELEPORT && e.type!=JUMPPAD && e.type!=CHECKPOINT) continue;
			float dist = e.o.dist(o);
			if(dist < enttype[e.type].radius) tryuse(i, d);
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

	extentity *newentity() { return new fpsentity(); }

	void fixentity(extentity &e)
	{
		switch(e.type)
		{
			case WEAPON:
				while (e.attr1 <= -1) e.attr1 += NUMGUNS;
				while (e.attr1 >= NUMGUNS) e.attr1 -= NUMGUNS;
				if (e.attr2 < 0) e.attr2 = 0;
				break;
			default:
				break;
		}
	}

	const char *entnameinfo(entity &e) { return ""; }
	const char *entname(int i) { return i >= NOTUSED && i <= MAXENTTYPES ? enttype[i].name : ""; }

	void editent(int i)
	{
		extentity &e = *ents[i];
		if (e.type == ET_EMPTY) cleanlinks(i);
		fixentity(e);
		if (multiplayer(false))
			cl.cc.addmsg(SV_EDITENT, "ri9", i, (int)(e.o.x*DMF), (int)(e.o.y*DMF), (int)(e.o.z*DMF), e.type, e.attr1, e.attr2, e.attr3, e.attr4); // FIXME
	}

	float dropheight(entity &e)
	{
		if(e.type==MAPMODEL || e.type==BASE) return 0.0f;
		return 4.0f;
	}

	void entlink()
	{
		if (entgroup.length())
		{
			int index = entgroup[0];

			if (ents.inrange(index))
			{
				int type = ents[index]->type, last = -1;

				if (enttype[type].links)
				{
					loopv(entgroup)
					{
						index = entgroup[i];
	
						if (ents[index]->type == type)
						{
							if (ents.inrange(last))
							{
								int g;
	
								fpsentity &e = (fpsentity &)*ents[index];
								fpsentity &l = (fpsentity &)*ents[last];
	
								if ((g = l.links.find(index)) >= 0)
								{
									int h;
									if ((h = e.links.find(last)) >= 0) l.links.remove(g);
									else e.links.add(last);
								}
								else if ((g = e.links.find(last)) >= 0) e.links.remove(g);
								else l.links.add(index);
							}
							last = index;
						}
						else conoutf("entity %s is not linkable to a %s", enttype[type].name, enttype[index].name);
					}
				}
				else conoutf("entity %s is not linkable", enttype[type].name);
			}
		}
	}
	/*
	void entlink(int both)
	{
		if (entgroup.length())
		{
			int index = entgroup[0];

			if (ents.inrange(index))
			{
				int type = ents[index]->type, last = -1;

				if (enttype[type].links)
				{
					loopv(entgroup)
					{
						index = entgroup[i];
	
						if (ents[index]->type == type)
						{
							if (ents.inrange(last))
							{
								fpsentity &e = (fpsentity &)*ents[index];
								fpsentity &l = (fpsentity &)*ents[last];
	
								if (l.links.find(index) < 0) l.links.add(index);
								if (both && e.links.find(last) < 0) e.links.add(last);
							}
							last = index;
						}
						else conoutf("entity %s is not linkable to a %s", enttype[type].name, enttype[index].name);
					}
				}
				else conoutf("entity %s is not linkable", enttype[type].name);
			}
		}
	}
	*/
	void cleanlinks(int n)
	{
		loopv(ents) if (enttype[ents[i]->type].links)
		{
			fpsentity &e = (fpsentity &)*ents[i];

			loopvj(e.links)
			{
				if (e.links[j] == n)
				{
					e.links.remove(j);
					break;
				}
			}
		}
	}

	void gotocamera(int n, fpsent *d, int &delta)
	{
		int e = -1, tag = n, beenhere = -1;
		for (;;)
		{
			e = findentity(CAMERA, e + 1);
			if (e == beenhere || e < 0)
			{
				conoutf("no camera destination for tag %d", tag);
				return ;
			};
			if (beenhere < 0)
				beenhere = e;
			if (ents[e]->attr4 == tag)
			{
				d->o = ents[e]->o;
				d->yaw = ents[e]->attr1;
				d->pitch = ents[e]->attr2;
				delta = ents[e]->attr3;
				if (cl.player1->state == CS_EDITING)
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

	void readent(gzFile &g, int mtype, int mver, char *gid, int gver, int id, entity &e)
	{
		fpsentity &f = (fpsentity &)e;
		f.links.setsize(0);

		if (mtype == MAP_BFGZ)
		{
			int type = f.type;
			if (gver <= 49 && type >= 10) type--; // translation for these are done later..
			
			if (enttype[type].links && enttype[type].links <= gver)
			{
				int links = gzgetint(g);
				loopi(links) f.links.add(gzgetint(g));
				if (verbose >= 2) conoutf("entity %d loaded %d link(s)", id, links);
			}
		}
		else if (mtype == MAP_OCTA)
		{
			// sauerbraten version increments
			if (mver <= 10) if (f.type >= 7) f.type++;
			if (mver <= 12) if (f.type >= 8) f.type++;

			// now translate into our format
			if ((f.type >= 14 && f.type <= 18) || f.type >= 26) f.type = NOTUSED;
			else if (f.type >= 8 && f.type <= 13)
			{
				int gun = f.type-8, gunmap[NUMGUNS] = {
					GUN_SG, GUN_CG, GUN_RL, GUN_RIFLE, GUN_GL, GUN_PISTOL
				};
				f.type -= gun;
				f.attr1 = gunmap[gun];
				f.attr2 = 0;
			}
			else if (f.type >= 19) f.type -= 10;
		}
	}

	void writeent(gzFile &g, int id, entity &e)
	{
		fpsentity &d = (fpsentity &)e;

		if (enttype[d.type].links)
		{
			vector<int> links;
			int n = 0;

			loopv(ents)
			{
				fpsentity &f = (fpsentity &)e;
				
				if (f.type != ET_EMPTY)
				{
					if (enttype[f.type].links)
						if (d.links.find(i) >= 0) links.add(n); // align to indices

					n++;
				}
			}

			gzputint(g, links.length());
			loopv(links) gzputint(g, links[i]); // aligned index
		}
	}

	void initents(gzFile &g, int mtype, int mver, char *gid, int gver)
	{
		if (gver <= 49)
		{
			vector<short> teleyaw;
			loopv(ents) teleyaw.add(0);

			loopv(ents)
			{
				fpsentity &e = (fpsentity &)*ents[i];
			
				if (e.type == 10) // translate teledest to teleport and link them appropriately
				{
					int dest = -1;
					
					loopvj(ents) // see if this guy is sitting on top of a teleport already
					{
						fpsentity &f = (fpsentity &)*ents[j];
						
						if (f.type == 9 && e.o.dist(f.o) <= enttype[TELEPORT].radius*2.f &&
							(!ents.inrange(dest) || e.o.dist(f.o) < ents[dest]->o.dist(f.o)))
								dest = j;
					}

					if (ents.inrange(dest))
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
						
						if (f.type == 9 && f.attr1 == e.attr2)
						{
							f.links.add(dest);
							conoutf("WARNING: teleports %d and %d linked automatically", dest, j);
						}
					}
				}
			}
			loopv(ents)
			{
				fpsentity &e = (fpsentity &)*ents[i];
				if (e.type == 9)
				{
					e.attr1 = teleyaw[i]; // grab what we stored earlier
					e.attr2 = 0;
				}
				else if (e.type == 10) e.type = NOTUSED; // unused teledest?
				else if (e.type >= 11) e.type--;
			}
		}
	}

	void renderlinked(extentity &e)
	{
		fpsentity &d = (fpsentity &)e;

		loopv(d.links)
		{
			int index = d.links[i];

			if (ents.inrange(index))
			{
				fpsentity &f = (fpsentity &)*ents[index];
				bool both = false,
					hassel = editmode && (entgroup.find(efocus) >= 0 || efocus == enthover);

				loopvj(f.links)
				{
					int g = f.links[j];
					if (ents.inrange(g))
					{
						fpsentity &h = (fpsentity &)*ents[g];
						if (&h == &e)
						{
							both = true;
							break;
						}
					}
				}

				vec fr = d.o, to = f.o;

				fr.z += RENDERPUSHZ;
				to.z += RENDERPUSHZ;

				vec col(0.5f, 0, both ? 0.5f : 0.0f);
				renderline(fr, to, col.x, col.y, col.z, hassel);

				if (hassel)
				{
					vec dr = to;
					float yaw, pitch;

					dr.sub(fr);
					dr.normalize();

					vectoyawpitch(dr, yaw, pitch);

					dr.mul(RENDERPUSHX);
					dr.add(fr);

					rendertris(dr, yaw, pitch, 2.f, col.x*2.f, 0.0f, col.z*2.f, true, hassel);
				}
			}
		}
	}

	void renderentshow(extentity &e, bool force)
	{
		if (!force && showentradius())
		{
			if (enttype[e.type].height > 0.f || enttype[e.type].radius > 0.f)
			{
				renderradius(e.o, enttype[e.type].height, enttype[e.type].radius);
			}
		}

		switch (e.type)
		{
			case PLAYERSTART:
			case MAPMODEL:
			{
				if (!force && showentdir()) renderdir(e.o, e.attr1, 0);
				break;
			}
			case TELEPORT:
			{
				if (!force || showteleportlinks()) renderlinked(e);
				if (!force && showentdir()) renderdir(e.o, e.attr1, 0);
				break;
			}
			case BASE:
			{
				if (!force || showbaselinks()) renderlinked(e);
				break;
			}
			case CHECKPOINT:
			{
				if (!force || showcheckpointlinks()) renderlinked(e);
				break;
			}
			case CAMERA:
			{
				if (!force || showcameralinks()) renderlinked(e);
				if (!force && showentdir()) renderdir(e.o, e.attr1, e.attr2);
				break;
			}
			case WAYPOINT:
			{
				if (!force || showwaypointlinks()) renderlinked(e);
				break;
			}
			default:
				break;
		}
	}

	void renderentities()
	{
		if (rendernormally) // important, don't render lines and stuff otherwise!
		{
			#define entfocus(i, f) { int n = efocus = (i); if(n >= 0) { extentity &e = *ents[n]; f; } }
	
			if (editmode)
			{
				renderprimitive(true);
				
				loopv(entgroup) entfocus(entgroup[i], renderentshow(e, false));
				if (enthover >= 0) entfocus(enthover, renderentshow(e, false));
				
				loopv(ents)
				{
					entfocus(i, { renderentshow(e, true); });
				}
				
				renderprimitive(false);
			}
			
			loopv(ents) // sounds are here so they only execute once per frame
			{
				fpsentity &e = (fpsentity &)*ents[i];

				if (e.type == MAPSOUND && mapsounds.inrange(e.attr1))
				{
					if (!sounds.inrange(e.schan) || !sounds[e.schan].inuse)
						e.schan = playsound(e.attr1, &e.o, false, true);
				}
			}
		}

		loopv(ents)
		{
			extentity &e = *ents[i];
			if (e.type == TELEPORT || (e.type == WEAPON && (e.spawned || editmode)))
			{
				const char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);

				if (mdlname)
				{
					rendermodel(&e.light, mdlname, ANIM_MAPMODEL|ANIM_LOOP,
						0, 0, e.o, 0.f, 0.f, 0.f, 0.f, 0, NULL,
						MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED);
				}
			}
		}
	}
} et;
