#ifdef GAMESERVER
#define stfstate stfservstate
#endif
struct stfstate
{
	static const int OCCUPYPOINTS = 15;
	static const int OCCUPYLIMIT = 100;
	static const int SCORESECS = 10;

	struct flag
	{
		vec o;
		int owner, enemy;
#ifndef GAMESERVER
		string name, info;
		extentity *ent;
		bool hasflag;
		int lasthad;
#endif
		int owners, enemies, converted, securetime;

		flag()
#ifndef GAMESERVER
          : ent(NULL)
#endif
        { reset(); }

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
#ifndef GAMESERVER
			hasflag = false;
			lasthad = 0;
#endif
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

	vector<flag> flags;

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
		flag &b = flags.add();
		b.o = o;
	}

	void initflag(int i, int owner, int enemy, int converted)
	{
		if(!flags.inrange(i)) return;
		flag &b = flags[i];
		b.owner = owner;
		b.enemy = enemy;
		b.converted = converted;
	}

	bool hasflags(int team)
	{
		loopv(flags)
		{
			flag &b = flags[i];
			if(b.owner && b.owner == team) return true;
		}
		return false;
	}

	float disttoenemy(flag &b)
	{
		float dist = 1e10f;
		loopv(flags)
		{
			flag &e = flags[i];
			if(e.owner && b.owner != e.owner)
				dist = min(dist, b.o.dist(e.o));
		}
		return dist;
	}

	bool insideflag(const flag &b, const vec &o, float scale = 1.f)
	{
		float dx = (b.o.x-o.x), dy = (b.o.y-o.y), dz = (b.o.z-o.z);
		return dx*dx + dy*dy <= (enttype[FLAG].radius*enttype[FLAG].radius)*scale && fabs(dz) <= enttype[FLAG].radius*scale;
	}
};

#ifndef GAMESERVER
namespace stf
{
	extern stfstate st;
	extern void sendflags(ucharbuf &p);
	extern void updateflag(int i, int owner, int enemy, int converted);
	extern void setscore(int team, int total);
	extern void setupflags();
	extern void drawlast(int w, int h, int &tx, int &ty, float blend);
	extern void drawblips(int w, int h, int s, float blend);
	extern int drawinventory(int x, int y, int s, float blend);
	extern void preload();
	extern void render();
	extern void adddynlights();
	extern void aifind(gameent *d, aistate &b, vector<interest> &interests);
	extern bool aicheck(gameent *d, aistate &b);
	extern bool aidefend(gameent *d, aistate &b);
	extern bool aipursue(gameent *d, aistate &b);
    extern void removeplayer(gameent *d);
}
#endif
