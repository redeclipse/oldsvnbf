// rendergl.cpp: core opengl rendering stuff

#include "engine.h"

bool hasVBO = false, hasDRE = false, hasOQ = false, hasTR = false, hasFBO = false, hasDS = false, hasTF = false, hasBE = false, hasBC = false, hasCM = false, hasNP2 = false, hasTC = false, hasTE = false, hasMT = false, hasD3 = false, hasAF = false, hasVP2 = false, hasVP3 = false, hasPP = false, hasMDA = false, hasTE3 = false, hasTE4 = false, hasVP = false, hasFP = false, hasGLSL = false, hasGM = false, hasNVFB = false, hasSGIDT = false, hasSGISH = false, hasDT = false, hasSH = false, hasNVPCF = false, hasRN = false, hasPBO = false, hasFBB = false;
int hasstencil = 0;

VAR(renderpath, 1, 0, 0);

// GL_ARB_vertex_buffer_object, GL_ARB_pixel_buffer_object
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

// GL_EXT_framebuffer_blit
PFNGLBLITFRAMEBUFFEREXTPROC         glBlitFramebuffer_         = NULL;

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

// GL_EXT_blend_color
PFNGLBLENDCOLOREXTPROC glBlendColor_ = NULL;

// GL_EXT_multi_draw_arrays
PFNGLMULTIDRAWARRAYSEXTPROC   glMultiDrawArrays_ = NULL;
PFNGLMULTIDRAWELEMENTSEXTPROC glMultiDrawElements_ = NULL;

// GL_ARB_texture_compression
PFNGLCOMPRESSEDTEXIMAGE3DARBPROC    glCompressedTexImage3D_    = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DARBPROC    glCompressedTexImage2D_    = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DARBPROC    glCompressedTexImage1D_    = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC glCompressedTexSubImage3D_ = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC glCompressedTexSubImage2D_ = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC glCompressedTexSubImage1D_ = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC   glGetCompressedTexImage_   = NULL;

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
VAR(sdl_backingstore_bug, -1, 0, 1);
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

VAR(dbgexts, 0, 0, 1);

void gl_checkextensions()
{
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    const char *exts = (const char *)glGetString(GL_EXTENSIONS);
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    const char *version = (const char *)glGetString(GL_VERSION);
    conoutf("\fmRenderer: %s (%s)", renderer, vendor);
    conoutf("\fmDriver: %s", version);

#ifdef __APPLE__
    extern int mac_osversion();
    int osversion = mac_osversion();  /* 0x1050 = 10.5 (Leopard) */
    sdl_backingstore_bug = -1;
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
        if(dbgexts) conoutf("\frUsing GL_ARB_texture_env_combine extension.");
    }
    else conoutf("\frWARNING: No texture_env_combine extension! (your video card is WAY too old)");

    if(strstr(exts, "GL_ARB_multitexture"))
    {
        glActiveTexture_       = (PFNGLACTIVETEXTUREARBPROC)      getprocaddress("glActiveTextureARB");
        glClientActiveTexture_ = (PFNGLCLIENTACTIVETEXTUREARBPROC)getprocaddress("glClientActiveTextureARB");
        glMultiTexCoord2f_     = (PFNGLMULTITEXCOORD2FARBPROC)    getprocaddress("glMultiTexCoord2fARB");
        glMultiTexCoord3f_     = (PFNGLMULTITEXCOORD3FARBPROC)    getprocaddress("glMultiTexCoord3fARB");
        glMultiTexCoord4f_     = (PFNGLMULTITEXCOORD4FARBPROC)    getprocaddress("glMultiTexCoord4fARB");
        hasMT = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_multitexture extension.");
    }
    else conoutf("\frWARNING: No multitexture extension!");

    if(strstr(exts, "GL_ARB_vertex_buffer_object"))
    {
        hasVBO = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_vertex_buffer_object extension.");
    }
    else conoutf("\frWARNING: No vertex_buffer_object extension! (geometry heavy maps will be SLOW)");

    if(strstr(exts, "GL_ARB_pixel_buffer_object"))
    {
        hasPBO = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_pixel_buffer_object extension.");
    }

    if(hasVBO || hasPBO)
    {
        glGenBuffers_       = (PFNGLGENBUFFERSARBPROC)      getprocaddress("glGenBuffersARB");
        glBindBuffer_       = (PFNGLBINDBUFFERARBPROC)      getprocaddress("glBindBufferARB");
        glMapBuffer_        = (PFNGLMAPBUFFERARBPROC)       getprocaddress("glMapBufferARB");
        glUnmapBuffer_      = (PFNGLUNMAPBUFFERARBPROC)     getprocaddress("glUnmapBufferARB");
        glBufferData_       = (PFNGLBUFFERDATAARBPROC)      getprocaddress("glBufferDataARB");
        glBufferSubData_    = (PFNGLBUFFERSUBDATAARBPROC)   getprocaddress("glBufferSubDataARB");
        glDeleteBuffers_    = (PFNGLDELETEBUFFERSARBPROC)   getprocaddress("glDeleteBuffersARB");
        glGetBufferSubData_ = (PFNGLGETBUFFERSUBDATAARBPROC)getprocaddress("glGetBufferSubDataARB");
    }

    if(strstr(exts, "GL_EXT_draw_range_elements"))
    {
        glDrawRangeElements_ = (PFNGLDRAWRANGEELEMENTSEXTPROC)getprocaddress("glDrawRangeElementsEXT");
        hasDRE = true;
        if(dbgexts) conoutf("\frUsing GL_EXT_draw_range_elements extension.");
    }

    if(strstr(exts, "GL_EXT_multi_draw_arrays"))
    {
        glMultiDrawArrays_   = (PFNGLMULTIDRAWARRAYSEXTPROC)  getprocaddress("glMultiDrawArraysEXT");
        glMultiDrawElements_ = (PFNGLMULTIDRAWELEMENTSEXTPROC)getprocaddress("glMultiDrawElementsEXT");
        hasMDA = true;
        if(dbgexts) conoutf("\frUsing GL_EXT_multi_draw_arrays extension.");
    }

#ifdef __APPLE__
    // floating point FBOs not fully supported until 10.5
    if(osversion>=0x1050)
#endif
    if(strstr(exts, "GL_ARB_texture_float") || strstr(exts, "GL_ATI_texture_float"))
    {
        hasTF = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_texture_float extension");
        shadowmap = 1;
        extern int smoothshadowmappeel;
        smoothshadowmappeel = 1;
    }

    if(strstr(exts, "GL_NV_float_buffer"))
    {
        hasNVFB = true;
        if(dbgexts) conoutf("\frUsing GL_NV_float_buffer extension.");
    }

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
        if(dbgexts) conoutf("\frUsing GL_EXT_framebuffer_object extension.");

        if(strstr(exts, "GL_EXT_framebuffer_blit"))
        {
            glBlitFramebuffer_     = (PFNGLBLITFRAMEBUFFEREXTPROC)        getprocaddress("glBlitFramebufferEXT");
            hasFBB = true;
            if(dbgexts) conoutf("\frUsing GL_EXT_framebuffer_blit extension.");
        }
    }
    else conoutf("\frWARNING: No framebuffer object support. (reflective water may be slow)");

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
            if(dbgexts) conoutf("\frUsing GL_ARB_occlusion_query extension.");
#if defined(__APPLE__) && SDL_BYTEORDER == SDL_BIG_ENDIAN
            if(strstr(vendor, "ATI") && (osversion<0x1050)) ati_oq_bug = 1;
#endif
            if(ati_oq_bug) conoutf("\frWARNING: Using ATI occlusion query bug workaround. (use \"/ati_oq_bug 0\" to disable if unnecessary)");
        }
    }
    if(!hasOQ)
    {
        conoutf("\frWARNING: No occlusion query support! (large maps may be SLOW)");
        zpass = 0;
        extern int vacubesize;
        vacubesize = 64;
        waterreflect = 0;
    }

    extern int reservedynlighttc, reserveshadowmaptc, maxtexsize, batchlightmaps, ffdynlights;
    if(strstr(vendor, "ATI"))
    {
        floatvtx = 1;
        //conoutf("\frWARNING: ATI cards may show garbage in skybox. (use \"/ati_skybox_bug 1\" to fix)");

        reservedynlighttc = 2;
        reserveshadowmaptc = 3;
        minimizetcusage = 1;
        emulatefog = 1;
        extern int depthfxprecision;
        if(hasTF) depthfxprecision = 1;

        //ati_texgen_bug = 1;
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
        ffdynlights = 0;

        if(!hasOQ) waterrefract = 0;

#ifdef __APPLE__
        apple_vp_bug = 1;
#endif
    }
    else if(strstr(vendor, "Tungsten") || strstr(vendor, "Mesa") || strstr(vendor, "DRI") || strstr(vendor, "Microsoft") || strstr(vendor, "S3 Graphics"))
    {
        avoidshaders = 1;
        floatvtx = 1;
        maxtexsize = 256;
        reservevpparams = 20;
        batchlightmaps = 0;
        ffdynlights = 0;

        if(!hasOQ) waterrefract = 0;
    }
    //if(floatvtx) conoutf("\frWARNING: Using floating point vertexes. (use \"/floatvtx 0\" to disable)");

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
                if(apple_glsldepth_bug) conoutf("\frWARNING: Using Apple GLSL depth bug workaround. (use \"/apple_glsldepth_bug 0\" to disable if unnecessary");
            }
        }

        if(strstr(vendor, "ATI")) ati_dph_bug = 1;
        else if(strstr(vendor, "Tungsten")) mesa_program_bug = 1;

#ifdef __APPLE__
        if(osversion>=0x1050) // fixed in 1055 for some hardware.. but not all.
        {
            apple_ff_bug = 1;
            conoutf("\frWARNING: Using Leopard ARB_position_invariant bug workaround. (use \"/apple_ff_bug 0\" to disable if unnecessary)");
        }
#endif

        extern int matskel;
        if(!avoidshaders) matskel = 0;
    }

    if(strstr(exts, "GL_NV_vertex_program2_option")) { usevp2 = 1; hasVP2 = true; }
    if(strstr(exts, "GL_NV_vertex_program3")) { usevp3 = 1; hasVP3 = true; }

    if(strstr(exts, "GL_EXT_gpu_program_parameters"))
    {
        glProgramEnvParameters4fv_   = (PFNGLPROGRAMENVPARAMETERS4FVEXTPROC)  getprocaddress("glProgramEnvParameters4fvEXT");
        glProgramLocalParameters4fv_ = (PFNGLPROGRAMLOCALPARAMETERS4FVEXTPROC)getprocaddress("glProgramLocalParameters4fvEXT");
        hasPP = true;
        if(dbgexts) conoutf("\frUsing GL_EXT_gpu_program_parameters extension.");
    }

    if(strstr(exts, "GL_EXT_texture_rectangle") || strstr(exts, "GL_ARB_texture_rectangle"))
    {
        usetexrect = 1;
        hasTR = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_texture_rectangle extension.");
    }
    else if(hasMT && hasVP && hasFP) conoutf("\frWARNING: No texture rectangle support. (no full screen shaders)");

    if(strstr(exts, "GL_EXT_packed_depth_stencil") || strstr(exts, "GL_NV_packed_depth_stencil"))
    {
        hasDS = true;
        if(dbgexts) conoutf("\frUsing GL_EXT_packed_depth_stencil extension.");
    }

    if(strstr(exts, "GL_EXT_blend_minmax"))
    {
        glBlendEquation_ = (PFNGLBLENDEQUATIONEXTPROC) getprocaddress("glBlendEquationEXT");
        hasBE = true;
        if(strstr(vendor, "ATI")) ati_minmax_bug = 1;
        if(dbgexts) conoutf("\frUsing GL_EXT_blend_minmax extension.");
    }

    if(strstr(exts, "GL_EXT_blend_color"))
    {
        glBlendColor_ = (PFNGLBLENDCOLOREXTPROC) getprocaddress("glBlendColorEXT");
        hasBC = true;
        if(dbgexts) conoutf("\frUsing GL_EXT_blend_color extension.");
    }

    if(strstr(exts, "GL_ARB_texture_cube_map"))
    {
        GLint val;
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &val);
        hwcubetexsize = val;
        hasCM = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_texture_cube_map extension.");
    }
    else conoutf("\frWARNING: No cube map texture support. (no reflective glass)");

    extern int usenp2;
    if(strstr(exts, "GL_ARB_texture_non_power_of_two"))
    {
        hasNP2 = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_texture_non_power_of_two extension.");
    }
    else if(usenp2) conoutf("\frWARNING: Non-power-of-two textures not supported!");

    if(strstr(exts, "GL_ARB_texture_compression") && strstr(exts, "GL_EXT_texture_compression_s3tc"))
    {
        glCompressedTexImage3D_ =    (PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)   getprocaddress("glCompressedTexImage3DARB");
        glCompressedTexImage2D_ =    (PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)   getprocaddress("glCompressedTexImage2DARB");
        glCompressedTexImage1D_ =    (PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)   getprocaddress("glCompressedTexImage1DARB");
        glCompressedTexSubImage3D_ = (PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)getprocaddress("glCompressedTexSubImage3DARB");
        glCompressedTexSubImage2D_ = (PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)getprocaddress("glCompressedTexSubImage2DARB");
        glCompressedTexSubImage1D_ = (PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)getprocaddress("glCompressedTexSubImage1DARB");
        glGetCompressedTexImage_ =   (PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)  getprocaddress("glGetCompressedTexImageARB");

        hasTC = true;
        if(dbgexts) conoutf("\frUsing GL_EXT_texture_compression_s3tc extension.");
    }

    if(strstr(exts, "GL_EXT_texture_filter_anisotropic"))
    {
       GLint val;
       glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &val);
       hwmaxaniso = val;
       hasAF = true;
       if(dbgexts) conoutf("\frUsing GL_EXT_texture_filter_anisotropic extension.");
    }

    if(strstr(exts, "GL_SGIS_generate_mipmap"))
    {
        hasGM = true;
        if(dbgexts) conoutf("\frUsing GL_SGIS_generate_mipmap extension.");
    }

    if(strstr(exts, "GL_ARB_depth_texture"))
    {
        hasSGIDT = hasDT = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_depth_texture extension.");
    }
    else if(strstr(exts, "GL_SGIX_depth_texture"))
    {
        hasSGIDT = true;
        if(dbgexts) conoutf("\frUsing GL_SGIX_depth_texture extension.");
    }

    if(strstr(exts, "GL_ARB_shadow"))
    {
        hasSGISH = hasSH = true;
        if(strstr(vendor, "NVIDIA")) hasNVPCF = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_shadow extension.");
    }
    else if(strstr(exts, "GL_SGIX_shadow"))
    {
        hasSGISH = true;
        if(dbgexts) conoutf("\frUsing GL_SGIX_shadow extension.");
    }

    if(strstr(exts, "GL_EXT_rescale_normal"))
    {
        hasRN = true;
        if(dbgexts) conoutf("\frUsing GL_EXT_rescale_normal extension.");
    }

    if(!hasSGIDT && !hasSGISH) shadowmap = 0;

    if(strstr(exts, "GL_EXT_gpu_shader4") && !avoidshaders)
    {
        // on DX10 or above class cards (i.e. GF8 or RadeonHD) enable expensive features
        extern int grass, glare, maxdynlights, depthfxsize, depthfxrect, depthfxfilter, blurdepthfx;
        grass = 1;
        if(hasOQ)
        {
            waterfallrefract = 1;
            glare = 1;
            maxdynlights = MAXDYNLIGHTS;
            if(hasTR)
            {
                depthfxsize = 10;
                depthfxrect = 1;
                depthfxfilter = 0;
                blurdepthfx = 0;
            }
        }
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
	glViewport(0, 0, w, h);
	glClearColor(0, 0, 0, 0);
	glClearDepth(1);
	glDepthFunc(GL_LESS);
	glDisable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);


	glDisable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glHint(GL_FOG_HINT, GL_NICEST);
	GLfloat fogcolor[4] = { 0, 0, 0, 0 };
	glFogfv(GL_FOG_COLOR, fogcolor);


	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

	glCullFace(GL_FRONT);
	glDisable(GL_CULL_FACE);

#ifdef __APPLE__
    if(sdl_backingstore_bug)
    {
        if(fsaa)
        {
            sdl_backingstore_bug = 1;
            // since SDL doesn't add kCGLPFABackingStore to the pixelformat and so it isn't guaranteed to be preserved - only manifests when using fsaa?
            //conoutf(CON_WARN, "WARNING: Using SDL backingstore workaround. (use \"/sdl_backingstore_bug 0\" to disable if unnecessary)");
        }
        else sdl_backingstore_bug = -1;
    }
#endif

    extern int useshaders;
    if(!useshaders || (useshaders<0 && avoidshaders) || !hasMT || !hasVP || !hasFP)
    {
        if(!hasMT || !hasVP || !hasFP) conoutf("\frWARNING: No shader support! Using fixed-function fallback. (no fancy visuals for you)");
        else if(useshaders<0 && !hasTF) conoutf("\frWARNING: Disabling shaders for extra performance. (use \"/shaders 1\" to enable shaders if desired)");
        renderpath = R_FIXEDFUNCTION;
        conoutf("\fmRendering using the OpenGL fixed-function path.");
        if(ati_texgen_bug) conoutf("\frWARNING: Using ATI texgen bug workaround. (use \"/ati_texgen_bug 0\" to disable if unnecessary)");
        if(nvidia_texgen_bug) conoutf("\frWARNING: Using NVIDIA texgen bug workaround. (use \"/nvidia_texgen_bug 0\" to disable if unnecessary)");
    }
    else
    {
        renderpath = hasGLSL ? R_GLSLANG : R_ASMSHADER;
        if(renderpath==R_GLSLANG) conoutf("\fmRendering using the OpenGL GLSL shader path.");
        else conoutf("\fmRendering using the OpenGL assembly shader path.");
    }

    if(fsaa) glEnable(GL_MULTISAMPLE);

    inittmus();
    setuptexcompress();
}

void cleanupgl()
{
    if(glIsEnabled(GL_MULTISAMPLE)) glDisable(GL_MULTISAMPLE);

    extern int nomasks, nolights, nowater;
    nomasks = nolights = nowater = 0;
}

VAR(wireframe, 0, 0, 1);

physent camera, *camera1 = &camera;
vec worldpos, camerapos, camdir, camright, camup;

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
int xtraverts, xtravertsva;

VARW(fog, 16, 4000, INT_MAX-1);
HVARW(fogcolour, 0, 0x8099B3, 0xFFFFFF);

void vecfromcursor(float x, float y, float z, vec &dir)
{
    vec4 dir1, dir2;
    invmvpmatrix.transform(vec(x*2-1, 1-2*y, z*2-1), dir1);
    invmvpmatrix.transform(vec(x*2-1, 1-2*y, -1), dir2);
    dir = vec(dir1.x, dir1.y, dir1.z).div(dir1.w);
    dir.sub(vec(dir2.x, dir2.y, dir2.z).div(dir2.w));
    dir.normalize();
}

void vectocursor(vec &v, float &x, float &y, float &z)
{
    vec4 screenpos;
    mvpmatrix.transform(v, screenpos);

	x = screenpos.x/screenpos.w*0.5f + 0.5f;
	y = 0.5f - screenpos.y/screenpos.w*0.5f;
	z = screenpos.z/screenpos.w*0.5f + 0.5f;
}

FVAR(nearplane, 1e-3f, 0.54f, 1e3f);

void project(float fovy, float aspect, int farplane, bool flipx, bool flipy, bool swapxy, float zscale)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if(swapxy) glRotatef(90, 0, 0, 1);
    if(flipx || flipy!=swapxy || zscale!=1) glScalef(flipx ? -1 : 1, flipy!=swapxy ? -1 : 1, zscale);
    GLdouble ydist = nearplane * tan(fovy/2*RAD), xdist = ydist * aspect;
    glFrustum(-xdist, xdist, -ydist, ydist, nearplane, farplane);
    glMatrixMode(GL_MODELVIEW);
}

VAR(reflectclip, 0, 6, 64);

glmatrixf clipmatrix;

void genclipmatrix(float a, float b, float c, float d)
{
    // transform the clip plane into camera space
    float clip[4];
    loopi(4) clip[i] = a*invmvmatrix[i*4 + 0] + b*invmvmatrix[i*4 + 1] + c*invmvmatrix[i*4 + 2] + d*invmvmatrix[i*4 + 3];

    float x = ((clip[0]<0 ? -1 : (clip[0]>0 ? 1 : 0)) + projmatrix[8]) / projmatrix[0],
          y = ((clip[1]<0 ? -1 : (clip[1]>0 ? 1 : 0)) + projmatrix[9]) / projmatrix[5],
          w = (1 + projmatrix[10]) / projmatrix[14],
          scale = 2 / (x*clip[0] + y*clip[1] - clip[2] + w*clip[3]);
    clipmatrix = projmatrix;
    clipmatrix[2] = clip[0]*scale;
    clipmatrix[6] = clip[1]*scale;
    clipmatrix[10] = clip[2]*scale + 1.0f;
    clipmatrix[14] = clip[3]*scale;
}

void setclipmatrix()
{
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(clipmatrix.v);
    glMatrixMode(GL_MODELVIEW);
}

void undoclipmatrix()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}

FVAR(polygonoffsetfactor, -1e4f, -3.0f, 1e4f);
FVAR(polygonoffsetunits, -1e4f, -3.0f, 1e4f);
FVAR(depthoffset, -1e4f, 0.01f, 1e4f);

void enablepolygonoffset(GLenum type)
{
    if(!depthoffset)
    {
        glPolygonOffset(polygonoffsetfactor, polygonoffsetunits);
        glEnable(type);
        return;
    }

    bool clipped = reflectz < 1e15f && reflectclip;

    glmatrixf offsetmatrix = clipped ? clipmatrix : projmatrix;
    offsetmatrix[14] += depthoffset * projmatrix[10];

    glMatrixMode(GL_PROJECTION);
    if(!clipped) glPushMatrix();
    glLoadMatrixf(offsetmatrix.v);
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
    if(clipped) glLoadMatrixf(clipmatrix.v);
    else glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void calcspherescissor(const vec &center, float size, float &sx1, float &sy1, float &sx2, float &sy2)
{
    vec worldpos(center);
    if(reflecting) worldpos.z = 2*reflectz - worldpos.z;
    vec e(mvmatrix.transformx(worldpos),
          mvmatrix.transformy(worldpos),
          mvmatrix.transformz(worldpos));
    float zz = e.z*e.z, xx = e.x*e.x, yy = e.y*e.y, rr = size*size,
          dx = zz*(xx + zz) - rr*zz, dy = zz*(yy + zz) - rr*zz,
          focaldist = 1.0f/tan(fovy*0.5f*RAD);
    sx1 = sy1 = -1;
    sx2 = sy2 = 1;
    #define CHECKPLANE(c, dir, focaldist, low, high) \
    do { \
        float nc = (size*e.c dir drt)/(c##c + zz), \
              nz = (size - nc*e.c)/e.z, \
              pz = (c##c + zz - rr)/(e.z - nz/nc*e.c); \
        if(pz < 0) \
        { \
            float c = nz*(focaldist)/nc, \
                  pc = -pz*nz/nc; \
            if(pc < e.c) low = c; \
            else if(pc > e.c) high = c; \
        } \
    } while(0)
    if(dx > 0)
    {
        float drt = sqrt(dx);
        CHECKPLANE(x, -, focaldist/aspect, sx1, sx2);
        CHECKPLANE(x, +, focaldist/aspect, sx1, sx2);
    }
    if(dy > 0)
    {
        float drt = sqrt(dy);
        CHECKPLANE(y, -, focaldist, sy1, sy2);
        CHECKPLANE(y, +, focaldist, sy1, sy2);
    }
}

static int scissoring = 0;
static GLint oldscissor[4];

int pushscissor(float sx1, float sy1, float sx2, float sy2)
{
    scissoring = 0;

    if(sx1 <= -1 && sy1 <= -1 && sx2 >= 1 && sy2 >= 1) return 0;

    sx1 = max(sx1, -1.0f);
    sy1 = max(sy1, -1.0f);
    sx2 = min(sx2, 1.0f);
    sy2 = min(sy2, 1.0f);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    int sx = viewport[0] + int(floor((sx1+1)*0.5f*viewport[2])),
        sy = viewport[1] + int(floor((sy1+1)*0.5f*viewport[3])),
        sw = viewport[0] + int(ceil((sx2+1)*0.5f*viewport[2])) - sx,
        sh = viewport[1] + int(ceil((sy2+1)*0.5f*viewport[3])) - sy;
    if(sw <= 0 || sh <= 0) return 0;

    if(glIsEnabled(GL_SCISSOR_TEST))
    {
        glGetIntegerv(GL_SCISSOR_BOX, oldscissor);
        sw += sx;
        sh += sy;
        sx = max(sx, int(oldscissor[0]));
        sy = max(sy, int(oldscissor[1]));
        sw = min(sw, int(oldscissor[0] + oldscissor[2])) - sx;
        sh = min(sh, int(oldscissor[1] + oldscissor[3])) - sy;
        if(sw <= 0 || sh <= 0) return 0;
        scissoring = 2;
    }
    else scissoring = 1;

    glScissor(sx, sy, sw, sh);
    if(scissoring<=1) glEnable(GL_SCISSOR_TEST);

    return scissoring;
}

void popscissor()
{
    if(scissoring>1) glScissor(oldscissor[0], oldscissor[1], oldscissor[2], oldscissor[3]);
    else if(scissoring) glDisable(GL_SCISSOR_TEST);
    scissoring = 0;
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
    switch(fogmat)
    {
        case MAT_WATER:
            loopk(3) fogc[k] += blend*watercolor[k]/255.0f;
            end += logblend*min(fog, max(waterfog*4, 32));
            break;

        case MAT_LAVA:
            loopk(3) fogc[k] += blend*lavacolor[k]/255.0f;
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

bool dopostfx = false;

void invalidatepostfx()
{
    dopostfx = false;
}

static void blendfogoverlay(int fogmat, float blend, float *overlay)
{
    float maxc;
    switch(fogmat)
    {
        case MAT_WATER:
            maxc = max(watercolor[0], max(watercolor[1], watercolor[2]));
            loopk(3) overlay[k] += blend*max(0.4f, watercolor[k]/min(32.0f + maxc*7.0f/8.0f, 255.0f));
            break;

        case MAT_LAVA:
            maxc = max(lavacolor[0], max(lavacolor[1], lavacolor[2]));
            loopk(3) overlay[k] += blend*max(0.4f, lavacolor[k]/min(32.0f + maxc*7.0f/8.0f, 255.0f));
            break;

        default:
            loopk(3) overlay[k] += blend;
            break;
    }
}

void drawfogoverlay(int fogmat, float fogblend, int abovemat)
{
    notextureshader->set();
    glDisable(GL_TEXTURE_2D);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    float overlay[3] = { 0, 0, 0 };
    blendfogoverlay(fogmat, fogblend, overlay);
    blendfogoverlay(abovemat, 1-fogblend, overlay);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3fv(overlay);
    glBegin(GL_QUADS);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glVertex2f(1, 1);
    glVertex2f(-1, 1);
    glEnd();
    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glEnable(GL_TEXTURE_2D);
    defaultshader->set();
}

bool renderedgame = false, renderedavatar = false;

void rendergame()
{
    game::render();
    if(!shadowmapping) renderedgame = true;
}

void renderavatar(bool early)
{
    game::renderavatar(early);
}

extern void viewproject(float zscale = 1);

VARP(skyboxglare, 0, 1, 1);
FVAR(firstpersondepth, 0, 0.5f, 1);

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
    renderparticles();

    if(game::thirdpersonview()) renderavatar(false);
    else
    {
        viewproject(firstpersondepth);
        renderavatar(false);
        viewproject();
    }

    glFogf(GL_FOG_START, oldfogstart);
    glFogf(GL_FOG_END, oldfogend);
    glFogfv(GL_FOG_COLOR, oldfogcolor);

    refracting = 0;
    glaring = false;
}

VARP(reflectmms, 0, 1, 1);

void drawreflection(float z, bool refract, bool clear)
{
	float fogc[4] = { watercolor[0]/256.0f, watercolor[1]/256.0f, watercolor[2]/256.0f, 1.0f };

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
        genclipmatrix(0, 0, refracting>0 ? 1 : -1, refracting>0 ? -zclip : zclip);
        setclipmatrix();
    }


    renderreflectedgeom(refracting<0 && z>=0 && caustics, fogging);

    if(reflecting || refracting>0 || z<0)
    {
        if(fading) glColorMask(COLORMASK, GL_TRUE);
        if(reflectclip && z>=0) undoclipmatrix();
        drawskybox(farplane, false);
        if(reflectclip && z>=0) setclipmatrix();
        if(fading) glColorMask(COLORMASK, GL_FALSE);
    }
    else if(fading) glColorMask(COLORMASK, GL_FALSE);

    if(renderpath!=R_FIXEDFUNCTION && fogging) setfogplane(1, z);
    renderdecals();

    if(reflectmms) renderreflectedmapmodels();
    rendergame();

    if(renderpath!=R_FIXEDFUNCTION && fogging) setfogplane(1, z);
    if(refracting) rendergrass();
    rendermaterials();
    renderparticles();

    if(game::thirdpersonview() || reflecting) renderavatar(false);

    if(fading) glColorMask(COLORMASK, GL_TRUE);

    if(renderpath!=R_FIXEDFUNCTION && fogging) setfogplane();

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

void drawcubemap(int size, int level, const vec &o, float yaw, float pitch, bool flipx, bool flipy, bool swapxy)
{
	float fovx = 90.f, fovy = 90.f, aspect = 1.f;
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

    updatedynlights();

    int fogmat = lookupmaterial(camera1->o)&MATF_VOLUME, abovemat = MAT_AIR;
    float fogblend = 1.0f, causticspass = 0.0f;
    if(fogmat==MAT_WATER || fogmat==MAT_LAVA)
    {
        float z = findsurface(fogmat, camera1->o, abovemat) - WATER_OFFSET;
        if(camera1->o.z < z + 1) fogblend = min(z + 1 - camera1->o.z, 1.0f);
        else fogmat = abovemat;
        if(level && caustics && fogmat==MAT_WATER && camera1->o.z < z)
            causticspass = renderpath==R_FIXEDFUNCTION ? 1.0f : min(z - camera1->o.z, 1.0f);
    }
    else
    {
    	fogmat = MAT_AIR;
    }
    setfog(fogmat, fogblend, abovemat);
    if(level && fogmat != MAT_AIR)
    {
        float blend = abovemat==MAT_AIR ? fogblend : 1.0f;
        fovy += blend*sinf(lastmillis/1000.0)*2.0f;
        aspect += blend*sinf(lastmillis/1000.0+PI)*0.1f;
    }

	int farplane = hdr.worldsize*2;

    project(fovy, aspect, farplane, flipx, flipy, swapxy);
	transplayer();

    glEnable(GL_FOG);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

	xtravertsva = xtraverts = glde = gbatches = 0;

    if(level && !hasFBO)
    {
        if(dopostfx)
        {
            drawglaretex();
            drawdepthfxtex();
            drawreflections();
        }
        else dopostfx = true;
    }

	visiblecubes(fovx, fovy);

	if(level && shadowmap && !hasFBO) rendershadowmap();

	glClear(GL_DEPTH_BUFFER_BIT);

	if(limitsky()) drawskybox(farplane, true);
	rendergeom(level ? causticspass : 0);
	if(level) queryreflections();
    if(level >= 2) generategrass();
    if(!limitsky()) drawskybox(farplane, false);
    if(level >= 2) renderdecals();

	rendermapmodels();

	if(level >= 2)
	{
		rendergame();
	    if(hasFBO)
	    {
	        drawglaretex();
	        drawdepthfxtex();
	        drawreflections();
	    }
	    renderwater();
        rendergrass();
		rendermaterials();
		renderparticles();
    }

	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

	if(level >= 2) addglare();

	glDisable(GL_TEXTURE_2D);

	camera1 = oldcamera;
    envmapping = false;
}

VARP(scr_virtw, 0, 1024, INT_MAX-1);
VARP(scr_virth, 0, 768, INT_MAX-1);
VARP(scr_minw, 0, 640, INT_MAX-1);
VARP(scr_minh, 0, 480, INT_MAX-1);

void getscreenres(int &w, int &h)
{
    float wk = 1, hk = 1;
    if(w < scr_virtw) wk = float(scr_virtw)/w;
    if(h < scr_virth) hk = float(scr_virth)/h;
    wk = hk = max(wk, hk);
    w = int(ceil(w*wk));
    h = int(ceil(h*hk));
}

void gettextres(int &w, int &h)
{
	if(w < scr_minw || h < scr_minh)
	{
		if(scr_minw > w*scr_minh/h)
		{
			h = h*scr_minw/w;
			w = scr_minw;
		}
		else
		{
			w = w*scr_minh/h;
			h = scr_minh;
		}
	}
}

const char *loadback = "textures/loadback", *loadbackinfo = "";

void loadbackground(int w, int h, const char *name, Texture *t)
{
	float cx = 0.5f*w, cy = (0.5f*(h-32))+32, // hacked for renderprogress
		  aw = (h-32)*4.0f/3.0f, ah = (h-32);
	if(aw > w)
	{
		aw = w;
		ah = w*3.0f/4.0f;
	}
	glClearColor(0.f, 0.f, 0.f, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glColor3f(1, 1, 1);
	if(name && *name)
	{
		settexture(name);
		glBegin(GL_QUADS);

		glTexCoord2f(0, 0); glVertex2f(cx-aw/2, cy-ah/2);
		glTexCoord2f(1, 0); glVertex2f(cx+aw/2, cy-ah/2);
		glTexCoord2f(1, 1); glVertex2f(cx+aw/2, cy+ah/2);
		glTexCoord2f(0, 1); glVertex2f(cx-aw/2, cy+ah/2);

		if(w > aw)
		{
			glTexCoord2f(0, 0); glVertex2f(0, cy-ah/2);
			glTexCoord2f(0, 0); glVertex2f(cx-aw/2, cy-ah/2);
			glTexCoord2f(0, 1); glVertex2f(cx-aw/2, cy+ah/2);
			glTexCoord2f(0, 1); glVertex2f(0, cy+ah/2);

			glTexCoord2f(1, 0); glVertex2f(cx+aw/2, cy-ah/2);
			glTexCoord2f(1, 0); glVertex2f(w, cy-ah/2);
			glTexCoord2f(1, 1); glVertex2f(w, cy+ah/2);
			glTexCoord2f(1, 1); glVertex2f(cx+aw/2, cy+ah/2);
		}

		if(h > ah)
		{
			glTexCoord2f(0, 0); glVertex2f(cx-aw/2, 0);
			glTexCoord2f(1, 0); glVertex2f(cx+aw/2, 0);
			glTexCoord2f(1, 0); glVertex2f(cx+aw/2, cy-ah/2);
			glTexCoord2f(0, 0); glVertex2f(cx-aw/2, cy-ah/2);

			glTexCoord2f(0, 1); glVertex2f(cx-aw/2, cy+ah/2);
			glTexCoord2f(1, 1); glVertex2f(cx+aw/2, cy+ah/2);
			glTexCoord2f(1, 1); glVertex2f(cx+aw/2, h);
			glTexCoord2f(0, 1); glVertex2f(cx-aw/2, h);
		}

		glEnd();
	}
    settexture("textures/logo", 3);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(w-256, 2);
    glTexCoord2f(1, 0); glVertex2f(w, 2);
    glTexCoord2f(1, 1); glVertex2f(w, 66);
    glTexCoord2f(0, 1); glVertex2f(w-256, 66);
    glEnd();

    settexture("textures/cube2badge", 3);
    glBegin(GL_QUADS); // goes off the edge on purpose
    glTexCoord2f(0, 0); glVertex2f(w-108, 36);
    glTexCoord2f(1, 0); glVertex2f(w-12, 36);
    glTexCoord2f(1, 1); glVertex2f(w-12, 68);
    glTexCoord2f(0, 1); glVertex2f(w-108, 68);
    glEnd();

    if(t)
    {
        float tx1 = 378/1024.f, tx2 = 648/1024.f, txc = cx-aw/2 + aw*(tx1 + tx2)/2, txr = aw*(tx2 - tx1)/2,
              ty1 = 307/1024.f, ty2 = 658/1024.f, tyc = cy-ah/2 + ah*(ty1 + ty2)/2, tyr = ah*(ty2 - ty1)/2;

        glBindTexture(GL_TEXTURE_2D, t->id);
        glBegin(GL_TRIANGLE_FAN);
        const int subdiv = 16;
        const float solid = 0.8f;
        loopi(subdiv+1)
        {
            float angle = float(i*2*M_PI)/subdiv, rx = cosf(angle), ry = sinf(angle);
            glTexCoord2f(0.5f + solid*0.5f*rx, 0.5f + solid*0.5f*ry);
            glVertex2f(txc + solid*txr*rx, tyc + solid*tyr*ry);
        }
        glEnd();
        glBegin(GL_QUAD_STRIP);
        loopi(subdiv+1)
        {
            float angle = float(i*2*M_PI)/subdiv, rx = cosf(angle), ry = sinf(angle);
            glColor4f(1, 1, 1, 0);
            glTexCoord2f(0.5f + 0.5f*rx, 0.5f + 0.5f*ry);
            glVertex2f(txc + txr*rx, tyc + tyr*ry);
            glColor4f(1, 1, 1, 1);
            glTexCoord2f(0.5f + solid*0.5f*rx, 0.5f + solid*0.5f*ry);
            glVertex2f(txc + solid*txr*rx, tyc + solid*tyr*ry);
        }
        glEnd();
        glColor3f(1, 1, 1);
    }

	glPushMatrix();
	glScalef(1/3.0f, 1/3.0f, 1);
	if(loadbackinfo && *loadbackinfo)
		draw_textx("%s", FONTH/2, h*3-FONTH-FONTH/2, 255, 255, 255, 255, TEXT_LEFT_JUSTIFY, -1, -1, loadbackinfo);
	draw_textx("v%.2f (%s)", w*3-FONTH, h*3-FONTH*2-FONTH/2, 255, 255, 255, 255, TEXT_RIGHT_JUSTIFY, -1, -1, float(ENG_VERSION)/100.f, ENG_RELEASE);
	draw_textx("%s", w*3-FONTH/2, h*3-FONTH-FONTH/2, 255, 255, 255, 255, TEXT_RIGHT_JUSTIFY, -1, -1, ENG_URL);
	glPopMatrix();
}

string backgroundcaption = "";
Texture *backgroundmapshot = NULL;
string backgroundmapname = "";

void restorebackground()
{
    if(renderedframe) return;
    renderbackground(backgroundcaption[0] ? backgroundcaption : NULL, backgroundmapshot, backgroundmapname[0] ? backgroundmapname : NULL, true);
}

void renderbackground(const char *caption, Texture *mapshot, const char *mapname, bool restore)
{
    if(!inbetweenframes) return;

	int w = screen->w, h = screen->h;
	if(caption && mapname)
    {
        defformatstring(mapcaption)("%s - %s", mapname, caption);
        setcaption(mapcaption);
    }
    else setcaption(caption);
    smartmusic(false, true);
    getscreenres(w, h);
	gettextres(w, h);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    defaultshader->set();
	loopi(restore ? 1 : 2)
	{
		loadbackground(w, h, kidmode ? "textures/kidback" : loadback, mapshot);

        if(caption)
        {
            glPushMatrix();
            glScalef(1/3.0f, 1/3.0f, 1);
            draw_text(caption, 70, FONTH/2);
            glPopMatrix();
        }
#if 0
		if(t)
		{
			glBindTexture(GL_TEXTURE_2D, t->id);
			int sz = 256, x = (w-sz)/2, y = min(384, h-256);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(x,	y);
			glTexCoord2f(1, 0); glVertex2f(x+sz, y);
			glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
			glTexCoord2f(0, 1); glVertex2f(x,	y+sz);
			glEnd();
			settexture(guioverlaytex);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(x,	y);
			glTexCoord2f(1, 0); glVertex2f(x+sz, y);
			glTexCoord2f(1, 1); glVertex2f(x+sz, y+sz);
			glTexCoord2f(0, 1); glVertex2f(x,	y+sz);
			glEnd();
		}
#endif
        if(mapname)
        {
            glPushMatrix();
            glScalef(1/3.0f, 1/3.0f, 1);
			draw_textx("%s", (w/2)*3, (h/2)*3+FONTH, 255, 255, 255, 255, TEXT_CENTERED, -1, -1, mapname);
            glPopMatrix();
        }
		if(!restore) swapbuffers();
	}
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);

    if(!restore)
    {
        renderedframe = false;
        copystring(backgroundcaption, caption ? caption : "");
        backgroundmapshot = mapshot;
        copystring(backgroundmapname, mapname ? mapname : "");
    }
}

void drawslice(float start, float length, float x, float y, float size)
{
    float end = start + length,
          sx = cosf((start + 0.25f)*2*M_PI), sy = -sinf((start + 0.25f)*2*M_PI),
          ex = cosf((end + 0.25f)*2*M_PI), ey = -sinf((end + 0.25f)*2*M_PI);
    glBegin(GL_TRIANGLE_FAN);
    glTexCoord2f(0.5f, 0.5f); glVertex2f(x, y);

    if(start < 0.125f || start >= 0.875f) { glTexCoord2f(0.5f + 0.5f*sx/sy, 0); glVertex2f(x + sx/sy*size, y - size);  }
    else if(start < 0.375f) { glTexCoord2f(1, 0.5f - 0.5f*sy/sx); glVertex2f(x + size, y - sy/sx*size); }
    else if(start < 0.625f) { glTexCoord2f(0.5f - 0.5f*sx/sy, 1); glVertex2f(x - sx/sy*size, y + size); }
    else { glTexCoord2f(0, 0.5f + 0.5f*sy/sx); glVertex2f(x - size, y + sy/sx*size); }

    if(start <= 0.125f && end >= 0.125f) { glTexCoord2f(1, 0); glVertex2f(x + size, y - size); }
    if(start <= 0.375f && end >= 0.375f) { glTexCoord2f(1, 1); glVertex2f(x + size, y + size); }
    if(start <= 0.625f && end >= 0.625f) { glTexCoord2f(0, 1); glVertex2f(x - size, y + size); }
    if(start <= 0.875f && end >= 0.875f) { glTexCoord2f(0, 0); glVertex2f(x - size, y - size); }

    if(end < 0.125f || end >= 0.875f) { glTexCoord2f(0.5f + 0.5f*ex/ey, 0); glVertex2f(x + ex/ey*size, y - size);  }
    else if(end < 0.375f) { glTexCoord2f(1, 0.5f - 0.5f*ey/ex); glVertex2f(x + size, y - ey/ex*size); }
    else if(end < 0.625f) { glTexCoord2f(0.5f - 0.5f*ex/ey, 1); glVertex2f(x - ex/ey*size, y + size); }
    else { glTexCoord2f(0, 0.5f + 0.5f*ey/ex); glVertex2f(x - size, y + ey/ex*size); }
    glEnd();
}

void drawfadedslice(float start, float length, float x, float y, float size, float alpha, float r, float g, float b, float minsize)
{
    float end = start + length,
          sx = cosf((start + 0.25f)*2*M_PI), sy = -sinf((start + 0.25f)*2*M_PI),
          ex = cosf((end + 0.25f)*2*M_PI), ey = -sinf((end + 0.25f)*2*M_PI);

    #define SLICEVERT(ox, oy) \
    { \
        glTexCoord2f(0.5f + (ox)*0.5f, 0.5f + (oy)*0.5f); \
        glVertex2f(x + (ox)*size, y + (oy)*size); \
    }
    #define SLICESPOKE(ox, oy) \
    { \
        SLICEVERT((ox)*minsize, (oy)*minsize); \
        SLICEVERT(ox, oy); \
    }

    glBegin(GL_TRIANGLE_STRIP);
    glColor4f(r, g, b, alpha);
    if(start < 0.125f || start >= 0.875f) SLICESPOKE(sx/sy, -1)
    else if(start < 0.375f) SLICESPOKE(1, -sy/sx)
    else if(start < 0.625f) SLICESPOKE(-sx/sy, 1)
    else SLICESPOKE(-1, sy/sx)

    if(start <= 0.125f && end >= 0.125f) { glColor4f(r, g, b, alpha - alpha*(0.125f - start)/(end - start)); SLICESPOKE(1, -1) }
    if(start <= 0.375f && end >= 0.375f) { glColor4f(r, g, b, alpha - alpha*(0.375f - start)/(end - start)); SLICESPOKE(1, 1) }
    if(start <= 0.625f && end >= 0.625f) { glColor4f(r, g, b, alpha - alpha*(0.625f - start)/(end - start)); SLICESPOKE(-1, 1) }
    if(start <= 0.875f && end >= 0.875f) { glColor4f(r, g, b, alpha - alpha*(0.875f - start)/(end - start)); SLICESPOKE(-1, -1) }

    glColor4f(r, g, b, 0);
    if(end < 0.125f || end >= 0.875f) SLICESPOKE(ex/ey, -1)
    else if(end < 0.375f) SLICESPOKE(1, -ey/ex)
    else if(end < 0.625f) SLICESPOKE(-ex/ey, 1)
    else SLICESPOKE(-1, ey/ex)
    glEnd();
}

extern int actualvsync;
int lastoutofloop = 0;
float loadprogress = 0;
void renderprogress(float bar1, const char *text1, float bar2, const char *text2, GLuint tex)	// also used during loading
{
	if(!inbetweenframes || ((actualvsync > 0 || verbose) && lastoutofloop && SDL_GetTicks()-lastoutofloop < 20))
		return;
	clientkeepalive();

    #ifdef __APPLE__
    interceptkey(SDLK_UNKNOWN); // keep the event queue awake to avoid 'beachball' cursor
    #endif

	if(verbose >= 4)
	{
		if (text2) conoutf("\fm%s [%.2f%%], %s [%.2f%%]", text1, bar1*100.f, text2, bar2*100.f);
		else if (text1) conoutf("\fm%s [%.2f%%]", text1, bar1*100.f);
		else conoutf("\fmprogressing [%.2f%%]", text1, bar1*100.f, text2, bar2*100.f);
	}

	int w = screen->w, h = screen->h;
    getscreenres(w, h);
	gettextres(w, h);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, w*3, h*3, 0, -1, 1);
	notextureshader->set();

	glColor3f(0, 0, 0);
	glBegin(GL_QUADS);
	glVertex2f(0,	0);
	glVertex2f(w*3, 0);
	glVertex2f(w*3, 204);
	glVertex2f(0,	204);
	glEnd();

	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	defaultshader->set();

	if(text1)
	{
		if(bar1 > 0)
		{
			glColor3f(1, 1, 1);
			settexture("textures/progress", 3);
			drawslice(0, clamp(bar1, 0.f, 1.f), 96, 96, 96);
			draw_textx("\fg%d%%", 96, 96-FONTH/2, 255, 255, 255, 255, TEXT_CENTERED, -1, -1, int(bar1*100));
		}
		else
		{
			glColor3f(1, 1, 1);
			settexture("textures/wait", 3);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(0, 0);
			glTexCoord2f(1, 0); glVertex2f(192, 0);
			glTexCoord2f(1, 1); glVertex2f(192, 192);
			glTexCoord2f(0, 1); glVertex2f(0, 192);
			glEnd();
			draw_textx("\fywait", 96, 96-FONTH/2, 255, 255, 255, 255, TEXT_CENTERED, -1, -1);
		}
		if(text2 && bar2 > 0)
			draw_textx("%s %s [\fs\fo%d%%\fS]", 192+FONTW, 96-FONTH/2, 255, 255, 255, 255, TEXT_LEFT_JUSTIFY, -1, -1, text1, text2, int(bar2*100));
		else draw_textx("%s", 192+FONTW, 96-FONTH/2, 255, 255, 255, 255, TEXT_LEFT_JUSTIFY, -1, -1, text1);
	}

	glDisable(GL_BLEND);
	if(tex)
	{
		glPushMatrix();
		glScalef(3.0f, 3.0f, 1);
		const int subdiv = 16;
		float cx = 0.5f*w, cy = (0.5f*h)+12, aw = h*4.0f/3.0f, ah = h;
		if(aw > w) { aw = w; ah = w*3.0f/4.0f; }
        float tx1 = 378/1024.f, tx2 = 648/1024.f, txc = cx-aw/2 + aw*(tx1 + tx2)/2, txr = aw*(tx2 - tx1)/2,
              ty1 = 307/1024.f, ty2 = 658/1024.f, tyc = cy-ah/2 + ah*(ty1 + ty2)/2, tyr = ah*(ty2 - ty1)/2;
		glColor3f(1, 1, 1);
		glBindTexture(GL_TEXTURE_2D, tex);
        glBegin(GL_TRIANGLE_FAN);
        loopi(subdiv+1)
        {
            float angle = float(i*2*M_PI)/subdiv, rx = cosf(angle), ry = sinf(angle);
            glTexCoord2f(0.5f + 0.5f*rx, 0.5f + 0.5f*ry);
            glVertex2f(txc + txr*rx, tyc + tyr*ry);
        }
        glEnd();
#if 0
        glBegin(GL_QUAD_STRIP);
        loopi(subdiv+1)
        {
            float angle = float(i*2*M_PI)/subdiv, rx = cosf(angle), ry = sinf(angle);
            glTexCoord2f(0.5f + 0.5f*rx, 0.5f + 0.5f*ry);
            glVertex2f(txc + txr*rx, tyc + tyr*ry);
            glTexCoord2f(0.5f + 0.5f*rx, 0.5f + 0.5f*ry);
            glVertex2f(txc + txr*rx, tyc + tyr*ry);
        }
        glEnd();
#endif
		glPopMatrix();
	}

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	swapbuffers();
	lastoutofloop = SDL_GetTicks();
	autoadjustlevel = 100;
}

glmatrixf mvmatrix, projmatrix, mvpmatrix, invmvmatrix, invmvpmatrix;

void readmatrices()
{
    glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix.v);
    glGetFloatv(GL_PROJECTION_MATRIX, projmatrix.v);

    mvpmatrix.mul(projmatrix, mvmatrix);
    invmvmatrix.invert(mvmatrix);
    invmvpmatrix.invert(mvpmatrix);
}

VARP(showhud, 0, 1, 1);
FVARP(hudblend, 0, 1.f, 1);

float cursorx = 0.5f, cursory = 0.5f;
vec cursordir(0, 0, 0);

struct framebuffercopy
{
    GLuint tex;
    GLenum target;
    int w, h;

    framebuffercopy() : tex(0), target(GL_TEXTURE_2D), w(0), h(0) {}

    void cleanup()
    {
        if(!tex) return;
        glDeleteTextures(1, &tex);
		tex = 0;
    }

    void setup()
    {
        if(tex) return;
        glGenTextures(1, &tex);
        if(hasTR)
        {
            target = GL_TEXTURE_RECTANGLE_ARB;
            w = screen->w;
            h = screen->h;
        }
        else
        {
            target = GL_TEXTURE_2D;
            for(w = 1; w < screen->w; w *= 2);
            for(h = 1; h < screen->h; h *= 2);
        }
        createtexture(tex, w, h, NULL, 3, false, GL_RGB, target);
    }

    void copy()
    {
        if(target == GL_TEXTURE_RECTANGLE_ARB)
        {
            if(w != screen->w || h != screen->h) cleanup();
        }
        else if(w < screen->w || h < screen->h || w/2 >= screen->w || h/2 >= screen->h) cleanup();
        if(!tex) setup();

        glBindTexture(target, tex);
        glCopyTexSubImage2D(target, 0, 0, 0, 0, 0, screen->w, screen->h);
    }

	void draw(float sx, float sy, float sw, float sh)
	{
		float tx = 0, ty = 0, tw = float(screen->w)/w, th = float(screen->h)/h;
		if(target == GL_TEXTURE_RECTANGLE_ARB)
		{
			glDisable(GL_TEXTURE_2D);
			glEnable(GL_TEXTURE_RECTANGLE_ARB);
			rectshader->set();
			tx *= w;
			ty *= h;
			tw *= w;
			th *= h;
		}
        glBindTexture(target, tex);
        glBegin(GL_QUADS);
        glTexCoord2f(tx,    ty);    glVertex2f(sx,    sy);
        glTexCoord2f(tx+tw, ty);    glVertex2f(sx+sw, sy);
        glTexCoord2f(tx+tw, ty+th); glVertex2f(sx+sw, sy+sh);
        glTexCoord2f(tx,    ty+th); glVertex2f(sx,    sy+sh);
        glEnd();
		if(target == GL_TEXTURE_RECTANGLE_ARB)
		{
			glEnable(GL_TEXTURE_2D);
            glDisable(GL_TEXTURE_RECTANGLE_ARB);
			defaultshader->set();
		}
	}
};

#define DTR 0.0174532925
enum { VW_NORMAL = 0, VW_MAGIC, VW_STEREO, VW_STEREO_BLEND = VW_STEREO, VW_STEREO_BLEND_REDCYAN, VW_STEREO_AVG, VW_MAX, VW_STEREO_REDCYAN = VW_MAX };
enum { VP_LEFT, VP_RIGHT, VP_MAX, VP_CAMERA = VP_MAX };

framebuffercopy views[VP_MAX];

VARFP(viewtype, VW_NORMAL, VW_NORMAL, VW_MAX, loopi(VP_MAX) views[i].cleanup());
VARP(stereoblend, 0, 50, 100);
FVARP(stereodist, 0, 0.5f, 10000);
FVARP(stereoplane, 1e-3f, 10.f, 1000);
FVARP(stereonear, 0, 3.f, 10000);

int fogmat = MAT_AIR, abovemat = MAT_AIR;
float fogblend = 1.0f, causticspass = 0.0f;

GLenum colormask[3] = { GL_TRUE, GL_TRUE, GL_TRUE };

void setcolormask(bool r, bool g, bool b)
{
	colormask[0] = r ? GL_TRUE : GL_FALSE;
	colormask[1] = g ? GL_TRUE : GL_FALSE;
	colormask[2] = b ? GL_TRUE : GL_FALSE;
}

bool needsview(int v, int targtype)
{
    switch(v)
    {
        case VW_NORMAL: return targtype == VP_CAMERA;
        case VW_MAGIC: return targtype == VP_LEFT || targtype == VP_RIGHT;
        case VW_STEREO_BLEND:
        case VW_STEREO_BLEND_REDCYAN: return targtype >= VP_LEFT && targtype <= VP_CAMERA;
        case VW_STEREO_AVG:
        case VW_STEREO_REDCYAN: return targtype == VP_LEFT || targtype == VP_RIGHT;
    }
    return false;
}

bool copyview(int v, int targtype)
{
    switch(v)
    {
        case VW_MAGIC: return targtype == VP_LEFT || targtype == VP_RIGHT;
        case VW_STEREO_BLEND:
        case VW_STEREO_BLEND_REDCYAN: return targtype == VP_RIGHT;
        case VW_STEREO_AVG: return targtype == VP_LEFT;
    }
    return false;
}

bool clearview(int v, int targtype)
{
    switch(v)
    {
        case VW_STEREO_BLEND:
        case VW_STEREO_BLEND_REDCYAN: return targtype == VP_LEFT || targtype == VP_CAMERA;
        case VW_STEREO_REDCYAN: return targtype == VP_LEFT;
    }
    return true;
}

static int curview = VP_CAMERA;

void viewproject(float zscale)
{
    if(curview != VP_LEFT && curview != VP_RIGHT) project(fovy, aspect, farplane, false, false, false, zscale);
    else
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        if(zscale != 1) glScalef(1, 1, zscale);
        float top = stereonear*tan(DTR*fovy/2), right = aspect*top, iod = stereodist/2, fs = iod*stereonear/stereoplane;
        glFrustum(curview == VP_LEFT ? -right+fs : -right-fs, curview == VP_LEFT ? right+fs : right-fs, -top, top, stereonear, farplane);
        glTranslatef(curview == VP_LEFT ? iod : -iod, 0.f, 0.f);
        glMatrixMode(GL_MODELVIEW);
    }
}

void drawnoview()
{
    xtravertsva = xtraverts = glde = gbatches = 0;

	glClearColor(0.f, 0.f, 0.f, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    int w = screen->w, h = screen->h;
    gettextres(w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);

    glEnable(GL_TEXTURE_2D);

    defaultshader->set();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    loadbackground(w, h);
    glDisable(GL_BLEND);

	if(showhud)
	{
		hud::drawhud(w, h);
		render_texture_panel(w, h);
		if(commandmillis<0) UI::render();
		hud::drawlast(w, h);
	}

    glDisable(GL_TEXTURE_2D);
}

void drawview(int targtype)
{
    curview = targtype;

	defaultshader->set();
	updatedynlights();

	setfog(fogmat, fogblend, abovemat);
  	viewproject();
	transplayer();
	if(targtype == VP_LEFT || targtype == VP_RIGHT)
	{
		if(viewtype >= VW_STEREO)
        {
            switch(viewtype)
            {
                case VW_STEREO_BLEND: setcolormask(targtype == VP_LEFT, false, targtype == VP_RIGHT); break;
                case VW_STEREO_AVG: setcolormask(targtype == VP_LEFT, true, targtype == VP_RIGHT); break;
                case VW_STEREO_BLEND_REDCYAN:
                case VW_STEREO_REDCYAN: setcolormask(targtype == VP_LEFT, targtype == VP_RIGHT, targtype == VP_RIGHT); break;
            }
            glColorMask(COLORMASK, GL_TRUE);
        }
	}

	readmatrices();

    glEnable(GL_FOG);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

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

	glClearColor(0, 0, 0, 0);
	glClear(GL_DEPTH_BUFFER_BIT|(wireframe && editmode && clearview(viewtype, targtype) ? GL_COLOR_BUFFER_BIT : 0)|(hasstencil ? GL_STENCIL_BUFFER_BIT : 0));

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if(limitsky()) drawskybox(farplane, true);

	rendergeom(causticspass);
	extern int outline, blankgeom;
	if(!wireframe && editmode && (outline || (fullbright && blankgeom))) renderoutline();

	queryreflections();
    generategrass();

	if(!limitsky()) drawskybox(farplane, false);

    renderdecals(true);

	rendermapmodels();
	rendergame();
	renderavatar(true);

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	if(hasFBO)
	{
		drawglaretex();
		drawdepthfxtex();
		drawreflections();
	}

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	renderwater();
    rendergrass();

	rendermaterials();
	renderparticles(true);

    if(game::thirdpersonview()) renderavatar(false);
    else
    {
        viewproject(0.5f);
	    renderavatar(false);
        viewproject();
    }

    if(wireframe && editmode) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

	addglare();
    if(fogmat==MAT_WATER || fogmat==MAT_LAVA) drawfogoverlay(fogmat, fogblend, abovemat);
	renderpostfx();

	glDisable(GL_TEXTURE_2D);
	notextureshader->set();
	if(editmode && showhud)
	{
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        renderblendbrush();

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		cursorupdate();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glDepthMask(GL_TRUE);
        glDisable(GL_DEPTH_TEST);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	int w = screen->w, h = screen->h;
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
	if(showhud)
	{
		hud::drawhud(w, h);
		render_texture_panel(w, h);
		if(commandmillis<0) UI::render();
		hud::drawlast(w, h);
	}
	glDisable(GL_TEXTURE_2D);

	renderedgame = false;

    if(targtype == VP_LEFT || targtype == VP_RIGHT)
    {
        if(viewtype >= VW_STEREO)
        {
            setcolormask();
            glColorMask(COLORMASK, GL_TRUE);
        }
    }
}

void gl_drawframe(int w, int h)
{
    if(connected(false) && client::ready())
    {
		fogmat = lookupmaterial(camera1->o)&MATF_VOLUME;
		causticspass = 0.f;
		if(fogmat == MAT_WATER || fogmat == MAT_LAVA)
		{
			float z = findsurface(fogmat, camera1->o, abovemat) - WATER_OFFSET;
			if(camera1->o.z < z + 1) fogblend = min(z + 1 - camera1->o.z, 1.0f);
			else fogmat = abovemat;
			if(caustics && fogmat == MAT_WATER && camera1->o.z < z)
				causticspass = renderpath==R_FIXEDFUNCTION ? 1.0f : min(z - camera1->o.z, 1.0f);

			float blend = abovemat == MAT_AIR ? fogblend : 1.0f;
			fovy += blend*sinf(lastmillis/1000.0)*2.0f;
			aspect += blend*sinf(lastmillis/1000.0+PI)*0.1f;
		}
		else fogmat = MAT_AIR;

		farplane = hdr.worldsize*2;
		project(fovy, aspect, farplane);
		transplayer();
		readmatrices();
		game::project(w, h);

		int copies = 0, oldcurtime = curtime;
		loopi(VP_MAX) if(needsview(viewtype, i))
		{
			drawview(i);
			if(copyview(viewtype, i))
			{
				views[i].copy();
				copies++;
			}
			curtime = 0;
		}
		if(needsview(viewtype, VP_CAMERA)) drawview(VP_CAMERA);
		curtime = oldcurtime;

		if(!copies) return;

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, 1, 0, 1, -1, 1);
		glDisable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);
		defaultshader->set();
		glColor3f(1.f, 1.f, 1.f);
		switch(viewtype)
		{
			case VW_MAGIC:
			{
				views[VP_LEFT].draw(0, 0, 0.5f, 1);
				views[VP_RIGHT].draw(0.5f, 0, 0.5f, 1);
				break;
			}
			case VW_STEREO_BLEND:
			case VW_STEREO_BLEND_REDCYAN:
			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				if(viewtype == VW_STEREO_BLEND) glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
				glColor4f(1.f, 1.f, 1.f, stereoblend/100.f); views[VP_RIGHT].draw(0, 0, 1, 1);
				if(viewtype == VW_STEREO_BLEND) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				glDisable(GL_BLEND);
				break;
			}
			case VW_STEREO_AVG:
			{
				glEnable(GL_BLEND);
				if(hasBC)
				{
					glBlendFunc(GL_ONE, GL_CONSTANT_COLOR_EXT);
					glBlendColor_(0.f, 0.5f, 1.f, 1.f);
				}
				else
				{
					glDisable(GL_TEXTURE_2D);
					glBlendFunc(GL_ZERO, GL_SRC_COLOR);
					glColor3f(0.f, 0.5f, 1.f);
					glBegin(GL_QUADS);
					glVertex2f(0, 0);
					glVertex2f(1, 0);
					glVertex2f(1, 1);
					glVertex2f(0, 1);
					glEnd();
					glEnable(GL_TEXTURE_2D);
					glBlendFunc(GL_ONE, GL_ONE);
				}
				glColor3f(1.f, 0.5f, 0.f);
				views[VP_LEFT].draw(0, 0, 1, 1);
				glDisable(GL_BLEND);
				break;
			}
		}
		glDisable(GL_TEXTURE_2D);
    }
    else drawnoview();
}

void usetexturing(bool on)
{
    if(on)
    {
        defaultshader->set();
        glEnable(GL_TEXTURE_2D);
    }
    else
    {
        notextureshader->set();
        glDisable(GL_TEXTURE_2D);
    }
}
