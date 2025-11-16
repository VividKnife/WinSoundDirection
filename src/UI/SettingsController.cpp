#include "UI/SettingsController.h"

#include "Audio/SpatialAudioRouter.h"
#include "Config/ConfigManager.h"
#include "Hotkeys/HotkeyController.h"
#include "UI/OverlayWindow.h"

#include <commdlg.h>
#include <commctrl.h>

#include <algorithm>
#include <string>
#include <cmath>

namespace
{
constexpr int IDD_OPACITY_DIALOG = 2001;
constexpr int IDC_OPACITY_SLIDER = 2002;
constexpr int IDC_OPACITY_VALUE = 2003;

constexpr int IDD_RANGE_DIALOG = 2004;
constexpr int IDC_RANGE_SLIDER = 2005;
constexpr int IDC_RANGE_VALUE = 2006;

INT_PTR CALLBACK OpacityDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        auto controller = reinterpret_cast<UI::SettingsController*>(lParam);
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(controller));

        HWND slider = GetDlgItem(hwndDlg, IDC_OPACITY_SLIDER);
        SendMessageW(slider, TBM_SETRANGE, TRUE, MAKELONG(20, 100));

        int pos = static_cast<int>(controller->CurrentOpacity() * 100.0f + 0.5f);
        if (pos < 20) pos = 20;
        if (pos > 100) pos = 100;
        SendMessageW(slider, TBM_SETPOS, TRUE, pos);

        wchar_t buf[32];
        swprintf_s(buf, L"%d%%", pos);
        SetWindowTextW(GetDlgItem(hwndDlg, IDC_OPACITY_VALUE), buf);
        return TRUE;
    }
    case WM_HSCROLL:
    {
        auto controller = reinterpret_cast<UI::SettingsController*>(GetWindowLongPtrW(hwndDlg, GWLP_USERDATA));
        if (!controller)
        {
            break;
        }

        HWND slider = reinterpret_cast<HWND>(lParam);
        if (slider == GetDlgItem(hwndDlg, IDC_OPACITY_SLIDER))
        {
            int pos = static_cast<int>(SendMessageW(slider, TBM_GETPOS, 0, 0));
            if (pos < 20) pos = 20;
            if (pos > 100) pos = 100;

            float opacity = pos / 100.0f;
            controller->UpdateOpacityFromDialog(opacity);

            wchar_t buf[32];
            swprintf_s(buf, L"%d%%", pos);
            SetWindowTextW(GetDlgItem(hwndDlg, IDC_OPACITY_VALUE), buf);
        }
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK RangeDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        auto controller = reinterpret_cast<UI::SettingsController*>(lParam);
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(controller));

        HWND slider = GetDlgItem(hwndDlg, IDC_RANGE_SLIDER);
        // Map 50..200 -> 0.5..2.0
        SendMessageW(slider, TBM_SETRANGE, TRUE, MAKELONG(50, 200));

        float current = controller->CurrentDetectionRange();
        int pos = static_cast<int>(current * 100.0f + 0.5f);
        if (pos < 50) pos = 50;
        if (pos > 200) pos = 200;
        SendMessageW(slider, TBM_SETPOS, TRUE, pos);

        wchar_t buf[32];
        swprintf_s(buf, L"x%.2f", static_cast<float>(pos) / 100.0f);
        SetWindowTextW(GetDlgItem(hwndDlg, IDC_RANGE_VALUE), buf);
        return TRUE;
    }
    case WM_HSCROLL:
    {
        auto controller = reinterpret_cast<UI::SettingsController*>(GetWindowLongPtrW(hwndDlg, GWLP_USERDATA));
        if (!controller)
        {
            break;
        }

        HWND slider = reinterpret_cast<HWND>(lParam);
        if (slider == GetDlgItem(hwndDlg, IDC_RANGE_SLIDER))
        {
            int pos = static_cast<int>(SendMessageW(slider, TBM_GETPOS, 0, 0));
            if (pos < 50) pos = 50;
            if (pos > 200) pos = 200;

            float scale = static_cast<float>(pos) / 100.0f;
            controller->UpdateDetectionRangeFromDialog(scale);

            wchar_t buf[32];
            swprintf_s(buf, L"x%.2f", scale);
            SetWindowTextW(GetDlgItem(hwndDlg, IDC_RANGE_VALUE), buf);
        }
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }

    return FALSE;
}
}

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
    // Audio mode (top group)
    HMENU audioMenu = CreatePopupMenu();
    auto mode = m_config->AudioMode();
    AppendMenuW(audioMenu, MF_STRING | ((mode == Config::AudioModeOverride::Auto) ? MF_CHECKED : 0), MenuId_AudioModeAuto, L"Automatic");
    AppendMenuW(audioMenu, MF_STRING | ((mode == Config::AudioModeOverride::Headphone) ? MF_CHECKED : 0), MenuId_AudioModeHeadphone, L"Headphone (LR only)");
    AppendMenuW(audioMenu, MF_STRING | ((mode == Config::AudioModeOverride::Multichannel) ? MF_CHECKED : 0), MenuId_AudioModeMultichannel, L"Multichannel (3D)");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(audioMenu), L"Audio Mode");

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    // Visual tuning
    AppendMenuW(menu, MF_STRING, MenuId_OpacityDialog, L"Opacity...");
    AppendMenuW(menu, MF_STRING, MenuId_DetectionRange, L"Detection Range...");
    AppendMenuW(menu, MF_STRING, MenuId_SensitivityIncrease, L"Increase Sensitivity");
    AppendMenuW(menu, MF_STRING, MenuId_SensitivityDecrease, L"Decrease Sensitivity");
    AppendMenuW(menu, MF_STRING, MenuId_PickColor, L"Theme Color...");

    // Pattern presets
    HMENU patternMenu = CreatePopupMenu();
    auto preset = CurrentPatternPreset();
    UINT conservativeFlags = MF_STRING | ((preset == PatternPreset::Conservative) ? MF_CHECKED : 0);
    UINT balancedFlags = MF_STRING | ((preset == PatternPreset::Balanced) ? MF_CHECKED : 0);
    UINT aggressiveFlags = MF_STRING | ((preset == PatternPreset::Aggressive) ? MF_CHECKED : 0);

    AppendMenuW(patternMenu, conservativeFlags, MenuId_PatternPresetConservative, L"Conservative");
    AppendMenuW(patternMenu, balancedFlags, MenuId_PatternPresetBalanced, L"Balanced (default)");
    AppendMenuW(patternMenu, aggressiveFlags, MenuId_PatternPresetAggressive, L"Aggressive");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(patternMenu), L"Pattern Preset");

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    // Hotkeys
    HMENU hotkeyMenu = CreatePopupMenu();
    AppendMenuW(hotkeyMenu, MF_STRING | ((m_config->Hotkeys().key == VK_HOME) ? MF_CHECKED : 0), MenuId_HotkeyHome, L"Home");
    AppendMenuW(hotkeyMenu, MF_STRING | ((m_config->Hotkeys().key == VK_INSERT) ? MF_CHECKED : 0), MenuId_HotkeyInsert, L"Insert");
    AppendMenuW(hotkeyMenu, MF_STRING | ((m_config->Hotkeys().key == VK_F8) ? MF_CHECKED : 0), MenuId_HotkeyF8, L"F8");
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(hotkeyMenu), L"Toggle Hotkey");

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, MenuId_Save, L"Save Settings");
}

void SettingsController::OnMenuCommand(UINT id)
{
    switch (id)
    {
    case MenuId_OpacityDialog:
        ShowOpacityDialog();
        break;
    case MenuId_DetectionRange:
        ShowDetectionRangeDialog();
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
    case MenuId_PatternPresetConservative:
        ApplyPatternPresetConservative();
        break;
    case MenuId_PatternPresetBalanced:
        ApplyPatternPresetBalanced();
        break;
    case MenuId_PatternPresetAggressive:
        ApplyPatternPresetAggressive();
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
    case MenuId_AudioModeAuto:
        m_config->SetAudioMode(Config::AudioModeOverride::Auto);
        m_config->Save();
        break;
    case MenuId_AudioModeHeadphone:
        m_config->SetAudioMode(Config::AudioModeOverride::Headphone);
        m_config->Save();
        break;
    case MenuId_AudioModeMultichannel:
        m_config->SetAudioMode(Config::AudioModeOverride::Multichannel);
        m_config->Save();
        break;
    }
}

void SettingsController::ShowOpacityDialog()
{
    DialogBoxParamW(m_instance, MAKEINTRESOURCEW(IDD_OPACITY_DIALOG), m_overlay->Handle(), OpacityDialogProc, reinterpret_cast<LPARAM>(this));
}

void SettingsController::ShowDetectionRangeDialog()
{
    DialogBoxParamW(m_instance, MAKEINTRESOURCEW(IDD_RANGE_DIALOG), m_overlay->Handle(), RangeDialogProc, reinterpret_cast<LPARAM>(this));
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

void SettingsController::ApplyPatternPresetConservative()
{
    auto& sensitivity = m_config->Sensitivity();
    // Require stronger, clearer events; narrower rhythm window and direction
    sensitivity.strongMagnitude = 0.7f;
    sensitivity.strongJump = 0.35f;
    sensitivity.rhythmMinInterval = 0.30f;
    sensitivity.rhythmMaxInterval = 0.60f;
    sensitivity.rhythmDirectionDeg = 30.0f;

    if (m_router)
    {
        m_router->ApplySensitivity();
    }
    m_config->Save();
}

void SettingsController::ApplyPatternPresetBalanced()
{
    auto& sensitivity = m_config->Sensitivity();
    // Default tuning: compromise between stability and responsiveness
    sensitivity.strongMagnitude = 0.6f;
    sensitivity.strongJump = 0.25f;
    sensitivity.rhythmMinInterval = 0.25f;
    sensitivity.rhythmMaxInterval = 0.70f;
    sensitivity.rhythmDirectionDeg = 40.0f;

    if (m_router)
    {
        m_router->ApplySensitivity();
    }
    m_config->Save();
}

void SettingsController::ApplyPatternPresetAggressive()
{
    auto& sensitivity = m_config->Sensitivity();
    // Easier to trigger Strong/Medium; wider rhythm and direction windows
    sensitivity.strongMagnitude = 0.5f;
    sensitivity.strongJump = 0.15f;
    sensitivity.rhythmMinInterval = 0.20f;
    sensitivity.rhythmMaxInterval = 0.80f;
    sensitivity.rhythmDirectionDeg = 60.0f;

    if (m_router)
    {
        m_router->ApplySensitivity();
    }
    m_config->Save();
}

SettingsController::PatternPreset SettingsController::CurrentPatternPreset() const
{
    const auto& s = m_config->Sensitivity();

    auto approxEq = [](float a, float b)
    {
        return std::fabs(a - b) < 0.01f;
    };

    auto matches = [&](float strongMag,
                       float strongJump,
                       float minInt,
                       float maxInt,
                       float dirDeg)
    {
        return approxEq(s.strongMagnitude, strongMag) &&
               approxEq(s.strongJump, strongJump) &&
               approxEq(s.rhythmMinInterval, minInt) &&
               approxEq(s.rhythmMaxInterval, maxInt) &&
               approxEq(s.rhythmDirectionDeg, dirDeg);
    };

    if (matches(0.7f, 0.35f, 0.30f, 0.60f, 30.0f))
    {
        return PatternPreset::Conservative;
    }
    if (matches(0.6f, 0.25f, 0.25f, 0.70f, 40.0f))
    {
        return PatternPreset::Balanced;
    }
    if (matches(0.5f, 0.15f, 0.20f, 0.80f, 60.0f))
    {
        return PatternPreset::Aggressive;
    }

    return PatternPreset::Custom;
}

void SettingsController::UpdateOpacityFromDialog(float opacity)
{
    auto& theme = m_config->Theme();
    theme.opacity = std::clamp(opacity, 0.2f, 1.0f);
    m_overlay->UpdateTransparency();
    m_config->Save();
}

void SettingsController::UpdateDetectionRangeFromDialog(float scale)
{
    auto& sensitivity = m_config->Sensitivity();
    sensitivity.distanceScale = std::clamp(scale, 0.5f, 2.0f);
    if (m_router)
    {
        m_router->ApplySensitivity();
    }
    m_config->Save();
}
