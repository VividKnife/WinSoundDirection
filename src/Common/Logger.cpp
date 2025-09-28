#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::unique_ptr<std::ofstream> Logger::s_logFile = nullptr;
std::mutex Logger::s_logMutex;
LogLevel Logger::s_currentLevel = LogLevel::Info;
bool Logger::s_initialized = false;

void Logger::Initialize(const std::string& logFile)
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    
    if (s_initialized)
        return;
    
    s_logFile = std::make_unique<std::ofstream>(logFile, std::ios::app);
    s_initialized = true;
    
    Info("Logger initialized");
}

void Logger::Shutdown()
{
    std::lock_guard<std::mutex> lock(s_logMutex);
    
    if (!s_initialized)
        return;
    
    Info("Logger shutting down");
    
    if (s_logFile && s_logFile->is_open())
    {
        s_logFile->close();
    }
    
    s_logFile.reset();
    s_initialized = false;
}

void Logger::SetLogLevel(LogLevel level)
{
    s_currentLevel = level;
}

void Logger::Debug(const std::string& message)
{
    Log(LogLevel::Debug, message);
}

void Logger::Info(const std::string& message)
{
    Log(LogLevel::Info, message);
}

void Logger::Warning(const std::string& message)
{
    Log(LogLevel::Warning, message);
}

void Logger::Error(const std::string& message)
{
    Log(LogLevel::Error, message);
}

void Logger::Log(LogLevel level, const std::string& message)
{
    if (level < s_currentLevel || !s_initialized)
        return;
    
    std::lock_guard<std::mutex> lock(s_logMutex);
    
    std::string timestamp = GetTimestamp();
    std::string levelStr = LogLevelToString(level);
    std::string logMessage = "[" + timestamp + "] [" + levelStr + "] " + message;
    
    // 输出到控制台
    std::cout << logMessage << std::endl;
    
    // 输出到文件
    if (s_logFile && s_logFile->is_open())
    {
        *s_logFile << logMessage << std::endl;
        s_logFile->flush();
    }
}

std::string Logger::GetTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::LogLevelToString(LogLevel level)
{
    switch (level)
    {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        default:                return "UNKNOWN";
    }
}