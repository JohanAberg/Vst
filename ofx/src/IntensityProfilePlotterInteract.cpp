#include "IntensityProfilePlotterInteract.h"
#include "IntensityProfilePlotterPlugin.h"
#include "ofxInteract.h"
#include "ofxParam.h"

#include <cmath>

static const double POINT_HIT_RADIUS = 5.0; // pixels
static const double POINT_DISPLAY_RADIUS = 6.0; // pixels

IntensityProfilePlotterInteract::IntensityProfilePlotterInteract(OfxInteractHandle handle, OFX::ImageEffect* effect)
    : OFX::OverlayInteract(handle)
    , _dragState(kDragNone)
    , _instance(nullptr)
{
    // Get the effect instance from the parameter
    if (effect) {
        _instance = dynamic_cast<IntensityProfilePlotterPlugin*>(effect);
    }
    // Note: Can also access via _effect member from Interact base class if needed
}

IntensityProfilePlotterInteract::~IntensityProfilePlotterInteract()
{
}

void IntensityProfilePlotterInteract::pixelToNormalized(double px, double py, double& nx, double& ny, const OFX::DrawArgs& args)
{
    // TODO: Get render window from effect or args
    // For now, use viewport if available
    double width = 1920.0; // Default, should get from effect
    double height = 1080.0;
    
    nx = px / width;
    ny = py / height;
    
    // Clamp to [0, 1]
    nx = std::max(0.0, std::min(1.0, nx));
    ny = std::max(0.0, std::min(1.0, ny));
}

void IntensityProfilePlotterInteract::normalizedToPixel(double nx, double ny, double& px, double& py, const OFX::DrawArgs& args)
{
    // TODO: Get render window from effect or args
    double width = 1920.0; // Default, should get from effect
    double height = 1080.0;
    
    px = nx * width;
    py = ny * height;
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
    // TODO: Implement using DrawSuite C API
    // DrawSuite must be fetched from host and used via C API
    (void)args; (void)x; (void)y; (void)selected;
}

void IntensityProfilePlotterInteract::drawLine(const OFX::DrawArgs& args, double x1, double y1, double x2, double y2)
{
    // TODO: Implement using DrawSuite C API
    (void)args; (void)x1; (void)y1; (void)x2; (void)y2;
}

bool IntensityProfilePlotterInteract::draw(const OFX::DrawArgs& args)
{
    if (!_instance) return false;
    
    // TODO: Implement drawing using DrawSuite C API
    // DrawSuite must be fetched from host
    
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
    
    // Convert to pixel coordinates (stub - needs proper implementation)
    double px1, py1, px2, py2;
    // TODO: Fix coordinate conversion - PenArgs vs DrawArgs
    px1 = point1[0] * 1920.0; py1 = point1[1] * 1080.0;
    px2 = point2[0] * 1920.0; py2 = point2[1] * 1080.0;
    
    // Hit test
    double pixelScale = args.pixelScale.x; // pixelScale is OfxPointD
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
    
    // Convert pixel to normalized coordinates (stub)
    double nx, ny;
    // TODO: Fix coordinate conversion
    nx = args.penPosition.x / 1920.0;
    ny = args.penPosition.y / 1080.0;
    
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
