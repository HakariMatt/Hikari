#include <metal_stdlib>
#include "../../include/gpu_types.h"
using namespace metal;

typedef struct {
	float3 origin;
	float3 dir;
} ray;

ray camera_get_ray(gpu_camera cam, float u, float v) {
	float3 lower_left = float3(cam.lower_left.x, cam.lower_left.y, cam.lower_left.z);
    float3 horizontal = float3(cam.horizontal.x, cam.horizontal.y, cam.horizontal.z);
    float3 vertical   = float3(cam.vertical.x, cam.vertical.y, cam.vertical.z);
    float3 origin     = float3(cam.origin.x, cam.origin.y, cam.origin.z);
	float3 dir = (lower_left + u * horizontal + v * vertical) - origin;
	return (ray){origin, normalize(dir)};
}

float3 sky_color(ray r) {
	float a = 0.5 * (r.dir.z + 1.0);
    float3 white = float3(1.0, 1.0, 1.0);
    float3 blue  = float3(0.3, 0.5, 1.0);
    return mix(blue, white, a);
}


float3 ray_color(ray r, uint depth) {
	return sky_color(r);
}


kernel void render_sample( device float* out           [[buffer(0)]],
                           constant uint& sample_num   [[buffer(1)]],
                           constant uint& width        [[buffer(2)]],
                           constant uint& height       [[buffer(3)]],
                           constant gpu_camera& cam    [[buffer(4)]],
                           uint2 gid                   [[thread_position_in_grid]])
{
    if (gid.x >= width || gid.y >= height) return;
    uint idx = (gid.y * width + gid.x) * 3;

    float3 old_pixel = float3(out[idx+0], out[idx+1], out[idx+2]);

    float u = float(gid.x) / float(width - 1);
    float v = 1.0 - float(gid.y) / float(height - 1);

    ray r = camera_get_ray(cam, u, v);

    float3 sample = ray_color(r, 1);
    float3 color = old_pixel + ((sample - old_pixel) / (sample_num+1));
    // colour_add(old_px, colour_divide(colour_sub(total_light, old_px), (colour){s+1,s+1,s+1}));

    out[idx + 0] = color.x;
    out[idx + 1] = color.y;
    out[idx + 2] = color.z;
}
