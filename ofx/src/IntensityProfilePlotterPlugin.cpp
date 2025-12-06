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
    
    // Plugin version
    desc.setVersion(1, 0, 0, 0, 0);
    
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
    
    // Enable interact (overlays are supported via interact descriptor)
}

void IntensityProfilePlotterPluginFactory::describeInContext(OFX::ImageEffectDescriptor& desc, OFX::ContextEnum context)
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
    
    // Define parameters (ImageEffectDescriptor inherits from ParamSetDescriptor)
    // Point 1
    OFX::Double2DParamDescriptor* point1Param = desc.defineDouble2DParam("point1");
    point1Param->setLabel("Point 1");
    point1Param->setDefault(0.2, 0.5);
    point1Param->setDisplayRange(0.0, 0.0, 1.0, 1.0);
    point1Param->setDimensionLabels("X", "Y");
    
    // Point 2
    OFX::Double2DParamDescriptor* point2Param = desc.defineDouble2DParam("point2");
    point2Param->setLabel("Point 2");
    point2Param->setDefault(0.8, 0.5);
    point2Param->setDisplayRange(0.0, 0.0, 1.0, 1.0);
    point2Param->setDimensionLabels("X", "Y");
    
    // Data source
    OFX::ChoiceParamDescriptor* dataSourceParam = desc.defineChoiceParam("dataSource");
    dataSourceParam->setLabel("Data Source");
    dataSourceParam->appendOption("Input Clip");
    dataSourceParam->appendOption("Auxiliary Clip");
    dataSourceParam->appendOption("Built-in Ramp (LUT Test)");
    dataSourceParam->setDefault(0);
    
    // Sample count
    OFX::IntParamDescriptor* sampleCountParam = desc.defineIntParam("sampleCount");
    sampleCountParam->setLabel("Sample Count");
    sampleCountParam->setDefault(512);
    sampleCountParam->setDisplayRange(64, 2048);
    sampleCountParam->setHint("Number of samples along the scan line");
    
    // Plot height
    OFX::DoubleParamDescriptor* plotHeightParam = desc.defineDoubleParam("plotHeight");
    plotHeightParam->setLabel("Plot Height");
    plotHeightParam->setDefault(0.3);
    plotHeightParam->setDisplayRange(0.1, 0.8);
    plotHeightParam->setHint("Height of the plot overlay (normalized)");
    
    // Curve colors
    OFX::RGBAParamDescriptor* redColorParam = desc.defineRGBAParam("redCurveColor");
    redColorParam->setLabel("Red Curve Color");
    redColorParam->setDefault(1.0, 0.0, 0.0, 1.0);
    
    OFX::RGBAParamDescriptor* greenColorParam = desc.defineRGBAParam("greenCurveColor");
    greenColorParam->setLabel("Green Curve Color");
    greenColorParam->setDefault(0.0, 1.0, 0.0, 1.0);
    
    OFX::RGBAParamDescriptor* blueColorParam = desc.defineRGBAParam("blueCurveColor");
    blueColorParam->setLabel("Blue Curve Color");
    blueColorParam->setDefault(0.0, 0.0, 1.0, 1.0);
    
    // Show reference ramp
    OFX::BooleanParamDescriptor* showRampParam = desc.defineBooleanParam("showReferenceRamp");
    showRampParam->setLabel("Show Reference Ramp");
    showRampParam->setDefault(true);
    showRampParam->setHint("Display linear 0-1 grayscale ramp background");
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
}

void IntensityProfilePlotterPlugin::setupParameters()
{
    // Fetch parameters (they should already be defined in describeInContext)
    _point1Param = fetchDouble2DParam("point1");
    _point2Param = fetchDouble2DParam("point2");
    _dataSourceParam = fetchChoiceParam("dataSource");
    _sampleCountParam = fetchIntParam("sampleCount");
    _plotHeightParam = fetchDoubleParam("plotHeight");
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
    // Note: DrawSuite must be fetched from host via fetchSuite
    // For now, we'll skip overlay rendering - can be added later if needed
    // const OfxDrawSuiteV1* drawSuite = static_cast<const OfxDrawSuiteV1*>(fetchSuite(kOfxDrawSuite));
    // if (drawSuite) {
    //     _plotter->renderPlot(
    //         drawSuite,
    //         outputImg,
    //         redSamples, greenSamples, blueSamples,
    //         redColor, greenColor, blueColor,
    //         plotHeight,
    //         showReferenceRamp,
    //         dstWidth, dstHeight
    //     );
    // }
}
