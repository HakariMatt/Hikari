#ifndef _V3_C
#define _V3_C

#include "types.h"
#include <math.h>

static inline v3 v3_add(v3 a, v3 b) { return (v3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline v3 v3_sub(v3 a, v3 b) { return (v3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline v3 v3_scale(v3 a, f64 s) { return (v3){a.x*s, a.y*s, a.z*s}; }
static inline f64 v3_dot(v3 a, v3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline v3 v3_cross(v3 a, v3 b) { return (v3){ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x }; }
static inline f64 v3_len(v3 a) { return sqrt(v3_dot(a,a)); }
static inline v3 v3_norm(v3 a) { return v3_scale(a, 1.0/v3_len(a)); }
static inline v3 ray_at(ray r, f64 t) { return v3_add(r.origin, v3_scale(r.dir, t)); }

#endif
