#include "game.h"
namespace client
{
    bool sendinfo = false, isready = false, remote = false,
        demoplayback = false, needsmap = false, gettingmap = false, sendcrc = false;
    int lastping = 0, sessionid = 0;
    string connectpass = "";
    int needclipboard = -1;

    struct mapvote
    {
        vector<gameent *> players;
        string map;
        int millis, mode, muts;

        mapvote() {}
        ~mapvote() { players.shrink(0); }
    };
    vector<mapvote> mapvotes;

    extern int votecmp(mapvote *a, mapvote *b);
    VARF(IDF_PERSIST, sortvotes, 0, 0, 1, {
        mapvotes.sort(votecmp);
    });
    VARF(IDF_PERSIST, cleanvotes, 0, 0, 1, {
        if(cleanvotes && !mapvotes.empty()) loopvrev(mapvotes) if(mapvotes[i].players.empty()) mapvotes.remove(i);
    });

    int votecmp(mapvote *a, mapvote *b)
    {
        if(sortvotes)
        {
            if(a->players.length() > b->players.length()) return -1;
            if(b->players.length() > a->players.length()) return 1;
        }
        if(a->millis < b->millis) return -1;
        if(b->millis < a->millis) return 1;
        return 0;
    }

    void clearvotes(gameent *d)
    {
        int found = 0;
        loopvrev(mapvotes) if(mapvotes[i].players.find(d) >= 0)
        {
            mapvotes[i].players.removeobj(d); found++;
            if(cleanvotes && mapvotes[i].players.empty()) mapvotes.remove(i);
        }
        if(found && !mapvotes.empty()) mapvotes.sort(votecmp);
    }

    void vote(gameent *d, const char *text, int mode, int muts)
    {
        mapvote *m = NULL;
        if(!mapvotes.empty()) loopvrev(mapvotes)
        {
            if(mapvotes[i].players.find(d) >= 0)
            {
                if(!strcmp(text, mapvotes[i].map) && mode == mapvotes[i].mode && muts == mapvotes[i].muts) return;
                mapvotes[i].players.removeobj(d);
                if(cleanvotes && mapvotes[i].players.empty()) mapvotes.remove(i);
            }
            if(!strcmp(text, mapvotes[i].map) && mode == mapvotes[i].mode && muts == mapvotes[i].muts) m = &mapvotes[i];
        }
        if(!m)
        {
            m = &mapvotes.add();
            copystring(m->map, text);
            m->mode = mode;
            m->muts = muts;
            m->millis = totalmillis;
        }
        m->players.add(d);
        mapvotes.sort(votecmp);
        SEARCHBINDCACHE(votekey)("showgui maps 2", 0);
        SEARCHBINDCACHE(gamekey)("showgui maps 1", 0);
        conoutft(CON_MESG, "%s suggests: \fs\fy%s\fS on map \fs\fo%s\fS, press \fs\fc%s\fS to vote or \fs\fc%s\fS to select your own", game::colorname(d), server::gamename(mode, muts), text, votekey, gamekey);
    }

    void getvotes(int vote, int player)
    {
        if(!vote) intret(mapvotes.length());
        else
        {
            mkstring(text); vote--; player--;
            if(mapvotes.inrange(vote))
            {
                if(mapvotes[vote].players.inrange(player)) formatstring(text)("%d", mapvotes[vote].players[player]->clientnum);
                else formatstring(text)("%d %d %d \"%s\"", mapvotes[vote].players.length(), mapvotes[vote].mode, mapvotes[vote].muts, escapetext(mapvotes[vote].map));
            }
            result(text);
        }
    }
    ICOMMAND(0, getvote, "ii", (int *num, int *player), getvotes(*num, *player));

    string authname = "", authkey = "";

    void setauthkey(const char *name, const char *key)
    {
        copystring(authname, name);
        copystring(authkey, key);
    }
    ICOMMAND(0, authkey, "ss", (char *name, char *key), setauthkey(name, key));

    // collect c2s messages conveniently
    vector<uchar> messages;
    bool messagereliable = false;

    VAR(IDF_PERSIST, colourchat, 0, 1, 1);
    VAR(IDF_PERSIST, showlaptimes, 0, 2, 3); // 0 = off, 1 = only player, 2 = +humans, 3 = +bots
    SVAR(IDF_PERSIST, serversort, "");

    ICOMMAND(0, mastermode, "i", (int *val), addmsg(N_MASTERMODE, "ri", *val));
    ICOMMAND(0, getname, "", (), result(escapetext(game::player1->name)));
    ICOMMAND(0, getteam, "", (), result(teamtype[game::player1->team].name));
    ICOMMAND(0, getteamicon, "", (), result(hud::teamtex(game::player1->team)));

    const char *getname() { return game::player1->name; }

    void switchname(const char *name)
    {
        if(name[0])
        {
            string text; filtertext(text, name);
            copystring(game::player1->name, text, MAXNAMELEN);
            addmsg(N_SWITCHNAME, "rs", game::player1->name);
        }
        else conoutft(CON_MESG, "\fgyour name is: %s", *game::player1->name ? game::colorname(game::player1) : "<not set>");
    }
    ICOMMAND(0, name, "s", (char *s), switchname(s));

    int teamname(const char *team)
    {
        if(m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
        {
            if(team[0])
            {
                int t = atoi(team);

                loopi(numteams(game::gamemode, game::mutators))
                {
                    if((t && t == i+TEAM_FIRST) || !strcasecmp(teamtype[i+TEAM_FIRST].name, team))
                    {
                        return i+TEAM_FIRST;
                    }
                }
            }
            return TEAM_FIRST;
        }
        return TEAM_NEUTRAL;
    }

    void switchteam(const char *team)
    {
        if(team[0])
        {
            if(m_fight(game::gamemode) && m_team(game::gamemode, game::mutators) && game::player1->state != CS_SPECTATOR && game::player1->state != CS_EDITING)
            {
                int t = teamname(team);
                if(t != game::player1->team)
                {
                    if(game::player1->team != t) hud::lastteam = 0;
                    game::player1->team = t;
                    addmsg(N_SWITCHTEAM, "ri", game::player1->team);
                }
            }
            else conoutft(CON_MESG, "\frcan only change teams when actually playing in team games");
        }
        else conoutft(CON_MESG, "\fs\fgyour team is:\fS \fs%s%s\fS", teamtype[game::player1->team].chat, teamtype[game::player1->team].name);
    }
    ICOMMAND(0, team, "s", (char *s), switchteam(s));

    void writeclientinfo(stream *f)
    {
        f->printf("name \"%s\"\n\n", game::player1->name);
    }

    void connectattempt(const char *name, int port, const char *password, const ENetAddress &address)
    {
        if(*password) { copystring(connectpass, password); }
        else memset(connectpass, 0, sizeof(connectpass));
    }

    void connectfail()
    {
        memset(connectpass, 0, sizeof(connectpass));
    }

    void gameconnect(bool _remote)
    {
        remote = _remote;
        if(editmode) toggleedit();
    }

    void gamedisconnect(int clean)
    {
        if(editmode) toggleedit();
        gettingmap = needsmap = remote = isready = sendinfo = false;
        sessionid = 0;
        messages.shrink(0);
        mapvotes.shrink(0);
        messagereliable = false;
        projs::remove(game::player1);
        removetrackedparticles(game::player1);
        removetrackedsounds(game::player1);
        game::player1->clientnum = -1;
        game::player1->privilege = PRIV_NONE;
        game::gamemode = G_EDITMODE;
        game::mutators = 0;
        loopv(game::players) if(game::players[i]) game::clientdisconnected(i);
        emptymap(0, true, NULL, true);
        smartmusic(true, false);
        enumerate(*idents, ident, id, {
            if(id.flags&IDF_CLIENT) switch(id.type)
            {
                case ID_VAR: setvar(id.name, id.def.i, true); break;
                case ID_FVAR: setfvar(id.name, id.def.f, true); break;
                case ID_SVAR: setsvar(id.name, *id.def.s ? id.def.s : "", true); break;
                default: break;
            }
        });
        if(clean) game::clientmap[0] = '\0';
    }

    bool allowedittoggle(bool edit)
    {
        bool allow = edit || m_edit(game::gamemode); // && game::player1->state == CS_ALIVE);
        if(!allow) conoutft(CON_MESG, "\fryou must be in edit mode to start editing");
        return allow;
    }

    void edittoggled(bool edit)
    {
        game::player1->editspawn(lastmillis, m_weapon(game::gamemode, game::mutators), m_health(game::gamemode, game::mutators), m_insta(game::gamemode, game::mutators), m_arena(game::gamemode, game::mutators), spawngrenades >= (m_insta(game::gamemode, game::mutators) ? 2 : 1));
        game::player1->state = edit ? CS_EDITING : (m_edit(game::gamemode) ? CS_ALIVE : CS_DEAD);
        game::player1->resetinterp();
        game::resetstate();
        physics::entinmap(game::player1, true); // find spawn closest to current floating pos
        projs::remove(game::player1);
        if(m_edit(game::gamemode)) addmsg(N_EDITMODE, "ri", edit ? 1 : 0);
        entities::edittoggled(edit);
    }

    const char *getclientname(int cn, int colour)
    {
        gameent *d = game::getclient(cn);
        if(colour && d) return game::colorname(d);
        return d ? d->name : "";
    }
    ICOMMAND(0, getclientname, "ii", (int *cn, int *colour), result(escapetext(getclientname(*cn, *colour))));

    int getclientteam(int cn)
    {
        gameent *d = game::getclient(cn);
        return d ? d->team : -1;
    }
    ICOMMAND(0, getclientteam, "i", (int *cn), intret(getclientteam(*cn)));

    bool ismaster(int cn)
    {
        gameent *d = game::getclient(cn);
        return d && d->privilege >= PRIV_MASTER;
    }
    ICOMMAND(0, ismaster, "i", (int *cn), intret(ismaster(*cn) ? 1 : 0));

    bool isadmin(int cn)
    {
        gameent *d = game::getclient(cn);
        return d && d->privilege >= PRIV_ADMIN;
    }
    ICOMMAND(0, isadmin, "i", (int *cn), intret(isadmin(*cn) ? 1 : 0));

    bool isspectator(int cn)
    {
        gameent *d = game::getclient(cn);
        return d && d->state == CS_SPECTATOR;
    }
    ICOMMAND(0, isspectator, "i", (int *cn), intret(isspectator(*cn) ? 1 : 0));

    bool isai(int cn, int type)
    {
        gameent *d = game::getclient(cn);
        int aitype = type > 0 && type < AI_MAX ? type : AI_BOT;
        return d && d->aitype == aitype;
    }
    ICOMMAND(0, isai, "ii", (int *cn, int *type), intret(isai(*cn, *type) ? 1 : 0));

    int parseplayer(const char *arg)
    {
        char *end;
        int n = strtol(arg, &end, 10);
        if(*arg && !*end)
        {
            if(n!=game::player1->clientnum && !game::players.inrange(n)) return -1;
            return n;
        }
        // try case sensitive first
        loopv(game::players) if(game::players[i])
        {
            gameent *o = game::players[i];
            if(!strcmp(arg, o->name)) return o->clientnum;
        }
        // nothing found, try case insensitive
        loopv(game::players) if(game::players[i])
        {
            gameent *o = game::players[i];
            if(!strcasecmp(arg, o->name)) return o->clientnum;
        }
        return -1;
    }
    ICOMMAND(0, getclientnum, "s", (char *name), intret(name[0] ? parseplayer(name) : game::player1->clientnum));

    void listclients(bool local)
    {
        vector<char> buf;
        string cn;
        int numclients = 0;
        if(local)
        {
            formatstring(cn)("%d", game::player1->clientnum);
            buf.put(cn, strlen(cn));
            numclients++;
        }
        loopv(game::players) if(game::players[i])
        {
            formatstring(cn)("%d", game::players[i]->clientnum);
            if(numclients++) buf.add(' ');
            buf.put(cn, strlen(cn));
        }
        buf.add('\0');
        result(buf.getbuf());
    }
    ICOMMAND(0, listclients, "i", (int *local), listclients(*local!=0));

    void clearbans()
    {
        addmsg(N_CLEARBANS, "r");
    }
    ICOMMAND(0, clearbans, "", (char *s), clearbans());

    void kick(const char *arg)
    {
        int i = parseplayer(arg);
        if(i>=0 && i!=game::player1->clientnum) addmsg(N_KICK, "ri", i);
    }
    ICOMMAND(0, kick, "s", (char *s), kick(s));

    void setteam(const char *arg1, const char *arg2)
    {
        if(m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
        {
            int i = parseplayer(arg1);
            if(i>=0 && i!=game::player1->clientnum)
            {
                int t = teamname(arg2);
                if(t) addmsg(N_SETTEAM, "ri2", i, t);
            }
        }
        else conoutft(CON_MESG, "\frcan only change teams in team games");
    }
    ICOMMAND(0, setteam, "ss", (char *who, char *team), setteam(who, team));

    void hashpwd(const char *pwd)
    {
        if(game::player1->clientnum<0) return;
        string hash;
        server::hashpassword(game::player1->clientnum, sessionid, pwd, hash);
        result(hash);
    }
    COMMAND(0, hashpwd, "s");

    void setmaster(const char *arg)
    {
        if(!arg[0]) return;
        int val = 1;
        mkstring(hash);
        if(!arg[1] && isdigit(arg[0])) val = atoi(arg);
        else server::hashpassword(game::player1->clientnum, sessionid, arg, hash);
        addmsg(N_SETMASTER, "ris", val, hash);
    }
    COMMAND(0, setmaster, "s");

    void tryauth()
    {
        if(!authname[0]) return;
        addmsg(N_AUTHTRY, "rs", authname);
    }
    ICOMMAND(0, auth, "", (), tryauth());

    void togglespectator(int val, const char *who)
    {
        int i = who[0] ? parseplayer(who) : game::player1->clientnum;
        if(i>=0) addmsg(N_SPECTATOR, "rii", i, val);
    }
    ICOMMAND(0, spectator, "is", (int *val, char *who), togglespectator(*val, who));

    ICOMMAND(0, checkmaps, "", (), addmsg(N_CHECKMAPS, "r"));

    void addmsg(int type, const char *fmt, ...)
    {
        static uchar buf[MAXTRANS];
        ucharbuf p(buf, MAXTRANS);
        putint(p, type);
        int numi = 1, nums = 0;
        bool reliable = false;
        if(fmt)
        {
            va_list args;
            va_start(args, fmt);
            while(*fmt) switch(*fmt++)
            {
                case 'r': reliable = true; break;
                case 'v':
                {
                    int n = va_arg(args, int);
                    int *v = va_arg(args, int *);
                    loopi(n) putint(p, v[i]);
                    numi += n;
                    break;
                }
                case 'i':
                {
                    int n = isdigit(*fmt) ? *fmt++-'0' : 1;
                    loopi(n) putint(p, va_arg(args, int));
                    numi += n;
                    break;
                }
                case 'f':
                {
                    int n = isdigit(*fmt) ? *fmt++-'0' : 1;
                    loopi(n) putfloat(p, (float)va_arg(args, double));
                    numi += n;
                    break;
                }
                case 's': sendstring(va_arg(args, const char *), p); nums++; break;
            }
            va_end(args);
        }
        int num = nums?0:numi, msgsize = msgsizelookup(type);
        if(msgsize && num!=msgsize) { defformatstring(s)("inconsistent msg size for %d (%d != %d)", type, num, msgsize); fatal(s); }
        if(reliable) messagereliable = true;
        messages.put(buf, p.length());
    }

    void saytext(gameent *d, int flags, char *text)
    {
        filtertext(text, text, true, colourchat ? false : true); mkstring(s);
        defformatstring(m)("%s", game::colorname(d));
        if(flags&SAY_TEAM)
        {
            defformatstring(t)(" (\fs%s%s\fS)", teamtype[d->team].chat, teamtype[d->team].name);
            concatstring(m, t);
        }
        if(flags&SAY_ACTION) formatstring(s)("\fv* \fs%s\fS \fs\fv%s\fS", m, text);
        else formatstring(s)("\fa<\fs\fw%s\fS> \fs\fw%s\fS", m, text);

        if(d->state != CS_SPECTATOR)
        {
            defformatstring(ds)("<sub>%s", s);
            part_textcopy(d->abovehead(), ds, PART_TEXT, game::aboveheadfade, 0xFFFFFF, 2, 1, -10, 0, d);
        }
        int snd = S_CHAT;
        ident *wid = idents->access(flags&SAY_ACTION ? "on_action" : "on_text");
        if(wid && wid->action)
        {
            defformatstring(fn)("%s", escapetext(game::colorname(d)));
            defformatstring(ft)("%s", escapetext(text));
            defformatstring(fs)("%s", escapetext(s));
            defformatstring(act)("%s %d %d \"%s\" \"%s\" \"%s\"", flags&SAY_ACTION ? "on_action" : "on_text", d->clientnum, flags&SAY_TEAM ? 1 : 0, fn, ft, fs);
            int ret = execute(act);
            if((ret >= S_PAIN1 && ret <= S_DIE2) || ret >= S_MAX) snd = ret;
        }
        conoutft(CON_CHAT, "%s", s);
        playsound(snd, d->o, d, d == game::focus ? SND_FORCED : SND_DIRECT, 255-int(camera1->o.dist(d->o)/(getworldsize()/2)*200));
    }

    void toserver(int flags, char *text)
    {
        if(ready())
        {
            saytext(game::player1, flags, text);
            addmsg(N_TEXT, "ri2s", game::player1->clientnum, flags, text);
        }
    }
    ICOMMAND(0, say, "C", (char *s), toserver(SAY_NONE, s));
    ICOMMAND(0, me, "C", (char *s), toserver(SAY_ACTION, s));
    ICOMMAND(0, sayteam, "C", (char *s), toserver(SAY_TEAM, s));
    ICOMMAND(0, meteam, "C", (char *s), toserver(SAY_ACTION|SAY_TEAM, s));

    void parsecommand(gameent *d, const char *cmd, const char *arg)
    {
        ident *id = idents->access(cmd);
        if(id && id->flags&IDF_CLIENT)
        {
            string val;
            val[0] = 0;
            switch(id->type)
            {
                case ID_COMMAND: case ID_CCOMMAND:
                {
#if 0 // these shouldn't get here
                    string s;
                    formatstring(s)("%s %s", cmd, arg);
                    char *ret = executeret(s);
                    if(ret)
                    {
                        conoutft(CON_MESG, "\fg%s: %s", cmd, ret);
                        delete[] ret;
                    }
#endif
                    return;
                }
                case ID_VAR:
                {
                    int ret = atoi(arg);
                    *id->storage.i = ret;
                    id->changed();
                    formatstring(val)(id->flags&IDF_HEX ? (id->maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id->storage.i);
                    break;
                }
                case ID_FVAR:
                {
                    float ret = atof(arg);
                    *id->storage.f = ret;
                    id->changed();
                    formatstring(val)("%s", floatstr(*id->storage.f));
                    break;
                }
                case ID_SVAR:
                {
                    delete[] *id->storage.s;
                    *id->storage.s = newstring(arg);
                    id->changed();
                    formatstring(val)("%s", *id->storage.s);
                    break;
                }
                default: return;
            }
            if(d || verbose >= 2)
                conoutft(d != game::player1 ? CON_INFO : CON_MESG, "\fg%s set %s to %s", d ? game::colorname(d) : "the server", cmd, val);
        }
        else conoutft(d != game::player1 ? CON_INFO : CON_MESG, "\fr%s sent unknown command: %s", d ? game::colorname(d) : "the server", cmd);
    }

    bool sendcmd(int nargs, const char *cmd, const char *arg)
    {
        if(ready())
        {
            addmsg(N_COMMAND, "ri2ss", game::player1->clientnum, nargs, cmd, arg);
            return true;
        }
        else
        {
            defformatstring(scmd)("sv_%s", cmd);
            if(server::servcmd(nargs, scmd, arg))
            {
                parsecommand(NULL, cmd, arg);
                return true;
            }
        }
        return false;
    }

    void changemapserv(char *name, int gamemode, int mutators, bool temp)
    {
        game::gamemode = gamemode; game::mutators = mutators;
        server::modecheck(game::gamemode, game::mutators);
        game::nextmode = game::gamemode; game::nextmuts = game::mutators;
        game::timeremaining = -1;
        game::maptime = 0;
        mapvotes.shrink(0);
        if(editmode) toggleedit();
        if(m_demo(gamemode)) return;
        needsmap = false;
        if(!name || !*name || !load_world(name, temp))
        {
            emptymap(0, true, NULL);
            setnames(name, MAP_BFGZ);
            needsmap = true;
        }
        if(m_stf(gamemode)) stf::setupflags();
        else if(m_ctf(gamemode)) ctf::setupflags();
    }

    void receivefile(uchar *data, int len)
    {
        ucharbuf p(data, len);
        int type = getint(p);
        data += p.length();
        len -= p.length();
        switch(type)
        {
            case N_SENDDEMO:
            {
                defformatstring(fname)("%d.dmo", totalmillis);
                stream *demo = openfile(fname, "wb");
                if(!demo) return;
                conoutft(CON_MESG, "received demo \"%s\"", fname);
                demo->write(data, len);
                delete demo;
                break;
            }

            case N_SENDMAPCONFIG:
            case N_SENDMAPSHOT:
            case N_SENDMAPFILE:
            {
                const char *reqmap = mapname, *reqext = "xxx";
                if(type == N_SENDMAPCONFIG) reqext = "cfg";
                else if(type == N_SENDMAPSHOT) reqext = "png";
                else if(type == N_SENDMAPFILE) reqext = "bgz";
                if(!reqmap || !*reqmap) reqmap = "maps/untitled";
                defformatstring(reqfile)(strstr(reqmap, "temp/")==reqmap || strstr(reqmap, "temp\\")==reqmap ? "%s" : "temp/%s", reqmap);
                defformatstring(reqfext)("%s.%s", reqfile, reqext);
                stream *f = openfile(reqfext, "wb");
                if(!f)
                {
                    conoutft(CON_MESG, "\frfailed to open map file: %s", reqfext);
                    return;
                }
                gettingmap = true;
                f->write(data, len);
                delete f;
                break;
            }
        }
        return;
    }
    ICOMMAND(0, getmap, "", (), if(multiplayer(false)) addmsg(N_GETMAP, "r"));

    void stopdemo()
    {
        if(remote) addmsg(N_STOPDEMO, "r");
        else server::stopdemo();
    }
    ICOMMAND(0, stopdemo, "", (), stopdemo());

    void recorddemo(int val)
    {
        addmsg(N_RECORDDEMO, "ri", val);
    }
    ICOMMAND(0, recorddemo, "i", (int *val), recorddemo(*val));

    void cleardemos(int val)
    {
        addmsg(N_CLEARDEMOS, "ri", val);
    }
    ICOMMAND(0, cleardemos, "i", (int *val), cleardemos(*val));

    void getdemo(int i)
    {
        if(i<=0) conoutft(CON_MESG, "getting demo...");
        else conoutft(CON_MESG, "getting demo %d...", i);
        addmsg(N_GETDEMO, "ri", i);
    }
    ICOMMAND(0, getdemo, "i", (int *val), getdemo(*val));

    void listdemos()
    {
        conoutft(CON_MESG, "listing demos...");
        addmsg(N_LISTDEMOS, "r");
    }
    ICOMMAND(0, listdemos, "", (), listdemos());

    void changemap(const char *name) // request map change, server may ignore
    {
        int nextmode = game::nextmode, nextmuts = game::nextmuts; // in case stopdemo clobbers these
        if(!remote) stopdemo();
        if(!connected())
        {
            server::changemap(name, nextmode, nextmuts);
            localconnect(true);
        }
        else
        {
            string reqfile;
            copystring(reqfile, !strncasecmp(name, "temp/", 5) || !strncasecmp(name, "temp\\", 5) ? name+5 : name);
            addmsg(N_MAPVOTE, "rsi2", reqfile, nextmode, nextmuts);
        }
    }
    ICOMMAND(0, map, "s", (char *s), changemap(s));

    void sendmap()
    {
        conoutft(CON_MESG, "sending map...");
        const char *reqmap = mapname;
        if(!reqmap || !*reqmap) reqmap = "maps/untitled";
        bool edit = m_edit(game::gamemode);
        defformatstring(reqfile)("%s%s", edit ? "temp/" : "", reqmap);
        loopi(3)
        {
            string reqfext;
            switch(i)
            {
                case 2: formatstring(reqfext)("%s.cfg", reqfile); break;
                case 1: formatstring(reqfext)("%s.png", reqfile); break;
                case 0: default:
                    formatstring(reqfext)("%s.bgz", reqfile);
                    if(edit)
                    {
                        save_world(reqfile, edit, true);
                        setnames(reqmap, MAP_BFGZ);
                    }
                    break;
            }
            stream *f = openfile(reqfext, "rb");
            if(f)
            {
                conoutft(CON_MESG, "\fgtransmitting file: %s", reqfext);
                sendfile(-1, 2, f, "ri", N_SENDMAPFILE+i);
                if(needclipboard >= 0) needclipboard++;
                delete f;
            }
            else conoutft(CON_MESG, "\frfailed to open map file: %s", reqfext);
        }
    }
    ICOMMAND(0, sendmap, "", (), sendmap());

    void gotoplayer(const char *arg)
    {
        if(game::player1->state!=CS_SPECTATOR && game::player1->state!=CS_EDITING) return;
        int i = parseplayer(arg);
        if(i>=0 && i!=game::player1->clientnum)
        {
            gameent *d = game::getclient(i);
            if(!d) return;
            game::player1->o = d->o;
            vec dir;
            vecfromyawpitch(game::player1->yaw, game::player1->pitch, 1, 0, dir);
            game::player1->o.add(dir.mul(-32));
            game::player1->resetinterp();
        }
    }
    ICOMMAND(0, goto, "s", (char *s), gotoplayer(s));

    bool ready() { return connected(false) && isready && game::maptime > 0; }
    ICOMMAND(0, ready, "", (), intret(ready()));

    int state() { return game::player1->state; }
    ICOMMAND(0, getstate, "", (), intret(state()));
    int otherclients()
    {
        int n = 0; // ai don't count
        loopv(game::players) if(game::players[i] && game::players[i]->aitype < 0) n++;
        return n;
    }

    void editvar(ident *id, bool local)
    {
        if(id && id->flags&IDF_WORLD && local && m_edit(game::gamemode))
        {
            switch(id->type)
            {
                case ID_VAR:
                    addmsg(N_EDITVAR, "risi", id->type, id->name, *id->storage.i);
                    break;
                case ID_FVAR:
                    addmsg(N_EDITVAR, "risf", id->type, id->name, *id->storage.f);
                    break;
                case ID_SVAR:
                    addmsg(N_EDITVAR, "riss", id->type, id->name, *id->storage.s);
                    break;
                case ID_ALIAS:
                    addmsg(N_EDITVAR, "riss", id->type, id->name, id->action);
                    break;
                default: break;
            }
        }
    }

    void sendclipboard()
    {
        uchar *outbuf = NULL;
        int inlen = 0, outlen = 0;
        if(!packeditinfo(localedit, inlen, outbuf, outlen))
        {
            outbuf = NULL;
            inlen = outlen = 0;
        }
        packetbuf p(16 + outlen, ENET_PACKET_FLAG_RELIABLE);
        putint(p, N_CLIPBOARD);
        putint(p, inlen);
        putint(p, outlen);
        if(outlen > 0) p.put(outbuf, outlen);
        sendclientpacket(p.finalize(), 1);
        needclipboard = -1;
    }

    void edittrigger(const selinfo &sel, int op, int arg1, int arg2, int arg3)
    {
        if(m_edit(game::gamemode) && game::player1->state == CS_EDITING) switch(op)
        {
            case EDIT_FLIP:
            case EDIT_COPY:
            case EDIT_PASTE:
            case EDIT_DELCUBE:
            {
                switch(op)
                {
                    case EDIT_COPY: needclipboard = 0; break;
                    case EDIT_PASTE:
                        if(needclipboard > 0)
                        {
                            c2sinfo(true);
                            sendclipboard();
                        }
                        break;
                }
                addmsg(N_EDITF + op, "ri9i4",
                    sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
                    sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner);
                break;
            }
            case EDIT_MAT:
            case EDIT_ROTATE:
            {
                addmsg(N_EDITF + op, "ri9i5",
                    sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
                    sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
                    arg1);
                break;
            }
            case EDIT_FACE:
            case EDIT_TEX:
            case EDIT_REPLACE:
            {
                addmsg(N_EDITF + op, "ri9i6",
                    sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
                    sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
                    arg1, arg2);
                break;
            }
            case EDIT_REMIP:
            {
                addmsg(N_EDITF + op, "r");
                break;
            }
        }
    }

    void sendintro()
    {
        packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        putint(p, N_CONNECT);
        sendstring(game::player1->name, p);
        mkstring(hash);
        if(connectpass[0])
        {
            server::hashpassword(game::player1->clientnum, sessionid, connectpass, hash);
            memset(connectpass, 0, sizeof(connectpass));
        }
        sendstring(hash, p);
        sendstring(authname, p);
        sendclientpacket(p.finalize(), 1);
    }

    static void sendposition(gameent *d, packetbuf &q)
    {
        putint(q, N_POS);
        putuint(q, d->clientnum);
        // 3 bits phys state, 1 bit sprinting, 2 bits move, 2 bits strafe
        uchar physstate = d->physstate | ((physics::sprinting(d) ? 1 : 0)<<3) | ((d->move&3)<<4) | ((d->strafe&3)<<6);
        q.put(physstate);
        ivec o = ivec(vec(d->o.x, d->o.y, d->o.z-d->height).mul(DMF));
        uint vel = min(int(d->vel.magnitude()*DVELF), 0xFFFF), fall = min(int(d->falling.magnitude()*DVELF), 0xFFFF);
        // 3 bits position, 1 bit velocity, 3 bits falling, 1 bit aim, 1 bit crouching, 1 bit conopen
        uint flags = 0;
        if(o.x < 0 || o.x > 0xFFFF) flags |= 1<<0;
        if(o.y < 0 || o.y > 0xFFFF) flags |= 1<<1;
        if(o.z < 0 || o.z > 0xFFFF) flags |= 1<<2;
        if(vel > 0xFF) flags |= 1<<3;
        if(fall > 0)
        {
            flags |= 1<<4;
            if(fall > 0xFF) flags |= 1<<5;
            if(d->falling.x || d->falling.y || d->falling.z > 0) flags |= 1<<6;
        }
        if((int)d->aimyaw!=(int)d->yaw || (int)d->aimpitch!=(int)d->pitch) flags |= 1<<7;
        if(d->action[AC_CROUCH]) flags |= 1<<8;
        if(d->conopen) flags |= 1<<9;
        putuint(q, flags);
        loopk(3)
        {
            q.put(o[k]&0xFF);
            q.put((o[k]>>8)&0xFF);
            if(o[k] < 0 || o[k] > 0xFFFF) q.put((o[k]>>16)&0xFF);
        }
        uint dir = (d->yaw < 0 ? 360 + int(d->yaw)%360 : int(d->yaw)%360) + clamp(int(d->pitch+90), 0, 180)*360;
        q.put(dir&0xFF);
        q.put((dir>>8)&0xFF);
        q.put(clamp(int(d->roll+90), 0, 180));
        q.put(vel&0xFF);
        if(vel > 0xFF) q.put((vel>>8)&0xFF);
        float velyaw, velpitch;
        vectoyawpitch(d->vel, velyaw, velpitch);
        uint veldir = (velyaw < 0 ? 360 + int(velyaw)%360 : int(velyaw)%360) + clamp(int(velpitch+90), 0, 180)*360;
        q.put(veldir&0xFF);
        q.put((veldir>>8)&0xFF);
        if(fall > 0)
        {
            q.put(fall&0xFF);
            if(fall > 0xFF) q.put((fall>>8)&0xFF);
            if(d->falling.x || d->falling.y || d->falling.z > 0)
            {
                float fallyaw, fallpitch;
                vectoyawpitch(d->falling, fallyaw, fallpitch);
                uint falldir = (fallyaw < 0 ? 360 + int(fallyaw)%360 : int(fallyaw)%360) + clamp(int(fallpitch+90), 0, 180)*360;
                q.put(falldir&0xFF);
                q.put((falldir>>8)&0xFF);
            }
        }
        if((int)d->aimyaw!=(int)d->yaw || (int)d->aimpitch!=(int)d->pitch)
        {
            uint aimdir = (d->aimyaw < 0 ? 360 + int(d->aimyaw)%360 : int(d->aimyaw)%360) + clamp(int(d->aimpitch+90), 0, 180)*360;
            q.put(aimdir&0xFF);
            q.put((aimdir>>8)&0xFF);
        }
    }

    void sendpositions()
    {
        gameent *d = NULL;
        loopi(game::numdynents()) if((d = (gameent *)game::iterdynents(i)))
        {
            if((d == game::player1 || (d->ai && !d->ai->suspended)) && (d->state == CS_ALIVE || d->state == CS_EDITING))
            {
                packetbuf q(100);
                sendposition(d, q);
                for(int j = i+1; j < game::numdynents(); j++)
                {
                    gameent *e = (gameent *)game::iterdynents(j);
                    if(e && (e == game::player1 || (e->ai && !e->ai->suspended)) && (e->state == CS_ALIVE || e->state == CS_EDITING))
                        sendposition(e, q);
                }
                sendclientpacket(q.finalize(), 0);
                break;
            }
        }
    }

    void sendmessages()
    {
        packetbuf p(MAXTRANS);
        if(sendcrc)
        {
            p.reliable();
            sendcrc = false;
            putint(p, N_MAPCRC);
            sendstring(game::clientmap, p);
            putint(p, game::clientmap[0] ? getmapcrc() : 0);
        }
        if(sendinfo && !needsmap)
        {
            p.reliable();
            putint(p, N_GAMEINFO);
            putint(p, game::numplayers);
            entities::putitems(p);
            putint(p, -1);
            if(m_stf(game::gamemode)) stf::sendflags(p);
            else if(m_ctf(game::gamemode)) ctf::sendflags(p);
            sendinfo = false;
        }
        if(messages.length())
        {
            p.put(messages.getbuf(), messages.length());
            messages.setsize(0);
            if(messagereliable) p.reliable();
            messagereliable = false;
        }
        if(lastmillis-lastping>250)
        {
            putint(p, N_PING);
            putint(p, lastmillis);
            lastping = lastmillis;
        }

        sendclientpacket(p.finalize(), 1);
    }

    void c2sinfo(bool force) // send update to the server
    {
        static int lastupdate = -1000;
        if(totalmillis-lastupdate < 40 && !force) return;    // don't update faster than 25fps
        lastupdate = totalmillis;
        sendpositions();
        sendmessages();
        flushclient();
    }

    void parsestate(gameent *d, ucharbuf &p, bool resume = false)
    {
        if(!d) { static gameent dummy; d = &dummy; }
        if(d == game::player1 || d->ai) getint(p);
        else d->state = getint(p);
        d->points = getint(p);
        d->frags = getint(p);
        d->health = getint(p);
        d->cptime = getint(p);
        if(resume && (d == game::player1 || d->ai))
        {
            d->weapreset(false);
            getint(p);
            loopi(WEAP_MAX) getint(p);
        }
        else
        {
            d->weapreset(true);
            d->lastweap = d->weapselect = getint(p);
            if(m_arena(game::gamemode, game::mutators)) d->loadweap = d->weapselect;
            loopi(WEAP_MAX) d->ammo[i] = getint(p);
        }
    }

    void updatepos(gameent *d)
    {
        // update the position of other clients in the game in our world
        // don't care if he's in the scenery or other players,
        // just don't overlap with our client

        const float r = game::player1->radius+d->radius;
        const float dx = game::player1->o.x-d->o.x;
        const float dy = game::player1->o.y-d->o.y;
        const float dz = game::player1->o.z-d->o.z;
        const float rz = game::player1->aboveeye+game::player1->height;
        const float fx = (float)fabs(dx), fy = (float)fabs(dy), fz = (float)fabs(dz);
        if(fx<r && fy<r && fz<rz && d->state!=CS_SPECTATOR && d->state!=CS_WAITING && d->state!=CS_DEAD)
        {
            if(fx<fy) d->o.y += dy<0 ? r-fy : -(r-fy);  // push aside
            else      d->o.x += dx<0 ? r-fx : -(r-fx);
        }
        int lagtime = lastmillis-d->lastupdate;
        if(lagtime)
        {
            if(d->lastupdate) d->plag = (d->plag*5+lagtime)/6;
            d->lastupdate = lastmillis;
        }
    }

    void parsepositions(ucharbuf &p)
    {
        int type;
        while(p.remaining()) switch(type = getint(p))
        {
            case N_POS:                        // position of another client
            {
                int lcn = getuint(p), physstate = p.get(), flags = getuint(p);
                vec o, vel, falling;
                float yaw, pitch, roll, aimyaw, aimpitch;
                loopk(3)
                {
                    int n = p.get(); n |= p.get()<<8; if(flags&(1<<k)) { n |= p.get()<<16; if(n&0x800000) n |= -1<<24; }
                    o[k] = n/DMF;
                }
                int dir = p.get(); dir |= p.get()<<8;
                aimyaw = yaw = dir%360;
                aimpitch = pitch = clamp(dir/360, 0, 180)-90;
                roll = clamp(int(p.get()), 0, 180)-90;
                int mag = p.get(); if(flags&(1<<3)) mag |= p.get()<<8;
                dir = p.get(); dir |= p.get()<<8;
                vecfromyawpitch(dir%360, clamp(dir/360, 0, 180)-90, 1, 0, vel);
                vel.mul(mag/DVELF);
                if(flags&(1<<4))
                {
                    mag = p.get(); if(flags&(1<<5)) mag |= p.get()<<8;
                    if(flags&(1<<6))
                    {
                        dir = p.get(); dir |= p.get()<<8;
                        vecfromyawpitch(dir%360, clamp(dir/360, 0, 180)-90, 1, 0, falling);
                    }
                    else falling = vec(0, 0, -1);
                    falling.mul(mag/DVELF);
                }
                else falling = vec(0, 0, 0);
                if(flags&(1<<7))
                {
                    dir = p.get(); dir |= p.get()<<8;
                    aimyaw = dir%360;
                    aimpitch = clamp(dir/360, 0, 180)-90;
                }
                gameent *d = game::getclient(lcn);
                if(!d || d==game::player1 || d->ai) continue;
                float oldyaw = d->yaw, oldpitch = d->pitch, oldaimyaw = d->aimyaw, oldaimpitch = d->aimpitch;
                d->action[AC_SPRINT] = physstate&(1<<3) ? true : false;
                d->conopen = flags&(1<<9) ? true : false;
                d->yaw = yaw;
                d->pitch = pitch;
                d->roll = roll;
                d->aimyaw = aimyaw;
                d->aimpitch = aimpitch;
                d->move = (physstate>>4)&2 ? -1 : (physstate>>4)&1;
                d->strafe = (physstate>>6)&2 ? -1 : (physstate>>6)&1;
                bool crouch = d->action[AC_CROUCH];
                d->action[AC_CROUCH] = flags&(1<<8) ? true : false;
                if(crouch != d->action[AC_CROUCH]) d->actiontime[AC_CROUCH] = lastmillis;
                vec oldpos(d->o);
                //if(game::allowmove(d))
                //{
                    d->o = o;
                    d->o.z += d->height;
                    d->vel = vel;
                    d->falling = falling;
                    d->physstate = physstate&7;
                    physics::updatephysstate(d);
                    updatepos(d);
                //}
                if(d->state==CS_DEAD || d->state==CS_WAITING)
                {
                    d->resetinterp();
                    d->smoothmillis = 0;
                }
                else if(physics::smoothmove && d->smoothmillis>=0 && oldpos.dist(d->o) < physics::smoothdist)
                {
                    d->newpos = d->o;
                    d->newpos.z -= d->height;
                    d->newyaw = d->yaw;
                    d->newpitch = d->pitch;
                    d->newaimyaw = d->aimyaw;
                    d->newaimpitch = d->aimpitch;

                    d->o = oldpos;
                    d->yaw = oldyaw;
                    d->pitch = oldpitch;
                    d->aimyaw = oldaimyaw;
                    d->aimpitch = oldaimpitch;

                    oldpos.z -= d->height;
                    (d->deltapos = oldpos).sub(d->newpos);

                    d->deltayaw = oldyaw - d->newyaw;
                    if(d->deltayaw > 180) d->deltayaw -= 360;
                    else if(d->deltayaw < -180) d->deltayaw += 360;
                    d->deltapitch = oldpitch - d->newpitch;

                    d->deltaaimyaw = oldaimyaw - d->newaimyaw;
                    if(d->deltaaimyaw > 180) d->deltaaimyaw -= 360;
                    else if(d->deltaaimyaw < -180) d->deltaaimyaw += 360;
                    d->deltaaimpitch = oldaimpitch - d->newaimpitch;

                    d->smoothmillis = lastmillis;
                }
                else d->smoothmillis = 0;
                break;
            }

            default:
                neterr("type");
                return;
        }
    }

    void parsemessages(int cn, gameent *d, ucharbuf &p)
    {
        static char text[MAXTRANS];
        int type = -1, prevtype = -1;
        bool mapchanged = false;

        while(p.remaining())
        {
            prevtype = type;
            type = getint(p);
            if(verbose > 5) conoutf("[client] msg: %d, prev: %d", type, prevtype);
            switch(type)
            {
                case N_SERVERINIT:                 // welcome messsage from the server
                {
                    int mycn = getint(p), gver = getint(p);
                    if(gver!=GAMEVERSION)
                    {
                        conoutft(CON_MESG, "\fryou are using a different game version (you: %d, server: %d)", GAMEVERSION, gver);
                        disconnect();
                        return;
                    }
                    sessionid = getint(p);
                    game::player1->clientnum = mycn;
                    if(getint(p)) conoutft(CON_MESG, "\frthe server is password protected");
                    else if(verbose) conoutf("\fathe server welcomes us, yay");
                    sendintro();
                    break;
                }
                case N_WELCOME: isready = true; break;

                case N_CLIENT:
                {
                    int lcn = getint(p), len = getuint(p);
                    ucharbuf q = p.subbuf(len);
                    gameent *t = game::getclient(lcn);
                    parsemessages(lcn, t, q);
                    break;
                }

                case N_SPHY: // simple phys events
                {
                    int lcn = getint(p), st = getint(p);
                    gameent *t = game::getclient(lcn);
                    if(t && (st == SPHY_EXTINGUISH || (t != game::player1 && !t->ai))) switch(st)
                    {
                        case SPHY_JUMP:
                        {
                            playsound(S_JUMP, t->o, t); regularshape(PART_SMOKE, int(t->radius), 0x111111, 21, 20, 150, t->feetpos(), 1, 1, -10, 0, 10.f);
                            t->actiontime[AC_JUMP] = lastmillis;
                            break;
                        }
                        case SPHY_DASH: case SPHY_BOOST: case SPHY_KICK:
                        {
                            playsound(S_IMPULSE, t->o, t); game::impulseeffect(t, true);
                            break;
                        }
                        case SPHY_POWER:
                        {
                            t->setweapstate(t->weapselect, WEAP_S_POWER, getint(p), lastmillis);
                            break;
                        }
                        case SPHY_EXTINGUISH:
                        {
                            t->resetfire();
                            playsound(S_EXTINGUISH, t->o, t, 0, t != game::focus ? 128 : 224, -1, -1);
                            part_create(PART_SMOKE, 500, t->feetpos(t->height/2), 0xAAAAAA, t->height/2, 0.5f, -10);
                            break;
                        }
                        default: break;
                    }
                    break;
                }

                case N_ANNOUNCE:
                {
                    int snd = getint(p), targ = getint(p);
                    getstring(text, p);
                    if(targ >= 0 && text[0]) game::announce(snd, targ, NULL, "%s", text);
                    else game::announce(snd, -1, NULL, "");
                    break;
                }

                case N_TEXT:
                {
                    int tcn = getint(p);
                    gameent *t = game::getclient(tcn);
                    int flags = getint(p);
                    getstring(text, p);
                    if(!t) break;
                    saytext(t, flags, text);
                    break;
                }

                case N_COMMAND:
                {
                    int lcn = getint(p);
                    gameent *f = game::getclient(lcn);
                    string cmd;
                    getstring(cmd, p);
                    getstring(text, p);
                    parsecommand(f ? f : NULL, cmd, text);
                    break;
                }

                case N_EXECLINK:
                {
                    int tcn = getint(p), index = getint(p);
                    gameent *t = game::getclient(tcn);
                    if(!t || !d || (t->clientnum != d->clientnum && t->ownernum != d->clientnum) || t == game::player1 || t->ai) break;
                    entities::execlink(t, index, false);
                    break;
                }

                case N_MAPCHANGE:
                {
                    int hasmap = getint(p);
                    if(hasmap) getstring(text, p);
                    int getit = getint(p), mode = getint(p), muts = getint(p);
                    changemapserv(hasmap && getit != 1 ? text : NULL, mode, muts, getit == 2);
                    mapchanged = true;
                    if(needsmap) switch(getit)
                    {
                        case 0: case 2:
                        {
                            conoutft(CON_MESG, "\fcserver requested map change to %s, and we need it, so asking for it", hasmap ? text : "<temp>");
                            addmsg(N_GETMAP, "r");
                            break;
                        }
                        case 1:
                        {
                            conoutft(CON_MESG, "\fcserver is requesting the map from another client for us");
                            break;
                        }
                        default: needsmap = false; break;
                    }
                    break;
                }

                case N_GAMEINFO:
                {
                    int n;
                    while((n = getint(p)) != -1) entities::setspawn(n, getint(p));
                    sendinfo = false;
                    break;
                }

                case N_NEWGAME: // server requests next game
                {
                    hud::sb.showscores(false);
                    if(!menuactive()) showgui("maps", 1);
                    if(game::intermission) hud::lastnewgame = totalmillis;
                    break;
                }

                case N_SWITCHNAME:
                    getstring(text, p);
                    if(!d) break;
                    filtertext(text, text, true, true, true, MAXNAMELEN);
                    if(!text[0]) copystring(text, "unnamed");
                    if(strcmp(d->name, text))
                    {
                        string oldname, newname;
                        copystring(oldname, game::colorname(d, NULL, "", false));
                        copystring(newname, game::colorname(d, text));
                        if(game::showplayerinfo)
                            conoutft(game::showplayerinfo > 1 ? int(CON_EVENT) : int(CON_MESG), "\fm%s is now known as %s", oldname, newname);
                    }
                    copystring(d->name, text, MAXNAMELEN+1);
                    break;

                case N_CLIENTINIT: // another client either connected or changed name/team
                {
                    int cn = getint(p);
                    gameent *d = game::newclient(cn);
                    if(!d)
                    {
                        getstring(text, p);
                        getint(p);
                        break;
                    }
                    getstring(text, p);
                    filtertext(text, text, true, true, true, MAXNAMELEN);
                    if(!text[0]) copystring(text, "unnamed");
                    if(d->name[0])        // already connected
                    {
                        if(strcmp(d->name, text))
                        {
                            string oldname, newname;
                            copystring(oldname, game::colorname(d, NULL, "", false));
                            copystring(newname, game::colorname(d, text));
                            if(game::showplayerinfo)
                                conoutft(game::showplayerinfo > 1 ? int(CON_EVENT) : int(CON_MESG), "\fm%s is now known as %s", oldname, newname);
                        }
                    }
                    else                    // new client
                    {
                        if(game::showplayerinfo)
                            conoutft(game::showplayerinfo > 1 ? int(CON_EVENT) : int(CON_MESG), "\fg%s has joined the game", game::colorname(d, text, "", false));
                        if(needclipboard >= 0) needclipboard++;
                        game::cameras.shrink(0);
                    }
                    copystring(d->name, text, MAXNAMELEN+1);
                    int team = clamp(getint(p), int(TEAM_NEUTRAL), int(TEAM_ENEMY));
                    if(d == game::focus && d->team != team) hud::lastteam = 0;
                    d->team = team;
                    break;
                }

                case N_DISCONNECT:
                {
                    int lcn = getint(p), reason = getint(p);
                    game::clientdisconnected(lcn, reason);
                    break;
                }

                case N_SPAWN:
                {
                    int lcn = getint(p);
                    gameent *f = game::newclient(lcn);
                    if(f && f != game::player1 && !f->ai)
                    {
                        f->respawn(lastmillis, m_health(game::gamemode, game::mutators));
                        parsestate(f, p);
                        if(f->aitype < AI_START)
                        {
                            playsound(S_RESPAWN, f->o, f);
                            if(game::dynlighteffects)
                                adddynlight(f->headpos(), f->height*2, vec(teamtype[f->team].colour>>16, (teamtype[f->team].colour>>8)&0xFF, teamtype[f->team].colour&0xFF).mul(2.f/0xFF), 250, 250);
                        }
                    }
                    else parsestate(NULL, p);
                    break;
                }

                case N_LOADWEAP:
                {
                    hud::sb.showscores(false);
                    game::player1->loadweap = -1;
                    if(!menuactive()) showgui("loadout", -1);
                    break;
                }

                case N_SPAWNSTATE:
                {
                    int lcn = getint(p), ent = getint(p);
                    gameent *f = game::newclient(lcn);
                    if(!f)
                    {
                        parsestate(NULL, p);
                        break;
                    }
                    if(f == game::player1 && editmode) toggleedit();
                    f->respawn(lastmillis, m_health(game::gamemode, game::mutators));
                    parsestate(f, p);
                    f->state = CS_ALIVE;
                    if(f == game::player1 || f->ai)
                    {
                        addmsg(N_SPAWN, "ri", f->clientnum);
                        entities::spawnplayer(f, ent, true);
                        if(f->aitype < AI_START)
                        {
                            playsound(S_RESPAWN, f->o, f);
                            if(game::dynlighteffects)
                                adddynlight(f->headpos(), f->height*2, vec(teamtype[f->team].colour>>16, (teamtype[f->team].colour>>8)&0xFF, teamtype[f->team].colour&0xFF).mul(2.f/0xFF), 250, 250);
                        }
                    }
                    ai::spawned(f, ent);
                    if(f == game::focus) game::resetcamera();
                    break;
                }

                case N_SHOTFX:
                {
                    int scn = getint(p), weap = getint(p), flags = getint(p);
                    float scale = getint(p)/DMF;
                    vec from;
                    loopk(3) from[k] = getint(p)/DMF;
                    int ls = getint(p);
                    vector<vec> locs;
                    loopj(ls)
                    {
                        vec &to = locs.add();
                        loopk(3) to[k] = getint(p)/DMF;
                    }
                    gameent *s = game::getclient(scn);
                    if(!s || !isweap(weap) || s == game::player1 || s->ai) break;
                    int sub = WEAP2(weap, sub, flags&HIT_ALT);
                    if(sub > 1 && WEAP2(weap, power, flags&HIT_ALT)) sub = int(ceilf(sub*scale));
                    projs::shootv(weap, flags, sub, scale, from, locs, s, false);
                    break;
                }

                case N_DAMAGE:
                {
                    int tcn = getint(p),
                        acn = getint(p),
                        weap = getint(p),
                        flags = getint(p),
                        damage = getint(p),
                        health = getint(p);
                    vec dir;
                    loopk(3) dir[k] = getint(p)/DNF;
                    dir.normalize();
                    gameent *target = game::getclient(tcn), *actor = game::getclient(acn);
                    if(!target || !actor) break;
                    game::damaged(weap, flags, damage, health, target, actor, lastmillis, dir);
                    break;
                }

                case N_RELOAD:
                {
                    int trg = getint(p), weap = getint(p), amt = getint(p), ammo = getint(p);
                    gameent *target = game::getclient(trg);
                    if(!target || !isweap(weap)) break;
                    weapons::weapreload(target, weap, amt, ammo, false);
                    break;
                }

                case N_REGEN:
                {
                    int trg = getint(p), heal = getint(p), amt = getint(p);
                    gameent *f = game::getclient(trg);
                    if(!f) break;
                    if(!amt)
                    {
                        f->impulse[IM_METER] = 0;
                        f->resetfire();
                    }
                    else if(amt > 0 && (!f->lastregen || lastmillis-f->lastregen >= 500)) playsound(S_REGEN, f->o, f);
                    f->health = heal; f->lastregen = lastmillis;
                    break;
                }

                case N_DIED:
                {
                    int vcn = getint(p), acn = getint(p), frags = getint(p), style = getint(p), weap = getint(p), flags = getint(p), damage = getint(p);
                    gameent *victim = game::getclient(vcn), *actor = game::getclient(acn);
                    if(!actor || !victim) break;
                    actor->frags = frags;
                    game::killed(weap, flags, damage, victim, actor, style);
                    victim->lastdeath = lastmillis;
                    victim->weapreset(true);
                    break;
                }

                case N_POINTS:
                {
                    int acn = getint(p), add = getint(p), points = getint(p);
                    gameent *actor = game::getclient(acn);
                    if(!actor) break;
                    actor->lastpoints = add;
                    actor->points = points;
                    break;
                }

                case N_DROP:
                {
                    int trg = getint(p), weap = getint(p), ds = getint(p);
                    gameent *target = game::getclient(trg);
                    bool local = target && (target == game::player1 || target->ai);
                    if(ds) loopj(ds)
                    {
                        int gs = getint(p), drop = getint(p);
                        if(target) projs::drop(target, gs, drop, local);
                    }
                    if(isweap(weap) && target)
                    {
                        target->weapswitch(weap, lastmillis);
                        if(issound(target->wschan)) removesound(target->wschan);
                        playsound(S_SWITCH, target->o, target, 0, -1, -1, -1, &target->wschan);
                    }
                    break;
                }

                case N_WEAPSELECT:
                {
                    int trg = getint(p), weap = getint(p);
                    gameent *target = game::getclient(trg);
                    if(!target || !isweap(weap)) break;
                    weapons::weapselect(target, weap, false);
                    break;
                }

                case N_RESUME:
                {
                    for(;;)
                    {
                        int lcn = getint(p);
                        if(p.overread() || lcn < 0) break;
                        gameent *f = game::newclient(lcn);
                        if(f && f!=game::player1 && !f->ai) f->respawn(0, m_health(game::gamemode, game::mutators));
                        parsestate(f, p, true);
                    }
                    break;
                }

                case N_ITEMSPAWN:
                {
                    int ent = getint(p), value = getint(p);
                    if(!entities::ents.inrange(ent)) break;
                    entities::setspawn(ent, value);
                    if(entities::ents[ent]->spawned)
                    {
                        playsound(S_ITEMSPAWN, entities::ents[ent]->o);
                        int sweap = m_weapon(game::gamemode, game::mutators), attr = entities::ents[ent]->type == WEAPON ? w_attr(game::gamemode, entities::ents[ent]->attrs[0], sweap) : entities::ents[ent]->attrs[0],
                            colour = entities::ents[ent]->type == WEAPON ? weaptype[attr].colour : 0xFFFFFF;
                        if(entities::showentdescs)
                        {
                            vec pos = vec(entities::ents[ent]->o).add(vec(0, 0, 4));
                            const char *texname = entities::showentdescs >= 2 ? hud::itemtex(entities::ents[ent]->type, attr) : NULL;
                            if(texname && *texname) part_icon(pos, textureload(texname, 3), 2, 1, -10, 0, game::aboveheadfade, colour);
                            else
                            {
                                const char *item = entities::entinfo(entities::ents[ent]->type, entities::ents[ent]->attrs, false);
                                if(item && *item)
                                {
                                    defformatstring(ds)("<emphasis>%s", item);
                                    part_textcopy(pos, ds, PART_TEXT, game::aboveheadfade, 0xFFFFFF, 2, 1, -10);
                                }
                            }
                        }
                        if(game::dynlighteffects)
                            adddynlight(entities::ents[ent]->o, enttype[entities::ents[ent]->type].radius*2, vec(colour>>16, (colour>>8)&0xFF, colour&0xFF).mul(2.f/0xFF), 250, 250);
                    }
                    break;
                }

                case N_TRIGGER:
                {
                    int ent = getint(p), st = getint(p);
                    entities::setspawn(ent, st);
                    break;
                }

                case N_ITEMACC:
                { // uses a specific drop so the client knows what to replace
                    int lcn = getint(p), ent = getint(p), spawn = getint(p), weap = getint(p), drop = getint(p);
                    gameent *target = game::getclient(lcn);
                    if(!target) break;
                    if(entities::ents.inrange(ent) && enttype[entities::ents[ent]->type].usetype == EU_ITEM)
                        entities::useeffects(target, ent, spawn, weap, drop);
                    break;
                }

                case N_EDITVAR:
                {
                    int t = getint(p);
                    bool commit = true;
                    getstring(text, p);
                    ident *id = idents->access(text);
                    if(!d || !id || !(id->flags&IDF_WORLD) || id->type != t) commit = false;
                    switch(t)
                    {
                        case ID_VAR:
                        {
                            int val = getint(p);
                            if(commit)
                            {
                                if(val > id->maxval) val = id->maxval;
                                else if(val < id->minval) val = id->minval;
                                setvar(text, val, true);
                                defformatstring(str)(id->flags&IDF_HEX ? (id->maxval==0xFFFFFF ? "0x%.6X" : "0x%X") : "%d", *id->storage.i);
                                conoutft(CON_MESG, "\fg%s set worldvar %s to %s", game::colorname(d), id->name, str);
                            }
                            break;
                        }
                        case ID_FVAR:
                        {
                            float val = getfloat(p);
                            if(commit)
                            {
                                if(val > id->maxvalf) val = id->maxvalf;
                                else if(val < id->minvalf) val = id->minvalf;
                                setfvar(text, val, true);
                                conoutft(CON_MESG, "\fg%s set worldvar %s to %s", game::colorname(d), id->name, floatstr(*id->storage.f));
                            }
                            break;
                        }
                        case ID_SVAR:
                        {
                            string val;
                            getstring(val, p);
                            if(commit)
                            {
                                setsvar(text, val, true);
                                conoutft(CON_MESG, "\fg%s set worldvar %s to %s", game::colorname(d), id->name, *id->storage.s);
                            }
                            break;
                        }
                        case ID_ALIAS:
                        {
                            string val;
                            getstring(val, p);
                            if(commit || !id) // set aliases anyway
                            {
                                worldalias(text, val);
                                conoutft(CON_MESG, "\fg%s set worldalias %s to %s", game::colorname(d), text, val);
                            }
                            break;
                        }
                        default: break;
                    }
                    break;
                }

                case N_CLIPBOARD:
                {
                    int cn = getint(p), unpacklen = getint(p), packlen = getint(p);
                    gameent *d = game::getclient(cn);
                    ucharbuf q = p.subbuf(max(packlen, 0));
                    if(d) unpackeditinfo(d->edit, q.buf, q.maxlen, unpacklen);
                    break;
                }

                case N_EDITF:            // coop editing messages
                case N_EDITT:
                case N_EDITM:
                case N_FLIP:
                case N_COPY:
                case N_PASTE:
                case N_ROTATE:
                case N_REPLACE:
                case N_DELCUBE:
                {
                    if(!d) return;
                    selinfo s;
                    s.o.x = getint(p); s.o.y = getint(p); s.o.z = getint(p);
                    s.s.x = getint(p); s.s.y = getint(p); s.s.z = getint(p);
                    s.grid = getint(p); s.orient = getint(p);
                    s.cx = getint(p); s.cxs = getint(p); s.cy = getint(p), s.cys = getint(p);
                    s.corner = getint(p);
                    int dir, mode, tex, newtex, mat, allfaces;
                    ivec moveo;
                    switch(type)
                    {
                        case N_EDITF: dir = getint(p); mode = getint(p); mpeditface(dir, mode, s, false); break;
                        case N_EDITT: tex = getint(p); allfaces = getint(p); mpedittex(tex, allfaces, s, false); break;
                        case N_EDITM: mat = getint(p); mpeditmat(mat, s, false); break;
                        case N_FLIP: mpflip(s, false); break;
                        case N_COPY: if(d) mpcopy(d->edit, s, false); break;
                        case N_PASTE: if(d) mppaste(d->edit, s, false); break;
                        case N_ROTATE: dir = getint(p); mprotate(dir, s, false); break;
                        case N_REPLACE: tex = getint(p); newtex = getint(p); mpreplacetex(tex, newtex, s, false); break;
                        case N_DELCUBE: mpdelcube(s, false); break;
                    }
                    break;
                }
                case N_REMIP:
                {
                    if(!d) return;
                    conoutft(CON_MESG, "%s remipped", game::colorname(d));
                    mpremip(false);
                    break;
                }
                case N_EDITENT:            // coop edit of ent
                {
                    if(!d) return;
                    int i = getint(p);
                    float x = getint(p)/DMF, y = getint(p)/DMF, z = getint(p)/DMF;
                    int type = getint(p), numattrs = getint(p);
                    static vector<int> attrs; attrs.setsize(0);
                    loopk(numattrs) attrs.add(getint(p));
                    mpeditent(i, vec(x, y, z), type, attrs, false);
                    entities::setspawn(i, 0);
                    entities::clearentcache();
                    break;
                }

                case N_EDITLINK:
                {
                    if(!d) return;
                    int b = getint(p), index = getint(p), node = getint(p);
                    entities::linkents(index, node, b!=0, false, false);
                }

                case N_PONG:
                    addmsg(N_CLIENTPING, "i", game::player1->ping = (game::player1->ping*5+lastmillis-getint(p))/6);
                    break;

                case N_CLIENTPING:
                    if(!d) return;
                    d->ping = getint(p);
                    break;

                case N_TICK:
                    game::timeupdate(getint(p));
                    break;

                case N_SERVMSG:
                {
                    int lev = getint(p);
                    getstring(text, p);
                    conoutft(lev >= 0 && lev < CON_MAX ? lev : CON_INFO, "%s", text);
                    break;
                }

                case N_SENDDEMOLIST:
                {
                    int demos = getint(p);
                    if(!demos) conoutft(CON_MESG, "\frno demos available");
                    else loopi(demos)
                    {
                        getstring(text, p);
                        conoutft(CON_MESG, "\fa%d. %s", i+1, text);
                    }
                    break;
                }

                case N_DEMOPLAYBACK:
                {
                    int on = getint(p);
                    if(on) game::player1->state = CS_SPECTATOR;
                    else
                    {
                        loopv(game::players) if(game::players[i]) game::clientdisconnected(i);
                    }
                    demoplayback = on!=0;
                    game::player1->clientnum = getint(p);
                    break;
                }

                case N_CURRENTMASTER:
                {
                    int mn = getint(p), priv = getint(p);
                    if(mn >= 0)
                    {
                        gameent *m = game::getclient(mn);
                        if(m) m->privilege = priv;
                    }
                    break;
                }

                case N_EDITMODE:
                {
                    int val = getint(p);
                    if(!d) break;
                    if(val) d->state = CS_EDITING;
                    else
                    {
                        d->state = CS_ALIVE;
                        d->editspawn(lastmillis, m_weapon(game::gamemode, game::mutators), m_health(game::gamemode, game::mutators), m_insta(game::gamemode, game::mutators), m_arena(game::gamemode, game::mutators), spawngrenades >= (m_insta(game::gamemode, game::mutators) ? 2 : 1));
                    }
                    d->resetinterp();
                    projs::remove(d);
                    break;
                }

                case N_SPECTATOR:
                {
                    int sn = getint(p), val = getint(p);
                    gameent *s = game::newclient(sn);
                    if(!s) break;
                    if(s == game::player1) game::focus = game::player1;
                    if(val)
                    {
                        if(s == game::player1 && editmode) toggleedit();
                        s->state = CS_SPECTATOR;
                        s->checkpoint = -1;
                        s->cpmillis = 0;
                    }
                    else if(s->state == CS_SPECTATOR)
                    {
                        s->state = CS_WAITING;
                        s->checkpoint = -1;
                        s->cpmillis = 0;
                        if(s != game::player1 && !s->ai) s->resetinterp();
                    }
                    break;
                }

                case N_WAITING:
                {
                    int sn = getint(p);
                    gameent *s = game::newclient(sn);
                    if(!s) break;
                    if(s == game::player1)
                    {
                        if(editmode) toggleedit();
                        hud::sb.showscores(false);
                        game::focus = game::player1;
                    }
                    else if(!s->ai) s->resetinterp();
                    if(s->state == CS_ALIVE) s->lastdeath = lastmillis; // so spawndelay shows properly
                    s->state = CS_WAITING;
                    s->weapreset(true);
                    break;
                }

                case N_SETTEAM:
                {
                    int wn = getint(p), tn = getint(p);
                    gameent *w = game::getclient(wn);
                    if(!w) return;
                    if(w == game::focus && w->team != tn) hud::lastteam = 0;
                    w->team = tn;
                    break;
                }

                case N_FLAGINFO:
                {
                    int flag = getint(p), converted = getint(p),
                            owner = getint(p), enemy = getint(p);
                    if(m_stf(game::gamemode)) stf::updateflag(flag, owner, enemy, converted);
                    break;
                }

                case N_FLAGS:
                {
                    int numflags = getint(p);
                    loopi(numflags)
                    {
                        int kin = getint(p), converted = getint(p), owner = getint(p), enemy = getint(p);
                        stf::st.initflag(i, kin, owner, enemy, converted);
                    }
                    break;
                }

                case N_MAPVOTE:
                {
                    int vn = getint(p);
                    gameent *v = game::getclient(vn);
                    getstring(text, p);
                    filtertext(text, text);
                    int reqmode = getint(p), reqmuts = getint(p);
                    if(!v) break;
                    vote(v, text, reqmode, reqmuts);
                    break;
                }

                case N_CHECKPOINT:
                {
                    int tn = getint(p), laptime = getint(p), besttime = getint(p);
                    gameent *t = game::getclient(tn);
                    if(!t) break;
                    t->cplast = laptime;
                    t->cptime = besttime;
                    t->cpmillis = t->impulse[IM_METER] = 0;
                    if(showlaptimes > (t != game::focus ? (t->aitype >= 0 ? 2 : 1) : 0))
                    {
                        defformatstring(best)("%s", hud::timetostr(besttime));
                        conoutft(t != game::focus ? CON_INFO : CON_SELF, "%s lap time: \fs\fg%s\fS (best: \fs\fy%s\fS)", game::colorname(t), hud::timetostr(laptime), best);
                    }
                }

                case N_SCORE:
                {
                    int team = getint(p), total = getint(p);
                    if(m_stf(game::gamemode)) stf::setscore(team, total);
                    else if(m_ctf(game::gamemode)) ctf::setscore(team, total);
                    break;
                }

                case N_INITFLAGS:
                {
                    ctf::parseflags(p, m_ctf(game::gamemode));
                    break;
                }

                case N_DROPFLAG:
                {
                    int ocn = getint(p), flag = getint(p);
                    vec droploc;
                    loopk(3) droploc[k] = getint(p)/DMF;
                    gameent *o = game::newclient(ocn);
                    if(o && m_ctf(game::gamemode)) ctf::dropflag(o, flag, droploc);
                    break;
                }

                case N_SCOREFLAG:
                {
                    int ocn = getint(p), relayflag = getint(p), goalflag = getint(p), score = getint(p);
                    gameent *o = game::newclient(ocn);
                    if(o && m_ctf(game::gamemode)) ctf::scoreflag(o, relayflag, goalflag, score);
                    break;
                }

                case N_RETURNFLAG:
                {
                    int ocn = getint(p), flag = getint(p);
                    gameent *o = game::newclient(ocn);
                    if(o && m_ctf(game::gamemode)) ctf::returnflag(o, flag);
                    break;
                }

                case N_TAKEFLAG:
                {
                    int ocn = getint(p), flag = getint(p);
                    gameent *o = game::newclient(ocn);
                    if(o && m_ctf(game::gamemode)) ctf::takeflag(o, flag);
                    break;
                }

                case N_RESETFLAG:
                {
                    int flag = getint(p);
                    if(m_ctf(game::gamemode)) ctf::resetflag(flag);
                    break;
                }

                case N_GETMAP:
                {
                    conoutft(CON_MESG, "\fcserver has requested we send the map..");
                    if(!needsmap && !gettingmap) sendmap();
                    else
                    {
                        if(!gettingmap) conoutft(CON_MESG, "\fowe don't have the map though, so asking for it instead");
                        else conoutft(CON_MESG, "\fobut we're in the process of getting it");
                        addmsg(N_GETMAP, "r");
                    }
                    break;
                }

                case N_SENDMAP:
                {
                    conoutft(CON_MESG, "\fcmap data has been uploaded");
                    if(needsmap && !gettingmap)
                    {
                        conoutft(CON_MESG, "\fowe want the map too, so asking for it");
                        addmsg(N_GETMAP, "r");
                    }
                    break;
                }

                case N_NEWMAP:
                {
                    int size = getint(p);
                    if(size>=0) emptymap(size, true);
                    else enlargemap(true);
                    mapvotes.shrink(0);
                    needsmap = false;
                    if(d && d!=game::player1)
                    {
                        int newsize = 0;
                        while(1<<newsize < getworldsize()) newsize++;
                        conoutft(CON_MESG, size>=0 ? "%s started a new map of size %d" : "%s enlarged the map to size %d", game::colorname(d), newsize);
                    }
                    break;
                }

                case N_INITAI:
                {
                    int bn = getint(p), on = getint(p), at = getint(p), et = getint(p), sk = clamp(getint(p), 1, 101);
                    getstring(text, p);
                    int tm = getint(p);
                    gameent *b = game::newclient(bn);
                    if(!b) break;
                    ai::init(b, at, et, on, sk, bn, text, tm);
                    break;
                }

                case N_AUTHCHAL:
                {
                    uint id = (uint)getint(p);
                    getstring(text, p);
                    if(authname[0])
                    {
                        conoutft(CON_MESG, "server is challenging authentication details..");
                        vector<char> buf;
                        answerchallenge(authkey, text, buf);
                        addmsg(N_AUTHANS, "ris", id, buf.getbuf());
                    }
                    break;
                }

                default:
                {
                    neterr("type");
                    return;
                }
            }
        }
    }

    int serverstat(serverinfo *a)
    {
        if(a->attr.length() > 4 && a->numplayers >= a->attr[4])
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
            case MM_PASSWORD:
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

    void resetserversort()
    {
        defformatstring(u)("serversort [%d %d %d]", SINFO_STATUS, SINFO_NUMPLRS, SINFO_PING);
        execute(u);
    }
    ICOMMAND(0, serversortreset, "", (), resetserversort());

    int servercompare(serverinfo *a, serverinfo *b)
    {
        if(!serversort || !*serversort) resetserversort();
        int ac = a->address.host == ENET_HOST_ANY || a->ping >= serverinfo::WAITING || a->attr.empty() || a->attr[0] != GAMEVERSION ? -1 : 0,
            bc = b->address.host == ENET_HOST_ANY || b->ping >= serverinfo::WAITING || b->attr.empty() || b->attr[0] != GAMEVERSION ? -1 : 0;
        if(!ac)
        {
            if(!strcmp(a->sdesc, servermaster)) ac = 3;
            else if(!strcmp(a->name, "localhost")) ac = 2;
            else ac = 1;
        }

        if(!bc)
        {
            if(!strcmp(b->sdesc, servermaster)) bc = 3;
            else if(!strcmp(b->name, "localhost")) bc = 2;
            else bc = 1;
        }
        if(ac > bc) return -1;
        if(ac < bc) return 1;

        #define retcp(c) if(c) { return c; }
        #define retsw(c,d,e) \
            if(c != d) \
            { \
                if(e) { return c > d ? 1 : -1; } \
                else { return c < d ? 1 : -1; } \
            }

        int len = execute("listlen $serversort");

        loopi(len)
        {
            defformatstring(s)("at $serversort %d", i);

            int style = execute(s);
            serverinfo *aa = a, *ab = b;

            if(style < 0)
            {
                style = 0-style;
                aa = b;
                ab = a;
            }

            switch(style)
            {
                case SINFO_STATUS:
                {
                    retsw(serverstat(aa), serverstat(ab), true);
                    break;
                }
                case SINFO_DESC:
                {
                    retcp(strcmp(aa->sdesc, ab->sdesc));
                    break;
                }
                case SINFO_MODE:
                {
                    if(aa->attr.length() > 1) ac = aa->attr[1];
                    else ac = 0;

                    if(ab->attr.length() > 1) bc = ab->attr[1];
                    else bc = 0;

                    retsw(ac, bc, true);
                }
                case SINFO_MUTS:
                {
                    if(aa->attr.length() > 2) ac = aa->attr[2];
                    else ac = 0;

                    if(ab->attr.length() > 2) bc = ab->attr[2];
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
                    if(aa->attr.length() > 3) ac = aa->attr[3];
                    else ac = 0;

                    if(ab->attr.length() > 3) bc = ab->attr[3];
                    else bc = 0;

                    retsw(ac, bc, false);
                    break;
                }
                case SINFO_NUMPLRS:
                {
                    retsw(aa->numplayers, ab->numplayers, false);
                    break;
                }
                case SINFO_MAXPLRS:
                {
                    if(aa->attr.length() > 4) ac = aa->attr[4];
                    else ac = 0;

                    if(ab->attr.length() > 4) bc = ab->attr[4];
                    else bc = 0;

                    retsw(ac, bc, false);
                    break;
                }
                case SINFO_PING:
                {
                    retsw(aa->ping, ab->ping, true);
                    break;
                }
                default: break;
            }
        }
        return strcmp(a->name, b->name);
    }

    void parsepacketclient(int chan, packetbuf &p)  // processes any updates from the server
    {
        switch(chan)
        {
            case 0:
                parsepositions(p);
                break;

            case 1:
                parsemessages(-1, NULL, p);
                break;

            case 2:
                receivefile(p.buf, p.maxlen);
                break;
        }
    }

    void getservers(int server, int prop)
    {
        mkstring(text); server--;
        if(servers.inrange(server))
        {
            serverinfo *si = servers[server];
            switch(prop)
            {
                default: copystring(text, "0"); break;
                case 0:
                {
                    defformatstring(sn)("%s", escapetext(si->name));
                    defformatstring(sd)("%s", escapetext(si->sdesc));
                    defformatstring(sm)("%s", escapetext(si->map));
                    formatstring(text)("%d \"%s\" %d \"%s\" \"%s\" %d %d", serverstat(si), sn, si->port, sd, sm, si->numplayers, si->ping);
                    loopv(si->attr) { defformatstring(s)(" %d", si->attr[i]); concatstring(text, s); }
                    break;
                }
                case 1:
                {
                    loopv(si->players)
                    {
                        defformatstring(s)("%s\"%s\"", text[0] ? " " : "", escapetext(si->players[i]));
                        concatstring(text, s);
                    }
                    break;
                }
            }
        }
        else formatstring(text)("%d", servers.length());
        result(text);
    }
    ICOMMAND(0, getserver, "ii", (int *server, int *prop), getservers(*server, *prop));
}
