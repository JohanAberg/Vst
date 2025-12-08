#include "IntensityProfilePlotterPlugin.h"
#include "IntensityProfilePlotterInteract.h"
#include "IntensitySampler.h"
#include "ProfilePlotter.h"
#include "GPURenderer.h"
#include "CPURenderer.h"

#include "ofxImageEffect.h"
#include "ofxParam.h"
#include "ofxMemory.h"
#include "ofxDrawSuite.h"
#include "ofxsImageEffect.h"
#include "ofxsParam.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// Interact factory
typedef OFX::DefaultEffectOverlayDescriptor<IntensityProfilePlotterInteractDescriptor, IntensityProfilePlotterInteract> IntensityProfilePlotterInteractFactory;

// Plugin factory
mDeclarePluginFactory(IntensityProfilePlotterPluginFactory, {}, {});

void IntensityProfilePlotterPluginFactory::describe(OFX::ImageEffectDescriptor& desc)
{
    desc.setLabels("Intensity Profile Plotter", "Intensity Profile Plotter", "Intensity Profile Plotter");
    desc.setPluginGrouping("Colorist Tools");
    desc.setPluginDescription(
        "GPU-accelerated intensity profile visualization tool for analyzing color transforms. "
        "Provides interactive scan line selection with RGB curve plotting and LUT testing capabilities."
    );
    
    // Plugin version (major, minor, micro, build, label)
    desc.setVersion(1, 0, 0, 0, "");
    
    // Supported contexts - only Filter for video effects
    desc.addSupportedContext(OFX::eContextFilter);
    
    // Supported pixel depths
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);
    
    // Set render thread safety
    desc.setRenderThreadSafety(OFX::eRenderInstanceSafe);
    
    // Standard flags
    desc.setSingleInstance(false);
    desc.setHostFrameThreading(false);
    desc.setSupportsMultiResolution(true);
    desc.setSupportsTiles(true);
    desc.setTemporalClipAccess(false);
    desc.setRenderTwiceAlways(false);
    desc.setSupportsMultipleClipPARs(false);
}

void IntensityProfilePlotterPluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
    // Source clip
    OFX::ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(true);
    srcClip->setIsMask(false);
    
    // Output clip (required for video effects)
    OFX::ClipDescriptor* outClip = desc.defineClip(kOfxImageEffectOutputClipName);
    outClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    outClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    outClip->setSupportsTiles(true);
    
    // Auxiliary clip (optional)
    OFX::ClipDescriptor* auxClip = desc.defineClip("Auxiliary");
    auxClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    auxClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    auxClip->setTemporalClipAccess(false);
    auxClip->setSupportsTiles(true);
    auxClip->setIsMask(false);
    auxClip->setOptional(true);
    
    // Define parameters (ImageEffectDescriptor inherits from ParamSetDescriptor)
    // Point 1
    OFX::Double2DParamDescriptor* point1Param = desc.defineDouble2DParam("point1");
    point1Param->setLabel("Point 1");
    point1Param->setDefault(0.2, 0.5);
    point1Param->setDisplayRange(0.0, 0.0, 1.0, 1.0);
    point1Param->setDimensionLabels("X", "Y");
    point1Param->setAnimates(false);
    
    // Point 2
    OFX::Double2DParamDescriptor* point2Param = desc.defineDouble2DParam("point2");
    point2Param->setLabel("Point 2");
    point2Param->setDefault(0.8, 0.5);
    point2Param->setDisplayRange(0.0, 0.0, 1.0, 1.0);
    point2Param->setDimensionLabels("X", "Y");
    point2Param->setAnimates(false);
    
    // Data source
    OFX::ChoiceParamDescriptor* dataSourceParam = desc.defineChoiceParam("dataSource");
    dataSourceParam->setLabel("Data Source");
    dataSourceParam->appendOption("Input Clip");
    dataSourceParam->appendOption("Auxiliary Clip");
    dataSourceParam->appendOption("Built-in Ramp (LUT Test)");
    dataSourceParam->setDefault(0);
    dataSourceParam->setAnimates(false);
    
    // Sample count
    OFX::IntParamDescriptor* sampleCountParam = desc.defineIntParam("sampleCount");
    sampleCountParam->setLabel("Sample Count");
    sampleCountParam->setDefault(512);
    sampleCountParam->setDisplayRange(64, 2048);
    sampleCountParam->setHint("Number of samples along the scan line");
    sampleCountParam->setAnimates(false);
    
    // Plot height
    OFX::DoubleParamDescriptor* plotHeightParam = desc.defineDoubleParam("plotHeight");
    plotHeightParam->setLabel("Plot Height");
    plotHeightParam->setDefault(0.3);
    plotHeightParam->setDisplayRange(0.1, 0.8);
    plotHeightParam->setHint("Height of the plot overlay (normalized)");
    plotHeightParam->setAnimates(false);

    // Plot rectangle position (top-left) and size (normalized)
    OFX::Double2DParamDescriptor* plotRectPosParam = desc.defineDouble2DParam("plotRectPos");
    plotRectPosParam->setLabel("Plot Rect Position");
    plotRectPosParam->setDefault(0.05, 0.05);
    plotRectPosParam->setDisplayRange(0.0, 0.0, 1.0, 1.0);
    plotRectPosParam->setHint("Top-left normalized position of the plot rectangle");
    plotRectPosParam->setAnimates(false);

    OFX::Double2DParamDescriptor* plotRectSizeParam = desc.defineDouble2DParam("plotRectSize");
    plotRectSizeParam->setLabel("Plot Rect Size");
    plotRectSizeParam->setDefault(0.3, 0.2);
    plotRectSizeParam->setDisplayRange(0.05, 0.05, 1.0, 1.0);
    plotRectSizeParam->setHint("Width and height of the plot rectangle (normalized)");
    plotRectSizeParam->setAnimates(false);
    
    // Line width
    OFX::IntParamDescriptor* lineWidthParam = desc.defineIntParam("lineWidth");
    lineWidthParam->setLabel("Line Width");
    lineWidthParam->setDefault(2);
    lineWidthParam->setDisplayRange(1, 10);
    lineWidthParam->setHint("Width of the intensity curve lines in pixels");
    lineWidthParam->setAnimates(false);
    
    // Curve colors
    OFX::RGBAParamDescriptor* redColorParam = desc.defineRGBAParam("redCurveColor");
    redColorParam->setLabel("Red Curve Color");
    redColorParam->setDefault(1.0, 0.0, 0.0, 1.0);
    redColorParam->setAnimates(false);
    
    OFX::RGBAParamDescriptor* greenColorParam = desc.defineRGBAParam("greenCurveColor");
    greenColorParam->setLabel("Green Curve Color");
    greenColorParam->setDefault(0.0, 1.0, 0.0, 1.0);
    greenColorParam->setAnimates(false);
    
    OFX::RGBAParamDescriptor* blueColorParam = desc.defineRGBAParam("blueCurveColor");
    blueColorParam->setLabel("Blue Curve Color");
    blueColorParam->setDefault(0.0, 0.0, 1.0, 1.0);
    blueColorParam->setAnimates(false);
    
    // Show reference ramp
    OFX::BooleanParamDescriptor* showRampParam = desc.defineBooleanParam("showReferenceRamp");
    showRampParam->setLabel("Show Reference Ramp");
    showRampParam->setDefault(true);
    showRampParam->setHint("Display linear 0-1 grayscale ramp background");
    showRampParam->setAnimates(false);
    
    // Set up overlay interact
    desc.setOverlayInteractDescriptor(new OFX::DefaultEffectOverlayDescriptor<IntensityProfilePlotterInteractDescriptor, IntensityProfilePlotterInteract>());
}

OFX::ImageEffect* IntensityProfilePlotterPluginFactory::createInstance(OfxImageEffectHandle handle, OFX::ContextEnum context)
{
    return new IntensityProfilePlotterPlugin(handle);
}

// Plugin registration
namespace OFX {
namespace Plugin {
void getPluginIDs(OFX::PluginFactoryArray &ids)
{
    static IntensityProfilePlotterPluginFactory p("com.coloristtools.IntensityProfilePlotter", 1, 0);
    ids.push_back(&p);
}
}
}

// Implementation
IntensityProfilePlotterPlugin::IntensityProfilePlotterPlugin(OfxImageEffectHandle handle)
    : ImageEffect(handle)
    , _srcClip(nullptr)
    , _dstClip(nullptr)
    , _auxClip(nullptr)
    , _point1Param(nullptr)
    , _point2Param(nullptr)
    , _dataSourceParam(nullptr)
    , _sampleCountParam(nullptr)
    , _plotHeightParam(nullptr)
    , _plotRectPosParam(nullptr)
    , _plotRectSizeParam(nullptr)
    , _lineWidthParam(nullptr)
    , _redCurveColorParam(nullptr)
    , _greenCurveColorParam(nullptr)
    , _blueCurveColorParam(nullptr)
    , _showReferenceRampParam(nullptr)
    , _interact(nullptr)
{
    setupClips();
    setupParameters();
    
    // Initialize components
    _sampler = std::make_unique<IntensitySampler>();
    _plotter = std::make_unique<ProfilePlotter>();
    
    // Create interact (will be created by descriptor when needed)
    // _interact = new IntensityProfilePlotterInteract(getOfxImageEffectHandle(), this);
}

IntensityProfilePlotterPlugin::~IntensityProfilePlotterPlugin()
{
    delete _interact;
}

void IntensityProfilePlotterPlugin::setupClips()
{
    _srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
    _auxClip = fetchClip("Auxiliary");
    _dstClip = fetchClip(kOfxImageEffectOutputClipName);
}

void IntensityProfilePlotterPlugin::setupParameters()
{
    // Fetch parameters (they should already be defined in describeInContext)
    _point1Param = fetchDouble2DParam("point1");
    _point2Param = fetchDouble2DParam("point2");
    _dataSourceParam = fetchChoiceParam("dataSource");
    _sampleCountParam = fetchIntParam("sampleCount");
    _plotHeightParam = fetchDoubleParam("plotHeight");
    _plotRectPosParam = fetchDouble2DParam("plotRectPos");
    _plotRectSizeParam = fetchDouble2DParam("plotRectSize");
    _lineWidthParam = fetchIntParam("lineWidth");
    _redCurveColorParam = fetchRGBAParam("redCurveColor");
    _greenCurveColorParam = fetchRGBAParam("greenCurveColor");
    _blueCurveColorParam = fetchRGBAParam("blueCurveColor");
    _showReferenceRampParam = fetchBooleanParam("showReferenceRamp");
}

bool IntensityProfilePlotterPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments& args, 
                                                           OfxRectD& rod)
{
    // Output ROD matches input ROD
    rod = _srcClip->getRegionOfDefinition(args.time);
    return true;
}

void IntensityProfilePlotterPlugin::getClipPreferences(OFX::ClipPreferencesSetter& clipPreferences)
{
    // Output matches input - use default behavior
    // clipPreferences.setOutputFrameVarying(_srcClip->getFrameVarying());
    // clipPreferences.setOutputHasContinousSamples(_srcClip->hasContinuousSamples());
}

bool IntensityProfilePlotterPlugin::isIdentity(const OFX::IsIdentityArguments& args, 
                                               OFX::Clip*& identityClip, double& identityTime)
{
    // Never identity - we always render the overlay
    return false;
}

void IntensityProfilePlotterPlugin::render(const OFX::RenderArguments& args)
{
    // Get output image
    OFX::Clip* outputClip = fetchClip(kOfxImageEffectOutputClipName);
    OFX::Image* outputImg = outputClip->fetchImage(args.time);
    if (!outputImg || outputImg->getRenderScale().x != args.renderScale.x) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to fetch output image");
        return;
    }
    
    // Get source image
    OFX::Image* srcImg = _srcClip->fetchImage(args.time);
    if (!srcImg) {
        setPersistentMessage(OFX::Message::eMessageError, "", "Failed to fetch source image");
        return;
    }
    
    // Get parameters
    double point1[2], point2[2];
    _point1Param->getValueAtTime(args.time, point1[0], point1[1]);
    _point2Param->getValueAtTime(args.time, point2[0], point2[1]);
    
    int dataSource;
    _dataSourceParam->getValueAtTime(args.time, dataSource);
    int sampleCount;
    _sampleCountParam->getValueAtTime(args.time, sampleCount);
    double plotHeight;
    _plotHeightParam->getValueAtTime(args.time, plotHeight);
    double plotRectPos[2];
    _plotRectPosParam->getValueAtTime(args.time, plotRectPos[0], plotRectPos[1]);
    double plotRectSize[2];
    _plotRectSizeParam->getValueAtTime(args.time, plotRectSize[0], plotRectSize[1]);
    int lineWidth;
    _lineWidthParam->getValueAtTime(args.time, lineWidth);
    
    double redColor[4], greenColor[4], blueColor[4];
    _redCurveColorParam->getValueAtTime(args.time, redColor[0], redColor[1], redColor[2], redColor[3]);
    _greenCurveColorParam->getValueAtTime(args.time, greenColor[0], greenColor[1], greenColor[2], greenColor[3]);
    _blueCurveColorParam->getValueAtTime(args.time, blueColor[0], blueColor[1], blueColor[2], blueColor[3]);
    
    bool showReferenceRamp = _showReferenceRampParam->getValueAtTime(args.time);
    
    // Get image dimensions
    OfxRectI srcBounds = srcImg->getBounds();
    int srcWidth = srcBounds.x2 - srcBounds.x1;
    int srcHeight = srcBounds.y2 - srcBounds.y1;
    
    OfxRectI dstBounds = outputImg->getBounds();
    int dstWidth = dstBounds.x2 - dstBounds.x1;
    int dstHeight = dstBounds.y2 - dstBounds.y1;
    
    // Copy source to output first
    if (srcImg != outputImg) {
        OFX::BitDepthEnum srcBitDepth = srcImg->getPixelDepth();
        OFX::PixelComponentEnum srcComponents = srcImg->getPixelComponents();
        
        if (srcBitDepth == OFX::eBitDepthFloat && 
            (srcComponents == OFX::ePixelComponentRGB || srcComponents == OFX::ePixelComponentRGBA)) {
            
            float* srcData = (float*)srcImg->getPixelData();
            float* dstData = (float*)outputImg->getPixelData();
            
            size_t pixelCount = dstWidth * dstHeight;
            size_t componentCount = (srcComponents == OFX::ePixelComponentRGBA) ? 4 : 3;
            
            memcpy(dstData, srcData, pixelCount * componentCount * sizeof(float));
        }
    }
    
    // Sample intensity data
    std::vector<float> redSamples, greenSamples, blueSamples;
    
    if (dataSource == 2) {
        // Built-in ramp: generate linear 0-1 signal
        for (int i = 0; i < sampleCount; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
            redSamples.push_back(t);
            greenSamples.push_back(t);
            blueSamples.push_back(t);
        }
    } else {
        // Sample from clip
        OFX::Image* sampleImg = (dataSource == 1 && _auxClip->isConnected()) ? 
                                 _auxClip->fetchImage(args.time) : srcImg;
        
        if (sampleImg) {
            _sampler->sampleIntensity(
                sampleImg,
                point1, point2,
                sampleCount,
                srcWidth, srcHeight,
                redSamples, greenSamples, blueSamples
            );
        }
    }
    
    // Render plot overlay directly into the output buffer inside a shaded rectangle
    if (!redSamples.empty() && !greenSamples.empty() && !blueSamples.empty()) {
        float* dstData = (float*)outputImg->getPixelData();
        int nComponents = (outputImg->getPixelComponents() == OFX::ePixelComponentRGBA) ? 4 : 3;
        int rowBytes = outputImg->getRowBytes();

        // Compute rectangle bounds in pixels (clamped to frame)
        int rectX = static_cast<int>(plotRectPos[0] * dstWidth);
        int rectY = static_cast<int>(plotRectPos[1] * dstHeight);
        int rectW = static_cast<int>(plotRectSize[0] * dstWidth);
        int rectH = static_cast<int>(plotRectSize[1] * dstHeight);
        rectX = std::max(0, std::min(rectX, dstWidth - 1));
        rectY = std::max(0, std::min(rectY, dstHeight - 1));
        rectW = std::max(10, std::min(rectW, dstWidth - rectX));
        rectH = std::max(10, std::min(rectH, dstHeight - rectY));

        // Shade background (simple alpha blend toward dark gray)
        const float shadeR = 0.1f, shadeG = 0.1f, shadeB = 0.1f, shadeA = 0.35f;
        for (int y = rectY; y < rectY + rectH; ++y) {
            for (int x = rectX; x < rectX + rectW; ++x) {
                int offset = (y * dstWidth + x) * nComponents;
                dstData[offset + 0] = dstData[offset + 0] * (1.0f - shadeA) + shadeR * shadeA;
                dstData[offset + 1] = dstData[offset + 1] * (1.0f - shadeA) + shadeG * shadeA;
                dstData[offset + 2] = dstData[offset + 2] * (1.0f - shadeA) + shadeB * shadeA;
                if (nComponents == 4) dstData[offset + 3] = 1.0f;
            }
        }

        // Plot height within the rectangle (scaled by plotHeight parameter)
        int plotHeightPx = static_cast<int>(rectH * plotHeight);
        plotHeightPx = std::max(1, std::min(plotHeightPx, rectH));
        int yBase = rectY + rectH - 1;

        // Draw the intensity curves mapped into the rectangle
        for (int i = 0; i < sampleCount - 1; ++i) {
            float x1 = static_cast<float>(i) / static_cast<float>(sampleCount - 1);
            float x2 = static_cast<float>(i + 1) / static_cast<float>(sampleCount - 1);

            int px1 = rectX + static_cast<int>(x1 * (rectW - 1));
            int px2 = rectX + static_cast<int>(x2 * (rectW - 1));

            // Draw red curve (values mapped to vertical range inside rect)
            int py1_r = yBase - static_cast<int>(redSamples[i] * plotHeightPx);
            int py2_r = yBase - static_cast<int>(redSamples[i + 1] * plotHeightPx);
            drawLine(dstData, dstWidth, dstHeight, nComponents, rowBytes,
                     px1, py1_r, px2, py2_r, redColor[0], redColor[1], redColor[2], lineWidth,
                     rectX, rectY, rectW, rectH);

            // Draw green curve
            int py1_g = yBase - static_cast<int>(greenSamples[i] * plotHeightPx);
            int py2_g = yBase - static_cast<int>(greenSamples[i + 1] * plotHeightPx);
            drawLine(dstData, dstWidth, dstHeight, nComponents, rowBytes,
                     px1, py1_g, px2, py2_g, greenColor[0], greenColor[1], greenColor[2], lineWidth,
                     rectX, rectY, rectW, rectH);

            // Draw blue curve
            int py1_b = yBase - static_cast<int>(blueSamples[i] * plotHeightPx);
            int py2_b = yBase - static_cast<int>(blueSamples[i + 1] * plotHeightPx);
            drawLine(dstData, dstWidth, dstHeight, nComponents, rowBytes,
                     px1, py1_b, px2, py2_b, blueColor[0], blueColor[1], blueColor[2], lineWidth,
                     rectX, rectY, rectW, rectH);
        }
    }
}

void IntensityProfilePlotterPlugin::drawLine(float* buffer, int width, int height, int nComp, int rowBytes,
                                              int x1, int y1, int x2, int y2, 
                                              float r, float g, float b, int lineWidth,
                                              int clipX, int clipY, int clipW, int clipH)
{
    // Helper function to set a pixel
    auto setPixel = [&](int px, int py) {
        if (px >= clipX && px < clipX + clipW && py >= clipY && py < clipY + clipH &&
            px >= 0 && px < width && py >= 0 && py < height) {
            int offset = (py * width + px) * nComp;
            buffer[offset + 0] = r;
            buffer[offset + 1] = g;
            buffer[offset + 2] = b;
            if (nComp == 4) buffer[offset + 3] = 1.0f;
        }
    };
    
    // Calculate line angle for perpendicular offset
    int dx = x2 - x1;
    int dy = y2 - y1;
    float length = sqrtf(static_cast<float>(dx * dx + dy * dy));
    if (length < 0.001f) {
        // Degenerate line, just draw a point with width
        int halfWidth = lineWidth / 2;
        for (int wy = -halfWidth; wy <= halfWidth; ++wy) {
            for (int wx = -halfWidth; wx <= halfWidth; ++wx) {
                setPixel(x1 + wx, y1 + wy);
            }
        }
        return;
    }
    
    // Normalized perpendicular vector
    float perpX = -dy / length;
    float perpY = dx / length;
    
    // Draw multiple parallel lines to create thickness
    int halfWidth = lineWidth / 2;
    for (int w = -halfWidth; w <= halfWidth; ++w) {
        int offsetX1 = static_cast<int>(x1 + perpX * w);
        int offsetY1 = static_cast<int>(y1 + perpY * w);
        int offsetX2 = static_cast<int>(x2 + perpX * w);
        int offsetY2 = static_cast<int>(y2 + perpY * w);
        
        // Bresenham line drawing for this parallel line
        int adx = abs(offsetX2 - offsetX1);
        int ady = abs(offsetY2 - offsetY1);
        int sx = (offsetX1 < offsetX2) ? 1 : -1;
        int sy = (offsetY1 < offsetY2) ? 1 : -1;
        int err = adx - ady;
        
        int x = offsetX1, y = offsetY1;
        
        while (true) {
            setPixel(x, y);
            
            if (x == offsetX2 && y == offsetY2) break;
            
            int e2 = 2 * err;
            if (e2 > -ady) {
                err -= ady;
                x += sx;
            }
            if (e2 < adx) {
                err += adx;
                y += sy;
            }
        }
    }
}