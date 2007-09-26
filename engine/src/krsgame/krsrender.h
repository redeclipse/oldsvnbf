// Blood Frontier - KRSGAME - Kart Racing Simulator by Quinton Reeves
// This is the KRS rendering processes.

void fixcamerarange(physent *d, bool limitpitch)
{
	float pitchamt = limitpitch ? 90.0f : 360.f;
	if(d->pitch>pitchamt) d->pitch = pitchamt;
	if(d->pitch<-pitchamt) d->pitch = -pitchamt;
	while(d->yaw<0.0f) d->yaw += 360.0f;
	while(d->yaw>=360.0f) d->yaw -= 360.0f;
}

void fixcamerarange()
{
	physent *d = isthirdperson() ? camera1 : player1;
	fixcamerarange(d, player1->state == CS_EDITING || player1->state == CS_SPECTATOR);
}

void mousemove(int dx, int dy)
{
	const float SENSF = 33.0f;	 // try match quake sens
	physent *d = isthirdperson() ? camera1 : player1;
	
	d->yaw += (dx/SENSF)*(sensitivity/(float)sensitivityscale);
	d->pitch -= (dy/SENSF)*(sensitivity/(float)sensitivityscale)*(invmouse ? -1 : 1);
}

void recomputecamera()
{
	if (player1->state == CS_EDITING || player1->state == CS_SPECTATOR)
	{
		camera1 = player1;
		fixcamerarange(camera1, true);
	}
	else
	{
		camera1 = camera;

		camera1->o = player1->o;
		camera1->o.z -= player1->eyeheight*0.5f;
		
		if (isthirdperson())
		{
			vec old(camera1->o);

			camera1->move = -1;
			camera1->o.z += thirdpersonheight;
			
			fixcamerarange(camera1, false);
			
			loopi(10)
			{
				if(!moveplayer(camera1, 10, false, thirdpersondistance)) break;
			}
		}
		else
		{
			camera1->yaw = player1->yaw;
			camera1->pitch = player1->pitch;
		}
		fixcamerarange(camera1, false);
	}
}

void renderplayer(krsent *d, const char *mdlname, const char *vwepname, const char *shieldname, const char *pupname, int attack, int attackdelay, int lastaction, int lastpain, float sink)
{
    int anim = ANIM_IDLE|ANIM_LOOP;
    float yaw = d->yaw, pitch = d->pitch, roll = d->roll;
    if(d->type==ENT_PLAYER && d!=player && orientinterpolationtime)
    {
        if(d->orientmillis!=lastmillis)
        {
            if(yaw-d->lastyaw>=180) yaw -= 360;
            else if(d->lastyaw-yaw>=180) yaw += 360;
            d->lastyaw += (yaw-d->lastyaw)*min(orientinterpolationtime, lastmillis-d->orientmillis)/float(orientinterpolationtime);
            d->lastpitch += (pitch-d->lastpitch)*min(orientinterpolationtime, lastmillis-d->orientmillis)/float(orientinterpolationtime);
            d->orientmillis = lastmillis;
        }
        yaw = d->lastyaw;
        pitch = d->lastpitch;
    }
    vec o(vec(d->o).sub(vec(0, 0, d->eyeheight)));
	if (iskart(d) && !d->timeinair && (!d->inwater || d->floor.z < ph.floorz(d)))
	{
		float q = 0.f;
		loopi(WL_MAX) q += d->wheels[1][i].x;
		o.x = q/float(WL_MAX);
	}
    o.z -= sink;
    int varseed = (int)(size_t)d, basetime = 0;
    if(animoverride) anim = animoverride|ANIM_LOOP;
    else if(d->state==CS_DEAD)
    {
        pitch = roll = 0;
        anim = ANIM_DYING;
        basetime = lastaction;
        varseed += lastaction;
        int t = lastmillis-lastaction;
        if(t<0 || t>20000) return;
        if(t>500) { anim = ANIM_DEAD|ANIM_LOOP; if(t>1600) { t -= 1600; o.z -= t*t/10000000000.0f*t/16.0f; } }
        if(o.z<-1000) return;
    }
    else if(d->state==CS_EDITING || d->state==CS_SPECTATOR) anim = ANIM_EDIT|ANIM_LOOP;
    else if(d->state==CS_LAGGED)                            anim = ANIM_LAG|ANIM_LOOP;
    else
    {
        if(lastmillis-lastpain<300) 
        { 
            anim = ANIM_PAIN;
            basetime = lastpain;
            varseed += lastpain; 
        }
        else if(attack<0 || (d->type!=ENT_AI && lastmillis-lastaction<attackdelay)) 
        { 
            anim = attack<0 ? -attack : attack; 
            basetime = lastaction; 
            varseed += lastaction;
        }

        if(d->strafe) anim |= ((d->strafe>0 ? ANIM_LEFT : ANIM_RIGHT))<<ANIM_SECONDARY;

        if(d->inwater && d->physstate<=PHYS_FALL) anim |= ((d->move || d->strafe || d->vel.z+d->gravity.z>0 ? ANIM_SWIM : ANIM_SINK)|ANIM_LOOP)<<ANIM_SECONDARY;
        else if(d->timeinair>100) anim |= (ANIM_JUMP|ANIM_END)<<ANIM_SECONDARY;
        else if(d->move>0) anim |= (ANIM_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
        else if(d->move<0) anim |= (ANIM_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;

        if((anim&ANIM_INDEX)==ANIM_IDLE && (anim>>ANIM_SECONDARY)&ANIM_INDEX) anim >>= ANIM_SECONDARY;
    }
    if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX)) anim |= (ANIM_IDLE|ANIM_LOOP)<<ANIM_SECONDARY;
    int flags = MDL_CULL_VFC | MDL_CULL_OCCLUDED;
    if(d->type!=ENT_PLAYER) flags |= MDL_CULL_DIST;
    if((anim&ANIM_INDEX)!=ANIM_DEAD) flags |= MDL_SHADOW;
    modelattach a[4] = { { NULL }, { NULL }, { NULL } };
    int ai = 0;
    if(vwepname)
    {
        a[ai].name = vwepname;
        a[ai].type = MDL_ATTACH_VWEP;
        a[ai].anim = ANIM_VWEP|ANIM_LOOP;
        a[ai].basetime = 0;
        ai++;
    }
    if(shieldname)
    {
        a[ai].name = shieldname;
        a[ai].type = MDL_ATTACH_SHIELD;
        a[ai].anim = ANIM_SHIELD|ANIM_LOOP;
        a[ai].basetime = 0;
        ai++;
    }
    if(pupname)
    {
        a[ai].name = pupname;
        a[ai].type = MDL_ATTACH_POWERUP;
        a[ai].anim = ANIM_POWERUP|ANIM_LOOP;
        a[ai].basetime = 0;
        ai++;
    }
    vec color, dir;
    rendermodel(color, dir, mdlname, anim, varseed, 0, o, testanims && d==player ? 0 : yaw+90, pitch, 0, basetime, d, flags, a[0].name ? a : NULL);
}

void rendergame()
{
	startmodelbatches();
	if (player1->state != CS_EDITING && player1->state != CS_SPECTATOR)
	{
		renderplayer(player1, kartmdl[0], NULL, NULL, NULL, 0, 0, 0, 0, 0.f);
		
		if (showdebug())
		{
			float col[WL_MAX][3] = {
				{ 1.f, 0.f, 0.f },
				{ 0.f, 1.f, 0.f },
				{ 0.f, 0.f, 1.f },
				{ 1.f, 0.f, 1.f }
			};

			renderprimitive(true);
			loopi(WL_MAX)
			{
				renderline(player1->wheels[0][i], player1->wheels[1][i], col[i][0], col[i][1], col[i][2]);
			}
			renderline(player1->wheels[1][WL_FRONT], player1->wheels[1][WL_REAR], 0.5f, 0.5f, 0.5f);
			renderline(player1->wheels[1][WL_LEFT], player1->wheels[1][WL_RIGHT], 0.5f, 0.5f, 0.5f);
			renderprimitive(false);
		}
	}
	endmodelbatches();
}


void drawhudgun() {}

void gameplayhud(int w, int h)
{
	int ox = w*900/h, oy = 900;

	glLoadIdentity();
	glOrtho(0, ox, oy, 0, -1, 1);
	
	glEnable(GL_BLEND);

	extern int hudblend;
	float fade = 1.f, amt = hudblend*0.01f;

	fade *= amt;

	glColor4f(1.f, 1.f, 1.f, amt);
	rendericon("icons/sauer.jpg", 20, oy-75, 64, 64);

	if (showdebug())
	{
		ox = w*1800/h;
		oy = 1800;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);

		draw_textx("%4.2f, %4.2f, %4.2f POS", ox-16, oy-75, 255, 255, 255, int(255.f*fade), AL_RIGHT, player1->o.x, player1->o.y, player1->o.z);
		draw_textx("%4.2f, %4.2f, %4.2f VEL", ox-16, oy-145, 255, 255, 255, int(255.f*fade), AL_RIGHT, player1->vel.x, player1->vel.y, player1->vel.z);
		draw_textx("%4.2f, %4.2f, %4.2f GRV", ox-16, oy-215, 255, 255, 255, int(255.f*fade), AL_RIGHT, player1->gravity.x, player1->gravity.y, player1->gravity.z);
		draw_textx("%4.2f:%4.2f SPD", ox-16, oy-285, 255, 255, 255, int(255.f*fade), AL_RIGHT, ph.speed(player1), vec(player1->vel.x, player1->vel.y, 0).magnitude());
		draw_textx("%4.2f:%4.2f / %4.2f:%4.2f ROT", ox-16, oy-355, 255, 255, 255, int(255.f*fade), AL_RIGHT, player1->pitch, player1->targpitch, player1->roll, player1->targroll);
	}
	glDisable(GL_BLEND);
}

void g3d_gamemenus() {}
bool wantcrosshair() { return (!hidehud && !isthirdperson()) || menuactive(); }
bool gamethirdperson() { return player1->state != CS_EDITING && player1->state != CS_SPECTATOR; }

bool gethudcolour(vec &colour)
{
	return false;
}
