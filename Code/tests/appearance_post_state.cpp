#include <CharacterCreation/AppearancePostState.h>
#include <catch2/catch.hpp>
#include <string>
#include <vector>
#include <limits>
using namespace STRE::CharacterCreation;

TEST_CASE("Appearance postcheck reports every failed field without short circuit", "[appearance-apply]")
{
    AppearancePostState before, actual;
    before.Weight = actual.Weight = 50.f;
    before.ActorPointer = actual.ActorPointer = 1;
    before.BasePointer = actual.BasePointer = 2;
    before.ActorId = actual.ActorId = 3;
    before.BaseId = actual.BaseId = 4;
    before.ServerId = actual.ServerId = 5;
    before.FormId = actual.FormId = 6;
    before.CachedRefId = actual.CachedRefId = 7;
    before.ProvenanceActorId = actual.ProvenanceActorId = 8;
    before.ProvenanceBaseId = actual.ProvenanceBaseId = 9;
    before.RuntimeRace = actual.RuntimeRace = 10;
    before.BaseRace = actual.BaseRace = 11;
    before.Sex = actual.Sex = 12;
    before.RemoteMarker = actual.RemoteMarker = 13;
    before.PlayerMarker = actual.PlayerMarker = 14;
    before.PrivateBase = actual.PrivateBase = 15;
    SECTION("ActorPointer")
    {
        ++actual.ActorPointer;
    }
    SECTION("BasePointer")
    {
        ++actual.BasePointer;
    }
    SECTION("ActorId")
    {
        ++actual.ActorId;
    }
    SECTION("BaseId")
    {
        ++actual.BaseId;
    }
    SECTION("ServerId")
    {
        ++actual.ServerId;
    }
    SECTION("FormId")
    {
        ++actual.FormId;
    }
    SECTION("CachedRefId")
    {
        ++actual.CachedRefId;
    }
    SECTION("ProvenanceActorId")
    {
        ++actual.ProvenanceActorId;
    }
    SECTION("ProvenanceBaseId")
    {
        ++actual.ProvenanceBaseId;
    }
    SECTION("RuntimeRace")
    {
        ++actual.RuntimeRace;
    }
    SECTION("BaseRace")
    {
        ++actual.BaseRace;
    }
    SECTION("Sex")
    {
        ++actual.Sex;
    }
    SECTION("RemoteMarker")
    {
        ++actual.RemoteMarker;
    }
    SECTION("PlayerMarker")
    {
        ++actual.PlayerMarker;
    }
    SECTION("PrivateBase")
    {
        ++actual.PrivateBase;
    }
    std::vector<std::string> failures;
    size_t visited{};
    REQUIRE_FALSE(CheckAppearancePostState(
        before, before, actual,
        [&](const char* field, auto pre, auto expected, auto value, bool matches)
        {
            ++visited;
            if (!matches)
            {
                failures.emplace_back(field);
                REQUIRE(value != expected);
                REQUIRE(pre == expected);
            }
        }));
    REQUIRE(visited == 16);
    REQUIRE(failures.size() == 1);
}

TEST_CASE("Appearance postcheck treats weight as diagnostic without masking fatal race mismatch", "[appearance-apply]")
{
    AppearancePostState before, expected, actual;
    before.Weight = actual.Weight = 50.f;
    expected.Weight = 75.f;
    SECTION("weight only")
    {
    }
    SECTION("weight and race")
    {
        actual.BaseRace = 1;
    }
    SECTION("weight NaN")
    {
        actual.Weight = std::numeric_limits<float>::quiet_NaN();
    }
    std::vector<std::string> failures;
    const bool passed = CheckAppearancePostState(
        before, expected, actual,
        [&](const char* field, auto, auto, auto, bool matches)
        {
            if (!matches)
                failures.emplace_back(field);
        });
    REQUIRE(passed == (actual.BaseRace == 0));
    REQUIRE(failures.back() == "Weight");
    REQUIRE(failures.size() == (actual.BaseRace ? 2 : 1));
}

TEST_CASE("Appearance postcheck accepts restored descriptor weight with stable identity", "[appearance-apply]")
{
    AppearancePostState before, expected, actual;
    before.Weight = 50.f;
    expected.Weight = actual.Weight = 75.f;
    size_t visited{};
    REQUIRE(CheckAppearancePostState(
        before, expected, actual,
        [&](const char*, auto, auto, auto, bool matches)
        {
            ++visited;
            REQUIRE(matches);
        }));
    REQUIRE(visited == 16);
}
