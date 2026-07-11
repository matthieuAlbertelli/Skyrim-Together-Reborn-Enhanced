#pragma once

#include "Message.h"

#include <Structs/TradeReconciliation.h>

#include <cstdint>

struct NotifyTradeReconcile final : ServerMessage
{
    static constexpr ServerOpcode Opcode =
        kNotifyTradeReconcile;

    NotifyTradeReconcile()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(
        TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(
        TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const NotifyTradeReconcile& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               Revision == acRhs.Revision &&
               ApplyId == acRhs.ApplyId &&
               ReconcileId == acRhs.ReconcileId &&
               Plan == acRhs.Plan;
    }

    std::uint64_t SessionId{};
    std::uint64_t Revision{};
    std::uint64_t ApplyId{};
    std::uint64_t ReconcileId{};
    TradeReconciliationPlanData Plan{};
};
