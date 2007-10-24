// rendergl.cpp: core opengl rendering stuff

#include "pch.h"
#include "engine.h"

bool hasVBO = false, hasDRE = false, hasOQ = false, hasTR = false, hasFBO = false, hasDS = false, hasTF = false, hasBE = false, hasCM = false, hasNP2 = false, hasTC = false, hasTE = false, hasMT = false, hasD3, hasstencil = false;
int renderpath;

// GL_ARB_vertex_buffer_object
PFNGLGENBUFFERSARBPROC	glGenBuffers_	= NULL;
PFNGLBINDBUFFERARBPROC	glBindBuffer_	= NULL;
PFNGLMAPBUFFERARBPROC	 glMapBuffer_	 = NULL;
PFNGLUNMAPBUFFERARBPROC	glUnmapBuffer_	= NULL;
PFNGLBUFFERDATAARBPROC	glBufferData_	= NULL;
PFNGLBUFFERSUBDATAARBPROC glBufferSubData_ = NULL;
PFNGLDELETEBUFFERSARBPROC glDeleteBuffers_ = NULL;

// GL_ARB_multitexture
PFNGLACTIVETEXTUREARBPROC		glActiveTexture_		= NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTexture_ = NULL;
PFNGLMULTITEXCOORD2FARBPROC	 glMultiTexCoord2f_	 = NULL;
PFNGLMULTITEXCOORD3FARBPROC	 glMultiTexCoord3f_	 = NULL;
 
// GL_ARB_vertex_program, GL_ARB_fragment_program
PFNGLGENPROGRAMSARBPROC			glGenPrograms_			= NULL;
PFNGLDELETEPROGRAMSARBPROC		 glDeletePrograms_		 = NULL;
PFNGLBINDPROGRAMARBPROC			glBindProgram_			= NULL;
PFNGLPROGRAMSTRINGARBPROC		  glProgramString_		  = NULL;
PFNGLGETPROGRAMIVARBPROC           glGetProgramiv_           = NULL;

PFNGLPROGRAMENVPARAMETER4FARBPROC  glProgramEnvParameter4f_  = NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fv_ = NULL;

// GL_ARB_occlusion_query
PFNGLGENQUERIESARBPROC		glGenQueries_		= NULL;
PFNGLDELETEQUERIESARBPROC	 glDeleteQueries_	 = NULL;
PFNGLBEGINQUERYARBPROC		glBeginQuery_		= NULL;
PFNGLENDQUERYARBPROC		  glEndQuery_		  = NULL;
PFNGLGETQUERYIVARBPROC		glGetQueryiv_		= NULL;
PFNGLGETQUERYOBJECTIVARBPROC  glGetQueryObjectiv_  = NULL;
PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuiv_ = NULL;

// GL_EXT_framebuffer_object
PFNGLBINDRENDERBUFFEREXTPROC		glBindRenderbuffer_		= NULL;
PFNGLDELETERENDERBUFFERSEXTPROC	 glDeleteRenderbuffers_	 = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC		 glGenRenderbuffers_		= NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC	 glRenderbufferStorage_	 = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  glCheckFramebufferStatus_  = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC		 glBindFramebuffer_		 = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC	  glDeleteFramebuffers_	  = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC		 glGenFramebuffers_		 = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC	glFramebufferTexture2D_	= NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbuffer_ = NULL;
PFNGLGENERATEMIPMAPEXTPROC		  glGenerateMipmap_		  = NULL;

// GL_ARB_shading_language_100, GL_ARB_shader_objects, GL_ARB_fragment_shader, GL_ARB_vertex_shader
PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObject_	  = NULL;
PFNGLDELETEOBJECTARBPROC			  glDeleteObject_			 = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC		  glUseProgramObject_		 = NULL; 
PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObject_		= NULL;
PFNGLSHADERSOURCEARBPROC			  glShaderSource_			 = NULL;
PFNGLCOMPILESHADERARBPROC			 glCompileShader_			= NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC	  glGetObjectParameteriv_	 = NULL;
PFNGLATTACHOBJECTARBPROC			  glAttachObject_			 = NULL;
PFNGLGETINFOLOGARBPROC				glGetInfoLog_				= NULL;
PFNGLLINKPROGRAMARBPROC				glLinkProgram_			  = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocation_		= NULL;
PFNGLUNIFORM4FVARBPROC				glUniform4fv_				= NULL;
PFNGLUNIFORM1IARBPROC				 glUniform1i_				= NULL;

// GL_EXT_draw_range_elements
PFNGLDRAWRANGEELEMENTSEXTPROC glDrawRangeElements_ = NULL;

// GL_EXT_blend_minmax
PFNGLBLENDEQUATIONEXTPROC glBlendEquation_ = NULL;

void *getprocaddress(const char *name)
{
	return SDL_GL_GetProcAddress(name);
}

VARP(ati_skybox_bug, 0, 0, 1);
#ifdef BFRONTIER
VARP(ati_texgen_bug, 0, 0, 1);
VARP(ati_oq_bug, 0, 0, 1);
VARP(nvidia_texgen_bug, 0, 0, 1);
VARP(apple_glsldepth_bug, 0, 0, 1);
VARP(apple_minmax_bug, 0, 0, 1);
VARP(intel_quadric_bug, 0, 0, 1);
#else
VAR(ati_texgen_bug, 0, 0, 1);
VAR(ati_oq_bug, 0, 0, 1);
VAR(nvidia_texgen_bug, 0, 0, 1);
VAR(apple_glsldepth_bug, 0, 0, 1);
VAR(apple_minmax_bug, 0, 0, 1);
VAR(intel_quadric_bug, 0, 0, 1);
#endif
VAR(minimizetcusage, 1, 0, 0);
VAR(emulatefog, 1, 0, 0);

void gl_init(int w, int h, int bpp, int depth, int fsaa)
{
	#define fogvalues 0.5f, 0.6f, 0.7f, 1.0f

	glViewport(0, 0, w, h);
	glClearColor(fogvalues);
	glClearDepth(1);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);
	
	
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 0.25f);
	glHint(GL_FOG_HINT, GL_NICEST);
	GLfloat fogcolor[4] = { fogvalues };
	glFogfv(GL_FOG_COLOR, fogcolor);
	

	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glPolygonOffset(-3.0f, -3.0f);

	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

	const char *vendor = (const char *)glGetString(GL_VENDOR);
	const char *exts = (const char *)glGetString(GL_EXTENSIONS);
	const char *renderer = (const char *)glGetString(GL_RENDERER);
	const char *version = (const char *)glGetString(GL_VERSION);
	conoutf("Renderer: %s (%s)", renderer, vendor);
	conoutf("Driver: %s", version);
	
	//extern int shaderprecision;
	// default to low precision shaders on certain cards, can be overridden with -f3
	// char *weakcards[] = { "GeForce FX", "Quadro FX", "6200", "9500", "9550", "9600", "9700", "9800", "X300", "X600", "FireGL", "Intel", "Chrome", NULL } 
	// if(shaderprecision==2) for(char **wc = weakcards; *wc; wc++) if(strstr(renderer, *wc)) shaderprecision = 1;
  
	if(strstr(exts, "GL_EXT_texture_env_combine") || strstr(exts, "GL_ARB_texture_env_combine")) hasTE = true;
	else conoutf("WARNING: No texture_env_combine extension! (your video card is WAY too old)");

	if(strstr(exts, "GL_EXT_texture_env_dot3") || strstr(exts, "GL_ARB_texture_env_dot3")) hasD3 = true;

	if(strstr(exts, "GL_ARB_multitexture"))
	{
		glActiveTexture_		= (PFNGLACTIVETEXTUREARBPROC)	  getprocaddress("glActiveTextureARB");
		glClientActiveTexture_ = (PFNGLCLIENTACTIVETEXTUREARBPROC)getprocaddress("glClientActiveTextureARB");
		glMultiTexCoord2f_	 = (PFNGLMULTITEXCOORD2FARBPROC)	getprocaddress("glMultiTexCoord2fARB");
		glMultiTexCoord3f_	 = (PFNGLMULTITEXCOORD3FARBPROC)	getprocaddress("glMultiTexCoord3fARB");
		hasMT = true;
	}
	else conoutf("WARNING: No multitexture extension!");

	if(strstr(exts, "GL_ARB_vertex_buffer_object"))
	{
		glGenBuffers_	= (PFNGLGENBUFFERSARBPROC)	getprocaddress("glGenBuffersARB");
		glBindBuffer_	= (PFNGLBINDBUFFERARBPROC)	getprocaddress("glBindBufferARB");
		glMapBuffer_	 = (PFNGLMAPBUFFERARBPROC)	getprocaddress("glMapBufferARB");
		glUnmapBuffer_	= (PFNGLUNMAPBUFFERARBPROC)  getprocaddress("glUnmapBufferARB");
		glBufferData_	= (PFNGLBUFFERDATAARBPROC)	getprocaddress("glBufferDataARB");
		glBufferSubData_ = (PFNGLBUFFERSUBDATAARBPROC)getprocaddress("glBufferSubDataARB");
		glDeleteBuffers_ = (PFNGLDELETEBUFFERSARBPROC)getprocaddress("glDeleteBuffersARB");
		hasVBO = true;
		//conoutf("Using GL_ARB_vertex_buffer_object extension.");
	}
	else conoutf("WARNING: No vertex_buffer_object extension! (geometry heavy maps will be SLOW)");

	if(strstr(exts, "GL_EXT_draw_range_elements"))
	{
		glDrawRangeElements_ = (PFNGLDRAWRANGEELEMENTSEXTPROC)getprocaddress("glDrawRangeElementsEXT");
		hasDRE = true;
	}

	if(strstr(vendor, "ATI"))
	{
		floatvtx = 1;
		conoutf("WARNING: ATI cards may show garbage in skybox. (use \"/ati_skybox_bug 1\" to fix)");

        extern int reservedynlighttc, reserveshadowmaptc;
        reservedynlighttc = 2;
        reserveshadowmaptc = 3;
		minimizetcusage = 1;
        emulatefog = 1;
	}
	else if(strstr(vendor, "Tungsten"))
	{
		floatvtx = 1;
	}
	else if(strstr(vendor, "Intel"))
	{
		intel_quadric_bug = 1;
	} 
	if(floatvtx) conoutf("WARNING: Using floating point vertexes. (use \"/floatvtx 0\" to disable)");

	extern int useshaders;
	if(!useshaders || !hasMT || !strstr(exts, "GL_ARB_vertex_program") || !strstr(exts, "GL_ARB_fragment_program"))
	{
		conoutf("WARNING: No shader support! Using fixed function fallback. (no fancy visuals for you)");
		renderpath = R_FIXEDFUNCTION;
		if(strstr(vendor, "ATI") && !useshaders) ati_texgen_bug = 1;
		else if(strstr(vendor, "NVIDIA")) nvidia_texgen_bug = 1;
		if(ati_texgen_bug) conoutf("WARNING: Using ATI texgen bug workaround. (use \"/ati_texgen_bug 0\" to disable if unnecessary)");
		if(nvidia_texgen_bug) conoutf("WARNING: Using NVIDIA texgen bug workaround. (use \"/nvidia_texgen_bug 0\" to disable if unnecessary)");
	}
	else
	{
		glGenPrograms_ =			(PFNGLGENPROGRAMSARBPROC)			getprocaddress("glGenProgramsARB");
		glDeletePrograms_ =		 (PFNGLDELETEPROGRAMSARBPROC)		getprocaddress("glDeleteProgramsARB");
		glBindProgram_ =			(PFNGLBINDPROGRAMARBPROC)			getprocaddress("glBindProgramARB");
		glProgramString_ =		  (PFNGLPROGRAMSTRINGARBPROC)		 getprocaddress("glProgramStringARB");
        glGetProgramiv_ =           (PFNGLGETPROGRAMIVARBPROC)          getprocaddress("glGetProgramivARB");
		glProgramEnvParameter4f_ =  (PFNGLPROGRAMENVPARAMETER4FARBPROC) getprocaddress("glProgramEnvParameter4fARB");
		glProgramEnvParameter4fv_ = (PFNGLPROGRAMENVPARAMETER4FVARBPROC)getprocaddress("glProgramEnvParameter4fvARB");
		renderpath = R_ASMSHADER;

		if(strstr(exts, "GL_ARB_shading_language_100") && strstr(exts, "GL_ARB_shader_objects") && strstr(exts, "GL_ARB_vertex_shader") && strstr(exts, "GL_ARB_fragment_shader"))
		{
			glCreateProgramObject_ =		(PFNGLCREATEPROGRAMOBJECTARBPROC)	 getprocaddress("glCreateProgramObjectARB");
			glDeleteObject_ =				(PFNGLDELETEOBJECTARBPROC)			getprocaddress("glDeleteObjectARB");
			glUseProgramObject_ =			(PFNGLUSEPROGRAMOBJECTARBPROC)		getprocaddress("glUseProgramObjectARB");
			glCreateShaderObject_ =		 (PFNGLCREATESHADEROBJECTARBPROC)	  getprocaddress("glCreateShaderObjectARB");
			glShaderSource_ =				(PFNGLSHADERSOURCEARBPROC)			getprocaddress("glShaderSourceARB");
			glCompileShader_ =			  (PFNGLCOMPILESHADERARBPROC)			getprocaddress("glCompileShaderARB");
			glGetObjectParameteriv_ =		(PFNGLGETOBJECTPARAMETERIVARBPROC)	getprocaddress("glGetObjectParameterivARB");
			glAttachObject_ =				(PFNGLATTACHOBJECTARBPROC)			getprocaddress("glAttachObjectARB");
			glGetInfoLog_ =				 (PFNGLGETINFOLOGARBPROC)			  getprocaddress("glGetInfoLogARB");
			glLinkProgram_ =				(PFNGLLINKPROGRAMARBPROC)			 getprocaddress("glLinkProgramARB");
			glGetUniformLocation_ =		 (PFNGLGETUNIFORMLOCATIONARBPROC)	  getprocaddress("glGetUniformLocationARB");
			glUniform4fv_ =				 (PFNGLUNIFORM4FVARBPROC)			  getprocaddress("glUniform4fvARB");
			glUniform1i_ =				  (PFNGLUNIFORM1IARBPROC)				getprocaddress("glUniform1iARB");

			extern bool checkglslsupport();			
			if(checkglslsupport())
			{
				renderpath = R_GLSLANG;
				conoutf("Rendering using the OpenGL 1.5 GLSL shader path.");
#ifdef __APPLE__
				apple_glsldepth_bug = 1;
#endif
				if(apple_glsldepth_bug) conoutf("WARNING: Using Apple GLSL depth bug workaround. (use \"/apple_glsldepth_bug 0\" to disable if unnecessary");
			}
		}
		if(renderpath==R_ASMSHADER) conoutf("Rendering using the OpenGL 1.5 assembly shader path.");
	}

	if(strstr(exts, "GL_ARB_occlusion_query"))
	{
		GLint bits;
		glGetQueryiv_ = (PFNGLGETQUERYIVARBPROC)getprocaddress("glGetQueryivARB");
		glGetQueryiv_(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, &bits);
		if(bits)
		{
			glGenQueries_ =		(PFNGLGENQUERIESARBPROC)		getprocaddress("glGenQueriesARB");
			glDeleteQueries_ =	 (PFNGLDELETEQUERIESARBPROC)	getprocaddress("glDeleteQueriesARB");
			glBeginQuery_ =		(PFNGLBEGINQUERYARBPROC)		getprocaddress("glBeginQueryARB");
			glEndQuery_ =		  (PFNGLENDQUERYARBPROC)		 getprocaddress("glEndQueryARB");
			glGetQueryObjectiv_ =  (PFNGLGETQUERYOBJECTIVARBPROC) getprocaddress("glGetQueryObjectivARB");
			glGetQueryObjectuiv_ = (PFNGLGETQUERYOBJECTUIVARBPROC)getprocaddress("glGetQueryObjectuivARB");
			hasOQ = true;
			//conoutf("Using GL_ARB_occlusion_query extension.");
#if defined(__APPLE__) && SDL_BYTEORDER == SDL_BIG_ENDIAN
			if(strstr(vendor, "ATI")) ati_oq_bug = 1; 
#endif			
			if(ati_oq_bug) conoutf("WARNING: Using ATI occlusion query bug workaround. (use \"/ati_oq_bug 0\" to disable if unnecessary)");
		}
	}
	if(!hasOQ)
	{
		conoutf("WARNING: No occlusion query support! (large maps may be SLOW)");
		if(renderpath==R_FIXEDFUNCTION) zpass = 0;
	}

	if(renderpath!=R_FIXEDFUNCTION)
	{
		if(strstr(exts, "GL_ARB_texture_rectangle"))
		{
			hasTR = true;
			//conoutf("Using GL_ARB_texture_rectangle extension.");
		}
		else conoutf("WARNING: No texture rectangle support. (no full screen shaders)");
	}

	if(strstr(exts, "GL_EXT_framebuffer_object"))
	{
		glBindRenderbuffer_		= (PFNGLBINDRENDERBUFFEREXTPROC)		getprocaddress("glBindRenderbufferEXT");
		glDeleteRenderbuffers_	 = (PFNGLDELETERENDERBUFFERSEXTPROC)	getprocaddress("glDeleteRenderbuffersEXT");
		glGenRenderbuffers_		= (PFNGLGENFRAMEBUFFERSEXTPROC)		getprocaddress("glGenRenderbuffersEXT");
		glRenderbufferStorage_	 = (PFNGLRENDERBUFFERSTORAGEEXTPROC)	getprocaddress("glRenderbufferStorageEXT");
		glCheckFramebufferStatus_  = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) getprocaddress("glCheckFramebufferStatusEXT");
		glBindFramebuffer_		 = (PFNGLBINDFRAMEBUFFEREXTPROC)		getprocaddress("glBindFramebufferEXT");
		glDeleteFramebuffers_	  = (PFNGLDELETEFRAMEBUFFERSEXTPROC)	 getprocaddress("glDeleteFramebuffersEXT");
		glGenFramebuffers_		 = (PFNGLGENFRAMEBUFFERSEXTPROC)		getprocaddress("glGenFramebuffersEXT");
		glFramebufferTexture2D_	= (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)	getprocaddress("glFramebufferTexture2DEXT");
		glFramebufferRenderbuffer_ = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)getprocaddress("glFramebufferRenderbufferEXT");
		glGenerateMipmap_		  = (PFNGLGENERATEMIPMAPEXTPROC)		 getprocaddress("glGenerateMipmapEXT");
		hasFBO = true;
		//conoutf("Using GL_EXT_framebuffer_object extension.");
	}
	else conoutf("WARNING: No framebuffer object support. (reflective water may be slow)");

	if(strstr(exts, "GL_EXT_packed_depth_stencil") || strstr(exts, "GL_NV_packed_depth_stencil"))
	{
		hasDS = true;
		//conoutf("Using GL_EXT_packed_depth_stencil extension.");
	}

    if(strstr(exts, "GL_EXT_blend_minmax"))
    {
        glBlendEquation_ = (PFNGLBLENDEQUATIONEXTPROC) getprocaddress("glBlendEquationEXT");
        hasBE = true;
#ifdef __APPLE__
        apple_minmax_bug = 1;
#endif
        //conoutf("Using GL_EXT_blend_minmax extension.");
    }

    if(strstr(exts, "GL_ARB_texture_float") || strstr(exts, "GL_ATI_texture_float"))
    {
        hasTF = true;
        //conoutf("Using GL_ARB_texture_float extension");
        shadowmap = 1;
    }

	if(strstr(exts, "GL_ARB_texture_cube_map"))
	{
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, (GLint *)&hwcubetexsize);
		hasCM = true;
		//conoutf("Using GL_ARB_texture_cube_map extension.");
	}
	else conoutf("WARNING: No cube map texture support. (no reflective glass)");

	if(strstr(exts, "GL_ARB_texture_non_power_of_two")) 
	{
		hasNP2 = true;
		//conoutf("Using GL_ARB_texture_non_power_of_two extension.");
	}
	else conoutf("WARNING: Non-power-of-two textures not supported!");

	if(strstr(exts, "GL_EXT_texture_compression_s3tc"))
	{
		hasTC = true;
		//conoutf("Using GL_EXT_texture_compression_s3tc extension.");
	}

	if(fsaa) glEnable(GL_MULTISAMPLE);

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *)&hwtexsize);

	inittmus();
}

VAR(wireframe, 0, 0, 1);

vec worldpos, camright, camup;

void findorientation()
{
#ifdef BFRONTIER // gamme view control
	cl->findorientation();
#else
	vec dir;
	vecfromyawpitch(camera1->yaw, camera1->pitch, 1, 0, dir);
	vecfromyawpitch(camera1->yaw, 0, 0, -1, camright);
	vecfromyawpitch(camera1->yaw, camera1->pitch+90, 1, 0, camup);

	if(raycubepos(camera1->o, dir, worldpos, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
		worldpos = dir.mul(10).add(camera1->o); //otherwise 3dgui won't work when outside of map
#endif
}

void transplayer()
{
	glLoadIdentity();

	glRotatef(camera1->roll, 0, 0, 1);
	glRotatef(camera1->pitch, -1, 0, 0);
	glRotatef(camera1->yaw, 0, 1, 0);

	// move from RH to Z-up LH quake style worldspace
	glRotatef(-90, 1, 0, 0);
	glScalef(1, -1, 1);

	glTranslatef(-camera1->o.x, -camera1->o.y, -camera1->o.z);	
}

#ifdef BFRONTIER // game view control
VARFP(fov, 1, 120, 360, cl->fixview());
#else
VARP(fov, 10, 105, 150);
#endif

int xtraverts, xtravertsva;

#ifdef BFRONTIER
VARW(fog, 16, 4000, INT_MAX-1);
VARW(fogcolour, 0, 0x8099B3, 0xFFFFFF);
#else
VAR(fog, 16, 4000, 1000024);
VAR(fogcolour, 0, 0x8099B3, 0xFFFFFF);
#endif

#ifdef BFRONTIER // game view control, extra thirdperson variables
VARFP(thirdperson, 0, 0, 1, cl->fixview());
VARFP(thirdpersonscale, 0, 150, INT_MAX-1, cl->fixview()); // pitch scale
#else
VAR(thirdperson, 0, 0, 1);
VAR(thirdpersondistance, 10, 50, 1000);
#endif
physent *camera1 = NULL;
bool deathcam = false;
#ifdef BFRONTIER // game view control, extra thirdperson support
bool isthirdperson() { return cl->gamethirdperson() || (reflecting && !refracting); }
#else
bool isthirdperson() { return player!=camera1 || player->state==CS_DEAD || (reflecting && !refracting); }
#endif
void recomputecamera()
{
#ifdef BFRONTIER // game view control
	cl->recomputecamera();
#else
    cl->setupcamera();
	if(deathcam && player->state!=CS_DEAD) deathcam = false;
	extern int testanims;
	if(((editmode && !testanims) || !thirdperson) && player->state!=CS_DEAD)
	{
		//if(camera1->state==CS_DEAD) camera1->o.z -= camera1->eyeheight-0.8f;
		camera1 = player;
	}
	else
	{
		static physent tempcamera;
		camera1 = &tempcamera;
		if(deathcam) camera1->o = player->o;
		else
		{
			*camera1 = *player;
			if(player->state==CS_DEAD) deathcam = true;
		}
		camera1->reset();
		camera1->type = ENT_CAMERA;
		camera1->move = -1;
		camera1->eyeheight = 2;
		
		loopi(10)
		{
			if(!moveplayer(camera1, 10, true, thirdpersondistance)) break;
		}
	}
#endif
}

#ifdef BFRONTIER // redefined for extern in iengine.h
void project(float fovy, float aspect, int farplane, bool flipx, bool flipy)
#else
void project(float fovy, float aspect, int farplane, bool flipx = false, bool flipy = false)
#endif
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if(flipx || flipy) glScalef(flipx ? -1 : 1, flipy ? -1 : 1, 1);
	gluPerspective(fovy, aspect, 0.54f, farplane);
	glMatrixMode(GL_MODELVIEW);
}

void genclipmatrix(float a, float b, float c, float d, GLfloat matrix[16])
{
	// transform the clip plane into camera space
	GLdouble clip[4] = {a, b, c, d};
	glClipPlane(GL_CLIP_PLANE0, clip);
	glGetClipPlane(GL_CLIP_PLANE0, clip);

	glGetFloatv(GL_PROJECTION_MATRIX, matrix);
	float x = ((clip[0]<0 ? -1 : (clip[0]>0 ? 1 : 0)) + matrix[8]) / matrix[0],
		  y = ((clip[1]<0 ? -1 : (clip[1]>0 ? 1 : 0)) + matrix[9]) / matrix[5],
		  w = (1 + matrix[10]) / matrix[14], 
		  scale = 2 / (x*clip[0] + y*clip[1] - clip[2] + w*clip[3]);
	matrix[2] = clip[0]*scale;
	matrix[6] = clip[1]*scale; 
	matrix[10] = clip[2]*scale + 1.0f;
	matrix[14] = clip[3]*scale;
}

void setclipmatrix(GLfloat matrix[16])
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf(matrix);
	glMatrixMode(GL_MODELVIEW);
}

void undoclipmatrix()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

VAR(reflectclip, 0, 6, 64);
VARP(reflectmms, 0, 1, 1);

void setfogplane(const plane &p, bool flush)
{
	static float fogselect[4] = {0, 0, 0, 0};
	setenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
	setenvparamfv("fogplane", SHPARAM_VERTEX, 9, p.v);
	if(flush)
	{
		flushenvparam(SHPARAM_VERTEX, 8);
		flushenvparam(SHPARAM_VERTEX, 9);
	}
}

void setfogplane(float scale, float z, bool flush)
{
	float fogselect[4] = {1, 1, 1, 1}, fogplane[4] = {0, 0, 0, 0};
	if(scale || z)
	{
		loopk(4) fogselect[k] = 0;

		fogplane[2] = scale;
		fogplane[3] = -z;
	}  
	setenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
	setenvparamfv("fogplane", SHPARAM_VERTEX, 9, fogplane);
	if(flush)
	{
		flushenvparam(SHPARAM_VERTEX, 8);
		flushenvparam(SHPARAM_VERTEX, 9);
	}
}

void drawreflection(float z, bool refract, bool clear)
{
	uchar wcol[3];
	getwatercolour(wcol);
	float fogc[4] = { wcol[0]/256.0f, wcol[1]/256.0f, wcol[2]/256.0f, 1.0f };
	
	if(refract && !waterfog)
	{
		glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

	reflecting = z;
	if(refract) refracting = z;

	float oldfogstart, oldfogend, oldfogcolor[4];
    if(renderpath==R_FIXEDFUNCTION && refract && refractfog) glDisable(GL_FOG);
    else if((renderpath!=R_FIXEDFUNCTION && refract) || camera1->o.z < z)
	{
		glGetFloatv(GL_FOG_START, &oldfogstart);
		glGetFloatv(GL_FOG_END, &oldfogend);
		glGetFloatv(GL_FOG_COLOR, oldfogcolor);

		if(refract)
		{
			glFogi(GL_FOG_START, 0);
			glFogi(GL_FOG_END, waterfog);
			glFogfv(GL_FOG_COLOR, fogc);
		}
		else
		{
			glFogi(GL_FOG_START, (fog+64)/8);
			glFogi(GL_FOG_END, fog);
			float fogc[4] = { (fogcolour>>16)/256.0f, ((fogcolour>>8)&255)/256.0f, (fogcolour&255)/256.0f, 1.0f };
			glFogfv(GL_FOG_COLOR, fogc);
		}
	}

	if(clear)
	{
		glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	if(!refract && camera1->o.z >= z)
	{
		glPushMatrix();
		glTranslatef(0, 0, 2*z);
		glScalef(1, 1, -1);

		glCullFace(GL_BACK);
	}

	int farplane = max(max(fog*2, 384), hdr.worldsize*2);
	//if(!refract && explicitsky) drawskybox(farplane, true, z);

	GLfloat clipmatrix[16];
	if(reflectclip)
	{
		float zoffset = reflectclip/4.0f, zclip;
		if(refract)
		{
			zclip = z+zoffset;
			if(camera1->o.z<=zclip) zclip = z;
		}
		else
		{
			zclip = z-zoffset;
			if(camera1->o.z>=zclip && camera1->o.z<=z+4.0f) zclip = z;
		}
		genclipmatrix(0, 0, refract ? -1 : 1, refract ? zclip : -zclip, clipmatrix);
		setclipmatrix(clipmatrix);
	}

	//if(!refract && explicitsky) drawskylimits(true, z);

    renderreflectedgeom(z, refract, caustics && refract, (renderpath!=R_FIXEDFUNCTION || refractfog) && refract);

    if(refract && renderpath!=R_FIXEDFUNCTION) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

	if(reflectmms) renderreflectedmapmodels(z, refract);
	cl->rendergame();

	if(!refract /*&& !explicitsky*/) 
	{
		if(reflectclip) undoclipmatrix();
		defaultshader->set();
		drawskybox(farplane, false, z);
		if(reflectclip) setclipmatrix(clipmatrix);
	}

	setfogplane(1, z);
	if(refract) rendergrass();
	rendermaterials(z, refract);
	render_particles(0);

    if(refract && renderpath!=R_FIXEDFUNCTION) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	setfogplane();

	if(reflectclip) undoclipmatrix();

	if(!refract && camera1->o.z >= z)
	{
		glPopMatrix();

		glCullFace(GL_FRONT);
	}

    if(renderpath==R_FIXEDFUNCTION && refract && refractfog) glEnable(GL_FOG);
    else if((renderpath!=R_FIXEDFUNCTION && refract) || camera1->o.z < z)
	{
		glFogf(GL_FOG_START, oldfogstart);
		glFogf(GL_FOG_END, oldfogend);
		glFogfv(GL_FOG_COLOR, oldfogcolor);
	}
	
	refracting = 0;
	reflecting = 0;
}

static void setfog(int fogmat)
{
	glFogi(GL_FOG_START, (fog+64)/8);
	glFogi(GL_FOG_END, fog);
	float fogc[4] = { (fogcolour>>16)/256.0f, ((fogcolour>>8)&255)/256.0f, (fogcolour&255)/256.0f, 1.0f };
	glFogfv(GL_FOG_COLOR, fogc);
	glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);
	
	if(fogmat==MAT_WATER || fogmat==MAT_LAVA)
	{
		uchar col[3];
		if(fogmat==MAT_WATER) getwatercolour(col);
		else getlavacolour(col);
		float fogwc[4] = { col[0]/256.0f, col[1]/256.0f, col[2]/256.0f, 1 };
		glFogfv(GL_FOG_COLOR, fogwc); 
		glFogi(GL_FOG_START, 0);
		glFogi(GL_FOG_END, min(fog, max((fogmat==MAT_WATER ? waterfog : lavafog)*4, 32)));//(fog+96)/8);
	}

	if(renderpath!=R_FIXEDFUNCTION) setfogplane();
}

bool envmapping = false;

void drawcubemap(int size, const vec &o, float yaw, float pitch)
{
    envmapping = true;

	physent *oldcamera = camera1;
	static physent cmcamera;
	cmcamera = *player;
	cmcamera.reset();
	cmcamera.type = ENT_CAMERA;
	cmcamera.o = o;
	cmcamera.yaw = yaw;
	cmcamera.pitch = pitch;
	cmcamera.roll = 0;
	camera1 = &cmcamera;
	
	defaultshader->set();

	cube &c = lookupcube(int(o.x), int(o.y), int(o.z));
	int fogmat = c.ext ? c.ext->material : MAT_AIR;
	if(fogmat!=MAT_WATER && fogmat!=MAT_LAVA) fogmat = MAT_AIR;

	setfog(fogmat);

	glClear(GL_DEPTH_BUFFER_BIT);

	int farplane = max(max(fog*2, 384), hdr.worldsize*2);

	project(90.0f, 1.0f, farplane, true, true);

	transplayer();

	glEnable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	xtravertsva = xtraverts = glde = 0;

	visiblecubes(worldroot, hdr.worldsize/2, 0, 0, 0, size, size, 90);

	if(limitsky()) drawskybox(farplane, true);

	rendergeom();

//	queryreflections();

	rendermapmodels();

	defaultshader->set();

	if(!limitsky()) drawskybox(farplane, false);

//	drawreflections();

//	renderwater();
//	rendermaterials();

	glDisable(GL_TEXTURE_2D);

	camera1 = oldcamera;
    envmapping = false;
}

VAR(hudgunfov, 10, 65, 150);

void gl_drawhud(int w, int h, int fogmat);

void gl_drawframe(int w, int h)
{
#ifdef BFRONTIER
	if (cc->ready())
	{
#endif
		defaultshader->set();
	
		recomputecamera();
		
		cleardynlights();
		cl->adddynlights();
	
		float fovy = (float)fov*h/w;
		float aspect = w/(float)h;
		cube &c = lookupcube((int)camera1->o.x, (int)camera1->o.y, int(camera1->o.z + camera1->aboveeye*0.5f));
		int fogmat = c.ext ? c.ext->material : MAT_AIR;
		if(fogmat!=MAT_WATER && fogmat!=MAT_LAVA) fogmat = MAT_AIR;
	
		setfog(fogmat);
		if(fogmat!=MAT_AIR)
		{
			fovy += (float)sin(lastmillis/1000.0)*2.0f;
			aspect += (float)sin(lastmillis/1000.0+PI)*0.1f;
		}
	
		int farplane = max(max(fog*2, 384), hdr.worldsize*2);
	
		project(fovy, aspect, farplane);
	
		transplayer();
	
		glEnable(GL_TEXTURE_2D);
	
		glPolygonMode(GL_FRONT_AND_BACK, wireframe && editmode ? GL_LINE : GL_FILL);
		
		xtravertsva = xtraverts = glde = 0;
	
		if(!hasFBO) drawreflections();
	
		visiblecubes(worldroot, hdr.worldsize/2, 0, 0, 0, w, h, fov);
		
		extern GLuint shadowmapfb;
		if(shadowmap && !shadowmapfb) rendershadowmap();
	
		glClear(GL_DEPTH_BUFFER_BIT|(wireframe && editmode ? GL_COLOR_BUFFER_BIT : 0)|(hasstencil ? GL_STENCIL_BUFFER_BIT : 0));
	
		if(limitsky()) drawskybox(farplane, true);
	
 	   bool causticspass = false;
 	   if(caustics && fogmat==MAT_WATER)
 	   {
 	       cube &s = lookupcube((int)camera1->o.x, (int)camera1->o.y, int(camera1->o.z + camera1->aboveeye*1.25f));
 	       if(s.ext && s.ext->material==MAT_WATER) causticspass = true;
 	   }
    	rendergeom(causticspass);
	
		queryreflections();
	
		if(!wireframe) renderoutline();
	
		rendermapmodels();
	
		if(!waterrefract) 
		{
			defaultshader->set();
			cl->rendergame();
		}
	
		defaultshader->set();
	
		if(!limitsky()) drawskybox(farplane, false);
	
		if(hasFBO) drawreflections();
	
		if(waterrefract) 
		{
			defaultshader->set();
			cl->rendergame();
		}
	
		renderwater();
		rendergrass();
	
		rendermaterials();
		render_particles(curtime);
	
		if(!isthirdperson()) 
		{
			project(hudgunfov, aspect, farplane);
			cl->drawhudgun();
			project(fovy, aspect, farplane);
		}
	
		glDisable(GL_FOG);
		glDisable(GL_CULL_FACE);
	
		renderfullscreenshader(w, h);
		
#ifndef BFRONTIER
		defaultshader->set();
		g3d_render();
#endif

		glDisable(GL_TEXTURE_2D);
		notextureshader->set();
	
		gl_drawhud(w, h, fogmat);
	
		glEnable(GL_CULL_FACE);
		glEnable(GL_FOG);
#ifdef BFRONTIER
	}
	else
	{
		float fovy = (float)fov*h/w;
		float aspect = w/(float)h;
		project(fovy, aspect, hdr.worldsize*2);
		transplayer();
	
		glEnable(GL_TEXTURE_2D);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		
		xtravertsva = xtraverts = glde = 0;
	
		glClearColor(0.f, 0.f, 0.f, 1);
		glClear(GL_DEPTH_BUFFER_BIT|(wireframe && editmode ? GL_COLOR_BUFFER_BIT : 0)|(hasstencil ? GL_STENCIL_BUFFER_BIT : 0));

		glDisable(GL_FOG);
		glDisable(GL_CULL_FACE);
	
		gl_drawhud(w, h, MAT_AIR);
		glEnable(GL_CULL_FACE);
		glEnable(GL_FOG);
	}
#endif
}

#ifdef BFRONTIER // better crosshair support
VARP(crosshairsize, 0, 15, 1000);
VARP(cursorsize, 0, 30, 1000);
#else
VARP(crosshairsize, 0, 15, 50);
VARP(cursorsize, 0, 30, 50);
VARP(damageblendfactor, 0, 300, 1000);

int dblend = 0;
void damageblend(int n) { dblend += n; }
#endif

VARP(hidestats, 0, 0, 1);
VARP(hidehud, 0, 0, 1);
VARP(crosshairfx, 0, 1, 1);

static Texture *crosshair = NULL;

void loadcrosshair(const char *name)
{
    crosshair = textureload(name, 3, true);
#ifdef BFRONTIER // moved data
    if(crosshair==notexture) crosshair = textureload("packages/textures/crosshair.png", 3, true);
#else
    if(crosshair==notexture) crosshair = textureload("data/crosshair.png", 3, true);
#endif
}

COMMAND(loadcrosshair, "s");

void writecrosshairs(FILE *f)
{
    fprintf(f, "loadcrosshair \"%s\"\n", crosshair->name);
    fprintf(f, "\n");
}

void drawcrosshair(int w, int h)
{
	bool windowhit = g3d_windowhit(true, false);
#ifdef BFRONTIER // game crosshair control
	if(!windowhit && !cl->wantcrosshair()) return;
#else
    if(!windowhit && (hidehud || player->state==CS_SPECTATOR)) return;
#endif

	static Texture *cursor = NULL;
#ifdef BFRONTIER // moved data
    if(!cursor) cursor = textureload("packages/textures/guicursor.png", 3, true);
    if(!crosshair) crosshair = textureload("packages/textures/crosshair.png", 3, true);
#else
    if(!cursor) cursor = textureload("data/guicursor.png", 3, true);
    if(!crosshair) crosshair = textureload("data/crosshair.png", 3, true);
#endif
	if((windowhit ? cursor : crosshair)->bpp==32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	else glBlendFunc(GL_ONE, GL_ONE);
	float r = 1, g = 1, b = 1;
	if(!windowhit && crosshairfx) cl->crosshaircolor(r, g, b);
	glColor3f(r, g, b);
	float chsize = (windowhit ? cursorsize : crosshairsize)*w/300.0f;
	float cx = 0.5f, cy = 0.5f;
	if(windowhit) g3d_cursorpos(cx, cy);
	float x = cx*w*3.0f - (windowhit ? 0 : chsize/2.0f);
	float y = cy*h*3.0f - (windowhit ? 0 : chsize/2.0f);
	glBindTexture(GL_TEXTURE_2D, (windowhit ? cursor : crosshair)->gl);
	glBegin(GL_QUADS);
	glTexCoord2d(0.0, 0.0); glVertex2f(x,		  y);
	glTexCoord2d(1.0, 0.0); glVertex2f(x + chsize, y);
	glTexCoord2d(1.0, 1.0); glVertex2f(x + chsize, y + chsize);
	glTexCoord2d(0.0, 1.0); glVertex2f(x,		  y + chsize);
	glEnd();
}

VARP(showfpsrange, 0, 0, 1);

void gl_drawhud(int w, int h, int fogmat)
{
	if(editmode && !hidehud)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glDepthMask(GL_FALSE);
		cursorupdate();
		glDepthMask(GL_TRUE);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gettextres(w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

    glColor3f(1, 1, 1);

    extern int debugsm;
    if(debugsm)
    {
        extern void viewshadowmap();
        viewshadowmap();
    }

    glEnable(GL_BLEND);

#ifdef BFRONTIER // game hud colour control
	vec colour;
	if(cc->ready() && cl->gethudcolour(colour))
	{
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		glBegin(GL_QUADS);
		glColor3f(colour.x, colour.y, colour.z);
		glVertex2i(0, 0);
		glVertex2i(w, 0);
		glVertex2i(w, h);
		glVertex2i(0, h);
		glEnd();
		glDepthMask(GL_TRUE);
	}

	glEnable(GL_TEXTURE_2D);
	defaultshader->set();

	glDisable(GL_BLEND);
	cl->gameplayhud(w, h); // can make more dramatic changes this way without getting in the way
	g3d_render();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glEnable(GL_BLEND);
#else
	if(dblend || fogmat==MAT_WATER || fogmat==MAT_LAVA)
	{
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		glBegin(GL_QUADS);
		if(dblend) glColor3f(1.0f, 0.1f, 0.1f);
		else
		{
			uchar col[3];
			if(fogmat==MAT_WATER) getwatercolour(col);
			else getlavacolour(col);
			float maxc = max(col[0], max(col[1], col[2]));
			float blend[3];
			loopi(3) blend[i] = col[i] / min(32 + maxc*7/8, 255);
			glColor3fv(blend);
			//glColor3f(0.1f, 0.5f, 1.0f);
		}
		glVertex2i(0, 0);
		glVertex2i(w, 0);
		glVertex2i(w, h);
		glVertex2i(0, h);
		glEnd();
		glDepthMask(GL_TRUE);
		dblend -= curtime*100/damageblendfactor;
		if(dblend<0) dblend = 0;
	}

	glEnable(GL_TEXTURE_2D);
	defaultshader->set();
#endif

	glLoadIdentity();
	glOrtho(0, w*3, h*3, 0, -1, 1);

	int abovegameplayhud = h*3*1650/1800-FONTH*3/2; // hack
	int hoff = abovegameplayhud - (editmode ? FONTH*4 : 0);

	char *command = getcurcommand();
	if(command) rendercommand(FONTH/2, hoff); else hoff += FONTH;

	drawcrosshair(w, h);

	if(!hidehud)
	{
        /*int coff = */ renderconsole(w, h);
		// can render stuff below console output here		

		if(!hidestats)
		{
			extern void getfps(int &fps, int &bestdiff, int &worstdiff);
			int fps, bestdiff, worstdiff;
			getfps(fps, bestdiff, worstdiff);
#ifdef BFRONTIER // extended performace stats
			if(showfpsrange) draw_textx("%d+%d-%d:%d", w*3-4, 4, 255, 255, 255, 255, false, AL_RIGHT, fps, bestdiff, worstdiff, perf);
			else draw_textx("%d:%d", w*3-6, 4, 255, 255, 255, 255, false, AL_RIGHT, fps, perf);
#else
			if(showfpsrange) draw_textf("fps %d+%d-%d", w*3-7*FONTH, h*3-100, fps, bestdiff, worstdiff);
			else draw_textf("fps %d", w*3-5*FONTH, h*3-100, fps);
#endif

			if(editmode)
			{
				draw_textf("cube %s%d", FONTH/2, abovegameplayhud-FONTH*3, selchildcount<0 ? "1/" : "", abs(selchildcount));
				draw_textf("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", FONTH/2, abovegameplayhud-FONTH*2, wtris/1024, vtris*100/max(wtris, 1), wverts/1024, vverts*100/max(wverts, 1), xtraverts/1024, xtravertsva/1024);
				draw_textf("ond:%d va:%d gl:%d oq:%d lm:%d, rp:%d", FONTH/2, abovegameplayhud-FONTH, allocnodes*8, allocva, glde, getnumqueries(), lightmaps.length(), rplanes);
			}
		}

		if(editmode) 
		{
			char *editinfo = executeret("edithud");
			if(editinfo)
			{
				draw_text(editinfo, FONTH/2, abovegameplayhud);
				DELETEA(editinfo);
			}
		}

#ifndef BFRONTIER
		cl->gameplayhud(w, h);
#endif
		render_texture_panel(w, h);
	}

	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
}

#ifdef BFRONTIER // blending, entity directions, and other useful functions for primitives
VARP(hudblend, 0, 60, 100);
VARP(showentdir, 0, 1, 1);

#define rendernearfar(a,b,c,d,e) \
	if (d) { \
		loopj(2) { \
			if (!j) { glDepthFunc(GL_GREATER); glColor3f(a*0.25f, b*0.25f, c*0.25f); } \
			else { glDepthFunc(GL_LESS); glColor3f(a, b, c); } \
			e; \
		} \
	} else { \
		glColor3f(a, b, c); \
		e; \
	}


void renderprimitive(bool on)
{
	if (on)
	{
		notextureshader->set();
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	}
	else
	{
		defaultshader->set();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_CULL_FACE);
		glDisable(GL_BLEND);
	}
}

void renderline(vec &fr, vec &to, float r, float g, float b, bool nf)
{
	rendernearfar(r, g, b, nf,
	{
		glBegin(GL_LINES);
		glVertex3f(fr.x, fr.y, fr.z);
		glVertex3f(to.x, to.y, to.z);
		glEnd();
		xtraverts += 2;
	});
}

void rendertris(vec &fr, float yaw, float pitch, float size, float r, float g, float b, bool fill, bool nf)
{
	rendernearfar(r, g, b, nf,
	{
		vec to;
		float ty;
	
		glBegin(GL_TRIANGLES);
		glPolygonMode(GL_FRONT_AND_BACK, fill ? GL_FILL : GL_LINE);
	
		glVertex3f(fr.x, fr.y, fr.z);
	
		ty = yaw - 45.f;
		if (ty < 0.f) ty += 360.f;
		else if (ty > 360.f) ty -= 360.f;
	
		vecfromyawpitch(ty, pitch, -1, 0, to);
		to.mul(size);
		to.add(fr);
		glVertex3f(to.x, to.y, to.z);
	
		ty = yaw + 45.f;
		if (ty < 0.f) ty += 360.f;
		else if (ty > 360.f) ty -= 360.f;
	
		vecfromyawpitch(ty, pitch, -1, 0, to);
		to.mul(size);
		to.add(fr);
		glVertex3f(to.x, to.y, to.z);
	
		glEnd();
		xtraverts += 3;
	});
}

void renderlineloop(vec &o, float height, float xradius, float yradius, float z, int type, float r, float g, float b, bool nf)
{
	rendernearfar(r, g, b, nf,
	{
		glBegin(GL_LINE_LOOP);
		loopi(16)
		{
			vec p;
			switch (type)
			{
				case 0:
					p = vec(xradius*cosf(2*M_PI*i/16.0f), height*sinf(2*M_PI*i/16.0f), 0);
					p.rotate_around_x((z+90)*RAD);
					break;
				case 1:
					p = vec(height*cosf(2*M_PI*i/16.0f), yradius*sinf(2*M_PI*i/16.0f), 0);
					p.rotate_around_y((z+90)*RAD);
					break;
				case 2:
				default:
					p = vec(xradius*cosf(2*M_PI*i/16.0f), yradius*sinf(2*M_PI*i/16.0f), 0);
					p.rotate_around_z((z+90)*RAD);
					break;
			}
			p.add(o);
			glVertex3fv(p.v);
		}
		xtraverts += 16;
		glEnd();
	});
}

void renderentdir(vec &o, float yaw, float pitch, bool nf)
{
	vec fr = o, to, dr;

	vecfromyawpitch(yaw, pitch, 1, 0, dr);

	to = dr;
	to.mul(RENDERPUSHX);
	to.add(fr);
	fr.z += RENDERPUSHZ;
	to.z += RENDERPUSHZ;

	renderline(fr, to, 0.f, 0.f, 1.f, nf);

	dr.mul(0.5f);
	to.add(dr);
	rendertris(to, yaw, pitch, 2.f, 0.f, 0.f, 1.f, true, nf);
}

void renderentradius(vec &o, float height, float radius, bool nf)
{
	renderlineloop(o, height, radius, radius, 0.f, 0, 0.f, 1.f, 1.f, nf);
	renderlineloop(o, height, radius, radius, 0.f, 1, 0.f, 1.f, 1.f, nf);
	renderlineloop(o, height, radius, radius, 0.f, 2, 0.f, 1.f, 1.f, nf);
}

bool rendericon(const char *icon, int x, int y, int xs, int ys)
{
	Texture *t;
	
	if ((t = textureload(icon, 0, true, false)) != notexture)
	{
		glBindTexture(GL_TEXTURE_2D, t->gl);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(x,	y);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(x+xs, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(x+xs, y+ys);
		glTexCoord2f(0.0f, 1.0f); glVertex2i(x,	y+ys);
		glEnd();
		return true;
	}
	return false;
}

extern int scr_w, scr_h, fov;

bool getlos(vec &o, vec &q, float yaw, float pitch, float mdist, float fx, float fy)
{
	float dist = o.dist(q);

	if (mdist <= 0.f || dist <= mdist)
	{
		float fovx = fx, fovy = fy;

		if (fovx <= 0.f) fovx = (float)fov;
		if (fovy <= 0.f) fovy = (float)fov*scr_h/scr_w;

		float x = fabs((asin((q.z - o.z) / dist) / RAD) - pitch);
		float y = fabs((-(float)atan2(q.x - o.x, q.y - o.y)/PI*180+180) - yaw);
		return x <= fovx && y <= fovy;
	}
	return false;
}

bool getsight(physent *d, vec &q, vec &v, float mdist, float fx, float fy)
{
	if (getlos(d->o, q, d->yaw, d->pitch, mdist, fx, fy)) return raycubelos(d->o, q, v);
	return false;
}
#endif
