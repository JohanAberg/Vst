#ifndef INTENSITY_PROFILE_PLOTTER_PLUGIN_H
#define INTENSITY_PROFILE_PLOTTER_PLUGIN_H

#include "ofxImageEffect.h"
#include "ofxInteract.h"
#include "ofxDrawSuite.h"
#include "ofxsImageEffect.h"
#include "ofxsInteract.h"
#include <memory>
#include <vector>
#include <mutex>

class IntensityProfilePlotterInteract;
class IntensitySampler;
class ProfilePlotter;

/**
 * Intensity Profile Plotter OFX Plugin
 * 
 * Provides GPU-accelerated visualization of intensity profiles along a user-defined scan line.
 * Supports multiple data sources: input clip, auxiliary clip, and built-in ramp for LUT testing.
 */
class IntensityProfilePlotterPlugin : public OFX::ImageEffect
{
public:
    IntensityProfilePlotterPlugin(OfxImageEffectHandle handle);
    virtual ~IntensityProfilePlotterPlugin();

    // OFX::ImageEffect overrides
    virtual void render(const OFX::RenderArguments& args) override;
    virtual bool getRegionOfDefinition(const OFX::RegionOfDefinitionArguments& args, 
                                       OfxRectD& rod) override;
    virtual void getClipPreferences(OFX::ClipPreferencesSetter& clipPreferences) override;
    virtual bool isIdentity(const OFX::IsIdentityArguments& args, 
                           OFX::Clip*& identityClip, double& identityTime) override;

    // Parameter accessors
    OFX::Double2DParam* getPoint1Param() const { return _point1Param; }
    OFX::Double2DParam* getPoint2Param() const { return _point2Param; }
    OFX::ChoiceParam* getDataSourceParam() const { return _dataSourceParam; }
    OFX::IntParam* getSampleCountParam() const { return _sampleCountParam; }
    OFX::DoubleParam* getPlotHeightParam() const { return _plotHeightParam; }
    OFX::Double2DParam* getPlotRectPosParam() const { return _plotRectPosParam; }
    OFX::Double2DParam* getPlotRectSizeParam() const { return _plotRectSizeParam; }
    OFX::DoubleParam* getWhitePointParam() const { return _whitePointParam; }
    OFX::IntParam* getLineWidthParam() const { return _lineWidthParam; }
    OFX::RGBAParam* getRedCurveColorParam() const { return _redCurveColorParam; }
    OFX::RGBAParam* getGreenCurveColorParam() const { return _greenCurveColorParam; }
    OFX::RGBAParam* getBlueCurveColorParam() const { return _blueCurveColorParam; }
    OFX::BooleanParam* getShowReferenceRampParam() const { return _showReferenceRampParam; }

    // Clip accessors for overlay sampling
    OFX::Clip* getSourceClip() { if(!_srcClip) setupClips(); return _srcClip; }
    OFX::Clip* getOutputClip() { if(!_dstClip) setupClips(); return _dstClip; }
    
    // Store sampled curve data for interact to render
    void setCurveSamples(const std::vector<float>& red, const std::vector<float>& green, const std::vector<float>& blue)
    {
        std::lock_guard<std::mutex> lock(_sampleMutex);
        _redSamples = red;
        _greenSamples = green;
        _blueSamples = blue;
    }
    
    void getCurveSamples(std::vector<float>& red, std::vector<float>& green, std::vector<float>& blue) const
    {
        std::lock_guard<std::mutex> lock(_sampleMutex);
        red = _redSamples;
        green = _greenSamples;
        blue = _blueSamples;
    }

private:
    void setupParameters();
    void setupClips();
    
    // Clips
    OFX::Clip* _srcClip = nullptr;
    OFX::Clip* _dstClip = nullptr;
    OFX::Clip* _auxClip = nullptr;
    
    // Parameters
    OFX::Double2DParam* _point1Param = nullptr;      // Normalized coordinates [0-1, 0-1]
    OFX::Double2DParam* _point2Param = nullptr;      // Normalized coordinates [0-1, 0-1]
    OFX::ChoiceParam* _dataSourceParam = nullptr;    // 0=Input Clip, 1=Auxiliary Clip, 2=Built-in Ramp
    OFX::IntParam* _sampleCountParam = nullptr;       // Number of samples along scan line
    OFX::DoubleParam* _plotHeightParam = nullptr;    // Height of plot overlay (normalized)
    OFX::Double2DParam* _plotRectPosParam = nullptr;  // Top-left normalized position of plot rect
    OFX::Double2DParam* _plotRectSizeParam = nullptr; // Normalized size of plot rect
    OFX::DoubleParam* _whitePointParam = nullptr;     // Map this intensity to 1.0
    OFX::IntParam* _lineWidthParam = nullptr;         // Line width in pixels
    OFX::RGBAParam* _redCurveColorParam = nullptr;
    OFX::RGBAParam* _greenCurveColorParam = nullptr;
    OFX::RGBAParam* _blueCurveColorParam = nullptr;
    OFX::BooleanParam* _showReferenceRampParam = nullptr;
    OFX::StringParam* _versionParam = nullptr;
    
    // Components
    std::unique_ptr<IntensitySampler> _sampler;
    std::unique_ptr<ProfilePlotter> _plotter;
    
    // Interact (OSM)
    IntensityProfilePlotterInteract* _interact = nullptr;
    
    // Curve sample cache for interact rendering
    mutable std::mutex _sampleMutex;
    std::vector<float> _redSamples;
    std::vector<float> _greenSamples;
    std::vector<float> _blueSamples;
};

#endif // INTENSITY_PROFILE_PLOTTER_PLUGIN_H
