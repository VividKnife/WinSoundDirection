#pragma once

#include "../Common/Types.h"
#include "../Common/Config.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <spatialaudioclient.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

class AudioCaptureEngine
{
public:
    AudioCaptureEngine();
    ~AudioCaptureEngine();

    // 初始化和清理
    bool Initialize();
    void Shutdown();

    // 音频捕获控制
    void StartCapture();
    void StopCapture();
    bool IsCapturing() const { return m_isCapturing; }

    // 数据访问
    SpatialAudioData GetCurrentAudioData();
    void SetSensitivity(float sensitivity);

    // 配置更新
    void UpdateConfig(const AudioConfig& config);

    // 设备管理
    bool IsSpatialAudioSupported() const { return m_spatialAudioSupported; }
    std::vector<std::string> GetAvailableDevices();
    bool SelectDevice(const std::string& deviceId);

    // 事件回调
    void SetAudioDataCallback(std::function<void(const SpatialAudioData&)> callback);

private:
    // 设备初始化
    bool InitializeAudioDevice();
    bool InitializeSpatialAudio();
    bool InitializeWASAPI();
    void CleanupAudioResources();

    // 设备检测
    bool DetectSpatialAudioSupport();
    bool EnumerateAudioDevices();

    // 音频捕获线程
    void AudioCaptureThread();
    void ProcessAudioData();

    // 空间音频处理
    bool ProcessSpatialAudioFrame();
    void ExtractSpatialData(const void* audioData, UINT32 frameCount);

    // WASAPI处理
    bool ProcessWASAPIFrame();
    void ExtractStereoData(const void* audioData, UINT32 frameCount);

    // 数据转换
    DirectionVector CalculateDirectionFromSpatial(const float* spatialData, UINT32 channelCount);
    DirectionVector SimulateDirectionFromStereo(const float* leftChannel, const float* rightChannel, UINT32 frameCount);

    // 成员变量
    AudioConfig m_config;
    std::atomic<bool> m_isCapturing;
    std::atomic<bool> m_spatialAudioSupported;
    
    // COM接口
    IMMDeviceEnumerator* m_deviceEnumerator;
    IMMDevice* m_audioDevice;
    IAudioClient* m_audioClient;
    IAudioCaptureClient* m_captureClient;
    ISpatialAudioClient* m_spatialAudioClient;
    ISpatialAudioObjectRenderStream* m_spatialAudioStream;

    // 音频格式
    WAVEFORMATEX* m_audioFormat;
    UINT32 m_bufferFrameCount;

    // 线程管理
    std::thread m_captureThread;
    std::atomic<bool> m_threadRunning;

    // 数据同步
    std::mutex m_dataMutex;
    SpatialAudioData m_currentAudioData;
    
    // 回调
    std::function<void(const SpatialAudioData&)> m_audioDataCallback;

    // 设备信息
    std::vector<std::string> m_availableDevices;
    std::string m_selectedDeviceId;
};