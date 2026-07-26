#include <Messages/CharacterBuildRequest.h>

#include <TiltedCore/Serialization.hpp>

#include <algorithm>
#include <utility>

void CharacterBuildRequest::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, BuildVersion);
    RaceId.Serialize(aWriter);
    Serialization::WriteString(aWriter, ClassId);
    Serialization::WriteVarInt(aWriter, Selections.size());
    for (const CharacterBuildSelectionData& selection : Selections)
        selection.Serialize(aWriter);
}

void CharacterBuildRequest::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);

    BuildVersion = static_cast<std::uint32_t>(
        Serialization::ReadVarInt(aReader));
    RaceId.Deserialize(aReader);
    ClassId = Serialization::ReadString(aReader);

    Selections.clear();
    const std::uint64_t selectionCount =
        Serialization::ReadVarInt(aReader);
    Selections.reserve(
        static_cast<std::size_t>(
            std::min<std::uint64_t>(selectionCount, 64)));

    for (std::uint64_t i = 0; i < selectionCount; ++i)
    {
        CharacterBuildSelectionData selection;
        selection.Deserialize(aReader);
        if (i < 64)
            Selections.push_back(std::move(selection));
    }
}
