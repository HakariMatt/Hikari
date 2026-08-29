#include <stddef.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include "include/Hikari.h"

int main(void) {
	HikariSettings settings = hikari_default_settings();
	settings.samples = 256;
	HikariContext* ctx = hikari_create(settings, "assets/luts/lut.bin");
	if (!ctx) return 1;

	if (hikari_load_obj(ctx, "assets/models/Hikari_with_bg.obj") != 0) {
		fprintf(stderr, "Failed to load obj\n");
		hikari_destroy(ctx);
		return 1;
	}

	node_t white_matte = hikari_node_diffuse(ctx, (HikariColour){1,1,1});
	hikari_matrial_output_connect(ctx, "CH0242_Body", white_matte);
	hikari_matrial_output_connect(ctx, "CH0242_Hair_2", white_matte);
	hikari_matrial_output_connect(ctx, "CH0242_EyeMouth", white_matte);
	hikari_matrial_output_connect(ctx, "CH0242_Face", white_matte);
	hikari_matrial_output_connect(ctx, "CH0242_Eyebrow", white_matte);
	hikari_matrial_output_connect(ctx, "Floor", white_matte);
	hikari_matrial_output_connect(ctx, "Halo", hikari_node_emission(ctx, (HikariColour){0.296138, 1.0, 0.520996}, 3));
	hikari_matrial_output_connect(ctx, "Light", hikari_node_emission(ctx, (HikariColour){1, 1, 1}, 10));

	hikari_make_camera(ctx, (HikariVec3){ -0.93779, -2.75192, 0.924639}, (HikariVec3){ 0.0, 0.0, 0.547059 }, (HikariVec3){0,0,1}, 24, 0, 1);

	hikari_render_async(ctx);

	size_t prev_sample = 0;
	size_t curr_sample = hikari_get_samples_done(ctx);
	// fprintf(stderr, "Sampling... [%zu/%d]\n", hikari_get_samples_done(ctx), settings.samples);
	while(!hikari_is_done(ctx)) {
		curr_sample = hikari_get_samples_done(ctx);
		if (prev_sample != curr_sample) {
			fprintf(stderr, "Sampling... [%zu/%d]\n", curr_sample, settings.samples);
			prev_sample = curr_sample;
		}
	}
	printf("\n");

	hikari_render_join(ctx);


	size_t image_size = settings.width * settings.height * 3;
	unsigned char* outimg = malloc(image_size);
	float* img = hikari_get_framebuffer(ctx);
	for (size_t i = 0; i < image_size; ++i) {
		float p = img[i];
		if (p <= 0.0031308) p *= 12.92f;
		else p = 1.055 * powf(p, 1/2.4) - 0.055;
		outimg[i] = (unsigned char)fmax(fmin(p * 255.0, 255.0), 0.0);
	}

	char filename[128];
	sprintf(filename, "renders/hikari_%s_%dx%d_%d.ppm", hikari_get_backend_name(ctx), settings.width, settings.height, settings.samples);
	FILE* f = fopen(filename, "wb");
	if (!f) {
		fprintf(stderr, "Couldn't open file");
		goto cleanup;
	}

	fprintf(f, "P6\n%d %d\n255\n", settings.width, settings.height);
	fwrite(outimg, 1, image_size, f);
	fclose(f);
	free(outimg);

cleanup:
	hikari_destroy(ctx);
	return 0;
}
