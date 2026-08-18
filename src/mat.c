#include <string.h>
#include <stdlib.h>

#include "../include/mat.h"
#include "../include/v3.h"
#include "../include/log.h"


v3 sample_cosine_hemisphere(v3 n, f64 *pdf_out, u32* rng_state) {
	f64 u1 = random_f64(rng_state);
	f64 u2 = random_f64(rng_state);
    f64 r   = sqrt(u1);
    f64 phi = 2.0 * M_PI * u2;
    f64 x = r * cos(phi);
    f64 y = r * sin(phi);
    f64 z = sqrt(fmax(0.0, 1.0 - x*x - y*y));

    *pdf_out = z / M_PI;

    v3 t, b;
    build_onb(n, &t, &b);

    v3 dir = v3_add(v3_add(v3_scale(t, x), v3_scale(b, y)), v3_scale(n, z));
    return dir;
}

mat_node_value_data eval_value(mat_lib* lib, i32 socket_idx, shading_ctx* ctx) {
	mat_node_socket* s = &lib->sockets[socket_idx];
	if (s->link == -1) return s->data;

	// for now
	return lib->nodes[s->link].out_data;
	// later with texturing and procedural noise will need switch case on node type
	// and do all the ctx shenanigans
}

bsdf_result eval_bsdf(mat_lib* lib, i32 node_idx, shading_ctx* ctx) {
	if (node_idx == -1) return (bsdf_result){0};
	mat_node* n = &lib->nodes[node_idx];

	switch (n->type) {
		case NODE_DIFFUSE: {
			v3 colour = eval_value(lib, n->input_start, ctx).v3;
			f64 pdf = 0;
			v3 dir = sample_cosine_hemisphere(ctx->normal, &pdf, ctx->rng_state);
			if (v3_dot(dir, ctx->true_normal) <= 0) dir = ctx->normal;
			return (bsdf_result){ .attenuation = colour, .dir = dir, .scattered = 1 };
		}
		case NODE_EMISSION: {
			v3 colour    = eval_value(lib, n->input_start + 0, ctx).v3;
			f64 strength = eval_value(lib, n->input_start + 1, ctx).value;
			return (bsdf_result){ .emission = v3_scale(colour, strength), .scattered = 0 };
		}
		default:
			return (bsdf_result){0};
	}
}

int mat_lib_push_material(mat_lib* lib, mat m) {
    if (lib->mat_count >= lib->mat_cap) {
    size_t new_cap = (lib->mat_cap == 0) ? 32 : lib->mat_cap * 2;
        mat* new_materials = realloc(lib->materials, new_cap * sizeof(mat));
        if (!new_materials) return -1;

        lib->materials = new_materials;
        lib->mat_cap = new_cap;
    }

    lib->materials[lib->mat_count] = m;
    lib->mat_count++;
    return 0;
}

int mat_get(mat_lib* lib, const char* name) {

	for (int i = 0; i < lib->mat_count; ++i) {
		if (strcmp(name, lib->materials[i].name) == 0) return i;
	}
	return -1;
}

int mat_create(mat_lib* lib, char* name) {
	int id = mat_get(lib, name);
	if (id != -1) {
		print(WARNING, "Material `%s` already exists.", name);
		return id;
	}

	mat m = {
		.name = strdup(name),
		.root_socket = -1
	};

	if (mat_lib_push_material(lib, m) == -1) {
		print(ERROR, "Failed to add material `%s` to library.", name);
		return -1;
	}
	print(INFO, "Material `%s` created.", name);
	return lib->mat_count-1;
}

static i32 mat_lib_push_socket(mat_lib* lib, mat_node_socket s) {
	if (lib->socket_count >= lib->socket_cap) {
		lib->socket_cap = lib->socket_cap ? lib->socket_cap * 2 : 64;
		lib->sockets = realloc(lib->sockets, lib->socket_cap * sizeof(mat_node_socket));
	}
	lib->sockets[lib->socket_count] = s;
	return (i32)(lib->socket_count++);
}

static i32 mat_lib_push_node(mat_lib* lib, mat_node n) {
	if (lib->node_count >= lib->node_cap) {
		lib->node_cap = lib->node_cap ? lib->node_cap * 2 : 64;
		lib->nodes = realloc(lib->nodes, lib->node_cap * sizeof(mat_node));
	}
	lib->nodes[lib->node_count] = n;
	return (i32)(lib->node_count++);
}

i32 mat_node_diffuse(mat_lib* lib, v3 colour) {
	i32 start = mat_lib_push_socket(lib, (mat_node_socket){ .type = NV_COLOUR, .data.v3 = colour, .link = -1 });
	return mat_lib_push_node(lib, (mat_node){
		.type = NODE_DIFFUSE, .out_type = NV_BSDF,
		.input_start = start, .input_count = 1
	});
}

i32 mat_node_emission(mat_lib* lib, v3 colour, f64 strength) {
	i32 start = mat_lib_push_socket(lib, (mat_node_socket){ .type = NV_COLOUR, .data.v3 = colour, .link = -1 });
	mat_lib_push_socket(lib, (mat_node_socket){ .type = NV_FLOAT, .data.value = strength, .link = -1 });
	return mat_lib_push_node(lib, (mat_node){
		.type = NODE_EMISSION, .out_type = NV_BSDF,
		.input_start = start, .input_count = 2
	});
}

// mat_node* diffuse_bsdf(colour c) {
// 	mat_node* m = malloc(sizeof(mat_node));
// 	mat_node_socket* s = malloc(sizeof(mat_node_socket));
// 	if (!m) return NULL;

// 	s->type = NV_COLOUR;
// 	s->data.v3 = colour_to_v3(c);
// 	s->link = NULL;
// 	m->;
// }
