#ifndef _INTERSECT_H
#define _INTERSECT_H

#include "types.h"
#include "v3.h"

hit_result hit_triangle(v3 v0, v3 v1, v3 v2, ray r);

// Returns true if the ray hits (or starts inside) the AABB
int hit_bbox(ray r, boundbox b);

#endif
