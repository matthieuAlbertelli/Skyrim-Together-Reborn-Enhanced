#include <TiltedOnlinePCH.h>

#include <Services/CampaignNativeLoadService.h>

#include <CampaignNativeLoad.h>
#include <CampaignNativeSave.h>
#include <Events/UpdateEvent.h>
#include <Services/CampaignRuntimeGateService.h>
#include <Services/CampaignService.h>
#include <Services/TransportService.h>
#include <Structs/Campaign.h>

using namespace STRE::Campaign;

namespace
{
CampaignNativeLoadService* s_pCampaignNativeLoadService{};

bool NativeSaveIsActive(CampaignNativeSaveLifecycleState aState) noexcept
{
    return aState == CampaignNativeSaveLifecycleState::Requested ||
        aState == CampaignNativeSaveLifecycleState::Processing ||
        aState == CampaignNativeSaveLifecycleState::AwaitingCompletion;
}

bool IsValidIdentity(std::string_view acIdentity)
{
    TiltedPhoques::String identity;
    identity.assign(acIdentity.data(), acIdentity.size());
    return IsValidCampaignNativeSaveIdentity(identity);
}
}

CampaignNativeLoadService::CampaignNativeLoadService(
    entt::dispatcher& aDispatcher,
    CampaignService& aCampaignService,
    CampaignRuntimeGateService& aGate,
    TransportService& aTransport) noexcept
    : m_campaignService(aCampaignService)
    , m_gate(aGate)
    , m_transport(aTransport)
    , m_updateConnection(
          aDispatcher.sink<UpdateEvent>()
              .connect<&CampaignNativeLoadService::OnUpdate>(this))
{
    s_pCampaignNativeLoadService = this;
    auto directory = CampaignIdentityStore::ResolveDefaultDirectory();
    if (!directory)
    {
        spdlog::error(
            "[STRE][CampaignNativeLoad] INITIALIZATION_FAILED "
            "reason=artifact-store-unavailable");
        return;
    }
    m_store = std::make_unique<CampaignIdentityStore>(
        std::move(directory.Value));
    Log("INITIALIZED", "trigger=campaign-recovery-protocol");
}

CampaignNativeLoadService::~CampaignNativeLoadService() noexcept
{
    if (s_pCampaignNativeLoadService == this)
        s_pCampaignNativeLoadService = nullptr;
}

CampaignNativeLoadService* CampaignNativeLoadService::TryGet() noexcept
{
    return s_pCampaignNativeLoadService;
}

bool CampaignNativeLoadService::RequestForValidation(
    std::string_view acNativeSaveIdentity) noexcept
{
    try
    {
        std::string identity(
            acNativeSaveIdentity.data(), acNativeSaveIdentity.size());
        if (!m_request.Request(identity))
        {
            spdlog::warn(
                "[STRE][CampaignNativeLoad] REQUEST_REJECTED "
                "reason={} identity={} thread_id={}",
                IsValidIdentity(identity)
                    ? "request-not-idle"
                    : "invalid-identity",
                identity,
                GetCurrentThreadId());
            return false;
        }

        Log("REQUESTED", fmt::format(
            "identity={} thread_id={}",
            identity,
            GetCurrentThreadId()).c_str());

        const auto admission = m_campaignService.GetAdmission();
        if (!admission)
        {
            Fail(CampaignNativeLoadFailure::CampaignAdmissionUnavailable);
            return false;
        }
        if (!m_store)
        {
            Fail(CampaignNativeLoadFailure::ArtifactUnavailable);
            return false;
        }

        const std::string checkpointId = identity.substr(
            kCampaignNativeSaveIdentityPrefix.size());
        if (!CampaignIdentityStore::IsValidCacheId(admission->CampaignId) ||
            !CampaignIdentityStore::IsValidCacheId(checkpointId))
        {
            Fail(CampaignNativeLoadFailure::InvalidIdentity);
            return false;
        }

        auto cached = m_store->LoadCheckpointArtifact(
            admission->CampaignId, checkpointId);
        if (!cached || !cached.Value)
        {
            Fail(CampaignNativeLoadFailure::ArtifactUnavailable);
            return false;
        }
        const auto parsed = ParseNativeSaveBundleArtifact(
            identity,
            cached.Value->Fingerprint,
            cached.Value->Metadata);
        if (!parsed || parsed.Value != *cached.Value ||
            cached.Value->Bundle.LogicalIdentity != identity)
        {
            Fail(CampaignNativeLoadFailure::ArtifactInvalid);
            return false;
        }

        const CampaignNativeSaveLifecycleSnapshot nativeSaveStatus =
            CampaignNativeSave::GetStatus();
        if (NativeSaveIsActive(nativeSaveStatus.State))
        {
            Fail(CampaignNativeLoadFailure::NativeSaveBusy);
            return false;
        }

        m_expectedArtifact = std::move(*cached.Value);
        const CampaignNativeSaveRequestResult accepted =
            CampaignNativeSave::ValidateExistingOnGameThread(
                identity, *m_expectedArtifact);
        if (!accepted.WasAccepted())
        {
            Fail(
                accepted.State ==
                        CampaignNativeSaveRequestState::RequestAlreadyActive
                    ? CampaignNativeLoadFailure::NativeSaveBusy
                    : CampaignNativeLoadFailure::ArtifactValidationRejected);
            return false;
        }

        SetDeadline(std::chrono::seconds{35});
        Log("ARTIFACT_VALIDATION_SCHEDULED",
            fmt::format("identity={}", identity).c_str());
        return true;
    }
    catch (...)
    {
        Fail(CampaignNativeLoadFailure::InternalFailure);
        return false;
    }
}

bool CampaignNativeLoadService::ReleaseForValidation() noexcept
{
    const CampaignNativeLoadState state = m_request.Snapshot().State;
    if (state != CampaignNativeLoadState::Completed &&
        state != CampaignNativeLoadState::Failed)
    {
        Log("RELEASE_REJECTED", "reason=request-not-terminal");
        return false;
    }

    if (m_gate.IsLocked() && !m_gate.ReleaseManagedLoad())
    {
        Log("RELEASE_REJECTED", "reason=gate-release-failed");
        return false;
    }
    (void)m_gate.CancelManagedLoad();

    const std::string identity = m_request.Snapshot().Identity;
    if (!m_request.Reset())
        return false;
    m_expectedArtifact.reset();
    m_deadlineActive = false;
    m_completedLogged = false;
    Log("RELEASED", fmt::format("identity={}", identity).c_str());
    return true;
}

bool CampaignNativeLoadService::RequestForRecovery(
    std::string_view acNativeSaveIdentity,
    const NativeSaveBundleArtifact& acExpectedArtifact) noexcept
{
    try
    {
        const std::string identity(
            acNativeSaveIdentity.data(), acNativeSaveIdentity.size());
        const auto parsed = ParseNativeSaveBundleArtifact(
            identity,
            acExpectedArtifact.Fingerprint,
            acExpectedArtifact.Metadata);
        if (!parsed || parsed.Value != acExpectedArtifact ||
            acExpectedArtifact.Bundle.LogicalIdentity != identity)
        {
            Log("RECOVERY_REQUEST_REJECTED",
                "reason=invalid-server-artifact-proof");
            return false;
        }

        const auto admission = m_campaignService.GetAdmission();
        if (!admission || !m_store)
            return false;
        const std::string checkpointId = identity.substr(
            kCampaignNativeSaveIdentityPrefix.size());
        auto cached = m_store->LoadCheckpointArtifact(
            admission->CampaignId, checkpointId);
        if (!cached || !cached.Value ||
            *cached.Value != acExpectedArtifact)
        {
            Log("RECOVERY_REQUEST_REJECTED",
                "reason=local-artifact-differs-from-committed-proof");
            return false;
        }
        return RequestForValidation(identity);
    }
    catch (...)
    {
        return false;
    }
}

bool CampaignNativeLoadService::ResetForRecoveryRetry() noexcept
{
    if (m_request.Snapshot().State != CampaignNativeLoadState::Failed)
        return false;
    (void)m_gate.CancelManagedLoad();
    if (!m_request.Reset())
        return false;
    m_expectedArtifact.reset();
    m_deadlineActive = false;
    m_completedLogged = false;
    Log("RECOVERY_RETRY_RESET");
    return true;
}

bool CampaignNativeLoadService::ReleaseForRecovery() noexcept
{
    if (m_request.Snapshot().State != CampaignNativeLoadState::Completed)
    {
        Log("RECOVERY_RELEASE_REJECTED",
            "reason=load-proof-not-complete");
        return false;
    }
    return ReleaseForValidation();
}

bool CampaignNativeLoadService::OnNativeLoadEnter(
    const char* apNativeSaveName) noexcept
{
    const std::string_view actual = apNativeSaveName
        ? std::string_view{apNativeSaveName}
        : std::string_view{};
    const bool managed = m_request.OnNativeLoadEnter(actual);
    if (managed)
    {
        Log("NATIVE_ENTER", fmt::format(
            "identity={} nativeName={} boundary=Load_Impl thread_id={}",
            m_request.Snapshot().Identity,
            actual,
            GetCurrentThreadId()).c_str());
    }
    else if (m_request.Snapshot().Failure ==
             CampaignNativeLoadFailure::UnexpectedNativeLoad)
    {
        Log("UNRELATED_NATIVE_ENTER",
            fmt::format("nativeName={}", actual).c_str());
        (void)m_gate.CancelManagedLoad();
    }
    return managed;
}

bool CampaignNativeLoadService::HasAuthoritativeAdmission() const noexcept
{
    return m_campaignService.GetAdmission().has_value();
}

bool CampaignNativeLoadService::IsCampaignRuntimeSensitive() const noexcept
{
    return HasAuthoritativeAdmission() || m_gate.IsLocked();
}

void CampaignNativeLoadService::OnNativeLoadReturn(
    bool aManaged,
    bool aSucceeded) noexcept
{
    if (!aManaged)
        return;
    Log("NATIVE_RETURN", aSucceeded
        ? "success=true completion=unproven"
        : "success=false completion=failed");
    m_request.OnNativeLoadReturn(aManaged, aSucceeded);
    if (!aSucceeded)
        (void)m_gate.CancelManagedLoad();
}

void CampaignNativeLoadService::OnPostLoad() noexcept
{
    if (!m_request.OnPostLoad())
        return;
    Log("POST_LOAD", "boundary=TESLoadGameEvent");
    m_request.ObserveGateLocked();
    Log("GATE_LOCKED", "state=LockedAfterLoad");
}

void CampaignNativeLoadService::OnGuardMenuPostDisplay(
    bool aGamePaused) noexcept
{
    OnPostLoadSafetyObserved(true, aGamePaused);
}

void CampaignNativeLoadService::OnPostLoadSafetyObserved(
    bool aGuardMenuOpen,
    bool aGamePaused) noexcept
{
    if (!m_request.IsActive() ||
        !m_request.Snapshot().GateLocked)
        return;
    const CampaignNativeLoadFailure failure =
        m_request.ObservePostLoadGuardState(
            aGuardMenuOpen, aGamePaused);
    Log(
        aGuardMenuOpen ? "GUARD_MENU_ACTIVE" : "GUARD_MENU_UNAVAILABLE",
        "menu=STRECampaignGateMenu");
    Log("GAME_PAUSED", aGamePaused ? "value=true" : "value=false");
    if (failure != CampaignNativeLoadFailure::None)
        Fail(failure);
}

void CampaignNativeLoadService::OnTransportUpdate(bool aConnected) noexcept
{
    if (!m_request.IsActive() || !m_request.Snapshot().GateLocked)
        return;
    if (!aConnected)
    {
        Fail(CampaignNativeLoadFailure::TransportUnavailable);
        return;
    }
    if (!m_request.Snapshot().TransportAlive)
        Log("TRANSPORT_ALIVE", "connected=true update_observed=true");
    m_request.ObserveTransportAlive();
}

void CampaignNativeLoadService::OnGateArmFailure() noexcept
{
    Fail(CampaignNativeLoadFailure::GateArmFailed);
}

void CampaignNativeLoadService::OnUpdate(const UpdateEvent&) noexcept
{
    try
    {
        if (m_request.Snapshot().State ==
            CampaignNativeLoadState::ValidatingArtifact)
        {
            const CampaignNativeSaveLifecycleSnapshot status =
                CampaignNativeSave::GetStatus();
            if (status.Identity != m_request.Snapshot().Identity)
            {
                if (NativeSaveIsActive(status.State))
                    Fail(CampaignNativeLoadFailure::NativeSaveBusy);
            }
            else if (status.State == CampaignNativeSaveLifecycleState::Failed)
            {
                Fail(CampaignNativeLoadFailure::ArtifactValidationFailed);
            }
            else if (status.State == CampaignNativeSaveLifecycleState::Completed)
            {
                if (!status.Artifact || !m_expectedArtifact ||
                    *status.Artifact != *m_expectedArtifact)
                {
                    Fail(CampaignNativeLoadFailure::ArtifactInvalid);
                }
                else if (m_request.MarkArtifactValidated())
                {
                    Log("ARTIFACT_VALIDATED", fmt::format(
                        "identity={} fingerprint={}",
                        status.Identity,
                        NativeSaveSha256ToHex(
                            status.Artifact->Fingerprint)).c_str());
                    BeginNativeInvocation();
                }
            }
        }

        if (m_deadlineActive && m_request.IsActive() &&
            std::chrono::steady_clock::now() >= m_deadline)
        {
            const auto& snapshot = m_request.Snapshot();
            if (snapshot.State == CampaignNativeLoadState::ValidatingArtifact)
                Fail(CampaignNativeLoadFailure::Timeout);
            else if (!snapshot.PostLoadObserved)
                Fail(CampaignNativeLoadFailure::PostLoadMissing);
            else if (!snapshot.GateLocked)
                Fail(CampaignNativeLoadFailure::GateNotLocked);
            else if (!snapshot.GuardMenuObserved)
                Fail(CampaignNativeLoadFailure::GuardMenuUnavailable);
            else if (!snapshot.GamePaused)
                Fail(CampaignNativeLoadFailure::GameNotPaused);
            else if (!snapshot.TransportAlive)
                Fail(CampaignNativeLoadFailure::TransportUnavailable);
            else
                Fail(CampaignNativeLoadFailure::Timeout);
        }

        if (m_request.Snapshot().State == CampaignNativeLoadState::Completed &&
            !m_completedLogged)
        {
            m_completedLogged = true;
            m_deadlineActive = false;
            Log("COMPLETED", fmt::format(
                "identity={} proof=artifact+enter+return+postload+gate+menu+pause+transport",
                m_request.Snapshot().Identity).c_str());
        }
    }
    catch (...)
    {
        Fail(CampaignNativeLoadFailure::InternalFailure);
    }
}

void CampaignNativeLoadService::BeginNativeInvocation() noexcept
{
    if (!m_gate.ArmManagedLoad())
    {
        Fail(CampaignNativeLoadFailure::GateArmFailed);
        return;
    }
    if (!m_request.BeginInvocation())
    {
        (void)m_gate.CancelManagedLoad();
        Fail(CampaignNativeLoadFailure::InternalFailure);
        return;
    }

    SetDeadline(std::chrono::seconds{60});
    Log("SCHEDULED", fmt::format(
        "identity={} boundary=RunnerService-game-update thread_id={}",
        m_request.Snapshot().Identity,
        GetCurrentThreadId()).c_str());
    Log("INVOKE", fmt::format(
        "identity={} nativeName={} device=-1 outputStats=0 checkForMods=true thread_id={}",
        m_request.Snapshot().Identity,
        m_request.Snapshot().Identity,
        GetCurrentThreadId()).c_str());

    const CampaignNativeLoadInvokeResult result =
        CampaignNativeLoad::InvokeValidated(m_request.Snapshot().Identity);
    if (result == CampaignNativeLoadInvokeResult::BoundaryUnavailable)
    {
        (void)m_gate.CancelManagedLoad();
        Fail(CampaignNativeLoadFailure::NativeBoundaryUnavailable);
    }
    else if (result == CampaignNativeLoadInvokeResult::NativeRejected &&
             m_request.Snapshot().State != CampaignNativeLoadState::Failed)
    {
        Fail(CampaignNativeLoadFailure::NativeLoadRejected);
    }
}

void CampaignNativeLoadService::Fail(
    CampaignNativeLoadFailure aFailure) noexcept
{
    if (!m_request.Fail(aFailure))
        return;
    m_deadlineActive = false;
    Log("FAILED", fmt::format(
        "identity={} reason={}",
        m_request.Snapshot().Identity,
        ToString(aFailure)).c_str());
}

void CampaignNativeLoadService::SetDeadline(
    std::chrono::seconds aDuration) noexcept
{
    m_deadline = std::chrono::steady_clock::now() + aDuration;
    m_deadlineActive = true;
}

void CampaignNativeLoadService::Log(
    const char* apEvent,
    const char* apDetail) const noexcept
{
    spdlog::info(
        "[STRE][CampaignNativeLoad] {} {}",
        apEvent,
        apDetail ? apDetail : "");
}
