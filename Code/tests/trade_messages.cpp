#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Allocator.hpp>
#include <TiltedCore/Buffer.hpp>
#include <TiltedCore/Serialization.hpp>

#include <catch2/catch.hpp>
#include <glm/glm.hpp>

#include <Messages/ClientMessageFactory.h>
#include <Messages/ServerMessageFactory.h>
#include <Messages/NotifyTradeCancelled.h>
#include <Messages/NotifyTradeInvite.h>
#include <Messages/NotifyTradeStarted.h>
#include <Messages/NotifyTradeState.h>
#include <Messages/NotifyTradeApply.h>
#include <Messages/TradeInviteRequest.h>
#include <Messages/TradeInviteResponseRequest.h>
#include <Messages/TradeOfferUpdateRequest.h>
#include <Messages/TradeConfirmRequest.h>
#include <Messages/TradeCancelRequest.h>
#include <Messages/TradeApplyResultRequest.h>
#include <Structs/TradeOffer.h>
#include <Structs/TradeApplication.h>

using namespace TiltedPhoques;

TEST_CASE("Trade invite response request round-trips through the client factory", "[trade.protocol]")
{
    TradeInviteResponseRequest request;
    request.SessionId = 0xFEDCBA9876543210ull;
    request.Accepted = true;

    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    request.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ClientMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == request.GetOpcode());

    auto pRequest = CastUnique<TradeInviteResponseRequest>(std::move(pMessage));
    REQUIRE(*pRequest == request);
}

TEST_CASE("Trade invite notification preserves 64-bit session identifiers", "[trade.protocol]")
{
    NotifyTradeInvite notification;
    notification.SessionId = 0xFEDCBA9876543210ull;
    notification.InviterId = 0xF1234567u;
    notification.ExpiryTick = 0xABCDEF0123456789ull;

    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    notification.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ServerMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == notification.GetOpcode());

    auto pNotification = CastUnique<NotifyTradeInvite>(std::move(pMessage));
    REQUIRE(*pNotification == notification);
}

TEST_CASE("Trade started notification round-trips through the server factory", "[trade.protocol]")
{
    NotifyTradeStarted notification;
    notification.SessionId = 0x123456789ABCDEF0ull;
    notification.InitiatorId = 17;
    notification.RecipientId = 29;
    notification.Revision = 7;

    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    notification.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ServerMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == notification.GetOpcode());

    auto pNotification = CastUnique<NotifyTradeStarted>(std::move(pMessage));
    REQUIRE(*pNotification == notification);
}

TEST_CASE("Trade cancelled notification round-trips through the server factory", "[trade.protocol]")
{
    NotifyTradeCancelled notification;
    notification.SessionId = 91;
    notification.ActorId = 29;
    notification.Reason = TradeCancelReason::PlayerDisconnected;

    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    notification.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ServerMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == notification.GetOpcode());

    auto pNotification = CastUnique<NotifyTradeCancelled>(std::move(pMessage));
    REQUIRE(*pNotification == notification);
}


TEST_CASE("Trade offer update request round-trips through the client factory", "[trade.protocol]")
{
    TradeOfferUpdateRequest request;
    request.SessionId = 0x1020304050607080ull;
    request.ExpectedRevision = 19;
    request.Offer.Items = {
        {0x1111222233334444ull, 2},
        {0xAAAABBBBCCCCDDDDull, 7}
    };

    Buffer buffer(256);
    Buffer::Writer writer(&buffer);
    request.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ClientMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == request.GetOpcode());

    auto pRequest = CastUnique<TradeOfferUpdateRequest>(std::move(pMessage));
    REQUIRE(*pRequest == request);
    REQUIRE(pRequest->Offer.Valid);
}

TEST_CASE("Trade confirmation request round-trips through the client factory", "[trade.protocol]")
{
    TradeConfirmRequest request;
    request.SessionId = 1234;
    request.Revision = 22;

    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    request.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ClientMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == request.GetOpcode());

    auto pRequest = CastUnique<TradeConfirmRequest>(std::move(pMessage));
    REQUIRE(*pRequest == request);
}

TEST_CASE("Trade cancellation request round-trips through the client factory", "[trade.protocol]")
{
    TradeCancelRequest request;
    request.SessionId = 0xABCDEF0123456789ull;

    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    request.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ClientMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == request.GetOpcode());

    auto pRequest = CastUnique<TradeCancelRequest>(std::move(pMessage));
    REQUIRE(*pRequest == request);
}

TEST_CASE("Authoritative trade state round-trips through the server factory", "[trade.protocol]")
{
    NotifyTradeState notification;
    notification.SessionId = 77;
    notification.Revision = 5;
    notification.State = 1;
    notification.TerminalError = 0;
    notification.InitiatorId = 12;
    notification.RecipientId = 24;
    notification.InitiatorOffer.Items = {
        {1001, 2},
        {1002, 4}
    };
    notification.RecipientOffer.Items = {
        {2001, 1}
    };
    notification.InitiatorConfirmed = true;
    notification.RecipientConfirmed = false;
    notification.InitiatorConfirmedRevision = 5;
    notification.RecipientConfirmedRevision = 0;

    Buffer buffer(512);
    Buffer::Writer writer(&buffer);
    notification.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ServerMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() == notification.GetOpcode());

    auto pNotification = CastUnique<NotifyTradeState>(std::move(pMessage));
    REQUIRE(*pNotification == notification);
    REQUIRE(pNotification->InitiatorOffer.Valid);
    REQUIRE(pNotification->RecipientOffer.Valid);
}

TEST_CASE("Trade offer decoder rejects oversized snapshots", "[trade.protocol]")
{
    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    Serialization::WriteVarInt(writer, TradeOfferData::MaxLines + 1);

    Buffer::Reader reader(&buffer);
    TradeOfferData offer;
    offer.Deserialize(reader);

    REQUIRE_FALSE(offer.Valid);
    REQUIRE(offer.Items.empty());
}


TEST_CASE("Trade apply notification preserves signed mutations", "[trade.protocol]")
{
    NotifyTradeApply notification;
    notification.SessionId = 77;
    notification.Revision = 9;
    notification.ApplyId = 123456789;
    notification.Plan.Mutations = {
        {1001, -3},
        {2002, 5}
    };

    Buffer buffer(256);
    Buffer::Writer writer(&buffer);
    notification.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ServerMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() ==
        notification.GetOpcode());

    auto pNotification =
        CastUnique<NotifyTradeApply>(
            std::move(pMessage));

    REQUIRE(*pNotification == notification);
    REQUIRE(pNotification->Plan.Valid);
}

TEST_CASE("Trade apply result request round-trips through the client factory", "[trade.protocol]")
{
    TradeApplyResultRequest request;
    request.SessionId = 77;
    request.Revision = 9;
    request.ApplyId = 123456789;
    request.Result =
        TradeApplyResultCode::LocalStateMismatch;

    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    request.Serialize(writer);

    Buffer::Reader reader(&buffer);
    const ClientMessageFactory factory;
    auto pMessage = factory.Extract(reader);

    REQUIRE(pMessage);
    REQUIRE(pMessage->GetOpcode() ==
        request.GetOpcode());

    auto pRequest =
        CastUnique<TradeApplyResultRequest>(
            std::move(pMessage));

    REQUIRE(*pRequest == request);
    REQUIRE(pRequest->Valid);
}

TEST_CASE("Trade apply decoder rejects oversized mutation plans", "[trade.protocol]")
{
    Buffer buffer(128);
    Buffer::Writer writer(&buffer);
    Serialization::WriteVarInt(
        writer,
        TradeMutationPlanData::MaxMutations + 1);

    Buffer::Reader reader(&buffer);
    TradeMutationPlanData plan;
    plan.Deserialize(reader);

    REQUIRE_FALSE(plan.Valid);
    REQUIRE(plan.Mutations.empty());
}
