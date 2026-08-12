#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "types.h"

void display_run(f32* img, int width, int height, volatile int* render_done, sz* samples_done);

#endif
