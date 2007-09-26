// Blood Frontier - MASTERSERVER - Mod-friendly alternative master server.

#ifndef __utl_h__
#define __utl_h__

#define concat(a,b) strncat(a, b, strlen (b) > (sizeof (a) - strlen(a) - 1) ? strlen (b) - (sizeof (a) - strlen(a) - 1) : strlen (b));

extern char *utl_sanitise (char *dst, size_t size, char *src);

#endif
