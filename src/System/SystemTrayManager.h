#pragma once

#include "../Common/Types.h"
#include "../Common/Config.h"
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <functional>
#include <map>

// 托盘菜单项ID
enum class TrayMenuId
{
    Show = 2000,
    Hide = 2001,
    Settings = 2002,
    About = 2003,
    Exit = 2004,
    Separator1 = 2005,
    Separator2 = 2006
};

// 托盘事件类型
enum class TrayEvent
{
    LeftClick,
    RightClick,
    DoubleClick,
    MiddleClick
};

class SystemTrayManager
{
public:
    SystemTrayManager();
    ~SystemTrayManager();

    // 初始化和清理
    bool Initialize();
    void Shutdown();

    // 托盘图标管理
    bool CreateTrayIcon();
    void UpdateTrayIcon(const std::wstring& tooltip);
    void ShowTrayIcon();
    void HideTrayIcon();
    void RemoveTrayIcon();

    // 菜单管理
    void SetTrayMenu(HMENU menu);
    void ShowContextMenu();
    void UpdateMenuItems(bool isVisible);

    // 事件回调
    void SetTrayEventCallback(std::function<void(TrayEvent)> callback);
    void SetMenuCallback(TrayMenuId menuId, std::function<void()> callback);

    // 便捷回调设置
    void SetShowCallback(std::function<void()> callback);
    void SetHideCallback(std::function<void()> callback);
    void SetSettingsCallback(std::function<void()> callback);
    void SetExitCallback(std::function<void()> callback);

    // 状态查询
    bool IsTrayIconVisible() const { return m_iconVisible; }

    // 通知功能
    void ShowBalloonTip(const std::wstring& title, const std::wstring& message, DWORD timeout = 3000);

private:
    // 消息窗口处理
    bool CreateMessageWindow();
    void DestroyMessageWindow();
    static LRESULT CALLBACK TrayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // 托盘消息处理
    void HandleTrayIconClick(UINT msg);
    void HandleMenuCommand(WPARAM wParam);

    // 菜单创建
    HMENU CreateDefaultMenu();
    void AddMenuItem(HMENU menu, TrayMenuId id, const std::wstring& text, bool enabled = true);
    void AddMenuSeparator(HMENU menu);

    // 图标资源
    HICON LoadTrayIcon();
    HICON CreateDefaultIcon();

    // 工具方法
    POINT GetCursorPosition();
    void SetForegroundWindowFix();

    // 成员变量
    bool m_initialized;
    bool m_iconVisible;
    HWND m_messageWindow;
    NOTIFYICONDATA m_nid;
    HMENU m_contextMenu;
    HICON m_trayIcon;

    // 回调函数
    std::function<void(TrayEvent)> m_eventCallback;
    std::map<TrayMenuId, std::function<void()>> m_menuCallbacks;

    // 窗口类名和消息ID
    static const wchar_t* MESSAGE_WINDOW_CLASS;
    static const UINT WM_TRAYICON;
    static const UINT TRAY_ICON_ID;
    static bool s_classRegistered;
};