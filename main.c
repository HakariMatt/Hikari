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

static inline v3 v3_add(v3 a, v3 b) { return (v3){a.x+b.x, a.y+b.y, a.z+b.z}; }
static inline v3 v3_sub(v3 a, v3 b) { return (v3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline v3 v3_scale(v3 a, f64 s) { return (v3){a.x*s, a.y*s, a.z*s}; }
static inline f64 v3_dot(v3 a, v3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline v3 v3_cross(v3 a, v3 b) { return (v3){ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x }; }
static inline f64 v3_len(v3 a) { return sqrt(v3_dot(a,a)); }
static inline v3 v3_norm(v3 a) { return v3_scale(a, 1.0/v3_len(a)); }
static inline v3 ray_at(ray r, f64 t) { return v3_add(r.origin, v3_scale(r.dir, t)); }

#define WIDTH 640
#define HEIGHT 480
#define IMG_SIZE WIDTH*HEIGHT*3

int main(void) {
	u8* img = malloc(IMG_SIZE);
	for (usz y = 0; y < HEIGHT; ++y) {
		for (usz x = 0; x < WIDTH; ++x) {
			usz idx = (y * WIDTH + x) * 3;
			img[idx + 0] = (u8)((float)x / WIDTH  * 255);  // red   = horizontal gradient
        	img[idx + 1] = (u8)((float)y / HEIGHT * 255);  // green = vertical gradient
         	img[idx + 2] = 255;
		}
	}
	FILE* f = fopen("output.ppm", "wb");
	fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
	fwrite(img, 1, IMG_SIZE, f);
	fclose(f);
	return 0;
}
