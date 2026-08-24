#pragma once

#include <CampaignClientAdmissionState.h>
#include <CampaignIdentityStore.h>
#include <CampaignHelgenStateCache.h>
#include <Structs/Campaign.h>

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct CampaignCommandResponse;
struct ConnectedEvent;
struct DisconnectedEvent;
struct NotifyCampaignSnapshot;
struct NotifyCampaignLobbyState;
struct NotifyCampaignHelgenState;
struct TransportService;

struct CampaignClientCommandOutcome
{
    CampaignProtocolOperation Operation{CampaignProtocolOperation::Create};
    CampaignProtocolResult Result{CampaignProtocolResult::InvalidRequest};
    std::string MutationId;
    std::string CampaignId;
    std::uint64_t StateVersion{};
    std::string JoinCode;
};

struct CampaignClientLobbyMember
{
    std::string Name;
    bool Present{};
};

struct CampaignClientLobbyState
{
    std::string JoinCode;
    std::uint64_t StateVersion{};
    std::vector<CampaignClientLobbyMember> Members;
    bool CanStart{};
};

class CampaignService final
{
public:
    CampaignService(entt::dispatcher& aDispatcher, TransportService& aTransport) noexcept;

    [[nodiscard]] std::optional<TiltedPhoques::String>
    GetDurablePlayerIdForAuthentication() const noexcept;
    [[nodiscard]] const std::optional<CampaignSnapshotData>&
    GetLatestSnapshot() const noexcept { return m_latestSnapshot; }
    [[nodiscard]] std::optional<STRE::Campaign::CampaignClientAdmission>
    GetAdmission() const noexcept { return m_admissionState.GetAdmission(); }
    [[nodiscard]] const std::optional<CampaignClientCommandOutcome>&
    GetLastCommandOutcome() const noexcept { return m_lastCommandOutcome; }
    [[nodiscard]] const std::optional<CampaignClientLobbyState>&
    GetLobbyState() const noexcept { return m_lobbyState; }
    [[nodiscard]] const std::string& GetStorageError() const noexcept
    {
        return m_storageError;
    }

    [[nodiscard]] std::string CreateCampaign(
        const std::string& acDisplayName,
        const std::string& acMutationId = {}) noexcept;
    [[nodiscard]] std::string JoinCampaign(
        const std::string& acCampaignId,
        std::uint64_t aExpectedRevision,
        const std::string& acMutationId = {}) noexcept;
    [[nodiscard]] std::string JoinCampaignByCode(
        const std::string& acJoinCode,
        const std::string& acDisplayName,
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

    [[nodiscard]] bool SignalHelgenInvestigationReady() noexcept;
    [[nodiscard]] bool IsHelgenInvestigationStartAuthorized() const noexcept
    {
        return m_helgenState.IsInvestigationStartAuthorized();
    }
    [[nodiscard]] bool AreAllRequiredPlayersOutsideHelgen() const noexcept
    {
        return m_helgenState.AreAllRequiredPlayersOutside();
    }

private:
    [[nodiscard]] std::string GenerateMutationId() const;
    void OnCommandResponse(
        const CampaignCommandResponse& acResponse) noexcept;
    void OnSnapshot(
        const NotifyCampaignSnapshot& acNotification) noexcept;
    void OnLobbyState(
        const NotifyCampaignLobbyState& acNotification) noexcept;
    void OnHelgenState(const NotifyCampaignHelgenState& acNotification) noexcept;
    void OnConnected(const ConnectedEvent&) noexcept;
    void OnDisconnected(const DisconnectedEvent&) noexcept;

    TransportService& m_transport;
    std::unique_ptr<STRE::Campaign::CampaignIdentityStore> m_store;
    std::optional<std::string> m_playerId;
    std::string m_storageError;
    bool m_bindingCacheAvailable{};
    STRE::Campaign::CampaignClientAdmissionState m_admissionState;
    std::optional<CampaignSnapshotData> m_latestSnapshot;
    std::optional<CampaignClientCommandOutcome> m_lastCommandOutcome;
    std::optional<CampaignClientLobbyState> m_lobbyState;
    CampaignHelgenStateCache m_helgenState;
    std::atomic_bool m_helgenReadinessRejectionLogged{};

    entt::scoped_connection m_responseConnection;
    entt::scoped_connection m_snapshotConnection;
    entt::scoped_connection m_lobbyStateConnection;
    entt::scoped_connection m_helgenStateConnection;
    entt::scoped_connection m_connectedConnection;
    entt::scoped_connection m_disconnectedConnection;
};
