#ifndef _RENDER_BACKEND_H
#define _RENDER_BACKEND_H

#include "types.h"
#include "scene.h"

typedef struct {
	volatile int done;
	sz samples_done;

	int should_stop;
} render_state;

typedef struct render_backend render_backend;
typedef struct {
    f32* img;
    sz width, height;
    camera cam;
    scene scene;
    render_state* state;
    void* ctx;
    struct render_backend* backend;
} render_args;

struct render_backend {
	char* name;
	int  (*init)(render_args* args);
	void (*render)(render_args* args);
	void (*shutdown)(render_args* args);
};

extern const render_backend cpu_backend_ops;
#ifdef HIKARI_METAL
extern const render_backend metal_backend_ops;
#endif

#endif
