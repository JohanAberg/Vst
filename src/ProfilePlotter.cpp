#include "ProfilePlotter.h"
#include "ofxImageEffect.h"
#include "ofxDrawSuite.h"
#include <algorithm>
#include <cmath>

ProfilePlotter::ProfilePlotter()
{
}

ProfilePlotter::~ProfilePlotter()
{
}

void ProfilePlotter::renderPlot(
    OFX::DrawSuite* drawSuite,
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
    int imageHeight)
{
    if (!drawSuite || !drawSuite->drawSuiteSupported()) {
        return;
    }
    
    // Calculate plot dimensions
    int plotWidth = static_cast<int>(imageWidth * 0.8); // 80% of image width
    int plotHeightPixels = static_cast<int>(imageHeight * plotHeight);
    int plotX = (imageWidth - plotWidth) / 2; // Center horizontally
    int plotY = imageHeight - plotHeightPixels - 20; // Bottom with margin
    
    // Begin drawing
    if (!drawSuite->beginDraw(outputImage)) {
        return;
    }
    
    // Draw reference ramp background if enabled
    if (showReferenceRamp) {
        drawReferenceRamp(drawSuite, plotX, plotY, plotWidth, plotHeightPixels);
    }
    
    // Draw grid
    drawGrid(drawSuite, plotX, plotY, plotWidth, plotHeightPixels);
    
    // Draw curves
    if (!redSamples.empty()) {
        drawCurve(drawSuite, redSamples, redColor, plotX, plotY, plotWidth, plotHeightPixels);
    }
    if (!greenSamples.empty()) {
        drawCurve(drawSuite, greenSamples, greenColor, plotX, plotY, plotWidth, plotHeightPixels);
    }
    if (!blueSamples.empty()) {
        drawCurve(drawSuite, blueSamples, blueColor, plotX, plotY, plotWidth, plotHeightPixels);
    }
    
    // End drawing
    drawSuite->endDraw();
}

void ProfilePlotter::drawReferenceRamp(
    OFX::DrawSuite* drawSuite,
    int plotX, int plotY,
    int plotWidth, int plotHeight)
{
    // Draw linear 0-1 grayscale ramp
    const int rampSteps = plotWidth;
    
    for (int x = 0; x < rampSteps; ++x) {
        float grayValue = static_cast<float>(x) / static_cast<float>(rampSteps - 1);
        
        // Set color (grayscale)
        drawSuite->setColour(grayValue, grayValue, grayValue, 1.0);
        
        // Draw vertical line
        int xPos = plotX + x;
        drawSuite->drawLine(xPos, plotY, xPos, plotY + plotHeight);
    }
}

void ProfilePlotter::drawGrid(
    OFX::DrawSuite* drawSuite,
    int plotX, int plotY,
    int plotWidth, int plotHeight)
{
    // Draw grid lines for reference
    drawSuite->setColour(0.5, 0.5, 0.5, 0.3);
    drawSuite->setLineWidth(1.0);
    
    // Horizontal lines (intensity levels: 0.0, 0.25, 0.5, 0.75, 1.0)
    for (int i = 0; i <= 4; ++i) {
        float level = i * 0.25f;
        int y = plotY + static_cast<int>((1.0f - level) * plotHeight);
        drawSuite->drawLine(plotX, y, plotX + plotWidth, y);
    }
    
    // Vertical lines (sample positions: 0%, 25%, 50%, 75%, 100%)
    for (int i = 0; i <= 4; ++i) {
        int x = plotX + static_cast<int>((i * 0.25) * plotWidth);
        drawSuite->drawLine(x, plotY, x, plotY + plotHeight);
    }
    
    // Border
    drawSuite->setColour(1.0, 1.0, 1.0, 0.8);
    drawSuite->setLineWidth(2.0);
    drawSuite->drawRectangle(plotX, plotY, plotWidth, plotHeight, false);
}

void ProfilePlotter::drawCurve(
    OFX::DrawSuite* drawSuite,
    const std::vector<float>& samples,
    const double color[4],
    int plotX, int plotY,
    int plotWidth, int plotHeight)
{
    if (samples.empty()) return;
    
    // Set curve color
    drawSuite->setColour(color[0], color[1], color[2], color[3]);
    drawSuite->setLineWidth(2.0);
    
    // Draw curve as connected line segments
    int sampleCount = static_cast<int>(samples.size());
    
    for (int i = 0; i < sampleCount - 1; ++i) {
        float value1 = std::max(0.0f, std::min(1.0f, samples[i]));
        float value2 = std::max(0.0f, std::min(1.0f, samples[i + 1]));
        
        // Convert to plot coordinates
        int x1 = plotX + static_cast<int>((static_cast<float>(i) / static_cast<float>(sampleCount - 1)) * plotWidth);
        int y1 = plotY + static_cast<int>((1.0f - value1) * plotHeight);
        
        int x2 = plotX + static_cast<int>((static_cast<float>(i + 1) / static_cast<float>(sampleCount - 1)) * plotWidth);
        int y2 = plotY + static_cast<int>((1.0f - value2) * plotHeight);
        
        drawSuite->drawLine(x1, y1, x2, y2);
    }
}
