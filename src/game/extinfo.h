
#define EXT_ACK                         -1
#define EXT_VERSION                     104
#define EXT_NO_ERROR                    0
#define EXT_ERROR                       1
#define EXT_PLAYERSTATS_RESP_IDS        -10
#define EXT_PLAYERSTATS_RESP_STATS      -11
#define EXT_UPTIME                      0
#define EXT_PLAYERSTATS                 1
#define EXT_TEAMSCORE                   2

/*
    Client:
    -----
    A: 0 EXT_UPTIME
    B: 0 EXT_PLAYERSTATS cn #a client number or -1 for all players#
    C: 0 EXT_TEAMSCORE

    Server:
    --------
    A: 0 EXT_UPTIME EXT_ACK EXT_VERSION uptime #in seconds#
    B: 0 EXT_PLAYERSTATS cn #send by client# EXT_ACK EXT_VERSION 0 or 1 #error, if cn was > -1 and client does not exist# ...
         EXT_PLAYERSTATS_RESP_IDS pid(s) #1 packet#
         EXT_PLAYERSTATS_RESP_STATS pid playerdata #1 packet for each player#
    C: 0 EXT_TEAMSCORE EXT_ACK EXT_VERSION 0 or 1 #error, no teammode# remaining_time gamemode loop(teamdata [numflags flags] or -1)

    Errors:
    --------------
    B:C:default: 0 command EXT_ACK EXT_VERSION EXT_ERROR
*/

    void extinfoplayer(ucharbuf &p, clientinfo *ci)
    {
        ucharbuf q = p;
        putint(q, EXT_PLAYERSTATS_RESP_STATS); // send player stats following
        putint(q, ci->clientnum); //add player id
		putint(q, ci->ping);
        sendstring(ci->name, q);
        sendstring(teamtype[ci->team].name, q); //backward compatibility mode
        putint(q, ci->state.frags);
        putint(q, ci->state.flags);
        putint(q, ci->state.deaths);
        putint(q, ci->state.teamkills);
        putint(q, ci->state.damage*100/max(ci->state.shotdamage,1));
        putint(q, ci->state.health);
        putint(q, ci->state.spree);
        putint(q, ci->state.weapselect);
        putint(q, ci->privilege);
        putint(q, ci->state.state);
        uint ip = getclientip(ci->clientnum);
        q.put((uchar*)&ip, 3);
        sendqueryreply(q);
    }

    void extinfoteams(ucharbuf &p)
    {
        putint(p, m_fight(gamemode) && m_team(gamemode, mutators) ? 0 : 1);
        putint(p, gamemode);
        putint(p, minremain);
        if(!m_team(gamemode, mutators) || !m_fight(gamemode)) return;

        vector<teamscore> scores;

        //most taken from scoreboard.h
        if(m_stf(gamemode))
        {
            loopv(stfmode.scores) scores.add(teamscore(stfmode.scores[i].team, stfmode.scores[i].total));
            loopv(clients) if(clients[i]->team) //check all teams available, since stfmode.scores contains only teams with scores
            {
                teamscore *ts = NULL;
                loopvj(scores) if(scores[j].team == clients[i]->team) { ts = &scores[j]; break; }
                if(!ts) scores.add(teamscore(clients[i]->team, 0));
            }
        }
        else if(m_ctf(gamemode))
        {
            loopv(ctfmode.scores) scores.add(teamscore(ctfmode.scores[i].team, ctfmode.scores[i].total));
            loopv(clients) if(clients[i]->team) //check all teams available, since ctfmode.scores contains only teams with scores
            {
                teamscore *ts = NULL;
                loopvj(scores) if(scores[j].team == clients[i]->team) { ts = &scores[j]; break; }
                if(!ts) scores.add(teamscore(clients[i]->team, 0));
            }
        }
        else
        {
            loopv(clients) if(clients[i]->team)
            {
                clientinfo *ci = clients[i];
                teamscore *ts = NULL;
                loopvj(scores) if(scores[j].team == ci->team) { ts = &scores[j]; break; }
                if(!ts) scores.add(teamscore(ci->team, ci->state.frags));
                else ts->score += ci->state.frags;
            }
        }

        loopv(scores)
        {
            sendstring(teamtype[scores[i].team].name, p); //backward compatibility mode
            putint(p, (int)scores[i].score);

            if(m_stf(gamemode))
            {
                int flags = 0;
                loopvj(stfmode.flags) if(stfmode.flags[j].owner == scores[i].team) flags++;
                putint(p, flags);
                loopvj(stfmode.flags) if(stfmode.flags[j].owner == scores[i].team) putint(p, j);
            }
            else putint(p,-1); //no flags follow
        }
    }

    void extqueryreply(ucharbuf &req, ucharbuf &p)
    {
        int extcmd = getint(req); // extended commands

        //Build a new packet
        putint(p, EXT_ACK); //send ack
        putint(p, EXT_VERSION); //send version of extended info

        switch(extcmd)
        {
            case EXT_UPTIME:
            {
                putint(p, uint(totalmillis)/1000); //in seconds
                break;
            }

            case EXT_PLAYERSTATS:
            {
                int cn = getint(req); //a special player, -1 for all

                clientinfo *ci = NULL;
                if(cn >= 0)
                {
                    loopv(clients) if(clients[i]->clientnum == cn) { ci = clients[i]; break; }
                    if(!ci)
                    {
                        putint(p, EXT_ERROR); //client requested by id was not found
                        sendqueryreply(p);
                        return;
                    }
                }

                putint(p, EXT_NO_ERROR); //so far no error can happen anymore

                ucharbuf q = p; //remember buffer position
                putint(q, EXT_PLAYERSTATS_RESP_IDS); //send player ids following
                if(ci) putint(q, ci->clientnum);
                else loopv(clients) putint(q, clients[i]->clientnum);
                sendqueryreply(q);

                if(ci) extinfoplayer(p, ci);
                else loopv(clients) extinfoplayer(p, clients[i]);
                return;
            }

            case EXT_TEAMSCORE:
            {
                extinfoteams(p);
                break;
            }

            default:
            {
                putint(p, EXT_ERROR);
                break;
            }
        }
        sendqueryreply(p);
    }

