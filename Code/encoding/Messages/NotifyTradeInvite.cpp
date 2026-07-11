#include <Messages/NotifyTradeInvite.h>
#include <TiltedCore/Serialization.hpp>

void NotifyTradeInvite::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, SessionId);
    Serialization::WriteVarInt(aWriter, InviterId);
    Serialization::WriteVarInt(aWriter, ExpiryTick);
}

void NotifyTradeInvite::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    SessionId = Serialization::ReadVarInt(aReader);
    InviterId = Serialization::ReadVarInt(aReader) & 0xFFFFFFFF;
    ExpiryTick = Serialization::ReadVarInt(aReader);
}
