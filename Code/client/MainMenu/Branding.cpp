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
    const float center = (aWidth - safeWidth) * 0.5f + safeWidth * 0.38f;
    const auto fit = [center](float aAspect, float aMaxWidth, float aMaxHeight, float aTop)
    {
        if (!std::isfinite(aAspect) || aAspect <= 0)
            return BrandingRect{};
        const float width = std::min(aMaxWidth, aMaxHeight * aAspect);
        const float height = width / aAspect;
        return BrandingRect{center - width * 0.5f, aTop, width, height};
    };
    return {
        fit(aEmblemAspect, safeWidth * 0.26f, aHeight * 0.46f, aHeight * 0.10f),
        fit(aWordmarkAspect, safeWidth * 0.36f, aHeight * 0.11f, aHeight * 0.60f),
        center,
        aHeight * 0.74f,
        safeWidth * 0.42f,
        std::min(aHeight, safeWidth * (9.0f / 16.0f)) * 0.028f};
}
} // namespace STRE::MainMenu
