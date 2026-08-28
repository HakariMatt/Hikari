#include <pthread.h>
#include <stdlib.h>

#include "../include/Hikari.h"
#include "../include/hikari_internal.h"
#include "../include/render_backend.h"
#include "../include/log.h"


v3 hikari_colour_to_v3(HikariColour colour) {
	return (v3) {colour.r, colour.g, colour.b};
}

v3 hikari_vec3_to_v3(HikariVec3 v) {
	return (v3) {v.x, v.y, v.z};
}

HikariSettings hikari_default_settings() {
	return (HikariSettings) {
		.width = 256, .height = 256,
		.samples = 4, .max_bounces = 8, .min_rr_depth = 4,
		.lambda_min = 380, .lambda_max = 780,
		.bvh_leaf_tris = 4, .bvh_max_depth = 24
	};
}

HikariContext* hikari_create(const HikariSettings settings, const char* spectral_lut_path) {
	if (!spectral_lut_path) return NULL;

	RGB2Spec* spec_model = rgb2spec_load(spectral_lut_path);
	if (!spec_model) return NULL;

	HikariContext* ctx = malloc(sizeof(HikariContext));
	if (!ctx) {
		rgb2spec_free(spec_model);
		return NULL;
	}

	float* image = malloc(settings.width * settings.height * 3 * sizeof(float));
	if (!image) {
		rgb2spec_free(spec_model);
		free(ctx);
		return NULL;
	}

	const render_backend* backend = &cpu_backend_ops;
	#ifdef HIKARI_METAL
		backend = &metal_backend_ops;
	#endif

	*ctx = (HikariContext){
		.settings = settings,
		.image = image,
		.spec_model = spec_model,
		.backend = backend,
		.scene = (scene){0},
		.mat_lib = (mat_lib){0},
		.emission_list = (emission_list){0},
		.render_state = (render_state){0},
		.render_args = (render_args){0},
		.has_camera = 0
	};

	ctx->scene.mat_lib = &ctx->mat_lib;
	ctx->scene.spec_model = ctx->spec_model;
	ctx->scene.emission_list = &ctx->emission_list;

	ctx->render_args.img = ctx->image;
	ctx->render_args.settings = ctx->settings;
	ctx->render_args.state = &ctx->render_state;

	return ctx;
}

void hikari_destroy(HikariContext* ctx) {
	if (!ctx) return;
	if (ctx->render_args.ctx) ctx->backend->shutdown(&ctx->render_args);
	scene_free(&ctx->scene);
	rgb2spec_free(ctx->spec_model);
	free(ctx->image);
	free(ctx);
}

int hikari_load_obj(HikariContext* ctx, const char* filename) {
	if (!ctx || !filename) return -1;
	scene_load_obj(&ctx->scene, filename);
	return 0;
}

void hikari_make_camera(HikariContext* ctx,
						HikariVec3     pos,
						HikariVec3     lookat,
						HikariVec3     vup,
						double         fov_deg,
						double         aperture,
						double         focus_distance
) {
	if (!ctx) return;

	ctx->scene.camera = camera_make(
		hikari_vec3_to_v3(pos),
		hikari_vec3_to_v3(lookat),
		hikari_vec3_to_v3(vup),
		fov_deg, (double)ctx->settings.width / ctx->settings.height,
		aperture, focus_distance);
	ctx->has_camera = 1;
}

node_t hikari_node_diffuse(HikariContext* ctx, HikariColour colour) {
	if (!ctx) return -1;

	return mat_node_diffuse(&ctx->mat_lib, hikari_colour_to_v3(colour));
}

node_t hikari_node_emission(HikariContext* ctx, HikariColour colour, float strength) {
	if (!ctx) return -1;

	return mat_node_emission(&ctx->mat_lib, hikari_colour_to_v3(colour), (double)strength);
}

void hikari_matrial_output_connect(HikariContext* ctx, const char* material_name, node_t node_id) {
	if (!ctx || !material_name) return;

	mat_node_connect_output(&ctx->mat_lib, material_name, node_id);
}

int hikari_render(HikariContext* ctx) {
	if (!ctx) return -1;

	if (!ctx->has_camera) {
		print(ERROR, "Scene does not have camera");
		return -1;
	}

	build_emission_list(&ctx->scene);

	ctx->render_args.scene = ctx->scene;

	if (!ctx->backend_initialized) {
		if(ctx->backend->init(&ctx->render_args) != 0) return -1;
		ctx->backend_initialized = 1;
	}

	ctx->backend->render(&ctx->render_args);
	ctx->render_state.done = 1;

	return 0;
}

static void* hikari_render_thread_fn(void* arg) {
	hikari_render((HikariContext*)arg);
	return NULL;
}

int hikari_render_async(HikariContext* ctx) {
	if (!ctx) return -1;

	if (pthread_create(&ctx->render_thread, NULL, hikari_render_thread_fn, ctx) != 0) return -1;
	ctx->has_thread = 1;

	return 0;
}

int hikari_render_join(HikariContext* ctx) {
	if (!ctx) return -1;
	if (!ctx->has_thread) return -1;

	pthread_join(ctx->render_thread, NULL);
	ctx->has_thread = 0;
	return 0;
}

void hikari_render_cancel(HikariContext* ctx) {
	if (ctx) ctx->render_state.should_stop = 1;
}

char* hikari_get_backend_name(HikariContext* ctx) {
	if (ctx)
		return ctx->backend->name;
	return NULL;
}

const float* hikari_get_framebuffer(HikariContext* ctx) {
	if (!ctx) return NULL;

	return ctx->render_args.img;
}
size_t hikari_get_samples_done(HikariContext* ctx) {
	if (!ctx) return 0;

	return ctx->render_state.samples_done;
}
int hikari_is_done(HikariContext* ctx) {
	if (!ctx) return 0;

	return ctx->render_state.done;
}
