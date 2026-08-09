#ifndef _RENDER_C
#define _RENDER_C

#include <stdio.h>

#include "settings.h"
#include "types.h"
#include "v3.c"
#include "intersect.c"
#include "camera.c"
#include "bvh.c"
#include "colour.c"

static colour sky_colour(ray r) {
	v3 unit_dir = v3_norm(r.dir);
    f64 a = 0.5 * (unit_dir.y + 1.0);
    v3 white = {1.0, 1.0, 1.0}, blue = {0.3, 0.5, 1.0};
    v3 t = v3_add(v3_scale(white, 1.0 - a), v3_scale(blue, a));
    return (colour){t.x, t.y, t.z};
}

static colour ray_color(ray r, object obj, sz depth) {
	if (depth >= MAX_BOUNCES) {
		return (colour){0,0,0};
	}

	if (!hit_bbox(r, obj.bbox)) {
        return sky_colour(r);
    }

	hit_result hr = bvh_hit(obj.bvh, obj.mesh, r, INFINITY);
	if (!hr.hit) {
    	return sky_colour(r);
	}

	v3 bounce_dir = v3_reflect(r.dir, hr.normal);
	ray new_r = {v3_add(ray_at(r, hr.t), v3_scale(hr.normal, 0.0000001)), bounce_dir};

    colour incoming = ray_color(new_r, obj, depth+1);
    return colour_multiply(incoming, (colour){0.8, 0.8, 0.8});

}

void render(u8* img, int width, int height, camera cam, object obj) {
	sz done = 0;
	#pragma omp parallel for schedule(dynamic)
	for (sz y = 0; y < height; ++y) {
		for (sz x = 0; x < width; ++x) {
			f64 u = (f64)x / (width - 1);
			f64 v = 1.0 - (f64)y / (height - 1);

			ray r = camera_get_ray(cam, u, v);
			colour col = ray_color(r, obj, 1);

			sz idx = (y * width + x) * 3;
			img[idx + 0] = (u8)(255.999 * col.r);
			img[idx + 1] = (u8)(255.999 * col.g);
			img[idx + 2] = (u8)(255.999 * col.b);
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
