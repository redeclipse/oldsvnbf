struct clientcom : iclientcom
{
	GAMECLIENT &cl;

	bool c2sinit, senditemstoserver, isready, remote, demoplayback, spectator;
	int lastping;

	IVARP(centerchat, 0, 1, 1);
	IVARP(colourchat, 0, 1, 1);

	ISVARP(serversort, "");

	clientcom(GAMECLIENT &_cl) : cl(_cl),
		c2sinit(false), senditemstoserver(false),
		isready(false), remote(false), demoplayback(false), spectator(false),
		lastping(0)
	{
        CCOMMAND(say, "C", (clientcom *self, char *s), self->toserver(SAY_NONE, s));
        CCOMMAND(me, "C", (clientcom *self, char *s), self->toserver(SAY_ACTION, s));
        CCOMMAND(sayteam, "C", (clientcom *self, char *s), self->toserver(SAY_TEAM, s));
        CCOMMAND(meteam, "C", (clientcom *self, char *s), self->toserver(SAY_ACTION|SAY_TEAM, s));
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
        CCOMMAND(getteam, "", (clientcom *self), result(teamtype[self->cl.player1->team].name));

		CCOMMAND(serversortreset, "", (clientcom *self), self->resetserversort());
        if(!*serversort()) resetserversort();
	}

	void switchname(const char *name)
	{
		if(name[0])
		{
			c2sinit = false;
			s_strncpy(cl.player1->name, name, MAXNAMELEN);
		}
		else conoutf("your name is: %s", cl.colorname(cl.player1));
	}

	int teamname(const char *team)
	{
		if(m_team(cl.gamemode, cl.mutators))
		{
			if(team[0])
			{
				int t = atoi(team);

				loopi(numteams(cl.gamemode, cl.mutators))
				{
					if((t && t == i+TEAM_ALPHA) || !strcasecmp(teamtype[i+TEAM_ALPHA].name, team))
					{
						return i+TEAM_ALPHA;
					}
				}
			}
			return TEAM_ALPHA;
		}
		return TEAM_NEUTRAL;
	}

	void switchteam(const char *team)
	{
		if(m_team(cl.gamemode, cl.mutators))
		{
			if(team[0])
			{
				int t = teamname(team);
				if(t != cl.player1->team)
				{
					c2sinit = false;
					cl.player1->team = t;
				}
			}
			else conoutf("your team is: %s", teamtype[cl.player1->team].name);
		}
		else conoutf("can only change teams in team games");
	}

	int numchannels() { return 3; }

	void mapstart() { if(!spectator || cl.player1->privilege) senditemstoserver = true; }

	void writeclientinfo(FILE *f)
	{
		fprintf(f, "name \"%s\"\n\n", cl.player1->name);
	}

	void gameconnect(bool _remote)
	{
		remote = _remote;
		if(editmode) toggleedit();
	}

	void gamedisconnect(int clean)
	{
		if(editmode) toggleedit();
		remote = isready = c2sinit = spectator = false;
		cl.player1->clientnum = -1;
		cl.player1->lifesequence = 0;
		cl.player1->privilege = PRIV_NONE;
        removetrackedparticles();
		loopv(cl.players) DELETEP(cl.players[i]);
		cleardynentcache();
		emptymap(0, true, NULL, true);
	}

	bool allowedittoggle(bool edit)
	{
		bool allow = edit || m_edit(cl.gamemode);
		if(!allow) conoutf("you must be both alive and in coopedit to enter editmode");
		return allow;
	}

	void edittoggled(bool edit)
	{
		if(!edit)
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

		if(m_edit(cl.gamemode))
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
		if(m_team(cl.gamemode, cl.mutators))
		{
			int i = parseplayer(arg1);
			if(i>=0 && i!=cl.player1->clientnum)
			{
				int t = teamname(arg2);
				if(t) addmsg(SV_SETTEAM, "ri2", i, t);
			}
		}
		else conoutf("can only change teams in team games");
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
        if(msgsize && num!=msgsize) { s_sprintfd(s)("inconsistent msg size for %d (%d != %d)", type, num, msgsize); fatal(s); }
		int len = p.length();
		messages.add(len&0xFF);
		messages.add((len>>8)|(reliable ? 0x80 : 0));
		loopi(len) messages.add(buf[i]);
	}

	void saytext(fpsent *d, int flags, char *text)
	{
		if(!colourchat()) filtertext(text, text);

		string s, t, m;
		s_sprintf(t)("%s", m_team(cl.gamemode, cl.mutators) && (flags & SAY_TEAM) ? teamtype[d->team].chat : teamtype[TEAM_NEUTRAL].chat);
		s_sprintf(m)("%s", cl.colorname(d, NULL, "", m_team(cl.gamemode, cl.mutators) && (flags & SAY_TEAM) ? false : true));

		if(flags&SAY_ACTION) s_sprintf(s)("%s* \fs%s\fS \fs%s\fS", t, m, text);
		else s_sprintf(s)("%s<\fs\fw%s\fS> \fs\fw%s\fS", t, m, text);

		if(d->state != CS_DEAD && d->state != CS_SPECTATOR)
		{
			s_sprintfd(ds)("@%s", &s);
			particle_text(d->abovehead(), ds, 9);
		}

		console("%s", (centerchat() ? CON_CENTER : 0)|CON_NORMAL, s);
		playsound(S_CHAT);
	}

	void toserver(int flags, char *text)
	{
		saytext(cl.player1, flags, text);
		addmsg(SV_TEXT, "ri2s", cl.player1->clientnum, flags, text);
	}

	void toservcmd(char *text, bool msg)
	{
		addmsg(SV_COMMAND, "rs", text);
	}

	void updateposition(fpsent *d)
	{
        if(d->state==CS_ALIVE || d->state==CS_EDITING)
		{
			// send position updates separately so as to not stall out aiming
			ENetPacket *packet = enet_packet_create(NULL, 100, 0);
			ucharbuf q(packet->data, packet->dataLength);
			putint(q, SV_POS);
			putint(q, d->clientnum);
			putuint(q, (int)(d->o.x*DMF));			  // quantize coordinates to 1/4th of a cube, between 1 and 3 bytes
			putuint(q, (int)(d->o.y*DMF));
			putuint(q, (int)(d->o.z*DMF));
			putint(q, (int)d->yaw);
			putint(q, (int)d->pitch);
			putint(q, (int)d->roll);
			putint(q, (int)d->aimyaw);
			putint(q, (int)d->aimpitch);
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
	}

	int sendpacketclient(ucharbuf &p, bool &reliable)
	{
		updateposition(cl.player1);
		loopv(cl.players)
			if(cl.players[i] && cl.players[i]->bot)
				updateposition(cl.players[i]);

		if(senditemstoserver)
		{
			reliable = true;
			putint(p, SV_ITEMLIST);
			cl.et.putitems(p);
			putint(p, -1);
			if(m_stf(cl.gamemode)) cl.stf.sendflags(p);
            else if(m_ctf(cl.gamemode)) cl.ctf.sendflags(p);
			senditemstoserver = false;
		}
		if(!c2sinit)	// tell other clients who I am
		{
			reliable = true;
			c2sinit = true;
			putint(p, SV_INITC2S);
			sendstring(cl.player1->name, p);
			putint(p, cl.player1->team);
			loopv(cl.players) if(cl.players[i] && cl.players[i]->bot)
			{
				fpsent *f = cl.players[i];
				putint(p, SV_INITBOT);
				putint(p, f->ownernum);
				putint(p, f->skill);
				putint(p, f->clientnum);
				sendstring(f->name, p);
				putint(p, f->team);
			}
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

    void parsestate(fpsent *d, ucharbuf &p, bool resume = false)
    {
        if(!d) { static fpsent dummy; d = &dummy; }
		if(d==cl.player1) getint(p);
		else d->state = getint(p);
		d->frags = getint(p);
        d->lifesequence = getint(p);
        d->health = getint(p);
        if(resume && d==cl.player1)
        {
            getint(p);
            loopi(NUMGUNS) getint(p);
        }
        else
        {
            d->gunselect = getint(p);
            loopi(NUMGUNS) d->ammo[i] = getint(p);
        }
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
				int lcn = getint(p);
				vec o, vel, falling;
				float yaw, pitch, roll, aimyaw, aimpitch;
				int physstate, f;
				o.x = getuint(p)/DMF;
				o.y = getuint(p)/DMF;
				o.z = getuint(p)/DMF;
				yaw = (float)getint(p);
				pitch = (float)getint(p);
				roll = (float)getint(p);
				aimyaw = (float)getint(p);
				aimpitch = (float)getint(p);
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
				fpsent *d = cl.getclient(lcn);
                if(!d || seqcolor!=(d->lifesequence&1) || d==cl.player1 || d->bot) continue;
                float oldyaw = d->yaw, oldpitch = d->pitch, oldaimyaw = d->aimyaw, oldaimpitch = d->aimpitch;
				d->yaw = yaw;
				d->pitch = pitch;
				d->roll = roll;
				d->aimyaw = aimyaw;
				d->aimpitch = aimpitch;
				d->strafe = (f&3)==3 ? -1 : f&3;
				f >>= 2;
				d->move = (f&3)==3 ? -1 : f&3;
				f >>= 2;
				bool crouch = d->crouching;
				d->crouching = f&1 ? true : false;
				if(crouch != d->crouching) d->crouchtime = lastmillis;
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
                    d->newaimyaw = d->aimyaw;
                    d->newaimpitch = d->aimpitch;

                    d->o = oldpos;
                    d->yaw = oldyaw;
                    d->pitch = oldpitch;
                    d->aimyaw = oldaimyaw;
                    d->aimpitch = oldaimpitch;

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
		int type = -1, prevtype = -1;
		bool mapchanged = false;

		while(p.remaining())
		{
			prevtype = type;
			type = getint(p);
			//conoutf("[client] msg: %d, prev: %d", type, prevtype);
			switch(type)
			{
				case SV_INITS2C:					// welcome messsage from the server
				{
					int mycn = getint(p), gver = getint(p), hasmap = getint(p);
					if(gver!=GAMEVERSION)
					{
						conoutf("you are using a different game version (you: %d, server: %d)", GAMEVERSION, gver);
						disconnect();
						return;
					}
					cl.player1->clientnum = mycn;	  // we are now fully ready
					switch (hasmap)
					{
						case 0:
							changemap(getmapname());
							break;
						case 2:
							senditemstoserver = true;
							break;
						case 1:
						default:
							break;
					}
					isready = true;
					break;
				}

				case SV_CLIENT:
				{
					int lcn = getint(p), len = getuint(p);
					ucharbuf q = p.subbuf(len);
					parsemessages(lcn, cl.getclient(lcn), q);
					break;
				}

				case SV_ANNOUNCE:
				{
					int snd = getint(p);
					getstring(text, p);
					cl.et.announce(snd, text);
					break;
				}

				case SV_SOUND:
				{
					int tcn = getint(p), snd = getint(p);
					if(snd < 0 || snd >= S_MAX) break;
					fpsent *t = cl.getclient(tcn);
					if(!t) break;
					else playsound(snd, &t->o);
					break;
				}

				case SV_TEXT:
				{
					int tcn = getint(p);
					fpsent *t = cl.getclient(tcn);
					int flags = getint(p);
					getstring(text, p);
					if(!t) break;
					saytext(t, flags, text);
					break;
				}

				case SV_EXECLINK:
				{
					int tcn = getint(p), index = getint(p);
					fpsent *t = cl.getclient(tcn);
					if(!t || !d || (t->clientnum!=d->clientnum && t->ownernum!=d->clientnum)) break;
					cl.et.execlink(t, index, false);
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

				case SV_FORCEDEATH:
				{
					int lcn = getint(p);
					fpsent *f = cl.newclient(lcn);
					if(!f) break;
					if(f==cl.player1)
					{
						if(editmode) toggleedit();
						cl.sb.showscores(true);
					}
					f->state = CS_DEAD;
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

				case SV_MAPRELOAD: // server requests next map
				{
					if(cl.intermission) cl.sb.showscores(false);
					if(m_stf(cl.gamemode)) showgui("stfmaps");
					else if(m_ctf(cl.gamemode)) showgui("ctfmaps");
					else showgui("maps");
					break;
				}

				case SV_INITC2S:			// another client either connected or changed name/team
				{
					d = cl.newclient(cn);
					if(!d)
					{
						getstring(text, p);
						getint(p);
						break;
					}
					getstring(text, p);
					if(!text[0]) s_strcpy(text, "unnamed");
					if(d->name[0])		  // already connected
					{
						if(strcmp(d->name, text))
						{
							string oldname, newname;
							s_strcpy(oldname, cl.colorname(d, NULL, false));
							s_strcpy(newname, cl.colorname(d, text));
							conoutf("%s is now known as %s", oldname, newname);
						}
					}
					else					// new client
					{
						c2sinit = false;	// send new players my info again
						d->respawn(lastmillis);
						conoutf("connected: %s", cl.colorname(d, text, false));
						loopv(cl.players)	// clear copies since new player doesn't have them
							if(cl.players[i]) freeeditinfo(cl.players[i]->edit);
						extern editinfo *localedit;
						freeeditinfo(localedit);
					}
					s_strncpy(d->name, text, MAXNAMELEN);
					d->team = getint(p);
					break;
				}

				case SV_CDIS:
					cl.clientdisconnected(getint(p));
					break;

				case SV_SPAWN:
				{
					int lcn = getint(p);
					fpsent *f = cl.newclient(lcn);
					f->respawn(lastmillis);
					parsestate(f, p);
					f->state = CS_SPAWNING;
					playsound(S_RESPAWN, &f->o);
					break;
				}

				case SV_SPAWNSTATE:
				{
					int lcn = getint(p);
					fpsent *f = cl.newclient(lcn);
					bool local = f == cl.player1 || f->bot;
					if(f == cl.player1 && editmode) toggleedit();
					f->respawn(lastmillis);
					parsestate(f, p);
					f->state = CS_ALIVE;
					if(local)
					{
						cl.et.findplayerspawn(f, m_stf(cl.gamemode) ? cl.stf.pickspawn(f->team) : -1, m_team(cl.gamemode, cl.mutators) ? f->team : -1);
						if(f==cl.player1)
						{
							cl.sb.showscores(false);
							cl.lasthit = 0;
						}
						addmsg(SV_SPAWN, "ri3", f->clientnum, f->lifesequence, f->gunselect);
						playsound(S_RESPAWN, &f->o);
					}
					cl.bot.spawned(f);
					break;
				}

				case SV_SHOTFX:
				{
					int scn = getint(p), gun = getint(p), power = clamp(getint(p), 0, 100);
					vec from, to;
					loopk(3) from[k] = getint(p)/DMF;
					loopk(3) to[k] = getint(p)/DMF;
					fpsent *s = cl.getclient(scn);
					if(!s || !isgun(gun)) break;
					if(gun==GUN_SG) cl.ws.createrays(from, to);
					s->setgunstate(gun, GUNSTATE_SHOOT, guntype[gun].adelay, lastmillis);
					cl.ws.shootv(gun, power, from, to, s, false);
					break;
				}

				case SV_DAMAGE:
				{
					int tcn = getint(p),
						acn = getint(p),
						gun = getint(p),
						flags = getint(p),
						damage = getint(p),
						health = getint(p);
					vec dir;
					loopk(3) dir[k] = getint(p)/DNF;
					fpsent *target = cl.getclient(tcn), *actor = cl.getclient(acn);
					if(!target || !actor) break;
					cl.damaged(gun, flags, damage, target, actor, lastmillis, dir);
					target->health = health; // just in case
					break;
				}

				case SV_RELOAD:
				{
					int trg = getint(p), gun = getint(p), amt = getint(p);
					fpsent *target = cl.getclient(trg);
					if(!target || gun <= -1 || gun >= NUMGUNS) break;
					target->gunreload(gun, amt, lastmillis);
					playsound(S_RELOAD, &target->o);
					break;
				}

				case SV_REGEN:
				{
					int trg = getint(p), amt = getint(p);
					fpsent *target = cl.getclient(trg);
					if(!target) break;
					target->health = amt;
					target->lastregen = lastmillis;
					particle_splash(3, max((MAXHEALTH-target->health)/10, 1), 10000, target->o);
					playsound(S_REGEN, &target->o, ((MAXHEALTH-target->health)*255)/MAXHEALTH);
					break;
				}

				case SV_DIED:
				{
					int vcn = getint(p), acn = getint(p), frags = getint(p),
						gun = getint(p), flags = getint(p), damage = getint(p);
					fpsent *victim = cl.getclient(vcn), *actor = cl.getclient(acn);
					if(!actor) break;
					actor->frags = frags;
					if(actor!=cl.player1 && !m_stf(cl.gamemode) && !m_ctf(cl.gamemode))
					{
						s_sprintfd(ds)("@%d", actor->frags);
						particle_text(actor->abovehead(), ds, 9);
					}
					if(!victim) break;
					cl.killed(gun, flags, damage, victim, actor);
					break;
				}

				case SV_GUNSELECT:
				{
					int trg = getint(p), gun = getint(p);
					fpsent *target = cl.getclient(trg);
					if(!target || gun <= -1 || gun >= NUMGUNS) break;
					target->setgun(gun, lastmillis);
					playsound(S_SWITCH, &target->o);
					break;
				}

				case SV_TAUNT:
				{
					int lcn = getint(p);
					fpsent *f = cl.getclient(lcn);
					if(!f) break;
					f->lasttaunt = lastmillis;
					break;
				}

				case SV_RESUME:
				{
					for(;;)
					{
						int lcn = getint(p);
						if(p.overread() || lcn < 0) break;
						fpsent *f = cl.getclient(lcn);
						parsestate(f, p, true);
					}
					break;
				}

				case SV_ITEMSPAWN:
				{
					int i = getint(p);
					if(!cl.et.ents.inrange(i)) break;
					cl.et.setspawn(i, true);
					playsound(S_ITEMSPAWN, &cl.et.ents[i]->o);
					const char *name = cl.et.itemname(i);
					if(name) particle_text(cl.et.ents[i]->o, name, 9);
					break;
				}

				case SV_ITEMACC:			// server acknowledges that I picked up this item
				{
					int lcn = getint(p), i = getint(p);
					fpsent *f = cl.getclient(lcn);
					if(!f) break;
					cl.et.useeffects(f, lastmillis, i);
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
								if(id->minval > id->maxval || val < id->minval || val > id->maxval)
									commit = false;
								setvar(text, val, true);
								conoutf("%s set variable %s to %d", cl.colorname(d), id->name, *id->storage.i);
							}
							break;
						}
						case ID_FVAR:
						{
							float val = getfloat(p);
							if(commit)
							{
								setfvar(text, val, true);
								conoutf("%s set float variable %s to %f", cl.colorname(d), id->name, *id->storage.f);
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
								conoutf("%s set string variable %s to %s", cl.colorname(d), id->name, *id->storage.s);
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
								conoutf("%s set world alias %s to %s", cl.colorname(d), text, val);
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

				case SV_EDITLINK:
				{
					if(!d) return;
					int b = getint(p), index = getint(p), node = getint(p);
					cl.et.linkents(index, node, b!=0, false, false);
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
						fpsent *m = cl.getclient(mn);
						if(m) m->privilege = priv;
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
					fpsent *s = cl.newclient(sn);
					if(!s) break;
					if(s == cl.player1) spectator = val!=0;
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
					int wn = getint(p), tn = getint(p);
					fpsent *w = cl.getclient(wn);
					if(!w) return;
					w->team = tn;
					break;
				}

				case SV_FLAGINFO:
				{
					int flag = getint(p), converted = getint(p),
							owner = getint(p), enemy = getint(p);
					if(m_stf(cl.gamemode)) cl.stf.updateflag(flag, owner, enemy, converted);
					break;
				}

				case SV_FLAGS:
				{
					int flag = 0, converted;
					while((converted = getint(p))>=0)
					{
						int owner = getint(p), enemy = getint(p);
						cl.stf.initflag(flag++, owner, enemy, converted);
					}
					break;
				}

				case SV_TEAMSCORE:
				{
					int team = getint(p), total = getint(p);
					if(m_stf(cl.gamemode)) cl.stf.setscore(team, total);
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
					fpsent *o = cl.newclient(ocn);
					if(o && m_ctf(cl.gamemode)) cl.ctf.dropflag(o, flag, droploc);
					break;
				}

				case SV_SCOREFLAG:
				{
					int ocn = getint(p), relayflag = getint(p), goalflag = getint(p), score = getint(p);
					fpsent *o = cl.newclient(ocn);
					if(o && m_ctf(cl.gamemode)) cl.ctf.scoreflag(o, relayflag, goalflag, score);
					break;
				}

				case SV_RETURNFLAG:
				{
					int ocn = getint(p), flag = getint(p);
					fpsent *o = cl.newclient(ocn);
					if(o && m_ctf(cl.gamemode)) cl.ctf.returnflag(o, flag);
					break;
				}

				case SV_TAKEFLAG:
				{
					int ocn = getint(p), flag = getint(p);
					fpsent *o = cl.newclient(ocn);
					if(o && m_ctf(cl.gamemode)) cl.ctf.takeflag(o, flag);
					break;
				}

				case SV_RESETFLAG:
				{
					int flag = getint(p);
					if(m_ctf(cl.gamemode)) cl.ctf.resetflag(flag);
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

				case SV_INITBOT:
				{
					int on = getint(p), sk = getint(p), bn = getint(p);
					fpsent *b = cl.newclient(bn);
					if(!b)
					{
						getint(p);
						getstring(text, p);
						break;
					}

					bool connecting = !b->name[0];
					fpsent *o = cl.getclient(on);
					s_sprintfd(m)("%s", o ? cl.colorname(o) : "unknown");

					b->ownernum = on;
					b->skill = clamp(sk, 1, 100);
					getstring(text, p);
					s_strncpy(b->name, text, MAXNAMELEN);
					b->team = getint(p);

					if(o)
					{
						if(connecting)
							conoutf("%s (skill: %d) introduced and assigned to %s", cl.colorname(b), b->skill, m);
						else conoutf("%s (skill: %d) reassigned to %s", cl.colorname(b), b->skill, m);
					}

					if(cl.player1->clientnum == b->ownernum) cl.bot.create(b);
					else if(b->bot) cl.bot.destroy(b);
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

	void changemapserv(char *name, int gamemode, int mutators)
	{
		if(editmode) toggleedit();
		cl.gamemode = gamemode; cl.mutators = mutators;
		sv->modecheck(&cl.gamemode, &cl.mutators);
		cl.nextmode = cl.gamemode; cl.nextmuts = cl.mutators;
		cl.minremain = -1;
		if(editmode && !allowedittoggle(editmode)) toggleedit();
		if(m_demo(gamemode)) return;
		isready = false;
		load_world(name);
		if(m_stf(gamemode)) cl.stf.setupflags();
        else if(m_ctf(gamemode)) cl.ctf.setupflags();
		isready = true;
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
				extern string mapfile, mapname;
				const char *file = findfile(mapfile, "wb");
				FILE *map = fopen(file, "wb");
				if(!map) return;
				conoutf("received map");
				fwrite(data, 1, len, map);
				fclose(map);
				load_world(mapname);
				break;
			}
		}
	}

	void getmap()
	{
		if(!m_edit(cl.gamemode)) { conoutf("\"getmap\" only works while editing"); return; }
		conoutf("getting map...");
		addmsg(SV_GETMAP, "r");
	}

	void stopdemo()
	{
		if(remote) addmsg(SV_STOPDEMO, "r");
		else
		{
			loopv(cl.players) if(cl.players[i]) cl.clientdisconnected(i);

			extern igameserver *sv;
			((GAMESERVER *)sv)->enddemoplayback();
		}
	}

    void recorddemo(int val)
	{
        addmsg(SV_RECORDDEMO, "ri", val);
	}

	void cleardemos(int val)
	{
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
		s_sprintfd(mname)("%s", getmapname());
		save_world(mname, true);
		s_sprintfd(fname)("%s.bgz", mname);
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

	bool ready() { return isready; }
	int state() { return cl.player1->state; }
	int otherclients()
	{
		int n = 0; // bots don't count
		loopv(cl.players) if(cl.players[i] && cl.players[i]->ownernum < 0) n++;
		return n;
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
		s_sprintfd(u)("serversort [%d %d %d]", SINFO_STATUS, SINFO_PLAYERS, SINFO_PING);
		execute(u);
	}

	int servercompare(serverinfo *a, serverinfo *b)
	{
		int ac = 0, bc = 0;

		if(a->address.host != ENET_HOST_ANY && a->ping < 999 &&
			a->attr.length() && a->attr[0] == GAMEVERSION) ac = 1;

		if(b->address.host != ENET_HOST_ANY && b->ping < 999 &&
			b->attr.length() && b->attr[0] == GAMEVERSION) bc = 1;

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
			s_sprintfd(s)("at $serversort %d", i);

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

    bool serverinfostartcolumn(g3d_gui *g, int i)
    {
    	if(i > -1 && i < SINFO_MAX)
    	{
			g->pushlist();

			if(g->buttonf("%s ", 0xA0A0A0, NULL, serverinfotypes[i]) & G3D_UP)
			{
				string st; st[0] = 0;
				bool invert = false;
				int len = execute("listlen $serversort");
				loopk(len)
				{
					s_sprintfd(s)("at $serversort %d", k);

					int n = execute(s);
					if(abs(n) != i)
					{
						s_sprintfd(t)("%s%d", st[0] ? " " : "", n);
						s_sprintf(st)("%s%s", st[0] ? st : "", t);
					}
					else if(!k) invert = true;
				}
				s_sprintfd(u)("serversort = \"%d%s%s\"",
					invert ? 0-i : i, st[0] ? " " : "", st[0] ? st : "");
				execute(u);
			}

			g->mergehits(true);
			return true;
    	}
        return false;
    }

    void serverinfoendcolumn(g3d_gui *g, int i)
    {
        g->mergehits(false);
        g->poplist();
    }

    bool serverinfoentry(g3d_gui *g, int i, serverinfo &si)
    {
		string text; text[0] = 0;
		int colour = serverstatus[serverstat(&si)].colour;
		switch(i)
		{
			case SINFO_STATUS:
			{
				if(g->button("", colour, serverstatus[serverstat(&si)].icon) & G3D_UP)
					return true;
				break;
			}
			case SINFO_HOST:
			{
				if(g->buttonf("%s ", colour, NULL, si.name) & G3D_UP) return true;
				break;
			}
			case SINFO_DESC:
			{
				s_strncpy(text, si.sdesc, 18);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_PING:
			{
				s_sprintf(text)("%d", si.ping);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_PLAYERS:
			{
				s_sprintf(text)("%d", si.numplayers);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_MAXCLIENTS:
			{
				if(si.attr.length() > 4 && si.attr[4] >= 0)
					s_sprintf(text)("%d", si.attr[4]);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_GAME:
			{
				if(si.attr.length() > 2)
					s_sprintf(text)("%s", sv->gamename(si.attr[1], si.attr[2]));
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_MAP:
			{
				s_strncpy(text, si.map, 18);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_TIME:
			{
				if(si.attr.length() > 3 && si.attr[3] >= 0)
					s_sprintf(text)("%d %s", si.attr[3], si.attr[3] == 1 ? "min" : "mins");
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			default:
				break;
		}
		return false;
    }

	const char *serverinfogui(g3d_gui *g, vector<serverinfo *> &servers)
	{
		const char *name = NULL;
		for(int start = 0; start < servers.length();)
		{
			if(start > 0) g->tab();
			int end = servers.length();
			g->pushlist();
			loopi(SINFO_MAX)
			{
				if(!serverinfostartcolumn(g, i)) break;
				for(int j = start; j < end; j++)
				{
					if(!i && g->shouldtab()) { end = j; break; }
					serverinfo &si = *servers[j];
					if(si.ping >= 0 && si.attr.length() && si.attr[0]==GAMEVERSION)
					{
						if(serverinfoentry(g, i, si)) name = si.name;
					}
				}
				serverinfoendcolumn(g, i);
			}
			g->poplist();
			start = end;
		}
		return name;
	}
} cc;
