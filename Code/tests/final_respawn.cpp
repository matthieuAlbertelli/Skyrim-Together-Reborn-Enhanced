#include <CharacterCreation/FinalRespawn.h>
#include <CharacterCreation/RemoteMaterializationLifecycle.h>
#include <catch2/catch.hpp>
#include <string_view>

using namespace STRE::CharacterCreation;

TEST_CASE("Final rematerialization automatically requires connected official Applied build", "[final-respawn]")
{
    REQUIRE(FinalRespawnEligible(true, true, true, 42, 42));
    REQUIRE_FALSE(FinalRespawnEligible(false, true, true, 42, 42));
    REQUIRE_FALSE(FinalRespawnEligible(true, false, true, 42, 42));
    REQUIRE_FALSE(FinalRespawnEligible(true, true, false, 42, 42));
    REQUIRE_FALSE(FinalRespawnEligible(true, true, true, 0, 0));
    REQUIRE_FALSE(FinalRespawnEligible(true, true, true, 41, 42));
    REQUIRE_FALSE(FinalRespawnEligible(true, true, true, 42, 0));
}

TEST_CASE("Final respawn ignores every sit sleep state while retaining independent safety guards", "[final-respawn]")
{
    for (unsigned sit = 0; sit < 16; ++sit)
    {
        CAPTURE(sit);
        const auto posture = sit << 14;
        REQUIRE(FinalRespawnActorStateSafe(posture, 0));
        for (unsigned bit = 18; bit != 32; ++bit)
            REQUIRE_FALSE(FinalRespawnActorStateSafe(posture | (1u << bit), 0)); // Fly/life/knock/attack.
        for (unsigned bit : {5u, 6u, 7u, 10u, 11u, 13u})
            REQUIRE_FALSE(FinalRespawnActorStateSafe(posture, 1u << bit));
        REQUIRE_FALSE(FinalRespawnActorStateSafe(posture | (1u << 8), 0));
        REQUIRE_FALSE(FinalRespawnActorStateSafe(posture | (1u << 10), 0));
    }
}

TEST_CASE("Reported creation state is movement inhibition and permits the LAB gate", "[final-respawn]")
{
    const auto state = DecodeFinalRespawnActorState(0x01200041, 0x00001008);
    REQUIRE(state.Life == 9);
    REQUIRE(std::string_view(state.LifeName()) == "dont-move");
    REQUIRE(state.Knock == 0);
    REQUIRE(state.Attack == 0);
    REQUIRE(state.Fly == 0);
    REQUIRE(state.Weapon == 0);
    REQUIRE(state.Recoil == 0);
    REQUIRE_FALSE(state.Staggered);
    REQUIRE_FALSE(state.Sprinting);
    REQUIRE_FALSE(state.Swimming);
    REQUIRE(state.Safe());
    REQUIRE(std::string_view(state.VetoReason()) == "none");
    REQUIRE(FinalRespawnActorStateSafe(0x01200041, 0x00001008));
    for (unsigned posture = 0; posture < 16; ++posture)
        REQUIRE(FinalRespawnActorStateSafe(0x01200041 | (posture << 14), 0x00001008));
}

TEST_CASE("Only DontMove adds a permitted life state without hiding other native hazards", "[final-respawn]")
{
    for (uint32_t life = 0; life < 16; ++life)
    {
        CAPTURE(life);
        const auto flags = life << 21;
        const auto state = DecodeFinalRespawnActorState(flags, 0);
        REQUIRE(state.Life == life);
        REQUIRE(state.LifeAllowsReplacement() == (life == 0 || life == 9));
        REQUIRE(state.Safe() == state.LifeAllowsReplacement());
        if (life != 0 && life != 9)
            REQUIRE(std::string_view(state.VetoReason()) == (life <= 8 ? "life-transition-or-incapacitated" : "life-unknown"));
    }
    for (uint32_t life : {0u, 9u})
    {
        CAPTURE(life);
        const auto flags = (life << 21) | 0x41u; // Ordinary movement bits remain irrelevant.
        for (uint32_t value = 1; value < 16; ++value)
        {
            const auto state = DecodeFinalRespawnActorState(flags | (value << 28), 0x1008);
            REQUIRE(state.Attack == value);
            REQUIRE_FALSE(state.AttackIdle());
            REQUIRE_FALSE(state.Safe());
            REQUIRE(std::string_view(state.VetoReason()) == "attack-active");
        }
        for (uint32_t value = 1; value < 8; ++value)
        {
            auto state = DecodeFinalRespawnActorState(flags | (value << 25), 0x1008);
            REQUIRE(state.Knock == value);
            REQUIRE_FALSE(state.KnockIdle());
            REQUIRE_FALSE(state.Safe());
            REQUIRE(std::string_view(state.VetoReason()) == "knock-transition-active");
            state = DecodeFinalRespawnActorState(flags | (value << 18), 0x1008);
            REQUIRE(state.Fly == value);
            REQUIRE_FALSE(state.FlyIdle());
            REQUIRE_FALSE(state.Safe());
            REQUIRE(std::string_view(state.VetoReason()) == "fly-state-outside-quiescent-lab");
            state = DecodeFinalRespawnActorState(flags, 0x1008 | (value << 5));
            REQUIRE(state.Weapon == value);
            REQUIRE_FALSE(state.WeaponSheathed());
            REQUIRE_FALSE(state.Safe());
            REQUIRE(std::string_view(state.VetoReason()) == "weapon-state-outside-quiescent-lab");
        }
        for (uint32_t value = 1; value < 4; ++value)
        {
            const auto state = DecodeFinalRespawnActorState(flags, 0x1008 | (value << 10));
            REQUIRE(state.Recoil == value);
            REQUIRE_FALSE(state.RecoilIdle());
            REQUIRE_FALSE(state.Safe());
            REQUIRE(std::string_view(state.VetoReason()) == "recoil-active");
        }
        auto state = DecodeFinalRespawnActorState(flags, 0x1008 | (1u << 13));
        REQUIRE_FALSE(state.NotStaggered());
        REQUIRE_FALSE(state.Safe());
        REQUIRE(std::string_view(state.VetoReason()) == "stagger-active");
        state = DecodeFinalRespawnActorState(flags | (1u << 8), 0x1008);
        REQUIRE_FALSE(state.NotSprinting());
        REQUIRE_FALSE(state.Safe());
        REQUIRE(std::string_view(state.VetoReason()) == "sprinting-outside-quiescent-lab");
        state = DecodeFinalRespawnActorState(flags | (1u << 10), 0x1008);
        REQUIRE_FALSE(state.NotSwimming());
        REQUIRE_FALSE(state.Safe());
        REQUIRE(std::string_view(state.VetoReason()) == "swimming-outside-quiescent-lab");
    }
}

TEST_CASE("Actor state policy differs from the previous gate only for DontMove", "[final-respawn]")
{
    // Exhaust the previously combined fly/life/knock/attack mask, including
    // overlapping hazards. Separate flags2 and movement vetoes are tested above.
    for (uint32_t fields = 0; fields < 0x4000; ++fields)
    {
        const auto flags = (fields << 18) | 0x41u;
        const bool previouslySafe = fields == 0;
        const bool onlyDontMove = fields == (9u << 3);
        const auto state = DecodeFinalRespawnActorState(flags, 0x1008);
        REQUIRE(state.Safe() == (previouslySafe || onlyDontMove));
        REQUIRE((std::string_view(state.VetoReason()) == "none") == state.Safe());
    }
}

TEST_CASE("Final transaction keeps logical identity and quarantines late old removal", "[final-respawn]")
{
    RemoteMaterializationLifecycle cycle;
    const MaterializationKey key{7, 0x100003, 3, 91};
    const MaterializationForms old{0xFF000011, 0xFF000012}, candidate{0xFF000013, 0xFF000014};
    REQUIRE(cycle.Reserve(key, old));
    REQUIRE_FALSE(cycle.Reserve(key, old));
    REQUIRE(cycle.RecordCandidate(key, candidate));
    REQUIRE(cycle.Observe(key, candidate.Actor, true) == DiscoveryRoute::CandidateStaged);
    REQUIRE(cycle.MarkReady(key));
    REQUIRE(cycle.Commit(key));
    REQUIRE_FALSE(cycle.Commit(key));
    REQUIRE(cycle.Key() == key);
    REQUIRE(cycle.TakeRetirementIntents(key) == 1);
    REQUIRE(cycle.TakeRetirementIntents(key) == 0);
    REQUIRE(cycle.Observe(key, old.Actor, false) == DiscoveryRoute::OldRetirementObserved);
    REQUIRE(cycle.BoundActor() == candidate.Actor);
    auto stale = key;
    ++stale.Generation;
    REQUIRE(cycle.Observe(stale, candidate.Actor, false) == DiscoveryRoute::Stale);
    REQUIRE(cycle.BoundActor() == candidate.Actor);
}

TEST_CASE("Final candidate failure preserves old binding and retires candidate once", "[final-respawn]")
{
    RemoteMaterializationLifecycle cycle;
    const MaterializationKey key{7, 0x100003, 3, 91};
    const MaterializationForms old{0xFF000011, 0xFF000012}, candidate{0xFF000013, 0xFF000014};
    REQUIRE(cycle.Reserve(key, old));
    REQUIRE(cycle.RecordCandidate(key, candidate));
    REQUIRE(cycle.Observe(key, candidate.Actor, true) == DiscoveryRoute::CandidateStaged);
    REQUIRE(cycle.Observe(key, candidate.Actor, false) == DiscoveryRoute::CandidateLostBeforeCommit);
    REQUIRE(cycle.BoundActor() == old.Actor);
    REQUIRE_FALSE(cycle.Commit(key));
    REQUIRE(cycle.TakeRetirementIntents(key) == 2);
    REQUIRE(cycle.TakeRetirementIntents(key) == 0);
}
