// Blood Frontier - BOT - Offline test and practice AI by Quinton Reeves
// Included by: fpsgame/bot/bot.h
// This is the core of the waypoint navigational system used by the bots.

int waynode(vec &v, fpsent *d = NULL)
{ 
	int w = -1;

	loopv(cl.et.ents)
	{
		if (cl.et.ents[i]->type == WAYPOINT &&
			(!d || !cl.et.ents.inrange(d->bottarg) || i != d->bottarg || cl.et.ents[i]->o.dist(v) <= BOTISNEAR) &&
			(!cl.et.ents.inrange(w) || cl.et.ents[i]->o.dist(v) < cl.et.ents[w]->o.dist(v)))
		{
			w = i;
		}
	}
	return w;
}

void wayspawn()
{ 
	loopv(cl.et.ents)
	{
		switch (cl.et.ents[i]->type)
		{
			case I_SHELLS:
			case I_BULLETS:
			case I_ROCKETS:
			case I_ROUNDS:
			case I_GRENADES:
			case I_CARTRIDGES:
			case I_HEALTH:
			case I_BOOST:
			case I_GREENARMOUR:
			case I_YELLOWARMOUR:
			case I_QUAD:
				newentity(vec(cl.et.ents[i]->o).sub(vec(0, 0, 2)), WAYPOINT, 0, 0, 0, 0);
				break;
			case TELEPORT:
			case TELEDEST:
			case JUMPPAD:
			case BASE:
				newentity(vec(cl.et.ents[i]->o).add(vec(0, 0, 2)), WAYPOINT, 0, 0, 0, 0);
				break;
		}
	}
}

void waylink(int n, int m)
{ 
	if (n != m && cl.et.ents.inrange(n) && cl.et.ents.inrange(m) &&
			cl.et.ents[n]->type == WAYPOINT && cl.et.ents[m]->type == WAYPOINT)
	{
		fpsentity &e = (fpsentity &)*cl.et.ents[n];
		if (e.links.find(m) < 0)
			e.links.add(m);
	}
}

void wayposition(fpsent *d)
{
	if (d->state == CS_ALIVE)
	{
		if (botdrop && !isbot(d))
		{
			int oldnode = d->botnode;
			vec v(vec(d->o).sub(vec(0, 0, d->eyeheight)));
			
			loopv(cl.et.ents)
			{
				if (cl.et.ents[i]->type == WAYPOINT && cl.et.ents[i]->o.dist(v) <= botwaynear &&
					(!cl.et.ents.inrange(d->botnode) ||
						cl.et.ents[i]->o.dist(v) < cl.et.ents[d->botnode]->o.dist(v)))
				{
					d->botnode = i;
				}
			}
			
			if (!cl.et.ents.inrange(d->botnode) ||
				cl.et.ents[d->botnode]->o.dist(v) >= botwaydist)
			{
				d->botnode = cl.et.ents.length();
				newentity(v, WAYPOINT, 0, 0, 0, 0);
			}

			if (d->botnode != oldnode && cl.et.ents.inrange(oldnode)
				&& cl.et.ents.inrange(d->botnode))
			{
				waylink(oldnode, d->botnode);
				if (!d->timeinair) waylink(d->botnode, oldnode);
			}
		}
		else
		{
			loopi(3)
			{
				if ((d->botnode =
					waynode(vec(d->o).sub(vec(0, 0, d->eyeheight)), d)) >= 0)
						break;
				
				reset(d, i ? true : false);
			}
		}
		
		avoid.add(d->botnode);
	}
	else
	{
		d->botnode = -1;
	}
}

void wayupdpos(fpsent *d, vec &o)
{ 
	if (!cl.intermission && botdrop && d->state == CS_ALIVE && !isbot(d))
	{
		if (cl.et.ents.inrange(d->botnode))
		{
			int from = waynode(o);

			if (cl.et.ents.inrange(from))
			{
				waylink(d->botnode, from);
				d->botnode = from;
			}
		}
	}
}

struct nodeq
{
	float weight, goal;
	bool dead;
	vector<int> nodes;

	nodeq() : weight(0.f), goal(0.f), dead(false) {}
	~nodeq() {}
};

bool wayfind(fpsent *d, vec &target, int flags = 0)
{ 
	bool result = false;

	d->botroute.setsize(0);

	if (cl.et.ents.inrange(d->botnode) && cl.et.ents[d->botnode]->type == WAYPOINT)
	{
		int goal = waynode(target);
		fpsentity &f = (fpsentity &)*cl.et.ents[d->botnode];

		if (cl.et.ents.inrange(goal) && cl.et.ents[goal]->type == WAYPOINT)
		{
			fpsentity &g = (fpsentity &)*cl.et.ents[goal];

			vector<nodeq *> queue;
			int q = 0;

			queue.add(new nodeq());
			queue[q]->nodes.add(d->botnode);
			queue[q]->goal = f.o.dist(g.o);

			while (queue.inrange(q) && queue[q]->nodes.last() != goal)
			{
				fpsentity &e = (fpsentity &)*cl.et.ents[queue[q]->nodes.last()];

				float w = queue[q]->weight;
				vector<int> s = queue[q]->nodes;
				int a = 0;

				loopvj(e.links)
				{
					int v = e.links[j];

					if (cl.et.ents.inrange(v) && cl.et.ents[v]->type == WAYPOINT)
					{
						bool skip = false;

						loopvk(queue)
						{
							if (queue[k]->nodes.find(v) >= 0 ||
									((flags & PATH_AVOID) && v != goal && avoid.find(v) >= 0) ||
									((flags & PATH_GTONE) && v == goal && queue.length() == 1))
							{
								skip = true;
								break;
							}
						} // don't revisit shorter noded paths
						if (!skip)
						{
							fpsentity &h = (fpsentity &)*cl.et.ents[v];

							int r = q; // continue this line for the first one

							if (a)
							{
								r = queue.length();
								queue.add(new nodeq());
								queue[r]->nodes = s;
								queue[r]->weight = w;
							}
							queue[r]->nodes.add(v);
							queue[r]->weight += e.o.dist(h.o);
							queue[r]->goal = h.o.dist(g.o);
							a++;
						}
					}
				}
				if (!a)
				{
					queue[q]->dead = true;
				} // this one ain't going anywhere..

				q = -1; // get shortest path
				loopvj(queue)
				{
					if (!queue[j]->dead &&
						(!queue.inrange(q) || queue[j]->weight + queue[j]->goal < queue[q]->weight + queue[q]->goal))
						q = j;
				}
			}

			if (!queue.inrange(q) && !(flags & PATH_ABS)) // didn't get there, resort to failsafe proximity match
			{
				q = -1;

				loopvj(queue)
				{
					int u = -1;

					loopvrev(queue[j]->nodes) // find the closest node in this branch
					{
						if (!queue[j]->nodes.inrange(u) || cl.et.ents[queue[j]->nodes[i]]->o.dist(g.o) < cl.et.ents[queue[j]->nodes[u]]->o.dist(g.o))
							u = i;
					}

					if (queue[j]->nodes.inrange(u))
					{
						loopvrev(queue[j]->nodes) // trim the node list to the end at the shortest
						{
							if (i <= u)
								break;
							queue[j]->nodes.remove(i);
						}

						if (!queue.inrange(q) || cl.et.ents[queue[j]->nodes[u]]->o.dist(g.o) < cl.et.ents[queue[q]->nodes.last()]->o.dist(g.o))
							q = j;
					}
				}
			}

			if (queue.inrange(q))
			{
				if (flags & PATH_BUTONE && queue[q]->nodes.length() > 1) queue[q]->nodes.remove(queue[q]->nodes.length()-1);
				d->botroute = queue[q]->nodes;
				result = true;
			}

			loopv(queue) DELETEP(queue[i]); // purge
		}

		if (!result && !(flags & PATH_ABS)) // random search
		{
			for (int c = d->botnode; cl.et.ents.inrange(c) && cl.et.ents[c]->type == WAYPOINT; )
			{
				fpsentity &e = (fpsentity &)*cl.et.ents[c];
				int bwp = -1;

				loopv(e.links)
				{
					int node = e.links[i];

					if (d->botroute.find(node) < 0 && (!(flags & PATH_AVOID) || avoid.find(node) >= 0))
						if (!cl.et.ents.inrange(bwp) ||
								(cl.et.ents.inrange(node) && cl.et.ents[node]->o.dist(target) < cl.et.ents[bwp]->o.dist(target)))
							bwp = node;
				}
				if (cl.et.ents.inrange(bwp))
				{
					d->botroute.add(bwp);
				}
				c = bwp;
			}
			if (d->botroute.length())
				result = true;
		}
	}
	if (!result && flags)
	{
		if (flags & PATH_AVOID)
		{
			flags &= ~PATH_AVOID;
			return wayfind(d, target, flags);
		}
	}
	return result;
}
