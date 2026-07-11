#include <Messages/NotifyTradeState.h>

#include <TiltedCore/Serialization.hpp>

void NotifyTradeState::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, SessionId);
    Serialization::WriteVarInt(aWriter, Revision);
    Serialization::WriteVarInt(aWriter, State);
    Serialization::WriteVarInt(aWriter, TerminalError);

    Serialization::WriteVarInt(aWriter, InitiatorId);
    Serialization::WriteVarInt(aWriter, RecipientId);

    InitiatorOffer.Serialize(aWriter);
    RecipientOffer.Serialize(aWriter);

    Serialization::WriteBool(aWriter, InitiatorConfirmed);
    Serialization::WriteBool(aWriter, RecipientConfirmed);

    Serialization::WriteVarInt(aWriter, InitiatorConfirmedRevision);
    Serialization::WriteVarInt(aWriter, RecipientConfirmedRevision);
}

void NotifyTradeState::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ServerMessage::DeserializeRaw(aReader);

    SessionId = Serialization::ReadVarInt(aReader);
    Revision = Serialization::ReadVarInt(aReader);
    State = static_cast<std::uint8_t>(Serialization::ReadVarInt(aReader));
    TerminalError = static_cast<std::uint8_t>(Serialization::ReadVarInt(aReader));

    InitiatorId = static_cast<std::uint32_t>(Serialization::ReadVarInt(aReader));
    RecipientId = static_cast<std::uint32_t>(Serialization::ReadVarInt(aReader));

    InitiatorOffer.Deserialize(aReader);
    RecipientOffer.Deserialize(aReader);

    InitiatorConfirmed = Serialization::ReadBool(aReader);
    RecipientConfirmed = Serialization::ReadBool(aReader);

    InitiatorConfirmedRevision = Serialization::ReadVarInt(aReader);
    RecipientConfirmedRevision = Serialization::ReadVarInt(aReader);
}
