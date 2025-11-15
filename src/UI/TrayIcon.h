#pragma once

#include <shellapi.h>
#include <windows.h>

#include <memory>
#include <string>

namespace Config { class ConfigManager; }
namespace Diagnostics { class PerformanceMonitor; }
namespace UI { class OverlayWindow; class SettingsController; }

namespace UI
{
class TrayIcon
{
public:
    TrayIcon(HINSTANCE instance,
             OverlayWindow* overlay,
             SettingsController* settings,
             std::shared_ptr<Config::ConfigManager> config,
             std::shared_ptr<Diagnostics::PerformanceMonitor> performance);
    ~TrayIcon();

    void Create();
    void Destroy();

private:
    void UpdateTooltip();
    void HandleCommand(UINT id);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HINSTANCE m_instance;
    OverlayWindow* m_overlay;
    SettingsController* m_settings;
    std::shared_ptr<Config::ConfigManager> m_config;
    std::shared_ptr<Diagnostics::PerformanceMonitor> m_performance;

    HWND m_messageWindow{nullptr};
    NOTIFYICONDATAW m_nid{};
};
}
