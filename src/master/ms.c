// Blood Frontier - MASTERSERVER - Mod-friendly alternative master server.

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include "def.h"

void ms_header (void)
{
	fprintf (stdout, "Pragma: No-Cache\n");
	fprintf (stdout, "Content-type: text/plain\n\n");
}

int main (void)
{
	dbg_setup ();

	dbg cgi_setup ();
	dbg ms_header ();
	
	
	// to make it http://yourserver/ms/ms.cgi?action=register|list[&data=blah]
	
	struct cgivar *sa = cgi_var ("action", 1), *sb = cgi_var ("data", 1);
	
	dbg if (sa)
	{
		dbg if (!strcasecmp (sa->value, "register"))
		{
			dbg fprintf (stdout, "Register: %s\n", sb ? sb->value : "");
		}
		else if (!strcasecmp (sa->value, "list"))
		{
			dbg fprintf (stdout, "List: %s\n", sb ? sb->value : "");
		}
		else
		{
			dbg fprintf (stdout, "Page %s not found.\n", sa->value);
		}
	}
	else
	{
		dbg fprintf (stdout, "Page not found.\n");
	}
	return 0;
}
