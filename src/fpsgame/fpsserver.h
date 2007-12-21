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
		int type, attr1, attr2, attr3, attr4, attr5, spawntime;
		char spawned;
	};

    static const int DEATHMILLIS = 300;

	enum { GE_NONE = 0, GE_SHOT, GE_RELOAD, GE_EXPLODE, GE_HIT, GE_SUICIDE, GE_PICKUP };

	struct shotevent
	{
		int type;
        int millis, id;
		int gun;
		float from[3], to[3];
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
		reloadevent reload;
		explodeevent explode;
		hitevent hit;
		suicideevent suicide;
		pickupevent pickup;
	};

    template <int N>
    struct projectilestate
    {
        int projs[N];
        int numprojs;

        projectilestate() : numprojs(0) {}

        void reset() { numprojs = 0; }

        void add(int val)
        {
            if(numprojs>=N) numprojs = 0;
            projs[numprojs++] = val;
        }

        bool remove(int val)
        {
            loopi(numprojs) if(projs[i]==val)
            {
                projs[i] = projs[--numprojs];
                return true;
            }
            return false;
        }
    };

	struct gamestate : fpsstate
	{
		vec o;
		int state;
        projectilestate<8> rockets, grenades;
		int frags;
		int lasttimeplayed, timeplayed;
		float effectiveness;

		gamestate() : state(CS_DEAD) {}

		bool isalive(int gamemillis)
		{
			return state==CS_ALIVE || (state==CS_DEAD && gamemillis - lastdeath <= DEATHMILLIS);
		}

        bool waitexpired(int gamemillis)
        {
			int lasttime = gamemillis - lastshot;
			loopi (NUMGUNS) if (lasttime < gunwait[i]) return false;
			return true;
        }

		void reset()
		{
			if(state!=CS_SPECTATOR) state = CS_DEAD;
			lifesequence = 0;
            rockets.reset();
            grenades.reset();

			timeplayed = 0;
			effectiveness = 0;
			frags = 0;

			respawn();
		}

		void respawn()
		{
			fpsstate::respawn();
			o = vec(-1e10f, -1e10f, -1e10f);
		}
	};

	struct savedscore
	{
		uint ip;
		string name;
		int frags, timeplayed;
		float effectiveness;

		void save(gamestate &gs)
		{
			frags = gs.frags;
			timeplayed = gs.timeplayed;
			effectiveness = gs.effectiveness;
		}

		void restore(gamestate &gs)
		{
			gs.frags = frags;
			gs.timeplayed = timeplayed;
			gs.effectiveness = effectiveness;
		}
	};

	struct clientinfo
	{
		int clientnum;
		string name, team, mapvote;
		int modevote, mutsvote;
		int privilege;
        bool spectator, local, timesync, wantsmaster;
        int gameoffset, lastevent;
		gamestate state;
		vector<gameevent> events;
		vector<uchar> position, messages;
        vector<clientinfo *> targets;

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
            targets.setsizenodelete(0);
            timesync = false;
            lastevent = 0;
		}

		void reset()
		{
			name[0] = team[0] = 0;
			privilege = PRIV_NONE;
            spectator = local = wantsmaster = false;
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

    #define MM_MODE 0xF
    #define MM_AUTOAPPROVE 0x1000
    #define MM_DEFAULT (MM_MODE | MM_AUTOAPPROVE)

	enum { MM_OPEN = 0, MM_VETO, MM_LOCKED, MM_PRIVATE };

	bool notgotitems, notgotbases;		// true when map has changed and waiting for clients to send item
	int gamemode, mutators;
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
        virtual void leavegame(clientinfo *ci, bool disconnecting = false) {}

		virtual void moved(clientinfo *ci, const vec &oldpos, const vec &newpos) {}
		virtual bool canspawn(clientinfo *ci, bool connecting = false, bool tryspawn = false) { return true; }
		virtual void spawned(clientinfo *ci) {}
        virtual int fragvalue(clientinfo *victim, clientinfo *actor)
        {
            if(victim==actor || isteam(victim->team, actor->team)) return -1;
            return 1;
        }
		virtual void died(clientinfo *victim, clientinfo *actor) {}
		virtual void changeteam(clientinfo *ci, const char *oldteam, const char *newteam) {}
		virtual void initclient(clientinfo *ci, ucharbuf &p, bool connecting) {}
		virtual void update() {}
		virtual void reset(bool empty) {}
		virtual void intermission() {}
		virtual bool damage(clientinfo *target, clientinfo *actor, int damage, int gun, const vec &hitpush = vec(0, 0, 0)) { return true; }
	};

	servmode *smode;
	vector<servmode *> smuts;

	#define CAPTURESERV 1
	#include "fpscapture.h"
	#undef CAPTURESERV
	#include "fpsduel.h"

	#define mutate(b) loopvk(smuts) { servmode *mut = smuts[k]; { b; } }

	#define Q_INT(c,n) { if(!c->local) { ucharbuf buf = c->messages.reserve(5); putint(buf, n); c->messages.addbuf(buf); } }
	#define Q_STR(c,text) { if(!c->local) { ucharbuf buf = c->messages.reserve(2*strlen(text)+1); sendstring(text, buf); c->messages.addbuf(buf); } }

	struct scr
	{
		char *name;
		int val;
		scr(char *_n, int _v) : name(_n), val(_v) {}
		~scr() {}
	};
	clientinfo *cmdcontext;
	string scresult, motd;
	fpsserver()
		: notgotitems(true), notgotbases(false),
			gamemode(defaultmode()), mutators(0), interm(0), minremain(0),
			mapreload(false), lastsend(0),
			mastermode(MM_OPEN), mastermask(MM_DEFAULT), currentmaster(-1), masterupdate(false),
			mapdata(NULL), reliablemessages(false),
			demonextmatch(false), demotmp(NULL), demorecord(NULL), demoplayback(NULL), nextplayback(0),
			smode(NULL), capturemode(*this), duelmutator(*this)
	{
		motd[0] = '\0'; serverdesc[0] = '\0'; masterpass[0] = '\0';
		smuts.setsize(0);
	}

	void *newinfo() { return new clientinfo; }
	void deleteinfo(void *ci) { delete (clientinfo *)ci; }

	vector<server_entity> sents;
	vector<savedscore> scores;

	void sendservmsg(const char *s) { sendf(-1, 1, "ris", SV_SERVMSG, s); }

	void resetitems()
	{
		sents.setsize(0);
		//cps.reset();
	}

	int spawntime(int type) { return ((MAXCLIENTS+41)-nonspectators())*100; } // so, range is 4100..30000 ms

	bool pickup(int i, int sender)		 // server side item pickup, acknowledge first client that gets it
	{
		if(minremain<=0 || !sents.inrange(i) || !sents[i].spawned) return false;
		clientinfo *ci = (clientinfo *)getinfo(sender);
		if(!ci || (!ci->local && !ci->state.canpickup(sents[i].type, sents[i].attr1, sents[i].attr2, gamemillis))) return false;
		sents[i].spawned = false;
		sents[i].spawntime = spawntime(sents[i].type);
		sendf(-1, 1, "ri3", SV_ITEMACC, i, sender);
		ci->state.pickup(gamemillis, sents[i].type, sents[i].attr1, sents[i].attr2);
		return true;
	}

	void vote(char *map, int reqmode, int reqmuts, int sender)
	{
		clientinfo *ci = (clientinfo *)getinfo(sender);
        if(!ci || ci->state.state==CS_SPECTATOR && !ci->privilege) return;
		s_strcpy(ci->mapvote, map);
		ci->modevote = reqmode;
		ci->mutsvote = reqmuts;
		if(!ci->mapvote[0]) return;
		if(ci->local || mapreload || (ci->privilege && mastermode>=MM_VETO))
		{
			if(demorecord) enddemorecord();
			if(!ci->local && !mapreload)
			{
				s_sprintfd(msg)("%s forced %s on map %s", privname(ci->privilege), gamename(reqmode, reqmuts), map);
				sendservmsg(msg);
			}
			sendf(-1, 1, "risi2", SV_MAPCHANGE, ci->mapvote, ci->modevote, ci->mutsvote);
			changemap(ci->mapvote, ci->modevote, ci->mutsvote);
		}
		else
		{
			s_sprintfd(msg)("%s suggests %s on map %s (select map to vote)", colorname(ci), gamename(reqmode, reqmuts), map);
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
				if(m_capture(gamemode)) rank = 1;
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
				if(isteam(ci->team, teamnames[i])) continue;
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
		teamscore teamscores[2] = { teamscore("blue"), teamscore("red") };
		const int numteams = sizeof(teamscores)/sizeof(teamscores[0]);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(!ci->team[0]) continue;
			ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
			ci->state.lasttimeplayed = lastmillis;

			loopj(numteams) if(isteam(ci->team, teamscores[j].name))
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
			if(m_capture(gamemode))
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
		if(!m_mp(gamemode) || m_edit(gamemode)) return;

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

        sendservmsg("recording demo");

        demorecord = f;

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
			if(hdr.version!=DEMO_VERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.version<DEMO_VERSION ? "older" : "newer");
			else if(hdr.protocol!=PROTOCOL_VERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.protocol<PROTOCOL_VERSION ? "older" : "newer");
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

	void changemap(const char *s, int mode, int muts)
	{
		if(m_demo(gamemode)) enddemoplayback();
		else enddemorecord();

		mapreload = false;
		gamemode = mode;
		mutators = muts;
		gamemillis = 0;
		minremain = 10; // FIXME
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

		if (m_team(gamemode, mutators)) autoteam();

		// server modes
		if (m_capture(gamemode)) smode = &capturemode;
		else smode = NULL;
		if(smode) smode->reset(false);

		// mutators
		smuts.setsize(0);
		if (m_duel(gamemode, mutators)) smuts.add(&duelmutator);
		mutate(mut->reset(false));

		// and the rest
		if(m_timed(gamemode) && hasnonlocalclients()) sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			ci->mapchange();
			ci->state.lasttimeplayed = lastmillis;
            if(m_mp(gamemode) && ci->state.state!=CS_SPECTATOR) sendspawn(ci);
		}

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
			if(oi->state.state==CS_SPECTATOR && !oi->privilege) continue;
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
		loopv(votes) if(!best || votes[i].count > best->count || (votes[i].count == best->count && rnd(2))) best = &votes[i];
		if(force || (best && best->count > maxvotes/2))
		{
			if(demorecord) enddemorecord();
			if(best && (best->count > (force ? 1 : maxvotes/2)))
			{
				sendservmsg(force ? "vote passed by default" : "vote passed by majority");
				sendf(-1, 1, "risii", SV_MAPCHANGE, best->map, best->mode, best->muts);
				changemap(best->map, best->mode, best->muts);
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
		if(type>=SV_EDITENT && type<=SV_GETMAP && !m_edit(gamemode)) return -1;
		// server only messages
		static int servtypes[] = { SV_INITS2C, SV_MAPRELOAD, SV_SERVMSG, SV_DAMAGE, SV_SHOTFX, SV_DIED, SV_SPAWNSTATE, SV_FORCEDEATH, SV_ARENAWIN, SV_ITEMACC, SV_ITEMSPAWN, SV_TIMEUP, SV_CDIS, SV_CURRENTMASTER, SV_PONG, SV_RESUME, SV_TEAMSCORE, SV_BASEINFO, SV_SENDDEMOLIST, SV_SENDDEMO, SV_DEMOPLAYBACK, SV_SENDMAP, SV_CLIENT };
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
#if 0
					if(ci->state.armourtype==A_GREEN && ci->state.armour>0) f |= 1<<4;
					if(ci->state.armourtype==A_YELLOW && ci->state.armour>0) f |= 1<<5;
					if(ci->state.quadmillis) f |= 1<<6;
					if(ci->state.maxhealth>100) f |= ((ci->state.maxhealth-100)/itemstats[I_BOOST-I_SHELLS].add)<<7;
#endif
					curmsg = p.length();
					ucharbuf buf = ci->position.reserve(4);
					putuint(buf, f);
					ci->position.addbuf(buf);
				}
				if(smode && ci->state.state==CS_ALIVE) smode->moved(ci, oldpos, ci->state.o);
				mutate(mut->moved(ci, oldpos, ci->state.o));
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
				mutate({
					if (val) mut->leavegame(ci);
					else mut->entergame(ci);
				});
				ci->state.state = val ? CS_EDITING : CS_ALIVE;
				if(val)
				{
					ci->events.setsizenodelete(0);
                    ci->state.rockets.reset();
                    ci->state.grenades.reset();
				}
				QUEUE_MSG;
				break;
			}

			case SV_TRYSPAWN:
                if (ci->state.state!=CS_DEAD || ci->state.lastspawn>=0)
                {
					int nospawn = 0;
					if (smode && !smode->canspawn(ci, false, true)) { nospawn++; }
					mutate({
						if (!mut->canspawn(ci, false, true)) { nospawn++; }
					});
					if (nospawn) break;
                }

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

			case SV_SPAWN:
			{
				int ls = getint(p), gunselect = getint(p);
				if((ci->state.state!=CS_ALIVE && ci->state.state!=CS_DEAD) || ls!=ci->state.lifesequence || ci->state.lastspawn<0) break;
				ci->state.lastspawn = -1;
				ci->state.state = CS_ALIVE;
				ci->state.gunselect = gunselect;
				if(smode) smode->spawned(ci);
				mutate(mut->spawned(ci));
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
                #define seteventmillis(event) \
                { \
                    event.id = getint(p); \
                    if(!ci->timesync || (ci->events.length()==1 && ci->state.waitexpired(gamemillis))) \
                    { \
                        ci->timesync = true; \
                        ci->gameoffset = gamemillis - event.id; \
                        event.millis = gamemillis; \
                    } \
                    else event.millis = ci->gameoffset + event.id; \
				}
                seteventmillis(shot.shot);
				shot.shot.gun = getint(p);
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

			case SV_RELOAD:
			{
				gameevent &reload = ci->addevent();
				reload.type = GE_RELOAD;
                seteventmillis(reload.reload);
				reload.reload.gun = getint(p);
                break;
			}

			case SV_EXPLODE:
			{
				gameevent &exp = ci->addevent();
				exp.type = GE_EXPLODE;
                seteventmillis(exp.explode);
				exp.explode.gun = getint(p);
                exp.explode.id = getint(p);
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
			{
				QUEUE_MSG;
                int action = getint(p);
                QUEUE_INT(action);
				getstring(text, p); // filtering is chosen by the client
                QUEUE_STR(text);
				break;
			}

			case SV_COMMAND:
			{
				getstring(text, p);
				filtertext(text, text);
				//servcmd(ci, text, false);
				break;
			}

			case SV_INITC2S:
			{
				QUEUE_MSG;
				bool connected = !ci->name[0];
				getstring(text, p);
				//filtertext(text, text, false, MAXNAMELEN);
				if(!text[0]) s_strcpy(text, "unnamed");
				QUEUE_STR(text);
				s_strncpy(ci->name, text, MAXNAMELEN+1);
				if(!ci->local && connected)
				{
					savedscore &sc = findscore(ci, false);
					if(&sc)
					{
						sc.restore(ci->state);
						sendf(-1, 1, "ri7", SV_RESUME, sender, ci->state.state, ci->state.lifesequence, ci->state.gunselect, sc.frags, -1);
					}
				}
				getstring(text, p);
				//filtertext(text, text, false, MAXTEAMLEN);
				if(!ci->local && connected && m_team(gamemode, mutators))
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
				mutate(mut->changeteam(ci, ci->team, text));
				s_strncpy(ci->team, text, MAXTEAMLEN+1);
				QUEUE_MSG;
				break;
			}

			case SV_MAPVOTE:
			case SV_MAPCHANGE:
			{
				getstring(text, p);
				filtertext(text, text);
				int reqmode = getint(p), reqmuts = getint(p);
				if(type!=SV_MAPVOTE && !mapreload) break;
				if(!ci->local && !m_mp(reqmode)) reqmode = G_DEATHMATCH;
				vote(text, reqmode, reqmuts, sender);
				break;
			}

			case SV_ITEMLIST:
			{
				int n;
				while((n = getint(p))!=-1)
				{
					server_entity se = { getint(p), getint(p), getint(p), getint(p), getint(p), getint(p), false, 0 },
					sn = { 0, 0, 0, 0, 0, 0, false, 0 };
					if(notgotitems)
					{
						while(sents.length()<n) sents.add(sn);
						sents.add(se);
						sents[n].spawned = true;
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
				getint(p);
				getstring(text, p);
				getstring(text, p);
				QUEUE_MSG;
				break;

			case SV_BASES:
				if(smode==&capturemode) capturemode.parsebases(p);
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
                    if(smode) smode->leavegame(spinfo);
					mutate(mut->leavegame(spinfo));
					spinfo->state.state = CS_SPECTATOR;
				}
				else if(spinfo->state.state==CS_SPECTATOR && !val)
				{
					spinfo->state.state = CS_DEAD;
					spinfo->state.respawn();
					int nospawn = 0;
					if (smode && !smode->canspawn(spinfo)) { nospawn++; }
					mutate({
						if (!mut->canspawn(spinfo)) { nospawn++; }
					});
					if (!nospawn) sendspawn(spinfo);
				}
				break;
			}

			case SV_SETTEAM:
			{
				int who = getint(p);
				getstring(text, p);
				//filtertext(text, text, false, MAXTEAMLEN);
				if(!ci->privilege || who<0 || who>=getnumclients()) break;
				clientinfo *wi = (clientinfo *)getinfo(who);
				if(!wi) break;
				if (wi->state.state == CS_ALIVE && strcmp(wi->team, text))
				{
					if (smode) smode->changeteam(wi, wi->team, text);
					mutate(mut->changeteam(wi, wi->team, text));
				}
				s_strncpy(wi->team, text, MAXTEAMLEN+1);
				sendf(sender, 1, "riis", SV_SETTEAM, who, text);
				QUEUE_INT(SV_SETTEAM);
				QUEUE_INT(who);
				QUEUE_STR(text);
				break;
			}

			case SV_FORCEINTERMISSION:
				if(m_sp(gamemode)) startintermission();
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
				if(m_demo(gamemode)) enddemoplayback();
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

			case SV_EDITVAR:
			{
				getstring(text, p);
				getint(p);
				break;
			}

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
					mutate(mut->reset(true));
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
                if(mastermask&MM_AUTOAPPROVE) break;
                clientinfo *candidate = (clientinfo *)getinfo(mn);
                if(!candidate || !candidate->wantsmaster || mn==sender) break;// || getclientip(mn)==getclientip(sender)) break;
                setmaster(candidate, true, "", true);
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
		int hasmap = smapname[0] ? (sents.length() ? 1 : 2) : 0;
		putint(p, SV_INITS2C);
		putint(p, n);
		putint(p, PROTOCOL_VERSION);
		putint(p, hasmap);
		if(hasmap)
		{
			putint(p, SV_MAPCHANGE);
			sendstring(smapname, p);
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
		}
		if(ci && (m_demo(gamemode) || m_mp(gamemode)) && ci->state.state!=CS_SPECTATOR)
		{
			int nospawn = 0;
            if(smode && !smode->canspawn(ci, true)) { nospawn++; }
            mutate(
				if (!mut->canspawn(ci, true)) { nospawn++; }
			);

			if (nospawn)
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
				putint(p, gs.gunselect);
				loopi(NUMGUNS) putint(p, gs.ammo[i]);
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
				putint(p, oi->state.frags);
			}
			putint(p, -1);
		}
		if(smode) smode->initclient(ci, p, true);
		mutate(mut->initclient(ci, p, true));

		//putint(p, SV_COMMAND);
		//s_sprintfd(ver)("version %d", BFRONTIER);
		//sendstring(ver, p);

		if (motd[0])
		{
			putint(p, SV_SERVMSG);
			sendstring(motd, p);
		}
		return 1;
	}

	void checkintermission()
	{
		if(minremain>0)
		{
			minremain = gamemillis>=gamelimit ? 0 : (gamelimit - gamemillis + 60000 - 1)/60000;
			sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
			if (!minremain)
			{
				if (smode) smode->intermission();
				mutate(mut->intermission());
			}
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
		gs.spawnstate(gamemode, mutators);
		gs.lifesequence++;
	}

	void sendspawn(clientinfo *ci)
	{
		gamestate &gs = ci->state;
		spawnstate(ci);
		sendf(ci->clientnum, 1, "ri4v", SV_SPAWNSTATE, gs.lifesequence, gs.health, gs.gunselect, NUMGUNS, &gs.ammo[0]);
		gs.lastspawn = gamemillis;
	}

	void dodamage(clientinfo *target, clientinfo *actor, int damage, int gun, const vec &hitpush = vec(0, 0, 0))
	{
		gamestate &ts = target->state;
		if (smode && !smode->damage(target, actor, damage, gun, hitpush)) { return; }
		mutate(if (!mut->damage(target, actor, damage, gun, hitpush)) { return; });
		//if (!teamdamage && m_team(gamemode, mutators) && isteam(target->team, actor->team)) return;
		//damage *= damagescale/100;
		ts.dodamage(damage, gamemillis);
		sendf(-1, 1, "ri7i3", SV_DAMAGE, target->clientnum, actor->clientnum, gun, damage, ts.health, ts.lastpain, int(hitpush.x*DNF), int(hitpush.y*DNF), int(hitpush.z*DNF));
		if(ts.health<=0)
		{
            int fragvalue = smode ? smode->fragvalue(target, actor) : (target == actor || isteam(target->team, actor->team) ? -1 : 1);
            actor->state.frags += fragvalue;
            if(fragvalue > 0)
			{
				int friends = 0, enemies = 0; // note: friends also includes the fragger
				if(m_team(gamemode, mutators)) loopv(clients) if(!isteam(clients[i]->team, actor->team)) enemies++; else friends++;
				else { friends = 1; enemies = clients.length()-1; }
                actor->state.effectiveness += fragvalue*friends/float(max(enemies, 1));
			}
			sendf(-1, 1, "ri4", SV_DIED, target->clientnum, actor->clientnum, actor->state.frags);
            target->position.setsizenodelete(0);
			if(smode) smode->died(target, actor);
			mutate(mut->died(target, actor));
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
        ci->state.frags += smode ? smode->fragvalue(ci, ci) : -1;
		sendf(-1, 1, "ri4", SV_DIED, ci->clientnum, ci->clientnum, gs.frags);
        ci->position.setsizenodelete(0);
		if(smode) smode->died(ci, NULL);
		mutate(mut->died(ci, NULL));
		gs.state = CS_DEAD;
		gs.respawn();
	}

	void processevent(clientinfo *ci, explodeevent &e)
	{
		gamestate &gs = ci->state;
		switch(e.gun)
		{
			case GUN_RL:
                if(!gs.rockets.remove(e.id)) return;
				break;

			case GUN_GL:
                if(!gs.grenades.remove(e.id)) return;
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

			int damage = int(guns[e.gun].damage*(1-h.dist/RL_DISTSCALE/RL_DAMRAD));
			dodamage(target, ci, damage, e.gun, h.dir);
		}
	}

	void processevent(clientinfo *ci, shotevent &e)
	{
		gamestate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !gs.canshoot(e.gun, e.millis)) { return; }
		if (guns[e.gun].max) gs.ammo[e.gun]--;
		gs.lastshot = gs.gunlast[e.gun] = e.millis;
		gs.gunwait[e.gun] = guns[e.gun].adelay;
		sendf(-1, 1, "ri9x", SV_SHOTFX, ci->clientnum, e.gun,
				int(e.from[0]*DMF), int(e.from[1]*DMF), int(e.from[2]*DMF),
				int(e.to[0]*DMF), int(e.to[1]*DMF), int(e.to[2]*DMF),
				ci->clientnum);
		switch(e.gun)
		{
            case GUN_RL: gs.rockets.add(e.id); break;
            case GUN_GL: gs.grenades.add(e.id); break;
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
					int damage = h.rays*guns[e.gun].damage;
					dodamage(target, ci, damage, e.gun, h.dir);
				}
				break;
			}
		}
	}

	void processevent(clientinfo *ci, reloadevent &e)
	{
		gamestate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !gs.canreload(e.gun, e.millis)) { return; }
		gs.pickup(e.millis, WEAPON, e.gun, guns[e.gun].add);
		sendf(-1, 1, "ri4", SV_RELOAD, ci->clientnum, e.gun, gs.ammo[e.gun]);
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
			if (ci->state.state == CS_ALIVE)
			{
				int lastpain = gamemillis-ci->state.lastpain,
					lastregen = gamemillis-ci->state.lastregen;

				if (!m_insta(gamemode, mutators) && ci->state.health < 100 && lastpain >= 5000 && lastregen >= 2500)
				{
					int health = ci->state.health - (ci->state.health % 10);
					ci->state.health = min(health + 10, 100);
					ci->state.lastregen = gamemillis;
					sendf(-1, 1, "ri4", SV_REGEN, ci->clientnum, ci->state.health, ci->state.lastregen);
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
					case GE_RELOAD: processevent(ci, e.reload); break;
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

		if(m_demo(gamemode)) readdemo();
		else if(minremain>0)
		{
			processevents();
			if(curtime) loopv(sents) if(sents[i].spawntime && sents[i].type == WEAPON)
			{
				sents[i].spawntime -= curtime;
				if(sents[i].spawntime<=0)
				{
					sents[i].spawntime = 0;
					sents[i].spawned = true;
					sendf(-1, 1, "ri2", SV_ITEMSPAWN, i);
				}
			}
			if(smode) smode->update();
			mutate(mut->update());

			/*
			if (!m_capture(gamemode) && fraglimit > 0 && minremain > 0)
			{
				vector<scr *> scrs;

				loopv(clients)
				{
					clientinfo *ci = clients[i];
					if (m_team(gamemode, mutators))
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
			*/
		}

		while(bannedips.length() && bannedips[0].time-totalmillis>4*60*60000) bannedips.remove(0);

		if(masterupdate)
		{
			clientinfo *m = currentmaster>=0 ? (clientinfo *)getinfo(currentmaster) : NULL;
			sendf(-1, 1, "ri3", SV_CURRENTMASTER, currentmaster, m ? m->privilege : 0);
			masterupdate = false;
		}

		if ((m_timed(gamemode) && hasnonlocalclients()) && gamemillis-curtime>0 && gamemillis/60000!=(gamemillis-curtime)/60000)
			checkintermission();

		if (interm && gamemillis > interm)
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
			case 'N': s_strcpy(serverdesc, &arg[2]); return true;
			case 'P': s_strcpy(masterpass, &arg[2]); return true;
			case 'O': if(atoi(&arg[2])) mastermask = (1<<MM_OPEN) | (1<<MM_VETO); return true;
			case 'M': s_strcpy(motd, &arg[2]); return true;
		}
		return false;
	}

	void serverinit()
	{
		smapname[0] = '\0';
		resetitems();
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
                s_sprintfd(msg)("%s wants master. Type \"/approvemaster %d\" to approve.", colorname(ci), ci->clientnum);
                sendservmsg(msg);
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
        s_sprintfd(msg)("%s %s %s", colorname(ci), val ? (approved ? "approved for" : "claimed") : "relinquished", name);
		sendservmsg(msg);
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
		if(ci->privilege) setmaster(ci, false);
		if(smode) smode->leavegame(ci, true);
		mutate(mut->leavegame(ci));
		ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		savescore(ci);
		sendf(-1, 1, "ri2", SV_CDIS, n);
		clients.removeobj(ci);
		if(clients.empty())
		{
			bannedips.setsize(0);
		}
		else checkvotes();
	}

	char *servername() { return "bloodfrontierserver"; }
	int serverinfoport()
	{
		return BFRONTIER_SERVINFO_PORT;
	}
	int serverport()
	{
		return BFRONTIER_SERVER_PORT;
	}
	char *getdefaultmaster()
	{
		return "acord.woop.us/";
	}

	void serverinforeply(ucharbuf &p)
	{
		putint(p, clients.length());
		putint(p, 5);					// number of attrs following
		putint(p, PROTOCOL_VERSION);	// a // generic attributes, passed back below
		putint(p, gamemode);			// b
		putint(p, mutators);			// c
		putint(p, minremain);			// d
		putint(p, maxclients);
		putint(p, mastermode);
		sendstring(smapname, p);
		sendstring(serverdesc, p);
	}

	IVARP(serversplit, 3, 10, INT_MAX-1);

	int serverstat(serverinfo *a)
	{
		if (a->attr.length() > 4 && a->numplayers >= a->attr[4])
		{
			return SSTAT_FULL;
		}
		else if(a->attr.length() > 5) switch(a->attr[5])
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
					if (aa->attr.length() > 4) ac = aa->attr[4];
					else ac = 0;

					if (ab->attr.length() > 4) bc = ab->attr[4];
					else bc = 0;

					retsw(ac, bc, false);
					break;
				}
				case SINFO_GAME:
				{
					if (aa->attr.length() > 1) ac = aa->attr[1];
					else ac = 0;

					if (ab->attr.length() > 1) bc = ab->attr[1];
					else bc = 0;

					retsw(ac, bc, true);

					if (aa->attr.length() > 2) ac = aa->attr[2];
					else ac = 0;

					if (ab->attr.length() > 2) bc = ab->attr[2];
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
					if (aa->attr.length() > 3) ac = aa->attr[3];
					else ac = 0;

					if (ab->attr.length() > 3) bc = ab->attr[3];
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
							if (si.attr.length() > 4 && si.attr[4] >= 0)
								s_sprintf(text)("%d", si.attr[4]);
							else text[0] = 0;
							if (cgui->button(text, 0xFFFFDD, NULL) & G3D_UP) name = si.name;
							break;
						}
						case SINFO_GAME:
						{
							if (si.attr.length() > 2)
								s_sprintf(text)("%s", gamename(si.attr[1], si.attr[2]));
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
							if (si.attr.length() > 3 && si.attr[3] >= 0)
								s_sprintf(text)("%d %s", si.attr[3], si.attr[3] == 1 ? "min" : "mins");
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

	void receivefile(int sender, uchar *data, int len)
	{
		if(!m_edit(gamemode) || len > 1024*1024) return;
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
		s_sprintf(cname)("%s \fs\f5(%d)\fS", name, ci->clientnum);
		return cname;
	}
	
    char *gameid() { return "bfa"; }
    char *gamename(int mode, int muts)
    {
    	static string gname;
    	gname[0] = 0;
    	int gmode = mode >= 0 && mode < G_MAX ? mode : G_DEATHMATCH;
		loopi(G_M_NUM)
		{
			if ((gametype[gmode].mutators & mutstype[i].type) && (muts & mutstype[i].type))
			{
				s_sprintfd(name)("%s%s%s", *gname ? gname : "", *gname ? "-" : "", mutstype[i].name);
				s_strcpy(gname, name);
			}
		}
		s_sprintfd(mname)("%s%s%s", *gname ? gname : "", *gname ? " " : "", gametype[gmode].name);
		s_strcpy(gname, mname);
		return gname;
    }
	char *defaultmap() { return "usm01"; }
	int defaultmode() { return G_DEATHMATCH; }

	bool canload(char *type)
	{
		if (strcmp(type, gameid()) == 0) return true;
		if (strcmp(type, "fps") == 0 || strcmp(type, "bfg") == 0) return true;
		return false;
	}

	void servsend(int cn, const char *s, ...)
	{
		s_sprintfdlv(str, s, s);
		sendf(cn, 1, "ris", SV_SERVMSG, str);
	}

	/*
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

	bool hasmode(clientinfo *ci, int m)
	{
		if (ci != NULL)
		{
			switch (m)
			{
				case PRIV_HOST:
				case PRIV_ADMIN:
				case PRIV_MASTER:
				{
					if (ci->privilege >= m) return true;
					else if (m == PRIV_HOST) return false;
					loopv(clients) if (clients[i]->privilege > ci->privilege) return false;
					break;
				}
				case PRIV_NONE:
				default:
					break;
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

	void settime()
	{
		gamelimit = timelimit*60000;
		checkintermission();
	}

	*/
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
};
