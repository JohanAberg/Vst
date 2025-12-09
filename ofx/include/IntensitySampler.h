#ifndef INTENSITY_SAMPLER_H
#define INTENSITY_SAMPLER_H

#include "ofxImageEffect.h"
#include "ofxsImageEffect.h"
#include <vector>
#include <memory>

/**
 * Samples intensity values along a scan line from an OFX image.
 * Supports both GPU-accelerated and CPU fallback implementations.
 */
class IntensitySampler
{
public:
    enum Backend {
        Auto = 0,      // Try GPU first, fallback to CPU
        OpenCL = 1,    // Force OpenCL, fallback to CPU if unavailable
        CPU = 2        // Force CPU only
    };
    
    IntensitySampler();
    ~IntensitySampler();
    
    /**
     * Set the backend to use for sampling (GPU vs CPU)
     * @param backend Backend mode (Auto, OpenCL, or CPU)
     */
    void setBackend(Backend backend) { _forcedBackend = backend; }
    
    /**
     * Sample intensity values along a scan line defined by two normalized points.
     * 
     * @param image Source OFX image to sample from
     * @param point1 Normalized coordinates [0-1, 0-1] of first endpoint
     * @param point2 Normalized coordinates [0-1, 0-1] of second endpoint
     * @param sampleCount Number of samples to take along the line
     * @param imageWidth Width of the image in pixels
     * @param imageHeight Height of the image in pixels
     * @param redSamples Output vector for red channel samples
     * @param greenSamples Output vector for green channel samples
     * @param blueSamples Output vector for blue channel samples
     */
    void sampleIntensity(
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
    
    /**
     * Get the name of the renderer used for the last sample operation.
     * @return "Metal (GPU)", "OpenCL (GPU)", or "CPU"
     */
    const char* getLastUsedRenderer() const { return _lastUsedRenderer; }

private:
    void sampleCPU(
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

    bool sampleGPU(
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

    bool _gpuAvailable;
    std::unique_ptr<class GPURenderer> _gpuRenderer;  // Cached GPU renderer
    std::unique_ptr<class CPURenderer> _cpuRenderer;  // Cached CPU renderer
    const char* _lastUsedRenderer = "Not sampled yet";
    Backend _forcedBackend = Auto;  // Backend selection mode
};

#endif // INTENSITY_SAMPLER_H