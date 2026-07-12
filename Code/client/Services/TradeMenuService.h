#pragma once

#include <Trade/TradeTypes.h>

#include <cstdint>
#include <optional>

struct ClientTradeSessionState;
struct ImguiService;
struct TradeService;
struct TransportService;
struct World;

class TradeMenuService
{
public:
    TradeMenuService(
        World& aWorld,
        TransportService& aTransport,
        TradeService& aTradeService,
        ImguiService& aImguiService) noexcept;

    ~TradeMenuService() noexcept = default;

    TP_NOCOPYMOVE(TradeMenuService);

    enum class Category : std::uint8_t
    {
        All,
        Weapons,
        Armor,
        Consumables,
        Books,
        Misc
    };

private:
    void OnDraw() noexcept;
    void DrawInvite() noexcept;
    void DrawTrade(
        const ClientTradeSessionState& acState) noexcept;
    void DrawInventoryPane(
        const ClientTradeSessionState& acState,
        const Trade::Participant& acLocalParticipant) noexcept;
    void DrawOffersPane(
        const ClientTradeSessionState& acState,
        const Trade::Participant& acLocalParticipant,
        const Trade::Participant& acRemoteParticipant) noexcept;
    void DrawOfferList(
        const char* apTitle,
        const Trade::Offer& acOffer,
        bool aAllowRemoval) noexcept;
    void DrawFooter(
        const ClientTradeSessionState& acState,
        const Trade::Participant& acLocalParticipant,
        const Trade::Participant& acRemoteParticipant) noexcept;

    void SetOfferedQuantity(
        Trade::ItemId aItemId,
        std::uint32_t aQuantity,
        const Trade::Offer& acCurrentOffer) noexcept;

    World& m_world;
    TransportService& m_transport;
    TradeService& m_tradeService;

    bool m_visible{};
    Category m_category{Category::All};

    std::optional<Trade::SessionId> m_lastObservedPendingInvite;
    std::optional<Trade::SessionId> m_lastObservedActiveSession;
    std::optional<Trade::SessionId> m_dismissedTerminalSession;
    std::optional<Trade::SessionId> m_selectionSession;
    std::optional<Trade::ItemId> m_selectedItem;
    int m_selectedQuantity{1};

    entt::scoped_connection m_drawConnection;
};
