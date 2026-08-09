#ifndef _RENDER_C
#define _RENDER_C

#include <stdio.h>

#include "types.h"
#include "v3.c"
#include "intersect.c"
#include "camera.c"

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

	for (sz i = 0; i < obj.mesh.ntris; ++i) {
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

void render(u8* img, int width, int height, camera cam, object obj) {
	sz done = 0;
	#pragma omp parallel for schedule(dynamic)
	for (sz y = 0; y < height; ++y) {
		for (sz x = 0; x < width; ++x) {
			f64 u = (f64)x / (width - 1);
			f64 v = 1.0 - (f64)y / (height - 1);

			ray r = camera_get_ray(cam, u, v);
			v3 col = ray_color(r, obj);

			sz idx = (y * width + x) * 3;
			img[idx + 0] = (u8)(255.999 * col.x);
			img[idx + 1] = (u8)(255.999 * col.y);
			img[idx + 2] = (u8)(255.999 * col.z);
		}

		// #pragma omp atomic
	 	// done++;

	    // if (done % 16 == 0) {          // print every 16 rows
	    //     #pragma omp critical
	    //     {
	    //         f64 pct = 100.0 * (f64)done / height;
	    //         fprintf(stderr, "\rrendering... %5.1f%%", pct);
	    //         fflush(stderr);
	    //     }
	    // }
	}
}

#endif
