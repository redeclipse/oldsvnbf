#include "game.h"
namespace entities
{
	vector<extentity *> ents;

	VARP(showentdescs, 0, 2, 3);
	VARP(showentinfo, 0, 1, 5);
	VARP(showentnoisy, 0, 0, 2);
	VARP(showentdir, 0, 2, 3); // 0 = off, 1 = only selected, 2 = always when editing, 3 = always in editmode
	VARP(showentradius, 0, 1, 3);
	VARP(showentlinks, 0, 1, 3);
	VARP(showlighting, 0, 0, 1);
	VAR(dropwaypoints, 0, 0, 1); // drop waypoints during play

	vector<extentity *> &getents() { return ents; }

	int triggertime(extentity &e)
	{
		switch(e.type)
		{
			case TRIGGER: case MAPMODEL: case PARTICLES: case MAPSOUND: case LIGHTFX: case TELEPORT: case PUSHER:
				return m_speedtime(1000); break;
			default: break;
		}
		return 0;
	}

	const char *entinfo(int type, vector<int> &attr, bool full)
	{
		static string entinfostr; entinfostr[0] = 0;
		#define addentinfo(s) if(*(s)) { \
			if(entinfostr[0]) concatstring(entinfostr, ", "); \
			concatstring(entinfostr, s); \
		}
		switch(type)
		{
			case PLAYERSTART: case FLAG:
			{
				if(valteam(attr[0], TEAM_FIRST))
				{
					defformatstring(str)("team %s", teamtype[attr[0]].name);
					addentinfo(str);
				}
				else if(attr[0] < TEAM_MAX)
				{
					defformatstring(str)("%s", teamtype[attr[0]].name);
					addentinfo(str);
				}
				if(attr[3] && attr[3] > -G_MAX && attr[3] < G_MAX)
				{
					string ds;
					if(attr[3]<0) formatstring(ds)("not %s", gametype[-attr[3]].name);
					else formatstring(ds)("%s", gametype[attr[3]].name);
					addentinfo(ds);
				}
				break;
			}
			case LIGHTFX:
			{
				if(full)
				{
					const char *lfxnames[LFX_MAX+1] = { "spotlight", "dynlight", "flicker", "pulse", "glow", "" };
					addentinfo(lfxnames[attr[0] < 0 || attr[0] >= LFX_MAX ? LFX_MAX : attr[0]]);
					loopi(LFX_MAX-1) if(attr[4]&(1<<(LFX_S_MAX+i))) { defformatstring(ds)("+%s", lfxnames[i+1]); addentinfo(ds); break; }
					if(attr[4]&LFX_S_RAND1) addentinfo("rnd-min");
					if(attr[4]&LFX_S_RAND2) addentinfo("rnd-max");
				}
				break;
			}
			case ACTOR:
			{
				if(full && attr[0] >= AI_START && attr[0] < AI_MAX)
				{
					addentinfo(aitype[attr[0]].name);
					if(attr[3] && attr[3] > -G_MAX && attr[3] < G_MAX)
					{
						string ds;
						if(attr[3]<0) formatstring(ds)("not %s", gametype[-attr[3]].name);
						else formatstring(ds)("%s", gametype[attr[3]].name);
						addentinfo(ds);
					}
				}
				break;
			}
			case WEAPON:
			{
				int sweap = m_spawnweapon(game::gamemode, game::mutators), attr1 = weapattr(attr[0], sweap);
				if(isweap(attr1))
				{
					defformatstring(str)("\fs%s%s\fS", weaptype[attr1].text, weaptype[attr1].name);
					addentinfo(str);
					if(full)
					{
						if(attr[2] && attr[2] > -G_MAX && attr[2] < G_MAX)
						{
							string ds;
							if(attr[2]<0) formatstring(ds)("not %s", gametype[-attr[2]].name);
							else formatstring(ds)("%s", gametype[attr[2]].name);
							addentinfo(ds);
						}
						if(attr[3]&WEAP_F_FORCED) addentinfo("forced");
					}
				}
				break;
			}
			case MAPMODEL:
			{
				if(mapmodels.inrange(attr[0])) addentinfo(mapmodels[attr[0]].name);
				if(full)
				{
					if(attr[4]&MMT_HIDE) addentinfo("hide");
					if(attr[4]&MMT_NOCLIP) addentinfo("noclip");
					if(attr[4]&MMT_NOSHADOW) addentinfo("noshadow");
					if(attr[4]&MMT_NODYNSHADOW) addentinfo("nodynshadow");
				}
				break;
			}
			case MAPSOUND:
			{
				if(mapsounds.inrange(attr[0])) addentinfo(mapsounds[attr[0]].sample->name);
				if(full)
				{
					if(attr[4]&SND_NOATTEN) addentinfo("noatten");
					if(attr[4]&SND_NODELAY) addentinfo("nodelay");
					if(attr[4]&SND_NOCULL) addentinfo("nocull");
					if(attr[4]&SND_NOPAN) addentinfo("nopan");
				}
				break;
			}
			case TRIGGER:
			{
				if(full)
				{
					const char *trgnames[TR_MAX+1] = { "toggle", "link", "script", "once", "exit", "" }, *actnames[TA_MAX+1] = { "manual", "proximity", "action", "" };
					addentinfo(trgnames[attr[1] < 0 || attr[1] >= TR_MAX ? TR_MAX : attr[1]]);
					addentinfo(actnames[attr[2] < 0 || attr[2] >= TA_MAX ? TA_MAX : attr[2]]);
					if(attr[4] >= 2) addentinfo(attr[4] ? "routed" : "inert");
					addentinfo(attr[4]%2 ? "on" : "off");
				}
				break;
			}
			case WAYPOINT:
			{
				if(full)
				{
					const char *wpnames[WP_MAX+1] = { "common", "player", "enemy", "linked", "camera", "" }, *wpsnames[WP_S_MAX+1] = { "", "defend", "project", "" };
					addentinfo(wpnames[attr[0] < 0 || attr[0] >= WP_MAX ? WP_MAX : attr[0]]);
					addentinfo(wpsnames[attr[1] < 0 || attr[1] >= WP_S_MAX ? WP_S_MAX : attr[1]]);
					if(attr[4]&WP_F_CROUCH) addentinfo("crouch");
				}
				break;
			}
			default: break;
		}
		return entinfostr[0] ? entinfostr : "";
	}

	const char *entmdlname(int type, vector<int> &attr)
	{
		switch(type)
		{
			case PLAYERSTART: return teamtype[attr[0]].tpmdl;
			case WEAPON:
			{
				int sweap = m_spawnweapon(game::gamemode, game::mutators), attr1 = weapattr(attr[0], sweap);
				return weaptype[attr1].item;
			}
			case FLAG: return teamtype[attr[0]].flag;
			case ACTOR: if(attr[0] >= AI_START && attr[0] < AI_MAX) return aitype[attr[0]].mdl;
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
			loopi(game::numdynents()) if((f = (gameent *)game::iterdynents(i)) && f->type == ENT_PLAYER)
			{
				loopk(WEAP_MAX) if(f->entid[k] == n) f->entid[k] = -1;
			}
			int sweap = m_spawnweapon(game::gamemode, game::mutators), attr = e.type == WEAPON ? weapattr(e.attrs[0], sweap) : e.attrs[0],
				colour = e.type == WEAPON ? weaptype[attr].colour : 0xFFFFFF;
			if(showentdescs)
			{
				const char *texname = showentdescs >= 2 ? hud::itemtex(e.type, attr) : NULL;
				if(texname && *texname) part_icon(d->abovehead(), textureload(texname, 3), 1, 2, -10, 0, game::aboveheadfade, colour, 0, 1, d);
				else
				{
					const char *item = entities::entinfo(e.type, e.attrs, false);
					if(item && *item)
					{
						defformatstring(ds)("@%s (%d)", item, e.type);
						part_text(d->abovehead(), ds, PART_TEXT, game::aboveheadfade, colour, 2, -10, 0, d);
					}
				}
			}
			if(isweap(g))
			{
				d->setweapstate(g, WEAP_S_SWITCH, WEAPSWITCHDELAY, lastmillis);
				d->ammo[g] = d->entid[g] = -1;
				if(d->weapselect != g)
				{
					d->lastweap = d->weapselect;
					d->weapselect = g;
				}
			}
			d->useitem(n, e.type, attr, e.attrs, sweap, lastmillis);
			game::spawneffect(PART_FIREBALL, pos, e.type == WEAPON ? weaptype[attr].colour : 0x6666FF, enttype[e.type].radius, 5);
			playsound(S_ITEMPICKUP, d->o, d);
			if(ents.inrange(r) && ents[r]->type == WEAPON)
			{
				gameentity &f = *(gameentity *)ents[r];
				attr = weapattr(f.attrs[0], sweap);
				if(isweap(attr)) projs::drop(d, attr, r, d == game::player1 || d->ai);
			}
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
			case TRIGGER: case TELEPORT: case PUSHER: if(e.attrs[3]) return e.attrs[3]; // fall through
			default: return enttype[e.type].radius;
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
			left = right = numindices/2;
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

	bool docacheclear = false;
	void clearentcache()
	{
		entcache.setsizenodelete(0);
		entcachedepth = -1;
		entcachemin = vec(1e16f, 1e16f, 1e16f);
		entcachemax = vec(-1e16f, -1e16f, -1e16f);
	}
	ICOMMAND(clearentcache, "", (void), clearentcache());

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

	bool allowuse(gameent *d, int n)
	{
		if(!d || !d->ai || !d->ai->hasprevnode(n)) switch(ents[n]->attrs[0])
		{
			case WP_COMMON: return true; break;
			case WP_PLAYER: if(d->type == ENT_PLAYER) return true; break;
			case WP_ENEMY: if(d->aitype >= AI_START) return true; break;
			case WP_LINKED: if(ents.inrange(d->aientity) && ents[n]->links.find(d->aientity) >= 0) return true; break;
			case WP_CAMERA: if(d == game::player1 && d->state == CS_SPECTATOR) return true; break;
		}
		return false;
	}

	int closestent(int type, const vec &pos, float mindist, bool links, gameent *d)
	{
		if(entcachedepth<0) buildentcache();

		entcachestack.setsizenodelete(0);

		int closest = -1;
		entcachenode *curnode = &entcache[0];
		#define CHECKCLOSEST(branch) do { \
			int n = curnode->childindex(branch); \
			extentity &e = *ents[n]; \
			if(e.type == type && (!links || !e.links.empty()) && allowuse(d, n)) \
			{ \
				float dist = e.o.squaredist(pos); \
				if(dist < mindist*mindist) { closest = n; mindist = sqrtf(dist); } \
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

	void findentswithin(int type, const vec &pos, float mindist, float maxdist, vector<int> &results)
	{
		float mindist2 = mindist*mindist, maxdist2 = maxdist*maxdist;

		if(entcachedepth<0) buildentcache();

		entcachestack.setsizenodelete(0);

		entcachenode *curnode = &entcache[0];
		#define CHECKWITHIN(branch) do { \
			int n = curnode->childindex(branch); \
			extentity &e = *ents[n]; \
			if(e.type == type && allowuse(NULL, n)) \
			{ \
				float dist = e.o.squaredist(pos); \
				if(dist > mindist2 && dist < maxdist2) results.add(n); \
			} \
		} while(0)
		for(;;)
		{
			int axis = curnode->axis();
			float dist1 = pos[axis] - curnode->split[0], dist2 = curnode->split[1] - pos[axis];
			if(dist1 >= maxdist)
			{
				if(dist2 < maxdist)
				{
					if(!curnode->isleaf(1)) { curnode = &entcache[curnode->childindex(1)]; continue; }
					CHECKWITHIN(1);
				}
			}
			else if(curnode->isleaf(0))
			{
				CHECKWITHIN(0);
				if(dist2 < maxdist)
				{
					if(!curnode->isleaf(1)) { curnode = &entcache[curnode->childindex(1)]; continue; }
					CHECKWITHIN(1);
				}
			}
			else
			{
				if(dist2 < maxdist)
				{
					if(!curnode->isleaf(1)) entcachestack.add(&entcache[curnode->childindex(1)]);
					else CHECKWITHIN(1);
				}
				curnode = &entcache[curnode->childindex(0)];
				continue;
			}
			if(entcachestack.empty()) return;
			curnode = entcachestack.pop();
		}
	}

	void avoidset::avoidnear(dynent *d, const vec &pos, float limit)
	{
		float limit2 = limit*limit;

		if(entcachedepth<0) buildentcache();

		entcachestack.setsizenodelete(0);

		entcachenode *curnode = &entcache[0];
		#define CHECKNEAR(branch) do { \
			int n = curnode->childindex(branch); \
			extentity &e = *ents[n]; \
			if(e.type == WAYPOINT && e.o.squaredist(pos) < limit2) add(d, n); \
		} while(0)
		for(;;)
		{
			int axis = curnode->axis();
			float dist1 = pos[axis] - curnode->split[0], dist2 = curnode->split[1] - pos[axis];
			if(dist1 >= limit)
			{
				if(dist2 < limit)
				{
					if(!curnode->isleaf(1)) { curnode = &entcache[curnode->childindex(1)]; continue; }
					CHECKNEAR(1);
				}
			}
			else if(curnode->isleaf(0))
			{
				CHECKNEAR(0);
				if(dist2 < limit)
				{
					if(!curnode->isleaf(1)) { curnode = &entcache[curnode->childindex(1)]; continue; }
					CHECKNEAR(1);
				}
			}
			else
			{
				if(dist2 < limit)
				{
					if(!curnode->isleaf(1)) entcachestack.add(&entcache[curnode->childindex(1)]);
					else CHECKNEAR(1);
				}
				curnode = &entcache[curnode->childindex(0)];
				continue;
			}
			if(entcachestack.empty()) return;
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
				float radius = (e.type == TRIGGER || e.type == TELEPORT || e.type == PUSHER) && e.attrs[3] ? e.attrs[3] : enttype[e.type].radius; \
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

	gameent *trigger = NULL;
	ICOMMAND(triggerclientnum, "", (), intret(trigger ? trigger->clientnum : -1));

	void runtrigger(int n, gameent *d, bool act = true)
	{
		gameentity &e = *(gameentity *)ents[n];
		if(lastmillis-e.lastuse >= triggertime(e)/2)
		{
			e.lastuse = lastmillis;
			switch(e.attrs[1])
			{
				case TR_TOGGLE: case TR_LINK: case TR_ONCE: case TR_EXIT:
				{ // wait for ack
					if(e.attrs[1] == TR_EXIT && !m_story(game::gamemode)) break;
					client::addmsg(SV_TRIGGER, "ri2", d->clientnum, n);
					break;
				}
				case TR_SCRIPT:
				{
					defformatstring(s)("on_trigger_%d", e.attrs[0]);
					trigger = d; RUNWORLD(s); trigger = NULL;
					break;
				}
				default: break;
			}
			if(act && e.attrs[2] == TA_ACTION) d->action[AC_USE] = false;
		}
	}

	void runtriggers(int n, gameent *d)
	{
		loopv(ents) if(ents[i]->type == TRIGGER && ents[i]->attrs[0] == n && ents[i]->attrs[2] == TA_MANUAL) runtrigger(i, d, false);
	}
	ICOMMAND(exectrigger, "i", (int *n), if(worldidents) runtriggers(*n, trigger ? trigger : game::player1));

	void execitem(int n, gameent *d, bool &tried)
	{
		gameentity &e = *(gameentity *)ents[n];
		switch(enttype[e.type].usetype)
		{
			case EU_ITEM: if(d->action[AC_USE] && !m_noitems(game::gamemode, game::mutators))
			{
				if(game::allowmove(d) && d->weapwaited(d->weapselect, lastmillis, d->skipwait(d->weapselect, lastmillis, WEAP_S_RELOAD)))
				{
					int sweap = m_spawnweapon(game::gamemode, game::mutators), attr = e.type == WEAPON ? weapattr(e.attrs[0], sweap) : e.attrs[0];
					if(d->canuse(e.type, attr, e.attrs, sweap, lastmillis, WEAP_S_RELOAD))
					{
						client::addmsg(SV_ITEMUSE, "ri3", d->clientnum, lastmillis-game::maptime, n);
						d->setweapstate(d->weapselect, WEAP_S_WAIT, WEAPSWITCHDELAY, lastmillis);
						d->action[AC_USE] = false;
					}
					else tried = true;
				}
				else tried = true;
			} break;
			case EU_AUTO: switch(e.type)
			{
				case TELEPORT:
				{
					if(lastmillis-e.lastuse >= triggertime(e)/2)
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
								int r = e.type == TELEPORT ? rnd(teleports.length()) : 0;
								gameentity &f = *(gameentity *)ents[teleports[r]];
								d->timeinair = 0;
								d->falling = vec(0, 0, 0);
								d->o = vec(f.o).add(vec(0, 0, d->height*0.5f));
								d->yaw = f.attrs[0] < 0 ? (lastmillis/5)%360 : f.attrs[0];
								d->pitch = f.attrs[1];
								if(physics::entinmap(d, true))
								{
									float mag = m_speedscale(max(d->vel.magnitude(), f.attrs[2] ? float(f.attrs[2]) : 50.f));
									vecfromyawpitch(d->yaw, d->pitch, 1, 0, d->vel);
									d->vel.mul(mag);
									game::fixfullrange(d->yaw, d->pitch, d->roll, true);
									f.lastuse = f.lastemit = e.lastemit;
									execlink(d, n, true); execlink(d, teleports[r], true);
									if(d == game::player1) game::resetcamera();
									teleported = true;
									break;
								}
								teleports.remove(r); // must've really sucked, try another one
							}
							if(!teleported) game::suicide(d, HIT_SPAWN);
						}
					}
					break;
				}
				case PUSHER:
				{
					float mag = m_speedscale(10.f);
					if(e.attrs[4] && e.attrs[4] < e.attrs[3])
					{
						vec m = vec(d->o).sub(vec(0, 0, d->height*0.5f));
						float dist = m.dist(e.o);
						if(dist >= e.attrs[4]) mag *= 1.f-clamp((dist-e.attrs[4])/float(e.attrs[3]-e.attrs[4]), 0.f, 1.f);
					}
					vec dir = vec((int)(char)e.attrs[2], (int)(char)e.attrs[1], (int)(char)e.attrs[0]).mul(mag);
					if(d->ai) d->ai->becareful = true;
					d->falling = vec(0, 0, 0);
					d->physstate = PHYS_FALL;
					if(!d->timeinair) d->timeinair = 1;
					loopk(3)
					{
						if((d->vel.v[k] > 0.f && dir.v[k] < 0.f) || (d->vel.v[k] < 0.f && dir.v[k] > 0.f) || (fabs(dir.v[k]) > fabs(d->vel.v[k])))
							d->vel.v[k] = dir.v[k];
					}
					e.lastuse = e.lastemit = lastmillis; execlink(d, n, true);
					break;
				}
				case TRIGGER:
				{
					if((e.attrs[2] == TA_ACTION && d->action[AC_USE] && d == game::player1) || e.attrs[2] == TA_AUTO) runtrigger(n, d);
					break;
				}
			} break;
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
			if(tried && d->action[AC_USE] && d == game::player1)
			{
				playsound(S_ERROR, d->o, d);
				d->action[AC_USE] = false;
			}
		}
		if(m_ctf(game::gamemode)) ctf::checkflags(d);
	}

	void putitems(ucharbuf &p)
	{
		loopv(ents) if(enttype[ents[i]->type].usetype == EU_ITEM || ents[i]->type == PLAYERSTART || ents[i]->type == ACTOR || ents[i]->type == TRIGGER)
		{
			gameentity &e = *(gameentity *)ents[i];
			putint(p, i);
			putint(p, int(e.type));
			putint(p, e.attrs.length());
			putint(p, e.kin.length());
			loopvj(e.attrs) putint(p, e.attrs[j]);
			loopvj(e.kin) putint(p, e.kin[j]);
		}
	}

	void setspawn(int n, int m)
	{
		if(ents.inrange(n))
		{
			gameentity &e = *(gameentity *)ents[n];
			bool on = m%2, spawned = e.spawned;
			if((e.spawned = on) == true) e.lastspawn = lastmillis;
			if(e.type == TRIGGER)
			{
				if((m >= 2 || e.lastemit <= 0 || e.spawned != spawned) && (e.attrs[1] == TR_TOGGLE || e.attrs[1] == TR_LINK || e.attrs[1] == TR_ONCE))
				{
					e.lastemit = m <= 1 ? lastmillis-(e.lastemit ? max(triggertime(e)-(lastmillis-e.lastemit), 0) : triggertime(e)) : -1;
					execlink(NULL, n, false);
					loopv(e.kin) if(ents.inrange(e.kin[i]))
					{
						gameentity &f = *(gameentity *)ents[e.kin[i]];
						f.spawned = e.spawned; f.lastemit = e.lastemit;
						execlink(NULL, e.kin[i], false, n);
					}
				}
			}
			else if(enttype[e.type].usetype == EU_ITEM)
			{
				loopv(projs::projs)
				{
					projent &proj = *projs::projs[i];
					if(proj.projtype != PRJ_ENT || proj.id != n || !ents.inrange(proj.id)) continue;
					int sweap = m_spawnweapon(game::gamemode, game::mutators), attr = entities::ents[proj.id]->type == WEAPON ? weapattr(entities::ents[proj.id]->attrs[0], sweap) : entities::ents[proj.id]->attrs[0],
						colour = entities::ents[proj.id]->type == WEAPON ? weaptype[attr].colour : 0x6666FF;
					game::spawneffect(PART_FIREBALL, proj.o, colour, enttype[ents[proj.id]->type].radius, 5);
					proj.beenused = true;
					proj.state = CS_DEAD;
				}
				gameent *d = NULL;
				loopi(game::numdynents()) if((d = (gameent *)game::iterdynents(i)) && d->type == ENT_PLAYER)
				{
					loopk(WEAP_MAX) if(d->entid[k] == n) d->entid[k] = -1;
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
		return (showentinfo || game::player1->state == CS_EDITING) && (!enttype[e.type].noisy || showentnoisy >= 2 || (showentnoisy && game::player1->state == CS_EDITING));
	}

	void fixentity(int n)
	{
		gameentity &e = *(gameentity *)ents[n];
		int num = max(5, enttype[e.type].numattrs);
		while(e.attrs.length() < num) e.attrs.add(0);
		while(e.attrs.length() > num) e.attrs.pop();
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
				fixentity(ent);
			}
			else continue;
		}
		if(issound(e.schan))
		{
			removesound(e.schan); e.schan = -1; // prevent clipping when moving around
			if(e.type == MAPSOUND) e.lastemit = lastmillis+1000;
		}
		switch(e.type)
		{
			case MAPMODEL:
				while(e.attrs[1] < 0) e.attrs[1] += 360;
				while(e.attrs[1] >= 360) e.attrs[1] -= 360;
				while(e.attrs[2] < -90) e.attrs[2] += 180;
				while(e.attrs[2] > 90) e.attrs[2] -= 180;
				while(e.attrs[3] < 0) e.attrs[3] += 360;
				while(e.attrs[3] >= 360) e.attrs[3] -= 360;
			case PARTICLES:
			case MAPSOUND:
			case LIGHTFX:
			{
				loopv(e.links) if(ents.inrange(e.links[i]) && ents[e.links[i]]->type == TRIGGER)
				{
					e.lastemit = ents[e.links[i]]->lastemit;
					e.spawned = TRIGSTATE(ents[e.links[i]]->spawned, ents[e.links[i]]->attrs[4]);
				}
				break;
			}
			case PUSHER:
			{
				if(e.attrs[3] < 0) e.attrs[3] = 0;
				if(e.attrs[4] < 0 || e.attrs[4] >= e.attrs[3]) e.attrs[4] = 0;
				break;
			}
			case TRIGGER:
			{
				while(e.attrs[1] < 0) e.attrs[1] += TR_MAX;
				while(e.attrs[1] >= TR_MAX) e.attrs[1] -= TR_MAX;
				while(e.attrs[2] < 0) e.attrs[2] += TA_MAX;
				while(e.attrs[2] >= TA_MAX) e.attrs[2] -= TA_MAX;
				while(e.attrs[4] < 0) e.attrs[4] += 4;
				while(e.attrs[4] >= 4) e.attrs[4] -= 4;
				if(e.attrs[4] >= 2)
				{
					while(e.attrs[0] < 0) e.attrs[0] += TRIGGERIDS+1;
					while(e.attrs[0] > TRIGGERIDS) e.attrs[0] -= TRIGGERIDS+1;
				}
				loopv(e.links) if(ents.inrange(e.links[i]) && (ents[e.links[i]]->type == MAPMODEL || ents[e.links[i]]->type == PARTICLES || ents[e.links[i]]->type == MAPSOUND || ents[e.links[i]]->type == LIGHTFX))
				{
					ents[e.links[i]]->lastemit = e.lastemit;
					ents[e.links[i]]->spawned = TRIGSTATE(e.spawned, e.attrs[4]);
				}
				break;
			}
			case WEAPON:
				while(e.attrs[0] < 0) e.attrs[0] += WEAP_TOTAL; // don't allow superimposed weaps
				while(e.attrs[0] >= WEAP_TOTAL) e.attrs[0] -= WEAP_TOTAL;
				while(e.attrs[2] <= -G_MAX) e.attrs[2] += G_MAX*2;
				while(e.attrs[2] >= G_MAX) e.attrs[2] -= G_MAX*2;
				break;
			case PLAYERSTART:
				while(e.attrs[0] < 0) e.attrs[0] += TEAM_MAX;
				while(e.attrs[0] >= TEAM_MAX) e.attrs[0] -= TEAM_MAX;
				while(e.attrs[1] < 0) e.attrs[1] += 360;
				while(e.attrs[1] >= 360) e.attrs[1] -= 360;
				while(e.attrs[2] < -90) e.attrs[2] += 180;
				while(e.attrs[2] > 90) e.attrs[2] -= 180;
				while(e.attrs[3] <= -G_MAX) e.attrs[3] += G_MAX*2;
				while(e.attrs[3] >= G_MAX) e.attrs[3] -= G_MAX*2;
				break;
			case ACTOR:
				while(e.attrs[0] < 0) e.attrs[0] += AI_MAX;
				while(e.attrs[0] >= AI_MAX) e.attrs[0] -= AI_MAX;
				while(e.attrs[1] < 0) e.attrs[1] += 360;
				while(e.attrs[1] >= 360) e.attrs[1] -= 360;
				while(e.attrs[2] < -90) e.attrs[2] += 180;
				while(e.attrs[2] > 90) e.attrs[2] -= 180;
				while(e.attrs[3] <= -G_MAX) e.attrs[3] += G_MAX*2;
				while(e.attrs[3] >= G_MAX) e.attrs[3] -= G_MAX*2;
				break;
			case FLAG:
				while(e.attrs[0] < 0) e.attrs[0] += TEAM_MAX;
				while(e.attrs[0] >= TEAM_MAX) e.attrs[0] -= TEAM_MAX;
				while(e.attrs[1] < 0) e.attrs[1] += 360;
				while(e.attrs[1] >= 360) e.attrs[1] -= 360;
				while(e.attrs[2] < -90) e.attrs[2] += 180;
				while(e.attrs[2] > 90) e.attrs[2] -= 180;
				while(e.attrs[3] <= -G_MAX) e.attrs[3] += G_MAX*2;
				while(e.attrs[3] >= G_MAX) e.attrs[3] -= G_MAX*2;
				break;
			case TELEPORT:
				while(e.attrs[0] < -1) e.attrs[0] += 361;
				while(e.attrs[0] >= 360) e.attrs[0] -= 360;
				while(e.attrs[2] < -90) e.attrs[2] += 180;
				while(e.attrs[2] > 90) e.attrs[2] -= 180;
				break;
			case WAYPOINT:
				while(e.attrs[0] < 0) e.attrs[0] += WP_MAX;
				while(e.attrs[0] >= WP_MAX) e.attrs[0] -= WP_MAX;
				while(e.attrs[1] < 0) e.attrs[1] += WP_S_MAX;
				while(e.attrs[1] >= WP_S_MAX) e.attrs[1] -= WP_S_MAX;
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
	void execlink(gameent *d, int index, bool local, int ignore)
	{
		if(ents.inrange(index) && maylink(ents[index]->type))
		{
			bool commit = false;
			loopv(ents) if(ents[i]->links.find(index) >= 0)
			{
				if(ents.inrange(ignore) && ents[ignore]->links.find(index) >= 0) continue;
				bool both = ents[index]->links.find(i) >= 0;
				switch(ents[i]->type)
				{
					case MAPMODEL:
					{
						ents[i]->lastemit = ents[index]->lastemit;
						if(ents[index]->type == TRIGGER) ents[i]->spawned = TRIGSTATE(ents[index]->spawned, ents[index]->attrs[4]);
						break;
					}
					case LIGHTFX:
					case PARTICLES:
					{
						ents[i]->lastemit = ents[index]->lastemit;
						if(ents[index]->type == TRIGGER) ents[i]->spawned = TRIGSTATE(ents[index]->spawned, ents[index]->attrs[4]);
						else if(local) commit = true;
						break;
					}
					case MAPSOUND:
					{
						ents[i]->lastemit = ents[index]->lastemit;
						if(ents[index]->type == TRIGGER) ents[i]->spawned = TRIGSTATE(ents[index]->spawned, ents[index]->attrs[4]);
						else if(local) commit = true;
						if(mapsounds.inrange(ents[i]->attrs[0]) && !issound(((gameentity *)ents[i])->schan))
						{
							int flags = SND_MAP;
							if(ents[i]->attrs[4]&SND_NOATTEN) flags |= SND_NOATTEN;
							if(ents[i]->attrs[4]&SND_NODELAY) flags |= SND_NODELAY;
							if(ents[i]->attrs[4]&SND_NOCULL) flags |= SND_NOCULL;
							if(ents[i]->attrs[4]&SND_NOPAN) flags |= SND_NOPAN;
							playsound(ents[i]->attrs[0], both ? ents[i]->o : ents[index]->o, NULL, flags, ents[i]->attrs[3], ents[i]->attrs[1], ents[i]->attrs[2], &((gameentity *)ents[i])->schan);
						}
						break;
					}
					default: break;
				}
			}
			if(d && commit) client::addmsg(SV_EXECLINK, "ri2", d->clientnum, index);
		}
	}

	bool tryspawn(dynent *d, const vec &o, short yaw, short pitch)
	{
		d->yaw = yaw;
		d->pitch = pitch;
		d->roll = 0;
		d->o = vec(o).add(vec(0, 0, d->height+1));
		game::fixrange(d->yaw, d->pitch);
		return physics::entinmap(d, true);
	}

	void spawnplayer(gameent *d, int ent, bool suicide)
	{
		if(ent >= 0 && ents.inrange(ent))
		{
			switch(ents[ent]->type)
			{
				case ACTOR: if(d->type == ENT_PLAYER) break;
				case PLAYERSTART: if(tryspawn(d, ents[ent]->o, ents[ent]->attrs[1], ents[ent]->attrs[2])) return;
				default: if(tryspawn(d, ents[ent]->o, rnd(360), 0)) return;
			}
		}
		else if(d->type == ENT_PLAYER)
		{
			loopk(3)
			{
				vector<int> spawns;
				switch(k)
				{
					case 0: if(m_fight(game::gamemode) && m_team(game::gamemode, game::mutators) && !m_stf(game::gamemode))
								loopv(ents) if(ents[i]->type == PLAYERSTART && ents[i]->attrs[0] == d->team) spawns.add(i);
					case 1: if(spawns.empty()) loopv(ents) if(ents[i]->type == PLAYERSTART) spawns.add(i);
					case 2: if(spawns.empty()) loopv(ents) if(ents[i]->type == WEAPON) spawns.add(i);
					default: break;
				}
				while(!spawns.empty())
				{
					int r = rnd(spawns.length());
					gameentity &e = *(gameentity *)ents[spawns[r]];
					if(tryspawn(d, e.o, e.type == PLAYERSTART ? e.attrs[1] : rnd(360), e.type == PLAYERSTART ? e.attrs[2] : 0))
						return;
					spawns.remove(r); // must've really sucked, try another one
				}
			}
			d->yaw = d->pitch = d->roll = 0;
			d->o.x = d->o.y = d->o.z = getworldsize();
			d->o.x *= 0.5f; d->o.y *= 0.5f;
			if(physics::entinmap(d, true)) return;
		}
		if(!m_edit(game::gamemode) && m_play(game::gamemode) && suicide) game::suicide(d, HIT_SPAWN);
	}

	void editent(int i)
	{
		extentity &e = *ents[i];
		fixentity(i);
		if(m_edit(game::gamemode)) client::addmsg(SV_EDITENT, "ri5iv", i, (int)(e.o.x*DMF), (int)(e.o.y*DMF), (int)(e.o.z*DMF), e.type, e.attrs.length(), e.attrs.length(), e.attrs.getbuf()); // FIXME
		docacheclear = true;
	}

	float dropheight(extentity &e)
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
					if(local && m_edit(game::gamemode)) client::addmsg(SV_EDITLINK, "ri3", 0, index, node);
					if(verbose > 2) conoutf("\fdentity %s (%d) and %s (%d) delinked", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
					return true;
				}
				else if(toggle && canlink(node, index))
				{
					f.links.add(index);
					if(recip && (h = e.links.find(node)) < 0) e.links.add(node);
					fixentity(node);
					if(local && m_edit(game::gamemode)) client::addmsg(SV_EDITLINK, "ri3", 1, node, index);
					if(verbose > 2) conoutf("\fdentity %s (%d) and %s (%d) linked", enttype[ents[node]->type].name, node, enttype[ents[index]->type].name, index);
					return true;
				}
			}
			else if(toggle && canlink(node, index) && (g = f.links.find(index)) >= 0)
			{
				f.links.remove(g);
				if(recip && (h = e.links.find(node)) >= 0) e.links.remove(h);
				fixentity(node);
				if(local && m_edit(game::gamemode)) client::addmsg(SV_EDITLINK, "ri3", 0, node, index);
				if(verbose > 2) conoutf("\fdentity %s (%d) and %s (%d) delinked", enttype[ents[node]->type].name, node, enttype[ents[index]->type].name, index);
				return true;
			}
			else if(toggle || add)
			{
				e.links.add(node);
				if(recip && (h = f.links.find(index)) < 0) f.links.add(index);
				fixentity(index);
				if(local && m_edit(game::gamemode)) client::addmsg(SV_EDITLINK, "ri3", 1, index, node);
				if(verbose > 2) conoutf("\fdentity %s (%d) and %s (%d) linked", enttype[ents[index]->type].name, index, enttype[ents[node]->type].name, node);
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

	float route(int node, int goal, vector<int> &route, const avoidset &obstacles, gameent *d, bool check)
	{
		if(!ents.inrange(node) || !ents.inrange(goal) || ents[goal]->type != ents[node]->type || goal == node || ents[node]->links.empty())
			return 0;

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

		if(d)
		{
			if(d->ai) loopi(ai::NUMPREVNODES) if(d->ai->prevnodes[i] != node && nodes.inrange(d->ai->prevnodes[i]))
			{
				nodes[d->ai->prevnodes[i]].id = routeid;
				nodes[d->ai->prevnodes[i]].curscore = -1.f;
				nodes[d->ai->prevnodes[i]].estscore = 0.f;
			}
			if(check)
			{
				vec pos = d->feetpos();
				loopavoid(obstacles, d, { if(ents.inrange(ent) && ents[ent]->type == ents[node]->type)
				{
					if(ent != node && ent != goal && ents[node]->links.find(ent) < 0 && ents[goal]->links.find(ent) < 0)
					{
						nodes[ent].id = routeid;
						nodes[ent].curscore = -1.f;
						nodes[ent].estscore = 0.f;
					}
				}});
			}
		}

		nodes[node].id = routeid;
		nodes[node].curscore = 0.f;
		nodes[node].estscore = 0.f;
		nodes[node].prev = NULL;
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
				if(ents.inrange(link) && ents[link]->type == ents[node]->type && (link == node || link == goal || !ents[link]->links.empty()) && allowuse(d, link))
				{
					linkq &n = nodes[link];
					float curscore = prevscore + ents[link]->o.dist(ent.o);
					if(n.id == routeid && curscore >= n.curscore) continue;
					n.curscore = curscore;
					n.prev = m;
					if(n.id != routeid)
					{
						n.estscore = ents[link]->o.dist(ents[goal]->o);
						if(n.estscore <= float(enttype[ents[link]->type].radius*4) && (lowest < 0 || n.estscore < nodes[lowest].estscore))
							lowest = link;
						n.id = routeid;
						if(link == goal) goto foundgoal;
						queue.add(&n);
					}
				}
			}
		}
		foundgoal:

		routeid++;
		float score = 0;
		if(lowest >= 0) // otherwise nothing got there
		{
			for(linkq *m = &nodes[lowest]; m != NULL; m = m->prev)
				route.add(m - &nodes[0]); // just keep it stored backward
			if(!route.empty()) score = nodes[lowest].score();
		}

		if(verbose >= 3)
			conoutf("\fdroute %d to %d (%d) generated %d nodes in %fs", node, goal, lowest, route.length(), (SDL_GetTicks()-routestart)/1000.0f);

		return score;
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

	bool clipped(const vec &o, bool aiclip)
	{
		cube &c = lookupcube(o.x, o.y, o.z);
		//if(isentirelysolid(c)) return true;
		int material = c.ext ? c.ext->material : MAT_AIR, clipmat = material&MATF_CLIP;
		if(clipmat == MAT_CLIP || (aiclip && clipmat == MAT_AICLIP)) return true;
		if(int(material&MATF_FLAGS) == MAT_DEATH) return true;
		if(int(material&MATF_VOLUME) == MAT_LAVA) return true;
		return false;
	}

	void entitycheck(gameent *d)
	{
		int cleanairnodes = 0;
		if(d->state == CS_ALIVE)
		{
			vec v = d->feetpos();
			bool clip = clipped(v, true), shoulddrop = (m_play(game::gamemode) || dropwaypoints) && !d->ai && !clip;
			float dist = float(shoulddrop ? enttype[WAYPOINT].radius : (d->ai ? ai::JUMPMIN : ai::NEARDIST));
			int curnode = closestent(WAYPOINT, v, dist, false, d), prevnode = d->lastnode;

			if(!ents.inrange(curnode) && shoulddrop)
			{
				int cmds = WP_F_NONE;
				if(physics::iscrouching(d)) cmds |= WP_F_CROUCH;
				curnode = ents.length();
				static vector<int> wpattrs;
				if(wpattrs.empty()) { wpattrs.add(int(WP_COMMON)); wpattrs.add(int(WP_S_NONE)); wpattrs.add(0); wpattrs.add(0); wpattrs.add(cmds); }
				newentity(v, WAYPOINT, wpattrs);
				if(d->timeinair) d->airnodes.add(curnode);
			}

			if(ents.inrange(curnode))
			{
				if(shoulddrop && ents.inrange(d->lastnode) && d->lastnode != curnode)
					entitylink(d->lastnode, curnode, !d->timeinair && !d->onladder);
				d->lastnode = curnode;
			}
			else if(!ents.inrange(d->lastnode) || entities::ents[d->lastnode]->o.squaredist(v) > ai::CLOSEDIST*ai::CLOSEDIST)
				d->lastnode = closestent(WAYPOINT, v, ai::FARDIST, false, d);

			if(clip) cleanairnodes = 2;
			else if(!d->timeinair) cleanairnodes = 1;
			if(d->ai && ents.inrange(prevnode) && d->lastnode != prevnode) d->ai->addprevnode(prevnode);
		}
		else
		{
			d->lastnode = -1;
			cleanairnodes = 2;
		}

		if(cleanairnodes && !d->airnodes.empty())
		{
			if(cleanairnodes > 1)
			{
				loopv(d->airnodes) if(ents.inrange(d->airnodes[i]))
					ents[d->airnodes[i]]->type = NOTUSED;
				loopvk(ents) if(!ents[k]->links.empty())
				{
					loopvrev(ents[k]->links) if(d->airnodes.find(ents[k]->links[i]) >= 0)
						ents[k]->links.remove(i);
				}
			}
			d->airnodes.setsizenodelete(0);
		}
	}

	void readent(stream *g, int mtype, int mver, char *gid, int gver, int id)
	{
		gameentity &f = *(gameentity *)ents[id];
		f.mark = 0;
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
				// 7	SPOTLIGHT		7	LIGHTFX
				// 						8	SUNLIGHT
				case 1: case 2: case 3: case 4: case 5: case 6: case 7:
				{
					break;
				}

				// 8	I_SHELLS		9	WEAPON		WEAP_SHOTGUN
				// 9	I_BULLETS		9	WEAPON		WEAP_SMG
				// 10	I_ROCKETS		9	WEAPON		WEAP_PLASMA
				// 11	I_ROUNDS		9	WEAPON		WEAP_RIFLE
				// 12	I_GL			9	WEAPON		WEAP_GRENADE
				// 13	I_CARTRIDGES	9	WEAPON		WEAP_PISTOL
				case 8: case 9: case 10: case 11: case 12: case 13:
				{
					int weap = f.type-8, weapmap[6] = {
						WEAP_SHOTGUN, WEAP_SMG, WEAP_PLASMA, WEAP_RIFLE, WEAP_GRENADE, WEAP_PISTOL
					};

					if(weap >= 0 && weap <= 5)
					{
						f.type = WEAPON;
						f.attrs[0] = weapmap[weap];
						f.attrs[1] = 0;
					}
					else f.type = NOTUSED;
					break;
				}
				// 18	I_QUAD			9	WEAPON		WEAP_FLAMER
				case 18:
				{
					f.type = WEAPON;
					f.attrs[0] = WEAP_FLAMER;
					f.attrs[1] = 0;
					break;
				}

				// 19	TELEPORT		10	TELEPORT
				// 20	TELEDEST		10	TELEPORT (linked)
				case 19: case 20:
				{
					if(f.type == 20)
					{
						f.mark = f.attrs[1]+1; // needs translating later
						f.attrs[1] = -1;
					}
					else
					{
						f.mark = -(f.attrs[0]+1);
						f.attrs[0] = -1;
					}
					f.attrs[2] = f.attrs[3] = f.attrs[4] = 0;
					f.type = TELEPORT;
					break;
				}
				// 21	MONSTER			11	NOTUSED
				case 21:
				{
					f.type = NOTUSED;
					break;
				}
				// 22	CARROT			12	TRIGGER		0
				case 22:
				{
					f.type = NOTUSED;
					f.attrs[0] = f.attrs[1] = f.attrs[2] = f.attrs[3] = f.attrs[4] = 0;
					break;
				}
				// 23	JUMPPAD			13	PUSHER
				case 23:
				{
					f.type = PUSHER;
					break;
				}
				// 24	BASE			14	FLAG		1:idx		TEAM_NEUTRAL
				case 24:
				{
					f.type = FLAG;
					if(f.attrs[0] < 0) f.attrs[0] = 0;
					f.attrs[1] = TEAM_NEUTRAL; // spawn as neutrals
					break;
				}
				// 25	RESPAWNPOINT	15	CHECKPOINT
				case 25:
				{
					f.type = CHECKPOINT;
					break;
				}
				// 30	FLAG			14	FLAG		#			2:team
				case 30:
				{
					f.type = FLAG;
					f.attrs[0] = 0;
					if(f.attrs[1] <= 0) f.attrs[1] = -1; // needs a team
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

	void writeent(stream *g, int id)
	{
	}

	void importentities(int mtype, int mver, int gver)
	{
		int flag = 0, teams[TEAM_NUM] = { 0, 0, 0, 0 };
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(verbose) progress(float(i)/float(ents.length()), "importing entities...");

			switch(e.type)
			{
				case TELEPORT:
				{
					if(e.mark > 0) // translate teledest to teleport and link them appropriately
					{
						loopvj(ents) if(j != i) // find linked teleport(s)
						{
							gameentity &f = *(gameentity *)ents[j];
							if(f.type == TELEPORT && -f.mark == e.mark)
							{
								if(verbose) conoutf("\frWARNING: teledest %d and teleport %d linked automatically", i, j);
								f.links.add(i);
							}
						}
					}
					break;
				}
				case WEAPON:
				{
					float mindist = float(enttype[WEAPON].radius*enttype[WEAPON].radius*6);
					int weaps[WEAP_MAX];
					loopj(WEAP_MAX) weaps[j] = j != e.attrs[0] ? 0 : 1;
					loopvj(ents) if(j != i)
					{
						gameentity &f = *(gameentity *)ents[j];
						if(f.type == WEAPON && e.o.squaredist(f.o) <= mindist && isweap(f.attrs[0]))
						{
							weaps[f.attrs[0]]++;
							f.type = NOTUSED;
							if(verbose) conoutf("\frWARNING: culled tightly packed weapon %d [%d]", j, f.attrs[0]);
						}
					}
					int best = e.attrs[0];
					loopj(WEAP_MAX) if(weaps[j] > weaps[best])
						best = j;
					e.attrs[0] = best;
					break;
				}
				case FLAG: // replace bases/neutral flags near team flags
				{
					if(valteam(e.attrs[1], TEAM_FIRST)) teams[e.attrs[1]-TEAM_FIRST]++;
					else if(e.attrs[1] == TEAM_NEUTRAL)
					{
						int dest = -1;

						loopvj(ents) if(j != i)
						{
							gameentity &f = *(gameentity *)ents[j];

							if(f.type == FLAG && f.attrs[1] != TEAM_NEUTRAL &&
								(!ents.inrange(dest) || e.o.dist(f.o) < ents[dest]->o.dist(f.o)) &&
									e.o.dist(f.o) <= enttype[FLAG].radius*4.f)
										dest = j;
						}

						if(ents.inrange(dest))
						{
							gameentity &f = *(gameentity *)ents[dest];
							if(verbose) conoutf("\frWARNING: old base %d (%d, %d) replaced with flag %d (%d, %d)", i, e.attrs[0], e.attrs[1], dest, f.attrs[0], f.attrs[1]);
							if(!f.attrs[0]) f.attrs[0] = e.attrs[0]; // give it the old base idx
							e.type = NOTUSED;
						}
						else if(e.attrs[0] > flag) flag = e.attrs[0]; // find the highest idx
					}
					break;
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
					if(e.mark > 0) // second pass teledest translation
					{
						int dest = -1;
						loopvj(ents) if(j != i) // see if this teledest is sitting on top of a teleport already
						{
							gameentity &f = *(gameentity *)ents[j];

							if(f.type == TELEPORT && f.mark < 0 &&
								(!ents.inrange(dest) || e.o.dist(f.o) < ents[dest]->o.dist(f.o)) &&
									e.o.dist(f.o) <= enttype[TELEPORT].radius*4.f)
										dest = j;
						}
						if(ents.inrange(dest))
						{
							gameentity &f = *(gameentity *)ents[dest];
							if(verbose) conoutf("\frWARNING: replaced teledest %d with closest teleport %d", i, dest);
							f.attrs[0] = e.attrs[0]; // copy the yaw
							loopvk(e.links) if(f.links.find(e.links[k]) < 0) f.links.add(e.links[k]);
							loopvj(ents) if(j != i && j != dest)
							{
								gameentity &g = *(gameentity *)ents[j];
								if(g.type == TELEPORT)
								{
									int link = g.links.find(i);
									if(link >= 0)
									{
										g.links.remove(link);
										if(g.links.find(dest) < 0) g.links.add(dest);
										if(verbose) conoutf("\frWARNING: imported link to teledest %d to teleport %d", i, j);
									}
								}
							}
							e.type = NOTUSED; // get rid of ye olde teledest
							e.links.setsize(0);
							break;
						}
						else if(verbose) conoutf("\frWARNING: teledest %d has become a teleport", i);
					}
					break;
				}
				case FLAG:
				{
					if(!e.attrs[0]) e.attrs[0] = ++flag; // assign a sane idx
					if(!valteam(e.attrs[1], TEAM_NEUTRAL)) // assign a team
					{
						int lowest = -1;
						loopk(TEAM_NUM) if(lowest<0 || teams[k] < teams[lowest]) lowest = i;
						e.attrs[1] = lowest+TEAM_FIRST;
						teams[lowest]++;
					}
					break;
				}
			}
		}
	}

	void updateoldentities(int mtype, int mver, int gver)
	{
		loopvj(ents)
		{
			gameentity &e = *(gameentity *)ents[j];
			if(verbose) progress(float(j)/float(ents.length()), "updating old entities...");
			switch(e.type)
			{
				case LIGHTFX:
				{
					if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && gver <= 159))
					{
						e.attrs[1] = e.attrs[0];
						e.attrs[0] = LFX_SPOTLIGHT;
						e.attrs[2] = e.attrs[3] = e.attrs[4] = 0;
					}
					break;
				}
				case PLAYERSTART:
				{
					if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && gver <= 158))
					{
						short yaw = e.attrs[0];
						e.attrs[0] = e.attrs[1];
						e.attrs[1] = yaw;
						e.attrs[2] = e.attrs[3] = e.attrs[4] = 0;
					}
					break;
				}
				case PARTICLES:
				{
					if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && mver <= 36))
					{
						switch(e.attrs[0])
						{
							case 0: if(e.attrs[3] <= 0) break;
							case 4: case 7: case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15: case 5: case 6:
								e.attrs[3] = (((e.attrs[3]&0xF)<<4)|((e.attrs[3]&0xF0)<<8)|((e.attrs[3]&0xF00)<<12))+0x0F0F0F;
								if(e.attrs[0] != 5 && e.attrs[0] != 6) break;
							case 3:
								e.attrs[2] = (((e.attrs[2]&0xF)<<4)|((e.attrs[2]&0xF0)<<8)|((e.attrs[2]&0xF00)<<12))+0x0F0F0F; break;
							default: break;
						}
					}
					break;
				}
				case TELEPORT:
				{
					if(mtype == MAP_OCTA)
					{
						e.attrs[2] = 100; // give a push
						e.attrs[4] = e.attrs[1] >= 0 ? 0x2CE : 0;
						e.attrs[1] = e.attrs[3] = 0;
						e.o.z += 8; // teleport in BF is at middle
						if(e.attrs[0] >= 0 && clipped(e.o))
						{
							vec dir;
							vecfromyawpitch(e.attrs[0], e.attrs[1], 1, 0, dir);
							e.o.add(dir);
						}
						e.mark = 0;
					}
					break;
				}
				case WEAPON:
				{
					if(mtype == MAP_BFGZ && gver <= 90)
					{ // move grenade to the end of the weapon array
						if(e.attrs[0] >= 4) e.attrs[0]--;
						else if(e.attrs[0] == 3) e.attrs[0] = WEAP_GRENADE;
					}
					if(mtype == MAP_BFGZ && gver <= 97 && e.attrs[0] >= 4)
						e.attrs[0]++; // add in pistol
					if(mtype != MAP_BFGZ || gver <= 112) e.attrs[1] = 0;
					break;
				}
				case TRIGGER:
				{
					if(mtype == MAP_BFGZ && gver <= 158) e.attrs[4] = 0;
					break;
				}
				case PUSHER:
				{
					if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && gver <= 95))
						e.attrs[0] = int(e.attrs[0]*1.25f);
					break;
				}
				case FLAG:
				{
					if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && gver <= 158))
					{
						e.attrs[0] = e.attrs[1];
						e.attrs[1] = e.attrs[2];
						e.attrs[2] = e.attrs[3];
						e.attrs[3] = e.attrs[4] = 0;
					}
					break;
				}
				case WAYPOINT:
				{
					if(!m_edit(game::gamemode) && clipped(e.o, true)) e.type = NOTUSED;
					else if(mtype == MAP_BFGZ)
					{
						if(gver <= 90) e.attrs[0] = e.attrs[1] = e.attrs[2] = e.attrs[3] = e.attrs[4] = 0;
						else if(gver <= 159)
						{
							e.attrs[4] = e.attrs[0];
							e.attrs[0] = WP_COMMON;
							e.attrs[1] = WP_S_NONE;
							e.attrs[2] = e.attrs[3] = 0;
						}
					}
					break;
				}
				default: break;
			}
		}
	}

	void importwaypoints(int mtype, int mver, int gver)
	{
		const char *mname = mapname;
		if(!mname || !*mname) return;
		string wptname;
		formatstring(wptname)("%s.wpt", mname);

		stream *f = opengzfile(wptname, "rb");
		if(!f) return;
		char magic[4];
		if(f->read(magic, 4) < 4 || memcmp(magic, "OWPT", 4)) { delete f; return; }

		int numents = ents.length()-1; // -1 because OCTA waypoints count from 1 upward
		ushort numwp = f->getlil<ushort>();
		loopi(numwp)
		{
			if(f->end()) break;
			vec o;
			o.x = f->getlil<float>();
			o.y = f->getlil<float>();
			o.z = f->getlil<float>();
			extentity *e = NULL;
			e = newent();
			ents.add(e);
			loopk(5) e->attrs.add(0);
			e->type = !m_edit(game::gamemode) && clipped(o, true) ? NOTUSED : WAYPOINT;
			e->o = o;
			int numlinks = clamp(f->getchar(), 0, 6);
			loopi(numlinks) e->links.add(numents+f->getlil<ushort>());
		}
		delete f;
		conoutf("loaded %d waypoints from %s", numwp, wptname);
	}

	void initents(stream *g, int mtype, int mver, char *gid, int gver)
	{
		if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && mver <= 36)) loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			int num = max(5, enttype[e.type].numattrs);
			while(e.attrs.length() < num) e.attrs.add(0);
			while(e.attrs.length() > num) e.attrs.pop();
		}

		if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && gver <= 49)) importentities(mtype, mver, gver);
		if(mtype == MAP_OCTA || (mtype == MAP_BFGZ && gver < GAMEVERSION)) updateoldentities(mtype, mver, gver);
		if(mtype == MAP_OCTA) importwaypoints(mtype, mver, gver);
		loopv(ents) fixentity(i);
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(enttype[e.type].usetype == EU_ITEM || e.type == TRIGGER)
			{
				setspawn(i, 0);
				if(e.type == TRIGGER) // find shared kin
				{
					loopvj(e.links) if(ents.inrange(e.links[j]))
					{
						loopvk(ents) if(ents[k]->type == TRIGGER && ents[k]->links.find(e.links[j]) >= 0)
						{
							if(((gameentity *)ents[i])->kin.find(k) < 0) ((gameentity *)ents[i])->kin.add(k);
							if(((gameentity *)ents[k])->kin.find(i) < 0) ((gameentity *)ents[k])->kin.add(i);
						}
					}
				}
			}
		}
		clearentcache();
	}

	void edittoggled(bool edit) { clearentcache(); }

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
				loopvj(f.links) if(f.links[j] == idx)
				{
					both = true;
					break;
				}
				part_trace(e.o, f.o, 1.f, 1, both ? 0xAA44CC : 0x660088);
			}
		}
	}

	void renderentshow(gameentity &e, int idx, int level)
	{
		if(e.o.squaredist(camera1->o) > maxparticledistance*maxparticledistance) return;
		if(showentradius >= level)
		{
			switch(e.type)
			{
				case PLAYERSTART:
				{
					part_radius(vec(e.o).add(vec(0, 0, game::player1->zradius/2)), vec(game::player1->xradius, game::player1->yradius, game::player1->zradius/2), 1, 1, teamtype[e.attrs[0]].colour);
					break;
				}
				case MAPSOUND:
				{
					part_radius(e.o, vec(e.attrs[1], e.attrs[1], e.attrs[1]), 1, 1, 0x00FFFF);
					part_radius(e.o, vec(e.attrs[2], e.attrs[2], e.attrs[2]), 1, 1, 0x00FFFF);
					break;
				}
				case LIGHT:
				{
					int s = e.attrs[0] ? e.attrs[0] : hdr.worldsize,
						colour = (e.attrs[1]<<16)|(e.attrs[2]<<8)|e.attrs[3];
					part_radius(e.o, vec(s, s, s), 1, 1, colour);
					break;
				}
				case LIGHTFX:
				{
					if(e.attrs[0] == LFX_SPOTLIGHT) loopv(e.links) if(ents.inrange(e.links[i]) && ents[e.links[i]]->type == LIGHT)
					{
						gameentity &f = *(gameentity *)ents[e.links[i]];
						float radius = f.attrs[0];
						if(!radius) radius = 2*e.o.dist(f.o);
						vec dir = vec(e.o).sub(f.o).normalize();
						float angle = max(1, min(90, int(e.attrs[1])));
						int colour = (f.attrs[1]<<16)|(f.attrs[2]<<8)|f.attrs[3];
						part_cone(f.o, dir, radius, angle, 1, colour);
						break;
					}
					break;
				}
				case FLAG:
				{
					float radius = (float)enttype[e.type].radius;
					part_radius(e.o, vec(radius, radius, radius), 1, 1, teamtype[e.attrs[0]].colour);
					radius = radius*2/3; // ctf pickup dist
					part_radius(e.o, vec(radius, radius, radius), 1, 1, teamtype[e.attrs[0]].colour);
					break;
				}
				case WAYPOINT:
				{
					int s = e.attrs[4] ? e.attrs[4] : enttype[e.type].radius;
					part_radius(e.o, vec(s, s, s), 1, 1, 0x008888);
					break;
				}
				default:
				{
					float radius = (float)enttype[e.type].radius;
					if((e.type == TRIGGER || e.type == TELEPORT || e.type == PUSHER) && e.attrs[3])
						radius = (float)e.attrs[3];
					if(radius > 0.f) part_radius(e.o, vec(radius, radius, radius), 1, 1, 0x00FFFF);
					if(e.type == PUSHER && e.attrs[4] && e.attrs[4] < e.attrs[3])
						part_radius(e.o, vec(e.attrs[4], e.attrs[4], e.attrs[4]), 1, 1, 0x00FFFF);
					break;
				}
			}
		}

		switch(e.type)
		{
			case PLAYERSTART:
			{
				if(showentdir >= level) part_dir(e.o, e.attrs[1], e.attrs[2], 4.f, 1, teamtype[e.attrs[0]].colour);
				break;
			}
			case MAPMODEL:
			{
				if(showentdir >= level) part_dir(e.o, e.attrs[1], e.attrs[2], 4.f, 1, 0x00FFFF);
				break;
			}
			case SUNLIGHT:
			{
				if(showentdir >= level)
				{
					int colour = ((e.attrs[2]/2)<<16)|((e.attrs[3]/2)<<8)|(e.attrs[4]/2/3), offset = e.attrs[5] ? e.attrs[5] : 10;
					loopk(9)
					{
						int yaw = e.attrs[0], pitch = e.attrs[1];
						switch(k)
						{
							case 0: default: break;
							case 1: pitch += offset; break;
							case 2: yaw += offset/2; pitch += offset/2; break;
							case 3: yaw += offset; break;
							case 4: yaw += offset/2; pitch -= offset/2; break;
							case 5: pitch -= offset; break;
							case 6: yaw -= offset/2; pitch -= offset/2; break;
							case 7: yaw -= offset; break;
							case 8: yaw -= offset/2; pitch += offset/2; break;
						}
						while(yaw >= 360) yaw -= 360; while(yaw < 0) yaw += 360;
						while(pitch >= 180) pitch -= 360; while(pitch < -180) pitch += 360;
						part_dir(e.o, yaw, pitch, getworldsize()*2, 1, colour);
					}
				}
				break;
			}
			case ACTOR:
			{
				if(showentdir >= level) part_dir(e.o, e.attrs[1], e.attrs[2], 4.f, 1, 0xAAAAAA);
				break;
			}
			case TELEPORT:
			case CAMERA:
			{
				if(showentdir >= level)
				{
					if(e.attrs[0] < 0) part_dir(e.o, (lastmillis/5)%360, e.attrs[1], 4.f, 1, 0x00FFFF);
					else part_dir(e.o, e.attrs[0], e.attrs[1], 8.f, 1, 0x00FFFF);
				}
				break;
			}
			case PUSHER:
			{
				if(showentdir >= level)
				{
					vec dir = vec((int)(char)e.attrs[2], (int)(char)e.attrs[1], (int)(char)e.attrs[0]);
					float mag = dir.magnitude();
					float yaw = 0.f, pitch = 0.f;
					vectoyawpitch(dir.normalize(), yaw, pitch);
					part_dir(e.o, yaw, pitch, 4.f+mag, 1, 0x00FFFF);
				}
				break;
			}
			default: break;
		}

		if(enttype[e.type].links)
			if(showentlinks >= level || (e.type == WAYPOINT && (dropwaypoints || ai::aidebug >= 6)))
				renderlinked(e, idx);
	}

	void renderentlight(gameentity &e)
	{
		adddynlight(vec(e.o), float(e.attrs[0] ? e.attrs[0] : hdr.worldsize)*0.75f, vec(e.attrs[1], e.attrs[2], e.attrs[3]).div(383.f));
	}

	void adddynlights()
	{
		if(game::player1->state == CS_EDITING && showlighting)
		{
			#define islightable(q) ((q)->type == LIGHT && (q)->attrs[0] > 0 && !(q)->links.length())
			loopv(entgroup)
			{
				int n = entgroup[i];
				if(ents.inrange(n) && islightable(ents[n]) && n != enthover)
					renderfocus(n, renderentlight(e));
			}
			if(ents.inrange(enthover) && islightable(ents[enthover]))
				renderfocus(enthover, renderentlight(e));
		}
		loopv(ents) if(ents[i]->type == LIGHTFX && ents[i]->attrs[0] != LFX_SPOTLIGHT)
		{
			if(ents[i]->spawned || ents[i]->lastemit)
			{
				if(!ents[i]->spawned && ents[i]->lastemit > 0 && lastmillis-ents[i]->lastemit > triggertime(*ents[i])/2)
					continue;
			}
			else
			{
				bool lonely = true;
				loopvk(ents[i]->links) if(ents.inrange(ents[i]->links[k]) && ents[ents[i]->links[k]]->type != LIGHT) { lonely = false; break; }
				if(!lonely) continue;
			}
			loopvk(ents[i]->links) if(ents.inrange(ents[i]->links[k]) && ents[ents[i]->links[k]]->type == LIGHT)
				makelightfx(*ents[i], *ents[ents[i]->links[k]]);
		}
	}

	void preload()
	{
		static bool weapf[WEAP_MAX];
		int sweap = m_spawnweapon(game::gamemode, game::mutators);
		loopi(WEAP_MAX) weapf[i] = (i == sweap ? true : false);
		loopv(ents)
		{
			extentity &e = *ents[i];
			if(e.type == MAPMODEL || e.type == FLAG) continue;
			else if(e.type == WEAPON)
			{
				int attr = weapattr(e.attrs[0], sweap);
				if(isweap(attr) && !weapf[attr])
				{
					weapons::preload(attr);
					weapf[attr] = true;
				}
			}
		}
	}

 	void update()
	{
		entitycheck(game::player1);
		loopv(game::players) if(game::players[i]) entitycheck(game::players[i]);
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			if(e.type == MAPSOUND && e.links.empty() && mapsounds.inrange(e.attrs[0]) && !issound(e.schan))
			{
				int flags = SND_MAP|SND_LOOP; // ambient sounds loop
				if(e.attrs[4]&SND_NOATTEN) flags |= SND_NOATTEN;
				if(e.attrs[4]&SND_NODELAY) flags |= SND_NODELAY;
				if(e.attrs[4]&SND_NOCULL) flags |= SND_NOCULL;
				if(e.attrs[4]&SND_NOPAN) flags |= SND_NOPAN;
				playsound(e.attrs[0], e.o, NULL, flags, e.attrs[3], e.attrs[1], e.attrs[2], &e.schan);
			}
		}
		if(docacheclear)
		{
			clearentcache();
			docacheclear = false;
		}
	}

	void render()
	{
		if(rendermainview) loopv(ents) // important, don't render lines and stuff otherwise!
			renderfocus(i, renderentshow(e, i, game::player1->state == CS_EDITING ? ((entgroup.find(i) >= 0 || enthover == i) ? 1 : 2) : 3));
		if(!envmapping)
		{
			loopv(ents)
			{
				gameentity &e = *(gameentity *)ents[i];
				if(e.type <= NOTUSED || e.type >= MAXENTTYPES) continue;
				bool active = enttype[e.type].usetype == EU_ITEM && e.spawned && !m_noitems(game::gamemode, game::mutators);
				if(m_edit(game::gamemode) || active)
				{
					const char *mdlname = entmdlname(e.type, e.attrs);
					vec pos = e.o;
					if(mdlname && *mdlname)
					{
						int flags = MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_DIST|MDL_CULL_OCCLUDED;
						float fade = 1, yaw = 0, pitch = 0;
						if(!active)
						{
							fade = 0.5f;
							if(e.type == FLAG || e.type == PLAYERSTART) { yaw = e.attrs[1]+(e.type == PLAYERSTART ? 90 : 0); pitch = e.attrs[2]; }
							else if(e.type == ACTOR) { yaw = e.attrs[1]+90; pitch = e.attrs[2]; }
						}
						else
						{
							int millis = lastmillis-e.lastspawn;
							if(millis < 1000) fade = float(millis)/1000.f;
						}
						rendermodel(&e.light, mdlname, ANIM_MAPMODEL|ANIM_LOOP, pos, yaw, pitch, 0.f, flags, NULL, NULL, 0, 0, fade);
					}
				}
			}
		}
	}

	void maketeleport(gameentity &e)
	{
		float yaw = e.attrs[0] < 0 ? (lastmillis/5)%360 : e.attrs[0], radius = float(e.attrs[3] ? e.attrs[3] : enttype[e.type].radius);
		int attr = int(e.attrs[4]), colour = (((attr&0xF)<<4)|((attr&0xF0)<<8)|((attr&0xF00)<<12))+0x0F0F0F;
		part_portal(e.o, radius, yaw, e.attrs[1], PART_TELEPORT, 0, colour);
	}

	void drawparticle(gameentity &e, const vec &o, int idx, bool spawned)
	{
		switch(e.type)
		{
			case PARTICLES:
				if(idx < 0 || e.links.empty()) makeparticles(e);
				else if(e.spawned || (e.lastemit > 0 && lastmillis-e.lastemit <= triggertime(e)/2))
					makeparticle(o, e.attrs);
				break;

			case TELEPORT:
				if(e.attrs[4]) maketeleport(e);
				break;
		}

		bool hasent = false, edit = m_edit(game::gamemode) && cansee(e);
		vec off(0, 0, 2.f), pos(o);
		if(enttype[e.type].usetype == EU_ITEM) pos.add(off);
		if(edit)
		{
			hasent = game::player1->state == CS_EDITING && idx >= 0 && (entgroup.find(idx) >= 0 || enthover == idx);
			part_create(hasent ? PART_EDIT_ONTOP : PART_EDIT, 1, o, hasent ? 0xAA22FF : 0x441188, hasent ? 2.f : 1.f);
			if(showentinfo >= 2 || game::player1->state == CS_EDITING)
			{
				defformatstring(s)("@%s%s (%d)", hasent ? "\fp" : "\fv", enttype[e.type].name, idx >= 0 ? idx : 0);
				part_text(pos.add(off), s, hasent ? PART_TEXT_ONTOP : PART_TEXT);
				if(showentinfo >= 3 || hasent) loopk(enttype[e.type].numattrs)
				{
					formatstring(s)("@%s%s:%d", hasent ? "\fw" : "\fd", enttype[e.type].attrs[k], e.attrs[k]);
					part_text(pos.add(off), s, hasent ? PART_TEXT_ONTOP : PART_TEXT);
				}
			}
		}
		bool item = showentdescs && enttype[e.type].usetype == EU_ITEM && spawned, notitem = (edit && (showentinfo >= 4 || hasent));
		if(item || notitem)
		{
			int sweap = m_spawnweapon(game::gamemode, game::mutators), attr = e.type == WEAPON && !edit ? weapattr(e.attrs[0], sweap) : e.attrs[0],
				colour = e.type == WEAPON ? weaptype[attr].colour : 0xFFFFFF;
			const char *itext = !notitem && item && showentdescs >= 3 ? hud::itemtex(e.type, attr) : NULL;
			if(itext && *itext) part_icon(pos.add(off), textureload(hud::itemtex(e.type, attr), 3), 1, 1.5f, 1, colour); // a little smaller than the normal ones
			else
			{
				const char *item = entinfo(e.type, e.attrs, showentinfo >= 5 || hasent);
				if(item && *item)
				{
					defformatstring(ds)("@%s", item);
					part_text(pos.add(off), ds, hasent ? PART_TEXT_ONTOP : PART_TEXT, 1, colour);
				}
			}
		}
	}

	void drawparticles()
	{
		float maxdist = float(maxparticledistance)*float(maxparticledistance);
		int ignoretypes = m_edit(game::gamemode) ? NOTUSED : MAXENTTYPES;
		loopv(ents)
		{
			gameentity &e = *(gameentity *)ents[i];
			switch(e.type)
			{
				case PARTICLES: case TELEPORT:
					break;
				default:
					if(enttype[e.type].usetype != EU_ITEM && e.type <= ignoretypes) continue;
					break;
			}
			if(e.o.squaredist(camera1->o) > maxdist) continue;
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
