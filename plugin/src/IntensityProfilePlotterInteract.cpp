#include "IntensityProfilePlotterInteract.h"
#include "IntensityProfilePlotterPlugin.h"
#include "ofxInteract.h"
#include "ofxParam.h"

#include <cmath>

static const double POINT_HIT_RADIUS = 5.0; // pixels
static const double POINT_DISPLAY_RADIUS = 6.0; // pixels

IntensityProfilePlotterInteract::IntensityProfilePlotterInteract(OfxInteractHandle handle)
    : OFX::OverlayInteract(handle)
    , _dragState(kDragNone)
    , _instance(nullptr)
{
}

IntensityProfilePlotterInteract::IntensityProfilePlotterInteract(IntensityProfilePlotterPlugin* instance)
    : OFX::OverlayInteract(instance)
    , _dragState(kDragNone)
    , _instance(instance)
{
}

IntensityProfilePlotterInteract::~IntensityProfilePlotterInteract()
{
}

void IntensityProfilePlotterInteract::pixelToNormalized(double px, double py, double& nx, double& ny, const OFX::DrawArgs& args)
{
    OfxRectI bounds = args.renderWindow;
    double width = bounds.x2 - bounds.x1;
    double height = bounds.y2 - bounds.y1;
    
    nx = (px - bounds.x1) / width;
    ny = (py - bounds.y1) / height;
    
    // Clamp to [0, 1]
    nx = std::max(0.0, std::min(1.0, nx));
    ny = std::max(0.0, std::min(1.0, ny));
}

void IntensityProfilePlotterInteract::normalizedToPixel(double nx, double ny, double& px, double& py, const OFX::DrawArgs& args)
{
    OfxRectI bounds = args.renderWindow;
    double width = bounds.x2 - bounds.x1;
    double height = bounds.y2 - bounds.y1;
    
    px = bounds.x1 + nx * width;
    py = bounds.y1 + ny * height;
}

bool IntensityProfilePlotterInteract::hitTestPoint(double x, double y, double px, double py, double pixelScale)
{
    double dx = x - px;
    double dy = y - py;
    double distance = std::sqrt(dx * dx + dy * dy);
    return distance <= (POINT_HIT_RADIUS / pixelScale);
}

void IntensityProfilePlotterInteract::drawPoint(const OFX::DrawArgs& args, double x, double y, bool selected)
{
    OFX::OverlayInteract* overlay = getOverlayInteract();
    if (!overlay) return;
    
    double pixelScale = args.pixelScale;
    double radius = POINT_DISPLAY_RADIUS / pixelScale;
    
    // Draw outer circle (white/black outline)
    overlay->setColour(selected ? 1.0 : 0.0, selected ? 1.0 : 0.0, selected ? 1.0 : 0.0, 1.0);
    overlay->drawCircle(x, y, radius + 1.0 / pixelScale);
    
    // Draw inner circle (colored)
    overlay->setColour(1.0, 1.0, 0.0, 0.8);
    overlay->drawCircle(x, y, radius);
}

void IntensityProfilePlotterInteract::drawLine(const OFX::DrawArgs& args, double x1, double y1, double x2, double y2)
{
    OFX::OverlayInteract* overlay = getOverlayInteract();
    if (!overlay) return;
    
    overlay->setColour(1.0, 1.0, 0.0, 0.6);
    overlay->setLineWidth(1.0 / args.pixelScale);
    overlay->drawLine(x1, y1, x2, y2);
}

bool IntensityProfilePlotterInteract::draw(const OFX::DrawArgs& args)
{
    if (!_instance) return false;
    
    OFX::OverlayInteract* overlay = getOverlayInteract();
    if (!overlay) return false;
    
    // Get current parameter values
    double point1[2], point2[2];
    _instance->getPoint1Param()->getValueAtTime(args.time, point1[0], point1[1]);
    _instance->getPoint2Param()->getValueAtTime(args.time, point2[0], point2[1]);
    
    // Convert normalized to pixel coordinates
    double px1, py1, px2, py2;
    normalizedToPixel(point1[0], point1[1], px1, py1, args);
    normalizedToPixel(point2[0], point2[1], px2, py2, args);
    
    // Draw scan line
    drawLine(args, px1, py1, px2, py2);
    
    // Draw endpoints
    bool point1Selected = (_dragState == kDragPoint1);
    bool point2Selected = (_dragState == kDragPoint2);
    drawPoint(args, px1, py1, point1Selected);
    drawPoint(args, px2, py2, point2Selected);
    
    return true;
}

bool IntensityProfilePlotterInteract::penDown(const OFX::PenArgs& args)
{
    if (!_instance) return false;
    
    // Get current parameter values
    double point1[2], point2[2];
    _instance->getPoint1Param()->getValueAtTime(args.time, point1[0], point1[1]);
    _instance->getPoint2Param()->getValueAtTime(args.time, point2[0], point2[1]);
    
    // Convert to pixel coordinates
    double px1, py1, px2, py2;
    normalizedToPixel(point1[0], point1[1], px1, py1, args);
    normalizedToPixel(point2[0], point2[1], px2, py2, args);
    
    // Hit test
    double pixelScale = args.pixelScale;
    if (hitTestPoint(args.penPosition.x, args.penPosition.y, px1, py1, pixelScale)) {
        _dragState = kDragPoint1;
        return true;
    } else if (hitTestPoint(args.penPosition.x, args.penPosition.y, px2, py2, pixelScale)) {
        _dragState = kDragPoint2;
        return true;
    }
    
    _dragState = kDragNone;
    return false;
}

bool IntensityProfilePlotterInteract::penMotion(const OFX::PenArgs& args)
{
    if (!_instance) return false;
    
    if (_dragState == kDragNone) {
        return false;
    }
    
    // Convert pixel to normalized coordinates
    double nx, ny;
    pixelToNormalized(args.penPosition.x, args.penPosition.y, nx, ny, args);
    
    // Update parameter
    if (_dragState == kDragPoint1) {
        _instance->getPoint1Param()->setValueAtTime(args.time, nx, ny);
    } else if (_dragState == kDragPoint2) {
        _instance->getPoint2Param()->setValueAtTime(args.time, nx, ny);
    }
    
    return true;
}

bool IntensityProfilePlotterInteract::penUp(const OFX::PenArgs& args)
{
    if (_dragState != kDragNone) {
        _dragState = kDragNone;
        return true;
    }
    return false;
}
