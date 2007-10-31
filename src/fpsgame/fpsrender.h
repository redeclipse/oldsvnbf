
struct fpsrender
{	  
	fpsclient &cl;

	fpsrender(fpsclient &_cl) : cl(_cl) {}

	vector<fpsent *> bestplayers;
    vector<const char *> bestteams;

#ifndef BFRONTIER
    IVARP(ogro, 0, 0, 1);
#endif

	void renderplayer(fpsent *d, const char *mdlname)
	{
#ifdef BFRONTIER
        int lastaction = d->gunlast[d->gunselect],
			attack = ANIM_SHOOT,
			delay = d->gunwait[d->gunselect] + 50;
		if(cl.intermission && d->state!=CS_DEAD)
		{
			lastaction = cl.lastmillis;
			attack = ANIM_LOSE|ANIM_LOOP;
			delay = 1000;
			if(m_team(cl.gamemode, cl.mutators)) loopv(bestteams) { if(!strcmp(bestteams[i], d->team)) { attack = ANIM_WIN|ANIM_LOOP; break; } }
			else if(bestplayers.find(d)>=0) attack = ANIM_WIN|ANIM_LOOP;
		}
		else if (d->state == CS_ALIVE && cl.lastmillis-d->lasttaunt < 1000 && cl.lastmillis-lastaction>delay)
		{
			lastaction = d->lasttaunt;
			attack = ANIM_TAUNT;
			delay = 1000;
		}
        modelattach a[4] = { { NULL }, { NULL }, { NULL }, { NULL } };
		static const char *vweps[] = { "vwep/pistol", "vwep/shotgun", "vwep/chaingun", "vwep/grenades", "vwep/rockets", "vwep/rifle"};
        int ai = 0;
        if (d->gunselect<=GUN_RIFLE)
		{
            a[ai].name = vweps[d->gunselect];
            a[ai].type = MDL_ATTACH_VWEP;
            a[ai].anim = ANIM_VWEP|ANIM_LOOP;
            a[ai].basetime = 0;
            ai++;
		}
        renderclient(d, mdlname, a[0].name ? a : NULL, attack, delay, lastaction, cl.intermission ? 0 : d->lastpain);
#else
        int lastaction = d->lastaction, attack = d->gunselect==GUN_FIST ? ANIM_PUNCH : ANIM_SHOOT, delay = ogro() ? 300 : cl.ws.reloadtime(d->gunselect)+50;
		if(cl.intermission && d->state!=CS_DEAD)
		{
			lastaction = cl.lastmillis;
			attack = ANIM_LOSE|ANIM_LOOP;
			delay = 1000;
			int gamemode = cl.gamemode;
			if(m_teammode) loopv(bestteams) { if(!strcmp(bestteams[i], d->team)) { attack = ANIM_WIN|ANIM_LOOP; break; } }
			else if(bestplayers.find(d)>=0) attack = ANIM_WIN|ANIM_LOOP;
		}
		else if(d->state==CS_ALIVE && cl.lastmillis-d->lasttaunt<1000 && cl.lastmillis-d->lastaction>delay)
		{
			lastaction = d->lasttaunt;
			attack = ANIM_TAUNT;
			delay = 1000;
		}
        modelattach a[4] = { { NULL }, { NULL }, { NULL }, { NULL } };
        static const char *vweps[] = {"vwep/fist", "vwep/shotg", "vwep/chaing", "vwep/rocket", "vwep/rifle", "vwep/gl", "vwep/pistol"};
        int ai = 0;
        if((!ogro() || d->gunselect!=GUN_FIST) && d->gunselect<=GUN_PISTOL)
		{
            a[ai].name = ogro() ? "monster/ogro/vwep" : vweps[d->gunselect];
            a[ai].type = MDL_ATTACH_VWEP;
            a[ai].anim = ANIM_VWEP|ANIM_LOOP;
            a[ai].basetime = 0;
            ai++;
		}
        if(!ogro() && d->state==CS_ALIVE)
        {
            if(d->quadmillis)
            {
                a[ai].name = "quadspheres";
                a[ai].type = MDL_ATTACH_POWERUP;
                a[ai].anim = ANIM_POWERUP|ANIM_LOOP;
                a[ai].basetime = 0;
                ai++;
            }
            if(d->armour)
            {
                a[ai].name = d->armourtype==A_GREEN ? "shield/green" : (d->armourtype==A_YELLOW ? "shield/yellow" : NULL);
                a[ai].type = MDL_ATTACH_SHIELD;
                a[ai].anim = ANIM_SHIELD|ANIM_LOOP;
                a[ai].basetime = 0;
                ai++;
            }
        }
        renderclient(d, mdlname, a[0].name ? a : NULL, attack, delay, lastaction, cl.intermission ? 0 : d->lastpain);
#if 0
		if(d->state!=CS_DEAD && d->quadmillis) 
		{
			vec color(1, 1, 1), dir(0, 0, 1);
			rendermodel(color, dir, "quadrings", ANIM_MAPMODEL|ANIM_LOOP, 0, 0, vec(d->o).sub(vec(0, 0, d->eyeheight/2)), 360*cl.lastmillis/1000.0f, 0, 0, 0, NULL, MDL_DYNSHADOW | MDL_CULL_VFC | MDL_CULL_DIST);
		}
#endif
#endif
	}

	IVARP(teamskins, 0, 0, 1);

#ifdef BFRONTIER
	void rendergame()
	{
		if(cl.intermission)
		{
			if(m_team(cl.gamemode, cl.mutators)) { bestteams.setsize(0); cl.sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); cl.sb.bestplayers(bestplayers); }
		}

		startmodelbatches();

        const char *ffamdl = "player", *bluemdl = "player/blue", *redmdl = "player/red";

		fpsent *d;
        loopv(cl.players) if((d = cl.players[i]) && d->state!=CS_SPECTATOR && d->state!=CS_SPAWNING)
		{
			if (cl.player1->state == CS_SPECTATOR && cl.players[i]->clientnum == -cl.cameranum && !isthirdperson()) continue;
            const char *mdlname = teamskins() || m_team(cl.gamemode, cl.mutators) ? (isteam(cl.player1->team, d->team) ? bluemdl : redmdl) : ffamdl;
			if(d->state!=CS_DEAD || d->superdamage<50) renderplayer(d, mdlname);
			s_strcpy(d->info, cl.colorname(d, NULL, "@"));
			if(d->state!=CS_DEAD) particle_text(d->abovehead(), d->info, m_team(cl.gamemode, cl.mutators) ? (isteam(cl.player1->team, d->team) ? 16 : 13) : 11, 1);
		}
		if(isthirdperson() && (cl.player1->state != CS_SPECTATOR || cl.player1->clientnum == -cl.cameranum)) renderplayer(cl.player1, teamskins() || m_team(cl.gamemode, cl.mutators) ? bluemdl : ffamdl);

		cl.et.renderentities();
		cl.ws.renderbouncers();
		if(m_capture(cl.gamemode)) cl.cpc.renderbases();

		endmodelbatches();
	}
#else
	void rendergame(int gamemode)
	{
		if(cl.intermission)
		{
			if(m_teammode) { bestteams.setsize(0); cl.sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); cl.sb.bestplayers(bestplayers); }
		}

		startmodelbatches();

        const char *ffamdl = ogro() ? "monster/ogro" : "ironsnout",
                   *bluemdl = ogro() ? "monster/ogro/blue" : "ironsnout/blue",
                   *redmdl = ogro() ? "monster/ogro/red" : "ironsnout/red";

		fpsent *d;
        loopv(cl.players) if((d = cl.players[i]) && d->state!=CS_SPECTATOR && d->state!=CS_SPAWNING)
		{
            const char *mdlname = teamskins() || m_teammode ? (isteam(cl.player1->team, d->team) ? bluemdl : redmdl) : ffamdl;
			if(d->state!=CS_DEAD || d->superdamage<50) renderplayer(d, mdlname);
			s_strcpy(d->info, cl.colorname(d, NULL, "@"));
			if(d->maxhealth>100) { s_sprintfd(sn)(" +%d", d->maxhealth-100); s_strcat(d->info, sn); }
			if(d->state!=CS_DEAD) particle_text(d->abovehead(), d->info, m_teammode ? (isteam(cl.player1->team, d->team) ? 16 : 13) : 11, 1);
		}
        if(isthirdperson()) renderplayer(cl.player1, teamskins() || m_teammode ? bluemdl : ffamdl);

		cl.ms.monsterrender();
		cl.et.renderentities();
		cl.ws.renderprojectiles();
		if(m_capture) cl.cpc.renderbases();

		endmodelbatches();
	}
#endif
};
