#include "RenderEngine.h"
#include "../Common/Logger.h"
#include "../Common/ErrorHandler.h"
#include "../Common/WindowsCompat.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

RenderEngine::RenderEngine()
    : m_initialized(false)
    , m_hwnd(nullptr)
    , m_d2dFactory(nullptr)
    , m_renderTarget(nullptr)
    , m_writeFactory(nullptr)
    , m_globalTransparency(0.8f)
    , m_lastRenderTime(0)
    , m_frameCount(0)
    , m_averageFrameTime(0.0f)
{
    m_resources = std::make_unique<RenderResources>();
    Logger::Info("RenderEngine created");
}

RenderEngine::~RenderEngine()
{
    Shutdown();
    Logger::Info("RenderEngine destroyed");
}

bool RenderEngine::Initialize(HWND hwnd)
{
    Logger::Info("Initializing RenderEngine...");
    
    m_hwnd = hwnd;
    
    // 初始化Direct2D
    if (!InitializeDirect2D())
    {
        Logger::Error("Failed to initialize Direct2D");
        return false;
    }
    
    // 初始化DirectWrite
    if (!InitializeDirectWrite())
    {
        Logger::Error("Failed to initialize DirectWrite");
        return false;
    }
    
    // 创建渲染目标
    if (!CreateRenderTarget(hwnd))
    {
        Logger::Error("Failed to create render target");
        return false;
    }
    
    // 创建渲染资源
    if (!CreateRenderResources())
    {
        Logger::Error("Failed to create render resources");
        return false;
    }
    
    // 初始化动画状态
    for (int i = static_cast<int>(CardinalDirection::Front); 
         i <= static_cast<int>(CardinalDirection::BackRight); i++)
    {
        CardinalDirection dir = static_cast<CardinalDirection>(i);
        m_animations[dir] = AnimationState();
    }
    
    m_initialized = true;
    Logger::Info("RenderEngine initialized successfully");
    return true;
}

void RenderEngine::Shutdown()
{
    if (!m_initialized)
        return;
        
    Logger::Info("Shutting down RenderEngine...");
    
    CleanupRenderResources();
    
    if (m_renderTarget)
    {
        m_renderTarget->Release();
        m_renderTarget = nullptr;
    }
    
    if (m_writeFactory)
    {
        m_writeFactory->Release();
        m_writeFactory = nullptr;
    }
    
    if (m_d2dFactory)
    {
        m_d2dFactory->Release();
        m_d2dFactory = nullptr;
    }
    
    m_initialized = false;
    Logger::Info("RenderEngine shutdown complete");
}

void RenderEngine::Render(const ProcessedDirection& direction)
{
    if (!m_initialized || !m_renderTarget)
        return;
    
    DWORD currentTime = GetTickCount();
    
    // 更新动画
    UpdateAnimations();
    
    // 设置目标强度
    SetTargetIntensity(direction.primary, direction.intensity);
    
    // 开始渲染
    m_renderTarget->BeginDraw();
    
    // 清除背景
    RenderBackground();
    
    // 渲染指南针
    if (m_config.showCompass)
    {
        RenderCompass();
    }
    
    // 渲染主要方向指示器
    if (direction.primary != CardinalDirection::None)
    {
        float animatedIntensity = GetAnimatedIntensity(direction.primary);
        RenderDirectionIndicator(direction.primary, animatedIntensity);
    }
    
    // 渲染次要方向指示器
    for (CardinalDirection secondaryDir : direction.secondary)
    {
        float secondaryIntensity = direction.intensity * 0.6f; // 次要方向强度较低
        RenderDirectionIndicator(secondaryDir, secondaryIntensity);
    }
    
    // 渲染强度计
    if (m_config.showIntensityMeter)
    {
        RenderIntensityMeter(direction.intensity);
    }
    
    // 结束渲染
    HRESULT hr = m_renderTarget->EndDraw();
    
    if (hr == D2DERR_RECREATE_TARGET)
    {
        HandleDeviceLost();
    }
    else if (FAILED(hr))
    {
        ErrorHandler::HandleRenderError(RenderErrorType::DrawingError, "EndDraw failed");
    }
    
    // 更新性能统计
    if (m_lastRenderTime > 0)
    {
        float frameTime = static_cast<float>(currentTime - m_lastRenderTime);
        m_averageFrameTime = (m_averageFrameTime * m_frameCount + frameTime) / (m_frameCount + 1);
        m_frameCount++;
    }
    m_lastRenderTime = currentTime;
    
    m_currentDirection = direction;
}

void RenderEngine::Clear()
{
    if (!m_initialized || !m_renderTarget)
        return;
    
    m_renderTarget->BeginDraw();
    m_renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.0f));
    m_renderTarget->EndDraw();
}

void RenderEngine::SetTheme(const VisualTheme& theme)
{
    m_config.theme = theme;
    UpdateBrushColors();
    Logger::Debug("Visual theme updated");
}

void RenderEngine::SetTransparency(float alpha)
{
    m_globalTransparency = clamp(alpha, 0.0f, 1.0f);
    UpdateBrushColors();
    Logger::Debug("Transparency set to: " + std::to_string(m_globalTransparency));
}

void RenderEngine::UpdateConfig(const VisualConfig& config)
{
    m_config = config;
    UpdateBrushColors();
    Logger::Debug("Visual configuration updated");
}

bool RenderEngine::ResizeRenderTarget(UINT width, UINT height)
{
    if (!m_renderTarget)
        return false;
    
    D2D1_SIZE_U size = D2D1::SizeU(width, height);
    HRESULT hr = m_renderTarget->Resize(size);
    
    if (FAILED(hr))
    {
        ErrorHandler::HandleRenderError(RenderErrorType::ResourceCreationFailed, "Resize failed");
        return false;
    }
    
    Logger::Debug("Render target resized to " + std::to_string(width) + "x" + std::to_string(height));
    return true;
}

void RenderEngine::HandleDeviceLost()
{
    Logger::Warning("Render target lost, recreating...");
    
    CleanupRenderResources();
    
    if (m_renderTarget)
    {
        m_renderTarget->Release();
        m_renderTarget = nullptr;
    }
    
    // 重新创建渲染目标和资源
    if (CreateRenderTarget(m_hwnd) && CreateRenderResources())
    {
        Logger::Info("Render target recreated successfully");
    }
    else
    {
        ErrorHandler::HandleRenderError(RenderErrorType::RenderTargetLost, "Failed to recreate render target");
    }
}

D2D1_SIZE_F RenderEngine::GetRenderTargetSize() const
{
    if (m_renderTarget)
        return m_renderTarget->GetSize();
    
    return D2D1::SizeF(0, 0);
}

bool RenderEngine::InitializeDirect2D()
{
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2dFactory);
    
    if (FAILED(hr))
    {
        ErrorHandler::HandleRenderError(RenderErrorType::Direct2DInitFailed, "D2D1CreateFactory failed");
        return false;
    }
    
    return true;
}

bool RenderEngine::InitializeDirectWrite()
{
    HRESULT hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_writeFactory)
    );
    
    if (FAILED(hr))
    {
        ErrorHandler::HandleRenderError(RenderErrorType::Direct2DInitFailed, "DWriteCreateFactory failed");
        return false;
    }
    
    return true;
}

bool RenderEngine::CreateRenderTarget(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    
    D2D1_SIZE_U size = D2D1::SizeU(
        rc.right - rc.left,
        rc.bottom - rc.top
    );
    
    HRESULT hr = m_d2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, size),
        &m_renderTarget
    );
    
    if (FAILED(hr))
    {
        ErrorHandler::HandleRenderError(RenderErrorType::ResourceCreationFailed, "CreateHwndRenderTarget failed");
        return false;
    }
    
    return true;
}

bool RenderEngine::CreateRenderResources()
{
    if (!m_renderTarget)
        return false;
    
    HRESULT hr;
    
    // 创建画刷
    hr = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(m_config.theme.primaryColor.r, m_config.theme.primaryColor.g, 
                    m_config.theme.primaryColor.b, m_config.theme.primaryColor.a * m_globalTransparency),
        &m_resources->primaryBrush
    );
    if (FAILED(hr)) return false;
    
    hr = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(m_config.theme.secondaryColor.r, m_config.theme.secondaryColor.g,
                    m_config.theme.secondaryColor.b, m_config.theme.secondaryColor.a * m_globalTransparency),
        &m_resources->secondaryBrush
    );
    if (FAILED(hr)) return false;
    
    hr = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(m_config.theme.backgroundColor.r, m_config.theme.backgroundColor.g,
                    m_config.theme.backgroundColor.b, m_config.theme.backgroundColor.a * m_globalTransparency),
        &m_resources->backgroundBrush
    );
    if (FAILED(hr)) return false;
    
    hr = m_renderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::White, 0.8f * m_globalTransparency),
        &m_resources->textBrush
    );
    if (FAILED(hr)) return false;
    
    // 创建文本格式
    hr = m_writeFactory->CreateTextFormat(
        L"Arial",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        12.0f,
        L"en-us",
        &m_resources->textFormat
    );
    if (FAILED(hr)) return false;
    
    m_resources->textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_resources->textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    
    return true;
}

void RenderEngine::CleanupRenderResources()
{
    if (m_resources->primaryBrush)
    {
        m_resources->primaryBrush->Release();
        m_resources->primaryBrush = nullptr;
    }
    
    if (m_resources->secondaryBrush)
    {
        m_resources->secondaryBrush->Release();
        m_resources->secondaryBrush = nullptr;
    }
    
    if (m_resources->backgroundBrush)
    {
        m_resources->backgroundBrush->Release();
        m_resources->backgroundBrush = nullptr;
    }
    
    if (m_resources->textBrush)
    {
        m_resources->textBrush->Release();
        m_resources->textBrush = nullptr;
    }
    
    if (m_resources->textFormat)
    {
        m_resources->textFormat->Release();
        m_resources->textFormat = nullptr;
    }
}

void RenderEngine::RenderDirectionIndicator(CardinalDirection dir, float intensity)
{
    if (dir == CardinalDirection::None || intensity <= 0.0f)
        return;
    
    D2D1_POINT_2F position = GetDirectionPosition(dir);
    float size = GetIndicatorSize(dir, intensity);
    
    switch (m_config.theme.style)
    {
        case IndicatorStyle::Circle:
            RenderCircleIndicator(position, size, intensity);
            break;
        case IndicatorStyle::Arrow:
            RenderArrowIndicator(position, GetDirectionAngle(dir), intensity);
            break;
        case IndicatorStyle::Dot:
            RenderDotIndicator(position, intensity);
            break;
        case IndicatorStyle::Ring:
            RenderRingIndicator(position, size, intensity);
            break;
    }
}

void RenderEngine::RenderCompass()
{
    RenderCompassRose();
    RenderDirectionLabels();
}

void RenderEngine::RenderIntensityMeter(float intensity)
{
    D2D1_SIZE_F size = GetRenderTargetSize();
    
    // 在右上角渲染强度条
    D2D1_RECT_F barRect = D2D1::RectF(
        size.width - 30.0f,
        10.0f,
        size.width - 10.0f,
        10.0f + 100.0f * intensity
    );
    
    m_renderTarget->FillRectangle(barRect, m_resources->primaryBrush);
    
    // 边框
    D2D1_RECT_F borderRect = D2D1::RectF(
        size.width - 30.0f,
        10.0f,
        size.width - 10.0f,
        110.0f
    );
    
    m_renderTarget->DrawRectangle(borderRect, m_resources->secondaryBrush, 1.0f);
}

void RenderEngine::RenderBackground()
{
    D2D1_SIZE_F size = GetRenderTargetSize();
    D2D1_RECT_F rect = D2D1::RectF(0, 0, size.width, size.height);
    
    m_renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.0f));
    
    if (m_config.theme.backgroundColor.a > 0.0f)
    {
        m_renderTarget->FillRectangle(rect, m_resources->backgroundBrush);
    }
}

void RenderEngine::RenderCircleIndicator(const D2D1_POINT_2F& center, float radius, float intensity)
{
    D2D1_COLOR_F color = GetDirectionColor(CardinalDirection::Front, intensity);
    m_resources->primaryBrush->SetColor(color);
    
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(center, radius, radius);
    m_renderTarget->FillEllipse(ellipse, m_resources->primaryBrush);
    
    // 外边框
    m_renderTarget->DrawEllipse(ellipse, m_resources->secondaryBrush, 2.0f);
}

void RenderEngine::RenderArrowIndicator(const D2D1_POINT_2F& center, float angle, float intensity)
{
    float size = m_config.theme.indicatorSize * intensity;
    float angleRad = DegreesToRadians(angle);
    
    // 箭头顶点
    D2D1_POINT_2F tip = D2D1::Point2F(
        center.x + size * std::cos(angleRad),
        center.y + size * std::sin(angleRad)
    );
    
    // 箭头底部两点
    D2D1_POINT_2F base1 = D2D1::Point2F(
        center.x + (size * 0.6f) * std::cos(angleRad + 2.5f),
        center.y + (size * 0.6f) * std::sin(angleRad + 2.5f)
    );
    
    D2D1_POINT_2F base2 = D2D1::Point2F(
        center.x + (size * 0.6f) * std::cos(angleRad - 2.5f),
        center.y + (size * 0.6f) * std::sin(angleRad - 2.5f)
    );
    
    // 绘制箭头线条
    D2D1_COLOR_F color = GetDirectionColor(CardinalDirection::Front, intensity);
    m_resources->primaryBrush->SetColor(color);
    
    m_renderTarget->DrawLine(center, tip, m_resources->primaryBrush, 3.0f);
    m_renderTarget->DrawLine(tip, base1, m_resources->primaryBrush, 2.0f);
    m_renderTarget->DrawLine(tip, base2, m_resources->primaryBrush, 2.0f);
}

void RenderEngine::RenderDotIndicator(const D2D1_POINT_2F& center, float intensity)
{
    float radius = 5.0f + intensity * 10.0f;
    
    D2D1_COLOR_F color = GetDirectionColor(CardinalDirection::Front, intensity);
    m_resources->primaryBrush->SetColor(color);
    
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(center, radius, radius);
    m_renderTarget->FillEllipse(ellipse, m_resources->primaryBrush);
}

void RenderEngine::RenderRingIndicator(const D2D1_POINT_2F& center, float radius, float intensity)
{
    D2D1_COLOR_F color = GetDirectionColor(CardinalDirection::Front, intensity);
    m_resources->primaryBrush->SetColor(color);
    
    float strokeWidth = 2.0f + intensity * 4.0f;
    
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(center, radius, radius);
    m_renderTarget->DrawEllipse(ellipse, m_resources->primaryBrush, strokeWidth);
}

void RenderEngine::RenderCompassRose()
{
    D2D1_POINT_2F center = GetCenterPoint();
    float radius = 80.0f;
    
    // 绘制主要方向线
    for (int i = 0; i < 4; i++)
    {
        float angle = i * 90.0f;
        float angleRad = DegreesToRadians(angle);
        
        D2D1_POINT_2F start = D2D1::Point2F(
            center.x + (radius - 20.0f) * std::cos(angleRad),
            center.y + (radius - 20.0f) * std::sin(angleRad)
        );
        
        D2D1_POINT_2F end = D2D1::Point2F(
            center.x + radius * std::cos(angleRad),
            center.y + radius * std::sin(angleRad)
        );
        
        m_renderTarget->DrawLine(start, end, m_resources->secondaryBrush, 2.0f);
    }
    
    // 绘制次要方向线
    for (int i = 0; i < 4; i++)
    {
        float angle = i * 90.0f + 45.0f;
        float angleRad = DegreesToRadians(angle);
        
        D2D1_POINT_2F start = D2D1::Point2F(
            center.x + (radius - 15.0f) * std::cos(angleRad),
            center.y + (radius - 15.0f) * std::sin(angleRad)
        );
        
        D2D1_POINT_2F end = D2D1::Point2F(
            center.x + radius * std::cos(angleRad),
            center.y + radius * std::sin(angleRad)
        );
        
        m_renderTarget->DrawLine(start, end, m_resources->secondaryBrush, 1.0f);
    }
}

void RenderEngine::RenderDirectionLabels()
{
    D2D1_POINT_2F center = GetCenterPoint();
    float radius = 100.0f;
    
    const wchar_t* labels[] = { L"N", L"E", L"S", L"W" };
    
    for (int i = 0; i < 4; i++)
    {
        float angle = i * 90.0f;
        float angleRad = DegreesToRadians(angle);
        
        D2D1_POINT_2F position = D2D1::Point2F(
            center.x + radius * std::cos(angleRad),
            center.y + radius * std::sin(angleRad)
        );
        
        D2D1_RECT_F textRect = D2D1::RectF(
            position.x - 10.0f,
            position.y - 10.0f,
            position.x + 10.0f,
            position.y + 10.0f
        );
        
        m_renderTarget->DrawText(
            labels[i],
            1,
            m_resources->textFormat,
            textRect,
            m_resources->textBrush
        );
    }
}

void RenderEngine::UpdateAnimations()
{
    DWORD currentTime = GetTickCount();
    
    for (auto& pair : m_animations)
    {
        AnimationState& state = pair.second;
        
        if (state.isAnimating)
        {
            float deltaTime = (currentTime - state.lastUpdateTime) / 1000.0f;
            float diff = state.targetIntensity - state.currentIntensity;
            
            if (std::abs(diff) > 0.01f)
            {
                state.currentIntensity += diff * state.animationSpeed * deltaTime;
            }
            else
            {
                state.currentIntensity = state.targetIntensity;
                state.isAnimating = false;
            }
        }
        
        state.lastUpdateTime = currentTime;
    }
}

float RenderEngine::GetAnimatedIntensity(CardinalDirection direction)
{
    auto it = m_animations.find(direction);
    if (it != m_animations.end())
    {
        return it->second.currentIntensity;
    }
    return 0.0f;
}

void RenderEngine::SetTargetIntensity(CardinalDirection direction, float intensity)
{
    auto it = m_animations.find(direction);
    if (it != m_animations.end())
    {
        it->second.targetIntensity = intensity;
        it->second.isAnimating = true;
    }
}

D2D1_POINT_2F RenderEngine::GetDirectionPosition(CardinalDirection direction)
{
    D2D1_POINT_2F center = GetCenterPoint();
    float radius = 60.0f;
    float angle = GetDirectionAngle(direction);
    float angleRad = DegreesToRadians(angle);
    
    return D2D1::Point2F(
        center.x + radius * std::cos(angleRad),
        center.y + radius * std::sin(angleRad)
    );
}

float RenderEngine::GetDirectionAngle(CardinalDirection direction)
{
    switch (direction)
    {
        case CardinalDirection::Front:      return 0.0f;
        case CardinalDirection::FrontRight: return 45.0f;
        case CardinalDirection::Right:      return 90.0f;
        case CardinalDirection::BackRight:  return 135.0f;
        case CardinalDirection::Back:       return 180.0f;
        case CardinalDirection::BackLeft:   return 225.0f;
        case CardinalDirection::Left:       return 270.0f;
        case CardinalDirection::FrontLeft:  return 315.0f;
        case CardinalDirection::Up:         return 0.0f;   // 特殊处理
        case CardinalDirection::Down:       return 0.0f;   // 特殊处理
        default:                            return 0.0f;
    }
}

D2D1_POINT_2F RenderEngine::GetCenterPoint()
{
    D2D1_SIZE_F size = GetRenderTargetSize();
    return D2D1::Point2F(size.width * 0.5f, size.height * 0.5f);
}

D2D1_COLOR_F RenderEngine::GetDirectionColor(CardinalDirection direction, float intensity)
{
    D2D1_COLOR_F baseColor = m_config.theme.primaryColor;
    
    // 根据强度调整颜色亮度
    float alpha = baseColor.a * intensity * m_globalTransparency;
    
    return D2D1::ColorF(baseColor.r, baseColor.g, baseColor.b, alpha);
}

float RenderEngine::GetIndicatorSize(CardinalDirection direction, float intensity)
{
    return m_config.theme.indicatorSize * (0.5f + intensity * 0.5f);
}

void RenderEngine::UpdateBrushColors()
{
    if (!m_initialized || !m_resources->primaryBrush)
        return;
    
    m_resources->primaryBrush->SetColor(
        D2D1::ColorF(m_config.theme.primaryColor.r, m_config.theme.primaryColor.g,
                    m_config.theme.primaryColor.b, m_config.theme.primaryColor.a * m_globalTransparency)
    );
    
    m_resources->secondaryBrush->SetColor(
        D2D1::ColorF(m_config.theme.secondaryColor.r, m_config.theme.secondaryColor.g,
                    m_config.theme.secondaryColor.b, m_config.theme.secondaryColor.a * m_globalTransparency)
    );
    
    m_resources->backgroundBrush->SetColor(
        D2D1::ColorF(m_config.theme.backgroundColor.r, m_config.theme.backgroundColor.g,
                    m_config.theme.backgroundColor.b, m_config.theme.backgroundColor.a * m_globalTransparency)
    );
}

float RenderEngine::DegreesToRadians(float degrees)
{
    return degrees * M_PI / 180.0f;
}