#include <Messages/CharacterBuildAppliedRequest.h>

#include <TiltedCore/Serialization.hpp>

void CharacterBuildAppliedRequest::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, Revision);
    Serialization::WriteVarInt(aWriter, InventoryHash);
    Serialization::WriteVarInt(aWriter, SpellHash);
}

void CharacterBuildAppliedRequest::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    Revision = Serialization::ReadVarInt(aReader);
    InventoryHash = Serialization::ReadVarInt(aReader);
    SpellHash = Serialization::ReadVarInt(aReader);
}
