#include "AudioCaptureEngine.h"
#include "../Common/Logger.h"
#include "../Common/ErrorHandler.h"
#include "../Common/WindowsCompat.h"
#include <comdef.h>
#include <functiondiscoverykeys_devpkey.h>
#include <propvarutil.h>
#include <thread>
#include <mutex>

AudioCaptureEngine::AudioCaptureEngine()
    : m_isCapturing(false)
    , m_spatialAudioSupported(false)
    , m_deviceEnumerator(nullptr)
    , m_audioDevice(nullptr)
    , m_audioClient(nullptr)
    , m_captureClient(nullptr)
    , m_spatialAudioClient(nullptr)
    , m_spatialAudioStream(nullptr)
    , m_audioFormat(nullptr)
    , m_bufferFrameCount(0)
    , m_threadRunning(false)
{
    Logger::Info("AudioCaptureEngine created");
}

AudioCaptureEngine::~AudioCaptureEngine()
{
    Shutdown();
    Logger::Info("AudioCaptureEngine destroyed");
}

bool AudioCaptureEngine::Initialize()
{
    Logger::Info("Initializing AudioCaptureEngine...");

    // 初始化COM
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        ErrorHandler::HandleAudioError(AudioErrorType::InitializationFailed, "COM initialization failed");
        return false;
    }

    // 创建设备枚举器
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                         __uuidof(IMMDeviceEnumerator), (void**)&m_deviceEnumerator);
    if (FAILED(hr))
    {
        ErrorHandler::HandleAudioError(AudioErrorType::InitializationFailed, "Failed to create device enumerator");
        return false;
    }

    // 枚举音频设备
    if (!EnumerateAudioDevices())
    {
        Logger::Warning("Failed to enumerate audio devices");
    }

    // 初始化音频设备
    if (!InitializeAudioDevice())
    {
        Logger::Error("Failed to initialize audio device");
        return false;
    }

    // 检测空间音频支持
    m_spatialAudioSupported = DetectSpatialAudioSupport();
    
    if (m_spatialAudioSupported)
    {
        Logger::Info("Spatial Audio supported, initializing spatial audio client");
        if (!InitializeSpatialAudio())
        {
            Logger::Warning("Failed to initialize spatial audio, falling back to WASAPI");
            m_spatialAudioSupported = false;
        }
    }

    if (!m_spatialAudioSupported)
    {
        Logger::Info("Using WASAPI for audio capture");
        if (!InitializeWASAPI())
        {
            Logger::Error("Failed to initialize WASAPI");
            return false;
        }
    }

    Logger::Info("AudioCaptureEngine initialized successfully");
    return true;
}

void AudioCaptureEngine::Shutdown()
{
    Logger::Info("Shutting down AudioCaptureEngine...");

    StopCapture();
    CleanupAudioResources();

    CoUninitialize();
    Logger::Info("AudioCaptureEngine shutdown complete");
}

void AudioCaptureEngine::StartCapture()
{
    if (m_isCapturing)
        return;

    Logger::Info("Starting audio capture...");

    m_threadRunning = true;
    m_captureThread = std::thread(&AudioCaptureEngine::AudioCaptureThread, this);
    m_isCapturing = true;

    Logger::Info("Audio capture started");
}

void AudioCaptureEngine::StopCapture()
{
    if (!m_isCapturing)
        return;

    Logger::Info("Stopping audio capture...");

    m_threadRunning = false;
    m_isCapturing = false;

    if (m_captureThread.joinable())
    {
        m_captureThread.join();
    }

    Logger::Info("Audio capture stopped");
}

SpatialAudioData AudioCaptureEngine::GetCurrentAudioData()
{
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_currentAudioData;
}

void AudioCaptureEngine::SetSensitivity(float sensitivity)
{
    m_config.sensitivity = clamp(sensitivity, 0.0f, 1.0f);
    Logger::Debug("Audio sensitivity set to: " + std::to_string(m_config.sensitivity));
}

void AudioCaptureEngine::UpdateConfig(const AudioConfig& config)
{
    m_config = config;
    Logger::Info("Audio configuration updated");
}

std::vector<std::string> AudioCaptureEngine::GetAvailableDevices()
{
    return m_availableDevices;
}

bool AudioCaptureEngine::SelectDevice(const std::string& deviceId)
{
    m_selectedDeviceId = deviceId;
    Logger::Info("Selected audio device: " + deviceId);
    
    // 重新初始化设备
    bool wasCapturing = m_isCapturing;
    if (wasCapturing)
        StopCapture();

    CleanupAudioResources();
    bool success = InitializeAudioDevice();

    if (wasCapturing && success)
        StartCapture();

    return success;
}

void AudioCaptureEngine::SetAudioDataCallback(std::function<void(const SpatialAudioData&)> callback)
{
    m_audioDataCallback = callback;
}

bool AudioCaptureEngine::InitializeAudioDevice()
{
    Logger::Debug("Initializing audio device...");

    HRESULT hr;
    
    // 获取默认音频设备
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_audioDevice);
    if (FAILED(hr))
    {
        ErrorHandler::HandleAudioError(AudioErrorType::DeviceNotFound, "Failed to get default audio device");
        return false;
    }

    // 激活音频客户端
    hr = m_audioDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
    if (FAILED(hr))
    {
        ErrorHandler::HandleAudioError(AudioErrorType::InitializationFailed, "Failed to activate audio client");
        return false;
    }

    Logger::Debug("Audio device initialized successfully");
    return true;
}

bool AudioCaptureEngine::InitializeSpatialAudio()
{
    Logger::Debug("Initializing spatial audio...");

    HRESULT hr;

    // 获取空间音频客户端
    hr = m_audioDevice->Activate(__uuidof(ISpatialAudioClient), CLSCTX_ALL, nullptr, (void**)&m_spatialAudioClient);
    if (FAILED(hr))
    {
        Logger::Warning("Failed to activate spatial audio client");
        return false;
    }

    // 检查空间音频支持
    hr = m_spatialAudioClient->IsAudioObjectFormatSupported(AudioObjectType_Dynamic);
    if (hr != S_OK)
    {
        Logger::Warning("Spatial audio format not supported");
        return false;
    }
    if (supportedFormat) {
        CoTaskMemFree(supportedFormat);
    }

    // 创建空间音频流
    SpatialAudioObjectRenderStreamActivationParams activationParams = {};
    activationParams.ObjectFormat = nullptr; // 使用默认格式
    activationParams.StaticObjectTypeMask = AudioObjectType_None;
    activationParams.MinDynamicObjectCount = 1;
    activationParams.MaxDynamicObjectCount = 16;
    activationParams.Category = AudioCategory_GameEffects;
    activationParams.EventHandle = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    PROPVARIANT activationProp;
    PropVariantInit(&activationProp);
    activationProp.vt = VT_BLOB;
    activationProp.blob.cbSize = sizeof(activationParams);
    activationProp.blob.pBlobData = reinterpret_cast<BYTE*>(&activationParams);
    
    hr = m_spatialAudioClient->ActivateSpatialAudioStream(&activationProp, 
                                                         __uuidof(ISpatialAudioObjectRenderStream),
                                                         (void**)&m_spatialAudioStream);
    PropVariantClear(&activationProp);
    if (FAILED(hr))
    {
        Logger::Warning("Failed to activate spatial audio stream");
        CloseHandle(activationParams.EventHandle);
        return false;
    }

    Logger::Info("Spatial audio initialized successfully");
    return true;
}

bool AudioCaptureEngine::InitializeWASAPI()
{
    Logger::Debug("Initializing WASAPI...");

    HRESULT hr;

    // 获取音频格式
    hr = m_audioClient->GetMixFormat(&m_audioFormat);
    if (FAILED(hr))
    {
        ErrorHandler::HandleAudioError(AudioErrorType::InitializationFailed, "Failed to get audio format");
        return false;
    }

    // 初始化音频客户端
    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                  AUDCLNT_STREAMFLAGS_LOOPBACK,
                                  10000000, // 1秒缓冲
                                  0,
                                  m_audioFormat,
                                  nullptr);
    if (FAILED(hr))
    {
        ErrorHandler::HandleAudioError(AudioErrorType::InitializationFailed, "Failed to initialize audio client");
        return false;
    }

    // 获取缓冲区大小
    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if (FAILED(hr))
    {
        ErrorHandler::HandleAudioError(AudioErrorType::InitializationFailed, "Failed to get buffer size");
        return false;
    }

    // 获取捕获客户端
    hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_captureClient);
    if (FAILED(hr))
    {
        ErrorHandler::HandleAudioError(AudioErrorType::InitializationFailed, "Failed to get capture client");
        return false;
    }

    Logger::Info("WASAPI initialized successfully");
    return true;
}

void AudioCaptureEngine::CleanupAudioResources()
{
    Logger::Debug("Cleaning up audio resources...");

    if (m_captureClient)
    {
        m_captureClient->Release();
        m_captureClient = nullptr;
    }

    if (m_spatialAudioStream)
    {
        m_spatialAudioStream->Release();
        m_spatialAudioStream = nullptr;
    }

    if (m_spatialAudioClient)
    {
        m_spatialAudioClient->Release();
        m_spatialAudioClient = nullptr;
    }

    if (m_audioClient)
    {
        m_audioClient->Release();
        m_audioClient = nullptr;
    }

    if (m_audioDevice)
    {
        m_audioDevice->Release();
        m_audioDevice = nullptr;
    }

    if (m_deviceEnumerator)
    {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }

    if (m_audioFormat)
    {
        CoTaskMemFree(m_audioFormat);
        m_audioFormat = nullptr;
    }
}

bool AudioCaptureEngine::DetectSpatialAudioSupport()
{
    Logger::Debug("Detecting spatial audio support...");

    if (!m_audioDevice)
        return false;

    ISpatialAudioClient* testClient = nullptr;
    HRESULT hr = m_audioDevice->Activate(__uuidof(ISpatialAudioClient), CLSCTX_ALL, nullptr, (void**)&testClient);
    
    if (SUCCEEDED(hr) && testClient)
    {
        // 测试是否支持动态对象
        hr = testClient->IsAudioObjectFormatSupported(AudioObjectType_Dynamic);
        testClient->Release();
        
        bool supported = (hr == S_OK);
        Logger::Info("Spatial audio support: " + std::string(supported ? "YES" : "NO"));
        return supported;
    }

    Logger::Info("Spatial audio support: NO");
    return false;
}

bool AudioCaptureEngine::EnumerateAudioDevices()
{
    Logger::Debug("Enumerating audio devices...");

    if (!m_deviceEnumerator)
        return false;

    IMMDeviceCollection* deviceCollection = nullptr;
    HRESULT hr = m_deviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
    
    if (FAILED(hr))
    {
        Logger::Warning("Failed to enumerate audio devices");
        return false;
    }

    UINT deviceCount = 0;
    hr = deviceCollection->GetCount(&deviceCount);
    
    if (SUCCEEDED(hr))
    {
        m_availableDevices.clear();
        
        for (UINT i = 0; i < deviceCount; i++)
        {
            IMMDevice* device = nullptr;
            hr = deviceCollection->Item(i, &device);
            
            if (SUCCEEDED(hr))
            {
                LPWSTR deviceId = nullptr;
                hr = device->GetId(&deviceId);
                
                if (SUCCEEDED(hr))
                {
                    // 转换为字符串
                    int size = WideCharToMultiByte(CP_UTF8, 0, deviceId, -1, nullptr, 0, nullptr, nullptr);
                    std::string deviceIdStr(size - 1, 0);
                    WideCharToMultiByte(CP_UTF8, 0, deviceId, -1, &deviceIdStr[0], size, nullptr, nullptr);
                    
                    m_availableDevices.push_back(deviceIdStr);
                    CoTaskMemFree(deviceId);
                }
                
                device->Release();
            }
        }
        
        Logger::Info("Found " + std::to_string(m_availableDevices.size()) + " audio devices");
    }

    deviceCollection->Release();
    return true;
}

void AudioCaptureEngine::AudioCaptureThread()
{
    Logger::Debug("Audio capture thread started");

    // 启动音频客户端
    HRESULT hr = m_audioClient->Start();
    if (FAILED(hr))
    {
        ErrorHandler::HandleAudioError(AudioErrorType::CaptureError, "Failed to start audio client");
        return;
    }

    while (m_threadRunning)
    {
        ProcessAudioData();
        Sleep(10); // 100Hz更新频率
    }

    // 停止音频客户端
    m_audioClient->Stop();
    Logger::Debug("Audio capture thread ended");
}

void AudioCaptureEngine::ProcessAudioData()
{
    bool success = false;
    
    if (m_spatialAudioSupported && m_spatialAudioStream)
    {
        success = ProcessSpatialAudioFrame();
    }
    else if (m_captureClient)
    {
        success = ProcessWASAPIFrame();
    }

    if (!success)
    {
        // 处理错误，可能需要重新初始化
        static int errorCount = 0;
        errorCount++;
        
        if (errorCount > 100) // 连续错误过多
        {
            ErrorHandler::HandleAudioError(AudioErrorType::CaptureError, "Too many capture errors");
            m_threadRunning = false;
        }
    }
}

bool AudioCaptureEngine::ProcessSpatialAudioFrame()
{
    // 空间音频处理逻辑
    // 这里需要实现具体的空间音频数据提取
    // 由于空间音频API比较复杂，这里提供基础框架
    
    try
    {
        // 获取可用的音频对象数量
        UINT32 availableObjectCount = 0;
        HRESULT hr = m_spatialAudioStream->GetAvailableDynamicObjectCount(&availableObjectCount);
        
        if (SUCCEEDED(hr) && availableObjectCount > 0)
        {
            // 处理空间音频对象
            // 这里需要根据实际的空间音频数据格式来实现
            
            std::lock_guard<std::mutex> lock(m_dataMutex);
            
            // 模拟空间音频数据（实际实现需要从API获取真实数据）
            m_currentAudioData.intensity = 0.5f;
            m_currentAudioData.confidence = 0.8f;
            m_currentAudioData.primaryDirection = DirectionVector(0.0f, 0.0f, 1.0f);
            
            // 触发回调
            if (m_audioDataCallback)
            {
                m_audioDataCallback(m_currentAudioData);
            }
        }
        
        return true;
    }
    catch (...)
    {
        Logger::Error("Exception in ProcessSpatialAudioFrame");
        return false;
    }
}

bool AudioCaptureEngine::ProcessWASAPIFrame()
{
    HRESULT hr;
    UINT32 packetLength = 0;
    
    hr = m_captureClient->GetNextPacketSize(&packetLength);
    if (FAILED(hr))
        return false;

    while (packetLength != 0)
    {
        BYTE* audioData = nullptr;
        UINT32 frameCount = 0;
        DWORD flags = 0;

        hr = m_captureClient->GetBuffer(&audioData, &frameCount, &flags, nullptr, nullptr);
        if (FAILED(hr))
            break;

        if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT))
        {
            ExtractStereoData(audioData, frameCount);
        }

        hr = m_captureClient->ReleaseBuffer(frameCount);
        if (FAILED(hr))
            break;

        hr = m_captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr))
            break;
    }

    return SUCCEEDED(hr);
}

void AudioCaptureEngine::ExtractStereoData(const void* audioData, UINT32 frameCount)
{
    if (!audioData || frameCount == 0)
        return;

    const float* samples = static_cast<const float*>(audioData);
    UINT32 channelCount = m_audioFormat->nChannels;

    if (channelCount >= 2)
    {
        // 计算左右声道的平均强度
        float leftSum = 0.0f, rightSum = 0.0f;
        
        for (UINT32 i = 0; i < frameCount; i++)
        {
            leftSum += std::abs(samples[i * channelCount]);
            rightSum += std::abs(samples[i * channelCount + 1]);
        }

        float leftAvg = leftSum / frameCount;
        float rightAvg = rightSum / frameCount;

        // 模拟方向计算
        DirectionVector direction = SimulateDirectionFromStereo(&leftAvg, &rightAvg, 1);

        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_currentAudioData.primaryDirection = direction;
        m_currentAudioData.intensity = (leftAvg + rightAvg) * 0.5f * m_config.sensitivity;
        m_currentAudioData.confidence = 0.6f; // 立体声模拟的置信度较低

        // 触发回调
        if (m_audioDataCallback)
        {
            m_audioDataCallback(m_currentAudioData);
        }
    }
}

DirectionVector AudioCaptureEngine::SimulateDirectionFromStereo(const float* leftChannel, const float* rightChannel, UINT32 frameCount)
{
    DirectionVector direction;

    if (frameCount == 0)
        return direction;

    float leftIntensity = *leftChannel;
    float rightIntensity = *rightChannel;

    // 简单的立体声方向模拟
    float balance = (rightIntensity - leftIntensity) / (rightIntensity + leftIntensity + 0.001f);
    
    // 将平衡值转换为方位角
    direction.azimuth = balance * 90.0f; // -90° 到 +90°
    direction.elevation = 0.0f; // 立体声无法检测垂直方向
    direction.x = std::sin(direction.azimuth * M_PI / 180.0f);
    direction.y = 0.0f;
    direction.z = std::cos(direction.azimuth * M_PI / 180.0f);
    direction.distance = (leftIntensity + rightIntensity) * 0.5f;

    return direction;
}