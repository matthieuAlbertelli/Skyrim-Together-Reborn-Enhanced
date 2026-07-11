#include <Messages/NotifyTradeReconcile.h>

#include <TiltedCore/Serialization.hpp>

void NotifyTradeReconcile::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, SessionId);
    Serialization::WriteVarInt(aWriter, Revision);
    Serialization::WriteVarInt(aWriter, ApplyId);
    Serialization::WriteVarInt(aWriter, ReconcileId);
    Plan.Serialize(aWriter);
}

void NotifyTradeReconcile::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    SessionId = Serialization::ReadVarInt(aReader);
    Revision = Serialization::ReadVarInt(aReader);
    ApplyId = Serialization::ReadVarInt(aReader);
    ReconcileId = Serialization::ReadVarInt(aReader);
    Plan.Deserialize(aReader);
}
