struct clientcom : iclientcom
{
	fpsclient &cl;

	bool c2sinit;		// whether we need to tell the other clients our stats

	bool senditemstoserver;	 // after a map change, since server doesn't have map data
	int lastping;

    bool connected, remote, demoplayback;

	bool spectator;

	fpsent *player1;
	
#ifdef BFRONTIER // server version, centerprint chat
	IVARP(centerchat, 0, 1, 1);
	IVARP(colourchat, 0, 1, 1);
#endif
    clientcom(fpsclient &_cl) : cl(_cl), c2sinit(false), senditemstoserver(false), lastping(0), connected(false), remote(false), demoplayback(false), spectator(false), player1(_cl.player1)
	{
#ifdef BFRONTIER
        CCOMMAND(say, "C", (clientcom *self, char *s), self->toserver(s, false));
        CCOMMAND(me, "C", (clientcom *self, char *s), self->toserver(s, true));
#else
        CCOMMAND(say, "C", (clientcom *self, char *s), self->toserver(s));
#endif
        CCOMMAND(name, "s", (clientcom *self, char *s), self->switchname(s));
        CCOMMAND(team, "s", (clientcom *self, char *s), self->switchteam(s));
        CCOMMAND(map, "s", (clientcom *self, char *s), self->changemap(s));
        CCOMMAND(clearbans, "", (clientcom *self, char *s), self->clearbans());
        CCOMMAND(kick, "s", (clientcom *self, char *s), self->kick(s));
        CCOMMAND(goto, "s", (clientcom *self, char *s), self->gotoplayer(s));
        CCOMMAND(spectator, "is", (clientcom *self, int *val, char *who), self->togglespectator(*val, who));
        CCOMMAND(mastermode, "i", (clientcom *self, int *val), if(self->remote) self->addmsg(SV_MASTERMODE, "ri", *val));
        CCOMMAND(setmaster, "s", (clientcom *self, char *s), self->setmaster(s));
        CCOMMAND(approvemaster, "s", (clientcom *self, char *s), self->approvemaster(s));
        CCOMMAND(setteam, "ss", (clientcom *self, char *who, char *team), self->setteam(who, team));
        CCOMMAND(getmap, "", (clientcom *self), self->getmap());
        CCOMMAND(sendmap, "", (clientcom *self), self->sendmap());
        CCOMMAND(listdemos, "", (clientcom *self), self->listdemos());
        CCOMMAND(getdemo, "i", (clientcom *self, int *val), self->getdemo(*val));
        CCOMMAND(recorddemo, "i", (clientcom *self, int *val), self->recorddemo(*val));
        CCOMMAND(stopdemo, "", (clientcom *self), self->stopdemo());
        CCOMMAND(cleardemos, "i", (clientcom *self, int *val), self->cleardemos(*val));

        extern void result(const char *s);
        CCOMMAND(getname, "", (clientcom *self), result(self->player1->name));
        CCOMMAND(getteam, "", (clientcom *self), result(self->player1->team));
	}

#ifdef BFRONTIER // priv checker
	bool haspriv(int flag, bool quiet = false)
	{
		if (player1->privilege>=flag) return true;
		else if (!quiet)
		{
			char *_privs[] = { "none", "master", "admin" };
			conoutf("access denied, you need %s", _privs[flag]);
		}
		return false;
	}
#endif
	void switchname(const char *name)
	{
		if(name[0]) 
		{ 
			c2sinit = false; 
			filtertext(player1->name, name, false, MAXNAMELEN);
		}
		else conoutf("your name is: %s", cl.colorname(player1));
	}

	void switchteam(const char *team)
	{
		if(team[0]) 
		{ 
			c2sinit = false; 
			filtertext(player1->team, team, false, MAXTEAMLEN);
		}
		else conoutf("your team is: %s", player1->team);
	}

	int numchannels() { return 3; }

	void mapstart() { if(!spectator || player1->privilege) senditemstoserver = true; }

	void initclientnet()
	{
	}

	void writeclientinfo(FILE *f)
	{
		fprintf(f, "name \"%s\"\nteam \"%s\"\n\n", player1->name, player1->team);
	}

	void gameconnect(bool _remote)
	{
#ifdef BFRONTIER // local server support
		connected = _remote;
#else
		connected = true;
#endif
		remote = _remote;
		if(editmode) toggleedit();
	}

	void gamedisconnect()
	{
#ifndef BFRONTIER // server version, alternate follow method
        if(remote) cl.stopfollowing();
#endif
		connected = false;
		player1->clientnum = -1;
		c2sinit = false;
		player1->lifesequence = 0;
		player1->privilege = PRIV_NONE;
		spectator = false;
        removetrackedparticles();
		loopv(cl.players) DELETEP(cl.players[i]);
		cleardynentcache();
	}

	bool allowedittoggle()
	{
#ifdef BFRONTIER
		bool allow = !connected || !remote || m_edit(cl.gamemode);
#else
		bool allow = !connected || !remote || cl.gamemode==1;
#endif
		if(!allow) conoutf("editing in multiplayer requires coopedit mode (1)");
		if(allow && spectator) return false;
		return allow;
	}

	void edittoggled(bool on)
	{
		addmsg(SV_EDITMODE, "ri", on ? 1 : 0);
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
		if(i>=0 && i!=player1->clientnum) addmsg(SV_KICK, "ri", i);
	}

	void setteam(const char *arg1, const char *arg2)
	{
		if(!remote) return;
		int i = parseplayer(arg1);
		if(i>=0 && i!=player1->clientnum) addmsg(SV_SETTEAM, "ris", i, arg2);
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
#ifdef BFRONTIER // alternate follow method
		if (!who[0]) cl.cameranum = -player1->clientnum;
#else
		if(!remote) return;
#endif
        int i = who[0] ? parseplayer(who) : player1->clientnum;
		if(i>=0) addmsg(SV_SPECTATOR, "rii", i, val);
	}

	// collect c2s messages conveniently
	vector<uchar> messages;

	void addmsg(int type, const char *fmt = NULL, ...)
	{
		if(remote && spectator && (!player1->privilege || type<SV_MASTERMODE))
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

#ifdef BFRONTIER // centerprint chat, sounds, server commands
	void saytext(fpsent *d, char *text, bool action)
	{
		string s;
		if (action) s_sprintf(s)("\fs\f5*\fr \fs\f5%s\fr \fs\f5%s\fr", cl.colorname(d), text);
		else s_sprintf(s)("\fs\f0<\fr\fs\f0%s\fr\fs\f0>\fr \fs\f0%s\fr", cl.colorname(d), text);
		console("%s", (centerchat() ? CON_CENTER : 0)|CON_LEFT, s);
		playsound(S_CHAT);
	}

	void toserver(char *text, bool action)
	{
		saytext(player1, text, action);
		addmsg(SV_TEXT, "rsi", text, action ? 1 : 0);
	}
	
	void toservcmd(char *text, bool msg)
	{
		addmsg(SV_COMMAND, "rs", text);
	}
#else
    void toserver(char *text) { conoutf("%s:\f0 %s", cl.colorname(player1), text); addmsg(SV_TEXT, "rs", text); }
#endif

	int sendpacketclient(ucharbuf &p, bool &reliable, dynent *d)
	{
        if(d->state==CS_ALIVE || d->state==CS_EDITING)
		{
			// send position updates separately so as to not stall out aiming
			ENetPacket *packet = enet_packet_create(NULL, 100, 0);
			ucharbuf q(packet->data, packet->dataLength);
			putint(q, SV_POS);
			putint(q, player1->clientnum);
			putuint(q, (int)(d->o.x*DMF));			  // quantize coordinates to 1/4th of a cube, between 1 and 3 bytes
			putuint(q, (int)(d->o.y*DMF));
			putuint(q, (int)(d->o.z*DMF));
			putuint(q, (int)d->yaw);
			putint(q, (int)d->pitch);
			putint(q, (int)d->roll);
			putint(q, (int)(d->vel.x*DVELF));		  // quantize to itself, almost always 1 byte
			putint(q, (int)(d->vel.y*DVELF));
			putint(q, (int)(d->vel.z*DVELF));
            putuint(q, d->physstate | (d->gravity.x || d->gravity.y ? 0x20 : 0) | (d->gravity.z ? 0x10 : 0) | ((((fpsent *)d)->lifesequence&1)<<6));
			if(d->gravity.x || d->gravity.y)
			{
				putint(q, (int)(d->gravity.x*DVELF));	  // quantize to itself, almost always 1 byte
				putint(q, (int)(d->gravity.y*DVELF));
			}
			if(d->gravity.z) putint(q, (int)(d->gravity.z*DVELF));
			// pack rest in almost always 1 byte: strafe:2, move:2, garmour: 1, yarmour: 1, quad: 1
			uint flags = (d->strafe&3) | ((d->move&3)<<2);
			putuint(q, flags);
			enet_packet_resize(packet, q.length());
			sendpackettoserv(packet, 0);
		}
		if(senditemstoserver)
		{
			reliable = true;
			putint(p, SV_ITEMLIST);
#ifdef BFRONTIER
			cl.et.putitems(p);
			putint(p, -1);
			if(m_capture(cl.gamemode)) cl.cpc.sendbases(p);
#else
			int gamemode = cl.gamemode;
			if(!m_noitems) cl.et.putitems(p, gamemode);
			putint(p, -1);
			if(m_capture) cl.cpc.sendbases(p);
#endif
			senditemstoserver = false;
		}
		if(!c2sinit)	// tell other clients who I am
		{
			reliable = true;
			c2sinit = true;
			putint(p, SV_INITC2S);
			sendstring(player1->name, p);
			sendstring(player1->team, p);
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
		if(!spectator && p.remaining()>=10 && cl.lastmillis-lastping>250)
		{
			putint(p, SV_PING);
			putint(p, cl.lastmillis);
			lastping = cl.lastmillis;
		}
		return 1;
	}

	void updatepos(fpsent *d)
	{
		// update the position of other clients in the game in our world
		// don't care if he's in the scenery or other players,
		// just don't overlap with our client

		const float r = player1->radius+d->radius;
		const float dx = player1->o.x-d->o.x;
		const float dy = player1->o.y-d->o.y;
		const float dz = player1->o.z-d->o.z;
		const float rz = player1->aboveeye+d->eyeheight;
		const float fx = (float)fabs(dx), fy = (float)fabs(dy), fz = (float)fabs(dz);
		if(fx<r && fy<r && fz<rz && player1->state!=CS_SPECTATOR && d->state!=CS_DEAD)
		{
			if(fx<fy) d->o.y += dy<0 ? r-fy : -(r-fy);  // push aside
			else	  d->o.x += dx<0 ? r-fx : -(r-fx);
		}
		int lagtime = cl.lastmillis-d->lastupdate;
		if(lagtime)
		{
            if(d->state!=CS_SPAWNING && d->lastupdate) d->plag = (d->plag*5+lagtime)/6;
			d->lastupdate = cl.lastmillis;
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
				vec o, vel, gravity;
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
				gravity = vec(0, 0, 0);
				if(physstate&0x20)
				{
					gravity.x = getint(p)/DVELF;
					gravity.y = getint(p)/DVELF;
				}
				if(physstate&0x10) gravity.z = getint(p)/DVELF;
                int seqcolor = (physstate>>6)&1;
				f = getuint(p);
				fpsent *d = cl.getclient(cn);
                if(!d || seqcolor!=(d->lifesequence&1)) continue;
				d->yaw = yaw;
				d->pitch = pitch;
				d->roll = roll;
				d->strafe = (f&3)==3 ? -1 : f&3;
				f >>= 2;
				d->move = (f&3)==3 ? -1 : f&3;
				f >>= 2;
#ifdef BFRONTIER // blood frontier game
				f >>= 3;
#else
				if(f&1) { d->armourtype = A_GREEN; d->armour = 1; }
				else if(f&2) { d->armourtype = A_YELLOW; d->armour = 1; }
				else { d->armourtype = A_BLUE; d->armour = 0; }
				d->quadmillis = f&4 ? 1 : 0;
				f >>= 3;
				d->maxhealth = 100 + f*itemstats[I_BOOST-I_SHELLS].add;
#endif
                if(cl.allowmove(d))
                {
                    d->o = o;
                    d->vel = vel;
                    d->physstate = physstate & 0x0F;
                    d->gravity = gravity;
                    updatephysstate(d);
                    updatepos(d);
                }
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
#ifndef BFRONTIER
		int gamemode = cl.gamemode;
#endif
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
				player1->clientnum = mycn;	  // we are now fully connected
#ifdef BFRONTIER // blood frontier extension, local servers
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
#else
				if(!hasmap && (cl.gamemode==1 || cl.getclientmap()[0])) changemap(cl.getclientmap()); // we are the first client on this server, set map
#endif
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
#ifdef BFRONTIER
				playsound(getint(p), &d->o, &d->vel);
#else
				playsound(getint(p), &d->o);
#endif
				break;

			case SV_TEXT:
			{
				if(!d) return;
				getstring(text, p);
				if (!colourchat()) filtertext(text, text);
				if(d->state!=CS_DEAD && d->state!=CS_SPECTATOR) 
				{
					s_sprintfd(ds)("@%s", &text);
					particle_text(d->abovehead(), ds, 9);
				}
#ifdef BFRONTIER // centerprint chat
				int action = getint(p);
				saytext(d, text, action!=0);
#else
                conoutf("%s:\f0 %s", cl.colorname(d), &text);
#endif
				break;
			}

#ifdef BFRONTIER
			case SV_MAPCHANGE:
			{
				getstring(text, p);
				int mode = getint(p), muts = getint(p);
				changemapserv(text, mode, muts);
				mapchanged = true;
				break;
			}
#else
			case SV_MAPCHANGE:
				getstring(text, p);
				changemapserv(text, getint(p));
				mapchanged = true;
				break;
#endif

			case SV_ARENAWIN:
			{
				int acn = getint(p);
				fpsent *alive = acn<0 ? NULL : (acn==player1->clientnum ? player1 : cl.getclient(acn));
#ifdef BFRONTIER // extended console support
				console("arena round is over! next round in 5 seconds...", CON_LEFT|CON_CENTER);
				if(!alive) console("everyone died!", CON_LEFT|CON_CENTER);
				else if(m_team(cl.gamemode, cl.mutators)) console("team %s has won the round", CON_LEFT|CON_CENTER, alive->team);
				else if(alive==player1) console("you are the last man standing!", CON_LEFT|CON_CENTER);
				else console("%s is the last man standing", CON_LEFT|CON_CENTER, cl.colorname(alive));

				bool win = false;
				if(alive)
				{
					if(m_team(cl.gamemode, cl.mutators) && isteam(cl.player1->team, alive->team)) win = true;
					else if(alive == cl.player1) win = true;
				}
				playsound(win ? S_V_YOUWIN : S_V_YOULOSE);
#else
                conoutf("arena round is over! next round in 5 seconds...");
                if(!alive) conoutf("everyone died!");
                else if(m_teammode) conoutf("team %s has won the round", alive->team);
                else if(alive==player1) conoutf("you are the last man standing!");
                else conoutf("%s is the last man standing", cl.colorname(alive));
#endif
				break;
			}

			case SV_FORCEDEATH:
			{
				int cn = getint(p);
				fpsent *d = cn==player1->clientnum ? player1 : cl.newclient(cn);
				if(!d) break;
				if(d==player1)
				{
					if(editmode) toggleedit();
					cl.sb.showscores(true);
				}
				d->state = CS_DEAD;
#ifdef BFRONTIER // rank calculator
				cl.calcranks();
#endif
				break;
			}

			case SV_ITEMLIST:
			{
				int n;
#ifndef BFRONTIER // local servers, blood frontier support
				if(mapchanged) { senditemstoserver = false; cl.et.resetspawns(); }
#endif
				while((n = getint(p))!=-1)
				{
					if(mapchanged) cl.et.setspawn(n, true);
					getint(p); // type
#ifdef BFRONTIER
					loopi(5) getint(p); // attr
#endif
				}
				break;
			}

			case SV_MAPRELOAD:		  // server requests next map
			{
#ifdef BFRONTIER
				s_sprintfd(nextmapalias)("nextmap_%s%s", m_capture(cl.gamemode) ? "capture_" : "", mapname);
				const char *map = getalias(nextmapalias);	 // look up map in the cycle
				addmsg(SV_MAPCHANGE, "rsii", *map ? map : sv->defaultmap(), cl.nextmode, cl.nextmuts);
#else
				s_sprintfd(nextmapalias)("nextmap_%s%s", m_capture ? "capture_" : "", cl.getclientmap());
				const char *map = getalias(nextmapalias);	 // look up map in the cycle
				addmsg(SV_MAPCHANGE, "rsi", *map ? map : cl.getclientmap(), cl.nextmode);
#endif
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
				filtertext(text, text, false, MAXNAMELEN);
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
				filtertext(d->team, text, false, MAXTEAMLEN);
#ifdef BFRONTIER // rank calculator
				cl.calcranks();
#endif
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
#ifdef BFRONTIER // respawn sound
				playsound(S_RESPAWN, &d->o, &d->vel);
#endif
				break;
			}

			case SV_SPAWNSTATE:
			{
				if(editmode) toggleedit();
				player1->respawn();
				player1->lifesequence = getint(p); 
				player1->health = getint(p);
#ifdef BFRONTIER
				player1->gunselect = getint(p);
				loopi(NUMGUNS) player1->ammo[i] = getint(p);
				player1->state = CS_ALIVE;
				findplayerspawn(player1, m_capture(cl.gamemode) ? cl.cpc.pickspawn(player1->team) : -1);
#else
				player1->maxhealth = getint(p);
				player1->armour = getint(p);
				player1->armourtype = getint(p);
				player1->gunselect = getint(p);
				loopi(GUN_PISTOL-GUN_SG+1) player1->ammo[GUN_SG+i] = getint(p);
				player1->state = CS_ALIVE;
				findplayerspawn(player1, m_capture ? cl.cpc.pickspawn(player1->team) : -1);
#endif
				cl.sb.showscores(false);
#ifndef BFRONTIER // extended console
                if(m_arena) conoutf("new round starting... fight!");
#endif
				addmsg(SV_SPAWN, "rii", player1->lifesequence, player1->gunselect);
				break;
			}

			case SV_SHOTFX:
			{
				int scn = getint(p), gun = getint(p);
				vec from, to;
				loopk(3) from[k] = getint(p)/DMF;
				loopk(3) to[k] = getint(p)/DMF;
				fpsent *s = cl.getclient(scn);
				if(!s) break;
				if(gun==GUN_SG) cl.ws.createrays(from, to);
				s->gunselect = max(gun, 0);
#ifdef BFRONTIER // extended gun waiting
				gunvar(s->gunwait, s->gunselect) = 0;
				gunvar(s->gunlast, s->gunselect) = cl.lastmillis;
#else
				s->gunwait = 0;
				s->lastaction = cl.lastmillis;
#endif
				s->lastattackgun = s->gunselect;
				cl.ws.shootv(gun, from, to, s, false);
				break;
			}

			case SV_DAMAGE:
			{
				int tcn = getint(p), 
					acn = getint(p),
					damage = getint(p), 
#ifdef BFRONTIER
					health = getint(p),
					millis = getint(p);
#else
					armour = getint(p),
					health = getint(p);
#endif
				fpsent *target = tcn==player1->clientnum ? player1 : cl.getclient(tcn),
						*actor = acn==player1->clientnum ? player1 : cl.getclient(acn);
				if(!target || !actor) break;
#ifdef BFRONTIER // local servers
				target->health = health;
				cl.damaged(damage, target, actor, millis);
#else
				target->armour = armour;
				target->health = health;
				cl.damaged(damage, target, actor, false);
#endif
				break;
			}
#ifdef BFRONTIER
			case SV_RELOAD:
			{
				int trg = getint(p), gun = getint(p), amt = getint(p);
				fpsent *target = trg == player1->clientnum ? player1 : cl.getclient(trg);
				if (!target || gun <= -1 || gun >= NUMGUNS) break;
				target->ammo[gun] = amt;
				break;
			}

			case SV_REGENERATE:
			{
				int trg = getint(p), amt = getint(p);
				fpsent *target = trg == player1->clientnum ? player1 : cl.getclient(trg);
				if (!target) break;
				target->health = amt;
				break;
			}
#endif

			case SV_HITPUSH:
			{
				int gun = getint(p), damage = getint(p);
				vec dir;
				loopk(3) dir[k] = getint(p)/DNF;
				player1->hitpush(damage, dir, NULL, gun);
				break;
			}

			case SV_DIED:
			{
				int vcn = getint(p), acn = getint(p), frags = getint(p);
				fpsent *victim = vcn==player1->clientnum ? player1 : cl.getclient(vcn),
						*actor = acn==player1->clientnum ? player1 : cl.getclient(acn);
				if(!actor) break;
				actor->frags = frags;
				if(actor!=player1)
				{
					s_sprintfd(ds)("@%d", actor->frags);
					particle_text(actor->abovehead(), ds, 9);
				}
				if(!victim) break;
				cl.killed(victim, actor);
#ifdef BFRONTIER // rank calculator
				cl.calcranks();
#endif
				break;
			}

			case SV_GUNSELECT:
			{
				if(!d) return;
				int gun = getint(p);
				d->gunselect = max(gun, 0);
#ifdef BFRONTIER
				playsound(S_WEAPLOAD, &d->o, &d->vel);
#else
				playsound(S_WEAPLOAD, &d->o);
#endif
				break;
			}

			case SV_TAUNT:
			{
				if(!d) return;
				d->lasttaunt = cl.lastmillis;
				break;
			}

			case SV_RESUME:
			{
				for(;;)
				{
					int cn = getint(p);
					if(cn<0) break;
#ifdef BFRONTIER
					int state = getint(p), lifesequence = getint(p), gunselect = getint(p), frags = getint(p);
#else
					int state = getint(p), lifesequence = getint(p), gunselect = getint(p), maxhealth = getint(p), frags = getint(p);
#endif
					fpsent *d = (cn == player1->clientnum ? player1 : cl.newclient(cn));
					if(!d) continue;
					if(d!=player1) 
					{
						d->state = state;
						d->gunselect = gunselect;
					}
					d->lifesequence = lifesequence;
#ifndef BFRONTIER
					if(d->state==CS_ALIVE && d->health==d->maxhealth) d->health = maxhealth;
					d->maxhealth = maxhealth;
#endif
					d->frags = frags;
				}
				break;
			}

			case SV_ITEMSPAWN:
			{
				int i = getint(p);
				if(!cl.et.ents.inrange(i)) break;
				cl.et.setspawn(i, true);
#ifdef BFRONTIER
				playsound(S_ITEMSPAWN, &cl.et.ents[i]->o);
#else
				playsound(S_ITEMSPAWN, &cl.et.ents[i]->o);
#endif
				char *name = cl.et.itemname(i);
				if(name) particle_text(cl.et.ents[i]->o, name, 9);
				break;
			}

			case SV_ITEMACC:			// server acknowledges that I picked up this item
			{
				int i = getint(p), cn = getint(p);
				fpsent *d = cn==player1->clientnum ? player1 : cl.getclient(cn);
				cl.et.pickupeffects(i, d);
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
				addmsg(SV_CLIENTPING, "i", player1->ping = (player1->ping*5+cl.lastmillis-getint(p))/6);
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

//#ifdef BFRONTIER
//			case SV_COMMAND:
//				getstring(text, p);
//				getservcmd(text);
//				break;
//#endif
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
				if(on) player1->state = CS_SPECTATOR;
				else stopdemo();
                demoplayback = on!=0;
				break;
			}

			case SV_CURRENTMASTER:
			{
				int mn = getint(p), priv = getint(p);
				player1->privilege = PRIV_NONE;
				loopv(cl.players) if(cl.players[i]) cl.players[i]->privilege = PRIV_NONE;
				if(mn>=0)
				{
					fpsent *m = mn==player1->clientnum ? player1 : cl.newclient(mn);
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
				if(sn==player1->clientnum)
				{
					spectator = val!=0;
					s = player1;
				}
				else s = cl.newclient(sn);
				if(!s) return;
				if(val)
				{
					if(s==player1 && editmode) toggleedit();
					s->state = CS_SPECTATOR;
				}
				else if(s->state==CS_SPECTATOR) 
				{
					s->state = CS_DEAD;
					if(s==player1) cl.sb.showscores(true);
				}
				break;
			}

			case SV_SETTEAM:
			{
				int wn = getint(p);
				getstring(text, p);
				fpsent *w = wn==player1->clientnum ? player1 : cl.getclient(wn);
				if(!w) return;
				filtertext(w->team, text, false, MAXTEAMLEN);
#ifdef BFRONTIER // rank calculator
				cl.calcranks();
#endif
				break;
			}

			case SV_BASEINFO:
			{
				int base = getint(p);
				string owner, enemy;
#ifdef BFRONTIER
				int converted = getint(p);
				getstring(text, p);
				s_strcpy(owner, text);
				getstring(text, p);
				s_strcpy(enemy, text);
				if(m_capture(cl.gamemode)) cl.cpc.updatebase(base, owner, enemy, converted);
#else
				getstring(text, p);
				s_strcpy(owner, text);
				getstring(text, p);
				s_strcpy(enemy, text);
				int converted = getint(p), ammo = getint(p);
				int gamemode = cl.gamemode;
				if(m_capture) cl.cpc.updatebase(base, owner, enemy, converted, ammo);
#endif
				break;
			}

			case SV_BASES:
			{
#ifdef BFRONTIER
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
#else
				int base = 0, ammotype;
				while((ammotype = getint(p))>=0)
				{
					string owner, enemy;
					getstring(text, p);
					s_strcpy(owner, text);
					getstring(text, p);
					s_strcpy(enemy, text);
					int converted = getint(p), ammo = getint(p);
					cl.cpc.initbase(base++, ammotype, owner, enemy, converted, ammo);
				}
#endif
				break;
			}

			case SV_TEAMSCORE:
			{
				getstring(text, p);
#ifdef BFRONTIER
				int total = getint(p);
				if(m_capture(cl.gamemode)) cl.cpc.setscore(text, total);
#else
				int total = getint(p), gamemode = cl.gamemode;
				if(m_capture) cl.cpc.setscore(text, total);
#endif
				break;
			}

#ifndef BFRONTIER
			case SV_REPAMMO:
			{
				int ammotype = getint(p);
				int gamemode = cl.gamemode;
				if(m_capture) cl.cpc.receiveammo(ammotype);
				break;
			}

			case SV_ANNOUNCE:
			{
				int t = getint(p);
				if	 (t==I_QUAD)  { playsound(S_V_QUAD10);  conoutf("\f2quad damage will spawn in 10 seconds!"); }
				else if(t==I_BOOST) { playsound(S_V_BOOST10); conoutf("\f2+10 health will spawn in 10 seconds!"); }
				break;
			}
#endif

			case SV_NEWMAP:
			{
				int size = getint(p);
				if(size>=0) emptymap(size, true);
				else enlargemap(true);
				if(d && d!=player1)
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

#ifdef BFRONTIER
	void changemapserv(const char *name, int gamemode, int mutators)
	{
		if (remote && !connected) connected = true;
		if (remote && !m_mp(gamemode)) gamemode = G_DEATHMATCH;
		cl.nextmode = cl.gamemode = gamemode;
		cl.nextmuts = cl.mutators = mutators;
		cl.minremain = -1;
		if(editmode && !allowedittoggle()) toggleedit();
		if(m_demo(cl.gamemode)) return;
		load_world(name);
		if(m_capture(gamemode)) cl.cpc.setupbases();
		if(editmode) edittoggled(editmode);
	}
#else
	void changemapserv(const char *name, int gamemode)		// forced map change from the server
	{
		if(remote && !m_mp(gamemode)) gamemode = 0;
		cl.gamemode = gamemode;
		cl.nextmode = gamemode;
		cl.minremain = -1;
		if(editmode && !allowedittoggle()) toggleedit();
		if(m_demo) return;
		if(gamemode==1 && !name[0]) emptymap(0, true);
		else load_world(name);
		if(m_capture) cl.cpc.setupbases();
		if(editmode) edittoggled(editmode);
	}
#endif

	void changemap(const char *name) // request map change, server may ignore
	{
#ifdef BFRONTIER
		if(!spectator || player1->privilege) addmsg(SV_MAPVOTE, "rsii", name, cl.nextmode, cl.nextmuts);
#else
		if(!spectator || player1->privilege) addmsg(SV_MAPVOTE, "rsi", name, cl.nextmode);
#endif
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
				s_sprintfd(fname)("%d.dmo", cl.lastmillis);
				FILE *demo = openfile(fname, "wb");
				if(!demo) return;
				conoutf("received demo \"%s\"", fname);
				fwrite(data, 1, len, demo);
				fclose(demo);
				break;
			}

			case SV_SENDMAP:
			{
#ifdef BFRONTIER
				if(!m_edit(cl.gamemode)) return;
				string oldname;
				s_strcpy(oldname, mapname);
				s_sprintfd(mname)("%s", makefile(mapname, "packages/", ".bgz", false, true));
				s_sprintfd(fname)("packages/%s.bgz", mname);
				const char *file = findfile(fname, "wb");
				FILE *map = fopen(file, "wb");
				if(!map) return;
				conoutf("received map");
				fwrite(data, 1, len, map);
				fclose(map);
				load_world(mname, oldname[0] ? oldname : NULL);
#else
				if(cl.gamemode!=1) return;
				string oldname;
				s_strcpy(oldname, cl.getclientmap());
				s_sprintfd(mname)("getmap_%d", cl.lastmillis);
				s_sprintfd(fname)("packages/base/%s.ogz", mname);
				const char *file = findfile(fname, "wb");
				FILE *map = fopen(file, "wb");
				if(!map) return;
				conoutf("received map");
				fwrite(data, 1, len, map);
				fclose(map);
				load_world(mname, oldname[0] ? oldname : NULL);
				remove(file);
#endif
				break;
			}
		}
	}

	void getmap()
	{
#ifdef BFRONTIER
		if(!m_edit(cl.gamemode)) { conoutf("\"getmap\" only works in coopedit mode"); return; }
#else
		if(cl.gamemode!=1) { conoutf("\"getmap\" only works in coopedit mode"); return; }
#endif
		conoutf("getting map...");
		addmsg(SV_GETMAP, "r");
	}

	void stopdemo()
	{
		if(remote)
		{
#ifdef BFRONTIER // priv checker
			if(!haspriv(PRIV_ADMIN)) return;
#else
            if(player1->privilege<PRIV_ADMIN) return;
#endif
			addmsg(SV_STOPDEMO, "r");
		}
		else
		{
			loopv(cl.players) if(cl.players[i]) cl.clientdisconnected(i);

			extern igameserver *sv;
			((fpsserver *)sv)->enddemoplayback();
		}
	}

    void recorddemo(int val)
	{
#ifdef BFRONTIER // priv checker
		if(!haspriv(PRIV_ADMIN)) return;
#else
        if(player1->privilege<PRIV_ADMIN) return;
#endif
        addmsg(SV_RECORDDEMO, "ri", val);
	}

	void cleardemos(int val)
	{
#ifdef BFRONTIER // priv checker
		if(!haspriv(PRIV_ADMIN)) return;
#else
        if(player1->privilege<PRIV_ADMIN) return;
#endif
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
#ifdef BFRONTIER
		if(!m_edit(cl.gamemode) || (spectator && !player1->privilege)) { conoutf("\"sendmap\" only works in coopedit mode"); return; }
#else
		if(cl.gamemode!=1 || (spectator && !player1->privilege)) { conoutf("\"sendmap\" only works in coopedit mode"); return; }
#endif
		conoutf("sending map...");
		s_sprintfd(mname)("sendmap_%d", cl.lastmillis);
		save_world(mname, true);
		s_sprintfd(fname)("packages/base/%s.ogz", mname);
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
		if(player1->state!=CS_SPECTATOR && player1->state!=CS_EDITING) return;
		int i = parseplayer(arg);
		if(i>=0 && i!=player1->clientnum) 
		{
			fpsent *d = cl.getclient(i);
			if(!d) return;
			player1->o = d->o;
			vec dir;
			vecfromyawpitch(player1->yaw, player1->pitch, 1, 0, dir);
			player1->o.add(dir.mul(-32));
		}
	}
#ifdef BFRONTIER // other stuff
	bool ready() { return connected; }
	int otherclients() { return cl.players.length(); }

	void getservcmd(char *text) // messages from the server to intercept, borrowed from executeret()
	{
		const int MAXWORDS = 25;
		char *w[MAXWORDS], *p = text;
		extern char *parseword(char *&p, int context);
		
		int numargs = MAXWORDS;
		loopi(MAXWORDS)
		{
			w[i] = "";
			if(i>numargs) continue;
			char *s = parseword(p, IDC_SERVER);
			if(s) w[i] = s;
			else numargs = i;
		}
		
		p += strcspn(p, "\0");
		
		int n;
		bool msg = true;
		string buf;
		buf[0] = 0;
		
		if (!strcmp(w[0], "something"))
		{
			if (*w[1] && (n = atoi(w[1])))
			{
				msg = n!=0;
			}
		}
		
		if (buf[0]) toservcmd(buf, msg);
	}
#endif
};
