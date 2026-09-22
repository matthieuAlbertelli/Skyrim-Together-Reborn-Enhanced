#include "../client/MainMenu/Branding.h"

#include <catch2/catch.hpp>
#include <array>
#include <limits>

using namespace STRE::MainMenu;

TEST_CASE("Branding starts with actual background and reveals once per first menu visit", "[main-menu][main-menu-branding]")
{
    BrandingReveal reveal;
    reveal.Enter(false);
    REQUIRE(reveal.Sample(10, false).Emblem == 0);
    REQUIRE(reveal.Sample(40, false).Subtitle == 0); // intro/retained frame/loading do not start it
    REQUIRE(reveal.Sample(50, true).Emblem == 0);
    auto frame = reveal.Sample(50.375, true);
    REQUIRE(frame.Emblem == Approx(0.5));
    REQUIRE(frame.Wordmark == 0);
    REQUIRE(frame.Subtitle == 0);
    frame = reveal.Sample(50.825, true);
    REQUIRE(frame.Emblem == 1);
    REQUIRE(frame.Wordmark == Approx(0.5));
    REQUIRE(frame.Subtitle == 0);
    frame = reveal.Sample(51.275, true);
    REQUIRE(frame.Wordmark == 1);
    REQUIRE(frame.Subtitle == Approx(0.5));
    REQUIRE(reveal.Sample(51.5, true).Subtitle == 1);
    REQUIRE(reveal.Sample(999, true).Subtitle == 1);
    REQUIRE(reveal.Sample(1000, false).Emblem == 0); // error/close omits decoration
    reveal.Enter(true);
    REQUIRE(reveal.Sample(1001, false).Subtitle == 0);
    frame = reveal.Sample(1002, true);
    REQUIRE(frame.Emblem == 1);
    REQUIRE(frame.Wordmark == 1);
    REQUIRE(frame.Subtitle == 1);
}

TEST_CASE("Early menu exit and invalid clocks cannot restart or corrupt branding", "[main-menu][main-menu-branding]")
{
    BrandingReveal reveal;
    reveal.Enter(false);
    REQUIRE(reveal.Sample(std::numeric_limits<double>::quiet_NaN(), true).Emblem == 0);
    REQUIRE(reveal.Sample(20, true).Emblem == 0);
    REQUIRE(reveal.Sample(19, true).Emblem == 0);
    reveal.Enter(true); // exited before first reveal finished
    REQUIRE(reveal.Sample(21, true).Subtitle == 1);
    reveal.Enter(true);
    REQUIRE(reveal.Sample(30, true).Subtitle == 1);
}

TEST_CASE("Branding layout preserves proportions and a coherent safe group across viewports", "[main-menu][main-menu-branding]")
{
    for (const auto size : {std::array<float, 2>{1920, 1080}, {2560, 1080}, {3440, 1440}, {1280, 720}, {800, 600}, {320, 800}})
    {
        const auto layout = LayoutBranding(size[0], size[1], 0.5f, 5.0f);
        REQUIRE(layout.Emblem.Width / layout.Emblem.Height == Approx(0.5));
        REQUIRE(layout.Wordmark.Width / layout.Wordmark.Height == Approx(5.0));
        REQUIRE(layout.Emblem.X + layout.Emblem.Width * 0.5f == Approx(layout.CenterX));
        REQUIRE(layout.Wordmark.X + layout.Wordmark.Width * 0.5f == Approx(layout.CenterX));
        REQUIRE(layout.Emblem.Y + layout.Emblem.Height < layout.Wordmark.Y);
        REQUIRE(layout.Wordmark.Y + layout.Wordmark.Height < layout.SubtitleY);
        REQUIRE(layout.SubtitleY + layout.FontSize < size[1]);
        REQUIRE(layout.CenterX - layout.SubtitleMaxWidth * 0.5f >= 0);
        REQUIRE(layout.CenterX + layout.SubtitleMaxWidth * 0.5f <= size[0]);
    }
    const auto standard = LayoutBranding(1920, 1080, 0.5f, 5);
    const auto wide = LayoutBranding(2560, 1080, 0.5f, 5);
    REQUIRE(wide.Emblem.Width == standard.Emblem.Width);
    REQUIRE(wide.CenterX - standard.CenterX == 320);
    REQUIRE(LayoutBranding(0, 1080, 1, 1).FontSize == 0);
    REQUIRE(LayoutBranding(1920, -1, 1, 1).FontSize == 0);
    REQUIRE(LayoutBranding(std::numeric_limits<float>::infinity(), 1080, 1, 1).FontSize == 0);
    REQUIRE(LayoutBranding(20000, 1080, 1, 1).FontSize == 0);
    REQUIRE(LayoutBranding(1920, 1080, 0, 5).Emblem.Width == 0);
    REQUIRE(LayoutBranding(1920, 1080, 0, 5).Wordmark.Width > 0);
}
