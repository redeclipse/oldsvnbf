#include "pch.h"
#include "engine.h"

hashtable<const char *, soundsample> soundsamples;
vector<soundslot> gamesounds, mapsounds;
vector<sound> sounds;

bool nosound = true;
Mix_Music *music = NULL;
char *musicfile = NULL, *musicdonecmd = NULL;
int soundsatonce = 0, lastsoundmillis = 0;

void setmusicvol(int musicvol)
{
	if(!nosound)
	{
		if(music) Mix_VolumeMusic((musicvol*MIX_MAX_VOLUME)/255);
	}
}


VARP(soundvol, 0, 255, 255);
VARFP(musicvol, 0, 255, 255, setmusicvol(musicvol));
VARF(soundmono, 0, 0, 1, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(soundchans, 0, 1024, INT_MAX-1, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(soundfreq, 0, 44100, 48000, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARF(soundbufferlen, 0, 1024, INT_MAX-1, initwarning("sound configuration", INIT_RESET, CHANGE_SOUND));
VARP(soundmaxatonce, 0, 64, INT_MAX-1);
VARP(soundmaxdist, 0, 4096, INT_MAX-1);

void initsound()
{
	if(nosound)
	{
		if(Mix_OpenAudio(soundfreq, MIX_DEFAULT_FORMAT, soundmono ? 1 : 2, soundbufferlen) == -1)
		{
			conoutf("sound initialisation failed: %s", Mix_GetError());
			return;
		}

		Mix_AllocateChannels(soundchans);
		nosound = false;
	}
    initmumble();
}

void musicdone(bool docmd)
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
}

void stopsound()
{
	if(nosound) return;
	nosound = true;
	musicdone(false);
	clearsound();
	soundsamples.clear();
	gamesounds.setsizenodelete(0);
    closemumble();
	Mix_CloseAudio();
}

void removesound(int c)
{
	if(Mix_Playing(c)) Mix_HaltChannel(c);
	if(sounds[c].inuse) sounds[c].inuse = false;
}

void clearsound()
{
	loopv(sounds) removesound(i);
	mapsounds.setsizenodelete(0);
}

void playmusic(char *name, char *cmd)
{
	if(nosound || !musicvol)

	musicdone(false);

	if(soundvol && musicvol && *name)
	{
		if(cmd[0]) musicdonecmd = newstring(cmd);
		s_sprintfd(sn)("%s", name);
		const char *file = findfile(sn, "rb");

		if((music = Mix_LoadMUS(file)))
		{
            musicfile = newstring(file);
			Mix_PlayMusic(music, cmd[0] ? 0 : -1);
			Mix_VolumeMusic((musicvol*MIX_MAX_VOLUME)/255);
		}
		else
		{
			conoutf("could not play music: %s", sn);
		}
	}
}

COMMANDN(music, playmusic, "ss");

int findsound(const char *name, int vol, vector<soundslot> &sounds)
{
	loopv(sounds)
	{
		if(!strcmp(sounds[i].sample->name, name) && (!vol || sounds[i].vol == vol)) return i;
	}
	return -1;
}

int addsound(const char *name, int vol, int material, vector<soundslot> &sounds)
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

		if(!sample->sound) { conoutf("failed to load sample: %s", sample->name); return -1; }
	}
	soundslot &slot = sounds.add();
	slot.sample = sample;
	slot.vol = vol >= 0 ? min(vol, 255) : 255;
	switch(material)
	{
		case MAT_AIR:
		case MAT_CLIP:
		case MAT_NOCLIP:
		case MAT_WATER:
		case MAT_LAVA:
			slot.material = material;
			break;
		default:
			slot.material = MAT_NOCLIP;
			break;
	}
	return sounds.length()-1;
}

ICOMMAND(registersound, "sis", (char *n, int *v, char *m), intret(addsound(n, *v, *m ? findmaterial(m, true) : MAT_NOCLIP, gamesounds)));
ICOMMAND(mapsound, "sis", (char *n, int *v, char *m), intret(addsound(n, *v, *m ? findmaterial(m, true) : MAT_NOCLIP, mapsounds)));

void updatesound(int chan)
{
	bool posliquid = isliquid(lookupmaterial(*sounds[chan].pos)&MATF_VOLUME),
			camliquid = isliquid(lookupmaterial(camera1->o)&MATF_VOLUME);
	int vol = clamp(((soundvol*sounds[chan].vol*sounds[chan].slot->vol*MIX_MAX_VOLUME)/255/255/255), 0, MIX_MAX_VOLUME);

	vec v;
	sounds[chan].dist = camera1->o.dist(*sounds[chan].pos, v);

	if(!(sounds[chan].flags&SND_NOATTEN))
	{
		if(!soundmono && (v.x != 0 || v.y != 0))
		{
			float yaw = -atan2f(v.x, v.y) - camera1->yaw*RAD; // relative angle of sound along X-Y axis
			sounds[chan].curpan = int(255.9f*(0.5f*sinf(yaw)+0.5f)); // range is from 0 (left) to 255 (right)
		}
		else sounds[chan].curpan = 127;

		if(camliquid && sounds[chan].slot->material == MAT_AIR) vol = int(vol*0.1f);
		else if(posliquid || camliquid) vol = int(vol*0.25f);

		float maxrad = float(sounds[chan].maxrad > 0 && sounds[chan].maxrad < soundmaxdist ? sounds[chan].maxrad : soundmaxdist);
		if(vol && sounds[chan].dist <= maxrad)
		{
			float minrad = float(sounds[chan].minrad < maxrad ? sounds[chan].minrad : maxrad);

			if(sounds[chan].dist > minrad)
				sounds[chan].curvol = int(float(vol)*((maxrad-minrad-sounds[chan].dist)/(maxrad-minrad)));
			else if(sounds[chan].dist <= camera1->radius)
				sounds[chan].curvol = vol;
		}
		else sounds[chan].curvol = 0;
	}
	else
	{
		sounds[chan].curvol = vol;
		sounds[chan].curpan = 127;
	}

	Mix_Volume(chan, sounds[chan].curvol);
	Mix_SetPanning(chan, 255-sounds[chan].curpan, sounds[chan].curpan);

	if(!(sounds[chan].flags&SND_NODELAY) && Mix_Paused(chan))
	{ // delay the sound based on average physical constants
		int delay = int((sounds[chan].dist/8.f)*(posliquid || camliquid ? 1.497f : 0.343f));
		if(totalmillis >= sounds[chan].millis+delay) Mix_Resume(chan);
	}
}

void updatesounds()
{
    updatemumble();
	if(nosound) return;

	loopv(sounds) if(sounds[i].inuse)
	{
		if(Mix_Playing(i)) updatesound(i);
		else sounds[i].inuse = false;
	}
	if(music && !Mix_PlayingMusic()) musicdone(true);
}

int playsound(int n, vec *pos, int vol, int maxrad, int minrad, int flags)
{
	if(nosound || !soundvol) return -1;

	if(!(flags&SND_MAP))
	{
		if(totalmillis == lastsoundmillis) soundsatonce++;
		else soundsatonce = 1;
		lastsoundmillis = totalmillis;
		if(soundmaxatonce && soundsatonce > soundmaxatonce) return -1;
	}

	physent *player = (physent *)cl->iterdynents(0);
	if(!player) player = camera1;
	vec *p = pos != NULL ? pos : &player->o;

	vector<soundslot> &soundset = flags&SND_MAP ? mapsounds : gamesounds;

	if(soundset.inrange(n) && soundset[n].sample->sound)
	{
		int chan = Mix_PlayChannel(-1, soundset[n].sample->sound, flags&SND_LOOP ? -1 : 0);

		if(chan >= 0)
		{
			if(!(flags&SND_NODELAY)) Mix_Pause(chan);

			while(chan >= sounds.length()) sounds.add().inuse = false;

			sounds[chan].slot = &soundset[n];
			sounds[chan].vol = vol > 0 && vol < 255 ? vol : 255;
			sounds[chan].maxrad = maxrad > 0 ? maxrad : getworldsize()/2;
			sounds[chan].minrad = minrad > 0 ? minrad : 0;
			sounds[chan].inuse = true;
			sounds[chan].flags = flags;
			sounds[chan].millis = totalmillis;

			if(flags&SND_COPY)
			{
				sounds[chan].posval = vec(*p);
				sounds[chan].pos = &sounds[chan].posval;
			}
			else sounds[chan].pos = p;

			updatesound(chan);
			return chan;
		}
		else conoutf("cannot play sound %d (%s): %s", n, soundset[n].sample->name, Mix_GetError());
	}
	else conoutf("unregistered sound: %d", n);

	return -1;
}

void sound(int *n, int *vol) { intret(playsound(*n, NULL, *vol ? *vol : 255)); }
COMMAND(sound, "ii");

void resetsound()
{
    const SDL_version *v = Mix_Linked_Version();
    if(SDL_VERSIONNUM(v->major, v->minor, v->patch) <= SDL_VERSIONNUM(1, 2, 8))
    {
        conoutf("Sound reset not available in-game due to SDL_mixer-1.2.8 bug. Please restart for changes to take effect.");
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

