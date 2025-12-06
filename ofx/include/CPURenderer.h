#ifndef CPU_RENDERER_H
#define CPU_RENDERER_H

#include "ofxImageEffect.h"
#include "ofxsImageEffect.h"
#include <vector>

/**
 * CPU fallback implementation for intensity sampling.
 * Used when GPU acceleration is not available.
 */
class CPURenderer
{
public:
    CPURenderer();
    ~CPURenderer();
    
    /**
     * Sample intensity values using CPU implementation.
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
    void bilinearSample(
        const float* imageData,
        int imageWidth,
        int imageHeight,
        int componentCount,
        double x, double y,
        float& red, float& green, float& blue
    );
};

#endif // CPU_RENDERER_H
