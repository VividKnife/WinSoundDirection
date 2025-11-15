#include "Audio/SpatialAudioRouter.h"

#include "Config/ConfigManager.h"
#include "Rendering/DirectionVisualizer.h"
#include "Util/DispatcherTimer.h"

#include <Psapi.h>
#include <windows.h>

#include <chrono>

#include <chrono>

using namespace Audio;

SpatialAudioRouter::SpatialAudioRouter(std::shared_ptr<Config::ConfigManager> config,
                                       SpatialAudioEngine* engine,
                                       Rendering::DirectionVisualizer* visualizer)
    : m_config(std::move(config))
    , m_engine(engine)
    , m_visualizer(visualizer)
{
}

SpatialAudioRouter::~SpatialAudioRouter()
{
    Stop();
}

void SpatialAudioRouter::Start()
{
    if (m_running.exchange(true))
    {
        return;
    }

    m_thread = std::thread(&SpatialAudioRouter::Worker, this);
    ApplySensitivity();
}

void SpatialAudioRouter::Stop()
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

void SpatialAudioRouter::ApplySensitivity()
{
    if (m_visualizer)
    {
        m_visualizer->SetSensitivity(m_config->Sensitivity());
    }
}

void SpatialAudioRouter::Worker()
{
    using namespace std::chrono_literals;
    Util::DispatcherTimer timer{16ms};

    FILETIME lastKernel{}, lastUser{};
    auto lastSampleTime = std::chrono::steady_clock::now();

    auto evaluateLoad = [&](std::chrono::steady_clock::time_point now)
    {
        FILETIME creation{}, exit{}, kernel{}, user{};
        if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
        {
            auto toUint64 = [](const FILETIME& ft)
            {
                ULARGE_INTEGER value;
                value.LowPart = ft.dwLowDateTime;
                value.HighPart = ft.dwHighDateTime;
                return value.QuadPart;
            };

            ULONGLONG kernelDiff = toUint64(kernel) - toUint64(lastKernel);
            ULONGLONG userDiff = toUint64(user) - toUint64(lastUser);
            const double elapsed = std::chrono::duration<double>(now - lastSampleTime).count();
            if (elapsed > 0.0)
            {
                double cpuPercent = ((kernelDiff + userDiff) / 10000000.0) / elapsed * 100.0;

                PROCESS_MEMORY_COUNTERS counters{};
                counters.cb = sizeof(counters);
                SIZE_T memoryMb = 0;
                if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
                {
                    memoryMb = counters.WorkingSetSize / (1024 * 1024);
                }

                const auto& limits = m_config->Limits();
                if (cpuPercent > limits.maxCpuPercent || memoryMb > limits.maxMemoryMb)
                {
                    timer.SetInterval(48ms);
                }
                else
                {
                    timer.SetInterval(16ms);
                }
            }

            lastKernel = kernel;
            lastUser = user;
            lastSampleTime = now;
        }
    };

    while (m_running)
    {
        timer.Wait();

        evaluateLoad(std::chrono::steady_clock::now());

        if (!m_engine || !m_visualizer)
        {
            continue;
        }

        const auto direction = m_engine->GetDirectionSnapshot();
        m_visualizer->UpdateDirection(direction);
    }
}
