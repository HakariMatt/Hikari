#ifndef _RENDER_H
#define _RENDER_H

#include "types.h"
#include "scene.h"
#include "render_backend.h"

typedef struct {
	v4 value;
	f64 lambda0;
} light_sample;

// void render(f32* img, sz width, sz height, camera cam, scene* sc);
void render_progressive(render_args*);

#endif
