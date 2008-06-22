struct botclient
{
	GAMECLIENT &cl;

	static const int BOTISNEAR		= 8;			// is near
	static const int BOTISFAR		= 96;			// too far
	static const int BOTJUMPHEIGHT	= 6;			// decides to jump
	static const int BOTJUMPIMPULSE	= 24;			// impulse to jump
	static const int BOTJUMPMAX		= 32;			// too high
	static const int BOTLOSRANGE	= 514;			// line of sight range
	static const int BOTFOVRANGE	= 128;			// field of view range

	#define BOTLOSDIST(x)			(BOTLOSRANGE-(x*2.0f))
	#define BOTFOVX(x)				(BOTFOVRANGE-(x*0.5f))
	#define BOTFOVY(x)				((BOTFOVRANGE*3/4)-(x*0.5f))

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

	fpsent *getclosestplayer(fpsent *d)
	{
		fpsent *closest = cl.player1->state == CS_ALIVE ? cl.player1 : NULL;
		loopv(cl.players)
			if(cl.players[i] && cl.players[i]->state == CS_ALIVE &&
				(!closest || cl.players[i]->o.dist(d->o) < closest->o.dist(d->o)))
					closest = cl.players[i];
		return closest;
	}

	bool doreset(fpsent *d)
	{
		d->stopmoving();
		d->bot->removestate();
		return true;
	}

	bool doupdate(fpsent *d)
	{
		fpsent *closest = getclosestplayer(d);
		if(closest)
		{
			d->bot->setstate(BS_PURSUE, closest->clientnum);
			return true;
		}
		return false;
	}

	bool domove(fpsent *d, int goal)
	{
		if(cl.et.ents.inrange(goal))
		{
			d->move = 1;
			d->bot->targpos = vec(cl.et.ents[goal]->o).add(vec(0, 0, d->height-1));
			vec dir(d->bot->targpos);
			dir.sub(d->o);
			if(dir.z > BOTJUMPHEIGHT && !d->jumping && !d->timeinair) d->jumping = true;
			dir.normalize();
			vectoyawpitch(dir, d->yaw, d->pitch);
			d->aimyaw = d->yaw;
			d->aimpitch = d->pitch;
			d->bot->removestate(); // bounce back to the parent
			return true;
		}
		return false;
	}

	bool dopursue(fpsent *d, int goal)
	{
		fpsent *enemy = goal == cl.player1->clientnum ? cl.player1 : cl.getclient(goal);

		if(enemy && enemy->state == CS_ALIVE && getclosestplayer(d) == enemy)
		{
			int node = enemy->lastnode;
			if(cl.et.ents.inrange(node))
			{
				vector<int> avoid;
				avoid.setsize(0);
				cl.et.linkroute(d->lastnode, node, d->bot->route, avoid);
				if(d->bot->route.inrange(1))
				{
					d->bot->setstate(BS_MOVE, d->bot->route[1]);
					return true;
				}
			}
		}
		return false;
	}

	void think(fpsent *d)
	{
		botstate &bs = d->bot->curstate();

		// the state stack works like a chain of commands, certain commands
		// simply replace each other, others spawn new commands to the stack
		// the ai reads the top command from the stack and executes it or
		// pops the stack and goes back along the history until it finds a
		// suitable command to execute

		if(lastmillis >= bs.millis+bs.interval)
		{
			switch(bs.type)
			{
				case BS_WAIT:
				{
					d->stopmoving();
					if(d->state == CS_DEAD)
					{
						if(d->respawned != d->lifesequence && !cl.respawnwait(d))
							cl.respawnself(d);
					}
					else if(d->state == CS_ALIVE) d->bot->setstate(BS_UPDATE);
					break;
				}
				case BS_UPDATE:
				{
					if(d->state == CS_ALIVE && doupdate(d)) break;
					doreset(d);
					break;
				}
				case BS_MOVE:
				{
					if(d->state == CS_ALIVE && domove(d, bs.goal)) break;
					doreset(d);
					break;
				}
				case BS_DEFEND:
				case BS_PURSUE:
					if(d->state == CS_ALIVE && dopursue(d, bs.goal)) break;
					doreset(d);
					break;
				case BS_ATTACK:
				case BS_INTEREST:
				case BS_MAX:
				default: doreset(d); break;
			}
		}
		d->lastupdate = lastmillis;

		cl.ph.move(d, 10, true);
	}

	IVAR(debugbot, 0, 0, 1);

	void render()
	{
		if(!debugbot()) return;
		loopv(cl.players) if(cl.players[i] && cl.players[i]->state == CS_ALIVE)
		{
			fpsent *d = cl.players[i];
			if (d->ownernum >= 0 && d->ownernum == cl.player1->clientnum && d->bot)
			{
				botstate &bs = d->bot->state.last();
				const char *bnames[BS_MAX] = {
					"wait", "update", "move", "defend", "pursue", "attack", "interest"
				};
				s_sprintfd(s)("@%s [%d] (%.2f)", bnames[bs.type], bs.goal, max((bs.millis+bs.interval-lastmillis)/1000.f, 0.f));
				particle_text(vec(d->abovehead()).add(vec(0, 0, 2)), s, 14, 1);

				if(rendernormally)
				{
					renderprimitive(true);
					int last = -1;
					loopv(d->bot->route)
					{
						if (d->bot->route.inrange(last))
						{
							int index = d->bot->route[i], prev = d->bot->route[last];

							if (cl.et.ents.inrange(index) && cl.et.ents.inrange(prev))
							{
								fpsentity &e = (fpsentity &) *cl.et.ents[index], &f = (fpsentity &) *cl.et.ents[prev];
								renderline(vec(f.o).add(vec(0, 0, 1)), vec(e.o).add(vec(0, 0, 1)), 128.f, 64.f, 64.f, false);
							}
						}
						last = i;
					}
					renderline(d->o, d->bot->targpos, 64.f, 128.f, 64.f, false);
					renderprimitive(false);
				}
			}
		}
	}
} bot;
