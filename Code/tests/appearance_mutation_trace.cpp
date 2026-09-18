#include <CharacterCreation/AppearanceMutationTrace.h>
#include <catch2/catch.hpp>
#include <string>
using STRE::CharacterCreation::AppearanceMutationFingerprint;

TEST_CASE("Appearance trace names each changed field and stays silent for identical samples", "[appearance-trace]")
{
    AppearanceMutationFingerprint before;
    REQUIRE(before.Changes(before).empty());
    for (size_t i = 0; i < before.Values.size(); ++i)
    {
        auto after = before;
        after.Values[i] = 1;
        REQUIRE(after.Changes(before) == AppearanceMutationFingerprint::Names[i]);
    }
    auto after = before;
    after.Values[AppearanceMutationFingerprint::Sex] = 1;
    after.Values[AppearanceMutationFingerprint::Head] = 12;
    REQUIRE(after.Changes(before) == "sex,head");
}

TEST_CASE("Appearance trace distinguishes recreation from in-place changes and absent identity", "[appearance-trace]")
{
    using F = AppearanceMutationFingerprint;
    F before;
    before.Values[F::ActorPointer] = 100;
    before.Values[F::BasePointer] = 200;
    before.Values[F::ActorId] = 1;
    before.Values[F::BaseId] = 2;
    for (auto field : {F::ActorId, F::ActorPointer, F::BaseId, F::BasePointer})
    {
        auto after = before;
        ++after.Values[field];
        REQUIRE(std::string(after.Outcome(before)) == "REMOTE_RECREATED");
    }
    auto after = before;
    after.Values[F::RuntimeRace] = 3;
    after.Values[F::Sex] = 1;
    REQUIRE(std::string(after.Outcome(before)) == "IN_PLACE_MUTATION");
    after.Values[F::ActorPointer] = 0;
    REQUIRE(std::string(after.Outcome(before)) == "IDENTITY_UNAVAILABLE");
    REQUIRE(std::string(before.Outcome(after)) == "IDENTITY_UNAVAILABLE");
}
