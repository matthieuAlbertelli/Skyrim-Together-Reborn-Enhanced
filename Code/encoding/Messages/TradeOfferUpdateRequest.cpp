#include <Messages/TradeOfferUpdateRequest.h>

#include <TiltedCore/Serialization.hpp>

void TradeOfferUpdateRequest::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteVarInt(aWriter, SessionId);
    Serialization::WriteVarInt(aWriter, ExpectedRevision);
    Offer.Serialize(aWriter);
}

void TradeOfferUpdateRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    ClientMessage::DeserializeRaw(aReader);

    SessionId = Serialization::ReadVarInt(aReader);
    ExpectedRevision = Serialization::ReadVarInt(aReader);
    Offer.Deserialize(aReader);
}
