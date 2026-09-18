#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>
#include <optional>
#include <CharacterCreation/AppearanceApply.h>
#include <CharacterCreation/AppearanceClassification.h>
#include <CharacterCreation/AppearanceDomain.h>
#include <catch2/catch.hpp>
using namespace STRE::CharacterCreation;

TEST_CASE("Sex experiment classification and gender models are restricted", "[sex-appearance]")
{
    const GameId nord{0, 0x13746}, elf{0, 0x13743};
    CharacterAppearanceDescriptor d{nord, 0, 50};
    REQUIRE(ClassifyAppearanceUpdate(d, nord, nord, 0, true, true) == AppearanceClassification::SameRaceSameSex);
    d.Race = elf;
    REQUIRE(ClassifyAppearanceUpdate(d, nord, nord, 0, true, true) == AppearanceClassification::RaceChangeSameSex);
    d.Sex = 1;
    REQUIRE(ClassifyAppearanceUpdate(d, nord, nord, 0, true, true) == AppearanceClassification::RaceAndSexChange);
    d.Race = nord;
    REQUIRE(ClassifyAppearanceUpdate(d, nord, nord, 0, true, true) == AppearanceClassification::SameRaceSexChange);
    REQUIRE(ClassifyAppearanceUpdate(d, nord, nord, 0, true, false) == AppearanceClassification::Invalid);
    REQUIRE(ClassifyAppearanceUpdate(d, nord, elf, 0, true, true) == AppearanceClassification::Invalid);
    REQUIRE(ClassifyAppearanceUpdate(d, nord, nord, 0, false, true) == AppearanceClassification::Invalid);
    d.Sex = 2;
    REQUIRE(ClassifyAppearanceUpdate(d, nord, nord, 0, true, true) == AppearanceClassification::Invalid);
    REQUIRE(AppearanceModelsValid("male.nif", "female.nif"));
    REQUIRE_FALSE(AppearanceModelsValid("", "female.nif"));
    REQUIRE_FALSE(AppearanceModelsValid("male.nif", ""));
    REQUIRE_FALSE(AppearanceModelsValid(std::string(513, 'x'), "female.nif"));
}
TEST_CASE("Sex Deserialize is one shot and target sex gates the only reset", "[sex-appearance]")
{
    SexAppearanceCycle cycle;
    AppearancePostState expected;
    expected.Sex = 1;
    cycle.Begin(expected);
    REQUIRE_FALSE(cycle.RequestReset({1, 2, 3}));
    REQUIRE(cycle.RequestDeserialize());
    REQUIRE_FALSE(cycle.RequestDeserialize());
    REQUIRE_FALSE(cycle.RequestReset({1, 2, 3}));
    SECTION("sex not applied")
    {
        REQUIRE_FALSE(cycle.Verify(true, 0));
        REQUIRE_FALSE(cycle.RequestReset({1, 2, 3}));
        REQUIRE_FALSE(cycle.RequestDeserialize());
        REQUIRE(cycle.State == SexAppearanceCycle::Stage::Failed);
    }
    SECTION("identity failed")
    {
        REQUIRE_FALSE(cycle.Verify(false, 1));
        REQUIRE_FALSE(cycle.RequestReset({1, 2, 3}));
    }
    SECTION("verified then reset failure")
    {
        REQUIRE(cycle.Verify(true, 1));
        REQUIRE(cycle.RequestReset({1, 2, 3}));
        REQUIRE_FALSE(cycle.RequestReset({4, 5, 6}));
        REQUIRE_FALSE(cycle.ResetReturned(false));
        REQUIRE_FALSE(cycle.RequestReset({1, 2, 3}));
        REQUIRE_FALSE(cycle.Complete(true));
    }
}
TEST_CASE("Sex readiness requires new root and gender head before Generated completion", "[sex-appearance]")
{
    SexAppearanceCycle c;
    AppearancePostState expected;
    expected.Sex = 1;
    c.Begin(expected);
    REQUIRE(c.RequestDeserialize());
    REQUIRE(c.Verify(true, 1));
    REQUIRE(c.RequestReset({1, 2, 3}));
    REQUIRE(c.ResetReturned(true));
    REQUIRE_FALSE(c.Complete(true));
    REQUIRE(c.Tick());
    REQUIRE_FALSE(c.Observe({1, 2, 3}));
    REQUIRE_FALSE(c.Observe({6, 2, 3}));
    REQUIRE_FALSE(c.Observe({1, 4, 3}));
    REQUIRE_FALSE(c.Observe({1, 2, 5}));
    REQUIRE_FALSE(c.Observe({0, 4, 5}));
    REQUIRE_FALSE(c.Observe({6, 0, 5}));
    REQUIRE_FALSE(c.Observe({6, 4, 0}));
    SECTION("root and face")
    {
        REQUIRE(c.Observe({6, 4, 3}));
    }
    SECTION("root and head")
    {
        REQUIRE(c.Observe({6, 2, 5}));
    }
    SECTION("all changed")
    {
        REQUIRE(c.Observe({6, 4, 5}));
    }
    REQUIRE_FALSE(c.RequestReset({1, 2, 3}));
    REQUIRE_FALSE(c.Complete(false));
    REQUIRE(c.Complete(true));
}
TEST_CASE("Sex invariants reject target regression and preserve diagnostic weight policy", "[sex-appearance]")
{
    AppearancePostState expected;
    expected.Sex = 1;
    expected.RuntimeRace = expected.BaseRace = 0x13746;
    expected.ActorPointer = 9;
    expected.ProvenanceBaseId = 7;
    auto actual = expected;
    SECTION("sex wrong")
    {
        actual.Sex = 0;
    }
    SECTION("runtime race wrong")
    {
        actual.RuntimeRace = 0x13743;
    }
    SECTION("base race wrong")
    {
        actual.BaseRace = 0x13743;
    }
    SECTION("actor replaced")
    {
        actual.ActorPointer = 10;
    }
    SECTION("provenance changed")
    {
        actual.ProvenanceBaseId = 8;
    }
    SECTION("weight diagnostic")
    {
        actual.Weight = 75;
        REQUIRE(CheckAppearancePostState(expected, expected, actual, [](const char*, auto, auto, auto, bool) {}));
        return;
    }
    REQUIRE_FALSE(CheckAppearancePostState(expected, expected, actual, [](const char*, auto, auto, auto, bool) {}));
}
TEST_CASE("Sex timeout and latest snapshot do not add reset or switch requests", "[sex-appearance]")
{
    AppearanceApply apply;
    NotifyCharacterAppearanceUpdate first;
    first.AppearanceBuffer = "first";
    first.FaceTints.Entries.emplace_back();
    apply.Receive(first);
    REQUIRE(apply.Begin({1, 2, 3}));
    AppearancePostState expected;
    expected.Sex = 1;
    apply.Sex.Begin(expected);
    REQUIRE_FALSE(apply.RequestReset());
    REQUIRE_FALSE(apply.Race.RequestSwitch());
    REQUIRE(apply.Sex.RequestDeserialize());
    REQUIRE(apply.Sex.Verify(true, 1));
    REQUIRE(apply.Sex.RequestReset({1, 2, 3}));
    REQUIRE(apply.Sex.ResetReturned(true));
    first.AppearanceBuffer = "next";
    apply.Receive(first);
    first.AppearanceBuffer = "latest";
    apply.Receive(first);
    REQUIRE(apply.Active->AppearanceBuffer == "first");
    REQUIRE(apply.Active->FaceTints.Entries.size() == 1);
    REQUIRE(apply.Latest->AppearanceBuffer == "latest");
    for (uint32_t tick = 0; tick < SexAppearanceCycle::MaxTicks; ++tick)
    {
        REQUIRE(apply.Sex.Tick());
        REQUIRE_FALSE(apply.Sex.Observe({1, 2, 3}));
    }
    REQUIRE_FALSE(apply.Sex.Tick());
    REQUIRE_FALSE(apply.Sex.RequestReset({1, 2, 3}));
    apply.Finish(false);
    REQUIRE(apply.BlockFaceGen);
    REQUIRE(apply.Begin({6, 4, 5}));
    REQUIRE_FALSE(apply.Sex.Enabled);
    REQUIRE(apply.RequestReset());
}
