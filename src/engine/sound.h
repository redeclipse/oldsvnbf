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
	int vol;

	soundslot() : sample(NULL), vol(0) {}
	~soundslot() {}
};


struct sound
{
	soundslot *slot;
	vec *pos, posval;
	int vol, curvol, curpan;
	int flags, maxrad, minrad;
	float dist;
	bool inuse, map;

	sound() : slot(NULL), vol(255), curvol(255), curpan(127), flags(0), maxrad(0), minrad(0), dist(0.f), inuse(false) {}
	~sound() {}
};

#define SND_COPY	0x0001
#define SND_LOOP	0x0002
#define SND_MAP		0x0004

extern hashtable<const char *, soundsample> soundsamples;
extern vector<soundslot> gamesounds, mapsounds;
extern vector<sound> sounds;

extern void initsound();
extern void stopsound();
extern void checksound();
extern int addsound(const char *name, int vol, vector<soundslot> &sounds);
extern void removesound(int c);
extern void clearsound();
extern int playsound(int n, vec *pos = NULL, int vol = 255, int maxrad = 0, int minrad = 0, int flags = 0);

extern int soundvol, musicvol, soundmono, soundchans, soundbufferlen, soundfreq, maxsoundsatonce;
