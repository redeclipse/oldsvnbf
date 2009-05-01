#define isctfhome(a,b)	((a.base&BASE_HOME) && (a.team == b || a.team == TEAM_NEUTRAL))
#define isctfflag(a,b)	((a.base&BASE_FLAG) && (a.team == b || a.team == TEAM_NEUTRAL))

#ifdef GAMESERVER
#define ctfstate ctfservstate
#endif
struct ctfstate
{
    struct flag
    {
        vec droploc, spawnloc;
        int team, droptime, base;
#ifdef GAMESERVER
        int owner;
#else
        bool pickup;
        gameent *owner;
        extentity *ent;
        int interptime, taketime;
#endif

        flag()
#ifndef GAMESERVER
          : ent(NULL)
#endif
        { reset(); }

        void reset()
        {
            droploc = spawnloc = vec(0, 0, 0);
            base = BASE_NONE;
#ifdef GAMESERVER
            owner = -1;
#else
            pickup = false;
            owner = NULL;
            interptime = taketime = 0;
#endif
            team = TEAM_NEUTRAL;
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

	struct score
	{
		int team, total;
	};

	vector<score> scores;

    void reset()
    {
        flags.setsize(0);
		scores.setsize(0);
    }

    int addflag(const vec &o, int team, int base = BASE_NONE, int i = -1)
    {
    	int x = i < 0 ? flags.length() : i;
    	while(!flags.inrange(x)) flags.add();
		flag &f = flags[x];
		f.reset();
		f.team = team;
		f.spawnloc = o;
		f.base = base;
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

    void dropflag(int i, const vec &o, int t)
    {
		flag &f = flags[i];
		f.droploc = o;
		f.droptime = t;
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
	extern void setscore(int team, int total);
	extern void checkflags(gameent *d);
	extern void drawlast(int w, int h, int &tx, int &ty, float blend);
	extern void drawblips(int w, int h, int s, float blend);
	extern int drawinventory(int x, int y, int s, float blend);
	extern void preload();
	extern void render();
	extern void adddynlights();
	extern void aifind(gameent *d, ai::aistate &b, vector<ai::interest> &interests);
	extern bool aicheck(gameent *d, ai::aistate &b);
	extern bool aidefend(gameent *d, ai::aistate &b);
	extern bool aipursue(gameent *d, ai::aistate &b);
    extern void removeplayer(gameent *d);
}
#endif
