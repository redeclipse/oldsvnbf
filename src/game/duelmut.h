#ifdef GAMESERVER
struct duelservmode : servmode
{
	int duelround, dueltime;
	vector<clientinfo *> duelqueue, allowed, playing;

	duelservmode() {}

	void position(clientinfo *ci, bool clean)
	{
		if(allowbroadcast(ci->clientnum))
		{
			int n = duelqueue.find(ci);
			if(n >= 0)
			{
				if(clean)
				{
					n -= GVAR(duelreset) ? 2 : 1;
					if(n < 0) return;
				}
				if(m_survivor(gamemode, mutators)) srvmsgf(ci->clientnum, "\fayou are \fs\fgqueued\fS for the next round");
				else
				{
					if(m_team(gamemode, mutators) || n > 1) srvmsgf(ci->clientnum, "\fayou are \fs\fg#%d\fS in the queue", n+(m_team(gamemode, mutators) ? 1 : 0));
					else if(n) srvmsgf(ci->clientnum, "\fayou are \fs\fgNEXT\fS in the queue");
				}
			}
		}
	}

	void queue(clientinfo *ci, bool top = false, bool wait = true, bool clean = false)
	{
		if(ci->online && ci->state.state != CS_SPECTATOR && ci->state.state != CS_EDITING)
		{
			int n = duelqueue.find(ci);
			if(top)
			{
				if(n >= 0) duelqueue.remove(n);
				duelqueue.insert(0, ci);
			}
			else if(n < 0) duelqueue.add(ci);
			if(wait && ci->state.state != CS_WAITING) waiting(ci, 1, 1);
			if(!clean) position(ci, false);
		}
	}

	void entergame(clientinfo *ci) { queue(ci); }
	void leavegame(clientinfo *ci, bool disconnecting = false)
	{
		duelqueue.removeobj(ci);
		allowed.removeobj(ci);
		playing.removeobj(ci);
	}

	bool damage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush)
	{
		if(dueltime) return false;
		return true;
	}

	bool canspawn(clientinfo *ci, bool tryspawn = false)
	{
		if(allowed.find(ci) >= 0) return true;
		if(tryspawn) queue(ci, false, duelround > 0 || duelqueue.length() > 1);
		return false; // you spawn when we want you to buddy
	}

	void spawned(clientinfo *ci) { allowed.removeobj(ci); }

	void died(clientinfo *ci, clientinfo *at) {}

	void clearitems()
	{
		if(!m_noitems(gamemode, mutators))
		{
			loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM && !finditem(i, true, false))
			{
				if(m_arena(gamemode, mutators) && sents[i].type == WEAPON && sents[i].attrs[0] != WEAP_GRENADE)
					continue;
				loopvk(clients)
				{
					clientinfo *ci = clients[k];
					ci->state.dropped.remove(i);
					loopj(WEAP_MAX) if(ci->state.entid[j] == i)
						ci->state.entid[j] = -1;
				}
				sents[i].millis = gamemillis; // hijack its spawn time
				sents[i].spawned = true;
				sendf(-1, 1, "ri2", SV_ITEMSPAWN, i);
			}
		}
	}

	void cleanup()
	{
		loopvrev(duelqueue)
			if(duelqueue[i]->state.state != CS_DEAD && duelqueue[i]->state.state != CS_WAITING)
				duelqueue.remove(i);
	}

	void clear()
	{
		dueltime = gamemillis+GVAR(duellimit);
		playing.setsize(0);
	}

	void update()
	{
		if(interm || !numclients() || !hasgameinfo) return;

		if(dueltime < 0)
		{
			if(duelqueue.length() >= 2)
			{
				clearitems();
				dueltime = gamemillis+GVAR(duellimit);
			}
			else
			{
				loopv(clients) queue(clients[i]); // safety
				return;
			}
		}
		else cleanup();

		if(dueltime)
		{
			if(gamemillis >= dueltime)
			{
				vector<clientinfo *> alive;
				loopv(clients) queue(clients[i], clients[i]->state.state == CS_ALIVE, GVAR(duelreset) || clients[i]->state.state != CS_ALIVE, true);
				allowed.setsize(0); playing.setsize(0);
				if(!duelqueue.empty())
				{
					if(m_survivor(gamemode, mutators) || GVAR(duelclear)) clearitems();
					loopv(clients) position(clients[i], true);
					loopv(duelqueue)
					{
						if(m_duel(gamemode, mutators) && alive.length() >= 2) break;
						clientinfo *ci = duelqueue[i];
						if(ci->state.state != CS_ALIVE)
						{
							if(ci->state.state != CS_WAITING) waiting(ci, 1, 1);
							if(m_duel(gamemode, mutators) && m_team(gamemode, mutators))
							{
								bool skip = false;
								loopv(alive) if(ci->team == alive[i]->team) { skip = true; break; }
								if(skip) continue;
							}
							if(allowed.find(ci) < 0) allowed.add(ci);
						}
						else
						{
							ci->state.health = m_health(gamemode, mutators);
							ci->state.lastregen = gamemillis;
							ci->state.lastfire = ci->state.lastfireburn = 0;
							sendf(-1, 1, "ri4", SV_REGEN, ci->clientnum, ci->state.health, 0); // amt = 0 regens impulse
						}
						alive.add(ci);
						playing.add(ci);
					}
					duelround++;
					if(m_duel(gamemode, mutators))
					{
						defformatstring(namea)("%s", colorname(alive[0]));
						defformatstring(nameb)("%s", colorname(alive[1]));
						defformatstring(fight)("\faduel between %s and %s, round \fs\fr#%d\fS", namea, nameb, duelround);
						sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_V_FIGHT, CON_MESG, fight);
					}
					else if(m_survivor(gamemode, mutators))
					{
						defformatstring(fight)("\falast one left alive wins, round \fs\fr#%d\fS", duelround);
						sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_V_FIGHT, CON_MESG, fight);
					}
					dueltime = 0;
				}
			}
		}
		else if(allowed.empty())
		{
			vector<clientinfo *> alive;
			loopv(clients) if(clients[i]->state.state == CS_ALIVE) alive.add(clients[i]);
			if(m_survivor(gamemode, mutators) && m_team(gamemode, mutators) && !alive.empty())
			{
				bool found = false;
				loopv(alive) if(i && alive[i]->team != alive[i-1]->team) { found = true; break; }
				if(!found)
				{
					srvmsgf(-1, "\fateam \fs%s%s\fS are the victors", teamtype[alive[0]->team].chat, teamtype[alive[0]->team].name);
					loopv(playing) if(allowbroadcast(playing[i]->clientnum))
					{
						if(playing[i]->team == alive[0]->team)
						{
							sendf(playing[i]->clientnum, 1, "ri3s", SV_ANNOUNCE, S_V_YOUWIN, -1, "");
							givepoints(playing[i], playing.length());
						}
						else sendf(playing[i]->clientnum, 1, "ri3s", SV_ANNOUNCE, S_V_YOULOSE, -1, "");
					}
					clear();
				}
			}
			else switch(alive.length())
			{
				case 0:
				{
					srvmsgf(-1, "\faeveryone died, epic fail");
					loopv(playing) if(allowbroadcast(playing[i]->clientnum))
						sendf(playing[i]->clientnum, 1, "ri3s", SV_ANNOUNCE, S_V_YOULOSE, -1, "");
					clear();
					break;
				}
				case 1:
				{
					srvmsgf(-1, "\fa%s was the victor", colorname(alive[0]));
					loopv(playing) if(allowbroadcast(playing[i]->clientnum))
					{
						if(playing[i] == alive[0])
						{
							sendf(playing[i]->clientnum, 1, "ri3s", SV_ANNOUNCE, S_V_YOUWIN, -1, "");
							givepoints(playing[i], playing.length());
						}
						else sendf(playing[i]->clientnum, 1, "ri3s", SV_ANNOUNCE, S_V_YOULOSE, -1, "");
					}
					clear();
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
		duelqueue.setsize(0);
		playing.setsize(0);
	}
} duelmutator;
#endif
