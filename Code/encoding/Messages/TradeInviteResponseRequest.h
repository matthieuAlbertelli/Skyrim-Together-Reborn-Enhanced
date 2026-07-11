#pragma once

#include "Message.h"

#include <cstdint>

struct TradeInviteResponseRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kTradeInviteResponseRequest;

    TradeInviteResponseRequest()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const TradeInviteResponseRequest& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               Accepted == acRhs.Accepted;
    }

    std::uint64_t SessionId{};
    bool Accepted{};
};
