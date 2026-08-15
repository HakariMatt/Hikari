#include <metal_stdlib>
using namespace metal;

kernel void stub_fill(device float* out        [[buffer(0)]],
                       constant uint& width    [[buffer(1)]],
                       constant uint& height   [[buffer(2)]],
                       uint2 gid               [[thread_position_in_grid]])
{
    if (gid.x >= width || gid.y >= height) return;
    uint idx = (gid.y * width + gid.x) * 3;
    out[idx + 0] = float(gid.x) / float(width);
    out[idx + 1] = float(gid.y) / float(height);
    out[idx + 2] = 0.2;
}
