#ifndef _SPECTRUM_H
#define _SPECTRUM_H

#include "types.h"

typedef struct {
	f64 x_bar, y_bar, z_bar;
} cmf_xyz;

// CIE 1931 CMF from 380nm to 780nm in 1 nm increment
// X, Y, Z
extern const cmf_xyz cmfs[401];

#endif
