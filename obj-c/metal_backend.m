#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <string.h>
#include "../include/metal_backend.h"
#include "../include/log.h"

@interface HikariMetalCtx : NSObject
@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) id<MTLCommandQueue> queue;
@property (nonatomic, strong) id<MTLComputePipelineState> pipeline;
@end

@implementation HikariMetalCtx
@end

metal_ctx* metal_init(void) {
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            print(ERROR, "No Metal device found");
            return NULL;
        }

        NSURL* libURL = [NSURL fileURLWithPath:@"assets/shaders/shader.metallib"];
        NSError* err = nil;
        id<MTLLibrary> lib = [device newLibraryWithURL:libURL error:&err];
        if (!lib) {
            print(ERROR, "Failed to load shader.metallib: %s", [[err localizedDescription] UTF8String]);
            return NULL;
        }

        id<MTLFunction> fn = [lib newFunctionWithName:@"stub_fill"];
        id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:fn error:&err];
        if (!pipeline) {
            print(ERROR, "Pipeline creation failed: %s", [[err localizedDescription] UTF8String]);
            return NULL;
        }

        HikariMetalCtx* ctx = [HikariMetalCtx new];
        ctx.device = device;
        ctx.queue = [device newCommandQueue];
        ctx.pipeline = pipeline;

        print(INFO, "Metal backend initialized on %s", [[device name] UTF8String]);

        return (metal_ctx*)CFBridgingRetain(ctx); // hand ownership to the C side
    }
}

void metal_render_sample(metal_ctx* c_ctx, f32* img, sz width, sz height, camera cam, u32 seed) {
    (void)cam; (void)seed; // unused until real material/BVH work lands
    if (!c_ctx) return;

    @autoreleasepool {
        HikariMetalCtx* ctx = (__bridge HikariMetalCtx*)c_ctx;

        sz img_bytes = width * height * 3 * sizeof(f32);
        id<MTLBuffer> out_buf = [ctx.device newBufferWithLength:img_bytes
                                                          options:MTLResourceStorageModeShared];

        uint32_t w = (uint32_t)width, h = (uint32_t)height;
        id<MTLBuffer> w_buf = [ctx.device newBufferWithBytes:&w length:sizeof(w) options:MTLResourceStorageModeShared];
        id<MTLBuffer> h_buf = [ctx.device newBufferWithBytes:&h length:sizeof(h) options:MTLResourceStorageModeShared];

        id<MTLCommandBuffer> cmd = [ctx.queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];

        [enc setComputePipelineState:ctx.pipeline];
        [enc setBuffer:out_buf offset:0 atIndex:0];
        [enc setBuffer:w_buf offset:0 atIndex:1];
        [enc setBuffer:h_buf offset:0 atIndex:2];

        MTLSize grid = MTLSizeMake(width, height, 1);
        NSUInteger tw = ctx.pipeline.threadExecutionWidth;
        NSUInteger th = ctx.pipeline.maxTotalThreadsPerThreadgroup / tw;
        MTLSize threadgroup = MTLSizeMake(tw, th, 1);

        [enc dispatchThreads:grid threadsPerThreadgroup:threadgroup];
        [enc endEncoding];

        [cmd commit];
        [cmd waitUntilCompleted]; // fine for the stub; revisit for progressive preview later

        memcpy(img, out_buf.contents, img_bytes);
    }
}

void metal_shutdown(metal_ctx* c_ctx) {
    if (c_ctx) CFBridgingRelease(c_ctx); // gives ARC the object back, it dealloc's normally
}
