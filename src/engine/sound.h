#include "SDL_mixer.h"

extern bool nosound;
extern int soundvol;

#define SOUNDMINDIST		16.0f
#define SOUNDMAXDIST		10000.f

struct soundsample
{
	Mix_Chunk *sound;
	char *name;

	soundsample() : name(NULL) {}
	~soundsample() { DELETEA(name); }
};

struct soundslot
{
	soundsample *sample;
	int vol, material;

	soundslot() : sample(NULL), vol(0), material(-1) {}
	~soundslot() {}
};


struct sound
{
	soundslot *slot;
	vec pos;
	physent *owner;
	int vol, curvol, curpan;
	int flags, maxrad, minrad;
	float dist;
	bool inuse, map;
	int millis;

	sound() : slot(NULL), owner(NULL),
		vol(255), curvol(255), curpan(127), flags(0),
			maxrad(0), minrad(0), dist(0.f), inuse(false), millis(0) {}
	~sound() {}
};

#define SND_LOOP	0x0001
#define SND_MAP		0x0002
#define SND_NOATTEN	0x0004	// disable attenuation
#define SND_NODELAY	0x0008	// disable delay

extern hashtable<const char *, soundsample> soundsamples;
extern vector<soundslot> gamesounds, mapsounds;
extern vector<sound> sounds;

#define issound(c) (sounds.inrange(c) && sounds[c].inuse)

extern void initsound();
extern void stopsound();
extern void updatesounds();
extern int addsound(const char *name, int vol, int material, vector<soundslot> &sounds);
extern void removesound(int c);
extern void clearsound();
extern int playsound(int n, int flags, int vol, vec &pos, physent *d = NULL, int maxrad = 1024, int minrad = 2);
extern void removetrackedsounds(physent *d);

extern void initmumble();
extern void closemumble();
extern void updatemumble();

extern int soundvol, musicvol, soundmono, soundchans, soundbufferlen, soundfreq, maxsoundsatonce;
