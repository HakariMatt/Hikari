#ifndef _GPU_TYPES_H
#define _GPU_TYPES_H

#ifdef __METAL_VERSION__
    typedef packed_float3 gpu_v3;
#else
    typedef struct { float x, y, z; } gpu_v3;
#endif

typedef struct {
    gpu_v3 origin;
    gpu_v3 lower_left;
    gpu_v3 horizontal;
    gpu_v3 vertical;
} gpu_camera;

typedef struct {
  const unsigned int width, height;
  gpu_camera cam;
} gpu_args;

#endif
