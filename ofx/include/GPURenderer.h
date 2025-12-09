#ifndef GPU_RENDERER_H
#define GPU_RENDERER_H

#include "ofxImageEffect.h"
#include "ofxsImageEffect.h"
#include <vector>
#include <mutex>
#include <unordered_map>

#ifdef HAVE_OPENCL
#include <CL/cl.h>
#endif

/**
 * GPU-accelerated rendering implementation.
 * Supports Metal (macOS) and OpenCL (cross-platform) backends.
 */
class GPURenderer
{
public:
    GPURenderer();
    ~GPURenderer();
    
    static bool isAvailable();
    
    /**
     * Get the name of the active GPU backend ("Metal", "OpenCL", or "None").
     */
    static const char* getBackendName();
    
    /**
     * Set host-provided OpenCL command queue for non-blocking GPU execution.
     * If not called, a queue will be created automatically.
     */
#ifdef HAVE_OPENCL
    void setHostOpenCLQueue(cl_command_queue hostQueue);
#endif
    
    /**
     * Sample intensity values using GPU acceleration.
     * 
     * @param image Source OFX image
     * @param point1 Normalized coordinates of first endpoint
     * @param point2 Normalized coordinates of second endpoint
     * @param sampleCount Number of samples
     * @param imageWidth Image width in pixels
     * @param imageHeight Image height in pixels
     * @param redSamples Output red channel samples
     * @param greenSamples Output green channel samples
     * @param blueSamples Output blue channel samples
     * @return true if GPU sampling succeeded, false to fallback to CPU
     */
    bool sampleIntensity(
        OFX::Image* image,
        const double point1[2],
        const double point2[2],
        int sampleCount,
        int imageWidth,
        int imageHeight,
        std::vector<float>& redSamples,
        std::vector<float>& greenSamples,
        std::vector<float>& blueSamples
    );

private:
    bool sampleMetal(
        OFX::Image* image,
        const double point1[2],
        const double point2[2],
        int sampleCount,
        int imageWidth,
        int imageHeight,
        std::vector<float>& redSamples,
        std::vector<float>& greenSamples,
        std::vector<float>& blueSamples
    );
    
    bool sampleOpenCL(
        OFX::Image* image,
        const double point1[2],
        const double point2[2],
        int sampleCount,
        int imageWidth,
        int imageHeight,
        std::vector<float>& redSamples,
        std::vector<float>& greenSamples,
        std::vector<float>& blueSamples
    );
    
    static bool _metalAvailable;
    static bool _openclAvailable;
    static bool _availabilityChecked;
    
    void checkAvailability();
    
    // Optimization #1: Kernel caching to avoid recompilation
#ifdef HAVE_OPENCL
    std::unordered_map<uintptr_t, cl_program> _cachedPrograms;  // device -> compiled program
    std::mutex _programCacheMutex;
    
    cl_program getCachedOrCompileProgram(cl_context context, cl_device_id device);
    
    // Optimization #2: Buffer pool to reduce malloc/free overhead
    struct BufferPoolEntry {
        cl_mem buffer;
        size_t size;
        bool inUse;
    };
    std::vector<BufferPoolEntry> _bufferPool;  // Pre-allocated GPU memory cache
    std::mutex _bufferPoolMutex;
    
    /**
     * Get or allocate a GPU buffer of at least `size` bytes.
     * Reuses existing buffers from pool if available to avoid clCreateBuffer overhead.
     */
    cl_mem getOrAllocateBuffer(cl_context context, size_t size, cl_mem_flags flags, cl_int& err);
    
    /**
     * Release a buffer back to the pool for reuse rather than deallocating immediately.
     */
    void releaseBufferToPool(cl_mem buffer);
    
    /**
     * Clear all pooled buffers (called in destructor).
     */
    void clearBufferPool();
    
    // Optimization #3: Pinned (page-locked) host memory for faster GPU transfers
    std::vector<float> _pinnedHostMemory;  // Pre-allocated pinned memory for sampling results
    bool _havePinnedMemory;
    
    /**
     * Allocate pinned memory for faster GPU-to-CPU transfers.
     * Returns true if pinned memory was successfully allocated.
     */
    bool allocatePinnedMemory(cl_context context, size_t size);
    
    // Optimization #4: Async GPU queue support (CRITICAL for responsiveness)
    cl_command_queue _hostOpenCLQueue = nullptr;  // Host-provided queue (we don't own)
    bool _ownsOpenCLQueue = true;  // Track if we created the queue
    
    /**
     * Set a host-provided OpenCL command queue.
     * When host provides its queue, plugin enqueues work and returns immediately.
     * Host manages GPU synchronization for better responsiveness.
     * 
     * @param hostQueue OpenCL command queue from host (don't release in plugin)
     */
    void setHostOpenCLQueue(cl_command_queue hostQueue);
#endif
};

#endif // GPU_RENDERER_H
