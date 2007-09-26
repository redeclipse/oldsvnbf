struct md2;

md2 *loadingmd2 = 0;

float md2normaltable[256][3] =
{
	{ -0.525731f,  0.000000f,  0.850651f },	 { -0.442863f,  0.238856f,  0.864188f },	 { -0.295242f,  0.000000f,  0.955423f },	 { -0.309017f,  0.500000f,  0.809017f }, 
	{ -0.162460f,  0.262866f,  0.951056f },	 {  0.000000f,  0.000000f,  1.000000f },	 {  0.000000f,  0.850651f,  0.525731f },	 { -0.147621f,  0.716567f,  0.681718f }, 
	{  0.147621f,  0.716567f,  0.681718f },	 {  0.000000f,  0.525731f,  0.850651f },	 {  0.309017f,  0.500000f,  0.809017f },	 {  0.525731f,  0.000000f,  0.850651f }, 
	{  0.295242f,  0.000000f,  0.955423f },	 {  0.442863f,  0.238856f,  0.864188f },	 {  0.162460f,  0.262866f,  0.951056f },	 { -0.681718f,  0.147621f,  0.716567f }, 
	{ -0.809017f,  0.309017f,  0.500000f },	 { -0.587785f,  0.425325f,  0.688191f },	 { -0.850651f,  0.525731f,  0.000000f },	 { -0.864188f,  0.442863f,  0.238856f }, 
	{ -0.716567f,  0.681718f,  0.147621f },	 { -0.688191f,  0.587785f,  0.425325f },	 { -0.500000f,  0.809017f,  0.309017f },	 { -0.238856f,  0.864188f,  0.442863f }, 
	{ -0.425325f,  0.688191f,  0.587785f },	 { -0.716567f,  0.681718f, -0.147621f },	 { -0.500000f,  0.809017f, -0.309017f },	 { -0.525731f,  0.850651f,  0.000000f }, 
	{  0.000000f,  0.850651f, -0.525731f },	 { -0.238856f,  0.864188f, -0.442863f },	 {  0.000000f,  0.955423f, -0.295242f },	 { -0.262866f,  0.951056f, -0.162460f }, 
	{  0.000000f,  1.000000f,  0.000000f },	 {  0.000000f,  0.955423f,  0.295242f },	 { -0.262866f,  0.951056f,  0.162460f },	 {  0.238856f,  0.864188f,  0.442863f }, 
	{  0.262866f,  0.951056f,  0.162460f },	 {  0.500000f,  0.809017f,  0.309017f },	 {  0.238856f,  0.864188f, -0.442863f },	 {  0.262866f,  0.951056f, -0.162460f }, 
	{  0.500000f,  0.809017f, -0.309017f },	 {  0.850651f,  0.525731f,  0.000000f },	 {  0.716567f,  0.681718f,  0.147621f },	 {  0.716567f,  0.681718f, -0.147621f }, 
	{  0.525731f,  0.850651f,  0.000000f },	 {  0.425325f,  0.688191f,  0.587785f },	 {  0.864188f,  0.442863f,  0.238856f },	 {  0.688191f,  0.587785f,  0.425325f }, 
	{  0.809017f,  0.309017f,  0.500000f },	 {  0.681718f,  0.147621f,  0.716567f },	 {  0.587785f,  0.425325f,  0.688191f },	 {  0.955423f,  0.295242f,  0.000000f }, 
	{  1.000000f,  0.000000f,  0.000000f },	 {  0.951056f,  0.162460f,  0.262866f },	 {  0.850651f, -0.525731f,  0.000000f },	 {  0.955423f, -0.295242f,  0.000000f }, 
	{  0.864188f, -0.442863f,  0.238856f },	 {  0.951056f, -0.162460f,  0.262866f },	 {  0.809017f, -0.309017f,  0.500000f },	 {  0.681718f, -0.147621f,  0.716567f }, 
	{  0.850651f,  0.000000f,  0.525731f },	 {  0.864188f,  0.442863f, -0.238856f },	 {  0.809017f,  0.309017f, -0.500000f },	 {  0.951056f,  0.162460f, -0.262866f }, 
	{  0.525731f,  0.000000f, -0.850651f },	 {  0.681718f,  0.147621f, -0.716567f },	 {  0.681718f, -0.147621f, -0.716567f },	 {  0.850651f,  0.000000f, -0.525731f }, 
	{  0.809017f, -0.309017f, -0.500000f },	 {  0.864188f, -0.442863f, -0.238856f },	 {  0.951056f, -0.162460f, -0.262866f },	 {  0.147621f,  0.716567f, -0.681718f }, 
	{  0.309017f,  0.500000f, -0.809017f },	 {  0.425325f,  0.688191f, -0.587785f },	 {  0.442863f,  0.238856f, -0.864188f },	 {  0.587785f,  0.425325f, -0.688191f }, 
	{  0.688191f,  0.587785f, -0.425325f },	 { -0.147621f,  0.716567f, -0.681718f },	 { -0.309017f,  0.500000f, -0.809017f },	 {  0.000000f,  0.525731f, -0.850651f }, 
	{ -0.525731f,  0.000000f, -0.850651f },	 { -0.442863f,  0.238856f, -0.864188f },	 { -0.295242f,  0.000000f, -0.955423f },	 { -0.162460f,  0.262866f, -0.951056f }, 
	{  0.000000f,  0.000000f, -1.000000f },	 {  0.295242f,  0.000000f, -0.955423f },	 {  0.162460f,  0.262866f, -0.951056f },	 { -0.442863f, -0.238856f, -0.864188f }, 
	{ -0.309017f, -0.500000f, -0.809017f },	 { -0.162460f, -0.262866f, -0.951056f },	 {  0.000000f, -0.850651f, -0.525731f },	 { -0.147621f, -0.716567f, -0.681718f }, 
	{  0.147621f, -0.716567f, -0.681718f },	 {  0.000000f, -0.525731f, -0.850651f },	 {  0.309017f, -0.500000f, -0.809017f },	 {  0.442863f, -0.238856f, -0.864188f }, 
	{  0.162460f, -0.262866f, -0.951056f },	 {  0.238856f, -0.864188f, -0.442863f },	 {  0.500000f, -0.809017f, -0.309017f },	 {  0.425325f, -0.688191f, -0.587785f }, 
	{  0.716567f, -0.681718f, -0.147621f },	 {  0.688191f, -0.587785f, -0.425325f },	 {  0.587785f, -0.425325f, -0.688191f },	 {  0.000000f, -0.955423f, -0.295242f }, 
	{  0.000000f, -1.000000f,  0.000000f },	 {  0.262866f, -0.951056f, -0.162460f },	 {  0.000000f, -0.850651f,  0.525731f },	 {  0.000000f, -0.955423f,  0.295242f }, 
	{  0.238856f, -0.864188f,  0.442863f },	 {  0.262866f, -0.951056f,  0.162460f },	 {  0.500000f, -0.809017f,  0.309017f },	 {  0.716567f, -0.681718f,  0.147621f }, 
	{  0.525731f, -0.850651f,  0.000000f },	 { -0.238856f, -0.864188f, -0.442863f },	 { -0.500000f, -0.809017f, -0.309017f },	 { -0.262866f, -0.951056f, -0.162460f }, 
	{ -0.850651f, -0.525731f,  0.000000f },	 { -0.716567f, -0.681718f, -0.147621f },	 { -0.716567f, -0.681718f,  0.147621f },	 { -0.525731f, -0.850651f,  0.000000f }, 
	{ -0.500000f, -0.809017f,  0.309017f },	 { -0.238856f, -0.864188f,  0.442863f },	 { -0.262866f, -0.951056f,  0.162460f },	 { -0.864188f, -0.442863f,  0.238856f }, 
	{ -0.809017f, -0.309017f,  0.500000f },	 { -0.688191f, -0.587785f,  0.425325f },	 { -0.681718f, -0.147621f,  0.716567f },	 { -0.442863f, -0.238856f,  0.864188f }, 
	{ -0.587785f, -0.425325f,  0.688191f },	 { -0.309017f, -0.500000f,  0.809017f },	 { -0.147621f, -0.716567f,  0.681718f },	 { -0.425325f, -0.688191f,  0.587785f }, 
	{ -0.162460f, -0.262866f,  0.951056f },	 {  0.442863f, -0.238856f,  0.864188f },	 {  0.162460f, -0.262866f,  0.951056f },	 {  0.309017f, -0.500000f,  0.809017f }, 
	{  0.147621f, -0.716567f,  0.681718f },	 {  0.000000f, -0.525731f,  0.850651f },	 {  0.425325f, -0.688191f,  0.587785f },	 {  0.587785f, -0.425325f,  0.688191f }, 
	{  0.688191f, -0.587785f,  0.425325f },	 { -0.955423f,  0.295242f,  0.000000f },	 { -0.951056f,  0.162460f,  0.262866f },	 { -1.000000f,  0.000000f,  0.000000f }, 
	{ -0.850651f,  0.000000f,  0.525731f },	 { -0.955423f, -0.295242f,  0.000000f },	 { -0.951056f, -0.162460f,  0.262866f },	 { -0.864188f,  0.442863f, -0.238856f }, 
	{ -0.951056f,  0.162460f, -0.262866f },	 { -0.809017f,  0.309017f, -0.500000f },	 { -0.864188f, -0.442863f, -0.238856f },	 { -0.951056f, -0.162460f, -0.262866f }, 
	{ -0.809017f, -0.309017f, -0.500000f },	 { -0.681718f,  0.147621f, -0.716567f },	 { -0.681718f, -0.147621f, -0.716567f },	 { -0.850651f,  0.000000f, -0.525731f }, 
	{ -0.688191f,  0.587785f, -0.425325f },	 { -0.587785f,  0.425325f, -0.688191f },	 { -0.425325f,  0.688191f, -0.587785f },	 { -0.425325f, -0.688191f, -0.587785f }, 
	{ -0.587785f, -0.425325f, -0.688191f },	 { -0.688191f, -0.587785f, -0.425325f }
};

struct md2 : vertmodel
{
	struct md2_header
	{
		int magic;
		int version;
		int skinwidth, skinheight;
		int framesize;
		int numskins, numvertices, numtexcoords;
		int numtriangles, numglcommands, numframes;
		int offsetskins, offsettexcoords, offsettriangles;
		int offsetframes, offsetglcommands, offsetend;
	};

	struct md2_vertex
	{
		uchar vertex[3], normalindex;
	};

	struct md2_frame
	{
		float	  scale[3];
		float	  translate[3];
		char		name[16];
	};
	
	md2(const char *name) : vertmodel(name) {}

	int type() { return MDL_MD2; }

	struct md2meshgroup : meshgroup
	{
		void gentcverts(int *glcommands, vector<tcvert> &tcverts, vector<tri> &tris)
		{
			hashtable<ivec, int> tchash;
			vector<ushort> idxs;
			for(int *command = glcommands; (*command)!=0;)
			{
				int numvertex = *command++;
				bool isfan;
				if(isfan = (numvertex<0)) numvertex = -numvertex;
				idxs.setsizenodelete(0);
				loopi(numvertex)
				{
					union { int i; float f; } u, v;
					u.i = *command++;
					v.i = *command++;
					int vindex = *command++;
					ivec tckey(u.i, v.i, vindex);
					int *idx = tchash.access(tckey);
					if(!idx)
					{
						idx = &tchash[tckey];
						*idx = tcverts.length();
						tcvert &tc = tcverts.add();
						tc.u = u.f;
						tc.v = v.f;
						tc.index = (ushort)vindex;
					}		
					idxs.add(*idx);
				}
				loopi(numvertex-2) 
				{ 
					tri &t = tris.add();
					if(isfan)
					{
						t.vert[0] = idxs[0];
						t.vert[1] = idxs[i+1];
						t.vert[2] = idxs[i+2];
					}
					else loopk(3) t.vert[k] = idxs[i&1 && k ? i+(1-(k-1))+1 : i+k];
				}
			}
		}
		
		bool load(char *filename)
		{
			FILE *file = openfile(filename, "rb");
			if(!file) return false;

			md2_header header;
			fread(&header, sizeof(md2_header), 1, file);
			endianswap(&header, sizeof(int), sizeof(md2_header)/sizeof(int));

			if(header.magic!=844121161 || header.version!=8) 
			{
				fclose(file);
				return false;
			}
		  
			name = newstring(filename);

			numframes = header.numframes;

			mesh &m = *new mesh;
			m.group = this;
			meshes.add(&m);

			int *glcommands = new int[header.numglcommands];
			fseek(file, header.offsetglcommands, SEEK_SET); 
			int numglcommands = fread(glcommands, sizeof(int), header.numglcommands, file);
			endianswap(glcommands, sizeof(int), numglcommands);
			if(numglcommands < header.numglcommands) memset(&glcommands[numglcommands], 0, (header.numglcommands-numglcommands)*sizeof(int));

			vector<tcvert> tcgen;
			vector<tri> trigen;
			gentcverts(glcommands, tcgen, trigen);
			delete[] glcommands;

			m.numtcverts = tcgen.length();
			m.tcverts = new tcvert[m.numtcverts];
			memcpy(m.tcverts, tcgen.getbuf(), m.numtcverts*sizeof(tcvert));
			m.numtris = trigen.length();
			m.tris = new tri[m.numtris];
			memcpy(m.tris, trigen.getbuf(), m.numtris*sizeof(tri));

			m.numverts = header.numvertices;
			m.verts = new vert[m.numverts*numframes];

			int frame_offset = header.offsetframes;
			vert *curvert = m.verts;
			loopi(header.numframes)
			{
				md2_frame frame;
				fseek(file, frame_offset, SEEK_SET);
				fread(&frame, sizeof(md2_frame), 1, file);
				endianswap(&frame, sizeof(float), 6);
				loopj(header.numvertices)
				{
					md2_vertex v;
					fread(&v, sizeof(md2_vertex), 1, file);
					curvert->pos = vec(v.vertex[0]*frame.scale[0]+frame.translate[0],
										-(v.vertex[1]*frame.scale[1]+frame.translate[1]),
										v.vertex[2]*frame.scale[2]+frame.translate[2]);
					float *norm = md2normaltable[v.normalindex];
					curvert->norm = vec(norm[0], -norm[1], norm[2]);
					curvert++;
				}
				frame_offset += header.framesize;
			}
				 
			fclose(file);

			return true;
		}
	};

	struct md2part : part
	{
		void getdefaultanim(animstate &as, int anim, int varseed, dynent *d)
		{
            //                      0              3              6   7   8   9   10   11  12  13   14  15  16  17
            //                      D    D    D    D    D    D    A   P   I   R,  E    J   T   W    FO  SA  GS  GI
            static int _frame[] = { 178, 184, 190, 183, 189, 197, 46, 54, 0,  40, 162, 67, 95, 112, 72, 84, 7,  6 };
            static int _range[] = { 6,   6,   8,   1,   1,   1,   8,  4,  40, 6,  1,   1,  17, 11,  12, 11, 18, 1 };
			//					  DE DY I  F  B  L  R  PU SH PA J	SI SW ED  LA  T	WI  LO  GS  GI
            static int animfr[] = { 5, 2, 8, 9, 9, 9, 9, 6, 6, 7, 11, 8, 9, 10, 14, 12, 13, 15, 16, 17 };
			
			anim &= ANIM_INDEX;
			if((size_t)anim >= sizeof(animfr)/sizeof(animfr[0]))
			{
				as.frame = 0;
				as.range = 1;
				return;
			}
			int n = animfr[anim];
            if(anim==ANIM_DYING || anim==ANIM_DEAD) n -= uint(varseed)%3;
			as.frame = _frame[n];
			as.range = _range[n];
		}
	};

	void render(int anim, int varseed, float speed, int basetime, float pitch, const vec &axis, dynent *d, modelattach *a, const vec &dir, const vec &campos, const plane &fogplane)
	{
		if(!loaded) return;

		parts[0]->render(anim, varseed, speed, basetime, pitch, axis, d, dir, campos, fogplane);

		if(a) for(int i = 0; a[i].name; i++)
		{
			md2 *m = (md2 *)a[i].m;
			if(!m) continue;
			m->setskin();
			part *p = m->parts[0];
			p->index = parts.length()+i;
			p->render(anim, varseed, speed, basetime, pitch, axis, d, dir, campos, fogplane);
		}
	}

    void extendbb(int frame, vec &center, vec &radius, modelattach &a)
    {
        vec acenter, aradius;
        a.m->boundbox(frame, acenter, aradius);
        vec bbmin, bbmax;
        loopi(3)
        {
            bbmin[i] = min(acenter[i]-aradius[i], center[i]-radius[i]);
            bbmax[i] = max(acenter[i]+aradius[i], center[i]+radius[i]);
        }
        radius = bbmax;
        radius.sub(bbmin).mul(0.5f);
        center = bbmin;
        center.add(radius);
    }   

	meshgroup *loadmeshes(char *name)
	{
		md2meshgroup *group = new md2meshgroup();
		if(!group->load(name)) { delete group; return NULL; }
		return group;
	}

	bool load()
	{ 
		if(loaded) return true;
		part &mdl = *new md2part;
		parts.add(&mdl);
		mdl.model = this;
		mdl.index = 0;
		const char *pname = parentdir(loadname);
		s_sprintfd(name1)("packages/models/%s/tris.md2", loadname);
		mdl.meshes = sharemeshes(path(name1));
		if(!mdl.meshes)
		{
			s_sprintfd(name2)("packages/models/%s/tris.md2", pname);	// try md2 in parent folder (vert sharing)
			mdl.meshes = loadmeshes(path(name2));
			if(!mdl.meshes) return false;
		}
		Texture *tex, *masks;
		loadskin(loadname, pname, tex, masks);
		mdl.initskins(tex, masks);
        if(tex==notexture) conoutf("could not load model skin for %s", name1);
		loadingmd2 = this;
		s_sprintfd(name3)("packages/models/%s/md2.cfg", loadname);
		if(!execfile(name3))
		{
			s_sprintf(name3)("packages/models/%s/md2.cfg", pname);
			execfile(name3);
		}
		loadingmd2 = 0;
		loopv(parts) parts[i]->meshes = parts[i]->meshes->scaleverts(scale/4.0f, i ? vec(0, 0, 0) : vec(translate.x, -translate.y, translate.z));
		return loaded = true;
	}
};

void md2pitch(float *pitchscale, float *pitchoffset, float *pitchmin, float *pitchmax)
{
	if(!loadingmd2 || loadingmd2->parts.empty()) { conoutf("not loading an md2"); return; }
	md2::part &mdl = *loadingmd2->parts.last();

	mdl.pitchscale = *pitchscale;
	mdl.pitchoffset = *pitchoffset;
	if(*pitchmin || *pitchmax)
	{
		mdl.pitchmin = *pitchmin;
		mdl.pitchmax = *pitchmax;
	}
	else
	{
		mdl.pitchmin = -360*mdl.pitchscale;
		mdl.pitchmax = 360*mdl.pitchscale;
	}
}

void md2anim(char *anim, int *frame, int *range, float *speed, int *priority)
{
	if(!loadingmd2 || loadingmd2->parts.empty()) { conoutf("not loading an md2"); return; }
	vector<int> anims;
	findanims(anim, anims);
	if(anims.empty()) conoutf("could not find animation %s", anim);
	else loopv(anims)
	{
		loadingmd2->parts.last()->setanim(anims[i], *frame, *range, *speed, *priority);
	}
}

COMMAND(md2pitch, "ffff");
COMMAND(md2anim, "siifi");

