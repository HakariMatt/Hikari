#ifndef _V3_H
#define _V3_H

#include "types.h"
#include "rng.h"
#include <math.h>

static inline v3 v3_add(v3 a, v3 b) { return (v3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline v3 v3_sub(v3 a, v3 b) { return (v3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline v3 v3_scale(v3 a, f64 s) { return (v3){a.x*s, a.y*s, a.z*s}; }
static inline f64 v3_dot(v3 a, v3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline v3 v3_cross(v3 a, v3 b) { return (v3){ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x }; }
static inline f64 v3_len(v3 a) { return sqrt(v3_dot(a,a)); }
static inline v3 v3_norm(v3 a) { return v3_scale(a, 1.0/v3_len(a)); }
static inline v3 v3_reflect(v3 v, v3 n) { return v3_sub(v, v3_scale(n, (2 * v3_dot(v, n)))); }
static inline v3 ray_at(ray r, f64 t) { return v3_add(r.origin, v3_scale(r.dir, t)); }
static inline v3 v3_random(u32* state) {
	return (v3){
		.x = (random_f64(state) * 2) - 1,
		.y = (random_f64(state) * 2) - 1,
		.z = (random_f64(state) * 2) - 1,
	};
}
static inline v3 random_point_in_circle(u32* rngState)
{
	f64 angle = random_f64(rngState) * 2 * M_PI;
	v3 point_on_circle = {cos(angle), sin(angle), 0};
	return v3_scale(point_on_circle, sqrt(random_f64(rngState)));
}
static inline void build_onb(v3 n, v3 *tangent, v3 *bitangent) {
    f64 sign = n.z >= 0.0 ? 1.0 : -1.0;
    f64 a = -1.0 / (sign + n.z);
    f64 b = n.x * n.y * a;
    *tangent   = (v3){1.0 + sign * n.x * n.x * a, sign * b, -sign * n.x};
    *bitangent = (v3){b, sign + n.y * n.y * a, -n.y};
}

#endif
