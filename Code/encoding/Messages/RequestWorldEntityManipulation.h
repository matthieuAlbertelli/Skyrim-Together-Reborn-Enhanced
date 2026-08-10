#pragma once

#include "Message.h"
#include <Structs/GameId.h>
#include <Structs/WorldEntity.h>

struct RequestWorldEntityManipulation final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kRequestWorldEntityManipulation;

    RequestWorldEntityManipulation()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const RequestWorldEntityManipulation& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() && WorldEntityId == acRhs.WorldEntityId &&
               PlacedReferenceId == acRhs.PlacedReferenceId && Action == acRhs.Action && Transform == acRhs.Transform;
    }

    std::uint64_t WorldEntityId{};
    // Stable server-space identity of a placed TESObjectREFR. Set only when the
    // client needs the server to lazily adopt a reference that has no WorldEntity yet.
    GameId PlacedReferenceId{};
    WorldEntityManipulationAction Action{WorldEntityManipulationAction::Start};
    WorldEntityTransform Transform{};
};
