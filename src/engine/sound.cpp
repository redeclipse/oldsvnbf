// sound.cpp: uses fmod on windows and sdl_mixer on unix (both had problems on the other platform)

#include "pch.h"
#include "engine.h"

hashtable<const char *, soundsample> sndsamples;
vector<soundslot> gamesounds, mapsounds;
vector<soundchan> sndchans;

bool nosound = true;

VARP(soundvol, 0, 255, 255);
VARF(soundchans, 0, 1024, 4093, initwarning());
VARF(soundfreq, 0, 44100, 48000, initwarning());

void initsound()
{
	return;
}

int findsound(const char *name, int vol, vector<soundslot> &sounds)
{
	loopv(sounds)
	{
		if(!strcmp(sounds[i].sample->name, name) && (!vol || sounds[i].vol==vol)) return i;
	}
	return -1;
}

int addsound(const char *name, int vol, int maxuses, vector<soundslot> &sounds)
{
	soundsample *s = sndsamples.access(name);
	if(!s)
	{
		char *n = newstring(name);
		s = &sndsamples[n];
		s->name = n;
		s->sound = NULL;
	}
	soundslot &slot = sounds.add();
	slot.sample = s;
	slot.vol = vol ? vol : 255;
	slot.maxuses = maxuses;
	return sounds.length()-1;
}

ICOMMAND(registersound, "sii", (char *n, int *v, int *m), intret(addsound(n, *v, *m < 0 ? -1 : *m, gamesounds)));
ICOMMAND(mapsound, "sii", (char *n, int *v, int *m), intret(addsound(n, *v, *m < 0 ? -1 : *m, mapsounds)));

void checksound()
{
	if(nosound) return;
}

int playsound(int n, vec *p, vec *v, float mindist, float maxdist, vector<soundslot> &sounds)
{
	return -1;
}

int playsoundv(int n, vec &p, vec &v, float mindist, float maxdist, vector<soundslot> &sounds)
{
	return -1;
}

ICOMMAND(sound, "i", (int *i), playsound(*i));

void clearmapsounds()
{
	loopv(sndchans)
	{
		soundchan &s = sndchans[i];
		if (s.playing()) s.stop();
	}
	mapsounds.setsizenodelete(0);
}

void clear_sound()
{
	if(nosound) return;

	gamesounds.setsizenodelete(0);
	clearmapsounds();
	sndsamples.clear();
	nosound = true;
}
