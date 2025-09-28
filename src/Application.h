#pragma once

#include "Common/Types.h"
#include "Common/Config.h"
#include <windows.h>
#include <memory>

// 前向声明
class AudioCaptureEngine;
class DirectionProcessor;
class RenderEngine;
class WindowManager;
class HotkeyManager;
class SystemTrayManager;
class ConfigManager;

class Application
{
public:
    Application();
    ~Application();

    bool Initialize(HINSTANCE hInstance);
    int Run();
    void Shutdown();

    // 应用程序控制
    void ToggleVisibility();
    void ShowApplication();
    void HideApplication();
    void ExitApplication();

    // 配置管理
    const ApplicationConfig& GetConfig() const { return m_config; }
    void UpdateConfig(const ApplicationConfig& config);

private:
    // 初始化各个组件
    bool InitializeComponents();
    void ShutdownComponents();

    // 消息循环
    void ProcessMessages();
    void UpdateApplication();

    // 事件处理
    void OnAudioDataReceived(const SpatialAudioData& data);
    void OnConfigChanged();

    // 成员变量
    HINSTANCE m_hInstance;
    bool m_running;
    bool m_visible;
    
    ApplicationConfig m_config;
    
    // 核心组件
    std::unique_ptr<AudioCaptureEngine> m_audioEngine;
    std::unique_ptr<DirectionProcessor> m_directionProcessor;
    std::unique_ptr<RenderEngine> m_renderEngine;
    std::unique_ptr<WindowManager> m_windowManager;
    std::unique_ptr<HotkeyManager> m_hotkeyManager;
    std::unique_ptr<SystemTrayManager> m_trayManager;
    std::unique_ptr<ConfigManager> m_configManager;
};