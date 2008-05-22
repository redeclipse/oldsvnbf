#include "pch.h"
#include "engine.h"

int curtime = 0, lastmillis = 0, totalmillis = 0;

VARP(verbose, 0, 0, 3);

void conoutf(const char *s, ...) { s_sprintfdlv(str, s, s); printf("%s\n", str); }
void console(const char *s, int n, ...) { s_sprintfdlv(str, n, s); printf("%s\n", str); }
void servertoclient(int chan, uchar *buf, int len) {}

void cleanup()
{
	writecfg();
}

void fatal(const char *s, ...)
{
    va_list args;
    va_start(args, s);
    vprintf(s, args);
    va_end(args);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	atexit(cleanup);
	execfile("config.cfg");

    if(argc>=2) setvar("masterport", atoi(argv[1]));
    if(argc>=3) setsvar("masterip", argv[2]);

	if(enet_initialize()<0) fatal("Unable to initialise network module");

    setupmaster();

    for(;;)
    {
		int _lastmillis = lastmillis;
		lastmillis = totalmillis = (int)enet_time_get();
		curtime = lastmillis-_lastmillis;

        checkmaster();
    }

    return EXIT_SUCCESS;
}

