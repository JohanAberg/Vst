#include "GPURenderer.h"
#include "ofxImageEffect.h"

#ifdef __APPLE__
#include <Metal/Metal.h>
#include <MetalKit/MetalKit.h>
#include <Foundation/Foundation.h>
#endif

#ifdef HAVE_OPENCL
#include <CL/cl.h>
#endif

#include <cmath>
#include <algorithm>

bool GPURenderer::_metalAvailable = false;
bool GPURenderer::_openclAvailable = false;
bool GPURenderer::_availabilityChecked = false;

GPURenderer::GPURenderer()
{
    checkAvailability();
}

GPURenderer::~GPURenderer()
{
}

void GPURenderer::checkAvailability()
{
    if (_availabilityChecked) {
        return;
    }
    
#ifdef __APPLE__
    // Check Metal availability
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        _metalAvailable = (device != nil);
        if (device) {
            [device release];
        }
    }
#endif

#ifdef HAVE_OPENCL
    // Check OpenCL availability
    cl_uint platformCount = 0;
    clGetPlatformIDs(0, nullptr, &platformCount);
    _openclAvailable = (platformCount > 0);
#else
    _openclAvailable = false;
#endif

    _availabilityChecked = true;
}

bool GPURenderer::isAvailable()
{
    GPURenderer renderer;
    return _metalAvailable || _openclAvailable;
}

bool GPURenderer::sampleIntensity(
    OFX::Image* image,
    const double point1[2],
    const double point2[2],
    int sampleCount,
    int imageWidth,
    int imageHeight,
    std::vector<float>& redSamples,
    std::vector<float>& greenSamples,
    std::vector<float>& blueSamples)
{
    // Try Metal first (macOS priority)
#ifdef __APPLE__
    if (_metalAvailable) {
        if (sampleMetal(image, point1, point2, sampleCount, imageWidth, imageHeight,
                       redSamples, greenSamples, blueSamples)) {
            return true;
        }
    }
#endif

    // Fallback to OpenCL
#ifdef HAVE_OPENCL
    if (_openclAvailable) {
        if (sampleOpenCL(image, point1, point2, sampleCount, imageWidth, imageHeight,
                        redSamples, greenSamples, blueSamples)) {
            return true;
        }
    }
#endif

    return false;
}

#ifdef __APPLE__
bool GPURenderer::sampleMetal(
    OFX::Image* image,
    const double point1[2],
    const double point2[2],
    int sampleCount,
    int imageWidth,
    int imageHeight,
    std::vector<float>& redSamples,
    std::vector<float>& greenSamples,
    std::vector<float>& blueSamples)
{
    @autoreleasepool {
        // Get Metal device
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            return false;
        }
        
        // Create command queue
        id<MTLCommandQueue> commandQueue = [device newCommandQueue];
        if (!commandQueue) {
            return false;
        }
        
        // Load Metal library (embedded in bundle)
        NSBundle* bundle = [NSBundle bundleForClass:[NSObject class]];
        NSURL* libraryURL = [bundle URLForResource:@"intensitySampler" withExtension:@"metallib"];
        
        if (!libraryURL) {
            // Try loading from default location
            NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
            libraryURL = [NSURL fileURLWithPath:[resourcePath stringByAppendingPathComponent:@"intensitySampler.metallib"]];
        }
        
        NSError* error = nil;
        id<MTLLibrary> library = [device newLibraryWithURL:libraryURL error:&error];
        
        if (!library || error) {
            // Fallback: try to compile from source (for development)
            return false;
        }
        
        // Get kernel function
        id<MTLFunction> kernelFunction = [library newFunctionWithName:@"sampleIntensity"];
        if (!kernelFunction) {
            return false;
        }
        
        // Create compute pipeline
        id<MTLComputePipelineState> pipelineState = [device newComputePipelineStateWithFunction:kernelFunction error:&error];
        if (!pipelineState || error) {
            return false;
        }
        
        // Get image data
        float* imageData = (float*)image->getPixelData();
        if (!imageData) {
            return false;
        }
        
        OFX::PixelComponentEnum components = image->getPixelComponents();
        int componentCount = (components == OFX::ePixelComponentRGBA) ? 4 : 3;
        
        // Create input buffer
        size_t imageDataSize = imageWidth * imageHeight * componentCount * sizeof(float);
        id<MTLBuffer> inputBuffer = [device newBufferWithBytes:imageData length:imageDataSize options:MTLResourceStorageModeShared];
        
        // Create output buffer
        size_t outputSize = sampleCount * 3 * sizeof(float);
        id<MTLBuffer> outputBuffer = [device newBufferWithLength:outputSize options:MTLResourceStorageModeShared];
        
        // Create parameter buffer
        struct Parameters {
            float point1X, point1Y;
            float point2X, point2Y;
            int imageWidth, imageHeight;
            int sampleCount;
            int componentCount;
        } params;
        
        params.point1X = static_cast<float>(point1[0]);
        params.point1Y = static_cast<float>(point1[1]);
        params.point2X = static_cast<float>(point2[0]);
        params.point2Y = static_cast<float>(point2[1]);
        params.imageWidth = imageWidth;
        params.imageHeight = imageHeight;
        params.sampleCount = sampleCount;
        params.componentCount = componentCount;
        
        id<MTLBuffer> paramBuffer = [device newBufferWithBytes:&params length:sizeof(params) options:MTLResourceStorageModeShared];
        
        // Create command buffer
        id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
        id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
        
        // Set pipeline state
        [encoder setComputePipelineState:pipelineState];
        
        // Set buffers
        [encoder setBuffer:inputBuffer offset:0 atIndex:0];
        [encoder setBuffer:outputBuffer offset:0 atIndex:1];
        [encoder setBuffer:paramBuffer offset:0 atIndex:2];
        
        // Dispatch threads
        MTLSize threadgroupSize = MTLSizeMake(64, 1, 1);
        MTLSize gridSize = MTLSizeMake(sampleCount, 1, 1);
        [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
        
        [encoder endEncoding];
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
        
        // Read results
        float* outputData = (float*)[outputBuffer contents];
        
        redSamples.clear();
        greenSamples.clear();
        blueSamples.clear();
        redSamples.reserve(sampleCount);
        greenSamples.reserve(sampleCount);
        blueSamples.reserve(sampleCount);
        
        for (int i = 0; i < sampleCount; ++i) {
            redSamples.push_back(outputData[i * 3 + 0]);
            greenSamples.push_back(outputData[i * 3 + 1]);
            blueSamples.push_back(outputData[i * 3 + 2]);
        }
        
        return true;
    }
}
#else
bool GPURenderer::sampleMetal(
    OFX::Image* image,
    const double point1[2],
    const double point2[2],
    int sampleCount,
    int imageWidth,
    int imageHeight,
    std::vector<float>& redSamples,
    std::vector<float>& greenSamples,
    std::vector<float>& blueSamples)
{
    return false;
}
#endif

#ifdef HAVE_OPENCL
bool GPURenderer::sampleOpenCL(
    OFX::Image* image,
    const double point1[2],
    const double point2[2],
    int sampleCount,
    int imageWidth,
    int imageHeight,
    std::vector<float>& redSamples,
    std::vector<float>& greenSamples,
    std::vector<float>& blueSamples)
{
    // OpenCL implementation
    // This is a simplified version - full implementation would load kernel from file
    cl_int err;
    
    // Get platform
    cl_platform_id platform;
    cl_uint platformCount;
    err = clGetPlatformIDs(1, &platform, &platformCount);
    if (err != CL_SUCCESS || platformCount == 0) {
        return false;
    }
    
    // Get device
    cl_device_id device;
    cl_uint deviceCount;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &deviceCount);
    if (err != CL_SUCCESS || deviceCount == 0) {
        // Try CPU as fallback
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, &deviceCount);
        if (err != CL_SUCCESS || deviceCount == 0) {
            return false;
        }
    }
    
    // Create context
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    if (err != CL_SUCCESS) {
        return false;
    }
    
    // Create command queue
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        clReleaseContext(context);
        return false;
    }
    
    // Load kernel source (would be loaded from file in production)
    // For now, return false to indicate OpenCL needs kernel file
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    return false; // Requires kernel file loading implementation
}
#else
bool GPURenderer::sampleOpenCL(
    OFX::Image* image,
    const double point1[2],
    const double point2[2],
    int sampleCount,
    int imageWidth,
    int imageHeight,
    std::vector<float>& redSamples,
    std::vector<float>& greenSamples,
    std::vector<float>& blueSamples)
{
    return false;
}
#endif
