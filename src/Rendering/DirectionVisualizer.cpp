#include "Rendering/DirectionVisualizer.h"

#include <d2d1helper.h>
#include <dwrite.h>

#include "Util/ComException.h"

#include <algorithm>
#include <cwchar>
#include <numbers>

using namespace Rendering;

DirectionVisualizer::DirectionVisualizer(std::shared_ptr<Config::ConfigManager> config)
    : m_config(std::move(config))
    , m_sensitivity(m_config->Sensitivity())
{
    D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &m_factory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(m_dwriteFactory.ReleaseAndGetAddressOf()));
}

DirectionVisualizer::~DirectionVisualizer() = default;

void DirectionVisualizer::Initialize(HWND hwnd)
{
    CreateDeviceResources(hwnd);
}

void DirectionVisualizer::Resize(UINT width, UINT height)
{
    m_width = width;
    m_height = height;

    if (m_renderTarget)
    {
        m_renderTarget->Resize(D2D1::SizeU(width, height));
    }
}

void DirectionVisualizer::Render()
{
    if (!m_renderTarget)
    {
        return;
    }

    auto state = CurrentState();
    if (!state.visible)
    {
        m_renderTarget->BeginDraw();
        m_renderTarget->Clear(D2D1::ColorF(0, 0));
        m_renderTarget->EndDraw();
        return;
    }

    auto direction = state.direction;

    m_renderTarget->BeginDraw();
    m_renderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, m_config->Theme().opacity * 0.2f));

    const auto center = D2D1::Point2F(static_cast<float>(m_width) / 2.0f, static_cast<float>(m_height) / 2.0f);
    const float radius = std::min(m_width, m_height) * 0.45f;

    m_renderTarget->DrawEllipse(D2D1::Ellipse(center, radius, radius), m_backgroundBrush.Get(), 2.0f);

    const float arrowLength = radius * direction.magnitude;
    const float azimuth = direction.azimuth;
    const float elevation = direction.elevation;

    const float x = std::sin(azimuth) * std::cos(elevation);
    const float y = -std::sin(elevation);
    const float z = std::cos(azimuth) * std::cos(elevation);

    const D2D1_POINT_2F arrowEnd{
        center.x + arrowLength * x,
        center.y + arrowLength * z,
    };

    m_renderTarget->DrawLine(center, arrowEnd, m_primaryBrush.Get(), 4.0f);

    wchar_t buffer[128];
    swprintf_s(buffer, L"Az %.0f°\nEl %.0f°\n%s",
               direction.azimuth * 180.0f / std::numbers::pi_v<float>,
               direction.elevation * 180.0f / std::numbers::pi_v<float>,
               direction.dominantSessionName.empty() ? L"" : direction.dominantSessionName.c_str());

    D2D1_RECT_F textRect{ center.x - radius, center.y + radius * 0.25f, center.x + radius, center.y + radius };
    m_renderTarget->DrawTextW(buffer, static_cast<UINT32>(wcslen(buffer)), m_textFormat.Get(), textRect, m_primaryBrush.Get());

    m_renderTarget->EndDraw();
}

void DirectionVisualizer::UpdateDirection(const Audio::AudioDirection& direction)
{
    std::scoped_lock lock{m_mutex};
    m_state.direction = direction;
}

void DirectionVisualizer::SetVisible(bool visible)
{
    std::scoped_lock lock{m_mutex};
    m_state.visible = visible;
}

void DirectionVisualizer::SetSensitivity(const Config::SensitivityConfig& sensitivity)
{
    std::scoped_lock lock{m_mutex};
    m_sensitivity = sensitivity;
}

VisualState DirectionVisualizer::CurrentState() const
{
    std::scoped_lock lock{m_mutex};
    return m_state;
}

void DirectionVisualizer::CreateDeviceResources(HWND hwnd)
{
    if (m_renderTarget)
    {
        return;
    }

    D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(m_width, m_height));

    THROW_IF_FAILED(m_factory->CreateHwndRenderTarget(rtProps, hwndProps, &m_renderTarget));

    const auto primaryColor = ColorFromConfig();
    THROW_IF_FAILED(m_renderTarget->CreateSolidColorBrush(primaryColor, &m_primaryBrush));
    THROW_IF_FAILED(m_renderTarget->CreateSolidColorBrush(D2D1::ColorF(0.2f, 0.2f, 0.2f, m_config->Theme().opacity), &m_backgroundBrush));

    THROW_IF_FAILED(m_dwriteFactory->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL,
                                                      DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"en-US", &m_textFormat));
    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

D2D1::ColorF DirectionVisualizer::ColorFromConfig() const
{
    const auto color = m_config->Theme().primaryColor;
    return D2D1::ColorF(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, m_config->Theme().opacity);
}
