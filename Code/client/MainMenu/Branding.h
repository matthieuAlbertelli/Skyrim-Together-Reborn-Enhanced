#pragma once

#include <string_view>

namespace STRE::MainMenu
{
inline constexpr std::string_view cEmblemFile = "Branding/emblem.png";
inline constexpr std::string_view cWordmarkFile = "Branding/skyrim-wordmark.png";
inline constexpr std::string_view cBackdropFile = "branding_backdrop.png";

struct BackdropConfig
{
    bool Enabled{true};
    float Opacity{0.35f};
    float Scale{1.0f};
    float OffsetX{}, OffsetY{}; // Fractions of viewport width/height.
};

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
    float CenterX{}, CenterY{}, SubtitleY{}, SubtitleMaxWidth{}, FontSize{};
};

// Coordinates relative to the viewport. Anchor in the left half; cap size at
// 16:9 proportions rather than stretching the group on ultrawide displays.
[[nodiscard]] BrandingLayout LayoutBranding(float aWidth, float aHeight, float aEmblemAspect, float aWordmarkAspect) noexcept;
[[nodiscard]] BrandingRect LayoutBackdrop(float aWidth, float aHeight, float aAspect, const BackdropConfig& aConfig) noexcept;
} // namespace STRE::MainMenu
