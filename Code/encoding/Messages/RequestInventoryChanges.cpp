#include <Messages/RequestInventoryChanges.h>
#include <TiltedCore/Serialization.hpp>

void RequestInventoryChanges::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, ServerId);
    Item.Serialize(aWriter);
    Serialization::WriteBool(aWriter, Drop);
    Serialization::WriteBool(aWriter, UpdateClients);
    Serialization::WriteVarInt(aWriter, DroppedFormId);
    Serialization::WriteVarInt(aWriter, WorldEntityId);
    Serialization::WriteBool(aWriter, TransformUpdate);
    Serialization::WriteFloat(aWriter, PositionX);
    Serialization::WriteFloat(aWriter, PositionY);
    Serialization::WriteFloat(aWriter, PositionZ);
    Serialization::WriteFloat(aWriter, RotationX);
    Serialization::WriteFloat(aWriter, RotationY);
    Serialization::WriteFloat(aWriter, RotationZ);
}

void RequestInventoryChanges::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);

    ServerId = Serialization::ReadVarInt(aReader) & 0xFFFFFFFF;
    Item.Deserialize(aReader);
    Drop = Serialization::ReadBool(aReader);
    UpdateClients = Serialization::ReadBool(aReader);
    DroppedFormId = Serialization::ReadVarInt(aReader) & 0xFFFFFFFF;
    WorldEntityId = Serialization::ReadVarInt(aReader);
    TransformUpdate = Serialization::ReadBool(aReader);
    PositionX = Serialization::ReadFloat(aReader);
    PositionY = Serialization::ReadFloat(aReader);
    PositionZ = Serialization::ReadFloat(aReader);
    RotationX = Serialization::ReadFloat(aReader);
    RotationY = Serialization::ReadFloat(aReader);
    RotationZ = Serialization::ReadFloat(aReader);
}
