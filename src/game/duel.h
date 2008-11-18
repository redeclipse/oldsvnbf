struct duelservmode : servmode
{
	static const int DUELMILLIS = 15000;

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
				loopk(GUN_MAX) ci->state.entid[k] = -1;
			}
			if(msg)
			{
				if(n > 0) sv.srvoutf(ci->clientnum, "\fyyou are \fs\fg#%d\fS in the queue", n+1);
				else sv.srvoutf(ci->clientnum, "\fyyou are \fs\fgNEXT\fS in the queue", n+1);
			}
		}
	}

	void entergame(clientinfo *ci) { queue(ci, m_duel(sv.gamemode, sv.mutators)); }
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
		if(tryspawn) queue(ci, m_duel(sv.gamemode, sv.mutators));
		return false; // you spawn when we want you to buddy
	}

	void died(clientinfo *ci, clientinfo *at) {}
	void changeteam(clientinfo *ci, int oldteam, int newteam) {}

	void clearitems()
	{
		if(sv_itemsallowed >= (m_insta(sv.gamemode, sv.mutators) ? 2 : 1))
		{
			loopvj(sv.sents) if(enttype[sv.sents[j].type].usetype == EU_ITEM && !sv.finditem(j, true, 0))
			{
				loopvk(sv.clients)
				{
					clientinfo *ci = sv.clients[k];
					ci->state.dropped.remove(j);
				}
				sv.sents[j].millis = sv.gamemillis; // hijack its spawn time
				sv.sents[j].spawned = true;
				sendf(-1, 1, "ri2", SV_ITEMSPAWN, j);
			}
		}
	}

	void cleanup()
	{
		loopvrev(duelqueue)
			if(!sv.clients.inrange(duelqueue[i]) || !sv.clients[duelqueue[i]]->name[0] || sv.clients[duelqueue[i]]->state.state == CS_SPECTATOR || sv.clients[duelqueue[i]]->state.state == CS_ALIVE)
				duelqueue.remove(i);
	}

	void update()
	{
		if(sv.interm || !sv.numclients()) return;

		cleanup();

		vector<clientinfo *> alive;
		alive.setsize(0);
		loopv(sv.clients) if(sv.clients[i]->name[0] && sv.clients[i]->state.state == CS_ALIVE)
			alive.add(sv.clients[i]);

		if(dueltime)
		{
			if(sv.gamemillis >= dueltime)
			{
				loopvj(sv.clients) if(sv.clients[j]->name[0] && sv.clients[j]->state.state != CS_ALIVE && sv.clients[j]->state.state != CS_SPECTATOR)
					queue(sv.clients[j], m_duel(sv.gamemode, sv.mutators));

				if(m_lms(sv.gamemode, sv.mutators) || alive.length() < 2) // while waiting for next round our two guys spawn
				{
					loopv(duelqueue)
					{
						if(m_duel(sv.gamemode, sv.mutators) && alive.length() >= 2) break;
						if(sv.clients.inrange(duelqueue[i]))
						{
							clientinfo *ci = sv.clients[duelqueue[i]];
							if(ci->state.state != CS_ALIVE)
							{
								ci->state.state = CS_ALIVE;
								ci->state.respawn(sv.gamemillis);
								sv.sendspawn(ci);
								alive.add(ci);
							}
						}
					}
					cleanup();
				}

				if(m_duel(sv.gamemode, sv.mutators) && alive.length() > 2)
				{
					loopvrev(alive)
					{
						queue(alive[i], m_duel(sv.gamemode, sv.mutators));
						alive.remove(i);
						if(alive.length() <= 2) break;
					}
				}

				clearitems();
				duelround++;
				if(m_duel(sv.gamemode, sv.mutators))
				{
					s_sprintfd(namea)("%s", sv.colorname(alive[0]));
					s_sprintfd(nameb)("%s", sv.colorname(alive[1]));
					s_sprintfd(fight)("Duel between %s and %s, round \fs\fy#%d\fS.. FIGHT!", namea, nameb, duelround);
					sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);
				}
				else if(m_lms(sv.gamemode, sv.mutators))
				{
					s_sprintfd(fight)("Last one left alive wins, round \fs\fy#%d\fS.. FIGHT!", duelround);
					sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);
				}
				dueltime = 0;
			}
		}
		else if(alive.length() < 2)
		{
			if(!alive.empty())
			{
				sv.srvoutf(-1, "\fy%s was the last one left alive", sv.colorname(alive[0]));
				sendf(alive[0]->clientnum, 1, "ri2s", SV_ANNOUNCE, S_V_YOUWIN, "you win!");
				alive[0]->state.health = MAXHEALTH;
				alive[0]->state.lastregen = sv.gamemillis;
				sendf(-1, 1, "ri3", SV_REGEN, alive[0]->clientnum, alive[0]->state.health);
			}
			else sv.srvoutf(-1, "\fyeveryone died, fail!");
			dueltime = sv.gamemillis + DUELMILLIS;
		}
	}

	void reset(bool empty)
	{
		duelround = 0;
		dueltime = sv.gamemillis + DUELMILLIS;
		duelqueue.setsize(0);
		loopv(sv.clients) if(sv.clients[i]->name[0] && sv.clients[i]->state.state != CS_SPECTATOR)
			queue(sv.clients[i], m_duel(sv.gamemode, sv.mutators));
		clearitems();
	}
} duelmutator;
