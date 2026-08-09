#ifndef _CAMERA_C
#define _CAMERA_C

#include <math.h>
#include "types.h"
#include "v3.c"

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

#endif
