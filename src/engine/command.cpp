// command.cpp: implements the parsing and execution of a tiny script language which
// is largely backwards compatible with the quake console language.

#include "pch.h"
#include "engine.h"

vector<attr *> attrs;
ABOOL(attrs, test_bool, true);
AINT(attrs, test_int, 1, 0, 1);
AFLOAT(attrs, test_float, 1.f, 0.f, 1.f);

ICOMMAND(test_attr, "", (void), {
	loopv (attrs)
	{
		ACASE(attrs[i],
			{
				conoutf("attribute %s has bad type (%d)", a->name, a->type);
			},
			{
				conoutf("boolean attribute %s is set to %s", a->name, a->value ? "true" : "false");
			},
			{
				conoutf("integer attribute %s is set to %d (range: %d .. %d)", a->name, a->value, a->minval, a->maxval);
			},
			{
				conoutf("float attribute %s is set to %.2f (range: %.2f .. %.2f)", a->name, a->value, a->minval, a->maxval);
			}
		);
	}
});

void itoa(char *s, int i) { s_sprintf(s)("%d", i); }
char *exchangestr(char *o, const char *n) { delete[] o; return newstring(n); }

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
    if(id._type != ID_ALIAS) return;
	identstack *stack = new identstack;
	stack->action = id._isexecuting==id._action ? newstring(id._action) : id._action;
	stack->next = id._stack;
	id._stack = stack;
	id._action = val;
}

void popident(ident &id)
{
    if(id._type != ID_ALIAS || !id._stack) return;
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
		ident init(ID_ALIAS, name, newstring(""), IDC_GLOBAL&(persistidents ? IDC_PERSIST : 0));
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

void aliasa(const char *name, char *action)
{
	ident *b = idents->access(name);
	if(!b)
	{
		ident b(ID_ALIAS, newstring(name), action, IDC_GLOBAL&(persistidents ? IDC_PERSIST : 0));
		if(overrideidents) b._override = OVERRIDDEN;
		idents->access(b._name, &b);
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
			if (b->_context & IDC_PERSIST && !persistidents) b->_context &= ~IDC_PERSIST;
			else if (!(b->_context & IDC_PERSIST) && persistidents) b->_context |= IDC_PERSIST;
		}
	}
}

void alias(const char *name, const char *action) { aliasa(name, newstring(action)); }

COMMAND(alias, "ss");

// variable's and commands are registered through globals, see cube.h

int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), int context)
{
	if(!idents) idents = new identtable;
	ident v(ID_VAR, name, min, cur, max, storage, (void *)fun, context);
	idents->access(name, &v);
	return cur;
}

#define GETVAR(id, name, retval) \
	ident *id = idents->access(name); \
	if(!id || id->_type!=ID_VAR) return retval;
void setvar(const char *name, int i, bool dofunc) 
{
	GETVAR(id, name, );
	*id->_storage = i;
	if(dofunc) id->changed();
}
int getvar(const char *name) 
{
	GETVAR(id, name, 0);
	return *id->_storage;
}
int getvarmin(const char *name) 
{
	GETVAR(id, name, 0);
	return id->_min;
}
int getvarmax(const char *name) 
{
	GETVAR(id, name, 0);
	return id->_max;
}
bool identexists(const char *name) { return idents->access(name)!=NULL; }
ident *getident(const char *name) { return idents->access(name); }

const char *getalias(const char *name)
{
	ident *i = idents->access(name);
	return i && i->_type==ID_ALIAS ? i->_action : "";
}

bool addcommand(const char *name, void (*fun)(), const char *narg, int context)
{
	if(!idents) idents = new identtable;
	ident c(ID_COMMAND, name, narg, (void *)fun, NULL, context);
	idents->access(name, &c);
	return false;
}

void addident(const char *name, ident *id)
{
	if(!idents) idents = new identtable;
	idents->access(name, id);
}

static vector<vector<char> *> wordbufs;
static int bufnest = 0;

char *parseexp(const char *&p, int right);

void parsemacro(const char *&p, int level, vector<char> &wordbuf)
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
    static vector<char> ident;
    ident.setsizenodelete(0);
    while(isalnum(*p) || *p=='_') ident.add(*p++);
    ident.add(0);
    const char *alias = getalias(ident.getbuf());
	while(*alias) wordbuf.add(*alias++);
}

char *parseexp(const char *&p, int right)          // parse any nested set of () or []
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
            const char *end = p+strcspn(p, "\"\r\n\0");
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

char *parseword(const char *&p, int context)						// parse single argument, including expressions
{
	for(;;)
	{
		p += strspn(p, context & IDC_SERVER ? " " : " \t\r");
		if(p[0]!='/' || p[1]!='/') break;
		p += strcspn(p, context & IDC_SERVER ? "\0" : "\n\0");
	}
	if(*p=='\"')
	{
		p++;
		const char *word = p;
		p += strcspn(p, context & IDC_SERVER ? "\"\0" : "\"\r\n\0");
		char *s = newstring(word, p-word);
		if(*p=='\"') p++;
		return s;
	}
	if(context != IDC_SERVER && *p=='(') return parseexp(p, ')');
	if(context != IDC_SERVER && *p=='[') return parseexp(p, ']');
	const char *word = p;
	for(;;)
	{
		p += strcspn(p, context & IDC_SERVER ? "/ " : "/; \t\r\n\0");
		if(p[0]!='/' || p[1]=='/') break;
		else if(p[1]=='\0') { p++; break; }
		p += 2;
	}
	if(p-word==0) return NULL;
	char *s = newstring(word, p-word);
	if(context != IDC_SERVER && *s=='$') return lookup(s);				// substitute variables
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

char *executeret(const char *p, int context)
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
            w[i] = (char *)"";
			if(i>numargs) continue;
			char *s = parseword(p, context);			 // parse and evaluate exps
			if(s) w[i] = s;
			else numargs = i;
		}

		p += strcspn(p, context & IDC_SERVER ? "\0" : ";\n\0");
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
			if (context & IDC_SERVER && (!id || id->_context != IDC_SERVER))
			{
				s_sprintfd(z)("invalid server command: %s", c);
				setretval(newstring(z));
			}
			else if (!id)
			{
				if(!parseint(c) && *c!='0')
					conoutf("unknown command: %s", c);
				setretval(newstring(c));
			}
#ifndef STANDALONE
			else if (context != IDC_SERVER && id->_context & IDC_SERVER)
			{
				s_sprintfd(z)("%s", conc(w, numargs, true));
				cc->toservcmd(z, true);
			}
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
                    for(const char *a = id->_narg; *a; a++) switch(*a)
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
							if(id->_context & IDC_PERSIST)
							{
								conoutf("cannot override persistent variable %s", id->_name);
								break;
							}
							if(id->_override==NO_OVERRIDE) id->_override = *id->_storage;
						}
						else
						{
#ifndef STANDALONE
							if (id->_context & IDC_WORLD)
							{
								if (!editmode)
								{
									conoutf("world variable %s may only be modified in editmode", id->_name);
									break;
								}
							}
#endif
							if(id->_override!=NO_OVERRIDE) id->_override = NO_OVERRIDE;
						}

						int i1 = parseint(w[1]);
						if(i1<id->_min || i1>id->_max)
						{
							i1 = i1<id->_min ? id->_min : id->_max;				// clamp to valid range
							conoutf("valid range for %s is %d..%d", id->_name, id->_min, id->_max);
						}

						*id->_storage = i1;
						id->changed();											 // call trigger function if available

#ifndef STANDALONE
						if (!overrideidents && id->_context & IDC_WORLD)
							cl->editvariable(id->_name, i1); // update others
#endif
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
								ident init(ID_ALIAS, newstring(argname), newstring(""), IDC_GLOBAL&(persistidents ? IDC_PERSIST : 0));
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
					setretval(executeret(id->_action, context));
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

int execute(const char *p, int context)
{
	char *ret = executeret(p, context);
	int i = 0;
	if(ret) { i = parseint(ret); delete[] ret; }
	return i;
}

bool execfile(const char *cfgfile)
{
	string s;
	s_strcpy(s, cfgfile);
	char *buf = loadfile(s, NULL);
	if(!buf) return false;
	execute(buf);
	delete[] buf;
#ifndef STANDALONE
	if (verbose >= 3) conoutf("loaded script '%s'", cfgfile);
#endif
	return true;
}

void exec(const char *cfgfile)
{
	if(!execfile(cfgfile)) conoutf("could not read \"%s\"", cfgfile);
}

#ifndef STANDALONE
void writecfg()
{
	FILE *f = openfile("config.cfg", "w");
	if(!f) return;
	fprintf(f, "// Automatically written by Blood Frontier\n");
	cc->writeclientinfo(f);
	fprintf(f, "if (= $version %d) [\n\n", BFRONTIER);
	enumerate(*idents, ident, id,
		if (id._type == ID_VAR && id._context & IDC_PERSIST)
		{
			fprintf(f, "%s %d\n", id._name, *id._storage);
		}
	);
	writebinds(f);
	enumerate(*idents, ident, id,
		if(id._type == ID_ALIAS && id._context & IDC_PERSIST && id._override == NO_OVERRIDE && !strstr(id._name, "nextmap_") && id._action[0])
		{
			fprintf(f, "\"%s\" = [%s]\n", id._name, id._action);
		}
	);
	writecompletions(f);
	fprintf(f, "] [ echo \"WARNING: config from different version ignored\" ]\n");
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
void mina (int *a, int *b) { intret(min(*a, *b)); }      COMMANDN(min, mina, "ii");
void maxa (int *a, int *b) { intret(max(*a, *b)); }      COMMANDN(max, maxa, "ii");

void anda (char *a, char *b) { intret(execute(a)!=0 && execute(b)!=0); }
void ora  (char *a, char *b) { intret(execute(a)!=0 || execute(b)!=0); }

COMMANDN(&&, anda, "ss");
COMMANDN(||, ora, "ss");

void rndn(int *a)		  { intret(*a>0 ? rnd(*a) : 0); }  COMMANDN(rnd, rndn, "i");

void strcmpa(char *a, char *b) { intret(strcmp(a,b)==0); }  COMMANDN(strcmp, strcmpa, "ss");

ICOMMAND(echo, "C", (char *s), conoutf("\fs\fw%s\fr", s));

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

ICOMMAND(exists, "ss", (char *a, char *b), intret(fileexists(a, *b ? b : "r")));

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
