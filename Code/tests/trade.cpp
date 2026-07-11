#include <catch2/catch.hpp>

#include <Trade/TradeSession.h>

#include <initializer_list>

namespace
{
Trade::Offer MakeOffer(std::initializer_list<Trade::OfferLine> aItems)
{
    Trade::Offer offer;
    offer.Items.assign(aItems.begin(), aItems.end());
    return offer;
}
}

TEST_CASE("Trade session rejects identical participants", "[trade.session]")
{
    Trade::Session session(42, 10, 10, 1000, 3000);

    REQUIRE(session.GetState() == Trade::State::Failed);
    REQUIRE(session.GetTerminalError() == Trade::Error::InvalidParticipant);
    REQUIRE(session.IsTerminal());
}

TEST_CASE("Only the recipient can accept a trade", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 3000);

    const Trade::Result initiatorResult = session.Accept(10, 1100);
    REQUIRE_FALSE(initiatorResult.Succeeded());
    REQUIRE(initiatorResult.ErrorCode == Trade::Error::NotRecipient);
    REQUIRE(session.GetState() == Trade::State::PendingAcceptance);

    const Trade::Result recipientResult = session.Accept(20, 1200);
    REQUIRE(recipientResult.Succeeded());
    REQUIRE(recipientResult.Changed);
    REQUIRE(session.GetState() == Trade::State::Negotiating);
    REQUIRE(session.GetLastActivityTick() == 1200);

    const Trade::Result repeatedResult = session.Accept(20, 1300);
    REQUIRE(repeatedResult.Succeeded());
    REQUIRE_FALSE(repeatedResult.Changed);
    REQUIRE(session.GetLastActivityTick() == 1200);
}

TEST_CASE("Trade offer validation rejects invalid and duplicate lines", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 3000);
    REQUIRE(session.Accept(20, 1100).Succeeded());

    const Trade::Revision revision = session.GetRevision();

    const Trade::Result invalidItem = session.UpdateOffer(10, revision, MakeOffer({{0, 1}}), 1200);
    REQUIRE_FALSE(invalidItem.Succeeded());
    REQUIRE(invalidItem.ErrorCode == Trade::Error::InvalidItem);

    const Trade::Result emptyQuantity = session.UpdateOffer(10, revision, MakeOffer({{100, 0}}), 1200);
    REQUIRE_FALSE(emptyQuantity.Succeeded());
    REQUIRE(emptyQuantity.ErrorCode == Trade::Error::EmptyQuantity);

    const Trade::Result duplicateItem = session.UpdateOffer(10, revision, MakeOffer({{100, 1}, {100, 2}}), 1200);
    REQUIRE_FALSE(duplicateItem.Succeeded());
    REQUIRE(duplicateItem.ErrorCode == Trade::Error::DuplicateItem);

    REQUIRE(session.GetRevision() == revision);
    REQUIRE(session.GetInitiator().CurrentOffer.Items.empty());
}

TEST_CASE("Trade offers are canonical and repeated snapshots are idempotent", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 3000);
    REQUIRE(session.Accept(20, 1100).Succeeded());

    const Trade::Revision initialRevision = session.GetRevision();
    const Trade::Result updateResult = session.UpdateOffer(
        10,
        initialRevision,
        MakeOffer({{200, 2}, {100, 1}}),
        1200);

    REQUIRE(updateResult.Succeeded());
    REQUIRE(updateResult.Changed);
    REQUIRE(session.GetRevision() == initialRevision + 1);
    REQUIRE(session.GetInitiator().CurrentOffer.Items.size() == 2);
    REQUIRE(session.GetInitiator().CurrentOffer.Items[0].Item == 100);
    REQUIRE(session.GetInitiator().CurrentOffer.Items[1].Item == 200);

    const Trade::Revision updatedRevision = session.GetRevision();
    const Trade::Result repeatedResult = session.UpdateOffer(
        10,
        initialRevision,
        MakeOffer({{100, 1}, {200, 2}}),
        1300);

    REQUIRE(repeatedResult.Succeeded());
    REQUIRE_FALSE(repeatedResult.Changed);
    REQUIRE(session.GetRevision() == updatedRevision);
    REQUIRE(session.GetLastActivityTick() == 1200);
}

TEST_CASE("A real offer change increments the revision and clears confirmations", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 3000);
    REQUIRE(session.Accept(20, 1100).Succeeded());

    const Trade::Revision initialRevision = session.GetRevision();
    REQUIRE(session.Confirm(10, initialRevision, 1200).Succeeded());
    REQUIRE(session.GetInitiator().Confirmed);

    const Trade::Result updateResult = session.UpdateOffer(
        20,
        initialRevision,
        MakeOffer({{300, 3}}),
        1300);

    REQUIRE(updateResult.Succeeded());
    REQUIRE(session.GetRevision() == initialRevision + 1);
    REQUIRE_FALSE(session.GetInitiator().Confirmed);
    REQUIRE_FALSE(session.GetRecipient().Confirmed);
    REQUIRE(session.GetInitiator().ConfirmedRevision == 0);
    REQUIRE(session.GetRecipient().ConfirmedRevision == 0);
}

TEST_CASE("Stale trade revisions never mutate the session", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 3000);
    REQUIRE(session.Accept(20, 1100).Succeeded());

    const Trade::Revision oldRevision = session.GetRevision();
    REQUIRE(session.UpdateOffer(10, oldRevision, MakeOffer({{100, 1}}), 1200).Succeeded());

    const Trade::Result confirmResult = session.Confirm(20, oldRevision, 1300);
    REQUIRE_FALSE(confirmResult.Succeeded());
    REQUIRE(confirmResult.ErrorCode == Trade::Error::StaleRevision);
    REQUIRE_FALSE(session.GetRecipient().Confirmed);
    REQUIRE(session.GetState() == Trade::State::Negotiating);
}

TEST_CASE("Two confirmations on the current revision lock the trade", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 3000);
    REQUIRE(session.Accept(20, 1100).Succeeded());

    const Trade::Revision revision = session.GetRevision();

    const Trade::Result firstConfirmation = session.Confirm(10, revision, 1200);
    REQUIRE(firstConfirmation.Succeeded());
    REQUIRE(firstConfirmation.Changed);
    REQUIRE(session.GetState() == Trade::State::Negotiating);

    const Trade::Result repeatedConfirmation = session.Confirm(10, revision, 1250);
    REQUIRE(repeatedConfirmation.Succeeded());
    REQUIRE_FALSE(repeatedConfirmation.Changed);
    REQUIRE(session.GetLastActivityTick() == 1200);

    const Trade::Result secondConfirmation = session.Confirm(20, revision, 1300);
    REQUIRE(secondConfirmation.Succeeded());
    REQUIRE(secondConfirmation.Changed);
    REQUIRE(session.GetState() == Trade::State::Locked);

    const Trade::Result delayedDuplicate = session.Confirm(10, revision, 1400);
    REQUIRE(delayedDuplicate.Succeeded());
    REQUIRE_FALSE(delayedDuplicate.Changed);
    REQUIRE(session.GetState() == Trade::State::Locked);
}

TEST_CASE("Cancellation is idempotent and terminal", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 3000);
    REQUIRE(session.Accept(20, 1100).Succeeded());

    const Trade::Result cancelResult = session.Cancel(10, 1200);
    REQUIRE(cancelResult.Succeeded());
    REQUIRE(cancelResult.Changed);
    REQUIRE(session.GetState() == Trade::State::Cancelled);

    const Trade::Result repeatedCancel = session.Cancel(20, 1300);
    REQUIRE(repeatedCancel.Succeeded());
    REQUIRE_FALSE(repeatedCancel.Changed);
    REQUIRE(session.GetLastActivityTick() == 1200);

    const Trade::Result updateResult = session.UpdateOffer(
        10,
        session.GetRevision(),
        MakeOffer({{100, 1}}),
        1400);
    REQUIRE_FALSE(updateResult.Succeeded());
    REQUIRE(updateResult.ErrorCode == Trade::Error::AlreadyTerminal);
}

TEST_CASE("Applying and completion follow the transaction state machine", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 3000);
    REQUIRE(session.Accept(20, 1100).Succeeded());

    const Trade::Revision revision = session.GetRevision();
    REQUIRE(session.Confirm(10, revision, 1200).Succeeded());
    REQUIRE(session.Confirm(20, revision, 1300).Succeeded());
    REQUIRE(session.GetState() == Trade::State::Locked);

    REQUIRE_FALSE(session.Complete(1350).Succeeded());

    REQUIRE(session.StartApplying(1400).Succeeded());
    REQUIRE(session.GetState() == Trade::State::Applying);

    const Trade::Result repeatedStart = session.StartApplying(1450);
    REQUIRE(repeatedStart.Succeeded());
    REQUIRE_FALSE(repeatedStart.Changed);
    REQUIRE(session.GetLastActivityTick() == 1400);

    REQUIRE_FALSE(session.Cancel(10, 1450).Succeeded());

    const Trade::Result completeResult = session.Complete(1500);
    REQUIRE(completeResult.Succeeded());
    REQUIRE(completeResult.Changed);
    REQUIRE(session.GetState() == Trade::State::Completed);

    const Trade::Result repeatedComplete = session.Complete(1600);
    REQUIRE(repeatedComplete.Succeeded());
    REQUIRE_FALSE(repeatedComplete.Changed);
}

TEST_CASE("Trade expiry records a structured terminal reason", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 2000);

    const Trade::Result earlyResult = session.Expire(1999);
    REQUIRE(earlyResult.Succeeded());
    REQUIRE_FALSE(earlyResult.Changed);
    REQUIRE(session.GetState() == Trade::State::PendingAcceptance);

    const Trade::Result expiryResult = session.Expire(2000);
    REQUIRE_FALSE(expiryResult.Succeeded());
    REQUIRE(expiryResult.Changed);
    REQUIRE(expiryResult.ErrorCode == Trade::Error::SessionExpired);
    REQUIRE(session.GetState() == Trade::State::Cancelled);
    REQUIRE(session.GetTerminalError() == Trade::Error::SessionExpired);

    const Trade::Result repeatedExpiry = session.Expire(2100);
    REQUIRE_FALSE(repeatedExpiry.Succeeded());
    REQUIRE_FALSE(repeatedExpiry.Changed);
    REQUIRE(repeatedExpiry.ErrorCode == Trade::Error::SessionExpired);
}

TEST_CASE("Trade failure records its reason and is idempotent", "[trade.session]")
{
    Trade::Session session(42, 10, 20, 1000, 3000);

    const Trade::Result failureResult = session.Fail(Trade::Error::InvalidItem, 1100);
    REQUIRE(failureResult.Succeeded());
    REQUIRE(failureResult.Changed);
    REQUIRE(session.GetState() == Trade::State::Failed);
    REQUIRE(session.GetTerminalError() == Trade::Error::InvalidItem);

    const Trade::Result repeatedFailure = session.Fail(Trade::Error::InvalidItem, 1200);
    REQUIRE(repeatedFailure.Succeeded());
    REQUIRE_FALSE(repeatedFailure.Changed);
    REQUIRE(session.GetLastActivityTick() == 1100);
}
