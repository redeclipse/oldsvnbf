// this file defines static map entities ("entity") and dynamic entities (players/monsters, "dynent")
// the gamecode extends these types to add game specific functionality

// ET_*: the only static entity types dictated by the engine... rest are gamecode dependent

enum { ET_EMPTY=0, ET_LIGHT, ET_MAPMODEL, ET_PLAYERSTART, ET_ENVMAP, ET_PARTICLES, ET_SOUND, ET_LIGHTFX, ET_SUNLIGHT, ET_GAMESPECIFIC };
enum { LFX_SPOTLIGHT = 0, LFX_DYNLIGHT, LFX_FLICKER, LFX_PULSE, LFX_GLOW, LFX_MAX };
enum { LFX_S_NONE = 0, LFX_S_RAND1 = 1<<0, LFX_S_RAND2 = 1<<1, LFX_S_MAX = 2 };

struct entbase                                   // persistent map entity
{
    vec o;                                      // position
    uchar type;                                 // type is one of the above
};

struct entitylight
{
    vec color, dir;
    int millis;

    entitylight() : color(1, 1, 1), dir(0, 0, 1), millis(-1) {}
};

enum
{
	MMT_NONE = 0,
	MMT_HIDE = 1<<0, // hide when triggered (trigger)
	MMT_NOCLIP = 1<<1, // always not collide
	MMT_NOSHADOW = 1<<2, // doesn't cast a lightmap shadow
	MMT_NODYNSHADOW = 1<<3, // doesn't cast a shadow map (trigger)
};

struct entity : entbase
{
	vector<int> attrs, links;

    entity() { reset(); }

    void reset()
    {
		attrs.setsize(0);
		links.setsize(0);
    }
};

struct extentity : entity                       // part of the entity that doesn't get saved to disk
{
    uchar spawned, inoctanode, visible;        // the only dynamic state of a map entity
    entitylight light;
	int lastemit, emit[3];

    extentity() { reset(); }

    void reset()
    {
    	entity::reset();
		spawned = inoctanode = visible = false;
		lastemit = emit[0] = emit[1] = emit[2] = 0;
    }
};

extern int efocus, enthover, entorient;
#define entfocus(i, f)  { int n = efocus = (i); if(n>=0) { extentity &e = *entities::getents()[n]; f; } }

vec &lightposition(const extentity &light, const vec &target = vec(-1, -1, -1));
#define lightcolour(n,m) n.attrs[1+m+(n.type != ET_SUNLIGHT ? 0 : 1)]

enum { CS_ALIVE = 0, CS_DEAD, CS_SPAWNING, CS_EDITING, CS_SPECTATOR, CS_WAITING };

enum { PHYS_FLOAT = 0, PHYS_FALL, PHYS_SLIDE, PHYS_SLOPE, PHYS_FLOOR, PHYS_STEP_UP, PHYS_STEP_DOWN, PHYS_BOUNCE };

enum { ENT_PLAYER = 0, ENT_AI, ENT_INANIMATE, ENT_PROJ, ENT_RAGDOLL, ENT_DUMMY, ENT_CAMERA };

enum { COLLIDE_AABB = 0, COLLIDE_ELLIPSE };

#define CROUCHHEIGHT 0.7f
#define CROUCHTIME 200

struct physent                                  // base entity type, can be affected by physics
{
	vec o, vel, falling;						// origin and velocity
    vec deltapos, newpos;
    float yaw, pitch, roll;
    float aimyaw, aimpitch;
    float maxspeed, weight;                     // cubes per second, 100 for player
    int timeinair;
    float radius, height, aboveeye;             // bounding box size
    float xradius, yradius, zradius, zmargin;
    vec floor;                                  // the normal of floor the dynent is on

	int inmaterial;
    bool inliquid, onladder;
    float submerged;

    bool jumping, crouching, impulsing;
    int jumptime, crouchtime, impulsetime, impulsemillis, impulsedash;

    char move, strafe;

    uchar physstate;                            // one of PHYS_* above
    uchar state;                                // one of CS_* above
    uchar type;                                 // one of ENT_* above
    uchar collidetype;                          // one of COLLIDE_* above

    physent() : maxspeed(100), weight(100.f), radius(3.f), height(14.f), aboveeye(1.f),
        xradius(3.f), yradius(3.f), zradius(14.f), zmargin(0),
		state(CS_ALIVE), type(ENT_PLAYER),
		collidetype(COLLIDE_ELLIPSE)
	{
		reset();
	}

    void resetinterp()
    {
        newpos = o;
        newpos.z -= height;
        deltapos = vec(0, 0, 0);
    }

    void reset()
    {
    	inmaterial = timeinair = jumptime = crouchtime = impulsetime = impulsemillis = impulsedash = 0;
    	inliquid = onladder = jumping = crouching = impulsing = false;
        strafe = move = 0;
        physstate = PHYS_FALL;
        o = vel = falling = vec(0, 0, 0);
    	yaw = pitch = roll = aimyaw = aimpitch = 0.f;
        floor = vec(0, 0, 1);
        resetinterp();
    }

    vec abovehead(float offset = 1.f) const { return vec(o).add(vec(0, 0, aboveeye+offset)); }
    vec feetpos(float offset = 0) const { return vec(o).add(vec(0, 0, offset-height)); }
    vec headpos(float offset = 0) const { return vec(o).add(vec(0, 0, offset)); }
};

enum
{
    ANIM_IDLE = 0, ANIM_FORWARD, ANIM_BACKWARD, ANIM_LEFT, ANIM_RIGHT, ANIM_DEAD, ANIM_DYING, ANIM_SWIM,
    ANIM_MAPMODEL, ANIM_TRIGGER_ON, ANIM_TRIGGER_OFF,
    ANIM_GAMESPECIFIC
};

#define ANIM_ALL         0x7F
#define ANIM_INDEX       0x7F
#define ANIM_LOOP        (1<<7)
#define ANIM_START       (1<<8)
#define ANIM_END         (1<<9)
#define ANIM_REVERSE     (1<<10)
#define ANIM_DIR         0x780
#define ANIM_SECONDARY   11
#define ANIM_NOSKIN      (1<<22)
#define ANIM_SETTIME     (1<<23)
#define ANIM_FULLBRIGHT  (1<<24)
#define ANIM_REUSE       (1<<25)
#define ANIM_NORENDER    (1<<26)
#define ANIM_RAGDOLL     (1<<27)
#define ANIM_SETSPEED    (1<<28)
#define ANIM_FLAGS       (0x1FF<<22)

struct animinfo // description of a character's animation
{
    int anim, frame, range, basetime;
    float speed;
    uint varseed;

    animinfo() : anim(0), frame(0), range(0), basetime(0), speed(100.0f), varseed(0) { }

    bool operator==(const animinfo &o) const { return frame==o.frame && range==o.range && (anim&(ANIM_SETTIME|ANIM_DIR))==(o.anim&(ANIM_SETTIME|ANIM_DIR)) && (anim&ANIM_SETTIME || basetime==o.basetime) && speed==o.speed; }
    bool operator!=(const animinfo &o) const { return frame!=o.frame || range!=o.range || (anim&(ANIM_SETTIME|ANIM_DIR))!=(o.anim&(ANIM_SETTIME|ANIM_DIR)) || (!(anim&ANIM_SETTIME) && basetime!=o.basetime) || speed!=o.speed; }
};

struct animinterpinfo // used for animation blending of animated characters
{
    animinfo prev, cur;
    int lastswitch;
    void *lastmodel;

    animinterpinfo() : lastswitch(-1), lastmodel(NULL) {}
};

#define MAXANIMPARTS 2

struct occludequery;
struct ragdolldata;

struct dynent : physent                         // animated characters, or characters that can receive input
{
    entitylight light;
    animinterpinfo animinterp[MAXANIMPARTS];
    ragdolldata *ragdoll;
    occludequery *query;
    int occluded, lastrendered;

    dynent() : ragdoll(NULL), query(NULL), occluded(0), lastrendered(0)
    {
        reset();
    }

    ~dynent()
    {
#ifndef STANDALONE
        extern void cleanragdoll(dynent *d);
        if(ragdoll) cleanragdoll(this);
#endif
    }

    void normalize_yaw(float angle)
    {
        while(yaw<angle-180.0f) yaw += 360.0f;
        while(yaw>angle+180.0f) yaw -= 360.0f;
    }
};


