#include <Messages/NotifyInventoryChanges.h>
#include <TiltedCore/Serialization.hpp>

void NotifyInventoryChanges::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, ServerId);
    Item.Serialize(aWriter);
    Serialization::WriteBool(aWriter, Drop);
    Serialization::WriteVarInt(aWriter, WorldEntityId);
    Serialization::WriteBool(aWriter, BindOnly);
    Serialization::WriteVarInt(aWriter, OriginFormId);
    Serialization::WriteBool(aWriter, Snapshot);
    Serialization::WriteBool(aWriter, TransformUpdate);
    Serialization::WriteBool(aWriter, HasTransform);
    Serialization::WriteFloat(aWriter, PositionX);
    Serialization::WriteFloat(aWriter, PositionY);
    Serialization::WriteFloat(aWriter, PositionZ);
    Serialization::WriteFloat(aWriter, RotationX);
    Serialization::WriteFloat(aWriter, RotationY);
    Serialization::WriteFloat(aWriter, RotationZ);
}

void NotifyInventoryChanges::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    ServerId = Serialization::ReadVarInt(aReader) & 0xFFFFFFFF;
    Item.Deserialize(aReader);
    Drop = Serialization::ReadBool(aReader);
    WorldEntityId = Serialization::ReadVarInt(aReader);
    BindOnly = Serialization::ReadBool(aReader);
    OriginFormId = Serialization::ReadVarInt(aReader) & 0xFFFFFFFF;
    Snapshot = Serialization::ReadBool(aReader);
    TransformUpdate = Serialization::ReadBool(aReader);
    HasTransform = Serialization::ReadBool(aReader);
    PositionX = Serialization::ReadFloat(aReader);
    PositionY = Serialization::ReadFloat(aReader);
    PositionZ = Serialization::ReadFloat(aReader);
    RotationX = Serialization::ReadFloat(aReader);
    RotationY = Serialization::ReadFloat(aReader);
    RotationZ = Serialization::ReadFloat(aReader);
}
