#ifndef _HIKARI_INTERNAL_H
#define _HIKARI_INTERNAL_H

#include <pthread.h>

#include "Hikari.h"
#include "render_backend.h"
#include "scene.h"
#include "types.h"

struct HikariContext {
    HikariSettings settings;
    scene scene;
    mat_lib mat_lib;
    emission_list emission_list;
    render_state render_state;
    render_args render_args;
    RGB2Spec* spec_model;
    float* image;
    const render_backend* backend;
    int backend_initialized;
    pthread_t render_thread;
    int has_thread;
    int has_camera;
};

#endif
