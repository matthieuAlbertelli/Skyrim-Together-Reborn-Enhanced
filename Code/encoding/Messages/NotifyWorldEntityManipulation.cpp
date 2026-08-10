#include <Messages/NotifyWorldEntityManipulation.h>
#include <TiltedCore/Serialization.hpp>

using TiltedPhoques::Serialization;

void NotifyWorldEntityManipulation::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, WorldEntityId);
    PlacedReferenceId.Serialize(aWriter);
    Serialization::WriteVarInt(aWriter, static_cast<std::uint8_t>(Action));
    Serialization::WriteVarInt(aWriter, AuthorityPlayerId);
    Transform.Serialize(aWriter);
}

void NotifyWorldEntityManipulation::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);
    WorldEntityId = Serialization::ReadVarInt(aReader);
    PlacedReferenceId.Deserialize(aReader);
    Action = static_cast<WorldEntityManipulationAction>(Serialization::ReadVarInt(aReader) & 0xFF);
    AuthorityPlayerId = static_cast<std::uint32_t>(Serialization::ReadVarInt(aReader));
    Transform.Deserialize(aReader);
}
