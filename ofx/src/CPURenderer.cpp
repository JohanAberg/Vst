#include "CPURenderer.h"
#include "ofxImageEffect.h"
#include <cmath>
#include <algorithm>

CPURenderer::CPURenderer()
{
}

CPURenderer::~CPURenderer()
{
}

void CPURenderer::sampleIntensity(
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
    
    // Get image data
    float* imageData = (float*)image->getPixelData();
    if (!imageData) {
        return;
    }
    
    int rowBytes = image->getRowBytes();
    
    OFX::PixelComponentEnum components = image->getPixelComponents();
    int componentCount = (components == OFX::ePixelComponentRGBA) ? 4 : 3;
    
    // Convert normalized coordinates to pixel coordinates
    double px1 = point1[0] * imageWidth;
    double py1 = point1[1] * imageHeight;
    double px2 = point2[0] * imageWidth;
    double py2 = point2[1] * imageHeight;
    
    // Sample along the line
    for (int i = 0; i < sampleCount; ++i) {
        double t = static_cast<double>(i) / static_cast<double>(sampleCount - 1);
        
        // Interpolate position along line
        double x = px1 + t * (px2 - px1);
        double y = py1 + t * (py2 - py1);
        
        // Clamp to image bounds
        x = std::max(0.0, std::min(static_cast<double>(imageWidth - 1), x));
        y = std::max(0.0, std::min(static_cast<double>(imageHeight - 1), y));
        
        // Bilinear sample
        float red, green, blue;
        bilinearSample(imageData, rowBytes, imageWidth, imageHeight, componentCount, x, y, red, green, blue);
        
        redSamples.push_back(red);
        greenSamples.push_back(green);
        blueSamples.push_back(blue);
    }
}

void CPURenderer::bilinearSample(
    const float* imageData,
    int rowBytes,
    int imageWidth,
    int imageHeight,
    int componentCount,
    double x, double y,
    float& red, float& green, float& blue)
{
    // Get integer coordinates
    int x0 = static_cast<int>(std::floor(x));
    int y0 = static_cast<int>(std::floor(y));
    int x1 = std::min(x0 + 1, imageWidth - 1);
    int y1 = std::min(y0 + 1, imageHeight - 1);
    
    // Get fractional parts
    double fx = x - x0;
    double fy = y - y0;
    
    // Sample four corners
    auto samplePixel = [&](int px, int py, float& r, float& g, float& b) {
        // Use rowBytes for correct stride
        const float* row = (const float*)((const char*)imageData + py * rowBytes);
        int index = px * componentCount;
        r = row[index + 0];
        g = row[index + 1];
        b = row[index + 2];
    };
    
    float r00, g00, b00, r10, g10, b10, r01, g01, b01, r11, g11, b11;
    samplePixel(x0, y0, r00, g00, b00);
    samplePixel(x1, y0, r10, g10, b10);
    samplePixel(x0, y1, r01, g01, b01);
    samplePixel(x1, y1, r11, g11, b11);
    
    // Bilinear interpolation
    float r0 = static_cast<float>(r00 * (1.0 - fx) + r10 * fx);
    float g0 = static_cast<float>(g00 * (1.0 - fx) + g10 * fx);
    float b0 = static_cast<float>(b00 * (1.0 - fx) + b10 * fx);
    
    float r1 = static_cast<float>(r01 * (1.0 - fx) + r11 * fx);
    float g1 = static_cast<float>(g01 * (1.0 - fx) + g11 * fx);
    float b1 = static_cast<float>(b01 * (1.0 - fx) + b11 * fx);
    
    red = static_cast<float>(r0 * (1.0 - fy) + r1 * fy);
    green = static_cast<float>(g0 * (1.0 - fy) + g1 * fy);
    blue = static_cast<float>(b0 * (1.0 - fy) + b1 * fy);
}
