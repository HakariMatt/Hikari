#include <metal_stdlib>
#include <metal_raytracing>
#include <metal_math>
#include "../../include/gpu_types.h"
#include "../../include/settings.h"
using namespace metal;
using namespace metal::raytracing;

typedef struct {
	float3 point;
	ray r;
	float3 normal;
	float3 true_normal;
	thread uint* rng_state;
} shading_ctx;

typedef struct {
	device const gpu_mat* materials;
	device const gpu_mat_node* nodes;
	device const gpu_mat_node_socket* sockets;
} mat_lib;

ray camera_get_ray(gpu_camera cam, float u, float v) {
	float3 lower_left = float3(cam.lower_left.x, cam.lower_left.y, cam.lower_left.z);
    float3 horizontal = float3(cam.horizontal.x, cam.horizontal.y, cam.horizontal.z);
    float3 vertical   = float3(cam.vertical.x, cam.vertical.y, cam.vertical.z);
    float3 origin     = float3(cam.origin.x, cam.origin.y, cam.origin.z);
	float3 dir = (lower_left + u * horizontal + v * vertical) - origin;
	return ray(origin, normalize(dir), 0.0f, INFINITY);
}

// from https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
uint wellons3(uint x)
{
    x ^= x >> 17;
    x *= 0xed5ad4bbU;
    x ^= x >> 11;
    x *= 0xac4c1b51U;
    x ^= x >> 15;
    x *= 0x31848babU;
    x ^= x >> 14;
    return x;
}

uint hash3(uint x, uint y, uint z)
{
    uint h = 0x9e3779b9u;

    h ^= wellons3(x + 0x9e3779b9u);
    h = wellons3(h);

    h ^= wellons3(y + 0x9e3779b9u);
    h = wellons3(h);

    h ^= wellons3(z + 0x9e3779b9u);
    h = wellons3(h);

    return h;
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

float3 random_point_in_circle(thread uint* rng_state)
{
	uint angle = random_f32(rng_state) * 2 * 3.14156f;
	float3 point_on_circle = float3(fast::cos(angle), fast::sin(angle), 0);
	return point_on_circle * fast::sqrt(random_f32(rng_state));
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

float3 sky_colour(ray r) {
	return float3(0);
	float a = 0.5 * (r.direction.z + 1.0);
    float3 white = float3(1.0, 1.0, 1.0);
    float3 blue  = float3(0.3, 0.5, 1.0);
    return mix(blue, white, a);
}

void build_onb(float3 n, thread float3 *tangent, thread float3 *bitangent) {
    float s = n.z >= 0.0 ? 1.0 : -1.0;
    float a = -1.0 / (s + n.z);
    float b = n.x * n.y * a;
    *tangent   = float3(1.0 + s * n.x * n.x * a, s * b, -s * n.x);
    *bitangent = float3(b, s + n.y * n.y * a, -n.y);
}

float3 sample_cosine_hemisphere(float3 n, thread float *pdf_out, thread uint* rng_state) {
	float u1 = random_f32(rng_state);
	float u2 = random_f32(rng_state);
    float r   = sqrt(u1);
    float phi = 2.0 * M_PI_F * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);
    float z = sqrt(fmax(0.0, 1.0 - x*x - y*y));

    *pdf_out = z / M_PI_F;

    float3 t, b;
    build_onb(n, &t, &b);

    return t * x + b * y + n * z;
}


gpu_mat_node_value_data eval_value(mat_lib lib, int socket_idx, thread shading_ctx* ctx) {
	gpu_mat_node_socket s = lib.sockets[socket_idx];
	if (s.link == -1) return s.data;

	// for now
	return lib.nodes[s.link].out_data;
	// later with texturing and procedural noise will need switch case on node type
	// and do all the ctx shenanigans
}

gpu_bsdf_result eval_bsdf(mat_lib lib, int node_idx, thread shading_ctx* ctx) {
	if (node_idx == -1) return (gpu_bsdf_result){0};
	gpu_mat_node n = lib.nodes[node_idx];

	switch (n.type) {
		case GPU_NODE_DIFFUSE: {
			float3 colour = eval_value(lib, n.input_start, ctx).v3;
			float pdf = 1;
			float3 dir = sample_cosine_hemisphere(ctx->normal, &pdf, ctx->rng_state);

			return (gpu_bsdf_result){ .attenuation = colour, .dir = dir, .scattered = 1 };
		}
		case GPU_NODE_EMISSION: {
			float3 colour    = eval_value(lib, n.input_start + 0, ctx).v3;
			float strength = eval_value(lib, n.input_start + 1, ctx).value;
			return (gpu_bsdf_result){ .emission = colour * strength, .scattered = 0 };
		}
		default:
			return (gpu_bsdf_result){0};
	}
}


float3 ray_colour(ray r,
                  acceleration_structure<> accel_struct,
                  device const gpu_v3* normals,
                  device const gpu_tri_attrs* tri_attrs,
                  mat_lib m_lib,
                  thread uint* rng_state)
{
    float3 radiance = float3(0,0,0);
    float3 throughput = float3(1,1,1);

    intersector<triangle_data> isect;

    for (uint depth = 0; depth < MAX_BOUNCES; ++depth) {
        intersection_result<triangle_data> result = isect.intersect(r, accel_struct);

        if (result.type != intersection_type::triangle) {
            radiance += throughput * sky_colour(r);
            break;
        }

        gpu_tri_attrs attrs = tri_attrs[result.primitive_id];
        float3 normal = get_normal(result, normals, attrs);

        shading_ctx ctx = {
            .point = ray_at(r, result.distance) + (normal * 1e-4f),
            .r = r,
            .normal = normal,
            .true_normal = attrs.true_normal,
            .rng_state = rng_state
        };

        gpu_mat m = m_lib.materials[attrs.mat_id];
        gpu_bsdf_result bsdf = eval_bsdf(m_lib, m.root_socket, &ctx);

        radiance += throughput * bsdf.emission;

        if (!bsdf.scattered) break;

        throughput *= bsdf.attenuation;

        float p = max(max(throughput.x, throughput.y), throughput.z);
        if (p < random_f32(rng_state)) break;

        r = ray(ctx.point, bsdf.dir, 1e-4f, INFINITY);
    }

    return radiance;
}

kernel void render_sample(  device float* out                          [[buffer(0)]],
                            constant gpu_args& args                    [[buffer(1)]],
                            constant uint& sample_num                  [[buffer(2)]],
                            acceleration_structure<> accel_struct      [[buffer(3)]],
                            device const gpu_v3* normals               [[buffer(4)]],
                            device const gpu_tri_attrs* tri_attrs      [[buffer(5)]],
                            device const gpu_mat* materials            [[buffer(6)]],
                            device const gpu_mat_node* nodes           [[buffer(7)]],
                            device const gpu_mat_node_socket* sockets  [[buffer(8)]],
                            uint2 gid                                  [[thread_position_in_grid]])
{
    if (gid.x >= args.width || gid.y >= args.height) return;
    uint idx = (gid.y * args.width + gid.x) * 3;

    mat_lib m_lib = {
    	.materials = materials,
     	.nodes = nodes,
      	.sockets = sockets
    };

    //uint rng_state = wellons3((gid.x * 4013 + gid.y * 3307 + sample_num * 5107));
    uint rng_state = hash3(gid.x, gid.y, sample_num);

    float3 old_pixel = float3(out[idx+0], out[idx+1], out[idx+2]);

    float u = float(gid.x) / float(args.width - 1);
    float v = 1.0 - float(gid.y) / float(args.height - 1);

    float3 jitter = random_point_in_circle(&rng_state) * 1e-3;

    u += jitter.x;
    v += jitter.y;

    ray r = camera_get_ray(args.cam, u, v);
    float3 color = ray_colour(r, accel_struct, normals, tri_attrs, m_lib, &rng_state);

    //color.x = pow(color.x, 1/2.4f);
    //color.y = pow(color.y, 1/2.4f);
    //color.z = pow(color.z, 1/2.4f);

    float3 image = old_pixel + ((color - old_pixel) / (sample_num+1));
    out[idx + 0] = image.x;
    out[idx + 1] = image.y;
    out[idx + 2] = image.z;
}
