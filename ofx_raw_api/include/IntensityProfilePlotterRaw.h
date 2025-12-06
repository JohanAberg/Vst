#ifndef INTENSITY_PROFILE_PLOTTER_RAW_H
#define INTENSITY_PROFILE_PLOTTER_RAW_H

// Raw OFX API version - no Support library
// Include OFX headers for type definitions
#include "ofxCore.h"
#include "ofxImageEffect.h"
#include "ofxParam.h"

// Instance data structure
struct IntensityProfilePlotterInstanceData {
    // Clip handles
    OfxImageClipHandle sourceClip;
    OfxImageClipHandle auxClip;
    OfxImageClipHandle outputClip;
    
    // Parameter handles
    OfxParamHandle point1Param;
    OfxParamHandle point2Param;
    OfxParamHandle dataSourceParam;
    OfxParamHandle sampleCountParam;
    OfxParamHandle plotHeightParam;
    OfxParamHandle redCurveColorParam;
    OfxParamHandle greenCurveColorParam;
    OfxParamHandle blueCurveColorParam;
    OfxParamHandle showReferenceRampParam;
    
    // Component pointers (will be created on instance creation)
    void* sampler;  // IntensitySampler*
    void* plotter;  // ProfilePlotter*
};

#endif // INTENSITY_PROFILE_PLOTTER_RAW_H

