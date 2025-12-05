#include "IntensitySampler.h"
#include "GPURenderer.h"
#include "CPURenderer.h"
#include "ofxImageEffect.h"

#include <cmath>
#include <algorithm>

IntensitySampler::IntensitySampler()
    : _gpuAvailable(false)
{
    // Check GPU availability
    _gpuAvailable = GPURenderer::isAvailable();
}

IntensitySampler::~IntensitySampler()
{
}

void IntensitySampler::sampleIntensity(
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
    // Clear output vectors
    redSamples.clear();
    greenSamples.clear();
    blueSamples.clear();
    redSamples.reserve(sampleCount);
    greenSamples.reserve(sampleCount);
    blueSamples.reserve(sampleCount);
    
    // Try GPU first, fallback to CPU
    if (_gpuAvailable && sampleGPU(image, point1, point2, sampleCount, imageWidth, imageHeight,
                                    redSamples, greenSamples, blueSamples)) {
        return;
    }
    
    // CPU fallback
    sampleCPU(image, point1, point2, sampleCount, imageWidth, imageHeight,
              redSamples, greenSamples, blueSamples);
}

bool IntensitySampler::sampleGPU(
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
    // Delegate to GPURenderer
    GPURenderer renderer;
    return renderer.sampleIntensity(image, point1, point2, sampleCount, imageWidth, imageHeight,
                                    redSamples, greenSamples, blueSamples);
}

void IntensitySampler::sampleCPU(
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
    // Delegate to CPURenderer
    CPURenderer renderer;
    renderer.sampleIntensity(image, point1, point2, sampleCount, imageWidth, imageHeight,
                             redSamples, greenSamples, blueSamples);
}
