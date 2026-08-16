#include <stdio.h>
#include <stdlib.h>

#include "../include/log.h"
#include "../include/metal_backend.h"
#include "../include/types.h"
#include "../include/settings.h"
#include "../include/camera.h"
#include "../include/scene.h"
#include "../include/render_backend.h"

#define IMG_SIZE WIDTH*HEIGHT*3

int main(void) {
    metal_ctx* ctx = metal_init();
    if (!ctx) { fprintf(stderr, "init failed\n"); return 1; }

    f32* img = malloc(IMG_SIZE*sizeof(f32));

    // scene setup
    scene sc = {0};


    camera cam = camera_make(
        (v3)CAMERA_POS, (v3)CAMERA_LOOKAT, (v3){0,0,1},
        CAMERA_FOV, (f64)WIDTH/HEIGHT, 0.0, 1.0
    );



    render_state state = {0};
    render_args args = {
    	.img = img,
        .cam = cam,
        .width = WIDTH,
        .height = HEIGHT,
        .scene = sc,
        .state = &state,
    };


	for (size_t i = 0; i < N_SAMPLES; ++i) {
		printf("\rsample %zu... ", i);
		fflush(stdout);
		metal_render_sample(ctx, args, i);
		args.state->samples_done = i;
	}
	printf("\n");

    u8* out = malloc(IMG_SIZE);
    for (sz i = 0; i < (sz)(IMG_SIZE); ++i) out[i] = (u8)(img[i] * 255.0f);

    FILE* f = fopen("output.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", WIDTH, HEIGHT);
    fwrite(out, 1, IMG_SIZE, f);
    fclose(f);

    metal_shutdown(ctx);
    free(img); free(out);
    printf("done\n");
    return 0;
}
