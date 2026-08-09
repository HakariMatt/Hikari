#ifndef _BVH_C
#define _BVH_C

#include <stdlib.h>
#include <math.h>
#include "types.h"
// #include "v3.c"
// #include "mesh.c"
#include "intersect.c"

#define BVH_MAX_DEPTH   24
#define BVH_LEAF_TRIS   4

static boundbox bvh_compute_bbox(mesh m, sz* tri_idxs, sz count) {
	f64 max_x = -INFINITY, max_y = -INFINITY, max_z = -INFINITY;
	f64 min_x =  INFINITY, min_y =  INFINITY, min_z =  INFINITY;

	for (sz i = 0; i < count; ++i) {
		tri t = m.tris[tri_idxs[i]];
		v3 verts[3] = { m.verts[t.a], m.verts[t.b], m.verts[t.c] };
		for (int j = 0; j < 3; ++j) {
			v3 p = verts[j];
			max_x = fmax(max_x, p.x); min_x = fmin(min_x, p.x);
			max_y = fmax(max_y, p.y); min_y = fmin(min_y, p.y);
			max_z = fmax(max_z, p.z); min_z = fmin(min_z, p.z);
		}
	}
	return (boundbox){ max_x, min_x, max_y, min_y, max_z, min_z };
}

static v3 bvh_centroid(mesh m, sz tri_idx) {
	tri t = m.tris[tri_idx];
	v3 a = m.verts[t.a], b = m.verts[t.b], c = m.verts[t.c];
	return (v3){ (a.x+b.x+c.x)/3.0, (a.y+b.y+c.y)/3.0, (a.z+b.z+c.z)/3.0 };
}

// qsort comparator state — axis to sort centroids on
static mesh g_sort_mesh;
static int g_sort_axis;
static int bvh_cmp_centroid(const void* pa, const void* pb) {
	sz ia = *(const sz*)pa, ib = *(const sz*)pb;
	v3 ca = bvh_centroid(g_sort_mesh, ia);
	v3 cb = bvh_centroid(g_sort_mesh, ib);
	f64 va = g_sort_axis == 0 ? ca.x : g_sort_axis == 1 ? ca.y : ca.z;
	f64 vb = g_sort_axis == 0 ? cb.x : g_sort_axis == 1 ? cb.y : cb.z;
	return (va > vb) - (va < vb);
}

static bvh_node* bvh_make_leaf(mesh m, sz* tri_idxs, sz count) {
	bvh_node* node = malloc(sizeof(bvh_node));
	node->bbox = bvh_compute_bbox(m, tri_idxs, count);
	node->tri_idxs = malloc(count * sizeof(sz));
	for (sz i = 0; i < count; ++i) node->tri_idxs[i] = tri_idxs[i];
	node->tri_count = count;
	node->childA = NULL;
	node->childB = NULL;
	return node;
}

static bvh_node* bvh_build(mesh m, sz* tri_idxs, sz count, sz depth) {
	if (count <= BVH_LEAF_TRIS || depth >= BVH_MAX_DEPTH) {
		return bvh_make_leaf(m, tri_idxs, count);
	}

	boundbox bbox = bvh_compute_bbox(m, tri_idxs, count);

	f64 ax_len[3] = {
		fabs(bbox.max_x - bbox.min_x),
		fabs(bbox.max_y - bbox.min_y),
		fabs(bbox.max_z - bbox.min_z),
	};
	sz axis = 0;
	for (sz i = 1; i < 3; ++i) {
		if (ax_len[i] > ax_len[axis]) axis = i;
	}
	f64 split_val = axis == 0 ? (bbox.min_x + bbox.max_x) * 0.5
	              : axis == 1 ? (bbox.min_y + bbox.max_y) * 0.5
	              :             (bbox.min_z + bbox.max_z) * 0.5;

	sz* left  = malloc(count * sizeof(sz));
	sz* right = malloc(count * sizeof(sz));
	sz nleft = 0, nright = 0;

	for (sz i = 0; i < count; ++i) {
		v3 c = bvh_centroid(m, tri_idxs[i]);
		f64 v = axis == 0 ? c.x : axis == 1 ? c.y : c.z;
		if (v < split_val) left[nleft++]  = tri_idxs[i];
		else               right[nright++] = tri_idxs[i];
	}

	// degenerate case: everything landed on one side — fall back to a median split
	if (nleft == 0 || nright == 0) {
		for (sz i = 0; i < count; ++i) left[i] = tri_idxs[i]; // reuse as scratch
		g_sort_mesh = m;
		g_sort_axis = (int)axis;
		qsort(left, count, sizeof(sz), bvh_cmp_centroid);

		nleft = count / 2;
		nright = count - nleft;
		for (sz i = 0; i < nright; ++i) right[i] = left[nleft + i]; // left[0..nleft) already sorted in place
	}

	bvh_node* node = malloc(sizeof(bvh_node));
	node->bbox = bbox;
	node->tri_idxs = NULL;
	node->tri_count = 0;
	node->childA = bvh_build(m, left,  nleft,  depth + 1);
	node->childB = bvh_build(m, right, nright, depth + 1);

	free(left);
	free(right);
	return node;
}

static bvh_node* bvh_build_root(mesh m) {
	sz* all = malloc(m.ntris * sizeof(sz));
	for (sz i = 0; i < m.ntris; ++i) all[i] = i;
	bvh_node* root = bvh_build(m, all, m.ntris, 0);
	free(all);
	return root;
}

static void bvh_free(bvh_node* node) {
	if (!node) return;
	if (node->tri_idxs) free(node->tri_idxs);
	bvh_free(node->childA);
	bvh_free(node->childB);
	free(node);
}

// Traversal: replaces the brute-force triangle loop in render.c's ray_color.
// closest is the current best t (pass INFINITY at the top level call).
static hit_result bvh_hit(bvh_node* node, mesh m, ray r, f64 closest) {
	hit_result miss = { .hit = 0 };
	if (!node || !hit_bbox(r, node->bbox)) return miss;

	if (!node->childA && !node->childB) { // leaf
		hit_result best = miss;
		for (sz i = 0; i < node->tri_count; ++i) {
			tri t = m.tris[node->tri_idxs[i]];
			hit_result hr = hit_triangle(m.verts[t.a], m.verts[t.b], m.verts[t.c], r);
			if (hr.hit && hr.t < closest) {
				closest = hr.t;
				best = hr;
			}
		}
		return best;
	}

	hit_result hitA = bvh_hit(node->childA, m, r, closest);
	if (hitA.hit) closest = hitA.t;
	hit_result hitB = bvh_hit(node->childB, m, r, closest);

	if (hitB.hit) return hitB;      // hitB.t is already < closest (which includes hitA.t)
	return hitA;
}

#endif
