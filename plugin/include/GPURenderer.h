#ifndef GPU_RENDERER_H
#define GPU_RENDERER_H

#include "ofxImageEffect.h"
#include <vector>

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
};

#endif // GPU_RENDERER_H
