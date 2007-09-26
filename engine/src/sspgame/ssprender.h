// Blood Frontier - SSPGAME - Side Scrolling Platformer by Quinton Reeves
// This is the SSP rendering processes.

void fixcamerarange(physent *d)
{
	const float MAXPITCH = 90.0f;
	if(d->pitch>MAXPITCH) d->pitch = MAXPITCH;
	if(d->pitch<-MAXPITCH) d->pitch = -MAXPITCH;
	while(d->yaw<0.0f) d->yaw += 360.0f;
	while(d->yaw>=360.0f) d->yaw -= 360.0f;
}

void fixcamerarange()
{
	physent *d = iscamera() ? (physent *)player1 : (menuactive() ? (physent *)camera : NULL);
	if (d) fixcamerarange(d);
}

void mousemove(int dx, int dy)
{
	const float SENSF = 33.0f;	 // try match quake sens

	if (iscamera())
	{
		player1->yaw += (dx/SENSF)*(sensitivity/(float)sensitivityscale);
		player1->pitch -= (dy/SENSF)*(sensitivity/(float)sensitivityscale)*(invmouse ? -1 : 1);
	
		fixcamerarange((physent *)player1);
	}
	else if (menuactive())
	{
		camera->yaw += (dx/SENSF)*(sensitivity/(float)sensitivityscale);
		camera->pitch -= (dy/SENSF)*(sensitivity/(float)sensitivityscale)*(invmouse ? -1 : 1);
	
		fixcamerarange((physent *)camera);
	}
}

void recomputecamera()
{
	if (!iscamera())
	{
		camera1 = camera;
		fov = getvar("camerafov"); // force our fov

		if (endtime <= 0 && (getvar("cuttime") <= 0 || !getvar("cuthold")))
		{
			camera->o = player1->o;
			camera->o.z -= player1->eyeheight;
			
			if (isxaxis(player1->direction))
			{
				renderinc(camera->o.x, player1->renderdepth, 250, player1->renderpath, getworldsize(), false);
			}
			else
			{
				renderinc(camera->o.y, player1->renderdepth, 250, player1->renderpath, getworldsize(), false);
			}

			camera->pos = camera->o;

			camera->o.x -= getvar("cameradist");
			camera->o.y += getvar("cameradoff")+(player1->vel.y*(getvar("cameradvel")*0.01f));
			camera->o.z += getvar("camerazoff")+(player1->vel.z*(getvar("camerazvel")*0.01f));

			camera->eyeheight = 1;

			// keep camera in the level, for aesthetic reasons really
			#define keepinworld(x,y) \
				if (x < y) { x = y; } \
				else if (x > getworldsize()-y) { x = getworldsize()-y; }
			
			//keepinworld(camera->o.x, -getvar("cameradist"));
			keepinworld(camera->o.y, getvar("clipdsize"));
			keepinworld(camera->o.z, getvar("clipzsize"));

			if (lastmillis-maptime <= CARDTIME)
			{
				float fade = (float(lastmillis-maptime)/float(CARDTIME));
				
				vec v(camera->pos), w(camera->pos);
				v.sub(camera1->o);
				v.mul(fade);
				w.sub(v);
				camera1->o = w;
			}

			if (!menuactive())
			{
				if (getvar("camerarot"))
				{
					vec v(camera->pos);
					keepinworld(v.y, getvar("clipdsize"));
					keepinworld(v.z, getvar("clipzsize"));
					v.sub(camera->o);
					v.normalize();
					vectoyawpitch(v, camera->yaw, camera->pitch);
				}
				else
				{
					camera->yaw = 90.f;
					camera->pitch = 0.f;
				}
			}
		}
	}
	else camera1 = player1;
		
	fixcamerarange(camera1);
}

void rendersspact(sspent *d, const char *mdl, const char *vwep, const char *shield, const char *pup, int attack, int reload)
{
	float yaw = d->yaw, pitch = d->pitch;
	float depth = d->path;

	renderinc(d->yaw, d->renderyaw, 250, d->renderdir, 360.f, true);

	if (isxaxis(d->direction))
	{
		renderinc(d->o.x, d->renderdepth, 250, d->renderpath, getworldsize(), false);
	}
	else
	{
		renderinc(d->o.y, d->renderdepth, 250, d->renderpath, getworldsize(), false);
	}

	modelattach a[4] = { { NULL }, { NULL }, { NULL }, { NULL } };

	int ai = 0;
	if(vwep)
	{
		a[ai].name = vwep;
		a[ai].type = MDL_ATTACH_VWEP;
		a[ai].anim = ANIM_VWEP|ANIM_LOOP;
		a[ai].basetime = 0;
		ai++;
	}
	if(pup)
	{
		a[ai].name = pup;
		a[ai].type = MDL_ATTACH_POWERUP;
		a[ai].anim = ANIM_POWERUP|ANIM_LOOP;
		a[ai].basetime = 0;
		ai++;
	}
	if(shield)
	{
		a[ai].name = shield;
		a[ai].type = MDL_ATTACH_SHIELD;
		a[ai].anim = ANIM_SHIELD|ANIM_LOOP;
		a[ai].basetime = 0;
		ai++;
	}

    renderclient(d, mdl, a[0].name ? a : NULL, attack, reload, d->lastaction, d->lastpain);

	d->yaw = yaw;
	d->pitch = pitch;
	
	if (isxaxis(d->direction)) d->o.x = depth;
	else d->o.y = depth;
}

void renderplayer()
{
	int pain = lastmillis-player1->lastpain;
	
	if (!iscamera() && (player1->state == CS_DEAD || pain > getvar("invulnerable") || pain%500 < 300))
	{
		int strafe = player1->strafe, move = player1->move, idle = player1->idle - getvar("idletime");
		float yaw = player1->yaw, pitch = player1->pitch;
		
		if (!isfinished() && getvar("cuttime") <= 0)
		{	
			if (isxaxis(player1->direction))
			{
				player1->move = (strafe ? 1 : 0);
			}
			else
			{
				player1->move = (move ? 1 : 0);
			}
			player1->strafe = 0;

			if (idle > 0)
			{
				vectoyawpitch(vec(camera1->o).sub(player1->o), player1->yaw, player1->pitch);
				
				if (idle < 1000)
				{
					player1->yaw = yaw+((player1->yaw-yaw)*(float(idle)/1000.f));
					player1->pitch = pitch+((player1->pitch-pitch)*(float(idle)/1000.f));
				}
			}
		}

		#define hasgun(a,b) (player1->sd.gun < 0 ? b : a)
		rendersspact(
			player1,
			powers[player1->getpower()],
			hasgun(vweps[player1->sd.gun], NULL),
			shields[player1->sd.shield],
			player1->invincible ? "quadspheres" : NULL,
			isfinished() ? ANIM_WIN|ANIM_LOOP : (
				idle > 1000 ? ANIM_TAUNT|ANIM_LOOP : hasgun(vanim[player1->sd.gun], 0)
			),
			isfinished() || idle > 1000 ? 1000 : hasgun(wp.reloadtime(player1->sd.gun) + 10, 0)
		);

		player1->move = move;
		player1->strafe = strafe;
		player1->yaw = yaw;
		player1->pitch = pitch;
	}
}

void lighteffects(dynent *e, vec &color, vec &dir)
{
	sspent *d = (sspent *)e;

	if ((d->type == ENT_PLAYER || d->type == ENT_AI) && d->state == CS_ALIVE)
	{
		int offset = (lastmillis%1000)+1;
	
		if (d->invincible)
		{
			offset = (d->invincible%1000)+1;
			if (offset > 500) offset = 1000 - offset;
			color.x = max(color.x, 1.f-(float(offset)*0.001f));
		}
	}
}

void adddynlights()
{
	loopi(numdynents())
	{
		sspent *d = (sspent *)iterdynents(i);
		
		if (d && d->state == CS_ALIVE)
		{
			int illum = 0;
			vec color(0, 0, 0);

			if ((d->type == ENT_PLAYER || d->type == ENT_AI) && d->invincible)
			{
				int offset = (d->invincible%1000)+1;
				float scale = (float(d->invincible)/float(getvar("invincible")))+0.1f;
				if (offset > 500) offset = 1000 - offset;
				illum = int((ILLUMINATE-(ILLUMINATE*(offset*0.001f)))*scale);
				color = vec(1, 0, 0);
			}
		
			if (illum)
			{
				adddynlight(vec(d->o).sub(vec(0, 0, d->eyeheight/2)), illum, color, 0, 0);
			}
		}
	}
	
	pr.dynlightprojs();
}

void rendergame()
{
    startmodelbatches();

	renderplayer();
	bl.renderblocks();
	en.renderenemies();
	et.renderentities();
	pr.renderprojectiles();

    endmodelbatches();
}

void playerprerender()
{
	show_out_of_renderloop_progress(0.f, "pre-rendering.. player");
	loopi(MAXPOWER+PW_MAX)
	{
		show_out_of_renderloop_progress(float(i)/float(MAXPOWER+PW_MAX), "pre-rendering.. player (models)");
		//renderclient(player1, powers[i], "vwep/chaing", NULL, "quadspheres", 0, 0, 0, 0);
	}
	loopi(GN_MAX)
	{
		show_out_of_renderloop_progress(float(i)/float(GN_MAX), "pre-rendering.. player (vweps)");
		//renderclient(player1, powers[2], vweps[i], NULL, "quadspheres", 0, 0, 0, 0);
	}
	show_out_of_renderloop_progress(1.f, "pre-rendering.. player done.");
}

void prerender()
{
	//playerprerender();
	et.entprerender();
	pr.projprerender();
}

void drawhudgun() {}

void gameplayhud(int w, int h)
{
	int ox = w*900/h, oy = 900;

	glLoadIdentity();
	glOrtho(0, ox, oy, 0, -1, 1);

	glEnable(GL_BLEND);

	if (!titlecard(ox, oy, lastmillis-maptime))
	{
		if (!hidehud && !iscamera() && !menuactive())
		{
			float fade = 1.f, amt = hudblend*0.01f;

			if (endtime > 0) fade = 1.f-(float(lastmillis-endtime)/float(CARDTIME+CARDFADE));
			else if (lastmillis-maptime <= CARDTIME+CARDFADE+CARDFADE) fade = amt*(float(lastmillis-maptime-CARDTIME-CARDFADE)/float(CARDFADE));
			else fade *= amt;
	
			glColor4f(1.f, 1.f, 1.f, endtime > 0 ? fade : amt);
			rendericon("packages/icons/sauer.jpg", 20, oy-75, 64, 64);

			if (maptext[0])
			{
				draw_textx("%s", 100, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT, maptext);
			}
			else
			{
				if (player1->sd.lives > 0)
				{
					glColor4f(1.f, 1.f, 1.f, fade);
					rendericon(icons[player1->getpower()], 100, oy-75, 64, 64);
					draw_textx("x %d", 180, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT, player1->sd.lives);
		
					glColor4f(1.f, 1.f, 1.f, fade);
					rendericon("packages/icons/coins.jpg", 350, oy-75, 64, 64);
					draw_textx("x %d", 430, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT, player1->sd.coins);
					
					glColor4f(1.f, 1.f, 1.f, fade);
					
					if (endtime <= 0 && getvar("timelimit") > 0)
					{
						if (isfinished())
						{
							rendericon("packages/icons/flag_red.jpg", 600, oy-75, 64, 64);
							draw_textx("Level Done!", 680, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT);
						}
						else
						{
							if (et.ents.inrange(player1->sd.chkpoint) && et.ents[player1->sd.chkpoint]->type == SE_GOAL && et.ents[player1->sd.chkpoint]->attr1 == 0 && et.ents[player1->sd.chkpoint]->attr2 > 0)
								rendericon("packages/icons/flag_blue.jpg", 600, oy-75, 64, 64);
							else
								rendericon("packages/icons/flag_neutral.jpg", 600, oy-75, 64, 64);
		
							if (leveltime >= getvar("timelimit"))
								draw_textx("Time Up!", 680, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT);
							else if (player1->state == CS_DEAD)
								draw_textx("You Died!", 680, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT);
							else
								draw_textx("%d", 680, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT, (getvar("timelimit")-leveltime)/1000);
						}
					}
					
					glColor4f(1.f, 1.f, 1.f, fade);
					rendericon("packages/icons/blank.jpg", ox-80, oy-75, 64, 64);
		
					if (player1->sd.inventory >= PW_HIT)
					{
						glColor4f(1.0f, 1.0f, 1.0f, fade*0.75f);
						rendericon(picons[player1->sd.inventory], ox-80, oy-75, 64, 64);
					}
				}
				else
					draw_textx("GAME OVER!", 100, oy-75, 255, 255, 255, int(255.f*fade), AL_LEFT);
			}
		}
	}
	glDisable(GL_BLEND);
}

void g3d_gamemenus() {}
bool wantcrosshair() { return (iscamera() && !hidehud) || menuactive(); }
bool gamethirdperson() { return false; }

bool gethudcolour(vec &colour)
{
	if (lastmillis-maptime <= CARDTIME)
	{
		float fade = (float(lastmillis-maptime)/float(CARDTIME));

		colour = vec(fade, fade, fade);

		return true;
	}
	else if (endtime > 0 && lastmillis-endtime <= CARDTIME)
	{
		float fade = 1.f-(float(lastmillis-endtime)/float(CARDTIME));

		colour = vec(fade, fade, fade);

		return true;
	}
	return false;
}
