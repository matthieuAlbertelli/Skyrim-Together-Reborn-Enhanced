#pragma once

#include <Trade/TradeError.h>
#include <Trade/TradeTypes.h>

namespace Trade
{
class Session
{
public:
    Session(
        SessionId aId,
        PlayerId aInitiatorId,
        PlayerId aRecipientId,
        Tick aCreatedTick,
        Tick aExpiryTick) noexcept;

    [[nodiscard]] SessionId GetId() const noexcept { return m_id; }
    [[nodiscard]] State GetState() const noexcept { return m_state; }
    [[nodiscard]] Revision GetRevision() const noexcept { return m_revision; }
    [[nodiscard]] Tick GetCreatedTick() const noexcept { return m_createdTick; }
    [[nodiscard]] Tick GetLastActivityTick() const noexcept { return m_lastActivityTick; }
    [[nodiscard]] Tick GetExpiryTick() const noexcept { return m_expiryTick; }
    [[nodiscard]] Error GetTerminalError() const noexcept { return m_terminalError; }

    [[nodiscard]] const Participant& GetInitiator() const noexcept { return m_initiator; }
    [[nodiscard]] const Participant& GetRecipient() const noexcept { return m_recipient; }

    [[nodiscard]] bool IsParticipant(PlayerId aPlayerId) const noexcept;
    [[nodiscard]] bool IsTerminal() const noexcept;

    Result Accept(PlayerId aPlayerId, Tick aCurrentTick) noexcept;
    Result Reject(PlayerId aPlayerId, Tick aCurrentTick) noexcept;
    Result UpdateOffer(
        PlayerId aPlayerId,
        Revision aExpectedRevision,
        Offer aOffer,
        Tick aCurrentTick) noexcept;
    Result Confirm(
        PlayerId aPlayerId,
        Revision aRevision,
        Tick aCurrentTick) noexcept;
    Result Cancel(PlayerId aPlayerId, Tick aCurrentTick) noexcept;
    Result StartApplying(Tick aCurrentTick) noexcept;
    Result Complete(Tick aCurrentTick) noexcept;
    Result Fail(Error aReason, Tick aCurrentTick) noexcept;
    Result Expire(Tick aCurrentTick) noexcept;

private:
    [[nodiscard]] Participant* FindParticipant(PlayerId aPlayerId) noexcept;
    [[nodiscard]] const Participant* FindParticipant(PlayerId aPlayerId) const noexcept;
    [[nodiscard]] static Error ValidateAndCanonicalizeOffer(Offer& aOffer) noexcept;

    void ResetConfirmations() noexcept;
    void Touch(Tick aCurrentTick) noexcept;
    void EnterTerminalState(State aState, Error aReason, Tick aCurrentTick) noexcept;

    SessionId m_id{};
    Participant m_initiator{};
    Participant m_recipient{};
    State m_state{State::PendingAcceptance};
    Revision m_revision{1};
    Tick m_createdTick{};
    Tick m_lastActivityTick{};
    Tick m_expiryTick{};
    Error m_terminalError{Error::None};
};
}
