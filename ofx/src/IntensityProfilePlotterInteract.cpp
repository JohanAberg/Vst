#include "IntensityProfilePlotterInteract.h"
#include "IntensityProfilePlotterPlugin.h"
#include "ofxInteract.h"
#include "ofxParam.h"

#include <cmath>
#include <OpenGL/gl.h>

static const double POINT_HIT_RADIUS = 15.0; // pixels
static const double POINT_DISPLAY_RADIUS = 10.0; // pixels

IntensityProfilePlotterInteract::IntensityProfilePlotterInteract(OfxInteractHandle handle, OFX::ImageEffect* effect)
    : OFX::OverlayInteract(handle)
    , _dragState(kDragNone)
    , _instance(nullptr)
    , _lineDragOffset(0.0)
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

void IntensityProfilePlotterInteract::pixelToNormalized(double px, double py, double& nx, double& ny)
{
    // Standard image dimensions (1920x1080)
    double width = 1920.0;
    double height = 1080.0;
    
    nx = px / width;
    ny = py / height;
    
    // Clamp to [0, 1]
    nx = std::max(0.0, std::min(1.0, nx));
    ny = std::max(0.0, std::min(1.0, ny));
}

void IntensityProfilePlotterInteract::normalizedToPixel(double nx, double ny, double& px, double& py)
{
    // Standard image dimensions (1920x1080)
    double width = 1920.0;
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

bool IntensityProfilePlotterInteract::hitTestLine(double x, double y, double px1, double py1, double px2, double py2, double pixelScale, double& t)
{
    // Vector from P1 to P2
    double dx = px2 - px1;
    double dy = py2 - py1;
    double lineLen2 = dx * dx + dy * dy;
    
    if (lineLen2 < 1e-6) {
        return false; // Points are too close
    }
    
    // Vector from P1 to cursor
    double cx = x - px1;
    double cy = y - py1;
    
    // Project cursor onto line
    t = (cx * dx + cy * dy) / lineLen2;
    
    // Clamp t to [0, 1] to stay on the line segment
    t = std::max(0.0, std::min(1.0, t));
    
    // Closest point on line
    double closestX = px1 + t * dx;
    double closestY = py1 + t * dy;
    
    // Distance to line
    double distX = x - closestX;
    double distY = y - closestY;
    double distance = std::sqrt(distX * distX + distY * distY);
    
    // Hit if within threshold
    return distance <= (POINT_HIT_RADIUS / pixelScale);
}

void IntensityProfilePlotterInteract::drawPoint(const OFX::DrawArgs& args, double x, double y, bool selected)
{
    // Draw using OpenGL immediate mode (deprecated but widely supported in OFX)
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);
    
    // Set color based on selection state
    if (selected) {
        glColor3f(1.0f, 1.0f, 0.0f); // Yellow when selected
    } else {
        glColor3f(1.0f, 1.0f, 1.0f); // White normally
    }
    
    // Draw filled circle
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(x, y);
    const int segments = 20;
    for (int i = 0; i <= segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        double dx = POINT_DISPLAY_RADIUS * cos(angle);
        double dy = POINT_DISPLAY_RADIUS * sin(angle);
        glVertex2d(x + dx, y + dy);
    }
    glEnd();
    
    // Draw outline
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        double dx = POINT_DISPLAY_RADIUS * cos(angle);
        double dy = POINT_DISPLAY_RADIUS * sin(angle);
        glVertex2d(x + dx, y + dy);
    }
    glEnd();
    
    glPopAttrib();
}

void IntensityProfilePlotterInteract::drawLine(const OFX::DrawArgs& args, double x1, double y1, double x2, double y2)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    
    glBegin(GL_LINES);
    glVertex2d(x1, y1);
    glVertex2d(x2, y2);
    glEnd();
    
    glPopAttrib();
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
    normalizedToPixel(point1[0], point1[1], px1, py1);
    normalizedToPixel(point2[0], point2[1], px2, py2);
    
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
    
    // Convert normalized to pixel coordinates
    // Use simple scaling based on typical HD dimensions
    double px1 = point1[0] * 1920.0;
    double py1 = point1[1] * 1080.0;
    double px2 = point2[0] * 1920.0;
    double py2 = point2[1] * 1080.0;
    
    // Hit test points first (higher priority)
    double pixelScale = args.pixelScale.x;
    if (hitTestPoint(args.penPosition.x, args.penPosition.y, px1, py1, pixelScale)) {
        _dragState = kDragPoint1;
        return true;
    } else if (hitTestPoint(args.penPosition.x, args.penPosition.y, px2, py2, pixelScale)) {
        _dragState = kDragPoint2;
        return true;
    }
    
    // Then test the line connecting the points
    double t = 0.0;
    if (hitTestLine(args.penPosition.x, args.penPosition.y, px1, py1, px2, py2, pixelScale, t)) {
        _dragState = kDragLine;
        _lineDragOffset = t; // Store position along the line
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
    
    // Convert pixel to normalized coordinates using simple scaling
    double nx = args.penPosition.x / 1920.0;
    double ny = args.penPosition.y / 1080.0;
    
    // Clamp to [0, 1]
    nx = std::max(0.0, std::min(1.0, nx));
    ny = std::max(0.0, std::min(1.0, ny));
    
    // Update parameter
    if (_dragState == kDragPoint1) {
        _instance->getPoint1Param()->setValueAtTime(args.time, nx, ny);
    } else if (_dragState == kDragPoint2) {
        _instance->getPoint2Param()->setValueAtTime(args.time, nx, ny);
    } else if (_dragState == kDragLine) {
        // Get current point positions
        double point1[2], point2[2];
        _instance->getPoint1Param()->getValueAtTime(args.time, point1[0], point1[1]);
        _instance->getPoint2Param()->getValueAtTime(args.time, point2[0], point2[1]);
        
        // Convert current points to pixel space
        double px1 = point1[0] * 1920.0;
        double py1 = point1[1] * 1080.0;
        double px2 = point2[0] * 1920.0;
        double py2 = point2[1] * 1080.0;
        
        // Direction vector of the line
        double dx = px2 - px1;
        double dy = py2 - py1;
        double lineLen = std::sqrt(dx * dx + dy * dy);
        
        if (lineLen > 1e-6) {
            // Normalize direction
            dx /= lineLen;
            dy /= lineLen;
            
            // Project cursor position onto the line's direction
            double mouseX = args.penPosition.x;
            double mouseY = args.penPosition.y;
            
            // Vector from P1 to mouse
            double mouseRelX = mouseX - px1;
            double mouseRelY = mouseY - py1;
            
            // Project onto line direction to get the offset
            double projection = mouseRelX * dx + mouseRelY * dy;
            
            // Calculate the new center point along the line
            double newCenterX = px1 + projection * dx;
            double newCenterY = py1 + projection * dy;
            
            // The offset from initial P1
            double offsetX = newCenterX - px1;
            double offsetY = newCenterY - py1;
            
            // Move both points by this offset
            double newP1X = point1[0] + (offsetX / 1920.0);
            double newP1Y = point1[1] + (offsetY / 1080.0);
            double newP2X = point2[0] + (offsetX / 1920.0);
            double newP2Y = point2[1] + (offsetY / 1080.0);
            
            // Clamp to [0, 1]
            newP1X = std::max(0.0, std::min(1.0, newP1X));
            newP1Y = std::max(0.0, std::min(1.0, newP1Y));
            newP2X = std::max(0.0, std::min(1.0, newP2X));
            newP2Y = std::max(0.0, std::min(1.0, newP2Y));
            
            // Update both parameters
            _instance->getPoint1Param()->setValueAtTime(args.time, newP1X, newP1Y);
            _instance->getPoint2Param()->setValueAtTime(args.time, newP2X, newP2Y);
        }
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
