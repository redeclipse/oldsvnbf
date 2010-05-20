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
        if(!hasflaginfo || ci->state.aitype >= AI_START) return;
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
            ivec p(vec(ci->state.o.dist(o) > enttype[FLAG].radius ? ci->state.o : o).mul(DMF));
            sendf(-1, 1, "ri6", N_DROPFLAG, ci->clientnum, i, p.x, p.y, p.z);
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

    int addscore(int team)
    {
        score &cs = findscore(team);
        cs.total++;
        return cs.total;
    }

    void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
    {
        if(!hasflaginfo || ci->state.aitype >= AI_START) return;
        loopv(flags) if(flags[i].owner == ci->clientnum)
        {
            loopvk(flags)
            {
                flag &f = flags[k];
                if(isctfhome(f, ci->team) && (f.owner < 0 || (GAME(ctfstyle) == 1 && f.owner == ci->clientnum && i == k)) && !f.droptime && newpos.dist(f.spawnloc) <= enttype[FLAG].radius*2/3)
                {
                    ctfstate::returnflag(i, gamemillis);
                    givepoints(ci, 5);
                    if(flags[i].team != ci->team)
                    {
                        ci->state.flags++;
                        int score = addscore(ci->team);
                        sendf(-1, 1, "ri5", N_SCOREFLAG, ci->clientnum, i, k, score);
                        if(GAME(ctflimit) && score >= GAME(ctflimit))
                        {
                            sendf(-1, 1, "ri3s", N_ANNOUNCE, S_GUIBACK, CON_MESG, "\fccpature limit has been reached");
                            startintermission();
                        }
                    }
                    else sendf(-1, 1, "ri3", N_RETURNFLAG, ci->clientnum, i);
                }
            }
        }
    }

    void takeflag(clientinfo *ci, int i)
    {
        if(!hasflaginfo || !flags.inrange(i) || ci->state.state!=CS_ALIVE || !ci->team || ci->state.aitype >= AI_START) return;
        flag &f = flags[i];
        if(!(f.base&BASE_FLAG) || f.owner >= 0 || (f.team == ci->team && GAME(ctfstyle) <= 2 && (GAME(ctfstyle) == 2 || !f.droptime))) return;
        if(!GAME(ctfstyle) && f.team == ci->team)
        {
            ctfstate::returnflag(i, gamemillis);
            givepoints(ci, 5);
            sendf(-1, 1, "ri3", N_RETURNFLAG, ci->clientnum, i);
        }
        else
        {
            ctfstate::takeflag(i, ci->clientnum, gamemillis);
            if(f.team != ci->team) givepoints(ci, 3);
            sendf(-1, 1, "ri3", N_TAKEFLAG, ci->clientnum, i);
        }
    }

    void resetflag(clientinfo *ci, int i)
    {
        if(!hasflaginfo || !flags.inrange(i) || ci->state.ownernum >= 0) return;
        flag &f = flags[i];
        if(!(f.base&BASE_FLAG) || f.owner >= 0 || !f.droptime || f.votes.find(ci->clientnum) >= 0) return;
        f.votes.add(ci->clientnum);
        if(f.votes.length() >= numclients()/2)
        {
            ctfstate::returnflag(i, gamemillis);
            sendf(-1, 1, "ri2", N_RESETFLAG, i);
        }
    }

    void update()
    {
        if(!hasflaginfo) return;
        loopv(flags)
        {
            flag &f = flags[i];
            switch(GAME(ctfstyle))
            {
                case 3:
                    if(f.owner >= 0 && f.taketime && gamemillis-f.taketime >= GAME(ctfresetdelay))
                    {
                        clientinfo *ci = (clientinfo *)getinfo(f.owner);
                        if(f.team != ci->team)
                        {
                            ctfstate::returnflag(i, gamemillis);
                            givepoints(ci, 5);
                            ci->state.flags++;
                            int score = addscore(ci->team);
                            sendf(-1, 1, "ri5", N_SCOREFLAG, ci->clientnum, i, -1, score);
                            if(GAME(ctflimit) && score >= GAME(ctflimit))
                            {
                                sendf(-1, 1, "ri3s", N_ANNOUNCE, S_GUIBACK, CON_MESG, "\fccpature limit has been reached");
                                startintermission();
                            }
                        }
                        break;
                    }
                default:
                    if(f.owner < 0 && f.droptime && gamemillis-f.droptime >= GAME(ctfresetdelay))
                    {
                        ctfstate::returnflag(i, gamemillis);
                        loopvk(clients) if(isctfflag(f, clients[k]->team)) givepoints(clients[k], -5);
                        sendf(-1, 1, "ri2", N_RESETFLAG, i);
                    }
                    break;
            }
        }
    }

    void initclient(clientinfo *ci, packetbuf &p, bool connecting)
    {
        if(connecting)
        {
            loopv(scores)
            {
                score &cs = scores[i];
                putint(p, N_SCORE);
                putint(p, cs.team);
                putint(p, cs.total);
            }
        }
        putint(p, N_INITFLAGS);
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
        if(hasflaginfo && GAME(regenflag)) loopv(flags)
        {
            flag &f = flags[i];
            bool insidehome = (isctfhome(f, ci->team) && f.owner < 0 && !f.droptime && ci->state.o.dist(f.spawnloc) <= enttype[FLAG].radius*2.f);
            if(insidehome || (GAME(regenflag) == 2 && f.owner == ci->clientnum))
            {
                if(GAME(extrahealth)) total = max(GAME(extrahealth), total);
                if(ci->state.lastregen && GAME(regenguard)) delay = GAME(regenguard);
                if(GAME(regenextra)) amt = GAME(regenextra);
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

    int points(clientinfo *victim, clientinfo *actor)
    {
        bool isteam = victim==actor || victim->team == actor->team;
        int p = isteam ? -1 : 1, v = p;
        loopv(flags) if(flags[i].owner == victim->clientnum) p += v;
        return p;
    }
} ctfmode;
