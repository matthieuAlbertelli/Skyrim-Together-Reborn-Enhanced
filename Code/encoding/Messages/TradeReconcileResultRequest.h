#pragma once

#include "Message.h"

#include <Structs/TradeReconciliation.h>

#include <cstdint>

struct TradeReconcileResultRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode =
        kTradeReconcileResultRequest;

    TradeReconcileResultRequest()
        : ClientMessage(Opcode)
    {
    }

    void SerializeRaw(
        TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(
        TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const TradeReconcileResultRequest& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               Revision == acRhs.Revision &&
               ApplyId == acRhs.ApplyId &&
               ReconcileId == acRhs.ReconcileId &&
               Result == acRhs.Result &&
               Valid == acRhs.Valid;
    }

    std::uint64_t SessionId{};
    std::uint64_t Revision{};
    std::uint64_t ApplyId{};
    std::uint64_t ReconcileId{};
    TradeReconcileResultCode Result{
        TradeReconcileResultCode::Success};
    bool Valid{true};
};
