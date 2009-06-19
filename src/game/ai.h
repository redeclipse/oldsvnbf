struct gameent;

enum { AI_NONE = 0, AI_BOT, AI_BSOLDIER, AI_RSOLDIER, AI_YSOLDIER, AI_GSOLDIER, AI_MAX };
#define isaitype(a)	(a >= 0 && a <= AI_MAX-1)

struct aitypes
{
	int type;	const char *name,	*mdl;
};
#ifdef GAMESERVER
aitypes aitype[] = {
	{ AI_NONE,		"",				"" },
	{ AI_BOT,		"bot",			"actors/player" },
	{ AI_BSOLDIER,	"alpha",		"actors/player/alpha" },
	{ AI_RSOLDIER,	"beta",			"actors/player/beta" },
	{ AI_YSOLDIER,	"delta",		"actors/player/delta" },
	{ AI_GSOLDIER,	"gamma",		"actors/player/gamma" },
};
#else
extern aitypes aitype[];
#endif

enum
{
	WP_NONE = 0,
	WP_CROUCH = 1<<0,
};

namespace ai
{
	const float CLOSEDIST		= float(enttype[WAYPOINT].radius);	// is close
	const float NEARDIST		= CLOSEDIST*4.f;					// is near
	const float NEARDISTSQ		= NEARDIST*NEARDIST;				// .. squared (constant for speed)
	const float FARDIST			= CLOSEDIST*16.f;					// too far
	const float JUMPMIN			= CLOSEDIST*0.25f;					// decides to jump
	const float JUMPMAX			= CLOSEDIST*1.5f;					// max jump
	const float SIGHTMIN		= CLOSEDIST*2.f;					// minimum line of sight
	const float SIGHTMAX		= CLOSEDIST*64.f;					// maximum line of sight
	const float VIEWMIN			= 70.f;								// minimum field of view
	const float VIEWMAX			= 150.f;							// maximum field of view

	// ai state information for the owner client
	enum
	{
		AI_S_WAIT = 0,		// waiting for next command
		AI_S_DEFEND,		// defend goal target
		AI_S_PURSUE,		// pursue goal target
		AI_S_INTEREST,		// interest in goal entity
		AI_S_MAX
	};

	enum
	{
		AI_T_NODE,
		AI_T_PLAYER,
		AI_T_AFFINITY,
		AI_T_ENTITY,
		AI_T_DROP,
		AI_T_MAX
	};

	struct interest
	{
		int state, node, target, targtype;
		float score;
		interest() : state(-1), node(-1), target(-1), targtype(-1), score(0.f) {}
		~interest() {}
	};

	struct aistate
	{
		int type, millis, next, targtype, target, idle;
		bool override;

		aistate(int m, int t, int r = -1, int v = -1) : type(t), millis(m), targtype(r), target(v)
		{
			reset();
		}
		~aistate() {}

		void reset()
		{
			next = millis;
			idle = 0;
			override = false;
		}
	};

	const int NUMPREVNODES = 3;

	struct aiinfo
	{
		vector<aistate> state;
		vector<int> route;
		vec target, spot;
		int enemy, enemyseen, enemymillis, prevnodes[NUMPREVNODES],
			lastrun, lasthunt, lastaction, jumpseed, jumprand, propelseed;
		float targyaw, targpitch, views[3];
		bool dontmove, tryreset, clear;

		aiinfo()
		{
			reset();
			loopk(3) views[k] = 0.f;
		}
		~aiinfo() { state.setsize(0); route.setsize(0); }

		void wipe()
		{
			state.setsize(0);
			route.setsize(0);
			addstate(AI_S_WAIT);
			clear = false;
		}

		void reset(bool tryit = false)
		{
			wipe();
			if(!tryit)
			{
				spot = target = vec(0, 0, 0);
				enemy = -1;
				lastaction = lasthunt = enemyseen = enemymillis = 0;
				lastrun = propelseed = jumpseed = lastmillis;
				jumprand = lastmillis+5000;
				dontmove = false;
			}
            memset(prevnodes, -1, sizeof(prevnodes));
			targyaw = rnd(360);
			targpitch = 0.f;
			tryreset = tryit;
		}

		bool hasprevnode(int n) const
		{
			loopi(NUMPREVNODES) if(prevnodes[i] == n) return true;
			return false;
		}
		void addprevnode(int n)
		{
			if(prevnodes[0] != n)
			{
				memmove(&prevnodes[1], prevnodes, sizeof(prevnodes) - sizeof(prevnodes[0]));
				prevnodes[0] = n;
			}
		}

		aistate &addstate(int t, int r = -1, int v = -1)
		{
			return state.add(aistate(lastmillis, t, r, v));
		}

		void removestate(int index = -1)
		{
			if(index < 0) state.pop();
			else if(state.inrange(index)) state.remove(index);
			if(!state.length()) addstate(AI_S_WAIT);
		}

		aistate &setstate(int t, int r = 1, int v = -1, bool pop = true)
		{
			if(pop) removestate();
			return addstate(t, r, v);
		}

		aistate &getstate(int idx = -1)
		{
			if(state.inrange(idx)) return state[idx];
			return state.last();
		}
	};

	extern vec aitarget;
	extern int aidebug, showaiinfo;

	extern float viewdist(int x = 101);
	extern float viewfieldx(int x = 101);
	extern float viewfieldy(int x = 101);
	extern bool targetable(gameent *d, gameent *e, bool z = true);
	extern bool cansee(gameent *d, vec &x, vec &y, vec &targ = aitarget);

	extern void init(gameent *d, int at, int on, int sk, int bn, char *name, int tm);

	extern bool badhealth(gameent *d);
	extern bool checkothers(vector<int> &targets, gameent *d = NULL, int state = -1, int targtype = -1, int target = -1, bool teams = false);
	extern bool makeroute(gameent *d, aistate &b, int node, bool changed = true, bool check = true);
	extern bool makeroute(gameent *d, aistate &b, const vec &pos, bool changed = true, bool check = true);
	extern bool randomnode(gameent *d, aistate &b, const vec &pos, float guard = NEARDIST, float wander = FARDIST);
	extern bool randomnode(gameent *d, aistate &b, float guard = NEARDIST, float wander = FARDIST);
	extern bool violence(gameent *d, aistate &b, gameent *e, bool pursue = false);
	extern bool patrol(gameent *d, aistate &b, const vec &pos, float guard = NEARDIST, float wander = FARDIST, int walk = 1, bool retry = false);
	extern bool defend(gameent *d, aistate &b, const vec &pos, float guard = NEARDIST, float wander = FARDIST, int walk = 1);

	extern void spawned(gameent *d);
	extern void damaged(gameent *d, gameent *e);
	extern void killed(gameent *d, gameent *e);
	extern void update();
	extern void avoid();
	extern void think(gameent *d, bool run);
	extern void render();
};
