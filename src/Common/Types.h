#pragma once

#include <vector>
#include <set>
#include <functional>
#include <d2d1.h>

// 基础数据结构
struct Point
{
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

struct Size
{
    int width, height;
    Size(int w = 0, int h = 0) : width(w), height(h) {}
};

// 3D方向向量
struct DirectionVector
{
    float x, y, z;          // 3D空间坐标
    float azimuth;          // 水平角度 (0-360°)
    float elevation;        // 垂直角度 (-90° to +90°)
    float distance;         // 相对距离

    DirectionVector() : x(0), y(0), z(0), azimuth(0), elevation(0), distance(0) {}
    DirectionVector(float x, float y, float z) : x(x), y(y), z(z), azimuth(0), elevation(0), distance(0) {}
};

// 空间音频数据
struct SpatialAudioData
{
    DirectionVector primaryDirection;
    float intensity;
    float confidence;
    std::vector<DirectionVector> secondaryDirections;

    SpatialAudioData() : intensity(0), confidence(0) {}
};

// 基本方向枚举
enum class CardinalDirection
{
    None = 0,
    Front,
    Back,
    Left,
    Right,
    Up,
    Down,
    FrontLeft,
    FrontRight,
    BackLeft,
    BackRight
};

// 处理后的方向数据
struct ProcessedDirection
{
    CardinalDirection primary;
    float intensity;
    std::vector<CardinalDirection> secondary;

    ProcessedDirection() : primary(CardinalDirection::None), intensity(0) {}
};

// 视觉主题样式
enum class IndicatorStyle
{
    Circle,
    Arrow,
    Dot,
    Ring
};

enum class AnimationStyle
{
    None,
    Smooth,
    Pulse,
    Fade
};

// 视觉主题
struct VisualTheme
{
    D2D1_COLOR_F primaryColor;
    D2D1_COLOR_F secondaryColor;
    D2D1_COLOR_F backgroundColor;
    float indicatorSize;
    IndicatorStyle style;

    VisualTheme()
    {
        primaryColor = D2D1::ColorF(D2D1::ColorF::Red, 0.8f);
        secondaryColor = D2D1::ColorF(D2D1::ColorF::Orange, 0.6f);
        backgroundColor = D2D1::ColorF(D2D1::ColorF::Black, 0.3f);
        indicatorSize = 50.0f;
        style = IndicatorStyle::Circle;
    }
};

// 快捷键信息
struct HotkeyInfo
{
    UINT virtualKey;
    UINT modifiers;
    std::function<void()> callback;

    HotkeyInfo() : virtualKey(0), modifiers(0) {}
    HotkeyInfo(UINT vk, UINT mod, std::function<void()> cb) 
        : virtualKey(vk), modifiers(mod), callback(cb) {}
};