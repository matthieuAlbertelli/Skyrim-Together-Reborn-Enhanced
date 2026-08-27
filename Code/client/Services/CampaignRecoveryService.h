#pragma once

#include <CampaignRecoveryState.h>
#include <Structs/NativeSaveBundle.h>

#include <optional>
#include <string>

class CampaignNativeLoadService;
class CampaignService;
struct CampaignRuntimeGateService;
struct CampaignRecoveryComplete;
struct CampaignRecoveryLoadRequest;
struct CampaignRecoverySnapshot;
struct DisconnectedEvent;
struct NotifyCampaignSnapshot;
struct TransportService;
struct UpdateEvent;

class CampaignRecoveryService final
{
public:
    CampaignRecoveryService(
        entt::dispatcher& aDispatcher,
        TransportService& aTransport,
        CampaignService& aCampaignService,
        CampaignRuntimeGateService& aGate,
        CampaignNativeLoadService& aNativeLoad) noexcept;

    TP_NOCOPYMOVE(CampaignRecoveryService);

    [[nodiscard]] STRE::Campaign::CampaignClientRecoveryStage
    GetStage() const noexcept
    {
        return m_state.GetStage();
    }

private:
    void OnCampaignSnapshot(
        const NotifyCampaignSnapshot& acNotification) noexcept;
    void OnLoadRequest(
        const CampaignRecoveryLoadRequest& acRequest) noexcept;
    void OnRecoverySnapshot(
        const CampaignRecoverySnapshot& acSnapshot) noexcept;
    void OnRecoveryComplete(
        const CampaignRecoveryComplete& acComplete) noexcept;
    void OnDisconnected(const DisconnectedEvent&) noexcept;
    void OnUpdate(const UpdateEvent&) noexcept;
    void SendLoaded(bool aSucceeded) noexcept;
    void SendSnapshotApplied() noexcept;

    TransportService& m_transport;
    CampaignService& m_campaignService;
    CampaignRuntimeGateService& m_gate;
    CampaignNativeLoadService& m_nativeLoad;
    STRE::Campaign::CampaignRecoveryState m_state;
    std::optional<STRE::Campaign::NativeSaveBundleArtifact>
        m_expectedArtifact;
    std::string m_protectedCampaign;
    bool m_protectOnDisconnect{};

    entt::scoped_connection m_campaignSnapshotConnection;
    entt::scoped_connection m_loadRequestConnection;
    entt::scoped_connection m_recoverySnapshotConnection;
    entt::scoped_connection m_recoveryCompleteConnection;
    entt::scoped_connection m_disconnectedConnection;
    entt::scoped_connection m_updateConnection;
};
