#pragma once

#include "Message.h"

#include <Structs/TradeApplication.h>

#include <cstdint>

struct NotifyTradeApply final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyTradeApply;

    NotifyTradeApply()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const NotifyTradeApply& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               Revision == acRhs.Revision &&
               ApplyId == acRhs.ApplyId &&
               Plan == acRhs.Plan;
    }

    std::uint64_t SessionId{};
    std::uint64_t Revision{};
    std::uint64_t ApplyId{};
    TradeMutationPlanData Plan{};
};
