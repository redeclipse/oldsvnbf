struct ctfstate
{
    struct flag
    {
        vec droploc, spawnloc;
        int team, score, droptime;
#ifdef GAMESERVER
        int owner;
#else
        bool pickup;
        gameent *owner;
        extentity *ent;
        int interptime;
#endif

        flag() { reset(); }

        void reset()
        {
            droploc = spawnloc = vec(0, 0, 0);
#ifdef GAMESERVER
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

#ifndef GAMESERVER
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

#ifdef GAMESERVER
    void takeflag(int i, int owner)
#else
    void takeflag(int i, gameent *owner)
#endif
    {
		flag &f = flags[i];
		f.owner = owner;
		f.droptime = 0;
#ifndef GAMESERVER
		f.pickup = false;
#endif
    }

    void dropflag(int i, const vec &o, int droptime)
    {
		flag &f = flags[i];
		f.droploc = o;
		f.droptime = droptime;
#ifdef GAMESERVER
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
#ifdef GAMESERVER
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

#ifndef GAMESERVER
namespace ctf
{
	extern ctfstate st;
	extern void sendflags(ucharbuf &p);
	extern void parseflags(ucharbuf &p, bool commit);
	extern void dropflag(gameent *d, int i, const vec &droploc);
	extern void scoreflag(gameent *d, int relay, int goal, int score);
	extern void returnflag(gameent *d, int i);
	extern void takeflag(gameent *d, int i);
	extern void resetflag(int i);
	extern void setupflags();
	extern void checkflags(gameent *d);
	extern void drawblips(int w, int h, int s, float blend);
	extern int drawinventory(int x, int y, int s, float blend);
	extern int respawnwait(gameent *d);
	extern void preload();
	extern void render();
	extern void aifind(gameent *d, aistate &b, vector<interest> &interests);
	extern bool aicheck(gameent *d, aistate &b);
	extern bool aidefend(gameent *d, aistate &b);
	extern bool aipursue(gameent *d, aistate &b);
}
#endif
