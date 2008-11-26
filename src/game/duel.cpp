#ifdef GAMESERVER
struct duelservmode : servmode
{
	static const int DUELMILLIS = 15000;

	int duelround, dueltime;
	vector<int> duelqueue;

	duelservmode() {}

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
				if(n > 0) srvoutf(ci->clientnum, "\fyyou are \fs\fg#%d\fS in the queue", n+1);
				else srvoutf(ci->clientnum, "\fyyou are \fs\fgNEXT\fS in the queue", n+1);
			}
		}
	}

	void entergame(clientinfo *ci) { queue(ci, m_duel(gamemode, mutators)); }
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
		if(tryspawn) queue(ci, m_duel(gamemode, mutators));
		return false; // you spawn when we want you to buddy
	}

	void died(clientinfo *ci, clientinfo *at) {}
	void changeteam(clientinfo *ci, int oldteam, int newteam) {}

	void clearitems()
	{
		if(sv_itemsallowed >= (m_insta(gamemode, mutators) ? 2 : 1))
		{
			loopvj(sents) if(enttype[sents[j].type].usetype == EU_ITEM && !finditem(j, true, 0))
			{
				loopvk(clients)
				{
					clientinfo *ci = clients[k];
					ci->state.dropped.remove(j);
				}
				sents[j].millis = gamemillis; // hijack its spawn time
				sents[j].spawned = true;
				sendf(-1, 1, "ri2", SV_ITEMSPAWN, j);
			}
		}
	}

	void cleanup()
	{
		loopvrev(duelqueue)
			if(!clients.inrange(duelqueue[i]) || !clients[duelqueue[i]]->name[0] || clients[duelqueue[i]]->state.state == CS_SPECTATOR || clients[duelqueue[i]]->state.state == CS_ALIVE)
				duelqueue.remove(i);
	}

	void update()
	{
		if(interm || !numclients()) return;

		cleanup();

		vector<clientinfo *> alive;
		alive.setsize(0);
		loopv(clients) if(clients[i]->name[0] && clients[i]->state.state == CS_ALIVE)
			alive.add(clients[i]);

		if(dueltime)
		{
			if(gamemillis >= dueltime)
			{
				loopvj(clients) if(clients[j]->name[0] && clients[j]->state.state != CS_ALIVE && clients[j]->state.state != CS_SPECTATOR)
					queue(clients[j], m_duel(gamemode, mutators));

				if(m_lms(gamemode, mutators) || alive.length() < 2) // while waiting for next round our two guys spawn
				{
					loopv(duelqueue)
					{
						if(m_duel(gamemode, mutators) && alive.length() >= 2) break;
						if(clients.inrange(duelqueue[i]))
						{
							clientinfo *ci = clients[duelqueue[i]];
							if(ci->state.state != CS_ALIVE)
							{
								ci->state.state = CS_ALIVE;
								ci->state.respawn(gamemillis);
								sendspawn(ci);
								alive.add(ci);
							}
						}
					}
					cleanup();
				}

				if(m_duel(gamemode, mutators) && alive.length() > 2)
				{
					loopvrev(alive)
					{
						queue(alive[i], m_duel(gamemode, mutators));
						alive.remove(i);
						if(alive.length() <= 2) break;
					}
				}

				clearitems();
				duelround++;
				if(m_duel(gamemode, mutators))
				{
					s_sprintfd(namea)("%s", colorname(alive[0]));
					s_sprintfd(nameb)("%s", colorname(alive[1]));
					s_sprintfd(fight)("Duel between %s and %s, round \fs\fy#%d\fS.. FIGHT!", namea, nameb, duelround);
					sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);
				}
				else if(m_lms(gamemode, mutators))
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
				srvoutf(-1, "\fy%s was the last one left alive", colorname(alive[0]));
				sendf(alive[0]->clientnum, 1, "ri2s", SV_ANNOUNCE, S_V_YOUWIN, "you win!");
				alive[0]->state.health = MAXHEALTH;
				alive[0]->state.lastregen = gamemillis;
				sendf(-1, 1, "ri3", SV_REGEN, alive[0]->clientnum, alive[0]->state.health);
			}
			else srvoutf(-1, "\fyeveryone died, fail!");
			dueltime = gamemillis + DUELMILLIS;
		}
	}

	void reset(bool empty)
	{
		duelround = 0;
		dueltime = gamemillis + DUELMILLIS;
		duelqueue.setsize(0);
		loopv(clients) if(clients[i]->name[0] && clients[i]->state.state != CS_SPECTATOR)
			queue(clients[i], m_duel(gamemode, mutators));
		clearitems();
	}
} duelmutator;
#endif
