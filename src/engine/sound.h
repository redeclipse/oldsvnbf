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
	bool inuse;
	
	sound() : slot(NULL), inuse(false) {}
	~sound() {}
};

extern hashtable<const char *, soundsample> soundsamples;
extern vector<soundslot> gamesounds, mapsounds;
extern vector<sound> sounds;

extern void initsound();
extern void stopsound();
extern void checksound();
extern int addsound(const char *name, int vol, vector<soundslot> &sounds);
extern void removesound(int c, bool clear = false);
extern void clearsound();
extern int playsound(int n, vec *pos = NULL, bool copy = false, bool mapsnd = false);

extern int soundvol, musicvol, soundmono, soundchans, soundbufferlen, soundfreq, maxsoundsatonce;
