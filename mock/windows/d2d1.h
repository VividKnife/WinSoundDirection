#pragma once
// Mock Direct2D headers for cross-platform development

#ifdef MOCK_WINDOWS_APIS

#include "windows.h"

// Forward declarations
struct ID2D1Factory;
struct ID2D1HwndRenderTarget;
struct ID2D1SolidColorBrush;

// Color structure
typedef struct D2D1_COLOR_F {
    float r;
    float g;
    float b;
    float a;
} D2D1_COLOR_F;

// Point structure
typedef struct D2D1_POINT_2F {
    float x;
    float y;
} D2D1_POINT_2F;

// Size structure
typedef struct D2D1_SIZE_F {
    float width;
    float height;
} D2D1_SIZE_F;

typedef struct D2D1_SIZE_U {
    UINT width;
    UINT height;
} D2D1_SIZE_U;

// Rectangle structure
typedef struct D2D1_RECT_F {
    float left;
    float top;
    float right;
    float bottom;
} D2D1_RECT_F;

// Ellipse structure
typedef struct D2D1_ELLIPSE {
    D2D1_POINT_2F point;
    float radiusX;
    float radiusY;
} D2D1_ELLIPSE;

// Factory type
typedef enum D2D1_FACTORY_TYPE {
    D2D1_FACTORY_TYPE_SINGLE_THREADED = 0,
    D2D1_FACTORY_TYPE_MULTI_THREADED = 1
} D2D1_FACTORY_TYPE;

// Render target properties
typedef struct D2D1_RENDER_TARGET_PROPERTIES {
    int dummy;
} D2D1_RENDER_TARGET_PROPERTIES;

typedef struct D2D1_HWND_RENDER_TARGET_PROPERTIES {
    HWND hwnd;
    D2D1_SIZE_U pixelSize;
} D2D1_HWND_RENDER_TARGET_PROPERTIES;

// Mock interfaces
struct ID2D1Factory {
    virtual long CreateHwndRenderTarget(
        const D2D1_RENDER_TARGET_PROPERTIES* renderTargetProperties,
        const D2D1_HWND_RENDER_TARGET_PROPERTIES* hwndRenderTargetProperties,
        ID2D1HwndRenderTarget** hwndRenderTarget
    ) = 0;
    virtual void Release() = 0;
};

struct ID2D1HwndRenderTarget {
    virtual void BeginDraw() = 0;
    virtual long EndDraw() = 0;
    virtual void Clear(const D2D1_COLOR_F* clearColor) = 0;
    virtual D2D1_SIZE_F GetSize() = 0;
    virtual long Resize(const D2D1_SIZE_U* pixelSize) = 0;
    virtual long CreateSolidColorBrush(const D2D1_COLOR_F* color, ID2D1SolidColorBrush** solidColorBrush) = 0;
    virtual void FillEllipse(const D2D1_ELLIPSE* ellipse, ID2D1SolidColorBrush* brush) = 0;
    virtual void DrawEllipse(const D2D1_ELLIPSE* ellipse, ID2D1SolidColorBrush* brush, float strokeWidth) = 0;
    virtual void FillRectangle(const D2D1_RECT_F* rect, ID2D1SolidColorBrush* brush) = 0;
    virtual void DrawRectangle(const D2D1_RECT_F* rect, ID2D1SolidColorBrush* brush, float strokeWidth) = 0;
    virtual void DrawLine(D2D1_POINT_2F point0, D2D1_POINT_2F point1, ID2D1SolidColorBrush* brush, float strokeWidth) = 0;
    virtual void Release() = 0;
};

struct ID2D1SolidColorBrush {
    virtual void SetColor(const D2D1_COLOR_F* color) = 0;
    virtual void Release() = 0;
};

// Helper functions
namespace D2D1 {
    inline D2D1_COLOR_F ColorF(float r, float g, float b, float a = 1.0f) {
        D2D1_COLOR_F color = {r, g, b, a};
        return color;
    }
    
    inline D2D1_POINT_2F Point2F(float x, float y) {
        D2D1_POINT_2F point = {x, y};
        return point;
    }
    
    inline D2D1_SIZE_F SizeF(float width, float height) {
        D2D1_SIZE_F size = {width, height};
        return size;
    }
    
    inline D2D1_SIZE_U SizeU(UINT width, UINT height) {
        D2D1_SIZE_U size = {width, height};
        return size;
    }
    
    inline D2D1_RECT_F RectF(float left, float top, float right, float bottom) {
        D2D1_RECT_F rect = {left, top, right, bottom};
        return rect;
    }
    
    inline D2D1_ELLIPSE Ellipse(D2D1_POINT_2F center, float radiusX, float radiusY) {
        D2D1_ELLIPSE ellipse = {center, radiusX, radiusY};
        return ellipse;
    }
    
    inline D2D1_RENDER_TARGET_PROPERTIES RenderTargetProperties() {
        D2D1_RENDER_TARGET_PROPERTIES props = {0};
        return props;
    }
    
    inline D2D1_HWND_RENDER_TARGET_PROPERTIES HwndRenderTargetProperties(HWND hwnd, D2D1_SIZE_U pixelSize) {
        D2D1_HWND_RENDER_TARGET_PROPERTIES props = {hwnd, pixelSize};
        return props;
    }
}

// Mock factory creation function
inline long D2D1CreateFactory(D2D1_FACTORY_TYPE factoryType, ID2D1Factory** factory) {
    *factory = nullptr;
    return E_FAIL;
}

// Error codes
#define D2DERR_RECREATE_TARGET 0x8899000CL

#endif // MOCK_WINDOWS_APIS