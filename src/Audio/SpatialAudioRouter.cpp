#include "Audio/SpatialAudioRouter.h"

#include "Config/ConfigManager.h"
#include "Rendering/DirectionVisualizer.h"
#include "Util/DispatcherTimer.h"

#include <Psapi.h>
#include <windows.h>

#include <chrono>
#include <cmath>

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

        // 计算当前可视化模式并更新 UI 文本
        const auto overrideMode = m_config->AudioMode();
        std::wstring label;
        if (overrideMode == Config::AudioModeOverride::Headphone)
        {
            label = L"Headphone mode (LR only)";
        }
        else if (overrideMode == Config::AudioModeOverride::Multichannel)
        {
            label = L"Multichannel mode (3D)";
        }
        else // Auto
        {
            if (m_engine->IsStereo())
            {
                label = L"Headphone mode (LR only)";
            }
            else if (m_engine->IsMultichannel() || m_engine->IsSpatialAudioActive())
            {
                label = L"Multichannel mode (3D)";
            }
            else
            {
                label = L"Stereo (LR only)";
            }
        }

        // Append current pattern preset to label
        const auto& s = m_config->Sensitivity();
        auto approxEq = [](float a, float b)
        {
            return std::fabs(a - b) < 0.01f;
        };

        auto matches = [&](float strongMag,
                           float strongJump,
                           float minInt,
                           float maxInt,
                           float dirDeg)
        {
            return approxEq(s.strongMagnitude, strongMag) &&
                   approxEq(s.strongJump, strongJump) &&
                   approxEq(s.rhythmMinInterval, minInt) &&
                   approxEq(s.rhythmMaxInterval, maxInt) &&
                   approxEq(s.rhythmDirectionDeg, dirDeg);
        };

        const wchar_t* presetLabel = L"Custom";
        if (matches(0.7f, 0.35f, 0.30f, 0.60f, 30.0f))
        {
            presetLabel = L"Conservative";
        }
        else if (matches(0.6f, 0.25f, 0.25f, 0.70f, 40.0f))
        {
            presetLabel = L"Balanced";
        }
        else if (matches(0.5f, 0.15f, 0.20f, 0.80f, 60.0f))
        {
            presetLabel = L"Aggressive";
        }

        label += L" | Pattern: ";
        label += presetLabel;

        m_visualizer->SetModeLabel(label);
        m_visualizer->UpdateDirection(direction);
    }
}
