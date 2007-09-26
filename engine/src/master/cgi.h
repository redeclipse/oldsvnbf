// Blood Frontier - MASTERSERVER - Mod-friendly alternative master server.

#ifndef __cgi_h__
#define __cgi_h__

struct cgivar
{
	char name[MAXSIZE];
	char value[MAXSIZE];
	struct cgivar *next;
};

struct cgiself
{
	char host[MAXSIZE];
	char page[MAXSIZE];
	char addr[MAXSIZE];

	char data[MAXSIZE * MAXVARS * 2];

	long int stamp;
	
	struct cgivar *vars;
};
extern struct cgiself cgi;

extern void cgi_setup (void);
extern struct cgivar *cgi_var (const char *name, long int num);

#endif
