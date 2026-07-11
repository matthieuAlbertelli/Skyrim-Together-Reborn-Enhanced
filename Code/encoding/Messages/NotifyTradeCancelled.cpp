#include <Messages/NotifyTradeCancelled.h>
#include <TiltedCore/Serialization.hpp>

void NotifyTradeCancelled::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, SessionId);
    Serialization::WriteVarInt(aWriter, ActorId);
    Serialization::WriteVarInt(aWriter, static_cast<std::uint8_t>(Reason));
}

void NotifyTradeCancelled::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    SessionId = Serialization::ReadVarInt(aReader);
    ActorId = static_cast<std::uint32_t>(Serialization::ReadVarInt(aReader));
    Reason = static_cast<TradeCancelReason>(Serialization::ReadVarInt(aReader));
}
