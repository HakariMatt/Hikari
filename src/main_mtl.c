#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "../include/log.h"
#include "../include/metal_backend.h"
#include "../include/types.h"
#include "../include/settings.h"
#include "../include/camera.h"
#include "../include/scene.h"
#include "../include/render_backend.h"
#include "../include/mat.h"
#include "../include/display.h"

#define IMG_SIZE WIDTH*HEIGHT*3
#define STOPWATCH(x) clock_gettime(CLOCK_MONOTONIC, &(x))

static void render(render_args* args) {
	struct timespec t0, t1;

	STOPWATCH(t0);
	for (size_t i = 0; i < N_SAMPLES; ++i) {
		metalRenderSample((metal_ctx*)args->ctx, *args, i);
		args->state->samples_done = i;
	}
	STOPWATCH(t1);

	print(INFO, "Rendered in %.3f s.", (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9);
}

static void* render_thread_fn(void* args) {
    render_args* a = (render_args*)args;
    render(a);
    a->state->done = 1;
    return NULL;
}

int main(void) {

    f32* img = malloc(IMG_SIZE*sizeof(f32));
    memset(img, 0, IMG_SIZE);

    // scene setup
    scene sc = {0};
    mat_lib m_lib = {0};
    sc.mat_lib = &m_lib;

    scene_load_obj(&sc, "assets/models/Hikari.obj");

    camera cam = camera_make(
        (v3)CAMERA_POS, (v3)CAMERA_LOOKAT, (v3){0,0,1},
        CAMERA_FOV, (f64)WIDTH/HEIGHT, 0.0, 1.0
    );

    metal_ctx* ctx = metalInit();
    if (!ctx) { print(ERROR, "init failed\n"); return 1; }
    if (metalUploadScene(ctx, &sc) != 0) { print(ERROR, "scene upload failed\n"); return 1; }

    render_state state = {0};
    render_args args = {
        .img = img, .cam = cam, .width = WIDTH, .height = HEIGHT,
        .scene = sc, .state = &state, .ctx = ctx
    };

    pthread_t render_thread;
    pthread_create(&render_thread, NULL, render_thread_fn, &args);
    display_run(&args);
    pthread_join(render_thread, NULL);

    u8* out = malloc(IMG_SIZE);
    for (sz i = 0; i < (sz)(IMG_SIZE); ++i) out[i] = (u8)(img[i] * 255.0f);

    FILE* f = fopen("output.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    fwrite(out, 1, IMG_SIZE, f);
    fclose(f);

    metalShutdown(ctx);
    free(img); free(out);

    return 0;
}
