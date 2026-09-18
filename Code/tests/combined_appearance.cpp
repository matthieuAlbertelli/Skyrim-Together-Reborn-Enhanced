#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>
#include <CharacterCreation/AppearanceApply.h>
#include <catch2/catch.hpp>

using namespace STRE::CharacterCreation;
using CombinedStage = CombinedAppearanceCycle::Stage;

namespace
{
CombinedAppearanceCycle cycle()
{
    CombinedAppearanceCycle c;
    AppearancePostState source;
    source.ActorPointer = 10;
    source.BasePointer = 20;
    source.ProvenanceBaseId = 30;
    source.RuntimeRace = source.BaseRace = 1;
    source.Sex = 0;
    c.Begin(source, 2, 1, 50.f);
    return c;
}
bool check(const AppearancePostState& wanted, const AppearancePostState& actual)
{
    return CheckAppearancePostState(wanted, wanted, actual, [](const char*, auto, auto, auto, bool) {});
}
void reachRaceWait(CombinedAppearanceCycle& c)
{
    REQUIRE(c.RequestSwitch());
    REQUIRE(c.VerifyRace(true));
    REQUIRE(c.RequestRaceReset({1, 2, 3}));
    REQUIRE(c.RaceResetReturned(true));
}
void reachFinalWait(CombinedAppearanceCycle& c)
{
    reachRaceWait(c);
    REQUIRE(c.ObserveRace({4, 5, 6}, 1));
    REQUIRE(c.RequestFinalDeserialize(2));
    REQUIRE(c.VerifyFinal(true));
    REQUIRE(c.RequestFinalReset({4, 5, 0}));
    REQUIRE(c.FinalResetReturned(true));
}
} // namespace

TEST_CASE("Combined reserves two sequential rebuilds and a single final tint cycle", "[combined-appearance]")
{
    auto c = cycle();
    REQUIRE_FALSE(c.RequestFinalDeserialize(2));
    REQUIRE_FALSE(c.RequestRaceReset({}));
    REQUIRE_FALSE(c.RequestFinalReset({}));
    REQUIRE_FALSE(c.RequestFaceGen());
    REQUIRE(c.RequestSwitch());
    REQUIRE_FALSE(c.RequestSwitch());
    REQUIRE_FALSE(c.RequestRaceReset({}));
    REQUIRE(c.VerifyRace(true));
    REQUIRE_FALSE(c.RequestFinalDeserialize(2));
    REQUIRE(c.RequestRaceReset({1, 2, 3}));
    REQUIRE_FALSE(c.RequestRaceReset({1, 2, 3}));
    REQUIRE_FALSE(c.ObserveRace({4, 5, 6}, 1));
    REQUIRE(c.RaceResetReturned(true));
    REQUIRE(c.Expected().Sex == 0);
    REQUIRE(c.Expected().RuntimeRace == 2);
    REQUIRE_FALSE(c.RequestFinalDeserialize(2));
    REQUIRE_FALSE(c.RequestFaceGen());
    REQUIRE(c.ObserveRace({4, 5, 6}, 1));
    REQUIRE(c.RequestFinalDeserialize(2));
    REQUIRE_FALSE(c.RequestFinalDeserialize(2));
    REQUIRE(c.Expected().Sex == 1);
    REQUIRE_FALSE(c.RequestFinalReset({}));
    REQUIRE(c.VerifyFinal(true));
    REQUIRE(c.RequestFinalReset({4, 5, 0}));
    REQUIRE(c.Ticks == 0);
    REQUIRE_FALSE(c.RequestFinalReset({}));
    REQUIRE_FALSE(c.ObserveFinal({7, 8, 9}));
    REQUIRE(c.FinalResetReturned(true));
    REQUIRE_FALSE(c.RequestFaceGen());
    REQUIRE(c.ObserveFinal({7, 8, 9}));
    REQUIRE(c.RequestFaceGen());
    REQUIRE_FALSE(c.RequestFaceGen());
    REQUIRE_FALSE(c.Complete(false));
    REQUIRE(c.Complete(true));
    REQUIRE_FALSE(c.Complete(true));
    REQUIRE(c.SwitchCount == 1);
    REQUIRE(c.DeserializeCount == 1);
    REQUIRE(c.ResetCount == 2);
    REQUIRE(c.FaceGenCount == 1);
    REQUIRE(c.State == CombinedStage::Applied);
    REQUIRE_FALSE(c.RequestSwitch());
    REQUIRE_FALSE(c.RequestRaceReset({}));
    REQUIRE_FALSE(c.RequestFinalReset({}));
}

TEST_CASE("Combined switch or race postcheck failure forbids reset and final bytes", "[combined-appearance]")
{
    auto c = cycle();
    REQUIRE(c.RequestSwitch());
    auto actual = c.RaceExpected;
    bool issued = true;
    SECTION("native SwitchRace unavailable")
    {
        issued = false;
    }
    SECTION("wrong runtime race")
    {
        actual.RuntimeRace = 1;
    }
    SECTION("wrong base race")
    {
        actual.BaseRace = 1;
    }
    SECTION("source sex changed early")
    {
        actual.Sex = 1;
    }
    SECTION("identity lost")
    {
        ++actual.ActorPointer;
    }
    SECTION("provenance lost")
    {
        ++actual.ProvenanceBaseId;
    }
    REQUIRE_FALSE(c.VerifyRace(issued && check(c.RaceExpected, actual)));
    REQUIRE_FALSE(c.RequestRaceReset({}));
    REQUIRE_FALSE(c.RequestFinalDeserialize(2));
    REQUIRE(c.ResetCount == 0);
    REQUIRE(c.DeserializeCount == 0);
}

TEST_CASE("Combined race reset failure cannot consume final appearance", "[combined-appearance]")
{
    auto c = cycle();
    REQUIRE(c.RequestSwitch());
    REQUIRE(c.VerifyRace(true));
    REQUIRE(c.RequestRaceReset({1, 2, 3}));
    REQUIRE_FALSE(c.RaceResetReturned(false));
    REQUIRE_FALSE(c.RequestRaceReset({}));
    REQUIRE_FALSE(c.RequestFinalDeserialize(2));
    REQUIRE_FALSE(c.RequestFinalReset({}));
    REQUIRE(c.ResetCount == 1);
    REQUIRE(c.DeserializeCount == 0);
}

TEST_CASE("Combined race-stage timeout forbids Deserialize and reset two", "[combined-appearance]")
{
    auto c = cycle();
    reachRaceWait(c);
    for (uint32_t tick = 0; tick < CombinedAppearanceCycle::MaxStageTicks; ++tick)
    {
        REQUIRE(c.Tick());
        REQUIRE_FALSE(c.ObserveRace({4, 5, 0}, 1));
        REQUIRE_FALSE(c.RequestFinalDeserialize(2));
    }
    REQUIRE_FALSE(c.Tick());
    REQUIRE(c.State == CombinedStage::Failed);
    REQUIRE_FALSE(c.RequestFinalReset({}));
    REQUIRE_FALSE(c.RequestFaceGen());
    REQUIRE(c.DeserializeCount == 0);
    REQUIRE(c.ResetCount == 1);
    REQUIRE(c.FaceGenCount == 0);
}

TEST_CASE("Combined readiness uses each phase's own pre-reset geometry", "[combined-appearance]")
{
    auto c = cycle();
    reachRaceWait(c);
    for (auto nodes : {AppearanceNodes{4, 2, 3}, {1, 5, 3}, {1, 2, 6}, {4, 5, 0}, {4, 0, 6}, {0, 5, 6}})
        REQUIRE_FALSE(c.ObserveRace(nodes, 1));
    REQUIRE(c.ObserveRace({4, 5, 6}, 1));
    REQUIRE(c.RequestFinalDeserialize(2));
    REQUIRE(c.VerifyFinal(true));
    REQUIRE(c.RequestFinalReset({4, 5, 6}));
    REQUIRE(c.FinalResetReturned(true));
    REQUIRE_FALSE(c.ObserveFinal({4, 5, 6})); // New versus reset1 is insufficient.
    REQUIRE_FALSE(c.ObserveFinal({7, 5, 6}));
    REQUIRE_FALSE(c.ObserveFinal({4, 8, 9}));
    REQUIRE_FALSE(c.ObserveFinal({7, 8, 0}));
    SECTION("new root plus face")
    {
        REQUIRE(c.ObserveFinal({7, 8, 6}));
    }
    SECTION("new root plus head")
    {
        REQUIRE(c.ObserveFinal({7, 5, 9}));
    }
    SECTION("all renewed")
    {
        REQUIRE(c.ObserveFinal({7, 8, 9}));
    }
}

TEST_CASE("Combined final Deserialize postcheck failure cannot reset twice", "[combined-appearance]")
{
    auto c = cycle();
    reachRaceWait(c);
    REQUIRE(c.ObserveRace({4, 5, 6}, 1));
    REQUIRE(c.RequestFinalDeserialize(2));
    auto actual = c.FinalExpected;
    SECTION("sex stayed at source")
    {
        actual.Sex = 0;
    }
    SECTION("race not preserved")
    {
        actual.BaseRace = 1;
    }
    SECTION("identity lost")
    {
        ++actual.BasePointer;
    }
    SECTION("provenance lost")
    {
        ++actual.ProvenanceBaseId;
    }
    REQUIRE_FALSE(c.VerifyFinal(check(c.FinalExpected, actual)));
    REQUIRE_FALSE(c.RequestFinalDeserialize(2));
    REQUIRE_FALSE(c.RequestFinalReset({}));
    REQUIRE_FALSE(c.RequestFaceGen());
    REQUIRE(c.ResetCount == 1);
    REQUIRE(c.DeserializeCount == 1);
}

TEST_CASE("Combined reset two failure cannot start tints or retry", "[combined-appearance]")
{
    auto c = cycle();
    reachRaceWait(c);
    REQUIRE(c.ObserveRace({4, 5, 6}, 1));
    REQUIRE(c.RequestFinalDeserialize(2));
    REQUIRE(c.VerifyFinal(true));
    REQUIRE(c.RequestFinalReset({4, 5, 0}));
    REQUIRE_FALSE(c.FinalResetReturned(false));
    REQUIRE_FALSE(c.RequestFinalReset({}));
    REQUIRE_FALSE(c.RequestFaceGen());
    REQUIRE(c.ResetCount == 2);
    REQUIRE(c.FaceGenCount == 0);
}

TEST_CASE("Combined final head and FaceGen timeouts are bounded without a third reset", "[combined-appearance]")
{
    auto c = cycle();
    reachFinalWait(c);
    SECTION("head never returns")
    {
        for (uint32_t i = 0; i < CombinedAppearanceCycle::MaxStageTicks; ++i)
        {
            REQUIRE(c.Tick());
            REQUIRE_FALSE(c.ObserveFinal({7, 8, 0}));
        }
        REQUIRE(c.FaceGenCount == 0);
    }
    SECTION("material never generates")
    {
        REQUIRE(c.ObserveFinal({7, 8, 9}));
        REQUIRE(c.RequestFaceGen());
        for (uint32_t i = 0; i < CombinedAppearanceCycle::MaxStageTicks; ++i)
        {
            REQUIRE(c.Tick());
            REQUIRE_FALSE(c.Complete(false));
        }
        REQUIRE(c.FaceGenCount == 1);
    }
    REQUIRE_FALSE(c.Tick());
    REQUIRE_FALSE(c.RequestFinalReset({}));
    REQUIRE(c.ResetCount == 2);
}

TEST_CASE("Combined phase invariant loss fails closed and retains only latest follow-up", "[combined-appearance]")
{
    AppearanceApply apply;
    NotifyCharacterAppearanceUpdate b, c;
    b.AppearanceBuffer = "B immutable final bytes";
    c.AppearanceBuffer = "C latest";
    apply.Receive(b);
    REQUIRE(apply.Begin({1, 2, 3}));
    apply.Combined = cycle();
    reachRaceWait(apply.Combined);
    SECTION("source sex drift during race wait")
    {
        auto actual = apply.Combined.Expected();
        actual.Sex = 1;
        REQUIRE_FALSE(check(apply.Combined.Expected(), actual));
    }
    SECTION("provenance drift during final wait")
    {
        REQUIRE(apply.Combined.ObserveRace({4, 5, 6}, 1));
        REQUIRE(apply.Combined.RequestFinalDeserialize(2));
        REQUIRE(apply.Combined.VerifyFinal(true));
        REQUIRE(apply.Combined.RequestFinalReset({4, 5, 0}));
        REQUIRE(apply.Combined.FinalResetReturned(true));
        auto actual = apply.Combined.Expected();
        ++actual.ProvenanceBaseId;
        REQUIRE_FALSE(check(apply.Combined.Expected(), actual));
    }
    apply.Receive(c);
    REQUIRE(apply.Active->AppearanceBuffer == b.AppearanceBuffer);
    REQUIRE_FALSE(apply.Begin({}));
    REQUIRE_FALSE(apply.RequestReset());
    REQUIRE_FALSE(apply.RequestRaceReset({}));
    REQUIRE_FALSE(apply.ObserveHead({4, 5, 6}));
    apply.Combined.NativeCallInProgress = true;
    apply.Finish(false); // Native adapter stops on the failed observation above.
    REQUIRE_FALSE(apply.Combined.NativeCallInProgress);
    REQUIRE(apply.Combined.State == CombinedStage::Failed);
    REQUIRE(apply.BlockFaceGen);
    REQUIRE_FALSE(apply.Combined.RequestFinalDeserialize(2));
    REQUIRE_FALSE(apply.Combined.RequestFinalReset({}));
    REQUIRE(apply.Latest->AppearanceBuffer == "C latest");
    REQUIRE(apply.Begin({4, 5, 6}));
    REQUIRE_FALSE(apply.Combined.Enabled);
    REQUIRE(apply.Active->AppearanceBuffer == "C latest");
}

TEST_CASE("Combined active snapshot spans both phases before latest begins", "[combined-appearance]")
{
    AppearanceApply apply;
    NotifyCharacterAppearanceUpdate b, c, d;
    b.AppearanceBuffer = "B";
    c.AppearanceBuffer = "C";
    d.AppearanceBuffer = "D";
    apply.Receive(b);
    REQUIRE(apply.Begin({1, 2, 3}));
    apply.Combined = cycle();
    reachRaceWait(apply.Combined);
    apply.Receive(c);
    REQUIRE_FALSE(apply.Begin({}));
    REQUIRE(apply.Combined.ObserveRace({4, 5, 6}, 1));
    REQUIRE(apply.Combined.RequestFinalDeserialize(2));
    REQUIRE(apply.Active->AppearanceBuffer == "B");
    REQUIRE(apply.Combined.VerifyFinal(true));
    REQUIRE(apply.Combined.RequestFinalReset({4, 5, 0}));
    REQUIRE(apply.Combined.FinalResetReturned(true));
    apply.Receive(d);
    REQUIRE(apply.Active->AppearanceBuffer == "B");
    REQUIRE(apply.Latest->AppearanceBuffer == "D");
    REQUIRE_FALSE(apply.Begin({}));
    REQUIRE(apply.Combined.ObserveFinal({7, 8, 9}));
    REQUIRE(apply.Combined.RequestFaceGen());
    REQUIRE_FALSE(apply.CompleteTints(true));
    REQUIRE(apply.Combined.Complete(true));
    apply.Finish(true);
    REQUIRE(apply.Combined.ResetCount == 2);
    REQUIRE(apply.Begin({7, 8, 9}));
    REQUIRE_FALSE(apply.Combined.Enabled);
    REQUIRE(apply.Active->AppearanceBuffer == "D");
}

namespace
{
// Native-free adapter driver: each call is one service update. The production
// cycle's update identity prevents even an accidental same-update resume.
void raceDriverStep(AppearanceApply& apply, uint64_t update, const AppearancePostState& actual, AppearanceNodes nodes, bool waiting = false, int64_t count = 0, size_t worn = 0)
{
    auto& c = apply.Combined;
    if (c.State == CombinedStage::RaceStageReady)
    {
        if (c.RaceReadyUpdate == update)
            return;
        if (!apply.Active || !check(c.Expected(), actual) || waiting || !RenewedAppearanceGeometry(c.BeforeRaceReset, nodes) || c.InventoryLost(count, worn))
        {
            apply.Finish(false);
            return;
        }
        REQUIRE(c.RequestFinalDeserialize(update));
        REQUIRE(c.VerifyFinal(true));
        REQUIRE(c.RequestFinalReset({4, 5, 0}));
        REQUIRE(c.FinalResetReturned(true));
        return;
    }
    if (c.State == CombinedStage::WaitingForRaceGeometry)
    {
        REQUIRE(c.Tick());
        c.ObserveRace(nodes, update);
        return;
    }
}
AppearanceApply barrierApply()
{
    AppearanceApply apply;
    NotifyCharacterAppearanceUpdate snapshot;
    snapshot.AppearanceBuffer = "immutable B";
    apply.Receive(snapshot);
    REQUIRE(apply.Begin({1, 2, 3}));
    apply.Combined = cycle();
    reachRaceWait(apply.Combined);
    return apply;
}
} // namespace

TEST_CASE("Combined driver returns at race readiness and resumes only in a later service update", "[combined-appearance]")
{
    auto apply = barrierApply();
    auto& c = apply.Combined;
    raceDriverStep(apply, 41, c.RaceExpected, {4, 5, 6});
    REQUIRE(c.State == CombinedStage::RaceStageReady);
    REQUIRE(c.DeserializeCount == 0);
    REQUIRE(c.ResetCount == 1);
    REQUIRE_FALSE(c.RequestFinalDeserialize(41));
    raceDriverStep(apply, 41, c.RaceExpected, {4, 5, 6});
    REQUIRE(c.State == CombinedStage::RaceStageReady);
    REQUIRE(c.DeserializeCount == 0);
    NotifyCharacterAppearanceUpdate latest;
    latest.AppearanceBuffer = "latest C";
    apply.Receive(latest);
    REQUIRE(apply.Active->AppearanceBuffer == "immutable B");
    REQUIRE(apply.Latest->AppearanceBuffer == "latest C");
    raceDriverStep(apply, 42, c.RaceExpected, {4, 5, 6});
    REQUIRE(c.State == CombinedStage::WaitingForFinalGeometry);
    REQUIRE(apply.Active->AppearanceBuffer == "immutable B");
    REQUIRE(apply.Latest->AppearanceBuffer == "latest C");
    REQUIRE(c.DeserializeCount == 1);
    REQUIRE(c.ResetCount == 2);
    REQUIRE(c.SwitchCount == 1);
    raceDriverStep(apply, 43, c.FinalExpected, {7, 8, 9});
    REQUIRE(c.DeserializeCount == 1);
    REQUIRE(c.ResetCount == 2);
    REQUIRE(c.ObserveFinal({7, 8, 9}));
    REQUIRE(c.RequestFaceGen());
    REQUIRE_FALSE(c.RequestFaceGen());
    REQUIRE(c.Complete(true));
    REQUIRE(c.FaceGenCount == 1);
}

TEST_CASE("Combined driver fails closed on post-barrier invariant or geometry loss", "[combined-appearance]")
{
    auto apply = barrierApply();
    auto& c = apply.Combined;
    raceDriverStep(apply, 41, c.RaceExpected, {4, 5, 6});
    auto actual = c.RaceExpected;
    AppearanceNodes nodes{4, 5, 6};
    bool waiting = false;
    int64_t count = 0;
    SECTION("runtime race drift")
    {
        ++actual.RuntimeRace;
    }
    SECTION("base race drift")
    {
        ++actual.BaseRace;
    }
    SECTION("source sex drift")
    {
        actual.Sex = 1;
    }
    SECTION("provenance drift")
    {
        ++actual.ProvenanceBaseId;
    }
    SECTION("actor identity drift")
    {
        ++actual.ActorPointer;
    }
    SECTION("base identity drift")
    {
        ++actual.BasePointer;
    }
    SECTION("server identity drift")
    {
        ++actual.ServerId;
    }
    SECTION("cached identity drift")
    {
        ++actual.CachedRefId;
    }
    SECTION("Active lost")
    {
        apply.Active.reset();
    }
    SECTION("WaitingFor3D returned")
    {
        waiting = true;
    }
    SECTION("root lost")
    {
        nodes.ThirdPerson = 0;
    }
    SECTION("face lost")
    {
        nodes.Face = 0;
    }
    SECTION("head lost")
    {
        nodes.Head = 0;
    }
    SECTION("root no longer renewed")
    {
        nodes.ThirdPerson = 1;
    }
    SECTION("face and head no longer renewed")
    {
        nodes.Face = 2;
        nodes.Head = 3;
    }
    SECTION("inventory lost")
    {
        count = -1;
    }
    SECTION("equipment lost")
    {
        c.EquippedCount = 1;
    }
    raceDriverStep(apply, 42, actual, nodes, waiting, count);
    REQUIRE(c.State == CombinedStage::Failed);
    REQUIRE(c.DeserializeCount == 0);
    REQUIRE(c.ResetCount == 1);
    REQUIRE(c.FaceGenCount == 0);
    REQUIRE_FALSE(c.RequestFinalDeserialize(43));
    REQUIRE_FALSE(c.RequestFinalReset({}));
}
