#include "Diagnostics/PerformanceMonitor.h"

#include "Config/ConfigManager.h"

#include <Psapi.h>
#include <windows.h>

#include <chrono>

using namespace Diagnostics;

PerformanceMonitor::PerformanceMonitor(std::shared_ptr<Config::ConfigManager> config)
    : m_config(std::move(config))
{
    Start();
}

PerformanceMonitor::~PerformanceMonitor()
{
    Stop();
}

void PerformanceMonitor::Start()
{
    if (m_running.exchange(true))
    {
        return;
    }

    m_thread = std::thread(&PerformanceMonitor::Worker, this);
}

void PerformanceMonitor::Stop()
{
    if (!m_running.exchange(false))
    {
        return;
    }

    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

PerformanceSnapshot PerformanceMonitor::GetLatest() const
{
    std::scoped_lock lock{m_mutex};
    return m_snapshot;
}

void PerformanceMonitor::Worker()
{
    using namespace std::chrono_literals;
    while (m_running)
    {
        {
            auto snapshot = Sample();
            std::scoped_lock lock{m_mutex};
            m_snapshot = snapshot;
        }
        std::this_thread::sleep_for(1s);
    }
}

PerformanceSnapshot PerformanceMonitor::Sample() const
{
    PerformanceSnapshot snapshot;

    FILETIME idleTime{}, kernelTime{}, userTime{};
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime))
    {
        static ULARGE_INTEGER lastIdle{}, lastKernel{}, lastUser{};
        ULARGE_INTEGER idle{ idleTime.dwLowDateTime, idleTime.dwHighDateTime };
        ULARGE_INTEGER kernel{ kernelTime.dwLowDateTime, kernelTime.dwHighDateTime };
        ULARGE_INTEGER user{ userTime.dwLowDateTime, userTime.dwHighDateTime };

        const ULONGLONG idleDiff = idle.QuadPart - lastIdle.QuadPart;
        const ULONGLONG kernelDiff = kernel.QuadPart - lastKernel.QuadPart;
        const ULONGLONG userDiff = user.QuadPart - lastUser.QuadPart;
        const ULONGLONG total = kernelDiff + userDiff;

        if (total > 0)
        {
            snapshot.cpuPercent = (1.0 - (static_cast<double>(idleDiff) / total)) * 100.0;
        }

        lastIdle = idle;
        lastKernel = kernel;
        lastUser = user;
    }

    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    {
        snapshot.memoryMb = counters.WorkingSetSize / (1024 * 1024);
    }

    return snapshot;
}
