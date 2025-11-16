#include "Rendering/DirectionVisualizer.h"

#include <d2d1helper.h>
#include <dwrite.h>

#include "Util/ComException.h"

#include <algorithm>
#include <cwchar>
#include <cmath>

using namespace Rendering;

namespace
{
constexpr float kPi = 3.14159265358979323846f;
}

DirectionVisualizer::DirectionVisualizer(std::shared_ptr<Config::ConfigManager> config)
    : m_config(std::move(config))
    , m_sensitivity(m_config->Sensitivity())
{
    D2D1_FACTORY_OPTIONS options{};
    THROW_IF_FAILED(D2D1CreateFactory(
        D2D1_FACTORY_TYPE_MULTI_THREADED,
        __uuidof(ID2D1Factory),
        &options,
        reinterpret_cast<void**>(m_factory.ReleaseAndGetAddressOf())));

    THROW_IF_FAILED(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(m_dwriteFactory.ReleaseAndGetAddressOf())));
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

    m_renderTarget->BeginDraw();
    m_renderTarget->Clear(D2D1::ColorF(0.05f, 0.05f, 0.07f, m_config->Theme().opacity * 0.85f));

    if (!state.modeLabel.empty())
    {
        D2D1_RECT_F modeRect{ 12.0f, 6.0f, static_cast<FLOAT>(m_width) - 12.0f, 30.0f };
        m_renderTarget->DrawTextW(state.modeLabel.c_str(),
                                  static_cast<UINT32>(state.modeLabel.size()),
                                  m_textFormat.Get(),
                                  modeRect,
                                  m_accentBrush ? m_accentBrush.Get() : m_primaryBrush.Get());
    }

    const auto center = D2D1::Point2F(static_cast<float>(m_width) / 2.0f, static_cast<float>(m_height) / 2.0f);
    const float radius = std::min(m_width, m_height) * 0.45f;

    // Crosshair
    m_renderTarget->DrawLine(D2D1::Point2F(center.x, center.y - radius),
                             D2D1::Point2F(center.x, center.y + radius),
                             m_backgroundBrush.Get(), 1.0f);
    m_renderTarget->DrawLine(D2D1::Point2F(center.x - radius, center.y),
                             D2D1::Point2F(center.x + radius, center.y),
                             m_backgroundBrush.Get(), 1.0f);

    // Outer ring
    m_renderTarget->DrawEllipse(D2D1::Ellipse(center, radius, radius),
                                m_accentBrush ? m_accentBrush.Get() : m_backgroundBrush.Get(),
                                2.5f);

    // Draw all radar hits with individual fade-out
    const auto now = std::chrono::steady_clock::now();
    constexpr float trailSeconds = 1.5f;

    float baseOpacity = m_primaryBrush->GetOpacity();

    {
        std::scoped_lock lock{m_mutex};

        // Remove expired hits
        m_hits.erase(std::remove_if(m_hits.begin(), m_hits.end(),
                       [&](const RadarHit& hit)
                       {
                           float age = std::chrono::duration<float>(now - hit.time).count();
                           return age >= trailSeconds;
                       }),
                     m_hits.end());

        for (const auto& hit : m_hits)
        {
            float age = std::chrono::duration<float>(now - hit.time).count();
            if (age < 0.0f || age >= trailSeconds)
            {
                continue;
            }

            float fade = 1.0f - (age / trailSeconds);

              // Apply detection range scale (distanceScale): 0.5~2.0
              float scale = m_sensitivity.distanceScale;
              if (scale < 0.5f) scale = 0.5f;
              if (scale > 2.0f) scale = 2.0f;

              const float r = radius * std::clamp(hit.radiusFactor * scale, 0.05f, 1.0f);
            const float azimuth = hit.direction.azimuth;
            const float elevation = hit.direction.elevation;

            const float x = std::sin(azimuth) * std::cos(elevation);
            const float z = std::cos(azimuth) * std::cos(elevation);

            const D2D1_POINT_2F p{
                center.x + r * x,
                center.y - r * z,
            };

            const float dotRadius = 4.0f + 2.0f * hit.direction.magnitude;

            m_primaryBrush->SetOpacity(baseOpacity * fade);
            m_renderTarget->FillEllipse(D2D1::Ellipse(p, dotRadius, dotRadius),
                                        m_primaryBrush.Get());
        }
    }

    m_primaryBrush->SetOpacity(baseOpacity);

    // Text uses latest hit direction if available, otherwise current state
    Audio::AudioDirection textDir{};
    {
        std::scoped_lock lock{m_mutex};
        if (!m_hits.empty())
        {
            textDir = m_hits.back().direction;
        }
        else
        {
            textDir = m_state.direction;
        }
    }

    wchar_t buffer[128];
    const wchar_t* name = textDir.dominantSessionName.empty() ? L"" : textDir.dominantSessionName.c_str();
    swprintf_s(buffer, L"Az(horiz) %.0f deg\nEl(vert) %.0f deg\n%ls",
               textDir.azimuth * 180.0f / kPi,
               textDir.elevation * 180.0f / kPi,
               name);

    D2D1_RECT_F textRect{ center.x - radius, center.y + radius * 0.25f, center.x + radius, center.y + radius };
    m_renderTarget->DrawTextW(buffer,
                              static_cast<UINT32>(wcslen(buffer)),
                              m_textFormat.Get(),
                              textRect,
                              m_primaryBrush.Get());

    m_renderTarget->EndDraw();
}

void DirectionVisualizer::UpdateDirection(const Audio::AudioDirection& direction)
{
    std::scoped_lock lock{m_mutex};
    m_state.direction = direction;

    // Record non-background, strong enough hits for radar trail
    if (!direction.isBackground && direction.magnitude > 0.15f)
    {
        // Update reference magnitude for relative near/far feeling
        if (m_referenceMagnitude <= 0.0f)
        {
            m_referenceMagnitude = direction.magnitude;
        }
        else
        {
            // Exponential moving average
            m_referenceMagnitude = 0.7f * m_referenceMagnitude + 0.3f * direction.magnitude;
        }

        float ref = (m_referenceMagnitude > 0.001f) ? m_referenceMagnitude : direction.magnitude;
        float relative = (ref > 0.001f) ? (direction.magnitude / ref) : 1.0f;

        // Map relative magnitude to radius factor: louder -> closer to center
        float radiusFactor = 1.0f - std::clamp(relative, 0.0f, 1.0f); // 0 near, 1 far
        radiusFactor = std::clamp(radiusFactor, 0.15f, 1.0f);

        RadarHit hit;
        hit.direction = direction;
        hit.radiusFactor = radiusFactor;
        hit.time = std::chrono::steady_clock::now();
        m_hits.push_back(hit);
    }
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

void DirectionVisualizer::SetModeLabel(const std::wstring& label)
{
    std::scoped_lock lock{m_mutex};
    m_state.modeLabel = label;
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
    D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps =
        D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(m_width, m_height));

    THROW_IF_FAILED(m_factory->CreateHwndRenderTarget(rtProps, hwndProps, &m_renderTarget));

    const auto primaryColor = ColorFromConfig();
    THROW_IF_FAILED(m_renderTarget->CreateSolidColorBrush(primaryColor, &m_primaryBrush));

    const auto accent = m_config->Theme().accentColor;
    THROW_IF_FAILED(m_renderTarget->CreateSolidColorBrush(
                        D2D1::ColorF(GetRValue(accent) / 255.0f,
                                     GetGValue(accent) / 255.0f,
                                     GetBValue(accent) / 255.0f,
                                     m_config->Theme().opacity * 0.6f),
                        &m_accentBrush));

    THROW_IF_FAILED(m_renderTarget->CreateSolidColorBrush(
                        D2D1::ColorF(0.3f, 0.3f, 0.35f, m_config->Theme().opacity * 0.7f),
                        &m_backgroundBrush));

    THROW_IF_FAILED(m_dwriteFactory->CreateTextFormat(L"Segoe UI",
                                                      nullptr,
                                                      DWRITE_FONT_WEIGHT_REGULAR,
                                                      DWRITE_FONT_STYLE_NORMAL,
                                                      DWRITE_FONT_STRETCH_NORMAL,
                                                      16.0f,
                                                      L"",
                                                      &m_textFormat));
    m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
}

D2D1::ColorF DirectionVisualizer::ColorFromConfig() const
{
    const auto color = m_config->Theme().primaryColor;
    return D2D1::ColorF(GetRValue(color) / 255.0f,
                        GetGValue(color) / 255.0f,
                        GetBValue(color) / 255.0f,
                        m_config->Theme().opacity);
}
