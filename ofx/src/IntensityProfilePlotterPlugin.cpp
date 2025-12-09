#include "IntensityProfilePlotterPlugin.h"
#include "IntensityProfilePlotterInteract.h"
#include "IntensitySampler.h"
#include "ProfilePlotter.h"
#include "GPURenderer.h"
#include "CPURenderer.h"

// Cached renderer name for overlay display
static const char* s_cachedRendererName = nullptr;

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
    desc.setVersion(2, 0, 0, 21, "");
    
    // Supported contexts
    desc.addSupportedContext(OFX::eContextFilter);
    
    // Supported pixel depths
    desc.addSupportedBitDepth(OFX::eBitDepthFloat);
    
    // Set render thread safety - plugin is fully thread-safe
    desc.setRenderThreadSafety(OFX::eRenderFullySafe);
    desc.setHostFrameThreading(true);  // Enable host-managed multithreading for better performance
    
#ifdef __APPLE__
    desc.setSupportsMetalRender(true);
#endif
    
    // Standard flags
    desc.setSingleInstance(false);
    desc.setSupportsMultiResolution(true);
    desc.setSupportsTiles(true);
    desc.setTemporalClipAccess(false);  // No temporal dependencies - implies no sequential rendering needed
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
    
    // Enable plot overlay
    OFX::BooleanParamDescriptor* enablePlotParam = desc.defineBooleanParam("enablePlot");
    enablePlotParam->setLabel("Enable Plot");
    enablePlotParam->setDefault(true);
    enablePlotParam->setHint("Enable/disable the intensity plot visualization");
    enablePlotParam->setAnimates(false);
    
    // Backend selection (Auto, OpenCL, CPU)
    OFX::ChoiceParamDescriptor* backendParam = desc.defineChoiceParam("backend");
    backendParam->setLabel("Backend");
    backendParam->appendOption("Auto");
    backendParam->appendOption("OpenCL");
    backendParam->appendOption("CPU");
    backendParam->setDefault(0); // Auto
    backendParam->setHint("Select processing backend: Auto (Metal/OpenCL/CPU), OpenCL (force GPU), or CPU only");
    backendParam->setAnimates(false);

    // Rectangle shade value
    OFX::DoubleParamDescriptor* rectShadeParam = desc.defineDoubleParam("rectShade");
    rectShadeParam->setLabel("Rectangle Shade");
    rectShadeParam->setDefault(0.0);
    rectShadeParam->setRange(0.0, 1.0);
    rectShadeParam->setDisplayRange(0.0, 1.0);
    rectShadeParam->setHint("Multiply pixels under the plot rectangle (0=black, 1=unchanged)." );
    rectShadeParam->setAnimates(true);
    
    // Renderer status (read-only string showing which backend is active)
    OFX::StringParamDescriptor* rendererParam = desc.defineStringParam("_renderer");
    rendererParam->setLabel("Renderer");
    rendererParam->setDefault("Initializing...");
    rendererParam->setHint("Active rendering backend: Metal (macOS GPU), OpenCL (cross-platform GPU), or CPU fallback");
    rendererParam->setEvaluateOnChange(false);
    rendererParam->setAnimates(false);
    
    // Version info (read-only string)
    OFX::StringParamDescriptor* versionParam = desc.defineStringParam("_version");
    versionParam->setLabel("Version");
    versionParam->setDefault("2.0.0.21 (" __DATE__ " " __TIME__ ")");
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
        _enablePlotParam = fetchBooleanParam("enablePlot");
        _rectShadeParam = fetchDoubleParam("rectShade");

        _backendParam = fetchChoiceParam("backend");

        _rendererParam = fetchStringParam("_renderer");
        _versionParam = fetchStringParam("_version");
        if (_versionParam) {
            const std::string buildVersion = std::string("2.0.0.21 ") + __DATE__ + " " + __TIME__;
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

void IntensityProfilePlotterPlugin::getRegionsOfInterest(const OFX::RegionsOfInterestArguments& args,
                                                          OFX::RegionOfInterestSetter& rois)
{
    // Optimization: Tell the host we only need a thin region around the scan line
    // This can significantly reduce memory and processing for high-res images
    try {
        // Ensure clips are initialized
        if (!_srcClip) setupClips();
        if (!_srcClip || !_srcClip->isConnected()) {
            return;  // No source clip, use default ROI
        }
        
        if (!_point1Param) setupParameters();
        if (!_point2Param) setupParameters();
        
        if (_point1Param && _point2Param) {
            // Get scan line endpoints in normalized coordinates
            OFX::Double2DParam* p1 = _point1Param;
            OFX::Double2DParam* p2 = _point2Param;
            
            double x1, y1, x2, y2;
            p1->getValueAtTime(args.time, x1, y1);
            p2->getValueAtTime(args.time, x2, y2);
            
            // Convert to RoD space (typically pixel coordinates)
            OfxRectD rod = _srcClip->getRegionOfDefinition(args.time);
            double width = rod.x2 - rod.x1;
            double height = rod.y2 - rod.y1;
            
            // Validate ROD dimensions
            if (width <= 0 || height <= 0) {
                return;  // Invalid ROD, use default
            }
            
            double px1 = rod.x1 + x1 * width;
            double py1 = rod.y1 + y1 * height;
            double px2 = rod.x1 + x2 * width;
            double py2 = rod.y1 + y2 * height;
            
            // Create bounding box around scan line with small margin
            double margin = 2.0;  // pixels
            OfxRectD roi;
            roi.x1 = std::min(px1, px2) - margin;
            roi.y1 = std::min(py1, py2) - margin;
            roi.x2 = std::max(px1, px2) + margin;
            roi.y2 = std::max(py1, py2) + margin;
            
            // Clamp to RoD
            roi.x1 = std::max(rod.x1, roi.x1);
            roi.y1 = std::max(rod.y1, roi.y1);
            roi.x2 = std::min(rod.x2, roi.x2);
            roi.y2 = std::min(rod.y2, roi.y2);
            
            // Set RoI for source clip
            rois.setRegionOfInterest(*_srcClip, roi);
        }
    } catch (...) {
        // If ROI optimization fails, fall back to default (full frame)
    }
}

void IntensityProfilePlotterPlugin::purgeCaches()
{
    // Free GPU resources and cached samples when host requests memory cleanup
    std::lock_guard<std::mutex> lock(_sampleMutex);
    _redSamples.clear();
    _redSamples.shrink_to_fit();
    _greenSamples.clear();
    _greenSamples.shrink_to_fit();
    _blueSamples.clear();
    _blueSamples.shrink_to_fit();
    
    // Recreate sampler to free GPU resources
    _sampler.reset();
}

void IntensityProfilePlotterPlugin::beginSequenceRender(const OFX::BeginSequenceRenderArguments& args)
{
    // Minimal initialization - just return
    // Disable any GPU initialization that might hang
    return;
}

const char* IntensityProfilePlotterPlugin::getRendererName() const
{
    // Return cached name if available
    if (s_cachedRendererName) {
        return s_cachedRendererName;
    }
    
    // Fallback: check current backend state
    if (GPURenderer::isAvailable()) {
        return GPURenderer::getBackendName();
    }
    return "CPU";
}

void IntensityProfilePlotterPlugin::endSequenceRender(const OFX::EndSequenceRenderArguments& args)
{
    // Optionally free resources after sequence render completes
    // For now, keep sampler alive for interactive session
}

bool IntensityProfilePlotterPlugin::isIdentity(const OFX::IsIdentityArguments& args, 
                                               OFX::Clip*& identityClip, double& identityTime)
{
    // Optimization: Skip rendering if plot is disabled
    // This avoids the expensive pixel copy when the overlay isn't even visible
    try {
        if (!_enablePlotParam) {
            _enablePlotParam = fetchBooleanParam("enablePlot");
        }
        if (_enablePlotParam && !_enablePlotParam->getValueAtTime(args.time)) {
            // Plot disabled - return identity to skip render entirely
            if (!_srcClip) {
                _srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
            }
            identityClip = _srcClip;
            identityTime = args.time;
            return true;  // Skip render when plot is off
        }
    } catch (...) {
        // If parameter fetch fails, proceed with normal render
    }
    
    // Plot is enabled - must render to copy pixels and draw overlay
    return false;
}

void IntensityProfilePlotterPlugin::render(const OFX::RenderArguments& args)    
{
    try {
        // Fetch clips
        OFX::Clip* srcClip = fetchClip(kOfxImageEffectSimpleSourceClipName);
        OFX::Clip* dstClip = fetchClip(kOfxImageEffectOutputClipName);
        
        if (!srcClip || !dstClip) {
            return;
        }
        
        // Fetch images - use unique_ptr for automatic cleanup
        std::unique_ptr<OFX::Image> srcImg(srcClip->fetchImage(args.time));
        std::unique_ptr<OFX::Image> dstImg(dstClip->fetchImage(args.time));
        
        if (!srcImg || !dstImg) {
            return;
        }
        
        // QUICK FIX: Just do a simple memcpy instead of pixel-by-pixel processing
        // This is a temporary fix to test if the plugin render works at all
        void* srcPtr = srcImg->getPixelData();
        void* dstPtr = dstImg->getPixelData();
        int srcRowBytes = srcImg->getRowBytes();
        int dstRowBytes = dstImg->getRowBytes();
        OfxRectI srcBounds = srcImg->getBounds();
        OfxRectI dstBounds = dstImg->getBounds();
        OfxRectI renderWindow = args.renderWindow;
        
        if (srcPtr && dstPtr) {
            // Simple fast copy for each row in the render window
            int width = renderWindow.x2 - renderWindow.x1;
            int height = renderWindow.y2 - renderWindow.y1;
            OFX::BitDepthEnum bitDepth = srcImg->getPixelDepth();
            OFX::PixelComponentEnum components = srcImg->getPixelComponents();
            
            int bytesPerComponent;
            switch (bitDepth) {
                case OFX::eBitDepthUByte:  bytesPerComponent = 1; break;
                case OFX::eBitDepthUShort: bytesPerComponent = 2; break;
                case OFX::eBitDepthHalf:   bytesPerComponent = 2; break;
                case OFX::eBitDepthFloat:  bytesPerComponent = 4; break;
                default: bytesPerComponent = 4; break;
            }
            
            int componentCount = (components == OFX::ePixelComponentRGBA) ? 4 : 3;
            int bytesPerPixel = bytesPerComponent * componentCount;
            int rowBytes = width * bytesPerPixel;
            
            // Fast memcpy for each row
            int xOffset = renderWindow.x1 - srcBounds.x1;
            int yOffset = renderWindow.y1 - srcBounds.y1;
            
            for (int y = 0; y < height; ++y) {
                char* srcRow = (char*)srcPtr + (yOffset + y) * srcRowBytes + xOffset * bytesPerPixel;
                char* dstRow = (char*)dstPtr + (yOffset + y) * dstRowBytes + xOffset * bytesPerPixel;
                std::memcpy(dstRow, srcRow, rowBytes);
            }
        }
        // Images automatically released when unique_ptr goes out of scope
    } catch (...) {
        // Silently catch exceptions - don't want to crash host
    }
}
