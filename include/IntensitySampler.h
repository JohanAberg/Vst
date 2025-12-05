#ifndef INTENSITY_SAMPLER_H
#define INTENSITY_SAMPLER_H

#include "ofxImageEffect.h"
#include <vector>

/**
 * Samples intensity values along a scan line from an OFX image.
 * Supports both GPU-accelerated and CPU fallback implementations.
 */
class IntensitySampler
{
public:
    IntensitySampler();
    ~IntensitySampler();
    
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
};

#endif // INTENSITY_SAMPLER_H
