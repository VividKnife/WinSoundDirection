#include "WindowManager.h"
#include "../Common/Logger.h"
#include "../Common/ErrorHandler.h"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

const wchar_t* WindowManager::WINDOW_CLASS_NAME = L"SpatialAudioVisualizerOverlay";
bool WindowManager::s_classRegistered = false;

WindowManager::WindowManager()
    : m_hInstance(nullptr)
    , m_overlayWindow(nullptr)
    , m_isVisible(false)
    , m_isInitialized(false)
    , m_alwaysOnTop(true)
    , m_clickThrough(false)
    , m_isDragging(false)
    , m_fullscreenCompatibilityEnabled(true)
{
    m_fullscreenState = std::make_unique<FullscreenState>();
    memset(&m_dragStartPos, 0, sizeof(POINT));
    memset(&m_windowStartPos, 0, sizeof(POINT));
    
    Logger::Info("WindowManager created");
}

WindowManager::~WindowManager()
{
    Shutdown();
    Logger::Info("WindowManager destroyed");
}

bool WindowManager::Initialize(HINSTANCE hInstance, const WindowConfig& config)
{
    Logger::Info("Initializing WindowManager...");
    
    m_hInstance = hInstance;
    m_config = config;
    
    // 注册窗口类
    RegisterWindowClass();
    
    // 创建覆盖窗口
    if (!CreateOverlayWindow())
    {
        Logger::Error("Failed to create overlay window");
        return false;
    }
    
    // 应用配置
    UpdateConfig(config);
    
    // 启动定时器
    SetTimer(m_overlayWindow, FULLSCREEN_CHECK_TIMER, 1000, nullptr); // 每秒检查一次全屏
    SetTimer(m_overlayWindow, TOPMOST_ENSURE_TIMER, 5000, nullptr);   // 每5秒确保置顶
    
    m_isInitialized = true;
    Logger::Info("WindowManager initialized successfully");
    return true;
}

void WindowManager::Shutdown()
{
    if (!m_isInitialized)
        return;
        
    Logger::Info("Shutting down WindowManager...");
    
    // 停止定时器
    if (m_overlayWindow)
    {
        KillTimer(m_overlayWindow, FULLSCREEN_CHECK_TIMER);
        KillTimer(m_overlayWindow, TOPMOST_ENSURE_TIMER);
    }
    
    // 销毁窗口
    DestroyWindow();
    
    // 注销窗口类
    UnregisterWindowClass();
    
    m_isInitialized = false;
    Logger::Info("WindowManager shutdown complete");
}

bool WindowManager::CreateOverlayWindow()
{
    Logger::Debug("Creating overlay window...");
    
    // 创建窗口
    m_overlayWindow = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        WINDOW_CLASS_NAME,
        L"Spatial Audio Visualizer",
        WS_POPUP,
        m_config.position.x,
        m_config.position.y,
        m_config.size.width,
        m_config.size.height,
        nullptr,
        nullptr,
        m_hInstance,
        this // 传递this指针给窗口过程
    );
    
    if (!m_overlayWindow)
    {
        DWORD error = GetLastError();
        ErrorHandler::HandleWindowError(WindowErrorType::CreationFailed, 
                                      "CreateWindowEx failed with error: " + std::to_string(error));
        return false;
    }
    
    // 应用窗口样式
    ApplyWindowStyles();
    
    // 设置透明度
    SetTransparency(0.8f);
    
    // 显示窗口
    if (m_config.startMinimized)
    {
        HideWindow();
    }
    else
    {
        ShowWindow();
    }
    
    Logger::Debug("Overlay window created successfully");
    return true;
}

void WindowManager::DestroyWindow()
{
    if (m_overlayWindow)
    {
        ::DestroyWindow(m_overlayWindow);
        m_overlayWindow = nullptr;
        Logger::Debug("Overlay window destroyed");
    }
}

void WindowManager::SetAlwaysOnTop(bool enable)
{
    m_alwaysOnTop = enable;
    m_config.alwaysOnTop = enable;
    
    if (m_overlayWindow)
    {
        HWND insertAfter = enable ? HWND_TOPMOST : HWND_NOTOPMOST;
        
        if (!SetWindowPos(m_overlayWindow, insertAfter, 0, 0, 0, 0, 
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
        {
            ErrorHandler::HandleWindowError(WindowErrorType::SetTopMostFailed, 
                                          "SetWindowPos failed for topmost");
        }
        else
        {
            Logger::Debug("Always on top: " + std::string(enable ? "enabled" : "disabled"));
        }
    }
}

void WindowManager::SetClickThrough(bool enable)
{
    m_clickThrough = enable;
    m_config.clickThrough = enable;
    
    if (m_overlayWindow)
    {
        LONG_PTR exStyle = GetWindowLongPtr(m_overlayWindow, GWL_EXSTYLE);
        
        if (enable)
        {
            exStyle |= WS_EX_TRANSPARENT;
        }
        else
        {
            exStyle &= ~WS_EX_TRANSPARENT;
        }
        
        SetWindowLongPtr(m_overlayWindow, GWL_EXSTYLE, exStyle);
        Logger::Debug("Click through: " + std::string(enable ? "enabled" : "disabled"));
    }
}

void WindowManager::SetPosition(int x, int y)
{
    m_config.position.x = x;
    m_config.position.y = y;
    
    if (m_overlayWindow)
    {
        SetWindowPos(m_overlayWindow, nullptr, x, y, 0, 0, 
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        
        ClampWindowToScreen();
        Logger::Debug("Window position set to (" + std::to_string(x) + ", " + std::to_string(y) + ")");
    }
}

void WindowManager::SetSize(int width, int height)
{
    m_config.size.width = width;
    m_config.size.height = height;
    
    if (m_overlayWindow)
    {
        SetWindowPos(m_overlayWindow, nullptr, 0, 0, width, height, 
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        
        Logger::Debug("Window size set to " + std::to_string(width) + "x" + std::to_string(height));
    }
}

void WindowManager::SetTransparency(float alpha)
{
    if (m_overlayWindow)
    {
        BYTE alphaValue = static_cast<BYTE>(alpha * 255);
        
        if (!SetLayeredWindowAttributes(m_overlayWindow, 0, alphaValue, LWA_ALPHA))
        {
            ErrorHandler::HandleWindowError(WindowErrorType::PositionError, 
                                          "SetLayeredWindowAttributes failed");
        }
        else
        {
            Logger::Debug("Window transparency set to: " + std::to_string(alpha));
        }
    }
}

void WindowManager::ShowWindow()
{
    if (m_overlayWindow && !m_isVisible)
    {
        ::ShowWindow(m_overlayWindow, SW_SHOWNOACTIVATE);
        m_isVisible = true;
        
        if (m_eventCallback)
        {
            m_eventCallback(WindowEvent::Restored, 0);
        }
        
        Logger::Debug("Window shown");
    }
}

void WindowManager::HideWindow()
{
    if (m_overlayWindow && m_isVisible)
    {
        ::ShowWindow(m_overlayWindow, SW_HIDE);
        m_isVisible = false;
        
        if (m_eventCallback)
        {
            m_eventCallback(WindowEvent::Minimized, 0);
        }
        
        Logger::Debug("Window hidden");
    }
}

void WindowManager::UpdateConfig(const WindowConfig& config)
{
    m_config = config;
    
    if (m_overlayWindow)
    {
        SetPosition(config.position.x, config.position.y);
        SetSize(config.size.width, config.size.height);
        SetAlwaysOnTop(config.alwaysOnTop);
        SetClickThrough(config.clickThrough);
        
        if (config.hideInFullscreen)
        {
            EnableFullscreenCompatibility(true);
        }
    }
    
    Logger::Debug("Window configuration updated");
}

bool WindowManager::IsFullscreenApplicationRunning()
{
    CheckFullscreenApplications();
    return m_fullscreenState->isFullscreenDetected;
}

void WindowManager::EnableFullscreenCompatibility(bool enable)
{
    m_fullscreenCompatibilityEnabled = enable;
    Logger::Debug("Fullscreen compatibility: " + std::string(enable ? "enabled" : "disabled"));
}

void WindowManager::SetEventCallback(std::function<void(WindowEvent, LPARAM)> callback)
{
    m_eventCallback = callback;
}

void WindowManager::EnableDragging(bool enable)
{
    // 拖拽功能通过鼠标消息处理实现
    Logger::Debug("Dragging: " + std::string(enable ? "enabled" : "disabled"));
}

LRESULT CALLBACK WindowManager::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WindowManager* windowManager = nullptr;
    
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        windowManager = static_cast<WindowManager*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(windowManager));
    }
    else
    {
        windowManager = reinterpret_cast<WindowManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
    
    if (windowManager)
    {
        return windowManager->HandleMessage(hwnd, msg, wParam, lParam);
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT WindowManager::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
            OnPaint(hwnd);
            return 0;
            
        case WM_SIZE:
            OnSize(hwnd, wParam, lParam);
            return 0;
            
        case WM_MOVE:
            OnMove(hwnd, lParam);
            return 0;
            
        case WM_LBUTTONDOWN:
            OnMouseDown(hwnd, wParam, lParam);
            return 0;
            
        case WM_LBUTTONUP:
            OnMouseUp(hwnd, wParam, lParam);
            return 0;
            
        case WM_MOUSEMOVE:
            OnMouseMove(hwnd, lParam);
            return 0;
            
        case WM_RBUTTONUP:
            OnRightClick(hwnd, lParam);
            return 0;
            
        case WM_LBUTTONDBLCLK:
            OnDoubleClick(hwnd, lParam);
            return 0;
            
        case WM_TIMER:
            if (wParam == FULLSCREEN_CHECK_TIMER)
            {
                HandleFullscreenDetection();
            }
            else if (wParam == TOPMOST_ENSURE_TIMER)
            {
                EnsureTopmost();
            }
            return 0;
            
        case WM_CLOSE:
            if (m_eventCallback)
            {
                m_eventCallback(WindowEvent::Closed, lParam);
            }
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void WindowManager::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    
    // 渲染由RenderEngine处理，这里只是验证绘制区域
    
    EndPaint(hwnd, &ps);
}

void WindowManager::OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);
    
    m_config.size.width = width;
    m_config.size.height = height;
    
    if (m_eventCallback)
    {
        m_eventCallback(WindowEvent::Resized, lParam);
    }
}

void WindowManager::OnMove(HWND hwnd, LPARAM lParam)
{
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    
    m_config.position.x = x;
    m_config.position.y = y;
    
    if (m_eventCallback)
    {
        m_eventCallback(WindowEvent::Moved, lParam);
    }
}

void WindowManager::OnMouseDown(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (!m_clickThrough)
    {
        m_isDragging = true;
        SetCapture(hwnd);
        
        GetCursorPos(&m_dragStartPos);
        
        RECT windowRect;
        GetWindowRect(hwnd, &windowRect);
        m_windowStartPos.x = windowRect.left;
        m_windowStartPos.y = windowRect.top;
    }
}

void WindowManager::OnMouseUp(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    if (m_isDragging)
    {
        m_isDragging = false;
        ReleaseCapture();
        
        SaveWindowPosition();
    }
}

void WindowManager::OnMouseMove(HWND hwnd, LPARAM lParam)
{
    if (m_isDragging)
    {
        POINT currentPos;
        GetCursorPos(&currentPos);
        
        int deltaX = currentPos.x - m_dragStartPos.x;
        int deltaY = currentPos.y - m_dragStartPos.y;
        
        int newX = m_windowStartPos.x + deltaX;
        int newY = m_windowStartPos.y + deltaY;
        
        SetPosition(newX, newY);
    }
}

void WindowManager::OnRightClick(HWND hwnd, LPARAM lParam)
{
    if (m_eventCallback)
    {
        m_eventCallback(WindowEvent::RightClick, lParam);
    }
}

void WindowManager::OnDoubleClick(HWND hwnd, LPARAM lParam)
{
    if (m_eventCallback)
    {
        m_eventCallback(WindowEvent::DoubleClick, lParam);
    }
}

void WindowManager::ApplyWindowStyles()
{
    if (!m_overlayWindow)
        return;
    
    // 设置扩展样式
    LONG_PTR exStyle = WS_EX_LAYERED | WS_EX_NOACTIVATE;
    
    if (m_alwaysOnTop)
        exStyle |= WS_EX_TOPMOST;
    
    if (m_clickThrough)
        exStyle |= WS_EX_TRANSPARENT;
    
    SetWindowLongPtr(m_overlayWindow, GWL_EXSTYLE, exStyle);
    
    // 设置窗口样式
    SetWindowLongPtr(m_overlayWindow, GWL_STYLE, WS_POPUP);
}

void WindowManager::UpdateLayeredWindowAttributes()
{
    if (m_overlayWindow)
    {
        SetLayeredWindowAttributes(m_overlayWindow, 0, 200, LWA_ALPHA);
    }
}

void WindowManager::HandleFullscreenDetection()
{
    if (!m_fullscreenCompatibilityEnabled)
        return;
    
    bool wasFullscreen = m_fullscreenState->isFullscreenDetected;
    CheckFullscreenApplications();
    bool isFullscreen = m_fullscreenState->isFullscreenDetected;
    
    if (wasFullscreen != isFullscreen)
    {
        if (isFullscreen && m_config.hideInFullscreen)
        {
            Logger::Debug("Fullscreen application detected, hiding window");
            HideWindow();
        }
        else if (!isFullscreen && m_config.hideInFullscreen)
        {
            Logger::Debug("Fullscreen application closed, showing window");
            ShowWindow();
        }
    }
}

void WindowManager::EnsureTopmost()
{
    if (m_alwaysOnTop && m_overlayWindow && m_isVisible)
    {
        SetWindowPos(m_overlayWindow, HWND_TOPMOST, 0, 0, 0, 0, 
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void WindowManager::CheckFullscreenApplications()
{
    HWND foregroundWindow = GetForegroundWindow();
    
    if (foregroundWindow && foregroundWindow != m_overlayWindow)
    {
        bool isFullscreen = IsWindowFullscreen(foregroundWindow);
        
        m_fullscreenState->isFullscreenDetected = isFullscreen;
        m_fullscreenState->fullscreenWindow = isFullscreen ? foregroundWindow : nullptr;
        m_fullscreenState->lastCheckTime = GetTickCount();
    }
}

bool WindowManager::IsWindowFullscreen(HWND hwnd)
{
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo = { sizeof(MONITORINFO) };
    
    if (GetMonitorInfo(monitor, &monitorInfo))
    {
        RECT screenRect = monitorInfo.rcMonitor;
        
        // 检查窗口是否覆盖整个屏幕
        return (windowRect.left <= screenRect.left &&
                windowRect.top <= screenRect.top &&
                windowRect.right >= screenRect.right &&
                windowRect.bottom >= screenRect.bottom);
    }
    
    return false;
}

void WindowManager::ClampWindowToScreen()
{
    if (!m_overlayWindow)
        return;
    
    RECT workArea = GetWorkArea();
    RECT windowRect;
    GetWindowRect(m_overlayWindow, &windowRect);
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    int x = std::max(workArea.left, std::min(windowRect.left, workArea.right - windowWidth));
    int y = std::max(workArea.top, std::min(windowRect.top, workArea.bottom - windowHeight));
    
    if (x != windowRect.left || y != windowRect.top)
    {
        SetWindowPos(m_overlayWindow, nullptr, x, y, 0, 0, 
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

RECT WindowManager::GetWorkArea()
{
    RECT workArea;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
    return workArea;
}

void WindowManager::SaveWindowPosition()
{
    if (m_overlayWindow)
    {
        RECT rect;
        GetWindowRect(m_overlayWindow, &rect);
        
        m_config.position.x = rect.left;
        m_config.position.y = rect.top;
        
        Logger::Debug("Window position saved: (" + 
                     std::to_string(rect.left) + ", " + std::to_string(rect.top) + ")");
    }
}

void WindowManager::RestoreWindowPosition()
{
    SetPosition(m_config.position.x, m_config.position.y);
}

void WindowManager::RegisterWindowClass()
{
    if (s_classRegistered)
        return;
    
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr; // 透明背景
    wc.lpszClassName = WINDOW_CLASS_NAME;
    
    if (RegisterClassExW(&wc))
    {
        s_classRegistered = true;
        Logger::Debug("Window class registered");
    }
    else
    {
        ErrorHandler::HandleWindowError(WindowErrorType::CreationFailed, 
                                      "RegisterClassEx failed");
    }
}

void WindowManager::UnregisterWindowClass()
{
    if (s_classRegistered)
    {
        UnregisterClassW(WINDOW_CLASS_NAME, m_hInstance);
        s_classRegistered = false;
        Logger::Debug("Window class unregistered");
    }
}

std::wstring WindowManager::GetWindowClassName() const
{
    return WINDOW_CLASS_NAME;
}