#include <catch2/catch.hpp>

#include <Trade/TradeReconciliation.h>

#include <initializer_list>
#include <utility>
#include <vector>

namespace
{
Trade::MutationPlan MakeMutationPlan()
{
    Trade::MutationPlan plan;
    plan.Session = 42;
    plan.RevisionNumber = 7;
    plan.Initiator.Player = 10;
    plan.Recipient.Player = 20;
    plan.Initiator.Mutations = {
        {100, -2},
        {200, 3}
    };
    plan.Recipient.Mutations = {
        {100, 2},
        {200, -3}
    };
    return plan;
}

Trade::Application MakeApplication()
{
    return Trade::Application(
        99,
        MakeMutationPlan(),
        1000,
        5000);
}

Trade::InventorySnapshot MakeInventory(
    Trade::PlayerId aOwner,
    std::initializer_list<Trade::InventoryItemState> aItems)
{
    Trade::InventorySnapshot inventory;
    inventory.Owner = aOwner;
    inventory.Items.assign(
        aItems.begin(),
        aItems.end());
    return inventory;
}
}

TEST_CASE("Reconciliation plan restores authoritative absolute quantities", "[trade.reconciliation]")
{
    Trade::Application application =
        MakeApplication();

    const Trade::ReconciliationPlanResult result =
        Trade::BuildReconciliationPlan(
            application,
            MakeInventory(
                10,
                {{100, 5, true, false}}),
            MakeInventory(
                20,
                {{200, 8, true, false}}));

    REQUIRE(result.Succeeded());
    REQUIRE(result.Plan.Session == 42);
    REQUIRE(result.Plan.RevisionNumber == 7);
    REQUIRE(result.Plan.Application == 99);

    REQUIRE(result.Plan.Initiator.Targets ==
        std::vector<Trade::InventoryTarget>{
            {100, 5},
            {200, 0}
        });

    REQUIRE(result.Plan.Recipient.Targets ==
        std::vector<Trade::InventoryTarget>{
            {100, 0},
            {200, 8}
        });
}

TEST_CASE("Reconciliation plan rejects ambiguous authoritative inventory", "[trade.reconciliation]")
{
    Trade::Application application =
        MakeApplication();

    const Trade::ReconciliationPlanResult result =
        Trade::BuildReconciliationPlan(
            application,
            MakeInventory(
                10,
                {{100, 5, true, true}}),
            MakeInventory(
                20,
                {{200, 8, true, false}}));

    REQUIRE_FALSE(result.Succeeded());
    REQUIRE(result.ErrorCode ==
        Trade::Error::AmbiguousItem);
}

TEST_CASE("Reconciliation results are idempotent and settle after both participants", "[trade.reconciliation]")
{
    Trade::ReconciliationPlan plan;
    plan.Session = 42;
    plan.RevisionNumber = 7;
    plan.Application = 99;
    plan.Initiator.Player = 10;
    plan.Recipient.Player = 20;

    Trade::Reconciliation reconciliation(
        123,
        std::move(plan),
        1000,
        5000);

    const Trade::Result first =
        reconciliation.RecordResult(
            10,
            Trade::ReconcileOutcome::Success,
            1200);

    REQUIRE(first.Succeeded());
    REQUIRE(first.Changed);

    const Trade::Result duplicate =
        reconciliation.RecordResult(
            10,
            Trade::ReconcileOutcome::Success,
            1300);

    REQUIRE(duplicate.Succeeded());
    REQUIRE_FALSE(duplicate.Changed);
    REQUIRE_FALSE(reconciliation.IsSettled());

    REQUIRE(reconciliation.RecordResult(
        20,
        Trade::ReconcileOutcome::Success,
        1400).Succeeded());

    REQUIRE(reconciliation.IsSettled());
    REQUIRE(reconciliation.AllSucceeded());
    REQUIRE_FALSE(reconciliation.HasFailed());
}

TEST_CASE("Reconciliation detects conflicting results and unavailable participants", "[trade.reconciliation]")
{
    Trade::ReconciliationPlan plan;
    plan.Session = 42;
    plan.RevisionNumber = 7;
    plan.Application = 99;
    plan.Initiator.Player = 10;
    plan.Recipient.Player = 20;

    Trade::Reconciliation reconciliation(
        123,
        std::move(plan),
        1000,
        5000);

    REQUIRE(reconciliation.RecordResult(
        10,
        Trade::ReconcileOutcome::Success,
        1200).Succeeded());

    const Trade::Result conflict =
        reconciliation.RecordResult(
            10,
            Trade::ReconcileOutcome::LocalStateMismatch,
            1300);

    REQUIRE_FALSE(conflict.Succeeded());
    REQUIRE(conflict.ErrorCode ==
        Trade::Error::ReconcileResultConflict);

    REQUIRE(reconciliation.MarkUnavailable(
        20,
        1400).Succeeded());

    REQUIRE(reconciliation.IsSettled());
    REQUIRE(reconciliation.HasFailed());
    REQUIRE_FALSE(reconciliation.AllSucceeded());
}

TEST_CASE("Reconciliation timeout only applies while pending", "[trade.reconciliation]")
{
    Trade::ReconciliationPlan plan;
    plan.Session = 42;
    plan.RevisionNumber = 7;
    plan.Application = 99;
    plan.Initiator.Player = 10;
    plan.Recipient.Player = 20;

    Trade::Reconciliation reconciliation(
        123,
        std::move(plan),
        1000,
        5000);

    REQUIRE_FALSE(reconciliation.IsTimedOut(4999));
    REQUIRE(reconciliation.IsTimedOut(5000));

    REQUIRE(reconciliation.MarkUnavailable(
        10,
        5100).Succeeded());
    REQUIRE(reconciliation.MarkUnavailable(
        20,
        5100).Succeeded());

    REQUIRE_FALSE(reconciliation.IsTimedOut(6000));
}
