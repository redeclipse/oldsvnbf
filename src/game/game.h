#ifndef __GAME_H__
#define __GAME_H__

#include "engine.h"

#define GAMEID              "bfa"
#define GAMEVERSION         167
#define DEMO_VERSION        GAMEVERSION

#define MAXAI 256
#define MAXPLAYERS (MAXCLIENTS + MAXAI)

// network quantization scale
#define DMF 16.0f           // for world locations
#define DNF 100.0f          // for normalized vectors
#define DVELF 1.0f          // for playerspeed based velocity vectors

enum
{
    S_JUMP = S_GAMESPECIFIC, S_IMPULSE, S_LAND, S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6, S_DIE1, S_DIE2, S_SPLASH1, S_SPLASH2, S_UNDERWATER,
    S_SPLAT, S_SPLOSH, S_DEBRIS, S_TINK, S_RICOCHET, S_WHIZZ, S_WHIRR, S_BEEP, S_EXPLODE, S_ENERGY, S_HUM, S_BURN, S_BURNING, S_BURNFIRE, S_EXTINGUISH, S_BZAP, S_BZZT,
    S_RELOAD, S_SWITCH, S_MELEE, S_MELEE2, S_PISTOL, S_PISTOL2, S_SWORD, S_SWORD2, S_SHOTGUN, S_SHOTGUN2, S_SMG, S_SMG2, S_GRENADE, S_GRENADE2, S_FLAMER, S_FLAMER2, S_PLASMA, S_PLASMA2, S_RIFLE, S_RIFLE2,
    S_ITEMPICKUP, S_ITEMSPAWN, S_REGEN, S_DAMAGE1, S_DAMAGE2, S_DAMAGE3, S_DAMAGE4, S_DAMAGE5, S_DAMAGE6, S_DAMAGE7, S_DAMAGE8, S_BURNDAMAGE,
    S_RESPAWN, S_CHAT, S_ERROR, S_ALARM, S_V_FLAGSECURED, S_V_FLAGOVERTHROWN, S_V_FLAGPICKUP, S_V_FLAGDROP, S_V_FLAGRETURN, S_V_FLAGSCORE, S_V_FLAGRESET,
    S_V_FIGHT, S_V_CHECKPOINT, S_V_ONEMINUTE, S_V_HEADSHOT, S_V_SPREE1, S_V_SPREE2, S_V_SPREE3, S_V_SPREE4, S_V_SPREE5, S_V_SPREE6, S_V_MKILL1, S_V_MKILL2, S_V_MKILL3,
    S_V_REVENGE, S_V_DOMINATE, S_V_YOUWIN, S_V_YOULOSE, S_V_MCOMPLETE, S_V_FRAGGED, S_V_OWNED, S_V_DENIED,
    S_NOTIFY, S_SHELL1, S_SHELL2, S_SHELL3, S_SHELL4, S_SHELL5, S_SHELL6, S_CRITDAMAGE, S_ROCKET, S_ROCKET2, // move after release
    S_MAX
};

enum                                // entity types
{
    NOTUSED = ET_EMPTY, LIGHT = ET_LIGHT, MAPMODEL = ET_MAPMODEL, PLAYERSTART = ET_PLAYERSTART, ENVMAP = ET_ENVMAP, PARTICLES = ET_PARTICLES,
    MAPSOUND = ET_SOUND, LIGHTFX = ET_LIGHTFX, SUNLIGHT = ET_SUNLIGHT, WEAPON = ET_GAMESPECIFIC,
    TELEPORT, ACTOR, TRIGGER, PUSHER, FLAG, CHECKPOINT, CAMERA, WAYPOINT, MAXENTTYPES
};

enum { EU_NONE = 0, EU_ITEM, EU_AUTO, EU_ACT, EU_MAX };

enum { TR_TOGGLE = 0, TR_LINK, TR_SCRIPT, TR_ONCE, TR_EXIT, TR_MAX };
enum { TA_MANUAL = 0, TA_AUTO, TA_ACTION, TA_MAX };
#define TRIGGERIDS      16
#define TRIGSTATE(a,b)  (b%2 ? !a : a)

enum { CP_RESPAWN = 0, CP_START, CP_FINISH, CP_LAST, CP_MAX };

struct enttypes
{
    int type,           priority, links,    radius, usetype,    numattrs,
            canlink,
            reclink;
    bool    noisy;  const char *name,           *attrs[9];
};
#ifdef GAMESERVER
enttypes enttype[] = {
    {
        NOTUSED,        -1,         0,      0,      EU_NONE,    0,
            0,
            0,
            true,               "none",         { "" }
    },
    {
        LIGHT,          1,          59,     0,      EU_NONE,    4,
            (1<<LIGHTFX),
            (1<<LIGHTFX),
            false,              "light",        { "radius", "red",      "green",    "blue"  }
    },
    {
        MAPMODEL,       1,          58,     0,      EU_NONE,    6,
            (1<<TRIGGER),
            (1<<TRIGGER),
            false,              "mapmodel",     { "type",   "yaw",      "rot",      "blend",    "scale",    "flags" }
    },
    {
        PLAYERSTART,    1,          59,     0,      EU_NONE,    5,
            0,
            0,
            false,              "playerstart",  { "team",   "yaw",      "pitch",    "mode",     "id" }
    },
    {
        ENVMAP,         1,          0,      0,      EU_NONE,    1,
            0,
            0,
            false,              "envmap",       { "radius" }
    },
    {
        PARTICLES,      1,          59,     0,      EU_NONE,    9,
            (1<<TELEPORT)|(1<<TRIGGER)|(1<<PUSHER),
            (1<<TRIGGER)|(1<<PUSHER),
            false,              "particles",    { "type",   "a",        "b",        "c",        "d",        "e",        "f",        "g",        "i" }
    },
    {
        MAPSOUND,       1,          58,     0,      EU_NONE,    5,
            (1<<TELEPORT)|(1<<TRIGGER)|(1<<PUSHER),
            (1<<TRIGGER)|(1<<PUSHER),
            false,              "sound",        { "type",   "maxrad",   "minrad",   "volume",   "flags" }
    },
    {
        LIGHTFX,        1,          1,      0,      EU_NONE,    5,
            (1<<LIGHT)|(1<<TELEPORT)|(1<<TRIGGER)|(1<<PUSHER),
            (1<<LIGHT)|(1<<TRIGGER)|(1<<PUSHER),
            false,              "lightfx",      { "type",   "mod",      "min",      "max",      "flags" }
    },
    {
        SUNLIGHT,       1,          160,    0,      EU_NONE,    6,
            0,
            0,
            false,              "sunlight",     { "yaw",    "pitch",    "red",      "green",    "blue",     "offset" }
    },
    {
        WEAPON,         2,          59,     16,     EU_ITEM,    4,
            0,
            0,
            false,              "weapon",       { "type",   "flags",    "mode",     "id" }
    },
    {
        TELEPORT,       1,          50,     12,     EU_AUTO,    5,
            (1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX)|(1<<TELEPORT),
            (1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            false,              "teleport",     { "yaw",    "pitch",    "push",     "radius",   "colour" }
    },
    {
        ACTOR,          1,          59,     0,      EU_NONE,    8,
            (1<<FLAG)|(1<<WAYPOINT),
            0,
            false,              "actor",        { "type",   "yaw",      "pitch",    "mode",     "id",       "weap",     "health",   "speed" }
    },
    {
        TRIGGER,        1,          58,     16,     EU_AUTO,    5,
            (1<<MAPMODEL)|(1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            (1<<MAPMODEL)|(1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            false,              "trigger",      { "id",     "type",     "action",   "radius",   "state" }
    },
    {
        PUSHER,         1,          58,     12,     EU_AUTO,    6,
            (1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            (1<<MAPSOUND)|(1<<PARTICLES)|(1<<LIGHTFX),
            false,              "pusher",       { "yaw",    "pitch",    "force",    "maxrad",   "minrad",   "type" }
    },
    {
        FLAG,           1,          48,     36,     EU_NONE,    5,
            (1<<FLAG),
            0,
            false,              "flag",         { "team",   "yaw",      "pitch",    "mode",     "id" }
    },
    {
        CHECKPOINT,     1,          48,     16,     EU_AUTO,    6,
            0,
            0,
            false,              "checkpoint",   { "radius", "yaw",      "pitch",    "mode",     "id",       "type" }
    },
    {
        CAMERA,         1,          48,     0,      EU_NONE,    3,
            (1<<CAMERA),
            0,
            false,              "camera",       { "type",   "mindist",  "maxdist" }
    },
    {
        WAYPOINT,       0,          1,      16,     EU_NONE,    1,
            (1<<WAYPOINT),
            0,
            true,               "waypoint",     { "flags" }
    }
};
#else
extern enttypes enttype[];
#endif

enum
{
    ANIM_PAIN = ANIM_GAMESPECIFIC, ANIM_JUMP,
    ANIM_IMPULSE_FORWARD, ANIM_IMPULSE_BACKWARD, ANIM_IMPULSE_LEFT, ANIM_IMPULSE_RIGHT, ANIM_IMPULSE_DASH,
    ANIM_SINK, ANIM_EDIT, ANIM_LAG, ANIM_SWITCH, ANIM_PICKUP, ANIM_WIN, ANIM_LOSE,
    ANIM_CROUCH, ANIM_CRAWL_FORWARD, ANIM_CRAWL_BACKWARD, ANIM_CRAWL_LEFT, ANIM_CRAWL_RIGHT,
    ANIM_MELEE, ANIM_MELEE_PRIMARY, ANIM_MELEE_SECONDARY,
    ANIM_WIELD, ANIM_WIELD_PRIMARY, ANIM_WIELD_SECONDARY,
    ANIM_LIGHT, ANIM_LIGHT_PRIMARY, ANIM_LIGHT_SECONDARY, ANIM_LIGHT_RELOAD,
    ANIM_HEAVY, ANIM_HEAVY_PRIMARY, ANIM_HEAVY_SECONDARY, ANIM_HEAVY_RELOAD,
    ANIM_GRASP, ANIM_GRASP_PRIMARY, ANIM_GRASP_SECONDARY, ANIM_GRASP_RELOAD, ANIM_GRASP_POWER,
    ANIM_VWEP, ANIM_SHIELD, ANIM_POWERUP,
    ANIM_MAX
};

#define WEAPSWITCHDELAY PHYSMILLIS*2

#ifndef GAMESERVER
#define FIRECOLOURS 8
const int firecols[FIRECOLOURS] = { 0xFF5808, 0x981808, 0x782808, 0x481808, 0x983818, 0x601808, 0xFF1808, 0x381808 };
#endif

#include "weapons.h"

enum
{
    FRAG_NONE = 0, FRAG_HEADSHOT = 1<<1, FRAG_OBLITERATE = 1<<2,
    FRAG_SPREE1 = 1<<3, FRAG_SPREE2 = 1<<4, FRAG_SPREE3 = 1<<5, FRAG_SPREE4 = 1<<6, FRAG_SPREE5 = 1<<7, FRAG_SPREE6 = 1<<8,
    FRAG_MKILL1 = 1<<9, FRAG_MKILL2 = 1<<10, FRAG_MKILL3 = 1<<11, FRAG_REVENGE = 1<<12, FRAG_DOMINATE = 1<<13,
    FRAG_SPREES = 6, FRAG_SPREE = 3, FRAG_MKILL = 9,
    FRAG_CHECK = FRAG_SPREE1|FRAG_SPREE2|FRAG_SPREE3|FRAG_SPREE4|FRAG_SPREE5|FRAG_SPREE6,
    FRAG_MULTI = FRAG_MKILL1|FRAG_MKILL2|FRAG_MKILL3,
};

enum
{
    G_DEMO = 0, G_EDITMODE, G_CAMPAIGN, G_DEATHMATCH, G_STF, G_CTF, G_TRIAL, G_MAX,
    G_START = G_EDITMODE, G_PLAY = G_CAMPAIGN, G_FIGHT = G_DEATHMATCH, G_RAND = G_CTF-G_DEATHMATCH+1
};
enum
{
    G_M_NONE = 0,
    G_M_TEAM = 1<<0, G_M_INSTA = 1<<1, G_M_MEDIEVAL = 1<<2, G_M_DEMOLITION = 1<<3,
    G_M_DUEL = 1<<4, G_M_SURVIVOR = 1<<5, G_M_ARENA = 1<<6,
    G_M_ALL = G_M_TEAM|G_M_INSTA|G_M_MEDIEVAL|G_M_DEMOLITION|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA,
    G_M_NUM = 7
};

struct gametypes
{
    int type,           mutators,                                                                           implied;        const char *name;
};
#ifdef GAMESERVER
gametypes gametype[] = {
    { G_DEMO,           G_M_NONE,                                                                           G_M_NONE,       "demo" },
    { G_EDITMODE,       G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,                                    G_M_NONE,       "editing" },
    { G_CAMPAIGN,       G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,                           G_M_TEAM,       "campaign" },
    { G_DEATHMATCH,     G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,     G_M_NONE,       "deathmatch" },
    { G_STF,            G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,                           G_M_TEAM,       "secure-the-flag" },
    { G_CTF,            G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,                           G_M_TEAM,       "capture-the-flag" },
    { G_TRIAL,          G_M_TEAM|G_M_INSTA|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,                           G_M_NONE,       "time-trial" },
}, mutstype[] = {
    { G_M_TEAM,         G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,     G_M_TEAM,       "team" },
    { G_M_INSTA,        G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,     G_M_INSTA,      "insta" },
    { G_M_MEDIEVAL,     G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_MEDIEVAL,                              G_M_MEDIEVAL,   "medieval" },
    { G_M_DEMOLITION,   G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_DEMOLITION,                            G_M_DEMOLITION, "demolition" },
    { G_M_DUEL,         G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,                  G_M_DUEL,       "duel" },
    { G_M_SURVIVOR,     G_M_TEAM|G_M_INSTA|G_M_SURVIVOR|G_M_ARENA|G_M_MEDIEVAL|G_M_DEMOLITION,              G_M_SURVIVOR,   "survivor" },
    { G_M_ARENA,        G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_SURVIVOR|G_M_ARENA,                                 G_M_ARENA,      "arena" },
};
#else
extern gametypes gametype[], mutstype[];
#endif

#define m_game(a)           (a > -1 && a < G_MAX)
#define m_check(a,b)        (!a || (a < 0 ? -a != b : a == b))

#define m_demo(a)           (a == G_DEMO)
#define m_edit(a)           (a == G_EDITMODE)
#define m_campaign(a)       (a == G_CAMPAIGN)
#define m_dm(a)             (a == G_DEATHMATCH)
#define m_stf(a)            (a == G_STF)
#define m_ctf(a)            (a == G_CTF)
#define m_trial(a)          (a == G_TRIAL)

#define m_play(a)           (a >= G_PLAY)
#define m_flag(a)           (m_stf(a) || m_ctf(a))
#define m_fight(a)          (a >= G_FIGHT)

#define m_team(a,b)         ((b & G_M_TEAM) || (gametype[a].implied & G_M_TEAM))
#define m_insta(a,b)        ((b & G_M_INSTA) || (gametype[a].implied & G_M_INSTA))
#define m_medieval(a,b)     ((b & G_M_MEDIEVAL) || (gametype[a].implied & G_M_MEDIEVAL))
#define m_demolition(a,b)   ((b & G_M_DEMOLITION) || (gametype[a].implied & G_M_DEMOLITION))
#define m_duel(a,b)         ((b & G_M_DUEL) || (gametype[a].implied & G_M_DUEL))
#define m_survivor(a,b)     ((b & G_M_SURVIVOR) || (gametype[a].implied & G_M_SURVIVOR))
#define m_arena(a,b)        ((b & G_M_ARENA) || (gametype[a].implied & G_M_ARENA))

#define m_limited(a,b)      (m_insta(a, b) || m_medieval(a, b) || m_demolition(a, b))
#define m_duke(a,b)         (m_duel(a, b) || m_survivor(a, b))
#define m_regen(a,b)        (!m_duke(a, b) && !m_insta(a, b))

#define m_weapon(a,b)       (m_arena(a,b) ? -1 : (m_medieval(a,b) ? WEAP_SWORD : (m_demolition(a,b) ? WEAP_ROCKET : (m_edit(a) || m_trial(a) ? GAME(limitedweapon) : (m_insta(a,b) ? GAME(instaweapon) : GAME(spawnweapon))))))
#define m_delay(a,b)        (m_play(a) && !m_duke(a,b) ? (m_trial(a) ? GAME(trialdelay) : ((m_insta(a, b) ? GAME(instadelay) : GAME(spawndelay)))) : 0)
#define m_protect(a,b)      (m_duke(a,b) ? GAME(duelprotect) : (m_insta(a, b) ? GAME(instaprotect) : GAME(spawnprotect)))
#define m_noitems(a,b)      (!m_trial(a) && GAME(itemsallowed) < (m_insta(a,b) ? 2 : 1))
#define m_health(a,b)       (m_insta(a,b) ? 1 : GAME(maxhealth))

#define w_reload(w1,w2)     (w1 != WEAP_MELEE && (w1 == (isweap(w2) ? w2 : WEAP_PISTOL) || (isweap(w1) && WEAP(w1, reloads))))
#define w_carry(w1,w2)      (w1 > WEAP_MELEE && w1 != (isweap(w2) ? w2 : WEAP_PISTOL) && (isweap(w1) && WEAP(w1, carried)))
#define w_attr(a,w1,w2)     (m_edit(a) || (w1 >= WEAP_OFFSET && w1 != w2) ? w1 : (w2 == WEAP_GRENADE ? WEAP_ROCKET : WEAP_GRENADE))
#define w_spawn(weap)       int(GAME(itemspawntime)*WEAP(weap, frequency))

// network messages codes, c2s, c2c, s2c
enum
{
    N_CONNECT = 0, N_SERVERINIT, N_WELCOME, N_CLIENTINIT, N_POS, N_SPHY, N_TEXT, N_COMMAND, N_ANNOUNCE, N_DISCONNECT,
    N_SHOOT, N_DESTROY, N_SUICIDE, N_DIED, N_POINTS, N_DAMAGE, N_SHOTFX,
    N_LOADWEAP, N_TRYSPAWN, N_SPAWNSTATE, N_SPAWN, N_DROP, N_WEAPSELECT,
    N_MAPCHANGE, N_MAPVOTE, N_CHECKPOINT, N_ITEMSPAWN, N_ITEMUSE, N_TRIGGER, N_EXECLINK,
    N_PING, N_PONG, N_CLIENTPING, N_TICK, N_NEWGAME, N_ITEMACC, N_SERVMSG, N_GAMEINFO, N_RESUME,
    N_EDITMODE, N_EDITENT, N_EDITLINK, N_EDITVAR, N_EDITF, N_EDITT, N_EDITM, N_FLIP, N_COPY, N_PASTE, N_ROTATE, N_REPLACE, N_DELCUBE, N_REMIP, N_CLIPBOARD, N_NEWMAP,
    N_GETMAP, N_SENDMAP, N_SENDMAPFILE, N_SENDMAPSHOT, N_SENDMAPCONFIG,
    N_MASTERMODE, N_KICK, N_CLEARBANS, N_CURRENTMASTER, N_SPECTATOR, N_WAITING, N_SETMASTER, N_SETTEAM,
    N_FLAGS, N_FLAGINFO,
    N_TAKEFLAG, N_RETURNFLAG, N_RESETFLAG, N_DROPFLAG, N_SCOREFLAG, N_INITFLAGS, N_SCORE,
    N_LISTDEMOS, N_SENDDEMOLIST, N_GETDEMO, N_SENDDEMO,
    N_DEMOPLAYBACK, N_RECORDDEMO, N_STOPDEMO, N_CLEARDEMOS,
    N_CLIENT, N_RELOAD, N_REGEN,
    N_ADDBOT, N_DELBOT, N_INITAI,
    N_MAPCRC, N_CHECKMAPS,
    N_SWITCHNAME, N_SWITCHTEAM,
    N_AUTHTRY, N_AUTHCHAL, N_AUTHANS,
    NUMSV
};

#ifdef GAMESERVER
char msgsizelookup(int msg)
{
    static const int msgsizes[] =               // size inclusive message token, 0 for variable or not-checked sizes
    {
        N_CONNECT, 0, N_SERVERINIT, 5, N_WELCOME, 1, N_CLIENTINIT, 0, N_POS, 0, N_SPHY, 0, N_TEXT, 0, N_COMMAND, 0,
        N_ANNOUNCE, 0, N_DISCONNECT, 3,
        N_SHOOT, 0, N_DESTROY, 0, N_SUICIDE, 3, N_DIED, 8, N_POINTS, 4, N_DAMAGE, 10, N_SHOTFX, 0,
        N_LOADWEAP, 0, N_TRYSPAWN, 2, N_SPAWNSTATE, 0, N_SPAWN, 0,
        N_DROP, 0, N_WEAPSELECT, 0,
        N_MAPCHANGE, 0, N_MAPVOTE, 0, N_CHECKPOINT, 0, N_ITEMSPAWN, 3, N_ITEMUSE, 0, N_TRIGGER, 0, N_EXECLINK, 3,
        N_PING, 2, N_PONG, 2, N_CLIENTPING, 2,
        N_TICK, 2, N_NEWGAME, 1, N_ITEMACC, 0,
        N_SERVMSG, 0, N_GAMEINFO, 0, N_RESUME, 0,
        N_EDITMODE, 2, N_EDITENT, 0, N_EDITLINK, 4, N_EDITVAR, 0, N_EDITF, 16, N_EDITT, 16, N_EDITM, 15, N_FLIP, 14, N_COPY, 14, N_PASTE, 14, N_ROTATE, 15, N_REPLACE, 16, N_DELCUBE, 14, N_REMIP, 1, N_NEWMAP, 2,
        N_GETMAP, 0, N_SENDMAP, 0, N_SENDMAPFILE, 0, N_SENDMAPSHOT, 0, N_SENDMAPCONFIG, 0,
        N_MASTERMODE, 2, N_KICK, 2, N_CLEARBANS, 1, N_CURRENTMASTER, 3, N_SPECTATOR, 3, N_WAITING, 2, N_SETMASTER, 0, N_SETTEAM, 0,
        N_FLAGS, 0, N_FLAGINFO, 0,
        N_DROPFLAG, 0, N_SCOREFLAG, 5, N_RETURNFLAG, 3, N_TAKEFLAG, 3, N_RESETFLAG, 2, N_INITFLAGS, 0, N_SCORE, 0,
        N_LISTDEMOS, 1, N_SENDDEMOLIST, 0, N_GETDEMO, 2, N_SENDDEMO, 0,
        N_DEMOPLAYBACK, 3, N_RECORDDEMO, 2, N_STOPDEMO, 1, N_CLEARDEMOS, 2,
        N_CLIENT, 0, N_RELOAD, 0, N_REGEN, 0,
        N_ADDBOT, 0, N_DELBOT, 0, N_INITAI, 0,
        N_MAPCRC, 0, N_CHECKMAPS, 1,
        N_SWITCHNAME, 0, N_SWITCHTEAM, 0,
        N_AUTHTRY, 0, N_AUTHCHAL, 0, N_AUTHANS, 0,
        -1
    };
    static int sizetable[NUMSV] = { -1 };
    if(sizetable[0] < 0)
    {
        memset(sizetable, -1, sizeof(sizetable));
        for(const int *p = msgsizes; *p >= 0; p += 2) sizetable[p[0]] = p[1];
    }
    return msg >= 0 && msg < NUMSV ? sizetable[msg] : -1;
}
#else
extern char msgsizelookup(int msg);
#endif
enum { SPHY_NONE = 0, SPHY_JUMP, SPHY_DASH, SPHY_BOOST, SPHY_KICK, SPHY_POWER, SPHY_EXTINGUISH, SPHY_MAX };
enum { CON_CHAT = CON_GAMESPECIFIC, CON_EVENT, CON_MAX, CON_LO = CON_MESG, CON_HI = CON_SELF, CON_IMPORTANT = CON_SELF };

#define DEMO_MAGIC "BFDZ"
struct demoheader
{
    char magic[16];
    int version, gamever;
};

enum
{
    TEAM_NEUTRAL = 0, TEAM_ALPHA, TEAM_BETA, TEAM_ENEMY, TEAM_MAX,
    TEAM_FIRST = TEAM_ALPHA, TEAM_LAST = TEAM_BETA, TEAM_NUM = (TEAM_LAST-TEAM_FIRST)+1, TEAM_COUNT = TEAM_LAST+1
};
struct teamtypes
{
    int type,           colour; const char  *name,
        *tpmdl,                             *fpmdl,
        *flag,          *icon,              *chat,      *colname;
};
#ifdef GAMESERVER
teamtypes teamtype[] = {
    {
        TEAM_NEUTRAL,   0x666666,           "neutral",
        "actors/player",                    "actors/player/vwep",
        "flag",         "team",             "\fa",      "grey"
    },
    {
        TEAM_ALPHA,     0x2222AA,           "alpha",
        "actors/player/alpha",              "actors/player/alpha/vwep",
        "flag/alpha",   "teamalpha",        "\fb",      "blue"
    },
    {
        TEAM_BETA,      0xAA2222,           "beta",
        "actors/player/beta",               "actors/player/beta/vwep",
        "flag/beta",    "teambeta",         "\fr",      "red"
    },
    {
        TEAM_ENEMY,     0x222222,           "enemy",
        "actors/player",                    "actors/player/vwep",
        "flag",         "team",             "\fd",      "dark grey"
    }
};
#else
extern teamtypes teamtype[];
#endif
enum { BASE_NONE = 0, BASE_HOME = 1<<0, BASE_FLAG = 1<<1, BASE_BOTH = BASE_HOME|BASE_FLAG };

#define numteams(a,b)   (m_fight(a) && m_team(a,b) ? TEAM_NUM : 1)
#define isteam(a,b,c,d) (m_fight(a) && m_team(a,b) ? (c >= d && c <= numteams(a,b)) : c == TEAM_NEUTRAL)
#define valteam(a,b)    (a >= b && a <= TEAM_NUM)
#define adjustscaled(t,n,s) { \
    if(n > 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/float(s))); if(n <= 0) n = (t)0; } \
    else if(n < 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/float(s))); if(n >= 0) n = (t)0; } \
}

#define MAXNAMELEN 24
enum { SAY_NONE = 0, SAY_ACTION = 1<<0, SAY_TEAM = 1<<1, SAY_NUM = 2 };
enum { PRIV_NONE = 0, PRIV_MASTER, PRIV_ADMIN, PRIV_MAX };

#define MM_MODE 0xF
#define MM_AUTOAPPROVE 0x1000
#define MM_FREESERV (MM_AUTOAPPROVE|MM_MODE)
#define MM_VETOSERV ((1<<MM_OPEN)|(1<<MM_VETO))
#define MM_COOPSERV (MM_AUTOAPPROVE|MM_VETOSERV|(1<<MM_LOCKED))
#define MM_OPENSERV (MM_AUTOAPPROVE|(1<<MM_OPEN))

enum { MM_OPEN = 0, MM_VETO, MM_LOCKED, MM_PRIVATE, MM_PASSWORD };
enum { CAMERA_NONE = 0, CAMERA_PLAYER, CAMERA_FOLLOW, CAMERA_ENTITY, CAMERA_MAX };
enum { SINFO_STATUS = 0, SINFO_NAME, SINFO_PORT, SINFO_QPORT, SINFO_DESC, SINFO_MODE, SINFO_MUTS, SINFO_MAP, SINFO_TIME, SINFO_NUMPLRS, SINFO_MAXPLRS, SINFO_PING, SINFO_MAX };
enum { SSTAT_OPEN = 0, SSTAT_LOCKED, SSTAT_PRIVATE, SSTAT_FULL, SSTAT_UNKNOWN, SSTAT_MAX };

enum { AC_ATTACK = 0, AC_ALTERNATE, AC_RELOAD, AC_USE, AC_JUMP, AC_SPRINT, AC_CROUCH, AC_SPECIAL, AC_TOTAL, AC_DASH = AC_TOTAL, AC_MAX };
enum { IM_METER = 0, IM_TYPE, IM_TIME, IM_COUNT, IM_MAX };
enum { IM_T_NONE = 0, IM_T_BOOST, IM_T_DASH, IM_T_KICK, IM_T_SKATE, IM_T_MAX, IM_T_WALL = IM_T_KICK };

#define CROUCHHEIGHT 0.7f
#define PHYSMILLIS 250

#include "ai.h"
#include "vars.h"

// inherited by gameent and server clients
struct gamestate
{
    int health, ammo[WEAP_MAX], entid[WEAP_MAX];
    int lastweap, loadweap, weapselect, weapload[WEAP_MAX], weapshot[WEAP_MAX], weapstate[WEAP_MAX], weapwait[WEAP_MAX], weaplast[WEAP_MAX];
    int lastdeath, lastspawn, lastrespawn, lastpain, lastregen, lastfire;
    int aitype, aientity, ownernum, skill, points, frags, deaths, cpmillis, cptime;

    gamestate() : loadweap(-1), weapselect(WEAP_MELEE), lastdeath(0), lastspawn(0), lastrespawn(0), lastpain(0), lastregen(0), lastfire(0),
        aitype(-1), aientity(-1), ownernum(-1), skill(0), points(0), frags(0), deaths(0), cpmillis(0), cptime(0) {}
    ~gamestate() {}

    int hasweap(int weap, int sweap, int level = 0, int exclude = -1)
    {
        if(isweap(weap) && weap != exclude)
        {
            if(ammo[weap] > 0 || (w_reload(weap, sweap) && !ammo[weap])) switch(level)
            {
                case 0: default: return true; break; // has weap at all
                case 1: if(w_carry(weap, sweap)) return true; break; // only carriable
                case 2: if(ammo[weap] > 0) return true; break; // only with actual ammo
                case 3: if(ammo[weap] > 0 && w_reload(weap, sweap)) return true; break; // only reloadable with actual ammo
                case 4: if(ammo[weap] >= (w_reload(weap, sweap) ? 0 : WEAP(weap, max))) return true; break; // only reloadable or those with < max
                case 5: if(w_carry(weap, sweap) || (!w_reload(weap, sweap) && weap >= WEAP_OFFSET)) return true; break; // special case for usable weapons
            }
        }
        return false;
    }

    int bestweap(int sweap, bool last = false)
    {
        if(last && hasweap(lastweap, sweap)) return lastweap;
        loopirev(WEAP_MAX) if(hasweap(i, sweap, 3)) return i; // reloadable first
        loopirev(WEAP_MAX) if(hasweap(i, sweap, 1)) return i; // carriable second
        loopirev(WEAP_MAX) if(hasweap(i, sweap, 0)) return i; // any just to bail us out
        return weapselect;
    }

    int carry(int sweap, int level = 1, int exclude = -1)
    {
        int carry = 0;
        loopi(WEAP_MAX) if(hasweap(i, sweap, level, exclude)) carry++;
        return carry;
    }

    int drop(int sweap, int exclude = -1)
    {
        int weap = -1;
        if(hasweap(weapselect, sweap, 1, exclude)) weap = weapselect;
        else loopi(WEAP_MAX) if(hasweap(i, sweap, 1, exclude))
        {
            weap = i;
            break;
        }
        return weap;
    }

    void weapreset(bool full = false)
    {
        loopi(WEAP_MAX)
        {
            weapstate[i] = WEAP_S_IDLE;
            weapwait[i] = weaplast[i] = weapload[i] = weapshot[i] = 0;
            if(full) ammo[i] = entid[i] = -1;
        }
    }

    void setweapstate(int weap, int state, int delay, int millis)
    {
        weapstate[weap] = state;
        weapwait[weap] = delay;
        weaplast[weap] = millis;
    }

    void weapswitch(int weap, int millis, int state = WEAP_S_SWITCH)
    {
        if(isweap(weap))
        {
            lastweap = weapselect;
            setweapstate(lastweap, WEAP_S_SWITCH, WEAPSWITCHDELAY, millis);
            weapselect = weap;
            setweapstate(weap, state, WEAPSWITCHDELAY, millis);
        }
    }

    bool weapwaited(int weap, int millis, int skip = 0)
    {
        if(!weapwait[weap] || weapstate[weap] == WEAP_S_IDLE || weapstate[weap] == WEAP_S_POWER || (skip && skip&(1<<weapstate[weap]))) return true;
        return millis-weaplast[weap] >= weapwait[weap];
    }

    int skipwait(int weap, int flags, int millis, int skip, bool override = false)
    {
        int skipstate = skip;
        if(WEAP2(weap, sub, flags&HIT_ALT) && (skip&(1<<WEAP_S_RELOAD)) && weapstate[weap] == WEAP_S_RELOAD && millis-weaplast[weap] < weapwait[weap])
        {
            if(!override && ammo[weap]-weapload[weap] < WEAP2(weap, sub, flags&HIT_ALT))
                skipstate &= ~(1<<WEAP_S_RELOAD);
        }
        return skipstate;
    }

    bool canswitch(int weap, int sweap, int millis, int skip = 0)
    {
        if((aitype >= AI_START || weap != WEAP_MELEE || sweap == WEAP_MELEE || weapselect == WEAP_MELEE) && weap != weapselect && weapwaited(weapselect, millis, skipwait(weapselect, 0, millis, skip, true)) && hasweap(weap, sweap) && weapwaited(weap, millis, skipwait(weap, 0, millis, skip, true)))
            return true;
        return false;
    }

    bool canshoot(int weap, int flags, int sweap, int millis, int skip = 0)
    {
        if((hasweap(weap, sweap) && ammo[weap] >= (WEAP2(weap, power, flags&HIT_ALT) ? 1 : WEAP2(weap, sub, flags&HIT_ALT))) && weapwaited(weap, millis, skipwait(weap, flags, millis, skip)))
            return true;
        return false;
    }

    bool canreload(int weap, int sweap, int millis)
    {
        if(weap == weapselect && w_reload(weap, sweap) && hasweap(weap, sweap) && ammo[weap] < WEAP(weap, max) && weapwaited(weap, millis))
            return true;
        return false;
    }

    bool canuse(int type, int attr, vector<int> &attrs, int sweap, int millis, int skip = 0)
    {
        //if((type != TRIGGER || attrs[2] == TA_AUTO) && enttype[type].usetype == EU_AUTO) return true;
        //if(weapwaited(weapselect, millis, skipwait(weapselect, 0, millis, skip)))
        switch(enttype[type].usetype)
        {
            case EU_AUTO: case EU_ACT: return true; break;
            case EU_ITEM:
            { // can't use when reloading or firing
                if(isweap(attr) && !hasweap(attr, sweap, 4) && weapwaited(weapselect, millis, skipwait(weapselect, 0, millis, skip, true)))
                    return true;
                break;
            }
            default: break;
        }
        return false;
    }

    void useitem(int id, int type, int attr, vector<int> &attrs, int sweap, int millis)
    {
        switch(type)
        {
            case TRIGGER: break;
            case WEAPON:
            {
                int prev = ammo[attr];
                weapswitch(attr, millis, hasweap(attr, sweap) ? WEAP_S_SWITCH : WEAP_S_PICKUP);
                ammo[attr] = clamp(max(ammo[attr], 0)+WEAP(attr, add), 1, WEAP(attr, max));
                weapload[attr] = ammo[attr]-prev;
                entid[attr] = id;
                break;
            }
            default: break;
        }
    }

    void clearstate()
    {
        lastdeath = lastpain = lastregen = lastfire = 0;
        lastrespawn = -1;
    }

    void mapchange()
    {
        points = cpmillis = cptime = 0;
    }

    void respawn(int millis, int heal)
    {
        health = heal;
        lastspawn = millis;
        clearstate();
        weapreset(true);
    }

    void spawnstate(int gamemode, int mutators, int sweap = -1, int heal = 0)
    {
        health = heal ? heal : m_health(gamemode, mutators);
        weapreset(true);
        if(!isweap(sweap)) sweap = aitype >= AI_START ? WEAP_MELEE : m_weapon(gamemode, mutators);
        if(aitype < AI_START && sweap != WEAP_MELEE) ammo[WEAP_MELEE] = WEAP(WEAP_MELEE, max);
        if(isweap(sweap)) ammo[sweap] = max(WEAP(sweap, reloads) ? WEAP(sweap, add) : WEAP(sweap, max), 1);
        if(GAME(spawngrenades) >= (m_insta(gamemode, mutators) || m_trial(gamemode) ? 2 : 1) && sweap != WEAP_GRENADE)
            ammo[WEAP_GRENADE] = max(WEAP(WEAP_GRENADE, max), 1);
        if(m_arena(gamemode, mutators))
        {
            int aweap = loadweap;
            while(aweap < WEAP_OFFSET || aweap >= WEAP_ITEM) aweap = rnd(WEAP_ITEM-WEAP_OFFSET)+WEAP_OFFSET; // pistol = random
            ammo[aweap] = max(WEAP(aweap, reloads) ? WEAP(aweap, add) : WEAP(aweap, max), 1);
            lastweap = weapselect = aweap;
        }
        else
        {
            loadweap = -1;
            lastweap = weapselect = sweap;
        }
    }

    void editspawn(int gamemode, int mutators, int sweap = -1, int heal = 0)
    {
        clearstate();
        spawnstate(gamemode, mutators, sweap, heal);
    }

    int respawnwait(int millis, int delay)
    {
        return lastdeath ? max(0, delay-(millis-lastdeath)) : 0;
    }

    // just subtract damage here, can set death, etc. later in code calling this
    void dodamage(int heal)
    {
        lastregen = 0;
        health = heal;
    }

    int protect(int millis, int delay)
    {
        int amt = 0;
        if(aitype < AI_START && lastspawn && delay && millis-lastspawn <= delay) amt = delay-(millis-lastspawn);
        return amt;
    }

    bool onfire(int millis, int len) { return len && lastfire && millis-lastfire <= len; }
};

namespace server
{
    extern void stopdemo();
    extern void hashpassword(int cn, int sessionid, const char *pwd, char *result, int maxlen = MAXSTRLEN);
}

#if !defined(GAMESERVER) && !defined(STANDALONE)
struct gameentity : extentity
{
    int schan;
    int lastuse, lastspawn;
    int mark;
    vector<int> kin;

    gameentity() : schan(-1), lastuse(0), lastspawn(0), mark(0) { kin.shrink(0); }
    ~gameentity()
    {
        if(issound(schan)) removesound(schan);
        schan = -1;
        kin.shrink(0);
    }
};

enum
{
    ST_NONE     = 0,
    ST_CAMERA   = 1<<0,
    ST_CURSOR   = 1<<1,
    ST_GAME     = 1<<2,
    ST_SPAWNS   = 1<<3,
    ST_DEFAULT  = ST_CAMERA|ST_CURSOR|ST_GAME,
    ST_VIEW     = ST_CURSOR|ST_CAMERA,
    ST_ALL      = ST_CAMERA|ST_CURSOR|ST_GAME|ST_SPAWNS,
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
const char * const animnames[] =
{
    "idle", "forward", "backward", "left", "right", "dead", "dying", "swim",
    "mapmodel", "trigger on", "trigger off", "pain", "jump",
    "impulse forward", "impulse backward", "impulse left", "impulse right", "impulse dash",
    "sink", "edit", "lag", "switch", "pickup", "win", "lose",
    "crouch", "crawl forward", "crawl backward", "crawl left", "crawl right",
    "melee", "melee primary", "melee secondary",
    "wield", "wield primary", "wield secondary",
    "light", "light primary", "light secondary", "light reload",
    "heavy", "heavy primary", "heavy secondary", "heavy reload",
    "grasp", "grasp primary", "grasp secondary", "grasp reload", "grasp power",
    "vwep", "shield", "powerup",
    ""
};
#else
extern const char * const animnames[];
#endif

struct gameent : dynent, gamestate
{
    editinfo *edit; ai::aiinfo *ai;
    int team, clientnum, privilege, lastnode, checkpoint, cplast, respawned, suicided, lastupdate, lastpredict, plag, ping, lastflag, totaldamage,
        actiontime[AC_MAX], impulse[IM_MAX], lastsprint, smoothmillis, turnmillis, turnside, aschan, vschan, wschan, fschan, lasthit, lastkill, lastattacker, lastpoints, quake,
        lastpush, lastjump;
    float deltayaw, deltapitch, newyaw, newpitch, deltaaimyaw, deltaaimpitch, newaimyaw, newaimpitch, turnyaw, turnroll;
    vec head, torso, muzzle, origin, eject, waist, lfoot, rfoot, legs, hrad, trad, lrad;
    bool action[AC_MAX], conopen, k_up, k_down, k_left, k_right, obliterated;
    string name, info, obit;
    vector<int> airnodes;
    vector<gameent *> dominating, dominated;

    gameent() : edit(NULL), ai(NULL), team(TEAM_NEUTRAL), clientnum(-1), privilege(PRIV_NONE), checkpoint(-1), cplast(0), lastupdate(0), lastpredict(0), plag(0), ping(0),
        totaldamage(0), smoothmillis(-1), turnmillis(0), aschan(-1), vschan(-1), wschan(-1), fschan(-1),
        lastattacker(-1), lastpoints(0), quake(0), lastpush(0), lastjump(0),
        head(-1, -1, -1), torso(-1, -1, -1), muzzle(-1, -1, -1), origin(-1, -1, -1), eject(-1, -1, -1), waist(-1, -1, -1),
        lfoot(-1, -1, -1), rfoot(-1, -1, -1), legs(-1, -1, -1), hrad(-1, -1, -1), trad(-1, -1, -1), lrad(-1, -1, -1),
        conopen(false), k_up(false), k_down(false), k_left(false), k_right(false), obliterated(false)
    {
        name[0] = info[0] = obit[0] = 0;
        weight = 200; // so we can control the 'gravity' feel
        maxspeed = 50; // ditto for movement
        dominating.shrink(0);
        dominated.shrink(0);
        checktags();
        respawn(-1, 100);
    }
    ~gameent()
    {
        removesounds();
        freeeditinfo(edit);
        if(ai) delete ai;
        removetrackedparticles(this);
        removetrackedsounds(this);
    }

    void removesounds()
    {
        if(issound(aschan)) removesound(aschan);
        if(issound(vschan)) removesound(vschan);
        if(issound(wschan)) removesound(wschan);
        if(issound(fschan)) removesound(fschan);
        aschan = vschan = wschan = fschan = -1;
    }

    void stopmoving(bool full)
    {
        if(full) move = strafe = lastsprint = 0;
        loopi(AC_MAX)
        {
            action[i] = false;
            actiontime[i] = 0;
        }
    }

    void clearstate()
    {
        loopi(IM_MAX) impulse[i] = 0;
        cplast = lasthit = lastkill = quake = lastpush = turnmillis = turnside = 0;
        turnroll = turnyaw = 0;
        lastflag = respawned = suicided = lastnode = -1;
        obit[0] = 0;
        obliterated = false;
    }

    void respawn(int millis, int heal)
    {
        stopmoving(true);
        removesounds();
        clearstate();
        physent::reset();
        gamestate::respawn(millis, heal);
        airnodes.setsize(0);
    }

    void editspawn(int gamemode, int mutators, int sweap = -1, int heal = 0)
    {
        stopmoving(true);
        clearstate();
        inmaterial = timeinair = 0;
        inliquid = onladder = false;
        strafe = move = 0;
        physstate = PHYS_FALL;
        vel = falling = vec(0, 0, 0);
        floor = vec(0, 0, 1);
        resetinterp();
        gamestate::editspawn(gamemode, mutators, sweap, heal);
        airnodes.setsize(0);
    }

    void resetstate(int millis, int heal)
    {
        respawn(millis, heal);
        frags = deaths = totaldamage = 0;
    }

    void mapchange(int millis, int heal)
    {
        checkpoint = -1;
        dominating.shrink(0);
        dominated.shrink(0);
        resetstate(millis, heal);
        gamestate::mapchange();
    }

    void cleartags() { head = torso = muzzle = origin = eject = waist = lfoot = rfoot = vec(-1, -1, -1); }

    vec checkoriginpos()
    {
        if(origin == vec(-1, -1, -1))
        {
            vec dir, right; vecfromyawpitch(yaw, pitch, 1, 0, dir); vecfromyawpitch(yaw, pitch, 0, -1, right);
            dir.mul(radius*0.5f); right.mul(radius); dir.z -= height*0.0625f;
            origin = vec(o).add(dir).add(right);
        }
        return origin;
    }

    vec originpos(bool melee = false, bool secondary = false)
    {
        if(melee) return secondary ? feetpos() : headpos(height*0.0625f);
        return checkoriginpos();
    }

    vec checkmuzzlepos()
    {
        if(muzzle == vec(-1, -1, -1))
        {
            vec dir, right; vecfromyawpitch(yaw, pitch, 1, 0, dir); vecfromyawpitch(yaw, pitch, 0, -1, right);
            dir.mul(radius); right.mul(radius); dir.z -= height*0.0625f;
            muzzle = vec(o).add(dir).add(right);
        }
        return muzzle;
    }

    vec muzzlepos(int weap, bool secondary = false)
    {
        if(isweap(weap) && weaptype[weap].muzzle) return checkmuzzlepos();
        return originpos(weap == WEAP_MELEE, secondary);
    }

    vec checkejectpos()
    {
        if(eject == vec(-1, -1, -1)) eject = checkmuzzlepos();
        return eject;
    }

    vec ejectpos(int weap)
    {
        if(isweap(weap) && weaptype[weap].eject) return checkejectpos();
        return muzzlepos(weap);
    }

    void checkhitboxes()
    {
        float hsize = max(xradius*0.45f, yradius*0.45f); if(head == vec(-1, -1, -1)) { torso = head; head = o; head.z -= hsize; }
        vec dir; vecfromyawpitch(yaw, pitch+90, 1, 0, dir); dir.mul(hsize); head.add(dir); hrad = vec(xradius*0.45f, yradius*0.45f, hsize);
        if(torso == vec(-1, -1, -1)) { torso = o; torso.z -= height*0.5f; } torso.z += hsize*0.5f;
        float tsize = (head.z-hrad.z)-torso.z; trad = vec(xradius, yradius, tsize);
        float lsize = ((torso.z-trad.z)-(o.z-height))*0.5f; legs = torso; legs.z -= trad.z+lsize; lrad = vec(xradius*0.8f, yradius*0.8f, lsize);
        if(waist == vec(-1, -1, -1))
        {
            vecfromyawpitch(yaw, 0, -1, 0, dir); dir.mul(radius*1.15f); dir.z -= height*0.5f;
            waist = vec(o).add(dir);
        }
        if(lfoot == vec(-1, -1, -1))
        {
            vecfromyawpitch(yaw, 0, 0, -1, dir); dir.mul(radius); dir.z -= height;
            lfoot = vec(o).add(dir);
        }
        if(rfoot == vec(-1, -1, -1))
        {
            vecfromyawpitch(yaw, 0, 0, 1, dir); dir.mul(radius); dir.z -= height;
            rfoot = vec(o).add(dir);
        }
    }

    bool wantshitbox() { return type == ENT_PLAYER || (type == ENT_AI && (!isaitype(aitype) || aistyle[aitype].canmove)); }

    void checktags()
    {
        checkoriginpos();
        checkmuzzlepos();
        checkejectpos();
        if(wantshitbox()) checkhitboxes();
    }

    float calcroll(bool crouch)
    {
        float r = roll, wobble = float(rnd(15)-7)*(float(min(quake, 100))/100.f);
        switch(state)
        {
            case CS_SPECTATOR: case CS_WAITING: r = wobble*0.5f; break;
            case CS_ALIVE: if(crouch) wobble *= 0.5f;
            case CS_DEAD: r += wobble; break;
            default: break;
        }
        return r;
    }

    void doimpulse(int cost, int type, int millis)
    {
        impulse[IM_METER] += cost;
        impulse[IM_TIME] = millis;
        if(!lastjump && type > IM_T_NONE && type < IM_T_WALL)
        {
            impulse[IM_TYPE] = IM_T_NONE;
            lastjump = millis;
        }
        else
        {
            impulse[IM_TYPE] = type;
            impulse[IM_COUNT]++;
        }
        resetphys();
    }

    void dojumpreset(bool full = false)
    {
        if(full) lastjump = 0;
        else resetphys();
        timeinair = turnside = impulse[IM_COUNT] = impulse[IM_TYPE] = 0;
    }

    void resetfire()
    {
        if(issound(fschan)) removesound(fschan);
        fschan = -1;
        lastfire = 0;
    }
};

enum { PRJ_SHOT = 0, PRJ_GIBS, PRJ_DEBRIS, PRJ_EJECT, PRJ_ENT, PRJ_MAX };

struct projent : dynent
{
    vec from, to, norm;
    int addtime, lifetime, lifemillis, waittime, spawntime, fadetime, lastradial, lasteffect, lastbounce, beenused, extinguish;
    float movement, roll, lifespan, lifesize, scale;
    bool local, limited, stuck, escaped;
    int projtype, projcollide;
    float elasticity, reflectivity, relativity, waterfric;
    int schan, id, weap, flags, hitflags;
    entitylight light;
    gameent *owner;
    physent *hit, *target;
    const char *mdl;

    projent() : norm(0, 0, 1), projtype(PRJ_SHOT), id(-1), hitflags(HITFLAG_NONE), owner(NULL), hit(NULL), target(NULL), mdl(NULL) { reset(); }
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
        addtime = lifetime = lifemillis = waittime = spawntime = fadetime = lastradial = lasteffect = lastbounce = beenused = flags = 0;
        schan = id = weap = -1;
        movement = roll = lifespan = lifesize = 0.f;
        scale = 1.f;
        extinguish = 0;
        limited = stuck = escaped = false;
        projcollide = BOUNCE_GEOM|BOUNCE_PLAYER;
    }

    bool ready(bool used = true)
    {
        if(owner && (!used || !beenused) && waittime <= 0 && state != CS_DEAD)
            return true;
        return false;
    }
};

namespace client
{
    extern bool demoplayback, sendinfo, sendcrc;
    extern void clearvotes(gameent *d);
    extern void addmsg(int type, const char *fmt = NULL, ...);
    extern void c2sinfo(bool force = false);
}

namespace physics
{
    extern float gravity, liquidspeed, liquidcurb, floorcurb, aircurb;
    extern int smoothmove, smoothdist;
    extern bool allowimpulse(int level = 2);
    extern bool sprinting(physent *d, bool last = false, bool turn = true, bool move = true);
    extern bool canimpulse(physent *d, int cost = 0, int level = 2);
    extern bool movecamera(physent *pl, const vec &dir, float dist, float stepdist);
    extern void smoothplayer(gameent *d, int res, bool local);
    extern void update();
}

namespace projs
{
    extern vector<projent *> projs;

    extern void reset();
    extern void update();
    extern void create(vec &from, vec &to, bool local, gameent *d, int type, int lifetime, int lifemillis, int waittime, int speed, int id = 0, int weap = -1, int flags = 0, float scale = 1);
    extern void preload();
    extern void remove(gameent *owner);
    extern void shootv(int weap, int flags, int offset, float scale, vec &from, vector<vec> &locs, gameent *d, bool local);
    extern void drop(gameent *d, int g, int n, bool local = true);
    extern void adddynlights();
    extern void render();
}

namespace weapons
{
    extern int autoreloading;
    extern int slot(gameent *d, int n, bool back = false);
    extern bool weapselect(gameent *d, int weap, bool local = true);
    extern bool weapreload(gameent *d, int weap, int load = -1, int ammo = -1, bool local = true);
    extern void reload(gameent *d);
    extern bool doshot(gameent *d, vec &targ, int weap, bool pressed = false, bool secondary = false, int force = 0);
    extern void shoot(gameent *d, vec &targ, int force = 0);
    extern void preload();
}

namespace hud
{
    extern char *conopentex, *playertex, *deadtex, *dominatingtex, *dominatedtex, *inputtex, *bliptex, *cardtex, *flagtex, *arrowtex, *alerttex;
    extern int hudwidth, hudheight, hudsize, lastteam, lastnewgame, damageresidue, damageresiduefade, shownotices, radaraffinitynames, inventorygame, inventoryaffinity, teamkillnum;
    extern float noticescale, inventoryblend, inventoryskew, inventorygrow, radaraffinityblend, radarblipblend, radaraffinitysize;
    extern vector<int> teamkills;
    extern bool chkcond(int val, bool cond);
    extern char *timetostr(int dur, int style = 0);
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
    extern void drawblip(const char *tex, float area, int w, int h, float s, float blend, vec &dir, float r = 1.f, float g = 1.f, float b = 1.f, const char *font = "sub", const char *text = NULL, ...);
    extern int drawprogress(int x, int y, float start, float length, float size, bool left, float r = 1.f, float g = 1.f, float b = 1.f, float fade = 1.f, float skew = 1.f, const char *font = NULL, const char *text = NULL, ...);
    extern int drawitem(const char *tex, int x, int y, float size, bool left = false, float r = 1.f, float g = 1.f, float b = 1.f, float fade = 1.f, float skew = 1.f, const char *font = NULL, const char *text = NULL, ...);
    extern int drawitemsubtext(int x, int y, float size, int align = TEXT_RIGHT_UP, float skew = 1.f, const char *font = NULL, float blend = 1.f, const char *text = NULL, ...);
    extern int drawweapons(int x, int y, int s, float blend = 1.f);
    extern int drawhealth(int x, int y, int s, float blend = 1.f);
    extern void drawinventory(int w, int h, int edge, float blend = 1.f);
    extern void damage(int n, const vec &loc, gameent *actor, int weap, int flags);
    extern const char *teamtex(int team = TEAM_NEUTRAL);
    extern const char *itemtex(int type, int stype);
}

namespace game
{
    extern int numplayers, gamemode, mutators, nextmode, nextmuts, timeremaining, maptime,
            zoomtime, lastzoom, lasttvcam, lasttvchg, spectvtime, waittvtime, showplayerinfo,
                bloodfade, bloodsize, debrisfade, fogdist, aboveheadfade, announcefilter, dynlighteffects, shownamesabovehead, thirdpersonfollow;
    extern float bloodscale, debrisscale;
    extern bool intermission, zooming;
    extern vec swaypush, swaydir;
    extern string clientmap;

    extern gameent *player1, *focus;
    extern vector<gameent *> players;

    struct avatarent : dynent
    {
        avatarent() { type = ENT_CAMERA; }
    };
    extern avatarent avatarmodel;

    struct camstate
    {
        int ent, idx;
        vec pos, dir;
        vector<int> cansee;
        float mindist, maxdist, score;

        camstate() : idx(-1), mindist(4), maxdist(4096), score(0) { reset(); }
        ~camstate() {}

        void reset()
        {
            cansee.shrink(0);
            dir = vec(0, 0, 0);
            score = 0;
        }

        static int camsort(const camstate *a, const camstate *b)
        {
            if(a->score < b->score) return -1;
            if(a->score > b->score) return 1;
            return 0;
        }
    };
    extern vector<camstate> cameras;

    extern gameent *newclient(int cn);
    extern gameent *getclient(int cn);
    extern gameent *intersectclosest(vec &from, vec &to, gameent *at);
    extern void clientdisconnected(int cn, int reason = DISC_NONE);
    extern char *colorname(gameent *d, char *name = NULL, const char *prefix = "", bool team = true, bool dupname = true);
    extern void announce(int idx, int targ, gameent *d, const char *msg, ...);
    extern void respawn(gameent *d);
    extern void impulseeffect(gameent *d, bool effect);
    extern void suicide(gameent *d, int flags);
    extern void fixrange(float &yaw, float &pitch);
    extern void fixfullrange(float &yaw, float &pitch, float &roll, bool full);
    extern void getyawpitch(const vec &from, const vec &pos, float &yaw, float &pitch);
    extern void scaleyawpitch(float &yaw, float &pitch, float targyaw, float targpitch, float frame = 1.f, float scale = 1.f);
    extern bool allowmove(physent *d);
    extern int mousestyle();
    extern int deadzone();
    extern int panspeed();
    extern void checkzoom();
    extern bool inzoom();
    extern bool inzoomswitch();
    extern void zoomview(bool down);
    extern bool tvmode();
    extern void resetcamera();
    extern void resetworld();
    extern void resetstate();
    extern void hiteffect(int weap, int flags, int damage, gameent *d, gameent *actor, vec &dir, bool local = false);
    extern void damaged(int weap, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir);
    extern void killed(int weap, int flags, int damage, gameent *d, gameent *actor, int style);
    extern void timeupdate(int timeremain);
}

namespace entities
{
    extern int showentdescs;
    extern vector<extentity *> ents;
    extern int lastenttype[MAXENTTYPES], lastusetype[EU_MAX];
    extern void clearentcache();
    extern int closestent(int type, const vec &pos, float mindist, bool links = false);
    extern bool collateitems(gameent *d, vector<actitem> &actitems);
    extern void checkitems(gameent *d);
    extern void putitems(packetbuf &p);
    extern void execlink(gameent *d, int index, bool local, int ignore = -1);
    extern void setspawn(int n, int m);
    extern bool tryspawn(dynent *d, const vec &o, short yaw = 0, short pitch = 0);
    extern void spawnplayer(gameent *d, int ent = -1, bool suicide = false);
    extern const char *entinfo(int type, vector<int> &attr, bool full = false);
    extern void useeffects(gameent *d, int n, bool s, int g, int r);
    extern const char *entmdlname(int type, vector<int> &attr);
    extern bool clipped(const vec &o, bool aiclip = false);
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
            obstacles.setsize(0);
            entities.setsize(0);
        }

        void add(dynent *ent)
        {
            obstacle &ob = obstacles.add(obstacle(ent));
            if(!ent) ob.above = enttype[WAYPOINT].radius;
            else switch(ent->type)
            {
                case ENT_PLAYER: case ENT_AI:
                {
                    gameent *e = (gameent *)ent;
                    ob.above = e->abovehead().z;
                    break;
                }
                case ENT_PROJ:
                {
                    projent *p = (projent *)ob.ent;
                    if(p->projtype == PRJ_SHOT && WEAP2(p->weap, explode, p->flags&HIT_ALT))
                        ob.above = p->o.z+(WEAP2(p->weap, explode, p->flags&HIT_ALT)*p->lifesize)+1.f;
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
                    if(!ob.ent || ob.ent != (d)) \
                    { \
                        for(; cur < next; cur++) if((v).entities.inrange(cur)) \
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
                    if(!ob.ent || ob.ent != d || (d->blocked && ob.ent == d))
                    {
                        for(; cur < next; cur++) if(entities[cur] == n)
                        {
                            if(ob.above < 0) return -1;
                            vec above(pos.x, pos.y, ob.above);
                            if(above.z-d->o.z >= ai::JUMPMAX)
                                return -1; // too much scotty
                            int node = closestent(WAYPOINT, above, ai::SIGHTMIN, true);
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
    extern float route(int node, int goal, vector<int> &route, const avoidset &obstacles, gameent *d = NULL, bool check = true);
}
#elif defined(GAMESERVER)
namespace client
{
    extern const char *getname();
}
#endif
#include "ctf.h"
#include "stf.h"
#ifndef GAMESERVER
#include "scoreboard.h"
#endif

#endif

