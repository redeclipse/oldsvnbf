// this file defines static map entities ("entity") and dynamic entities (players/monsters, "dynent")
// the gamecode extends these types to add game specific functionality

// ET_*: the only static entity types dictated by the engine... rest are gamecode dependent

enum { ET_EMPTY=0, ET_LIGHT, ET_MAPMODEL, ET_PLAYERSTART, ET_ENVMAP, ET_PARTICLES, ET_SOUND, ET_SPOTLIGHT, ET_GAMESPECIFIC };

struct entity                                   // persistent map entity
{
    vec o;                                      // position
    short attr1, attr2, attr3, attr4, attr5;
    uchar type;                                 // type is one of the above
    uchar reserved;
};

struct entitylight
{
    vec color, dir;
    int millis;

    entitylight() : color(1, 1, 1), dir(0, 0, 1), millis(-1) {}
};

struct extentity : entity                       // part of the entity that doesn't get saved to disk
{
    uchar spawned, inoctanode, visible;        // the only dynamic state of a map entity
    entitylight light;
	vector<int> links;

    extentity() : visible(false)
    {
    	links.setsize(0);
	}
};

extern int efocus, enthover, entorient;
#define entfocus(i, f)  { int n = efocus = (i); if(n>=0) { extentity &e = *et->getents()[n]; f; } }

enum { CS_ALIVE = 0, CS_DEAD, CS_SPAWNING, CS_LAGGED, CS_EDITING, CS_SPECTATOR };

enum { PHYS_FLOAT = 0, PHYS_FALL, PHYS_SLIDE, PHYS_SLOPE, PHYS_FLOOR, PHYS_STEP_UP, PHYS_STEP_DOWN, PHYS_BOUNCE };

enum { ENT_PLAYER = 0, ENT_AI, ENT_INANIMATE, ENT_CAMERA, ENT_BOUNCE };

enum { COLLIDE_AABB = 0, COLLIDE_ELLIPSE };

#define CROUCHHEIGHT 0.7f

struct physent                                  // base entity type, can be affected by physics
{
	vec o, vel, falling;						// origin and velocity
    vec deltapos, newpos;
    float yaw, pitch, roll;
    float aimyaw, aimpitch;
    float maxspeed, weight;                     // cubes per second, 100 for player
    int timeinair;
    float radius, height, aboveeye;             // bounding box size
    float xradius, yradius, zmargin;
    vec floor;                                  // the normal of floor the dynent is on

	int inmaterial;
    bool inliquid;
    bool jumping, crouching;
    int jumptime, crouchtime, lastimpulse;
    bool blocked, moving;                       // used by physics to signal ai
    physent *onplayer;
    int lastmove, lastmoveattempt, collisions, stacks;

    char move, strafe;

    uchar physstate;                            // one of PHYS_* above
    uchar state;                                // one of CS_* above
    uchar type;                                 // one of ENT_* above
    uchar collidetype;                          // one of COLLIDE_* above

    physent() : maxspeed(100), weight(100.f), radius(5.5f), height(15.f), aboveeye(1.f),
        xradius(5.5f), yradius(5.5f), zmargin(0),
		blocked(false), moving(true),
		onplayer(NULL), lastmove(0), lastmoveattempt(0), collisions(0), stacks(0),
		state(CS_ALIVE), type(ENT_PLAYER),
		collidetype(COLLIDE_ELLIPSE)
	{
		reset();
	}

    void resetinterp()
    {
        newpos = o;
        deltapos = vec(0, 0, 0);
    }

    void reset()
    {
    	inmaterial = 0;
    	inliquid = jumping = crouching  = false;
        jumptime = crouchtime = timeinair = lastimpulse = 0;
        strafe = move = 0;
        physstate = PHYS_FALL;
		o = vel = falling = vec(0, 0, 0);
    	yaw = pitch = roll = aimyaw = aimpitch = 0.f;
        floor = vec(0, 0, 1);
        resetinterp();
    }
};

enum
{
    ANIM_DEAD = 0, ANIM_DYING, ANIM_IDLE,
    ANIM_FORWARD, ANIM_BACKWARD, ANIM_LEFT, ANIM_RIGHT,
    ANIM_PAIN, ANIM_JUMP, ANIM_SINK, ANIM_SWIM, ANIM_MAPMODEL,
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
#define ANIM_ENVMAP      (1<<23)
#define ANIM_TRANSLUCENT (1<<24)
#define ANIM_SHADOW      (1<<25)
#define ANIM_SETTIME     (1<<26)
#define ANIM_FULLBRIGHT  (1<<27)
#define ANIM_REUSE       (1<<28)
#define ANIM_NORENDER    (1<<29)
#define ANIM_LOOKUP      (1<<30)
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

struct dynent : physent                         // animated characters, or characters that can receive input
{
    entitylight light;
    animinterpinfo animinterp[MAXANIMPARTS];
    occludequery *query;
    int occluded, lastrendered;

    dynent() : query(NULL), occluded(0), lastrendered(0)
    {
        reset();
    }

    void stopmoving()
    {
        move = strafe = 0;
    }

    void reset()
    {
        physent::reset();
        stopmoving();
    }

    vec abovehead() { return vec(o).add(vec(0, 0, aboveeye+4)); }

    void normalize_yaw(float angle)
    {
        while(yaw<angle-180.0f) yaw += 360.0f;
        while(yaw>angle+180.0f) yaw -= 360.0f;
    }
};


