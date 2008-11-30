#include "pch.h"
#include "engine.h"
#include "game.h"
namespace weapons
{
	VARP(autoreload, 0, 1, 10);// auto reload when 0:never 1:empty 2+:every(this*rdelay)
	VARP(skipplasma, 0, 0, 1); // whether to skip plasma when switching
	VARP(skipgrenades, 0, 1, 1); // whether to skip grenades when switching

	ICOMMAND(gunselect, "", (), intret(world::player1->gunselect));
	ICOMMAND(ammo, "s", (char *a),
	{
		int n = a[0] ? atoi(a) : world::player1->gunselect;
		if(n <= -1 || n >= GUN_MAX) return;
		intret(world::player1->ammo[n]);
	});

	void weaponswitch(gameent *d, int a = -1, int b = -1)
	{
		if(a < -1 || b < -1 || a >= GUN_MAX || b >= GUN_MAX) return;
		if(d->reqswitch >= 0) return;
		if (!world::allowmove(world::player1) || world::inzoom()) return;
		int s = d->gunselect;

		loopi(GUN_MAX) // only loop the amount of times we have guns for
		{
			if(a >= 0) s = a;
			else s += b;

			while(s > GUN_MAX-1) s -= GUN_MAX;
			while(s < 0) s += GUN_MAX;
			if(a < 0 && ((skipplasma && s == GUN_PLASMA) || (skipgrenades && s == GUN_GL)))
				continue;

			if(d->canswitch(s, lastmillis))
			{
				client::addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-world::maptime, s);
				d->reqswitch = lastmillis;
				return;
			}
			else if(a >= 0) break;
		}
		playsound(S_DENIED, d->o, d);
	}
	ICOMMAND(weapon, "ss", (char *a, char *b), weaponswitch(world::player1, a[0] ? atoi(a) : -1, b[0] ? atoi(b) : -1));

	bool doautoreload(gameent *d)
	{
		if(autoreload && !d->ammo[d->gunselect]) return true;
		if(autoreload > 1 && lastmillis-d->gunlast[d->gunselect] > guntype[d->gunselect].rdelay*autoreload)
			return true;

		return false;
	}

	void reload(gameent *d)
	{
		if(guntype[d->gunselect].rdelay <= 0)
		{
			int bestgun = d->bestgun();
			if(d->ammo[d->gunselect] <= 0 && d->canswitch(bestgun, lastmillis) && d->reqswitch < 0)
			{
				client::addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-world::maptime, bestgun);
				d->reqswitch = lastmillis;
			}
		}
		else if(d->canreload(d->gunselect, lastmillis) && d->reqreload < 0 && (d->reloading || doautoreload(d)))
		{
			client::addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-world::maptime, d->gunselect);
			d->reqreload = lastmillis;
		}
	}

	void offsetray(vec &from, vec &to, int spread, int z, vec &dest)
	{
		float f = to.dist(from)*float(spread)/10000.f;
		for(;;)
		{
			#define RNDD rnd(101)-50
			vec v(RNDD, RNDD, RNDD);
			if(v.magnitude() > 50) continue;
			v.mul(f);
			v.z = z > 0 ? v.z/float(z) : 0;
			dest = to;
			dest.add(v);
			vec dir = vec(dest).sub(from).normalize();
			raycubepos(from, dir, dest, 0, RAY_CLIPMAT|RAY_POLY);
			return;
		}
	}

	void shoot(gameent *d, vec &targ, int force)
	{
		if(!d->canshoot(d->gunselect, lastmillis)) return;

		int power = force;
		if(guntype[d->gunselect].power)
		{
			if(!power)
			{
				if(d->gunstate[d->gunselect] != GNS_POWER) // FIXME: not synched in MP yet!!
				{
					if(d->attacking) d->setgunstate(d->gunselect, GNS_POWER, 0, lastmillis);
					else return;
				}

				power = lastmillis-d->gunlast[d->gunselect];
				if(d->attacking && power < guntype[d->gunselect].power+guntype[d->gunselect].time)
					return;
			}
			d->attacking = false;
		}
		else if(!d->attacking) return;

		if(guntype[d->gunselect].max) d->ammo[d->gunselect] = max(d->ammo[d->gunselect]-1, 0);
		d->setgunstate(d->gunselect, GNS_SHOOT, guntype[d->gunselect].adelay, lastmillis);
		d->totalshots += int(guntype[d->gunselect].damage*damagescale)*guntype[d->gunselect].rays;

		vec to = targ, from = d->muzzle, unitv;
		float dist = to.dist(from, unitv);
		unitv.div(dist);
		vec kickback(unitv);
		kickback.mul(guntype[d->gunselect].kick*(physics::iscrouching(d) ? 0.1f : 1.f));
		d->vel.add(kickback);
		if(d == world::player1) world::quakewobble += guntype[d->gunselect].wobble;
		float barrier = raycube(from, unitv, dist, RAY_CLIPMAT|RAY_POLY);
		// move along the eye ray towards the gun origin, stopping when something is hit
		// nudge the target a tiny bit forward in the direction of the camera for stability
        if(barrier <= 1e-3f)
        {
            vec eyedir(from);
            eyedir.sub(d->o);
            float eyedist = eyedir.magnitude();
            eyedir.div(eyedist);
            eyedist = raycube(d->o, eyedir, eyedist, RAY_CLIPMAT);
            (from = eyedir).mul(eyedist).add(d->o);
            (to = camdir).mul(1e-3f).add(from);
        }
		else if(barrier < dist)
		{
			to = unitv;
			to.mul(barrier);
			to.add(from);
		}

		vector<vec> vshots;
		vector<ivec> shots;
		#define addshot(q) { vshots.add(q); shots.add(ivec(int(q.x*DMF), int(q.y*DMF), int(q.z*DMF))); }
		loopi(guntype[d->gunselect].rays)
		{
			vec dest;
			if(guntype[d->gunselect].spread)
				offsetray(from, to, guntype[d->gunselect].spread, guntype[d->gunselect].zdiv, dest);
			else dest = to;
			if(d->gunselect == GUN_GL)
			{
				float t = from.dist(dest);
				dest.z += t/8;
			}
			addshot(dest);
		}
		projs::shootv(d->gunselect, power, from, vshots, d, true);
		client::addmsg(SV_SHOOT, "ri7iv",
			d->clientnum, lastmillis-world::maptime, d->gunselect, power,
				int(from.x*DMF), int(from.y*DMF), int(from.z*DMF),
					shots.length(), shots.length()*sizeof(ivec)/sizeof(int), shots.getbuf());
	}

    void preload(int gun)
    {
    	int g = gun < 0 ? (m_insta(world::gamemode, world::mutators) ? instaspawngun : spawngun) : gun;
    	if(isgun(g))
        {
        	string mdl;
            s_sprintf(mdl)("weapons/%s", guntype[g].name);
            loadmodel(mdl, -1, true);
			s_sprintf(mdl)("weapons/%s/vwep", guntype[g].name);
			loadmodel(mdl, -1, true);
        }
    }
}
