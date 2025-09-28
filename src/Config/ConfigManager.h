#pragma once

#include "../Common/Types.h"
#include "../Common/Config.h"
#include <string>
#include <memory>

// JSON库的前向声明（使用nlohmann/json）
namespace nlohmann {
    class json;
}

// 配置存储类型
enum class ConfigStorageType
{
    JsonFile,
    Registry,
    IniFile
};

class ConfigManager
{
public:
    ConfigManager();
    ~ConfigManager();

    // 初始化和清理
    bool Initialize();
    void Shutdown();

    // 配置加载和保存
    ApplicationConfig LoadConfig();
    bool SaveConfig(const ApplicationConfig& config);

    // 配置文件管理
    bool ConfigFileExists() const;
    bool CreateDefaultConfig();
    bool BackupConfig();
    bool RestoreConfig();

    // 配置验证
    bool ValidateConfig(const ApplicationConfig& config) const;
    ApplicationConfig GetDefaultConfig() const;

    // 配置迁移
    bool MigrateConfig(const std::string& fromVersion, const std::string& toVersion);

    // 设置存储类型
    void SetStorageType(ConfigStorageType type);
    ConfigStorageType GetStorageType() const { return m_storageType; }

    // 配置路径管理
    std::string GetConfigFilePath() const;
    std::string GetConfigDirectory() const;
    bool EnsureConfigDirectory();

private:
    // JSON序列化
    nlohmann::json SerializeConfig(const ApplicationConfig& config) const;
    ApplicationConfig DeserializeConfig(const nlohmann::json& json) const;

    // 各个配置部分的序列化
    nlohmann::json SerializeAudioConfig(const AudioConfig& config) const;
    nlohmann::json SerializeVisualConfig(const VisualConfig& config) const;
    nlohmann::json SerializeWindowConfig(const WindowConfig& config) const;
    nlohmann::json SerializeHotkeyConfig(const HotkeyConfig& config) const;
    nlohmann::json SerializePerformanceConfig(const PerformanceConfig& config) const;
    nlohmann::json SerializeVisualTheme(const VisualTheme& theme) const;

    // 各个配置部分的反序列化
    AudioConfig DeserializeAudioConfig(const nlohmann::json& json) const;
    VisualConfig DeserializeVisualConfig(const nlohmann::json& json) const;
    WindowConfig DeserializeWindowConfig(const nlohmann::json& json) const;
    HotkeyConfig DeserializeHotkeyConfig(const nlohmann::json& json) const;
    PerformanceConfig DeserializePerformanceConfig(const nlohmann::json& json) const;
    VisualTheme DeserializeVisualTheme(const nlohmann::json& json) const;

    // 文件操作
    bool LoadJsonFromFile(const std::string& filePath, nlohmann::json& json) const;
    bool SaveJsonToFile(const std::string& filePath, const nlohmann::json& json) const;

    // 注册表操作（Windows）
    bool LoadFromRegistry(ApplicationConfig& config) const;
    bool SaveToRegistry(const ApplicationConfig& config) const;

    // INI文件操作
    bool LoadFromIniFile(ApplicationConfig& config) const;
    bool SaveToIniFile(const ApplicationConfig& config) const;

    // 配置验证辅助方法
    bool ValidateAudioConfig(const AudioConfig& config) const;
    bool ValidateVisualConfig(const VisualConfig& config) const;
    bool ValidateWindowConfig(const WindowConfig& config) const;
    bool ValidateHotkeyConfig(const HotkeyConfig& config) const;
    bool ValidatePerformanceConfig(const PerformanceConfig& config) const;

    // 工具方法
    std::string GetAppDataPath() const;
    std::string GetExecutablePath() const;
    bool CreateDirectoryRecursive(const std::string& path) const;

    // 成员变量
    bool m_initialized;
    ConfigStorageType m_storageType;
    std::string m_configFilePath;
    std::string m_configDirectory;
    ApplicationConfig m_currentConfig;
    
    // 配置常量
    static const std::string CONFIG_FILE_NAME;
    static const std::string CONFIG_DIRECTORY_NAME;
    static const std::string BACKUP_FILE_SUFFIX;
    static const std::string REGISTRY_KEY_PATH;
};