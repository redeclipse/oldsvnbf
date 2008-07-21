// network quantization scale
#define DMF 16.0f			// for world locations
#define DNF 100.0f			// for normalized vectors
#define DVELF 1.0f			// for playerspeed based velocity vectors

// hardcoded sounds, defined in sounds.cfg
enum
{
	S_JUMP = 0, S_LAND, S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6, S_DIE1, S_DIE2,
	S_SPLASH1, S_SPLASH2, S_UNDERWATER, S_SPLAT, S_DEBRIS, S_WHIZZ, S_WHIRR,
	S_RELOAD, S_SWITCH, S_PISTOL, S_SG, S_CG,
	S_GLFIRE, S_GLEXPL, S_GLHIT, S_FLFIRE, S_FLBURN, S_RLFIRE, S_RLEXPL, S_RLFLY, S_RIFLE,
	S_ITEMPICKUP, S_ITEMSPAWN, 	S_REGEN,
	S_DAMAGE1, S_DAMAGE2, S_DAMAGE3, S_DAMAGE4, S_DAMAGE5, S_DAMAGE6, S_DAMAGE7, S_DAMAGE8,
	S_RESPAWN, S_CHAT, S_DENIED, S_MENUPRESS, S_MENUBACK,
	S_V_FLAGSECURED, S_V_FLAGOVERTHROWN,
    S_V_FLAGPICKUP, S_V_FLAGDROP, S_V_FLAGRETURN, S_V_FLAGSCORE, S_V_FLAGRESET,
	S_V_FIGHT, S_V_CHECKPOINT, S_V_ONEMINUTE, S_V_HEADSHOT,
	S_V_SPREE1, S_V_SPREE2, S_V_SPREE3, S_V_SPREE4,
	S_V_YOUWIN, S_V_YOULOSE, S_V_MCOMPLETE, S_V_FRAGGED, S_V_OWNED,
	S_MAX
};


enum								// entity types
{
	NOTUSED = ET_EMPTY,				// 0  entity slot not in use in map
	LIGHT = ET_LIGHT,				// 1  radius, intensity
	MAPMODEL = ET_MAPMODEL,			// 2  angle, idx
	PLAYERSTART = ET_PLAYERSTART,	// 3  angle, team
	ENVMAP = ET_ENVMAP,				// 4  radius
	PARTICLES = ET_PARTICLES,		// 5  type, [others]
	MAPSOUND = ET_SOUND,			// 6  idx, maxrad, minrad, volume
	SPOTLIGHT = ET_SPOTLIGHT,		// 7  radius
	WEAPON = ET_GAMESPECIFIC,		// 8  gun, ammo
	TELEPORT,						// 9  yaw, pitch, roll, push
	MONSTER,						// 10 angle, type
	TRIGGER,						// 11 idx, type, acttype, resettime
	PUSHER,							// 12 zpush, ypush, xpush
	FLAG,							// 13 idx, team
	CHECKPOINT,						// 14 idx
	CAMERA,							// 15
	WAYPOINT,						// 16
	ANNOUNCER,						// 17 maxrad, minrad, volume
	CONNECTION,						// 18
	MAXENTTYPES						// 19
};

enum { EU_NONE = 0, EU_ITEM, EU_AUTO, EU_ACT, EU_MAX };
enum { TR_NONE = 0, TR_MMODEL, TR_SCRIPT, TR_MAX };
enum { TA_NONE = 0, TA_AUTO, TA_ACT, TA_LINK, TA_MAX };

struct enttypes
{
	int	type, 		links,	radius,	height, usetype;		const char *name;
} enttype[] = {
	{ NOTUSED,		0,		0,		0,		EU_NONE,		"none" },
	{ LIGHT,		59,		0,		0,		EU_NONE,		"light" },
	{ MAPMODEL,		58,		0,		0,		EU_NONE,		"mapmodel" },
	{ PLAYERSTART,	59,		0,		0,		EU_NONE,		"playerstart" },
	{ ENVMAP,		0,		0,		0,		EU_NONE,		"envmap" },
	{ PARTICLES,	59,		0,		0,		EU_NONE,		"particles" },
	{ MAPSOUND,		58,		0,		0,		EU_NONE,		"sound" },
	{ SPOTLIGHT,	59,		0,		0,		EU_NONE,		"spotlight" },
	{ WEAPON,		59,		16,		16,		EU_ITEM,		"weapon" },
	{ TELEPORT,		50,		12,		12,		EU_AUTO,		"teleport" },
	{ MONSTER,		59,		0,		0,		EU_NONE,		"monster" },
	{ TRIGGER,		58,		16,		16,		EU_AUTO,		"trigger" },
	{ PUSHER,		58,		12,		12,		EU_AUTO,		"pusher" },
	{ FLAG,			48,		32,		16,		EU_NONE,		"flag" },
	{ CHECKPOINT,	48,		16,		16,		EU_NONE,		"checkpoint" }, // FIXME
	{ CAMERA,		48,		0,		0,		EU_NONE,		"camera" },
	{ WAYPOINT,		1,		8,		8,		EU_NONE,		"waypoint" },
	{ ANNOUNCER,	64,		0,		0,		EU_NONE,		"announcer" },
	{ CONNECTION,	70,		8,		8,		EU_NONE,		"connection" },
};

enum { ETYPE_NONE = 0, ETYPE_WORLD, ETYPE_DYNAMIC };

#ifndef STANDALONE
struct fpsentity : extentity
{
	vec pos;
	int owner, enttype, schan, lastemit;
	bool mark;

	fpsentity() : owner(-1), enttype(ETYPE_WORLD), schan(-1), lastemit(0), mark(false) {}
	~fpsentity()
	{
		if (issound(schan)) removesound(schan);
		schan = -1;
	}
};

const char *animnames[] =
{
	"dead", "dying", "idle",
	"forward", "backward", "left", "right",
	"pain", "jump", "sink", "swim", "mapmodel",
	"edit", "lag", "switch", "taunt", "win", "lose",
	"crouch", "crawl forward", "crawl backward", "crawl left", "crawl right",
	"pistol", "pistol shoot", "pistol reload",
	"shotgun", "shotgun shoot", "shotgun reload",
	"chaingun", "chaingun shoot", "chaingun reload",
	"grenades", "grenades throw", "grenades reload", "grenades power",
	"flamer", "flamer shoot", "flamer reload",
	"rifle", "rifle shoot", "rifle reload",
	"vwep", "shield", "powerup",
	""
};
#endif

enum
{
	ANIM_EDIT = ANIM_GAMESPECIFIC, ANIM_LAG, ANIM_SWITCH, ANIM_TAUNT, ANIM_WIN, ANIM_LOSE,
	ANIM_CROUCH, ANIM_CRAWL_FORWARD, ANIM_CRAWL_BACKWARD, ANIM_CRAWL_LEFT, ANIM_CRAWL_RIGHT,
    ANIM_PISTOL, ANIM_PISTOL_SHOOT, ANIM_PISTOL_RELOAD,
    ANIM_SHOTGUN, ANIM_SHOTGUN_SHOOT, ANIM_SHOTGUN_RELOAD,
    ANIM_CHAINGUN, ANIM_CHAINGUN_SHOOT, ANIM_CHAINGUN_RELOAD,
    ANIM_GRENADES, ANIM_GRENADES_THROW, ANIM_GREANDES_RELOAD, ANIM_GRENADES_POWER,
    ANIM_FLAMER, ANIM_FLAMER_SHOOT, ANIM_FLAMER_RELOAD,
    ANIM_RIFLE, ANIM_RIFLE_SHOOT, ANIM_RIFLE_RELOAD,
    ANIM_VWEP, ANIM_SHIELD, ANIM_POWERUP,
    ANIM_MAX
};

#define SGRAYS			15
#define SGSPREAD		4
#define GUNSWITCHDELAY	800

enum
{
	GUN_PISTOL = 0,
	GUN_SG,
	GUN_CG,
	GUN_GL,
	GUN_FLAMER,
	GUN_RIFLE,
	NUMGUNS
};

enum
{
	GUNSTATE_IDLE = 0,
	GUNSTATE_SHOOT,
	GUNSTATE_RELOAD,
	GUNSTATE_POWER,
	GUNSTATE_SWITCH,
	GUNSTATE_MAX
};

struct guntypes
{
	int info, 		anim,			sound, 		esound, 	fsound,		rsound,		add,	max,	adelay,	rdelay,	damage,	speed,	power,	time,	kick,	wobble,	scale,	radius;	const char *name,	*vwep;
} guntype[NUMGUNS] =
{
	{ GUN_PISTOL,	ANIM_PISTOL,	S_PISTOL,	-1,			S_WHIRR,	-1,			12,		12,		250,	1000,	25,		0,		0,		0,		-10,    10,		0,		0,		"pistol",			"weapons/pistol/vwep" },
	{ GUN_SG,		ANIM_SHOTGUN,	S_SG,		-1,			S_WHIRR,	-1,			1,		8,		500,	1500,	10,		0,		0,		0,		-30,    30, 	0,		0,		"shotgun",			"weapons/shotgun/vwep" },
	{ GUN_CG,		ANIM_CHAINGUN,	S_CG,		-1,			S_WHIRR,	-1,			40,		40,		100,	1000,	15,		0,		0,		0,		-5,	     4,		0,		0,		"chaingun",			"weapons/chaingun/vwep" },
	{ GUN_GL,		ANIM_GRENADES,	S_GLFIRE,	S_GLEXPL,	S_WHIZZ,	S_GLHIT,	2,		4,		1500,	0,		250,	150,	1000,	3000,	-15,    10,		8,		64,		"grenades",			"weapons/grenades/vwep" },
	{ GUN_FLAMER,	ANIM_FLAMER,	S_FLFIRE,	S_FLBURN,	-1,			-1,			100,	100,	150, 	2000,	25,		80,		0,		3000,	-1,		 1,		8,		20,		"flamer",			"weapons/flamer/vwep" },
	{ GUN_RIFLE,	ANIM_RIFLE,		S_RIFLE,	-1,			S_WHIRR,	-1,			1,		6,		500,	1000,	100,	0,		0,		0,		-30,  	20,		0,		0,		"rifle",			"weapons/rifle/vwep" },
};
#define isgun(gun)	(gun > -1 && gun < NUMGUNS)

#define HIT_LEGS		0x01
#define HIT_TORSO		0x02
#define HIT_HEAD		0x04

#define HIT_BURN		0x10
#define HIT_EXPLODE		0x10

enum
{
	G_DEMO = 0,
	G_LOBBY,
	G_EDITMODE,
	G_MISSION,
	G_DEATHMATCH,
	G_STF,
	G_CTF,
	G_MAX
};

#define G_M_TEAM			0x0001	// team
#define G_M_INSTA			0x0002	// instagib
#define G_M_DUEL			0x0004	// duel
#define G_M_PROG			0x0008	// progressive

#define G_M_MULTI			0x0010	// mutli team
#define G_M_DLMS			0x0020	// last man standing

#define G_M_NUM				6

#define G_M_ALL				G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_PROG|G_M_MULTI|G_M_DLMS
#define G_M_FIGHT			G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_MULTI|G_M_DLMS
#define G_M_FLAG			G_M_TEAM|G_M_INSTA|G_M_PROG|G_M_MULTI

struct gametypes
{
	int	type,			mutators,		implied;			const char *name;
} gametype[] = {
	{ G_DEMO,			0,				0,					"Demo" },
	{ G_LOBBY,			0,				0,					"Lobby" },
	{ G_EDITMODE,		0,				0,					"Editing" },
	{ G_MISSION,		0,				0,					"Mission" },
	{ G_DEATHMATCH,		G_M_FIGHT,		0,					"Deathmatch" },
	{ G_STF,			G_M_FLAG,		G_M_TEAM,			"Secure the Flag" },
	{ G_CTF,			G_M_FLAG,		G_M_TEAM,			"Capture the Flag" },
}, mutstype[] = {
	{ G_M_TEAM,			G_M_ALL,		0,					"Team" },
	{ G_M_INSTA,		G_M_ALL,		0,					"Instagib" },
	{ G_M_DUEL,			G_M_ALL,		0,					"Duel" },
	{ G_M_PROG,			G_M_ALL,		0,					"Progressive" },
	{ G_M_MULTI,		G_M_ALL,		G_M_TEAM,			"Multi-sided" },
	{ G_M_DLMS,			G_M_ALL,		G_M_DUEL,			"Last Man Standing" },
};

#define m_game(a)			(a > -1 && a < G_MAX)

#define m_demo(a)			(a == G_DEMO)
#define m_lobby(a)			(a == G_LOBBY)
#define m_edit(a)			(a == G_EDITMODE)
#define m_mission(a)		(a == G_MISSION)
#define m_dm(a)				(a == G_DEATHMATCH)
#define m_stf(a)			(a == G_STF)
#define m_ctf(a)			(a == G_CTF)

#define m_fight(a)			(a >= G_DEATHMATCH)
#define m_flag(a)			(m_stf(a) || m_ctf(a))
#define m_timed(a)			(m_fight(a))

#define m_team(a,b)			((m_dm(a) && (b & G_M_TEAM)) || m_flag(a))
#define m_insta(a,b)		(m_fight(a) && (b & G_M_INSTA))
#define m_duel(a,b)			(m_fight(a) && (b & G_M_DUEL))
#define m_prog(a,b)			(m_flag(a) && (b & G_M_PROG))
#define m_multi(a,b)		(m_team(a, b) && (b & G_M_MULTI))
#define m_dlms(a,b)			(m_duel(a, b) && (b & G_M_DLMS))

// network messages codes, c2s, c2c, s2c
enum
{
	SV_INITS2C = 0, SV_INITC2S, SV_POS, SV_TEXT, SV_COMMAND, SV_ANNOUNCE, SV_SOUND, SV_CDIS,
	SV_SHOOT, SV_EXPLODE, SV_SUICIDE,
	SV_DIED, SV_DAMAGE, SV_SHOTFX,
	SV_TRYSPAWN, SV_SPAWNSTATE, SV_SPAWN, SV_FORCEDEATH,
	SV_GUNSELECT, SV_TAUNT,
	SV_MAPCHANGE, SV_MAPVOTE, SV_ITEMSPAWN, SV_ITEMUSE, SV_EXECLINK,
	SV_PING, SV_PONG, SV_CLIENTPING,
	SV_TIMEUP, SV_MAPREQUEST, SV_ITEMACC,
	SV_SERVMSG, SV_ITEMLIST, SV_RESUME,
    SV_EDITMODE, SV_EDITENT, SV_EDITLINK, SV_EDITVAR, SV_EDITF, SV_EDITT, SV_EDITM, SV_FLIP, SV_COPY, SV_PASTE, SV_ROTATE, SV_REPLACE, SV_DELCUBE, SV_REMIP, SV_NEWMAP, SV_GETMAP, SV_SENDMAP,
	SV_MASTERMODE, SV_KICK, SV_CLEARBANS, SV_CURRENTMASTER, SV_SPECTATOR, SV_SETMASTER, SV_SETTEAM, SV_APPROVEMASTER,
	SV_FLAGS, SV_FLAGINFO,
    SV_TAKEFLAG, SV_RETURNFLAG, SV_RESETFLAG, SV_DROPFLAG, SV_SCOREFLAG, SV_INITFLAGS,
	SV_TEAMSCORE, SV_FORCEINTERMISSION,
	SV_LISTDEMOS, SV_SENDDEMOLIST, SV_GETDEMO, SV_SENDDEMO,
	SV_DEMOPLAYBACK, SV_RECORDDEMO, SV_STOPDEMO, SV_CLEARDEMOS,
	SV_CLIENT, SV_RELOAD, SV_REGEN,
	SV_ADDBOT, SV_DELBOT, SV_INITBOT
};

char msgsizelookup(int msg)
{
	char msgsizesl[] =				// size inclusive message token, 0 for variable or not-checked sizes
	{
		SV_INITS2C, 3, SV_INITC2S, 0, SV_POS, 0, SV_TEXT, 0, SV_COMMAND, 0,
		SV_ANNOUNCE, 0, SV_SOUND, 3, SV_CDIS, 2,
		SV_SHOOT, 0, SV_EXPLODE, 0, SV_SUICIDE, 2,
		SV_DIED, 7, SV_DAMAGE, 10, SV_SHOTFX, 9,
		SV_TRYSPAWN, 2, SV_SPAWNSTATE, 13, SV_SPAWN, 4, SV_FORCEDEATH, 2,
		SV_GUNSELECT, 0, SV_TAUNT, 2,
		SV_MAPCHANGE, 0, SV_MAPVOTE, 0, SV_ITEMSPAWN, 2, SV_ITEMUSE, 4, SV_EXECLINK, 3,
		SV_PING, 2, SV_PONG, 2, SV_CLIENTPING, 2,
		SV_TIMEUP, 2, SV_MAPREQUEST, 1, SV_ITEMACC, 3,
		SV_SERVMSG, 0, SV_ITEMLIST, 0, SV_RESUME, 0,
        SV_EDITMODE, 2, SV_EDITENT, 10, SV_EDITLINK, 4, SV_EDITVAR, 0, SV_EDITF, 16, SV_EDITT, 16, SV_EDITM, 15, SV_FLIP, 14, SV_COPY, 14, SV_PASTE, 14, SV_ROTATE, 15, SV_REPLACE, 16, SV_DELCUBE, 14, SV_REMIP, 1, SV_NEWMAP, 2, SV_GETMAP, 1, SV_SENDMAP, 0,
		SV_MASTERMODE, 2, SV_KICK, 2, SV_CLEARBANS, 1, SV_CURRENTMASTER, 3, SV_SPECTATOR, 3, SV_SETMASTER, 0, SV_SETTEAM, 0, SV_APPROVEMASTER, 2,
		SV_FLAGS, 0, SV_FLAGINFO, 0,
        SV_DROPFLAG, 6, SV_SCOREFLAG, 5, SV_RETURNFLAG, 3, SV_TAKEFLAG, 3, SV_RESETFLAG, 2, SV_INITFLAGS, 0,
		SV_TEAMSCORE, 0, SV_FORCEINTERMISSION, 1,
		SV_LISTDEMOS, 1, SV_SENDDEMOLIST, 0, SV_GETDEMO, 2, SV_SENDDEMO, 0,
		SV_DEMOPLAYBACK, 2, SV_RECORDDEMO, 2, SV_STOPDEMO, 1, SV_CLEARDEMOS, 2,
		SV_CLIENT, 0, SV_RELOAD, 0, SV_REGEN, 0,
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

enum { TEAM_NEUTRAL = 0, TEAM_ALPHA, TEAM_BETA, TEAM_DELTA, TEAM_GAMMA, TEAM_MAX };
struct teamtypes
{
	int	type,		colour;	const char *name,	*tpmdl,			*fpmdl,				*flag,			*icon,			*chat;
} teamtype[] = {
	{ TEAM_NEUTRAL,	0x2F2F2F,	"neutral",		"player",		"player/vwep",		"flag",			"team",			"\fa" },
	{ TEAM_ALPHA,	0x2222FF,	"alpha",		"player/alpha",	"player/alpha/vwep","flag/alpha",	"teamalpha",	"\fb" },
	{ TEAM_BETA,	0xFF2222,	"beta",			"player/beta",	"player/beta/vwep",	"flag/beta",	"teambeta",		"\fr" },
	{ TEAM_DELTA,	0xFFFF22,	"delta",		"player/delta",	"player/delta/vwep","flag/delta",	"teamdelta",	"\fy" },
	{ TEAM_GAMMA,	0x22FF22,	"gamma",		"player/gamma",	"player/gamma/vwep","flag/gamma",	"teamgamma",	"\fg" }
};

enum { PRJ_SHOT = 0, PRJ_GIBS, PRJ_DEBRIS };

#define PLATFORMBORDER	0.2f
#define PLATFORMMARGIN	10.0f

#define MAXNAMELEN		16
#define MAXHEALTH		100
#define MAXCARRY		2
#define MAXTEAMS		(TEAM_MAX-TEAM_ALPHA) // don't count neutral
#define numteams(a,b)	(m_multi(a, b) ? MAXTEAMS : MAXTEAMS/2)
#define isteam(a,b)		(a >= b && a <= TEAM_MAX-1)

#define REGENWAIT		3000
#define REGENTIME		1000
#define REGENHEAL		10

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
const char *serverinfotypes[] = {
	"",
	"host",
	"desc",
	"ping",
	"pl",
	"max",
	"game",
	"map",
	"time"
};

struct serverstatuses
{
	int type,				colour;		const char *icon;
} serverstatus[] = {
	{ SSTAT_OPEN,			0xFFFFFF,	"server" },
	{ SSTAT_LOCKED,			0xFF8800,	"serverlock" },
	{ SSTAT_PRIVATE,		0x8888FF,	"serverpriv" },
	{ SSTAT_FULL,			0xFF8888,	"serverfull" },
	{ SSTAT_UNKNOWN,		0x888888,	"serverunk" }
};
#endif

// inherited by fpsent and server clients
struct fpsstate
{
	int health, ammo[NUMGUNS];
	int lastgun, gunselect, gunstate[NUMGUNS], gunwait[NUMGUNS], gunlast[NUMGUNS];
	int lastdeath, lifesequence, lastspawn, lastrespawn, lastpain, lastregen;
	int ownernum, skill, spree;

	fpsstate() : lifesequence(0), ownernum(-1), skill(0), spree(0) {}
	~fpsstate() {}

	int bestgun()
	{
		int best = -1;
		loopi(NUMGUNS) if(guntype[i].rdelay > 0 && ammo[i] >= 0) best = i;
		return best;
	}

	bool canswitch(int gun, int millis)
	{
		if (isgun(gun) && (gunselect != gun) && ammo[gun] >= 0 && (guntype[gun].rdelay > 0 || ammo[gun] > 0) && (millis-gunlast[gun] >= gunwait[gun]) && (millis-gunlast[gunselect] >= gunwait[gunselect]))
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
			case TRIGGER:
			{
				return true;
				break;
			}
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

	void gunreset()
	{
		loopi(NUMGUNS)
		{
			gunstate[i] = GUNSTATE_IDLE;
			gunwait[i] = gunlast[i] = 0;
			ammo[i] = -1;
		}
		lastgun = gunselect = -1;
	}

	void setgunstate(int gun, int state, int delay, int millis)
	{
		gunstate[gun] = state;
		gunwait[gun] = delay;
		gunlast[gun] = millis;
	}

	void gunswitch(int gun, int millis)
	{
		if(gun != gunselect)
		{
			lastgun = gunselect;
			setgunstate(lastgun, GUNSTATE_SWITCH, GUNSWITCHDELAY, millis);
		}
		gunselect = gun;
		setgunstate(gunselect, GUNSTATE_SWITCH, GUNSWITCHDELAY, millis);
	}

	void gunreload(int gun, int amt, int millis)
	{
		setgunstate(gun, GUNSTATE_RELOAD, guntype[gun].rdelay, millis);
		ammo[gun] = amt;
	}

	void useitem(int millis, int type, int attr1, int attr2)
	{
		switch (type)
		{
			case TRIGGER:
			{
				break;
			}
			case WEAPON:
			{
				if (ammo[attr1] < 0) ammo[attr1] = 0;

				int carry = 0;
				loopi(NUMGUNS) if (ammo[i] >= 0 && guntype[i].rdelay > 0) carry++;
				if (carry > MAXCARRY)
				{
					if (gunselect != attr1 && guntype[gunselect].rdelay > 0)
					{
						ammo[gunselect] = -1;
						gunswitch(attr1, millis);
					}
					else loopi(NUMGUNS) if (ammo[i] >= 0 && i != attr1 && guntype[i].rdelay > 0)
					{
						ammo[i] = -1;
						break;
					}
				}
				else if(gunselect != attr1) gunswitch(attr1, millis);
				else setgunstate(attr1, GUNSTATE_RELOAD, guntype[attr1].rdelay ? guntype[attr1].rdelay : guntype[attr1].adelay, millis);
				ammo[attr1] = min(ammo[attr1] + (attr2 > 0 ? attr2 : guntype[attr1].add), guntype[attr1].max);
				break;
			}
			default: break;
		}
	}

	void respawn(int millis)
	{
		health = MAXHEALTH;
		lastdeath = lastpain = lastregen = spree = 0;
		lastspawn = millis;
		lastrespawn = -1;
		gunreset();
	}

	void spawnstate(int gamemode, int mutators)
	{
		if(m_insta(gamemode, mutators))
		{
			health = 1;
			lastgun = gunselect = GUN_RIFLE;
			loopi(NUMGUNS)
			{
				gunstate[i] = GUNSTATE_IDLE;
				ammo[i] = (i == GUN_RIFLE ? guntype[i].add : -1);
			}
		}
		else
		{
			health = MAXHEALTH;
			lastgun = gunselect = GUN_PISTOL;
			loopi(NUMGUNS)
			{
				gunstate[i] = GUNSTATE_IDLE;
				ammo[i] = (i == GUN_PISTOL || i == GUN_GL ? guntype[i].add : -1);
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
	BS_DEFEND,		// defend goal target
	BS_PURSUE,		// pursue goal target
	BS_ATTACK,		// attack goal target
	BS_INTEREST,	// interest in goal entity
	BS_MAX
};

static const int botframetimes[BS_MAX] = { 10, 50, 50, 10, 50 };

enum
{
	BT_NODE,
	BT_PLAYER,
	BT_ENTITY,
	BT_FLAG,
	BT_MAX
};

struct botstate
{
	int type, millis, expire, next, targtype, target, cycle;
	bool goal, override;

	botstate(int _type, int _millis) : type(_type), millis(_millis)
	{
		reset();
	}
	~botstate()
	{
	}

	void reset()
	{
		expire = cycle = 0;
		next = lastmillis;
		targtype = target = -1;
		goal = override = false;
	}
};

struct botinfo
{
	vector<botstate> state;
	vector<int> route, avoid;
	vec target, spot;
	int enemy;

	botinfo()
	{
		reset();
	}
	~botinfo()
	{
		state.setsize(0);
		route.setsize(0);
		avoid.setsize(0);
	}

	void reset()
	{
		state.setsize(0);
		route.setsize(0);
		avoid.setsize(0);
		addstate(BS_WAIT);
		spot = target = vec(0, 0, 0);
		enemy = NULL;
	}

	botstate &addstate(int type)
	{
		botstate bs(type, lastmillis);
		return state.add(bs);
	}

	void removestate(int index = -1)
	{
		if(index < 0 && state.length()) state.pop();
		else if(state.inrange(index)) state.remove(index);
		enemy = -1;
		if(!state.length()) addstate(BS_WAIT);
	}

	botstate &setstate(int type, bool pop = true)
	{
		if(pop) removestate();
		return addstate(type);
	}

	botstate &getstate(int idx = -1)
	{
		if(state.inrange(idx)) return state[idx];
		return state.last();
	}
};

struct fpsent : dynent, fpsstate
{
	int clientnum, privilege, lastupdate, plag, ping;
	bool attacking, reloading, useaction, obliterated;
	int attacktime, reloadtime, usetime;
	int lasttaunt, lastuse, lastusemillis, lastflag;
	int frags, deaths, totaldamage, totalshots;
	editinfo *edit;
    vec deltapos, newpos;
    float deltayaw, deltapitch, newyaw, newpitch;
    float deltaaimyaw, deltaaimpitch, newaimyaw, newaimpitch;
    int smoothmillis;
	int lastimpulse, oldnode, lastnode;
	int respawned, suicided;
	int wschan;
	botinfo *bot;

	string name, info;
	int team;

	fpsent() : clientnum(-1), privilege(PRIV_NONE), lastupdate(0), plag(0), ping(0), frags(0), deaths(0), totaldamage(0), totalshots(0), edit(NULL), smoothmillis(-1), lastimpulse(0), wschan(-1), bot(NULL)
	{
		name[0] = info[0] = 0;
		team = TEAM_NEUTRAL;
		respawn(-1);
	}
	~fpsent()
	{
		freeeditinfo(edit);
		if(bot) delete bot;
	}

	void hitpush(int damage, const vec &dir, fpsent *actor, int gun)
	{
		vec push(dir);
		push.mul(damage/weight);
		vel.add(push);
	}

	void stopactions()
	{
		attacking = reloading = useaction = false;
		attacktime = reloadtime = usetime = 0;
	}

	void stopmoving()
	{
		dynent::stopmoving();
		stopactions();
	}

	void setgun(int gun, int millis)
	{
		gunswitch(gun, millis);
		stopactions();
	}

	void respawn(int millis)
	{
		stopmoving();
		dynent::reset();
		fpsstate::respawn(millis);
		obliterated = false;
		lasttaunt = lastuse = lastusemillis = lastimpulse = 0;
		lastflag = respawned = suicided = -1;
	}
};

