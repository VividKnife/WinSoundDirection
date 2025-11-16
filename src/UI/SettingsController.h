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
    float CurrentOpacity() const { return m_config->Theme().opacity; }
    void UpdateOpacityFromDialog(float opacity);
    float CurrentDetectionRange() const { return m_config->Sensitivity().distanceScale; }
    void UpdateDetectionRangeFromDialog(float scale);

private:
    void BuildMenu(HMENU menu);
    void ShowOpacityDialog();
    void ShowDetectionRangeDialog();
    void AdjustTransparency(float delta);
    void AdjustSensitivity(float delta);
    void PickThemeColor();
    HINSTANCE m_instance;
    OverlayWindow* m_overlay;
    Audio::SpatialAudioRouter* m_router;
    Hotkeys::HotkeyController* m_hotkeys;
    std::shared_ptr<Config::ConfigManager> m_config;

    enum MenuId
    {
        MenuId_OpacityDialog = 1,
        MenuId_DetectionRange,
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
        MenuId_AudioModeAuto,
        MenuId_AudioModeHeadphone,
        MenuId_AudioModeMultichannel,
    };
};
}
