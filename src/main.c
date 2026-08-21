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
#include "../include/log.h"
#include "../include/mat.h"

#include "../include/render_backend.h"
#ifdef HIKARI_METAL
#include "../include/metal_backend.h"
#endif


#define IMG_SIZE WIDTH*HEIGHT*3

// static void* render_thread_fn(void* arg) {
//     render_args* a = (render_args*)arg;
//     render_progressive(a);
//     a->state->done = 1;
//     return NULL;
// }

static void* render_thread_fn(void* arg) {
	render_args* a = (render_args*)arg;
	a->backend->render(a);
	a->state->done = 1;
	return NULL;
}

int main(int argc, char* argv[]) {
	int force_cpu = 0;
	for (int i = 1; i < argc; ++i)
		if (strcmp(argv[i], "-cpu") == 0) force_cpu = 1;

	const render_backend* backend = &cpu_backend_ops;
	#ifdef HIKARI_METAL
		if (!force_cpu) backend = &metal_backend_ops;
	#endif
		print(INFO, "Using %s backend", backend->name);

	// scene setup
	scene sc = {0};

	mat_lib m_lib = {0};
	sc.mat_lib = &m_lib;

	scene_load_obj(&sc, "assets/models/Hikari_in_cornell_box.obj");

    camera cam = camera_make(
        (v3)CAMERA_POS, (v3)CAMERA_LOOKAT, (v3){0,0,1},
        CAMERA_FOV, (f64)WIDTH/HEIGHT, 0.0, 1.0
    );

    sz i = 0;
    int white_matte = mat_node_diffuse(&m_lib, (v3){1,1,1});

    // Hikari materials
    i = mat_get(sc.mat_lib, "Halo");
    sc.mat_lib->materials[i].root_socket = mat_node_emission(&m_lib, (v3){0.296138, 1.000000, 0.520996}, 1);
    // sc.mat_lib->materials[i].root_socket = mat_node_diffuse(&m_lib, (v3){0.000000, 0.954206, 1.000000});

    i = mat_get(sc.mat_lib, "CH0242_Body");
    sc.mat_lib->materials[i].root_socket = white_matte;

    i = mat_get(sc.mat_lib, "CH0242_Hair_2");
    sc.mat_lib->materials[i].root_socket = white_matte;

    i = mat_get(sc.mat_lib, "CH0242_EyeMouth");
    sc.mat_lib->materials[i].root_socket = white_matte;

    i = mat_get(sc.mat_lib, "CH0242_Face");
    sc.mat_lib->materials[i].root_socket = white_matte;

    // i = mat_get(sc.mat_lib, "Floor");
    // sc.mat_lib->materials[i].root_socket = mat_node_diffuse(&m_lib, (v3){0.056085, 0.057917, 0.072421});

    i = mat_get(sc.mat_lib, "Light");
    sc.mat_lib->materials[i].root_socket = mat_node_emission(&m_lib, (v3){1, 1, 1}, 20);

    i = mat_get(sc.mat_lib, "Back");
    sc.mat_lib->materials[i].root_socket = white_matte;
    i = mat_get(sc.mat_lib, "Ceiling");
    sc.mat_lib->materials[i].root_socket = white_matte;
    i = mat_get(sc.mat_lib, "Floor");
    sc.mat_lib->materials[i].root_socket = white_matte;

    i = mat_get(sc.mat_lib, "Right");
    sc.mat_lib->materials[i].root_socket = mat_node_diffuse(&m_lib, (v3){0.163521, 0.800015, 0.122934});
    i = mat_get(sc.mat_lib, "Left");
    sc.mat_lib->materials[i].root_socket = mat_node_diffuse(&m_lib, (v3){0.800007, 0.171799, 0.122933});

    // creating pixel buffer
    f32* img = malloc(IMG_SIZE*sizeof(f32));
    memset(img, 0, IMG_SIZE);

    // rendering
    render_state rs = {0};
    render_args rargs = {
		.img = img, .width = WIDTH, .height = HEIGHT,
		.cam = cam, .scene = sc, .state = &rs,
		.ctx = NULL, .backend = backend
	};

    if (backend->init(&rargs) != 0) {
		print(ERROR, "%s backend init failed", backend->name);
		return 1;
	}

    pthread_t render_thread;
	pthread_create(&render_thread, NULL, render_thread_fn, &rargs);
	display_run(&rargs);
	pthread_join(render_thread, NULL);

	backend->shutdown(&rargs);

    // converting to 8bit rgb image
	u8* outimg = malloc(IMG_SIZE);
	for (sz i = 0; i < IMG_SIZE; ++i) {
		f32 p = img[i];
		p = powf(p, 1/2.4);
		outimg[i] = (u8)fmax(fmin(p * 255.0, 255.0), 0.0);
	}

	// writing image to file
	char filename[128];
	sprintf(filename, "renders/hikari_%s_%dx%d_%d.ppm", backend->name, WIDTH, HEIGHT, N_SAMPLES);
    FILE* f = fopen(filename, "wb");
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
