// script binding functionality


enum { ID_VAR, ID_COMMAND, ID_ICOMMAND, ID_ALIAS };

enum { NO_OVERRIDE = INT_MAX, OVERRIDDEN = 0 };

struct identstack
{
    char *action;
    identstack *next;
};

template <class T> struct tident
{
    int _type;           // one of ID_* above
    char *_name;
    int _min, _max;      // ID_VAR
    int _override;       // either NO_OVERRIDE, OVERRIDDEN, or value
#ifdef BFRONTIER
    bool _server, _world;
#endif
    union
    {
        void (__cdecl *_fun)();      // ID_VAR, ID_COMMAND
        identstack *_stack;  // ID_ALIAS
    };
    union
    {
        char *_narg;     // ID_COMMAND, ID_ICOMMAND
        int _val;        // ID_VAR
        char *_action;   // ID_ALIAS
    };
    bool _persist;       // ID_VAR, ID_ALIAS
    union
    {
        char *_isexecuting;  // ID_ALIAS
        int *_storage;       // ID_VAR
    };
    T *self;
    
    tident() {}
#ifdef BFRONTIER
    // ID_VAR
    tident(int t, char *n, int m, int c, int x, int *s, void *f = NULL, bool p = false, bool _srv = false, bool _wld = false)
        : _type(t), _name(n), _min(m), _max(x), _override(NO_OVERRIDE), _server(_srv), _world(_wld), _fun((void (__cdecl *)(void))f), _val(c), _persist(p), _storage(s) {}
    // ID_ALIAS
    tident(int t, char *n, char *a, bool p, bool _srv = false, bool _wld = false)
        : _type(t), _name(n), _override(NO_OVERRIDE), _server(_srv), _world(_wld), _stack(NULL), _action(a), _persist(p) {}
    // ID_COMMAND, ID_ICOMMAND
    tident(int t, char *n, char *narg, void *f = NULL, T *_s = NULL, bool _srv = false, bool _wld = false)
        : _type(t), _name(n), _server(_srv), _world(_wld), _fun((void (__cdecl *)(void))f), _narg(narg), self(_s) {}
#else
    // ID_VAR
    tident(int t, char *n, int m, int c, int x, int *s, void *f = NULL, bool p = false)
        : _type(t), _name(n), _min(m), _max(x), _override(NO_OVERRIDE), _fun((void (__cdecl *)(void))f), _val(c), _persist(p), _storage(s) {}
    // ID_ALIAS
    tident(int t, char *n, char *a, bool p)
        : _type(t), _name(n), _override(NO_OVERRIDE), _stack(NULL), _action(a), _persist(p) {}
    // ID_COMMAND, ID_ICOMMAND
    tident(int t, char *n, char *narg, void *f = NULL, T *_s = NULL)
        : _type(t), _name(n), _fun((void (__cdecl *)(void))f), _narg(narg), self(_s) {}
#endif
    virtual ~tident() {}        

    tident &operator=(const tident &o) { memcpy(this, &o, sizeof(tident)); return *this; }        // force vtable copy, ugh
    
    int operator()() { return _val; }
    
    virtual void run(char **args) {}
    virtual void changed() { if(_fun) _fun(); }
};

typedef tident<void> ident;

extern void addident(char *name, ident *id);
extern void intret(int v);
extern void result(const char *s);

// nasty macros for registering script functions, abuses globals to avoid excessive infrastructure
#ifdef BFRONTIER
// helpers and other misc stuff
typedef hashtable<char *, ident> identtable;
extern identtable *idents;

#define _COMMANDN(name, fun, nargs, server, world) static bool __dummy_##fun = addcommand(#name, (void (*)())fun, nargs, server, world)
#define COMMANDN(name, fun, nargs) _COMMANDN(name, fun, nargs, false, false)
#define COMMAND(name, nargs) _COMMANDN(name, name, nargs, false, false)
#define COMMANDWN(name, fun, nargs) _COMMANDN(name, fun, nargs, false, true)
#define COMMANDW(name, nargs) _COMMANDN(name, name, nargs, false, true)
#define _VAR(name, global, min, cur, max, persist, server, world)  int global = variable(#name, min, cur, max, &global, NULL, persist, server, world)
#define VARN(name, global, min, cur, max) _VAR(name, global, min, cur, max, false, false, false)
#define VAR(name, min, cur, max) _VAR(name, name, min, cur, max, false, false, false)
#define VARP(name, min, cur, max) _VAR(name, name, min, cur, max, true, false, false)
#define VARW(name, min, cur, max) _VAR(name, name, min, cur, max, false, false, true)
#define _VARF(name, global, min, cur, max, body, persist, server, world)  void var_##name(); int global = variable(#name, min, cur, max, &global, var_##name, persist, server, world); void var_##name() { body; }
#define VARFN(name, global, min, cur, max, body) _VARF(name, global, min, cur, max, body, false, false, false)
#define VARF(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, false, false, false)
#define VARFP(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, true, false, false)
#define VARFW(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, false, false, true)

#define _COMMAND(t, ts, sv, tv, n, g, b, s, w) struct cmd_##n : tident<t> { cmd_##n(ts) : tident<t>(ID_ICOMMAND, #n, g, NULL, sv, s, w) { addident(_name, (ident *)this); } void run(char **args) { b; } } icom_##n tv
#define ICOMMAND(n, g, b) _COMMAND(void, , NULL, , n, g, b, false, false)
#define ICOMMANDW(n, g, b) _COMMAND(void, , NULL, , n, g, b, false, true)
#define CCOMMAND(t, n, g, b) _COMMAND(t, t *_s, _s, (this), n, g, b, false, false)
#define CCOMMANDW(t, n, g, b) _COMMAND(t, t *_s, _s, (this), n, g, b, false, true)
 
#define _IVAR(n, m, c, x, b, p, s, w) struct var_##n : ident { var_##n() : ident(ID_VAR, #n, m, c, x, &_val, NULL, p, s, w) { addident(_name, this); } b } n
#define IVAR(n, m, c, x)  _IVAR(n, m, c, x, , false, false, false)
#define IVARF(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, false, false, false)
#define IVARP(n, m, c, x)  _IVAR(n, m, c, x, , true, false, false)
#define IVARW(n, m, c, x)  _IVAR(n, m, c, x, , false, false, true)
#define IVARFP(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, true, false, false)
#define IVARFW(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, true, false, true)

#define _CVAR(t, ts, sv, tv, n, m, c, x, b) \
	static struct var_##n : tident<t> { \
		var_##n(ts) : tident<t>(ID_VAR, #n, m, c, x, &_val) { \
			self = (t *)sv; \
			addident(_name, (ident *)this); \
		} \
		~var_##n() { } \
		b; \
	} ivar_##n tv;

#define CVAR(t, n, m, c, x)  _CVAR(t, t *_s, _s, (this), n, m, c, x, )
#define CVARF(t, n, m, c, x, b) _CVAR(t, t *_s, _s, (this), n, m, c, x, void changed() { b; })

#define _SCOMMAND(t, ts, sv, tv, n, v, g, b) \
	struct cmd_##n : tident<t> \
	{ \
		int _mode; \
		cmd_##n(ts) : tident<t>(ID_ICOMMAND, #n, g, NULL, sv, true) \
		{ \
			_mode = v; \
			addident(_name, (ident *)this); \
		} \
		void run(char **args) { b; } \
	} icom_##n tv;
#define SCOMMAND(t, n, v, g, b) _SCOMMAND(t, t *_s, _s, (this), n, v, g, b)
#else
#define COMMANDN(name, fun, nargs) static bool __dummy_##fun = addcommand(#name, (void (*)())fun, nargs)
#define COMMAND(name, nargs) COMMANDN(name, name, nargs)
#define _VAR(name, global, min, cur, max, persist)  int global = variable(#name, min, cur, max, &global, NULL, persist)
#define VARN(name, global, min, cur, max) _VAR(name, global, min, cur, max, false)
#define VAR(name, min, cur, max) _VAR(name, name, min, cur, max, false)
#define VARP(name, min, cur, max) _VAR(name, name, min, cur, max, true)
#define _VARF(name, global, min, cur, max, body, persist)  void var_##name(); int global = variable(#name, min, cur, max, &global, var_##name, persist); void var_##name() { body; }
#define VARFN(name, global, min, cur, max, body) _VARF(name, global, min, cur, max, body, false)
#define VARF(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, false)
#define VARFP(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, true)

// new style macros, have the body inline, and allow binds to happen anywhere, even inside class constructors, and access the surrounding class
#define _COMMAND(t, ts, sv, tv, n, g, b) struct cmd_##n : tident<t> { cmd_##n(ts) : tident<t>(ID_ICOMMAND, #n, g, NULL, sv) { addident(_name, (ident *)this); } void run(char **args) { b; } } icom_##n tv
#define ICOMMAND(n, g, b) _COMMAND(void, , NULL, , n, g, b)
#define CCOMMAND(t, n, g, b) _COMMAND(t, t *_s, _s, (this), n, g, b)
 
#define _IVAR(n, m, c, x, b, p) struct var_##n : ident { var_##n() : ident(ID_VAR, #n, m, c, x, &_val, NULL, p) { addident(_name, this); } b } n
#define IVAR(n, m, c, x)  _IVAR(n, m, c, x, , false)
#define IVARF(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, false)
#define IVARP(n, m, c, x)  _IVAR(n, m, c, x, , true)
#define IVARFP(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, true)
//#define ICALL(n, a) { char *args[] = a; icom_##n.run(args); }
//
#endif
