#pragma once

#include <CampaignBootstrapState.h>

#include <cstdint>
#include <string>

struct CampaignCommandResponse;
class CampaignService;
struct ConnectedEvent;
struct ConnectionErrorEvent;
struct DisconnectedEvent;
struct TransportService;
struct UiSurfaceService;
struct UpdateEvent;

class CampaignBootstrapService final
{
public:
    CampaignBootstrapService(
        entt::dispatcher& aDispatcher,
        TransportService& aTransport,
        CampaignService& aCampaignService,
        UiSurfaceService& aUiSurfaceService) noexcept;

    TP_NOCOPYMOVE(CampaignBootstrapService);

    void BeginFreshGame() noexcept;
    void ChooseSolo() noexcept;
    void ShowCreate() noexcept;
    void ShowJoin() noexcept;
    void Create(
        std::string aAddress,
        std::string aPassword,
        std::string aDisplayName) noexcept;
    void Join(
        std::string aAddress,
        std::string aPassword,
        std::string aJoinCode,
        std::string aDisplayName) noexcept;
    void Start() noexcept;
    void Back() noexcept;

    [[nodiscard]] STRE::Campaign::CampaignBootstrapPhase GetPhase() const noexcept
    {
        return m_state.GetPhase();
    }

private:
    void OnUpdate(const UpdateEvent&) noexcept;
    void OnConnected(const ConnectedEvent&) noexcept;
    void OnDisconnected(const DisconnectedEvent&) noexcept;
    void OnConnectionError(const ConnectionErrorEvent&) noexcept;

    [[nodiscard]] bool Connect(
        const std::string& acAddress,
        const std::string& acPassword) noexcept;
    void SendCreate() noexcept;
    void SendJoin() noexcept;
    void ProcessCommandOutcome() noexcept;
    void ObserveCanonicalState() noexcept;
    void AuthorizeCharacterCreation() noexcept;
    void PublishState(bool aForce = false) noexcept;
    void SetError(std::string aErrorCode) noexcept;

    entt::dispatcher& m_dispatcher;
    TransportService& m_transport;
    CampaignService& m_campaignService;
    UiSurfaceService& m_uiSurfaceService;
    STRE::Campaign::CampaignBootstrapState m_state;
    std::string m_joinCode;
    std::string m_displayName;
    std::string m_pendingMutation;
    std::string m_lastOutcomeToken;
    std::string m_errorCode;
    std::string m_lastStateJson;
    bool m_waitingForResume{};

    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_connectedConnection;
    entt::scoped_connection m_disconnectedConnection;
    entt::scoped_connection m_connectionErrorConnection;
};
