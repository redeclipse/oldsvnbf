
struct vec
{
    union
    {
        struct { float x, y, z; };
        float v[3];
    };

    vec() {}
    vec(int a) : x(a), y(a), z(a) {}
    vec(float a) : x(a), y(a), z(a) {}
    vec(float a, float b, float c) : x(a), y(b), z(c) {}
    vec(int v[3]) : x(v[0]), y(v[1]), z(v[2]) {}
    vec(float *v) : x(v[0]), y(v[1]), z(v[2]) {}

    vec(float yaw, float pitch) : x(sinf(yaw)*cosf(pitch)), y(-cosf(yaw)*cosf(pitch)), z(sinf(pitch)) {}

    float &operator[](int i)       { return v[i]; }
    float  operator[](int i) const { return v[i]; }

    vec &set(int i, float f) { v[i] = f; return *this; }

    bool operator==(const vec &o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const vec &o) const { return x != o.x || y != o.y || z != o.z; }

    bool iszero() const { return x==0 && y==0 && z==0; }
    float squaredlen() const { return x*x + y*y + z*z; }
    float dot(const vec &o) const { return x*o.x + y*o.y + z*o.z; }
	vec &abs()               { x = fabs(x); y = fabs(y); z = fabs(z); return *this; }
	vec &mul(const vec &o)   { x *= o.x; y *= o.y; z *= o.z; return *this; }
	vec &mul(float f)        { x *= f; y *= f; z *= f; return *this; }
	vec &div(const vec &o)   { x /= o.x; y /= o.y; z /= o.z; return *this; }
	vec &div(float f)        { x /= f; y /= f; z /= f; return *this; }
    vec &add(const vec &o)   { x += o.x; y += o.y; z += o.z; return *this; }
    vec &add(float f)        { x += f; y += f; z += f; return *this; }
    vec &sub(const vec &o)   { x -= o.x; y -= o.y; z -= o.z; return *this; }
    vec &sub(float f)        { x -= f; y -= f; z -= f; return *this; }
    vec &neg()               { return mul(-1); }
    float magnitude() const  { return sqrtf(squaredlen()); }
    vec &normalize()         { div(magnitude()); return *this; }
    bool isnormalized() const { float m = squaredlen(); return (m>0.99f && m<1.01f); }
    float dist(const vec &e) const { vec t; return dist(e, t); }
    float dist(const vec &e, vec &t) const { t = *this; t.sub(e); return t.magnitude(); }
    bool reject(const vec &o, float max) { return x>o.x+max || x<o.x-max || y>o.y+max || y<o.y-max; }
    vec &cross(const vec &a, const vec &b) { x = a.y*b.z-a.z*b.y; y = a.z*b.x-a.x*b.z; z = a.x*b.y-a.y*b.x; return *this; }
	vec &apply(const vec &o, float elasticity = 1.0f)
	{
		x = ((o.x > 0.f && x < 0.f) || (o.x < 0.f && x > 0.f) ? fabs(x) * o.x : x) * elasticity;
		y = ((o.y > 0.f && y < 0.f) || (o.y < 0.f && y > 0.f) ? fabs(y) * o.y : y) * elasticity;
		z = ((o.z > 0.f && z < 0.f) || (o.z < 0.f && z > 0.f) ? fabs(z) * o.z : z) * elasticity;
		return *this;
	}
	vec &influence(const vec &o, const vec &p, float elasticity = 1.0f)
	{
		if ((o.x >= 0.f && p.x > 0.f) || (o.x <= 0.f && p.x < 0.f)) x += p.x * elasticity;
		if ((o.y >= 0.f && p.y > 0.f) || (o.y <= 0.f && p.y < 0.f)) y += p.y * elasticity;
		if ((o.z >= 0.f && p.z > 0.f) || (o.z <= 0.f && p.z < 0.f)) z += p.z * elasticity;
		return *this;
	}

    void rotate_around_z(float angle) { *this = vec(cosf(angle)*x-sinf(angle)*y, cosf(angle)*y+sinf(angle)*x, z); }
    void rotate_around_x(float angle) { *this = vec(x, cosf(angle)*y-sinf(angle)*z, cosf(angle)*z+sinf(angle)*y); }
    void rotate_around_y(float angle) { *this = vec(cosf(angle)*x-sinf(angle)*z, y, cosf(angle)*z+sinf(angle)*x); }

    void rotate(float angle, const vec &d)
    {
        float c = cosf(angle), s = sinf(angle);
        rotate(c, s, d);
    }

    void rotate(float c, float s, const vec &d)
    {
        *this = vec(x*(d.x*d.x*(1-c)+c) + y*(d.x*d.y*(1-c)-d.z*s) + z*(d.x*d.z*(1-c)+d.y*s),
                    x*(d.y*d.x*(1-c)+d.z*s) + y*(d.y*d.y*(1-c)+c) + z*(d.y*d.z*(1-c)-d.x*s),
                    x*(d.x*d.z*(1-c)-d.y*s) + y*(d.y*d.z*(1-c)+d.x*s) + z*(d.z*d.z*(1-c)+c));
    }

    void orthogonal(const vec &d)
    {
        int i = fabs(d.x) > fabs(d.y) ? (fabs(d.x) > fabs(d.z) ? 0 : 2) : (fabs(d.y) > fabs(d.z) ? 1 : 2);
        v[i] = d[(i+1)%3];
        v[(i+1)%3] = -d[i];
        v[(i+2)%3] = 0;
    }

    template<class T> float dist_to_bb(const T &min, const T &max) const
    {
        float sqrdist = 0;
        loopi(3)
        {
            if     (v[i] < min[i]) { float delta = v[i]-min[i]; sqrdist += delta*delta; }
            else if(v[i] > max[i]) { float delta = max[i]-v[i]; sqrdist += delta*delta; }
        }
        return sqrtf(sqrdist);
    }

    template<class T, class S> float dist_to_bb(const T &o, S size) const
    {
        return dist_to_bb(o, T(o).add(size));
    }
};

struct vec4 : vec
{
    float w;
    vec4() : w(0) {}
    vec4(vec &_v, float _w = 0) : w(_w) { *(vec *)this = _v; }
};

struct matrix
{
    vec X, Y, Z;

    matrix() {}
    matrix(const vec &_X, const vec &_Y, const vec &_Z) : X(_X), Y(_Y), Z(_Z) {}

    void transform(vec &o) { o = vec(o.dot(X), o.dot(Y), o.dot(Z)); }

    void orthonormalize()
    {
        X.sub(vec(Z).mul(Z.dot(X)));
        Y.sub(vec(Z).mul(Z.dot(Y)))
         .sub(vec(X).mul(X.dot(Y)));
    }

    void rotate(float angle, const vec &d)
    {
        float c = cosf(angle), s = sinf(angle);
        rotate(c, s, d);
    }

    void rotate(float c, float s, const vec &d)
    {
        X = vec(d.x*d.x*(1-c)+c, d.x*d.y*(1-c)-d.z*s, d.x*d.z*(1-c)+d.y*s);
        Y = vec(d.y*d.x*(1-c)+d.z*s, d.y*d.y*(1-c)+c, d.y*d.z*(1-c)-d.x*s);
        Z = vec(d.x*d.z*(1-c)-d.y*s, d.y*d.z*(1-c)+d.x*s, d.z*d.z*(1-c)+c);
    }

    void transposedtransform(vec &d)
    {
        d = vec(X.x*d.x + Y.x*d.y + Z.x*d.z,
                X.y*d.x + Y.y*d.y + Z.y*d.z,
                X.z*d.x + Y.z*d.y + Z.z*d.z);
    }
};

struct plane : vec
{
    float offset;

    float dist(const vec &p) const { return dot(p)+offset; }
    bool operator==(const plane &p) const { return x==p.x && y==p.y && z==p.z && offset==p.offset; }
    bool operator!=(const plane &p) const { return x!=p.x || y!=p.y || z!=p.z || offset!=p.offset; }

    plane() {}
    plane(vec &c, float off) : vec(c), offset(off) {}
    plane(int d, float off)
    {
        x = y = z = 0.0f;
        v[d] = 1.0f;
        offset = -off;
    }
    plane(float a, float b, float c, float d) : vec(a, b, c), offset(d) {}

    void toplane(const vec &n, const vec &p)
    {
        x = n.x; y = n.y; z = n.z;
        offset = -dot(p);
    }

    bool toplane(const vec &a, const vec &b, const vec &c)
    {
        cross(vec(b).sub(a), vec(c).sub(a));
        float mag = magnitude();
        if(!mag) return false;
        div(mag);
        offset = -dot(a);
        return true;
    }

    bool rayintersect(const vec &o, const vec &ray, float &dist)
    {
        float cosalpha = dot(ray);
        if(cosalpha==0) return false;
        float deltac = offset+dot(o);
        dist -= deltac/cosalpha;
        return true;
    }

    float zintersect(const vec &p) const { return -(x*p.x+y*p.y+offset)/z; }
    float zdist(const vec &p) const { return p.z-zintersect(p); }
};

struct triangle
{
    vec a, b, c;

    triangle(const vec &a, const vec &b, const vec &c) : a(a), b(b), c(c) {}
    triangle() {}

    triangle &add(const vec &o) { a.add(o); b.add(o); c.add(o); return *this; }
    triangle &sub(const vec &o) { a.sub(o); b.sub(o); c.sub(o); return *this; }

    bool operator==(const triangle &t) const { return a == t.a && b == t.b && c == t.c; }
};

/**

Sauerbraten uses 3 different linear coordinate systems
which are oriented around each of the axis dimensions.

So any point within the game can be defined by four coordinates: (d, x, y, z)

d is the reference axis dimension
x is the coordinate of the ROW dimension
y is the coordinate of the COL dimension
z is the coordinate of the reference dimension (DEPTH)

typically, if d is not used, then it is implicitly the Z dimension.
ie: d=z => x=x, y=y, z=z

**/

// DIM: X=0 Y=1 Z=2.
const int R[3] = {1, 2, 0}; // row
const int C[3] = {2, 0, 1}; // col
const int D[3]  = {0, 1, 2}; // depth

struct ivec
{
    union
    {
        struct { int x, y, z; };
        int v[3];
    };

    ivec() {}
    ivec(const vec &v) : x(int(v.x)), y(int(v.y)), z(int(v.z)) {}
    ivec(int i)
    {
        x = ((i&1)>>0);
        y = ((i&2)>>1);
        z = ((i&4)>>2);
    }
    ivec(int a, int b, int c) : x(a), y(b), z(c) {}
    ivec(int d, int row, int col, int depth)
    {
        v[R[d]] = row;
        v[C[d]] = col;
        v[D[d]] = depth;
    }
    ivec(int i, int cx, int cy, int cz, int size)
    {
        x = cx+((i&1)>>0)*size;
        y = cy+((i&2)>>1)*size;
        z = cz+((i&4)>>2)*size;
    }
    vec tovec() const { return vec(x, y, z); }
    int toint() const { return (x>0?1:0) + (y>0?2:0) + (z>0?4:0); }

    int &operator[](int i)       { return v[i]; }
    int  operator[](int i) const { return v[i]; }

    //int idx(int i) { return v[i]; }
    bool operator==(const ivec &v) const { return x==v.x && y==v.y && z==v.z; }
    bool operator!=(const ivec &v) const { return x!=v.x || y!=v.y || z!=v.z; }
    ivec &shl(int n) { x<<= n; y<<= n; z<<= n; return *this; }
    ivec &shr(int n) { x>>= n; y>>= n; z>>= n; return *this; }
    ivec &mul(int n) { x *= n; y *= n; z *= n; return *this; }
    ivec &div(int n) { x /= n; y /= n; z /= n; return *this; }
    ivec &add(int n) { x += n; y += n; z += n; return *this; }
    ivec &sub(int n) { x -= n; y -= n; z -= n; return *this; }
    ivec &add(const ivec &v) { x += v.x; y += v.y; z += v.z; return *this; }
    ivec &sub(const ivec &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    ivec &mask(int n) { x &= n; y &= n; z &= n; return *this; }
    ivec &cross(const ivec &a, const ivec &b) { x = a.y*b.z-a.z*b.y; y = a.z*b.x-a.x*b.z; z = a.x*b.y-a.y*b.x; return *this; }
    int dot(const ivec &o) const { return x*o.x + y*o.y + z*o.z; }
};

static inline bool htcmp(const ivec &x, const ivec &y)
{
    return x == y;
}

static inline uint hthash(const ivec &k)
{
    return k.x^k.y^k.z;
}

struct svec
{
    union
    {
        struct { short x, y, z; };
        short v[3];
    };

    svec() {}
    svec(short x, short y, short z) : x(x), y(y), z(z) {}

    short &operator[](int i)       { return v[i]; }
    short  operator[](int i) const { return v[i]; }

    bool operator==(const svec &v) const { return x==v.x && y==v.y && z==v.z; }
    bool operator!=(const svec &v) const { return x!=v.x || y!=v.y || z!=v.z; }

    void add(const svec &o) { x += o.x; y += o.y; z += o.z; }
    void sub(const svec &o) { x -= o.x; y -= o.y; z -= o.z; }
    void mul(int f) { x *= f; y *= f; z *= f; }
    void div(int f) { x /= f; y /= f; z /= f; }

    bool iszero() const { return x==0 && y==0 && z==0; }
};

struct bvec
{
    union
    {
        struct { uchar x, y, z; };
        uchar v[3];
    };

    bvec() {}
    bvec(uchar x, uchar y, uchar z) : x(x), y(y), z(z) {}
    bvec(const vec &v) : x((uchar)((v.x+1)*255/2)), y((uchar)((v.y+1)*255/2)), z((uchar)((v.z+1)*255/2)) {}

    uchar &operator[](int i)       { return v[i]; }
    uchar  operator[](int i) const { return v[i]; }

    bool operator==(const bvec &v) const { return x==v.x && y==v.y && z==v.z; }
    bool operator!=(const bvec &v) const { return x!=v.x || y!=v.y || z!=v.z; }

    bool iszero() const { return x==0 && y==0 && z==0; }

    vec tovec() const { return vec(x*(2.0f/255.0f)-1.0f, y*(2.0f/255.0f)-1.0f, z*(2.0f/255.0f)-1.0f); }
};

extern bool raysphereintersect(vec c, float radius, const vec &o, const vec &ray, float &dist);
extern bool rayrectintersect(const vec &b, const vec &s, const vec &o, const vec &ray, float &dist, int &orient);

enum { INTERSECT_NONE, INTERSECT_OVERLAP, INTERSECT_BEFORESTART, INTERSECT_MIDDLE, INTERSECT_AFTEREND };
extern int intersect_plane_line(vec &linestart, vec &linestop, vec &planeorig, vec &planenormal, vec &intersectionpoint);

