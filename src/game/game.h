#ifndef __GAME_H__
#define __GAME_H__

#include "engine.h"

#define GAMEID				"bfa"
#define GAMEVERSION			159
#define DEMO_VERSION		GAMEVERSION

#define MAXBOTS 100
#define MAXPLAYERS (MAXCLIENTS + MAXBOTS)

// network quantization scale
#define DMF 16.0f			// for world locations
#define DNF 100.0f			// for normalized vectors
#define DVELF 1.0f			// for playerspeed based velocity vectors

#ifdef GAMESERVER
#define GVAR(name)			(sv_##name)
#else
#define GVAR(name)			(name)
#endif
enum
{
	S_JUMP = S_GAMESPECIFIC, S_IMPULSE, S_LAND, S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6, S_DIE1, S_DIE2,
	S_SPLASH1, S_SPLASH2, S_UNDERWATER,
	S_SPLAT, S_SPLOSH, S_DEBRIS, S_TINK, S_RICOCHET, S_WHIZZ, S_WHIRR, S_EXPLODE, S_ENERGY, S_HUM, S_BURN, S_BURNING, S_BZAP, S_BZZT,
	S_RELOAD, S_SWITCH, S_PISTOL, S_SG, S_SMG, S_GLFIRE, S_FLFIRE, S_PLASMA, S_RIFLE, S_PAINT,
	S_ITEMPICKUP, S_ITEMSPAWN, S_REGEN,
	S_DAMAGE1, S_DAMAGE2, S_DAMAGE3, S_DAMAGE4, S_DAMAGE5, S_DAMAGE6, S_DAMAGE7, S_DAMAGE8,
	S_RESPAWN, S_CHAT, S_DENIED,
	S_V_FLAGSECURED, S_V_FLAGOVERTHROWN,
    S_V_FLAGPICKUP, S_V_FLAGDROP, S_V_FLAGRETURN, S_V_FLAGSCORE, S_V_FLAGRESET,
	S_V_FIGHT, S_V_CHECKPOINT, S_V_ONEMINUTE, S_V_HEADSHOT,
	S_V_SPREE1, S_V_SPREE2, S_V_SPREE3, S_V_SPREE4,
	S_V_YOUWIN, S_V_YOULOSE, S_V_MCOMPLETE, S_V_FRAGGED, S_V_OWNED, S_V_DENIED,
	S_MAX
};

enum								// entity types
{
	NOTUSED = ET_EMPTY,
	LIGHT = ET_LIGHT,
	MAPMODEL = ET_MAPMODEL,
	PLAYERSTART = ET_PLAYERSTART,
	ENVMAP = ET_ENVMAP,
	PARTICLES = ET_PARTICLES,
	MAPSOUND = ET_SOUND,
	SPOTLIGHT = ET_SPOTLIGHT,
	WEAPON = ET_GAMESPECIFIC,
	TELEPORT,
	ACTOR,
	TRIGGER,
	PUSHER,
	FLAG,
	CHECKPOINT,
	CAMERA,
	WAYPOINT,
	MAXENTTYPES
};

enum { EU_NONE = 0, EU_ITEM, EU_AUTO, EU_ACT, EU_MAX };

enum { TR_TOGGLE = 0, TR_LINK, TR_SCRIPT, TR_MAX };
enum { TA_MANUAL = 0, TA_AUTO, TA_ACTION, TA_MAX };
#define TRIGSTATE(a,b) (b ? !a : a)

struct enttypes
{
	int	type,			links,	radius,	usetype,
			canlink,
			reclink;
	bool	noisy;	const char *name,
			*attrs[5];
};
#ifdef GAMESERVER
enttypes enttype[] = {
	{
		NOTUSED,		0,		0,		EU_NONE,
			0,
			0,
			true,				"none",
			{ "",		"",			"",			"",			"" }
	},
	{
		LIGHT,			59,		0,		EU_NONE,
			inttobit(SPOTLIGHT),
			inttobit(SPOTLIGHT),
			false,				"light",
			{ "radius",	"red",		"green",	"blue",		"" }
	},
	{
		MAPMODEL,		58,		0,		EU_NONE,
			inttobit(TRIGGER),
			inttobit(TRIGGER),
			false,				"mapmodel",
			{ "id",		"yaw",		"pitch",	"roll",		"flags" }
	},
	{
		PLAYERSTART,	59,		0,		EU_NONE,
			0,
			0,
			false,				"playerstart",
			{ "team",	"yaw",		"pitch",		"",			"" }
	},
	{
		ENVMAP,			0,		0,		EU_NONE,
			0,
			0,
			false,				"envmap",
			{ "radius",	"",			"",			"",			"" }
	},
	{
		PARTICLES,		59,		0,		EU_NONE,
			inttobit(TELEPORT)|inttobit(TRIGGER)|inttobit(PUSHER),
			inttobit(TRIGGER)|inttobit(PUSHER),
			false,				"particles",
			{ "type",	"a",		"b",		"c",		"d" }
	},
	{
		MAPSOUND,		58,		0,		EU_NONE,
			inttobit(TELEPORT)|inttobit(TRIGGER)|inttobit(PUSHER),
			inttobit(TRIGGER)|inttobit(PUSHER),
			false,				"sound",
			{ "id",		"maxrad",	"minrad",	"volume",	"flags" }
	},
	{
		SPOTLIGHT,		59,		0,		EU_NONE,
			inttobit(LIGHT),
			inttobit(LIGHT),
			false,				"spotlight",
			{ "radius",	"",			"",			"",			"" }
	},
	{
		WEAPON,			59,		16,		EU_ITEM,
			0,
			0,
			false,				"weapon",
			{ "id",		"flags",	"",			"",			"" }
	},
	{
		TELEPORT,		50,		12,		EU_AUTO,
			inttobit(MAPSOUND)|inttobit(PARTICLES)|inttobit(TELEPORT),
			inttobit(MAPSOUND)|inttobit(PARTICLES),
			false,				"teleport",
			{ "yaw",	"pitch",	"push",		"radius",	"colour" }
	},
	{
		ACTOR,			59,		0,		EU_NONE,
			0,
			0,
			false,				"actor",
			{ "",		"",			"",			"",			"" }
	},
	{
		TRIGGER,		58,		16,		EU_AUTO,
			inttobit(MAPMODEL)|inttobit(MAPSOUND)|inttobit(PARTICLES),
			inttobit(MAPMODEL)|inttobit(MAPSOUND)|inttobit(PARTICLES),
			false,				"trigger",
			{ "id",		"type",		"action",	"radius",	"state" }
	},
	{
		PUSHER,			58,		12,		EU_AUTO,
			inttobit(MAPSOUND)|inttobit(PARTICLES),
			inttobit(MAPSOUND)|inttobit(PARTICLES),
			false,				"pusher",
			{ "zpush",	"ypush",	"xpush",	"radius",	"" }
	},
	{
		FLAG,			48,		32,		EU_NONE,
			inttobit(FLAG),
			0,
			false,				"flag",
			{ "team",	"yaw",		"pitch",	"mode",		"" }
	},
	{
		CHECKPOINT,		48,		16,		EU_NONE,
			inttobit(CHECKPOINT),
			0,
			false,				"checkpoint",
			{ "id",		"",			"",			"",			"" }
	},
	{
		CAMERA,			48,		0,		EU_NONE,
			inttobit(CAMERA),
			0,
			false,				"camera",
			{ "id",		"mindist",	"maxdist",	"",			"" }
	},
	{
		WAYPOINT,		1,		16,		EU_NONE,
			inttobit(WAYPOINT),
			0,
			true,				"waypoint",
			{ "flags",	"",			"",			"",			"" }
	}
};
#else
extern enttypes enttype[];
#endif

enum
{
	ANIM_PAIN = ANIM_GAMESPECIFIC, ANIM_JUMP, ANIM_IMPULSE, ANIM_SINK,
	ANIM_EDIT, ANIM_LAG, ANIM_SWITCH, ANIM_TAUNT, ANIM_WIN, ANIM_LOSE,
	ANIM_CROUCH, ANIM_CRAWL_FORWARD, ANIM_CRAWL_BACKWARD, ANIM_CRAWL_LEFT, ANIM_CRAWL_RIGHT,
    ANIM_PISTOL, ANIM_PISTOL_SHOOT, ANIM_PISTOL_RELOAD,
    ANIM_SHOTGUN, ANIM_SHOTGUN_SHOOT, ANIM_SHOTGUN_RELOAD,
    ANIM_SMG, ANIM_SMG_SHOOT, ANIM_SMG_RELOAD,
    ANIM_GRENADE, ANIM_GRENADE_THROW, ANIM_GREANDES_RELOAD, ANIM_GRENADE_POWER,
    ANIM_FLAMER, ANIM_FLAMER_SHOOT, ANIM_FLAMER_RELOAD,
    ANIM_PLASMA, ANIM_PLASMA_SHOOT, ANIM_PLASMA_RELOAD,
    ANIM_RIFLE, ANIM_RIFLE_SHOOT, ANIM_RIFLE_RELOAD,
    ANIM_PAINTGUN, ANIM_PAINTGUN_SHOOT, ANIM_PAINTGUN_RELOAD,
    ANIM_VWEP, ANIM_SHIELD, ANIM_POWERUP,
    ANIM_MAX
};

#define WEAPSWITCHDELAY	750
#define EXPLOSIONSCALE	16.f

enum
{
	WEAPON_PISTOL = 0,
	WEAPON_SG,
	WEAPON_SMG,
	WEAPON_FLAMER,
	WEAPON_PLASMA,
	WEAPON_RIFLE,
	WEAPON_GRENADE,
	WEAPON_TOTAL, // end of selectable weapon set
	WEAPON_PAINT = WEAPON_TOTAL,
	WEAPON_MAX // end of superimposed weapon set
};
#define isweap(a)		(a > -1 && a < WEAPON_MAX)

enum
{
	WEAPFLAG_NONE = 0,
	WEAPFLAG_FORCED = 1<<0, // forced spawned
};

enum
{
	WPSTATE_IDLE = 0,
	WPSTATE_SHOOT,
	WPSTATE_RELOAD,
	WPSTATE_POWER,
	WPSTATE_SWITCH,
	WPSTATE_PICKUP,
	WPSTATE_WAIT,
	WPSTATE_MAX
};

enum
{
	IMPACT_GEOM		= 1<<0,
	BOUNCE_GEOM		= 1<<1,
	IMPACT_PLAYER	= 1<<2,
	BOUNCE_PLAYER	= 1<<3,
	COLLIDE_TRACE	= 1<<4,
	COLLIDE_OWNER	= 1<<5,
	COLLIDE_GEOM	= IMPACT_GEOM | BOUNCE_GEOM,
	COLLIDE_PLAYER	= IMPACT_PLAYER | BOUNCE_PLAYER,
};

struct weaptypes
{
	int	info, 				anim,				kick,	wobble,		colour,
			sound, 		esound, 	fsound,		rsound,
			add,	max,	adelay,	rdelay,	damage,	speed,	power,	time,
			delay,	explode,	rays,	spread,	zdiv,	collide;
	bool	radial,	taper,	extinguish,	reloads,	zooms,		fullauto;
	float	elasticity,	reflectivity,	relativity,	waterfric,	weight,	partsize,	partlen,	hitpush;
	const char
			*name, 		*text,	*item,						*vwep,
			*proj;
};
#ifdef GAMESERVER
weaptypes weaptype[WEAPON_MAX] =
{
	{
		WEAPON_PISTOL,		ANIM_PISTOL,		-20,	15,			0x999999,
			S_PISTOL,	S_BZAP,		S_WHIZZ,	-1,
			10,		10,		100,    1000,	25,		2500,	0,		2000,
			0,		0,			1,		1,		1,		IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,
			false,	false,	false,		true,		false,		false,
			0.f,		0.f,			0.05f,		2.0f,		0.f,	0.45f,		16.f,		150.f,
			"pistol",	"\fa",	"weapons/pistol/item",		"weapons/pistol/vwep",
			""
	},
	{
		WEAPON_SG,			ANIM_SHOTGUN,		-50,    35,			0xFFFF22,
			S_SG,		S_BZAP,		S_WHIZZ,	S_RICOCHET,
			1,		8,		500,	1250,	15,		2500,	0,		1000,
			0,		0,			20,		40,		1,		BOUNCE_GEOM|IMPACT_PLAYER|COLLIDE_TRACE|COLLIDE_OWNER,
			false,	false,	false,		true,		false,		false,
			0.5f,		50.f,			0.05f,		2.0f,		30.f,	0.35f,		48.f,		25.f, // remember, this is PER ray
			"shotgun",	"\fy",	"weapons/shotgun/item",		"weapons/shotgun/vwep",
			""
	},
	{
		WEAPON_SMG,			ANIM_SMG,	-10,	5,					0xFFAA22,
			S_SMG,		S_BZAP,		S_WHIZZ,	S_RICOCHET,
			40,		40,		75,    1500,	20,		3000,	0,		1000,
			0,		0,			1,		5,		4,		BOUNCE_GEOM|IMPACT_PLAYER|COLLIDE_TRACE|COLLIDE_OWNER,
			false,	false,	false,		true,		false,		true,
			0.75f,		30.f,			0.05f,		2.0f,		0.f,	0.25f,		48.f,		100.f,
			"smg",		"\fo",	"weapons/smg/item",			"weapons/smg/vwep",
			""
	},
	{
		WEAPON_FLAMER,		ANIM_FLAMER,		-3,		1,			0xFF2222,
			S_FLFIRE,	S_BURN,		S_BURNING,	-1,
			50,		50,		100, 	2000,	15,		200,	0,		500,
			0,		32,			1,		5,		2,		BOUNCE_GEOM|BOUNCE_PLAYER|COLLIDE_OWNER,
			true,	false,	true,		true,		false,		true,
			0.15f,		45.f,			0.25f,		1.5f,		25.f,	36.f,		0.f,		25.f,
			"flamer",	"\fr",	"weapons/flamer/item",		"weapons/flamer/vwep",
			""
	},
	{
		WEAPON_PLASMA,		ANIM_PLASMA,		-10,	5,			0x22FFFF,
			S_PLASMA,	S_ENERGY,	S_HUM,		-1,
			20,		20,		350,	1000,	35,		2000,	0,		1000,
			0,		24,			1,		5,		0,		IMPACT_GEOM|IMPACT_PLAYER,
			true,	true,	true,		true,		false,		true,
			0.f,		0.f,			0.125f,		1.0f,		0.f,	16.f,		0.f,		100.f,
			"plasma",	"\fc",	"weapons/plasma/item",		"weapons/plasma/vwep",
			""
	},
	{
		WEAPON_RIFLE,		ANIM_RIFLE,			-30,  	20,			0xBB66FF,
			S_RIFLE,	S_ENERGY,	S_BZZT,		-1,
			1,		5,		750,	1500,	150,	25000,	0,		10000,
			0,		0,			1,		5,		2,		IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,
			false,	false,	false,		true,		true,		false,
			0.f,		 0.f,			0.05f,		2.0f,		0.f,	0.6f,		512.f,		500.f,
			"rifle",	"\fv",	"weapons/rifle/item",		"weapons/rifle/vwep",
			""
	},
	{
		WEAPON_GRENADE,		ANIM_GRENADE,		-5,		5,		0x22FF22,
			S_GLFIRE,	S_EXPLODE,	S_WHIRR,	S_TINK,
			1,		2,		1500,	6000,	300,	350,	3000,	3000,
			100,	64,			1,		0,		0,		BOUNCE_GEOM|BOUNCE_PLAYER|COLLIDE_OWNER,
			false,	false,	false,		false,		false,		false,
			0.35f,		0.f,			1.0f,		2.0f,		50.f,	4.f,		0.f,		1000.f,
			"grenade",	"\fg",	"weapons/grenade/item",	"weapons/grenade/vwep",
			"projectiles/grenade"
	},
	{
		WEAPON_PAINT,		ANIM_PAINTGUN,		-5,  	5,			0xFF22AA,
			S_PAINT,	S_SPLAT,	S_WHIZZ,	-1,
			10,		10,		500,	1000,	25,		1000,	0,		10000,
			0,		0,			1,		0,		0,		IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,
			false,	false,	false,		true,		true,		true,
			0.f,		 0.f,			0.05f,		2.0f,		0.f,	2.f,		0.f,		25.f,
			"paintgun",	"\fm",	"weapons/paintgun/item",		"weapons/paintgun/vwep",
			""
	},
};
#else
extern weaptypes weaptype[];
#endif

enum
{
	HIT_NONE 	= 0,
	HIT_LEGS	= 1<<1,
	HIT_TORSO	= 1<<2,
	HIT_HEAD	= 1<<3,
	HIT_FULL	= 1<<4,
	HIT_PROJ	= 1<<5,
	HIT_EXPLODE	= 1<<6,
	HIT_BURN	= 1<<7,
	HIT_MELT	= 1<<8,
	HIT_FALL	= 1<<9,
	HIT_WAVE	= 1<<10,
	HIT_SPAWN	= 1<<11,
	HIT_LOST	= 1<<12,
	HIT_KILL	= 1<<13
};

#define hithurts(x) (x&HIT_BURN || x&HIT_EXPLODE || x&HIT_PROJ || x&HIT_MELT || x&HIT_FALL)

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
	G_M_MULTI	= 1<<0,
	G_M_TEAM	= 1<<1,
	G_M_INSTA	= 1<<2,
	G_M_DUEL	= 1<<3,
	G_M_LMS		= 1<<4,
	G_M_PAINT	= 1<<5,
	G_M_VAMP	= 1<<6,
	G_M_ARENA	= 1<<7,
	G_M_DM		= G_M_INSTA|G_M_PAINT|G_M_VAMP|G_M_ARENA,
	G_M_TEAMS	= G_M_MULTI|G_M_TEAM|G_M_INSTA|G_M_PAINT|G_M_VAMP|G_M_ARENA,
	G_M_PBALL	= G_M_MULTI|G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_LMS|G_M_VAMP|G_M_PAINT,
	G_M_CLASS	= G_M_MULTI|G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_LMS|G_M_VAMP|G_M_ARENA,
	G_M_ALL		= G_M_MULTI|G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_LMS|G_M_PAINT|G_M_VAMP|G_M_ARENA,
};
#define G_M_NUM 8

struct gametypes
{
	int	type,			mutators,				implied;		const char *name;
};
#ifdef GAMESERVER
gametypes gametype[] = {
	{ G_DEMO,			G_M_NONE,				G_M_NONE,				"demo" },
	{ G_LOBBY,			G_M_NONE,				G_M_NONE,				"lobby" },
	{ G_EDITMODE,		G_M_NONE,				G_M_NONE,				"editing" },
	{ G_MISSION,		G_M_NONE,				G_M_NONE,				"mission" },
	{ G_DEATHMATCH,		G_M_ALL,				G_M_NONE,				"deathmatch" },
	{ G_STF,			G_M_TEAMS,				G_M_TEAM,				"secure-the-flag" },
	{ G_CTF,			G_M_TEAMS,				G_M_TEAM,				"capture-the-flag" },
}, mutstype[] = {
	{ G_M_MULTI,		G_M_ALL,				G_M_TEAM|G_M_MULTI,		"multi" },
	{ G_M_TEAM,			G_M_TEAMS,				G_M_TEAM,				"team" },
	{ G_M_INSTA,		G_M_ALL,				G_M_INSTA,				"insta" },
	{ G_M_DUEL,			G_M_DM|G_M_DUEL,		G_M_DUEL,				"duel" },
	{ G_M_LMS,			G_M_DM|G_M_LMS,			G_M_LMS,				"last-man" },
	{ G_M_PAINT,		G_M_PBALL,				G_M_PAINT,				"paintball" },
	{ G_M_VAMP,			G_M_ALL,				G_M_VAMP,				"vampire" },
	{ G_M_ARENA,		G_M_CLASS,				G_M_ARENA,				"arena" },
};
#else
extern gametypes gametype[], mutstype[];
#endif

#define m_game(a)			(a > -1 && a < G_MAX)

#define m_demo(a)			(a == G_DEMO)
#define m_lobby(a)			(a == G_LOBBY)
#define m_edit(a)			(a == G_EDITMODE)
#define m_mission(a)		(a == G_MISSION)
#define m_dm(a)				(a == G_DEATHMATCH)
#define m_stf(a)			(a == G_STF)
#define m_ctf(a)			(a == G_CTF)

#define m_play(a)			(a >= G_MISSION)
#define m_flag(a)			(m_stf(a) || m_ctf(a))
#define m_fight(a)			(a >= G_DEATHMATCH)

#define m_multi(a,b)		((b & G_M_MULTI) || (gametype[a].implied & G_M_MULTI))
#define m_team(a,b)			((b & G_M_TEAM) || (gametype[a].implied & G_M_TEAM))
#define m_insta(a,b)		((b & G_M_INSTA) || (gametype[a].implied & G_M_INSTA))
#define m_duel(a,b)			((b & G_M_DUEL) || (gametype[a].implied & G_M_DUEL))
#define m_lms(a,b)			((b & G_M_LMS) || (gametype[a].implied & G_M_LMS))
#define m_paint(a,b)		((b & G_M_PAINT) || (gametype[a].implied & G_M_PAINT))
#define m_vamp(a,b)			((b & G_M_VAMP) || (gametype[a].implied & G_M_VAMP))
#define m_arena(a,b)		((b & G_M_ARENA) || (gametype[a].implied & G_M_ARENA))

#define m_duke(a,b)			(m_duel(a, b) || m_lms(a, b))
#define m_regen(a,b)		(!m_duke(a,b) && !m_insta(a,b) && !m_paint(a,b))

#define m_spawnweapon(a,b)	(m_paint(a,b) ? WEAPON_PAINT : (m_arena(a,b) ? -1 : (m_insta(a,b) ? GVAR(instaspawnweapon) : GVAR(spawnweapon))))
#define m_spawndelay(a,b)	(!m_duke(a,b) ? (int((m_stf(a) ? GVAR(stfspawndelay) : (m_ctf(a) ? GVAR(ctfspawndelay) : GVAR(spawndelay)))*(m_insta(a, b) ? GVAR(instaspawnscale) : 1)*(m_paint(a, b) ? GVAR(paintspawnscale) : 1)*1000)) : 0)
#define m_noitems(a,b)		(m_paint(a,b) || m_arena(a,b) || (GVAR(itemsallowed) < (m_insta(a,b) ? 2 : 1)))
#define m_maxhealth(a,b)	(m_insta(a,b) ? 1 : GVAR(maxhealth))
#define m_speedscale(a)		(float(a)*GVAR(speedscale))
#define m_speedlerp(a)		(float(a)*(1.f/GVAR(speedscale)))
#define m_speedtimex(a)		(max(int(m_speedlerp(a)), 1))
#define m_speedtime(a)		(int(m_speedlerp(a)))

#define weaploads(a,b)		(b < 0 || a == b || weaptype[a].reloads)
#define weapcarry(a,b)		(b >= 0 && a != b && weaptype[a].reloads)
#define weapattr(a,b)		(a != b ? a : (b != WEAPON_GRENADE ? WEAPON_GRENADE : WEAPON_PISTOL))

enum { FLAGMODE_NONE = 0, FLAGMODE_STF, FLAGMODE_CTF, FLAGMODE_MAX };
#define chkflagmode(a,b)	(!b || (b == FLAGMODE_STF && m_stf(a)) || (b == FLAGMODE_CTF && m_ctf(a)))

// network messages codes, c2s, c2c, s2c
enum
{
	SV_CONNECT = 0, SV_SERVERINIT, SV_WELCOME, SV_CLIENTINIT, SV_POS, SV_PHYS, SV_TEXT, SV_COMMAND, SV_ANNOUNCE, SV_DISCONNECT,
	SV_SHOOT, SV_DESTROY, SV_SUICIDE, SV_DIED, SV_FRAG, SV_DAMAGE, SV_SHOTFX,
	SV_ARENAWEAP, SV_TRYSPAWN, SV_SPAWNSTATE, SV_SPAWN,
	SV_DROP, SV_WEAPSELECT, SV_TAUNT,
	SV_MAPCHANGE, SV_MAPVOTE, SV_ITEMSPAWN, SV_ITEMUSE, SV_TRIGGER, SV_EXECLINK,
	SV_PING, SV_PONG, SV_CLIENTPING,
	SV_TIMEUP, SV_NEWGAME, SV_ITEMACC,
	SV_SERVMSG, SV_GAMEINFO, SV_RESUME,
    SV_EDITMODE, SV_EDITENT, SV_EDITLINK, SV_EDITVAR, SV_EDITF, SV_EDITT, SV_EDITM, SV_FLIP, SV_COPY, SV_PASTE, SV_ROTATE, SV_REPLACE, SV_DELCUBE, SV_REMIP, SV_NEWMAP,
    SV_GETMAP, SV_SENDMAP, SV_SENDMAPFILE, SV_SENDMAPSHOT, SV_SENDMAPCONFIG,
	SV_MASTERMODE, SV_KICK, SV_CLEARBANS, SV_CURRENTMASTER, SV_SPECTATOR, SV_WAITING, SV_SETMASTER, SV_SETTEAM,
	SV_FLAGS, SV_FLAGINFO,
    SV_TAKEFLAG, SV_RETURNFLAG, SV_RESETFLAG, SV_DROPFLAG, SV_SCOREFLAG, SV_INITFLAGS, SV_TEAMSCORE,
	SV_LISTDEMOS, SV_SENDDEMOLIST, SV_GETDEMO, SV_SENDDEMO,
	SV_DEMOPLAYBACK, SV_RECORDDEMO, SV_STOPDEMO, SV_CLEARDEMOS,
	SV_CLIENT, SV_RELOAD, SV_REGEN,
	SV_ADDBOT, SV_DELBOT, SV_INITAI,
    SV_AUTHTRY, SV_AUTHCHAL, SV_AUTHANS
};

#ifdef GAMESERVER
char msgsizelookup(int msg)
{
	char msgsizesl[] =				// size inclusive message token, 0 for variable or not-checked sizes
	{
		SV_CONNECT, 0, SV_SERVERINIT, 5, SV_WELCOME, 1, SV_CLIENTINIT, 0, SV_POS, 0, SV_PHYS, 0, SV_TEXT, 0, SV_COMMAND, 0,
		SV_ANNOUNCE, 0, SV_DISCONNECT, 2,
		SV_SHOOT, 0, SV_DESTROY, 0, SV_SUICIDE, 3, SV_DIED, 6, SV_FRAG, 4, SV_DAMAGE, 10, SV_SHOTFX, 9,
		SV_ARENAWEAP, 0, SV_TRYSPAWN, 2, SV_SPAWNSTATE, 0, SV_SPAWN, 0,
		SV_DROP, 0, SV_WEAPSELECT, 0, SV_TAUNT, 2,
		SV_MAPCHANGE, 0, SV_MAPVOTE, 0, SV_ITEMSPAWN, 2, SV_ITEMUSE, 0, SV_TRIGGER, 0, SV_EXECLINK, 3,
		SV_PING, 2, SV_PONG, 2, SV_CLIENTPING, 2,
		SV_TIMEUP, 2, SV_NEWGAME, 1, SV_ITEMACC, 0,
		SV_SERVMSG, 0, SV_GAMEINFO, 0, SV_RESUME, 0,
        SV_EDITMODE, 2, SV_EDITENT, 11, SV_EDITLINK, 4, SV_EDITVAR, 0, SV_EDITF, 16, SV_EDITT, 16, SV_EDITM, 15, SV_FLIP, 14, SV_COPY, 14, SV_PASTE, 14, SV_ROTATE, 15, SV_REPLACE, 16, SV_DELCUBE, 14, SV_REMIP, 1, SV_NEWMAP, 2,
        SV_GETMAP, 0, SV_SENDMAP, 0, SV_SENDMAPFILE, 0, SV_SENDMAPSHOT, 0, SV_SENDMAPCONFIG, 0,
		SV_MASTERMODE, 2, SV_KICK, 2, SV_CLEARBANS, 1, SV_CURRENTMASTER, 3, SV_SPECTATOR, 3, SV_WAITING, 2, SV_SETMASTER, 0, SV_SETTEAM, 0,
		SV_FLAGS, 0, SV_FLAGINFO, 0,
        SV_DROPFLAG, 0, SV_SCOREFLAG, 5, SV_RETURNFLAG, 3, SV_TAKEFLAG, 3, SV_RESETFLAG, 2, SV_INITFLAGS, 0, SV_TEAMSCORE, 0,
		SV_LISTDEMOS, 1, SV_SENDDEMOLIST, 0, SV_GETDEMO, 2, SV_SENDDEMO, 0,
		SV_DEMOPLAYBACK, 3, SV_RECORDDEMO, 2, SV_STOPDEMO, 1, SV_CLEARDEMOS, 2,
		SV_CLIENT, 0, SV_RELOAD, 0, SV_REGEN, 0,
		SV_ADDBOT, 0, SV_DELBOT, 0, SV_INITAI, 0,
        SV_AUTHTRY, 0, SV_AUTHCHAL, 0, SV_AUTHANS, 0,
		-1
	};
	for(char *p = msgsizesl; *p>=0; p += 2) if(*p==msg) return p[1];
	return -1;
}
#else
extern char msgsizelookup(int msg);
#endif
enum { SPHY_NONE = 0, SPHY_JUMP, SPHY_IMPULSE, SPHY_POWER, SPHY_MAX };
enum { CON_CHAT = CON_GAMESPECIFIC, CON_MAX };

#define DEMO_MAGIC "BFDZ"
struct demoheader
{
	char magic[16];
	int version, gamever;
};

enum
{
	TEAM_NEUTRAL = 0, TEAM_ALPHA, TEAM_BETA, TEAM_DELTA, TEAM_GAMMA, TEAM_ENEMY, TEAM_MAX,
	TEAM_FIRST = TEAM_ALPHA, TEAM_LAST = TEAM_GAMMA, TEAM_NUM = TEAM_LAST-(TEAM_FIRST-1)
};
struct teamtypes
{
	int	type,			colour;	const char	*name,
		*tpmdl,								*fpmdl,
		*flag,			*icon,				*chat;
};
#ifdef GAMESERVER
teamtypes teamtype[] = {
	{
		TEAM_NEUTRAL,	0x666666,			"neutral",
		"actors/player",					"actors/player/vwep",
		"flag",			"team",				"\fd"
	},
	{
		TEAM_ALPHA,		0x2222AA,			"alpha",
		"actors/player/alpha",				"actors/player/alpha/vwep",
		"flag/alpha",	"teamalpha",		"\fb"
	},
	{
		TEAM_BETA,		0xAA2222,			"beta",
		"actors/player/beta",				"actors/player/beta/vwep",
		"flag/beta",	"teambeta",			"\fr"
	},
	{
		TEAM_DELTA,		0xAAAA22,			"delta",
		"actors/player/delta",				"actors/player/delta/vwep",
		"flag/delta",	"teamdelta",		"\fy"
	},
	{
		TEAM_GAMMA,		0x22AA22,			"gamma",
		"actors/player/gamma",				"actors/player/gamma/vwep",
		"flag/gamma",	"teamgamma",		"\fg"
	},
	{
		TEAM_ENEMY,		0x22AAAA,			"enemy",
		"actors/player",					"actors/player/vwep",
		"flag",			"team",				"\fc"
	}
};
#else
extern teamtypes teamtype[];
#endif
enum
{
	BASE_NONE = 0,
	BASE_HOME = 1<<0,
	BASE_FLAG = 1<<1,
	BASE_BOTH = BASE_HOME|BASE_FLAG
};

#define numteams(a,b)	(m_multi(a,b) ? TEAM_NUM : TEAM_NUM/2)
#define isteam(a,b,c,d)	(m_team(a,b) ? (c >= d && c <= numteams(a,b)+(TEAM_FIRST-1)) : c == TEAM_NEUTRAL)
#define valteam(a,b)	(a >= b && a <= TEAM_NUM)

const int paintcolours[10] = {
    0xA0A0A0, 0x2222FF, 0xFF2222, 0xFFFF22, 0x22FF22,
    0xFF8822, 0xFF22FF, 0x22FFFF, 0xFF2288, 0x222222
};

#define MAXNAMELEN		16

enum
{
	SAY_NONE	= 0,
	SAY_ACTION	= 1<<0,
	SAY_TEAM	= 1<<1,
};
#define SAY_NUM 2

enum
{
	PRIV_NONE = 0,
	PRIV_MASTER,
	PRIV_ADMIN,
	PRIV_MAX
};

#define MM_MODE 0xF
#define MM_AUTOAPPROVE 0x1000
#define MM_PRIVSERV (MM_MODE | MM_AUTOAPPROVE)
#define MM_PUBSERV ((1<<MM_OPEN) | (1<<MM_VETO))
#define MM_COOPSERV (MM_AUTOAPPROVE | MM_PUBSERV | (1<<MM_LOCKED))

enum { MM_OPEN = 0, MM_VETO, MM_LOCKED, MM_PRIVATE, MM_PASSWORD };

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

#include "ai.h"

// inherited by gameent and server clients
struct gamestate
{
	int health, ammo[WEAPON_MAX], entid[WEAPON_MAX];
	int lastweap, arenaweap, weapselect, weapload[WEAPON_MAX], weapstate[WEAPON_MAX], weapwait[WEAPON_MAX], weaplast[WEAPON_MAX];
	int lastdeath, lastspawn, lastrespawn, lastpain, lastregen;
	int aitype, ownernum, skill, spree;

	gamestate() : arenaweap(-1), lastdeath(0), lastspawn(0), lastrespawn(0), lastpain(0), lastregen(0),
		aitype(AI_NONE), ownernum(-1), skill(0), spree(0) {}
	~gamestate() {}

	int hasweap(int weap, int sweap, int level = 0, int exclude = -1)
	{
		if(isweap(weap) && weap != exclude)
		{
			if(ammo[weap] > 0 || (weaploads(weap, sweap) && !ammo[weap])) switch(level)
			{
				case 0: default: return true; break; // has weap at all
				case 1: if(weapcarry(weap, sweap)) return true; break; // only carriable
				case 2: if(ammo[weap] > 0) return true; break; // only with actual ammo
				case 3: if(ammo[weap] > 0 && weaploads(weap, sweap)) return true; break; // only reloadable with actual ammo
				case 4: if(ammo[weap] >= (weaploads(weap, sweap) ? 0 : weaptype[weap].max)) return true; break; // only reloadable or those with < max
			}
		}
		return false;
	}

	int bestweap(int sweap, bool last = false)
	{
		if(last && hasweap(lastweap, sweap)) return lastweap;
		loopirev(WEAPON_MAX) if(hasweap(i, sweap, 3)) return i; // reloadable first
		loopirev(WEAPON_MAX) if(hasweap(i, sweap, 1)) return i; // carriable second
		loopirev(WEAPON_MAX) if(hasweap(i, sweap, 0)) return i; // any just to bail us out
		return weapselect;
	}

	int carry(int sweap)
	{
		int carry = 0;
		loopi(WEAPON_MAX) if(hasweap(i, sweap, 1)) carry++;
		return carry;
	}

	int drop(int sweap, int exclude = -1)
	{
		int weap = -1;
		if(hasweap(weapselect, sweap, 1, exclude)) weap = weapselect;
		else loopi(WEAPON_MAX) if(hasweap(i, sweap, 1, exclude))
		{
			weap = i;
			break;
		}
		return weap;
	}

	void weapreset(bool full = false)
	{
		loopi(WEAPON_MAX)
		{
			weapstate[i] = WPSTATE_IDLE;
			weapwait[i] = weaplast[i] = weapload[i] = 0;
			if(full) ammo[i] = -1;
			entid[i] = -1;
		}
		if(full) lastweap = weapselect = -1;
	}

	void setweapstate(int weap, int state, int delay, int millis)
	{
		weapstate[weap] = state;
		weapwait[weap] = delay;
		weaplast[weap] = millis;
	}

	void weapswitch(int weap, int millis, int state = WPSTATE_SWITCH)
	{
		lastweap = weapselect;
		setweapstate(lastweap, WPSTATE_SWITCH, WEAPSWITCHDELAY, millis);
		weapselect = weap;
		setweapstate(weap, state, WEAPSWITCHDELAY, millis);
	}

	bool weapwaited(int weap, int millis, int skip = -1)
	{
		if(skip >= 0 && weapstate[weap] == skip) return true;
		if(!weapwait[weap] || weapstate[weap] == WPSTATE_IDLE || weapstate[weap] == WPSTATE_POWER) return true;
		return millis-weaplast[weap] >= weapwait[weap];
	}

	int skipwait(int weap, int skip)
	{
		return skip != WPSTATE_RELOAD || ammo[weap] > weaptype[weap].add || (weapload[weap] >= 0 && weapload[weap] < weaptype[weap].add) ? skip : -1;
	}

	bool canswitch(int weap, int sweap, int millis, int skip = -1)
	{
		if(weap != weapselect && weapwaited(weapselect, millis, skipwait(weapselect, skip)) && hasweap(weap, sweap) && weapwaited(weap, millis, skipwait(weap, skip)))
			return true;
		return false;
	}

	bool canshoot(int weap, int sweap, int millis, int skip = -1)
	{
		if(hasweap(weap, sweap) && ammo[weap] > 0 && weapwaited(weap, millis, skipwait(weap, skip)))
			return true;
		return false;
	}

	bool canreload(int weap, int sweap, int millis)
	{
		if(weapwaited(weapselect, millis) && weaploads(weap, sweap) && hasweap(weap, sweap) && ammo[weap] < weaptype[weap].max && weapwaited(weap, millis))
			return true;
		return false;
	}

	bool canuse(int type, int attr1, int attr2, int attr3, int attr4, int attr5, int sweap, int millis, int skip = -1)
	{
		if((type != TRIGGER || attr3 == TA_AUTO) && enttype[type].usetype == EU_AUTO)
			return true;
		if(weapwaited(weapselect, millis, skipwait(weapselect, skip))) switch(type)
		{
			case TRIGGER:
			{
				return true;
				break;
			}
			case WEAPON:
			{ // can't use when reloading or firing
				if(isweap(attr1) && !hasweap(attr1, sweap, 4) && weapwaited(attr1, millis))
					return true;
				break;
			}
			default: break;
		}
		return false;
	}

	void useitem(int id, int type, int attr1, int attr2, int attr3, int attr4, int attr5, int sweap, int millis)
	{
		switch(type)
		{
			case TRIGGER: break;
			case WEAPON:
			{
				weapswitch(attr1, millis, hasweap(attr1, sweap) ? (weapselect != attr1 ? WPSTATE_SWITCH : WPSTATE_RELOAD) : WPSTATE_PICKUP);
				ammo[attr1] = clamp((ammo[attr1] > 0 ? ammo[attr1] : 0)+weaptype[attr1].add, 1, weaptype[attr1].max);
				entid[attr1] = id;
				break;
			}
			default: break;
		}
	}

	void clearstate()
	{
		lastdeath = lastpain = lastregen = spree = 0;
		lastrespawn = -1;
	}

	void respawn(int millis, int heal)
	{
		health = heal;
		lastspawn = millis;
		clearstate();
		weapreset(true);
	}

	void spawnstate(int sweap, int heal, bool arena = false)
	{
		health = heal;
		weapreset(true);
		if(!isweap(sweap)) sweap = WEAPON_PISTOL;
		ammo[sweap] = weaptype[sweap].reloads ? weaptype[sweap].add : weaptype[sweap].max;
		if(arena)
		{
			int aweap = arenaweap;
			if(aweap <= WEAPON_PISTOL || aweap >= WEAPON_TOTAL) aweap = rnd(WEAPON_TOTAL-1)+1; // pistol = random
			ammo[aweap] = weaptype[aweap].reloads ? weaptype[aweap].add : weaptype[aweap].max;
			lastweap = weapselect = aweap;
		}
		else
		{
			arenaweap = -1;
			lastweap = weapselect = sweap;
		}
	}

	void editspawn(int millis, int sweap, int heal)
	{
		clearstate();
		spawnstate(sweap, heal);
	}

    int respawnwait(int millis, int delay)
    {
        return lastdeath ? max(0, delay-(millis-lastdeath)) : 0;
    }

	// just subtract damage here, can set death, etc. later in code calling this
	void dodamage(int millis, int heal)
	{
		lastregen = 0;
		lastpain = millis;
		health = heal;
	}

	int protect(int millis, int delay)
	{
		int amt = 0;
		if(lastspawn && delay && millis-lastspawn <= delay) amt = delay-(millis-lastspawn);
		return amt;
	}
};

#if !defined(GAMESERVER) && !defined(STANDALONE)
struct gameentity : extentity
{
	int schan;
	int lastuse, lastspawn;
	int mark;
	vector<int> kin;

	gameentity() : schan(-1), lastuse(0), lastspawn(0), mark(0) { kin.setsize(0); }
	~gameentity()
	{
		if(issound(schan)) removesound(schan);
		schan = -1;
		kin.setsize(0);
	}
};

enum
{
	ST_NONE		= 0,
	ST_CAMERA	= 1<<0,
	ST_CURSOR	= 1<<1,
	ST_GAME		= 1<<2,
	ST_SPAWNS	= 1<<3,
	ST_DEFAULT	= ST_CAMERA|ST_CURSOR|ST_GAME,
	ST_VIEW		= ST_CURSOR|ST_CAMERA,
	ST_ALL		= ST_CAMERA|ST_CURSOR|ST_GAME|ST_SPAWNS,
};

enum { ITEM_ENT = 0, ITEM_PROJ, ITEM_MAX };
struct actitem
{
	int type, target;
	float score;

	actitem() : type(ITEM_ENT), target(-1), score(0.f) {}
	~actitem() {}
};
#ifdef GAMEWORLD
const char *animnames[] =
{
	"idle", "forward", "backward", "left", "right", "dead", "dying", "swim",
	"mapmodel", "trigger on", "trigger off",
	"pain", "jump", "impulse", "sink",
	"edit", "lag", "switch", "taunt", "win", "lose",
	"crouch", "crawl forward", "crawl backward", "crawl left", "crawl right",
	"pistol", "pistol shoot", "pistol reload",
	"shotgun", "shotgun shoot", "shotgun reload",
	"smg", "smg shoot", "smg reload",
	"grenade", "grenade throw", "grenade reload", "grenade power",
	"flamer", "flamer shoot", "flamer reload",
	"plasma", "plasma shoot", "plasma reload",
	"rifle", "rifle shoot", "rifle reload",
	"paintgun", "paintgun shoot", "paintgun reload",
	"vwep", "shield", "powerup",
	""
};
#else
extern const char *animnames[];
#endif
struct serverstatuses
{
	int type,				colour;		const char *icon;
};
#ifdef GAMEWORLD
serverstatuses serverstatus[] = {
	{ SSTAT_OPEN,			0xDDDDDD,	"server" },
	{ SSTAT_LOCKED,			0xDD8800,	"serverlock" },
	{ SSTAT_PRIVATE,		0x8888DD,	"serverpriv" },
	{ SSTAT_FULL,			0xDD8888,	"serverfull" },
	{ SSTAT_UNKNOWN,		0x888888,	"serverunk" }
};
const char *serverinfotypes[] = {
	"",	"desc", "ping", "pl", "max", "game", "map", "time"
};
#else
extern serverstatuses serverstatus[];
extern const char *serverinfotypes[];
#endif

struct gameent : dynent, gamestate
{
	int team, clientnum, privilege, lastupdate, lastpredict, plag, ping,
		attacktime, reloadtime, usetime, lasttaunt, lastflag, frags, deaths, totaldamage,
			totalshots, smoothmillis, lastnode, respawned, suicided, vschan, dschan, wschan,
				lasthit, lastkill;
	editinfo *edit;
    float deltayaw, deltapitch, newyaw, newpitch;
    float deltaaimyaw, deltaaimpitch, newaimyaw, newaimpitch;
	ai::aiinfo *ai;
    vec muzzle, affinity;
	bool attacking, reloading, useaction, conopen, k_up, k_down, k_left, k_right;
	string name, info, obit;
	vector<int> airnodes;

	gameent() : team(TEAM_NEUTRAL), clientnum(-1), privilege(PRIV_NONE), lastupdate(0), lastpredict(0), plag(0), ping(0),
		frags(0), deaths(0), totaldamage(0), totalshots(0), smoothmillis(-1), vschan(-1), dschan(-1), wschan(-1), edit(NULL), ai(NULL),
		muzzle(-1, -1, -1), affinity(-1, -1, -1), conopen(false), k_up(false), k_down(false), k_left(false), k_right(false)
	{
		name[0] = info[0] = obit[0] = 0;
		weight = 250; // so we can control the 'gravity' feel
		maxspeed = 50; // ditto for movement
		respawn(-1, 100);
	}
	~gameent()
	{
		freeeditinfo(edit);
		if(ai) delete ai;
		removetrackedparticles(this);
		removetrackedsounds(this);
		if(issound(vschan)) removesound(vschan);
		if(issound(dschan)) removesound(dschan);
		if(issound(wschan)) removesound(wschan);
		vschan = dschan = wschan = -1;
	}

	void stopmoving(bool full)
	{
		if(full) move = strafe = 0;
		attacking = reloading = useaction = false;
		attacktime = reloadtime = usetime = 0;
	}

	void clearstate()
	{
        lasttaunt = lasthit = lastkill = 0;
		lastflag = respawned = suicided = lastnode = -1;
		obit[0] = 0;
	}

	void respawn(int millis, int heal)
	{
		stopmoving(true);
		clearstate();
		physent::reset();
		gamestate::respawn(millis, heal);
		airnodes.setsizenodelete(0);
	}

	void editspawn(int millis, int sweap, int heal)
	{
		stopmoving(true);
		clearstate();
    	inmaterial = timeinair = jumptime = crouchtime = lastimpulse = 0;
    	inliquid = onladder = jumping = crouching = false;
        strafe = move = 0;
        physstate = PHYS_FALL;
		vel = falling = vec(0, 0, 0);
        floor = vec(0, 0, 1);
        resetinterp();
		gamestate::editspawn(millis, sweap, heal);
		airnodes.setsizenodelete(0);
	}

	void resetstate(int millis, int heal)
	{
		respawn(millis, heal);
		frags = deaths = totaldamage = totalshots = 0;
	}
};

enum { PRJ_SHOT = 0, PRJ_GIBS, PRJ_DEBRIS, PRJ_ENT };

struct projent : dynent
{
	vec from, to, norm;
	int addtime, lifetime, lifemillis, waittime, spawntime, lastradial, lasteffect, lastbounce;
	float movement, roll, lifespan, lifesize;
	bool local, beenused, radial, extinguish, canrender, limited;
	int projtype, projcollide;
	float elasticity, reflectivity, relativity, waterfric;
	int schan, id, weap, colour;
	entitylight light;
	gameent *owner;
	physent *hit;
	const char *mdl;

	projent() : norm(0, 0, 1), projtype(PRJ_SHOT), id(-1), owner(NULL), hit(NULL), mdl(NULL) { reset(); }
	~projent()
	{
		removetrackedparticles(this);
		removetrackedsounds(this);
		if(issound(schan)) removesound(schan);
		schan = -1;
	}

	void reset()
	{
		physent::reset();
		type = ENT_PROJ;
		state = CS_ALIVE;
		norm = vec(0, 0, 1);
		addtime = lifetime = lifemillis = waittime = spawntime = lastradial = lasteffect = lastbounce = 0;
		schan = id = -1;
		movement = roll = lifespan = lifesize = 0.f;
		colour = 0xFFFFFF;
		beenused = radial = extinguish = limited = false;
		canrender = true;
		projcollide = BOUNCE_GEOM|BOUNCE_PLAYER;
	}

	bool ready()
	{
		if(owner && !beenused && waittime <= 0 && state != CS_DEAD)
			return true;
		return false;
	}
};


namespace server
{
	extern void stopdemo();
    extern void hashpassword(int cn, int sessionid, const char *pwd, char *result, int maxlen = MAXSTRLEN);
}

namespace client
{
	extern bool demoplayback, sendinfo;
	extern void addmsg(int type, const char *fmt = NULL, ...);
    extern void c2sinfo();
}

namespace physics
{
	extern int smoothmove, smoothdist;
	extern bool canimpulse(physent *d);
    extern bool movecamera(physent *pl, const vec &dir, float dist, float stepdist);
	extern void smoothplayer(gameent *d, int res, bool local);
	extern void update();
}

namespace projs
{
	extern vector<projent *> projs;

	extern void reset();
	extern void update();
	extern void create(vec &from, vec &to, bool local, gameent *d, int type, int lifetime, int lifemillis, int waittime, int speed, int id = 0, int weap = -1);
	extern void preload();
	extern void remove(gameent *owner);
	extern void shootv(int weap, int power, vec &from, vector<vec> &locs, gameent *d, bool local);
	extern void drop(gameent *d, int g, int n, bool local = true);
	extern void adddynlights();
	extern void render();
}

namespace weapons
{
	extern int autoreloading;
	extern bool weapselect(gameent *d, int weap, bool local = true);
	extern bool weapreload(gameent *d, int weap, int load = -1, int ammo = -1, bool local = true);
	extern void reload(gameent *d);
	extern void shoot(gameent *d, vec &targ, int force = 0);
	extern void preload(int weap = -1);
}

namespace hud
{
	extern int hudwidth, hudsize, damageresidue, radarflagnames;
	extern vector<int> teamkills;
	extern bool hastv(int val);
	extern void drawquad(float x, float y, float w, float h, float tx1 = 0, float ty1 = 0, float tx2 = 1, float ty2 = 1);
	extern void drawtex(float x, float y, float w, float h, float tx = 0, float ty = 0, float tw = 1, float th = 1);
	extern void drawsized(float x, float y, float s);
	extern void colourskew(float &r, float &g, float &b, float skew = 1.f);
	extern void healthskew(int &s, float &r, float &g, float &b, float &fade, float ss, bool throb = 1.f);
	extern void skewcolour(float &r, float &g, float &b, bool t = false);
	extern void skewcolour(int &r, int &g, int &b, bool t = false);
	extern void drawindicator(int weap, int x, int y, int s);
	extern void drawclip(int weap, int x, int y, float s);
	extern void drawpointerindex(int index, int x, int y, int s, float r = 1.f, float g = 1.f, float b = 1.f, float fade = 1.f);
	extern void drawpointer(int w, int h, int index);
	extern int numteamkills();
	extern float radarrange();
	extern void drawblip(int w, int h, int s, float blend, int idx, vec &dir, float r = 1.f, float g = 1.f, float b = 1.f, const char *font = "sub", const char *text = NULL, ...);
	extern int drawitem(const char *tex, int x, int y, float size, bool left = false, float r = 1.f, float g = 1.f, float b = 1.f, float fade = 1.f, float skew = 1.f, const char *font = NULL, const char *text = NULL, ...);
	extern void drawitemsubtext(int x, int y, float size, bool left = false, float skew = 1.f, const char *font = NULL, float blend = 1.f, const char *text = NULL, ...);
	extern int drawweapons(int x, int y, int s, float blend = 1.f);
	extern int drawhealth(int x, int y, int s, float blend = 1.f);
	extern void drawinventory(int w, int h, int edge, float blend = 1.f);
	extern void damagecompass(int n, const vec &loc, gameent *actor, int weap);
	extern const char *flagtex(int team = TEAM_NEUTRAL);
	extern const char *itemtex(int type, int stype);
}

namespace game
{
	extern int numplayers, gamemode, mutators, nextmode, nextmuts, minremain, maptime,
			quakewobble, lastzoom, lastspec, lastspecchg, spectvtime, showplayerinfo,
				noblood, fogdist, announcefilter;
	extern bool intermission, zooming;

	extern gameent *player1;
	extern vector<gameent *> players;

	extern gameent *newclient(int cn);
	extern gameent *getclient(int cn);
	extern gameent *intersectclosest(vec &from, vec &to, gameent *at);
	extern void clientdisconnected(int cn);
	extern char *colorname(gameent *d, char *name = NULL, const char *prefix = "", bool team = true, bool dupname = true);
	extern void announce(int idx, int targ, const char *msg, ...);
	extern void respawn(gameent *d);
	extern void spawneffect(const vec &o, int colour = 0xFFFFFF, int radius = 4, int fade = 250, float size = 2.f);
	extern void suicide(gameent *d, int flags);
	extern void fixrange(float &yaw, float &pitch);
	extern void fixfullrange(float &yaw, float &pitch, float &roll, bool full);
	extern void getyawpitch(const vec &from, const vec &pos, float &yaw, float &pitch);
	extern void scaleyawpitch(float &yaw, float &pitch, float targyaw, float targpitch, float frame = 1.f, float scale = 1.f);
	extern bool allowmove(physent *d);
	extern int mousestyle();
	extern int deadzone();
	extern int panspeed();
	extern bool inzoom();
	extern bool inzoomswitch();
	extern int zoominterval();
	extern bool tvmode();
	extern void resetcamera();
	extern void resetworld();
	extern void resetstate();
	extern void damaged(int weap, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir);
	extern void killed(int weap, int flags, int damage, gameent *d, gameent *actor);
	extern void timeupdate(int timeremain);
}

namespace entities
{
	extern int showentdescs;
	extern vector<extentity *> ents;
	extern void clearentcache();
	extern int closestent(int type, const vec &pos, float mindist, bool links = false);
	extern bool collateitems(gameent *d, vector<actitem> &actitems);
	extern void checkitems(gameent *d);
	extern void putitems(ucharbuf &p);
	extern void execlink(gameent *d, int index, bool local, int ignore = -1);
	extern void setspawn(int n, bool on);
	extern bool tryspawn(dynent *d, const vec &o, short yaw = 0, short pitch = 0);
	extern void spawnplayer(gameent *d, int ent = -1, bool recover = false, bool suicide = false);
	extern const char *entinfo(int type, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0, bool full = false);
	extern void useeffects(gameent *d, int n, bool s, int g, int r);
	extern const char *entmdlname(int type, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0);
	extern bool clipped(const vec &o, bool aiclip = false);
	extern void preload();
	extern void edittoggled(bool edit);
	extern const char *findname(int type);
	extern void adddynlights();
	extern void render();
	extern void update();
	struct avoidset
	{
		struct obstacle
		{
			dynent *ent;
			int numentities;
            float above;

			obstacle(dynent *ent) : ent(ent), numentities(0), above(-1) {}
		};

		vector<obstacle> obstacles;
		vector<int> entities;

		void clear()
		{
			obstacles.setsizenodelete(0);
			entities.setsizenodelete(0);
		}

		void add(dynent *ent)
		{
			obstacle &ob = obstacles.add(obstacle(ent));
			switch(ent->type)
			{
				case ENT_PLAYER:
				{
					gameent *e = (gameent *)ent;
					ob.above = e->abovehead().z;
					break;
				}
				case ENT_PROJ:
				{
					projent *p = (projent *)ob.ent;
					if(p->projtype == PRJ_SHOT && weaptype[p->weap].explode)
						ob.above = p->o.z+(weaptype[p->weap].explode*p->lifesize)+1.f;
					break;
				}
			}
		}

		void add(dynent *ent, int entity)
		{
			if(obstacles.empty() || ent!=obstacles.last().ent) add(ent);
			obstacles.last().numentities++;
			entities.add(entity);
		}

        void avoidnear(dynent *d, const vec &pos, float limit);

		#define loopavoid(v, d, body) \
			if(!(v).obstacles.empty()) \
			{ \
				int cur = 0; \
				loopv((v).obstacles) \
				{ \
					const entities::avoidset::obstacle &ob = (v).obstacles[i]; \
					int next = cur + ob.numentities; \
					if(ob.ent && ob.ent != (d)) \
					{ \
						for(; cur < next; cur++) \
						{ \
							int ent = (v).entities[cur]; \
							body; \
						} \
					} \
					cur = next; \
				} \
			}

		bool find(int entity, gameent *d) const
		{
			loopavoid(*this, d, { if(ent == entity) return true; });
			return false;
		}

		int remap(gameent *d, int n, vec &pos)
		{
			if(!obstacles.empty())
			{
				int cur = 0;
				loopv(obstacles)
				{
					obstacle &ob = obstacles[i];
					int next = cur + ob.numentities;
					if(ob.ent && ob.ent != d)
					{
						for(; cur < next; cur++) if(entities[cur] == n)
						{
							if(ob.above < 0) return -1;
							vec above(pos.x, pos.y, ob.above);
							if(above.z-d->o.z >= ai::JUMPMAX)
								return -1; // too much scotty
							int node = closestent(WAYPOINT, above, ai::NEARDIST, true);
							if(ents.inrange(node) && node != n)
							{ // try to reroute above their head?
								if(!find(node, d))
								{
									pos = ents[node]->o;
									return node;
								}
								else return -1;
							}
							else
							{
								vec old = d->o;
								d->o = vec(above).add(vec(0, 0, d->height));
								bool col = collide(d, vec(0, 0, 1));
								d->o = old;
								if(col)
								{
									pos = above;
									return n;
								}
								else return -1;
							}
						}
					}
					cur = next;
				}
			}
			return n;
		}
	};
    extern void findentswithin(int type, const vec &pos, float mindist, float maxdist, vector<int> &results);
	extern bool route(gameent *d, int node, int goal, vector<int> &route, const avoidset &obstacles, bool check = true);
}
#elif defined(GAMESERVER)
namespace client
{
    extern const char *getname();
}
#endif
#include "ctf.h"
#include "stf.h"
#include "vars.h"
#ifndef GAMESERVER
#include "scoreboard.h"
#endif

#endif

