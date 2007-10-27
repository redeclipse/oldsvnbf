// creation of scoreboard

struct scoreboard : g3d_callback
{
	bool scoreson;
	vec menupos;
	int menustart;
	fpsclient &cl;

#ifdef BFRONTIER
	IVARP(scoresinfo, 0, 1, 1);
#endif
	IVARP(showclientnum, 0, 0, 1);
    IVARP(showpj, 0, 1, 1);
    IVARP(showping, 0, 1, 1);
    IVARP(showspectators, 0, 1, 1);
    IVARP(highlightscore, 0, 1, 1);
    IVARP(showconnecting, 0, 0, 1);

	scoreboard(fpsclient &_cl) : scoreson(false), cl(_cl)
	{
        CCOMMAND(showscores, "D", (scoreboard *self, int *down), self->showscores(*down!=0));
	}

	void showscores(bool on)
	{
		if(!scoreson && on)
		{
			menupos = menuinfrontofplayer();
			menustart = starttime();
		}
		scoreson = on;
	}

	struct sline { string s; };

	struct teamscore
	{
        const char *team;
		int score;
		teamscore() {}
		teamscore(char *s, int n) : team(s), score(n) {}
	};

	static int teamscorecmp(const teamscore *x, const teamscore *y)
	{
		if(x->score > y->score) return -1;
		if(x->score < y->score) return 1;
		return strcmp(x->team, y->team);
	}
	
	static int playersort(const fpsent **a, const fpsent **b)
	{
		if((*a)->state==CS_SPECTATOR)
		{
			if((*b)->state==CS_SPECTATOR) return strcmp((*a)->name, (*b)->name);
			else return 1;
		}
		else if((*b)->state==CS_SPECTATOR) return -1;
		if((*a)->frags > (*b)->frags) return -1;
		if((*a)->frags < (*b)->frags) return 1;
		return strcmp((*a)->name, (*b)->name);
	}

    void bestplayers(vector<fpsent *> &best)
	{
		loopi(cl.numdynents())
		{
			fpsent *o = (fpsent *)cl.iterdynents(i);
            if(o && o->type!=ENT_AI && o->state!=CS_SPECTATOR) best.add(o);
		}
        best.sort(playersort);   
		while(best.length()>1 && best.last()->frags < best[0]->frags) best.drop();
	}

	void sortteams(vector<teamscore> &teamscores)
	{
#ifdef BFRONTIER
		if(m_capture(cl.gamemode))
#else
		int gamemode = cl.gamemode;
		if(m_capture)
#endif
		{
			loopv(cl.cpc.scores) teamscores.add(teamscore(cl.cpc.scores[i].team, cl.cpc.scores[i].total));
		}
		loopi(cl.numdynents())
		{
			fpsent *o = (fpsent *)cl.iterdynents(i);
			if(o && o->type!=ENT_AI)
			{
				teamscore *ts = NULL;
				loopv(teamscores) if(!strcmp(teamscores[i].team, o->team)) { ts = &teamscores[i]; break; }
#ifdef BFRONTIER
				if(!ts) teamscores.add(teamscore(o->team, m_capture(cl.gamemode) ? 0 : o->frags));
				else if(!m_capture(cl.gamemode)) ts->score += o->frags;
#else
				if(!ts) teamscores.add(teamscore(o->team, m_capture ? 0 : o->frags));
				else if(!m_capture) ts->score += o->frags;
#endif
			}
		}
		teamscores.sort(teamscorecmp);
	}

    void bestteams(vector<const char *> &best)
	{
		vector<teamscore> teamscores;
		sortteams(teamscores);
		while(teamscores.length()>1 && teamscores.last().score < teamscores[0].score) teamscores.drop();
		loopv(teamscores) best.add(teamscores[i].team);
	}

    struct scoregroup : teamscore
    {
        vector<fpsent *> players;
    };
    vector<scoregroup *> groups;
    vector<fpsent *> spectators;

    static int scoregroupcmp(const scoregroup **x, const scoregroup **y)
    {
        if(!(*x)->team)
        {
            if((*y)->team) return 1;
        }
        else if(!(*y)->team) return -1;
        if((*x)->score > (*y)->score) return -1;
        if((*x)->score < (*y)->score) return 1;
        if((*x)->players.length() > (*y)->players.length()) return -1;
        if((*x)->players.length() < (*y)->players.length()) return 1;
        return (*x)->team && (*y)->team ? strcmp((*x)->team, (*y)->team) : 0;
    }

    int groupplayers()
    {
#ifdef BFRONTIER
        int numgroups = 0;
#else
        int gamemode = cl.gamemode, numgroups = 0;
#endif
        spectators.setsize(0);
        loopi(cl.numdynents())
        {
            fpsent *o = (fpsent *)cl.iterdynents(i);
            if(!o || o->type==ENT_AI || (!showconnecting() && !o->name[0])) continue;
            if(o->state==CS_SPECTATOR) { spectators.add(o); continue; }
#ifdef BFRONTIER
            const char *team = m_team(cl.gamemode, cl.mutators) && o->team[0] ? o->team : NULL;
            bool found = false;
            loopj(numgroups)
            {
                scoregroup &g = *groups[j];
                if(team!=g.team && (!team || !g.team || !isteam(team, g.team))) continue;
                if(team && !m_capture(cl.gamemode)) g.score += o->frags;
                g.players.add(o);
                found = true;
            }
#else
            const char *team = m_teammode && o->team[0] ? o->team : NULL;
            bool found = false;
            loopj(numgroups)
            {
                scoregroup &g = *groups[j];
                if(team!=g.team && (!team || !g.team || strcmp(team, g.team))) continue;
                if(team && !m_capture) g.score += o->frags;
                g.players.add(o);
                found = true;
            }
#endif
            if(found) continue;
            if(numgroups>=groups.length()) groups.add(new scoregroup);
            scoregroup &g = *groups[numgroups++];
            g.team = team;
#ifdef BFRONTIER
            g.score = team ? (m_capture(cl.gamemode) ? cl.cpc.findscore(o->team).total : o->frags) : 0;
#else
            g.score = team ? (m_capture ? cl.cpc.findscore(o->team).total : o->frags) : 0;
#endif
            g.players.setsize(0);
            g.players.add(o);
        }
        loopi(numgroups) groups[i]->players.sort(playersort);
        spectators.sort(playersort);
        groups.sort(scoregroupcmp, 0, numgroups);
        return numgroups;
    }

	void gui(g3d_gui &g, bool firstpass)
	{
        g.start(menustart, 0.03f, NULL, false);
	
#ifdef BFRONTIER
		s_sprintfd(modemapstr)("%s: %s", m_name(cl.gamemode), mapname);
        if((m_timed(cl.gamemode) || cl.cc.demoplayback) && cl.minremain >= 0)
#else
		int gamemode = cl.gamemode;
		s_sprintfd(modemapstr)("%s: %s", fpsserver::modestr(gamemode), cl.getclientmap()[0] ? cl.getclientmap() : "[new map]");
        if((gamemode>1 || (gamemode==0 && (multiplayer(false) || cl.cc.demoplayback))) && cl.minremain >= 0)
#endif
		{
			if(!cl.minremain) s_strcat(modemapstr, ", intermission");
			else
			{
				s_sprintfd(timestr)(", %d %s remaining", cl.minremain, cl.minremain==1 ? "minute" : "minutes");
				s_strcat(modemapstr, timestr);
			}
		}
		g.text(modemapstr, 0xFFFF80, "server");
		
#ifdef BFRONTIER
		if (!cl.minremain || scoresinfo())
		{
			int accuracy = cl.player1->totaldamage*100/max(cl.player1->totalshots, 1);
			
			g.separator();

			g.textf("%s: \fs\f0%d\fS frag(s), \fs\f0%d\fS death(s)", 0xFFFFFF, "player", cl.player1->name, cl.player1->frags, cl.player1->deaths);
			g.textf("damage: \fs\f0%d\fS hp, wasted: \fs\f0%d\fS, accuracy: \fs\f0%d%%\fS", 0xFFFFFF, "info", cl.player1->totaldamage, cl.player1->totalshots-cl.player1->totaldamage, accuracy);

			if(m_sp(cl.gamemode))
			{
				int pen, score = 0;
				
				pen = (cl.lastmillis-cl.maptime)/1000;
				score += pen;
				if(pen)
					g.textf("time taken: \fs\f0%d\fS second(s)", 0xFFFFFF, "info", pen);
				
				pen = cl.player1->deaths*60;
				score += pen;
				if(pen)
					g.textf("penalty for \fs\f0%d\fS deaths: \fs\f0%d\fS second(s)", 0xFFFFFF, "info", pen);
				
				pen = 100-accuracy;
				score += pen;
				if(pen)
					g.textf("penalty for missed shots: \fs\f0%d\fS second(s)", 0xFFFFFF, "info", pen);
				
				s_sprintfd(aname)("bestscore_%s", mapname);
				const char *bestsc = getalias(aname);
				int bestscore = *bestsc ? atoi(bestsc) : score;
				if(score<bestscore) bestscore = score;
				s_sprintfd(nscore)("%d", bestscore);
				alias(aname, nscore);
				g.textf("SCORE: \fs\f0%d\fS second(s), best: \fs\f0%d\fS second(s)", 0xFFFFFF, "info", score, bestscore);
			}
		}
		g.separator();
#endif
        int numgroups = groupplayers();
        loopk(numgroups)
        {
            if((k%2)==0) g.pushlist(); // horizontal
            
            scoregroup &sg = *groups[k];
#ifdef BFRONTIER
            const char *icon = sg.team && m_team(cl.gamemode, cl.mutators) ? (isteam(cl.player1->team, sg.team) ? "player_blue" : "player_red") : "player";
            int bgcolor = sg.team && m_team(cl.gamemode, cl.mutators) ? (isteam(cl.player1->team, sg.team) ? 0x3030C0 : 0xC03030) : 0,
                fgcolor = 0xFFFF80;
#else
            const char *icon = cl.fr.ogro() ? "ogro" : (sg.team && m_teammode ? (isteam(cl.player1->team, sg.team) ? "player_blue" : "player_red") : "player");
            int bgcolor = sg.team && m_teammode ? (isteam(cl.player1->team, sg.team) ? 0x3030C0 : 0xC03030) : 0,
                fgcolor = 0xFFFF80;
#endif

            g.pushlist(); // vertical
            g.pushlist(); // horizontal

            #define loopscoregroup(o, b) \
                loopv(sg.players) \
                { \
                    fpsent *o = sg.players[i]; \
                    b; \
                }    

            g.pushlist();
#ifdef BFRONTIER
            if(sg.team && m_team(cl.gamemode, cl.mutators))
            {
                g.pushlist();
                g.background(bgcolor, numgroups>1 ? 3 : 5);
                g.strut(1);
                g.poplist();
            }
            g.text("", 0, "server");
            loopscoregroup(o,
            {
                if(o==cl.player1 && highlightscore() && cl.cc.demoplayback)
                {
                    g.pushlist();
                    g.background(0x808080, numgroups>1 ? 3 : 5);
                }
                g.text("", 0, icon);
                if(o==cl.player1 && highlightscore() && cl.cc.demoplayback) g.poplist();
            });
            g.poplist();

            if(sg.team && m_team(cl.gamemode, cl.mutators))
            {
                g.pushlist(); // vertical
                if(m_capture(cl.gamemode) && sg.score>=10000) g.textf("%s: WIN", fgcolor, NULL, sg.team);
                else g.textf("%s: %d", fgcolor, NULL, sg.team, sg.score);

                g.pushlist(); // horizontal
            }
            if(!m_capture(cl.gamemode))
            { 
                g.pushlist();
                g.strut(7);
                g.text("frags", fgcolor);
                loopscoregroup(o, g.textf("%d", 0xFFFFDD, NULL, o->frags));
                g.poplist();
            }
            if(multiplayer(false) || cl.cc.demoplayback)
            {
                if(showpj())
                {
                    g.pushlist();
                    g.strut(6);
                    g.text("pj", fgcolor);
                    loopscoregroup(o,
					{
                        if(o->state==CS_LAGGED) g.text("LAG", 0xFFFFDD);
                        else g.textf("%d", 0xFFFFDD, NULL, o->plag);
                    });
                    g.poplist();
                }
        
                if(showping())
				{
                    g.pushlist();
                    g.text("ping", fgcolor);
                    g.strut(6);
                    loopscoregroup(o, g.textf("%d", 0xFFFFDD, NULL, o->ping));
                    g.poplist();
				}
			}
 
            g.pushlist();
            g.text("name", fgcolor);
            loopscoregroup(o, 
			{
                int status = 0xFFFFDD;
                if(o->privilege) status = o->privilege>=PRIV_ADMIN ? 0xFF8000 : 0x40FF80;
                else if(o->state==CS_DEAD) status = 0x606060;
                g.text(cl.colorname(o), status);
            });
            g.poplist();

            if(showclientnum() || cl.player1->privilege>=PRIV_MASTER)
			{
                g.space(1);
                g.pushlist();
                g.text("cn", fgcolor);
                loopscoregroup(o, g.textf("%d", 0xFFFFDD, NULL, o->clientnum));
                g.poplist();
            }
            
            if(sg.team && m_team(cl.gamemode, cl.mutators))
			{ 
                g.poplist(); // horizontal
				g.poplist(); // vertical
			}

            g.poplist(); // horizontal
            g.poplist(); // vertical

            if(k+1<numgroups && (k+1)%2) g.space(2);
            else g.poplist(); // horizontal
        }
#else
            if(sg.team && m_teammode)
            {
                g.pushlist();
                g.background(bgcolor, numgroups>1 ? 3 : 5);
                g.strut(1);
                g.poplist();
            }
            g.text("", 0, "server");
            loopscoregroup(o,
            {
                if(o==cl.player1 && highlightscore() && (multiplayer(false) || cl.cc.demoplayback))
                {
                    g.pushlist();
                    g.background(0x808080, numgroups>1 ? 3 : 5);
                }
                g.text("", 0, icon);
                if(o==cl.player1 && highlightscore() && (multiplayer(false) || cl.cc.demoplayback)) g.poplist();
            });
            g.poplist();

            if(sg.team && m_teammode)
            {
                g.pushlist(); // vertical
                if(m_capture && sg.score>=10000) g.textf("%s: WIN", fgcolor, NULL, sg.team);
                else g.textf("%s: %d", fgcolor, NULL, sg.team, sg.score);

                g.pushlist(); // horizontal
            }
            if(!m_capture)
            { 
                g.pushlist();
                g.strut(7);
                g.text("frags", fgcolor);
                loopscoregroup(o, g.textf("%d", 0xFFFFDD, NULL, o->frags));
                g.poplist();
            }
            if(multiplayer(false) || cl.cc.demoplayback)
            {
                if(showpj())
                {
                    g.pushlist();
                    g.strut(6);
                    g.text("pj", fgcolor);
                    loopscoregroup(o,
					{
                        if(o->state==CS_LAGGED) g.text("LAG", 0xFFFFDD);
                        else g.textf("%d", 0xFFFFDD, NULL, o->plag);
                    });
                    g.poplist();
                }
        
                if(showping())
				{
                    g.pushlist();
                    g.text("ping", fgcolor);
                    g.strut(6);
                    loopscoregroup(o, g.textf("%d", 0xFFFFDD, NULL, o->ping));
                    g.poplist();
				}
			}
 
            g.pushlist();
            g.text("name", fgcolor);
            loopscoregroup(o, 
			{
                int status = 0xFFFFDD;
                if(o->privilege) status = o->privilege>=PRIV_ADMIN ? 0xFF8000 : 0x40FF80;
                else if(o->state==CS_DEAD) status = 0x606060;
                g.text(cl.colorname(o), status);
            });
            g.poplist();

            if(showclientnum() || cl.player1->privilege>=PRIV_MASTER)
			{
                g.space(1);
                g.pushlist();
                g.text("cn", fgcolor);
                loopscoregroup(o, g.textf("%d", 0xFFFFDD, NULL, o->clientnum));
                g.poplist();
            }
            
            if(sg.team && m_teammode)
			{ 
                g.poplist(); // horizontal
				g.poplist(); // vertical
			}

            g.poplist(); // horizontal
            g.poplist(); // vertical

            if(k+1<numgroups && (k+1)%2) g.space(2);
            else g.poplist(); // horizontal
        }
#endif
        if(showspectators() && spectators.length())
        {
            if(showclientnum() || cl.player1->privilege>=PRIV_MASTER)
            {
                g.pushlist();
                
                g.pushlist();
                g.text("spectator", 0xFFFF80, "server");
                loopv(spectators) 
                {
                    fpsent *o = spectators[i];
                    if(o==cl.player1 && highlightscore())
                    {
                        g.pushlist();
                        g.background(0x808080, 3);
					}
#ifdef BFRONTIER
                    g.text(cl.colorname(o), 0xFFFFDD, "player");
#else
                    g.text(cl.colorname(o), 0xFFFFDD, cl.fr.ogro() ? "ogro" : "player");
#endif
                    if(o==cl.player1 && highlightscore()) g.poplist();
				}
                g.poplist();

                g.space(1);
                g.pushlist();
                g.text("cn", 0xFFFF80);
                loopv(spectators) g.textf("%d", 0xFFFFDD, NULL, spectators[i]->clientnum);
                g.poplist();

                g.poplist();
            }
            else
            {
                g.textf("%d spectator%s", 0xFFFF80, "server", spectators.length(), spectators.length()!=1 ? "s" : "");
                loopv(spectators)
                {
                    if((i%3)==0) 
					{
                        g.pushlist();
#ifdef BFRONTIER
                        g.text("", 0xFFFFDD, "player");
#else
                        g.text("", 0xFFFFDD, cl.fr.ogro() ? "ogro" : "player");
#endif
                    }
                    fpsent *o = spectators[i];
                    int status = 0xFFFFDD;
                    if(o->privilege) status = o->privilege>=PRIV_ADMIN ? 0xFF8000 : 0x40FF80;
                    if(o==cl.player1 && highlightscore())
					{
                        g.pushlist();
                        g.background(0x808080);
                    }
                    g.text(cl.colorname(o), status);
                    if(o==cl.player1 && highlightscore()) g.poplist();
                    if(i+1<spectators.length() && (i+1)%3) g.space(1);
                    else g.poplist();
				}
			}
		}

		g.end();
	}
	
	void show()
	{
		if(scoreson) 
		{
			g3d_addgui(this, menupos, true);
		}
	}
};
