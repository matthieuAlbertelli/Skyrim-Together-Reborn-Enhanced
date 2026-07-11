#include <Trade/TradeInventory.h>

#include <Trade/TradeSession.h>

#include <algorithm>
#include <limits>
#include <utility>

namespace Trade
{
namespace
{
struct ItemLookupResult
{
    const InventoryItemState* pItem{};
    bool Duplicate{};
};

ItemLookupResult FindInventoryItem(
    const InventorySnapshot& acInventory,
    ItemId aItem) noexcept
{
    ItemLookupResult result;

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

Error ValidateOfferedItem(
    const InventorySnapshot& acInventory,
    const OfferLine& acOfferLine) noexcept
{
    if (acOfferLine.Quantity > static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
        return Error::QuantityOverflow;

    const ItemLookupResult lookup = FindInventoryItem(acInventory, acOfferLine.Item);

    if (!lookup.pItem)
        return Error::ItemUnavailable;

    if (lookup.Duplicate || lookup.pItem->Ambiguous)
        return Error::AmbiguousItem;

    if (!lookup.pItem->Transferable)
        return Error::ItemNotTransferable;

    if (lookup.pItem->Quantity < acOfferLine.Quantity)
        return Error::InsufficientQuantity;

    return Error::None;
}

Error AddMutation(
    std::vector<InventoryMutation>& aMutations,
    ItemId aItem,
    std::int64_t aDelta) noexcept
{
    auto mutationIt = std::find_if(
        aMutations.begin(),
        aMutations.end(),
        [aItem](const InventoryMutation& acMutation) noexcept
        {
            return acMutation.Item == aItem;
        });

    const std::int64_t currentDelta =
        mutationIt != aMutations.end() ? mutationIt->Delta : 0;

    const std::int64_t combinedDelta = currentDelta + aDelta;
    if (combinedDelta < std::numeric_limits<std::int32_t>::min() ||
        combinedDelta > std::numeric_limits<std::int32_t>::max())
    {
        return Error::QuantityOverflow;
    }

    if (mutationIt == aMutations.end())
    {
        if (combinedDelta != 0)
        {
            aMutations.push_back(
                InventoryMutation{aItem, static_cast<std::int32_t>(combinedDelta)});
        }

        return Error::None;
    }

    if (combinedDelta == 0)
    {
        aMutations.erase(mutationIt);
        return Error::None;
    }

    mutationIt->Delta = static_cast<std::int32_t>(combinedDelta);
    return Error::None;
}

Error ValidateIncomingItem(
    const InventorySnapshot& acInventory,
    ItemId aItem) noexcept
{
    const ItemLookupResult lookup = FindInventoryItem(acInventory, aItem);

    if (!lookup.pItem)
        return Error::None;

    if (lookup.Duplicate || lookup.pItem->Ambiguous)
        return Error::AmbiguousItem;

    if (!lookup.pItem->Transferable)
        return Error::ItemNotTransferable;

    return Error::None;
}

Error AddOfferToPlan(
    const Offer& acOffer,
    const InventorySnapshot& acOwnerInventory,
    const InventorySnapshot& acRecipientInventory,
    PlayerMutationPlan& aOwnerPlan,
    PlayerMutationPlan& aRecipientPlan) noexcept
{
    for (const OfferLine& line : acOffer.Items)
    {
        const Error validationError = ValidateOfferedItem(acOwnerInventory, line);
        if (validationError != Error::None)
            return validationError;

        const Error recipientValidationError =
            ValidateIncomingItem(acRecipientInventory, line.Item);
        if (recipientValidationError != Error::None)
            return recipientValidationError;

        const std::int64_t quantity = line.Quantity;

        const Error ownerMutationError =
            AddMutation(aOwnerPlan.Mutations, line.Item, -quantity);
        if (ownerMutationError != Error::None)
            return ownerMutationError;

        const Error recipientMutationError =
            AddMutation(aRecipientPlan.Mutations, line.Item, quantity);
        if (recipientMutationError != Error::None)
            return recipientMutationError;
    }

    return Error::None;
}

void CanonicalizePlan(PlayerMutationPlan& aPlan) noexcept
{
    std::sort(
        aPlan.Mutations.begin(),
        aPlan.Mutations.end(),
        [](const InventoryMutation& acLeft, const InventoryMutation& acRight) noexcept
        {
            return acLeft.Item < acRight.Item;
        });
}
}

MutationPlanResult BuildMutationPlan(
    const Session& acSession,
    const InventorySnapshot& acInitiatorInventory,
    const InventorySnapshot& acRecipientInventory) noexcept
{
    if (acSession.GetState() != State::Locked)
        return MutationPlanResult::Failure(Error::InvalidState);

    if (acInitiatorInventory.Owner != acSession.GetInitiator().Id ||
        acRecipientInventory.Owner != acSession.GetRecipient().Id)
    {
        return MutationPlanResult::Failure(Error::InvalidParticipant);
    }

    MutationPlan plan;
    plan.Session = acSession.GetId();
    plan.RevisionNumber = acSession.GetRevision();
    plan.Initiator.Player = acSession.GetInitiator().Id;
    plan.Recipient.Player = acSession.GetRecipient().Id;

    Error error = AddOfferToPlan(
        acSession.GetInitiator().CurrentOffer,
        acInitiatorInventory,
        acRecipientInventory,
        plan.Initiator,
        plan.Recipient);

    if (error != Error::None)
        return MutationPlanResult::Failure(error);

    error = AddOfferToPlan(
        acSession.GetRecipient().CurrentOffer,
        acRecipientInventory,
        acInitiatorInventory,
        plan.Recipient,
        plan.Initiator);

    if (error != Error::None)
        return MutationPlanResult::Failure(error);

    CanonicalizePlan(plan.Initiator);
    CanonicalizePlan(plan.Recipient);

    return MutationPlanResult::Success(std::move(plan));
}
}
