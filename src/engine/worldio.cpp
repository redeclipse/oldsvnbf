// worldio.cpp: loading & saving of maps and savegames

#include "pch.h"
#include "engine.h"

sometype mapexts[] = {
	{ ".bgz", MAP_BFGZ },
	{ ".ogz", MAP_OCTA },
};
string bgzname[MAP_MAX], pcfname, mcfname, picname, mapname;

void setnames(const char *fname, const char *cname)
{
	string fn, cn;

	if (fname != NULL) s_sprintf(fn)("%s", fname);
	else s_sprintf(fn)("maps/untitled");

	if (cname != NULL) s_sprintf(cn)("%s", cname);
	else s_sprintf(cn)("%s", fn);

	string name, pakname, cfgname;
	s_strncpy(name, cn, 100);

	char *slash = strpbrk(name, "/\\");
	if(slash)
	{
		s_strncpy(pakname, name, slash-name+1);
		s_strcpy(cfgname, slash+1);
	}
	else
	{
		s_sprintf(pakname)("maps");
		s_strcpy(cfgname, name);
	}
	if(strpbrk(fn, "/\\")) s_strcpy(mapname, fn);
	else s_sprintf(mapname)("maps/%s", fn);

	loopi(MAP_MAX) s_sprintf(bgzname[i])("%s%s", mapname, mapexts[i]);

	s_sprintf(pcfname)("%s/package.cfg", pakname);
	s_sprintf(mcfname)("%s/%s.cfg", pakname, cfgname);
	s_sprintf(picname)("%s.jpg", mapname);
}

ushort readushort(gzFile f)
{
	ushort t;
	gzread(f, &t, sizeof(ushort));
	endianswap(&t, sizeof(ushort), 1);
	return t;
}

void writeushort(gzFile f, ushort u)
{
	endianswap(&u, sizeof(ushort), 1);
	gzwrite(f, &u, sizeof(ushort));
}

enum { OCTSAV_CHILDREN = 0, OCTSAV_EMPTY, OCTSAV_SOLID, OCTSAV_NORMAL, OCTSAV_LODCUBE };

void savec(cube *c, gzFile f, bool nolms)
{
	loopi(8)
	{
		if(c[i].children && (!c[i].ext || !c[i].ext->surfaces))
		{
			gzputc(f, OCTSAV_CHILDREN);
			savec(c[i].children, f, nolms);
		}
		else
		{
			int oflags = 0;
			if(c[i].ext && c[i].ext->merged) oflags |= 0x80;
			if(c[i].children) gzputc(f, oflags | OCTSAV_LODCUBE);
			else if(isempty(c[i])) gzputc(f, oflags | OCTSAV_EMPTY);
			else if(isentirelysolid(c[i])) gzputc(f, oflags | OCTSAV_SOLID);
			else
			{
				gzputc(f, oflags | OCTSAV_NORMAL);
				gzwrite(f, c[i].edges, 12);
			}
			loopj(6) writeushort(f, c[i].texture[j]);
			uchar mask = 0;
			if(c[i].ext)
			{
				if(c[i].ext->material != MAT_AIR) mask |= 0x80;
				if(c[i].ext->normals && !nolms)
				{
					mask |= 0x40;
					loopj(6) if(c[i].ext->normals[j].normals[0] != bvec(128, 128, 128)) mask |= 1 << j;
				}
			}
			// save surface info for lighting
			if(!c[i].ext || !c[i].ext->surfaces || nolms)
			{
				gzputc(f, mask);
				if(c[i].ext)
				{
					if(c[i].ext->material != MAT_AIR) gzputc(f, c[i].ext->material);
					if(c[i].ext->normals && !nolms) loopj(6) if(mask & (1 << j))
					{
						loopk(sizeof(surfaceinfo)) gzputc(f, 0);
						gzwrite(f, &c[i].ext->normals[j], sizeof(surfacenormals));
					}
				}
			}
			else
			{
				loopj(6) if(c[i].ext->surfaces[j].lmid >= LMID_RESERVED) mask |= 1 << j;
				gzputc(f, mask);
				if(c[i].ext->material != MAT_AIR) gzputc(f, c[i].ext->material);
				loopj(6) if(mask & (1 << j))
				{
					surfaceinfo tmp = c[i].ext->surfaces[j];
					endianswap(&tmp.x, sizeof(ushort), 3);
					gzwrite(f, &tmp, sizeof(surfaceinfo));
					if(c[i].ext->normals) gzwrite(f, &c[i].ext->normals[j], sizeof(surfacenormals));
				}
			}
			if(c[i].ext && c[i].ext->merged)
			{
				gzputc(f, c[i].ext->merged | (c[i].ext->mergeorigin ? 0x80 : 0));
				if(c[i].ext->mergeorigin)
				{
					gzputc(f, c[i].ext->mergeorigin);
					int index = 0;
					loopj(6) if(c[i].ext->mergeorigin&(1<<j))
					{
						mergeinfo tmp = c[i].ext->merges[index++];
						endianswap(&tmp, sizeof(ushort), 4);
						gzwrite(f, &tmp, sizeof(mergeinfo));
					}
				}
			}
			if(c[i].children) savec(c[i].children, f, nolms);
		}
	}
}

cube *loadchildren(gzFile f);

void loadc(gzFile f, cube &c)
{
	bool haschildren = false;
	int octsav = gzgetc(f);
	switch(octsav&0x7)
	{
		case OCTSAV_CHILDREN:
			c.children = loadchildren(f);
			return;

		case OCTSAV_LODCUBE: haschildren = true;	break;
		case OCTSAV_EMPTY:  emptyfaces(c);		  break;
		case OCTSAV_SOLID:  solidfaces(c);		  break;
		case OCTSAV_NORMAL: gzread(f, c.edges, 12); break;

		default:
			fatal("garbage in map");
	}
	loopi(6) c.texture[i] = hdr.version<14 ? gzgetc(f) : readushort(f);
	if(hdr.version < 7) loopi(3) gzgetc(f); //gzread(f, c.colour, 3);
	else
	{
		uchar mask = gzgetc(f);
		if(mask & 0x80) ext(c).material = gzgetc(f);
		if(mask & 0x3F)
		{
			uchar lit = 0, bright = 0;
			newsurfaces(c);
			if(mask & 0x40) newnormals(c);
			loopi(6)
			{
				if(mask & (1 << i))
				{
					gzread(f, &c.ext->surfaces[i], sizeof(surfaceinfo));
					endianswap(&c.ext->surfaces[i].x, sizeof(ushort), 3);
					if(hdr.version < 10) ++c.ext->surfaces[i].lmid;
					if(hdr.version < 18)
					{
						if(c.ext->surfaces[i].lmid >= LMID_AMBIENT1) ++c.ext->surfaces[i].lmid;
						if(c.ext->surfaces[i].lmid >= LMID_BRIGHT1) ++c.ext->surfaces[i].lmid;
					}
					if(hdr.version < 19)
					{
						if(c.ext->surfaces[i].lmid >= LMID_DARK) c.ext->surfaces[i].lmid += 2;
					}
					if(mask & 0x40) gzread(f, &c.ext->normals[i], sizeof(surfacenormals));
				}
				else c.ext->surfaces[i].lmid = LMID_AMBIENT;
				if(c.ext->surfaces[i].lmid == LMID_BRIGHT) bright |= 1 << i;
				else if(c.ext->surfaces[i].lmid != LMID_AMBIENT) lit |= 1 << i;
			}
			if(!lit)
			{
				freesurfaces(c);
				if(bright) brightencube(c);
			}
		}
		if(hdr.version >= 20)
		{
			if(octsav&0x80)
			{
				int merged = gzgetc(f);
				ext(c).merged = merged&0x3F;
				if(merged&0x80)
				{
					c.ext->mergeorigin = gzgetc(f);
					int nummerges = 0;
					loopi(6) if(c.ext->mergeorigin&(1<<i)) nummerges++;
					if(nummerges)
					{
						c.ext->merges = new mergeinfo[nummerges];
						loopi(nummerges)
						{
							gzread(f, &c.ext->merges[i], sizeof(mergeinfo));
							endianswap(&c.ext->merges[i], sizeof(ushort), 4);
						}
					}
				}
			}
		}
	}
	c.children = (haschildren ? loadchildren(f) : NULL);
}

cube *loadchildren(gzFile f)
{
	cube *c = newcubes();
	loopi(8) loadc(f, c[i]);
	// TODO: remip c from children here
	return c;
}

void save_config()
{
	FILE *h = openfile(mcfname, "w");
	if (!h) { conoutf("could not write config to %s", mcfname); return; }

	// config
	fprintf(h, "// %s (%s)\n// config generated by Blood Frontier\n\n", mapname, hdr.maptitle);

	// skybox
	fprintf(h, "loadsky \"%s\" %.f\n\n", lastsky, spinsky);

	// texture slots
	#define saveslot(s,b) \
		if (b) \
		{ \
			if (s.shader) \
			{ \
				fprintf(h, "setshader %s\n", s.shader->name); \
				loopvj(s.params) \
				{ \
					fprintf(h, "set%sparam", s.params[j].type == SHPARAM_UNIFORM ? "uniform" : (s.params[j].type == SHPARAM_PIXEL ? "pixel" : "vertex")); \
					if (s.params[j].type == SHPARAM_UNIFORM) fprintf(h, " \"%s\"", s.params[j].name); \
					else fprintf(h, " %d", s.params[j].index);\
					loopk(4) fprintf(h, " %.f", s.params[j].val[k]); \
					fprintf(h, "\n"); \
				} \
			} \
		} \
		loopvj(s.sts) \
		{ \
			fprintf(h, "texture"); \
			if (b) fprintf(h, " %s", findtexturename(s.sts[j].type)); \
			else if (!j) fprintf(h, " %s", findmaterialname(i)); \
			else fprintf(h, " 1"); \
			fprintf(h, " \"%s\"", s.sts[j].lname); \
			fprintf(h, " %d %d %d %.f\n", \
				s.sts[j].rotation, s.sts[j].xoffset, s.sts[j].yoffset, s.sts[j].scale); \
		} \
		if (b) \
		{ \
			if (s.autograss) \
			{ \
				fprintf(h, "autograss \"%s\"\n", s.autograss); \
			} \
		} \
		fprintf(h, "\n");

	loopi(MAT_EDIT)
	{
		if (verbose >= 2) show_out_of_renderloop_progress(float(i)/float(MAT_EDIT), "saving material slots...");

		if (i == MAT_WATER || i == MAT_LAVA)
		{
			saveslot(materialslots[i], false);
		}
	}
	if (verbose >= 2) conoutf("saved %d material slots", MAT_EDIT);

	loopv(slots)
	{
		if (verbose >= 2) show_out_of_renderloop_progress(float(i)/float(slots.length()), "saving texture slots...");
		saveslot(slots[i], true);
	}
	if (verbose >= 2) conoutf("saved %d texture slots", slots.length());

	loopv(mapmodels)
	{
		if (verbose >= 2) show_out_of_renderloop_progress(float(i)/float(mapmodels.length()), "saving mapmodel slots...");
		fprintf(h, "mmodel \"%s\" %d\n", mapmodels[i].name, mapmodels[i].tex);
	}
	if (mapmodels.length()) fprintf(h, "\n");
	if (verbose >= 2) conoutf("saved %d mapmodel slots", mapmodels.length());

	loopv(mapsounds)
	{
		if (verbose >= 2) show_out_of_renderloop_progress(float(i)/float(mapsounds.length()), "saving mapsound slots...");
		fprintf(h, "mapsound \"%s\" %d\n", mapsounds[i].sample->name, mapsounds[i].vol);
	}
	if (mapsounds.length()) fprintf(h, "\n");
	if (verbose >= 2) conoutf("saved %d mapsound slots", mapsounds.length());

	fclose(h);
	if (verbose) conoutf("saved config '%s'", mcfname);
}

void save_world(const char *mname, bool nolms)
{
	int savingstart = SDL_GetTicks();

	string fname;
	if(strpbrk(mname, "/\\")) s_strcpy(fname, mname);
	else s_sprintf(fname)("maps/%s", mname);

	setnames(fname);

	gzFile f = opengzfile(bgzname[MAP_BFGZ], "wb9");
	if (!f) { conoutf("error saving '%s' to '%s': file error", mapname, bgzname[MAP_BFGZ]); return; }

	strncpy(hdr.head, "BFGZ", 4);
	hdr.version = MAPVERSION;
	hdr.headersize = sizeof(bfgz);
	hdr.gamever = BFRONTIER;
	hdr.numents = 0;
	hdr.revision++;
	strncpy(hdr.gameid, sv->gameid(), 4);

	const vector<extentity *> &ents = et->getents();
	loopv(ents)
	{
		if(ents[i]->type!=ET_EMPTY)
		{
			hdr.numents++;
		}
	}

	hdr.lightmaps = nolms ? 0 : lightmaps.length();

	bfgz tmp = hdr;
	endianswap(&tmp.version, sizeof(int), 7);
	gzwrite(f, &tmp, sizeof(bfgz));

	// world variables
	int numvars = 0, vars = 0;
	enumerate(*idents, ident, id, {
		if (id.type == ID_VAR && id.world) numvars++;
	});
	gzputint(f, numvars);
	enumerate(*idents, ident, id, {
		if (id.type == ID_VAR && id.world)
		{
			vars++;
			if (verbose >= 2) show_out_of_renderloop_progress(float(vars)/float(numvars), "saving world variables...");
			gzputint(f, (int)strlen(id.name));
			gzwrite(f, id.name, (int)strlen(id.name)+1);
			gzputint(f, *id.storage.i);
		}
	});

	if (verbose >= 2) conoutf("saved %d variables", vars);

	// texture slots
	writeushort(f, texmru.length());
	loopv(texmru) writeushort(f, texmru[i]);

	// entities
	int count = 0;
	loopv(ents) // extended
	{
		if (verbose >= 2)
			show_out_of_renderloop_progress(float(i)/float(ents.length()), "saving entities...");
		if(ents[i]->type!=ET_EMPTY)
		{
			entity tmp = *ents[i];
			endianswap(&tmp.o, sizeof(int), 3);
			endianswap(&tmp.attr1, sizeof(short), 5);
			gzwrite(f, &tmp, sizeof(entity));
			et->writeent(f, i, *ents[i]);
			count++;
		}
	}
	if (verbose >= 2) conoutf("saved %d entities", hdr.numents);

	savec(worldroot, f, nolms);
	if(!nolms) loopv(lightmaps)
	{
		LightMap &lm = lightmaps[i];
		if (verbose >= 2) show_out_of_renderloop_progress(float(i)/float(lightmaps.length()), "saving lightmaps...");
		gzputc(f, lm.type | (lm.unlitx>=0 ? 0x80 : 0));
		if(lm.unlitx>=0)
		{
			writeushort(f, ushort(lm.unlitx));
			writeushort(f, ushort(lm.unlity));
		}
		gzwrite(f, lm.data, sizeof(lm.data));
	}
	if (verbose >= 2) conoutf("saved %d lightmaps", lightmaps.length());

	show_out_of_renderloop_progress(0, "saving world...");
	cl->saveworld(f);

	gzclose(f);
	save_config();
	conoutf("saved map '%s' v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-savingstart)/1000.0f);
}

void swapXZ(cube *c)
{
	loopi(8)
	{
		swap(c[i].faces[0],   c[i].faces[2]);
		swap(c[i].texture[0], c[i].texture[4]);
		swap(c[i].texture[1], c[i].texture[5]);
		if(c[i].ext && c[i].ext->surfaces)
		{
			swap(c[i].ext->surfaces[0], c[i].ext->surfaces[4]);
			swap(c[i].ext->surfaces[1], c[i].ext->surfaces[5]);
		}
		if(c[i].children) swapXZ(c[i].children);
	}
}

void load_world(const char *mname, const char *cname)		// still supports all map formats that have existed since the earliest cube betas!
{
	int loadingstart = SDL_GetTicks();
	setnames(mname, cname);
	Texture *mapshot = textureload(picname, 0, true, false);
	bool samegame = true;
	int maptype = -1, eif = 0;
    computescreen(mname, mapshot!=notexture ? mapshot : NULL);

	gzFile f;
	loopi(MAP_MAX) if ((f = opengzfile(bgzname[i], "rb9"))) break;
	if (!f)
	{
		conoutf("error loading '%s': file unavailable", mapname);
		s_sprintfd(m)("%s", mname);
		emptymap(12, true, m);
		return;
	}

	resetmap();

	binary newhdr;
	gzread(f, &newhdr, sizeof(binary));
	endianswap(&newhdr.version, sizeof(int), 2);
	memcpy(&hdr, &newhdr, sizeof(binary));

	if(strncmp(newhdr.head, "BFGZ", 4) == 0)
	{
		gzread(f, &hdr.worldsize, hdr.headersize-sizeof(binary));
		endianswap(&hdr.worldsize, sizeof(int), 5);

		if(hdr.version > MAPVERSION)
		{
			conoutf("error loading '%s': requires a newer version of Blood Frontier", mapname);
			gzclose(f);
			s_sprintfd(m)("%s", mname);
			emptymap(12, true, m);
			return;
		}

		if (hdr.version >= 25 || (hdr.version == 24 && hdr.gamever >= 44))
		{
			int numvars = hdr.version >= 24 && hdr.gamever >= 50 ? gzgetint(f) : gzgetc(f), vars = 0;
			show_out_of_renderloop_progress(0, "loading variables...");
			loopi (numvars)
			{
				vars++;
				if (verbose >= 2) show_out_of_renderloop_progress(float(i)/float(vars), "loading variables...");
				int len = hdr.version >= 24 && hdr.gamever >= 50 ? gzgetint(f) : gzgetc(f);
				string vname;
				gzread(f, vname, len+1);
				int val = gzgetint(f);
				ident *id = idents->access(vname);
				if (id != NULL && id->type == ID_VAR && id->world && id->maxval >= id->minval)
				{
					if (val > id->maxval) val = id->maxval;
					else if (val < id->minval) val = id->minval;
					setvar(vname, val, true);
					if (verbose >= 3) conoutf("%s set to %d", id->name, *id->storage.i);
				}
				else conoutf("ignoring definition for %s = %d", vname, val);
			}
			if (verbose >= 2) conoutf("loaded %d variables", vars);
		}

		if (hdr.version <= 24)
			s_strncpy(hdr.gameid, "bfa", 4); // all previous maps were bfa-fps

		if (!sv->canload(hdr.gameid))
		{
			conoutf("WARNING: loading map from %s game type in %s, ignoring game specific data", hdr.gameid, sv->gameid());
			samegame = false;
		}

		maptype = MAP_BFGZ;
	}
	else if(strncmp(newhdr.head, "OCTA", 4) == 0)
	{
		octa ohdr;
		memcpy(&ohdr, &newhdr, sizeof(binary));
		gzread(f, &ohdr.worldsize, hdr.headersize-sizeof(binary));
		endianswap(&ohdr.worldsize, sizeof(int), 7);

		if(ohdr.version > OCTAVERSION)
		{
			conoutf("error loading '%s': requires a newer version of Cube 2 support", mapname);
			gzclose(f);
			s_sprintfd(m)("%s", mname);
			emptymap(12, true, m);
			return;
		}
		maptype = MAP_OCTA;

		strncpy(hdr.head, "BFGZ", 4);
		hdr.gamever = BFRONTIER;
		hdr.worldsize = ohdr.worldsize;
		hdr.numents = ohdr.numents;
		hdr.lightmaps = ohdr.lightmaps;
		hdr.revision = 1;
		strncpy(hdr.maptitle, ohdr.maptitle, 128);

		if(ohdr.version<=20) conoutf("loading older / less efficient map format, may benefit from \"calclight 2\", then \"savecurrentmap\"");
		if(!ohdr.ambient) ohdr.ambient = 25;
		if(!ohdr.lerpsubdivsize)
		{
			if(!ohdr.lerpangle) ohdr.lerpangle = 44;
			ohdr.lerpsubdiv = 2;
			ohdr.lerpsubdivsize = 4;
		}
		setvar("lightprecision", ohdr.mapprec ? ohdr.mapprec : 32);
		setvar("lighterror", ohdr.maple ? ohdr.maple : 8);
		setvar("bumperror", ohdr.mapbe ? ohdr.mapbe : 3);
		setvar("lightlod", ohdr.mapllod);
		setvar("ambient", ohdr.ambient);
		setvar("fullbright", 0, false);
		setvar("fullbrightlevel", 128, false);
		setvar("lerpangle", ohdr.lerpangle);
		setvar("lerpsubdiv", ohdr.lerpsubdiv);
		setvar("lerpsubdivsize", ohdr.lerpsubdivsize);

		string gameid;
		s_strcpy(gameid, "fps");

		if (hdr.version >= 16)
		{
			int len = gzgetc(f);
			gzread(f, gameid, len+1);
		}

		if (!sv->canload(gameid))
		{
			conoutf("WARNING: loading map from %s sauerbraten game type in %s, ignoring game specific data", gameid, sv->gameid());
			samegame = false;
		}

		if(hdr.version>=16)
		{
			eif = readushort(f);
			int extrasize = readushort(f);
			loopj(extrasize) gzgetc(f);
		}
	}
	else
	{
		conoutf("error loading '%s': malformatted header", mapname);
		gzclose(f);
		s_sprintfd(m)("%s", mname);
		emptymap(12, true, m);
		return;
	}
	show_out_of_renderloop_progress(0, "clearing world...");

	texmru.setsize(0);
	if(hdr.version<14)
	{
		uchar oldtl[256];
		gzread(f, oldtl, sizeof(oldtl));
		loopi(256) texmru.add(oldtl[i]);
	}
	else
	{
		ushort nummru = readushort(f);
		loopi(nummru) texmru.add(readushort(f));
	}

	freeocta(worldroot);
	worldroot = NULL;

	show_out_of_renderloop_progress(0, "loading entities...");

	vector<extentity *> &ents = et->getents();
	loopi(hdr.numents)
	{
		if (verbose >= 2) show_out_of_renderloop_progress(float(i)/float(hdr.numents), "loading entities...");
		extentity &e = *et->newentity();
		ents.add(&e);
		gzread(f, &e, sizeof(entity));
		endianswap(&e.o, sizeof(int), 3);
		endianswap(&e.attr1, sizeof(short), 5);
		e.spawned = false;
		e.inoctanode = false;
		if(samegame)
		{
			if (maptype == MAP_OCTA) { loopj(eif) gzgetc(f); }
			et->readent(f, maptype, i, hdr.version, e);
		}
		else
		{
			loopj(eif) gzgetc(f);
		}
		if(hdr.version <= 14 && e.type >= ET_MAPMODEL && e.type <= 16)
		{
			if(e.type == 16) e.type = ET_MAPMODEL;
			else e.type++;
		}
		if(hdr.version <= 20 && e.type >= ET_ENVMAP) e.type++;
		if(hdr.version <= 21 && e.type >= ET_PARTICLES) e.type++;
		if(hdr.version <= 22 && e.type >= ET_SOUND) e.type++;
		if(hdr.version <= 23 && e.type >= ET_SPOTLIGHT) e.type++;
		if(!samegame)
		{
			if(e.type>=ET_GAMESPECIFIC || hdr.version<=14)
			{
				ents.pop();
				continue;
			}
		}
		if(!insideworld(e.o))
		{
			if(e.type != ET_LIGHT && e.type != ET_SPOTLIGHT)
			{
				conoutf("warning: ent outside of world: enttype[%s] index %d (%f, %f, %f)", et->entname(e.type), i, e.o.x, e.o.y, e.o.z);
			}
		}
		if(hdr.version <= 14 && e.type == ET_MAPMODEL)
		{
			e.o.z += e.attr3;
			if(e.attr4) conoutf("warning: mapmodel ent (index %d) uses texture slot %d", i, e.attr4);
			e.attr3 = e.attr4 = 0;
		}
	}
	if (verbose >= 2) conoutf("loaded %d entities", hdr.numents);

	show_out_of_renderloop_progress(0, "loading octree...");
	worldroot = loadchildren(f);

	if(hdr.version <= 11)
		swapXZ(worldroot);

	if(hdr.version <= 8)
		converttovectorworld();

	show_out_of_renderloop_progress(0, "validating...");
	validatec(worldroot, hdr.worldsize>>1);

	if(hdr.version >= 7) loopi(hdr.lightmaps)
	{
		if (verbose >= 2) show_out_of_renderloop_progress(i/(float)hdr.lightmaps, "loading lightmaps...");
		LightMap &lm = lightmaps.add();
		if(hdr.version >= 17)
		{
			int type = gzgetc(f);
			lm.type = type&0xF;
			if(hdr.version >= 20 && type&0x80)
			{
				lm.unlitx = readushort(f);
				lm.unlity = readushort(f);
			}
		}
		gzread(f, lm.data, 3 * LM_PACKW * LM_PACKH);
		lm.finalize();
	}

	if (verbose >= 2) conoutf("loaded %d lightmaps", hdr.lightmaps);

	show_out_of_renderloop_progress(0, "loading world...");
	cl->loadworld(f, maptype);

	gzclose(f);
	conoutf("loaded map '%s' v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-loadingstart)/1000.0f);
	console("%s", CON_CENTER|CON_NORMAL, hdr.maptitle);

	overrideidents = worldidents = true;
	persistidents = false;
	if (!execfile(pcfname)) exec("package.cfg");
	if (!execfile(mcfname)) exec("map.cfg");
	persistidents = true;
	overrideidents = worldidents = false;

	loopv(ents)
	{
		extentity &e = *ents[i];
		if(e.type==ET_MAPMODEL && e.attr2 >= 0)
		{
			mapmodelinfo &mmi = getmminfo(e.attr2);
			if(!&mmi) conoutf("could not find map model: %d", e.attr2);
			else if(!loadmodel(NULL, e.attr2, true)) conoutf("could not load model: %s", mmi.name);
		}
	}

    cl->preload();

	initlights();
	if (maptype == MAP_OCTA)
	{
		mpremip(true);
		conoutf("WARNING: map imported from cube 2 format, some artifacts may be present, please check your map before saving");
	}
	allchanged(true);

    computescreen(mname, mapshot!=notexture ? mapshot : NULL);
	attachentities();

	show_out_of_renderloop_progress(0, "starting world...");
	startmap(cname ? cname : mname);
}

ICOMMAND(savemap, "s", (char *mname), save_world(*mname ? mname : mapname, false));
ICOMMAND(savecurrentmap, "", (), save_world(mapname, false));

void writeobj(char *name)
{
	bool oldVBO = hasVBO;
	hasVBO = false;
	allchanged();
	s_sprintfd(fname)("%s.obj", name);
    FILE *f = openfile(fname, "w");
	if(!f) return;
	fprintf(f, "# obj file octa world\n");
	extern vector<vtxarray *> valist;
	loopv(valist)
	{
		vtxarray &va = *valist[i];
		uchar *verts = (uchar *)va.vbuf;
		if(!verts) continue;
		int vtxsize = VTXSIZE;
		loopj(va.verts)
		{
			vvec vv;
			if(floatvtx) { vec &f = *(vec *)verts; loopk(3) vv[k] = short(f[k]); }
			else vv = *(vvec *)verts;
			vec v = vv.tovec(va.x, va.y, va.z);
			if(vv.x&((1<<VVEC_FRAC)-1)) fprintf(f, "v %.3f ", v.x); else fprintf(f, "v %d ", int(v.x));
			if(vv.y&((1<<VVEC_FRAC)-1)) fprintf(f, "%.3f ", v.y); else fprintf(f, "%d ", int(v.y));
			if(vv.z&((1<<VVEC_FRAC)-1)) fprintf(f, "%.3f\n", v.z); else fprintf(f, "%d\n", int(v.z));
			verts += vtxsize;
		}
		ushort *ebuf = va.ebuf;
        loopi(va.tris)
		{
			fprintf(f, "f");
			for(int k = 0; k<3; k++) fprintf(f, " %d", ebuf[k]-va.verts);
			ebuf += 3;
			fprintf(f, "\n");
		}
	}
	fclose(f);
	hasVBO = oldVBO;
	allchanged();
}

COMMAND(writeobj, "s");

char *getmaptitle() { return hdr.maptitle; }
ICOMMAND(getmaptitle, "", (void), result(getmaptitle()));
