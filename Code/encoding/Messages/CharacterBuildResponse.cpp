#include <Messages/CharacterBuildResponse.h>

#include <TiltedCore/Serialization.hpp>

void CharacterBuildResponse::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(
        aWriter,
        static_cast<std::uint8_t>(Result));
    Serialization::WriteVarInt(aWriter, Revision);
    Serialization::WriteVarInt(aWriter, ServerId);
    Build.Serialize(aWriter);
}

void CharacterBuildResponse::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    Result = static_cast<CharacterBuildResult>(
        static_cast<std::uint8_t>(
            Serialization::ReadVarInt(aReader)));
    Revision = Serialization::ReadVarInt(aReader);
    ServerId = static_cast<std::uint32_t>(
        Serialization::ReadVarInt(aReader));
    Build.Deserialize(aReader);
}
