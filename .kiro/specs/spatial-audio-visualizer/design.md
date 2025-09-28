# 设计文档

## 概述

空间音效可视化工具是一个基于 Windows 平台的实时音频方向显示应用程序。该工具通过 Windows Spatial Audio Platform APIs 获取 3D 音频数据，并使用 DirectX/Direct2D 渲染一个始终置顶的半透明覆盖界面，实时显示声音来源的空间方向信息。

## 架构

### 系统架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    应用程序层                                │
├─────────────────────────────────────────────────────────────┤
│  UI层           │  音频处理层        │  配置管理层           │
│  - 渲染引擎     │  - 空间音频捕获    │  - 设置存储           │
│  - 事件处理     │  - 方向计算        │  - 用户偏好           │
│  - 窗口管理     │  - 信号处理        │  - 主题管理           │
├─────────────────────────────────────────────────────────────┤
│                   Windows API层                             │
│  - Windows Spatial Audio APIs                              │
│  - DirectX/Direct2D                                        │
│  - Win32 Window Management                                  │
│  - WASAPI (Windows Audio Session API)                      │
└─────────────────────────────────────────────────────────────┘
```

### 核心组件

1. **音频捕获引擎** - 使用 WASAPI 和 Spatial Audio APIs
2. **方向计算模块** - 处理 3D 音频数据并计算方向向量
3. **渲染引擎** - 使用 Direct2D 进行硬件加速渲染
4. **窗口管理器** - 处理置顶窗口和全屏兼容性
5. **配置系统** - 管理用户设置和偏好

## 组件和接口

### 1. 音频捕获引擎 (AudioCaptureEngine)

```cpp
class AudioCaptureEngine {
public:
    bool Initialize();
    void StartCapture();
    void StopCapture();
    SpatialAudioData GetCurrentAudioData();
    void SetSensitivity(float sensitivity);

private:
    ISpatialAudioClient* m_spatialAudioClient;
    IAudioClient* m_audioClient;
    std::thread m_captureThread;
};

struct SpatialAudioData {
    DirectionVector primaryDirection;
    float intensity;
    float confidence;
    std::vector<DirectionVector> secondaryDirections;
};

struct DirectionVector {
    float x, y, z;  // 3D空间坐标
    float azimuth;  // 水平角度 (0-360°)
    float elevation; // 垂直角度 (-90° to +90°)
    float distance; // 相对距离
};
```

### 2. 方向计算模块 (DirectionProcessor)

```cpp
class DirectionProcessor {
public:
    ProcessedDirection ProcessAudioData(const SpatialAudioData& data);
    void SetProcessingParameters(const ProcessingConfig& config);

private:
    DirectionVector CalculatePrimaryDirection(const SpatialAudioData& data);
    float CalculateIntensity(const SpatialAudioData& data);
    bool ValidateDirection(const DirectionVector& direction);
};

struct ProcessedDirection {
    CardinalDirection primary;
    float intensity;
    std::vector<CardinalDirection> secondary;
};

enum class CardinalDirection {
    Front, Back, Left, Right, Up, Down,
    FrontLeft, FrontRight, BackLeft, BackRight
};
```

### 3. 渲染引擎 (RenderEngine)

```cpp
class RenderEngine {
public:
    bool Initialize(HWND hwnd);
    void Render(const ProcessedDirection& direction);
    void SetTheme(const VisualTheme& theme);
    void SetTransparency(float alpha);

private:
    ID2D1Factory* m_d2dFactory;
    ID2D1HwndRenderTarget* m_renderTarget;
    std::map<CardinalDirection, ID2D1Brush*> m_brushes;

    void RenderDirectionIndicator(CardinalDirection dir, float intensity);
    void RenderCompass();
    void RenderIntensityMeter();
};

struct VisualTheme {
    D2D1_COLOR_F primaryColor;
    D2D1_COLOR_F secondaryColor;
    D2D1_COLOR_F backgroundColor;
    float indicatorSize;
    IndicatorStyle style;
};
```

### 4. 窗口管理器 (WindowManager)

```cpp
class WindowManager {
public:
    bool CreateOverlayWindow();
    void SetAlwaysOnTop(bool enable);
    void SetClickThrough(bool enable);
    void SetPosition(int x, int y);
    void SetSize(int width, int height);
    void ShowWindow();
    void HideWindow();
    bool IsWindowVisible();

private:
    HWND m_overlayWindow;
    bool m_isFullscreenCompatible;
    bool m_isVisible;

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void HandleFullscreenDetection();
    void EnsureTopmost();
};
```

### 5. 快捷键管理器 (HotkeyManager)

```cpp
class HotkeyManager {
public:
    bool RegisterGlobalHotkey(int hotkeyId, UINT modifiers, UINT vk);
    void UnregisterHotkey(int hotkeyId);
    void SetToggleHotkey(UINT vk, UINT modifiers = 0);

private:
    std::map<int, HotkeyInfo> m_registeredHotkeys;
    HWND m_messageWindow;

    static LRESULT CALLBACK HotkeyWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void HandleHotkeyPressed(int hotkeyId);
};

struct HotkeyInfo {
    UINT virtualKey;
    UINT modifiers;
    std::function<void()> callback;
};
```

### 6. 系统托盘管理器 (SystemTrayManager)

```cpp
class SystemTrayManager {
public:
    bool CreateTrayIcon();
    void UpdateTrayIcon(const std::wstring& tooltip);
    void ShowTrayIcon();
    void HideTrayIcon();
    void SetTrayMenu(HMENU menu);

private:
    NOTIFYICONDATA m_nid;
    HWND m_messageWindow;
    HMENU m_contextMenu;

    static LRESULT CALLBACK TrayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void HandleTrayIconClick(UINT msg);
    void ShowContextMenu();
};
```

## 数据模型

### 配置数据模型

```cpp
struct ApplicationConfig {
    AudioConfig audio;
    VisualConfig visual;
    WindowConfig window;
    HotkeyConfig hotkey;
    PerformanceConfig performance;
};

struct AudioConfig {
    float sensitivity = 0.5f;
    float noiseThreshold = 0.1f;
    bool enableDirectionFiltering = true;
    std::set<CardinalDirection> enabledDirections;
    int updateFrequency = 60; // Hz
};

struct VisualConfig {
    VisualTheme theme;
    float transparency = 0.8f;
    int indicatorSize = 50;
    bool showCompass = true;
    bool showIntensityMeter = true;
    AnimationStyle animation = AnimationStyle::Smooth;
};

struct WindowConfig {
    Point position = {100, 100};
    Size size = {200, 200};
    bool alwaysOnTop = true;
    bool clickThrough = false;
    bool hideInFullscreen = false;
    bool startMinimized = false;
};

struct HotkeyConfig {
    UINT toggleKey = VK_HOME;
    UINT toggleModifiers = 0;
    bool enableGlobalHotkeys = true;
    bool showTrayIcon = true;
};
```

## 错误处理

### 错误类型和处理策略

1. **音频设备错误**

   - 检测：定期检查音频设备状态
   - 处理：显示错误提示，尝试重新初始化
   - 恢复：自动重连机制

2. **空间音效不支持**

   - 检测：启动时验证 Spatial Audio 支持
   - 处理：显示警告信息，提供替代方案
   - 降级：使用立体声模拟空间效果

3. **渲染错误**

   - 检测：Direct2D 操作返回值检查
   - 处理：回退到 GDI+渲染
   - 恢复：重新创建渲染资源

4. **窗口管理错误**
   - 检测：窗口创建和置顶操作失败
   - 处理：尝试不同的窗口样式
   - 恢复：重新创建窗口

### 错误处理接口

```cpp
class ErrorHandler {
public:
    void HandleAudioError(AudioErrorType error);
    void HandleRenderError(RenderErrorType error);
    void HandleWindowError(WindowErrorType error);
    void ShowUserNotification(const std::string& message, NotificationType type);

private:
    void LogError(const std::string& error);
    bool AttemptRecovery(ErrorType type);
};
```

## 测试策略

### 单元测试

1. **音频处理测试**

   - 模拟音频数据输入
   - 验证方向计算准确性
   - 测试边界条件和异常情况

2. **渲染测试**

   - 验证不同主题的渲染效果
   - 测试透明度和动画
   - 性能基准测试

3. **配置管理测试**
   - 设置保存和加载
   - 默认值验证
   - 配置迁移测试

### 集成测试

1. **系统集成测试**

   - 与 Windows Spatial Audio 的集成
   - 全屏游戏兼容性测试
   - 多显示器支持测试

2. **性能测试**
   - CPU 和内存使用监控
   - 长时间运行稳定性测试
   - 不同硬件配置下的性能测试

### 用户验收测试

1. **功能测试**

   - 方向检测准确性验证
   - 用户界面易用性测试
   - 配置选项完整性测试

2. **兼容性测试**
   - 不同 Windows 版本测试
   - 各种音频设备测试
   - 主流 FPS 游戏兼容性测试

## 性能优化

### 音频处理优化

1. **缓冲区管理**

   - 使用环形缓冲区减少内存分配
   - 异步音频处理避免阻塞 UI 线程

2. **计算优化**
   - 使用 SIMD 指令加速向量计算
   - 缓存频繁计算的结果

### 渲染优化

1. **硬件加速**

   - 使用 Direct2D 硬件加速
   - 减少不必要的重绘操作

2. **资源管理**
   - 预加载渲染资源
   - 智能资源释放机制

### 系统资源优化

1. **线程管理**

   - 音频捕获独立线程
   - 渲染线程与主线程分离

2. **内存管理**
   - 对象池减少内存分配
   - 智能指针管理资源生命周期
