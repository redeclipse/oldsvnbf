
struct entities : icliententities
{
	fpsclient &cl;
	
	vector<extentity *> ents;

	entities(fpsclient &_cl) : cl(_cl)
	{
#ifdef BFRONTIER
		CCOMMAND(entities, entdelink, "i", self->extentdelink(atoi(args[0])));
		CCOMMAND(entities, entlink, "i", self->extentlink(atoi(args[0])));
#endif
	}

	vector<extentity *> &getents() { return ents; }
	
	char *itemname(int i)
	{
#ifdef BFRONTIER
		if (g_bf) return NULL;
		int t = ents[i]->type;
		if(t<I_SHELLS || t>I_QUAD) return NULL;
		return getitem(t-I_SHELLS).name;
#else
		int t = ents[i]->type;
		if(t<I_SHELLS || t>I_QUAD) return NULL;
		return itemstats[t-I_SHELLS].name;
#endif
	}
	
	char *entmdlname(int type)
	{
#ifdef BFRONTIER
		if (g_bf)
		{
			static char *bfmdlnames[] =
			{
				NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
				"ammo/shells", "ammo/bullets", "ammo/rockets", "ammo/rrounds", "ammo/grenades", "ammo/cartridges",
				"health", "boost", "armor/green", "armor/yellow", "quad", "teleporter",
				NULL, NULL,
				"carrot",
				NULL, NULL,
				"checkpoint"
			};
			return bfmdlnames[type];
		}
#endif
		static char *entmdlnames[] =
		{
			NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
			"ammo/shells", "ammo/bullets", "ammo/rockets", "ammo/rrounds", "ammo/grenades", "ammo/cartridges",
			"health", "boost", "armor/green", "armor/yellow", "quad", "teleporter",
			NULL, NULL,
			"carrot",
			NULL, NULL,
			"checkpoint"
		};
		return entmdlnames[type];
	}

	void renderent(extentity &e, int type, float z, float yaw, int anim = ANIM_MAPMODEL|ANIM_LOOP, int basetime = 0, float speed = 10.0f)
	{
		char *mdlname = entmdlname(type);
		if(!mdlname) return;
		rendermodel(e.color, e.dir, mdlname, anim, 0, 0, vec(e.o).add(vec(0, 0, z)), yaw, 0, speed, basetime, NULL, MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED);
	}

	void renderentities()
	{
#ifdef BFRONTIER
#define entfocus(i, f) \
	{ int n = efocus = (i); if(n>=0) { extentity &e = *ents[n]; f; } }

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
#endif
		loopv(ents)
		{
			extentity &e = *ents[i];

#ifdef BFRONTIER
			if (isext(e.type))
			{
				extentitem &t = extentitems[e.type - EXTENT];

				if (!t.render)
					continue;

				float z = (t.zoff == -1 ? (float)(1 + sin(cl.lastmillis / 100.0 + e.o.x + e.o.y) / 20) : t.zoff);
				float yaw = (t.yaw == -1 ? cl.lastmillis / 10.0f : t.yaw);
				char *imdl = extentmdl(e);

				if (imdl) extentityrender(e, imdl, z, yaw);
			}
			else
			{
#endif
				if(e.type==CARROT || e.type==RESPAWNPOINT)
				{
					renderent(e, e.type, (float)(1+sin(cl.lastmillis/100.0+e.o.x+e.o.y)/20), cl.lastmillis/(e.attr2 ? 1.0f : 10.0f));
					continue;
				}
				if(!e.spawned && e.type!=TELEPORT) continue;
				if(e.type<I_SHELLS || e.type>TELEPORT) continue;
				renderent(e, e.type, (float)(1+sin(cl.lastmillis/100.0+e.o.x+e.o.y)/20), cl.lastmillis/10.0f);
#ifdef BFRONTIER
			}
#endif
		}
	}

	void rumble(const extentity &e)
	{
		playsound(S_RUMBLE, &e.o);
	}

	void trigger(extentity &e)
	{
		switch(e.attr3)
		{
			case 29:
				cl.ms.endsp(false);
				break;
		}
	}

	void addammo(int type, int &v, bool local = true)
	{
#ifdef BFRONTIER
		itemstat &is = getitem(type-I_SHELLS);
#else
		itemstat &is = itemstats[type-I_SHELLS];
#endif
		v += is.add;
		if(v>is.max) v = is.max;
		if(local) cl.playsoundc(is.sound);
	}

	void repammo(fpsent *d, int type)
	{
		addammo(type, d->ammo[type-I_SHELLS+GUN_SG]);
	}

	// these two functions are called when the server acknowledges that you really
	// picked up the item (in multiplayer someone may grab it before you).

	void pickupeffects(int n, fpsent *d)
	{
        if(!ents.inrange(n)) return;
		int type = ents[n]->type;
		if(type<I_SHELLS || type>I_QUAD) return;
        ents[n]->spawned = false;
        if(!d) return;
#ifdef BFRONTIER
		itemstat &is = getitem(type-I_SHELLS);
		if(d!=cl.player1 || isthirdperson()) particle_text(d->abovehead(), is.name, 15);
		playsound(getitem(type-I_SHELLS).sound, d!=cl.player1 ? &d->o : NULL); 
#else
		itemstat &is = itemstats[type-I_SHELLS];
		if(d!=cl.player1 || isthirdperson()) particle_text(d->abovehead(), is.name, 15);
		playsound(itemstats[type-I_SHELLS].sound, d!=cl.player1 ? &d->o : NULL); 
#endif
		if(d!=cl.player1) return;
		d->pickup(type);
		switch(type)
		{
			case I_BOOST:
				conoutf("\f2you have a permanent +10 health bonus! (%d)", d->maxhealth);
				playsound(S_V_BOOST);
				break;

			case I_QUAD:
				conoutf("\f2you got the quad!");
				playsound(S_V_QUAD);
				break;
		}
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
#ifdef BFRONTIER
				cl.bc.wayupdpos(d, ents[n]->o);
#endif
				d->o = ents[e]->o;
				d->yaw = ents[e]->attr1;
				d->pitch = 0;
				d->vel = vec(0, 0, 0);//vec(cosf(RAD*(d->yaw-90)), sinf(RAD*(d->yaw-90)), 0);
				entinmap(d);
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
				if(d->canpickup(ents[n]->type))
				{
#ifdef BFRONTIER
					if (d==cl.player1) cl.cc.addmsg(SV_ITEMPICKUP, "ri", n);
					else if (cl.bc.isbot(d)) cl.bc.pickup(n, d);
#else
					cl.cc.addmsg(SV_ITEMPICKUP, "ri", n);
#endif
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
#ifdef BFRONTIER
				cl.bc.wayupdpos(d, ents[n]->o);
#endif
				d->lastpickup = ents[n]->type;
				d->lastpickupmillis = cl.lastmillis;
				vec v((int)(char)ents[n]->attr3*10.0f, (int)(char)ents[n]->attr2*10.0f, ents[n]->attr1*12.5f);
				d->timeinair = 0;
				d->gravity = vec(0, 0, 0);
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

	void checkquad(int time, fpsent *d)
	{
		if(d->quadmillis && (d->quadmillis -= time)<=0)
		{
			d->quadmillis = 0;
			cl.playsoundc(S_PUPOUT, d);
			if(d==cl.player1) conoutf("\f2quad damage is over");
		}
	}

	void putitems(ucharbuf &p, int gamemode)			// puts items in network stream and also spawns them locally
	{
		loopv(ents) if(ents[i]->type>=I_SHELLS && ents[i]->type<=I_QUAD && (!m_capture || ents[i]->type<I_SHELLS || ents[i]->type>I_CARTRIDGES))
		{
			putint(p, i);
			putint(p, ents[i]->type);
			
			ents[i]->spawned = (m_sp || (ents[i]->type!=I_QUAD && ents[i]->type!=I_BOOST)); 
		}
	}

	void resetspawns() { loopv(ents) ents[i]->spawned = false; }
	void setspawn(int i, bool on) { if(ents.inrange(i)) ents[i]->spawned = on; }

	extentity *newentity() { return new fpsentity(); }

	void fixentity(extentity &e)
	{
#ifdef BFRONTIER
		extentfix(e);
#endif
		switch(e.type)
		{
			case MONSTER:
			case TELEDEST:
				e.attr2 = e.attr1;
			case RESPAWNPOINT:
				e.attr1 = (int)cl.player1->yaw;
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
				dir = vec(e.attr3, e.attr2, e.attr1).normalize();
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
#ifdef BFRONTIER
		if (i >= MAXENTTYPES) return extentname(i);
#endif
		static const char *entnames[] =
		{
			"none?", "light", "mapmodel", "playerstart", "envmap", "particles", "sound", "spotlight",
			"shells", "bullets", "rockets", "riflerounds", "grenades", "cartridges",
			"health", "healthboost", "greenarmour", "yellowarmour", "quaddamage",
			"teleport", "teledest",
			"monster", "carrot", "jumppad",
			"base", "respawnpoint",
			"", "", "", "",
		};
		return i>=0 && size_t(i)<sizeof(entnames)/sizeof(entnames[0]) ? entnames[i] : "";
	}
	
	int extraentinfosize() { return 0; }		// size in bytes of what the 2 methods below read/write... so it can be skipped by other games

	void writeent(entity &e, char *buf)	// write any additional data to disk (except for ET_ ents)
	{
	}

	void readent(entity &e, char *buf)	 // read from disk, and init
	{
		int ver = getmapversion();
		if(ver <= 10)
		{
			if(e.type >= 7) e.type++;
		}
		if(ver <= 12)
		{
			if(e.type >= 8) e.type++;
		}
	}

	void editent(int i)
	{
		extentity &e = *ents[i];
#ifdef BFRONTIER
		extentedit(i);
		if (multiplayer(false) && e.type < MAXENTTYPES) cl.cc.addmsg(SV_EDITENT, "ri9", i, (int)(e.o.x*DMF), (int)(e.o.y*DMF), (int)(e.o.z*DMF), e.type, e.attr1, e.attr2, e.attr3, e.attr4); // FIXME
#else
		cl.cc.addmsg(SV_EDITENT, "ri9", i, (int)(e.o.x*DMF), (int)(e.o.y*DMF), (int)(e.o.z*DMF), e.type, e.attr1, e.attr2, e.attr3, e.attr4);
#endif
	}

	float dropheight(entity &e)
	{
		if(e.type==MAPMODEL || e.type==BASE) return 0.0f;
		return 4.0f;
	}

#ifdef BFRONTIER
	IVARP(showallwp, 0, 0, 1);

	const char *extentname(int i)
	{
		bool chk = isext(i);
		if (i >= MAXENTTYPES && !chk)
			return "reserved"; // so we can add extents
		return chk ? extentitems[i - EXTENT].name : "";
	}

	bool isext(int type, int want = 0)
	{
		return type >= EXTENT && type < MAXEXTENT && (!want || type == want);
	}

	bool extentpure(int i)
	{
		return ents.inrange(i) && isext(ents[i]->type) ? extentitems[i - EXTENT].pure : false;
	}

	void extentdelink(int both)
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

	void extentlink(int both)
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

	void extentlinks(int n)
	{
		loopv(ents)
		{
			if (isext(ents[i]->type))
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
	}

	void extentfix(extentity &d)
	{
		if (isext(d.type))
		{
			fpsentity &e = (fpsentity &)d;

			e.links.setsize(0);

			switch (e.type)
			{
				case CAMERA:  // place with "newent camera idx [pan]"
				e.attr4 = e.attr1;
				e.attr3 = e.attr2;
				e.attr2 = (int)cl.player1->pitch;
				e.attr1 = (int)cl.player1->yaw;
				break;
				case WAYPOINT:
				e.attr1 = e.attr1 >= 0 && e.attr1 <= 1 ? e.attr1 : (e.attr1 > 1 ? 0 : 1);
				e.attr2 = e.attr2 >= 0 ? e.attr2 : 0;
				e.attr3 = e.attr3 >= 0 ? e.attr3 : 0;
				break;
				default:
				break;
			}
		}
	}

	void extentedit(int i)
	{
		if (ents[i]->type == ET_EMPTY)
			extentlinks(i); // cleanup links
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

	bool wantext() { return true; }

	void readext(gzFile &g, int version, int reg, int id, entity &e)
	{
		fpsentity &f = (fpsentity &)e;
		
		f.links.setsize(0);
		
		if (version >= 2)
		{
			int r = reg >= 0 ? reg : 0, n = id + n; // realign to entity table
			
			switch (f.type)
			{
				case WAYPOINT:
				{
					int links = gzgetint(g);
		
					loopi(links)
					{
						int link = gzgetint(g) + r;
						f.links.add(link);
					}
				}
			}
		}
	}

	void writeext(gzFile &g, int reg, int id, int num, entity &e)
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
					if (isext(ents[i]->type))
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

	void extentityrender(extentity &e, const char *mdlname, float z, float yaw, int frame = 0, int anim = ANIM_MAPMODEL | ANIM_LOOP, int basetime = 0, float speed = 10.0f)
	{
		rendermodel(e.color, e.dir, mdlname, anim, 0, 0, vec(e.o).add(vec(0, 0, z)), yaw, 0, speed, basetime, NULL, MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_DIST | MDL_CULL_OCCLUDED);
	}

	char *extentmdl(extentity &e)
	{
		char *imdl = NULL;
		/*
		switch (e.type)
		{
			default:
				break;
		}
		*/
		return imdl;
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
					if (ents.inrange(g) && isext(ents[g]->type))
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

					if (showentradius)
					{
						renderentradius(d.o, BOTISNEAR, BOTISNEAR);
						renderentradius(d.o, BOTISFAR, BOTISFAR);
					}
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
			if (isext(e.type))
			{
				extentitem &t = extentitems[e.type - EXTENT];
				renderentradius(e.o, t.dist, t.dist);
			}
			else if (e.type < MAXENTTYPES && e.type >= I_SHELLS &&
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
#endif
};
