#ifndef _SPECTRUM_H
#define _SPECTRUM_H

#include "types.h"
#include "../rgb2spec/rgb2spec.h"

typedef struct {
	f64 x_bar, y_bar, z_bar;
} cmf_xyz;

// CIE 1931 CMF from 380nm to 780nm in 1 nm increment
// X, Y, Z
extern const cmf_xyz cmfs[401];
static const f64 cmf_norm_k = 1.0 / 106.856;

v4 spectral_upsample(RGB2Spec* model, v3 rgb, shading_ctx* ctx);
f64 wrap_wavelength(f64 lambda, f64 l_min, f64 l_max);

#endif
