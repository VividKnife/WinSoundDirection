#pragma once

#include <windows.h>

#include <memory>
#include <string>

namespace Config { class ConfigManager; }
namespace Rendering { class DirectionVisualizer; }

namespace UI
{
class OverlayWindow
{
public:
    OverlayWindow(HINSTANCE instance,
                  std::shared_ptr<Rendering::DirectionVisualizer> visualizer,
                  std::shared_ptr<Config::ConfigManager> config);

    void Create(int cmdShow);
    void Destroy();

    void Show();
    void Hide();
    void Toggle();

    void UpdateTransparency();
    void ForceRender();
    void SetSettingsController(class SettingsController* controller) { m_settingsController = controller; }

    [[nodiscard]] HWND Handle() const noexcept { return m_hwnd; }
    [[nodiscard]] bool IsVisible() const noexcept { return m_visible; }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void RegisterClass();
    void UpdateVisuals();
    void BeginDrag(POINT point);
    void PerformDrag(POINT point);
    void EndDrag();

    HINSTANCE m_instance;
    std::shared_ptr<Rendering::DirectionVisualizer> m_visualizer;
    std::shared_ptr<Config::ConfigManager> m_config;
    HWND m_hwnd{nullptr};
    bool m_visible{true};
    bool m_dragging{false};
    POINT m_dragOffset{};
    class SettingsController* m_settingsController{nullptr};
};
}
