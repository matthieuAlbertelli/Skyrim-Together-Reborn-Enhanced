#pragma once

#include "Message.h"

#include <Structs/TradeOffer.h>

#include <cstdint>

struct TradeOfferUpdateRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kTradeOfferUpdateRequest;

    TradeOfferUpdateRequest()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const TradeOfferUpdateRequest& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               ExpectedRevision == acRhs.ExpectedRevision &&
               Offer == acRhs.Offer;
    }

    std::uint64_t SessionId{};
    std::uint64_t ExpectedRevision{};
    TradeOfferData Offer{};
};
