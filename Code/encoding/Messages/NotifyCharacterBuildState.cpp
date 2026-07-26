#include <Messages/NotifyCharacterBuildState.h>

#include <TiltedCore/Serialization.hpp>

void NotifyCharacterBuildState::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(
        aWriter,
        static_cast<std::uint8_t>(State));
    Serialization::WriteVarInt(aWriter, PlayerId);
    Serialization::WriteVarInt(aWriter, ServerId);
    Serialization::WriteVarInt(aWriter, Revision);
    Build.Serialize(aWriter);
}

void NotifyCharacterBuildState::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    State = static_cast<CharacterBuildNetworkState>(
        static_cast<std::uint8_t>(
            Serialization::ReadVarInt(aReader)));
    PlayerId = static_cast<std::uint32_t>(
        Serialization::ReadVarInt(aReader));
    ServerId = static_cast<std::uint32_t>(
        Serialization::ReadVarInt(aReader));
    Revision = Serialization::ReadVarInt(aReader);
    Build.Deserialize(aReader);
}
