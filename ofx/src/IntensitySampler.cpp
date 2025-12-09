#include "IntensitySampler.h"
#include "GPURenderer.h"
#include "CPURenderer.h"
#include "ofxImageEffect.h"

#include <cmath>
#include <algorithm>

IntensitySampler::IntensitySampler()
    : _gpuAvailable(false)
    , _gpuRenderer(nullptr)
    , _cpuRenderer(nullptr)
    , _forcedBackend(Auto)
{
    // Check GPU availability and pre-create renderer
    _gpuAvailable = GPURenderer::isAvailable();
    if (_gpuAvailable) {
        try {
            _gpuRenderer = std::make_unique<GPURenderer>();
        } catch (...) {
            _gpuAvailable = false;
            _gpuRenderer = nullptr;
        }
    }
    // Always have CPU fallback ready
    try {
        _cpuRenderer = std::make_unique<CPURenderer>();
    } catch (...) {
        _cpuRenderer = nullptr;
    }
}

IntensitySampler::~IntensitySampler()
{
    // Unique_ptr handles cleanup automatically
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
    
    // Backend selection logic
    if (_forcedBackend == CPU) {
        _lastUsedRenderer = "CPU";
        sampleCPU(image, point1, point2, sampleCount, imageWidth, imageHeight,
                  redSamples, greenSamples, blueSamples);
        return;
    }
    if (_forcedBackend == OpenCL) {
        if (_gpuRenderer && _gpuRenderer->isAvailable() && std::string(_gpuRenderer->getBackendName()) == "OpenCL (GPU)") {
            if (sampleGPU(image, point1, point2, sampleCount, imageWidth, imageHeight,
                         redSamples, greenSamples, blueSamples)) {
                _lastUsedRenderer = _gpuRenderer->getBackendName();
                return;
            }
        }
        // Fallback to CPU if OpenCL not available
        _lastUsedRenderer = "CPU";
        sampleCPU(image, point1, point2, sampleCount, imageWidth, imageHeight,
                  redSamples, greenSamples, blueSamples);
        return;
    }
    // Auto: try GPU (Metal/OpenCL), fallback to CPU
    if (_gpuAvailable && sampleGPU(image, point1, point2, sampleCount, imageWidth, imageHeight,
                                    redSamples, greenSamples, blueSamples)) {
        _lastUsedRenderer = GPURenderer::getBackendName();
        return;
    }
    // CPU fallback
    _lastUsedRenderer = "CPU";
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
    // Use cached GPU renderer for better performance
    if (_gpuRenderer) {
        return _gpuRenderer->sampleIntensity(image, point1, point2, sampleCount, imageWidth, imageHeight,
                                            redSamples, greenSamples, blueSamples);
    }
    return false;
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
    // Use cached CPU renderer for better performance
    if (_cpuRenderer) {
        _cpuRenderer->sampleIntensity(image, point1, point2, sampleCount, imageWidth, imageHeight,
                                     redSamples, greenSamples, blueSamples);
    }
}
