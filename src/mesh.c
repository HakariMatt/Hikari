#ifndef _MESH_C
#define _MESH_C

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "types.h"

static object mesh_load_obj(const char* path) {
	FILE* f = fopen(path, "r");
	if (!f) { fprintf(stderr, "could not open %s\n", path); exit(1); }

	sz vcap = 1024, vcount = 0;
	v3* verts = malloc(vcap * sizeof(v3));

	sz tcap = 1024, tcount = 0;
	tri* tris = malloc(tcap * sizeof(tri));

	char line[512];
	while (fgets(line, sizeof(line), f)) {
		if (line[0] == 'v' && line[1] == ' ') {
			v3 p;
			if (sscanf(line + 2, "%lf %lf %lf", &p.x, &p.y, &p.z) == 3) {
				if (vcount == vcap) { vcap *= 2; verts = realloc(verts, vcap * sizeof(v3)); }
				verts[vcount++] = p;
			}
		} else if (line[0] == 'f' && line[1] == ' ') {
			// parse all vertex indices on the line (first int of each token),
			// then fan-triangulate: (0,1,2), (0,2,3), (0,3,4), ...
			u32 idx[32];
			sz n = 0;
			char* p = line + 1;
			while (*p && n < 32) {
				while (*p == ' ') p++;
				if (*p == '\0' || *p == '\n') break;
				int v;
				if (sscanf(p, "%d", &v) != 1) break;
				idx[n++] = (u32)(v - 1); // OBJ is 1-indexed
				while (*p && *p != ' ' && *p != '\n') p++; // skip to next token
			}
			for (sz i = 1; i + 1 < n; ++i) {
				if (tcount == tcap) { tcap *= 2; tris = realloc(tris, tcap * sizeof(tri)); }
				tris[tcount++] = (tri){ idx[0], idx[i], idx[i+1] };
			}
		}
	}
	fclose(f);
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
	return (object){
		(mesh){ verts, vcount, tris, tcount },
		(boundbox){ max_x, min_x, max_y, min_y, max_z, min_z }
	};
	// return (mesh){ verts, vcount, tris, tcount };
}

static void mesh_free(mesh m) {
	free(m.verts);
	free(m.tris);
}

#endif
