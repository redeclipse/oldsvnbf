#ifdef GAMESERVER
struct duelservmode : servmode
{
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
				n = duelqueue.find(ci->clientnum);
			}
			if(ci->state.state != CS_WAITING)
			{
				sendf(-1, 1, "ri2", SV_WAITING, ci->clientnum);
				ci->state.state = CS_WAITING;
				ci->state.weapreset(false);
			}
			if(msg)
			{
				if(n > 0) srvmsgf(ci->clientnum, "\fyyou are \fs\fg#%d\fS in the queue", n+1);
				else srvmsgf(ci->clientnum, "\fyyou are \fs\fgNEXT\fS in the queue", n+1);
			}
		}
	}

	void entergame(clientinfo *ci) { queue(ci, m_duel(gamemode, mutators)); }
	void leavegame(clientinfo *ci)
	{
		int n = duelqueue.find(ci->clientnum);
		if(n >= 0) duelqueue.remove(n);
	}

	bool damage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush)
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
		if(!m_noitems(gamemode, mutators))
		{
			loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM && !finditem(i, true, 0))
			{
				loopvk(clients)
				{
					clientinfo *ci = clients[k];
					ci->state.dropped.remove(i);
					loopj(WEAPON_MAX) if(ci->state.entid[j] == i)
						ci->state.entid[j] = -1;
				}
				sents[i].millis = gamemillis; // hijack its spawn time
				sents[i].spawned = true;
				sendf(-1, 1, "ri2", SV_ITEMSPAWN, i);
			}
		}
	}

	void clearqueue(bool init = false)
	{
		duelqueue.setsize(0);
		loopv(clients) if(clients[i]->name[0] && clients[i]->state.state != CS_SPECTATOR)
			queue(clients[i], !init && m_duel(gamemode, mutators), true);
	}

	void cleanup()
	{
		loopvrev(duelqueue)
			if(!clients.inrange(duelqueue[i]) || !clients[duelqueue[i]]->name[0] || clients[duelqueue[i]]->state.state == CS_SPECTATOR || clients[duelqueue[i]]->state.state == CS_ALIVE)
				duelqueue.remove(i);
	}

	void update()
	{
		if(interm || !numclients() || notgotinfo) return;
		if(dueltime < 0)
		{
			dueltime = gamemillis+(sv_duellimit*1000);
			clearqueue(true);
			clearitems();
		}
		cleanup();
		if(dueltime)
		{
			if(gamemillis >= dueltime)
			{
				loopv(clients) if(clients[i]->name[0] && clients[i]->state.state == CS_ALIVE) queue(clients[i], false, true);
				clearitems();
				vector<clientinfo *> alive;
				loopv(duelqueue)
				{
					if(m_duel(gamemode, mutators) && alive.length() >= 2) break;
					if(clients.inrange(duelqueue[i]))
					{
						clientinfo *ci = clients[duelqueue[i]];
						ci->state.state = CS_ALIVE;
						ci->state.respawn(gamemillis, m_maxhealth(gamemode, mutators));
						sendspawn(ci);
						alive.add(ci);
					}
				}
				cleanup();

				duelround++;
				if(m_duel(gamemode, mutators))
				{
					s_sprintfd(namea)("%s", colorname(alive[0]));
					s_sprintfd(nameb)("%s", colorname(alive[1]));
					s_sprintfd(fight)("\faduel between %s and %s, round \fs\fy#%d\fS.. FIGHT!", namea, nameb, duelround);
					sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);
				}
				else if(m_lms(gamemode, mutators))
				{
					s_sprintfd(fight)("\falast one left alive wins, round \fs\fy#%d\fS.. FIGHT!", duelround);
					sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);
				}
				dueltime = 0;
			}
		}
		else
		{
			vector<clientinfo *> alive;
			loopv(clients) if(clients[i]->name[0] && clients[i]->state.state == CS_ALIVE) alive.add(clients[i]);
			switch(alive.length())
			{
				case 0:
				{
					srvmsgf(-1, "\fyeveryone died, fail!");
					dueltime = gamemillis+(sv_duellimit*1000);
					break;
				}
				case 1:
				{
					srvmsgf(-1, "\fy%s was the last one left alive", colorname(alive[0]));
					sendf(alive[0]->clientnum, 1, "ri2s", SV_ANNOUNCE, S_V_YOUWIN, "\fayou survived!");
					dueltime = gamemillis+(sv_duellimit*1000);
					break;
				}
				default: break;
			}
		}
	}

	void reset(bool empty)
	{
		duelround = 0;
		dueltime = -1;
		cleanup();
	}
} duelmutator;
#endif
