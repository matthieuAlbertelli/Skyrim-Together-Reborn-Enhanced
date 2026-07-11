#include <Services/TradeService.h>

#include <Services/TransportService.h>

#include <Messages/NotifyTradeCancelled.h>
#include <Messages/NotifyTradeInvite.h>
#include <Messages/NotifyTradeStarted.h>
#include <Messages/TradeInviteRequest.h>
#include <Messages/TradeInviteResponseRequest.h>

#include <World.h>

TradeService::TradeService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransportService) noexcept
    : m_world(aWorld)
    , m_transport(aTransportService)
    , m_tradeInviteConnection(aDispatcher.sink<NotifyTradeInvite>().connect<&TradeService::OnNotifyTradeInvite>(this))
    , m_tradeStartedConnection(aDispatcher.sink<NotifyTradeStarted>().connect<&TradeService::OnNotifyTradeStarted>(this))
    , m_tradeCancelledConnection(aDispatcher.sink<NotifyTradeCancelled>().connect<&TradeService::OnNotifyTradeCancelled>(this))
{
}

void TradeService::InvitePlayer(std::uint32_t aPlayerId) const noexcept
{
    if (!m_transport.IsConnected())
    {
        spdlog::warn("[TradeService]: cannot send trade invite, transport is not connected");
        return;
    }

    TradeInviteRequest request;
    request.TargetPlayerId = aPlayerId;

    m_transport.Send(request);
    spdlog::info("[TradeService]: sent trade invite request to player {}", aPlayerId);
}

void TradeService::AcceptInvite(std::uint64_t aSessionId) const noexcept
{
    RespondToInvite(aSessionId, true);
}

void TradeService::RejectInvite(std::uint64_t aSessionId) const noexcept
{
    RespondToInvite(aSessionId, false);
}

void TradeService::RespondToInvite(std::uint64_t aSessionId, bool aAccepted) const noexcept
{
    if (!m_transport.IsConnected())
    {
        spdlog::warn("[TradeService]: cannot respond to trade invite, transport is not connected");
        return;
    }

    if (!m_pendingInvite || *m_pendingInvite != aSessionId)
    {
        spdlog::warn(
            "[TradeService]: cannot respond to unknown trade invite session {}",
            aSessionId);
        return;
    }

    TradeInviteResponseRequest request;
    request.SessionId = aSessionId;
    request.Accepted = aAccepted;

    m_transport.Send(request);
    spdlog::info(
        "[TradeService]: sent trade invite response session {} accepted={}",
        aSessionId,
        aAccepted);
}

void TradeService::OnNotifyTradeInvite(const NotifyTradeInvite& acTradeInvite) noexcept
{
    m_pendingInvite = acTradeInvite.SessionId;

    spdlog::info(
        "[TradeService]: received trade invite session {} from player {}, expires at tick {}",
        acTradeInvite.SessionId,
        acTradeInvite.InviterId,
        acTradeInvite.ExpiryTick);
}

void TradeService::OnNotifyTradeStarted(const NotifyTradeStarted& acTradeStarted) noexcept
{
    if (m_pendingInvite && *m_pendingInvite == acTradeStarted.SessionId)
        m_pendingInvite.reset();

    m_activeSession = acTradeStarted.SessionId;

    spdlog::info(
        "[TradeService]: trade session {} started between players {} and {} at revision {}",
        acTradeStarted.SessionId,
        acTradeStarted.InitiatorId,
        acTradeStarted.RecipientId,
        acTradeStarted.Revision);
}

void TradeService::OnNotifyTradeCancelled(const NotifyTradeCancelled& acTradeCancelled) noexcept
{
    if (m_pendingInvite && *m_pendingInvite == acTradeCancelled.SessionId)
        m_pendingInvite.reset();

    if (m_activeSession && *m_activeSession == acTradeCancelled.SessionId)
        m_activeSession.reset();

    spdlog::info(
        "[TradeService]: trade session {} cancelled reason={} actor={}",
        acTradeCancelled.SessionId,
        static_cast<std::uint32_t>(acTradeCancelled.Reason),
        acTradeCancelled.ActorId);
}
