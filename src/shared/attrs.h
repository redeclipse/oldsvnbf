// attributes with types
enum
{
	ATTR_NONE = 0,
	ATTR_BOOL,
	ATTR_INT,
	ATTR_FLOAT,
	ATTR_MAX
};

struct attr
{
	const char *name;		// name of this attribute
	int type;				// the type of this attribute
	
	attr(const char *n = NULL, int t = ATTR_NONE) : name(n), type(t) {}
	~attr() {}
};

struct attr_bool : attr
{
	bool value;
	
	attr_bool(const char *n = NULL) : attr(n, ATTR_BOOL), value(false) {}
	~attr_bool() {}
};

struct attr_int : attr
{
	int value, minval, maxval;
	
	attr_int(const char *n = NULL) : attr(n, ATTR_INT), value(0), minval(0), maxval(0) {}
	~attr_int() {}
};

struct attr_float : attr
{
	float value, minval, maxval;
	
	attr_float(const char *n = NULL) : attr(n, ATTR_FLOAT), value(0.f), minval(0.f), maxval(0.f) {}
	~attr_float() {}
};

// macros

#define ATTR(a, o, n, v, f) \
	struct _attr_##n : attr_##o \
	{ \
		_attr_##n() : attr_##o(#n) \
		{ \
			loopv(a) if (!strcasecmp(a[i]->name, #n)) return; \
			f; \
			a.add(this); \
		} \
		~_attr_##n() \
		{ \
			a.removeobj(this); \
		} \
	} atr_##n;

#define ACASE(o, _n, _b, _i, _f) \
	switch (o->type) \
	{ \
		default: \
		case ATTR_NONE: \
		{ \
			attr *a = (attr *)o; \
			_n; \
			break; \
		} \
		case ATTR_BOOL: \
		{ \
			attr_bool *a = (attr_bool *)o; \
			_b; \
			break; \
		} \
		case ATTR_INT: \
		{ \
			attr_int *a = (attr_int *)o; \
			_i; \
			break; \
		} \
		case ATTR_FLOAT: \
		{ \
			attr_float *a = (attr_float *)o; \
			_f; \
			break; \
		} \
	}
	

// object types

#define ABOOL(a, n, v) \
	ATTR(a, bool, n, v, { value = (bool)v; } );

#define AINT(a, n, v, x, y) \
	ATTR(a, int, n, v, { value = (int)v; minval = (int)x; maxval = (int)y; } );

#define AFLOAT(a, n, v, x, y) \
	ATTR(a, float, n, v, { value = (float)v; minval = (float)x; maxval = (float)y; } );

