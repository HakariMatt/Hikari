#include <metal_stdlib>
#include <metal_raytracing>
#include <metal_math>
#include "../../include/gpu_types.h"
#include "../../include/settings.h"
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

uint hash_seed(uint2 gid, uint sample_num) {
    uint seed = gid.x * 1973u + gid.y * 9277u + sample_num * 26699u;
    return seed | 1u;
}

uint next_random(thread uint* state) {
	*state = (*state) * 747796405 + 2891336453;
	uint result = ((*state >> ((*state >> 28) + 4)) ^ *state) * 277803737;
	result = (result >> 22) ^ result;
	return result;
}

float random_f32(thread uint* state) {
	return next_random(state) * (1.0 / 4294967296.0);
}

float3 random_dir(thread uint* state) {
	return normalize(float3(
		random_f32(state) * 2 - 1,
		random_f32(state) * 2 - 1,
		random_f32(state) * 2 - 1
	));
}

float3 random_point_in_circle(thread uint* rng_state)
{
	uint angle = random_f32(rng_state) * 2 * 3.14156f;
	float3 point_on_circle = float3(fast::cos(angle), fast::sin(angle), 0);
	return point_on_circle * fast::sqrt(random_f32(rng_state));
}

float3 sky_colour(ray r) {
	float a = 0.5 * (r.direction.z + 1.0);
    float3 white = float3(1.0, 1.0, 1.0);
    float3 blue  = float3(0.3, 0.5, 1.0);
    return mix(blue, white, a);
}

float3 get_normal(	intersection_result<triangle_data> result,
					device const gpu_v3* normals,
					gpu_tri_attrs attrs)
{
	float2 bary = result.triangle_barycentric_coord;
    float b0 = 1.0 - bary.x - bary.y, b1 = bary.x, b2 = bary.y;

    float3 n0 = float3(normals[attrs.n0].x, normals[attrs.n0].y, normals[attrs.n0].z);
    float3 n1 = float3(normals[attrs.n1].x, normals[attrs.n1].y, normals[attrs.n1].z);
    float3 n2 = float3(normals[attrs.n2].x, normals[attrs.n2].y, normals[attrs.n2].z);

    return normalize(b0 * n0 + b1 * n1 + b2 * n2);
}

float3 ray_at(ray r, float t) {
	return r.origin + (r.direction * t);
}

float3 ray_colour(ray r,
                  acceleration_structure<> accel_struct,
                  device const gpu_v3* normals,
                  device const gpu_tri_attrs* tri_attrs,
                  uint depth, thread uint* rng_state)
{
	if (depth >= MAX_BOUNCES) return float3(0,0,0);

    intersector<triangle_data> isect;
    intersection_result<triangle_data> result = isect.intersect(r, accel_struct);

    if (result.type != intersection_type::triangle) {
        return sky_colour(r);
    }

    gpu_tri_attrs attrs = tri_attrs[result.primitive_id];
    float3 normal = get_normal(result, normals, attrs);
    float3 bounce_dir = normalize(random_dir(rng_state) + normal);
    if (dot(bounce_dir, attrs.true_normal) < 0) bounce_dir = normal;

    ray new_r = ray(
    	ray_at(r, result.distance) + (normal * 1e-7),
    	bounce_dir,
     	0.0f,
     	INFINITY
    );

    float3 incoming = ray_colour(new_r, accel_struct, normals, tri_attrs, depth+1, rng_state);

    return incoming * float3(0.9f, 0.9f, 0.9f);
    //return incoming;
}

kernel void render_sample(  device float* out                      [[buffer(0)]],
                            constant gpu_args& args                [[buffer(1)]],
                            constant uint& sample_num              [[buffer(2)]],
                            acceleration_structure<> accel_struct  [[buffer(3)]],
                            device const gpu_v3* normals           [[buffer(4)]],
                            device const gpu_tri_attrs* tri_attrs  [[buffer(5)]],
                            uint2 gid                              [[thread_position_in_grid]])
{
    if (gid.x >= args.width || gid.y >= args.height) return;
    uint idx = (gid.y * args.width + gid.x) * 3;

    uint rng_state = hash_seed(gid, sample_num);

    float3 old_pixel = float3(out[idx+0], out[idx+1], out[idx+2]);

    float u = float(gid.x) / float(args.width - 1);
    float v = 1.0 - float(gid.y) / float(args.height - 1);

    float3 jitter = random_point_in_circle(&rng_state) * 1e-3;

    u += jitter.x;
    v += jitter.y;

    ray r = camera_get_ray(args.cam, u, v);
    float3 color = ray_colour(r, accel_struct, normals, tri_attrs, 1, &rng_state);

    color.x = pow(color.x, 1/2.4f);
    color.y = pow(color.y, 1/2.4f);
    color.z = pow(color.z, 1/2.4f);

    float3 image = old_pixel + ((color - old_pixel) / (sample_num+1));
    out[idx + 0] = image.x;
    out[idx + 1] = image.y;
    out[idx + 2] = image.z;
}
