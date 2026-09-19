#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <glm/vec3.hpp>

namespace STRE::CharacterCreation
{
struct StandingCreationPositionResult
{
    std::optional<size_t> Index;
    const char* Reason{};
};

// Temporary placement contract: rank sealed durable PlayerIds. Sort a local copy;
// never mutate roster order, use SlotIds or transient connection/player numbers.
inline StandingCreationPositionResult ResolveStandingCreationPositionIndex(std::span<const std::string_view> aPlayerIds, std::string_view aLocalPlayerId) noexcept
{
    if (aLocalPlayerId.empty())
        return {{}, "missing-local-player-id"};
    if (aPlayerIds.empty())
        return {{}, "sealed-roster-unavailable"};
    if (aPlayerIds.size() > 10)
        return {{}, "creation-position-index-out-of-range"};
    std::array<std::string_view, 10> sorted;
    const auto end = std::copy(aPlayerIds.begin(), aPlayerIds.end(), sorted.begin());
    if (std::find(sorted.begin(), end, std::string_view{}) != end)
        return {{}, "missing-roster-player-id"};
    std::sort(sorted.begin(), end);
    if (std::adjacent_find(sorted.begin(), end) != end)
        return {{}, "duplicate-player-id"};
    const auto found = std::lower_bound(sorted.begin(), end, aLocalPlayerId);
    if (found == end || *found != aLocalPlayerId)
        return {{}, "local-player-not-in-roster"};
    return {static_cast<size_t>(found - sorted.begin()), nullptr};
}

inline std::optional<size_t> StandingCreationPositionIndex(std::span<const std::string_view> aPlayerIds, std::string_view aLocalPlayerId) noexcept
{
    return ResolveStandingCreationPositionIndex(aPlayerIds, aLocalPlayerId).Index;
}

// Plugin-local XMarkerHeading references, in PlayerCreationMarker01..10 order.
// Resolve through the loaded plugin; these are never loaded FormIDs.
inline std::optional<uint32_t> CreationMarkerLocalFormId(size_t aIndex) noexcept
{
    constexpr std::array<uint32_t, 10> markers{
        0x000D6B08, 0x000D6B09, 0x000D6B13, 0x000D6B12, 0x000D6B0A,
        0x000D6B11, 0x000D6B0B, 0x000D6B10, 0x000D6B0D, 0x000D6B0F};
    return aIndex < markers.size() ? std::optional<uint32_t>{markers[aIndex]} : std::nullopt;
}

inline bool CreationMarkerTransformValid(glm::vec3 aPosition, glm::vec3 aRotation) noexcept
{
    return std::isfinite(aPosition.x) && std::isfinite(aPosition.y) && std::isfinite(aPosition.z)
        && std::isfinite(aRotation.x) && std::isfinite(aRotation.y) && std::isfinite(aRotation.z);
}

// Validate the move geometrically, without consulting or normalizing posture.
inline bool CreationPositionReached(glm::vec3 aActual, glm::vec3 aTarget) noexcept
{
    constexpr float tolerance = 32.f;
    const auto delta = aActual - aTarget;
    return std::isfinite(delta.x) && std::isfinite(delta.y) && std::isfinite(delta.z) && delta.x * delta.x + delta.y * delta.y + delta.z * delta.z <= tolerance * tolerance;
}

enum class CreationMoveStatus
{
    Pending,
    Reached,
    TimedOut,
    InvalidPosition
};
inline CreationMoveStatus ObserveCreationMove(bool aSameCell, glm::vec3 aActual, glm::vec3 aTarget, double aElapsedSeconds) noexcept
{
    const auto delta = aActual - aTarget;
    if (!std::isfinite(delta.x) || !std::isfinite(delta.y) || !std::isfinite(delta.z))
        return CreationMoveStatus::InvalidPosition;
    if (aSameCell && CreationPositionReached(aActual, aTarget))
        return CreationMoveStatus::Reached;
    return aElapsedSeconds >= 5.0 ? CreationMoveStatus::TimedOut : CreationMoveStatus::Pending;
}
} // namespace STRE::CharacterCreation
