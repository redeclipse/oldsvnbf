#include "pch.h"
#include "engine.h"

Texture *sky[6] = { 0, 0, 0, 0, 0, 0 };
float spinsky = 0;
string lastsky = "";

void loadsky(char *basename, float *spin)
{
	spinsky = *spin;

	if(strcmp(lastsky, basename)==0) return;
	static char *side[] = { "ft", "bk", "lf", "rt", "dn", "up" };
	loopi(6)
	{
		s_sprintfd(name)("%s_%s.jpg", basename, side[i]);
        if((sky[i] = textureload(name, 3, true, false))==notexture)
		{
			strcpy(name+strlen(name)-3, "png");
            if((sky[i] = textureload(name, 3, true, false))==notexture) conoutf("could not load sky texture %s_%s", basename, side[i]);
		}
	}
	s_strcpy(lastsky, basename);
}

COMMAND(loadsky, "sf");

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

void draw_envbox(int w, float zclip = 0.0f)
{
	if(!sky[0]) fatal("no skybox");

	float vclip = 1-zclip;
	int z = int(ceil(2*w*(vclip-0.5f)));

	glDepthMask(GL_FALSE);

	draw_envbox_face(1.0f, vclip, -w, -w,  z,
					 0.0f, vclip,  w, -w,  z,
					 0.0f, 0.0f,  w, -w, -w,
					 1.0f, 0.0f, -w, -w, -w, sky[0]->gl);

	draw_envbox_face(1.0f, vclip, +w,  w,  z,
					 0.0f, vclip, -w,  w,  z,
					 0.0f, 0.0f, -w,  w, -w,
					 1.0f, 0.0f, +w,  w, -w, sky[1]->gl);

	draw_envbox_face(0.0f, 0.0f, -w, -w, -w,
					 1.0f, 0.0f, -w,  w, -w,
					 1.0f, vclip, -w,  w,  z,
					 0.0f, vclip, -w, -w,  z, sky[2]->gl);

	draw_envbox_face(1.0f, vclip, +w, -w,  z,
					 0.0f, vclip, +w,  w,  z,
					 0.0f, 0.0f, +w,  w, -w,
					 1.0f, 0.0f, +w, -w, -w, sky[3]->gl);

	if(!zclip)
		draw_envbox_face(0.0f, 1.0f, -w,  w,  w,
						 0.0f, 0.0f, +w,  w,  w,
						 1.0f, 0.0f, +w, -w,  w,
						 1.0f, 1.0f, -w, -w,  w, sky[4]->gl);

	draw_envbox_face(0.0f, 1.0f, +w,  w, -w,
					 0.0f, 0.0f, -w,  w, -w,
					 1.0f, 0.0f, -w, -w, -w,
					 1.0f, 1.0f, +w, -w, -w, sky[5]->gl);

	glDepthMask(GL_TRUE);
}

VARP(sparklyfix, 0, 1, 1);
VAR(showsky, 0, 1, 1);

bool drawskylimits(bool explicitonly, float zreflect)
{
	nocolorshader->set();

	glDisable(GL_TEXTURE_2D);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	bool rendered = rendersky(explicitonly, zreflect);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glEnable(GL_TEXTURE_2D);

	defaultshader->set();

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

	defaultshader->set();
}

void drawskybox(int farplane, bool limited, float zreflect)
{
	if(limited && !zreflect)
	{
		if(!drawskylimits(false, 0) && !editmode && insideworld(camera1->o)) return;
	}

    bool fog = glIsEnabled(GL_FOG)==GL_TRUE;
    if(fog) glDisable(GL_FOG);

	glPushMatrix();
	glLoadIdentity();
	glRotatef(camera1->roll, 0, 0, 1);
	glRotatef(camera1->pitch, -1, 0, 0);
	glRotatef(camera1->yaw+spinsky*lastmillis/1000.0f, 0, 1, 0);
	glRotatef(90, 1, 0, 0);
	if(zreflect && camera1->o.z>=zreflect) glScalef(1, 1, -1);
	glColor3f(1, 1, 1);
	extern int ati_skybox_bug;
	if(limited) glDepthFunc(editmode || !insideworld(camera1->o) || ati_skybox_bug || zreflect ? GL_ALWAYS : GL_GEQUAL);
	draw_envbox(farplane/2, zreflect ? (zreflect+0.5f*(farplane-hdr.worldsize))/farplane : 0);
	glPopMatrix();

	if(limited)
	{
		glDepthFunc(GL_LESS);
		if(!zreflect && editmode && showsky) drawskyoutline();
	}

    if(fog) glEnable(GL_FOG);
}

bool limitsky()
{
	extern int ati_skybox_bug, explicitsky;
	extern double skyarea;
	return explicitsky || (!ati_skybox_bug && sparklyfix && skyarea / (double(hdr.worldsize)*double(hdr.worldsize)*6) < 0.9);
}

