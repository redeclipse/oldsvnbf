// worldio.cpp: loading & saving of maps and savegames

#include "cube.h"
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
SVARW(maptitle, "");

void setnames(const char *fname, int type)
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
                int numsurfs = 6;
                loopj(6)
                {
                    surfaceinfo &surface = c[i].ext->surfaces[j];
                    if(surface.lmid >= LMID_RESERVED || surface.layer!=LAYER_TOP)
                    {
                        mask |= 1 << j;
                        if(surface.layer&LAYER_BLEND) numsurfs++;
                    }
                }
				gzputc(f, mask);
				if(c[i].ext->material != MAT_AIR) gzputc(f, c[i].ext->material);
                loopj(numsurfs) if(j >= 6 || mask & (1 << j))
                {
                    surfaceinfo tmp = c[i].ext->surfaces[j];
                    endianswap(&tmp.x, sizeof(ushort), 2);
                    gzwrite(f, &tmp, sizeof(surfaceinfo));
                    if(j < 6 && c[i].ext->normals) gzwrite(f, &c[i].ext->normals[j], sizeof(surfacenormals));
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
            if((maptype == MAP_OCTA && hdr.version <= 26) || (maptype == MAP_BFGZ && hdr.version <= 30))
            {
                static uchar matconv[] = { MAT_AIR, MAT_WATER, MAT_CLIP, MAT_GLASS|MAT_CLIP, MAT_NOCLIP, MAT_LAVA|MAT_DEATH, MAT_AICLIP, MAT_DEATH };
                mat = size_t(mat) < sizeof(matconv)/sizeof(matconv[0]) ? matconv[mat] : MAT_AIR;
            }
            ext(c).material = mat;
        }
		if(mask & 0x3F)
		{
			uchar lit = 0, bright = 0;
            static surfaceinfo surfaces[12];
            memset(surfaces, 0, 6*sizeof(surfaceinfo));
			if(mask & 0x40) newnormals(c);
            int numsurfs = 6;
            loopi(numsurfs)
			{
                if(i >= 6 || mask & (1 << i))
				{
                    gzread(f, &surfaces[i], sizeof(surfaceinfo));
                    endianswap(&surfaces[i].x, sizeof(ushort), 2);
                    if(hdr.version < 10) ++surfaces[i].lmid;
                    if(hdr.version < 18)
                    {
                        if(surfaces[i].lmid >= LMID_AMBIENT1) ++surfaces[i].lmid;
                        if(surfaces[i].lmid >= LMID_BRIGHT1) ++surfaces[i].lmid;
                    }
                    if(hdr.version < 19)
                    {
                        if(surfaces[i].lmid >= LMID_DARK) surfaces[i].lmid += 2;
                    }
                    if(i < 6)
                    {
                        if(mask & 0x40) gzread(f, &c.ext->normals[i], sizeof(surfacenormals));
                        if(surfaces[i].layer != LAYER_TOP) lit |= 1 << i;
                        else if(surfaces[i].lmid == LMID_BRIGHT) bright |= 1 << i;
                        else if(surfaces[i].lmid != LMID_AMBIENT) lit |= 1 << i;
                        if(surfaces[i].layer&LAYER_BLEND) numsurfs++;
                    }
				}
                else surfaces[i].lmid = LMID_AMBIENT;
            }
            if(lit) newsurfaces(c, surfaces, numsurfs);
            else if(bright) brightencube(c);
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

void saveslotconfig(FILE *h, Slot &s, int index)
{
    if(index >= 0)
    {
        if(s.shader)
        {
            fprintf(h, "setshader %s\n", s.shader->name);
        }
        loopvj(s.params)
        {
            fprintf(h, "set%sparam", s.params[j].type == SHPARAM_LOOKUP ? "shader" : (s.params[j].type == SHPARAM_UNIFORM ? "uniform" : (s.params[j].type == SHPARAM_PIXEL ? "pixel" : "vertex")));
            if(s.params[j].type == SHPARAM_LOOKUP || s.params[j].type == SHPARAM_UNIFORM) fprintf(h, " \"%s\"", s.params[j].name);
            else fprintf(h, " %d", s.params[j].index);
            loopk(4) fprintf(h, " %f", s.params[j].val[k]);
            fprintf(h, "\n");
        }
    }
    loopvj(s.sts)
    {
        fprintf(h, "texture");
        if(index >= 0) fprintf(h, " %s", findtexturename(s.sts[j].type));
        else if(!j) fprintf(h, " %s", findmaterialname(-index));
        else fprintf(h, " 1");
        fprintf(h, " \"%s\"", s.sts[j].lname);
        if(!j)
        {
            fprintf(h, " %d %d %d %f",
                s.rotation, s.xoffset, s.yoffset, s.scale);
            if(index >= 0) fprintf(h, " // %d", index);
        }
        fprintf(h, "\n");
    }
    if(index >= 0)
    {
        if(s.scrollS != 0.f || s.scrollT != 0.f)
            fprintf(h, "texscroll %f %f\n", s.scrollS * 1000.0f, s.scrollT * 1000.0f);
        if(s.layer != 0)
        {
            if(s.layermaskname) fprintf(h, "texlayer %d \"%s\" %d %f\n", s.layer, s.layermaskname, s.layermaskmode, s.layermaskscale);
            else fprintf(h, "texlayer %d\n", s.layer);
        }
        if(s.autograss) fprintf(h, "autograss \"%s\"\n", s.autograss);
    }
    fprintf(h, "\n");
}

void save_config(char *mname)
{
	backup(mname, ".cfg", hdr.revision);
	s_sprintfd(fname)("%s.cfg", mname);
	FILE *h = openfile(fname, "w");
	if(!h) { conoutf("\frcould not write config to %s", fname); return; }

	// config
	fprintf(h, "// %s (%s)\n// Config generated by Blood Frontier\n\n", mapname, maptitle);

	int vars = 0;
	fprintf(h, "// Variables stored in map file, may be uncommented here, or changed from editmode.\n");
	enumerate(*idents, ident, id, {
		if(id.flags&IDF_WORLD) switch(id.type)
		{
            case ID_VAR: fprintf(h, "// %s %d\n", id.name, *id.storage.i); vars++;break;
            case ID_FVAR: fprintf(h, "// %s %s\n", id.name, floatstr(*id.storage.f)); vars++; break;
            case ID_SVAR: fprintf(h, "// %s [%s]\n", id.name, *id.storage.s); vars++;break;
			default: break;
		}
	});
	if(vars) fprintf(h, "\n");
	if(verbose >= 2) conoutf("\fwwrote %d variable values", vars);

	int aliases = 0;
	enumerate(*idents, ident, id, {
		if(id.type == ID_ALIAS && id.flags&IDF_WORLD && strlen(id.name) && strlen(id.action))
		{
			aliases++;
            fprintf(h, "\"%s\" = [%s]\n", id.name, id.action);
		}
	});
	if(aliases) fprintf(h, "\n");
	if(verbose >= 2) conoutf("\fwsaved %d aliases", aliases);

	// texture slots
	loopi(MAT_EDIT)
	{
		if(verbose) renderprogress(float(i)/float(MAT_EDIT), "saving material slots...");

		if(i == MAT_WATER || i == MAT_LAVA)
		{
			saveslotconfig(h, materialslots[i], -i);
		}
	}
	if(verbose) conoutf("\fwsaved %d material slots", MAT_EDIT);

	loopv(slots)
	{
		if(verbose) renderprogress(float(i)/float(slots.length()), "saving texture slots...");
		saveslotconfig(h, slots[i], i);
	}
	if(verbose) conoutf("\fwsaved %d texture slots", slots.length());

	loopv(mapmodels)
	{
		if(verbose) renderprogress(float(i)/float(mapmodels.length()), "saving mapmodel slots...");
		fprintf(h, "mmodel \"%s\"\n", mapmodels[i].name);
	}
	if(mapmodels.length()) fprintf(h, "\n");
	if(verbose) conoutf("\fwsaved %d mapmodel slots", mapmodels.length());

	loopv(mapsounds)
	{
		if(verbose) renderprogress(float(i)/float(mapsounds.length()), "saving mapsound slots...");
		fprintf(h, "mapsound \"%s\" %d \"%s\" %d %d\n", mapsounds[i].sample->name, mapsounds[i].vol, findmaterialname(mapsounds[i].material), mapsounds[i].maxrad, mapsounds[i].minrad);
	}
	if(mapsounds.length()) fprintf(h, "\n");
	if(verbose) conoutf("\fwsaved %d mapsound slots", mapsounds.length());

	fclose(h);
	if(verbose) conoutf("\fwsaved config %s", fname);
}
ICOMMAND(savemapconfig, "s", (char *mname), save_config(*mname ? mname : mapname));

VARFP(mapshotsize, 0, 256, INT_MAX-1, mapshotsize -= mapshotsize%2);

void save_mapshot(char *mname)
{
	backup(mname, ifmtexts[imageformat], hdr.revision);

    GLuint tex;
	glGenTextures(1, &tex);
	glViewport(0, 0, mapshotsize, mapshotsize);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
    ImageData image(mapshotsize, mapshotsize, 3);
	memset(image.data, 0, 3*mapshotsize*mapshotsize);
	glFrontFace(GL_CCW);
	drawcubemap(mapshotsize, 2, camera1->o, camera1->yaw, camera1->pitch, false, false, false);
	glReadPixels(0, 0, mapshotsize, mapshotsize, GL_RGB, GL_UNSIGNED_BYTE, image.data);

	saveimage(mname, image, imageformat, compresslevel, true);

	glDeleteTextures(1, &tex);
    glFrontFace(GL_CCW);
	glViewport(0, 0, screen->w, screen->h);
}
ICOMMAND(savemapshot, "s", (char *mname), save_mapshot(*mname ? mname : mapname));

VARP(autosaveconfig, 0, 1, 1);
VARP(autosavemapshot, 0, 1, 1);

void save_world(const char *mname, bool nodata, bool forcesave)
{
	int savingstart = SDL_GetTicks();

	setnames(mname, MAP_BFGZ);

	backup(mapname, mapexts[MAP_BFGZ].name, hdr.revision);
	gzFile f = opengzfile(mapfile, "wb9");
	if(!f) { conoutf("\frerror saving %s to %s: file error", mapname, mapfile); return; }

	if(autosavemapshot || forcesave) save_mapshot(mapname);
	if(autosaveconfig || forcesave) save_config(mapname);

	renderbackground("saving map..");
	strncpy(hdr.head, "BFGZ", 4);
	hdr.version = MAPVERSION;
	hdr.headersize = sizeof(bfgz);
	hdr.gamever = server::gamever();
	hdr.numents = 0;
	hdr.revision++;
	strncpy(hdr.gameid, server::gameid(), 4);

	const vector<extentity *> &ents = entities::getents();
	loopv(ents)
	{
		if(ents[i]->type!=ET_EMPTY)
		{
			hdr.numents++;
		}
	}

    hdr.numpvs = nodata ? 0 : getnumviewcells();
    hdr.blendmap = nodata ? 0 : shouldsaveblendmap();
	hdr.lightmaps = nodata ? 0 : lightmaps.length();

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
			if(verbose) renderprogress(float(vars)/float(numvars), "saving world variables...");
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

	if(verbose) conoutf("\fwsaved %d variables", vars);

	// texture slots
	writeushort(f, texmru.length());
	loopv(texmru) writeushort(f, texmru[i]);

	// entities
	int count = 0;
	loopv(ents) // extended
	{
		if(verbose) renderprogress(float(i)/float(ents.length()), "saving entities...");
		if(ents[i]->type!=ET_EMPTY)
		{
			entity tmp = *ents[i];
			endianswap(&tmp.o, sizeof(int), 3);
			endianswap(&tmp.attr, sizeof(short), ENTATTRS);
			gzwrite(f, &tmp, sizeof(entity));
			entities::writeent(f, i, *ents[i]);
			extentity &e = (extentity &)*ents[i];
			if(entities::maylink(e.type))
			{
				vector<int> links;
				int n = 0;
				loopvk(ents)
				{
					extentity &f = (extentity &)*ents[k];
					if(f.type != ET_EMPTY)
					{
						if(entities::maylink(f.type) && e.links.find(k) >= 0)
							links.add(n); // align to indices
						n++;
					}
				}

				gzputint(f, links.length());
				loopvj(links) gzputint(f, links[j]); // aligned index
				if(verbose >= 2) conoutf("\fwentity %s (%d) saved %d links", entities::findname(e.type), i, links.length());
			}
			count++;
		}
	}
	if(verbose) conoutf("\fwsaved %d entities", count);

	savec(worldroot, f, nodata);
    if(!nodata)
    {
        loopv(lightmaps)
        {
            if(verbose) renderprogress(float(i)/float(lightmaps.length()), "saving lightmaps...");
            LightMap &lm = lightmaps[i];
            gzputc(f, lm.type | (lm.unlitx>=0 ? 0x80 : 0));
            if(lm.unlitx>=0)
            {
                writeushort(f, ushort(lm.unlitx));
                writeushort(f, ushort(lm.unlity));
            }
            gzwrite(f, lm.data, lm.bpp*LM_PACKW*LM_PACKH);
        }
        if(verbose) conoutf("\fwsaved %d lightmaps", lightmaps.length());
        if(getnumviewcells()>0)
        {
            if(verbose) renderprogress(0, "saving PVS...");
            savepvs(f);
            if(verbose) conoutf("\fwsaved %d PVS view cells", getnumviewcells());
        }
        if(shouldsaveblendmap())
        {
            if(verbose) renderprogress(0, "saving blendmap...");
            saveblendmap(f);
            if(verbose) conoutf("\fwsaved blendmap");
        }
    }

	renderprogress(0, "saving world...");
	world::saveworld(f);
	gzclose(f);

	conoutf("\fwsaved map %s v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-savingstart)/1000.0f);
}

ICOMMAND(savemap, "s", (char *mname), save_world(*mname ? mname : mapname));
ICOMMAND(savecurrentmap, "", (), save_world(mapname));

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

static void sanevars()
{
	setvar("fullbright", 0, false);
	setvar("blankgeom", 0, false);
}

bool load_world(const char *mname, bool temp)		// still supports all map formats that have existed since the earliest cube betas!
{
	int loadingstart = SDL_GetTicks();
	loop(tempfile, 2) if(tempfile || temp)
	{
		loop(format, MAP_MAX)
		{
			setnames(mname, format);
			if(!tempfile) loopk(2)
			{
				s_sprintfd(s)("temp/%s", k ? mapfile : mapname);
				s_strcpy(k ? mapfile : mapname, s);
			}

			gzFile f = opengzfile(mapfile, "rb9");
			if(!f) continue;

			Texture *mapshot = notexture;
            const char *mapshotname = strrchr(mapname, '/');
            if(!mapshotname) mapshotname = strrchr(mapname, '\\');
            if(mapshotname) mapshotname++;
            else mapshotname = mapname;

			bool samegame = true;
			int eif = 0;

			bfgz newhdr;
			if(gzread(f, &newhdr, sizeof(binary))!=(int)sizeof(binary))
			{
				conoutf("\frerror loading %s: malformatted header", mapname);
				gzclose(f);
				return false;
			}
			endianswap(&newhdr.version, sizeof(int), 2);

			string oldmaptitle;
			if(strncmp(newhdr.head, "BFGZ", 4) == 0)
			{
				#define BFGZCOMPAT(ver) \
					bfgzcompat##ver chdr; \
					memcpy(&chdr, &newhdr, sizeof(binary)); \
					if((size_t)newhdr.headersize > sizeof(chdr) || gzread(f, &chdr.worldsize, newhdr.headersize-sizeof(binary))!=newhdr.headersize-(int)sizeof(binary)) \
					{ \
						conoutf("\frerror loading %s: malformatted header", mapname); \
						gzclose(f); \
						return false; \
					}
				if(newhdr.version <= 25)
				{
					BFGZCOMPAT(25);
					endianswap(&chdr.worldsize, sizeof(int), 5);
					memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*2);
					newhdr.numpvs = 0;
					newhdr.lightmaps = chdr.lightmaps;
					newhdr.blendmap = 0;
					memcpy(&newhdr.gamever, &chdr.gamever, sizeof(int)*2);
                    memcpy(&newhdr.gameid, &chdr.gameid, 4);
					s_strcpy(oldmaptitle, chdr.maptitle);
				}
				else if(newhdr.version <= 32)
				{
					BFGZCOMPAT(32);
					endianswap(&chdr.worldsize, sizeof(int), 6);
					memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*4);
					newhdr.blendmap = 0;
					memcpy(&newhdr.gamever, &chdr.gamever, sizeof(int)*2);
                    memcpy(&newhdr.gameid, &chdr.gameid, 4);
					s_strcpy(oldmaptitle, chdr.maptitle);
				}
				else if(newhdr.version <= 33)
				{
					BFGZCOMPAT(33);
					endianswap(&chdr.worldsize, sizeof(int), 7);
					memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*7);
					memcpy(&newhdr.gameid, &chdr.gameid, 4);
					s_strcpy(oldmaptitle, chdr.maptitle);
				}
				else
				{
					if((size_t)newhdr.headersize > sizeof(newhdr) || gzread(f, &newhdr.worldsize, newhdr.headersize-sizeof(binary))!=newhdr.headersize-(int)sizeof(binary))
					{
						conoutf("\frerror loading %s: malformatted header", mapname);
						gzclose(f);
						return false;
					}
					endianswap(&newhdr.worldsize, sizeof(int), 7);
					oldmaptitle[0] = 0;
				}

				if(newhdr.version > MAPVERSION)
				{
					conoutf("\frerror loading %s: requires a newer version of Blood Frontier", mapname);
					gzclose(f);
					return false;
				}

                resetmap(false);

				hdr = newhdr;

				mapshot = textureload(mapname, 0, true, false);
				renderbackground("loading...", mapshot!=notexture ? mapshot : NULL, mapshotname);

				maptype = MAP_BFGZ;

				if(hdr.version <= 24) s_strncpy(hdr.gameid, "bfa", 4); // all previous maps were bfa-fps
				if(verbose) conoutf("\fwloading v%d map from %s game v%d", hdr.version, hdr.gameid, hdr.gamever);

				if(hdr.version >= 25 || (hdr.version == 24 && hdr.gamever >= 44))
				{
					int numvars = hdr.version >= 25 ? gzgetint(f) : gzgetc(f), vars = 0;
					overrideidents = worldidents = true;
					persistidents = false;
					renderprogress(0, "loading variables...");
					loopi(numvars)
					{
						vars++;
						if(verbose) renderprogress(float(i)/float(vars), "loading variables...");
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
							if(!proceed && verbose)
								conoutf("\frWARNING: ignoring variable %s stored in map", vname);
						}
					}
					persistidents = true;
					overrideidents = worldidents = false;
					if(verbose) conoutf("\fwloaded %d variables", vars);
				}
				sanevars();

				if(!server::canload(hdr.gameid))
				{
					if(verbose) conoutf("\frWARNING: loading map from %s game type in %s, ignoring game specific data", hdr.gameid, server::gameid());
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
					if(gzread(f, &chdr.worldsize, sizeof(octacompat25)-sizeof(binary))!=sizeof(octacompat25)-(int)sizeof(binary))
					{
						conoutf("\frerror loading %s: malformatted header", mapname);
						gzclose(f);
						return false;
					}
					endianswap(&chdr.worldsize, sizeof(int), 7);
					memcpy(&ohdr.worldsize, &chdr.worldsize, sizeof(int)*2);
					ohdr.numpvs = 0;
					memcpy(&ohdr.lightmaps, &chdr.lightmaps, sizeof(octacompat25)-sizeof(binary)-sizeof(int)*3);
				}
				else
				{
					if(gzread(f, &ohdr.worldsize, sizeof(octa)-sizeof(binary))!=sizeof(octa)-(int)sizeof(binary))
					{
						conoutf("\frerror loading %s: malformatted header", mapname);
						gzclose(f);
						return false;
					}
					endianswap(&ohdr.worldsize, sizeof(int), 7);
				}
				s_strcpy(oldmaptitle, ohdr.maptitle);

				if(ohdr.version > OCTAVERSION)
				{
					conoutf("\frerror loading %s: requires a newer version of Cube 2 support", mapname);
					gzclose(f);
					return false;
				}

                resetmap(false);

				hdr = newhdr;

				mapshot = textureload(mapname, 0, true, false);
				renderbackground("loading...", mapshot!=notexture ? mapshot : NULL, mapshotname);

				maptype = MAP_OCTA;

				strncpy(hdr.head, ohdr.head, 4);
				hdr.gamever = 0; // sauer has no gamever
				hdr.worldsize = ohdr.worldsize;
				if(hdr.worldsize > 1<<18) hdr.worldsize = 1<<18;
				hdr.numents = ohdr.numents;
				hdr.numpvs = ohdr.numpvs;
				hdr.lightmaps = ohdr.lightmaps;
				hdr.revision = 1;

				if(ohdr.version<=20) conoutf("\frloading older / less efficient map format, may benefit from \"calclight 2\", then \"savecurrentmap\"");
				if(!ohdr.ambient) ohdr.ambient = 25;
				if(!ohdr.lerpsubdivsize)
				{
					if(!ohdr.lerpangle) ohdr.lerpangle = 44;
					ohdr.lerpsubdiv = 2;
					ohdr.lerpsubdivsize = 4;
				}

				sanevars();
				setvar("lightprecision", ohdr.mapprec ? ohdr.mapprec : 32);
				setvar("lighterror", ohdr.maple ? ohdr.maple : 8);
				setvar("bumperror", ohdr.mapbe ? ohdr.mapbe : 3);
				setvar("lightlod", ohdr.mapllod);
				setvar("ambient", ohdr.ambient);
				setvar("lerpangle", ohdr.lerpangle);
				setvar("lerpsubdiv", ohdr.lerpsubdiv);
				setvar("lerpsubdivsize", ohdr.lerpsubdivsize);

				string gameid;
				if(hdr.version >= 16)
				{
					int len = gzgetc(f);
					gzread(f, gameid, len+1);
				}
				else s_strcpy(gameid, "fps");
				strncpy(hdr.gameid, gameid, 4);

				if(!server::canload(hdr.gameid))
				{
					if(verbose) conoutf("\frWARNING: loading OCTA v%d map from %s game, ignoring game specific data", hdr.version, hdr.gameid);
					samegame = false;
				}
				else if(verbose) conoutf("\fwloading OCTA v%d map from %s game", hdr.version, hdr.gameid);

				if(hdr.version>=16)
				{
					eif = readushort(f);
					int extrasize = readushort(f);
					loopj(extrasize) gzgetc(f);
				}

				if(hdr.version<25) hdr.numpvs = 0;
				if(hdr.version<28) hdr.blendmap = 0;
			}
			else
			{
				gzclose(f);
				continue;
			}

			if((maptype == MAP_OCTA || (maptype == MAP_BFGZ && hdr.version <= 33)) && oldmaptitle[0])
				setsvar("maptitle", oldmaptitle, true);

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

			vector<extentity *> &ents = entities::getents();
			loopi(hdr.numents)
			{
				if(verbose) renderprogress(float(i)/float(hdr.numents), "loading entities...");
				extentity &e = *entities::newent();
				ents.add(&e);
				gzread(f, &e, sizeof(entity));
				endianswap(&e.o, sizeof(int), 3);
				endianswap(&e.attr, sizeof(short), ENTATTRS);
				e.links.setsize(0);
				e.spawned = false;
				e.lastemit = 0;
				e.inoctanode = false;
				if((maptype == MAP_OCTA && hdr.version <= 27) || (maptype == MAP_BFGZ && hdr.version <= 31))
					e.attr[4] = 0; // init ever-present attr5
				if(maptype == MAP_OCTA)
				{
					loopj(eif) gzgetc(f);
				}
				// sauerbraten version increments
				if(hdr.version <= 10 && e.type >= 7) e.type++;
				if(hdr.version <= 12 && e.type >= 8) e.type++;
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
					entities::deleteent(ents.pop());
					continue;
				}
				entities::readent(f, maptype, hdr.version, hdr.gameid, hdr.gamever, i, e);
				if(maptype == MAP_BFGZ && entities::maylink(hdr.gamever <= 49 && e.type >= 10 ? e.type-1 : e.type, hdr.gamever))
				{
					int links = gzgetint(f);
					loopk(links)
					{
						int ln = gzgetint(f);
						e.links.add(ln);
					}
					if(verbose >= 2) conoutf("\fwentity %s (%d) loaded %d link(s)", entities::findname(e.type), i, links);
				}
				if(maptype == MAP_OCTA && e.type == ET_PARTICLES && e.attr[0] >= 11)
				{
					if(e.attr[0] <= 12) e.attr[0] += 3;
					else e.attr[0] = 0; // bork it up
				}
				if(hdr.version <= 14 && e.type == ET_MAPMODEL)
				{
					e.o.z += e.attr[2];
					if(e.attr[3] && verbose) conoutf("\frWARNING: mapmodel ent (index %d) uses texture slot %d", i, e.attr[3]);
					e.attr[2] = e.attr[3] = 0;
				}
				if(hdr.version <= 31 && e.type == ET_MAPMODEL)
				{
					int angle = e.attr[0];
					e.attr[0] = e.attr[1];
					e.attr[1] = angle;
					e.attr[2] = e.attr[3] = e.attr[4] = 0;
				}
				if(verbose && !insideworld(e.o) && e.type != ET_LIGHT && e.type != ET_SPOTLIGHT)
					conoutf("\frWARNING: ent outside of world: enttype[%s] index %d (%f, %f, %f)", entities::findname(e.type), i, e.o.x, e.o.y, e.o.z);
			}
			entities::initents(f, maptype, hdr.version, hdr.gameid, hdr.gamever);
			if(verbose) conoutf("\fwloaded %d entities", hdr.numents);

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
				if(verbose) renderprogress(i/(float)hdr.lightmaps, "loading lightmaps...");
				LightMap &lm = lightmaps.add();
				if(hdr.version >= 17)
				{
					int type = gzgetc(f);
					lm.type = type&0x7F;
					if(hdr.version >= 20 && type&0x80)
					{
						lm.unlitx = readushort(f);
						lm.unlity = readushort(f);
					}
				}
				if(lm.type&LM_ALPHA && (lm.type&LM_TYPE)!=LM_BUMPMAP1) lm.bpp = 4;
				lm.data = new uchar[lm.bpp*LM_PACKW*LM_PACKH];
				gzread(f, lm.data, lm.bpp * LM_PACKW * LM_PACKH);
				lm.finalize();
			}

			if(hdr.numpvs > 0) loadpvs(f);
			if(hdr.blendmap) loadblendmap(f);

			if(verbose) conoutf("\fwloaded %d lightmaps", hdr.lightmaps);

			renderprogress(0, "loading world...");
			world::loadworld(f, maptype);

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

			vector<int> mapmodels;
			loopv(ents)
			{
				extentity &e = *ents[i];

				if((maptype == MAP_OCTA || (maptype == MAP_BFGZ && hdr.version <= 29)) && ents[i]->type == ET_SPOTLIGHT)
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
						if(a.links.find(i) < 0)
						{
							a.links.add(i);
							if(verbose) conoutf("\frWARNING: auto linked spotlight %d to light %d", i, closest);
						}
					}
				}
				if(e.type == ET_MAPMODEL && e.attr[0] >= 0)
				{
					if(mapmodels.find(e.attr[0]) < 0) mapmodels.add(e.attr[0]);
				}
			}

			loopv(mapmodels)
			{
				loadprogress = float(i+1)/mapmodels.length();
				int mmindex = mapmodels[i];
				mapmodelinfo &mmi = getmminfo(mmindex);
				if(!&mmi) conoutf("\frcould not find map model: %d", mmindex);
				else if(!loadmodel(NULL, mmindex, true))
					conoutf("\frcould not load model: %s", mmi.name);
			}
			loadprogress = 0;

			gzclose(f);
			conoutf("\fwloaded map %s v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-loadingstart)/1000.0f);

			if((maptype == MAP_OCTA && hdr.version <= 25) || (maptype == MAP_BFGZ && hdr.version <= 26))
				fixlightmapnormals();

			entitiesinoctanodes();
			initlights();
			allchanged(true);

			renderbackground("loading...", mapshot!=notexture ? mapshot : NULL, mapshotname);
			renderprogress(0, "starting world...");
			world::startmap(mapname);
			return true;
		}
	}
	conoutf("\frunable to load %s", mname);
	return false;
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
char *getmaptitle() { return maptitle; }
ICOMMAND(maptitle, "", (void), result(getmaptitle()));
