#pragma once

#include <string_view>

namespace STRE::MainMenu
{
inline constexpr std::string_view cEmblemFile = "Branding/emblem.png";
inline constexpr std::string_view cWordmarkFile = "Branding/skyrim-wordmark.png";

struct BrandingOpacity
{
    float Emblem{}, Wordmark{}, Subtitle{};
};

// Render-thread, process-local decoration only; no input or playback state.
class BrandingReveal
{
public:
    void Enter(bool aReturning) noexcept;
    [[nodiscard]] BrandingOpacity Sample(double aNow, bool aBackgroundVisible) noexcept;

private:
    bool m_returning{};
    bool m_started{};
    double m_start{};
};

struct BrandingRect
{
    float X{}, Y{}, Width{}, Height{};
};

struct BrandingLayout
{
    BrandingRect Emblem, Wordmark;
    float CenterX{}, SubtitleY{}, SubtitleMaxWidth{}, FontSize{};
};

// Coordinates relative to the viewport. A centered 16:9 safe area prevents
// ultrawide stretching and leaves the vanilla actions on the right unobscured.
[[nodiscard]] BrandingLayout LayoutBranding(float aWidth, float aHeight, float aEmblemAspect, float aWordmarkAspect) noexcept;
} // namespace STRE::MainMenu
