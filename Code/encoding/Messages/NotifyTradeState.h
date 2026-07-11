#pragma once

#include "Message.h"

#include <Structs/TradeOffer.h>

#include <cstdint>

struct NotifyTradeState final : ServerMessage
{
    static constexpr ServerOpcode Opcode = kNotifyTradeState;

    NotifyTradeState()
        : ServerMessage(Opcode)
    {
    }

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const NotifyTradeState& acRhs) const noexcept
    {
        return GetOpcode() == acRhs.GetOpcode() &&
               SessionId == acRhs.SessionId &&
               Revision == acRhs.Revision &&
               State == acRhs.State &&
               TerminalError == acRhs.TerminalError &&
               InitiatorId == acRhs.InitiatorId &&
               RecipientId == acRhs.RecipientId &&
               InitiatorOffer == acRhs.InitiatorOffer &&
               RecipientOffer == acRhs.RecipientOffer &&
               InitiatorConfirmed == acRhs.InitiatorConfirmed &&
               RecipientConfirmed == acRhs.RecipientConfirmed &&
               InitiatorConfirmedRevision == acRhs.InitiatorConfirmedRevision &&
               RecipientConfirmedRevision == acRhs.RecipientConfirmedRevision;
    }

    std::uint64_t SessionId{};
    std::uint64_t Revision{};
    std::uint8_t State{};
    std::uint8_t TerminalError{};

    std::uint32_t InitiatorId{};
    std::uint32_t RecipientId{};

    TradeOfferData InitiatorOffer{};
    TradeOfferData RecipientOffer{};

    bool InitiatorConfirmed{};
    bool RecipientConfirmed{};

    std::uint64_t InitiatorConfirmedRevision{};
    std::uint64_t RecipientConfirmedRevision{};
};
