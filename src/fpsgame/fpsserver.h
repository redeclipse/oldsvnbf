#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define _dup	dup
#define _fileno fileno
#endif

struct GAMESERVER : igameserver
{
	struct server_entity			// server side version of "entity" type
	{
		int type, attr1, attr2, attr3, attr4, attr5, spawntime;
		char spawned;
	};

    static const int DEATHMILLIS = 300;

	enum { GE_NONE = 0, GE_SHOT, GE_SWITCH, GE_RELOAD, GE_EXPLODE, GE_HIT, GE_SUICIDE, GE_USE };

	struct shotevent
	{
		int type;
        int millis, id;
		int gun;
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
		int type;
	};

	struct useevent
	{
		int type;
		int millis, id;
		int ent;
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
        projectilestate rockets, grenades, flames;
		int frags;
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
            rockets.reset();
            grenades.reset();
            flames.reset();

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
			name[0] = team[0] = 0;
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

	bool notgotitems, notgotbases;		// true when map has changed and waiting for clients to send item
	int gamemode, mutators;
	int gamemillis, gamelimit;

	string serverdesc;
	string smapname;
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
            if(victim==actor || isteam(victim->team, actor->team)) return -1;
            return 1;
        }
		virtual void died(clientinfo *victim, clientinfo *actor) {}
		virtual void changeteam(clientinfo *ci, const char *oldteam, const char *newteam) {}
		virtual void initclient(clientinfo *ci, ucharbuf &p, bool connecting) {}
		virtual void update() {}
		virtual void reset(bool empty) {}
		virtual void intermission() {}
		virtual bool damage(clientinfo *target, clientinfo *actor, int damage, int gun, int flags, const vec &hitpush = vec(0, 0, 0)) { return true; }
	};

	servmode *smode;
	vector<servmode *> smuts;

	#define CAPTURESERV 1
	#include "capture.h"
	#undef CAPTURESERV

    #define CTFSERV 1
    #include "ctf.h"
    #undef CTFSERV

	#include "duel.h"

	#define mutate(b) loopvk(smuts) { servmode *mut = smuts[k]; { b; } }

	string  motd;
	GAMESERVER()
		: notgotitems(true), notgotbases(false),
			gamemode(defaultmode()), mutators(0), interm(0), minremain(10),
			mapreload(false), lastsend(0),
			mastermode(MM_VETO), mastermask(MM_DEFAULT), currentmaster(-1), masterupdate(false),
			mapdata(NULL), reliablemessages(false),
			demonextmatch(false), demotmp(NULL), demorecord(NULL), demoplayback(NULL), nextplayback(0),
			smode(NULL), capturemode(*this), ctfmode(*this), duelmutator(*this)
	{
		motd[0] = serverdesc[0] = masterpass[0] = '\0';
		smuts.setsize(0);
#ifndef STANDALONE
		CCOMMAND(gameid, "", (GAMESERVER *self), result(self->gameid()));
		CCOMMAND(gamever, "", (GAMESERVER *self), intret(self->gamever()));
		CCOMMAND(gamename, "ii", (GAMESERVER *self, int *g, int *m), result(self->gamename(*g, *m)));
		CCOMMAND(defaultmap, "", (GAMESERVER *self), result(self->defaultmap()));
		CCOMMAND(defaultmode, "", (GAMESERVER *self), intret(self->defaultmode()));
#endif
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

	void vote(char *map, int reqmode, int reqmuts, int sender)
	{
		clientinfo *ci = (clientinfo *)getinfo(sender);
        if(!ci || (ci->state.state==CS_SPECTATOR && !ci->privilege) || !m_game(reqmode)) return;
		s_strcpy(ci->mapvote, map);
		ci->modevote = reqmode;
		ci->mutsvote = reqmuts|gametype[reqmode].implied;
		if(!ci->mapvote[0]) return;
		if(mapreload || (ci->privilege && mastermode>=MM_VETO))
		{
			if(demorecord) enddemorecord();
			if(!mapreload)
			{
				s_sprintfd(msg)("%s forced %s on map %s", privname(ci->privilege), gamename(ci->modevote, ci->mutsvote), map);
				sendservmsg(msg);
			}
			sendf(-1, 1, "risi2", SV_MAPCHANGE, ci->mapvote, ci->modevote, ci->mutsvote);
			changemap(ci->mapvote, ci->modevote, ci->mutsvote);
		}
		else
		{
			s_sprintfd(msg)("%s suggests %s on map %s (select map to vote)", colorname(ci), gamename(ci->modevote, ci->mutsvote), map);
			sendservmsg(msg);
			checkvotes();
		}
	}

	void autoteam()
	{
		vector<clientinfo *> team[TEAM_MAX];
		float teamrank[TEAM_MAX] = { 0, 0, 0, 0 };
		int numteams = (m_team(gamemode, mutators) && m_ttwo(gamemode, mutators) ? TEAM_MAX : TEAM_MAX/2);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			int worstteam = -1;
			float worstrank = 1e16f;
			loopj(numteams) if (teamrank[i] < worstrank) { worstrank = teamrank[j]; worstteam = j; }
			team[worstteam].add(ci);
			float rank = ci->state.effectiveness/max(ci->state.timeplayed, 1);
			teamrank[worstteam] += (!m_capture(gamemode) && !m_ctf(gamemode) && rank != 0 ? rank : 1);
			ci->state.lasttimeplayed = -1;
		}
		loopi(numteams)
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
		teamscore teamscores[TEAM_MAX] = { teamscore(teamnames[0]), teamscore(teamnames[1]), teamscore(teamnames[2]), teamscore(teamnames[3]) };
		int numteams = (m_team(gamemode, mutators) && m_ttwo(gamemode, mutators) ? TEAM_MAX : TEAM_MAX/2);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(!ci->team[0]) continue;
			ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
			ci->state.lasttimeplayed = lastmillis;

			loopj(numteams) if(isteam(ci->team, teamscores[j].name))
			{
				teamscore &ts = teamscores[j];
				float rank = ci->state.effectiveness/max(ci->state.timeplayed, 1);
				ts.rank += rank != 0 ? rank : 1;
				ts.clients++;
				break;
			}
		}
		teamscore *worst = &teamscores[numteams-1];
		loopi(numteams-1)
		{
			teamscore &ts = teamscores[i];
			if(m_capture(gamemode) || m_ctf(gamemode))
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
			endianswap(&hdr.gamever, sizeof(int), 1);
			if(hdr.version!=DEMO_VERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.version<DEMO_VERSION ? "older" : "newer");
			else if(hdr.gamever!=GAMEVERSION) s_sprintf(msg)("demo \"%s\" requires an %s version of Blood Frontier", file, hdr.gamever<GAMEVERSION ? "older" : "newer");
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
        else if(m_ctf(gamemode)) smode = &ctfmode;
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

		if(!m_duel(gamemode, mutators))
			sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, "fight!");
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
			if((oi->state.state==CS_SPECTATOR && !oi->privilege) || oi->state.ownernum >= 0) continue;
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
#if 0
        // other message types can get sent by accident if a master forces spectator on someone, so disabling this case for now and checking for spectator state in message handlers
        // spectators can only connect and talk
		static int spectypes[] = { SV_INITC2S, SV_POS, SV_TEXT, SV_PING, SV_CLIENTPING, SV_GETMAP, SV_SETMASTER };
		if(ci && ci->state.state==CS_SPECTATOR && !ci->privilege)
		{
			loopi(sizeof(spectypes)/sizeof(int)) if(type == spectypes[i]) return type;
			return -1;
		}
#endif
		// only allow edit messages in coop-edit mode
		if(type>=SV_EDITENT && type<=SV_GETMAP && !m_edit(gamemode)) return -1;
		// server only messages
		static int servtypes[] = { SV_INITS2C, SV_MAPRELOAD, SV_SERVMSG, SV_DAMAGE, SV_SHOTFX, SV_DIED, SV_SPAWNSTATE, SV_FORCEDEATH, SV_ITEMACC, SV_ITEMSPAWN, SV_TIMEUP, SV_CDIS, SV_CURRENTMASTER, SV_PONG, SV_RESUME, SV_TEAMSCORE, SV_BASEINFO, SV_ANNOUNCE, SV_SENDDEMOLIST, SV_SENDDEMO, SV_DEMOPLAYBACK, SV_SENDMAP, SV_DROPFLAG, SV_SCOREFLAG, SV_RETURNFLAG, SV_CLIENT };
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
		if(millis<33) return false;
		bool flush = buildworldstate();
		lastsend += millis - (millis%33);
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
		int type;
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
        while((curmsg = p.length()) < p.maxlen) switch(type = checktype(getint(p), ci))
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
                if(havecn && (cp->state.state==CS_ALIVE || cp->state.state==CS_EDITING))
				{
					cp->position.setsizenodelete(0);
					while(curmsg<p.length()) cp->position.add(p.buf[curmsg++]);
				}
				uint f = getuint(p);
                if(havecn && (cp->state.state==CS_ALIVE || cp->state.state==CS_EDITING))
				{
					f &= 0xF;
					curmsg = p.length();
					ucharbuf buf = cp->position.reserve(4);
					putuint(buf, f);
					cp->position.addbuf(buf);
				}
				if(havecn)
				{
					if(smode && cp->state.state==CS_ALIVE) smode->moved(cp, oldpos, cp->state.o);
					mutate(mut->moved(cp, oldpos, cp->state.o));
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
                    ci->state.flames.reset();
				}
				QUEUE_MSG;
				break;
			}

			case SV_TRYSPAWN:
            {
				int lcn = getint(p);
				clientinfo *cp = (clientinfo *)getinfo(lcn);
				if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) cp = ci;
                if (cp->state.state!=CS_DEAD || cp->state.lastspawn>=0)
                {
					int nospawn = 0;
					if (smode && !smode->canspawn(cp, false, true)) { nospawn++; }
					mutate(if (!mut->canspawn(cp, false, true)) { nospawn++; });
					if (nospawn) break;
                }

				if(cp->state.lastdeath) cp->state.respawn();
				sendspawn(cp);
				break;
            }

			case SV_GUNSELECT:
			{
                int lcn = getint(p), id = getint(p), gun = getint(p);
				clientinfo *cp = (clientinfo *)getinfo(lcn);
				if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) cp = ci;
				gameevent &gunsel = cp->addevent();
				gunsel.type = GE_SWITCH;
				gunsel.gunsel.id = id;
				gunsel.gunsel.gun = gun;
                seteventmillis(gunsel.gunsel, ci->state.canswitch(gunsel.gunsel.gun, gamemillis));
                break;
			}

			case SV_SPAWN:
			{
				int lcn = getint(p), ls = getint(p), gunselect = getint(p);
				clientinfo *cp = (clientinfo *)getinfo(lcn);
				if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) cp = ci;
				if((cp->state.state!=CS_ALIVE && cp->state.state!=CS_DEAD) || ls!=cp->state.lifesequence || cp->state.lastspawn<0) break;
				cp->state.lastspawn = -1;
				cp->state.state = CS_ALIVE;
				cp->state.gunselect = gunselect;
				if(smode) smode->spawned(cp);
				mutate(mut->spawned(cp));
                QUEUE_MSG;
				break;
			}

			case SV_SUICIDE:
			{
                int lcn = getint(p);
				clientinfo *cp = (clientinfo *)getinfo(lcn);
				if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) cp = ci;
				gameevent &suicide = cp->addevent();
				suicide.type = GE_SUICIDE;
				break;
			}

			case SV_SHOOT:
			{
                int lcn = getint(p);
				clientinfo *cp = (clientinfo *)getinfo(lcn);
				if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) cp = ci;
				gameevent &shot = cp->addevent();
				shot.type = GE_SHOT;
				shot.shot.id = getint(p);
				shot.shot.gun = getint(p);
                seteventmillis(shot.shot, ci->state.canshoot(shot.shot.gun, gamemillis));
				loopk(3) shot.shot.from[k] = getint(p)/DMF;
				loopk(3) shot.shot.to[k] = getint(p)/DMF;
				int hits = getint(p);
				loopk(hits)
				{
					gameevent &hit = cp->addevent();
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
				if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) cp = ci;
				gameevent &reload = cp->addevent();
				reload.type = GE_RELOAD;
				reload.reload.id = id;
				reload.reload.gun = gun;
                seteventmillis(reload.reload, ci->state.canreload(reload.reload.gun, gamemillis));
                break;
			}

			case SV_EXPLODE:
			{
                int lcn = getint(p);
				clientinfo *cp = (clientinfo *)getinfo(lcn);
				if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) cp = ci;
				gameevent &exp = cp->addevent();
				exp.type = GE_EXPLODE;
				exp.explode.id = getint(p);
                seteventmillis(exp.explode, true);
				exp.explode.gun = getint(p);
                exp.explode.id = getint(p);
				int hits = getint(p);
				loopk(hits)
				{
					gameevent &hit = cp->addevent();
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
                int lcn = getint(p), id = getint(p), ent = getint(p);
				clientinfo *cp = (clientinfo *)getinfo(lcn);
				if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) cp = ci;
				gameevent &use = cp->addevent();
				use.type = GE_USE;
				use.use.ent = ent;
				use.use.id = id;
                seteventmillis(use.use, sents.inrange(ent) ? ci->state.canuse(sents[ent].type, sents[ent].attr1, sents[ent].attr2, gamemillis) : false);
				break;
			}

			case SV_TEXT:
            {
                int lcn = getint(p), flags = getint(p);
                getstring(text, p);
				clientinfo *cp = (clientinfo *)getinfo(lcn);
				if(!cp || (cp->clientnum!=ci->clientnum && cp->state.ownernum!=ci->clientnum)) cp = ci;
                if(cp->state.state == CS_SPECTATOR || (flags&SAY_TEAM && !cp->team[0])) break;
                loopv(clients)
                {
                    clientinfo *t = clients[i];
                    if(t == cp || t->state.state == CS_SPECTATOR || (flags&SAY_TEAM && strcmp(cp->team, t->team))) continue;
                    sendf(t->clientnum, 1, "ri3s", SV_TEXT, cp->clientnum, flags, text);
                }
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
				if(!text[0]) s_strcpy(text, "unnamed");
				QUEUE_STR(text);
				s_strncpy(ci->name, text, MAXNAMELEN+1);
				if(connected)
				{
					savedscore &sc = findscore(ci, false);
					if(&sc)
					{
						sc.restore(ci->state);
						sendf(-1, 1, "ri7", SV_RESUME, sender, ci->state.state, ci->state.lifesequence, ci->state.gunselect, sc.frags, -1);
					}
				}
				getstring(text, p);
				if(connected && m_team(gamemode, mutators))
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
				if(connected && !m_duel(gamemode, mutators))
					sendf(sender, 1, "ri2s", SV_ANNOUNCE, S_V_FIGHT, "fight!");
				break;
			}

			case SV_MAPVOTE:
			case SV_MAPCHANGE:
			{
				getstring(text, p);
				filtertext(text, text);
				int reqmode = getint(p), reqmuts = getint(p);
				if(type!=SV_MAPVOTE && !mapreload) break;
				if(!m_mp(reqmode)) reqmode = G_DEATHMATCH;
				vote(text, reqmode, reqmuts, sender);
				break;
			}

			case SV_ITEMLIST:
			{
                bool commit = ci->state.state!=CS_SPECTATOR && notgotitems;
				int n;
				while((n = getint(p))!=-1)
				{
					server_entity se = { getint(p), getint(p), getint(p), getint(p), getint(p), getint(p), false, 0 },
						sn = { 0, 0, 0, 0, 0, 0, false, 0 };

					if (commit)
					{
						while(sents.length()<n) sents.add(sn);
						sents.add(se);
						sents[n].spawned = true;
					}
				}
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
                if(ci->state.state!=CS_SPECTATOR && smode==&capturemode) capturemode.parsebases(p);
				break;

            case SV_TAKEFLAG:
            {
                int flag = getint(p);
                if(ci->state.state!=CS_SPECTATOR && smode==&ctfmode) ctfmode.takeflag(ci, flag);
                break;
            }

            case SV_INITFLAGS:
                if(ci->state.state!=CS_SPECTATOR && smode==&ctfmode) ctfmode.parseflags(p);
                break;

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
                if(!ci->privilege && (ci->state.state==CS_SPECTATOR || spectator!=sender)) break;
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
				if(ci->privilege<PRIV_ADMIN) break;
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
                if(ci->state.state==CS_SPECTATOR) break;
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
                if(mastermask&MM_AUTOAPPROVE || ci->state.state==CS_SPECTATOR) break;
                clientinfo *candidate = (clientinfo *)getinfo(mn);
                if(!candidate || !candidate->wantsmaster || mn==sender || getclientip(mn)==getclientip(sender)) break;
                setmaster(candidate, true, "", true);
                break;
            }

			case SV_ADDBOT:
			{
                if(ci->state.state==CS_SPECTATOR && !ci->privilege) break;
				addbot(ci);
				break;
			}

			case SV_DELBOT:
			{
				int lcn = getint(p);
				clientinfo *cp = (clientinfo *)getinfo(lcn);
				if (cp && cp->state.ownernum >= 0 && cp->state.ownernum == ci->clientnum)
					deletebot(cp);
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
	}

	void addbot(clientinfo *ci)
	{
		int lcn = addclient(ST_REMOTE);
		clientinfo *cp = (clientinfo *)getinfo(lcn);
		cp->clientnum = lcn;
		cp->state.ownernum = ci->clientnum;
		cp->state.state = CS_DEAD;
		clients.add(cp);
		cp->state.lasttimeplayed = lastmillis;
		s_strncpy(cp->name, "bot", MAXNAMELEN);

		//if(smode) smode->initclient(cp, p, true);
		//mutate(mut->initclient(cp, p, true));
		string text;
		const char *worst = chooseworstteam(text);
		if(worst) s_strncpy(text, worst, MAXTEAMLEN);
		else s_strncpy(text, teamnames[0], MAXTEAMLEN);
		if(smode && cp->state.state==CS_ALIVE && strcmp(cp->team, text)) smode->changeteam(cp, cp->team, text);
		mutate(mut->changeteam(cp, cp->team, text));
		s_strncpy(cp->team, text, MAXTEAMLEN);
		sendf(-1, 1, "ri3ss", SV_INITBOT, cp->state.ownernum, cp->clientnum, cp->name, cp->team);

		if(m_demo(gamemode) || m_mp(gamemode))
		{
			int nospawn = 0;
			if(smode && !smode->canspawn(cp, true)) { nospawn++; }
			mutate(if (!mut->canspawn(cp, true)) { nospawn++; });

			if (nospawn)
			{
				cp->state.state = CS_DEAD;
				sendf(-1, 1, "ri2", SV_FORCEDEATH, cp->clientnum);
			}
			else
			{
				sendspawn(cp);
			}
		}
	}

	void deletebot(clientinfo *ci)
	{
		int lcn = ci->clientnum;
		if(smode) smode->leavegame(ci, true);
		mutate(mut->leavegame(ci));
		ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		savescore(ci);
		sendf(-1, 1, "ri2", SV_CDIS, lcn);
		clients.removeobj(ci);
		delclient(lcn);
	}

	int welcomepacket(ucharbuf &p, int n, ENetPacket *packet)
	{
		clientinfo *ci = (clientinfo *)getinfo(n);
		int hasmap = smapname[0] ? (sents.length() ? 1 : 2) : 0;
		putint(p, SV_INITS2C);
		putint(p, n);
		putint(p, GAMEVERSION);
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
				putint(p, ci->clientnum);
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
			loopv(clients)
			{
				clientinfo *cp = clients[i];
				if (cp && cp->state.ownernum >= 0)
				{
					putint(p, SV_INITBOT);
					putint(p, cp->state.ownernum);
					putint(p, cp->clientnum);
					sendstring(cp->name, p);
					sendstring(cp->team, p);
				}
			}

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

		if (motd[0])
		{
			putint(p, SV_SERVMSG);
			sendstring(motd, p);
		}
		return 1;
	}

	void checkintermission()
	{
		if (clients.length())
		{
			if(minremain > 0)
			{
				minremain = gamemillis >= gamelimit ? 0 : (gamelimit - gamemillis + 60000 - 1)/60000;
				sendf(-1, 1, "ri2", SV_TIMEUP, minremain);
				if (!minremain)
				{
					if (smode) smode->intermission();
					mutate(mut->intermission());
				}
				else if(minremain == 1)
					sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_ONEMINUTE, "only one minute left of play!");
			}
			if (!interm && minremain <= 0) interm = gamemillis+10000;
		}
	}

	void startintermission()
	{
		gamelimit = min(gamelimit, gamemillis);
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
		gs.spawnstate(gamemode, mutators);
		gs.lifesequence++;
	}

	void sendspawn(clientinfo *ci)
	{
		gamestate &gs = ci->state;
		spawnstate(ci);
		int own = ci->state.ownernum >= 0 ? ci->state.ownernum : ci->clientnum;
		sendf(own, 1, "ri5v", SV_SPAWNSTATE, ci->clientnum, gs.lifesequence, gs.health, gs.gunselect, NUMGUNS, &gs.ammo[0]);
		gs.lastspawn = gamemillis;
	}

	void dodamage(clientinfo *target, clientinfo *actor, int damage, int gun, int flags, const vec &hitpush = vec(0, 0, 0))
	{
		gamestate &ts = target->state;
		if(flags&HIT_LEGS) damage = damage/2;
		else if (flags&HIT_HEAD) damage = damage*2;
		if (smode && !smode->damage(target, actor, damage, gun, flags, hitpush)) { return; }
		mutate(if (!mut->damage(target, actor, damage, gun, flags, hitpush)) { return; });
		//if (!teamdamage && m_team(gamemode, mutators) && isteam(target->team, actor->team)) return;
		//damage *= damagescale/100;
		ts.dodamage(damage, gamemillis);
		sendf(-1, 1, "ri8i3", SV_DAMAGE, target->clientnum, actor->clientnum, gun, flags, damage, ts.health, ts.lastpain, int(hitpush.x*DNF), int(hitpush.y*DNF), int(hitpush.z*DNF));
		if(ts.health<=0)
		{
            int fragvalue = smode ? smode->fragvalue(target, actor) : (target == actor || (m_team(gamemode, mutators) && isteam(target->team, actor->team)) ? -1 : 1);
            actor->state.frags += fragvalue;
            if(fragvalue > 0)
			{
				int friends = 0, enemies = 0; // note: friends also includes the fragger
				if(m_team(gamemode, mutators)) loopv(clients) if(!isteam(clients[i]->team, actor->team)) enemies++; else friends++;
				else { friends = 1; enemies = clients.length()-1; }
                actor->state.effectiveness += fragvalue*friends/float(max(enemies, 1));
			}
			sendf(-1, 1, "ri7", SV_DIED, target->clientnum, actor->clientnum, actor->state.frags, gun, flags, damage);
            target->position.setsizenodelete(0);
			if(smode) smode->died(target, actor);
			mutate(mut->died(target, actor));
			ts.state = CS_DEAD;
			ts.lastdeath = gamemillis;
			// don't issue respawn yet until DEATHMILLIS has elapsed
			// ts.respawn();
			if(fragvalue > 0)
			{
				actor->state.spree++;
				switch (actor->state.spree)
				{
					case 5:
					{
						s_sprintfd(spr)("%s is wreaking carnage!", actor->name);
						sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_SPREE1, spr);
						break;
					}
					case 10:
					{
						s_sprintfd(spr)("%s is slaughtering the opposition!", actor->name);
						sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_SPREE2, spr);
						break;
					}
					case 25:
					{
						s_sprintfd(spr)("%s is on a massacre!", actor->name);
						sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_SPREE3, spr);
						break;
					}
					case 50:
					{
						s_sprintfd(spr)("%s is creating a bloodbath!", actor->name);
						sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_SPREE4, spr);
						break;
					}
					default:
					{
						if(flags&HIT_HEAD)
						{
							s_sprintfd(spr)("%s scored a headshot!", actor->name);
							sendf(-1, 1, "ri2s", SV_ANNOUNCE, S_V_HEADSHOT, spr);
						}
						break;
					}
				}
			}
		}
	}

	void processevent(clientinfo *ci, suicideevent &e)
	{
		gamestate &gs = ci->state;
		if(gs.state!=CS_ALIVE) return;
        ci->state.frags += smode ? smode->fragvalue(ci, ci) : -1;
		sendf(-1, 1, "ri7", SV_DIED, ci->clientnum, ci->clientnum, gs.frags, -1, 0, ci->state.health);
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
            if(!target || target->state.state!=CS_ALIVE || h.lifesequence!=target->state.lifesequence || h.dist<0 || h.dist>guntype[e.gun].radius) continue;

			int j = 1;
			for(j = 1; j<i; j++) if(ci->events[j].hit.target==h.target) break;
			if(j<i) continue;

			int damage = int(guntype[e.gun].damage*(1-h.dist/guntype[e.gun].scale/guntype[e.gun].radius));
			dodamage(target, ci, damage, e.gun, h.flags, h.dir);
		}
	}

	void processevent(clientinfo *ci, shotevent &e)
	{
		gamestate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !gs.canshoot(e.gun, e.millis)) { return; }
		if (guntype[e.gun].max) gs.ammo[e.gun]--;
		gs.gunstate[e.gun] = GUNSTATE_SHOOT;
		gs.lastshot = gs.gunlast[e.gun] = e.millis;
		gs.gunwait[e.gun] = guntype[e.gun].adelay;
		sendf(-1, 1, "ri9x", SV_SHOTFX, ci->clientnum, e.gun,
				int(e.from[0]*DMF), int(e.from[1]*DMF), int(e.from[2]*DMF),
				int(e.to[0]*DMF), int(e.to[1]*DMF), int(e.to[2]*DMF),
				ci->clientnum);
		switch(e.gun)
		{
            case GUN_RL: gs.rockets.add(e.id); break;
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
		if(!gs.isalive(gamemillis) || !gs.canswitch(e.gun, e.millis)) { return; }
		gs.gunswitch(e.gun, e.millis);
		sendf(-1, 1, "ri4", SV_GUNSELECT, ci->clientnum, e.gun, e.millis);
	}

	void processevent(clientinfo *ci, reloadevent &e)
	{
		gamestate &gs = ci->state;
		if(!gs.isalive(gamemillis) || !gs.canreload(e.gun, e.millis)) { return; }
		gs.useitem(e.millis, WEAPON, e.gun, guntype[e.gun].add);
		sendf(-1, 1, "ri5", SV_RELOAD, ci->clientnum, e.gun, e.millis, gs.ammo[e.gun]);
	}

	void processevent(clientinfo *ci, useevent &e)
	{
		gamestate &gs = ci->state;
		if(minremain<=0 || !gs.isalive(gamemillis) || !sents.inrange(e.ent) || !sents[e.ent].spawned || !gs.canuse(sents[e.ent].type, sents[e.ent].attr1, sents[e.ent].attr2, e.millis))
		{
#if 0
			const char *msg = (minremain<=0 ? "time up" :
				(!gs.isalive(gamemillis) ? "not alive" :
				(!sents.inrange(e.ent) ? "entity not in range" :
				(!sents[e.ent].spawned ? "entity not spawned" :
				(!gs.canuse(sents[e.ent].type, sents[e.ent].attr1, sents[e.ent].attr2, e.millis) ? "cannot use entity" :
					"unknown reason")))));
			servsend(ci->clientnum, "cannot pickup %d (%d): %s", e.ent, e.millis, msg);
#endif
			return;
		}
		sents[e.ent].spawned = false;
		sents[e.ent].spawntime = spawntime(sents[e.ent].type);
		ci->state.useitem(e.millis, sents[e.ent].type, sents[e.ent].attr1, sents[e.ent].attr2);
		sendf(-1, 1, "ri4", SV_ITEMACC, ci->clientnum, e.millis, e.ent);
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
		else if(minremain>0)
		{
			processevents();
			if(curtime && !m_insta(gamemode, mutators))
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
			mutate(mut->update());
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
		loopvrev(clients)
			if(clients[i]!=ci && clients[i]->state.ownernum==ci->clientnum)
				deletebot(clients[i]);
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

	void serverinforeply(ucharbuf &p)
	{
		int numplayers = 0;
		loopv(clients) if(clients[i] && clients[i]->state.ownernum < 0) numplayers++;
		putint(p, numplayers);
		putint(p, 6);					// number of attrs following
		putint(p, GAMEVERSION);	// 1 // generic attributes, passed back below
		putint(p, gamemode);			// 2
		putint(p, mutators);			// 3
		putint(p, minremain);			// 4
		putint(p, maxclients);			// 5
		putint(p, mastermode);			// 6
		sendstring(smapname, p);
		sendstring(serverdesc, p);
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
		s_sprintfd(msg)("[%s uploaded map to server, \"/getmap\" to receive it]", colorname(ci));
		sendservmsg(msg);
	}

	bool duplicatename(clientinfo *ci, char *name)
	{
		if(!name) name = ci->name;
		loopv(clients) if(clients[i]!=ci && !strcmp(name, clients[i]->name)) return true;
		return false;
	}

	const char *colorname(clientinfo *ci, char *name = NULL)
	{
		if(!name) name = ci->name;
		if(name[0] && !duplicatename(ci, name)) return name;
		static string cname;
		s_sprintf(cname)("%s \fs\f5(%d)\fS", name, ci->clientnum);
		return cname;
	}

    const char *gameid() { return GAMEID; }
    int gamever() { return GAMEVERSION; }
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
	const char *defaultmap() { return "usm01"; }
	int defaultmode() { return G_DEATHMATCH; }

	bool canload(char *type)
	{
		if (strcmp(type, GAMEID) == 0) return true;
		if (strcmp(type, "fps") == 0 || strcmp(type, "bfg") == 0) return true;
		return false;
	}

	void servsend(int cn, const char *s, ...)
	{
		s_sprintfdlv(str, s, s);
		sendf(cn, 1, "ris", SV_SERVMSG, str);
	}
};
