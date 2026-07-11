#include <Messages/TradeApplyResultRequest.h>

#include <TiltedCore/Serialization.hpp>

void TradeApplyResultRequest::SerializeRaw(
    TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, SessionId);
    Serialization::WriteVarInt(aWriter, Revision);
    Serialization::WriteVarInt(aWriter, ApplyId);
    Serialization::WriteVarInt(aWriter, static_cast<std::uint8_t>(Result));
}

void TradeApplyResultRequest::DeserializeRaw(
    TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);

    SessionId = Serialization::ReadVarInt(aReader);
    Revision = Serialization::ReadVarInt(aReader);
    ApplyId = Serialization::ReadVarInt(aReader);

    const std::uint64_t result = Serialization::ReadVarInt(aReader);
    Valid = result <= static_cast<std::uint8_t>(
        TradeApplyResultCode::LocalStateMismatch);

    Result = Valid
        ? static_cast<TradeApplyResultCode>(result)
        : TradeApplyResultCode::MalformedPlan;
}
