#ifndef INTENSITY_PROFILE_PLOTTER_INTERACT_H
#define INTENSITY_PROFILE_PLOTTER_INTERACT_H

#include "ofxInteract.h"
#include "ofxParam.h"
#include "ofxsInteract.h"
#include "ofxImageEffect.h"

namespace OFX { class Image; }

class IntensityProfilePlotterPlugin;

/**
 * On-Screen Manipulator (OSM) for interactive scan line definition.
 * Allows users to drag two endpoints (P1 and P2) directly in the viewer.
 */
class IntensityProfilePlotterInteract : public OFX::OverlayInteract
{
public:
    IntensityProfilePlotterInteract(OfxInteractHandle handle, OFX::ImageEffect* effect = nullptr);
    virtual ~IntensityProfilePlotterInteract();
    
    // OFX::Interact overrides
    virtual bool draw(const OFX::DrawArgs& args) override;
    virtual bool penMotion(const OFX::PenArgs& args) override;
    virtual bool penDown(const OFX::PenArgs& args) override;
    virtual bool penUp(const OFX::PenArgs& args) override;
    
    void setInstance(IntensityProfilePlotterPlugin* instance) { _instance = instance; }

private:
    enum DragState {
        kDragNone,
        kDragPoint1,
        kDragPoint2,
        kDragLine,
        kDragRectMove,
        kDragRectTL,
        kDragRectTR,
        kDragRectBL,
        kDragRectBR
    };
    
    DragState _dragState;
    IntensityProfilePlotterPlugin* _instance;
    double _lineDragOffset;
    double _lastMouseX, _lastMouseY; // Track last mouse position for delta calculation
    double _lineP1Start[2]; // Store point1 position when line drag starts
    double _lineP2Start[2]; // Store point2 position when line drag starts
    double _rectStartPos[2];
    double _rectStartSize[2];
    double _rectDragStartX, _rectDragStartY;
    
    // Hit testing
    bool hitTestPoint(double x, double y, double px, double py, double pixelScale);
    bool hitTestLine(double x, double y, double px1, double py1, double px2, double py2, double pixelScale, double& t);
    int hitTestRectHandles(double x, double y, double rx, double ry, double rw, double rh, double pixelScale);
    bool hitTestRectBody(double x, double y, double rx, double ry, double rw, double rh);
    void drawPoint(const OFX::DrawArgs& args, double x, double y, bool selected);
    void drawLine(const OFX::DrawArgs& args, double x1, double y1, double x2, double y2);
    void drawRect(const OFX::DrawArgs& args, double rx, double ry, double rw, double rh, bool selected);
    void drawHandle(const OFX::DrawArgs& args, double x, double y, bool selected);
    void drawPlot(const OFX::DrawArgs& args, OFX::Image& src, int imgW, int imgH, double nx1, double ny1, double nx2, double ny2);
};

// Descriptor for the interact
class IntensityProfilePlotterInteractDescriptor : public OFX::DefaultEffectOverlayDescriptor<IntensityProfilePlotterInteractDescriptor, IntensityProfilePlotterInteract>
{
public:
    IntensityProfilePlotterInteractDescriptor()
        : OFX::DefaultEffectOverlayDescriptor<IntensityProfilePlotterInteractDescriptor, IntensityProfilePlotterInteract>()
    {
    }
};

#endif // INTENSITY_PROFILE_PLOTTER_INTERACT_H
