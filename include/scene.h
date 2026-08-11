#ifndef _SCENE_H
#define _SCENE_H

#include "types.h"

void scene_load_obj(scene* scene, char* filepath);
void scene_object_push(scene* s, object o);
void scene_free(scene* sc);

#endif
