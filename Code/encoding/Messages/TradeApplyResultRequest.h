#pragma once

#include "Message.h"

#include <Structs/TradeApplication.h>

#include <cstdint>

struct TradeApplyResultRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kTradeApplyResultRequest;

    TradeApplyResultRequest()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const TradeApplyResultRequest& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               Revision == acRhs.Revision &&
               ApplyId == acRhs.ApplyId &&
               Result == acRhs.Result &&
               Valid == acRhs.Valid;
    }

    std::uint64_t SessionId{};
    std::uint64_t Revision{};
    std::uint64_t ApplyId{};
    TradeApplyResultCode Result{TradeApplyResultCode::Success};
    bool Valid{true};
};
