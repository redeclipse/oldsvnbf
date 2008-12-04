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
#ifndef GAMESERVER
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
		float dx = (b.o.x-o.x), dy = (b.o.y-o.y), dz = (b.o.z-o.z);
		return dx*dx + dy*dy <= enttype[FLAG].radius*enttype[FLAG].radius && fabs(dz) <= enttype[FLAG].radius;
	}
};

#ifndef GAMESERVER
namespace stf
{
	extern stfstate st;
	extern void sendflags(ucharbuf &p);
	extern int pickspawn(int team);
	extern void updateflag(int i, int owner, int enemy, int converted);
	extern void setscore(int team, int total);
	extern void setupflags();
	extern void drawblips(int w, int h, int s, float blend);
	extern int respawnwait(gameent *d);
	extern int drawinventory(int x, int y, int s, float blend);
	extern void preload();
	extern void render();
	extern void aifind(gameent *d, aistate &b, vector<interest> &interests);
	extern bool aicheck(gameent *d, aistate &b);
	extern bool aidefend(gameent *d, aistate &b);
	extern bool aipursue(gameent *d, aistate &b);
}
#endif
