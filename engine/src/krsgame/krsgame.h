// Blood Frontier - KRSGAME - Kart Racing Simulator by Quinton Reeves
// This is the primary KRS definitions.

// defines
extern int curtexnum, curtime, gamespeed, lastmillis;
extern int hidehud, fov, invmouse, sensitivity, sensitivityscale;
extern int musicvol, soundvol, entselradius;
extern int testanims, animoverride, orientinterpolationtime;

extern physent *player, *camera1, *hitplayer;

extern bool menuactive();
extern void show_out_of_renderloop_progress(float bar1, const char *text1, float bar2 = 0, const char *text2 = NULL, GLuint tex = 0);

enum
{
	WL_REAR = 0,
	WL_LEFT,
	WL_FRONT,
	WL_RIGHT,
	WL_MAX
};

#define MAXGEARS 11
float gears[MAXGEARS] = { 100.f, 120.f, 140.f, 160.f, 180.f, 200.f, 220.f, 240.f, 260.f, 280.f, 300.f };

char *kartmdl[1] = { "sauerhog" };

struct cament : physent
{
	cament()
	{
		type = ENT_CAMERA;
	}
	~cament() {}
	
};

struct krsent : dynent
{
	vec wheels[2][WL_MAX];
	float targpitch, targroll;
	
	krsent() {}
	~krsent() {}
	
	void setbounds()
	{
		o.z -= eyeheight;
		setbbfrommodel((dynent *)this, kartmdl[0]);
		o.z += eyeheight;
	}
};

struct krsentity : extentity
{
	krsentity() {}
	~krsentity() {}
};

struct krsdummyserver : igameserver
{
	void *newinfo() { return NULL; }
	void deleteinfo(void *ci) {}
	void serverinit() {}
	void clientdisconnect(int n) {}
	int clientconnect(int n, uint ip) { return DISC_NONE; }
	void localdisconnect(int n) {}
	void localconnect(int n) {}
	char *servername() { return "none"; }
	void parsepacket(int sender, int chan, bool reliable, ucharbuf &p) {}
	bool sendpackets() { return false; }
	int welcomepacket(ucharbuf &p, int n) { return -1; }
	void serverinforeply(ucharbuf &p) {}
	void serverupdate(int lastmillis, int totalmillis) {}
	bool servercompatible(char *name, char *sdec, char *map, int ping, const vector<int> &attr, int np) { return false; }
	int serverinfoport() { return 0; }
	int serverport() { return 0; }
	char *getdefaultmaster() { return "none"; }
	int getmastertype() { return 1; }
	void sendservmsg(const char *s) {}
	char *gameident() { return "krs"; }
	char *gamepakdir() { return "krs"; }

	char *defaultmap() { return "start"; }
	char *gamename() { return "Kart Racing Simulator"; }
	
	string _gametitle;
	char *gametitle()
	{
		s_sprintf(_gametitle)("Free Play");
		return _gametitle;
	}
	
};
