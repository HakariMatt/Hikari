#ifndef _MESH_H
#define _MESH_H

#include "types.h"

object mesh_load_obj(const char* path);
void mesh_free(mesh m);

#endif
