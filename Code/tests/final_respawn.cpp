#include <CharacterCreation/FinalRespawn.h>
#include <CharacterCreation/FinalRespawnAdmission.h>
#include <CharacterCreation/RemoteMaterializationLifecycle.h>
#include <catch2/catch.hpp>
#include <string_view>

using namespace STRE::CharacterCreation;

namespace
{
using AdmissionClock = FinalRespawnAdmission::Clock;
const AdmissionClock::time_point admissionStart{};
const FinalRespawnAdmissionBinding admissionBinding{7, 0x100003, 3, 2, 0xFF000011, 0xFF000012, 0x1234, 0x5678};
struct AdmissionTransaction
{
    FinalRespawnAdmission Admission;
    RemoteMaterializationLifecycle Lifecycle;
    unsigned Reservations{}, Creations{};
    AdmissionTransaction() { Admission.Begin(admissionStart); }
    FinalRespawnAdmissionResult Update(int aMilliseconds, uint32_t aWeapon, const char* aGuard = nullptr,
                                       FinalRespawnAdmissionBinding aBinding = admissionBinding, uint32_t aExtraFlags1 = 0, uint32_t aExtraFlags2 = 0)
    {
        auto result = Admission.Observe(admissionStart + std::chrono::milliseconds(aMilliseconds), aBinding,
            DecodeFinalRespawnActorState(0x41 | aExtraFlags1, 0x1008 | (aWeapon << 5) | aExtraFlags2), aGuard);
        if (result.Decision == FinalRespawnAdmissionDecision::Ready)
        {
            const MaterializationKey key{aBinding.Session, aBinding.Entity, aBinding.Server, 91};
            REQUIRE(Lifecycle.Reserve(key, {aBinding.Actor, aBinding.Base}));
            ++Reservations;
            REQUIRE(Lifecycle.RecordCandidate(key, {0xFF000013, 0xFF000014}));
            ++Creations;
        }
        return result;
    }
};
}

TEST_CASE("Pre-reservation admission preserves the immediate safe path and consumes Ready once", "[final-respawn]")
{
    AdmissionTransaction flow;
    REQUIRE(flow.Update(0, 0).Decision == FinalRespawnAdmissionDecision::Ready);
    REQUIRE(flow.Reservations == 1);
    REQUIRE(flow.Creations == 1);
    REQUIRE(flow.Update(1, 0).Decision == FinalRespawnAdmissionDecision::Rejected);
    REQUIRE(flow.Reservations == 1);
    REQUIRE(flow.Creations == 1);
}

TEST_CASE("WantToSheathe waits for actual sheathed state with or without intermediate Sheathing", "[final-respawn]")
{
    for (bool intermediate : {false, true})
    {
        AdmissionTransaction flow;
        REQUIRE(flow.Update(0, 4).Decision == FinalRespawnAdmissionDecision::Pending);
        REQUIRE(flow.Lifecycle.State() == MaterializationState::Idle);
        REQUIRE_FALSE(FinalRespawnActorStateSafe(0x41, 0x1088));
        if (intermediate)
        {
            REQUIRE(flow.Update(100, 5).Decision == FinalRespawnAdmissionDecision::Pending);
            REQUIRE_FALSE(FinalRespawnActorStateSafe(0x41, 0x10A8));
        }
        REQUIRE(flow.Reservations == 0);
        REQUIRE(flow.Creations == 0);
        REQUIRE(flow.Lifecycle.TakeRetirementIntents(flow.Lifecycle.Key()) == 0);
        REQUIRE(flow.Update(200, 0).Decision == FinalRespawnAdmissionDecision::Ready);
        REQUIRE(flow.Reservations == 1);
        REQUIRE(flow.Creations == 1);
        // Exercise the existing pure lifecycle through commit after admission.
        // Native projection/readiness and the visible result still require Skyrim.
        const auto key = flow.Lifecycle.Key();
        REQUIRE(flow.Lifecycle.BoundActor() == admissionBinding.Actor);
        REQUIRE_FALSE(flow.Lifecycle.Commit(key)); // Discovery/readiness cannot be skipped.
        REQUIRE(flow.Lifecycle.Observe(key, 0xFF000013, true) == DiscoveryRoute::CandidateStaged);
        REQUIRE(flow.Lifecycle.MarkReady(key));
        REQUIRE(flow.Lifecycle.Commit(key));
        REQUIRE(flow.Lifecycle.BoundActor() == 0xFF000013);
        REQUIRE_FALSE(flow.Lifecycle.Commit(key));
        REQUIRE(flow.Lifecycle.TakeRetirementIntents(key) == 1); // Old only, after commit.
        REQUIRE(flow.Lifecycle.TakeRetirementIntents(key) == 0);
        flow.Admission.Begin(admissionStart + std::chrono::seconds(9)); // Duplicate final cannot rearm.
        REQUIRE(flow.Update(300, 0).Decision == FinalRespawnAdmissionDecision::Rejected);
        REQUIRE(flow.Lifecycle.State() == MaterializationState::Committed);
        REQUIRE(flow.Reservations == 1);
        REQUIRE(flow.Creations == 1);
    }
}

TEST_CASE("Persistent sheathing and duplicate finals share the original Applied deadline", "[final-respawn]")
{
    for (uint32_t weapon : {4u, 5u})
    {
        AdmissionTransaction flow;
        // Nine seconds consumed waiting for matching Applied; no new weapon budget.
        flow.Admission.Begin(admissionStart + std::chrono::seconds(9));
        REQUIRE(flow.Update(9000, weapon).Decision == FinalRespawnAdmissionDecision::Pending);
        REQUIRE(flow.Update(9999, weapon).Decision == FinalRespawnAdmissionDecision::Pending);
        flow.Admission.Begin(admissionStart + std::chrono::milliseconds(9999));
        const auto expired = flow.Update(10000, weapon);
        REQUIRE(expired.Decision == FinalRespawnAdmissionDecision::Rejected);
        REQUIRE(std::string_view(expired.Reason) == "weapon-sheathing-timeout");
        REQUIRE(flow.Update(10001, 0).Decision == FinalRespawnAdmissionDecision::Rejected);
        REQUIRE(flow.Reservations == 0);
        REQUIRE(flow.Creations == 0);
        REQUIRE(flow.Lifecycle.State() == MaterializationState::Idle);
        REQUIRE(flow.Lifecycle.TakeRetirementIntents(flow.Lifecycle.Key()) == 0);
    }
    AdmissionTransaction lateSafe;
    REQUIRE(lateSafe.Update(10000, 0).Decision == FinalRespawnAdmissionDecision::Rejected);
    REQUIRE(lateSafe.Creations == 0);
}

TEST_CASE("Sheathing cannot hide a binding canonical runtime disconnect or recovery failure", "[final-respawn]")
{
    for (const auto* guard : {"provenance-mismatch", "unresolvable-final-race", "unsupported-runtime", "canonical-final-changed",
                             "cachedref-mismatch", "actor-lookup-failed", "assignment-pending", "waiting-for-3d", "transport-disconnected", "recovery-locked"})
        for (bool alreadyWaiting : {false, true})
        {
            CAPTURE(guard, alreadyWaiting);
            AdmissionTransaction flow;
            if (alreadyWaiting)
                REQUIRE(flow.Update(0, 4).Decision == FinalRespawnAdmissionDecision::Pending);
            const auto result = flow.Update(100, 4, guard);
            REQUIRE(result.Decision == FinalRespawnAdmissionDecision::Rejected);
            REQUIRE(std::string_view(result.Reason) == guard);
            REQUIRE(flow.Update(200, 0).Decision == FinalRespawnAdmissionDecision::Rejected);
            REQUIRE(flow.Reservations == 0);
            REQUIRE(flow.Creations == 0);
            REQUIRE(flow.Lifecycle.TakeRetirementIntents(flow.Lifecycle.Key()) == 0);
        }
}

TEST_CASE("Every pre-reservation identity and native token is fenced across observations", "[final-respawn]")
{
    for (unsigned field = 0; field != 8; ++field)
    {
        AdmissionTransaction flow;
        REQUIRE(flow.Update(0, 4).Decision == FinalRespawnAdmissionDecision::Pending);
        auto changed = admissionBinding;
        switch (field)
        {
        case 0: ++changed.Session; break;
        case 1: changed.Entity += 0x100000; break; // EnTT version, same logical index.
        case 2: ++changed.Server; break;
        case 3: ++changed.Player; break;
        case 4: ++changed.Actor; break;
        case 5: ++changed.Base; break;
        case 6: ++changed.ActorToken; break;
        case 7: ++changed.BaseToken; break;
        }
        const auto result = flow.Update(100, 0, nullptr, changed);
        REQUIRE(result.Decision == FinalRespawnAdmissionDecision::Rejected);
        REQUIRE(std::string_view(result.Reason) == "pre-reservation-binding-changed");
        REQUIRE(flow.Creations == 0);
    }
}

TEST_CASE("Pending weapons do not mask any other ActorState veto", "[final-respawn]")
{
    for (uint32_t weapon = 0; weapon != 8; ++weapon)
    {
        for (const auto flags : {std::pair{1u << 21, 0u}, {1u << 25, 0u}, {1u << 28, 0u}, {1u << 18, 0u},
                                {0u, 1u << 10}, {0u, 1u << 13}, {1u << 8, 0u}, {1u << 10, 0u}})
        {
            AdmissionTransaction flow;
            REQUIRE(flow.Update(0, weapon, nullptr, admissionBinding, flags.first, flags.second).Decision == FinalRespawnAdmissionDecision::Rejected);
            REQUIRE(flow.Creations == 0);
        }
        if (weapon != 0 && weapon != 4 && weapon != 5)
        {
            AdmissionTransaction flow;
            REQUIRE(flow.Update(0, weapon).Decision == FinalRespawnAdmissionDecision::Rejected);
        }
    }
}

TEST_CASE("New danger after reservation still aborts instead of readmitting a transaction", "[final-respawn]")
{
    for (uint32_t flags2 : {4u << 5, 5u << 5, 1u << 10, 1u << 13})
    {
        AdmissionTransaction flow;
        REQUIRE(flow.Update(0, 0).Decision == FinalRespawnAdmissionDecision::Ready);
        REQUIRE_FALSE(FinalRespawnActorStateSafe(0x41, 0x1008 | flags2));
        const auto key = flow.Lifecycle.Key();
        flow.Lifecycle.Abort(key);
        REQUIRE_FALSE(flow.Lifecycle.Commit(key));
        REQUIRE(flow.Lifecycle.TakeRetirementIntents(key) == 2); // Candidate only, never old.
        REQUIRE(flow.Update(100, 0).Decision == FinalRespawnAdmissionDecision::Rejected);
        REQUIRE(flow.Creations == 1);
    }
}

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
