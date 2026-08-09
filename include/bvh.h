#ifndef _BVH_H
#define _BVH_H

#include "types.h"

bvh_node* bvh_build_root(mesh m);
void bvh_free(bvh_node* node);
hit_result bvh_hit(bvh_node* node, mesh m, ray r, f64 closest);

#endif
