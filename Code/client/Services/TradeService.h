#pragma once

#include <cstdint>
#include <optional>

struct World;
struct TransportService;
struct NotifyTradeInvite;
struct NotifyTradeStarted;
struct NotifyTradeCancelled;

struct TradeService
{
    TradeService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransportService) noexcept;
    ~TradeService() noexcept = default;

    TP_NOCOPYMOVE(TradeService);

    void InvitePlayer(std::uint32_t aPlayerId) const noexcept;
    void AcceptInvite(std::uint64_t aSessionId) const noexcept;
    void RejectInvite(std::uint64_t aSessionId) const noexcept;

    [[nodiscard]] std::optional<std::uint64_t> GetPendingInvite() const noexcept
    {
        return m_pendingInvite;
    }

    [[nodiscard]] std::optional<std::uint64_t> GetActiveSession() const noexcept
    {
        return m_activeSession;
    }

protected:
    void OnNotifyTradeInvite(const NotifyTradeInvite& acTradeInvite) noexcept;
    void OnNotifyTradeStarted(const NotifyTradeStarted& acTradeStarted) noexcept;
    void OnNotifyTradeCancelled(const NotifyTradeCancelled& acTradeCancelled) noexcept;

private:
    void RespondToInvite(std::uint64_t aSessionId, bool aAccepted) const noexcept;

    World& m_world;
    TransportService& m_transport;

    std::optional<std::uint64_t> m_pendingInvite;
    std::optional<std::uint64_t> m_activeSession;

    entt::scoped_connection m_tradeInviteConnection;
    entt::scoped_connection m_tradeStartedConnection;
    entt::scoped_connection m_tradeCancelledConnection;
};
