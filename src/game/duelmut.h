#ifdef GAMESERVER
struct duelservmode : servmode
{
    int duelround, dueltime, duelcheck, dueldeath;
    vector<clientinfo *> duelqueue, allowed, playing;

    duelservmode() {}

    void position(clientinfo *ci, bool clean)
    {
        if(allowbroadcast(ci->clientnum) && ci->state.aitype < AI_START)
        {
            int n = duelqueue.find(ci);
            if(n >= 0)
            {
                if(clean)
                {
                    n -= GAME(duelreset) ? 2 : 1;
                    if(n < 0) return;
                }
                if(m_survivor(gamemode, mutators)) srvmsgf(ci->clientnum, "\fwyou are \fs\fgqueued\fS for the next round");
                else
                {
                    if(n) srvmsgf(ci->clientnum, "\fwyou are \fs\fg#%d\fS in the queue", n+1);
                    else srvmsgf(ci->clientnum, "\fwyou are \fs\fzgyNEXT\fS in the queue");
                }
            }
        }
    }

    void queue(clientinfo *ci, bool top = false, bool wait = true, bool clean = false)
    {
        if(ci->online && ci->state.state != CS_SPECTATOR && ci->state.state != CS_EDITING && ci->state.aitype < AI_START)
        {
            int n = duelqueue.find(ci);
            if(top)
            {
                if(n >= 0) duelqueue.remove(n);
                duelqueue.insert(0, ci);
            }
            else if(n < 0) duelqueue.add(ci);
            if(wait && ci->state.state != CS_WAITING) waiting(ci, 1, 1);
            if(!clean) position(ci, false);
        }
    }

    void entergame(clientinfo *ci) { queue(ci); }
    void leavegame(clientinfo *ci, bool disconnecting = false)
    {
        duelqueue.removeobj(ci);
        allowed.removeobj(ci);
        playing.removeobj(ci);
    }

    bool damage(clientinfo *target, clientinfo *actor, int damage, int weap, int flags, const ivec &hitpush)
    {
        if(dueltime && target->state.aitype < AI_START) return false;
        return true;
    }

    bool canspawn(clientinfo *ci, bool tryspawn = false)
    {
        if(allowed.find(ci) >= 0 || ci->state.aitype >= AI_START) return true;
        if(tryspawn) queue(ci, false, duelround > 0 || duelqueue.length() > 1);
        return false; // you spawn when we want you to buddy
    }

    void spawned(clientinfo *ci) { allowed.removeobj(ci); }

    void died(clientinfo *ci, clientinfo *at) {}

    void clearitems()
    {
        loopv(sents) if(enttype[sents[i].type].usetype == EU_ITEM && hasitem(i)) setspawn(i, true);
    }

    void cleanup()
    {
        loopvrev(duelqueue) if(duelqueue[i]->state.state != CS_DEAD && duelqueue[i]->state.state != CS_WAITING) duelqueue.remove(i);
        loopvrev(allowed) if(allowed[i]->state.state != CS_DEAD && allowed[i]->state.state != CS_WAITING) allowed.remove(i);
    }

    void clear()
    {
        duelcheck = dueldeath = 0;
        dueltime = gamemillis+GAME(duellimit);
        playing.shrink(0);
    }

    void update()
    {
        if(interm || numclients(-1, true, AI_BOT) <= 1 || !hasgameinfo) return;

        if(dueltime < 0)
        {
            if(duelqueue.length() >= 2) clear();
            else
            {
                loopv(clients) queue(clients[i]); // safety
                return;
            }
        }
        else cleanup();

        if(dueltime)
        {
            if(gamemillis >= dueltime)
            {
                vector<clientinfo *> alive;
                loopv(clients) if(clients[i]->state.aitype < AI_START)
                    queue(clients[i], clients[i]->state.state == CS_ALIVE || clients[i]->state.aitype >= AI_START, GAME(duelreset) || clients[i]->state.state != CS_ALIVE || clients[i]->state.aitype >= AI_START, true);
                allowed.shrink(0); playing.shrink(0);
                if(!duelqueue.empty())
                {
                    loopv(clients) if(clients[i]->state.aitype < AI_START) position(clients[i], true);
                    loopv(duelqueue)
                    {
                        if(m_duel(gamemode, mutators) && alive.length() >= 2) break;
                        clientinfo *ci = duelqueue[i];
                        if(ci->state.state != CS_ALIVE)
                        {
                            if(ci->state.state != CS_WAITING) waiting(ci, 1, 1);
                            if(ci->state.aitype < AI_START && m_duel(gamemode, mutators) && m_team(gamemode, mutators))
                            {
                                bool skip = false;
                                loopv(alive) if(ci->team == alive[i]->team) { skip = true; break; }
                                if(skip) continue;
                            }
                            if(allowed.find(ci) < 0) allowed.add(ci);
                        }
                        else
                        {
                            ci->state.health = m_health(gamemode, mutators);
                            ci->state.lastregen = gamemillis;
                            ci->state.lastfire = ci->state.lastfireburn = 0;
                            sendf(-1, 1, "ri4", N_REGEN, ci->clientnum, ci->state.health, 0); // amt = 0 regens impulse
                            dropitems(ci, 1);
                        }
                        alive.add(ci);
                        playing.add(ci);
                    }
                    duelround++;
                    string fight;
                    if(m_duel(gamemode, mutators))
                    {
                        defformatstring(namea)("%s", colorname(alive[0]));
                        defformatstring(nameb)("%s", colorname(alive[1]));
                        formatstring(fight)("\fcduel between %s and %s, round \fs\fr#%d\fS", namea, nameb, duelround);
                    }
                    else if(m_survivor(gamemode, mutators))
                        formatstring(fight)("\fclast one left alive wins, round \fs\fr#%d\fS", duelround);
                    loopv(playing) if(allowbroadcast(playing[i]->clientnum))
                        sendf(playing[i]->clientnum, 1, "ri3s", N_ANNOUNCE, S_V_FIGHT, CON_MESG, fight);
                    if(m_survivor(gamemode, mutators) || GAME(duelclear)) clearitems();
                    dueltime = dueldeath = 0;
                    duelcheck = gamemillis;
                }
            }
        }
        else
        {
            bool cleanup = false;
            vector<clientinfo *> alive;
            loopv(clients)
                if(clients[i]->state.aitype < AI_START && clients[i]->state.state == CS_ALIVE && clients[i]->state.aitype < AI_START)
                    alive.add(clients[i]);
            if(!allowed.empty() && duelcheck && gamemillis-duelcheck >= 5000) loopvrev(allowed)
            {
                if(alive.find(allowed[i]) < 0) spectator(allowed[i]);
                allowed.remove(i);
                cleanup = true;
            }
            if(allowed.empty())
            {
                if(m_survivor(gamemode, mutators) && m_team(gamemode, mutators) && !alive.empty())
                {
                    bool found = false;
                    loopv(alive) if(i && alive[i]->team != alive[i-1]->team) { found = true; break; }
                    if(!found)
                    {
                        if(!dueldeath) dueldeath = gamemillis;
                        else if(gamemillis-dueldeath > DEATHMILLIS)
                        {
                            if(!cleanup)
                            {
                                srvmsgf(-1, "\fcteam \fs%s%s\fS are the victors", teamtype[alive[0]->team].chat, teamtype[alive[0]->team].name);
                                loopv(playing) if(allowbroadcast(playing[i]->clientnum))
                                {
                                    if(playing[i]->team == alive[0]->team)
                                    {
                                        sendf(playing[i]->clientnum, 1, "ri3s", N_ANNOUNCE, S_V_YOUWIN, -1, "");
                                        givepoints(playing[i], playing.length());
                                    }
                                    else sendf(playing[i]->clientnum, 1, "ri3s", N_ANNOUNCE, S_V_YOULOSE, -1, "");
                                }
                            }
                            clear();
                        }
                    }
                }
                else switch(alive.length())
                {
                    case 0:
                    {
                        if(!cleanup)
                        {
                            srvmsgf(-1, "\fceveryone died, \fzoyepic fail");
                            loopv(playing) if(allowbroadcast(playing[i]->clientnum))
                                sendf(playing[i]->clientnum, 1, "ri3s", N_ANNOUNCE, S_V_YOULOSE, -1, "");
                        }
                        clear();
                        break;
                    }
                    case 1:
                    {
                        if(!dueldeath) dueldeath = gamemillis;
                        else if(gamemillis-dueldeath > DEATHMILLIS)
                        {
                            if(!cleanup)
                            {
                                srvmsgf(-1, "\fc%s was the victor", colorname(alive[0]));
                                loopv(playing) if(allowbroadcast(playing[i]->clientnum))
                                {
                                    if(playing[i] == alive[0])
                                    {
                                        sendf(playing[i]->clientnum, 1, "ri3s", N_ANNOUNCE, S_V_YOUWIN, -1, "");
                                        givepoints(playing[i], playing.length());
                                    }
                                    else sendf(playing[i]->clientnum, 1, "ri3s", N_ANNOUNCE, S_V_YOULOSE, -1, "");
                                }
                            }
                            clear();
                        }
                        break;
                    }
                    default: break;
                }
            }
        }
    }

    void reset(bool empty)
    {
        duelround = duelcheck = dueldeath = 0;
        dueltime = -1;
        allowed.shrink(0);
        duelqueue.shrink(0);
        playing.shrink(0);
    }
} duelmutator;
#endif
