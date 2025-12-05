//
// Intensity Sampler Metal Kernel
// Samples RGB intensity values along a scan line defined by two points
//

#include <metal_stdlib>
using namespace metal;

// Parameters structure
struct Parameters {
    float2 point1;          // Normalized coordinates [0-1, 0-1]
    float2 point2;          // Normalized coordinates [0-1, 0-1]
    int2 imageSize;         // Image width and height
    int sampleCount;        // Number of samples along the line
    int componentCount;     // 3 for RGB, 4 for RGBA
};

// Bilinear sampling function
float3 bilinearSample(
    device const float* imageData,
    int imageWidth,
    int imageHeight,
    int componentCount,
    float x,
    float y
) {
    // Clamp coordinates
    x = clamp(x, 0.0f, float(imageWidth - 1));
    y = clamp(y, 0.0f, float(imageHeight - 1));
    
    // Get integer coordinates
    int x0 = int(floor(x));
    int y0 = int(floor(y));
    int x1 = min(x0 + 1, imageWidth - 1);
    int y1 = min(y0 + 1, imageHeight - 1);
    
    // Get fractional parts
    float fx = x - float(x0);
    float fy = y - float(y0);
    
    // Sample four corners
    auto samplePixel = [&](int px, int py) -> float3 {
        int index = (py * imageWidth + px) * componentCount;
        float r = imageData[index + 0];
        float g = imageData[index + 1];
        float b = imageData[index + 2];
        return float3(r, g, b);
    };
    
    float3 c00 = samplePixel(x0, y0);
    float3 c10 = samplePixel(x1, y0);
    float3 c01 = samplePixel(x0, y1);
    float3 c11 = samplePixel(x1, y1);
    
    // Bilinear interpolation
    float3 c0 = mix(c00, c10, fx);
    float3 c1 = mix(c01, c11, fx);
    return mix(c0, c1, fy);
}

// Main kernel function
kernel void sampleIntensity(
    device const float* inputImage [[buffer(0)]],
    device float* outputSamples [[buffer(1)]],
    device const Parameters& params [[buffer(2)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= uint(params.sampleCount)) {
        return;
    }
    
    // Calculate position along scan line
    float t = float(id) / float(params.sampleCount - 1);
    
    // Convert normalized coordinates to pixel coordinates
    float2 point1Pixel = float2(
        params.point1.x * float(params.imageSize.x),
        params.point1.y * float(params.imageSize.y)
    );
    float2 point2Pixel = float2(
        params.point2.x * float(params.imageSize.x),
        params.point2.y * float(params.imageSize.y)
    );
    
    // Interpolate position along line
    float2 position = mix(point1Pixel, point2Pixel, t);
    
    // Sample using bilinear interpolation
    float3 rgb = bilinearSample(
        inputImage,
        params.imageSize.x,
        params.imageSize.y,
        params.componentCount,
        position.x,
        position.y
    );
    
    // Write output (packed as RGBRGBRGB...)
    int outputIndex = int(id) * 3;
    outputSamples[outputIndex + 0] = rgb.r;
    outputSamples[outputIndex + 1] = rgb.g;
    outputSamples[outputIndex + 2] = rgb.b;
}
