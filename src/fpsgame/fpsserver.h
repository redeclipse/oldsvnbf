struct GAMESERVER : igameserver
{
	struct srventity
	{
		int type, attr1, attr2, attr3, attr4, attr5, spawntime;
		bool spawned;

		srventity() : type(NOTUSED),
			attr1(0), attr2(0), attr3(0), attr4(0), attr5(0),
			spawntime(0), spawned(false) {}
		~srventity() {}
	};

    static const int DEATHMILLIS = 300;

	enum { GE_NONE = 0, GE_SHOT, GE_SWITCH, GE_RELOAD, GE_EXPLODE, GE_HIT, GE_SUICIDE, GE_USE };

	struct shotevent
	{
		int type;
        int millis, id;
		int gun, power;
		float from[3], to[3];
	};

	struct switchevent
	{
		int type;
		int millis, id;
		int gun;
	};

	struct reloadevent
	{
		int type;
		int millis, id;
		int gun;
	};

	struct explodeevent
	{
		int type;
        int millis, id;
		int gun;
	};

	struct hitevent
	{
		int type;
		int flags;
		int target;
		int lifesequence;
		union
		{
			int rays;
			float dist;
		};
		float dir[3];
	};

	struct suicideevent
	{
		int type, flags;
	};

	struct useevent
	{
		int type;
		int millis, id;
		int ent, owner;
	};

	union gameevent
	{
		int type;
		shotevent shot;
		switchevent gunsel;
		reloadevent reload;
		explodeevent explode;
		hitevent hit;
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

        void push(int val)
        {
        	reset();
			add(val);
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
    };

	struct gamestate : fpsstate
	{
		vec o;
		int state;
        projectilestate gundrop, rockets, grenades, flames;
		int frags, deaths, teamkills, shotdamage, damage;
		int lasttimeplayed, timeplayed;
		float effectiveness;

		gamestate() : state(CS_DEAD) {}

		bool isalive(int gamemillis)
		{
			return state==CS_ALIVE || (state==CS_DEAD && gamemillis-lastdeath <= DEATHMILLIS);
		}

		void reset()
		{
			if(state!=CS_SPECTATOR) state = CS_DEAD;
			lifesequence = 0;
			gundrop.reset();
            rockets.reset();
            grenades.reset();
            flames.reset();

			timeplayed = 0;
			effectiveness = 0;
            frags = deaths = teamkills = shotdamage = damage = 0;

			respawn(-1);
		}

		void respawn(int millis)
		{
			fpsstate::respawn(millis);
			o = vec(-1e10f, -1e10f, -1e10f);
		}
	};

	struct savedscore
	{
		uint ip;
		string name;
		int frags, timeplayed, deaths, teamkills, shotdamage, damage;
		float effectiveness;

		void save(gamestate &gs)
		{
			frags = gs.frags;
            deaths = gs.deaths;
            teamkills = gs.teamkills;
            shotdamage = gs.shotdamage;
            damage = gs.damage;
			timeplayed = gs.timeplayed;
			effectiveness = gs.effectiveness;
		}

		void restore(gamestate &gs)
		{
			gs.frags = frags;
            gs.deaths = deaths;
            gs.teamkills = teamkills;
            gs.shotdamage = shotdamage;
            gs.damage = damage;
			gs.timeplayed = timeplayed;
			gs.effectiveness = effectiveness;
		}
	};

	struct clientinfo
	{
		int clientnum, team;
		string name, mapvote;
		int modevote, mutsvote;
		int privilege;
        bool spectator, timesync, wantsmaster;
        int gameoffset, lastevent;
		gamestate state;
		vector<gameevent> events;
		vector<uchar> position, messages;
        vector<clientinfo *> targets;

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
            lastevent = 0;
		}

		void reset()
		{
			team = TEAM_NEUTRAL;
			name[0] = 0;
			privilege = PRIV_NONE;
            spectator = wantsmaster = false;
			position.setsizenodelete(0);
			messages.setsizenodelete(0);
			mapchange();
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

	bool notgotitems, notgotflags;		// true when map has changed and waiting for clients to send item
	int gamemode, mutators;
	int gamemillis, gamelimit;

	string serverdesc;
	string smapname;
	int interm, minremain, oldtimelimit;
	bool maprequest;
	enet_uint32 lastsend;
	int mastermode, mastermask;
	int currentmaster;
	bool masterupdate;
	string masterpass;
	FILE *mapdata;

	vector<ban> bannedips;
	vector<clientinfo *> clients;
	vector<worldstate *> worldstates;
	bool reliablemessages;

	struct demofile
	{
		string info;
		uchar *data;
		int len;
	};

	#define MAXDEMOS 5
	vector<demofile> demos;

	bool demonextmatch;
	FILE *demotmp;
	gzFile demorecord, demoplayback;
	int nextplayback;

	struct servmode
	{
		GAMESERVER &sv;

		servmode(GAMESERVER &sv) : sv(sv) {}
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
		virtual bool damage(clientinfo *target, clientinfo *actor, int damage, int gun, int flags, const vec &hitpush = vec(0, 0, 0)) { return true; }
	};

	servmode *smode;
	vector<servmode *> smuts;

	#define STFSERV 1
	#include "stf.h"
	#undef STFSERV

    #define CTFSERV 1
    #include "ctf.h"
    #undef CTFSERV

	#include "duel.h"

	#define mutate(a,b) loopvk(a) { servmode *mut = a[k]; { b; } }

	#define BOTSERV 1
	#include "bot.h"
	#undef BOTSERV

	string  motd;
	GAMESERVER()
		: notgotitems(true), notgotflags(false),
			gamemode(defaultmode()), mutators(0),
			interm(0), minremain(10), oldtimelimit(10),
			maprequest(false), lastsend(0),
			mastermode(MM_VETO), mastermask(MM_DEFAULT), currentmaster(-1), masterupdate(false),
			mapdata(NULL), reliablemessages(false),
			demonextmatch(false), demotmp(NULL), demorecord(NULL), demoplayback(NULL), nextplayback(0),
			smode(NULL), stfmode(*this), ctfmode(*this), duelmutator(*this),
			bot(*this)
	{
		CCOMMAND(gameid, "", (GAMESERVER *self), result(self->gameid()));
		CCOMMAND(gamever, "", (GAMESERVER *self), intret(self->gamever()));
		CCOMMAND(gamename, "ii", (GAMESERVER *self, int *g, int *m), result(self->gamename(*g, *m)));
		CCOMMAND(defaultmap, "", (GAMESERVER *self), result(self->defaultmap()));
		CCOMMAND(defaultmode, "", (GAMESERVER *self), intret(self->defaultmode()));
		CCOMMAND(mutscheck, "ii", (GAMESERVER *self, int *g, int *m), intret(self->mutscheck(*g, *m)));

		motd[0] = serverdesc[0] = masterpass[0] = '\0';
		smuts.setsize(0);
		cleanup(true);
	}

	void *newinfo() { return new clientinfo; }
	void deleteinfo(void *ci) { delete (clientinfo *)ci; }

	const char *privname(int type)
	{
		switch(type)
		{
			case PRIV_ADMIN: return "admin";
			case PRIV_MASTER: return "master";
			default: return "unknown";
		}
	}

	bool haspriv(clientinfo *ci, int flag, bool msg = false)
	{
		if(flag <= PRIV_MASTER && !nonspectators(ci->clientnum, true)) return true;
		else if(ci->privilege >= flag) return true;
		else if(msg) srvoutf(ci->clientnum, "\fraccess denied, you need %s", privname(flag));
		return false;
	}

	void cleanup(bool init)
	{
		gamemode = defaultmode();
		mutators = 0;
		modecheck(&gamemode, &mutators);
		s_strcpy(smapname, defaultmap());
		resetitems();
		bannedips.setsize(0);
		if(!init)
		{
			enumerate(*idents, ident, id, {
				if(id.flags&IDF_GAME && !strncmp(id.name, "sv_", 3)) // reset vars
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
						case ID_ALIAS:
						{
							worldalias(id.name, "");
							break;
						}
						default: break;
					}
				}
			});
			execfile("server.cfg");
		}
	}

	vector<srventity> sents;
	vector<savedscore> scores;

	void srvoutf(int cn, const char *s, ...)
	{
		s_sprintfdlv(str, s, s);
		sendf(cn, 1, "ris", SV_SERVMSG, str);
	}

	void resetitems()
	{
		sents.setsize(0);
		//cps.reset();
	}

	int spawntime(int type)
	{
		return ((MAXCLIENTS+41)-nonspectators())*100; // so, range is 4100..30000 ms
	}

	void vote(char *map, int reqmode, int reqmuts, int sender)
	{
		clientinfo *ci = (clientinfo *)getinfo(sender);
        if(!ci || (ci->state.state==CS_SPECTATOR && !haspriv(ci, PRIV_MASTER, true)) || !m_game(reqmode)) return;

		s_strcpy(ci->mapvote, map);
		if(!ci->mapvote[0]) return;

		ci->modevote = reqmode; ci->mutsvote = reqmuts;
		modecheck(&ci->modevote, &ci->mutsvote);

		if(haspriv(ci, PRIV_MASTER) && mastermode >= MM_VETO)
		{
			if(demorecord) enddemorecord();
			srvoutf(-1, "%s forced %s on map %s", privname(ci->privilege), gamename(ci->modevote, ci->mutsvote), map);
			sendf(-1, 1, "ri2si2", SV_MAPCHANGE, 1, ci->mapvote, ci->modevote, ci->mutsvote);
			changemap(ci->mapvote, ci->modevote, ci->mutsvote);
		}
		else
		{
			srvoutf(-1, "%s suggests %s on map %s (select map to vote)", colorname(ci), gamename(ci->modevote, ci->mutsvote), map);
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

	int chooseworstteam(clientinfo *who)
	{
		teamscore teamscores[MAXTEAMS] = { teamscore(TEAM_ALPHA), teamscore(TEAM_BETA), teamscore(TEAM_DELTA), teamscore(TEAM_GAMMA) };
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(!ci->team) continue;
			ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
			ci->state.lasttimeplayed = lastmillis;

			loopj(numteams(gamemode, mutators)) if(ci->team == teamscores[j].team)
			{
				teamscore &ts = teamscores[j];
				float rank = ci->state.effectiveness/max(ci->state.timeplayed, 1);
				ts.rank += rank;
				ts.clients += !sv_botratio || ci->state.ownernum >= 0 ? 1 : sv_botratio+(nonspectators(-1, true) <= 1 ? 1 : 0);
				break;
			}
		}
		teamscore *worst = &teamscores[0];
		loopi(numteams(gamemode, mutators))
		{
			teamscore &ts = teamscores[i];
			if(who->state.ownernum >= 0 || m_stf(gamemode) || m_ctf(gamemode))
			{
				if(ts.clients < worst->clients || (ts.clients == worst->clients && ts.rank < worst->rank)) worst = &ts;
			}
			else if(ts.rank < worst->rank || (ts.rank == worst->rank && ts.clients < worst->clients)) worst = &ts;
		}
		return worst->team;
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
		srvoutf(-1, "demo \"%s\" recorded", d.info);
		d.data = new uchar[len];
		d.len = len;
		fread(d.data, 1, len, demotmp);
		fclose(demotmp);
		demotmp = NULL;
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

        srvoutf(-1, "recording demo");

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
        welcomepacket(p, -1, packet);
        writedemo(1, p.buf, p.len);
        enet_packet_destroy(packet);

		uchar buf[MAXTRANS];
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			uchar header[16];
			ucharbuf q(&buf[sizeof(header)], sizeof(buf)-sizeof(header));
			putint(q, SV_INITC2S);
			sendstring(ci->name, q);
			putint(q, ci->team);

			ucharbuf h(header, sizeof(header));
			putint(h, SV_CLIENT);
			putint(h, ci->clientnum);
			putuint(h, q.len);

			memcpy(&buf[sizeof(header)-h.len], header, h.len);

			writedemo(1, &buf[sizeof(header)-h.len], h.len+q.len);
		}
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
			srvoutf(-1, "cleared all demos");
		}
		else if(demos.inrange(n-1))
		{
			delete[] demos[n-1].data;
			demos.remove(n-1);
			srvoutf(-1, "cleared demo %d", n);
		}
	}

	void senddemo(int cn, int num)
	{
		if(!num) num = demos.length();
		if(!demos.inrange(num-1)) return;
		demofile &d = demos[num-1];
		sendf(cn, 2, "rim", SV_SENDDEMO, d.len, d.data);
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
			srvoutf(-1, "%s", msg);
			return;
		}

		srvoutf(-1, "playing demo \"%s\"", file);

		sendf(-1, 1, "rii", SV_DEMOPLAYBACK, 1);

		if(gzread(demoplayback, &nextplayback, sizeof(nextplayback))!=sizeof(nextplayback))
		{
			enddemoplayback();
			return;
		}
		endianswap(&nextplayback, sizeof(nextplayback), 1);
	}

	void enddemoplayback()
	{
		if(!demoplayback) return;
		gzclose(demoplayback);
		demoplayback = NULL;

		sendf(-1, 1, "rii", SV_DEMOPLAYBACK, 0);

		srvoutf(-1, "demo playback finished");

		loopv(clients)
		{
			ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
			ucharbuf p(packet->data, packet->dataLength);
            welcomepacket(p, clients[i]->clientnum, packet);
			enet_packet_resize(packet, p.length());
			sendpacket(clients[i]->clientnum, 1, packet);
			if(!packet->referenceCount) enet_packet_destroy(packet);
		}
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

	void changemap(const char *s, int mode, int muts)
	{
		if(m_demo(gamemode)) enddemoplayback();
		else enddemorecord();

		maprequest = false;
		gamemode = mode; mutators = muts;
		modecheck(&gamemode, &mutators);
		gamemillis = 0;
		oldtimelimit = sv_timelimit;
		minremain = sv_timelimit ? sv_timelimit : -1;
		gamelimit = sv_timelimit ? minremain*60000 : 0;
		interm = 0;
		s_strcpy(smapname, s);
		resetitems();
		notgotitems = true;
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

		// server modes
		if (m_stf(gamemode)) smode = &stfmode;
        else if(m_ctf(gamemode)) smode = &ctfmode;
		else smode = NULL;

		smuts.setsize(0);
		if (m_duel(gamemode, mutators)) smuts.add(&duelmutator);


		if(smode) smode->reset(false);
		mutate(smuts, mut->reset(false));

		loopv(clients)
		{
			clientinfo *ci = clients[i];

			int team = TEAM_NEUTRAL;
			if(m_team(gamemode, mutators)) team = chooseworstteam(ci);
			sendf(-1, 1, "ri3", SV_SETTEAM, ci->clientnum, team);

			if(smode) smode->changeteam(ci, ci->team, team);
			mutate(smuts, mut->changeteam(ci, ci->team, team));

			ci->team = team;

			ci->mapchange();
            ci->state.lasttimeplayed = lastmillis;
            if(ci->state.state != CS_SPECTATOR)
				sendspawn(ci);
		}

		if(m_timed(gamemode) && hasnonlocalclients())
			sendf(-1, 1, "ri2", SV_TIMEUP, minremain);

		if(m_demo(gamemode)) setupdemoplayback();
		else if(demonextmatch)
		{
			demonextmatch = false;
			setupdemorecord();
		}
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

	struct votecount
	{
		char *map;
		int mode, muts, count;
		votecount() {}
		votecount(char *s, int n, int m) : map(s), mode(n), muts(m), count(0) {}
	};

	void checkvotes(bool force = false)
	{
		vector<votecount> votes;
		int maxvotes = 0;
		loopv(clients)
		{
			clientinfo *oi = clients[i];
			if((oi->state.state==CS_SPECTATOR && !haspriv(oi, PRIV_MASTER, false)) || oi->state.ownernum >= 0)
				continue;
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

		int reqvotes = max(maxvotes / 2, force ? 1 : 2);
		if(force || (best && best->count >= reqvotes))
		{
			if(demorecord) enddemorecord();
			srvoutf(-1, "%s", force ? "vote passed by default" : "vote passed by majority");
			if(best && best->count >= reqvotes)
			{
				sendf(-1, 1, "ri2sii", SV_MAPCHANGE, 1, best->map, best->mode, best->muts);
				changemap(best->map, best->mode, best->muts);
			}
			else
			{
				sendf(-1, 1, "ri2sii", SV_MAPCHANGE, 1, smapname[0] ? smapname : defaultmap(), gamemode, mutators);
				changemap(smapname[0] ? smapname : defaultmap(), gamemode, mutators);
			}
		}
	}

	int nonspectators(int exclude = -1, bool nobots = false)
	{
		int n = 0;
		loopv(clients)
			if(clients[i]->clientnum != exclude && clients[i]->state.state != CS_SPECTATOR &&
				(!nobots || clients[i]->state.ownernum < 0))
				n++;
		return n;
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
		if(type >= SV_EDITENT && type <= SV_GETMAP && !m_edit(gamemode)) return -1;
		// server only messages
		static int servtypes[] = { SV_INITS2C, SV_NEWGAME, SV_MAPCHANGE, SV_SERVMSG, SV_DAMAGE, SV_SHOTFX, SV_DIED, SV_SPAWNSTATE, SV_FORCEDEATH, SV_ITEMACC, SV_ITEMSPAWN, SV_TIMEUP, SV_CDIS, SV_CURRENTMASTER, SV_PONG, SV_RESUME, SV_TEAMSCORE, SV_FLAGINFO, SV_ANNOUNCE, SV_SENDDEMOLIST, SV_SENDDEMO, SV_DEMOPLAYBACK, SV_SENDMAP, SV_REGEN, SV_SCOREFLAG, SV_RETURNFLAG, SV_CLIENT };
		if(ci) loopi(sizeof(servtypes)/sizeof(int)) if(type == servtypes[i]) return -1;
		return type;
	}

	static void freecallback(ENetPacket *packet)
	{
		extern igameserver *sv;
		((GAMESERVER *)sv)->cleanworldstate(packet);
	}

	void cleanworldstate(ENetPacket *packet)
	{
		loopv(worldstates)
		{
			worldstate *ws = worldstates[i];
			if(packet->data >= ws->positions.getbuf() && packet->data <= &ws->positions.last()) ws->uses--;
			else if(packet->data >= ws->messages.getbuf() && packet->data <= &ws->messages.last()) ws->uses--;
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
			if(psize && (pkt[i].posoff<0 || psize-ci.position.length()>0))
			{
				packet = enet_packet_create(&ws.positions[pkt[i].posoff<0 ? 0 : pkt[i].posoff+ci.position.length()],
											pkt[i].posoff<0 ? psize : psize-ci.position.length(),
											ENET_PACKET_FLAG_NO_ALLOCATE);
				sendpacket(ci.clientnum, 0, packet);
				if(!packet->referenceCount) enet_packet_destroy(packet);
				else { ++ws.uses; packet->freeCallback = freecallback; }
			}
			ci.position.setsizenodelete(0);

			if(msize && (pkt[i].msgoff<0 || msize-pkt[i].msglen>0))
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
		if(millis<(enet_uint32)sv_netframetime) return false;
		bool flush = buildworldstate();
		lastsend += millis - (millis%sv_netframetime);
		return flush;
	}

	void parsepacket(int sender, int chan, bool reliable, ucharbuf &p)	 // has to parse exactly each byte of the packet
	{
		if(sender<0) return;
		if(chan==2)
		{
			receivefile(sender, p.buf, p.maxlen);
			return;
		}
		if(reliable) reliablemessages = true;
		char text[MAXTRANS];
		int type = -1, prevtype = -1;
		clientinfo *ci = sender>=0 ? (clientinfo *)getinfo(sender) : NULL;
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
		#define seteventmillis(event, eventcond) \
		{ \
			if(!cp->timesync || (cp->events.length()==1 && eventcond)) \
			{ \
				cp->timesync = true; \
				cp->gameoffset = gamemillis - event.id; \
				event.millis = gamemillis; \
			} \
			else event.millis = cp->gameoffset + event.id; \
		}

		int curmsg;
        while((curmsg = p.length()) < p.maxlen)
		{
			prevtype = type;
			switch(type = checktype(getint(p), ci))
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
					loopi(8) getint(p);
					int physstate = getuint(p);
					if(physstate&0x20) loopi(2) getint(p);
					if(physstate&0x10) getint(p);
                    getuint(p);
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

				case SV_EDITMODE:
				{
					int val = getint(p);
					if(ci->state.state!=(val ? CS_ALIVE : CS_EDITING) || (gamemode!=1)) break;
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
						ci->state.gundrop.reset();
						ci->state.rockets.reset();
						ci->state.grenades.reset();
						ci->state.flames.reset();
					}
					QUEUE_MSG;
					break;
				}

				case SV_TRYSPAWN:
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					int nospawn = 0;
					if(smode && !smode->canspawn(cp, false, true)) { nospawn++; }
					mutate(smuts, if (!mut->canspawn(cp, false, true)) { nospawn++; });
					if(cp->state.state!=CS_DEAD || cp->state.lastrespawn>=0 || nospawn)
						break;
					if(cp->state.lastdeath) cp->state.respawn(gamemillis);
					sendspawn(cp);
					break;
				}

				case SV_GUNSELECT:
				{
					int lcn = getint(p), id = getint(p), gun = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					gameevent &gunsel = cp->addevent();
					gunsel.type = GE_SWITCH;
					gunsel.gunsel.id = id;
					gunsel.gunsel.gun = gun;
					seteventmillis(gunsel.gunsel, cp->state.canswitch(gunsel.gunsel.gun, gamemillis));
					break;
				}

				case SV_SPAWN:
				{
					int lcn = getint(p), ls = getint(p), gunselect = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					if((cp->state.state!=CS_ALIVE && cp->state.state!=CS_DEAD) || ls!=cp->state.lifesequence || cp->state.lastrespawn<0)
						break;
					cp->state.lastrespawn = -1;
					cp->state.state = CS_ALIVE;
					cp->state.gunselect = gunselect;
					if(smode) smode->spawned(cp);
					mutate(smuts, mut->spawned(cp););
					QUEUE_BUF(100,
					{
						putint(buf, SV_SPAWN);
						sendstate(cp, buf);
					});
					break;
				}

				case SV_SUICIDE:
				{
					int lcn = getint(p), flags = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					gameevent &suicide = cp->addevent();
					suicide.type = GE_SUICIDE;
					suicide.suicide.flags = flags;
					break;
				}

				case SV_SHOOT:
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
					gameevent &shot = havecn ? cp->addevent() : dummyevent;
					shot.type = GE_SHOT;
					shot.shot.id = getint(p);
					shot.shot.gun = getint(p);
					shot.shot.power = getint(p);
					if(havecn) seteventmillis(shot.shot, cp->state.canshoot(shot.shot.gun, gamemillis));
					loopk(3) shot.shot.from[k] = getint(p)/DMF;
					loopk(3) shot.shot.to[k] = getint(p)/DMF;
					int hits = getint(p);
					loopk(hits)
					{
						gameevent &hit = havecn ? cp->addevent() : dummyevent;
						hit.type = GE_HIT;
						hit.hit.flags = getint(p);
						hit.hit.target = getint(p);
						hit.hit.lifesequence = getint(p);
						hit.hit.rays = getint(p);
						loopk(3) hit.hit.dir[k] = getint(p)/DNF;
					}
					break;
				}

				case SV_RELOAD:
				{
					int lcn = getint(p), id = getint(p), gun = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
					gameevent &reload = havecn ? cp->addevent() : dummyevent;
					reload.type = GE_RELOAD;
					reload.reload.id = id;
					reload.reload.gun = gun;
					if(havecn) seteventmillis(reload.reload, cp->state.canreload(reload.reload.gun, gamemillis));
					break;
				}

				case SV_EXPLODE:
				{
					int lcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					bool havecn = (cp && (cp->clientnum == ci->clientnum || cp->state.ownernum == ci->clientnum));
					gameevent &exp = havecn ? cp->addevent() : dummyevent;
					exp.type = GE_EXPLODE;
					exp.explode.id = getint(p);
					if(havecn) seteventmillis(exp.explode, true);
					exp.explode.gun = getint(p);
					exp.explode.id = getint(p);
					int hits = getint(p);
					loopk(hits)
					{
						gameevent &hit = havecn ? cp->addevent() : dummyevent;
						hit.type = GE_HIT;
						hit.hit.flags = getint(p);
						hit.hit.target = getint(p);
						hit.hit.lifesequence = getint(p);
						hit.hit.dist = getint(p)/DMF;
						loopk(3) hit.hit.dir[k] = getint(p)/DNF;
					}
					break;
				}

				case SV_ITEMUSE:
				{
					int lcn = getint(p), id = getint(p), ent = getint(p), tcn = getint(p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					gameevent &use = cp->addevent();
					use.type = GE_USE;
					use.use.id = id;
					use.use.ent = ent;
					use.use.owner = tcn;
					seteventmillis(use.use, use.use.ent >= 0 ? (sents.inrange(ent) ? cp->state.canuse(sents[ent].type, sents[ent].attr1, sents[ent].attr2, gamemillis) : false) : true);
					break;
				}

				case SV_TEXT:
				{
					int lcn = getint(p), flags = getint(p);
					getstring(text, p);
					clientinfo *cp = (clientinfo *)getinfo(lcn);
					if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) break;
					if(cp->state.state == CS_SPECTATOR || (flags&SAY_TEAM && !cp->team)) break;
					loopv(clients)
					{
						clientinfo *t = clients[i];
						if(t == cp || t->state.state == CS_SPECTATOR || (flags&SAY_TEAM && cp->team != t->team)) continue;
						sendf(t->clientnum, 1, "ri3s", SV_TEXT, cp->clientnum, flags, text);
					}
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
					QUEUE_MSG;
					bool connected = !ci->name[0];

					getstring(text, p);
					if(!text[0]) s_strcpy(text, "unnamed");
					QUEUE_STR(text);
					s_strncpy(ci->name, text, MAXNAMELEN+1);

					if(connected)
					{
						savedscore &sc = findscore(ci, false);
						if(&sc)
						{
							sc.restore(ci->state);
							gamestate &gs = ci->state;
							sendf(-1, 1, "ri7vi", SV_RESUME, sender,
								gs.state, gs.frags,
								gs.lifesequence, gs.health,
								gs.gunselect, GUN_MAX, &gs.ammo[0], -1);
						}
					}

					int team = getint(p);
					bool badteam = m_team(gamemode, mutators) ?
						(team < TEAM_ALPHA || team > numteams(gamemode, mutators)) :
							team != TEAM_NEUTRAL;
					if(connected || badteam)
					{
						if(m_team(gamemode, mutators)) team = chooseworstteam(ci);
						else team = TEAM_NEUTRAL;
						sendf(sender, 1, "ri3", SV_SETTEAM, sender, team);
					}
					QUEUE_INT(team);

					if(team != ci->team)
					{
						if(smode) smode->changeteam(ci, ci->team, team);
						mutate(smuts, mut->changeteam(ci, ci->team, team));
						connected = true;
					}
					ci->team = team;
					if(connected) bot.checkbots(true);

					QUEUE_MSG;
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

				case SV_ITEMLIST:
				{
					bool commit = notgotitems;
					int n;
					while((n = getint(p))!=-1)
					{
						srventity se, sn;
						se.type = getint(p);
						se.attr1 = getint(p);
						se.attr2 = getint(p);
						se.attr3 = getint(p);
						se.attr4 = getint(p);
						se.attr5 = getint(p);
						se.spawned = false;

						if(commit && (enttype[se.type].usetype == EU_ITEM || se.type == TRIGGER))
						{
							while(sents.length() < n) sents.add(sn);
							sents.add(se);
							sents[n].spawned = m_noitems(gamemode, mutators) ? false : true;
						}
					}
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
					getint(p);
					QUEUE_MSG;
					break;

				case SV_MASTERMODE:
				{
					int mm = getint(p);
					if(haspriv(ci, PRIV_MASTER, true) && mm >= MM_OPEN && mm <= MM_PRIVATE)
					{
						if(haspriv(ci, PRIV_ADMIN) || (mastermask&(1<<mm)))
						{
							mastermode = mm;
							srvoutf(-1, "mastermode is now %d", mastermode);
						}
						else
						{
							srvoutf(sender, "mastermode %d is disabled on this server", mm);
						}
					}
					break;
				}

				case SV_CLEARBANS:
				{
					if(haspriv(ci, PRIV_MASTER, true))
					{
						bannedips.setsize(0);
						srvoutf(-1, "cleared all bans");
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
						disconnect_client(victim, DISC_KICK);
					}
					break;
				}

				case SV_SPECTATOR:
				{
					int spectator = getint(p), val = getint(p);
					if((ci->state.state==CS_SPECTATOR || spectator!=sender) && !haspriv(ci, PRIV_MASTER, true)) break;
					clientinfo *spinfo = (clientinfo *)getinfo(spectator);
					if(!spinfo) break;

					sendf(-1, 1, "ri3", SV_SPECTATOR, spectator, val);

					if(spinfo->state.state!=CS_SPECTATOR && val)
					{
						if(smode) smode->leavegame(spinfo);
						mutate(smuts, mut->leavegame(spinfo));
						spinfo->state.state = CS_SPECTATOR;
                    	spinfo->state.timeplayed += lastmillis - spinfo->state.lasttimeplayed;
						bot.removebots(spinfo);
						bot.checkbots(true);
					}
					else if(spinfo->state.state==CS_SPECTATOR && !val)
					{
						spinfo->state.state = CS_DEAD;
						spinfo->state.respawn(gamemillis);
						int nospawn = 0;
						if (smode && !smode->canspawn(spinfo)) { nospawn++; }
						mutate(smuts, {
							if (!mut->canspawn(spinfo)) { nospawn++; }
						});
						if (!nospawn) sendspawn(spinfo);
	                    spinfo->state.lasttimeplayed = lastmillis;
						bot.checkbots(true);
					}
					break;
				}

				case SV_SETTEAM:
				{
					int who = getint(p), team = getint(p);
					if(who<0 || who>=getnumclients() || !haspriv(ci, PRIV_MASTER, true)) break;
					clientinfo *wi = (clientinfo *)getinfo(who);
					if(!wi || !m_team(gamemode, mutators) || team < TEAM_ALPHA || team > numteams(gamemode, mutators)) break;
					bool teamed = false;
					if(wi->team != team)
					{
						if (smode) smode->changeteam(wi, wi->team, team);
						mutate(smuts, mut->changeteam(wi, wi->team, team));
						teamed = true;
					}
					wi->team = team;
					sendf(sender, 1, "ri3", SV_SETTEAM, who, team);
					if(teamed) bot.checkbots(true);
					QUEUE_INT(SV_SETTEAM);
					QUEUE_INT(who);
					QUEUE_INT(team);
					break;
				}

				case SV_FORCEINTERMISSION:
					if(m_mission(gamemode)) startintermission();
					break;

				case SV_RECORDDEMO:
				{
					int val = getint(p);
					if(!haspriv(ci, PRIV_ADMIN, true)) break;
					demonextmatch = val!=0;
					srvoutf(-1, "demo recording is %s for next match", demonextmatch ? "enabled" : "disabled");
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
							QUEUE_INT(getint(p));
							break;
						}
						case ID_FVAR:
						{
							QUEUE_FLT(getfloat(p));
							break;
						}
						case ID_SVAR:
						case ID_ALIAS:
						{
							getstring(text, p);
							QUEUE_STR(text);
							break;
						}
						default: break;
					}
					break;
				}

				case SV_GETMAP:
					if(mapdata) sendfile(sender, 2, mapdata, "ri", SV_SENDMAP);
					else sendf(sender, 1, "ris", SV_SERVMSG, "no map to send");
					break;

				case SV_NEWMAP:
				{
					int size = getint(p);
					if(ci->state.state==CS_SPECTATOR) break;
					if(size>=0)
					{
						smapname[0] = '\0';
						resetitems();
						notgotitems = false;
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
					setmaster(ci, val!=0, text);
					// don't broadcast the master password
					break;
				}

				case SV_APPROVEMASTER:
				{
					int mn = getint(p);
					if(mastermask&MM_AUTOAPPROVE || ci->state.state==CS_SPECTATOR) break;
					clientinfo *candidate = (clientinfo *)getinfo(mn);
					if(!candidate || !candidate->wantsmaster || mn==sender || getclientip(mn)==getclientip(sender)) break;
					setmaster(candidate, true, "", true);
					break;
				}

				case SV_ADDBOT:
				{
					bot.reqadd(ci, getint(p));
					break;
				}

				case SV_DELBOT:
				{
					bot.reqdel(ci);
					break;
				}

				case SV_INITBOT:
				{
					QUEUE_MSG;
					QUEUE_INT(getint(p));
					QUEUE_INT(getint(p));
					QUEUE_INT(getint(p));
					getstring(text, p);
					QUEUE_STR(text);
					QUEUE_INT(getint(p));
					break;
				}

				default:
				{
					int size = msgsizelookup(type);
					if(size==-1) { disconnect_client(sender, DISC_TAGT); return; }
					if(size>0) loopi(size-1) getint(p);
					if(ci && ci->state.state!=CS_SPECTATOR) QUEUE_MSG;
					break;
				}
			}
			//conoutf("\fy[server] msg: %d, prev: %d", type, prevtype);
		}
	}

	void parsecommand(clientinfo *ci, int nargs, char *cmd, char *arg)
	{
		s_sprintfd(cmdname)("sv_%s", cmd);
		ident *id = idents->access(cmdname);
		if(id && id->flags&IDF_GAME)
		{
			if(haspriv(ci, PRIV_MASTER, true))
			{
				string val;
				val[0] = 0;
				switch(id->type)
				{
					case ID_COMMAND:
					{
						string s;
						if(nargs <= 1 || !arg) s_strcpy(s, cmd);
						else s_sprintf(s)("sv_%s %s", cmd, arg);
						char *ret = executeret(s);
						if(ret)
						{
							srvoutf(-1, "\fy%s: %s returned %s", colorname(ci), cmd, ret);
							delete[] ret;
						}
						return;
					}
					case ID_VAR:
					{
						if(nargs <= 1 || !arg)
						{
							srvoutf(ci->clientnum, "\fy%s = %d", cmd, *id->storage.i);
							return;
						}
						if(id->maxval < id->minval)
						{
							srvoutf(ci->clientnum, "\frcannot override variable: %s", cmd);
							return;
						}
						int ret = atoi(arg);
						if(ret < id->minval || ret > id->maxval)
						{
							srvoutf(ci->clientnum, "\frvalid range for %s is %d..%d", cmd, id->minval, id->maxval);
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
							srvoutf(ci->clientnum, "\fy%s = %f", cmd, *id->storage.f);
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
							srvoutf(ci->clientnum, strchr(*id->storage.s, '"') ? "%s = [%s]" : "%s = \"%s\"", cmd, *id->storage.s);
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
			}
		}
		else srvoutf(ci->clientnum, "\frunknown command: %s", cmd);
	}

	int welcomepacket(ucharbuf &p, int n, ENetPacket *packet)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		putint(p, SV_INITS2C);
		putint(p, n);
		putint(p, GAMEVERSION);
		putint(p, SV_MAPCHANGE);
		if(!smapname[0])
		{
			putint(p, 0);
			if(m_edit(gamemode) && nonspectators(ci->clientnum, true))
			{
				clientinfo *best = NULL;
				loopv(clients) if(clients[i]->name[0] && clients[i]->state.ownernum < 0 && clients[i]->state.state != CS_SPECTATOR)
				{
					clientinfo *cs = clients[i];
					if(haspriv(cs, PRIV_MASTER)) { best = cs; break; }
					if(!best || cs->state.timeplayed > best->state.timeplayed)
						best = cs;
				}
				if(best) sendf(best->clientnum, 1, "ri", SV_GETMAP);
			}
		}
		else
		{
			putint(p, 1);
			sendstring(smapname, p);
		}
		putint(p, gamemode);
		putint(p, mutators);
		if(!ci || (m_timed(gamemode) && hasnonlocalclients()))
		{
			putint(p, SV_TIMEUP);
			putint(p, minremain);
		}
		putint(p, SV_ITEMLIST);
		loopv(sents) if(sents[i].spawned)
		{
			putint(p, i);
			putint(p, sents[i].type);
			putint(p, sents[i].attr1);
			putint(p, sents[i].attr2);
			putint(p, sents[i].attr3);
			putint(p, sents[i].attr4);
			putint(p, sents[i].attr5);
		}
		putint(p, -1);

		enumerate(*idents, ident, id, {
			if(id.flags&IDF_GAME && !strncmp(id.name, "sv_", 3)) // reset vars
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

		if(ci && ci->state.state!=CS_SPECTATOR)
		{
			int nospawn = 0;
			if(smode && !smode->canspawn(ci, true)) { nospawn++; }
			mutate(smuts, if(!mut->canspawn(ci, true)) { nospawn++; });
			if(nospawn)
			{
				ci->state.state = CS_DEAD;
				putint(p, SV_FORCEDEATH);
				putint(p, n);
				sendf(-1, 1, "ri2x", SV_FORCEDEATH, n, n);
			}
			else
			{
				spawnstate(ci);
				putint(p, SV_SPAWNSTATE);
				sendstate(ci, p);
				ci->state.lastrespawn = ci->state.lastspawn = gamemillis;
			}
		}
		if(ci && ci->state.state==CS_SPECTATOR)
		{
			putint(p, SV_SPECTATOR);
			putint(p, n);
			putint(p, 1);
			sendf(-1, 1, "ri3x", SV_SPECTATOR, n, 1, n);
		}

		if(clients.length() > 1)
		{
			putint(p, SV_RESUME);
			loopv(clients)
			{
				clientinfo *oi = clients[i];
				if(oi->clientnum == n) continue;
				sendstate(oi, p);
			}
			putint(p, -1);
		}

		if(motd[0])
		{
			putint(p, SV_SERVMSG);
			sendstring(motd, p);
		}

		enet_packet_resize(packet, packet->dataLength + MAXTRANS);
		p.buf = packet->data;

		if(smode) smode->initclient(ci, p, true);
		mutate(smuts, mut->initclient(ci, p, true));

		return 1;
	}

	void checkintermission()
	{
		if(clients.length())
		{
			if(minremain)
			{
				if(sv_timelimit != oldtimelimit)
				{
					if(sv_timelimit) gamelimit += (sv_timelimit-oldtimelimit)*60000;
					oldtimelimit = sv_timelimit;
				}
				if(sv_timelimit)
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
				interm = gamemillis+10000;
			}
		}
	}

	void startintermission()
	{
		minremain = 0;
		gamelimit = min(gamelimit, gamemillis);
		sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
		checkintermission();
	}

	void clearevent(clientinfo *ci)
	{
		int n = 1;
		while(n<ci->events.length() && ci->events[n].type==GE_HIT) n++;
		ci->events.remove(0, n);
	}

	void spawnstate(clientinfo *ci)
	{
		gamestate &gs = ci->state;
		gs.spawnstate(m_insta(gamemode, mutators) ? sv_instaspawngun : sv_spawngun);
		gs.lifesequence++;
	}

	void sendspawn(clientinfo *ci)
	{
		gamestate &gs = ci->state;
		spawnstate(ci);
		int own = ci->state.ownernum >= 0 ? ci->state.ownernum : ci->clientnum;
		sendf(own, 1, "ri7v", SV_SPAWNSTATE, ci->clientnum, gs.state, gs.frags, gs.lifesequence, gs.health, gs.gunselect, GUN_MAX, &gs.ammo[0]);
		gs.lastrespawn = gs.lastspawn = gamemillis;
	}

    void sendstate(clientinfo *ci, ucharbuf &p)
    {
		gamestate &gs = ci->state;
        putint(p, ci->clientnum);
        putint(p, gs.state);
        putint(p, gs.frags);
        putint(p, gs.lifesequence);
        putint(p, gs.health);
        putint(p, gs.gunselect);
        loopi(GUN_MAX) putint(p, gs.ammo[i]);
    }

	void dodamage(clientinfo *target, clientinfo *actor, int damage, int gun, int flags, const vec &hitpush = vec(0, 0, 0))
	{
		gamestate &ts = target->state;
		if(gamemillis-ts.lastspawn <= REGENWAIT) return;
		int setdamage = damage, realdamage = damage;

		if(flags&HIT_LEGS) setdamage = damage/4;
		else if (flags&HIT_TORSO) setdamage = damage/2;
		if(smode && !smode->damage(target, actor, setdamage, gun, flags, hitpush)) { return; }
		mutate(smuts, if(!mut->damage(target, actor, setdamage, gun, flags, hitpush)) { return; });

		if(!m_fight(gamemode) || (!sv_teamdamage && m_team(gamemode, mutators) && actor->team == target->team))
			realdamage = 0;
		else if(m_insta(gamemode, mutators)) realdamage = max(setdamage, ts.health);
		else realdamage = setdamage;

		ts.dodamage(realdamage, gamemillis);
        actor->state.damage += realdamage;
		sendf(-1, 1, "ri7i3", SV_DAMAGE, target->clientnum, actor->clientnum, gun, flags, setdamage, ts.health, int(hitpush.x*DNF), int(hitpush.y*DNF), int(hitpush.z*DNF));

		if(realdamage && ts.health <= 0)
		{
            target->state.deaths++;
            if(actor!=target && m_team(gamemode, mutators) && actor->team == target->team) actor->state.teamkills++;
            int fragvalue = smode ? smode->fragvalue(target, actor) : (target == actor || (m_team(gamemode, mutators) && target->team == actor->team) ? -1 : 1);
            actor->state.frags += fragvalue;
            if(fragvalue > 0)
			{
				int friends = 0, enemies = 0; // note: friends also includes the fragger
				if(m_team(gamemode, mutators)) loopv(clients) if(clients[i]->team != actor->team) enemies++; else friends++;
				else { friends = 1; enemies = clients.length()-1; }
                actor->state.effectiveness += fragvalue*friends/float(max(enemies, 1));
			}
			sendf(-1, 1, "ri7", SV_DIED, target->clientnum, actor->clientnum, actor->state.frags, gun, flags, realdamage);
			if(isgun(ts.gunselect) && !m_noitems(gamemode, mutators))
				ts.gundrop.push(ts.gunselect);
            target->position.setsizenodelete(0);
			if(smode) smode->died(target, actor);
			mutate(smuts, mut->died(target, actor));
			ts.state = CS_DEAD;
			ts.lastdeath = gamemillis;
			// don't issue respawn yet until DEATHMILLIS has elapsed
			if(fragvalue > 0)
			{
				actor->state.spree++;
				switch(actor->state.spree)
				{
					case 5:
					{
						s_sprintfd(spr)("%s is wreaking carnage!", colorname(actor));
						sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_SPREE1, spr);
						break;
					}
					case 10:
					{
						s_sprintfd(spr)("%s is slaughtering the opposition!", colorname(actor));
						sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_SPREE2, spr);
						break;
					}
					case 25:
					{
						s_sprintfd(spr)("%s is on a massacre!", colorname(actor));
						sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_SPREE3, spr);
						break;
					}
					case 50:
					{
						s_sprintfd(spr)("%s is creating a bloodbath!", colorname(actor));
						sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_SPREE4, spr);
						break;
					}
					default:
					{
						if(flags&HIT_HEAD)
						{
							s_sprintfd(spr)("%s scored a headshot!", colorname(actor));
							sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_HEADSHOT, spr);
						}
						break;
					}
				}
				//if(sv_fraglimit && actor->state.frags >= sv_fraglimit)
				//	startintermission();
			}
		}
	}

	void processevent(clientinfo *ci, suicideevent &e)
	{
		gamestate &gs = ci->state;
		if(gs.state != CS_ALIVE)
		{
			//sendf(-1, 1, "ri3", SV_SOUND, ci->clientnum, S_DENIED);
			return;
		}
        ci->state.frags += smode ? smode->fragvalue(ci, ci) : -1;
        ci->state.deaths++;
		sendf(-1, 1, "ri7", SV_DIED, ci->clientnum, ci->clientnum, gs.frags, -1, e.flags, ci->state.health);
		if(isgun(gs.gunselect) && !m_noitems(gamemode, mutators))
			gs.gundrop.push(gs.gunselect);
        ci->position.setsizenodelete(0);
		if(smode) smode->died(ci, NULL);
		mutate(smuts, mut->died(ci, NULL));
		gs.state = CS_DEAD;
		gs.respawn(gamemillis);
	}

	void processevent(clientinfo *ci, explodeevent &e)
	{
		gamestate &gs = ci->state;
		switch(e.gun)
		{
#if 0
			case GUN_RL:
                if(!gs.rockets.remove(e.id)) return;
				break;
#endif
			case GUN_GL:
                if(!gs.grenades.remove(e.id)) return;
				break;

			case GUN_FLAMER:
                if(!gs.flames.remove(e.id)) return;
				break;

			default:
				return;
		}
		for(int i = 1; i<ci->events.length() && ci->events[i].type==GE_HIT; i++)
		{
			hitevent &h = ci->events[i].hit;
			clientinfo *target = (clientinfo *)getinfo(h.target);
            if(!target || target->state.state!=CS_ALIVE || h.lifesequence!=target->state.lifesequence || h.dist<0 || h.dist>guntype[e.gun].explode) continue;

			int j = 1;
			for(j = 1; j<i; j++) if(ci->events[j].hit.target==h.target) break;
			if(j<i) continue;

			int damage = int(guntype[e.gun].damage*(1-h.dist/guntype[e.gun].scale/guntype[e.gun].explode));
			dodamage(target, ci, damage, e.gun, h.flags, h.dir);
		}
	}

	void processevent(clientinfo *ci, shotevent &e)
	{
		gamestate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !gs.canshoot(e.gun, e.millis) || !isgun(e.gun))
		{
			//sendf(-1, 1, "ri3", SV_SOUND, ci->clientnum, S_DENIED);
			if(guntype[e.gun].max && isgun(e.gun))
				gs.ammo[e.gun] = max(gs.ammo[e.gun]-1, 0); // keep synched!
			return;
		}
		if(guntype[e.gun].max)
			gs.ammo[e.gun] = max(gs.ammo[e.gun]-1, 0); // keep synched!
		gs.setgunstate(e.gun, GUNSTATE_SHOOT, guntype[e.gun].adelay, e.millis);
		sendf(-1, 1, "ri4i6x", SV_SHOTFX, ci->clientnum, e.gun, e.power,
				int(e.from[0]*DMF), int(e.from[1]*DMF), int(e.from[2]*DMF),
				int(e.to[0]*DMF), int(e.to[1]*DMF), int(e.to[2]*DMF),
				ci->clientnum);
		gs.shotdamage += guntype[e.gun].damage*(e.gun==GUN_SG ? SGRAYS : 1);
		switch(e.gun)
		{
#if 0
            case GUN_RL: gs.rockets.add(e.id); break;
#endif
            case GUN_GL: gs.grenades.add(e.id); break;
            case GUN_FLAMER: gs.flames.add(e.id); break;
			default:
			{
				int totalrays = 0, maxrays = e.gun==GUN_SG ? SGRAYS : 1;
				for(int i = 1; i<ci->events.length() && ci->events[i].type==GE_HIT; i++)
				{
					hitevent &h = ci->events[i].hit;
					clientinfo *target = (clientinfo *)getinfo(h.target);
                    if(!target || target->state.state!=CS_ALIVE || h.lifesequence!=target->state.lifesequence || h.rays<1) continue;

					totalrays += h.rays;
					if(totalrays>maxrays) continue;
					int damage = h.rays*guntype[e.gun].damage;
					dodamage(target, ci, damage, e.gun, h.flags, h.dir);
				}
				break;
			}
		}
	}

	void processevent(clientinfo *ci, switchevent &e)
	{
		gamestate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !gs.canswitch(e.gun, e.millis))
		{
			//sendf(-1, 1, "ri3", SV_SOUND, ci->clientnum, S_DENIED);
			return;
		}
		gs.gunswitch(e.gun, e.millis);
		sendf(-1, 1, "ri3", SV_GUNSELECT, ci->clientnum, e.gun);
	}

	void processevent(clientinfo *ci, reloadevent &e)
	{
		gamestate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !gs.canreload(e.gun, e.millis))
		{
			//sendf(-1, 1, "ri3", SV_SOUND, ci->clientnum, S_DENIED);
			return;
		}
		int dropped = gs.useitem(e.millis, true, WEAPON, e.gun, guntype[e.gun].add);
		sendf(-1, 1, "ri4", SV_RELOAD, ci->clientnum, e.gun, gs.ammo[e.gun]);
		if(isgun(dropped) && !m_noitems(gamemode, mutators))
			gs.gundrop.push(dropped);
	}

	void processevent(clientinfo *ci, useevent &e)
	{
		gamestate &gs = ci->state;
		if(!minremain || !gs.isalive(gamemillis) || m_noitems(gamemode, mutators))
		{
			//sendf(-1, 1, "ri3", SV_SOUND, ci->clientnum, S_DENIED);
			return;
		}
		int dropped = -1;
		if(e.ent >= 0)
		{
			if(!sents.inrange(e.ent) || !sents[e.ent].spawned || !gs.canuse(sents[e.ent].type, sents[e.ent].attr1, sents[e.ent].attr2, e.millis))
				return;
			sents[e.ent].spawned = false;
			sents[e.ent].spawntime = spawntime(sents[e.ent].type);
			dropped = gs.useitem(e.millis, false, sents[e.ent].type, sents[e.ent].attr1, sents[e.ent].attr2);
			sendf(-1, 1, "ri4", SV_ITEMACC, ci->clientnum, e.ent, -1);
		}
		else if(e.owner >= 0)
		{
			clientinfo *cp = (clientinfo *)getinfo(e.owner);
			if(!cp || cp->state.gundrop.projs.empty()) return;
			int gun = cp->state.gundrop.projs[0];
			if(!isgun(gun) || !gs.canuse(WEAPON, gun, guntype[gun].add, e.millis) || m_noitems(gamemode, mutators))
				return;
			dropped = gs.useitem(e.millis, false, WEAPON, gun, guntype[gun].add);
			sendf(-1, 1, "ri4", SV_ITEMACC, ci->clientnum, -1, e.owner);
			cp->state.gundrop.reset();
		}
		if(isgun(dropped) && !m_noitems(gamemode, mutators))
		{
			gs.gundrop.reset();
			gs.gundrop.add(dropped);
		}
	}

	void processevents()
	{
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if (ci->state.state == CS_ALIVE)
			{
				int lastpain = gamemillis-ci->state.lastpain,
					lastregen = gamemillis-ci->state.lastregen;

				if (!m_insta(gamemode, mutators) && ci->state.health < MAXHEALTH && lastpain >= REGENWAIT && lastregen >= REGENTIME)
				{
					int health = ci->state.health - (ci->state.health % REGENHEAL);
					ci->state.health = min(health + REGENHEAL, MAXHEALTH);
					ci->state.lastregen = gamemillis;
					sendf(-1, 1, "ri3", SV_REGEN, ci->clientnum, ci->state.health);
				}
			}

			while (ci->events.length())
			{
				gameevent &e = ci->events[0];
                if(e.type<GE_SUICIDE)
                {
                    if(e.shot.millis>gamemillis) break;
                    if(e.shot.millis<ci->lastevent) { clearevent(ci); continue; }
                    ci->lastevent = e.shot.millis;
                }
				switch(e.type)
				{
					case GE_SHOT: processevent(ci, e.shot); break;
					case GE_EXPLODE: processevent(ci, e.explode); break;
					case GE_SWITCH: processevent(ci, e.gunsel); break;
					case GE_RELOAD: processevent(ci, e.reload); break;
					// untimed events
					case GE_SUICIDE: processevent(ci, e.suicide); break;
					case GE_USE: processevent(ci, e.use); break;
				}
				clearevent(ci);
			}
		}
	}

	void serverupdate()
	{
		gamemillis += curtime;

		if(m_demo(gamemode)) readdemo();
		else if(minremain)
		{
			processevents();
			if(curtime && !m_noitems(gamemode, mutators))
            {
                loopv(sents) if(sents[i].spawntime && sents[i].type == WEAPON)
			    {
				    sents[i].spawntime -= curtime;
				    if(sents[i].spawntime<=0)
				    {
					    sents[i].spawntime = 0;
					    sents[i].spawned = true;
					    sendf(-1, 1, "ri2", SV_ITEMSPAWN, i);
				    }
                }
			}
			if(smode) smode->update();
			mutate(smuts, mut->update());
			bot.checkbots();
		}

		while(bannedips.length() && bannedips[0].time-totalmillis>4*60*60000)
			bannedips.remove(0);

		if(masterupdate)
		{
			clientinfo *m = currentmaster>=0 ? (clientinfo *)getinfo(currentmaster) : NULL;
			sendf(-1, 1, "ri3", SV_CURRENTMASTER, currentmaster, m ? m->privilege : 0);
			masterupdate = false;
		}

		if((m_timed(gamemode) && hasnonlocalclients()) &&
			((sv_timelimit != oldtimelimit) ||
			(gamemillis-curtime>0 && gamemillis/60000!=(gamemillis-curtime)/60000)))
				checkintermission();

		if(interm && gamemillis >= interm) // wait then call for next map
		{
			if(!maprequest)
			{
				if(demorecord) enddemorecord();
				sendf(-1, 1, "ri", SV_NEWGAME);
				maprequest = true;
				interm = gamemillis+20000;
			}
			else
			{
				interm = 0;
				checkvotes(true);
			}
		}
	}

	bool serveroption(char *arg)
	{
		if(arg[0]=='-' && arg[1]=='s') switch(arg[2])
		{
			case 'd': s_strcpy(serverdesc, &arg[3]); return true;
			case 'p': s_strcpy(masterpass, &arg[3]); return true;
			case 'o': if(atoi(&arg[3])) mastermask = (1<<MM_OPEN) | (1<<MM_VETO); return true;
			case 'M': s_strcpy(motd, &arg[3]); return true;
			default: break;
		}
		return false;
	}

    void setmaster(clientinfo *ci, bool val, const char *pass = "", bool approved = false)
	{
        if(approved && (!val || !ci->wantsmaster)) return;
		const char *name = "";
		if(val)
		{
			if(ci->privilege)
			{
				if(!masterpass[0] || !pass[0]==(ci->privilege!=PRIV_ADMIN)) return;
			}
            else if(ci->state.state==CS_SPECTATOR && (!masterpass[0] || strcmp(masterpass, pass))) return;
            loopv(clients) if(ci!=clients[i] && clients[i]->privilege)
			{
				if(masterpass[0] && !strcmp(masterpass, pass)) clients[i]->privilege = PRIV_NONE;
				else return;
			}
            if(masterpass[0] && !strcmp(masterpass, pass)) ci->privilege = PRIV_ADMIN;
            else if(!approved && !(mastermask&MM_AUTOAPPROVE) && !ci->privilege)
            {
                ci->wantsmaster = true;
                srvoutf(-1, "%s wants master. Type \"/approve %d\" to approve.", colorname(ci), ci->clientnum);
                return;
            }
			else ci->privilege = PRIV_MASTER;
			name = privname(ci->privilege);
		}
		else
		{
			if(!ci->privilege) return;
			name = privname(ci->privilege);
			ci->privilege = 0;
		}
		mastermode = MM_OPEN;
        srvoutf(-1, "%s %s %s", colorname(ci), val ? (approved ? "approved for" : "claimed") : "relinquished", name);
		currentmaster = val ? ci->clientnum : -1;
		masterupdate = true;
        loopv(clients) clients[i]->wantsmaster = false;
	}

	int clientconnect(int n, uint ip)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		ci->clientnum = n;
		clients.add(ci);
		loopv(bannedips) if(bannedips[i].ip==ip) return DISC_IPBAN;
		if(mastermode>=MM_PRIVATE) return DISC_PRIVATE;
		if(mastermode>=MM_LOCKED) ci->state.state = CS_SPECTATOR;
		if(currentmaster>=0) masterupdate = true;
		ci->state.lasttimeplayed = lastmillis;
		return DISC_NONE;
	}

	void clientdisconnect(int n)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		bot.removebots(ci);
		if(ci->privilege) setmaster(ci, false);
		if(smode) smode->leavegame(ci, true);
		mutate(smuts, mut->leavegame(ci));
		ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		savescore(ci);
		sendf(-1, 1, "ri2", SV_CDIS, n);
		clients.removeobj(ci);
		if(clients.empty()) cleanup(false);
		else
		{
			checkvotes();
			bot.checkbots(true);
		}
	}

	#include "extinfo.h"

	const char *servername() { return "bloodfrontierserver"; }
	int serverinfoport()
	{
		return SERVINFO_PORT;
	}
	int serverport()
	{
		return SERVER_PORT;
	}
	const char *getdefaultmaster()
	{
		return "bloodfrontier.com";
	}

    void serverinforeply(ucharbuf &req, ucharbuf &p)
	{
        if(!getint(req))
        {
            extserverinforeply(req, p);
            return;
        }

		int numplayers = 0;
		loopv(clients) if(clients[i] && clients[i]->state.ownernum < 0) numplayers++;
		putint(p, numplayers);
		putint(p, 6);					// number of attrs following
		putint(p, GAMEVERSION);			// 1
		putint(p, gamemode);			// 2
		putint(p, mutators);			// 3
		putint(p, minremain);			// 4
		putint(p, maxclients);			// 5
		putint(p, mastermode);			// 6
		sendstring(smapname, p);
		sendstring(serverdesc, p);
		sendserverinforeply(p);
	}

	void receivefile(int sender, uchar *data, int len)
	{
		if(!m_edit(gamemode) || len > 1024*1024) return;
		clientinfo *ci = (clientinfo *)getinfo(sender);
		if(ci->state.state==CS_SPECTATOR && !ci->privilege) return;
		if(mapdata) { fclose(mapdata); mapdata = NULL; }
		if(!len) return;
		mapdata = tmpfile();
        if(!mapdata) { sendf(sender, 1, "ris", SV_SERVMSG, "failed to open temporary file for map"); return; }
		fwrite(data, 1, len, mapdata);
		sendf(-1, 1, "ri2", SV_SENDMAP, sender);
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
		s_sprintf(cname)("\fs%s\fS", name);
		if(!name[0] || ci->state.ownernum >= 0 || (dupname && duplicatename(ci, name)))
		{
			s_sprintfd(s)(" [\fs%s%d\fS]", ci->state.ownernum >= 0 ? "\fc" : "\fm", ci->clientnum);
			s_strcat(cname, s);
		}
		if(team && m_team(gamemode, mutators))
		{
			s_sprintfd(s)(" (\fs%s%s\fS)", teamtype[ci->team].chat, teamtype[ci->team].name);
			s_strcat(cname, s);
		}
		return cname;
	}

    const char *gameid() { return GAMEID; }
    int gamever() { return GAMEVERSION; }
    char *gamename(int mode, int muts)
    {
    	static string gname;
    	gname[0] = 0;
    	if(gametype[mode].mutators && muts)
    	{
			loopi(G_M_NUM)
			{
				if ((gametype[mode].mutators & mutstype[i].type) && (muts & mutstype[i].type) &&
					!(gametype[mode].implied & mutstype[i].type))
				{
					s_sprintfd(name)("%s%s%s", *gname ? gname : "", *gname ? "-" : "", mutstype[i].name);
					s_strcpy(gname, name);
				}
			}
    	}
		s_sprintfd(mname)("%s%s%s", *gname ? gname : "", *gname ? " " : "", gametype[mode].name);
		s_strcpy(gname, mname);
		return gname;
    }

    void modecheck(int *mode, int *muts)
    {
		if(!m_game(*mode))
		{
			*mode = G_DEATHMATCH;
			*muts = gametype[*mode].implied;
		}

		if(gametype[*mode].mutators && *muts)
		{
			loopi(G_M_NUM)
			{
				if(!(gametype[*mode].mutators & mutstype[i].type) && (*muts & mutstype[i].type))
					*muts &= ~mutstype[i].type;

				if(gametype[*mode].implied && (gametype[*mode].implied & mutstype[i].type) && !(*muts & mutstype[i].type))
					*muts |= mutstype[i].type;

				if(*muts & mutstype[i].type)
				{
					loopj(G_M_NUM)
					{
						if(mutstype[i].mutators && !(mutstype[i].mutators & mutstype[j].type) && (*muts & mutstype[j].type))
							*muts &= ~mutstype[j].type;

						if(mutstype[i].implied && (mutstype[i].implied & mutstype[j].type) && !(*muts & mutstype[j].type))
							*muts |= mutstype[j].type;

						if((mutstype[i].implied & mutstype[j].type) && !(*muts & mutstype[j].type))
							*muts |= mutstype[j].type;
					}
				}
			}
		}
		else *muts = 0;
    }

	int mutscheck(int mode, int muts)
	{
		int gm = mode, mt = muts;
		modecheck(&gm, &mt);
		return mt;
	}

	const char *defaultmap() { return "refuge"; }
	int defaultmode() { return G_LOBBY; }

	bool canload(char *type)
	{
		if(strcmp(type, GAMEID) == 0) return true;
		if(strcmp(type, "fps") == 0 || strcmp(type, "bfg") == 0) return true;
		return false;
	}
};
