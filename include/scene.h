#ifndef _SCENE_H
#define _SCENE_H

#include "types.h"
#include "mat.h"
#include "camera.h"

typedef struct {
	sz obj_id;
	sz tri_id;
	f64 area;
	f64 cum_area;
} emission_tris;

typedef struct {
	emission_tris* tris;
	sz count;
	sz cap;
	f64 total_area;
} emission_list;

typedef struct {
	camera camera;
	object* objects;
	sz obj_count;
    sz obj_cap;
    mat_lib* mat_lib;
    RGB2Spec* spec_model;
    emission_list* emission_list;
} scene;

void scene_make_camera(scene* sc, v3 pos, v3 lookat, v3 vup, f64 vfov_deg, f64 aspect, f64 aperture, f64 focus_dist);
int scene_load_obj(scene* scene, const char* filepath);
void scene_free(scene* sc);
void build_emission_list(scene* sc);
int is_emissive(mat_lib* lib, sz mat_id);

#endif
