#include "IntensityProfilePlotterInteract.h"
#include "IntensityProfilePlotterPlugin.h"
#include "ofxInteract.h"
#include "ofxParam.h"

#include <cmath>
#include <algorithm>
#include <OpenGL/gl.h>

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
    
    // TODO: Implement drawing using DrawSuite C API
    // DrawSuite must be fetched from host
    
    // Get current parameter values
    double point1[2], point2[2];
    _instance->getPoint1Param()->getValueAtTime(args.time, point1[0], point1[1]);
    _instance->getPoint2Param()->getValueAtTime(args.time, point2[0], point2[1]);
    double rectPos[2], rectSize[2];
    _instance->getPlotRectPosParam()->getValueAtTime(args.time, rectPos[0], rectPos[1]);
    _instance->getPlotRectSizeParam()->getValueAtTime(args.time, rectSize[0], rectSize[1]);

    // Convert normalized to pixel coordinates
    double px1, py1, px2, py2;
    normalizedToPixel(point1[0], point1[1], px1, py1);
    normalizedToPixel(point2[0], point2[1], px2, py2);
    double rx, ry;
    normalizedToPixel(rectPos[0], rectPos[1], rx, ry);
    double rw = rectSize[0] * 1920.0;
    double rh = rectSize[1] * 1080.0;

    // Draw rectangle and handles
    bool rectSelected = (_dragState == kDragRectMove || _dragState == kDragRectTL || _dragState == kDragRectTR || _dragState == kDragRectBL || _dragState == kDragRectBR);
    drawRect(args, rx, ry, rw, rh, rectSelected);
    drawHandle(args, rx, ry, _dragState == kDragRectTL);
    drawHandle(args, rx + rw, ry, _dragState == kDragRectTR);
    drawHandle(args, rx, ry + rh, _dragState == kDragRectBL);
    drawHandle(args, rx + rw, ry + rh, _dragState == kDragRectBR);

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
    
    // Rect info
    double rectPos[2], rectSize[2];
    _instance->getPlotRectPosParam()->getValueAtTime(args.time, rectPos[0], rectPos[1]);
    _instance->getPlotRectSizeParam()->getValueAtTime(args.time, rectSize[0], rectSize[1]);
    double rx, ry;
    normalizedToPixel(rectPos[0], rectPos[1], rx, ry);
    double rw = rectSize[0] * 1920.0;
    double rh = rectSize[1] * 1080.0;

    // Hit test points first (higher priority)
    double pixelScale = args.pixelScale.x;
    if (hitTestPoint(args.penPosition.x, args.penPosition.y, px1, py1, pixelScale)) {
        _dragState = kDragPoint1;
        return true;
    } else if (hitTestPoint(args.penPosition.x, args.penPosition.y, px2, py2, pixelScale)) {
        _dragState = kDragPoint2;
        return true;
    }

    // Then test rectangle handles
    int rectHandle = hitTestRectHandles(args.penPosition.x, args.penPosition.y, rx, ry, rw, rh, pixelScale);
    if (rectHandle != -1) {
        _dragState = static_cast<DragState>(rectHandle);
        _rectDragStartX = args.penPosition.x;
        _rectDragStartY = args.penPosition.y;
        _rectStartPos[0] = rectPos[0];
        _rectStartPos[1] = rectPos[1];
        _rectStartSize[0] = rectSize[0];
        _rectStartSize[1] = rectSize[1];
        return true;
    }

    // Then test rectangle body for move
    if (hitTestRectBody(args.penPosition.x, args.penPosition.y, rx, ry, rw, rh)) {
        _dragState = kDragRectMove;
        _rectDragStartX = args.penPosition.x;
        _rectDragStartY = args.penPosition.y;
        _rectStartPos[0] = rectPos[0];
        _rectStartPos[1] = rectPos[1];
        _rectStartSize[0] = rectSize[0];
        _rectStartSize[1] = rectSize[1];
        return true;
    }
    
    // Then test the line connecting the points
    double t = 0.0;
    if (hitTestLine(args.penPosition.x, args.penPosition.y, px1, py1, px2, py2, pixelScale, t)) {
        _dragState = kDragLine;
        _lineDragOffset = t; // Store position along the line
        _lastMouseX = args.penPosition.x;
        _lastMouseY = args.penPosition.y;
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
    nx = std::max(0.0, std::min(1.0, nx));
    ny = std::max(0.0, std::min(1.0, ny));

    // Update parameter
    if (_dragState == kDragPoint1) {
        _instance->getPoint1Param()->setValue(nx, ny);
    } else if (_dragState == kDragPoint2) {
        _instance->getPoint2Param()->setValue(nx, ny);
    } else if (_dragState == kDragLine) {
        double point1[2], point2[2];
        _instance->getPoint1Param()->getValueAtTime(args.time, point1[0], point1[1]);
        _instance->getPoint2Param()->getValueAtTime(args.time, point2[0], point2[1]);

        double deltaX = args.penPosition.x - _lastMouseX;
        double deltaY = args.penPosition.y - _lastMouseY;
        double deltaNormX = deltaX / 1920.0;
        double deltaNormY = deltaY / 1080.0;

        double newP1X = point1[0] + deltaNormX;
        double newP1Y = point1[1] + deltaNormY;
        double newP2X = point2[0] + deltaNormX;
        double newP2Y = point2[1] + deltaNormY;

        newP1X = std::max(0.0, std::min(1.0, newP1X));
        newP1Y = std::max(0.0, std::min(1.0, newP1Y));
        newP2X = std::max(0.0, std::min(1.0, newP2X));
        newP2Y = std::max(0.0, std::min(1.0, newP2Y));

        _instance->getPoint1Param()->setValue(newP1X, newP1Y);
        _instance->getPoint2Param()->setValue(newP2X, newP2Y);

        _lastMouseX = args.penPosition.x;
        _lastMouseY = args.penPosition.y;
    } else if (_dragState == kDragRectMove || _dragState == kDragRectTL || _dragState == kDragRectTR ||
               _dragState == kDragRectBL || _dragState == kDragRectBR) {
        double deltaX = (args.penPosition.x - _rectDragStartX) / 1920.0;
        double deltaY = (args.penPosition.y - _rectDragStartY) / 1080.0;

        double newPosX = _rectStartPos[0];
        double newPosY = _rectStartPos[1];
        double newSizeX = _rectStartSize[0];
        double newSizeY = _rectStartSize[1];

        const double minSize = 0.05;

        if (_dragState == kDragRectMove) {
            newPosX = _rectStartPos[0] + deltaX;
            newPosY = _rectStartPos[1] + deltaY;
        } else {
            // Resize by adjusting the relevant edges
            if (_dragState == kDragRectTL || _dragState == kDragRectBL) {
                double newRight = _rectStartPos[0] + _rectStartSize[0];
                newPosX = _rectStartPos[0] + deltaX;
                newPosX = std::max(0.0, std::min(newPosX, newRight - minSize));
                newSizeX = newRight - newPosX;
            }
            if (_dragState == kDragRectTR || _dragState == kDragRectBR) {
                double newWidth = _rectStartSize[0] + deltaX;
                newSizeX = std::max(minSize, newWidth);
            }
            if (_dragState == kDragRectTL || _dragState == kDragRectTR) {
                double newBottom = _rectStartPos[1] + _rectStartSize[1];
                newPosY = _rectStartPos[1] + deltaY;
                newPosY = std::max(0.0, std::min(newPosY, newBottom - minSize));
                newSizeY = newBottom - newPosY;
            }
            if (_dragState == kDragRectBL || _dragState == kDragRectBR) {
                double newHeight = _rectStartSize[1] + deltaY;
                newSizeY = std::max(minSize, newHeight);
            }
        }

        // Clamp within [0,1]
        newPosX = std::max(0.0, std::min(1.0 - newSizeX, newPosX));
        newPosY = std::max(0.0, std::min(1.0 - newSizeY, newPosY));
        newSizeX = std::max(minSize, std::min(1.0 - newPosX, newSizeX));
        newSizeY = std::max(minSize, std::min(1.0 - newPosY, newSizeY));

        _instance->getPlotRectPosParam()->setValue(newPosX, newPosY);
        _instance->getPlotRectSizeParam()->setValue(newSizeX, newSizeY);
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
