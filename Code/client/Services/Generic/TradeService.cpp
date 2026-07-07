#include <Services/TradeService.h>

#include <Services/TransportService.h>

#include <Messages/NotifyTradeInvite.h>
#include <Messages/TradeInviteRequest.h>

#include <World.h>

TradeService::TradeService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransportService) noexcept
    : m_world(aWorld)
    , m_transport(aTransportService)
    , m_tradeInviteConnection(aDispatcher.sink<NotifyTradeInvite>().connect<&TradeService::OnNotifyTradeInvite>(this))
{
}

void TradeService::InvitePlayer(uint32_t aPlayerId) const noexcept
{
    if (!m_transport.IsConnected())
    {
        spdlog::warn("[TradeService]: cannot send trade invite, transport is not connected");
        return;
    }

    TradeInviteRequest request;
    request.TargetPlayerId = aPlayerId;

    if (m_transport.Send(request))
        spdlog::info("[TradeService]: sent trade invite request to player {}", aPlayerId);
    else
        spdlog::warn("[TradeService]: failed to send trade invite request to player {}", aPlayerId);
}

void TradeService::OnNotifyTradeInvite(const NotifyTradeInvite& acTradeInvite) noexcept
{
    spdlog::info(
        "[TradeService]: received trade invite session {} from player {}, expires at tick {}",
        acTradeInvite.SessionId,
        acTradeInvite.InviterId,
        acTradeInvite.ExpiryTick);
}
