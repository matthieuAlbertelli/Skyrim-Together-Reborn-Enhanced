#include <Services/TradeService.h>

#include <GameServer.h>
#include <World.h>

#include <Messages/NotifyTradeInvite.h>
#include <Messages/TradeInviteRequest.h>

TradeService::TradeService(World& aWorld, entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_tradeInviteConnection(aDispatcher.sink<PacketEvent<TradeInviteRequest>>().connect<&TradeService::OnTradeInviteRequest>(this))
{
}

void TradeService::OnTradeInviteRequest(const PacketEvent<TradeInviteRequest>& acPacket) noexcept
{
    auto& message = acPacket.Packet;

    Player* const pInviter = acPacket.pPlayer;
    Player* const pInvitee = m_world.GetPlayerManager().GetById(message.TargetPlayerId);

    if (!pInviter)
        return;

    spdlog::info("[TradeService]: invite request from player {} to player {}", pInviter->GetId(), message.TargetPlayerId);

    if (!pInvitee)
    {
        spdlog::warn("[TradeService]: invite rejected, target player {} does not exist", message.TargetPlayerId);
        return;
    }

    if (pInvitee == pInviter)
    {
        spdlog::warn("[TradeService]: invite rejected, player {} tried to trade with themselves", pInviter->GetId());
        return;
    }

    NotifyTradeInvite notify;
    notify.SessionId = m_nextSessionId++;
    notify.InviterId = pInviter->GetId();
    notify.ExpiryTick = GameServer::Get()->GetTick() + 30000;

    pInvitee->Send(notify);

    spdlog::info(
        "[TradeService]: sent invite session {} from player {} to player {}",
        notify.SessionId,
        notify.InviterId,
        message.TargetPlayerId);
}
