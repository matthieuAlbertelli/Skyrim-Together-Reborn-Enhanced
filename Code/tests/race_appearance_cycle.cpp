#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>
#include <optional>
#include <CharacterCreation/AppearanceApply.h>
#include <catch2/catch.hpp>

using namespace STRE::CharacterCreation;

TEST_CASE("Race activation remains exact-runtime restricted", "[race-appearance]")
{
    REQUIRE(RaceSwitchRuntimeSupported("1.6.1170.0"));
    REQUIRE_FALSE(RaceSwitchRuntimeSupported("1.6.640.0"));
    REQUIRE_FALSE(RaceSwitchRuntimeSupported("1.6.1170"));
    REQUIRE_FALSE(RaceSwitchPlayerArgument);
}

TEST_CASE("Race switch postconditions reject old races and changed identity but not diagnostic weight", "[race-appearance]")
{
    AppearancePostState before;
    before.ActorPointer = 10;
    before.BasePointer = 20;
    before.ProvenanceBaseId = 30;
    before.RuntimeRace = before.BaseRace = 1;
    RaceAppearanceCycle cycle;
    cycle.Begin(2, before);
    auto actual = cycle.Expected;
    auto check = [&]
    {
        return CheckAppearancePostState(before, cycle.Expected, actual, [](const char*, auto, auto, auto, bool) {});
    };
    REQUIRE(check());
    SECTION("runtime race old")
    {
        actual.RuntimeRace = 1;
    }
    SECTION("base race old after deserialize")
    {
        actual.BaseRace = 1;
    }
    SECTION("actor replaced")
    {
        ++actual.ActorPointer;
    }
    SECTION("private base replaced")
    {
        ++actual.BasePointer;
    }
    SECTION("provenance changed")
    {
        ++actual.ProvenanceBaseId;
    }
    SECTION("weight only")
    {
        actual.Weight = 75;
        REQUIRE(check());
        return;
    }
    REQUIRE_FALSE(check());
    REQUIRE(cycle.RequestSwitch());
    REQUIRE_FALSE(cycle.Returned(check()));
    REQUIRE(cycle.State == RaceAppearanceCycle::Stage::Failed);
    REQUIRE_FALSE(cycle.Deserialized(true));
}

TEST_CASE("Race cycle requires deferred root and head readiness independent of completion event", "[race-appearance]")
{
    AppearanceApply apply;
    NotifyCharacterAppearanceUpdate first;
    first.AppearanceBuffer = "active";
    apply.Receive(first);
    REQUIRE(apply.Begin({1, 2, 3}));
    apply.Race.Begin(2, {});
    REQUIRE_FALSE(apply.RequestReset());
    REQUIRE(apply.Race.RequestSwitch());
    REQUIRE_FALSE(apply.Race.RequestSwitch());
    REQUIRE_FALSE(apply.CompleteTints(true));
    REQUIRE(apply.Race.Returned(true));
    REQUIRE(apply.Race.RequestDeserialize());
    REQUIRE(apply.Race.Deserialized(true));
    REQUIRE(apply.RequestRaceReset({1, 2, 3}));
    REQUIRE_FALSE(apply.RequestRaceReset({9, 9, 9}));
    REQUIRE(apply.RaceResetReturned(true));
    SECTION("event received")
    {
        apply.Race.EventSeen = true;
    }
    SECTION("no event")
    {
        REQUIRE_FALSE(apply.Race.EventSeen);
    }
    REQUIRE_FALSE(apply.ObserveHead({1, 2, 3}));

    REQUIRE_FALSE(apply.ObserveHead({0, 4, 5}));
    REQUIRE_FALSE(apply.ObserveHead({6, 0, 0}));
    REQUIRE(apply.Race.State == RaceAppearanceCycle::Stage::WaitingForRaceHead);

    REQUIRE_FALSE(apply.CompleteTints(true));
    first.AppearanceBuffer = "followup";
    apply.Receive(first);
    first.AppearanceBuffer = "latest";
    apply.Receive(first);
    REQUIRE(apply.Active->AppearanceBuffer == "active");
    REQUIRE(apply.Latest->AppearanceBuffer == "latest");
    REQUIRE(apply.ObserveHead({6, 7, 8}));
    REQUIRE(apply.Race.State == RaceAppearanceCycle::Stage::ApplyingTints);
    REQUIRE_FALSE(apply.CompleteTints(false));
    REQUIRE(apply.CompleteTints(true));
    REQUIRE(apply.Race.State == RaceAppearanceCycle::Stage::Applied);
    REQUIRE(apply.Begin({6, 7, 8}));
    REQUIRE_FALSE(apply.Race.Enabled);
    REQUIRE(apply.RequestReset()); // 4E is still usable for the next same-race snapshot.
}

TEST_CASE("Race timeout and observed inventory loss fail closed without retry", "[race-appearance]")
{
    AppearanceApply apply;
    apply.Receive({});
    REQUIRE(apply.Begin({1, 2, 3}));
    apply.Race.Begin(2, {});
    REQUIRE(apply.Race.RequestSwitch());
    REQUIRE(apply.Race.Returned(true));
    REQUIRE(apply.Race.RequestDeserialize());
    REQUIRE(apply.Race.Deserialized(true));
    REQUIRE(apply.RequestRaceReset({1, 2, 3}));
    REQUIRE_FALSE(apply.RequestRaceReset({9, 9, 9}));
    REQUIRE(apply.RaceResetReturned(true));
    for (uint32_t tick = 0; tick < AppearanceApply::MaxTicks; ++tick)
        REQUIRE_FALSE(apply.ObserveHead({1, 2, 3}));
    REQUIRE_FALSE(apply.ObserveHead({6, 7, 8}));
    REQUIRE(apply.Ticks == 120);
    apply.Race.InventoryCount = 20;
    apply.Race.EquippedCount = 2;
    REQUIRE_FALSE(apply.Race.InventoryLost(20, 2));
    REQUIRE(apply.Race.InventoryLost(19, 2));
    REQUIRE(apply.Race.InventoryLost(20, 1));
    apply.Finish(false);
    REQUIRE(apply.Race.State == RaceAppearanceCycle::Stage::Failed);
    REQUIRE(apply.BlockFaceGen);
    REQUIRE_FALSE(apply.RequestReset());
    REQUIRE_FALSE(apply.Race.RequestSwitch());
}

TEST_CASE("Race reset is only allowed after successful deserialization and cannot retry failure", "[race-appearance]")
{
    AppearanceApply apply;
    apply.Receive({});
    REQUIRE(apply.Begin({10, 20, 30}));
    apply.Race.Begin(2, {});
    REQUIRE_FALSE(apply.RequestRaceReset({1, 2, 3}));
    REQUIRE(apply.Race.RequestSwitch());
    REQUIRE_FALSE(apply.RequestRaceReset({1, 2, 3}));
    REQUIRE(apply.Race.Returned(true));
    REQUIRE(apply.Race.RequestDeserialize());
    REQUIRE_FALSE(apply.RequestRaceReset({1, 2, 3}));
    SECTION("failed deserialize")
    {
        REQUIRE_FALSE(apply.Race.Deserialized(false));
        REQUIRE_FALSE(apply.RequestRaceReset({1, 2, 3}));
    }
    SECTION("reset fails")
    {
        REQUIRE(apply.Race.Deserialized(true));
        REQUIRE(apply.RequestRaceReset({1, 2, 3}));
        REQUIRE(apply.Before.Head == 3);
        REQUIRE(apply.Ticks == 0);
        REQUIRE_FALSE(apply.RaceResetReturned(false));
        REQUIRE_FALSE(apply.Active);
        REQUIRE(apply.BlockFaceGen);
        REQUIRE(apply.Race.State == RaceAppearanceCycle::Stage::Failed);
        REQUIRE_FALSE(apply.RequestRaceReset({1, 2, 3}));
        REQUIRE_FALSE(apply.CompleteTints(true));
    }
}

TEST_CASE("Race readiness requires a new root and changed face or head relative to pre-reset", "[race-appearance]")
{
    AppearanceApply apply;
    apply.Receive({});
    REQUIRE(apply.Begin({10, 20, 30}));
    apply.Race.Begin(2, {});
    REQUIRE(apply.Race.RequestSwitch());
    REQUIRE(apply.Race.Returned(true));
    REQUIRE(apply.Race.RequestDeserialize());
    REQUIRE(apply.Race.Deserialized(true));
    REQUIRE(apply.RequestRaceReset({1, 2, 3}));
    REQUIRE(apply.RaceResetReturned(true));
    REQUIRE_FALSE(apply.ObserveHead({1, 2, 3}));
    REQUIRE_FALSE(apply.ObserveHead({0, 4, 5}));
    REQUIRE_FALSE(apply.ObserveHead({6, 0, 5}));
    REQUIRE_FALSE(apply.ObserveHead({6, 4, 0}));
    REQUIRE_FALSE(apply.CompleteTints(true));
    bool ready = true;
    SECTION("root only")
    {
        REQUIRE_FALSE(apply.ObserveHead({6, 2, 3}));
        ready = false;
    }
    SECTION("face only")
    {
        REQUIRE_FALSE(apply.ObserveHead({1, 4, 3}));
        ready = false;
    }
    SECTION("head only")
    {
        REQUIRE_FALSE(apply.ObserveHead({1, 2, 5}));
        ready = false;
    }
    SECTION("root and face")
    {
        REQUIRE(apply.ObserveHead({6, 4, 3}));
    }
    SECTION("root and head")
    {
        REQUIRE(apply.ObserveHead({6, 2, 5}));
    }
    SECTION("all changed")
    {
        REQUIRE(apply.ObserveHead({6, 4, 5}));
    }
    REQUIRE_FALSE(apply.RequestRaceReset({1, 2, 3}));
    REQUIRE(apply.CompleteTints(true) == ready);
}
