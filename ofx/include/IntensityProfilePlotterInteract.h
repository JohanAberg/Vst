#ifndef INTENSITY_PROFILE_PLOTTER_INTERACT_H
#define INTENSITY_PROFILE_PLOTTER_INTERACT_H

#include "ofxInteract.h"
#include "ofxParam.h"
#include "ofxsInteract.h"

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
        kDragLine
    };
    
    DragState _dragState;
    IntensityProfilePlotterPlugin* _instance;
    double _lineDragOffset;
    double _lastMouseX, _lastMouseY; // Track last mouse position for delta calculation
    
    // Hit testing
    bool hitTestPoint(double x, double y, double px, double py, double pixelScale);
    bool hitTestLine(double x, double y, double px1, double py1, double px2, double py2, double pixelScale, double& t);
    void drawPoint(const OFX::DrawArgs& args, double x, double y, bool selected);
    void drawLine(const OFX::DrawArgs& args, double x1, double y1, double x2, double y2);
    
    // Convert between pixel and normalized coordinates
    void pixelToNormalized(double px, double py, double& nx, double& ny);
    void normalizedToPixel(double nx, double ny, double& px, double& py);
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
