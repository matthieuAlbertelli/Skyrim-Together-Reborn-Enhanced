#include <Trade/TradeApplication.h>

namespace Trade
{
Application::Application(
    ApplyId aId,
    MutationPlan aPlan,
    Tick aStartedTick,
    Tick aDeadlineTick) noexcept
    : m_id(aId)
    , m_plan(std::move(aPlan))
    , m_initiator{m_plan.Initiator.Player}
    , m_recipient{m_plan.Recipient.Player}
    , m_startedTick(aStartedTick)
    , m_lastActivityTick(aStartedTick)
    , m_deadlineTick(aDeadlineTick)
{
}

bool Application::IsParticipant(PlayerId aPlayerId) const noexcept
{
    return m_initiator.Id == aPlayerId || m_recipient.Id == aPlayerId;
}

bool Application::IsSettled() const noexcept
{
    return m_initiator.Status != ApplyStatus::Pending &&
           m_recipient.Status != ApplyStatus::Pending;
}

bool Application::AllSucceeded() const noexcept
{
    return m_initiator.Status == ApplyStatus::Succeeded &&
           m_recipient.Status == ApplyStatus::Succeeded;
}

bool Application::HasFailed() const noexcept
{
    return m_initiator.Status == ApplyStatus::Failed ||
           m_recipient.Status == ApplyStatus::Failed;
}

bool Application::IsTimedOut(Tick aCurrentTick) const noexcept
{
    return !IsSettled() && aCurrentTick >= m_deadlineTick;
}

Result Application::RecordResult(
    PlayerId aPlayerId,
    ApplyOutcome aOutcome,
    Tick aCurrentTick) noexcept
{
    ApplyParticipant* const pParticipant = FindParticipant(aPlayerId);
    if (!pParticipant)
        return Result::Failure(Error::NotParticipant);

    const ApplyStatus newStatus =
        aOutcome == ApplyOutcome::Success
            ? ApplyStatus::Succeeded
            : ApplyStatus::Failed;

    if (pParticipant->Status == ApplyStatus::Pending)
    {
        pParticipant->Status = newStatus;
        pParticipant->Outcome = aOutcome;
        Touch(aCurrentTick);
        return Result::Success();
    }

    if (pParticipant->Status == newStatus &&
        pParticipant->Outcome == aOutcome)
    {
        return Result::Success(false);
    }

    return Result::Failure(Error::ApplyResultConflict);
}

ApplyParticipant* Application::FindParticipant(PlayerId aPlayerId) noexcept
{
    if (m_initiator.Id == aPlayerId)
        return &m_initiator;

    if (m_recipient.Id == aPlayerId)
        return &m_recipient;

    return nullptr;
}

void Application::Touch(Tick aCurrentTick) noexcept
{
    if (aCurrentTick > m_lastActivityTick)
        m_lastActivityTick = aCurrentTick;
}
}
