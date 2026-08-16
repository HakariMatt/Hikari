#ifndef _METAL_BACKEND_H
#define _METAL_BACKEND_H

#include "types.h"
#include "camera.h"

typedef struct metal_ctx metal_ctx; // opaque — never dereferenced on the C side

metal_ctx* metal_init(void);
void metal_render_sample(metal_ctx* ctx, f32* img, sz s, sz width, sz height, camera cam, u32 seed);
void metal_shutdown(metal_ctx* ctx);

#endif
