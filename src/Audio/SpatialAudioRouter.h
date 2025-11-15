#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "Audio/SpatialAudioEngine.h"

namespace Config { class ConfigManager; }
namespace Rendering { class DirectionVisualizer; }

namespace Audio
{
class SpatialAudioRouter
{
public:
    SpatialAudioRouter(std::shared_ptr<Config::ConfigManager> config,
                       SpatialAudioEngine* engine,
                       Rendering::DirectionVisualizer* visualizer);
    ~SpatialAudioRouter();

    void Start();
    void Stop();

    void ApplySensitivity();

private:
    void Worker();

    std::shared_ptr<Config::ConfigManager> m_config;
    SpatialAudioEngine* m_engine;
    Rendering::DirectionVisualizer* m_visualizer;

    std::atomic<bool> m_running{false};
    std::thread m_thread;
};
}
