#define GAMESERVER 1
#include "game.h"

namespace server
{
	struct srventity
	{
		int type;
		bool spawned;
		int millis;
		vector<int> attrs, kin;

		srventity() : type(NOTUSED), spawned(false), millis(0) { reset(); }
		~srventity() { reset(); }

		void reset()
		{
			attrs.setsize(0);
			kin.setsize(0);
		}
	};

    static const int DEATHMILLIS = 300;

    struct clientinfo;

    struct gameevent
    {
        virtual ~gameevent() {}
        virtual bool flush(clientinfo *ci, int fmillis);
        virtual void process(clientinfo *ci) {}
        virtual bool keepable() const { return false; }
    };

    struct timedevent : gameevent
    {
        int millis;
        bool flush(clientinfo *ci, int fmillis);
    };

	struct shotevent : timedevent
	{
        int id, weap, flags, power, num;
		ivec from;
		vector<ivec> shots;
        void process(clientinfo *ci);
	};

	struct switchevent : timedevent
	{
		int id, weap;
        void process(clientinfo *ci);
	};

	struct dropevent : timedevent
	{
		int id, weap;
        void process(clientinfo *ci);
	};

	struct reloadevent : timedevent
	{
		int id, weap;
        void process(clientinfo *ci);
	};

	struct hitset
	{
		int flags, target, id;
		union
		{
			int rays;
			int dist;
		};
		ivec dir;
	};

	struct destroyevent : timedevent
	{
        int id, weap, flags, radial;
		vector<hitset> hits;
        bool keepable() const { return true; }
        void process(clientinfo *ci);
	};

	struct suicideevent : gameevent
	{
		int flags;
        void process(clientinfo *ci);
	};

	struct useevent : timedevent
	{
		int id, ent;
        void process(clientinfo *ci);
	};

    struct projectilestate
    {
        vector<int> projs;
        projectilestate() { reset(); }
        void reset() { projs.setsize(0); }
        void add(int val)
        {
			projs.add(val);
        }
        bool remove(int val)
        {
            loopv(projs) if(projs[i]==val)
            {
                projs.remove(i);
                return true;
            }
            return false;
        }
        bool find(int val)
        {
            loopv(projs) if(projs[i]==val) return true;
            return false;
        }
    };

	extern int gamemode, mutators;
	struct servstate : gamestate
	{
		vec o;
		int state;
        projectilestate dropped, weapshots[WEAP_MAX][2];
		int score, frags, spree, rewards, flags, deaths, teamkills, shotdamage, damage;
		int lasttimeplayed, timeplayed, aireinit, lastfireburn, lastfireowner;
		vector<int> fraglog, fragmillis, cpnodes;

		servstate() : state(CS_SPECTATOR), aireinit(0), lastfireburn(0), lastfireowner(-1) {}

		bool isalive(int millis)
		{
			return state == CS_ALIVE || ((state == CS_DEAD || state == CS_WAITING) && millis-lastdeath <= DEATHMILLIS);
		}

		void reset(bool change = false)
		{
			if(state != CS_SPECTATOR) state = CS_DEAD;
			dropped.reset();
            loopi(WEAP_MAX) loopj(2) weapshots[i][j].reset();
			if(!change) score = timeplayed = 0;
			else gamestate::mapchange();
            frags = spree = rewards = flags = deaths = teamkills = shotdamage = damage = 0;
            fraglog.setsize(0); fragmillis.setsize(0); cpnodes.setsize(0);
			respawn(0, m_health(server::gamemode, server::mutators));
		}

		void respawn(int millis, int heal)
		{
			lastfireburn = 0; lastfireowner = -1;
			gamestate::respawn(millis, heal);
			o = vec(-1e10f, -1e10f, -1e10f);
		}
	};

	struct savedscore
	{
		uint ip;
		string name;
		int points, score, frags, spree, rewards, flags, timeplayed, deaths, teamkills, shotdamage, damage;

		void save(servstate &gs)
		{
			points = gs.points;
			score = gs.score;
			frags = gs.frags;
			spree = gs.spree;
			rewards = gs.rewards;
			flags = gs.flags;
            deaths = gs.deaths;
            teamkills = gs.teamkills;
            shotdamage = gs.shotdamage;
            damage = gs.damage;
			timeplayed = gs.timeplayed;
		}

		void restore(servstate &gs)
		{
			gs.points = points;
			gs.score = score;
			gs.frags = frags;
			gs.spree = spree;
			gs.rewards = rewards;
			gs.flags = flags;
            gs.deaths = deaths;
            gs.teamkills = teamkills;
            gs.shotdamage = shotdamage;
            gs.damage = damage;
			gs.timeplayed = timeplayed;
		}
	};

	struct votecount
	{
		char *map;
		int mode, muts, count;
		votecount() {}
		votecount(char *s, int n, int m) : map(s), mode(n), muts(m), count(0) {}
	};

	struct clientinfo
	{
		int clientnum, connectmillis, sessionid, ping, team;
		string name, mapvote;
		int modevote, mutsvote, lastvote;
		int privilege;
        bool connected, local, timesync, online, wantsmap;
        int gameoffset, lastevent;
		servstate state;
		vector<gameevent *> events;
		vector<uchar> position, messages;
        int posoff, msgoff, msglen;
        uint authreq;
        string authname;
		string clientmap;
		int mapcrc;
		bool warned;

		clientinfo() { reset(); }
        ~clientinfo() { events.deletecontentsp(); }

		void addevent(gameevent *e)
		{
            if(state.state==CS_SPECTATOR || events.length()>100) delete e;
			else events.add(e);
		}

		void mapchange(bool change = true)
		{
			mapvote[0] = 0;
			state.reset(change);
			events.deletecontentsp();
            timesync = false;
            lastevent = gameoffset = lastvote = 0;
			team = TEAM_NEUTRAL;
			clientmap[0] = '\0';
			mapcrc = 0;
			warned = false;
		}

		void reset()
		{
			ping = 0;
			name[0] = 0;
			privilege = PRIV_NONE;
            connected = local = online = wantsmap = false;
            authreq = 0;
			position.setsizenodelete(0);
			messages.setsizenodelete(0);
			mapchange(false);
		}

		int getmillis(int millis, int id)
		{
			if(!timesync)
			{
				timesync = true;
				gameoffset = millis-id;
				return millis;
			}
			return gameoffset+id;
		}
	};

	struct worldstate
	{
		int uses;
		vector<uchar> positions, messages;
	};

	struct ban
	{
		int time;
		uint ip;
	};

	namespace aiman {
		bool autooverride = false, dorefresh = false;
		extern int findaiclient(int exclude = -1);
		extern bool addai(int type, int ent, int skill, bool req = false);
		extern void deleteai(clientinfo *ci);
		extern bool delai(int type, bool req = false);
		extern void removeai(clientinfo *ci, bool complete = false);
		extern bool reassignai(int exclude = -1);
		extern void checkskills();
		extern void clearai(bool all = true);
		extern void checkai();
		extern void reqadd(clientinfo *ci, int skill);
		extern void reqdel(clientinfo *ci);
	}

	bool hasgameinfo = false;
	int gamemode = G_LOBBY, mutators = 0;
	int gamemillis = 0, gamelimit = 0;

	string smapname;
	int interm = 0, minremain = -1, oldtimelimit = -1;
	bool maprequest = false;
	enet_uint32 lastsend = 0;
	int mastermode = MM_OPEN, mastermask = MM_OPENSERV;
	bool masterupdate = false, mapsending = false, shouldcheckvotes = false;
	stream *mapdata[3] = { NULL, NULL, NULL };

    vector<uint> allowedips;
	vector<ban> bannedips;
	vector<clientinfo *> clients, connects;
	vector<worldstate *> worldstates;
	bool reliablemessages = false;

	struct demofile
	{
		string info;
		uchar *data;
		int len;
	};

	#define MAXDEMOS 5
	vector<demofile> demos;

	bool demonextmatch = false;
	stream *demotmp = NULL, *demorecord = NULL, *demoplayback = NULL;
	int nextplayback = 0, triggerid = 0;
	struct triggergrp
	{
		int id;
		vector<int> ents;
		triggergrp() { reset(); }
		void reset(int n = 0) { id = n; ents.setsize(0); }
	} triggers[TRIGGERIDS+1];

	struct servmode
	{
		servmode() {}
		virtual ~servmode() {}

		virtual void entergame(clientinfo *ci) {}
        virtual void leavegame(clientinfo *ci, bool disconnecting = false) {}

		virtual void moved(clientinfo *ci, const vec &oldpos, const vec &newpos) {}
		virtual bool canspawn(clientinfo *ci, bool tryspawn = false) { return true; }
		virtual void spawned(clientinfo *ci) {}
        virtual int points(clientinfo *victim, clientinfo *actor)
        {
            if(victim==actor || victim->team == actor->team) return -1;
            return 1;
        }
		virtual void died(clientinfo *victim, clientinfo *actor = NULL) {}
		virtual void changeteam(clientinfo *ci, int oldteam, int newteam) {}
		virtual void initclient(clientinfo *ci, packetbuf &p, bool connecting) {}
		virtual void update() {}
		virtual void reset(bool empty) {}
		virtual void intermission() {}
		virtual bool damage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush = ivec(0, 0, 0)) { return true; }
		virtual void regen(clientinfo *ci, int &total, int &amt, int &delay) {}
	};

	vector<srventity> sents;
	vector<savedscore> scores;
	servmode *smode;
	vector<servmode *> smuts;
	#define mutate(a,b) loopvk(a) { servmode *mut = a[k]; { b; } }

	SVAR(serverdesc, "");
	SVAR(servermotd, "");
	SVAR(serverpass, "");
    SVAR(adminpass, "");
    VARF(serveropen, 0, 1, 3, {
		switch(serveropen)
		{
			case 0: default: mastermask = MM_FREESERV; break;
			case 1: mastermask = MM_OPENSERV; break;
			case 2: mastermask = MM_COOPSERV; break;
			case 3: mastermask = MM_VETOSERV; break;
		}
	});

	VAR(modelimit, 0, G_LOBBY, G_MAX-1);
	VAR(modelock, 0, 3, 5); // 0 = off, 1 = master only (+1 admin only), 3 = master can only set limited mode and higher (+1 admin), 5 = no mode selection
	VAR(mapslock, 0, 2, 5); // 0 = off, 1 = master can select non-allow maps (+1 admin), 3 = master can select non-rotation maps (+1 admin), 5 = no map selection
	VAR(varslock, 0, 1, 2); // 0 = master, 1 = admin only, 2 = nobody
	VAR(votelock, 0, 2, 5); // 0 = off, 1 = master can select same game (+1 admin), 3 = master only can vote (+1 admin), 5 = no voting
	VAR(votewait, 0, 3000, INT_MAX-1);

	ICOMMAND(gameid, "", (), result(gameid()));
	ICOMMAND(gamever, "", (), intret(gamever()));

	void resetgamevars(bool flush)
	{
		string val;
		enumerate(*idents, ident, id, {
			if(id.flags&IDF_SERVER) // reset vars
			{
				val[0] = 0;
				switch(id.type)
				{
					case ID_VAR:
					{
						setvar(id.name, id.def.i, true);
                        if(flush) formatstring(val)(id.flags&IDF_HEX ? (id.maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id.storage.i);
						break;
					}
					case ID_FVAR:
					{
						setfvar(id.name, id.def.f, true);
                        if(flush) formatstring(val)("%f", *id.storage.f);
						break;
					}
					case ID_SVAR:
					{
						setsvar(id.name, id.def.s && *id.def.s ? id.def.s : "", true);
                        if(flush) formatstring(val)("%s", *id.storage.s);
						break;
					}
					default: break;
				}
				if(flush) sendf(-1, 1, "ri2ss", SV_COMMAND, -1, &id.name[3], val);
			}
		});
		execfile("servexec.cfg", false);
	}
	ICOMMANDG(resetvars, "", (), resetgamevars(true));

	const char *pickmap(const char *suggest, int mode, int muts)
	{
		const char *map = GVAR(defaultmap);
		if(!map || !*map) map = choosemap(suggest, mode, muts, m_campaign(gamemode) ? 1 : GVAR(maprotate));
		return map;
	}

	void setpause(bool on = false)
	{
		if(sv_gamepaused != on ? 1 : 0)
		{
			setvar("sv_gamepaused", on ? 1 : 0, true);
			sendf(-1, 1, "ri2ss", SV_COMMAND, -1, "gamepaused", on ? 1 : 0);
		}
	}

	void cleanup()
	{
		setpause(false);
		if(GVAR(resetmmonend)) mastermode = MM_OPEN;
		if(GVAR(resetvarsonend)) resetgamevars(true);
		if(GVAR(resetbansonend)) bannedips.setsize(0);
		changemap();
	}

	void start() { cleanup(); }

	void *newinfo() { return new clientinfo; }
	void deleteinfo(void *ci) { delete (clientinfo *)ci; }

	const char *mastermodename(int type)
	{
		switch(type)
		{
			case MM_OPEN: return "open";
			case MM_VETO: return "veto";
			case MM_LOCKED: return "locked";
			case MM_PRIVATE: return "private";
			case MM_PASSWORD: return "password";
			default: return "unknown";
		}
	}

	const char *privname(int type)
	{
		switch(type)
		{
			case PRIV_ADMIN: return "admin";
			case PRIV_MASTER: return "master";
			case PRIV_MAX: return "local";
			default: return "alone";
		}
	}

	int numclients(int exclude, bool nospec, int aitype)
	{
		int n = 0;
		loopv(clients)
		{
			if(clients[i]->clientnum >= 0 && clients[i]->name[0] && clients[i]->clientnum != exclude &&
				(!nospec || clients[i]->state.state != CS_SPECTATOR) &&
					(clients[i]->state.aitype < 0 || (aitype >= 0 && clients[i]->state.aitype <= aitype && clients[i]->state.ownernum >= 0)))
						n++;
		}
		return n;
	}

	bool haspriv(clientinfo *ci, int flag, const char *msg = NULL)
	{
		if(ci->local || ci->privilege >= flag) return true;
		else if(mastermask&MM_AUTOAPPROVE && flag <= PRIV_MASTER && !numclients(ci->clientnum, false, -1)) return true;
		else if(msg)
			srvmsgf(ci->clientnum, "\fraccess denied, you need to be %s to %s", privname(flag), msg);
		return false;
	}

	bool duplicatename(clientinfo *ci, char *name)
	{
		if(!name) name = ci->name;
		loopv(clients) if(clients[i]!=ci && !strcmp(name, clients[i]->name)) return true;
		return false;
	}

	const char *colorname(clientinfo *ci, char *name = NULL, bool team = true, bool dupname = true)
	{
		if(!name) name = ci->name;
		static string cname;
		formatstring(cname)("\fs%s%s", teamtype[ci->team].chat, name);
		if(!name[0] || ci->state.aitype >= 0 || (dupname && duplicatename(ci, name)))
		{
			defformatstring(s)(" [\fs\fc%s%d\fS]", ci->state.aitype >= 0 ? "\fe" : "", ci->clientnum);
			concatstring(cname, s);
		}
		concatstring(cname, "\fS");
		return cname;
	}

    const char *gameid() { return GAMEID; }
    int gamever() { return GAMEVERSION; }
    const char *gamename(int mode, int muts)
    {
    	if(!m_game(mode))
    	{
			mode = G_DEATHMATCH;
			muts = gametype[mode].implied;
    	}
    	static string gname;
    	gname[0] = 0;
    	if(gametype[mode].mutators && muts) loopi(G_M_NUM)
		{
			if((gametype[mode].mutators&mutstype[i].type) && (muts&mutstype[i].type) && (!gametype[mode].implied || !(gametype[mode].implied&mutstype[i].type)))
			{
				defformatstring(name)("%s%s%s", *gname ? gname : "", *gname ? "-" : "", mutstype[i].name);
				copystring(gname, name);
			}
		}
		defformatstring(mname)("%s%s%s", *gname ? gname : "", *gname ? " " : "", gametype[mode].name);
		copystring(gname, mname);
		return gname;
    }
	ICOMMAND(gamename, "iii", (int *g, int *m), result(gamename(*g, *m)));

    void modecheck(int *mode, int *muts, int trying)
    {
		if(!m_game(*mode))
		{
			*mode = G_DEATHMATCH;
			*muts = gametype[*mode].implied;
		}
		#define modecheckreset(a) { if(*muts && ++count < G_M_NUM*4) { i = 0; a; } else { *muts = 0; break; } }
		if(!gametype[*mode].mutators) *muts = G_M_NONE;
		else
		{
			int count = 0;
			if(gametype[*mode].implied) *muts |= gametype[*mode].implied;
			if(*muts) loopi(G_M_NUM)
			{
				if(trying && !(gametype[*mode].mutators&mutstype[i].type) && (trying&mutstype[i].type)) trying &= ~mutstype[i].type;
				if(!(gametype[*mode].mutators&mutstype[i].type) && (*muts&mutstype[i].type))
				{
					*muts &= ~mutstype[i].type;
					modecheckreset(continue);
				}
				if(*muts&mutstype[i].type) loopj(G_M_NUM)
				{
					if(mutstype[i].mutators && !(mutstype[i].mutators&mutstype[j].type) && (*muts&mutstype[j].type))
					{
						if(trying && (trying&mutstype[j].type) && !(gametype[*mode].implied&mutstype[i].type)) *muts &= ~mutstype[i].type;
						else *muts &= ~mutstype[j].type;
						modecheckreset(break);
					}
					if(mutstype[i].implied && (mutstype[i].implied&mutstype[j].type) && !(*muts&mutstype[j].type))
					{
						*muts |= mutstype[j].type;
						modecheckreset(break);
					}
				}
			}
		}
    }

	int mutscheck(int mode, int muts, int trying)
	{
		int gm = mode, mt = muts;
		modecheck(&gm, &mt, trying);
		return mt;
	}
	ICOMMAND(mutscheck, "iii", (int *g, int *m, int *t), intret(mutscheck(*g, *m, *t)));

	void changemode(int *mode, int *muts)
	{
		if(*mode < 0)
		{
			if(GVAR(defaultmode) >= G_START) *mode = GVAR(defaultmode);
			else *mode = rnd(G_RAND)+G_FIGHT;
		}
		if(*muts < 0)
		{
			if(GVAR(defaultmuts) >= G_M_NONE) *muts = GVAR(defaultmuts);
			else
			{
				*muts = G_M_NONE;
				int num = rnd(G_M_NUM+1);
				if(num) loopi(num)
				{
					int rmut = rnd(G_M_NUM+1);
					if(rmut) *muts |= 1<<(rmut-1);
				}
			}
		}
		modecheck(mode, muts);
	}

	const char *choosemap(const char *suggest, int mode, int muts, int force)
	{
		static string mapchosen;
		if(suggest && *suggest) copystring(mapchosen, suggest);
		else *mapchosen = 0;
		int rotate = force ? force : GVAR(maprotate);
		if(rotate)
		{
			const char *maplist = GVAR(mainmaps);
			if(m_campaign(mode)) maplist = GVAR(storymaps);
			else if(m_duel(mode, muts)) maplist = GVAR(duelmaps);
			else if(m_stf(mode)) maplist = GVAR(stfmaps);
			else if(m_ctf(mode)) maplist = m_multi(mode, muts) ? GVAR(mctfmaps) : GVAR(ctfmaps);
			else if(m_trial(mode)) maplist = GVAR(trialmaps);
			if(maplist && *maplist)
			{
				int n = listlen(maplist), p = -1, c = -1;
				if(*mapchosen)
				{
					loopi(n)
					{
						char *maptxt = indexlist(maplist, i);
						if(maptxt)
						{
							string maploc;
							if(strpbrk(maptxt, "/\\")) copystring(maploc, maptxt);
							else formatstring(maploc)("maps/%s", maptxt);
							if(!strcmp(mapchosen, maptxt) || !strcmp(mapchosen, maploc))
							{
								p = i;
								if(rotate == 1) c = i >= 0 && i < n-1 ? i+1 : 0;
							}
							DELETEA(maptxt);
						}
						if(p >= 0) break;
					}
				}
				if(c < 0)
				{
					c = n ? rnd(n) : 0;
					if(c == p) c = p >= 0 && p < n-1 ? p+1 : 0;
				}
				char *mapidx = c >= 0 ? indexlist(maplist, c) : NULL;
				if(mapidx)
				{
					copystring(mapchosen, mapidx);
					DELETEA(mapidx);
				}
			}
		}
		return *mapchosen ? mapchosen : pickmap(suggest, mode, muts);
	}

	bool canload(const char *type)
	{
		if(!strcmp(type, gameid()) || !strcmp(type, "fps") || !strcmp(type, "bfg"))
			return true;
		return false;
	}

	void startintermission()
	{
		setpause(false);
		minremain = 0;
		gamelimit = min(gamelimit, gamemillis);
		if(smode) smode->intermission();
		mutate(smuts, mut->intermission());
		maprequest = false;
		interm = gamemillis+GVAR(intermlimit);
		sendf(-1, 1, "ri2", SV_TIMEUP, 0);
	}

	void checklimits()
	{
		if(m_fight(gamemode))
		{
			if(m_trial(gamemode))
			{
				loopv(clients) if(clients[i]->state.cpmillis < 0 && gamemillis+clients[i]->state.cpmillis >= GVAR(triallimit))
				{
					sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_GUIBACK, CON_MESG, "\fctime trial wait period has timed out");
					startintermission();
					return;
				}
			}
			if(GVAR(timelimit) != oldtimelimit || (gamemillis-curtime>0 && gamemillis/60000!=(gamemillis-curtime)/60000))
			{
				if(GVAR(timelimit) != oldtimelimit)
				{
					if(GVAR(timelimit)) gamelimit += (GVAR(timelimit)-oldtimelimit)*60000;
					oldtimelimit = GVAR(timelimit);
				}
				if(minremain)
				{
					if(GVAR(timelimit))
					{
						if(gamemillis >= gamelimit) minremain = 0;
						else minremain = (gamelimit-gamemillis+60000-1)/60000;
					}
					else minremain = -1;
					if(!minremain)
					{
						sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_GUIBACK, CON_MESG, "\fctime limit has been reached");
						startintermission();
						return; // bail
					}
					else
					{
						sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
						if(minremain == 1) sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_V_ONEMINUTE, CON_MESG, "\fcone minute remains");
					}
				}
			}
			if(GVAR(fraglimit) && !m_ctf(gamemode) && !m_stf(gamemode) && !m_trial(gamemode))
			{
				if(m_team(gamemode, mutators))
				{
					int teamscores[TEAM_NUM] = { 0, 0, 0, 0 };
					loopv(clients) if(clients[i]->state.aitype < AI_START && clients[i]->team >= TEAM_FIRST && isteam(gamemode, mutators, clients[i]->team, TEAM_FIRST))
						teamscores[clients[i]->team-TEAM_FIRST] += clients[i]->state.frags;
					int best = -1;
					loopi(TEAM_NUM) if(best < 0 || teamscores[i] > teamscores[best])
						best = i;
					if(best >= 0 && teamscores[best] >= GVAR(fraglimit))
					{
						sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_GUIBACK, CON_MESG, "\fcfrag limit has been reached");
						startintermission();
						return; // bail
					}
				}
				else
				{
					int best = -1;
					loopv(clients) if(clients[i]->state.aitype < AI_START && (best < 0 || clients[i]->state.frags > clients[best]->state.frags))
						best = i;
					if(best >= 0 && clients[best]->state.frags >= GVAR(fraglimit))
					{
						sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_GUIBACK, CON_MESG, "\fcfrag limit has been reached");
						startintermission();
						return; // bail
					}
				}
			}
		}
	}

	bool hasitem(int i)
	{
		if(m_noitems(gamemode, mutators)) return false;
		switch(sents[i].type)
		{
			case WEAPON:
				if((sents[i].attrs[3] > 0 && sents[i].attrs[3] != triggerid) || !m_check(sents[i].attrs[2], gamemode)) return false;
				if(m_arena(gamemode, mutators) && sents[i].attrs[0] != WEAP_GRENADE) return false;
				break;
			default: break;
		}
		return true;
	}

	bool finditem(int i, bool spawned = true, bool timeit = false)
	{
		if(!m_noitems(gamemode, mutators))
		{
			if(sents[i].spawned) return true;
			int sweap = m_weapon(gamemode, mutators);
			if(sents[i].type != WEAPON || w_carry(w_attr(gamemode, sents[i].attrs[0], sweap), sweap))
			{
				loopvk(clients)
				{
					clientinfo *ci = clients[k];
					if(ci->state.dropped.projs.find(i) >= 0 && (!spawned || (timeit && gamemillis < sents[i].millis)))
						return true;
					else loopj(WEAP_MAX) if(ci->state.entid[j] == i) return spawned;
				}
			}
			if(spawned && timeit && gamemillis < sents[i].millis) return true;
		}
		return false;
	}

	int sortitems(int *a, int *b) { return rnd(3)-1; }
	void setupitems(bool update)
	{
		static vector<int> items; items.setsizenodelete(0);
		loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM && hasitem(i))
		{
			sents[i].millis += GVAR(itemspawndelay);
			switch(GVAR(itemspawnstyle))
			{
				case 1: items.add(i); break;
				case 2: sents[i].millis += rnd(GVAR(itemspawntime)); break;
				default: break;
			}
		}
		if(!items.empty())
		{
			items.sort(sortitems);
			loopv(items) sents[items[i]].millis += GVAR(itemspawndelay)*i;
		}
	}

	void setuptriggers(bool update)
	{
		loopi(TRIGGERIDS+1) triggers[i].reset(i);
		if(update)
		{
			loopv(sents) if(sents[i].type == TRIGGER && sents[i].attrs[4] >= 2 && sents[i].attrs[0] >= 0 && sents[i].attrs[0] <= TRIGGERIDS+1)
				triggers[sents[i].attrs[0]].ents.add(i);
		}
		else triggerid = 0;

		if(triggerid <= 0)
		{
			static vector<int> valid; valid.setsizenodelete(0);
			loopi(TRIGGERIDS) if(!triggers[i+1].ents.empty()) valid.add(triggers[i+1].id);
			if(!valid.empty()) triggerid = valid[rnd(valid.length())];
		}

		if(triggerid > 0) loopi(TRIGGERIDS) if(triggers[i+1].id != triggerid) loopvk(triggers[i+1].ents)
		{
			bool spawn = sents[triggers[i+1].ents[k]].attrs[4]%2;
			if(spawn != sents[triggers[i+1].ents[k]].spawned)
			{
				sents[triggers[i+1].ents[k]].spawned = spawn;
				sents[triggers[i+1].ents[k]].millis = gamemillis;
			}
			sendf(-1, 1, "ri3", SV_TRIGGER, triggers[i+1].ents[k], 1+(spawn ? 2 : 1));
			loopvj(sents[triggers[i+1].ents[k]].kin) if(sents.inrange(sents[triggers[i+1].ents[k]].kin[j]))
			{
				sents[sents[triggers[i+1].ents[k]].kin[j]].spawned = sents[triggers[i+1].ents[k]].spawned;
				sents[sents[triggers[i+1].ents[k]].kin[j]].millis = sents[triggers[i+1].ents[k]].millis;
			}
		}
	}

	struct spawn
	{
		int spawncycle;
		vector<int> ents;
		vector<int> cycle;

		spawn() { reset(); }
		~spawn() {}

		void reset()
		{
			ents.setsize(0);
			cycle.setsize(0);
			spawncycle = 0;
		}
		void add(int n)
		{
			ents.add(n);
			cycle.add(0);
		}
	} spawns[TEAM_LAST+1];
	int nplayers, totalspawns;

	void setupspawns(bool update, int players = 0)
	{
		nplayers = totalspawns = 0;
		loopi(TEAM_LAST+1) spawns[i].reset();
		if(update)
		{
			int numt = numteams(gamemode, mutators), cplayers = 0;
			if(m_fight(gamemode) && m_team(gamemode, mutators))
			{
				loopk(3)
				{
					loopv(sents) if(sents[i].type == PLAYERSTART && (sents[i].attrs[4] == triggerid || !sents[i].attrs[4]) && m_check(sents[i].attrs[3], gamemode))
					{
						if(!k && !isteam(gamemode, mutators, sents[i].attrs[0], TEAM_FIRST)) continue;
						else if(k == 1 && sents[i].attrs[0] == TEAM_NEUTRAL) continue;
						else if(k == 2 && sents[i].attrs[0] != TEAM_NEUTRAL) continue;
						spawns[!k && m_team(gamemode, mutators) ? sents[i].attrs[0] : TEAM_NEUTRAL].add(i);
						totalspawns++;
					}
					if(!k && m_team(gamemode, mutators))
					{
						loopi(numt) if(spawns[i+TEAM_FIRST].ents.empty())
						{
							loopj(TEAM_LAST+1) spawns[j].reset();
							totalspawns = 0;
							break;
						}
					}
					if(totalspawns) break;
				}
			}
			else
			{ // use all neutral spawns
				loopv(sents) if(sents[i].type == PLAYERSTART && sents[i].attrs[0] == TEAM_NEUTRAL && (sents[i].attrs[4] == triggerid || !sents[i].attrs[4]) && m_check(sents[i].attrs[3], gamemode))
				{
					spawns[TEAM_NEUTRAL].add(i);
					totalspawns++;
				}
			}
			if(!totalspawns)
			{ // use all spawns
				loopv(sents) if(sents[i].type == PLAYERSTART && (sents[i].attrs[4] == triggerid || !sents[i].attrs[4]) && m_check(sents[i].attrs[3], gamemode))
				{
					spawns[TEAM_NEUTRAL].add(i);
					totalspawns++;
				}
			}

			if(totalspawns) cplayers = totalspawns/2;
			else
			{ // we can cheat and use weapons for spawns
				loopv(sents) if(sents[i].type == WEAPON)
				{
					spawns[TEAM_NEUTRAL].add(i);
					totalspawns++;
				}
				cplayers = totalspawns/3;
			}
			nplayers = players > 0 ? players : cplayers;
			if(m_fight(gamemode) && m_team(gamemode, mutators))
			{
				int offt = nplayers%numt;
				if(offt) nplayers += numt-offt;
			}
		}
	}

	int pickspawn(clientinfo *ci)
	{
		if(ci->state.aitype >= AI_START) return ci->state.aientity;
		else
		{
			if((m_campaign(gamemode) || m_trial(gamemode) || m_lobby(gamemode)) && !ci->state.cpnodes.empty())
			{
				int checkpoint = ci->state.cpnodes.last();
				if(sents.inrange(checkpoint)) return checkpoint;
			}
			if(totalspawns && GVAR(spawnrotate))
			{
				int cycle = -1, team = m_fight(gamemode) && m_team(gamemode, mutators) && !spawns[ci->team].ents.empty() ? ci->team : TEAM_NEUTRAL;
				if(!spawns[team].ents.empty())
				{
					switch(GVAR(spawnrotate))
					{
						case 2:
						{
							int num = 0, lowest = -1;
							loopv(spawns[team].cycle) if(lowest < 0 || spawns[team].cycle[i] < lowest) lowest = spawns[team].cycle[i];
							loopv(spawns[team].cycle) if(spawns[team].cycle[i] == lowest) num++;
							if(num > 0)
							{
								int r = rnd(num), n = 0;
								loopv(spawns[team].cycle) if(spawns[team].cycle[i] == lowest)
								{
									if(n == r)
									{
										spawns[team].cycle[i]++;
										spawns[team].spawncycle = cycle = i;
										break;
									}
									n++;
								}
								break;
							}
							// fall through if this fails..
						}
						case 1: default:
						{
							if(++spawns[team].spawncycle >= spawns[team].ents.length()) spawns[team].spawncycle = 0;
							cycle = spawns[team].spawncycle;
							break;
						}
					}
					if(spawns[team].ents.inrange(cycle)) return spawns[team].ents[cycle];
				}
			}
		}
		return -1;
	}

	void setupgameinfo(int np)
	{
		loopvk(clients) clients[k]->state.dropped.reset();
		setuptriggers(true);
		if(m_fight(gamemode)) setupitems(true);
		setupspawns(true, m_trial(gamemode) || m_lobby(gamemode) ? 0 : (m_campaign(gamemode) ? GVAR(storyplayers) : np));
		hasgameinfo = aiman::dorefresh = true;
	}

	void sendspawn(clientinfo *ci)
	{
		servstate &gs = ci->state;
		int weap = m_weapon(gamemode, mutators), maxhealth = m_health(gamemode, mutators);
		bool grenades = GVAR(spawngrenades) >= (m_insta(gamemode, mutators) || m_trial(gamemode) ? 2 : 1), arena = m_arena(gamemode, mutators);
		if(ci->state.aitype >= AI_START)
		{
			weap = aistyle[ci->state.aitype].weap;
			if(!isweap(weap)) weap = rnd(WEAP_SUPER-1)+1;
			maxhealth = aistyle[ci->state.aitype].health;
			arena = grenades = false;
		}
		gs.spawnstate(weap, maxhealth, arena, grenades);
		int spawn = pickspawn(ci);
		sendf(ci->clientnum, 1, "ri8v", SV_SPAWNSTATE, ci->clientnum, spawn, gs.state, gs.frags, gs.health, gs.cptime, gs.weapselect, WEAP_MAX, &gs.ammo[0]);
		gs.lastrespawn = gs.lastspawn = gamemillis;
	}

	template<class T>
    void sendstate(servstate &gs, T &p)
    {
        putint(p, gs.state);
        putint(p, gs.frags);
        putint(p, gs.health);
        putint(p, gs.cptime);
        putint(p, gs.weapselect);
        loopi(WEAP_MAX) putint(p, gs.ammo[i]);
    }

	void relayf(int r, const char *s, ...)
	{
		defvformatstring(str, s, s);
		string st;
		filtertext(st, str);
#ifdef IRC
		ircoutf(r, "%s", st);
#endif
#ifdef STANDALONE
		printf("%s\n", st);
#endif
	}

	void srvmsgf(int cn, const char *s, ...)
	{
		if(cn < 0 || allowbroadcast(cn))
		{
			defvformatstring(str, s, s);
			int conlevel = CON_MESG;
			switch(cn)
			{
				case -3: conlevel = CON_CHAT; break;
				case -2: conlevel = CON_EVENT; break;
				default: break;
			}
			sendf(cn, 1, "ri2s", SV_SERVMSG, conlevel, str);
		}
	}

	void srvoutf(int r, const char *s, ...)
	{
		defvformatstring(str, s, s);
		srvmsgf(-1, "%s", str);
		relayf(r, "%s", str);
	}

	void writedemo(int chan, void *data, int len)
	{
		if(!demorecord) return;
		int stamp[3] = { gamemillis, chan, len };
		lilswap(stamp, 3);
		demorecord->write(stamp, sizeof(stamp));
		demorecord->write(data, len);
	}

	void recordpacket(int chan, void *data, int len)
	{
		writedemo(chan, data, len);
	}

	void listdemos(int cn)
	{
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
		putint(p, SV_SENDDEMOLIST);
		putint(p, demos.length());
		loopv(demos) sendstring(demos[i].info, p);
		sendpacket(cn, 1, p.finalize());
	}

	void cleardemos(int n)
	{
		if(!n)
		{
			loopv(demos) delete[] demos[i].data;
			demos.setsize(0);
			srvoutf(4, "cleared all demos");
		}
		else if(demos.inrange(n-1))
		{
			delete[] demos[n-1].data;
			demos.remove(n-1);
			srvoutf(4, "cleared demo %d", n);
		}
	}

	void senddemo(int cn, int num)
	{
		if(!num) num = demos.length();
		if(!demos.inrange(num-1)) return;
		demofile &d = demos[num-1];
		sendf(cn, 2, "rim", SV_SENDDEMO, d.len, d.data);
	}

    void sendwelcome(clientinfo *ci);
    int welcomepacket(packetbuf &p, clientinfo *ci);

	void enddemoplayback()
	{
		if(!demoplayback) return;
        DELETEP(demoplayback);

		loopv(clients) sendf(clients[i]->clientnum, 1, "ri3", SV_DEMOPLAYBACK, 0, clients[i]->clientnum);

		srvoutf(4, "demo playback finished");

		loopv(clients) sendwelcome(clients[i]);
	}

	void setupdemoplayback()
	{
		demoheader hdr;
		string msg;
		msg[0] = '\0';
		defformatstring(file)("%s.dmo", smapname);
		demoplayback = opengzfile(file, "rb");
		if(!demoplayback) formatstring(msg)("could not read demo \"%s\"", file);
		else if(demoplayback->read(&hdr, sizeof(demoheader))!=sizeof(demoheader) || memcmp(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic)))
			formatstring(msg)("\"%s\" is not a demo file", file);
		else
		{
            lilswap(&hdr.version, 2);
			if(hdr.version!=DEMO_VERSION) formatstring(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.version<DEMO_VERSION ? "older" : "newer");
			else if(hdr.gamever!=GAMEVERSION) formatstring(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.gamever<GAMEVERSION ? "older" : "newer");
		}
		if(msg[0])
		{
            DELETEP(demoplayback);
			srvoutf(4, "%s", msg);
			return;
		}

		srvoutf(4, "playing demo \"%s\"", file);

		sendf(-1, 1, "ri3", SV_DEMOPLAYBACK, 1, -1);

		if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
		{
			enddemoplayback();
			return;
		}
		lilswap(&nextplayback, 1);
	}

	void readdemo()
	{
		if(!demoplayback || paused) return;
		while(gamemillis>=nextplayback)
		{
			int chan, len;
			if(demoplayback->read(&chan, sizeof(chan))!=sizeof(chan) ||
				demoplayback->read(&len, sizeof(len))!=sizeof(len))
			{
				enddemoplayback();
				return;
			}
            lilswap(&chan, 1);
            lilswap(&len, 1);
			ENetPacket *packet = enet_packet_create(NULL, len, 0);
			if(!packet || demoplayback->read(packet->data, len)!=len)
			{
				if(packet) enet_packet_destroy(packet);
				enddemoplayback();
				return;
			}
			sendpacket(-1, chan, packet);
			if(!packet->referenceCount) enet_packet_destroy(packet);
			if(demoplayback->read(&nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
			{
				enddemoplayback();
				return;
			}
			lilswap(&nextplayback, 1);
		}
	}

	void enddemorecord()
	{
		if(!demorecord) return;

        DELETEP(demorecord);

		if(!demotmp) return;

		int len = demotmp->size();
		if(demos.length()>=MAXDEMOS)
		{
			delete[] demos[0].data;
			demos.remove(0);
		}
		demofile &d = demos.add();
		time_t t = time(NULL);
		char *timestr = ctime(&t), *trim = timestr + strlen(timestr);
		while(trim>timestr && isspace(*--trim)) *trim = '\0';
		formatstring(d.info)("%s: %s, %s, %.2f%s", timestr, gamename(gamemode, mutators), smapname, len > 1024*1024 ? len/(1024*1024.f) : len/1024.0f, len > 1024*1024 ? "MB" : "kB");
		srvoutf(4, "demo \"%s\" recorded", d.info);
		d.data = new uchar[len];
		d.len = len;
        demotmp->seek(0, SEEK_SET);
		demotmp->read(d.data, len);
        DELETEP(demotmp);
	}

	void setupdemorecord()
	{
		if(m_demo(gamemode) || m_edit(gamemode)) return;

        demotmp = opentempfile("demorecord", "w+b");
        stream *f = opengzfile(NULL, "wb", demotmp);
        if(!f) { DELETEP(demotmp); return; }

        srvoutf(4, "recording demo");

        demorecord = f;

		demoheader hdr;
		memcpy(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic));
		hdr.version = DEMO_VERSION;
		hdr.gamever = GAMEVERSION;
        lilswap(&hdr.version, 2);
        demorecord->write(&hdr, sizeof(demoheader));

        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        welcomepacket(p, NULL);
        writedemo(1, p.buf, p.len);
	}

	void endmatch()
	{
		setpause(false);
		if(demorecord) enddemorecord();
		if(GVAR(resetmmonend) >= 2) mastermode = MM_OPEN;
		if(GVAR(resetvarsonend) >= 2) resetgamevars(true);
		if(GVAR(resetbansonend) >= 2) bannedips.setsize(0);
	}

	bool checkvotes(bool force = false)
	{
        shouldcheckvotes = false;

		vector<votecount> votes;
		int maxvotes = 0;
		loopv(clients)
		{
			clientinfo *oi = clients[i];
			if(oi->state.aitype >= 0) continue;
			maxvotes++;
			if(!oi->mapvote[0]) continue;
			votecount *vc = NULL;
			loopvj(votes) if(!strcmp(oi->mapvote, votes[j].map) && oi->modevote == votes[j].mode && oi->mutsvote == votes[j].muts)
			{
				vc = &votes[j];
				break;
			}
			if(!vc) vc = &votes.add(votecount(oi->mapvote, oi->modevote, oi->mutsvote));
			vc->count++;
		}

		votecount *best = NULL;
		int morethanone = 0;
		loopv(votes) if(!best || votes[i].count >= best->count)
		{
			if(best && votes[i].count == best->count) morethanone++;
			else morethanone = 0;
			best = &votes[i];
		}
		bool gotvotes = best && best->count >= min(max(maxvotes/2, force ? 1 : 2), force ? 1 : maxvotes);
		if(force && gotvotes && morethanone)
		{
			int r = rnd(morethanone+1), n = 0;
			loopv(votes) if(votes[i].count == best->count)
			{
				if(n != r) n++;
				else { best = &votes[i]; break; }
			}
		}
		if(force || gotvotes)
		{
			endmatch();
			if(gotvotes)
			{
				srvoutf(3, "vote passed: \fs\fc%s\fS on map \fs\fc%s\fS", gamename(best->mode, best->muts), best->map);
				sendf(-1, 1, "ri2si3", SV_MAPCHANGE, 1, best->map, 0, best->mode, best->muts);
				changemap(best->map, best->mode, best->muts);
			}
			else
			{
				int mode = GVAR(defaultmode) >= 0 ? gamemode : -1, muts = GVAR(defaultmuts) >= 0 ? mutators : -1;
				changemode(&mode, &muts);
				const char *map = choosemap(smapname, mode, muts);
				srvoutf(3, "server chooses: \fs\fc%s\fS on map \fs\fc%s\fS", gamename(mode, muts), map);
				sendf(-1, 1, "ri2si3", SV_MAPCHANGE, 1, map, 0, mode, muts);
				changemap(map, mode, muts);
			}
			return true;
		}
		return false;
	}

	bool vote(char *reqmap, int &reqmode, int &reqmuts, int sender)
	{
		clientinfo *ci = (clientinfo *)getinfo(sender); modecheck(&reqmode, &reqmuts);
        if(!ci || !m_game(reqmode) || !reqmap || !*reqmap) return false;
        switch(votelock)
        {
        	case 1: case 2: if(reqmode == gamemode && reqmuts == mutators && smapname[0] && !strcmp(reqmap, smapname) && !haspriv(ci, votelock == 1 ? PRIV_MASTER : PRIV_ADMIN, "vote for the same game again")) return false; break;
			case 3: case 4: if(!haspriv(ci, votelock == 3 ? PRIV_MASTER : PRIV_ADMIN, "vote for a new game")) return false; break;
			case 5: if(!haspriv(ci, PRIV_MAX, "vote for a new game")) return false; break;
        }
        bool hasveto = haspriv(ci, PRIV_MASTER) && (mastermode >= MM_VETO || !numclients(ci->clientnum, false, -1));
        if(!hasveto)
        {
        	if(ci->lastvote && lastmillis-ci->lastvote <= votewait) return false;
        	if(ci->modevote == reqmode && ci->mutsvote == reqmuts && !strcmp(ci->mapvote, reqmap)) return false;
        }
		if(reqmode < G_LOBBY && !ci->local)
		{
			srvmsgf(ci->clientnum, "\fraccess denied, you must be a local client");
			return false;
		}
		switch(modelock)
		{
			case 0: default: break;
			case 1: case 2: if(!haspriv(ci, modelock == 1 ? PRIV_MASTER : PRIV_ADMIN, "change game modes")) return false; break;
			case 3: case 4: if(reqmode < modelimit && !haspriv(ci, modelock == 3 ? PRIV_MASTER : PRIV_ADMIN, "change to a locked game mode")) return false; break;
			case 5: if(!haspriv(ci, PRIV_MAX, "change game modes")) return false; break;
		}
		if(reqmode != G_EDITMODE && mapslock)
		{
			const char *maplist = NULL;
			switch(mapslock)
			{
				default: break;
				case 1: case 2: maplist = GVAR(allowmaps); break;
				case 3: case 4:
				{
					if(m_campaign(reqmode)) maplist = GVAR(storymaps);
					else if(m_duel(reqmode, reqmuts)) maplist = GVAR(duelmaps);
					else if(m_stf(reqmode)) maplist = GVAR(stfmaps);
					else if(m_ctf(reqmode)) maplist = m_multi(reqmode, reqmuts) ? GVAR(mctfmaps) : GVAR(ctfmaps);
					else if(m_trial(reqmode)) maplist = GVAR(trialmaps);
					else if(m_fight(reqmode)) maplist = GVAR(mainmaps);
					else maplist = GVAR(allowmaps);
					break;
				}
				case 5: if(!haspriv(ci, PRIV_MAX, "select a custom maps")) return false; break;
			}
			if(maplist && *maplist)
			{
				int n = listlen(maplist);
				bool found = false;
				string maploc;
				if(strpbrk(reqmap, "/\\")) copystring(maploc, reqmap);
				else formatstring(maploc)("maps/%s", reqmap);
				loopi(n)
				{
					char *maptxt = indexlist(maplist, i);
					if(maptxt)
					{
						string cmapname;
						if(strpbrk(maptxt, "/\\")) copystring(cmapname, maptxt);
						else formatstring(cmapname)("maps/%s", maptxt);
						if(!strcmp(maploc, cmapname)) found = true;
						DELETEA(maptxt);
					}
					if(found) break;
				}
				if(!found && !haspriv(ci, mapslock%2 ? PRIV_MASTER : PRIV_ADMIN, "select a custom maps")) return false;
			}
		}
		copystring(ci->mapvote, reqmap);
		ci->modevote = reqmode;
		ci->mutsvote = reqmuts;
		if(hasveto)
		{
			endmatch();
			srvoutf(3, "%s forced: \fs\fc%s\fS on map \fs\fc%s\fS", colorname(ci), gamename(ci->modevote, ci->mutsvote), ci->mapvote);
			sendf(-1, 1, "ri2si3", SV_MAPCHANGE, 1, ci->mapvote, 0, ci->modevote, ci->mutsvote);
			changemap(ci->mapvote, ci->modevote, ci->mutsvote);
			return false;
		}
		return checkvotes() ? false : true;
	}

	extern void waiting(clientinfo *ci, int doteam = 0, int drop = 2, bool exclude = false);

	void setteam(clientinfo *ci, int team, bool reset = true, bool info = false)
	{
		if(ci->team != team)
		{
			bool sm = false;
			if(reset) waiting(ci, 0, 1);
			else if(ci->state.state == CS_ALIVE)
			{
				if(smode) smode->leavegame(ci);
				mutate(smuts, mut->leavegame(ci));
				sm = true;
			}
			ci->team = team;
			if(sm)
			{
				if(smode) smode->entergame(ci);
				mutate(smuts, mut->entergame(ci));
			}
			if(ci->state.aitype < 0) aiman::dorefresh = true; // get the ai to reorganise
		}
		if(info) sendf(-1, 1, "ri3", SV_SETTEAM, ci->clientnum, ci->team);
	}

	struct teamscore
	{
		int team;
		float score;
		int clients;

		teamscore(int n) : team(n), score(0.f), clients(0) {}
		teamscore(int n, float r) : team(n), score(r), clients(0) {}
		teamscore(int n, int s) : team(n), score(s), clients(0) {}

		~teamscore() {}
	};

	int chooseteam(clientinfo *ci, int suggest = -1)
	{
		if(ci->state.aitype >= AI_START) return TEAM_ENEMY;
		else if(m_fight(gamemode) && m_team(gamemode, mutators) && ci->state.state != CS_SPECTATOR && ci->state.state != CS_EDITING)
		{
			int team = isteam(gamemode, mutators, suggest, TEAM_FIRST) ? suggest : -1, balance = GVAR(teambalance);
			if(balance < 3 && ci->state.aitype >= 0) balance = 1;
			if(balance || team < 0)
			{
				teamscore teamscores[TEAM_NUM] = {
					teamscore(TEAM_ALPHA), teamscore(TEAM_BETA), teamscore(TEAM_GAMMA), teamscore(TEAM_DELTA)
				};
				loopv(clients)
				{
					clientinfo *cp = clients[i];
					if(!cp->team || cp == ci || cp->state.state == CS_SPECTATOR || cp->state.state == CS_EDITING) continue;
					if((cp->state.aitype >= 0 && cp->state.ownernum < 0) || cp->state.aitype >= AI_START) continue;
					if(ci->state.aitype >= 0 || (ci->state.aitype < 0 && cp->state.aitype < 0))
					{ // remember: ai just balance teams
						cp->state.timeplayed += lastmillis-cp->state.lasttimeplayed;
						cp->state.lasttimeplayed = lastmillis;
						teamscore &ts = teamscores[cp->team-TEAM_FIRST];
						ts.score += cp->state.score/float(max(cp->state.timeplayed, 1));
						ts.clients++;
					}
				}
				teamscore *worst = &teamscores[0];
				if(balance != 3 || ci->state.aitype >= 0)
				{
					loopi(numteams(gamemode, mutators))
					{
						teamscore &ts = teamscores[i];
						switch(balance)
						{
							case 2:
							{
								if(ts.score < worst->score || (ts.score == worst->score && ts.clients < worst->clients))
									worst = &ts;
								break;
							}
							case 3:
							{
								if(!i)
								{
									worst = &teamscores[1];
									break; // don't use team alpha for bots in this case
								}
							} // fall through
							case 1: default:
							{
								if(ts.clients < worst->clients || (ts.clients == worst->clients && ts.score < worst->score))
									worst = &ts;
								break;
							}
						}
					}
				}
				team = worst->team;
			}
			return team;
		}
		return TEAM_NEUTRAL;
	}

    void stopdemo()
    {
        if(m_demo(gamemode)) enddemoplayback();
        else enddemorecord();
    }

	savedscore &findscore(clientinfo *ci, bool insert)
	{
		uint ip = getclientip(ci->clientnum);
		if(!ip) return *(savedscore *)0;
		if(!insert)
        {
            loopv(clients)
		    {
			    clientinfo *oi = clients[i];
			    if(oi->clientnum != ci->clientnum && getclientip(oi->clientnum) == ip && !strcmp(oi->name, ci->name))
			    {
				    oi->state.timeplayed += lastmillis-oi->state.lasttimeplayed;
				    oi->state.lasttimeplayed = lastmillis;
				    static savedscore curscore;
				    curscore.save(oi->state);
				    return curscore;
			    }
		    }
        }
		loopv(scores)
		{
			savedscore &sc = scores[i];
			if(sc.ip == ip && !strcmp(sc.name, ci->name)) return sc;
		}
		if(!insert) return *(savedscore *)0;
		savedscore &sc = scores.add();
		sc.ip = ip;
		copystring(sc.name, ci->name);
		return sc;
	}

	void savescore(clientinfo *ci)
	{
		savedscore &sc = findscore(ci, true);
		if(&sc) sc.save(ci->state);
	}

	void givepoints(clientinfo *ci, int points)
	{
		ci->state.score += points; ci->state.points += points;
		sendf(-1, 1, "ri4", SV_POINTS, ci->clientnum, points, ci->state.points);
	}

	struct droplist { int weap, ent; };
	void dropitems(clientinfo *ci, int level = 2)
	{
		if(ci->state.aitype >= AI_START) ci->state.weapreset(false);
		else
		{
			servstate &ts = ci->state;
			vector<droplist> drop;
			int sweap = m_weapon(gamemode, mutators);
			if(level >= 2 && GVAR(kamikaze) && (GVAR(kamikaze) > 2 || (ts.hasweap(WEAP_GRENADE, sweap) && (GVAR(kamikaze) > 1 || ts.weapselect == WEAP_GRENADE))))
			{
				ts.weapshots[WEAP_GRENADE][0].add(-1);
				droplist &d = drop.add();
				d.weap = WEAP_GRENADE;
				d.ent = -1;
			}
			if(!m_noitems(gamemode, mutators))
			{
				loopi(WEAP_MAX) if(ts.hasweap(i, sweap, 1) && sents.inrange(ts.entid[i]))
				{
					sents[ts.entid[i]].millis = gamemillis;
					if(level && GVAR(itemdropping) && !(sents[ts.entid[i]].attrs[1]&WEAP_F_FORCED))
					{
						ts.dropped.add(ts.entid[i]);
						droplist &d = drop.add();
						d.weap = i;
						d.ent = ts.entid[i];
						if(w_carry(i, sweap)) sents[ts.entid[i]].millis += GVAR(itemspawntime);
					}
				}
			}
			if(level && !drop.empty())
				sendf(-1, 1, "ri3iv", SV_DROP, ci->clientnum, -1, drop.length(), drop.length()*sizeof(droplist)/sizeof(int), drop.getbuf());
			ts.weapreset(false);
		}
	}

	#include "stfmode.h"
    #include "ctfmode.h"
	#include "duelmut.h"
	#include "aiman.h"

	void changemap(const char *name, int mode, int muts)
	{
		hasgameinfo = maprequest = mapsending = shouldcheckvotes = aiman::autooverride = false;
		aiman::clearai(m_play(mode) ? false : true);
		aiman::dorefresh = true;
        stopdemo();
		gamemode = mode; mutators = muts; changemode(&gamemode, &mutators);
		nplayers = gamemillis = interm = 0;
		oldtimelimit = GVAR(timelimit);
		minremain = GVAR(timelimit) ? GVAR(timelimit) : -1;
		gamelimit = GVAR(timelimit) ? minremain*60000 : 0;
		sents.setsize(0); scores.setsize(0);
		setuptriggers(false); setupspawns(false);

		const char *reqmap = name && *name ? name : pickmap(smapname, gamemode, mutators);
#ifdef STANDALONE // interferes with savemap on clients, in which case we can just use the auto-request
		loopi(3)
		{
			if(mapdata[i]) DELETEP(mapdata[i]);
			const char *reqext = "xxx";
			switch(i)
			{
				case 2: reqext = "cfg"; break;
				case 1: reqext = "png"; break;
				default: case 0: reqext = "bgz"; break;
			}
			defformatstring(reqfile)(strstr(reqmap, "maps/")==reqmap || strstr(reqmap, "maps\\")==reqmap ? "%s" : "maps/%s", reqmap);
			defformatstring(reqfext)("%s.%s", reqfile, reqext);
			if(!(mapdata[i] = openfile(reqfext, "rb")) && !i)
			{
				loopk(3) if(mapdata[k]) DELETEP(mapdata[k]);
				break;
			}
		}
#endif
		copystring(smapname, reqmap);

		// server modes
		if(m_stf(gamemode)) smode = &stfmode;
        else if(m_ctf(gamemode)) smode = &ctfmode;
		else smode = NULL;
		smuts.setsize(0);
		if(m_duke(gamemode, mutators)) smuts.add(&duelmutator);
		if(smode) smode->reset(false);
		mutate(smuts, mut->reset(false));

		if(m_demo(gamemode)) kicknonlocalclients(DISC_PRIVATE);
		loopv(clients) clients[i]->mapchange(true);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
            if(ci->state.state == CS_SPECTATOR) continue;
            else if(ci->state.aitype < 0 && m_play(gamemode))
            {
                ci->state.state = CS_SPECTATOR;
                sendf(-1, 1, "ri3", SV_SPECTATOR, ci->clientnum, 1);
				setteam(ci, TEAM_NEUTRAL, false, true);
            }
            else
			{
				ci->state.state = CS_DEAD;
				waiting(ci, 2, 1);
			}
		}

		if(m_fight(gamemode) && numclients()) sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
		if(m_demo(gamemode)) setupdemoplayback();
		else if(demonextmatch)
		{
			demonextmatch = false;
			setupdemorecord();
		}
	}

	struct crcinfo
	{
		int crc, matches;

		crcinfo(int crc, int matches) : crc(crc), matches(matches) {}

		static int compare(const crcinfo *x, const crcinfo *y)
		{
			if(x->matches > y->matches) return -1;
			if(x->matches < y->matches) return 1;
			return 0;
		}
	};

	void checkmaps(int req = -1)
	{
		if(m_edit(gamemode) || !smapname[0]) return;
		vector<crcinfo> crcs;
		int total = 0, unsent = 0, invalid = 0;
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->state.state==CS_SPECTATOR || ci->state.aitype >= 0) continue;
			total++;
			if(!ci->clientmap[0])
			{
				if(ci->mapcrc < 0) invalid++;
				else if(!ci->mapcrc) unsent++;
			}
			else
			{
				crcinfo *match = NULL;
				loopvj(crcs) if(crcs[j].crc == ci->mapcrc) { match = &crcs[j]; break; }
				if(!match) crcs.add(crcinfo(ci->mapcrc, 1));
				else match->matches++;
			}
		}
		if(total - unsent < min(total, 4)) return;
		crcs.sort(crcinfo::compare);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->state.state==CS_SPECTATOR || ci->state.aitype >= 0 || ci->clientmap[0] || ci->mapcrc >= 0 || (req < 0 && ci->warned)) continue;
			srvmsgf(req, "\fy\fzRe%s has modified map \"%s\"", colorname(ci), smapname);
			if(req < 0) ci->warned = true;
		}
		if(crcs.empty() || crcs.length() < 2) return;
		loopv(crcs)
		{
			crcinfo &info = crcs[i];
			if(i || info.matches <= crcs[i+1].matches) loopvj(clients)
			{
				clientinfo *ci = clients[j];
				if(ci->state.state==CS_SPECTATOR || ci->state.aitype >= 0 || !ci->clientmap[0] || ci->mapcrc != info.crc || (req < 0 && ci->warned)) continue;
				srvmsgf(req, "\fy\fzRe%s has modified map \"%s\"", colorname(ci), smapname);
				if(req < 0) ci->warned = true;
			}
		}
	}

	bool servcmd(int nargs, const char *cmd, const char *arg)
	{ // incoming command from scripts
		ident *id = idents->access(cmd);
		if(id && id->flags&IDF_SERVER)
		{
			static string scmdval; scmdval[0] = 0;
			switch(id->type)
			{
				case ID_CCOMMAND:
				case ID_COMMAND:
				{
					string s;
					if(nargs <= 1 || !arg) formatstring(s)("%s", cmd);
					else formatstring(s)("%s %s", cmd, arg);
					char *ret = executeret(s);
					if(ret)
					{
						if(*ret) conoutft(CON_MESG, "\fg%s returned %s", cmd, ret);
						delete[] ret;
					}
					return true;
				}
				case ID_VAR:
				{
					if(nargs <= 1 || !arg)
					{
						conoutft(CON_MESG, id->flags&IDF_HEX ? (id->maxval==0xFFFFFF ? "\fg%s = 0x%.6X" : "\fg%s = 0x%X") : "\fg%s = %d", cmd, *id->storage.i);
						return true;
					}
					if(id->maxval < id->minval)
					{
						conoutft(CON_MESG, "\frcannot override variable: %s", cmd);
						return true;
					}
					int ret = atoi(arg);
					if(ret < id->minval || ret > id->maxval)
					{
						conoutft(CON_MESG,
							id->flags&IDF_HEX ?
                                    (id->minval <= 255 ? "\frvalid range for %s is %d..0x%X" : "\frvalid range for %s is 0x%X..0x%X") :
                                    "\frvalid range for %s is %d..%d", cmd, id->minval, id->maxval);
						return true;
					}
					*id->storage.i = ret;
					id->changed();
					formatstring(scmdval)(id->flags&IDF_HEX ? (id->maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id->storage.i);
					break;
				}
				case ID_FVAR:
				{
					if(nargs <= 1 || !arg)
					{
						conoutft(CON_MESG, "\fg%s = %s", cmd, floatstr(*id->storage.f));
						return true;
					}
					float ret = atof(arg);
					if(ret < id->minvalf || ret > id->maxvalf)
					{
						conoutft(CON_MESG, "\frvalid range for %s is %s..%s", cmd, floatstr(id->minvalf), floatstr(id->maxvalf));
						return true;
					}
					*id->storage.f = ret;
					id->changed();
					formatstring(scmdval)("%s", floatstr(*id->storage.f));
					break;
				}
				case ID_SVAR:
				{
					if(nargs <= 1 || !arg)
					{
						conoutft(CON_MESG, strchr(*id->storage.s, '"') ? "\fg%s = [%s]" : "\fg%s = \"%s\"", cmd, *id->storage.s);
						return true;
					}
					delete[] *id->storage.s;
					*id->storage.s = newstring(arg);
					id->changed();
					formatstring(scmdval)("%s", *id->storage.s);
					break;
				}
				default: return false;
			}
			sendf(-1, 1, "ri2ss", SV_COMMAND, -1, &id->name[3], scmdval);
			arg = scmdval;
			return true;
		}
		return false; // parse will spit out "unknown command" in this case
	}

	void parsecommand(clientinfo *ci, int nargs, const char *cmd, const char *arg)
	{ // incoming commands from clients
		defformatstring(cmdname)("sv_%s", cmd);
		ident *id = idents->access(cmdname);
		if(id && id->flags&IDF_SERVER)
		{
			mkstring(val);
			switch(id->type)
			{
				case ID_CCOMMAND:
				case ID_COMMAND:
				{
					if(!haspriv(ci, varslock >= 2 ? PRIV_MAX : (varslock ? PRIV_ADMIN : PRIV_MASTER), "change variables")) return;
					string s;
					if(nargs <= 1 || !arg) formatstring(s)("sv_%s", cmd);
					else formatstring(s)("sv_%s %s", cmd, arg);
					char *ret = executeret(s);
					if(ret && *ret) srvoutf(3, "\fg%s executed %s (returned: %s)", colorname(ci), cmd, ret);
					else srvoutf(3, "\fg%s executed %s", colorname(ci), cmd);
					if(ret) delete[] ret;
					return;
				}
				case ID_VAR:
				{
					if(nargs <= 1 || !arg)
					{
						srvmsgf(ci->clientnum, id->flags&IDF_HEX ? (id->maxval==0xFFFFFF ? "\fg%s = 0x%.6X" : "\fg%s = 0x%X") : "\fg%s = %d", cmd, *id->storage.i);
						return;
					}
					else if(!haspriv(ci, varslock >= 2 ? PRIV_MAX : (varslock ? PRIV_ADMIN : PRIV_MASTER), "change variables")) return;
					if(id->maxval < id->minval)
					{
						srvmsgf(ci->clientnum, "\frcannot override variable: %s", cmd);
						return;
					}
					int ret = atoi(arg);
					if(ret < id->minval || ret > id->maxval)
					{
						srvmsgf(ci->clientnum,
							id->flags&IDF_HEX ?
								(id->minval <= 255 ? "\frvalid range for %s is %d..0x%X" : "\frvalid range for %s is 0x%X..0x%X") :
								"\frvalid range for %s is %d..%d", cmd, id->minval, id->maxval);
						return;
					}
					*id->storage.i = ret;
					id->changed();
					formatstring(val)(id->flags&IDF_HEX ? (id->maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id->storage.i);
					break;
				}
				case ID_FVAR:
				{
					if(nargs <= 1 || !arg)
					{
						srvmsgf(ci->clientnum, "\fg%s = %s", cmd, floatstr(*id->storage.f));
						return;
					}
					else if(!haspriv(ci, varslock >= 2 ? PRIV_MAX : (varslock ? PRIV_ADMIN : PRIV_MASTER), "change variables")) return;
					float ret = atof(arg);
					if(ret < id->minvalf || ret > id->maxvalf)
					{
						srvmsgf(ci->clientnum, "\frvalid range for %s is %s..%s", cmd, floatstr(id->minvalf), floatstr(id->maxvalf));
						return;
					}
					*id->storage.f = ret;
					id->changed();
					formatstring(val)("%s", floatstr(*id->storage.f));
					break;
				}
				case ID_SVAR:
				{
					if(nargs <= 1 || !arg)
					{
						srvmsgf(ci->clientnum, strchr(*id->storage.s, '"') ? "\fg%s = [%s]" : "\fg%s = \"%s\"", cmd, *id->storage.s);
						return;
					}
					else if(!haspriv(ci, varslock >= 2 ? PRIV_MAX : (varslock ? PRIV_ADMIN : PRIV_MASTER), "change variables")) return;
					delete[] *id->storage.s;
					*id->storage.s = newstring(arg);
					id->changed();
					formatstring(val)("%s", *id->storage.s);
					break;
				}
				default: return;
			}
			sendf(-1, 1, "ri2ss", SV_COMMAND, ci->clientnum, &id->name[3], val);
			relayf(3, "\fg%s set %s to %s", colorname(ci), &id->name[3], val);
		}
		else srvmsgf(ci->clientnum, "\frunknown command: %s", cmd);
	}

	clientinfo *choosebestclient()
	{
		clientinfo *best = NULL;
		loopv(clients)
		{
			clientinfo *cs = clients[i];
			if(cs->state.aitype >= 0 || !cs->name[0] || !cs->online || cs->wantsmap) continue;
			if(!best || cs->state.timeplayed > best->state.timeplayed) best = cs;
		}
		return best;
	}

    void sendservinit(clientinfo *ci)
    {
        sendf(ci->clientnum, 1, "ri5", SV_SERVERINIT, ci->clientnum, GAMEVERSION, ci->sessionid, serverpass[0] ? 1 : 0);
    }

	bool restorescore(clientinfo *ci)
	{
        savedscore &sc = findscore(ci, false);
        if(&sc)
        {
            sc.restore(ci->state);
            return true;
        }
        return false;
	}

    void sendresume(clientinfo *ci)
    {
		servstate &gs = ci->state;
		sendf(-1, 1, "ri7vi", SV_RESUME, ci->clientnum, gs.state, gs.frags, gs.health, gs.cptime, gs.weapselect, WEAP_MAX, &gs.ammo[0], -1);
		sendf(-1, 1, "ri4", SV_POINTS, ci->clientnum, 0, ci->state.points);
    }

	void putinitclient(clientinfo *ci, packetbuf &p)
	{
        if(ci->state.aitype >= 0)
        {
        	if(ci->state.ownernum >= 0)
        	{
				putint(p, SV_INITAI);
				putint(p, ci->clientnum);
				putint(p, ci->state.ownernum);
				putint(p, ci->state.aitype);
				putint(p, ci->state.aientity);
				putint(p, ci->state.skill);
				sendstring(ci->name, p);
				putint(p, ci->team);
        	}
        }
        else
        {
			putint(p, SV_CLIENTINIT);
			putint(p, ci->clientnum);
			sendstring(ci->name, p);
			putint(p, ci->team);
        }
	}

    void sendinitclient(clientinfo *ci)
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        putinitclient(ci, p);
        sendpacket(-1, 1, p.finalize(), ci->clientnum);
    }

    void welcomeinitclient(packetbuf &p, int exclude = -1)
    {
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(!ci->connected || ci->clientnum == exclude) continue;

            putinitclient(ci, p);
        }
    }

    void sendwelcome(clientinfo *ci)
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        int chan = welcomepacket(p, ci);
        sendpacket(ci->clientnum, chan, p.finalize());
    }

	int welcomepacket(packetbuf &p, clientinfo *ci)
	{
        putint(p, SV_WELCOME);
		putint(p, SV_MAPCHANGE);
		if(!smapname[0]) putint(p, 0);
		else
		{
			putint(p, 1);
			sendstring(smapname, p);
		}
        if(!ci) putint(p, 0);
		else if(!ci->online && m_edit(gamemode) && numclients(ci->clientnum, false, -1))
		{
			ci->wantsmap = true;
			clientinfo *best = choosebestclient();
			if(best)
			{
				loopi(3) if(mapdata[i]) DELETEP(mapdata[i]);
				mapsending = false;
				sendf(best->clientnum, 1, "ri", SV_GETMAP);
				putint(p, 1);
			}
			else putint(p, 0);
		}
		else
		{
			ci->wantsmap = false;
			if(ci->online) putint(p, 2); // we got a temp map eh?
			else putint(p, ci->local ? -1 : 0);
		}
		putint(p, gamemode);
		putint(p, mutators);

		enumerate(*idents, ident, id, {
			if(id.flags&IDF_SERVER) // reset vars
			{
				mkstring(val);
				switch(id.type)
				{
					case ID_VAR:
					{
						formatstring(val)(id.flags&IDF_HEX ? (id.maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id.storage.i);
						break;
					}
					case ID_FVAR:
					{
						formatstring(val)("%s", floatstr(*id.storage.f));
						break;
					}
					case ID_SVAR:
					{
						formatstring(val)("%s", *id.storage.s);
						break;
					}
					default: break;
				}
				putint(p, SV_COMMAND);
				putint(p, -1);
				sendstring(&id.name[3], p);
				sendstring(val, p);
			}
		});

		if(!ci || (m_fight(gamemode) && numclients()))
		{
			putint(p, SV_TIMEUP);
			putint(p, minremain);
		}

		if(hasgameinfo)
		{
			putint(p, SV_GAMEINFO);
			loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM || sents[i].type == TRIGGER)
			{
				putint(p, i);
				if(enttype[sents[i].type].usetype == EU_ITEM) putint(p, finditem(i, false) ? 1 : 0);
				else putint(p, sents[i].spawned ? 1 : 0);
			}
			putint(p, -1);
		}

        if(ci)
        {
			ci->state.state = CS_SPECTATOR;
			ci->team = TEAM_NEUTRAL;
			putint(p, SV_SPECTATOR);
			putint(p, ci->clientnum);
			putint(p, 1);
			sendf(-1, 1, "ri3x", SV_SPECTATOR, ci->clientnum, 1, ci->clientnum);
            putint(p, SV_SETTEAM);
            putint(p, ci->clientnum);
            putint(p, ci->team);
		}
		if(!ci || clients.length() > 1)
		{
			putint(p, SV_RESUME);
			loopv(clients)
			{
				clientinfo *oi = clients[i];
				if(ci && oi->clientnum == ci->clientnum) continue;
				putint(p, oi->clientnum);
				sendstate(oi->state, p);
			}
			putint(p, -1);
            welcomeinitclient(p, ci ? ci->clientnum : -1);
			loopv(clients)
			{
				clientinfo *oi = clients[i];
				if(oi->state.aitype >= 0 || (ci && oi->clientnum == ci->clientnum)) continue;
				putint(p, SV_POINTS);
				putint(p, oi->clientnum);
				putint(p, 0);
				putint(p, oi->state.points);
				if(oi->mapvote[0])
				{
					putint(p, SV_MAPVOTE);
					putint(p, oi->clientnum);
					sendstring(oi->mapvote, p);
					putint(p, oi->modevote);
					putint(p, oi->mutsvote);
				}
			}
		}

		if(*servermotd)
		{
			putint(p, SV_ANNOUNCE);
			putint(p, S_GUIACT);
			putint(p, CON_CHAT);
			sendstring(servermotd, p);
		}

		if(smode) smode->initclient(ci, p, true);
		mutate(smuts, mut->initclient(ci, p, true));
		if(ci) ci->online = true;
		aiman::dorefresh = true;
		return 1;
	}

	void clearevent(clientinfo *ci) { delete ci->events.remove(0); }

	void dodamage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush = ivec(0, 0, 0))
	{
		int realdamage = damage, realflags = flags, nodamage = 0; realflags &= ~HIT_SFLAGS;
		if((realflags&HIT_WAVE || (isweap(weap) && !weaptype[weap].explode[realflags&HIT_ALT ? 1 : 0])) && (realflags&HIT_FULL))
			realflags &= ~HIT_FULL;
		if(smode && !smode->damage(target, actor, realdamage, weap, realflags, hitpush)) { nodamage++; }
		mutate(smuts, if(!mut->damage(target, actor, realdamage, weap, realflags, hitpush)) { nodamage++; });
		if((actor == target && !GVAR(selfdamage)) || (m_trial(gamemode) && !GVAR(trialdamage))) nodamage++;
		else if(m_team(gamemode, mutators) && actor->team == target->team)
		{
			if(weap == WEAP_MELEE) nodamage++;
			else if(m_campaign(gamemode)) { if(target->team == TEAM_NEUTRAL) nodamage++; }
			else if(m_fight(gamemode)) switch(GVAR(teamdamage))
			{
				case 2: default: break;
				case 1: if(actor == target || actor->state.aitype < 0) break;
				case 0: nodamage++; break;
			}
		}
		if(nodamage || !hithurts(realflags)) realflags = HIT_WAVE|(flags&HIT_ALT ? HIT_ALT : 0); // so it impacts, but not hurts
		else
		{
			target->state.dodamage(target->state.health -= realdamage);
			target->state.lastpain = gamemillis;
			actor->state.damage += realdamage;
			if(target->state.health <= 0) realflags |= HIT_KILL;
			if(GVAR(fireburntime) && doesburn(weap, flags))
			{
				target->state.lastfire = target->state.lastfireburn = gamemillis;
				target->state.lastfireowner = actor->clientnum;
			}
		}
		sendf(-1, 1, "ri7i3", SV_DAMAGE, target->clientnum, actor->clientnum, weap, realflags, realdamage, target->state.health, hitpush.x, hitpush.y, hitpush.z);
		if(GVAR(vampire) && actor->state.state == CS_ALIVE)
		{
			int total = m_health(gamemode, mutators), amt = 0, delay = 0;
			if(smode) smode->regen(actor, total, amt, delay);
			if(total && actor->state.health < total)
			{
				int rgn = actor->state.health, heal = clamp(actor->state.health+realdamage, 0, total), eff = heal-rgn;
				actor->state.health = heal; actor->state.lastregen = gamemillis;
				sendf(-1, 1, "ri4", SV_REGEN, actor->clientnum, actor->state.health, eff);
			}
		}

		if(realflags&HIT_KILL)
		{
            int fragvalue = target == actor || (m_team(gamemode, mutators) && target->team == actor->team) ? -1 : 1,
				pointvalue = smode ? smode->points(target, actor) : fragvalue, style = FRAG_NONE;
			if(!m_insta(gamemode, mutators) && (realflags&HIT_EXPLODE || realdamage > m_health(gamemode, mutators)*3/2))
				style = FRAG_OBLITERATE;
            actor->state.frags += fragvalue;

			if(m_team(gamemode, mutators) && actor->team == target->team)
			{
				actor->state.spree = 0;
				if(actor->state.aitype < AI_START)
				{
					if(actor != target) actor->state.teamkills++;
					pointvalue *= 2;
				}
			}
			else if(actor != target)
			{
				int logs = 0;
				target->state.spree = 0;
				if(actor->state.aitype >= AI_START) pointvalue *= -3;
				else
				{
					actor->state.spree++;
					actor->state.fraglog.add(target->clientnum);
					if((flags&HIT_PROJ) && (flags&HIT_HEAD))
					{
						style |= FRAG_HEADSHOT;
						pointvalue *= 2;
					}
					if(m_fight(gamemode) && GVAR(multikilldelay))
					{
						logs = 0;
						loopv(actor->state.fragmillis)
						{
							if(lastmillis-actor->state.fragmillis[i] > GVAR(multikilldelay)) actor->state.fragmillis.remove(i--);
							else logs++;
						}
						if(!logs) actor->state.rewards &= ~FRAG_MULTI;
						actor->state.fragmillis.add(lastmillis); logs++;
						if(logs >= 2)
						{
							int offset = clamp(logs-2, 0, 2), type = 1<<(FRAG_MKILL+offset); // double, triple, multi..
							if(!(actor->state.rewards&type))
							{
								style |= type;
								actor->state.rewards |= type;
								pointvalue *= offset+1;
								//loopv(actor->state.fragmillis) actor->state.fragmillis[i] = lastmillis;
							}
						}
					}
					if(actor->state.spree <= GVAR(spreecount)*FRAG_SPREES && !(actor->state.spree%GVAR(spreecount)))
					{
						int offset = clamp((actor->state.spree/GVAR(spreecount)), 1, int(FRAG_SPREES))-1, type = 1<<(FRAG_SPREE+offset);
						if(!(actor->state.rewards&type))
						{
							style |= type;
							actor->state.rewards |= type;
							pointvalue *= offset+1;
						}
					}
					if(m_fight(gamemode))
					{
						logs = 0;
						loopv(target->state.fraglog) if(target->state.fraglog[i] == actor->clientnum) { logs++; target->state.fraglog.remove(i--); }
						if(logs >= GVAR(dominatecount))
						{
							style |= FRAG_REVENGE;
							pointvalue *= GVAR(dominatecount);
						}
						logs = 0;
						loopv(actor->state.fraglog) if(actor->state.fraglog[i] == target->clientnum) logs++;
						if(logs == GVAR(dominatecount))
						{
							style |= FRAG_DOMINATE;
							pointvalue *= GVAR(dominatecount);
						}
					}
				}
			}
			else actor->state.spree = 0;
			target->state.deaths++;
			dropitems(target);
			if(actor != target && actor->state.aitype >= AI_START) givepoints(target, pointvalue); else givepoints(actor, pointvalue);
			sendf(-1, 1, "ri8", SV_DIED, target->clientnum, actor->clientnum, actor->state.frags, style, weap, realflags, realdamage);
			target->position.setsizenodelete(0);
			if(smode) smode->died(target, actor);
			mutate(smuts, mut->died(target, actor));
			target->state.state = CS_DEAD; // don't issue respawn yet until DEATHMILLIS has elapsed
			target->state.lastdeath = gamemillis;
		}
	}

	void suicideevent::process(clientinfo *ci)
	{
		servstate &gs = ci->state;
		if(gs.state != CS_ALIVE) return;
		if(!(flags&HIT_DEATH) && !(flags&HIT_LOST))
		{
			if(smode && !smode->damage(ci, ci, ci->state.health, -1, flags)) { return; }
			mutate(smuts, if(!mut->damage(ci, ci, ci->state.health, -1, flags)) { return; });
		}
		int fragvalue = -1, pointvalue = smode ? smode->points(ci, ci) : fragvalue;
        ci->state.frags += fragvalue;
        ci->state.spree = 0;
        if(!flags && (m_trial(gamemode) || m_lobby(gamemode)))
        {
        	ci->state.cpmillis = 0;
			ci->state.cpnodes.setsize(0);
        }
        ci->state.deaths++;
		dropitems(ci); givepoints(ci, pointvalue);
		if(!(flags&HIT_FULL)) flags |= HIT_FULL;
		if(GVAR(fireburntime) && (flags&HIT_MELT || flags&HIT_BURN))
		{
			ci->state.lastfire = ci->state.lastfireburn = gamemillis;
			ci->state.lastfireowner = ci->clientnum;
		}
		sendf(-1, 1, "ri8", SV_DIED, ci->clientnum, ci->clientnum, ci->state.frags, 0, -1, flags, ci->state.health);
        ci->position.setsizenodelete(0);
		if(smode) smode->died(ci, NULL);
		mutate(smuts, mut->died(ci, NULL));
		gs.state = CS_DEAD;
		gs.lastdeath = gamemillis;
	}

	int calcdamage(int weap, int &flags, int radial, float size, float dist)
	{
		int damage = weaptype[weap].damage[flags&HIT_ALT ? 1 : 0];
		if(radial) damage = int(damage*(1.f-dist/EXPLOSIONSCALE/max(size, 1e-3f)));
		if(!hithurts(flags)) flags = HIT_WAVE|(flags&HIT_ALT ? HIT_ALT : 0); // so it impacts, but not hurts
		else if((flags&HIT_FULL) && !weaptype[weap].explode[flags&HIT_ALT ? 1 : 0]) flags &= ~HIT_FULL;
		if(hithurts(flags))
		{
			if(flags&HIT_FULL || flags&HIT_HEAD) damage = int(damage*GVAR(damagescale));
			else if(flags&HIT_TORSO) damage = int(damage*0.5f*GVAR(damagescale));
			else if(flags&HIT_LEGS) damage = int(damage*0.25f*GVAR(damagescale));
			else damage = 0;
		}
		else damage = int(damage*GVAR(damagescale));
		return damage;
	}

	void destroyevent::process(clientinfo *ci)
	{
		servstate &gs = ci->state;
		if(isweap(weap))
		{
			if(gs.weapshots[weap][flags&HIT_ALT ? 1 : 0].find(id) < 0) return;
			if(hits.empty()) gs.weapshots[weap][flags&HIT_ALT ? 1 : 0].remove(id);
			else
			{
				loopv(hits)
				{
					hitset &h = hits[i];
					int hflags = flags|h.flags;
					float size = radial ? (hflags&HIT_WAVE ? radial*GVAR(wavepusharea) : radial) : 0.f, dist = float(h.dist)/DMF;
					clientinfo *target = (clientinfo *)getinfo(h.target);
					if(!target || target->state.state != CS_ALIVE || (size && (dist<0 || dist>size)) || target->state.protect(gamemillis, m_protect(gamemode, mutators)))
						continue;
					int damage = calcdamage(weap, hflags, radial, size, dist);
					if(damage > 0 && (hithurts(hflags) || hflags&HIT_WAVE)) dodamage(target, ci, damage, weap, hflags, h.dir);
				}
			}
		}
		else if(weap == -1)
		{
			gs.dropped.remove(id);
			if(sents.inrange(id) && !finditem(id, false)) sents[id].millis = gamemillis;
		}
	}

	void takeammo(clientinfo *ci, int weap, int amt = 1)
	{
		if(weap != WEAP_MELEE) ci->state.ammo[weap] = max(ci->state.ammo[weap]-amt, 0);
	}

	void shotevent::process(clientinfo *ci)
	{
		servstate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !isweap(weap))
		{
			if(GVAR(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: shoot [%d] failed - unexpected message", weap);
			return;
		}
		if(!gs.canshoot(weap, flags, m_weapon(gamemode, mutators), millis))
		{
			if(!gs.canshoot(weap, flags, m_weapon(gamemode, mutators), millis, (1<<WEAP_S_RELOAD)))
			{
				if(weaptype[weap].sub[flags&HIT_ALT ? 1 : 0] && weaptype[weap].max)
					ci->state.ammo[weap] = max(ci->state.ammo[weap]-weaptype[weap].sub[flags&HIT_ALT ? 1 : 0], 0);
				if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: shoot [%d] failed - current state disallows it", weap);
				return;
			}
			else if(gs.weapload[weap] > 0)
			{
				takeammo(ci, weap, gs.weapload[weap]+weaptype[weap].sub[flags&HIT_ALT ? 1 : 0]);
				gs.weapload[weap] = -gs.weapload[weap];
				sendf(-1, 1, "ri5", SV_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap]);
			}
			else return;
		}
		else takeammo(ci, weap, weaptype[weap].sub[flags&HIT_ALT ? 1 : 0]);
		gs.setweapstate(weap, WEAP_S_SHOOT, weaptype[weap].adelay[flags&HIT_ALT ? 1 : 0], millis);
		sendf(-1, 1, "ri8ivx", SV_SHOTFX, ci->clientnum, weap, flags, power, from[0], from[1], from[2], shots.length(), shots.length()*sizeof(ivec)/sizeof(int), shots.getbuf(), ci->clientnum);
		gs.weapshot[weap] = weaptype[weap].sub[flags&HIT_ALT ? 1 : 0];
		gs.shotdamage += weaptype[weap].damage[flags&HIT_ALT ? 1 : 0]*shots.length();
		loopv(shots) gs.weapshots[weap][flags&HIT_ALT ? 1 : 0].add(id);
	}

	void switchevent::process(clientinfo *ci)
	{
		servstate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !isweap(weap))
		{
			if(GVAR(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: switch [%d] failed - unexpected message", weap);
			sendf(ci->clientnum, 1, "ri3", SV_WEAPSELECT, ci->clientnum, gs.weapselect);
			return;
		}
		if(!gs.canswitch(weap, m_weapon(gamemode, mutators), millis, (1<<WEAP_S_SWITCH)))
		{
			if(!gs.canswitch(weap, m_weapon(gamemode, mutators), millis, (1<<WEAP_S_RELOAD)))
			{
				if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: switch [%d] failed - current state disallows it", weap);
				sendf(ci->clientnum, 1, "ri3", SV_WEAPSELECT, ci->clientnum, gs.weapselect);
				return;
			}
			else if(gs.weapload[gs.weapselect] > 0)
			{
				takeammo(ci, gs.weapselect, gs.weapload[gs.weapselect]);
				gs.weapload[gs.weapselect] = -gs.weapload[gs.weapselect];
				sendf(-1, 1, "ri5", SV_RELOAD, ci->clientnum, gs.weapselect, gs.weapload[gs.weapselect], gs.ammo[gs.weapselect]);
			}
			else return;
		}
		gs.weapswitch(weap, millis);
		sendf(-1, 1, "ri3x", SV_WEAPSELECT, ci->clientnum, weap, ci->clientnum);
	}

	void dropevent::process(clientinfo *ci)
	{
		servstate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !isweap(weap))
		{
			if(GVAR(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: drop [%d] failed - unexpected message", weap);
			return;
		}
		int sweap = m_weapon(gamemode, mutators);
		if(!gs.hasweap(weap, sweap, weap == WEAP_GRENADE ? 2 : 0) || (weap != WEAP_GRENADE && m_noitems(gamemode, mutators)))
		{
			if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: drop [%d] failed - current state disallows it", weap);
			return;
		}
		if(weap == WEAP_GRENADE)
		{
			int nweap = -1; // try to keep this weapon
			gs.entid[weap] = -1;
			gs.weapshots[WEAP_GRENADE][0].add(-1);
			takeammo(ci, WEAP_GRENADE, 1);
			if(!gs.hasweap(weap, sweap))
			{
				nweap = gs.bestweap(sweap, true);
				gs.weapswitch(nweap, millis);
			}
			else gs.setweapstate(weap, WEAP_S_SHOOT, weaptype[weap].adelay[0], millis);
			sendf(-1, 1, "ri6", SV_DROP, ci->clientnum, nweap, 1, weap, -1);
			return;
		}
		else if(!sents.inrange(gs.entid[weap]) || (sents[gs.entid[weap]].attrs[1]&WEAP_F_FORCED))
		{
			if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: drop [%d] failed - not droppable entity", weap);
			return;
		}
		int dropped = gs.entid[weap];
		gs.ammo[weap] = gs.entid[weap] = -1;
		int nweap = gs.bestweap(sweap, true); // switch to best weapon
		if(w_carry(weap, sweap)) sents[dropped].millis = gamemillis+GVAR(itemspawntime);
		gs.dropped.add(dropped);
		gs.weapswitch(nweap, millis);
		sendf(-1, 1, "ri6", SV_DROP, ci->clientnum, nweap, 1, weap, dropped);
	}

	void reloadevent::process(clientinfo *ci)
	{
		servstate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !isweap(weap))
		{
			if(GVAR(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: reload [%d] failed - unexpected message", weap);
			sendf(ci->clientnum, 1, "ri5", SV_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap]);
			return;
		}
		if(!gs.canreload(weap, m_weapon(gamemode, mutators), millis))
		{
			if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: reload [%d] failed - current state disallows it", weap);
			sendf(ci->clientnum, 1, "ri5", SV_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap]);
			return;
		}
		gs.setweapstate(weap, WEAP_S_RELOAD, weaptype[weap].rdelay, millis);
		int oldammo = gs.ammo[weap];
		gs.ammo[weap] = min(max(gs.ammo[weap], 0) + weaptype[weap].add, weaptype[weap].max);
		gs.weapload[weap] = gs.ammo[weap]-oldammo;
		sendf(-1, 1, "ri5x", SV_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap], ci->clientnum);
	}

	void useevent::process(clientinfo *ci)
	{
		servstate &gs = ci->state;
		if(gs.state != CS_ALIVE || !sents.inrange(ent) || enttype[sents[ent].type].usetype != EU_ITEM || m_noitems(gamemode, mutators))
		{
			if(GVAR(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: use [%d] failed - unexpected message", ent);
			return;
		}
		int sweap = m_weapon(gamemode, mutators), attr = sents[ent].type == WEAPON ? w_attr(gamemode, sents[ent].attrs[0], sweap) : sents[ent].attrs[0];
		if(!gs.canuse(sents[ent].type, attr, sents[ent].attrs, sweap, millis, (1<<WEAP_S_SWITCH)))
		{
			if(!gs.canuse(sents[ent].type, attr, sents[ent].attrs, sweap, millis, (1<<WEAP_S_RELOAD)))
			{
				if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: use [%d] failed - current state disallows it", ent);
				return;
			}
			else if(gs.weapload[gs.weapselect] > 0)
			{
				takeammo(ci, gs.weapselect, gs.weapload[gs.weapselect]);
				gs.weapload[gs.weapselect] = -gs.weapload[gs.weapselect];
				sendf(-1, 1, "ri5", SV_RELOAD, ci->clientnum, gs.weapselect, gs.weapload[gs.weapselect], gs.ammo[gs.weapselect]);
			}
			else return;
		}
		if(!sents[ent].spawned && !(sents[ent].attrs[1]&WEAP_F_FORCED))
		{
			bool found = false;
			loopv(clients)
			{
				clientinfo *cp = clients[i];
				if(cp->state.dropped.projs.find(ent) >= 0)
				{
					cp->state.dropped.remove(ent);
					found = true;
				}
			}
			if(!found)
			{
				if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: use [%d] failed - doesn't seem to be spawned anywhere", ent);
				return;
			}
		}

		int weap = -1, dropped = -1;
		if(sents[ent].type == WEAPON && gs.ammo[attr] < 0 && w_carry(attr, sweap) && gs.carry(sweap) >= GVAR(maxcarry))
			weap = gs.drop(sweap, attr);
		if(weap != WEAP_MELEE && isweap(weap))
		{
			dropped = gs.entid[weap];
			gs.setweapstate(weap, WEAP_S_SWITCH, WEAPSWITCHDELAY, millis);
			gs.ammo[weap] = gs.entid[weap] = -1;
		}
		gs.useitem(ent, sents[ent].type, attr, sents[ent].attrs, sweap, millis);
		if(sents.inrange(dropped))
		{
			gs.dropped.add(dropped);
			if(!(sents[dropped].attrs[1]&WEAP_F_FORCED))
			{
				sents[dropped].spawned = false;
				sents[dropped].millis = gamemillis+GVAR(itemspawntime);
			}
		}
		if(!(sents[ent].attrs[1]&WEAP_F_FORCED))
		{
			sents[ent].spawned = false;
			sents[ent].millis = gamemillis+GVAR(itemspawntime);
		}
		sendf(-1, 1, "ri6", SV_ITEMACC, ci->clientnum, ent, sents[ent].spawned ? 1 : 0, weap, dropped);
	}

    bool gameevent::flush(clientinfo *ci, int fmillis)
    {
        process(ci);
        return true;
    }

    bool timedevent::flush(clientinfo *ci, int fmillis)
    {
        if(millis > fmillis) return false;
        else if(millis >= ci->lastevent)
        {
            ci->lastevent = millis;
            process(ci);
        }
        return true;
    }

	void flushevents(clientinfo *ci, int millis)
	{
		while(ci->events.length())
		{
			gameevent *ev = ci->events[0];
            if(ev->flush(ci, millis)) clearevent(ci);
            else break;
		}
	}

	void processevents()
	{
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			flushevents(ci, gamemillis);
		}
	}

	void cleartimedevents(clientinfo *ci)
	{
		int keep = 0;
		loopv(ci->events)
		{
            if(ci->events[i]->keepable())
			{
			    if(keep < i)
                {
                    for(int j = keep; j < i; j++) delete ci->events[j];
                    ci->events.remove(keep, i - keep);
                    i = keep;
                }
				keep = i+1;
				continue;
			}
		}
        while(ci->events.length() > keep) delete ci->events.pop();
	}

	void waiting(clientinfo *ci, int doteam, int drop, bool exclude)
	{
		if(ci->state.state != CS_SPECTATOR && ci->state.state != CS_EDITING)
		{
			if(m_campaign(gamemode) && ci->state.cpnodes.empty())
			{
				int maxnodes = -1;
				loopv(clients)
				{
					clientinfo *oi = clients[i];
					if(oi->clientnum >= 0 && oi->name[0] && oi->state.aitype < AI_START && (!clients.inrange(maxnodes) || oi->state.cpnodes.length() > clients[maxnodes]->state.cpnodes.length()))
						maxnodes = i;
				}
				if(clients.inrange(maxnodes)) loopv(clients[maxnodes]->state.cpnodes) ci->state.cpnodes.add(clients[maxnodes]->state.cpnodes[i]);
			}
			if(ci->state.state == CS_ALIVE)
			{
				dropitems(ci, drop);
				if(smode) smode->died(ci);
				mutate(smuts, mut->died(ci));
				ci->state.lastdeath = gamemillis;
			}
			if(exclude) sendf(-1, 1, "ri2x", SV_WAITING, ci->clientnum, ci->clientnum);
			else sendf(-1, 1, "ri2", SV_WAITING, ci->clientnum);
			ci->state.state = CS_WAITING;
			ci->state.weapreset(false);
			if(m_arena(gamemode, mutators) && ci->state.arenaweap < 0 && ci->state.aitype < 0) sendf(ci->clientnum, 1, "ri", SV_ARENAWEAP);
			if(doteam && (doteam == 2 || !isteam(gamemode, mutators, ci->team, TEAM_FIRST)))
				setteam(ci, chooseteam(ci, ci->team), false, true);
		}
	}

	void hashpassword(int cn, int sessionid, const char *pwd, char *result, int maxlen)
	{
		char buf[2*sizeof(string)];
		formatstring(buf)("%d %d ", cn, sessionid);
		copystring(&buf[strlen(buf)], pwd);
        if(!hashstring(buf, result, maxlen)) *result = '\0';
	}

	bool checkpassword(clientinfo *ci, const char *wanted, const char *given)
	{
		string hash;
		hashpassword(ci->clientnum, ci->sessionid, wanted, hash, sizeof(string));
		return !strcmp(hash, given);
    }

	#include "auth.h"

	int triggertime(int i)
	{
		if(sents.inrange(i)) switch(sents[i].type)
		{
			case TRIGGER: case MAPMODEL: case PARTICLES: case MAPSOUND: case TELEPORT: case PUSHER: return 1000; break;
			default: break;
		}
		return 0;
	}

	void checkents()
	{
		loopv(sents) switch(sents[i].type)
		{
			case TRIGGER:
			{
				if(sents[i].attrs[1] == TR_LINK && sents[i].spawned && gamemillis >= sents[i].millis && (sents[i].attrs[4] == triggerid || !sents[i].attrs[4]))
				{
					sents[i].spawned = false;
					sents[i].millis = gamemillis+(triggertime(i)*2);
					sendf(-1, 1, "ri3", SV_TRIGGER, i, 0);
					loopvj(sents[i].kin) if(sents.inrange(sents[i].kin[j]))
					{
						sents[sents[i].kin[j]].spawned = sents[i].spawned;
						sents[sents[i].kin[j]].millis = sents[i].millis;
					}
				}
				break;
			}
			default:
			{
				if(enttype[sents[i].type].usetype == EU_ITEM && hasitem(i) && !finditem(i, true, true))
				{
					loopvk(clients)
					{
						clientinfo *ci = clients[k];
						ci->state.dropped.remove(i);
						loopj(WEAP_MAX) if(ci->state.entid[j] == i)
							ci->state.entid[j] = -1;
					}
					sents[i].spawned = true;
					sents[i].millis = gamemillis+GVAR(itemspawntime);
					sendf(-1, 1, "ri2", SV_ITEMSPAWN, i);
				}
				break;
			}
		}
	}

	void checkclients()
	{
		loopv(clients) if(clients[i]->name[0] && clients[i]->online)
		{
			clientinfo *ci = clients[i];
			if(ci->state.state == CS_ALIVE)
			{
				if(GVAR(fireburntime) && ci->state.lastfire)
				{
					if(gamemillis-ci->state.lastfire <= GVAR(fireburntime))
					{
						if(gamemillis-ci->state.lastfireburn >= GVAR(fireburndelay))
						{
							clientinfo *co = (clientinfo *)getinfo(ci->state.lastfireowner);
							dodamage(ci, co ? co : ci, GVAR(fireburndamage), -1, HIT_BURN);
							ci->state.lastfireburn += GVAR(fireburndelay);
						}
						continue;
					}
					else ci->state.lastfire = ci->state.lastfireburn = 0;
				}
				if(!m_regen(gamemode, mutators) || ci->state.aitype >= AI_START) continue;
				int total = m_health(gamemode, mutators), amt = GVAR(regenhealth), delay = ci->state.lastregen ? GVAR(regentime) : GVAR(regendelay);
				if(smode) smode->regen(ci, total, amt, delay);
				if(delay && (ci->state.health < total || ci->state.health > total) && gamemillis-(ci->state.lastregen ? ci->state.lastregen : ci->state.lastpain) >= delay)
				{
					int low = 0;
					if(ci->state.health > total) { amt = -GVAR(regenhealth); total = ci->state.health; low = m_health(gamemode, mutators); }
					int rgn = ci->state.health, heal = clamp(ci->state.health+amt, low, total), eff = heal-rgn;
					if(eff)
					{
						ci->state.health = heal; ci->state.lastregen = gamemillis;
						sendf(-1, 1, "ri4", SV_REGEN, ci->clientnum, ci->state.health, eff);
					}
				}
			}
			else if(ci->state.state == CS_WAITING)
			{
				if(m_arena(gamemode, mutators) && ci->state.arenaweap < 0 && ci->state.aitype < 0) continue;
				if(m_trial(gamemode) && ci->state.cpmillis < 0) continue;
				int delay = m_delay(gamemode, mutators);
				if(ci->state.aitype >= AI_START)
				{
					if(ci->state.lastdeath)
					{
						if(m_campaign(gamemode)) continue;
						else if(!m_duke(gamemode, mutators)) delay = 30000;
					}
				}
				if(delay && ci->state.respawnwait(gamemillis, delay)) continue;
				int nospawn = 0;
				if(smode && !smode->canspawn(ci, false)) { nospawn++; }
				mutate(smuts, if (!mut->canspawn(ci, false)) { nospawn++; });
				if(!nospawn)
				{
					if(ci->state.lastdeath) flushevents(ci, ci->state.lastdeath + DEATHMILLIS);
					cleartimedevents(ci);
					ci->state.state = CS_DEAD; // safety
					ci->state.respawn(gamemillis, m_health(gamemode, mutators));
					sendspawn(ci);
				}
			}
		}
	}

	void cleanbans()
	{
		while(bannedips.length() && bannedips[0].time-totalmillis>4*60*60000)
			bannedips.remove(0);
	}

	void serverupdate()
	{
		if(numclients())
		{
			if(!paused) gamemillis += curtime;
			if(m_demo(gamemode)) readdemo();
			else if(!paused && !interm)
			{
				processevents();
				checkents();
				checklimits();
				checkclients();
				if(smode) smode->update();
				mutate(smuts, mut->update());
			}
			cleanbans();
			loopv(connects) if(totalmillis-connects[i]->connectmillis > 15000) disconnect_client(connects[i]->clientnum, DISC_TIMEOUT);

			if(masterupdate)
			{
				loopv(clients) sendf(-1, 1, "ri3", SV_CURRENTMASTER, clients[i]->clientnum, clients[i]->privilege);
				masterupdate = false;
			}

			if(interm && gamemillis >= interm) // wait then call for next map
			{
				if(GVAR(votelimit) && !maprequest)
				{
					if(demorecord) enddemorecord();
					sendf(-1, 1, "ri", SV_NEWGAME);
					maprequest = true;
					interm = gamemillis+GVAR(votelimit);
				}
				else
				{
					interm = 0;
					checkvotes(true);
				}
			}
            if(shouldcheckvotes) checkvotes();
		}
		else if(!GVAR(resetbansonend)) cleanbans();
		aiman::checkai();
		auth::update();
	}

    bool allowbroadcast(int n)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        return ci && ci->connected && ci->state.aitype < 0;
    }

    int peerowner(int n)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        if(ci) return ci->state.aitype >= 0 ? ci->state.ownernum : ci->clientnum;
        return -1;
    }

    int reserveclients() { return 3; }

    int allowconnect(clientinfo *ci, const char *pwd)
    {
        if(ci->local) return DISC_NONE;
        if(m_demo(gamemode)) return DISC_PRIVATE;
        if(serverpass[0])
        {
            if(!checkpassword(ci, serverpass, pwd)) return DISC_PRIVATE;
            return DISC_NONE;
        }
        if(adminpass[0] && checkpassword(ci, adminpass, pwd)) return DISC_NONE;
        if(numclients() >= serverclients) return DISC_MAXCLIENTS;
        uint ip = getclientip(ci->clientnum);
        loopv(bannedips) if(bannedips[i].ip == ip) return DISC_IPBAN;
        if(mastermode >= MM_PRIVATE && allowedips.find(ip) < 0) return DISC_PRIVATE;
        return DISC_NONE;
    }

	int clientconnect(int n, uint ip, bool local)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		ci->clientnum = n;
        ci->connectmillis = totalmillis;
        ci->sessionid = (rnd(0x1000000)*((totalmillis%10000)+1))&0xFFFFFF;
        ci->local = local;
		connects.add(ci);
        if(!local)
        {
        	if(m_demo(gamemode) || servertype <= 0) return DISC_PRIVATE;
#ifndef STANDALONE
        	bool haslocal = false;
        	loopv(clients) if(clients[i]->local) { haslocal = true; break; }
        	if(!haslocal) return DISC_PRIVATE;
#endif
        }
        sendservinit(ci);
		return DISC_NONE;
	}

	void clientdisconnect(int n, bool local)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		bool complete = !numclients(n, false, -1);
        if(ci->connected)
        {
        	loopv(clients) if(clients[i] != ci)
        	{
        		loopvk(clients[i]->state.fraglog) if(clients[i]->state.fraglog[k] == ci->clientnum)
					clients[i]->state.fraglog.remove(k--);
        	}
		    if(ci->state.state == CS_ALIVE) dropitems(ci, 0);
		    if(ci->privilege) auth::setmaster(ci, false);
		    if(smode) smode->leavegame(ci, true);
		    mutate(smuts, mut->leavegame(ci, true));
		    ci->state.timeplayed += lastmillis-ci->state.lasttimeplayed;
		    savescore(ci);
		    sendf(-1, 1, "ri2", SV_DISCONNECT, n);
		    ci->connected = false;
		    if(ci->name[0]) relayf(2, "\fo%s has left the game", colorname(ci));
		    aiman::removeai(ci, complete);
		    if(!complete) aiman::dorefresh = true;
		    clients.removeobj(ci);
        }
        else connects.removeobj(ci);
		if(complete) cleanup();
		else shouldcheckvotes = true;
	}

	#include "extinfo.h"
    void queryreply(ucharbuf &req, ucharbuf &p)
	{
        if(!getint(req))
        {
            extqueryreply(req, p);
            return;
        }
		putint(p, numclients());
		putint(p, 6);					// number of attrs following
		putint(p, GAMEVERSION);			// 1
		putint(p, gamemode);			// 2
		putint(p, mutators);			// 3
		putint(p, minremain);			// 4
		putint(p, serverclients);		// 5
		putint(p, serverpass[0] ? MM_PASSWORD : (m_demo(gamemode) ? MM_PRIVATE : mastermode)); // 6
		sendstring(smapname, p);
		if(*serverdesc) sendstring(serverdesc, p);
		else
		{
			#ifdef STANDALONE
			sendstring("unnamed", p);
			#else
            const char *cname = client::getname();
            if(!cname || !cname[0]) cname = "unnamed";
            sendstring(cname, p);
			#endif
		}
		sendqueryreply(p);
	}

	bool receivefile(int sender, uchar *data, int len)
	{
		clientinfo *ci = (clientinfo *)getinfo(sender);
		ucharbuf p(data, len);
		int type = getint(p), n = 0;
		data += p.length();
		len -= p.length();
		switch(type)
		{
			case SV_SENDMAPFILE: case SV_SENDMAPSHOT: case SV_SENDMAPCONFIG:
				n = type-SV_SENDMAPFILE;
				break;
			default: srvmsgf(sender, "bad map file type %d"); return false;
		}
		if(mapdata[n])
		{
			if(ci != choosebestclient())
			{
				srvmsgf(sender, "sorry, the map isn't needed from you");
				return false;
			}
            DELETEP(mapdata[n]);
		}
		if(!len)
		{
			srvmsgf(sender, "you sent a zero length packet for map data");
			return false;
		}
		mapdata[n] = opentempfile(((const char *[3]){ "mapdata", "mapshot", "mapconf" })[n], "w+b");
        if(!mapdata[n])
        {
        	srvmsgf(sender, "failed to open temporary file for map");
        	return false;
		}
		mapsending = true;
		mapdata[n]->write(data, len);
		return n == 2;
	}

	int checktype(int type, clientinfo *ci)
	{
		// only allow edit messages in coop-edit mode
		if(type >= SV_EDITENT && type <= SV_NEWMAP && !m_edit(gamemode)) return -1;
		// server only messages
		static int servtypes[] = { SV_SERVERINIT, SV_CLIENTINIT, SV_WELCOME, SV_NEWGAME, SV_MAPCHANGE, SV_SERVMSG, SV_DAMAGE, SV_SHOTFX, SV_DIED, SV_POINTS, SV_SPAWNSTATE, SV_ITEMACC, SV_ITEMSPAWN, SV_TIMEUP, SV_DISCONNECT, SV_CURRENTMASTER, SV_PONG, SV_RESUME, SV_SCORE, SV_FLAGINFO, SV_ANNOUNCE, SV_SENDDEMOLIST, SV_SENDDEMO, SV_DEMOPLAYBACK, SV_REGEN, SV_SCOREFLAG, SV_RETURNFLAG, SV_CLIENT, SV_AUTHCHAL };
		if(ci) loopi(sizeof(servtypes)/sizeof(int)) if(type == servtypes[i]) return -1;
		return type;
	}

	static void freecallback(ENetPacket *packet)
	{
		extern void cleanworldstate(ENetPacket *packet);
		cleanworldstate(packet);
	}

	void cleanworldstate(ENetPacket *packet)
	{
		loopv(worldstates)
		{
			worldstate *ws = worldstates[i];
            if(ws->positions.inbuf(packet->data) || ws->messages.inbuf(packet->data)) ws->uses--;
			else continue;
			if(!ws->uses)
			{
				delete ws;
				worldstates.remove(i);
			}
			break;
		}
	}

	bool buildworldstate()
	{
		worldstate &ws = *new worldstate;
		loopv(clients)
		{
			clientinfo &ci = *clients[i];
			if(ci.position.empty()) ci.posoff = -1;
			else
			{
				ci.posoff = ws.positions.length();
				loopvj(ci.position) ws.positions.add(ci.position[j]);
			}
			if(ci.messages.empty()) ci.msgoff = -1;
			else
			{
				ci.msgoff = ws.messages.length();
				ucharbuf p = ws.messages.reserve(16);
				putint(p, SV_CLIENT);
				putint(p, ci.clientnum);
				putuint(p, ci.messages.length());
				ws.messages.addbuf(p);
				loopvj(ci.messages) ws.messages.add(ci.messages[j]);
				ci.msglen = ws.messages.length() - ci.msgoff;
			}
		}
		int psize = ws.positions.length(), msize = ws.messages.length();
		if(psize) recordpacket(0, ws.positions.getbuf(), psize);
		if(msize) recordpacket(1, ws.messages.getbuf(), msize);
		loopi(psize) { uchar c = ws.positions[i]; ws.positions.add(c); }
		loopi(msize) { uchar c = ws.messages[i]; ws.messages.add(c); }
		ws.uses = 0;
		loopv(clients)
		{
			clientinfo &ci = *clients[i];
			ENetPacket *packet;
			if(ci.state.aitype < 0 && psize && (ci.posoff<0 || psize-ci.position.length()>0))
			{
				packet = enet_packet_create(&ws.positions[ci.posoff<0 ? 0 : ci.posoff+ci.position.length()],
											ci.posoff<0 ? psize : psize-ci.position.length(),
											ENET_PACKET_FLAG_NO_ALLOCATE);
				sendpacket(ci.clientnum, 0, packet);
				if(!packet->referenceCount) enet_packet_destroy(packet);
				else { ++ws.uses; packet->freeCallback = freecallback; }
			}
			ci.position.setsizenodelete(0);

			if(ci.state.aitype < 0 && msize && (ci.msgoff<0 || msize-ci.msglen>0))
			{
				packet = enet_packet_create(&ws.messages[ci.msgoff<0 ? 0 : ci.msgoff+ci.msglen],
											ci.msgoff<0 ? msize : msize-ci.msglen,
											(reliablemessages ? ENET_PACKET_FLAG_RELIABLE : 0) | ENET_PACKET_FLAG_NO_ALLOCATE);
				sendpacket(ci.clientnum, 1, packet);
				if(!packet->referenceCount) enet_packet_destroy(packet);
				else { ++ws.uses; packet->freeCallback = freecallback; }
			}
			ci.messages.setsizenodelete(0);
		}
		reliablemessages = false;
		if(!ws.uses)
		{
			delete &ws;
			return false;
		}
		else
		{
			worldstates.add(&ws);
			return true;
		}
	}

	bool sendpackets()
	{
		if(clients.empty()) return false;
		enet_uint32 millis = enet_time_get()-lastsend;
		if(millis<33) return false;
		bool flush = buildworldstate();
		lastsend += millis - (millis%33);
		return flush;
	}

	void parsepacket(int sender, int chan, packetbuf &p)	 // has to parse exactly each byte of the packet
	{
		if(sender<0) return;
        char text[MAXTRANS];
        int type = -1, prevtype = -1;
        clientinfo *ci = sender>=0 ? (clientinfo *)getinfo(sender) : NULL;
        if(ci && !ci->connected)
        {
            if(chan==0) return;
            else if(chan!=1 || getint(p)!=SV_CONNECT) { disconnect_client(sender, DISC_TAGT); return; }
            else
            {
                getstring(text, p);
                //filtertext(text, text, true, MAXNAMELEN);
                if(!text[0]) copystring(text, "unnamed");
				filtertext(text, text, true, MAXNAMELEN);
                copystring(ci->name, text, MAXNAMELEN+1);

                getstring(text, p);
                int disc = allowconnect(ci, text);
                if(disc)
                {
                    disconnect_client(sender, disc);
                    return;
                }

                connects.removeobj(ci);
                clients.add(ci);

                ci->connected = true;
                masterupdate = true;
                ci->state.lasttimeplayed = lastmillis;

                sendwelcome(ci);
                if(restorescore(ci)) sendresume(ci);
				sendinitclient(ci);
                relayf(2, "\fg%s has joined the game", colorname(ci));
            }
        }
		else if(chan==2)
		{
			if(receivefile(sender, p.buf, p.maxlen))
			{
				mapsending = false;
				sendf(-1, 1, "ri", SV_SENDMAP);
			}
			return;
		}
		if(p.packet->flags&ENET_PACKET_FLAG_RELIABLE) reliablemessages = true;
		#define QUEUE_MSG { while(curmsg<p.length()) ci->messages.add(p.buf[curmsg++]); }
		#define QUEUE_BUF(body) { curmsg = p.length(); body; }
        #define QUEUE_INT(n) QUEUE_BUF(putint(ci->messages, n))
        #define QUEUE_UINT(n) QUEUE_BUF(putuint(ci->messages, n))
        #define QUEUE_FLT(n) QUEUE_BUF(putfloat(ci->messages, n))
        #define QUEUE_STR(text) QUEUE_BUF(sendstring(text, ci->messages))

		int curmsg;
        while((curmsg = p.length()) < p.maxlen)
		{
			int curtype = getint(p);
			prevtype = type;
			switch(type = checktype(curtype, ci))
			{
				case SV_POS:
				{
					int lcn = getint(p);
					if(lcn<0)
					{
						disconnect_client(sender, DISC_CN);
						return;
					}

					bool havecn = true;
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum != sender && cp->state.ownernum != sender))
						havecn = false;

					vec oldpos, pos;
					loopi(3) pos[i] = getuint(p)/DMF;
					if(havecn)
					{
						oldpos = cp->state.o;
						cp->state.o = pos;
					}
                    getuint(p);
                    loopi(5) getint(p);
					int physstate = getuint(p);
					if(physstate&0x20) loopi(2) getint(p);
					if(physstate&0x10) getint(p);
                    int flags = getuint(p);
                    if(flags&0x20) { getuint(p); getint(p); }
					if(havecn && (cp->state.state==CS_ALIVE || cp->state.state==CS_EDITING))
					{
						cp->position.setsizenodelete(0);
						while(curmsg<p.length()) cp->position.add(p.buf[curmsg++]);
					}
					if(havecn && cp->state.state==CS_ALIVE)
					{
						if(smode) smode->moved(cp, oldpos, cp->state.o);
						mutate(smuts, mut->moved(cp, oldpos, cp->state.o));
					}
					break;
				}

				case SV_PHYS:
				{
					int lcn = getint(p), idx = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					if(idx == SPHY_EXTINGUISH)
					{
						if(!cp->state.lastfire || gamemillis-cp->state.lastfire > GVAR(fireburntime))
							break;
						cp->state.lastfire = cp->state.lastfireburn = 0;
					}
					QUEUE_MSG;
					break;
				}

				case SV_EDITMODE:
				{
					int val = getint(p);
					if((!val && ci->state.state != CS_EDITING) || !m_edit(gamemode) || ci->state.aitype >= 0) break;
					//if(val && ci->state.state != CS_ALIVE) break;
					ci->state.dropped.reset();
					loopk(WEAP_MAX) loopj(2) ci->state.weapshots[k][j].reset();
					ci->state.editspawn(gamemillis, m_weapon(gamemode, mutators), m_health(gamemode, mutators), m_arena(gamemode, mutators), GVAR(spawngrenades) >= (m_insta(gamemode, mutators) ? 2 : 1));
					if(val)
					{
						if(smode) smode->leavegame(ci);
						mutate(smuts, mut->leavegame(ci));
						ci->state.state = CS_EDITING;
						ci->events.deletecontentsp();
					}
					else
					{
						ci->state.state = CS_ALIVE;
						if(smode) smode->entergame(ci);
						mutate(smuts, mut->entergame(ci));
					}
					QUEUE_MSG;
					break;
				}

				case SV_MAPCRC:
				{
					getstring(text, p);
					int crc = getint(p);
					if(!ci) break;
					if(strcmp(text, smapname))
					{
						if(ci->clientmap[0])
						{
							ci->clientmap[0] = '\0';
							ci->mapcrc = 0;
						}
						else if(ci->mapcrc > 0) ci->mapcrc = 0;
						break;
					}
					copystring(ci->clientmap, text);
					ci->mapcrc = text[0] ? crc : 1;
					checkmaps();
					break;
				}

				case SV_CHECKMAPS:
					checkmaps(sender);
					break;

				case SV_TRYSPAWN:
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					if(cp->state.state != CS_DEAD || cp->state.lastrespawn >= 0 || gamemillis-cp->state.lastdeath <= DEATHMILLIS) break;
					if(!ci->clientmap[0] && !ci->mapcrc)
					{
						ci->mapcrc = -1;
						checkmaps();
					}
					if(smode) smode->canspawn(cp, true);
					mutate(smuts, mut->canspawn(cp, true));
					cp->state.state = CS_DEAD;
					waiting(cp, 1, 1);
					break;
				}

				case SV_ARENAWEAP:
				{
					int lcn = getint(p), aweap = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					cp->state.arenaweap = aweap;
					break;
				}

				case SV_WEAPSELECT:
				{
					int lcn = getint(p), id = getint(p), weap = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
                    switchevent *ev = new switchevent;
					ev->id = id;
					ev->weap = weap;
					ev->millis = cp->getmillis(gamemillis, ev->id);
                    cp->addevent(ev);
					break;
				}

				case SV_SPAWN:
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					if((cp->state.state!=CS_ALIVE && cp->state.state!=CS_DEAD && cp->state.state!=CS_WAITING) || cp->state.lastrespawn < 0)
						break;
					cp->state.lastrespawn = -1;
					cp->state.state = CS_ALIVE;
					if(smode) smode->spawned(cp);
					mutate(smuts, mut->spawned(cp););
					QUEUE_BUF({
						putint(ci->messages, SV_SPAWN);
						putint(ci->messages, cp->clientnum);
						sendstate(cp->state, ci->messages);
					});
					break;
				}

				case SV_SUICIDE:
				{
					int lcn = getint(p), flags = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
                    suicideevent *ev = new suicideevent;
					ev->flags = flags;
                    cp->addevent(ev);
					break;
				}

				case SV_SHOOT:
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
                    shotevent *ev = new shotevent;
					ev->id = getint(p);
					ev->weap = getint(p);
					ev->flags = getint(p);
					ev->power = getint(p);
					if(havecn) ev->millis = cp->getmillis(gamemillis, ev->id);
					loopk(3) ev->from[k] = getint(p);
					ev->num = getint(p);
					loopj(ev->num)
					{
                        if(p.overread() || !isweap(ev->weap)) break;
						ivec &dest = ev->shots.add();
						loopk(3) dest[k] = getint(p);
					}
                    if(havecn) cp->addevent(ev);
                    else delete ev;
					break;
				}

				case SV_DROP:
				{ // gee this looks familiar
					int lcn = getint(p), id = getint(p), weap = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum != ci->clientnum && cp->state.ownernum != ci->clientnum))
						break;
                    dropevent *ev = new dropevent;
					ev->id = id;
					ev->weap = weap;
					ev->millis = cp->getmillis(gamemillis, ev->id);
                    cp->events.add(ev);
					break;
				}

				case SV_RELOAD:
				{
					int lcn = getint(p), id = getint(p), weap = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum != ci->clientnum && cp->state.ownernum != ci->clientnum))
						break;
                    reloadevent *ev = new reloadevent;
					ev->id = id;
					ev->weap = weap;
					ev->millis = cp->getmillis(gamemillis, ev->id);
                    cp->events.add(ev);
					break;
				}

				case SV_DESTROY: // cn millis weap id radial hits
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
                    destroyevent *ev = new destroyevent;
					ev->id = getint(p);
					if(havecn) ev->millis = cp->getmillis(gamemillis, ev->id); // this is the event millis
					ev->weap = getint(p);
					ev->flags = getint(p);
					ev->id = getint(p); // this is the actual id
					ev->radial = getint(p);
					int hits = getint(p);
					loopj(hits)
					{
                        if(p.overread()) break;
                        if(!havecn)
                        {
                            loopi(7) getint(p);
                            continue;
                        }
						hitset &hit = ev->hits.add();
						hit.flags = getint(p);
						hit.target = getint(p);
						hit.id = getint(p);
						hit.dist = getint(p);
						loopk(3) hit.dir[k] = getint(p);
					}
                    if(havecn) cp->events.add(ev);
                    else delete ev;
					break;
				}

				case SV_ITEMUSE:
				{
					int lcn = getint(p), id = getint(p), ent = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
                    useevent *ev = new useevent;
					ev->id = id;
					ev->ent = ent;
					ev->millis = cp->getmillis(gamemillis, ev->id);
                    cp->events.add(ev);
					break;
				}

				case SV_TRIGGER:
				{
					int lcn = getint(p), ent = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					if(sents.inrange(ent))
					{
						if(sents[ent].type == CHECKPOINT)
						{
							if(cp->state.cpnodes.find(ent) < 0 && (sents[ent].attrs[4] == triggerid || !sents[ent].attrs[4]) && m_check(sents[ent].attrs[3], gamemode))
							{
								if(m_trial(gamemode)) switch(sents[ent].attrs[5])
								{
									case CP_LAST: case CP_FINISH:
									{
										int laptime = gamemillis-cp->state.cpmillis;
										if(cp->state.cptime <= 0 || laptime < cp->state.cptime)
										{
											cp->state.cptime = laptime;
											if(sents[ent].attrs[5] == CP_FINISH) { cp->state.cpmillis = -gamemillis; waiting(cp, 0, 0); }
										}
										sendf(-1, 1, "ri4", SV_CHECKPOINT, cp->clientnum, laptime, cp->state.cptime);
									}
									case CP_RESPAWN: if(sents[ent].attrs[5] == CP_RESPAWN && cp->state.cpmillis) break;
									case CP_START:
									{
										cp->state.cpmillis = gamemillis;
										cp->state.cpnodes.setsize(0);
									}
									default: break;
								}
								cp->state.cpnodes.add(ent);
							}
						}
						else if(sents[ent].type == TRIGGER)
						{
							bool commit = false, kin = false;
							if(sents[ent].attrs[4] == triggerid || !sents[ent].attrs[4]) switch(sents[ent].attrs[1])
							{
								case TR_TOGGLE:
								{
									if(!sents[ent].spawned || sents[ent].attrs[2] != TA_AUTO)
									{
										sents[ent].millis = gamemillis+(triggertime(ent)*2);
										sents[ent].spawned = !sents[ent].spawned;
										commit = kin = true;
									}
									//else sendf(cp->clientnum, 1, "ri3", SV_TRIGGER, ent, sents[ent].spawned ? 1 : 0);
									break;
								}
								case TR_ONCE: if(sents[ent].spawned) break;
								case TR_LINK:
								{
									sents[ent].millis = gamemillis+(triggertime(ent)*2); kin = true;
									if(!sents[ent].spawned)
									{
										sents[ent].spawned = true;
										commit = true;
									}
									//else sendf(cp->clientnum, 1, "ri3", SV_TRIGGER, ent, sents[ent].spawned ? 1 : 0);
									break;
								}
								case TR_EXIT:
								{
									if(sents[ent].spawned) break;
									if(m_campaign(gamemode) || m_lobby(gamemode))
									{
										sents[ent].spawned = true;
										startintermission();
									}
								}
							}
							if(commit) sendf(-1, 1, "ri3", SV_TRIGGER, ent, sents[ent].spawned ? 1 : 0);
							if(kin) loopvj(sents[ent].kin) if(sents.inrange(sents[ent].kin[j]))
							{
								sents[sents[ent].kin[j]].spawned = sents[ent].spawned;
								sents[sents[ent].kin[j]].millis = sents[ent].millis;
							}
						}
					}
					else if(GVAR(serverdebug)) srvmsgf(cp->clientnum, "sync error: cannot trigger %d - not a trigger", ent);
					break;
				}

				case SV_TEXT:
				{
					int lcn = getint(p), flags = getint(p);
					getstring(text, p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					loopv(clients)
					{
						clientinfo *t = clients[i];
						if(t == cp || !allowbroadcast(t->clientnum) || (flags&SAY_TEAM && cp->team != t->team)) continue;
						sendf(t->clientnum, 1, "ri3s", SV_TEXT, cp->clientnum, flags, text);
					}
					defformatstring(m)("%s", colorname(cp));
					if(flags&SAY_TEAM)
					{
						defformatstring(t)(" (\fs%s%s\fS)", teamtype[cp->team].chat, teamtype[cp->team].name);
						concatstring(m, t);
					}
					if(flags&SAY_ACTION) relayf(0, "\fv* \fs%s\fS \fs\fv%s\fS", m, text);
					else relayf(0, "\fa<\fs\fw%s\fS> \fs\fw%s\fS", m, text);
					break;
				}

				case SV_COMMAND:
				{
					int lcn = getint(p), nargs = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					string cmd;
					getstring(cmd, p);
					getstring(text, p);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					parsecommand(cp, nargs, cmd, text);
					break;
				}

				case SV_SWITCHNAME:
				{
					QUEUE_MSG;
					getstring(text, p);
					if(!text[0]) copystring(text, "unnamed");
					filtertext(text, text, true, MAXNAMELEN);
					copystring(ci->name, text, MAXNAMELEN+1);
					QUEUE_STR(ci->name);
					break;
				}

				case SV_SWITCHTEAM:
				{
					int team = getint(p);
					if(((ci->state.state == CS_SPECTATOR || ci->state.state == CS_EDITING) && team != TEAM_NEUTRAL) || !isteam(gamemode, mutators, team, TEAM_FIRST) || ci->state.aitype >= AI_START)
						team = chooseteam(ci, team);
					if(ci->team != team)
					{
						setteam(ci, team);
						sendf(-1, 1, "ri3", SV_SETTEAM, sender, team);
					}
					break;
				}

				case SV_MAPVOTE:
				{
					getstring(text, p);
					filtertext(text, text);
					if(!strncasecmp(text, "maps/", 5) || !strncasecmp(text, "maps\\", 5))
					{
						defformatstring(map)("%s", &text[5]);
						copystring(text, map);
					}
					int reqmode = getint(p), reqmuts = getint(p);
					if(vote(text, reqmode, reqmuts, sender))
					{
						sendf(-1, 1, "ri2si2", SV_MAPVOTE, sender, text, reqmode, reqmuts);
						relayf(3, "\fc%s suggests: \fs\fw%s on map %s\fS", colorname(ci), gamename(reqmode, reqmuts), text);
					}
					break;
				}

				case SV_GAMEINFO:
				{
					bool valid = !hasgameinfo && !strcmp(ci->clientmap, smapname);
					int n, np = getint(p);
					while((n = getint(p)) != -1)
					{
						int type = getint(p), numattr = getint(p), numkin = getint(p);
						if(valid && (enttype[type].usetype == EU_ITEM || type == PLAYERSTART || type == CHECKPOINT || type == ACTOR || type == TRIGGER))
						{
							while(sents.length() <= n) sents.add();
							sents[n].reset();
							sents[n].type = type;
							sents[n].spawned = false; // wait a bit then load 'em up
							sents[n].millis = gamemillis;
							loopk(numattr) sents[n].attrs.add(getint(p));
							if(numattr < 5) loopk(5-numattr) sents[n].attrs.add(0);
							loopk(numkin) sents[n].kin.add(getint(p));
						}
						else
						{
							loopk(numattr) getint(p);
							loopk(numkin) getint(p);
						}
					}
					if(valid) setupgameinfo(np);
					break;
				}

				case SV_SCORE:
					getint(p);
					getint(p);
					QUEUE_MSG;
					break;

				case SV_FLAGINFO:
					getint(p);
					getint(p);
					getint(p);
					getint(p);
					QUEUE_MSG;
					break;

				case SV_FLAGS:
					if(smode==&stfmode) stfmode.parseflags(p);
					break;

				case SV_TAKEFLAG:
				{
					int lcn = getint(p), flag = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum) || cp->state.state==CS_SPECTATOR) break;
					if(smode==&ctfmode) ctfmode.takeflag(cp, flag);
					break;
				}

				case SV_RESETFLAG:
				{
					int flag = getint(p);
					if(!ci) break;
					if(smode==&ctfmode) ctfmode.resetflag(ci, flag);
					break;
				}

				case SV_DROPFLAG:
				{
					int lcn = getint(p);
					vec droploc;
					loopk(3) droploc[k] = getint(p)/DMF;
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum) || cp->state.state==CS_SPECTATOR) break;
					if(smode==&ctfmode) ctfmode.dropflag(cp, droploc);
					break;
				}

				case SV_INITFLAGS:
				{
					if(smode==&ctfmode) ctfmode.parseflags(p);
					break;
				}

				case SV_PING:
					sendf(sender, 1, "i2", SV_PONG, getint(p));
					break;

				case SV_CLIENTPING:
				{
					int ping = getint(p);
					if(ci)
					{
						ci->ping = ping;
						loopv(clients) if(clients[i]->state.ownernum == ci->clientnum) clients[i]->ping = ping;
					}
					QUEUE_MSG;
					break;
				}

				case SV_MASTERMODE:
				{
					int mm = getint(p);
					if(haspriv(ci, PRIV_MASTER, "change mastermode") && mm >= MM_OPEN && mm <= MM_PRIVATE)
					{
						if(haspriv(ci, PRIV_ADMIN) || (mastermask&(1<<mm)))
						{
							mastermode = mm;
                            allowedips.setsize(0);
                            if(mm >= MM_PRIVATE)
                            {
                                loopv(clients) allowedips.add(getclientip(clients[i]->clientnum));
                            }
							srvoutf(3, "\fymastermode is now \fs\fc%d\fS (\fs\fc%s\fS)", mastermode, mastermodename(mm));
						}
						else srvmsgf(sender, "\frmastermode %d (%s) is disabled on this server", mm, mastermodename(mm));
					}
					break;
				}

				case SV_CLEARBANS:
				{
					if(haspriv(ci, PRIV_MASTER, "clear bans"))
					{
						bannedips.setsize(0);
						srvoutf(3, "cleared all bans");
					}
					break;
				}

				case SV_KICK:
				{
					int victim = getint(p);
					if(haspriv(ci, PRIV_MASTER, "kick people") && victim>=0 && victim<getnumclients() && ci->clientnum!=victim && getinfo(victim))
					{
						ban &b = bannedips.add();
						b.time = totalmillis;
						b.ip = getclientip(victim);
                        allowedips.removeobj(b.ip);
						disconnect_client(victim, DISC_KICK);
					}
					break;
				}

				case SV_SPECTATOR:
				{
					int spectator = getint(p), val = getint(p);
					if(((mastermode >= MM_LOCKED && ci->state.state == CS_SPECTATOR) || spectator != sender) && !haspriv(ci, PRIV_MASTER, spectator != sender ? "spectate others" : "unspectate")) break;
					clientinfo *cp = (clientinfo *)getinfo(spectator);
					if(!cp || cp->state.aitype >= 0) break;
					if(cp->state.state != CS_SPECTATOR && val)
					{
						sendf(-1, 1, "ri3", SV_SPECTATOR, spectator, val);
						if(cp->state.state == CS_ALIVE) dropitems(cp, 1);
						if(smode) smode->leavegame(cp);
						mutate(smuts, mut->leavegame(cp));
						cp->state.cpnodes.setsize(0);
						cp->state.cpmillis = 0;
						cp->state.state = CS_SPECTATOR;
                    	cp->state.timeplayed += lastmillis-cp->state.lasttimeplayed;
						setteam(cp, TEAM_NEUTRAL, false, true);
						aiman::dorefresh = true;
					}
					else if(cp->state.state == CS_SPECTATOR && !val)
					{
						cp->state.cpnodes.setsize(0);
						cp->state.cpmillis = 0;
						cp->state.state = CS_DEAD;
	                    cp->state.lasttimeplayed = lastmillis;
						waiting(cp, 2, 1);
						if(smode) smode->entergame(cp);
						mutate(smuts, mut->entergame(cp));
						aiman::dorefresh = true;
						if(cp->clientmap[0] || cp->mapcrc) checkmaps();
					}
					break;
				}

				case SV_SETTEAM:
				{
					int who = getint(p), team = getint(p);
					if(who<0 || who>=getnumclients() || !haspriv(ci, PRIV_MASTER, "change the team of others")) break;
					clientinfo *cp = (clientinfo *)getinfo(who);
					if(!cp || !m_team(gamemode, mutators) || !m_fight(gamemode) || cp->state.aitype >= AI_START) break;
					if(cp->state.state == CS_SPECTATOR || cp->state.state == CS_EDITING || !isteam(gamemode, mutators, team, TEAM_FIRST)) break;
					setteam(cp, team, true, true);
					break;
				}

				case SV_RECORDDEMO:
				{
					int val = getint(p);
					if(!haspriv(ci, PRIV_ADMIN, "record demos")) break;
					demonextmatch = val!=0;
					srvoutf(4, "\fodemo recording is \fs\fc%s\fS for next match", demonextmatch ? "enabled" : "disabled");
					break;
				}

				case SV_STOPDEMO:
				{
					if(!haspriv(ci, PRIV_ADMIN, "stop demos")) break;
					if(m_demo(gamemode)) enddemoplayback();
					else enddemorecord();
					break;
				}

				case SV_CLEARDEMOS:
				{
					int demo = getint(p);
					if(!haspriv(ci, PRIV_ADMIN, "clear demos")) break;
					cleardemos(demo);
					break;
				}

				case SV_LISTDEMOS:
					if(ci->state.state==CS_SPECTATOR) break;
					listdemos(sender);
					break;

				case SV_GETDEMO:
				{
					int n = getint(p);
					if(ci->state.state==CS_SPECTATOR) break;
					senddemo(sender, n);
					break;
				}

				case SV_EDITENT:
				{
					int n = getint(p), oldtype = NOTUSED;
					bool tweaked = false;
					loopk(3) getint(p);
					if(sents.inrange(n)) oldtype = sents[n].type;
					else while(sents.length() <= n) sents.add();
					if((sents[n].type = getint(p)) != oldtype) tweaked = true;
					int numattrs = getint(p);
					while(sents[n].attrs.length() < max(5, numattrs)) sents[n].attrs.add(0);
					loopk(numattrs) sents[n].attrs[k] = getint(p);
					hasgameinfo = true;
					QUEUE_MSG;
					if(tweaked)
					{
						sents[n].spawned = false;
						sents[n].millis = gamemillis;
						if(enttype[sents[n].type].usetype == EU_ITEM)
						{
							loopvk(clients)
							{
								clientinfo *cq = clients[k];
								cq->state.dropped.remove(n);
								loopj(WEAP_MAX) if(cq->state.entid[j] == n) cq->state.entid[j] = -1;
							}
							sents[n].millis += GVAR(itemspawndelay)*3;
						}
						else if(sents[n].type == TRIGGER) setuptriggers(true);
					}
					break;
				}

				case SV_EDITVAR:
				{
					QUEUE_INT(SV_EDITVAR);
					int t = getint(p);
					QUEUE_INT(t);
					getstring(text, p);
					QUEUE_STR(text);
					switch(t)
					{
						case ID_VAR:
						{
							int val = getint(p);
							relayf(3, "\fg%s set worldvar %s to %d", colorname(ci), text, val);
							QUEUE_INT(val);
							break;
						}
						case ID_FVAR:
						{
							float val = getfloat(p);
							relayf(3, "\fg%s set worldvar %s to %s", colorname(ci), text, floatstr(val));
							QUEUE_FLT(val);
							break;
						}
						case ID_SVAR:
						case ID_ALIAS:
						{
							string val;
							getstring(val, p);
							relayf(3, "\fg%s set world%s %s to %s", colorname(ci), t == ID_ALIAS ? "alias" : "var", text, val);
							QUEUE_STR(val);
							break;
						}
						default: break;
					}
					break;
				}

				case SV_GETMAP:
				{
					ci->wantsmap = true;
					if(!mapsending && mapdata[0])
					{
						loopk(3) if(mapdata[k])
							sendfile(sender, 2, mapdata[k], "ri", SV_SENDMAPFILE+k);
						sendwelcome(ci);
					}
					else
					{
						if(!mapsending)
						{
							clientinfo *best = choosebestclient();
							if(best)
							{
								loopk(3) if(mapdata[k]) DELETEP(mapdata[k]);
								mapsending = false;
								sendf(best->clientnum, 1, "ri", SV_GETMAP);
							}
						}
						srvmsgf(ci->clientnum, "map is being uploaded, please wait..");
					}
					break;
				}

				case SV_NEWMAP:
				{
					int size = getint(p);
					if(ci->state.state==CS_SPECTATOR) break;
					if(size>=0)
					{
						smapname[0] = '\0';
						sents.setsize(0);
						hasgameinfo = true;
						if(smode) smode->reset(true);
						mutate(smuts, mut->reset(true));
					}
					QUEUE_MSG;
					break;
				}

				case SV_SETMASTER:
				{
					int val = getint(p);
					getstring(text, p);
					auth::setmaster(ci, val!=0, text);
					// don't broadcast the master password
					break;
				}

				case SV_ADDBOT:
				{
					aiman::reqadd(ci, getint(p));
					break;
				}

				case SV_DELBOT:
				{
					aiman::reqdel(ci);
					break;
				}

				case SV_AUTHTRY:
				{
					getstring(text, p);
					auth::tryauth(ci, text);
					break;
				}

				case SV_AUTHANS:
				{
					uint id = (uint)getint(p);
					getstring(text, p);
					auth::answerchallenge(ci, id, text);
					break;
				}

				default:
				{
					int size = msgsizelookup(type);
					if(size==-1)
					{
						conoutf("\fy[tag error] from: %d, cur: %d, msg: %d, prev: %d", sender, curtype, type, prevtype);
						disconnect_client(sender, DISC_TAGT);
						return;
					}
					if(size>0) loopi(size-1) getint(p);
					if(ci) QUEUE_MSG;
					break;
				}
			}
			if(verbose > 5) conoutf("\fy[server] from: %d, cur: %d, msg: %d, prev: %d", sender, curtype, type, prevtype);
		}
	}

	bool serveroption(char *arg)
	{
		if(arg[0]=='-' && arg[1]=='s') switch(arg[2])
		{
			case 'd': setsvar("serverdesc", &arg[3]); return true;
			case 'P': setsvar("adminpass", &arg[3]); return true;
            case 'k': setsvar("serverpass", &arg[3]); return true;
			case 'o': setvar("serveropen", atoi(&arg[2])); return true;
			case 'M': setsvar("servermotd", &arg[3]); return true;
			default: break;
		}
		return false;
	}
};
#undef GAMESERVER
