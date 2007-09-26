// Blood Frontier - SSPGAME - Side Scrolling Platformer by Quinton Reeves
// This is the SSP blocks

struct sspblock
{
	sspclient &cl;

	vector<blockent *> blocks;
	blockent *block;

	sspblock(sspclient &_cl) : cl(_cl), block(NULL)
	{
		// Variables
		#define blockvariable(a,b,c,d) \
			CVARF(sspblock, a, b, 0, c, if (self->block != NULL) { d; });

		blockvariable(bv_attr1,		-INT_MAX+1,	INT_MAX-1,	self->block->block1 = (short)*_storage);
		blockvariable(bv_attr2,		-INT_MAX+1,	INT_MAX-1,	self->block->block2 = (short)*_storage);
		blockvariable(bv_attr3,		-INT_MAX+1,	INT_MAX-1,	self->block->block3 = (short)*_storage);
		blockvariable(bv_attr4,		-INT_MAX+1,	INT_MAX-1,	self->block->block4 = (short)*_storage);
		blockvariable(bv_e1,		1,	-1, return);
		blockvariable(bv_e2,		1,	-1, return);
		blockvariable(bv_entity,	0,			SE_MAX-ET_GAMESPECIFIC,	self->block->blockentity = (uchar)*_storage);
		blockvariable(bv_push,		0,			1,			self->block->blockpush = *_storage ? true : false);
		blockvariable(bv_power,		-1,			INT_MAX-1,	self->block->sd.power = *_storage);
		blockvariable(bv_texture,	0,			1,			self->block->blockotex = *_storage ? true : false);
		blockvariable(bv_type,		0,			2,			
			self->block->blocktype = *_storage;
			self->block->state = self->block->blocktype == 2 ? CS_DEAD : CS_ALIVE;
		);

		// Commands
		#define blockscript(a,b,c) \
			CCOMMAND(sspblock, a, b, if (self->block != NULL) { c; });

		blockscript(bc_name,	"s",	s_strcpy(self->block->blockname, args[0]));
	}
	~sspblock() { }

	void varblock(blockent *d)
	{
		setvar("bv_attr1",		(int)d->block1);
		setvar("bv_attr2",		(int)d->block2);
		setvar("bv_attr3",		(int)d->block3);
		setvar("bv_attr4",		(int)d->block4);
		setvar("bv_e1",		(int)d->e1);
		setvar("bv_e2",		(int)d->e2);
		setvar("bv_entity",	(int)d->blockentity);
		setvar("bv_push",		d->blockpush ? 1 : 0);
		setvar("bv_power",		d->sd.power);
		setvar("bv_texture",	d->blockotex ? 1 : 0);
		setvar("bv_type",		d->blocktype);
	}

	void runblock(blockent *d, char *script)
	{
		if ((block = d) != NULL)
		{
			s_sprintfd(s)("bl_%d_%s", block->blockid, script);
			if (identexists(s))
			{
				varblock(d);
				execute(s);
				d->blockentity += ET_GAMESPECIFIC-1;
				cl.et.fixent(&d->blockentity, &d->block1, &d->block2, &d->block3, &d->block4, NULL);
				d->blockentity -= ET_GAMESPECIFIC-1;
			}
			block = NULL;
		}
	}

	void addblock(sspentity &e)
	{
		blockent *d = new blockent;

		d->o = e.o;
		
		d->blockid = e.attr1;
		d->blocktex = e.attr2;
		d->e1 = e.attr3;
		d->e2 = e.attr4;
		d->clamp = d->path = d->o.x;
		d->o.z += d->eyeheight;
		d->blockpos = d->o;

		blocks.add(d);
		
		runblock(d, "init");
	}

	bool damageblock(blockent *d, sspent *at, vec &o)
	{
		if (d->sd.power != 0 || d->blocktype > 0)
		{
			int power = d->sd.power;
			
			runblock(d, "hurt");
	
			if (d->sd.power > 0 && (d->blocktype > 0 || d->blockentity > 0 || at->sd.power > 1)) d->sd.power--;
	
			if (d->blocktype == 0 && d->sd.power <= 0)
			{
				int numdebris = rnd(getvar("debrisamt")-5)+5;
				loopi(numdebris)
				{
					vec w(0, rnd(int(ENTBLK*4))-int(ENTBLK*2), rnd(int(ENTBLK*2))+int(ENTBLK));
					vec v(w);
					v.normalize();
					w.add(d->o);
					cl.pr.spawnprojectile(w, v, d, 0.5f, 0.5f, PR_DEBRIS, rnd(4)+1, 0, 0, 0, 0, 0, rnd(1000)+1000, rnd(100)+20);
				}
				d->state = CS_DEAD;
			}
	
			loopi(cl.numdynents())
			{
				sspent *e = (sspent *)cl.iterdynents(i);
	
				if (e->type == ENT_PLAYER || e->type == ENT_AI)
				{
					if (e != NULL && !d->o.reject(e->o, d->radius+(e->radius*0.75f))) // hurt things standing on top of this
					{
						if (e->o.z-e->eyeheight >= d->o.z && e->o.z-e->eyeheight <= d->o.z+HITCLOB)
						{
							cl.wp.hitpush(1, (sspent *)e, (sspent *)d, d->o, e->o);
							e->lastaction = lastmillis;
							e->blocked = true;
						}
					}
				}
			}
	
			if (d->sd.power != power || d->blocktype == 0)
			{
				d->lastaction = lastmillis;

				if (d->blockentity > 0)
				{
					d->blockentity += ET_GAMESPECIFIC-1;
		
					short attr1 = d->blockentity == SE_POWERUP && !d->block2 && at->sd.power < 2 ? 
						(short)PW_HIT : d->block1;
		
					float height = cl.et.sspitems[d->blockentity].height;
					float radius = cl.et.sspitems[d->blockentity].radius;
					
					vec w(d->abovehead());
					vec v(0, 0, 1.f);
					if (d->blockentity == SE_POWERUP || d->blockentity == SE_COIN ||
						d->blockentity == SE_INVINCIBLE || d->blockentity == SE_LIFE)
					{
						if (isxaxis(at->direction)) v.y = o.y;
						else v.x = o.x;
					}
					else if (d->blockentity == SE_JUMPPAD)
					{
						height = radius = 0.5f;
					}
		
					v.add(w);
		
					cl.pr.newprojectile(w, v, at, height, radius, PR_ENT, d->blockentity, attr1, d->block2, d->block3, d->block4, 0, INT_MAX-1, getvar("entspeed"), true, false);
		
					d->blockentity -= ET_GAMESPECIFIC-1;
				}
				return true;
			}
			
			at->vel.z = -ENTBLK*ENTBLK;
		}

		return false;
	}

	void clearblocks()
	{
		blocks.deletecontentsp();
		blocks.setsize(0);
	}


	void checkblocks()
	{
		sspent *at = cl.player1;
	
		if (at->state == CS_ALIVE)
		{
			loopv(blocks)
			{
				blockent *d = blocks[i];
				
				if (d != NULL)
				{
					if (d->sd.power != 0 || d->blocktype > 0)
					{
						if (lastmillis-d->lastaction > BLKTIME/2 &&
							!d->o.reject(at->o, d->radius+at->radius+1))
						{
							if (d->blockpush && lastmillis-d->lastaction > BLKTIME && at->o.z-at->eyeheight >= d->o.z-d->eyeheight-1.f && at->o.z-at->eyeheight <= d->o.z)
							{
								d->lastaction = lastmillis;
								d->vel = dirvec[at->direction];
								d->vel.mul(75.f);
								return;
							}
							
							if (at->o.z >= d->o.z-d->eyeheight-HITCLOB && at->o.z <= d->o.z-d->eyeheight-(HITCLOB/2))
							{
								vec v(d->o);
								v.sub(at->o);
								v.normalize();
								if (damageblock(d, at, v)) return;
							}
						}
						if (d->blockpush) moveplayer((physent *)d, 2, false);
					}
				}
			}
		}
	}

	void renderblocks()
	{
		loopv(blocks)
		{
			blockent *d = blocks[i];

			if (d != NULL)
			{
				vec color, dir;

				if (d->blocktype == 1 || d->state == CS_ALIVE || lastmillis-d->lastaction < BLKTIME)
				{
					int anim = ANIM_MAPMODEL, basetime = d->lastaction;
					float myaw = d->yaw;
					vec o(d->o);
					o.z -= d->eyeheight;
	
					if (lastmillis-d->lastaction < BLKTIME)
					{
						if (d->blocktype > 0 || d->blockentity > 0 || d->sd.power != 0)
						{
							anim |= ANIM_START;
	
							if (lastmillis-d->lastaction < BLKTIME/2)
							{
								myaw += int(360 * (float(lastmillis-d->lastaction)/(BLKTIME/2.f))) % 360;
	
								o.z += (lastmillis-d->lastaction)/50.f;
								
								if (d->blocktype > 0 || d->blockentity > 0)
									part_spawn(o, vec(d->xradius, d->yradius, d->eyeheight), 0, 2, curtime/4, 500, COL_YELLOW);
							}
							else o.z += (BLKTIME-(lastmillis-d->lastaction))/50.f;
						}
						else
						{
							if (lastmillis-d->lastaction > BLKTIME/2) o.z -= ((lastmillis-d->lastaction)-(BLKTIME/2))/10.f;
						}
					}
					else anim |= ANIM_START;
	
					const char *mdlname = (d->blocktype > 0 ? "block/item" : "block/destruct");
					int mdlflags = MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_CULL_DIST|MDL_SHADOW;
					
					if (d->state == CS_DEAD || d->sd.power == 0) mdlflags |= MDL_TRANSLUCENT;
					
					rendermodel(color, dir, mdlname, anim, 0, d->blockotex ? 0 : d->blocktex, o, myaw+90, d->pitch/4, 10.0f, basetime, d, mdlflags, NULL);
				}
				if (editmode)
				{
					part_textf(d->abovehead(), "%s", false, 0, COL_LBLUE, d->blockname);

					if (d->blockentity > 0)
					{
						sspclient::sspentities::sspitem &t = cl.et.sspitems[d->blockentity+ET_GAMESPECIFIC-1];
						
						if (t.render)
						{
							char *imdl = cl.et.renderentmdl(d->blockentity+ET_GAMESPECIFIC-1, d->block1, d->block2, d->block3, d->block4, 0);
							if (imdl) rendermodel(color, dir, imdl, ANIM_MAPMODEL|ANIM_LOOP, 0, 0, vec(d->o).add(vec(0, 0, t.height)), d->yaw, d->pitch/4, 10.f, 0, d, MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_CULL_DIST|MDL_SHADOW, NULL);
						}
					}
				}
			}
		}
	}
};
