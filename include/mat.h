#ifndef _MAT_H
#define _MAT_H

#include "types.h"

typedef enum {
	NODE_CONST_COLOR,
	NODE_CONST_FLOAT,

	NODE_DIFFUSE
} mat_node_type;

typedef enum {
	NV_COLOR,
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

typedef struct mat_node mat_node;
typedef struct {
	mat_node_value_type type;
	mat_node_value_data data;
	mat_node* link;
} mat_node_socket;

typedef struct {
	mat_node_socket* sockets;
	sz count;
	sz cap;
} mat_node_sockets;

typedef struct mat_node {
	mat_node_type type;
	mat_node_value_type out_type;
	mat_node_value_data out_data;
	mat_node_sockets inputs;
} mat_node;

typedef struct {
	char* name;
	mat_node_socket root_socket;
} mat;

typedef struct {
	mat* materials;
	sz count;
	sz cap;
} mat_lib;


bsdf_result eval_bsdf(mat_node_socket* s, shading_ctx* ctx);

#endif
