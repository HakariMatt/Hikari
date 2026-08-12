#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/settings.h"
#include "../include/types.h"
#include "../include/v3.h"
#include "../include/intersect.h"
#include "../include/camera.h"
#include "../include/bvh.h"
#include "../include/colour.h"
#include "../include/render.h"

u32 rng_state;

static colour sky_colour(ray r) {
	v3 unit_dir = v3_norm(r.dir);
    f64 a = 0.5 * (unit_dir.z + 1.0);
    v3 white = {1.0, 1.0, 1.0}, blue = {0.3, 0.5, 1.0};
    v3 t = v3_add(v3_scale(blue, 1.0 - a), v3_scale(white, a));
    return (colour){t.x, t.y, t.z};
 	// return (colour) {1,1,1};
}

// modified function from
// https://github.com/SebLague/Ray-Tracing/blob/Episode01/Assets/Scripts/Shaders/RayTracing.shader
static u32 next_random(u32* state) {
	((*state)) = (*state) * 747796405 + 2891336453;
	u32 result = (((*state) >> (((*state) >> 28) + 4)) ^ (*state)) * 277803737;
	result = (result >> 22) ^ result;
	return result;
}

static f64 random_value(u32* state) {
	return next_random(state) * (1.0 / 4294967296.0);  // [0, 1)
}

v3 random_dir() {
	return (v3){
		.x = (random_value(&rng_state) * 2) - 1,
		.y = (random_value(&rng_state) * 2) - 1,
		.z = (random_value(&rng_state) * 2) - 1,
	};
}

static v3 random_point_in_circle(u32* rngState)
{
	f64 angle = random_value(rngState) * 2 * M_PI;
	v3 point_on_circle = {cos(angle), sin(angle), 0};
	return v3_scale(point_on_circle, sqrt(random_value(rngState)));
}

static colour ray_color(ray r, scene* sc, sz depth) {
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

	f64 roughness = 0.2;

	v3 bounce_dir = v3_norm(v3_add(best_h.normal, random_dir()));
	// v3 bounce_dir = v3_norm( v3_add( v3_add( best_h.normal, random_dir() ), v3_scale( v3_reflect( r.dir, best_h.normal ), 1-roughness )));

	ray new_r = {v3_add(ray_at(r, best_h.t), v3_scale(best_h.normal, 0.0000001)), bounce_dir};

    colour incoming = ray_color(new_r, sc, depth+1);

    // TODO: implement random early exit somehow
    // f64 p = fmax(incoming.r, fmax(incoming.g, incoming.b));
    // if (random_value(&rng_state) >= p) stop tracing this path
    colour obj_colours[] = {
   		colour_srgb_i(0xff,0x79,0x79),
   		colour_srgb_i(0xff,0xbe,0x76),
   		colour_srgb_i(0xf6,0xe5,0x8d),
   		colour_srgb_i(0xba,0xdc,0x58),
    };

    return colour_multiply(incoming, obj_colours[best_obj]);
}

// void render(f32* img, sz width, sz height, camera cam, scene* sc) {
// 	#pragma omp parallel for schedule(dynamic)
// 	for (sz y = 0; y < height; ++y) {
// 		for (sz x = 0; x < width; ++x) {
// 			colour total_light = {0};

// 			for (sz s = 0; s < N_SAMPLES; ++s) {
// 				f64 u = (f64)x / (width - 1);
// 				f64 v = 1.0 - (f64)y / (height - 1);

// 				v3 jitter = v3_scale(random_point_in_circle(&rng_state), 1e-8);
// 				u += jitter.x;
// 				v += jitter.y;

// 				ray r = camera_get_ray(cam, u, v);
// 				colour sample = colour_add(total_light, ray_color(r, sc, 1));
// 				total_light = sample;
// 			}

// 			colour pixel = {
// 				total_light.r / N_SAMPLES,
// 				total_light.g / N_SAMPLES,
// 				total_light.b / N_SAMPLES,
// 			};

// 			colour_gamma(&pixel, 2.4);
// 			colour_clip(&pixel, 1.0);

// 			sz idx = (y * width + x) * 3;
// 			img[idx + 0] = (u8)(255 * pixel.r);
// 			img[idx + 1] = (u8)(255 * pixel.g);
// 			img[idx + 2] = (u8)(255 * pixel.b);
// 		}
// 	}
// }

void render_progressive(f32* img, sz width, sz height, camera cam, scene* sc, sz* samples_done) {
	for (sz s = 0; s < N_SAMPLES; ++s) {
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
				img[idx + 1] = 0.0;
				img[idx + 2] = 1.0;

				f64 u = (f64)x / (width - 1);
				f64 v = 1.0 - (f64)y / (height - 1);

				u += jitter.x;
				v += jitter.y;

				colour total_light = {0};

				ray r = camera_get_ray(cam, u, v);
				colour sample = colour_add(total_light, ray_color(r, sc, 1));
				total_light = sample;

				colour_gamma(&total_light, 2.4);
				colour_clip(&total_light, 1.0);

				colour pixel = colour_add(old_px, colour_divide(colour_sub(total_light, old_px), (colour){s+1,s+1,s+1}));
				img[idx + 0] = pixel.r;
				img[idx + 1] = pixel.g;
				img[idx + 2] = pixel.b;
			}
		}
		*samples_done = s+1;
	}
}
