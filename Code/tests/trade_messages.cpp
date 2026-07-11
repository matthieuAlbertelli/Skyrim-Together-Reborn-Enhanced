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
#include <Messages/TradeInviteRequest.h>
#include <Messages/TradeInviteResponseRequest.h>

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
