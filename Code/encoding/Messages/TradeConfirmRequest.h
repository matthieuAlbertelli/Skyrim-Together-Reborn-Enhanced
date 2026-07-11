#pragma once

#include "Message.h"

#include <cstdint>

struct TradeConfirmRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kTradeConfirmRequest;

    TradeConfirmRequest()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const TradeConfirmRequest& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               Revision == acRhs.Revision;
    }

    std::uint64_t SessionId{};
    std::uint64_t Revision{};
};
