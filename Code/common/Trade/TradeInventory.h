#pragma once

#include <Trade/TradeError.h>
#include <Trade/TradeTypes.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace Trade
{
class Session;

struct InventoryItemState
{
    ItemId Item{};
    std::uint64_t Quantity{};
    bool Transferable{};
    bool Ambiguous{};

    bool operator==(const InventoryItemState&) const noexcept = default;
};

struct InventorySnapshot
{
    PlayerId Owner{};
    std::vector<InventoryItemState> Items{};

    bool operator==(const InventorySnapshot&) const noexcept = default;
};

struct InventoryMutation
{
    ItemId Item{};
    std::int32_t Delta{};

    bool operator==(const InventoryMutation&) const noexcept = default;
};

struct PlayerMutationPlan
{
    PlayerId Player{};
    std::vector<InventoryMutation> Mutations{};

    bool operator==(const PlayerMutationPlan&) const noexcept = default;
};

struct MutationPlan
{
    SessionId Session{};
    Revision RevisionNumber{};
    PlayerMutationPlan Initiator{};
    PlayerMutationPlan Recipient{};

    bool operator==(const MutationPlan&) const noexcept = default;
};

struct MutationPlanResult
{
    Error ErrorCode{Error::None};
    MutationPlan Plan{};

    [[nodiscard]] bool Succeeded() const noexcept
    {
        return ErrorCode == Error::None;
    }

    [[nodiscard]] static MutationPlanResult Failure(Error aError) noexcept
    {
        MutationPlanResult result;
        result.ErrorCode = aError;
        return result;
    }

    [[nodiscard]] static MutationPlanResult Success(MutationPlan aPlan) noexcept
    {
        MutationPlanResult result;
        result.Plan = std::move(aPlan);
        return result;
    }
};

[[nodiscard]] MutationPlanResult BuildMutationPlan(
    const Session& acSession,
    const InventorySnapshot& acInitiatorInventory,
    const InventorySnapshot& acRecipientInventory) noexcept;
}
