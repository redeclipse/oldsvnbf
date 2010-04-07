// script binding functionality


enum { ID_VAR, ID_FVAR, ID_SVAR, ID_COMMAND, ID_CCOMMAND, ID_ALIAS };

enum { NO_OVERRIDE = INT_MAX, OVERRIDDEN = 0 };

enum { IDF_PERSIST = 1<<0, IDF_OVERRIDE = 1<<1, IDF_WORLD = 1<<2, IDF_COMPLETE = 1<<3, IDF_TEXTURE = 1<<4, IDF_CLIENT = 1<<5, IDF_SERVER = 1<<6, IDF_HEX = 1<<7, IDF_ADMIN = 1<<8 };

struct identstack
{
    char *action;
    identstack *next;
};

union identval
{
    int i;      // ID_VAR
    float f;    // ID_FVAR
    char *s;    // ID_SVAR
};

union identvalptr
{
    int *i;   // ID_VAR
    float *f; // ID_FVAR
    char **s; // ID_SVAR
};

struct ident
{
    int type;           // one of ID_* above
    const char *name;
    union
    {
        int minval;    // ID_VAR
        float minvalf; // ID_FVAR
    };
    union
    {
        int maxval;    // ID_VAR
        float maxvalf; // ID_FVAR
    };
    int override; // either NO_OVERRIDE, OVERRIDDEN, or value
    union
    {
        void (__cdecl *fun)(); // ID_VAR, ID_COMMAND, ID_CCOMMAND
        identstack *stack;     // ID_ALIAS
    };
    union
    {
        const char *narg; // ID_COMMAND, ID_CCOMMAND
        char *action;     // ID_ALIAS
        identval val;// ID_VAR, ID_FVAR, ID_SVAR
    };
    union
    {
        void *self;        // ID_COMMAND, ID_CCOMMAND
        char *isexecuting; // ID_ALIAS
        identval overrideval; // ID_VAR, ID_FVAR, ID_SVAR
    };
    identval def;
    identvalptr storage; // ID_VAR, ID_FVAR, ID_SVAR
    int flags;

    ident() {}
    // ID_VAR
    ident(int t, const char *n, int m, int c, int x, int *s, void *f, int flags = IDF_COMPLETE)
        : type(t), name(n), minval(m), maxval(x), override(NO_OVERRIDE), fun((void (__cdecl *)())f), flags(flags)
    { val.i = def.i = c; storage.i = s; }
    // ID_FVAR
    ident(int t, const char *n, float m, float c, float x, float *s, void *f, int flags = IDF_COMPLETE)
        : type(t), name(n), minvalf(m), maxvalf(x), override(NO_OVERRIDE), fun((void (__cdecl *)())f), flags(flags)
    { val.f = def.f = c; storage.f = s; }
    // ID_SVAR
    ident(int t, const char *n, char *d, char *c, char **s, void *f, int flags = IDF_COMPLETE)
        : type(t), name(n), override(NO_OVERRIDE), fun((void (__cdecl *)())f), flags(flags)
    { val.s = c; def.s = d; storage.s = s; }
    // ID_ALIAS
    ident(int t, const char *n, char *a, int flags)
        : type(t), name(n), override(NO_OVERRIDE), stack(NULL), action(a), isexecuting(NULL), flags(flags) {}
    // ID_COMMAND, ID_CCOMMAND
    ident(int t, const char *n, const char *narg, void *f, void *s, int flags = IDF_COMPLETE)
        : type(t), name(n), override(NO_OVERRIDE), fun((void (__cdecl *)(void))f), narg(narg), self(s), flags(flags) {}

    virtual ~ident() {}

    ident &operator=(const ident &o) { memcpy(this, &o, sizeof(ident)); return *this; }        // force vtable copy, ugh

    virtual void changed() { if(fun) fun(); }
};

extern void addident(const char *name, ident *id);
extern const char *intstr(int v);
extern void intret(int v);
extern const char *floatstr(float v);
extern void floatret(float v);
extern void result(const char *s);

typedef hashtable<const char *, ident> identtable;
extern identtable *idents;

extern int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), int flags);
extern float fvariable(const char *name, float min, float cur, float max, float *storage, void (*fun)(), int flags);
extern char *svariable(const char *name, const char *cur, char **storage, void (*fun)(), int flags);
extern void setvar(const char *name, int i, bool dofunc = false);
extern void setfvar(const char *name, float f, bool dofunc = false);
extern void setsvar(const char *name, const char *str, bool dofunc = false);
extern void setvarchecked(ident *id, int val);
extern void setfvarchecked(ident *id, float val);
extern void setsvarchecked(ident *id, const char *val);
extern void touchvar(const char *name);
extern int getvar(const char *name);
extern int getvarmin(const char *name);
extern int getvarmax(const char *name);
extern int getvardef(const char *name);
extern bool identexists(const char *name);
extern ident *getident(const char *name);
extern ident *newident(const char *name);
extern bool addcommand(const char *name, void (*fun)(), const char *narg, int flags = IDF_COMPLETE);

extern int execute(const char *p);
extern char *executeret(const char *p);
extern bool execfile(const char *cfgfile, bool msg = true);
extern void alias(const char *name, const char *action);
extern void worldalias(const char *name, const char *action);
extern const char *getalias(const char *name);

extern bool overrideidents, worldidents, interactive;

extern char *parseword(const char *&p, int arg, int &infix);
extern char *parsetext(const char *&p);
extern void writeescapedstring(stream *f, const char *s);
extern void explodelist(const char *s, vector<char *> &elems);
extern int listlen(const char *s);
extern char *indexlist(const char *s, int pos);

extern int sortidents(ident **x, ident **y);
extern void writeescapedstring(stream *f, const char *s);

extern void clearoverrides();
extern void checksleep(int millis);
extern void clearsleep(bool clearoverrides = true, bool clearworlds = false);

// nasty macros for registering script functions, abuses globals to avoid excessive infrastructure
#define COMMANDN(flags, name, fun, nargs) static bool __dummy_##fun = addcommand(#name, (void (*)())fun, nargs, flags|IDF_COMPLETE)
#define COMMAND(flags, name, nargs) COMMANDN(flags, name, name, nargs)

#define _VAR(name, global, min, cur, max, flags) int global = variable(#name, min, cur, max, &global, NULL, flags|IDF_COMPLETE)
#define VARN(flags, name, global, min, cur, max) _VAR(name, global, min, cur, max, flags)
#define VAR(flags, name, min, cur, max) _VAR(name, name, min, cur, max, flags)
#define _VARF(name, global, min, cur, max, body, flags)  void var_##name(); int global = variable(#name, min, cur, max, &global, var_##name, flags|IDF_COMPLETE); void var_##name() { body; }
#define VARFN(flags, name, global, min, cur, max, body) _VARF(name, global, min, cur, max, body, flags)
#define VARF(flags, name, min, cur, max, body) _VARF(name, name, min, cur, max, body, flags)

#define _FVAR(name, global, min, cur, max, flags) float global = fvariable(#name, min, cur, max, &global, NULL, flags|IDF_COMPLETE)
#define FVARN(flags, name, global, min, cur, max) _FVAR(name, global, min, cur, max, flags)
#define FVAR(flags, name, min, cur, max) _FVAR(name, name, min, cur, max, flags)
#define _FVARF(name, global, min, cur, max, body, flags) void var_##name(); float global = fvariable(#name, min, cur, max, &global, var_##name, flags|IDF_COMPLETE); void var_##name() { body; }
#define FVARFN(flags, name, global, min, cur, max, body) _FVARF(name, global, min, cur, max, body, flags)
#define FVARF(flags, name, min, cur, max, body) _FVARF(name, name, min, cur, max, body, flags)

#define _SVAR(name, global, cur, flags) char *global = svariable(#name, cur, &global, NULL, flags|IDF_COMPLETE)
#define SVARN(flags, name, global, cur) _SVAR(name, global, cur, flags)
#define SVAR(flags, name, cur) _SVAR(name, name, cur, flags)
#define _SVARF(name, global, cur, body, flags) void var_##name(); char *global = svariable(#name, cur, &global, var_##name, flags|IDF_COMPLETE); void var_##name() { body; }
#define SVARFN(flags, name, global, cur, body) _SVARF(name, global, cur, body, flags)
#define SVARF(flags, name, cur, body) _SVARF(name, name, cur, body, flags)

// new style macros, have the body inline, and allow binds to happen anywhere, even inside class constructors, and access the surrounding class
#define _COMMAND(idtype, tv, n, m, g, proto, b, f) \
    struct cmd_##m : ident \
    { \
        cmd_##m(void *self = NULL) : ident(idtype, #n, g, (void *)run, self, f|IDF_COMPLETE) \
        { \
            addident(name, this); \
        } \
        static void run proto { b; } \
    } icom_##m tv
#define ICOMMAND(f, n, g, proto, b) _COMMAND( ID_COMMAND, , n, n, g, proto, b, f)
#define ICOMMANDN(f, n, name, g, proto, b) _COMMAND(ID_COMMAND, , n, name, g, proto, b, f)
#define CCOMMAND(f, n, g, proto, b) _COMMAND(ID_CCOMMAND, (this), n, n, g, proto, b, f)
#define CCOMMANDN(f, n, name, g, proto, b) _COMMAND(ID_CCOMMAND, (this), n, name, g, proto, b, f)

#define _IVAR(n, m, c, x, b, f) \
    struct var_##n : ident \
    { \
        var_##n() : ident(ID_VAR, #n, m, c, x, &val.i, (void *)NULL, f|IDF_COMPLETE) \
        { \
            addident(name, this); \
        } \
        int operator()() { return val.i; } \
        b \
    } n
#define IVAR(f, n, m, c, x)  _IVAR(n, m, c, x, , f)
#define IVARF(f, n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, f)

#define _IFVAR(n, m, c, x, b, f) \
    struct var_##n : ident \
    { \
        var_##n() : ident(ID_FVAR, #n, m, c, x, &val.f, (void *)NULL, f|IDF_COMPLETE) \
        { \
            addident(name, this); \
        } \
        float operator()() { return val.f; } \
        b \
    } n
#define IFVAR(f, n, m, c, x)  _IFVAR(n, m, c, x, , f)
#define IFVARF(f, n, m, c, x, b) _IFVAR(n, m, c, x, void changed() { b; }, f)

#define _ISVAR(n, c, b, f) \
    struct var_##n : ident \
    { \
        var_##n() : ident(ID_SVAR, #n, newstring(c), newstring(c), &val.s, (void *)NULL, f|IDF_COMPLETE) \
        { \
            addident(name, this); \
        } \
        char *operator()() { return val.s; } \
        b \
    } n
#define ISVAR(f, n, c)  _ISVAR(n, c, , f)
#define ISVARF(f, n, c, b) _ISVAR(n, c, void changed() { b; }, f)

// game world controlling stuff
#define RUNWORLD(n) { ident *wid = idents->access(n); if(wid && wid->action && wid->flags&IDF_WORLD) { bool _oldworldidents = worldidents; worldidents = true; execute(wid->action); worldidents = _oldworldidents; }; }

#if defined(GAMEWORLD)
#define IDF_GAME IDF_CLIENT
#define GAME(name) (name)
#define PHYS(name) (force##name >= 0 ? force##name : physics::name)
#define GICOMMAND(flags, n, g, proto, svbody, ccbody) _COMMAND(ID_COMMAND, , n, n, g, proto, ccbody, flags|IDF_GAME)
#define GICOMMANDN(flags, n, name, g, proto, svbody, ccbody) _COMMAND(ID_COMMAND, , n, name, g, proto, ccbody, flags|IDF_GAME)
#define GCCOMMAND(flags, n, g, proto, svbody, ccbody) _COMMAND(ID_CCOMMAND, (this), n, n, g, proto, ccbody, flags|IDF_GAME)
#define GCCOMMANDN(flags, n, name, g, proto, svbody, ccbody) _COMMAND(ID_CCOMMAND, (this), n, name, g, proto, ccbody, flags|IDF_GAME)
#define GVARN(flags, name, global, min, cur, max) _VAR(name, global, min, cur, max, flags|IDF_GAME)
#define GVAR(flags, name, min, cur, max) _VAR(name, name, min, cur, max, flags|IDF_GAME)
#define GVARF(flags, name, min, cur, max, svbody, ccbody) _VARF(name, name, min, cur, max, ccbody, flags|IDF_GAME)
#define GFVARN(flags, name, global, min, cur, max) _FVAR(name, global, min, cur, max, flags|IDF_GAME)
#define GFVAR(flags, name, min, cur, max) _FVAR(name, name, min, cur, max, flags|IDF_GAME)
#define GFVARF(flags, name, min, cur, max, svbody, ccbody) _FVARF(name, name, min, cur, max, ccbody, flags|IDF_GAME)
#define GSVARN(flags, name, global, cur) _SVAR(name, global, cur, flags|IDF_GAME)
#define GSVAR(flags, name, cur) _SVAR(name, name, cur, flags|IDF_GAME)
#define GSVARF(flags, name, cur, svbody, ccbody) _SVARF(name, name, cur, ccbody, flags|IDF_GAME)
#elif defined(GAMESERVER)
#define GAME(name) (sv_##name)
#define IDF_GAME IDF_SERVER
#define GICOMMAND(flags, n, g, proto, svbody, ccbody) _COMMAND(ID_COMMAND, , sv_##n, sv_##n, g, proto, svbody, flags|IDF_GAME)
#define GICOMMANDN(flags, n, name, g, proto, svbody, ccbody) _COMMAND(ID_COMMAND, , sv_##n, name, g, proto, svbody, flags|IDF_GAME)
#define GCCOMMAND(flags, n, g, proto, svbody, ccbody) _COMMAND(ID_CCOMMAND, (this), sv_##n, sv_##n, g, proto, svbody, flags|IDF_GAME)
#define GCCOMMANDN(flags, n, name, g, proto, svbody, ccbody) _COMMAND(ID_CCOMMAND, (this), sv_##n, name, g, proto, svbody, flags|IDF_GAME)
#define GVARN(flags, name, global, min, cur, max) _VAR(sv_##name, global, min, cur, max, flags|IDF_GAME)
#define GVAR(flags, name, min, cur, max) _VAR(sv_##name, sv_##name, min, cur, max, flags|IDF_GAME)
#define GVARF(flags, name, min, cur, max, svbody, ccbody) _VARF(sv_##name, sv_##name, min, cur, max, svbody, flags|IDF_GAME)
#define GFVARN(flags, name, global, min, cur, max) _FVAR(sv_##name, global, min, cur, max, flags|IDF_GAME)
#define GFVAR(flags, name, min, cur, max) _FVAR(sv_##name, sv_##name, min, cur, max, flags|IDF_GAME)
#define GFVARF(flags, name, min, cur, max, svbody, ccbody) _FVARF(sv_##name, sv_##name, min, cur, max, svbody, flags|IDF_GAME)
#define GSVARN(flags, name, global, cur) _SVAR(sv_##name, global, cur, flags|IDF_GAME)
#define GSVAR(flags, name, cur) _SVAR(sv_##name, sv_##name, cur, flags|IDF_GAME)
#define GSVARF(flags, name, cur, svbody, ccbody) _SVARF(sv_##name, sv_##name, cur, svbody, flags|IDF_GAME)
#else
#define GAME(name) (name)
#define PHYS(name) (force##name >= 0 ? force##name : physics::name)
#define GICOMMAND(flags, n, g, proto, svbody, ccbody)
#define GICOMMANDN(flags, n, name, g, proto, svbody, ccbody)
#define GCCOMMAND(flags, n, g, proto, svbody, ccbody)
#define GCCOMMANDN(flags, n, name, g, proto, svbody, ccbody)
#define GVARN(flags, name, global, min, cur, max) extern int name
#define GVAR(flags, name, min, cur, max) extern int name
#define GVARF(flags, name, min, cur, max, svbody, ccbody) extern int name
#define GVARN(flags, name, global, min, cur, max) extern int name
#define GVAR(flags, name, min, cur, max) extern int name
#define GVARF(flags, name, min, cur, max, svbody, ccbody) extern int name
#define GFVARN(flags, name, global, min, cur, max) extern float name
#define GFVAR(flags, name, min, cur, max) extern float name
#define GFVARF(flags, name, min, cur, max, svbody, ccbody) extern float name
#define GSVARN(flags, name, global, cur) extern char *name
#define GSVAR(flags, name, cur) extern char *name
#define GSVARF(flags, name, cur, svbody, ccbody) extern char *name
#endif
