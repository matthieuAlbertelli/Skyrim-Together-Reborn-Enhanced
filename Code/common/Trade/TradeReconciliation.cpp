#include <Trade/TradeReconciliation.h>

#include <algorithm>
#include <limits>

namespace Trade
{
namespace
{
struct InventoryLookup
{
    const InventoryItemState* pItem{};
    bool Duplicate{};
};

InventoryLookup FindInventoryItem(
    const InventorySnapshot& acInventory,
    ItemId aItem) noexcept
{
    InventoryLookup result;

    for (const InventoryItemState& item : acInventory.Items)
    {
        if (item.Item != aItem)
            continue;

        if (result.pItem)
        {
            result.Duplicate = true;
            return result;
        }

        result.pItem = &item;
    }

    return result;
}

Error AddTarget(
    PlayerReconciliationPlan& aPlan,
    const InventorySnapshot& acInventory,
    ItemId aItem) noexcept
{
    const auto existing = std::find_if(
        aPlan.Targets.begin(),
        aPlan.Targets.end(),
        [aItem](const InventoryTarget& acTarget) noexcept
        {
            return acTarget.Item == aItem;
        });

    if (existing != aPlan.Targets.end())
        return Error::None;

    const InventoryLookup lookup =
        FindInventoryItem(acInventory, aItem);

    if (lookup.Duplicate ||
        (lookup.pItem && lookup.pItem->Ambiguous))
    {
        return Error::AmbiguousItem;
    }

    if (lookup.pItem && !lookup.pItem->Transferable)
        return Error::ItemNotTransferable;

    const std::uint64_t quantity =
        lookup.pItem ? lookup.pItem->Quantity : 0;

    if (quantity >
        static_cast<std::uint64_t>(
            std::numeric_limits<std::uint32_t>::max()))
    {
        return Error::QuantityOverflow;
    }

    aPlan.Targets.push_back(
        InventoryTarget{
            aItem,
            static_cast<std::uint32_t>(quantity)});

    return Error::None;
}

Error BuildPlayerPlan(
    const PlayerMutationPlan& acMutationPlan,
    const InventorySnapshot& acInventory,
    PlayerReconciliationPlan& aPlan) noexcept
{
    if (acMutationPlan.Player != acInventory.Owner)
        return Error::InvalidParticipant;

    aPlan.Player = acMutationPlan.Player;
    aPlan.Targets.reserve(
        acMutationPlan.Mutations.size());

    for (const InventoryMutation& mutation :
         acMutationPlan.Mutations)
    {
        const Error error =
            AddTarget(aPlan, acInventory, mutation.Item);

        if (error != Error::None)
            return error;
    }

    std::sort(
        aPlan.Targets.begin(),
        aPlan.Targets.end(),
        [](const InventoryTarget& acLeft,
           const InventoryTarget& acRight) noexcept
        {
            return acLeft.Item < acRight.Item;
        });

    return Error::None;
}
}

ReconciliationPlanResult BuildReconciliationPlan(
    const Application& acApplication,
    const InventorySnapshot& acInitiatorInventory,
    const InventorySnapshot& acRecipientInventory) noexcept
{
    const MutationPlan& mutationPlan =
        acApplication.GetPlan();

    if (acInitiatorInventory.Owner !=
            mutationPlan.Initiator.Player ||
        acRecipientInventory.Owner !=
            mutationPlan.Recipient.Player)
    {
        return ReconciliationPlanResult::Failure(
            Error::InvalidParticipant);
    }

    ReconciliationPlan plan;
    plan.Session = mutationPlan.Session;
    plan.RevisionNumber =
        mutationPlan.RevisionNumber;
    plan.Application = acApplication.GetId();

    Error error = BuildPlayerPlan(
        mutationPlan.Initiator,
        acInitiatorInventory,
        plan.Initiator);

    if (error != Error::None)
        return ReconciliationPlanResult::Failure(error);

    error = BuildPlayerPlan(
        mutationPlan.Recipient,
        acRecipientInventory,
        plan.Recipient);

    if (error != Error::None)
        return ReconciliationPlanResult::Failure(error);

    return ReconciliationPlanResult::Success(
        std::move(plan));
}

Reconciliation::Reconciliation(
    ReconcileId aId,
    ReconciliationPlan aPlan,
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

bool Reconciliation::IsParticipant(
    PlayerId aPlayerId) const noexcept
{
    return m_initiator.Id == aPlayerId ||
           m_recipient.Id == aPlayerId;
}

bool Reconciliation::IsSettled() const noexcept
{
    return m_initiator.Status != ReconcileStatus::Pending &&
           m_recipient.Status != ReconcileStatus::Pending;
}

bool Reconciliation::AllSucceeded() const noexcept
{
    return m_initiator.Status == ReconcileStatus::Succeeded &&
           m_recipient.Status == ReconcileStatus::Succeeded;
}

bool Reconciliation::HasFailed() const noexcept
{
    return m_initiator.Status == ReconcileStatus::Failed ||
           m_recipient.Status == ReconcileStatus::Failed;
}

bool Reconciliation::IsTimedOut(
    Tick aCurrentTick) const noexcept
{
    return !IsSettled() &&
           aCurrentTick >= m_deadlineTick;
}

Result Reconciliation::RecordResult(
    PlayerId aPlayerId,
    ReconcileOutcome aOutcome,
    Tick aCurrentTick) noexcept
{
    ReconcileParticipant* const pParticipant =
        FindParticipant(aPlayerId);

    if (!pParticipant)
        return Result::Failure(Error::NotParticipant);

    const ReconcileStatus newStatus =
        aOutcome == ReconcileOutcome::Success
            ? ReconcileStatus::Succeeded
            : ReconcileStatus::Failed;

    if (pParticipant->Status ==
        ReconcileStatus::Pending)
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

    return Result::Failure(
        Error::ReconcileResultConflict);
}

Result Reconciliation::MarkUnavailable(
    PlayerId aPlayerId,
    Tick aCurrentTick) noexcept
{
    return RecordResult(
        aPlayerId,
        ReconcileOutcome::ParticipantUnavailable,
        aCurrentTick);
}

ReconcileParticipant* Reconciliation::FindParticipant(
    PlayerId aPlayerId) noexcept
{
    if (m_initiator.Id == aPlayerId)
        return &m_initiator;

    if (m_recipient.Id == aPlayerId)
        return &m_recipient;

    return nullptr;
}

void Reconciliation::Touch(
    Tick aCurrentTick) noexcept
{
    if (aCurrentTick > m_lastActivityTick)
        m_lastActivityTick = aCurrentTick;
}
}
