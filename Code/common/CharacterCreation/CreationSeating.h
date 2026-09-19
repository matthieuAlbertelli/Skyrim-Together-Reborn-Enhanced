#pragma once
#include <array>
#include <cstdint>
#include <optional>

namespace STRE::CharacterCreation
{
inline std::optional<uint32_t> CreationSeatLocalFormId(size_t aIndex) noexcept
{
    constexpr std::array<uint32_t, 10> seats{
        0x000BF3DD, 0x000BF3DC, 0x000C516E, 0x000C516F, 0x000C5173,
        0x000C5171, 0x000C5174, 0x000C5172, 0x000C5176, 0x000C5178};
    return aIndex < seats.size() ? std::optional<uint32_t>{seats[aIndex]} : std::nullopt;
}

enum class SeatAction { Pending, Conflict, Complete, Activate, AwaitEntry };
struct SeatProjection
{
    uint32_t Actor{};
    uintptr_t Token{};
    bool Issued{};
    bool Completed{};

    SeatAction Observe(uint32_t aActor, uintptr_t aToken, bool aReady, bool aCorrectFurniture,
                       bool aEntryConfirmed, bool aOtherFurniture, bool aOccupied) noexcept
    {
        if (!aReady || !aActor || !aToken)
            return SeatAction::Pending;
        if (Actor != aActor || Token != aToken)
        {
            Actor = aActor;
            Token = aToken;
            Issued = false;
            Completed = false;
        }
        if (Completed || (aCorrectFurniture && aEntryConfirmed))
        {
            Completed = true;
            return SeatAction::Complete;
        }
        if (aOtherFurniture || (aOccupied && !aCorrectFurniture && !Issued))
            return SeatAction::Conflict;
        if (Issued || aCorrectFurniture)
            return SeatAction::AwaitEntry;
        return SeatAction::Activate;
    }
};
}
