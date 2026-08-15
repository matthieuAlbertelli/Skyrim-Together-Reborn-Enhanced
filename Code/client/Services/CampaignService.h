#pragma once

#include <CampaignIdentityStore.h>
#include <Structs/Campaign.h>

#include <memory>
#include <optional>
#include <string>

struct CampaignCommandResponse;
struct DisconnectedEvent;
struct NotifyCampaignSnapshot;
struct TransportService;

struct CampaignClientAdmission
{
    std::string CampaignId;
    std::string CampaignSlotId;
    std::string CharacterBindingId;
};

struct CampaignClientCommandOutcome
{
    CampaignProtocolOperation Operation{CampaignProtocolOperation::Create};
    CampaignProtocolResult Result{CampaignProtocolResult::InvalidRequest};
    std::string MutationId;
    std::string CampaignId;
    std::uint64_t StateVersion{};
};

class CampaignService final
{
public:
    CampaignService(
        entt::dispatcher& aDispatcher,
        TransportService& aTransport) noexcept;

    [[nodiscard]] std::optional<TiltedPhoques::String>
    GetDurablePlayerIdForAuthentication() const noexcept;
    [[nodiscard]] const std::optional<CampaignSnapshotData>&
    GetLatestSnapshot() const noexcept { return m_latestSnapshot; }
    [[nodiscard]] const std::optional<CampaignClientAdmission>&
    GetAdmission() const noexcept { return m_admission; }
    [[nodiscard]] const std::optional<CampaignClientCommandOutcome>&
    GetLastCommandOutcome() const noexcept { return m_lastCommandOutcome; }
    [[nodiscard]] const std::string& GetStorageError() const noexcept
    {
        return m_storageError;
    }

    [[nodiscard]] std::string CreateCampaign(
        const std::string& acMutationId = {}) noexcept;
    [[nodiscard]] std::string JoinCampaign(
        const std::string& acCampaignId,
        std::uint64_t aExpectedRevision,
        const std::string& acMutationId = {}) noexcept;
    [[nodiscard]] bool ResumeCampaign(
        const std::string& acCampaignId) noexcept;
    [[nodiscard]] std::string StartCampaign(
        const std::string& acCampaignId,
        std::uint64_t aExpectedRevision,
        const std::string& acMutationId = {}) noexcept;
    [[nodiscard]] std::string SetReady(
        const std::string& acCampaignId,
        std::uint64_t aExpectedRevision,
        bool aReady,
        const std::string& acMutationId = {}) noexcept;
    [[nodiscard]] std::string LeaveCampaign(
        const std::string& acCampaignId,
        std::uint64_t aExpectedRevision,
        const std::string& acMutationId = {}) noexcept;

private:
    [[nodiscard]] std::string GenerateMutationId() const;
    void OnCommandResponse(
        const CampaignCommandResponse& acResponse) noexcept;
    void OnSnapshot(
        const NotifyCampaignSnapshot& acNotification) noexcept;
    void OnDisconnected(const DisconnectedEvent&) noexcept;

    TransportService& m_transport;
    std::unique_ptr<STRE::Campaign::CampaignIdentityStore> m_store;
    std::optional<std::string> m_playerId;
    std::string m_storageError;
    bool m_bindingCacheAvailable{};
    std::optional<CampaignClientAdmission> m_admission;
    std::optional<CampaignSnapshotData> m_latestSnapshot;
    std::optional<CampaignClientCommandOutcome> m_lastCommandOutcome;

    entt::scoped_connection m_responseConnection;
    entt::scoped_connection m_snapshotConnection;
    entt::scoped_connection m_disconnectedConnection;
};
