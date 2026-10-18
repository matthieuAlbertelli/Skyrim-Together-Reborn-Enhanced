#pragma once
#include <cstdint>

namespace STRE::CharacterCreation
{
// Value-only correlation; tokens are never retained for native dereference.
struct SeatingEntryIdentity
{
    uint32_t Actor{}, Seat{};
    uintptr_t ActorToken{}, SeatToken{};

    bool Matches(const SeatingEntryIdentity& aOther) const noexcept
    {
        return Actor && Seat && ActorToken && SeatToken &&
               Actor == aOther.Actor && Seat == aOther.Seat &&
               ActorToken == aOther.ActorToken && SeatToken == aOther.SeatToken;
    }
};

// Engine observations, not a claim about the rendered pose. Runtime acceptance
// still requires seeing the player sit and use the camera normally.
struct SeatingEntryEvidence
{
    bool CorrectFurniture{};
    bool LogicalSitState{};
    bool RootReady{};
    bool GraphReady{};
    bool EnterEvent{};
    bool FurnitureVariableValid{};
    bool InFurniture{};
    bool SittingVariableValid{};
    bool IdleSitting{};
    bool IdentityMatches{};

    bool LogicalSeated() const noexcept { return CorrectFurniture && LogicalSitState; }
    bool EntryConfirmed() const noexcept
    {
        // isIdleSitting is diagnostic only: the native chair can enter with it
        // false or unavailable while all required observations agree.
        return IdentityMatches && LogicalSeated() && RootReady && GraphReady && EnterEvent &&
               FurnitureVariableValid && InFurniture;
    }
};
}
