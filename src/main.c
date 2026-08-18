#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

#include "../include/settings.h"
#include "../include/display.h"
#include "../include/types.h"
#include "../include/camera.h"
#include "../include/scene.h"
#include "../include/bvh.h"
#include "../include/render.h"
#include "../include/log.h"
#include "../include/mat.h"
#include "../include/colour.h"

// #define STOPWATCH(x) clock_gettime(CLOCK_MONOTONIC, &(x))

// static double seconds_since(struct timespec start, struct timespec end) {
//     return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) * 1e-9;
// }

#define IMG_SIZE WIDTH*HEIGHT*3

static void* render_thread_fn(void* arg) {
    render_args* a = (render_args*)arg;
    render_progressive(a);
    a->state->done = 1;
    return NULL;
}

int main(void) {

	// scene setup
	scene sc = {0};

	mat_lib m_lib = {0};
	sc.mat_lib = &m_lib;

    camera cam = camera_make(
        (v3)CAMERA_POS, (v3)CAMERA_LOOKAT, (v3){0,0,1},
        CAMERA_FOV, (f64)WIDTH/HEIGHT, 0.0, 1.0
    );

   	scene_load_obj(&sc, "assets/models/Hikari_with_bg.obj");

    sz i = 0;

    i = mat_get(sc.mat_lib, "Halo");
    int halo = mat_node_emission(&m_lib, (v3){0.296138, 1.000000, 0.520996}, 5);
    sc.mat_lib->materials[i].root_socket = halo;

    i = mat_get(sc.mat_lib, "CH0242_Body");
    int body = mat_node_diffuse(&m_lib, (v3){0.000000, 0.954206, 1.000000});
    sc.mat_lib->materials[i].root_socket = body;

    i = mat_get(sc.mat_lib, "CH0242_Hair_2");
    int hair = mat_node_diffuse(&m_lib, (v3){0, 1, 0.223717});
    sc.mat_lib->materials[i].root_socket = hair;

    i = mat_get(sc.mat_lib, "CH0242_EyeMouth");
    int eye = mat_node_diffuse(&m_lib, (v3){1,1,1});
    sc.mat_lib->materials[i].root_socket = eye;

    i = mat_get(sc.mat_lib, "CH0242_Face");
    int face = mat_node_diffuse(&m_lib, (v3){1.000000, 0.822792, 0.693886});
    sc.mat_lib->materials[i].root_socket = face;

    i = mat_get(sc.mat_lib, "Floor");
    int floor = mat_node_diffuse(&m_lib, (v3){0.056085, 0.057917, 0.072421});
    sc.mat_lib->materials[i].root_socket = floor;

    i = mat_get(sc.mat_lib, "Light");
    int light = mat_node_emission(&m_lib, (v3){1, 1, 1}, 50);
    sc.mat_lib->materials[i].root_socket = light;

    for (sz i = 0; i < sc.obj_count; ++i) {
    	sc.objects[i].bvh = bvh_build_root(sc.objects[i].mesh);
    }

    // creating pixel buffer
    f32* img = malloc(IMG_SIZE*sizeof(f32));
    memset(img, 0, IMG_SIZE);

    // rendering
    render_state rs = {0};
    render_args rargs = {
    	.img = img,
     	.width = WIDTH,
      	.height = HEIGHT,
       	.cam = cam,
        .scene = sc,
        .state = &rs
    };
    pthread_t render_thread;
    pthread_create(&render_thread, NULL, render_thread_fn, &rargs);
    display_run(&rargs);
    pthread_join(render_thread, NULL);

    // converting to 8bit rgb image
	u8* outimg = malloc(IMG_SIZE);
	for (sz i = 0; i < IMG_SIZE; ++i) {
		outimg[i] = (u8)fmax(fmin(img[i] * 255.0, 255.0), 0.0);
	}

	// writing image to file
    FILE* f = fopen("output.ppm", "wb");
    if (!f) print(ERROR, "Couldn't open file");

    fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    fwrite(outimg, 1, IMG_SIZE, f);
    fclose(f);

    print(INFO, "Image saved.");

    // clean up
    scene_free(&sc);
    free(img);
    free(outimg);

    return 0;
}
