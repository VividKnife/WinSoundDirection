#include "HotkeyManager.h"
#include "../Common/Logger.h"
#include "../Common/ErrorHandler.h"
#include "../Common/WindowsCompat.h"

const wchar_t* HotkeyManager::MESSAGE_WINDOW_CLASS = L"SpatialAudioVisualizerHotkey";
bool HotkeyManager::s_classRegistered = false;

HotkeyManager::HotkeyManager()
    : m_initialized(false)
    , m_messageWindow(nullptr)
{
    Logger::Info("HotkeyManager created");
}

HotkeyManager::~HotkeyManager()
{
    Shutdown();
    Logger::Info("HotkeyManager destroyed");
}

bool HotkeyManager::Initialize()
{
    Logger::Info("Initializing HotkeyManager...");

    // 创建消息窗口
    if (!CreateMessageWindow())
    {
        Logger::Error("Failed to create message window for hotkeys");
        return false;
    }

    // 加载默认快捷键
    LoadDefaultHotkeys();

    m_initialized = true;
    Logger::Info("HotkeyManager initialized successfully");
    return true;
}

void HotkeyManager::Shutdown()
{
    if (!m_initialized)
        return;

    Logger::Info("Shutting down HotkeyManager...");

    // 注销所有快捷键
    UnregisterAllHotkeys();

    // 销毁消息窗口
    DestroyMessageWindow();

    m_initialized = false;
    Logger::Info("HotkeyManager shutdown complete");
}

bool HotkeyManager::RegisterGlobalHotkey(HotkeyId id, UINT modifiers, UINT vk)
{
    return RegisterGlobalHotkey(HotkeyIdToInt(id), modifiers, vk);
}

bool HotkeyManager::RegisterGlobalHotkey(int hotkeyId, UINT modifiers, UINT vk)
{
    if (!m_initialized)
        return false;

    // 检查是否已经注册
    if (m_registeredHotkeys.find(hotkeyId) != m_registeredHotkeys.end())
    {
        Logger::Warning("Hotkey ID " + std::to_string(hotkeyId) + " already registered");
        UnregisterHotkeyInternal(hotkeyId);
    }

    // 注册快捷键
    if (RegisterHotkeyInternal(hotkeyId, modifiers, vk))
    {
        // 保存快捷键信息
        HotkeyInfo info;
        info.virtualKey = vk;
        info.modifiers = modifiers;
        m_registeredHotkeys[hotkeyId] = info;

        Logger::Info("Registered hotkey: " + HotkeyToString(modifiers, vk) + 
                    " (ID: " + std::to_string(hotkeyId) + ")");
        return true;
    }

    return false;
}

void HotkeyManager::UnregisterHotkey(HotkeyId id)
{
    UnregisterHotkey(HotkeyIdToInt(id));
}

void HotkeyManager::UnregisterHotkey(int hotkeyId)
{
    auto it = m_registeredHotkeys.find(hotkeyId);
    if (it != m_registeredHotkeys.end())
    {
        UnregisterHotkeyInternal(hotkeyId);
        m_registeredHotkeys.erase(it);
        
        Logger::Info("Unregistered hotkey ID: " + std::to_string(hotkeyId));
    }
}

void HotkeyManager::UnregisterAllHotkeys()
{
    Logger::Debug("Unregistering all hotkeys...");

    for (const auto& pair : m_registeredHotkeys)
    {
        UnregisterHotkeyInternal(pair.first);
    }

    m_registeredHotkeys.clear();
    Logger::Debug("All hotkeys unregistered");
}

void HotkeyManager::SetToggleHotkey(UINT vk, UINT modifiers)
{
    RegisterGlobalHotkey(HotkeyId::ToggleVisibility, modifiers, vk);
}

void HotkeyManager::SetSettingsHotkey(UINT vk, UINT modifiers)
{
    RegisterGlobalHotkey(HotkeyId::ShowSettings, modifiers, vk);
}

void HotkeyManager::SetExitHotkey(UINT vk, UINT modifiers)
{
    RegisterGlobalHotkey(HotkeyId::ExitApplication, modifiers, vk);
}

void HotkeyManager::UpdateConfig(const HotkeyConfig& config)
{
    m_config = config;

    if (m_config.enableGlobalHotkeys)
    {
        // 重新注册切换快捷键
        SetToggleHotkey(m_config.toggleKey, m_config.toggleModifiers);
    }
    else
    {
        // 禁用所有快捷键
        UnregisterAllHotkeys();
    }

    Logger::Debug("Hotkey configuration updated");
}

void HotkeyManager::LoadDefaultHotkeys()
{
    Logger::Debug("Loading default hotkeys...");

    // 默认快捷键：Home键切换显示
    SetToggleHotkey(VK_HOME, 0);

    // 默认快捷键：Ctrl+Shift+S 显示设置
    SetSettingsHotkey('S', MOD_CONTROL | MOD_SHIFT);

    // 默认快捷键：Ctrl+Shift+Q 退出程序
    SetExitHotkey('Q', MOD_CONTROL | MOD_SHIFT);

    Logger::Debug("Default hotkeys loaded");
}

void HotkeyManager::SetHotkeyCallback(HotkeyId id, std::function<void()> callback)
{
    m_callbacks[id] = callback;
}

void HotkeyManager::SetToggleCallback(std::function<void()> callback)
{
    SetHotkeyCallback(HotkeyId::ToggleVisibility, callback);
}

void HotkeyManager::SetSettingsCallback(std::function<void()> callback)
{
    SetHotkeyCallback(HotkeyId::ShowSettings, callback);
}

void HotkeyManager::SetExitCallback(std::function<void()> callback)
{
    SetHotkeyCallback(HotkeyId::ExitApplication, callback);
}

bool HotkeyManager::IsHotkeyRegistered(HotkeyId id) const
{
    return m_registeredHotkeys.find(HotkeyIdToInt(id)) != m_registeredHotkeys.end();
}

std::vector<HotkeyId> HotkeyManager::GetRegisteredHotkeys() const
{
    std::vector<HotkeyId> result;
    
    for (const auto& pair : m_registeredHotkeys)
    {
        HotkeyId id = IntToHotkeyId(pair.first);
        if (id != static_cast<HotkeyId>(0))
        {
            result.push_back(id);
        }
    }
    
    return result;
}

std::string HotkeyManager::VirtualKeyToString(UINT vk)
{
    switch (vk)
    {
        case VK_HOME:       return "Home";
        case VK_END:        return "End";
        case VK_INSERT:     return "Insert";
        case VK_DELETE:     return "Delete";
        case VK_PRIOR:      return "Page Up";
        case VK_NEXT:       return "Page Down";
        case VK_UP:         return "Up Arrow";
        case VK_DOWN:       return "Down Arrow";
        case VK_LEFT:       return "Left Arrow";
        case VK_RIGHT:      return "Right Arrow";
        case VK_F1:         return "F1";
        case VK_F2:         return "F2";
        case VK_F3:         return "F3";
        case VK_F4:         return "F4";
        case VK_F5:         return "F5";
        case VK_F6:         return "F6";
        case VK_F7:         return "F7";
        case VK_F8:         return "F8";
        case VK_F9:         return "F9";
        case VK_F10:        return "F10";
        case VK_F11:        return "F11";
        case VK_F12:        return "F12";
        case VK_ESCAPE:     return "Escape";
        case VK_TAB:        return "Tab";
        case VK_RETURN:     return "Enter";
        case VK_SPACE:      return "Space";
        case VK_BACK:       return "Backspace";
        default:
            if (vk >= 'A' && vk <= 'Z')
            {
                return std::string(1, static_cast<char>(vk));
            }
            else if (vk >= '0' && vk <= '9')
            {
                return std::string(1, static_cast<char>(vk));
            }
            return "Unknown";
    }
}

std::string HotkeyManager::ModifiersToString(UINT modifiers)
{
    std::string result;
    
    if (modifiers & MOD_CONTROL)
        result += "Ctrl+";
    if (modifiers & MOD_ALT)
        result += "Alt+";
    if (modifiers & MOD_SHIFT)
        result += "Shift+";
    if (modifiers & MOD_WIN)
        result += "Win+";
    
    return result;
}

std::string HotkeyManager::HotkeyToString(UINT modifiers, UINT vk)
{
    return ModifiersToString(modifiers) + VirtualKeyToString(vk);
}

bool HotkeyManager::CreateMessageWindow()
{
    // 注册窗口类
    if (!s_classRegistered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = HotkeyWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = MESSAGE_WINDOW_CLASS;

        if (!RegisterClassExW(&wc))
        {
            Logger::Error("Failed to register hotkey window class");
            return false;
        }

        s_classRegistered = true;
    }

    // 创建消息窗口
    m_messageWindow = CreateWindowExW(
        0,
        MESSAGE_WINDOW_CLASS,
        L"Hotkey Message Window",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!m_messageWindow)
    {
        Logger::Error("Failed to create hotkey message window");
        return false;
    }

    Logger::Debug("Hotkey message window created");
    return true;
}

void HotkeyManager::DestroyMessageWindow()
{
    if (m_messageWindow)
    {
        DestroyWindow(m_messageWindow);
        m_messageWindow = nullptr;
        Logger::Debug("Hotkey message window destroyed");
    }

    if (s_classRegistered)
    {
        UnregisterClassW(MESSAGE_WINDOW_CLASS, GetModuleHandle(nullptr));
        s_classRegistered = false;
    }
}

LRESULT CALLBACK HotkeyManager::HotkeyWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HotkeyManager* hotkeyManager = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        hotkeyManager = static_cast<HotkeyManager*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(hotkeyManager));
    }
    else
    {
        hotkeyManager = reinterpret_cast<HotkeyManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (hotkeyManager)
    {
        return hotkeyManager->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT HotkeyManager::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_HOTKEY:
            HandleHotkeyPressed(static_cast<int>(wParam));
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void HotkeyManager::HandleHotkeyPressed(int hotkeyId)
{
    Logger::Debug("Hotkey pressed: ID " + std::to_string(hotkeyId));

    HotkeyId id = IntToHotkeyId(hotkeyId);
    auto it = m_callbacks.find(id);
    
    if (it != m_callbacks.end() && it->second)
    {
        try
        {
            it->second();
        }
        catch (const std::exception& e)
        {
            Logger::Error("Exception in hotkey callback: " + std::string(e.what()));
        }
    }
    else
    {
        Logger::Warning("No callback registered for hotkey ID: " + std::to_string(hotkeyId));
    }
}

bool HotkeyManager::RegisterHotkeyInternal(int id, UINT modifiers, UINT vk)
{
    if (!m_messageWindow)
        return false;

    if (RegisterHotKey(m_messageWindow, id, modifiers, vk))
    {
        return true;
    }
    else
    {
        DWORD error = GetLastError();
        Logger::Warning("Failed to register hotkey " + HotkeyToString(modifiers, vk) + 
                       " (Error: " + std::to_string(error) + ")");
        return false;
    }
}

void HotkeyManager::UnregisterHotkeyInternal(int id)
{
    if (m_messageWindow)
    {
        UnregisterHotKey(m_messageWindow, id);
    }
}

int HotkeyManager::HotkeyIdToInt(HotkeyId id) const
{
    return static_cast<int>(id);
}

HotkeyId HotkeyManager::IntToHotkeyId(int id) const
{
    switch (id)
    {
        case static_cast<int>(HotkeyId::ToggleVisibility):
            return HotkeyId::ToggleVisibility;
        case static_cast<int>(HotkeyId::ShowSettings):
            return HotkeyId::ShowSettings;
        case static_cast<int>(HotkeyId::ExitApplication):
            return HotkeyId::ExitApplication;
        case static_cast<int>(HotkeyId::ResetPosition):
            return HotkeyId::ResetPosition;
        case static_cast<int>(HotkeyId::ToggleClickThrough):
            return HotkeyId::ToggleClickThrough;
        default:
            return static_cast<HotkeyId>(0);
    }
}