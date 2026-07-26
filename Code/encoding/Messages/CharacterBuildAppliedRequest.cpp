#include <Messages/CharacterBuildAppliedRequest.h>

#include <TiltedCore/Serialization.hpp>

void CharacterBuildAppliedRequest::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, Revision);
    Serialization::WriteVarInt(aWriter, InventoryHash);
}

void CharacterBuildAppliedRequest::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    Revision = Serialization::ReadVarInt(aReader);
    InventoryHash = Serialization::ReadVarInt(aReader);
}
