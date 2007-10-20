extern FMOD_RESULT snderr;
extern FMOD_SYSTEM *sndsys;
extern bool nosound;
extern int soundvol;

#define SNDMINDIST	1.5f
#define SNDMAXDIST	10000.f

#define SNDCHK(b,r) \
	if ((snderr = b) != FMOD_OK) r;

#define SNDERR(b,r) \
	SNDCHK(b, \
	{ \
		conoutf("sound error: [%d] %s", snderr, FMOD_ErrorString(snderr)); \
		r; \
	})

struct soundsample
{
	FMOD_SOUND *sound;
	char *name;

	soundsample() : name(NULL) {}
	~soundsample() { DELETEA(name); }

	void load(const char *name, int loop = 0)
	{
		SNDERR(FMOD_System_CreateSound(sndsys, name, FMOD_3D|FMOD_SOFTWARE|FMOD_3D_WORLDRELATIVE, NULL, &sound), return);
		SNDERR(FMOD_Sound_SetMode(sound, FMOD_LOOP_NORMAL), );
		SNDERR(FMOD_Sound_SetLoopCount(sound, loop), );
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
	
	soundchan() { stop(); }
	~soundchan() {}

	void init(FMOD_CHANNEL *c, soundslot *s, float m = SNDMINDIST, float n = SNDMAXDIST)
	{
		channel = c;
		slot = s;
		mindist = m;
		maxdist = n;
		inuse = true;
	}

	void pause(bool t)
	{
		if (inuse)
		{
			SNDERR(FMOD_Channel_SetPaused(channel, t), );
		}
	}

	void stop()
	{
		if (inuse)
		{
			if (playing()) SNDERR(FMOD_Channel_Stop(channel), );
			inuse = false;
			pos = vel = NULL;
		}
	}

	bool playing()
	{
		FMOD_BOOL playing;
		SNDCHK(FMOD_Channel_IsPlaying(channel, &playing), return false);
		return playing != 0;
	}
	
	void update()
	{
		if (inuse)
		{
			if (playing())
			{
				FMOD_VECTOR psp = { pos->x, pos->y, pos->z }, psv = { vel->x, vel->y, vel->z };
				SNDERR(FMOD_Channel_Set3DAttributes(channel, &psp, &psv), );
				SNDERR(FMOD_Channel_Set3DMinMaxDistance(channel, mindist, maxdist), );
				SNDERR(FMOD_Channel_SetVolume(channel, (float(slot->vol)/255.f)*(float(soundvol)/255.f)), );
			}
			else stop();
		}
	}
	
	void position(vec *p, vec *v)
	{
		if (inuse)
		{
			pos = p;
			vel = v;
			update();
		}
	}
	
	void positionv(vec &p, vec &v)
	{
		if (inuse)
		{
			_pos = p;
			_vel = v;
			position(&_pos, &_vel);
		}
	}
};

extern hashtable<char *, soundsample> soundsamples;
extern vector<soundslot> gamesounds, mapsounds;
extern vector<soundchan> soundchans;

extern void checksound();
extern int addsound(char *name, int vol, int maxuses, vector<soundslot> &sounds);
extern int playsound(int n, vec *p = NULL, vec *v = NULL, float mindist = SNDMINDIST, float maxdist = SNDMAXDIST, vector<soundslot> &sounds = gamesounds);
extern int playsoundv(int n, vec &p, vec &v, float mindist = SNDMINDIST, float maxdist = SNDMAXDIST, vector<soundslot> &sounds = gamesounds);
extern void clearmapsounds();
extern void initsound();
