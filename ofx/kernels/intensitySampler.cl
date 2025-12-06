//
// Intensity Sampler OpenCL Kernel
// Samples RGB intensity values along a scan line defined by two points
//

// Parameters structure (packed for OpenCL)
// Layout: point1.x, point1.y, point2.x, point2.y, imageWidth, imageHeight, sampleCount, componentCount
typedef struct {
    float point1X, point1Y;
    float point2X, point2Y;
    int imageWidth, imageHeight;
    int sampleCount;
    int componentCount;
} Parameters;

// Bilinear sampling function
float3 bilinearSample(
    __global const float* imageData,
    int imageWidth,
    int imageHeight,
    int componentCount,
    float x,
    float y
) {
    // Clamp coordinates
    x = clamp(x, 0.0f, (float)(imageWidth - 1));
    y = clamp(y, 0.0f, (float)(imageHeight - 1));
    
    // Get integer coordinates
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int x1 = min(x0 + 1, imageWidth - 1);
    int y1 = min(y0 + 1, imageHeight - 1);
    
    // Get fractional parts
    float fx = x - (float)x0;
    float fy = y - (float)y0;
    
    // Sample four corners
    int index00 = (y0 * imageWidth + x0) * componentCount;
    int index10 = (y0 * imageWidth + x1) * componentCount;
    int index01 = (y1 * imageWidth + x0) * componentCount;
    int index11 = (y1 * imageWidth + x1) * componentCount;
    
    float3 c00 = (float3)(imageData[index00 + 0], imageData[index00 + 1], imageData[index00 + 2]);
    float3 c10 = (float3)(imageData[index10 + 0], imageData[index10 + 1], imageData[index10 + 2]);
    float3 c01 = (float3)(imageData[index01 + 0], imageData[index01 + 1], imageData[index01 + 2]);
    float3 c11 = (float3)(imageData[index11 + 0], imageData[index11 + 1], imageData[index11 + 2]);
    
    // Bilinear interpolation
    float3 c0 = mix(c00, c10, fx);
    float3 c1 = mix(c01, c11, fx);
    return mix(c0, c1, fy);
}

// Main kernel function
__kernel void sampleIntensity(
    __global const float* inputImage,
    __global float* outputSamples,
    __global const Parameters* params
) {
    int id = get_global_id(0);
    
    if (id >= params->sampleCount) {
        return;
    }
    
    // Calculate position along scan line
    float t = (float)id / (float)(params->sampleCount - 1);
    
    // Convert normalized coordinates to pixel coordinates
    float2 point1Pixel = (float2)(
        params->point1X * (float)(params->imageWidth),
        params->point1Y * (float)(params->imageHeight)
    );
    float2 point2Pixel = (float2)(
        params->point2X * (float)(params->imageWidth),
        params->point2Y * (float)(params->imageHeight)
    );
    
    // Interpolate position along line
    float2 position = mix(point1Pixel, point2Pixel, t);
    
    // Sample using bilinear interpolation
    float3 rgb = bilinearSample(
        inputImage,
        params->imageSize.x,
        params->imageSize.y,
        params->componentCount,
        position.x,
        position.y
    );
    
    // Write output (packed as RGBRGBRGB...)
    int outputIndex = id * 3;
    outputSamples[outputIndex + 0] = rgb.x;
    outputSamples[outputIndex + 1] = rgb.y;
    outputSamples[outputIndex + 2] = rgb.z;
}
