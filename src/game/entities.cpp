#include "pch.h"
#include "pch.h"
#include "engine.h"
#include "game.h"
namespace entities
{
	vector<extentity *> ents;
	bool autodropped = false;
	int autodroptime = -1;

	VARP(showentnames, 0, 1, 2);
	VARP(showentinfo, 0, 1, 5);
	VARP(showentnoisy, 0, 0, 2);
	VARP(showentdir, 0, 1, 3);
	VARP(showentradius, 0, 1, 3);
	VARP(showentlinks, 0, 1, 3);
	VARP(showlighting, 0, 1, 1);

	VAR(dropwaypoints, 0, 0, 1); // drop waypoints during play
	VAR(autodropwaypoints, 0, 300, INT_MAX-1); // secs after map start we start and keep dropping waypoints
	FVARP(waypointmergescale, 1e-3f, 0.35f, 1000);

	vector<extentity *> &getents() { return ents; }

	int triggertime(extentity &e)
	{
		switch(e.type)
		{
			case TRIGGER: case MAPMODEL: case PARTICLES: case MAPSOUND:
				return m_speedtimex(1000); break;
			default: break;
		}
		return 0;
	}

	const char *entinfo(int type, int attr1, int attr2, int attr3, int attr4, int attr5, bool full)
	{
		static string entinfostr;
		string str;
		entinfostr[0] = 0;
		#define addentinfo(s) { \
			if(entinfostr[0]) s_strcat(entinfostr, ", "); \
			s_strcat(entinfostr, s); \
		}
		if(type == PLAYERSTART || type == FLAG)
		{
			if(valteam(attr2, TEAM_FIRST))
			{
				s_sprintf(str)("team %s", teamtype[attr2].name);
				addentinfo(str);
			}
			else if(attr2 < TEAM_MAX)
			{
				s_sprintf(str)("%s", teamtype[attr2].name);
				addentinfo(str);
			}
		}
		else if(type == WEAPON)
		{
			int sweap = m_spawnweapon(world::gamemode, world::mutators),
				attr = weapattr(attr1, sweap);
			if(isweap(attr))
			{
				s_sprintf(str)("\fs%s%s\fS", weaptype[attr].text, weaptype[attr].name);
				addentinfo(str);
				if(full)
				{
					if(attr2&WEAPFLAG_FORCED) addentinfo("forced");
				}
			}
		}
		else if(type == MAPMODEL)
		{
			if(mapmodels.inrange(attr1)) addentinfo(mapmodels[attr1].name);
			if(full)
			{
				if(attr5&MMT_HIDE) addentinfo("hide");
				if(attr5&MMT_NOCLIP) addentinfo("noclip");
				if(attr5&MMT_NOSHADOW) addentinfo("noshadow");
				if(attr5&MMT_NODYNSHADOW) addentinfo("nodynshadow");
			}
		}
		else if(type == MAPSOUND)
		{
			if(mapsounds.inrange(attr1)) addentinfo(mapsounds[attr1].sample->name);
			if(full)
			{
				if(attr5&SND_NOATTEN) addentinfo("noatten");
				if(attr5&SND_NODELAY) addentinfo("nodelay");
				if(attr5&SND_NOCULL) addentinfo("nocull");
			}
		}
		else if(type == TRIGGER)
		{
			if(full)
			{
				const char *trignames[2][4] = {
						{ "toggle", "links", "script", "" },
						{ "(disabled)", "(proximity)", "(action)" }
				};
				int tr = attr2 <= TR_NONE || attr2 >= TR_MAX ? TR_NONE : attr2,
					ta = attr3 <= TA_NONE || attr3 >= TA_MAX ? TA_NONE : attr3;
				s_sprintf(str)("execute %s %s", trignames[0][tr], trignames[1][ta]);
				addentinfo(str);
			}
		}
		else if(type == WAYPOINT)
		{
			if(full)
			{
				if(attr1 & WP_CROUCH) addentinfo("crouch");
			}
		}
		return entinfostr;
	}

	const char *entmdlname(int type, int attr1, int attr2, int attr3, int attr4, int attr5)
	{
		switch(type)
		{
			case WEAPON:
			{
				int sweap = m_spawnweapon(world::gamemode, world::mutators), attr = weapattr(attr1, sweap);
				return weaptype[attr].item;
			}
			case FLAG: return teamtype[attr2].flag;
			default: break;
		}
		return "";
	}

	// these two functions are called when the server acknowledges that you really
	// picked up the item (in multiplayer someone may grab it before you).

	void useeffects(gameent *d, int n, bool s, int g, int r)
	{
		if(ents.inrange(n))
		{
			gameentity &e = *(gameentity *)ents[n];
			vec pos = e.o;
			loopv(projs::projs)
			{
				projent &proj = *projs::projs[i];
				if(proj.projtype != PRJ_ENT || proj.id != n) continue;
				pos = proj.o;
				proj.beenused = true;
				proj.state = CS_DEAD;
			}
			gameent *f = NULL;
			loopi(world::numdynents()) if((f = (gameent *)world::iterdynents(i)) && f->type == ENT_PLAYER)
			{
				loopk(WEAPON_MAX) if(f->entid[k] == n) f->entid[k] = -1;
			}
			const char *item = entinfo(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4]);
			if(item && d != world::player1)
			{
				s_sprintfd(ds)("@%s", item);
				part_text(world::abovehead(d), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 3.f);
			}
			playsound(S_ITEMPICKUP, d->o, d);
			if(isweap(g))
			{
				d->setweapstate(g, WPSTATE_SWITCH, WEAPSWITCHDELAY, lastmillis);
				d->ammo[g] = d->entid[g] = -1;
				d->weapselect = g;
			}
			int sweap = m_spawnweapon(world::gamemode, world::mutators), attr = weapattr(e.attr[0], sweap);
			d->useitem(n, e.type, attr, e.attr[1], e.attr[2], e.attr[3], e.attr[4], sweap, lastmillis);
			if(ents.inrange(r) && ents[r]->type == WEAPON)
			{
				gameentity &f = *(gameentity *)ents[r];
				attr = weapattr(f.attr[0], sweap);
				if(isweap(attr)) projs::drop(d, attr, r, d == world::player1 || d->ai);
			}
			world::spawneffect(pos, 0x6666FF, enttype[e.type].radius);
			e.spawned = s;
		}
	}

    struct entcachenode
    {
        float split[2];
        uint child[2];

        int axis() const { return child[0]>>30; }
        int childindex(int which) const { return child[which]&0x3FFFFFFF; }
        bool isleaf(int which) const { return (child[1]&(1<<(30+which)))!=0; }
    };

    vector<entcachenode> entcache;
    int entcachedepth = -1;
    vec entcachemin(1e16f, 1e16f, 1e16f), entcachemax(-1e16f, -1e16f, -1e16f);

    float calcentcacheradius(extentity &e)
    {
        switch(e.type)
        {
            case WAYPOINT: return 0;
            case TRIGGER: case TELEPORT: case PUSHER:
                if(e.attr[3]) return e.attr[3];
                // fall through
            default:
                return enttype[e.type].radius;
        }
    }

    static void buildentcache(int *indices, int numindices, int depth = 1)
    {
        vec vmin(1e16f, 1e16f, 1e16f), vmax(-1e16f, -1e16f, -1e16f);
        loopi(numindices)
        {
            extentity &e = *ents[indices[i]];
            float radius = calcentcacheradius(e);
            loopk(3)
            {
                vmin[k] = min(vmin[k], e.o[k]-radius);
                vmax[k] = max(vmax[k], e.o[k]+radius);
            }
        }
        if(depth==1)
        {
            entcachemin = vmin;
            entcachemax = vmax;
        }

        int axis = 2;
        loopk(2) if(vmax[k] - vmin[k] > vmax[axis] - vmin[axis]) axis = k;

        float split = 0.5f*(vmax[axis] + vmin[axis]), splitleft = -1e16f, splitright = 1e16f;
        int left, right;
        for(left = 0, right = numindices; left < right;)
        {
            extentity &e = *ents[indices[left]];
            float radius = calcentcacheradius(e);
            if(max(split - (e.o[axis]-radius), 0.0f) > max((e.o[axis]+radius) - split, 0.0f))
            {
                ++left;
                splitleft = max(splitleft, e.o[axis]+radius);
            }
            else
            {
                --right;
                swap(indices[left], indices[right]);
                splitright = min(splitright, e.o[axis]-radius);
            }
        }

        if(!left || right==numindices)
        {
            left = numindices/2;
            right = numindices - left;
            splitleft = -1e16f;
            splitright = 1e16f;
            loopi(numindices)
            {
                extentity &e = *ents[indices[i]];
                float radius = calcentcacheradius(e);
                if(i < left) splitleft = max(splitleft, e.o[axis]+radius);
                else splitright = min(splitright, e.o[axis]-radius);
            }
        }

        int node = entcache.length();
        entcache.add();
        entcache[node].split[0] = splitleft;
        entcache[node].split[1] = splitright;

        if(left==1) entcache[node].child[0] = (axis<<30) | indices[0];
        else
        {
            entcache[node].child[0] = (axis<<30) | entcache.length();
            if(left) buildentcache(indices, left, depth+1);
        }

        if(numindices-right==1) entcache[node].child[1] = (1<<31) | (left==1 ? 1<<30 : 0) | indices[right];
        else
        {
            entcache[node].child[1] = (left==1 ? 1<<30 : 0) | entcache.length();
            if(numindices-right) buildentcache(&indices[right], numindices-right, depth+1);
        }

        entcachedepth = max(entcachedepth, depth);
    }

    void clearentcache()
    {
        entcache.setsizenodelete(0);
        entcachedepth = -1;
        entcachemin = vec(1e16f, 1e16f, 1e16f);
        entcachemax = vec(-1e16f, -1e16f, -1e16f);
    }

    void buildentcache()
    {
        entcache.setsizenodelete(0);
        vector<int> indices;
        loopv(ents)
        {
            extentity &e = *ents[i];
            if(e.type==WAYPOINT || enttype[e.type].usetype != EU_NONE) indices.add(i);
        }
        buildentcache(indices.getbuf(), indices.length());
    }

    struct entcachestack
    {
        entcachenode *node;
        float tmin, tmax;
    };

    vector<entcachenode *> entcachestack;

    int closestent(int type, const vec &pos, float mindist, bool links = false)
    {
        if(entcachedepth<0) buildentcache();

        entcachestack.setsizenodelete(0);

        int closest = -1;
        entcachenode *curnode = &entcache[0];
        #define CHECKCLOSEST(branch) do { \
            int n = curnode->childindex(branch); \
            extentity &e = *ents[n]; \
            if(e.type == type && (!links || !e.links.empty())) \
            { \
                float dist = e.o.squaredist(pos); \
                if(dist < mindist) { closest = n; mindist = dist; } \
            } \
        } while(0)
        for(;;)
        {
            int axis = curnode->axis();
            float dist1 = pos[axis] - curnode->split[0], dist2 = curnode->split[1] - pos[axis];
            if(dist1 >= mindist)
            {
                if(dist2 < mindist)
                {
                    if(!curnode->isleaf(1)) { curnode = &entcache[curnode->childindex(1)]; continue; }
                    CHECKCLOSEST(1);
                }
            }
            else if(curnode->isleaf(0))
            {
                CHECKCLOSEST(0);
                if(dist2 < mindist)
                {
                    if(!curnode->isleaf(1)) { curnode = &entcache[curnode->childindex(1)]; continue; }
                    CHECKCLOSEST(1);
                }
            }
            else
            {
                if(dist2 < mindist)
                {
                    if(!curnode->isleaf(1)) entcachestack.add(&entcache[curnode->childindex(1)]);
                    else CHECKCLOSEST(1);
                }
                curnode = &entcache[curnode->childindex(0)];
                continue;
            }
            if(entcachestack.empty()) return closest;
            curnode = entcachestack.pop();
        }
    }

    void collateents(const vec &pos, float xyrad, float zrad, vector<actitem> &actitems)
    {
        if(entcachedepth<0) buildentcache();

        entcachestack.setsizenodelete(0);

        entcachenode *curnode = &entcache[0];
        #define CHECKITEM(branch) do { \
            int n = curnode->childindex(branch); \
            extentity &e = *ents[n]; \
            if(enttype[e.type].usetype != EU_NONE && (enttype[e.type].usetype!=EU_ITEM || e.spawned)) \
            { \
                float radius = (e.type == TRIGGER || e.type == TELEPORT || e.type == PUSHER) && e.attr[3] ? e.attr[3] : enttype[e.type].radius; \
                if(overlapsbox(pos, zrad, xyrad, e.o, radius, radius)) \
                { \
                    actitem &t = actitems.add(); \
                    t.type = ITEM_ENT; \
                    t.target = n; \
                    t.score = pos.squaredist(e.o); \
                } \
            } \
        } while(0)
        for(;;)
        {
            int axis = curnode->axis();
            float mindist = axis<2 ? xyrad : zrad, dist1 = pos[axis] - curnode->split[0], dist2 = curnode->split[1] - pos[axis];
            if(dist1 >= mindist)
            {
                if(dist2 < mindist)
                {
                    if(!curnode->isleaf(1)) { curnode = &entcache[curnode->childindex(1)]; continue; }
                    CHECKITEM(1);
                }
            }
            else if(curnode->isleaf(0))
            {
                CHECKITEM(0);
                if(dist2 < mindist)
                {
                    if(!curnode->isleaf(1)) { curnode = &entcache[curnode->childindex(1)]; continue; }
                    CHECKITEM(1);
                }
            }
            else
            {
                if(dist2 < mindist)
                {
                    if(!curnode->isleaf(1)) entcachestack.add(&entcache[curnode->childindex(1)]);
                    else CHECKITEM(1);
                }
                curnode = &entcache[curnode->childindex(0)];
                continue;
            }
            if(entcachestack.empty()) return;
            curnode = entcachestack.pop();
        }
    }

	static int sortitems(const actitem *a, const actitem *b)
	{
		if(a->score > b->score) return -1;
		if(a->score < b->score) return 1;
		return 0;
	}

	bool collateitems(gameent *d, vector<actitem> &actitems)
	{
		float eye = d->height*0.5f;
		vec m = vec(d->o).sub(vec(0, 0, eye));

        collateents(m, d->radius, eye, actitems);
		loopv(projs::projs)
		{
			projent &proj = *projs::projs[i];
			if(proj.projtype != PRJ_ENT || !proj.ready()) continue;
			if(!ents.inrange(proj.id) || enttype[ents[proj.id]->type].usetype != EU_ITEM) continue;
			if(!overlapsbox(m, eye, d->radius, proj.o, enttype[ents[proj.id]->type].radius, enttype[ents[proj.id]->type].radius))
				continue;
			actitem &t = actitems.add();
			t.type = ITEM_PROJ;
			t.target = i;
			t.score = m.squaredist(proj.o);
		}
		if(!actitems.empty())
		{
			actitems.sort(sortitems); // sort items so last is closest
			return true;
		}
		return false;
	}

	void execitem(int n, gameent *d, bool &tried)
	{
		gameentity &e = *(gameentity *)ents[n];
		switch(enttype[e.type].usetype)
		{
			case EU_ITEM:
			{
				if(d->useaction)
				{
					if(world::allowmove(d) && d->requse < 0)
					{
						int sweap = m_spawnweapon(world::gamemode, world::mutators), attr = e.type == WEAPON ? weapattr(e.attr[0], sweap) : e.attr[0];
						if(d->canuse(e.type, attr, e.attr[1], e.attr[2], e.attr[3], e.attr[4], sweap, lastmillis))
						{
							client::addmsg(SV_ITEMUSE, "ri3", d->clientnum, lastmillis-world::maptime, n);
							d->requse = lastmillis;
							d->useaction = false;
						}
						else tried = true;
					}
					else tried = true;
				}
				break;
			}
			case EU_AUTO:
			{
				if(e.type != TRIGGER || ((e.attr[2] == TA_ACT && d->useaction && d == world::player1) || e.attr[2] == TA_AUTO))
				{
					switch(e.type)
					{
						case TELEPORT:
						{
							if(lastmillis-e.lastuse >= triggertime(e))
							{
								e.lastuse = e.lastemit = lastmillis;
								static vector<int> teleports;
								teleports.setsize(0);
								loopv(e.links)
									if(ents.inrange(e.links[i]) && ents[e.links[i]]->type == e.type)
										teleports.add(e.links[i]);
								if(!teleports.empty())
								{
									bool teleported = false;
									while(!teleports.empty())
									{
										int r = e.type == TELEPORT ? rnd(teleports.length()) : 0, t = teleports[r];
										gameentity &f = *(gameentity *)ents[t];
										d->timeinair = 0;
										d->falling = vec(0, 0, 0);
										d->o = vec(f.o).add(vec(0, 0, (d->height*0.5f)-d->aboveeye));
										if(physics::entinmap(d, false))
										{
											d->yaw = f.attr[0];
											d->pitch = f.attr[1];
											float mag = m_speedscale(max(d->vel.magnitude(), f.attr[2] ? float(f.attr[2]) : 50.f));
											vecfromyawpitch(d->yaw, d->pitch, 1, 0, d->vel);
											d->vel.mul(mag);
											world::fixfullrange(d->yaw, d->pitch, d->roll, true);
											f.lastuse = f.lastemit = e.lastemit;
											execlink(d, n, true);
											execlink(d, t, true);
											if(d == world::player1) world::resetcamera();
											teleported = true;
											break;
										}
										teleports.remove(r); // must've really sucked, try another one
									}
									if(!teleported) world::suicide(d, HIT_SPAWN);
								}
							}
							break;
						}
						case PUSHER:
						{
							vec dir = vec((int)(char)e.attr[2], (int)(char)e.attr[1], (int)(char)e.attr[0]).mul(m_speedscale(10.f));
							d->timeinair = 0;
							d->falling = vec(0, 0, 0);
							loopk(3)
							{
								if((d->vel.v[k] > 0.f && dir.v[k] < 0.f) || (d->vel.v[k] < 0.f && dir.v[k] > 0.f) || (fabs(dir.v[k]) > fabs(d->vel.v[k])))
									d->vel.v[k] = dir.v[k];
							}
							if(lastmillis-e.lastuse >= triggertime(e)/2) e.lastuse = e.lastemit = lastmillis;
							execlink(d, n, true);
							break;
						}
						case TRIGGER:
						{
							if((!e.spawned || e.attr[1] != TR_NONE || e.attr[2] != TA_AUTO) && lastmillis-e.lastuse >= triggertime(e)/2)
							{
								e.lastuse = lastmillis;
								switch(e.attr[1])
								{
									case TR_NONE: case TR_LINK:
									{ // wait for ack
										client::addmsg(SV_TRIGGER, "ri2", d->clientnum, n);
										break;
									}
									case TR_SCRIPT:
									{
										if(d == world::player1)
										{
											s_sprintfd(s)("on_trigger_%d", e.attr[0]);
											RUNWORLD(s);
										}
										break;
									}
									default: break;
								}
								if(e.attr[2] == TA_ACT) d->useaction = false;
							}
							break;
						}
					}
				}
				break;
			}
		}
	}

	void checkitems(gameent *d)
	{
		static vector<actitem> actitems;
        actitems.setsizenodelete(0);
		if(collateitems(d, actitems))
		{
			bool tried = false;
			while(!actitems.empty())
			{
				actitem &t = actitems.last();
				int ent = -1;
				switch(t.type)
				{
					case ITEM_ENT:
					{
						if(!ents.inrange(t.target)) break;
						ent = t.target;
						break;
					}
					case ITEM_PROJ:
					{
						if(!projs::projs.inrange(t.target)) break;
						projent &proj = *projs::projs[t.target];
						ent = proj.id;
						break;
					}
					default: break;
				}
				if(ents.inrange(ent)) execitem(ent, d, tried);
				actitems.pop();
			}
			if(tried && d->useaction && d == world::player1)
			{
				playsound(S_DENIED, d->o, d);
				d->useaction = false;
			}
		}
		if(m_ctf(world::gamemode)) ctf::checkflags(d);
	}

	void putitems(ucharbuf &p)
	{
		loopv(ents) if(enttype[ents[i]->type].usetype == EU_ITEM || ents[i]->type == PLAYERSTART || ents[i]->type == TRIGGER)
		{
			gameentity &e = *(gameentity *)ents[i];
			putint(p, i);
			putint(p, int(e.type));
			putint(p, int(e.attr[0]));
			putint(p, int(e.attr[1]));
			putint(p, int(e.attr[2]));
			putint(p, int(e.attr[3]));
			putint(p, int(e.attr[4]));
		}
	}

	void setspawn(int n, bool on)
	{
		if(ents.inrange(n))
		{
			gameentity &e = *(gameentity *)ents[n];
			if((e.spawned = on) != false) e.lastspawn = lastmillis;
			if(e.type == TRIGGER)
			{
				if(e.attr[1] == TR_NONE || e.attr[1] == TR_LINK)
				{
					int millis = lastmillis-e.lastemit, delay = triggertime(e);
					if(e.lastemit && millis < delay) // skew the animation forward
						e.lastemit = lastmillis-(delay-millis);
					else e.lastemit = lastmillis;
					execlink(NULL, n, false);
				}
			}
			else if(enttype[e.type].usetype == EU_ITEM)
			{
				loopv(projs::projs)
				{
					projent &proj = *projs::projs[i];
					if(proj.projtype != PRJ_ENT || proj.id != n || !ents.inrange(proj.id)) continue;
					world::spawneffect(proj.o, 0x6666FF, enttype[ents[proj.id]->type].radius);
					proj.beenused = true;
					proj.state = CS_DEAD;
				}
				gameent *d = NULL;
				loopi(world::numdynents()) if((d = (gameent *)world::iterdynents(i)) && d->type == ENT_PLAYER)
				{
					loopk(WEAPON_MAX) if(d->entid[k] == n) d->entid[k] = -1;
				}
			}
		}
	}

	extentity *newent() { return new gameentity; }
    void deleteent(extentity *e) { delete (gameentity *)e; }

    void clearents()
    {
        clearentcache();
        while(ents.length()) deleteent(ents.pop());
    }

	bool cansee(extentity &e)
	{
		return (showentinfo || world::player1->state == CS_EDITING) && (!enttype[e.type].noisy || showentnoisy >= 2 || (showentnoisy && world::player1->state == CS_EDITING));
	}

	void fixentity(int n)
	{
		gameentity &e = *(gameentity *)ents[n];
		loopvrev(e.links)
		{
			int ent = e.links[i];
			if(!canlink(n, ent, verbose >= 2)) e.links.remove(i);
			else if(ents.inrange(ent))
			{
				gameentity &f = *(gameentity *)ents[ent];
				if(((enttype[e.type].reclink&inttobit(f.type)) || (enttype[f.type].reclink&inttobit(e.type))) && f.links.find(n) < 0)
				{
					f.links.add(n);
					if(verbose) conoutf("\frWARNING: automatic reciprocal link between %d and %d added", n, ent);
				}
				else continue;
			}
			else continue;
			fixentity(ent);
		}
		if(issound(e.schan))
		{
			removesound(e.schan);
			e.schan = -1; // prevent clipping when moving around
			if(e.type == MAPSOUND) e.lastemit = lastmillis;
		}
		switch(e.type)
		{
			case MAPMODEL:
			case PARTICLES:
			case MAPSOUND:
			{
				loopv(e.links) if(ents.inrange(e.links[i]) && ents[e.links[i]]->type == TRIGGER)
				{
					if(ents[e.links[i]]->lastemit < e.lastemit)
					{
						e.lastemit = ents[e.links[i]]->lastemit;
						e.spawned = ents[e.links[i]]->spawned;
					}
				}
				break;
			}
			case TRIGGER:
			{
				loopv(e.links) if(ents.inrange(e.links[i]) &&
					(ents[e.links[i]]->type == MAPMODEL || ents[e.links[i]]->type == PARTICLES || ents[e.links[i]]->type == MAPSOUND))
				{
					if(e.lastemit < ents[e.links[i]]->lastemit)
					{
						ents[e.links[i]]->lastemit = e.lastemit;
						ents[e.links[i]]->spawned = e.spawned;
					}
				}
				break;
			}
			case WEAPON:
				while(e.attr[0] < 0) e.attr[0] += WEAPON_TOTAL; // don't allow superimposed weaps
				while(e.attr[0] >= WEAPON_TOTAL) e.attr[0] -= WEAPON_TOTAL;
				break;
			case PLAYERSTART:
				while(e.attr[0] < 0) e.attr[0] += 360;
				while(e.attr[0] >= 360) e.attr[0] -= 360;
			case FLAG:
				while(e.attr[1] < 0) e.attr[1] += TEAM_MAX;
				while(e.attr[1] >= TEAM_MAX) e.attr[1] -= TEAM_MAX;
				break;
			case TELEPORT:
				while(e.attr[0] < 0) e.attr[0] += 360;
				while(e.attr[0] >= 360) e.attr[0] -= 360;
				while(e.attr[1] < 0) e.attr[1] += 360;
				while(e.attr[1] >= 360) e.attr[1] -= 360;
				break;
			default:
				break;
		}
	}

	const char *findname(int type)
	{
		if(type >= NOTUSED && type < MAXENTTYPES) return enttype[type].name;
		return "";
	}

	int findtype(char *type)
	{
		loopi(MAXENTTYPES) if(!strcmp(type, enttype[i].name)) return i;
		return NOTUSED;
	}

	// these functions are called when the client touches the item
	void execlink(gameent *d, int index, bool local)
	{
		if(ents.inrange(index) && maylink(ents[index]->type))
		{
			gameentity &e = *(gameentity *)ents[index];
			bool commit = false;
			loopv(ents)
			{
				gameentity &f = *(gameentity *)ents[i];
				if(f.links.find(index) >= 0)
				{
					bool both = e.links.find(i) >= 0;
					switch(f.type)
					{
						case MAPMODEL:
						{
							if(e.type == TRIGGER)
							{
								f.spawned = e.spawned;
								f.lastemit = e.lastemit;
							}
							break;
						}
						case PARTICLES:
						{
							if(e.type == TRIGGER || (!f.lastemit || lastmillis-f.lastemit >= triggertime(f)/2))
							{
								f.lastemit = e.lastemit;
								commit = e.type != TRIGGER && local;
							}
							break;
						}
						case MAPSOUND:
						{
							if(mapsounds.inrange(f.attr[0]) && !issound(f.schan))
							{
								f.lastemit = e.lastemit;
								int flags = SND_MAP;
								if(f.attr[4]&SND_NOATTEN) flags |= SND_NOATTEN;
								if(f.attr[4]&SND_NODELAY) flags |= SND_NODELAY;
								if(f.attr[4]&SND_NOCULL) flags |= SND_NOCULL;
								playsound(f.attr[0], both ? f.o : e.o, NULL, flags, f.attr[3], f.attr[1], f.attr[2], &f.schan);
								commit = e.type != TRIGGER && local;
							}
							break;
						}
						default: break;
					}
				}
			}
			if(d && commit) client::addmsg(SV_EXECLINK, "ri2", d->clientnum, index);
		}
	}

	bool tryspawn(dynent *d, const vec &o, float yaw = 0.f)
	{
		d->yaw = yaw;
		d->pitch = d->roll = 0;
		d->o = o;
		world::fixrange(d->yaw, d->pitch);
		return physics::entinmap(d, true);
	}

	bool spawnplayer(gameent *d, int ent, bool recover, bool suicide)
	{
		if(ents.inrange(ent) && tryspawn(d, ents[ent]->o, float(ents[ent]->attr[0]))) return true;
		if(recover)
		{
			if(m_team(world::gamemode, world::mutators))
			{
				loopv(ents) if(ents[i]->type == PLAYERSTART && ents[i]->attr[1] == d->team && tryspawn(d, ents[i]->o, float(ents[i]->attr[0])))
					return true;
			}
			loopv(ents) if(ents[i]->type == PLAYERSTART && tryspawn(d, ents[i]->o, float(ents[i]->attr[0]))) return true;
			loopv(ents) if(ents[i]->type == WEAPON && tryspawn(d, ents[i]->o)) return true;
			d->o.x = d->o.y = d->o.z = 0.5f*getworldsize();
			if(physics::entinmap(d, false)) return true;
		}
		if(m_edit(world::gamemode))
		{
			physics::entinmap(d, true);
			return true;
		}
		else if(m_play(world::gamemode) && suicide)
			world::suicide(d, HIT_SPAWN);
		return false;
	}

	void editent(int i)
	{
		extentity &e = *ents[i];
		fixentity(i);
		if(m_edit(world::gamemode))
		{
			client::addmsg(SV_EDITENT, "ri9i", i, (int)(e.o.x*DMF), (int)(e.o.y*DMF), (int)(e.o.z*DMF), e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4]); // FIXME
			clearentcache();
		}
	}

	float dropheight(entity &e)
	{
		if(e.type==MAPMODEL || e.type==FLAG) return 0.0f;
		return 4.0f;
	}

	bool maylink(int type, int ver)
	{
		if(enttype[type].links && enttype[type].links <= (ver ? ver : GAMEVERSION))
				return true;
		return false;
	}

	bool canlink(int index, int node, bool msg)
	{
		if(ents.inrange(index) && ents.inrange(node))
		{
			if(index != node && maylink(ents[index]->type) && maylink(ents[node]->type) &&
					(enttype[ents[index]->type].canlink&inttobit(ents[node]->type)))
						return true;
			if(msg)
				conoutf("\frentity %s (%d) and %s (%d) are not linkable", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);

			return false;
		}
		if(msg) conoutf("\frentity %d and %d are unable to be linked as one does not seem to exist", index, node);
		return false;
	}

	bool linkents(int index, int node, bool add, bool local, bool toggle)
	{
		if(ents.inrange(index) && ents.inrange(node) && index != node && canlink(index, node, local && verbose))
		{
			gameentity &e = *(gameentity *)ents[index], &f = *(gameentity *)ents[node];
			bool recip = (enttype[e.type].reclink&inttobit(f.type)) || (enttype[f.type].reclink&inttobit(e.type));
			int g = -1, h = -1;
			if((toggle || !add) && (g = e.links.find(node)) >= 0)
			{
				if(!add || (toggle && (!canlink(node, index) || (h = f.links.find(index)) >= 0)))
				{
					e.links.remove(g);
					if(recip) f.links.remove(h);
					fixentity(index);
					if(local && m_edit(world::gamemode)) client::addmsg(SV_EDITLINK, "ri3", 0, index, node);
					if(verbose > 2) conoutf("\fwentity %s (%d) and %s (%d) delinked", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
					return true;
				}
				else if(toggle && canlink(node, index))
				{
					f.links.add(index);
					if(recip && (h = e.links.find(node)) < 0) e.links.add(node);
					fixentity(node);
					if(local && m_edit(world::gamemode)) client::addmsg(SV_EDITLINK, "ri3", 1, node, index);
					if(verbose > 2) conoutf("\fwentity %s (%d) and %s (%d) linked", enttype[ents[node]->type].name, node, enttype[ents[index]->type].name, index);
					return true;
				}
			}
			else if(toggle && canlink(node, index) && (g = f.links.find(index)) >= 0)
			{
				f.links.remove(g);
				if(recip && (h = e.links.find(node)) >= 0) e.links.remove(h);
				fixentity(node);
				if(local && m_edit(world::gamemode)) client::addmsg(SV_EDITLINK, "ri3", 0, node, index);
				if(verbose > 2) conoutf("\fwentity %s (%d) and %s (%d) delinked", enttype[ents[node]->type].name, node, enttype[ents[index]->type].name, index);
				return true;
			}
			else if(toggle || add)
			{
				e.links.add(node);
				if(recip && (h = f.links.find(index)) < 0) f.links.add(index);
				fixentity(index);
				if(local && m_edit(world::gamemode)) client::addmsg(SV_EDITLINK, "ri3", 1, index, node);
				if(verbose > 2) conoutf("\fwentity %s (%d) and %s (%d) linked", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
				return true;
			}
		}
		if(verbose > 2)
			conoutf("\frentity %s (%d) and %s (%d) failed linking", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
		return false;
	}

	struct linkq
	{
		uint id;
		float curscore, estscore;
		linkq *prev;

		linkq() : id(0), curscore(0.f), estscore(0.f), prev(NULL) {}

        float score() const { return curscore + estscore; }
	};

	bool route(gameent *d, int node, int goal, vector<int> &route, avoidset &obstacles, bool retry, float *score)
	{
        if(!ents.inrange(node) || !ents.inrange(goal) || ents[goal]->type != ents[node]->type || goal == node || ents[node]->links.empty())
			return false;

		static uint routeid = 1;
		static vector<linkq> nodes;
		static vector<linkq *> queue;

		int routestart = verbose >= 3 ? SDL_GetTicks() : 0;

		if(!routeid)
		{
			loopv(nodes) nodes[i].id = 0;
			routeid = 1;
		}
		while(nodes.length() < ents.length()) nodes.add();

		if(!retry)
		{
			loopavoid(obstacles, d,
			{
				if(ents.inrange(ent) && ents[ent]->type == ents[node]->type && (ent == node || ent == goal || !ents[ent]->links.empty()))
				{
					nodes[ent].id = routeid;
					nodes[ent].curscore = -1.f;
					nodes[ent].estscore = 0.f;
				}
			});
		}

		nodes[node].id = routeid;
		nodes[goal].curscore = nodes[node].curscore = 0.f;
		nodes[goal].estscore = nodes[node].estscore = 0.f;
		nodes[goal].prev = nodes[node].prev = NULL;
		queue.setsizenodelete(0);
		queue.add(&nodes[node]);
		route.setsizenodelete(0);

		int lowest = -1;
		while(!queue.empty())
		{
			int q = queue.length()-1;
			loopi(queue.length()-1) if(queue[i]->score() < queue[q]->score()) q = i;
			linkq *m = queue.removeunordered(q);
			float prevscore = m->curscore;
			m->curscore = -1.f;
			int current = int(m-&nodes[0]);
			extentity &ent = *ents[current];
			vector<int> &links = ent.links;
			loopv(links)
			{
				int link = links[i];
				if(ents.inrange(link) && ents[link]->type == ents[node]->type && (link == node || link == goal || !ents[link]->links.empty()))
				{
					linkq &n = nodes[link];
					float curscore = prevscore + ents[link]->o.dist(ent.o);
					if(n.id == routeid && curscore >= n.curscore) continue;
					n.curscore = curscore;
					n.prev = m;
					if(n.id != routeid)
					{
						n.estscore = ents[link]->o.dist(ents[goal]->o);
						if((retry || n.estscore <= float(enttype[ents[link]->type].radius*4)) && (lowest < 0 || n.estscore < nodes[lowest].estscore))
							lowest = link;
						if(link != goal) queue.add(&n);
						else queue.setsizenodelete(0);
						n.id = routeid;
					}
				}
			}
		}

        routeid++;

		if(lowest >= 0) // otherwise nothing got there
		{
			for(linkq *m = &nodes[lowest]; m != NULL; m = m->prev)
				route.add(m - &nodes[0]); // just keep it stored backward
			if(!route.empty() && score) *score = nodes[lowest].score();
		}

		if(verbose >= 3)
			conoutf("\fwroute %d to %d (%d) generated %d nodes in %fs", node, goal, lowest, route.length(), (SDL_GetTicks()-routestart)/1000.0f);

		return !route.empty();
	}

	int entitynode(const vec &v, bool links, bool drop)
	{
        float mindist = float((enttype[WAYPOINT].radius*enttype[WAYPOINT].radius)*(drop ? 1.f : 4.f));
		if(!drop && !autodropped) return closestent(WAYPOINT, v, mindist, links);
        int n = -1;
        loopv(ents) if(ents[i]->type == WAYPOINT && (!links || !ents[i]->links.empty()))
        {
            float u = ents[i]->o.squaredist(v);
            if(u <= mindist)
            {
                n = i;
                mindist = u;
            }
        }
		return n;
	}

	void entitylink(int index, int node, bool both = true)
	{
		if(ents.inrange(index) && ents.inrange(node))
		{
			gameentity &e = *(gameentity *)ents[index], &f = *(gameentity *)ents[node];
			if(e.links.find(node) < 0) linkents(index, node, true, true, false);
			if(both && f.links.find(index) < 0) linkents(node, index, true, true, false);
		}
	}

	void entitycheck(gameent *d)
	{
		bool autodrop = autodroptime >= 0 && lastmillis-autodroptime <= autodropwaypoints*1000;
		if(!autodrop && !dropwaypoints && autodropped)
		{
			clearentcache();
			autodropped = false;
		}
		if(d->state == CS_ALIVE)
		{
			vec v(world::feetpos(d, 0.f));
			if((dropwaypoints || autodrop) && ((m_play(world::gamemode) && d->aitype == AI_NONE) || d == world::player1))
			{
				int curnode = entitynode(v, false, true);
				if(!ents.inrange(curnode))
				{
					int cmds = WP_NONE;
					if(physics::iscrouching(d)) cmds |= WP_CROUCH;
					curnode = ents.length();
					newentity(v, WAYPOINT, cmds, 0, 0, 0, 0);
					autodropped = true;
				}
				if(ents.inrange(d->lastnode) && d->lastnode != curnode)
					entitylink(d->lastnode, curnode, !d->timeinair);
				d->lastnode = curnode;
			}
			else
			{
				int curnode = entitynode(v);
				if(ents.inrange(curnode)) d->lastnode = curnode;
				else d->lastnode = -1;
			}
		}
		else d->lastnode = -1;
	}

	void mergewaypoints()
	{
		float mindist = (enttype[WAYPOINT].radius*enttype[WAYPOINT].radius)*waypointmergescale;
		int totalmerges = 0, totalpasses = 0;
		while(true)
		{
			int merges = 0;
			loopvj(ents) if(ents[j]->type == WAYPOINT)
			{
				gameentity &e = *(gameentity *)ents[j];
				if(verbose) renderprogress(float(j)/float(ents.length()), "merging waypoints...");
				vec avg(e.o);
				loopvk(ents) if(k != j && ents[k]->type == WAYPOINT)
				{
					gameentity &f = *(gameentity *)ents[k];
					if(e.o.squaredist(f.o) <= mindist)
					{
						if(verbose >= 2) conoutf("\frWARNING: automatically transposing waypoint %d into %d", k, j);
						loopv(f.links) if(f.links[i] != j && e.links.find(f.links[i]) < 0)
						{
							e.links.add(f.links[i]);
							if(verbose >= 3) conoutf("\frWARNING: %d received link %d", j, f.links[i]);
						}
						loopv(ents) if(i != k)
						{
							gameentity &g = *(gameentity *)ents[i];
							int id = g.links.find(k);
							if(id >= 0)
							{
								g.links.remove(id);
								if(g.links.find(j) < 0)
								{
									g.links.add(j);
									if(verbose >= 3) conoutf("\frWARNING: %d received link %d from old child %d", i, j, k);
								}
							}
						}
						f.links.setsize(0);
						f.type = NOTUSED;
						e.o.add(f.o).mul(0.5f);
						merges++;
					}
				}
			}
			totalpasses++;
			if(!merges) break;
			totalmerges += merges;
			if(verbose >= 2) conoutf("\frWARNING: %d waypoint(s) merged, taking another pass", merges);
		}
		if(verbose) conoutf("\frWARNING: transposed %d total waypoint(s) in %d pass(es)", totalmerges, totalpasses);
	}
	ICOMMAND(mergewaypoints, "", (void), mergewaypoints());

	void readent(gzFile &g, int mtype, int mver, char *gid, int gver, int id, entity &e)
	{
		gameentity &f = (gameentity &)e;
		f.mark = false;
		if(mtype == MAP_OCTA)
		{
			// translate into our format
			switch(f.type)
			{
				// 1	LIGHT			1	LIGHT
				// 2	MAPMODEL		2	MAPMODEL
				// 3	PLAYERSTART		3	PLAYERSTART
				// 4	ENVMAP			4	ENVMAP
				// 5	PARTICLES		5	PARTICLES
				// 6	MAPSOUND		6	MAPSOUND
				// 7	SPOTLIGHT		7	SPOTLIGHT
				case 1: case 2: case 3: case 4: case 5: case 6: case 7:
				{
					break;
				}

				// 8	I_SHELLS		8	WEAPON		WEAPON_SG
				// 9	I_BULLETS		8	WEAPON		WEAPON_CG
				// 10	I_ROCKETS		8	WEAPON		WEAPON_CARBINE
				// 11	I_ROUNDS		8	WEAPON		WEAPON_RIFLE
				// 12	I_GRENADES		8	WEAPON		WEAPON_GL
				// 13	I_CARTRIDGES	8	WEAPON		WEAPON_PLASMA
				case 8: case 9: case 10: case 11: case 12: case 13:
				{
					int weap = f.type-8, weapmap[6] = {
						WEAPON_SG, WEAPON_CG, WEAPON_CARBINE, WEAPON_RIFLE, WEAPON_GL, WEAPON_PLASMA
					};

					if(weap >= 0 && weap <= 5)
					{
						f.type = WEAPON;
						f.attr[0] = weapmap[weap];
						f.attr[1] = 0;
					}
					else f.type = NOTUSED;
					break;
				}
				// 18	I_QUAD			8	WEAPON		WEAPON_FLAMER
				case 18:
				{
					f.type = WEAPON;
					f.attr[0] = WEAPON_FLAMER;
					f.attr[1] = 0;
					break;
				}

				// 19	TELEPORT		9	TELEPORT
				// 20	TELEDEST		9	TELEPORT (linked)
				case 19: case 20:
				{
					if(f.type == 20) f.mark = true; // needs translating later
					f.type = TELEPORT;
					break;
				}
				// 21	MONSTER			10	NOTUSED
				case 21:
				{
					f.type = NOTUSED;
					break;
				}
				// 22	CARROT			11	TRIGGER		0
				case 22:
				{
					f.type = TRIGGER;
					f.attr[0] = f.attr[1] = f.attr[2] = f.attr[3] = f.attr[4] = 0;
					break;
				}
				// 23	JUMPPAD			12	PUSHER
				case 23:
				{
					f.type = PUSHER;
					break;
				}
				// 24	BASE			13	FLAG		1:idx		TEAM_NEUTRAL
				case 24:
				{
					f.type = FLAG;
					if(f.attr[0] < 0) f.attr[0] = 0;
					f.attr[1] = TEAM_NEUTRAL; // spawn as neutrals
					break;
				}
				// 25	RESPAWNPOINT	14	CHECKPOINT
				case 25:
				{
					f.type = CHECKPOINT;
					break;
				}
				// 30	FLAG			13	FLAG		#			2:team
				case 30:
				{
					f.type = FLAG;
					f.attr[0] = 0;
					if(f.attr[1] <= 0) f.attr[1] = -1; // needs a team
					break;
				}

				// 14	I_HEALTH		-	NOTUSED
				// 15	I_BOOST			-	NOTUSED
				// 16	I_GREENARMOUR	-	NOTUSED
				// 17	I_YELLOWARMOUR	-	NOTUSED
				// 26	BOX				-	NOTUSED
				// 27	BARREL			-	NOTUSED
				// 28	PLATFORM		-	NOTUSED
				// 29	ELEVATOR		-	NOTUSED
				default:
				{
					if(verbose) conoutf("\frWARNING: ignoring entity %d type %d", id, f.type);
					f.type = NOTUSED;
					break;
				}
			}
		}
	}

	void writeent(gzFile &g, int id, entity &e)
	{
	}

	void initents(gzFile &g, int mtype, int mver, char *gid, int gver)
	{
		renderprogress(0, "checking entities...");
		if(gver <= 49 || mtype == MAP_OCTA)
		{
			int flag = 0, teams[TEAM_NUM] = { 0, 0, 0, 0 };
			vector<short> teleyaw;
			loopv(ents) teleyaw.add(0);

			loopv(ents)
			{
				gameentity &e = *(gameentity *)ents[i];
				if(verbose) renderprogress(float(i)/float(ents.length()), "importing entities...");

				if(e.type == TELEPORT && e.mark) // translate teledest to teleport and link them appropriately
				{
					int dest = -1;

					loopvj(ents) // see if this guy is sitting on top of a teleport already
					{
						gameentity &f = *(gameentity *)ents[j];

						if(f.type == TELEPORT && !f.mark &&
							(!ents.inrange(dest) || e.o.dist(f.o) < ents[dest]->o.dist(f.o)) &&
								e.o.dist(f.o) <= enttype[TELEPORT].radius*4.f)
									dest = j;
					}

					if(ents.inrange(dest))
					{
						if(verbose) conoutf("\frWARNING: replaced teledest %d with closest teleport %d", i, dest);
						e.type = NOTUSED; // get rid of this guy then
					}
					else
					{
						if(verbose) conoutf("\frWARNING: modified teledest %d to a teleport", i);
						dest = i;
					}

					teleyaw[dest] = e.attr[0]; // store the yaw for later

					loopvj(ents) // find linked teleport(s)
					{
						gameentity &f = *(gameentity *)ents[j];
						if(f.type == TELEPORT && !f.mark && f.attr[0] == e.attr[1])
						{
							if(verbose) conoutf("\frimported teleports %d and %d linked automatically", dest, j);
							f.links.add(dest);
						}
					}
				}
				if(e.type == WEAPON)
				{
					float mindist = float(enttype[WEAPON].radius*enttype[WEAPON].radius*3);
					int weaps[WEAPON_MAX];
					loopj(WEAPON_MAX) weaps[j] = j != e.attr[0] ? 0 : 1;
					loopvj(ents) if(j != i)
					{
						gameentity &f = *(gameentity *)ents[j];
						if(f.type == WEAPON && e.o.squaredist(f.o) <= mindist && isweap(f.attr[0]))
						{
							weaps[f.attr[0]]++;
							f.type = NOTUSED;
							if(verbose) conoutf("\frculled tightly packed weapon %d [%d]", j, f.attr[0]);
						}
					}
					int best = e.attr[0];
					loopj(WEAPON_MAX) if(weaps[j] > weaps[best])
						best = j;
					e.attr[0] = best;
				}
				if(e.type == FLAG) // replace bases/neutral flags near team flags
				{
					if(valteam(e.attr[1], TEAM_FIRST)) teams[e.attr[1]-TEAM_FIRST]++;
					else if(e.attr[1] == TEAM_NEUTRAL)
					{
						int dest = -1;

						loopvj(ents) if(j != i)
						{
							gameentity &f = *(gameentity *)ents[j];

							if(f.type == FLAG && f.attr[1] != TEAM_NEUTRAL &&
								(!ents.inrange(dest) || e.o.dist(f.o) < ents[dest]->o.dist(f.o)) &&
									e.o.dist(f.o) <= enttype[FLAG].radius*4.f)
										dest = j;
						}

						if(ents.inrange(dest))
						{
							gameentity &f = *(gameentity *)ents[dest];
							conoutf("\frWARNING: old base %d (%d, %d) replaced with flag %d (%d, %d)", i, e.attr[0], e.attr[1], dest, f.attr[0], f.attr[1]);
							if(!f.attr[0]) f.attr[0] = e.attr[0]; // give it the old base idx
							e.type = NOTUSED;
						}
						else if(e.attr[0] > flag) flag = e.attr[0]; // find the highest idx
					}
				}
			}

			loopv(ents)
			{
				gameentity &e = *(gameentity *)ents[i];

				switch(e.type)
				{
					case TELEPORT:
					{
						e.attr[0] = teleyaw[i]; // grab what we stored earlier
						e.attr[1] = e.attr[3] = 0; // not set in octa
						e.attr[2] = 100; // give a push
						e.attr[4] = 0x11A; // give it a pretty blueish teleport like sauer's
						e.o.z += world::player1->height/2; // teleport in BF is at middle
						e.mark = false;
						break;
					}
					case WAYPOINT:
					{
						e.attr[0] = 0;
						break;
					}
					case FLAG:
					{
						if(!e.attr[0]) e.attr[0] = ++flag; // assign a sane idx
						if(!valteam(e.attr[1], TEAM_NEUTRAL)) // assign a team
						{
							int lowest = -1;
							loopk(TEAM_NUM)
								if(lowest<0 || teams[k] < teams[lowest])
									lowest = i;
							e.attr[1] = lowest+TEAM_FIRST;
							teams[lowest]++;
						}
						break;
					}
				}
			}
		}
		renderprogress(0.25f, "checking entities...");

		physent dummyent;
		dummyent.height = dummyent.radius = dummyent.xradius = dummyent.yradius = 1;
		loopvj(ents)
		{
			gameentity &e = *(gameentity *)ents[j];
			if(verbose) renderprogress(float(j)/float(ents.length()), "updating old entities...");
			switch(e.type)
			{
				case WEAPON:
				{
					if(mtype == MAP_BFGZ && gver <= 90)
					{ // move grenades to the end of the weapon array
						if(e.attr[0] >= 4) e.attr[0]--;
						else if(e.attr[0] == 3) e.attr[0] = WEAPON_GL;
					}
					if(mtype == MAP_BFGZ && gver <= 97 && e.attr[0] >= 4)
						e.attr[0]++; // add in carbine
					if(mtype != MAP_BFGZ || gver <= 112) e.attr[1] = 0;
					break;
				}
				case PUSHER:
				{
					if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && gver <= 95))
						e.attr[0] = int(e.attr[0]*1.25f);
					break;
				}
				case WAYPOINT:
				{
					if(mtype == MAP_BFGZ && gver <= 90)
						e.attr[0] = e.attr[1] = e.attr[2] = e.attr[3] = e.attr[4] = 0;
					break;
				}
				default: break;
			}
		}
		if(m_play(world::gamemode)) mergewaypoints();
		loopvj(ents) fixentity(j);
		loopvj(ents) if(enttype[ents[j]->type].usetype == EU_ITEM || ents[j]->type == TRIGGER)
			setspawn(j, false);
		renderprogress(1.f, "checking entities...");
	}

	void mapstart()
	{
		autodropped = false;
		autodroptime = autodropwaypoints && m_play(world::gamemode) ? lastmillis : -1;
		if(autodroptime >= 0 && verbose)
			conoutf("\faauto-dropping waypoints for %d second(s)..", autodropwaypoints);
	}

	void edittoggled(bool edit)
	{
		clearentcache();
	}

	#define renderfocus(i,f) { gameentity &e = *(gameentity *)ents[i]; f; }

	void renderlinked(gameentity &e, int idx)
	{
		loopv(e.links)
		{
			int index = e.links[i];

			if(ents.inrange(index))
			{
				gameentity &f = *(gameentity *)ents[index];
				bool both = false;

				loopvj(f.links)
				{
					int g = f.links[j];
					if(ents.inrange(g) && g > idx) // smaller dual links are done already?
					{
						gameentity &h = *(gameentity *)ents[g];
						if(&h == &e)
						{
							both = true;
							break;
						}
					}
				}
#if 0
				part_flare(vec(e.o).add(vec(0, 0, RENDERPUSHZ)),
					vec(f.o).add(vec(0, 0, RENDERPUSHZ)),
						10, PART_LINE, both ? 0xFF8822 : 0xFF2222, 0.2f);
#else
				vec fr(vec(e.o).add(vec(0, 0, RENDERPUSHZ))), dr(vec(f.o).add(vec(0, 0, RENDERPUSHZ)));
				vec col(0.75f, both ? 0.5f : 0.0f, 0.f);
				renderline(fr, dr, col.x, col.y, col.z, false);
				dr.sub(fr);
				dr.normalize();
				float yaw, pitch;
				vectoyawpitch(dr, yaw, pitch);
				dr.mul(RENDERPUSHX);
				dr.add(fr);
				rendertris(dr, yaw, pitch, 2.f, col.x*2.f, col.y*2.f, col.z*2.f, true, false);
#endif
			}
		}
	}

	void renderentshow(gameentity &e, int idx, int level)
	{
		if(level != 1 && e.o.squaredist(camera1->o) > maxparticledistance*maxparticledistance) return;
		if(!level || showentradius >= level)
		{
			switch(e.type)
			{
				case MAPSOUND:
				{
					renderradius(e.o, e.attr[1], e.attr[1], e.attr[1], false);
					renderradius(e.o, e.attr[2], e.attr[2], e.attr[2], false);
					break;
				}
				case LIGHT:
				{
					int s = e.attr[0] ? e.attr[0] : hdr.worldsize;
					renderradius(e.o, s, s, s, false);
					break;
				}
				case FLAG:
				{
					float radius = (float)enttype[e.type].radius;
					renderradius(e.o, radius, radius, radius, false);
					radius *= 0.5f;
					renderradius(e.o, radius, radius, radius, false);
					break;
				}
				default:
				{
					float radius = (float)enttype[e.type].radius;
					if((e.type == TRIGGER || e.type == TELEPORT || e.type == PUSHER) && e.attr[3])
						radius = (float)e.attr[3];
					if(radius > 0.f) renderradius(e.o, radius, radius, radius, false);
					break;
				}
			}
		}

		switch(e.type)
		{
			case PLAYERSTART:
			{
				if(!level || showentdir >= level) renderdir(e.o, e.attr[0], 0.f, false);
				break;
			}
			case MAPMODEL:
			{
				if(!level || showentdir >= level) renderdir(e.o, e.attr[1], e.attr[2], false);
				break;
			}
			case TELEPORT:
			case CAMERA:
			{
				if(!level || showentdir >= level) renderdir(e.o, e.attr[0], e.attr[1], false);
				break;
			}
			case PUSHER:
			{
				if(!level || showentdir >= level)
				{
					vec dir = vec((int)(char)e.attr[2], (int)(char)e.attr[1], (int)(char)e.attr[0]);
					float yaw = 0.f, pitch = 0.f;
					vectoyawpitch(dir, yaw, pitch);
					renderdir(e.o, yaw, pitch, false);
				}
				break;
			}
			default: break;
		}

		if(enttype[e.type].links)
			if(!level || showentlinks >= level || (e.type == WAYPOINT && dropwaypoints))
				renderlinked(e, idx);
	}

	void renderentlight(gameentity &e)
	{
		vec colour = vec(e.attr[1], e.attr[2], e.attr[3]).div(264.f);
		adddynlight(vec(e.o), float(e.attr[0] ? e.attr[0] : hdr.worldsize)*0.9f, colour);
	}

	void adddynlights()
	{
		if(world::player1->state == CS_EDITING && showlighting)
		{
			#define islightable(q) ((q)->type == LIGHT && (q)->attr[0] > 0)
            loopv(entgroup)
            {
                int n = entgroup[i];
                if(ents.inrange(n) && islightable(ents[n]) && n != enthover)
                    renderfocus(n, renderentlight(e));
            }
            if(ents.inrange(enthover) && islightable(ents[enthover]))
                renderfocus(enthover, renderentlight(e));
		}
	}

    void preload()
    {
		static bool weapf[WEAPON_MAX];
		int sweap = m_spawnweapon(world::gamemode, world::mutators);
		loopi(WEAPON_MAX) weapf[i] = (i == sweap ? true : false);
		loopv(ents)
		{
			extentity &e = *ents[i];
			if(e.type == MAPMODEL || e.type == FLAG) continue;
			else if(e.type == WEAPON)
			{
				int attr = weapattr(e.attr[0], sweap);
				if(isweap(attr) && !weapf[attr])
				{
					weapons::preload(attr);
					weapf[attr] = true;
				}
			}
#if 0
			else
			{
				const char *mdlname = entmdlname(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4]);
				if(mdlname && *mdlname) loadmodel(mdlname, -1, true);
			}
#endif
		}
    }

	TVAR(teleporttex, "textures/teleport", 0);

 	void update()
	{
		entitycheck(world::player1);
		loopv(world::players) if(world::players[i]) entitycheck(world::players[i]);
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(e.type == MAPSOUND && e.links.empty() && (!e.lastemit || lastmillis-e.lastemit >= triggertime(e)) && mapsounds.inrange(e.attr[0]) && !issound(e.schan))
			{
				int flags = SND_MAP|SND_LOOP; // ambient sounds loop
				if(e.attr[4]&SND_NOATTEN) flags |= SND_NOATTEN;
				if(e.attr[4]&SND_NODELAY) flags |= SND_NODELAY;
				if(e.attr[4]&SND_NOCULL) flags |= SND_NOCULL;
				playsound(e.attr[0], e.o, NULL, flags, e.attr[3], e.attr[1], e.attr[2], &e.schan);
			}
		}
	}

    void renderteleport(gameentity &e, Texture *t)
    {
		glPushMatrix();
		glEnable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		else glBlendFunc(GL_ONE, GL_ONE);
        defaultshader->set();

		vec dir;
		vecfromyawpitch((float)e.attr[0], (float)e.attr[1], 1, 0, dir);
		vec o(vec(e.o).add(dir));
		glTranslatef(o.x, o.y, o.z);
		glRotatef((float)e.attr[0]-180.f, 0, 0, 1);
		glRotatef((float)e.attr[1], 1, 0, 0);
		float radius = (float)(e.attr[3] ? e.attr[3] : enttype[e.type].radius);
		glScalef(radius, radius, radius);

		glBindTexture(GL_TEXTURE_2D, t->id);

		int attr = int(e.attr[4]), colour = (((attr&0xF)<<4)|((attr&0xF0)<<8)|((attr&0xF00)<<12))+0x0F0F0F;
		float r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f;
		glColor3f(r, g, b);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.f, 0.f, 1.f);
		glTexCoord2f(1.0f, 0.0f); glVertex3f(1.f, 0.f, 1.f);
		glTexCoord2f(1.0f, 1.0f); glVertex3f(1.f, 0.f, -1.f);
		glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.f, 0.f, -1.f);
		xtraverts += 4;
		glEnd();
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
		glPopMatrix();
	}

	void render()
	{
		if(rendermainview) // important, don't render lines and stuff otherwise!
		{
			int level = (m_edit(world::gamemode) ? 2 : ((showentdir==3 || showentradius==3 || showentlinks==3) ? 3 : 0));
			if(level)
            {
            	bool editing = world::player1->state == CS_EDITING;
                renderprimitive(true);
                loopv(ents) renderfocus(i, renderentshow(e, i, editing && (entgroup.find(i) >= 0 || enthover == i) ? 1 : level));
                renderprimitive(false);
            }
		}

		if(!shadowmapping && !envmapping)
		{
			Texture *t = textureload(teleporttex, 0, true);
			loopv(ents)
			{
				gameentity &e = *(gameentity *)ents[i];
				if(e.type == TELEPORT && e.attr[4] && e.o.dist(camera1->o) < maxparticledistance)
					renderteleport(e, t);
			}
		}

		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
			bool active = enttype[e.type].usetype == EU_ITEM && e.spawned;
			if(m_edit(world::gamemode) || active)
			{
				const char *mdlname = entmdlname(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4]);
				if(mdlname && *mdlname)
				{
					int flags = MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED;
					if(!active) flags |= MDL_TRANSLUCENT;
					rendermodel(&e.light, mdlname, ANIM_MAPMODEL|ANIM_LOOP, e.o, 0.f, 0.f, 0.f, flags);
				}
			}
		}
	}

	void drawparticle(gameentity &e, vec &o, int idx, bool spawned)
	{
		if(e.type == NOTUSED || o.dist(camera1->o) > maxparticledistance) return;
		if(e.type == PARTICLES)
		{
			if(idx < 0 || e.links.empty()) makeparticles((entity &)e);
			else if(e.lastemit && lastmillis-e.lastemit < triggertime(e))
				makeparticle(o, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4]);
		}

		bool hasent = false, edit = m_edit(world::gamemode) && cansee(e);
		vec off(0, 0, 2.f), pos(o);
		if(enttype[e.type].usetype == EU_ITEM) pos.add(off);
		if(edit)
		{
			hasent = world::player1->state == CS_EDITING && idx >= 0 && (entgroup.find(idx) >= 0 || enthover == idx);
			part_create(PART_EDIT, 1, o, hasent ? 0x8822FF : 0x6611EE, hasent ? 3.0f : 1.5f);
			if(showentinfo >= 2 || world::player1->state == CS_EDITING)
			{
				s_sprintfd(s)("@%s%s (%d)", hasent ? "\fp" : "\fv", enttype[e.type].name, idx >= 0 ? idx : 0);
				part_text(pos.add(off), s);
				if(showentinfo >= 3 || hasent)
				{
					loopk(5)
					{
						if(*enttype[e.type].attrs[k])
						{
							s_sprintf(s)("@%s%s:%d", hasent ? "\fw" : "\fa", enttype[e.type].attrs[k], e.attr[k]);
							part_text(pos.add(off), s);
						}
						else break;
					}
				}
			}
		}
		bool item = showentnames && enttype[e.type].usetype == EU_ITEM && spawned,
			notitem = (edit && (showentinfo >= 4 || hasent));
		if(item || notitem)
		{
			s_sprintfd(s)("@%s%s", notitem ? (hasent ? "\fw" : "\fa") : "", entinfo(e.type, e.attr[0], e.attr[1], e.attr[2], e.attr[3], e.attr[4], notitem && (showentinfo >= 5 || hasent)));
			part_text(pos.add(off), s);
		}
	}

	void drawparticles()
	{
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			drawparticle(e, e.o, i, e.spawned);
		}
		loopv(projs::projs)
		{
			projent &proj = *projs::projs[i];
			if(proj.projtype != PRJ_ENT || !ents.inrange(proj.id)) continue;
			gameentity &e = *(gameentity *)ents[proj.id];
			drawparticle(e, proj.o, -1, proj.ready());
		}
	}
}
