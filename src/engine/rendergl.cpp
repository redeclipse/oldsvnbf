// rendergl.cpp: core opengl rendering stuff

#include "engine.h"

bool hasVBO = false, hasDRE = false, hasOQ = false, hasTR = false, hasFBO = false, hasDS = false, hasTF = false, hasBE = false, hasBC = false, hasCM = false, hasNP2 = false, hasTC = false, hasTE = false, hasMT = false, hasD3 = false, hasAF = false, hasVP2 = false, hasVP3 = false, hasPP = false, hasMDA = false, hasTE3 = false, hasTE4 = false, hasVP = false, hasFP = false, hasGLSL = false, hasGM = false, hasNVFB = false, hasSGIDT = false, hasSGISH = false, hasDT = false, hasSH = false, hasNVPCF = false, hasRN = false, hasPBO = false, hasFBB = false, hasUBO = false, hasBUE = false, hasFC = false, hasTEX = false;
int hasstencil = 0;

VAR(0, renderpath, 1, 0, 0);

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
PFNGLACTIVETEXTUREARBPROC       glActiveTexture_        = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTexture_ = NULL;
PFNGLMULTITEXCOORD2FARBPROC  glMultiTexCoord2f_  = NULL;
PFNGLMULTITEXCOORD3FARBPROC  glMultiTexCoord3f_  = NULL;
PFNGLMULTITEXCOORD4FARBPROC  glMultiTexCoord4f_     = NULL;

// GL_ARB_vertex_program, GL_ARB_fragment_program
PFNGLGENPROGRAMSARBPROC         glGenPrograms_          = NULL;
PFNGLDELETEPROGRAMSARBPROC       glDeletePrograms_       = NULL;
PFNGLBINDPROGRAMARBPROC         glBindProgram_          = NULL;
PFNGLPROGRAMSTRINGARBPROC         glProgramString_        = NULL;
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
PFNGLGENQUERIESARBPROC      glGenQueries_       = NULL;
PFNGLDELETEQUERIESARBPROC    glDeleteQueries_    = NULL;
PFNGLBEGINQUERYARBPROC      glBeginQuery_       = NULL;
PFNGLENDQUERYARBPROC          glEndQuery_         = NULL;
PFNGLGETQUERYIVARBPROC      glGetQueryiv_       = NULL;
PFNGLGETQUERYOBJECTIVARBPROC  glGetQueryObjectiv_  = NULL;
PFNGLGETQUERYOBJECTUIVARBPROC glGetQueryObjectuiv_ = NULL;

// GL_EXT_framebuffer_object
PFNGLBINDRENDERBUFFEREXTPROC        glBindRenderbuffer_     = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC  glDeleteRenderbuffers_  = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC      glGenRenderbuffers_        = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC  glRenderbufferStorage_  = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC  glCheckFramebufferStatus_  = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC      glBindFramebuffer_      = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC    glDeleteFramebuffers_   = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC      glGenFramebuffers_      = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC    glFramebufferTexture2D_ = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbuffer_ = NULL;
PFNGLGENERATEMIPMAPEXTPROC        glGenerateMipmap_       = NULL;

// GL_EXT_framebuffer_blit
PFNGLBLITFRAMEBUFFEREXTPROC         glBlitFramebuffer_         = NULL;

// GL_ARB_shading_language_100, GL_ARB_shader_objects, GL_ARB_fragment_shader, GL_ARB_vertex_shader
PFNGLCREATEPROGRAMOBJECTARBPROC       glCreateProgramObject_      = NULL;
PFNGLDELETEOBJECTARBPROC              glDeleteObject_             = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC          glUseProgramObject_         = NULL;
PFNGLCREATESHADEROBJECTARBPROC        glCreateShaderObject_       = NULL;
PFNGLSHADERSOURCEARBPROC              glShaderSource_             = NULL;
PFNGLCOMPILESHADERARBPROC             glCompileShader_            = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC      glGetObjectParameteriv_     = NULL;
PFNGLATTACHOBJECTARBPROC              glAttachObject_             = NULL;
PFNGLGETINFOLOGARBPROC                glGetInfoLog_               = NULL;
PFNGLLINKPROGRAMARBPROC               glLinkProgram_              = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC        glGetUniformLocation_       = NULL;
PFNGLUNIFORM1FARBPROC                 glUniform1f_                = NULL;
PFNGLUNIFORM2FARBPROC                 glUniform2f_                = NULL;
PFNGLUNIFORM3FARBPROC                 glUniform3f_                = NULL;
PFNGLUNIFORM4FARBPROC                 glUniform4f_                = NULL;
PFNGLUNIFORM1FVARBPROC                glUniform1fv_               = NULL;
PFNGLUNIFORM2FVARBPROC                glUniform2fv_               = NULL;
PFNGLUNIFORM3FVARBPROC                glUniform3fv_               = NULL;
PFNGLUNIFORM4FVARBPROC                glUniform4fv_               = NULL;
PFNGLUNIFORM1IARBPROC                 glUniform1i_                = NULL;
PFNGLBINDATTRIBLOCATIONARBPROC        glBindAttribLocation_       = NULL;
PFNGLGETACTIVEUNIFORMARBPROC          glGetActiveUniform_         = NULL;

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

// GL_ARB_uniform_buffer_object
PFNGLGETUNIFORMINDICESPROC       glGetUniformIndices_       = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC     glGetActiveUniformsiv_     = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC    glGetUniformBlockIndex_    = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glGetActiveUniformBlockiv_ = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC     glUniformBlockBinding_     = NULL;
PFNGLBINDBUFFERBASEPROC          glBindBufferBase_          = NULL;
PFNGLBINDBUFFERRANGEPROC         glBindBufferRange_         = NULL;

// GL_EXT_bindable_uniform
PFNGLUNIFORMBUFFEREXTPROC        glUniformBuffer_        = NULL;
PFNGLGETUNIFORMBUFFERSIZEEXTPROC glGetUniformBufferSize_ = NULL;
PFNGLGETUNIFORMOFFSETEXTPROC     glGetUniformOffset_     = NULL;

// GL_EXT_fog_coord
PFNGLFOGCOORDPOINTEREXTPROC glFogCoordPointer_ = NULL;

void *getprocaddress(const char *name)
{
    return SDL_GL_GetProcAddress(name);
}

VAR(IDF_PERSIST, ati_skybox_bug, 0, 0, 1);
VAR(0, ati_oq_bug, 0, 0, 1);
VAR(0, ati_minmax_bug, 0, 0, 1);
VAR(0, ati_dph_bug, 0, 0, 1);
VAR(0, ati_line_bug, 0, 0, 1);
VAR(0, ati_cubemap_bug, 0, 0, 1);
VAR(0, ati_ubo_bug, 0, 0, 1);
VAR(0, nvidia_scissor_bug, 0, 0, 1);
VAR(0, apple_glsldepth_bug, 0, 0, 1);
VAR(0, apple_ff_bug, 0, 0, 1);
VAR(0, apple_vp_bug, 0, 0, 1);
VAR(0, sdl_backingstore_bug, -1, 0, 1);
VAR(0, intel_quadric_bug, 0, 0, 1);
VAR(0, mesa_program_bug, 0, 0, 1);
VAR(0, avoidshaders, 1, 0, 0);
VAR(0, minimizetcusage, 1, 0, 0);
VAR(0, emulatefog, 1, 0, 0);
VAR(0, usevp2, 1, 0, 0);
VAR(0, usevp3, 1, 0, 0);
VAR(0, usetexrect, 1, 0, 0);
VAR(0, hasglsl, 1, 0, 0);
VAR(0, useubo, 1, 0, 0);
VAR(0, usebue, 1, 0, 0);
VAR(0, rtscissor, 0, 1, 1);
VAR(0, blurtile, 0, 1, 1);
VAR(0, rtsharefb, 0, 1, 1);

static bool checkseries(const char *s, int low, int high)
{
    while(*s && !isdigit(*s)) ++s;
    if(!*s) return false;
    int n = 0;
    while(isdigit(*s)) n = n*10 + (*s++ - '0');
    return n >= low && n < high;
}

VAR(0, dbgexts, 0, 0, 1);

void gl_checkextensions()
{
    const char *vendor = (const char *)glGetString(GL_VENDOR);
    const char *exts = (const char *)glGetString(GL_EXTENSIONS);
    const char *renderer = (const char *)glGetString(GL_RENDERER);
    const char *version = (const char *)glGetString(GL_VERSION);
    conoutf("renderer: %s (%s)", renderer, vendor);
    conoutf("driver: %s", version);

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
        if(strstr(exts, "GL_ARB_texture_env_crossbar")) hasTEX = true;
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
    else conoutf("\frWARNING: No multitexture extension");

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
        extern int vacubesize;
        vacubesize = 64;
        waterreflect = 0;
    }

    extern int reservedynlighttc, reserveshadowmaptc, maxtexsize, batchlightmaps, ffdynlights;
    if(strstr(vendor, "ATI"))
    {
        //conoutf("\frWARNING: ATI cards may show garbage in skybox. (use \"/ati_skybox_bug 1\" to fix)");

        reservedynlighttc = 2;
        reserveshadowmaptc = 3;
        minimizetcusage = 1;
        emulatefog = 1;
        extern int depthfxprecision;
        if(hasTF) depthfxprecision = 1;

    }
    else if(strstr(vendor, "NVIDIA"))
    {
        reservevpparams = 10;
        rtsharefb = 0; // work-around for strange driver stalls involving when using many FBOs
        extern int filltjoints;
        if(!strstr(exts, "GL_EXT_gpu_shader4")) filltjoints = 0; // DX9 or less NV cards seem to not cause many sparklies

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
        maxtexsize = 256;
        reservevpparams = 20;
        batchlightmaps = 0;
        ffdynlights = 0;

        if(!hasOQ) waterrefract = 0;
    }

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

        if(strstr(vendor, "ATI")) ati_dph_bug = ati_line_bug = 1;
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
        glUniform1f_ =                  (PFNGLUNIFORM1FARBPROC)               getprocaddress("glUniform1fARB");
        glUniform2f_ =                  (PFNGLUNIFORM2FARBPROC)               getprocaddress("glUniform2fARB");
        glUniform3f_ =                  (PFNGLUNIFORM3FARBPROC)               getprocaddress("glUniform3fARB");
        glUniform4f_ =                  (PFNGLUNIFORM4FARBPROC)               getprocaddress("glUniform4fARB");
        glUniform1fv_ =                 (PFNGLUNIFORM1FVARBPROC)              getprocaddress("glUniform1fvARB");
        glUniform2fv_ =                 (PFNGLUNIFORM2FVARBPROC)              getprocaddress("glUniform2fvARB");
        glUniform3fv_ =                 (PFNGLUNIFORM3FVARBPROC)              getprocaddress("glUniform3fvARB");
        glUniform4fv_ =                 (PFNGLUNIFORM4FVARBPROC)              getprocaddress("glUniform4fvARB");
        glUniform1i_ =                  (PFNGLUNIFORM1IARBPROC)               getprocaddress("glUniform1iARB");
        glBindAttribLocation_ =         (PFNGLBINDATTRIBLOCATIONARBPROC)      getprocaddress("glBindAttribLocationARB");
        glGetActiveUniform_ =           (PFNGLGETACTIVEUNIFORMARBPROC)        getprocaddress("glGetActiveUniformARB");
        if(!hasVP || !hasFP)
        {
            glEnableVertexAttribArray_ =  (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)  getprocaddress("glEnableVertexAttribArrayARB");
            glDisableVertexAttribArray_ = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) getprocaddress("glDisableVertexAttribArrayARB");
            glVertexAttribPointer_ =      (PFNGLVERTEXATTRIBPOINTERARBPROC)      getprocaddress("glVertexAttribPointerARB");
        }

        extern bool checkglslsupport();
        if(checkglslsupport())
        {
            hasGLSL = true;
            hasglsl = 1;
#ifdef __APPLE__
            //if(osversion<0x1050) ??
            if(hasVP && hasFP) apple_glsldepth_bug = 1;
#endif
            if(apple_glsldepth_bug) conoutf("\frWARNING: Using Apple GLSL depth bug workaround. (use \"/apple_glsldepth_bug 0\" to disable if unnecessary");
        }
    }

    bool hasshaders = (hasVP && hasFP) || hasGLSL;
    if(hasshaders)
    {
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

    if(strstr(exts, "GL_ARB_uniform_buffer_object"))
    {
        glGetUniformIndices_       = (PFNGLGETUNIFORMINDICESPROC)      getprocaddress("glGetUniformIndices");
        glGetActiveUniformsiv_     = (PFNGLGETACTIVEUNIFORMSIVPROC)    getprocaddress("glGetActiveUniformsiv");
        glGetUniformBlockIndex_    = (PFNGLGETUNIFORMBLOCKINDEXPROC)   getprocaddress("glGetUniformBlockIndex");
        glGetActiveUniformBlockiv_ = (PFNGLGETACTIVEUNIFORMBLOCKIVPROC)getprocaddress("glGetActiveUniformBlockiv");
        glUniformBlockBinding_     = (PFNGLUNIFORMBLOCKBINDINGPROC)    getprocaddress("glUniformBlockBinding");
        glBindBufferBase_          = (PFNGLBINDBUFFERBASEPROC)         getprocaddress("glBindBufferBase");
        glBindBufferRange_         = (PFNGLBINDBUFFERRANGEPROC)        getprocaddress("glBindBufferRange");

        useubo = 1;
        hasUBO = true;
        if(strstr(vendor, "ATI")) ati_ubo_bug = 1;
        if(dbgexts) conoutf("\frUsing GL_ARB_uniform_buffer_object extension.");
    }
    else if(strstr(exts, "GL_EXT_bindable_uniform"))
    {
        glUniformBuffer_        = (PFNGLUNIFORMBUFFEREXTPROC)       getprocaddress("glUniformBufferEXT");
        glGetUniformBufferSize_ = (PFNGLGETUNIFORMBUFFERSIZEEXTPROC)getprocaddress("glGetUniformBufferSizeEXT");
        glGetUniformOffset_     = (PFNGLGETUNIFORMOFFSETEXTPROC)    getprocaddress("glGetUniformOffsetEXT");

        usebue = 1;
        hasBUE = true;
        if(strstr(vendor, "ATI")) ati_ubo_bug = 1;
        if(dbgexts) conoutf("\frUsing GL_EXT_bindable_uniform extension.");
    }

    if(strstr(exts, "GL_EXT_texture_rectangle") || strstr(exts, "GL_ARB_texture_rectangle"))
    {
        usetexrect = 1;
        hasTR = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_texture_rectangle extension.");
    }
    else if(hasMT && hasshaders) conoutf("\frWARNING: No texture rectangle support. (no full screen shaders)");

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

    if(strstr(exts, "GL_EXT_fog_coord"))
    {
        glFogCoordPointer_ = (PFNGLFOGCOORDPOINTEREXTPROC) getprocaddress("glFogCoordPointerEXT");
        hasFC = true;
        if(dbgexts) conoutf("\frUsing GL_EXT_fog_coord extension.");
    }

    if(strstr(exts, "GL_ARB_texture_cube_map"))
    {
        GLint val;
        glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, &val);
        hwcubetexsize = val;
        hasCM = true;
        // On Catalyst 10.2, issuing an occlusion query on the first draw using a given cubemap texture causes a nasty crash
        if(strstr(vendor, "ATI")) ati_cubemap_bug = 1;
        if(dbgexts) conoutf("\frUsing GL_ARB_texture_cube_map extension.");
    }
    else conoutf("\frWARNING: No cube map texture support. (no reflective glass)");

    extern int usenp2;
    if(strstr(exts, "GL_ARB_texture_non_power_of_two"))
    {
        hasNP2 = true;
        if(dbgexts) conoutf("\frUsing GL_ARB_texture_non_power_of_two extension.");
    }
    else if(usenp2) conoutf("\frWARNING: Non-power-of-two textures not supported");

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
        if(strstr(vendor, "NVIDIA") || strstr(renderer, "Radeon HD")) hasNVPCF = true;
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
        extern int grass, motionblur, glare, maxdynlights, depthfxsize, depthfxrect, depthfxfilter, blurdepthfx;
        grass = 1;
        motionblur = 1;
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
COMMAND(0, glext, "s");

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
    //glHint(GL_FOG_HINT, GL_NICEST);
    GLfloat fogcolor[4] = { 0, 0, 0, 0 };
    glFogfv(GL_FOG_COLOR, fogcolor);


    glEnable(GL_LINE_SMOOTH);
    //glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
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

    extern int useshaders, forceglsl;
    bool hasshaders = (hasVP && hasFP) || hasGLSL;
    if(!useshaders || (useshaders<0 && avoidshaders) || !hasMT || !hasshaders)
    {
        if(!hasMT || !hasshaders) conoutf("\frWARNING: No shader support! Using fixed-function fallback. (no fancy visuals for you)");
        else if(useshaders<0 && !hasTF) conoutf("\frWARNING: Disabling shaders for extra performance. (use \"/shaders 1\" to enable shaders if desired)");
        renderpath = R_FIXEDFUNCTION;
    }
    else renderpath = hasGLSL ? (!hasVP || !hasFP || forceglsl > 0 ? R_GLSLANG : R_ASMGLSLANG) : R_ASMSHADER;

    static const char * const rpnames[4] = { "fixed-function", "assembly shader", "GLSL shader", "assembly/GLSL shader" };
    conoutf("Rendering using the OpenGL %s path.", rpnames[renderpath]);

    inittmus();
    setuptexcompress();
}

void cleanupgl()
{
    extern int nomasks, nolights, nowater;
    nomasks = nolights = nowater = 0;

    extern void cleanupmotionblur();
    cleanupmotionblur();

    extern void clearminimap();
    clearminimap();
}

#define VARRAY_INTERNAL
#include "varray.h"

VAR(0, wireframe, 0, 0, 1);

ICOMMAND(0, getcampos, "", (),
{
    defformatstring(pos)("%s %s %s", floatstr(camera1->o.x), floatstr(camera1->o.y), floatstr(camera1->o.z));
    result(pos);
});

physent camera, *camera1 = &camera;
vec worldpos, camerapos, camdir, camright, camup;

void findorientation(vec &o, float yaw, float pitch, vec &pos)
{
    vec dir;
    vecfromyawpitch(yaw, pitch, 1, 0, dir);
    if(raycubepos(o, dir, pos, 0, RAY_CLIPMAT|RAY_SKIPFIRST) == -1)
        pos = dir.mul(2*hdr.worldsize).add(o); //otherwise gui won't work when outside of map
}

void transplayer()
{
    // move from RH to Z-up LH quake style worldspace
    glLoadMatrixf(viewmatrix.v);

    glRotatef(camera1->roll, 0, 1, 0);
    glRotatef(camera1->pitch, -1, 0, 0);
    glRotatef(camera1->yaw, 0, 0, -1);

    glTranslatef(-camera1->o.x, -camera1->o.y, -camera1->o.z);
}

float curfov = 100, fovy, aspect;
int farplane, xtraverts, xtravertsva;

VAR(IDF_WORLD, fog, 16, 4000, INT_MAX-1);
bvec fogcolor(0x80, 0x99, 0xB3);
VARF(IDF_HEX|IDF_WORLD, fogcolour, 0, 0x8099B3, 0xFFFFFF,
{
    fogcolor = bvec((fogcolour>>16)&0xFF, (fogcolour>>8)&0xFF, fogcolour&0xFF);
});

void vecfromcursor(float x, float y, float z, vec &dir)
{
    vec dir1 = invmvpmatrix.perspectivetransform(vec(x*2-1, 1-2*y, z*2-1)),
        dir2 = invmvpmatrix.perspectivetransform(vec(x*2-1, 1-2*y, -1));
    (dir = dir1).sub(dir2).normalize();
}

void vectocursor(vec &v, float &x, float &y, float &z)
{
    vec screenpos = mvpmatrix.perspectivetransform(v);

    x = screenpos.x*0.5f + 0.5f;
    y = 0.5f - screenpos.y*0.5f;
    z = screenpos.z*0.5f + 0.5f;
}

extern const glmatrixf viewmatrix(vec4(-1, 0, 0, 0), vec4(0, 0, 1, 0), vec4(0, -1, 0, 0));
glmatrixf mvmatrix, projmatrix, mvpmatrix, invmvmatrix, invmvpmatrix;

void readmatrices()
{
    glGetFloatv(GL_MODELVIEW_MATRIX, mvmatrix.v);
    glGetFloatv(GL_PROJECTION_MATRIX, projmatrix.v);

    mvpmatrix.mul(projmatrix, mvmatrix);
    invmvmatrix.invert(mvmatrix);
    invmvpmatrix.invert(mvpmatrix);
}

FVAR(0, nearplane, 1e-3f, 0.54f, 1e3f);

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

VAR(0, reflectclip, 0, 6, 64);

glmatrixf clipmatrix;

static const glmatrixf dummymatrix;
static int projectioncount = 0;
void pushprojection(const glmatrixf &m = dummymatrix)
{
    glMatrixMode(GL_PROJECTION);
    if(projectioncount <= 0) glPushMatrix();
    if(&m != &dummymatrix) glLoadMatrixf(m.v);
    if(fogging)
    {
        glMultMatrixf(mvmatrix.v);
        glMultMatrixf(invfogmatrix.v);
    }
    glMatrixMode(GL_MODELVIEW);
    projectioncount++;
}

void popprojection()
{
    --projectioncount;
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    if(projectioncount > 0)
    {
        glPushMatrix();
        if(fogging)
        {
            glMultMatrixf(mvmatrix.v);
            glMultMatrixf(invfogmatrix.v);
        }
    }
    glMatrixMode(GL_MODELVIEW);
}

FVAR(0, polygonoffsetfactor, -1e4f, -3.0f, 1e4f);
FVAR(0, polygonoffsetunits, -1e4f, -3.0f, 1e4f);
FVAR(0, depthoffset, -1e4f, 0.01f, 1e4f);

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
    if(fogging)
    {
        glMultMatrixf(mvmatrix.v);
        glMultMatrixf(invfogmatrix.v);
    }
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
    if(clipped) 
    {
        glLoadMatrixf(clipmatrix.v);
        if(fogging)
        {
            glMultMatrixf(mvmatrix.v);
            glMultMatrixf(invfogmatrix.v);
        }
    }
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
    if(e.z > 2*size) { sx1 = sy1 = 1; sx2 = sy2 = -1; return; }
    float zzrr = e.z*e.z - size*size,
          dx = e.x*e.x + zzrr, dy = e.y*e.y + zzrr,
          focaldist = 1.0f/tan(fovy*0.5f*RAD);
    sx1 = sy1 = -1;
    sx2 = sy2 = 1;
    #define CHECKPLANE(c, dir, focaldist, low, high) \
    do { \
        float nzc = (cz*cz + 1) / (cz dir drt) - cz, \
              pz = (d##c)/(nzc*e.c - e.z); \
        if(pz > 0) \
        { \
            float c = (focaldist)*nzc, \
                  pc = pz*nzc; \
            if(pc < e.c) low = c; \
            else if(pc > e.c) high = c; \
        } \
    } while(0)
    if(dx > 0)
    {
        float cz = e.x/e.z, drt = sqrtf(dx)/size;
        CHECKPLANE(x, -, focaldist/aspect, sx1, sx2);
        CHECKPLANE(x, +, focaldist/aspect, sx1, sx2);
    }
    if(dy > 0)
    {
        float cz = e.y/e.z, drt = sqrtf(dy)/size;
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

glmatrixf envmatrix;

void setenvmatrix()
{
    envmatrix = fogging ? fogmatrix : mvmatrix;
    if(reflecting) envmatrix.reflectz(reflectz);
    envmatrix.transpose();
}

static float findsurface(int fogmat, const vec &v, int &abovemat)
{
    ivec o(v), co;
    int csize;
    do
    {
        cube &c = lookupcube(o.x, o.y, o.z, 0, co, csize);
        if(!c.ext || (c.ext->material&MATF_VOLUME) != fogmat)
        {
            abovemat = c.ext && isliquid(c.ext->material&MATF_VOLUME) ? c.ext->material&MATF_VOLUME : MAT_AIR;
            return o.z;
        }
        o.z = co.z + csize;
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
            loopk(3) fogc[k] += blend*watercol[k]/255.0f;
            end += logblend*min(fog, max(waterfog*4, 32));
            break;

        case MAT_LAVA:
            loopk(3) fogc[k] += blend*lavacol[k]/255.0f;
            end += logblend*min(fog, max(lavafog*4, 32));
            break;

        default:
            loopk(3) fogc[k] += blend*fogcolor[k]/255.0f;
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
}

GLuint motiontex = 0;
int motionw = 0, motionh = 0, lastmotion = 0;

void cleanupmotionblur()
{
    if(motiontex) { glDeleteTextures(1, &motiontex); motiontex = 0; }
    motionw = motionh = 0;
    lastmotion = 0;
}

VARF(IDF_PERSIST, motionblur, 0, 0, 1, { if(!motionblur) cleanupmotionblur(); });
VAR(IDF_PERSIST, motionblurmillis, 1, 5, 1000);
FVAR(IDF_PERSIST, motionblurscale, 0, 1, 1);

void addmotionblur()
{
    extern int viewtype;
    if(!motionblur || viewtype || !hasTR || max(screen->w, screen->h) > hwtexsize) return;

    if(!motiontex || motionw != screen->w || motionh != screen->h)
    {
        if(!motiontex) glGenTextures(1, &motiontex);
        motionw = screen->w;
        motionh = screen->h;
        lastmotion = 0;
        createtexture(motiontex, motionw, motionh, NULL, 3, 0, GL_RGB, GL_TEXTURE_RECTANGLE_ARB);
    }

    float amount = min(hud::motionblur(motionblurscale), 1.0f);
    if(amount <= 0)
    {
        lastmotion = 0;
        return;
    }

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, motiontex);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_RECTANGLE_ARB);

    rectshader->set();

    glColor4f(1, 1, 1, lastmotion ? pow(amount, max(float(lastmillis - lastmotion)/motionblurmillis, 1.0f)) : 0);
    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(      0,       0); glVertex2f(-1, -1);
    glTexCoord2f(motionw,       0); glVertex2f( 1, -1);
    glTexCoord2f(      0, motionh); glVertex2f(-1,  1);
    glTexCoord2f(motionw, motionh); glVertex2f( 1,  1);
    glEnd();

    glDisable(GL_TEXTURE_RECTANGLE_ARB);
    glEnable(GL_TEXTURE_2D);

    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    if(lastmillis - lastmotion >= motionblurmillis)
    {
        lastmotion = lastmillis - lastmillis%motionblurmillis;

        glCopyTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, 0, 0, screen->w, screen->h);
    }
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
            maxc = max(watercol[0], max(watercol[1], watercol[2]));
            loopk(3) overlay[k] += blend*max(0.4f, watercol[k]/min(32.0f + maxc*7.0f/8.0f, 255.0f));
            break;

        case MAT_LAVA:
            maxc = max(lavacol[0], max(lavacol[1], lavacol[2]));
            loopk(3) overlay[k] += blend*max(0.4f, lavacol[k]/min(32.0f + maxc*7.0f/8.0f, 255.0f));
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
    glBegin(GL_TRIANGLE_STRIP);
    glVertex2f(-1, -1);
    glVertex2f(1, -1);
    glVertex2f(-1, 1);
    glVertex2f(1, 1);
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

VAR(IDF_PERSIST, skyboxglare, 0, 1, 1);
FVAR(0, firstpersondepth, 0, 0.5f, 1);

void drawglare()
{
    glaring = true;
    refracting = -1;

    float oldfogstart, oldfogend, oldfogcolor[4], zerofog[4] = { 0, 0, 0, 1 };
    glGetFloatv(GL_FOG_START, &oldfogstart);
    glGetFloatv(GL_FOG_END, &oldfogend);
    glGetFloatv(GL_FOG_COLOR, oldfogcolor);

    glFogf(GL_FOG_START, (fog+64)/8);
    glFogf(GL_FOG_END, fog);
    glFogfv(GL_FOG_COLOR, zerofog);

    glClearColor(0, 0, 0, 1);
    extern int skyglare, skybgglare;
    glClear((skyboxglare && (skyglare || skybgglare) ? 0 : GL_COLOR_BUFFER_BIT) | GL_DEPTH_BUFFER_BIT);

    if(skyboxglare && limitsky()) drawskybox(farplane, true);

    rendergeom();

    if(skyboxglare && !limitsky()) drawskybox(farplane, false);

    renderreflectedmapmodels();
    rendergame();

    renderwater();
    rendermaterials();
    renderalphageom();
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

VAR(IDF_PERSIST, reflectmms, 0, 1, 1);
VAR(IDF_WORLD, refractsky, 0, 0, 1);

glmatrixf fogmatrix, invfogmatrix;

void drawreflection(float z, bool refract)
{
    reflectz = z < 0 ? 1e16f : z;
    reflecting = !refract;
    refracting = refract ? (z < 0 || camera1->o.z >= z ? -1 : 1) : 0;
    fading = renderpath!=R_FIXEDFUNCTION && waterrefract && waterfade && hasFBO && z>=0;
    fogging = refracting<0 && z>=0;

    float oldfogstart, oldfogend, oldfogcolor[4];
    glGetFloatv(GL_FOG_START, &oldfogstart);
    glGetFloatv(GL_FOG_END, &oldfogend);
    glGetFloatv(GL_FOG_COLOR, oldfogcolor);

    if(fogging)
    {
        glFogf(GL_FOG_START, camera1->o.z - z);
        glFogf(GL_FOG_END, camera1->o.z - (z-waterfog));
        GLfloat m[16] =
        {
             1,   0,  0, 0,
             0,   1,  0, 0,
             0,   0,  1, 0,
            -camera1->o.x, -camera1->o.y, -camera1->o.z, 1
        };
        memcpy(fogmatrix.v, m, sizeof(m));
        invfogmatrix.invert(fogmatrix);
        pushprojection();
        glPushMatrix();
        glLoadMatrixf(fogmatrix.v);
        float fogc[4] = { watercol.x/255.0f, watercol.y/255.0f, watercol.z/255.0f, 1.0f };
        glFogfv(GL_FOG_COLOR, fogc);
    }
    else
    {
        glFogf(GL_FOG_START, (fog+64)/8);
        glFogf(GL_FOG_END, fog);
        float fogc[4] = { fogcolor.x/255.0f, fogcolor.y/255.0f, fogcolor.z/255.0f, 1.0f };
        glFogfv(GL_FOG_COLOR, fogc);
    }

    if(fading)
    {
        float scale = fogging ? -0.25f : 0.25f, offset = 2*fabs(scale) - scale*z; 
        setenvparamf("waterfadeparams", SHPARAM_VERTEX, 8, scale, offset, -scale, offset + camera1->o.z*scale);
        setenvparamf("waterfadeparams", SHPARAM_PIXEL, 8, scale, offset, -scale, offset + camera1->o.z*scale);
    }

    if(reflecting)
    {
        glPushMatrix();
        glTranslatef(0, 0, 2*z);
        glScalef(1, 1, -1);

        glFrontFace(GL_CCW);
    }

    setenvmatrix();

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
        plane clipplane;
        invmvmatrix.transposetransform(plane(0, 0, refracting>0 ? 1 : -1, refracting>0 ? -zclip : zclip), clipplane);
        clipmatrix.clip(clipplane, projmatrix);
        pushprojection(clipmatrix);
    }

    renderreflectedgeom(refracting<0 && z>=0 && caustics, fogging);

    if(reflecting || refracting>0 || (refracting<0 && refractsky) || z<0)
    {
        if(fading) glColorMask(COLORMASK, GL_TRUE);
        if(reflectclip && z>=0) popprojection();
        if(fogging)
        {
            popprojection();
            glPopMatrix();
        }
        drawskybox(farplane, false);
        if(fogging)
        {
            pushprojection();
            glPushMatrix();
            glLoadMatrixf(fogmatrix.v);
        }
        if(reflectclip && z>=0) pushprojection(clipmatrix);
        if(fading) glColorMask(COLORMASK, GL_FALSE);
    }
    else if(fading) glColorMask(COLORMASK, GL_FALSE);

    renderdecals();

    if(reflectmms) renderreflectedmapmodels();
    rendergame();

    if(refracting) rendergrass();
    rendermaterials();
    renderalphageom(fogging);
    renderparticles();

    if(game::thirdpersonview() || reflecting) renderavatar(false);

    if(fading) glColorMask(COLORMASK, GL_TRUE);

    if(reflectclip && z>=0) popprojection();

    if(reflecting)
    {
        glPopMatrix();

        glFrontFace(GL_CW);
    }

    if(fogging)
    {
        popprojection();
        glPopMatrix();
    }
    glFogf(GL_FOG_START, oldfogstart);
    glFogf(GL_FOG_END, oldfogend);
    glFogfv(GL_FOG_COLOR, oldfogcolor);

    reflectz = 1e16f;
    refracting = 0;
    reflecting = fading = fogging = false;

    setenvmatrix();
}

bool envmapping = false;

void drawcubemap(int size, int level, const vec &o, float yaw, float pitch, bool flipx, bool flipy, bool swapxy)
{
    float fovy = 90.f, aspect = 1.f;
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
    setviewcell(camera1->o);

    defaultshader->set();

    updatedynlights();

    int fogmat = lookupmaterial(camera1->o)&MATF_VOLUME, abovemat = MAT_AIR;
    float fogblend = 1.0f, causticspass = 0.0f;
    if(fogmat==MAT_WATER || fogmat==MAT_LAVA)
    {
        float z = findsurface(fogmat, camera1->o, abovemat) - WATER_OFFSET;
        if(camera1->o.z < z + 1) fogblend = min(z + 1 - camera1->o.z, 1.0f);
        else fogmat = abovemat;
        if(level > 1 && caustics && fogmat==MAT_WATER && camera1->o.z < z)
            causticspass = renderpath==R_FIXEDFUNCTION ? 1.0f : min(z - camera1->o.z, 1.0f);
    }
    else
    {
        fogmat = MAT_AIR;
    }
    setfog(fogmat, fogblend, abovemat);
    if(level > 1 && fogmat != MAT_AIR)
    {
        float blend = abovemat==MAT_AIR ? fogblend : 1.0f;
        fovy += blend*sinf(lastmillis/1000.0)*2.0f;
        aspect += blend*sinf(lastmillis/1000.0+PI)*0.1f;
    }

    int farplane = hdr.worldsize*2;

    project(fovy, aspect, farplane, flipx, flipy, swapxy);
    transplayer();
    readmatrices();
    setenvmatrix();

    glEnable(GL_FOG);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    xtravertsva = xtraverts = glde = gbatches = 0;

    if(level > 1 && !hasFBO)
    {
        if(dopostfx)
        {
            drawglaretex();
            drawdepthfxtex();
            drawreflections();
        }
        else dopostfx = true;
    }

    visiblecubes();

    if(level > 1 && shadowmap && !hasFBO) rendershadowmap();

    glClear(GL_DEPTH_BUFFER_BIT);

    if(limitsky()) drawskybox(farplane, true);
    rendergeom(level > 1 ? causticspass : 0);
    if(level > 1) queryreflections();
    if(level) generategrass();
    if(!limitsky()) drawskybox(farplane, false);
    if(level > 1) renderdecals();

    rendermapmodels();
    renderalphageom();

    if(level) rendergame();
    if(level > 1)
    {
        if(hasFBO)
        {
            drawglaretex();
            drawdepthfxtex();
            drawreflections();
        }
        renderwater(); // water screws up for some reason
    }
    rendergrass();
    rendermaterials();
    renderparticles();

    glDisable(GL_FOG);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    if(level > 1) addglare();

    glDisable(GL_TEXTURE_2D);

    camera1 = oldcamera;
    envmapping = false;
}

bool minimapping = false;

GLuint minimaptex = 0;
vec minimapcenter(0, 0, 0), minimapradius(0, 0, 0), minimapscale(0, 0, 0);

void clearminimap()
{
    if(minimaptex) { glDeleteTextures(1, &minimaptex); minimaptex = 0; }
}

VAR(IDF_WORLD, minimapheight, 0, 0, 2<<16);
bvec minimapcolor(0, 0, 0);
VARF(IDF_HEX|IDF_WORLD, minimapcolour, 0, 0, 0xFFFFFF,
{
    minimapcolor = bvec((minimapcolour>>16)&0xFF, (minimapcolour>>8)&0xFF, minimapcolour&0xFF);
});
VAR(IDF_WORLD, minimapclip, 0, 0, 1);
VARF(IDF_PERSIST, minimapsize, 7, 8, 10, { if(minimaptex) drawminimap(); });

void bindminimap()
{
    glBindTexture(GL_TEXTURE_2D, minimaptex);
}

void clipminimap(ivec &bbmin, ivec &bbmax, cube *c = worldroot, int x = 0, int y = 0, int z = 0, int size = hdr.worldsize>>1)
{
    loopi(8)
    {
        ivec o(i, x, y, z, size);
        if(c[i].children) clipminimap(bbmin, bbmax, c[i].children, o.x, o.y, o.z, size>>1);
        else if(!isentirelysolid(c[i]) && (!c[i].ext || (c[i].ext->material&MATF_CLIP)!=MAT_CLIP))
        {
            loopk(3) bbmin[k] = min(bbmin[k], o[k]);
            loopk(3) bbmax[k] = max(bbmax[k], o[k] + size);
        }
    }
}

void drawminimap()
{
    if(!hud::needminimap()) { clearminimap(); return; }

    progress(0, "generating mini-map...");

    int size = 1<<minimapsize, sizelimit = min(hwtexsize, min(screen->w, screen->h));
    while(size > sizelimit) size /= 2;
    if(!minimaptex) glGenTextures(1, &minimaptex);

    extern vector<vtxarray *> valist;
    ivec bbmin(hdr.worldsize, hdr.worldsize, hdr.worldsize), bbmax(0, 0, 0);
    loopv(valist)
    {
        vtxarray *va = valist[i];
        loopk(3)
        {
            if(va->geommin[k]>va->geommax[k]) continue;
            bbmin[k] = min(bbmin[k], va->geommin[k]);
            bbmax[k] = max(bbmax[k], va->geommax[k]);
        }
    }
    if(minimapclip)
    {
        ivec clipmin(hdr.worldsize, hdr.worldsize, hdr.worldsize), clipmax(0, 0, 0);
        clipminimap(clipmin, clipmax);
        loopk(2) bbmin[k] = max(bbmin[k], clipmin[k]);
        loopk(2) bbmax[k] = min(bbmax[k], clipmax[k]);
    }

    minimapradius = bbmax.tovec().sub(bbmin.tovec()).mul(0.5f);
    minimapcenter = bbmin.tovec().add(minimapradius);
    minimapradius.x = minimapradius.y = max(minimapradius.x, minimapradius.y);
    minimapscale = vec((0.5f - 1.0f/size)/minimapradius.x, (0.5f - 1.0f/size)/minimapradius.y, 1.0f);

    envmapping = minimapping = true;

    physent *oldcamera = camera1;
    static physent cmcamera;
    cmcamera = *camera1;
    cmcamera.reset();
    cmcamera.type = ENT_CAMERA;
    cmcamera.o = vec(minimapcenter.x, minimapcenter.y, max(minimapcenter.z + minimapradius.z + 1, float(minimapheight)));
    cmcamera.yaw = 0;
    cmcamera.pitch = -90;
    cmcamera.roll = 0;
    camera1 = &cmcamera;
    setviewcell(vec(-1, -1, -1));

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-minimapradius.x, minimapradius.x, -minimapradius.y, minimapradius.y, 0, camera1->o.z + 1);
    glScalef(-1, 1, 1);
    glMatrixMode(GL_MODELVIEW);

    transplayer();

    defaultshader->set();

    GLfloat fogc[4] = { minimapcolor.x/255.0f, minimapcolor.y/255.0f, minimapcolor.z/255.0f, 1.0f };
    glFogf(GL_FOG_START, 0);
    glFogf(GL_FOG_END, 1000000);
    glFogfv(GL_FOG_COLOR, fogc);

    glClearColor(fogc[0], fogc[1], fogc[2], fogc[3]);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glViewport(1, 1, size-2, size-2);
    glScissor(1, 1, size-2, size-2);
    glEnable(GL_SCISSOR_TEST);

    glDisable(GL_FOG);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glFrontFace(GL_CCW);

    xtravertsva = xtraverts = glde = gbatches = 0;

    visiblecubes(false);
    queryreflections();
    drawreflections();

    loopi(minimapheight > 0 && minimapheight < minimapcenter.z + minimapradius.z ? 2 : 1)
    {
        if(i)
        {
            glClear(GL_DEPTH_BUFFER_BIT);
            camera1->o.z = minimapheight;
            transplayer();
        }
        rendergeom();
        rendermapmodels();
        renderwater();
        rendermaterials();
        renderalphageom();
    }

    glFrontFace(GL_CW);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_FOG);

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, screen->w, screen->h);

    camera1 = oldcamera;
    envmapping = minimapping = false;

    glBindTexture(GL_TEXTURE_2D, minimaptex);
    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5, 0, 0, size, size, 0);
    setuptexparameters(minimaptex, NULL, 3, 1, GL_RGB5, GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

VAR(IDF_PERSIST, scr_virtw, 0, 1024, INT_MAX-1);
VAR(IDF_PERSIST, scr_virth, 0, 768, INT_MAX-1);
VAR(IDF_PERSIST, scr_minw, 0, 640, INT_MAX-1);
VAR(IDF_PERSIST, scr_minh, 0, 480, INT_MAX-1);

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
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(tx,    ty);    glVertex2f(sx,    sy);
        glTexCoord2f(tx+tw, ty);    glVertex2f(sx+sw, sy);
        glTexCoord2f(tx,    ty+th); glVertex2f(sx,    sy+sh);
        glTexCoord2f(tx+tw, ty+th); glVertex2f(sx+sw, sy+sh);
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

VARF(IDF_PERSIST, viewtype, VW_NORMAL, VW_NORMAL, VW_MAX, loopi(VP_MAX) views[i].cleanup());
FVAR(IDF_PERSIST, stereoblend, 0, 0.5f, 1);
FVAR(IDF_PERSIST, stereodist, 0, 0.5f, 10000);
FVAR(IDF_PERSIST, stereoplane, 1e-3f, 40.f, 1000);
FVAR(IDF_PERSIST, stereonear, 0, 2.f, 10000);

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

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    int w = screen->w, h = screen->h;
    gettextres(w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, w, h, 0, -1, 1);

    glClearColor(0.f, 0.f, 0.f, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    defaultshader->set();

    hud::update(screen->w, screen->h);
    hud::drawhud(true);
    hud::drawlast();

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

    visiblecubes();
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
    renderavatar(true);

    rendermapmodels();
    rendergame();

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
    renderalphageom();

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

    addmotionblur();
    addglare();
    if(fogmat==MAT_WATER || fogmat==MAT_LAVA) drawfogoverlay(fogmat, fogblend, abovemat);
    renderpostfx();

    glDisable(GL_TEXTURE_2D);
    notextureshader->set();
    if(editmode)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);

        renderblendbrush();
        rendereditcursor();

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
    hud::drawhud();
    render_texture_panel(w, h);
    hud::drawlast();
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
    if(client::ready())
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
        setenvmatrix();

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
                glColor4f(1.f, 1.f, 1.f, stereoblend); views[VP_RIGHT].draw(0, 0, 1, 1);
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
                    glBegin(GL_TRIANGLE_STRIP);
                    glVertex2f(0, 0);
                    glVertex2f(1, 0);
                    glVertex2f(0, 1);
                    glVertex2f(1, 1);
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
