#ifndef PROFILE_PLOTTER_H
#define PROFILE_PLOTTER_H

#include "ofxImageEffect.h"
#include "ofxDrawSuite.h"
#include "ofxsImageEffect.h"
#include <vector>

/**
 * Renders intensity profile plots as overlay graphics.
 * Uses OFX DrawSuite to draw curves and reference ramp.
 */
class ProfilePlotter
{
public:
    ProfilePlotter();
    ~ProfilePlotter();
    
    /**
     * Render the intensity profile plot overlay.
     * 
     * @param drawSuite OFX DrawSuite for rendering
     * @param outputImage Output image to draw on
     * @param redSamples Red channel intensity samples
     * @param greenSamples Green channel intensity samples
     * @param blueSamples Blue channel intensity samples
     * @param redColor RGBA color for red curve
     * @param greenColor RGBA color for green curve
     * @param blueColor RGBA color for blue curve
     * @param plotHeight Normalized height of plot area [0-1]
     * @param showReferenceRamp Whether to show reference grayscale ramp
     * @param imageWidth Width of output image
     * @param imageHeight Height of output image
     */
    void renderPlot(
        const OfxDrawSuiteV1* drawSuite,
        OFX::Image* outputImage,
        const std::vector<float>& redSamples,
        const std::vector<float>& greenSamples,
        const std::vector<float>& blueSamples,
        const double redColor[4],
        const double greenColor[4],
        const double blueColor[4],
        double plotHeight,
        bool showReferenceRamp,
        int imageWidth,
        int imageHeight
    );

private:
    void drawReferenceRamp(
        const OfxDrawSuiteV1* drawSuite,
        int plotX, int plotY,
        int plotWidth, int plotHeight
    );
    
    void drawCurve(
        const OfxDrawSuiteV1* drawSuite,
        const std::vector<float>& samples,
        const double color[4],
        int plotX, int plotY,
        int plotWidth, int plotHeight
    );
    
    void drawGrid(
        const OfxDrawSuiteV1* drawSuite,
        int plotX, int plotY,
        int plotWidth, int plotHeight
    );
};

#endif // PROFILE_PLOTTER_H
