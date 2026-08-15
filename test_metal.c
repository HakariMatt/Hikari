#include <stdio.h>
#include <stdlib.h>
#include "include/metal_backend.h"

#define W 480
#define H 720

int main(void) {
    metal_ctx* ctx = metal_init();
    if (!ctx) { fprintf(stderr, "init failed\n"); return 1; }

    f32* img = malloc(W*H*3*sizeof(f32));
    camera dummy_cam = {0};
    metal_render_sample(ctx, img, W, H, dummy_cam, 0);

    u8* out = malloc(W*H*3);
    for (sz i = 0; i < (sz)(W*H*3); ++i) out[i] = (u8)(img[i] * 255.0f);

    FILE* f = fopen("stub_output.ppm", "wb");
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    fwrite(out, 1, W*H*3, f);
    fclose(f);

    metal_shutdown(ctx);
    free(img); free(out);
    printf("wrote stub_output.ppm\n");
    return 0;
}
