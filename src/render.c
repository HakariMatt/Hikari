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
#include "../include/spectrum.h"
#include "../include/render.h"
#include "../include/log.h"
#include "../include/mat.h"


static int cpu_backend_init(render_args* args) {
	for (sz i = 0; i < args->scene.obj_count; ++i) {
		args->scene.objects[i].bvh = bvh_build_root(args->scene.objects[i].mesh);
	}
	return 0;
}

static void cpu_backend_shutdown(render_args* args) { (void)args; }

const render_backend cpu_backend_ops = {
	.name = "cpu",
	.init = cpu_backend_init,
	.render = render_progressive,
	.shutdown = cpu_backend_shutdown,
};

static f64 random_wavelength(u32* state) {
	f64 u = random_f64(state);
	return u * (LAMBDA_MAX - LAMBDA_MIN) + LAMBDA_MIN;
}

static colour sky_colour(ray r) {
	v3 unit_dir = v3_norm(r.dir);
    f64 a = 0.5 * (unit_dir.z + 1.0);
    v3 white = {1,1,1}, blue = {.3,.5,1};
    v3 t = v3_add(v3_scale(blue, 1.0 - a), v3_scale(white, a));
    return (colour){t.x, t.y, t.z};
    // return (colour){0,0,0};
}


static inline f64 wrap_wavelength(f64 lambda, f64 l_min, f64 l_max) {
	f64 range = l_max - l_min;
	f64 offset = fmod(lambda - l_min, range);
	if (offset < 0.0) offset += range;
	return l_min + offset;
}


static light_sample trace_path(ray r, scene* sc, u32* rng_state) {

	f64 hero_wavelength = random_wavelength(rng_state);
	light_sample radiance = { {0,0,0,0}, hero_wavelength };
	light_sample throughput = { {1,1,1,1}, hero_wavelength };

	for (sz depth = 0; depth < MAX_BOUNCES; ++depth) {
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

		if (!best_h.hit) {
			radiance.value = v4_add(radiance.value, v4_scale(throughput.value, 0));
			break;
		}

		shading_ctx ctx = {
			.point = ray_at(r, best_h.t),
			.normal = best_h.normal,
			.true_normal = best_h.true_normal,
			.r = r,
			.rng_state = rng_state
		};

		mat m = sc->mat_lib->materials[best_h.mat_id];
		bsdf_result bsdf = eval_bsdf(sc->mat_lib, m.root_socket, &ctx);

		// temporary values, for testing
		f64 emission = (bsdf.emission.x + bsdf.emission.y + bsdf.emission.z) / 3;
		f64 attenuation = (bsdf.attenuation.x + bsdf.attenuation.y + bsdf.attenuation.z) / 3;

		radiance.value = v4_add(radiance.value, v4_scale(throughput.value, emission));
		if (!bsdf.scattered) break;

		throughput.value = v4_scale(throughput.value, attenuation);

		f64 p = fmax(fmax(throughput.value.x, throughput.value.y), fmax(throughput.value.z, throughput.value.w));
		if (p < random_f64(rng_state)) break;

		r = (ray) {
			.origin = v3_add(ctx.point, v3_scale(best_h.normal, 1e-7)),
			.dir = bsdf.dir
		};
	}

	return radiance;
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


				ray r = camera_get_ray(cam, u, v);

				light_sample sample = trace_path(r, sc, &rng_state);

				colour colour_sample = {0};

				v3 xyz = {0,0,0};
				for (int i = 0; i < 4; ++i) {
					f64 lambda_i = wrap_wavelength(sample.lambda0 + i * (LAMBDA_BAR / 4), LAMBDA_MIN, LAMBDA_MAX);
					v3 cmf = cmf_lookup(lambda_i);
					f64 value = 0;
					switch (i) {
						case 0: value = sample.value.x; break;
						case 1: value = sample.value.y; break;
						case 2: value = sample.value.z; break;
						case 3: value = sample.value.w; break;
					}
					xyz = v3_add(xyz, v3_scale(cmf, value));
				}

				xyz = v3_scale(xyz, (LAMBDA_BAR / 4.0) * CMF_NORM_K);

				colour_sample = v3_to_colour(xyz_to_srgb(E_to_D65(xyz)));

				colour pixel = colour_add(old_px, colour_divide(colour_sub(colour_sample, old_px), (colour){s+1,s+1,s+1}));
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
