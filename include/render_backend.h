#ifndef _RENDER_BACKEND_H
#define _RENDER_BACKEND_H

#include "types.h"
#include "scene.h"

typedef struct {
	volatile int done;
	sz samples_done;

	int should_stop;
} render_state;

typedef struct {
    f32* img;
    sz width, height;
    camera cam;
    scene scene;
    render_state* state;
} render_args;

#endif