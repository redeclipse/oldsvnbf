#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _dup	dup
#define _fileno fileno
#endif

struct fpsserver : igameserver
{
	struct server_entity			// server side version of "entity" type
	{
		int type;
		int spawntime;
		char spawned;
	};

	static const int DEATHMILLIS = 250;

	enum { GE_NONE = 0, GE_SHOT, GE_EXPLODE, GE_HIT, GE_SUICIDE, GE_PICKUP };

	struct shotevent
	{
		int type;
		int millis;
		int gun;
		float from[3], to[3];
	};

	struct explodeevent
	{
		int type;
		int millis;
		int gun;
	};

	struct hitevent
	{
		int type;
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
		int type;
	};

	struct pickupevent
	{
		int type;
		int ent;
	};

	union gameevent
	{
		int type;
		shotevent shot;
		explodeevent explode;
		hitevent hit;
		suicideevent suicide;
		pickupevent pickup;
	};

	struct gamestate : fpsstate
	{
		vec o;
		int state;
#ifdef BFRONTER
		int lastdeath, lifesequence;
#else
		int lastdeath, lastspawn, lifesequence;
		int lastshot;
#endif
		int rockets, grenades;
		int frags;
		int lasttimeplayed, timeplayed;
		float effectiveness;

		gamestate() : state(CS_DEAD) {}
	
		bool isalive(int gamemillis)
		{
			return state==CS_ALIVE || (state==CS_DEAD && gamemillis - lastdeath <= DEATHMILLIS);
		}

#ifdef BFRONTIER
        bool waitexpired(int gamemillis, int gun)
        {
            return gamemillis - gunvar(gunlast,gun) >= gunvar(gunwait,gun);
        }
#else
        bool waitexpired(int gamemillis)
        {
            return gamemillis - lastshot >= gunwait;
        }
#endif

		void reset()
		{
			if(state!=CS_SPECTATOR) state = CS_DEAD;
			lifesequence = 0;
			maxhealth = 100;
			rockets = grenades = 0;

			timeplayed = 0;
			effectiveness = 0;
			frags = 0;

			respawn();
		}

		void respawn()
		{
			fpsstate::respawn();
			o = vec(-1e10f, -1e10f, -1e10f);
			lastdeath = 0;
#ifndef BFRONTIER
			lastspawn = -1;
			lastshot = 0;
#endif
		}
	};

	struct savedscore
	{
		uint ip;
		string name;
		int maxhealth, frags;
		int timeplayed;
		float effectiveness;

		void save(gamestate &gs)
		{
			maxhealth = gs.maxhealth;
			frags = gs.frags;
			timeplayed = gs.timeplayed;
			effectiveness = gs.effectiveness;
		}

		void restore(gamestate &gs)
		{
			gs.maxhealth = maxhealth;
			gs.frags = frags;
			gs.timeplayed = timeplayed;
			gs.effectiveness = effectiveness;
		}
	};

	struct clientinfo
	{
#ifdef BFRONTIER
		int modver;
#endif
		int clientnum;
		string name, team, mapvote;
		int modevote;
		int privilege;
        bool spectator, local, timesync;
        int gameoffset, lastevent;
		gamestate state;
		vector<gameevent> events;
		vector<uchar> position, messages;

		clientinfo() { reset(); }

		gameevent &addevent()
		{
			static gameevent dummy;
			if(events.length()>100) return dummy;
			return events.add();
		}

		void mapchange()
		{
			mapvote[0] = 0;
			state.reset();
			events.setsizenodelete(0);
            timesync = false;
            lastevent = 0;
		}

		void reset()
		{
#ifdef BFRONTIER
			modver = 0;
#endif
			name[0] = team[0] = 0;
			privilege = PRIV_NONE;
			spectator = local = false;
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
	
	enum { MM_OPEN = 0, MM_VETO, MM_LOCKED, MM_PRIVATE };
 
	bool notgotitems, notgotbases;		// true when map has changed and waiting for clients to send item
	int gamemode;
	int gamemillis, gamelimit;

	string serverdesc;
	string smapname;
	int lastmillis, totalmillis, curtime;
	int interm, minremain;
	bool mapreload;
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
		fpsserver &sv;

		servmode(fpsserver &sv) : sv(sv) {}
		virtual ~servmode() {}

		virtual void entergame(clientinfo *ci) {}
		virtual void leavegame(clientinfo *ci) {}

		virtual void moved(clientinfo *ci, const vec &oldpos, const vec &newpos) {}
#ifdef BFRONTIER
		virtual bool canspawn(clientinfo *ci, bool connecting = false, bool tryspawn = false) { return true; }
#else
        virtual bool canspawn(clientinfo *ci, bool connecting = false) { return true; }
#endif
		virtual void spawned(clientinfo *ci) {}
		virtual void died(clientinfo *victim, clientinfo *actor) {}
		virtual void changeteam(clientinfo *ci, const char *oldteam, const char *newteam) {}
		virtual void initclient(clientinfo *ci, ucharbuf &p, bool connecting) {}
		virtual void update() {}
		virtual void reset(bool empty) {}
		virtual void intermission() {}
#ifdef BFRONTIER
		virtual bool damage(clientinfo *target, clientinfo *actor, int damage, int gun, const vec &hitpush = vec(0, 0, 0)) { return true; }
#endif
	};

	struct arenaservmode : servmode
	{
		int arenaround;

		arenaservmode(fpsserver &sv) : servmode(sv), arenaround(0) {}

#ifdef BFRONTIER
		bool canspawn(clientinfo *ci, bool connecting = false, bool tryspawn = false) 
#else
        bool canspawn(clientinfo *ci, bool connecting = false) 
#endif
		{ 
			if(connecting && sv.nonspectators(ci->clientnum)<=1) return true;
			return false; 
		}

		void reset(bool empty)
		{
			arenaround = 0;
		}
	
		void update()
		{
			if(sv.interm || sv.gamemillis<arenaround || !sv.nonspectators()) return;
	
			if(arenaround)
			{
				arenaround = 0;
				loopv(sv.clients) if(sv.clients[i]->state.state==CS_DEAD || sv.clients[i]->state.state==CS_ALIVE) 
				{
					sv.clients[i]->state.respawn();
					sv.sendspawn(sv.clients[i]);
				}
				return;
			}

			int gamemode = sv.gamemode;
			clientinfo *alive = NULL;
			bool dead = false;
			loopv(sv.clients)
			{
				clientinfo *ci = sv.clients[i];
				if(ci->state.state==CS_ALIVE || (ci->state.state==CS_DEAD && ci->state.lastspawn>=0))
				{
					if(!alive) alive = ci;
					else if(!m_teammode || strcmp(alive->team, ci->team)) return;
				}
				else if(ci->state.state==CS_DEAD) dead = true;
			}
			if(!dead) return;
			sendf(-1, 1, "ri2", SV_ARENAWIN, !alive ? -1 : alive->clientnum);
			arenaround = sv.gamemillis+5000;
		}
	};

	#define CAPTURESERV 1
	#include "capture.h"
	#undef CAPTURESERV

	arenaservmode arenamode;
	captureservmode capturemode;
	servmode *smode;

#ifdef BFRONTIER
	#include "duel.h"
	duelservmode duelmode;

	enum
	{
		CM_ANY = 0,
		CM_ALL,
		CM_MASTER
	};
	
	#define SRVCMD(n, v, g, b) SCOMMAND(fpsserver, n, v, g, { \
				if (self->cmdcontext) \
				{ \
					if (self->hasmode(self->cmdcontext, v)) { b; } \
					else { s_sprintf(self->scresult)("insufficient access"); } \
					result(self->scresult); \
				} \
			});
	
	#define SRVVAR(n, p, v, m, x, b) SRVCMD(n, v, "i", self->servvar(self->cmdcontext, #n, p, m, x, args[0]); b)
	#define POWERUPS I_QUAD-I_SHELLS+1
	
	#define Q_INT(c,n) { if(!c->local) { ucharbuf buf = c->messages.reserve(5); putint(buf, n); c->messages.addbuf(buf); } }
	#define Q_STR(c,text) { if(!c->local) { ucharbuf buf = c->messages.reserve(2*strlen(text)+1); sendstring(text, buf); c->messages.addbuf(buf); } }

	enum
	{
		SPAWN_HEALTH, SPAWN_MAXHEALTH, SPAWN_ARMOUR, SPAWN_ARMOURTYPE, SPAWN_GUNSELECT,
		SPAWN_SG, SPAWN_CG, SPAWN_RL, SPAWN_RIFLE, SPAWN_GL, SPAWN_PISTOL,
		SPAWN_QUAD, SPAWNSTATES
	};
	
	struct scr
	{
		char *name;
		int val;
		
		scr(char *_n, int _v) : name(_n), val(_v) {}
		~scr() {}
	};

	clientinfo *cmdcontext;
	int timelimit, fraglimit, damagescale, instakill, teamdamage;
	bool powerups[POWERUPS];
	int spawnstates[SPAWNSTATES];
	string scresult, motd;
	fpsserver() : notgotitems(true), notgotbases(false),
					gamemode(0), interm(0), minremain(0), mapreload(false),
					lastsend(0), mastermode(MM_OPEN), mastermask(~0), currentmaster(-1), masterupdate(false),
					mapdata(NULL), reliablemessages(false), demonextmatch(false),
					demotmp(NULL), demorecord(NULL), demoplayback(NULL), nextplayback(0),
					arenamode(*this), capturemode(*this), smode(NULL),
					duelmode(*this), cmdcontext(NULL)
	{
		SRVCMD(version, CM_ANY, "si", self->setversion(self->cmdcontext, args[0], atoi(args[1])));
		
		SRVVAR(timelimit, &self->timelimit, CM_ALL, 0, INT_MAX-1, self->settime());
		SRVVAR(fraglimit, &self->fraglimit, CM_ALL, 0, INT_MAX-1, );
		
		SRVVAR(duelgame, &self->duelmode.duelgame, CM_ALL, 0, 10000, );

		SRVVAR(damagescale, &self->damagescale, CM_ALL, 0, 1000, );
		SRVVAR(instakill, &self->instakill, CM_ALL, 0, 1, );
		SRVVAR(teamdamage, &self->teamdamage, CM_ALL, 0, 1, );
		
		SRVCMD(powerup, CM_ALL, "ss", self->setpowerup(self->cmdcontext, args[0], args[1]));
		SRVCMD(spawnstate, CM_ALL, "ss", self->setspawnstate(self->cmdcontext, args[0], args[1]));
		
		motd[0] = 0;
		resetvars();
#else
	fpsserver() : notgotitems(true), notgotbases(false), gamemode(0), interm(0), minremain(0), mapreload(false), lastsend(0), mastermode(MM_OPEN), mastermask(~0), currentmaster(-1), masterupdate(false), mapdata(NULL), reliablemessages(false), demonextmatch(false), demotmp(NULL), demorecord(NULL), demoplayback(NULL), nextplayback(0), arenamode(*this), capturemode(*this), smode(NULL)
	{
#endif
		serverdesc[0] = '\0';
		masterpass[0] = '\0';
	}

	void *newinfo() { return new clientinfo; }
	void deleteinfo(void *ci) { delete (clientinfo *)ci; } 
	
	vector<server_entity> sents;
	vector<savedscore> scores;

	static const char *modestr(int n)
	{
#ifdef BFRONTIER
		static const char *modenames[] =
		{
			"Demo", "SP", "DMSP", "\fs\f0FFA\fr", "\fs\f5CoopEdit\fr", "\fs\f0FFA/Duel\fr", "\fs\f0Teamplay\fr",
			"\fs\f3Insta\fr", "\fs\f3TeamInsta\fr", "\fs\f2Efficiency\fr", "\f2TeamEfficiency\fr",
			"\fs\f3InstaArena\fr", "\fs\f3InstaClanArena\fr", "\fs\f2TacticsArena\fr", "\f2TacticsClanArena\fr",
			"\fs\f1Capture\fr", "\fs\f1InstaCapture\fr"
		};
#else
		static const char *modenames[] =
		{
			"demo", "SP", "DMSP", "ffa/default", "coopedit", "ffa/duel", "teamplay",
			"instagib", "instagib team", "efficiency", "efficiency team",
			"insta arena", "insta clan arena", "tactics arena", "tactics clan arena",
			"capture", "insta capture"
		};
#endif
		return (n>=-3 && size_t(n+3)<sizeof(modenames)/sizeof(modenames[0])) ? modenames[n+3] : "unknown";
	}

	void sendservmsg(const char *s) { sendf(-1, 1, "ris", SV_SERVMSG, s); }

	void resetitems() 
	{ 
		sents.setsize(0);
		//cps.reset(); 
	}

	int spawntime(int type)
	{
		if(m_classicsp) return INT_MAX;
        int np = nonspectators();
		np = np<3 ? 4 : (np>4 ? 2 : 3);		 // spawn times are dependent on number of players
		int sec = 0;
		switch(type)
		{
			case I_SHELLS:
			case I_BULLETS:
			case I_ROCKETS:
			case I_ROUNDS:
			case I_GRENADES:
			case I_CARTRIDGES: sec = np*4; break;
			case I_HEALTH: sec = np*5; break;
			case I_GREENARMOUR:
			case I_YELLOWARMOUR: sec = 20; break;
			case I_BOOST:
			case I_QUAD: sec = 40+rnd(40); break;
		}
		return sec*1000;
	}
		
	bool pickup(int i, int sender)		 // server side item pickup, acknowledge first client that gets it
	{
		if(minremain<=0 || !sents.inrange(i) || !sents[i].spawned) return false;
		clientinfo *ci = (clientinfo *)getinfo(sender);
		if(!ci || (!ci->local && !ci->state.canpickup(sents[i].type))) return false;
		sents[i].spawned = false;
		sents[i].spawntime = spawntime(sents[i].type);
		sendf(-1, 1, "ri3", SV_ITEMACC, i, sender);
		ci->state.pickup(sents[i].type);
		return true;
	}

	void vote(char *map, int reqmode, int sender)
	{
		clientinfo *ci = (clientinfo *)getinfo(sender);
        if(!ci || ci->state.state==CS_SPECTATOR && !ci->privilege) return;
		s_strcpy(ci->mapvote, map);
		ci->modevote = reqmode;
		if(!ci->mapvote[0]) return;
		if(ci->local || mapreload || (ci->privilege && mastermode>=MM_VETO))
		{
			if(demorecord) enddemorecord();
			if(!ci->local && !mapreload) 
			{
				s_sprintfd(msg)("%s forced %s on map %s", privname(ci->privilege), modestr(reqmode), map);
				sendservmsg(msg);
			}
			sendf(-1, 1, "risi", SV_MAPCHANGE, ci->mapvote, ci->modevote);
			changemap(ci->mapvote, ci->modevote);
		}
		else 
		{
			s_sprintfd(msg)("%s suggests %s on map %s (select map to vote)", colorname(ci), modestr(reqmode), map);
			sendservmsg(msg);
			checkvotes();
		}
	}

	clientinfo *choosebestclient(float &bestrank)
	{
		clientinfo *best = NULL;
		bestrank = -1;
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->state.timeplayed<0) continue;
			float rank = ci->state.effectiveness/max(ci->state.timeplayed, 1);
			if(!best || rank > bestrank) { best = ci; bestrank = rank; }
		}
		return best;
	}  

	void autoteam()
	{
		static const char *teamnames[2] = {"good", "evil"};
		vector<clientinfo *> team[2];
		float teamrank[2] = {0, 0};
		for(int round = 0, remaining = clients.length(); remaining>=0; round++)
		{
			int first = round&1, second = (round+1)&1, selected = 0;
			while(teamrank[first] <= teamrank[second])
			{
				float rank;
				clientinfo *ci = choosebestclient(rank);
				if(!ci) break;
				if(m_capture) rank = 1;
				else if(selected && rank<=0) break;	
				ci->state.timeplayed = -1;
				team[first].add(ci);
				teamrank[first] += rank;
				selected++;
				if(rank<=0) break;
			}
			if(!selected) break;
			remaining -= selected;
		}
		loopi(sizeof(team)/sizeof(team[0]))
		{
			loopvj(team[i])
			{
				clientinfo *ci = team[i][j];
				if(!strcmp(ci->team, teamnames[i])) continue;
				s_strncpy(ci->team, teamnames[i], MAXTEAMLEN+1);
				sendf(-1, 1, "riis", SV_SETTEAM, ci->clientnum, teamnames[i]);
			}
		}
	}

	struct teamscore
	{
		const char *name;
		float rank;
		int clients;

		teamscore(const char *name) : name(name), rank(0), clients(0) {}
	};
	
	const char *chooseworstteam(const char *suggest)
	{
		teamscore teamscores[2] = { teamscore("good"), teamscore("evil") };
		const int numteams = sizeof(teamscores)/sizeof(teamscores[0]);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(!ci->team[0]) continue;
			ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
			ci->state.lasttimeplayed = lastmillis;

			loopj(numteams) if(!strcmp(ci->team, teamscores[j].name)) 
			{ 
				teamscore &ts = teamscores[j];
				ts.rank += ci->state.effectiveness/max(ci->state.timeplayed, 1);
				ts.clients++;
				break;
			}
		}
		teamscore *worst = &teamscores[numteams-1];
		loopi(numteams-1)
		{
			teamscore &ts = teamscores[i];
			if(m_capture)
			{
				if(ts.clients < worst->clients || (ts.clients == worst->clients && ts.rank < worst->rank)) worst = &ts;
			}
			else if(ts.rank < worst->rank || (ts.rank == worst->rank && ts.clients < worst->clients)) worst = &ts;
		}
		return worst->name;
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
		s_sprintf(d.info)("%s: %s, %s, %.2f%s", timestr, modestr(gamemode), smapname, len > 1024*1024 ? len/(1024*1024.f) : len/1024.0f, len > 1024*1024 ? "MB" : "kB");
		s_sprintfd(msg)("demo \"%s\" recorded", d.info);
		sendservmsg(msg);
		d.data = new uchar[len];
		d.len = len;
		fread(d.data, 1, len, demotmp);
		fclose(demotmp);
		demotmp = NULL;
	}

	void setupdemorecord()
	{
		if(haslocalclients() || !m_mp(gamemode) || gamemode==1) return;
		demotmp = tmpfile();
		if(!demotmp) return;
		setvbuf(demotmp, NULL, _IONBF, 0);

		demorecord = gzdopen(_dup(_fileno(demotmp)), "wb9");
		if(!demorecord)
		{
			fclose(demotmp);
			demotmp = NULL;
			return;
		}

        sendservmsg("recording demo");

		demoheader hdr;
		memcpy(hdr.magic, DEMO_MAGIC, sizeof(hdr.magic));
		hdr.version = DEMO_VERSION;
		hdr.protocol = PROTOCOL_VERSION;
		endianswap(&hdr.version, sizeof(int), 1);
		endianswap(&hdr.protocol, sizeof(int), 1);
		gzwrite(demorecord, &hdr, sizeof(demoheader));

		uchar buf[MAXTRANS];
		ucharbuf p(buf, sizeof(buf));
		welcomepacket(p, -1);
		writedemo(1, buf, p.len);

		loopv(clients)
		{
			clientinfo *ci = clients[i];
			uchar header[16];
			ucharbuf q(&buf[sizeof(header)], sizeof(buf)-sizeof(header));
			putint(q, SV_INITC2S);
			sendstring(ci->name, q);
			sendstring(ci->team, q);

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
			sendservmsg("cleared all demos");
		}
		else if(demos.inrange(n-1))
		{
			delete[] demos[n-1].data;
			demos.remove(n-1);
			s_sprintfd(msg)("cleared demo %d", n);
			sendservmsg(msg);
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
			endianswap(&hdr.protocol, sizeof(int), 1);
#ifdef BFRONTIER
			if(hdr.version!=DEMO_VERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.version<DEMO_VERSION ? "older" : "newer");
			else if(hdr.protocol!=PROTOCOL_VERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.protocol<PROTOCOL_VERSION ? "older" : "newer");
#else
			if(hdr.version!=DEMO_VERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Sauerbraten", file, hdr.version<DEMO_VERSION ? "older" : "newer");
			else if(hdr.protocol!=PROTOCOL_VERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Sauerbraten", file, hdr.protocol<PROTOCOL_VERSION ? "older" : "newer");
#endif
		}
		if(msg[0])
		{
			if(demoplayback) { gzclose(demoplayback); demoplayback = NULL; }
			sendservmsg(msg);
			return;
		}

		s_sprintf(msg)("playing demo \"%s\"", file);
		sendservmsg(msg);

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

		sendservmsg("demo playback finished");

		loopv(clients)
		{
			ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
			ucharbuf p(packet->data, packet->dataLength);
			welcomepacket(p, clients[i]->clientnum);
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
 
	void changemap(const char *s, int mode)
	{
		if(m_demo) enddemoplayback();
		else enddemorecord();

		mapreload = false;
		gamemode = mode;
		gamemillis = 0;
#ifdef BFRONTIER
		minremain = timelimit;
#else
        minremain = m_teammode ? 15 : 10;
#endif
		gamelimit = minremain*60000;
		interm = 0;
		s_strcpy(smapname, s);
		resetitems();
		notgotitems = true;
		scores.setsize(0);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		}
		if(m_teammode) autoteam();

#ifdef BFRONTIER
		if (!m_capture && duelmode.duelgame) smode = &duelmode;
		else
#endif
		if(m_arena) smode = &arenamode;
		else if(m_capture) smode = &capturemode;
		else smode = NULL;
		if(smode) smode->reset(false);

		if(gamemode>1 || (gamemode==0 && hasnonlocalclients())) sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			ci->mapchange();
			ci->state.lasttimeplayed = lastmillis;
            if(m_mp(gamemode) && ci->state.state!=CS_SPECTATOR) sendspawn(ci);
		}

		if(m_demo) setupdemoplayback();
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
		if(!insert) loopv(clients)
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
		int mode, count;
		votecount() {}
		votecount(char *s, int n) : map(s), mode(n), count(0) {}
	};

	void checkvotes(bool force = false)
	{
		vector<votecount> votes;
		int maxvotes = 0;
		loopv(clients)
		{
			clientinfo *oi = clients[i];
			if(oi->state.state==CS_SPECTATOR && !oi->privilege) continue;
			maxvotes++;
			if(!oi->mapvote[0]) continue;
			votecount *vc = NULL;
			loopvj(votes) if(!strcmp(oi->mapvote, votes[j].map) && oi->modevote==votes[j].mode)
			{ 
				vc = &votes[j];
				break;
			}
			if(!vc) vc = &votes.add(votecount(oi->mapvote, oi->modevote));
			vc->count++;
		}
		votecount *best = NULL;
		loopv(votes) if(!best || votes[i].count > best->count || (votes[i].count == best->count && rnd(2))) best = &votes[i];
		if(force || (best && best->count > maxvotes/2))
		{
			if(demorecord) enddemorecord();
			if(best && (best->count > (force ? 1 : maxvotes/2)))
			{ 
				sendservmsg(force ? "vote passed by default" : "vote passed by majority");
				sendf(-1, 1, "risi", SV_MAPCHANGE, best->map, best->mode);
				changemap(best->map, best->mode); 
			}
			else
			{
				mapreload = true;
				if(clients.length()) sendf(-1, 1, "ri", SV_MAPRELOAD);
			}
		}
	}

	int nonspectators(int exclude = -1)
	{
		int n = 0;
		loopv(clients) if(i!=exclude && clients[i]->state.state!=CS_SPECTATOR) n++;
		return n;
	}

	int checktype(int type, clientinfo *ci)
	{
		if(ci && ci->local) return type;
		// spectators can only connect and talk
		static int spectypes[] = { SV_INITC2S, SV_POS, SV_TEXT, SV_PING, SV_CLIENTPING, SV_GETMAP, SV_SETMASTER };
		if(ci && ci->state.state==CS_SPECTATOR && !ci->privilege)
		{
			loopi(sizeof(spectypes)/sizeof(int)) if(type == spectypes[i]) return type;
			return -1;
		}
		// only allow edit messages in coop-edit mode
		if(type>=SV_EDITENT && type<=SV_GETMAP && gamemode!=1) return -1;
		// server only messages
		static int servtypes[] = { SV_INITS2C, SV_MAPRELOAD, SV_SERVMSG, SV_DAMAGE, SV_HITPUSH, SV_SHOTFX, SV_DIED, SV_SPAWNSTATE, SV_FORCEDEATH, SV_ARENAWIN, SV_ITEMACC, SV_ITEMSPAWN, SV_TIMEUP, SV_CDIS, SV_CURRENTMASTER, SV_PONG, SV_RESUME, SV_TEAMSCORE, SV_BASEINFO, SV_ANNOUNCE, SV_SENDDEMOLIST, SV_SENDDEMO, SV_DEMOPLAYBACK, SV_SENDMAP, SV_CLIENT };
		if(ci) loopi(sizeof(servtypes)/sizeof(int)) if(type == servtypes[i]) return -1;
		return type;
	}

	static void freecallback(ENetPacket *packet)
	{
		extern igameserver *sv;
		((fpsserver *)sv)->cleanworldstate(packet);
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
		enet_uint32 curtime = enet_time_get()-lastsend;
		if(curtime<33) return false;
		bool flush = buildworldstate();
		lastsend += curtime - (curtime%33);
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
		int cn = -1, type;
		clientinfo *ci = sender>=0 ? (clientinfo *)getinfo(sender) : NULL;
		#define QUEUE_MSG { if(!ci->local) while(curmsg<p.length()) ci->messages.add(p.buf[curmsg++]); }
		#define QUEUE_INT(n) { if(!ci->local) { curmsg = p.length(); ucharbuf buf = ci->messages.reserve(5); putint(buf, n); ci->messages.addbuf(buf); } }
		#define QUEUE_UINT(n) { if(!ci->local) { curmsg = p.length(); ucharbuf buf = ci->messages.reserve(4); putuint(buf, n); ci->messages.addbuf(buf); } }
		#define QUEUE_STR(text) { if(!ci->local) { curmsg = p.length(); ucharbuf buf = ci->messages.reserve(2*strlen(text)+1); sendstring(text, buf); ci->messages.addbuf(buf); } }
		int curmsg;
        while((curmsg = p.length()) < p.maxlen) switch(type = checktype(getint(p), ci))
		{
			case SV_POS:
			{
				cn = getint(p);
				if(cn<0 || cn>=getnumclients() || cn!=sender)
				{
					conoutf("parsepacket pos from %d on %d but cn = %d", sender, chan, cn);
					disconnect_client(sender, DISC_CN);
					return;
				}
				vec oldpos(ci->state.o);
				loopi(3) ci->state.o[i] = getuint(p)/DMF;
				getuint(p);
				loopi(5) getint(p);
				int physstate = getuint(p);
				if(physstate&0x20) loopi(2) getint(p);
				if(physstate&0x10) getint(p);
                if(!ci->local && (ci->state.state==CS_ALIVE || ci->state.state==CS_EDITING))
				{
					ci->position.setsizenodelete(0);
					while(curmsg<p.length()) ci->position.add(p.buf[curmsg++]);
				}
				uint f = getuint(p);
                if(!ci->local && (ci->state.state==CS_ALIVE || ci->state.state==CS_EDITING))
				{
					f &= 0xF;
					if(ci->state.armourtype==A_GREEN && ci->state.armour>0) f |= 1<<4;
					if(ci->state.armourtype==A_YELLOW && ci->state.armour>0) f |= 1<<5;
					if(ci->state.quadmillis) f |= 1<<6;
#ifdef BFRONTIER
					if(!g_bf && ci->state.maxhealth>100) f |= ((ci->state.maxhealth-100)/getitem(I_BOOST-I_SHELLS).add)<<7;
#else
					if(ci->state.maxhealth>100) f |= ((ci->state.maxhealth-100)/itemstats[I_BOOST-I_SHELLS].add)<<7;
#endif
					curmsg = p.length();
					ucharbuf buf = ci->position.reserve(4);
					putuint(buf, f);
					ci->position.addbuf(buf);
				}
				if(smode && ci->state.state==CS_ALIVE) smode->moved(ci, oldpos, ci->state.o);
				break;
			}

			case SV_EDITMODE:
			{
				int val = getint(p);
				if(ci->state.state!=(val ? CS_ALIVE : CS_EDITING) || (!ci->local && gamemode!=1)) break;
				if(smode)
				{
					if(val) smode->leavegame(ci);
					else smode->entergame(ci);
				}
				ci->state.state = val ? CS_EDITING : CS_ALIVE;
				if(val)
				{
					ci->events.setsizenodelete(0);
					ci->state.rockets = ci->state.grenades = 0;
				}
				QUEUE_MSG;
				break;
			}

			case SV_TRYSPAWN:
#ifdef BFRONTIER
                if(ci->state.state!=CS_DEAD || ci->state.lastspawn>=0 || (smode && !smode->canspawn(ci, false, true)))
                {
					servsend(ci->clientnum, "you are %s", ci->state.state!=CS_DEAD ? "not dead" : (ci->state.lastspawn>=0 ? "not spawned" : "forbidden to spawn"));
                	break;
                }
#else
                if(ci->state.state!=CS_DEAD || ci->state.lastspawn>=0 || (smode && !smode->canspawn(ci))) break;
#endif
				if(ci->state.lastdeath) ci->state.respawn();
				sendspawn(ci);
				break;

			case SV_GUNSELECT:
			{
				int gunselect = getint(p);
				ci->state.gunselect = gunselect;
				QUEUE_MSG;
				break;
			}

#ifdef BFRONTIER
			case SV_TRYRELOAD:
			{
				if (g_bf) getint(p);
				break;
			}
#endif
			case SV_SPAWN:
			{
				int ls = getint(p), gunselect = getint(p);
				if((ci->state.state!=CS_ALIVE && ci->state.state!=CS_DEAD) || ls!=ci->state.lifesequence || ci->state.lastspawn<0) break; 
				ci->state.lastspawn = -1;
				ci->state.state = CS_ALIVE;
				ci->state.gunselect = gunselect;
				if(smode) smode->spawned(ci);
#ifdef BFRONTIER
				if (spawnstates[SPAWN_QUAD] >= 0) // only works if map has a quad
				{
					loopv(sents)
					{
						if (sents[i].type == I_QUAD)
						{
							sendf(ci->clientnum, 1, "ri3", SV_ITEMACC, i, ci->clientnum);
							break;
						}
					}
				}
#endif
				QUEUE_MSG;
				break;
			}

			case SV_SUICIDE:
			{
				gameevent &suicide = ci->addevent();
				suicide.type = GE_SUICIDE;
				break;
			}

			case SV_SHOOT:
			{
				gameevent &shot = ci->addevent();
				shot.type = GE_SHOT;
#ifdef BFRONTIER
                #define seteventmillis(event) \
                { \
					int offset = getint(p); \
					event.gun = getint(p); \
                    if(!ci->timesync || (ci->events.length()==1 && ci->state.waitexpired(gamemillis, event.gun))) \
                    { \
                        ci->timesync = true; \
                        ci->gameoffset = gamemillis - offset; \
                        event.millis = gamemillis; \
                    } \
                    else event.millis = ci->gameoffset + offset; \
				}
#else
                #define seteventmillis(event) \
                { \
                    if(!ci->timesync || (ci->events.length()==1 && ci->state.waitexpired(gamemillis))) \
                    { \
                        ci->timesync = true; \
                        ci->gameoffset = gamemillis - getint(p); \
                        event.millis = gamemillis; \
                    } \
                    else event.millis = ci->gameoffset + getint(p); \
				}
#endif

                seteventmillis(shot.shot);
#ifndef BFRONTIER
				shot.shot.gun = getint(p);
#endif
				loopk(3) shot.shot.from[k] = getint(p)/DMF;
				loopk(3) shot.shot.to[k] = getint(p)/DMF;
				int hits = getint(p);
				loopk(hits)
				{
					gameevent &hit = ci->addevent();
					hit.type = GE_HIT;
					hit.hit.target = getint(p);
					hit.hit.lifesequence = getint(p);
					hit.hit.rays = getint(p);
					loopk(3) hit.hit.dir[k] = getint(p)/DNF;
				}
				break;
			}

			case SV_EXPLODE:
			{
				gameevent &exp = ci->addevent();
				exp.type = GE_EXPLODE;
                seteventmillis(exp.explode);
#ifndef BFRONTIER
				exp.explode.gun = getint(p);
#endif
				int hits = getint(p);
				loopk(hits)
				{
					gameevent &hit = ci->addevent();
					hit.type = GE_HIT;
					hit.hit.target = getint(p);
					hit.hit.lifesequence = getint(p);
					hit.hit.dist = getint(p)/DMF;
					loopk(3) hit.hit.dir[k] = getint(p)/DNF;
				}
				break;
			}

			case SV_ITEMPICKUP:
			{
				int n = getint(p);
				gameevent &pickup = ci->addevent();
				pickup.type = GE_PICKUP;
				pickup.pickup.ent = n;
				break;
			}

			case SV_TEXT:
				QUEUE_MSG;
				getstring(text, p);
				filtertext(text, text);
#ifdef BFRONTIER
				if (text[0] == '!') { servcmd(ci, text, true); QUEUE_STR(""); }
				else QUEUE_STR(text);
#else
                QUEUE_STR(text);
#endif
				break;

#ifdef BFRONTIER
			case SV_SERVCMD:
				getstring(text, p);
				filtertext(text, text);
				servcmd(ci, text, false);
				break;
#endif
			case SV_INITC2S:
			{
				QUEUE_MSG;
				bool connected = !ci->name[0];
				getstring(text, p);
				filtertext(text, text, false, MAXNAMELEN);
				if(!text[0]) s_strcpy(text, "unnamed");
				QUEUE_STR(text);
				s_strncpy(ci->name, text, MAXNAMELEN+1);
				if(!ci->local && connected)
				{
					savedscore &sc = findscore(ci, false);
					if(&sc) 
					{
						sc.restore(ci->state);
						sendf(-1, 1, "ri8", SV_RESUME, sender, ci->state.state, ci->state.lifesequence, ci->state.gunselect, sc.maxhealth, sc.frags, -1);
					}
				}
				getstring(text, p);
				filtertext(text, text, false, MAXTEAMLEN);
				if(!ci->local && connected && m_teammode)
				{
					const char *worst = chooseworstteam(text);
					if(worst)
					{
						s_strcpy(text, worst);
						sendf(sender, 1, "riis", SV_SETTEAM, sender, worst);
						QUEUE_STR(worst);
					}
					else QUEUE_STR(text);
				}
				else QUEUE_STR(text);
				if(smode && ci->state.state==CS_ALIVE && strcmp(ci->team, text)) smode->changeteam(ci, ci->team, text);
				s_strncpy(ci->team, text, MAXTEAMLEN+1);
				QUEUE_MSG;
				break;
			}

			case SV_MAPVOTE:
			case SV_MAPCHANGE:
			{
				getstring(text, p);
				filtertext(text, text);
				int reqmode = getint(p);
				if(type!=SV_MAPVOTE && !mapreload) break;
				if(!ci->local && !m_mp(reqmode)) reqmode = 0;
				vote(text, reqmode, sender);
				break;
			}

			case SV_ITEMLIST:
			{
				int n;
				while((n = getint(p))!=-1)
				{
					server_entity se = { getint(p), false, 0 };
					if(notgotitems)
					{
						while(sents.length()<=n) sents.add(se);
						if(gamemode>=0 && (sents[n].type==I_QUAD || sents[n].type==I_BOOST)) sents[n].spawntime = spawntime(sents[n].type);
						else sents[n].spawned = true;
					}
				}
				notgotitems = false;
				break;
			}

			case SV_TEAMSCORE:
				getstring(text, p);
				getint(p);
				QUEUE_MSG;
				break;

			case SV_BASEINFO:
				getint(p);
				getstring(text, p);
				getstring(text, p);
				getint(p);
				QUEUE_MSG;
				break;

			case SV_BASES:
				if(smode==&capturemode) capturemode.parsebases(p);
				break;

			case SV_REPAMMO:
				if(smode==&capturemode) capturemode.replenishammo(ci);
				break;

			case SV_PING:
				sendf(sender, 1, "i2", SV_PONG, getint(p));
				break;

			case SV_MASTERMODE:
			{
				int mm = getint(p);
				if(ci->privilege && mm>=MM_OPEN && mm<=MM_PRIVATE)
				{
					if(ci->privilege>=PRIV_ADMIN || (mastermask&(1<<mm)))
					{
						mastermode = mm;
						s_sprintfd(s)("mastermode is now %d", mastermode);
						sendservmsg(s);
					}
					else
					{
						s_sprintfd(s)("mastermode %d is disabled on this server", mm);
						sendf(sender, 1, "ris", SV_SERVMSG, s); 
					}
				}	
				break;
			}
			
			case SV_CLEARBANS:
			{
                if(ci->privilege)
                {
					bannedips.setsize(0);
					sendservmsg("cleared all bans");
                }
				break;
			}

			case SV_KICK:
			{
				int victim = getint(p);
				if(ci->privilege && victim>=0 && victim<getnumclients() && ci->clientnum!=victim && getinfo(victim))
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
				if(!ci->privilege && spectator!=sender) break;
				clientinfo *spinfo = (clientinfo *)getinfo(spectator);
				if(!spinfo) break;

				sendf(-1, 1, "ri3", SV_SPECTATOR, spectator, val);

				if(spinfo->state.state!=CS_SPECTATOR && val)
				{
					if(smode && spinfo->state.state==CS_ALIVE) smode->leavegame(spinfo);
					spinfo->state.state = CS_SPECTATOR;
				}
				else if(spinfo->state.state==CS_SPECTATOR && !val)
				{
					spinfo->state.state = CS_DEAD;
					spinfo->state.respawn();
                    if(!smode || smode->canspawn(spinfo)) sendspawn(spinfo);
				}
				break;
			}

			case SV_SETTEAM:
			{
				int who = getint(p);
				getstring(text, p);
				filtertext(text, text, false, MAXTEAMLEN);
				if(!ci->privilege || who<0 || who>=getnumclients()) break;
				clientinfo *wi = (clientinfo *)getinfo(who);
				if(!wi) break;
				if(smode && wi->state.state==CS_ALIVE && strcmp(wi->team, text)) smode->changeteam(wi, wi->team, text);
				s_strncpy(wi->team, text, MAXTEAMLEN+1);
				sendf(sender, 1, "riis", SV_SETTEAM, who, text);
				QUEUE_INT(SV_SETTEAM);
				QUEUE_INT(who);
				QUEUE_STR(text);
				break;
			} 

			case SV_FORCEINTERMISSION:
				if(m_sp) startintermission();
				break;

			case SV_RECORDDEMO:
			{
                int val = getint(p);
				if(ci->privilege<PRIV_ADMIN) break;
                demonextmatch = val!=0;
				s_sprintfd(msg)("demo recording is %s for next match", demonextmatch ? "enabled" : "disabled"); 
				sendservmsg(msg);
				break;
			}

			case SV_STOPDEMO:
			{
				if(!ci->local && ci->privilege<PRIV_ADMIN) break;
				if(m_demo) enddemoplayback();
				else enddemorecord();
				break;
			}

			case SV_CLEARDEMOS:
			{
				int demo = getint(p);
				if(ci->privilege<PRIV_ADMIN) break;
				cleardemos(demo);
				break;
			}

			case SV_LISTDEMOS:
				listdemos(sender);
				break;

			case SV_GETDEMO:
				senddemo(sender, getint(p));
				break;

			case SV_GETMAP:
				if(mapdata)
				{
					sendf(sender, 1, "ris", SV_SERVMSG, "server sending map...");
					sendfile(sender, 2, mapdata, "ri", SV_SENDMAP);
				}
				else sendf(sender, 1, "ris", SV_SERVMSG, "no map to send"); 
				break;

			case SV_NEWMAP:
			{
				int size = getint(p);
				if(size>=0)
				{
					smapname[0] = '\0';
					resetitems();
					notgotitems = false;
					if(smode) smode->reset(true);
#ifdef BFRONTIER
					resetvars();
#endif
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

			default:
			{
				int size = msgsizelookup(type);
				if(size==-1) { disconnect_client(sender, DISC_TAGT); return; }
				if(size>0) loopi(size-1) getint(p);
				if(ci) QUEUE_MSG;
				break;
			}
		}
	}

	int welcomepacket(ucharbuf &p, int n)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		int hasmap = (gamemode==1 && clients.length()>1) || (smapname[0] && (minremain>0 || (ci && ci->state.state==CS_SPECTATOR) || nonspectators(n)));
		putint(p, SV_INITS2C);
		putint(p, n);
		putint(p, PROTOCOL_VERSION);
		putint(p, hasmap);
		if(hasmap)
		{
			putint(p, SV_MAPCHANGE);
			sendstring(smapname, p);
			putint(p, gamemode);
			if(!ci || gamemode>1 || (gamemode==0 && hasnonlocalclients()))
			{
				putint(p, SV_TIMEUP);
				putint(p, minremain);
			}
			putint(p, SV_ITEMLIST);
			loopv(sents) if(sents[i].spawned)
			{
				putint(p, i);
				putint(p, sents[i].type);
			}
			putint(p, -1);
		}
		if(ci && (m_demo || m_mp(gamemode)) && ci->state.state!=CS_SPECTATOR)
		{
            if(smode && !smode->canspawn(ci, true))
			{
				ci->state.state = CS_DEAD;
				putint(p, SV_FORCEDEATH);
				putint(p, n);
				sendf(-1, 1, "ri2x", SV_FORCEDEATH, n, n);
			}
			else
			{
				gamestate &gs = ci->state;
				spawnstate(ci);
				putint(p, SV_SPAWNSTATE);
				putint(p, gs.lifesequence);
				putint(p, gs.health);
				putint(p, gs.maxhealth);
				putint(p, gs.armour);
				putint(p, gs.armourtype);
				putint(p, gs.gunselect);
				loopi(GUN_PISTOL-GUN_SG+1) putint(p, gs.ammo[GUN_SG+i]);
				gs.lastspawn = gamemillis; 
			}
		}
		if(ci && ci->state.state==CS_SPECTATOR)
		{
			putint(p, SV_SPECTATOR);
			putint(p, n);
			putint(p, 1);
			sendf(-1, 1, "ri3x", SV_SPECTATOR, n, 1, n);	
		}
		if(clients.length()>1)
		{
			putint(p, SV_RESUME);
			loopv(clients)
			{
				clientinfo *oi = clients[i];
				if(oi->clientnum==n) continue;
				putint(p, oi->clientnum);
				putint(p, oi->state.state);
				putint(p, oi->state.lifesequence);
				putint(p, oi->state.gunselect);
				putint(p, oi->state.maxhealth);
				putint(p, oi->state.frags);
			}
			putint(p, -1);
		}
		if(smode) smode->initclient(ci, p, true);
#ifdef BFRONTIER
		putint(p, SV_SERVMSG);
		s_sprintfd(ver)("#MOD# version Blood Frontier %d", BFRONTIER);
		sendstring(ver, p);
		
		if (motd[0])
		{
			putint(p, SV_SERVMSG);
			sendstring(motd, p);
		}
#endif
		return 1;
	}

	void checkintermission()
	{
		if(minremain>0)
		{
			minremain = gamemillis>=gamelimit ? 0 : (gamelimit - gamemillis + 60000 - 1)/60000;
			sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
			if(!minremain && smode) smode->intermission();
		}
		if(!interm && minremain<=0) interm = gamemillis+10000;
	}

	void startintermission() { gamelimit = min(gamelimit, gamemillis); checkintermission(); }

	void clearevent(clientinfo *ci)
	{
		int n = 1;
		while(n<ci->events.length() && ci->events[n].type==GE_HIT) n++;
		ci->events.remove(0, n);
	}

	void spawnstate(clientinfo *ci)
	{
		gamestate &gs = ci->state;
		gs.spawnstate(gamemode);
#ifdef BFRONTIER
		loopi(SPAWNSTATES)
		{
			if (spawnstates[i] < 0) continue;
			
			switch (i)
			{
				case SPAWN_HEALTH:
				{
					ci->state.health = spawnstates[i];
					break;
				}
				case SPAWN_MAXHEALTH:
				{
					ci->state.maxhealth = spawnstates[i];
					break;
				}
				case SPAWN_ARMOUR:
				{
					ci->state.armour = spawnstates[i];
					break;
				}
				case SPAWN_ARMOURTYPE:
				{
					ci->state.armourtype = spawnstates[i];
					break;
				}
				case SPAWN_GUNSELECT:
				{
					ci->state.gunselect = spawnstates[i];
					break;
				}
				case SPAWN_SG:
				case SPAWN_CG:
				case SPAWN_RL:
				case SPAWN_RIFLE:
				case SPAWN_GL:
				case SPAWN_PISTOL:
				{
					ci->state.ammo[i-SPAWN_SG+1] = spawnstates[i];
					break;
				}
				default:
					break;
			}
		}
#endif
		gs.lifesequence++;
	}

	void sendspawn(clientinfo *ci)
	{
		gamestate &gs = ci->state;
		spawnstate(ci);
		sendf(ci->clientnum, 1, "ri7v", SV_SPAWNSTATE, gs.lifesequence,
			gs.health, gs.maxhealth,
			gs.armour, gs.armourtype,
			gs.gunselect, GUN_PISTOL-GUN_SG+1, &gs.ammo[GUN_SG]);
		gs.lastspawn = gamemillis;
	}

	void dodamage(clientinfo *target, clientinfo *actor, int damage, int gun, const vec &hitpush = vec(0, 0, 0))
	{
		gamestate &ts = target->state;
#ifdef BFRONTIER
		if (smode && !smode->damage(target, actor, damage, gun, hitpush)) return;
		if (!teamdamage && m_teammode && !strcmp(target->team, actor->team)) return;
		damage *= damagescale/100;
#endif
		ts.dodamage(damage);
		sendf(-1, 1, "ri6", SV_DAMAGE, target->clientnum, actor->clientnum, damage, ts.armour, ts.health); 
		if(target!=actor && !hitpush.iszero()) 
		{
			vec v(hitpush);
			if(!v.iszero()) v.normalize();
			sendf(target->clientnum, 1, "ri6", SV_HITPUSH, gun, damage,
				int(v.x*DNF), int(v.y*DNF), int(v.z*DNF));
		}
		if(ts.health<=0)
		{
			if(target!=actor && (!m_teammode || strcmp(target->team, actor->team)))
			{
				actor->state.frags++;

				int friends = 0, enemies = 0; // note: friends also includes the fragger
				if(m_teammode) loopv(clients) if(strcmp(clients[i]->team, actor->team)) enemies++; else friends++;
				else { friends = 1; enemies = clients.length()-1; }
				actor->state.effectiveness += friends/float(max(enemies, 1));
			}
			else actor->state.frags--;
			sendf(-1, 1, "ri4", SV_DIED, target->clientnum, actor->clientnum, actor->state.frags);
            target->position.setsizenodelete(0);
			if(smode) smode->died(target, actor);
			ts.state = CS_DEAD;
			ts.lastdeath = gamemillis;
			// don't issue respawn yet until DEATHMILLIS has elapsed
			// ts.respawn();
		}
	}

	void processevent(clientinfo *ci, suicideevent &e)
	{
		gamestate &gs = ci->state;
		if(gs.state!=CS_ALIVE) return;
		gs.frags--;
		sendf(-1, 1, "ri4", SV_DIED, ci->clientnum, ci->clientnum, gs.frags);
        ci->position.setsizenodelete(0);
		if(smode) smode->died(ci, NULL);
		gs.state = CS_DEAD;
		gs.respawn();
	}

	void processevent(clientinfo *ci, explodeevent &e)
	{
		gamestate &gs = ci->state;
		switch(e.gun)
		{
			case GUN_RL:
				if(gs.rockets<1) return;
				gs.rockets--;
				break;

			case GUN_GL:
				if(gs.grenades<1) return;
				gs.grenades--;
				break;

			default:
				return;
		}
		for(int i = 1; i<ci->events.length() && ci->events[i].type==GE_HIT; i++)
		{
			hitevent &h = ci->events[i].hit;
			clientinfo *target = (clientinfo *)getinfo(h.target);
            if(!target || target->state.state!=CS_ALIVE || h.lifesequence!=target->state.lifesequence || h.dist<0 || h.dist>RL_DAMRAD) continue;

			int j = 1;
			for(j = 1; j<i; j++) if(ci->events[j].hit.target==h.target) break;
			if(j<i) continue;

#ifdef BFRONTIER
			int damage = getgun(e.gun).damage;
#else
			int damage = guns[e.gun].damage;
#endif
			if(gs.quadmillis) damage *= 4;		
			damage = int(damage*(1-h.dist/RL_DISTSCALE/RL_DAMRAD));
			if(e.gun==GUN_RL && target==ci) damage /= RL_SELFDAMDIV;
			dodamage(target, ci, damage, e.gun, h.dir);
		}
	}
		
	void processevent(clientinfo *ci, shotevent &e)
	{
		gamestate &gs = ci->state;
#ifdef BFRONTIER
		int wait = e.millis - gunvar(gs.gunlast,e.gun);
		if(!gs.isalive(gamemillis) ||
           wait<gunvar(gs.gunwait,e.gun) ||
			e.gun<GUN_FIST || e.gun>GUN_PISTOL ||
			gs.ammo[e.gun]<=0)
			return;
		if(e.gun!=GUN_FIST) gs.ammo[e.gun]--;
		gunvar(gs.gunlast,e.gun) = e.millis; 
		gunvar(gs.gunwait,e.gun) = getgun(e.gun).attackdelay; 
#else
		int wait = e.millis - gs.lastshot;
		if(!gs.isalive(gamemillis) ||
           wait<gs.gunwait ||
			e.gun<GUN_FIST || e.gun>GUN_PISTOL ||
			gs.ammo[e.gun]<=0)
			return;
		if(e.gun!=GUN_FIST) gs.ammo[e.gun]--;
		gs.lastshot = e.millis; 
		gs.gunwait = guns[e.gun].attackdelay; 
#endif
		sendf(-1, 1, "ri9x", SV_SHOTFX, ci->clientnum, e.gun,
				int(e.from[0]*DMF), int(e.from[1]*DMF), int(e.from[2]*DMF),
				int(e.to[0]*DMF), int(e.to[1]*DMF), int(e.to[2]*DMF),
				ci->clientnum);
		switch(e.gun)
		{
			case GUN_RL: gs.rockets = min(gs.rockets+1, 8); break;
			case GUN_GL: gs.grenades = min(gs.grenades+1, 8); break;
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
#ifdef BFRONTIER
					int damage = h.rays*getgun(e.gun).damage;
#else
					int damage = h.rays*guns[e.gun].damage;
#endif
					if(gs.quadmillis) damage *= 4;
					dodamage(target, ci, damage, e.gun, h.dir);
				}
				break;
			}
		}
	}

	void processevent(clientinfo *ci, pickupevent &e)
	{
		gamestate &gs = ci->state;
		if(m_mp(gamemode) && !gs.isalive(gamemillis)) return;
		pickup(e.ent, ci->clientnum);
	}

	void processevents()
	{
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(curtime>0 && ci->state.quadmillis) ci->state.quadmillis = max(ci->state.quadmillis-curtime, 0);
			while(ci->events.length())
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
					// untimed events
					case GE_SUICIDE: processevent(ci, e.suicide); break;
					case GE_PICKUP: processevent(ci, e.pickup); break;
				}
				clearevent(ci);
			}
		}
	}
						 
	void serverupdate(int _lastmillis, int _totalmillis)
	{
		curtime = _lastmillis - lastmillis;
		gamemillis += curtime;
		lastmillis = _lastmillis;
		totalmillis = _totalmillis;

		if(m_demo) readdemo();
		else if(minremain>0)
		{
			processevents();
#ifdef BFRONTIER
			if(curtime) loopv(sents) if(sents[i].spawntime && sents[i].type >= I_SHELLS && sents[i].type <= I_QUAD && powerups[sents[i].type-I_SHELLS])
#else
			if(curtime) loopv(sents) if(sents[i].spawntime) // spawn entities when timer reached
#endif
			{
				int oldtime = sents[i].spawntime;
				sents[i].spawntime -= curtime;
				if(sents[i].spawntime<=0)
				{
					sents[i].spawntime = 0;
					sents[i].spawned = true;
					sendf(-1, 1, "ri2", SV_ITEMSPAWN, i);
				}
				else if(sents[i].spawntime<=10000 && oldtime>10000 && (sents[i].type==I_QUAD || sents[i].type==I_BOOST))
				{
					sendf(-1, 1, "ri2", SV_ANNOUNCE, sents[i].type);
				}
			}
			if(smode) smode->update();
#ifdef BFRONTIER
			if (!m_capture && fraglimit > 0 && minremain > 0)
			{
				vector<scr *> scrs;
				
				loopv(clients)
				{
					clientinfo *ci = clients[i];
					if (m_teammode)
					{
						scradd(scrs, ci->team, ci->state.frags);
					}
					else
					{
						scradd(scrs, ci->name, ci->state.frags);
					}
				}
				
				if (scrs.length() > 0)
				{
					scrs.sort(scrsort);
					if (scrs[0]->val >= fraglimit)
						startintermission();
				}
				loopv(scrs) DELETEP(scrs[i]);
			}
#endif
		}

		while(bannedips.length() && bannedips[0].time-totalmillis>4*60*60000) bannedips.remove(0);
		
		if(masterupdate) 
		{ 
			clientinfo *m = currentmaster>=0 ? (clientinfo *)getinfo(currentmaster) : NULL;
			sendf(-1, 1, "ri3", SV_CURRENTMASTER, currentmaster, m ? m->privilege : 0); 
			masterupdate = false; 
		} 
	
		if((gamemode>1 || (gamemode==0 && hasnonlocalclients())) && gamemillis-curtime>0 && gamemillis/60000!=(gamemillis-curtime)/60000) checkintermission();
		if(interm && gamemillis>interm)
		{
			if(demorecord) enddemorecord();
			interm = 0;
			checkvotes(true);
		}
	}

	bool serveroption(char *arg)
	{
		if(arg[0]=='-') switch(arg[1])
		{
			case 'n': s_strcpy(serverdesc, &arg[2]); return true;
			case 'p': s_strcpy(masterpass, &arg[2]); return true;
			case 'o': if(atoi(&arg[2])) mastermask = (1<<MM_OPEN) | (1<<MM_VETO); return true;
#ifdef BFRONTIER
			case 'M': s_strcpy(motd, &arg[2]); return true;
			case 'G':
			{
				gameid = atoi(&arg[2]);
				if (gameid < 0 || gameid >= GAME_MAX)
				{
					conoutf("invalid game id '%d'", gameid);
					gameid = GAME_BF;
				}
				return true;
			}
#endif
		}
		return false;
	}

	void serverinit()
	{
		smapname[0] = '\0';
		resetitems();
#ifdef BFRONTIER
		resetvars();
#endif
	}
	
	const char *privname(int type)
	{
		switch(type)
		{
			case PRIV_ADMIN: return "admin";
			case PRIV_MASTER: return "master";
			default: return "unknown";
		}
	}

	void setmaster(clientinfo *ci, bool val, const char *pass = "")
	{
		const char *name = "";
		if(val) 
		{
			if(ci->privilege)
			{
				if(!masterpass[0] || !pass[0]==(ci->privilege!=PRIV_ADMIN)) return;
			}
			else if(ci->state.state==CS_SPECTATOR && (!masterpass[0] || !pass[0])) return;
            loopv(clients) if(ci!=clients[i] && clients[i]->privilege)
			{
				if(masterpass[0] && !strcmp(masterpass, pass)) clients[i]->privilege = PRIV_NONE;
				else return;
			}
			if(masterpass[0] && pass[0] && !strcmp(masterpass, pass)) ci->privilege = PRIV_ADMIN;
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
		s_sprintfd(msg)("%s %s %s", colorname(ci), val ? "claimed" : "relinquished", name);
		sendservmsg(msg);
		currentmaster = val ? ci->clientnum : -1;
		masterupdate = true;
	}

	void localconnect(int n)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		ci->clientnum = n;
		ci->local = true;
		clients.add(ci);
	}

	void localdisconnect(int n)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		if(smode && ci->state.state==CS_ALIVE) smode->leavegame(ci);
		clients.removeobj(ci);
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
		if(ci->privilege) setmaster(ci, false);
		if(smode && ci->state.state==CS_ALIVE) smode->leavegame(ci);
		ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed; 
		savescore(ci);
		sendf(-1, 1, "ri2", SV_CDIS, n); 
		clients.removeobj(ci);
#ifdef BFRONTIER
		if(clients.empty())
		{
			bannedips.setsize(0);
			resetvars();
		}
#else
		if(clients.empty()) bannedips.setsize(0); // bans clear when server empties
#endif
		else checkvotes();
	}

#ifdef BFRONTIER
	char *servername() { return "someserver"; }
	int serverinfoport()
	{
		switch (gameid)
		{
			case GAME_BF: return BFRONTIER_SERVINFO_PORT;
			case GAME_SAUER: return SAUERBRATEN_SERVINFO_PORT;
		}
		return 0;
	}
	int serverport()
	{
		switch (gameid)
		{
			case GAME_BF: return BFRONTIER_SERVER_PORT;
			case GAME_SAUER: return SAUERBRATEN_SERVER_PORT;
		}
		return 0;
	}
	char *getdefaultmaster()
	{
		switch (gameid)
		{
			case GAME_BF: return "acord.woop.us/";
			case GAME_SAUER: return "sauerbraten.org/masterserver/";
		}
		return "none";
	}
#else
    char *servername() { return "sauerbratenserver"; }
	int serverinfoport() { return SAUERBRATEN_SERVINFO_PORT; }
	int serverport() { return SAUERBRATEN_SERVER_PORT; }
	char *getdefaultmaster() { return "sauerbraten.org/masterserver/"; } 
#endif

	void serverinforeply(ucharbuf &p)
	{
		putint(p, clients.length());
		putint(p, 5);					// number of attrs following
		putint(p, PROTOCOL_VERSION);	// a // generic attributes, passed back below
		putint(p, gamemode);			// b
		putint(p, minremain);			// c
		putint(p, maxclients);
		putint(p, mastermode);
		sendstring(smapname, p);
		sendstring(serverdesc, p);
	}

#ifdef BFRONTIER
	int serverstat(serverinfo *a)
	{
		if (a->attr.length() > 3 && a->numplayers >= a->attr[3])
		{
			return SSTAT_FULL;
		}
		else if(a->attr.length() > 4) switch(a->attr[4])
		{
			case MM_LOCKED:
			{
				return SSTAT_LOCKED;
			}
			case MM_PRIVATE:
			{
				return SSTAT_PRIVATE;
			}
			default:
			{
				return SSTAT_OPEN;
			}
		}
		return SSTAT_UNKNOWN;
	}

	void serversortreset()
	{
		s_sprintfd(u)("serversort = \"%d %d %d\"", SINFO_STATUS, SINFO_PLAYERS, SINFO_PING);
		executeret(u);
	}

	int servercompare(serverinfo *a, serverinfo *b)
	{
		int ac = 0, bc = 0;
		
		if (a->address.host != ENET_HOST_ANY && a->ping < 999 &&
			a->attr.length() && a->attr[0] == PROTOCOL_VERSION) ac = 1;
		
		if (b->address.host != ENET_HOST_ANY && b->ping < 999 &&
			b->attr.length() && b->attr[0] == PROTOCOL_VERSION) bc = 1;
		
		if(ac > bc) return -1;
		if(ac < bc) return 1;

		#define retcp(c) if (c) { return c; }

		#define retsw(c,d,e) \
			if (c != d) \
			{ \
				if (e) { return c > d ? 1 : -1; } \
				else { return c < d ? 1 : -1; } \
			}

		if (!identexists("serversort")) { serversortreset(); }
		int len = atoi(executeret("listlen $serversort"));

		loopi(len)
		{
			s_sprintfd(s)("at $serversort %d", i);
			
			int style = atoi(executeret(s));
			serverinfo *aa = a, *ab = b;
			
			if (style < 0)
			{
				style = 0-style;
				aa = b;
				ab = a;
			}
			
			switch (style)
			{
				case SINFO_STATUS:
				{
					retsw(serverstat(aa), serverstat(ab), true);
					break;
				}
				case SINFO_HOST:
				{
					retcp(strcmp(aa->name, ab->name));
					break;
				}
				case SINFO_DESC:
				{
					retcp(strcmp(aa->sdesc, ab->sdesc));
					break;
				}
				case SINFO_PING:
				{
					retsw(aa->ping, ab->ping, true);
					break;
				}
				case SINFO_PLAYERS:
				{
					retsw(aa->numplayers, ab->numplayers, false);
					break;
				}
				case SINFO_MAXCLIENTS:
				{
					if (aa->attr.length() > 3) ac = aa->attr[3];
					else ac = 0;
					
					if (ab->attr.length() > 3) bc = ab->attr[3];
					else bc = 0;
	
					retsw(ac, bc, false);
					break;
				}
				case SINFO_MODE:
				{
					if (aa->attr.length() > 1) ac = aa->attr[1];
					else ac = 0;
					
					if (ab->attr.length() > 1) bc = ab->attr[1];
					else bc = 0;
					
					retsw(ac, bc, true);
					break;
				}
				case SINFO_MAP:
				{
					retcp(strcmp(aa->map, ab->map));
					break;
				}
				case SINFO_TIME:
				{
					if (aa->attr.length() > 2) ac = aa->attr[2];
					else ac = 0;
					
					if (ab->attr.length() > 2) bc = ab->attr[2];
					else bc = 0;
					
					retsw(ac, bc, false);
					break;
				}
				default:
					break;
			}
		}
		return strcmp(a->name, b->name);
	}
	
	IVARP(serversplit, 3, 10, INT_MAX-1);

	const char *serverinfogui(g3d_gui *cgui, vector<serverinfo> &servers)
	{
		const char *name = NULL;

		cgui->pushlist(); // h

		loopi(SINFO_MAX)
		{
			cgui->pushlist(); // v
			cgui->pushlist(); // h

			if (i == SINFO_ICON && cgui->button("", 0xFFFFDD, "info") & G3D_UP)
			{
				serversortreset();
			}
			else if (cgui->button(serverinfotypes[i], 0xA0A0A0, NULL) & G3D_UP)
			{
				string st; st[0] = 0;
				bool invert = false;
				
				if (!identexists("serversort")) { serversortreset(); }
				int len = atoi(executeret("listlen $serversort"));
				
				loopk(len)
				{
					s_sprintfd(s)("at $serversort %d", k);
					int n = atoi(executeret(s));
					
					if (abs(n) != i)
					{
						s_sprintfd(t)("%s%d", st[0] ? " " : "", n);
						s_sprintf(st)("%s%s", st[0] ? st : "", t);
					}
					else if (!k) invert = true;
				}
				s_sprintfd(u)("serversort = \"%d%s%s\"",
					invert ? 0-i : i, st[0] ? " " : "", st[0] ? st : "");
				executeret(u);
			}

			cgui->poplist(); // v

			loopvj(servers)
			{
				serverinfo &si = servers[j];
#if 0
				if (j > 0 && !(j%serversplit()))
				{
					cgui->poplist(); // h
					cgui->pushlist(); // v
				}
#endif
				if (si.address.host != ENET_HOST_ANY && si.ping != 999)
				{
					string text;
					
					cgui->pushlist(); // h
	
					switch (i)
					{
						case SINFO_ICON:
						{
							cgui->text("", 0xFFFFDD, "server");
							break;
						}
						case SINFO_STATUS:
						{
							s_sprintf(text)("%s", serverstatustypes[serverstat(&si)]);
							if (cgui->button(text, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						case SINFO_HOST:
						{
							if (cgui->button(si.name, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						case SINFO_DESC:
						{
							s_strncpy(text, si.sdesc, 18);
							if (cgui->button(text, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						case SINFO_PING:
						{
							s_sprintf(text)("%d", si.ping);
							if (cgui->button(text, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						case SINFO_PLAYERS:
						{
							s_sprintf(text)("%d", si.numplayers);
							if (cgui->button(text, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						case SINFO_MAXCLIENTS:
						{
							if (si.attr.length() > 3 && si.attr[3] >= 0) s_sprintf(text)("%d", si.attr[3]);
							else text[0] = 0;

							if (cgui->button(text, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						case SINFO_MODE:
						{
							if (si.attr.length() > 1) s_sprintf(text)("%s", modestr(si.attr[1]));
							else text[0] = 0;
							
							if (cgui->button(text, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						case SINFO_MAP:
						{
							s_strncpy(text, si.map, 18);
							if (cgui->button(text, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						case SINFO_TIME:
						{
							if (si.attr.length() > 2 && si.attr[2] >= 0) s_sprintf(text)("%d %s", si.attr[2], si.attr[2] == 1 ? "min" : "mins");
							else text[0] = 0;
							
							if (cgui->button(text, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						default:
							break;
					}
					
					cgui->text(" ", 0xFFFFDD, NULL);
					
					cgui->poplist(); // v
				}
			}
			cgui->poplist(); // h
		}
		
		cgui->poplist(); // v
		
		return name;
	}
#else
	bool servercompatible(char *name, char *sdec, char *map, int ping, const vector<int> &attr, int np)
	{
		return attr.length() && attr[0]==PROTOCOL_VERSION;
	}

	void serverinfostr(char *buf, const char *name, const char *sdesc, const char *map, int ping, const vector<int> &attr, int np)
	{
		if(attr[0]!=PROTOCOL_VERSION) s_sprintf(buf)("[%s protocol] %s", attr[0]<PROTOCOL_VERSION ? "older" : "newer", name);
		else 
		{
			string numcl;
			if(attr.length()>=4) s_sprintf(numcl)("%d/%d", np, attr[3]);
			else s_sprintf(numcl)("%d", np);
			if(attr.length()>=5) switch(attr[4])
			{
				case MM_LOCKED: s_strcat(numcl, " L"); break;
				case MM_PRIVATE: s_strcat(numcl, " P"); break;
			}
			
			s_sprintf(buf)("%d\t%s\t%s, %s: %s %s", ping, numcl, map[0] ? map : "[unknown]", modestr(attr[1]), name, sdesc);
		}
	}
#endif

	void receivefile(int sender, uchar *data, int len)
	{
		if(gamemode != 1 || len > 1024*1024) return;
		clientinfo *ci = (clientinfo *)getinfo(sender);
		if(ci->state.state==CS_SPECTATOR && !ci->privilege) return;
		if(mapdata) { fclose(mapdata); mapdata = NULL; }
		if(!len) return;
		mapdata = tmpfile();
		if(!mapdata) return;
		fwrite(data, 1, len, mapdata);
		s_sprintfd(msg)("[%s uploaded map to server, \"/getmap\" to receive it]", colorname(ci));
		sendservmsg(msg);
	}

	bool duplicatename(clientinfo *ci, char *name)
	{
		if(!name) name = ci->name;
		loopv(clients) if(clients[i]!=ci && !strcmp(name, clients[i]->name)) return true;
		return false;
	}

	char *colorname(clientinfo *ci, char *name = NULL)
	{
		if(!name) name = ci->name;
		if(name[0] && !duplicatename(ci, name)) return name;
		static string cname;
		s_sprintf(cname)("%s \fs\f5(%d)\fr", name, ci->clientnum);
		return cname;
	}	
#ifdef BFRONTIER
	char *gameident()
	{
		switch (gameid)
		{
			case GAME_BF: return "bfg";
			case GAME_SAUER: return "fps";
		}
		return "unk";
	}
	char *gamepakdir()
	{
		switch (gameid)
		{
			case GAME_BF: return "bf";
			case GAME_SAUER: return "base";
		}
		return "unk";
	}

	char *gamename()
	{
		switch (gameid)
		{
			case GAME_BF: return "Alpha";
			case GAME_SAUER: return "Sauerbraten";
		}
		return "Unknown";
	}

	char *gametitle()
	{
		static char *gname[] = {
			"",							// <
			"Demo Playback",			// -3
			"Singleplayer Classic",		// -2
			"Singleplayer Deathmatch",	// -1
			"Free for All",				// 0
			"Cooperative Edit",			// 1
			"Free for All / Duel",		// 2
			"Team Deathmatch",			// 3
			"Instagib",					// 4
			"Team Instagib",			// 5
			"Efficiency",				// 6
			"Team Efficiency",			// 7
			"Instagib Arena",			// 8
			"Team Instagib Arena",		// 9
			"Tactics Arena",			// 10
			"Team Tactics Arena",		// 11
			"Capture",					// 12
			"Instagib Capture"			// 13
		};
		return gname[gamemode >= -3 && gamemode <= 13 ? gamemode+4 : 0];
	}

	char *defaultmap()
	{
		switch (gameid)
		{
			case GAME_BF: return "usm01";
			case GAME_SAUER: return "metl4";
		}
		return "start";
	}

	bool canload(char *type, int version)
	{
		if (!strcmp(type, gameident())) return true;
		else if (g_bf && !strcmp(type, "fps")) return true;
		return false;
	}

	void servsend(int cn, const char *s, ...)
	{
		s_sprintfdlv(str, s, s);
		sendf(cn, 1, "ris", SV_SERVMSG, str);
	}
	
	void servcmd(clientinfo *ci, char *text, bool say)
	{
		if (say) *text++;
		
		cmdcontext = ci;
		
		scresult[0] = 0;
		char *ret = executeret(text, true);
		if (ret) // our reply is the result()
		{
			servsend(ci->clientnum, "%s", ret);
			delete[] ret;
		}
		
		cmdcontext = NULL;
	}

	void resetvars()
	{
		cmdcontext = NULL;
		
		timelimit = 10;
		fraglimit = 0;
		
		damagescale = 100;
		instakill = 0;
		teamdamage = 1;
		
		loopi(POWERUPS) powerups[i] = true;
		loopi(SPAWNSTATES) spawnstates[i] = -1;
		
		duelmode.duelgame = 0;
	}
	
	bool hasmode(clientinfo *ci, int m)
	{
		if (ci != NULL)
		{
			switch (m)
			{
				case CM_ALL:
				case CM_MASTER:
				{
					if (ci->privilege)
						return true;
					else if (m == CM_MASTER)
						return false;
					loopv(clients) if (clients[i]->privilege && ci->clientnum != clients[i]->clientnum)
						return false;
					return true;
					break;
				}
				case CM_ANY:
				default:
				{
					return true;
					break;
				}
			}
			return true;
		}
		return false;
	}
	
	void servvar(clientinfo *ci, const char *name, int *var, int m, int x, char *val)
	{
		if (*val)
		{
			int a = atoi(val);
			if (a < m || a > x)
			{
				s_sprintf(scresult)("valid range for %s is %d..%d", name, m, x);
			}
			else
			{
				*var = a;
				servsend(-1, "%s set %s = %d", colorname(ci), name, *var);
			}
		}
		else
			s_sprintf(scresult)("%s = %d", name, *var);
	}

	void setversion(clientinfo *ci, char *mod, int version)
	{
		if (version > 0) ci->modver = version;
		s_sprintf(scresult)("\f2Blood Frontier support enabled");
	}
	
	void settime()
	{
		gamelimit = timelimit*60000;
		checkintermission();
	}
	
	void setpowerup(clientinfo *ci, char *pup, char *type)
	{
		if (!m_noitems && !m_capture)
		{
			if (*pup)
			{
				if (!strcasecmp("reset", pup))
				{
					loopi(POWERUPS) powerups[i] = true;
					servsend(-1, "%s reset all powerups", colorname(ci));
				}
				else
				{
					char *pups[POWERUPS] = {
						"shells", "bullets", "rockets", "riflerounds", "grenades", "cartridges",
						"health", "healthboost", "greenarmour", "yellowarmour", "quaddamage"
					};
											
					int x = -1;
					loopi(POWERUPS) if (!strcasecmp(pups[i], pup))
					{
						x = i;
						break;
					}
					
					if (x >= 0)
					{
						if (*type)
						{
							bool ret = true;
							
							if (!strcasecmp("off", type) || !strcasecmp("false", type)) ret = false;
							else if (!strcasecmp("on", type) || !strcasecmp("true", type)) ret = true;
							else ret = atoi(type) ? true : false;
							
							if (ret != powerups[x])
							{
								powerups[x] = ret;
								
								// this is all a nasty hack to stay compatible with pure clients and
								// provide an instantaneous (no map change) result during gameplay
								// the side effect being that the issuer of the command will not see
								// the items unspawn, but they also will not be able to pick them up
								
								loopv(sents)
								{
									server_entity &e = sents[i];
									if (e.type == I_SHELLS+x)
									{
										if (e.spawned)
										{
											Q_INT(ci, SV_ITEMPICKUP);
											Q_INT(ci, i);
										}
										
										e.spawned = false;
										e.spawntime = spawntime(e.type);
									}
								}
								servsend(-1, "%s %s the %s", colorname(ci), powerups[x] ? "enabled" : "disabled", pups[x]);
							}
						}
						else
						{
							s_sprintf(scresult)("powerup %s is currently: %s", pups[x], powerups[x] ? "enabled" : "disabled");
						}
					}
					else
						s_sprintf(scresult)("invalid powerup: %s", pup);
				}
			}
			else
				s_sprintf(scresult)("insufficient parameters");
		}
		else
			s_sprintf(scresult)("not available in this game mode");
	}
	
	void setspawnstate(clientinfo *ci, char *st, char *type)
	{
		if (*st)
		{
			if (!strcasecmp("reset", st))
			{
				loopi(SPAWNSTATES) spawnstates[i] = true;
				servsend(-1, "%s reset all spawnstates", colorname(ci));
			}
			else
			{
				char *sts[SPAWNSTATES] = {
					"health", "maxhealth", "armour", "armourtype", "gunselect",
					"shells", "bullets", "rockets", "riflerounds", "grenades", "cartridges",
					"quaddamage"
				};
										
				int x = -1;
				loopi(SPAWNSTATES) if (!strcasecmp(sts[i], st))
				{
					x = i;
					break;
				}
				
				if (x >= 0)
				{
					if (*type)
					{
						int ret = max(atoi(type), -1);
						
						switch (x)
						{
							case SPAWN_HEALTH:
							case SPAWN_MAXHEALTH:
							case SPAWN_ARMOUR:
							{
								ret = min(ret, 999); // yeah, don't be insane
								break;
							}
							case SPAWN_ARMOURTYPE:
							{
								ret = min(ret, A_YELLOW);
								break;
							}
							case SPAWN_GUNSELECT:
							{
								bool wastext = false;
								loopi(NUMGUNS)
								{
									if (!strcasecmp(getgun(i).name, type))
									{
										ret = i;
										wastext = true;
										break;
									}
								}
								if (!wastext) ret = min(ret, NUMGUNS-1);
							}
							case SPAWN_SG:
							case SPAWN_CG:
							case SPAWN_RL:
							case SPAWN_RIFLE:
							case SPAWN_GL:
							case SPAWN_PISTOL:
							{
								ret = min(ret, getitem(x-SPAWN_SG).max);
								break;
							}
							case SPAWN_QUAD:
							{
								ret = min(ret, 1);
							}
							default:
								break;
						}
						
						if (ret != spawnstates[x])
						{
							spawnstates[x] = ret;
							
							if (spawnstates[x] >= 0)
							{
								switch (x)
								{
									case SPAWN_GUNSELECT: // gunselect
										servsend(-1, "%s set spawnstate %s to %s", colorname(ci), sts[x], getgun(spawnstates[x]).name);
										break;
									default:
										servsend(-1, "%s set spawnstate %s to %d", colorname(ci), sts[x], spawnstates[x]);
										break;
								}
							}
							else
								servsend(-1, "%s reset spawnstate %s to default", colorname(ci), sts[x]);
	
						}
					}
					else
					{
						if (spawnstates[x] >= 0)
						{
							switch (x)
							{
								case SPAWN_GUNSELECT: // gunselect
									s_sprintf(scresult)("spawnstate %s is currently: %s", sts[x], getgun(spawnstates[x]).name);
									break;
								default:
									s_sprintf(scresult)("spawnstate %s is currently: %d", sts[x], spawnstates[x]);
									break;
							}
						}
						else
							s_sprintf(scresult)("spawnstate %s is using defaults", sts[x]);
					}
				}
				else
					s_sprintf(scresult)("invalid spawnstate: %s", st);
			}
		}
		else
			s_sprintf(scresult)("insufficient parameters");
	}

	
	static int scrsort(const scr **a, const scr **b)
	{
		if((*a)->val > (*b)->val) return -1;
		if((*a)->val < (*b)->val) return 1;
		return strcmp((*a)->name, (*b)->name);
	}
	
	void scradd(vector<scr *> &dest, char *name, int amt)
	{
		loopvk(dest)
		{
			if (!strcmp(dest[k]->name, name))
			{
				dest[k]->val += amt;
				return;
			}
		}
		dest.add(new scr(name, amt));
	}
#endif
};
