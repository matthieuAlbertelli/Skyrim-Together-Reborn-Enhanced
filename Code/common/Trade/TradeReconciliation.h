#pragma once

#include <Trade/TradeApplication.h>
#include <Trade/TradeInventory.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace Trade
{
struct InventoryTarget
{
    ItemId Item{};
    std::uint32_t Quantity{};

    bool operator==(const InventoryTarget&) const noexcept = default;
};

struct PlayerReconciliationPlan
{
    PlayerId Player{};
    std::vector<InventoryTarget> Targets{};

    bool operator==(const PlayerReconciliationPlan&) const noexcept = default;
};

struct ReconciliationPlan
{
    SessionId Session{};
    Revision RevisionNumber{};
    ApplyId Application{};
    PlayerReconciliationPlan Initiator{};
    PlayerReconciliationPlan Recipient{};

    bool operator==(const ReconciliationPlan&) const noexcept = default;
};

struct ReconciliationPlanResult
{
    Error ErrorCode{Error::None};
    ReconciliationPlan Plan{};

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return ErrorCode == Error::None;
    }

    [[nodiscard]] static ReconciliationPlanResult Failure(Error aError) noexcept
    {
        ReconciliationPlanResult result;
        result.ErrorCode = aError;
        return result;
    }

    [[nodiscard]] static ReconciliationPlanResult Success(
        ReconciliationPlan aPlan) noexcept
    {
        ReconciliationPlanResult result;
        result.Plan = std::move(aPlan);
        return result;
    }
};

[[nodiscard]] ReconciliationPlanResult BuildReconciliationPlan(
    const Application& acApplication,
    const InventorySnapshot& acInitiatorInventory,
    const InventorySnapshot& acRecipientInventory) noexcept;

enum class ReconcileOutcome : std::uint8_t
{
    Success,
    MalformedPlan,
    SessionMismatch,
    LocalPlayerUnavailable,
    ItemUnavailable,
    ItemNotTransferable,
    AmbiguousItem,
    LocalStateMismatch,
    ParticipantUnavailable
};

enum class ReconcileStatus : std::uint8_t
{
    Pending,
    Succeeded,
    Failed
};

struct ReconcileParticipant
{
    PlayerId Id{};
    ReconcileStatus Status{ReconcileStatus::Pending};
    ReconcileOutcome Outcome{ReconcileOutcome::Success};

    bool operator==(const ReconcileParticipant&) const noexcept = default;
};

class Reconciliation
{
public:
    Reconciliation(
        ReconcileId aId,
        ReconciliationPlan aPlan,
        Tick aStartedTick,
        Tick aDeadlineTick) noexcept;

    [[nodiscard]] ReconcileId GetId() const noexcept { return m_id; }
    [[nodiscard]] SessionId GetSessionId() const noexcept { return m_plan.Session; }
    [[nodiscard]] Revision GetRevision() const noexcept { return m_plan.RevisionNumber; }
    [[nodiscard]] ApplyId GetApplyId() const noexcept { return m_plan.Application; }
    [[nodiscard]] Tick GetStartedTick() const noexcept { return m_startedTick; }
    [[nodiscard]] Tick GetLastActivityTick() const noexcept { return m_lastActivityTick; }
    [[nodiscard]] Tick GetDeadlineTick() const noexcept { return m_deadlineTick; }

    [[nodiscard]] const ReconciliationPlan& GetPlan() const noexcept { return m_plan; }
    [[nodiscard]] const ReconcileParticipant& GetInitiator() const noexcept { return m_initiator; }
    [[nodiscard]] const ReconcileParticipant& GetRecipient() const noexcept { return m_recipient; }

    [[nodiscard]] bool IsParticipant(PlayerId aPlayerId) const noexcept;
    [[nodiscard]] bool IsSettled() const noexcept;
    [[nodiscard]] bool AllSucceeded() const noexcept;
    [[nodiscard]] bool HasFailed() const noexcept;
    [[nodiscard]] bool IsTimedOut(Tick aCurrentTick) const noexcept;

    Result RecordResult(
        PlayerId aPlayerId,
        ReconcileOutcome aOutcome,
        Tick aCurrentTick) noexcept;

    Result MarkUnavailable(
        PlayerId aPlayerId,
        Tick aCurrentTick) noexcept;

private:
    [[nodiscard]] ReconcileParticipant* FindParticipant(
        PlayerId aPlayerId) noexcept;
    void Touch(Tick aCurrentTick) noexcept;

    ReconcileId m_id{};
    ReconciliationPlan m_plan{};
    ReconcileParticipant m_initiator{};
    ReconcileParticipant m_recipient{};
    Tick m_startedTick{};
    Tick m_lastActivityTick{};
    Tick m_deadlineTick{};
};
}
