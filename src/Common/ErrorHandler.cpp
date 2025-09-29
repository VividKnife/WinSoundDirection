#include "ErrorHandler.h"
#include "Logger.h"
#include "WindowsCompat.h"

std::function<void(ErrorType, const std::string&)> ErrorHandler::s_errorCallback = nullptr;
bool ErrorHandler::s_initialized = false;

void ErrorHandler::Initialize()
{
    s_initialized = true;
    Logger::Info("ErrorHandler initialized");
}

void ErrorHandler::Shutdown()
{
    s_initialized = false;
    s_errorCallback = nullptr;
    Logger::Info("ErrorHandler shutdown");
}

void ErrorHandler::HandleAudioError(AudioErrorType error, const std::string& details)
{
    std::string errorMsg = AudioErrorToString(error);
    if (!details.empty())
        errorMsg += ": " + details;
    
    LogError(ErrorType::AudioDevice, errorMsg);
    
    // 尝试恢复
    if (error == AudioErrorType::DeviceDisconnected || error == AudioErrorType::CaptureError)
    {
        AttemptRecovery(ErrorType::AudioDevice);
    }
    
    // 显示用户通知
    if (error == AudioErrorType::SpatialAudioNotSupported)
    {
        ShowUserNotification("Your audio device does not support spatial audio. The program will use stereo mode.", NotificationType::Warning);
    }
}

void ErrorHandler::HandleRenderError(RenderErrorType error, const std::string& details)
{
    std::string errorMsg = RenderErrorToString(error);
    if (!details.empty())
        errorMsg += ": " + details;
    
    LogError(ErrorType::Rendering, errorMsg);
    
    // 尝试恢复
    if (error == RenderErrorType::RenderTargetLost)
    {
        AttemptRecovery(ErrorType::Rendering);
    }
}

void ErrorHandler::HandleWindowError(WindowErrorType error, const std::string& details)
{
    std::string errorMsg = WindowErrorToString(error);
    if (!details.empty())
        errorMsg += ": " + details;
    
    LogError(ErrorType::WindowManagement, errorMsg);
    
    // 尝试恢复
    AttemptRecovery(ErrorType::WindowManagement);
}

void ErrorHandler::ShowUserNotification(const std::string& message, NotificationType type)
{
    UINT iconType = MB_ICONINFORMATION;
    std::string title = "Spatial Audio Visualizer";
    
    switch (type)
    {
        case NotificationType::Warning:
            iconType = MB_ICONWARNING;
            title += " - Warning";
            break;
        case NotificationType::Error:
            iconType = MB_ICONERROR;
            title += " - Error";
            break;
        default:
            break;
    }
    
    MessageBoxA(nullptr, message.c_str(), title.c_str(), iconType | MB_OK);
}

bool ErrorHandler::AttemptRecovery(ErrorType type)
{
    Logger::Info("Attempting recovery for error type: " + ErrorTypeToString(type));
    
    // 这里将在后续实现具体的恢复逻辑
    // 现在只是记录尝试恢复的日志
    
    if (s_errorCallback)
    {
        s_errorCallback(type, "Recovery attempted");
    }
    
    return false; // 暂时返回false，具体恢复逻辑将在相应组件中实现
}

void ErrorHandler::SetErrorCallback(std::function<void(ErrorType, const std::string&)> callback)
{
    s_errorCallback = callback;
}

void ErrorHandler::LogError(ErrorType type, const std::string& error)
{
    std::string fullMessage = "[" + ErrorTypeToString(type) + "] " + error;
    Logger::Error(fullMessage);
}

std::string ErrorHandler::ErrorTypeToString(ErrorType type)
{
    switch (type)
    {
        case ErrorType::AudioDevice:        return "AudioDevice";
        case ErrorType::SpatialAudio:       return "SpatialAudio";
        case ErrorType::Rendering:          return "Rendering";
        case ErrorType::WindowManagement:   return "WindowManagement";
        case ErrorType::Configuration:      return "Configuration";
        case ErrorType::Performance:        return "Performance";
        default:                            return "Unknown";
    }
}

std::string ErrorHandler::AudioErrorToString(AudioErrorType error)
{
    switch (error)
    {
        case AudioErrorType::DeviceNotFound:           return "Audio device not found";
        case AudioErrorType::DeviceDisconnected:      return "Audio device disconnected";
        case AudioErrorType::InitializationFailed:    return "Audio initialization failed";
        case AudioErrorType::CaptureError:            return "Audio capture error";
        case AudioErrorType::SpatialAudioNotSupported: return "Spatial audio not supported";
        default:                                       return "Unknown audio error";
    }
}

std::string ErrorHandler::RenderErrorToString(RenderErrorType error)
{
    switch (error)
    {
        case RenderErrorType::Direct2DInitFailed:      return "Direct2D initialization failed";
        case RenderErrorType::RenderTargetLost:       return "Render target lost";
        case RenderErrorType::ResourceCreationFailed: return "Resource creation failed";
        case RenderErrorType::DrawingError:           return "Drawing error";
        default:                                       return "Unknown render error";
    }
}

std::string ErrorHandler::WindowErrorToString(WindowErrorType error)
{
    switch (error)
    {
        case WindowErrorType::CreationFailed:              return "Window creation failed";
        case WindowErrorType::SetTopMostFailed:           return "Set topmost failed";
        case WindowErrorType::PositionError:              return "Window position error";
        case WindowErrorType::FullscreenDetectionFailed:  return "Fullscreen detection failed";
        default:                                           return "Unknown window error";
    }
}