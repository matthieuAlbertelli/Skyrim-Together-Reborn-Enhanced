#pragma once

#include "Message.h"

#include <cstdint>

struct NotifyTradeInvite final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyTradeInvite;

    NotifyTradeInvite()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const NotifyTradeInvite& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               InviterId == acRhs.InviterId &&
               ExpiryTick == acRhs.ExpiryTick;
    }

    uint64_t SessionId{};
    uint32_t InviterId{};
    uint64_t ExpiryTick{};
};
