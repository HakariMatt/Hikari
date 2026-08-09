#ifndef _INTERSECT_C
#define _INTERSECT_C

#include "types.h"
#include "v3.c"

static hit_result hit_triangle(v3 v0, v3 v1, v3 v2, ray r) {
	hit_result miss = { .hit = 0 };
	const f64 eps = 1e-8;

	v3 e1 = v3_sub(v1, v0);
	v3 e2 = v3_sub(v2, v0);
	v3 pvec = v3_cross(r.dir, e2);
	f64 det = v3_dot(e1, pvec);

	if (fabs(det) < eps) return miss; // parallel to triangle
	f64 inv_det = 1.0 / det;

	v3 tvec = v3_sub(r.origin, v0);
	f64 u = v3_dot(tvec, pvec) * inv_det;
	if (u < 0.0 || u > 1.0) return miss;

	v3 qvec = v3_cross(tvec, e1);
	f64 v = v3_dot(r.dir, qvec) * inv_det;
	if (v < 0.0 || u + v > 1.0) return miss;

	f64 t = v3_dot(e2, qvec) * inv_det;
	if (t < eps) return miss; // behind origin

	v3 n = v3_norm(v3_cross(e1, e2));
	if (det < 0) n = v3_scale(n, -1.0); // face toward the ray
	return (hit_result){ t, n, 1 };
}

// Returns true if the ray hits (or starts inside) the AABB
static int hit_bbox(ray r, boundbox b) {
    f64 tmin = 0.0;
    f64 tmax = INFINITY;

    // X slab
    f64 invD = 1.0 / r.dir.x;
    f64 t0 = (b.min_x - r.origin.x) * invD;
    f64 t1 = (b.max_x - r.origin.x) * invD;
    if (invD < 0.0) { f64 tmp = t0; t0 = t1; t1 = tmp; }
    tmin = fmax(tmin, t0);
    tmax = fmin(tmax, t1);
    if (tmax < tmin) return 0;

    // Y slab
    invD = 1.0 / r.dir.y;
    t0 = (b.min_y - r.origin.y) * invD;
    t1 = (b.max_y - r.origin.y) * invD;
    if (invD < 0.0) { f64 tmp = t0; t0 = t1; t1 = tmp; }
    tmin = fmax(tmin, t0);
    tmax = fmin(tmax, t1);
    if (tmax < tmin) return 0;

    // Z slab
    invD = 1.0 / r.dir.z;
    t0 = (b.min_z - r.origin.z) * invD;
    t1 = (b.max_z - r.origin.z) * invD;
    if (invD < 0.0) { f64 tmp = t0; t0 = t1; t1 = tmp; }
    tmin = fmax(tmin, t0);
    tmax = fmin(tmax, t1);

    return tmax >= tmin;
}

#endif
