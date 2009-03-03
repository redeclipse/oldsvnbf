#ifdef __GNUC__
#define gamma __gamma
#endif

#include <math.h>

#ifdef __GNUC__
#undef gamma
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#ifdef __GNUC__
#include <new>
#else
#include <new.h>
#endif
#include <time.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#define _dup	dup
#define _fileno fileno
#endif


#ifdef WIN32
  #define _WINDOWS
  #ifndef __GNUC__
    #define ZLIB_DLL
	#ifndef STANDALONE
    #include <eh.h>
    #include <dbghelp.h>
	#endif
  #endif
#endif
#include <zlib.h>

#ifndef STANDALONE
#include <SDL.h>
#include <SDL_image.h>
#ifdef TTF2FONT
#include <SDL_ttf.h>
#endif
#ifdef INTERFACE
#define GL_GLEXT_LEGACY
#define __glext_h__
#define NO_SDL_GLEXT
#include <SDL_opengl.h>
#undef __glext_h__
#include "GL/glext.h"
#endif
#endif

#if defined(INTERFACE) || defined(STANDALONE)
#include <enet/enet.h>
#endif

#ifdef __sun__
#undef sun
#undef MAXNAMELEN
#endif

#include "tools.h"
#include "command.h"
#include "geom.h"
#include "ents.h"

#include "iengine.h"
#include "igame.h"

