struct duelservmode : servmode
{
	static const int DUELMILLIS = 3000;

	int duelround, dueltime;
	vector<int> duelqueue;

	duelservmode(GAMESERVER &sv) : servmode(sv) {}

	void queue(clientinfo *ci, bool msg, bool dead)
	{
		if(dead && ci->state.state != CS_DEAD)
		{
			ci->state.state = CS_DEAD;
			sendf(-1, 1, "ri2", SV_FORCEDEATH, ci->clientnum);
		}

		if(ci->state.state == CS_DEAD)
		{
			int n = duelqueue.find(ci->clientnum)+1;
			if(!n)
			{
				n = duelqueue.length()+1;
				duelqueue.add(ci->clientnum);
			}

			if(msg)
			{
				if(m_dlms(sv.gamemode, sv.mutators))
					sv.srvoutf(ci->clientnum, "waiting for next round..");
				else
				{
					const char *r = NULL;
					if(!(n%3) && n != 13) r = "rd";
					else if(!(n%2) && n != 12) r = "nd";
					else if(!(n%1) && n != 11) r = "st";
					else r = "th";
					sv.srvoutf(ci->clientnum, "you are %d%s in the duel queue", n, r ? r : "");
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
		if(n >= 0) duelqueue.remove(n);
	}

	//void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
	//{
	//}

	bool damage(clientinfo *target, clientinfo *actor, int damage, int gun, int flags, const vec &hitpush = vec(0, 0, 0))
	{
		if(dueltime) return false;
		return true;
	}

	bool canspawn(clientinfo *ci, bool connecting = false, bool tryspawn = false)
	{
		if(tryspawn) queue(ci, true, true);
		return false; // you spawn when we want you to buddy
	}

	//void spawned(clientinfo *ci)
	//{
	//}

	void died(clientinfo *ci, clientinfo *at)
	{
		queue(ci, true, true);
	}

	void changeteam(clientinfo *ci, int oldteam, int newteam)
	{
		queue(ci, true, true);
	}

	void update()
	{
		if(sv.interm || sv.gamemillis < dueltime || sv.nonspectators() < 2) return;
		vector<clientinfo *> alive;

		alive.setsize(0);
		#define alivecheck(c,e) \
			if(c->name[0] && alive.find(c) < 0 && (c->state.state == CS_ALIVE || c->state.state == CS_DEAD)) \
			{ \
				if(e || c->state.state == CS_ALIVE) alive.add(c); \
				else if(duelqueue.find(c->clientnum) || c->state.state == CS_ALIVE) \
					queue(c, true, true); \
			}

		loopv(sv.clients)
		{
			clientinfo *ci = sv.clients[i];
			alivecheck(ci, false);
		}

		if(dueltime)
		{
			loopv(duelqueue) if(sv.clients.inrange(duelqueue[i]))
			{
				clientinfo *ci = sv.clients[duelqueue[i]];
				alivecheck(ci, true);
			}

			if(alive.length() >= 2)
			{
				loopv(alive) if(m_dlms(sv.gamemode, sv.mutators) || i <= 1)
				{
					alive[i]->state.state = CS_ALIVE;
					alive[i]->state.respawn(sv.gamemillis);
					sv.sendspawn(alive[i]);
					int n = duelqueue.find(alive[i]->clientnum);
					if(n >= 0) duelqueue.remove(n);
				}
				duelround++;

				s_sprintfd(fight)("round %d .. fight!", duelround);
				sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);

				loopvj(sv.sents) if(!sv.sents[j].spawned && !m_noitems(sv.gamemode, sv.mutators))
				{
					sv.sents[j].spawned = true;
					sendf(-1, 1, "ri2", SV_ITEMSPAWN, j);
				}
				loopvj(sv.clients)
				{
					clientinfo *ci = sv.clients[j];
					loopk(GUN_MAX) ci->state.entid[k] = -1;
					ci->state.dropped.reset();
				}
				dueltime = 0;
			}
		}
		else if(alive.length() <= 1)
		{
			if(alive.length())
				sv.srvoutf(-1, "%s won the duel!", sv.colorname(alive[0]));
			else
				sv.srvoutf(-1, "everyone died!");
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
