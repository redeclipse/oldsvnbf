#include "cube.h"
#include "iengine.h"
#include "igame.h"

extern int lastmillis, verbose;
extern void fatal(char *s, char *o);
extern void conoutf(const char *s, ...);
extern void console(const char *s, int n, ...);

extern igameclient *cl;
extern igameserver *sv;
extern iclientcom *cc;
extern icliententities *et;
