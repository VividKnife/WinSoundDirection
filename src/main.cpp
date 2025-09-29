#include "Application.h"
#include "Common/Logger.h"
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 初始化日志系统
    Logger::Initialize();
    Logger::Info("Spatial Audio Visualizer starting...");

    try
    {
        // 创建应用程序实例
        Application app;
        
        // 初始化应用程序
        if (!app.Initialize(hInstance))
        {
            Logger::Error("Failed to initialize application");
            return -1;
        }

        // 运行主消息循环
        int result = app.Run();
        
        // 清理
        app.Shutdown();
        
        Logger::Info("Application shutdown complete");
        return result;
    }
    catch (const std::exception& e)
    {
        Logger::Error("Unhandled exception: " + std::string(e.what()));
        return -1;
    }
}