// Blood Frontier - SSPGAME - Side Scrolling Platformer by Quinton Reeves
// This is the primary SSP definitions.

// defines
#define SSPDEBUG		1

#define SSPVERSION		2
#define SSPSAVEVER		2

#define XPLAYER			12.0f	// player default x position
#define XWORLD			64.0f	// world default x position

#define HITCLOB			4.0f	// clobber extended radius
#define ENTPART			4.0f	// extra push for entity particles
#define ENTBLK			3.9f	// space blocks consume
#define EXPLOSION		16.0f	// explosion radius
#define ILLUMINATE		48.0f	// dynlight radius
#define BLKTIME			600		// time for block animations

#define SGRAYS			20		// sspweap
#define SGSPREAD		4		// sspweap
#define OFFMILLIS		500		// sspweap

#define MAXPOWER		3		// maximum power level of player
#define MAXSAVES		3

enum {
	ENT_BLOCK = 100,
	ENT_MAX
};

// externs
extern int curtexnum, curtime, gamespeed, lastmillis;
extern int hidehud, fov, hudgunfov, invmouse, sensitivity, sensitivityscale;
extern int musicvol, soundvol, entselradius;

extern physent *player, *camera1, *hitplayer;

extern bool menuactive();
extern void show_out_of_renderloop_progress(float bar1, const char *text1, float bar2 = 0, const char *text2 = NULL, GLuint tex = 0);

// globals
bool inbounds(vec &d, float h1, float r1, vec &v, float h2, float r2)
{
	return d.x <= v.x+r2+r1 && d.x >= v.x-r2-r1 &&
	d.y <= v.y+r2+r1 && d.y >= v.y-r2-r1 &&
	d.z <= v.z+h2+h1 && d.z >= v.z-h2-h1;
}

enum {
	SF_JUMP = 0, SF_LAND, SF_SPLASH1, SF_SPLASH2, SF_RUMBLE,
	SF_PAIN1, SF_PAIN2, SF_PAIN3, SF_PAIN4, SF_PAIN5, SF_PAIN6,
	SF_DIE1, SF_DIE2,
	SF_HIT1, SF_HIT2, SF_HIT3, SF_HIT4, SF_HIT5, SF_HIT6, SF_HIT7, SF_HIT8,
	SF_PUNCH, SF_SG, SF_CG, SF_RL, SF_RIFLE, SF_GL, SF_BOMB, SF_FIRE, SF_ICE, SF_SLIME, SF_EXPLODE,
	SF_COIN, SF_POWERUP, SF_SHIELD, SF_INVINCIBLE, SF_INVOVER, SF_LIFE,
	SF_CHKPOINT, SF_GOAL, SF_JUMPPAD, SF_TELEPORT,
	SF_MAX
};

enum { PR_GIBS, PR_DEBRIS, PR_ENT, PR_BNC, PR_PROJ };

enum {
	WT_WATER = 0,
	WT_HURT,
	WT_KILL,
	WT_MAX
};

// movement
enum { DR_NONE = 0, DR_LEFT, DR_UP, DR_RIGHT, DR_DOWN, DR_MAX };

float diryaw[DR_MAX] = { 0.0f, 0.0f, 90.0f, 180.0f, 270.0f };
vec dirvec[DR_MAX] = { vec(0, 0, 0), vec(0, -1, 0), vec(1, 0, 0), vec(0, 1, 0), vec(-1, 0, 0) };

#define dirnorm(d,v) { v.x *= dirvec[d].x; v.y *= dirvec[d].y; }

#define isxaxis(d)		(d == DR_NONE || d == DR_LEFT || d == DR_RIGHT)
#define isyaxis(d)		(!isxaxis(d))

#define nilamt(a,b,c)		(a <= 0.f ? a+b : a)

#define renderinc(a,b,i,t,x,y) \
{ \
	if (a != b) \
	{ \
		if (lastmillis-t < i) \
		{ \
			float xx = a-b; \
			if (y) \
			{ \
				if (xx >= (x*0.5f)) xx -= x; \
				else if (xx <= (-x*0.5f)) xx += x; \
			} \
			float xy = ((xx/float(i))*float(lastmillis-t)); \
			a = b+xy; \
			if (a >= x) a -= x; \
			else if (a < 0.f) a += x; \
		} \
		else b = a; \
	} \
}

// player
enum {
	GN_FIST = 0, GN_SG, GN_CG, GN_RL, GN_RIFLE, GN_GL, GN_BOMB,
	GN_FIREBALL, GN_ICEBALL, GN_SLIMEBALL,
	GN_MAX
};

enum {
	CH_POWER = 0, CH_WEAPON, CH_SHIELD, CH_MAX
};

char *cheats[CH_MAX] = { "power", "weapon", "shield" };

enum {
	PW_HIT = 0, PW_GRENADE, PW_ROCKET, PW_BOMB, PW_MAX
};

char *pname[PW_MAX] = {
	"life", "grenades", "rockets", "bombs"
};

char *picons[PW_MAX] = {
	"packages/icons/ssp/item_life.jpg",
	"packages/icons/ssp/item_grenades.jpg",
	"packages/icons/ssp/item_rockets.jpg",
	"packages/icons/ssp/item_bombs.jpg"
};

int pgun[PW_MAX] = {
	GN_FIST, GN_GL, GN_RL, GN_BOMB
};

int pcol[PW_MAX] = {
	COL_WHITE, COL_CYAN, COL_RED, COL_BLUE
};

char *powers[MAXPOWER+PW_MAX] = {
	"ironsnout/ssp/small",
	"ironsnout/ssp/small",
	"ironsnout/ssp/life",
	"ironsnout/ssp/grenades",
	"ironsnout/ssp/rockets",
	"ironsnout/ssp/bombs"
};

char *icons[MAXPOWER+PW_MAX] = {
	"packages/icons/ssp/player.jpg",
	"packages/icons/ssp/player.jpg",
	"packages/icons/ssp/player.jpg",
	"packages/icons/ssp/player_grenades.jpg",
	"packages/icons/ssp/player_rockets.jpg",
	"packages/icons/ssp/player_bombs.jpg",
};

int pw[GN_MAX] = {
	PW_HIT,
	PW_HIT,
	PW_HIT,
	PW_ROCKET,
	PW_HIT,
	PW_GRENADE,
	PW_BOMB,
	PW_HIT,
	PW_HIT,
	PW_HIT
};

int vanim[GN_MAX] = {
	ANIM_PUNCH, ANIM_SHOOT, ANIM_SHOOT, ANIM_SHOOT, ANIM_SHOOT, ANIM_SHOOT, ANIM_PUNCH,
	ANIM_SHOOT, ANIM_SHOOT, ANIM_SHOOT
};

char *vweps[GN_MAX] = {
	"vwep/fist",
	"vwep/chaing",
	"vwep/chaing",
	"vwep/chaing",
	"vwep/chaing",
	"vwep/chaing",
	NULL,
	"vwep/chaing",
	"vwep/chaing",
	"vwep/chaing"
};

enum {
	SH_NONE = 0, SH_ELEC, SH_FIRE, SH_MAX
};

int scol[SH_MAX] = {
	COL_WHITE, COL_LBLUE, COL_ORANGE
};

int stimes[SH_MAX] = {
	0, 0, 500
};

char *shields[SH_MAX] = {
	NULL,
	"shield/ssp/elec",
	"shield/ssp/fire"
};

char *sname[SH_MAX] = {
	"none",
	"electric",
	"fire"
};

struct sspsave
{
	int 	coins, 	lives,	power,	gun,	shield,		inventory,	chkpoint,	world,	level;
} dsave = {	0, 		5,		PW_HIT,	-1,		SH_NONE,	-1,			-1,			1,		1 };
#define NUMSAVE sizeof(sspsave)/sizeof(int)

struct cament : physent
{
	vec pos;
	
	cament()
	{
		type = ENT_CAMERA;
	}
	~cament() {}
	
};

struct actent : dynent
{
	int lastaction, lastpain, lastspecial, lastent, lastenttime, direction, timeinwater;
	float clamp, path, offz;
	bool doclamp;
	char depth;

	int renderdir, renderpath;
	float renderyaw, renderpitch, renderdepth;

	actent()
	{
		depth = 0;
		direction = DR_NONE;
		doclamp = true;
		actsetup();
	}
	~actent() {}
	
	void actsetup()
	{
		lastaction = lastpain = lastspecial = lastmillis-getvar("enttime");
		lastenttime = timeinwater = 0;
		lastent = -1;
	}

	void setposition(int p, float x, float y, float z)
	{
		lastaction = lastpain = lastspecial = lastmillis-getvar("enttime");
		direction = p;

		if (isxaxis(direction))
		{
			o.x = clamp = path = x;
			o.y = y;
		}
		else
		{
			o.x = x;
			o.y = clamp = path = y;
		}
		o.z = z;

		renderyaw = yaw = diryaw[direction];
		renderpitch = pitch = 0.f;

		//if (type == ENT_PLAYER || type == ENT_AI || type == ENT_BLOCK)
		//{
			o.z += eyeheight;
			vec orig = o;
			loopi(100)
			{
				if(collide(this)) { renderdepth = (isxaxis(direction) ? o.x : o.y); return; }
				o = orig;
				o.z += i*0.5; // just use z axis, x and y position are important
			}
			o = orig;
		//}
		renderdepth = (isxaxis(direction) ? o.x : o.y);
	}

	void setdir(int p, bool q)
	{
		if (direction != p)
		{
			renderyaw = yaw;
			renderdir = lastmillis;

			if (q)
			{
				dirnorm(direction,vel);
			
				if (((direction == DR_LEFT || direction == DR_RIGHT) && (p == DR_UP || p == DR_DOWN)) ||
					((direction == DR_UP || direction == DR_DOWN) && (p == DR_LEFT || p == DR_RIGHT)))
				{
					float vx = vel.x, vy = vel.y;
					vel.x = vy; vel.y = vx;
				}
	
				dirnorm(p,vel);
			}
		}

		direction = p;
		yaw = diryaw[direction];
	}

	void setpath(float p, bool c)
	{
		if (path != p)
		{
			renderdepth = (isxaxis(direction) ? o.x : o.y);
			renderpath = lastmillis;
		}

		path = p;
		if (c) clamp = path;
	}
};

struct sspent : actent
{
	bool attacking, special, crouch;
	int idle, gunwait, invincible, hits, weight;

	sspsave sd;

	sspent()
	{
		memcpy(&sd, &dsave, sizeof(sspsave));
		attacking = false;
		weight = 100;
	}
	~sspent() {}

	void spawn(bool respawn)
	{
		dynent::reset();
		actent::actsetup();
		if (state == CS_DEAD) state = CS_ALIVE;
		special = crouch = false;
		idle = gunwait = invincible = hits = 0;
		setpower(respawn ? 1 : sd.power);
	}

	void setbounds(char *mdl)
	{
		o.z -= eyeheight;
		setbbfrommodel((dynent *)this, mdl);
		o.z += eyeheight;
	}
	
	void setinventory(int a)
	{
		if (a != sd.inventory && (a > PW_HIT || a > sd.inventory))
		{
			sd.inventory = a;
		}
	}

	int getpower()
	{
		if (sd.power > 2) return sd.power + pw[sd.gun] - 1;
		return sd.power;
	}

	void setpower(int p)
	{
		sd.power = p >= 0 && (type != ENT_PLAYER || p <= MAXPOWER) ? p : 0;

		if (type == ENT_PLAYER)
		{
			if (sd.power <= 1) sd.gun = -1;
			else if (sd.power <= 2) sd.gun = GN_FIST;
			setbounds(powers[this->getpower()]);
		}
	}
};

// projectiles
struct projent : actent
{
	sspent *owner;
	int projtype, enttype, gun, shield;
	int lifetime, offsetmillis;
	vec to, offset;
	float lastyaw;
	int attr1, attr2, attr3, attr4, attr5;

	projent() : owner(NULL), projtype(-1), enttype(-1) { aboveeye = roll = 0; }
	~projent() { }
};

// enemy
struct enemyent : sspent
{
	string enemyname, enemymodel, enemyvwep;
	int enemytype, enemyid, enemyindex, enemydir, enemyweight;
	bool enemyawake, enemyjumped;
	int enemyjump;
	vec enemypos;

	enemyent() : enemytype(0), enemyid(0), enemydir(DR_NONE) { enemyset(); }
	~enemyent() { }

	void enemyreset()
	{
		lastaction = lastpain = lastspecial = enemyjump = lastmillis-getvar("enttime");
		enemyawake = false;
		move = strafe = sd.power = gunwait = timeinwater = invincible = 0;
		state = CS_ALIVE;
	}

	void enemyset()
	{
		dynent::reset();
		enemyreset();
		state = CS_LAGGED;
		type = ENT_AI;
		sd.lives = direction = 0;
		sd.gun = -1;
		sd.shield = SH_NONE;
		enemyname[0] = enemymodel[0] = enemyvwep[0] = 0;
	}
};

// block
struct blockent : sspent
{
	string blockname;
	vec blockpos;
	int blockid, blocktype, blocktex, blockfall;
	bool blockotex, blockpush;

	uchar blockentity;
	short block1, block2, block3, block4, e1, e2;

	blockent()
	{
		dynent::reset();
		actent::actsetup();
		
		invincible = blocktype = blockentity = block1 = block2 = block3 = block4 = 0;
		blockotex = blockpush = false;
		sd.power = 1;
		
		radius = xradius = yradius = ENTBLK;
		aboveeye = 0.f;
		eyeheight = ENTBLK*2;
		pitch = yaw = 0.f;
		
		type = ENT_BLOCK;
		state = CS_ALIVE;
		lastaction = lastpain = lastspecial = lastmillis-BLKTIME;
	}
	~blockent() {}
};

// ents
enum {
	SE_COIN = ET_GAMESPECIFIC,	// amt
	SE_POWERUP,					// type, force
	SE_INVINCIBLE,				// none
	SE_LIFE,					// amt
	SE_ENEMY,					// type, id, dir
	SE_GOAL,					// trigger
	SE_JUMPPAD,					// zpush, ypush, pitch
	SE_TELEPORT,				// id, type
	SE_TELEDEST,				// id, dir
	SE_PATH,					// id
	SE_BLOCK,					// id, tex
	SE_AXIS,					// dir, persist
	SE_SCRIPT,					// id, persist
	SE_SHIELD,					// type
	SE_MAX
};

#define EXTRAENTSIZE 5

struct sspentity : extentity
{
	int link;
	//char extr1, extr2, extr3, extr4, extr5;
	sspentity() : link(-1) {}
	~sspentity() { }
};

struct sspdummyserver : igameserver
{
	void *newinfo() { return NULL; }
	void deleteinfo(void *ci) {}
	void serverinit() {}
	void clientdisconnect(int n) {}
	int clientconnect(int n, uint ip) { return DISC_NONE; }
	void localdisconnect(int n) {}
	void localconnect(int n) {}
	char *servername() { return "none"; }
	void parsepacket(int sender, int chan, bool reliable, ucharbuf &p) {}
	bool sendpackets() { return false; }
	int welcomepacket(ucharbuf &p, int n) { return -1; }
	void serverinforeply(ucharbuf &p) {}
	void serverupdate(int lastmillis, int totalmillis) {}
	bool servercompatible(char *name, char *sdec, char *map, int ping, const vector<int> &attr, int np) { return false; }
	int serverinfoport() { return 0; }
	int serverport() { return 0; }
	char *getdefaultmaster() { return "none"; }
	int getmastertype() { return 1; }
	void sendservmsg(const char *s) {}
	char *gameident() { return "ssp"; }
	char *gamepakdir() { return "ssp"; }

	char *defaultmap() { return "start"; }
	char *gamename() { return "Side Scrolling Platformer"; }
	string _gametitle;
	char *gametitle()
	{
		char *defname = defaultmap();
	
		if (strcmp(mapname, defname))
		{
			if (getvar("world") > 0)
				s_sprintf(_gametitle)("World %d - %d", getvar("world"), getvar("level"));
			else if (getvar("level") > 0)
				s_sprintf(_gametitle)("Bonus Level");
			else
				s_sprintf(_gametitle)("Hidden Level");
		}
		else s_sprintf(_gametitle)("Save Select");
	
		return _gametitle;
	}
};
