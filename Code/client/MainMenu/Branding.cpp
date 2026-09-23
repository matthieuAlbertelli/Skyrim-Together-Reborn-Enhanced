#include "Branding.h"

#include <algorithm>
#include <cmath>

namespace STRE::MainMenu
{
void BrandingReveal::Enter(bool aReturning) noexcept
{
    m_returning = aReturning;
    m_started = false;
}

BrandingOpacity BrandingReveal::Sample(double aNow, bool aBackgroundVisible) noexcept
{
    if (!aBackgroundVisible || !std::isfinite(aNow))
        return {};
    if (m_returning)
        return {1, 1, 1};
    if (!m_started)
    {
        m_started = true;
        m_start = aNow;
    }
    const double elapsed = std::max(aNow - m_start, 0.0);
    const auto fade = [elapsed](double aDelay)
    {
        return static_cast<float>(std::clamp((elapsed - aDelay) / 0.45, 0.0, 1.0));
    };
    return {fade(0.15), fade(0.60), fade(1.05)};
}

BrandingLayout LayoutBranding(float aWidth, float aHeight, float aEmblemAspect, float aWordmarkAspect) noexcept
{
    if (!std::isfinite(aWidth) || !std::isfinite(aHeight) || aWidth <= 0 || aHeight <= 0 || aWidth > 16384 || aHeight > 16384)
        return {};
    const float safeWidth = std::min(aWidth, aHeight * (16.0f / 9.0f));
    const float unit = safeWidth * (9.0f / 16.0f);
    const float center = aWidth * 0.25f;
    const float centerY = aHeight * 0.525f;
    const auto fit = [center](float aAspect, float aMaxWidth, float aMaxHeight, float aTop)
    {
        if (!std::isfinite(aAspect) || aAspect <= 0)
            return BrandingRect{};
        const float width = std::min(aMaxWidth, aMaxHeight * aAspect);
        const float height = width / aAspect;
        return BrandingRect{center - width * 0.5f, aTop, width, height};
    };
    return {
        fit(aEmblemAspect, safeWidth * 0.22f, unit * 0.38f, centerY - unit * 0.305f),
        fit(aWordmarkAspect, safeWidth * 0.34f, unit * 0.095f, centerY + unit * 0.12f),
        center,
        centerY,
        centerY + unit * 0.245f,
        safeWidth * 0.42f,
        unit * 0.028f};
}

BrandingRect LayoutBackdrop(float aWidth, float aHeight, float aAspect, const BackdropConfig& aConfig) noexcept
{
    const auto group = LayoutBranding(aWidth, aHeight, 0, 0);
    if (!aConfig.Enabled || group.FontSize <= 0 || !std::isfinite(aAspect) || aAspect <= 0 || !std::isfinite(aConfig.Scale) || !std::isfinite(aConfig.OffsetX) ||
        !std::isfinite(aConfig.OffsetY))
        return {};
    const float scale = std::clamp(aConfig.Scale, 0.5f, 1.5f);
    const float width = std::min(aWidth * 0.48f, aHeight * 0.90f * aAspect) * scale;
    // Keep the complete transparent canvas on-screen and out of the right
    // half, even at extreme settings. Never clip off the feathered perimeter.
    const float fit = std::min({1.0f, aWidth * 0.5f / width, aHeight * aAspect / width});
    const float fittedWidth = std::min(width * fit, aWidth * 0.5f);
    const float fittedHeight = std::min(fittedWidth / aAspect, aHeight);
    const float x = group.CenterX + std::clamp(aConfig.OffsetX, -0.25f, 0.25f) * aWidth;
    const float y = group.CenterY + std::clamp(aConfig.OffsetY, -0.25f, 0.25f) * aHeight;
    return {std::clamp(x - fittedWidth * 0.5f, 0.0f, aWidth * 0.5f - fittedWidth), std::clamp(y - fittedHeight * 0.5f, 0.0f, aHeight - fittedHeight), fittedWidth, fittedHeight};
}
} // namespace STRE::MainMenu
