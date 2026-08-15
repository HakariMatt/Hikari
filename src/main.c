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

	// mat_node RGB_node = {
	// 	.type = NODE_CONST_COLOUR,
	// 	.out_type = NV_COLOUR,
	// 	.out_data.v3 = (v3) {0,0,0},	// should be able to set its value in API
	// 	.inputs = (mat_node_sockets){0}
	// };

	mat_node_socket halo_sockets[] = {
		(mat_node_socket) { .type = NV_COLOUR, .data.v3 = {0.296138, 1.000000, 0.520996}, .link = NULL },
		(mat_node_socket) { .type = NV_FLOAT, .data.value = 5, .link = NULL },
	};
	mat_node halo = {
		.type = NODE_EMISSION,
		.out_type = NV_BSDF,
		.out_data = {0},
		.inputs = (mat_node_sockets){
			.sockets = halo_sockets,
			.count = 2,
			.cap = 2,
		}
	};

	mat_node_socket light_sockets[] = {
		(mat_node_socket) { .type = NV_COLOUR, .data.v3 = {1, 1, 1}, .link = NULL },
		(mat_node_socket) { .type = NV_FLOAT, .data.value = 20, .link = NULL },
	};
	mat_node light = {
		.type = NODE_EMISSION,
		.out_type = NV_BSDF,
		.out_data = {0},
		.inputs = (mat_node_sockets){
			.sockets = light_sockets,
			.count = 2,
			.cap = 2,
		}
	};

	mat_node_socket body_sockets[] = {
		(mat_node_socket) { .type = NV_COLOUR, .data.v3 = {0.000000, 0.954206, 1.000000}, .link = NULL },
	};
	mat_node body = {
		.type = NODE_DIFFUSE,
		.out_type = NV_BSDF,
		.out_data = {0},
		.inputs = (mat_node_sockets){
			.sockets = body_sockets,
			.count = 1,
			.cap = 1,
		}
	};

	mat_node_socket hair_sockets[] = {
		(mat_node_socket) { .type = NV_COLOUR, .data.v3 = {0, 1, 0.223717}, .link = NULL },
	};
	mat_node hair = {
		.type = NODE_DIFFUSE,
		.out_type = NV_BSDF,
		.out_data = {0},
		.inputs = (mat_node_sockets){
			.sockets = hair_sockets,
			.count = 1,
			.cap = 1,
		}
	};

	mat_node_socket eye_mouth_sockets[] = {
		(mat_node_socket) { .type = NV_COLOUR, .data.v3 = {1,1,1}, .link = NULL },
	};
	mat_node eye_mouth = {
		.type = NODE_DIFFUSE,
		.out_type = NV_BSDF,
		.out_data = {0},
		.inputs = (mat_node_sockets){
			.sockets = eye_mouth_sockets,
			.count = 1,
			.cap = 1,
		}
	};

	mat_node_socket face_sockets[] = {
		(mat_node_socket) { .type = NV_COLOUR, .data.v3 = {1.000000, 0.822792, 0.693886}, .link = NULL },
	};
	mat_node face = {
		.type = NODE_DIFFUSE,
		.out_type = NV_BSDF,
		.out_data = {0},
		.inputs = (mat_node_sockets){
			.sockets = face_sockets,
			.count = 1,
			.cap = 1,
		}
	};

	mat_node_socket floor_sockets[] = {
		(mat_node_socket) { .type = NV_COLOUR, .data.v3 = {0.056085, 0.057917, 0.072421}, .link = NULL },
	};
	mat_node floor = {
		.type = NODE_DIFFUSE,
		.out_type = NV_BSDF,
		.out_data = {0},
		.inputs = (mat_node_sockets){
			.sockets = floor_sockets,
			.count = 1,
			.cap = 1,
		}
	};

	mat_lib m_lib = {0};
	// int i = mat_create(&m_lib, )

	sc.mat_lib = &m_lib;

    camera cam = camera_make(
        (v3)CAMERA_POS, (v3)CAMERA_LOOKAT, (v3){0,0,1},
        CAMERA_FOV, (f64)WIDTH/HEIGHT, 0.0, 1.0
    );

   	scene_load_obj(&sc, "assets/models/Hikari_with_bg.obj");

    int i = 0;

    i = mat_get(sc.mat_lib, "Halo");
    sc.mat_lib->materials[i].root_socket.link = &halo;
    i = mat_get(sc.mat_lib, "CH0242_Body");
    sc.mat_lib->materials[i].root_socket.link = &body;
    i = mat_get(sc.mat_lib, "CH0242_Hair_2");
    sc.mat_lib->materials[i].root_socket.link = &hair;
    i = mat_get(sc.mat_lib, "CH0242_EyeMouth");
    sc.mat_lib->materials[i].root_socket.link = &eye_mouth;
    i = mat_get(sc.mat_lib, "CH0242_Face");
    sc.mat_lib->materials[i].root_socket.link = &face;
    i = mat_get(sc.mat_lib, "Floor");
    sc.mat_lib->materials[i].root_socket.link = &floor;
    i = mat_get(sc.mat_lib, "Light");
    sc.mat_lib->materials[i].root_socket.link = &light;

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
