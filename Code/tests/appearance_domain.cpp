#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>
#include <CharacterCreation/AppearanceDomain.h>
#include <CharacterCreation/AppearanceClassification.h>
#include <CharacterCreation/AppearanceApply.h>
#include <catch2/catch.hpp>
#include <vector>

using namespace STRE::CharacterCreation;
using Kind = AppearanceClassification;

namespace
{
bool supported(uint32_t id)
{
    return SupportedAppearanceRace(id, true, true, "male.nif", "different-female.nif");
}
struct State
{
    uint32_t Race;
    uint8_t Sex;
};
Kind classify(State from, State to)
{
    return ClassifyAppearanceTransition({7, from.Race}, from.Sex, {7, to.Race}, to.Sex);
}
} // namespace

TEST_CASE("Appearance domain admits exactly eight audited vanilla records", "[appearance-domain]")
{
    REQUIRE(VanillaHumanoidAppearanceBaseIds.size() == 8);
    for (auto id : VanillaHumanoidAppearanceBaseIds)
    {
        CAPTURE(id);
        REQUIRE(supported(id));
        REQUIRE_FALSE(SupportedAppearanceRace(id, false, true, "male.nif", "female.nif"));
        REQUIRE_FALSE(SupportedAppearanceRace(id, true, false, "male.nif", "female.nif"));
        REQUIRE_FALSE(SupportedAppearanceRace(id, true, true, "", "female.nif"));
        REQUIRE_FALSE(SupportedAppearanceRace(id, true, true, "male.nif", ""));
        REQUIRE_FALSE(SupportedAppearanceRace(id, true, true, std::string(513, 'x'), "female.nif"));
        REQUIRE_FALSE(SupportedAppearanceRace(id, true, true, "male.nif", std::string(513, 'x')));
        REQUIRE(SupportedAppearanceRace(id, true, true, std::string(512, 'x'), std::string(512, 'y')));
    }
    for (auto id : {0u, 0x13740u, 0x13745u, 0x88794u, 0xCDD84u, 0x1013746u, 0xFF013746u})
        REQUIRE_FALSE(supported(id));
}

TEST_CASE("Generic classification knows equality and sex, not race identities", "[appearance-domain]")
{
    REQUIRE(classify({11, 0}, {11, 0}) == Kind::SameRaceSameSex);
    REQUIRE(classify({11, 0}, {12, 0}) == Kind::RaceChangeSameSex);
    REQUIRE(classify({11, 0}, {11, 1}) == Kind::SameRaceSexChange);
    REQUIRE(classify({11, 0}, {12, 1}) == Kind::RaceAndSexChange);
    REQUIRE(classify({0, 0}, {12, 0}) == Kind::Invalid);
    REQUIRE(classify({11, 0}, {0, 0}) == Kind::Invalid);
    REQUIRE(classify({11, 2}, {12, 0}) == Kind::Invalid);
    REQUIRE(classify({11, 0}, {12, 2}) == Kind::Invalid);
    CharacterAppearanceDescriptor invalid{{7, 11}, 0, -1.f};
    REQUIRE(ClassifyAppearanceUpdate(invalid, {7, 11}, {7, 11}, 0, true, true) == Kind::Invalid);
}

TEST_CASE("Every admitted race selects the same sex-only strategy in both directions", "[appearance-domain]")
{
    for (auto id : VanillaHumanoidAppearanceBaseIds)
        for (uint8_t sex : {0, 1})
        {
            CAPTURE(id, sex);
            REQUIRE(supported(id));
            REQUIRE(classify({id, sex}, {id, uint8_t(1 - sex)}) == Kind::SameRaceSexChange);
            SexAppearanceCycle cycle;
            AppearancePostState target;
            target.RuntimeRace = target.BaseRace = id;
            target.Sex = 1 - sex;
            cycle.Begin(target);
            REQUIRE(cycle.RequestDeserialize());
            REQUIRE_FALSE(cycle.RequestDeserialize());
            REQUIRE(cycle.Verify(true, uint32_t(target.Sex)));
            REQUIRE(cycle.RequestReset({1, 2, 3}));
            REQUIRE_FALSE(cycle.RequestReset({1, 2, 3}));
        }
}

TEST_CASE("Representative race-only and combined transitions select symmetric strategies", "[appearance-domain]")
{
    for (auto pair : {std::pair{0x13746u, 0x13743u}, {0x13741u, 0x13747u}, {0x13747u, 0x13742u}, {0x13744u, 0x13742u}})
        for (bool reverse : {false, true})
            for (uint8_t sex : {0, 1})
            {
                const auto from = reverse ? pair.second : pair.first;
                const auto to = reverse ? pair.first : pair.second;
                CAPTURE(from, to, sex);
                REQUIRE(supported(from));
                REQUIRE(supported(to));
                REQUIRE(classify({from, sex}, {to, sex}) == Kind::RaceChangeSameSex);
                REQUIRE(classify({from, sex}, {to, uint8_t(1 - sex)}) == Kind::RaceAndSexChange);
            }
}

TEST_CASE("Race-only cycle preserves call order and one-shot reservations", "[appearance-domain]")
{
    RaceAppearanceCycle cycle;
    AppearancePostState final;
    final.RuntimeRace = final.BaseRace = 1;
    final.Sex = 0;
    cycle.Begin(2, final);
    REQUIRE_FALSE(cycle.RequestDeserialize());
    REQUIRE_FALSE(cycle.RequestReset());
    REQUIRE(cycle.RequestSwitch());
    REQUIRE_FALSE(cycle.RequestSwitch());
    REQUIRE_FALSE(cycle.RequestDeserialize());
    REQUIRE(cycle.Returned(true));
    REQUIRE_FALSE(cycle.Deserialized(true));
    REQUIRE(cycle.RequestDeserialize());
    REQUIRE_FALSE(cycle.RequestDeserialize());
    REQUIRE_FALSE(cycle.RequestReset());
    REQUIRE(cycle.Deserialized(true));
    REQUIRE_FALSE(cycle.Deserialized(true));
    REQUIRE(cycle.RequestReset());
    REQUIRE_FALSE(cycle.RequestReset());
    REQUIRE_FALSE(cycle.Observe({1, 2, 3}, {4, 5, 6}));
    REQUIRE(cycle.ResetReturned(true));
    REQUIRE(cycle.Observe({1, 2, 3}, {4, 5, 6}));
    REQUIRE(cycle.State == RaceAppearanceCycle::Stage::ApplyingTints);
}

TEST_CASE("Race-only invariant failures cannot reach reset", "[appearance-domain]")
{
    AppearancePostState before;
    before.RuntimeRace = before.BaseRace = 1;
    before.Sex = 0;
    before.ProvenanceBaseId = 50;
    RaceAppearanceCycle cycle;
    auto expected = before;
    expected.Sex = 1;
    cycle.Begin(2, expected);
    REQUIRE(cycle.RequestSwitch());
    auto actual = cycle.Expected;
    actual.Sex = before.Sex;
    auto switchExpected = cycle.Expected;
    switchExpected.Sex = before.Sex;
    const auto check = [](auto wanted, auto actual)
    {
        return CheckAppearancePostState(wanted, wanted, actual, [](const char*, auto, auto, auto, bool) {});
    };
    SECTION("wrong race after SwitchRace")
    {
        actual.RuntimeRace = 1;
        REQUIRE_FALSE(cycle.Returned(check(switchExpected, actual)));
        REQUIRE_FALSE(cycle.RequestDeserialize());
        REQUIRE_FALSE(cycle.RequestReset());
        return;
    }
    REQUIRE(cycle.Returned(check(switchExpected, actual)));
    REQUIRE(cycle.RequestDeserialize());
    actual = cycle.Expected;
    SECTION("target sex missing")
    {
        actual.Sex = 0;
    }
    SECTION("target base race lost")
    {
        actual.BaseRace = 1;
    }
    SECTION("private provenance lost")
    {
        actual.ProvenanceBaseId = 51;
    }
    REQUIRE_FALSE(cycle.Deserialized(check(cycle.Expected, actual)));
    REQUIRE_FALSE(cycle.RequestReset());
    REQUIRE_FALSE(cycle.RequestDeserialize());
}

TEST_CASE("Authorized intermediate sequences stay classified within the domain", "[appearance-domain]")
{
    // Value-only sequence closure; this does not execute or predict native engine success.
    const std::vector<std::vector<State>> sequences{
        {{0x13746, 0}, {0x13743, 0}, {0x13743, 1}},
        {{0x13746, 0}, {0x13746, 1}, {0x13743, 1}},
        {{0x13741, 0}, {0x13747, 1}},
        {{0x13744, 1}, {0x13742, 1}, {0x13742, 0}, {0x13741, 0}}};
    const std::vector<std::vector<Kind>> expected{
        {Kind::RaceChangeSameSex, Kind::SameRaceSexChange},
        {Kind::SameRaceSexChange, Kind::RaceChangeSameSex},
        {Kind::RaceAndSexChange},
        {Kind::RaceChangeSameSex, Kind::SameRaceSexChange, Kind::RaceChangeSameSex}};
    for (size_t s = 0; s < sequences.size(); ++s)
        for (size_t i = 1; i < sequences[s].size(); ++i)
        {
            REQUIRE(supported(sequences[s][i - 1].Race));
            REQUIRE(supported(sequences[s][i].Race));
            REQUIRE(classify(sequences[s][i - 1], sequences[s][i]) == expected[s][i - 1]);
        }
}

TEST_CASE("Latest after an active race cycle is reclassified from the attained state", "[appearance-domain]")
{
    AppearanceApply apply;
    NotifyCharacterAppearanceUpdate b, c;
    b.AppearanceBuffer = "B";
    b.Descriptor = {{0, 0x13743}, 0, 50};
    c.AppearanceBuffer = "C";
    c.Descriptor = {{0, 0x13743}, 1, 50};
    apply.Receive(b);
    REQUIRE(apply.Begin({1, 2, 3}));
    apply.Race.Begin(0x13743, {});
    REQUIRE(apply.Race.RequestSwitch());
    apply.Receive(c);
    REQUIRE(apply.Active->AppearanceBuffer == "B");
    REQUIRE(apply.Latest->AppearanceBuffer == "C");
    REQUIRE_FALSE(apply.Begin({1, 2, 3}));
    REQUIRE(apply.Race.Returned(true));
    REQUIRE(apply.Race.RequestDeserialize());
    REQUIRE(apply.Race.Deserialized(true));
    REQUIRE(apply.RequestRaceReset({1, 2, 3}));
    REQUIRE(apply.RaceResetReturned(true));
    REQUIRE(apply.ObserveHead({4, 5, 6}));
    REQUIRE_FALSE(apply.CompleteTints(false));
    REQUIRE(apply.CompleteTints(true));
    REQUIRE(apply.Begin({4, 5, 6}));
    REQUIRE(apply.Active->AppearanceBuffer == "C");
    REQUIRE_FALSE(apply.Latest);
    REQUIRE(classify({0x13743, 0}, {apply.Active->Descriptor.Race.BaseId, apply.Active->Descriptor.Sex}) == Kind::SameRaceSexChange);
    AppearancePostState target;
    target.RuntimeRace = target.BaseRace = 0x13743;
    target.Sex = 1;
    apply.Sex.Begin(target);
    REQUIRE(apply.Sex.RequestDeserialize());
    REQUIRE(apply.Sex.Verify(true, 1));
    REQUIRE(apply.Sex.RequestReset({4, 5, 6}));
    REQUIRE(apply.Sex.ResetReturned(true));
    REQUIRE(apply.Sex.Observe({7, 8, 9}));
    REQUIRE(apply.Sex.Complete(true));
    apply.Finish(true);
    REQUIRE_FALSE(apply.Active);
    REQUIRE_FALSE(apply.BlockFaceGen);
}
