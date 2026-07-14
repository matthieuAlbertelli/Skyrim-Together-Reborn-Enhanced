#pragma once

#include <Trade/TradeTypes.h>
#include <Services/UiSurfaceService.h>

#include <cstdint>
#include <optional>
#include <string>

struct ClientTradeSessionState;
struct TradeService;
struct TransportService;
struct UpdateEvent;
struct World;

class TradeMenuService
{
public:
    TradeMenuService(
        World& aWorld,
        TransportService& aTransport,
        TradeService& aTradeService,
        UiSurfaceService& aUiSurfaceService,
        entt::dispatcher& aDispatcher) noexcept;

    ~TradeMenuService() noexcept = default;

    TP_NOCOPYMOVE(TradeMenuService);

    void AcceptInvite(Trade::SessionId aSessionId) noexcept;
    void RejectInvite(Trade::SessionId aSessionId) noexcept;
    void SetOfferedQuantity(
        Trade::ItemId aItemId,
        std::uint32_t aQuantity) noexcept;
    void ConfirmTrade() noexcept;
    void CancelTrade() noexcept;
    void DismissTerminalState() noexcept;
    void DismissOutgoingInvite() noexcept;

    [[nodiscard]] bool IsVisible() const noexcept
    {
        return m_visible;
    }

private:
    void OnUpdate(const UpdateEvent& acEvent) noexcept;
    void SetVisible(bool aVisible) noexcept;
    void PushState(bool aForce = false) noexcept;

    [[nodiscard]] std::string BuildStateJson() const;
    [[nodiscard]] const Trade::Participant* GetLocalParticipant(
        const ClientTradeSessionState& acState) const noexcept;
    [[nodiscard]] const Trade::Participant* GetRemoteParticipant(
        const ClientTradeSessionState& acState) const noexcept;

    World& m_world;
    TransportService& m_transport;
    TradeService& m_tradeService;
    UiSurfaceService& m_uiSurfaceService;

    bool m_visible{};
    UiSurface m_previousSurface{UiSurface::None};
    double m_refreshAccumulator{};

    std::optional<Trade::SessionId> m_lastPendingInvite;
    std::optional<Trade::SessionId> m_lastActiveSession;
    std::optional<std::uint32_t> m_lastOutgoingInviteTarget;
    std::optional<Trade::SessionId> m_dismissedTerminalSession;

    std::string m_lastStateJson;

    entt::scoped_connection m_updateConnection;
};
