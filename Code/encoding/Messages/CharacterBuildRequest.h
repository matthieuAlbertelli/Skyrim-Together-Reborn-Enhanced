#pragma once

#include "Message.h"

#include <Structs/CharacterBuild.h>

struct CharacterBuildRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kCharacterBuildRequest;

    CharacterBuildRequest()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const CharacterBuildRequest& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
            BuildVersion == acRhs.BuildVersion &&
            RaceId == acRhs.RaceId &&
            ClassId == acRhs.ClassId &&
            Selections == acRhs.Selections;
    }

    std::uint32_t BuildVersion{};
    GameId RaceId{};
    String ClassId{};
    Vector<CharacterBuildSelectionData> Selections{};
};
