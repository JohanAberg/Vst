#ifndef INTENSITY_PROFILE_PLOTTER_INTERACT_H
#define INTENSITY_PROFILE_PLOTTER_INTERACT_H

#include "ofxInteract.h"
#include "ofxParam.h"

class IntensityProfilePlotterPlugin;

/**
 * On-Screen Manipulator (OSM) for interactive scan line definition.
 * Allows users to drag two endpoints (P1 and P2) directly in the viewer.
 */
class IntensityProfilePlotterInteract : public OFX::OverlayInteract
{
public:
    IntensityProfilePlotterInteract(OfxInteractHandle handle);
    IntensityProfilePlotterInteract(IntensityProfilePlotterPlugin* instance);
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
        kDragPoint2
    };
    
    DragState _dragState;
    IntensityProfilePlotterPlugin* _instance;
    
    // Hit testing
    bool hitTestPoint(double x, double y, double px, double py, double pixelScale);
    void drawPoint(const OFX::DrawArgs& args, double x, double y, bool selected);
    void drawLine(const OFX::DrawArgs& args, double x1, double y1, double x2, double y2);
    
    // Convert between pixel and normalized coordinates
    void pixelToNormalized(double px, double py, double& nx, double& ny, const OFX::DrawArgs& args);
    void normalizedToPixel(double nx, double ny, double& px, double& py, const OFX::DrawArgs& args);
};

// Descriptor for the interact
class IntensityProfilePlotterInteractDescriptor : public OFX::DefaultEffectOverlayDescriptor<IntensityProfilePlotterInteract>
{
public:
    IntensityProfilePlotterInteractDescriptor()
        : OFX::DefaultEffectOverlayDescriptor<IntensityProfilePlotterInteract>()
    {
    }
};

#endif // INTENSITY_PROFILE_PLOTTER_INTERACT_H
