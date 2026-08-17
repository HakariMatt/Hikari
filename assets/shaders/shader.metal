#include <metal_stdlib>
#include <metal_raytracing>
#include "../../include/gpu_types.h"
using namespace metal;
using namespace metal::raytracing;

ray camera_get_ray(gpu_camera cam, float u, float v) {
	float3 lower_left = float3(cam.lower_left.x, cam.lower_left.y, cam.lower_left.z);
    float3 horizontal = float3(cam.horizontal.x, cam.horizontal.y, cam.horizontal.z);
    float3 vertical   = float3(cam.vertical.x, cam.vertical.y, cam.vertical.z);
    float3 origin     = float3(cam.origin.x, cam.origin.y, cam.origin.z);
	float3 dir = (lower_left + u * horizontal + v * vertical) - origin;
	return ray(origin, normalize(dir), 0.0f, INFINITY);
}

float3 sky_colour(ray r) {
	float a = 0.5 * (r.direction.z + 1.0);
    float3 white = float3(1.0, 1.0, 1.0);
    float3 blue  = float3(0.3, 0.5, 1.0);
    return mix(blue, white, a);
}

float3 ray_colour(ray r, acceleration_structure<> accel_struct) {
	intersector<triangle_data> isect;
    intersection_result<triangle_data> result = isect.intersect(r, accel_struct);

    float3 colour;
    if (result.type == intersection_type::triangle) {
        //uint mat_id = tri_attrs[result.primitive_id].mat_id;
        colour = float3(1,1,1);
    } else {
        colour = sky_colour(r);
    }

    return colour;
}

kernel void render_sample(  device float* out                      [[buffer(0)]],
                            constant gpu_args& args                [[buffer(1)]],
                            constant uint& sample_num              [[buffer(2)]],
                            acceleration_structure<> accel_struct  [[buffer(3)]],
                            device const gpu_tri_attrs* tri_attrs  [[buffer(5)]],
                            uint2 gid                              [[thread_position_in_grid]])
{
    if (gid.x >= args.width || gid.y >= args.height) return;
    uint idx = (gid.y * args.width + gid.x) * 3;

    float3 old_pixel = float3(out[idx+0], out[idx+1], out[idx+2]);

    float u = float(gid.x) / float(args.width - 1);
    float v = 1.0 - float(gid.y) / float(args.height - 1);

    ray r = camera_get_ray(args.cam, u, v);



    float3 image = old_pixel + ((ray_colour(r, accel_struct) - old_pixel) / (sample_num+1));
    out[idx + 0] = image.x;
    out[idx + 1] = image.y;
    out[idx + 2] = image.z;
}
