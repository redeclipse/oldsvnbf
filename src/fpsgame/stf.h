// stf.h: client and server state for stf gamemode

struct stfstate
{
	static const int OCCUPYPOINTS = 15;
	static const int OCCUPYLIMIT = 100;
	static const int SECURESCORE = 1;
	static const int SCORESECS = 10;
	static const int RESPAWNSECS = 3;

	struct flaginfo
	{
		vec o;
		int owner, enemy;
#ifndef STFSERV
        vec pos;
		string name, info;
		extentity *ent;
#endif
		int owners, enemies, converted, securetime;

		flaginfo() { reset(); }

		void noenemy()
		{
			enemy = TEAM_NEUTRAL;
			enemies = 0;
			converted = 0;
		}

		void reset()
		{
			noenemy();
			owner = TEAM_NEUTRAL;
			securetime = -1;
            owners = 0;
		}

		bool enter(int team)
		{
            if(owner == team)
			{
                owners++;
                return false;
            }
            if(!enemies)
            {
                if(enemy != team)
                {
                    converted = 0;
					enemy = team;
                }
				enemies++;
				return true;
			}
			else if(enemy != team) return false;
			else enemies++;
			return false;
		}

		bool steal(int team)
		{
            return !enemies && owner != team;
		}

		bool leave(int team)
		{
            if(owner == team)
            {
                owners--;
                return false;
            }
			if(enemy != team) return false;
			enemies--;
			return !enemies;
		}

		int occupy(int team, int units)
		{
			if(enemy != team) return -1;
			converted += units;
            if(units<0)
            {
                if(converted<=0) noenemy();
                return -1;
            }
            else if(converted<(owner ? 2 : 1)*OCCUPYLIMIT) return -1;
			if(owner) { owner = TEAM_NEUTRAL; converted = 0; enemy = team; return 0; }
            else { owner = team; securetime = 0; owners = enemies; noenemy(); return 1; }
        }
	};

	vector<flaginfo> flags;

	struct score
	{
		int team, total;
	};

	vector<score> scores;

	int secured;

	stfstate() : secured(0) {}

	void reset()
	{
		flags.setsize(0);
		scores.setsize(0);
		secured = 0;
	}

	score &findscore(int team)
	{
		loopv(scores)
		{
			score &cs = scores[i];
			if(cs.team == team) return cs;
		}
		score &cs = scores.add();
		cs.team = team;
		cs.total = 0;
		return cs;
	}

	void addflag(const vec &o)
	{
		flaginfo &b = flags.add();
		b.o = o;
	}

	void initflag(int i, int owner, int enemy, int converted)
	{
		if(!flags.inrange(i)) return;
		flaginfo &b = flags[i];
		b.owner = owner;
		b.enemy = enemy;
		b.converted = converted;
	}

	bool hasflags(int team)
	{
		loopv(flags)
		{
			flaginfo &b = flags[i];
			if(b.owner && b.owner == team) return true;
		}
		return false;
	}

	float disttoenemy(flaginfo &b)
	{
		float dist = 1e10f;
		loopv(flags)
		{
			flaginfo &e = flags[i];
			if(e.owner && b.owner != e.owner)
				dist = min(dist, b.o.dist(e.o));
		}
		return dist;
	}

	bool insideflag(const flaginfo &b, const vec &o)
	{
		float dx = (b.o.x-o.x), dy = (b.o.y-o.y), dz = (b.o.z-o.z+14);
		return dx*dx + dy*dy <= enttype[FLAG].radius*enttype[FLAG].radius && fabs(dz) <= enttype[FLAG].height;
	}
};

#ifndef STFSERV

struct stfclient : stfstate
{
    static const int FIREBALLRADIUS = 5;

	GAMECLIENT &cl;

    IVARP(securetether, 0, 1, 1);

	stfclient(GAMECLIENT &cl) : cl(cl)
	{
	}

    void rendertether(fpsent *d)
    {
        int oldflag = d->lastflag;
        d->lastflag = -1;
        vec pos(d->o.x, d->o.y, d->o.z + (d->aboveeye - d->height)/2);
        if(d->state==CS_ALIVE)
        {
            loopv(flags)
            {
                flaginfo &b = flags[i];
                if(!insideflag(b, d->o) || (b.owner != d->team && b.enemy != d->team)) continue;
                part_flare(pos, b.pos, 1, 15, teamtype[d->team].colour, 0.28f, d);
                if(oldflag < 0)
					regularshape(4, 16, teamtype[d->team].colour, 256, 5, 1000, pos, 4.8f);
                d->lastflag = i;
            }
        }
        if(d->lastflag < 0 && oldflag >= 0)
			regularshape(4, 16, teamtype[d->team].colour, 256, 5, 1000, pos, 4.8f);
    }

    void preload()
    {
        loopi(TEAM_MAX) loadmodel(teamtype[i].flag, -1, true);
    }

	void render()
	{
        extern bool shadowmapping;
        if(securetether() && !shadowmapping)
        {
            loopv(cl.players)
            {
                fpsent *d = cl.players[i];
                if(d) rendertether(d);
            }
            rendertether(cl.player1);
        }

		loopv(flags)
		{
			flaginfo &b = flags[i];
			const char *flagname = teamtype[b.owner].flag;
            rendermodel(&b.ent->light, flagname, ANIM_MAPMODEL|ANIM_LOOP, b.o, 0, 0, 0, MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_OCCLUDED);
			int attack = b.enemy ? b.enemy : b.owner;
			if(b.enemy && b.owner)
				s_sprintf(b.info)("\fs%s%s\fS vs. \fs%s%s\fS", teamtype[b.owner].colour, teamtype[b.owner].name, teamtype[b.enemy].colour, teamtype[b.enemy].name);
			else if(attack) s_sprintf(b.info)("%s", teamtype[attack].name);
			else b.info[0] = '\0';

            vec above(b.pos);
            above.z += FIREBALLRADIUS+1.0f;
			part_text(above, b.info, 10, 1, 0xFFFFDD);
			if(attack)
			{
				above.z += 6.0f;
				part_meter(above, b.converted/float((b.owner ? 2 : 1) * OCCUPYLIMIT), 11, 1, teamtype[attack].colour);
			}
		}
	}

	void drawblips(int x, int y, int s, int type, bool skipenemy = false)
	{
		settexture(cl.bliptex());
		switch(max(type+1, 0))
		{
			case 2: glColor4f(0.f, 0.f, 1.f, 1.f); break;
			case 1: glColor4f(1.f, 1.f, 1.f, 1.f); break;
			case 0:
			default: glColor4f(1.f, 0.f, 0.f, 1.f); break;
		}
		glBegin(GL_QUADS);
		loopv(flags)
		{
			flaginfo &b = flags[i];
			if(skipenemy && b.enemy) continue;
			switch(type)
			{
				case 1: if(!b.owner || b.owner != cl.player1->team) continue; break;
				case 0: if(b.owner) continue; break;
				case -1: if(!b.owner || b.owner == cl.player1->team) continue; break;
				case -2: if(!b.enemy || b.enemy == cl.player1->team) continue; break;
			}
			vec dir(b.o);
			dir.sub(camera1->o);
			dir.z = 0.0f;
			float dist = dir.magnitude();
			if(dist >= cl.radarrange()) dir.mul(cl.radarrange()/dist);
			dir.rotate_around_z(-camera1->yaw*RAD);
			cl.drawradar(x + s*0.5f*0.95f*(1.0f+dir.x/cl.radarrange()), y + s*0.5f*0.95f*(1.0f+dir.y/cl.radarrange()), 0.1f*s);
		}
		glEnd();
	}

    int respawnwait(fpsent *d)
    {
        return max(0, (m_insta(cl.gamemode, cl.mutators) ? RESPAWNSECS/2 : RESPAWNSECS)-(lastmillis-d->lastpain)/1000);
    }

	void drawhud(int w, int h)
	{
		int x = FONTH/4, s = h/5, y = h-s-(FONTH/4);
		bool showenemies = lastmillis%1000 >= 500;
		drawblips(x, y, s, 1, showenemies);
		drawblips(x, y, s, 0, showenemies);
		drawblips(x, y, s, -1, showenemies);
		if(showenemies) drawblips(x, y, s, -2);
	}

	void setupflags()
	{
		reset();
		loopv(cl.et.ents)
		{
			extentity *e = cl.et.ents[i];
			if(e->type!=FLAG) continue;
			flaginfo &b = flags.add();
			b.o = e->o;
            b.pos = b.o;
            abovemodel(b.pos, teamtype[TEAM_NEUTRAL].flag);
            b.pos.z += FIREBALLRADIUS-2;
			s_sprintfd(alias)("flag_%d", e->attr1);
			const char *name = getalias(alias);
			if(name[0]) s_strcpy(b.name, name);
			else s_sprintf(b.name)("flag %d", flags.length());
			b.ent = e;
		}
		vec center(0, 0, 0);
		loopv(flags) center.add(flags[i].o);
		center.div(flags.length());
	}

	void sendflags(ucharbuf &p)
	{
		putint(p, SV_FLAGS);
		loopv(flags)
		{
			flaginfo &b = flags[i];
			putint(p, int(b.o.x*DMF));
			putint(p, int(b.o.y*DMF));
			putint(p, int(b.o.z*DMF));
		}
		putint(p, -1);
	}

	void updateflag(int i, int owner, int enemy, int converted)
	{
		if(!flags.inrange(i)) return;
		flaginfo &b = flags[i];
		if(owner)
		{
			if(b.owner != owner)
			{
				conoutf("\f2%s secured %s", teamtype[owner].name, b.name);
				if(owner == cl.player1->team) cl.et.announce(S_V_FLAGSECURED);
			}
		}
		else if(b.owner)
		{
			conoutf("\f2%s overthrew %s", teamtype[b.owner].name, b.name);
			if(b.owner == cl.player1->team) cl.et.announce(S_V_FLAGOVERTHROWN);
		}
        if(b.owner != owner) particle_splash(0, 200, 250, b.pos);
		b.owner = owner;
		b.enemy = enemy;
		b.converted = converted;
	}

	void setscore(int team, int total)
	{
		findscore(team).total = total;
		if(total>=10000) conoutf("team %s secured all flags", teamtype[team].name);
	}

    int closesttoenemy(int team, bool noattacked = false, bool farthest = false)
	{
        float bestdist = farthest ? -1e10f : 1e10f;
		int best = -1;
		int attackers = INT_MAX, attacked = -1;
		loopv(flags)
		{
			flaginfo &b = flags[i];
			if(!b.owner || b.owner != team) continue;
			if(noattacked && b.enemy) continue;
			float dist = disttoenemy(b);
            if(farthest ? dist > bestdist : dist < bestdist)
			{
				best = i;
				bestdist = dist;
			}
			else if(b.enemy && b.enemies < attackers)
			{
				attacked = i;
				attackers = b.enemies;
			}
		}
		if(best < 0) return attacked;
		return best;
	}

	int pickspawn(int team)
	{
		int closest = closesttoenemy(team, true);
		if(closest < 0) closest = closesttoenemy(team, false);
		if(closest < 0) return -1;
		flaginfo &b = flags[closest];

        float bestdist = 1e10f, altdist = 1e10f;
        int best = -1, alt = -1;
		loopv(cl.et.ents)
		{
			extentity *e = cl.et.ents[i];
			if(e->type!=PLAYERSTART) continue;
			float dist = e->o.dist(b.o);
			if(dist < bestdist)
			{
                alt = best;
                altdist = bestdist;
				best = i;
				bestdist = dist;
			}
            else if(dist < altdist)
            {
                alt = i;
                altdist = dist;
            }
		}
        return rnd(2) ? best : alt;
	}
} stf;

#else

struct stfservmode : stfstate, servmode
{
	int scoresec;
	bool notgotflags;

	stfservmode(GAMESERVER &sv) : servmode(sv), scoresec(0), notgotflags(false) {}

	void reset(bool empty)
	{
		stfstate::reset();
		scoresec = 0;
		notgotflags = !empty;
	}

	void stealflag(int n, int team)
	{
		flaginfo &b = flags[n];
		loopv(sv.clients)
		{
			GAMESERVER::clientinfo *ci = sv.clients[i];
			if(!ci->spectator && ci->state.state==CS_ALIVE && ci->team && ci->team == team && insideflag(b, ci->state.o))
				b.enter(ci->team);
		}
		sendflaginfo(n);
	}

	void moveflags(int team, const vec &oldpos, const vec &newpos)
	{
		if(!team || sv.minremain<0) return;
		loopv(flags)
		{
			flaginfo &b = flags[i];
			bool leave = insideflag(b, oldpos),
				 enter = insideflag(b, newpos);
			if(leave && !enter && b.leave(team)) sendflaginfo(i);
			else if(enter && !leave && b.enter(team)) sendflaginfo(i);
			else if(leave && enter && b.steal(team)) stealflag(i, team);
		}
	}

	void leaveflags(int team, const vec &o)
	{
		moveflags(team, o, vec(-1e10f, -1e10f, -1e10f));
	}

	void enterflags(int team, const vec &o)
	{
		moveflags(team, vec(-1e10f, -1e10f, -1e10f), o);
	}

	void addscore(int team, int n)
	{
		if(!n) return;
		score &cs = findscore(team);
		cs.total += n;
		sendf(-1, 1, "ri3", SV_TEAMSCORE, team, cs.total);
	}

	void update()
	{
		if(sv.minremain<0) return;
		endcheck();
		int t = sv.gamemillis/1000 - (sv.gamemillis-curtime)/1000;
		if(t<1) return;
		loopv(flags)
		{
			flaginfo &b = flags[i];
			if(b.enemy)
			{
                if((!b.owners || !b.enemies) && b.occupy(b.enemy, (m_insta(sv.gamemode, sv.mutators) ? OCCUPYPOINTS*2 : OCCUPYPOINTS)*(b.enemies ? b.enemies : -(1+b.owners))*t)==1) addscore(b.owner, SECURESCORE);
				sendflaginfo(i);
			}
			else if(b.owner)
			{
				b.securetime += t;
				int score = b.securetime/SCORESECS - (b.securetime-t)/SCORESECS;
				if(score) addscore(b.owner, score);
				sendflaginfo(i);
			}
		}
	}

	void sendflaginfo(int i)
	{
		flaginfo &b = flags[i];
		sendf(-1, 1, "ri5", SV_FLAGINFO, i, b.enemy ? b.converted : 0, b.owner, b.enemy);
	}

	void sendflags()
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
		if(connecting)
        {
            loopv(scores)
		    {
			    score &cs = scores[i];
			    putint(p, SV_TEAMSCORE);
			    putint(p, cs.team);
			    putint(p, cs.total);
		    }
        }
		putint(p, SV_FLAGS);
		loopv(flags)
		{
			flaginfo &b = flags[i];
			putint(p, b.converted);
			putint(p, b.owner);
			putint(p, b.enemy);
		}
		putint(p, -1);
	}

	void endcheck()
	{
		int lastteam = NULL;

		loopv(flags)
		{
			flaginfo &b = flags[i];
			if(b.owner)
			{
				if(!lastteam) lastteam = b.owner;
				else if(lastteam != b.owner)
				{
					lastteam = TEAM_NEUTRAL;
					break;
				}
			}
			else
			{
				lastteam = TEAM_NEUTRAL;
				break;
			}
		}

		if(!lastteam) return;
		findscore(lastteam).total = 10000;
		sendf(-1, 1, "ri3", SV_TEAMSCORE, lastteam, 10000);
		sv.startintermission();
	}

	void entergame(clientinfo *ci)
	{
        if(notgotflags || ci->state.state!=CS_ALIVE) return;
		enterflags(ci->team, ci->state.o);
	}

	void spawned(clientinfo *ci)
	{
		if(notgotflags) return;
		enterflags(ci->team, ci->state.o);
	}

    void leavegame(clientinfo *ci, bool disconnecting = false)
	{
        if(notgotflags || ci->state.state!=CS_ALIVE) return;
		leaveflags(ci->team, ci->state.o);
	}

	void died(clientinfo *ci, clientinfo *actor)
	{
		if(notgotflags) return;
		leaveflags(ci->team, ci->state.o);
	}

	void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
	{
		if(notgotflags) return;
		moveflags(ci->team, oldpos, newpos);
	}

	void changeteam(clientinfo *ci, int oldteam, int newteam)
	{
		if(notgotflags) return;
		leaveflags(oldteam, ci->state.o);
		enterflags(newteam, ci->state.o);
	}

	void parseflags(ucharbuf &p)
	{
		int x = 0;
		while((x = getint(p))>=0)
		{
			vec o;
			o.x = x/DMF;
			o.y = getint(p)/DMF;
			o.z = getint(p)/DMF;
			if(notgotflags) addflag(o);
		}
		if(notgotflags)
		{
			notgotflags = false;
			sendflags();
			loopv(sv.clients) if(sv.clients[i]->state.state==CS_ALIVE) entergame(sv.clients[i]);
		}
	}
} stfmode;

#endif

