#pragma once

#include <string>
#include <functional>

enum class ErrorType
{
    AudioDevice,
    SpatialAudio,
    Rendering,
    WindowManagement,
    Configuration,
    Performance
};

enum class AudioErrorType
{
    DeviceNotFound,
    DeviceDisconnected,
    InitializationFailed,
    CaptureError,
    SpatialAudioNotSupported
};

enum class RenderErrorType
{
    Direct2DInitFailed,
    RenderTargetLost,
    ResourceCreationFailed,
    DrawingError
};

enum class WindowErrorType
{
    CreationFailed,
    SetTopMostFailed,
    PositionError,
    FullscreenDetectionFailed
};

enum class NotificationType
{
    Info,
    Warning,
    Error
};

class ErrorHandler
{
public:
    static void Initialize();
    static void Shutdown();
    
    // 错误处理方法
    static void HandleAudioError(AudioErrorType error, const std::string& details = "");
    static void HandleRenderError(RenderErrorType error, const std::string& details = "");
    static void HandleWindowError(WindowErrorType error, const std::string& details = "");
    
    // 用户通知
    static void ShowUserNotification(const std::string& message, NotificationType type);
    
    // 恢复尝试
    static bool AttemptRecovery(ErrorType type);
    
    // 设置错误回调
    static void SetErrorCallback(std::function<void(ErrorType, const std::string&)> callback);
    
private:
    static void LogError(ErrorType type, const std::string& error);
    static std::string ErrorTypeToString(ErrorType type);
    static std::string AudioErrorToString(AudioErrorType error);
    static std::string RenderErrorToString(RenderErrorType error);
    static std::string WindowErrorToString(WindowErrorType error);
    
    static std::function<void(ErrorType, const std::string&)> s_errorCallback;
    static bool s_initialized;
};