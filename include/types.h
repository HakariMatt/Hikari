#ifndef _TYPES_H
#define _TYPES_H

#include <stdint.h>
#include <stddef.h>

typedef uint8_t   u8;
typedef int8_t    i8;
typedef uint16_t u16;
typedef int16_t  i16;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint64_t u64;
typedef int64_t  i64;
typedef float    f32;
typedef double   f64;
typedef size_t    sz;

typedef struct { f64 x, y, z; } v3;
typedef struct { v3 origin, dir; } ray;

typedef struct { u32 a, b, c; } tri;

typedef struct {
	v3 origin, lower_left, horizontal, vertical;
	v3 u, v, w;
	f64 lens_radius;
} camera;

typedef struct {
	v3*  verts;
	sz   nverts;
	tri* tris;
	sz   ntris;
} mesh;

typedef struct {
	f64 max_x, min_x, max_y, min_y, max_z, min_z;
} boundbox;

typedef struct bvh_node {
	boundbox bbox;
	sz* tri_idxs;
	sz  tri_count;
	struct bvh_node* childA;
	struct bvh_node* childB;
} bvh_node;

typedef struct {
	mesh mesh;
	boundbox bbox;
	bvh_node* bvh;
} object;

typedef struct { f64 t; v3 normal; int hit; } hit_result;

#endif
