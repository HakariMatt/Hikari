#include "../include/mat.h"
#include "../include/v3.h"

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

		default:
			break;
	}

	return (bsdf_result) { .attenuation = {0}, .dir = {0}, .emission = {0,0,0}, .scattered = 0};
}
