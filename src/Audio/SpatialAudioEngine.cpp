#include "Audio/SpatialAudioEngine.h"

#include "Config/ConfigManager.h"
#include "Util/ComException.h"

#include <Functiondiscoverykeys_devpkey.h>
#include <audiopolicy.h>
#include <endpointvolume.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <vector>

using namespace Audio;

namespace
{
constexpr DWORD kStreamFlags = AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
constexpr REFERENCE_TIME kBufferDuration100ns = 2000000; // 200ms

float ToDecibels(float value)
{
    constexpr float epsilon = 1e-6f;
    return 20.0f * std::log10f(std::max(value, epsilon));
}

std::wstring GetSessionDisplayName(Microsoft::WRL::ComPtr<IAudioSessionControl2> session)
{
    LPWSTR displayName{};
    if (SUCCEEDED(session->GetDisplayName(&displayName)) && displayName)
    {
        std::wstring value{displayName};
        CoTaskMemFree(displayName);
        if (!value.empty())
        {
            return value;
        }
    }

    DWORD pid{};
    if (SUCCEEDED(session->GetProcessId(&pid)))
    {
        return L"PID " + std::to_wstring(pid);
    }

    return L"System";
}
}

SpatialAudioEngine::SpatialAudioEngine(std::shared_ptr<Config::ConfigManager> config)
    : m_config(std::move(config))
{
}

SpatialAudioEngine::~SpatialAudioEngine()
{
    Shutdown();
}

void SpatialAudioEngine::Initialize()
{
    InitializeDevice();
    InitializeAudioClient();
    InitializeSessions();

    m_running = true;
    m_captureThread = std::thread(&SpatialAudioEngine::ProcessingLoop, this);
}

void SpatialAudioEngine::Shutdown()
{
    if (!m_running.exchange(false))
    {
        return;
    }

    if (m_stopEvent)
    {
        SetEvent(m_stopEvent);
    }

    if (m_captureThread.joinable())
    {
        m_captureThread.join();
    }

    if (m_audioClient)
    {
        m_audioClient->Stop();
    }

    if (m_waveFormat)
    {
        CoTaskMemFree(m_waveFormat);
        m_waveFormat = nullptr;
    }

    if (m_sampleEvent)
    {
        CloseHandle(m_sampleEvent);
        m_sampleEvent = nullptr;
    }

    if (m_stopEvent)
    {
        CloseHandle(m_stopEvent);
        m_stopEvent = nullptr;
    }

    m_captureClient.Reset();
    m_audioClient.Reset();
    m_sessionManager.Reset();
    m_device.Reset();
    m_deviceEnumerator.Reset();
}

AudioDirection SpatialAudioEngine::GetDirectionSnapshot()
{
    std::scoped_lock lock{m_mutex};
    return m_latestDirection;
}

void SpatialAudioEngine::InitializeDevice()
{
    THROW_IF_FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&m_deviceEnumerator)));
    THROW_IF_FAILED(m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device));
}

void SpatialAudioEngine::InitializeAudioClient()
{
    THROW_IF_FAILED(m_device->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, nullptr, &m_audioClient));

    THROW_IF_FAILED(m_audioClient->GetMixFormat(&m_waveFormat));
 
    const auto wfx = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_waveFormat);
    const bool isFloat = m_waveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        (m_waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE && wfx->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
    if (!isFloat)
    {
        throw std::runtime_error("Expected float mix format for loopback capture");
    }
 
    const bool isExtensible = (m_waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE);
    const DWORD channelMask = isExtensible ? wfx->dwChannelMask : (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
 
    // 立体声（典型耳机 / 虚拟环绕终端）
    m_isStereo = (m_waveFormat->nChannels <= 2) &&
        ((channelMask & ~(SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT)) == 0);
 
    // 多声道（5.1 / 7.1 等）
    m_isMultichannel = (m_waveFormat->nChannels >= 6);
 
    // 是否有明显的空间声道（顶部/后方/侧面）
    m_isSpatialAudio = (channelMask & (SPEAKER_TOP_FRONT_LEFT | SPEAKER_BACK_LEFT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT)) != 0;

    m_sampleEvent = CreateEventExW(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);
    m_stopEvent = CreateEventExW(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE);

    THROW_IF_FAILED(m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                              kStreamFlags,
                                              kBufferDuration100ns,
                                              0,
                                              m_waveFormat,
                                              nullptr));

    THROW_IF_FAILED(m_audioClient->SetEventHandle(m_sampleEvent));
    THROW_IF_FAILED(m_audioClient->GetService(IID_PPV_ARGS(&m_captureClient)));
}

void SpatialAudioEngine::InitializeSessions()
{
    THROW_IF_FAILED(m_device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, &m_sessionManager));
}

void SpatialAudioEngine::ProcessingLoop()
{
    THROW_IF_FAILED(m_audioClient->Start());

    HANDLE waitHandles[] = { m_stopEvent, m_sampleEvent };

    while (m_running)
    {
        const auto waitResult = WaitForMultipleObjects(static_cast<DWORD>(std::size(waitHandles)), waitHandles, FALSE, INFINITE);
        if (waitResult == WAIT_OBJECT_0)
        {
            break;
        }
        else if (waitResult == WAIT_OBJECT_0 + 1)
        {
            UINT32 packetFrames = 0;
            THROW_IF_FAILED(m_captureClient->GetNextPacketSize(&packetFrames));

            while (packetFrames > 0)
            {
                BYTE* data{};
                UINT32 framesToRead{};
                DWORD flags{};
                THROW_IF_FAILED(m_captureClient->GetBuffer(&data, &framesToRead, &flags, nullptr, nullptr));

                if ((flags & AUDCLNT_BUFFERFLAGS_SILENT) == 0)
                {
                    ProcessBuffer(data, framesToRead);
                }
                else
                {
                    ProcessBuffer(nullptr, framesToRead);
                }

                UpdateDominantSession();

                THROW_IF_FAILED(m_captureClient->ReleaseBuffer(framesToRead));
                THROW_IF_FAILED(m_captureClient->GetNextPacketSize(&packetFrames));
            }
        }
    }
}

void SpatialAudioEngine::ProcessBuffer(BYTE* data, UINT32 frames)
{
    const UINT32 channelCount = m_waveFormat->nChannels;
    ChannelEnergy energy;

    if (data)
    {
        const auto* samples = reinterpret_cast<const float*>(data);
        energy = CalculateChannelEnergy(samples, frames, channelCount);
    }

    const auto direction = ResolveDirection(energy);

    std::scoped_lock lock{m_mutex};
    m_latestDirection = direction;
}

SpatialAudioEngine::ChannelEnergy SpatialAudioEngine::CalculateChannelEnergy(const float* samples, UINT32 frames, UINT32 channelCount) const
{
    ChannelEnergy energy;

    if (!samples || channelCount == 0)
    {
        return energy;
    }

    std::vector<double> rms(channelCount, 0.0);

    for (UINT32 frame = 0; frame < frames; ++frame)
    {
        for (UINT32 channel = 0; channel < channelCount; ++channel)
        {
            const auto sample = samples[frame * channelCount + channel];
            rms[channel] += sample * sample;
        }
    }

    for (auto& value : rms)
    {
        value = std::sqrt(value / std::max<UINT32>(1, frames));
    }

    const auto wfx = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(m_waveFormat);
    const DWORD mask = (m_waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE) ? wfx->dwChannelMask : (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);

    auto channelSpeaker = [&](UINT32 index) -> DWORD
    {
        static const DWORD speakerOrder[] = {
            SPEAKER_FRONT_LEFT,
            SPEAKER_FRONT_RIGHT,
            SPEAKER_FRONT_CENTER,
            SPEAKER_LOW_FREQUENCY,
            SPEAKER_BACK_LEFT,
            SPEAKER_BACK_RIGHT,
            SPEAKER_FRONT_LEFT_OF_CENTER,
            SPEAKER_FRONT_RIGHT_OF_CENTER,
            SPEAKER_BACK_CENTER,
            SPEAKER_SIDE_LEFT,
            SPEAKER_SIDE_RIGHT,
            SPEAKER_TOP_CENTER,
            SPEAKER_TOP_FRONT_LEFT,
            SPEAKER_TOP_FRONT_CENTER,
            SPEAKER_TOP_FRONT_RIGHT,
            SPEAKER_TOP_BACK_LEFT,
            SPEAKER_TOP_BACK_CENTER,
            SPEAKER_TOP_BACK_RIGHT,
        };

        if (mask == 0)
        {
            return speakerOrder[index < std::size(speakerOrder) ? index : 0];
        }

        UINT32 bitIndex = 0;
        for (DWORD speakerBit : speakerOrder)
        {
            if ((mask & speakerBit) != 0)
            {
                if (bitIndex == index)
                {
                    return speakerBit;
                }
                ++bitIndex;
            }
        }

        return speakerOrder[index < std::size(speakerOrder) ? index : 0];
    };

    for (UINT32 channel = 0; channel < channelCount; ++channel)
    {
        const double level = rms[channel];
        const double db = ToDecibels(static_cast<float>(level));
        const double clampedDb = db - m_config->Sensitivity().thresholdDb;
        const float normalized = static_cast<float>(std::clamp(clampedDb / 60.0, 0.0, 1.0));

        const DWORD speaker = channelSpeaker(channel);

        if (speaker & (SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER))
        {
            energy.front += normalized;
        }
        // 把 SIDE 通道也计入“后方”，因为很多 7.1 配置用 SIDE_* 做后环绕
        if (speaker & (SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT | SPEAKER_BACK_CENTER))
        {
            energy.back += normalized;
        }
        if (speaker & (SPEAKER_SIDE_LEFT | SPEAKER_BACK_LEFT | SPEAKER_FRONT_LEFT))
        {
            energy.left += normalized;
        }
        if (speaker & (SPEAKER_SIDE_RIGHT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_RIGHT))
        {
            energy.right += normalized;
        }
        if (speaker & (SPEAKER_TOP_FRONT_LEFT | SPEAKER_TOP_FRONT_RIGHT | SPEAKER_TOP_BACK_LEFT | SPEAKER_TOP_BACK_RIGHT))
        {
            energy.top += normalized;
        }
        if (speaker & (SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_CENTER))
        {
            energy.bottom += normalized;
        }
    }

    return energy;
}

AudioDirection SpatialAudioEngine::ResolveDirection(const ChannelEnergy& energy) const
{
    AudioDirection direction;

    float front = m_config->Filter().front ? energy.front : 0.0f;
    float back = m_config->Filter().back ? energy.back : 0.0f;
    float left = m_config->Filter().left ? energy.left : 0.0f;
    float right = m_config->Filter().right ? energy.right : 0.0f;
    float top = m_config->Filter().up ? energy.top : 0.0f;
    float bottom = m_config->Filter().down ? energy.bottom : 0.0f;

    const auto overrideMode = m_config->AudioMode();

    // 根据配置和检测结果决定当前是否按“耳机模式（仅左右）”展示
    const auto overrideMode = m_config->AudioMode();
    bool headphoneMode = false;
    if (m_config->AudioMode() == Config::AudioModeOverride::Headphone)
    {
        headphoneMode = true;
    }
    else if (m_config->AudioMode() == Config::AudioModeOverride::Multichannel)
    {
        headphoneMode = false;
    }
    else // Auto
    {
        headphoneMode = m_isStereo;
    }

    if (headphoneMode)
    {
        front = back = 0.0f;
        top = bottom = 0.0f;
    }

    const float horizontalTotal = front + back + left + right;
    const float verticalTotal = top + bottom;
    const float magnitude = horizontalTotal + verticalTotal;
    const float lrTotal = left + right;

    // BGM 检测：左右几乎完全平衡时视为背景音，忽略
    const float lrTotal = left + right;
    if ((left + right) > 0.0001f)
    {
        const float lrDiff = std::fabs(left - right);
        const float balance = lrDiff / (left + right);
        if (balance < 0.1f) // 左右差异低于 10%
        {
            direction.isBackground = true;
            direction.magnitude = 0.0f;
            return direction;
        }
    }

    if (magnitude <= 0.001f)
    {
        direction.magnitude = 0.0f;
        return direction;
    }

    const float x = right - left;
    const float z = front - back;
    const float y = top - bottom;

    direction.azimuth = std::atan2f(x, z);
    direction.elevation = std::atan2f(y, std::sqrt(x * x + z * z));
    direction.magnitude = magnitude / 6.0f;

    return direction;
}

void SpatialAudioEngine::UpdateDominantSession()
{
    if (!m_sessionManager)
    {
        return;
    }

    Microsoft::WRL::ComPtr<IAudioSessionEnumerator> enumerator;
    if (FAILED(m_sessionManager->GetSessionEnumerator(&enumerator)))
    {
        return;
    }

    int sessionCount = 0;
    if (FAILED(enumerator->GetCount(&sessionCount)))
    {
        return;
    }

    float strongestLevel = -1000.0f;
    std::wstring strongestName;

    for (int i = 0; i < sessionCount; ++i)
    {
        Microsoft::WRL::ComPtr<IAudioSessionControl> control;
        if (FAILED(enumerator->GetSession(i, &control)))
        {
            continue;
        }

        Microsoft::WRL::ComPtr<IAudioSessionControl2> control2;
        if (FAILED(control.As(&control2)))
        {
            continue;
        }

        Microsoft::WRL::ComPtr<IAudioMeterInformation> meter;
        if (FAILED(control2.As(&meter)))
        {
            continue;
        }

        float peak = 0.0f;
        if (FAILED(meter->GetPeakValue(&peak)))
        {
            continue;
        }

        const float db = ToDecibels(peak);
        if (db > strongestLevel)
        {
            strongestLevel = db;
            strongestName = GetSessionDisplayName(control2);
        }
    }

    std::scoped_lock lock{m_mutex};
    m_latestDirection.dominantSessionName = strongestName;
}
