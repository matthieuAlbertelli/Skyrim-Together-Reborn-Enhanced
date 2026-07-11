#include <catch2/catch.hpp>

#include <Trade/TradeApplication.h>

namespace
{
Trade::MutationPlan MakePlan()
{
    Trade::MutationPlan plan;
    plan.Session = 42;
    plan.RevisionNumber = 7;
    plan.Initiator.Player = 10;
    plan.Recipient.Player = 20;
    plan.Initiator.Mutations.push_back({100, -2});
    plan.Recipient.Mutations.push_back({100, 2});
    return plan;
}
}

TEST_CASE("Trade application settles after both successful results", "[trade.application]")
{
    Trade::Application application(
        99,
        MakePlan(),
        1000,
        5000);

    REQUIRE_FALSE(application.IsSettled());
    REQUIRE_FALSE(application.AllSucceeded());

    const Trade::Result first =
        application.RecordResult(
            10,
            Trade::ApplyOutcome::Success,
            1200);

    REQUIRE(first.Succeeded());
    REQUIRE(first.Changed);
    REQUIRE_FALSE(application.IsSettled());

    const Trade::Result second =
        application.RecordResult(
            20,
            Trade::ApplyOutcome::Success,
            1300);

    REQUIRE(second.Succeeded());
    REQUIRE(second.Changed);
    REQUIRE(application.IsSettled());
    REQUIRE(application.AllSucceeded());
    REQUIRE_FALSE(application.HasFailed());
}

TEST_CASE("Trade application result retransmission is idempotent", "[trade.application]")
{
    Trade::Application application(
        99,
        MakePlan(),
        1000,
        5000);

    REQUIRE(application.RecordResult(
        10,
        Trade::ApplyOutcome::Success,
        1200).Succeeded());

    const Trade::Result duplicate =
        application.RecordResult(
            10,
            Trade::ApplyOutcome::Success,
            1400);

    REQUIRE(duplicate.Succeeded());
    REQUIRE_FALSE(duplicate.Changed);
    REQUIRE(application.GetInitiator().Status ==
        Trade::ApplyStatus::Succeeded);
}

TEST_CASE("Trade application rejects conflicting duplicate results", "[trade.application]")
{
    Trade::Application application(
        99,
        MakePlan(),
        1000,
        5000);

    REQUIRE(application.RecordResult(
        10,
        Trade::ApplyOutcome::Success,
        1200).Succeeded());

    const Trade::Result conflict =
        application.RecordResult(
            10,
            Trade::ApplyOutcome::LocalStateMismatch,
            1300);

    REQUIRE_FALSE(conflict.Succeeded());
    REQUIRE(conflict.ErrorCode ==
        Trade::Error::ApplyResultConflict);
}

TEST_CASE("Trade application exposes participant failure", "[trade.application]")
{
    Trade::Application application(
        99,
        MakePlan(),
        1000,
        5000);

    const Trade::Result failure =
        application.RecordResult(
            20,
            Trade::ApplyOutcome::InsufficientQuantity,
            1200);

    REQUIRE(failure.Succeeded());
    REQUIRE(application.HasFailed());
    REQUIRE_FALSE(application.AllSucceeded());
    REQUIRE(application.GetRecipient().Status ==
        Trade::ApplyStatus::Failed);
    REQUIRE(application.GetRecipient().Outcome ==
        Trade::ApplyOutcome::InsufficientQuantity);
}

TEST_CASE("Trade application validates participants and timeout", "[trade.application]")
{
    Trade::Application application(
        99,
        MakePlan(),
        1000,
        5000);

    const Trade::Result outsider =
        application.RecordResult(
            999,
            Trade::ApplyOutcome::Success,
            1200);

    REQUIRE_FALSE(outsider.Succeeded());
    REQUIRE(outsider.ErrorCode ==
        Trade::Error::NotParticipant);

    REQUIRE_FALSE(application.IsTimedOut(4999));
    REQUIRE(application.IsTimedOut(5000));
}
