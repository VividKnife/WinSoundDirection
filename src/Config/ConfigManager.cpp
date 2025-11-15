#include "ConfigManager.h"
#include "../Common/Logger.h"
#include "../Common/ErrorHandler.h"
#include "../Common/WindowsCompat.h"
#include <fstream>
#include <filesystem>
#include <cstdlib>

#ifndef MOCK_WINDOWS_APIS
#include <shlobj.h>
#endif

// 简化的JSON实现（避免外部依赖）
#include <map>
#include <vector>
#include <variant>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <stdexcept>

// 简单的JSON类实现
namespace nlohmann {
    class json {
    public:
        using object_type = std::map<std::string, json>;
        using array_type = std::vector<json>;
        using value_type = std::variant<std::nullptr_t, bool, int, float, std::string, object_type, array_type>;

        json() : value(nullptr) {}
        json(bool b) : value(b) {}
        json(int i) : value(i) {}
        json(float f) : value(f) {}
        json(const std::string& s) : value(s) {}
        json(const char* s) : value(std::string(s)) {}

        static json object()
        {
            json j;
            j.value = object_type();
            return j;
        }

        static json array()
        {
            json j;
            j.value = array_type();
            return j;
        }

        json& operator[](const std::string& key)
        {
            if (!std::holds_alternative<object_type>(value))
            {
                value = object_type();
            }
            return std::get<object_type>(value)[key];
        }

        const json& operator[](const std::string& key) const
        {
            static json null_json;
            if (std::holds_alternative<object_type>(value))
            {
                const auto& map = std::get<object_type>(value);
                auto it = map.find(key);
                if (it != map.end())
                    return it->second;
            }
            return null_json;
        }

        void push_back(const json& element)
        {
            if (!std::holds_alternative<array_type>(value))
            {
                value = array_type();
            }
            std::get<array_type>(value).push_back(element);
        }

        bool is_null() const { return std::holds_alternative<std::nullptr_t>(value); }
        bool is_bool() const { return std::holds_alternative<bool>(value); }
        bool is_number() const { return std::holds_alternative<int>(value) || std::holds_alternative<float>(value); }
        bool is_string() const { return std::holds_alternative<std::string>(value); }
        bool is_object() const { return std::holds_alternative<object_type>(value); }
        bool is_array() const { return std::holds_alternative<array_type>(value); }

        bool get_bool() const { return std::holds_alternative<bool>(value) ? std::get<bool>(value) : false; }
        int get_int() const
        {
            if (std::holds_alternative<int>(value))
                return std::get<int>(value);
            if (std::holds_alternative<float>(value))
                return static_cast<int>(std::get<float>(value));
            return 0;
        }
        float get_float() const
        {
            if (std::holds_alternative<float>(value))
                return std::get<float>(value);
            if (std::holds_alternative<int>(value))
                return static_cast<float>(std::get<int>(value));
            return 0.0f;
        }
        std::string get_string() const
        {
            return std::holds_alternative<std::string>(value) ? std::get<std::string>(value) : "";
        }

        const array_type& get_array() const
        {
            static const array_type emptyArray;
            if (std::holds_alternative<array_type>(value))
                return std::get<array_type>(value);
            return emptyArray;
        }

        array_type& get_array()
        {
            if (!std::holds_alternative<array_type>(value))
                value = array_type();
            return std::get<array_type>(value);
        }

        std::string dump(int indent = -1) const
        {
            std::ostringstream oss;
            DumpInternal(oss, indent, 0);
            return oss.str();
        }

        static json parse(const std::string& str)
        {
            size_t pos = 0;
            json result = ParseValue(str, pos);
            SkipWhitespace(str, pos);
            if (pos != str.size())
            {
                throw std::runtime_error("Unexpected characters after JSON value");
            }
            return result;
        }

    private:
        value_type value;

        static void SkipWhitespace(const std::string& str, size_t& pos)
        {
            while (pos < str.size() && std::isspace(static_cast<unsigned char>(str[pos])))
            {
                ++pos;
            }
        }

        static json ParseValue(const std::string& str, size_t& pos)
        {
            SkipWhitespace(str, pos);
            if (pos >= str.size())
            {
                throw std::runtime_error("Unexpected end of JSON input");
            }

            char ch = str[pos];
            if (ch == '{')
                return ParseObject(str, pos);
            if (ch == '[')
                return ParseArray(str, pos);
            if (ch == '"')
                return json(ParseString(str, pos));
            if (ch == 't' || ch == 'f')
                return json(ParseBool(str, pos));
            if (ch == 'n')
                return ParseNull(str, pos);
            if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch)))
                return ParseNumber(str, pos);

            throw std::runtime_error("Invalid character in JSON input");
        }

        static json ParseObject(const std::string& str, size_t& pos)
        {
            json obj = json::object();
            ++pos; // skip '{'
            SkipWhitespace(str, pos);
            if (pos < str.size() && str[pos] == '}')
            {
                ++pos;
                return obj;
            }

            while (pos < str.size())
            {
                SkipWhitespace(str, pos);
                std::string key = ParseString(str, pos);
                SkipWhitespace(str, pos);
                if (pos >= str.size() || str[pos] != ':')
                    throw std::runtime_error("Expected ':' in JSON object");
                ++pos; // skip ':'
                json value = ParseValue(str, pos);
                obj[key] = value;
                SkipWhitespace(str, pos);
                if (pos < str.size() && str[pos] == ',')
                {
                    ++pos;
                    continue;
                }
                if (pos < str.size() && str[pos] == '}')
                {
                    ++pos;
                    break;
                }
                throw std::runtime_error("Expected ',' or '}' in JSON object");
            }

            return obj;
        }

        static json ParseArray(const std::string& str, size_t& pos)
        {
            json arr = json::array();
            ++pos; // skip '['
            SkipWhitespace(str, pos);
            if (pos < str.size() && str[pos] == ']')
            {
                ++pos;
                return arr;
            }

            while (pos < str.size())
            {
                json value = ParseValue(str, pos);
                arr.push_back(value);
                SkipWhitespace(str, pos);
                if (pos < str.size() && str[pos] == ',')
                {
                    ++pos;
                    continue;
                }
                if (pos < str.size() && str[pos] == ']')
                {
                    ++pos;
                    break;
                }
                throw std::runtime_error("Expected ',' or ']' in JSON array");
            }

            return arr;
        }

        static std::string ParseString(const std::string& str, size_t& pos)
        {
            if (str[pos] != '"')
                throw std::runtime_error("Expected '"' to begin JSON string");
            ++pos; // skip opening quote
            std::ostringstream result;
            bool closed = false;

            while (pos < str.size())
            {
                char ch = str[pos++];
                if (ch == '\\')
                {
                    if (pos >= str.size())
                        throw std::runtime_error("Invalid escape sequence in JSON string");
                    char esc = str[pos++];
                    switch (esc)
                    {
                        case '"': result << '"'; break;
                        case '\\': result << '\\'; break;
                        case '/': result << '/'; break;
                        case 'b': result << '\b'; break;
                        case 'f': result << '\f'; break;
                        case 'n': result << '\n'; break;
                        case 'r': result << '\r'; break;
                        case 't': result << '\t'; break;
                        default:
                            throw std::runtime_error("Unsupported escape sequence in JSON string");
                    }
                }
                else if (ch == '"')
                {
                    closed = true;
                    break;
                }
                else
                {
                    result << ch;
                }
            }

            if (!closed)
                throw std::runtime_error("Unterminated JSON string literal");

            return result.str();
        }

        static json ParseNumber(const std::string& str, size_t& pos)
        {
            size_t start = pos;
            if (str[pos] == '-')
                ++pos;
            while (pos < str.size() && std::isdigit(static_cast<unsigned char>(str[pos])))
                ++pos;
            bool isFloat = false;
            if (pos < str.size() && str[pos] == '.')
            {
                isFloat = true;
                ++pos;
                while (pos < str.size() && std::isdigit(static_cast<unsigned char>(str[pos])))
                    ++pos;
            }
            if (pos < str.size() && (str[pos] == 'e' || str[pos] == 'E'))
            {
                isFloat = true;
                ++pos;
                if (pos < str.size() && (str[pos] == '+' || str[pos] == '-'))
                    ++pos;
                while (pos < str.size() && std::isdigit(static_cast<unsigned char>(str[pos])))
                    ++pos;
            }
            std::string numberStr = str.substr(start, pos - start);
            if (isFloat)
            {
                return json(std::stof(numberStr));
            }
            return json(std::stoi(numberStr));
        }

        static bool ParseBool(const std::string& str, size_t& pos)
        {
            if (str.compare(pos, 4, "true") == 0)
            {
                pos += 4;
                return true;
            }
            if (str.compare(pos, 5, "false") == 0)
            {
                pos += 5;
                return false;
            }
            throw std::runtime_error("Invalid boolean literal in JSON");
        }

        static json ParseNull(const std::string& str, size_t& pos)
        {
            if (str.compare(pos, 4, "null") != 0)
                throw std::runtime_error("Invalid null literal in JSON");
            pos += 4;
            return json();
        }

        void DumpInternal(std::ostringstream& oss, int indent, int depth) const
        {
            auto writeIndent = [&](int level) {
                if (indent >= 0)
                {
                    oss << std::string(level * indent, ' ');
                }
            };

            if (std::holds_alternative<std::nullptr_t>(value))
            {
                oss << "null";
            }
            else if (std::holds_alternative<bool>(value))
            {
                oss << (std::get<bool>(value) ? "true" : "false");
            }
            else if (std::holds_alternative<int>(value))
            {
                oss << std::get<int>(value);
            }
            else if (std::holds_alternative<float>(value))
            {
                oss << std::get<float>(value);
            }
            else if (std::holds_alternative<std::string>(value))
            {
                oss << '"' << std::get<std::string>(value) << '"';
            }
            else if (std::holds_alternative<object_type>(value))
            {
                oss << '{';
                if (indent >= 0)
                    oss << '\n';
                bool first = true;
                for (const auto& pair : std::get<object_type>(value))
                {
                    if (!first)
                    {
                        oss << ',';
                        if (indent >= 0)
                            oss << '\n';
                    }
                    first = false;
                    writeIndent(depth + 1);
                    oss << '"' << pair.first << '"' << ':';
                    if (indent >= 0)
                        oss << ' ';
                    pair.second.DumpInternal(oss, indent, depth + 1);
                }
                if (indent >= 0)
                {
                    oss << '\n';
                    writeIndent(depth);
                }
                oss << '}';
            }
            else if (std::holds_alternative<array_type>(value))
            {
                oss << '[';
                if (indent >= 0)
                    oss << '\n';
                const auto& arr = std::get<array_type>(value);
                for (size_t i = 0; i < arr.size(); ++i)
                {
                    if (i > 0)
                    {
                        oss << ',';
                        if (indent >= 0)
                            oss << '\n';
                    }
                    writeIndent(depth + 1);
                    arr[i].DumpInternal(oss, indent, depth + 1);
                }
                if (indent >= 0)
                {
                    oss << '\n';
                    writeIndent(depth);
                }
                oss << ']';
            }
        }
    };
}

const std::string ConfigManager::CONFIG_FILE_NAME = "config.json";
const std::string ConfigManager::CONFIG_DIRECTORY_NAME = "SpatialAudioVisualizer";
const std::string ConfigManager::BACKUP_FILE_SUFFIX = ".backup";
const std::string ConfigManager::REGISTRY_KEY_PATH = "SOFTWARE\\SpatialAudioVisualizer";

namespace
{
    std::string ToLowerCopy(const std::string& value)
    {
        std::string result = value;
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return result;
    }

    std::string DirectionToString(CardinalDirection direction)
    {
        switch (direction)
        {
            case CardinalDirection::Front: return "front";
            case CardinalDirection::Back: return "back";
            case CardinalDirection::Left: return "left";
            case CardinalDirection::Right: return "right";
            case CardinalDirection::Up: return "up";
            case CardinalDirection::Down: return "down";
            case CardinalDirection::FrontLeft: return "front-left";
            case CardinalDirection::FrontRight: return "front-right";
            case CardinalDirection::BackLeft: return "back-left";
            case CardinalDirection::BackRight: return "back-right";
            default: return "none";
        }
    }

    CardinalDirection DirectionFromString(const std::string& value)
    {
        std::string normalized = ToLowerCopy(value);
        if (normalized == "front") return CardinalDirection::Front;
        if (normalized == "back") return CardinalDirection::Back;
        if (normalized == "left") return CardinalDirection::Left;
        if (normalized == "right") return CardinalDirection::Right;
        if (normalized == "up") return CardinalDirection::Up;
        if (normalized == "down") return CardinalDirection::Down;
        if (normalized == "front-left" || normalized == "front_left") return CardinalDirection::FrontLeft;
        if (normalized == "front-right" || normalized == "front_right") return CardinalDirection::FrontRight;
        if (normalized == "back-left" || normalized == "back_left") return CardinalDirection::BackLeft;
        if (normalized == "back-right" || normalized == "back_right") return CardinalDirection::BackRight;
        return CardinalDirection::None;
    }

    nlohmann::json SerializeColor(const D2D1_COLOR_F& color)
    {
        nlohmann::json jsonColor = nlohmann::json::object();
        jsonColor["r"] = color.r;
        jsonColor["g"] = color.g;
        jsonColor["b"] = color.b;
        jsonColor["a"] = color.a;
        return jsonColor;
    }

    D2D1_COLOR_F DeserializeColor(const nlohmann::json& jsonColor, const D2D1_COLOR_F& fallback)
    {
        D2D1_COLOR_F color = fallback;
        if (!jsonColor.is_object())
            return color;

        if (!jsonColor["r"].is_null()) color.r = jsonColor["r"].get_float();
        if (!jsonColor["g"].is_null()) color.g = jsonColor["g"].get_float();
        if (!jsonColor["b"].is_null()) color.b = jsonColor["b"].get_float();
        if (!jsonColor["a"].is_null()) color.a = jsonColor["a"].get_float();

        return color;
    }
}

ConfigManager::ConfigManager()
    : m_initialized(false)
    , m_storageType(ConfigStorageType::JsonFile)
{
    Logger::Info("ConfigManager created");
}

ConfigManager::~ConfigManager()
{
    Shutdown();
    Logger::Info("ConfigManager destroyed");
}

bool ConfigManager::Initialize()
{
    Logger::Info("Initializing ConfigManager...");

    // 确定配置目录
    std::filesystem::path basePath = std::filesystem::path(GetAppDataPath());
    m_configDirectory = (basePath / CONFIG_DIRECTORY_NAME).string();
    m_configFilePath = (std::filesystem::path(m_configDirectory) / CONFIG_FILE_NAME).string();

    // 确保配置目录存在
    if (!EnsureConfigDirectory())
    {
        Logger::Error("Failed to create config directory");
        return false;
    }

    // 如果配置文件不存在，创建默认配置
    if (!ConfigFileExists())
    {
        Logger::Info("Config file not found, creating default configuration");
        if (!CreateDefaultConfig())
        {
            Logger::Warning("Failed to create default config file");
        }
    }

    m_initialized = true;
    Logger::Info("ConfigManager initialized successfully");
    return true;
}

void ConfigManager::Shutdown()
{
    if (!m_initialized)
        return;

    Logger::Info("Shutting down ConfigManager...");
    
    // 保存当前配置
    SaveConfig(m_currentConfig);
    
    m_initialized = false;
    Logger::Info("ConfigManager shutdown complete");
}

ApplicationConfig ConfigManager::LoadConfig()
{
    Logger::Debug("Loading configuration...");

    ApplicationConfig config = GetDefaultConfig();

    try
    {
        switch (m_storageType)
        {
            case ConfigStorageType::JsonFile:
            {
                nlohmann::json json;
                if (LoadJsonFromFile(m_configFilePath, json))
                {
                    config = DeserializeConfig(json);
                    Logger::Info("Configuration loaded from JSON file");
                }
                else
                {
                    Logger::Warning("Failed to load config from JSON file, using defaults");
                }
                break;
            }
            
            case ConfigStorageType::Registry:
                if (LoadFromRegistry(config))
                {
                    Logger::Info("Configuration loaded from registry");
                }
                else
                {
                    Logger::Warning("Failed to load config from registry, using defaults");
                }
                break;
                
            case ConfigStorageType::IniFile:
                if (LoadFromIniFile(config))
                {
                    Logger::Info("Configuration loaded from INI file");
                }
                else
                {
                    Logger::Warning("Failed to load config from INI file, using defaults");
                }
                break;
        }

        // 验证配置
        if (!ValidateConfig(config))
        {
            Logger::Warning("Loaded configuration is invalid, using defaults");
            config = GetDefaultConfig();
        }
    }
    catch (const std::exception& e)
    {
        Logger::Error("Exception while loading config: " + std::string(e.what()));
        config = GetDefaultConfig();
    }

    m_currentConfig = config;
    return config;
}

bool ConfigManager::SaveConfig(const ApplicationConfig& config)
{
    Logger::Debug("Saving configuration...");

    if (!ValidateConfig(config))
    {
        Logger::Error("Cannot save invalid configuration");
        return false;
    }

    try
    {
        // 备份现有配置
        BackupConfig();

        bool success = false;
        
        switch (m_storageType)
        {
            case ConfigStorageType::JsonFile:
            {
                nlohmann::json json = SerializeConfig(config);
                success = SaveJsonToFile(m_configFilePath, json);
                break;
            }
            
            case ConfigStorageType::Registry:
                success = SaveToRegistry(config);
                break;
                
            case ConfigStorageType::IniFile:
                success = SaveToIniFile(config);
                break;
        }

        if (success)
        {
            m_currentConfig = config;
            Logger::Info("Configuration saved successfully");
        }
        else
        {
            Logger::Error("Failed to save configuration");
        }

        return success;
    }
    catch (const std::exception& e)
    {
        Logger::Error("Exception while saving config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::ConfigFileExists() const
{
    return std::filesystem::exists(m_configFilePath);
}

bool ConfigManager::CreateDefaultConfig()
{
    ApplicationConfig defaultConfig = GetDefaultConfig();
    return SaveConfig(defaultConfig);
}

bool ConfigManager::BackupConfig()
{
    if (!ConfigFileExists())
        return true;

    try
    {
        std::string backupPath = m_configFilePath + BACKUP_FILE_SUFFIX;
        std::filesystem::copy_file(m_configFilePath, backupPath, 
                                 std::filesystem::copy_options::overwrite_existing);
        
        Logger::Debug("Configuration backed up to: " + backupPath);
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::Warning("Failed to backup config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::RestoreConfig()
{
    std::string backupPath = m_configFilePath + BACKUP_FILE_SUFFIX;
    
    if (!std::filesystem::exists(backupPath))
    {
        Logger::Warning("No backup config file found");
        return false;
    }

    try
    {
        std::filesystem::copy_file(backupPath, m_configFilePath,
                                 std::filesystem::copy_options::overwrite_existing);
        
        Logger::Info("Configuration restored from backup");
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::Error("Failed to restore config: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::ValidateConfig(const ApplicationConfig& config) const
{
    return ValidateAudioConfig(config.audio) &&
           ValidateVisualConfig(config.visual) &&
           ValidateWindowConfig(config.window) &&
           ValidateHotkeyConfig(config.hotkey) &&
           ValidatePerformanceConfig(config.performance);
}

ApplicationConfig ConfigManager::GetDefaultConfig() const
{
    ApplicationConfig config;
    
    // 音频配置默认值
    config.audio.sensitivity = 0.5f;
    config.audio.noiseThreshold = 0.1f;
    config.audio.enableDirectionFiltering = true;
    config.audio.updateFrequency = 60;
    
    // 视觉配置默认值
    config.visual.transparency = 0.8f;
    config.visual.indicatorSize = 50;
    config.visual.showCompass = true;
    config.visual.showIntensityMeter = true;
    config.visual.animation = AnimationStyle::Smooth;
    
    // 窗口配置默认值
    config.window.position = Point(100, 100);
    config.window.size = Size(200, 200);
    config.window.alwaysOnTop = true;
    config.window.clickThrough = false;
    config.window.hideInFullscreen = false;
    config.window.startMinimized = false;
    
    // 快捷键配置默认值
    config.hotkey.toggleKey = VK_HOME;
    config.hotkey.toggleModifiers = 0;
    config.hotkey.enableGlobalHotkeys = true;
    config.hotkey.showTrayIcon = true;
    
    // 性能配置默认值
    config.performance.maxCpuUsage = 5;
    config.performance.maxMemoryUsage = 50;
    config.performance.enablePerformanceMonitoring = true;
    config.performance.adaptiveQuality = true;
    
    config.configVersion = "1.0";
    
    return config;
}

void ConfigManager::SetStorageType(ConfigStorageType type)
{
    m_storageType = type;
    Logger::Debug("Config storage type changed to: " + std::to_string(static_cast<int>(type)));
}

std::string ConfigManager::GetConfigFilePath() const
{
    return m_configFilePath;
}

std::string ConfigManager::GetConfigDirectory() const
{
    return m_configDirectory;
}

bool ConfigManager::EnsureConfigDirectory()
{
    try
    {
        if (!std::filesystem::exists(m_configDirectory))
        {
            std::filesystem::create_directories(m_configDirectory);
            Logger::Debug("Created config directory: " + m_configDirectory);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::Error("Failed to create config directory: " + std::string(e.what()));
        return false;
    }
}

nlohmann::json ConfigManager::SerializeConfig(const ApplicationConfig& config) const
{
    nlohmann::json json;
    
    json["version"] = config.configVersion;
    json["audio"] = SerializeAudioConfig(config.audio);
    json["visual"] = SerializeVisualConfig(config.visual);
    json["window"] = SerializeWindowConfig(config.window);
    json["hotkey"] = SerializeHotkeyConfig(config.hotkey);
    json["performance"] = SerializePerformanceConfig(config.performance);
    
    return json;
}

ApplicationConfig ConfigManager::DeserializeConfig(const nlohmann::json& json) const
{
    ApplicationConfig config = GetDefaultConfig();
    
    if (!json["version"].is_null())
        config.configVersion = json["version"].get_string();
    
    if (!json["audio"].is_null())
        config.audio = DeserializeAudioConfig(json["audio"]);
    
    if (!json["visual"].is_null())
        config.visual = DeserializeVisualConfig(json["visual"]);
    
    if (!json["window"].is_null())
        config.window = DeserializeWindowConfig(json["window"]);
    
    if (!json["hotkey"].is_null())
        config.hotkey = DeserializeHotkeyConfig(json["hotkey"]);
    
    if (!json["performance"].is_null())
        config.performance = DeserializePerformanceConfig(json["performance"]);
    
    return config;
}

nlohmann::json ConfigManager::SerializeAudioConfig(const AudioConfig& config) const
{
    nlohmann::json json;
    
    json["sensitivity"] = config.sensitivity;
    json["noiseThreshold"] = config.noiseThreshold;
    json["enableDirectionFiltering"] = config.enableDirectionFiltering;
    json["updateFrequency"] = config.updateFrequency;
    nlohmann::json directions = nlohmann::json::array();
    for (CardinalDirection direction : config.enabledDirections)
    {
        directions.push_back(DirectionToString(direction));
    }
    json["enabledDirections"] = directions;

    return json;
}

AudioConfig ConfigManager::DeserializeAudioConfig(const nlohmann::json& json) const
{
    AudioConfig config;
    
    if (!json["sensitivity"].is_null())
        config.sensitivity = json["sensitivity"].get_float();
    
    if (!json["noiseThreshold"].is_null())
        config.noiseThreshold = json["noiseThreshold"].get_float();
    
    if (!json["enableDirectionFiltering"].is_null())
        config.enableDirectionFiltering = json["enableDirectionFiltering"].get_bool();

    if (!json["updateFrequency"].is_null())
        config.updateFrequency = json["updateFrequency"].get_int();

    if (json["enabledDirections"].is_array())
    {
        config.enabledDirections.clear();
        for (const auto& value : json["enabledDirections"].get_array())
        {
            CardinalDirection direction = DirectionFromString(value.get_string());
            if (direction != CardinalDirection::None)
            {
                config.enabledDirections.insert(direction);
            }
        }

        if (config.enabledDirections.empty())
        {
            // 避免配置导致所有方向被禁用
            config.enabledDirections = AudioConfig().enabledDirections;
        }
    }

    return config;
}

bool ConfigManager::LoadJsonFromFile(const std::string& filePath, nlohmann::json& json) const
{
    try
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        json = nlohmann::json::parse(content);
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::Error("Failed to load JSON from file: " + std::string(e.what()));
        return false;
    }
}

bool ConfigManager::SaveJsonToFile(const std::string& filePath, const nlohmann::json& json) const
{
    try
    {
        std::ofstream file(filePath);
        if (!file.is_open())
            return false;
        
        file << json.dump(2);
        return true;
    }
    catch (const std::exception& e)
    {
        Logger::Error("Failed to save JSON to file: " + std::string(e.what()));
        return false;
    }
}

std::string ConfigManager::GetAppDataPath() const
{
#ifndef MOCK_WINDOWS_APIS
    wchar_t* path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path)))
    {
        std::wstring wpath(path);
        CoTaskMemFree(path);

        // 转换为多字节字符串
        int size = WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string result(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wpath.c_str(), -1, &result[0], size, nullptr, nullptr);

        return result;
    }

    return "."; // 回退到当前目录
#else
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig != nullptr && *xdgConfig != '\0')
    {
        return std::string(xdgConfig);
    }

    const char* home = std::getenv("HOME");
    if (home != nullptr && *home != '\0')
    {
        return (std::filesystem::path(home) / ".config").string();
    }

    return ".";
#endif
}

bool ConfigManager::ValidateAudioConfig(const AudioConfig& config) const
{
    return config.sensitivity >= 0.0f && config.sensitivity <= 1.0f &&
           config.noiseThreshold >= 0.0f && config.noiseThreshold <= 1.0f &&
           config.updateFrequency > 0 && config.updateFrequency <= 120 &&
           !config.enabledDirections.empty();
}

bool ConfigManager::ValidateVisualConfig(const VisualConfig& config) const
{
    return config.transparency >= 0.0f && config.transparency <= 1.0f &&
           config.indicatorSize > 0 && config.indicatorSize <= 200;
}

bool ConfigManager::ValidateWindowConfig(const WindowConfig& config) const
{
    return config.size.width > 0 && config.size.height > 0 &&
           config.size.width <= 2000 && config.size.height <= 2000;
}

bool ConfigManager::ValidateHotkeyConfig(const HotkeyConfig& config) const
{
    return true; // 快捷键配置基本都是有效的
}

bool ConfigManager::ValidatePerformanceConfig(const PerformanceConfig& config) const
{
    return config.maxCpuUsage > 0 && config.maxCpuUsage <= 100 &&
           config.maxMemoryUsage > 0 && config.maxMemoryUsage <= 1000;
}

// 简化实现其他序列化方法
nlohmann::json ConfigManager::SerializeVisualConfig(const VisualConfig& config) const
{
    nlohmann::json json;
    json["transparency"] = config.transparency;
    json["indicatorSize"] = config.indicatorSize;
    json["showCompass"] = config.showCompass;
    json["showIntensityMeter"] = config.showIntensityMeter;
    json["theme"] = SerializeVisualTheme(config.theme);
    return json;
}

VisualConfig ConfigManager::DeserializeVisualConfig(const nlohmann::json& json) const
{
    VisualConfig config;
    if (!json["transparency"].is_null()) config.transparency = json["transparency"].get_float();
    if (!json["indicatorSize"].is_null()) config.indicatorSize = json["indicatorSize"].get_int();
    if (!json["showCompass"].is_null()) config.showCompass = json["showCompass"].get_bool();
    if (!json["showIntensityMeter"].is_null()) config.showIntensityMeter = json["showIntensityMeter"].get_bool();
    if (!json["theme"].is_null()) config.theme = DeserializeVisualTheme(json["theme"]);
    return config;
}

nlohmann::json ConfigManager::SerializeWindowConfig(const WindowConfig& config) const
{
    nlohmann::json json;
    json["position_x"] = config.position.x;
    json["position_y"] = config.position.y;
    json["size_width"] = config.size.width;
    json["size_height"] = config.size.height;
    json["alwaysOnTop"] = config.alwaysOnTop;
    json["clickThrough"] = config.clickThrough;
    return json;
}

WindowConfig ConfigManager::DeserializeWindowConfig(const nlohmann::json& json) const
{
    WindowConfig config;
    if (!json["position_x"].is_null()) config.position.x = json["position_x"].get_int();
    if (!json["position_y"].is_null()) config.position.y = json["position_y"].get_int();
    if (!json["size_width"].is_null()) config.size.width = json["size_width"].get_int();
    if (!json["size_height"].is_null()) config.size.height = json["size_height"].get_int();
    if (!json["alwaysOnTop"].is_null()) config.alwaysOnTop = json["alwaysOnTop"].get_bool();
    if (!json["clickThrough"].is_null()) config.clickThrough = json["clickThrough"].get_bool();
    return config;
}

nlohmann::json ConfigManager::SerializeHotkeyConfig(const HotkeyConfig& config) const
{
    nlohmann::json json;
    json["toggleKey"] = static_cast<int>(config.toggleKey);
    json["toggleModifiers"] = static_cast<int>(config.toggleModifiers);
    json["enableGlobalHotkeys"] = config.enableGlobalHotkeys;
    json["showTrayIcon"] = config.showTrayIcon;
    return json;
}

HotkeyConfig ConfigManager::DeserializeHotkeyConfig(const nlohmann::json& json) const
{
    HotkeyConfig config;
    if (!json["toggleKey"].is_null()) config.toggleKey = static_cast<UINT>(json["toggleKey"].get_int());
    if (!json["toggleModifiers"].is_null()) config.toggleModifiers = static_cast<UINT>(json["toggleModifiers"].get_int());
    if (!json["enableGlobalHotkeys"].is_null()) config.enableGlobalHotkeys = json["enableGlobalHotkeys"].get_bool();
    if (!json["showTrayIcon"].is_null()) config.showTrayIcon = json["showTrayIcon"].get_bool();
    return config;
}

nlohmann::json ConfigManager::SerializePerformanceConfig(const PerformanceConfig& config) const
{
    nlohmann::json json;
    json["maxCpuUsage"] = config.maxCpuUsage;
    json["maxMemoryUsage"] = config.maxMemoryUsage;
    json["enablePerformanceMonitoring"] = config.enablePerformanceMonitoring;
    json["adaptiveQuality"] = config.adaptiveQuality;
    return json;
}

PerformanceConfig ConfigManager::DeserializePerformanceConfig(const nlohmann::json& json) const
{
    PerformanceConfig config;
    if (!json["maxCpuUsage"].is_null()) config.maxCpuUsage = json["maxCpuUsage"].get_int();
    if (!json["maxMemoryUsage"].is_null()) config.maxMemoryUsage = json["maxMemoryUsage"].get_int();
    if (!json["enablePerformanceMonitoring"].is_null()) config.enablePerformanceMonitoring = json["enablePerformanceMonitoring"].get_bool();
    if (!json["adaptiveQuality"].is_null()) config.adaptiveQuality = json["adaptiveQuality"].get_bool();
    return config;
}

nlohmann::json ConfigManager::SerializeVisualTheme(const VisualTheme& theme) const
{
    nlohmann::json json = nlohmann::json::object();
    json["primaryColor"] = SerializeColor(theme.primaryColor);
    json["secondaryColor"] = SerializeColor(theme.secondaryColor);
    json["backgroundColor"] = SerializeColor(theme.backgroundColor);
    json["indicatorSize"] = theme.indicatorSize;
    json["style"] = static_cast<int>(theme.style);
    return json;
}

VisualTheme ConfigManager::DeserializeVisualTheme(const nlohmann::json& json) const
{
    VisualTheme theme;
    theme.primaryColor = DeserializeColor(json["primaryColor"], theme.primaryColor);
    theme.secondaryColor = DeserializeColor(json["secondaryColor"], theme.secondaryColor);
    theme.backgroundColor = DeserializeColor(json["backgroundColor"], theme.backgroundColor);
    if (!json["indicatorSize"].is_null())
        theme.indicatorSize = json["indicatorSize"].get_float();
    if (!json["style"].is_null())
    {
        int styleValue = json["style"].get_int();
        if (styleValue >= static_cast<int>(IndicatorStyle::Circle) &&
            styleValue <= static_cast<int>(IndicatorStyle::Ring))
        {
            theme.style = static_cast<IndicatorStyle>(styleValue);
        }
    }
    return theme;
}

bool ConfigManager::LoadFromRegistry(ApplicationConfig& config) const
{
    // Windows注册表实现（简化）
    return false;
}

bool ConfigManager::SaveToRegistry(const ApplicationConfig& config) const
{
    // Windows注册表实现（简化）
    return false;
}

bool ConfigManager::LoadFromIniFile(ApplicationConfig& config) const
{
    // INI文件实现（简化）
    return false;
}

bool ConfigManager::SaveToIniFile(const ApplicationConfig& config) const
{
    // INI文件实现（简化）
    return false;
}

bool ConfigManager::MigrateConfig(const std::string& fromVersion, const std::string& toVersion)
{
    Logger::Info("Migrating config from version " + fromVersion + " to " + toVersion);
    // 配置迁移逻辑（简化）
    return true;
}