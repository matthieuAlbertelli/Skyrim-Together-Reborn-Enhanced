#include <catch2/catch.hpp>

#include <Trade/TradeInventory.h>
#include <Trade/TradeSession.h>

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <utility>

namespace
{
Trade::Offer MakeOffer(std::initializer_list<Trade::OfferLine> aItems)
{
    Trade::Offer offer;
    offer.Items.assign(aItems.begin(), aItems.end());
    return offer;
}

Trade::InventorySnapshot MakeInventory(
    Trade::PlayerId aOwner,
    std::initializer_list<Trade::InventoryItemState> aItems)
{
    Trade::InventorySnapshot inventory;
    inventory.Owner = aOwner;
    inventory.Items.assign(aItems.begin(), aItems.end());
    return inventory;
}

Trade::Session MakeLockedSession(
    Trade::Offer aInitiatorOffer,
    Trade::Offer aRecipientOffer = {})
{
    Trade::Session session(42, 10, 20, 1000, 5000);
    REQUIRE(session.Accept(20, 1100).Succeeded());

    Trade::Revision revision = session.GetRevision();
    REQUIRE(session.UpdateOffer(10, revision, std::move(aInitiatorOffer), 1200).Succeeded());

    revision = session.GetRevision();
    REQUIRE(session.UpdateOffer(20, revision, std::move(aRecipientOffer), 1300).Succeeded());

    revision = session.GetRevision();
    REQUIRE(session.Confirm(10, revision, 1400).Succeeded());
    REQUIRE(session.Confirm(20, revision, 1500).Succeeded());
    REQUIRE(session.GetState() == Trade::State::Locked);

    return session;
}
}

TEST_CASE("Trade mutation plan transfers both offers", "[trade.inventory]")
{
    Trade::Session session = MakeLockedSession(
        MakeOffer({{100, 3}}),
        MakeOffer({{200, 2}}));

    const Trade::InventorySnapshot initiator =
        MakeInventory(10, {{100, 10, true, false}});
    const Trade::InventorySnapshot recipient =
        MakeInventory(20, {{200, 5, true, false}});

    const Trade::MutationPlanResult result =
        Trade::BuildMutationPlan(session, initiator, recipient);

    REQUIRE(result.Succeeded());
    REQUIRE(result.Plan.Session == 42);
    REQUIRE(result.Plan.RevisionNumber == session.GetRevision());
    REQUIRE(result.Plan.Initiator.Player == 10);
    REQUIRE(result.Plan.Recipient.Player == 20);

    REQUIRE(result.Plan.Initiator.Mutations ==
        std::vector<Trade::InventoryMutation>{{100, -3}, {200, 2}});
    REQUIRE(result.Plan.Recipient.Mutations ==
        std::vector<Trade::InventoryMutation>{{100, 3}, {200, -2}});
}

TEST_CASE("Trade mutation plan rejects insufficient quantities", "[trade.inventory]")
{
    Trade::Session session = MakeLockedSession(MakeOffer({{100, 6}}));

    const Trade::MutationPlanResult result = Trade::BuildMutationPlan(
        session,
        MakeInventory(10, {{100, 5, true, false}}),
        MakeInventory(20, {}));

    REQUIRE_FALSE(result.Succeeded());
    REQUIRE(result.ErrorCode == Trade::Error::InsufficientQuantity);
}

TEST_CASE("Trade mutation plan rejects non-transferable items", "[trade.inventory]")
{
    Trade::Session session = MakeLockedSession(MakeOffer({{100, 1}}));

    const Trade::MutationPlanResult result = Trade::BuildMutationPlan(
        session,
        MakeInventory(10, {{100, 1, false, false}}),
        MakeInventory(20, {}));

    REQUIRE_FALSE(result.Succeeded());
    REQUIRE(result.ErrorCode == Trade::Error::ItemNotTransferable);
}

TEST_CASE("Trade mutation plan rejects ambiguous inventory entries", "[trade.inventory]")
{
    Trade::Session session = MakeLockedSession(MakeOffer({{100, 1}}));

    SECTION("Snapshot explicitly marks the item ambiguous")
    {
        const Trade::MutationPlanResult result = Trade::BuildMutationPlan(
            session,
            MakeInventory(10, {{100, 2, true, true}}),
            MakeInventory(20, {}));

        REQUIRE_FALSE(result.Succeeded());
        REQUIRE(result.ErrorCode == Trade::Error::AmbiguousItem);
    }

    SECTION("Snapshot contains duplicate logical item identifiers")
    {
        const Trade::MutationPlanResult result = Trade::BuildMutationPlan(
            session,
            MakeInventory(10, {
                {100, 1, true, false},
                {100, 1, true, false}
            }),
            MakeInventory(20, {}));

        REQUIRE_FALSE(result.Succeeded());
        REQUIRE(result.ErrorCode == Trade::Error::AmbiguousItem);
    }
}

TEST_CASE("Trade mutation plan rejects quantities that cannot become Inventory entry deltas", "[trade.inventory]")
{
    Trade::Session session = MakeLockedSession(
        MakeOffer({{
            100,
            static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()) + 1u
        }}));

    const Trade::MutationPlanResult result = Trade::BuildMutationPlan(
        session,
        MakeInventory(10, {{
            100,
            static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max()) + 1u,
            true,
            false
        }}),
        MakeInventory(20, {}));

    REQUIRE_FALSE(result.Succeeded());
    REQUIRE(result.ErrorCode == Trade::Error::QuantityOverflow);
}

TEST_CASE("Reciprocal offers for the same item are netted deterministically", "[trade.inventory]")
{
    Trade::Session session = MakeLockedSession(
        MakeOffer({{100, 5}}),
        MakeOffer({{100, 2}}));

    const Trade::MutationPlanResult result = Trade::BuildMutationPlan(
        session,
        MakeInventory(10, {{100, 5, true, false}}),
        MakeInventory(20, {{100, 2, true, false}}));

    REQUIRE(result.Succeeded());
    REQUIRE(result.Plan.Initiator.Mutations ==
        std::vector<Trade::InventoryMutation>{{100, -3}});
    REQUIRE(result.Plan.Recipient.Mutations ==
        std::vector<Trade::InventoryMutation>{{100, 3}});
}

TEST_CASE("Mutation planning requires a locked session and matching inventory owners", "[trade.inventory]")
{
    Trade::Session negotiating(42, 10, 20, 1000, 5000);
    REQUIRE(negotiating.Accept(20, 1100).Succeeded());

    const Trade::MutationPlanResult invalidState = Trade::BuildMutationPlan(
        negotiating,
        MakeInventory(10, {}),
        MakeInventory(20, {}));

    REQUIRE_FALSE(invalidState.Succeeded());
    REQUIRE(invalidState.ErrorCode == Trade::Error::InvalidState);

    Trade::Session locked = MakeLockedSession({});

    const Trade::MutationPlanResult wrongOwner = Trade::BuildMutationPlan(
        locked,
        MakeInventory(999, {}),
        MakeInventory(20, {}));

    REQUIRE_FALSE(wrongOwner.Succeeded());
    REQUIRE(wrongOwner.ErrorCode == Trade::Error::InvalidParticipant);
}


TEST_CASE("Trade mutation plan rejects incompatible destination stacks", "[trade.inventory]")
{
    Trade::Session session = MakeLockedSession(
        MakeOffer({{100, 1}}));

    SECTION("Destination contains a non-transferable instance")
    {
        const Trade::MutationPlanResult result =
            Trade::BuildMutationPlan(
                session,
                MakeInventory(10, {{100, 1, true, false}}),
                MakeInventory(20, {{100, 1, false, false}}));

        REQUIRE_FALSE(result.Succeeded());
        REQUIRE(result.ErrorCode ==
            Trade::Error::ItemNotTransferable);
    }

    SECTION("Destination contains an ambiguous stack")
    {
        const Trade::MutationPlanResult result =
            Trade::BuildMutationPlan(
                session,
                MakeInventory(10, {{100, 1, true, false}}),
                MakeInventory(20, {{100, 1, true, true}}));

        REQUIRE_FALSE(result.Succeeded());
        REQUIRE(result.ErrorCode ==
            Trade::Error::AmbiguousItem);
    }
}
