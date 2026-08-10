#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include "../include/settings.h"
#include "../include/display.h"
#include "../include/types.h"
#include "../include/camera.h"
#include "../include/mesh.h"
#include "../include/bvh.h"
#include "../include/render.h"

// #define SECONDS(t1, t2) (((double) (t2 - t1)) / CLOCKS_PER_SEC)

static double seconds_since(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
}

#define IMG_SIZE WIDTH*HEIGHT*3

typedef struct {
    u8* img;
    int width, height;
    camera cam;
    object obj;
    volatile int done;
} render_args;

static void* render_thread_fn(void* arg) {
    render_args* a = (render_args*)arg;
    render(a->img, a->width, a->height, a->cam, a->obj);
    a->done = 1;
    return NULL;
}

int main(void) {
	struct timespec t0, t1, t2, t3, t4;

    // camera cam = camera_make(
    //     (v3){-1.5,1.5,1.5}, (v3){0,.56,0}, (v3){0,1,0},
    //     30.0, (f64)WIDTH/HEIGHT, 0.0, 1.0
    // );
    // camera cam = camera_make(
    //     (v3){0,-2.46,0}, (v3){0,0,0}, (v3){0,0,1},
    //     90.0, (f64)WIDTH/HEIGHT, 0.0, 1.0
    // );
    camera cam = camera_make(
        (v3)CAMERA_POS, (v3)CAMERA_LOOKAT, (v3){0,0,1},
        CAMERA_FOV, (f64)WIDTH/HEIGHT, 0.0, 1.0
    );

    clock_gettime(CLOCK_MONOTONIC, &t0);
    object obj = mesh_load_obj("Matomi.obj");

    clock_gettime(CLOCK_MONOTONIC, &t1);

    obj.bvh = bvh_build_root(obj.mesh);
    clock_gettime(CLOCK_MONOTONIC, &t2);

    u8* img = malloc(IMG_SIZE);
    memset(img, 0, IMG_SIZE);

    render_args rargs = { img, WIDTH, HEIGHT, cam, obj, 0 };
    pthread_t render_thread;
    pthread_create(&render_thread, NULL, render_thread_fn, &rargs);

    display_run(img, WIDTH, HEIGHT, &rargs.done);

    pthread_join(render_thread, NULL);

    clock_gettime(CLOCK_MONOTONIC, &t3);

    FILE* f = fopen("output.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    fwrite(img, 1, IMG_SIZE, f);
    fclose(f);

    clock_gettime(CLOCK_MONOTONIC, &t4);

    mesh_free(obj.mesh);
    bvh_free(obj.bvh);
    free(img);

    // fprintf(stderr, "\nDone\n");
    fprintf(stderr, "Model loaded in %.5f seconds\n", seconds_since(t0, t1));
    fprintf(stderr, "BVH built in    %.5f seconds\n", seconds_since(t1, t2));
    fprintf(stderr, "Rendered in     %.5f seconds\n", seconds_since(t2, t3));
    fprintf(stderr, "Image saved in  %.5f seconds\n", seconds_since(t3, t4));
    return 0;
}
