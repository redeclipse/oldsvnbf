#include "pch.h"
#include "engine.h"

Texture *sky[6] = { 0, 0, 0, 0, 0, 0 };

void loadsky(char *basename)
{
	loopi(6)
	{
        const char *side = cubemapsides[i].name;
		s_sprintfd(name)("%s_%s", basename, side);
		if((sky[i] = textureload(name, 3, true, false))==notexture)
			conoutf("could not load sky texture %s_%s", basename, side);
	}
}

SVARFW(skybox, "skyboxes/black", { if(skybox[0]) loadsky(skybox); });
FVARW(spinsky, 0);
VARW(yawsky, 0, 0, 360);

void draw_envbox_face(float s0, float t0, int x0, int y0, int z0,
					  float s1, float t1, int x1, int y1, int z1,
					  float s2, float t2, int x2, int y2, int z2,
					  float s3, float t3, int x3, int y3, int z3,
					  GLuint texture)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glBegin(GL_QUADS);
	glTexCoord2f(s3, t3); glVertex3i(x3, y3, z3);
	glTexCoord2f(s2, t2); glVertex3i(x2, y2, z2);
	glTexCoord2f(s1, t1); glVertex3i(x1, y1, z1);
	glTexCoord2f(s0, t0); glVertex3i(x0, y0, z0);
	glEnd();
	xtraverts += 4;
}

void draw_envbox(int w, float zclip = 0.0f, int faces = 0x3F)
{
    float vclip = 1-zclip;
    int z = int(ceil(2*w*(vclip-0.5f)));

    glDepthMask(GL_FALSE);

    if(faces&0x01)
        draw_envbox_face(0.0f, 0.0f, -w, -w, -w,
                         1.0f, 0.0f, -w,  w, -w,
                         1.0f, vclip, -w,  w,  z,
                         0.0f, vclip, -w, -w,  z, sky[0] ? sky[0]->id : notexture->id);

    if(faces&0x02)
        draw_envbox_face(1.0f, vclip, +w, -w,  z,
                         0.0f, vclip, +w,  w,  z,
                         0.0f, 0.0f, +w,  w, -w,
                         1.0f, 0.0f, +w, -w, -w, sky[1] ? sky[1]->id : notexture->id);

    if(faces&0x04)
        draw_envbox_face(1.0f, vclip, -w, -w,  z,
                         0.0f, vclip,  w, -w,  z,
                         0.0f, 0.0f,  w, -w, -w,
                         1.0f, 0.0f, -w, -w, -w, sky[2] ? sky[2]->id : notexture->id);

    if(faces&0x08)
        draw_envbox_face(1.0f, vclip, +w,  w,  z,
                         0.0f, vclip, -w,  w,  z,
                         0.0f, 0.0f, -w,  w, -w,
                         1.0f, 0.0f, +w,  w, -w, sky[3] ? sky[3]->id : notexture->id);

    if(!zclip && faces&0x10)
        draw_envbox_face(0.0f, 1.0f, -w,  w,  w,
                         0.0f, 0.0f, +w,  w,  w,
                         1.0f, 0.0f, +w, -w,  w,
                         1.0f, 1.0f, -w, -w,  w, sky[4] ? sky[4]->id : notexture->id);

    if(faces&0x20)
        draw_envbox_face(0.0f, 1.0f, +w,  w, -w,
                         0.0f, 0.0f, -w,  w, -w,
                         1.0f, 0.0f, -w, -w, -w,
                         1.0f, 1.0f, +w, -w, -w, sky[5] ? sky[5]->id : notexture->id);

    glDepthMask(GL_TRUE);
}

VARP(sparklyfix, 0, 0, 1);
VAR(showsky, 0, 1, 1);
VAR(clipsky, 0, 1, 1);

bool drawskylimits(bool explicitonly)
{
    nocolorshader->set();

    glDisable(GL_TEXTURE_2D);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    bool rendered = rendersky(explicitonly);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_TEXTURE_2D);

    return rendered;
}

void drawskyoutline()
{
	notextureshader->set();

	glDisable(GL_TEXTURE_2D);
	glDepthMask(GL_FALSE);
	extern int wireframe;
    if(!wireframe)
    {
        glEnable(GL_POLYGON_OFFSET_LINE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
	glColor3f(0.5f, 0.0f, 0.5f);
	rendersky(true);
    if(!wireframe)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glDisable(GL_POLYGON_OFFSET_LINE);
    }
	glDepthMask(GL_TRUE);
	glEnable(GL_TEXTURE_2D);

	if(!glaring) defaultshader->set();
}

void drawskybox(int farplane, bool limited)
{
    extern int renderedskyfaces, renderedskyclip; // , renderedsky, renderedexplicitsky;
    bool alwaysrender = editmode || !insideworld(camera1->o) || reflecting,
         explicitonly = false;
    if(limited)
    {
        explicitonly = alwaysrender || !sparklyfix || refracting;
        if(!drawskylimits(explicitonly) && !alwaysrender) return;
        extern int ati_skybox_bug;
        if(!alwaysrender && !renderedskyfaces && !ati_skybox_bug) explicitonly = false;
    }
    else if(!alwaysrender)
    {
        extern vtxarray *visibleva;
        renderedskyfaces = 0;
        renderedskyclip = INT_MAX;
        for(vtxarray *va = visibleva; va; va = va->next)
        {
            if(va->occluded >= OCCLUDE_BB && va->skyfaces&0x80) continue;
            renderedskyfaces |= va->skyfaces&0x3F;
            if(!(va->skyfaces&0x1F) || camera1->o.z < va->skyclip) renderedskyclip = min(renderedskyclip, va->skyclip);
            else renderedskyclip = 0;
        }
        if(!renderedskyfaces) return;
    }

    if(alwaysrender)
    {
        renderedskyfaces = 0x3F;
        renderedskyclip = 0;
    }
    else if(spinsky && renderedskyfaces&0x0F) renderedskyfaces |= 0x0F;

    if(glaring)
    {
        static Shader *skyboxglareshader = NULL;
        if(!skyboxglareshader) skyboxglareshader = lookupshaderbyname("skyboxglare");
        skyboxglareshader->set();
    }
    else defaultshader->set();

    bool fog = glIsEnabled(GL_FOG)==GL_TRUE;
    if(fog) glDisable(GL_FOG);

    glPushMatrix();
    glLoadIdentity();
    glRotatef(camera1->roll, 0, 0, 1);
    glRotatef(camera1->pitch, -1, 0, 0);
    glRotatef(camera1->yaw+spinsky*lastmillis/1000.0f+yawsky, 0, 1, 0);
    glRotatef(90, 1, 0, 0);
    if(reflecting) glScalef(1, 1, -1);
    glColor3f(1, 1, 1);
    if(limited)
    {
        if(explicitonly) glDisable(GL_DEPTH_TEST);
        else glDepthFunc(GL_GEQUAL);
    }
    float skyclip = clipsky ? max(renderedskyclip-1, 0) : 0;
    if(reflectz<hdr.worldsize && reflectz>skyclip) skyclip = reflectz;
    draw_envbox(farplane/2, skyclip ? 0.5f + 0.5f*(skyclip-camera1->o.z)/float(hdr.worldsize) : 0, renderedskyfaces);
    glPopMatrix();

    if(limited)
    {
        if(explicitonly) glEnable(GL_DEPTH_TEST);
        else glDepthFunc(GL_LESS);
        if(!reflecting && !refracting && !envmapping && editmode && showsky) drawskyoutline();
    }

    if(fog) glEnable(GL_FOG);
}

VARN(skytexture, useskytexture, 0, 1, 1);

int explicitsky = 0;
double skyarea = 0;

bool limitsky()
{
    return (explicitsky && (useskytexture || editmode)) || (sparklyfix && skyarea / (double(hdr.worldsize)*double(hdr.worldsize)*6) < 0.9);
}

