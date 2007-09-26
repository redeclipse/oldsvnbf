// Blood Frontier - KRSGAME - Kart Racing Simulator by Quinton Reeves
// This is the KRS entities.

struct krsentities : icliententities
{
	krsclient &cl;
	vector<krsentity *> ents;

	struct krsitem
	{
		char *name;								// name
		bool touch, pickup, proj;						// touchable / collectable / by projectile
		int render, wait;						// render type (0: don't, 1: bobbing/rotating, 2: static) / wait time
		float yaw, zoff, height, radius, elast;	// yaw / z offset / height / radius / bounciness
	} *krsitems;

	krsentities(krsclient &_cl) : cl(_cl)
	{
		//	n					t		p		j		r	w	y		z		h		r		e
		static krsitem _krsitems[] = {
			{ "none?",			false,	false,	false,	0,	0, 	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "light",			false,	false,	false,	0,	0, 	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "mapmodel",		false,	false,	false,	0,	0, 	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "playerstart",	false,	false,	false,	0,	0, 	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "envmap",			false,	false,	false,	0,	0, 	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "particles",		false,	false,	false,	0,	0, 	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "sound",			false,	false,	false,	0,	0, 	0.f,	0.f,	0.f,	0.f,	0.0f },
			{ "spotlight",		false,	false,	false,	0,	0, 	0.f,	0.f,	0.f,	0.f,	0.0f }
		};
		krsitems = _krsitems;
	}
	~krsentities() { }

	vector<extentity *> &getents() { return (vector<extentity *> &)ents; }

	void fixentity(extentity &d) {}
	void editent(int i) {}
	const char *entnameinfo(entity &e) { return ""; }
	const char *entname(int i) { return i >= 0 && i < ET_GAMESPECIFIC ? krsitems[i].name : ""; }

	int extraentinfosize() { return 0; }
	void writeent(entity &e, char *buf) {}
	void readent (entity &e, char *buf) {}
	float dropheight(entity &e) { return 0.f; }

	void rumble(const extentity &e) {}
	void trigger(extentity &e) {}

	extentity *newentity() { return new krsentity; }
};
