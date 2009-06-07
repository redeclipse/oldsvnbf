#include "game.h"
namespace weapons
{
	VARP(autoreloading, 0, 2, 2); // 0 = don't autoreload at all, 1 = only reload when gun is empty, 2 = always reload weapons that don't add a full clip
	VARP(skipspawnweapon, 0, 0, 1); // whether to skip spawnweapon when switching
	VARP(skippistol, 0, 0, 1); // whether to skip pistol when switching
	VARP(skipgrenade, 0, 0, 1); // whether to skip grenade when switching

	ICOMMAND(weapselect, "", (), intret(game::player1->weapselect));
	ICOMMAND(ammo, "s", (char *a),
	{
		int n = a[0] ? atoi(a) : game::player1->weapselect;
		intret(isweap(n) ? game::player1->ammo[n] : -1);
	});

	bool weapselect(gameent *d, int weap, bool local)
	{
		if(!local || d->canswitch(weap, m_spawnweapon(game::gamemode, game::mutators), lastmillis, WPSTATE_RELOAD))
		{
			if(local) client::addmsg(SV_WEAPSELECT, "ri3", d->clientnum, lastmillis-game::maptime, weap);
			playsound(S_SWITCH, d->o, d);
			d->weapswitch(weap, lastmillis);
			d->reloading = false;
			return true;
		}
		return false;
	}

	bool weapreload(gameent *d, int weap, int load, int ammo, bool local)
	{
		if(!local || d->canreload(weap, m_spawnweapon(game::gamemode, game::mutators), lastmillis))
		{
			if(local)
			{
				client::addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-game::maptime, weap);
				playsound(S_RELOAD, d->o, d);
				d->setweapstate(weap, WPSTATE_RELOAD, weaptype[weap].rdelay, lastmillis);
				int oldammo = d->ammo[weap];
				ammo = min(max(d->ammo[weap], 0) + weaptype[weap].add, weaptype[weap].max);
				load = ammo-oldammo;
			}
			else if(load < 0 && d->ammo[weap] < ammo && (d == game::player1 || d->ai))
				return false; // because we've already gone ahead..
			d->weapload[weap] = load;
			d->ammo[weap] = min(ammo, weaptype[weap].max);
			return true;
		}
		return false;
	}

	void weaponswitch(gameent *d, int a = -1, int b = -1)
	{
		if(a < -1 || b < -1 || a >= WEAPON_MAX || b >= WEAPON_MAX) return;
		if(!d->weapwaited(d->weapselect, lastmillis, d->skipwait(d->weapselect, WPSTATE_RELOAD))) return;
		int s = d->weapselect;
		loopi(WEAPON_MAX) // only loop the amount of times we have weaps for
		{
			if(a >= 0) s = a;
			else s += b;

			while(s > WEAPON_MAX-1) s -= WEAPON_MAX;
			while(s < 0) s += WEAPON_MAX;

			if(a < 0)
			{ // weapon skipping when scrolling
				if(skipspawnweapon && s == m_spawnweapon(game::gamemode, game::mutators)) continue;
				if(skippistol && s == WEAPON_PISTOL) continue;
				if(skipgrenade && s == WEAPON_GRENADE) continue;
			}

			if(weapselect(d, s)) return;
			else if(a >= 0) break;
		}
		if(d == game::player1) playsound(S_DENIED, d->o, d);
	}
	ICOMMAND(weapon, "ss", (char *a, char *b), weaponswitch(game::player1, *a ? atoi(a) : -1, *b ? atoi(b) : -1));

	void drop(gameent *d, int a = -1)
	{
		int weap = isweap(a) ? a : d->weapselect;
		if(!m_noitems(game::gamemode, game::mutators) && isweap(weap) && ((weap == WEAPON_GRENADE && d->ammo[weap] > 0) || entities::ents.inrange(d->entid[weap])) && d->weapwaited(d->weapselect, lastmillis, d->skipwait(d->weapselect, WPSTATE_RELOAD)))
		{
			client::addmsg(SV_DROP, "ri3", d->clientnum, lastmillis-game::maptime, weap);
			d->setweapstate(d->weapselect, WPSTATE_WAIT, WEAPSWITCHDELAY, lastmillis);
			d->reloading = false;
		}
		else if(d == game::player1) playsound(S_DENIED, d->o, d);
	}
	ICOMMAND(drop, "s", (char *n), drop(game::player1, *n ? atoi(n) : -1));

	void reload(gameent *d)
	{
		int sweap = m_spawnweapon(game::gamemode, game::mutators);

		if(d->canreload(d->weapselect, sweap, lastmillis) && weaptype[d->weapselect].add < weaptype[d->weapselect].max && autoreloading > 1 && !d->attacking && !d->reloading && !d->useaction)
			d->reloading = true;

		if(!d->hasweap(d->weapselect, sweap)) weapselect(d, d->bestweap(sweap, true));
		else if(d->reloading || (autoreloading && !d->ammo[d->weapselect])) weapreload(d, d->weapselect);
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
		if(!game::allowmove(d)) return;
		int offset = 1;
		if(!d->canshoot(d->weapselect, m_spawnweapon(game::gamemode, game::mutators), lastmillis))
		{
			if(!d->canshoot(d->weapselect, m_spawnweapon(game::gamemode, game::mutators), lastmillis, WPSTATE_RELOAD)) return;
			else offset = 0;
		}
		int power = clamp(force, 0, weaptype[d->weapselect].power);
		if(weaptype[d->weapselect].power)
		{
			if(!power)
			{
				if(d->weapstate[d->weapselect] != WPSTATE_POWER)
				{
					if(d->attacking)
					{
						client::addmsg(SV_PHYS, "ri2", d->clientnum, SPHY_POWER);
						d->setweapstate(d->weapselect, WPSTATE_POWER, 0, lastmillis);
					}
					else return;
				}
				power = clamp(lastmillis-d->weaplast[d->weapselect], 0, weaptype[d->weapselect].power);
				if(d->attacking && power < weaptype[d->weapselect].power) return;
			}
			d->attacking = false;
		}
		else if(!d->attacking) return;

		if(!offset)
		{
			offset = 1+max(d->weapload[d->weapselect], 1);
			d->weapload[d->weapselect] = -d->weapload[d->weapselect];
		}

		d->reloading = false;
		int adelay = weaptype[d->weapselect].adelay;
		if(!weaptype[d->weapselect].fullauto)
		{
			d->attacking = false;
			if(d->ai) adelay += int(adelay*(((101-d->skill)+rnd(111-d->skill))/100.f));
		}
		d->setweapstate(d->weapselect, WPSTATE_SHOOT, adelay, lastmillis);
		d->ammo[d->weapselect] = max(d->ammo[d->weapselect]-offset, 0);
		d->totalshots += int(weaptype[d->weapselect].damage*damagescale)*weaptype[d->weapselect].rays;

		vec to = targ, from = d->muzzle, unitv;
		float dist = to.dist(from, unitv);
		unitv.div(dist);
		d->vel.add(vec(unitv).mul(m_speedscale(weaptype[d->weapselect].kick*(physics::iscrouching(d) ? 0.1f : 1.f))));
		if(d == game::player1) game::quakewobble += weaptype[d->weapselect].wobble;

		// move along the eye ray towards the weap origin, stopping when something is hit
		// nudge the target a tiny bit forward in the direction of the camera for stability
		float barrier = raycube(from, unitv, dist, RAY_CLIPMAT|RAY_POLY);
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
			int spread = d == game::player1 && weaptype[d->weapselect].zooms && game::inzoom() ? 0 : weaptype[d->weapselect].spread;
			if(spread) offsetray(from, to, weaptype[d->weapselect].spread, weaptype[d->weapselect].zdiv, dest);
			else dest = to;
			if(d->weapselect == WEAPON_GRENADE) dest.z += from.dist(dest)/8;
			addshot;
		}
		projs::shootv(d->weapselect, power, from, vshots, d, true);
		client::addmsg(SV_SHOOT, "ri7iv",
			d->clientnum, lastmillis-game::maptime, d->weapselect, power,
				int(from.x*DMF), int(from.y*DMF), int(from.z*DMF),
					shots.length(), shots.length()*sizeof(ivec)/sizeof(int), shots.getbuf());
	}

    void preload(int weap)
    {
    	int g = weap < 0 ? m_spawnweapon(game::gamemode, game::mutators) : weap;
    	if(isweap(g))
        {
            loadmodel(weaptype[g].item, -1, true);
			loadmodel(weaptype[g].vwep, -1, true);
        }
    }
}
