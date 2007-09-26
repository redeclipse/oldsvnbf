// Blood Frontier - MASTERSERVER - Mod-friendly alternative master server.

#include "def.h"

struct dbgself dbgd;

void dbg_setup (void)
{
#ifdef DBG
	struct sigaction sa;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = dbg_fatal;
	sigaction (SIGSEGV, &sa, NULL);
#endif
}

void dbg_fatal (int num)
{
#ifdef DBG
	fprintf (stdout, "Error %d (%s) at %s:%li in %s.\n\n", num, strsignal (num), dbgd.file, dbgd.line, dbgd.func);
#else
	fprintf (stdout, "Fatal Error\n\n");
#endif
	exit (num);
}
