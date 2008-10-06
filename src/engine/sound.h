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
	int vol, material, maxrad, minrad;

	soundslot() : sample(NULL), vol(255), material(MAT_AIR), maxrad(-1), minrad(-1) {}
	~soundslot() {}
};


struct sound
{
	soundslot *slot;
	vec pos;
	physent *owner;
	int vol, curvol, curpan;
	int flags, maxrad, minrad;
	int millis, ends, slotnum, chan, *hook;

	sound() : hook(NULL) { reset(); }
	~sound() {}

	void reset()
	{
		slot = NULL;
		owner = NULL;
		vol = curvol = 255;
		curpan = 127;
		flags = maxrad = minrad = millis = ends = slotnum = 0;
		chan = -1;
		if(hook) *hook = -1;
		hook = NULL;
	}
};

#define SND_LOOP	0x0001
#define SND_MAP		0x0002

#define SND_NOATTEN	0x0010	// disable attenuation
#define SND_NODELAY	0x0020	// disable delay
#define SND_NOCULL	0x0040	// disable culling

extern hashtable<const char *, soundsample> soundsamples;
extern vector<soundslot> gamesounds, mapsounds;
extern vector<sound> sounds;

#define issound(c) (sounds.inrange(c) && sounds[c].chan >= 0 && Mix_Playing(c))

extern void initsound();
extern void stopsound();
extern void updatesounds();
extern int addsound(const char *name, int vol, int material, int maxrad, int minrad, vector<soundslot> &sounds);
extern void removesound(int c);
extern void clearsound();
extern int playsound(int n, int flags, int vol, vec &pos, physent *d = NULL, int *hook = NULL, int ends = 0, int maxrad = -1, int minrad = -1);
extern void removetrackedsounds(physent *d);

extern void initmumble();
extern void closemumble();
extern void updatemumble();

extern int soundvol, musicvol, soundmono, soundchans, soundbufferlen, soundfreq, maxsoundsatonce;
