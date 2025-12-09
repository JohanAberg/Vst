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
    , _lineP1Start{0.0, 0.0}
    , _lineP2Start{0.0, 0.0}
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    
    const int segments = 20;
    
    // Draw black shadow halo for visibility
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(x, y);
    for (int i = 0; i <= segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        double dx = (POINT_DISPLAY_RADIUS + 3.0) * cos(angle);
        double dy = (POINT_DISPLAY_RADIUS + 3.0) * sin(angle);
        glVertex2d(x + dx, y + dy);
    }
    glEnd();
    
    // Draw filled circle with bright color
    if (selected) {
        glColor3f(1.0f, 0.5f, 0.0f); // Bright orange when selected
    } else {
        glColor3f(0.0f, 1.0f, 1.0f); // Cyan normally
    }
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(x, y);
    for (int i = 0; i <= segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        double dx = POINT_DISPLAY_RADIUS * cos(angle);
        double dy = POINT_DISPLAY_RADIUS * sin(angle);
        glVertex2d(x + dx, y + dy);
    }
    glEnd();
    
    // Draw white outline for contrast
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        double dx = POINT_DISPLAY_RADIUS * cos(angle);
        double dy = POINT_DISPLAY_RADIUS * sin(angle);
        glVertex2d(x + dx, y + dy);
    }
    glEnd();
    
    // Draw inner black outline for extra definition
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        double angle = 2.0 * M_PI * i / segments;
        double dx = (POINT_DISPLAY_RADIUS - 1.0) * cos(angle);
        double dy = (POINT_DISPLAY_RADIUS - 1.0) * sin(angle);
        glVertex2d(x + dx, y + dy);
    }
    glEnd();
    
    glPopAttrib();
}

void IntensityProfilePlotterInteract::drawLine(const OFX::DrawArgs& args, double x1, double y1, double x2, double y2)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    // Draw black shadow outline for visibility
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    glLineWidth(7.0f);
    glBegin(GL_LINES);
    glVertex2d(x1, y1);
    glVertex2d(x2, y2);
    glEnd();
    
    // Draw main line in cyan
    glColor3f(0.0f, 1.0f, 1.0f);
    glLineWidth(3.0f);
    glBegin(GL_LINES);
    glVertex2d(x1, y1);
    glVertex2d(x2, y2);
    glEnd();
    
    // Draw white center line for extra pop
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINES);
    glVertex2d(x1, y1);
    glVertex2d(x2, y2);
    glEnd();
    
    // Draw midpoint rectangle hint for drag affordance
    double midX = (x1 + x2) * 0.5;
    double midY = (y1 + y2) * 0.5;
    double rectSize = 8.0;
    
    // Draw dark shadow quad for depth
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2d(midX - rectSize - 1, midY - rectSize - 1);
    glVertex2d(midX + rectSize + 1, midY - rectSize - 1);
    glVertex2d(midX + rectSize + 1, midY + rectSize + 1);
    glVertex2d(midX - rectSize - 1, midY + rectSize + 1);
    glEnd();
    
    // Draw light gray fill
    glColor4f(0.7f, 0.7f, 0.7f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2d(midX - rectSize, midY - rectSize);
    glVertex2d(midX + rectSize, midY - rectSize);
    glVertex2d(midX + rectSize, midY + rectSize);
    glVertex2d(midX - rectSize, midY + rectSize);
    glEnd();
    
    // Draw white outline
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(midX - rectSize, midY - rectSize);
    glVertex2d(midX + rectSize, midY - rectSize);
    glVertex2d(midX + rectSize, midY + rectSize);
    glVertex2d(midX - rectSize, midY + rectSize);
    glEnd();
    
    glPopAttrib();
}

void IntensityProfilePlotterInteract::drawRect(const OFX::DrawArgs& args, double rx, double ry, double rw, double rh, bool selected)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POLYGON_SMOOTH);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    
    double half = HANDLE_SIZE * 0.5;
    
    // Draw black shadow
    glColor4f(0.0f, 0.0f, 0.0f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2d(x - half - 2, y - half - 2);
    glVertex2d(x + half + 2, y - half - 2);
    glVertex2d(x + half + 2, y + half + 2);
    glVertex2d(x - half - 2, y + half + 2);
    glEnd();
    
    // Draw filled square with bright color
    if (selected) {
        glColor3f(1.0f, 0.5f, 0.0f); // Bright orange when selected
    } else {
        glColor3f(1.0f, 0.0f, 1.0f); // Magenta normally
    }
    glBegin(GL_QUADS);
    glVertex2d(x - half, y - half);
    glVertex2d(x + half, y - half);
    glVertex2d(x + half, y + half);
    glVertex2d(x - half, y + half);
    glEnd();
    
    // Draw white outline
    glColor3f(1.0f, 1.0f, 1.0f);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x - half, y - half);
    glVertex2d(x + half, y - half);
    glVertex2d(x + half, y + half);
    glVertex2d(x - half, y + half);
    glEnd();
    
    // Draw inner black outline
    glColor3f(0.0f, 0.0f, 0.0f);
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x - half + 1, y - half + 1);
    glVertex2d(x + half - 1, y - half + 1);
    glVertex2d(x + half - 1, y + half - 1);
    glVertex2d(x - half + 1, y + half - 1);
    glEnd();
    
    glPopAttrib();
}

// Simple bitmap font for displaying text in OpenGL
// Uses 5x7 pixel grid for each character
void IntensityProfilePlotterInteract::drawText(double x, double y, const char* text, double scale)
{
    if (!text) return;
    
    // Simple 5x7 bitmap font (only ASCII characters we need)
    // Each character is represented as 7 bytes (rows), 5 bits wide
    static const unsigned char font5x7[][7] = {
        // 'A' = 65
        {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},
        // 'C' = 67
        {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E},
        // 'E' = 69
        {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},
        // 'G' = 71
        {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F},
        // 'L' = 76
        {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},
        // 'M' = 77
        {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11},
        // 'N' = 78
        {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},
        // 'O' = 79
        {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
        // 'P' = 80
        {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},
        // 'T' = 84
        {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},
        // 'U' = 85
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},
        // '(' = 40
        {0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02},
        // ')' = 41
        {0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08},
        // ' ' = 32 (space)
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    };
    
    // Map characters to font array indices
    auto getCharIndex = [](char c) -> int {
        switch(c) {
            case 'A': return 0;
            case 'C': return 1;
            case 'E': return 2;
            case 'G': return 3;
            case 'L': return 4;
            case 'M': return 5;
            case 'N': return 6;
            case 'O': return 7;
            case 'P': return 8;
            case 'T': return 9;
            case 'U': return 10;
            case '(': return 11;
            case ')': return 12;
            case ' ': return 13;
            default: return 13; // Space for unknown chars
        }
    };
    
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    double charWidth = 6.0 * scale;  // 5 pixels + 1 pixel spacing
    double charHeight = 8.0 * scale; // 7 pixels + 1 pixel spacing
    double pixelSize = scale;
    
    double cursorX = x;
    
    for (const char* p = text; *p; ++p) {
        int idx = getCharIndex(*p);
        const unsigned char* glyph = font5x7[idx];
        
        // Draw each pixel of the character
        for (int row = 0; row < 7; ++row) {
            unsigned char rowData = glyph[row];
            for (int col = 0; col < 5; ++col) {
                if (rowData & (1 << (4 - col))) {
                    // Draw black shadow for readability
                    glColor4f(0.0f, 0.0f, 0.0f, 0.9f);
                    glBegin(GL_QUADS);
                    glVertex2d(cursorX + col * pixelSize - 1, y - row * pixelSize - 1);
                    glVertex2d(cursorX + (col + 1) * pixelSize + 1, y - row * pixelSize - 1);
                    glVertex2d(cursorX + (col + 1) * pixelSize + 1, y - (row + 1) * pixelSize + 1);
                    glVertex2d(cursorX + col * pixelSize - 1, y - (row + 1) * pixelSize + 1);
                    glEnd();
                    
                    // Draw white pixel
                    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                    glBegin(GL_QUADS);
                    glVertex2d(cursorX + col * pixelSize, y - row * pixelSize);
                    glVertex2d(cursorX + (col + 1) * pixelSize, y - row * pixelSize);
                    glVertex2d(cursorX + (col + 1) * pixelSize, y - (row + 1) * pixelSize);
                    glVertex2d(cursorX + col * pixelSize, y - (row + 1) * pixelSize);
                    glEnd();
                }
            }
        }
        
        cursorX += charWidth;
    }
    
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
    if (!rectPosParam) rectPosParam = _instance->fetchDouble2DParam("plotRectPos");
    OFX::Double2DParam* rectSizeParam = _instance->getPlotRectSizeParam();
    if (!rectSizeParam) rectSizeParam = _instance->fetchDouble2DParam("plotRectSize");
    if (rectPosParam) rectPosParam->getValueAtTime(args.time, rectPos[0], rectPos[1]);
    if (rectSizeParam) rectSizeParam->getValueAtTime(args.time, rectSize[0], rectSize[1]);

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

    // Draw dashed reference line at whitepoint = 1.0
    if (whitePoint > 0.0) {
        const double refY = rectY + (1.0 / whitePoint) * rectH;
        if (refY >= rectY && refY <= rectY + rectH) {
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(2, 0xAAAA); // Dashed pattern
            glColor3f(0.6f, 0.6f, 0.6f);
            glLineWidth(1.5f);
            glBegin(GL_LINES);
            glVertex2d(rectX, refY);
            glVertex2d(rectX + rectW, refY);
            glEnd();
            glDisable(GL_LINE_STIPPLE);
            
            // Draw \"1\" marker next to the line
            glColor3f(0.8f, 0.8f, 0.8f);
            glBegin(GL_LINES);
            // Draw a small \"1\" shape
            glVertex2d(rectX + rectW + 8, refY - 8);
            glVertex2d(rectX + rectW + 8, refY + 8);
            glVertex2d(rectX + rectW + 6, refY - 6);
            glVertex2d(rectX + rectW + 8, refY - 8);
            glEnd();
        }
    }

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
        auto getFrameSize = [&](int& w, int& h) {
            w = h = 0;
            if (auto srcClip = _instance->getSourceClip()) {
                // Prefer full frame ROD to avoid being constrained by tile-sized fetches
                OfxRectD rod = srcClip->getRegionOfDefinition(args.time);
                w = static_cast<int>(std::round(rod.x2 - rod.x1));
                h = static_cast<int>(std::round(rod.y2 - rod.y1));
                if (w <= 0 || h <= 0) {
                    std::unique_ptr<OFX::Image> src(srcClip->fetchImage(args.time));
                    if (src) {
                        OfxRectI b = src->getBounds();
                        w = b.x2 - b.x1;
                        h = b.y2 - b.y1;
                    }
                }
            }
        };

        // Get point parameters
        double point1[2] = {0.2, 0.5}, point2[2] = {0.8, 0.5};
        OFX::Double2DParam* p1 = _instance->fetchDouble2DParam("point1");
        if (p1) p1->getValueAtTime(args.time, point1[0], point1[1]);
        
        OFX::Double2DParam* p2 = _instance->fetchDouble2DParam("point2");
        if (p2) p2->getValueAtTime(args.time, point2[0], point2[1]);

        int imgW = 0;
        int imgH = 0;
        getFrameSize(imgW, imgH);
        std::unique_ptr<OFX::Image> src;
        if (auto srcClip = _instance->getSourceClip()) {
            src.reset(srcClip->fetchImage(args.time));
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

        // Get plot rect parameters
        double rectPos[2] = {0.05, 0.05};
        double rectSize[2] = {0.3, 0.2};
        OFX::Double2DParam* rectPosParam = _instance->fetchDouble2DParam("plotRectPos");
        if (rectPosParam) rectPosParam->getValueAtTime(args.time, rectPos[0], rectPos[1]);
        OFX::Double2DParam* rectSizeParam = _instance->fetchDouble2DParam("plotRectSize");
        if (rectSizeParam) rectSizeParam->getValueAtTime(args.time, rectSize[0], rectSize[1]);
        
        double rx = rectPos[0] * width;
        double ry = rectPos[1] * height;
        double rw = rectSize[0] * width;
        double rh = rectSize[1] * height;
        
        // Draw plot of sampled RGB values along the line
        if (src) {
            drawPlot(args, *src, imgW, imgH, point1[0], point1[1], point2[0], point2[1]);
        }
        
        // Always draw resize handles on plot rect corners
        drawHandle(args, rx, ry, _dragState == kDragRectTL);
        drawHandle(args, rx + rw, ry, _dragState == kDragRectTR);
        drawHandle(args, rx, ry + rh, _dragState == kDragRectBL);
        drawHandle(args, rx + rw, ry + rh, _dragState == kDragRectBR);
        
        // Draw renderer status text in top-left corner
        const char* rendererName = _instance->getRendererName();
        if (rendererName && rendererName[0]) {
            // Position text in top-left with some padding
            double textX = 20.0;
            double textY = height - 20.0;
            double textScale = 2.0; // Make text larger for visibility
            
            // Draw semi-transparent background box for better readability
            double textWidth = 0;
            for (const char* p = rendererName; *p; ++p) {
                textWidth += 6.0 * textScale;
            }
            double textHeight = 8.0 * textScale;
            
            glPushAttrib(GL_ALL_ATTRIB_BITS);
            glDisable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            // Background box
            glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
            glBegin(GL_QUADS);
            glVertex2d(textX - 5, textY + 5);
            glVertex2d(textX + textWidth + 5, textY + 5);
            glVertex2d(textX + textWidth + 5, textY - textHeight - 5);
            glVertex2d(textX - 5, textY - textHeight - 5);
            glEnd();
            
            glPopAttrib();
            
            // Draw the text
            drawText(textX, textY, rendererName, textScale);
        }
    } catch (...) {}
    
    return true;
}

bool IntensityProfilePlotterInteract::penDown(const OFX::PenArgs& args)
{
    if (!_instance) return false;
    
    try {
        auto getFrameSize = [&](int& w, int& h) {
            w = h = 0;
            if (auto srcClip = _instance->getSourceClip()) {
                OfxRectD rod = srcClip->getRegionOfDefinition(args.time);
                w = static_cast<int>(std::round(rod.x2 - rod.x1));
                h = static_cast<int>(std::round(rod.y2 - rod.y1));
                if (w <= 0 || h <= 0) {
                    std::unique_ptr<OFX::Image> src(srcClip->fetchImage(args.time));
                    if (src) {
                        OfxRectI b = src->getBounds();
                        w = b.x2 - b.x1;
                        h = b.y2 - b.y1;
                    }
                }
            }
        };

        // Get point parameters
        double point1[2] = {0.2, 0.5}, point2[2] = {0.8, 0.5};
        OFX::Double2DParam* p1 = _instance->fetchDouble2DParam("point1");
        if (p1) p1->getValueAtTime(args.time, point1[0], point1[1]);
        
        OFX::Double2DParam* p2 = _instance->fetchDouble2DParam("point2");
        if (p2) p2->getValueAtTime(args.time, point2[0], point2[1]);
        
        int imgW = 0;
        int imgH = 0;
        getFrameSize(imgW, imgH);

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
        
        // Hit test line (for dragging both points together)
        double t = 0.0;
        if (hitTestLine(args.penPosition.x, args.penPosition.y, px1, py1, px2, py2, pixelScale, t)) {
            _dragState = kDragLine;
            _lastMouseX = args.penPosition.x;
            _lastMouseY = args.penPosition.y;
            _lineP1Start[0] = point1[0];
            _lineP1Start[1] = point1[1];
            _lineP2Start[0] = point2[0];
            _lineP2Start[1] = point2[1];
            return true;
        }
        
        // Hit test plot rectangle
        double rectPos[2] = {0.05, 0.05};
        double rectSize[2] = {0.3, 0.2};
        OFX::Double2DParam* rectPosParam = _instance->fetchDouble2DParam("plotRectPos");
        if (rectPosParam) rectPosParam->getValueAtTime(args.time, rectPos[0], rectPos[1]);
        OFX::Double2DParam* rectSizeParam = _instance->fetchDouble2DParam("plotRectSize");
        if (rectSizeParam) rectSizeParam->getValueAtTime(args.time, rectSize[0], rectSize[1]);
        
        double rx = rectPos[0] * width;
        double ry = rectPos[1] * height;
        double rw = rectSize[0] * width;
        double rh = rectSize[1] * height;
        
        // Hit test rect handles first (higher priority)
        int handleHit = hitTestRectHandles(args.penPosition.x, args.penPosition.y, rx, ry, rw, rh, pixelScale);
        if (handleHit >= 0) {
            _dragState = static_cast<DragState>(handleHit);
            _lastMouseX = args.penPosition.x;
            _lastMouseY = args.penPosition.y;
            _rectStartPos[0] = rectPos[0];
            _rectStartPos[1] = rectPos[1];
            _rectStartSize[0] = rectSize[0];
            _rectStartSize[1] = rectSize[1];
            return true;
        }
        
        // Hit test rect body (for moving)
        if (hitTestRectBody(args.penPosition.x, args.penPosition.y, rx, ry, rw, rh)) {
            _dragState = kDragRectMove;
            _lastMouseX = args.penPosition.x;
            _lastMouseY = args.penPosition.y;
            _rectStartPos[0] = rectPos[0];
            _rectStartPos[1] = rectPos[1];
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
        auto getFrameSize = [&](int& w, int& h) {
            w = h = 0;
            if (auto srcClip = _instance->getSourceClip()) {
                OfxRectD rod = srcClip->getRegionOfDefinition(args.time);
                w = static_cast<int>(std::round(rod.x2 - rod.x1));
                h = static_cast<int>(std::round(rod.y2 - rod.y1));
                if (w <= 0 || h <= 0) {
                    std::unique_ptr<OFX::Image> src(srcClip->fetchImage(args.time));
                    if (src) {
                        OfxRectI b = src->getBounds();
                        w = b.x2 - b.x1;
                        h = b.y2 - b.y1;
                    }
                }
            }
        };

        int imgW = 0;
        int imgH = 0;
        getFrameSize(imgW, imgH);

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
        } else if (_dragState == kDragLine) {
            // Calculate delta from drag start in normalized coords
            double dx = (args.penPosition.x - _lastMouseX) / width;
            double dy = (args.penPosition.y - _lastMouseY) / height;
            
            // Apply delta to both points
            double newP1X = std::max(0.0, std::min(1.0, _lineP1Start[0] + dx));
            double newP1Y = std::max(0.0, std::min(1.0, _lineP1Start[1] + dy));
            double newP2X = std::max(0.0, std::min(1.0, _lineP2Start[0] + dx));
            double newP2Y = std::max(0.0, std::min(1.0, _lineP2Start[1] + dy));
            
            OFX::Double2DParam* p1 = _instance->fetchDouble2DParam("point1");
            OFX::Double2DParam* p2 = _instance->fetchDouble2DParam("point2");
            if (p1) p1->setValue(newP1X, newP1Y);
            if (p2) p2->setValue(newP2X, newP2Y);
            return true;
        } else if (_dragState == kDragRectMove) {
            // Move rect
            double dx = (args.penPosition.x - _lastMouseX) / width;
            double dy = (args.penPosition.y - _lastMouseY) / height;
            double newX = std::max(0.0, std::min(1.0 - _rectStartSize[0], _rectStartPos[0] + dx));
            double newY = std::max(0.0, std::min(1.0 - _rectStartSize[1], _rectStartPos[1] + dy));
            
            OFX::Double2DParam* rectPosParam = _instance->fetchDouble2DParam("plotRectPos");
            if (rectPosParam) rectPosParam->setValue(newX, newY);
            return true;
        } else if (_dragState >= kDragRectTL && _dragState <= kDragRectBR) {
            // Resize rect by dragging corner handles
            double dx = (args.penPosition.x - _lastMouseX) / width;
            double dy = (args.penPosition.y - _lastMouseY) / height;
            
            double newX = _rectStartPos[0];
            double newY = _rectStartPos[1];
            double newW = _rectStartSize[0];
            double newH = _rectStartSize[1];
            
            if (_dragState == kDragRectTL) {
                newX = std::max(0.0, std::min(_rectStartPos[0] + _rectStartSize[0] - 0.05, _rectStartPos[0] + dx));
                newY = std::max(0.0, std::min(_rectStartPos[1] + _rectStartSize[1] - 0.05, _rectStartPos[1] + dy));
                newW = _rectStartSize[0] - (newX - _rectStartPos[0]);
                newH = _rectStartSize[1] - (newY - _rectStartPos[1]);
            } else if (_dragState == kDragRectTR) {
                newY = std::max(0.0, std::min(_rectStartPos[1] + _rectStartSize[1] - 0.05, _rectStartPos[1] + dy));
                newW = std::max(0.05, std::min(1.0 - _rectStartPos[0], _rectStartSize[0] + dx));
                newH = _rectStartSize[1] - (newY - _rectStartPos[1]);
            } else if (_dragState == kDragRectBL) {
                newX = std::max(0.0, std::min(_rectStartPos[0] + _rectStartSize[0] - 0.05, _rectStartPos[0] + dx));
                newW = _rectStartSize[0] - (newX - _rectStartPos[0]);
                newH = std::max(0.05, std::min(1.0 - _rectStartPos[1], _rectStartSize[1] + dy));
            } else if (_dragState == kDragRectBR) {
                newW = std::max(0.05, std::min(1.0 - _rectStartPos[0], _rectStartSize[0] + dx));
                newH = std::max(0.05, std::min(1.0 - _rectStartPos[1], _rectStartSize[1] + dy));
            }
            
            OFX::Double2DParam* rectPosParam = _instance->fetchDouble2DParam("plotRectPos");
            OFX::Double2DParam* rectSizeParam = _instance->fetchDouble2DParam("plotRectSize");
            if (rectPosParam) rectPosParam->setValue(newX, newY);
            if (rectSizeParam) rectSizeParam->setValue(newW, newH);
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
