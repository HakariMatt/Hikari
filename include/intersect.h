#ifndef _INTERSECT_H
#define _INTERSECT_H

#include "types.h"
#include "v3.h"

hit_result hit_triangle(mesh* m, sz tri_id, ray r);

// Returns true if the ray hits (or starts inside) the AABB
int hit_bbox(ray r, boundbox b);

#endif
