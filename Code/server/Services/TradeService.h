#pragma once

#include <Events/PacketEvent.h>

struct World;
struct TradeInviteRequest;

class TradeService
{
public:
    TradeService(World& aWorld, entt::dispatcher& aDispatcher) noexcept;

    void OnTradeInviteRequest(const PacketEvent<TradeInviteRequest>& acPacket) noexcept;

private:
    World& m_world;

    uint32_t m_nextSessionId{1};

    entt::scoped_connection m_tradeInviteConnection;
};
