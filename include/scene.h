#ifndef _SCENE_H
#define _SCENE_H

#include "types.h"
#include "mat.h"

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
	object* objects;
	sz obj_count;
    sz obj_cap;
    mat_lib* mat_lib;
    RGB2Spec* spec_model;
    emission_list* emission_list;
} scene;

void scene_load_obj(scene* scene, char* filepath);
void scene_object_push(scene* s, object o);
void scene_free(scene* sc);
void build_emission_list(scene* sc);

#endif
