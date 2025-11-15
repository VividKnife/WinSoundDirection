#include "UI/TrayIcon.h"

#include "Diagnostics/PerformanceMonitor.h"
#include "UI/OverlayWindow.h"
#include "UI/SettingsController.h"

#include <cwchar>
#include <string>

using namespace UI;

namespace
{
constexpr UINT WM_TRAYICON = WM_APP + 100;
constexpr UINT ID_TRAYICON = 1001;
constexpr UINT ID_TRAY_MENU_SHOW = 2001;
constexpr UINT ID_TRAY_MENU_SETTINGS = 2002;
constexpr UINT ID_TRAY_MENU_EXIT = 2003;
constexpr UINT ID_TRAY_MENU_PERFORMANCE = 2004;
}

TrayIcon::TrayIcon(HINSTANCE instance,
                   OverlayWindow* overlay,
                   SettingsController* settings,
                   std::shared_ptr<Config::ConfigManager> config,
                   std::shared_ptr<Diagnostics::PerformanceMonitor> performance)
    : m_instance(instance)
    , m_overlay(overlay)
    , m_settings(settings)
    , m_config(std::move(config))
    , m_performance(std::move(performance))
{
}

TrayIcon::~TrayIcon()
{
    Destroy();
}

void TrayIcon::Create()
{
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_instance;
    wc.lpszClassName = L"SpatialAudioTrayMessageWindow";
    RegisterClassW(&wc);

    m_messageWindow = CreateWindowExW(0, wc.lpszClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, m_instance, this);

    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_messageWindow;
    m_nid.uID = ID_TRAYICON;
    m_nid.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON;
    m_nid.uCallbackMessage = WM_TRAYICON;
    m_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wcsncpy_s(m_nid.szTip, L"Spatial Audio Visualizer", _TRUNCATE);

    Shell_NotifyIconW(NIM_ADD, &m_nid);
    UpdateTooltip();
}

void TrayIcon::Destroy()
{
    if (m_nid.hWnd)
    {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_nid.hWnd = nullptr;
    }

    if (m_messageWindow)
    {
        DestroyWindow(m_messageWindow);
        m_messageWindow = nullptr;
    }
}

void TrayIcon::UpdateTooltip()
{
    std::wstring tooltip = L"Spatial Audio Visualizer";
    if (m_performance)
    {
        auto stats = m_performance->GetLatest();
        wchar_t buffer[128];
        swprintf_s(buffer, L"\nCPU %.1f%% MEM %zu MB", stats.cpuPercent, stats.memoryMb);
        tooltip += buffer;
    }

    wcsncpy_s(m_nid.szTip, tooltip.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void TrayIcon::HandleCommand(UINT id)
{
    switch (id)
    {
    case ID_TRAY_MENU_SHOW:
        m_overlay->Toggle();
        break;
    case ID_TRAY_MENU_SETTINGS:
        POINT pt;
        GetCursorPos(&pt);
        m_settings->ShowContextMenu(pt);
        break;
    case ID_TRAY_MENU_PERFORMANCE:
        UpdateTooltip();
        break;
    case ID_TRAY_MENU_EXIT:
        PostQuitMessage(0);
        break;
    }
}

LRESULT CALLBACK TrayIcon::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto icon = reinterpret_cast<TrayIcon*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(icon));
    }

    auto icon = reinterpret_cast<TrayIcon*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!icon)
    {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_TRAYICON:
        if (LOWORD(lParam) == WM_LBUTTONDBLCLK)
        {
            icon->m_overlay->Show();
        }
        else if (LOWORD(lParam) == WM_RBUTTONUP)
        {
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING, ID_TRAY_MENU_SHOW, icon->m_overlay->IsVisible() ? L"Hide" : L"Show");
            AppendMenuW(menu, MF_STRING, ID_TRAY_MENU_SETTINGS, L"Settings");
            AppendMenuW(menu, MF_STRING, ID_TRAY_MENU_PERFORMANCE, L"Refresh Performance");
            AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
            AppendMenuW(menu, MF_STRING, ID_TRAY_MENU_EXIT, L"Exit");
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            const UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, nullptr);
            if (cmd)
            {
                icon->HandleCommand(cmd);
            }
            DestroyMenu(menu);
        }
        break;
    case WM_COMMAND:
        icon->HandleCommand(LOWORD(wParam));
        break;
    case WM_DESTROY:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}
