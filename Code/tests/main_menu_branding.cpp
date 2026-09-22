#include "../client/MainMenu/Branding.h"
#include "../client/MainMenu/PresentationPolicy.h"

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
        REQUIRE(layout.CenterX + layout.SubtitleMaxWidth * 0.5f <= size[0] * 0.5f);
        REQUIRE(layout.Emblem.X >= 0);
        REQUIRE(layout.Wordmark.X + layout.Wordmark.Width <= size[0] * 0.5f);
        const float visualCenter = (layout.Emblem.Y + layout.SubtitleY + layout.FontSize) * 0.5f;
        REQUIRE(visualCenter >= size[1] * 0.50f);
        REQUIRE(visualCenter <= size[1] * 0.55f);
    }
    const auto standard = LayoutBranding(1920, 1080, 0.5f, 5);
    const auto wide = LayoutBranding(2560, 1080, 0.5f, 5);
    REQUIRE(wide.Emblem.Width == standard.Emblem.Width);
    REQUIRE(standard.CenterX == 480);
    REQUIRE(wide.CenterX == 640);
    REQUIRE(LayoutBranding(0, 1080, 1, 1).FontSize == 0);
    REQUIRE(LayoutBranding(1920, -1, 1, 1).FontSize == 0);
    REQUIRE(LayoutBranding(std::numeric_limits<float>::infinity(), 1080, 1, 1).FontSize == 0);
    REQUIRE(LayoutBranding(20000, 1080, 1, 1).FontSize == 0);
    REQUIRE(LayoutBranding(1920, 1080, 0, 5).Emblem.Width == 0);
    REQUIRE(LayoutBranding(1920, 1080, 0, 5).Wordmark.Width > 0);
}

TEST_CASE("Backdrop configuration is bounded and does not change presentation controls", "[main-menu][main-menu-branding]")
{
    const auto defaults = ParseConfig("");
    REQUIRE(defaults.Backdrop.Enabled);
    REQUIRE(defaults.Backdrop.Opacity == Approx(0.35));
    const auto config = ParseConfig("[Branding]\nBackdropEnabled=false\nBackdropOpacity=0.42\nBackdropScale=1.2\nBackdropOffsetX=-0.05\nBackdropOffsetY=0.1\n"
                                    "SkipKeyboard=57\nEnabled=false\n[Presentation]\nIntroAudio=false\nBackdropOpacity=0.9\n");
    REQUIRE_FALSE(config.Backdrop.Enabled);
    REQUIRE(config.Backdrop.Opacity == Approx(0.42));
    REQUIRE(config.Backdrop.Scale == Approx(1.2));
    REQUIRE(config.Backdrop.OffsetX == Approx(-0.05));
    REQUIRE(config.Backdrop.OffsetY == Approx(0.1));
    REQUIRE(config.Enabled);
    REQUIRE(config.SkipKeyboard == defaults.SkipKeyboard);
    REQUIRE_FALSE(config.IntroAudio);
    for (const auto* invalid : {"nan", "inf", "-inf", "1e99", "0.3junk", "", "2", "-1"})
    {
        const std::string value = invalid;
        const auto bad = ParseConfig("[Branding]\nBackdropOpacity=" + value + "\nBackdropScale=" + value + "\nBackdropOffsetX=" + value + "\n");
        REQUIRE(bad.Backdrop.Opacity == defaults.Backdrop.Opacity);
        REQUIRE(bad.Backdrop.Scale == defaults.Backdrop.Scale);
        REQUIRE(bad.Backdrop.OffsetX == defaults.Backdrop.OffsetX);
    }
    REQUIRE(ParseConfig("[Branding]\nBackdropEnabled=yes").Backdrop.Enabled);
    REQUIRE(ParseConfig("[Branding]\nBackdropOpacity=0").Backdrop.Opacity == 0);
    REQUIRE(ParseConfig("[Branding]\nBackdropOpacity=1\nBackdropScale=1.5\nBackdropOffsetY=-0.25").Backdrop.OffsetY == -0.25f);
    REQUIRE(ParseConfig(std::string(4097, 'x')).Backdrop.Opacity == defaults.Backdrop.Opacity);
}

TEST_CASE("Backdrop keeps aspect and the full feathered canvas inside the left half", "[main-menu][main-menu-branding]")
{
    for (const auto size : {std::array<float, 2>{1920, 1080}, {2560, 1080}, {3440, 1440}, {800, 600}, {320, 800}})
        for (const float aspect : {0.75f, 1.0f, 1.5f})
            for (const float scale : {0.5f, 1.0f, 1.5f})
                for (const float offset : {-0.25f, 0.0f, 0.25f})
                {
                    BackdropConfig config;
                    config.Scale = scale;
                    config.OffsetX = config.OffsetY = offset;
                    const auto rect = LayoutBackdrop(size[0], size[1], aspect, config);
                    REQUIRE(rect.Width / rect.Height == Approx(aspect));
                    REQUIRE(rect.X >= 0);
                    REQUIRE(rect.Y >= 0);
                    REQUIRE(rect.X + rect.Width <= size[0] * 0.5f + 0.001f);
                    REQUIRE(rect.Y + rect.Height <= size[1] + 0.001f);
                }
    BackdropConfig config;
    const auto group = LayoutBranding(1920, 1080, 0.5f, 5);
    const auto rect = LayoutBackdrop(1920, 1080, 1, config);
    REQUIRE(rect.X + rect.Width * 0.5f == Approx(group.CenterX));
    REQUIRE(rect.Y + rect.Height * 0.5f == Approx(group.CenterY));
    config.Enabled = false;
    REQUIRE(LayoutBackdrop(1920, 1080, 1, config).Width == 0);
    config.Enabled = true;
    REQUIRE(LayoutBackdrop(1920, 1080, 0, config).Width == 0); // absent texture
    REQUIRE(LayoutBackdrop(0, 1080, 1, config).Width == 0);
    config.Scale = std::numeric_limits<float>::quiet_NaN();
    REQUIRE(LayoutBackdrop(1920, 1080, 1, config).Width == 0);
}
