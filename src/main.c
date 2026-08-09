#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#include "settings.h"
#include "display.c"
#include "types.h"
#include "camera.c"
#include "mesh.c"
#include "render.c"
#include "bvh.c"

#define SECONDS(t1, t2) (((double) (t2 - t1)) / CLOCKS_PER_SEC)

// #define LOWRES

// #ifdef LOWRES
// 	#define WIDTH  320
// 	#define HEIGHT 240
// #else
// 	#define WIDTH  (640)
// 	#define HEIGHT (480)
// #endif
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

    camera cam = camera_make(
        (v3){-1.5,1.5,1.5}, (v3){0,.56,0}, (v3){0,1,0},
        30.0, (f64)WIDTH/HEIGHT, 0.0, 1.0
    );

	time_t start = clock();
    object obj = mesh_load_obj("/Users/hakarimatt/Documents/Programming/Raytracer/Matomi.obj");
    time_t mesh_loaded = clock();

    obj.bvh = bvh_build_root(obj.mesh);
    time_t bvh_built = clock();

    u8* img = malloc(IMG_SIZE);
    memset(img, 0, IMG_SIZE); // starts black instead of garbage

    render_args rargs = { img, WIDTH, HEIGHT, cam, obj, 0 };
    pthread_t render_thread;
    pthread_create(&render_thread, NULL, render_thread_fn, &rargs);

    display_run(img, WIDTH, HEIGHT, &rargs.done); // blocks until window closed

    pthread_join(render_thread, NULL);

    time_t rendered = clock();

    FILE* f = fopen("output.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    fwrite(img, 1, IMG_SIZE, f);
    fclose(f);

    time_t image_written = clock();

    mesh_free(obj.mesh);
    bvh_free(obj.bvh);
    free(img);

    // fprintf(stderr, "\nDone\n");
    fprintf(stderr, "Model loaded in %.5f seconds\n", SECONDS(start, mesh_loaded));
    fprintf(stderr, "BVH built in    %.5f seconds\n", SECONDS(mesh_loaded, bvh_built));
    fprintf(stderr, "Rendered in     %.5f seconds\n", SECONDS(bvh_built, rendered));
    fprintf(stderr, "Image saved in  %.5f seconds\n", SECONDS(rendered, image_written));
    return 0;
}
