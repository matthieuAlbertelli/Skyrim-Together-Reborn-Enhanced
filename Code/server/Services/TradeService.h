#pragma once

#include <Events/PacketEvent.h>
#include <Trade/TradeSession.h>

#include <cstdint>
#include <unordered_map>

struct World;
struct Player;
struct PlayerLeaveEvent;
struct UpdateEvent;
struct TradeInviteRequest;
struct TradeInviteResponseRequest;
enum class TradeCancelReason : std::uint8_t;

class TradeService
{
public:
    TradeService(World& aWorld, entt::dispatcher& aDispatcher) noexcept;

    void OnTradeInviteRequest(const PacketEvent<TradeInviteRequest>& acPacket) noexcept;
    void OnTradeInviteResponseRequest(const PacketEvent<TradeInviteResponseRequest>& acPacket) noexcept;
    void OnPlayerLeave(const PlayerLeaveEvent& acEvent) noexcept;
    void OnUpdate(const UpdateEvent& acEvent) noexcept;

private:
    [[nodiscard]] Trade::SessionId AllocateSessionId() noexcept;
    [[nodiscard]] Trade::Session* FindSession(Trade::SessionId aSessionId) noexcept;
    [[nodiscard]] const Trade::Session* FindSession(Trade::SessionId aSessionId) const noexcept;
    [[nodiscard]] bool IsPlayerInSession(Trade::PlayerId aPlayerId) const noexcept;

    void SendInvite(Player& aInvitee, const Trade::Session& acSession) const noexcept;
    void SendStarted(const Trade::Session& acSession) const noexcept;
    void SendCancelled(
        const Trade::Session& acSession,
        TradeCancelReason aReason,
        Trade::PlayerId aActorId,
        Player* apIgnoredPlayer = nullptr) const noexcept;
    void RemoveSession(Trade::SessionId aSessionId) noexcept;

    World& m_world;

    std::unordered_map<Trade::SessionId, Trade::Session> m_sessions;
    std::unordered_map<Trade::PlayerId, Trade::SessionId> m_playerSessions;

    Trade::SessionId m_nextSessionId{1};
    Trade::Tick m_nextExpirySweepTick{};

    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_playerLeaveConnection;
    entt::scoped_connection m_tradeInviteConnection;
    entt::scoped_connection m_tradeInviteResponseConnection;
};
