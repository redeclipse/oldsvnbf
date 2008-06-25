struct botclient
{
	GAMECLIENT &cl;

	static const int BOTISNEAR		= 8;			// is near
	static const int BOTISFAR		= 96;			// too far
	static const int BOTJUMPHEIGHT	= 8;			// decides to jump
	static const int BOTJUMPIMPULSE	= 24;			// impulse to jump
	static const int BOTJUMPMAX		= 32;			// too high
	static const int BOTLOSRANGE	= 514;			// line of sight range
	static const int BOTFOVRANGE	= 128;			// field of view range

	IVAR(botskill, 1, 1, 100);
	IVAR(botdebug, 0, 2, 4);

	#define BOTRATE					(101-botskill())
	#define BOTLOSDIST				(BOTLOSRANGE-(BOTRATE*2.0f))
	#define BOTFOVX					(BOTFOVRANGE-(BOTRATE*0.5f))
	#define BOTFOVY					((BOTFOVRANGE*3/4)-(BOTRATE*0.5f))
	#define BOTWAIT(x,y)			(clamp(rnd(x*BOTRATE)+rnd(y*BOTRATE), y*BOTRATE, (x+y)*BOTRATE))

	botclient(GAMECLIENT &_cl) : cl(_cl)
	{
		CCOMMAND(addbot, "", (botclient *self), self->addbot());
		CCOMMAND(delbot, "", (botclient *self), self->delbot());
	}

	void addbot()
	{
		cl.cc.addmsg(SV_ADDBOT, "r");
	}

	void delbot()
	{
		loopvrev(cl.players) if(cl.players[i])
		{
			fpsent *d = cl.players[i];
			if (d->ownernum >= 0 && d->ownernum == cl.player1->clientnum)
			{
				cl.cc.addmsg(SV_DELBOT, "ri", d->clientnum);
				break;
			}
		}
	}

	void update()
	{
		loopv(cl.players) if(cl.players[i])
		{
			fpsent *d = cl.players[i];
			if(d->ownernum >= 0 && d->ownernum == cl.player1->clientnum && d->bot)
				think(d);
		}
	}

	bool hastarget(fpsent *d)
	{
		float cyaw = fabs(fabs(d->bot->targyaw)-fabs(d->yaw)),
			cpitch = fabs(fabs(d->bot->targpitch)-fabs(d->pitch)),
			amt = BOTRATE*0.25f;
		if (cyaw <= amt && cpitch <= amt) return true;
		return false;
	}

	float disttonode(fpsent *d, int node, vector<int> &route)
	{
		if(cl.et.ents.inrange(node))
		{
			vector<int> avoid;
			return cl.et.linkroute(d->lastnode, node, route, avoid, ROUTE_GTONE|ROUTE_ABS);
		}
		return 1e16f;
	}

	void doreset(fpsent *d, bool full = true)
	{
		if(full)
		{
			d->stopmoving();
			d->bot->reset();
		}
		else d->bot->removestate();
	}

	void dowait(fpsent *d, botstate &bs)
	{
		d->stopmoving();

		if(d->state == CS_DEAD)
		{
			if(d->respawned != d->lifesequence && !cl.respawnwait(d))
				cl.respawnself(d);
		}
		else if(d->state == CS_ALIVE)
		{
			botstate &bt = d->bot->addstate(BS_UPDATE, BOTWAIT(10, 5));
			bt.targpos = d->o;
			d->bot->targyaw = d->yaw;
			d->bot->targpitch = d->pitch;
		}
	}

	bool doupdate(fpsent *d, botstate &bs)
	{
		if(d->state == CS_ALIVE)
		{
			int state = BS_WAIT;

			bs.dist = 1e16f;
			d->stopmoving();

			#define getclosestnode(n, s, t, o) \
			{ \
				vector<int> proute; \
				float pdist = disttonode(d, n, proute); \
				if(pdist < bs.dist) \
				{ \
					state = s; \
					bs.target = t; \
					bs.targpos = o; \
					bs.dist = pdist; \
					bs.route = proute; \
				} \
			}

			loopv(cl.et.ents)
			{
				fpsentity &e = (fpsentity &)*cl.et.ents[i];
				switch(e.type)
				{
					case WEAPON:
					{
						if(e.spawned && e.attr1 != GUN_GL && d->ammo[e.attr1] <= 0 && e.attr1 > d->bestgun(lastmillis))
							getclosestnode(i, BS_INTEREST, i, vec(e.o).add(vec(0, 0, enttype[e.type].height)));
						break;
					}
					default: break;
				}
			}

			if(cl.player1->state == CS_ALIVE)
				getclosestnode(cl.player1->lastnode, BS_PURSUE, cl.player1->clientnum, cl.player1->o);
			loopv(cl.players)
				if(cl.players[i] && cl.players[i] != d && cl.players[i]->state == CS_ALIVE)
					getclosestnode(cl.players[i]->lastnode, BS_PURSUE, cl.players[i]->clientnum, cl.players[i]->o);

			if(state == BS_WAIT) // scout
			{
				bs.target = -1;
				bs.dist = 0.f;

				loopv(cl.et.ents)
				{
					fpsentity &e = (fpsentity &)*cl.et.ents[i];
					if(e.type == WAYPOINT && e.o.dist(d->o) > bs.dist)
					{
						bs.target = i;
						bs.dist = e.o.dist(d->o);
					}
				}

				if(cl.et.ents.inrange(bs.target))
				{
					fpsentity &e = (fpsentity &)*cl.et.ents[bs.target];
					state = BS_INTEREST;
					bs.dist = disttonode(d, bs.target, bs.route);
					bs.targpos = vec(e.o).add(vec(0, 0, enttype[e.type].height));
				}
			}

			if(state != BS_WAIT)
			{
				botstate &bt = d->bot->addstate(state, BOTWAIT(5, 2), BOTRATE);
				bt.route = bs.route;
				bt.target = bs.target;
				bt.targpos = bs.targpos;
				bt.dist = bs.dist;
				return true;
			}
		}
		return false;
	}

	bool domove(fpsent *d, botstate &bs)
	{
		if(d->state == CS_ALIVE)
		{
			d->stopmoving();
			vec off(vec(bs.targpos).sub(d->o));

			if(off.z > BOTJUMPHEIGHT && !d->timeinair)
				d->jumping = true;

			d->move = 1;
			d->bot->removestate(); // bounce back to the parent
			return true;
		}
		return false;
	}

	bool doattack(fpsent *d, botstate &bs)
	{
		if(d->state == CS_ALIVE)
		{
			d->stopmoving();

			if(hastarget(d))
			{
				d->attacking = true;
				d->attacktime = lastmillis;
				d->bot->removestate(); // bounce back to the parent
			}
			return true;
		}
		return false;
	}

	bool wantdefer(fpsent *d, botstate &bs, fpsent *e, vec &target)
	{
		if(getsight((physent *)d, e->o, target, BOTLOSDIST, BOTFOVX, BOTFOVY) &&
			d->canshoot(d->gunselect, lastmillis)) return true;
		return false;
	}

	bool dodefer(fpsent *d, botstate &bs)
	{
		int target = -1;
		float dist = 1e16f;
		vec targpos, targ;

		if(cl.player1->state == CS_ALIVE && cl.player1->o.dist(d->o) < dist && wantdefer(d, bs, cl.player1, targ))
		{
			target = cl.player1->clientnum;
			targpos = targ;
		}
		loopv(cl.players)
		{
			if(cl.players[i] && cl.players[i]->state == CS_ALIVE && cl.players[i]->o.dist(d->o) < dist && wantdefer(d, bs, cl.players[i], targ))
			{
				target = cl.players[i]->clientnum;
				targpos = targ;
			}
		}

		if(target >= 0)
		{
			botstate &bt = d->bot->addstate(BS_ATTACK, BOTWAIT(6, 4), BOTRATE/10);
			bs.targpos = bt.targpos = targpos;
			bt.target = target;
			bt.route = bs.route;
			bt.dist = bs.dist;
			return true;
		}
		return false;
	}

	bool dohunt(fpsent *d, botstate &bs)
	{
		int node = bs.route.find(d->lastnode);
		if(bs.route.inrange(node) && bs.route.inrange(node+1))
		{
			botstate &bt = d->bot->addstate(BS_MOVE);
			bs.targpos = bt.targpos = vec(cl.et.ents[bs.route[node+1]]->o).add(vec(0, 0, d->height-1));
			bt.target = bs.target;
			bt.route = bs.route;
			bt.dist = bs.dist;
			return true;
		}
		return false;
	}

	bool dointerest(fpsent *d, botstate &bs)
	{
		if(d->state == CS_ALIVE)
		{
			if(dodefer(d, bs)) return true;
			if(cl.et.ents.inrange(bs.target))
			{
				fpsentity &e = (fpsentity &)*cl.et.ents[bs.target];
				float eye = d->height*0.5f;
				vec m = d->o;
				m.z -= eye;

				switch(e.type)
				{
					case WEAPON:
					{
						if(!e.spawned || d->ammo[e.attr1] > 0 || e.attr1 <= d->bestgun(lastmillis))
							return false;
						break;
					}
					default: break;
				}

				if(insidesphere(m, eye, d->radius, e.o, enttype[e.type].height, enttype[e.type].radius))
				{
					if(enttype[e.type].usetype == ETU_ITEM && d->canuse(e.type, e.attr1, e.attr2, lastmillis))
					{
						d->useaction = true;
						d->usetime = lastmillis;
					}
					d->bot->removestate();
					return true;
				}

				if(dohunt(d, bs)) return true;
			}
		}
		return false;
	}

	bool dopursue(fpsent *d, botstate &bs)
	{
		if(d->state == CS_ALIVE)
		{
			if(dodefer(d, bs)) return true;
			fpsent *e = bs.target == cl.player1->clientnum ? cl.player1 : cl.getclient(bs.target);
			if(e && e->state == CS_ALIVE && dohunt(d, bs)) return true;
		}
		return false;
	}

	void doaim(fpsent *d, botstate &bs)
	{
		float amt = float(lastmillis-d->lastupdate)/float(BOTRATE);

		d->bot->targyaw = -(float)atan2(bs.targpos.x-d->o.x, bs.targpos.y-d->o.y)/PI*180+180;

		if (d->yaw < d->bot->targyaw-180.0f) d->yaw += 360.0f;
		if (d->yaw > d->bot->targyaw+180.0f) d->yaw -= 360.0f;

		float dist = d->o.dist(bs.targpos);
		d->bot->targpitch = asin((bs.targpos.z-d->o.z)/dist)/RAD;

		if (d->bot->targpitch > 90.f) d->bot->targpitch = 90.f;
		if (d->bot->targpitch < -90.f) d->bot->targpitch = -90.f;

		float offyaw = fabs(d->bot->targyaw-d->yaw), offpitch = fabs(d->bot->targpitch-d->pitch);

		if (d->bot->targyaw > d->yaw)    // slowly turn bot towards target
		{
			d->yaw += amt*offyaw;
			if (d->bot->targyaw < d->yaw) d->yaw = d->bot->targyaw;
		}
		else if (d->bot->targyaw < d->yaw)
		{
			d->yaw -= amt*offyaw;
			if (d->bot->targyaw > d->yaw) d->yaw = d->bot->targyaw;
		}
		if (d->bot->targpitch > d->pitch)
		{
			d->pitch += amt*offpitch;
			if (d->bot->targpitch < d->pitch) d->pitch = d->bot->targpitch;
		}
		else if (d->bot->targpitch < d->pitch)
		{
			d->pitch -= amt*offpitch;
			if (d->bot->targpitch > d->pitch) d->pitch = d->bot->targpitch;
		}

		cl.fixrange(d->yaw, d->pitch);
		findorientation(d->o, d->yaw, d->pitch, d->bot->targpos);
		d->aimyaw = d->yaw;
		d->aimpitch = d->pitch;
	}

	void think(fpsent *d)
	{
		if(!d->bot->state.length()) doreset(d, true);

		// the state stack works like a chain of commands, certain commands
		// simply replace each other, others spawn new commands to the stack
		// the ai reads the top command from the stack and executes it or
		// pops the stack and goes back along the history until it finds a
		// suitable command to execute

		botstate &bs = d->bot->getstate();
		int secs = lastmillis - bs.millis;

		if(d->state == CS_ALIVE && bs.type != BS_WAIT)
			doaim(d, bs);

		if(secs >= bs.interval)
		{
			bool result = false;
			switch(bs.type)
			{
				case BS_WAIT: result = true; dowait(d, bs); break;
				case BS_UPDATE: result = doupdate(d, bs); break;
				case BS_MOVE: result = domove(d, bs); break;
				case BS_PURSUE: result = dopursue(d, bs); break;
				case BS_ATTACK: result = doattack(d, bs); break;
				case BS_INTEREST: result = dointerest(d, bs); break;
				case BS_DEFEND: default: break;
			}
			bs.millis = lastmillis;
			bs.cycle++;
			if(!result || (bs.cycles && bs.cycle >= bs.cycles))
				doreset(d, false);
		}
		cl.ph.move(d, 10, true);

		if(d->state == CS_ALIVE && bs.type != BS_WAIT)
		{
			cl.et.checkitems(d);
			cl.ws.shoot(d, d->bot->targpos);
			if(d->gunselect != d->bestgun(lastmillis) && d->canswitch(d->bestgun(lastmillis), lastmillis))
			{
				d->setgun(d->bestgun(lastmillis), lastmillis);
				cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
				cl.playsoundc(S_SWITCH, d);
			}
			else if(d->ammo[d->gunselect] <= 0 && d->canreload(d->gunselect, lastmillis))
			{
				d->gunreload(d->gunselect, guntype[d->gunselect].add, lastmillis);
				cl.cc.addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
				cl.playsoundc(S_RELOAD, d);
			}
		}
		d->lastupdate = lastmillis;
	}

	void drawstate(fpsent *d, botstate &bs, bool top, int above)
	{
		const char *bnames[BS_MAX] = {
			"wait", "update", "move", "defend", "pursue", "attack", "interest"
		};
		s_sprintfd(s)("@%s%s [%d:%d] (%.2f)", top ? "\fy" : "\fw", bnames[bs.type], bs.goal(), bs.target, max((bs.millis+bs.interval-lastmillis)/1000.f, 0.f));
		particle_text(vec(d->abovehead()).add(vec(0, 0, above)), s, 14, 1);
	}

	void drawroute(fpsent *d, botstate &bs)
	{
		renderprimitive(true);
		int last = -1;
		loopv(bs.route)
		{
			if (bs.route.inrange(last))
			{
				int index = bs.route[i], prev = bs.route[last];

				if (cl.et.ents.inrange(index) && cl.et.ents.inrange(prev))
				{
					fpsentity &e = (fpsentity &) *cl.et.ents[index], &f = (fpsentity &) *cl.et.ents[prev];
					renderline(vec(f.o).add(vec(0, 0, 1)), vec(e.o).add(vec(0, 0, 1)), 128.f, 64.f, 64.f, false);
				}
			}
			last = i;
		}
		if(botdebug() > 3)
			renderline(vec(d->o).sub(vec(0, 0, d->height*0.5f)), bs.targpos, 64.f, 128.f, 64.f, false);
		renderprimitive(false);
	}

	void render()
	{
		if(botdebug())
		{
			loopv(cl.players) if(cl.players[i] && cl.players[i]->state == CS_ALIVE && cl.players[i]->bot)
			{
				fpsent *d = cl.players[i];
				bool top = true;
				int above = 0;
				loopvrev(d->bot->state)
				{
					botstate &bs = d->bot->getstate(i);
					drawstate(d, bs, top, above += 2);
					if(botdebug() > 2 && top && rendernormally && bs.type != BS_WAIT)
						drawroute(d, bs);
					if(top)
					{
						if(botdebug() > 1) top = false;
						else break;
					}
				}
			}
		}
	}
} bot;
