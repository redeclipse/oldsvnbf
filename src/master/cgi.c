// Blood Frontier - MASTERSERVER - Mod-friendly alternative master server.

#include "def.h"

struct cgiself cgi;

void cgi_setup (void)
{
	snprintf (cgi.host, sizeof (cgi.host) - 1, "%s", getenv ("SERVER_NAME"));
	snprintf (cgi.page, sizeof (cgi.page) - 1, "%s", getenv ("SCRIPT_NAME"));
	snprintf (cgi.addr, sizeof (cgi.addr) - 1, "%s", getenv ("REMOTE_ADDR"));
	snprintf (cgi.data, sizeof (cgi.data) - 1, "%s", getenv ("QUERY_STRING"));

	cgi.stamp = time (NULL);

	cgi.vars = NULL;

	char tmp[2][MAXSIZE];

	tmp[0][0] = tmp[1][0] = 0;
	
	dbg if (cgi.data[0])
	{
    	long int x = 0, y = 0, type = 0, z = 0, len = 0;
    
    	len = strlen (cgi.data);
    
    	dbg for (x = y = type = 0; cgi.data[x]; x++)
    	{
    		z = 0;
    		
    		dbg if (!type)
    		{
        		dbg if (cgi.data[x] == '=')
        		{
					tmp[type][y] = 0;

					type = 1;
					y = 0;
					z = 1;
				}
    		}
    		else
    		{
        		dbg if (cgi.data[x] == '&')
        		{
					tmp[type][y] = 0;
        			
        			type = 0;
        			y = 0;
        			
                	dbg if ((tmp[0][0] != 0) && (tmp[1][0] != 0))
                	{
                		struct cgivar *sa = cgi.vars;
                		
                		dbg if (!sa)
                		{
                			sa = (struct cgivar *)malloc (sizeof (struct cgivar));
                			sa->next = NULL;
                			cgi.vars = sa;
                		}
                		else
                		{
                			sa->next = (struct cgivar *)malloc (sizeof (struct cgivar));
                			sa = sa->next;
                		}
                		dbg if (!sa)
                			dbg_fatal (SIGILL);
            
                		snprintf (sa->name, sizeof (sa->name) - 1, "%s", tmp[0]);
                		snprintf (sa->value, sizeof (sa->value) - 1, "%s", tmp[1]);
                	}
            		tmp[0][0] = tmp[1][0] = 0;
        			
        			z = 1;
        		}
    		}
    		dbg if (!z)
    		{
    			dbg if (cgi.data[x] == '+')
    			{
    				tmp[type][y] = ' ';
    				y++;
    			}
    			else if ((cgi.data[x] == '%') && ((x + 2) < len) && (isxdigit (cgi.data[x + 1])) && (isxdigit (cgi.data[x + 2])))
    			{
    				char hex[3];
    				
    				hex[0] = cgi.data[x + 1];
    				hex[1] = cgi.data[x + 2];
    				hex[2] = 0;
    			
    				tmp[type][y] = strtol (hex, NULL, 16);
    				
    				y++;
    				x += 2;
    			}
    			else
    			{
    				tmp[type][y] = cgi.data[x];
    				y++;
    			}
    		}
    	}
    	tmp[type][y] = 0;
    	
    	dbg if ((tmp[0][0] != 0) && (tmp[1][0] != 0))
    	{
    		struct cgivar *sa = cgi.vars;
    		
    		dbg if (!sa)
    		{
    			sa = (struct cgivar *)malloc (sizeof (struct cgivar));
    			sa->next = NULL;
    			cgi.vars = sa;
    		}
    		else
    		{
    			sa->next = (struct cgivar *)malloc (sizeof (struct cgivar));
    			sa = sa->next;
    		}
    		dbg if (!sa)
    			dbg_fatal (SIGILL);

    		snprintf (sa->name, sizeof (sa->name) - 1, "%s", tmp[0]);
    		snprintf (sa->value, sizeof (sa->value) - 1, "%s", tmp[1]);
    	}
		tmp[0][0] = tmp[1][0] = 0;
	}
}

struct cgivar *cgi_var (const char *name, long int num)
{
	long int n = 0;
	struct cgivar *sa;
	
	dbg for (sa = cgi.vars; sa; sa = sa->next)
	{
		dbg if (!strcasecmp (sa->name, name))
		{
			n++;
			dbg if (n >= num)
			{
				return sa;
			}
		}
	}
	return NULL;
}
