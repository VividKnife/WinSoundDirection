#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <chrono>

#include "Audio/SpatialAudioEngine.h"
#include "Config/ConfigManager.h"

namespace Rendering
{
struct VisualState
{
    Audio::AudioDirection direction;
    bool visible{true};
    std::wstring modeLabel;
};

struct RadarHit
{
    Audio::AudioDirection direction;
    float radiusFactor{1.0f};
    std::chrono::steady_clock::time_point time;
};

class DirectionVisualizer
{
public:
    explicit DirectionVisualizer(std::shared_ptr<Config::ConfigManager> config);
    ~DirectionVisualizer();

    void Initialize(HWND hwnd);
    void Resize(UINT width, UINT height);
    void Render();
    void UpdateDirection(const Audio::AudioDirection& direction);
    void SetVisible(bool visible);
    void SetSensitivity(const Config::SensitivityConfig& sensitivity);
    void SetModeLabel(const std::wstring& label);

    [[nodiscard]] bool IsVisible() const noexcept { return m_state.visible; }
    [[nodiscard]] VisualState CurrentState() const;

private:
    void CreateDeviceResources(HWND hwnd);
    void UpdateGeometry();
    D2D1::ColorF ColorFromConfig() const;

    std::shared_ptr<Config::ConfigManager> m_config;

    Microsoft::WRL::ComPtr<ID2D1Factory> m_factory;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_primaryBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_backgroundBrush;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_accentBrush;
    Microsoft::WRL::ComPtr<IDWriteFactory> m_dwriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> m_textFormat;

    VisualState m_state;
    Config::SensitivityConfig m_sensitivity;
    std::vector<RadarHit> m_hits;
    float m_referenceMagnitude{0.0f};

    UINT m_width{320};
    UINT m_height{320};

    mutable std::mutex m_mutex;
};
}
