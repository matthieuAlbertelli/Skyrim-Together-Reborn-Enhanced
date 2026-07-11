#pragma once

#include "Message.h"

#include <cstdint>

struct NotifyTradeStarted final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyTradeStarted;

    NotifyTradeStarted()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const NotifyTradeStarted& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               InitiatorId == acRhs.InitiatorId &&
               RecipientId == acRhs.RecipientId &&
               Revision == acRhs.Revision;
    }

    std::uint64_t SessionId{};
    std::uint32_t InitiatorId{};
    std::uint32_t RecipientId{};
    std::uint64_t Revision{};
};
