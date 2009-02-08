#define GAMESERVER 1
#include "pch.h"
#include "engine.h"
#include "game.h"
#include "hash.h"

namespace server
{
	struct srventity
	{
		int type;
		short attr[ENTATTRS];
		bool spawned;
		int millis;

		srventity() : type(NOTUSED), spawned(false), millis(0) { loopi(ENTATTRS) attr[i] = 0; }
		~srventity() {}
	};

    static const int DEATHMILLIS = 300;

	enum { GAMEEVENT_NONE = 0, GAMEEVENT_SHOT, GAMEEVENT_SWITCH, GAMEEVENT_RELOAD, GAMEEVENT_DESTROY, GAMEEVENT_USE, GAMEEVENT_SUICIDE };

	struct shotevent
	{
        int millis, id;
		int weap, power, num;
		ivec from;
		vector<ivec> shots;
	};

	struct switchevent
	{
		int millis, id;
		int weap;
	};

	struct reloadevent
	{
		int millis, id;
		int weap;
	};

	struct hitset
	{
		int flags;
		int target;
		int lifesequence;
		union
		{
			int rays;
			int dist;
		};
		ivec dir;
	};

	struct destroyevent
	{
        int millis, id;
		int weap, radial;
		vector<hitset> hits;
	};

	struct suicideevent
	{
		int type, flags;
	};

	struct useevent
	{
		int millis, id;
		int ent;
	};

	struct gameevent
	{
		int type;
		shotevent shot;
		destroyevent destroy;
		switchevent weapsel;
		reloadevent reload;
		suicideevent suicide;
		useevent use;
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
        projectilestate dropped, weapshots[WEAPON_MAX];
		int frags, flags, deaths, teamkills, shotdamage, damage;
		int lasttimeplayed, timeplayed, aireinit;
		float effectiveness;

		servstate() : state(CS_DEAD), aireinit(0) {}

		bool isalive(int millis)
		{
			return state==CS_ALIVE || (state==CS_DEAD && millis-lastdeath <= DEATHMILLIS);
		}

		void reset()
		{
			if(state!=CS_SPECTATOR) state = CS_DEAD;
			dropped.reset();
            loopi(WEAPON_MAX) weapshots[i].reset();
			timeplayed = 0;
			lifesequence = effectiveness = 0;
            frags = flags = deaths = teamkills = shotdamage = damage = 0;
			respawn(0, m_maxhealth(server::gamemode, server::mutators));
		}

		void respawn(int millis, int heal)
		{
			gamestate::respawn(millis, heal);
			o = vec(-1e10f, -1e10f, -1e10f);
		}

		bool isai(int type = -1, bool all = true)
		{
			return (type < 0 ? aitype != AI_NONE : aitype == type) && (all || aireinit >= 0) && (all || ownernum >= 0);
		}
	};

	struct savedscore
	{
		uint ip;
		string name;
		int frags, flags, timeplayed, deaths, teamkills, shotdamage, damage;
		float effectiveness;

		void save(servstate &gs)
		{
			frags = gs.frags;
			flags = gs.flags;
            deaths = gs.deaths;
            teamkills = gs.teamkills;
            shotdamage = gs.shotdamage;
            damage = gs.damage;
			timeplayed = gs.timeplayed;
			effectiveness = gs.effectiveness;
		}

		void restore(servstate &gs)
		{
			gs.frags = frags;
			gs.flags = flags;
            gs.deaths = deaths;
            gs.teamkills = teamkills;
            gs.shotdamage = shotdamage;
            gs.damage = damage;
			gs.timeplayed = timeplayed;
			gs.effectiveness = effectiveness;
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
		int modevote, mutsvote;
		int privilege;
        bool connected, local, spectator, timesync, online, wantsmap;
        int gameoffset, lastevent;
		servstate state;
		vector<gameevent> events;
		vector<uchar> position, messages;
        vector<clientinfo *> targets;
        uint authreq;
        string authname;

		clientinfo() { reset(); }

		gameevent &addevent()
		{
			static gameevent dummy;
            if(state.state==CS_SPECTATOR || events.length()>100) return dummy;
			return events.add();
		}

		void mapchange()
		{
			mapvote[0] = 0;
			state.reset();
			events.setsizenodelete(0);
            targets.setsizenodelete(0);
            timesync = false;
            lastevent = gameoffset = 0;
		}

		void reset()
		{
			ping = 0;
			team = TEAM_NEUTRAL;
			name[0] = 0;
			privilege = PRIV_NONE;
            connected = local = spectator = online = wantsmap = false;
            authreq = 0;
			position.setsizenodelete(0);
			messages.setsizenodelete(0);
			mapchange();
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
		extern bool addai(int type, int skill, bool req = false);
		extern void deleteai(clientinfo *ci);
		extern bool delai(int type, bool req = false);
		extern void removeai(clientinfo *ci, bool complete = false);
		extern bool reassignai(int exclude = -1);
		extern void checkskills();
		extern void clearai();
		extern void checkai();
		extern void reqadd(clientinfo *ci, int skill);
		extern void reqdel(clientinfo *ci);
	}

	bool notgotinfo = true, notgotflags = false;
	int gamemode = G_LOBBY, mutators = 0;
	int gamemillis = 0, gamelimit = 0;

	string smapname;
	int interm = 0, minremain = 15, oldtimelimit = 15;
	bool maprequest = false;
	enet_uint32 lastsend = 0;
	int mastermode = MM_OPEN, mastermask = MM_DEFAULT, currentmaster = -1;
	bool masterupdate = false, mapsending = false;
	FILE *mapdata[3] = { NULL, NULL, NULL };

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
	FILE *demotmp = NULL;
	gzFile demorecord = NULL, demoplayback = NULL;
	int nextplayback = 0;

	struct servmode
	{
		servmode() {}
		virtual ~servmode() {}

		virtual void entergame(clientinfo *ci) {}
        virtual void leavegame(clientinfo *ci, bool disconnecting = false) {}

		virtual void moved(clientinfo *ci, const vec &oldpos, const vec &newpos) {}
		virtual bool canspawn(clientinfo *ci, bool connecting = false, bool tryspawn = false) { return true; }
		virtual void spawned(clientinfo *ci) {}
        virtual int fragvalue(clientinfo *victim, clientinfo *actor)
        {
            if(victim==actor || victim->team == actor->team) return -1;
            return 1;
        }
		virtual void died(clientinfo *victim, clientinfo *actor) {}
		virtual void changeteam(clientinfo *ci, int oldteam, int newteam) {}
		virtual void initclient(clientinfo *ci, ucharbuf &p, bool connecting) {}
		virtual void update() {}
		virtual void reset(bool empty) {}
		virtual void intermission() {}
		virtual bool damage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush = ivec(0, 0, 0)) { return true; }
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

	ICOMMAND(gameid, "", (), result(gameid()));
	ICOMMAND(gamever, "", (), intret(gamever()));

	void cleanup()
	{
		bannedips.setsize(0);
		aiman::clearai();
		enumerate(*idents, ident, id, {
			if(id.flags&IDF_SERVER) // reset vars
			{
				switch(id.type)
				{
					case ID_VAR:
					{
						setvar(id.name, id.def.i, true);
						break;
					}
					case ID_FVAR:
					{
						setfvar(id.name, id.def.f, true);
						break;
					}
					case ID_SVAR:
					{
						setsvar(id.name, *id.def.s ? id.def.s : "", true);
						break;
					}
					default: break;
				}
			}
		});
		execfile("servexec.cfg");
		changemap(GVAR(defaultmap), GVAR(defaultmode), GVAR(defaultmuts));
	}

	void start() { cleanup(); }

	void *newinfo() { return new clientinfo; }
	void deleteinfo(void *ci) { delete (clientinfo *)ci; }

	const char *privname(int type)
	{
		switch(type)
		{
			case PRIV_ADMIN: return "admin";
			case PRIV_MASTER: return "master";
			default: return "alone";
		}
	}

	int numclients(int exclude, bool nospec, bool noai)
	{
		int n = 0;
		loopv(clients)
			if(clients[i]->clientnum >= 0 && clients[i]->name[0] && clients[i]->connected && clients[i]->clientnum != exclude &&
				(!nospec || clients[i]->state.state != CS_SPECTATOR) && (!noai || !clients[i]->state.isai(-1, false)))
					n++;
		return n;
	}

	bool haspriv(clientinfo *ci, int flag, bool msg = false)
	{
		if(flag <= PRIV_MASTER && !numclients(ci->clientnum, false, true)) return true;
		else if(ci->local || ci->privilege >= flag) return true;
		else if(msg) srvmsgf(ci->clientnum, "\fraccess denied, you need %s", privname(flag));
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
		const char *chat = team && m_team(gamemode, mutators) ? teamtype[ci->team].chat : teamtype[TEAM_NEUTRAL].chat;
		s_sprintf(cname)("\fs%s%s", chat, name);
		if(!name[0] || ci->state.isai() || (dupname && duplicatename(ci, name)))
		{
			s_sprintfd(s)(" [\fs%s%d\fS]", ci->state.isai() ? "\fc" : "\fm", ci->clientnum);
			s_strcat(cname, s);
		}
		s_strcat(cname, "\fS");
		return cname;
	}

    const char *gameid() { return GAMEID; }
    int gamever() { return GAMEVERSION; }
    char *gamename(int mode, int muts)
    {
    	static string gname;
    	gname[0] = 0;
    	if(gametype[mode].mutators && muts) loopi(G_M_NUM)
		{
			if ((gametype[mode].mutators & mutstype[i].type) && (muts & mutstype[i].type) &&
				(!gametype[mode].implied || !(gametype[mode].implied & mutstype[i].type)))
			{
				s_sprintfd(name)("%s%s%s", *gname ? gname : "", *gname ? "-" : "", mutstype[i].name);
				s_strcpy(gname, name);
			}
		}
		s_sprintfd(mname)("%s%s%s", *gname ? gname : "", *gname ? " " : "", gametype[mode].name);
		s_strcpy(gname, mname);
		return gname;
    }
	ICOMMAND(gamename, "iii", (int *g, int *m), result(gamename(*g, *m)));

    void modecheck(int *mode, int *muts)
    {
		if(!m_game(*mode))
		{
			*mode = G_DEATHMATCH;
			*muts = gametype[*mode].implied;
		}

		#define modecheckreset { i = 0; continue; }
		if(gametype[*mode].mutators && *muts) loopi(G_M_NUM)
		{
			if(!(gametype[*mode].mutators & mutstype[i].type) && (*muts & mutstype[i].type))
			{
				*muts &= ~mutstype[i].type;
				modecheckreset;
			}
			if(gametype[*mode].implied && (gametype[*mode].implied & mutstype[i].type) && !(*muts & mutstype[i].type))
			{
				*muts |= mutstype[i].type;
				modecheckreset;
			}
			if(*muts & mutstype[i].type) loopj(G_M_NUM)
			{
				if(mutstype[i].mutators && !(mutstype[i].mutators & mutstype[j].type) && (*muts & mutstype[j].type))
				{
					*muts &= ~mutstype[j].type;
					modecheckreset;
				}
				if(mutstype[i].implied && (mutstype[i].implied & mutstype[j].type) && !(*muts & mutstype[j].type))
				{
					*muts |= mutstype[j].type;
					modecheckreset;
				}
			}
		}
		else *muts = G_M_NONE;
		if(kidmode > 1 && !(*muts&G_M_PAINT)) *muts |= G_M_PAINT;
    }

	int mutscheck(int mode, int muts)
	{
		int gm = mode, mt = muts;
		modecheck(&gm, &mt);
		return mt;
	}
	ICOMMAND(mutscheck, "iii", (int *g, int *m), intret(mutscheck(*g, *m)));

	const char *choosemap(const char *suggest)
	{
		static string mapchosen;
		if(suggest && *suggest) s_strcpy(mapchosen, suggest);
		else *mapchosen = 0;
		if(GVAR(maprotate))
		{
			const char *maplist = GVAR(mainmaps);
			if(m_lobby(gamemode)) maplist = GVAR(lobbymaps);
			else if(m_mission(gamemode)) maplist = GVAR(missionmaps);
			else if(m_stf(gamemode)) maplist = GVAR(stfmaps);
			else if(m_ctf(gamemode)) maplist = m_multi(gamemode, mutators) ? GVAR(mctfmaps) : GVAR(ctfmaps);
			if(maplist && *maplist)
			{
				int n = listlen(maplist), c = -1;
				if(GVAR(maprotate) > 1) c = n ? rnd(n) : 0;
				else loopi(n)
				{
					char *maptxt = indexlist(maplist, i);
					if(maptxt)
					{
						string maploc;
						if(strpbrk(maptxt, "/\\")) s_strcpy(maploc, maptxt);
						else s_sprintf(maploc)("maps/%s", maptxt);
						if(*mapchosen && (!strcmp(mapchosen, maptxt) || !strcmp(mapchosen, maploc)))
							c = i >= 0 && i < n-1 ? i+1 : 0;
						DELETEP(maptxt);
					}
					if(c >= 0) break;
				}
				char *mapidx = c >= 0 ? indexlist(maplist, c) : NULL;
				if(mapidx)
				{
					s_strcpy(mapchosen, mapidx);
					DELETEP(mapidx);
				}
			}
		}
		return *mapchosen ? mapchosen : GVAR(defaultmap);
	}

	bool canload(char *type)
	{
		if(!strcmp(type, gameid()) || !strcmp(type, "fps") || !strcmp(type, "bfg"))
			return true;
		return false;
	}

	void checkintermission()
	{
		if(minremain)
		{
			if(GVAR(timelimit) != oldtimelimit)
			{
				if(GVAR(timelimit)) gamelimit += (GVAR(timelimit)-oldtimelimit)*60000;
				oldtimelimit = GVAR(timelimit);
			}
			if(GVAR(timelimit))
			{
				if(gamemillis >= gamelimit) minremain = 0;
				else minremain = (gamelimit-gamemillis+60000-1)/60000;
			}
			else minremain = -1;
			sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
			if(!minremain)
			{
				if(smode) smode->intermission();
				mutate(smuts, mut->intermission());
			}
			else if(minremain == 1)
				sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_ONEMINUTE, "only one minute left of play!");
		}
		if(!minremain && !interm)
		{
			maprequest = false;
			interm = gamemillis+(GVAR(intermlimit)*1000);
		}
	}

	void startintermission()
	{
		minremain = 0;
		gamelimit = min(gamelimit, gamemillis);
		sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
		checkintermission();
	}

	bool finditem(int i, bool spawned = true, int spawntime = 0)
	{
		if(sents[i].spawned) return true;
		else
		{
			loopvk(clients)
			{
				clientinfo *ci = clients[k];
				if(ci->state.dropped.projs.find(i) >= 0 && (!spawned || (spawntime && gamemillis-sents[i].millis <= spawntime)))
					return true;
				else loopj(WEAPON_MAX) if(ci->state.entid[j] == i) return spawned;
			}
			if(spawned && spawntime && gamemillis-sents[i].millis <= spawntime)
				return true;
		}
		return false;
	}

	struct spawn
	{
		int spawncycle;
		vector<int> ents;

		spawn() { reset(); }
		~spawn() {}

		void reset()
		{
			spawncycle = 0;
			ents.setsize(0);
		}
		void add(int n)
		{
			ents.add(n);
			spawncycle = rnd(ents.length());
		}
	} spawns[TEAM_LAST+1];
	int numplayers, totalspawns;

	void setupspawns(bool update, int players = 0)
	{
		aiman::autooverride = false;
		numplayers = totalspawns = 0;
		loopi(TEAM_LAST+1) spawns[i].reset();
		if(update)
		{
			loopv(sents) if(sents[i].type == PLAYERSTART && isteam(gamemode, mutators, sents[i].attr[1], TEAM_FIRST))
			{
				spawns[sents[i].attr[1]].add(i);
				totalspawns++;
			}
			if(totalspawns && m_team(gamemode, mutators))
			{
				loopi(numteams(gamemode, mutators)) if(spawns[i+TEAM_FIRST].ents.empty())
				{
					loopj(TEAM_LAST+1) spawns[j].reset();
					totalspawns = 0;
					break;
				}
			}
			if(!totalspawns)
			{
				loopv(sents) if(sents[i].type == PLAYERSTART)
				{
					spawns[TEAM_NEUTRAL].add(i);
					totalspawns++;
				}
			}
			if(!totalspawns) // we can cheat and use weapons for spawns
			{
				loopv(sents) if(sents[i].type == WEAPON)
				{
					spawns[TEAM_NEUTRAL].add(i);
					totalspawns++;
				}
			}
			numplayers = players;
		}
	}

	int pickspawn(clientinfo *ci)
	{
		if(totalspawns && GVAR(spawnrotate))
		{
			int team = m_team(gamemode, mutators) && !spawns[ci->team].ents.empty() ? ci->team : TEAM_NEUTRAL;
			if(!spawns[team].ents.empty())
			{
				switch(GVAR(spawnrotate))
				{
					case 1: default: spawns[team].spawncycle++; break;
					case 2:
					{
						int r = rnd(spawns[team].ents.length());
						spawns[team].spawncycle = r != spawns[team].spawncycle ? r : r + 1;
						break;
					}
				}
				if(spawns[team].spawncycle >= spawns[team].ents.length()) spawns[team].spawncycle = 0;
				return spawns[team].ents[spawns[team].spawncycle];
			}
		}
		return -1;
	}

	void spawnstate(clientinfo *ci)
	{
		servstate &gs = ci->state;
		gs.spawnstate(m_spawnweapon(gamemode, mutators), m_maxhealth(gamemode, mutators), !m_noitems(gamemode, mutators));
		gs.lifesequence++;
	}

	void sendspawn(clientinfo *ci)
	{
		servstate &gs = ci->state;
		spawnstate(ci);
		int spawn = pickspawn(ci);
		sendf(ci->clientnum, 1, "ri8v",
			SV_SPAWNSTATE, ci->clientnum, spawn, gs.state, gs.frags, gs.lifesequence, gs.health, gs.weapselect, WEAPON_MAX, &gs.ammo[0]);
		gs.lastrespawn = gs.lastspawn = gamemillis;
	}

    void sendstate(clientinfo *ci, ucharbuf &p, int spawn = -2)
    {
		servstate &gs = ci->state;
        putint(p, ci->clientnum);
        if(spawn >= -1) putint(p, spawn);
        putint(p, gs.state);
        putint(p, gs.frags);
        putint(p, gs.lifesequence);
        putint(p, gs.health);
        putint(p, gs.weapselect);
        loopi(WEAPON_MAX) putint(p, gs.ammo[i]);
    }

	void relayf(int r, const char *s, ...)
	{
		s_sprintfdlv(str, s, s);
		string st;
		filtertext(st, str);
		ircoutf(r, "%s", st);
	}

	void srvmsgf(int cn, const char *s, ...)
	{
		s_sprintfdlv(str, s, s);
		sendf(cn, 1, "ris", SV_SERVMSG, str);
	}

	void srvoutf(const char *s, ...)
	{
		s_sprintfdlv(str, s, s);
		srvmsgf(-1, "%s", str);
		relayf(2, "%s", str);
	}

	void writedemo(int chan, void *data, int len)
	{
		if(!demorecord) return;
		int stamp[3] = { gamemillis, chan, len };
		endianswap(stamp, sizeof(int), 3);
		gzwrite(demorecord, stamp, sizeof(stamp));
		gzwrite(demorecord, data, len);
	}

	void recordpacket(int chan, void *data, int len)
	{
		writedemo(chan, data, len);
	}

	void listdemos(int cn)
	{
		ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
		if(!packet) return;
		ucharbuf p(packet->data, packet->dataLength);
		putint(p, SV_SENDDEMOLIST);
		putint(p, demos.length());
		loopv(demos) sendstring(demos[i].info, p);
		enet_packet_resize(packet, p.length());
		sendpacket(cn, 1, packet);
		if(!packet->referenceCount) enet_packet_destroy(packet);
	}

	void cleardemos(int n)
	{
		if(!n)
		{
			loopv(demos) delete[] demos[i].data;
			demos.setsize(0);
			srvoutf("cleared all demos");
		}
		else if(demos.inrange(n-1))
		{
			delete[] demos[n-1].data;
			demos.remove(n-1);
			srvoutf("cleared demo %d", n);
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
    int welcomepacket(ucharbuf &p, clientinfo *ci, ENetPacket *packet);

	void enddemoplayback()
	{
		if(!demoplayback) return;
		gzclose(demoplayback);
		demoplayback = NULL;

		loopv(clients) sendf(clients[i]->clientnum, 1, "ri3", SV_DEMOPLAYBACK, 0, clients[i]->clientnum);

		srvoutf("demo playback finished");

		loopv(clients) sendwelcome(clients[i]);
	}

	void setupdemoplayback()
	{
		demoheader hdr;
		string msg;
		msg[0] = '\0';
		s_sprintfd(file)("%s.dmo", smapname);
		demoplayback = opengzfile(file, "rb9");
		if(!demoplayback) s_sprintf(msg)("could not read demo \"%s\"", file);
		else if(gzread(demoplayback, &hdr, sizeof(demoheader))!=sizeof(demoheader) || memcmp(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic)))
			s_sprintf(msg)("\"%s\" is not a demo file", file);
		else
		{
			endianswap(&hdr.version, sizeof(int), 1);
			endianswap(&hdr.gamever, sizeof(int), 1);
			if(hdr.version!=DEMO_VERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.version<DEMO_VERSION ? "older" : "newer");
			else if(hdr.gamever!=GAMEVERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.gamever<GAMEVERSION ? "older" : "newer");
		}
		if(msg[0])
		{
			if(demoplayback) { gzclose(demoplayback); demoplayback = NULL; }
			srvoutf("%s", msg);
			return;
		}

		srvoutf("playing demo \"%s\"", file);

		sendf(-1, 1, "rii", SV_DEMOPLAYBACK, 1);

		if(gzread(demoplayback, &nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
		{
			enddemoplayback();
			return;
		}
		endianswap(&nextplayback, sizeof(nextplayback), 1);
	}

	void readdemo()
	{
		if(!demoplayback) return;
		while(gamemillis>=nextplayback)
		{
			int chan, len;
			if(gzread(demoplayback, &chan, sizeof(chan))!=sizeof(chan) ||
				gzread(demoplayback, &len, sizeof(len))!=sizeof(len))
			{
				enddemoplayback();
				return;
			}
			endianswap(&chan, sizeof(chan), 1);
			endianswap(&len, sizeof(len), 1);
			ENetPacket *packet = enet_packet_create(NULL, len, 0);
			if(!packet || gzread(demoplayback, packet->data, len)!=len)
			{
				if(packet) enet_packet_destroy(packet);
				enddemoplayback();
				return;
			}
			sendpacket(-1, chan, packet);
			if(!packet->referenceCount) enet_packet_destroy(packet);
			if(gzread(demoplayback, &nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
			{
				enddemoplayback();
				return;
			}
			endianswap(&nextplayback, sizeof(nextplayback), 1);
		}
	}

	void enddemorecord()
	{
		if(!demorecord) return;

		gzclose(demorecord);
		demorecord = NULL;

#ifdef WIN32
        demotmp = fopen("demorecord", "rb");
#endif
		if(!demotmp) return;

		fseek(demotmp, 0, SEEK_END);
		int len = ftell(demotmp);
		rewind(demotmp);
		if(demos.length()>=MAXDEMOS)
		{
			delete[] demos[0].data;
			demos.remove(0);
		}
		demofile &d = demos.add();
		time_t t = time(NULL);
		char *timestr = ctime(&t), *trim = timestr + strlen(timestr);
		while(trim>timestr && isspace(*--trim)) *trim = '\0';
		s_sprintf(d.info)("%s: %s, %s, %.2f%s", timestr, gamename(gamemode, mutators), smapname, len > 1024*1024 ? len/(1024*1024.f) : len/1024.0f, len > 1024*1024 ? "MB" : "kB");
		srvoutf("demo \"%s\" recorded", d.info);
		d.data = new uchar[len];
		d.len = len;
		fread(d.data, 1, len, demotmp);
		fclose(demotmp);
		demotmp = NULL;
	}

    int buildinitc2s(clientinfo *ci, uchar *buf, int bufsize)
    {
        ucharbuf q(&buf[16], bufsize-16);
        putint(q, SV_INITC2S);
        sendstring(ci->name, q);
        putint(q, ci->team);

        ucharbuf h(buf, 16);
        putint(h, SV_CLIENT);
        putint(h, ci->clientnum);
        putuint(h, q.len);

        memmove(&buf[h.len], q.buf, q.len);

        return h.len + q.len;
    }

	void setupdemorecord()
	{
		if(m_demo(gamemode) || m_edit(gamemode)) return;

#ifdef WIN32
        gzFile f = gzopen("demorecord", "wb9");
        if(!f) return;
#else
		demotmp = tmpfile();
		if(!demotmp) return;
		setvbuf(demotmp, NULL, _IONBF, 0);

        gzFile f = gzdopen(_dup(_fileno(demotmp)), "wb9");
        if(!f)
		{
			fclose(demotmp);
			demotmp = NULL;
			return;
		}
#endif

        srvoutf("recording demo");

        demorecord = f;

		demoheader hdr;
		memcpy(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic));
		hdr.version = DEMO_VERSION;
		hdr.gamever = GAMEVERSION;
		endianswap(&hdr.version, sizeof(int), 1);
		endianswap(&hdr.gamever, sizeof(int), 1);
		gzwrite(demorecord, &hdr, sizeof(demoheader));

        ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, 0);
        ucharbuf p(packet->data, packet->dataLength);
        welcomepacket(p, NULL, packet);
        writedemo(1, p.buf, p.len);
        enet_packet_destroy(packet);

        uchar buf[MAXTRANS];
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			writedemo(1, buf, buildinitc2s(ci, buf, sizeof(buf)));
		}
	}

	void checkvotes(bool force = false)
	{
		vector<votecount> votes;
		int maxvotes = 0;
		loopv(clients)
		{
			clientinfo *oi = clients[i];
			if((oi->state.state==CS_SPECTATOR && !haspriv(oi, PRIV_MASTER, false)) || oi->state.isai()) continue;
			maxvotes++;
			if(!oi->mapvote[0]) continue;
			votecount *vc = NULL;
			loopvj(votes) if(!strcmp(oi->mapvote, votes[j].map) && oi->modevote==votes[j].mode && oi->mutsvote==votes[j].muts)
			{
				vc = &votes[j];
				break;
			}
			if(!vc) vc = &votes.add(votecount(oi->mapvote, oi->modevote, oi->mutsvote));
			vc->count++;
		}

		votecount *best = NULL;
		loopv(votes) if(!best || votes[i].count > best->count) best = &votes[i];
		bool gotvotes = best && best->count >= min(max(maxvotes/2, force ? 1 : 2), force ? 1 : maxvotes);
		if(force || gotvotes)
		{
			if(demorecord) enddemorecord();
			if(gotvotes)
			{
				srvoutf("\fcvote passed by majority: \fs\fw%s on map %s\fS", gamename(best->mode, best->muts), best->map);
				sendf(-1, 1, "ri2si3", SV_MAPCHANGE, 1, best->map, 0, best->mode, best->muts);
				changemap(best->map, best->mode, best->muts);
			}
			else
			{
				const char *map = choosemap(smapname);
				srvoutf("\fcvote defaulted, server chooses: \fs\fw%s on map %s\fS", gamename(gamemode, mutators), map);
				sendf(-1, 1, "ri2si3", SV_MAPCHANGE, 1, map, 0, gamemode, mutators);
				changemap(map, gamemode, mutators);
			}
		}
	}

	void vote(char *map, int reqmode, int reqmuts, int sender)
	{
		clientinfo *ci = (clientinfo *)getinfo(sender);
        if(!ci || (ci->state.state==CS_SPECTATOR && !haspriv(ci, PRIV_MASTER, true)) || !m_game(reqmode)) return;
		if(reqmode < G_LOBBY && !ci->local)
		{
			srvmsgf(ci->clientnum, "\fraccess denied, you must be a local client");
			return;
		}

		s_strcpy(ci->mapvote, map);
		if(!ci->mapvote[0]) return;

		ci->modevote = reqmode; ci->mutsvote = reqmuts;
		modecheck(&ci->modevote, &ci->mutsvote);

		if(haspriv(ci, PRIV_MASTER) && (mastermode >= MM_VETO || !numclients(ci->clientnum, false, true)))
		{
			if(demorecord) enddemorecord();
			srvoutf("\fc%s [%s] forced: \fs\fw%s on map %s\fS", colorname(ci), privname(ci->privilege), gamename(ci->modevote, ci->mutsvote), map);
			sendf(-1, 1, "ri2si3", SV_MAPCHANGE, 1, ci->mapvote, 0, ci->modevote, ci->mutsvote);
			changemap(ci->mapvote, ci->modevote, ci->mutsvote);
		}
		else
		{
			srvoutf("\fc%s suggests: \fs\fw%s on map %s\fS", colorname(ci), gamename(ci->modevote, ci->mutsvote), map);
			checkvotes();
		}
	}

	struct teamscore
	{
		int team;
		union
		{
			int score;
			float rank;
		};
		int clients;

		teamscore(int n) : team(n), rank(0.f), clients(0) {}
		teamscore(int n, float r) : team(n), rank(r), clients(0) {}
		teamscore(int n, int s) : team(n), score(s), clients(0) {}

		~teamscore() {}
	};

	int chooseteam(clientinfo *who, int suggest = -1)
	{
		if(m_team(gamemode, mutators))
		{
			int team = isteam(gamemode, mutators, suggest, TEAM_FIRST) ? suggest : -1;
			if(GVAR(teambalance) || team < 0)
			{
				teamscore teamscores[TEAM_NUM] = {
					teamscore(TEAM_ALPHA), teamscore(TEAM_BETA), teamscore(TEAM_DELTA), teamscore(TEAM_GAMMA)
				};
				loopv(clients)
				{
					clientinfo *ci = clients[i];
					if(!ci->team || ci == who || ci->state.state == CS_SPECTATOR) continue;
					ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
					ci->state.lasttimeplayed = lastmillis;
					loopj(numteams(gamemode, mutators)) if(ci->team == teamscores[j].team)
					{
						teamscore &ts = teamscores[j];
						float rank = ci->state.effectiveness/max(ci->state.timeplayed, 1);
						ts.rank += rank;
						ts.clients++;
						break;
					}
				}
				teamscore *worst = &teamscores[0];
				loopi(numteams(gamemode, mutators))
				{
					teamscore &ts = teamscores[i];
					switch(GVAR(teambalance))
					{
						case 1: default:
						{
							if(ts.clients < worst->clients || (ts.clients == worst->clients && ts.rank < worst->rank))
								worst = &ts;
							break;
						}
						case 2:
						{
							if(ts.rank < worst->rank || (ts.rank == worst->rank && ts.clients < worst->clients))
								worst = &ts;
							break;
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
				    oi->state.timeplayed += lastmillis - oi->state.lasttimeplayed;
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
		s_strcpy(sc.name, ci->name);
		return sc;
	}

	void savescore(clientinfo *ci)
	{
		savedscore &sc = findscore(ci, true);
		if(&sc) sc.save(ci->state);
	}

	struct droplist { int weap, ent; };
	void dropitems(clientinfo *ci, bool discon = false)
	{
		servstate &ts = ci->state;
		vector<droplist> drop;
		int sweap = m_spawnweapon(gamemode, mutators);
		if(!discon && GVAR(kamikaze) && (GVAR(kamikaze) > 2 || (ts.hasweap(WEAPON_GL, sweap) && (GVAR(kamikaze) > 1 || ts.weapselect == WEAPON_GL))))
		{
			ts.weapshots[WEAPON_GL].add(-1);
			droplist &d = drop.add();
			d.weap = WEAPON_GL;
			d.ent = -1;
		}
		if(!m_noitems(gamemode, mutators) && (discon || GVAR(itemdropping)))
		{
			loopi(WEAPON_MAX) if(ts.hasweap(i, sweap, 1) && sents.inrange(ts.entid[i]))
			{
				if(!(sents[ts.entid[i]].attr[1]&WEAPFLAG_FORCED))
				{
					if(!discon)
					{
						ts.dropped.add(ts.entid[i]);
						droplist &d = drop.add();
						d.weap = i;
						d.ent = ts.entid[i];
					}
					sents[ts.entid[i]].spawned = false;
					sents[ts.entid[i]].millis = discon ? gamemillis-(GVAR(itemspawntime)*1000) : gamemillis;
				}
			}
		}
		if(!discon)
		{
			ts.weapreset(false);
			if(!drop.empty())
				sendf(-1, 1, "ri2iv", SV_DROP, ci->clientnum, drop.length(), drop.length()*sizeof(droplist)/sizeof(int), drop.getbuf());
		}
	}

	#include "stfmode.h"
    #include "ctfmode.h"
	#include "duelmut.h"
	#include "aiman.h"

	void changemap(const char *s, int mode, int muts)
	{
		loopi(3) if(mapdata[i])
		{
			fclose(mapdata[i]);
			mapdata[i] = NULL;
		}
		maprequest = mapsending = false;
        stopdemo();
		gamemode = mode >= 0 ? mode : GVAR(defaultmode);
		mutators = muts >= 0 ? muts : GVAR(defaultmuts);
		modecheck(&gamemode, &mutators);
		if(!m_play(gamemode)) aiman::clearai();
		else aiman::autooverride = false;
		numplayers = gamemillis = interm = 0;
		oldtimelimit = GVAR(timelimit);
		minremain = GVAR(timelimit) ? GVAR(timelimit) : -1;
		gamelimit = GVAR(timelimit) ? minremain*60000 : 0;
		s_strcpy(smapname, s && *s ? s : GVAR(defaultmap));
		sents.setsize(0);
		setupspawns(false);
		notgotinfo = true;
		scores.setsize(0);

		if(m_team(gamemode, mutators))
		{
			loopv(clients)
			{
				clientinfo *ci = clients[i];
				ci->team = TEAM_NEUTRAL; // to be reset below
				ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
			}
		}

		if(m_demo(gamemode)) kicknonlocalclients(DISC_PRIVATE);

		// server modes
		if(m_stf(gamemode)) smode = &stfmode;
        else if(m_ctf(gamemode)) smode = &ctfmode;
		else smode = NULL;

		smuts.setsize(0);
		if(m_duke(gamemode, mutators)) smuts.add(&duelmutator);

		if(smode) smode->reset(false);
		mutate(smuts, mut->reset(false));

		loopv(clients)
		{
			clientinfo *ci = clients[i];
			int team = chooseteam(ci, ci->team);
			sendf(-1, 1, "ri3", SV_SETTEAM, ci->clientnum, team);
			if(smode) smode->changeteam(ci, ci->team, team);
			mutate(smuts, mut->changeteam(ci, ci->team, team));
			ci->team = team;
			ci->mapchange();
            ci->state.lasttimeplayed = lastmillis;
            if(ci->state.state != CS_SPECTATOR)
				sendspawn(ci);
		}

		if(m_timed(gamemode) && numclients(-1, false, true)) sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
		if(m_demo(gamemode)) setupdemoplayback();
		else if(demonextmatch)
		{
			demonextmatch = false;
			setupdemorecord();
		}
	}

	void parsecommand(clientinfo *ci, int nargs, char *cmd, char *arg)
	{
		s_sprintfd(cmdname)("sv_%s", cmd);
		ident *id = idents->access(cmdname);
		if(id && id->flags&IDF_SERVER)
		{
			if(haspriv(ci, PRIV_MASTER, true))
			{
				string val;
				val[0] = 0;
				switch(id->type)
				{
#if 0
					case ID_COMMAND:
					{
						string s;
						if(nargs <= 1 || !arg) s_sprintf(s)("sv_%s", cmd);
						else s_sprintf(s)("sv_%s %s", cmd, arg);
						char *ret = executeret(s);
						if(ret)
						{
							srvoutf("\fm%s: %s returned %s", colorname(ci), cmd, ret);
							delete[] ret;
						}
						return;
					}
#endif
					case ID_VAR:
					{
						if(nargs <= 1 || !arg)
						{
							srvmsgf(ci->clientnum, "\fm%s = %d", cmd, *id->storage.i);
							return;
						}
						if(id->maxval < id->minval)
						{
							srvmsgf(ci->clientnum, "\frcannot override variable: %s", cmd);
							return;
						}
						int ret = atoi(arg);
						if(ret < id->minval || ret > id->maxval)
						{
							srvmsgf(ci->clientnum, "\frvalid range for %s is %d..%d", cmd, id->minval, id->maxval);
							return;
						}
                        *id->storage.i = ret;
                        id->changed();
                        s_sprintf(val)("%d", *id->storage.i);
						break;
					}
					case ID_FVAR:
					{
						if(nargs <= 1 || !arg)
						{
							srvmsgf(ci->clientnum, "\fm%s = %f", cmd, *id->storage.f);
							return;
						}
						float ret = atof(arg);
                        *id->storage.f = ret;
                        id->changed();
                        s_sprintf(val)("%f", *id->storage.f);
						break;
					}
					case ID_SVAR:
					{
						if(nargs <= 1 || !arg)
						{
							srvmsgf(ci->clientnum, strchr(*id->storage.s, '"') ? "\fm%s = [%s]" : "\fm%s = \"%s\"", cmd, *id->storage.s);
							return;
						}
						delete[] *id->storage.s;
                        *id->storage.s = newstring(arg);
                        id->changed();
                        s_sprintf(val)("%s", *id->storage.s);
						break;
					}
					default: return;
				}
				sendf(-1, 1, "ri2ss", SV_COMMAND, ci->clientnum, cmd, val);
				relayf(2, "\fm%s set %s to %s", colorname(ci), cmd, val);
			}
		}
		else srvmsgf(ci->clientnum, "\frunknown command: %s", cmd);
	}

	clientinfo *choosebestclient()
	{
		clientinfo *best = NULL;
		loopv(clients)
		{
			clientinfo *cs = clients[i];
			if(cs->state.isai() || !cs->name[0] || !cs->online || !cs->connected || cs->wantsmap) continue;
			if(!best || cs->state.timeplayed > best->state.timeplayed) best = cs;
		}
		return best;
	}

    void sendinits2c(clientinfo *ci)
    {
        sendf(ci->clientnum, 1, "ri5", SV_INITS2C, ci->clientnum, GAMEVERSION, ci->sessionid, serverpass[0] ? 1 : 0);
    }

    void sendresume(clientinfo *ci)
    {
        savedscore &sc = findscore(ci, false);
        if(&sc)
        {
            sc.restore(ci->state);
            servstate &gs = ci->state;
            sendf(-1, 1, "ri7vi", SV_RESUME, ci->clientnum,
                gs.state, gs.frags,
                gs.lifesequence, gs.health,
                gs.weapselect, WEAPON_MAX, &gs.ammo[0], -1);
        }
    }

    void sendinitc2s(clientinfo *ci)
    {
        ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        enet_packet_resize(packet, buildinitc2s(ci, packet->data, packet->dataLength));
        sendpacket(-1, 1, packet, ci->clientnum);
        if(!packet->referenceCount) enet_packet_destroy(packet);
    }

    void sendwelcome(clientinfo *ci)
    {
        ENetPacket *packet = enet_packet_create (NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        ucharbuf p(packet->data, packet->dataLength);
        int chan = welcomepacket(p, ci, packet);
        enet_packet_resize(packet, p.length());
        sendpacket(ci->clientnum, chan, packet);
        if(!packet->referenceCount) enet_packet_destroy(packet);
    }

	int welcomepacket(ucharbuf &p, clientinfo *ci, ENetPacket *packet)
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
		else if(!ci->online && m_edit(gamemode) && numclients(ci->clientnum, false, true))
		{
			ci->wantsmap = true;
			clientinfo *best = choosebestclient();
			if(best)
			{
				loopi(3) if(mapdata[i])
				{
					fclose(mapdata[i]);
					mapdata[i] = NULL;
				}
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
			else putint(p, 0);
		}
		putint(p, gamemode);
		putint(p, mutators);

		enumerate(*idents, ident, id, {
			if(id.flags&IDF_SERVER) // reset vars
			{
				string val; val[0] = 0;
				s_sprintfd(cmd)("%s", &id.name[3]);
				switch(id.type)
				{
					case ID_VAR:
					{
						s_sprintf(val)("%d", *id.storage.i);
						break;
					}
					case ID_FVAR:
					{
						s_sprintf(val)("%f", *id.storage.f);
						break;
					}
					case ID_SVAR:
					{
						s_sprintf(val)("%s", *id.storage.s);
						break;
					}
					default: break;
				}
				if(cmd[0])
				{
					putint(p, SV_COMMAND);
					putint(p, -1);
					sendstring(cmd, p);
					sendstring(val, p);
				}
			}
		});

		if(!ci || (m_timed(gamemode) && numclients(-1, false, true)))
		{
			putint(p, SV_TIMEUP);
			putint(p, minremain);
		}

		if(!notgotinfo)
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
            putint(p, SV_SETTEAM);
            putint(p, ci->clientnum);
            putint(p, ci->team);
        }
		if(ci && ci->state.state!=CS_SPECTATOR)
		{
			int nospawn = 0;
			if(smode && !smode->canspawn(ci, true, false)) { nospawn++; }
			mutate(smuts, if(!mut->canspawn(ci, true, false)) { nospawn++; });
			if(nospawn)
			{
				ci->state.state = CS_WAITING;
				ci->state.weapreset(false);
				putint(p, SV_WAITING);
				putint(p, ci->clientnum);
				sendf(-1, 1, "ri2x", SV_WAITING, ci->clientnum, ci->clientnum);
			}
			else
			{
				spawnstate(ci);
				putint(p, SV_SPAWNSTATE);
				sendstate(ci, p, pickspawn(ci));
				ci->state.lastrespawn = ci->state.lastspawn = gamemillis;
			}
		}
		if(ci && ci->state.state==CS_SPECTATOR)
		{
			putint(p, SV_SPECTATOR);
			putint(p, ci->clientnum);
			putint(p, 1);
			sendf(-1, 1, "ri3x", SV_SPECTATOR, ci->clientnum, 1, ci->clientnum);
		}

		if(clients.length() > 1)
		{
			putint(p, SV_RESUME);
			loopv(clients)
			{
				clientinfo *oi = clients[i];
				if(ci && oi->clientnum == ci->clientnum) continue;
				sendstate(oi, p, -2);
			}
			putint(p, -1);
		}

		if(*servermotd)
		{
			putint(p, SV_SERVMSG);
			sendstring(servermotd, p);
		}

		enet_packet_resize(packet, packet->dataLength + MAXTRANS);
		p.buf = packet->data;
        p.maxlen = packet->dataLength;

		if(smode) smode->initclient(ci, p, true);
		mutate(smuts, mut->initclient(ci, p, true));
		ci->online = true;
		return 1;
	}

	void clearevent(clientinfo *ci)
	{
		ci->events.remove(0, 1);
	}

	void dodamage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush = ivec(0, 0, 0))
	{
		servstate &ts = target->state;
		int sp = GVAR(spawnprotecttime)*1000, pf = m_paint(gamemode, mutators) ? GVAR(paintfreezetime)*1000 : 0;
		if(ts.spawnprotect(gamemillis, sp, pf) || ts.damageprotect(gamemillis, pf)) return;

		int realdamage = damage, realflags = flags, nodamage = 0;
		if(smode && !smode->damage(target, actor, realdamage, weap, realflags, hitpush)) { nodamage++; }
		mutate(smuts, if(!mut->damage(target, actor, realdamage, weap, realflags, hitpush)) { nodamage++; });
		if(!m_play(gamemode) || (!GVAR(teamdamage) && m_team(gamemode, mutators) && actor->team == target->team && actor != target))
			nodamage++;

		if(nodamage || !hithurts(realflags)) realflags = HIT_WAVE; // so it impacts, but not hurts
		if(hithurts(realflags))
		{
			if(realflags&HIT_FULL || realflags&HIT_HEAD) realdamage = int(realdamage*GVAR(damagescale));
			else if(realflags&HIT_TORSO) realdamage = int(realdamage*0.5f*GVAR(damagescale));
			else if(realflags&HIT_LEGS) realdamage = int(realdamage*0.25f*GVAR(damagescale));
			else realdamage = int(realdamage*GVAR(damagescale));
			ts.dodamage(gamemillis, (ts.health -= realdamage));
			actor->state.damage += realdamage;
		}
		else realdamage = int(realdamage*0.5f*GVAR(damagescale));

		if(hithurts(realflags) && realdamage && ts.health <= 0) realflags |= HIT_KILL;
		else realflags &= ~HIT_KILL;

		sendf(-1, 1, "ri7i3", SV_DAMAGE, target->clientnum, actor->clientnum, weap, realflags, realdamage, ts.health, hitpush.x, hitpush.y, hitpush.z);

		if(realflags&HIT_KILL || m_paint(gamemode, mutators))
		{
            if(realflags&HIT_KILL)
            {
				target->state.deaths++;
				if(actor!=target && m_team(gamemode, mutators) && actor->team == target->team) actor->state.teamkills++;
            }

            int fragvalue = smode ? smode->fragvalue(target, actor) : (target == actor || (m_team(gamemode, mutators) && target->team == actor->team) ? -1 : 1);
            if(m_paint(gamemode, mutators) && !m_insta(gamemode, mutators) && realflags&HIT_KILL) fragvalue *= 2;
            actor->state.frags += fragvalue;

			if(fragvalue > 0)
			{
				int friends = 0, enemies = 0; // note: friends also includes the fragger
				if(m_team(gamemode, mutators)) loopv(clients) if(clients[i]->team != actor->team) enemies++; else friends++;
				else { friends = 1; enemies = clients.length()-1; }
				actor->state.effectiveness += fragvalue*friends/float(max(enemies, 1));
				actor->state.spree++;
			}
			sendf(-1, 1, "ri4", SV_FRAG, actor->clientnum, actor->state.frags, actor->state.spree);

			if(realflags&HIT_KILL)
			{
				dropitems(target);
				// don't issue respawn yet until DEATHMILLIS has elapsed
				sendf(-1, 1, "ri6", SV_DIED, target->clientnum, actor->clientnum, weap, realflags, realdamage);
				target->position.setsizenodelete(0);
				if(smode) smode->died(target, actor);
				mutate(smuts, mut->died(target, actor));
				ts.state = CS_DEAD;
				ts.lastdeath = gamemillis;
				ts.weapreset(false);
			}
		}
	}

	void processevent(clientinfo *ci, suicideevent &e)
	{
		servstate &gs = ci->state;
		if(gs.state != CS_ALIVE) return;
        ci->state.frags += smode ? smode->fragvalue(ci, ci) : -1;
        ci->state.deaths++;
		dropitems(ci);
		sendf(-1, 1, "ri4", SV_FRAG, ci->clientnum, ci->state.frags, ci->state.spree);
		sendf(-1, 1, "ri6", SV_DIED, ci->clientnum, ci->clientnum, -1, e.flags, ci->state.health);
        ci->position.setsizenodelete(0);
		if(smode) smode->died(ci, NULL);
		mutate(smuts, mut->died(ci, NULL));
		gs.state = CS_DEAD;
		gs.lastdeath = gamemillis;
		gs.weapreset(false);
	}

	void processevent(clientinfo *ci, destroyevent &e)
	{
		servstate &gs = ci->state;
		if(isweap(e.weap))
		{
			if(!gs.weapshots[e.weap].find(e.id) < 0) return;
			else if(!weaptype[e.weap].radial || !e.radial) // 0 is destroy
			{
				gs.weapshots[e.weap].remove(e.id);
				e.radial = weaptype[e.weap].explode;
			}
			loopv(e.hits)
			{
				hitset &h = e.hits[i];
				float size = e.radial ? (h.flags&HIT_WAVE ? e.radial*3 : e.radial) : 0.f,
					dist = float(h.dist)/DMF;
				clientinfo *target = (clientinfo *)getinfo(h.target);
				if(!target || target->state.state!=CS_ALIVE || h.lifesequence!=target->state.lifesequence || (size && (dist<0 || dist>size))) continue;
				int damage = e.radial  ? int(weaptype[e.weap].damage*(1.f-dist/EXPLOSIONSCALE/size)) : weaptype[e.weap].damage;
				dodamage(target, ci, damage, e.weap, h.flags, h.dir);
			}
		}
		else if(e.weap == -1) gs.dropped.remove(e.id);
	}

	void processevent(clientinfo *ci, shotevent &e)
	{
		servstate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !isweap(e.weap))
		{
			if(weaptype[e.weap].max && isweap(e.weap)) gs.ammo[e.weap] = max(gs.ammo[e.weap]-1, 0); // keep synched!
			if(GVAR(serverdebug) > 1) srvmsgf(ci->clientnum, "sync error: shoot [%d] failed - unexpected message", e.weap);
			return;
		}
		if(!gs.canshoot(e.weap, m_spawnweapon(gamemode, mutators), e.millis))
		{
			if(weaptype[e.weap].max && isweap(e.weap)) gs.ammo[e.weap] = max(gs.ammo[e.weap]-1, 0); // keep synched!
			if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: shoot [%d] failed - current state disallows it", e.weap);
			return;
		}
		if(weaptype[e.weap].max) gs.ammo[e.weap] = max(gs.ammo[e.weap]-1, 0); // keep synched!
		gs.setweapstate(e.weap, WPSTATE_SHOOT, weaptype[e.weap].adelay, e.millis);
		sendf(-1, 1, "ri7ivx", SV_SHOTFX, ci->clientnum,
			e.weap, e.power, e.from[0], e.from[1], e.from[2],
					e.shots.length(), e.shots.length()*sizeof(ivec)/sizeof(int), e.shots.getbuf(),
						ci->clientnum);
		gs.shotdamage += weaptype[e.weap].damage*e.weap*e.num;
	}

	void processevent(clientinfo *ci, switchevent &e)
	{
		servstate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !isweap(e.weap))
		{
			if(GVAR(serverdebug) > 1) srvmsgf(ci->clientnum, "sync error: switch [%d] failed - unexpected message", e.weap);
			return;
		}
		if(!gs.canswitch(e.weap, m_spawnweapon(gamemode, mutators), e.millis))
		{
			if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: switch [%d] failed - current state disallows it", e.weap);
			return;
		}
		gs.weapswitch(e.weap, e.millis);
		sendf(-1, 1, "ri3", SV_WEAPSELECT, ci->clientnum, e.weap);
	}

	void processevent(clientinfo *ci, reloadevent &e)
	{
		servstate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !isweap(e.weap))
		{
			if(GVAR(serverdebug) > 1) srvmsgf(ci->clientnum, "sync error: reload [%d] failed - unexpected message", e.weap);
			return;
		}
		if(!gs.canreload(e.weap, m_spawnweapon(gamemode, mutators), e.millis))
		{
			if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: reload [%d] failed - current state disallows it", e.weap);
			return;
		}
		gs.setweapstate(e.weap, WPSTATE_RELOAD, weaptype[e.weap].rdelay, e.millis);
		gs.ammo[e.weap] = clamp(max(gs.ammo[e.weap], 0) + weaptype[e.weap].add, weaptype[e.weap].add, weaptype[e.weap].max);
		sendf(-1, 1, "ri4", SV_RELOAD, ci->clientnum, e.weap, gs.ammo[e.weap]);
	}

	void processevent(clientinfo *ci, useevent &e)
	{
		servstate &gs = ci->state;
		if(gs.state != CS_ALIVE || m_noitems(gamemode, mutators) || !sents.inrange(e.ent))
		{
			if(GVAR(serverdebug) > 1) srvmsgf(ci->clientnum, "sync error: use [%d] failed - unexpected message", e.ent);
			return;
		}
		int sweap = m_spawnweapon(gamemode, mutators), attr = sents[e.ent].type == WEAPON ? weapattr(sents[e.ent].attr[0], sweap) : sents[e.ent].attr[0];
		if(!gs.canuse(sents[e.ent].type, attr, sents[e.ent].attr[1], sents[e.ent].attr[2], sents[e.ent].attr[3], sents[e.ent].attr[4], sweap, e.millis))
		{
			if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: use [%d] failed - current state disallows it", e.ent);
			return;
		}
		if(!sents[e.ent].spawned && !(sents[e.ent].attr[1]&WEAPFLAG_FORCED))
		{
			bool found = false;
			loopv(clients)
			{
				clientinfo *cp = clients[i];
				if(cp->state.dropped.projs.find(e.ent) >= 0)
				{
					cp->state.dropped.remove(e.ent);
					found = true;
				}
			}
			if(!found)
			{
				if(GVAR(serverdebug)) srvmsgf(ci->clientnum, "sync error: use [%d] failed - doesn't seem to be spawned anywhere", e.ent);
				return;
			}
		}

		int weap = -1, dropped = -1;
		if(sents[e.ent].type == WEAPON && gs.ammo[attr] < 0 && gs.carry(sweap) >= GVAR(maxcarry)) weap = gs.drop(attr, sweap);
		if(isweap(weap))
		{
			dropped = gs.entid[weap];
			gs.setweapstate(weap, WPSTATE_SWITCH, WEAPSWITCHDELAY, e.millis);
			gs.ammo[weap] = gs.entid[weap] = -1;
			gs.weapselect = weap;
		}
		gs.useitem(e.ent, sents[e.ent].type, attr, sents[e.ent].attr[1], sents[e.ent].attr[2], sents[e.ent].attr[3], sents[e.ent].attr[4], sweap, e.millis);
		if(sents.inrange(dropped))
		{
			gs.dropped.add(dropped);
			if(!(sents[dropped].attr[1]&WEAPFLAG_FORCED))
			{
				sents[dropped].spawned = false;
				sents[dropped].millis = gamemillis;
			}
		}
		if(!(sents[e.ent].attr[1]&WEAPFLAG_FORCED))
		{
			sents[e.ent].spawned = false;
			sents[e.ent].millis = gamemillis;
		}
		sendf(-1, 1, "ri6", SV_ITEMACC, ci->clientnum, e.ent, sents[e.ent].spawned ? 1 : 0, weap, dropped);
	}

	void flushevents(clientinfo *ci, int millis)
	{
		while(ci->events.length())
		{
			gameevent &ev = ci->events[0];
			#define chkevent(q) \
			{ \
				if(q.millis > millis) return; \
				else if(q.millis >= ci->lastevent) \
				{ \
					ci->lastevent = q.millis; \
					processevent(ci, q); \
				} \
				clearevent(ci); \
			}
			switch(ev.type)
			{
				case GAMEEVENT_SHOT: { chkevent(ev.shot); break; }
				case GAMEEVENT_SWITCH: { chkevent(ev.weapsel); break; }
				case GAMEEVENT_RELOAD: { chkevent(ev.reload); break; }
				case GAMEEVENT_DESTROY: { chkevent(ev.destroy); break; }
				case GAMEEVENT_USE: { chkevent(ev.use); break; }
				case GAMEEVENT_SUICIDE: { processevent(ci, ev.suicide); clearevent(ci); break; }
			}
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
			switch(ci->events[i].type)
			{
				case GAMEEVENT_DESTROY:
					if(keep < i) { ci->events.remove(keep, i - keep); i = keep; }
					keep = i+1;
					continue;
			}
		}
		ci->events.setsize(keep);
	}

	void hashpassword(int cn, int sessionid, const char *pwd, char *result)
	{
		char buf[2*sizeof(string)];
		s_sprintf(buf)("%d %d ", cn, sessionid);
		s_strcpy(&buf[strlen(buf)], pwd);
		tiger::hashval hv;
		tiger::hash((uchar *)buf, strlen(buf), hv);
		loopi(sizeof(hv.bytes))
		{
			uchar c = hv.bytes[i];
			*result++ = "0123456789abcdef"[c&0xF];
			*result++ = "0123456789abcdef"[c>>4];
		}
		*result = '\0';
	}

	bool checkpassword(clientinfo *ci, const char *wanted, const char *given)
	{
		string hash;
		hashpassword(ci->clientnum, ci->sessionid, wanted, hash);
		return !strcmp(hash, given);
    }

	#include "auth.h"

	int triggertime(int i)
	{
		if(sents.inrange(i)) switch(sents[i].type)
		{
			case TRIGGER: case MAPMODEL: case PARTICLES: case MAPSOUND:
				return m_speedtime(1000); break;
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
				if(sents[i].attr[1] == TR_LINK && sents[i].spawned && gamemillis-sents[i].millis >= triggertime(i)*2)
				{
					sents[i].spawned = false;
					sents[i].millis = gamemillis;
					sendf(-1, 1, "ri2", SV_TRIGGER, i, 0);
				}
				break;
			}
			default:
			{
				if(!m_noitems(gamemode, mutators) && enttype[sents[i].type].usetype == EU_ITEM)
				{
					if(!finditem(i, true, GVAR(itemspawntime)*1000))
					{
						loopvk(clients)
						{
							clientinfo *ci = clients[k];
							ci->state.dropped.remove(i);
							loopj(WEAPON_MAX) if(ci->state.entid[j] == i)
								ci->state.entid[j] = -1;
						}
						sents[i].spawned = true;
						sents[i].millis = gamemillis;
						sendf(-1, 1, "ri2", SV_ITEMSPAWN, i);
					}
				}
				break;
			}
		}
	}

	void checkclients()
	{
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->state.state == CS_ALIVE)
			{
				int m = m_maxhealth(gamemode, mutators);
				if(m_regen(gamemode, mutators) && ci->state.health < m && GVAR(regenhealth) && GVAR(regendelay) && GVAR(regentime))
				{
					int lastpain = gamemillis-ci->state.lastpain, lastregen = gamemillis-ci->state.lastregen;
					if((!ci->state.lastregen && lastpain >= GVAR(regendelay)*1000) || (ci->state.lastregen && lastregen >= GVAR(regentime)*1000))
					{
						int health = ci->state.health - (ci->state.health % GVAR(regenhealth));
						ci->state.health = min(health + GVAR(regenhealth), m);
						ci->state.lastregen = gamemillis;
						sendf(-1, 1, "ri3", SV_REGEN, ci->clientnum, ci->state.health);
					}
				}
			}
			else if(ci->state.state == CS_WAITING)
			{ // duelmut needs rewriting to take advantage of this
				if(!ci->state.respawnwait(gamemillis, m_spawndelay(gamemode, mutators)))
				{
					int nospawn = 0;
					if(smode && !smode->canspawn(ci, false, false)) { nospawn++; }
					mutate(smuts, if (!mut->canspawn(ci, false, false)) { nospawn++; });
					if(!nospawn)
					{
						ci->state.state = CS_DEAD; // safety
						ci->state.respawn(gamemillis, m_maxhealth(gamemode, mutators));
						sendspawn(ci);
					}
				}
			}
		}
	}

	void serverupdate()
	{
		if(numclients(-1, false, true))
		{
			gamemillis += curtime;
			if(m_demo(gamemode)) readdemo();
			else if(!interm)
			{
				processevents();
				checkents();
				if(smode) smode->update();
				mutate(smuts, mut->update());
			}

			while(bannedips.length() && bannedips[0].time-totalmillis>4*60*60000)
				bannedips.remove(0);
			loopv(connects) if(totalmillis-connects[i]->connectmillis>15000)
				disconnect_client(connects[i]->clientnum, DISC_TIMEOUT);

			if(masterupdate)
			{
				clientinfo *m = currentmaster>=0 ? (clientinfo *)getinfo(currentmaster) : NULL;
				sendf(-1, 1, "ri3", SV_CURRENTMASTER, currentmaster, m ? m->privilege : 0);
				masterupdate = false;
			}

			if((m_timed(gamemode) && numclients(-1, false, true)) &&
				((GVAR(timelimit) != oldtimelimit) || (gamemillis-curtime>0 && gamemillis/60000!=(gamemillis-curtime)/60000)))
					checkintermission();

			if(interm && gamemillis >= interm) // wait then call for next map
			{
				if(GVAR(votelimit) && !maprequest)
				{
					if(demorecord) enddemorecord();
					sendf(-1, 1, "ri", SV_NEWGAME);
					maprequest = true;
					interm = gamemillis+(GVAR(votelimit)*1000);
				}
				else
				{
					interm = 0;
					checkvotes(true);
				}
			}
			else aiman::checkai();
			checkclients();
		}
		auth::update();
	}

    bool allowbroadcast(int n)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        return ci && ci->connected && !ci->state.isai();
    }

    int peerowner(int n)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        if(ci) return ci->state.isai() ? ci->state.ownernum : ci->clientnum;
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
        if(numclients(-1, false, true) >= serverclients) return DISC_MAXCLIENTS;
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
        if(!local && m_demo(gamemode)) return DISC_PRIVATE;
        sendinits2c(ci);
		return DISC_NONE;
	}

	void clientdisconnect(int n, bool local)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		bool complete = !numclients(n, false, true);
        if(ci->connected)
        {
		    if(ci->privilege) auth::setmaster(ci, false);
		    if(smode) smode->leavegame(ci, true);
		    mutate(smuts, mut->leavegame(ci));
		    ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		    savescore(ci);
		    dropitems(ci, true);
		    sendf(-1, 1, "ri2", SV_CDIS, n);
		    ci->connected = false;
		    if(ci->name[0]) relayf(2, "\fo%s has left the game", colorname(ci));
		    aiman::removeai(ci, complete);
		    clients.removeobj(ci);
        }
        else connects.removeobj(ci);
		if(complete) cleanup();
		else checkvotes();
	}

	#include "extinfo.h"
    void queryreply(ucharbuf &req, ucharbuf &p)
	{
        if(!getint(req))
        {
            extqueryreply(req, p);
            return;
        }
		putint(p, numclients(-1, false, true));
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
			sendstring("umanned", p);
			#else
			char *ret = executeret("getname");
			if(ret)
			{
				sendstring(ret, p);
				delete[] ret;
			}
			else sendstring("unnamed", p);
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
			fclose(mapdata[n]);
			mapdata[n] = NULL;
		}
		if(!len)
		{
			srvmsgf(sender, "you sent a zero length packet for map data!");
			return false;
		}
		mapdata[n] = tmpfile();
        if(!mapdata[n])
        {
        	srvmsgf(sender, "failed to open temporary file for map");
        	return false;
		}
		mapsending = true;
		fwrite(data, 1, len, mapdata[n]);
		return n == 2;
	}

	int checktype(int type, clientinfo *ci)
	{
#if 0
        // other message types can get sent by accident if a master forces spectator on someone, so disabling this case for now and checking for spectator state in message handlers
        // spectators can only connect and talk
		static int spectypes[] = { SV_INITC2S, SV_POS, SV_TEXT, SV_PING, SV_CLIENTPING, SV_GETMAP, SV_SETMASTER };
		if(ci && ci->state.state==CS_SPECTATOR && !haspriv(ci, PRIV_MASTER))
		{
			loopi(sizeof(spectypes)/sizeof(int)) if(type == spectypes[i]) return type;
			return -1;
		}
#endif
		// only allow edit messages in coop-edit mode
		if(type >= SV_EDITENT && type <= SV_NEWMAP && !m_edit(gamemode)) return -1;
		// server only messages
		static int servtypes[] = { SV_INITS2C, SV_WELCOME, SV_NEWGAME, SV_MAPCHANGE, SV_SERVMSG, SV_DAMAGE, SV_SHOTFX, SV_DIED, SV_FRAG, SV_SPAWNSTATE, SV_FORCEDEATH, SV_ITEMACC, SV_ITEMSPAWN, SV_TIMEUP, SV_CDIS, SV_CURRENTMASTER, SV_PONG, SV_RESUME, SV_TEAMSCORE, SV_FLAGINFO, SV_ANNOUNCE, SV_SENDDEMOLIST, SV_SENDDEMO, SV_DEMOPLAYBACK, SV_REGEN, SV_SCOREFLAG, SV_RETURNFLAG, SV_CLIENT, SV_AUTHCHAL };
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
		static struct { int posoff, msgoff, msglen; } pkt[MAXCLIENTS];
		worldstate &ws = *new worldstate;
		loopv(clients)
		{
			clientinfo &ci = *clients[i];
			if(ci.position.empty()) pkt[i].posoff = -1;
			else
			{
				pkt[i].posoff = ws.positions.length();
				loopvj(ci.position) ws.positions.add(ci.position[j]);
			}
			if(ci.messages.empty()) pkt[i].msgoff = -1;
			else
			{
				pkt[i].msgoff = ws.messages.length();
				ucharbuf p = ws.messages.reserve(16);
				putint(p, SV_CLIENT);
				putint(p, ci.clientnum);
				putuint(p, ci.messages.length());
				ws.messages.addbuf(p);
				loopvj(ci.messages) ws.messages.add(ci.messages[j]);
				pkt[i].msglen = ws.messages.length() - pkt[i].msgoff;
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
			if(!ci.state.isai() && psize && (pkt[i].posoff<0 || psize-ci.position.length()>0))
			{
				packet = enet_packet_create(&ws.positions[pkt[i].posoff<0 ? 0 : pkt[i].posoff+ci.position.length()],
											pkt[i].posoff<0 ? psize : psize-ci.position.length(),
											ENET_PACKET_FLAG_NO_ALLOCATE);
				sendpacket(ci.clientnum, 0, packet);
				if(!packet->referenceCount) enet_packet_destroy(packet);
				else { ++ws.uses; packet->freeCallback = freecallback; }
			}
			ci.position.setsizenodelete(0);

			if(!ci.state.isai() && msize && (pkt[i].msgoff<0 || msize-pkt[i].msglen>0))
			{
				packet = enet_packet_create(&ws.messages[pkt[i].msgoff<0 ? 0 : pkt[i].msgoff+pkt[i].msglen],
											pkt[i].msgoff<0 ? msize : msize-pkt[i].msglen,
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

	void parsepacket(int sender, int chan, bool reliable, ucharbuf &p)	 // has to parse exactly each byte of the packet
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
                filtertext(text, text, false, MAXNAMELEN);
                if(!text[0]) s_strcpy(text, "unnamed");
                s_strncpy(ci->name, text, MAXNAMELEN+1);

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
                if(mastermode>=MM_LOCKED) ci->state.state = CS_SPECTATOR;
                if(currentmaster>=0) masterupdate = true;
                ci->state.lasttimeplayed = lastmillis;

                ci->team = chooseteam(ci);

                sendwelcome(ci);
                sendresume(ci);
                sendinitc2s(ci);
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
		if(reliable) reliablemessages = true;
		#define QUEUE_MSG { while(curmsg<p.length()) ci->messages.add(p.buf[curmsg++]); }
        #define QUEUE_BUF(size, body) { \
            curmsg = p.length(); \
            ucharbuf buf = ci->messages.reserve(size); \
            { body; } \
            ci->messages.addbuf(buf); \
        }
        #define QUEUE_INT(n) QUEUE_BUF(5, putint(buf, n))
        #define QUEUE_UINT(n) QUEUE_BUF(4, putuint(buf, n))
        #define QUEUE_FLT(n) QUEUE_BUF(4, putfloat(buf, n))
        #define QUEUE_STR(text) QUEUE_BUF(2*strlen(text)+1, sendstring(text, buf))

		static gameevent dummyevent;
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
					if(havecn)
					{
						if(smode) smode->moved(cp, oldpos, cp->state.o);
						mutate(smuts, mut->moved(cp, oldpos, cp->state.o));
					}
					break;
				}

				case SV_PHYS:
				{
					int lcn = getint(p);
					getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					QUEUE_MSG;
					break;
				}

				case SV_EDITMODE:
				{
					int val = getint(p);
					if((!val && ci->state.state != CS_EDITING) || !m_edit(gamemode)) break;
					if(val && ci->state.state != CS_ALIVE && ci->state.state != CS_SPECTATOR) break;
					if(smode)
					{
						if(val) smode->leavegame(ci);
						else smode->entergame(ci);
					}
					mutate(smuts, {
						if (val) mut->leavegame(ci);
						else mut->entergame(ci);
					});
					ci->state.state = val ? CS_EDITING : CS_ALIVE;
					if(val)
					{
						ci->events.setsizenodelete(0);
						ci->state.dropped.reset();
						loopk(WEAPON_MAX) ci->state.weapshots[k].reset();
						ci->state.weapreset(false);
					}
					QUEUE_MSG;
					break;
				}

				case SV_TRYSPAWN:
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					if(cp->state.state != CS_DEAD || cp->state.lastrespawn >= 0) break;
					int sdelay = m_spawndelay(gamemode, mutators), wait = cp->state.respawnwait(gamemillis, sdelay);
					if(wait && sdelay-wait <= min(sdelay, GVAR(spawndelaywait)*1000)) break;
					int nospawn = 0;
					if(smode && !smode->canspawn(cp, false, true)) { nospawn++; }
					mutate(smuts, if (!mut->canspawn(cp, false, true)) { nospawn++; });
					if(!nospawn)
					{
						if(cp->state.lastdeath) flushevents(cp, cp->state.lastdeath + DEATHMILLIS);
						cleartimedevents(cp);
						sendf(-1, 1, "ri2", SV_WAITING, cp->clientnum);
						cp->state.state = CS_WAITING;
						cp->state.weapreset(false);
					}
					break;
				}

				case SV_WEAPSELECT:
				{
					int lcn = getint(p), id = getint(p), weap = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					gameevent &ev = cp->addevent();
					ev.type = GAMEEVENT_SWITCH;
					ev.weapsel.id = id;
					ev.weapsel.weap = weap;
					ev.weapsel.millis = cp->getmillis(gamemillis, ev.weapsel.id);
					break;
				}

				case SV_SPAWN:
				{
					int lcn = getint(p), ls = getint(p), weapselect = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					if((cp->state.state!=CS_ALIVE && cp->state.state!=CS_DEAD) || ls!=cp->state.lifesequence || cp->state.lastrespawn<0)
						break;
					cp->state.lastrespawn = -1;
					cp->state.state = CS_ALIVE;
					cp->state.weapselect = weapselect;
					if(smode) smode->spawned(cp);
					mutate(smuts, mut->spawned(cp););
					QUEUE_BUF(100,
					{
						putint(buf, SV_SPAWN);
						sendstate(cp, buf, -2);
					});
					break;
				}

				case SV_SUICIDE:
				{
					int lcn = getint(p), flags = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					gameevent &ev = cp->addevent();
					ev.type = GAMEEVENT_SUICIDE;
					ev.suicide.flags = flags;
					break;
				}

				case SV_SHOOT:
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
					gameevent &ev = havecn ? cp->addevent() : dummyevent;
					ev.type = GAMEEVENT_SHOT;
					ev.shot.id = getint(p);
					ev.shot.weap = getint(p);
					ev.shot.power = getint(p);
					if(havecn) ev.shot.millis = cp->getmillis(gamemillis, ev.shot.id);
					loopk(3) ev.shot.from[k] = getint(p);
					ev.shot.num = getint(p);
					loopj(ev.shot.num)
					{
                        if(p.overread() || !isweap(ev.shot.weap) || j >= weaptype[ev.shot.weap].rays) break;
						ivec &dest = ev.shot.shots.add();
						loopk(3) dest[k] = getint(p);
					}
					break;
				}

				case SV_DROP:
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
					int sweap = m_spawnweapon(gamemode, mutators), weap = getint(p);
					if(!havecn || !isweap(weap) || !cp->state.hasweap(weap, sweap) || m_noitems(gamemode, mutators)) break;
					if(!sents.inrange(cp->state.entid[weap]) || (sents[cp->state.entid[weap]].attr[1]&WEAPFLAG_FORCED)) break;
					cp->state.dropped.add(cp->state.entid[weap]);
					sents[cp->state.entid[weap]].spawned = false;
					sents[cp->state.entid[weap]].millis = gamemillis;
					sendf(-1, 1, "ri5", SV_DROP, cp->clientnum, 1, weap, cp->state.entid[weap]);
					cp->state.setweapstate(weap, WPSTATE_SWITCH, 0, 0);
					cp->state.entid[weap] = cp->state.ammo[weap] = -1;
					break;
				}

				case SV_RELOAD:
				{
					int lcn = getint(p), id = getint(p), weap = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum != ci->clientnum && cp->state.ownernum != ci->clientnum))
						break;
					gameevent &ev = cp->addevent();
					ev.type = GAMEEVENT_RELOAD;
					ev.reload.id = id;
					ev.reload.weap = weap;
					ev.reload.millis = cp->getmillis(gamemillis, ev.reload.id);
					break;
				}

				case SV_DESTROY: // cn millis weap id radial hits
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
					gameevent &ev = havecn ? cp->addevent() : dummyevent;
					ev.type = GAMEEVENT_DESTROY;
					ev.destroy.id = getint(p);
					if(havecn) ev.destroy.millis = cp->getmillis(gamemillis, ev.destroy.id); // this is the event millis
					ev.destroy.weap = getint(p);
					ev.destroy.id = getint(p); // this is the actual id
					ev.destroy.radial = getint(p);
					int hits = getint(p);
					loopj(hits)
					{
						hitset &hit = havecn && j < MAXCLIENTS ? ev.destroy.hits.add() : dummyevent.destroy.hits.add();
						hit.flags = getint(p);
						hit.target = getint(p);
						hit.lifesequence = getint(p);
						hit.dist = getint(p);
						loopk(3) hit.dir[k] = getint(p);
					}
					break;
				}

				case SV_ITEMUSE:
				{
					int lcn = getint(p), id = getint(p), ent = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					gameevent &ev = cp->addevent();
					ev.type = GAMEEVENT_USE;
					ev.use.id = id;
					ev.use.ent = ent;
					ev.use.millis = cp->getmillis(gamemillis, ev.use.id);
					break;
				}

				case SV_TRIGGER:
				{
					int lcn = getint(p), ent = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					if(sents.inrange(ent) && sents[ent].type == TRIGGER)
					{
						bool commit = false;
						switch(sents[ent].attr[1])
						{
							case TR_NONE:
							{
								if(!sents[ent].spawned || sents[ent].attr[2] != TA_AUTO)
								{
									sents[ent].millis = gamemillis;
									sents[ent].spawned = !sents[ent].spawned;
									commit = true;
								}
								//else sendf(cp->clientnum, 1, "ri3", SV_TRIGGER, ent, sents[ent].spawned ? 1 : 0);
								break;
							}
							case TR_LINK:
							{
								sents[ent].millis = gamemillis;
								if(!sents[ent].spawned)
								{
									sents[ent].spawned = true;
									commit = true;
								}
								//else sendf(cp->clientnum, 1, "ri3", SV_TRIGGER, ent, sents[ent].spawned ? 1 : 0);
								break;
							}
						}
						if(commit) sendf(-1, 1, "ri3", SV_TRIGGER, ent, sents[ent].spawned ? 1 : 0);
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
					if(flags&SAY_TEAM && cp->state.state==CS_SPECTATOR) break;
					loopv(clients)
					{
						clientinfo *t = clients[i];
						if(t == cp || t->state.isai() || (flags&SAY_TEAM && (t->state.state==CS_SPECTATOR || cp->team != t->team))) continue;
						sendf(t->clientnum, 1, "ri3s", SV_TEXT, cp->clientnum, flags, text);
					}
					bool team = m_team(gamemode, mutators) && flags&SAY_TEAM;
					s_sprintfd(m)("%s", colorname(cp));
					if(team)
					{
						s_sprintfd(t)(" (\fs%s%s\fS)", teamtype[cp->team].chat, teamtype[cp->team].name);
						s_strcat(m ,t);
					}
					if(flags&SAY_ACTION) relayf(1, "\fm* \fs%s\fS \fs\fm%s\fS", m, text);
					else relayf(1, "\fa<\fs\fw%s\fS> \fs\fw%s\fS", m, text);
					break;
				}

				case SV_COMMAND:
				{
					int lcn = getint(p), nargs = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					string cmd;
					getstring(cmd, p);
					if(nargs > 1) getstring(text, p);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					parsecommand(cp, nargs, cmd, nargs > 1 ? text : NULL);
					break;
				}

				case SV_INITC2S:
				{
					getstring(text, p);
					if(!text[0]) s_strcpy(text, "unnamed");
				    if(strcmp(ci->name, text))
					{
						string oldname, newname;
						s_strcpy(oldname, colorname(ci));
						s_strcpy(newname, colorname(ci, text));
						relayf(2, "\fm%s is now known as %s", oldname, newname);
					}
					s_strncpy(ci->name, text, MAXNAMELEN+1);
					int team = getint(p);
					if(!isteam(gamemode, mutators, team, TEAM_FIRST))
					{
						team = chooseteam(ci, team);
						sendf(sender, 1, "ri3", SV_SETTEAM, sender, team);
					}
					if(team != ci->team)
					{
						if(smode) smode->changeteam(ci, ci->team, team);
						mutate(smuts, mut->changeteam(ci, ci->team, team));
						ci->team = team;
						aiman::dorefresh = true;
					}
                    sendinitc2s(ci);
					break;
				}

				case SV_MAPVOTE:
				{
					getstring(text, p);
					filtertext(text, text);
					int reqmode = getint(p), reqmuts = getint(p);
					vote(text, reqmode, reqmuts, sender);
					break;
				}

				case SV_GAMEINFO:
				{
					int n, np = getint(p);
					while((n = getint(p)) != -1)
					{
						int type = getint(p), attr1 = getint(p), attr2 = getint(p),
							attr3 = getint(p), attr4 = getint(p), attr5 = getint(p);
						if(notgotinfo && (enttype[type].usetype == EU_ITEM || type == PLAYERSTART || type == TRIGGER))
						{
							while(sents.length() <= n) sents.add();
							sents[n].type = type;
							sents[n].attr[0] = attr1;
							sents[n].attr[1] = attr2;
							sents[n].attr[2] = attr3;
							sents[n].attr[3] = attr4;
							sents[n].attr[4] = attr5;
							sents[n].spawned = false; // wait a bit then load 'em up
							sents[n].millis = gamemillis;
							if(enttype[sents[n].type].usetype == EU_ITEM)
							{
								sents[n].millis -= GVAR(itemspawntime)*1000; // rewind
								if(GVAR(itemspawndelay)) sents[n].millis += GVAR(itemspawndelay)*1000;
							}
						}
					}
					if(notgotinfo)
					{
						loopvk(clients)
						{
							clientinfo *cp = clients[k];
							cp->state.dropped.reset();
							cp->state.weapreset(false);
						}
						setupspawns(true, np);
						notgotinfo = false;
					}
#if 0
					else // get their state in check then?
					{
						vector<int> gdat;
						loopvk(sents) if(enttype[sents[k].type].usetype == EU_ITEM || sents[k].type == TRIGGER)
						{
							gdat.add(k);
							if(enttype[sents[k].type].usetype == EU_ITEM) gdat.add(finditem(k, false) ? 1 : 0);
							else gdat.add(sents[k].spawned ? 1 : 0);
						}
						gdat.add(-1);
						sendf(sender, 1, "riiv", SV_GAMEINFO, gdat.length(), gdat.length(), gdat.getbuf());
					}
#endif
					break;
				}

				case SV_TEAMSCORE:
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
					if(haspriv(ci, PRIV_MASTER, true) && mm >= MM_OPEN && mm <= MM_PRIVATE)
					{
						if(haspriv(ci, PRIV_ADMIN) || (mastermask&(1<<mm)))
						{
							mastermode = mm;
                            allowedips.setsize(0);
                            if(mm>=MM_PRIVATE)
                            {
                                loopv(clients) allowedips.add(getclientip(clients[i]->clientnum));
                            }
							srvoutf("mastermode is now %d", mastermode);
						}
						else
						{
							srvmsgf(sender, "mastermode %d is disabled on this server", mm);
						}
					}
					break;
				}

				case SV_CLEARBANS:
				{
					if(haspriv(ci, PRIV_MASTER, true))
					{
						bannedips.setsize(0);
						srvoutf("cleared all bans");
					}
					break;
				}

				case SV_KICK:
				{
					int victim = getint(p);
					if(haspriv(ci, PRIV_MASTER, true) && victim>=0 && victim<getnumclients() && ci->clientnum!=victim && getinfo(victim))
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
					if(((mastermode >= 2 && ci->state.state==CS_SPECTATOR) || spectator != sender) && !haspriv(ci, PRIV_MASTER, true)) break;
					clientinfo *spinfo = (clientinfo *)getinfo(spectator);
					if(!spinfo) break;

					sendf(-1, 1, "ri3", SV_SPECTATOR, spectator, val);

					if(spinfo->state.state!=CS_SPECTATOR && val)
					{
						dropitems(spinfo);
						if(smode) smode->leavegame(spinfo);
						mutate(smuts, mut->leavegame(spinfo));
						spinfo->state.state = CS_SPECTATOR;
                    	spinfo->state.timeplayed += lastmillis - spinfo->state.lasttimeplayed;
                    	aiman::dorefresh = true;
					}
					else if(spinfo->state.state==CS_SPECTATOR && !val)
					{
						spinfo->state.state = CS_DEAD;
						spinfo->state.respawn(gamemillis, m_maxhealth(gamemode, mutators));
						int nospawn = 0;
						if(smode && !smode->canspawn(spinfo, false, true)) { nospawn++; }
						mutate(smuts, {
							if (!mut->canspawn(spinfo, false, true)) { nospawn++; }
						});
						if (!nospawn) sendspawn(spinfo);
	                    spinfo->state.lasttimeplayed = lastmillis;
						aiman::dorefresh = true;
					}
					break;
				}

				case SV_SETTEAM:
				{
					int who = getint(p), team = getint(p);
					if(who<0 || who>=getnumclients() || !haspriv(ci, PRIV_MASTER, true)) break;
					clientinfo *wi = (clientinfo *)getinfo(who);
					if(!wi || !m_team(gamemode, mutators) || !isteam(gamemode, mutators, team, TEAM_FIRST)) break;
					if(wi->team != team)
					{
						if(smode) smode->changeteam(wi, wi->team, team);
						mutate(smuts, mut->changeteam(wi, wi->team, team));
						wi->team = team;
						aiman::dorefresh = true;
					}
					sendf(sender, 1, "ri3", SV_SETTEAM, who, team);
					QUEUE_INT(SV_SETTEAM);
					QUEUE_INT(who);
					QUEUE_INT(team);
					break;
				}

				case SV_RECORDDEMO:
				{
					int val = getint(p);
					if(!haspriv(ci, PRIV_ADMIN, true)) break;
					demonextmatch = val!=0;
					srvoutf("demo recording is %s for next match", demonextmatch ? "enabled" : "disabled");
					break;
				}

				case SV_STOPDEMO:
				{
					if(!haspriv(ci, PRIV_ADMIN, true)) break;
					if(m_demo(gamemode)) enddemoplayback();
					else enddemorecord();
					break;
				}

				case SV_CLEARDEMOS:
				{
					int demo = getint(p);
					if(!haspriv(ci, PRIV_ADMIN, true)) break;
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
					int n = getint(p);
					loopk(3) getint(p);
					while(sents.length() <= n) sents.add();
					sents[n].type = getint(p);
					sents[n].attr[0] = getint(p);
					sents[n].attr[1] = getint(p);
					sents[n].attr[2] = getint(p);
					sents[n].attr[3] = getint(p);
					sents[n].attr[4] = getint(p);
					QUEUE_MSG;
					loopvk(clients)
					{
						clientinfo *cq = clients[k];
						cq->state.dropped.remove(n);
						loopj(WEAPON_MAX) if(cq->state.entid[j] == n)
							cq->state.entid[j] = -1;
					}
					if(enttype[sents[n].type].usetype == EU_ITEM || sents[n].type == TRIGGER)
					{
						sents[n].spawned = false; // wait a bit then load 'em up
						sents[n].millis = gamemillis;
						if(enttype[sents[n].type].usetype == EU_ITEM)
						{
							sents[n].millis -= GVAR(itemspawntime)*1000; // rewind
							if(GVAR(itemspawndelay)) sents[n].millis += GVAR(itemspawndelay)*1000;
							else sents[n].millis += GVAR(itemspawntime)*500; // half?
						}
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
							relayf(2, "\fm%s set worldvar %s to %d", colorname(ci), text, val);
							QUEUE_INT(val);
							break;
						}
						case ID_FVAR:
						{
							float val = getfloat(p);
							relayf(2, "\fm%s set worldvar %s to %f", colorname(ci), text, val);
							QUEUE_FLT(val);
							break;
						}
						case ID_SVAR:
						case ID_ALIAS:
						{
							string val;
							getstring(val, p);
							relayf(2, "\fm%s set world%s %s to %s", colorname(ci), t == ID_ALIAS ? "alias" : "var", text, val);
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
					if(!mapsending && mapdata[2])
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
								loopk(3) if(mapdata[k])
								{
									fclose(mapdata[k]);
									mapdata[k] = NULL;
								}
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
						notgotinfo = false;
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

				case SV_INITAI:
				{
					int lcn = getint(p);
					loopk(3) getint(p);
                    getstring(text, p);
                    getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
                    QUEUE_MSG;
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
			case 'o': if(atoi(&arg[3])) mastermask = (1<<MM_OPEN) | (1<<MM_VETO); return true;
			case 'M': setsvar("servermotd", &arg[3]); return true;
			default: break;
		}
		return false;
	}
};
#undef GAMESERVER
