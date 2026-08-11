#include "../include/scene.h"
#include "../include/bvh.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static boundbox boundbox_make(v3* verts, sz vcount) {
	f64 max_x = -INFINITY, max_y = -INFINITY, max_z = -INFINITY;
	f64 min_x =  INFINITY, min_y =  INFINITY, min_z =  INFINITY;

	for (sz i = 0; i < vcount; ++i) {
	    v3 vert = verts[i];
	    max_x = fmax(max_x, vert.x);
	    max_y = fmax(max_y, vert.y);
	    max_z = fmax(max_z, vert.z);
	    min_x = fmin(min_x, vert.x);
	    min_y = fmin(min_y, vert.y);
	    min_z = fmin(min_z, vert.z);
	}

	return (boundbox){ max_x, min_x, max_y, min_y, max_z, min_z };
}

void scene_object_push(scene* s, object o) {
	if (s->obj_count >= s->obj_cap) {
		if (s->obj_cap == 0) s->obj_cap = 32;
		s->obj_cap *= 2;
		s->objects = realloc(s->objects, s->obj_cap * sizeof(object));
	}
	s->objects[s->obj_count] = o;
	s->obj_count++;
}

void scene_load_obj(scene* scene, char* filepath) {
	if (!scene) return;

	FILE* f = fopen(filepath, "r");
	if (!f) {
		fprintf(stderr, "File `%s` couldn't be opened\n", filepath);
		return;
	}

	sz vcap = 1024; sz vcount = 0;
	v3* v = malloc(vcap*sizeof(v3));
	sz vtcap = 1024; sz vtcount = 0;
	v3* vt = malloc(vtcap*sizeof(v3));
	sz vncap = 1024; sz vncount = 0;
	v3* vn = malloc(vncap*sizeof(v3));
	sz tcap = 1024; sz tcount = 0;
	tri* t = malloc(vcap*sizeof(tri));

	// sz current_line = 1;

	char buf[512];
	while (fgets(buf, sizeof(buf), f)) {
		if (buf[0] == 'v' && buf[1] == ' ') {
			// scan vertex
			v3 p = {0};
			if (sscanf(buf + 2, "%lf %lf %lf", &p.x, &p.z, &p.y) == 3) {
				p.y *= -1;
				if (vcount == vcap) { vcap *= 2; v = realloc(v, vcap * sizeof(v3)); }
				v[vcount++] = p;
			}
		}
		else if (buf[0] == 'v' && buf[1] == 't' && buf[2] == ' ') {
			// scan texture coordinate
			v3 p = {0};
			if (sscanf(buf + 3, "%lf %lf", &p.x, &p.y) == 2) {
				if (vtcount == vtcap) { vtcap *= 2; vt = realloc(vt, vtcap * sizeof(v3)); }
				vt[vtcount++] = p;
			}
		}
		else if (buf[0] == 'v' && buf[1] == 'n' && buf[2] == ' ') {
			// scan vertex normal
			v3 p = {0};
			if (sscanf(buf + 3, "%lf %lf %lf", &p.x, &p.z, &p.y) == 3) {
				p.y *= -1;
				if (vncount == vncap) { vncap *= 2; vn = realloc(vn, vncap * sizeof(v3)); }
				vn[vncount++] = p;
			}
		}
		else if (buf[0] == 'f' && buf[1] == ' ') {
			// scan face (v_id/vt_id/vn_id)
			u32 v_idx[32];
			u32 vt_idx[32];
			u32 vn_idx[32];
			sz n = 0;
			char* p = buf + 1;
			while (*p && n < 32) {
				while (*p == ' ') p++;
				if (*p == '\0' || *p == '\n') break;
				int v_id = -1, vt_id = -1, vn_id = -1;

				sscanf(p, "%d", &v_id);
				while (*p != '/' && *p != ' ' && *p != '\n') p++;
				if (*p == '/') {
					p++;
					if (*p != '/') {
						sscanf(p, "%d", &vt_id);
						while (*p != '/' && *p != ' ' && *p != '\n') p++;
					}
					if (*p == '/') {
						p++;
						sscanf(p, "%d", &vn_id);
					}
				}

				// if (sscanf(p, "%d/%d/%d", &v_id, &vt_id, &vn_id) != 3) break;
				v_idx[n] = (u32)(v_id - 1); // obj starts with index 1 for whatever reason...
				vt_idx[n] = (u32)(vt_id - 1);
				vn_idx[n] = (u32)(vn_id - 1);
				n++;
				while (*p && *p != ' ' && *p != '\n') p++;
			}
			for (sz i = 1; i + 1 < n; ++i) {
			// if (n >= 3) {
				if (tcount == tcap) { tcap *= 2; t = realloc(t, tcap * sizeof(tri)); }
				t[tcount++] = (tri){
					(v3){  v_idx[0],  v_idx[i],  v_idx[i+1] },
					(v3){ vt_idx[0], vt_idx[i], vt_idx[i+1] },
					(v3){ vn_idx[0], vn_idx[i], vn_idx[i+1] }
				};
				// }
				// t[tcount++] = (tri){
				// 	(v3){  v_idx[0],  v_idx[1],  v_idx[2] },
				// 	(v3){ vt_idx[0], vt_idx[1], vt_idx[2] },
				// 	(v3){ vn_idx[0], vn_idx[1], vn_idx[2] }
				// };
			}
			// current_line++;
		}
	}
	object o = {
		(mesh) { v, vcount, vt, vtcount, vn, vncount, t, tcount },
		boundbox_make(v, vcount),
		NULL
	};

	scene_object_push(scene, o);
}

void mesh_free(mesh m) {
	free(m.verts);
	free(m.tris);
}

void scene_free(scene* sc) {
	for (sz i = 0; i > sc->obj_count; ++i) {
		bvh_free(sc->objects[i].bvh);
		free(sc->objects[i].mesh.verts);
		free(sc->objects[i].mesh.t_coords);
		free(sc->objects[i].mesh.v_norms);
		free(sc->objects[i].mesh.tris);
	}
}
