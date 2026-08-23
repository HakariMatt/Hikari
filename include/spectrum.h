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

v4 spectral_upsample(RGB2Spec* model, v3 rgb, f64 lambda0);
f64 wrap_wavelength(f64 lambda, f64 l_min, f64 l_max);

#endif
