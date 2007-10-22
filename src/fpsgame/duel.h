struct duelservmode : servmode
{
	int duelgame, duelround, dueltime;
	vector<int> duelqueue;

	duelservmode(fpsserver &sv) : servmode(sv) {}

	void queue(clientinfo *ci, bool msg, bool dead)
	{
		if (dead && ci->state.state != CS_DEAD)
		{
			ci->state.state = CS_DEAD;
			sendf(-1, 1, "ri2", SV_FORCEDEATH, ci->clientnum);
		}
		
		if (ci->state.state == CS_DEAD)
		{
			int n = duelqueue.find(ci->clientnum)+1;
			if (!n)
			{
				n = duelqueue.length()+1;
				duelqueue.add(ci->clientnum);
			}
			
			if (msg)
			{
				const char *r = NULL;
				if (!(n%9) || !(n%8) || !(n%7) || !(n%6) || !(n%5) || !(n%4) || !(n%10) ||
					n == 11 || n == 12 || n == 13) r = "th";
				else if (!(n%3)) r = "rd";
				else if (!(n%2)) r = "nd";
				else if (!(n%1)) r = "st";
				
				sv.servsend(ci->clientnum, "you are %d%s in the duel queue", n, r ? r : "");
			}
		}
	}

	void initclient(clientinfo *ci, ucharbuf &p, bool connecting)
	{
		queue(ci, true, true);
	}

	void entergame(clientinfo *ci)
	{
		queue(ci, true, true);
	}
	
	void leavegame(clientinfo *ci)
	{
		int n = duelqueue.find(ci->clientnum);
		if (n >= 0) duelqueue.remove(n);
	}

	void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
	{
	}

	bool damage(clientinfo *target, clientinfo *actor, int damage, int gun, const vec &hitpush = vec(0, 0, 0))
	{
		if (dueltime) return false;
		return true;
	}

	bool canspawn(clientinfo *ci, bool connecting = false, bool tryspawn = false)
	{
		if (tryspawn) queue(ci, true, true);
		return false; // you spawn when we want you to buddy
	}

	void spawned(clientinfo *ci)
	{
	}

	void died(clientinfo *ci, clientinfo *at)
	{
		queue(ci, true, true);
	}

	void changeteam(clientinfo *ci, const char *oldteam, const char *newteam)
	{
	}
	
	void update()
	{
		if(sv.interm || sv.gamemillis < dueltime || sv.nonspectators() < 2) return;
		vector<clientinfo *> alive;
		
		alive.setsize(0);
		#define alivecheck(c,d,e) \
			if (c->name[0] && alive.find(c) < 0 && \
				(c->state.state == CS_ALIVE || c->state.state == CS_DEAD)) \
			{ \
				if (alive.length() < 2 && (e || c->state.state == CS_ALIVE) && \
					(!alive.length() || !m_team(sv.gamemode, sv.mutators) || !isteam(c->team, alive[0]->team))) \
				{ \
					alive.add(c); \
				} \
				else if (d < 0 || c->state.state == CS_ALIVE) \
				{ \
					queue(c, true, true); \
				} \
			}

		loopv(sv.clients)
		{
			clientinfo *ci = sv.clients[i];
			int n = duelqueue.find(ci->clientnum);
			alivecheck(ci, n, false);
		}

		if (dueltime)
		{
			loopv(duelqueue)
			{
				if (sv.clients.inrange(duelqueue[i]))
				{
					clientinfo *ci = sv.clients[duelqueue[i]];
					alivecheck(ci, i, true);
					break;
				}
			}

			if (alive.length() > 1)
			{
				string pl[2];
	
				loopv(alive)
				{
					int n = duelqueue.find(alive[i]->clientnum);
					
					if (i < 2)
					{
						alive[i]->state.state = CS_ALIVE;
						alive[i]->state.respawn();
						sv.sendspawn(alive[i]);
						s_strcpy(pl[i], sv.colorname(alive[i]));
						if (n >= 0) duelqueue.remove(n);
					}
				}
	
				sv.servsend(-1, "duel #%d, %s vs %s", ++duelround, pl[0], pl[1]);

				if (!m_insta(sv.gamemode, sv.mutators))
				{
					loopvj(sv.sents)
					{
						if (!sv.sents[j].spawned)
						{
							sv.sents[j].spawntime = 0;
							sv.sents[j].spawned = true;
							sendf(-1, 1, "ri2", SV_ITEMSPAWN, j);
						}
					}
				}
				dueltime = 0;
			}
		}
		else if (alive.length() < 2)
		{
			if (alive.length())
			{
				if (m_team(sv.gamemode, sv.mutators))
					sv.servsend(-1, "%s won the duel for team %s!", sv.colorname(alive[0]), alive[0]->team);
				else
					sv.servsend(-1, "%s won the duel!", sv.colorname(alive[0]));
			}
			else
			{
				sv.servsend(-1, "both duellers died!");
			}
			dueltime = sv.gamemillis + duelgame;
		}
	}

	void reset(bool empty)
	{
		duelround = 0;
		dueltime = sv.gamemillis + duelgame;
		duelqueue.setsize(0);
		
		loopv(sv.clients)
		{
			clientinfo *ci = sv.clients[i];
			queue(ci, true, true);
		}
	}

	void intermission()
	{
	}
};
