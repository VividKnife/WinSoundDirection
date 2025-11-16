#pragma once

#include <memory>
#include <windows.h>

namespace App
{
class SpatialVisualizerApp;

class ApplicationHost
{
public:
    ApplicationHost(HINSTANCE instance, int cmdShow);
    ~ApplicationHost();
    int Run();

private:
    void Initialize();
    void Shutdown();

    HINSTANCE m_instance;
    int m_cmdShow;
    std::unique_ptr<SpatialVisualizerApp> m_app;
};
}
