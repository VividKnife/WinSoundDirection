#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

#include <windows.h>

namespace Config
{
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

    bool IsDirectionEnabled(const std::wstring& direction) const;
    void SetDirectionEnabled(const std::wstring& direction, bool enabled);

private:
    std::filesystem::path GetConfigPath() const;

    ThemeConfig m_theme;
    SensitivityConfig m_sensitivity;
    DirectionFilter m_filter;
    HotkeyConfig m_hotkeys;
    PerformanceLimits m_limits;
};
}
