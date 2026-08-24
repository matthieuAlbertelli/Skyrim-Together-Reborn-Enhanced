#pragma once

#include <CampaignCheckpointClient.h>

#include <memory>

struct CampaignCheckpointSaveRequest;
class CampaignService;
struct TransportService;
struct UpdateEvent;

class CampaignCheckpointService final
{
public:
    CampaignCheckpointService(
        entt::dispatcher& aDispatcher,
        TransportService& aTransport,
        CampaignService& aCampaignService) noexcept;

private:
    void OnCheckpointRequest(
        const CampaignCheckpointSaveRequest& acRequest) noexcept;
    void OnUpdate(const UpdateEvent&) noexcept;
    void SendResult(
        const STRE::Campaign::CampaignCheckpointClientResult& acResult) noexcept;
    void SendFailureForAction(
        const STRE::Campaign::CampaignCheckpointClientAction& acAction) noexcept;

    TransportService& m_transport;
    CampaignService& m_campaignService;
    std::unique_ptr<STRE::Campaign::CampaignIdentityStore> m_store;
    std::unique_ptr<STRE::Campaign::CampaignCheckpointClient> m_client;

    entt::scoped_connection m_requestConnection;
    entt::scoped_connection m_updateConnection;
};
