#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>
#include <optional>
#include <CharacterCreation/AppearanceApply.h>
#include <catch2/catch.hpp>
#include <limits>
#include <CharacterCreation/AppearancePostState.h>
#include <CharacterCreation/AppearanceTintHash.h>
#include <CharacterCreation/AppearanceClassification.h>

using namespace STRE::CharacterCreation;

TEST_CASE("Appearance same-race guard allows weight changes but rejects race and sex changes", "[appearance-apply]")
{
    const GameId nord{0, 0x13746};
    CharacterAppearanceDescriptor incoming{nord, 0, 10.f};
    REQUIRE(SameAppearanceRaceSex(incoming, nord, nord, 0));
    incoming.Weight = 100.f;
    REQUIRE(SameAppearanceRaceSex(incoming, nord, nord, 0));
    REQUIRE_FALSE(SameAppearanceRaceSex(incoming, GameId{0, 0x13743}, nord, 0));
    REQUIRE_FALSE(SameAppearanceRaceSex(incoming, nord, GameId{0, 0x13743}, 0));
    REQUIRE_FALSE(SameAppearanceRaceSex(incoming, nord, nord, 1));
    incoming.Race = GameId{1, nord.BaseId};
    REQUIRE_FALSE(SameAppearanceRaceSex(incoming, nord, nord, 0));
}

TEST_CASE("Appearance descriptor rejects invalid metadata on the wire", "[appearance-apply]")
{
    RequestCharacterAppearanceUpdate sent;
    sent.AppearanceBuffer = "npc";
    sent.Descriptor = {GameId{0, 0x13746}, 0, 50.f};
    SECTION("missing race")
    {
        sent.Descriptor.Race = {};
    }
    SECTION("dynamic race")
    {
        sent.Descriptor.Race.ModId = UINT32_MAX;
    }
    SECTION("sex out of range")
    {
        sent.Descriptor.Sex = 2;
    }
    SECTION("weight negative")
    {
        sent.Descriptor.Weight = -1.f;
    }
    SECTION("weight too large")
    {
        sent.Descriptor.Weight = 101.f;
    }
    SECTION("weight NaN")
    {
        sent.Descriptor.Weight = std::numeric_limits<float>::quiet_NaN();
    }
    REQUIRE_FALSE(sent.IsValid());
    TiltedPhoques::Buffer buffer(128);
    TiltedPhoques::Buffer::Writer writer(&buffer);
    sent.SerializeRaw(writer);
    TiltedPhoques::Buffer::Reader reader(&buffer);
    RequestCharacterAppearanceUpdate received;
    received.DeserializeRaw(reader);
    REQUIRE_FALSE(received.IsValid());
}

TEST_CASE("Appearance applier keeps active snapshot immutable and only latest follow-up", "[appearance-apply]")
{
    AppearanceApply apply;
    NotifyCharacterAppearanceUpdate snapshot;
    snapshot.AppearanceBuffer = "first";
    apply.Receive(snapshot);
    snapshot.AppearanceBuffer = "second";
    apply.Receive(snapshot);
    REQUIRE(apply.Begin({1, 2, 3}));
    REQUIRE(apply.Active->AppearanceBuffer == "second");
    snapshot.AppearanceBuffer = "third";
    apply.Receive(snapshot);
    snapshot.AppearanceBuffer = "latest";
    apply.Receive(snapshot);
    REQUIRE_FALSE(apply.Begin({4, 5, 6}));
    REQUIRE(apply.Active->AppearanceBuffer == "second");
    REQUIRE(apply.Latest->AppearanceBuffer == "latest");
    REQUIRE(apply.BlockFaceGen);
    apply.Finish(true);
    REQUIRE_FALSE(apply.BlockFaceGen);
    REQUIRE(apply.Begin({4, 5, 6}));
    REQUIRE(apply.Active->AppearanceBuffer == "latest");
    apply.Finish(true);
    REQUIRE_FALSE(apply.Begin({7, 8, 9}));
}

TEST_CASE("Appearance applier requires a nonnull face or head transition rather than main 3D", "[appearance-apply]")
{
    AppearanceApply apply;
    apply.Receive(NotifyCharacterAppearanceUpdate{});
    REQUIRE(apply.Begin({1, 2, 3}));
    REQUIRE_FALSE(apply.ObserveHead({9, 2, 3}));
    REQUIRE_FALSE(apply.ObserveHead({9, 0, 0}));
    REQUIRE(apply.State == AppearanceApply::Stage::WaitingForHead);
    REQUIRE(apply.ObserveHead({1, 2, 4}));
    REQUIRE(apply.State == AppearanceApply::Stage::ApplyingTints);
    REQUIRE(apply.TintTarget.Head == 4);
    REQUIRE(apply.Ticks == 3);
}

TEST_CASE("Appearance timeout never enables tints and rebind discards old readiness", "[appearance-apply]")
{
    AppearanceApply apply;
    apply.Receive(NotifyCharacterAppearanceUpdate{});
    REQUIRE(apply.Begin({1, 2, 3}));
    for (uint32_t i = 0; i < AppearanceApply::MaxTicks; ++i)
        REQUIRE_FALSE(apply.ObserveHead({1, 2, 3}));
    REQUIRE(apply.Ticks == AppearanceApply::MaxTicks);
    REQUIRE_FALSE(apply.ObserveHead({1, 4, 5}));
    REQUIRE(apply.State == AppearanceApply::Stage::WaitingForHead);
    apply.Finish(false);
    REQUIRE(apply.BlockFaceGen);
    REQUIRE_FALSE(apply.Active);
    apply.Rebind();
    REQUIRE_FALSE(apply.BlockFaceGen);
    REQUIRE(apply.Ticks == 0);
}

TEST_CASE("Different diagnostic weight permits a single reset and Generated completes tints", "[appearance-apply]")
{
    AppearancePostState before, expected, actual;
    before.Weight = actual.Weight = 50.f;
    expected.Weight = 75.f;
    REQUIRE(CheckAppearancePostState(before, expected, actual, [](const char*, auto, auto, auto, bool) {}));
    AppearanceApply apply;
    apply.Receive(NotifyCharacterAppearanceUpdate{});
    REQUIRE(apply.Begin({1, 2, 3}));
    REQUIRE(apply.RequestReset());
    REQUIRE(apply.ResetRequested);
    REQUIRE_FALSE(apply.RequestReset());
    REQUIRE_FALSE(apply.CompleteTints(true));
    REQUIRE(apply.ObserveHead({1, 4, 5}));
    REQUIRE(apply.Transition.Head == 5);
    REQUIRE_FALSE(apply.CompleteTints(false));
    REQUIRE(apply.Active);
    REQUIRE(apply.CompleteTints(true));
    REQUIRE_FALSE(apply.Active);
    REQUIRE_FALSE(apply.BlockFaceGen);
    REQUIRE_FALSE(apply.RequestReset());
}

TEST_CASE("Appearance tint diagnostic hash covers every ordered field", "[appearance-apply]")
{
    Tints original;
    Tints::Entry entry{};
    entry.Type = 1;
    entry.Color = 0x12345678;
    entry.Alpha = 0.5f;
    entry.Name = "skin";
    original.Entries.push_back(entry);
    Tints changed = original;
    REQUIRE(AppearanceTintHash(changed) == AppearanceTintHash(original));
    SECTION("type")
    {
        ++changed.Entries[0].Type;
    }
    SECTION("color")
    {
        ++changed.Entries[0].Color;
    }
    SECTION("alpha")
    {
        changed.Entries[0].Alpha = 0.25f;
    }
    SECTION("name")
    {
        changed.Entries[0].Name = "hair";
    }
    SECTION("count")
    {
        changed.Entries.push_back(entry);
    }
    REQUIRE(AppearanceTintHash(changed) != AppearanceTintHash(original));
}
TEST_CASE("Appearance classification separates race change from unsupported sex and invalid state", "[appearance-apply]")
{
    const GameId nord{0, 0x13746}, highElf{0, 0x13743};
    CharacterAppearanceDescriptor descriptor{nord, 0, 75.f};
    REQUIRE(ClassifyAppearanceUpdate(descriptor, nord, nord, 0, true, true) == AppearanceClassification::SameRaceSameSex);
    descriptor.Race = highElf;
    REQUIRE(ClassifyAppearanceUpdate(descriptor, nord, nord, 0, true, true) == AppearanceClassification::RaceChangeSameSex);
    REQUIRE(ClassifyAppearanceUpdate(descriptor, nord, nord, 1, true, true) == AppearanceClassification::RaceAndSexChange);
    REQUIRE(ClassifyAppearanceUpdate(descriptor, nord, nord, 0, false, true) == AppearanceClassification::Invalid);
    REQUIRE(ClassifyAppearanceUpdate(descriptor, nord, nord, 0, true, false) == AppearanceClassification::Invalid);
    REQUIRE(ClassifyAppearanceUpdate(descriptor, nord, highElf, 0, true, true) == AppearanceClassification::Invalid);
    descriptor.Race = {};
    REQUIRE(ClassifyAppearanceUpdate(descriptor, nord, nord, 0, true, true) == AppearanceClassification::Invalid);
}