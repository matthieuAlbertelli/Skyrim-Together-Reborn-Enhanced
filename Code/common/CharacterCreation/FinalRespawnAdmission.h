#pragma once
#include <CharacterCreation/FinalRespawn.h>
#include <chrono>
#include <optional>

namespace STRE::CharacterCreation
{
enum class FinalRespawnAdmissionDecision
{
    Ready,
    Pending,
    Rejected
};
struct FinalRespawnAdmissionResult
{
    FinalRespawnAdmissionDecision Decision;
    const char* Reason;
};

// Value-only fence. Tokens are compared with a fresh lookup, never dereferenced.
struct FinalRespawnAdmissionBinding
{
    uint64_t Session{};
    uint32_t Entity{}, Server{}, Player{}, Actor{}, Base{};
    uintptr_t ActorToken{}, BaseToken{};
    bool operator==(const FinalRespawnAdmissionBinding&) const = default;
};

class FinalRespawnAdmission
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto Budget = std::chrono::seconds(10);

    void Begin(Clock::time_point aNow) noexcept
    {
        if (!m_started && !m_closed)
            m_started = aNow;
    }
    bool Waiting() const noexcept { return m_binding.has_value(); }
    auto Elapsed(Clock::time_point aNow) const noexcept { return m_started ? aNow - *m_started : Clock::duration::zero(); }
    bool Expired(Clock::time_point aNow) const noexcept { return m_started && Elapsed(aNow) >= Budget; }
    FinalRespawnAdmissionResult Reject(const char* aReason) noexcept
    {
        m_closed = true;
        return {FinalRespawnAdmissionDecision::Rejected, aReason};
    }
    FinalRespawnAdmissionResult
    Observe(Clock::time_point aNow, const FinalRespawnAdmissionBinding& aBinding, const FinalRespawnActorState& aState, const char* aOtherGuardFailure = nullptr) noexcept
    {
        if (m_closed)
            return {FinalRespawnAdmissionDecision::Rejected, "admission-already-consumed"};
        if (!m_started)
            return Reject("admission-not-started");
        if (aOtherGuardFailure)
            return Reject(aOtherGuardFailure);
        if (m_binding && *m_binding != aBinding)
            return Reject("pre-reservation-binding-changed");

        // Do not let a transitional weapon hide another unsafe state, including
        // predicates that Safe()/VetoReason() evaluate AFTER WeaponSheathed().
        if (!aState.LifeAllowsReplacement())
            return Reject(aState.Life <= 8 ? "life-transition-or-incapacitated" : "life-unknown");
        if (!aState.KnockIdle())
            return Reject("knock-transition-active");
        if (!aState.AttackIdle())
            return Reject("attack-active");
        if (!aState.FlyIdle())
            return Reject("fly-state-outside-quiescent-lab");
        if (!aState.RecoilIdle())
            return Reject("recoil-active");
        if (!aState.NotStaggered())
            return Reject("stagger-active");
        if (!aState.NotSprinting())
            return Reject("sprinting-outside-quiescent-lab");
        if (!aState.NotSwimming())
            return Reject("swimming-outside-quiescent-lab");
        const bool sheathing = aState.Weapon == 4 || aState.Weapon == 5;
        if (!aState.WeaponSheathed() && !sheathing)
            return Reject("weapon-state-outside-quiescent-lab");
        if (Expired(aNow))
            return Reject(sheathing ? "weapon-sheathing-timeout" : "pre-reservation-timeout");
        if (aState.Safe())
        {
            m_closed = true; // Ready is consumed once, before the one reservation.
            return {FinalRespawnAdmissionDecision::Ready, "all-guards-passed"};
        }
        m_binding = aBinding;
        return {FinalRespawnAdmissionDecision::Pending, "weapon-sheathing-pending"};
    }

private:
    std::optional<Clock::time_point> m_started;
    std::optional<FinalRespawnAdmissionBinding> m_binding;
    bool m_closed{};
};
} // namespace STRE::CharacterCreation
