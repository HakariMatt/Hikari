#ifndef _BVH_H
#define _BVH_H

#include "types.h"

bvh_node* bvh_build_root(mesh m, u32 max_depth, u32 leaf_tris);
void bvh_free(bvh_node* node);
hit_result bvh_hit(bvh_node* node, mesh m, ray r, f64 closest);

#endif
