struct fpsrender
{
	GAMECLIENT &cl;

	fpsrender(GAMECLIENT &_cl) : cl(_cl) {}

	vector<fpsent *> bestplayers;
    vector<const char *> bestteams;

	void renderplayer(fpsent *d, bool local, const char *mdlname)
	{
        int lastaction = !isgun(d->gunselect) ? 0 : d->gunlast[d->gunselect], attack = ANIM_SHOOT, delay = !isgun(d->gunselect) ? 0 : d->gunwait[d->gunselect] + 50;

		if(cl.intermission && d->state != CS_DEAD)
		{
			lastaction = cl.lastmillis;
			attack = ANIM_LOSE|ANIM_LOOP;
			delay = 1000;
			if(m_team(cl.gamemode, cl.mutators)) loopv(bestteams) { if(!strcmp(bestteams[i], d->team)) { attack = ANIM_WIN|ANIM_LOOP; break; } }
			else if(bestplayers.find(d)>=0) attack = ANIM_WIN|ANIM_LOOP;
		}
        else if (d->state == CS_ALIVE && d->lasttaunt && cl.lastmillis-d->lasttaunt<1000 && cl.lastmillis-lastaction>delay)
		{
			lastaction = d->lasttaunt;
			attack = ANIM_TAUNT;
			delay = 1000;
		}
        modelattach a[4] = { { NULL }, { NULL }, { NULL }, { NULL } };
		static const char *vweps[] = { "vwep/pistol", "vwep/shotgun", "vwep/chaingun", "vwep/grenades", "vwep/rockets", "vwep/rifle"};
        int ai = 0;
        if (d->gunselect > -1 && d->gunselect < NUMGUNS)
		{
            a[ai].name = vweps[d->gunselect];
            a[ai].type = MDL_ATTACH_VWEP;
            a[ai].anim = ANIM_VWEP|ANIM_LOOP;
            a[ai].basetime = 0;
            ai++;
		}
        renderclient(d, local, mdlname, a[0].name ? a : NULL, attack, delay, lastaction, cl.intermission ? 0 : d->lastpain);
	}

	IVARP(teamskins, 0, 0, 1);

	void rendergame()
	{
		if(cl.intermission)
		{
			if(m_team(cl.gamemode, cl.mutators)) { bestteams.setsize(0); cl.sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); cl.sb.bestplayers(bestplayers); }
		}

		startmodelbatches();

        const char *mdlnames[3] = 
        { 
            "player", "player/blue", "player/red"
        };

		fpsent *d;
        loopv(cl.players) if((d = cl.players[i]) && d->state!=CS_SPECTATOR && d->state!=CS_SPAWNING)
		{
			if (cl.player1->state == CS_SPECTATOR && cl.players[i]->clientnum == -cl.cameranum && !isthirdperson()) continue;
            int mdl = teamskins() || m_team(cl.gamemode, cl.mutators) ? (isteam(cl.player1->team, d->team) ? 1 : 2) : 0;
			if(d->state!=CS_DEAD || d->superdamage<50) renderplayer(d, false, mdlnames[mdl]);
			s_strcpy(d->info, cl.colorname(d, NULL, "@"));
			if(d->state!=CS_DEAD) particle_text(d->abovehead(), d->info, m_team(cl.gamemode, cl.mutators) ? (isteam(cl.player1->team, d->team) ? 16 : 13) : 11, 1);
		}
		if(isthirdperson() && (cl.player1->state != CS_SPECTATOR || cl.player1->clientnum == -cl.cameranum)) renderplayer(cl.player1, true, teamskins() || m_team(cl.gamemode, cl.mutators) ? mdlnames[1] : mdlnames[0]);

		cl.et.renderentities();
		cl.pj.render();
		if(m_capture(cl.gamemode)) cl.cpc.renderbases();

		endmodelbatches();
	}
} fr;
