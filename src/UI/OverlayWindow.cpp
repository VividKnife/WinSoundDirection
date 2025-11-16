#include "UI/OverlayWindow.h"

#include "Config/ConfigManager.h"
#include "Rendering/DirectionVisualizer.h"
#include "UI/SettingsController.h"
#include "Util/ComException.h"

#include <dwmapi.h>
#include <windowsx.h>

#include <stdexcept>

using namespace UI;

namespace
{
constexpr wchar_t kWindowClassName[] = L"SpatialAudioVisualizerOverlay";
constexpr UINT_PTR kRenderTimerId = 1001;
constexpr UINT kRenderTimerIntervalMs = 16;
}

OverlayWindow::OverlayWindow(HINSTANCE instance,
                             Rendering::DirectionVisualizer* visualizer,
                             std::shared_ptr<Config::ConfigManager> config)
    : m_instance(instance)
    , m_visualizer(visualizer)
    , m_config(std::move(config))
{
}

void OverlayWindow::Create(int cmdShow)
{
    RegisterClass();

    m_hwnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
                             kWindowClassName,
                             L"Spatial Audio Visualizer",
                             WS_POPUP,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             320,
                             320,
                             nullptr,
                             nullptr,
                             m_instance,
                             this);

    if (!m_hwnd)
    {
        throw std::runtime_error("Failed to create overlay window");
    }

    SetLayeredWindowAttributes(m_hwnd, 0, static_cast<BYTE>(255 * m_config->Theme().opacity), LWA_ALPHA);
    SetWindowPos(m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    ShowWindow(m_hwnd, cmdShow);
    UpdateWindow(m_hwnd);

    m_visualizer->Initialize(m_hwnd);
    UpdateVisuals();
    SetTimer(m_hwnd, kRenderTimerId, kRenderTimerIntervalMs, nullptr);
}

void OverlayWindow::Destroy()
{
    if (m_hwnd)
    {
        KillTimer(m_hwnd, kRenderTimerId);
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

void OverlayWindow::Show()
{
    m_visible = true;
    if (m_visualizer)
    {
        m_visualizer->SetVisible(true);
    }
    ShowWindow(m_hwnd, SW_SHOW);
}

void OverlayWindow::Hide()
{
    m_visible = false;
    if (m_visualizer)
    {
        m_visualizer->SetVisible(false);
    }
    ShowWindow(m_hwnd, SW_HIDE);
}

void OverlayWindow::Toggle()
{
    if (m_visible)
    {
        Hide();
    }
    else
    {
        Show();
    }
}

void OverlayWindow::UpdateTransparency()
{
    SetLayeredWindowAttributes(m_hwnd, 0, static_cast<BYTE>(255 * m_config->Theme().opacity), LWA_ALPHA);
}

void OverlayWindow::ForceRender()
{
    if (m_visible)
    {
        m_visualizer->Render();
    }
}

void OverlayWindow::RegisterClass()
{
    static bool registered = false;
    if (registered)
    {
        return;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = m_instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = kWindowClassName;

    if (!RegisterClassExW(&wc))
    {
        throw std::runtime_error("Failed to register overlay class");
    }

    registered = true;
}

LRESULT CALLBACK OverlayWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto window = reinterpret_cast<OverlayWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
        window->m_hwnd = hwnd;
    }

    auto window = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (window)
    {
        return window->HandleMessage(message, wParam, lParam);
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

LRESULT OverlayWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TIMER:
        if (wParam == kRenderTimerId)
        {
            ForceRender();
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_LBUTTONDOWN:
        SetCapture(m_hwnd);
        BeginDrag({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
        return 0;
    case WM_MOUSEMOVE:
        if (m_dragging)
        {
            PerformDrag({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (m_dragging)
        {
            EndDrag();
            ReleaseCapture();
            return 0;
        }
        break;
    case WM_RBUTTONUP:
        SendMessageW(m_hwnd, WM_CONTEXTMENU, reinterpret_cast<WPARAM>(m_hwnd), lParam);
        return 0;
    case WM_SIZE:
        UpdateVisuals();
        return 0;
    case WM_CONTEXTMENU:
        if (m_settingsController)
        {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            if (pt.x == -1 && pt.y == -1)
            {
                GetCursorPos(&pt);
            }
            m_settingsController->ShowContextMenu(pt);
            return 0;
        }
        break;
    case WM_COMMAND:
        if (m_settingsController)
        {
            m_settingsController->OnMenuCommand(LOWORD(wParam));
            return 0;
        }
        break;
    case WM_DESTROY:
        KillTimer(m_hwnd, kRenderTimerId);
        break;
    }

    return DefWindowProcW(m_hwnd, message, wParam, lParam);
}

void OverlayWindow::UpdateVisuals()
{
    RECT rect{};
    GetClientRect(m_hwnd, &rect);
    m_visualizer->Resize(rect.right - rect.left, rect.bottom - rect.top);
}

void OverlayWindow::BeginDrag(POINT point)
{
    RECT windowRect{};
    GetWindowRect(m_hwnd, &windowRect);
    POINT screenPoint{ point.x, point.y };
    ClientToScreen(m_hwnd, &screenPoint);
    m_dragOffset = { screenPoint.x - windowRect.left, screenPoint.y - windowRect.top };
    m_dragging = true;
}

void OverlayWindow::PerformDrag(POINT point)
{
    if (!m_dragging)
    {
        return;
    }

    POINT screenPoint{ point.x, point.y };
    ClientToScreen(m_hwnd, &screenPoint);

    RECT rect{};
    GetWindowRect(m_hwnd, &rect);

    int newLeft = screenPoint.x - m_dragOffset.x;
    int newTop = screenPoint.y - m_dragOffset.y;

    SetWindowPos(m_hwnd, HWND_TOPMOST, newLeft, newTop, 0, 0, SWP_NOSIZE);
}

void OverlayWindow::EndDrag()
{
    m_dragging = false;
}
