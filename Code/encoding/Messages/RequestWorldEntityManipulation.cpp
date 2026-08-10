#include <Messages/RequestWorldEntityManipulation.h>
#include <TiltedCore/Serialization.hpp>

using TiltedPhoques::Serialization;

void RequestWorldEntityManipulation::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, WorldEntityId);
    PlacedReferenceId.Serialize(aWriter);
    Serialization::WriteVarInt(aWriter, static_cast<std::uint8_t>(Action));
    Transform.Serialize(aWriter);
}

void RequestWorldEntityManipulation::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);
    WorldEntityId = Serialization::ReadVarInt(aReader);
    PlacedReferenceId.Deserialize(aReader);
    Action = static_cast<WorldEntityManipulationAction>(Serialization::ReadVarInt(aReader) & 0xFF);
    Transform.Deserialize(aReader);
}
