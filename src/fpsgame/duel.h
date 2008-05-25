struct duelservmode : servmode
{
	static const int DUELMILLIS = 3000;

	int duelround, dueltime;
	vector<int> duelqueue;

	duelservmode(GAMESERVER &sv) : servmode(sv) {}

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
				if(m_dlms(sv.gamemode, sv.mutators))
					sv.servsend(ci->clientnum, "waiting for next round..");
				else
				{
					const char *r = NULL;
					if (!(n%3) && n != 13) r = "rd";
					else if (!(n%2) && n != 12) r = "nd";
					else if (!(n%1) && n != 11) r = "st";
					else r = "th";
					sv.servsend(ci->clientnum, "you are %d%s in the duel queue", n, r ? r : "");
				}
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

	//void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
	//{
	//}

	bool damage(clientinfo *target, clientinfo *actor, int damage, int gun, int flags, const vec &hitpush = vec(0, 0, 0))
	{
		if (dueltime) return false;
		return true;
	}

	bool canspawn(clientinfo *ci, bool connecting = false, bool tryspawn = false)
	{
		if (tryspawn) queue(ci, true, true);
		return false; // you spawn when we want you to buddy
	}

	//void spawned(clientinfo *ci)
	//{
	//}

	void died(clientinfo *ci, clientinfo *at)
	{
		queue(ci, true, true);
	}

	void changeteam(clientinfo *ci, const char *oldteam, const char *newteam)
	{
		queue(ci, true, true);
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
				if ((m_dlms(sv.gamemode, sv.mutators) || alive.length() <= 1) && \
					(e || c->state.state == CS_ALIVE) && \
					(!alive.length() || !m_team(sv.gamemode, sv.mutators) || \
						!isteam(c->team, alive[0]->team))) \
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

			if (alive.length() >= 2)
			{
				string pl[2];

				loopv(alive)
				{
					int n = duelqueue.find(alive[i]->clientnum);

					if (m_dlms(sv.gamemode, sv.mutators) || i <= 1)
					{
						alive[i]->state.state = CS_ALIVE;
						alive[i]->state.respawn();
						sv.sendspawn(alive[i]);
						if(i <= 1) s_strcpy(pl[i], sv.colorname(alive[i]));
						if(n >= 0) duelqueue.remove(n);
					}
				}

				duelround++;

				string fight;
				if(m_dlms(sv.gamemode, sv.mutators))
					s_sprintf(fight)("round %d .. fight!", duelround);
				else
					s_sprintf(fight)("round %d .. %s vs %s .. fight!", duelround, pl[0], pl[1]);

				sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);

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
		else if (alive.length() <= 1)
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
				sv.servsend(-1, "everyone died!");
			}
			dueltime = sv.gamemillis + DUELMILLIS;
		}
	}

	void reset(bool empty)
	{
		duelround = 0;
		dueltime = sv.gamemillis + DUELMILLIS;
		duelqueue.setsize(0);

		loopv(sv.clients)
		{
			clientinfo *ci = sv.clients[i];
			queue(ci, true, true);
		}
	}

	//void intermission()
	//{
	//}
} duelmutator;
