#pragma once

#include <CampaignIdentityStore.h>
#include <CampaignNativeLoadState.h>
#include <Structs/NativeSaveBundle.h>

#include <chrono>
#include <memory>
#include <optional>
#include <string_view>

class CampaignService;
struct CampaignRuntimeGateService;
struct TransportService;
struct UpdateEvent;

// Proves one exact local native load. Collective recovery owns policy and
// keeps the runtime gate locked until the server completes both barriers.
class CampaignNativeLoadService final
{
public:
    CampaignNativeLoadService(
        entt::dispatcher& aDispatcher,
        CampaignService& aCampaignService,
        CampaignRuntimeGateService& aGate,
        TransportService& aTransport) noexcept;
    ~CampaignNativeLoadService() noexcept;

    TP_NOCOPYMOVE(CampaignNativeLoadService);

    static CampaignNativeLoadService* TryGet() noexcept;

    [[nodiscard]] bool RequestForValidation(
        std::string_view acNativeSaveIdentity) noexcept;
    [[nodiscard]] bool ReleaseForValidation() noexcept;
    [[nodiscard]] bool RequestForRecovery(
        std::string_view acNativeSaveIdentity,
        const STRE::Campaign::NativeSaveBundleArtifact&
            acExpectedArtifact) noexcept;
    [[nodiscard]] bool ResetForRecoveryRetry() noexcept;
    [[nodiscard]] bool ReleaseForRecovery() noexcept;

    [[nodiscard]] bool OnNativeLoadEnter(
        const char* apNativeSaveName) noexcept;
    [[nodiscard]] bool HasAuthoritativeAdmission() const noexcept;
    [[nodiscard]] bool IsCampaignRuntimeSensitive() const noexcept;
    [[nodiscard]] bool IsManagedLoadActive() const noexcept
    {
        return m_request.IsActive();
    }
    void OnNativeLoadReturn(bool aManaged, bool aSucceeded) noexcept;
    void OnPostLoad() noexcept;
    void OnPostLoadSafetyObserved(
        bool aGuardMenuOpen,
        bool aGamePaused) noexcept;
    void OnGuardMenuPostDisplay(bool aGamePaused) noexcept;
    void OnTransportUpdate(bool aConnected) noexcept;
    void OnGateArmFailure() noexcept;

    [[nodiscard]] const STRE::Campaign::CampaignNativeLoadSnapshot&
    GetStatus() const noexcept { return m_request.Snapshot(); }

private:
    void OnUpdate(const UpdateEvent&) noexcept;
    void BeginNativeInvocation() noexcept;
    void Fail(STRE::Campaign::CampaignNativeLoadFailure aFailure) noexcept;
    void SetDeadline(std::chrono::seconds aDuration) noexcept;
    void Log(
        const char* apEvent,
        const char* apDetail = "") const noexcept;

    CampaignService& m_campaignService;
    CampaignRuntimeGateService& m_gate;
    TransportService& m_transport;
    std::unique_ptr<STRE::Campaign::CampaignIdentityStore> m_store;
    STRE::Campaign::CampaignNativeLoadRequest m_request;
    std::optional<STRE::Campaign::NativeSaveBundleArtifact>
        m_expectedArtifact;
    std::chrono::steady_clock::time_point m_deadline{};
    bool m_deadlineActive{};
    bool m_completedLogged{};

    entt::scoped_connection m_updateConnection;
};
