#pragma once

#include "../Common/Types.h"
#include "../Common/Config.h"
#include <windows.h>
#include <functional>
#include <memory>
#include <string>

// 窗口事件类型
enum class WindowEvent
{
    Moved,
    Resized,
    Minimized,
    Restored,
    Closed,
    RightClick,
    DoubleClick
};

// 全屏检测状态
struct FullscreenState
{
    bool isFullscreenDetected;
    HWND fullscreenWindow;
    RECT fullscreenRect;
    DWORD lastCheckTime;
    
    FullscreenState() : isFullscreenDetected(false), fullscreenWindow(nullptr), lastCheckTime(0)
    {
        memset(&fullscreenRect, 0, sizeof(RECT));
    }
};

class WindowManager
{
public:
    WindowManager();
    ~WindowManager();

    // 初始化和清理
    bool Initialize(HINSTANCE hInstance, const WindowConfig& config);
    void Shutdown();

    // 窗口创建和管理
    bool CreateOverlayWindow();
    void DestroyWindow();
    
    // 窗口属性
    void SetAlwaysOnTop(bool enable);
    void SetClickThrough(bool enable);
    void SetPosition(int x, int y);
    void SetSize(int width, int height);
    void SetTransparency(float alpha);
    
    // 窗口状态
    void ShowWindow();
    void HideWindow();
    bool IsWindowVisible() const { return m_isVisible; }
    HWND GetWindowHandle() const { return m_overlayWindow; }
    
    // 配置管理
    void UpdateConfig(const WindowConfig& config);
    WindowConfig GetCurrentConfig() const { return m_config; }
    
    // 全屏检测
    bool IsFullscreenApplicationRunning();
    void EnableFullscreenCompatibility(bool enable);
    
    // 事件处理
    void SetEventCallback(std::function<void(WindowEvent, LPARAM)> callback);
    
    // 拖拽功能
    void EnableDragging(bool enable);
    bool IsDragging() const { return m_isDragging; }

private:
    // 窗口过程
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    // 消息处理
    void OnPaint(HWND hwnd);
    void OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void OnMove(HWND hwnd, LPARAM lParam);
    void OnMouseDown(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void OnMouseUp(HWND hwnd, WPARAM wParam, LPARAM lParam);
    void OnMouseMove(HWND hwnd, LPARAM lParam);
    void OnRightClick(HWND hwnd, LPARAM lParam);
    void OnDoubleClick(HWND hwnd, LPARAM lParam);
    
    // 窗口样式管理
    void ApplyWindowStyles();
    void UpdateLayeredWindowAttributes();
    void SetWindowRegion();
    
    // 全屏检测
    void HandleFullscreenDetection();
    bool IsWindowFullscreen(HWND hwnd);
    void EnsureTopmost();
    void CheckFullscreenApplications();
    
    // 位置和大小管理
    void ClampWindowToScreen();
    RECT GetWorkArea();
    void SaveWindowPosition();
    void RestoreWindowPosition();
    
    // 工具方法
    void RegisterWindowClass();
    void UnregisterWindowClass();
    std::wstring GetWindowClassName() const;
    
    // 成员变量
    HINSTANCE m_hInstance;
    HWND m_overlayWindow;
    WindowConfig m_config;
    
    // 窗口状态
    bool m_isVisible;
    bool m_isInitialized;
    bool m_alwaysOnTop;
    bool m_clickThrough;
    bool m_isDragging;
    
    // 拖拽状态
    POINT m_dragStartPos;
    POINT m_windowStartPos;
    
    // 全屏检测
    std::unique_ptr<FullscreenState> m_fullscreenState;
    bool m_fullscreenCompatibilityEnabled;
    
    // 事件回调
    std::function<void(WindowEvent, LPARAM)> m_eventCallback;
    
    // 窗口类名
    static const wchar_t* WINDOW_CLASS_NAME;
    static bool s_classRegistered;
    
    // 定时器ID
    static const UINT_PTR FULLSCREEN_CHECK_TIMER = 1001;
    static const UINT_PTR TOPMOST_ENSURE_TIMER = 1002;
};