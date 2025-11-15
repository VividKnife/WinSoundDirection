#include "Hotkeys/HotkeyController.h"

#include "Config/ConfigManager.h"
#include "UI/OverlayWindow.h"

using namespace Hotkeys;

namespace
{
constexpr wchar_t kHotkeyWindowClass[] = L"SpatialAudioHotkeyWindow";
constexpr UINT WM_HOTKEY_EVENT = WM_APP + 1;
}

HotkeyController::HotkeyController(HINSTANCE instance, UI::OverlayWindow* overlay, std::shared_ptr<Config::ConfigManager> config)
    : m_instance(instance)
    , m_overlay(overlay)
    , m_config(std::move(config))
{
}

HotkeyController::~HotkeyController()
{
    Shutdown();
}

void HotkeyController::Register()
{
    if (!m_hwnd)
    {
        WNDCLASSW wc{};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = m_instance;
        wc.lpszClassName = kHotkeyWindowClass;
        RegisterClassW(&wc);

        m_hwnd = CreateWindowExW(0, kHotkeyWindowClass, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, m_instance, this);
    }

    UnregisterHotkey();
    RegisterHotkey();
}

void HotkeyController::Shutdown()
{
    UnregisterHotkey();
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void HotkeyController::RegisterHotkey()
{
    const auto hotkey = m_config->Hotkeys();
    RegisterHotKey(m_hwnd, m_hotkeyId, hotkey.modifier, hotkey.key);
}

void HotkeyController::UnregisterHotkey()
{
    if (m_hwnd)
    {
        UnregisterHotKey(m_hwnd, m_hotkeyId);
    }
}

LRESULT CALLBACK HotkeyController::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto controller = reinterpret_cast<HotkeyController*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(controller));
    }

    auto controller = reinterpret_cast<HotkeyController*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!controller)
    {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    if (message == WM_HOTKEY)
    {
        controller->m_overlay->Toggle();
        return 0;
    }

    if (message == WM_SETTINGCHANGE)
    {
        controller->UnregisterHotkey();
        controller->RegisterHotkey();
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}
