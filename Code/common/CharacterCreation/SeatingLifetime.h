#pragma once
#include <cstdint>

namespace STRE::CharacterCreation
{
// A bounded creation-cell observation, never a permanent player tracker. Keep
// the terminal tombstone so duplicate Applied cannot rearm it after departure.
struct SeatingLifetime
{
    bool Terminal{};
    bool Bound{};
    int64_t Started{};

    bool Observe(uint32_t aCreationCell, uint32_t aObserverCell, uint32_t aActorCell,
                 bool aCommitted, int64_t aNow) noexcept
    {
        if (Terminal) return false;
        if (!aCreationCell) return false;
        if (aObserverCell != aCreationCell || (aActorCell && aActorCell != aCreationCell) ||
            (Bound && !aCommitted) || (Bound && aNow - Started >= 40000))
        {
            Terminal = true;
            return false;
        }
        if (aCommitted && !Bound)
        {
            Bound = true;
            Started = aNow;
        }
        return aCommitted;
    }
};
}
