#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <audioclient.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <spatialaudiohrtf.h>
#include <wrl/client.h>

#include "Config/ConfigManager.h"

namespace Audio
{
struct AudioDirection
{
    float azimuth{0.0f};
    float elevation{0.0f};
    float magnitude{0.0f};
    std::wstring dominantSessionName;
};

class SpatialAudioEngine
{
public:
    explicit SpatialAudioEngine(std::shared_ptr<Config::ConfigManager> config);
    ~SpatialAudioEngine();

    void Initialize();
    void Shutdown();

    [[nodiscard]] AudioDirection GetDirectionSnapshot();
    [[nodiscard]] bool IsSpatialAudioActive() const noexcept { return m_isSpatialAudio; }

private:
    struct ChannelEnergy
    {
        float front{0.0f};
        float back{0.0f};
        float left{0.0f};
        float right{0.0f};
        float top{0.0f};
        float bottom{0.0f};
    };

    void InitializeDevice();
    void InitializeAudioClient();
    void InitializeSessions();
    void ProcessingLoop();
    void ProcessBuffer(BYTE* data, UINT32 frames);
    ChannelEnergy CalculateChannelEnergy(const float* samples, UINT32 frames, UINT32 channelCount) const;
    AudioDirection ResolveDirection(const ChannelEnergy& energy) const;
    void UpdateDominantSession();

    std::shared_ptr<Config::ConfigManager> m_config;

    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> m_deviceEnumerator;
    Microsoft::WRL::ComPtr<IMMDevice> m_device;
    Microsoft::WRL::ComPtr<IAudioClient3> m_audioClient;
    Microsoft::WRL::ComPtr<IAudioCaptureClient> m_captureClient;
    Microsoft::WRL::ComPtr<IAudioSessionManager2> m_sessionManager;

    HANDLE m_sampleEvent{nullptr};
    HANDLE m_stopEvent{nullptr};
    std::thread m_captureThread;
    std::atomic<bool> m_running{false};

    WAVEFORMATEX* m_waveFormat{nullptr};
    bool m_isSpatialAudio{false};

    mutable std::mutex m_mutex;
    AudioDirection m_latestDirection;
};
}
