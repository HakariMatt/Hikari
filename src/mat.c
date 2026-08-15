#include <string.h>
#include <stdlib.h>

#include "../include/mat.h"
#include "../include/v3.h"
#include "../include/log.h"

// example of a value node (Color Node)
// mat_node color_node = {
//     .type = NODE_CONST_COLOR,
//     .out_type = NV_COLOR,
//     .out_data = (v3){1.0, 0.5, 0.0},
//     .sockets = (mat_node_sockets){0}
// }
//
// example of a Diffuse BSDF node
// mat_node diffuse_bsdf = {
//     .type = NODE_DIFFUSE,
//     .out_type = NV_BSDF,
//     .out_data = {0},
//     .sockets = (mat_node_sockets){
//         .sockets = {
//             (mat_node_socket) { .type = NV_COLOR, .data = (v3){1,1,1}, .link = &color_node },
//             (mat_node_socket) { .type = NV_FLOAT, .data = 0.5, .link = NULL },
//         },
//         .count = 2,
//         .cap = 2,
//     }
// }
//
// this is basically
//
// [     RGB     ]      [  Diffuse BSDF  ]
// | 1.0 0.5 0.0 | -->--o Color     BSDF o
//                      |                |
//                      o Roughness: 0.5 |


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

mat_node_value_data eval_value(mat_node_socket* s, shading_ctx* ctx) {
	if (!s->link) return s->data;

	// for now
	return s->link->out_data;
	// later with texturing and procedural noise will need switch case on node type
	// and do all the ctx shenanigans
}

bsdf_result eval_bsdf(mat_node_socket* s, shading_ctx* ctx) {
	if (!s->link) return (bsdf_result){0};

	mat_node* n = s->link;
	switch (n->type) {
		case NODE_DIFFUSE:
			if (n->out_type != NV_BSDF) break;
			if (n->inputs.count != 1) break;

			v3 colour = eval_value(&n->inputs.sockets[0], ctx).v3;
			v3 dir = {0};
			f64 pdf = 0;

			dir = sample_cosine_hemisphere(ctx->normal, &pdf, ctx->rng_state);
			if (v3_dot(dir, ctx->true_normal) <= 0) dir = ctx->normal;

			return (bsdf_result) { .attenuation = colour, .dir = dir, .emission = {0}, .scattered = 1};

		case NODE_EMISSION:
			if (n->out_type != NV_BSDF) break;
			if (n->inputs.count != 2) break;
			v3 colour1 = eval_value(&n->inputs.sockets[0], ctx).v3;
			f64 strength = eval_value(&n->inputs.sockets[1], ctx).value;

			return (bsdf_result) { .attenuation = {0}, .dir = {0}, .emission = v3_scale(colour1, strength), .scattered = 0};

		default:
			break;
	}

	return (bsdf_result) { .attenuation = {0}, .dir = {0}, .emission = {0,0,0}, .scattered = 0};
}

int mat_lib_push_material(mat_lib* lib, mat m) {
    if (lib->count >= lib->cap) {
        size_t new_cap = (lib->cap == 0) ? 32 : lib->cap * 2;
        mat* new_materials = realloc(lib->materials, new_cap * sizeof(mat));
        if (!new_materials) return -1;

        lib->materials = new_materials;
        lib->cap = new_cap;
    }

    lib->materials[lib->count] = m;
    lib->count++;
    return 0;
}

int mat_get(mat_lib* lib, const char* name) {

	for (int i = 0; i < lib->count; ++i) {
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
		.root_socket = (mat_node_socket) { .type = NV_BSDF, .link = NULL }
	};

	if (mat_lib_push_material(lib, m) == -1) {
		print(ERROR, "Failed to add material `%s` to library. Library is now ruined :)", name);
		return -1;
	}
	print(INFO, "Material `%s` created.", name);
	return lib->count-1;
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
