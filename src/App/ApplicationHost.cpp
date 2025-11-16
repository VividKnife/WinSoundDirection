#include "App/ApplicationHost.h"

#include "App/SpatialVisualizerApp.h"
#include "Config/ConfigManager.h"
#include "Diagnostics/PerformanceMonitor.h"
#include "Util/ComInitializer.h"

#include <stdexcept>

using namespace App;

ApplicationHost::ApplicationHost(HINSTANCE instance, int cmdShow)
    : m_instance(instance)
    , m_cmdShow(cmdShow)
{
}

ApplicationHost::~ApplicationHost() = default;

int ApplicationHost::Run()
{
    try
    {
        Initialize();
        return m_app->Run();
    }
    catch (const std::exception& ex)
    {
        MessageBoxA(nullptr, ex.what(), "Spatial Audio Visualizer", MB_ICONERROR | MB_OK);
        return EXIT_FAILURE;
    }
}

void ApplicationHost::Initialize()
{
    Util::ComInitializer comInit;

    auto config = std::make_shared<Config::ConfigManager>();
    config->Load();

    auto performanceMonitor = std::make_shared<Diagnostics::PerformanceMonitor>(config);

    m_app = std::make_unique<SpatialVisualizerApp>(m_instance, m_cmdShow, std::move(config), std::move(performanceMonitor));
}

void ApplicationHost::Shutdown()
{
    if (m_app)
    {
        m_app->Shutdown();
        m_app.reset();
    }
}
