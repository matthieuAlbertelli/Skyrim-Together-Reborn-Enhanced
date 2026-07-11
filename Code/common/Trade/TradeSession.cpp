#include <Trade/TradeSession.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

namespace Trade
{
Session::Session(
    SessionId aId,
    PlayerId aInitiatorId,
    PlayerId aRecipientId,
    Tick aCreatedTick,
    Tick aExpiryTick) noexcept
    : m_id(aId)
    , m_initiator{aInitiatorId}
    , m_recipient{aRecipientId}
    , m_createdTick(aCreatedTick)
    , m_lastActivityTick(aCreatedTick)
    , m_expiryTick(aExpiryTick)
{
    if (aInitiatorId == aRecipientId)
        EnterTerminalState(State::Failed, Error::InvalidParticipant, aCreatedTick);
}

bool Session::IsParticipant(PlayerId aPlayerId) const noexcept
{
    return FindParticipant(aPlayerId) != nullptr;
}

bool Session::IsTerminal() const noexcept
{
    return m_state == State::Completed || m_state == State::Cancelled || m_state == State::Failed;
}

Result Session::Accept(PlayerId aPlayerId, Tick aCurrentTick) noexcept
{
    if (aPlayerId != m_recipient.Id)
        return Result::Failure(Error::NotRecipient);

    if (m_state == State::PendingAcceptance)
    {
        m_state = State::Negotiating;
        Touch(aCurrentTick);
        return Result::Success();
    }

    if (m_state == State::Negotiating || m_state == State::Locked || m_state == State::Applying ||
        m_state == State::Completed)
    {
        return Result::Success(false);
    }

    return Result::Failure(IsTerminal() ? Error::AlreadyTerminal : Error::InvalidState);
}

Result Session::Reject(PlayerId aPlayerId, Tick aCurrentTick) noexcept
{
    if (aPlayerId != m_recipient.Id)
        return Result::Failure(Error::NotRecipient);

    if (m_state == State::Cancelled)
        return Result::Success(false);

    if (m_state != State::PendingAcceptance)
        return Result::Failure(IsTerminal() ? Error::AlreadyTerminal : Error::InvalidState);

    EnterTerminalState(State::Cancelled, Error::None, aCurrentTick);
    return Result::Success();
}

Result Session::UpdateOffer(
    PlayerId aPlayerId,
    Revision aExpectedRevision,
    Offer aOffer,
    Tick aCurrentTick) noexcept
{
    if (m_state != State::Negotiating)
        return Result::Failure(IsTerminal() ? Error::AlreadyTerminal : Error::InvalidState);

    Participant* const pParticipant = FindParticipant(aPlayerId);
    if (!pParticipant)
        return Result::Failure(Error::NotParticipant);

    const Error validationError = ValidateAndCanonicalizeOffer(aOffer);
    if (validationError != Error::None)
        return Result::Failure(validationError);

    // A retransmitted full snapshot is safe even when it carries the revision
    // that preceded its first successful application.
    if (pParticipant->CurrentOffer == aOffer)
        return Result::Success(false);

    if (aExpectedRevision != m_revision)
        return Result::Failure(Error::StaleRevision);

    if (m_revision == std::numeric_limits<Revision>::max())
        return Result::Failure(Error::RevisionOverflow);

    pParticipant->CurrentOffer = std::move(aOffer);
    ++m_revision;
    ResetConfirmations();
    Touch(aCurrentTick);
    return Result::Success();
}

Result Session::Confirm(PlayerId aPlayerId, Revision aRevision, Tick aCurrentTick) noexcept
{
    Participant* const pParticipant = FindParticipant(aPlayerId);
    if (!pParticipant)
        return Result::Failure(Error::NotParticipant);

    if (aRevision != m_revision)
        return Result::Failure(Error::StaleRevision);

    if (m_state != State::Negotiating)
    {
        const bool isCompletedConfirmation =
            (m_state == State::Locked || m_state == State::Applying || m_state == State::Completed) &&
            pParticipant->Confirmed && pParticipant->ConfirmedRevision == m_revision;

        if (isCompletedConfirmation)
            return Result::Success(false);

        return Result::Failure(IsTerminal() ? Error::AlreadyTerminal : Error::InvalidState);
    }

    if (pParticipant->Confirmed && pParticipant->ConfirmedRevision == aRevision)
        return Result::Success(false);

    pParticipant->Confirmed = true;
    pParticipant->ConfirmedRevision = aRevision;
    Touch(aCurrentTick);

    if (m_initiator.Confirmed && m_recipient.Confirmed &&
        m_initiator.ConfirmedRevision == m_revision &&
        m_recipient.ConfirmedRevision == m_revision)
    {
        m_state = State::Locked;
    }

    return Result::Success();
}

Result Session::Cancel(PlayerId aPlayerId, Tick aCurrentTick) noexcept
{
    if (!IsParticipant(aPlayerId))
        return Result::Failure(Error::NotParticipant);

    if (m_state == State::Cancelled)
        return Result::Success(false);

    if (m_state == State::Completed || m_state == State::Failed)
        return Result::Failure(Error::AlreadyTerminal);

    if (m_state == State::Applying)
        return Result::Failure(Error::InvalidState);

    EnterTerminalState(State::Cancelled, Error::None, aCurrentTick);
    return Result::Success();
}

Result Session::StartApplying(Tick aCurrentTick) noexcept
{
    if (m_state == State::Applying)
        return Result::Success(false);

    if (m_state != State::Locked)
        return Result::Failure(IsTerminal() ? Error::AlreadyTerminal : Error::InvalidState);

    m_state = State::Applying;
    Touch(aCurrentTick);
    return Result::Success();
}

Result Session::Complete(Tick aCurrentTick) noexcept
{
    if (m_state == State::Completed)
        return Result::Success(false);

    if (m_state != State::Applying)
        return Result::Failure(IsTerminal() ? Error::AlreadyTerminal : Error::InvalidState);

    EnterTerminalState(State::Completed, Error::None, aCurrentTick);
    return Result::Success();
}

Result Session::Fail(Error aReason, Tick aCurrentTick) noexcept
{
    if (m_state == State::Failed && m_terminalError == aReason)
        return Result::Success(false);

    if (IsTerminal())
        return Result::Failure(Error::AlreadyTerminal);

    if (aReason == Error::None)
        return Result::Failure(Error::InvalidState);

    EnterTerminalState(State::Failed, aReason, aCurrentTick);
    return Result::Success();
}

Result Session::Expire(Tick aCurrentTick) noexcept
{
    if (m_state == State::Cancelled && m_terminalError == Error::SessionExpired)
        return Result::Failure(Error::SessionExpired, false);

    if (IsTerminal())
        return Result::Failure(Error::AlreadyTerminal);

    if (aCurrentTick < m_expiryTick)
        return Result::Success(false);

    EnterTerminalState(State::Cancelled, Error::SessionExpired, aCurrentTick);
    return Result::Failure(Error::SessionExpired, true);
}

Participant* Session::FindParticipant(PlayerId aPlayerId) noexcept
{
    if (m_initiator.Id == aPlayerId)
        return &m_initiator;

    if (m_recipient.Id == aPlayerId)
        return &m_recipient;

    return nullptr;
}

const Participant* Session::FindParticipant(PlayerId aPlayerId) const noexcept
{
    if (m_initiator.Id == aPlayerId)
        return &m_initiator;

    if (m_recipient.Id == aPlayerId)
        return &m_recipient;

    return nullptr;
}

Error Session::ValidateAndCanonicalizeOffer(Offer& aOffer) noexcept
{
    for (const OfferLine& line : aOffer.Items)
    {
        if (line.Item == 0)
            return Error::InvalidItem;

        if (line.Quantity == 0)
            return Error::EmptyQuantity;
    }

    std::sort(
        aOffer.Items.begin(),
        aOffer.Items.end(),
        [](const OfferLine& acLeft, const OfferLine& acRight) noexcept
        {
            return acLeft.Item < acRight.Item;
        });

    for (std::size_t i = 1; i < aOffer.Items.size(); ++i)
    {
        if (aOffer.Items[i - 1].Item == aOffer.Items[i].Item)
            return Error::DuplicateItem;
    }

    return Error::None;
}

void Session::ResetConfirmations() noexcept
{
    m_initiator.Confirmed = false;
    m_initiator.ConfirmedRevision = 0;
    m_recipient.Confirmed = false;
    m_recipient.ConfirmedRevision = 0;
}

void Session::Touch(Tick aCurrentTick) noexcept
{
    if (aCurrentTick > m_lastActivityTick)
        m_lastActivityTick = aCurrentTick;
}

void Session::EnterTerminalState(State aState, Error aReason, Tick aCurrentTick) noexcept
{
    m_state = aState;
    m_terminalError = aReason;
    Touch(aCurrentTick);
}
}
