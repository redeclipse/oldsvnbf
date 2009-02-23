#ifdef GAMESERVER
struct duelservmode : servmode
{
	int duelround, dueltime;
	vector<int> duelqueue, allowed;

	duelservmode() {}

	void queue(clientinfo *ci, bool top = false, bool wait = true)
	{
		if(ci->online && ci->state.state != CS_SPECTATOR && ci->state.state != CS_EDITING)
		{
			int n = duelqueue.find(ci->clientnum);
			if(n < 0)
			{
				if(top) duelqueue.insert(0, ci->clientnum);
				else duelqueue.add(ci->clientnum);
				n = duelqueue.find(ci->clientnum);
			}
			if(wait)
			{
				if(ci->state.state != CS_WAITING) waiting(ci);
				if(m_duel(gamemode, mutators) && allowbroadcast(ci->clientnum))
				{
					if(n > 1) srvmsgf(ci->clientnum, "you are \fs\fg#%d\fS in the queue", n);
					else if(n) srvmsgf(ci->clientnum, "you are \fs\fgNEXT\fS in the queue");
				}
			}
		}
	}

	void entergame(clientinfo *ci) { queue(ci, false, true); }
	void leavegame(clientinfo *ci)
	{
        duelqueue.removeobj(ci->clientnum);
        allowed.removeobj(ci->clientnum);
	}

	bool damage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush)
	{
		if(dueltime) return false;
		return true;
	}

	bool canspawn(clientinfo *ci, bool tryspawn = false)
	{
        if(allowed.find(ci->clientnum) >= 0) return true;
		if(tryspawn) queue(ci, false, false);
		return false; // you spawn when we want you to buddy
	}

    void spawned(clientinfo *ci)
    {
        allowed.removeobj(ci->clientnum);
    }

	void died(clientinfo *ci, clientinfo *at) {}

	void clearitems()
	{
		if(!m_noitems(gamemode, mutators))
		{
			loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM && !finditem(i, true, false))
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

	void clearqueue()
	{
		clearitems();
		duelqueue.setsize(0);
		loopv(clients) queue(clients[i], false, true);
        allowed.setsize(0);
	}

	void cleanup()
	{
		loopvrev(duelqueue)
			if(!clients.inrange(duelqueue[i]) || (clients[duelqueue[i]]->state.state != CS_DEAD && clients[duelqueue[i]]->state.state != CS_WAITING))
				duelqueue.remove(i);
	}

	void update()
	{
		if(interm || !numclients() || notgotinfo) return;

		if(dueltime < 0)
		{
			dueltime = gamemillis+(GVAR(duellimit)*1000);
			clearqueue();
		}
		else cleanup();

		if(dueltime)
		{
			if(gamemillis >= dueltime)
			{
				vector<clientinfo *> alive;
				loopv(clients) queue(clients[i], clients[i]->state.state == CS_ALIVE, clients[i]->state.state != CS_ALIVE);
				if(m_lms(gamemode, mutators) || GVAR(duelclear)) clearitems();
                allowed.setsize(0);
				loopv(duelqueue)
				{
					if(m_duel(gamemode, mutators) && alive.length() >= 2) break;
					if(clients.inrange(duelqueue[i]))
					{
						clientinfo *ci = clients[duelqueue[i]];
						if(ci->state.state != CS_ALIVE)
						{
                            if(allowed.find(ci->clientnum) < 0) allowed.add(ci->clientnum);
                            if(ci->state.state != CS_WAITING) waiting(ci);
						}
						else
						{
							ci->state.health = m_maxhealth(gamemode, mutators);
							ci->state.lastregen = gamemillis;
							sendf(-1, 1, "ri3", SV_REGEN, ci->clientnum, ci->state.health);
						}
						alive.add(ci);
					}
				}
				cleanup();
				duelround++;
				if(m_duel(gamemode, mutators))
				{
					s_sprintfd(namea)("%s", colorname(alive[0]));
					s_sprintfd(nameb)("%s", colorname(alive[1]));
					s_sprintfd(fight)("\fyduel between %s and %s, round \fs\fr#%d\fS.. FIGHT!", namea, nameb, duelround);
					sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);
				}
				else if(m_lms(gamemode, mutators))
				{
					s_sprintfd(fight)("\fylast one left alive wins, round \fs\fr#%d\fS.. FIGHT!", duelround);
					sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, fight);
				}
				dueltime = 0;
			}
		}
		else if(allowed.empty())
		{
			vector<clientinfo *> alive;
			loopv(clients) if(clients[i]->state.state == CS_ALIVE) alive.add(clients[i]);
			switch(alive.length())
			{
				case 0:
				{
					srvmsgf(-1, "\frhaha, everyone died, fail!");
					dueltime = gamemillis+(GVAR(duellimit)*1000);
					break;
				}
				case 1:
				{
					srvmsgf(-1, "\fy%s was the last one left alive", colorname(alive[0]));
					if(allowbroadcast(alive[0]->clientnum))
						sendf(alive[0]->clientnum, 1, "ri2s", SV_ANNOUNCE, S_V_YOUWIN, "\fgyou survived, yay you!");
					dueltime = gamemillis+(GVAR(duellimit)*1000);
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
        allowed.setsize(0);
	}
} duelmutator;
#endif
