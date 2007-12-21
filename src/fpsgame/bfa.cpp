#include "pch.h"
#include "engine.h"

#include "fpsdefine.h"
#include "fpsserver.h"
#ifndef STANDALONE
#include "fpsgame.h"
REGISTERGAME(fpsgame, "bfa", new fpsclient(), new fpsserver());
#else
REGISTERGAME(fpsgame, "bfa", NULL, new fpsserver());
#endif



