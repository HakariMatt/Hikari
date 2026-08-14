#ifndef _SCENE_H
#define _SCENE_H

#include "types.h"
#include "mat.h"

typedef struct {
	object* objects;
	sz obj_count;
    sz obj_cap;
    mat_lib* mat_lib;
} scene;

void scene_load_obj(scene* scene, char* filepath);
void scene_object_push(scene* s, object o);
void scene_free(scene* sc);

#endif
