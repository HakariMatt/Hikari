#ifndef _CAMERA_H
#define _CAMERA_H

#include "types.h"

camera camera_make(v3 lookfrom, v3 lookat, v3 vup, f64 vfov_deg, f64 aspect, f64 aperture, f64 focus_dist);
ray camera_get_ray(camera c, f64 s, f64 t);

#endif
