#include <Messages/NotifyTradeApply.h>

#include <TiltedCore/Serialization.hpp>

void NotifyTradeApply::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, SessionId);
    Serialization::WriteVarInt(aWriter, Revision);
    Serialization::WriteVarInt(aWriter, ApplyId);
    Plan.Serialize(aWriter);
}

void NotifyTradeApply::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    SessionId = Serialization::ReadVarInt(aReader);
    Revision = Serialization::ReadVarInt(aReader);
    ApplyId = Serialization::ReadVarInt(aReader);
    Plan.Deserialize(aReader);
}
