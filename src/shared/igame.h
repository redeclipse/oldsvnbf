// the interface the engine uses to run the gameplay module

struct icliententities
{
    virtual ~icliententities() {}

    virtual void editent(int i) = 0;
    virtual const char *entnameinfo(entity &e) = 0;
    virtual const char *entname(int i) = 0;
	virtual void readent(gzFile &g, int maptype, int id, entity &e) { return; }
	virtual void writeent(gzFile &g, int id, entity &e) { return; }
    virtual float dropheight(entity &e) = 0;
    virtual void rumble(extentity &e) = 0;
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

    virtual void gamedisconnect(int clean) = 0;
    virtual void parsepacketclient(int chan, ucharbuf &p) = 0;
    virtual int sendpacketclient(ucharbuf &p, bool &reliable, dynent *d) = 0;
    virtual void gameconnect(bool _remote) = 0;
    virtual bool allowedittoggle(bool edit) = 0;
    virtual void edittoggled(bool edit) {}
    virtual void writeclientinfo(FILE *f) = 0;
    virtual void toserver(char *text, bool action = false) = 0;
    virtual void changemap(const char *name) = 0;
	virtual bool ready() { return true; }
	virtual int otherclients() { return 0; }
	virtual void toservcmd(char *text, bool msg) { return; }
    virtual int numchannels() { return 1; }
};

struct igameclient
{
    virtual ~igameclient() {}

    virtual icliententities *getents() = 0;
    virtual iclientcom *getcom() = 0;

    virtual bool clientoption(char *arg) { return false; }
    virtual void updateworld(vec &pos, int curtime, int lm) = 0;
    virtual void initclient() = 0;
    virtual void editvariable(char *name, int value) = 0;
    virtual void edittrigger(const selinfo &sel, int op, int arg1 = 0, int arg2 = 0, int arg3 = 0) = 0;
    virtual void resetgamestate() = 0;
    virtual void suicide(physent *d) = 0;
    virtual void newmap(int size) = 0;
    virtual void startmap(const char *name) = 0;
    virtual void preload() {}
    virtual void gameplayhud(int w, int h) = 0;
    virtual void drawhudgun() = 0;
    virtual bool canjump() = 0;
    virtual bool allowmove(physent *d) { return true; }
    virtual dynent *iterdynents(int i) = 0;
    virtual int numdynents() = 0;
    virtual void rendergame() = 0;
    virtual void g3d_gamemenus() = 0;
    virtual void crosshaircolor(float &r, float &g, float &b) {}
    virtual void lighteffects(dynent *d, vec &color, vec &dir) {}
    virtual void adddynlights() {}
    virtual void particletrack(physent *owner, vec &o, vec &d) {}
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

	virtual void fixview()
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
		extern physent *camera1;
		const float SENSF = 33.0f;	 // try match quake sens
		camera1->yaw += (dx/SENSF)*(sensitivity/(float)sensitivityscale);
		camera1->pitch -= (dy/SENSF)*(sensitivity/(float)sensitivityscale)*(invmouse ? -1 : 1);
		fixview();
	}

	virtual void recomputecamera()
	{
		fixview();
	}

	virtual bool wantcrosshair()
	{
		extern int hidehud;
		extern bool menuactive();
		return !hidehud && !menuactive();
	}

	virtual bool gamethirdperson() { return false; } ;
	virtual bool gethudcolour(vec &colour) { return false; }

	virtual void loadworld(gzFile &f, int maptype) { return; };
	virtual void saveworld(gzFile &f, FILE *h) { return; };

	virtual int localplayers() { return 1; }
	virtual bool gui3d() { return true; }

	virtual vec feetpos(physent *d)
	{
		//if (d->type == ENT_PLAYER || d->type == ENT_AI)
		//	return vec(d->o).sub(vec(0, 0, d->eyeheight));
		return vec(d->o);
	}

	virtual void menuevent(int event) { return; }
	virtual char *gametitle() = 0;
	virtual char *gametext() = 0;
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
    virtual char *servername() = 0;
    virtual void recordpacket(int chan, void *data, int len) {}
    virtual void parsepacket(int sender, int chan, bool reliable, ucharbuf &p) = 0;
    virtual bool sendpackets() = 0;
    virtual int welcomepacket(ucharbuf &p, int n) = 0;
    virtual void serverinforeply(ucharbuf &p) = 0;
    virtual void serverupdate(int lastmillis, int totalmillis) = 0;
	virtual int servercompare(serverinfo *a, serverinfo *b) { return strcmp(a->name, b->name); }
    virtual const char *serverinfogui(g3d_gui *cgui, vector<serverinfo> &servers) { return NULL; }
    virtual int serverinfoport() = 0;
    virtual int serverport() = 0;
    virtual char *getdefaultmaster() = 0;
    virtual void sendservmsg(const char *s) = 0;
    virtual void changemap(const char *s, int mode = 0, int muts = 0) { return; }
    virtual char *gameid() = 0;
	virtual char *gamename(int mode, int muts) = 0;
    virtual char *defaultmap() = 0;
    virtual int defaultmode() = 0;
    virtual bool canload(char *type) = 0;
};

struct igame
{
    virtual ~igame() {}

    virtual igameclient *newclient() = 0;
    virtual igameserver *newserver() = 0;
};
