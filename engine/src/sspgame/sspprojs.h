// Blood Frontier - SSPGAME - Side Scrolling Platformer by Quinton Reeves
// This is the SSP projectiles

struct sspprojectiles
{
	sspclient &cl;

	vector<projent *> projectiles;

	sspprojectiles(sspclient &_cl) : cl(_cl) {}
	~sspprojectiles() {}

	void radialeffect(sspent *o, vec &v, sspent *at, int gun)
	{
		if(o->state!=CS_ALIVE) return;

		vec dir;
		float dist = radialdist(o, dir, v);

		if(dist<EXPLOSION)
		{
			cl.wp.hit(1, o, at, dir);
		}
	}

	float radialdist(sspent *o, vec &dir, vec &v)
	{
		vec middle = o->o;
		middle.z += (o->aboveeye-o->eyeheight)/2;
		float dist = middle.dist(v, dir);
		dir.div(dist);
		if(dist<0) dist = 0;
		return dist;
	}

	void explode(sspent *owner, vec &v, int gun)
	{
		cl.sfx(SF_EXPLODE, &v);

		part_splash(3, 100, 300, v, COL_YELLOW);
		part_fireball(v, EXPLOSION*2, 16, gun==GN_RL ? COL_FIRERED : COL_FIREORANGE);
        adddynlight(v, EXPLOSION*2, vec(1, 0.75f, 0.5f), 1000, 100);

		int numdebris = rnd(getvar("debrisamt")-5)+5;
		vec debrisvel = vec(owner->o).sub(v).normalize(), debrisorigin(v);
		if(gun==GN_RL) debrisorigin.add(vec(debrisvel).mul(8));
		loopi(numdebris) spawnprojectile(debrisorigin, debrisvel, owner, 0.5f, 0.5f, PR_DEBRIS, rnd(4)+1, 0, 0, 0, 0, 0, rnd(1000)+1000, rnd(100)+20);

		loopi(cl.numdynents())
		{
			sspent *o = (sspent *)cl.iterdynents(i);
			if(!o || o==owner) continue;
			radialeffect(o, v, owner, gun);
		}
	}

	void newprojectile(const vec &from, const vec &to, sspent *owner, float height, float radius, int type, int enttype, int attr1, int attr2, int attr3, int attr4, int attr5, int lifetime, int speed, bool doclamp, bool addvel, int gun = -1)
	{
		projent &proj = *(projectiles.add(new projent));
		proj.reset();
		proj.type = ENT_BOUNCE;
		proj.state = CS_ALIVE;
		proj.o = from;
		proj.radius = proj.xradius = proj.yradius = radius;
		proj.eyeheight = height;
		proj.lifetime = lifetime;
		proj.owner = owner;
		proj.projtype = type;
		proj.enttype = enttype;
		proj.attr1 = attr1;
		proj.attr2 = attr2;
		proj.attr3 = attr3;
		proj.attr4 = attr4;
		proj.attr5 = attr5;

		proj.maxspeed = speed;
		proj.gun = gun;
		proj.doclamp = doclamp;

		if (proj.owner)
		{
			proj.direction = proj.owner->direction;
			proj.path = proj.owner->path;
			proj.clamp = proj.owner->clamp;

			if (proj.doclamp) cl.update2d(&proj);
		}

		proj.to = to;

		vec dir(proj.to);
		dir.sub(from).normalize();
		proj.vel = dir;
		proj.vel.mul(cl.ph.speed((physent *)&proj));

		if (addvel)
		{
			float oldz = proj.vel.z;
			proj.vel.add(owner->vel);
			proj.vel.z = oldz;
		}
		
		if (proj.projtype == PR_PROJ) proj.vel.z = 0;

		avoidcollision(&proj, dir, owner, proj.radius);
		proj.setposition(proj.direction, proj.o.x, proj.o.y, proj.o.z);

		proj.offset = cl.wp.gunorigin(gun, from, to, owner);
		proj.offset.sub(proj.o);
		proj.offsetmillis = OFFMILLIS;
	}

	void projupdate(int time)
	{
		loopv(projectiles)
		{
			projent &proj = *(projectiles[i]);
			
			if(proj.projtype == PR_BNC && vec(proj.vel).add(proj.gravity).magnitude() > 50.0f)
				part_splash(6, 1, 150, proj.o, COL_GREY);
			
			vec old(proj.o), oldv(proj.vel);
			float elast = 0.f;
			int rtime = time, mtime;
			int exploded = 0;

			switch (proj.projtype)
			{
				case PR_ENT:
					elast = cl.et.sspitems[proj.enttype].elast;
					mtime = 15;
					break;
				case PR_BNC:
					elast = 0.9f;
					mtime = 10;
					break;
				case PR_PROJ:
					elast = 1.f;
					mtime = 10;
					break;
				default:
					elast = 0.6f;
					mtime = 30;
					break;
			}

			while(rtime > 0)
			{
				int qtime = min(mtime, rtime);
				rtime -= qtime;
				
				if ((proj.lifetime -= qtime) <= 0)
				{
					exploded = 1;
					break;
				}

				if (bounce(&proj, qtime/1000.f, elast))
				{
					if (hitplayer != NULL && hitplayer->type == ENT_BLOCK)
					{
						if (proj.projtype == PR_PROJ)
						{
							exploded = 1;
							break;
						}
						else
						{
							vec v(proj.o);
							v.sub(hitplayer->o);
							
							if (v.z > 0 || v.z < -hitplayer->eyeheight)
							{
								proj.vel.z *= -elast;
								proj.gravity.z *= -elast;
								proj.o = old;
								continue;
							}
							else
							{
								if (proj.projtype == PR_BNC)
								{
									exploded = 1;
									break;
								}
								else
								{
									proj.vel.x *= -elast;
									proj.gravity.x *= -elast;
									proj.vel.y *= -elast;
									proj.gravity.y *= -elast;
									proj.o = old;
									continue;
								}
							}
						}
					}
					else if ((proj.projtype == PR_PROJ || proj.projtype == PR_BNC) && hitplayer != NULL && proj.owner != hitplayer)
					{
						exploded = 1;
						break;
					}
					else continue;
					
					if (proj.projtype == PR_ENT)
					{
						sspclient::sspentities::sspitem &t = cl.et.sspitems[proj.enttype];
						part_spawn(proj.o, vec(t.radius, t.radius, t.height), 0, 6, 20, 500, COL_GREY);
					}

					exploded = 1;
					break;
				}

				if (!exploded && proj.projtype == PR_PROJ)
				{
					proj.vel.z = proj.gravity.z = 0;
					proj.o.z = old.z;
				}
			}

			if (!exploded && proj.projtype == PR_PROJ)
			{
				proj.vel.z = proj.gravity.z = 0;
				proj.o.z = old.z;
			}
			
			if (!exploded)
			{
				if (proj.projtype == PR_ENT)
				{
					sspclient::sspentities::sspitem &t = cl.et.sspitems[proj.enttype];
					proj.yaw = (t.render == 1 ? lastmillis/10.0f : t.yaw);

					loopj(cl.numdynents())
					{
						sspent *d = (sspent *)cl.iterdynents(j);
						
						if (cl.et.checkentity(d, &proj, -1, proj.o, proj.enttype, proj.attr1, proj.attr2, proj.attr3, proj.attr4, proj.attr5))
						{
							exploded = 2;
							break;
						}
					}
				}
				else if (proj.projtype != PR_PROJ)
				{
					float mag = old.sub(proj.o).magnitude();
					proj.roll += mag/(4*RAD);
				}
				if (!exploded)
				{
					proj.offsetmillis = max(proj.offsetmillis-time, 0);
					cl.et.checkentities((actent *)&proj);
				}
			}

			#define isinverted(a,b) ((a > 0 && b < 0) || (a < 0 && b > 0))

			if (!exploded &&
				((proj.projtype == PR_PROJ || (proj.projtype == PR_BNC && proj.gun == GN_GL)) &&
				(isinverted(oldv.x, proj.vel.x) || isinverted(oldv.y, proj.vel.y))))
				exploded = 1;

			#define outsideworld(a) (a <= 0 || a >= getworldsize())

			if (!exploded &&
				(outsideworld(proj.o.x) || outsideworld(proj.o.y) || outsideworld(proj.o.z)))
				exploded = 1;

			if (!exploded &&
				((getvar("watertype") == WT_KILL && getmatvec(proj.o) == MAT_WATER) ||
				getmatvec(proj.o) == MAT_LAVA))
				exploded = 1;

			if (exploded > 0)
			{
				if (exploded == 1)
				{
					if (proj.gun >= 0)
					{
						explode(proj.owner, proj.o, proj.gun);
					}
					else if (proj.projtype == PR_ENT)
					{
						sspclient::sspentities::sspitem &t = cl.et.sspitems[proj.enttype];
						part_spawn(proj.o, vec(t.radius, t.radius, t.height), 0, 6, 100, 500, COL_GREY);
					}
				}
				
				delete &proj;
				projectiles.remove(i--);
			}
			else
			{
				if (proj.doclamp) cl.update2d(&proj);

				if (proj.projtype == PR_PROJ)
				{
					if (cl.wp.guns[proj.gun].part >= 0)
					{
						 part_splash(3, 2, 300, proj.o, COL_GREY); 
						 part_splash(cl.wp.guns[proj.gun].part, 1, 1, proj.o, cl.wp.guns[proj.gun].col);
					}
					else
					{
						part_splash(6, 2, 300, proj.o, COL_GREY); 
					}
				}
			}
		}
	}

	void projectileeset()
	{
		projectiles.deletecontentsp();
		projectiles.setsize(0);
	}

	void spawnprojectile(vec &p, vec &vel, sspent *d, float height, float radius, int type, int enttype, int attr1, int attr2, int attr3, int attr4, int attr5, int lifetime, int speed)
	{
		vec to(rnd(100)-50, rnd(100)-50, rnd(200)-100);
		to.normalize();
		to.add(p);
		newprojectile(p, to, d, height, radius, type, enttype, attr1, attr2, attr3, attr4, attr5, lifetime, speed, false, true);
	}

	void superdamageeffect(vec &vel, sspent *d)
	{
		vec from = d->abovehead();
		from.y -= 16;
		int numgibs = rnd(getvar("gibamt")-5)+5;
		loopi(numgibs) spawnprojectile(from, vel, d, 1.5f, 1.5f, PR_GIBS, rnd(2), 0, 0, 0, 0, 0, rnd(1000)+1000, rnd(100)+20);
	}

	void projprerender()
	{
		char *projs[] = {
			"projectiles/grenade", "projectiles/rocket",
			"gibc", "gibh",
			"debris/debris01", "debris/debris02", "debris/debris03", "debris/debris04"
		};
		show_out_of_renderloop_progress(0.f, "pre-rendering.. projectiles");
		loopi(8)
		{
			vec v(1, 1, 1);
			show_out_of_renderloop_progress(float(i)/float(8), "pre-rendering.. projectiles (models)");
			rendermodel(v, v, projs[i], ANIM_MAPMODEL|ANIM_LOOP, 0, 0, vec(cl.player1->o).add(vec(0, 0, 8.f)), 0, 0, 10.f, 0, NULL, MDL_SHADOW);
		}
		show_out_of_renderloop_progress(1.f, "pre-rendering.. projectiles done.");
	}

	void checkprojectiles()
	{
		projupdate(curtime);
	}

	void renderprojectiles()
	{
		vec color, dir;
		loopv(projectiles)
		{
			projent &proj = *(projectiles[i]);
			float yaw = proj.yaw, pitch = proj.pitch, depth = proj.path;
			vec vel(proj.vel);
			vel.add(proj.gravity);

			if (proj.doclamp)
			{
				if (isxaxis(proj.direction))
				{
					renderinc(proj.o.x, proj.renderdepth, 250, proj.renderpath, getworldsize(), false);
				}
				else
				{
					renderinc(proj.o.y, proj.renderdepth, 250, proj.renderpath, getworldsize(), false);
				}
			}
			
			if (proj.projtype == PR_GIBS || proj.projtype == PR_DEBRIS || proj.projtype == PR_BNC)
			{
				if (proj.projtype == PR_BNC && proj.gun != GN_BOMB)
				{
					if(vel.magnitude() <= 25.0f) proj.yaw = proj.lastyaw;
					else
					{
						vectoyawpitch(vel, proj.yaw, proj.pitch);
						proj.yaw += 90;
						proj.lastyaw = proj.yaw;
					}
					proj.pitch = -proj.roll;
				}
			}
			else if (proj.projtype == PR_PROJ)
			{
				vec v(proj.vel);
				v.normalize();
				// the amount of distance in front of the smoke trail needs to change if the model does
				vectoyawpitch(v, proj.yaw, proj.pitch);
				proj.yaw += 90;
				v.mul(3);
			}
			
			const char *mdl = NULL;
			string mdlname;
			
			if (proj.projtype == PR_GIBS)
			{
				mdl = proj.enttype ? "gibc" : "gibh";
			}
			else if (proj.projtype == PR_DEBRIS)
			{
					s_sprintf(mdlname)("debris/debris0%d", proj.enttype);
					mdl = mdlname;
			}
			else if (proj.projtype == PR_ENT)
			{
				mdl = cl.et.renderentmdl(proj.enttype, proj.attr1, proj.attr2, proj.attr3, proj.attr4, proj.attr5);
			}
			else if (proj.projtype == PR_BNC)
			{
				if (proj.gun == GN_GL) mdl = "projectiles/grenade";
				else if (proj.gun == GN_BOMB) mdl = "tentus/bombs";
			}
			else if (proj.projtype == PR_PROJ && proj.gun == GN_RL)
			{
				mdl = "projectiles/rocket";
			}

			if (mdl != NULL)
			{
				vec pos = proj.o;
				
				proj.o.add(vec(proj.offset).mul(proj.offsetmillis/float(OFFMILLIS)));
				lightreaching(proj.o, color, dir);

				if (isxaxis(proj.direction))
				{
					renderinc(proj.o.x, proj.renderdepth, 250, proj.renderpath, getworldsize(), false);
				}
				else
				{
					renderinc(proj.o.y, proj.renderdepth, 250, proj.renderpath, getworldsize(), false);
				}
				
				rendermodel(color, dir, mdl, ANIM_MAPMODEL|ANIM_LOOP, 0, 0, proj.o, proj.yaw, proj.pitch, 0, 0, NULL, MDL_CULL_DIST|MDL_CULL_VFC|(proj.projtype!=PR_DEBRIS ? MDL_SHADOW : 0));

				proj.o = pos;
			}
			proj.yaw = yaw;
			proj.pitch = pitch;

			if (proj.doclamp)
			{
				if (isxaxis(proj.direction)) proj.o.x = depth;
				else proj.o.y = depth;
			}
		}
	}
	
	void dynlightprojs()
	{
		loopv(projectiles)
		{
			projent &proj = *(projectiles[i]);
			
			if (proj.gun >= 0 && (proj.type == PR_PROJ || proj.type == PR_BNC))
			{
				switch (proj.gun)
				{
					case GN_RL:
					case GN_BOMB:
						adddynlight(proj.o, int(EXPLOSION*0.5f), vec(0.75f, 0.5f, 0.25f), 0, 0);
						break;
					case GN_FIREBALL:
						adddynlight(proj.o, int(EXPLOSION*0.5f), vec(0.75f, 0.25f, 0.25f), 0, 0);
						break;
					case GN_ICEBALL:
						adddynlight(proj.o, int(EXPLOSION*0.5f), vec(0.25f, 0.25f, 0.75f), 0, 0);
						break;
					case GN_SLIMEBALL:
						adddynlight(proj.o, int(EXPLOSION*0.5f), vec(0.25f, 0.75f, 0.25f), 0, 0);
						break;
					default:
						break;
				}
			}
			//else if (proj.type == PR_ENT)
			//{
			//	dynlightent
			//}
		}
	}
};
