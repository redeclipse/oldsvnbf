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
            attrs.shrink(0);
            kin.shrink(0);
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
        void reset() { projs.shrink(0); }
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
        int score, spree, crits, rewards, flags, teamkills, shotdamage, damage;
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
            frags = spree = crits = rewards = flags = deaths = teamkills = shotdamage = damage = 0;
            fraglog.shrink(0); fragmillis.shrink(0); cpnodes.shrink(0);
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
        int points, score, frags, spree, crits, rewards, flags, timeplayed, deaths, teamkills, shotdamage, damage;

        void save(servstate &gs)
        {
            points = gs.points;
            score = gs.score;
            frags = gs.frags;
            spree = gs.spree;
            crits = gs.crits;
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
            gs.crits = crits;
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
        int clientnum, connectmillis, sessionid, overflow, ping, team;
        string name, mapvote;
        int modevote, mutsvote, lastvote;
        int privilege;
        bool connected, local, timesync, online, wantsmap, connectauth;
        int gameoffset, lastevent;
        servstate state;
        vector<gameevent *> events;
        vector<uchar> position, messages;
        int posoff, poslen, msgoff, msglen;
        uint authreq, authlevel;
        string authname;
        string clientmap;
        int mapcrc;
        bool warned;
        ENetPacket *clipboard;
        int lastclipboard, needclipboard;

        clientinfo() : clipboard(NULL) { reset(); }
        ~clientinfo() { events.deletecontents(); cleanclipboard(); }

        void addevent(gameevent *e)
        {
            if(state.state==CS_SPECTATOR || events.length()>100) delete e;
            else events.add(e);
        }

        void mapchange(bool change = true)
        {
            mapvote[0] = 0;
            state.reset(change);
            events.deletecontents();
            overflow = 0;
            timesync = wantsmap = false;
            lastevent = gameoffset = lastvote = 0;
            team = TEAM_NEUTRAL;
            clientmap[0] = '\0';
            mapcrc = 0;
            warned = false;
        }

        void cleanclipboard(bool fullclean = true)
        {
            if(clipboard) { if(--clipboard->referenceCount <= 0) enet_packet_destroy(clipboard); clipboard = NULL; }
            if(fullclean) lastclipboard = 0;
        }

        void reset()
        {
            ping = 0;
            name[0] = 0;
            privilege = PRIV_NONE;
            connected = local = online = wantsmap = connectauth = false;
            authreq = 0;
            authlevel = -1;
            position.setsize(0);
            messages.setsize(0);
            needclipboard = 0;
            cleanclipboard();
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

    namespace aiman {
        bool dorefresh = false;
        extern int findaiclient(int exclude = -1);
        extern bool addai(int type, int ent, int skill);
        extern void deleteai(clientinfo *ci);
        extern bool delai(int type);
        extern void removeai(clientinfo *ci, bool complete = false);
        extern bool reassignai(int exclude = -1);
        extern void checkskills();
        extern void clearai(bool all = true);
        extern void checkai();
    }

    bool hasgameinfo = false;
    int gamemode = -1, mutators = -1, gamemillis = 0, gamelimit = 0;
    string smapname;
    int interm = 0, timeremaining = -1, oldtimelimit = -1;
    bool maprequest = false;
    enet_uint32 lastsend = 0;
    int mastermode = MM_OPEN;
    bool masterupdate = false, mapsending = false, shouldcheckvotes = false;
    stream *mapdata[3] = { NULL, NULL, NULL };
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
        void reset(int n = 0) { id = n; ents.shrink(0); }
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

    SVAR(0, serverpass, "");
    SVAR(0, adminpass, "");

    int version[2] = {0};
    ICOMMAND(0, setversion, "ii", (int *a, int *b), version[0] = *a; version[1] = *b);

    int mastermask()
    {
        switch(GAME(serveropen))
        {
            case 0: default: return MM_FREESERV; break;
            case 1: return MM_OPENSERV; break;
            case 2: return MM_COOPSERV; break;
            case 3: return MM_VETOSERV; break;
        }
        return 0;
    }

    bool returningfiremod = false;

    bool eastereggs()
    {
        if(!GAME(alloweastereggs)) return false;
        time_t ct = time(NULL); // current time
        struct tm *lt = localtime(&ct);
        int month = lt->tm_mon+1, mday = lt->tm_mday; //, day = lt->tm_wday+1
        if(month == 4 && mday == 1) returningfiremod = true;
        return true;
    }

    #define setmod(a,b) \
    { \
        if(a != b) \
        { \
            setvar(#a, b, true); \
            sendf(-1, 1, "ri2ss", SV_COMMAND, -1, &((const char *)#a)[3], #b); \
        } \
    }

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
        if(eastereggs())
        {
            if(returningfiremod) setmod(sv_returningfire, 1);
        }
    }

    const char *pickmap(const char *suggest, int mode, int muts)
    {
        const char *map = GAME(defaultmap);
        if(!map || !*map) map = choosemap(suggest, mode, muts, m_campaign(gamemode) ? 1 : GAME(maprotate));
        return map;
    }

    void setpause(bool on = false)
    {
        if(sv_gamepaused != (on ? 1 : 0))
        {
            setvar("sv_gamepaused", on ? 1 : 0, true);
            sendf(-1, 1, "ri2ss", SV_COMMAND, -1, "gamepaused", on ? "1" : "0");
        }
    }

    void cleanup(bool init = false)
    {
        setpause(false);
        if(GAME(resetmmonend)) mastermode = MM_OPEN;
        if(GAME(resetbansonend)) loopv(bans) if(bans[i].time >= 0) bans.remove(i--);
        if(GAME(resetvarsonend) || init) resetgamevars(true);
        changemap();
    }

    void start() { cleanup(true); }

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
            defformatstring(s)(" [\fs%s%d\fS]", ci->state.aitype >= 0 ? "\fc" : "\fw", ci->clientnum);
            concatstring(cname, s);
        }
        concatstring(cname, "\fS");
        return cname;
    }

    bool haspriv(clientinfo *ci, int flag, const char *msg = NULL)
    {
        if(ci->local || ci->privilege >= flag) return true;
        else if(mastermask()&MM_AUTOAPPROVE && flag <= PRIV_MASTER && !numclients(ci->clientnum)) return true;
        else if(msg && *msg)
            srvmsgf(ci->clientnum, "\fraccess denied, you need to be %s to %s", privname(flag), msg);
        return false;
    }

    bool cmppriv(clientinfo *ci, clientinfo *cp, const char *msg = NULL)
    {
        mkstring(str); if(msg && *msg) formatstring(str)("%s %s", msg, colorname(cp));
        if(haspriv(ci, cp->local ? PRIV_MAX : cp->privilege, str)) return true;
        return false;
    }

    const char *gameid() { return GAMEID; }
    ICOMMAND(0, gameid, "", (), result(gameid()));
    int getver(int n)
    {
        switch(n)
        {
            case 0: return ENG_VERSION;
            case 1: return GAMEVERSION;
            case 2: case 3: return version[n%2];
        }
        return 0;
    }
    ICOMMAND(0, getversion, "i", (int *a), intret(getver(*a)));
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
    ICOMMAND(0, gamename, "iii", (int *g, int *m), result(gamename(*g, *m)));

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
    ICOMMAND(0, mutscheck, "iii", (int *g, int *m, int *t), intret(mutscheck(*g, *m, *t)));

    void changemode(int *mode, int *muts)
    {
        if(*mode < 0)
        {
            if(GAME(defaultmode) >= G_START) *mode = GAME(defaultmode);
            else *mode = rnd(G_RAND)+G_FIGHT;
        }
        if(*muts < 0)
        {
            if(GAME(defaultmuts) >= G_M_NONE) *muts = GAME(defaultmuts);
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
        int rotate = force ? force : GAME(maprotate);
        if(rotate)
        {
            const char *maplist = GAME(mainmaps);
            if(m_campaign(mode)) maplist = GAME(campaignmaps);
            else if(m_duel(mode, muts)) maplist = GAME(duelmaps);
            else if(m_stf(mode)) maplist = GAME(stfmaps);
            else if(m_ctf(mode)) maplist = m_multi(mode, muts) ? GAME(mctfmaps) : GAME(ctfmaps);
            else if(m_trial(mode)) maplist = GAME(trialmaps);
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
        timeremaining = 0;
        gamelimit = min(gamelimit, gamemillis);
        if(smode) smode->intermission();
        mutate(smuts, mut->intermission());
        maprequest = false;
        interm = gamemillis+GAME(intermlimit);
        sendf(-1, 1, "ri2", SV_TICK, 0);
    }

    void checklimits()
    {
        if(m_fight(gamemode))
        {
            if(m_trial(gamemode))
            {
                loopv(clients) if(clients[i]->state.cpmillis < 0 && gamemillis+clients[i]->state.cpmillis >= GAME(triallimit))
                {
                    sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_GUIBACK, CON_MESG, "\fctime trial wait period has timed out");
                    startintermission();
                    return;
                }
            }
            if(GAME(timelimit) != oldtimelimit || (gamemillis-curtime>0 && gamemillis/1000!=(gamemillis-curtime)/1000))
            {
                if(GAME(timelimit) != oldtimelimit)
                {
                    if(GAME(timelimit)) gamelimit += (GAME(timelimit)-oldtimelimit)*60000;
                    oldtimelimit = GAME(timelimit);
                }
                if(timeremaining)
                {
                    if(GAME(timelimit))
                    {
                        if(gamemillis >= gamelimit) timeremaining = 0;
                        else timeremaining = (gamelimit-gamemillis+1000-1)/1000;
                    }
                    else timeremaining = -1;
                    if(!timeremaining)
                    {
                        sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_GUIBACK, CON_MESG, "\fctime limit has been reached");
                        startintermission();
                        return; // bail
                    }
                    else
                    {
                        sendf(-1, 1, "ri2", SV_TICK, timeremaining);
                        if(timeremaining == 60) sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_V_ONEMINUTE, CON_MESG, "\fcone minute remains");
                    }
                }
            }
            if(GAME(fraglimit) && !m_flag(gamemode) && !m_trial(gamemode))
            {
                if(m_team(gamemode, mutators))
                {
                    int teamscores[TEAM_NUM] = {0};
                    loopv(clients) if(clients[i]->state.aitype < AI_START && clients[i]->team >= TEAM_FIRST && isteam(gamemode, mutators, clients[i]->team, TEAM_FIRST))
                        teamscores[clients[i]->team-TEAM_FIRST] += clients[i]->state.frags;
                    int best = -1;
                    loopi(TEAM_NUM) if(best < 0 || teamscores[i] > teamscores[best])
                        best = i;
                    if(best >= 0 && teamscores[best] >= GAME(fraglimit))
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
                    if(best >= 0 && clients[best]->state.frags >= GAME(fraglimit))
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
        if(!sents.inrange(i) || m_noitems(gamemode, mutators)) return false;
        switch(sents[i].type)
        {
            case WEAPON:
                if(!isweap(sents[i].attrs[0]) || (WEAP(sents[i].attrs[0], allowed) < (m_insta(gamemode, mutators) ? 2 : 1))) return false;
                if((sents[i].attrs[3] > 0 && sents[i].attrs[3] != triggerid) || !m_check(sents[i].attrs[2], gamemode)) return false;
                if(m_arena(gamemode, mutators) && sents[i].attrs[0] < WEAP_ITEM) return false;
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
            if(sents[i].type == WEAPON && w_carry(w_attr(gamemode, sents[i].attrs[0], sweap), sweap))
            {
                loopvk(clients)
                {
                    clientinfo *ci = clients[k];
                    if(ci->state.dropped.projs.find(i) >= 0 && (!spawned || (timeit && gamemillis < sents[i].millis))) return true;
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
        static vector<int> items; items.setsize(0);
        loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM && hasitem(i))
        {
            sents[i].millis += GAME(itemspawndelay);
            switch(GAME(itemspawnstyle))
            {
                case 1: items.add(i); break;
                case 2:
                {
                    int delay = sents[i].type == WEAPON && isweap(sents[i].attrs[0]) ? w_spawn(sents[i].attrs[0]) : GAME(itemspawntime);
                    if(delay > 1) sents[i].millis += delay/2+rnd(delay/2);
                    break;
                }
                default: break;
            }
        }
        if(!items.empty())
        {
            items.sort(sortitems);
            loopv(items) sents[items[i]].millis += GAME(itemspawndelay)*i;
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
            static vector<int> valid; valid.setsize(0);
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
            ents.shrink(0);
            cycle.shrink(0);
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
            if(totalspawns && GAME(spawnrotate))
            {
                int cycle = -1, team = m_fight(gamemode) && m_team(gamemode, mutators) && !spawns[ci->team].ents.empty() ? ci->team : TEAM_NEUTRAL;
                if(!spawns[team].ents.empty())
                {
                    switch(GAME(spawnrotate))
                    {
                        case 2:
                        {
                            int num = 0, lowest = -1;
                            loopv(spawns[team].cycle) if(lowest < 0 || spawns[team].cycle[i] < lowest) lowest = spawns[team].cycle[i];
                            loopv(spawns[team].cycle) if(spawns[team].cycle[i] == lowest) num++;
                            if(num > 0)
                            {
                                int r = rnd(num+1), n = 0;
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
        setupspawns(true, m_trial(gamemode) || m_lobby(gamemode) ? 0 : np);
        hasgameinfo = aiman::dorefresh = true;
    }

    void sendspawn(clientinfo *ci)
    {
        servstate &gs = ci->state;
        int weap = m_weapon(gamemode, mutators), maxhealth = m_health(gamemode, mutators);
        bool grenades = GAME(spawngrenades) >= (m_insta(gamemode, mutators) || m_trial(gamemode) ? 2 : 1), arena = m_arena(gamemode, mutators),
            melee = GAME(spawnmelee) >= (!m_insta(gamemode, mutators) || m_trial(gamemode) ? 1 : 2);
        if(ci->state.aitype >= AI_START)
        {
            weap = aistyle[ci->state.aitype].weap;
            if(!isweap(weap)) weap = rnd(WEAP_MAX-1)+1;
            maxhealth = aistyle[ci->state.aitype].health;
            arena = grenades = false;
            melee = true;
        }
        gs.spawnstate(weap, maxhealth, melee, arena, grenades);
        int spawn = pickspawn(ci);
        sendf(ci->clientnum, 1, "ri9v", SV_SPAWNSTATE, ci->clientnum, spawn, gs.state, gs.points, gs.frags, gs.health, gs.cptime, gs.weapselect, WEAP_MAX, &gs.ammo[0]);
        gs.lastrespawn = gs.lastspawn = gamemillis;
    }

    template<class T>
    void sendstate(servstate &gs, T &p)
    {
        putint(p, gs.state);
        putint(p, gs.points);
        putint(p, gs.frags);
        putint(p, gs.health);
        putint(p, gs.cptime);
        putint(p, gs.weapselect);
        loopi(WEAP_MAX) putint(p, gs.ammo[i]);
    }

    void relayf(int r, const char *s, ...)
    {
        defvformatstring(str, s, s);
#ifdef IRC
        ircoutf(r, "%s", str);
#endif
#ifdef STANDALONE
        string st;
        filtertext(st, str);
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
            demos.shrink(0);
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
        if(GAME(resetmmonend) >= 2) mastermode = MM_OPEN;
        if(GAME(resetvarsonend) >= 2) resetgamevars(true);
        if(GAME(resetbansonend) >= 2) loopv(bans) if(bans[i].time >= 0) bans.remove(i--);
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
                srvoutf(3, "vote passed: \fs\fy%s\fS on map \fs\fo%s\fS", gamename(best->mode, best->muts), best->map);
                sendf(-1, 1, "ri2si3", SV_MAPCHANGE, 1, best->map, 0, best->mode, best->muts);
                changemap(best->map, best->mode, best->muts);
            }
            else
            {
                int mode = GAME(defaultmode) >= 0 ? gamemode : -1, muts = GAME(defaultmuts) >= -1 ? mutators : -2;
                changemode(&mode, &muts);
                const char *map = choosemap(smapname, mode, muts);
                srvoutf(3, "server chooses: \fs\fy%s\fS on map \fs\fo%s\fS", gamename(mode, muts), map);
                sendf(-1, 1, "ri2si3", SV_MAPCHANGE, 1, map, 0, mode, muts);
                changemap(map, mode, muts);
            }
            return true;
        }
        return false;
    }

    bool mutscmp(int reqmuts, int limited)
    {
        if(reqmuts)
        {
            if(!limited) return false;
            loopi(G_M_NUM) if(reqmuts&(1<<i) && !(limited&(1<<i))) return false;
        }
        return true;
    }

    void vote(char *reqmap, int &reqmode, int &reqmuts, int sender)
    {
        clientinfo *ci = (clientinfo *)getinfo(sender); modecheck(&reqmode, &reqmuts);
        if(!ci || !m_game(reqmode) || !reqmap || !*reqmap) return;
        switch(GAME(votelock))
        {
            case 1: case 2: if(!m_edit(reqmode) && smapname[0] && !strcmp(reqmap, smapname) && !haspriv(ci, GAME(votelock) == 1 ? PRIV_MASTER : PRIV_ADMIN, "vote for the same map again")) return; break;
            case 3: case 4: if(!haspriv(ci, GAME(votelock) == 3 ? PRIV_MASTER : PRIV_ADMIN, "vote for a new game")) return; break;
            case 5: if(!haspriv(ci, PRIV_MAX, "vote for a new game")) return; break;
        }
        bool hasveto = haspriv(ci, PRIV_MASTER) && (mastermode >= MM_VETO || !numclients(ci->clientnum));
        if(!hasveto)
        {
            if(ci->lastvote && lastmillis-ci->lastvote <= GAME(votewait)) return;
            if(ci->modevote == reqmode && ci->mutsvote == reqmuts && !strcmp(ci->mapvote, reqmap)) return;
        }
        if(reqmode < G_LOBBY && !ci->local)
        {
            srvmsgf(ci->clientnum, "\fraccess denied, you must be a local client");
            return;
        }
        switch(GAME(modelock))
        {
            case 1: case 2: if(!haspriv(ci, GAME(modelock) == 1 ? PRIV_MASTER : PRIV_ADMIN, "change game modes")) return; break;
            case 3: case 4: if((reqmode < GAME(modelimit) || !mutscmp(reqmuts, GAME(mutslimit))) && !haspriv(ci, GAME(modelock) == 3 ? PRIV_MASTER : PRIV_ADMIN, "change to a locked game mode")) return; break;
            case 5: if(!haspriv(ci, PRIV_MAX, "change game modes")) return; break;
            case 0: default: break;
        }
        if(reqmode != G_EDITMODE && GAME(mapslock))
        {
            const char *maplist = NULL;
            switch(GAME(mapslock))
            {
                case 1: case 2: maplist = GAME(allowmaps); break;
                case 3: case 4:
                {
                    if(m_campaign(reqmode)) maplist = GAME(campaignmaps);
                    else if(m_duel(reqmode, reqmuts)) maplist = GAME(duelmaps);
                    else if(m_stf(reqmode)) maplist = GAME(stfmaps);
                    else if(m_ctf(reqmode)) maplist = m_multi(reqmode, reqmuts) ? GAME(mctfmaps) : GAME(ctfmaps);
                    else if(m_trial(reqmode)) maplist = GAME(trialmaps);
                    else if(m_fight(reqmode)) maplist = GAME(mainmaps);
                    else maplist = GAME(allowmaps);
                    break;
                }
                case 5: if(!haspriv(ci, PRIV_MAX, "select a custom maps")) return; break;
                case 0: default: break;
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
                if(!found && !haspriv(ci, GAME(mapslock)%2 ? PRIV_MASTER : PRIV_ADMIN, "select a custom maps")) return;
            }
        }
        copystring(ci->mapvote, reqmap); ci->modevote = reqmode; ci->mutsvote = reqmuts;
        if(hasveto)
        {
            endmatch();
            srvoutf(3, "%s forced: \fs\fy%s\fS on map \fs\fo%s\fS", colorname(ci), gamename(ci->modevote, ci->mutsvote), ci->mapvote);
            sendf(-1, 1, "ri2si3", SV_MAPCHANGE, 1, ci->mapvote, 0, ci->modevote, ci->mutsvote);
            changemap(ci->mapvote, ci->modevote, ci->mutsvote);
            return;
        }
        sendf(-1, 1, "ri2si2", SV_MAPVOTE, ci->clientnum, ci->mapvote, ci->modevote, ci->mutsvote);
        relayf(3, "%s suggests: \fs\fy%s\fS on map \fs\fo%s\fS", colorname(ci), gamename(ci->modevote, ci->mutsvote), ci->mapvote);
        checkvotes();
    }

    extern void waiting(clientinfo *ci, int doteam = 0, int drop = 2, bool exclude = false);

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

    void givepoints(clientinfo *ci, int points)
    {
        ci->state.score += points; ci->state.points += points;
        sendf(-1, 1, "ri4", SV_POINTS, ci->clientnum, points, ci->state.points);
    }

    void distpoints(clientinfo *ci, bool discon = false)
    {
        if(m_team(gamemode, mutators) && !m_flag(gamemode))
        {
            int friends = 0;
            loopv(clients) if(ci != clients[i] && clients[i]->state.aitype < AI_START && clients[i]->team == ci->team) friends++;
            if(friends)
            {
                int points = int(ci->state.points/float(friends)), offset = ci->state.points-abs(points);
                if(points || offset) loopv(clients) if(ci != clients[i] && clients[i]->state.aitype < AI_START && clients[i]->team == ci->team)
                {
                    int total = points;
                    if(offset > 0) { total++; offset--; }
                    else if(offset < 0) { total--; offset++; }
                    else if(!points) break;
                    clients[i]->state.points += total;
                    sendf(-1, 1, "ri4", SV_POINTS, clients[i]->clientnum, total, clients[i]->state.points);
                }
            }
            if(!discon) sendf(-1, 1, "ri4", SV_POINTS, ci->clientnum, -ci->state.points, 0);
            ci->state.points = 0;
        }
    }

    void savescore(clientinfo *ci)
    {
        savedscore &sc = findscore(ci, true);
        if(&sc) sc.save(ci->state);
    }

    void setteam(clientinfo *ci, int team, bool reset = true, bool info = false)
    {
        if(ci->team != team)
        {
            bool sm = false;
            distpoints(ci);
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
            int team = isteam(gamemode, mutators, suggest, TEAM_FIRST) ? suggest : -1, balance = GAME(teambalance);
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

    struct droplist { int weap, ent; };
    void dropitems(clientinfo *ci, int level = 2)
    {
        if(ci->state.aitype >= AI_START) ci->state.weapreset(false);
        else
        {
            servstate &ts = ci->state;
            vector<droplist> drop;
            int sweap = m_weapon(gamemode, mutators);
            if(level >= 2 && GAME(kamikaze) && (GAME(kamikaze) > 2 || (ts.hasweap(WEAP_GRENADE, sweap) && (GAME(kamikaze) > 1 || ts.weapselect == WEAP_GRENADE))))
            {
                ts.weapshots[WEAP_GRENADE][0].add(-1);
                droplist &d = drop.add();
                d.weap = WEAP_GRENADE;
                d.ent = -1;
            }
            if(!m_noitems(gamemode, mutators))
            {
                loopi(WEAP_MAX) if(i != WEAP_GRENADE && i != sweap && ts.hasweap(i, sweap) && sents.inrange(ts.entid[i]))
                {
                    sents[ts.entid[i]].millis = gamemillis;
                    if(level && GAME(itemdropping) && !(sents[ts.entid[i]].attrs[1]&WEAP_F_FORCED))
                    {
                        ts.dropped.add(ts.entid[i]);
                        droplist &d = drop.add();
                        d.weap = i;
                        d.ent = ts.entid[i];
                        sents[ts.entid[i]].millis += w_spawn(sents[ts.entid[i]].attrs[0]);
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

    #include "auth.h"

    void spectator(clientinfo *ci, int sender = -1)
    {
        if(!ci || ci->state.aitype >= 0) return;
        ci->state.state = CS_SPECTATOR;
        sendf(sender, 1, "ri3", SV_SPECTATOR, ci->clientnum, 1);
        setteam(ci, TEAM_NEUTRAL, false, true);
    }

    bool allowstate(clientinfo *ci, int n)
    {
        if(!ci) return false;
        if(ci->state.aitype < 0) switch(n)
        {
            case 0: if(ci->state.state == CS_SPECTATOR || gamemode >= G_EDITMODE) return false; // first spawn, falls through
            case 1: // try spawn
            {
                if(ci->wantsmap || (mastermode >= MM_LOCKED && ci->state.state == CS_SPECTATOR)) return false;
                if(ci->state.state == CS_ALIVE || (ci->state.lastdeath && gamemillis-ci->state.lastdeath <= DEATHMILLIS)) return false;
                break;
            }
            case 2: // spawn
            {
                if((ci->state.state != CS_DEAD && ci->state.state != CS_WAITING) || (ci->state.lastdeath && gamemillis-ci->state.lastdeath <= DEATHMILLIS)) return false;
                break;
            }
            case 3: return true; // spec
            case 5: if(ci->state.state != CS_EDITING) return false;
            case 4: // edit on/off
            {
                if(!m_edit(gamemode) || (mastermode >= MM_LOCKED && ci->state.state == CS_SPECTATOR)) return false;
                break;
            }
            default: break;
        }
        return true;
    }

    void changemap(const char *name, int mode, int muts)
    {
        hasgameinfo = maprequest = mapsending = shouldcheckvotes = false;
        aiman::clearai(m_play(mode) ? false : true);
        aiman::dorefresh = true;
        stopdemo();
        gamemode = mode; mutators = muts; changemode(&gamemode, &mutators);
        nplayers = gamemillis = interm = 0;
        oldtimelimit = GAME(timelimit);
        timeremaining = GAME(timelimit) ? GAME(timelimit)*60 : -1;
        gamelimit = GAME(timelimit) ? timeremaining*1000 : 0;
        sents.shrink(0); scores.shrink(0);
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
#else
        loopi(3) if(mapdata[i]) DELETEP(mapdata[i]);
#endif
        copystring(smapname, reqmap);

        // server modes
        if(m_stf(gamemode)) smode = &stfmode;
        else if(m_ctf(gamemode)) smode = &ctfmode;
        else smode = NULL;
        smuts.shrink(0);
        if(m_duke(gamemode, mutators)) smuts.add(&duelmutator);
        if(smode) smode->reset(false);
        mutate(smuts, mut->reset(false));

        if(m_demo(gamemode)) kicknonlocalclients(DISC_PRIVATE);
        loopv(clients) clients[i]->mapchange(true);
        loopv(clients)
        {
            clientinfo *ci = clients[i];
            if(allowstate(ci, 0))
            {
                ci->state.state = CS_DEAD;
                waiting(ci, 2, 1);
            }
            else spectator(ci);
        }

        if(m_fight(gamemode) && numclients()) sendf(-1, 1, "ri2", SV_TICK, timeremaining);
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
            int locked = max(id->flags&IDF_ADMIN ? 1 : 0, GAME(varslock));
            switch(id->type)
            {
                case ID_CCOMMAND:
                case ID_COMMAND:
                {
                    if(!haspriv(ci, locked >= 2 ? PRIV_MAX : (locked ? PRIV_ADMIN : PRIV_MASTER), "change variables")) return;
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
                    else if(!haspriv(ci, locked >= 2 ? PRIV_MAX : (locked ? PRIV_ADMIN : PRIV_MASTER), "change variables")) return;
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
                    else if(!haspriv(ci, locked >= 2 ? PRIV_MAX : (locked ? PRIV_ADMIN : PRIV_MASTER), "change variables")) return;
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
                    else if(!haspriv(ci, locked >= 2 ? PRIV_MAX : (locked ? PRIV_ADMIN : PRIV_MASTER), "change variables")) return;
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
        sendf(-1, 1, "ri8vi", SV_RESUME, ci->clientnum, gs.state, gs.points, gs.frags, gs.health, gs.cptime, gs.weapselect, WEAP_MAX, &gs.ammo[0], -1);
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
        else if(!ci->online && m_edit(gamemode) && numclients(ci->clientnum))
        {
            loopi(3) if(mapdata[i]) DELETEP(mapdata[i]);
            ci->wantsmap = true;
            if(!mapsending)
            {
                clientinfo *best = choosebestclient();
                if(best)
                {
                    srvmsgf(ci->clientnum, "map is being requested, please wait..");
                    sendf(best->clientnum, 1, "ri", SV_GETMAP);
                    mapsending = true;
                }
            }
            putint(p, 1); // already in progress
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
            putint(p, SV_TICK);
            putint(p, timeremaining);
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

        if(*GAME(servermotd))
        {
            putint(p, SV_ANNOUNCE);
            putint(p, S_GUIACT);
            putint(p, CON_CHAT);
            sendstring(GAME(servermotd), p);
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
        if((realflags&HIT_WAVE || (isweap(weap) && !WEAPEX(weap, realflags&HIT_ALT, gamemode, mutators))) && (realflags&HIT_FULL))
            realflags &= ~HIT_FULL;
        if(smode && !smode->damage(target, actor, realdamage, weap, realflags, hitpush)) { nodamage++; }
        mutate(smuts, if(!mut->damage(target, actor, realdamage, weap, realflags, hitpush)) { nodamage++; });
        if(actor->state.aitype < AI_START)
        {
            if((actor == target && !GAME(selfdamage)) || (m_trial(gamemode) && !GAME(trialdamage))) nodamage++;
            else if(m_team(gamemode, mutators) && actor->team == target->team && actor != target)
            {
                if(weap == WEAP_MELEE) nodamage++;
                else if(m_play(gamemode)) switch(GAME(teamdamage))
                {
                    case 2: default: break;
                    case 1: if(actor->state.aitype < 0 || target->state.aitype >= 0) break;
                    case 0: nodamage++; break;
                }
            }
        }
        if(nodamage || !hithurts(realflags)) realflags = HIT_WAVE|(flags&HIT_ALT ? HIT_ALT : 0); // so it impacts, but not hurts
        else
        {
            if(isweap(weap) && GAME(damagecritchance))
            {
                actor->state.crits++;
                if(WEAP(weap, critmult) > 0)
                {
                    int offset = GAME(damagecritchance)-actor->state.crits;
                    if(target != actor && WEAP(weap, critdist) > 0)
                    {
                        float dist = actor->state.o.dist(target->state.o);
                        if(dist <= WEAP(weap, critdist))
                            offset = int(offset*clamp(dist, 1.f, WEAP(weap, critdist))/WEAP(weap, critdist));
                    }
                    if(offset <= 0 || !rnd(offset))
                    {
                        realflags |= HIT_CRIT;
                        realdamage = int(realdamage*WEAP(weap, critmult));
                        actor->state.crits = 0;
                    }
                }
            }
            target->state.dodamage(target->state.health -= realdamage);
            target->state.lastpain = gamemillis;
            actor->state.damage += realdamage;
            if(target->state.health <= 0) realflags |= HIT_KILL;
            if(GAME(fireburntime) && doesburn(weap, flags))
            {
                target->state.lastfire = target->state.lastfireburn = gamemillis;
                target->state.lastfireowner = actor->clientnum;
            }
        }
        sendf(-1, 1, "ri7i3", SV_DAMAGE, target->clientnum, actor->clientnum, weap, realflags, realdamage, target->state.health, hitpush.x, hitpush.y, hitpush.z);
        if(GAME(vampire) && actor->state.state == CS_ALIVE)
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

            target->state.spree = 0;
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
                    if(m_fight(gamemode) && GAME(multikilldelay))
                    {
                        logs = 0;
                        loopv(actor->state.fragmillis)
                        {
                            if(lastmillis-actor->state.fragmillis[i] > GAME(multikilldelay)) actor->state.fragmillis.remove(i--);
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
                    if(actor->state.spree <= GAME(spreecount)*FRAG_SPREES && !(actor->state.spree%GAME(spreecount)))
                    {
                        int offset = clamp((actor->state.spree/GAME(spreecount)), 1, int(FRAG_SPREES))-1, type = 1<<(FRAG_SPREE+offset);
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
                        if(logs >= GAME(dominatecount))
                        {
                            style |= FRAG_REVENGE;
                            pointvalue *= GAME(dominatecount);
                        }
                        logs = 0;
                        loopv(actor->state.fraglog) if(actor->state.fraglog[i] == target->clientnum) logs++;
                        if(logs == GAME(dominatecount))
                        {
                            style |= FRAG_DOMINATE;
                            pointvalue *= GAME(dominatecount);
                        }
                    }
                }
            }
            target->state.deaths++;
            dropitems(target);
            if(actor != target && actor->state.aitype >= AI_START) givepoints(target, pointvalue); else givepoints(actor, pointvalue);
            sendf(-1, 1, "ri8", SV_DIED, target->clientnum, actor->clientnum, actor->state.frags, style, weap, realflags, realdamage);
            target->position.setsize(0);
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
            ci->state.cpnodes.shrink(0);
        }
        ci->state.deaths++;
        dropitems(ci); givepoints(ci, pointvalue);
        if(!(flags&HIT_FULL)) flags |= HIT_FULL;
        if(GAME(fireburntime) && (flags&HIT_MELT || flags&HIT_BURN))
        {
            ci->state.lastfire = ci->state.lastfireburn = gamemillis;
            ci->state.lastfireowner = ci->clientnum;
        }
        sendf(-1, 1, "ri8", SV_DIED, ci->clientnum, ci->clientnum, ci->state.frags, 0, -1, flags, ci->state.health);
        ci->position.setsize(0);
        if(smode) smode->died(ci, NULL);
        mutate(smuts, mut->died(ci, NULL));
        gs.state = CS_DEAD;
        gs.lastdeath = gamemillis;
    }

    int calcdamage(int weap, int &flags, int radial, float size, float dist)
    {
        int damage = WEAP2(weap, damage, flags&HIT_ALT); flags &= ~HIT_SFLAGS;
        if((flags&HIT_WAVE || (isweap(weap) && !WEAPEX(weap, flags&HIT_ALT, gamemode, mutators))) && flags&HIT_FULL) flags &= ~HIT_FULL;
        if(radial) damage = int(ceilf(damage*clamp(1.f-dist/size, 1e-3f, 1.f)));
        else if(WEAP2(weap, taper, flags&HIT_ALT) > 0) damage = int(ceilf(damage*clamp(dist, 0.f, 1.f)));
        if(!hithurts(flags)) flags = HIT_WAVE|(flags&HIT_ALT ? HIT_ALT : 0); // so it impacts, but not hurts
        if(hithurts(flags))
        {
            if(flags&HIT_FULL || flags&HIT_HEAD) damage = int(ceilf(damage*GAME(damagescale)));
            else if(flags&HIT_TORSO) damage = int(ceilf(damage*0.5f*GAME(damagescale)));
            else if(flags&HIT_LEGS) damage = int(ceilf(damage*0.25f*GAME(damagescale)));
            else damage = 0;
        }
        else damage = int(ceilf(damage*GAME(damagescale)));
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
                    float size = radial ? (hflags&HIT_WAVE ? radial*WEAP(weap, pusharea) : radial) : 0.f, dist = float(h.dist)/DNF;
                    clientinfo *target = (clientinfo *)getinfo(h.target);
                    if(!target || target->state.state != CS_ALIVE || (size>0 && (dist<0 || dist>size)) || target->state.protect(gamemillis, m_protect(gamemode, mutators)))
                        continue;
                    int damage = calcdamage(weap, hflags, radial, size, dist);
                    dodamage(target, ci, damage, weap, hflags, h.dir);
                }
            }
        }
        else if(weap == -1)
        {
            gs.dropped.remove(id);
            if(sents.inrange(id)) sents[id].millis = gamemillis;
        }
    }

    void takeammo(clientinfo *ci, int weap, int amt = 1) { ci->state.ammo[weap] = max(ci->state.ammo[weap]-amt, 0); }

    void shotevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(!gs.isalive(gamemillis) || !isweap(weap))
        {
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: shoot [%d] failed - unexpected message", weap);
            return;
        }
        if(!gs.canshoot(weap, flags, m_weapon(gamemode, mutators), millis))
        {
            if(!gs.canshoot(weap, flags, m_weapon(gamemode, mutators), millis, (1<<WEAP_S_RELOAD)))
            {
                if(WEAP2(weap, sub, flags&HIT_ALT) && WEAP(weap, max))
                    ci->state.ammo[weap] = max(ci->state.ammo[weap]-WEAP2(weap, sub, flags&HIT_ALT), 0);
                if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: shoot [%d] failed - current state disallows it", weap);
                return;
            }
            else if(gs.weapload[weap] > 0)
            {
                takeammo(ci, weap, gs.weapload[weap]+WEAP2(weap, sub, flags&HIT_ALT));
                gs.weapload[weap] = -gs.weapload[weap];
                sendf(-1, 1, "ri5", SV_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap]);
            }
            else return;
        }
        else takeammo(ci, weap, WEAP2(weap, sub, flags&HIT_ALT));
        gs.setweapstate(weap, WEAP_S_SHOOT, WEAP2(weap, adelay, flags&HIT_ALT), millis);
        sendf(-1, 1, "ri8ivx", SV_SHOTFX, ci->clientnum, weap, flags, power, from[0], from[1], from[2], shots.length(), shots.length()*sizeof(ivec)/sizeof(int), shots.getbuf(), ci->clientnum);
        gs.weapshot[weap] = WEAP2(weap, sub, flags&HIT_ALT);
        gs.shotdamage += WEAP2(weap, damage, flags&HIT_ALT)*shots.length();
        loopv(shots) gs.weapshots[weap][flags&HIT_ALT ? 1 : 0].add(id);
    }

    void switchevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(!gs.isalive(gamemillis) || !isweap(weap))
        {
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: switch [%d] failed - unexpected message", weap);
            sendf(ci->clientnum, 1, "ri3", SV_WEAPSELECT, ci->clientnum, gs.weapselect);
            return;
        }
        if(!gs.canswitch(weap, m_weapon(gamemode, mutators), millis, (1<<WEAP_S_SWITCH)))
        {
            if(!gs.canswitch(weap, m_weapon(gamemode, mutators), millis, (1<<WEAP_S_RELOAD)))
            {
                if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: switch [%d] failed - current state disallows it", weap);
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
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: drop [%d] failed - unexpected message", weap);
            return;
        }
        int sweap = m_weapon(gamemode, mutators);
        if(!gs.hasweap(weap, sweap, weap == WEAP_GRENADE ? 2 : 0) || (weap != WEAP_GRENADE && m_noitems(gamemode, mutators)))
        {
            if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: drop [%d] failed - current state disallows it", weap);
            return;
        }
        if(weap == WEAP_GRENADE)
        {
            int nweap = -1; // try to keep this weapon
            gs.weapshots[WEAP_GRENADE][0].add(-1);
            takeammo(ci, WEAP_GRENADE, 1);
            if(!gs.hasweap(weap, sweap))
            {
                nweap = gs.bestweap(sweap, true);
                gs.weapswitch(nweap, millis);
            }
            else gs.setweapstate(weap, WEAP_S_SHOOT, WEAP2(weap, adelay, false), millis);
            sendf(-1, 1, "ri6", SV_DROP, ci->clientnum, nweap, 1, weap, -1);
            return;
        }
        else if(!sents.inrange(gs.entid[weap]) || (sents[gs.entid[weap]].attrs[1]&WEAP_F_FORCED))
        {
            if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: drop [%d] failed - not droppable entity", weap);
            return;
        }
        int dropped = gs.entid[weap];
        gs.ammo[weap] = gs.entid[weap] = -1;
        int nweap = gs.bestweap(sweap, true); // switch to best weapon
        if(sents.inrange(dropped)) sents[dropped].millis = gamemillis+w_spawn(sents[dropped].attrs[0]);
        gs.dropped.add(dropped);
        gs.weapswitch(nweap, millis);
        sendf(-1, 1, "ri6", SV_DROP, ci->clientnum, nweap, 1, weap, dropped);
    }

    void reloadevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(!gs.isalive(gamemillis) || !isweap(weap))
        {
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: reload [%d] failed - unexpected message", weap);
            sendf(ci->clientnum, 1, "ri5", SV_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap]);
            return;
        }
        if(!gs.canreload(weap, m_weapon(gamemode, mutators), millis))
        {
            if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: reload [%d] failed - current state disallows it", weap);
            sendf(ci->clientnum, 1, "ri5", SV_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap]);
            return;
        }
        gs.setweapstate(weap, WEAP_S_RELOAD, WEAP(weap, rdelay), millis);
        int oldammo = gs.ammo[weap];
        gs.ammo[weap] = min(max(gs.ammo[weap], 0) + WEAP(weap, add), WEAP(weap, max));
        gs.weapload[weap] = gs.ammo[weap]-oldammo;
        sendf(-1, 1, "ri5x", SV_RELOAD, ci->clientnum, weap, gs.weapload[weap], gs.ammo[weap], ci->clientnum);
    }

    void useevent::process(clientinfo *ci)
    {
        servstate &gs = ci->state;
        if(gs.state != CS_ALIVE || !sents.inrange(ent) || enttype[sents[ent].type].usetype != EU_ITEM || m_noitems(gamemode, mutators))
        {
            if(GAME(serverdebug) >= 3) srvmsgf(ci->clientnum, "sync error: use [%d] failed - unexpected message", ent);
            return;
        }
        int sweap = m_weapon(gamemode, mutators), attr = sents[ent].type == WEAPON ? w_attr(gamemode, sents[ent].attrs[0], sweap) : sents[ent].attrs[0];
        if(!gs.canuse(sents[ent].type, attr, sents[ent].attrs, sweap, millis, (1<<WEAP_S_SWITCH)))
        {
            if(!gs.canuse(sents[ent].type, attr, sents[ent].attrs, sweap, millis, (1<<WEAP_S_RELOAD)))
            {
                if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: use [%d] failed - current state disallows it", ent);
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
        if(!(sents[ent].attrs[1]&WEAP_F_FORCED))
        {
            bool found = sents[ent].spawned;
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
                if(GAME(serverdebug)) srvmsgf(ci->clientnum, "sync error: use [%d] failed - doesn't seem to be spawned anywhere", ent);
                return;
            }
        }

        int weap = -1, dropped = -1;
        if(sents[ent].type == WEAPON && gs.ammo[attr] < 0 && w_carry(attr, sweap) && gs.carry(sweap) >= GAME(maxcarry))
            weap = gs.drop(sweap, attr);
        if(isweap(weap))
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
                sents[dropped].millis = gamemillis+w_spawn(sents[dropped].attrs[0]);
            }
        }
        if(!(sents[ent].attrs[1]&WEAP_F_FORCED))
        {
            sents[ent].spawned = false;
            sents[ent].millis = gamemillis+w_spawn(sents[ent].attrs[0]);
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
        if(m_arena(gamemode, mutators) && ci->state.loadweap < 0 && ci->state.aitype < 0) sendf(ci->clientnum, 1, "ri", SV_LOADWEAP);
        if(doteam && (doteam == 2 || !isteam(gamemode, mutators, ci->team, TEAM_FIRST)))
            setteam(ci, chooseteam(ci, ci->team), false, true);
    }

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
        int items[MAXENTTYPES] = {0}, lowest[MAXENTTYPES] = {-1}, sweap = m_weapon(gamemode, mutators);
        if(m_fight(gamemode) && !m_noitems(gamemode, mutators) && !m_arena(gamemode, mutators))
        {
            loopv(clients) if(clients[i]->clientnum >= 0 && clients[i]->name[0] && clients[i]->state.aitype < AI_START)
                items[WEAPON] += clients[i]->state.carry(sweap);
            loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM && hasitem(i) && (sents[i].type != WEAPON || w_carry(w_attr(gamemode, sents[i].attrs[0], sweap), sweap)))
            {
                if(finditem(i, true, true)) items[sents[i].type]++;
                else if(!sents.inrange(lowest[sents[i].type]) || sents[i].millis < sents[lowest[sents[i].type]].millis)
                    lowest[sents[i].type] = i;
            }
        }
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
                bool allowed = hasitem(i);
                if(enttype[sents[i].type].usetype == EU_ITEM && (allowed || sents[i].spawned))
                {
                    bool found = finditem(i, true, true);
                    if(allowed && m_fight(gamemode) && !m_noitems(gamemode, mutators) && !m_arena(gamemode, mutators) && i == lowest[sents[i].type] && (sents[i].type != WEAPON || w_carry(w_attr(gamemode, sents[i].attrs[0], sweap), sweap)))
                    {
                        float dist = float(items[sents[i].type])/float(numclients(-1, true, AI_BOT))/float(GAME(maxcarry));
                        if(dist < GAME(itemthreshold)) found = false;
                    }
                    if((!found && !sents[i].spawned) || (!allowed && sents[i].spawned))
                    {
                        loopvk(clients) clients[k]->state.dropped.remove(i);
                        if((sents[i].spawned = allowed) != false)
                        {
                            int delay = sents[i].type == WEAPON && isweap(sents[i].attrs[0]) ? w_spawn(sents[i].attrs[0]) : GAME(itemspawntime);
                            sents[i].millis = gamemillis+delay;
                        }
                        sendf(-1, 1, "ri3", SV_ITEMSPAWN, i, sents[i].spawned ? 1 : 0);
                        items[sents[i].type]++;
                    }
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
                if(ci->state.onfire(gamemillis, GAME(fireburntime)))
                {
                    if(gamemillis-ci->state.lastfireburn >= GAME(fireburndelay))
                    {
                        clientinfo *co = (clientinfo *)getinfo(ci->state.lastfireowner);
                        dodamage(ci, co ? co : ci, GAME(fireburndamage), -1, HIT_BURN);
                        ci->state.lastfireburn += GAME(fireburndelay);
                    }
                }
                else
                {
                    if(ci->state.lastfire) ci->state.lastfire = ci->state.lastfireburn = 0;
                    if(m_regen(gamemode, mutators) && ci->state.aitype < AI_START)
                    {
                        int total = m_health(gamemode, mutators), amt = GAME(regenhealth), delay = ci->state.lastregen ? GAME(regentime) : GAME(regendelay);
                        if(smode) smode->regen(ci, total, amt, delay);
                        if(delay && (ci->state.health < total || ci->state.health > total) && gamemillis-(ci->state.lastregen ? ci->state.lastregen : ci->state.lastpain) >= delay)
                        {
                            int low = 0;
                            if(ci->state.health > total) { amt = -GAME(regenhealth); total = ci->state.health; low = m_health(gamemode, mutators); }
                            int rgn = ci->state.health, heal = clamp(ci->state.health+amt, low, total), eff = heal-rgn;
                            if(eff)
                            {
                                ci->state.health = heal; ci->state.lastregen = gamemillis;
                                sendf(-1, 1, "ri4", SV_REGEN, ci->clientnum, ci->state.health, eff);
                            }
                        }
                    }
                }
            }
            else if(ci->state.state == CS_WAITING)
            {
                if(m_arena(gamemode, mutators) && ci->state.loadweap < 0 && ci->state.aitype < 0) continue;
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
            loopv(bans) if(bans[i].time >= 0 && totalmillis-bans[i].time > 4*60*60000) bans.remove(i--);
            loopv(connects) if(totalmillis-connects[i]->connectmillis > 15000) disconnect_client(connects[i]->clientnum, DISC_TIMEOUT);

            if(masterupdate)
            {
                loopv(clients) sendf(-1, 1, "ri3", SV_CURRENTMASTER, clients[i]->clientnum, clients[i]->privilege);
                masterupdate = false;
            }

            if(interm && gamemillis >= interm) // wait then call for next map
            {
                if(GAME(votelimit) && !maprequest)
                {
                    if(demorecord) enddemorecord();
                    sendf(-1, 1, "ri", SV_NEWGAME);
                    maprequest = true;
                    interm = gamemillis+GAME(votelimit);
                }
                else
                {
                    interm = 0;
                    checkvotes(true);
                }
            }
            if(shouldcheckvotes) checkvotes();
        }
        else if(!GAME(resetbansonend))
        {
            loopv(bans) if(bans[i].time >= 0 && totalmillis-bans[i].time > 4*60*60000) bans.remove(i--);
        }
        aiman::checkai();
        auth::update();
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

    void clientdisconnect(int n, bool local, int reason)
    {
        clientinfo *ci = (clientinfo *)getinfo(n);
        bool complete = !numclients(n);
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
            distpoints(ci, true); savescore(ci);
            sendf(-1, 1, "ri2", SV_DISCONNECT, n, reason);
            ci->connected = false;
            if(ci->name[0])
            {
                int amt = numclients(ci->clientnum);
                relayf(2, "\fo%s (%s) has left the game (%s, %d %s)", colorname(ci), gethostname(n), reason >= 0 ? disc_reasons[reason] : "normal", amt, amt != 1 ? "players" : "player");
            }
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
        putint(p, numclients(-1, true));
        putint(p, 6);                   // number of attrs following
        putint(p, GAMEVERSION);         // 1
        putint(p, gamemode);            // 2
        putint(p, mutators);            // 3
        putint(p, timeremaining/60);    // 4
        putint(p, GAME(serverclients)); // 5
        putint(p, serverpass[0] ? MM_PASSWORD : (m_demo(gamemode) ? MM_PRIVATE : mastermode)); // 6
        sendstring(smapname, p);
        if(*GAME(serverdesc)) sendstring(GAME(serverdesc), p);
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
        loopv(clients) if(clients[i]->clientnum >= 0 && clients[i]->name[0] && clients[i]->state.aitype < 0 && clients[i]->state.state != CS_SPECTATOR)
            sendstring(colorname(clients[i]), p);
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
        mapdata[n]->write(data, len);
        return n == 2;
    }

    int checktype(int type, clientinfo *ci)
    {
        if(ci && ci->local) return type;
        // only allow edit messages in coop-edit mode
        if(type >= SV_EDITENT && type <= SV_NEWMAP && (!m_edit(gamemode) || !ci || ci->state.state != CS_EDITING)) return -1;
        // server only messages
        static int servtypes[] = { SV_SERVERINIT, SV_CLIENTINIT, SV_WELCOME, SV_NEWGAME, SV_MAPCHANGE, SV_SERVMSG, SV_DAMAGE, SV_SHOTFX, SV_DIED, SV_POINTS, SV_SPAWNSTATE, SV_ITEMACC, SV_ITEMSPAWN, SV_TICK, SV_DISCONNECT, SV_CURRENTMASTER, SV_PONG, SV_RESUME, SV_SCORE, SV_FLAGINFO, SV_ANNOUNCE, SV_SENDDEMOLIST, SV_SENDDEMO, SV_DEMOPLAYBACK, SV_REGEN, SV_SCOREFLAG, SV_RETURNFLAG, SV_CLIENT, SV_AUTHCHAL };
        if(ci)
        {
            loopi(sizeof(servtypes)/sizeof(int)) if(type == servtypes[i]) return -1;
            if(type < SV_EDITENT || type > SV_NEWMAP || !m_edit(gamemode) || !ci || ci->state.state != CS_EDITING)
            {
                if(++ci->overflow >= 200) return -2;
            }
        }
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
            ci.overflow = 0;
            if(ci.position.empty()) ci.posoff = -1;
            else
            {
                ci.posoff = ws.positions.length();
                ws.positions.put(ci.position.getbuf(), ci.position.length());
                ci.poslen = ws.positions.length() - ci.posoff;
                ci.position.setsize(0);
            }
            if(ci.messages.empty()) ci.msgoff = -1;
            else
            {
                ci.msgoff = ws.messages.length();
                putint(ws.messages, SV_CLIENT);
                putint(ws.messages, ci.clientnum);
                putuint(ws.messages, ci.messages.length());
                ws.messages.put(ci.messages.getbuf(), ci.messages.length());
                ci.msglen = ws.messages.length() - ci.msgoff;
                ci.messages.setsize(0);
            }
        }
        int psize = ws.positions.length(), msize = ws.messages.length();
        if(psize)
        {
            recordpacket(0, ws.positions.getbuf(), psize);
            ucharbuf p = ws.positions.reserve(psize);
            p.put(ws.positions.getbuf(), psize);
            ws.positions.addbuf(p);
        }
        if(msize)
        {
            recordpacket(1, ws.messages.getbuf(), msize);
            ucharbuf p = ws.messages.reserve(msize);
            p.put(ws.messages.getbuf(), msize);
            ws.messages.addbuf(p);
        }
        ws.uses = 0;
        loopv(clients)
        {
            clientinfo &ci = *clients[i];
            ENetPacket *packet;
            if(ci.state.aitype < 0 && psize && (ci.posoff<0 || psize-ci.poslen>0))
            {
                packet = enet_packet_create(&ws.positions[ci.posoff<0 ? 0 : ci.posoff+ci.poslen],
                                            ci.posoff<0 ? psize : psize-ci.poslen,
                                            ENET_PACKET_FLAG_NO_ALLOCATE);
                sendpacket(ci.clientnum, 0, packet);
                if(!packet->referenceCount) enet_packet_destroy(packet);
                else { ++ws.uses; packet->freeCallback = freecallback; }
            }

            if(ci.state.aitype < 0 && msize && (ci.msgoff<0 || msize-ci.msglen>0))
            {
                packet = enet_packet_create(&ws.messages[ci.msgoff<0 ? 0 : ci.msgoff+ci.msglen],
                                            ci.msgoff<0 ? msize : msize-ci.msglen,
                                            (reliablemessages ? ENET_PACKET_FLAG_RELIABLE : 0) | ENET_PACKET_FLAG_NO_ALLOCATE);
                sendpacket(ci.clientnum, 1, packet);
                if(!packet->referenceCount) enet_packet_destroy(packet);
                else { ++ws.uses; packet->freeCallback = freecallback; }
            }
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

    bool sendpackets(bool force)
    {
        if(clients.empty()) return false;
        enet_uint32 millis = enet_time_get()-lastsend;
        if(millis<33 && !force) return false;
        bool flush = buildworldstate();
        lastsend += millis - (millis%33);
        return flush;
    }

    void sendclipboard(clientinfo *ci)
    {
        if(!ci->lastclipboard || !ci->clipboard) return;
        bool flushed = false;
        loopv(clients)
        {
            clientinfo &e = *clients[i];
            if(e.clientnum != ci->clientnum && e.needclipboard >= ci->lastclipboard)
            {
                if(!flushed) { flushserver(true); flushed = true; }
                sendpacket(e.clientnum, 1, ci->clipboard);
            }
        }
    }

    void parsepacket(int sender, int chan, packetbuf &p)     // has to parse exactly each byte of the packet
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
                if(!text[0]) copystring(text, "unnamed");
                filtertext(text, text, true, true, true, MAXNAMELEN);
                copystring(ci->name, text, MAXNAMELEN+1);

                string password = "", authname = "";
                getstring(text, p); copystring(password, text);
                getstring(text, p); copystring(authname, text);
                int disc = auth::allowconnect(ci, password, authname);
                if(disc)
                {
                    disconnect_client(sender, disc);
                    return;
                }

                connects.removeobj(ci);
                clients.add(ci);

                ci->connected = true;
                ci->needclipboard = totalmillis;
                masterupdate = true;
                ci->state.lasttimeplayed = lastmillis;

                sendwelcome(ci);
                if(restorescore(ci)) sendresume(ci);
                sendinitclient(ci);
                int amt = numclients();
                relayf(2, "\fg%s (%s) has joined the game (%d %s)", colorname(ci), gethostname(ci->clientnum), amt, amt != 1 ? "players" : "player");
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
                        cp->position.setsize(0);
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
                    if(!hasclient(cp, ci)) break;
                    if(idx == SPHY_EXTINGUISH)
                    {
                        if(cp->state.onfire(gamemillis, GAME(fireburntime))) cp->state.lastfire = cp->state.lastfireburn = 0;
                        else break; // don't propogate
                    }
                    QUEUE_MSG;
                    break;
                }

                case SV_EDITMODE:
                {
                    int val = getint(p);
                    if(!ci || ci->state.aitype >= 0) break;
                    if(!allowstate(ci, val ? 4 : 5) && !haspriv(ci, PRIV_MASTER, "unspectate and edit")) { spectator(ci); break; }
                    ci->state.dropped.reset();
                    loopk(WEAP_MAX) loopj(2) ci->state.weapshots[k][j].reset();
                    ci->state.editspawn(gamemillis, m_weapon(gamemode, mutators), m_health(gamemode, mutators), !m_insta(gamemode, mutators), m_arena(gamemode, mutators), GAME(spawngrenades) >= (m_insta(gamemode, mutators) ? 2 : 1));
                    if(val)
                    {
                        if(smode) smode->leavegame(ci);
                        mutate(smuts, mut->leavegame(ci));
                        ci->state.state = CS_EDITING;
                        ci->events.deletecontents();
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
                    if(!hasclient(cp, ci) || !allowstate(cp, 1)) break;
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

                case SV_LOADWEAP:
                {
                    int lcn = getint(p), aweap = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || !isweap(aweap)) break;
                    if(WEAP(aweap, allowed) < (m_insta(gamemode, mutators) ? 2 : 1))
                    {
                        if(cp->state.aitype < 0)
                        {
                            srvmsgf(cp->clientnum, "sorry, the \fs%s%s\fS has been disabled, please select a different weapon", weaptype[aweap].text, weaptype[aweap].name);
                            sendf(cp->clientnum, 1, "ri", SV_LOADWEAP);
                            break;
                        }
                        cp->state.loadweap = WEAP_MELEE;
                    }
                    cp->state.loadweap = aweap;
                    break;
                }

                case SV_WEAPSELECT:
                {
                    int lcn = getint(p), id = getint(p), weap = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci) || !isweap(weap)) break;
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
                    if(!hasclient(cp, ci) || !allowstate(cp, 2)) break;
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
                    if(!hasclient(cp, ci)) break;
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
                    if(!isweap(ev->weap)) havecn = false;
                    ev->flags = getint(p);
                    ev->power = getint(p);
                    if(havecn) ev->millis = cp->getmillis(gamemillis, ev->id);
                    loopk(3) ev->from[k] = getint(p);
                    ev->num = getint(p);
                    loopj(ev->num)
                    {
                        if(p.overread()) break;
                        ivec &dest = ev->shots.add();
                        loopk(3) dest[k] = getint(p);
                    }
                    if(havecn)
                    {
                        while(ev->shots.length() > WEAP2(ev->weap, rays, ev->flags&HIT_ALT))
                            ev->shots.remove(rnd(ev->shots.length()));
                        cp->addevent(ev);
                    }
                    else delete ev;
                    break;
                }

                case SV_DROP:
                { // gee this looks familiar
                    int lcn = getint(p), id = getint(p), weap = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!cp || (cp->clientnum != ci->clientnum && cp->state.ownernum != ci->clientnum) || !isweap(weap))
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
                    if(!cp || (cp->clientnum != ci->clientnum && cp->state.ownernum != ci->clientnum) || !isweap(weap))
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
                    ev->weap = getint(p);
                    if(!isweap(ev->weap)) havecn = false;
                    ev->flags = getint(p);
                    if(havecn) ev->millis = cp->getmillis(gamemillis, ev->id); // this is the event millis
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
                    if(!hasclient(cp, ci)) break;
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
                    if(!hasclient(cp, ci) || cp->state.state != CS_ALIVE) break;
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
                                        cp->state.cpnodes.shrink(0);
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
                    else if(GAME(serverdebug)) srvmsgf(cp->clientnum, "sync error: cannot trigger %d - not a trigger", ent);
                    break;
                }

                case SV_TEXT:
                {
                    int lcn = getint(p), flags = getint(p);
                    getstring(text, p);
                    clientinfo *cp = (clientinfo *)getinfo(lcn);
                    if(!hasclient(cp, ci)) break;
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
                    if(!hasclient(cp, ci)) break;
                    parsecommand(cp, nargs, cmd, text);
                    break;
                }

                case SV_SWITCHNAME:
                {
                    QUEUE_MSG;
                    defformatstring(oldname)("%s", colorname(ci));
                    getstring(text, p);
                    if(!text[0]) copystring(text, "unnamed");
                    filtertext(text, text, true, true, true, MAXNAMELEN);
                    copystring(ci->name, text, MAXNAMELEN+1);
                    relayf(2, "\fm* %s is now known as %s", oldname, colorname(ci));
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
                    vote(text, reqmode, reqmuts, sender);
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
                    if(!hasclient(cp, ci) || cp->state.state == CS_SPECTATOR) break;
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
                    if(!hasclient(cp, ci) || cp->state.state == CS_SPECTATOR) break;
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
                        if(haspriv(ci, PRIV_ADMIN) || (mastermask()&(1<<mm)))
                        {
                            mastermode = mm;
                            loopv(allows) if(allows[i].time >= 0) allows.remove(i--);
                            if(mm >= MM_PRIVATE)
                            {
                                loopv(clients)
                                {
                                    ipinfo &allow = allows.add();
                                    allow.time = totalmillis;
                                    allow.ip = getclientip(clients[i]->clientnum);
                                }
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
                        loopv(bans) if(bans[i].time >= 0) bans.remove(i--);
                        srvoutf(3, "\fccleared existing bans");
                    }
                    break;
                }

                case SV_KICK:
                {
                    int victim = getint(p);
                    if(haspriv(ci, PRIV_MASTER, "kick people") && victim >= 0 && ci->clientnum != victim)
                    {
                        clientinfo *cp = (clientinfo *)getinfo(victim);
                        if(!cp || !cmppriv(ci, cp, "kick")) break;
                        ipinfo &ban = bans.add();
                        ban.time = totalmillis;
                        ban.ip = getclientip(victim);
                        loopv(allows) if(allows[i].time >= 0 && allows[i].ip == ban.ip) allows.remove(i--);
                        disconnect_client(victim, DISC_KICK);
                    }
                    break;
                }

                case SV_SPECTATOR:
                {
                    int spectator = getint(p), val = getint(p);
                    clientinfo *cp = (clientinfo *)getinfo(spectator);
                    if(!cp || cp->state.aitype >= 0) break;
                    if((spectator != sender || !allowstate(cp, val ? 3 : 1)) && !haspriv(ci, PRIV_MASTER, spectator != sender ? "spectate others" : "unspectate")) break;
                    if(cp->state.state != CS_SPECTATOR && val)
                    {
                        if(cp->state.state == CS_ALIVE)
                        {
                            suicideevent ev;
                            ev.flags = 0;
                            ev.process(cp);
                        }
                        if(smode) smode->leavegame(cp);
                        mutate(smuts, mut->leavegame(cp));
                        distpoints(cp);
                        sendf(-1, 1, "ri3", SV_SPECTATOR, spectator, val);
                        cp->state.cpnodes.shrink(0);
                        cp->state.cpmillis = 0;
                        cp->state.state = CS_SPECTATOR;
                        cp->state.timeplayed += lastmillis-cp->state.lasttimeplayed;
                        setteam(cp, TEAM_NEUTRAL, false, true);
                        aiman::dorefresh = true;
                    }
                    else if(cp->state.state == CS_SPECTATOR && !val)
                    {
                        cp->state.cpnodes.shrink(0);
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
                    if(oldtype == PLAYERSTART || sents[n].type == PLAYERSTART) setupspawns(true);
                    hasgameinfo = true;
                    QUEUE_MSG;
                    if(tweaked)
                    {
                        sents[n].spawned = false;
                        sents[n].millis = gamemillis+GAME(itemspawndelay);
                        if(sents[n].type == TRIGGER) setuptriggers(true);
                    }
                    break;
                }

                case SV_EDITVAR:
                {
                    int t = getint(p);
                    getstring(text, p);
                    if(!ci || ci->state.state != CS_EDITING)
                    {
                        switch(t)
                        {
                            case ID_VAR: getint(p); break;
                            case ID_FVAR: getfloat(p); break;
                            case ID_SVAR: case ID_ALIAS: getstring(text, p); break;
                            default: break;
                        }
                        break;
                    }
                    QUEUE_INT(SV_EDITVAR);
                    QUEUE_INT(t);
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
                    clientinfo *best = choosebestclient();
                    ci->wantsmap = true;
                    if(ci == best) // we asked them for the map and they asked us, oops
                    {
                        mapsending = false;
                        best = choosebestclient();
                    }
                    if(!mapsending)
                    {
                        if(mapdata[0] && mapdata[1] && mapdata[2])
                        {
                            srvmsgf(ci->clientnum, "sending map, please wait..");
                            loopk(3) if(mapdata[k]) sendfile(sender, 2, mapdata[k], "ri", SV_SENDMAPFILE+k);
                            sendwelcome(ci);
                            ci->needclipboard = totalmillis;
                        }
                        else if(best)
                        {
                            loopk(3) if(mapdata[k]) DELETEP(mapdata[k]);
                            srvmsgf(ci->clientnum, "map is being requested, please wait..");
                            sendf(best->clientnum, 1, "ri", SV_GETMAP);
                            mapsending = true;
                        }
                        else srvmsgf(ci->clientnum, "there doesn't seem to be anybody to get the map from!");
                    }
                    else srvmsgf(ci->clientnum, "map is being uploaded, please be patient..");
                    break;
                }

                case SV_NEWMAP:
                {
                    int size = getint(p);
                    if(ci->state.state == CS_SPECTATOR) break;
                    if(size >= 0)
                    {
                        smapname[0] = '\0';
                        sents.shrink(0);
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
                    if(val != 0)
                    {
                        if(adminpass[0] && (ci->local || (text[0] && checkpassword(ci, adminpass, text))))
                            auth::setmaster(ci, true, PRIV_ADMIN);
                        else
                        {
                            bool fail = false;
                            if(ci->authlevel <= PRIV_NONE)
                            {
                                if(!(mastermask()&MM_AUTOAPPROVE) && !ci->privilege)
                                {
                                    srvmsgf(ci->clientnum, "\fraccess denied, you need auth/admin access to gain master");
                                    fail = true;
                                }
                                else
                                {
                                    loopv(clients) if(ci != clients[i] && clients[i]->privilege >= PRIV_MASTER)
                                    {
                                        srvmsgf(ci->clientnum, "\fraccess denied, there is already another master");
                                        fail = true;
                                        break;
                                    }
                                }
                            }
                            if(!fail) auth::setmaster(ci, true, ci->authlevel > PRIV_NONE ? ci->authlevel : PRIV_MASTER);
                        }
                    }
                    else auth::setmaster(ci, false);
                    break; // don't broadcast the master password
                }

                case SV_ADDBOT: getint(p); break;
                case SV_DELBOT: break;

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

                case SV_COPY:
                    ci->cleanclipboard();
                    ci->lastclipboard = totalmillis;
                    goto genericmsg;

                case SV_PASTE:
                    if(ci->state.state!=CS_SPECTATOR) sendclipboard(ci);
                    goto genericmsg;

                case SV_CLIPBOARD:
                {
                    int unpacklen = getint(p), packlen = getint(p);
                    ci->cleanclipboard(false);
                    if(ci->state.state==CS_SPECTATOR)
                    {
                        if(packlen > 0) p.subbuf(packlen);
                        break;
                    }
                    if(packlen <= 0 || packlen > (1<<16) || unpacklen <= 0)
                    {
                        if(packlen > 0) p.subbuf(packlen);
                        packlen = unpacklen = 0;
                    }
                    packetbuf q(32 + packlen, ENET_PACKET_FLAG_RELIABLE);
                    putint(q, SV_CLIPBOARD);
                    putint(q, ci->clientnum);
                    putint(q, unpacklen);
                    putint(q, packlen);
                    if(packlen > 0) p.get(q.subbuf(packlen).buf, packlen);
                    ci->clipboard = q.finalize();
                    ci->clipboard->referenceCount++;
                    break;
                }

                case -1:
                    conoutf("\fy[tag error] from: %d, cur: %d, msg: %d, prev: %d", sender, curtype, type, prevtype);
                    disconnect_client(sender, DISC_TAGT);
                    return;

                case -2:
                    disconnect_client(sender, DISC_OVERFLOW);
                    return;

                default: genericmsg:
                {
                    int size = msgsizelookup(type);
                    if(size<=0)
                    {
                        conoutf("\fy[tag error] from: %d, cur: %d, msg: %d, prev: %d", sender, curtype, type, prevtype);
                        disconnect_client(sender, DISC_TAGT);
                        return;
                    }
                    loopi(size-1) getint(p);
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
            case 'P': setsvar("adminpass", &arg[3]); return true;
            case 'k': setsvar("serverpass", &arg[3]); return true;
            default: break;
        }
        return false;
    }
};
#undef GAMESERVER
