#include <catch2/catch.hpp>
#include <Dialogue/ProgressGate.h>

using namespace STRE::Dialogue;
using namespace std::chrono_literals;

namespace
{
Observation Stre(bool aAllow = false)
{
    return {{1, 2, 3, 4, 0x040D6BC3, 5, 6}, cPlugin, true, aAllow};
}
const ProgressGate::Clock::time_point cStart{};
} // namespace

TEST_CASE("Dialogue progress never writes outside the STRE boundary", "[dialogue-gate]")
{
    ProgressGate gate;
    auto sample = Stre();
    REQUIRE(gate.Observe(sample, cStart) == GateAction::Arm);
    SECTION("vanilla")
    {
        sample.Plugin = "Skyrim.esm";
    }
    SECTION("another plugin")
    {
        sample.Plugin = "Other.esp";
    }
    SECTION("no menu")
    {
        sample.Key.Menu = 0;
    }
    SECTION("closed menu")
    {
        sample.MenuOpen = false;
    }
    SECTION("no movie")
    {
        sample.Key.Movie = 0;
    }
    SECTION("null INFO")
    {
        sample.Key.Info = 0;
    }
    SECTION("unknown form")
    {
        sample.Key.FormId = 0;
    }
    SECTION("missing or non-Boolean GFx variable")
    {
        sample.AllowProgress.reset();
    }
    CHECK(gate.Observe(sample, cStart + 1s) == GateAction::None);
    CHECK(gate.Observe(Stre(), cStart + 2s) == GateAction::Arm);
    CHECK(gate.Observe(Stre(), cStart + 2749ms) == GateAction::None);
}

TEST_CASE("Dialogue progress waits the full monotonic 750 ms", "[dialogue-gate]")
{
    ProgressGate gate;
    REQUIRE(gate.Observe(Stre(), cStart) == GateAction::Arm);
    for (int ms : {1, 100, 749})
        CHECK(gate.Observe(Stre(), cStart + std::chrono::milliseconds(ms)) == GateAction::None);
    CHECK(gate.Observe(Stre(), cStart + 750ms) == GateAction::Enable);
    // Without confirmed success there is no repeated write/log every frame.
    CHECK(gate.Observe(Stre(), cStart + 751ms) == GateAction::None);
    CHECK(gate.Observe(Stre(), cStart + 1h) == GateAction::None);
}

TEST_CASE("Already allowed dialogue produces no action", "[dialogue-gate]")
{
    ProgressGate gate;
    CHECK(gate.Observe(Stre(true), cStart) == GateAction::None);
    CHECK(gate.Observe(Stre(true), cStart + 1h) == GateAction::None);
    REQUIRE(gate.Observe(Stre(), cStart + 2h) == GateAction::Arm);
    CHECK(gate.Observe(Stre(true), cStart + 2h + 750ms) == GateAction::None);
    CHECK(gate.Observe(Stre(), cStart + 3h) == GateAction::Arm);
}

TEST_CASE("Every vanilla skip can arm a fresh segment without double progress", "[dialogue-gate]")
{
    ProgressGate gate;
    for (int segment = 0; segment < 6; ++segment)
    {
        const auto now = cStart + std::chrono::seconds(segment * 2);
        REQUIRE(gate.Observe(Stre(), now) == GateAction::Arm);
        REQUIRE(gate.Observe(Stre(), now + 749ms) == GateAction::None);
        REQUIRE(gate.Observe(Stre(), now + 750ms) == GateAction::Enable);
        gate.ConfirmEnabled();
        // Includes a skip between frames, without an observed true sample.
        if (segment % 2 == 0)
            CHECK(gate.Observe(Stre(true), now + 751ms) == GateAction::None);
    }
}

TEST_CASE("Dialogue response identity changes discard the previous deadline", "[dialogue-gate]")
{
    ProgressGate gate;
    auto sample = Stre();
    REQUIRE(gate.Observe(sample, cStart) == GateAction::Arm);
    SECTION("INFO pointer")
    {
        ++sample.Key.Info;
    }
    SECTION("INFO FormID")
    {
        ++sample.Key.FormId;
    }
    SECTION("response segment")
    {
        ++sample.Key.Response;
    }
    SECTION("speaker")
    {
        ++sample.Key.Speaker;
    }
    SECTION("menu")
    {
        ++sample.Key.Menu;
    }
    SECTION("movie")
    {
        ++sample.Key.Movie;
    }
    SECTION("session")
    {
        ++sample.Key.Session;
    }
    CHECK(gate.Observe(sample, cStart + 1s) == GateAction::Arm);
    CHECK(gate.Observe(sample, cStart + 1749ms) == GateAction::None);
    CHECK(gate.Observe(sample, cStart + 1750ms) == GateAction::Enable);
}

TEST_CASE("Explicit close or session reset invalidates a pending write", "[dialogue-gate]")
{
    ProgressGate gate;
    REQUIRE(gate.Observe(Stre(), cStart) == GateAction::Arm);
    gate.Reset();
    CHECK(gate.Observe(Stre(), cStart + 1s) == GateAction::Arm);
    CHECK(gate.Observe(Stre(), cStart + 1749ms) == GateAction::None);
}

TEST_CASE("A backwards time observation cannot open the dialogue gate", "[dialogue-gate]")
{
    ProgressGate gate;
    REQUIRE(gate.Observe(Stre(), cStart + 1s) == GateAction::Arm);
    CHECK(gate.Observe(Stre(), cStart) == GateAction::Arm);
    CHECK(gate.Observe(Stre(), cStart + 749ms) == GateAction::None);
}
