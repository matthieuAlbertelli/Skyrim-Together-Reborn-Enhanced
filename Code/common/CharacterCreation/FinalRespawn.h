#pragma once
#include <cstdint>

namespace STRE::CharacterCreation
{
// Only an authoritative Applied build can identify the one initial final.
// Appearance transition type deliberately is not an input.
inline bool FinalRespawnEligible(bool aConnected, bool aOfficialCreation, bool aApplied, uint64_t aFinalRevision, uint64_t aAppliedRevision) noexcept
{
    return aConnected && aOfficialCreation && aApplied && aFinalRevision && aFinalRevision == aAppliedRevision;
}

// Read-only policy for the 1.6.1170 LAB, not a general native Actor safety API.
// CommonLibSSE-NG field layout; runtime adapter must check exact 1.6.1170.
// Sit/sleep bits 14:4 are deliberately irrelevant, including WantToStand.
struct FinalRespawnActorState
{
    uint32_t Life{}, Knock{}, Attack{}, Fly{}, Weapon{}, Recoil{};
    bool Staggered{}, Sprinting{}, Swimming{};

    // Exact-build RE: Actor.SetDontMove(true), RVA 9EA930 -> 6750D0,
    // sets life 9 through 680740. It is movement inhibition, not knock/death.
    // See CHARACTER_APPEARANCE_SYNC.md for the evidence and scope of admission.
    constexpr bool LifeAllowsReplacement() const noexcept { return Life == 0 || Life == 9; }
    constexpr bool KnockIdle() const noexcept { return Knock == 0; }
    constexpr bool AttackIdle() const noexcept { return Attack == 0; }
    constexpr bool FlyIdle() const noexcept { return Fly == 0; }
    constexpr bool WeaponSheathed() const noexcept { return Weapon == 0; }
    constexpr bool RecoilIdle() const noexcept { return Recoil == 0; }
    constexpr bool NotStaggered() const noexcept { return !Staggered; }
    constexpr bool NotSprinting() const noexcept { return !Sprinting; }
    constexpr bool NotSwimming() const noexcept { return !Swimming; }

    constexpr bool Safe() const noexcept
    {
        return LifeAllowsReplacement() && KnockIdle() && AttackIdle() && FlyIdle() && WeaponSheathed() && RecoilIdle() && NotStaggered() && NotSprinting() && NotSwimming();
    }
    constexpr const char* LifeName() const noexcept
    {
        switch (Life)
        {
        case 0: return "alive";
        case 1: return "dying";
        case 2: return "dead";
        case 3: return "unconscious";
        case 4: return "reanimate";
        case 5: return "recycle";
        case 6: return "restrained";
        case 7: return "essential-down";
        case 8: return "bleedout";
        case 9: return "dont-move";
        default: return "unknown";
        }
    }
    constexpr const char* VetoReason() const noexcept
    {
        if (!LifeAllowsReplacement())
            return Life <= 8 ? "life-transition-or-incapacitated" : "life-unknown";
        if (!KnockIdle())
            return "knock-transition-active";
        if (!AttackIdle())
            return "attack-active";
        if (!FlyIdle())
            return "fly-state-outside-quiescent-lab";
        if (!WeaponSheathed())
            return "weapon-state-outside-quiescent-lab";
        if (!RecoilIdle())
            return "recoil-active";
        if (!NotStaggered())
            return "stagger-active";
        // These existing movement restrictions are conservative LAB limits,
        // not evidence that ordinary locomotion makes native deletion unsafe.
        if (!NotSprinting())
            return "sprinting-outside-quiescent-lab";
        if (!NotSwimming())
            return "swimming-outside-quiescent-lab";
        return "none";
    }
};

constexpr FinalRespawnActorState DecodeFinalRespawnActorState(uint32_t aFlags1, uint32_t aFlags2) noexcept
{
    return {(aFlags1 >> 21) & 15u, (aFlags1 >> 25) & 7u,        (aFlags1 >> 28) & 15u,      (aFlags1 >> 18) & 7u,       (aFlags2 >> 5) & 7u,
            (aFlags2 >> 10) & 3u,  (aFlags2 & (1u << 13)) != 0, (aFlags1 & (1u << 8)) != 0, (aFlags1 & (1u << 10)) != 0};
}

constexpr bool FinalRespawnActorStateSafe(uint32_t aFlags1, uint32_t aFlags2) noexcept
{
    return DecodeFinalRespawnActorState(aFlags1, aFlags2).Safe();
}
} // namespace STRE::CharacterCreation
