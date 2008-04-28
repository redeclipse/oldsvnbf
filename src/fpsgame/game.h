// network quantization scale
#define DMF 16.0f			// for world locations
#define DNF 100.0f			// for normalized vectors
#define DVELF 1.0f			// for playerspeed based velocity vectors

// hardcoded sounds, defined in sounds.cfg
enum
{
	S_JUMP = 0, S_LAND, S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6, S_DIE1, S_DIE2,
	S_SPLASH1, S_SPLASH2, S_SPLAT, S_DEBRIS, S_WHIZZ, S_WHIRR,
	S_RELOAD, S_SWITCH, S_PISTOL, S_SG, S_CG,
	S_GLFIRE, S_GLEXPL, S_GLHIT, S_RLFIRE, S_RLEXPL, S_RLFLY, S_RIFLE,
	S_ITEMPICKUP, S_ITEMSPAWN,
	S_V_BASECAP, S_V_BASELOST,
    S_FLAGPICKUP, S_FLAGDROP, S_FLAGRETURN, S_FLAGSCORE, S_FLAGRESET,
	S_V_FIGHT, S_V_CHECKPOINT, S_V_ONEMINUTE, S_V_YOUWIN, S_V_YOULOSE, S_V_FRAGGED, S_V_OWNED, S_V_HEADSHOT,
	S_V_SPREE1, S_V_SPREE2, S_V_SPREE3, S_V_SPREE4,
	S_REGEN, S_DAMAGE1, S_DAMAGE2, S_DAMAGE3, S_DAMAGE4, S_DAMAGE5, S_DAMAGE6, S_DAMAGE7, S_DAMAGE8,
	S_RESPAWN, S_CHAT, S_DENIED, S_MENUPRESS, S_MENUBACK
};


enum								// static entity types
{
	NOTUSED = ET_EMPTY,				// 0  entity slot not in use in map
	LIGHT = ET_LIGHT,				// 1  radius, intensity
	MAPMODEL = ET_MAPMODEL,			// 2  angle, idx
	PLAYERSTART = ET_PLAYERSTART,	// 3  angle, [team]
	ENVMAP = ET_ENVMAP,				// 4  radius
	PARTICLES = ET_PARTICLES,		// 5  type, [others]
	MAPSOUND = ET_SOUND,			// 6  idx, maxrad, minrad, volume
	SPOTLIGHT = ET_SPOTLIGHT,		// 7  radius
	WEAPON = ET_GAMESPECIFIC,		// 8  gun, ammo
	TELEPORT,						// 9  yaw, pitch, roll, push
	MONSTER,						// 10 [angle], [type]
	TRIGGER,						// 11
	JUMPPAD,						// 12 zpush, ypush, xpush
	BASE,							// 13 idx, team
	CHECKPOINT,						// 14 idx
	CAMERA,							// 15 yaw, pitch, pan (+:horiz/-:vert), idx
	WAYPOINT,						// 16 radius, weight
	MAXENTTYPES						// 17
};

enum { ETU_NONE = 0, ETU_ITEM, ETU_AUTO, ETU_MAX };

static struct enttypes
{
	int	type, 		links,	radius,	height, usetype;		const char *name;
} enttype[] = {
	{ NOTUSED,		0,		0,		0,		ETU_NONE,		"none" },
	{ LIGHT,		0,		0,		0,		ETU_NONE,		"light" },
	{ MAPMODEL,		58,		0,		0,		ETU_NONE,		"mapmodel" },
	{ PLAYERSTART,	0,		0,		0,		ETU_NONE,		"playerstart" },
	{ ENVMAP,		0,		0,		0,		ETU_NONE,		"envmap" },
	{ PARTICLES,	0,		0,		0,		ETU_NONE,		"particles" },
	{ MAPSOUND,		58,		0,		0,		ETU_NONE,		"sound" },
	{ SPOTLIGHT,	0,		0,		0,		ETU_NONE,		"spotlight" },
	{ WEAPON,		0,		16,		16,		ETU_ITEM,		"weapon" },
	{ TELEPORT,		50,		12,		12,		ETU_AUTO,		"teleport" },
	{ MONSTER,		0,		0,		0,		ETU_NONE,		"monster" },
	{ TRIGGER,		58,		16,		16,		ETU_NONE,		"trigger" }, // FIXME
	{ JUMPPAD,		58,		12,		12,		ETU_AUTO,		"jumppad" },
	{ BASE,			48,		32,		16,		ETU_NONE,		"base" },
	{ CHECKPOINT,	48,		16,		16,		ETU_NONE,		"checkpoint" }, // FIXME
	{ CAMERA,		48,		0,		0,		ETU_NONE,		"camera" },
	{ WAYPOINT,		1,		8,		8,		ETU_NONE,		"waypoint" }
};

struct fpsentity : extentity
{
	vector<int> links;
#ifndef STANDALONE
	int schan;
#endif

	fpsentity()
	{
		links.setsize(0);
#ifndef STANDALONE
		schan = -1;
#endif
	}
	~fpsentity()
	{
#ifndef STANDALONE
		if (sounds.inrange(schan) && sounds[schan].inuse) removesound(schan);
		schan = -1;
#endif
	}
};

#define SGRAYS			20
#define SGSPREAD		3

#define RL_DAMRAD		48
#define RL_DISTSCALE	6

enum
{
	GUN_PISTOL = 0,
	GUN_SG,
	GUN_CG,
	GUN_GL,
	GUN_RL,
	GUN_RIFLE,
	NUMGUNS
};

static struct guntypes
{
	int info, 		sound, 		esound, 	fsound,		rsound,		add,	max,	adelay,	rdelay,	damage,	speed,	time,	kick,	wobble;	const char *name;
} guntype[NUMGUNS] =
{
	{ GUN_PISTOL,	S_PISTOL,	-1,			S_WHIRR,	-1,			12,		12,		250,	2000,	10,		0,		0,		-10 ,	10,		"pistol" },
	{ GUN_SG,		S_SG,		-1,			S_WHIRR,	-1,			1,		8,		1000,	500,	5,		0,		0,		-30,	30, 	"shotgun" },
	{ GUN_CG,		S_CG,		-1,			S_WHIRR,	-1,			50,		50,		50,		3000,	5,		0,		0,		-4,		4,		"chaingun" },
	{ GUN_GL,		S_GLFIRE,	S_GLEXPL,	S_WHIZZ,	S_GLHIT,	2,		4,		1500,	0,		100,	100,	3000,	-15,	10,		"grenades" },
	{ GUN_RL,		S_RLFIRE,	S_RLEXPL,	S_RLFLY,	-1,			1,		1,		2500,	5000,	150,	150,	10000,	-40,	20,		"rockets" },
	{ GUN_RIFLE,	S_RIFLE,	-1,			S_WHIRR,	-1,			1,		5,		1500,	1000,	75,		0,		0,		-30,	20,		"rifle" },
};
#define isgun(gun)	(gun > -1 && gun < NUMGUNS)

#define HIT_LEGS		0x01
#define HIT_TORSO		0x02
#define HIT_HEAD		0x04
#define HIT_BURN		0x08

enum
{
	G_DEMO = 0,
	G_EDITMODE,
	G_SINGLEPLAYER,
	G_DEATHMATCH,
	G_CAPTURE,
	G_CTF,
	G_MAX
};

#define G_M_TEAM			0x0001	// team
#define G_M_INSTA			0x0002	// instagib
#define G_M_DUEL			0x0004	// duel
#define G_M_PROG			0x0008	// progressive
#define G_M_TTWO			0x0010	// mutli team

#define G_M_NUM				5

#define G_M_FRAG			G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_TTWO
#define G_M_BASE			G_M_INSTA|G_M_PROG|G_M_TTWO

static struct gametypes
{
	int	type,			mutators,		implied;			const char *name;
} gametype[] = {
	{ G_DEMO,			0,				0,					"Demo Playback" },
	{ G_EDITMODE,		0,				0,					"Editing" },
	{ G_SINGLEPLAYER,	0,				0,					"Singleplayer" },
	{ G_DEATHMATCH,		G_M_FRAG,		0,					"Deathmatch" },
	{ G_CAPTURE,		G_M_BASE,		G_M_TEAM,			"Base Capture" },
	{ G_CTF,			G_M_BASE,		G_M_TEAM,			"Capture the Flag" },
}, mutstype[] = {
	{ G_M_TEAM,			0,				0,					"Team" },
	{ G_M_INSTA,		0,				0,					"Instagib" },
	{ G_M_DUEL,			0,				0,					"Duel" },
	{ G_M_PROG,			0,				0,					"Progressive" },
	{ G_M_TTWO,			0,				G_M_TEAM,			"Times Two" },
};

#define m_game(a)			(a > -1 && a < G_MAX)

#define m_demo(a)			(a == G_DEMO)
#define m_edit(a)			(a == G_EDITMODE)
#define m_sp(a)				(a == G_SINGLEPLAYER)
#define m_dm(a)				(a == G_DEATHMATCH)
#define m_capture(a)		(a == G_CAPTURE)
#define m_ctf(a)			(a == G_CTF)

#define m_mp(a)				(a > G_DEMO && a < G_MAX)
#define m_frag(a)			(a >= G_DEATHMATCH)
#define m_base(a)			(m_capture(a) || m_ctf(a))
#define m_timed(a)			(m_frag(a))

#define m_team(a,b)			((m_dm(a) && (b & G_M_TEAM)) || m_base(a))
#define m_insta(a,b)		(m_frag(a) && (b & G_M_INSTA))
#define m_duel(a,b)			(m_frag(a) && (b & G_M_DUEL))
#define m_prog(a,b)			(m_base(a) && (b & G_M_PROG))
#define m_ttwo(a,b)			(m_team(a, b) && (b & G_M_TTWO))

#define isteam(a,b)			(!strcmp(a,b))

// network messages codes, c2s, c2c, s2c
enum
{
	SV_INITS2C = 0, SV_INITC2S, SV_POS, SV_TEXT, SV_SOUND, SV_CDIS,
	SV_SHOOT, SV_EXPLODE, SV_SUICIDE,
	SV_DIED, SV_DAMAGE, SV_SHOTFX,
	SV_TRYSPAWN, SV_SPAWNSTATE, SV_SPAWN, SV_FORCEDEATH,
	SV_GUNSELECT, SV_TAUNT,
	SV_MAPCHANGE, SV_MAPVOTE, SV_ITEMSPAWN, SV_ITEMUSE, SV_EXECLINK,
	SV_PING, SV_PONG, SV_CLIENTPING,
	SV_TIMEUP, SV_MAPRELOAD, SV_ITEMACC,
	SV_SERVMSG, SV_ITEMLIST, SV_RESUME,
    SV_EDITMODE, SV_EDITENT, SV_EDITLINK, SV_EDITVAR, SV_EDITSVAR, SV_EDITALIAS, SV_EDITF, SV_EDITT, SV_EDITM, SV_FLIP, SV_COPY, SV_PASTE, SV_ROTATE, SV_REPLACE, SV_DELCUBE, SV_REMIP, SV_NEWMAP, SV_GETMAP, SV_SENDMAP,
	SV_MASTERMODE, SV_KICK, SV_CLEARBANS, SV_CURRENTMASTER, SV_SPECTATOR, SV_SETMASTER, SV_SETTEAM, SV_APPROVEMASTER,
	SV_BASES, SV_BASEINFO,
    SV_TAKEFLAG, SV_RETURNFLAG, SV_RESETFLAG, SV_DROPFLAG, SV_SCOREFLAG, SV_INITFLAGS,
	SV_TEAMSCORE, SV_FORCEINTERMISSION,
	SV_LISTDEMOS, SV_SENDDEMOLIST, SV_GETDEMO, SV_SENDDEMO,
	SV_DEMOPLAYBACK, SV_RECORDDEMO, SV_STOPDEMO, SV_CLEARDEMOS,
	SV_CLIENT, SV_COMMAND, SV_RELOAD, SV_REGEN,
	SV_ADDBOT, SV_DELBOT, SV_INITBOT
};

static char msgsizelookup(int msg)
{
	static char msgsizesl[] =				// size inclusive message token, 0 for variable or not-checked sizes
	{
		SV_INITS2C, 4, SV_INITC2S, 0, SV_POS, 0, SV_TEXT, 0, SV_SOUND, 3, SV_CDIS, 2,
		SV_SHOOT, 0, SV_EXPLODE, 0, SV_SUICIDE, 2,
		SV_DIED, 7, SV_DAMAGE, 11, SV_SHOTFX, 9,
		SV_TRYSPAWN, 2, SV_SPAWNSTATE, 10, SV_SPAWN, 4, SV_FORCEDEATH, 2,
		SV_GUNSELECT, 3, SV_TAUNT, 2,
		SV_MAPCHANGE, 0, SV_MAPVOTE, 0, SV_ITEMSPAWN, 2, SV_ITEMUSE, 4, SV_EXECLINK, 3,
		SV_PING, 2, SV_PONG, 2, SV_CLIENTPING, 2,
		SV_TIMEUP, 2, SV_MAPRELOAD, 1, SV_ITEMACC, 4,
		SV_SERVMSG, 0, SV_ITEMLIST, 0, SV_RESUME, 0,
        SV_EDITMODE, 2, SV_EDITENT, 10, SV_EDITLINK, 4, SV_EDITVAR, 0, SV_EDITSVAR, 0, SV_EDITALIAS, 0, SV_EDITF, 16, SV_EDITT, 16, SV_EDITM, 15, SV_FLIP, 14, SV_COPY, 14, SV_PASTE, 14, SV_ROTATE, 15, SV_REPLACE, 16, SV_DELCUBE, 14, SV_REMIP, 1, SV_NEWMAP, 2, SV_GETMAP, 1, SV_SENDMAP, 0,
		SV_MASTERMODE, 2, SV_KICK, 2, SV_CLEARBANS, 1, SV_CURRENTMASTER, 3, SV_SPECTATOR, 3, SV_SETMASTER, 0, SV_SETTEAM, 0, SV_APPROVEMASTER, 2,
		SV_BASES, 0, SV_BASEINFO, 0,
        SV_DROPFLAG, 6, SV_SCOREFLAG, 5, SV_RETURNFLAG, 3, SV_TAKEFLAG, 2, SV_RESETFLAG, 2, SV_INITFLAGS, 0,
		SV_TEAMSCORE, 0, SV_FORCEINTERMISSION, 1,
		SV_LISTDEMOS, 1, SV_SENDDEMOLIST, 0, SV_GETDEMO, 2, SV_SENDDEMO, 0,
		SV_DEMOPLAYBACK, 2, SV_RECORDDEMO, 2, SV_STOPDEMO, 1, SV_CLEARDEMOS, 2,
		SV_CLIENT, 0, SV_COMMAND, 0, SV_RELOAD, 0, SV_REGEN, 0,
		SV_ADDBOT, 0, SV_DELBOT, 0, SV_INITBOT, 0,
		-1
	};
	for(char *p = msgsizesl; *p>=0; p += 2) if(*p==msg) return p[1];
	return -1;
}

#define DEMO_MAGIC "BFDZ"

struct demoheader
{
	char magic[16];
	int version, gamever;
};

#define MAXNAMELEN 16
#define MAXTEAMLEN 8
enum { TEAM_ALPHA = 0, TEAM_BETA, TEAM_DELTA, TEAM_GAMMA, TEAM_MAX };
static const char *teamnames[] = { "alpha", "beta", "delta", "gamma" };

enum { PRJ_SHOT = 0, PRJ_GIBS, PRJ_DEBRIS };

#define PLATFORMBORDER	0.2f
#define PLATFORMMARGIN	10.0f

#define MAXRADAR		256.f
#define MAXHEALTH		100
#define MAXCARRY		2

#define REGENWAIT		3000
#define REGENTIME		1000
#define REGENHEAL		5

#define ROUTE_ABS		0x0001
#define ROUTE_AVOID		0x0002
#define ROUTE_GTONE		0x0004

enum
{
	SAY_NONE = 0x0,
	SAY_ACTION = 0x1,
	SAY_TEAM = 0x2,
};

enum
{
	PRIV_NONE = 0,
	PRIV_MASTER,
	PRIV_ADMIN,
	PRIV_MAX
};

#define MM_MODE 0xF
#define MM_AUTOAPPROVE 0x1000
#define MM_DEFAULT (MM_MODE | MM_AUTOAPPROVE)

enum { MM_OPEN = 0, MM_VETO, MM_LOCKED, MM_PRIVATE };

#define MAXFOV			(gamethirdperson() ? 100 : 125)
#define MINFOV			(player1->gunselect == GUN_RIFLE ? 0 : 90)

enum
{
	CAMERA_NONE = 0,
	CAMERA_PLAYER,
	CAMERA_FOLLOW,
	CAMERA_ENTITY,
	CAMERA_MAX
};

enum
{
	SINFO_ICON = 0,
	SINFO_STATUS,
	SINFO_HOST,
	SINFO_DESC,
	SINFO_PING,
	SINFO_PLAYERS,
	SINFO_MAXCLIENTS,
	SINFO_GAME,
	SINFO_MAP,
	SINFO_TIME,
	SINFO_MAX
};

enum
{
	SSTAT_OPEN = 0,
	SSTAT_LOCKED,
	SSTAT_PRIVATE,
	SSTAT_FULL,
	SSTAT_UNKNOWN,
	SSTAT_MAX
};

#ifndef STANDALONE
static const char *serverinfotypes[] = {
	"",
	"status",
	"host",
	"desc",
	"ping",
	"pl",
	"max",
	"game",
	"map",
	"time"
};

static const char *serverstatustypes[] = {
	"\fs\fgopen\fS",
	"\fs\fblocked\fS",
	"\fs\fmprivate\fS",
	"\fs\frfull\fS",
	"\fs\fb?\fS"
};
#endif

// inherited by fpsent and server clients
struct fpsstate
{
	int health, ammo[NUMGUNS];
	int gunselect, gunwait[NUMGUNS], gunlast[NUMGUNS];
	int lastdeath, lifesequence, lastshot, lastspawn, lastpain, lastregen;
	int ownernum;

	fpsstate() : lifesequence(0), ownernum(-1) {}
	~fpsstate() {}

	bool canweapon(int gun, int millis)
	{
		if (isgun(gun) && (gunselect != gun) && (ammo[gun] >= 0 || guntype[gun].rdelay <= 0))
			return true;
		return false;
	}

	bool canshoot(int gun, int millis)
	{
		if (isgun(gun) && (ammo[gun] > 0) && (millis-gunlast[gun] >= gunwait[gun]))
			return true;
		return false;
	}

	bool canreload(int gun, int millis)
	{
		if (isgun(gun) && (ammo[gun] < guntype[gun].max && ammo[gun] >= 0) && (guntype[gun].rdelay > 0) && (millis-gunlast[gun] >= gunwait[gun]))
			return true;
		return false;
	}

	bool canuse(int type, int attr1, int attr2, int millis)
	{
		switch (type)
		{
			case WEAPON:
			{ // can't use when reloading or firing
				if (isgun(attr1) && (ammo[attr1] < guntype[attr1].max) && (millis-gunlast[attr1] >= gunwait[attr1]))
					return true;
				break;
			}
			default: break;
		}
		return false;
	}

	void useitem(int millis, int type, int attr1, int attr2)
	{
		switch (type)
		{
			case WEAPON:
			{
				guntypes &g = guntype[attr1];

				if (ammo[g.info] < 0) ammo[g.info] = 0;

				int carry = 0;
				loopi(NUMGUNS) if (ammo[i] >= 0 && guntype[i].rdelay > 0) carry++;
				if (carry > MAXCARRY)
				{
					if (gunselect != g.info && guntype[gunselect].rdelay > 0)
					{
						ammo[gunselect] = -1;
						gunselect = g.info;
					}
					else loopi(NUMGUNS) if (ammo[i] >= 0 && i != g.info && guntype[i].rdelay > 0)
					{
						ammo[i] = -1;
						break;
					}
				}

				ammo[g.info] = min(ammo[g.info] + (attr2 > 0 ? attr2 : g.add), g.max);

				lastshot = gunlast[g.info] = millis;
				gunwait[g.info] = (guntype[g.info].rdelay ? guntype[g.info].rdelay : guntype[g.info].adelay);
				break;
			}
			default: break;
		}
	}

	void respawn()
	{
		health = MAXHEALTH;
		lastdeath = lastshot = lastspawn = lastpain = lastregen = 0;
		loopi(NUMGUNS)
		{
			gunwait[i] = gunlast[i] = 0;
		}
		gunselect = -1;
		loopi(NUMGUNS) ammo[i] = -1;
	}

	void spawnstate(int gamemode, int mutators)
	{
		if(m_insta(gamemode, mutators))
		{
			health = 1;
			gunselect = GUN_RIFLE;
			loopi(NUMGUNS)
			{
				ammo[i] = i == GUN_RIFLE ? guntype[i].add : -1;
			}
		}
		else
		{
			health = MAXHEALTH;
			gunselect = GUN_PISTOL;
			loopi(NUMGUNS)
			{
				ammo[i] = i == GUN_PISTOL || i == GUN_SG ? guntype[i].add : -1;
			}
		}
	}

	// just subtract damage here, can set death, etc. later in code calling this
	int dodamage(int damage, int millis)
	{
		lastpain = lastregen = millis;
		health -= damage;
		return damage;
	}
};

// bot state information for the owner client
enum
{
	BS_WAIT = 0,	// waiting for next command
	BS_UPDATE,		// update current objective
	BS_MOVE,		// move toward goal waypoint
	BS_DEFEND,		// defend goal target
	BS_PURSUE,		// pursue goal target
	BS_ATTACK,		// attack goal target
	BS_INTEREST,	// game specific interest in goal entity
	BS_MAX
};

struct botstate
{
	int type, goal, millis, interval;

	botstate(int _type, int _goal = -1, int _millis = 0, int _interval = 0) :
		type(_type), goal(_goal), millis(_millis), interval(_interval) {}
	~botstate() {}
};

struct botinfo
{
	vector<botstate> state;	// stack of states
	vector<int> route;		// current route planned
	vec targpos;			// current targets

	botinfo()
	{
		reset();
		setstate();
	}
	~botinfo()
	{
		reset();
	}

	void reset()
	{
		state.setsize(0);
		route.setsize(0);
	}

	void addstate(int type, int goal = -1, int millis = 0, int interval = 0)
	{
		botstate bs(type, goal, millis, interval);
		state.add(bs);
	}

	void removestate(int index = -1)
	{
		if(index < 0) state.pop();
		else if(state.inrange(index)) state.remove(index);
		if(!state.length()) setstate();
	}

	void setstate(int type = BS_WAIT, int goal = -1, int interval = 0, bool pop = true)
	{
		bool popstate = pop && state.length() > 1;
		if(popstate) removestate();
		addstate(type, goal, lastmillis, interval);
	}

	botstate &getstate(int index)
	{
		if(state.inrange(index)) return state[index];
		return state.last();
	}

	botstate &curstate()
	{
		if(!state.length()) setstate();
		return state.last();
	}
};

struct fpsent : dynent, fpsstate
{
	int weight;
	int clientnum, privilege, lastupdate, plag, ping;
	int lastattackgun;
	bool attacking, reloading, useaction, leaning;
	int lasttaunt;
	int lastuse, lastusemillis, lastbase;
	int superdamage;
	int frags, deaths, totaldamage, totalshots;
	editinfo *edit;
    vec deltapos, newpos;
    float deltayaw, deltapitch, newyaw, newpitch;
    int smoothmillis;
	int spree, lastimpulse, lastnode;
	int respawned, suicided;
	botinfo *bot;

	string name, team, info;

	fpsent() : weight(100), clientnum(-1), privilege(PRIV_NONE), lastupdate(0), plag(0), ping(0), frags(0), deaths(0), totaldamage(0), totalshots(0), edit(NULL), smoothmillis(-1), spree(0), lastimpulse(0), bot(NULL)
	{
		name[0] = team[0] = info[0] = 0;
		respawn();
	}
	~fpsent()
	{
		freeeditinfo(edit);
		if(bot) delete bot;
	}

	void damageroll(float damage)
	{
		float damroll = 2.0f*damage;
		roll += roll>0 ? damroll : (roll<0 ? -damroll : (rnd(2) ? damroll : -damroll)); // give player a kick
	}

	void hitpush(int damage, const vec &dir, fpsent *actor, int gun)
	{
		vec push(dir);
		push.mul(damage/weight);
		vel.add(push);
	}

	void stopmoving()
	{
		dynent::stopmoving();
		attacking = reloading = useaction = leaning = false;
	}

	void respawn()
	{
		stopmoving();
		dynent::reset();
		fpsstate::respawn();
		lastattackgun = gunselect;
		lasttaunt = lastuse = lastusemillis = superdamage = spree = lastimpulse = 0;
		lastbase = respawned = suicided = -1;
	}
};
