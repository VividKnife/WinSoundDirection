#include "Application.h"
#include "Common/Logger.h"
#include "Common/ErrorHandler.h"
#include "Common/WindowsCompat.h"

// 组件头文件
#include "Audio/AudioCaptureEngine.h"
#include "Audio/DirectionProcessor.h"
#include "Rendering/RenderEngine.h"
#include "Window/WindowManager.h"
#include "Input/HotkeyManager.h"
#include "System/SystemTrayManager.h"
#include "Config/ConfigManager.h"

Application::Application()
    : m_hInstance(nullptr)
    , m_running(false)
    , m_visible(true)
{
    Logger::Info("Application created");
}

Application::~Application()
{
    Shutdown();
    Logger::Info("Application destroyed");
}

bool Application::Initialize(HINSTANCE hInstance)
{
    Logger::Info("Initializing application...");
    
    m_hInstance = hInstance;
    
    // 初始化错误处理系统
    ErrorHandler::Initialize();
    
    // 设置错误回调
    ErrorHandler::SetErrorCallback([this](ErrorType type, const std::string& message) {
        // 处理错误回调
        Logger::Warning("Error callback triggered: " + message);
    });
    
    // 初始化组件
    if (!InitializeComponents())
    {
        Logger::Error("Failed to initialize components");
        return false;
    }
    
    m_running = true;
    Logger::Info("Application initialized successfully");
    return true;
}

int Application::Run()
{
    Logger::Info("Starting application main loop");
    
    while (m_running)
    {
        ProcessMessages();
        UpdateApplication();
        
        // 简单的帧率控制
        Sleep(16); // ~60 FPS
    }
    
    Logger::Info("Application main loop ended");
    return 0;
}

void Application::Shutdown()
{
    if (!m_running)
        return;
    
    Logger::Info("Shutting down application...");
    
    m_running = false;
    
    ShutdownComponents();
    ErrorHandler::Shutdown();
    
    Logger::Info("Application shutdown complete");
}

void Application::ToggleVisibility()
{
    if (m_visible)
    {
        HideApplication();
    }
    else
    {
        ShowApplication();
    }
}

void Application::ShowApplication()
{
    if (!m_visible)
    {
        m_visible = true;
        Logger::Info("Application shown");
        
        if (m_windowManager)
        {
            m_windowManager->ShowWindow();
        }
        
        if (m_audioEngine && !m_audioEngine->IsCapturing())
        {
            m_audioEngine->StartCapture();
        }
        
        // 更新托盘菜单
        if (m_trayManager)
        {
            m_trayManager->UpdateMenuItems(true);
        }
    }
}

void Application::HideApplication()
{
    if (m_visible)
    {
        m_visible = false;
        Logger::Info("Application hidden");
        
        if (m_windowManager)
        {
            m_windowManager->HideWindow();
        }
        
        // 音频捕获继续运行，只是隐藏界面
        
        // 更新托盘菜单
        if (m_trayManager)
        {
            m_trayManager->UpdateMenuItems(false);
        }
    }
}

void Application::ExitApplication()
{
    Logger::Info("Exit application requested");
    m_running = false;
}

void Application::UpdateConfig(const ApplicationConfig& config)
{
    m_config = config;
    OnConfigChanged();
}

bool Application::InitializeComponents()
{
    Logger::Info("Initializing application components...");
    
    try
    {
        // 配置管理器
        m_configManager = std::make_unique<ConfigManager>();
        if (!m_configManager->Initialize())
        {
            Logger::Error("Failed to initialize ConfigManager");
            return false;
        }
        
        // 加载配置
        m_config = m_configManager->LoadConfig();
        
        // 窗口管理器
        m_windowManager = std::make_unique<WindowManager>();
        if (!m_windowManager->Initialize(m_hInstance, m_config.window))
        {
            Logger::Error("Failed to initialize WindowManager");
            return false;
        }
        
        // 设置窗口事件回调
        m_windowManager->SetEventCallback([this](WindowEvent event, LPARAM lParam) {
            switch (event)
            {
                case WindowEvent::RightClick:
                    // 显示设置菜单
                    break;
                case WindowEvent::DoubleClick:
                    ToggleVisibility();
                    break;
            }
        });
        
        // 渲染引擎
        m_renderEngine = std::make_unique<RenderEngine>();
        if (!m_renderEngine->Initialize(m_windowManager->GetWindowHandle()))
        {
            Logger::Error("Failed to initialize RenderEngine");
            return false;
        }
        
        // 音频捕获引擎
        m_audioEngine = std::make_unique<AudioCaptureEngine>();
        if (!m_audioEngine->Initialize())
        {
            Logger::Error("Failed to initialize AudioCaptureEngine");
            return false;
        }
        
        // 设置音频数据回调
        m_audioEngine->SetAudioDataCallback([this](const SpatialAudioData& data) {
            OnAudioDataReceived(data);
        });
        
        // 方向处理器
        m_directionProcessor = std::make_unique<DirectionProcessor>();
        m_directionProcessor->UpdateConfig(m_config.audio);
        
        // 快捷键管理器
        m_hotkeyManager = std::make_unique<HotkeyManager>();
        if (!m_hotkeyManager->Initialize())
        {
            Logger::Error("Failed to initialize HotkeyManager");
            return false;
        }
        
        // 设置快捷键回调
        m_hotkeyManager->SetToggleCallback([this]() {
            ToggleVisibility();
        });
        
        m_hotkeyManager->SetExitCallback([this]() {
            ExitApplication();
        });
        
        // 注册切换可见性快捷键
        m_hotkeyManager->UpdateConfig(m_config.hotkey);
        
        // 系统托盘管理器
        m_trayManager = std::make_unique<SystemTrayManager>();
        if (!m_trayManager->Initialize())
        {
            Logger::Error("Failed to initialize SystemTrayManager");
            return false;
        }
        
        // 设置托盘回调
        m_trayManager->SetShowCallback([this]() {
            ShowApplication();
        });
        
        m_trayManager->SetHideCallback([this]() {
            HideApplication();
        });
        
        m_trayManager->SetExitCallback([this]() {
            ExitApplication();
        });
        
        // 设置托盘事件回调
        m_trayManager->SetTrayEventCallback([this](TrayEvent event) {
            if (event == TrayEvent::DoubleClick)
            {
                ToggleVisibility();
            }
        });
        
        Logger::Info("All components initialized successfully");
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::Error("Exception during component initialization: " + std::string(e.what()));
        return false;
    }
}

void Application::ShutdownComponents()
{
    Logger::Info("Shutting down application components...");
    
    // 停止音频捕获
    if (m_audioEngine)
    {
        m_audioEngine->StopCapture();
    }
    
    // 按相反顺序关闭组件
    m_trayManager.reset();
    m_hotkeyManager.reset();
    m_directionProcessor.reset();
    m_audioEngine.reset();
    m_renderEngine.reset();
    m_windowManager.reset();
    m_configManager.reset();
    
    Logger::Info("All components shut down");
}

void Application::ProcessMessages()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            m_running = false;
            break;
        }
        
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Application::UpdateApplication()
{
    // 主要更新逻辑
    if (m_audioEngine && m_directionProcessor && m_renderEngine && m_visible)
    {
        // 获取当前音频数据
        SpatialAudioData audioData = m_audioEngine->GetCurrentAudioData();
        
        // 处理方向计算
        ProcessedDirection direction = m_directionProcessor->ProcessAudioData(audioData);
        
        // 更新渲染
        m_renderEngine->Render(direction);
    }
    else if (m_renderEngine && m_visible)
    {
        // 如果没有音频数据，清除显示
        m_renderEngine->Clear();
    }
}

void Application::OnAudioDataReceived(const SpatialAudioData& data)
{
    // 音频数据接收回调
    // 这里可以添加额外的音频数据处理逻辑
    // 比如记录统计信息、触发特殊事件等
    
    // 主要的处理逻辑在UpdateApplication中进行
}

void Application::OnConfigChanged()
{
    Logger::Info("Configuration changed, updating components...");
    
    // 更新各组件的配置
    if (m_windowManager)
        m_windowManager->UpdateConfig(m_config.window);
    
    if (m_renderEngine)
        m_renderEngine->UpdateConfig(m_config.visual);
    
    if (m_audioEngine)
        m_audioEngine->UpdateConfig(m_config.audio);
    
    if (m_directionProcessor)
        m_directionProcessor->UpdateConfig(m_config.audio);
    
    if (m_hotkeyManager)
        m_hotkeyManager->UpdateConfig(m_config.hotkey);
    
    // 保存配置
    if (m_configManager)
        m_configManager->SaveConfig(m_config);
}