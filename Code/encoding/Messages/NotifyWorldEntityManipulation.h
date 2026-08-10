#pragma once

#include "Message.h"
#include <Structs/GameId.h>
#include <Structs/WorldEntity.h>

struct NotifyWorldEntityManipulation final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyWorldEntityManipulation;

    NotifyWorldEntityManipulation()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const NotifyWorldEntityManipulation& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() && WorldEntityId == acRhs.WorldEntityId &&
               PlacedReferenceId == acRhs.PlacedReferenceId && Action == acRhs.Action &&
               AuthorityPlayerId == acRhs.AuthorityPlayerId && Transform == acRhs.Transform;
    }

    std::uint64_t WorldEntityId{};
    GameId PlacedReferenceId{};
    WorldEntityManipulationAction Action{WorldEntityManipulationAction::Start};
    std::uint32_t AuthorityPlayerId{};
    WorldEntityTransform Transform{};
};
