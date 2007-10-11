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
	}
};
