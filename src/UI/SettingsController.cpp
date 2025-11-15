#include "UI/SettingsController.h"

#include "Audio/SpatialAudioRouter.h"
#include "Config/ConfigManager.h"
#include "Hotkeys/HotkeyController.h"
#include "UI/OverlayWindow.h"

#include <commdlg.h>

#include <algorithm>
#include <string>

using namespace UI;

SettingsController::SettingsController(HINSTANCE instance,
                                       OverlayWindow* overlay,
                                       Audio::SpatialAudioRouter* router,
                                       Hotkeys::HotkeyController* hotkeys,
                                       std::shared_ptr<Config::ConfigManager> config)
    : m_instance(instance)
    , m_overlay(overlay)
    , m_router(router)
    , m_hotkeys(hotkeys)
    , m_config(std::move(config))
{
}

void SettingsController::ShowContextMenu(POINT screenPos)
{
    HMENU menu = CreatePopupMenu();
    BuildMenu(menu);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, screenPos.x, screenPos.y, 0, m_overlay->Handle(), nullptr);
    DestroyMenu(menu);
}

bool SettingsController::ProcessDialogMessage(MSG* msg)
{
    return false;
}

void SettingsController::BuildMenu(HMENU menu)
{
    AppendMenuW(menu, MF_STRING, MenuId_OpacityIncrease, L"Increase Opacity");
    AppendMenuW(menu, MF_STRING, MenuId_OpacityDecrease, L"Decrease Opacity");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, MenuId_SensitivityIncrease, L"Increase Sensitivity");
    AppendMenuW(menu, MF_STRING, MenuId_SensitivityDecrease, L"Decrease Sensitivity");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, MenuId_PickColor, L"Choose Theme Color...");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    AppendMenuW(menu, MF_STRING | (m_config->Filter().front ? MF_CHECKED : MF_UNCHECKED), MenuId_ToggleFront, L"Detect Front");
    AppendMenuW(menu, MF_STRING | (m_config->Filter().back ? MF_CHECKED : MF_UNCHECKED), MenuId_ToggleBack, L"Detect Back");
    AppendMenuW(menu, MF_STRING | (m_config->Filter().left ? MF_CHECKED : MF_UNCHECKED), MenuId_ToggleLeft, L"Detect Left");
    AppendMenuW(menu, MF_STRING | (m_config->Filter().right ? MF_CHECKED : MF_UNCHECKED), MenuId_ToggleRight, L"Detect Right");
    AppendMenuW(menu, MF_STRING | (m_config->Filter().up ? MF_CHECKED : MF_UNCHECKED), MenuId_ToggleUp, L"Detect Up");
    AppendMenuW(menu, MF_STRING | (m_config->Filter().down ? MF_CHECKED : MF_UNCHECKED), MenuId_ToggleDown, L"Detect Down");

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    HMENU hotkeyMenu = CreatePopupMenu();
    AppendMenuW(hotkeyMenu, MF_STRING | ((m_config->Hotkeys().key == VK_HOME) ? MF_CHECKED : 0), MenuId_HotkeyHome, L"Home");
    AppendMenuW(hotkeyMenu, MF_STRING | ((m_config->Hotkeys().key == VK_INSERT) ? MF_CHECKED : 0), MenuId_HotkeyInsert, L"Insert");
    AppendMenuW(hotkeyMenu, MF_STRING | ((m_config->Hotkeys().key == VK_F8) ? MF_CHECKED : 0), MenuId_HotkeyF8, L"F8");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(hotkeyMenu), L"Toggle Hotkey");
    AppendMenuW(menu, MF_STRING, MenuId_Save, L"Save Settings");
}

void SettingsController::OnMenuCommand(UINT id)
{
    switch (id)
    {
    case MenuId_OpacityIncrease:
        AdjustTransparency(0.05f);
        break;
    case MenuId_OpacityDecrease:
        AdjustTransparency(-0.05f);
        break;
    case MenuId_SensitivityIncrease:
        AdjustSensitivity(-1.0f);
        break;
    case MenuId_SensitivityDecrease:
        AdjustSensitivity(1.0f);
        break;
    case MenuId_PickColor:
        PickThemeColor();
        break;
    case MenuId_ToggleFront:
        ToggleDirection(L"front");
        break;
    case MenuId_ToggleBack:
        ToggleDirection(L"back");
        break;
    case MenuId_ToggleLeft:
        ToggleDirection(L"left");
        break;
    case MenuId_ToggleRight:
        ToggleDirection(L"right");
        break;
    case MenuId_ToggleUp:
        ToggleDirection(L"up");
        break;
    case MenuId_ToggleDown:
        ToggleDirection(L"down");
        break;
    case MenuId_HotkeyHome:
        m_config->Hotkeys().key = VK_HOME;
        m_config->Save();
        if (m_hotkeys)
        {
            m_hotkeys->Register();
        }
        break;
    case MenuId_HotkeyInsert:
        m_config->Hotkeys().key = VK_INSERT;
        m_config->Save();
        if (m_hotkeys)
        {
            m_hotkeys->Register();
        }
        break;
    case MenuId_HotkeyF8:
        m_config->Hotkeys().key = VK_F8;
        m_config->Save();
        if (m_hotkeys)
        {
            m_hotkeys->Register();
        }
        break;
    case MenuId_Save:
        m_config->Save();
        break;
    }
}

void SettingsController::AdjustTransparency(float delta)
{
    auto& theme = m_config->Theme();
    theme.opacity = std::clamp(theme.opacity + delta, 0.2f, 1.0f);
    m_overlay->UpdateTransparency();
    m_config->Save();
}

void SettingsController::AdjustSensitivity(float delta)
{
    auto& sensitivity = m_config->Sensitivity();
    sensitivity.thresholdDb = std::clamp(sensitivity.thresholdDb + delta, -80.0f, -10.0f);
    if (m_router)
    {
        m_router->ApplySensitivity();
    }
    m_config->Save();
}

void SettingsController::PickThemeColor()
{
    COLORREF customColors[16]{};
    CHOOSECOLORW cc{};
    cc.lStructSize = sizeof(CHOOSECOLORW);
    cc.hwndOwner = m_overlay->Handle();
    cc.lpCustColors = customColors;
    cc.rgbResult = m_config->Theme().primaryColor;
    cc.Flags = CC_RGBINIT | CC_FULLOPEN;

    if (ChooseColorW(&cc))
    {
        m_config->Theme().primaryColor = cc.rgbResult;
        m_config->Save();
        m_overlay->ForceRender();
    }
}

void SettingsController::ToggleDirection(const std::wstring& direction)
{
    const bool enabled = m_config->IsDirectionEnabled(direction);
    m_config->SetDirectionEnabled(direction, !enabled);
    m_config->Save();
}
