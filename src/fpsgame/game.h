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
	S_GLFIRE, S_GLEXPL, S_GLHIT, S_FLFIRE, S_FLBURNING, S_FLBURN, S_RIFLE,
	S_ITEMPICKUP, S_ITEMSPAWN, 	S_REGEN,
	S_DAMAGE1, S_DAMAGE2, S_DAMAGE3, S_DAMAGE4, S_DAMAGE5, S_DAMAGE6, S_DAMAGE7, S_DAMAGE8,
	S_RESPAWN, S_CHAT, S_DENIED,
	//S_V_PISTOL, S_V_SG, S_V_CG, S_V_GL, S_V_FLAMER, S_V_RIFLE,
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
	LIGHT = ET_LIGHT,				// 1  radius, intensity or red, green, blue
	MAPMODEL = ET_MAPMODEL,			// 2  angle, idx
	PLAYERSTART = ET_PLAYERSTART,	// 3  angle, team, id
	ENVMAP = ET_ENVMAP,				// 4  radius
	PARTICLES = ET_PARTICLES,		// 5  type, [others]
	MAPSOUND = ET_SOUND,			// 6  idx, maxrad, minrad, volume
	SPOTLIGHT = ET_SPOTLIGHT,		// 7  radius
	WEAPON = ET_GAMESPECIFIC,		// 8  gun, ammo
	TELEPORT,						// 9  yaw, pitch, roll, push
	OBSOLETED,						// 10
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
	{ OBSOLETED,	59,		0,		0,		EU_NONE,		"obsoleted" },
	{ TRIGGER,		58,		16,		16,		EU_AUTO,		"trigger" },
	{ PUSHER,		58,		12,		12,		EU_AUTO,		"pusher" },
	{ FLAG,			48,		16,		16,		EU_NONE,		"flag" },
	{ CHECKPOINT,	48,		16,		16,		EU_NONE,		"checkpoint" }, // FIXME
	{ CAMERA,		48,		0,		0,		EU_NONE,		"camera" },
	{ WAYPOINT,		1,		8,		8,		EU_NONE,		"waypoint" },
	{ ANNOUNCER,	64,		0,		0,		EU_NONE,		"announcer" },
	{ CONNECTION,	70,		8,		8,		EU_NONE,		"connection" },
};

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

#define SGRAYS			20
#define SGSPREAD		50
#define GUNSWITCHDELAY	800
#define PLAYERHEIGHT	15.f

enum
{
	GUN_PISTOL = 0,
	GUN_SG,
	GUN_CG,
	GUN_GL,
	GUN_FLAMER,
	GUN_RIFLE,
	GUN_MAX
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
	int	info, 		anim,			sound, 		esound, 	fsound,		rsound,		ssound,
		add,	charge,	max,adelay,	rdelay,	damage,	speed,	power,	time,	kick,	wobble,	scale,
		size,	explode; float offset,	elasticity,	relativity,	waterfric,	weight;
	const char *name,		*item,						*vwep;
} guntype[GUN_MAX] =
{
	{
		GUN_PISTOL,	ANIM_PISTOL,	S_PISTOL,	-1,			S_WHIRR,	-1,			S_ITEMSPAWN,
		12,		12,		64,	250,	1000,	20,		0,		0,		0,		-10,    10,		0,
		1,		0,				1.0f,	0.33f,		0.5f,		2.0f,		75.f,
				"pistol",	"weapons/pistol/item",		"weapons/pistol/vwep"
	},
	{
		GUN_SG,		ANIM_SHOTGUN,	S_SG,		-1,			S_WHIRR,	-1,			S_ITEMSPAWN,
		1,		8,		16,	600,	1200,	10,		0,		0,		0,		-30,    30, 	0,
		1,		0,				1.0f,	0.33f,		0.5f,		2.0f,		75.f,
				"shotgun",	"weapons/shotgun/item",		"weapons/shotgun/vwep"
	},
	{
		GUN_CG,		ANIM_CHAINGUN,	S_CG,		-1,			S_WHIRR,	-1,			S_ITEMSPAWN,
		40,		40,		120,100,    1000,	15,		0,		0,		0,		-5,	     5,		0,
		1,		0,				1.0f,	0.33f,		0.5f,		2.0f,		75.f,
				"chaingun",	"weapons/chaingun/item",	"weapons/chaingun/vwep"
	},
	{
		GUN_GL,		ANIM_GRENADES,	S_GLFIRE,	S_GLEXPL,	S_WHIZZ,	S_GLHIT,	S_ITEMSPAWN,
		2,		0,		4,	1500,	0,		200,	150,	1000,	3000,	-15,    10,		8,
		3,		64,				1.0f,	0.33f,		0.5f,		2.0f,		75.f,
				"grenades",	"weapons/grenades/item",	"weapons/grenades/vwep"
	},
	{
		GUN_FLAMER,	ANIM_FLAMER,	S_FLFIRE,	S_FLBURNING,S_FLBURN,	S_FLBURNING,S_ITEMSPAWN,
		50,		50,		200,100, 	2000,	15,		100,	0,		3000,	-1,		 1,		8,
		24,		28,				0.5f,	0.1f,		0.25f,		1.5f,		50.f,
				"flamer",	"weapons/flamer/item",		"weapons/flamer/vwep"
	},
	{
		GUN_RIFLE,	ANIM_RIFLE,		S_RIFLE,	-1,			S_WHIRR,	-1,			S_ITEMSPAWN,
		1,		5,		10,	800,	1600,	100,	0,		0,		0,		-35,  	25,		0,
		1,		0,				1.0f,	0.33f,		0.5f,		2.0f,		75.f,
				"rifle",	"weapons/rifle/item",		"weapons/rifle/vwep"
	}
};
#define isgun(gun)	(gun > -1 && gun < GUN_MAX)

enum
{
	HIT_NONE 	= 0,
	HIT_LEGS	= 1<<2,
	HIT_TORSO	= 1<<3,
	HIT_HEAD	= 1<<4,
	HIT_BURN	= 1<<5,
	HIT_EXPLODE	= 1<<6,
	HIT_MELT	= 1<<7,
	HIT_FALL	= 1<<8,
	HIT_NUM		= 7
};

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

enum
{
	G_M_NONE	= 0,
	G_M_TEAM	= 1<<2,
	G_M_INSTA	= 1<<3,
	G_M_DUEL	= 1<<4,
	G_M_PROG	= 1<<5,
	G_M_MULTI	= 1<<6,
	G_M_DLMS	= 1<<7,
	G_M_MAYHEM	= 1<<8,
	G_M_NOITEMS	= 1<<9,
	G_M_ALL		= G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_PROG|G_M_MULTI|G_M_DLMS|G_M_MAYHEM|G_M_NOITEMS,
	G_M_FIGHT	= G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_MULTI|G_M_DLMS|G_M_NOITEMS,
	G_M_STF		= G_M_TEAM|G_M_INSTA|G_M_PROG|G_M_MULTI|G_M_MAYHEM|G_M_NOITEMS,
	G_M_CTF		= G_M_TEAM|G_M_INSTA|G_M_PROG|G_M_MULTI|G_M_MAYHEM|G_M_NOITEMS,
	G_M_NUM		= 8
};

struct gametypes
{
	int	type,			mutators,		implied;			const char *name;
} gametype[] = {
	{ G_DEMO,			0,				0,					"Demo" },
	{ G_LOBBY,			0,				0,					"Lobby" },
	{ G_EDITMODE,		0,				0,					"Editing" },
	{ G_MISSION,		0,				0,					"Mission" },
	{ G_DEATHMATCH,		G_M_FIGHT,		0,					"Deathmatch" },
	{ G_STF,			G_M_STF,		G_M_TEAM,			"Secure the Flag" },
	{ G_CTF,			G_M_CTF,		G_M_TEAM,			"Capture the Flag" },
}, mutstype[] = {
	{ G_M_TEAM,			G_M_ALL,		0,					"Team" },
	{ G_M_INSTA,		G_M_ALL,		G_M_NOITEMS,		"Instagib" },
	{ G_M_DUEL,			G_M_ALL,		0,					"Duel" },
	{ G_M_PROG,			G_M_ALL,		0,					"Progressive" },
	{ G_M_MULTI,		G_M_ALL,		G_M_TEAM,			"Multi-sided" },
	{ G_M_DLMS,			G_M_ALL,		G_M_DUEL,			"Last Man Standing" },
	{ G_M_MAYHEM,		G_M_ALL,		0,					"Mayhem" },
	{ G_M_NOITEMS,		G_M_ALL,		0,					"No Items" },
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

#define m_team(a,b)			((b & G_M_TEAM) || (gametype[a].implied & G_M_TEAM))
#define m_insta(a,b)		((b & G_M_INSTA) || (gametype[a].implied & G_M_INSTA))
#define m_duel(a,b)			((b & G_M_DUEL) || (gametype[a].implied & G_M_DUEL))
#define m_prog(a,b)			((b & G_M_PROG) || (gametype[a].implied & G_M_PROG))
#define m_multi(a,b)		((b & G_M_MULTI) || (gametype[a].implied & G_M_MULTI))
#define m_dlms(a,b)			((b & G_M_DLMS) || (gametype[a].implied & G_M_DLMS))
#define m_mayhem(a,b)		((b & G_M_MAYHEM) || (gametype[a].implied & G_M_MAYHEM))
#define m_noitems(a,b)		((b & G_M_NOITEMS) || (gametype[a].implied & G_M_NOITEMS))

// network messages codes, c2s, c2c, s2c
enum
{
	SV_INITS2C = 0, SV_INITC2S, SV_POS, SV_TEXT, SV_COMMAND, SV_ANNOUNCE, SV_SOUND, SV_CDIS,
	SV_SHOOT, SV_EXPLODE, SV_SUICIDE,
	SV_DIED, SV_DAMAGE, SV_SHOTFX,
	SV_TRYSPAWN, SV_SPAWNSTATE, SV_SPAWN, SV_FORCEDEATH,
	SV_DROP, SV_GUNSELECT, SV_TAUNT,
	SV_MAPCHANGE, SV_MAPVOTE, SV_ITEMSPAWN, SV_ITEMUSE, SV_EXECLINK,
	SV_PING, SV_PONG, SV_CLIENTPING,
	SV_TIMEUP, SV_NEWGAME, SV_ITEMACC,
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
		SV_SHOOT, 0, SV_EXPLODE, 0, SV_SUICIDE, 3,
		SV_DIED, 8, SV_DAMAGE, 10, SV_SHOTFX, 9,
		SV_TRYSPAWN, 2, SV_SPAWNSTATE, 13, SV_SPAWN, 4, SV_FORCEDEATH, 2,
		SV_DROP, 4, SV_GUNSELECT, 0, SV_TAUNT, 2,
		SV_MAPCHANGE, 0, SV_MAPVOTE, 0, SV_ITEMSPAWN, 2, SV_ITEMUSE, 0, SV_EXECLINK, 3,
		SV_PING, 2, SV_PONG, 2, SV_CLIENTPING, 2,
		SV_TIMEUP, 2, SV_NEWGAME, 1, SV_ITEMACC, 0,
		SV_SERVMSG, 0, SV_ITEMLIST, 0, SV_RESUME, 0,
        SV_EDITMODE, 2, SV_EDITENT, 10, SV_EDITLINK, 4, SV_EDITVAR, 0, SV_EDITF, 16, SV_EDITT, 16, SV_EDITM, 15, SV_FLIP, 14, SV_COPY, 14, SV_PASTE, 14, SV_ROTATE, 15, SV_REPLACE, 16, SV_DELCUBE, 14, SV_REMIP, 1, SV_NEWMAP, 2, SV_GETMAP, 1, SV_SENDMAP, 0,
		SV_MASTERMODE, 2, SV_KICK, 2, SV_CLEARBANS, 1, SV_CURRENTMASTER, 3, SV_SPECTATOR, 3, SV_SETMASTER, 0, SV_SETTEAM, 0, SV_APPROVEMASTER, 2,
		SV_FLAGS, 0, SV_FLAGINFO, 0,
        SV_DROPFLAG, 0, SV_SCOREFLAG, 5, SV_RETURNFLAG, 3, SV_TAKEFLAG, 3, SV_RESETFLAG, 2, SV_INITFLAGS, 0,
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

enum { TEAM_NEUTRAL = 0, TEAM_ALPHA, TEAM_BETA, TEAM_DELTA, TEAM_GAMMA, TEAM_ENEMY, TEAM_MAX };
struct teamtypes
{
	int	type,		colour;	const char *name,	*tpmdl,			*fpmdl,				*flag,			*icon,			*chat;
} teamtype[] = {
	{ TEAM_NEUTRAL,	0x2F2F2F,	"neutral",		"player",		"player/vwep",		"flag",			"team",			"\fa" },
	{ TEAM_ALPHA,	0x2222FF,	"alpha",		"player/alpha",	"player/alpha/vwep","flag/alpha",	"teamalpha",	"\fb" },
	{ TEAM_BETA,	0xFF2222,	"beta",			"player/beta",	"player/beta/vwep",	"flag/beta",	"teambeta",		"\fr" },
	{ TEAM_DELTA,	0xFFFF22,	"delta",		"player/delta",	"player/delta/vwep","flag/delta",	"teamdelta",	"\fy" },
	{ TEAM_GAMMA,	0x22FF22,	"gamma",		"player/gamma",	"player/gamma/vwep","flag/gamma",	"teamgamma",	"\fg" },
	{ TEAM_ENEMY,	0xFFFFFF,	"enemy",		"player",		"player/vwep",		"flag",			"team",			"\fa" }
};

#define MAXNAMELEN		16
#define MAXHEALTH		100
#define MAXCARRY		2
#define MAXTEAMS		TEAM_GAMMA
#define numteams(a,b)	(m_multi(a, b) ? MAXTEAMS : MAXTEAMS/2)
#define isteam(a,b)		(a >= b && a <= MAXTEAMS)

enum { ENEMY_BOT = 0, ENEMY_BSOLDIER, ENEMY_RSOLDIER, ENEMY_YSOLDIER, ENEMY_GSOLDIER, ENEMY_MAX };
struct enemytypes
{
	int type,			colour; const char *name,	*mdl;
} enemytype[] = {
	{ ENEMY_BOT,		0x2F2F2F,	"neutral",		"player" },
	{ ENEMY_BSOLDIER,	0x2222FF,	"alpha",		"player/alpha" },
	{ ENEMY_RSOLDIER,	0xFF2222,	"beta",			"player/beta" },
	{ ENEMY_YSOLDIER,	0xFFFF22,	"delta",		"player/delta" },
	{ ENEMY_GSOLDIER,	0x22FF22,	"gamma",		"player/gamma" },
};

#define isenemy(a)		(a >= 0 && a <= ENEMY_MAX-1)

#define REGENWAIT		3000
#define REGENTIME		1000
#define REGENHEAL		10

enum
{
	SAY_NONE = 0,
	SAY_ACTION = 1<<2,
	SAY_TEAM = 1<<3,
	SAY_NUM = 2
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

SVARG(defaultmap, "overseer");
VARG(defaultmode, G_LOBBY, G_LOBBY, G_MAX-1);
VARG(defaultmuts, G_M_NONE, G_M_NONE, G_M_ALL);

VARG(teamdamage, 0, 1, 1);
VARG(entspawntime, 0, 0, 60);
VARG(timelimit, 0, 10, 60);
VARG(ctflimit, 0, 10, 100);
VARG(stflimit, 0, 1, 1);

VARG(spawngun, 0, GUN_PISTOL, GUN_MAX-1);
VARG(instaspawngun, 0, GUN_RIFLE, GUN_MAX-1);

VARG(botbalance, 0, 4, 16);
VARG(botratio, 0, 2, 10);
VARG(botminskill, 0, 35, 100);
VARG(botmaxskill, 0, 85, 100);

FVARG(gravityscale, 1.f);
FVARG(jumpscale, 1.f);
FVARG(speedscale, 1.f);

// inherited by fpsent and server clients
struct fpsstate
{
	int health, ammo[GUN_MAX], entid[GUN_MAX];
	int lastgun, gunselect, gunstate[GUN_MAX], gunwait[GUN_MAX], gunlast[GUN_MAX];
	int lastdeath, lifesequence, lastspawn, lastrespawn, lastpain, lastregen;
	int ownernum, skill, spree;

	fpsstate() : lifesequence(0), ownernum(-1), skill(0), spree(0) {}
	~fpsstate() {}

	int hasgun(int gun, int level = 0, int exclude = -1)
	{
		if(isgun(gun) && gun != exclude)
		{
			if(ammo[gun] > 0 || (guntype[gun].rdelay > 0 && !ammo[gun])) switch(level)
			{
				case 0: default: return true; break; // has gun at all
				case 1: if(guntype[gun].rdelay > 0) return true; break; // only carriables
				case 2: if(ammo[gun] > 0) return true; break; // only with actual ammo
				case 3: if(ammo[gun] > 0 && guntype[gun].rdelay > 0) return true; break; // only carriables with actual ammo
			}
		}
		return false;
	}

	int bestgun()
	{
		int best = -1;
		loopi(GUN_MAX) if(hasgun(i, 1)) best = i;
		return best;
	}

	int carry()
	{
		int carry = 0;
		loopi(GUN_MAX) if(hasgun(i, 1)) carry++;
		return carry;
	}

	int drop(int exclude = -1)
	{
		int gun = -1;
		if(hasgun(gunselect, 1, exclude)) gun = gunselect;
		else
		{
			loopi(GUN_MAX) if(hasgun(i, 1, exclude))
			{
				gun = i;
				break;
			}
		}
		return gun;
	}

	void gunreset(bool full = false)
	{
		loopi(GUN_MAX)
		{
			if(full)
			{
				gunstate[i] = GUNSTATE_IDLE;
				gunwait[i] = gunlast[i] = 0;
				ammo[i] = -1;
			}
			entid[i] = -1;
		}
		lastgun = gunselect = -1;
	}

	void setgunstate(int gun, int state, int delay, int millis)
	{
		if(isgun(gun))
		{
			gunstate[gun] = state;
			gunwait[gun] = delay;
			gunlast[gun] = millis;
		}
	}

	void gunswitch(int gun, int millis)
	{
		lastgun = gunselect;
		setgunstate(lastgun, GUNSTATE_SWITCH, GUNSWITCHDELAY, millis);
		gunselect = gun;
		setgunstate(gun, GUNSTATE_SWITCH, GUNSWITCHDELAY, millis);
	}

	bool gunwaited(int gun, int millis)
	{
		return !isgun(gun) || millis-gunlast[gun] >= gunwait[gun];
	}

	bool canswitch(int gun, int millis)
	{
		if(gun != gunselect && hasgun(gun) && gunwaited(gun, millis) && gunwaited(gunselect, millis))
			return true;
		return false;
	}

	bool canshoot(int gun, int millis)
	{
		if(hasgun(gun) && ammo[gun] > 0 && gunwaited(gun, millis))
			return true;
		return false;
	}

	bool canreload(int gun, int millis)
	{
		if(hasgun(gun, 1) && ammo[gun] < guntype[gun].charge && gunwaited(gun, millis) && gunwaited(gunselect, millis))
			return true;
		return false;
	}

	bool canuse(int type, int attr1, int attr2, int millis)
	{
		switch(type)
		{
			case TRIGGER:
			{
				return true;
				break;
			}
			case WEAPON:
			{ // can't use when reloading or firing
				if(isgun(attr1) && ammo[attr1] < guntype[attr1].max && gunwaited(attr1, millis) && gunwaited(gunselect, millis))
					return true;
				break;
			}
			default: break;
		}
		return false;
	}

	void useitem(int millis, int id, int type, int attr1, int attr2)
	{
		switch(type)
		{
			case TRIGGER: break;
			case WEAPON:
			{
				gunswitch(attr1, millis);
				ammo[attr1] = clamp(max(ammo[attr1], 0)+(attr2 > 0 ? attr2 : guntype[attr1].add), guntype[attr1].add, guntype[attr1].max);
				entid[attr1] = id;
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
		gunreset(true);
	}

	void spawnstate(int spawngun)
	{
		health = MAXHEALTH;
		lastgun = gunselect = spawngun;
		loopi(GUN_MAX)
		{
			gunstate[i] = GUNSTATE_IDLE;
			gunwait[i] = gunlast[i] = 0;
			ammo[i] = (i == spawngun || guntype[i].rdelay <= 0) ? guntype[i].add : -1;
			entid[i] = -1;
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

#ifndef STANDALONE
enum
{
	ST_NONE		= 0,
	ST_REQS		= 1<<2,
	ST_CAMERA	= 1<<3,
	ST_CURSOR	= 1<<4,
	ST_GAME		= 1<<5,
	ST_SPAWNS	= 1<<6,
	ST_DEFAULT	= ST_REQS|ST_CAMERA|ST_CURSOR|ST_GAME,
	ST_VIEW		= ST_CURSOR|ST_CAMERA,
	ST_ALL		= ST_REQS|ST_CAMERA|ST_CURSOR|ST_GAME|ST_SPAWNS,
	ST_NUM		= 5
};

struct fpsentity : extentity
{
	int schan, lastemit;
	bool mark;

	fpsentity() : schan(-1), lastemit(0), mark(false) {}
	~fpsentity()
	{
		if(issound(schan)) removesound(schan);
		schan = -1;
	}
};
enum { ITEM_ENT = 0, ITEM_PROJ, ITEM_MAX };
struct actitem
{
	int type, target;
	float score;

	actitem() : type(ITEM_ENT), target(-1), score(0.f) {}
	~actitem() {}
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

static const int botframetimes[BS_MAX] = { 100, 50, 50, 25, 50 };

enum
{
	BT_NODE,
	BT_PLAYER,
	BT_ENTITY,
	BT_FLAG,
	BT_DROP,
	BT_MAX
};

struct botstate
{
	int type, millis, expire, next, targtype, target, cycle;
	bool goal, override, defers;

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
		next = millis;
		if(type == BS_WAIT)
			next += rnd(1500) + 1500;
		targtype = target = -1;
		goal = override = false;
		defers = true;
	}
};

struct botinfo
{
	vector<botstate> state;
	vector<int> route, avoid;
	vec target, spot;
	int enemy, gunpref, lastreq;

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
		gunpref = rnd(GUN_MAX-1)+1;
		spot = target = vec(0, 0, 0);
		enemy = NULL;
		lastreq = 0;
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
	int clientnum, privilege, lastupdate, lastpredict, plag, ping;
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

	string name, info, obit;
	int team;

	fpsent() : clientnum(-1), privilege(PRIV_NONE), lastupdate(0), lastpredict(0), plag(0), ping(0), frags(0), deaths(0), totaldamage(0), totalshots(0), edit(NULL), smoothmillis(-1), lastimpulse(0), wschan(-1), bot(NULL)
	{
		name[0] = info[0] = obit[0] = 0;
		team = TEAM_NEUTRAL;
		respawn(-1);
	}
	~fpsent()
	{
		freeeditinfo(edit);
		if(bot) delete bot;
		if(issound(wschan)) removesound(wschan);
		wschan = -1;
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

	void respawn(int millis)
	{
		stopmoving();
		dynent::reset();
		fpsstate::respawn(millis);
		obliterated = false;
		lasttaunt = lastuse = lastusemillis = lastimpulse = 0;
		lastflag = respawned = suicided = -1;
		obit[0] = 0;
	}

	void resetstate(int millis)
	{
		respawn(millis);
		oldnode = lastnode = -1;
		frags = deaths = totaldamage = totalshots = 0;
		state = CS_DEAD;
	}
};

enum { PRJ_SHOT = 0, PRJ_GIBS, PRJ_DEBRIS, PRJ_ENT };

struct projent : dynent
{
	vec from, to;
	int lifetime, waittime;
	float movement, roll;
	bool local, beenused;
	int projtype;
	float elasticity, relativity, waterfric;
	int ent, attr1, attr2, attr3, attr4, attr5;
	int schan, id;
	entitylight light;
	fpsent *owner;
	const char *mdl;

	projent() : projtype(PRJ_SHOT), id(-1), mdl(NULL)
	{
		schan = -1;
		reset();
	}
	~projent()
	{
		removetrackedparticles(this);
		removetrackedsounds(this);
		if(issound(schan)) removesound(schan);
		schan = -1;
	}

	void reset()
	{
		type = ENT_BOUNCE;
		state = CS_ALIVE;
		lifetime = waittime = ent = attr1 = attr2 = attr3 = attr4 = attr5 = 0;
		schan = id = -1;
		movement = roll = 0.f;
		beenused = false;
		physent::reset();
	}

	bool ready()
	{
		if(owner && !beenused && waittime <= 0 && state != CS_DEAD)
			return true;
		return false;
	}
};
#endif
