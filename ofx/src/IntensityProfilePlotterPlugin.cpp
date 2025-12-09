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
    desc.setVersion(2, 0, 0, 13, "");
    
    // Supported contexts
    desc.addSupportedContext(OFX::eContextFilter);
    
    // Supported pixel depths
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);
    
    // Set render thread safety
    desc.setRenderThreadSafety(OFX::eRenderInstanceSafe);
    
#ifdef __APPLE__
    desc.setSupportsMetalRender(true);
#endif
    
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
    srcClip->addSupportedComponent(OFX::ePixelComponentRGB);
    srcClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    srcClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    srcClip->setTemporalClipAccess(false);
    srcClip->setSupportsTiles(true);
    srcClip->setIsMask(false);
    
    // Output clip (required for video effects)
    OFX::ClipDescriptor* outClip = desc.defineClip(kOfxImageEffectOutputClipName);
    outClip->addSupportedComponent(OFX::ePixelComponentRGB);
    outClip->addSupportedComponent(OFX::ePixelComponentRGBA);
    outClip->addSupportedComponent(OFX::ePixelComponentAlpha);
    outClip->setSupportsTiles(true);
    outClip->setIsMask(false);
    
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

    // White point mapping
    OFX::DoubleParamDescriptor* whitePointParam = desc.defineDoubleParam("whitePoint");
    whitePointParam->setLabel("White Point");
    whitePointParam->setDefault(1.0);
    whitePointParam->setDisplayRange(0.01, 10.0);
    whitePointParam->setHint("Input intensity mapped to graph value 1.0");
    whitePointParam->setAnimates(false);
    
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
    
    // Version info (read-only string)
    OFX::StringParamDescriptor* versionParam = desc.defineStringParam("_version");
    versionParam->setLabel("Version");
    versionParam->setDefault("2.0.0.16 (" __DATE__ " " __TIME__ ")");
    versionParam->setEvaluateOnChange(false);
    versionParam->setAnimates(false);
    
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
    static IntensityProfilePlotterPluginFactory p("com.coloristtools.intensityprofileplotter", 2, 0);
    ids.push_back(&p);
}
}
}

// Implementation
#include "IntensityProfilePlotterPlugin.h"
#include "IntensitySampler.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

// Define the plugin class
IntensityProfilePlotterPlugin::IntensityProfilePlotterPlugin(OfxImageEffectHandle handle)
    : OFX::ImageEffect(handle)
{
    // DO NOT call setupClips() or setupParameters() in constructor
    // OFX framework doesn't allow fetching clips/parameters during construction
    // They will be fetched on-demand during render
    
    // Initialize components with exception handling
    try {
        // _sampler = std::make_unique<IntensitySampler>();
        // _plotter = std::make_unique<ProfilePlotter>();
    } catch (...) {
        // If initialization fails, leave them null
        _sampler = nullptr;
        _plotter = nullptr;
    }
    
    // Create interact (will be created by descriptor when needed)
    // _interact = new IntensityProfilePlotterInteract(getOfxImageEffectHandle(), this);
}

IntensityProfilePlotterPlugin::~IntensityProfilePlotterPlugin()
{
    // delete _interact;
}

void IntensityProfilePlotterPlugin::setupClips()
{
    try {
        _srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
        _auxClip = nullptr; // Auxiliary clip not used
        _dstClip = fetchClip(kOfxImageEffectOutputClipName);
    } catch (...) {
        _srcClip = nullptr;
        _auxClip = nullptr;
        _dstClip = nullptr;
    }
}

void IntensityProfilePlotterPlugin::setupParameters()
{
    // Fetch parameters (they should already be defined in describeInContext)
    // Add null checks in case parameters aren't defined
    try {
        _point1Param = fetchDouble2DParam("point1");
        _point2Param = fetchDouble2DParam("point2");
        _dataSourceParam = fetchChoiceParam("dataSource");
        _sampleCountParam = fetchIntParam("sampleCount");
        _plotRectPosParam = fetchDouble2DParam("plotRectPos");
        _plotRectSizeParam = fetchDouble2DParam("plotRectSize");
        _whitePointParam = fetchDoubleParam("whitePoint");
        _lineWidthParam = fetchIntParam("lineWidth");
        _redCurveColorParam = fetchRGBAParam("redCurveColor");
        _greenCurveColorParam = fetchRGBAParam("greenCurveColor");
        _blueCurveColorParam = fetchRGBAParam("blueCurveColor");
        _showReferenceRampParam = fetchBooleanParam("showReferenceRamp");

        _versionParam = fetchStringParam("_version");
        if (_versionParam) {
            const std::string buildVersion = std::string("2.0.0.16 ") + __DATE__ + " " + __TIME__;
            _versionParam->setValue(buildVersion);
        }
    } catch (...) {}
}

bool IntensityProfilePlotterPlugin::getRegionOfDefinition(const OFX::RegionOfDefinitionArguments& args, 
                                                           OfxRectD& rod)
{
    // Output ROD matches input ROD
    try {
        OFX::Clip* srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
        if (srcClip) {
            rod = srcClip->getRegionOfDefinition(args.time);
            return true;
        }
    } catch (...) {}
    rod.x1 = rod.y1 = 0.0;
    rod.x2 = rod.y2 = 1.0;
    return false;
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
    // Always return false - we don't use identity
    return false;
}

void IntensityProfilePlotterPlugin::render(const OFX::RenderArguments& args)
{
    // Copy source to destination so the host sees the unmodified clip
    // We keep the overlay rendering in the interact only.
    if (!_srcClip || !_dstClip) {
        setupClips();
    }

    if (!_srcClip || !_dstClip) {
        return;
    }

    std::unique_ptr<OFX::Image> src(_srcClip->fetchImage(args.time));
    std::unique_ptr<OFX::Image> dst(_dstClip->fetchImage(args.time));
    if (!src || !dst) {
        return;
    }

    const OfxRectI& rw = args.renderWindow;
    int comps = 0;
    switch (dst->getPixelComponents()) {
    case OFX::ePixelComponentRGBA: comps = 4; break;
    case OFX::ePixelComponentRGB:  comps = 3; break;
    case OFX::ePixelComponentAlpha: comps = 1; break;
    default: return; // unsupported
    }

    // We declared float-only in describeInContext, so assume float pixels
    const size_t bytesPerPixel = comps * sizeof(float);
    const int width = rw.x2 - rw.x1;

    for (int y = rw.y1; y < rw.y2; ++y) {
        const float* s = reinterpret_cast<const float*>(src->getPixelAddress(rw.x1, y));
        float* d = reinterpret_cast<float*>(dst->getPixelAddress(rw.x1, y));
        if (!s || !d) {
            continue;
        }
        std::memcpy(d, s, static_cast<size_t>(width) * bytesPerPixel);
    }
}
