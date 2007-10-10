// command.cpp: implements the parsing and execution of a tiny script language which
// is largely backwards compatible with the quake console language.

#include "pch.h"
#ifdef STANDALONE
#ifdef BFRONTIER // better header
#include "minimal.h"
#else
#include "cube.h"
#include "iengine.h"
#include "igame.h"
extern int lastmillis;
extern void fatal(char *s, char *o);
extern void conoutf(const char *s, ...);
#endif
#else
#include "engine.h"
#endif

void itoa(char *s, int i) { s_sprintf(s)("%d", i); }
char *exchangestr(char *o, const char *n) { delete[] o; return newstring(n); }

#ifndef BFRONTIER // we put this in command.h
typedef hashtable<char *, ident> identtable;
#endif

identtable *idents = NULL;		// contains ALL vars/commands/aliases

bool overrideidents = false, persistidents = true;

void clearstack(ident &id)
{
	identstack *stack = id._stack;
	while(stack)
	{
		delete[] stack->action;
		identstack *tmp = stack;
		stack = stack->next;
		delete tmp;
	}
	id._stack = NULL;
}

void clear_command()
{
	enumerate(*idents, ident, i, if(i._type==ID_ALIAS) { DELETEA(i._name); DELETEA(i._action); if(i._stack) clearstack(i); });
	if(idents) idents->clear();
}

void clearoverrides()
{
	enumerate(*idents, ident, i,
		if(i._override!=NO_OVERRIDE)
		{
			switch(i._type)
			{
				case ID_ALIAS: 
					if(i._action[0]) i._action = exchangestr(i._action, ""); 
					break;
				case ID_VAR: 
					*i._storage = i._override;
					i.changed();
					break;
			}
			i._override = NO_OVERRIDE;
		});
}

void pushident(ident &id, char *val)
{
	identstack *stack = new identstack;
	stack->action = id._isexecuting==id._action ? newstring(id._action) : id._action;
	stack->next = id._stack;
	id._stack = stack;
	id._action = val;
}

void popident(ident &id)
{
	if(!id._stack) return;
	if(id._action != id._isexecuting) delete[] id._action;
	identstack *stack = id._stack;
	id._action = stack->action;
	id._stack = stack->next;
	delete stack;
}

void pusha(char *name, char *action)
{
	ident *id = idents->access(name);
	if(!id)
	{
		name = newstring(name);
#ifdef BFRONTIER // server and world vars
		ident init(ID_ALIAS, name, newstring(""), persistidents, false, false);
#else
		ident init(ID_ALIAS, name, newstring(""), persistidents);
#endif
		id = idents->access(name, &init);
	}
	pushident(*id, action);
}

void push(char *name, char *action)
{
	pusha(name, newstring(action));
}

void pop(char *name)
{
	ident *id = idents->access(name);
	if(id) popident(*id);
}

COMMAND(push, "ss");
COMMAND(pop, "s");

void aliasa(char *name, char *action)
{
	ident *b = idents->access(name);
	if(!b) 
	{
		name = newstring(name);
#ifdef BFRONTIER // server and world vars
		ident b(ID_ALIAS, name, action, persistidents, false, false);
#else
		ident b(ID_ALIAS, name, action, persistidents);
#endif
		if(overrideidents) b._override = OVERRIDDEN;
		idents->access(name, &b);
	}
	else if(b->_type != ID_ALIAS)
	{
		conoutf("cannot redefine builtin %s with an alias", name);
		delete[] action;
	}
	else 
	{
		if(b->_action != b->_isexecuting) delete[] b->_action;
		b->_action = action;
		if(overrideidents) b->_override = OVERRIDDEN;
		else 
		{
			if(b->_override != NO_OVERRIDE) b->_override = NO_OVERRIDE;
			if(b->_persist != persistidents) b->_persist = persistidents;
		}
	}
}

void alias(char *name, char *action) { aliasa(name, newstring(action)); }

COMMAND(alias, "ss");

// variable's and commands are registered through globals, see cube.h

#ifdef BFRONTIER // server and world vars
int variable(char *name, int min, int cur, int max, int *storage, void (*fun)(), bool persist, bool server, bool world)
{
	if(!idents) idents = new identtable;
	ident v(ID_VAR, name, min, cur, max, storage, (void *)fun, persist, server, world);
	idents->access(name, &v);
	return cur;
}
#else
int variable(char *name, int min, int cur, int max, int *storage, void (*fun)(), bool persist)
{
	if(!idents) idents = new identtable;
	ident v(ID_VAR, name, min, cur, max, storage, (void *)fun, persist);
	idents->access(name, &v);
	return cur;
}
#endif

#define GETVAR(id, name, retval) \
	ident *id = idents->access(name); \
	if(!id || id->_type!=ID_VAR) return retval;
void setvar(char *name, int i, bool dofunc) 
{ 
	GETVAR(id, name, );
	*id->_storage = i; 
	if(dofunc) id->changed();
} 
int getvar(char *name) 
{ 
	GETVAR(id, name, 0);
	return *id->_storage;
}
int getvarmin(char *name) 
{ 
	GETVAR(id, name, 0);
	return id->_min;
}
int getvarmax(char *name) 
{ 
	GETVAR(id, name, 0);
	return id->_max;
}
bool identexists(char *name) { return idents->access(name)!=NULL; }
ident *getident(char *name) { return idents->access(name); }

const char *getalias(char *name)
{
	ident *i = idents->access(name);
	return i && i->_type==ID_ALIAS ? i->_action : "";
}

#ifdef BFRONTIER // server and world vars
bool addcommand(char *name, void (*fun)(), char *narg, bool server, bool world)
{
	if(!idents) idents = new identtable;
	ident c(ID_COMMAND, name, narg, (void *)fun, NULL, server, world);
	idents->access(name, &c);
	return false;
}
#else
bool addcommand(char *name, void (*fun)(), char *narg)
{
	if(!idents) idents = new identtable;
	ident c(ID_COMMAND, name, narg, (void *)fun);
	idents->access(name, &c);
	return false;
}
#endif

void addident(char *name, ident *id)
{
	if(!idents) idents = new identtable;
	idents->access(name, id);
}

static vector<vector<char> *> wordbufs;
static int bufnest = 0;

char *parseexp(char *&p, int right);

void parsemacro(char *&p, int level, vector<char> &wordbuf)
{
	int escape = 1;
	while(*p=='@') p++, escape++;
	if(level > escape)
	{
		while(escape--) wordbuf.add('@');
		return;
	}
	if(*p=='(')
	{
		char *ret = parseexp(p, ')');
		if(ret)
		{
			for(char *sub = ret; *sub; ) wordbuf.add(*sub++);
			delete[] ret;
		}
		return;
	}
	char *ident = p;
	while(isalnum(*p) || *p=='_') p++;
	int c = *p;
	*p = 0;
	const char *alias = getalias(ident);
	*p = c;
	while(*alias) wordbuf.add(*alias++);
}

char *parseexp(char *&p, int right)		  // parse any nested set of () or []
{
	if(bufnest++>=wordbufs.length()) wordbufs.add(new vector<char>);
	vector<char> &wordbuf = *wordbufs[bufnest-1];
	int left = *p++;
	for(int brak = 1; brak; )
	{
		int c = *p++;
		if(c=='\r') continue;				// hack
		if(left=='[' && c=='@')
		{
			parsemacro(p, brak, wordbuf);
			continue;
		}
		if(c=='\"')
		{
			wordbuf.add(c);
			char *end = p+strcspn(p, "\"\r\n\0");
			while(p < end) wordbuf.add(*p++);
			if(*p=='\"') wordbuf.add(*p++);
			continue;
		}
		if(c=='/' && *p=='/')
		{
			p += strcspn(p, "\n\0");
			continue;
		}
			
		if(c==left) brak++;
		else if(c==right) brak--;
		else if(!c) 
		{ 
			p--;
			conoutf("missing \"%c\"", right);
			wordbuf.setsize(0); 
			bufnest--;
			return NULL; 
		}
		wordbuf.add(c);
	}
	wordbuf.pop();
	char *s;
	if(left=='(')
	{
		wordbuf.add(0);
		char *ret = executeret(wordbuf.getbuf());					// evaluate () exps directly, and substitute result
		wordbuf.pop();
		s = ret ? ret : newstring("");
	}
	else
	{
		s = newstring(wordbuf.getbuf(), wordbuf.length());
	}
	wordbuf.setsize(0);
	bufnest--;
	return s;
}

char *lookup(char *n)							// find value of ident referenced with $ in exp
{
	ident *id = idents->access(n+1);
	if(id) switch(id->_type)
	{
		case ID_VAR: { string t; itoa(t, *(id->_storage)); return exchangestr(n, t); }
		case ID_ALIAS: return exchangestr(n, id->_action);
	}
	conoutf("unknown alias lookup: %s", n+1);
	return n;
}

#ifdef BFRONTIER // server side support
char *parseword(char *&p, bool isserver)						// parse single argument, including expressions
#else
char *parseword(char *&p)                       // parse single argument, including expressions
#endif
{
	for(;;)
	{
#ifdef BFRONTIER // server side support
		p += strspn(p, isserver ? " " : " \t\r");
#else
        p += strspn(p, " \t\r");
#endif
		if(p[0]!='/' || p[1]!='/') break;
#ifdef BFRONTIER // server side support
		p += strcspn(p, isserver ? "\0" : "\n\0");  
#else
        p += strcspn(p, "\n\0");  
#endif
	}
	if(*p=='\"')
	{
		p++;
		char *word = p;
#ifdef BFRONTIER // server side support
		p += strcspn(p, isserver ? "\"\0" : "\"\r\n\0");
#else
        p += strcspn(p, "\"\r\n\0");
#endif
		char *s = newstring(word, p-word);
		if(*p=='\"') p++;
		return s;
	}
#ifdef BFRONTIER // server side support
	if(!isserver && *p=='(') return parseexp(p, ')');
	if(!isserver && *p=='[') return parseexp(p, ']');
#else
    if(*p=='(') return parseexp(p, ')');
    if(*p=='[') return parseexp(p, ']');
#endif
	char *word = p;
	for(;;)
	{
#ifdef BFRONTIER // server side support
		p += strcspn(p, isserver ? "/ " : "/; \t\r\n\0");
#else
        p += strcspn(p, "/; \t\r\n\0");
#endif
		if(p[0]!='/' || p[1]=='/') break;
		else if(p[1]=='\0') { p++; break; }
		p += 2;
	}
	if(p-word==0) return NULL;
	char *s = newstring(word, p-word);
#ifdef BFRONTIER // server side support
	if(!isserver && *s=='$') return lookup(s);				// substitute variables
#else
    if(*s=='$') return lookup(s);               // substitute variables
#endif
	return s;
}

char *conc(char **w, int n, bool space)
{
	int len = space ? max(n-1, 0) : 0;
	loopj(n) len += (int)strlen(w[j]);
	char *r = newstring("", len);
	loopi(n)		
	{
		strcat(r, w[i]);  // make string-list out of all arguments
		if(i==n-1) break;
		if(space) strcat(r, " ");
	}
	return r;
}

VARN(numargs, _numargs, 0, 0, 25);

#define parseint(s) strtol((s), NULL, 0)

char *commandret = NULL;

#ifndef STANDALONE
extern const char *addreleaseaction(const char *s);
#endif

#ifdef BFRONTIER // server side support
char *executeret(char *p, bool isserver)
#else
char *executeret(char *p)               // all evaluation happens here, recursively
#endif
{
	const int MAXWORDS = 25;					// limit, remove
	char *w[MAXWORDS];
	char *retval = NULL;
	#define setretval(v) { char *rv = v; if(rv) retval = rv; commandret = NULL; }
	for(bool cont = true; cont;)				// for each ; seperated statement
	{
		int numargs = MAXWORDS;
		loopi(MAXWORDS)						 // collect all argument values
		{
			w[i] = "";
			if(i>numargs) continue;
#ifdef BFRONTIER // server side support
			char *s = parseword(p, isserver);			 // parse and evaluate exps
#else
            char *s = parseword(p);             // parse and evaluate exps
#endif
			if(s) w[i] = s;
			else numargs = i;
		}
		
#ifdef BFRONTIER // server side support
		p += strcspn(p, isserver ? "\0" : ";\n\0");
#else
        p += strcspn(p, ";\n\0");
#endif
		cont = *p++!=0;						 // more statements if this isn't the end of the string
		char *c = w[0];
		if(*c=='/') c++;						// strip irc-style command prefix
		if(!*c) continue;						// empty statement
		
		DELETEA(retval);

		if(w[1][0]=='=' && !w[1][1])
		{
			aliasa(c, numargs>2 ? w[2] : newstring(""));
			w[2] = NULL;
		}
		else
		{
			ident *id = idents->access(c);
#ifdef BFRONTIER // server side support
			if (isserver && (!id || id->_server == false))
			{
				s_sprintfd(z)("invalid server command: %s", c);
				setretval(newstring(z));
			}
			else
#endif
			if(!id)
			{
				if(!parseint(c) && *c!='0')
					conoutf("unknown command: %s", c);
				setretval(newstring(c));
			}
#ifdef BFRONTIER // server side support, client only
#ifndef STANDALONE
			else if (!isserver && id->_server == true)
			{
				s_sprintfd(z)("%s", conc(w, numargs, true));
				cc->toservcmd(z, true);
			}
#endif
#endif
			else switch(id->_type)
			{
                case ID_CCOMMAND:
				case ID_COMMAND:					 // game defined commands
				{	
					void *v[MAXWORDS];
					union
					{
						int i;
						float f;
					} nstor[MAXWORDS];
					int n = 0, wn = 0;
                    char *cargs = NULL;
                    if(id->_type==ID_CCOMMAND) v[n++] = id->self;
					for(char *a = id->_narg; *a; a++) switch(*a)
					{
						case 's':								 v[n] = w[++wn];	 n++; break;
						case 'i': nstor[n].i = parseint(w[++wn]); v[n] = &nstor[n].i; n++; break;
						case 'f': nstor[n].f = atof(w[++wn]);	 v[n] = &nstor[n].f; n++; break;
#ifndef STANDALONE
						case 'D': nstor[n].i = addreleaseaction(id->_name) ? 1 : 0; v[n] = &nstor[n].i; n++; break;
#endif
						case 'V': v[n++] = w+1; nstor[n].i = numargs-1; v[n] = &nstor[n].i; n++; break;
                        case 'C': if(!cargs) cargs = conc(w+1, numargs-1, true); v[n++] = cargs; break;
						default: fatal("builtin declared with illegal type");
					}
					switch(n)
					{
						case 0: ((void (__cdecl *)()									  )id->_fun)();							 break;
						case 1: ((void (__cdecl *)(void *)								)id->_fun)(v[0]);						 break;
						case 2: ((void (__cdecl *)(void *, void *)						)id->_fun)(v[0], v[1]);					break;
						case 3: ((void (__cdecl *)(void *, void *, void *)				)id->_fun)(v[0], v[1], v[2]);			 break;
						case 4: ((void (__cdecl *)(void *, void *, void *, void *)		)id->_fun)(v[0], v[1], v[2], v[3]);		break;
						case 5: ((void (__cdecl *)(void *, void *, void *, void *, void *))id->_fun)(v[0], v[1], v[2], v[3], v[4]); break;
						case 6: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *))id->_fun)(v[0], v[1], v[2], v[3], v[4], v[5]); break;
						case 7: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *, void *))id->_fun)(v[0], v[1], v[2], v[3], v[4], v[5], v[6]); break;
						case 8: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *, void *, void *))id->_fun)(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]); break;
						default: fatal("builtin declared with too many args (use V?)");
					}
                    if(cargs) delete[] cargs;
					setretval(commandret);
					break;
				}

				case ID_VAR:						// game defined variables 
					if(!w[1][0]) conoutf("%s = %d", c, *id->_storage);	  // var with no value just prints its current value
					else if(id->_min>id->_max) conoutf("variable %s is read-only", id->_name);
					else
					{
						if(overrideidents)
						{
							if(id->_persist)
							{
								conoutf("cannot override persistent variable %s", id->_name);
								break;
							}
							if(id->_override==NO_OVERRIDE) id->_override = *id->_storage;
						}
						else if(id->_override!=NO_OVERRIDE) id->_override = NO_OVERRIDE;
						int i1 = parseint(w[1]);
						if(i1<id->_min || i1>id->_max)
						{
							i1 = i1<id->_min ? id->_min : id->_max;				// clamp to valid range
							conoutf("valid range for %s is %d..%d", id->_name, id->_min, id->_max);
						}
						*id->_storage = i1;
						id->changed();											 // call trigger function if available
					}
					break;
					
				case ID_ALIAS:							  // alias, also used as functions and (global) variables
				{
                    static vector<ident *> argids;
					for(int i = 1; i<numargs; i++)
					{
                        if(i > argids.length())
                        {
                            s_sprintfd(argname)("arg%d", i);
                            ident *id = idents->access(argname);
                            if(!id)
                            {
                                ident init(ID_ALIAS, newstring(argname), newstring(""), persistidents);
                                id = idents->access(init._name, &init);
                            }
                            argids.add(id);
                        }
                        pushident(*argids[i-1], w[i]); // set any arguments as (global) arg values so functions can access them
						w[i] = NULL;
					}
					_numargs = numargs-1;
					bool wasoverriding = overrideidents;
					if(id->_override!=NO_OVERRIDE) overrideidents = true;
					char *wasexecuting = id->_isexecuting;
					id->_isexecuting = id->_action;
#ifdef BFRONTIER // server side support
					setretval(executeret(id->_action, isserver));
#else
                    setretval(executeret(id->_action));
#endif
					if(id->_isexecuting != id->_action && id->_isexecuting != wasexecuting) delete[] id->_isexecuting;
					id->_isexecuting = wasexecuting;
					overrideidents = wasoverriding;
                    for(int i = 1; i<numargs; i++) popident(*argids[i-1]);
					break;
				}
			}
		}
		loopj(numargs) if(w[j]) delete[] w[j];
	}
	return retval;
}

#ifdef BFRONTIER // server side support
int execute(char *p, bool isserver)
{
	char *ret = executeret(p, isserver);
#else
int execute(char *p)
{
    char *ret = executeret(p);
#endif
	int i = 0; 
	if(ret) { i = parseint(ret); delete[] ret; }
	return i;
}

bool execfile(char *cfgfile)
{
	string s;
	s_strcpy(s, cfgfile);
	char *buf = loadfile(path(s), NULL);
	if(!buf) return false;
	execute(buf);
	delete[] buf;
	return true;
}

void exec(char *cfgfile)
{
	if(!execfile(cfgfile)) conoutf("could not read \"%s\"", cfgfile);
#ifdef BFRONTIER // verbose support
	else if (verbose >= 2) console("loaded script '%s'", CON_RIGHT, cfgfile);
#endif
}
#ifndef STANDALONE
void writecfg()
{
#ifdef BFRONTIER // game specific configs
	FILE *f = gameopen("config.cfg", "w");
	if(!f) return;
	fprintf(f, "// automatically written on exit\n\n");
#else
    FILE *f = openfile(path(cl->savedconfig(), true), "w");
	if(!f) return;
	fprintf(f, "// automatically written on exit, DO NOT MODIFY\n// delete this file to have %s overwrite these settings\n// modify settings in game, or put settings in %s to override anything\n\n", cl->defaultconfig(), cl->autoexec());
#endif
	cc->writeclientinfo(f);
	fprintf(f, "\n");
    writecrosshairs(f);
	enumerate(*idents, ident, id,
		if(id._type==ID_VAR && id._persist)
		{
			fprintf(f, "%s %d\n", id._name, *id._storage);
		}
	);
	fprintf(f, "\n");
	writebinds(f);
	fprintf(f, "\n");
	enumerate(*idents, ident, id,
		if(id._type==ID_ALIAS && id._persist && id._override==NO_OVERRIDE && !strstr(id._name, "nextmap_") && id._action[0])
		{
			fprintf(f, "\"%s\" = [%s]\n", id._name, id._action);
		}
	);
	fprintf(f, "\n");
	writecompletions(f);
	fclose(f);
}

COMMAND(writecfg, "");
#endif

// below the commands that implement a small imperative language. thanks to the semantics of
// () and [] expressions, any control construct can be defined trivially.

void intset(char *name, int v) { string b; itoa(b, v); alias(name, b); }
void intret			(int v) { string b; itoa(b, v); commandret = newstring(b); }

ICOMMAND(if, "sss", (char *cond, char *t, char *f), commandret = executeret(cond[0]!='0' ? t : f));

ICOMMAND(loop, "sis", (char *var, int *n, char *body), loopi(*n) { intset(var, i); execute(body); });
ICOMMAND(while, "ss", (char *cond, char *body), while(execute(cond)) execute(body));    // can't get any simpler than this :)

void concat(const char *s) { commandret = newstring(s); }
void result(const char *s) { commandret = newstring(s); }

void concatword(char **args, int *numargs)
{
	commandret = conc(args, *numargs, false);
}

void format(char **args, int *numargs)
{
	vector<char> s;
	char *f = args[0];
	while(*f)
	{
		int c = *f++;
		if(c == '%')
		{
			int i = *f++;
			if(i >= '1' && i <= '9')
			{
				i -= '0';
				const char *sub = i < *numargs ? args[i] : "";
				while(*sub) s.add(*sub++);
			}
			else s.add(i);
		}
		else s.add(c);
	}
	s.add('\0');
	result(s.getbuf());
}

#define whitespaceskip s += strspn(s, "\n\t ")
#define elementskip *s=='"' ? (++s, s += strcspn(s, "\"\n\0"), s += *s=='"') : s += strcspn(s, "\n\t \0")

void explodelist(char *s, vector<char *> &elems)
{
    whitespaceskip;
    while(*s)
    {
        char *elem = s;
        elementskip;
        elems.add(*elem=='"' ? newstring(elem+1, s-elem-(s[-1]=='"' ? 2 : 1)) : newstring(elem, s-elem));
        whitespaceskip;
    }
}

void listlen(char *s)
{
	int n = 0;
	whitespaceskip;
	for(; *s; n++) elementskip, whitespaceskip;
	intret(n);
}

void at(char *s, int *pos)
{
	whitespaceskip;
	loopi(*pos) elementskip, whitespaceskip;
	char *e = s;
	elementskip;
	if(*e=='"') 
	{
		e++;
        if(s[-1]=='"') --s;
	}
	*s = '\0';
	result(e);
}

void getalias_(char *s)
{
	result(getalias(s));
}

COMMAND(exec, "s");
COMMAND(concat, "C");
COMMAND(result, "s");
COMMAND(concatword, "V");
COMMAND(format, "V");
COMMAND(at, "si");
COMMAND(listlen, "s");
COMMANDN(getalias, getalias_, "s");

void add  (int *a, int *b) { intret(*a + *b); }		  COMMANDN(+, add, "ii");
void mul  (int *a, int *b) { intret(*a * *b); }		  COMMANDN(*, mul, "ii");
void sub  (int *a, int *b) { intret(*a - *b); }		  COMMANDN(-, sub, "ii");
void divi (int *a, int *b) { intret(*b ? *a / *b : 0); } COMMANDN(div, divi, "ii");
void mod  (int *a, int *b) { intret(*b ? *a % *b : 0); } COMMAND(mod, "ii");
void equal(int *a, int *b) { intret((int)(*a == *b)); }  COMMANDN(=, equal, "ii");
void nequal(int *a, int *b) { intret((int)(*a != *b)); } COMMANDN(!=, nequal, "ii");
void lt	(int *a, int *b) { intret((int)(*a < *b)); }	COMMANDN(<, lt, "ii");
void gt	(int *a, int *b) { intret((int)(*a > *b)); }	COMMANDN(>, gt, "ii");
void lte	(int *a, int *b) { intret((int)(*a <= *b)); } COMMANDN(<=, lte, "ii");
void gte	(int *a, int *b) { intret((int)(*a >= *b)); } COMMANDN(>=, gte, "ii");
void xora (int *a, int *b) { intret(*a ^ *b); }		  COMMANDN(^, xora, "ii");
void nota (int *a)		 { intret(*a == 0); }		  COMMANDN(!, nota, "i");

void anda (char *a, char *b) { intret(execute(a)!=0 && execute(b)!=0); }
void ora  (char *a, char *b) { intret(execute(a)!=0 || execute(b)!=0); }

COMMANDN(&&, anda, "ss");
COMMANDN(||, ora, "ss");

void rndn(int *a)		  { intret(*a>0 ? rnd(*a) : 0); }  COMMANDN(rnd, rndn, "i");

void strcmpa(char *a, char *b) { intret(strcmp(a,b)==0); }  COMMANDN(strcmp, strcmpa, "ss");

ICOMMAND(echo, "C", (char *s), conoutf("\f1%s", s));

void strstra(char *a, char *b) { char *s = strstr(a, b); intret(s ? s-a : -1); } COMMANDN(strstr, strstra, "ss");

struct sleepcmd
{
	int millis;
	char *command;
    bool override;
};
vector<sleepcmd> sleepcmds;

void addsleep(int *msec, char *cmd)
{
	sleepcmd &s = sleepcmds.add();
	s.millis = *msec+lastmillis;
	s.command = newstring(cmd);
    s.override = overrideidents;
}

COMMANDN(sleep, addsleep, "is");

void checksleep(int millis)
{
	loopv(sleepcmds)
	{
		sleepcmd &s = sleepcmds[i];
		if(s.millis && millis>s.millis)
		{
			char *cmd = s.command; // execute might create more sleep commands
			execute(cmd);
			delete[] cmd;
			sleepcmds.remove(i--);
		}
	}
}

void clearsleep(bool clearoverrides)
{
    int len = 0;
    loopv(sleepcmds) 
    {
        if(clearoverrides && !sleepcmds[i].override) sleepcmds[len++] = sleepcmds[i];
        else delete[] sleepcmds[i].command;
    }
    sleepcmds.setsize(len);
}

void clearsleep_(int *clearoverrides)
{
    clearsleep(*clearoverrides!=0 || overrideidents);
}

COMMANDN(clearsleep, clearsleep_, "i");

#ifdef BFRONTIER // extra script utils, definitions
ICOMMAND(max, "ii", (int a, int b), intret(max(a, b)));
ICOMMAND(min, "ii", (int a, int b), intret(min(a, b)));

ICOMMAND(exists, "ss", (char *a, char *b), intret(fileexists(a, *b ? b : "r")));

char *getgameident() { return sv->gameident(); }
ICOMMAND(getgameident, "", (void), result(getgameident()));

char *getdefaultmap() { return sv->defaultmap(); }
ICOMMAND(getdefaultmap, "", (void), result(getdefaultmap()));

#ifndef STANDALONE
ICOMMAND(getmaptitle, "", (void), result(getmaptitle()));
#endif

string _gettime;
char *gettime(char *format)
{
	time_t ltime;
	struct tm *t;

	ltime = time (NULL);
	t = localtime (&ltime);

	strftime (_gettime, sizeof (_gettime) - 1, format, t);
	
	return _gettime;
}
ICOMMAND(gettime, "s", (char *a), result(gettime(a)));

bool gameexec(char *name, bool quiet)
{
	s_sprintfd(s)("data/%s/%s", getgameident(), name);
	if (quiet) { return execfile(s); }
	else { exec(s); return true; }
}

FILE *gameopen(char *name, char *mode)
{
	s_sprintfd(s)("data/%s/%s", getgameident(), name);
	return openfile(s, mode);
}

int gzgetint(gzFile f)
{
	int t;
	gzread(f, &t, sizeof(int));
	endianswap(&t, sizeof(int), 1);
	return t;
}

void gzputint(gzFile f, int x)
{
	int t = (int)x;
	endianswap(&t, sizeof(int), 1);
	gzwrite(f, &t, sizeof(int));
}

float gzgetfloat(gzFile f)
{
	float t;
	gzread(f, &t, sizeof(float));
	endianswap(&t, sizeof(float), 1);
	return t;
}

void gzputfloat(gzFile f, float x)
{
	float t = (float)x;
	endianswap(&t, sizeof(float), 1);
	gzwrite(f, &t, sizeof(float));
}

#endif
