#ifndef _COLOUR_H
#define _COLOUR_H

#include "types.h"

typedef struct {
	f64 r, g, b;
} colour;

static colour colour_multiply(colour a, colour b) {
	return (colour) { a.r * b.r, a.g * b.g, a.b * b.b };
}

#endif
