extern bool nosound;
extern int soundvol;

#define SNDMINDIST	16.0f
#define SNDMAXDIST	10000.f

struct soundsample
{
	void *sound;
	char *name;

	soundsample() : name(NULL) {}
	~soundsample() { DELETEA(name); }

	void load(const char *file, int loop = 0)
	{
		return;
	}
};

struct soundslot
{
	soundsample *sample;
	int vol, maxuses;
};


struct soundchan
{
	void *channel;
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

	void init(void *c, soundslot *s, float m = SNDMINDIST, float n = SNDMAXDIST)
	{
		reset(true);
		channel = c;
		slot = s;
		mindist = m;
		maxdist = n;
	}

	void pause(bool t)
	{
		return;
	}

	void stop()
	{
		return;
	}

	bool playing()
	{
		return false;
	}
	
	void update()
	{
		return;
	}
	
	void position(vec *p, vec *v)
	{
		return;

		if (playing())
		{
			pos = p;
			vel = v;
			update();
		}
	}
	
	void positionv(vec &p, vec &v)
	{
		return;

		if (playing())
		{
			_pos = p;
			_vel = v;
			position(&_pos, &_vel);
		}
	}
};

extern hashtable<const char *, soundsample> sndsamples;
extern vector<soundslot> gamesounds, mapsounds;
extern vector<soundchan> sndchans;

extern void checksound();
extern int addsound(const char *name, int vol, int maxuses, vector<soundslot> &sounds);
extern int playsound(int n, vec *p = NULL, vec *v = NULL, float mindist = SNDMINDIST, float maxdist = SNDMAXDIST, vector<soundslot> &sounds = gamesounds);
extern int playsoundv(int n, vec &p, vec &v, float mindist = SNDMINDIST, float maxdist = SNDMAXDIST, vector<soundslot> &sounds = gamesounds);
extern void clearmapsounds();
extern void initsound();
