struct ctfstate
{
    static const int FLAGLIMIT = 10;

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
            interptime = 0;
#endif
            team = TEAM_NEUTRAL;
            score = 0;
            droptime = 0;
        }

#ifndef CTFSERV
        vec &pos()
        {
        	if(owner) return vec(owner->o).sub(owner->height);
        	if(droptime) return droploc;
        	return spawnloc;
        }
#endif
    };
    vector<flag> flags;

    void reset()
    {
        flags.setsize(0);
    }

    int addflag(const vec &o, int team, int i = -1)
    {
    	int x = i < 0 ? flags.length() : i;
    	while(!flags.inrange(x)) flags.add();
		flag &f = flags[x];
		f.reset();
		f.team = team;
		f.spawnloc = o;
		return x;
    }

#ifdef CTFSERV
    void takeflag(int i, int owner)
#else
    void takeflag(int i, fpsent *owner)
#endif
    {
		flag &f = flags[i];
		f.owner = owner;
		f.droptime = 0;
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

    int findscore(int team)
    {
        int score = 0;
        loopv(flags) if(flags[i].team==team) score += flags[i].score;
        return score;
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
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
            ivec o(vec(ci->state.o).mul(DMF));
            sendf(-1, 1, "ri6", SV_DROPFLAG, ci->clientnum, i, o.x, o.y, o.z);
            ctfstate::dropflag(i, o.tovec().div(DMF), lastmillis);
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

    bool canchangeteam(clientinfo *ci, int oldteam, int newteam)
    {
    	loopi(m_multi(sv.gamemode, sv.mutators) ? MAXTEAMS : MAXTEAMS/2)
    	{
			if (newteam == i+TEAM_ALPHA) return true;
    	}
    	return false;
    }

    void changeteam(clientinfo *ci, int oldteam, int newteam)
    {
        dropflag(ci);
    }

    void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
    {
        if(notgotflags) return;
        static const dynent dummy;
        vec o(newpos);
        o.z -= dummy.height;
        loopv(flags) if(flags[i].owner==ci->clientnum)
        {
            loopvk(flags) if(flags[k].team==ci->team)
            {
				flag &goal = flags[k];

				if(goal.owner<0 && !goal.droptime && o.dist(goal.spawnloc) < enttype[FLAG].radius)
				{
					returnflag(i);
					goal.score++;
					sendf(-1, 1, "ri5", SV_SCOREFLAG, ci->clientnum, i, k, goal.score);

	                if(findscore(goal.team) >= FLAGLIMIT)
	                	sv.startintermission();
				}
            }
        }
    }

    void takeflag(clientinfo *ci, int i)
    {
        if(notgotflags || !flags.inrange(i) || ci->state.state!=CS_ALIVE || !ci->team) return;
		flag &f = flags[i];
		if(f.team == ci->team)
		{
			if(!f.droptime || f.owner>=0) return;
			ctfstate::returnflag(i);
			sendf(-1, 1, "ri3", SV_RETURNFLAG, ci->clientnum, i);
		}
		else
		{
			if(f.owner>=0) return;
            loopv(flags) if(flags[i].owner==ci->clientnum) return;
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
            if(f.owner<0 && f.droptime && lastmillis - f.droptime >= RESETFLAGTIME)
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
            if(notgotflags) addflag(o, team, i);
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
        loopi(TEAM_MAX) loadmodel(teamtype[i].flag, -1, true);
    }

    void drawblip(int x, int y, int s, int i, bool flagblip)
    {
		flag &f = flags[i];
		vec dir;
        if(flagblip) dir = f.pos();
        else dir = f.spawnloc;
		dir.sub(camera1->o);
		dir.z = 0.0f;
        float size = flagblip ? 0.033f : 0.015f,
              dist = dir.magnitude();
        if(dist >= cl.radarrange()*(1 - 0.05f)) dir.mul(cl.radarrange()*(1 - 0.05f)/dist);
		dir.rotate_around_z(-camera1->yaw*RAD);

		int colour = teamtype[f.team].colour;
		float r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f,
			fade = 1.f;
		if(f.team != cl.player1->team && (!f.owner || f.owner->team != cl.player1->team))
			fade = clamp(1.f-(dist/cl.radarrange()), 0.f, 1.f);

        settexture(cl.bliptex());
		glColor4f(r, g, b, fade);
		glBegin(GL_QUADS);
        cl.drawsized(x + s*0.5f*(1.0f + dir.x/cl.radarrange() - (size*0.5f)), y + s*0.5f*(1.0f + dir.y/cl.radarrange() - (size*0.5f)), size*s);
		glEnd();
    }

    void drawblips(int w, int h)
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

		int s = h/4, x = w-s-FONTH/4, y = FONTH/4;
        loopv(flags)
        {
            flag &f = flags[i];
            if(!f.team || !f.ent) continue;
            drawblip(x, y, s, i, false);
            if(f.owner)
            {
                if(lastmillis%1000 >= 500) continue;
            }
            else if(f.droptime && lastmillis%300 >= 150) continue;
            drawblip(x, y, s, i, true);
        }
    }


    void render()
    {
        loopv(flags)
        {
            flag &f = flags[i];
            if(!f.team || !f.ent || f.owner) continue;
            const char *flagname = teamtype[f.team].flag;
            vec above(f.pos());
            rendermodel(NULL, flagname, ANIM_MAPMODEL|ANIM_LOOP,
                        above, 0.f, 0.f, 0.f, MDL_SHADOW | MDL_CULL_VFC | MDL_CULL_OCCLUDED);
            above.z += enttype[FLAG].height;
            s_sprintfd(info)("@%s flag", teamtype[f.team].name);
			part_text(above, info, 10, 1, teamtype[f.team].colour);
        }
    }

    void setupflags()
    {
        reset();
        loopv(cl.et.ents)
        {
            extentity *e = cl.et.ents[i];
            if(e->type!=FLAG || e->attr2<TEAM_ALPHA || e->attr2>numteams(cl.gamemode, cl.mutators)) continue;
            int index = addflag(e->o, e->attr2);
			if(flags.inrange(index))
            	flags[index].ent = e;
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
            int team = getint(p), score = getint(p), owner = getint(p), dropped = 0;
            vec droploc(0, 0, 0);
            if(owner<0)
            {
                dropped = getint(p);
                if(dropped) loopk(3) droploc[k] = getint(p)/DMF;
            }
            if(commit && flags.inrange(i))
            {
				flag &f = flags[i];
				f.team = team;
                f.score = score;
                f.owner = owner >= 0 ? cl.newclient(owner) : NULL;
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
        if(!flags.inrange(i)) return;
		flag &f = flags[i];
		f.interptime = lastmillis;
		ctfstate::dropflag(i, droploc, 1);
		if(!cl.ph.droptofloor(f.droploc, 2, 0))
		{
			f.droploc = vec(-1, -1, -1);
			f.interptime = 0;
		}
		conoutf("%s dropped the the \fs%s%s\fS flag", d==cl.player1 ? "you" : cl.colorname(d), teamtype[f.team].chat, teamtype[f.team].name);
		cl.et.announce(S_V_FLAGDROP);
    }

    void flagexplosion(int i, const vec &loc)
    {
		flag &f = flags[i];
		int colour = teamtype[f.team].colour;
		regularshape(4, enttype[FLAG].radius, colour, 6, 50, 250, vec(loc).sub(vec(0, 0, 4.f)), 4.8f);
		adddynlight(loc, enttype[FLAG].radius, vec((colour>>16)/255.f, ((colour>>8)&0xFF)/255.f, (colour&0xFF)/255.f), 900, 100);
    }

    void flageffect(int i, const vec &from, const vec &to)
    {
		flag &f = flags[i];
		if(from.x >= 0)
		{
			flagexplosion(i, from);
		}
		if(to.x >= 0)
		{
			flagexplosion(i, to);
		}
		if(from.x >= 0 && to.x >= 0)
		{
			part_trail(4, 250, from, to, teamtype[f.team].colour, 4.8f);
		}
    }

    void returnflag(fpsent *d, int i)
    {
        if(!flags.inrange(i)) return;
		flag &f = flags[i];
		flageffect(i, f.droploc, f.spawnloc);
		f.interptime = 0;
		ctfstate::returnflag(i);
		conoutf("%s returned the \fs%s%s\fS flag", d==cl.player1 ? "you" : cl.colorname(d), teamtype[f.team].chat, teamtype[f.team].name);
		cl.et.announce(S_V_FLAGRETURN);
    }

    void resetflag(int i)
    {
        if(!flags.inrange(i)) return;
		flag &f = flags[i];
		flageffect(i, f.droploc, f.spawnloc);
		f.interptime = 0;
		ctfstate::returnflag(i);
		conoutf("the \fs%s%s\fS flag has been reset", teamtype[f.team].chat, teamtype[f.team].name);
		cl.et.announce(S_V_FLAGRESET);
    }

    void scoreflag(fpsent *d, int relay, int goal, int score)
    {
        if(!flags.inrange(goal) || !flags.inrange(relay)) return;
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
		conoutf("%s scored for \fs%s%s\fS team", d==cl.player1 ? "you" : cl.colorname(d), teamtype[f.team].chat, teamtype[f.team].name);
		cl.et.announce(S_V_FLAGSCORE);

        int total = findscore(f.team);
        if(total >= FLAGLIMIT)
        	conoutf("team \fs%s%s\fS captured %d flags", teamtype[f.team].chat, teamtype[f.team].name, total);
    }

    void takeflag(fpsent *d, int i)
    {
        if(!flags.inrange(i)) return;
		flag &f = flags[i];
		int colour = teamtype[d->team].colour;
		regularshape(4, enttype[FLAG].radius, colour, 6, 50, 250, vec(f.spawnloc).sub(vec(0, 0, 4.f)), 4.8f);
		adddynlight(f.spawnloc, enttype[FLAG].radius, vec((colour>>16)/255.f, ((colour>>8)&0xFF)/255.f, (colour&0xFF)/255.f), 900, 100);
		f.interptime = lastmillis;
		conoutf("%s %s the \fs%s%s\fS flag", d==cl.player1 ? "you" : cl.colorname(d), f.droptime ? "picked up" : "stole", teamtype[f.team].chat, teamtype[f.team].name);
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
            if(!f.team || !f.ent || f.owner || (f.droptime ? f.droploc.x<0 : f.team==d->team)) continue;
            if(o.dist(f.pos()) < enttype[FLAG].radius)
            {
                if(f.pickup) continue;
                cl.cc.addmsg(SV_TAKEFLAG, "ri2", d->clientnum, i);
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

