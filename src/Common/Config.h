#pragma once

#include "Types.h"
#include <string>

// 音频配置
struct AudioConfig
{
    float sensitivity = 0.5f;
    float noiseThreshold = 0.1f;
    bool enableDirectionFiltering = true;
    std::set<CardinalDirection> enabledDirections;
    int updateFrequency = 60; // Hz

    AudioConfig()
    {
        // 默认启用所有方向
        enabledDirections.insert(CardinalDirection::Front);
        enabledDirections.insert(CardinalDirection::Back);
        enabledDirections.insert(CardinalDirection::Left);
        enabledDirections.insert(CardinalDirection::Right);
        enabledDirections.insert(CardinalDirection::Up);
        enabledDirections.insert(CardinalDirection::Down);
    }
};

// 视觉配置
struct VisualConfig
{
    VisualTheme theme;
    float transparency = 0.8f;
    int indicatorSize = 50;
    bool showCompass = true;
    bool showIntensityMeter = true;
    AnimationStyle animation = AnimationStyle::Smooth;
};

// 窗口配置
struct WindowConfig
{
    Point position = Point(100, 100);
    Size size = Size(200, 200);
    bool alwaysOnTop = true;
    bool clickThrough = false;
    bool hideInFullscreen = false;
    bool startMinimized = false;
};

// 快捷键配置
struct HotkeyConfig
{
    UINT toggleKey = VK_HOME;
    UINT toggleModifiers = 0;
    bool enableGlobalHotkeys = true;
    bool showTrayIcon = true;
};

// 性能配置
struct PerformanceConfig
{
    int maxCpuUsage = 5;        // 最大CPU使用率百分比
    int maxMemoryUsage = 50;    // 最大内存使用MB
    bool enablePerformanceMonitoring = true;
    bool adaptiveQuality = true; // 自适应质量调节
};

// 应用程序总配置
struct ApplicationConfig
{
    AudioConfig audio;
    VisualConfig visual;
    WindowConfig window;
    HotkeyConfig hotkey;
    PerformanceConfig performance;
    
    std::string configVersion = "1.0";
};