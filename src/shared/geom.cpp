
#include "pch.h"
#include "tools.h"
#include "geom.h"

bool raysphereintersect(vec c, float radius, const vec &o, const vec &ray, float &dist)
{
    c.sub(o);
    float v = c.dot(ray),
          inside = radius*radius - c.squaredlen(),
          d = inside + v*v;
    if(inside<0 && d<0) return false;
    dist += v - sqrt(d);
    return true;
}

bool rayrectintersect(const vec &b, const vec &s, const vec &o, const vec &ray, float &dist, int &orient)
{
    loopi(6)
    {
        int d = i>>1, dc = i&1; orient = i;
        if(dc == (ray[d]>0)) continue;
        float t = 0;
        plane pl(d, b[D[d]]+s[D[d]]*dc);
        if(pl.rayintersect(o, ray, t))
        {
            vec v(ray);
            v.mul(t);
            v.add(o);
            if(v[R[d]] >= b[R[d]] && v[R[d]] <= b[R[d]]+s[R[d]]
            && v[C[d]] >= b[C[d]] && v[C[d]] <= b[C[d]]+s[C[d]])
            {
                dist += t;
                return true;
            }
        }
    }
    return false;
}

int intersect_plane_line(vec &linestart, vec &linestop, vec &planeorig, vec &planenormal, vec &intersectionpoint)
{
    vec u = linestop;  u.sub(linestart);
    vec w = linestart; w.sub(planeorig);
    float d = planenormal.dot(u);
    float n = -planenormal.dot(w);
    if(fabs(d)<0.000001) return n==0 ? INTERSECT_OVERLAP : INTERSECT_NONE;                 
    float si = n/d;
    intersectionpoint = u.mul(si).add(linestart);              
    return si<0 ? INTERSECT_BEFORESTART : (si>1 ? INTERSECT_AFTEREND : INTERSECT_MIDDLE);
}
