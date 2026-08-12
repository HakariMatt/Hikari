#ifndef _RENDER_H
#define _RENDER_H

#include "types.h"

void render(f32* img, sz width, sz height, camera cam, scene* sc);
void render_progressive(f32* img, sz width, sz height, camera cam, scene* sc, sz* samples_done);

#endif
