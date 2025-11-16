#pragma once

#include <windows.h>

#include <memory>
#include <string>

#include "Config/ConfigManager.h"

namespace Audio { class SpatialAudioRouter; }
namespace Hotkeys { class HotkeyController; }
namespace UI { class OverlayWindow; }

namespace UI
{
class SettingsController
{
public:
    SettingsController(HINSTANCE instance,
                       OverlayWindow* overlay,
                       Audio::SpatialAudioRouter* router,
                       Hotkeys::HotkeyController* hotkeys,
                       std::shared_ptr<Config::ConfigManager> config);

    void ShowContextMenu(POINT screenPos);
    bool ProcessDialogMessage(MSG* msg);

    void OnMenuCommand(UINT id);
    void SetHotkeyController(Hotkeys::HotkeyController* hotkeys) { m_hotkeys = hotkeys; }

private:
    void BuildMenu(HMENU menu);
    void AdjustTransparency(float delta);
    void AdjustSensitivity(float delta);
    void PickThemeColor();
    void ToggleDirection(const std::wstring& direction);
    HINSTANCE m_instance;
    OverlayWindow* m_overlay;
    Audio::SpatialAudioRouter* m_router;
    Hotkeys::HotkeyController* m_hotkeys;
    std::shared_ptr<Config::ConfigManager> m_config;

    enum MenuId
    {
        MenuId_OpacityIncrease = 1,
        MenuId_OpacityDecrease,
        MenuId_SensitivityIncrease,
        MenuId_SensitivityDecrease,
        MenuId_PickColor,
        MenuId_ToggleFront,
        MenuId_ToggleBack,
        MenuId_ToggleLeft,
        MenuId_ToggleRight,
        MenuId_ToggleUp,
        MenuId_ToggleDown,
        MenuId_HotkeyHome,
        MenuId_HotkeyInsert,
        MenuId_HotkeyF8,
        MenuId_Save,
    };
};
}
