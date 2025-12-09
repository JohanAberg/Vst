#include "IntensityProfilePlotterInteract.h"
#include "IntensityProfilePlotterPlugin.h"
#include "ofxInteract.h"
#include "ofxParam.h"

#include <cmath>
#include <algorithm>
#include <vector>
#include <memory>

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

void IntensityProfilePlotterInteract::drawPlot(const OFX::DrawArgs& args, OFX::Image& src, int imgW, int imgH, double nx1, double ny1, double nx2, double ny2)
{
    if (!_instance) return;
    if (imgW <= 1 || imgH <= 1) return;

    OfxRectI bounds = src.getBounds();

    int comps = 0;
    switch (src.getPixelComponents()) {
    case OFX::ePixelComponentRGBA: comps = 4; break;
    case OFX::ePixelComponentRGB:  comps = 3; break;
    default: return;
    }

    int sampleCount = 256;
    OFX::IntParam* sampleCountParam = _instance->getSampleCountParam();
    if (!sampleCountParam) {
        sampleCountParam = _instance->fetchIntParam("sampleCount");
    }
    if (sampleCountParam) sampleCountParam->getValueAtTime(args.time, sampleCount);
    sampleCount = std::max(8, std::min(2048, sampleCount));

    double whitePoint = 1.0;
    OFX::DoubleParam* whitePointParam = _instance->getWhitePointParam();
    if (!whitePointParam) {
        whitePointParam = _instance->fetchDoubleParam("whitePoint");
    }
    if (whitePointParam) whitePointParam->getValueAtTime(args.time, whitePoint);
    if (whitePoint <= 0.0) whitePoint = 1.0;

    int lineWidth = 2;
    OFX::IntParam* lineWidthParam = _instance->getLineWidthParam();
    if (!lineWidthParam) {
        // Fallback: fetch if not cached
        lineWidthParam = _instance->fetchIntParam("lineWidth");
    }
    if (lineWidthParam) {
        lineWidthParam->getValueAtTime(args.time, lineWidth);
    }
    lineWidth = std::max(1, std::min(20, lineWidth));

    // Fetch curve colors
    double redColor[4] = {1.0, 0.2, 0.2, 1.0};
    double greenColor[4] = {0.2, 1.0, 0.2, 1.0};
    double blueColor[4] = {0.2, 0.4, 1.0, 1.0};
    OFX::RGBAParam* redParam = _instance->getRedCurveColorParam();
    OFX::RGBAParam* greenParam = _instance->getGreenCurveColorParam();
    OFX::RGBAParam* blueParam = _instance->getBlueCurveColorParam();
    if (!redParam) redParam = _instance->fetchRGBAParam("redCurveColor");
    if (!greenParam) greenParam = _instance->fetchRGBAParam("greenCurveColor");
    if (!blueParam) blueParam = _instance->fetchRGBAParam("blueCurveColor");
    if (redParam) redParam->getValueAtTime(args.time, redColor[0], redColor[1], redColor[2], redColor[3]);
    if (greenParam) greenParam->getValueAtTime(args.time, greenColor[0], greenColor[1], greenColor[2], greenColor[3]);
    if (blueParam) blueParam->getValueAtTime(args.time, blueColor[0], blueColor[1], blueColor[2], blueColor[3]);

    // Plot rect (normalized to image, map to pixels using image dims)
    double rectPos[2] = {0.05, 0.05};
    double rectSize[2] = {0.3, 0.2};
    OFX::Double2DParam* rectPosParam = _instance->getPlotRectPosParam();
    OFX::Double2DParam* rectSizeParam = _instance->getPlotRectSizeParam();
    if (rectPosParam) rectPosParam->getValue(rectPos[0], rectPos[1]);
    if (rectSizeParam) rectSizeParam->getValue(rectSize[0], rectSize[1]);

    const double rectX = rectPos[0] * imgW;
    const double rectY = rectPos[1] * imgH;
    const double rectW = rectSize[0] * imgW;
    const double rectH = rectSize[1] * imgH;

    std::vector<float> r(sampleCount), g(sampleCount), b(sampleCount);

    for (int i = 0; i < sampleCount; ++i) {
        const double t = (sampleCount == 1) ? 0.0 : static_cast<double>(i) / (sampleCount - 1);
        const double xn = nx1 + t * (nx2 - nx1);
        const double yn = ny1 + t * (ny2 - ny1);
        int ix = static_cast<int>(std::round(xn * imgW));
        int iy = static_cast<int>(std::round(yn * imgH));
        ix = std::clamp(ix, bounds.x1, bounds.x2 - 1);
        iy = std::clamp(iy, bounds.y1, bounds.y2 - 1);

        const float* px = reinterpret_cast<const float*>(src.getPixelAddress(ix, iy));
        if (!px) {
            r[i] = g[i] = b[i] = 0.0f;
            continue;
        }
        r[i] = px[0];
        if (comps >= 2) g[i] = px[1]; else g[i] = r[i];
        if (comps >= 3) b[i] = px[2]; else b[i] = r[i];
    }

    // Optional: draw reference ramp background
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);

    glColor4f(0.05f, 0.05f, 0.05f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2d(rectX, rectY);
    glVertex2d(rectX + rectW, rectY);
    glVertex2d(rectX + rectW, rectY + rectH);
    glVertex2d(rectX, rectY + rectH);
    glEnd();

    glColor3f(0.2f, 0.2f, 0.2f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(rectX, rectY);
    glVertex2d(rectX + rectW, rectY);
    glVertex2d(rectX + rectW, rectY + rectH);
    glVertex2d(rectX, rectY + rectH);
    glEnd();

    glLineWidth(static_cast<float>(lineWidth));
    
    auto plotChannel = [&](const std::vector<float>& c, double rC, double gC, double bC) {
        glColor3f(static_cast<float>(rC), static_cast<float>(gC), static_cast<float>(bC));
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < sampleCount; ++i) {
            const double t = (sampleCount == 1) ? 0.0 : static_cast<double>(i) / (sampleCount - 1);
            const double x = rectX + t * rectW;
            const double v = std::min(whitePoint, std::max(0.0, static_cast<double>(c[i])));
            const double y = rectY + (v / whitePoint) * rectH;
            glVertex2d(x, y);
        }
        glEnd();
    };

    plotChannel(r, redColor[0], redColor[1], redColor[2]);
    plotChannel(g, greenColor[0], greenColor[1], greenColor[2]);
    plotChannel(b, blueColor[0], blueColor[1], blueColor[2]);

    glPopAttrib();
}

bool IntensityProfilePlotterInteract::draw(const OFX::DrawArgs& args)
{
    if (!_instance && _effect) {
        _instance = dynamic_cast<IntensityProfilePlotterPlugin*>(_effect);
    }
    if (!_instance) return false;
    
    try {
        // Get point parameters
        double point1[2] = {0.2, 0.5}, point2[2] = {0.8, 0.5};
        OFX::Double2DParam* p1 = _instance->fetchDouble2DParam("point1");
        if (p1) p1->getValueAtTime(args.time, point1[0], point1[1]);
        
        OFX::Double2DParam* p2 = _instance->fetchDouble2DParam("point2");
        if (p2) p2->getValueAtTime(args.time, point2[0], point2[1]);

        int imgW = 0;
        int imgH = 0;
        std::unique_ptr<OFX::Image> src;
        if (auto srcClip = _instance->getSourceClip()) {
            src.reset(srcClip->fetchImage(args.time));
            if (src) {
                OfxRectI b = src->getBounds();
                imgW = b.x2 - b.x1;
                imgH = b.y2 - b.y1;
            }
        }

        const double width = imgW > 0 ? static_cast<double>(imgW) : 1920.0;
        const double height = imgH > 0 ? static_cast<double>(imgH) : 1080.0;

        // Convert to pixel coordinates using image dimensions when available
        double px1 = point1[0] * width;
        double py1 = point1[1] * height;
        double px2 = point2[0] * width;
        double py2 = point2[1] * height;
        
        // Draw line
        drawLine(args, px1, py1, px2, py2);
        
        // Draw points
        drawPoint(args, px1, py1, _dragState == kDragPoint1);
        drawPoint(args, px2, py2, _dragState == kDragPoint2);

        // Draw plot of sampled RGB values along the line
        if (src) {
            drawPlot(args, *src, imgW, imgH, point1[0], point1[1], point2[0], point2[1]);
        }
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
        
        int imgW = 0;
        int imgH = 0;
        if (auto srcClip = _instance->getSourceClip()) {
            std::unique_ptr<OFX::Image> src(srcClip->fetchImage(args.time));
            if (src) {
                OfxRectI b = src->getBounds();
                imgW = b.x2 - b.x1;
                imgH = b.y2 - b.y1;
            }
        }

        const double width = imgW > 0 ? static_cast<double>(imgW) : 1920.0;
        const double height = imgH > 0 ? static_cast<double>(imgH) : 1080.0;

        // Convert to pixel coordinates
        double px1 = point1[0] * width;
        double py1 = point1[1] * height;
        double px2 = point2[0] * width;
        double py2 = point2[1] * height;
        
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
        int imgW = 0;
        int imgH = 0;
        if (auto srcClip = _instance->getSourceClip()) {
            std::unique_ptr<OFX::Image> src(srcClip->fetchImage(args.time));
            if (src) {
                OfxRectI b = src->getBounds();
                imgW = b.x2 - b.x1;
                imgH = b.y2 - b.y1;
            }
        }

        const double width = imgW > 0 ? static_cast<double>(imgW) : 1920.0;
        const double height = imgH > 0 ? static_cast<double>(imgH) : 1080.0;

        // Convert pixel to normalized coordinates using image dimensions
        double nx = args.penPosition.x / width;
        double ny = args.penPosition.y / height;
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
