
// network quantization scale
#define DMF 16.0f			// for world locations
#define DNF 100.0f			// for normalized vectors
#define DVELF 1.0f			// for playerspeed based velocity vectors

enum						// static entity types
{
	NOTUSED = ET_EMPTY,		// entity slot not in use in map
	LIGHT = ET_LIGHT,		// lightsource, attr1 = radius, attr2 = intensity
	MAPMODEL = ET_MAPMODEL,	// attr1 = angle, attr2 = idx
	PLAYERSTART,			// attr1 = angle
	ENVMAP = ET_ENVMAP,		// attr1 = radius
	PARTICLES = ET_PARTICLES,
	MAPSOUND = ET_SOUND,
	SPOTLIGHT = ET_SPOTLIGHT,
#ifdef BFRONTIER
	WEAPON,
#else
    I_SHELLS, I_BULLETS, I_ROCKETS, I_ROUNDS, I_GRENADES, I_CARTRIDGES,
    I_HEALTH, I_BOOST,
    I_GREENARMOUR, I_YELLOWARMOUR,
    I_QUAD,
#endif
	TELEPORT,				// attr1 = idx
	TELEDEST,				// attr1 = angle, attr2 = idx
	MONSTER,				// attr1 = angle, attr2 = monstertype
	CARROT,					// attr1 = tag, attr2 = type
	JUMPPAD,				// attr1 = zpush, attr2 = ypush, attr3 = xpush
	BASE,
	RESPAWNPOINT,
#ifdef BFRONTIER
	CAMERA,					// attr1 = yaw, attr2 = pitch, attr3 = pan (+:horiz/-:vert), attr4 = idx
	WAYPOINT,				// none?
#endif
	MAXENTTYPES
};

struct fpsentity : extentity
{
#ifdef BFRONTIER
	vector<int> links;  // link list
	
	fpsentity()
	{
		links.setsize(0);
	}
	~fpsentity() {}
#else
	// extend with additional fields if needed...
#endif
};

#ifdef BFRONTIER
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
enum { M_NONE = 0, M_SEARCH, M_HOME, M_ATTACKING, M_PAIN, M_SLEEP, M_AIMING };  // monster states

#define m_noitems	 ((gamemode>=4 && gamemode<=11) || gamemode==13)
#define m_noitemsrail ((gamemode>=4 && gamemode<=5) || (gamemode>=8 && gamemode<=9) || gamemode==13)
#define m_arena		(gamemode>=8 && gamemode<=11)
#define m_tarena	  (gamemode>=10 && gamemode<=11)
#define m_capture	 (gamemode>=12 && gamemode<=13)
#define m_teammode	((gamemode>2 && gamemode&1) || m_capture)
#define m_sp		  (gamemode>=-2 && gamemode<0)
#define m_dmsp		(gamemode==-1)
#define m_classicsp	(gamemode==-2)
#define m_demo		(gamemode==-3)
#define isteam(a,b)	(m_teammode && strcmp(a, b)==0)

#define m_mp(mode)	(mode>=0 && mode<=13)
/*
#define m_mp(mode)		(mode >= 0 && mode <= 3)
#define m_dm			(gamemode == 1)
#define m_dom			(gamemode == 2)

#define m_sp			(gamemode>=-2 && gamemode<0)
#define m_dmsp			(gamemode==-1)
#define m_classicsp		(gamemode==-2)
#define m_demo			(gamemode==-3)
#define isteam(a,b)		(m_teammode && strcmp(a, b)==0)
*/
#else
enum { GUN_FIST = 0, GUN_SG, GUN_CG, GUN_RL, GUN_RIFLE, GUN_GL, GUN_PISTOL, GUN_FIREBALL, GUN_ICEBALL, GUN_SLIMEBALL, GUN_BITE, NUMGUNS };
enum { A_BLUE, A_GREEN, A_YELLOW };	 // armour types... take 20/40/60 % off
enum { M_NONE = 0, M_SEARCH, M_HOME, M_ATTACKING, M_PAIN, M_SLEEP, M_AIMING };  // monster states

#define m_noitems	 ((gamemode>=4 && gamemode<=11) || gamemode==13)
#define m_noitemsrail ((gamemode>=4 && gamemode<=5) || (gamemode>=8 && gamemode<=9) || gamemode==13)
#define m_arena		(gamemode>=8 && gamemode<=11)
#define m_tarena	  (gamemode>=10 && gamemode<=11)
#define m_capture	 (gamemode>=12 && gamemode<=13)
#define m_teammode	((gamemode>2 && gamemode&1) || m_capture)
#define m_sp		  (gamemode>=-2 && gamemode<0)
#define m_dmsp		(gamemode==-1)
#define m_classicsp	(gamemode==-2)
#define m_demo		(gamemode==-3)
#define isteam(a,b)	(m_teammode && strcmp(a, b)==0)

#define m_mp(mode)	(mode>=0 && mode<=13)
#endif

// hardcoded sounds, defined in sounds.cfg
enum
{
#ifdef BFRONTIER
	S_JUMP = 0, S_LAND, S_RIFLE, S_SG, S_CG,
	S_RLFIRE, S_RLHIT, S_WEAPLOAD, S_ITEMAMMO,
	S_ITEMSPAWN, S_TELEPORT, S_NOAMMO,
#else
	S_JUMP = 0, S_LAND, S_RIFLE, S_PUNCH1, S_SG, S_CG,
	S_RLFIRE, S_RLHIT, S_WEAPLOAD, S_ITEMAMMO, S_ITEMHEALTH,
	S_ITEMARMOUR, S_ITEMPUP, S_ITEMSPAWN, S_TELEPORT, S_NOAMMO, S_PUPOUT,
#endif
	S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6,
	S_DIE1, S_DIE2,
	S_FLAUNCH, S_FEXPLODE,
	S_SPLASH1, S_SPLASH2,
#ifdef BFRONTIER
	S_RUMBLE,
#else
	S_GRUNT1, S_GRUNT2, S_RUMBLE,
	S_PAINO,
	S_PAINR, S_DEATHR,
	S_PAINE, S_DEATHE,
	S_PAINS, S_DEATHS,
	S_PAINB, S_DEATHB,
	S_PAINP, S_PIGGR2,
	S_PAINH, S_DEATHH,
	S_PAIND, S_DEATHD,
	S_PIGR1, S_ICEBALL, S_SLIMEBALL,
#endif
	S_JUMPPAD, S_PISTOL,
	
	S_V_BASECAP, S_V_BASELOST,
	S_V_FIGHT,
#ifndef BFRONTIER
	S_V_BOOST, S_V_BOOST10,
	S_V_QUAD, S_V_QUAD10,
#endif
	S_V_RESPAWNPOINT, 
#ifdef BFRONTIER
	S_V_ONEMINUTE, S_V_YOUWIN, S_V_YOULOSE, S_V_FRAGGED, S_V_OWNED,
	S_V_SPREE1, S_V_SPREE2, S_V_SPREE3, S_V_SPREE4,
	S_DAMAGE1, S_DAMAGE2, S_DAMAGE3, S_DAMAGE4, S_DAMAGE5, S_DAMAGE6, S_DAMAGE7, S_DAMAGE8,
	S_RESPAWN, S_CHAT, S_MENUPRESS, S_MENUBACK
#endif
};

// network messages codes, c2s, c2c, s2c

#ifndef BFRONTIER
enum { PRIV_NONE = 0, PRIV_MASTER, PRIV_ADMIN };
#endif

enum
{
	SV_INITS2C = 0, SV_INITC2S, SV_POS, SV_TEXT, SV_SOUND, SV_CDIS,
	SV_SHOOT, SV_EXPLODE, SV_SUICIDE, 
	SV_DIED, SV_DAMAGE, SV_HITPUSH, SV_SHOTFX,
	SV_TRYSPAWN, SV_SPAWNSTATE, SV_SPAWN, SV_FORCEDEATH, SV_ARENAWIN,
	SV_GUNSELECT, SV_TAUNT,
	SV_MAPCHANGE, SV_MAPVOTE, SV_ITEMSPAWN, SV_ITEMPICKUP, SV_DENIED,
	SV_PING, SV_PONG, SV_CLIENTPING,
	SV_TIMEUP, SV_MAPRELOAD, SV_ITEMACC,
	SV_SERVMSG, SV_ITEMLIST, SV_RESUME,
    SV_EDITMODE, SV_EDITENT, SV_EDITF, SV_EDITT, SV_EDITM, SV_FLIP, SV_COPY, SV_PASTE, SV_ROTATE, SV_REPLACE, SV_DELCUBE, SV_REMIP, SV_NEWMAP, SV_GETMAP, SV_SENDMAP,
	SV_MASTERMODE, SV_KICK, SV_CLEARBANS, SV_CURRENTMASTER, SV_SPECTATOR, SV_SETMASTER, SV_SETTEAM,
#ifdef BFRONTIER
	SV_BASES, SV_BASEINFO, SV_TEAMSCORE, SV_FORCEINTERMISSION,
#else
	SV_BASES, SV_BASEINFO, SV_TEAMSCORE, SV_REPAMMO, SV_FORCEINTERMISSION, SV_ANNOUNCE,
#endif
	SV_LISTDEMOS, SV_SENDDEMOLIST, SV_GETDEMO, SV_SENDDEMO,
	SV_DEMOPLAYBACK, SV_RECORDDEMO, SV_STOPDEMO, SV_CLEARDEMOS,
	SV_CLIENT,
#ifdef BFRONTIER
	SV_COMMAND, SV_RELOAD, SV_REGENERATE,
#endif
};

static char msgsizelookup(int msg)
{
	static char msgsizesl[] =				// size inclusive message token, 0 for variable or not-checked sizes
	{
		SV_INITS2C, 4, SV_INITC2S, 0, SV_POS, 0, SV_TEXT, 0, SV_SOUND, 2, SV_CDIS, 2,
		SV_SHOOT, 0, SV_EXPLODE, 0, SV_SUICIDE, 1,
		SV_DIED, 4, SV_DAMAGE, 6, SV_HITPUSH, 6, SV_SHOTFX, 9,
#ifdef BFRONTIER
		SV_TRYSPAWN, 1, SV_SPAWNSTATE, 9, SV_SPAWN, 3, SV_FORCEDEATH, 2, SV_ARENAWIN, 2,
#else
		SV_TRYSPAWN, 1, SV_SPAWNSTATE, 13, SV_SPAWN, 3, SV_FORCEDEATH, 2, SV_ARENAWIN, 2,
#endif
		SV_GUNSELECT, 2, SV_TAUNT, 1,
		SV_MAPCHANGE, 0, SV_MAPVOTE, 0, SV_ITEMSPAWN, 2, SV_ITEMPICKUP, 2, SV_DENIED, 2,
		SV_PING, 2, SV_PONG, 2, SV_CLIENTPING, 2,
		SV_TIMEUP, 2, SV_MAPRELOAD, 1, SV_ITEMACC, 3,
		SV_SERVMSG, 0, SV_ITEMLIST, 0, SV_RESUME, 0,
        SV_EDITMODE, 2, SV_EDITENT, 10, SV_EDITF, 16, SV_EDITT, 16, SV_EDITM, 15, SV_FLIP, 14, SV_COPY, 14, SV_PASTE, 14, SV_ROTATE, 15, SV_REPLACE, 16, SV_DELCUBE, 14, SV_REMIP, 1, SV_NEWMAP, 2, SV_GETMAP, 1, SV_SENDMAP, 0,
		SV_MASTERMODE, 2, SV_KICK, 2, SV_CLEARBANS, 1, SV_CURRENTMASTER, 3, SV_SPECTATOR, 3, SV_SETMASTER, 0, SV_SETTEAM, 0,
#ifdef BFRONTIER
		SV_BASES, 0, SV_BASEINFO, 0, SV_TEAMSCORE, 0, SV_FORCEINTERMISSION, 1,
#else
		SV_BASES, 0, SV_BASEINFO, 0, SV_TEAMSCORE, 0, SV_REPAMMO, 1, SV_FORCEINTERMISSION, 1,  SV_ANNOUNCE, 2,
#endif
		SV_LISTDEMOS, 1, SV_SENDDEMOLIST, 0, SV_GETDEMO, 2, SV_SENDDEMO, 0,
		SV_DEMOPLAYBACK, 2, SV_RECORDDEMO, 2, SV_STOPDEMO, 1, SV_CLEARDEMOS, 2,
		SV_CLIENT, 0,
#ifdef BFRONTIER
		SV_COMMAND, 0, SV_RELOAD, 0, SV_REGENERATE, 0,
#endif
		-1
	};
	for(char *p = msgsizesl; *p>=0; p += 2) if(*p==msg) return p[1];
	return -1;
}

#ifdef BFRONTIER
#define BFRONTIER_SERVER_PORT		28795
#define BFRONTIER_SERVINFO_PORT		28796
#define PROTOCOL_VERSION			BFRONTIER
#else
#define SAUERBRATEN_SERVER_PORT 28785
#define SAUERBRATEN_SERVINFO_PORT 28786
#define PROTOCOL_VERSION 254			// bump when protocol changes
#endif
#define DEMO_VERSION 1				  // bump when demo format changes
#define DEMO_MAGIC "SAUERBRATEN_DEMO"

struct demoheader
{
	char magic[16]; 
	int version, protocol;
};

#define MAXNAMELEN 15
#define MAXTEAMLEN 4

#ifdef BFRONTIER
/*
static struct entstat { int id; char *name; } entstats[] =
{
	{ NOTUSED,		"empty" }
	{ LIGHT,		"light" } // lightsource, attr1 = radius, attr2 = intensity
	{ MAPMODEL = ET_MAPMODEL,	// attr1 = angle, attr2 = idx
	{ PLAYERSTART,			// attr1 = angle
	{ ENVMAP = ET_ENVMAP,		// attr1 = radius
	{ PARTICLES = ET_PARTICLES,
	{ MAPSOUND = ET_SOUND,
	{ SPOTLIGHT = ET_SPOTLIGHT,
	{ I_PISTOL,
	{ I_SG,
	{ I_CG,
	{ I_GL,
	{ I_RL,
	{ I_RIFLE,
	{ TELEPORT,				// attr1 = idx
	{ TELEDEST,				// attr1 = angle, attr2 = idx
	{ MONSTER,				// attr1 = angle, attr2 = monstertype
	{ CARROT,					// attr1 = tag, attr2 = type
	{ JUMPPAD,				// attr1 = zpush, attr2 = ypush, attr3 = xpush
	{ BASE,
	{ RESPAWNPOINT,
	{ CAMERA,					// attr1 = yaw, attr2 = pitch, attr3 = pan (+:horiz/-:vert), attr4 = idx
	{ WAYPOINT,				// none?
};
*/
static struct itemstat { int add, max, sound; char *name; int info; } itemstats[] =
{
    {10,	10,		S_ITEMAMMO,		"PI",	GUN_PISTOL },
	{8,		8,		S_ITEMAMMO,		"SG",	GUN_SG },
    {30,	30,		S_ITEMAMMO,		"CG",	GUN_CG },
    {2,		4,		S_ITEMAMMO,		"GL",	GUN_GL },
    {1,		1,		S_ITEMAMMO,		"RL",	GUN_RL },
    {5,		5,		S_ITEMAMMO,		"RI",	GUN_RIFLE },
};
#define getitem(n) itemstats[n]

#define SGRAYS 20
#define SGSPREAD 3

#define RL_DAMRAD 30
#define RL_SELFDAMDIV 2
#define RL_DISTSCALE 1.5f

static struct guninfo { short sound, attackdelay, reloaddelay, damage, projspeed, part, kickamount, wobbleamount; char *name; } guns[NUMGUNS] =
{
	{ S_PISTOL,		250,	2250,	13,		0,		0,	-10 ,	10,	"pistol" },
	{ S_SG,			1000,	4000,	5,		0,		0,	-50,	50, "shotgun" },
	{ S_CG,			75,		3075,	8,		0,		0,	-10,	10,	"chaingun" },
	{ S_FLAUNCH,	1500,	600,	400,	40,		0,	-5,		5,	"grenades" },
	{ S_RLFIRE,		2500,	5000,	250,	80,		0,	-75,	50,	"rockets" },
	{ S_RIFLE,		1500,	4500,	50,		0,		0,	-30,	30,	"rifle" },
};
#define getgun(n) guns[n]
#define gunallowed(am,gn,gs) (gn >= GUN_PISTOL && gn <= GUN_RIFLE && (gs < 0 || gn != gs) && (gn != GUN_GL || (gs > -2 && am[gn]) || gs < -2))
#else
static struct itemstat { int add, max, sound; char *name; int info; } itemstats[] =
{
    {10,	30,		S_ITEMAMMO,		"SG",	GUN_SG} ,
    {20,	60,		S_ITEMAMMO,		"CG",	GUN_CG },
    {5,		15,		S_ITEMAMMO,		"RL",	GUN_RL },
    {5,		15,		S_ITEMAMMO,		"RI",	GUN_RIFLE },
    {10,	30,		S_ITEMAMMO,		"GL",	GUN_GL} ,
    {30,	120,	S_ITEMAMMO,		"PI",	GUN_PISTOL },
    {25,	100,	S_ITEMHEALTH,	"H" },
    {10,	1000,	S_ITEMHEALTH,	"MH" },
    {100,	100,	S_ITEMARMOUR,	"GA",	A_GREEN },
    {200,	200,	S_ITEMARMOUR,	"YA",	A_YELLOW },
    {20000,	30000,	S_ITEMPUP,		"Q" },
};

#define SGRAYS 20
#define SGSPREAD 4
#define RL_DAMRAD 40
#define RL_SELFDAMDIV 2
#define RL_DISTSCALE 1.5f

static struct guninfo { short sound, attackdelay, damage, projspeed, part, kickamount; char *name; } guns[NUMGUNS] =
{
	{ S_PUNCH1,		250,	50,		0,		0,  1,	"fist" },
	{ S_SG,			1400,	10,		0,		0,	20,	"shotgun" },  // *SGRAYS
	{ S_CG,			100,	30,		0,		0,	7,	"chaingun" },
	{ S_RLFIRE,		800,	120,	80,		0,	10,	"rocketlauncher" },
	{ S_RIFLE,		1500,	100,	0,		0,	30,	"rifle" },
	{ S_FLAUNCH,	500,	75,		80,		0,	10,	"grenadelauncher" },
	{ S_PISTOL,		500,	25,		0,		0,	7,	"pistol" },
	{ S_FLAUNCH,	200,	20,		50,		4,	1,	"fireball" },
	{ S_ICEBALL,	200,	40,		30,		6,	1,	"iceball" },
	{ S_SLIMEBALL,	200,	30,		160,	7,	1,	"slimeball" },
	{ S_PIGR1,		250,	50,		0,		0,	1,	"bite" },
};
#endif

// inherited by fpsent and server clients
struct fpsstate
{
#ifdef BFRONTIER
	int health, lastspawn, gunselect, gunwait[NUMGUNS], gunlast[NUMGUNS];
#else
	int health, maxhealth;
	int armour, armourtype;
	int quadmillis;
	int gunselect, gunwait;
#endif
	int ammo[NUMGUNS];

#ifdef BFRONTIER
	fpsstate() {}

	void addammo(int gun)
	{
		ammo[gun] += getitem(gun).add;
	}

	bool hasmaxammo(int type)
	{
		const itemstat &is = getitem(type);
		return ammo[type] >= is.max;
	}

	bool canpickup(int type)
	{
		itemstat &is = getitem(type);
		return ammo[is.info] < is.max;
	}
 
	void pickup(int type, int amt)
	{
		itemstat &is = getitem(type);
		ammo[is.info] = min(ammo[is.info] + (amt > 0 ? amt : is.add), is.max);
	}
#else
	fpsstate() : maxhealth(100) {}

	void baseammo(int gun, int k = 2)
	{
		ammo[gun] = itemstats[gun-GUN_SG].add*k;
	}

	void addammo(int gun, int k = 1)
	{
		ammo[gun] += itemstats[gun-GUN_SG].add*k;
	}

	bool hasmaxammo(int type)
	{
		const itemstat &is = itemstats[type-I_SHELLS];
		return ammo[type-I_SHELLS+GUN_SG]>=is.max;
	}

	bool canpickup(int type)
	{
		if(type<I_SHELLS || type>I_QUAD) return false;
		itemstat &is = itemstats[type-I_SHELLS];
		switch(type)
		{
			case I_BOOST: return maxhealth<is.max;
			case I_HEALTH: return health<maxhealth;
			case I_GREENARMOUR:
				// (100h/100g only absorbs 200 damage)
				if(armourtype==A_YELLOW && armour>=100) return false;
			case I_YELLOWARMOUR: return armour<is.max;
			case I_QUAD: return quadmillis<is.max;
			default: return ammo[is.info]<is.max;
		}
	}
 
	void pickup(int type)
	{
		if(type<I_SHELLS || type>I_QUAD) return;
		itemstat &is = itemstats[type-I_SHELLS];
		switch(type)
		{
			case I_BOOST:
				maxhealth = min(maxhealth+is.add, is.max);
			case I_HEALTH: // boost also adds to health
				health = min(health+is.add, maxhealth);
				break;
			case I_GREENARMOUR:
			case I_YELLOWARMOUR:
				armour = min(armour+is.add, is.max);
				armourtype = is.info;
				break;
			case I_QUAD:
				quadmillis = min(quadmillis+is.add, is.max);
				break;
			default:
				ammo[is.info] = min(ammo[is.info]+is.add, is.max);
				break;
		}
	}
#endif

	void respawn()
	{
#ifdef BFRONTIER
		health = 100;
		lastspawn = -1;
		loopi(NUMGUNS)
		{
			gunwait[i] = gunlast[i] = 0;
		}
		gunselect = GUN_PISTOL;
		loopi(NUMGUNS) ammo[i] = 0;
#else
		health = maxhealth;
		armour = 0;
		armourtype = A_BLUE;
		quadmillis = 0;
		gunselect = GUN_PISTOL;
		gunwait = 0;
		loopi(NUMGUNS) ammo[i] = 0;
		ammo[GUN_FIST] = 1;
#endif
	}

	void spawnstate(int gamemode)
	{
#ifdef BFRONTIER
		if(m_noitemsrail)
		{
			health = 1;
			gunselect = GUN_RIFLE;
			addammo(GUN_RIFLE);
		}
		else
		{
			health = 100;
			loopi(NUMGUNS)
			{
				if (gunallowed(ammo, i, -3)) addammo(i);
			}
		}
#else
		if(m_noitems || m_capture)
		{
			gunselect = GUN_RIFLE;
			armour = 0;
			if(m_noitemsrail)
			{
				health = 1;
				ammo[GUN_RIFLE] = 100;
			}
			else
			{
				armour = 100;
				armourtype = A_GREEN;

				if(m_tarena || m_capture)
				{
					ammo[GUN_PISTOL] = 80;
					int spawngun1 = rnd(5)+1, spawngun2;
					gunselect = spawngun1;
					baseammo(spawngun1, m_capture ? 1 : 2);
					do spawngun2 = rnd(5)+1; while(spawngun1==spawngun2);
					baseammo(spawngun2, m_capture ? 1 : 2);
					if(!m_capture) ammo[GUN_GL] += 1;
				}
				else // efficiency 
				{
					loopi(5) baseammo(i+1);
					gunselect = GUN_CG;
					ammo[GUN_CG] /= 2;
				}
			}
		}
		else
		{
			ammo[GUN_PISTOL] = m_sp ? 80 : 40;
			ammo[GUN_GL] = 1;
		}
#endif
	}

	// just subtract damage here, can set death, etc. later in code calling this 
	int dodamage(int damage)
	{
#ifndef BFRONTIER
		int ad = damage*(armourtype+1)*25/100; // let armour absorb when possible
		if(ad>armour) ad = armour;
		armour -= ad;
		damage -= ad;
#endif
		health -= damage;
		return damage;		
	}
};
#ifdef BFRONTIER
#define gunvar(gw,gn) (gw)[gn]
#endif

struct fpsent : dynent, fpsstate
{	
	int weight;						 // affects the effectiveness of hitpush
	int clientnum, privilege, lastupdate, plag, ping;
	int lifesequence;					// sequence id for each respawn, used in damage test
#ifdef BFRONTIER
	int lastpain, lastattackgun;
#else
	int lastpain;
	int lastaction, lastattackgun;
#endif
	bool attacking;
	int lasttaunt;
	int lastpickup, lastpickupmillis;
	int superdamage;
	int frags, deaths, totaldamage, totalshots;
	editinfo *edit;
#ifdef BFRONTIER
	#include "botent.h"
	int spree, lastimpulse, nexthealth;
#endif

	string name, team, info;

#ifdef BFRONTIER
	fpsent() : weight(100), clientnum(-1), privilege(PRIV_NONE), lastupdate(0), plag(0), ping(0), lifesequence(0), lastpain(0), frags(0), deaths(0), totaldamage(0), totalshots(0), edit(NULL),
				botflags(0), botstate(M_NONE), spree(0), lastimpulse(0), nexthealth(0)
#else
    fpsent() : weight(100), clientnum(-1), privilege(PRIV_NONE), lastupdate(0), plag(0), ping(0), lifesequence(0), lastpain(0), frags(0), deaths(0), totaldamage(0), totalshots(0), edit(NULL)
#endif
				{ name[0] = team[0] = info[0] = 0; respawn(); }
	~fpsent() { freeeditinfo(edit); }

	void damageroll(float damage)
	{
		float damroll = 2.0f*damage;
		roll += roll>0 ? damroll : (roll<0 ? -damroll : (rnd(2) ? damroll : -damroll)); // give player a kick
	}

	void hitpush(int damage, const vec &dir, fpsent *actor, int gun)
	{
		vec push(dir);
		push.mul(80*damage/weight);
		if(gun==GUN_RL || gun==GUN_GL) push.mul(actor==this ? 5 : (type==ENT_AI ? 3 : 2));
		vel.add(push);
	}

	void respawn()
	{
		dynent::reset();
		fpsstate::respawn();
#ifndef BFRONTIER
		lastaction = 0;
#endif
		lastattackgun = gunselect;
		attacking = false;
		lasttaunt = 0;
		lastpickup = -1;
		lastpickupmillis = 0;
		superdamage = 0;
#ifdef BFRONTIER
		spree = lastimpulse = 0;
		extern int lastmillis;
		nexthealth = lastmillis + 300;
#endif
	}
};

#ifdef BFRONTIER
enum
{
	SINFO_ICON = 0,
	SINFO_STATUS,
	SINFO_HOST,
	SINFO_DESC,
	SINFO_PING,
	SINFO_PLAYERS,
	SINFO_MAXCLIENTS,
	SINFO_MODE,
	SINFO_MAP,
	SINFO_TIME,
	SINFO_MAX
};

static char *serverinfotypes[] = {
	"",
	"status",
	"host",
	"description",
	"ping",
	"pl",
	"max",
	"game mode",
	"map name",
	"time left"
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
static char *serverstatustypes[] = {
	"\fs\f0open\fr",
	"\fs\f1locked\fr",
	"\fs\f2private\fr",
	"\fs\f3full\fr",
	"\fs\f4?\fr"
};

#ifndef STANDALONE
#define FPSMODHUDMAX		((h*3/FONTH)*FONTH)-(FONTH*5)		// max hud length
#define ILLUMINATE			48.f
#define ENTPART				4.f

enum {
	HD_OLD = 0,
	HD_LEFT,
	HD_RIGHT,
	HD_MAX
};

struct botent : fpsent
{
	bool c2sinit, connected, spectator;
	int lastping;
	vector<uchar> messages;
	
	botent() : c2sinit(false), connected(false), spectator(false) {}
	~botent() {}
};

#define botd(a)			((botent *)(a->d))

#define BOTMAX			MAXCLIENTS-1					// max bots

#define BOTLOSDIST(x)	(514.0f - (x * 2.0))			// line of sight distance = 512 MAX
#define BOTFOVX(x)		(96.5f - (x * 0.5))				// line of sight fov x angle = 96 MAX
#define BOTFOVY(x)		(128.5f - (x * 0.5))			// line of sight fov y angle = 128 MAX

#define BOTGAMEMODE(x)	(x >= -2 && x <= 11)			// available game modes

#define PATH_ABS		0x0001
#define PATH_AVOID		0x0002
#define PATH_GTONE		0x0004
#define PATH_BUTONE		0x0008

#define BOT_PLAYER		0x0001
#define BOT_MONSTER		0x0002

#define BOTISNEAR		8					// is near
#define BOTISFAR		96					// too far
#define BOTJUMPDIST		6					// decides to jump
#define BOTJUMPWEAP		24					// weapon to jump
#define BOTJUMPMAX		32					// too high
#define BOTRADIALDIST	48					// radial distance
#define BOTMELEEDIST	12					// use melee
#define BOTCHECKRESET	500					// check this interval for reset
#define BOTCHECKCOORD	5000				// check this interval for coordindates
#define BOTJUMPTIME		100					// don't jump too soon
#define BOTJUMPWAIT		1000				// don't jump stupidly
#define BOTSCOUTDIST	20					// scout this many nodes randomly

int waypoints = 0;

VARP(botrate, 1, 10, 100);					// rate of action updates and judgement errors
VARP(botauto, 0, 0, 1);						// automically load bots on start of map

VAR(botdrop, 0, 0, 1);						// drop botwaypoints during play
VAR(botnum, 0, 0, MAXCLIENTS-1);			// set to force a number of bots in botauto
VAR(botwaydist, 0, BOTJUMPMAX, BOTISFAR);	// drop botwaypoints this far apart
VAR(botwaynear, 0, BOTISNEAR, BOTISFAR);	// waypoints join when this close to them

#endif
static char *msgnames[] = {
	"SV_INITS2C",
	"SV_INITC2S",
	"SV_POS",
	"SV_TEXT",
	"SV_SOUND",
	"SV_CDIS",
	"SV_SHOOT",
	"SV_EXPLODE",
	"SV_SUICIDE",
	"SV_DIED",
	"SV_DAMAGE",
	"SV_HITPUSH",
	"SV_SHOTFX",
	"SV_TRYSPAWN",
	"SV_SPAWNSTATE",
	"SV_SPAWN",
	"SV_FORCEDEATH",
	"SV_ARENAWIN",
	"SV_GUNSELECT",
	"SV_TAUNT",
	"SV_MAPCHANGE",
	"SV_MAPVOTE",
	"SV_ITEMSPAWN",
	"SV_ITEMPICKUP",
	"SV_DENIED",
	"SV_PING",
	"SV_PONG",
	"SV_CLIENTPING",
	"SV_TIMEUP",
	"SV_MAPRELOAD",
	"SV_ITEMACC",
	"SV_SERVMSG",
	"SV_ITEMLIST",
	"SV_RESUME",
	"SV_EDITMODE",
	"SV_EDITENT",
	"SV_EDITF",
	"SV_EDITT",
	"SV_EDITM",
	"SV_FLIP",
	"SV_COPY",
	"SV_PASTE",
	"SV_ROTATE",
	"SV_REPLACE",
	"SV_DELCUBE",
	"SV_REMIP",
	"SV_NEWMAP",
	"SV_GETMAP",
	"SV_SENDMAP",
	"SV_MASTERMODE",
	"SV_KICK",
	"SV_CLEARBANS",
	"SV_CURRENTMASTER",
	"SV_SPECTATOR",
	"SV_SETMASTER",
	"SV_SETTEAM",
	"SV_BASES",
	"SV_BASEINFO",
	"SV_TEAMSCORE",
	"SV_FORCEINTERMISSION",
	"SV_LISTDEMOS",
	"SV_SENDDEMOLIST",
	"SV_GETDEMO",
	"SV_SENDDEMO",
	"SV_DEMOPLAYBACK",
	"SV_RECORDDEMO",
	"SV_STOPDEMO",
	"SV_CLEARDEMOS",
	"SV_CLIENT",
	"SV_COMMAND",
	"SV_RELOAD",
	"SV_REGENERATE",
};
#endif
