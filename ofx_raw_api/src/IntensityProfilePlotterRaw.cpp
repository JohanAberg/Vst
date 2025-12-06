// Intensity Profile Plotter - Raw OFX API Version
// This version uses the raw OFX API without the Support library
// Based on the Basic example from OpenFX SDK

#include "IntensityProfilePlotterRaw.h"

#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxMemory.h"
#include "ofxMultiThread.h"
#include "ofxProperty.h"
#include "ofxParam.h"

#include <cstring>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <vector>
#include <memory>
#include <cfloat> // For FLT_MAX, etc.
#include <fstream> // For logging

// Logging helper for debugging
static void logMsg(const char* msg) {
    std::ofstream log("C:/Users/aberg/Documents/Vst/debug_log.txt", std::ios_base::app);
    if (log.is_open()) {
        log << msg << std::endl;
    }
}

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <GL/gl.h>

#include "ofxInteract.h"

// Define missing constants if needed
#ifndef kOfxImagePropPixelDepth
#define kOfxImagePropPixelDepth "OfxImagePropPixelDepth"
#endif

#ifndef kOfxImagePropComponents
#define kOfxImagePropComponents "OfxImagePropComponents"
#endif

#ifndef kOfxImageComponentAlpha
#define kOfxImageComponentAlpha "OfxImageComponentAlpha"
#endif

#ifndef kOfxInteractPropEffect
#define kOfxInteractPropEffect "OfxInteractPropEffect"
#endif

#ifndef kOfxImageEffectPluginPropOverlayInteractV1
#define kOfxImageEffectPluginPropOverlayInteractV1 "OfxImageEffectPluginPropOverlayInteractV1"
#endif

#ifndef kOfxInteractPropSlaveToParam
#define kOfxInteractPropSlaveToParam "OfxInteractPropSlaveToParam"
#endif

#if defined __APPLE__ || defined __linux__ || defined __FreeBSD__
#  define EXPORT __attribute__((visibility("default")))
#elif defined _WIN32
#  define EXPORT OfxExport
#else
#  error Not building on your operating system quite yet
#endif

// Interact state
struct InteractData {
    int dragPoint; // 0=none, 1=point1, 2=point2, 3=both (line drag)
    OfxParamHandle point1Param;
    OfxParamHandle point2Param;
    double initialOffsetX; // Offset from click to point1 when dragging line
    double initialOffsetY;
};

// Global host and suite pointers
static OfxHost* gHost = nullptr;
static OfxImageEffectSuiteV1* gEffectSuite = nullptr;
static OfxPropertySuiteV1* gPropSuite = nullptr;
static OfxParameterSuiteV1* gParamSuite = nullptr;
static OfxMemorySuiteV1* gMemorySuite = nullptr;
static OfxMultiThreadSuiteV1* gThreadSuite = nullptr;
static OfxInteractSuiteV1* gInteractSuite = nullptr;

// Debug flag to check if interact is active
static bool gInteractActive = false;

// Plugin identifier
#define PLUGIN_IDENTIFIER "com.coloristtools.IntensityProfilePlotterV3"

// Forward declarations
static OfxStatus describe(OfxImageEffectHandle effect);
static OfxStatus describeInContext(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs);
static OfxStatus createInstance(OfxImageEffectHandle effect);
static OfxStatus destroyInstance(OfxImageEffectHandle effect);
static OfxStatus render(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs);
static OfxStatus getRegionOfDefinition(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs);
static OfxStatus interactMain(const char *action, const void *handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs);

// Set host function - called by OFX host to provide suite pointers
static void setHostFunc(OfxHost* host)
{
    gHost = host;
    gEffectSuite = (OfxImageEffectSuiteV1*)host->fetchSuite(host->host, kOfxImageEffectSuite, 1);
    gPropSuite = (OfxPropertySuiteV1*)host->fetchSuite(host->host, kOfxPropertySuite, 1);
    gParamSuite = (OfxParameterSuiteV1*)host->fetchSuite(host->host, kOfxParameterSuite, 1);
    gMemorySuite = (OfxMemorySuiteV1*)host->fetchSuite(host->host, kOfxMemorySuite, 1);
    gThreadSuite = (OfxMultiThreadSuiteV1*)host->fetchSuite(host->host, kOfxMultiThreadSuite, 1);
    gInteractSuite = (OfxInteractSuiteV1*)host->fetchSuite(host->host, kOfxInteractSuite, 1);
}

// Get instance data helper
static IntensityProfilePlotterInstanceData* getInstanceData(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle effectProps;
    gEffectSuite->getPropertySet(effect, &effectProps);
    
    IntensityProfilePlotterInstanceData* instanceData = nullptr;
    gPropSuite->propGetPointer(effectProps, kOfxPropInstanceData, 0, (void**)&instanceData);
    return instanceData;
}

// Calculate distance from point to line segment
static double pointToLineDistance(double px, double py, double x1, double y1, double x2, double y2)
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    double length2 = dx * dx + dy * dy;
    
    if (length2 < 1e-10) {
        // Line segment is actually a point
        return sqrt((px - x1) * (px - x1) + (py - y1) * (py - y1));
    }
    
    // Calculate parameter t for closest point on line segment
    double t = ((px - x1) * dx + (py - y1) * dy) / length2;
    
    // Clamp t to [0, 1] to stay on segment
    t = std::max(0.0, std::min(1.0, t));
    
    // Calculate closest point on segment
    double closestX = x1 + t * dx;
    double closestY = y1 + t * dy;
    
    // Return distance to closest point
    return sqrt((px - closestX) * (px - closestX) + (py - closestY) * (py - closestY));
}

// Describe the plugin
static OfxStatus describe(OfxImageEffectHandle effect)
{
    OfxPropertySetHandle effectProps;
    gEffectSuite->getPropertySet(effect, &effectProps);
    
    // Set plugin properties
    char* label = (char*)"Intensity Profile Plotter V3";
    char* grouping = (char*)"Colorist Tools";
    char* desc = (char*)"Intensity profile visualization tool";
    gPropSuite->propSetString(effectProps, kOfxPropLabel, 0, label);
    gPropSuite->propSetString(effectProps, kOfxImageEffectPluginPropGrouping, 0, grouping);
    gPropSuite->propSetString(effectProps, kOfxPropPluginDescription, 0, desc);
    
    // Supported contexts
    char* context1 = (char*)kOfxImageEffectContextFilter;
    char* context2 = (char*)kOfxImageEffectContextGeneral;
    gPropSuite->propSetString(effectProps, kOfxImageEffectPropSupportedContexts, 0, context1);
    gPropSuite->propSetString(effectProps, kOfxImageEffectPropSupportedContexts, 1, context2);
    
    // Supported pixel depths
    char* bitDepth = (char*)kOfxBitDepthFloat;
    gPropSuite->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 0, bitDepth);
    
    // Multi-resolution and tiles (set as strings or ints depending on API)
    gPropSuite->propSetInt(effectProps, kOfxImageEffectPropSupportsMultiResolution, 0, 1);
    gPropSuite->propSetInt(effectProps, kOfxImageEffectPropSupportsTiles, 0, 1);
    gPropSuite->propSetInt(effectProps, kOfxImageEffectPropTemporalClipAccess, 0, 0);
    
    // Register overlay interact
    gPropSuite->propSetPointer(effectProps, kOfxImageEffectPluginPropOverlayInteractV1, 0, (void*)interactMain);
    
    return kOfxStatOK;
}

// Describe in context - define clips and parameters
static OfxStatus describeInContext(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs)
{
    // Define clips
    OfxPropertySetHandle outputClipProps;
    gEffectSuite->clipDefine(effect, kOfxImageEffectOutputClipName, &outputClipProps);
    char* rgb = (char*)kOfxImageComponentRGB;
    char* rgba = (char*)kOfxImageComponentRGBA;
    gPropSuite->propSetString(outputClipProps, kOfxImageEffectPropSupportedComponents, 0, rgb);
    gPropSuite->propSetString(outputClipProps, kOfxImageEffectPropSupportedComponents, 1, rgba);
    
    OfxPropertySetHandle sourceClipProps;
    gEffectSuite->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &sourceClipProps);
    gPropSuite->propSetString(sourceClipProps, kOfxImageEffectPropSupportedComponents, 0, rgb);
    gPropSuite->propSetString(sourceClipProps, kOfxImageEffectPropSupportedComponents, 1, rgba);
    
    // Define parameters - using correct pattern from Basic example
    OfxParamSetHandle paramSet;
    gEffectSuite->getParamSet(effect, &paramSet);
    
    // Point 1 (Double2D) - add one parameter at a time
    OfxPropertySetHandle point1Props;
    OfxStatus stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble2D, "point1", &point1Props);
    if (stat == kOfxStatOK && point1Props) {
        gPropSuite->propSetString(point1Props, kOfxPropLabel, 0, (char*)"Point 1");
        double point1Default[2] = {0.2, 0.5};
        gPropSuite->propSetDoubleN(point1Props, kOfxParamPropDefault, 2, point1Default);
    }
    
    // Point 2 (Double2D)
    OfxPropertySetHandle point2Props;
    stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble2D, "point2", &point2Props);
    if (stat == kOfxStatOK && point2Props) {
        gPropSuite->propSetString(point2Props, kOfxPropLabel, 0, (char*)"Point 2");
        double point2Default[2] = {0.8, 0.5};
        gPropSuite->propSetDoubleN(point2Props, kOfxParamPropDefault, 2, point2Default);
    }
    
    // Data source (Choice)
    OfxPropertySetHandle dataSourceProps;
    stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeChoice, "dataSource", &dataSourceProps);
    if (stat == kOfxStatOK && dataSourceProps) {
        gPropSuite->propSetString(dataSourceProps, kOfxPropLabel, 0, (char*)"Data Source");
        gPropSuite->propSetString(dataSourceProps, kOfxParamPropChoiceOption, 0, (char*)"Input Clip");
        gPropSuite->propSetString(dataSourceProps, kOfxParamPropChoiceOption, 1, (char*)"Auxiliary Clip");
        gPropSuite->propSetString(dataSourceProps, kOfxParamPropChoiceOption, 2, (char*)"Built-in Ramp (LUT Test)");
        gPropSuite->propSetInt(dataSourceProps, kOfxParamPropDefault, 0, 0);
    }
    
    // Sample count (Integer)
    OfxPropertySetHandle sampleCountProps;
    stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeInteger, "sampleCount", &sampleCountProps);
    if (stat == kOfxStatOK && sampleCountProps) {
        gPropSuite->propSetString(sampleCountProps, kOfxPropLabel, 0, (char*)"Sample Count");
        gPropSuite->propSetInt(sampleCountProps, kOfxParamPropDefault, 0, 512);
        gPropSuite->propSetInt(sampleCountProps, kOfxParamPropMin, 0, 2);
        gPropSuite->propSetInt(sampleCountProps, kOfxParamPropMax, 0, 4096);
    }
    
    // Plot height (Double)
    OfxPropertySetHandle plotHeightProps;
    stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeDouble, "plotHeight", &plotHeightProps);
    if (stat == kOfxStatOK && plotHeightProps) {
        gPropSuite->propSetString(plotHeightProps, kOfxPropLabel, 0, (char*)"Plot Height");
        gPropSuite->propSetDouble(plotHeightProps, kOfxParamPropDefault, 0, 0.3);
        gPropSuite->propSetDouble(plotHeightProps, kOfxParamPropMin, 0, 0.0);
        gPropSuite->propSetDouble(plotHeightProps, kOfxParamPropMax, 0, 1.0);
    }
    
    // Red curve color (RGBA)
    OfxPropertySetHandle redColorProps;
    stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeRGBA, "redCurveColor", &redColorProps);
    if (stat == kOfxStatOK && redColorProps) {
        gPropSuite->propSetString(redColorProps, kOfxPropLabel, 0, (char*)"Red Curve Color");
        double redColorDefault[4] = {1.0, 0.0, 0.0, 1.0};
        gPropSuite->propSetDoubleN(redColorProps, kOfxParamPropDefault, 4, redColorDefault);
    }
    
    // Green curve color (RGBA)
    OfxPropertySetHandle greenColorProps;
    stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeRGBA, "greenCurveColor", &greenColorProps);
    if (stat == kOfxStatOK && greenColorProps) {
        gPropSuite->propSetString(greenColorProps, kOfxPropLabel, 0, (char*)"Green Curve Color");
        double greenColorDefault[4] = {0.0, 1.0, 0.0, 1.0};
        gPropSuite->propSetDoubleN(greenColorProps, kOfxParamPropDefault, 4, greenColorDefault);
    }
    
    // Blue curve color (RGBA)
    OfxPropertySetHandle blueColorProps;
    stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeRGBA, "blueCurveColor", &blueColorProps);
    if (stat == kOfxStatOK && blueColorProps) {
        gPropSuite->propSetString(blueColorProps, kOfxPropLabel, 0, (char*)"Blue Curve Color");
        double blueColorDefault[4] = {0.0, 0.0, 1.0, 1.0};
        gPropSuite->propSetDoubleN(blueColorProps, kOfxParamPropDefault, 4, blueColorDefault);
    }
    
    // Show reference ramp (Boolean)
    OfxPropertySetHandle showRampProps;
    stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeBoolean, "showReferenceRamp", &showRampProps);
    if (stat == kOfxStatOK && showRampProps) {
        gPropSuite->propSetString(showRampProps, kOfxPropLabel, 0, (char*)"Show Reference Ramp");
        gPropSuite->propSetInt(showRampProps, kOfxParamPropDefault, 0, 1);
    }
    
    // Build Version (String, Read-only)
    OfxPropertySetHandle buildProps;
    stat = gParamSuite->paramDefine(paramSet, kOfxParamTypeString, "Build", &buildProps);
    if (stat == kOfxStatOK && buildProps) {
        gPropSuite->propSetString(buildProps, kOfxPropLabel, 0, (char*)"Build Version");
        char buildStr[256];
        // Combine Date and Time
        const char* date = __DATE__;
        const char* time = __TIME__;
        // Simple manual concatenation to avoid snprintf issues if library missing
        // "Dec  6 2025 12:00:00"
        int i = 0;
        for(int j=0; date[j]; j++) buildStr[i++] = date[j];
        buildStr[i++] = ' ';
        for(int j=0; time[j]; j++) buildStr[i++] = time[j];
        buildStr[i] = '\0';
        
        gPropSuite->propSetString(buildProps, kOfxParamPropDefault, 0, buildStr);
        gPropSuite->propSetInt(buildProps, kOfxParamPropEnabled, 0, 0); // Read-only (disabled)
    }

    return kOfxStatOK;
}

// Create instance - with parameter handles
static OfxStatus createInstance(OfxImageEffectHandle effect)
{
    // Allocate instance data
    IntensityProfilePlotterInstanceData* instanceData = new IntensityProfilePlotterInstanceData();
    memset(instanceData, 0, sizeof(IntensityProfilePlotterInstanceData));
    
    // Store instance data
    OfxPropertySetHandle effectProps;
    gEffectSuite->getPropertySet(effect, &effectProps);
    gPropSuite->propSetPointer(effectProps, kOfxPropInstanceData, 0, instanceData);
    
    // Get clip handles
    gEffectSuite->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &instanceData->sourceClip, nullptr);
    gEffectSuite->clipGetHandle(effect, kOfxImageEffectOutputClipName, &instanceData->outputClip, nullptr);
    
    // Get parameter handles
    OfxParamSetHandle paramSet;
    gEffectSuite->getParamSet(effect, &paramSet);
    gParamSuite->paramGetHandle(paramSet, "point1", &instanceData->point1Param, nullptr);
    gParamSuite->paramGetHandle(paramSet, "point2", &instanceData->point2Param, nullptr);
    gParamSuite->paramGetHandle(paramSet, "dataSource", &instanceData->dataSourceParam, nullptr);
    gParamSuite->paramGetHandle(paramSet, "sampleCount", &instanceData->sampleCountParam, nullptr);
    gParamSuite->paramGetHandle(paramSet, "plotHeight", &instanceData->plotHeightParam, nullptr);
    gParamSuite->paramGetHandle(paramSet, "redCurveColor", &instanceData->redCurveColorParam, nullptr);
    gParamSuite->paramGetHandle(paramSet, "greenCurveColor", &instanceData->greenCurveColorParam, nullptr);
    gParamSuite->paramGetHandle(paramSet, "blueCurveColor", &instanceData->blueCurveColorParam, nullptr);
    gParamSuite->paramGetHandle(paramSet, "showReferenceRamp", &instanceData->showReferenceRampParam, nullptr);
    
    return kOfxStatOK;
}

// Destroy instance
static OfxStatus destroyInstance(OfxImageEffectHandle effect)
{
    IntensityProfilePlotterInstanceData* instanceData = getInstanceData(effect);
    if (instanceData) {
        // Clean up sampler and plotter if they were created
        // For now, they're not allocated, so just delete instance data
        delete instanceData;
        
        // Clear instance data pointer
        OfxPropertySetHandle effectProps;
        gEffectSuite->getPropertySet(effect, &effectProps);
        gPropSuite->propSetPointer(effectProps, kOfxPropInstanceData, 0, nullptr);
    }
    return kOfxStatOK;
}

// Get region of definition
static OfxStatus getRegionOfDefinition(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs)
{
    IntensityProfilePlotterInstanceData* instanceData = getInstanceData(effect);
    if (!instanceData) {
        return kOfxStatErrBadHandle;
    }
    
    // Get time
    double time;
    gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
    
    // Get source clip's RoD
    OfxRectD rod;
    gEffectSuite->clipGetRegionOfDefinition(instanceData->sourceClip, time, &rod);
    
    // Set output RoD (same as input)
    gPropSuite->propSetDoubleN(outArgs, kOfxImageEffectPropRegionOfDefinition, 4, &rod.x1);
    
    return kOfxStatOK;
}

// Helper function to draw a line on the output image
static void drawLine(
    float* imageData,
    int imageWidth,
    int imageHeight,
    int rowBytes,
    int componentCount,
    int x1, int y1, int x2, int y2,
    float r, float g, float b, float a)
{
    // Simple line drawing using Bresenham-like algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    int x = x1;
    int y = y1;
    
    while (true) {
        // Draw pixel if within bounds
        if (x >= 0 && x < imageWidth && y >= 0 && y < imageHeight) {
            float* pixel = (float*)((char*)imageData + y * rowBytes) + x * componentCount;
            
            // Alpha blend
            float alpha = a;
            float invAlpha = 1.0f - alpha;
            pixel[0] = r * alpha + pixel[0] * invAlpha;
            pixel[1] = g * alpha + pixel[1] * invAlpha;
            pixel[2] = b * alpha + pixel[2] * invAlpha;
        }
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

// Helper to get pixel color from different formats
static void getPixel(const void* data, int x, int y, int width, int componentCount, int bitDepth, float& r, float& g, float& b) {
    if (bitDepth == 32) { // Float
        const float* ptr = (const float*)data;
        int idx = (y * width + x) * componentCount;
        r = ptr[idx + 0];
        g = ptr[idx + 1];
        b = ptr[idx + 2];
    } else if (bitDepth == 8) { // 8-bit integer
        const unsigned char* ptr = (const unsigned char*)data;
        int idx = (y * width + x) * componentCount;
        r = ptr[idx + 0] / 255.0f;
        g = ptr[idx + 1] / 255.0f;
        b = ptr[idx + 2] / 255.0f;
    } else if (bitDepth == 16) { // 16-bit integer
        const unsigned short* ptr = (const unsigned short*)data;
        int idx = (y * width + x) * componentCount;
        r = ptr[idx + 0] / 65535.0f;
        g = ptr[idx + 1] / 65535.0f;
        b = ptr[idx + 2] / 65535.0f;
    } else {
        r = g = b = 0.0f;
    }
}

// Helper function for bilinear sampling
static void bilinearSample(
    const void* imageData,
    int imageWidth,
    int imageHeight,
    int componentCount,
    int bitDepth,
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
    
    float r00, g00, b00, r10, g10, b10, r01, g01, b01, r11, g11, b11;
    getPixel(imageData, x0, y0, imageWidth, componentCount, bitDepth, r00, g00, b00);
    getPixel(imageData, x1, y0, imageWidth, componentCount, bitDepth, r10, g10, b10);
    getPixel(imageData, x0, y1, imageWidth, componentCount, bitDepth, r01, g01, b01);
    getPixel(imageData, x1, y1, imageWidth, componentCount, bitDepth, r11, g11, b11);
    
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

// Helper to draw a character (simple vector font)
static void drawChar(
    float* imageData, int width, int height, int rowBytes, int components,
    int x, int y, char c, int scale, float r, float g, float b)
{
    // Simple 4x6 grid (scale determines size)
    // Segments:
    // 0: Top (0,0)-(4,0)
    // 1: Mid (0,3)-(4,3)
    // 2: Bot (0,6)-(4,6)
    // 3: TL (0,0)-(0,3)
    // 4: BL (0,3)-(0,6)
    // 5: TR (4,0)-(4,3)
    // 6: BR (4,3)-(4,6)
    
    // Map digit to segments mask
    // 0: 0,2,3,4,5,6 (not 1)
    // 1: 5,6
    // 2: 0,1,2,5,4
    // 3: 0,1,2,5,6
    // 4: 1,3,5,6
    // 5: 0,1,2,3,6
    // 6: 0,1,2,3,4,6
    // 7: 0,5,6
    // 8: all
    // 9: 0,1,2,3,5,6
    // :: special
    
    int mask = 0;
    if (c == '0') mask = 0b1111101;
    else if (c == '1') mask = 0b1100000;
    else if (c == '2') mask = 0b0110111;
    else if (c == '3') mask = 0b1110101;
    else if (c == '4') mask = 0b1101010;
    else if (c == '5') mask = 0b1011101;
    else if (c == '6') mask = 0b1011111;
    else if (c == '7') mask = 0b1100001;
    else if (c == '8') mask = 0b1111111;
    else if (c == '9') mask = 0b1111101; // Same as 0 but with mid
    
    // Correction for 9: add mid (bit 1)
    if (c == '9') mask |= 0b0000010;
    
    if (c == ':') {
        // Draw two dots
        int s = scale;
        drawLine(imageData, width, height, rowBytes, components, x+2*s, y+2*s, x+2*s, y+2*s+s, r, g, b, 1.0f);
        drawLine(imageData, width, height, rowBytes, components, x+2*s, y+4*s, x+2*s, y+4*s+s, r, g, b, 1.0f);
        return;
    }
    
    // Coordinates
    int w = 4 * scale;
    int h = 6 * scale;
    int h2 = 3 * scale;
    
    if (mask & 1) drawLine(imageData, width, height, rowBytes, components, x, y, x+w, y, r, g, b, 1.0f); // Top
    if (mask & 2) drawLine(imageData, width, height, rowBytes, components, x, y+h2, x+w, y+h2, r, g, b, 1.0f); // Mid
    if (mask & 4) drawLine(imageData, width, height, rowBytes, components, x, y+h, x+w, y+h, r, g, b, 1.0f); // Bot
    if (mask & 8) drawLine(imageData, width, height, rowBytes, components, x, y, x, y+h2, r, g, b, 1.0f); // TL
    if (mask & 16) drawLine(imageData, width, height, rowBytes, components, x, y+h2, x, y+h, r, g, b, 1.0f); // BL
    if (mask & 32) drawLine(imageData, width, height, rowBytes, components, x+w, y, x+w, y+h2, r, g, b, 1.0f); // TR
    if (mask & 64) drawLine(imageData, width, height, rowBytes, components, x+w, y+h2, x+w, y+h, r, g, b, 1.0f); // BR
}

// Draw time string
static void drawTime(float* imageData, int width, int height, int rowBytes, int components) {
    const char* timeStr = __TIME__; // "HH:MM:SS"
    int x = 20;
    int y = 20;
    int scale = 3;
    int spacing = 6 * scale;
    
    for (int i = 0; timeStr[i]; i++) {
        drawChar(imageData, width, height, rowBytes, components, x, y, timeStr[i], scale, 1.0f, 1.0f, 0.0f); // Yellow
        x += spacing;
    }
}

// Render function with intensity sampling
static OfxStatus render(OfxImageEffectHandle effect, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs)
{
    // Reset interact active flag - will be set to true if drawInteract is called
    gInteractActive = false;
    
    IntensityProfilePlotterInstanceData* instanceData = getInstanceData(effect);
    if (!instanceData) {
        return kOfxStatErrBadHandle;
    }
    
    // Get render arguments
    double time;
    gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
    
    // Get parameter values
    double point1[2], point2[2];
    gParamSuite->paramGetValueAtTime(instanceData->point1Param, time, &point1[0], &point1[1]);
    gParamSuite->paramGetValueAtTime(instanceData->point2Param, time, &point2[0], &point2[1]);
    
    int dataSource = 0;
    gParamSuite->paramGetValueAtTime(instanceData->dataSourceParam, time, &dataSource);
    
    int sampleCount = 512;
    gParamSuite->paramGetValueAtTime(instanceData->sampleCountParam, time, &sampleCount);
    sampleCount = std::max(2, std::min(4096, sampleCount)); // Clamp to valid range
    
    // Get images - must be released before returning
    OfxPropertySetHandle outputImgProps = nullptr;
    OfxPropertySetHandle sourceImgProps = nullptr;
    
    OfxStatus status = gEffectSuite->clipGetImage(instanceData->outputClip, time, nullptr, &outputImgProps);
    if (status != kOfxStatOK || !outputImgProps) {
        return kOfxStatErrBadHandle;
    }
    
    // Get source clip image (input to this node)
    // Note: In DaVinci Resolve, the source clip is the input to the node, which includes
    // all effects from previous nodes, but NOT effects on the same node that process before this OFX.
    // To sample grades on the same node, we sample from the output clip instead (see sampling code below).
    status = gEffectSuite->clipGetImage(instanceData->sourceClip, time, nullptr, &sourceImgProps);
    if (status != kOfxStatOK || !sourceImgProps) {
        gEffectSuite->clipReleaseImage(outputImgProps);
        return kOfxStatErrBadHandle;
    }
    
    // Get image data
    void* outputData = nullptr;
    void* sourceData = nullptr;
    gPropSuite->propGetPointer(outputImgProps, kOfxImagePropData, 0, &outputData);
    gPropSuite->propGetPointer(sourceImgProps, kOfxImagePropData, 0, &sourceData);
    
    if (outputData && sourceData) {
        // Get image properties - use simple approach like RawMinimalPlugin
        int outputRowBytes, sourceRowBytes;
        gPropSuite->propGetInt(outputImgProps, kOfxImagePropRowBytes, 0, &outputRowBytes);
        gPropSuite->propGetInt(sourceImgProps, kOfxImagePropRowBytes, 0, &sourceRowBytes);
        
        // Get bounds - use propGetIntN to get all 4 values
        int outputBounds[4], sourceBounds[4];
        gPropSuite->propGetIntN(outputImgProps, kOfxImagePropBounds, 4, outputBounds);
        gPropSuite->propGetIntN(sourceImgProps, kOfxImagePropBounds, 4, sourceBounds);
        
        int sourceWidth = sourceBounds[2] - sourceBounds[0];
        int sourceHeight = sourceBounds[3] - sourceBounds[1];
        int outputWidth = outputBounds[2] - outputBounds[0];
        int outputHeight = outputBounds[3] - outputBounds[1];
        
        // Get component count early (needed for sampling and drawing)
        char* srcComponents = nullptr;
        gPropSuite->propGetString(sourceImgProps, kOfxImagePropComponents, 0, &srcComponents);
        int componentCount = 4; // Default RGBA
        if (srcComponents) {
            if (strcmp(srcComponents, kOfxImageComponentRGBA) == 0) componentCount = 4;
            else if (strcmp(srcComponents, kOfxImageComponentRGB) == 0) componentCount = 3;
            else if (strcmp(srcComponents, kOfxImageComponentAlpha) == 0) componentCount = 1;
        }
        
        // Always generate samples (built-in ramp works for any format)
        std::vector<float> redSamples, greenSamples, blueSamples;
        redSamples.reserve(sampleCount);
        greenSamples.reserve(sampleCount);
        blueSamples.reserve(sampleCount);
        
        if (dataSource == 2) {
            // Built-in ramp: generate linear 0-1 signal - ALWAYS works
            for (int i = 0; i < sampleCount; ++i) {
                float t = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
                redSamples.push_back(t);
                greenSamples.push_back(t);
                blueSamples.push_back(t);
            }
        } else {
            // Sample from source image - support 8-bit, 16-bit, and 32-bit float
            int srcBitDepth = 0;
            gPropSuite->propGetInt(sourceImgProps, kOfxImagePropPixelDepth, 0, &srcBitDepth);
            
            // Default to Float RGBA if properties fail (common in Resolve)
            if (srcBitDepth == 0) srcBitDepth = 32;
            
            // Always try to sample, using defaults if necessary
            // Convert normalized coordinates to pixel coordinates (relative to bounds)
            double px1 = sourceBounds[0] + point1[0] * sourceWidth;
            double py1 = sourceBounds[1] + point1[1] * sourceHeight;
            double px2 = sourceBounds[0] + point2[0] * sourceWidth;
            double py2 = sourceBounds[1] + point2[1] * sourceHeight;
            
            // Sample along the line
            for (int i = 0; i < sampleCount; ++i) {
                double t = static_cast<double>(i) / static_cast<double>(sampleCount - 1);
                
                // Interpolate position along line
                double x = px1 + t * (px2 - px1);
                double y = py1 + t * (py2 - py1);
                
                // Clamp to image bounds
                x = std::max(static_cast<double>(sourceBounds[0]), std::min(static_cast<double>(sourceBounds[2] - 1), x));
                y = std::max(static_cast<double>(sourceBounds[1]), std::min(static_cast<double>(sourceBounds[3] - 1), y));
                
                // Convert to relative coordinates for bilinear sampling
                double relX = x - sourceBounds[0];
                double relY = y - sourceBounds[1];
                
                // Bilinear sample
                float red, green, blue;
                bilinearSample(sourceData, sourceWidth, sourceHeight, componentCount, srcBitDepth, relX, relY, red, green, blue);
                
                redSamples.push_back(red);
                greenSamples.push_back(green);
                blueSamples.push_back(blue);
            }
        }
        
        // Copy source to output first - simple row-by-row copy like RawMinimalPlugin
        // Do this BEFORE checking format, to ensure we always have something to display
        int minHeight = (outputHeight < sourceHeight) ? outputHeight : sourceHeight;
        int minRowBytes = (outputRowBytes < sourceRowBytes) ? outputRowBytes : sourceRowBytes;
        
        for (int y = 0; y < minHeight; y++) {
            memcpy((char*)outputData + y * outputRowBytes,
                   (char*)sourceData + y * sourceRowBytes,
                   minRowBytes);
        }
        
        // Draw Build Time UNCONDITIONALLY
        float* outputPixels = (float*)outputData;
        drawTime(outputPixels, outputWidth, outputHeight, outputRowBytes, componentCount);
        
        // Render plot overlay - ALWAYS render if we have samples
        // Note: The line and handles are NOT drawn here - they are only drawn in drawInteract
        // when the OFX Control overlay is enabled
        if (!redSamples.empty() && outputHeight > 20 && outputWidth > 20) {
            // Reuse componentCount from outer scope (already determined above)

            // Get plot parameters
            double plotHeight = 0.3;
            gParamSuite->paramGetValueAtTime(instanceData->plotHeightParam, time, &plotHeight);
            
            double redColor[4] = {1.0, 0.0, 0.0, 1.0};
            double greenColor[4] = {0.0, 1.0, 0.0, 1.0};
            double blueColor[4] = {0.0, 0.0, 1.0, 1.0};
            gParamSuite->paramGetValueAtTime(instanceData->redCurveColorParam, time, &redColor[0], &redColor[1], &redColor[2], &redColor[3]);
            gParamSuite->paramGetValueAtTime(instanceData->greenCurveColorParam, time, &greenColor[0], &greenColor[1], &greenColor[2], &greenColor[3]);
            gParamSuite->paramGetValueAtTime(instanceData->blueCurveColorParam, time, &blueColor[0], &blueColor[1], &blueColor[2], &blueColor[3]);
            
            bool showReferenceRamp = true;
            gParamSuite->paramGetValueAtTime(instanceData->showReferenceRampParam, time, &showReferenceRamp);
            
            // Calculate plot area (bottom of image)
            int plotAreaHeight = static_cast<int>(outputHeight * plotHeight);
            if (plotAreaHeight < 20) plotAreaHeight = 20; // Minimum visible height
            if (plotAreaHeight > outputHeight / 2) plotAreaHeight = outputHeight / 2; // Max half height
            int plotY = outputHeight - plotAreaHeight;
            int plotWidth = outputWidth;
            
            // Draw background for plot area (semi-transparent dark)
            for (int y = plotY; y < outputHeight; y++) {
                float* line = (float*)((char*)outputData + y * outputRowBytes);
                for (int x = 0; x < outputWidth; x++) {
                    int idx = x * componentCount;
                    // Draw dark background (0.1 = very dark, almost black)
                    line[idx + 0] = 0.1f;
                    line[idx + 1] = 0.1f;
                    line[idx + 2] = 0.1f;
                }
            }
            
            // Draw reference ramp if enabled
            if (showReferenceRamp && plotAreaHeight > 20) {
                int rampWidth = 50; // Make it wider to be clearly visible
                int rampX = plotWidth - rampWidth - 10;
                for (int y = 0; y < plotAreaHeight; y++) {
                    float value = 1.0f - (static_cast<float>(y) / static_cast<float>(plotAreaHeight - 1));
                    float* line = (float*)((char*)outputData + (plotY + y) * outputRowBytes);
                    for (int x = rampX; x < rampX + rampWidth; x++) {
                        int idx = x * componentCount;
                        line[idx + 0] = value;
                        line[idx + 1] = value;
                        line[idx + 2] = value;
                    }
                }
            }
            
            // Draw Build Time
            drawTime(outputPixels, outputWidth, outputHeight, outputRowBytes, componentCount);
            
            // Draw curves - make lines thicker for visibility
            int numSamples = static_cast<int>(redSamples.size());
            if (numSamples > 1 && plotAreaHeight > 10) {
                // Draw thicker lines by drawing multiple times with slight offsets
                // Draw red curve - draw multiple times for thickness
                for (int offset = -1; offset <= 1; offset++) {
                    for (int i = 0; i < numSamples - 1; i++) {
                        int x1 = (i * plotWidth) / (numSamples - 1);
                        int x2 = ((i + 1) * plotWidth) / (numSamples - 1);
                        float y1 = redSamples[i];
                        float y2 = redSamples[i + 1];
                        
                        int py1 = plotY + static_cast<int>((1.0f - y1) * (plotAreaHeight - 1)) + offset;
                        int py2 = plotY + static_cast<int>((1.0f - y2) * (plotAreaHeight - 1)) + offset;
                        
                        // Draw line segment
                        drawLine(outputPixels, outputWidth, outputHeight, outputRowBytes, componentCount,
                                x1, py1, x2, py2, 
                                static_cast<float>(redColor[0]), static_cast<float>(redColor[1]), 
                                static_cast<float>(redColor[2]), static_cast<float>(redColor[3]));
                    }
                }
                    
                // Draw green curve - draw multiple times for thickness
                for (int offset = -1; offset <= 1; offset++) {
                    for (int i = 0; i < numSamples - 1; i++) {
                        int x1 = (i * plotWidth) / (numSamples - 1);
                        int x2 = ((i + 1) * plotWidth) / (numSamples - 1);
                        float y1 = greenSamples[i];
                        float y2 = greenSamples[i + 1];
                        
                        int py1 = plotY + static_cast<int>((1.0f - y1) * (plotAreaHeight - 1)) + offset;
                        int py2 = plotY + static_cast<int>((1.0f - y2) * (plotAreaHeight - 1)) + offset;
                        
                        drawLine(outputPixels, outputWidth, outputHeight, outputRowBytes, componentCount,
                                x1, py1, x2, py2, 
                                static_cast<float>(greenColor[0]), static_cast<float>(greenColor[1]), 
                                static_cast<float>(greenColor[2]), static_cast<float>(greenColor[3]));
                    }
                }
                    
                // Draw blue curve - draw multiple times for thickness
                for (int offset = -1; offset <= 1; offset++) {
                    for (int i = 0; i < numSamples - 1; i++) {
                        int x1 = (i * plotWidth) / (numSamples - 1);
                        int x2 = ((i + 1) * plotWidth) / (numSamples - 1);
                        float y1 = blueSamples[i];
                        float y2 = blueSamples[i + 1];
                        
                        int py1 = plotY + static_cast<int>((1.0f - y1) * (plotAreaHeight - 1)) + offset;
                        int py2 = plotY + static_cast<int>((1.0f - y2) * (plotAreaHeight - 1)) + offset;
                        
                        drawLine(outputPixels, outputWidth, outputHeight, outputRowBytes, componentCount,
                                x1, py1, x2, py2, 
                                static_cast<float>(blueColor[0]), static_cast<float>(blueColor[1]), 
                                static_cast<float>(blueColor[2]), static_cast<float>(blueColor[3]));
                    }
                }
            }
        } // End of if (!redSamples.empty())
    } // End of if (outputData && sourceData)
    
    // CRITICAL: Release images before returning
    if (sourceImgProps) {
        gEffectSuite->clipReleaseImage(sourceImgProps);
    }
    if (outputImgProps) {
        gEffectSuite->clipReleaseImage(outputImgProps);
    }
    
    return kOfxStatOK;
}

// Draw interact overlay
// Note: This is only called when the OFX Control option is enabled in the viewer
// The host (DaVinci Resolve) manages when the overlay is active
static OfxStatus drawInteract(InteractData* data, IntensityProfilePlotterInstanceData* instanceData, OfxTime time, OfxPropertySetHandle args) {
    // Mark interact as active
    gInteractActive = true;

    // Get parameters
    double p1[2], p2[2];
    gParamSuite->paramGetValueAtTime(data->point1Param, time, &p1[0], &p1[1]);
    gParamSuite->paramGetValueAtTime(data->point2Param, time, &p2[0], &p2[1]);
    
    // Get Source Clip RoD to scale coordinates
    OfxRectD rod;
    if (gEffectSuite->clipGetRegionOfDefinition(instanceData->sourceClip, time, &rod) != kOfxStatOK) {
        return kOfxStatFailed;
    }
    double width = rod.x2 - rod.x1;
    double height = rod.y2 - rod.y1;
    if (width <= 0) width = 1.0;
    if (height <= 0) height = 1.0;
    
    // Convert normalized to canonical
    double x1 = rod.x1 + p1[0] * width;
    double y1 = rod.y1 + p1[1] * height;
    double x2 = rod.x1 + p2[0] * width;
    double y2 = rod.y1 + p2[1] * height;
    
    // Get viewport size and pixel scale
    double pixelScale[2] = {1.0, 1.0};
    gPropSuite->propGetDoubleN(args, kOfxInteractPropPixelScale, 2, pixelScale);
    
    // Save state
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_COLOR_BUFFER_BIT);
    
    // Setup state for robust drawing
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Set line width (thicker for visibility)
    glLineWidth(4.0f);
    
    // Draw line (Bright Yellow with full alpha)
    glColor4f(1.0f, 1.0f, 0.0f, 1.0f); 
    
    glBegin(GL_LINES);
    glVertex2d(x1, y1);
    glVertex2d(x2, y2);
    glEnd();
    
    // Draw handles (FILLED SQUARES) - Make them MUCH BIGGER
    // Use at least 15 pixels regardless of pixel scale
    double handleSize = 15.0; // Fixed 15 pixels for maximum visibility
    
    // P1 Handle (Bright Green with full alpha)
    glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glVertex2d(x1 - handleSize, y1 - handleSize);
    glVertex2d(x1 + handleSize, y1 - handleSize);
    glVertex2d(x1 + handleSize, y1 + handleSize);
    glVertex2d(x1 - handleSize, y1 + handleSize);
    glEnd();
    
    // P1 Border (Black for contrast)
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x1 - handleSize, y1 - handleSize);
    glVertex2d(x1 + handleSize, y1 - handleSize);
    glVertex2d(x1 + handleSize, y1 + handleSize);
    glVertex2d(x1 - handleSize, y1 + handleSize);
    glEnd();
    
    // P2 Handle (Bright Red with full alpha)
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
    glLineWidth(4.0f);
    glBegin(GL_QUADS);
    glVertex2d(x2 - handleSize, y2 - handleSize);
    glVertex2d(x2 + handleSize, y2 - handleSize);
    glVertex2d(x2 + handleSize, y2 + handleSize);
    glVertex2d(x2 - handleSize, y2 + handleSize);
    glEnd();
    
    // P2 Border (Black for contrast)
    glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x2 - handleSize, y2 - handleSize);
    glVertex2d(x2 + handleSize, y2 - handleSize);
    glVertex2d(x2 + handleSize, y2 + handleSize);
    glVertex2d(x2 - handleSize, y2 + handleSize);
    glEnd();
    
    // Restore state
    glPopAttrib();
    
    logMsg("drawInteract: END - drew Yellow line and Green/Red squares");
    return kOfxStatOK;
}

// Interact main entry point
static OfxStatus interactMain(const char *action, const void *handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs) {
    // Handle Describe action explicitly
    if (strcmp(action, kOfxActionDescribe) == 0) {
        OfxInteractHandle interactDescriptor = (OfxInteractHandle)handle;
        OfxPropertySetHandle props = nullptr;
        if (gInteractSuite) {
            OfxStatus stat = gInteractSuite->interactGetPropertySet(interactDescriptor, &props);
            if (stat == kOfxStatOK && props) {
                // Tell the host that this interact depends on these parameters
                gPropSuite->propSetString(props, kOfxInteractPropSlaveToParam, 0, "point1");
                gPropSuite->propSetString(props, kOfxInteractPropSlaveToParam, 1, "point2");
            }
        }
        return kOfxStatOK;
    }

    OfxInteractHandle interact = (OfxInteractHandle)handle;
    OfxPropertySetHandle props = nullptr;
    OfxStatus stat = gInteractSuite->interactGetPropertySet(interact, &props);
    if (stat != kOfxStatOK || !props) return kOfxStatErrBadHandle;

    if (strcmp(action, kOfxActionCreateInstance) == 0) {
        InteractData* interactData = new InteractData();
        interactData->dragPoint = 0;
        interactData->point1Param = nullptr;
        interactData->point2Param = nullptr;
        interactData->initialOffsetX = 0.0;
        interactData->initialOffsetY = 0.0;
        
        // Get effect handle from interact properties
        // Use kOfxPropEffectInstance (not kOfxInteractPropEffect) - this is what the Support library uses
        OfxImageEffectHandle effect = nullptr;
        gPropSuite->propGetPointer(props, kOfxPropEffectInstance, 0, (void**)&effect);
        if (effect) {
            // Get parameter handles directly from the effect
            OfxParamSetHandle paramSet;
            gEffectSuite->getParamSet(effect, &paramSet);
            gParamSuite->paramGetHandle(paramSet, "point1", &interactData->point1Param, nullptr);
            gParamSuite->paramGetHandle(paramSet, "point2", &interactData->point2Param, nullptr);
        }
        
        gPropSuite->propSetPointer(props, kOfxPropInstanceData, 0, interactData);
        return kOfxStatOK;
    }
    
    // Interact state
    InteractData* interactData = nullptr;
    gPropSuite->propGetPointer(props, kOfxPropInstanceData, 0, (void**)&interactData);
    
    if (strcmp(action, kOfxActionDestroyInstance) == 0) {
        if (interactData) delete interactData;
        gPropSuite->propSetPointer(props, kOfxPropInstanceData, 0, nullptr);
        return kOfxStatOK;
    }
    
    if (!interactData) return kOfxStatErrBadHandle;
    
    // Get effect handle from props
    OfxImageEffectHandle effect = nullptr;
    gPropSuite->propGetPointer(props, kOfxPropEffectInstance, 0, (void**)&effect);
    if (!effect) {
        return kOfxStatErrBadHandle;
    }
    
    IntensityProfilePlotterInstanceData* instanceData = getInstanceData(effect);
    if (!instanceData) {
        return kOfxStatErrBadHandle;
    }
    
    // Ensure parameter handles are set (should be set in CreateInstance, but check anyway)
    if (!interactData->point1Param || !interactData->point2Param) {
        interactData->point1Param = instanceData->point1Param;
        interactData->point2Param = instanceData->point2Param;
        if (!interactData->point1Param || !interactData->point2Param) {
            return kOfxStatErrBadHandle;
        }
    }
    
    if (strcmp(action, kOfxInteractActionDraw) == 0) {
        OfxTime time;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        return drawInteract(interactData, instanceData, time, inArgs);
    }
    
    if (strcmp(action, kOfxInteractActionPenDown) == 0) {
        OfxTime time;
        gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
        double penX, penY;
        gPropSuite->propGetDouble(inArgs, kOfxInteractPropPenPosition, 0, &penX);
        gPropSuite->propGetDouble(inArgs, kOfxInteractPropPenPosition, 1, &penY);
        
        // Get pixel scale for hit testing
        double pixelScale[2] = {1.0, 1.0};
        gPropSuite->propGetDoubleN(inArgs, kOfxInteractPropPixelScale, 2, pixelScale);
        
        // Get scale info for coordinate conversion
        OfxRectD rod;
        gEffectSuite->clipGetRegionOfDefinition(instanceData->sourceClip, time, &rod);
        double width = rod.x2 - rod.x1;
        double height = rod.y2 - rod.y1;
        if (width <= 0) width = 1.0;
        if (height <= 0) height = 1.0;
        
        // Get parameter values (normalized 0..1)
        double p1[2], p2[2];
        gParamSuite->paramGetValueAtTime(interactData->point1Param, time, &p1[0], &p1[1]);
        gParamSuite->paramGetValueAtTime(interactData->point2Param, time, &p2[0], &p2[1]);
        
        // Convert normalized params to pixel coordinates (canonical)
        double px1 = rod.x1 + p1[0] * width;
        double py1 = rod.y1 + p1[1] * height;
        double px2 = rod.x1 + p2[0] * width;
        double py2 = rod.y1 + p2[1] * height;
        
        // Hit test in pixel coordinates
        double handleThreshold = 15.0; // pixels for handles
        double lineThreshold = 10.0; // pixels for line
        
        double dist1 = sqrt(pow(penX - px1, 2) + pow(penY - py1, 2));
        double dist2 = sqrt(pow(penX - px2, 2) + pow(penY - py2, 2));
        double distToLine = pointToLineDistance(penX, penY, px1, py1, px2, py2);
        
        // Test handles first (they have priority)
        if (dist1 < handleThreshold) {
            interactData->dragPoint = 1;
            return kOfxStatOK;
        } else if (dist2 < handleThreshold) {
            interactData->dragPoint = 2;
            return kOfxStatOK;
        } else if (distToLine < lineThreshold) {
            // Hit the line - drag both points together
            interactData->dragPoint = 3;
            // Store offset from click to point1
            interactData->initialOffsetX = penX - px1;
            interactData->initialOffsetY = penY - py1;
            return kOfxStatOK;
        }
        return kOfxStatReplyDefault;
    }
    
    if (strcmp(action, kOfxInteractActionPenMotion) == 0) {
        if (interactData->dragPoint != 0) {
            double penX, penY;
            gPropSuite->propGetDouble(inArgs, kOfxInteractPropPenPosition, 0, &penX);
            gPropSuite->propGetDouble(inArgs, kOfxInteractPropPenPosition, 1, &penY);
            
            // Get scale info
            OfxTime time;
            gPropSuite->propGetDouble(inArgs, kOfxPropTime, 0, &time);
            OfxRectD rod;
            gEffectSuite->clipGetRegionOfDefinition(instanceData->sourceClip, time, &rod);
            double width = rod.x2 - rod.x1;
            double height = rod.y2 - rod.y1;
            if (width <= 0) width = 1.0;
            if (height <= 0) height = 1.0;
            
            if (interactData->dragPoint == 3) {
                // Dragging both points together - maintain relative positions
                // Calculate new point1 position based on offset
                double newPx1 = penX - interactData->initialOffsetX;
                double newPy1 = penY - interactData->initialOffsetY;
                
                // Get current point positions to calculate offset between them
                double p1[2], p2[2];
                gParamSuite->paramGetValueAtTime(interactData->point1Param, time, &p1[0], &p1[1]);
                gParamSuite->paramGetValueAtTime(interactData->point2Param, time, &p2[0], &p2[1]);
                
                double px1 = rod.x1 + p1[0] * width;
                double py1 = rod.y1 + p1[1] * height;
                double px2 = rod.x1 + p2[0] * width;
                double py2 = rod.y1 + p2[1] * height;
                
                // Calculate offset between points
                double offsetX = px2 - px1;
                double offsetY = py2 - py1;
                
                // Calculate new point2 position
                double newPx2 = newPx1 + offsetX;
                double newPy2 = newPy1 + offsetY;
                
                // Convert to normalized coordinates
                double normX1 = (newPx1 - rod.x1) / width;
                double normY1 = (newPy1 - rod.y1) / height;
                double normX2 = (newPx2 - rod.x1) / width;
                double normY2 = (newPy2 - rod.y1) / height;
                
                // Clamp to 0..1
                normX1 = std::max(0.0, std::min(1.0, normX1));
                normY1 = std::max(0.0, std::min(1.0, normY1));
                normX2 = std::max(0.0, std::min(1.0, normX2));
                normY2 = std::max(0.0, std::min(1.0, normY2));
                
                // Update both points
                gParamSuite->paramSetValue(interactData->point1Param, normX1, normY1);
                gParamSuite->paramSetValue(interactData->point2Param, normX2, normY2);
            } else {
                // Dragging single point
                double normX = (penX - rod.x1) / width;
                double normY = (penY - rod.y1) / height;
                
                // Clamp to 0..1
                normX = std::max(0.0, std::min(1.0, normX));
                normY = std::max(0.0, std::min(1.0, normY));
                
                if (interactData->dragPoint == 1) {
                    gParamSuite->paramSetValue(interactData->point1Param, normX, normY);
                } else if (interactData->dragPoint == 2) {
                    gParamSuite->paramSetValue(interactData->point2Param, normX, normY);
                }
            }
            
            OfxInteractHandle interact = (OfxInteractHandle)handle;
            gInteractSuite->interactRedraw(interact);
            return kOfxStatOK;
        }
        return kOfxStatReplyDefault;
    }
    
    if (strcmp(action, kOfxInteractActionPenUp) == 0) {
        if (interactData->dragPoint != 0) {
            interactData->dragPoint = 0;
            return kOfxStatOK;
        }
        return kOfxStatReplyDefault;
    }
    
    return kOfxStatReplyDefault;
}

// Plugin main entry point
static OfxStatus pluginMain(const char* action, const void* handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs)
{
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;
    OfxStatus status = kOfxStatReplyDefault;
    
    if (strcmp(action, kOfxActionLoad) == 0) {
        status = kOfxStatOK;
    }
    else if (strcmp(action, kOfxActionUnload) == 0) {
        status = kOfxStatOK;
    }
    else if (strcmp(action, kOfxActionDescribe) == 0) {
        status = describe(effect);
    }
    else if (strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
        status = describeInContext(effect, inArgs);
    }
    else if (strcmp(action, kOfxActionCreateInstance) == 0) {
        status = createInstance(effect);
    }
    else if (strcmp(action, kOfxActionDestroyInstance) == 0) {
        status = destroyInstance(effect);
    }
    else if (strcmp(action, kOfxImageEffectActionGetRegionOfDefinition) == 0) {
        status = getRegionOfDefinition(effect, inArgs, outArgs);
    }
    else if (strcmp(action, kOfxImageEffectActionRender) == 0) {
        status = render(effect, inArgs, outArgs);
    }
    
    return status;
}

// Plugin structure
static OfxPlugin gPlugin = {
    kOfxImageEffectPluginApi,
    1,
    PLUGIN_IDENTIFIER,
    1,
    0,
    setHostFunc,
    pluginMain
};

// Entry points
EXPORT OfxPlugin* OfxGetPlugin(int nth)
{
    if (nth == 0) {
        return &gPlugin;
    }
    return nullptr;
}

EXPORT int OfxGetNumberOfPlugins(void)
{
    return 1;
}

