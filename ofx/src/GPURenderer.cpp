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
#include <cstring>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

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
    
    try {
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
    } catch (...) {
        // If any exception occurs, mark as unavailable
        _metalAvailable = false;
        _openclAvailable = false;
    }

    _availabilityChecked = true;
}

bool GPURenderer::isAvailable()
{
    // Don't create an instance - just check static flags
    // Ensure availability has been checked once
    if (!_availabilityChecked) {
        GPURenderer temp;
        // This calls checkAvailability() which sets the static flags
    }
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
        
        // Load Metal library from OFX plugin bundle
        // OFX plugins are bundles, so we need to get the Resources folder
        NSString* executablePath = [[NSProcessInfo processInfo] arguments][0];
        NSString* bundlePath = [[executablePath stringByDeletingLastPathComponent] stringByDeletingLastPathComponent];
        NSString* resourcesPath = [bundlePath stringByAppendingPathComponent:@"Resources"];
        NSString* libraryPath = [resourcesPath stringByAppendingPathComponent:@"intensitySampler.metallib"];
        
        NSURL* libraryURL = [NSURL fileURLWithPath:libraryPath];
        
        NSError* error = nil;
        id<MTLLibrary> library = nil;
        
        // Try loading from Resources folder
        if ([[NSFileManager defaultManager] fileExistsAtPath:libraryPath]) {
            library = [device newLibraryWithURL:libraryURL error:&error];
        }
        
        // If that fails, try main bundle (for development/testing)
        if (!library) {
            NSBundle* mainBundle = [NSBundle mainBundle];
            libraryURL = [mainBundle URLForResource:@"intensitySampler" withExtension:@"metallib"];
            if (libraryURL) {
                library = [device newLibraryWithURL:libraryURL error:&error];
            }
        }
        
        // Last resort: try to use default library
        if (!library) {
            library = [device newDefaultLibrary];
        }
        
        if (!library) {
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
        
        int rowBytes = image->getRowBytes();
        OFX::PixelComponentEnum components = image->getPixelComponents();
        int componentCount = (components == OFX::ePixelComponentRGBA) ? 4 : 3;
        
        // Pack image data to handle stride/padding and negative rowBytes
        std::vector<float> packedData;
        try {
            packedData.resize(imageWidth * imageHeight * componentCount);
        } catch (...) {
            return false;
        }
        
        for (int y = 0; y < imageHeight; ++y) {
            const float* srcRow = (const float*)((const char*)imageData + y * rowBytes);
            float* dstRow = packedData.data() + y * imageWidth * componentCount;
            std::memcpy(dstRow, srcRow, imageWidth * componentCount * sizeof(float));
        }
        
        // Create input buffer
        size_t imageDataSize = packedData.size() * sizeof(float);
        id<MTLBuffer> inputBuffer = [device newBufferWithBytes:packedData.data() length:imageDataSize options:MTLResourceStorageModeShared];
        
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
    cl_int err;

    // Only float images supported for now
    if (image->getPixelDepth() != OFX::eBitDepthFloat) {
        return false;
    }

    // Get platform
    cl_platform_id platform;
    cl_uint platformCount;
    err = clGetPlatformIDs(1, &platform, &platformCount);
    if (err != CL_SUCCESS || platformCount == 0) {
        return false;
    }

    // Get device (GPU first, CPU fallback)
    cl_device_id device;
    cl_uint deviceCount;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &deviceCount);
    if (err != CL_SUCCESS || deviceCount == 0) {
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 1, &device, &deviceCount);
        if (err != CL_SUCCESS || deviceCount == 0) {
            return false;
        }
    }

    // Create context and command queue
    cl_context context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    if (err != CL_SUCCESS) {
        return false;
    }

#ifdef CL_VERSION_2_0
    cl_command_queue queue = clCreateCommandQueueWithProperties(context, device, 0, &err);
#else
    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
#endif
    if (err != CL_SUCCESS) {
        clReleaseContext(context);
        return false;
    }

    // Kernel source (embedded)
    const char* kernelSrc = R"CLC(
    typedef struct {
        float point1X, point1Y;
        float point2X, point2Y;
        int imageWidth, imageHeight;
        int sampleCount;
        int componentCount;
    } Parameters;

    float3 bilinearSample(__global const float* imageData,
                          int imageWidth,
                          int imageHeight,
                          int componentCount,
                          float x,
                          float y) {
        x = clamp(x, 0.0f, (float)(imageWidth - 1));
        y = clamp(y, 0.0f, (float)(imageHeight - 1));

        int x0 = (int)floor(x);
        int y0 = (int)floor(y);
        int x1 = min(x0 + 1, imageWidth - 1);
        int y1 = min(y0 + 1, imageHeight - 1);

        float fx = x - (float)x0;
        float fy = y - (float)y0;

        int index00 = (y0 * imageWidth + x0) * componentCount;
        int index10 = (y0 * imageWidth + x1) * componentCount;
        int index01 = (y1 * imageWidth + x0) * componentCount;
        int index11 = (y1 * imageWidth + x1) * componentCount;

        float3 c00 = (float3)(imageData[index00 + 0], imageData[index00 + 1], imageData[index00 + 2]);
        float3 c10 = (float3)(imageData[index10 + 0], imageData[index10 + 1], imageData[index10 + 2]);
        float3 c01 = (float3)(imageData[index01 + 0], imageData[index01 + 1], imageData[index01 + 2]);
        float3 c11 = (float3)(imageData[index11 + 0], imageData[index11 + 1], imageData[index11 + 2]);

        float3 c0 = mix(c00, c10, fx);
        float3 c1 = mix(c01, c11, fx);
        return mix(c0, c1, fy);
    }

    __kernel void sampleIntensity(
        __global const float* inputImage,
        __global float* outputSamples,
        __global const Parameters* params) {
        int id = get_global_id(0);
        if (id >= params->sampleCount) return;

        float t = (float)id / (float)(max(1, params->sampleCount - 1));

        float2 p1 = (float2)(params->point1X * (float)(params->imageWidth),
                             params->point1Y * (float)(params->imageHeight));
        float2 p2 = (float2)(params->point2X * (float)(params->imageWidth),
                             params->point2Y * (float)(params->imageHeight));

        float2 pos = mix(p1, p2, t);

        float3 rgb = bilinearSample(inputImage,
                                    params->imageWidth,
                                    params->imageHeight,
                                    params->componentCount,
                                    pos.x,
                                    pos.y);

        int outIdx = id * 3;
        outputSamples[outIdx + 0] = rgb.x;
        outputSamples[outIdx + 1] = rgb.y;
        outputSamples[outIdx + 2] = rgb.z;
    }
    )CLC";

    // Build program
    const size_t lengths = std::strlen(kernelSrc);
    const char* sources[1] = { kernelSrc };
    const size_t sourceLengths[1] = { lengths };
    cl_program program = clCreateProgramWithSource(context, 1, sources, sourceLengths, &err);
    if (err != CL_SUCCESS) {
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    err = clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        // Optional: fetch build log
        size_t logSize = 0;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        std::vector<char> log(logSize + 1, 0);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    cl_kernel kernel = clCreateKernel(program, "sampleIntensity", &err);
    if (err != CL_SUCCESS) {
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    // Pack image data (remove stride)
    float* imageData = (float*)image->getPixelData();
    if (!imageData) {
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    OFX::PixelComponentEnum components = image->getPixelComponents();
    int componentCount = (components == OFX::ePixelComponentRGBA) ? 4 : 3;
    int rowBytes = image->getRowBytes();
    size_t packedSize = imageWidth * imageHeight * componentCount;

    std::vector<float> packed;
    try {
        packed.resize(packedSize);
    } catch (...) {
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    for (int y = 0; y < imageHeight; ++y) {
        const float* srcRow = (const float*)((const char*)imageData + y * rowBytes);
        float* dstRow = packed.data() + y * imageWidth * componentCount;
        std::memcpy(dstRow, srcRow, imageWidth * componentCount * sizeof(float));
    }

    // Buffers
    cl_mem inputBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                        packedSize * sizeof(float), packed.data(), &err);
    if (err != CL_SUCCESS) {
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    size_t outputSize = sampleCount * 3;
    cl_mem outputBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, outputSize * sizeof(float), nullptr, &err);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

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

    cl_mem paramBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                        sizeof(params), &params, &err);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(outputBuffer);
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    // Set kernel args
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &paramBuffer);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(paramBuffer);
        clReleaseMemObject(outputBuffer);
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    // Launch kernel
    size_t globalWorkSize[1] = { static_cast<size_t>(sampleCount) };
    size_t localWorkSize[1] = { 64 };
    if (globalWorkSize[0] < localWorkSize[0]) {
        localWorkSize[0] = globalWorkSize[0];
    }

    err = clEnqueueNDRangeKernel(queue, kernel, 1, nullptr, globalWorkSize, localWorkSize, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(paramBuffer);
        clReleaseMemObject(outputBuffer);
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    clFinish(queue);

    // Read results
    std::vector<float> output(outputSize);
    err = clEnqueueReadBuffer(queue, outputBuffer, CL_TRUE, 0, outputSize * sizeof(float), output.data(), 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        clReleaseMemObject(paramBuffer);
        clReleaseMemObject(outputBuffer);
        clReleaseMemObject(inputBuffer);
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return false;
    }

    // Fill output vectors
    redSamples.clear();
    greenSamples.clear();
    blueSamples.clear();
    redSamples.reserve(sampleCount);
    greenSamples.reserve(sampleCount);
    blueSamples.reserve(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        redSamples.push_back(output[i * 3 + 0]);
        greenSamples.push_back(output[i * 3 + 1]);
        blueSamples.push_back(output[i * 3 + 2]);
    }

    // Cleanup
    clReleaseMemObject(paramBuffer);
    clReleaseMemObject(outputBuffer);
    clReleaseMemObject(inputBuffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return true;
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
