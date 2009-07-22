// server-side ctf manager
struct ctfservmode : ctfstate, servmode
{
    bool hasflaginfo;

    ctfservmode() : hasflaginfo(false) {}

    void reset(bool empty)
    {
        ctfstate::reset();
        hasflaginfo = false;
    }

    void dropflag(clientinfo *ci, const vec &o)
    {
        if(!hasflaginfo) return;
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
			ivec p(vec(ci->state.o.dist(o) > enttype[FLAG].radius ? ci->state.o : o).mul(DMF));
            sendf(-1, 1, "ri6", SV_DROPFLAG, ci->clientnum, i, p.x, p.y, p.z);
            ctfstate::dropflag(i, p.tovec().div(DMF), gamemillis);
        }
    }

    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        dropflag(ci, ci->state.o);
    }

    void died(clientinfo *ci, clientinfo *actor)
    {
        dropflag(ci, ci->state.o);
    }

    int fragvalue(clientinfo *victim, clientinfo *actor)
    {
    	int value = 1;
    	loopv(flags) if(flags[i].owner == victim->clientnum) value++;
    	return victim==actor || victim->team == actor->team ? -value : value;
    }

	int addscore(int team)
	{
		score &cs = findscore(team);
		cs.total++;
		return cs.total;
	}

    void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
    {
        if(!hasflaginfo) return;
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
            loopvk(flags)
            {
				flag &f = flags[k];
				if(isctfhome(f, ci->team) && (f.owner < 0 || (f.owner == ci->clientnum && i == k)) && !f.droptime && newpos.dist(f.spawnloc) <= enttype[FLAG].radius*2/3)
				{
					ctfstate::returnflag(i);
					givepoints(ci, 5);
					if(flags[i].team != ci->team)
					{
						ci->state.flags++;
						int score = addscore(ci->team);
						sendf(-1, 1, "ri5", SV_SCOREFLAG, ci->clientnum, i, k, score);
						if(GVAR(ctflimit) && score >= GVAR(ctflimit))
						{
							sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_GUIBACK, CON_INFO, "\fccpature limit has been reached!");
							startintermission();
						}
					}
					else sendf(-1, 1, "ri3", SV_RETURNFLAG, ci->clientnum, i);
				}
            }
        }
    }

    void takeflag(clientinfo *ci, int i)
    {
        if(!hasflaginfo || !flags.inrange(i) || ci->state.state!=CS_ALIVE || !ci->team) return;
		flag &f = flags[i];
		if(!(f.base&BASE_FLAG) || f.owner >= 0 || (f.team == ci->team && !f.droptime)) return;
		ctfstate::takeflag(i, ci->clientnum);
		if(f.team != ci->team) givepoints(ci, 3);
		sendf(-1, 1, "ri3", SV_TAKEFLAG, ci->clientnum, i);
    }

    void update()
    {
        if(!hasflaginfo) return;
        loopv(flags)
        {
            flag &f = flags[i];
            if(f.owner < 0 && f.droptime && gamemillis-f.droptime >= RESETFLAGTIME)
            {
                ctfstate::returnflag(i);
                loopvk(clients) if(isctfflag(f, clients[k]->team)) givepoints(clients[k], -5);
                sendf(-1, 1, "ri2", SV_RESETFLAG, i);
            }
        }
    }

    void initclient(clientinfo *ci, ucharbuf &p, bool connecting)
    {
		if(connecting)
        {
            loopv(scores)
		    {
			    score &cs = scores[i];
			    putint(p, SV_SCORE);
			    putint(p, cs.team);
			    putint(p, cs.total);
		    }
        }
        putint(p, SV_INITFLAGS);
        putint(p, flags.length());
        loopv(flags)
        {
            flag &f = flags[i];
            putint(p, f.team);
            putint(p, f.base);
            putint(p, f.owner);
            if(f.owner<0)
            {
                putint(p, f.droptime);
                if(f.droptime)
                {
                    putint(p, int(f.droploc.x*DMF));
                    putint(p, int(f.droploc.y*DMF));
                    putint(p, int(f.droploc.z*DMF));
                }
            }
        }
    }

	void regen(clientinfo *ci, int &total, int &amt, int &delay)
	{
		if(hasflaginfo) loopv(flags)
        {
            flag &f = flags[i];
            bool insidehome = (isctfhome(f, ci->team) && f.owner < 0 && !f.droptime && ci->state.o.dist(f.spawnloc) <= enttype[FLAG].radius*2.f);
            if(insidehome || f.owner == ci->clientnum)
            {
            	if(insidehome)
				{
					if(GVAR(overctfhealth)) total = max(GVAR(overctfhealth), total);
					if(ci->state.lastregen && GVAR(regenctfguard)) delay = GVAR(regenctfguard);
				}
				if(GVAR(regenctfflag)) amt = GVAR(regenctfflag);
				return;
            }
		}
	}

    void parseflags(ucharbuf &p)
    {
    	int numflags = getint(p);
    	if(numflags)
    	{
			loopi(numflags)
			{
				int team = getint(p), base = getint(p);
				vec o;
				loopk(3) o[k] = getint(p)/DMF;
				if(!hasflaginfo) addflag(o, team, base, i);
			}
			hasflaginfo = true;
    	}
    }
} ctfmode;
