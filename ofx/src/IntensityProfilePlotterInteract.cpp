#include "IntensityProfilePlotterInteract.h"
#include "IntensityProfilePlotterPlugin.h"
#include "ofxInteract.h"
#include "ofxParam.h"

#include <cmath>
#include <algorithm>

#ifdef _WIN32
#define NOMINMAX  // Prevent windows.h from defining min/max macros
#include <windows.h>
#include <GL/gl.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#elif defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

static const double POINT_HIT_RADIUS = 15.0; // pixels
static const double POINT_DISPLAY_RADIUS = 10.0; // pixels
static const double HANDLE_SIZE = 14.0; // pixels
static const double HANDLE_HIT_RADIUS = 12.0; // pixels

IntensityProfilePlotterInteract::IntensityProfilePlotterInteract(OfxInteractHandle handle, OFX::ImageEffect* effect)
    : OFX::OverlayInteract(handle)
    , _dragState(kDragNone)
    , _instance(nullptr)
    , _lineDragOffset(0.0)
    , _lastMouseX(0.0)
    , _lastMouseY(0.0)
    , _rectDragStartX(0.0)
    , _rectDragStartY(0.0)
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

int IntensityProfilePlotterInteract::hitTestRectHandles(double x, double y, double rx, double ry, double rw, double rh, double pixelScale)
{
    // Returns drag state for handle or -1 if none
    struct Handle { double hx, hy; DragState state; };
    Handle handles[] = {
        {rx, ry, kDragRectTL},
        {rx + rw, ry, kDragRectTR},
        {rx, ry + rh, kDragRectBL},
        {rx + rw, ry + rh, kDragRectBR}
    };
    for (const auto& h : handles) {
        double dx = x - h.hx;
        double dy = y - h.hy;
        double dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= (HANDLE_HIT_RADIUS / pixelScale)) {
            return static_cast<int>(h.state);
        }
    }
    return -1;
}

bool IntensityProfilePlotterInteract::hitTestRectBody(double x, double y, double rx, double ry, double rw, double rh)
{
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
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

void IntensityProfilePlotterInteract::drawRect(const OFX::DrawArgs& args, double rx, double ry, double rw, double rh, bool selected)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);

    // DEBUG: Draw red square at fixed screen position (200, 200) to confirm drawRect executes
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2d(0.1, 0.8);
    glVertex2d(0.8, 0.8);
    glVertex2d(0.8, .2);
    glVertex2d(0.8, .2);
    glEnd();

    // DEBUG: Draw red square at fixed screen position (200, 200) to confirm drawRect executes
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2d(200, 200);
    glVertex2d(250, 200);
    glVertex2d(250, 250);
    glVertex2d(200, 250);
    glEnd();

    // Fill (semi-transparent)
    glColor4f(0.1f, 0.1f, 0.1f, 0.35f);
    glBegin(GL_QUADS);
    glVertex2d(rx, ry);
    glVertex2d(rx + rw, ry);
    glVertex2d(rx + rw, ry + rh);
    glVertex2d(rx, ry + rh);
    glEnd();

    // Border
    if (selected) {
        glColor3f(1.0f, 1.0f, 0.0f);
    } else {
        glColor3f(0.8f, 0.8f, 0.8f);
    }
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(rx, ry);
    glVertex2d(rx + rw, ry);
    glVertex2d(rx + rw, ry + rh);
    glVertex2d(rx, ry + rh);
    glEnd();

    glPopAttrib();
}

void IntensityProfilePlotterInteract::drawHandle(const OFX::DrawArgs& args, double x, double y, bool selected)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);
    if (selected) {
        glColor3f(1.0f, 1.0f, 0.0f);
    } else {
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    double half = HANDLE_SIZE * 0.5;
    glBegin(GL_QUADS);
    glVertex2d(x - half, y - half);
    glVertex2d(x + half, y - half);
    glVertex2d(x + half, y + half);
    glVertex2d(x - half, y + half);
    glEnd();
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x - half, y - half);
    glVertex2d(x + half, y - half);
    glVertex2d(x + half, y + half);
    glVertex2d(x - half, y + half);
    glEnd();
    glPopAttrib();
}

bool IntensityProfilePlotterInteract::draw(const OFX::DrawArgs& args)
{
    if (!_instance) return false;
    
    try {
        // Get point parameters
        double point1[2] = {0.2, 0.5}, point2[2] = {0.8, 0.5};
        OFX::Double2DParam* p1 = _instance->fetchDouble2DParam("point1");
        if (p1) p1->getValueAtTime(args.time, point1[0], point1[1]);
        
        OFX::Double2DParam* p2 = _instance->fetchDouble2DParam("point2");
        if (p2) p2->getValueAtTime(args.time, point2[0], point2[1]);
        
        // Convert to pixel coordinates
        double px1, py1, px2, py2;
        normalizedToPixel(point1[0], point1[1], px1, py1);
        normalizedToPixel(point2[0], point2[1], px2, py2);
        
        // Draw line
        drawLine(args, px1, py1, px2, py2);
        
        // Draw points
        drawPoint(args, px1, py1, _dragState == kDragPoint1);
        drawPoint(args, px2, py2, _dragState == kDragPoint2);
    } catch (...) {}
    
    return true;
}

bool IntensityProfilePlotterInteract::penDown(const OFX::PenArgs& args)
{
    if (!_instance) return false;
    
    try {
        // Get point parameters
        double point1[2] = {0.2, 0.5}, point2[2] = {0.8, 0.5};
        OFX::Double2DParam* p1 = _instance->fetchDouble2DParam("point1");
        if (p1) p1->getValueAtTime(args.time, point1[0], point1[1]);
        
        OFX::Double2DParam* p2 = _instance->fetchDouble2DParam("point2");
        if (p2) p2->getValueAtTime(args.time, point2[0], point2[1]);
        
        // Convert to pixel coordinates
        double px1 = point1[0] * 1920.0;
        double py1 = point1[1] * 1080.0;
        double px2 = point2[0] * 1920.0;
        double py2 = point2[1] * 1080.0;
        
        // Hit test points
        double pixelScale = args.pixelScale.x;
        if (hitTestPoint(args.penPosition.x, args.penPosition.y, px1, py1, pixelScale)) {
            _dragState = kDragPoint1;
            return true;
        } else if (hitTestPoint(args.penPosition.x, args.penPosition.y, px2, py2, pixelScale)) {
            _dragState = kDragPoint2;
            return true;
        }
    } catch (...) {}
    
    _dragState = kDragNone;
    return false;
}

bool IntensityProfilePlotterInteract::penMotion(const OFX::PenArgs& args)
{
    if (!_instance || _dragState == kDragNone) return false;
    
    try {
        // Convert pixel to normalized coordinates
        double nx = args.penPosition.x / 1920.0;
        double ny = args.penPosition.y / 1080.0;
        nx = std::max(0.0, std::min(1.0, nx));
        ny = std::max(0.0, std::min(1.0, ny));
        
        if (_dragState == kDragPoint1) {
            OFX::Double2DParam* p = _instance->fetchDouble2DParam("point1");
            if (p) p->setValue(nx, ny);
            return true;
        } else if (_dragState == kDragPoint2) {
            OFX::Double2DParam* p = _instance->fetchDouble2DParam("point2");
            if (p) p->setValue(nx, ny);
            return true;
        }
    } catch (...) {}
    
    return false;
}

bool IntensityProfilePlotterInteract::penUp(const OFX::PenArgs& args)
{
    if (_dragState != kDragNone) {
        _dragState = kDragNone;
        return true;
    }
    return false;
}
