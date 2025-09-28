#pragma once

#include "../Common/Types.h"
#include "../Common/Config.h"
#include <windows.h>
#include <map>
#include <functional>

// 快捷键ID枚举
enum class HotkeyId
{
    ToggleVisibility = 1000,
    ShowSettings = 1001,
    ExitApplication = 1002,
    ResetPosition = 1003,
    ToggleClickThrough = 1004
};

class HotkeyManager
{
public:
    HotkeyManager();
    ~HotkeyManager();

    // 初始化和清理
    bool Initialize();
    void Shutdown();

    // 快捷键注册
    bool RegisterGlobalHotkey(HotkeyId id, UINT modifiers, UINT vk);
    bool RegisterGlobalHotkey(int hotkeyId, UINT modifiers, UINT vk);
    void UnregisterHotkey(HotkeyId id);
    void UnregisterHotkey(int hotkeyId);
    void UnregisterAllHotkeys();

    // 预定义快捷键
    void SetToggleHotkey(UINT vk, UINT modifiers = 0);
    void SetSettingsHotkey(UINT vk, UINT modifiers);
    void SetExitHotkey(UINT vk, UINT modifiers);

    // 配置管理
    void UpdateConfig(const HotkeyConfig& config);
    void LoadDefaultHotkeys();

    // 回调设置
    void SetHotkeyCallback(HotkeyId id, std::function<void()> callback);
    void SetToggleCallback(std::function<void()> callback);
    void SetSettingsCallback(std::function<void()> callback);
    void SetExitCallback(std::function<void()> callback);

    // 状态查询
    bool IsHotkeyRegistered(HotkeyId id) const;
    std::vector<HotkeyId> GetRegisteredHotkeys() const;

    // 工具方法
    static std::string VirtualKeyToString(UINT vk);
    static std::string ModifiersToString(UINT modifiers);
    static std::string HotkeyToString(UINT modifiers, UINT vk);

private:
    // 消息窗口处理
    bool CreateMessageWindow();
    void DestroyMessageWindow();
    static LRESULT CALLBACK HotkeyWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // 快捷键处理
    void HandleHotkeyPressed(int hotkeyId);
    bool RegisterHotkeyInternal(int id, UINT modifiers, UINT vk);
    void UnregisterHotkeyInternal(int id);

    // 工具方法
    int HotkeyIdToInt(HotkeyId id) const;
    HotkeyId IntToHotkeyId(int id) const;

    // 成员变量
    bool m_initialized;
    HWND m_messageWindow;
    HotkeyConfig m_config;

    // 注册的快捷键
    std::map<int, HotkeyInfo> m_registeredHotkeys;

    // 回调函数
    std::map<HotkeyId, std::function<void()>> m_callbacks;

    // 窗口类名
    static const wchar_t* MESSAGE_WINDOW_CLASS;
    static bool s_classRegistered;
};