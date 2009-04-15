namespace hud
{
	struct scoreboard : g3d_callback
	{
		bool scoreson, shownscores;
		int menustart;

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

		IVARP(autoshowscores, 0, 1, 1);
		IVARP(showscoreswait, 0, 1, 1); // this uses spawndelay instead
		IVARP(showscoresdelay, 0, 3, INT_MAX-1); // otherwise use a static timespan
		IVARP(scoresinfo, 0, 1, 1);
		IVARP(highlightscore, 0, 1, 1);

		IVARP(showpj, 0, 0, 1);
		IVARP(showping, 0, 1, 1);
		IVARP(showclientnum, 0, 1, 1);
		IVARP(showskills, 0, 1, 1);
		IVARP(showownernum, 0, 0, 1);
		IVARP(showspectators, 0, 1, 1);
		IVARP(showconnecting, 0, 0, 1);

		scoreboard() : scoreson(false), shownscores(false)
		{
			CCOMMAND(showscores, "D", (scoreboard *self, int *down), self->showscores(*down!=0));
		}

		bool canshowscores()
		{
			if(autoshowscores() && game::player1->state == CS_DEAD && !scoreson && !shownscores)
			{
				if(!showscoresdelay() && !showscoreswait()) return true;
				else
				{
					int delay = showscoreswait() ? m_spawndelay(game::gamemode, game::mutators) : showscoresdelay()*1000;
					if(!delay || lastmillis-game::player1->lastdeath >= delay) return true;
				}
			}
			return false;
		}

		void showscores(bool on, bool interm = false)
		{
			if(!scoreson && on) menustart = starttime();
			scoreson = on;
			if(interm)
			{
				if(m_mission(game::gamemode)) game::announce(S_V_MCOMPLETE, "\fwmission complete!");
				else
				{
					if(!groupplayers()) return;
					scoregroup &sg = *groups[0];
					if(m_team(game::gamemode, game::mutators))
					{
						bool win = sg.players.find(game::player1) >= 0;
                        if(m_stf(game::gamemode) && sg.score==INT_MAX)
                            game::announce(win ? S_V_YOUWIN : S_V_YOULOSE, "\fw%s team \fs%s%s\fS secured all flags", win ? "your" : "enemy", teamtype[sg.team].chat, teamtype[sg.team].name);
						else
                            game::announce(win ? S_V_YOUWIN : S_V_YOULOSE, "\fw%s team \fs%s%s\fS won the match with a total score of %d", win ? "your" : "enemy", teamtype[sg.team].chat, teamtype[sg.team].name, sg.score);
					}
					else
					{
						bool win = sg.players[0] == game::player1;
						game::announce(win ? S_V_YOUWIN : S_V_YOULOSE, "\fw%s won the match with a total score of %d", win ? "you" : game::colorname(sg.players[0]), sg.players[0]->frags);
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
			loopi(game::numdynents())
			{
				gameent *o = (gameent *)game::iterdynents(i);
				if(o && o->type==ENT_PLAYER && o->state!=CS_SPECTATOR) best.add(o);
			}
			best.sort(playersort);
			while(best.length()>1 && best.last()->frags < best[0]->frags) best.drop();
		}

		void sortteams(vector<teamscore> &teamscores)
		{
			if(m_stf(game::gamemode))
			{
				loopv(stf::st.scores) teamscores.add(teamscore(stf::st.scores[i].team, stf::st.scores[i].total));
			}
			else if(m_ctf(game::gamemode))
			{
				loopv(ctf::st.scores) teamscores.add(teamscore(ctf::st.scores[i].team, ctf::st.scores[i].total));
			}
			loopi(game::numdynents())
			{
				gameent *o = (gameent *)game::iterdynents(i);
				if(o && o->type==ENT_PLAYER)
				{
					teamscore *ts = NULL;
					loopv(teamscores) if(teamscores[i].team == o->team) { ts = &teamscores[i]; break; }
					if(!ts) teamscores.add(teamscore(o->team, m_stf(game::gamemode) || m_ctf(game::gamemode) ? 0 : o->frags));
					else if(!m_stf(game::gamemode) && !m_ctf(game::gamemode)) ts->score += o->frags;
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
			loopi(game::numdynents())
			{
				gameent *o = (gameent *)game::iterdynents(i);
				if(!o || o->type!=ENT_PLAYER || (!showconnecting() && !o->name[0])) continue;
				if(o->state==CS_SPECTATOR) { spectators.add(o); continue; }
				int team = m_team(game::gamemode, game::mutators) ? o->team : TEAM_NEUTRAL;
				bool found = false;
				loopj(numgroups)
				{
					scoregroup &g = *groups[j];
					if(team != g.team) continue;
					if(team && !m_stf(game::gamemode) && !m_ctf(game::gamemode)) g.score += o->frags;
					g.players.add(o);
					found = true;
				}
				if(found) continue;
				if(numgroups>=groups.length()) groups.add(new scoregroup);
				scoregroup &g = *groups[numgroups++];
				g.team = team;
				if(!team) g.score = 0;
				else if(m_stf(game::gamemode)) g.score = stf::st.findscore(o->team).total;
				else if(m_ctf(game::gamemode)) g.score = ctf::st.findscore(o->team).total;
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

			g.textf("%s", 0xFFFFFF, "info", server::gamename(game::gamemode, game::mutators));
			if((m_fight(game::gamemode) || client::demoplayback) && game::minremain >= 0)
			{
				if(!game::minremain) g.textf("%s: intermission", 0xFFFFFF, "info", getmapname());
				else g.textf("%s: %d %s remain", 0xFFFFFF, "info", getmapname(), game::minremain, game::minremain==1 ? "minute" : "minutes");
			}
			else g.textf("%s", 0xFFFFFF, "info", getmapname());

			if(game::intermission || scoresinfo())
			{
				int accuracy = game::player1->totaldamage*100/max(game::player1->totalshots, 1);

				g.separator();

				g.textf("%s: \fs\f0%d\fS %s(s), \fs\f0%d\fS %s(s)", 0xFFFFFF, "player", game::player1->name,
					game::player1->frags, m_paint(game::gamemode, game::mutators) ? "tag" : "frag",
					game::player1->deaths, m_paint(game::gamemode, game::mutators) ? "out" : "death");
				g.textf("damage: \fs\f0%d\fS hp, wasted: \fs\f0%d\fS, accuracy: \fs\f0%d%%\fS", 0xFFFFFF, "info", game::player1->totaldamage, game::player1->totalshots-game::player1->totaldamage, accuracy);

				if(m_mission(game::gamemode))
				{
					int pen, score = 0;

					pen = (lastmillis-game::maptime)/1000;
					score += pen;
					if(pen)
						g.textf("time taken: \fs\f0%d\fS second(s)", 0xFFFFFF, "info", pen);

					pen = game::player1->deaths*60;
					score += pen;
					if(pen)
						g.textf("penalty for \fs\f0%d\fS deaths: \fs\f0%d\fS second(s)", 0xFFFFFF, "info", pen);

					pen = 100-accuracy;
					score += pen;
					if(pen)
						g.textf("penalty for missed shots: \fs\f0%d\fS second(s)", 0xFFFFFF, "info", pen);

					defformatstring(aname)("bestscore_%s", getmapname());
					const char *bestsc = getalias(aname);
					int bestscore = *bestsc ? atoi(bestsc) : score;
					if(score<bestscore) bestscore = score;
					defformatstring(nscore)("%d", bestscore);
					alias(aname, nscore);
					g.textf("SCORE: \fs\f0%d\fS second(s), best: \fs\f0%d\fS second(s)", 0xFFFFFF, "info", score, bestscore);
				}
			}
			g.separator();

			loopk(numgroups)
			{
				if((k%2)==0) g.pushlist(); // horizontal

				scoregroup &sg = *groups[k];
				const char *icon = sg.team && m_team(game::gamemode, game::mutators) ? teamtype[sg.team].icon : "player";
				int bgcolor = sg.team && m_team(game::gamemode, game::mutators) ? teamtype[sg.team].colour : 0,
					fgcolor = 0xFFFFFF;

				g.pushlist(); // vertical
				g.pushlist(); // horizontal

				#define loopscoregroup(b) \
					loopv(sg.players) \
					{ \
						gameent *o = sg.players[i]; \
						b; \
					}

				g.pushlist();
				if(sg.team && m_team(game::gamemode, game::mutators))
				{
					g.pushlist();
					g.background(bgcolor, numgroups>1 ? 3 : 5);
					g.strut(1);
					g.poplist();
				}
				g.pushlist();
				g.background(0xFFFFFF, numgroups>1 ? 3 : 5);
				g.text("", 0, "server");
				g.poplist();
				loopscoregroup({
					g.pushlist();
					bool highlight = o==game::player1 && highlightscore();
					int status = highlight ? 0xDDDDDD : 0xAAAAAA;
					if(o->state==CS_DEAD || o->state==CS_WAITING) status = highlight ? 0x888888 : 0x666666;
					else if(o->privilege)
					{
						if(o->privilege >= PRIV_ADMIN) status = highlight ? 0xFF8800 : 0xAA6600;
						else status = highlight ? 0x44FF88 : 0x33AA66;
					}
					if(status) g.background(status, numgroups>1 ? 3 : 5);
					const char *oicon = icon;
					g.text("", 0, oicon);
					g.poplist();
				});
				g.poplist();

				if(sg.team && m_team(game::gamemode, game::mutators))
				{
					g.pushlist(); // vertical

					if(m_stf(game::gamemode) && stflimit && sg.score >= stflimit) g.textf("%s: WIN", fgcolor, NULL, teamtype[sg.team].name);
					else g.textf("%s: %d", fgcolor, NULL, teamtype[sg.team].name, sg.score);

					g.pushlist(); // horizontal
				}

				if(!m_stf(game::gamemode) && !m_ctf(game::gamemode))
				{
					g.pushlist();
					g.strut(7);
					g.text(m_paint(game::gamemode, game::mutators) ? "tags" : "frags", fgcolor);
					loopscoregroup(g.textf("%d", 0xFFFFFF, NULL, o->frags));
					g.poplist();
				}

				if(showpj())
				{
					g.pushlist();
					g.strut(6);
					g.text("pj", fgcolor);
					loopscoregroup({
						g.textf("%s%d", 0xFFFFFF, NULL, lastmillis-o->lastupdate > 1000 ? "\fo" : "", o->plag);
					});
					g.poplist();
				}

				if(showping())
				{
					g.pushlist();
					g.text("ping", fgcolor);
					g.strut(6);
					loopscoregroup({
						if(o->aitype != AI_NONE)
						{
							gameent *od = game::getclient(o->ownernum);
							g.textf("%d", 0xFFFFFF, NULL, od ? od->ping : -1);
						}
						else g.textf("%d", 0xFFFFFF, NULL, o->ping);
					});
					g.poplist();
				}

				g.pushlist();
				g.text("name", fgcolor);
				loopscoregroup(g.text(game::colorname(o, NULL, "", false), 0xFFFFFF));
				g.poplist();

				if(showclientnum() || game::player1->privilege>=PRIV_MASTER)
				{
					g.space(1);
					g.pushlist();
					g.text("cn", fgcolor);
					loopscoregroup({
						g.textf("%d", 0xFFFFFF, NULL, o->clientnum);
					});
					g.poplist();
				}

				if(showskills())
				{
					g.space(1);
					g.pushlist();
					g.text("sk", fgcolor);
					loopscoregroup({
						if(o->aitype != AI_NONE) g.textf("%d", 0xFFFFFF, NULL, o->skill);
						else g.space(1);
					});
					g.poplist();
				}

				if(showownernum())
				{
					g.space(1);
					g.pushlist();
					g.text("on", fgcolor);
					loopscoregroup({
						if(o->aitype != AI_NONE) g.textf("%d", 0xFFFFFF, NULL, o->ownernum);
						else g.space(1);
					});
					g.poplist();
				}

				if(sg.team && m_team(game::gamemode, game::mutators))
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
				if(showclientnum() || game::player1->privilege>=PRIV_MASTER)
				{
					g.pushlist();
					g.pushlist();
					g.text("spectator", 0xFFFFFF, "server");
					loopv(spectators)
					{
						gameent *o = spectators[i];
						g.pushlist();
						bool highlight = o==game::player1 && highlightscore();
						int status = highlight ? 0xDDDDDD : 0xAAAAAA;
						if(o->state==CS_DEAD || o->state==CS_WAITING) status = highlight ? 0x888888 : 0x666666;
						else if(o->privilege)
						{
							if(o->privilege >= PRIV_ADMIN) status = highlight ? 0xFF8800 : 0xAA6600;
							else status = highlight ? 0x44FF88 : 0x33AA66;
						}
						if(status) g.background(status, 3);
						g.textf("%s", 0xFFFFFF, "player", game::colorname(o, NULL, "", false));
						g.poplist();
					}
					g.poplist();
					g.space(1);
					g.pushlist();
					g.text("cn", 0xFFFFFF);
					loopv(spectators)
					{
						gameent *o = spectators[i];
						g.textf("%d", 0xFFFFFF, NULL, o->clientnum);
					}
					g.poplist();
					g.poplist();
				}
				else
				{
					g.textf("%d spectator%s", 0xFFFFFF, "server", spectators.length(), spectators.length()!=1 ? "s" : "");
					loopv(spectators)
					{
						if((i%3)==0) g.pushlist();
						gameent *o = spectators[i];
						g.pushlist();
						bool highlight = o==game::player1 && highlightscore();
						int status = highlight ? 0xDDDDDD : 0xAAAAAA;
						if(o->state==CS_DEAD || o->state==CS_WAITING) status = highlight ? 0x888888 : 0x666666;
						else if(o->privilege)
						{
							if(o->privilege >= PRIV_ADMIN) status = highlight ? 0xFF8800 : 0xAA6600;
							else status = highlight ? 0x44FF88 : 0x33AA66;
						}
						if(status) g.background(status);
						g.textf("%s", 0xFFFFFF, (i%3)==0 ? "player" : NULL, game::colorname(o, NULL, "", false));
						g.poplist();
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
			if(game::player1->state == CS_DEAD) { if(scoreson) shownscores = true; }
			else shownscores = false;
		}

		int drawinventoryitem(int x, int y, int s, float skew, float fade, int pos, int team, int score, const char *name)
		{
			const char *colour = "\fa";
			switch(pos)
			{
				case 0: colour = "\fg"; break;
				case 1: colour = "\fy"; break;
				case 2: colour = "\fo"; break;
				case 3: colour = "\fr"; break;
				default: break;
			}
			int sy = hud::drawitem(hud::flagtex(team), x, y, s, true, 1.f, 1.f, 1.f, fade, skew, "default", "\fs%s[\fS%d\fs%s]\fS", colour, score, colour);
			hud::drawitemsubtext(x, y, s, true, skew, "sub", fade, "%s%s", team ? teamtype[team].chat : "\fw", name);
			return sy;
		}

		int drawinventory(int x, int y, int s, float blend)
		{
			int sy = 0, numgroups = groupplayers(), numout = 0;
			loopi(2) loopk(numgroups)
			{
				scoregroup &sg = *groups[k];
				if(m_team(game::gamemode, game::mutators))
				{
					if(!sg.team || ((sg.team != game::player1->team) == !i)) continue;
					sy += drawinventoryitem(x, y-sy, s, 1.25f-clamp(numout,1,3)*0.25f, blend, k, sg.team, sg.score, teamtype[sg.team].name);
					if((numout += 1) > 3) return sy;
				}
				else
				{
					if(sg.team) continue;
					loopvj(sg.players)
					{
						gameent *d = sg.players[j];
						if((d != game::player1) == !i) continue;
						sy += drawinventoryitem(x, y-sy, s, 1.25f-clamp(numout,1,3)*0.25f, blend, j, sg.team, d->frags, game::colorname(d, NULL, "", false));
						if((numout += 1) > 3) return sy;
					}
				}
			}
			return sy;
		}
	};
	extern scoreboard sb;
}
