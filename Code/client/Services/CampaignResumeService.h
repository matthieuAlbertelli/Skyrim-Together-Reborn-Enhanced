#pragma once

#include <CampaignResumeState.h>
#include <CampaignRecoveryUiState.h>
#include <CampaignContinueLoad.h>
#include <CampaignIdentityStore.h>
#include <CampaignLoadPolicy.h>

#include <optional>
#include <string>
#include <vector>

class CampaignService;
class CampaignRecoveryService;
struct CampaignMainMenuEnteredEvent;
struct DisconnectedEvent;
struct TransportService;
struct UiSurfaceService;
struct UpdateEvent;

class CampaignResumeService final
{
public:
    CampaignResumeService(
        entt::dispatcher& aDispatcher,
        TransportService& aTransport,
        CampaignService& aCampaignService,
        UiSurfaceService& aUiSurfaceService,
        CampaignRecoveryService& aRecoveryService) noexcept;

    TP_NOCOPYMOVE(CampaignResumeService);

    ~CampaignResumeService() noexcept;

    static CampaignResumeService* TryGet() noexcept;

    void Refresh() noexcept;
    void Retry() noexcept;
    void Select(std::string aToken) noexcept;
    void StayAndRecover() noexcept;
    void ReturnToMainMenu() noexcept;
    [[nodiscard]] STRE::Campaign::CampaignLoadTarget InspectNativeLoadTarget(
        std::string_view acSaveName) noexcept;
    [[nodiscard]] bool IsManagedLoadActive() const noexcept
    {
        return !m_loadedSaveIdentity.empty();
    }
    [[nodiscard]] bool OnNativeLoadEnter(const char* apSaveName) noexcept;
    void OnNativeLoadReturn(bool aManaged, bool aSucceeded) noexcept;
    void OnPostLoad() noexcept;
    [[nodiscard]] bool BeginContinueLoadPending(
        std::string_view acNativeSaveIdentity) noexcept;
    [[nodiscard]] bool ObserveContinueNativeResult(bool aAccepted) noexcept;
    [[nodiscard]] bool CommitContinueMainMenuClosed() noexcept;
    void CancelContinueLoadPending() noexcept;

private:
    struct RosterMemberProjection
    {
        bool Present{};
        bool Local{};
    };

    void LoadCandidates(bool aForce) noexcept;
    void OnUpdate(const UpdateEvent&) noexcept;
    void OnDisconnected(const DisconnectedEvent&) noexcept;
    void OnMainMenuEntered(
        const CampaignMainMenuEnteredEvent&) noexcept;
    void ProcessCommandOutcome() noexcept;
    void ObserveCanonicalState() noexcept;
    void DispatchPendingMainMenuRequest() noexcept;
    void PublishState(bool aForce = false) noexcept;
    [[nodiscard]] bool PrepareLoadedSaveIdentity(
        std::string_view acNativeSaveIdentity) noexcept;
    void ClearLoadedSaveIdentity() noexcept;
    void OpenMandatorySurface() noexcept;
    [[nodiscard]] STRE::Campaign::CampaignResumePhase
    GetProjectedPhase() const noexcept;

    TransportService& m_transport;
    CampaignService& m_campaignService;
    UiSurfaceService& m_uiSurfaceService;
    CampaignRecoveryService& m_recoveryService;
    STRE::Campaign::CampaignResumeState m_state;
    STRE::Campaign::CampaignRecoveryUiState m_recoveryUiState;
    std::string m_lastOutcomeToken;
    std::string m_lastStateJson;
    std::optional<STRE::Campaign::CampaignSaveMarker> m_loadedMarker;
    std::string m_loadedSaveIdentity;
    std::string m_loadError;
    STRE::Campaign::CampaignContinueResumeTransition m_continueTransition;
    std::vector<RosterMemberProjection> m_roster;
    bool m_resumeRequired{};
    bool m_recoveryObserved{};
    bool m_openSurfaceWhenReady{};

    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_disconnectedConnection;
    entt::scoped_connection m_mainMenuEnteredConnection;
};
