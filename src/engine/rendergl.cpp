// rendergl.cpp: core opengl rendering stuff

#include "pch.h"
#include "engine.h"

bool hasVBO = false, hasDRE = false, hasOQ = false, hasTR = false, hasFBO = false, hasDS = false, hasTF = false, hasBE = false, hasCM = false, hasNP2 = false, hasTC = false, hasTE = false, hasMT = false, hasD3 = false, hasstencil = false, hasAF = false, hasVP2 = false, hasVP3 = false, hasPP = false, hasMDA = false, hasTE3 = false, hasTE4 = false, hasVP = false, hasFP = false, hasGLSL = false, hasGM = false, hasNVFB = false;

VAR(renderpath, 1, 0, 0);

// GL_ARB_vertex_buffer_object
PFNGLGENBUFFERSARBPROC       glGenBuffers_       = NULL;
PFNGLBINDBUFFERARBPROC       glBindBuffer_       = NULL;
PFNGLMAPBUFFERARBPROC        glMapBuffer_        = NULL;
PFNGLUNMAPBUFFERARBPROC      glUnmapBuffer_      = NULL;
PFNGLBUFFERDATAARBPROC       glBufferData_       = NULL;
PFNGLBUFFERSUBDATAARBPROC    glBufferSubData_    = NULL;
PFNGLDELETEBUFFERSARBPROC    glDeleteBuffers_    = NULL;
PFNGLGETBUFFERSUBDATAARBPROC glGetBufferSubData_ = NULL;

// GL_ARB_multitexture
PFNGLACTIVETEXTUREARBPROC		glActiveTexture_		= NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTexture_ = NULL;
PFNGLMULTITEXCOORD2FARBPROC	 glMultiTexCoord2f_	 = NULL;
PFNGLMULTITEXCOORD3FARBPROC	 glMultiTexCoord3f_	 = NULL;
PFNGLMULTITEXCOORD4FARBPROC  glMultiTexCoord4f_     = NULL;

// GL_ARB_vertex_program, GL_ARB_fragment_program
PFNGLGENPROGRAMSARBPROC			glGenPrograms_			= NULL;
PFNGLDELETEPROGRAMSARBPROC		 glDeletePrograms_		 = NULL;
PFNGLBINDPROGRAMARBPROC			glBindProgram_			= NULL;
PFNGLPROGRAMSTRINGARBPROC		  glProgramString_		  = NULL;
PFNGLGETPROGRAMIVARBPROC           glGetProgramiv_           = NULL;
PFNGLPROGRAMENVPARAMETER4FARBPROC  glProgramEnvParameter4f_  = NULL;
PFNGLPROGRAMENVPARAMETER4FVARBPROC glProgramEnvParameter4fv_ = NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC  glEnableVertexAttribArray_  = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArray_ = NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC      glVertexAttribPointer_      = NULL;

// GL_EXT_gpu_program_parameters
PFNGLPROGRAMENVPARAMETERS4FVEXTPROC   glProgramEnvParameters4fv_   = NULL;
PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC glProgramLocalParameters4fv_ = NULL;

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

// GL_EXT_multi_draw_arrays
PFNGLMULTIDRAWARRAYSEXTPROC   glMultiDrawArrays_ = NULL;
PFNGLMULTIDRAWELEMENTSEXTPROC glMultiDrawElements_ = NULL;

void *getprocaddress(const char *name)
{
	return SDL_GL_GetProcAddress(name);
}

VARP(ati_skybox_bug, 0, 0, 1);
VAR(ati_texgen_bug, 0, 0, 1);
VAR(ati_oq_bug, 0, 0, 1);
VAR(ati_minmax_bug, 0, 0, 1);
VAR(ati_dph_bug, 0, 0, 1);
VAR(nvidia_texgen_bug, 0, 0, 1);
VAR(nvidia_scissor_bug, 0, 0, 1);
VAR(apple_glsldepth_bug, 0, 0, 1);
VAR(apple_ff_bug, 0, 0, 1);
VAR(apple_vp_bug, 0, 0, 1);
VAR(intel_quadric_bug, 0, 0, 1);
VAR(mesa_program_bug, 0, 0, 1);
VAR(avoidshaders, 1, 0, 0);
VAR(minimizetcusage, 1, 0, 0);
VAR(emulatefog, 1, 0, 0);
VAR(usevp2, 1, 0, 0);
VAR(usevp3, 1, 0, 0);
VAR(usetexrect, 1, 0, 0);
VAR(rtscissor, 0, 1, 1);
VAR(blurtile, 0, 1, 1);
VAR(rtsharefb, 0, 1, 1);

static bool checkseries(const char *s, int low, int high)
{
    while(*s && !isdigit(*s)) ++s;
    if(!*s) return false;
    int n = 0;
    while(isdigit(*s)) n = n*10 + (*s++ - '0');
    return n >= low && n < high;
}

void gl_checkextensions()
{
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    const char *exts = (const char *)glGetString(GL_EXTENSIONS);
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    const char *version = (const char *)glGetString(GL_VERSION);
    conoutf("Renderer: %s (%s)", renderer, vendor);
    conoutf("Driver: %s", version);

#ifdef __APPLE__
    extern int mac_osversion();
    int osversion = mac_osversion();  /* 0x1050 = 10.5 (Leopard) */
#endif

    //extern int shaderprecision;
    // default to low precision shaders on certain cards, can be overridden with -f3
    // char *weakcards[] = { "GeForce FX", "Quadro FX", "6200", "9500", "9550", "9600", "9700", "9800", "X300", "X600", "FireGL", "Intel", "Chrome", NULL }
    // if(shaderprecision==2) for(char **wc = weakcards; *wc; wc++) if(strstr(renderer, *wc)) shaderprecision = 1;

    if(strstr(exts, "GL_EXT_texture_env_combine") || strstr(exts, "GL_ARB_texture_env_combine"))
    {
        hasTE = true;
        if(strstr(exts, "GL_ATI_texture_env_combine3")) hasTE3 = true;
        if(strstr(exts, "GL_NV_texture_env_combine4")) hasTE4 = true;
        if(strstr(exts, "GL_EXT_texture_env_dot3") || strstr(exts, "GL_ARB_texture_env_dot3")) hasD3 = true;
    }
    else conoutf("WARNING: No texture_env_combine extension! (your video card is WAY too old)");

    if(strstr(exts, "GL_ARB_multitexture"))
    {
        glActiveTexture_       = (PFNGLACTIVETEXTUREARBPROC)      getprocaddress("glActiveTextureARB");
        glClientActiveTexture_ = (PFNGLCLIENTACTIVETEXTUREARBPROC)getprocaddress("glClientActiveTextureARB");
        glMultiTexCoord2f_     = (PFNGLMULTITEXCOORD2FARBPROC)    getprocaddress("glMultiTexCoord2fARB");
        glMultiTexCoord3f_     = (PFNGLMULTITEXCOORD3FARBPROC)    getprocaddress("glMultiTexCoord3fARB");
        glMultiTexCoord4f_     = (PFNGLMULTITEXCOORD4FARBPROC)    getprocaddress("glMultiTexCoord4fARB");
        hasMT = true;
    }
    else conoutf("WARNING: No multitexture extension!");

    if(strstr(exts, "GL_ARB_vertex_buffer_object"))
    {
        glGenBuffers_       = (PFNGLGENBUFFERSARBPROC)      getprocaddress("glGenBuffersARB");
        glBindBuffer_       = (PFNGLBINDBUFFERARBPROC)      getprocaddress("glBindBufferARB");
        glMapBuffer_        = (PFNGLMAPBUFFERARBPROC)       getprocaddress("glMapBufferARB");
        glUnmapBuffer_      = (PFNGLUNMAPBUFFERARBPROC)     getprocaddress("glUnmapBufferARB");
        glBufferData_       = (PFNGLBUFFERDATAARBPROC)      getprocaddress("glBufferDataARB");
        glBufferSubData_    = (PFNGLBUFFERSUBDATAARBPROC)   getprocaddress("glBufferSubDataARB");
        glDeleteBuffers_    = (PFNGLDELETEBUFFERSARBPROC)   getprocaddress("glDeleteBuffersARB");
        glGetBufferSubData_ = (PFNGLGETBUFFERSUBDATAARBPROC)getprocaddress("glGetBufferSubDataARB");
        hasVBO = true;
        //conoutf("Using GL_ARB_vertex_buffer_object extension.");
    }
    else conoutf("WARNING: No vertex_buffer_object extension! (geometry heavy maps will be SLOW)");

    if(strstr(exts, "GL_EXT_draw_range_elements"))
    {
        glDrawRangeElements_ = (PFNGLDRAWRANGEELEMENTSEXTPROC)getprocaddress("glDrawRangeElementsEXT");
        hasDRE = true;
    }

    if(strstr(exts, "GL_EXT_multi_draw_arrays"))
    {
        glMultiDrawArrays_   = (PFNGLMULTIDRAWARRAYSEXTPROC)  getprocaddress("glMultiDrawArraysEXT");
        glMultiDrawElements_ = (PFNGLMULTIDRAWELEMENTSEXTPROC)getprocaddress("glMultiDrawElementsEXT");
        hasMDA = true;
    }

#ifdef __APPLE__
    // floating point FBOs not fully supported until 10.5
    if(osversion>=0x1050)
#endif
    if(strstr(exts, "GL_ARB_texture_float") || strstr(exts, "GL_ATI_texture_float"))
    {
        hasTF = true;
        //conoutf("Using GL_ARB_texture_float extension");
        shadowmap = 1;
        extern int smoothshadowmappeel;
        smoothshadowmappeel = 1;
    }

    if(strstr(exts, "GL_NV_float_buffer")) hasNVFB = true;

    if(strstr(exts, "GL_EXT_framebuffer_object"))
    {
        glBindRenderbuffer_        = (PFNGLBINDRENDERBUFFEREXTPROC)       getprocaddress("glBindRenderbufferEXT");
        glDeleteRenderbuffers_     = (PFNGLDELETERENDERBUFFERSEXTPROC)    getprocaddress("glDeleteRenderbuffersEXT");
        glGenRenderbuffers_        = (PFNGLGENFRAMEBUFFERSEXTPROC)        getprocaddress("glGenRenderbuffersEXT");
        glRenderbufferStorage_     = (PFNGLRENDERBUFFERSTORAGEEXTPROC)    getprocaddress("glRenderbufferStorageEXT");
        glCheckFramebufferStatus_  = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC) getprocaddress("glCheckFramebufferStatusEXT");
        glBindFramebuffer_         = (PFNGLBINDFRAMEBUFFEREXTPROC)        getprocaddress("glBindFramebufferEXT");
        glDeleteFramebuffers_      = (PFNGLDELETEFRAMEBUFFERSEXTPROC)     getprocaddress("glDeleteFramebuffersEXT");
        glGenFramebuffers_         = (PFNGLGENFRAMEBUFFERSEXTPROC)        getprocaddress("glGenFramebuffersEXT");
        glFramebufferTexture2D_    = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)   getprocaddress("glFramebufferTexture2DEXT");
        glFramebufferRenderbuffer_ = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)getprocaddress("glFramebufferRenderbufferEXT");
        glGenerateMipmap_          = (PFNGLGENERATEMIPMAPEXTPROC)         getprocaddress("glGenerateMipmapEXT");
        hasFBO = true;
        //conoutf("Using GL_EXT_framebuffer_object extension.");
    }
    else conoutf("WARNING: No framebuffer object support. (reflective water may be slow)");

    if(strstr(exts, "GL_ARB_occlusion_query"))
    {
        GLint bits;
        glGetQueryiv_ = (PFNGLGETQUERYIVARBPROC)getprocaddress("glGetQueryivARB");
        glGetQueryiv_(GL_SAMPLES_PASSED_ARB, GL_QUERY_COUNTER_BITS_ARB, &bits);
        if(bits)
        {
            glGenQueries_ =        (PFNGLGENQUERIESARBPROC)       getprocaddress("glGenQueriesARB");
            glDeleteQueries_ =     (PFNGLDELETEQUERIESARBPROC)    getprocaddress("glDeleteQueriesARB");
            glBeginQuery_ =        (PFNGLBEGINQUERYARBPROC)       getprocaddress("glBeginQueryARB");
            glEndQuery_ =          (PFNGLENDQUERYARBPROC)         getprocaddress("glEndQueryARB");
            glGetQueryObjectiv_ =  (PFNGLGETQUERYOBJECTIVARBPROC) getprocaddress("glGetQueryObjectivARB");
            glGetQueryObjectuiv_ = (PFNGLGETQUERYOBJECTUIVARBPROC)getprocaddress("glGetQueryObjectuivARB");
            hasOQ = true;
            //conoutf("Using GL_ARB_occlusion_query extension.");
#if defined(__APPLE__) && SDL_BYTEORDER == SDL_BIG_ENDIAN
            if(strstr(vendor, "ATI") && (osversion<0x1050)) ati_oq_bug = 1;
#endif
            if(ati_oq_bug) conoutf("WARNING: Using ATI occlusion query bug workaround. (use \"/ati_oq_bug 0\" to disable if unnecessary)");
        }
    }
    if(!hasOQ)
    {
        conoutf("WARNING: No occlusion query support! (large maps may be SLOW)");
        zpass = 0;
        extern int vacubesize;
        vacubesize = 64;
        waterreflect = 0;
    }

    extern int reservedynlighttc, reserveshadowmaptc, maxtexsize, batchlightmaps;
    if(strstr(vendor, "ATI"))
    {
        floatvtx = 1;
        //conoutf("WARNING: ATI cards may show garbage in skybox. (use \"/ati_skybox_bug 1\" to fix)");

        reservedynlighttc = 2;
        reserveshadowmaptc = 3;
        minimizetcusage = 1;
        emulatefog = 1;
        extern int depthfxprecision;
        if(hasTF) depthfxprecision = 1;

        ati_texgen_bug = 1;
    }
    else if(strstr(vendor, "NVIDIA"))
    {
        reservevpparams = 10;
        rtsharefb = 0; // work-around for strange driver stalls involving when using many FBOs
        extern int filltjoints;
        if(!strstr(exts, "GL_EXT_gpu_shader4")) filltjoints = 0; // DX9 or less NV cards seem to not cause many sparklies

        nvidia_texgen_bug = 1;
        if(hasFBO && !hasTF) nvidia_scissor_bug = 1; // 5200 bug, clearing with scissor on an FBO messes up on reflections, may affect lesser cards too
        extern int fpdepthfx;
        if(hasTF && (!strstr(renderer, "GeForce") || !checkseries(renderer, 6000, 6600)))
            fpdepthfx = 1; // FP filtering causes software fallback on 6200?
    }
    else if(strstr(vendor, "Intel"))
    {
        avoidshaders = 1;
        intel_quadric_bug = 1;
        maxtexsize = 256;
        reservevpparams = 20;
        batchlightmaps = 0;

        if(!hasOQ) waterrefract = 0;

#ifdef __APPLE__
        apple_vp_bug = 1;
#endif
    }
    else if(strstr(vendor, "Tungsten") || strstr(vendor, "Mesa") || strstr(vendor, "Microsoft") || strstr(vendor, "S3 Graphics"))
    {
        avoidshaders = 1;
        floatvtx = 1;
        maxtexsize = 256;
        reservevpparams = 20;
        batchlightmaps = 0;

        if(!hasOQ) waterrefract = 0;
    }
    //if(floatvtx) conoutf("WARNING: Using floating point vertexes. (use \"/floatvtx 0\" to disable)");

    if(strstr(exts, "GL_ARB_vertex_program") && strstr(exts, "GL_ARB_fragment_program"))
    {
        hasVP = hasFP = true;
        glGenPrograms_ =              (PFNGLGENPROGRAMSARBPROC)              getprocaddress("glGenProgramsARB");
        glDeletePrograms_ =           (PFNGLDELETEPROGRAMSARBPROC)           getprocaddress("glDeleteProgramsARB");
        glBindProgram_ =              (PFNGLBINDPROGRAMARBPROC)              getprocaddress("glBindProgramARB");
        glProgramString_ =            (PFNGLPROGRAMSTRINGARBPROC)            getprocaddress("glProgramStringARB");
        glGetProgramiv_ =             (PFNGLGETPROGRAMIVARBPROC)             getprocaddress("glGetProgramivARB");
        glProgramEnvParameter4f_ =    (PFNGLPROGRAMENVPARAMETER4FARBPROC)    getprocaddress("glProgramEnvParameter4fARB");
        glProgramEnvParameter4fv_ =   (PFNGLPROGRAMENVPARAMETER4FVARBPROC)   getprocaddress("glProgramEnvParameter4fvARB");
        glEnableVertexAttribArray_ =  (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)  getprocaddress("glEnableVertexAttribArrayARB");
        glDisableVertexAttribArray_ = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) getprocaddress("glDisableVertexAttribArrayARB");
        glVertexAttribPointer_ =      (PFNGLVERTEXATTRIBPOINTERARBPROC)      getprocaddress("glVertexAttribPointerARB");

        if(strstr(exts, "GL_ARB_shading_language_100") && strstr(exts, "GL_ARB_shader_objects") && strstr(exts, "GL_ARB_vertex_shader") && strstr(exts, "GL_ARB_fragment_shader"))
        {
            glCreateProgramObject_ =        (PFNGLCREATEPROGRAMOBJECTARBPROC)     getprocaddress("glCreateProgramObjectARB");
            glDeleteObject_ =               (PFNGLDELETEOBJECTARBPROC)            getprocaddress("glDeleteObjectARB");
            glUseProgramObject_ =           (PFNGLUSEPROGRAMOBJECTARBPROC)        getprocaddress("glUseProgramObjectARB");
            glCreateShaderObject_ =         (PFNGLCREATESHADEROBJECTARBPROC)      getprocaddress("glCreateShaderObjectARB");
            glShaderSource_ =               (PFNGLSHADERSOURCEARBPROC)            getprocaddress("glShaderSourceARB");
            glCompileShader_ =              (PFNGLCOMPILESHADERARBPROC)           getprocaddress("glCompileShaderARB");
            glGetObjectParameteriv_ =       (PFNGLGETOBJECTPARAMETERIVARBPROC)    getprocaddress("glGetObjectParameterivARB");
            glAttachObject_ =               (PFNGLATTACHOBJECTARBPROC)            getprocaddress("glAttachObjectARB");
            glGetInfoLog_ =                 (PFNGLGETINFOLOGARBPROC)              getprocaddress("glGetInfoLogARB");
            glLinkProgram_ =                (PFNGLLINKPROGRAMARBPROC)             getprocaddress("glLinkProgramARB");
            glGetUniformLocation_ =         (PFNGLGETUNIFORMLOCATIONARBPROC)      getprocaddress("glGetUniformLocationARB");
            glUniform4fv_ =                 (PFNGLUNIFORM4FVARBPROC)              getprocaddress("glUniform4fvARB");
            glUniform1i_ =                  (PFNGLUNIFORM1IARBPROC)               getprocaddress("glUniform1iARB");

            extern bool checkglslsupport();
            if(checkglslsupport())
            {
                hasGLSL = true;
#ifdef __APPLE__
                //if(osversion<0x1050) ??
                apple_glsldepth_bug = 1;
#endif
                if(apple_glsldepth_bug) conoutf("WARNING: Using Apple GLSL depth bug workaround. (use \"/apple_glsldepth_bug 0\" to disable if unnecessary");
            }
        }

        if(strstr(vendor, "ATI")) ati_dph_bug = 1;
        else if(strstr(vendor, "Tungsten")) mesa_program_bug = 1;

#ifdef __APPLE__
        if(osversion>=0x1050)
        {
            apple_ff_bug = 1;
            conoutf("WARNING: Using Leopard ARB_position_invariant bug workaround. (use \"/apple_ff_bug 0\" to disable if unnecessary)");
        }
#endif
    }

    if(strstr(exts, "GL_NV_vertex_program2_option")) { usevp2 = 1; hasVP2 = true; }
    if(strstr(exts, "GL_NV_vertex_program3")) { usevp3 = 1; hasVP3 = true; }

    if(strstr(exts, "GL_EXT_gpu_program_parameters"))
    {
        glProgramEnvParameters4fv_   = (PFNGLPROGRAMENVPARAMETERS4FVEXTPROC)  getprocaddress("glProgramEnvParameters4fvEXT");
        glProgramLocalParameters4fv_ = (PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC)getprocaddress("glProgramLocalParameters4fvEXT");
        hasPP = true;
    }

    if(strstr(exts, "GL_EXT_texture_rectangle") || strstr(exts, "GL_ARB_texture_rectangle"))
    {
        usetexrect = 1;
        hasTR = true;
        //conoutf("Using GL_ARB_texture_rectangle extension.");
    }
    else if(hasMT && hasVP && hasFP) conoutf("WARNING: No texture rectangle support. (no full screen shaders)");

    if(strstr(exts, "GL_EXT_packed_depth_stencil") || strstr(exts, "GL_NV_packed_depth_stencil"))
    {
        hasDS = true;
        //conoutf("Using GL_EXT_packed_depth_stencil extension.");
    }

    if(strstr(exts, "GL_EXT_blend_minmax"))
    {
        glBlendEquation_ = (PFNGLBLENDEQUATIONEXTPROC) getprocaddress("glBlendEquationEXT");
        hasBE = true;
        if(strstr(vendor, "ATI")) ati_minmax_bug = 1;
        //conoutf("Using GL_EXT_blend_minmax extension.");
    }

    if(strstr(exts, "GL_ARB_texture_cube_map"))
    {
        GLint val;
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &val);
        hwcubetexsize = val;
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

    if(strstr(exts, "GL_EXT_texture_filter_anisotropic"))
    {
       GLint val;
       glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &val);
       hwmaxaniso = val;
       hasAF = true;
       //conoutf("Using GL_EXT_texture_filter_anisotropic extension.");
    }

    if(strstr(exts, "GL_SGIS_generate_mipmap"))
    {
        hasGM = true;
        //conoutf("Using GL_SGIS_generate_mipmap extension.");
    }

    GLint val;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
    hwtexsize = val;
}

void glext(char *ext)
{
    const char *exts = (const char *)glGetString(GL_EXTENSIONS);
    intret(strstr(exts, ext) ? 1 : 0);
}
COMMAND(glext, "s");

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

	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);

    extern int useshaders;
    if(!useshaders || (useshaders<0 && avoidshaders) || !hasMT || !hasVP || !hasFP)
    {
        if(!hasMT || !hasVP || !hasFP) conoutf("WARNING: No shader support! Using fixed-function fallback. (no fancy visuals for you)");
        else if(useshaders<0 && !hasTF) conoutf("WARNING: Disabling shaders for extra performance. (use \"/shaders 1\" to enable shaders if desired)");
        renderpath = R_FIXEDFUNCTION;
        conoutf("Rendering using the OpenGL 1.5 fixed-function path.");
        if(ati_texgen_bug) conoutf("WARNING: Using ATI texgen bug workaround. (use \"/ati_texgen_bug 0\" to disable if unnecessary)");
        if(nvidia_texgen_bug) conoutf("WARNING: Using NVIDIA texgen bug workaround. (use \"/nvidia_texgen_bug 0\" to disable if unnecessary)");
    }
    else
    {
        renderpath = hasGLSL ? R_GLSLANG : R_ASMSHADER;
        if(renderpath==R_GLSLANG) conoutf("Rendering using the OpenGL 1.5 GLSL shader path.");
        else conoutf("Rendering using the OpenGL 1.5 assembly shader path.");
    }

    if(fsaa) glEnable(GL_MULTISAMPLE);

    inittmus();
}

void cleanupgl()
{
    if(glIsEnabled(GL_MULTISAMPLE)) glDisable(GL_MULTISAMPLE);

    extern int nomasks, nolights, nowater;
    nomasks = nolights = nowater = 0;
}

VAR(wireframe, 0, 0, 1);

physent camera, *camera1 = &camera;
vec worldpos, camdir, camright, camup;

void findorientation(vec &o, float yaw, float pitch, vec &pos)
{
	vec dir;
	vecfromyawpitch(yaw, pitch, 1, 0, dir);
	if(raycubepos(o, dir, pos, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
		pos = dir.mul(2*hdr.worldsize).add(o); //otherwise 3dgui won't work when outside of map
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

float curfov = 100, fovy, aspect;
int farplane;
VARP(fov, 1, 120, 360);

int xtraverts, xtravertsva;

VARW(fog, 16, 4000, INT_MAX-1);
VARW(fogcolour, 0, 0x8099B3, 0xFFFFFF);

bool isthirdperson() { return cl->gamethirdperson() || reflecting; }

void vecfromcursor(float x, float y, float z, vec &dir)
{
	GLdouble cmvm[16], cpjm[16];
	GLint view[4];
	GLdouble dx, dy, dz;

	glGetDoublev(GL_MODELVIEW_MATRIX, cmvm);
	glGetDoublev(GL_PROJECTION_MATRIX, cpjm);
	glGetIntegerv(GL_VIEWPORT, view);

	gluUnProject(x*float(view[2]), (1.f-y)*float(view[3]), z, cmvm, cpjm, view, &dx, &dy, &dz);
	dir = vec((float)dx, (float)dy, (float)dz);
	gluUnProject(x*float(view[2]), (1.f-y)*float(view[3]), 0.0f, cmvm, cpjm, view, &dx, &dy, &dz);
	dir.sub(vec((float)dx, (float)dy, (float)dz));
	dir.normalize();
}

void vectocursor(vec &v, float &x, float &y, float &z)
{
	GLdouble cmvm[16], cpjm[16];
	GLint view[4];
	GLdouble dx, dy, dz;

	glGetDoublev(GL_MODELVIEW_MATRIX, cmvm);
	glGetDoublev(GL_PROJECTION_MATRIX, cpjm);
	glGetIntegerv(GL_VIEWPORT, view);

	gluProject(v.x, v.y, v.z, cmvm, cpjm, view, &dx, &dy, &dz);
	x = (float)dx;
	y = (float)view[3]-dy;
	z = (float)dz;
}

void project(float fovy, float aspect, int farplane, bool flipx, bool flipy, bool swapxy)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if(swapxy) glRotatef(90, 0, 0, 1);
    if(flipx || flipy!=swapxy) glScalef(flipx ? -1 : 1, flipy!=swapxy ? -1 : 1, 1);
    gluPerspective(fovy, aspect, 0.54f, farplane);
    glMatrixMode(GL_MODELVIEW);
}

VAR(reflectclip, 0, 6, 64);

GLfloat clipmatrix[16];

void genclipmatrix(float a, float b, float c, float d, GLfloat matrix[16])
{
	// transform the clip plane into camera space
    float clip[4];
    loopi(4) clip[i] = a*invmvmatrix[i*4 + 0] + b*invmvmatrix[i*4 + 1] + c*invmvmatrix[i*4 + 2] + d*invmvmatrix[i*4 + 3];

    memcpy(matrix, projmatrix, 16*sizeof(GLfloat));

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

FVAR(polygonoffsetfactor, -3.0f);
FVAR(polygonoffsetunits, -3.0f);
FVAR(depthoffset, 0.01f);

void enablepolygonoffset(GLenum type)
{
    if(!depthoffset)
    {
        glPolygonOffset(polygonoffsetfactor, polygonoffsetunits);
        glEnable(type);
        return;
    }

    bool clipped = reflectz < 1e15f && reflectclip;

    GLfloat offsetmatrix[16];
    memcpy(offsetmatrix, clipped ? clipmatrix : projmatrix, 16*sizeof(GLfloat));
    offsetmatrix[14] += depthoffset * projmatrix[10];

    glMatrixMode(GL_PROJECTION);
    if(!clipped) glPushMatrix();
    glLoadMatrixf(offsetmatrix);
    glMatrixMode(GL_MODELVIEW);
}

void disablepolygonoffset(GLenum type)
{
    if(!depthoffset)
    {
        glDisable(type);
        return;
    }

    bool clipped = reflectz < 1e15f && reflectclip;

    glMatrixMode(GL_PROJECTION);
    if(clipped) glLoadMatrixf(clipmatrix);
    else glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void setfogplane(const plane &p, bool flush)
{
	static float fogselect[4] = {0, 0, 0, 0};
    if(flush)
    {
        flushenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
        flushenvparamfv("fogplane", SHPARAM_VERTEX, 9, p.v);
    }
    else
    {
        setenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
        setenvparamfv("fogplane", SHPARAM_VERTEX, 9, p.v);
    }
}

void setfogplane(float scale, float z, bool flush, float fadescale, float fadeoffset)
{
    float fogselect[4] = {1, fadescale, fadeoffset, 0}, fogplane[4] = {0, 0, 0, 0};
    if(scale || z)
    {
        fogselect[0] = 0;
        fogplane[2] = scale;
        fogplane[3] = -z;
    }
    if(flush)
    {
        flushenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
        flushenvparamfv("fogplane", SHPARAM_VERTEX, 9, fogplane);
    }
    else
    {
        setenvparamfv("fogselect", SHPARAM_VERTEX, 8, fogselect);
        setenvparamfv("fogplane", SHPARAM_VERTEX, 9, fogplane);
    }
}

static float findsurface(int fogmat, const vec &v, int &abovemat)
{
    ivec o(v);
    do
    {
        cube &c = lookupcube(o.x, o.y, o.z);
        if(!c.ext || (c.ext->material&MATF_VOLUME) != fogmat)
        {
            abovemat = c.ext && isliquid(c.ext->material&MATF_VOLUME) ? c.ext->material&MATF_VOLUME : MAT_AIR;
            return o.z;
        }
        o.z = lu.z + lusize;
    }
    while(o.z < hdr.worldsize);
    abovemat = MAT_AIR;
    return hdr.worldsize;
}

static void blendfog(int fogmat, float blend, float logblend, float &start, float &end, float *fogc)
{
    uchar col[3];
    switch(fogmat)
    {
        case MAT_WATER:
            getwatercolour(col);
            loopk(3) fogc[k] += blend*col[k]/255.0f;
            end += logblend*min(fog, max(waterfog*4, 32));
            break;

        case MAT_LAVA:
            getlavacolour(col);
            loopk(3) fogc[k] += blend*col[k]/255.0f;
            end += logblend*min(fog, max(lavafog*4, 32));
            break;

        default:
            fogc[0] += blend*(fogcolour>>16)/255.0f;
            fogc[1] += blend*((fogcolour>>8)&255)/255.0f;
            fogc[2] += blend*(fogcolour&255)/255.0f;
            start += logblend*(fog+64)/8;
            end += logblend*fog;
            break;
    }
}

static void setfog(int fogmat, float below = 1, int abovemat = MAT_AIR)
{
    float fogc[4] = { 0, 0, 0, 1 };
    float start = 0, end = 0;
    float logscale = 256, logblend = log(1 + (logscale - 1)*below) / log(logscale);

    blendfog(fogmat, below, logblend, start, end, fogc);
    if(below < 1) blendfog(abovemat, 1-below, 1-logblend, start, end, fogc);

    glFogf(GL_FOG_START, start);
    glFogf(GL_FOG_END, end);
    glFogfv(GL_FOG_COLOR, fogc);
    glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);

    if(renderpath!=R_FIXEDFUNCTION) setfogplane();
}

static void blendfogoverlay(int fogmat, float blend, float *overlay)
{
    uchar col[3];
    float maxc;
    switch(fogmat)
    {
        case MAT_WATER:
            getwatercolour(col);
            maxc = max(col[0], max(col[1], col[2]));
            loopk(3) overlay[k] += blend*max(0.4f, col[k]/min(32.0f + maxc*7.0f/8.0f, 255.0f));
            break;

        case MAT_LAVA:
            getlavacolour(col);
            maxc = max(col[0], max(col[1], col[2]));
            loopk(3) overlay[k] += blend*max(0.4f, col[k]/min(32.0f + maxc*7.0f/8.0f, 255.0f));
            break;

        default:
            loopk(3) overlay[k] += blend;
            break;
    }
}

bool renderedgame = false;

void rendergame()
{
    cl->render();
    if(!shadowmapping) renderedgame = true;
}

VARP(skyboxglare, 0, 1, 1);

void drawglare()
{
    glaring = true;
    refracting = -1;

    float oldfogstart, oldfogend, oldfogcolor[4], zerofog[4] = { 0, 0, 0, 1 };
    glGetFloatv(GL_FOG_START, &oldfogstart);
    glGetFloatv(GL_FOG_END, &oldfogend);
    glGetFloatv(GL_FOG_COLOR, oldfogcolor);

    glFogi(GL_FOG_START, (fog+64)/8);
    glFogi(GL_FOG_END, fog);
    glFogfv(GL_FOG_COLOR, zerofog);

    glClearColor(0, 0, 0, 1);
    glClear((skyboxglare ? 0 : GL_COLOR_BUFFER_BIT) | GL_DEPTH_BUFFER_BIT);

    rendergeom();

    if(skyboxglare) drawskybox(farplane, false);

    renderreflectedmapmodels();
    rendergame();

    renderwater();
    rendermaterials();
    render_particles(0);

    glFogf(GL_FOG_START, oldfogstart);
    glFogf(GL_FOG_END, oldfogend);
    glFogfv(GL_FOG_COLOR, oldfogcolor);

    refracting = 0;
    glaring = false;
}

VARP(reflectmms, 0, 1, 1);

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

    reflectz = z < 0 ? 1e16f : z;
    reflecting = !refract;
    refracting = refract ? (z < 0 || camera1->o.z >= z ? -1 : 1) : 0;
    fading = renderpath!=R_FIXEDFUNCTION && waterrefract && waterfade && hasFBO && z>=0;
    fogging = refracting<0 && z>=0 && (renderpath!=R_FIXEDFUNCTION || refractfog);

    float oldfogstart, oldfogend, oldfogcolor[4];
    if(renderpath==R_FIXEDFUNCTION && fogging) glDisable(GL_FOG);
    else
    {
        glGetFloatv(GL_FOG_START, &oldfogstart);
        glGetFloatv(GL_FOG_END, &oldfogend);
        glGetFloatv(GL_FOG_COLOR, oldfogcolor);

        if(fogging)
        {
            glFogi(GL_FOG_START, 0);
            glFogi(GL_FOG_END, waterfog);
            glFogfv(GL_FOG_COLOR, fogc);
        }
        else
        {
            glFogi(GL_FOG_START, (fog+64)/8);
            glFogi(GL_FOG_END, fog);
            float fogc[4] = { (fogcolour>>16)/255.0f, ((fogcolour>>8)&255)/255.0f, (fogcolour&255)/255.0f, 1.0f };
            glFogfv(GL_FOG_COLOR, fogc);
        }
    }

	if(clear)
	{
		glClearColor(fogc[0], fogc[1], fogc[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

    if(reflecting)
    {
        glPushMatrix();
        glTranslatef(0, 0, 2*z);
        glScalef(1, 1, -1);

        glCullFace(GL_BACK);
    }

    if(reflectclip && z>=0)
    {
        float zoffset = reflectclip/4.0f, zclip;
        if(refracting<0)
        {
            zclip = z+zoffset;
            if(camera1->o.z<=zclip) zclip = z;
        }
        else
        {
            zclip = z-zoffset;
            if(camera1->o.z>=zclip && camera1->o.z<=z+4.0f) zclip = z;
            if(reflecting) zclip = 2*z - zclip;
        }
        genclipmatrix(0, 0, refracting>0 ? 1 : -1, refracting>0 ? -zclip : zclip, clipmatrix);
        setclipmatrix(clipmatrix);
    }


    renderreflectedgeom(refracting<0 && z>=0 && caustics, fogging);

    if(reflecting || refracting>0 || z<0)
    {
        if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        if(reflectclip && z>=0) undoclipmatrix();
        drawskybox(farplane, false);
        if(reflectclip && z>=0) setclipmatrix(clipmatrix);
        if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    }
    else if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

    if(reflectmms) renderreflectedmapmodels();
    rendergame();

    if(fogging) setfogplane(1, z);
    if(refracting) rendergrass();
    renderdecals(0);
    rendermaterials();
    render_particles(0);

    if(fading) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    if(fogging) setfogplane();

    if(reflectclip && z>=0) undoclipmatrix();

    if(reflecting)
    {
        glPopMatrix();

        glCullFace(GL_FRONT);
    }

    if(renderpath==R_FIXEDFUNCTION && fogging) glEnable(GL_FOG);
    else
	{
		glFogf(GL_FOG_START, oldfogstart);
		glFogf(GL_FOG_END, oldfogend);
		glFogfv(GL_FOG_COLOR, oldfogcolor);
	}

    reflectz = 1e16f;
    refracting = 0;
    reflecting = fading = fogging = false;
}

bool envmapping = false;

void drawcubemap(int size, const vec &o, float yaw, float pitch, bool full, bool flipx, bool flipy, bool swapxy)
{
    envmapping = true;

	physent *oldcamera = camera1;
	static physent cmcamera;
	cmcamera = *camera1;
	cmcamera.reset();
	cmcamera.type = ENT_CAMERA;
	cmcamera.o = o;
	cmcamera.yaw = yaw;
	cmcamera.pitch = pitch;
	cmcamera.roll = 0;
	camera1 = &cmcamera;

	defaultshader->set();

    int fogmat = lookupmaterial(o)&MATF_VOLUME;
	if(fogmat!=MAT_WATER && fogmat!=MAT_LAVA) fogmat = MAT_AIR;

	setfog(fogmat);

	glClear(GL_DEPTH_BUFFER_BIT);

	int farplane = hdr.worldsize*2;

    project(90.0f, 1.0f, farplane, flipx, flipy, swapxy);

	transplayer();

	glEnable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	xtravertsva = xtraverts = glde = gbatches = 0;

	visiblecubes(90, 90);

	if(limitsky()) drawskybox(farplane, true);

	rendergeom();

    if(!limitsky()) drawskybox(farplane, false);

	if(full) queryreflections();

	rendermapmodels();

	if(full)
	{
		drawreflections();

		renderwater();
		rendermaterials();
	}

	glDisable(GL_TEXTURE_2D);

	camera1 = oldcamera;
    envmapping = false;
}

void gl_drawhud(int w, int h, int fogmat, float fogblend, int abovemat);

const char *loadback = "textures/loadback";

void loadbackground(int w, int h)
{
	glColor3f(1, 1, 1);

	settexture(loadback);

	glBegin(GL_QUADS);

	glTexCoord2f(0, 0); glVertex2f(0, 0);
	glTexCoord2f(1, 0); glVertex2f(w, 0);
	glTexCoord2f(1, 1); glVertex2f(w, h);
	glTexCoord2f(0, 1); glVertex2f(0, h);

	glEnd();
}

static void getcomputescreenres(int &w, int &h)
{
    float wk = 1, hk = 1;
    if(w < 1024) wk = 1024.0f/w;
    if(h < 768) hk = 768.0f/h;
    wk = hk = max(wk, hk);
    w = int(ceil(w*wk));
    h = int(ceil(h*hk));
}

void computescreen(const char *text, Texture *t, const char *overlaytext)
{
	int w = screen->w, h = screen->h;
	if(overlaytext && text)
    {
        s_sprintfd(caption)("%s %s", text, overlaytext);
        setcaption(caption);
    }
    else setcaption(overlaytext ? overlaytext : text);
    getcomputescreenres(w, h);
	gettextres(w, h);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClearColor(0.f, 0.f, 0.f, 1);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    defaultshader->set();
	loopi(2)
	{
		glClear(GL_COLOR_BUFFER_BIT);

		loadbackground(w, h);

        if(text)
        {
            glPushMatrix();
            glScalef(1/3.0f, 1/3.0f, 1);
            draw_text(text, 70, 2*FONTH + FONTH/2);
            glPopMatrix();
        }
		if(t)
		{
			glDisable(GL_BLEND);
			glBindTexture(GL_TEXTURE_2D, t->id);
			int sz = 256, x = (w-sz)/2, y = min(384, h-256);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(x,	y);
			glTexCoord2f(1, 0); glVertex2f(x+sz, y);
			glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
			glTexCoord2f(0, 1); glVertex2f(x,	y+sz);
			glEnd();
			glEnable(GL_BLEND);
		}
        if(overlaytext)
        {
            int sz = 256, x = (w-sz)/2, y = min(384, h-256), tw = text_width(overlaytext);
            int tx = t && tw < sz*2 - FONTH/3 ?
                        2*(x + sz) - tw - FONTH/3 :
                        2*(x + sz/2) - tw/2,
                     ty = t ?
                        2*(y + sz) - FONTH*4/3 :
                        2*(y + sz/2) - FONTH/2;
            glPushMatrix();
            glScalef(1/2.0f, 1/2.0f, 1);
            draw_text(overlaytext, tx, ty);
            glPopMatrix();
        }

		int x = (w-512)/2, y = 128;
		settexture("textures/logo");
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(x,	 y);
		glTexCoord2f(1, 0); glVertex2f(x+512, y);
		glTexCoord2f(1, 1); glVertex2f(x+512, y+256);
		glTexCoord2f(0, 1); glVertex2f(x,	 y+256);
		glEnd();

		SDL_GL_SwapBuffers();
	}
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

static void bar(float bar, int w, int o, float r, float g, float b)
{
	int side = 2*FONTH;
	float x1 = side, x2 = min(bar, 1.0f)*(w*3-2*side)+side;
	float y1 = o*FONTH;
	glColor3f(r, g, b);
	glBegin(GL_TRIANGLE_STRIP);
	loopk(10)
	{
		float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 + sinf(M_PI/2 + k/9.0f*M_PI);
		glVertex2f(x2 - c*FONTH, y1 + s*FONTH);
		glVertex2f(x1 + c*FONTH, y1 + s*FONTH);
	}
	glEnd();

#if 0
	glColor3f(0.3f, 0.3f, 0.3f);
	glBegin(GL_LINE_LOOP);
	loopk(10)
	{
		float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 + sinf(M_PI/2 + k/9.0f*M_PI);
		glVertex2f(x1 + c*FONTH, y1 + s*FONTH);
	}
	loopk(10)
	{
		float c = cosf(M_PI/2 + k/9.0f*M_PI), s = 1 - sinf(M_PI/2 + k/9.0f*M_PI);
		glVertex2f(x2 - c*FONTH, y1 + s*FONTH);
	}
	glEnd();
#endif
}

void renderprogress(float bar1, const char *text1, float bar2, const char *text2, GLuint tex)	// also used during loading
{
	if (!inbetweenframes) return;

	clientkeepalive();

	if (verbose >= 4)
	{
		if (text2) conoutf("%s [%.2f%%], %s [%.2f%%]", text1, bar1*100.f, text2, bar2*100.f);
		else if (text1) conoutf("%s [%.2f%%]", text1, bar1*100.f);
		else conoutf("progressing [%.2f%%]", text1, bar1*100.f, text2, bar2*100.f);
	}

	int w = screen->w, h = screen->h;
    getcomputescreenres(w, h);
	gettextres(w, h);

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, w*3, h*3, 0, -1, 1);
	notextureshader->set();

	glLineWidth(3);

	if(text1)
	{
		bar(1, w, 4, 0, 0, 0.8f);
		if(bar1>0) bar(bar1, w, 4, 0, 0.5f, 1);
	}

	if(bar2>0)
	{
		bar(1, w, 6, 0.5f, 0, 0);
		bar(bar2, w, 6, 0.75f, 0, 0);
	}

	glLineWidth(1);

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	defaultshader->set();

	if(text1) draw_text(text1, 2*FONTH, 4*FONTH + FONTH/2);
	if(bar2>0) draw_text(text2, 2*FONTH, 6*FONTH + FONTH/2);

	glDisable(GL_BLEND);

	if(tex)
	{
		glBindTexture(GL_TEXTURE_2D, tex);
		int sz = 256, x = (w-sz)/2, y = min(384, h-256);
		sz *= 3;
		x *= 3;
		y *= 3;
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(x,	y);
		glTexCoord2f(1, 0); glVertex2f(x+sz, y);
		glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
		glTexCoord2f(0, 1); glVertex2f(x,	y+sz);
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
	SDL_GL_SwapBuffers();
}

bool dopostfx = false;

void invalidatepostfx()
{
    dopostfx = false;
}

GLfloat mvmatrix[16], projmatrix[16], mvpmatrix[16], invmvmatrix[16];

void readmatrices()
{
    glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix);
    glGetFloatv(GL_PROJECTION_MATRIX, projmatrix);

    loopi(4) loopj(4)
    {
        float c = 0;
        loopk(4) c += projmatrix[k*4 + j] * mvmatrix[i*4 + k];
        mvpmatrix[i*4 + j] = c;
    }

    loopi(3)
    {
        loopj(3) invmvmatrix[i*4 + j] = mvmatrix[i + j*4];
        invmvmatrix[i*4 + 3] = 0;
    }
    loopi(3)
    {
        float c = 0;
        loopj(3) c -= mvmatrix[i*4 + j] * mvmatrix[12 + j];
        invmvmatrix[12 + i] = c;
    }
    invmvmatrix[15] = 1;
}

void gl_drawframe(int w, int h)
{
	defaultshader->set();

    updatedynlights();

    int fogmat = lookupmaterial(camera1->o)&MATF_VOLUME, abovemat = MAT_AIR;
    float fogblend = 1.0f, causticspass = 0.0f;
    if(fogmat==MAT_WATER || fogmat==MAT_LAVA)
    {
        float z = findsurface(fogmat, camera1->o, abovemat) - WATER_OFFSET;
        if(camera1->o.z < z + 1) fogblend = min(z + 1 - camera1->o.z, 1.0f);
        else fogmat = abovemat;
        if(caustics && fogmat==MAT_WATER && camera1->o.z < z)
            causticspass = renderpath==R_FIXEDFUNCTION ? 1.0f : min(z - camera1->o.z, 1.0f);
    }
    else
    {
    	fogmat = MAT_AIR;
    }
    setfog(fogmat, fogblend, abovemat);
    if(fogmat!=MAT_AIR)
    {
        float blend = abovemat==MAT_AIR ? fogblend : 1.0f;
        fovy += blend*sinf(lastmillis/1000.0)*2.0f;
        aspect += blend*sinf(lastmillis/1000.0+PI)*0.1f;
    }

    farplane = hdr.worldsize*2;

	project(fovy, aspect, farplane);
	transplayer();
    readmatrices();
    cl->project(w, h, cursordir, cursorx, cursory);

	glEnable(GL_TEXTURE_2D);

	glPolygonMode(GL_FRONT_AND_BACK, wireframe && editmode ? GL_LINE : GL_FILL);

	xtravertsva = xtraverts = glde = gbatches = 0;

    if(!hasFBO)
    {
        if(dopostfx)
        {
            drawglaretex();
            drawdepthfxtex();
            drawreflections();
        }
        else dopostfx = true;
    }

    visiblecubes(curfov, fovy);

	if(shadowmap && !hasFBO) rendershadowmap();

	glClear(GL_DEPTH_BUFFER_BIT|(wireframe && editmode ? GL_COLOR_BUFFER_BIT : 0)|(hasstencil ? GL_STENCIL_BUFFER_BIT : 0));

	if(limitsky()) drawskybox(farplane, true);

	rendergeom(causticspass);

    extern int outline, blankgeom;
    if(!wireframe && editmode && (outline || (fullbright && blankgeom))) renderoutline();

	queryreflections();

    if(!limitsky()) drawskybox(farplane, false);

	rendermapmodels();
    rendergame();

    if(hasFBO)
    {
        drawglaretex();
        drawdepthfxtex();
        drawreflections();
    }

    renderdecals(curtime);
    renderwater();
    rendergrass();

	rendermaterials();
	render_particles(curtime);

	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);

    addglare();
	renderfullscreenshader(w, h);

    gl_drawhud(w, h, fogmat, fogblend, abovemat);

	glEnable(GL_CULL_FACE);
	glEnable(GL_FOG);

    renderedgame = false;
}

VARP(hidehud, 0, 0, 1);

float cursorx = 0.5f, cursory = 0.5f;
vec cursordir(0, 0, 0);

void gl_drawhud(int w, int h, int fogmat, float fogblend, int abovemat)
{
    defaultshader->set();

	if(!cc->ready())
	{
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, w, h, 0, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

        glDisable(GL_DEPTH_TEST);

		loadbackground(w, h);

        glEnable(GL_DEPTH_TEST);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

    g3d_render();

	glDisable(GL_TEXTURE_2D);
	notextureshader->set();

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

	if(cc->ready())
	{
		glEnable(GL_BLEND);
		vec colour;
		bool hashudcolour = cl->gethudcolour(colour);
		if (hashudcolour || fogmat==MAT_WATER || fogmat==MAT_LAVA)
		{
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
			if (hashudcolour) glColor3f(colour.x, colour.y, colour.z);
			else
			{
				float overlay[3] = { 0, 0, 0 };
				blendfogoverlay(fogmat, fogblend, overlay);
				blendfogoverlay(abovemat, 1-fogblend, overlay);
				glColor3fv(overlay);
			}
			glBegin(GL_QUADS);
			glVertex2f(0, 0);
			glVertex2f(w, 0);
			glVertex2f(w, h);
			glVertex2f(0, h);
			glEnd();
		}

		glDisable(GL_BLEND);
	}

    glColor3f(1, 1, 1);

    extern int debugsm;
    if(debugsm)
    {
        extern void viewshadowmap();
        viewshadowmap();
    }

    extern int debugglare;
    if(debugglare)
    {
        extern void viewglaretex();
        viewglaretex();
    }

    extern int debugdepthfx;
    if(debugdepthfx)
    {
        extern void viewdepthfxtex();
        viewdepthfxtex();
    }

	glEnable(GL_TEXTURE_2D);
	defaultshader->set();

	cl->drawhud(w, h); // can make more dramatic changes this way without getting in the way

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
}

VARP(hudblend, 0, 99, 100);

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

void renderdir(vec &o, float yaw, float pitch, bool nf)
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
	rendertris(to, yaw, pitch, 2.f, 0.f, 0.f, 0.5f, true, nf);
}

void renderradius(vec &o, float height, float radius, bool nf)
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
		glBindTexture(GL_TEXTURE_2D, t->id);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x,	y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x+xs, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x+xs, y+ys);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x,	y+ys);
		glEnd();
		return true;
	}
	return false;
}

extern int scr_w, scr_h, fov;
