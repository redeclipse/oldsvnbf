// server-side ai manager
namespace aiman
{
	int oldteambalance = -1, oldbotbalance = -1, oldbotminskill = -1, oldbotmaxskill = -1, oldbotlimit = -1;

	int findaiclient(int exclude)
	{
		vector<int> siblings;
		while(siblings.length() < clients.length()) siblings.add(-1);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->clientnum < 0 || ci->state.aitype >= 0 || !ci->name[0] || !ci->connected || ci->clientnum == exclude)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(clients) if(clients[j]->state.aitype >= 0 && clients[j]->state.ownernum == ci->clientnum)
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

	bool addai(int type, int ent, int skill, bool req)
	{
		int numbots = 0;
		loopv(clients)
		{
			if(type == AI_BOT && numbots >= GVAR(botlimit)) return false;
			if(clients[i]->state.aitype == type)
			{
				clientinfo *ci = clients[i];
				if(ci->state.ownernum < 0)
				{ // reuse a slot that was going to removed
					ci->state.ownernum = findaiclient();
					ci->state.aireinit = 1;
					ci->state.aitype = type;
					ci->state.aientity = ent;
					if(req) autooverride = true;
					return true;
				}
				if(type == AI_BOT) numbots++;
			}
		}
		if(type == AI_BOT && numbots >= GVAR(botlimit)) return false;
		int cn = addclient(ST_REMOTE);
		if(cn >= 0)
		{
			clientinfo *ci = (clientinfo *)getinfo(cn);
			if(ci)
			{
				int s = skill, m = int(max(GVAR(botmaxskill), GVAR(botminskill))*aitype[type].skill), n = int(min(GVAR(botminskill), m)*aitype[type].skill);
				if(skill > m || skill < n) s = (m != n ? rnd(m-n) + n + 1 : m);
				ci->clientnum = cn;
				ci->state.ownernum = findaiclient();
				ci->state.aireinit = 2;
				ci->state.aitype = type;
				ci->state.aientity = ent;
				ci->state.skill = clamp(s, 1, 101);
				clients.add(ci);
				ci->state.lasttimeplayed = lastmillis;
				copystring(ci->name, aitype[ci->state.aitype].name, MAXNAMELEN);
				ci->state.state = CS_DEAD;
				ci->team = TEAM_NEUTRAL;
				ci->online = ci->connected = true;
				if(req) autooverride = true;
				return true;
			}
			delclient(cn);
		}
		return false;
	}

	void deleteai(clientinfo *ci)
	{
		int cn = ci->clientnum;
		if(smode) smode->leavegame(ci, true);
		mutate(smuts, mut->leavegame(ci, true));
		ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		savescore(ci);
		dropitems(ci, true);
		sendf(-1, 1, "ri2", SV_DISCONNECT, cn);
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
			if(autooverride) dorefresh = true;
			autooverride = false;
			return true;
		}
		return false;
	}

	void reinitai(clientinfo *ci)
	{
		if(ci->state.ownernum < 0) deleteai(ci);
		else if(ci->state.aireinit >= 1)
		{
			ci->state.dropped.reset();
			loopk(WEAP_MAX) loopj(2) ci->state.weapshots[k][j].reset();
			sendf(-1, 1, "ri6si", SV_INITAI, ci->clientnum, ci->state.ownernum, ci->state.aitype, ci->state.aientity, ci->state.skill, ci->name, ci->team);
			if(ci->state.aireinit == 2)
			{
				waiting(ci, 2);
				if(smode) smode->entergame(ci);
				mutate(smuts, mut->entergame(ci));
			}
			ci->state.aireinit = 0;
		}
	}

	void shiftai(clientinfo *ci, int cn = -1)
	{
		if(cn < 0) { ci->state.aireinit = 0; ci->state.ownernum = -1; }
		else { ci->state.aireinit = 1; ci->state.ownernum = cn; }
	}

	void removeai(clientinfo *ci, bool complete)
	{ // either schedules a removal, or someone else to assign to
		loopv(clients) if(clients[i]->state.aitype >= 0 && clients[i]->state.ownernum == ci->clientnum)
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
			if(ci->clientnum < 0 || ci->state.aitype >= 0 || !ci->name[0] || !ci->connected || ci->clientnum == exclude)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(clients) if(clients[j]->state.aitype >= 0 && clients[j]->state.ownernum == ci->clientnum)
					siblings[i]++;
				if(!siblings.inrange(hi) || siblings[i] > siblings[hi]) hi = i;
				if(!siblings.inrange(lo) || siblings[i] < siblings[lo]) lo = i;
			}
		}
		if(siblings.inrange(hi) && siblings.inrange(lo) && (siblings[hi]-siblings[lo]) > 1)
		{
			clientinfo *ci = clients[hi];
			loopv(clients) if(clients[i]->state.aitype >= 0 && clients[i]->state.ownernum == ci->clientnum)
			{
				shiftai(clients[i], clients[lo]->clientnum);
				return true;
			}
		}
		return false;
	}

	void checksetup()
	{
		int m = max(GVAR(botmaxskill), GVAR(botminskill)), n = min(GVAR(botminskill), m), numbots = 0;
		loopv(clients) if(clients[i]->state.aitype >= 0 && clients[i]->state.ownernum >= 0)
		{
			clientinfo *ci = clients[i];
			int o = int(m*aitype[ci->state.aitype].skill), p = int(n*aitype[ci->state.aitype].skill);
			if(ci->state.skill > o || ci->state.skill < o)
			{ // needs re-skilling
				ci->state.skill = (o != p ? rnd(o-p) + p + 1 : o);
				if(!ci->state.aireinit) ci->state.aireinit = 1;
			}
			if(ci->state.aitype == AI_BOT)
			{
				numbots++;
				if(numbots >= GVAR(botlimit)) shiftai(ci, -1);
			}
		}
		loopv(sents) if(sents[i].type == ACTOR && sents[i].attrs[0] >= AI_START && sents[i].attrs[0] < AI_MAX && (sents[i].attrs[4] == triggerid || !sents[i].attrs[4]) && chkmode(sents[i].attrs[3], gamemode))
		{
			bool needent = true;
			loopvk(clients) if(clients[k]->state.aientity == i) { needent = false; break; }
			if(needent) addai(sents[i].attrs[0], i, -1, false);
		}
		int balance = 0;
		if(m_story(gamemode)) balance = nplayers; // story mode strictly obeys nplayers
		else if(m_fight(gamemode) && !m_trial(gamemode) && GVAR(botlimit) > 0)
		{
			int numt = numteams(gamemode, mutators), people = numclients(-1, true, -1);
			switch(GVAR(botbalance))
			{
				case -1: balance = 0; break; // no bots
				case  0: balance = max(people, nplayers); break; // use scaled numplayers
				default: balance = max(people, GVAR(botbalance)*numt); break; // balance to at least this*numteams
			}
			if(m_team(gamemode, mutators) && (balance > 0 || GVAR(teambalance) == 3))
			{ // skew this if teams are unbalanced
				if(!autooverride)
				{
					if(GVAR(teambalance) != 3)
					{
						int offt = balance%numt; // balance so all teams have even counts
						if(offt > 0) balance += numt-offt;
					}
					else balance = max(people*numt, numt); // humans vs. bots, just directly balance
				}
				loopvrev(clients)
				{
					clientinfo *ci = clients[i];
					if(ci->state.aitype == AI_BOT && ci->state.ownernum >= 0)
					{
						if(!autooverride && numclients(-1, true, AI_BOT) > balance)
							shiftai(ci, -1); // temporarily remove and cleanup later
						else
						{
							int teamb = chooseteam(ci, ci->team);
							if(ci->team != teamb) setteam(ci, teamb, true, true);
						}
					}
				}
			}
			int bots = balance-people;
			if(bots > GVAR(botlimit))
			{
				balance -= bots-GVAR(botlimit);
				if(m_team(gamemode, mutators)) balance -= balance%numt;
			}
		}
		if(!autooverride)
		{
			if(balance > 0)
			{
				while(numclients(-1, true, AI_BOT) < balance) if(!addai(AI_BOT, -1, -1)) break;
				while(numclients(-1, true, AI_BOT) > balance) if(!delai(AI_BOT)) break;
			}
			else clearai();
		}
	}

	void clearai(int type)
	{ // clear and remove all ai immediately
		loopvrev(clients) if(clients[i]->state.aitype >= type) deleteai(clients[i]);
		dorefresh = autooverride = false;
	}

	void checkai()
	{
		if(!m_demo(gamemode) && !m_lobby(gamemode) && numclients())
		{
			if(hasgameinfo)
			{
				#define checkold(n) if(old##n != GVAR(n)) { dorefresh = true; old##n = GVAR(n); }
				checkold(teambalance); checkold(botbalance); checkold(botminskill); checkold(botmaxskill); checkold(botlimit);
				if(dorefresh) { checksetup(); dorefresh = false; }
				loopvrev(clients) if(clients[i]->state.aitype >= 0) reinitai(clients[i]);
				while(true) if(!reassignai()) break;
			}
		}
		else clearai();
	}

	void reqadd(clientinfo *ci, int skill)
	{
		if(haspriv(ci, PRIV_MASTER, "add bots"))
		{
			if(m_lobby(gamemode) || m_story(gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else if(!addai(AI_BOT, -1, skill, true)) srvmsgf(ci->clientnum, "failed to create or assign bot");
		}
	}

	void reqdel(clientinfo *ci)
	{
		if(haspriv(ci, PRIV_MASTER, "remove bots"))
		{
			if(m_lobby(gamemode) || m_story(gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else if(!delai(AI_BOT, true)) srvmsgf(ci->clientnum, "failed to remove any bots");
		}
	}
}
