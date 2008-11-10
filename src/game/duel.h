struct duelservmode : servmode
{
	static const int DUELMILLIS = 5000;

	int duelround, dueltime;
	vector<int> duelqueue;

	duelservmode(gameserver &sv) : servmode(sv) {}

	void queue(clientinfo *ci, bool msg = false, bool top = false)
	{
		if(ci->name[0] && ci->state.state != CS_SPECTATOR)
		{
			int n = duelqueue.find(ci->clientnum);
			if(n < 0)
			{
				if(top) duelqueue.insert(0, ci->clientnum);
				else duelqueue.add(ci->clientnum);
				if((n = duelqueue.find(ci->clientnum)) < 0) return;
			}
			if(ci->state.state != CS_WAITING)
			{
				sendf(-1, 1, "ri2", SV_WAITING, ci->clientnum);
				ci->state.state = CS_WAITING;
			}
			if(msg)
			{
				if(!dueltime || n > 1)
					sv.srvoutf(ci->clientnum, "\foyou are \fs\fy#%d\fS in the queue", (dueltime ? n-1 : n+1));
				else sv.srvoutf(ci->clientnum, "\foyou are \fs\fyNEXT\fS in the queue", n+1);
			}
		}
	}

	void entergame(clientinfo *ci) { queue(ci, true); }
	void leavegame(clientinfo *ci)
	{
		int n = duelqueue.find(ci->clientnum);
		if(n >= 0) duelqueue.remove(n);
	}

	bool damage(clientinfo *target, clientinfo *actor, int damage, int gun, int flags, const vec &hitpush = vec(0, 0, 0))
	{
		if(dueltime) return false;
		return true;
	}

	bool canspawn(clientinfo *ci, bool connecting = false, bool tryspawn = false)
	{
		if(tryspawn) queue(ci, true);
		return false; // you spawn when we want you to buddy
	}

	void died(clientinfo *ci, clientinfo *at) {}
	void changeteam(clientinfo *ci, int oldteam, int newteam) {}

	void clearitems()
	{
		if(sv_itemsallowed >= (m_insta(sv.gamemode, sv.mutators) ? 2 : 1))
		{
			loopvj(sv.sents)
			{
				sv.sents[j].millis = sv.gamemillis;
				if(!sv.sents[j].spawned)
				{
					sv.sents[j].spawned = true;
					sendf(-1, 1, "ri2", SV_ITEMSPAWN, j);
				}
			}
		}
		loopvj(sv.clients)
		{
			clientinfo *ci = sv.clients[j];
			loopk(GUN_MAX) ci->state.entid[k] = -1;
			ci->state.dropped.reset();
		}
	}

	void update()
	{
		if(sv.interm) return;
		loopvrev(duelqueue) if(!sv.clients.inrange(duelqueue[i]) || !sv.clients[duelqueue[i]]->name[0] || sv.clients[duelqueue[i]]->state.state == CS_SPECTATOR)
			duelqueue.remove(i);

		vector<clientinfo *> alive;
		alive.setsize(0);
		loopv(sv.clients) if(sv.clients[i]->name[0] && sv.clients[i]->state.state == CS_ALIVE)
			alive.add(sv.clients[i]);

		if(dueltime)
		{
			if(sv.gamemillis >= dueltime)
			{
				loopv(duelqueue) if(sv.clients.inrange(duelqueue[i]))
					alive.add(sv.clients[duelqueue[i]]);
				if(alive.length() > 1)
				{
					clearitems();
					loopv(alive)
					{
						alive[i]->state.state = CS_ALIVE;
						alive[i]->state.respawn(sv.gamemillis);
						sv.sendspawn(alive[i]);
						int n = duelqueue.find(alive[i]->clientnum);
						if(n >= 0) duelqueue.remove(n);
						if(i) break;
					}
					duelround++;

					s_sprintfd(namea)("%s", sv.colorname(alive[0]));
					s_sprintfd(nameb)("%s", sv.colorname(alive[1]));
					s_sprintfd(fight)("%s vs %s, round \fs\fy#%d\fS, FIGHT!", namea, nameb, duelround);
					sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);
					dueltime = 0;
				}
				else
				{
					loopv(sv.clients) if(sv.clients[i]->name[0] && sv.clients[i]->state.state != CS_SPECTATOR)
						queue(sv.clients[i]);
				}
			}
			else if(sv.gamemillis >= dueltime-DUELMILLIS/2)
			{
				clearitems();
			}
		}
		else if(alive.length() < 2)
		{
			if(!alive.empty())
			{
				sv.srvoutf(-1, "\fo%s won the duel", sv.colorname(alive[0]));
				sendf(alive[0]->clientnum, 1, "ri2s", SV_ANNOUNCE, S_V_YOUWIN, "you win!");
				queue(alive[0], false, true);
			}
			else sv.srvoutf(-1, "everyone died!");
			dueltime = sv.gamemillis + DUELMILLIS;
			loopv(sv.clients) if(sv.clients[i]->name[0] && sv.clients[i]->state.state != CS_SPECTATOR)
				queue(sv.clients[i], true);
		}
	}

	void reset(bool empty)
	{
		duelround = 0;
		dueltime = sv.gamemillis + DUELMILLIS;
		duelqueue.setsize(0);
		loopv(sv.clients) if(sv.clients[i]->name[0] && sv.clients[i]->state.state != CS_SPECTATOR)
			queue(sv.clients[i], true);
	}
} duelmutator;
