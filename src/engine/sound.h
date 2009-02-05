enum
{
	S_GUIPRESS = 0, S_GUIBACK, S_GUIACT,
	S_GAMESPECIFIC
};

enum
{
	SND_NONE	= 0,
	SND_NOATTEN	= 1<<0,	// disable attenuation
	SND_NODELAY	= 1<<1,	// disable delay
	SND_NOCULL	= 1<<2,	// disable culling
	SND_LOOP	= 1<<3,
	SND_MAP		= 1<<4,
	SND_FORCED	= SND_NOATTEN|SND_NODELAY|SND_NOCULL,
	SND_MASKF	= SND_LOOP|SND_MAP
};

#ifndef STANDALONE
#include "SDL_mixer.h"
extern bool nosound;
extern int soundvol;
extern Mix_Music *music;
extern char *musicfile, *musicdonecmd;
extern int soundsatonce, lastsoundmillis;

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

extern hashtable<const char *, soundsample> soundsamples;
extern vector<soundslot> gamesounds, mapsounds;
extern vector<sound> sounds;

#define issound(c) (sounds.inrange(c) && sounds[c].chan >= 0 && Mix_Playing(c))

extern void initsound();
extern void stopsound();
extern void playmusic(const char *name, const char *cmd);
extern void musicdone(bool docmd);
extern void updatesounds();
extern int addsound(const char *name, int vol, int material, int maxrad, int minrad, vector<soundslot> &sounds);
extern void removesound(int c);
extern void clearsound();
extern int playsound(int n, const vec &pos, physent *d = NULL, int flags = 0, int vol = -1, int maxrad = -1, int minrad = -1, int *hook = NULL, int ends = 0);
extern void removetrackedsounds(physent *d);

extern void initmumble();
extern void closemumble();
extern void updatemumble();

extern int soundvol, musicvol, soundmono, soundchans, soundbufferlen, soundfreq, maxsoundsatonce;
#endif
