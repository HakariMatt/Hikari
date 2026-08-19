#ifndef _GPU_TYPES_H
#define _GPU_TYPES_H

#ifdef __METAL_VERSION__
    typedef packed_float3 gpu_v3;
#else
    typedef struct { float x, y, z; } gpu_v3;
#endif

typedef enum {
	GPU_NODE_CONST_COLOUR,
	GPU_NODE_CONST_FLOAT,

	GPU_NODE_DIFFUSE,
	GPU_NODE_EMISSION,
} gpu_mat_node_type;

typedef enum {
	GPU_NV_COLOUR,
	GPU_NV_FLOAT,
	GPU_NV_BSDF,
} gpu_mat_node_value_type;

typedef union {
	float value;
	gpu_v3 v3;
} gpu_mat_node_value_data;

typedef struct {
	gpu_v3 dir;
	gpu_v3 attenuation;
	gpu_v3 emission;
	int scattered;
} gpu_bsdf_result;

typedef struct {
	gpu_mat_node_value_type type;
	gpu_mat_node_value_data data;
	int link;
} gpu_mat_node_socket;

typedef struct {
	gpu_mat_node_type type;
	gpu_mat_node_value_type out_type;
	gpu_mat_node_value_data out_data;
	int input_start;
	int input_count;
} gpu_mat_node;

typedef struct {
	int root_socket;
} gpu_mat;

typedef struct {
    gpu_v3 origin;
    gpu_v3 lower_left;
    gpu_v3 horizontal;
    gpu_v3 vertical;
} gpu_camera;

typedef struct {
  const unsigned int width, height;
  gpu_camera cam;
} gpu_args;

typedef struct {
    unsigned int mat_id;
    unsigned int n0, n1, n2;
    gpu_v3 true_normal;
} gpu_tri_attrs;

#endif
