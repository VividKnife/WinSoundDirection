#include "App/SpatialVisualizerApp.h"

#include "Audio/SpatialAudioEngine.h"
#include "Audio/SpatialAudioRouter.h"
#include "Config/ConfigManager.h"
#include "Diagnostics/PerformanceMonitor.h"
#include "Hotkeys/HotkeyController.h"
#include "Rendering/DirectionVisualizer.h"
#include "UI/OverlayWindow.h"
#include "UI/SettingsController.h"
#include "UI/TrayIcon.h"

#include <shellapi.h>

using namespace App;

SpatialVisualizerApp::SpatialVisualizerApp(HINSTANCE instance,
                                           int cmdShow,
                                           std::shared_ptr<Config::ConfigManager> config,
                                           std::shared_ptr<Diagnostics::PerformanceMonitor> performanceMonitor)
    : m_instance(instance)
    , m_cmdShow(cmdShow)
    , m_config(std::move(config))
    , m_performanceMonitor(std::move(performanceMonitor))
{
}

SpatialVisualizerApp::~SpatialVisualizerApp()
{
    Shutdown();
}

int SpatialVisualizerApp::Run()
{
    InitializeWindow();
    InitializeAudio();
    InitializeUi();
    InitializeHotkeys();

    m_running = true;
    PumpMessages();
    return EXIT_SUCCESS;
}

void SpatialVisualizerApp::Shutdown()
{
    if (!m_running)
    {
        return;
    }

    m_running = false;

    if (m_hotkeys)
    {
        m_hotkeys->Shutdown();
        m_hotkeys.reset();
    }

    if (m_overlayWindow)
    {
        m_overlayWindow->Destroy();
        m_overlayWindow.reset();
    }

    if (m_trayIcon)
    {
        m_trayIcon->Destroy();
        m_trayIcon.reset();
    }

    if (m_audioRouter)
    {
        m_audioRouter->Stop();
        m_audioRouter.reset();
    }

    if (m_audioEngine)
    {
        m_audioEngine->Shutdown();
        m_audioEngine.reset();
    }

    if (m_settingsController)
    {
        m_settingsController.reset();
    }

    if (m_performanceMonitor)
    {
        m_performanceMonitor->Stop();
    }
}

void SpatialVisualizerApp::InitializeWindow()
{
    m_visualizer = std::make_unique<Rendering::DirectionVisualizer>(m_config);
    m_overlayWindow = std::make_unique<UI::OverlayWindow>(m_instance, m_visualizer.get(), m_config);
    m_overlayWindow->Create(m_cmdShow);
}

void SpatialVisualizerApp::InitializeAudio()
{
    m_audioEngine = std::make_unique<Audio::SpatialAudioEngine>(m_config);
    m_audioEngine->Initialize();
    m_audioRouter = std::make_unique<Audio::SpatialAudioRouter>(m_config, m_audioEngine.get(), m_visualizer.get());
    m_audioRouter->Start();
}

void SpatialVisualizerApp::InitializeUi()
{
    m_settingsController = std::make_unique<UI::SettingsController>(m_instance, m_overlayWindow.get(), m_audioRouter.get(), nullptr, m_config);
    m_trayIcon = std::make_unique<UI::TrayIcon>(m_instance, m_overlayWindow.get(), m_settingsController.get(), m_config, m_performanceMonitor);
    m_trayIcon->Create();
    m_overlayWindow->SetSettingsController(m_settingsController.get());
}

void SpatialVisualizerApp::InitializeHotkeys()
{
    m_hotkeys = std::make_unique<Hotkeys::HotkeyController>(m_instance, m_overlayWindow.get(), m_config);
    m_hotkeys->Register();
    if (m_settingsController)
    {
        m_settingsController->SetHotkeyController(m_hotkeys.get());
    }
}

void SpatialVisualizerApp::PumpMessages()
{
    MSG msg{};
    while (m_running && GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        if (!m_settingsController->ProcessDialogMessage(&msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}
