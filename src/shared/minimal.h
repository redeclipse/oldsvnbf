#include "cube.h"
#include "iengine.h"
#include "igame.h"

extern int curtime, lastmillis, totalmillis;
extern void fatal(const char *s, ...);
extern void conoutf(const char *s, ...);
extern void console(const char *s, int n, ...);

extern igameclient *cl;
extern igameserver *sv;
extern iclientcom *cc;
extern icliententities *et;
