#pragma once

#include <Trade/TradeInventory.h>

#include <cstdint>
#include <utility>

namespace Trade
{
enum class ApplyOutcome : std::uint8_t
{
    Success,
    MalformedPlan,
    SessionMismatch,
    LocalPlayerUnavailable,
    ItemUnavailable,
    InsufficientQuantity,
    ItemNotTransferable,
    AmbiguousItem,
    LocalStateMismatch
};

enum class ApplyStatus : std::uint8_t
{
    Pending,
    Succeeded,
    Failed
};

struct ApplyParticipant
{
    PlayerId Id{};
    ApplyStatus Status{ApplyStatus::Pending};
    ApplyOutcome Outcome{ApplyOutcome::Success};

    bool operator==(const ApplyParticipant&) const noexcept = default;
};

class Application
{
public:
    Application(
        ApplyId aId,
        MutationPlan aPlan,
        Tick aStartedTick,
        Tick aDeadlineTick) noexcept;

    [[nodiscard]] ApplyId GetId() const noexcept { return m_id; }
    [[nodiscard]] SessionId GetSessionId() const noexcept { return m_plan.Session; }
    [[nodiscard]] Revision GetRevision() const noexcept { return m_plan.RevisionNumber; }
    [[nodiscard]] Tick GetStartedTick() const noexcept { return m_startedTick; }
    [[nodiscard]] Tick GetLastActivityTick() const noexcept { return m_lastActivityTick; }
    [[nodiscard]] Tick GetDeadlineTick() const noexcept { return m_deadlineTick; }

    [[nodiscard]] const MutationPlan& GetPlan() const noexcept { return m_plan; }
    [[nodiscard]] const ApplyParticipant& GetInitiator() const noexcept { return m_initiator; }
    [[nodiscard]] const ApplyParticipant& GetRecipient() const noexcept { return m_recipient; }

    [[nodiscard]] bool IsParticipant(PlayerId aPlayerId) const noexcept;
    [[nodiscard]] bool IsSettled() const noexcept;
    [[nodiscard]] bool AllSucceeded() const noexcept;
    [[nodiscard]] bool HasFailed() const noexcept;
    [[nodiscard]] bool IsTimedOut(Tick aCurrentTick) const noexcept;

    Result RecordResult(
        PlayerId aPlayerId,
        ApplyOutcome aOutcome,
        Tick aCurrentTick) noexcept;

private:
    [[nodiscard]] ApplyParticipant* FindParticipant(PlayerId aPlayerId) noexcept;
    void Touch(Tick aCurrentTick) noexcept;

    ApplyId m_id{};
    MutationPlan m_plan{};
    ApplyParticipant m_initiator{};
    ApplyParticipant m_recipient{};
    Tick m_startedTick{};
    Tick m_lastActivityTick{};
    Tick m_deadlineTick{};
};
}
