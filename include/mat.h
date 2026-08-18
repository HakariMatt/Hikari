#ifndef _MAT_H
#define _MAT_H

#include "types.h"
#include "colour.h"

typedef enum {
	NODE_CONST_COLOUR,
	NODE_CONST_FLOAT,

	NODE_DIFFUSE,
	NODE_EMISSION,
} mat_node_type;

typedef enum {
	NV_COLOUR,
	NV_FLOAT,
	NV_BSDF,
} mat_node_value_type;

typedef union {
	f64 value;
	v3 v3;
} mat_node_value_data;

typedef struct {
	v3 point;
	ray r;
	v3 normal;
	v3 true_normal;
	u32* rng_state;
} shading_ctx;

typedef struct {
	v3 dir;
	v3 attenuation;
	v3 emission;
	int scattered;
} bsdf_result;

typedef struct {
	mat_node_value_type type;
	mat_node_value_data data;
	int link;
} mat_node_socket;

// typedef struct {
// 	mat_node_socket* sockets;
// 	sz count;
// 	sz cap;
// } mat_node_sockets;

typedef struct mat_node {
	mat_node_type type;
	mat_node_value_type out_type;
	mat_node_value_data out_data;
	int input_start;
	int input_count;
} mat_node;

typedef struct {
	char* name;
	int root_socket;
} mat;

typedef struct {
	mat* materials;
	sz mat_count;
	sz mat_cap;

	mat_node* nodes;
	sz node_count;
	sz node_cap;

	mat_node_socket* sockets;
	sz socket_count;
	sz socket_cap;
} mat_lib;


mat_node_value_data eval_value(mat_lib* lib, i32 socket_idx, shading_ctx* ctx);
bsdf_result eval_bsdf(mat_lib* lib, i32 node_idx, shading_ctx* ctx);
int mat_get(mat_lib* lib, const char* name);
int mat_create(mat_lib* lib, char* name);
i32 mat_node_diffuse(mat_lib* lib, v3 colour);
i32 mat_node_emission(mat_lib* lib, v3 colour, f64 strength);

// mat_node* diffuse_bsdf(colour c);

#endif
