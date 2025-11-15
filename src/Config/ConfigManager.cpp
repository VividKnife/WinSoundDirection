#include "Config/ConfigManager.h"

#include <ShlObj.h>

#include <array>
#include <fstream>
#include <stdexcept>

using namespace Config;

namespace
{
constexpr wchar_t kAppFolderName[] = L"SpatialAudioVisualizer";
constexpr wchar_t kConfigFileName[] = L"config.ini";

std::wstring ToHexColor(COLORREF color)
{
    wchar_t buffer[8];
    swprintf_s(buffer, L"#%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
    return buffer;
}

COLORREF FromHexColor(const std::wstring& value, COLORREF fallback)
{
    if (value.size() != 7 || value.front() != L'#')
    {
        return fallback;
    }

    unsigned int r{}, g{}, b{};
    if (swscanf_s(value.c_str() + 1, L"%02x%02x%02x", &r, &g, &b) != 3)
    {
        return fallback;
    }

    return RGB(r, g, b);
}

std::wstring ReadString(const std::filesystem::path& path, const wchar_t* section, const wchar_t* key, const std::wstring& fallback)
{
    wchar_t buffer[256];
    GetPrivateProfileStringW(section, key, fallback.c_str(), buffer, static_cast<DWORD>(std::size(buffer)), path.c_str());
    return buffer;
}

int ReadInt(const std::filesystem::path& path, const wchar_t* section, const wchar_t* key, int fallback)
{
    return GetPrivateProfileIntW(section, key, fallback, path.c_str());
}

double ReadDouble(const std::filesystem::path& path, const wchar_t* section, const wchar_t* key, double fallback)
{
    auto value = ReadString(path, section, key, std::to_wstring(fallback));
    try
    {
        return std::stod(value);
    }
    catch (...)
    {
        return fallback;
    }
}

void WriteString(const std::filesystem::path& path, const wchar_t* section, const wchar_t* key, const std::wstring& value)
{
    WritePrivateProfileStringW(section, key, value.c_str(), path.c_str());
}

void WriteDouble(const std::filesystem::path& path, const wchar_t* section, const wchar_t* key, double value)
{
    WriteString(path, section, key, std::to_wstring(value));
}
}

ConfigManager::ConfigManager() = default;

void ConfigManager::Load()
{
    const auto path = GetConfigPath();
    if (!std::filesystem::exists(path))
    {
        Save();
        return;
    }

    m_theme.primaryColor = FromHexColor(ReadString(path, L"theme", L"primary", ToHexColor(m_theme.primaryColor)), m_theme.primaryColor);
    m_theme.accentColor = FromHexColor(ReadString(path, L"theme", L"accent", ToHexColor(m_theme.accentColor)), m_theme.accentColor);
    m_theme.opacity = static_cast<float>(ReadDouble(path, L"theme", L"opacity", m_theme.opacity));

    m_sensitivity.thresholdDb = static_cast<float>(ReadDouble(path, L"sensitivity", L"thresholdDb", m_sensitivity.thresholdDb));
    m_sensitivity.smoothing = static_cast<float>(ReadDouble(path, L"sensitivity", L"smoothing", m_sensitivity.smoothing));

    m_filter.front = ReadInt(path, L"filter", L"front", m_filter.front ? 1 : 0) != 0;
    m_filter.back = ReadInt(path, L"filter", L"back", m_filter.back ? 1 : 0) != 0;
    m_filter.left = ReadInt(path, L"filter", L"left", m_filter.left ? 1 : 0) != 0;
    m_filter.right = ReadInt(path, L"filter", L"right", m_filter.right ? 1 : 0) != 0;
    m_filter.up = ReadInt(path, L"filter", L"up", m_filter.up ? 1 : 0) != 0;
    m_filter.down = ReadInt(path, L"filter", L"down", m_filter.down ? 1 : 0) != 0;

    m_hotkeys.modifier = static_cast<UINT>(ReadInt(path, L"hotkeys", L"modifier", static_cast<int>(m_hotkeys.modifier)));
    m_hotkeys.key = static_cast<UINT>(ReadInt(path, L"hotkeys", L"key", static_cast<int>(m_hotkeys.key)));

    m_limits.maxCpuPercent = ReadDouble(path, L"limits", L"cpu", m_limits.maxCpuPercent);
    m_limits.maxMemoryMb = static_cast<size_t>(ReadDouble(path, L"limits", L"memory", static_cast<double>(m_limits.maxMemoryMb)));
}

void ConfigManager::Save() const
{
    const auto path = GetConfigPath();
    std::filesystem::create_directories(path.parent_path());

    WriteString(path, L"theme", L"primary", ToHexColor(m_theme.primaryColor));
    WriteString(path, L"theme", L"accent", ToHexColor(m_theme.accentColor));
    WriteDouble(path, L"theme", L"opacity", m_theme.opacity);

    WriteDouble(path, L"sensitivity", L"thresholdDb", m_sensitivity.thresholdDb);
    WriteDouble(path, L"sensitivity", L"smoothing", m_sensitivity.smoothing);

    WriteDouble(path, L"filter", L"front", m_filter.front ? 1 : 0);
    WriteDouble(path, L"filter", L"back", m_filter.back ? 1 : 0);
    WriteDouble(path, L"filter", L"left", m_filter.left ? 1 : 0);
    WriteDouble(path, L"filter", L"right", m_filter.right ? 1 : 0);
    WriteDouble(path, L"filter", L"up", m_filter.up ? 1 : 0);
    WriteDouble(path, L"filter", L"down", m_filter.down ? 1 : 0);

    WriteDouble(path, L"hotkeys", L"modifier", m_hotkeys.modifier);
    WriteDouble(path, L"hotkeys", L"key", m_hotkeys.key);

    WriteDouble(path, L"limits", L"cpu", m_limits.maxCpuPercent);
    WriteDouble(path, L"limits", L"memory", static_cast<double>(m_limits.maxMemoryMb));
}

bool ConfigManager::IsDirectionEnabled(const std::wstring& direction) const
{
    if (direction == L"front") return m_filter.front;
    if (direction == L"back") return m_filter.back;
    if (direction == L"left") return m_filter.left;
    if (direction == L"right") return m_filter.right;
    if (direction == L"up") return m_filter.up;
    if (direction == L"down") return m_filter.down;
    return true;
}

void ConfigManager::SetDirectionEnabled(const std::wstring& direction, bool enabled)
{
    if (direction == L"front") m_filter.front = enabled;
    else if (direction == L"back") m_filter.back = enabled;
    else if (direction == L"left") m_filter.left = enabled;
    else if (direction == L"right") m_filter.right = enabled;
    else if (direction == L"up") m_filter.up = enabled;
    else if (direction == L"down") m_filter.down = enabled;
}

std::filesystem::path ConfigManager::GetConfigPath() const
{
    PWSTR appDataPath{};
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appDataPath)))
    {
        throw std::runtime_error("Unable to locate AppData folder");
    }

    std::filesystem::path path{appDataPath};
    CoTaskMemFree(appDataPath);
    return path / kAppFolderName / kConfigFileName;
}
