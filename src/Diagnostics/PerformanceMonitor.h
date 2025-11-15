#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

namespace Config { class ConfigManager; }

namespace Diagnostics
{
struct PerformanceSnapshot
{
    double cpuPercent{0.0};
    size_t memoryMb{0};
};

class PerformanceMonitor
{
public:
    explicit PerformanceMonitor(std::shared_ptr<Config::ConfigManager> config);
    ~PerformanceMonitor();

    void Start();
    void Stop();

    PerformanceSnapshot GetLatest() const;

private:
    void Worker();
    PerformanceSnapshot Sample() const;

    std::shared_ptr<Config::ConfigManager> m_config;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    mutable std::mutex m_mutex;
    PerformanceSnapshot m_snapshot;
};
}
