#pragma once

#include "Message.h"

#include <cstdint>

enum class TradeCancelReason : std::uint8_t
{
    Rejected,
    Expired,
    PlayerDisconnected,
    ParticipantUnavailable,
    CancelledByParticipant
};

struct NotifyTradeCancelled final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyTradeCancelled;

    NotifyTradeCancelled()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const NotifyTradeCancelled& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               ActorId == acRhs.ActorId &&
               Reason == acRhs.Reason;
    }

    std::uint64_t SessionId{};
    std::uint32_t ActorId{};
    TradeCancelReason Reason{TradeCancelReason::Rejected};
};
