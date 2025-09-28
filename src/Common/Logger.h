#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

enum class LogLevel
{
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3
};

class Logger
{
public:
    static void Initialize(const std::string& logFile = "spatial_audio_visualizer.log");
    static void Shutdown();
    
    static void SetLogLevel(LogLevel level);
    static void Debug(const std::string& message);
    static void Info(const std::string& message);
    static void Warning(const std::string& message);
    static void Error(const std::string& message);
    
private:
    static void Log(LogLevel level, const std::string& message);
    static std::string GetTimestamp();
    static std::string LogLevelToString(LogLevel level);
    
    static std::unique_ptr<std::ofstream> s_logFile;
    static std::mutex s_logMutex;
    static LogLevel s_currentLevel;
    static bool s_initialized;
};