#pragma once
#include <CharacterCreation/StandingCreation.h>
#include <CharacterCreation/CreationSeating.h>

namespace STRE::CharacterCreation
{
inline const char* LocalSeatApproachRejection(bool aLocalFinalized, bool aNativePlayer,
                                            bool aRemoteActor, size_t aRank) noexcept
{
    if (!aLocalFinalized || !aNativePlayer || aRemoteActor)
        return "approach-native-local-player-only";
    if (aRank >= 10)
        return "approach-rank-out-of-range";
    return nullptr;
}

struct SeatApproachBinding
{
    uint32_t Seat{}, Marker{};
};

// Reuse the existing MarkerXX/SeatXX pair in durable rank order. Geometry is
// authored in CK; this mapping never substitutes another player's reference.
inline std::optional<SeatApproachBinding> ResolveSeatApproachBinding(size_t aRank) noexcept
{
    const auto seat = CreationSeatLocalFormId(aRank);
    const auto marker = CreationMarkerLocalFormId(aRank);
    if (!seat || !marker)
        return {};
    return SeatApproachBinding{*seat, *marker};
}

struct SeatApproachTarget
{
    glm::vec3 Position{}, Rotation{};
};

// Exact authored transform, with no furniture offsets or obstacle model.
// XMarkerHeading must be upright. C++ never repairs an invalid CK transform.
inline std::optional<SeatApproachTarget> SeatApproachMarkerTarget(
    glm::vec3 aPosition, glm::vec3 aRotation) noexcept
{
    if (!CreationMarkerTransformValid(aPosition, aRotation) ||
        std::abs(aRotation.x) > 0.0001f || std::abs(aRotation.y) > 0.0001f)
        return {};
    return SeatApproachTarget{aPosition, aRotation};
}

enum class SeatApproachAction { Wait, ApplyFacing, Activate, Reject };
struct LocalSeatApproach
{
    enum class Phase { None, Moving, Facing, Ready, Failed };
    Phase State{Phase::None};
    SeatApproachTarget Target{};
    uint64_t MoveUpdate{}, FacingUpdate{};
    int64_t StartedMs{};
    const char* Failure{};

    void Start(SeatApproachTarget aTarget, uint64_t aUpdate, int64_t aNow) noexcept
    {
        Target = aTarget;
        MoveUpdate = aUpdate;
        StartedMs = aNow;
        State = Phase::Moving;
    }
    SeatApproachAction Reject(const char* aReason) noexcept
    {
        if (!Failure)
            Failure = aReason;
        State = Phase::Failed;
        return SeatApproachAction::Reject;
    }
    SeatApproachAction Observe(uint64_t aUpdate, int64_t aNow, bool aIdentityValid,
                               bool aSameCell, glm::vec3 aPosition, glm::vec3 aRotation) noexcept
    {
        if (State == Phase::Failed)
            return SeatApproachAction::Reject;
        if (!aIdentityValid)
            return Reject("approach-binding-changed");
        if (!CreationMarkerTransformValid(aPosition, aRotation))
            return Reject("approach-nonfinite-transform");
        if (aNow - StartedMs >= 5000)
            return Reject("approach-arrival-or-facing-timeout");
        const auto delta = aPosition - Target.Position;
        const bool reached = aSameCell && delta.x * delta.x + delta.y * delta.y + delta.z * delta.z <= 4.f;
        if (State == Phase::Moving && aUpdate > MoveUpdate && reached)
        {
            State = Phase::Facing;
            FacingUpdate = aUpdate;
            return SeatApproachAction::ApplyFacing;
        }
        if (State == Phase::Facing && aUpdate > FacingUpdate)
        {
            if (!reached)
                return Reject("approach-position-lost-after-facing");
            if (std::abs(std::remainder(aRotation.x - Target.Rotation.x, 6.283185307f)) > 0.05f ||
                std::abs(std::remainder(aRotation.y - Target.Rotation.y, 6.283185307f)) > 0.05f ||
                std::abs(std::remainder(aRotation.z - Target.Rotation.z, 6.283185307f)) > 0.05f)
                return Reject("approach-facing-not-observed");
            State = Phase::Ready;
            return SeatApproachAction::Activate;
        }
        return SeatApproachAction::Wait;
    }
};
}
