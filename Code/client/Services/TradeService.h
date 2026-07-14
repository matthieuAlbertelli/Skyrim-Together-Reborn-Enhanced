#pragma once

#include <Trade/TradeError.h>
#include <Trade/TradeTypes.h>

#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>

struct World;
struct TransportService;
struct NotifyTradeInvite;
struct NotifyTradeStarted;
struct NotifyTradeCancelled;
struct NotifyTradeState;
struct NotifyTradeApply;
struct NotifyTradeReconcile;
enum class TradeApplyResultCode : std::uint8_t;
enum class TradeReconcileResultCode : std::uint8_t;

struct ClientPendingTradeInviteState
{
    Trade::SessionId SessionId{};
    Trade::PlayerId InviterId{};
    Trade::Tick ExpiryTick{};
};

struct ClientTradeSessionState
{
    Trade::SessionId SessionId{};
    Trade::Revision Revision{};
    Trade::State State{Trade::State::PendingAcceptance};
    Trade::Error TerminalError{Trade::Error::None};
    Trade::Participant Initiator{};
    Trade::Participant Recipient{};
};

struct ClientTradeApplyRecord
{
    Trade::SessionId SessionId{};
    Trade::Revision Revision{};
    std::uint8_t ResultCode{};
};

struct ClientTradeReconcileRecord
{
    Trade::SessionId SessionId{};
    Trade::Revision Revision{};
    Trade::ApplyId Apply{};
    std::uint8_t ResultCode{};
};

struct TradeService
{
    TradeService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransportService) noexcept;
    ~TradeService() noexcept = default;

    TP_NOCOPYMOVE(TradeService);

    void InvitePlayer(std::uint32_t aPlayerId) noexcept;
    void AcceptInvite(std::uint64_t aSessionId) const noexcept;
    void RejectInvite(std::uint64_t aSessionId) const noexcept;

    void UpdateOffer(Trade::Offer aOffer) const noexcept;
    void ConfirmTrade() const noexcept;
    void CancelTrade() const noexcept;


    [[nodiscard]] std::optional<std::uint32_t> GetOutgoingInviteTarget() const noexcept
    {
        return m_outgoingInviteTarget;
    }

    void DismissOutgoingInvite() noexcept
    {
        m_outgoingInviteTarget.reset();
    }

    [[nodiscard]] std::optional<std::uint64_t> GetPendingInvite() const noexcept
    {
        if (!m_pendingInvite)
            return std::nullopt;

        return m_pendingInvite->SessionId;
    }

    [[nodiscard]] const std::optional<ClientPendingTradeInviteState>& GetPendingInviteState() const noexcept
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
    void OnNotifyTradeApply(const NotifyTradeApply& acTradeApply) noexcept;
    void OnNotifyTradeReconcile(const NotifyTradeReconcile& acTradeReconcile) noexcept;

private:
    void RespondToInvite(std::uint64_t aSessionId, bool aAccepted) const noexcept;
    [[nodiscard]] TradeApplyResultCode ApplyMutationPlan(
        const NotifyTradeApply& acTradeApply) noexcept;
    [[nodiscard]] TradeReconcileResultCode ApplyReconciliationPlan(
        const NotifyTradeReconcile& acTradeReconcile) noexcept;

    void SendApplyResult(
        const NotifyTradeApply& acTradeApply,
        TradeApplyResultCode aResult) const noexcept;
    void SendReconcileResult(
        const NotifyTradeReconcile& acTradeReconcile,
        TradeReconcileResultCode aResult) const noexcept;

    void RememberApplyResult(
        const NotifyTradeApply& acTradeApply,
        TradeApplyResultCode aResult) noexcept;
    void RememberReconcileResult(
        const NotifyTradeReconcile& acTradeReconcile,
        TradeReconcileResultCode aResult) noexcept;

    World& m_world;
    TransportService& m_transport;

    std::optional<std::uint32_t> m_outgoingInviteTarget;
    std::optional<ClientPendingTradeInviteState> m_pendingInvite;
    std::optional<std::uint64_t> m_activeSession;
    std::optional<ClientTradeSessionState> m_sessionState;

    std::unordered_map<Trade::ApplyId, ClientTradeApplyRecord> m_applyJournal;
    std::deque<Trade::ApplyId> m_applyJournalOrder;

    std::unordered_map<Trade::ReconcileId, ClientTradeReconcileRecord> m_reconcileJournal;
    std::deque<Trade::ReconcileId> m_reconcileJournalOrder;

    entt::scoped_connection m_tradeInviteConnection;
    entt::scoped_connection m_tradeStartedConnection;
    entt::scoped_connection m_tradeCancelledConnection;
    entt::scoped_connection m_tradeStateConnection;
    entt::scoped_connection m_tradeApplyConnection;
    entt::scoped_connection m_tradeReconcileConnection;
};
