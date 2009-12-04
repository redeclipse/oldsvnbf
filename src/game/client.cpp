#include "game.h"
namespace client
{
	bool sendinfo = false, isready = false, remote = false,
		demoplayback = false, needsmap = false, gettingmap = false, sendcrc = false;
	int lastping = 0, sessionid = 0;
    string connectpass = "";
	vector<mapvote> mapvotes;

	void vote(gameent *d, const char *text, int mode, int muts)
	{
		mapvote *m = NULL;
		loopv(mapvotes) if(mapvotes[i].player == d) { m = &mapvotes[i]; break; }
		if(!m) m = &mapvotes.add();
		m->player = d;
		copystring(m->map, text);
		m->mode = mode;
		m->muts = muts;
		SEARCHBINDCACHE(votekey)("showgui vote", 0);
		SEARCHBINDCACHE(gamekey)("showgui game", 0);
		conoutft(CON_MESG, "\fc%s suggests: \fs\fw%s on %s, press \fs\fc%s\fS to vote or \fs\fc%s\fS to select your own", game::colorname(d), server::gamename(mode, muts), text, votekey, gamekey);
	}
    void getvotes(int vote)
    {
    	if(!vote) intret(mapvotes.length());
    	else
    	{
			mkstring(text);
			if(mapvotes.inrange(--vote)) formatstring(text)("%d %d %d %s", mapvotes[vote].player->clientnum, mapvotes[vote].mode, mapvotes[vote].muts, mapvotes[vote].map);
			result(text);
    	}
    }
    ICOMMAND(getvote, "i", (int *num), getvotes(*num));

    int lastauth = 0;
    string authname = "", authkey = "";

    void setauthkey(const char *name, const char *key)
    {
        copystring(authname, name);
        copystring(authkey, key);
    }
    ICOMMAND(authkey, "ss", (char *name, char *key), setauthkey(name, key));

	// collect c2s messages conveniently
	vector<uchar> messages;
    bool messagereliable = false;

	VARP(colourchat, 0, 1, 1);
	VARP(showlaptimes, 0, 2, 3); // 0 = off, 1 = only player, 2 = +humans, 3 = +bots
	SVARP(serversort, "");

	ICOMMAND(mastermode, "i", (int *val), addmsg(SV_MASTERMODE, "ri", *val));
	ICOMMAND(getname, "", (), result(game::player1->name));
	ICOMMAND(getteam, "", (), result(teamtype[game::player1->team].name));
    ICOMMAND(getteamicon, "", (), result(hud::teamtex(game::player1->team)));

    const char *getname() { return game::player1->name; }

	void switchname(const char *name)
	{
		if(name[0])
		{
			string text; filtertext(text, name);
			copystring(game::player1->name, text, MAXNAMELEN);
			addmsg(SV_SWITCHNAME, "rs", game::player1->name);
		}
		else conoutft(CON_MESG, "\fgyour name is: %s", *game::player1->name ? game::colorname(game::player1) : "<not set>");
	}
	ICOMMAND(name, "s", (char *s), switchname(s));

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
					addmsg(SV_SWITCHTEAM, "ri", game::player1->team);
				}
			}
			else conoutft(CON_MESG, "\frcan only change teams when actually playing in team games");
		}
		else conoutft(CON_MESG, "\fs\fgyour team is:\fS \fs%s%s\fS", teamtype[game::player1->team].chat, teamtype[game::player1->team].name);
	}
	ICOMMAND(team, "s", (char *s), switchteam(s));

	int numchannels() { return 3; }

	void writeclientinfo(stream *f)
	{
		f->printf("name \"%s\"\n\n", game::player1->name);
	}

    void connectattempt(const char *name, int port, int qport, const char *password, const ENetAddress &address)
    {
        copystring(connectpass, password);
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
		messages.setsize(0);
		mapvotes.setsize(0);
        messagereliable = false;
		projs::remove(game::player1);
        removetrackedparticles(game::player1);
		removetrackedsounds(game::player1);
		game::player1->clientnum = -1;
		game::player1->privilege = PRIV_NONE;
		game::gamemode = game::mutators = -1;
		loopv(game::players) if(game::players[i]) game::clientdisconnected(i);
		emptymap(0, true, NULL, true);
		smartmusic(true, false);
		enumerate(*idents, ident, id, {
			if(id.flags&IDF_CLIENT) // reset vars
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
		if(clean)
		{
			game::clientmap[0] = '\0';
		}
	}

	bool allowedittoggle(bool edit)
	{
		bool allow = edit || m_edit(game::gamemode); // && game::player1->state == CS_ALIVE);
		if(!allow) conoutft(CON_MESG, "\fryou must be in edit mode to start editing");
		return allow;
	}

	void edittoggled(bool edit)
	{
		game::player1->editspawn(lastmillis, m_weapon(game::gamemode, game::mutators), m_health(game::gamemode, game::mutators), m_arena(game::gamemode, game::mutators), spawngrenades >= (m_insta(game::gamemode, game::mutators) ? 2 : 1));
		game::player1->state = edit ? CS_EDITING : CS_ALIVE;
		game::player1->resetinterp();
		game::resetstate();
		physics::entinmap(game::player1, true); // find spawn closest to current floating pos
		projs::remove(game::player1);
		if(m_edit(game::gamemode)) addmsg(SV_EDITMODE, "ri", edit ? 1 : 0);
		entities::edittoggled(edit);
	}

    const char *getclientname(int cn)
    {
        gameent *d = game::getclient(cn);
        return d ? d->name : "";
    }
    ICOMMAND(getclientname, "i", (int *cn), result(getclientname(*cn)));

    int getclientteam(int cn)
    {
        gameent *d = game::getclient(cn);
        return d ? d->team : -1;
    }
    ICOMMAND(getclientteam, "i", (int *cn), intret(getclientteam(*cn)));

    bool isspectator(int cn)
    {
        gameent *d = game::getclient(cn);
        return d->state==CS_SPECTATOR;
    }
    ICOMMAND(isspectator, "i", (int *cn), intret(isspectator(*cn) ? 1 : 0));

    bool isai(int cn, int type)
    {
        gameent *d = game::getclient(cn);
        int aitype = type > 0 && type < AI_MAX ? type : AI_BOT;
        return d->aitype==aitype;
    }
    ICOMMAND(isai, "ii", (int *cn, int *type), intret(isai(*cn, *type) ? 1 : 0));

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
    ICOMMAND(getclientnum, "s", (char *name), intret(name[0] ? parseplayer(name) : game::player1->clientnum));

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
    ICOMMAND(listclients, "i", (int *local), listclients(*local!=0));

	void clearbans()
	{
		addmsg(SV_CLEARBANS, "r");
	}
	ICOMMAND(clearbans, "", (char *s), clearbans());

	void kick(const char *arg)
	{
		int i = parseplayer(arg);
		if(i>=0 && i!=game::player1->clientnum) addmsg(SV_KICK, "ri", i);
	}
	ICOMMAND(kick, "s", (char *s), kick(s));

	void setteam(const char *arg1, const char *arg2)
	{
		if(m_fight(game::gamemode) && m_team(game::gamemode, game::mutators))
		{
			int i = parseplayer(arg1);
			if(i>=0 && i!=game::player1->clientnum)
			{
				int t = teamname(arg2);
				if(t) addmsg(SV_SETTEAM, "ri2", i, t);
			}
		}
		else conoutft(CON_MESG, "\frcan only change teams in team games");
	}
	ICOMMAND(setteam, "ss", (char *who, char *team), setteam(who, team));

    void hashpwd(const char *pwd)
    {
        if(game::player1->clientnum<0) return;
        string hash;
        server::hashpassword(game::player1->clientnum, sessionid, pwd, hash);
        result(hash);
    }
    COMMAND(hashpwd, "s");

    void setmaster(const char *arg)
    {
        if(!arg[0]) return;
        int val = 1;
        mkstring(hash);
        if(!arg[1] && isdigit(arg[0])) val = atoi(arg);
        else server::hashpassword(game::player1->clientnum, sessionid, arg, hash);
        addmsg(SV_SETMASTER, "ris", val, hash);
    }
	COMMAND(setmaster, "s");

    void tryauth()
    {
        if(!authname[0]) return;
        lastauth = lastmillis;
        addmsg(SV_AUTHTRY, "rs", authname);
    }
	ICOMMAND(auth, "", (), tryauth());

    void togglespectator(int val, const char *who)
	{
        int i = who[0] ? parseplayer(who) : game::player1->clientnum;
		if(i>=0) addmsg(SV_SPECTATOR, "rii", i, val);
	}
	ICOMMAND(spectator, "is", (int *val, char *who), togglespectator(*val, who));

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
		if(!colourchat) filtertext(text, text);
		mkstring(s);
		bool team = flags&SAY_TEAM;
		defformatstring(m)("%s", game::colorname(d));
		if(team)
		{
			defformatstring(t)(" (\fs%s%s\fS)", teamtype[d->team].chat, teamtype[d->team].name);
			concatstring(m, t);
		}
		if(flags&SAY_ACTION) formatstring(s)("\fm* \fs%s\fS \fs\fm%s\fS", m, text);
		else formatstring(s)("\fa<\fs\fw%s\fS> \fs\fw%s\fS", m, text);

		if(d->state != CS_SPECTATOR)
		{
			defformatstring(ds)("<sub>%s", s);
			part_textcopy(d->abovehead(), ds, PART_TEXT, game::aboveheadfade, 0xFFFFFF, 2, 1, -10, 0, d);
		}

		conoutft(CON_CHAT, "%s", s);
		playsound(S_CHAT, camera1->o, camera1, SND_FORCED);
	}

	void toserver(int flags, char *text)
	{
		if(ready())
		{
			saytext(game::player1, flags, text);
			addmsg(SV_TEXT, "ri2s", game::player1->clientnum, flags, text);
		}
	}
	ICOMMAND(say, "C", (char *s), toserver(SAY_NONE, s));
	ICOMMAND(me, "C", (char *s), toserver(SAY_ACTION, s));
	ICOMMAND(sayteam, "C", (char *s), toserver(SAY_TEAM, s));
	ICOMMAND(meteam, "C", (char *s), toserver(SAY_ACTION|SAY_TEAM, s));

	void parsecommand(gameent *d, const char *cmd, const char *arg)
	{
		ident *id = idents->access(cmd);
		if(id && id->flags&IDF_CLIENT)
		{
			string val;
			val[0] = 0;
			switch(id->type)
			{
#if 0 // these shouldn't get here
				case ID_COMMAND:
				case ID_COMMAND:
				{
					string s;
					formatstring(s)("%s %s", cmd, arg);
					char *ret = executeret(s);
					if(ret)
					{
						conoutft(CON_MESG, "\fg%s: %s", cmd, ret);
						delete[] ret;
					}
					return;
				}
#endif
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
			addmsg(SV_COMMAND, "ri2ss", game::player1->clientnum, nargs, cmd, arg);
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
		if(editmode) toggleedit();
		game::gamemode = gamemode; game::mutators = mutators;
		server::modecheck(&game::gamemode, &game::mutators);
		game::nextmode = game::gamemode; game::nextmuts = game::mutators;
		game::minremain = -1;
		game::maptime = 0;
		mapvotes.setsize(0);
		if(editmode && !allowedittoggle(editmode)) toggleedit();
		if(m_demo(gamemode)) return;
		needsmap = false;
		if(!name || !*name || !load_world(name, temp))
		{
			emptymap(0, true, NULL);
			setnames(name, MAP_BFGZ);
			needsmap = true;
		}
		if(editmode) edittoggled(editmode);
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
			case SV_SENDDEMO:
			{
				defformatstring(fname)("%d.dmo", lastmillis);
				stream *demo = openfile(fname, "wb");
				if(!demo) return;
				conoutft(CON_MESG, "received demo \"%s\"", fname);
				demo->write(data, len);
                delete demo;
				break;
			}

			case SV_SENDMAPCONFIG:
			case SV_SENDMAPSHOT:
			case SV_SENDMAPFILE:
			{
				const char *reqmap = mapname, *reqext = "xxx";
				if(type == SV_SENDMAPCONFIG) reqext = "cfg";
				else if(type == SV_SENDMAPSHOT) reqext = "png";
				else if(type == SV_SENDMAPFILE) reqext = "bgz";
				if(!reqmap || !*reqmap) reqmap = "maps/untitled";
				defformatstring(reqfile)(strstr(reqmap, "temp/")==reqmap || strstr(reqmap, "temp\\")==reqmap ? "%s" : "temp/%s", reqmap);
				defformatstring(reqfext)("%s.%s", reqfile, reqext);
				stream *f = openfile(reqfext, "wb");
				if(!f)
				{
					conoutf("\frfailed to open map file: %s", reqfext);
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
	ICOMMAND(getmap, "", (), addmsg(SV_GETMAP, "r"));

	void stopdemo()
	{
		if(remote) addmsg(SV_STOPDEMO, "r");
		else server::stopdemo();
	}
	ICOMMAND(stopdemo, "", (), stopdemo());

    void recorddemo(int val)
	{
        addmsg(SV_RECORDDEMO, "ri", val);
	}
	ICOMMAND(recorddemo, "i", (int *val), recorddemo(*val));

	void cleardemos(int val)
	{
        addmsg(SV_CLEARDEMOS, "ri", val);
	}
	ICOMMAND(cleardemos, "i", (int *val), cleardemos(*val));

    void getdemo(int i)
	{
		if(i<=0) conoutf("getting demo...");
		else conoutf("getting demo %d...", i);
		addmsg(SV_GETDEMO, "ri", i);
	}
	ICOMMAND(getdemo, "i", (int *val), getdemo(*val));

	void listdemos()
	{
		conoutf("listing demos...");
		addmsg(SV_LISTDEMOS, "r");
	}
	ICOMMAND(listdemos, "", (), listdemos());

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
			addmsg(SV_MAPVOTE, "rsi2", reqfile, nextmode, nextmuts);
        }
	}
	ICOMMAND(map, "s", (char *s), changemap(s));

	void sendmap()
	{
		conoutf("sending map...");
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
				conoutf("\fgtransmitting file: %s", reqfext);
				sendfile(-1, 2, f, "ri", SV_SENDMAPFILE+i);
                delete f;
			}
			else conoutf("\frfailed to open map file: %s", reqfext);
		}
	}
	ICOMMAND(sendmap, "", (), sendmap());

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
	ICOMMAND(goto, "s", (char *s), gotoplayer(s));

	bool ready() { return connected(false) && isready && game::maptime > 0; }
	ICOMMAND(ready, "", (), intret(ready()));

	int state() { return game::player1->state; }
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
					addmsg(SV_EDITVAR, "risi", id->type, id->name, *id->storage.i);
					break;
        		case ID_FVAR:
					addmsg(SV_EDITVAR, "risf", id->type, id->name, *id->storage.f);
					break;
        		case ID_SVAR:
					addmsg(SV_EDITVAR, "riss", id->type, id->name, *id->storage.s);
					break;
        		case ID_ALIAS:
					addmsg(SV_EDITVAR, "riss", id->type, id->name, id->action);
					break;
				default: break;
        	}
        }
	}

	void edittrigger(const selinfo &sel, int op, int arg1, int arg2, int arg3)
	{
        if(m_edit(game::gamemode)) switch(op)
		{
			case EDIT_FLIP:
			case EDIT_COPY:
			case EDIT_PASTE:
			case EDIT_DELCUBE:
			{
				addmsg(SV_EDITF + op, "ri9i4",
					sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
					sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner);
				break;
			}
			case EDIT_MAT:
			case EDIT_ROTATE:
			{
				addmsg(SV_EDITF + op, "ri9i5",
					sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
					sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
					arg1);
				break;
			}
			case EDIT_FACE:
			case EDIT_TEX:
			case EDIT_REPLACE:
			{
				addmsg(SV_EDITF + op, "ri9i6",
					sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
					sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
					arg1, arg2);
				break;
			}
            case EDIT_REMIP:
            {
                addmsg(SV_EDITF + op, "r");
                break;
            }
		}
	}

    void sendintro()
    {
		packetbuf p(MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
        putint(p, SV_CONNECT);
        sendstring(game::player1->name, p);
        mkstring(hash);
        if(connectpass[0])
        {
            server::hashpassword(game::player1->clientnum, sessionid, connectpass, hash);
            memset(connectpass, 0, sizeof(connectpass));
        }
        sendstring(hash, p);
        sendclientpacket(p.finalize(), 1);
    }

	void updateposition(gameent *d)
	{
        if((d->state==CS_ALIVE || d->state==CS_EDITING) && (!d->ai || !d->ai->suspended))
		{
			// send position updates separately so as to not stall out aiming
			packetbuf q(100);
			putint(q, SV_POS);
			putint(q, d->clientnum);
			putuint(q, (int)(d->o.x*DMF));			  // quantize coordinates to 1/4th of a cube, between 1 and 3 bytes
			putuint(q, (int)(d->o.y*DMF));
			putuint(q, (int)((d->o.z - d->height)*DMF));
			putuint(q, (int)d->yaw);
			putint(q, (int)d->pitch);
			putint(q, (int)d->roll);
			putint(q, (int)(d->vel.x*DVELF));		  // quantize to itself, almost always 1 byte
			putint(q, (int)(d->vel.y*DVELF));
			putint(q, (int)(d->vel.z*DVELF));
            putuint(q, d->physstate | (d->falling.x || d->falling.y ? 0x20 : 0) | (d->falling.z ? 0x10 : 0));
            if(d->falling.x || d->falling.y)
            {
                putint(q, (int)(d->falling.x*DVELF));      // quantize to itself, almost always 1 byte
                putint(q, (int)(d->falling.y*DVELF));
            }
            if(d->falling.z) putint(q, (int)(d->falling.z*DVELF));
			// pack rest in almost always 1 byte: strafe:2, move:2, crouching: 1, aimyaw/aimpitch: 1
			uint flags = (d->strafe&3) | ((d->move&3)<<2) |
				((d->action[AC_CROUCH] ? 1 : 0)<<4) | ((d->turnside || ((d->action[AC_IMPULSE] && (d->move || d->strafe))) ? 1 : 0)<<6) | ((d->conopen ? 1 : 0)<<7) |
					((int)d->aimyaw!=(int)d->yaw || (int)d->aimpitch!=(int)d->pitch ? 0x20 : 0);
			putuint(q, flags);
            if(flags&0x20)
            {
                putuint(q, (int)d->aimyaw);
                putint(q, (int)d->aimpitch);
            }
			sendclientpacket(q.finalize(), 0);
		}
	}

    void sendmessages(gameent *d)
    {
		packetbuf p(MAXTRANS);
		if(sendcrc)
		{
			p.reliable();
			sendcrc = false;
			putint(p, SV_MAPCRC);
			sendstring(game::clientmap, p);
			putint(p, game::clientmap[0] ? getmapcrc() : 0);
		}
		if(sendinfo && !needsmap)
		{
            p.reliable();
			putint(p, SV_GAMEINFO);
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
            messages.setsizenodelete(0);
            if(messagereliable) p.reliable();
            messagereliable = false;
        }
		if(lastmillis-lastping>250)
		{
			putint(p, SV_PING);
			putint(p, lastmillis);
			lastping = lastmillis;
		}

        sendclientpacket(p.finalize(), 1);
	}

    void c2sinfo() // send update to the server
    {
        static int lastupdate = -1000;
        if(totalmillis-lastupdate < 40) return;    // don't update faster than 25fps
        lastupdate = totalmillis;
        updateposition(game::player1);
        loopv(game::players) if(game::players[i] && game::players[i]->ai) updateposition(game::players[i]);
        sendmessages(game::player1);
        flushclient();
    }

    void parsestate(gameent *d, ucharbuf &p, bool resume = false)
    {
        if(!d) { static gameent dummy; d = &dummy; }
		if(d == game::player1 || d->ai) getint(p);
		else d->state = getint(p);
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
            if(m_arena(game::gamemode, game::mutators)) d->arenaweap = d->weapselect;
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
			else	  d->o.x += dx<0 ? r-fx : -(r-fx);
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
			case SV_POS:						// position of another client
			{
				int lcn = getint(p);
				vec o, vel, falling;
				float yaw, pitch, roll, aimyaw, aimpitch;
				int physstate, f;
				o.x = getuint(p)/DMF;
				o.y = getuint(p)/DMF;
				o.z = getuint(p)/DMF;
				aimyaw = yaw = (float)getuint(p);
				aimpitch = pitch = (float)getint(p);
				roll = (float)getint(p);
				vel.x = getint(p)/DVELF;
				vel.y = getint(p)/DVELF;
				vel.z = getint(p)/DVELF;
				physstate = getuint(p);
                falling = vec(0, 0, 0);
                if(physstate&0x20)
                {
                    falling.x = getint(p)/DVELF;
                    falling.y = getint(p)/DVELF;
                }
                if(physstate&0x10) falling.z = getint(p)/DVELF;
				f = getuint(p);
                if(f&0x20)
                {
                    aimyaw = (float)getuint(p);
                    aimpitch = (float)getint(p);
                }
				gameent *d = game::getclient(lcn);
                if(!d || d==game::player1 || d->ai) continue;
                float oldyaw = d->yaw, oldpitch = d->pitch, oldaimyaw = d->aimyaw, oldaimpitch = d->aimpitch;
				d->action[AC_IMPULSE] = f&0x40 ? true : false;
				d->conopen = f&0x80 ? true : false;
				d->yaw = yaw;
				d->pitch = pitch;
				d->roll = roll;
				d->aimyaw = aimyaw;
				d->aimpitch = aimpitch;
				d->strafe = (f&3)==3 ? -1 : f&3; f >>= 2;
				d->move = (f&3)==3 ? -1 : f&3; f >>= 2;
				bool crouch = d->action[AC_CROUCH];
				d->action[AC_CROUCH] = f&1 ? true : false;
				if(crouch != d->action[AC_CROUCH]) d->actiontime[AC_CROUCH] = lastmillis;
                vec oldpos(d->o);
                //if(game::allowmove(d))
                //{
                    d->o = o;
                    d->o.z += d->height;
                    d->vel = vel;
                    d->falling = falling;
                    d->physstate = physstate & 0x0F;
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
				case SV_SERVERINIT:					// welcome messsage from the server
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
                case SV_WELCOME: isready = true; break;

				case SV_CLIENT:
				{
					int lcn = getint(p), len = getuint(p);
					ucharbuf q = p.subbuf(len);
					gameent *t = game::getclient(lcn);
					parsemessages(lcn, t, q);
					break;
				}

				case SV_PHYS: // simple phys events
				{
					int lcn = getint(p), st = getint(p);
					gameent *t = game::getclient(lcn);
					if(t && t != game::player1 && !t->ai) switch(st)
					{
						case SPHY_JUMP:
						{
							playsound(S_JUMP, t->o, t); regularshape(PART_SMOKE, int(t->radius), 0x111111, 21, 20, 100, t->feetpos(), 1, 1, -10, 0, 10.f);
							t->actiontime[AC_JUMP] = lastmillis;
							break;
						}
						case SPHY_IMPULSE:
						{
							playsound(S_IMPULSE, t->o, t); game::impulseeffect(t, true);
							break;
						}
						case SPHY_POWER:
						{
							t->setweapstate(t->weapselect, WEAP_S_POWER, 0, lastmillis);
							break;
						}
						case SPHY_EXTINGUISH:
						{
							if(issound(t->fschan)) removesound(t->fschan);
							t->fschan = -1; t->lastfire = 0;
							playsound(S_EXTINGUISH, t->o, t, 0, 128);
							break;
						}
						default: break;
					}
					break;
				}

				case SV_ANNOUNCE:
				{
					int snd = getint(p), targ = getint(p);
					getstring(text, p);
					if(targ >= 0 && text[0]) game::announce(snd, targ, NULL, "%s", text);
					else game::announce(snd, -1, NULL, "");
					break;
				}

				case SV_TEXT:
				{
					int tcn = getint(p);
					gameent *t = game::getclient(tcn);
					int flags = getint(p);
					getstring(text, p);
					if(!t) break;
					saytext(t, flags, text);
					break;
				}

				case SV_COMMAND:
				{
					int lcn = getint(p);
					gameent *f = game::getclient(lcn);
					string cmd;
					getstring(cmd, p);
					getstring(text, p);
					parsecommand(f ? f : NULL, cmd, text);
					break;
				}

				case SV_EXECLINK:
				{
					int tcn = getint(p), index = getint(p);
					gameent *t = game::getclient(tcn);
					if(!t || !d || (t->clientnum != d->clientnum && t->ownernum != d->clientnum) || t == game::player1 || t->ai) break;
					entities::execlink(t, index, false);
					break;
				}

				case SV_MAPCHANGE:
				{
					int hasmap = getint(p);
					if(hasmap) getstring(text, p);
					int getit = getint(p), mode = getint(p), muts = getint(p);
					changemapserv(hasmap && getit != 1 ? text : NULL, mode, muts, getit == 2);
					mapchanged = true;
					if(needsmap && multiplayer(false))
					{
						switch(getit)
						{
							case 0:
							{
								conoutf("\fcserver requested map change to %s, and we need it, so asking for it", hasmap ? text : "<temp>");
								addmsg(SV_GETMAP, "r");
								break;
							}
							case 2:
							{
								conoutf("\fcseem to have failed to get map to %s, try /getmap", hasmap ? text : "<temp>");
								needsmap = false; // we failed sir
								break;
							}
						}
					}
					else needsmap = false; // assume empty map
					break;
				}

				case SV_GAMEINFO:
				{
					int n;
					while((n = getint(p)) != -1) entities::setspawn(n, getint(p));
					sendinfo = false;
					break;
				}

				case SV_NEWGAME: // server requests next game
				{
					hud::sb.showscores(false);
					if(!menuactive()) showgui("game");
					break;
				}

				case SV_SWITCHNAME:
					getstring(text, p);
					if(!d) break;
					filtertext(text, text, true, MAXNAMELEN);
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

				case SV_CLIENTINIT: // another client either connected or changed name/team
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
					filtertext(text, text, true, MAXNAMELEN);
					if(!text[0]) copystring(text, "unnamed");
					if(d->name[0])		  // already connected
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
					else					// new client
					{
						if(game::showplayerinfo)
							conoutft(game::showplayerinfo > 1 ? int(CON_EVENT) : int(CON_MESG), "\fg%s has joined the game", game::colorname(d, text, "", false));
						loopv(game::players)	// clear copies since new player doesn't have them
							if(game::players[i]) freeeditinfo(game::players[i]->edit);
						freeeditinfo(localedit);
					}
					copystring(d->name, text, MAXNAMELEN+1);
					int team = clamp(getint(p), int(TEAM_NEUTRAL), int(TEAM_ENEMY));
					if(d == game::player1 && d->team != team) hud::lastteam = 0;
					d->team = team;
					break;
				}

				case SV_DISCONNECT:
					game::clientdisconnected(getint(p));
					break;

				case SV_SPAWN:
				{
					int lcn = getint(p);
					gameent *f = game::newclient(lcn);
					if(f && f != game::player1 && !f->ai)
					{
						f->respawn(lastmillis, m_health(game::gamemode, game::mutators));
						parsestate(f, p);
						playsound(S_RESPAWN, f->o, f);
						game::spawneffect(PART_FIREBALL, vec(f->o).sub(vec(0, 0, f->height/2.f)), teamtype[f->team].colour, int(f->radius*2));
					}
					else parsestate(NULL, p);
					break;
				}

				case SV_ARENAWEAP:
				{
					hud::sb.showscores(false);
					if(!menuactive()) showgui("arena");
					break;
				}

				case SV_SPAWNSTATE:
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
						addmsg(SV_SPAWN, "ri", f->clientnum);
						entities::spawnplayer(f, ent, true);
						playsound(S_RESPAWN, f->o, f);
						game::spawneffect(PART_FIREBALL, vec(f->o).sub(vec(0, 0, f->height/2.f)), teamtype[f->team].colour, int(f->radius*2));
					}
					ai::spawned(f, ent);
					if(f == game::player1) game::resetcamera();
					break;
				}

				case SV_SHOTFX:
				{
					int scn = getint(p), weap = getint(p), flags = getint(p), power = getint(p);
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
					s->setweapstate(weap, WEAP_S_SHOOT, weaptype[weap].adelay[flags&HIT_ALT ? 1 : 0], lastmillis);
					s->weapshot[weap] = weaptype[weap].sub[flags&HIT_ALT ? 1 : 0];
					s->totalshots += int(weaptype[weap].damage[flags&HIT_ALT ? 1 : 0]*damagescale)*weaptype[weap].rays[flags&HIT_ALT ? 1 : 0];
					projs::shootv(weap, flags, power, from, locs, s, false);
					break;
				}

				case SV_DAMAGE:
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

				case SV_RELOAD:
				{
					int trg = getint(p), weap = getint(p), amt = getint(p), ammo = getint(p);
					gameent *target = game::getclient(trg);
					if(!target || !isweap(weap)) break;
					weapons::weapreload(target, weap, amt, ammo, false);
					break;
				}

				case SV_REGEN:
				{
					int trg = getint(p), heal = getint(p), amt = getint(p);
					gameent *f = game::getclient(trg);
					if(!f) break;
					if(!amt)
					{
						f->impulse[IM_METER] = 0;
						if(issound(f->fschan)) removesound(f->fschan);
						f->fschan = -1; f->lastfire = 0;
					}
					else if(amt > 0 && (!f->lastregen || lastmillis-f->lastregen >= 500)) playsound(S_REGEN, f->o, f);
					f->health = heal; f->lastregen = lastmillis;
					break;
				}

				case SV_DIED:
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

				case SV_POINTS:
				{
					int acn = getint(p), add = getint(p), points = getint(p);
					gameent *actor = game::getclient(acn);
					if(!actor) break;
					actor->lastpoints = add;
					actor->points = points;
					break;
				}

				case SV_DROP:
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

				case SV_WEAPSELECT:
				{
					int trg = getint(p), weap = getint(p);
					gameent *target = game::getclient(trg);
					if(!target || !isweap(weap)) break;
					weapons::weapselect(target, weap, false);
					break;
				}

				case SV_RESUME:
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

				case SV_ITEMSPAWN:
				{
					int ent = getint(p);
					if(!entities::ents.inrange(ent)) break;
					entities::setspawn(ent, 1);
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
								defformatstring(ds)("<emphasis>%s (%d)", item, entities::ents[ent]->type);
								part_textcopy(pos, ds, PART_TEXT, game::aboveheadfade, colour, 2, 1, -10);
							}
						}
					}
					game::spawneffect(PART_FIREBALL, entities::ents[ent]->o, entities::ents[ent]->type == WEAPON ? colour : 0x6666FF, enttype[entities::ents[ent]->type].radius, 5);
					break;
				}

				case SV_TRIGGER:
				{
					int ent = getint(p), st = getint(p);
					entities::setspawn(ent, st);
					break;
				}

				case SV_ITEMACC:
				{ // uses a specific drop so the client knows what to replace
					int lcn = getint(p), ent = getint(p), spawn = getint(p), weap = getint(p), drop = getint(p);
					gameent *target = game::getclient(lcn);
					if(!target) break;
					if(entities::ents.inrange(ent) && enttype[entities::ents[ent]->type].usetype == EU_ITEM)
						entities::useeffects(target, ent, spawn, weap, drop);
					break;
				}

				case SV_EDITVAR:
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

				case SV_EDITF:			  // coop editing messages
				case SV_EDITT:
				case SV_EDITM:
				case SV_FLIP:
				case SV_COPY:
				case SV_PASTE:
				case SV_ROTATE:
				case SV_REPLACE:
				case SV_DELCUBE:
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
						case SV_EDITF: dir = getint(p); mode = getint(p); mpeditface(dir, mode, s, false); break;
						case SV_EDITT: tex = getint(p); allfaces = getint(p); mpedittex(tex, allfaces, s, false); break;
						case SV_EDITM: mat = getint(p); mpeditmat(mat, s, false); break;
						case SV_FLIP: mpflip(s, false); break;
						case SV_COPY: if(d) mpcopy(d->edit, s, false); break;
						case SV_PASTE: if(d) mppaste(d->edit, s, false); break;
						case SV_ROTATE: dir = getint(p); mprotate(dir, s, false); break;
						case SV_REPLACE: tex = getint(p); newtex = getint(p); mpreplacetex(tex, newtex, s, false); break;
						case SV_DELCUBE: mpdelcube(s, false); break;
					}
					break;
				}
				case SV_REMIP:
				{
					if(!d) return;
					conoutft(CON_MESG, "%s remipped", game::colorname(d));
					mpremip(false);
					break;
				}
				case SV_EDITENT:			// coop edit of ent
				{
					if(!d) return;
					int i = getint(p);
					float x = getint(p)/DMF, y = getint(p)/DMF, z = getint(p)/DMF;
					int type = getint(p), numattrs = getint(p);
					static vector<int> attrs; attrs.setsizenodelete(0);
					loopk(numattrs) attrs.add(getint(p));
					mpeditent(i, vec(x, y, z), type, attrs, false);
					entities::setspawn(i, 0);
					entities::clearentcache();
					break;
				}

				case SV_EDITLINK:
				{
					if(!d) return;
					int b = getint(p), index = getint(p), node = getint(p);
					entities::linkents(index, node, b!=0, false, false);
				}

				case SV_PONG:
					addmsg(SV_CLIENTPING, "i", game::player1->ping = (game::player1->ping*5+lastmillis-getint(p))/6);
					break;

				case SV_CLIENTPING:
					if(!d) return;
					d->ping = getint(p);
					break;

				case SV_TIMEUP:
					game::timeupdate(getint(p));
					break;

				case SV_SERVMSG:
				{
					int lev = getint(p);
					getstring(text, p);
					conoutft(lev >= 0 && lev < CON_MAX ? lev : CON_INFO, "%s", text);
					break;
				}

				case SV_SENDDEMOLIST:
				{
					int demos = getint(p);
					if(!demos) conoutf("\frno demos available");
					else loopi(demos)
					{
						getstring(text, p);
						conoutf("\fa%d. %s", i+1, text);
					}
					break;
				}

				case SV_DEMOPLAYBACK:
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

				case SV_CURRENTMASTER:
				{
					int mn = getint(p), priv = getint(p);
					if(mn >= 0)
					{
						gameent *m = game::getclient(mn);
						if(m) m->privilege = priv;
					}
					break;
				}

				case SV_EDITMODE:
				{
					int val = getint(p);
					if(!d) break;
					if(val) d->state = CS_EDITING;
					else
					{
						d->state = CS_ALIVE;
						d->editspawn(lastmillis, m_weapon(game::gamemode, game::mutators), m_health(game::gamemode, game::mutators), m_arena(game::gamemode, game::mutators), spawngrenades >= (m_insta(game::gamemode, game::mutators) ? 2 : 1));
					}
					d->resetinterp();
					projs::remove(d);
					break;
				}

				case SV_SPECTATOR:
				{
					int sn = getint(p), val = getint(p);
					gameent *s = game::newclient(sn);
					if(!s) break;
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

				case SV_WAITING:
				{
					int sn = getint(p);
					gameent *s = game::newclient(sn);
					if(!s) break;
					if(s == game::player1)
					{
						if(editmode) toggleedit();
						hud::sb.showscores(false);
					}
					else if(!s->ai) s->resetinterp();
					if(s->state == CS_ALIVE) s->lastdeath = lastmillis; // so spawndelay shows properly
					s->state = CS_WAITING;
					s->weapreset(true);
					break;
				}

				case SV_SETTEAM:
				{
					int wn = getint(p), tn = getint(p);
					gameent *w = game::getclient(wn);
					if(!w) return;
					if(w == game::player1 && w->team != tn) hud::lastteam = 0;
					w->team = tn;
					break;
				}

				case SV_FLAGINFO:
				{
					int flag = getint(p), converted = getint(p),
							owner = getint(p), enemy = getint(p);
					if(m_stf(game::gamemode)) stf::updateflag(flag, owner, enemy, converted);
					break;
				}

				case SV_FLAGS:
				{
					int numflags = getint(p);
					loopi(numflags)
					{
						int converted = getint(p), owner = getint(p), enemy = getint(p);
						stf::st.initflag(i, owner, enemy, converted);
					}
					break;
				}

				case SV_MAPVOTE:
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

				case SV_CHECKPOINT:
				{
					int tn = getint(p), laptime = getint(p), besttime = getint(p);
					gameent *t = game::getclient(tn);
					if(!t) break;
					t->cplast = laptime;
					t->cptime = besttime;
					t->cpmillis = t->impulse[IM_METER] = 0;
					if(showlaptimes > (t != game::player1 ? (t->aitype >= 0 ? 2 : 1) : 0))
					{
						defformatstring(best)("%s", hud::sb.timetostr(besttime));
						conoutft(t != game::player1 ? CON_INFO : CON_SELF, "%s lap time: \fs\fg%s\fS (best: \fs\fy%s\fS)", game::colorname(t), hud::sb.timetostr(laptime), best);
					}
				}

				case SV_SCORE:
				{
					int team = getint(p), total = getint(p);
					if(m_stf(game::gamemode)) stf::setscore(team, total);
					else if(m_ctf(game::gamemode)) ctf::setscore(team, total);
					break;
				}

				case SV_INITFLAGS:
				{
					ctf::parseflags(p, m_ctf(game::gamemode));
					break;
				}

				case SV_DROPFLAG:
				{
					int ocn = getint(p), flag = getint(p);
					vec droploc;
					loopk(3) droploc[k] = getint(p)/DMF;
					gameent *o = game::newclient(ocn);
					if(o && m_ctf(game::gamemode)) ctf::dropflag(o, flag, droploc);
					break;
				}

				case SV_SCOREFLAG:
				{
					int ocn = getint(p), relayflag = getint(p), goalflag = getint(p), score = getint(p);
					gameent *o = game::newclient(ocn);
					if(o && m_ctf(game::gamemode)) ctf::scoreflag(o, relayflag, goalflag, score);
					break;
				}

				case SV_RETURNFLAG:
				{
					int ocn = getint(p), flag = getint(p);
					gameent *o = game::newclient(ocn);
					if(o && m_ctf(game::gamemode)) ctf::returnflag(o, flag);
					break;
				}

				case SV_TAKEFLAG:
				{
					int ocn = getint(p), flag = getint(p);
					gameent *o = game::newclient(ocn);
					if(o && m_ctf(game::gamemode)) ctf::takeflag(o, flag);
					break;
				}

				case SV_RESETFLAG:
				{
					int flag = getint(p);
					if(m_ctf(game::gamemode)) ctf::resetflag(flag);
					break;
				}

				case SV_GETMAP:
				{
					conoutf("\fcserver has requested we send the map..");
					if(!needsmap && !gettingmap) sendmap();
					else
					{
						if(!gettingmap) conoutf("\frwe don't have the map though, so asking for it instead");
						else conoutf("\frbut we're in the process of getting it");
							addmsg(SV_GETMAP, "r");
					}
					break;
				}

				case SV_SENDMAP:
				{
					conoutf("\fcmap data has been uploaded");
					if(needsmap && !gettingmap)
					{
						conoutf("\frwe want the map too, so asking for it");
						addmsg(SV_GETMAP, "r");
					}
					break;
				}

				case SV_NEWMAP:
				{
					int size = getint(p);
					if(size>=0) emptymap(size, true);
					else enlargemap(true);
					mapvotes.setsize(0);
					needsmap = false;
					if(d && d!=game::player1)
					{
						int newsize = 0;
						while(1<<newsize < getworldsize()) newsize++;
						conoutft(CON_MESG, size>=0 ? "%s started a new map of size %d" : "%s enlarged the map to size %d", game::colorname(d), newsize);
					}
					break;
				}

				case SV_INITAI:
				{
					int bn = getint(p), on = getint(p), at = getint(p), et = getint(p), sk = clamp(getint(p), 1, 101);
					getstring(text, p);
					int tm = getint(p);
					gameent *b = game::newclient(bn);
					if(!b) break;
					ai::init(b, at, et, on, sk, bn, text, tm);
					break;
				}

				case SV_AUTHCHAL:
				{
					uint id = (uint)getint(p);
					getstring(text, p);
					conoutft(CON_MESG, "server is challenging authentication details..");
					if(lastauth && lastmillis-lastauth < 60*1000 && authname[0])
					{
						vector<char> buf;
                        answerchallenge(authkey, text, buf);
						addmsg(SV_AUTHANS, "ris", id, buf.getbuf());
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
		defformatstring(u)("serversort [%d %d %d]", SINFO_STATUS, SINFO_PLAYERS, SINFO_PING);
		execute(u);
	}
	ICOMMAND(serversortreset, "", (), resetserversort());

	int servercompare(serverinfo *a, serverinfo *b)
	{
		if(!serversort || !*serversort) resetserversort();
		int ac = a->address.host == ENET_HOST_ANY || a->ping >= 999 || a->attr.empty() || a->attr[0] != GAMEVERSION ? -1 : 0,
			bc = b->address.host == ENET_HOST_ANY || b->ping >= 999 || b->attr.empty() || b->attr[0] != GAMEVERSION ? -1 : 0;
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

			switch (style)
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
					if(aa->attr.length() > 4) ac = aa->attr[4];
					else ac = 0;

					if(ab->attr.length() > 4) bc = ab->attr[4];
					else bc = 0;

					retsw(ac, bc, false);
					break;
				}
				case SINFO_GAME:
				{
					if(aa->attr.length() > 1) ac = aa->attr[1];
					else ac = 0;

					if(ab->attr.length() > 1) bc = ab->attr[1];
					else bc = 0;

					retsw(ac, bc, true);

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
				default:
					break;
			}
		}
		return strcmp(a->name, b->name);
	}

	void parsepacketclient(int chan, packetbuf &p)	// processes any updates from the server
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

    void serverstartcolumn(guient *g, int i)
    {
		g->pushlist();
		if(g->buttonf("%s ", 0xFFFFFF, NULL, i ? serverinfotypes[i] : "") & GUI_UP)
		{
			mkstring(st);
			bool invert = false;
			int len = execute("listlen $serversort");
			loopk(len)
			{
				defformatstring(s)("at $serversort %d", k);

				int n = execute(s);
				if(abs(n) != i)
				{
					defformatstring(t)("%s%d", st[0] ? " " : "", n);
					formatstring(st)("%s%s", st[0] ? st : "", t);
				}
				else if(!k) invert = true;
			}
			defformatstring(u)("serversort [%d%s%s]",
				invert ? 0-i : i, st[0] ? " " : "", st[0] ? st : "");
			execute(u);
		}

		g->mergehits(true);
    }

    void serverendcolumn(guient *g, int i)
    {
        g->mergehits(false);
        g->poplist();
    }

    bool serverentry(guient *g, int i, serverinfo *si)
    {
		mkstring(text);
		int status = serverstat(si), colour = serverstatus[status].colour;
		if(status == SSTAT_OPEN && !strcmp(si->sdesc, servermaster)) colour |= 0x222222;
		switch(i)
		{
			case SINFO_STATUS:
			{
				if(g->button("", colour, serverstatus[status].icon) & GUI_UP)
					return true;
				break;
			}
			case SINFO_DESC:
			{
				copystring(text, si->sdesc, 24);
				if(g->buttonf("%s ", colour, NULL, text) & GUI_UP) return true;
				break;
			}
			case SINFO_PING:
			{
				formatstring(text)("%d", si->ping);
				if(g->buttonf("%s ", colour, NULL, text) & GUI_UP) return true;
				break;
			}
			case SINFO_PLAYERS:
			{
				formatstring(text)("%d", si->numplayers);
				if(g->buttonf("%s ", colour, NULL, text) & GUI_UP) return true;
				break;
			}
			case SINFO_MAXCLIENTS:
			{
				if(si->attr.length() > 4 && si->attr[4] >= 0) formatstring(text)("%d", si->attr[4]);
				if(g->buttonf("%s ", colour, NULL, text) & GUI_UP) return true;
				break;
			}
			case SINFO_GAME:
			{
				if(si->attr.length() > 1) formatstring(text)("%s", server::gamename(si->attr[1], si->attr[2]));
				if(g->buttonf("%s ", colour, NULL, text) & GUI_UP) return true;
				break;
			}
			case SINFO_MAP:
			{
				copystring(text, si->map, 18);
				if(g->buttonf("%s ", colour, NULL, text) & GUI_UP) return true;
				break;
			}
			case SINFO_TIME:
			{
				if(si->attr.length() > 3 && si->attr[3] >= 0)
					formatstring(text)("%d %s", si->attr[3], si->attr[3] == 1 ? "min" : "mins");
				if(g->buttonf("%s ", colour, NULL, text) & GUI_UP) return true;
				break;
			}
			default:
				break;
		}
		return false;
    }

	int serverbrowser(guient *g)
	{
		if(servers.empty())
		{
			g->pushlist();
			g->text("No servers, press UPDATE to see some..", 0xFFFFFF);
			g->poplist();
			return -1;
		}
		loopv(servers) if(!strcmp(servers[i]->sdesc, servermaster))
		{
			if(servers[i]->attr[0] > GAMEVERSION)
			{
				g->pushlist();
				g->textf("\fs\fgNEW VERSION RELEASED!\fS Please visit \fs\fb%s\fS for more information.", 0xFFFFFF, "info", ENG_URL);
				g->poplist();
			}
			break;
		}
		int n = -1;
		for(int start = 0; start < servers.length();)
		{
			if(start > 0) g->tab();
			int end = servers.length();
			g->pushlist();
			loopi(SINFO_MAX)
			{
				serverstartcolumn(g, i);
				for(int j = start; j < end; j++)
				{
					if(!i && g->shouldtab()) { end = j; break; }
					serverinfo *si = servers[j];
					if(si->ping < 999 && si->attr.length() && si->attr[0] == GAMEVERSION)
					{
						if(serverentry(g, i, si)) n = j;
					}
				}
				serverendcolumn(g, i);
			}
			g->poplist();
			start = end;
		}
		return n;
	}
}
