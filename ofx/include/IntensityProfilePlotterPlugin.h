#ifndef INTENSITY_PROFILE_PLOTTER_PLUGIN_H
#define INTENSITY_PROFILE_PLOTTER_PLUGIN_H

#include "ofxImageEffect.h"
#include "ofxInteract.h"
#include "ofxDrawSuite.h"
#include "ofxsImageEffect.h"
#include "ofxsInteract.h"
#include <memory>

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
    OFX::RGBAParam* getRedCurveColorParam() const { return _redCurveColorParam; }
    OFX::RGBAParam* getGreenCurveColorParam() const { return _greenCurveColorParam; }
    OFX::RGBAParam* getBlueCurveColorParam() const { return _blueCurveColorParam; }
    OFX::BooleanParam* getShowReferenceRampParam() const { return _showReferenceRampParam; }

private:
    void setupParameters();
    void setupClips();
    void drawLine(float* buffer, int width, int height, int nComp, int rowBytes,
                  int x1, int y1, int x2, int y2, float r, float g, float b);
    
    // Clips
    OFX::Clip* _srcClip;
    OFX::Clip* _dstClip;
    OFX::Clip* _auxClip;
    
    // Parameters
    OFX::Double2DParam* _point1Param;      // Normalized coordinates [0-1, 0-1]
    OFX::Double2DParam* _point2Param;      // Normalized coordinates [0-1, 0-1]
    OFX::ChoiceParam* _dataSourceParam;    // 0=Input Clip, 1=Auxiliary Clip, 2=Built-in Ramp
    OFX::IntParam* _sampleCountParam;       // Number of samples along scan line
    OFX::DoubleParam* _plotHeightParam;    // Height of plot overlay (normalized)
    OFX::RGBAParam* _redCurveColorParam;
    OFX::RGBAParam* _greenCurveColorParam;
    OFX::RGBAParam* _blueCurveColorParam;
    OFX::BooleanParam* _showReferenceRampParam;
    
    // Components
    std::unique_ptr<IntensitySampler> _sampler;
    std::unique_ptr<ProfilePlotter> _plotter;
    
    // Interact (OSM)
    IntensityProfilePlotterInteract* _interact;
};

#endif // INTENSITY_PROFILE_PLOTTER_PLUGIN_H
