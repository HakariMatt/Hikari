#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../include/settings.h"
#include "../include/types.h"
#include "../include/v3.h"
#include "../include/intersect.h"
#include "../include/camera.h"
#include "../include/bvh.h"
#include "../include/colour.h"
#include "../include/render.h"
#include "../include/log.h"
#include "../include/mat.h"

static colour sky_colour(ray r) {
	v3 unit_dir = v3_norm(r.dir);
    f64 a = 0.5 * (unit_dir.z + 1.0);
    v3 white = {1,1,1}, blue = {.3,.5,1};
    v3 t = v3_add(v3_scale(blue, 1.0 - a), v3_scale(white, a));
    return (colour){t.x, t.y, t.z};
    // return (colour){0,0,0};
}

static colour ray_color(ray r, scene* sc, sz depth, u32* rng_state) {

	if (depth >= MAX_BOUNCES) {
		return (colour){0,0,0};
	}

	hit_result hr;
	hit_result best_h = {.hit = 0};
	sz best_obj = -1;
	f64 closest = INFINITY;

	for (sz i = 0; i < sc->obj_count; ++i) {
		if (!hit_bbox(r, sc->objects[i].bbox)) continue;

		hr = bvh_hit(sc->objects[i].bvh, sc->objects[i].mesh, r, closest);

		if (hr.hit && hr.t < closest) {
			closest = hr.t;
			best_h = hr;
			best_obj = i;
		}
	}

	if (!best_h.hit) return sky_colour(r);


	// f64 roughness = 0.2;

	shading_ctx ctx = {
		.point = ray_at(r, best_h.t),
		.normal = best_h.normal,
		.true_normal = best_h.true_normal,
		.r = r,
		.rng_state = rng_state
	};

	mat m = sc->mat_lib->materials[best_h.mat_id];
	bsdf_result bsdf = eval_bsdf(sc->mat_lib, m.root_socket, &ctx);
	if (!bsdf.scattered) return v3_to_colour(bsdf.emission);
	// if (!bsdf.scattered) return (colour){1,0,0};

	// v3 bounce_dir = v3_norm(v3_add(best_h.normal, v3_random(rng_state)));
	// v3 bounce_dir = v3_norm( v3_add( v3_add( best_h.normal, random_dir() ), v3_scale( v3_reflect( r.dir, best_h.normal ), 1-roughness )));

	ray new_r = {v3_add(ctx.point, v3_scale(best_h.normal, 1e-7)), bsdf.dir};

    colour incoming = ray_color(new_r, sc, depth+1, rng_state);

    // TODO: implement random early exit somehow
    f64 p = fmax(incoming.r, fmax(incoming.g, incoming.b));
    if (random_f64(rng_state) >= p) return incoming;

    return colour_multiply(incoming, v3_to_colour(bsdf.attenuation));
    // return incoming;
}

void render_progressive(render_args* args) {
	#define STOPWATCH(x) clock_gettime(CLOCK_MONOTONIC, &(x))
	struct timespec t0, t1;

	u32 rng_state;

	sz width = args->width;
	sz height = args->height;
	f32* img = args->img;
	camera cam = args->cam;
	scene* sc = &args->scene;

	print(INFO, "Width:   %zu", width);
	print(INFO, "Height:  %zu", height);
	print(INFO, "Samples: %zu", N_SAMPLES);
	print(INFO, "Bounces: %zu", MAX_BOUNCES);

	STOPWATCH(t0);

	for (sz s = 0; s < N_SAMPLES; ++s) {
		if (args->state->should_stop) break;
		#pragma omp parallel for schedule(dynamic)
		for (sz y = 0; y < height; ++y) {

			for (sz x = 0; x < width; ++x) {
				sz idx = (y * width + x) * 3;
				colour old_px = {
					img[idx + 0],
					img[idx + 1],
					img[idx + 2]
				};

				v3 jitter = v3_scale(random_point_in_circle(&rng_state), 1e-3);

				img[idx + 0] = 0.0;
				img[idx + 1] = 1.0;
				img[idx + 2] = 0.0;

				f64 u = (f64)x / (width - 1);
				f64 v = 1.0 - (f64)y / (height - 1);

				u += jitter.x;
				v += jitter.y;

				colour total_light = {0};

				ray r = camera_get_ray(cam, u, v);
				colour sample = colour_add(total_light, ray_color(r, sc, 1, &rng_state));
				total_light = sample;

				colour_gamma(&total_light, 2.4);
				colour_clip(&total_light, 1.0);

				colour pixel = colour_add(old_px, colour_divide(colour_sub(total_light, old_px), (colour){s+1,s+1,s+1}));
				img[idx + 0] = pixel.r;
				img[idx + 1] = pixel.g;
				img[idx + 2] = pixel.b;
			}
		}
		args->state->samples_done = s+1;
	}

	STOPWATCH(t1);

	print(INFO, "Rendered in %.1f s.", (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9);
}
