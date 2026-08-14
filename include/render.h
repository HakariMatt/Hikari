#ifndef _RENDER_H
#define _RENDER_H

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

// void render(f32* img, sz width, sz height, camera cam, scene* sc);
void render_progressive(render_args*);

#endif
