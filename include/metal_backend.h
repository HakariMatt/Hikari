#ifndef _METAL_BACKEND_H
#define _METAL_BACKEND_H

#include "types.h"
#include "camera.h"
#include "render_backend.h"

typedef struct metal_ctx metal_ctx; // opaque — never dereferenced on the C side

metal_ctx* metalInit(void);
int metalUploadScene(metal_ctx *c_ctx, scene *sc);
void metalRenderSample(metal_ctx* c_ctx, render_args renderArguments, uint32_t sampleNumber);
void metalShutdown(metal_ctx* c_ctx);

#endif
