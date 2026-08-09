#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
typedef size_t   usz;
typedef ssize_t  ssz;

typedef struct { f64 x, y, z; } v3;
typedef struct { v3 origin, dir; } ray;

typedef struct {
	v3 origin, lower_left, horizontal, vertical;
	v3 u, v, w;
	f64 lens_radius;
} camera;

typedef struct { u32 a, b, c; } tri;

typedef struct {
	v3*  verts;
	usz  nverts;
	tri* tris;
	usz  ntris;
} mesh;

typedef struct {
	f64 max_x, min_x, max_y, min_y, max_z, min_z;
} boundbox;

typedef struct {
	mesh mesh;
	boundbox bbox;
} object;

static inline v3 v3_add(v3 a, v3 b) { return (v3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline v3 v3_sub(v3 a, v3 b) { return (v3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline v3 v3_scale(v3 a, f64 s) { return (v3){a.x*s, a.y*s, a.z*s}; }
static inline f64 v3_dot(v3 a, v3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline v3 v3_cross(v3 a, v3 b) { return (v3){ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x }; }
static inline f64 v3_len(v3 a) { return sqrt(v3_dot(a,a)); }
static inline v3 v3_norm(v3 a) { return v3_scale(a, 1.0/v3_len(a)); }
static inline v3 ray_at(ray r, f64 t) { return v3_add(r.origin, v3_scale(r.dir, t)); }

camera camera_make(v3 lookfrom, v3 lookat, v3 vup,
	f64 vfov_deg, f64 aspect, f64 aperture, f64 focus_dist) {
	f64 theta = vfov_deg * M_PI / 180.0;
	f64 h = tan(theta/2);
	f64 viewport_h = 2.0 * h;
	f64 viewport_w = aspect * viewport_h;

	v3 w = v3_norm(v3_sub(lookfrom, lookat));
	v3 u = v3_norm(v3_cross(vup, w));
	v3 v = v3_cross(w, u);

	camera c;
	c.origin = lookfrom;
	c.horizontal = v3_scale(u, viewport_w * focus_dist);
	c.vertical = v3_scale(v, viewport_h * focus_dist);
	c.lower_left = v3_sub(v3_sub(v3_sub(c.origin, v3_scale(c.horizontal, 0.5)), v3_scale(c.vertical, 0.5)), v3_scale(w, focus_dist));

	c.u = u; c.v = v; c.w = w;
	c.lens_radius = aperture / 2;
	return c;
}

static inline ray camera_get_ray(camera c, f64 s, f64 t) {
	v3 dir = v3_sub(
		v3_add(v3_add(c.lower_left, v3_scale(c.horizontal, s)), v3_scale(c.vertical, t)),
		c.origin
	);
	return (ray){c.origin, dir};
}

static object mesh_load_obj(const char* path) {
	FILE* f = fopen(path, "r");
	if (!f) { fprintf(stderr, "could not open %s\n", path); exit(1); }

	usz vcap = 1024, vcount = 0;
	v3* verts = malloc(vcap * sizeof(v3));

	usz tcap = 1024, tcount = 0;
	tri* tris = malloc(tcap * sizeof(tri));

	char line[512];
	while (fgets(line, sizeof(line), f)) {
		if (line[0] == 'v' && line[1] == ' ') {
			v3 p;
			if (sscanf(line + 2, "%lf %lf %lf", &p.x, &p.y, &p.z) == 3) {
				if (vcount == vcap) { vcap *= 2; verts = realloc(verts, vcap * sizeof(v3)); }
				verts[vcount++] = p;
			}
		} else if (line[0] == 'f' && line[1] == ' ') {
			// parse all vertex indices on the line (first int of each token),
			// then fan-triangulate: (0,1,2), (0,2,3), (0,3,4), ...
			u32 idx[32];
			usz n = 0;
			char* p = line + 1;
			while (*p && n < 32) {
				while (*p == ' ') p++;
				if (*p == '\0' || *p == '\n') break;
				int v;
				if (sscanf(p, "%d", &v) != 1) break;
				idx[n++] = (u32)(v - 1); // OBJ is 1-indexed
				while (*p && *p != ' ' && *p != '\n') p++; // skip to next token
			}
			for (usz i = 1; i + 1 < n; ++i) {
				if (tcount == tcap) { tcap *= 2; tris = realloc(tris, tcap * sizeof(tri)); }
				tris[tcount++] = (tri){ idx[0], idx[i], idx[i+1] };
			}
		}
	}
	fclose(f);
	f64 max_x = -INFINITY, max_y = -INFINITY, max_z = -INFINITY;
	f64 min_x =  INFINITY, min_y =  INFINITY, min_z =  INFINITY;

	for (usz i = 0; i < vcount; ++i) {
	    v3 vert = verts[i];
	    max_x = fmax(max_x, vert.x);
	    max_y = fmax(max_y, vert.y);
	    max_z = fmax(max_z, vert.z);
	    min_x = fmin(min_x, vert.x);
	    min_y = fmin(min_y, vert.y);
	    min_z = fmin(min_z, vert.z);
	}
	return (object){
		(mesh){ verts, vcount, tris, tcount },
		(boundbox){ max_x, min_x, max_y, min_y, max_z, min_z }
	};
	// return (mesh){ verts, vcount, tris, tcount };
}

static void mesh_free(mesh m) {
	free(m.verts);
	free(m.tris);
}

typedef struct { f64 t; v3 normal; int hit; } hit_result;

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

static v3 ray_color(ray r, object obj) {
	if (!hit_bbox(r, obj.bbox)) {
        // sky gradient
        // v3 unit_dir = v3_norm(r.dir);
        // f64 a = 0.5 * (unit_dir.y + 1.0);
        // v3 white = {1.0, 1.0, 1.0}, blue = {0.5, 0.7, 1.0};
        // return v3_add(v3_scale(white, 1.0 - a), v3_scale(blue, a));
        return (v3){0,0,0};
    }
	f64 closest = INFINITY;
	v3 hit_normal = {0};
	int any_hit = 0;

	for (usz i = 0; i < obj.mesh.ntris; ++i) {
		tri t = obj.mesh.tris[i];
		hit_result hr = hit_triangle(obj.mesh.verts[t.a], obj.mesh.verts[t.b], obj.mesh.verts[t.c], r);
		if (hr.hit && hr.t < closest) {
			closest = hr.t;
			hit_normal = hr.normal;
			any_hit = 1;
		}
	}

	if (any_hit) {
		return v3_scale((v3){hit_normal.x+1, hit_normal.y+1, hit_normal.z+1}, 0.5);
	}

	// v3 unit_dir = v3_norm(r.dir);
	// f64 a = 0.5 * (unit_dir.y + 1.0);
	// v3 white = {1.0, 1.0, 1.0}, blue = {0.5, 0.7, 1.0};
	// return v3_add(v3_scale(white, 1.0 - a), v3_scale(blue, a));
	return (v3){0,0,0};
}

// #define LOWRES

#ifdef LOWRES
	#define WIDTH  320
	#define HEIGHT 240
#else
	#define WIDTH  (640)
	#define HEIGHT (480)
#endif
#define IMG_SIZE WIDTH*HEIGHT*3

int main(void) {

	camera cam = camera_make(
		(v3){-1.5,1.5,1.5}, (v3){0,.56,0}, (v3){0,1,0},
		30.0, (f64)WIDTH/HEIGHT, 0.0, 1.0
	);
	object obj = mesh_load_obj("Matomi.obj");
	u8* img = malloc(IMG_SIZE);

	usz done = 0;
	#pragma omp parallel for schedule(dynamic)
	for (usz y = 0; y < HEIGHT; ++y) {
		for (usz x = 0; x < WIDTH; ++x) {
			f64 u = (f64)x / (WIDTH - 1);
			f64 v = 1.0 - (f64)y / (HEIGHT - 1);

			ray r = camera_get_ray(cam, u, v);
			v3 col = ray_color(r, obj);

			usz idx = (y * WIDTH + x) * 3;
			img[idx + 0] = (u8)(255.999 * col.x);
			img[idx + 1] = (u8)(255.999 * col.y);
			img[idx + 2] = (u8)(255.999 * col.z);
		}

		#pragma omp atomic
	    done++;

	    if (done % 16 == 0) {          // print every 16 rows
	        #pragma omp critical
	        {
	            f64 pct = 100.0 * (f64)done / HEIGHT;
	            fprintf(stderr, "\rrendering... %5.1f%%", pct);
	            fflush(stderr);
	        }
	    }
	}

	FILE* f = fopen("output.ppm", "wb");
	fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
	fwrite(img, 1, IMG_SIZE, f);
	fclose(f);
	free(img);
	fprintf(stderr, "\nDone\n");
	return 0;
}
