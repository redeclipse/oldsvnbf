struct botcom : ibotcom
{
	fpsclient &cl;
	vector<extentity *> &ents;
	vector<int> avoid;
	
	#include "botutl.h"							// utilities
	#include "botmgr.h"							// management
	#include "botway.h"							// botwaypoints and nodes
	#include "botai.h"							// artificial intelligence

	botcom(fpsclient &_cl) : cl(_cl), ents(_cl.et.ents) {}
	~botcom() {}

	dynent *newclient()
	{
		botent *d = new botent();
		if (d)
		{
			s_sprintf(d->name)("bot");
			s_sprintf(d->team)("good");
		}
		return (dynent *)d;
	}

	bool allowed() { return true; }

	void update(bot *b)
	{
		if (botd(b)->connected)
		{
			if(botd(b)->state == CS_DEAD)
			{
				if(cl.lastmillis-botd(b)->lastpain<2000)
				{
					botd(b)->move = botd(b)->strafe = 0;
					cl.ph.move(botd(b), 10, false);
				}
				else if(!cl.intermission && botd(b)->state == CS_DEAD)
				{
					int last = cl.lastmillis-botd(b)->lastpain, gamemode = cl.gamemode;
					
					if(m_capture && last >= cl.cpc.RESPAWNSECS*1000)
					{
						//respawnself();
					}
				}
			}
			else if(!cl.intermission)
			{
				cl.ph.move(botd(b), 20, true);
			}
		}
	}

	bool haspriv(bot *b, int flag, bool quiet = false) // Blood Frontier
	{
		if (botd(b)->privilege>=flag) return true;
		return false;
	}

	void switchname(bot *b, const char *name)
	{
		if(name[0]) 
		{ 
			botd(b)->c2sinit = false; 
			filtertext(botd(b)->name, name, false, MAXNAMELEN);
		}
	}

	void switchteam(bot *b, const char *team)
	{
		if(team[0]) 
		{ 
			botd(b)->c2sinit = false; 
			filtertext(botd(b)->team, team, false, MAXTEAMLEN);
		}
	}

	void gameconnect(bot *b)
	{
		botd(b)->c2sinit = false;
		botd(b)->connected = false;
	}

	// collect c2s messages conveniently
	void addmsg(bot *b, int type, const char *fmt = NULL, ...)
	{
		if(botd(b)->spectator && (!botd(b)->privilege || type<SV_MASTERMODE))
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
        if(msgsize && num!=msgsize) { s_sprintfd(s)("bot inconsistent msg size for %d (%d != %d)", type, num, msgsize); fatal(s); }
		int len = p.length();
		botd(b)->messages.add(len&0xFF);
		botd(b)->messages.add((len>>8)|(reliable ? 0x80 : 0));
		loopi(len) botd(b)->messages.add(buf[i]);
	}

	int sendpacketclient(bot *b, ucharbuf &p, bool &reliable)
	{
        if(botd(b)->state==CS_ALIVE || botd(b)->state==CS_EDITING)
		{
			// send position updates separately so as to not stall out aiming
			ENetPacket *packet = enet_packet_create(NULL, 100, 0);
			ucharbuf q(packet->data, packet->dataLength);
			putint(q, SV_POS);
			putint(q, botd(b)->clientnum);
			putuint(q, (int)(botd(b)->o.x*DMF));			  // quantize coordinates to 1/4th of a cube, between 1 and 3 bytes
			putuint(q, (int)(botd(b)->o.y*DMF));
			putuint(q, (int)(botd(b)->o.z*DMF));
			putuint(q, (int)botd(b)->yaw);
			putint(q, (int)botd(b)->pitch);
			putint(q, (int)botd(b)->roll);
			putint(q, (int)(botd(b)->vel.x*DVELF));		  // quantize to itself, almost always 1 byte
			putint(q, (int)(botd(b)->vel.y*DVELF));
			putint(q, (int)(botd(b)->vel.z*DVELF));
            putuint(q, botd(b)->physstate | (botd(b)->gravity.x || botd(b)->gravity.y ? 0x20 : 0) | (botd(b)->gravity.z ? 0x10 : 0) | ((botd(b)->lifesequence&1)<<6));
			if(botd(b)->gravity.x || botd(b)->gravity.y)
			{
				putint(q, (int)(botd(b)->gravity.x*DVELF));	  // quantize to itself, almost always 1 byte
				putint(q, (int)(botd(b)->gravity.y*DVELF));
			}
			if(botd(b)->gravity.z) putint(q, (int)(botd(b)->gravity.z*DVELF));
			// pack rest in almost always 1 byte: strafe:2, move:2, garmour: 1, yarmour: 1, quad: 1
			uint flags = (botd(b)->strafe&3) | ((botd(b)->move&3)<<2);
			putuint(q, flags);
			enet_packet_resize(packet, q.length());
			botsendpackettoserv(b, packet, 0);
		}
		if(!botd(b)->c2sinit)	// tell other clients who I am
		{
			conoutf("bot sending init");
			reliable = true;
			botd(b)->c2sinit = true;
			putint(p, SV_INITC2S);
			sendstring(botd(b)->name, p);
			sendstring(botd(b)->team, p);
		}
		int i = 0;
		while(i < botd(b)->messages.length()) // send messages collected during the previous frames
		{
			int len = botd(b)->messages[i] | ((botd(b)->messages[i+1]&0x7F)<<8);
			if(p.remaining() < len) break;
			if(botd(b)->messages[i+1]&0x80) reliable = true;
			p.put(&botd(b)->messages[i+2], len);
			i += 2 + len;
		}
		botd(b)->messages.remove(0, i);
		if(!botd(b)->spectator && p.remaining()>=10 && cl.lastmillis-botd(b)->lastping>250)
		{
			putint(p, SV_PING);
			putint(p, cl.lastmillis);
			botd(b)->lastping = cl.lastmillis;
		}
		return 1;
	}

	void parsepacketclient(bot *b, int chan, ucharbuf &p)	// processes any updates from the server
	{
		switch (chan)
		{
			case 1:
			{
				parsemessages(b, p);
				break;
			}
			default:
			{
				while (p.remaining()) getint(p);
				break;
			}
		}
	}

	void parsemessages(bot *b, ucharbuf &p)
	{
		int gamemode = cl.gamemode;
		static char text[MAXTRANS];
		int last = -1, type;
		bool mapchanged = false;

		for(; p.remaining(); last = type)
		{
			switch(type = getint(p))
			{
				case SV_INITS2C:					// welcome messsage from the server
				{
					int mycn = getint(p), prot = getint(p);
					getint(p); // hasmap
					if(prot!=PROTOCOL_VERSION)
					{
						conoutf("bot using a different game protocol (you: %d, server: %d)", PROTOCOL_VERSION, prot);
						botdisconnect(b);
						return;
					}
					botd(b)->clientnum = mycn;	  // we are now fully connected
					botd(b)->connected = true;
					conoutf("bot connected successfully (%d)", botd(b)->clientnum);
					break;
				}
		
				case SV_CLIENT:
				{
					getint(p); // cn
					int len = getuint(p);
					ucharbuf q = p.subbuf(len);
					parsemessages(b, q);
					//getint(p);
					//int len = getuint(p);
					//ucharbuf q = p.subbuf(len);
					//while (q.remaining()) getint(q);
					break;
				}
		
				case SV_SOUND:
					getint(p);
					break;
		
				case SV_TEXT:
				{
					getstring(text, p);
					break;
				}
		
				case SV_MAPCHANGE:
					getstring(text, p);
					getint(p);
					mapchanged = true;
					break;
		
				case SV_ARENAWIN:
				{
					getint(p); // acn
					break;
				}
		
				case SV_FORCEDEATH:
				{
					int acn = getint(p);
					fpsent *d = acn==botd(b)->clientnum ? botd(b) : NULL;
					if(!d) break;
					d->state = CS_DEAD;
					break;
				}
		
				case SV_ITEMLIST:
				{
					int n;
					while((n = getint(p))!=-1)
					{
						getint(p); // type
					}
					break;
				}
		
				case SV_MAPRELOAD:		  // server requests next map
				{
					break;
				}
		
				case SV_INITC2S:			// another client either connected or changed name/team
				{
					botd(b)->c2sinit = false;
					getstring(text, p);
					getstring(text, p);
					break;
				}
		
				case SV_CDIS:
					break;
		
				case SV_SPAWN:
				{
					loopk(2) getint(p);
					break;
				}
		
				case SV_SPAWNSTATE:
				{
					botd(b)->respawn();
					botd(b)->lifesequence = getint(p); 
					botd(b)->health = getint(p);
					botd(b)->maxhealth = getint(p);
					botd(b)->armour = getint(p);
					botd(b)->armourtype = getint(p);
					botd(b)->gunselect = getint(p);
					loopi(GUN_PISTOL-GUN_SG+1) botd(b)->ammo[GUN_SG+i] = getint(p);
					botd(b)->state = CS_ALIVE;
					findplayerspawn(botd(b), m_capture ? cl.cpc.pickspawn(botd(b)->team) : -1);
					addmsg(b, SV_SPAWN, "rii", botd(b)->lifesequence, botd(b)->gunselect);
					break;
				}
		
				case SV_SHOTFX:
				{
					loopk(8) getint(p);
					break;
				}
		
				case SV_DAMAGE:
				{
					int tcn = getint(p);
					loopk(2) getint(p);
					int armour = getint(p), health = getint(p);
					fpsent *target = tcn==botd(b)->clientnum ? botd(b) : NULL;
					if(!target) break;
					target->armour = armour;
					target->health = health;
					//cl.damaged(damage, target, actor, false);
					break;
				}
		
				case SV_HITPUSH:
				{
					int gun = getint(p), damage = getint(p);
					vec dir;
					loopk(3) dir[k] = getint(p)/DNF;
					botd(b)->hitpush(damage, dir, NULL, gun);
					break;
				}
		
				case SV_DIED:
				{
					int vcn = getint(p), acn = getint(p), frags = getint(p);
					fpsent *victim = vcn==botd(b)->clientnum ? botd(b) : NULL,
							*actor = acn==botd(b)->clientnum ? botd(b) : NULL;
					if(!actor) break;
					actor->frags = frags;
					if(!victim) break;
					//cl.killed(victim, actor);
					break;
				}
		
				case SV_GUNSELECT:
				{
					getint(p); // gun
					break;
				}
		
				case SV_TAUNT:
				{
					break;
				}
		
				case SV_RESUME:
				{
					for(;;)
					{
						int acn = getint(p);
						if(acn<0) break;
						getint(p); // state
						int lifesequence = getint(p);
						getint(p); // gunselect
						int maxhealth = getint(p), frags = getint(p);
						fpsent *d = (acn == botd(b)->clientnum ? botd(b) : NULL);
						if(!d) continue;
						d->lifesequence = lifesequence;
						if(d->state==CS_ALIVE && d->health==d->maxhealth) d->health = maxhealth;
						d->maxhealth = maxhealth;
						d->frags = frags;
					}
					break;
				}
		
				case SV_ITEMSPAWN:
				{
					getint(p); // i
					break;
				}
		
				case SV_ITEMACC:			// server acknowledges that I picked up this item
				{
					int i = getint(p), acn = getint(p);
					fpsent *d = acn==botd(b)->clientnum ? botd(b) : NULL;
					if (d) i = 0; // FIXME
					//cl.et.pickupeffects(i, d);
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
						case SV_EDITF: dir = getint(p); mode = getint(p); break;
						case SV_EDITT: tex = getint(p); allfaces = getint(p); break;
						case SV_EDITM: mat = getint(p); break;
						case SV_FLIP: break;
						case SV_COPY: break;
						case SV_PASTE: break;
						case SV_ROTATE: dir = getint(p); break;
						case SV_REPLACE: tex = getint(p); newtex = getint(p); break;
						case SV_DELCUBE: break;
					}
					break;
				}
				case SV_REMIP:
				{
					break;
				}
				case SV_EDITENT:			// coop edit of ent
				{
					loopk(9) getint(p);
					break;
				}
		
				case SV_PONG:
					addmsg(b, SV_CLIENTPING, "i", botd(b)->ping = (botd(b)->ping*5+cl.lastmillis-getint(p))/6);
					break;
		
				case SV_CLIENTPING:
					getint(p);
					break;
		
				case SV_TIMEUP:
					getint(p); // time
					//cl.timeupdate(getint(p));
					break;
		
				case SV_SERVMSG:
					getstring(text, p);
					break;
		
				case SV_SENDDEMOLIST:
				{
					int demos = getint(p);
					if(demos) loopi(demos)
					{
						getstring(text, p);
					}
					break;
				}
		
				case SV_DEMOPLAYBACK:
				{
					int on = getint(p);
					if(on) botd(b)->state = CS_SPECTATOR;
					break;
				}
		
				case SV_CURRENTMASTER:
				{
					int mn = getint(p), priv = getint(p);
					botd(b)->privilege = PRIV_NONE;
					loopv(cl.players) if(cl.players[i]) cl.players[i]->privilege = PRIV_NONE;
					if(mn>=0)
					{
						fpsent *m = mn==botd(b)->clientnum ? botd(b) : NULL;
						if (m) m->privilege = priv;
					}
					break;
				}
		
				case SV_EDITMODE:
				{
					getint(p); // val
					break;
				}
		
				case SV_SPECTATOR:
				{
					int sn = getint(p), val = getint(p);
					fpsent *s = NULL;
					if(sn==botd(b)->clientnum)
					{
						botd(b)->spectator = val!=0;
						s = botd(b);
					}
					if(!s) return;
					if(val)
					{
						s->state = CS_SPECTATOR;
					}
					else if(s->state==CS_SPECTATOR) 
					{
						s->state = CS_DEAD;
					}
					break;
				}
		
				case SV_SETTEAM:
				{
					int wn = getint(p);
					getstring(text, p);
					fpsent *w = wn==botd(b)->clientnum ? botd(b) : NULL;
					if(!w) return;
					filtertext(w->team, text, false, MAXTEAMLEN);
					break;
				}
		
				case SV_BASEINFO:
				{
					getint(p); // base
					getstring(text, p);
					getstring(text, p);
					loopk(2) getint(p);
					break;
				}
		
				case SV_BASES:
				{
					int base = 0, ammotype;
					while((ammotype = getint(p))>=0)
					{
						getstring(text, p);
						getstring(text, p);
						loopk(2) getint(p);
						base++;
					}
					break;
				}
		
				case SV_TEAMSCORE:
				{
					getstring(text, p);
					getint(p); // total
					break;
				}
		
				case SV_REPAMMO:
				{
					getint(p); // ammotype
					//int ammotype = getint(p);
					//int gamemode = cl.gamemode;
					//cl.cpc.receiveammo(ammotype);
					break;
				}
		
				case SV_ANNOUNCE:
				{
					getint(p); // t
					break;
				}
		
				case SV_NEWMAP:
				{
					getint(p); // size
					break;
				}
		
				default:
					botneterr(b, "type", type);
					return;
			}
			conoutf("bot msg: %d (%s)", type, msgnames[type]);
		}
	}
};
