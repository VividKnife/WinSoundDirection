#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

#include <windows.h>

namespace Config
{
enum class AudioModeOverride
{
    Auto = 0,
    Headphone = 1,
    Multichannel = 2,
};

struct HotkeyConfig
{
    UINT modifier{MOD_CONTROL | MOD_ALT};
    UINT key{VK_HOME};
};

struct ThemeConfig
{
    COLORREF primaryColor{RGB(0, 153, 255)};
    COLORREF accentColor{RGB(255, 255, 255)};
    float opacity{0.75f};
};

struct SensitivityConfig
{
    float thresholdDb{-40.0f};
    float smoothing{0.25f};
    // Detection range / distance mapping scale (0.5~2.0 recommended)
    float distanceScale{1.0f};
    // Pattern detection thresholds (rough heuristics)
    float strongMagnitude{0.6f};
    float strongJump{0.25f};
    float rhythmMinInterval{0.25f};
    float rhythmMaxInterval{0.7f};
    float rhythmDirectionDeg{40.0f};
};

struct DirectionFilter
{
    bool front{true};
    bool back{true};
    bool left{true};
    bool right{true};
    bool up{true};
    bool down{true};
};

struct PerformanceLimits
{
    double maxCpuPercent{5.0};
    size_t maxMemoryMb{50};
};

class ConfigManager
{
public:
    ConfigManager();

    void Load();
    void Save() const;

    const ThemeConfig& Theme() const noexcept { return m_theme; }
    ThemeConfig& Theme() noexcept { return m_theme; }

    const SensitivityConfig& Sensitivity() const noexcept { return m_sensitivity; }
    SensitivityConfig& Sensitivity() noexcept { return m_sensitivity; }

    const DirectionFilter& Filter() const noexcept { return m_filter; }
    DirectionFilter& Filter() noexcept { return m_filter; }

    const HotkeyConfig& Hotkeys() const noexcept { return m_hotkeys; }
    HotkeyConfig& Hotkeys() noexcept { return m_hotkeys; }

    const PerformanceLimits& Limits() const noexcept { return m_limits; }
    PerformanceLimits& Limits() noexcept { return m_limits; }

    AudioModeOverride AudioMode() const noexcept { return m_audioMode; }
    void SetAudioMode(AudioModeOverride mode) noexcept { m_audioMode = mode; }

    bool IsDirectionEnabled(const std::wstring& direction) const;
    void SetDirectionEnabled(const std::wstring& direction, bool enabled);

private:
    std::filesystem::path GetConfigPath() const;

    ThemeConfig m_theme;
    SensitivityConfig m_sensitivity;
    DirectionFilter m_filter;
    HotkeyConfig m_hotkeys;
    PerformanceLimits m_limits;
    AudioModeOverride m_audioMode{AudioModeOverride::Auto};
};
}
