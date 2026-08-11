#ifndef _COLOUR_H
#define _COLOUR_H

#include "types.h"
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

#endif
