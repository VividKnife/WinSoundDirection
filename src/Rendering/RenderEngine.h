#pragma once

#include "../Common/Types.h"
#include "../Common/Config.h"
#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <map>
#include <memory>

// 渲染资源管理
struct RenderResources
{
    ID2D1SolidColorBrush* primaryBrush;
    ID2D1SolidColorBrush* secondaryBrush;
    ID2D1SolidColorBrush* backgroundBrush;
    ID2D1SolidColorBrush* textBrush;
    IDWriteTextFormat* textFormat;
    
    RenderResources() : primaryBrush(nullptr), secondaryBrush(nullptr), 
                       backgroundBrush(nullptr), textBrush(nullptr), textFormat(nullptr) {}
};

// 动画状态
struct AnimationState
{
    float currentIntensity;
    float targetIntensity;
    float animationSpeed;
    DWORD lastUpdateTime;
    bool isAnimating;
    
    AnimationState() : currentIntensity(0.0f), targetIntensity(0.0f), 
                      animationSpeed(5.0f), lastUpdateTime(0), isAnimating(false) {}
};

class RenderEngine
{
public:
    RenderEngine();
    ~RenderEngine();

    // 初始化和清理
    bool Initialize(HWND hwnd);
    void Shutdown();

    // 主要渲染方法
    void Render(const ProcessedDirection& direction);
    void Clear();

    // 配置管理
    void SetTheme(const VisualTheme& theme);
    void SetTransparency(float alpha);
    void UpdateConfig(const VisualConfig& config);

    // 窗口管理
    bool ResizeRenderTarget(UINT width, UINT height);
    void HandleDeviceLost();

    // 渲染状态
    bool IsInitialized() const { return m_initialized; }
    D2D1_SIZE_F GetRenderTargetSize() const;

private:
    // Direct2D初始化
    bool InitializeDirect2D();
    bool InitializeDirectWrite();
    bool CreateRenderTarget(HWND hwnd);
    bool CreateRenderResources();
    void CleanupRenderResources();

    // 核心渲染方法
    void RenderDirectionIndicator(CardinalDirection dir, float intensity);
    void RenderCompass();
    void RenderIntensityMeter(float intensity);
    void RenderBackground();

    // 指示器渲染
    void RenderCircleIndicator(const D2D1_POINT_2F& center, float radius, float intensity);
    void RenderArrowIndicator(const D2D1_POINT_2F& center, float angle, float intensity);
    void RenderDotIndicator(const D2D1_POINT_2F& center, float intensity);
    void RenderRingIndicator(const D2D1_POINT_2F& center, float radius, float intensity);

    // 辅助渲染
    void RenderCompassRose();
    void RenderDirectionLabels();
    void RenderIntensityBar(float intensity);

    // 动画系统
    void UpdateAnimations();
    float GetAnimatedIntensity(CardinalDirection direction);
    void SetTargetIntensity(CardinalDirection direction, float intensity);

    // 坐标转换
    D2D1_POINT_2F GetDirectionPosition(CardinalDirection direction);
    float GetDirectionAngle(CardinalDirection direction);
    D2D1_POINT_2F GetCenterPoint();

    // 颜色和样式
    D2D1_COLOR_F GetDirectionColor(CardinalDirection direction, float intensity);
    float GetIndicatorSize(CardinalDirection direction, float intensity);
    void UpdateBrushColors();

    // 数学工具
    D2D1_POINT_2F RotatePoint(const D2D1_POINT_2F& point, const D2D1_POINT_2F& center, float angle);
    float DegreesToRadians(float degrees);

    // 成员变量
    bool m_initialized;
    HWND m_hwnd;
    VisualConfig m_config;
    
    // Direct2D接口
    ID2D1Factory* m_d2dFactory;
    ID2D1HwndRenderTarget* m_renderTarget;
    
    // DirectWrite接口
    IDWriteFactory* m_writeFactory;
    
    // 渲染资源
    std::unique_ptr<RenderResources> m_resources;
    
    // 动画状态
    std::map<CardinalDirection, AnimationState> m_animations;
    
    // 当前渲染状态
    ProcessedDirection m_currentDirection;
    float m_globalTransparency;
    
    // 性能统计
    DWORD m_lastRenderTime;
    int m_frameCount;
    float m_averageFrameTime;
};