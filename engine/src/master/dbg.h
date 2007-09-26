// Blood Frontier - MASTERSERVER - Mod-friendly alternative master server.

#ifndef __dbg_h__
#define __dbg_h__

struct dbgself
{
	long int line;
	char file[1024];
	char func[1024];
};
extern struct dbgself dbgd;

#ifdef DBG
#define dbg	dbgd.line = __LINE__; snprintf (dbgd.file, sizeof (dbgd.file) - 1, "%s", __FILE__); snprintf (dbgd.func, sizeof (dbgd.func) - 1, "%s", __PRETTY_FUNCTION__);
#else
#define dbg ;
//#define dbg	fprintf(stderr, "%s:%d\n", __FILE__, __LINE__);
#endif

extern void dbg_setup (void);
extern void dbg_fatal (int num);

#endif
