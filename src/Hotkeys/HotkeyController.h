#pragma once

#include <windows.h>

#include <memory>

namespace Config { class ConfigManager; }
namespace UI { class OverlayWindow; }

namespace Hotkeys
{
class HotkeyController
{
public:
    HotkeyController(HINSTANCE instance, UI::OverlayWindow* overlay, std::shared_ptr<Config::ConfigManager> config);
    ~HotkeyController();

    void Register();
    void Shutdown();

private:
    void RegisterHotkey();
    void UnregisterHotkey();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HINSTANCE m_instance;
    UI::OverlayWindow* m_overlay;
    std::shared_ptr<Config::ConfigManager> m_config;

    HWND m_hwnd{nullptr};
    UINT m_hotkeyId{1};
};
}
