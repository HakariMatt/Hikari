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

u64 rng_state;

static colour sky_colour(ray r) {
	v3 unit_dir = v3_norm(r.dir);
    f64 a = 0.5 * (unit_dir.z + 1.0);
    v3 white = {1.0, 1.0, 1.0}, blue = {0.3, 0.5, 1.0};
    v3 t = v3_add(v3_scale(blue, 1.0 - a), v3_scale(white, a));
    return (colour){t.x, t.y, t.z};
}

// modified function from
// https://github.com/SebLague/Ray-Tracing/blob/Episode01/Assets/Scripts/Shaders/RayTracing.shader
static u64 next_random(u64* state) {
	((*state)) = (*state) * 747796405 + 2891336453;
	u64 result = (((*state) >> (((*state) >> 28) + 4)) ^ (*state)) * 277803737;
	result = (result >> 22) ^ result;
	return result;
}

static f64 random_value(u64* state) {
	return next_random(state) / 4294967295.0; // 2^32 - 1
}

static f64 random_value_normal_distribution(u64* state) {
	f64 theta = 2 * 3.1415926 * random_value(state);
	f64 rho = sqrt(-2 * log(random_value(state)));
	return rho * cos(theta);
}

static v3 random_dir() {
	return v3_norm((v3){
		.x = random_value(&rng_state),
		.y = random_value(&rng_state),
		.z = random_value(&rng_state),
	});
}

static v3 random_point_in_circle(u64* rngState)
{
	f64 angle = random_value(rngState) * 2 * M_PI;
	v3 point_on_circle = {cos(angle), sin(angle), 0};
	return v3_scale(point_on_circle, sqrt(random_value(rngState)));
}

static colour ray_color(ray r, scene* sc, sz depth) {
	if (depth >= MAX_BOUNCES) {
		return (colour){0,0,0};
	}

	if (!hit_bbox(r, sc->objects[0].bbox)) {
        return sky_colour(r);
    }

	hit_result hr = bvh_hit(sc->objects[0].bvh, sc->objects[0].mesh, r, INFINITY);
	if (!hr.hit) {
    	return sky_colour(r);
	}

	// v3 bounce_dir = v3_reflect(r.dir, hr.normal);

	// need better diffuse reflection function...
	v3 bounce_dir = v3_norm(v3_add(hr.normal, random_dir()));
	ray new_r = {v3_add(ray_at(r, hr.t), v3_scale(hr.normal, 0.0000001)), bounce_dir};

    colour incoming = ray_color(new_r, sc, depth+1);

    // f64 p = fmax(incoming.r, fmax(incoming.g, incoming.b));
    // if (random_value(&rng_state) >= p)

    return colour_multiply(incoming, (colour){1.0, 1.0, 1.0});
    // return incoming;

}

void render(u8* img, sz width, sz height, camera cam, scene* sc) {
	#pragma omp parallel for schedule(dynamic)
	for (sz y = 0; y < height; ++y) {
		for (sz x = 0; x < width; ++x) {
			colour total_light = {0};

			for (sz s = 0; s < N_SAMPLES; ++s) {
				f64 u = (f64)x / (width - 1);
				f64 v = 1.0 - (f64)y / (height - 1);

				v3 jitter = v3_scale(random_point_in_circle(&rng_state), 1e-8);
				u += jitter.x;
				v += jitter.y;

				ray r = camera_get_ray(cam, u, v);
				colour sample = colour_add(total_light, ray_color(r, sc, 1));
				total_light = sample;
			}

			colour pixel = {
				total_light.r / N_SAMPLES,
				total_light.g / N_SAMPLES,
				total_light.b / N_SAMPLES,
			};

			colour_gamma(&pixel, 2.4);
			colour_clip(&pixel, 1.0);

			sz idx = (y * width + x) * 3;
			img[idx + 0] = (u8)(255 * pixel.r);
			img[idx + 1] = (u8)(255 * pixel.g);
			img[idx + 2] = (u8)(255 * pixel.b);
		}
	}
}
