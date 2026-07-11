#include <Messages/NotifyTradeStarted.h>
#include <TiltedCore/Serialization.hpp>

void NotifyTradeStarted::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, SessionId);
    Serialization::WriteVarInt(aWriter, InitiatorId);
    Serialization::WriteVarInt(aWriter, RecipientId);
    Serialization::WriteVarInt(aWriter, Revision);
}

void NotifyTradeStarted::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    SessionId = Serialization::ReadVarInt(aReader);
    InitiatorId = static_cast<std::uint32_t>(Serialization::ReadVarInt(aReader));
    RecipientId = static_cast<std::uint32_t>(Serialization::ReadVarInt(aReader));
    Revision = Serialization::ReadVarInt(aReader);
}
