#pragma once

#include <memory>
#include <windows.h>

namespace Audio { class SpatialAudioEngine; class SpatialAudioRouter; }
namespace Config { class ConfigManager; }
namespace Diagnostics { class PerformanceMonitor; }
namespace Hotkeys { class HotkeyController; }
namespace Rendering { class DirectionVisualizer; }
namespace UI { class OverlayWindow; class SettingsController; class TrayIcon; }

namespace App
{
class SpatialVisualizerApp
{
public:
    SpatialVisualizerApp(HINSTANCE instance,
                         int cmdShow,
                         std::shared_ptr<Config::ConfigManager> config,
                         std::shared_ptr<Diagnostics::PerformanceMonitor> performanceMonitor);
    ~SpatialVisualizerApp();

    int Run();
    void Shutdown();

private:
    void InitializeWindow();
    void InitializeAudio();
    void InitializeUi();
    void InitializeHotkeys();
    void PumpMessages();

    HINSTANCE m_instance;
    int m_cmdShow;

    std::shared_ptr<Config::ConfigManager> m_config;
    std::shared_ptr<Diagnostics::PerformanceMonitor> m_performanceMonitor;

    std::unique_ptr<Audio::SpatialAudioEngine> m_audioEngine;
    std::unique_ptr<Audio::SpatialAudioRouter> m_audioRouter;
    std::unique_ptr<Rendering::DirectionVisualizer> m_visualizer;
    std::unique_ptr<UI::OverlayWindow> m_overlayWindow;
    std::unique_ptr<UI::SettingsController> m_settingsController;
    std::unique_ptr<UI::TrayIcon> m_trayIcon;
    std::unique_ptr<Hotkeys::HotkeyController> m_hotkeys;

    bool m_running{false};
};
}
