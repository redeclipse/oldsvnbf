#include "game.h"
namespace weapons
{
	VARP(autoreload, 0, 1, 10);// auto reload when 0:never 1:empty 2+:every(this*rdelay)
	VARP(skipspawnweapon, 0, 0, 1); // whether to skip spawnweapon when switching
	VARP(skipplasma, 0, 1, 1); // whether to skip plasma when switching
	VARP(skipgrenades, 0, 1, 1); // whether to skip grenades when switching

	ICOMMAND(weapselect, "", (), intret(world::player1->weapselect));
	ICOMMAND(ammo, "s", (char *a),
	{
		int n = a[0] ? atoi(a) : world::player1->weapselect;
		intret(isweap(n) ? world::player1->ammo[n] : -1);
	});

	void weaponswitch(gameent *d, int a = -1, int b = -1)
	{
		if(a < -1 || b < -1 || a >= WEAPON_MAX || b >= WEAPON_MAX) return;
		if(d->reqswitch >= 0 || world::inzoom()) return;
		int s = d->weapselect, sweap = m_spawnweapon(world::gamemode, world::mutators);
		loopi(WEAPON_MAX) // only loop the amount of times we have weaps for
		{
			if(a >= 0) s = a;
			else s += b;

			while(s > WEAPON_MAX-1) s -= WEAPON_MAX;
			while(s < 0) s += WEAPON_MAX;
			if(a < 0 && ((skipspawnweapon && s == m_spawnweapon(world::gamemode, world::mutators)) || (skipplasma && s == WEAPON_PLASMA) || (skipgrenades && s == WEAPON_GL)))
				continue;

			if(d->canswitch(s, sweap, lastmillis))
			{
				client::addmsg(SV_WEAPSELECT, "ri3", d->clientnum, lastmillis-world::maptime, s);
				d->reqswitch = lastmillis;
				return;
			}
			else if(a >= 0) break;
		}
		if(d == world::player1) playsound(S_DENIED, d->o, d);
	}
	ICOMMAND(weapon, "ss", (char *a, char *b), weaponswitch(world::player1, *a ? atoi(a) : -1, *b ? atoi(b) : -1));

	void drop(gameent *d, int a = -1)
	{
		int weap = isweap(a) ? a : d->weapselect;
		if(!m_noitems(world::gamemode, world::mutators) && isweap(weap) && ((weap == WEAPON_GL && d->ammo[weap] > 0) || entities::ents.inrange(d->entid[weap])) && d->reqswitch < 0)
		{
			client::addmsg(SV_DROP, "ri3", d->clientnum, lastmillis-world::maptime, weap);
			d->reqswitch = lastmillis;
		}
		else if(d == world::player1) playsound(S_DENIED, d->o, d);
	}
	ICOMMAND(drop, "s", (char *n), drop(world::player1, *n ? atoi(n) : -1));

	bool doautoreload(gameent *d)
	{
		if(autoreload && !d->ammo[d->weapselect]) return true;
		if(autoreload > 1 && lastmillis-d->weaplast[d->weapselect] > weaptype[d->weapselect].rdelay*autoreload)
			return true;
		return false;
	}

	void reload(gameent *d)
	{
		int sweap = m_spawnweapon(world::gamemode, world::mutators);
		if(!d->hasweap(d->weapselect, sweap))
		{
			int bestweap = d->bestweap(sweap);
			if(d->canswitch(bestweap, sweap, lastmillis) && d->reqswitch < 0)
			{
				client::addmsg(SV_WEAPSELECT, "ri3", d->clientnum, lastmillis-world::maptime, bestweap);
				d->reqswitch = lastmillis;
			}
		}
		else if(d->canreload(d->weapselect, sweap, lastmillis) && d->reqreload < 0 && (d->reloading || doautoreload(d)))
		{
			client::addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-world::maptime, d->weapselect);
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
		if(!d->canshoot(d->weapselect, m_spawnweapon(world::gamemode, world::mutators), lastmillis))
			return;

		int power = clamp(force, 0, weaptype[d->weapselect].power);
		if(weaptype[d->weapselect].power)
		{
			if(!power)
			{
				if(d->weapstate[d->weapselect] != WPSTATE_POWER)
				{
					if(d->attacking && world::allowmove(d))
					{
						client::addmsg(SV_PHYS, "ri2", d->clientnum, SPHY_POWER);
						d->setweapstate(d->weapselect, WPSTATE_POWER, 0, lastmillis);
					}
					else return;
				}

				power = clamp(lastmillis-d->weaplast[d->weapselect], 0, weaptype[d->weapselect].power);
				if(world::allowmove(d) && d->attacking && power < weaptype[d->weapselect].power) return;
			}
			d->attacking = false;
		}
		else if(!d->attacking || !world::allowmove(d)) return;

		if(weaptype[d->weapselect].max) d->ammo[d->weapselect] = max(d->ammo[d->weapselect]-1, 0);
		d->setweapstate(d->weapselect, WPSTATE_SHOOT, weaptype[d->weapselect].adelay, lastmillis);
		d->totalshots += int(weaptype[d->weapselect].damage*damagescale)*weaptype[d->weapselect].rays;

		vec to = targ, from = d->muzzle, unitv;
		float dist = to.dist(from, unitv);
		unitv.div(dist);
		vec kickback(unitv);
		kickback.mul(m_speedscale(weaptype[d->weapselect].kick*(physics::iscrouching(d) ? 0.1f : 1.f)));
		d->vel.add(kickback);
		if(d == world::player1) world::quakewobble += weaptype[d->weapselect].wobble;
		float barrier = raycube(from, unitv, dist, RAY_CLIPMAT|RAY_POLY);
		// move along the eye ray towards the weap origin, stopping when something is hit
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
		#define addshot { vshots.add(dest); shots.add(ivec(int(dest.x*DMF), int(dest.y*DMF), int(dest.z*DMF))); }
		loopi(weaptype[d->weapselect].rays)
		{
			vec dest;
			if(weaptype[d->weapselect].spread)
				offsetray(from, to, weaptype[d->weapselect].spread, weaptype[d->weapselect].zdiv, dest);
			else dest = to;
			if(d->weapselect == WEAPON_GL) dest.z += from.dist(dest)/8;
			addshot;
		}
		projs::shootv(d->weapselect, power, from, vshots, d, true);
		client::addmsg(SV_SHOOT, "ri7iv",
			d->clientnum, lastmillis-world::maptime, d->weapselect, power,
				int(from.x*DMF), int(from.y*DMF), int(from.z*DMF),
					shots.length(), shots.length()*sizeof(ivec)/sizeof(int), shots.getbuf());
	}

    void preload(int weap)
    {
    	int g = weap < 0 ? m_spawnweapon(world::gamemode, world::mutators) : weap;
    	if(isweap(g))
        {
            loadmodel(weaptype[g].item, -1, true);
			loadmodel(weaptype[g].vwep, -1, true);
        }
    }
}
