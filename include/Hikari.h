#ifndef HIKARI_H
#define HIKARI_H

#include <stddef.h>

typedef struct HikariContext HikariContext;
typedef struct {
	// image
	unsigned int width, height;

	// sampling
	unsigned int samples;
	unsigned int max_bounces;
	unsigned int min_rr_depth;

	// spectral
    double lambda_min, lambda_max;

    // BVH
    unsigned int bvh_max_depth;
    unsigned int bvh_leaf_tris;
} HikariSettings;
typedef int node_t;
typedef struct { float r, g, b; } HikariColour;
typedef struct { double x, y, z; } HikariVec3;

HikariSettings hikari_default_settings();

HikariContext* hikari_create(const HikariSettings settings, const char* spectral_lut_path);
void           hikari_destroy(HikariContext* ctx);

// scene
int            hikari_load_obj(HikariContext* ctx, const char* filename);
void           hikari_make_camera(HikariContext* ctx, HikariVec3 pos, HikariVec3 lookat, HikariVec3 vup, double fov_deg, double aperture, double focus_distance);

// materials
node_t         hikari_node_diffuse(HikariContext* ctx, HikariColour colour);
node_t         hikari_node_emission(HikariContext* ctx, HikariColour colour, float strength);
void           hikari_matrial_output_connect(HikariContext* ctx, const char* material_name, node_t node_id);

int            hikari_render(HikariContext* ctx);
int            hikari_render_async(HikariContext* ctx);
int            hikari_render_join(HikariContext* ctx);
void           hikari_render_cancel(HikariContext* ctx);

char*          hikari_get_backend_name(HikariContext* ctx);

const float*   hikari_get_framebuffer(HikariContext* ctx);
size_t         hikari_get_samples_done(HikariContext* ctx);
int            hikari_is_done(HikariContext* ctx);

#endif
