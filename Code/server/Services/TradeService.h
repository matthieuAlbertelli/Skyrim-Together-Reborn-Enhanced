#pragma once

#include <Events/PacketEvent.h>
#include <Trade/TradeApplication.h>
#include <Trade/TradeInventory.h>
#include <Trade/TradeSession.h>

#include <cstdint>
#include <unordered_map>

struct World;
struct Player;
struct PlayerLeaveEvent;
struct UpdateEvent;
struct TradeInviteRequest;
struct TradeInviteResponseRequest;
struct TradeOfferUpdateRequest;
struct TradeConfirmRequest;
struct TradeCancelRequest;
struct TradeApplyResultRequest;
enum class TradeCancelReason : std::uint8_t;

class TradeService
{
public:
    TradeService(World& aWorld, entt::dispatcher& aDispatcher) noexcept;

    void OnTradeInviteRequest(const PacketEvent<TradeInviteRequest>& acPacket) noexcept;
    void OnTradeInviteResponseRequest(const PacketEvent<TradeInviteResponseRequest>& acPacket) noexcept;
    void OnTradeOfferUpdateRequest(const PacketEvent<TradeOfferUpdateRequest>& acPacket) noexcept;
    void OnTradeConfirmRequest(const PacketEvent<TradeConfirmRequest>& acPacket) noexcept;
    void OnTradeCancelRequest(const PacketEvent<TradeCancelRequest>& acPacket) noexcept;
    void OnTradeApplyResultRequest(const PacketEvent<TradeApplyResultRequest>& acPacket) noexcept;
    void OnPlayerLeave(const PlayerLeaveEvent& acEvent) noexcept;
    void OnUpdate(const UpdateEvent& acEvent) noexcept;

private:
    [[nodiscard]] Trade::SessionId AllocateSessionId() noexcept;
    [[nodiscard]] Trade::ApplyId AllocateApplyId() noexcept;
    [[nodiscard]] Trade::Session* FindSession(Trade::SessionId aSessionId) noexcept;
    [[nodiscard]] const Trade::Session* FindSession(Trade::SessionId aSessionId) const noexcept;
    [[nodiscard]] bool IsPlayerInSession(Trade::PlayerId aPlayerId) const noexcept;
    [[nodiscard]] bool IsIndexedParticipant(
        Trade::PlayerId aPlayerId,
        Trade::SessionId aSessionId) const noexcept;

    [[nodiscard]] bool TryBuildInventorySnapshot(
        Trade::PlayerId aPlayerId,
        Trade::InventorySnapshot& aSnapshot) const noexcept;
    [[nodiscard]] Trade::MutationPlanResult ValidateAndBuildMutationPlan(
        const Trade::Session& acSession) const noexcept;
    [[nodiscard]] bool CommitMutationPlan(
        const Trade::MutationPlan& acPlan) noexcept;

    void BeginApplication(
        Trade::Session& aSession,
        Trade::MutationPlan aPlan,
        Trade::Tick aCurrentTick) noexcept;
    void SendInvite(Player& aInvitee, const Trade::Session& acSession) const noexcept;
    void SendStarted(const Trade::Session& acSession) const noexcept;
    void SendState(const Trade::Session& acSession, Player* apRecipient = nullptr) const noexcept;
    void SendApply(
        const Trade::Session& acSession,
        const Trade::Application& acApplication) const noexcept;
    void SendCancelled(
        const Trade::Session& acSession,
        TradeCancelReason aReason,
        Trade::PlayerId aActorId,
        Player* apIgnoredPlayer = nullptr) const noexcept;

    void ReleaseSessionParticipants(Trade::SessionId aSessionId) noexcept;
    void ScheduleTerminalCleanup(
        Trade::SessionId aSessionId,
        Trade::Tick aCurrentTick) noexcept;
    void RemoveSession(Trade::SessionId aSessionId) noexcept;

    World& m_world;

    std::unordered_map<Trade::SessionId, Trade::Session> m_sessions;
    std::unordered_map<Trade::PlayerId, Trade::SessionId> m_playerSessions;
    std::unordered_map<Trade::SessionId, Trade::Application> m_applications;
    std::unordered_map<Trade::SessionId, Trade::Tick> m_terminalCleanupTicks;

    Trade::SessionId m_nextSessionId{1};
    Trade::ApplyId m_nextApplyId{1};
    Trade::Tick m_nextExpirySweepTick{};

    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_playerLeaveConnection;
    entt::scoped_connection m_tradeInviteConnection;
    entt::scoped_connection m_tradeInviteResponseConnection;
    entt::scoped_connection m_tradeOfferUpdateConnection;
    entt::scoped_connection m_tradeConfirmConnection;
    entt::scoped_connection m_tradeCancelConnection;
    entt::scoped_connection m_tradeApplyResultConnection;
};
