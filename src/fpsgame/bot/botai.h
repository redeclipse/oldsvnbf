// Blood Frontier - BOT - Offline test and practice AI by Quinton Reeves
// Included by: fpsgame/bot/bot.h
// This is the main intelligence and logic for the bots.

void trans(fpsent *d, int stat, int mv, int st, bool dev, int n)
{ 
	if (stat >= M_NONE && stat <= M_AIMING)
	{
		d->move = mv;
		d->strafe = st;
		int q = (dev ? rnd(d->botrate) : 0), v = n + q;
		d->botstate = stat;
		d->botupdate = v;
	}
}

void think(int time, fpsent *d)
{ 
	if (cl.intermission) return;

	int gamemode = cl.gamemode;

	if ((d->botflags & BOT_PLAYER) && d->state == CS_EDITING)
	{
		d->state = CS_DEAD;
		trans(d, M_PAIN, 0, 0, false, 2500); // TODO: canspawn
	}

	if (d->state == CS_ALIVE || d->state == CS_DEAD) action(d);

	if (d->state == CS_ALIVE && d->botstate != M_PAIN)
	{ 
		vec o(d->o);
		vec v(vec(d->o).sub(vec(0, 0, d->eyeheight)));

		if (d->botflags & BOT_PLAYER)
		{ 
			cl.et.checkitems(d);
		}

		bool aiming = aimat(d);

		if (!aiming && d->botstate == M_HOME)
		{ 
			if (d->botnode == d->bottarg || d->botvec.dist(v) <= BOTISNEAR)
			{
				coord(d, 0);
			}
			else if ((!d->timeinair || d->inwater) && d->botvec.z - v.z >= BOTJUMPDIST)
			{
				if (d->botvec.z - v.z >= BOTJUMPMAX)
				{
					coord(d, 0, true);
				}
				else
				{
					d->jumpnext = true;
				}
			}
		}
		else if (d->botstate == M_SLEEP) coord(d, 0);

		int move = d->move, strafe = d->strafe;

		if (aiming || d->botstate == M_SLEEP)
		{ 
			d->move = d->strafe = 0;
		}

		bool dojump = false;
		
		if (d->jumpnext)
		{ 
			if (d->botjump >= cl.lastmillis || d->timeinair)
			{ 
				d->jumpnext = false;
			}
			else
			{ 
				dojump = true;

				if (!aiming && !d->inwater && d->botvec.z - v.z > BOTJUMPWEAP &&
					cl.lastmillis-gunvar(d->gunlast,d->gunselect) >= gunvar(d->gunwait,d->gunselect) &&
						d->botvec.z - v.z <= BOTJUMPMAX)
				{ 
					vec old(d->botvec);
					d->botvec = v;
					aimat(d, true);
					target(d);
					weapon(d, true);
					shoot(d, true);
					d->botvec = old;
				}
			}
		}

		cl.ph.move(d, d->botflags & BOT_PLAYER ? 20 : 2, false);
		cl.ph.updatewater(d, 0);

		d->move = move;
		d->strafe = strafe;

		if (dojump)
		{
			if (d->o.z == o.z) d->botjump = cl.lastmillis+BOTJUMPWAIT;
			else d->botjump = cl.lastmillis+BOTJUMPTIME;
		}

		if ((d->botflags & BOT_PLAYER) && !aiming && d->botstate != M_SLEEP)
		{
			int oldchk = d->botcheck;
			
			if (d->o == o)
			{
				d->botcheck += time;
				
#if 0
				if (oldchk < BOTCHECKRESET && d->botcheck >= BOTCHECKRESET)
				{
					reset(d, true); // maybe went off course?
					if (!find(d)) scout(d);
				}
				else
#endif
				if (oldchk < BOTCHECKCOORD && d->botcheck >= BOTCHECKCOORD) cl.suicide(d); // give up the ghost
			}
			else if (m_classicsp && d->botenemy == NULL &&
				cl.player1->state == CS_ALIVE && d->o.dist(cl.player1->o) >= BOTISFAR)
			{
				d->botcheck += time;
				if (!oldchk) wayfind(d, vec(cl.player1->o).sub(vec(0, 0, cl.player1->eyeheight)));
				else if (oldchk < BOTCHECKCOORD && d->botcheck >= BOTCHECKCOORD) cl.suicide(d); // give up the ghost
			}
			else d->botcheck = 0;
			
		}
		else d->botcheck = 0;
		
	}
	else
	{ 
		d->move = d->strafe = 0;
		d->jumpnext = false;

		if (cl.lastmillis - d->lastpain < 2000)
		{ 
			cl.ph.move(d, d->botflags & BOT_PLAYER ? 10 : 2, false);
		}
	}
	d->lastupdate = cl.lastmillis;
}

bool home(fpsent *d, int t)
{ 
	if (cl.et.ents.inrange(t))
	{
		d->bottarg = t;
		d->botvec = cl.et.ents[t]->o;
		trans(d, M_HOME, 1, 0, true, BOTCHECKCOORD);
		return true;
	}
	return false;
}

bool enemy(fpsent *d, fpsent *p, bool ok = false)
{ 
	int gamemode = cl.gamemode;

	if (((!botdrop && gamemode != 1) || p != cl.player1) && d != NULL && d->state == CS_ALIVE && p != NULL &&
		d != p && p->state == CS_ALIVE && !isteam(d->team, p->team) &&
		(!m_sp || (d->type != p->type)))
	{
		bool force = false;

		vec target;
		vec po(p->o.x, p->o.y, p->o.z);

		d->botenemy = p;
		wayfind(d, vec(d->botenemy->o).sub(vec(0, 0, d->botenemy->eyeheight)), PATH_AVOID|PATH_GTONE|PATH_BUTONE);

		if (d->botroute.length() &&
				(d->botnode == d->botroute.last() ||
				 (d->botroute.length() > 1 && d->botnode == d->botroute[d->botroute.length() - 2])))
		{
			if (cl.lastmillis-gunvar(d->gunlast,d->gunselect) >= gunvar(d->gunwait,d->gunselect))
			{
				force = true;
			}
		}

		if ((force || getsight((physent *)d, po, target, BOTLOSDIST(d->botrate), BOTFOVX(d->botrate), BOTFOVY(d->botrate)))
				&& cl.lastmillis-gunvar(d->gunlast,d->gunselect) >= gunvar(d->gunwait,d->gunselect))
		{
			if (!d->jumpnext && !d->timeinair &&
				d->botjump >= cl.lastmillis && !(rnd(d->botrate/10)))
			{
				d->jumpnext = true;
			}
			d->botvec = vec(d->botenemy->o).sub(vec(0, 0, d->botenemy->eyeheight));

			weapon(d); // choose most appropriate weapon
			aimat(d);
			aim(d);
			return true;
		}

		return ok;
	}

	if (d->botenemy != NULL)
	{
		d->botroute.setsize(0);
		d->botenemy = NULL;
	}

	return false;
}

void aim(fpsent *d)
{ 
	if (d->botenemy)
	{
		d->botvec = vec(d->botenemy->o).sub(vec(0, 0, d->eyeheight));

		if (d->yaw == d->botrot.y && cl.lastmillis-gunvar(d->gunlast,d->gunselect) >= gunvar(d->gunwait,d->gunselect))
		{
			trans(d, M_ATTACKING, 0, 0, true, d->botrate); // that's it, we're committed
		}
		else
		{
			bool c = d->o.dist(d->botenemy->o) <= BOTRADIALDIST;
			trans(d, M_AIMING, c ? -1 : 0, c ? rnd(3) - 1 : 0, true, d->botrate);
		}
	}
	else
	{
		coord(d, 0);
	}
}

void attack(fpsent *d)
{ 
	if (d->botenemy)
	{
		d->botvec = vec(d->botenemy->o).sub(vec(0, 0, d->eyeheight));

		if (d->yaw == d->botrot.y && cl.lastmillis-gunvar(d->gunlast,d->gunselect) >= gunvar(d->gunwait,d->gunselect))
		{
			target(d);
			shoot(d, false);
			coord(d, 0);
		}
		else
		{
			aim(d);
		}
	}
	else
	{
		coord(d, 0);
	}
}

void scout(fpsent *d)
{ 
	int gamemode = cl.gamemode;

	if (botdrop || m_mp(gamemode) || !wayfind(d, cl.player1->o, PATH_AVOID))
	{
		int f = -1;

		loopv(cl.et.ents) // try to scout to the furthest side of the level
		{
			if (cl.et.ents[i]->type == WAYPOINT &&
					(!cl.et.ents.inrange(f) || cl.et.ents[i]->o.dist(d->o) > cl.et.ents[f]->o.dist(d->o)))
			{
				f = i;
			}
		}

		if (cl.et.ents.inrange(f) && wayfind(d, cl.et.ents[f]->o, PATH_ABS | PATH_AVOID))
			return ;

		d->botroute.setsize(0);

		int node = d->botnode; // stuff it, plot out a random route

		loopi(BOTSCOUTDIST)
		{
			if (cl.et.ents.inrange(node) && cl.et.ents[node]->type == WAYPOINT)
			{
				fpsentity &e = (fpsentity &)*cl.et.ents[node];
				int q = e.links.length();
				vec target;

				node = -1;

				if (q > 1)
				{
					int n = rnd(q);

					// try a random node
					if (e.links.inrange(n) && cl.et.ents.inrange(e.links[n]) && d->botroute.find(e.links[n]) < 0)
					{
						node = e.links[n];
						d->botroute.add(node);
					}
					else
					{
						// resort to a loop and find one
						for (n = 0; n < q; n++)
						{
							if (e.links.inrange(n) && cl.et.ents.inrange(e.links[n]) && d->botroute.find(e.links[n]) < 0)
							{
								node = e.links[n];
								d->botroute.add(node);
								break;
							}
						}
					}
				}
				else if (q > 0 && d->botroute.find(e.links[0]) < 0)
				{
					node = e.links[0];
					d->botroute.add(node); // looks like there's only one choice
				}
			}
			else
				break;
		}
	}
}

void coord(fpsent *d, int retry, bool doreset = false)
{ 
	if (doreset) reset(d, true);

	if (d->botenemy != NULL)
	{
		if (enemy(d, d->botenemy, false)) return;
	}
	else if (d->health > d->botrate)
	{
		find(d, true);
	}

	if (cl.et.ents.inrange(d->botnode) && cl.et.ents[d->botnode]->type == WAYPOINT)
	{
		if (!d->botroute.length() || d->botnode == d->botroute.last())
		{
			d->botroute.setsize(0);
			if (!find(d)) scout(d);
		}

		if (d->botroute.length())
		{
			int last = -1;

			loopv(d->botroute)
			{
				if (cl.et.ents.inrange(last))
				{
					if (home(d, d->botroute[i])) return;
				}
				else if (d->botroute[i] == d->botnode)
				{
					last = d->botroute[i];
				}
			}
		}

		d->botroute.setsize(0);
		if (!find(d)) scout(d);
	}

	if (retry > 3)
	{
		if (d->botstate != M_SLEEP || d->botupdate <= 0)
		{
			trans(d, M_SLEEP, 0, 0, true, BOTCHECKCOORD);
		}
	}
	else
	{
		coord(d, retry + 1);
	}
}

bool find(fpsent *d, bool enemyonly = false)
{ 
	#define BOTFINDAMT 3
	int gamemode = cl.gamemode;
	vec target;
	float dist[BOTFINDAMT], c;
	int b, lvl = -1, targ[BOTFINDAMT], amt[BOTFINDAMT]; //, inbase = -1;
	
	#define inittarg(n) \
	if (n >= 0) \
	{ \
		lvl++; \
		dist[lvl] = 1e16f; \
		targ[lvl] = -1; \
		amt[lvl] = n; \
	}
	
	#define settarg(a,b) \
	if (a <= dist[lvl]) \
	{ \
		dist[lvl] = a; \
		targ[lvl] = b; \
	}
	
	#define disttarg(q,r) \
	if (q && q->state == CS_ALIVE && ((physent *)d) != ((physent *)q) && !isteam(d->team, q->team)) \
	{ \
		int bias = 1; \
		if (getsight((physent *)d, q->o, target, BOTLOSDIST(d->botrate), BOTFOVX(d->botrate), BOTFOVY(d->botrate))) bias += 1; \
		else if (enemyonly) bias -= 1; \
		if (bias) settarg(d->o.dist(q->o) / float(bias), r); \
	}
	
	if (d->botflags & BOT_PLAYER && !m_classicsp && !enemyonly)
	{
		inittarg(cl.et.ents.length()); // 0 - Entities
	
		loopv(cl.et.ents)
		{
			if (cl.et.ents[i]->spawned && cl.et.ents[i]->type == WEAPON && d->canpickup(cl.et.ents[i]->attr1))
			{
				settarg(cl.et.ents[i]->o.dist(d->o) / float(5), i);
			}
		}
	}
	else
	{
		inittarg(0);
	}
	
	if (m_mp(gamemode))
	{
		if (!botdrop && gamemode != 1)
		{
			inittarg(1); // 1 - Player
			disttarg(cl.player1, 0);
		}
		else inittarg(0);
	
		inittarg(cl.players.length()); // 2 - Other Players
		loopv(cl.players)
		{
			disttarg(cl.players[i], i);
		}
	}
	else
	{
		loopk(2) inittarg(0);
	}
	
	int a = -1;
	
	for (lvl = 0, c = 1e16f; lvl < BOTFINDAMT; lvl++)
	{ // find the closest one in sight
		if ((targ[lvl] >= 0) && (targ[lvl] < amt[lvl]) && (dist[lvl] <= c))
		{
			a = lvl;
			c = dist[lvl];
		}
	}
	
	if ((a >= 0) && (a < BOTFINDAMT))
	{
		b = targ[a];
	
		switch (a)
		{
			case 0:
			{
				int flags = PATH_ABS | PATH_GTONE;
				if (d->health < d->botrate) flags |= PATH_AVOID;
				return wayfind(d, cl.et.ents[b]->o, flags);
			}
			case 1:
			{
				return enemy(d, cl.player1, true);
			}
			case 2:
			{
				return enemy(d, cl.players[b], true);
			}
			default:
			break;
		}
	}
	return false;
}

void action(fpsent *d)
{ 
	if ((d->botupdate -= curtime) <= 0)
	{ 
		switch (d->botstate)
		{
			case M_PAIN:
			{  // pain is used for unspawned or dead
				spawn(d);
				break;
			}

			case M_SLEEP:
			case M_SEARCH:
			case M_HOME:
			{  // take a look around
				coord(d, 0, true);
				break;
			}

			case M_AIMING:
			{ 
				aim(d);
				break;
			}

			case M_ATTACKING:
			{ 
				attack(d);
				break;
			}

			default:
				break;
		}
	}

	if (d->botupdate <= 0 && d->botstate != M_PAIN)
	{ 
		coord(d, 0);
	}
}
