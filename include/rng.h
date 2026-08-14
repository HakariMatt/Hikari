#ifndef _RNG_H
#define _RNG_H

#include "types.h"

// modified function from
// https://github.com/SebLague/Ray-Tracing/blob/Episode01/Assets/Scripts/Shaders/RayTracing.shader
static u32 next_random(u32* state) {
	((*state)) = (*state) * 747796405 + 2891336453;
	u32 result = (((*state) >> (((*state) >> 28) + 4)) ^ (*state)) * 277803737;
	result = (result >> 22) ^ result;
	return result;
}

// random f64 [0, 1)
static f64 random_f64(u32* state) {
	return next_random(state) * (1.0 / 4294967296.0);
}

#endif
