#include <math.h>
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
#include "../include/scene.h"
#include "../include/bvh.h"
#include "../include/render.h"

#define STOPWATCH(x) clock_gettime(CLOCK_MONOTONIC, &(x))

// #define SECONDS(t1, t2) (((double) (t2 - t1)) / CLOCKS_PER_SEC)

static double seconds_since(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
}

#define IMG_SIZE WIDTH*HEIGHT*3

typedef struct {
    f32* img;
    sz width, height;
    camera cam;
    scene scene;
    sz samples_rendered;
    volatile int done;
} render_args;

static void* render_thread_fn(void* arg) {
    render_args* a = (render_args*)arg;
    render_progressive(a->img, a->width, a->height, a->cam, &a->scene, &a->samples_rendered);
    a->done = 1;
    return NULL;
}

int main(void) {
	struct timespec t0, t1, t2, t3, t4;

	scene sc = {0};

    camera cam = camera_make(
        (v3)CAMERA_POS, (v3)CAMERA_LOOKAT, (v3){0,0,1},
        CAMERA_FOV, (f64)WIDTH/HEIGHT, 0.0, 1.0
    );

    STOPWATCH(t0);

   	scene_load_obj(&sc, "assets/models/Hikari.obj");
   	// scene_load_obj(&sc, "assets/models/sphere.obj");
   	// scene_load_obj(&sc, "assets/models/scene.obj");

    printf("object 1:\n");
    printf(" - verts: %zu\n", sc.objects[0].mesh.nverts);
    printf(" - tris:  %zu\n", sc.objects[0].mesh.ntris);

    STOPWATCH(t1);

    for (sz i = 0; i < sc.obj_count; ++i) {
    	sc.objects[i].bvh = bvh_build_root(sc.objects[i].mesh);
    }

    STOPWATCH(t2);

    f32* img = malloc(IMG_SIZE*sizeof(f32));
    memset(img, 0, IMG_SIZE);

    render_args rargs = { img, WIDTH, HEIGHT, cam, sc, 0, 0 };
    pthread_t render_thread;
    pthread_create(&render_thread, NULL, render_thread_fn, &rargs);

    display_run(img, WIDTH, HEIGHT, &rargs.done, &rargs.samples_rendered);

    pthread_join(render_thread, NULL);

    STOPWATCH(t3);


	u8* outimg = malloc(IMG_SIZE);
	for (sz i = 0; i < IMG_SIZE; ++i) {
		outimg[i] = (u8)fmax(fmin(img[i] * 255.0, 255.0), 0.0);
	}

    FILE* f = fopen("output.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    fwrite(img, 1, IMG_SIZE, f);
    fclose(f);

    STOPWATCH(t4);

    scene_free(&sc);
    free(img);

    // fprintf(stderr, "\nDone\n");
    fprintf(stderr, "Model loaded in %.5f seconds\n", seconds_since(t0, t1));
    fprintf(stderr, "BVH built in    %.5f seconds\n", seconds_since(t1, t2));
    fprintf(stderr, "Rendered in     %.5f seconds\n", seconds_since(t2, t3));
    fprintf(stderr, "Image saved in  %.5f seconds\n", seconds_since(t3, t4));
    return 0;
}
