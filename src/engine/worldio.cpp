// worldio.cpp: loading & saving of maps and savegames

#include "pch.h"
#include "engine.h"

sometype mapexts[] = {
	{ ".bgz", MAP_BFGZ },
	{ ".ogz", MAP_OCTA },
};

sometype mapdirs[] = {
	{ "maps", MAP_BFGZ },
	{ "base", MAP_OCTA },
};

string mapfile, mapname;
VAR(maptype, 1, -1, -1);

void setnames(char *fname, int type)
{
	maptype = type >= 0 || type <= MAP_MAX-1 ? type : MAP_BFGZ;

	string fn;
	if(fname != NULL) s_sprintf(fn)("%s", fname);
	else s_sprintf(fn)("%s/untitled", mapdirs[maptype].name);

	if(strpbrk(fn, "/\\")) s_strcpy(mapname, fn);
	else s_sprintf(mapname)("%s/%s", mapdirs[maptype].name, fn);

	s_sprintf(mapfile)("%s%s", mapname, mapexts[maptype].name);
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
        if(mask & 0x80)
        {
            int mat = gzgetc(f);
            if((maptype == MAP_OCTA && hdr.version <= 26) ||
            	(maptype == MAP_BFGZ && hdr.version <= 30))
            {
                static uchar matconv[] = { MAT_AIR, MAT_WATER, MAT_CLIP, MAT_GLASS|MAT_CLIP, MAT_NOCLIP, MAT_LAVA|MAT_DEATH, MAT_AICLIP, MAT_DEATH };
                mat = size_t(mat) < sizeof(matconv)/sizeof(matconv[0]) ? matconv[mat] : MAT_AIR;
            }
            ext(c).material = mat;
        }
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
                            mergeinfo *m = &c.ext->merges[i];
                            gzread(f, m, sizeof(mergeinfo));
                            endianswap(m, sizeof(ushort), 4);
                            if(hdr.version <= 25)
                            {
                                int uorigin = m->u1 & 0xE000, vorigin = m->v1 & 0xE000;
                                m->u1 = (m->u1 - uorigin) << 2;
                                m->u2 = (m->u2 - uorigin) << 2;
                                m->v1 = (m->v1 - vorigin) << 2;
                                m->v2 = (m->v2 - vorigin) << 2;
                            }
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

void save_config(char *mname)
{
	backup(mname, ".cfg", hdr.revision);
	s_sprintfd(fname)("%s.cfg", mname);
	FILE *h = openfile(fname, "w");
	if(!h) { conoutf("could not write config to %s", fname); return; }

	// config
	fprintf(h, "// %s (%s)\n// config generated by Blood Frontier\n\n", mapname, hdr.maptitle);

	int aliases = 0;
	enumerate(*idents, ident, id, {
		if(id.type == ID_ALIAS && id.flags&IDF_WORLD && strlen(id.name))
		{
			aliases++;
            fprintf(h, "\"%s\" = [%s]\n", id.name, id.action);
		}
	});
	if(aliases) fprintf(h, "\n");

	if(verbose >= 2) conoutf("saved %d aliases", aliases);

	// texture slots
	#define saveslot(s,b) \
		if(b) \
		{ \
			if(s.shader) \
			{ \
				fprintf(h, "setshader %s\n", s.shader->name); \
			} \
			loopvj(s.params) \
			{ \
				fprintf(h, "set%sparam", s.params[j].type == SHPARAM_UNIFORM ? "uniform" : (s.params[j].type == SHPARAM_PIXEL ? "pixel" : "vertex")); \
				if(s.params[j].type == SHPARAM_UNIFORM) fprintf(h, " \"%s\"", s.params[j].name); \
				else fprintf(h, " %d", s.params[j].index);\
				loopk(4) fprintf(h, " %f", s.params[j].val[k]); \
				fprintf(h, "\n"); \
			} \
		} \
		loopvj(s.sts) \
		{ \
			fprintf(h, "texture"); \
			if(b) fprintf(h, " %s", findtexturename(s.sts[j].type)); \
			else if(!j) fprintf(h, " %s", findmaterialname(i)); \
			else fprintf(h, " 1"); \
			fprintf(h, " \"%s\"", s.sts[j].lname); \
			if(!j) fprintf(h, " %d %d %d %f\n", \
				s.rotation, s.xoffset, s.yoffset, s.scale); \
            else fprintf(h, "\n"); \
		} \
		if(b) \
		{ \
			if(s.scrollS != 0.f || s.scrollT != 0.f) \
				fprintf(h, "texscroll %f %f", s.scrollS, s.scrollT); \
			if(s.autograss) fprintf(h, "autograss \"%s\"\n", s.autograss); \
		} \
		fprintf(h, "\n");

	loopi(MAT_EDIT)
	{
		if(verbose >= 2) renderprogress(float(i)/float(MAT_EDIT), "saving material slots...");

		if(i == MAT_WATER || i == MAT_LAVA)
		{
			saveslot(materialslots[i], false);
		}
	}
	if(verbose >= 2) conoutf("saved %d material slots", MAT_EDIT);

	loopv(slots)
	{
		if(verbose >= 2) renderprogress(float(i)/float(slots.length()), "saving texture slots...");
		saveslot(slots[i], true);
	}
	if(verbose >= 2) conoutf("saved %d texture slots", slots.length());

	loopv(mapmodels)
	{
		if(verbose >= 2) renderprogress(float(i)/float(mapmodels.length()), "saving mapmodel slots...");
		fprintf(h, "mmodel \"%s\"\n", mapmodels[i].name);
	}
	if(mapmodels.length()) fprintf(h, "\n");
	if(verbose >= 2) conoutf("saved %d mapmodel slots", mapmodels.length());

	loopv(mapsounds)
	{
		if(verbose >= 2) renderprogress(float(i)/float(mapsounds.length()), "saving mapsound slots...");
		fprintf(h, "mapsound \"%s\" %d \"%s\"\n", mapsounds[i].sample->name, mapsounds[i].vol, findmaterialname(mapsounds[i].material));
	}
	if(mapsounds.length()) fprintf(h, "\n");
	if(verbose >= 2) conoutf("saved %d mapsound slots", mapsounds.length());

	fclose(h);
	if(verbose) conoutf("saved config %s", fname);
}
ICOMMAND(savemapconfig, "s", (char *mname), save_config(*mname ? mname : mapname));

VARFP(mapshotsize, 0, 256, 1024, mapshotsize -= mapshotsize%2);

void save_mapshot(char *mname)
{
	backup(mname, ifmtexts[imageformat], hdr.revision);

    GLuint tex;
	glGenTextures(1, &tex);
	glViewport(0, 0, mapshotsize, mapshotsize);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
    uchar *pixels = new uchar[3*mapshotsize*mapshotsize];
	memset(pixels, 0, 3*mapshotsize*mapshotsize);
	glFrontFace(GL_CCW);
	drawcubemap(mapshotsize, camera1->o, camera1->yaw, camera1->pitch, true, false, false, false);
	glReadPixels(0, 0, mapshotsize, mapshotsize, GL_RGB, GL_UNSIGNED_BYTE, pixels);

	SDL_Surface *image = SDL_CreateRGBSurface(SDL_SWSURFACE, mapshotsize, mapshotsize, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0);
	if(image)
	{
		uchar *dst = (uchar *)image->pixels;
		loopi(mapshotsize)
		{
			memcpy(dst, &pixels[3*mapshotsize*(mapshotsize-i-1)], 3*mapshotsize);
			endianswap(dst, 3, mapshotsize);
			dst += image->pitch;
		}

		savesurface(image, mname, imageformat, compresslevel);
		SDL_FreeSurface(image);
	}

	glDeleteTextures(1, &tex);
    glFrontFace(GL_CCW);
    delete[] pixels;
	glViewport(0, 0, screen->w, screen->h);
}
ICOMMAND(savemapshot, "s", (char *mname), save_mapshot(*mname ? mname : mapname));

VARP(autosaveconfig, 0, 1, 1);
VARP(autosavemapshot, 0, 1, 1);

void save_world(char *mname, bool nolms)
{
	int savingstart = SDL_GetTicks();

	setnames(mname, MAP_BFGZ);

	if(autosavemapshot) save_mapshot(mapname);
	if(autosaveconfig) save_config(mapname);

	backup(mapname, mapexts[MAP_BFGZ].name, hdr.revision); // x10 so we can do this more
	gzFile f = opengzfile(mapfile, "wb9");
	if(!f) { conoutf("error saving %s to %s: file error", mapname, mapfile); return; }

	strncpy(hdr.head, "BFGZ", 4);
	hdr.version = MAPVERSION;
	hdr.headersize = sizeof(bfgz);
	hdr.gamever = sv->gamever();
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

    hdr.numpvs = nolms ? 0 : getnumviewcells();
	hdr.lightmaps = nolms ? 0 : lightmaps.length();

	bfgz tmp = hdr;
	endianswap(&tmp.version, sizeof(int), 7);
	gzwrite(f, &tmp, sizeof(bfgz));

	// world variables
	int numvars = 0, vars = 0;
	enumerate(*idents, ident, id, {
		if((id.type == ID_VAR || id.type == ID_FVAR || id.type == ID_SVAR) && id.flags&IDF_WORLD && strlen(id.name)) numvars++;
	});
	gzputint(f, numvars);
	enumerate(*idents, ident, id, {
		if((id.type == ID_VAR || id.type == ID_FVAR || id.type == ID_SVAR) && id.flags&IDF_WORLD && strlen(id.name))
		{
			vars++;
			if(verbose >= 2) renderprogress(float(vars)/float(numvars), "saving world variables...");
			gzputint(f, (int)strlen(id.name));
			gzwrite(f, id.name, (int)strlen(id.name)+1);
			gzputint(f, id.type);
			switch(id.type)
			{
				case ID_VAR:
					gzputint(f, *id.storage.i);
					break;
				case ID_FVAR:
					gzputfloat(f, *id.storage.f);
					break;
				case ID_SVAR:
					gzputint(f, (int)strlen(*id.storage.s));
					gzwrite(f, *id.storage.s, (int)strlen(*id.storage.s)+1);
					break;
				default: break;
			}
		}
	});

	if(verbose >= 2) conoutf("saved %d variables", vars);

	// texture slots
	writeushort(f, texmru.length());
	loopv(texmru) writeushort(f, texmru[i]);

	// entities
	int count = 0;
	loopv(ents) // extended
	{
		if(verbose >= 2)
			renderprogress(float(i)/float(ents.length()), "saving entities...");
		if(ents[i]->type!=ET_EMPTY)
		{
			entity tmp = *ents[i];
			endianswap(&tmp.o, sizeof(int), 3);
			endianswap(&tmp.attr1, sizeof(short), 5);
			gzwrite(f, &tmp, sizeof(entity));
			et->writeent(f, i, *ents[i]);
			extentity &e = (extentity &)*ents[i];
			if(et->maylink(e.type))
			{
				vector<int> links;
				int n = 0;

				loopvk(ents)
				{
					extentity &f = (extentity &)*ents[k];

					if(f.type != ET_EMPTY)
					{
						if(et->maylink(f.type) && e.links.find(k) >= 0)
							links.add(n); // align to indices

						n++;
					}
				}

				gzputint(f, links.length());
				loopv(links) gzputint(f, links[i]); // aligned index
				if(verbose >= 2) conoutf("entity %s (%d) saved %d links", et->findname(e.type), i, links.length());
			}
			count++;
		}
	}
	if(verbose >= 2) conoutf("saved %d entities", count);

	savec(worldroot, f, nolms);
    if(!nolms)
    {
        loopv(lightmaps)
        {
            if(verbose >= 2) renderprogress(float(i)/float(lightmaps.length()), "saving lightmaps...");
            LightMap &lm = lightmaps[i];
            gzputc(f, lm.type | (lm.unlitx>=0 ? 0x80 : 0));
            if(lm.unlitx>=0)
            {
                writeushort(f, ushort(lm.unlitx));
                writeushort(f, ushort(lm.unlity));
            }
            gzwrite(f, lm.data, sizeof(lm.data));
        }
        if(verbose >= 2) conoutf("saved %d lightmaps", lightmaps.length());
        if(getnumviewcells()>0)
        {
            if(verbose >= 2) renderprogress(0, "saving PVS...");
            savepvs(f);
            if(verbose >= 2) conoutf("saved %d PVS view cells", getnumviewcells());
        }
    }

	renderprogress(0, "saving world...");
	cl->saveworld(f);
	gzclose(f);

	conoutf("saved map %s v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-savingstart)/1000.0f);
}

ICOMMAND(savemap, "s", (char *mname), save_world(*mname ? mname : mapname, false));
ICOMMAND(savecurrentmap, "", (), save_world(mapname, false));

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

static void fixoversizedcubes(cube *c, int size)
{
    if(size <= VVEC_INT_MASK+1) return;
    loopi(8)
    {
        if(!c[i].children) subdividecube(c[i], true, false);
        fixoversizedcubes(c[i].children, size>>1);
    }
}

void load_world(char *mname)		// still supports all map formats that have existed since the earliest cube betas!
{
	int loadingstart = SDL_GetTicks();

	loop(format, MAP_MAX)
	{
		setnames(mname, format);

		gzFile f = opengzfile(mapfile, "rb9");
		if(!f) continue;

		Texture *mapshot = textureload(mapname, 0, true, false);
		bool samegame = true;
		int eif = 0;
		computescreen("loading...", mapshot!=notexture ? mapshot : NULL, mapname);
		resetmap();

		binary newhdr;
		gzread(f, &newhdr, sizeof(binary));
		endianswap(&newhdr.version, sizeof(int), 2);
		memcpy(&hdr, &newhdr, sizeof(binary));

		if(strncmp(newhdr.head, "BFGZ", 4) == 0)
		{
			if(hdr.version <= 25)
			{
				bfgzcompat25 chdr;
				memcpy(&chdr, &hdr, sizeof(binary));
				gzread(f, &chdr.worldsize, hdr.headersize-sizeof(binary));
				endianswap(&chdr.worldsize, sizeof(int), 5);
				memcpy(&hdr.worldsize, &chdr.worldsize, sizeof(int)*2);
				hdr.numpvs = 0;
				memcpy(&hdr.lightmaps, &chdr.lightmaps, hdr.headersize-sizeof(binary)-sizeof(int)*2);
			}
			else
			{
				gzread(f, &hdr.worldsize, hdr.headersize-sizeof(binary));
				endianswap(&hdr.worldsize, sizeof(int), 6);
			}

			if(hdr.version > MAPVERSION)
			{
				conoutf("error loading %s: requires a newer version of Blood Frontier", mapname);
				gzclose(f);
				s_sprintfd(m)("%s", mapname);
				emptymap(12, true, m);
				return;
			}

			maptype = MAP_BFGZ;

			if(hdr.version <= 24) s_strncpy(hdr.gameid, "bfa", 4); // all previous maps were bfa-fps

			if(verbose >= 2) conoutf("loading v%d map from %s game v%d", hdr.version, hdr.gameid, hdr.gamever);

			if(hdr.version >= 25 || (hdr.version == 24 && hdr.gamever >= 44))
			{
				int numvars = hdr.version >= 25 ? gzgetint(f) : gzgetc(f), vars = 0;
				renderprogress(0, "loading variables...");
				loopi (numvars)
				{
					vars++;
					if(verbose >= 2) renderprogress(float(i)/float(vars), "loading variables...");
					int len = hdr.version >= 25 ? gzgetint(f) : gzgetc(f);
					if(len)
					{
						string vname;
						gzread(f, vname, len+1);
						ident *id = idents->access(vname);
						bool proceed = true;
						int type = hdr.version >= 28 ? gzgetint(f)+(hdr.version >= 29 ? 0 : 1) : (id ? id->type : ID_VAR);
						if(!id || type != id->type)
						{
							if(id && hdr.version <= 28 && id->type == ID_FVAR && type == ID_VAR)
								type = ID_FVAR;
							else proceed = false;
						}
						if(!id || !(id->flags&IDF_WORLD)) proceed = false;

						switch(type)
						{
							case ID_VAR:
							{
								int val = hdr.version >= 25 ? gzgetint(f) : gzgetc(f);
								if(proceed)
								{
									if(val > id->maxval) val = id->maxval;
									else if(val < id->minval) val = id->minval;
									setvar(vname, val, true);
								}
								break;
							}
							case ID_FVAR:
							{
								float val = hdr.version >= 29 ? gzgetfloat(f) : float(gzgetint(f))/100.f;
								if(proceed) setfvar(vname, val, true);
								break;
							}
							case ID_SVAR:
							{
								int slen = gzgetint(f);
								string val;
								gzread(f, val, slen+1);
								if(proceed && slen) setsvar(vname, val, true);
								break;
							}
							default:
							{
								if(hdr.version <= 27)
								{
									if(hdr.version >= 25) gzgetint(f);
									else gzgetc(f);
								}
								proceed = false;
								break;
							}
						}
						if(!proceed) conoutf("WARNING: ignoring variable %s stored in map", vname);
					}
				}
				if(verbose >= 2) conoutf("loaded %d variables", vars);
			}

			if(!sv->canload(hdr.gameid))
			{
				conoutf("WARNING: loading map from %s game type in %s, ignoring game specific data", hdr.gameid, sv->gameid());
				samegame = false;
			}
		}
		else if(strncmp(newhdr.head, "OCTA", 4) == 0)
		{
			octa ohdr;
			memcpy(&ohdr, &newhdr, sizeof(binary));

			if(ohdr.version <= 25)
			{
				octacompat25 chdr;
				memcpy(&chdr, &ohdr, sizeof(binary));
				gzread(f, &chdr.worldsize, ohdr.headersize-sizeof(binary));
				endianswap(&chdr.worldsize, sizeof(int), 7);
				memcpy(&ohdr.worldsize, &chdr.worldsize, sizeof(int)*2);
				ohdr.numpvs = 0;
				memcpy(&ohdr.lightmaps, &chdr.lightmaps, ohdr.headersize-sizeof(binary)-sizeof(int)*2);
			}
			else
			{
				gzread(f, &ohdr.worldsize, ohdr.headersize-sizeof(binary));
				endianswap(&ohdr.worldsize, sizeof(int), 7);
			}

			if(ohdr.version > OCTAVERSION)
			{
				conoutf("error loading %s: requires a newer version of Cube 2 support", mapname);
				gzclose(f);
				s_sprintfd(m)("%s", mapname);
				emptymap(12, true, m);
				return;
			}

			maptype = MAP_OCTA;

			strncpy(hdr.head, ohdr.head, 4);
			hdr.gamever = 0; // sauer has no gamever
			hdr.worldsize = ohdr.worldsize;
			if(hdr.worldsize > 1<<18) hdr.worldsize = 1<<18;
			hdr.numents = ohdr.numents;
			hdr.numpvs = ohdr.numpvs;
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

			if(hdr.version >= 16)
			{
				int len = gzgetc(f);
				gzread(f, gameid, len+1);
			}
			strncpy(hdr.head, gameid, 4);

			if(!sv->canload(hdr.gameid))
			{
				conoutf("WARNING: loading map from %s sauerbraten game type in %s, ignoring game specific data", gameid, sv->gameid());
				samegame = false;
			}
			else if(verbose >= 2) conoutf("loading map from %s game", hdr.gameid);

			if(hdr.version>=16)
			{
				eif = readushort(f);
				int extrasize = readushort(f);
				loopj(extrasize) gzgetc(f);
			}

			if(hdr.version<25) hdr.numpvs = 0;
		}
		else continue;

		renderprogress(0, "clearing world...");

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

		renderprogress(0, "loading entities...");

		vector<extentity *> &ents = et->getents();
		loopi(hdr.numents)
		{
			if(verbose >= 2) renderprogress(float(i)/float(hdr.numents), "loading entities...");
			extentity &e = *et->newent();
			ents.add(&e);
			gzread(f, &e, sizeof(entity));
			endianswap(&e.o, sizeof(int), 3);
			endianswap(&e.attr1, sizeof(short), 5);
			e.links.setsize(0);
			e.spawned = false;
			e.inoctanode = false;
			if(maptype == MAP_OCTA) { loopj(eif) gzgetc(f); }
			et->readent(f, maptype, hdr.version, hdr.gameid, hdr.gamever, i, e);
			if(maptype == MAP_BFGZ && et->maylink(hdr.gamever <= 49 && e.type >= 10 ? e.type-1 : e.type, hdr.gamever))
			{
				int links = gzgetint(f);
				loopk(links)
				{
					int ln = gzgetint(f);
					e.links.add(ln);
				}
				if(verbose >= 2) conoutf("entity %s (%d) loaded %d link(s)", et->findname(e.type), i, links);
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
			if(!samegame && (e.type>=ET_GAMESPECIFIC || hdr.version<=14))
			{
				ents.pop();
				continue;
			}
			if(!insideworld(e.o))
			{
				if(e.type != ET_LIGHT && e.type != ET_SPOTLIGHT)
				{
					conoutf("WARNING: ent outside of world: enttype[%s] index %d (%f, %f, %f)", et->findname(e.type), i, e.o.x, e.o.y, e.o.z);
				}
			}
			if(hdr.version <= 14 && e.type == ET_MAPMODEL)
			{
				e.o.z += e.attr3;
				if(e.attr4) conoutf("WARNING: mapmodel ent (index %d) uses texture slot %d", i, e.attr4);
				e.attr3 = e.attr4 = 0;
			}
		}

		et->initents(f, maptype, hdr.version, hdr.gameid, hdr.gamever);
		if(verbose >= 2) conoutf("loaded %d entities", hdr.numents);

		renderprogress(0, "loading octree...");
		worldroot = loadchildren(f);

		if(hdr.version <= 11)
			swapXZ(worldroot);

		if(hdr.version <= 8)
			converttovectorworld();

		if(hdr.worldsize > VVEC_INT_MASK+1 && hdr.version <= 25)
			fixoversizedcubes(worldroot, hdr.worldsize>>1);

		renderprogress(0, "validating...");
		validatec(worldroot, hdr.worldsize>>1);

		worldscale = 0;
		while(1<<worldscale < hdr.worldsize) worldscale++;

		if(hdr.version >= 7) loopi(hdr.lightmaps)
		{
			if(verbose >= 2) renderprogress(i/(float)hdr.lightmaps, "loading lightmaps...");
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

		if(hdr.numpvs > 0) loadpvs(f);

		if(verbose >= 2) conoutf("loaded %d lightmaps", hdr.lightmaps);

		renderprogress(0, "loading world...");
		cl->loadworld(f, maptype);

		overrideidents = worldidents = true;
		persistidents = false;
		s_sprintfd(cfgname)("%s.cfg", mapname);
		if(maptype == MAP_OCTA)
		{
			exec("octa.cfg"); // for use with -pSAUER_DIR
			exec(cfgname);
		}
		else if(!execfile(cfgname)) exec("map.cfg");
		persistidents = true;
		overrideidents = worldidents = false;

		loopv(ents)
		{
			extentity &e = *ents[i];

			if((maptype == MAP_OCTA || (maptype == MAP_BFGZ && hdr.version <= 29)) &&
				ents[i]->type == ET_SPOTLIGHT)
			{
				int closest = -1;
				float closedist = 1e10f;
				loopvk(ents) if(ents[k]->type == ET_LIGHT)
				{
					extentity &a = *ents[k];
					float dist = e.o.dist(a.o);
					if(dist < closedist)
					{
						closest = i;
						closedist = dist;
					}
				}
				if(ents.inrange(closest) && closedist <= 100)
				{
					extentity &a = *ents[closest];
					a.links.add(i);
					conoutf("WARNING: auto import linked spotlight %d to light %d", i, closest);
				}
			}
			if(e.type == ET_MAPMODEL && e.attr2 >= 0)
			{
				mapmodelinfo &mmi = getmminfo(e.attr2);
				if(!&mmi) conoutf("could not find map model: %d", e.attr2);
				else if(!loadmodel(NULL, e.attr2, true))
					conoutf("could not load model: %s", mmi.name);
			}
		}

		gzclose(f);
		conoutf("loaded map %s v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-loadingstart)/1000.0f);

		//if((maptype == MAP_OCTA && hdr.version <= 26) || (maptype == MAP_BFGZ && hdr.version <= 28))
		//	mpremip(true);

		if((maptype == MAP_OCTA && hdr.version <= 25) || (maptype == MAP_BFGZ && hdr.version <= 26))
			fixlightmapnormals();

		initlights();
		allchanged(true);

		computescreen("loading...", mapshot!=notexture ? mapshot : NULL, mapname);
		renderprogress(0, "starting world...");
		startmap(mapname);
		RUNWORLD(on_start);
		return;
	}
	conoutf("unable to load %s", mapname);
	emptymap(12, true, mname);
}

void writeobj(char *name)
{
    s_sprintfd(fname)("%s.obj", name);
    FILE *f = openfile(fname, "w");
    if(!f) return;
    fprintf(f, "# obj file of sauerbraten level\n");
    extern vector<vtxarray *> valist;
    loopv(valist)
    {
        vtxarray &va = *valist[i];
        ushort *edata = NULL;
        uchar *vdata = NULL;
        if(!readva(&va, edata, vdata)) continue;
        int vtxsize = VTXSIZE;
        uchar *vert = vdata;
        loopj(va.verts)
        {
            vec v;
            if(floatvtx) (v = *(vec *)vert).div(1<<VVEC_FRAC);
            else v = ((vvec *)vert)->tovec(va.o).add(0x8000>>VVEC_FRAC);
            if(v.x != floor(v.x)) fprintf(f, "v %.3f ", v.x); else fprintf(f, "v %d ", int(v.x));
            if(v.y != floor(v.y)) fprintf(f, "%.3f ", v.y); else fprintf(f, "%d ", int(v.y));
            if(v.z != floor(v.z)) fprintf(f, "%.3f\n", v.z); else fprintf(f, "%d\n", int(v.z));
            vert += vtxsize;
        }
        ushort *tri = edata;
        loopi(va.tris)
        {
            fprintf(f, "f");
            for(int k = 0; k<3; k++) fprintf(f, " %d", tri[k]-va.verts-va.voffset);
            tri += 3;
            fprintf(f, "\n");
        }
        delete[] edata;
        delete[] vdata;
    }
    fclose(f);
}

COMMAND(writeobj, "s");

int getworldsize() { return hdr.worldsize; }
char *getmapname() { return mapname; }
ICOMMAND(mapname, "", (void), result(getmapname()));
int getmapversion() { return hdr.version; }
ICOMMAND(mapversion, "", (void), intret(getmapversion()));
int getmaprevision() { return hdr.revision; }
ICOMMAND(maprevision, "", (void), intret(getmaprevision()));
char *getmaptitle() { return hdr.maptitle; }
ICOMMAND(maptitle, "", (void), result(getmaptitle()));
