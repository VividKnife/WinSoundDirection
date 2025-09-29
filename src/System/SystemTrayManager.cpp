#include "SystemTrayManager.h"
#include "../Common/Logger.h"
#include "../Common/ErrorHandler.h"
#include "../Common/WindowsCompat.h"
#include <commctrl.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

const wchar_t* SystemTrayManager::MESSAGE_WINDOW_CLASS = L"SpatialAudioVisualizerTray";
const UINT SystemTrayManager::WM_TRAYICON = WM_USER + 1;
const UINT SystemTrayManager::TRAY_ICON_ID = 1;
bool SystemTrayManager::s_classRegistered = false;

SystemTrayManager::SystemTrayManager()
    : m_initialized(false)
    , m_iconVisible(false)
    , m_messageWindow(nullptr)
    , m_contextMenu(nullptr)
    , m_trayIcon(nullptr)
{
    memset(&m_nid, 0, sizeof(NOTIFYICONDATA));
    Logger::Info("SystemTrayManager created");
}

SystemTrayManager::~SystemTrayManager()
{
    Shutdown();
    Logger::Info("SystemTrayManager destroyed");
}

bool SystemTrayManager::Initialize()
{
    Logger::Info("Initializing SystemTrayManager...");

    // 创建消息窗口
    if (!CreateMessageWindow())
    {
        Logger::Error("Failed to create tray message window");
        return false;
    }

    // 加载托盘图标
    m_trayIcon = LoadTrayIcon();
    if (!m_trayIcon)
    {
        Logger::Warning("Failed to load tray icon, using default");
        m_trayIcon = CreateDefaultIcon();
    }

    // 创建托盘图标
    if (!CreateTrayIcon())
    {
        Logger::Error("Failed to create tray icon");
        return false;
    }

    // 创建默认菜单
    m_contextMenu = CreateDefaultMenu();

    m_initialized = true;
    Logger::Info("SystemTrayManager initialized successfully");
    return true;
}

void SystemTrayManager::Shutdown()
{
    if (!m_initialized)
        return;

    Logger::Info("Shutting down SystemTrayManager...");

    // 移除托盘图标
    RemoveTrayIcon();

    // 清理菜单
    if (m_contextMenu)
    {
        DestroyMenu(m_contextMenu);
        m_contextMenu = nullptr;
    }

    // 清理图标
    if (m_trayIcon)
    {
        DestroyIcon(m_trayIcon);
        m_trayIcon = nullptr;
    }

    // 销毁消息窗口
    DestroyMessageWindow();

    m_initialized = false;
    Logger::Info("SystemTrayManager shutdown complete");
}

bool SystemTrayManager::CreateTrayIcon()
{
    Logger::Debug("Creating tray icon...");

    // 初始化NOTIFYICONDATA结构
    m_nid.cbSize = sizeof(NOTIFYICONDATA);
    m_nid.hWnd = m_messageWindow;
    m_nid.uID = TRAY_ICON_ID;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = m_trayIcon;
    wcscpy_s(m_nid.szTip, L"Spatial Audio Visualizer");

    // 添加托盘图标
    if (Shell_NotifyIcon(NIM_ADD, &m_nid))
    {
        m_iconVisible = true;
        Logger::Debug("Tray icon created successfully");
        return true;
    }
    else
    {
        Logger::Error("Failed to add tray icon");
        return false;
    }
}

void SystemTrayManager::UpdateTrayIcon(const std::wstring& tooltip)
{
    if (!m_iconVisible)
        return;

    wcscpy_s(m_nid.szTip, tooltip.c_str());
    m_nid.uFlags = NIF_TIP;

    Shell_NotifyIcon(NIM_MODIFY, &m_nid);
    Logger::Debug("Tray icon tooltip updated");
}

void SystemTrayManager::ShowTrayIcon()
{
    if (!m_iconVisible && m_initialized)
    {
        if (Shell_NotifyIcon(NIM_ADD, &m_nid))
        {
            m_iconVisible = true;
            Logger::Debug("Tray icon shown");
        }
    }
}

void SystemTrayManager::HideTrayIcon()
{
    if (m_iconVisible)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_nid);
        m_iconVisible = false;
        Logger::Debug("Tray icon hidden");
    }
}

void SystemTrayManager::RemoveTrayIcon()
{
    if (m_iconVisible)
    {
        Shell_NotifyIcon(NIM_DELETE, &m_nid);
        m_iconVisible = false;
        Logger::Debug("Tray icon removed");
    }
}

void SystemTrayManager::SetTrayMenu(HMENU menu)
{
    if (m_contextMenu && m_contextMenu != menu)
    {
        DestroyMenu(m_contextMenu);
    }
    
    m_contextMenu = menu;
    Logger::Debug("Tray menu set");
}

void SystemTrayManager::ShowContextMenu()
{
    if (!m_contextMenu)
        return;

    POINT cursorPos = GetCursorPosition();
    
    // 设置前台窗口以确保菜单正确显示
    SetForegroundWindowFix();

    // 显示上下文菜单
    TrackPopupMenu(
        m_contextMenu,
        TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
        cursorPos.x,
        cursorPos.y,
        0,
        m_messageWindow,
        nullptr
    );

    // 发送空消息以确保菜单正确关闭
    PostMessage(m_messageWindow, WM_NULL, 0, 0);
}

void SystemTrayManager::UpdateMenuItems(bool isVisible)
{
    if (!m_contextMenu)
        return;

    // 更新显示/隐藏菜单项
    UINT showFlags = isVisible ? MF_GRAYED : MF_ENABLED;
    UINT hideFlags = isVisible ? MF_ENABLED : MF_GRAYED;

    EnableMenuItem(m_contextMenu, static_cast<UINT>(TrayMenuId::Show), showFlags);
    EnableMenuItem(m_contextMenu, static_cast<UINT>(TrayMenuId::Hide), hideFlags);
}

void SystemTrayManager::SetTrayEventCallback(std::function<void(TrayEvent)> callback)
{
    m_eventCallback = callback;
}

void SystemTrayManager::SetMenuCallback(TrayMenuId menuId, std::function<void()> callback)
{
    m_menuCallbacks[menuId] = callback;
}

void SystemTrayManager::SetShowCallback(std::function<void()> callback)
{
    SetMenuCallback(TrayMenuId::Show, callback);
}

void SystemTrayManager::SetHideCallback(std::function<void()> callback)
{
    SetMenuCallback(TrayMenuId::Hide, callback);
}

void SystemTrayManager::SetSettingsCallback(std::function<void()> callback)
{
    SetMenuCallback(TrayMenuId::Settings, callback);
}

void SystemTrayManager::SetExitCallback(std::function<void()> callback)
{
    SetMenuCallback(TrayMenuId::Exit, callback);
}

void SystemTrayManager::ShowBalloonTip(const std::wstring& title, const std::wstring& message, DWORD timeout)
{
    if (!m_iconVisible)
        return;

    m_nid.uFlags = NIF_INFO;
    m_nid.dwInfoFlags = NIIF_INFO;
    m_nid.uTimeout = timeout;
    wcscpy_s(m_nid.szInfoTitle, title.c_str());
    wcscpy_s(m_nid.szInfo, message.c_str());

    Shell_NotifyIcon(NIM_MODIFY, &m_nid);
    Logger::Debug("Balloon tip shown: " + std::string(title.begin(), title.end()));
}

bool SystemTrayManager::CreateMessageWindow()
{
    // 注册窗口类
    if (!s_classRegistered)
    {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = TrayWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = MESSAGE_WINDOW_CLASS;

        if (!RegisterClassExW(&wc))
        {
            Logger::Error("Failed to register tray window class");
            return false;
        }

        s_classRegistered = true;
    }

    // 创建消息窗口
    m_messageWindow = CreateWindowExW(
        0,
        MESSAGE_WINDOW_CLASS,
        L"Tray Message Window",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        GetModuleHandle(nullptr),
        this
    );

    if (!m_messageWindow)
    {
        Logger::Error("Failed to create tray message window");
        return false;
    }

    Logger::Debug("Tray message window created");
    return true;
}

void SystemTrayManager::DestroyMessageWindow()
{
    if (m_messageWindow)
    {
        DestroyWindow(m_messageWindow);
        m_messageWindow = nullptr;
        Logger::Debug("Tray message window destroyed");
    }

    if (s_classRegistered)
    {
        UnregisterClassW(MESSAGE_WINDOW_CLASS, GetModuleHandle(nullptr));
        s_classRegistered = false;
    }
}

LRESULT CALLBACK SystemTrayManager::TrayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SystemTrayManager* trayManager = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        trayManager = static_cast<SystemTrayManager*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(trayManager));
    }
    else
    {
        trayManager = reinterpret_cast<SystemTrayManager*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (trayManager)
    {
        return trayManager->HandleMessage(hwnd, msg, wParam, lParam);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT SystemTrayManager::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_COMMAND:
            HandleMenuCommand(wParam);
            return 0;

        default:
            if (msg == WM_TRAYICON)
            {
                HandleTrayIconClick(static_cast<UINT>(lParam));
                return 0;
            }
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SystemTrayManager::HandleTrayIconClick(UINT msg)
{
    switch (msg)
    {
        case WM_LBUTTONUP:
            Logger::Debug("Tray icon left clicked");
            if (m_eventCallback)
                m_eventCallback(TrayEvent::LeftClick);
            break;

        case WM_RBUTTONUP:
            Logger::Debug("Tray icon right clicked");
            ShowContextMenu();
            if (m_eventCallback)
                m_eventCallback(TrayEvent::RightClick);
            break;

        case WM_LBUTTONDBLCLK:
            Logger::Debug("Tray icon double clicked");
            if (m_eventCallback)
                m_eventCallback(TrayEvent::DoubleClick);
            break;

        case WM_MBUTTONUP:
            Logger::Debug("Tray icon middle clicked");
            if (m_eventCallback)
                m_eventCallback(TrayEvent::MiddleClick);
            break;
    }
}

void SystemTrayManager::HandleMenuCommand(WPARAM wParam)
{
    TrayMenuId menuId = static_cast<TrayMenuId>(LOWORD(wParam));
    
    Logger::Debug("Tray menu command: " + std::to_string(static_cast<int>(menuId)));

    auto it = m_menuCallbacks.find(menuId);
    if (it != m_menuCallbacks.end() && it->second)
    {
        try
        {
            it->second();
        }
        catch (const std::exception& e)
        {
            Logger::Error("Exception in tray menu callback: " + std::string(e.what()));
        }
    }
}

HMENU SystemTrayManager::CreateDefaultMenu()
{
    HMENU menu = CreatePopupMenu();
    if (!menu)
        return nullptr;

    AddMenuItem(menu, TrayMenuId::Show, L"显示(&S)");
    AddMenuItem(menu, TrayMenuId::Hide, L"隐藏(&H)");
    AddMenuSeparator(menu);
    AddMenuItem(menu, TrayMenuId::Settings, L"设置(&T)...");
    AddMenuSeparator(menu);
    AddMenuItem(menu, TrayMenuId::About, L"关于(&A)...");
    AddMenuItem(menu, TrayMenuId::Exit, L"退出(&X)");

    Logger::Debug("Default tray menu created");
    return menu;
}

void SystemTrayManager::AddMenuItem(HMENU menu, TrayMenuId id, const std::wstring& text, bool enabled)
{
    UINT flags = MF_STRING;
    if (!enabled)
        flags |= MF_GRAYED;

    AppendMenuW(menu, flags, static_cast<UINT>(id), text.c_str());
}

void SystemTrayManager::AddMenuSeparator(HMENU menu)
{
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
}

HICON SystemTrayManager::LoadTrayIcon()
{
    // 从资源加载图标
    HICON icon = static_cast<HICON>(LoadImage(
        GetModuleHandle(nullptr),
        MAKEINTRESOURCE(101),
        IMAGE_ICON,
        16, 16,  // 托盘图标使用16x16
        LR_DEFAULTCOLOR
    ));
    
    if (!icon)
    {
        // 尝试加载标准尺寸
        icon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(101));
    }
    
    if (!icon)
    {
        // 最后尝试加载系统图标
        icon = LoadIcon(nullptr, IDI_APPLICATION);
    }

    return icon;
}

HICON SystemTrayManager::CreateDefaultIcon()
{
    // 创建一个简单的默认图标
    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);
    
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 16, 16);
    HBITMAP oldBitmap = static_cast<HBITMAP>(SelectObject(memDC, hBitmap));
    
    // 绘制简单的圆形图标
    HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
    HBRUSH oldBrush = static_cast<HBRUSH>(SelectObject(memDC, brush));
    
    Ellipse(memDC, 2, 2, 14, 14);
    
    SelectObject(memDC, oldBrush);
    SelectObject(memDC, oldBitmap);
    DeleteObject(brush);
    DeleteDC(memDC);
    ReleaseDC(nullptr, hdc);
    
    // 创建图标
    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmColor = hBitmap;
    iconInfo.hbmMask = hBitmap;
    
    HICON icon = CreateIconIndirect(&iconInfo);
    DeleteObject(hBitmap);
    
    return icon;
}

POINT SystemTrayManager::GetCursorPosition()
{
    POINT point;
    GetCursorPos(&point);
    return point;
}

void SystemTrayManager::SetForegroundWindowFix()
{
    // Windows需要这个技巧来正确显示托盘菜单
    SetForegroundWindow(m_messageWindow);
}