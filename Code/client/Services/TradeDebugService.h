#pragma once

#include <Trade/TradeTypes.h>

#include <optional>
#include <unordered_map>

struct ImguiService;
struct TradeService;
struct TransportService;
struct World;
struct ClientTradeSessionState;

class TradeDebugService
{
public:
    TradeDebugService(
        World& aWorld,
        TransportService& aTransport,
        TradeService& aTradeService,
        ImguiService& aImguiService) noexcept;

    ~TradeDebugService() noexcept = default;

    TP_NOCOPYMOVE(TradeDebugService);

private:
    void OnDraw() noexcept;
    void DrawInviteSection() noexcept;
    void DrawSession(
        const ClientTradeSessionState& acState) noexcept;
    void DrawOffers(
        const ClientTradeSessionState& acState,
        const Trade::Participant& acLocalParticipant,
        const Trade::Participant& acRemoteParticipant) noexcept;
    void DrawInventory(
        const ClientTradeSessionState& acState,
        const Trade::Participant& acLocalParticipant) noexcept;

    void SetOfferedQuantity(
        Trade::ItemId aItemId,
        std::uint32_t aQuantity,
        const Trade::Offer& acCurrentOffer) noexcept;

    World& m_world;
    TransportService& m_transport;
    TradeService& m_tradeService;

    bool m_visible{};
    int m_inviteTargetPlayerId{};

    std::optional<Trade::SessionId> m_lastObservedPendingInvite;
    std::optional<Trade::SessionId> m_lastObservedActiveSession;
    std::optional<Trade::SessionId> m_draftSession;
    Trade::Revision m_draftRevision{};

    std::unordered_map<Trade::ItemId, int> m_quantityDrafts;

    entt::scoped_connection m_drawConnection;
};
