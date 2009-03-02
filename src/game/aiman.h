// server-side ai manager
namespace aiman
{
	int oldteambalance = -1;
	float oldbotratio = 1e-4f; // lower than it can go

	int findaiclient(int exclude)
	{
		vector<int> siblings;
		while(siblings.length() < clients.length()) siblings.add(-1);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->clientnum < 0 || ci->state.aitype != AI_NONE || !ci->name[0] || !ci->connected || ci->clientnum == exclude)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(clients) if(clients[j]->state.aitype != AI_NONE && clients[j]->state.ownernum == ci->clientnum)
					siblings[i]++;
			}
		}
		while(!siblings.empty())
		{
			int q = -1;
			loopv(siblings)
				if(siblings[i] >= 0 && (!siblings.inrange(q) || siblings[i] < siblings[q]))
					q = i;
			if(siblings.inrange(q))
			{
				if(clients.inrange(q)) return clients[q]->clientnum;
				else siblings.removeunordered(q);
			}
			else break;
		}
		return -1;
	}

	bool addai(int type, int skill, bool req)
	{
		int numai = 0;
		loopv(clients) if(clients[i]->state.aitype == type)
		{
			if(clients[i]->state.ownernum < 0)
			{ // reuse a slot that was going to removed
				clientinfo *ci = clients[i];
				ci->state.ownernum = findaiclient();
				ci->state.aireinit = 2;
				ci->team = chooseteam(ci);
				if(req) autooverride = true;
				dorefresh = true;
				return true;
			}
			numai++;
		}
		int cn = numai < GVAR(botlimit) ? addclient(ST_REMOTE) : -1;
		if(cn >= 0)
		{
			clientinfo *ci = (clientinfo *)getinfo(cn);
			if(ci)
			{
				int s = skill, m = max(GVAR(botmaxskill), GVAR(botminskill)), n = min(GVAR(botminskill), m);
				if(skill > m || skill < n) s = (m != n ? rnd(m-n) + n + 1 : m);
				ci->clientnum = cn;
				ci->state.aitype = type;
				ci->state.ownernum = findaiclient();
				ci->state.skill = clamp(s, 1, 101);
				clients.add(ci);
				ci->state.lasttimeplayed = lastmillis;
				s_strncpy(ci->name, aitype[ci->state.aitype].name, MAXNAMELEN);
				ci->state.state = CS_DEAD;
				ci->team = chooseteam(ci);
				sendf(-1, 1, "ri5si", SV_INITAI, ci->clientnum, ci->state.ownernum, ci->state.aitype, ci->state.skill, ci->name, ci->team);
				waiting(ci);
				ci->state.aireinit = 0;
				ci->online = ci->connected = true;
				if(req) autooverride = true;
				dorefresh = true;
				return true;
			}
			delclient(cn);
		}
		return false;
	}

	void refreshai()
	{
		loopv(clients) if(clients[i]->state.aitype != AI_NONE && clients[i]->state.ownernum >= 0)
		{
			clientinfo *ci = clients[i];
			setteam(ci, chooseteam(ci, ci->team), true, true);
		}
	}

	void deleteai(clientinfo *ci)
	{
		int cn = ci->clientnum;
		if(smode) smode->leavegame(ci, true);
		mutate(smuts, mut->leavegame(ci, true));
		ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		savescore(ci);
		dropitems(ci, true);
		sendf(-1, 1, "ri2", SV_CDIS, cn);
		clients.removeobj(ci);
		delclient(cn);
		dorefresh = true;
	}

	bool delai(int type, bool req)
	{
		loopvrev(clients) if(clients[i]->state.aitype == type && clients[i]->state.ownernum >= 0)
		{
			deleteai(clients[i]);
			if(req) autooverride = true;
			return true;
		}
		if(req)
		{
			autooverride = false;
			return true;
		}
		return false;
	}

	void reinitai(clientinfo *ci)
	{
		if(ci->state.ownernum < 0) deleteai(ci);
		else if(ci->state.aireinit)
		{
			if(ci->state.aireinit >= 2)
			{
				waiting(ci);
				ci->state.dropped.reset();
				loopk(WEAPON_MAX) ci->state.weapshots[k].reset();
			}
			sendf(-1, 1, "ri5si", SV_INITAI, ci->clientnum, ci->state.ownernum, ci->state.aitype, ci->state.skill, ci->name, ci->team);
			ci->state.aireinit = 0;
		}
	}

	void shiftai(clientinfo *ci, int cn = -1)
	{
		if(cn < 0)
		{
			ci->state.dropped.reset();
            loopi(WEAPON_MAX) ci->state.weapshots[i].reset();
			ci->state.ownernum = -1;
			ci->state.aireinit = 0;
			ci->team = TEAM_NEUTRAL;
		}
		else
		{
			ci->state.aireinit = 2;
			ci->state.ownernum = cn;
		}
	}

	void removeai(clientinfo *ci, bool complete)
	{ // either schedules a removal, or someone else to assign to
		loopv(clients) if(clients[i]->state.aitype != AI_NONE && clients[i]->state.ownernum == ci->clientnum)
			shiftai(clients[i], complete ? -1 : findaiclient(ci->clientnum));
	}

	bool reassignai(int exclude)
	{
		vector<int> siblings;
		while(siblings.length() < clients.length()) siblings.add(-1);
		int hi = -1, lo = -1;
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->clientnum < 0 || ci->state.aitype != AI_NONE || !ci->name[0] || !ci->connected || ci->clientnum == exclude)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(clients) if(clients[j]->state.aitype != AI_NONE && clients[j]->state.ownernum == ci->clientnum)
					siblings[i]++;
				if(!siblings.inrange(hi) || siblings[i] > siblings[hi]) hi = i;
				if(!siblings.inrange(lo) || siblings[i] < siblings[lo]) lo = i;
			}
		}
		if(siblings.inrange(hi) && siblings.inrange(lo) && (siblings[hi]-siblings[lo]) > 1)
		{
			clientinfo *ci = clients[hi];
			loopv(clients) if(clients[i]->state.aitype != AI_NONE && clients[i]->state.ownernum == ci->clientnum)
			{
				shiftai(clients[i], clients[lo]->clientnum);
				return true;
			}
		}
		return false;
	}

	void checksetup()
	{
		int m = max(GVAR(botmaxskill), GVAR(botminskill)), n = min(GVAR(botminskill), m);
		loopv(clients) if(clients[i]->state.aitype != AI_NONE && clients[i]->state.ownernum >= 0 && !clients[i]->state.aireinit)
		{
			clientinfo *ci = clients[i];
			if(ci->state.skill > m || ci->state.skill < n)
			{ // needs re-skilling
				ci->state.skill = (m != n ? rnd(m-n) + n + 1 : m);
				ci->state.aireinit = 1;
			}
		}
		loopvrev(clients) if(clients[i]->state.aitype != AI_NONE) reinitai(clients[i]);
		if(dorefresh)
		{
			refreshai();
			dorefresh = false;
		}
	}

	void clearai(bool override)
	{ // clear and remove all ai immediately
		loopvrev(clients) if(clients[i]->state.aitype != AI_NONE)
			deleteai(clients[i]);
		autooverride = override;
		dorefresh = false;
	}

	void checkai()
	{
		if(!notgotinfo && !m_demo(gamemode) && !m_lobby(gamemode) && numclients(-1, false, true))
		{
			if(oldteambalance != GVAR(teambalance))
			{
				dorefresh = true;
				oldteambalance = GVAR(teambalance);
			}
			if(oldbotratio != GVAR(botratio))
			{
				dorefresh = true;
				oldbotratio = GVAR(botratio);
			}
			if(m_fight(gamemode) && !autooverride)
			{
				int balance = int(numplayers*GVAR(botscale));
				if(m_team(gamemode, mutators))
				{ // skew this if teams are unbalanced
					int numt = numteams(gamemode, mutators);
					if(GVAR(teambalance) < 6)
					{ // balance so all teams have even counts
						int teamcount[TEAM_NUM] = { 0, 0, 0, 0 }, highest = TEAM_NEUTRAL;
						loopv(clients)
						{
							clientinfo *cp = clients[i];
							if(!cp->team || cp->state.state == CS_SPECTATOR || cp->state.state == CS_EDITING) continue;
							int idx = cp->team-TEAM_FIRST;
							teamcount[idx]++;
							if(!highest || teamcount[idx] > teamcount[highest])
								highest = idx;
						}
						loopi(numt) if(teamcount[highest] > teamcount[i])
							balance += teamcount[highest]-teamcount[i];
					}
					else balance = max(numclients(-1, true, true)*numt, numt-1); // humans vs. bots, just directly balance
				}
				while(numclients(-1, true, false) < balance) if(!addai(AI_BOT, -1)) break;
				while(numclients(-1, true, false) > balance) if(!delai(AI_BOT)) break;
			}
			while(true) if(!reassignai()) break;
			checksetup();
		}
		else clearai();
	}

	void reqadd(clientinfo *ci, int skill)
	{
		if(haspriv(ci, PRIV_MASTER, true))
		{
			if(m_lobby(gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else if(!addai(AI_BOT, skill, true)) srvmsgf(ci->clientnum, "failed to create or assign bot");
		}
	}

	void reqdel(clientinfo *ci)
	{
		if(haspriv(ci, PRIV_MASTER, true))
		{
			if(m_lobby(gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else if(!delai(AI_BOT, true)) srvmsgf(ci->clientnum, "failed to remove any bots");
		}
	}
}
