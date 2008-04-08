// script binding functionality


enum { ID_VAR, ID_FVAR, ID_SVAR, ID_COMMAND, ID_CCOMMAND, ID_ALIAS };

enum { NO_OVERRIDE = INT_MAX, OVERRIDDEN = 0 };

struct identstack
{
    char *action;
    identstack *next;
};

struct ident
{
    int type;           // one of ID_* above
    const char *name;
    int minval, maxval; // ID_VAR
    int override; // either NO_OVERRIDE, OVERRIDDEN, or value
    union
    {
        void (__cdecl *fun)(); // ID_VAR, ID_COMMAND, ID_CCOMMAND
        identstack *stack;     // ID_ALIAS
    };
    union
    {
        int i;      // ID_VAR
        float f;    // ID_FVAR
        char *s;    // ID_SVAR
    } val, overrideval;
    union
    {
        int *i;   // ID_VAR
        float *f; // ID_FVAR
        char **s; // ID_SVAR
    } storage;
    union
    {
        const char *narg; // ID_COMMAND, ID_CCOMMAND
        char *action;     // ID_ALIAS
    };
    union
    {
        void *self;        // ID_COMMAND, ID_CCOMMAND
        char *isexecuting; // ID_ALIAS
    };
    bool persist, world;       // ID_VAR, ID_ALIAS
    bool complete;

    ident() {}
    // ID_VAR
    ident(int t, const char *n, int m, int c, int x, int *s, void *f = NULL, bool p = false, bool w = false)
        : type(t), name(n), minval(m), maxval(x), override(NO_OVERRIDE), fun((void (__cdecl *)())f), persist(p), world(w), complete(true)
    { val.i = c; storage.i = s; }
    // ID_FVAR
    ident(int t, const char *n, float c, float *s, void *f = NULL, bool p = false, bool w = false)
        : type(t), name(n), override(NO_OVERRIDE), fun((void (__cdecl *)())f), persist(p), world(w), complete(true)
    { val.f = c; storage.f = s; }
    // ID_SVAR
    ident(int t, const char *n, char *c, char **s, void *f = NULL, bool p = false, bool w = false)
        : type(t), name(n), override(NO_OVERRIDE), fun((void (__cdecl *)())f), persist(p), world(w), complete(true)
    { val.s = c; storage.s = s; }
    // ID_ALIAS
    ident(int t, const char *n, char *a, bool p, bool w = false)
        : type(t), name(n), override(NO_OVERRIDE), stack(NULL), action(a), persist(p), world(w), complete(true) {}
    // ID_COMMAND, ID_CCOMMAND
    ident(int t, const char *n, const char *narg, void *f = NULL, void *s = NULL)
        : type(t), name(n), fun((void (__cdecl *)(void))f), narg(narg), self(s), complete(true) {}

    virtual ~ident() {}

    ident &operator=(const ident &o) { memcpy(this, &o, sizeof(ident)); return *this; }        // force vtable copy, ugh

    virtual void changed() { if(fun) fun(); }
};

extern void addident(const char *name, ident *id);
extern void intret(int v);
extern void result(const char *s);

typedef hashtable<const char *, ident> identtable;
extern identtable *idents;

// nasty macros for registering script functions, abuses globals to avoid excessive infrastructure
#define COMMANDN(name, fun, nargs) static bool __dummy_##fun = addcommand(#name, (void (*)())fun, nargs)
#define COMMAND(name, nargs) COMMANDN(name, name, nargs)

#define _VAR(name, global, min, cur, max, persist, world)  int global = variable(#name, min, cur, max, &global, NULL, persist, world)
#define VARN(name, global, min, cur, max) _VAR(name, global, min, cur, max, false, false)
#define VARNP(name, global, min, cur, max) _VAR(name, global, min, cur, max, true, false)
#define VARNW(name, global, min, cur, max) _VAR(name, global, min, cur, max, false, true)
#define VAR(name, min, cur, max) _VAR(name, name, min, cur, max, false, false)
#define VARP(name, min, cur, max) _VAR(name, name, min, cur, max, true, false)
#define VARW(name, min, cur, max) _VAR(name, name, min, cur, max, false, true)
#define _VARF(name, global, min, cur, max, body, persist, world)  void var_##name(); int global = variable(#name, min, cur, max, &global, var_##name, persist, world); void var_##name() { body; }
#define VARFN(name, global, min, cur, max, body) _VARF(name, global, min, cur, max, body, false, false)
#define VARF(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, false, false)
#define VARFP(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, true, false)
#define VARFW(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, false, true)

#define _FVAR(name, global, cur, persist, world) float global = fvariable(#name, cur, &global, NULL, persist, world)
#define FVARN(name, global, cur) _FVAR(name, global, cur, false, false)
#define FVARNP(name, global, cur) _FVAR(name, global, cur, true, false)
#define FVAR(name, cur) _FVAR(name, name, cur, false, false)
#define FVARP(name, cur) _FVAR(name, name, cur, true, false)
#define FVARW(name, cur) _FVAR(name, name, cur, false, true)
#define _FVARF(name, global, cur, body, persist, world) void var_##name(); float global = fvariable(#name, cur, &global, var_##name, persist, world); void var_##name() { body; }
#define FVARFN(name, global, cur, body) _FVARF(name, global, cur, body, false, false)
#define FVARF(name, cur, body) _FVARF(name, name, cur, body, false, false)
#define FVARFP(name, cur, body) _FVARF(name, name, cur, body, true, false)
#define FVARFW(name, cur, body) _FVARF(name, name, cur, body, false, true)

#define _SVAR(name, global, cur, persist, world) char *global = svariable(#name, cur, &global, NULL, persist, world)
#define SVARN(name, global, cur) _SVAR(name, global, cur, false, false)
#define SVARNP(name, global, cur) _SVAR(name, global, cur, true, false)
#define SVAR(name, cur) _SVAR(name, name, cur, false, false)
#define SVARP(name, cur) _SVAR(name, name, cur, true, false)
#define SVARW(name, cur) _SVAR(name, name, cur, false, true)
#define _SVARF(name, global, cur, body, persist, world) void var_##name(); char *global = svariable(#name, cur, &global, var_##name, persist, world); void var_##name() { body; }
#define SVARFN(name, global, cur, body) _SVARF(name, global, cur, body, false, false)
#define SVARF(name, cur, body) _SVARF(name, name, cur, body, false, false)
#define SVARFP(name, cur, body) _SVARF(name, name, cur, body, true, false)
#define SVARFW(name, cur, body) _SVARF(name, name, cur, body, false, true)

// new style macros, have the body inline, and allow binds to happen anywhere, even inside class constructors, and access the surrounding class
#define _COMMAND(idtype, tv, n, g, proto, b) \
    struct cmd_##n : ident \
    { \
        cmd_##n(void *self = NULL) : ident(idtype, #n, g, (void *)run, self) \
        { \
            addident(name, this); \
        } \
        static void run proto { b; } \
    } icom_##n tv
#define ICOMMAND(n, g, proto, b) _COMMAND(ID_COMMAND, , n, g, proto, b)
#define CCOMMAND(n, g, proto, b) _COMMAND(ID_CCOMMAND, (this), n, g, proto, b)

#define _IVAR(n, m, c, x, b, p, w) \
	struct var_##n : ident \
	{ \
        var_##n() : ident(ID_VAR, #n, m, c, x, &val.i, NULL, p, w) \
		{ \
            addident(name, this); \
		} \
        int operator()() { return val.i; } \
        b \
    } n
#define IVAR(n, m, c, x)  _IVAR(n, m, c, x, , false, false)
#define IVARF(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, false, false)
#define IVARP(n, m, c, x)  _IVAR(n, m, c, x, , true, false)
#define IVARFP(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, true, false)
#define IVARW(n, m, c, x)  _IVAR(n, m, c, x, , false, true)
#define IVARFW(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, false, true)
//#define ICALL(n, a) { char *args[] = a; icom_##n.run(args); }
//
