#include "../include/intersect.h"
#include "../include/v3.h"

static v3 interpolate_normal(mesh* m, tri f, f64 t, ray r) {
	v3 p = ray_at(r, t);

	v3 n0 = m->v_norms[(int)f.v_norms_idx.x];
	v3 n1 = m->v_norms[(int)f.v_norms_idx.y];
	v3 n2 = m->v_norms[(int)f.v_norms_idx.z];

	v3 vx0 = m->verts[(int)f.verts_idx.x];
	v3 vx1 = m->verts[(int)f.verts_idx.y];
	v3 vx2 = m->verts[(int)f.verts_idx.z];

	v3 v0 = v3_sub(vx1, vx0);
	v3 v1 = v3_sub(vx2, vx0);
	v3 v2 = v3_sub(p, vx0);

	f64 d00 = v3_dot(v0, v0);
	f64 d01 = v3_dot(v0, v1);
	f64 d11 = v3_dot(v1, v1);
	f64 d20 = v3_dot(v2, v0);
	f64 d21 = v3_dot(v2, v1);

	f64 denom = d00*d11 - d01*d01;
	f64 beta = (d11*d20-d01*d21) / denom;
	f64 gamma = (d00*d21-d01*d20) / denom;
	f64 alpha = 1 - beta - gamma;

	v3 n = v3_add(v3_scale(n0, alpha), v3_add(v3_scale(n1, beta), v3_scale(n2, gamma)));
	return n;
}

hit_result hit_triangle(mesh* m, sz tri_id, ray r) {
	hit_result miss = { .hit = 0 };
	const f64 eps = 1e-8;

	tri tri = m->tris[tri_id];
	v3 v0 = m->verts[(sz)(tri.verts_idx.x)];
	v3 v1 = m->verts[(sz)(tri.verts_idx.y)];
	v3 v2 = m->verts[(sz)(tri.verts_idx.z)];

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

	v3 true_normal = v3_norm(v3_cross(e1, e2));;
	v3 normal;
	if (m->nv_norms == 0) normal = true_normal;
	else normal = interpolate_normal(m, tri, t, r);
	// if (det < 0) n = v3_scale(n, -1.0); // face toward the ray
	return (hit_result){
		.t = t,
		.normal = normal,
		.true_normal = true_normal,
		.hit = 1,
		.mat_id = tri.mat_id
	};
}

int hit_bbox(ray r, boundbox b) {
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
