#ifndef _METAL_BACKEND_H
#define _METAL_BACKEND_H

#include "types.h"
#include "camera.h"
#include "render_backend.h"

typedef struct metal_ctx metal_ctx; // opaque — never dereferenced on the C side

metal_ctx* metal_init(void);
void metal_render_sample(metal_ctx* c_ctx, render_args args, uint32_t sample);
void metal_shutdown(metal_ctx* ctx);

#endif
