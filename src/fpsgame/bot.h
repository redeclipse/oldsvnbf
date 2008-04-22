struct botclient
{
	GAMECLIENT &cl;
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
		loopvrev(cl.players)
		{
			fpsent *d = cl.players[i];
			if (d && d->ownernum >= 0 && d->ownernum == cl.player1->clientnum)
			{
				cl.cc.addmsg(SV_DELBOT, "ri", d->clientnum);
				break;
			}
		}
	}

	void update()
	{
		loopv(cl.players)
		{
			fpsent *d = cl.players[i];
			if (d && d->ownernum >= 0 && d->ownernum == cl.player1->clientnum && d->bot)
				think(d);
		}
	}

	void nextstate(fpsent *d, int type = BS_WAIT, int goal = -1, int interval = 0, bool pop = true)
	{
		bool popstate = pop && d->bot->state.length() > 1;
		if(popstate) d->bot->removestate();
		if(!popstate || (d->bot->state.last()).type == BS_NONE)
			d->bot->addstate(type, goal, lastmillis, interval);
	}

	bool search(fpsent *d)
	{
		int node = cl.player1->lastnode;
		if(cl.et.ents.inrange(node))
		{
			vector<int> avoid;
			avoid.setsize(0);
			cl.et.linkroute(d->lastnode, node, d->bot->route, avoid);
			if(d->bot->route.inrange(1))
			{
				nextstate(d, BS_MOVE, d->bot->route[1]);
				return true;
			}
		}
		return false;
	}

	bool move(fpsent *d, int goal)
	{
		if(cl.et.ents.inrange(goal))
		{
			d->move = 1;
			d->bot->target = vec(cl.et.ents[goal]->o).add(vec(0, 0, d->height-1));
			vec dir(d->bot->target);
			dir.sub(d->o);
			if(dir.z > 6.f && !d->jumping && !d->timeinair) d->jumping = true;
			dir.normalize();
			vectoyawpitch(dir, d->yaw, d->pitch);
			nextstate(d, BS_SEARCH); // move and search bounce between each other alot
			return true;
		}
		return false;
	}

	void think(fpsent *d)
	{
		botstate &bs = d->bot->state.last();

		if(lastmillis >= bs.millis+bs.interval)
		{
			switch(bs.type)
			{
				case BS_NONE: // wake up
				case BS_WAIT:
				{
					d->stopmoving();
					if(d->state == CS_DEAD)
					{
						if(d->respawned != d->lifesequence && !cl.respawnwait(d)) cl.respawnself(d);
					}
					else if(d->state == CS_ALIVE) nextstate(d, BS_SEARCH);
					break;
				}
				case BS_SEARCH:
				{
					if(d->state == CS_ALIVE && search(d)) break;
					d->stopmoving();
					nextstate(d, BS_WAIT, 0, 100);
					break;
				}
				case BS_MOVE:
				{
					if(d->state == CS_ALIVE && move(d, bs.goal)) break;
					d->stopmoving();
					nextstate(d, BS_WAIT, 0, 100);
					break;
				}
				case BS_DEFEND:
				case BS_ATTACK:
				case BS_PROTECT:
				case BS_MAX:
				default: break;
			}
		}
		d->lastupdate = lastmillis;

		cl.ph.move(d, 10, true);
		cl.ph.updatewater(d, 0);
	}

	IVAR(debugbot, 0, 0, 1);

	void render()
	{
		if(debugbot())
		{
			loopv(cl.players)
			{
				fpsent *d = cl.players[i];
				if (d && d->state == CS_ALIVE && d->ownernum >= 0 && d->ownernum == cl.player1->clientnum && d->bot)
				{
					botstate &bs = d->bot->state.last();
					const char *bnames[BS_MAX] = {
						"none", "wait", "search", "move", "defend", "attack", "protect"
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
						renderline(d->o, d->bot->target, 64.f, 128.f, 64.f, false);
						renderprimitive(false);
					}
				}
			}
		}
	}
} bot;
