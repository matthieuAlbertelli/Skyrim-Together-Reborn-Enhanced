#pragma once

#include "Message.h"

#include <Structs/CharacterBuild.h>

struct CharacterBuildResponse final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kCharacterBuildResponse;

    CharacterBuildResponse()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    CharacterBuildResult Result{CharacterBuildResult::Accepted};
    std::uint64_t Revision{};
    std::uint32_t ServerId{};
    CharacterBuildSnapshotData Build{};
};
