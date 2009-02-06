#include "pch.h"
#include "engine.h"

hashtable<const char *, soundsample> soundsamples;
vector<soundslot> gamesounds, mapsounds;
vector<sound> sounds;

bool nosound = true, playedmusic = false;
Mix_Music *music = NULL;
char *musicfile = NULL, *musicdonecmd = NULL;
int soundsatonce = 0, lastsoundmillis = 0, musictime = -1, oldmusicvol = -1;

VARP(soundvol, 0, 255, 255);
VARF(soundmono, 0, 0, 1, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(soundchans, 0, 64, INT_MAX-1, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(soundfreq, 0, 44100, 48000, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(soundbufferlen, 0, 1024, INT_MAX-1, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));

VARP(musicvol, 0, 32, 255);
VARP(musicfade, 0, 3000, INT_MAX-1);
SVARP(thememusic, "loops/mitaman/oppforce");

void initsound()
{
	if(nosound)
	{
		if(Mix_OpenAudio(soundfreq, MIX_DEFAULT_FORMAT, soundmono ? 1 : 2, soundbufferlen) == -1)
		{
			conoutf("\frsound initialisation failed: %s", Mix_GetError());
			return;
		}

		Mix_AllocateChannels(soundchans);
		nosound = false;
	}
    initmumble();
}

void stopmusic(bool docmd)
{
	if(Mix_PlayingMusic()) Mix_HaltMusic();
	if(music)
	{
		Mix_FreeMusic(music);
		music = NULL;
	}
    DELETEA(musicfile);
	if(musicdonecmd != NULL)
	{
		char *cmd = musicdonecmd;
		musicdonecmd = NULL;
		if(docmd) execute(cmd);
		delete[] cmd;
	}
	musictime = -1;
	if(oldmusicvol >= 0) oldmusicvol = musicvol;
	playedmusic = false;
}

void musicdone(bool docmd)
{
	if(musicfade && !docmd)
	{
		musictime = lastmillis;
		if(oldmusicvol >= 0) oldmusicvol = musicvol;
		playedmusic = false;
	}
	else stopmusic(docmd);
}

void stopsound()
{
	if(nosound) return;
	Mix_HaltChannel(-1);
	nosound = true;
	stopmusic(false);
	clearsound();
	soundsamples.clear();
	gamesounds.setsizenodelete(0);
    closemumble();
	Mix_CloseAudio();
}

void removesound(int c)
{
	Mix_HaltChannel(c);
	sounds[c].reset();
}

void clearsound()
{
	loopv(sounds) removesound(i);
	mapsounds.setsizenodelete(0);
}

void playmusic(const char *name, const char *cmd)
{
	if(nosound || !musicvol) return;

	stopmusic(false);

	if(soundvol && musicvol && *name)
	{
		if(cmd[0]) musicdonecmd = newstring(cmd);
		const char *exts[] = { "", ".wav", ".ogg" };
		string buf;
		loopi(sizeof(exts)/sizeof(exts[0]))
		{
			s_sprintf(buf)("sounds/%s%s", name, exts[i]);
			const char *file = findfile(buf, "rb");
			if((music = Mix_LoadMUS(file)))
			{
				musicfile = newstring(name);
				Mix_PlayMusic(music, cmd[0] ? 0 : -1);
				Mix_VolumeMusic((musicvol*MIX_MAX_VOLUME)/255);
				oldmusicvol = musicvol;
			}
		}
		if(!music) { conoutf("\frcould not play music: %s", name); return; }
	}
}

COMMANDN(music, playmusic, "ss");

void smartmusic(bool cond, bool autooff)
{
	if(nosound || !musicvol || (!cond && oldmusicvol < 0) || !*thememusic) return;
	if(!music || !Mix_PlayingMusic() || (cond && strcmp(musicfile, thememusic)))
	{
		playmusic(thememusic, "");
		playedmusic = autooff;
		if(!cond) oldmusicvol = -1;
	}
}
ICOMMAND(smartmusic, "ii", (int *a, int *b), smartmusic(*a, *b));

int findsound(const char *name, int vol, vector<soundslot> &sounds)
{
	loopv(sounds)
		if(!strcmp(sounds[i].sample->name, name) && (!vol || sounds[i].vol == vol)) return i;
	return -1;
}

int addsound(const char *name, int vol, int material, int maxrad, int minrad, bool unique, vector<soundslot> &sounds)
{
	soundsample *sample = soundsamples.access(name);
	if(!sample)
	{
		char *n = newstring(name);
		sample = &soundsamples[n];
		sample->name = n;
		sample->sound = NULL;
	}
	if(!sample->sound)
	{
		const char *exts[] = { "", ".wav", ".ogg" };
		string buf;
		loopi(sizeof(exts)/sizeof(exts[0]))
		{
			s_sprintf(buf)("sounds/%s%s", sample->name, exts[i]);
			const char *file = findfile(buf, "rb");
			if((sample->sound = Mix_LoadWAV(file)) != NULL) break;
		}

		if(!sample->sound) { conoutf("\frfailed to load sample: %s", sample->name); return -1; }
	}
    if(vol < 0 || vol > 255) vol = 255;
    if(maxrad <= 0) maxrad = -1;
    if(minrad < 0) minrad = -1;
    if(unique)
    {
        loopv(sounds)
        {
            soundslot &slot = sounds[i];
            if(slot.sample == sample && slot.vol == vol && slot.material == material && slot.maxrad == maxrad && slot.minrad == minrad) return i;
        }
    }
	soundslot &slot = sounds.add();
	slot.sample = sample;
	slot.vol = vol;
	slot.material = material;
	slot.maxrad = maxrad; // use these values if none are supplied when playing
	slot.minrad = minrad;
	return sounds.length()-1;
}

ICOMMAND(registersound, "sisssi", (char *n, int *v, char *m, char *w, char *x, int *u), intret(addsound(n, *v, *m ? findmaterial(m, true) : MAT_AIR, *w ? atoi(w) : -1, *x ? atoi(x) : -1, *u > 0, gamesounds)));
ICOMMAND(mapsound, "sisssi", (char *n, int *v, char *m, char *w, char *x, int *u), intret(addsound(n, *v, *m ? findmaterial(m, true) : MAT_AIR, *w ? atoi(w) : -1, *x ? atoi(x) : -1, *u > 0, mapsounds)));

void calcvol(int flags, int vol, int slotvol, int slotmat, int maxrad, int minrad, const vec &pos, int *curvol, int *curpan)
{
	bool posliquid = isliquid(lookupmaterial(pos)&MATF_VOLUME),
			camliquid = isliquid(lookupmaterial(camera1->o)&MATF_VOLUME);
	int svol = clamp(((soundvol*vol*slotvol*MIX_MAX_VOLUME)/255/255/255), 0, MIX_MAX_VOLUME);

	vec v;
	float dist = camera1->o.dist(pos, v);

	if(!(flags&SND_NOATTEN))
	{
		if(!soundmono && (v.x != 0 || v.y != 0))
		{
			float yaw = -atan2f(v.x, v.y) - camera1->yaw*RAD; // relative angle of sound along X-Y axis
			*curpan = int(255.9f*(0.5f*sinf(yaw)+0.5f)); // range is from 0 (left) to 255 (right)
		}
		else *curpan = 127;

		if(camliquid && slotmat == MAT_AIR) svol = int(svol*0.1f);
		else if(posliquid || camliquid) svol = int(svol*0.25f);

		float mrad = float(maxrad > 0 ? maxrad : 256),
			nrad = float(minrad < mrad ? minrad : mrad/2.f);

		if(dist <= nrad) *curvol = svol;
		else if(dist <= mrad) *curvol = int(float(svol)*((mrad-nrad-dist)/(mrad-nrad)));
		else *curvol = 0;
	}
	else
	{
		*curvol = svol;
		*curpan = 127;
	}
}

void updatesound(int chan)
{
	sound &s = sounds[chan];
	bool waiting = (!(s.flags&SND_NODELAY) && Mix_Paused(chan));
	if((s.flags&SND_NOCULL) || s.curvol > 0)
	{
		if(waiting)
		{ // delay the sound based on average physical constants
			bool liquid = (
				isliquid(lookupmaterial(s.pos)&MATF_VOLUME) || isliquid(lookupmaterial(camera1->o)&MATF_VOLUME)
			);
			float dist = camera1->o.dist(s.pos);
			int delay = int((dist/8.f)*(liquid ? 1.497f : 0.343f));
			if(lastmillis >= s.millis + delay)
			{
				Mix_Resume(chan);
				waiting = false;
			}
		}
		if(!waiting)
		{
			Mix_Volume(chan, s.curvol);
            SDL_LockAudio(); // workaround for race condition in inside Mix_SetPanning
			Mix_SetPanning(chan, 255-s.curpan, s.curpan);
            SDL_UnlockAudio();
		}
	}
	else
	{
		removesound(chan);
		if(verbose >= 4) conoutf("culled sound %d (%d)", chan, s.curvol);
	}
}

void updatesounds()
{
    updatemumble();
	if(nosound) return;

	loopv(sounds) if(sounds[i].chan >= 0)
	{
		sound &s = sounds[i];
		if((!s.ends || lastmillis < s.ends) && Mix_Playing(i))
		{
			if(s.owner) s.pos = s.owner->o;
			calcvol(
				s.flags, s.vol, s.slot->vol, s.slot->material, s.maxrad, s.minrad, s.pos,
				&s.curvol, &s.curpan
			);
			updatesound(i);
		}
		else removesound(i);
	}
	if(music || Mix_PlayingMusic())
	{
		if(nosound || !musicvol) stopmusic(false);
		else if(playedmusic && oldmusicvol >= 0) musicdone(false);
		else if(!Mix_PlayingMusic()) musicdone(true);
		else if(musictime >= 0)
		{
			if(!musicfade || lastmillis-musictime >= musicfade) stopmusic(false);
			else
			{
				float fade = clamp(float(lastmillis-musictime)/float(musicfade), 0.f, 1.f);
				int vol = int(musicvol*(1.f-fade));
				if(vol != oldmusicvol)
				{
					Mix_VolumeMusic((vol*MIX_MAX_VOLUME)/255);
					oldmusicvol = vol;
				}
			}
		}
		else if(musicvol != oldmusicvol)
		{
			Mix_VolumeMusic((musicvol*MIX_MAX_VOLUME)/255);
			oldmusicvol = musicvol;
		}
	}
}

int playsound(int n, const vec &pos, physent *d, int flags, int vol, int maxrad, int minrad, int *hook, int ends)
{
	if(nosound || !soundvol) return -1;

	vector<soundslot> &soundset = flags&SND_MAP ? mapsounds : gamesounds;

	if(soundset.inrange(n) && soundset[n].sample->sound)
	{
		soundslot *slot = &soundset[n];
		int cvol = 0, cpan = 0, v = vol > 0 && vol < 256 ? vol : 255, x = maxrad, y = minrad;

		if(x <= 0)
		{
			if(slot->maxrad > 0) x = slot->maxrad;
			else x = 256;
		}
		if(y < 0)
		{
			if(slot->minrad >= 0) y = slot->minrad;
			else y = 2;
		}

		calcvol(flags, v, slot->vol, slot->material, x, y, pos, &cvol, &cpan);

		if((flags&SND_NOCULL) || cvol > 0)
		{
			int chan = Mix_PlayChannel(-1, soundset[n].sample->sound, flags&SND_LOOP ? -1 : 0);
			if(chan < 0)
			{
				int lowest = -1;
				loopv(sounds) if(sounds[i].chan >= 0 && !(sounds[i].flags&SND_NOCULL) && !(sounds[i].flags&SND_MAP))
					if((flags&SND_NOCULL) || sounds[i].vol < cvol)
						if(!sounds.inrange(lowest) || sounds[i].vol < sounds[lowest].vol)
							lowest = i;

				if(sounds.inrange(lowest))
				{
					removesound(lowest);
					chan = Mix_PlayChannel(-1, soundset[n].sample->sound, flags&SND_LOOP ? -1 : 0);
					if(verbose >= 4) conoutf("culled channel %d (%d)", lowest, sounds[lowest].vol);
				}
			}
			if(chan >= 0)
			{
				if(!(flags&SND_NODELAY)) Mix_Pause(chan);

				while(chan >= sounds.length()) sounds.add();

				sound &s = sounds[chan];
				s.slot = slot;
				s.vol = v;
				s.maxrad = x;
				s.minrad = y;
				s.flags = flags;
				s.millis = lastmillis;
				s.ends = ends;
				s.slotnum = n;
				s.owner = d;
				s.pos = pos;
				s.curvol = cvol;
				s.curpan = cpan;
				s.chan = chan;
				if(hook)
				{
					*hook = s.chan;
					s.hook = hook;
				}
				else s.hook = NULL;
				updatesound(chan);
				return chan;
			}
			else if(verbose >= 2)
				conoutf("\frcannot play sound %d (%s): %s", n, soundset[n].sample->name, Mix_GetError());
		}
		else if(verbose >= 4) conoutf("culled sound %d (%d)", n, cvol);
	}
	else if(n > 0) conoutf("\frunregistered sound: %d", n);
	return -1;
}

void sound(int *n, int *vol)
{
	intret(playsound(*n, camera1->o, camera1, SND_FORCED, *vol ? *vol : -1));
}
COMMAND(sound, "ii");

void removetrackedsounds(physent *d)
{
	loopv(sounds)
		if(sounds[i].chan >= 0 && sounds[i].owner == d)
			removesound(i);
}

void resetsound()
{
    const SDL_version *v = Mix_Linked_Version();
    if(SDL_VERSIONNUM(v->major, v->minor, v->patch) <= SDL_VERSIONNUM(1, 2, 8))
    {
        conoutf("\frsound reset not available in-game due to SDL_mixer-1.2.8 bug, please restart for changes to take effect.");
        return;
    }
    clearchanges(CHANGE_SOUND);
    if(!nosound)
    {
        loopv(sounds) removesound(i);
        enumerate(soundsamples, soundsample, s, { Mix_FreeChunk(s.sound); s.sound = NULL; });
        if(music)
        {
            Mix_HaltMusic();
            Mix_FreeMusic(music);
        }
        Mix_CloseAudio();
    }
    initsound();
    if(nosound)
    {
        DELETEA(musicfile);
        DELETEA(musicdonecmd);
        music = NULL;
        gamesounds.setsizenodelete(0);
        mapsounds.setsizenodelete(0);
        soundsamples.clear();
        return;
    }
    if(music && (music = Mix_LoadMUS(musicfile)))
    {
        Mix_PlayMusic(music, musicdonecmd[0] ? 0 : -1);
        Mix_VolumeMusic((musicvol*MIX_MAX_VOLUME)/255);
    }
}

COMMAND(resetsound, "");

#ifdef WIN32

#include <wchar.h>

#else

#include <unistd.h>

#ifdef _POSIX_SHARED_MEMORY_OBJECTS
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <wchar.h>
#endif

#endif

#if defined(WIN32) || defined(_POSIX_SHARED_MEMORY_OBJECTS)
struct MumbleInfo
{
    int version, timestamp;
    vec pos, front, top;
    wchar_t name[256];
};
#endif

#ifdef WIN32
static HANDLE mumblelink = NULL;
static MumbleInfo *mumbleinfo = NULL;
#define VALID_MUMBLELINK (mumblelink && mumbleinfo)
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
static int mumblelink = -1;
static MumbleInfo *mumbleinfo = (MumbleInfo *)-1;
#define VALID_MUMBLELINK (mumblelink >= 0 && mumbleinfo != (MumbleInfo *)-1)
#endif

#ifdef VALID_MUMBLELINK
VARFP(mumble, 0, 1, 1, { if(mumble) initmumble(); else closemumble(); });
#else
VARFP(mumble, 0, 0, 1, { if(mumble) initmumble(); else closemumble(); });
#endif

void initmumble()
{
    if(!mumble) return;
#ifdef VALID_MUMBLELINK
    if(VALID_MUMBLELINK) return;

    #ifdef WIN32
        mumblelink = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, "MumbleLink");
        if(mumblelink)
        {
            mumbleinfo = (MumbleInfo *)MapViewOfFile(mumblelink, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(MumbleInfo));
            if(mumbleinfo) wcsncpy(mumbleinfo->name, L"BloodFrontier", 256);
        }
    #elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
        s_sprintfd(shmname)("/MumbleLink.%d", getuid());
        mumblelink = shm_open(shmname, O_RDWR, 0);
        if(mumblelink >= 0)
        {
            mumbleinfo = (MumbleInfo *)mmap(NULL, sizeof(MumbleInfo), PROT_READ|PROT_WRITE, MAP_SHARED, mumblelink, 0);
            if(mumbleinfo != (MumbleInfo *)-1) wcsncpy(mumbleinfo->name, L"BloodFrontier", 256);
        }
    #endif
    if(!VALID_MUMBLELINK) closemumble();
#else
    conoutf(CON_ERROR, "Mumble positional audio is not available on this platform.");
#endif
}

void closemumble()
{
#ifdef WIN32
    if(mumbleinfo) { UnmapViewOfFile(mumbleinfo); mumbleinfo = NULL; }
    if(mumblelink) { CloseHandle(mumblelink); mumblelink = NULL; }
#elif defined(_POSIX_SHARED_MEMORY_OBJECTS)
    if(mumbleinfo != (MumbleInfo *)-1) { munmap(mumbleinfo, sizeof(MumbleInfo)); mumbleinfo = (MumbleInfo *)-1; }
    if(mumblelink >= 0) { close(mumblelink); mumblelink = -1; }
#endif
}

static inline vec mumblevec(const vec &v, bool pos = false)
{
    // change from Z up, -Y forward to Y up, +Z forward
    // 8 cube units = 1 meter
    vec m(v.x, v.z, -v.y);
    if(pos) m.div(8);
    return m;
}

void updatemumble()
{
#ifdef VALID_MUMBLELINK
    if(!VALID_MUMBLELINK) return;

    static int timestamp = 0;

    mumbleinfo->version = 1;
    mumbleinfo->timestamp = ++timestamp;

    mumbleinfo->pos = mumblevec(camera1->o, true);
    mumbleinfo->front = mumblevec(vec(RAD*camera1->yaw, RAD*camera1->pitch));
    mumbleinfo->top = mumblevec(vec(RAD*camera1->yaw, RAD*(camera1->pitch+90)));
#endif
}

