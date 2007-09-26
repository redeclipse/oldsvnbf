// Blood Frontier - KRSGAME - Kart Racing Simulator by Quinton Reeves
// This is the primary KRS game module.

#ifdef BFRONTIER
#include "pch.h"
#include "cube.h"

#include "iengine.h"
#include "igame.h"

#include "krsgame.h"

struct krsclient : igameclient
{
	#include "krsrender.h"
	#include "krsents.h"

	struct krsdummycom : iclientcom
	{
		krsdummycom()
		{
			CCOMMAND(krsdummycom, map, "s", self->changemap(args[0]));
		}
		~krsdummycom() { }
	
		void gamedisconnect() {}
		void parsepacketclient(int chan, ucharbuf &p) {}
		int sendpacketclient(ucharbuf &p, bool &reliable, dynent *d) { return -1; }
		void gameconnect(bool _remote) {}
		bool allowedittoggle() { return true; }
		void writeclientinfo(FILE *f) {}
		bool ready() { return true; }
		void toserver(char *text) {}
		void changemap(const char *name) { load_world(name); }
	};
	
	struct krsdummybot : ibotcom
	{
		~krsdummybot() { }
	};
	
	struct krsdummyphysics : iphysics
	{
		krsclient &cl;
	
		krsdummyphysics(krsclient &_cl) : cl(_cl) { }
	
		float stairheight(physent *d) { return 4.1f; }
		float floorz(physent *d) { return 0.867f; }
		float slopez(physent *d) { return 0.5f; }
		float wallz(physent *d) { return 0.2f; }
		float jumpvel(physent *d) { return 100.f; }
		float gravity(physent *d) { return 150.f; }
		float speed(physent *d)
		{
			if (cl.iskart(d) && d->state != CS_SPECTATOR && d->state != CS_EDITING)
			{
				float mag = vec(d->vel.x, d->vel.y, 0).magnitude();
			
				if (mag && !d->timeinair)
				{
					loopi(MAXGEARS)
					{
						if (mag < gears[i]) return gears[i];
					}
					return gears[MAXGEARS-1];
				}
				return gears[0];
			}
			return 100.f;
		}
		float stepspeed(physent *d) { return 1.0f; }
	
		float watergravscale(physent *d)
		{
			if (cl.iskart(d))
			{
				return getmatvec(vec(d->o.x, d->o.y, d->o.z+1.f)) == MAT_AIR ? 1.f : 32.f;
			}
			return 1.f;
		}
		float waterdampen(physent *d)
		{
			if (cl.iskart(d))
			{
				return getmatvec(vec(d->o.x, d->o.y, d->o.z+1.f)) == MAT_AIR ? 1.6f : 16.f;
			}
			return 1.f;
		}
		float waterfric(physent *d)
		{
			if (cl.iskart(d))
			{
				return getmatvec(vec(d->o.x, d->o.y, d->o.z+1.f)) == MAT_AIR ? 1e8f : 32.f;
			}
			return 6.f;
		}
		
		float floorfric(physent *d)
		{
			if (cl.iskart(d))
			{
				float mag = vec(d->vel.x, d->vel.y, 0).magnitude();
				if (mag)
				{
					return 10.f+(10.f*(mag/(gears[MAXGEARS-1]*1.3f)));
				}
				return 10.f;
			}
			return 6.f;
		}
		float airfric(physent *d)
		{
			return cl.iskart(d) ? 5e2f : 30.f;
		}
	
		bool movepitch(physent *d)
		{
			if (d->type == ENT_CAMERA || (!d->timeinair && !d->inwater)) return true;
			return false;
		}
		void updateroll(physent *d) {}
	
		void trigger(physent *d, bool local, int floorlevel, int waterlevel) {}
	};
	
    krsdummyphysics	ph;
    krsentities		et;
	krsdummybot		bc;
    krsdummycom		cc;

    cament *camera;
    krsent *player1;
	
	IVAR(showdebug, 0, 1, 1);

    krsclient() : ph(*this), et(*this), camera(new cament()), player1(new krsent()) { }
    ~krsclient() { }

    char *getclientmap() { return mapname; }
	iphysics *getphysics() { return &ph; }
    icliententities *getents() { return &et; }
	ibotcom *getbot() { return &bc; }
    iclientcom *getcom() { return &cc; }

	bool gui3d() { return false; }

	vec feetpos(physent *d)
	{
		if (d->type == ENT_PLAYER || d->type == ENT_AI)
			return vec(d->o).sub(vec(0, 0, d->eyeheight-1));
		
		return vec(d->o);
	}
	bool iskart(physent *d) { return d->type == ENT_PLAYER && (d != player1 || isthirdperson()); }
	
	void updateplayer(krsent *d)
	{
		int move = d->move, strafe = d->strafe;
		vec o(d->o);
		
		if (d->type == ENT_PLAYER)
		{
			if (iskart(d))
			{
				d->strafe = 0; // strafe isn't used by movement
			}
			else
			{
				d->roll = 0.f;
			}

			moveplayer(d, 20, true);
			
			if (iskart(d))
			{
				d->move = move;
				d->strafe = strafe;
				
				loopi(WL_MAX)
				{
					d->wheels[0][i] = vec(d->xradius*cosf(2*M_PI*i/4.0f), d->yradius*sinf(2*M_PI*i/4.0f), 0);
					d->wheels[0][i].rotate_around_z((d->yaw+90)*RAD);
					d->wheels[0][i].add(d->o);

					vec down(0, 0, -1);
					if(raycubepos(d->wheels[0][i], down, d->wheels[1][i], 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
						d->wheels[1][i] = vec(down).mul(10).add(d->o);
	
					vec unitv;
					float dist = d->wheels[1][i].dist(d->wheels[0][i], unitv);
					unitv.div(dist);
	
					float shorten = 0.0f;
					if(dist > 1024) shorten = 1024;
	
					float barrier = raycube(d->wheels[0][i], unitv, dist, RAY_CLIPMAT|RAY_POLY);
	
					if(barrier < dist && (!shorten || barrier < shorten))
						shorten = barrier;
	
					if(shorten)
					{
						d->wheels[1][i] = unitv;
						d->wheels[1][i].mul(shorten);
						d->wheels[1][i].add(d->wheels[0][i]);
					}
				}

				if (!d->timeinair && (!d->inwater || d->floor.z < ph.floorz(d)))
				{
					vec q;
	
					q = d->wheels[1][WL_FRONT];
					q.sub(d->wheels[1][WL_REAR]);
					d->targpitch = asin(q.z/q.magnitude())/RAD;
	
					q = d->wheels[1][WL_RIGHT];
					q.sub(d->wheels[1][WL_LEFT]);
					d->targroll = asin(q.z/q.magnitude())/RAD;
				}
				else
				{
					d->targpitch = 0.f;
					d->targroll = 0.f;
				}
				
				float rotspeed = curtime/(d->timeinair ? 100.f : (d->inwater ? 50.f : 5.f));
				if (d->targpitch > d->pitch)
				{
					d->pitch += rotspeed;
					if (d->pitch > d->targpitch) d->pitch = d->targpitch;
				}
				else if (d->targpitch < d->pitch)
				{
					d->pitch -= rotspeed;
					if (d->pitch < d->targpitch) d->pitch = d->targpitch;
				}

				if (d->targroll > d->roll)
				{
					d->roll += rotspeed;
					if (d->roll > d->targroll) d->roll = d->targroll;
				}
				else if (d->targroll < d->roll)
				{
					d->roll -= rotspeed;
					if (d->roll < d->targroll) d->roll = d->targroll;
				}
		
				float mag = vec(d->vel.x, d->vel.y, 0).magnitude();
	
				if (mag)
				{
					if (strafe)
					{
						float yawamt = (float(curtime)*(mag/10.f))/500.f;
						
						#define incyaw(p) \
							if (strafe > 0 || move < 0) { p->yaw -= yawamt; } \
							else { p->yaw += yawamt; } \
							fixcamerarange(p, false);
						
						incyaw(d);
						incyaw(camera1);
					}
				}
			}
			else
			{
				d->roll = 0;
			}
		}
	}

    void updateworld(vec &pos, int ct, int lm)
    {
		if(!curtime) return;
		
		physicsframe();
		updateplayer(player1);
    }
	
    void initclient() {}
    void edittrigger(const selinfo &sel, int op, int arg1 = 0, int arg2 = 0, int arg3 = 0) {}
	void suicide(physent *d) {}
	void newmap(int size) {}
	
    int numdynents() { return 1; }
    dynent *iterdynents(int i)
	{
		if (!i) { return player1; } i--;
		return NULL;
	}

    void resetgamestate() {}
	void startmap(const char *name)
	{
		//player1->o = vec(getworldsize()*0.5f, getworldsize()*0.5f, getworldsize());
		player1->setbounds();
		findplayerspawn(player1);
		recomputecamera();
		camera1->yaw = player1->yaw;
	}

    bool canjump() { return !menuactive() && !player1->inwater; }
    void doattack(bool on)
    {
    	if (player1->state == CS_DEAD)
    	{
    		player1->state = CS_ALIVE;
    		findplayerspawn(player1);
    	}

    }

    void writegamedata(vector<char> &extras) {}
    void readgamedata(vector<char> &extras) {}

	void loadworld(const char *name)
	{
		//const char *worldname = *name ? name : "untitled";
		//s_strcpy(mapname, worldname);
	}

	void saveworld(const char *name) {}
};

REGISTERGAME(krsgame, "krs", new krsclient(), new krsdummyserver());
#endif

