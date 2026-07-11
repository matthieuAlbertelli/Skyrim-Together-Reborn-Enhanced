#include <Messages/TradeInviteResponseRequest.h>
#include <TiltedCore/Serialization.hpp>

void TradeInviteResponseRequest::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, SessionId);
    Serialization::WriteVarInt(aWriter, Accepted ? 1u : 0u);
}

void TradeInviteResponseRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);

    SessionId = Serialization::ReadVarInt(aReader);
    Accepted = (Serialization::ReadVarInt(aReader) & 1u) != 0;
}
