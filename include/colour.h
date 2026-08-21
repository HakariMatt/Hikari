#ifndef _COLOUR_H
#define _COLOUR_H

#include "types.h"
#include "v3.h"
#include "spectrum.h"
#include <math.h>

typedef struct {
	f64 r, g, b;
} colour;

static inline colour colour_multiply(colour a, colour b) {
	return (colour) { a.r * b.r, a.g * b.g, a.b * b.b };
}

static inline colour colour_add(colour a, colour b) {
	return (colour) { a.r+b.r, a.g+b.g, a.b+b.b };
}

static inline colour colour_sub(colour a, colour b) {
	return (colour) { a.r-b.r, a.g-b.g, a.b-b.b };
}

static inline colour colour_divide(colour a, colour b) {
	return (colour) { a.r/b.r, a.g/b.g, a.b/b.b };
}

static inline colour colour_scale(colour a, f64 b) {
	return (colour) { a.r*b, a.g*b, a.b*b };
}

static inline void colour_gamma(colour* a, f64 gamma) {
	f64 inv_gamma = 1.0 / gamma;
	a->r = pow(a->r, inv_gamma);
	a->g = pow(a->g, inv_gamma);
	a->b = pow(a->b, inv_gamma);
}

static inline void colour_clip(colour* a, f64 value) {
	a->r = fmin(a->r, value);
	a->g = fmin(a->g, value);
	a->b = fmin(a->b, value);
}

// (v * (2.51 * v + 0.03)) / (v * (2.43 * v + 0.59) + 0.14)
static inline colour colour_aces_tonemap(colour a) {
	return (colour) {
		(a.r * (2.51 * a.r + 0.03)) / (a.r * (2.43 * a.r + 0.59) + 0.14),
		(a.g * (2.51 * a.g + 0.03)) / (a.g * (2.43 * a.g + 0.59) + 0.14),
		(a.b * (2.51 * a.b + 0.03)) / (a.b * (2.43 * a.b + 0.59) + 0.14),
	};
}

static inline colour colour_srgb_i(int r, int g, int b) {
	return (colour) { r / 255.0, g / 255.0, b / 255.0 };
}

static inline colour v3_to_colour(v3 v) { return (colour){v.x, v.y, v.z}; }
static inline v3 colour_to_v3(colour v) { return (v3){v.r, v.g, v.b}; }

static inline v3 cmf_lookup(f64 lambda) {
	if (lambda < 380 || lambda > 780) return (v3){0,0,0};

	int floor_index = (int)floor(lambda) - 380;
	int ceiling_index = (int)ceil(lambda) - 380;

	cmf_xyz top = cmfs[ceiling_index];
	cmf_xyz bottom = cmfs[floor_index];
	f64 t = lambda - floor(lambda);

	return v3_lerp((v3){top.x_bar, top.y_bar, top.z_bar}, (v3){bottom.x_bar, bottom.y_bar, bottom.z_bar}, t);
}

static v3 xyz_to_srgb(v3 xyz) {
	return (v3) {
		 3.2404542 * xyz.x - 1.5371385 * xyz.y - 0.4985314 * xyz.z,
		-0.9692660 * xyz.x + 1.8760108 * xyz.y + 0.0415560 * xyz.z,
		 0.0556434 * xyz.x - 0.2040259 * xyz.y + 1.0572252 * xyz.z,
	};
}

static v3 E_to_D65(v3 xyz) {
	return (v3) {
		 0.95318743 * xyz.x - 0.02659057 * xyz.y + 0.02387315 * xyz.z,
		-0.03824666 * xyz.x + 1.02884062 * xyz.y + 0.00940604 * xyz.z,
	 	 0.00260677 * xyz.x - 0.00303325 * xyz.y + 1.08925647 * xyz.z,
	};
}

#endif
