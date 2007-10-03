#include "pch.h"
#include "engine.h"

bool BIH::triintersect(tri &tri, const vec &o, const vec &ray, float maxdist, float &dist, int mode)
{
    vec p;
    p.cross(ray, tri.c);
    float det = tri.b.dot(p);
    if(det == 0) return false;
    vec r(o); 
    r.sub(tri.a);
    float u = r.dot(p) / det; 
    if(u < 0 || u > 1) return false;
    vec q; 
    q.cross(r, tri.b);
    float v = ray.dot(q) / det;
    if(v < 0 || u + v > 1) return false;
    float f = tri.c.dot(q) / det;
    if(f < 0 || f > maxdist) return false;
    if(tri.tex && (mode&RAY_ALPHAPOLY)==RAY_ALPHAPOLY)
    {
        if(!tri.tex->alphamask)
        {
            loadalphamask(tri.tex); 
            if(!tri.tex->alphamask) { dist = f; return true; }
        }
        float s = tri.tc[0] + u*(tri.tc[2] - tri.tc[0]) + v*(tri.tc[4] - tri.tc[0]),
              t = tri.tc[1] + u*(tri.tc[3] - tri.tc[1]) + v*(tri.tc[5] - tri.tc[1]);
        int si = int(s*tri.tex->xs), ti = int(t*tri.tex->ys);
        if(!(tri.tex->alphamask[ti*((tri.tex->xs+7)/8) + si/8] & (1<<(si%8)))) return false;
    }
    dist = f;
    return true;
}

struct BIHStack
{
    BIHNode *node;
    float tmin, tmax;
};

bool BIH::traverse(const vec &o, const vec &ray, float maxdist, float &dist, int mode)
{
    if(!numnodes) return false;

    vec invray(ray.x ? 1/ray.x : 1e16f, ray.y ? 1/ray.y : 1e16f, ray.z ? 1/ray.z : 1e16f);
    float tmin, tmax;
    float t1 = (bbmin.x - o.x)*invray.x, 
          t2 = (bbmax.x - o.x)*invray.x;
    if(invray.x > 0) { tmin = t1; tmax = t2; } else { tmin = t2; tmax = t1; }
    t1 = (bbmin.y - o.y)*invray.y;
    t2 = (bbmax.y - o.y)*invray.y;
    if(invray.y > 0) { tmin = max(tmin, t1); tmax = min(tmax, t2); } else { tmin = max(tmin, t2); tmax = min(tmax, t1); }
    t1 = (bbmin.z - o.z)*invray.z;
    t2 = (bbmax.z - o.z)*invray.z;
    if(invray.z > 0) { tmin = max(tmin, t1); tmax = min(tmax, t2); } else { tmin = max(tmin, t2); tmax = min(tmax, t1); }
    if(tmin >= maxdist || tmin>=tmax) return false;
    tmax = min(tmax, maxdist);

    static vector<BIHStack> stack;
    stack.setsizenodelete(0);

    ivec order(ray.x>0 ? 0 : 1, ray.y>0 ? 0 : 1, ray.z>0 ? 0 : 1);
    BIHNode *curnode = &nodes[0];
    for(;;)
    {
        int axis = curnode->axis();
        int nearidx = order[axis], faridx = nearidx^1;
        float nearsplit = (curnode->split[nearidx] - o[axis])*invray[axis],
              farsplit = (curnode->split[faridx] - o[axis])*invray[axis];

        if(nearsplit <= tmin)
        {
            if(farsplit < tmax)
            {
                if(!curnode->isleaf(faridx))
                {
                    curnode = &nodes[curnode->childindex(faridx)];
                    tmin = max(tmin, farsplit);
                    continue;
                }
                else if(triintersect(tris[curnode->childindex(faridx)], o, ray, maxdist, dist, mode)) return true;
            }
        }
        else if(curnode->isleaf(nearidx))
        {
            if(triintersect(tris[curnode->childindex(nearidx)], o, ray, maxdist, dist, mode)) return true;
            if(farsplit < tmax)
            {
                if(!curnode->isleaf(faridx))
                {
                    curnode = &nodes[curnode->childindex(faridx)];
                    tmin = max(tmin, farsplit);
                    continue;
                }
                else if(triintersect(tris[curnode->childindex(faridx)], o, ray, maxdist, dist, mode)) return true;
            }
        }
        else
        {
            if(farsplit < tmax)
            {
                if(!curnode->isleaf(faridx))
                {
                    BIHStack &save = stack.add();
                    save.node = &nodes[curnode->childindex(faridx)];
                    save.tmin = max(tmin, farsplit);
                    save.tmax = tmax;
                }
                else if(triintersect(tris[curnode->childindex(faridx)], o, ray, maxdist, dist, mode)) return true;
            }
            curnode = &nodes[curnode->childindex(nearidx)];
            tmax = min(tmax, nearsplit);
            continue;
        }
        if(stack.empty()) return false;
        BIHStack &restore = stack.pop();
        curnode = restore.node;
        tmin = restore.tmin;
        tmax = restore.tmax;
    }
}

static BIH::tri *sorttris = NULL;
static int sortaxis = 0;

static int bihsort(const ushort *x, const ushort *y)
{
    BIH::tri &xtri = sorttris[*x], &ytri = sorttris[*y];
    float xmin = min(xtri.a[sortaxis], min(xtri.b[sortaxis], xtri.c[sortaxis])),
          ymin = min(ytri.a[sortaxis], min(ytri.b[sortaxis], ytri.c[sortaxis]));
    if(xmin < ymin) return -1;
    if(xmin > ymin) return 1;
    return 0;
}

void BIH::build(vector<BIHNode> &buildnodes, ushort *indices, int numindices, int depth)
{
    maxdepth = max(maxdepth, depth);
   
    vec vmin(1e16f, 1e16f, 1e16f), vmax(-1e16f, -1e16f, -1e16f);
    loopi(numindices)
    {
        tri &tri = tris[indices[i]];
        loopk(3)
        {
            float amin = min(tri.a[k], min(tri.b[k], tri.c[k])),
                  amax = max(tri.a[k], max(tri.b[k], tri.c[k]));
            vmin[k] = min(vmin[k], amin);
            vmax[k] = max(vmax[k], amax);
        }
    }
    if(depth==1)
    {
        bbmin = vmin;
        bbmax = vmax;
    }

    int axis = 2;
    loopk(2) if(vmax[k] - vmin[k] > vmax[axis] - vmin[axis]) axis = k;

/*
    sorttris = tris;
    sortaxis = axis;
    qsort(indices, numindices, sizeof(ushort), (int (__cdecl *)(const void *, const void *))bihsort);
    tri &median = tris[numindices/2];
    float split = min(median.a[axis], min(median.b[axis], median.c[axis]));
*/

    float split = 0.5f*(vmax[axis] + vmin[axis]);

    float splitleft = SHRT_MIN, splitright = SHRT_MAX;
    int left, right;
    for(left = 0, right = numindices; left < right;)
    {
        tri &tri = tris[indices[left]];
        float amin = min(tri.a[axis], min(tri.b[axis], tri.c[axis])),
              amax = max(tri.a[axis], max(tri.b[axis], tri.c[axis]));
        if(max(split - amin, 0) > max(amax - split, 0)) 
        {
            ++left;
            splitleft = max(splitleft, amax);
        }
        else 
        { 
            --right; 
            swap(ushort, indices[left], indices[right]); 
            splitright = min(splitright, amin);
        }
    }

    if(!left || right==numindices)
    {
        sorttris = tris;
        sortaxis = axis;
        qsort(indices, numindices, sizeof(ushort), (int (__cdecl *)(const void *, const void *))bihsort);

        left = right = numindices/2;
        splitleft = SHRT_MIN;
        splitright = SHRT_MAX;
        loopi(numindices)
        {
            tri &tri = tris[indices[i]];
            if(i < left) splitleft = max(splitleft, max(tri.a[axis], max(tri.b[axis], tri.c[axis])));
            else splitright = min(splitright, min(tri.a[axis], min(tri.b[axis], tri.c[axis])));
        }
    }

    int node = buildnodes.length();
    buildnodes.add();
    buildnodes[node].split[0] = short(ceil(splitleft));
    buildnodes[node].split[1] = short(floor(splitright));

    if(left==1) buildnodes[node].child[0] = (axis<<14) | indices[0];
    else
    {
        buildnodes[node].child[0] = (axis<<14) | buildnodes.length();
        build(buildnodes, indices, left, depth+1);
    }

    if(numindices-right==1) buildnodes[node].child[1] = (1<<15) | (left==1 ? 1<<14 : 0) | indices[right];
    else 
    {
        buildnodes[node].child[1] = (left==1 ? 1<<14 : 0) | buildnodes.length();
        build(buildnodes, &indices[right], numindices-right, depth+1);
    }
}
 
BIH::BIH(int _numtris, tri *_tris)
{
    numtris = _numtris;
    if(!numtris) 
    {
        tris = NULL;
        numnodes = 0;
        nodes = NULL;
        maxdepth = 0;
        return;
    }

    tris = new tri[numtris];
    memcpy(tris, _tris, numtris*sizeof(tri));

    vector<BIHNode> buildnodes;
    ushort *indices = new ushort[numtris];
    loopi(numtris) indices[i] = i;

    maxdepth = 0;

    build(buildnodes, indices, numtris);

    delete[] indices;

    numnodes = buildnodes.length();
    nodes = new BIHNode[numnodes];
    memcpy(nodes, buildnodes.getbuf(), numnodes*sizeof(BIHNode));

    // convert tri.b/tri.c to edges
    loopi(numtris)
    {
        tri &tri = tris[i];
        tri.b.sub(tri.a);
        tri.c.sub(tri.a);
    }
}

static inline void yawray(vec &o, vec &ray, float angle)
{
    angle *= RAD;
    float c = cosf(angle), s = sinf(angle),
          ox = o.x, oy = o.y,
          rx = ox+ray.x, ry = oy+ray.y;
    o.x = ox*c - oy*s;
    o.y = oy*c + ox*s;
    ray.x = rx*c - ry*s - o.x;
    ray.y = ry*c + rx*s - o.y;
    ray.normalize();
}

bool mmintersect(const extentity &e, const vec &o, const vec &ray, float maxdist, int mode, float &dist)
{
    model *m = loadmodel(NULL, e.attr2);
    if(!m) return false;
    if(mode&RAY_SHADOW)
    {
        if(!m->shadow || checktriggertype(e.attr3, TRIG_COLLIDE|TRIG_DISAPPEAR)) return false;
    }
    else if((mode&RAY_ENTS)!=RAY_ENTS && !m->collide) return false;
    if(!m->bih && !m->setBIH()) return false;
    if(!maxdist) maxdist = 1e16f;
    vec yo(o);
    yo.sub(e.o);
    float yaw = -180.0f-(float)((e.attr1+7)-(e.attr1+7)%15);
    vec yray(ray);
    if(yaw != 0) yawray(yo, yray, yaw);
    return m->bih->traverse(yo, yray, maxdist, dist, mode);
}

static bool planeboxoverlap(const plane &p, const vec &size)
{
    vec vmin, vmax;
    loopk(3)
    {
        if(p[k]>0)
        {
            vmin[k] = -size[k];
            vmax[k] = size[k];
        }
        else
        {
            vmin[k] = size[k];
            vmax[k] = size[k];
        }
    }
    if(p.dist(vmin) > 0) return false;
    return p.dist(vmax) >= 0;
}

#define EXTENTTEST(k) \
    { \
        float vmin = v.a.k, vmax = vmin; \
        if(v.b.k < vmin) vmin = v.b.k; \
        else if(v.b.k > vmax) vmax = v.b.k; \
        if(v.c.k < vmin) vmin = v.c.k; \
        else if(v.c.k > vmax) vmax = v.c.k; \
        if(vmin > size.k || vmax < -size.k) return false; \
    }
#define AXISTEST(e, s, t1, t2, u, v) \
    { \
        float p1 = s (e.v * t1.u - e.u * t1.v), \
              p2 = s (e.v * t2.u - e.u * t2.v), \
              pmin, pmax; \
        if(p1 < p2) { pmin = p1; pmax = p2; } else { pmin = p2; pmax = p1; } \
        float radius = fabs(e.v) * size.u + fabs(e.u) * size.v; \
        if(pmin > radius || pmax < -radius) return false; \
    }

static bool triboxoverlap(const vec &center, const vec &size, const triangle &tri, const vec &dir, float cutoff, vec &normal)
{
    triangle v(tri);
    v.b.add(tri.a);
    v.c.add(tri.a);
    v.sub(center);

    EXTENTTEST(x);
    EXTENTTEST(y);
    EXTENTTEST(z);

    triangle e;
    (e.a = v.b).sub(v.a);
    (e.b = v.c).sub(v.b);

    plane p;
    p.cross(e.a, e.b);
    p.offset = -p.dot(v.a);
    if(!planeboxoverlap(p, size)) return false;

    (e.c = v.a).sub(v.c);

    AXISTEST(e.a,  , v.a, v.c, y, z);
    AXISTEST(e.a, -, v.a, v.c, x, z);
    AXISTEST(e.a,  , v.b, v.c, x, y);

    AXISTEST(e.b,  , v.a, v.c, y, z);
    AXISTEST(e.b, -, v.a, v.c, x, z);
    AXISTEST(e.b,  , v.a, v.b, x, y);

    AXISTEST(e.c,  , v.a, v.b, y, z);
    AXISTEST(e.c, -, v.a, v.b, x, z);
    AXISTEST(e.c,  , v.a, v.b, x, y);

    p.normalize();

    if(!dir.iszero())
    {
        if(p.dot(dir)>=-cutoff) return false;
        float dist = p.dist(center) - vec(p.x*size.x, p.y*size.y, p.z*size.z).magnitude();
        if(dist < (dir.z*p.z < 0 ? -size.z/(dir.z < 0 ? 1.5f : 2.0f) : ((dir.x*p.x < 0 || dir.y*p.y < 0) ? -size.x : 0))) return false;
    }
    else p.normalize();

    normal = p;

    return true;
}

bool BIH::collide(const vec &o, const vec &radius, const vec &ray, float cutoff, vec &normal)
{
    if(!numnodes) return false;

    static vector<BIHNode *> stack;
    stack.setsizenodelete(0);

    ivec order(ray.x>0 ? 0 : 1, ray.y>0 ? 0 : 1, ray.z>0 ? 0 : 1);
    BIHNode *curnode = &nodes[0];
    for(;;)
    {
        int axis = curnode->axis();
        int nearidx = order[axis], faridx = nearidx^1;
        bool nearcollide = false, farcollide = false;
        if(!nearidx)
        {
            nearcollide = o[axis] - radius[axis] < curnode->split[0];
            farcollide = o[axis] + radius[axis] > curnode->split[1];
        }
        else
        {
            farcollide = o[axis] - radius[axis] < curnode->split[0];
            nearcollide = o[axis] + radius[axis] > curnode->split[1];
        } 
        if(!nearcollide)
        {
            if(farcollide)
            {
                if(!curnode->isleaf(faridx))
                {
                    curnode = &nodes[curnode->childindex(faridx)];
                    continue;
                }
                else if(triboxoverlap(o, radius, tris[curnode->childindex(faridx)], ray, cutoff, normal)) return true;
            }
        }
        else if(curnode->isleaf(nearidx))
        {
            if(triboxoverlap(o, radius, tris[curnode->childindex(nearidx)], ray, cutoff, normal)) return true;
            if(farcollide)
            {
                if(!curnode->isleaf(faridx))
                {
                    curnode = &nodes[curnode->childindex(faridx)];
                    continue;
                }
                else if(triboxoverlap(o, radius, tris[curnode->childindex(faridx)], ray, cutoff, normal)) return true;
            }
        }
        else
        {
            if(farcollide)
            {
                if(!curnode->isleaf(faridx)) stack.add(&nodes[curnode->childindex(faridx)]);
                else if(triboxoverlap(o, radius, tris[curnode->childindex(faridx)], ray, cutoff, normal)) return true;
            }
            curnode = &nodes[curnode->childindex(nearidx)];
            continue;
        }
        if(stack.empty()) return false;
        curnode = stack.pop();
    }
}

void rotatebb(vec &center, vec &radius, int yaw)
{
    yaw = (yaw+7)-(yaw+7)%15;
    yaw = 180-yaw;
    if(yaw<0) yaw += 360;
    switch(yaw)
    {
        case 0: break;
        case 180:
            center.x = -center.x;
            center.y = -center.y;
            break;
        case 90:
            swap(float, radius.x, radius.y);
            swap(float, center.x, center.y);
            center.x = -center.x;
            break;
        case 270:
            swap(float, radius.x, radius.y);
            swap(float, center.x, center.y);
            center.y = -center.y;
            break;
        default:
            radius.x = radius.y = max(radius.x, radius.y) + max(fabs(center.x), fabs(center.y));
            center.x = center.y = 0.0f;
            break;
    }
}

static bool bbcollide(physent *d, const vec &dir, const vec &o, const vec &radius, bool tricol, vec &normal)
{
    vec s(d->o);
    s.sub(o);
    float ax = fabs(s.x) - (radius.x+d->radius),
          ay = fabs(s.y) - (radius.y+d->radius),
          az = fabs(s.z) - (s.z>0 ? d->eyeheight+radius.z : d->aboveeye);
    if(ax>0 || ay>0 || az>0) return true;
    normal.x = normal.y = normal.z = 0;
#define TRYCOLLIDE(dim, ON, OP, N, P) \
    { \
        if(s.dim<0) { if(dir.iszero() || (dir.dim>0 && (d->type>=ENT_CAMERA || tricol || (N)))) { normal.dim = -1; return false; } } \
        else if(dir.iszero() || (dir.dim<0 && (d->type>=ENT_CAMERA || tricol || (P)))) { normal.dim = 1; return false; } \
    }
    if(ax>ay && ax>az) TRYCOLLIDE(x, O_LEFT, O_RIGHT, ax > -d->radius, ax > -d->radius);
    if(ay>az) TRYCOLLIDE(y, O_BACK, O_FRONT, ay > -d->radius, ay > -d->radius);
    TRYCOLLIDE(z, O_BOTTOM, O_TOP,
         az >= -(d->eyeheight+d->aboveeye)/4.0f,
         az >= -(d->eyeheight+d->aboveeye)/3.0f);
    return true;
}

VAR(tricol, 0, 0, 1);

bool mmcollide(physent *d, const vec &dir, float cutoff, octaentities &oc, vec &normal) // collide with a mapmodel
{
    const vector<extentity *> &ents = et->getents();
    loopv(oc.mapmodels)
    {
        extentity &e = *ents[oc.mapmodels[i]];
        if(e.attr3 && (e.triggerstate == TRIGGER_DISAPPEARED || !checktriggertype(e.attr3, TRIG_COLLIDE) || e.triggerstate == TRIGGERED)) continue;
        model *m = loadmodel(NULL, e.attr2);
        if(!m || !m->collide) continue;
        vec center, radius;
        m->collisionbox(0, center, radius);
        center.z -= radius.z;
        radius.z *= 2;
        rotatebb(center, radius, e.attr1);
        if(!bbcollide(d, dir, center.add(e.o), radius, tricol || m->tricollide, normal))
        {
            if(!tricol && !m->tricollide) return false;
            if(!m->bih && !m->setBIH()) continue;
            vec yo(d->o); 
            yo.sub(e.o);
            yo.z += 0.5f*(d->aboveeye - d->eyeheight);
            float yaw = -180.0f-(float)((e.attr1+7)-(e.attr1+7)%15);
            vec yray(dir);
            if(yaw != 0) yawray(yo, yray, yaw);
            if(m->bih->collide(yo, vec(d->radius, d->radius, 0.5f*(d->aboveeye + d->eyeheight)), yray, cutoff, normal))
            {
                normal.rotate_around_z(-yaw*RAD);
                return false;
            }
        }
    }
    return true;
}

