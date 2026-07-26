#pragma once

#include "Message.h"

#include <cstdint>

struct CharacterBuildAppliedRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCharacterBuildAppliedRequest;

    CharacterBuildAppliedRequest()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    std::uint64_t Revision{};
    std::uint64_t InventoryHash{};
};
