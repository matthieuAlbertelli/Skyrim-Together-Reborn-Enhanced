#include "../client/MainMenu/PresentationPolicy.h"

#include <catch2/catch.hpp>
#include <string>

using namespace STRE::MainMenu;

TEST_CASE("Main menu intro ends or skips once per controller", "[main-menu]")
{
    const bool skip = GENERATE(false, true);
    Controller controller;
    controller.Enter(0, true, true);
    REQUIRE(controller.GetState() == State::PlayingIntro);
    REQUIRE(controller.BlocksMenu());
    controller.Enter(1, true, true); // duplicate show is not a restart
    controller.Update(2, skip ? Playback::Pending : Playback::Ended, skip);
    REQUIRE(controller.GetState() == State::TransitionToMenu);
    REQUIRE_FALSE(controller.BlocksMenu());
    controller.AdvanceTransition(2);
    REQUIRE(controller.GetState() == State::PlayingBackground);
    controller.Leave();
    controller.Enter(10, true, true);
    REQUIRE(controller.GetState() == State::PlayingBackground);
    REQUIRE(controller.IntroAttempted());
}

TEST_CASE("Main menu assets are optional and errors terminate", "[main-menu]")
{
    Controller controller;
    SECTION("missing intro still plays background")
    {
        controller.Enter(0, false, true);
        REQUIRE(controller.GetState() == State::PlayingBackground);
    }
    SECTION("both missing expose vanilla")
    {
        controller.Enter(0, false, false);
        REQUIRE(controller.GetState() == State::VanillaFallback);
    }
    SECTION("broken intro tries background")
    {
        controller.Enter(0, true, true);
        controller.Update(1, Playback::Failed, false);
        controller.AdvanceTransition(1);
        REQUIRE(controller.GetState() == State::PlayingBackground);
    }
    SECTION("missing background exposes vanilla after intro")
    {
        controller.Enter(0, true, false);
        controller.Update(1, Playback::Ended, false);
        controller.AdvanceTransition(1);
        REQUIRE(controller.GetState() == State::VanillaFallback);
    }
    SECTION("broken background is not retried forever")
    {
        controller.Enter(0, false, true);
        controller.Update(1, Playback::Failed, false);
        for (int i = 0; i < 100; ++i)
        {
            controller.Update(2 + i, Playback::Failed, true);
            REQUIRE(controller.GetState() == State::VanillaFallback);
        }
        controller.Leave();
        controller.Enter(120, true, true);
        REQUIRE(controller.GetState() == State::VanillaFallback);
    }
    REQUIRE_FALSE(controller.BlocksMenu());
}

TEST_CASE("Main menu watchdog releases stalled or endless media", "[main-menu]")
{
    Controller controller;
    controller.Enter(0, true, true);
    SECTION("no first frame")
    {
        controller.Update(Controller::cStallSeconds, Playback::Pending, false);
    }
    SECTION("stall after a decoded frame")
    {
        controller.Update(7, Playback::Frame, false);
        controller.Update(14, Playback::Pending, false);
        REQUIRE(controller.BlocksMenu());
        controller.Update(15, Playback::Pending, false);
    }
    SECTION("intro has no EOS but frames continue")
    {
        controller.Update(Controller::cMaximumIntroSeconds, Playback::Frame, false);
    }
    REQUIRE(controller.GetState() == State::TransitionToMenu);
    controller.AdvanceTransition(600);
    controller.Update(608, Playback::Pending, false);
    REQUIRE(controller.GetState() == State::VanillaFallback);
    REQUIRE_FALSE(controller.BlocksMenu());
}

TEST_CASE("Main menu interruption and disabling cannot replay intro", "[main-menu]")
{
    Controller controller;
    controller.Enter(0, true, true);
    controller.Leave();
    controller.Enter(1, true, true);
    REQUIRE(controller.GetState() == State::PlayingBackground);
    controller.Disable();
    controller.Leave();
    controller.Enter(2, true, true);
    REQUIRE(controller.GetState() == State::Disabled);
    REQUIRE_FALSE(controller.BlocksMenu());
    Controller disabled(false);
    disabled.Enter(0, true, true);
    REQUIRE(disabled.GetState() == State::Disabled);
}

TEST_CASE("Main menu config defaults and bounded parsing", "[main-menu]")
{
    const auto defaults = ParseConfig("");
    REQUIRE(defaults.Enabled);
    REQUIRE(defaults.IntroAudio);
    REQUIRE(defaults.SkipKeyboard == 1);
    REQUIRE(defaults.SkipGamepad == 8192);
    auto config = ParseConfig("[Presentation]\r\nEnabled = false\nIntroAudio=false\nSkipKeyboard=57\nSkipGamepad=4096 ; A");
    REQUIRE_FALSE(config.Enabled);
    REQUIRE_FALSE(config.IntroAudio);
    REQUIRE(config.SkipKeyboard == 57);
    REQUIRE(config.SkipGamepad == 4096);
    for (auto invalid : {"-1", "0", "999999999999999999999", "abc", "1junk", "256", ""})
    {
        config = ParseConfig(std::string("[Presentation]\nSkipKeyboard=") + invalid);
        REQUIRE(config.SkipKeyboard == defaults.SkipKeyboard);
    }
    for (auto invalid : {"-1", "0", "3", "65536", "1.5", "nan"})
    {
        config = ParseConfig(std::string("[Presentation]\nSkipGamepad=") + invalid);
        REQUIRE(config.SkipGamepad == defaults.SkipGamepad);
    }
    config = ParseConfig("[Other]\nEnabled=false\n[Presentation]\nEnabled=oops\nIntroAudio=0");
    REQUIRE(config.Enabled);
    REQUIRE(config.IntroAudio);
    REQUIRE(ParseConfig(std::string(4097, 'x')).Enabled);
    REQUIRE(cAssetDirectory == "STRE/MainMenu");
    REQUIRE(cIntroFile == "intro.mp4");
    REQUIRE(cBackgroundFile == "background.mp4");
}

TEST_CASE("Main menu input swallows held buttons through release", "[main-menu]")
{
    InputLatch latch;
    const std::uint32_t device = GENERATE(0u, 1u, 2u);
    const std::uint32_t key = device == 2 ? 8192 : 1;
    REQUIRE(latch.Button(true, device, key, true));
    REQUIRE(latch.Button(false, device, key, true));
    REQUIRE_FALSE(latch.Button(false, device, key + 1, true));
    REQUIRE(latch.Button(false, device, key, false));
    REQUIRE_FALSE(latch.Button(false, device, key, true));
    REQUIRE(latch.Button(true, 99, 999999, true));
    REQUIRE_FALSE(latch.Button(false, 99, 999999, true));
    latch.Clear();
    REQUIRE_FALSE(latch.Button(false, device, key, false));
}
