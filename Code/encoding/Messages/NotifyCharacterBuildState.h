#pragma once

#include "Message.h"

#include <Structs/CharacterBuild.h>

struct NotifyCharacterBuildState final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyCharacterBuildState;

    NotifyCharacterBuildState()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    CharacterBuildNetworkState State{CharacterBuildNetworkState::Accepted};
    std::uint32_t PlayerId{};
    std::uint32_t ServerId{};
    std::uint64_t Revision{};
    CharacterBuildSnapshotData Build{};
};
