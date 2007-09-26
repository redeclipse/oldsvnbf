// the interface the engine uses to run the gameplay module

struct icliententities
{
    virtual ~icliententities() {}

    virtual void editent(int i) = 0;
    virtual const char *entnameinfo(entity &e) = 0;
    virtual const char *entname(int i) = 0;
    virtual int extraentinfosize() = 0;
    virtual void writeent(entity &e, char *buf) = 0;
    virtual void readent(entity &e, char *buf) = 0;
#ifdef BFRONTIER
	virtual bool wantext() { return false; }
	virtual bool isext(int type, int want = 0) { return false; }
	virtual void readext(gzFile &g, int version, int reg, int id, entity &e) { return; }
	virtual void writeext(gzFile &g, int reg, int id, int num, entity &e) { return; }
#endif
    virtual float dropheight(entity &e) = 0;
    virtual void rumble(const extentity &e) = 0;
    virtual void trigger(extentity &e) = 0;
    virtual void fixentity(extentity &e) = 0;
    virtual void entradius(extentity &e, float &radius, float &angle, vec &dir) {}
    virtual bool mayattach(extentity &e) { return false; }
    virtual bool attachent(extentity &e, extentity &a) { return false; }
    virtual extentity *newentity() = 0;
    virtual vector<extentity *> &getents() = 0;
};

struct iclientcom
{
    virtual ~iclientcom() {}

    virtual void gamedisconnect() = 0;
    virtual void parsepacketclient(int chan, ucharbuf &p) = 0;
    virtual int sendpacketclient(ucharbuf &p, bool &reliable, dynent *d) = 0;
    virtual void gameconnect(bool _remote) = 0;
    virtual bool allowedittoggle() = 0;
    virtual void edittoggled(bool on) {}
    virtual void writeclientinfo(FILE *f) = 0;
    virtual void toserver(char *text) = 0;
#ifndef BFRONTIER
    virtual void changemap(const char *name) = 0;
#endif
    virtual int numchannels() { return 1; }
#ifdef BFRONTIER
	virtual bool ready() { return true; }
	virtual int otherclients() { return 0; }
	virtual void toservcmd(char *text, bool msg) { return; }
#endif
};

#ifdef BFRONTIER
struct ibotcom
{
    virtual ~ibotcom() {}

	virtual dynent *newclient() { return NULL; }
	virtual bool allowed() { return false; }
	virtual void update(bot *b) { return; }
	virtual void parsepacketclient(bot *b, int chan, ucharbuf &p) { return; }
	virtual int sendpacketclient(bot *b, ucharbuf &p, bool &reliable) { return -1; }
	virtual void gameconnect(bot *b, bool _remote) { return; }
};

struct iphysics
{
    virtual ~iphysics() {}

	virtual float stairheight(physent *d) { return 4.1f; }
	virtual float floorz(physent *d) { return 0.867f; }
	virtual float slopez(physent *d) { return 0.5f; }
	virtual float wallz(physent *d) { return 0.2f; }
	virtual float jumpvel(physent *d) { return 125.0f; }
	virtual float gravity(physent *d) { return 200.0f; }
	virtual float speed(physent *d) { return 100.0f; }
	virtual float stepspeed(physent *d) { return 1.0f; }
	virtual float watergravscale(physent *d) { return d->move || d->strafe ? 0.f : 4.f; }
	virtual float waterdampen(physent *d) { return 8.f; }
	virtual float waterfric(physent *d) { return 20.f; }
	virtual float floorfric(physent *d) { return 6.f; }
	virtual float airfric(physent *d) { return 30.f; }
	virtual bool movepitch(physent *d) { return true; }
	virtual void updateroll(physent *d) { return; }
    virtual void trigger(physent *d, bool local, int floorlevel, int waterlevel) { return; }
};
#endif

struct igameclient
{
    virtual ~igameclient() {}

#ifndef BFRONTIER
    virtual char *gameident() = 0;
    virtual char *defaultmap() = 0;
    virtual char *savedconfig() = 0;
    virtual char *defaultconfig() = 0;
    virtual char *autoexec() = 0;
    virtual char *savedservers() { return NULL; }
#endif

    virtual icliententities *getents() = 0;
#ifdef BFRONTIER
    virtual iphysics *getphysics() = 0;
	virtual ibotcom *getbot() = 0;
#endif
    virtual iclientcom *getcom() = 0;

    virtual bool clientoption(char *arg) { return false; }
    virtual void updateworld(vec &pos, int curtime, int lm) = 0;
    virtual void initclient() = 0;
#ifndef BFRONTIER
    virtual void physicstrigger(physent *d, bool local, int floorlevel, int waterlevel) = 0;
#endif
    virtual void edittrigger(const selinfo &sel, int op, int arg1 = 0, int arg2 = 0, int arg3 = 0) = 0;
    virtual char *getclientmap() = 0;
    virtual void resetgamestate() = 0;
    virtual void suicide(physent *d) = 0;
    virtual void newmap(int size) = 0;
    virtual void startmap(const char *name) = 0;
    virtual void gameplayhud(int w, int h) = 0;
    virtual void drawhudgun() = 0;
    virtual bool canjump() = 0;
    virtual void doattack(bool on) = 0;
    virtual dynent *iterdynents(int i) = 0;
    virtual int numdynents() = 0;
    virtual void rendergame() = 0;
    virtual void writegamedata(vector<char> &extras) = 0;
    virtual void readgamedata(vector<char> &extras) = 0;
    virtual void g3d_gamemenus() = 0;
    virtual void crosshaircolor(float &r, float &g, float &b) {} 
    virtual void lighteffects(dynent *d, vec &color, vec &dir) {}
    virtual void setupcamera() {}
    virtual void adddynlights() {}
#ifdef BFRONTIER
	virtual void recomputecamera()
	{
		extern bool deathcam;
		extern physent *player, *camera1;
		
		if(deathcam && player->state!=CS_DEAD) deathcam = false;
		extern int testanims;
		if(((editmode && !testanims) || !thirdperson) && player->state!=CS_DEAD)
		{
			//if(camera1->state==CS_DEAD) camera1->o.z -= camera1->eyeheight-0.8f;
			camera1 = player;
		}
		else
		{
			static physent tempcamera;
			camera1 = &tempcamera;
			if(deathcam) camera1->o = player->o;
			else
			{
				*camera1 = *player;
				if(player->state==CS_DEAD) deathcam = true;
			}
			camera1->reset();
			camera1->type = ENT_CAMERA;
			camera1->move = -1;
			camera1->eyeheight = 2;
			
			loopi(10)
			{
				if(!moveplayer(camera1, 10, true, thirdpersondistance)) break;
			}
		}
	}
	virtual void findorientation()
	{
		extern physent *camera1;
		vec dir;
		vecfromyawpitch(camera1->yaw, camera1->pitch, 1, 0, dir);
		vecfromyawpitch(camera1->yaw, 0, 0, -1, camright);
		vecfromyawpitch(camera1->yaw, camera1->pitch+90, 1, 0, camup);
	
		if(raycubepos(camera1->o, dir, worldpos, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
			worldpos = dir.mul(10).add(camera1->o); //otherwise 3dgui won't work when outside of map
	}
	
	virtual void fixcamerarange()
	{
		extern physent *camera1;
		const float MAXPITCH = 90.0f;
		if(camera1->pitch>MAXPITCH) camera1->pitch = MAXPITCH;
		if(camera1->pitch<-MAXPITCH) camera1->pitch = -MAXPITCH;
		while(camera1->yaw<0.0f) camera1->yaw += 360.0f;
		while(camera1->yaw>=360.0f) camera1->yaw -= 360.0f;
	}
	
	virtual void mousemove(int dx, int dy)
	{
		extern int sensitivity, sensitivityscale, invmouse;
		extern physent *player, *camera1;
		const float SENSF = 33.0f;	 // try match quake sens
		camera1->yaw += (dx/SENSF)*(sensitivity/(float)sensitivityscale);
		camera1->pitch -= (dy/SENSF)*(sensitivity/(float)sensitivityscale)*(invmouse ? -1 : 1);
		extern void fixcamerarange();
		fixcamerarange();
		if(camera1!=player && player->state!=CS_DEAD)
		{
			player->yaw = camera1->yaw;
			player->pitch = camera1->pitch;
		}
	}
	virtual bool wantcrosshair()
	{
		extern physent *player;
		extern int hidehud;
		extern bool menuactive();
		return !(hidehud || player->state==CS_SPECTATOR) || menuactive();
	}
	
	virtual bool gamethirdperson() { extern int thirdperson; return thirdperson; } ;
	virtual bool gethudcolour(vec &colour) { return false; }

	virtual void loadworld(const char *name) { return; };
	virtual void saveworld(const char *name) { return; };

	virtual int localplayers() { return 1; }
	virtual bool gui3d() { return true; }

	virtual vec feetpos(physent *d)
	{
		//if (d->type == ENT_PLAYER || d->type == ENT_AI)
		//	return vec(d->o).sub(vec(0, 0, d->eyeheight));
		return vec(d->o);
	}
	
	virtual void menuevent(int event)
	{
		const char *s = NULL;
		
		switch (event)
		{
			case MN_BACK: s = "sfx/back"; break;
			case MN_INPUT: s = "sfx/input"; break;
			default: break;
		}
	
		if (s) playsoundname((char *)s);
	}
#endif
};

struct igameserver
{
    virtual ~igameserver() {}

    virtual bool serveroption(char *arg) { return false; }
    virtual void *newinfo() = 0;
    virtual void deleteinfo(void *ci) = 0;
    virtual void serverinit() = 0;
    virtual void clientdisconnect(int n) = 0;
    virtual int clientconnect(int n, uint ip) = 0;
    virtual void localdisconnect(int n) = 0;
    virtual void localconnect(int n) = 0;
    virtual char *servername() = 0;
    virtual void recordpacket(int chan, void *data, int len) {}
    virtual void parsepacket(int sender, int chan, bool reliable, ucharbuf &p) = 0;
    virtual bool sendpackets() = 0;
    virtual int welcomepacket(ucharbuf &p, int n) = 0;
    virtual void serverinforeply(ucharbuf &p) = 0;
    virtual void serverupdate(int lastmillis, int totalmillis) = 0;
#ifdef BFRONTIER
	virtual int servercompare(serverinfo *a, serverinfo *b) { return strcmp(a->name, b->name); }
    virtual const char *serverinfogui(g3d_gui *cgui, vector<serverinfo> &servers) { return NULL; }
#else
    virtual bool servercompatible(char *name, char *sdec, char *map, int ping, const vector<int> &attr, int np) = 0;
    virtual void serverinfostr(char *buf, const char *name, const char *desc, const char *map, int ping, const vector<int> &attr, int np) = 0;
#endif
    virtual int serverinfoport() = 0;
    virtual int serverport() = 0;
    virtual char *getdefaultmaster() = 0;
    virtual void sendservmsg(const char *s) = 0;
#ifdef BFRONTIER
    virtual void changemap(const char *s, int mode = 0) { return; }
	virtual int getmastertype() { return 0; }
    virtual char *gameident() = 0;
	virtual char *gamepakdir() = 0;
	virtual char *gamename() = 0;
	virtual char *gametitle() = 0;
    virtual char *defaultmap() = 0;

	virtual bool canload(char *type, int version)
	{
		if (!strcmp(type, gameident())) return true;
		return false;
	}
#endif
};

struct igame
{
    virtual ~igame() {}

    virtual igameclient *newclient() = 0;
    virtual igameserver *newserver() = 0;
};
