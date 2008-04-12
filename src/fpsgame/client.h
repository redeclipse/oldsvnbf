struct clientcom : iclientcom
{
	GAMECLIENT &cl;

	bool c2sinit;		// whether we need to tell the other clients our stats

	bool senditemstoserver;	 // after a map change, since server doesn't have map data
	int lastping;

    bool connected, remote, demoplayback;

	bool spectator;

	IVARP(centerchat, 0, 1, 1);
	IVARP(colourchat, 0, 1, 1);

	clientcom(GAMECLIENT &_cl) : cl(_cl), c2sinit(false), senditemstoserver(false), lastping(0), connected(false), remote(false), demoplayback(false), spectator(false)
	{
        CCOMMAND(say, "C", (clientcom *self, char *s), self->toserver(s, SAY_NONE));
        CCOMMAND(me, "C", (clientcom *self, char *s), self->toserver(s, SAY_ACTION));
        CCOMMAND(sayteam, "C", (clientcom *self, char *s), self->toserver(s, SAY_TEAM));
        CCOMMAND(meteam, "C", (clientcom *self, char *s), self->toserver(s, SAY_ACTION|SAY_TEAM));
        CCOMMAND(name, "s", (clientcom *self, char *s), self->switchname(s));
        CCOMMAND(team, "s", (clientcom *self, char *s), self->switchteam(s));
        CCOMMAND(map, "s", (clientcom *self, char *s), self->changemap(s));
        CCOMMAND(clearbans, "", (clientcom *self, char *s), self->clearbans());
        CCOMMAND(kick, "s", (clientcom *self, char *s), self->kick(s));
        CCOMMAND(goto, "s", (clientcom *self, char *s), self->gotoplayer(s));
        CCOMMAND(spectator, "is", (clientcom *self, int *val, char *who), self->togglespectator(*val, who));
        CCOMMAND(mastermode, "i", (clientcom *self, int *val), if(self->remote) self->addmsg(SV_MASTERMODE, "ri", *val));
        CCOMMAND(setmaster, "s", (clientcom *self, char *s), self->setmaster(s));
        CCOMMAND(approve, "s", (clientcom *self, char *s), self->approvemaster(s));
        CCOMMAND(setteam, "ss", (clientcom *self, char *who, char *team), self->setteam(who, team));
        CCOMMAND(getmap, "", (clientcom *self), self->getmap());
        CCOMMAND(sendmap, "", (clientcom *self), self->sendmap());
        CCOMMAND(listdemos, "", (clientcom *self), self->listdemos());
        CCOMMAND(getdemo, "i", (clientcom *self, int *val), self->getdemo(*val));
        CCOMMAND(recorddemo, "i", (clientcom *self, int *val), self->recorddemo(*val));
        CCOMMAND(stopdemo, "", (clientcom *self), self->stopdemo());
        CCOMMAND(cleardemos, "i", (clientcom *self, int *val), self->cleardemos(*val));
		CCOMMAND(ready, "", (clientcom *self), intret(self->ready()));

        extern void result(const char *s);
        CCOMMAND(getname, "", (clientcom *self), result(self->cl.player1->name));
        CCOMMAND(getteam, "", (clientcom *self), result(self->cl.player1->team));
	}

	bool haspriv(int flag, bool quiet = false)
	{
		if (cl.player1->privilege>=flag) return true;
		else if (!quiet)
		{
			const char *_privs[] = { "none", "master", "admin" };
			conoutf("access denied, you need %s", _privs[flag]);
		}
		return false;
	}

	void switchname(const char *name)
	{
		if(name[0])
		{
			c2sinit = false;
			s_strncpy(cl.player1->name, name, MAXNAMELEN);
			//filtertext(cl.player1->name, name, false, MAXNAMELEN);
		}
		else conoutf("your name is: %s", cl.colorname(cl.player1));
	}

	void switchteam(const char *team)
	{
		if(team[0])
		{
			c2sinit = false;
			s_strncpy(cl.player1->team, team, MAXTEAMLEN);
			//filtertext(cl.player1->team, team, false, MAXTEAMLEN);
		}
		else conoutf("your team is: %s", cl.player1->team);
	}

	int numchannels() { return 3; }

	void mapstart() { if(!spectator || cl.player1->privilege) senditemstoserver = true; }

	void writeclientinfo(FILE *f)
	{
		fprintf(f, "name \"%s\"\nteam \"%s\"\n\n", cl.player1->name, cl.player1->team);
	}

	void gameconnect(bool _remote)
	{
		remote = _remote;
		if (editmode) toggleedit();
	}

	void gamedisconnect(int clean)
	{
		remote = connected = c2sinit = spectator = false;
		cl.player1->clientnum = -1;
		cl.player1->lifesequence = 0;
		cl.player1->privilege = PRIV_NONE;
        removetrackedparticles();
		loopv(cl.players) DELETEP(cl.players[i]);
		cleardynentcache();
		if (!clean) lanconnect();
	}

	bool allowedittoggle(bool edit)
	{
		bool allow = !connected || m_edit(cl.gamemode) ||
			(cl.player1->state != CS_SPECTATOR && cl.player1->state != CS_ALIVE && cl.player1->state != CS_EDITING);
		if (!allow) conoutf("you must be both alive and in coopedit to enter editmode");
		return allow;
	}

	void edittoggled(bool edit)
	{
		if (!edit)
		{
			cl.player1->state = CS_ALIVE;
			cl.player1->o.z -= cl.player1->height; // entinmap wants feet pos
		}
		else
		{
			cl.resetgamestate();
			cl.player1->state = CS_EDITING;
		}
		cl.ph.entinmap(cl.player1, false); // find spawn closest to current floating pos
		addmsg(SV_EDITMODE, "ri", edit ? 1 : 0);
	}

	int parseplayer(const char *arg)
	{
		char *end;
		int n = strtol(arg, &end, 10);
		if(!cl.players.inrange(n)) return -1;
		if(*arg && !*end) return n;
		// try case sensitive first
		loopi(cl.numdynents())
		{
			fpsent *o = (fpsent *)cl.iterdynents(i);
			if(o && !strcmp(arg, o->name)) return o->clientnum;
		}
		// nothing found, try case insensitive
		loopi(cl.numdynents())
		{
			fpsent *o = (fpsent *)cl.iterdynents(i);
			if(o && !strcasecmp(arg, o->name)) return o->clientnum;
		}
		return -1;
	}

	void clearbans()
	{
        if(!remote) return;
		addmsg(SV_CLEARBANS, "r");
	}

	void kick(const char *arg)
	{
		if(!remote) return;
		int i = parseplayer(arg);
		if(i>=0 && i!=cl.player1->clientnum) addmsg(SV_KICK, "ri", i);
	}

	void setteam(const char *arg1, const char *arg2)
	{
		if(!remote) return;
		int i = parseplayer(arg1);
		if(i>=0 && i!=cl.player1->clientnum) addmsg(SV_SETTEAM, "ris", i, arg2);
	}

	void setmaster(const char *arg)
	{
		if(!remote || !arg[0]) return;
		int val = 1;
		const char *passwd = "";
		if(!arg[1] && isdigit(arg[0])) val = atoi(arg);
		else passwd = arg;
		addmsg(SV_SETMASTER, "ris", val, passwd);
	}

    void approvemaster(const char *who)
    {
        if(!remote) return;
        int i = parseplayer(who);
        if(i>=0) addmsg(SV_APPROVEMASTER, "ri", i);
    }

    void togglespectator(int val, const char *who)
	{
		if (!who[0]) cl.cameranum = -cl.player1->clientnum;
        int i = who[0] ? parseplayer(who) : cl.player1->clientnum;
		if(i>=0) addmsg(SV_SPECTATOR, "rii", i, val);
	}

	// collect c2s messages conveniently
	vector<uchar> messages;

	void addmsg(int type, const char *fmt = NULL, ...)
	{
		if(remote && spectator && (!cl.player1->privilege || type<SV_MASTERMODE))
		{
			static int spectypes[] = { SV_MAPVOTE, SV_GETMAP, SV_TEXT, SV_SETMASTER };
			bool allowed = false;
			loopi(sizeof(spectypes)/sizeof(spectypes[0])) if(type==spectypes[i])
			{
				allowed = true;
				break;
			}
			if(!allowed) return;
		}
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
				case 's': sendstring(va_arg(args, const char *), p); nums++; break;
			}
			va_end(args);
		}
		int num = nums?0:numi, msgsize = msgsizelookup(type);
        if(msgsize && num!=msgsize) { s_sprintfd(s)("inconsistent msg size for %d (%d != %d)", type, num, msgsize); fatal(s); }
		int len = p.length();
		messages.add(len&0xFF);
		messages.add((len>>8)|(reliable ? 0x80 : 0));
		loopi(len) messages.add(buf[i]);
	}

	void saytext(fpsent *d, char *text, int flags)
	{
		if (!colourchat()) filtertext(text, text);

		string s, t;
		s_sprintf(t)("%s", flags&SAY_TEAM ? "\fb" : "\fr");

		if (flags&SAY_ACTION) s_sprintf(s)("\fs%s*\fS \fs%s%s\fS \fs%s%s\fS", t, t, cl.colorname(d), t, text);
		else s_sprintf(s)("\fs%s<\fS\fs\fw%s\fS\fs%s>\fS \fs\fw%s\fS", t, cl.colorname(d), t, text);

		if(d->state != CS_DEAD && d->state != CS_SPECTATOR)
		{
			s_sprintfd(ds)("@%s", &s);
			particle_text(d->abovehead(), ds, 9);
		}

		console("%s", (centerchat() ? CON_CENTER : 0)|CON_NORMAL, s);
		playsound(S_CHAT);
	}

	void toserver(char *text, int flags)
	{
		saytext(cl.player1, text, flags);
		addmsg(SV_TEXT, "ris", flags, text);
	}

	void toservcmd(char *text, bool msg)
	{
		addmsg(SV_COMMAND, "rs", text);
	}

	int sendpacketclient(ucharbuf &p, bool &reliable, dynent *d)
	{
        if(d->state==CS_ALIVE || d->state==CS_EDITING)
		{
			// send position updates separately so as to not stall out aiming
			ENetPacket *packet = enet_packet_create(NULL, 100, 0);
			ucharbuf q(packet->data, packet->dataLength);
			putint(q, SV_POS);
			putint(q, cl.player1->clientnum);
			putuint(q, (int)(d->o.x*DMF));			  // quantize coordinates to 1/4th of a cube, between 1 and 3 bytes
			putuint(q, (int)(d->o.y*DMF));
			putuint(q, (int)(d->o.z*DMF));
			putuint(q, (int)d->yaw);
			putint(q, (int)d->pitch);
			putint(q, (int)d->roll);
			putint(q, (int)(d->vel.x*DVELF));		  // quantize to itself, almost always 1 byte
			putint(q, (int)(d->vel.y*DVELF));
			putint(q, (int)(d->vel.z*DVELF));
            putuint(q, d->physstate | (d->falling.x || d->falling.y ? 0x20 : 0) | (d->falling.z ? 0x10 : 0) | ((((fpsent *)d)->lifesequence&1)<<6));
            if(d->falling.x || d->falling.y)
            {
                putint(q, (int)(d->falling.x*DVELF));      // quantize to itself, almost always 1 byte
                putint(q, (int)(d->falling.y*DVELF));
            }
            if(d->falling.z) putint(q, (int)(d->falling.z*DVELF));
			// pack rest in almost always 1 byte: strafe:2, move:2, garmour: 1, yarmour: 1, quad: 1
			uint flags = (d->strafe&3) | ((d->move&3)<<2) | ((d->crouching ? 1 : 0)<<3);
			putuint(q, flags);
			enet_packet_resize(packet, q.length());
			sendpackettoserv(packet, 0);
		}
		if(senditemstoserver)
		{
			reliable = true;
			putint(p, SV_ITEMLIST);
			cl.et.putitems(p);
			putint(p, -1);
			if(m_capture(cl.gamemode)) cl.cpc.sendbases(p);
            else if(m_ctf(cl.gamemode)) cl.ctf.sendflags(p);
			senditemstoserver = false;
		}
		if(!c2sinit)	// tell other clients who I am
		{
			reliable = true;
			c2sinit = true;
			putint(p, SV_INITC2S);
			sendstring(cl.player1->name, p);
			sendstring(cl.player1->team, p);
		}
		int i = 0;
		while(i < messages.length()) // send messages collected during the previous frames
		{
			int len = messages[i] | ((messages[i+1]&0x7F)<<8);
			if(p.remaining() < len) break;
			if(messages[i+1]&0x80) reliable = true;
			p.put(&messages[i+2], len);
			i += 2 + len;
		}
		messages.remove(0, i);
		if(!spectator && p.remaining()>=10 && lastmillis-lastping>250)
		{
			putint(p, SV_PING);
			putint(p, lastmillis);
			lastping = lastmillis;
		}
		return 1;
	}

	void updatepos(fpsent *d)
	{
		// update the position of other clients in the game in our world
		// don't care if he's in the scenery or other players,
		// just don't overlap with our client

		const float r = cl.player1->radius+d->radius;
		const float dx = cl.player1->o.x-d->o.x;
		const float dy = cl.player1->o.y-d->o.y;
		const float dz = cl.player1->o.z-d->o.z;
		const float rz = cl.player1->aboveeye+cl.player1->height;
		const float fx = (float)fabs(dx), fy = (float)fabs(dy), fz = (float)fabs(dz);
		if(fx<r && fy<r && fz<rz && cl.player1->state!=CS_SPECTATOR && d->state!=CS_DEAD)
		{
			if(fx<fy) d->o.y += dy<0 ? r-fy : -(r-fy);  // push aside
			else	  d->o.x += dx<0 ? r-fx : -(r-fx);
		}
		int lagtime = lastmillis-d->lastupdate;
		if(lagtime)
		{
            if(d->state!=CS_SPAWNING && d->lastupdate) d->plag = (d->plag*5+lagtime)/6;
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
				int cn = getint(p);
				vec o, vel, falling;
				float yaw, pitch, roll;
				int physstate, f;
				o.x = getuint(p)/DMF;
				o.y = getuint(p)/DMF;
				o.z = getuint(p)/DMF;
				yaw = (float)getuint(p);
				pitch = (float)getint(p);
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
                int seqcolor = (physstate>>6)&1;
				f = getuint(p);
				fpsent *d = cl.getclient(cn);
                if(!d || seqcolor!=(d->lifesequence&1)) continue;
                float oldyaw = d->yaw, oldpitch = d->pitch;
				d->yaw = yaw;
				d->pitch = pitch;
				d->roll = roll;
				d->strafe = (f&3)==3 ? -1 : f&3;
				f >>= 2;
				d->move = (f&3)==3 ? -1 : f&3;
				f >>= 2;
				d->crouching = f&1 ? true : false;
#if 0
				if(f&1) { d->armourtype = A_GREEN; d->armour = 1; }
				else if(f&2) { d->armourtype = A_YELLOW; d->armour = 1; }
				else { d->armourtype = A_BLUE; d->armour = 0; }
				d->quadmillis = f&4 ? 1 : 0;
				f >>= 3;
				d->maxhealth = 100 + f*itemstats[I_BOOST-I_SHELLS].add;
#endif
                vec oldpos(d->o);
                if(cl.allowmove(d))
                {
                    d->o = o;
                    d->vel = vel;
                    d->falling = falling;
                    d->physstate = physstate & 0x0F;
                    cl.ph.updatephysstate(d);
                    updatepos(d);
                }
                if(cl.ph.smoothmove() && d->smoothmillis>=0 && oldpos.dist(d->o) < cl.ph.smoothdist())
                {
                    d->newpos = d->o;
                    d->newyaw = d->yaw;
                    d->newpitch = d->pitch;
                    d->o = oldpos;
                    d->yaw = oldyaw;
                    d->pitch = oldpitch;
                    (d->deltapos = oldpos).sub(d->newpos);
                    d->deltayaw = oldyaw - d->newyaw;
                    if(d->deltayaw > 180) d->deltayaw -= 360;
                    else if(d->deltayaw < -180) d->deltayaw += 360;
                    d->deltapitch = oldpitch - d->newpitch;
                    d->smoothmillis = lastmillis;
                }
                else d->smoothmillis = 0;
                if(d->state==CS_LAGGED || d->state==CS_SPAWNING) d->state = CS_ALIVE;
				break;
			}

			default:
				neterr("type");
				return;
		}
	}

	void parsepacketclient(int chan, ucharbuf &p)	// processes any updates from the server
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

	void parsemessages(int cn, fpsent *d, ucharbuf &p)
	{
		static char text[MAXTRANS];
		int type;
		bool mapchanged = false;

        while(p.remaining()) switch(type = getint(p))
		{
			case SV_INITS2C:					// welcome messsage from the server
			{
				int mycn = getint(p), prot = getint(p), hasmap = getint(p);
				if(prot!=PROTOCOL_VERSION)
				{
					conoutf("you are using a different game protocol (you: %d, server: %d)", PROTOCOL_VERSION, prot);
					disconnect();
					return;
				}
				connected = true;
				cl.player1->clientnum = mycn;	  // we are now fully connected
				switch (hasmap)
				{
					case 0:
						changemap(mapname);
						break;
					case 2:
						senditemstoserver = true;
						break;
					case 1:
					default:
						break;
				}
				break;
			}

			case SV_CLIENT:
			{
				int cn = getint(p), len = getuint(p);
				ucharbuf q = p.subbuf(len);
				parsemessages(cn, cl.getclient(cn), q);
				break;
			}

			case SV_SOUND:
				if(!d) return;
				playsound(getint(p), &d->o);
				break;

			case SV_TEXT:
			{
                int tcn = getint(p);
                fpsent *t = tcn == cl.player1->clientnum ? cl.player1 : cl.getclient(tcn);
				int flags = getint(p);
				getstring(text, p);
                if(!t) break;
				saytext(d, text, flags);
				break;
			}

			case SV_MAPCHANGE:
			{
				getstring(text, p);
				int mode = getint(p), muts = getint(p);
				changemapserv(text, mode, muts);
				mapchanged = true;
				break;
			}

			case SV_ARENAWIN:
			{
				int acn = getint(p);
				fpsent *alive = acn<0 ? NULL : (acn==cl.player1->clientnum ? cl.player1 : cl.getclient(acn));
				console("arena round is over! next round in 5 seconds...", CON_NORMAL|CON_CENTER);
				if(!alive) console("everyone died!", CON_NORMAL|CON_CENTER);
				else if(m_team(cl.gamemode, cl.mutators)) console("team %s has won the round", CON_NORMAL|CON_CENTER, alive->team);
				else if(alive==cl.player1) console("you are the last man standing!", CON_NORMAL|CON_CENTER);
				else console("%s is the last man standing", CON_NORMAL|CON_CENTER, cl.colorname(alive));

				bool win = false;
				if(alive)
				{
					if(m_team(cl.gamemode, cl.mutators) && isteam(cl.player1->team, alive->team)) win = true;
					else if(alive == cl.player1) win = true;
				}
				playsound(win ? S_V_YOUWIN : S_V_YOULOSE);
				break;
			}

			case SV_FORCEDEATH:
			{
				int cn = getint(p);
				fpsent *d = cn==cl.player1->clientnum ? cl.player1 : cl.newclient(cn);
				if(!d) break;
				if(d==cl.player1)
				{
					if(editmode) toggleedit();
					cl.sb.showscores(true);
				}
				d->state = CS_DEAD;
				cl.calcranks();
				break;
			}

			case SV_ITEMLIST:
			{
				int n;
				while((n = getint(p))!=-1)
				{
					if(mapchanged) cl.et.setspawn(n, true);
					getint(p); // type
					loopi(5) getint(p); // attr
				}
				break;
			}

			case SV_MAPRELOAD:		  // server requests next map
			{
                s_sprintfd(nextmapalias)("nextmap_%s%s", m_capture(cl.gamemode) ? "capture_" : (m_ctf(cl.gamemode) ? "ctf_" : ""), mapname);
				const char *map = getalias(nextmapalias);	 // look up map in the cycle
				addmsg(SV_MAPCHANGE, "rsii", *map ? map : sv->defaultmap(), cl.nextmode, cl.nextmuts);
				break;
			}

			case SV_INITC2S:			// another client either connected or changed name/team
			{
				d = cl.newclient(cn);
				if(!d)
				{
					getstring(text, p);
					getstring(text, p);
					break;
				}
				getstring(text, p);
				//filtertext(text, text, false, MAXNAMELEN);
				if(!text[0]) s_strcpy(text, "unnamed");
				if(d->name[0])		  // already connected
				{
					if(strcmp(d->name, text))
					{
						string oldname, newname;
						s_strcpy(oldname, cl.colorname(d));
						s_strcpy(newname, cl.colorname(d, text));
						conoutf("%s is now known as %s", oldname, newname);
					}
				}
				else					// new client
				{
					c2sinit = false;	// send new players my info again
					conoutf("connected: %s", cl.colorname(d, text));
					loopv(cl.players)	// clear copies since new player doesn't have them
						if(cl.players[i]) freeeditinfo(cl.players[i]->edit);
					extern editinfo *localedit;
					freeeditinfo(localedit);
				}
				s_strncpy(d->name, text, MAXNAMELEN+1);
				getstring(text, p);
				//filtertext(d->team, text, false, MAXTEAMLEN);
				cl.calcranks();
				break;
			}

			case SV_CDIS:
				cl.clientdisconnected(getint(p));
				break;

			case SV_SPAWN:
			{
				int ls = getint(p), gunselect = getint(p);
				if(!d) break;
				d->lifesequence = ls;
				d->gunselect = gunselect;
				d->state = CS_SPAWNING;
				playsound(S_RESPAWN, &d->o, 255, 0, true);
				break;
			}

			case SV_SPAWNSTATE:
			{
				if(editmode) toggleedit();
				cl.player1->respawn();
				cl.player1->lifesequence = getint(p);
				cl.player1->health = getint(p);
				cl.player1->gunselect = getint(p);
				loopi(NUMGUNS) cl.player1->ammo[i] = getint(p);
				cl.player1->state = CS_ALIVE;
				cl.et.findplayerspawn(cl.player1, m_capture(cl.gamemode) ? cl.cpc.pickspawn(cl.player1->team) : -1, m_ctf(cl.gamemode) ? ctfteamflag(cl.player1->team)+1 : 0);
				cl.sb.showscores(false);
                cl.lasthit = 0;
				addmsg(SV_SPAWN, "rii", cl.player1->lifesequence, cl.player1->gunselect);
				break;
			}

			case SV_SHOTFX:
			{
				int scn = getint(p), gun = getint(p);
				vec from, to;
				loopk(3) from[k] = getint(p)/DMF;
				loopk(3) to[k] = getint(p)/DMF;
				fpsent *s = cl.getclient(scn);
				if(!s || gun < 0) break;
				if(gun==GUN_SG) cl.ws.createrays(from, to);
				s->gunwait[gun] = 0;
				s->gunlast[gun] = lastmillis;
				s->lastattackgun = gun;
				cl.ws.shootv(gun, from, to, s, false);
				break;
			}

			case SV_DAMAGE:
			{
				int tcn = getint(p),
					acn = getint(p),
					gun = getint(p),
					damage = getint(p),
					health = getint(p),
					millis = getint(p);
				vec dir;
				loopk(3) dir[k] = getint(p)/DNF;
				fpsent *target = tcn==cl.player1->clientnum ? cl.player1 : cl.getclient(tcn),
						*actor = acn==cl.player1->clientnum ? cl.player1 : cl.getclient(acn);
				if(!target || !actor) break;
				cl.damaged(gun, damage, target, actor, millis, dir);
				target->health = health; // just in case
				break;
			}

			case SV_RELOAD:
			{
				int trg = getint(p), gun = getint(p), amt = getint(p);
				fpsent *target = trg == cl.player1->clientnum ? cl.player1 : cl.getclient(trg);
				if (!target || gun <= -1 || gun >= NUMGUNS) break;
				target->ammo[gun] = amt;
				break;
			}

			case SV_REGEN:
			{
				int trg = getint(p), amt = getint(p), ms = getint(p);
				fpsent *target = trg == cl.player1->clientnum ? cl.player1 : cl.getclient(trg);
				if (!target) break;
				playsound(S_REGEN, &target->o);
				target->health = amt;
				target->lastregen = ms;
				break;
			}

			case SV_DIED:
			{
				int vcn = getint(p), acn = getint(p), frags = getint(p);
				fpsent *victim = vcn==cl.player1->clientnum ? cl.player1 : cl.getclient(vcn),
						*actor = acn==cl.player1->clientnum ? cl.player1 : cl.getclient(acn);
				if(!actor) break;
				actor->frags = frags;
				if(actor!=cl.player1 && !m_capture(cl.gamemode) && !m_ctf(cl.gamemode))
				{
					s_sprintfd(ds)("@%d", actor->frags);
					particle_text(actor->abovehead(), ds, 9);
				}
				if(!victim) break;
				cl.killed(victim, actor);
				cl.calcranks();
				break;
			}

			case SV_GUNSELECT:
			{
				if(!d) return;
				int gun = getint(p);
				d->gunselect = gun;
				break;
			}

			case SV_TAUNT:
			{
				if(!d) return;
				d->lasttaunt = lastmillis;
				break;
			}

			case SV_RESUME:
			{
				for(;;)
				{
					int cn = getint(p);
					if(cn<0) break;
					int state = getint(p), lifesequence = getint(p), gunselect = getint(p), frags = getint(p);
					fpsent *d = (cn == cl.player1->clientnum ? cl.player1 : cl.newclient(cn));
					if(!d) continue;
					if(d!=cl.player1)
					{
						d->state = state;
						d->gunselect = gunselect;
					}
					d->lifesequence = lifesequence;
					d->frags = frags;
				}
				break;
			}

			case SV_ITEMSPAWN:
			{
				int i = getint(p);
				if(!cl.et.ents.inrange(i)) break;
				cl.et.setspawn(i, true);
				playsound(S_ITEMSPAWN, &cl.et.ents[i]->o, 255, 0, true);
                const char *name = cl.et.itemname(i);
				if(name) particle_text(cl.et.ents[i]->o, name, 9);
				break;
			}

			case SV_ITEMACC:			// server acknowledges that I picked up this item
			{
				int i = getint(p), cn = getint(p);
				fpsent *d = cn==cl.player1->clientnum ? cl.player1 : cl.getclient(cn);
				cl.et.useeffects(i, d);
				break;
			}

			case SV_EDITVAR:
			{
				if (!d) return;
				getstring(text, p);
				int val = getint(p);
				ident *id = idents->access(text);

				if (id->type == ID_VAR && id->world && id->maxval >= id->minval)
				{
					if (val > id->maxval) val = id->maxval;
					else if (val < id->minval) val = id->minval;

					setvar(text, val, true);

					conoutf("%s updated the value of %s to %d", d->name, id->name, *id->storage.i);
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
				if (!d) return;
				selinfo sel;
				sel.o.x = getint(p); sel.o.y = getint(p); sel.o.z = getint(p);
				sel.s.x = getint(p); sel.s.y = getint(p); sel.s.z = getint(p);
				sel.grid = getint(p); sel.orient = getint(p);
				sel.cx = getint(p); sel.cxs = getint(p); sel.cy = getint(p), sel.cys = getint(p);
				sel.corner = getint(p);
				int dir, mode, tex, newtex, mat, allfaces;
				ivec moveo;
				switch(type)
				{
					case SV_EDITF: dir = getint(p); mode = getint(p); mpeditface(dir, mode, sel, false); break;
					case SV_EDITT: tex = getint(p); allfaces = getint(p); mpedittex(tex, allfaces, sel, false); break;
					case SV_EDITM: mat = getint(p); mpeditmat(mat, sel, false); break;
					case SV_FLIP: mpflip(sel, false); break;
					case SV_COPY: if(d) mpcopy(d->edit, sel, false); break;
					case SV_PASTE: if(d) mppaste(d->edit, sel, false); break;
					case SV_ROTATE: dir = getint(p); mprotate(dir, sel, false); break;
					case SV_REPLACE: tex = getint(p); newtex = getint(p); mpreplacetex(tex, newtex, sel, false); break;
					case SV_DELCUBE: mpdelcube(sel, false); break;
				}
				break;
			}
            case SV_REMIP:
            {
                if(!d) return;
                conoutf("%s remipped", cl.colorname(d));
                mpremip(false);
                break;
            }
			case SV_EDITENT:			// coop edit of ent
			{
				if(!d) return;
				int i = getint(p);
				float x = getint(p)/DMF, y = getint(p)/DMF, z = getint(p)/DMF;
				int type = getint(p);
				int attr1 = getint(p), attr2 = getint(p), attr3 = getint(p), attr4 = getint(p);

				mpeditent(i, vec(x, y, z), type, attr1, attr2, attr3, attr4, false);
				break;
			}

			case SV_PONG:
				addmsg(SV_CLIENTPING, "i", cl.player1->ping = (cl.player1->ping*5+lastmillis-getint(p))/6);
				break;

			case SV_CLIENTPING:
				if(!d) return;
				d->ping = getint(p);
				break;

			case SV_TIMEUP:
				cl.timeupdate(getint(p));
				break;

			case SV_SERVMSG:
				getstring(text, p);
                conoutf("%s", text);
				break;

			case SV_SENDDEMOLIST:
			{
				int demos = getint(p);
				if(!demos) conoutf("no demos available");
				else loopi(demos)
				{
					getstring(text, p);
					conoutf("%d. %s", i+1, text);
				}
				break;
			}

			case SV_DEMOPLAYBACK:
			{
				int on = getint(p);
				if(on) cl.player1->state = CS_SPECTATOR;
				else stopdemo();
                demoplayback = on!=0;
				break;
			}

			case SV_CURRENTMASTER:
			{
				int mn = getint(p), priv = getint(p);
				cl.player1->privilege = PRIV_NONE;
				loopv(cl.players) if(cl.players[i]) cl.players[i]->privilege = PRIV_NONE;
				if(mn>=0)
				{
					fpsent *m = mn==cl.player1->clientnum ? cl.player1 : cl.newclient(mn);
					m->privilege = priv;
				}
				break;
			}

			case SV_EDITMODE:
			{
				int val = getint(p);
				if(!d) break;
				if(val) d->state = CS_EDITING;
				else d->state = CS_ALIVE;
				break;
			}

			case SV_SPECTATOR:
			{
				int sn = getint(p), val = getint(p);
				fpsent *s;
				if(sn==cl.player1->clientnum)
				{
					spectator = val!=0;
					s = cl.player1;
				}
				else s = cl.newclient(sn);
				if(!s) return;
				if(val)
				{
					if(s==cl.player1 && editmode) toggleedit();
					s->state = CS_SPECTATOR;
				}
				else if(s->state==CS_SPECTATOR)
				{
					s->state = CS_DEAD;
					if(s==cl.player1) cl.sb.showscores(true);
				}
				break;
			}

			case SV_SETTEAM:
			{
				int wn = getint(p);
				getstring(text, p);
				fpsent *w = wn==cl.player1->clientnum ? cl.player1 : cl.getclient(wn);
				if(!w) return;
				//filtertext(w->team, text, false, MAXTEAMLEN);
				cl.calcranks();
				break;
			}

			case SV_BASEINFO:
			{
				int base = getint(p);
				string owner, enemy;
				int converted = getint(p);
				getstring(text, p);
				s_strcpy(owner, text);
				getstring(text, p);
				s_strcpy(enemy, text);
				if(m_capture(cl.gamemode)) cl.cpc.updatebase(base, owner, enemy, converted);
				break;
			}

			case SV_BASES:
			{
				int base = 0, converted;
				while((converted = getint(p))>=0)
				{
					string owner, enemy;
					getstring(text, p);
					s_strcpy(owner, text);
					getstring(text, p);
					s_strcpy(enemy, text);
					cl.cpc.initbase(base++, owner, enemy, converted);
				}
				break;
			}

			case SV_TEAMSCORE:
			{
				getstring(text, p);
				int total = getint(p);
				if(m_capture(cl.gamemode)) cl.cpc.setscore(text, total);
				break;
			}

            case SV_INITFLAGS:
            {
                cl.ctf.parseflags(p, m_ctf(cl.gamemode));
                break;
            }

            case SV_DROPFLAG:
            {
                int ocn = getint(p), flag = getint(p);
                vec droploc;
                loopk(3) droploc[k] = getint(p)/DMF;
                fpsent *o = ocn==cl.player1->clientnum ? cl.player1 : cl.newclient(ocn);
                if(m_ctf(cl.gamemode)) cl.ctf.dropflag(o, flag, droploc);
                break;
            }

            case SV_SCOREFLAG:
            {
                int ocn = getint(p), relayflag = getint(p), goalflag = getint(p), score = getint(p);
                fpsent *o = ocn==cl.player1->clientnum ? cl.player1 : cl.newclient(ocn);
                if(m_ctf(cl.gamemode)) cl.ctf.scoreflag(o, relayflag, goalflag, score);
                break;
            }

            case SV_RETURNFLAG:
            {
                int ocn = getint(p), flag = getint(p);
                fpsent *o = ocn==cl.player1->clientnum ? cl.player1 : cl.newclient(ocn);
                if(m_ctf(cl.gamemode)) cl.ctf.returnflag(o, flag);
                break;
            }

            case SV_TAKEFLAG:
            {
                int ocn = getint(p), flag = getint(p);
                fpsent *o = ocn==cl.player1->clientnum ? cl.player1 : cl.newclient(ocn);
                if(m_ctf(cl.gamemode)) cl.ctf.takeflag(o, flag);
                break;
            }

            case SV_RESETFLAG:
            {
                int flag = getint(p);
                if(m_ctf(cl.gamemode)) cl.ctf.resetflag(flag);
                break;
            }

            case SV_CLEARTARGETS:
                if(m_assassin(cl.gamemode)) cl.asc.targets.setsize(0);
                break;

            case SV_CLEARHUNTERS:
                if(m_assassin(cl.gamemode)) cl.asc.hunters.setsize(0);
                break;

            case SV_ADDTARGET:
            {
                int tcn = getint(p);
                if(m_assassin(cl.gamemode))
                {
                    fpsent *t = cl.newclient(tcn);
                    if(cl.asc.targets.find(t)<0) cl.asc.targets.add(t);
                }
                break;
            }

            case SV_REMOVETARGET:
            {
                int tcn = getint(p);
                if(m_assassin(cl.gamemode))
                {
                    fpsent *t = cl.getclient(tcn);
                    if(t) cl.asc.targets.removeobj(t);
                }
                break;
            }

            case SV_ADDHUNTER:
            {
                int hcn = getint(p);
                if(m_assassin(cl.gamemode))
                {
                    fpsent *h = cl.newclient(hcn);
                    if(cl.asc.hunters.find(h)<0) cl.asc.hunters.add(h);
                }
                break;
            }

            case SV_REMOVEHUNTER:
            {
                int hcn = getint(p);
                if(m_assassin(cl.gamemode))
                {
                    fpsent *h = cl.getclient(hcn);
                    if(h) cl.asc.hunters.removeobj(h);
                }
                break;
            }

			case SV_NEWMAP:
			{
				int size = getint(p);
				if(size>=0) emptymap(size, true);
				else enlargemap(true);
				if(d && d!=cl.player1)
				{
					int newsize = 0;
					while(1<<newsize < getworldsize()) newsize++;
					conoutf(size>=0 ? "%s started a new map of size %d" : "%s enlarged the map to size %d", cl.colorname(d), newsize);
				}
				break;
			}

			default:
				neterr("type");
				return;
		}
	}

	void changemapserv(char *name, int gamemode, int mutators)
	{
		if (remote && !m_mp(gamemode)) gamemode = G_DEATHMATCH;
		cl.nextmode = cl.gamemode = gamemode;
		cl.nextmuts = cl.mutators = mutators;
		cl.minremain = -1;
		if(editmode && !allowedittoggle(editmode)) toggleedit();
		if(m_demo(gamemode)) return;
		load_world(name);
		if(m_capture(gamemode)) cl.cpc.setupbases();
        else if(m_assassin(gamemode)) cl.asc.reset();
        else if(m_ctf(gamemode)) cl.ctf.setupflags();
		if(editmode) edittoggled(editmode);
	}

	void changemap(const char *name) // request map change, server may ignore
	{
        if(spectator && !cl.player1->privilege) return;
        int nextmode = cl.nextmode, nextmuts = cl.nextmuts; // in case stopdemo clobbers these
        if(!remote) stopdemo();
        addmsg(SV_MAPVOTE, "rsii", name, nextmode, nextmuts);
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
				s_sprintfd(fname)("%d.dmo", lastmillis);
				FILE *demo = openfile(fname, "wb");
				if(!demo) return;
				conoutf("received demo \"%s\"", fname);
				fwrite(data, 1, len, demo);
				fclose(demo);
				break;
			}

			case SV_SENDMAP:
			{
				if(!m_edit(cl.gamemode)) return;
				string oldname;
				s_strcpy(oldname, mapname);
				const char *aname = makefile(mapname, ".bgz", false, true);
				s_sprintfd(mname)("%s", aname);
				s_sprintfd(fname)("%s.bgz", mname);
				const char *file = findfile(fname, "wb");
				FILE *map = fopen(file, "wb");
				if(!map) return;
				conoutf("received map");
				fwrite(data, 1, len, map);
				fclose(map);
				load_world(mname, oldname[0] ? oldname : NULL);
				break;
			}
		}
	}

	void getmap()
	{
		//if(!m_edit(cl.gamemode)) { conoutf("\"getmap\" only works in coopedit mode"); return; }
		conoutf("getting map...");
		addmsg(SV_GETMAP, "r");
	}

	void stopdemo()
	{
		if(remote)
		{
			if(!haspriv(PRIV_ADMIN)) return;
			addmsg(SV_STOPDEMO, "r");
		}
		else
		{
			loopv(cl.players) if(cl.players[i]) cl.clientdisconnected(i);

			extern igameserver *sv;
			((GAMESERVER *)sv)->enddemoplayback();
		}
	}

    void recorddemo(int val)
	{
		if(!haspriv(PRIV_ADMIN)) return;
        addmsg(SV_RECORDDEMO, "ri", val);
	}

	void cleardemos(int val)
	{
		if(!haspriv(PRIV_ADMIN)) return;
        addmsg(SV_CLEARDEMOS, "ri", val);
	}

    void getdemo(int i)
	{
		if(i<=0) conoutf("getting demo...");
		else conoutf("getting demo %d...", i);
		addmsg(SV_GETDEMO, "ri", i);
	}

	void listdemos()
	{
		conoutf("listing demos...");
		addmsg(SV_LISTDEMOS, "r");
	}

	void sendmap()
	{
		if(!m_edit(cl.gamemode) || (spectator && !cl.player1->privilege)) { conoutf("\"sendmap\" only works in coopedit mode"); return; }
		conoutf("sending map...");
		s_sprintfd(mname)("sendmap_%d", lastmillis);
		save_world(mname, true);
		s_sprintfd(fname)("maps/%s.ogz", mname);
		const char *file = findfile(fname, "rb");
		FILE *map = fopen(file, "rb");
		if(map)
		{
			fseek(map, 0, SEEK_END);
			if(ftell(map) > 1024*1024) conoutf("map is too large");
			else sendfile(-1, 2, map);
			fclose(map);
		}
		else conoutf("could not read map");
		remove(file);
	}

	void gotoplayer(const char *arg)
	{
		if(cl.player1->state!=CS_SPECTATOR && cl.player1->state!=CS_EDITING) return;
		int i = parseplayer(arg);
		if(i>=0 && i!=cl.player1->clientnum)
		{
			fpsent *d = cl.getclient(i);
			if(!d) return;
			cl.player1->o = d->o;
			vec dir;
			vecfromyawpitch(cl.player1->yaw, cl.player1->pitch, 1, 0, dir);
			cl.player1->o.add(dir.mul(-32));
		}
	}

	bool ready() { return connected; }
	int otherclients() { return cl.players.length(); }

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

	const char *serverinfogui(g3d_gui *cgui, vector<serverinfo *> &servers)
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
				serverinfo &si = *servers[j];
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
								s_sprintf(text)("%s", sv->gamename(si.attr[1], si.attr[2]));
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
} cc;
