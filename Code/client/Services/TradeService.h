#pragma once

struct World;
struct TransportService;
struct NotifyTradeInvite;

/**
 * @brief Client-side trade workflow facade.
 *
 * V1 only logs trade invitations and exposes a method to send an invite request.
 * Actual UI/session handling is intentionally deferred to the next step.
 */
struct TradeService
{
    TradeService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransportService) noexcept;
    ~TradeService() noexcept = default;

    TP_NOCOPYMOVE(TradeService);

    void InvitePlayer(uint32_t aPlayerId) const noexcept;

protected:
    void OnNotifyTradeInvite(const NotifyTradeInvite& acTradeInvite) noexcept;

private:
    World& m_world;
    TransportService& m_transport;

    entt::scoped_connection m_tradeInviteConnection;
};
