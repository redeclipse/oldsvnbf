struct scoreboard : g3d_callback
{
	bool scoreson;
	int menustart;
	gameclient &cl;

	struct sline { string s; };
	struct teamscore
	{
		int team, score;
		teamscore() {}
		teamscore(int s, int n) : team(s), score(n) {}
	};
    struct scoregroup : teamscore
    {
        vector<gameent *> players;
    };
    vector<scoregroup *> groups;
    vector<gameent *> spectators;

	IVARP(scoresinfo, 0, 1, 1);
	IVARP(showclientnum, 0, 1, 1);
    IVARP(showpj, 0, 1, 1);
    IVARP(showping, 0, 1, 1);
    IVARP(showspectators, 0, 1, 1);
    IVARP(highlightscore, 0, 1, 1);
    IVARP(showconnecting, 0, 0, 1);

	scoreboard(gameclient &_cl) : scoreson(false), cl(_cl)
	{
        CCOMMAND(showscores, "D", (scoreboard *self, int *down), self->showscores(*down!=0));
	}

	void showscores(bool on, bool interm = false)
	{
		if(!scoreson && on) menustart = starttime();
		scoreson = on;
		if(interm)
		{
			if(m_mission(cl.gamemode))
				cl.et.announce(S_V_MCOMPLETE, "mission complete!", true);
			else
			{
				if(!groupplayers()) return;
				scoregroup &sg = *groups[0];
				if(m_team(cl.gamemode, cl.mutators))
				{
					bool win = sg.players.find(cl.player1) >= 0;
					s_sprintfd(s)("%s team \fs%s%s\fS won the match with a total score of %d", win ? "your" : "enemy", teamtype[sg.team].chat, teamtype[sg.team].name, sg.score);
					cl.et.announce(win ? S_V_YOUWIN : S_V_YOULOSE, s, true);
				}
				else
				{
					bool win = sg.players[0] == cl.player1;
					s_sprintfd(s)("%s won the match with a total score of %d", win ? "you" : cl.colorname(sg.players[0]), sg.players[0]->frags);
					cl.et.announce(win ? S_V_YOUWIN : S_V_YOULOSE, s, true);
				}
			}
		}
	}

	static int teamscorecmp(const teamscore *x, const teamscore *y)
	{
		if(x->score > y->score) return -1;
		if(x->score < y->score) return 1;
		return x->team-y->team;
	}

	static int playersort(const gameent **a, const gameent **b)
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

    void bestplayers(vector<gameent *> &best)
	{
		loopi(cl.numdynents())
		{
			gameent *o = (gameent *)cl.iterdynents(i);
            if(o && o->type==ENT_PLAYER && o->state!=CS_SPECTATOR) best.add(o);
		}
        best.sort(playersort);
		while(best.length()>1 && best.last()->frags < best[0]->frags) best.drop();
	}

	void sortteams(vector<teamscore> &teamscores)
	{
		if(m_stf(cl.gamemode))
		{
			loopv(cl.stf.scores) teamscores.add(teamscore(cl.stf.scores[i].team, cl.stf.scores[i].total));
		}
        else if(m_ctf(cl.gamemode))
        {
        	loopi(numteams(cl.gamemode, cl.mutators))
				teamscores.add(teamscore(i+TEAM_ALPHA, cl.ctf.findscore(i+TEAM_ALPHA)));
        }
		loopi(cl.numdynents())
		{
			gameent *o = (gameent *)cl.iterdynents(i);
            if(o && o->type==ENT_PLAYER)
			{
				teamscore *ts = NULL;
				loopv(teamscores) if(teamscores[i].team == o->team) { ts = &teamscores[i]; break; }
				if(!ts) teamscores.add(teamscore(o->team, m_stf(cl.gamemode) || m_ctf(cl.gamemode) ? 0 : o->frags));
				else if(!m_stf(cl.gamemode) && !m_ctf(cl.gamemode)) ts->score += o->frags;
			}
		}
		teamscores.sort(teamscorecmp);
	}

    void bestteams(vector<int> &best)
	{
		vector<teamscore> teamscores;
		sortteams(teamscores);
		while(teamscores.length()>1 && teamscores.last().score < teamscores[0].score) teamscores.drop();
		loopv(teamscores) best.add(teamscores[i].team);
	}

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
        return (*x)->team && (*y)->team ? (*x)->team-(*y)->team : 0;
    }

    int groupplayers()
    {
        int numgroups = 0;
        spectators.setsize(0);
        loopi(cl.numdynents())
        {
            gameent *o = (gameent *)cl.iterdynents(i);
            if(!o || o->type!=ENT_PLAYER || (!showconnecting() && !o->name[0])) continue;
            if(o->state==CS_SPECTATOR) { spectators.add(o); continue; }
            int team = m_team(cl.gamemode, cl.mutators) ? o->team : TEAM_NEUTRAL;
            bool found = false;
            loopj(numgroups)
            {
                scoregroup &g = *groups[j];
                if(team != g.team) continue;
                if(team && !m_stf(cl.gamemode) && !m_ctf(cl.gamemode)) g.score += o->frags;
                g.players.add(o);
                found = true;
            }
            if(found) continue;
            if(numgroups>=groups.length()) groups.add(new scoregroup);
            scoregroup &g = *groups[numgroups++];
            g.team = team;
            if(!team) g.score = 0;
            else if(m_stf(cl.gamemode)) g.score = cl.stf.findscore(o->team).total;
            else if(m_ctf(cl.gamemode)) g.score = cl.ctf.findscore(o->team);
            else g.score = o->frags;

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

		int numgroups = groupplayers();

		s_sprintfd(modemapstr)("%s: %s", sv->gamename(cl.gamemode, cl.mutators), getmapname());
        if((m_timed(cl.gamemode) || cl.cc.demoplayback) && cl.minremain >= 0)
		{
			if(!cl.minremain) s_strcat(modemapstr, ", intermission");
			else
			{
				s_sprintfd(cstr)(", %d %s remain", cl.minremain, cl.minremain==1 ? "minute" : "minutes");
				s_strcat(modemapstr, cstr);
				//if(m_ctf(cl.gamemode) && ctflimit)
				//{
				//	int captures = clamp(groups[0]->score-ctflimit, 0, ctflimit);
				//	s_sprintf(cstr)(", %d %s to go", captures, captures==1 ? "capture" : "captures");
				//	s_strcat(modemapstr, cstr);
				//}
			}
		}
		g.text(modemapstr, 0xFFFF80, "server");

		if(!cl.minremain || scoresinfo())
		{
			int accuracy = cl.player1->totaldamage*100/max(cl.player1->totalshots, 1);

			g.separator();

			g.textf("%s: \fs\f0%d\fS frag(s), \fs\f0%d\fS death(s)", 0xFFFFFF, "player", cl.player1->name, cl.player1->frags, cl.player1->deaths);
			g.textf("damage: \fs\f0%d\fS hp, wasted: \fs\f0%d\fS, accuracy: \fs\f0%d%%\fS", 0xFFFFFF, "info", cl.player1->totaldamage, cl.player1->totalshots-cl.player1->totaldamage, accuracy);

			if(m_mission(cl.gamemode))
			{
				int pen, score = 0;

				pen = (lastmillis-cl.maptime)/1000;
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

				s_sprintfd(aname)("bestscore_%s", getmapname());
				const char *bestsc = getalias(aname);
				int bestscore = *bestsc ? atoi(bestsc) : score;
				if(score<bestscore) bestscore = score;
				s_sprintfd(nscore)("%d", bestscore);
				alias(aname, nscore);
				g.textf("SCORE: \fs\f0%d\fS second(s), best: \fs\f0%d\fS second(s)", 0xFFFFFF, "info", score, bestscore);
			}
		}
		g.separator();

        loopk(numgroups)
        {
            if((k%2)==0) g.pushlist(); // horizontal

            scoregroup &sg = *groups[k];
            const char *icon = sg.team && m_team(cl.gamemode, cl.mutators) ? teamtype[sg.team].icon : "player";
            int bgcolor = sg.team && m_team(cl.gamemode, cl.mutators) ? teamtype[sg.team].colour : 0,
                fgcolor = 0xFFFF80;

			g.pushlist(); // vertical
            g.pushlist(); // horizontal

            #define loopscoregroup(o, b) \
                loopv(sg.players) \
                { \
                    gameent *o = sg.players[i]; \
                    b; \
                }

            g.pushlist();
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
                const char *oicon = icon;
                g.text("", 0, oicon);
                if(o==cl.player1 && highlightscore() && cl.cc.demoplayback) g.poplist();
            });
            g.poplist();

            if(sg.team && m_team(cl.gamemode, cl.mutators))
            {
                g.pushlist(); // vertical

                if(m_stf(cl.gamemode) && sg.score>=10000) g.textf("%s: WIN", fgcolor, NULL, teamtype[sg.team].name);
                else g.textf("%s: %d", fgcolor, NULL, teamtype[sg.team].name, sg.score);

                g.pushlist(); // horizontal
            }

            if(!m_stf(cl.gamemode) && !m_ctf(cl.gamemode))
            {
                g.pushlist();
                g.strut(7);
                g.text("frags", fgcolor);
                loopscoregroup(o, g.textf("%d", 0xFFFFDD, NULL, o->frags));
                g.poplist();
            }

			if(showpj())
			{
				g.pushlist();
				g.strut(6);
				g.text("pj", fgcolor);
				loopscoregroup(o,
				{
					if(o->aitype != AI_NONE) g.textf("\fs%s%d\fS", 0xFF9955, NULL, o->ownernum == cl.player1->clientnum ? "\fg" : "\fc", o->skill);
					else if(o->state==CS_LAGGED) g.text("LAG", 0xFFFFDD);
					else g.textf("%d", 0xFFFFDD, NULL, o->plag);
				});
				g.poplist();
			}

			if(showping())
			{
				g.pushlist();
				g.text("ping", fgcolor);
				g.strut(6);
				loopscoregroup(o,
				{
					if(o->aitype != AI_NONE)
					{
						gameent *od = cl.getclient(o->ownernum);
						g.textf("\fs%s%d\fS", 0xFF9955, NULL, o->ownernum == cl.player1->clientnum ? "\fg" : "\fc", od ? od->ping : 0);
					}
					else g.textf("%d", 0xFFFFDD, NULL, o->ping);
				});
				g.poplist();
			}

            g.pushlist();
            g.text("name", fgcolor);
            loopscoregroup(o,
			{
                int status = 0xFFFFDD;
                if(o->privilege) status = o->privilege>=PRIV_ADMIN ? 0xFF8000 : 0x40FF80;
                else if(o->state==CS_DEAD) status = 0x606060;
                g.text(cl.colorname(o, NULL, "", false), status);
            });
            g.poplist();

            if(showclientnum() || cl.player1->privilege>=PRIV_MASTER)
			{
                g.space(1);
                g.pushlist();
                g.text("cn", fgcolor);
                loopscoregroup(o,
                {
                	if(o->aitype != AI_NONE)
						g.textf("\fw%d [\fs%s%d\fS]", 0xFF9955, NULL, o->clientnum, o->ownernum == cl.player1->clientnum ? "\fg" : "\fc", o->ownernum);
					else g.textf("%d", 0xFFFFDD, NULL, o->clientnum);
				});
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

		if(showspectators() && spectators.length())
        {
            if(showclientnum() || cl.player1->privilege>=PRIV_MASTER)
            {
                g.pushlist();

                g.pushlist();
                g.text("spectator", 0xFFFF80, "server");
                loopv(spectators)
                {
                    gameent *o = spectators[i];
                    if(o==cl.player1 && highlightscore())
                    {
                        g.pushlist();
                        g.background(0x808080, 3);
					}
                    g.text(cl.colorname(o, NULL, "", false), 0xFFFFDD, "player");
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
                        g.text("", 0xFFFFDD, "player");
                    }
                    gameent *o = spectators[i];
                    int status = 0xFFFFDD;
                    if(o->privilege) status = o->privilege>=PRIV_ADMIN ? 0xFF8000 : 0x40FF80;
                    if(o==cl.player1 && highlightscore())
					{
                        g.pushlist();
                        g.background(0x808080);
                    }
                    g.text(cl.colorname(o, NULL, "", false), status);
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
		if(scoreson) g3d_addgui(this);
	}
} sb;
