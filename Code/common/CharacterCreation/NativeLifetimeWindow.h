#pragma once
#include <cstdint>

namespace STRE::CharacterCreation
{
// Milliseconds relative to the armed creation. No engine lifetime interpretation.
struct NativeLifetimeWindow
{
    static constexpr uint64_t MaxObservationMs = 180000;
    static constexpr uint64_t PostRemovalMs = 30000;
    static constexpr uint64_t PollIntervalMs = 100;
    uint64_t LastPollMs{};
    uint64_t RemovalMs{};
    bool RemovalObserved{};

    void ObserveRemoval(uint64_t aNow) noexcept
    {
        if (!RemovalObserved)
        {
            RemovalObserved = true;
            RemovalMs = aNow;
        }
    }
    bool Expired(uint64_t aNow) const noexcept { return aNow >= MaxObservationMs || (RemovalObserved && aNow >= RemovalMs && aNow - RemovalMs >= PostRemovalMs); }
    bool PollDue(uint64_t aNow) noexcept
    {
        if (Expired(aNow) || aNow < LastPollMs || aNow - LastPollMs < PollIntervalMs)
            return false;
        LastPollMs = aNow;
        return true;
    }
};
} // namespace STRE::CharacterCreation
