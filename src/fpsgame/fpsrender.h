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
			lastaction = lastmillis;
			attack = ANIM_LOSE|ANIM_LOOP;
			delay = 1000;
			if(m_team(cl.gamemode, cl.mutators)) loopv(bestteams) { if(!strcmp(bestteams[i], d->team)) { attack = ANIM_WIN|ANIM_LOOP; break; } }
			else if(bestplayers.find(d)>=0) attack = ANIM_WIN|ANIM_LOOP;
		}
        else if (d->state == CS_ALIVE && d->lasttaunt && lastmillis-d->lasttaunt<1000 && lastmillis-lastaction>delay)
		{
			lastaction = d->lasttaunt;
			attack = ANIM_TAUNT;
			delay = 1000;
		}
		else if (isgun(d->gunselect) && lastmillis-d->gunlast[d->gunselect] < d->gunwait[d->gunselect])
		{
			if(d->gunstate[d->gunselect] == GUNSTATE_RELOAD)
				attack = ANIM_RELOAD;
		}
        modelattach a[4] = { { NULL }, { NULL }, { NULL }, { NULL } };
		static const char *vweps[] = { "weapons/pistol/vwep", "weapons/shotgun/vwep", "weapons/chaingun/vwep", "weapons/grenades/vwep", "weapons/rockets/vwep", "weapons/rifle/vwep"};
        int ai = 0;
        if (d->gunselect > -1 && d->gunselect < NUMGUNS)
		{
            a[ai].name = vweps[d->gunselect];
            a[ai].tag = "tag_weapon";
            a[ai].anim = ANIM_VWEP|ANIM_LOOP;
            a[ai].basetime = 0;
            ai++;
		}
        renderclient(d, local, mdlname, a[0].name ? a : NULL, attack, delay, lastaction, cl.intermission ? 0 : d->lastpain);
	}

	void render()
	{
		if(cl.intermission)
		{
			if(m_team(cl.gamemode, cl.mutators)) { bestteams.setsize(0); cl.sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); cl.sb.bestplayers(bestplayers); }
		}

		startmodelbatches();

        const char *mdlnames[3] = { "player", "player/blue", "player/red" };

		fpsent *d;
        loopv(cl.players) if((d = cl.players[i]) && d->state!=CS_SPECTATOR && d->state!=CS_SPAWNING)
		{
            int mdl = m_team(cl.gamemode, cl.mutators) ? (isteam(cl.player1->team, d->team) ? 1 : 2) : 0;
			if(d->state!=CS_DEAD || d->superdamage<50) renderplayer(d, false, mdlnames[mdl]);
			if(d->state!=CS_DEAD)
			{
				if (m_team(cl.gamemode, cl.mutators)) s_sprintf(d->info)("%s (%s)", cl.colorname(d, NULL, "@"), d->team);
				else s_sprintf(d->info)("%s", cl.colorname(d, NULL, "@"));
				particle_text(d->abovehead(), d->info, m_team(cl.gamemode, cl.mutators) ? (isteam(cl.player1->team, d->team) ? 16 : 13) : 11, 1);
			}
		}
		if(cl.player1->state == CS_ALIVE || cl.player1->state == CS_DEAD)
			renderplayer(cl.player1, true, m_team(cl.gamemode, cl.mutators) ? mdlnames[1] : mdlnames[0]);

		cl.et.render();
		cl.pj.render();
		if(m_capture(cl.gamemode)) cl.cpc.render();
        else if(m_ctf(cl.gamemode)) cl.ctf.render();

        cl.bot.render();

		endmodelbatches();

		if(rendernormally)
		{
			renderprimitive(true);
			vec v = cl.ws.gunorigin(cl.player1->gunselect, cl.player1->o, worldpos, cl.player1);
			renderline(v, worldpos, 0.2f, 0.0f, 0.0f, false);
			renderprimitive(false);
		}
	}

    void preload()
    {
        loadmodel("player", -1, true);
        loadmodel("player/blue", -1, true);
        loadmodel("player/red", -1, true);
    }
} fr;
