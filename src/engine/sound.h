extern FMOD_RESULT snderr;
extern FMOD_SYSTEM *sndsys;
extern bool nosound;
extern int soundvol;

#define SNDMINDIST	16.0f
#define SNDMAXDIST	10000.f

#define SNDERR(b,s,r) { \
	if ((snderr = b) != FMOD_OK) \
	{ \
		conoutf("sound error: failed to %s [%d] %s", s, snderr, FMOD_ErrorString(snderr)); \
		r; \
	} \
}

struct soundsample
{
	FMOD_SOUND *sound;
	char *name;

	soundsample() : name(NULL) {}
	~soundsample() { DELETEA(name); }

	void load(const char *name, int loop = 0)
	{
		string buf;
		s_sprintf(buf)("create sound '%s'", name);
		SNDERR(FMOD_System_CreateSound(sndsys, name, FMOD_3D|FMOD_SOFTWARE|FMOD_3D_WORLDRELATIVE, NULL, &sound), buf, return);

		s_sprintf(buf)("set loop mode on '%s'", name);
		SNDERR(FMOD_Sound_SetMode(sound, FMOD_LOOP_NORMAL), buf, );

		s_sprintf(buf)("set loop count on '%s'", name);
		SNDERR(FMOD_Sound_SetLoopCount(sound, loop), buf, );
	}
};

struct soundslot
{
	soundsample *sample;
	int vol, maxuses;
};

struct soundchan
{
	FMOD_CHANNEL *channel;
	soundslot *slot;
	vec *pos, _pos, *vel, _vel;
	float mindist, maxdist;
	bool inuse;
	
	soundchan() { reset(false); }
	~soundchan() {}

	void reset(bool i = false)
	{
		inuse = i;
		pos = vel = NULL;
	}

	void init(FMOD_CHANNEL *c, soundslot *s, float m = SNDMINDIST, float n = SNDMAXDIST)
	{
		reset(true);
		channel = c;
		slot = s;
		mindist = m;
		maxdist = n;
	}

	void pause(bool t)
	{
		if (playing())
		{
			SNDERR(FMOD_Channel_SetPaused(channel, t), "pause channel", );
		}
	}

	void stop()
	{
		if (playing())
		{
			SNDERR(FMOD_Channel_Stop(channel), "stop channel", );
			reset(false);
		}
	}

	bool playing()
	{
		FMOD_BOOL playing = (FMOD_BOOL)0;
		if (inuse) FMOD_Channel_IsPlaying(channel, &playing);
		if (inuse && !playing) reset(false);
		return playing != 0;
	}
	
	void update()
	{
		if (playing())
		{
			FMOD_VECTOR psp = { pos->x, pos->y, pos->z }, psv = { vel->x, vel->y, vel->z };
			SNDERR(FMOD_Channel_Set3DAttributes(channel, &psp, &psv), "set 3d attributes", );
			SNDERR(FMOD_Channel_Set3DMinMaxDistance(channel, mindist, maxdist), "set minmax distance", );
			SNDERR(FMOD_Channel_SetVolume(channel, (float(slot->vol)/255.f)*(float(soundvol)/255.f)), "set volume", );
		}
	}
	
	void position(vec *p, vec *v)
	{
		if (playing())
		{
			pos = p;
			vel = v;
			update();
		}
	}
	
	void positionv(vec &p, vec &v)
	{
		if (playing())
		{
			_pos = p;
			_vel = v;
			position(&_pos, &_vel);
		}
	}
};

extern hashtable<char *, soundsample> sndsamples;
extern vector<soundslot> gamesounds, mapsounds;
extern vector<soundchan> sndchans;

extern void checksound();
extern int addsound(char *name, int vol, int maxuses, vector<soundslot> &sounds);
extern int playsound(int n, vec *p = NULL, vec *v = NULL, float mindist = SNDMINDIST, float maxdist = SNDMAXDIST, vector<soundslot> &sounds = gamesounds);
extern int playsoundv(int n, vec &p, vec &v, float mindist = SNDMINDIST, float maxdist = SNDMAXDIST, vector<soundslot> &sounds = gamesounds);
extern void clearmapsounds();
extern void initsound();
