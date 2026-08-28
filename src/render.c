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
		args->scene.objects[i].bvh = bvh_build_root(
			args->scene.objects[i].mesh,
			args->settings.bvh_max_depth,
			args->settings.bvh_leaf_tris
		);
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

static f64 random_wavelength(u32* state, HikariSettings settings) {
	f64 u = random_f64(state);
	return u * (settings.lambda_max - settings.lambda_min) + settings.lambda_min;
}

static colour sky_colour(ray r) {
	v3 unit_dir = v3_norm(r.dir);
    f64 a = 0.5 * (unit_dir.z + 1.0);
    v3 white = {1,1,1}, blue = {.3,.5,1};
    v3 t = v3_add(v3_scale(blue, 1.0 - a), v3_scale(white, a));
    return (colour){t.x, t.y, t.z};
    // return (colour){0,0,0};
}

static sz emission_list_find(emission_list* list, f64 u) {
	sz lo = 0, hi = list->count - 1;
	while (lo < hi) {
		sz mid = (lo + hi) / 2;
		if (list->tris[mid].cum_area < u) lo = mid + 1;
		else hi = mid;
	}
	return lo;
}

static v3 sample_emission_point(scene* sc, u32* rng_state, f64* pdf, sz* mat_id, v3* normal_out) {
	f64 u = random_f64(rng_state) * sc->emission_list->total_area;
	emission_tris* e_tri = &sc->emission_list->tris[emission_list_find(sc->emission_list, u)];

	f64 u0 = random_f64(rng_state);
	f64 u1 = random_f64(rng_state);

	mesh m = sc->objects[e_tri->obj_id].mesh;
	tri t = m.tris[e_tri->tri_id];
	v3 v0 = m.verts[(int)t.verts_idx.x];
	v3 v1 = m.verts[(int)t.verts_idx.y];
	v3 v2 = m.verts[(int)t.verts_idx.z];

	if (u0 + u1 > 1) { u0 = 1.0 - u0; u1 = 1.0 - u1; }
	f64 b0 = 1.0 - u0 - u1, b1 = u0, b2 = u1;

	*pdf = 1.0 / sc->emission_list->total_area;
	*mat_id = t.mat_id;
	*normal_out = v3_norm(v3_cross(v3_sub(v1, v0), v3_sub(v2, v0)));

	return v3_add(v3_add(v3_scale(v0, b0), v3_scale(v1, b1)), v3_scale(v2, b2));
}

static hit_result intersect(scene* sc, ray r) {
	hit_result hr;
	hit_result best_h = {.hit = 0};
	f64 closest = INFINITY;
	sz best_obj = -1;
	for (sz i = 0; i < sc->obj_count; ++i) {
		if (!hit_bbox(r, sc->objects[i].bbox)) continue;

		hr = bvh_hit(sc->objects[i].bvh, sc->objects[i].mesh, r, closest);

		if (hr.hit && hr.t < closest) {
			closest = hr.t;
			best_h = hr;
			best_obj = i;
		}
	}
	return best_h;
}

static f64 power_heuristic(f64 pdf_a, f64 pdf_b) {
	f64 a2 = pdf_a*pdf_a, b2 = pdf_b*pdf_b;
	return a2 / (a2 + b2);
}

static light_sample trace_path(ray r, scene* sc, u32* rng_state, HikariSettings settings) {
	f64 hero_wavelength = random_wavelength(rng_state, settings);
	light_sample radiance = { {0,0,0,0}, hero_wavelength };
	light_sample throughput = { {1,1,1,1}, hero_wavelength };

	f64 prev_bsdf_pdf = 0;     // solid-angle pdf of the direction that produced `r`
	int prev_specular = 1;     // depth 0 (camera ray): no prior bsdf sample to weight against

	for (sz depth = 0; depth < settings.max_bounces; ++depth) {
		hit_result hr = intersect(sc, r);

		if (!hr.hit) {
			radiance.value = v4_add(radiance.value, v4_scale(throughput.value, 0));
			break;
		}

		if (v3_dot(r.dir, hr.true_normal) > 0) {
			r = (ray) {
				.origin = v3_add(ray_at(r, hr.t), v3_scale(hr.normal, 1e-7)),
				.dir = r.dir
			};
			continue;
		}

		mat m = sc->mat_lib->materials[hr.mat_id];
		v3 hit_point = ray_at(r, hr.t);

		// hit light right away
		if (is_emissive(sc->mat_lib, hr.mat_id)) {
			bsdf_result self_emit = eval_bsdf(sc->mat_lib, sc->spec_model, m.root_socket,
				&(shading_ctx){ .lambda0 = hero_wavelength, .lambda_min = settings.lambda_min, .lambda_max = settings.lambda_max });

			f64 weight = 1.0;
			if (!prev_specular) {
				f64 ray_len = v3_len(r.dir);
				f64 dist = hr.t * ray_len;
				f64 cos_light = fmax(v3_dot(v3_scale(r.dir, -1.0/ray_len), hr.true_normal), 0.0);
				f64 pdf_light = cos_light > 0
					? (1.0 / sc->emission_list->total_area) * dist * dist / cos_light
					: 0;
				weight = power_heuristic(prev_bsdf_pdf, pdf_light);
			}
			radiance.value = v4_add(radiance.value, v4_scale(v4_mul(throughput.value, self_emit.emission), weight));
		}

		// NEE
		f64 pdf_area;
		sz emission_mat_id;
		v3 light_normal;
		v3 light_point = sample_emission_point(sc, rng_state, &pdf_area, &emission_mat_id, &light_normal);

		v3 to_light = v3_sub(light_point, hit_point);
		f64 dist2 = v3_dot(to_light, to_light), dist = sqrt(dist2);
		v3 wi = v3_scale(to_light, 1.0/dist);

		f64 cos_surface = v3_dot(wi, hr.normal);
		f64 cos_light   = v3_dot(v3_scale(wi, -1.0), light_normal);

		if (cos_surface > 0 && cos_light > 0) {
			ray dls_ray = { .origin = v3_add(hit_point, v3_scale(hr.normal, 1e-7)), .dir = wi };
			hit_result dls_hit = intersect(sc, dls_ray);
			if (!(dls_hit.hit && dls_hit.t < dist - 1e-4)) {
				shading_ctx nee_ctx = {
					.point = hit_point, .lambda0 = hero_wavelength,
					.lambda_min = settings.lambda_min, .lambda_max = settings.lambda_max,
					.normal = hr.normal, .true_normal = hr.true_normal,
					.r = r, .rng_state = rng_state
				};
				v4 bsdf_val = eval_bsdf_response(sc->mat_lib, sc->spec_model, m.root_socket, &nee_ctx, wi);
				f64 pdf_bsdf = eval_bsdf_pdf(sc->mat_lib, m.root_socket, &nee_ctx, wi);

				mat light_mat = sc->mat_lib->materials[emission_mat_id];
				bsdf_result light_emit = eval_bsdf(sc->mat_lib, sc->spec_model, light_mat.root_socket,
					&(shading_ctx){ .lambda0 = hero_wavelength, .lambda_min = settings.lambda_min, .lambda_max = settings.lambda_max });

				f64 pdf_light = pdf_area * dist2 / cos_light;
				f64 weight = power_heuristic(pdf_light, pdf_bsdf);

				v4 contrib = v4_scale(v4_mul(bsdf_val, light_emit.emission), weight * cos_surface / pdf_light);
				radiance.value = v4_add(radiance.value, v4_mul(throughput.value, contrib));
			}
		}

		shading_ctx ctx = {
			.point = hit_point,
			.normal = hr.normal,
			.true_normal = hr.true_normal,
			.r = r,
			.lambda0 = hero_wavelength,
			.lambda_min = settings.lambda_min,
			.lambda_max = settings.lambda_max,
			.rng_state = rng_state
		};

		bsdf_result bsdf = eval_bsdf(sc->mat_lib, sc->spec_model, m.root_socket, &ctx);
		if (!bsdf.scattered) break;

		prev_bsdf_pdf = bsdf.pdf;
		prev_specular = 0;   // flip to 1 here if you ever add a delta BSDF (mirror/glass)

		throughput.value = v4_mul(throughput.value, bsdf.attenuation);

		f64 p = fmax(fmax(throughput.value.x, throughput.value.y), fmax(throughput.value.z, throughput.value.w));
		p = fmin(p, 1.0);
		if (depth > MIN_RR_DEPTH) {
			if (random_f64(rng_state) > p) break;
			throughput.value = v4_scale(throughput.value, 1.0/p);
		}

		r = (ray){ .origin = v3_add(ctx.point, v3_scale(hr.normal, 1e-7)), .dir = bsdf.dir };
	}

	return radiance;
}


void render_progressive(render_args* args) {
	#define STOPWATCH(x) clock_gettime(CLOCK_MONOTONIC, &(x))
	struct timespec t0, t1;

	u32 rng_state;

	sz width = args->settings.width;
	sz height = args->settings.height;
	f32* img = args->img;
	scene* sc = &args->scene;
	camera cam = sc->camera;

	// print(INFO, "Width:   %zu", width);
	// print(INFO, "Height:  %zu", height);
	// print(INFO, "Samples: %zu", args->settings.samples);
	// print(INFO, "Bounces: %zu", args->settings.max_bounces);

	STOPWATCH(t0);

	for (sz s = 0; s < args->settings.samples; ++s) {
		f64 lambda_min = args->settings.lambda_min;
		f64 lambda_max = args->settings.lambda_max;

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

				light_sample sample = trace_path(r, sc, &rng_state, args->settings);

				colour colour_sample = {0};

				v3 xyz = {0,0,0};
				for (int i = 0; i < 4; ++i) {
					f64 lambda_i = wrap_wavelength(sample.lambda0 + i * ((lambda_max - lambda_min) / 4.0), lambda_min, lambda_max);
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

				xyz = v3_scale(xyz, ((lambda_max - lambda_min) / 4.0) * cmf_norm_k);

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
