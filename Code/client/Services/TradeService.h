#pragma once

#include <Trade/TradeTypes.h>

#include <cstdint>
#include <optional>

struct World;
struct TransportService;
struct NotifyTradeInvite;
struct NotifyTradeStarted;
struct NotifyTradeCancelled;
struct NotifyTradeState;

struct ClientTradeSessionState
{
    Trade::SessionId SessionId{};
    Trade::Revision Revision{};
    Trade::State State{Trade::State::PendingAcceptance};
    Trade::Participant Initiator{};
    Trade::Participant Recipient{};
};

struct TradeService
{
    TradeService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransportService) noexcept;
    ~TradeService() noexcept = default;

    TP_NOCOPYMOVE(TradeService);

    void InvitePlayer(std::uint32_t aPlayerId) const noexcept;
    void AcceptInvite(std::uint64_t aSessionId) const noexcept;
    void RejectInvite(std::uint64_t aSessionId) const noexcept;

    void UpdateOffer(Trade::Offer aOffer) const noexcept;
    void ConfirmTrade() const noexcept;
    void CancelTrade() const noexcept;

    [[nodiscard]] std::optional<std::uint64_t> GetPendingInvite() const noexcept
    {
        return m_pendingInvite;
    }

    [[nodiscard]] std::optional<std::uint64_t> GetActiveSession() const noexcept
    {
        return m_activeSession;
    }

    [[nodiscard]] const std::optional<ClientTradeSessionState>& GetSessionState() const noexcept
    {
        return m_sessionState;
    }

protected:
    void OnNotifyTradeInvite(const NotifyTradeInvite& acTradeInvite) noexcept;
    void OnNotifyTradeStarted(const NotifyTradeStarted& acTradeStarted) noexcept;
    void OnNotifyTradeCancelled(const NotifyTradeCancelled& acTradeCancelled) noexcept;
    void OnNotifyTradeState(const NotifyTradeState& acTradeState) noexcept;

private:
    void RespondToInvite(std::uint64_t aSessionId, bool aAccepted) const noexcept;

    World& m_world;
    TransportService& m_transport;

    std::optional<std::uint64_t> m_pendingInvite;
    std::optional<std::uint64_t> m_activeSession;
    std::optional<ClientTradeSessionState> m_sessionState;

    entt::scoped_connection m_tradeInviteConnection;
    entt::scoped_connection m_tradeStartedConnection;
    entt::scoped_connection m_tradeCancelledConnection;
    entt::scoped_connection m_tradeStateConnection;
};
