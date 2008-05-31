struct ctfstate
{
    static const int FLAGRADIUS = 16;

    struct flag
    {
        vec droploc, spawnloc;
        int team, score, droptime;
#ifdef CTFSERV
        int owner;
#else
        bool pickup;
        fpsent *owner;
        extentity *ent;
        vec interploc;
        float interpangle;
        int interptime;
#endif

        flag() { reset(); }

        void reset()
        {
            droploc = spawnloc = vec(0, 0, 0);
#ifdef CTFSERV
            owner = -1;
#else
            pickup = false;
            owner = NULL;
            interploc = vec(0, 0, 0);
            interpangle = 0;
            interptime = 0;
#endif
            team = -1;
            score = 0;
            droptime = 0;
        }
    };
    vector<flag> flags;

    void reset()
    {
        flags.setsize(0);
    }

    void addflag(int i, const vec &o, int team)
    {
    	while(!flags.inrange(i)) flags.add();
		flag &f = flags[i];
		f.reset();
		f.team = team;
		f.spawnloc = o;
    }

#ifdef CTFSERV
    void takeflag(int i, int owner)
#else
    void takeflag(int i, fpsent *owner)
#endif
    {
		flag &f = flags[i];
		f.owner = owner;
#ifndef CTFSERV
		f.pickup = false;
#endif
    }

    void dropflag(int i, const vec &o, int droptime)
    {
		flag &f = flags[i];
		f.droploc = o;
		f.droptime = droptime;
#ifdef CTFSERV
		f.owner = -1;
#else
		f.pickup = false;
		f.owner = NULL;
#endif
    	}

    void returnflag(int i)
    {
		flag &f = flags[i];
		f.droptime = 0;
#ifdef CTFSERV
		f.owner = -1;
#else
		f.pickup = false;
		f.owner = NULL;
#endif
    }

	int numteams(bool multi)
	{
		return multi ? TEAM_MAX : TEAM_MAX/2;
	}

	int teamflag(const char *s, bool multi)
	{
		loopi(numteams(multi))
		{
			if (!strcmp(s, teamnames[i])) return i;
		}
		return -1;
	}

	const char *flagteam(int t, bool multi)
	{
		if (t<=0 || t>=numteams(multi)) return teamnames[0];
		return teamnames[t];
	}
};

#ifdef CTFSERV
struct ctfservmode : ctfstate, servmode
{
    static const int RESETFLAGTIME = 10000;

    bool notgotflags;

    ctfservmode(GAMESERVER &sv) : servmode(sv), notgotflags(false) {}

    void reset(bool empty)
    {
        ctfstate::reset();
        notgotflags = !empty;
    }

    void dropflag(clientinfo *ci)
    {
        if(notgotflags) return;
        loopv(flags) if(flags[i].owner==ci->clientnum)
        {
            ivec o(vec(ci->state.o).mul(DMF));
            sendf(-1, 1, "ri6", SV_DROPFLAG, ci->clientnum, i, o.x, o.y, o.z);
            ctfstate::dropflag(i, o.tovec().div(DMF), sv.gamemillis);
        }
    }

    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        dropflag(ci);
    }

    void died(clientinfo *ci, clientinfo *actor)
    {
        dropflag(ci);
    }

    bool canchangeteam(clientinfo *ci, const char *oldteam, const char *newteam)
    {
    	loopi(m_multi(sv.gamemode, sv.mutators) ? TEAM_MAX : TEAM_MAX/2)
    	{
			if (!strcmp(newteam, teamnames[i])) return true;
    	}
    	return false;
    }

    void changeteam(clientinfo *ci, const char *oldteam, const char *newteam)
    {
        dropflag(ci);
    }

    void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
    {
        if(notgotflags) return;
        static const dynent dummy;
        vec o(newpos);
        o.z -= dummy.height;
        loopv(flags)
        {
            flag &relay = flags[i];
            loopvk(flags)
            {
				flag &goal = flags[k];

				if(relay.owner==ci->clientnum && goal.team==teamflag(ci->team, m_multi(sv.gamemode, sv.mutators)) && goal.owner<0 && !goal.droptime && o.dist(goal.spawnloc) < FLAGRADIUS)
				{
					returnflag(i);
					goal.score++;
					sendf(-1, 1, "ri5", SV_SCOREFLAG, ci->clientnum, i, k, goal.score);
				}
            }
        }
    }

    void takeflag(clientinfo *ci, int i)
    {
        if(notgotflags || !flags.inrange(i) || ci->state.state!=CS_ALIVE || !ci->team[0]) return;
		flag &f = flags[i];
		if(f.team == teamflag(ci->team, m_multi(sv.gamemode, sv.mutators)))
		{
			if(!f.droptime || f.owner>=0) return;
			ctfstate::returnflag(i);
			sendf(-1, 1, "ri3", SV_RETURNFLAG, ci->clientnum, i);
		}
		else
		{
			if(f.owner>=0) return;
			ctfstate::takeflag(i, ci->clientnum);
			sendf(-1, 1, "ri3", SV_TAKEFLAG, ci->clientnum, i);
		}
    }

    void update()
    {
        if(sv.minremain<0 || notgotflags) return;
        loopv(flags)
        {
            flag &f = flags[i];
            if(f.owner<0 && f.droptime && sv.gamemillis - f.droptime >= RESETFLAGTIME)
            {
                returnflag(i);
                sendf(-1, 1, "ri2", SV_RESETFLAG, i);
            }
        }
    }

    void initclient(clientinfo *ci, ucharbuf &p, bool connecting)
    {
        putint(p, SV_INITFLAGS);
        putint(p, flags.length());
        loopv(flags)
        {
            flag &f = flags[i];
            putint(p, f.team);
            putint(p, f.score);
            putint(p, f.owner);
            if(f.owner<0)
            {
                putint(p, f.droptime ? 1 : 0);
                if(f.droptime)
                {
                    putint(p, int(f.droploc.x*DMF));
                    putint(p, int(f.droploc.y*DMF));
                    putint(p, int(f.droploc.z*DMF));
                }
            }
        }
    }

    void parseflags(ucharbuf &p)
    {
    	int numflags = getint(p);
        loopi(numflags)
        {
            int team = getint(p);
            vec o;
            loopk(3) o[k] = getint(p)/DMF;
            if(notgotflags) addflag(i, o, team);
        }
        notgotflags = false;
    }
} ctfmode;
#else
struct ctfclient : ctfstate
{
    static const int RESPAWNSECS = 3;

    GAMECLIENT &cl;

    ctfclient(GAMECLIENT &cl) : cl(cl)
    {
    }

    void preload()
    {
        static const char *flagmodels[2] = { "flags/red", "flags/blue" };
        loopi(2) loadmodel(flagmodels[i], -1, true);
    }

    void drawblips(int x, int y, int s, int i, bool flagblip)
    {
		flag &f = flags[i];
        settexture("textures/blip");
        if(f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators))) glColor4f(0.f, 0.f, 1.f, 1.f);
        else glColor4f(1.f, 0.f, 0.f, 1.f);
		vec dir;
        if(flagblip) dir = f.owner ? f.owner->o : (f.droptime ? f.droploc : f.spawnloc);
        else dir = f.spawnloc;
		dir.sub(camera1->o);
		dir.z = 0.0f;
        float size = flagblip ? 0.1f : 0.075f,
              xoffset = flagblip ? -2*(3/32.0f)*size : -size,
              yoffset = flagblip ? -2*(1 - 3/32.0f)*size : -size,
              dist = dir.magnitude();
        if(dist >= cl.radarrange()*(1 - 0.05f)) dir.mul(cl.radarrange()*(1 - 0.05f)/dist);
		dir.rotate_around_z(-camera1->yaw*RAD);
		glBegin(GL_QUADS);
        cl.drawradar(x + s*0.5f*(1.0f + dir.x/cl.radarrange() + xoffset), y + s*0.5f*(1.0f + dir.y/cl.radarrange() + yoffset), size*s);
		glEnd();
    }

    void drawhud(int w, int h)
    {
#if 0
        if(cl.player1->state == CS_ALIVE)
        {
            loopv(flags) if(flags[i].owner == cl.player1)
            {
                cl.drawicon(320, 0, 1820, 1650);
                break;
            }
        }
#endif

		int x = FONTH/4, s = h/5, y = h-s-(FONTH/4);
        loopv(flags)
        {
            flag &f = flags[i];
            if(!flagteam(f.team, m_multi(cl.gamemode, cl.mutators))) continue;
            drawblips(x, y, s, i, false);
            if(!f.ent) continue;
            if(f.owner)
            {
                if(lastmillis%1000 >= 500) continue;
            }
            else if(f.droptime && lastmillis%300 >= 150) continue;
            drawblips(x, y, s, i, true);
        }
    }

    vec interpflagpos(flag &f, float &angle)
    {
        vec pos = f.owner ? vec(f.owner->abovehead()).add(vec(0, 0, 1)) : (f.droptime ? f.droploc : f.spawnloc);
        angle = f.owner ? f.owner->yaw : (f.ent ? f.ent->attr1 : 0);
        if(pos.x < 0) return pos;
        if(f.interptime && f.interploc.x >= 0)
        {
            float t = min((lastmillis - f.interptime)/500.0f, 1.0f);
            pos.lerp(f.interploc, pos, t);
            angle += (1-t)*(f.interpangle - angle);
        }
        return pos;
    }

    vec interpflagpos(flag &f) { float angle; return interpflagpos(f, angle); }

    void render()
    {
        loopv(flags)
        {
            flag &f = flags[i];
            if(!f.ent || (!f.owner && f.droptime && f.droploc.x < 0)) continue;
            const char *flagname = f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators)) ? "flags/blue" : "flags/red";
            float angle;
            vec pos = interpflagpos(f, angle);
            rendermodel(!f.droptime && !f.owner ? &f.ent->light : NULL, flagname, ANIM_MAPMODEL|ANIM_LOOP,
                        interpflagpos(f), angle, 0,
                        MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_OCCLUDED | (f.droptime || f.owner ? MDL_LIGHT : 0));

            vec above(pos);
            above.z += enttype[BASE].height;
            s_sprintfd(info)("@%s flag", flagteam(f.team, m_multi(cl.gamemode, cl.mutators)));
			particle_text(above, info, f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators)) ? 16 : 13, 1);
        }
    }

    void setupflags()
    {
        reset();
        int x = 0;
        loopv(cl.et.ents)
        {
            extentity *e = cl.et.ents[i];
            if(e->type!=BASE || e->attr2<1 || e->attr2>numteams(m_multi(cl.gamemode, cl.mutators))) continue;
            addflag(x, e->o, e->attr2-1);
            flags[e->attr2-1].ent = e;
            x++;
        }
        vec center(0, 0, 0);
        loopv(flags) center.add(flags[i].spawnloc);
        center.div(flags.length());
    }

    void sendflags(ucharbuf &p)
    {
        putint(p, SV_INITFLAGS);
		putint(p, flags.length());
        loopv(flags)
        {
            flag &f = flags[i];
            putint(p, f.team);
            loopk(3) putint(p, int(f.spawnloc[k]*DMF));
        }
    }

    void parseflags(ucharbuf &p, bool commit)
    {
    	int numflags = getint(p);
        loopi(numflags)
        {
        	while (!flags.inrange(i)) flags.add();
            flag &f = flags[i];
            int team = getint(p), score = getint(p), owner = getint(p), dropped = 0;
            vec droploc(0, 0, 0);
            if(owner<0)
            {
                dropped = getint(p);
                if(dropped) loopk(3) droploc[k] = getint(p)/DMF;
            }
            if(commit)
            {
				f.team = team;
                f.score = score;
                f.owner = owner>=0 ? (owner==cl.player1->clientnum ? cl.player1 : cl.newclient(owner)) : NULL;
                f.droptime = dropped;
                f.droploc = dropped ? droploc : f.spawnloc;
                f.interptime = 0;

                if(dropped)
                {
                    if(!cl.ph.droptofloor(f.droploc, 2, 0)) f.droploc = vec(-1, -1, -1);
                }
            }
        }
    }

    void dropflag(fpsent *d, int i, const vec &droploc)
    {
		flag &f = flags[i];
		f.interploc = interpflagpos(f, f.interpangle);
		f.interptime = lastmillis;
		ctfstate::dropflag(i, droploc, 1);
		if(!cl.ph.droptofloor(f.droploc, 2, 0))
		{
			f.droploc = vec(-1, -1, -1);
			f.interptime = 0;
		}
		conoutf("%s dropped %s flag", d==cl.player1 ? "you" : cl.colorname(d), f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
		cl.et.announce(S_V_FLAGDROP);
    }

    void flagexplosion(int i, const vec &loc)
    {
		flag &f = flags[i];
		int ftype;
		vec color;
		if(f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators))) { ftype = 36; color = vec(0.25f, 0.25f, 1); }
		else { ftype = 35; color = vec(1, 0.25f, 0.25f); }
		particle_fireball(loc, 30, ftype);
		adddynlight(loc, 35, color, 900, 100);
		particle_splash(0, 150, 300, loc);
    }

    void flageffect(int i, const vec &from, const vec &to)
    {
		flag &f = flags[i];
		vec fromexp(from), toexp(to);
		if(from.x >= 0)
		{
			fromexp.z += 8;
			flagexplosion(i, fromexp);
		}
		if(to.x >= 0)
		{
			toexp.z += 8;
			flagexplosion(i, toexp);
		}
		if(from.x >= 0 && to.x >= 0)
			particle_flare(fromexp, toexp, 600, f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators)) ? 30 : 29);
    }

    void returnflag(fpsent *d, int i)
    {
		flag &f = flags[i];
		flageffect(i, interpflagpos(f), f.spawnloc);
		f.interptime = 0;
		ctfstate::returnflag(i);
		conoutf("%s returned %s flag", d==cl.player1 ? "you" : cl.colorname(d), f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
		cl.et.announce(S_V_FLAGRETURN);
    }

    void resetflag(int i)
    {
		flag &f = flags[i];
		flageffect(i, interpflagpos(f), f.spawnloc);
		f.interptime = 0;
		ctfstate::returnflag(i);
		conoutf("%s flag reset", f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
		cl.et.announce(S_V_FLAGRESET);
    }

	int findscore(const char *team)
	{
		int s = 0, t = teamflag(team, m_multi(cl.gamemode, cl.mutators));
		loopv(flags)
		{
			flag &f = flags[i];
			if (f.team == t) s += f.score;
		}
		return s;
	}

    void scoreflag(fpsent *d, int relay, int goal, int score)
    {
		flag &f = flags[goal];
		flageffect(goal, flags[goal].spawnloc, flags[relay].spawnloc);
		f.score = score;
		f.interptime = 0;
		ctfstate::returnflag(relay);
		if(d!=cl.player1)
		{
			s_sprintfd(ds)("@%d", score);
			particle_text(d->abovehead(), ds, 9);
		}
		conoutf("%s scored for %s team", d==cl.player1 ? "you" : cl.colorname(d), f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
		cl.et.announce(S_V_FLAGSCORE);
    }

    void takeflag(fpsent *d, int i)
    {
		flag &f = flags[i];
		f.interploc = interpflagpos(f, f.interpangle);
		f.interptime = lastmillis;
		conoutf("%s %s %s flag", d==cl.player1 ? "you" : cl.colorname(d), f.droptime ? "picked up" : "stole", f.team==teamflag(cl.player1->team, m_multi(cl.gamemode, cl.mutators)) ? "your" : "the enemy");
		ctfstate::takeflag(i, d);
		cl.et.announce(S_V_FLAGPICKUP);
    }

    void checkflags(fpsent *d)
    {
        vec o = d->o;
        o.z -= d->height;
        loopv(flags)
        {
            flag &f = flags[i];
            if(!f.ent || f.owner || (f.droptime ? f.droploc.x<0 : f.team==teamflag(d->team, m_multi(cl.gamemode, cl.mutators)))) continue;
            if(o.dist(f.droptime ? f.droploc : f.spawnloc) < FLAGRADIUS)
            {
                if(f.pickup) continue;
                cl.cc.addmsg(SV_TAKEFLAG, "ri", i);
                f.pickup = true;
            }
            else f.pickup = false;
       }
    }

    int respawnwait(fpsent *d)
    {
        return max(0, (m_insta(cl.gamemode, cl.mutators) ? RESPAWNSECS/2 : RESPAWNSECS)-(lastmillis-d->lastpain)/1000);
    }
} ctf;
#endif

