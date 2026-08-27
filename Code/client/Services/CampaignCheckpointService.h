#pragma once

#include <CampaignCheckpointClient.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

struct CampaignCheckpointSaveRequest;
struct NotifyCampaignCheckpointState;
struct DisconnectedEvent;
class CampaignService;
struct CampaignRuntimeGateService;
struct OverlayService;
struct TransportService;
struct UpdateEvent;
namespace STRE::Campaign
{
enum class CampaignSaveDecision : std::uint8_t;
enum class CampaignSaveOrigin : std::uint8_t;
}

class CampaignCheckpointService final
{
public:
    CampaignCheckpointService(
        entt::dispatcher& aDispatcher,
        TransportService& aTransport,
        CampaignService& aCampaignService,
        CampaignRuntimeGateService& aRuntimeGate,
        OverlayService& aOverlay) noexcept;
    ~CampaignCheckpointService() noexcept;

    static CampaignCheckpointService* TryGet() noexcept;

    [[nodiscard]] STRE::Campaign::CampaignSaveDecision
    HandleNativeSaveAttempt(
        STRE::Campaign::CampaignSaveOrigin aOrigin) noexcept;

private:
    void OnCheckpointRequest(
        const CampaignCheckpointSaveRequest& acRequest) noexcept;
    void OnUpdate(const UpdateEvent&) noexcept;
    void OnCheckpointState(
        const NotifyCampaignCheckpointState& acState) noexcept;
    void OnDisconnected(const DisconnectedEvent&) noexcept;
    void PublishPolicyState() noexcept;
    void Notify(std::string_view acTranslationKey) noexcept;
    void SendResult(
        const STRE::Campaign::CampaignCheckpointClientResult& acResult) noexcept;
    void SendFailureForAction(
        const STRE::Campaign::CampaignCheckpointClientAction& acAction) noexcept;

    TransportService& m_transport;
    CampaignService& m_campaignService;
    CampaignRuntimeGateService& m_runtimeGate;
    OverlayService& m_overlay;
    std::unique_ptr<STRE::Campaign::CampaignIdentityStore> m_store;
    std::unique_ptr<STRE::Campaign::CampaignCheckpointClient> m_client;
    bool m_intentPending{};
    std::string m_checkpointStateToken;
    std::string m_lastPolicyJson;

    entt::scoped_connection m_requestConnection;
    entt::scoped_connection m_stateConnection;
    entt::scoped_connection m_disconnectedConnection;
    entt::scoped_connection m_updateConnection;
};
