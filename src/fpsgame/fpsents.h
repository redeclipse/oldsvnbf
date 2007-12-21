struct entities : icliententities
{
	fpsclient &cl;

	vector<extentity *> ents;

	entities(fpsclient &_cl) : cl(_cl)
	{
		CCOMMAND(entdelink, "i", (entities *self, int *val), self->entdelink(*val));
		CCOMMAND(entlink, "i", (entities *self, int *val), self->entlink(*val));
	}

	vector<extentity *> &getents() { return ents; }

	char *itemname(int i)
	{
		int t = ents[i]->type;
		if(t == WEAPON)
		{
			int gun = ents[i]->attr1;
			if (gun <= -1 || gun >= NUMGUNS) gun = 0;
			return guns[gun].name;
		}
		return NULL;
	}

	char *entmdlname(int type, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0)
	{
		static char *bfmdlnames[] =
		{
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			NULL, NULL, NULL,
			"carrot",
			NULL, NULL,
			"checkpoint",
			NULL, NULL, NULL
		};
		static char *bfgunnames[] = {
			"ammo/pistol", "ammo/shotgun", "ammo/chaingun", "ammo/grenades", "ammo/rockets", "ammo/rifle"
		};
		switch (type)
		{
			case WEAPON:
				return bfgunnames[attr1];
			default:
				return bfmdlnames[type];
		}
	}

	void renderentities()
	{
		#define entfocus(i, f) { int n = efocus = (i); if(n >= 0) { extentity &e = *ents[n]; f; } }

		if (editmode || showallwp())
		{
			renderprimitive(true);
			if (editmode)
			{	loopv(entgroup) entfocus(entgroup[i], renderentshow(e));
				if (enthover >= 0) entfocus(enthover, renderentshow(e));
			}
			loopv(ents)
			{
				entfocus(i, {
					if (editmode || (showallwp() && e.type == WAYPOINT))
						renderentforce(e);
				});
			}
			renderprimitive(false);
		}

		loopv(ents)
		{
			extentity &e = *ents[i];

			if (e.type == CARROT || e.type == RESPAWNPOINT || e.type == TELEPORT ||
				(e.type==WEAPON && e.spawned))
			{
				char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);

				if (mdlname)
				{
					rendermodel(e.color, e.dir, mdlname, ANIM_MAPMODEL|ANIM_LOOP,
						0, 0, e.o, 0.f, 0.f, 0.f, 0.f, 0, NULL,
						MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED);
				}
			}
		}
	}

    void preloadentities()
    {
        loopv(ents)
        {
        	extentity &e = *ents[i];
			char *mdlname = entmdlname(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
            if(!mdlname) continue;
            loadmodel(mdlname, -1, true);
        }
    }

	void rumble(extentity &e) { playsound(S_RUMBLE, &e.o); }

	void trigger(extentity &e)
	{
		switch(e.attr3)
		{
			case 29:
				if (m_sp(cl.gamemode)) cl.intermission = true;
				break;
		}
	}

	// these two functions are called when the server acknowledges that you really
	// picked up the item (in multiplayer someone may grab it before you).

	void pickupeffects(int n, fpsent *d)
	{
        if(!ents.inrange(n)) return;
		if (ents[n]->type == WEAPON && ents[n]->attr1 > -1 && ents[n]->attr1 < NUMGUNS)
		{
			ents[n]->spawned = false;
			if(!d) return;
			guninfo &g = guns[ents[n]->attr1];
			if(d!=cl.player1 || isthirdperson()) particle_text(d->abovehead(), g.name, 15);
			playsound(S_ITEMAMMO, &ents[n]->o);
			if(d!=cl.player1) return;
			d->pickup(cl.lastmillis, ents[n]->type, ents[n]->attr1, ents[n]->attr2);
		}
		else return;
	}

	// these functions are called when the client touches the item

	void teleport(int n, fpsent *d)	 // also used by monsters
	{
		int e = -1, tag = ents[n]->attr1, beenhere = -1;
		for(;;)
		{
			e = findentity(TELEDEST, e+1);
			if(e==beenhere || e<0) { conoutf("no teleport destination for tag %d", tag); return; }
			if(beenhere<0) beenhere = e;
			if(ents[e]->attr2==tag)
			{
				d->o = ents[e]->o;
				d->yaw = ents[e]->attr1;
				d->pitch = 0;
				d->vel = vec(0, 0, 0);//vec(cosf(RAD*(d->yaw-90)), sinf(RAD*(d->yaw-90)), 0);
				cl.ph.entinmap(d, false);
				cl.playsoundc(S_TELEPORT, d);
				break;
			}
		}
	}

	void trypickup(int n, fpsent *d)
	{
		switch(ents[n]->type)
		{
			default:
				if (d->canpickup(ents[n]->type, ents[n]->attr1, ents[n]->attr2, cl.lastmillis))
				{
					if (d == cl.player1)
					{
						if (!cl.player1->pickingup) return;
						cl.cc.addmsg(SV_ITEMPICKUP, "ri", n);
					}
					ents[n]->spawned = false; // even if someone else gets it first
				}
				break;

			case TELEPORT:
			{
				if(d->lastpickup==ents[n]->type && cl.lastmillis-d->lastpickupmillis<500) break;
				d->lastpickup = ents[n]->type;
				d->lastpickupmillis = cl.lastmillis;
				teleport(n, d);
				break;
			}

			case RESPAWNPOINT:
				if(d!=cl.player1) break;
				if(n==cl.respawnent) break;
				cl.respawnent = n;
				conoutf("\f2respawn point set!");
				playsound(S_V_RESPAWNPOINT);
				break;

			case JUMPPAD:
			{
				if(d->lastpickup==ents[n]->type && cl.lastmillis-d->lastpickupmillis<300) break;
				d->lastpickup = ents[n]->type;
				d->lastpickupmillis = cl.lastmillis;
				vec v((int)(char)ents[n]->attr3*10.0f, (int)(char)ents[n]->attr2*10.0f, ents[n]->attr1*12.5f);
				d->timeinair = 0;
				d->gvel = vec(0, 0, 0);
				d->vel = v;
//				d->vel.z = 0;
//				d->vel.add(v);
				cl.playsoundc(S_JUMPPAD, d);
				break;
			}
		}
	}

	void checkitems(fpsent *d)
	{
		if(d==cl.player1 && (editmode || cl.cc.spectator)) return;
		vec o = d->o;
		o.z -= d->eyeheight;
		loopv(ents)
		{
			extentity &e = *ents[i];
			if(e.type==NOTUSED) continue;
			if(!e.spawned && e.type!=TELEPORT && e.type!=JUMPPAD && e.type!=RESPAWNPOINT) continue;
			float dist = e.o.dist(o);
			if(dist<(e.type==TELEPORT ? 16 : 12)) trypickup(i, d);
		}
	}

	void putitems(ucharbuf &p)
	{
		loopv(ents) if (ents[i]->type == WEAPON)
		{
			putint(p, i);
			putint(p, ents[i]->type);
			putint(p, ents[i]->attr1);
			putint(p, ents[i]->attr2);
			putint(p, ents[i]->attr3);
			putint(p, ents[i]->attr4);
			putint(p, ents[i]->attr5);
			ents[i]->spawned = true;
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

	void entradius(extentity &e, float &radius, float &angle, vec &dir)
	{
		switch(e.type)
		{
			case TELEPORT:
				loopv(ents) if(ents[i]->type == TELEDEST && e.attr1==ents[i]->attr2)
				{
					radius = e.o.dist(ents[i]->o);
					dir = vec(ents[i]->o).sub(e.o).normalize();
					break;
				}
				break;

			case JUMPPAD:
				radius = 4;
                dir = vec((int)(char)e.attr3*10.0f, (int)(char)e.attr2*10.0f, e.attr1*12.5f).normalize();
				break;

			case MONSTER:
			case TELEDEST:
			case MAPMODEL:
			case RESPAWNPOINT:
				radius = 4;
				vecfromyawpitch(e.attr1, 0, 1, 0, dir);
				break;
		}
	}

	const char *entnameinfo(entity &e) { return ""; }
	const char *entname(int i)
	{
		static const char *entnames[] =
		{
			"none?", "light", "mapmodel", "playerstart", "envmap", "particles", "sound", "spotlight",
			"weapon", "teleport", "teledest",
			"monster", "carrot", "jumppad",
			"base", "respawnpoint",
			"camera", "waypoint",
            "", "", "", ""
		};
		return i>=0 && size_t(i)<sizeof(entnames)/sizeof(entnames[0]) ? entnames[i] : "";
	}

	void editent(int i)
	{
		extentity &e = *ents[i];
		if (e.type == ET_EMPTY) entlinks(i);
		fixentity(e);
		if (multiplayer(false))
			cl.cc.addmsg(SV_EDITENT, "ri9", i, (int)(e.o.x*DMF), (int)(e.o.y*DMF), (int)(e.o.z*DMF), e.type, e.attr1, e.attr2, e.attr3, e.attr4); // FIXME
	}

	float dropheight(entity &e)
	{
		if(e.type==MAPMODEL || e.type==BASE) return 0.0f;
		return 4.0f;
	}

	IVARP(showallwp, 0, 0, 1);

	void entdelink(int both)
	{
		if (entgroup.length() > 0)
		{
			int index = entgroup[0];

			if (ents.inrange(index))
			{
				int type = ents[index]->type, last = -1;

				if (type != WAYPOINT)
					return ; // we only link waypoints so far

				loopv(entgroup)
				{
					index = entgroup[i];

					if (ents.inrange(index) && ents[index]->type == type)
					{
						if (ents.inrange(last))
						{
							int g;

							fpsentity &e = (fpsentity &)*ents[index];
							fpsentity &l = (fpsentity &)*ents[last];

							if ((g = l.links.find(index)) >= 0)
								l.links.remove(g);
							if (both && ((g = e.links.find(last)) >= 0))
								e.links.remove(g);
						}
						last = index;
					}
				}
			}
		}
	}

	void entlink(int both)
	{
		if (entgroup.length() > 0)
		{
			int index = entgroup[0];

			if (ents.inrange(index))
			{
				int type = ents[index]->type, last = -1;

				if (type != WAYPOINT)
					return ; // we only link waypoints so far

				loopv(entgroup)
				{
					index = entgroup[i];

					if (ents.inrange(index) && ents[index]->type == type)
					{
						if (ents.inrange(last))
						{
							fpsentity &e = (fpsentity &)*ents[index];
							fpsentity &l = (fpsentity &)*ents[last];

							if (l.links.find(index) < 0)
								l.links.add(index);
							if (both && e.links.find(last) < 0)
								e.links.add(last);
						}
						last = index;
					}
				}
			}
		}
	}

	void entlinks(int n)
	{
		loopv(ents) if (ents[i]->type == WAYPOINT)
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

	void readent(gzFile &g, int maptype, int id, entity &e)
	{
		if (maptype == MAP_BFGZ)
		{
			fpsentity &f = (fpsentity &)e;
			
			switch (f.type)
			{
				case WAYPOINT:
				{
					int links = gzgetint(g);
					f.links.setsize(0);
					loopi(links) f.links.add(gzgetint(g));
				}
			}
		}
		else if (maptype == MAP_OCTA)
		{
			int ver = getmapversion();

			if (ver <= 10)
			{
				if(e.type >= 7) e.type++;
			}
			if (ver <= 12)
			{
				if(e.type >= 8) e.type++;
			}

			if (e.type >= 8 && e.type <= 13)
			{
				int gun = e.type-8, gunmap[NUMGUNS] = {
					GUN_SG, GUN_CG, GUN_RL, GUN_RIFLE, GUN_GL, GUN_PISTOL
				};
				e.type = WEAPON;
				e.attr1 = gunmap[gun];
				e.attr2 = 0;
			}
			else if ((e.type >= 14 && e.type <= 18) || e.type >= 26) e.type = NOTUSED;
			else if (e.type >= 19) e.type -= 10;
		}
	}

	void writeent(gzFile &g, int id, entity &e)
	{
		fpsentity &f = (fpsentity &)e;

		switch (f.type)
		{
			case WAYPOINT:
			{
				vector<int> links;
				int n = 0;

				loopv(ents)
				{
					if (ents[i]->type != ET_EMPTY)
					{
						if (ents[i]->type == WAYPOINT)
						{
							if (f.links.find(i) >= 0) links.add(n); // align to indices
						}
						n++;
					}
				}

				gzputint(g, links.length());
				loopv(links)
				{
					gzputint(g, links[i]); // aligned index
				}
			}
		}
	}

	void renderwaypoint(extentity &e)
	{
		fpsentity &d = (fpsentity &)e;

		loopv(d.links)
		{
			int index = d.links[i];

			if (ents.inrange(index))
			{
				fpsentity &f = (fpsentity &)*ents[index];
				bool both = false, hassel = editmode && (entgroup.find(efocus) >= 0 || efocus == enthover);

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

				vec col(both ? 0.5f : 0.0f, 0, 0.5f);
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

					rendertris(dr, yaw, pitch, 2.f, col.x, col.y, col.z, true, hassel);
				}
			}
		}
	}

	void renderentforce(extentity &e)
	{
		switch (e.type)
		{
			case WAYPOINT:
			{
				if (showentdir && showallwp())
					renderwaypoint(e);
				break;
			}
			default:
			break;
		}
	}

	void renderentshow(extentity &e)
	{
		if (showentradius)
		{
			if (e.type <= MAXENTTYPES && e.type >= WEAPON &&
					 e.type != TELEPORT && e.type != TELEDEST && e.type != MONSTER)
			{
				if (e.type == BASE)
					renderentradius(e.o, cl.cpc.CAPTURERADIUS, cl.cpc.CAPTURERADIUS);
				else
					renderentradius(e.o, 12.f, 12.f);
			}
		}

		switch (e.type)
		{
			case ET_PLAYERSTART:
			case ET_MAPMODEL:
			{
				if (showentdir)
				{
					renderentdir(e.o, e.attr1, 0);
				}
				break;
			}
			case CAMERA:
			{
				if (showentdir && e.attr1 >= 0.f && e.attr1 <= 360.f)
				{
					renderentdir(e.o, e.attr1, e.attr2);
				}
				break;
			}
			case WAYPOINT:
			{
				if (showentdir && !showallwp())
					renderwaypoint(e);
				break;
			}
			default:
			break;
		}
	}
} et;
