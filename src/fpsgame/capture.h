// capture.h: client and server state for capture gamemode

struct capturestate
{
	static const int CAPTURERADIUS = 64;
	static const int CAPTUREHEIGHT = 24;
	static const int OCCUPYPOINTS = 15;
	static const int OCCUPYLIMIT = 100;
	static const int CAPTURESCORE = 1;
	static const int SCORESECS = 10;
#ifndef BFRONTIER
	static const int AMMOSECS = 15;
	static const int MAXAMMO = 5;
	static const int REPAMMODIST = 32;
#endif
	static const int RESPAWNSECS = 10;		

	struct baseinfo
	{
		vec o;
		string owner, enemy;
#ifndef CAPTURESERV
		string name, info;
		extentity *ent;
#endif
#ifdef BFRONTIER
		int enemies, converted, capturetime;
#else
		int ammotype, ammo, enemies, converted, capturetime;
#endif

		baseinfo() { reset(); }

		void noenemy()
		{
			enemy[0] = '\0';
			enemies = 0;
			converted = 0;
		}

		void reset()
		{
			noenemy();
			owner[0] = '\0';
			capturetime = -1;
#ifndef BFRONTIER
			ammotype = 0;
			ammo = 0;
#endif
		}

		bool enter(const char *team)
		{
			if(!enemy[0])
			{
				if(!strcmp(owner, team)) return false;
				s_strcpy(enemy, team);
				enemies++;
				return true;
			}
			else if(strcmp(enemy, team)) return false;
			else enemies++;
			return false;
		}

		bool steal(const char *team)
		{
			return !enemy[0] && strcmp(owner, team);
		}
			
		bool leave(const char *team)
		{
			if(strcmp(enemy, team)) return false;
			enemies--;
			if(!enemies) noenemy();
			return !enemies;
		}

		int occupy(const char *team, int units)
		{
			if(strcmp(enemy, team)) return -1;
			converted += units;
			if(converted<(owner[0] ? 2 : 1)*OCCUPYLIMIT) return -1;
			if(owner[0]) { owner[0] = '\0'; converted = 0; s_strcpy(enemy, team); return 0; }
#ifdef BFRONTIER
			else { s_strcpy(owner, team); capturetime = 0; noenemy(); return 1; }
#else
			else { s_strcpy(owner, team); ammo = 0; capturetime = 0; noenemy(); return 1; }
#endif
		}

#ifndef BFRONTIER
		bool addammo(int i)
		{
			if(ammo>=MAXAMMO) return false;
			ammo = min(ammo+i, MAXAMMO);
			return true;
		}

		bool takeammo(const char *team)
		{
			if(strcmp(owner, team) || ammo<=0) return false;
			ammo--;
			return true;
		}
#endif
	};

	vector<baseinfo> bases;

	struct score
	{
		string team;
		int total;
	};
	
	vector<score> scores;

	int captures;

	capturestate() : captures(0) {}

	void reset()
	{
		bases.setsize(0);
		scores.setsize(0);
		captures = 0;
	}

	score &findscore(const char *team)
	{
		loopv(scores)
		{
			score &cs = scores[i];
			if(!strcmp(cs.team, team)) return cs;
		}
		score &cs = scores.add();
		s_strcpy(cs.team, team);
		cs.total = 0;
		return cs;
	}

#ifdef BFRONTIER
	void addbase(const vec &o)
	{
		baseinfo &b = bases.add();
		b.o = o;
	}

	void initbase(int i, const char *owner, const char *enemy, int converted)
	{
		if(!bases.inrange(i)) return;
		baseinfo &b = bases[i];
		s_strcpy(b.owner, owner);
		s_strcpy(b.enemy, enemy);
		b.converted = converted;
	}
#else
	void addbase(int ammotype, const vec &o)
	{
		baseinfo &b = bases.add();
		b.ammotype = ammotype ? ammotype : rnd(5)+1;
		b.o = o;
	}

	void initbase(int i, int ammotype, const char *owner, const char *enemy, int converted, int ammo)
	{
		if(!bases.inrange(i)) return;
		baseinfo &b = bases[i];
		b.ammotype = ammotype;
		s_strcpy(b.owner, owner);
		s_strcpy(b.enemy, enemy);
		b.converted = converted;
		b.ammo = ammo;
	}
#endif

	bool hasbases(const char *team)
	{
		loopv(bases)
		{
			baseinfo &b = bases[i]; 
			if(b.owner[0] && !strcmp(b.owner, team)) return true;
		}
		return false;
	}

	float disttoenemy(baseinfo &b)
	{
		float dist = 1e10f;
		loopv(bases)
		{
			baseinfo &e = bases[i];
			if(e.owner[0] && strcmp(b.owner, e.owner))
				dist = min(dist, b.o.dist(e.o));
		}
		return dist;
	}

	bool insidebase(const baseinfo &b, const vec &o)
	{
		float dx = (b.o.x-o.x), dy = (b.o.y-o.y), dz = (b.o.z-o.z+14);
		return dx*dx + dy*dy <= CAPTURERADIUS*CAPTURERADIUS && fabs(dz) <= CAPTUREHEIGHT; 
	}
};

#ifndef CAPTURESERV

struct captureclient : capturestate
{
	fpsclient &cl;
	float radarscale;

	captureclient(fpsclient &cl) : cl(cl), radarscale(0)
	{
#ifndef BFRONTIER
        CCOMMAND(repammo, "", (captureclient *self), self->replenishammo());
#endif
	}
	
#ifndef BFRONTIER
	void replenishammo()
	{
		int gamemode = cl.gamemode;
		if(m_noitemsrail) return;
		loopv(bases)
		{
			baseinfo &b = bases[i];
			if(b.ammotype>0 && b.ammotype<=I_CARTRIDGES-I_SHELLS+1 && insidebase(b, cl.player1->o) && cl.player1->hasmaxammo(b.ammotype-1+I_SHELLS)) return;
		}
		cl.cc.addmsg(SV_REPAMMO, "r");
	}

	void receiveammo(int type)
	{
		type += I_SHELLS-1;
		if(type<I_SHELLS || type>I_CARTRIDGES) return;
		cl.et.repammo(cl.player1, type);
	}
#endif

	void renderbases()
	{
		loopv(bases)
		{
			baseinfo &b = bases[i];
			const char *flagname = b.owner[0] ? (strcmp(b.owner, cl.player1->team) ? "flags/red" : "flags/blue") : "flags/neutral";
			rendermodel(b.ent->color, b.ent->dir, flagname, ANIM_MAPMODEL|ANIM_LOOP, 0, 0, b.o, 0, 0, 0, 0, NULL, MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_OCCLUDED);
#ifndef BFRONTIER
			if(b.ammotype>0 && b.ammotype<=I_CARTRIDGES-I_SHELLS+1) loopi(b.ammo)
			{
				float angle = 2*M_PI*(cl.lastmillis/4000.0f + i/float(MAXAMMO));
				vec p(b.o);
				p.x += 10*cosf(angle);
				p.y += 10*sinf(angle);
				p.z += 4;
				rendermodel(b.ent->color, b.ent->dir, cl.et.entmdlname(I_SHELLS+b.ammotype-1), ANIM_MAPMODEL|ANIM_LOOP, 0, 0, p, 0, 0, 0, 0, NULL, MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_OCCLUDED);
			}
#endif
			int ttype = 11, mtype = -1;
			if(b.owner[0])
			{
				bool isowner = !strcmp(b.owner, cl.player1->team);
				if(b.enemy[0])
				{
					s_sprintf(b.info)("\f%d%s \f0vs. \f%d%s", isowner ? 3 : 1, b.enemy, isowner ? 1 : 3, b.owner);
					mtype = isowner ? 19 : 20; 
				}
				else { s_sprintf(b.info)("%s", b.owner); ttype = isowner ? 16 : 13; }
			}
			else if(b.enemy[0])
			{
				s_sprintf(b.info)("%s", b.enemy);
				if(strcmp(b.enemy, cl.player1->team)) { ttype = 13; mtype = 17; }
				else { ttype = 16; mtype = 18; }
			}
			else b.info[0] = '\0';
			vec above(b.o);
			abovemodel(above, flagname);
			above.z += 2.0f;
			particle_text(above, b.info, ttype, 1);
			if(mtype>=0)
			{
				above.z += 3.0f;
				particle_meter(above, b.converted/float((b.owner[0] ? 2 : 1) * OCCUPYLIMIT), mtype, 1);
			}
		}
	}

	void drawradar(float x, float y, float s)
	{
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x,	y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x+s, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x+s, y+s);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x,	y+s);
	}
	
	IVARP(maxradarscale, 0, 1024, 10000);

	void drawblips(int x, int y, int s, int type, bool skipenemy = false)
	{
#ifdef BFRONTIER // moved data
		const char *textures[3] = {"packages/textures/blip_red.png", "packages/textures/blip_grey.png", "packages/textures/blip_blue.png"};
#else
		const char *textures[3] = {"data/blip_red.png", "data/blip_grey.png", "data/blip_blue.png"};
#endif
		settexture(textures[max(type+1, 0)]);
		glBegin(GL_QUADS);
		float scale = radarscale<=0 || radarscale>maxradarscale() ? maxradarscale() : radarscale;
		loopv(bases)
		{
			baseinfo &b = bases[i];
			if(skipenemy && b.enemy[0]) continue;
			switch(type)
			{
				case 1: if(!b.owner[0] || strcmp(b.owner, cl.player1->team)) continue; break;
				case 0: if(b.owner[0]) continue; break;
				case -1: if(!b.owner[0] || !strcmp(b.owner, cl.player1->team)) continue; break;
				case -2: if(!b.enemy[0] || !strcmp(b.enemy, cl.player1->team)) continue; break;
			} 
			vec dir(b.o);
			dir.sub(cl.player1->o);
			dir.z = 0.0f;
			float dist = dir.magnitude();
			if(dist >= scale) dir.mul(scale/dist);
			dir.rotate_around_z(-cl.player1->yaw*RAD);
			drawradar(x + s*0.5f*0.95f*(1.0f+dir.x/scale), y + s*0.5f*0.95f*(1.0f+dir.y/scale), 0.05f*s);
		}
		glEnd();
	}
	
	void capturehud(int w, int h)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#ifdef BFRONTIER // moved data, extended hud
		int x = 1800*w/h*1/80, y = 1800*(hidehud || hidestats ? 28 : 27)/40, s = 1800*w/h*5/40;
		glColor4f(1, 1, 1, hudblend*0.01f);
		settexture("packages/textures/radar.png");
#else
        int x = 1800*w/h*34/40, y = 1800*1/40, s = 1800*w/h*5/40;
        glColor3f(1, 1, 1);
		settexture("data/radar.png");
#endif
		glBegin(GL_QUADS);
		drawradar(float(x), float(y), float(s));
		glEnd();
		bool showenemies = cl.lastmillis%1000 >= 500;
		drawblips(x, y, s, 1, showenemies);
		drawblips(x, y, s, 0, showenemies);
		drawblips(x, y, s, -1, showenemies);
		if(showenemies) drawblips(x, y, s, -2);
		if(cl.player1->state == CS_DEAD)
		{
			glPushMatrix();
			glLoadIdentity();
			glOrtho(0, w*900/h, 900, 0, -1, 1);
#ifdef BFRONTIER
            int wait = max(0, (m_insta(cl.gamemode, cl.mutators) ? RESPAWNSECS/2 : RESPAWNSECS)-(cl.lastmillis-cl.player1->lastpain)/1000);
#else
            int gamemode = cl.gamemode, wait = max(0, (m_noitemsrail ? RESPAWNSECS/2 : RESPAWNSECS)-(cl.lastmillis-cl.player1->lastpain)/1000);
#endif
            draw_textf("%d", (x+s/2)/2-(wait>=10 ? 28 : 16), (y+s/2)/2-32, wait);
			glPopMatrix();
		}
        glDisable(GL_BLEND);
	}

	void setupbases()
	{
		reset();
		loopv(cl.et.ents)
		{
			extentity *e = cl.et.ents[i];
			if(e->type!=BASE) continue; 
			baseinfo &b = bases.add();
			b.o = e->o;
#ifdef BFRONTIER
			s_sprintfd(alias)("base_%d", e->attr1);
#else
			b.ammotype = e->attr1;
			s_sprintfd(alias)("base_%d", e->attr2);
#endif
			const char *name = getalias(alias);
			if(name[0]) s_strcpy(b.name, name); else s_sprintf(b.name)("base %d", bases.length());
			b.ent = e;
		}
		vec center(0, 0, 0);
		loopv(bases) center.add(bases[i].o);
		center.div(bases.length());
		radarscale = 0;
		loopv(bases) radarscale = max(radarscale, 2*center.dist(bases[i].o));
	}
			
	void sendbases(ucharbuf &p)
	{
		putint(p, SV_BASES);
		loopv(bases)
		{
			baseinfo &b = bases[i];
#ifndef BFRONTIER
			putint(p, max(b.ammotype, 0));
#endif
			putint(p, int(b.o.x*DMF));
			putint(p, int(b.o.y*DMF));
			putint(p, int(b.o.z*DMF));
		}
		putint(p, -1);
	}

#ifdef BFRONTIER
	void updatebase(int i, const char *owner, const char *enemy, int converted)
#else
	void updatebase(int i, const char *owner, const char *enemy, int converted, int ammo)
#endif
	{
		if(!bases.inrange(i)) return;
		baseinfo &b = bases[i];
		if(owner[0])
		{
			if(strcmp(b.owner, owner)) 
			{ 
				conoutf("\f2%s captured %s", owner, b.name); 
				if(!strcmp(owner, cl.player1->team)) playsound(S_V_BASECAP); 
			}
		}
		else if(b.owner[0]) 
		{ 
			conoutf("\f2%s lost %s", b.owner, b.name); 
			if(!strcmp(b.owner, cl.player1->team)) playsound(S_V_BASELOST); 
		}
		s_strcpy(b.owner, owner);
		s_strcpy(b.enemy, enemy);
		b.converted = converted;
#ifndef BFRONTIER
		if(ammo>b.ammo) playsound(S_ITEMSPAWN, &b.o);
		b.ammo = ammo;
#endif
	}

	void setscore(const char *team, int total)
	{
		findscore(team).total = total;
		if(total>=10000) conoutf("team %s captured all bases", team);
	}

	int closesttoenemy(const char *team, bool noattacked = false)
	{
		float bestdist = 1e10f;
		int best = -1;
		int attackers = INT_MAX, attacked = -1;
		loopv(bases)
		{
			baseinfo &b = bases[i];
			if(!b.owner[0] || strcmp(b.owner, team)) continue;
			if(noattacked && b.enemy[0]) continue;
			float dist = disttoenemy(b);
			if(dist < bestdist)
			{
				best = i;
				bestdist = dist;
			}
			else if(b.enemy[0] && b.enemies < attackers)
			{
				attacked = i;
				attackers = b.enemies; 
			}
		}
		if(best < 0) return attacked;
		return best;
	}

	int pickspawn(const char *team)
	{
		int closest = closesttoenemy(team, true);
		if(closest < 0) closest = closesttoenemy(team, false);
		if(closest < 0) return -1;
		baseinfo &b = bases[closest];

		float bestdist = 1e10f;
		int best = -1;
		loopv(cl.et.ents)
		{
			extentity *e = cl.et.ents[i];
			if(e->type!=PLAYERSTART) continue;
			float dist = e->o.dist(b.o);
			if(dist < bestdist)
			{
				best = i;
				bestdist = dist;
			}
		}
		return best;
	}
};

#else

struct captureservmode : capturestate, servmode
{
	int scoresec;
	bool notgotbases;
 
	captureservmode(fpsserver &sv) : servmode(sv), scoresec(0), notgotbases(false) {}

	void reset(bool empty)
	{
		capturestate::reset();
		scoresec = 0;
		notgotbases = !empty;
	}

	void stealbase(int n, const char *team)
	{
		baseinfo &b = bases[n];
		loopv(sv.clients)
		{
			fpsserver::clientinfo *ci = sv.clients[i];
			if(!ci->spectator && ci->state.state==CS_ALIVE && ci->team[0] && !strcmp(ci->team, team) && insidebase(b, ci->state.o))
				b.enter(ci->team);
		}
		sendbaseinfo(n);
	}

#ifndef BFRONTIER
	void replenishammo(clientinfo *ci)
	{
		int gamemode = sv.gamemode;
		if(m_noitemsrail || notgotbases || ci->state.state!=CS_ALIVE || !ci->team[0]) return;
		loopv(bases)
		{
			baseinfo &b = bases[i];
			if(b.ammotype>0 && b.ammotype<=I_CARTRIDGES-I_SHELLS+1 && insidebase(b, ci->state.o) && !ci->state.hasmaxammo(b.ammotype-1+I_SHELLS) && b.takeammo(ci->team))
			{
				sendbaseinfo(i);
				sendf(ci->clientnum, 1, "rii", SV_REPAMMO, b.ammotype);
				ci->state.addammo(b.ammotype);
				break;
			}
		}
	}
#endif

	void movebases(const char *team, const vec &oldpos, const vec &newpos)
	{
		if(!team[0] || sv.minremain<0) return;
		loopv(bases)
		{
			baseinfo &b = bases[i];
			bool leave = insidebase(b, oldpos),
				 enter = insidebase(b, newpos);
			if(leave && !enter && b.leave(team)) sendbaseinfo(i);
			else if(enter && !leave && b.enter(team)) sendbaseinfo(i);
			else if(leave && enter && b.steal(team)) stealbase(i, team);
		}
	}

	void leavebases(const char *team, const vec &o)
	{
		movebases(team, o, vec(-1e10f, -1e10f, -1e10f));
	}
	
	void enterbases(const char *team, const vec &o)
	{
		movebases(team, vec(-1e10f, -1e10f, -1e10f), o);
	}
	
	void addscore(const char *team, int n)
	{
		if(!n) return;
		score &cs = findscore(team);
		cs.total += n;
		sendf(-1, 1, "risi", SV_TEAMSCORE, team, cs.total);
	}

	void update()
	{
		if(sv.minremain<0) return;
		endcheck();
		int t = sv.gamemillis/1000 - (sv.gamemillis-sv.curtime)/1000;
		if(t<1) return;
#ifndef BFRONTIER
		int gamemode = sv.gamemode;
#endif
		loopv(bases)
		{
			baseinfo &b = bases[i];
			if(b.enemy[0])
			{
#ifdef BFRONTIER
                if(b.occupy(b.enemy, (m_insta(sv.gamemode, sv.mutators) ? OCCUPYPOINTS*2 : OCCUPYPOINTS)*b.enemies*t)==1) addscore(b.owner, CAPTURESCORE);
#else
                if(b.occupy(b.enemy, (m_noitemsrail ? OCCUPYPOINTS*2 : OCCUPYPOINTS)*b.enemies*t)==1) addscore(b.owner, CAPTURESCORE);
#endif
				sendbaseinfo(i);
			}
			else if(b.owner[0])
			{
				b.capturetime += t;
#ifdef BFRONTIER
				int score = b.capturetime/SCORESECS - (b.capturetime-t)/SCORESECS;
				if(score) addscore(b.owner, score);
				sendbaseinfo(i);
#else
				int score = b.capturetime/SCORESECS - (b.capturetime-t)/SCORESECS,
					ammo = b.capturetime/AMMOSECS - (b.capturetime-t)/AMMOSECS;
				if(score) addscore(b.owner, score);
				if(!m_noitemsrail && b.addammo(ammo)) sendbaseinfo(i);
#endif
			}
		}
	}

	void sendbaseinfo(int i)
	{
		baseinfo &b = bases[i];
#ifdef BFRONTIER
		sendf(-1, 1, "riiiss", SV_BASEINFO, i, b.enemy[0] ? b.converted : 0, b.owner, b.enemy);
#else
		sendf(-1, 1, "riissii", SV_BASEINFO, i, b.owner, b.enemy, b.enemy[0] ? b.converted : 0, b.owner[0] ? b.ammo : 0);
#endif
	}

	void sendbases()
	{
		ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
		ucharbuf p(packet->data, packet->dataLength);
		initclient(NULL, p, false);
		enet_packet_resize(packet, p.length());
		sendpacket(-1, 1, packet);
		if(!packet->referenceCount) enet_packet_destroy(packet);
	}

	void initclient(clientinfo *ci, ucharbuf &p, bool connecting)
	{
		if(connecting) loopv(scores)
		{
			score &cs = scores[i];
			putint(p, SV_TEAMSCORE);
			sendstring(cs.team, p);
			putint(p, cs.total);
		}
		putint(p, SV_BASES);
		loopv(bases)
		{
			baseinfo &b = bases[i];
#ifdef BFRONTIER
			putint(p, b.converted);
#else
			putint(p, min(max(b.ammotype, 1), I_CARTRIDGES+1));
#endif
			sendstring(b.owner, p);
			sendstring(b.enemy, p);
#ifndef BFRONTIER
			putint(p, b.converted);
			putint(p, b.ammo);
#endif
		}
		putint(p, -1);
	}

	void endcheck()
	{
		const char *lastteam = NULL;

		loopv(bases)
		{
			baseinfo &b = bases[i];
			if(b.owner[0])
			{
				if(!lastteam) lastteam = b.owner;
				else if(strcmp(lastteam, b.owner))
				{
					lastteam = false;
					break;
				}
			}
			else
			{
				lastteam = false;
				break;
			}
		}

		if(!lastteam) return;
		findscore(lastteam).total = 10000;
		sendf(-1, 1, "risi", SV_TEAMSCORE, lastteam, 10000);
		sv.startintermission(); 
	}

	void entergame(clientinfo *ci) 
	{
		if(notgotbases) return;
		enterbases(ci->team, ci->state.o);
	}		

	void spawned(clientinfo *ci)
	{
		if(notgotbases) return;
		enterbases(ci->team, ci->state.o);
	}

	void leavegame(clientinfo *ci)
	{
		if(notgotbases) return;
		leavebases(ci->team, ci->state.o);
	}

	void died(clientinfo *ci, clientinfo *actor)
	{
		if(notgotbases) return;
		leavebases(ci->team, ci->state.o);
	}

	void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
	{
		if(notgotbases) return;
		movebases(ci->team, oldpos, newpos);
	}

	void changeteam(clientinfo *ci, const char *oldteam, const char *newteam)
	{
		if(notgotbases) return;
		leavebases(oldteam, ci->state.o);
		enterbases(newteam, ci->state.o);
	}

	void parsebases(ucharbuf &p)
	{
#ifdef BFRONTIER
		int x = 0;
		while((x = getint(p))>=0)
		{
			vec o;
			o.x = x/DMF;
			o.y = getint(p)/DMF;
			o.z = getint(p)/DMF;
			if(notgotbases) addbase(o);
		}
#else
        int ammotype;
		while((ammotype = getint(p))>=0)
		{
			vec o;
			o.x = getint(p)/DMF;
			o.y = getint(p)/DMF;
			o.z = getint(p)/DMF;
			if(notgotbases) addbase(ammotype, o);
		}
#endif
		if(notgotbases)
		{
			notgotbases = false;
			sendbases();
			loopv(sv.clients) if(sv.clients[i]->state.state==CS_ALIVE) entergame(sv.clients[i]);
		}
	}
};

#endif

