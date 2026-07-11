#pragma once

#include "Message.h"

#include <cstdint>

struct TradeCancelRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kTradeCancelRequest;

    TradeCancelRequest()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const TradeCancelRequest& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId;
    }

    std::uint64_t SessionId{};
};
