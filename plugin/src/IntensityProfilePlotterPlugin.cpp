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

#include <algorithm>
#include <cmath>
#include <cstring>

// Plugin entry point
static OfxStatus pluginLoad(OfxImageEffectHandle handle, OFX::PropertySet effectProps)
{
    return kOfxStatOK;
}

static OfxStatus pluginUnload(const OFX::PropertySet effectProps)
{
    return kOfxStatOK;
}

static OfxStatus pluginDescribe(OFX::ImageEffectDescriptor& desc)
{
    desc.setLabels("Intensity Profile Plotter", "Intensity Profile Plotter", "Intensity Profile Plotter");
    desc.setPluginGrouping("Colorist Tools");
    desc.setPluginDescription(
        "GPU-accelerated intensity profile visualization tool for analyzing color transforms. "
        "Provides interactive scan line selection with RGB curve plotting and LUT testing capabilities."
    );
    
    // Plugin version
    desc.setVersion(1, 0, 0);
    
    // Supported contexts
    desc.addSupportedContext(OFX::eContextFilter);
    desc.addSupportedContext(OFX::eContextGeneral);
    
    // Supported pixel depths
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);
    
    // Set render thread safety
    desc.setRenderThreadSafety(OFX::eRenderInstanceSafe);
    
    // Enable GPU rendering
    desc.setSupportsMultiResolution(true);
    desc.setSupportsTiles(true);
    desc.setTemporalClipAccess(false);
    desc.setRenderTwiceAlways(false);
    desc.setSupportsMultipleClipPARs(false);
    
    // Enable interact
    desc.setSupportsOverlays(true);
    desc.setOverlayInteractDescriptor(new IntensityProfilePlotterInteractDescriptor);
    
    return kOfxStatOK;
}

static OfxStatus pluginDescribeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
{
    // Source clip
    OFX::ClipDescriptor* srcClip = desc.defineClip(kOfxImageEffectSimpleSourceClipName);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGB);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(true);
    srcClip->setIsMask(false);
    srcClip->setOptional(false);
    
    // Auxiliary clip (optional)
    OFX::ClipDescriptor* auxClip = desc.defineClip("Auxiliary");
    auxClip->addSupportedComponent(OFX::ePixelComponentRGB);
    auxClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    auxClip->setTemporalClipAccess(false);
    auxClip->setSupportsTiles(true);
    auxClip->setIsMask(false);
    auxClip->setOptional(true);
    
    return kOfxStatOK;
}

static OfxStatus pluginCreateInstance(OfxImageEffectHandle handle)
{
    IntensityProfilePlotterPlugin* plugin = new IntensityProfilePlotterPlugin(handle);
    return plugin->getOfxStatus();
}

static OfxStatus pluginDestroyInstance(OfxImageEffectHandle handle)
{
    IntensityProfilePlotterPlugin* plugin = fetchPlugin<IntensityProfilePlotterPlugin>(handle);
    delete plugin;
    return kOfxStatOK;
}

// Plugin suite
static OfxPlugin plugin = {
    kOfxImageEffectPluginApi,
    1,
    "com.coloristtools.IntensityProfilePlotter",
    1,
    0,
    pluginLoad,
    pluginUnload,
    pluginDescribe,
    pluginDescribeInContext,
    pluginCreateInstance,
    pluginDestroyInstance
};

// Plugin registration
void OfxGetPlugin(int* pluginCount, OfxPlugin** plugins)
{
    *pluginCount = 1;
    *plugins = &plugin;
}

// Implementation
IntensityProfilePlotterPlugin::IntensityProfilePlotterPlugin(OfxImageEffectHandle handle)
    : OFX::ImageEffect(handle)
    , _srcClip(nullptr)
    , _auxClip(nullptr)
    , _point1Param(nullptr)
    , _point2Param(nullptr)
    , _dataSourceParam(nullptr)
    , _sampleCountParam(nullptr)
    , _plotHeightParam(nullptr)
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
    
    // Create interact
    _interact = new IntensityProfilePlotterInteract(this);
}

IntensityProfilePlotterPlugin::~IntensityProfilePlotterPlugin()
{
    delete _interact;
}

void IntensityProfilePlotterPlugin::setupClips()
{
    _srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
    _auxClip = fetchClip("Auxiliary");
}

void IntensityProfilePlotterPlugin::setupParameters()
{
    // Point 1 (normalized coordinates)
    _point1Param = fetchDouble2DParam("point1");
    if (!_point1Param) {
        _point1Param = createDouble2DParam("point1", "Point 1");
        _point1Param->setDefault(0.2, 0.5);
        _point1Param->setDisplayRange(0.0, 0.0, 1.0, 1.0);
        _point1Param->setDimensionLabels("X", "Y");
    }
    
    // Point 2 (normalized coordinates)
    _point2Param = fetchDouble2DParam("point2");
    if (!_point2Param) {
        _point2Param = createDouble2DParam("point2", "Point 2");
        _point2Param->setDefault(0.8, 0.5);
        _point2Param->setDisplayRange(0.0, 0.0, 1.0, 1.0);
        _point2Param->setDimensionLabels("X", "Y");
    }
    
    // Data source
    _dataSourceParam = fetchChoiceParam("dataSource");
    if (!_dataSourceParam) {
        _dataSourceParam = createChoiceParam("dataSource", "Data Source");
        _dataSourceParam->appendOption("Input Clip");
        _dataSourceParam->appendOption("Auxiliary Clip");
        _dataSourceParam->appendOption("Built-in Ramp (LUT Test)");
        _dataSourceParam->setDefault(0);
    }
    
    // Sample count
    _sampleCountParam = fetchIntParam("sampleCount");
    if (!_sampleCountParam) {
        _sampleCountParam = createIntParam("sampleCount", "Sample Count");
        _sampleCountParam->setDefault(512);
        _sampleCountParam->setDisplayRange(64, 2048);
        _sampleCountParam->setHint("Number of samples along the scan line");
    }
    
    // Plot height (normalized)
    _plotHeightParam = fetchDoubleParam("plotHeight");
    if (!_plotHeightParam) {
        _plotHeightParam = createDoubleParam("plotHeight", "Plot Height");
        _plotHeightParam->setDefault(0.3);
        _plotHeightParam->setDisplayRange(0.1, 0.8);
        _plotHeightParam->setHint("Height of the plot overlay (normalized)");
    }
    
    // Curve colors
    _redCurveColorParam = fetchRGBAColourDParam("redCurveColor");
    if (!_redCurveColorParam) {
        _redCurveColorParam = createRGBAColourDParam("redCurveColor", "Red Curve Color");
        _redCurveColorParam->setDefault(1.0, 0.0, 0.0, 1.0);
    }
    
    _greenCurveColorParam = fetchRGBAColourDParam("greenCurveColor");
    if (!_greenCurveColorParam) {
        _greenCurveColorParam = createRGBAColourDParam("greenCurveColor", "Green Curve Color");
        _greenCurveColorParam->setDefault(0.0, 1.0, 0.0, 1.0);
    }
    
    _blueCurveColorParam = fetchRGBAColourDParam("blueCurveColor");
    if (!_blueCurveColorParam) {
        _blueCurveColorParam = createRGBAColourDParam("blueCurveColor", "Blue Curve Color");
        _blueCurveColorParam->setDefault(0.0, 0.0, 1.0, 1.0);
    }
    
    // Show reference ramp
    _showReferenceRampParam = fetchBooleanParam("showReferenceRamp");
    if (!_showReferenceRampParam) {
        _showReferenceRampParam = createBooleanParam("showReferenceRamp", "Show Reference Ramp");
        _showReferenceRampParam->setDefault(true);
        _showReferenceRampParam->setHint("Display linear 0-1 grayscale ramp background");
    }
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
    // Output matches input
    clipPreferences.setOutputFrameVarying(_srcClip->getFrameVarying());
    clipPreferences.setOutputHasContinuousSamples(_srcClip->getHasContinuousSamples());
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
    OFX::Clip* outputClip = getOutputClip();
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
    
    int dataSource = _dataSourceParam->getValueAtTime(args.time);
    int sampleCount = _sampleCountParam->getValueAtTime(args.time);
    double plotHeight = _plotHeightParam->getValueAtTime(args.time);
    
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
    
    // Render plot overlay using DrawSuite
    OFX::DrawSuite* drawSuite = fetchDrawSuite();
    if (drawSuite && drawSuite->drawSuiteSupported()) {
        _plotter->renderPlot(
            drawSuite,
            outputImg,
            redSamples, greenSamples, blueSamples,
            redColor, greenColor, blueColor,
            plotHeight,
            showReferenceRamp,
            dstWidth, dstHeight
        );
    }
}
