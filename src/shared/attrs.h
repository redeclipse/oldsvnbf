// attributes with types
enum
{
	ATTR_NONE = 0,
	ATTR_BOOL,
	ATTR_INT,
	ATTR_FLOAT,
	ATTR_STRING,
	ATTR_VEC,
	ATTR_IVEC,
	ATTR_MAX
};

struct attr
{
	const char *name;				// name of this attribute
	int type;						// the type of this attribute
	char *valstr, *minstr, *maxstr;	// so we can return something nice with get

	attr(const char *n = NULL, int t = ATTR_NONE) :
		name(n), type(t), valstr(NULL), minstr(NULL), maxstr(NULL) {}
	virtual ~attr() {}

	void destroy()
	{
		if (valstr) DELETEA(valstr);
		if (minstr) DELETEA(minstr);
		if (maxstr) DELETEA(maxstr);
	}

	virtual void getval() = 0;
	virtual void setval(const char *v, bool msg = false) = 0;
};

extern void printattrval(attr *a);
extern void printattrrange(attr *a);

struct attr_bool : attr
{
	bool value, minval, maxval;

	attr_bool(const char *n = NULL) :
		attr(n, ATTR_BOOL), value(false) {}
	~attr_bool() {}

	bool conval(const char *v)
	{
		bool retval = false;
		if (isnumeric(*v)) retval = atoi(v) ? true : false;
		else if (!strcasecmp("false", v)) retval = false;
		else if (!strcasecmp("true", v)) retval = true;
		else if (!strcasecmp("off", v)) retval = false;
		else if (!strcasecmp("on", v)) retval = true;
		return retval;
	}

	char *strval(bool v)
	{
		defformatstring(s)("%s", v ? "true" : "false");
		return newstring(s); // must delete yourself!
	}

	void setrange(const char *x, const char *y)
	{
		if (*x)
		{
			minval = conval(x);
			if (minstr) DELETEA(minstr);
			minstr = strval(minval);
		}
		if (*y)
		{
			maxval = conval(y);
			if (maxstr) DELETEA(maxstr);
			maxstr = strval(maxval);
		}
	}

	void setval(const char *v, bool msg = false)
	{
		if (*v)
		{
			bool i = conval(v);
			value = i; getval();
			if (msg) printattrval(this);
		}
		else if (msg) printattrval(this);
	}
};

struct attr_int : attr
{
	int value, minval, maxval;

	attr_int(const char *n = NULL) :
		attr(n, ATTR_INT), value(0), minval(0), maxval(0) {}
	~attr_int() {}

	int conval(const char *v)
	{
		int retval = atoi(v);
		return retval;
	}

	char *strval(int v)
	{
		defformatstring(s)("%d", v);
		return newstring(s); // must delete yourself!
	}

	void setrange(const char *x, const char *y)
	{
		if (*x)
		{
			minval = conval(x);
			if (minstr) DELETEA(minstr);
			minstr = strval(minval);
		}
		if (*y)
		{
			maxval = conval(y);
			if (maxstr) DELETEA(maxstr);
			maxstr = strval(maxval);
		}
	}

	void setval(const char *v, bool msg = false)
	{
		if (*v)
		{
			int i = conval(v);

			if (i > maxval || i < minval)
			{
				if (msg) printattrrange(this);
			}
			else
			{
				value = i; getval();
				if (msg) printattrval(this);
			}
		}
		else if (msg) printattrval(this);
	}
};

struct attr_float : attr
{
	float value, minval, maxval;

	attr_float(const char *n = NULL) :
		attr(n, ATTR_FLOAT), value(0.f), minval(0.f), maxval(0.f) {}
	~attr_float() {}

	float conval(const char *v)
	{
		float retval = atof(v);
		return retval;
	}

	char *strval(float v)
	{
		defformatstring(s)("%f", v);
		return newstring(s); // must delete yourself!
	}

	void setrange(const char *x, const char *y)
	{
		if (*x)
		{
			minval = conval(x);
			if (minstr) DELETEA(minstr);
			minstr = strval(minval);
		}
		if (*y)
		{
			maxval = conval(y);
			if (maxstr) DELETEA(maxstr);
			maxstr = strval(maxval);
		}
	}

	void setval(const char *v, bool msg = false)
	{
		if (*v)
		{
			float i = conval(v);

			if (i > maxval || i < minval)
			{
				if (msg) printattrrange(this);
			}
			else
			{
				value = i; getval();
				if (msg) printattrval(this);
			}
		}
		else if (msg) printattrval(this);
	}
};

struct attr_string : attr
{
	char *value, *retval;
	size_t minval, maxval;

	attr_string(const char *n = NULL) :
		attr(n, ATTR_STRING), value(NULL), retval(NULL), minval(0), maxval(1) {}
	~attr_string() {}

	char *conval(const char *v)
	{
		if (retval) DELETEA(retval);
		retval = newstring(v);
		return retval;
	}

	char *strval(char *v)
	{
		return newstring(v); // must delete yourself!
	}

	void setrange(const char *x, const char *y)
	{
		if (*x)
		{
			minval = atoi(x);
			if (minstr) DELETEA(minstr);
			defformatstring(s)("%d", minval);
			minstr = newstring(s);
		}
		if (*y)
		{
			maxval = atoi(y);
			if (maxstr) DELETEA(maxstr);
			defformatstring(s)("%d", maxval);
			maxstr = newstring(s);
		}
	}

	void setval(const char *v, bool msg = false)
	{
		if (*v)
		{
			if (strlen(v) > maxval || strlen(v) < minval)
			{
				if (msg) printattrrange(this);
			}
			else
			{
				value = newstring(v); getval();
				if (msg) printattrval(this);
			}
		}
		else if (msg) printattrval(this);
	}
};

struct attr_vec : attr
{
	vec value, minval, maxval;

	attr_vec(const char *n = NULL) :
		attr(n, ATTR_VEC), value(0, 0, 0), minval(0, 0, 0), maxval(0, 0, 0) {}
	~attr_vec() {}

	vec conval(const char *v)
	{
		vec retval = vec(0, 0, 0);
		v += strspn(v, ",");
		int q = 0;

		while (*v)
		{
			float i = atof(v);

			retval.v[q] = i;

			if ((q += 1) > 2) break;

			v += strcspn(v, ",\0");
			v += strspn(v, ",");
		}
		return retval;
	}

	char *strval(vec &v)
	{
		defformatstring(s)("%f,%f,%f", v.x, v.y, v.z);
		return newstring(s); // must delete yourself!
	}

	void setrange(const char *x, const char *y)
	{
		if (*x)
		{
			minval = conval(x);
			if (minstr) DELETEA(minstr);
			minstr = strval(minval);
		}
		if (*y)
		{
			maxval = conval(y);
			if (maxstr) DELETEA(maxstr);
			maxstr = strval(maxval);
		}
	}

	void setval(const char *v, bool msg = false)
	{
		if (*v)
		{
			vec i = conval(v);

			if (i.x > maxval.x || i.x < minval.x ||
				i.y > maxval.y || i.y < minval.y ||
				i.z > maxval.z || i.z < minval.z)
			{
				if (msg) printattrrange(this);
			}
			else
			{
				value = i; getval();
				if (msg) printattrval(this);
			}
		}
		else if (msg) printattrval(this);
	}
};

struct attr_ivec : attr
{
	ivec value, minval, maxval;

	attr_ivec(const char *n = NULL) :
		attr(n, ATTR_IVEC), value(0, 0, 0), minval(0, 0, 0), maxval(0, 0, 0) {}
	~attr_ivec() {}

	ivec conval(const char *v)
	{
		ivec retval = ivec(0, 0, 0);
		v += strspn(v, ",");
		int q = 0;

		while (*v)
		{
			int i = atoi(v);

			retval.v[q] = i;

			if ((q += 1) > 2) break;

			v += strcspn(v, ",\0");
			v += strspn(v, ",");
		}
		return retval;
	}

	char *strval(ivec &v)
	{
		defformatstring(s)("%f,%f,%f", v.x, v.y, v.z);
		return newstring(s); // must delete yourself!
	}

	void setrange(const char *x, const char *y)
	{
		if (*x)
		{
			minval = conval(x);
			if (minstr) DELETEA(minstr);
			minstr = strval(minval);
		}
		if (*y)
		{
			maxval = conval(y);
			if (maxstr) DELETEA(maxstr);
			maxstr = strval(maxval);
		}
	}

	void setval(const char *v, bool msg = false)
	{
		if (*v)
		{
			ivec i = conval(v);

			if (i.x > maxval.x || i.x < minval.x ||
				i.y > maxval.y || i.y < minval.y ||
				i.z > maxval.z || i.z < minval.z)
			{
				if (msg) printattrrange(this);
			}
			else
			{
				value = i; getval();
				if (msg) printattrval(this);
			}
		}
		else if (msg) printattrval(this);
	}
};

// macros

#define ATTR(a, o, n, v, x, y) \
	struct _attr_##n : o \
	{ \
		_attr_##n() : o(#n) \
		{ \
			loopv(a) if (!strcasecmp(a[i]->name, #n)) return; \
			setrange(x, y); \
			setval(v); \
			a.add(this); \
		} \
		~_attr_##n() \
		{ \
			a.removeobj(this); \
			destroy(); \
		} \
		\
		void getval() \
		{ \
			if (valstr) DELETEA(valstr); \
			valstr = strval(value); \
		} \
	} atr_##n;

#define ACASE(o, _n, _b, _i, _f, _s, _v, _iv) \
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
		case ATTR_STRING: \
		{ \
			attr_string *a = (attr_string *)o; \
			_s; \
			break; \
		} \
		case ATTR_VEC: \
		{ \
			attr_vec *a = (attr_vec *)o; \
			_v; \
			break; \
		} \
		case ATTR_IVEC: \
		{ \
			attr_ivec *a = (attr_ivec *)o; \
			_iv; \
			break; \
		} \
	}


// object types

#define ABOOL(a, n, v)			ATTR(a, attr_bool, n, v, "false", "true");
#define AINT(a, n, v, x, y)		ATTR(a, attr_int, n, #v, #x, #y);
#define AFLOAT(a, n, v, x, y)	ATTR(a, attr_float, n, #v, #x, #y);
#define ASTRING(a, n, v, x, y)	ATTR(a, attr_string, n, v, #x, #y);
#define AVEC(a, n, v, x, y)		ATTR(a, attr_vec, n, v, x, y);
#define AIVEC(a, n, v, x, y)	ATTR(a, attr_ivec, n, v, x, y);
