// Blood Frontier - MASTERSERVER - Mod-friendly alternative master server.

#include "def.h"

char *utl_sanitise (char *dst, size_t size, char *src)
{
	char ret[MAXSIZE], out[MAXSIZE], tmp[6];
	long int x, len;

	snprintf (ret, sizeof (ret) - 1, "%s", src);
	out[0] = 0;

	dbg for (x = 0, len = strlen (ret); (x < len) && (ret[x]); x++)
	{
		if (!isalnum (ret[x]))
			snprintf (tmp, sizeof (tmp) - 1, "%%%02x", ret[x]);
		else
			snprintf (tmp, sizeof (tmp) - 1, "%c", ret[x]);

		fprintf (stdout, "[%s]", tmp);

		concat (out, tmp);
	}
	snprintf (dst, size, "%s", out);
	return dst;
}
