#include <TiltedOnlinePCH.h>

#include <Services/CampaignResumeService.h>

#include <Services/CampaignService.h>
#include <Services/OverlayService.h>
#include <Services/TransportService.h>
#include <Services/UiSurfaceService.h>
#include <Services/CampaignRuntimeGateService.h>
#include <Services/CampaignRecoveryService.h>

#include <Events/DisconnectedEvent.h>
#include <Events/CampaignMainMenuEnteredEvent.h>
#include <Events/UpdateEvent.h>

#include <Games/Skyrim/MainMenuRuntime.h>

#include <OverlayApp.hpp>
#include <Structs/Campaign.h>

#include <algorithm>
#include <string_view>
#include <utility>

namespace
{
CampaignResumeService* s_pCampaignResumeService{};

bool Succeeded(CampaignProtocolResult aResult) noexcept
{
    return aResult == CampaignProtocolResult::Applied ||
        aResult == CampaignProtocolResult::AcceptedNoOp ||
        aResult == CampaignProtocolResult::IdempotentReplay;
}

const char* ErrorCode(CampaignProtocolResult aResult) noexcept
{
    switch (aResult)
    {
    case CampaignProtocolResult::BindingMismatch:
        return "binding_mismatch";
    case CampaignProtocolResult::IdentityMismatch:
    case CampaignProtocolResult::DuplicateIdentity:
        return "identity_mismatch";
    case CampaignProtocolResult::CampaignNotFound:
        return "campaign_unavailable";
    case CampaignProtocolResult::SessionMismatch:
    case CampaignProtocolResult::PartyAlignmentFailed:
        return "session_unavailable";
    case CampaignProtocolResult::PersistenceFailure:
        return "server_error";
    default:
        return "resume_rejected";
    }
}

const char* PhaseName(
    STRE::Campaign::CampaignResumePhase aPhase) noexcept
{
    using STRE::Campaign::CampaignResumePhase;
    switch (aPhase)
    {
    case CampaignResumePhase::Ready: return "ready";
    case CampaignResumePhase::Submitting: return "submitting";
    case CampaignResumePhase::Admitted: return "admitted";
    case CampaignResumePhase::WaitingForRoster: return "waitingForRoster";
    case CampaignResumePhase::Recovery: return "recovery";
    case CampaignResumePhase::Synchronizing: return "synchronizing";
    case CampaignResumePhase::Active: return "active";
    case CampaignResumePhase::Error: return "error";
    case CampaignResumePhase::Unavailable:
    default: return "unavailable";
    }
}

const char* IncidentKindName(
    STRE::Campaign::CampaignRecoveryIncidentKind aKind) noexcept
{
    using STRE::Campaign::CampaignRecoveryIncidentKind;
    switch (aKind)
    {
    case CampaignRecoveryIncidentKind::RemotePlayerMissing:
        return "remotePlayerMissing";
    case CampaignRecoveryIncidentKind::MultiplePlayersMissing:
        return "multiplePlayersMissing";
    case CampaignRecoveryIncidentKind::LocalTransportLost:
        return "localTransportLost";
    case CampaignRecoveryIncidentKind::RosterRestored:
    default:
        return "rosterRestored";
    }
}

void AppendJsonString(std::string& aOutput, std::string_view acValue)
{
    aOutput.push_back('"');
    for (const unsigned char value : acValue)
    {
        switch (value)
        {
        case '"': aOutput += "\\\""; break;
        case '\\': aOutput += "\\\\"; break;
        case '\n': aOutput += "\\n"; break;
        case '\r': aOutput += "\\r"; break;
        case '\t': aOutput += "\\t"; break;
        default:
            if (value >= 0x20)
                aOutput.push_back(static_cast<char>(value));
            break;
        }
    }
    aOutput.push_back('"');
}
}

CampaignResumeService::CampaignResumeService(
    entt::dispatcher& aDispatcher,
    TransportService& aTransport,
    CampaignService& aCampaignService,
    UiSurfaceService& aUiSurfaceService,
    CampaignRecoveryService& aRecoveryService) noexcept
    : m_transport(aTransport)
    , m_campaignService(aCampaignService)
    , m_uiSurfaceService(aUiSurfaceService)
    , m_recoveryService(aRecoveryService)
    , m_updateConnection(aDispatcher.sink<UpdateEvent>()
          .connect<&CampaignResumeService::OnUpdate>(this))
    , m_disconnectedConnection(aDispatcher.sink<DisconnectedEvent>()
          .connect<&CampaignResumeService::OnDisconnected>(this))
    , m_mainMenuEnteredConnection(
          aDispatcher.sink<CampaignMainMenuEnteredEvent>()
              .connect<&CampaignResumeService::OnMainMenuEntered>(this))
{
    s_pCampaignResumeService = this;
}

CampaignResumeService::~CampaignResumeService() noexcept
{
    if (s_pCampaignResumeService == this)
        s_pCampaignResumeService = nullptr;
}

CampaignResumeService* CampaignResumeService::TryGet() noexcept
{
    return s_pCampaignResumeService;
}

void CampaignResumeService::Refresh() noexcept
{
    LoadCandidates(false);
}

void CampaignResumeService::Retry() noexcept
{
    if (!m_resumeRequired || !m_transport.IsOnline())
    {
        LoadCandidates(true);
        return;
    }

    const auto admission = m_campaignService.GetAdmission();
    if (!admission || !m_state.RetrySelected(admission->CampaignId))
    {
        LoadCandidates(true);
        return;
    }

    m_lastOutcomeToken.clear();
    m_campaignService.SetResumeRequiresCheckpointRestore(true);
    if (!m_campaignService.ResumeCampaign(admission->CampaignId, true))
    {
        m_campaignService.SetResumeRequiresCheckpointRestore(false);
        m_state.Fail("send_failed");
    }
    PublishState(true);
}

void CampaignResumeService::LoadCandidates(bool aForce) noexcept
{
    using STRE::Campaign::CampaignResumePhase;
    if (!aForce &&
        (m_state.GetPhase() == CampaignResumePhase::Submitting ||
         m_state.GetPhase() == CampaignResumePhase::Admitted ||
         m_state.GetPhase() == CampaignResumePhase::WaitingForRoster ||
         m_state.GetPhase() == CampaignResumePhase::Recovery ||
         m_state.GetPhase() == CampaignResumePhase::Active))
    {
        PublishState(true);
        return;
    }
    std::vector<STRE::Campaign::CampaignResumeCandidate> candidates;
    m_roster.clear();
    if (m_resumeRequired)
    {
        if (!m_loadedMarker)
        {
            (void)m_state.ReplaceCandidates({});
            m_state.Fail(m_loadError.empty()
                ? "save_marker_invalid" : m_loadError);
            PublishState(true);
            return;
        }

        auto binding = m_campaignService.LoadCampaignBinding(
            m_loadedMarker->CampaignId);
        if (!binding)
        {
            m_state.FailCache();
            PublishState(true);
            return;
        }
        if (!binding.Value ||
            binding.Value->CampaignId != m_loadedMarker->CampaignId ||
            binding.Value->CampaignSlotId !=
                m_loadedMarker->CampaignSlotId ||
            binding.Value->CharacterBindingId !=
                m_loadedMarker->CharacterBindingId)
        {
            (void)m_state.ReplaceCandidates({});
            m_state.Fail("save_binding_unavailable");
            PublishState(true);
            return;
        }
        candidates.push_back({
            STRE::Campaign::CampaignIdentityStore::GenerateOpaqueId(16),
            binding.Value->CampaignId});
    }
    else
    {
        auto bindings = m_campaignService.ListCampaignBindings();
        if (!bindings)
        {
            m_state.FailCache();
            PublishState(true);
            return;
        }
        candidates.reserve(bindings.Value.size());
        for (const auto& binding : bindings.Value)
        {
            candidates.push_back({
                STRE::Campaign::CampaignIdentityStore::GenerateOpaqueId(16),
                binding.CampaignId});
        }
    }
    if (!m_state.ReplaceCandidates(std::move(candidates)))
        m_state.FailCache();
    PublishState(true);
}

void CampaignResumeService::Select(std::string aToken) noexcept
{
    if (!m_transport.IsOnline())
    {
        m_state.Fail("not_connected");
        PublishState(true);
        return;
    }
    if (m_campaignService.GetAdmission())
    {
        m_state.Fail("already_admitted");
        PublishState(true);
        return;
    }

    const auto campaign = m_state.Select(aToken);
    if (!campaign)
    {
        PublishState(true);
        return;
    }
    m_lastOutcomeToken.clear();
    if (m_resumeRequired && !m_loadedMarker)
    {
        m_state.Fail(m_loadError.empty()
            ? "save_marker_invalid" : m_loadError);
    }
    else
    {
        m_campaignService.SetResumeRequiresCheckpointRestore(true);
        if (!m_campaignService.ResumeCampaign(*campaign, true))
        {
            m_campaignService.SetResumeRequiresCheckpointRestore(false);
            m_state.Fail("send_failed");
        }
    }
    PublishState(true);
}

void CampaignResumeService::StayAndRecover() noexcept
{
    if (!m_recoveryUiState.StayAndRecover())
        return;

    spdlog::info(
        "[STRE][CampaignRecoveryUi] DISCONNECT_ACTION action=StayAndRecover");
    spdlog::info(
        "[STRE][CampaignRecoveryUi] EXISTING_RECOVERY_UI_ENTERED source=disconnect-incident");
    OpenMandatorySurface();
    PublishState(true);
}

void CampaignResumeService::ReturnToMainMenu() noexcept
{
    if (!m_recoveryUiState.RequestMainMenu())
        return;

    spdlog::info(
        "[STRE][CampaignRecoveryUi] DISCONNECT_ACTION action=ReturnToMainMenu");
    PublishState(true);
}

void CampaignResumeService::OnUpdate(const UpdateEvent&) noexcept
{
    ProcessCommandOutcome();
    ObserveCanonicalState();
    if (m_recoveryUiState.GetMode() ==
        STRE::Campaign::CampaignRecoveryUiMode::DisconnectIncident)
    {
        const auto recoveryStage = m_recoveryService.GetStage();
        if (recoveryStage ==
                STRE::Campaign::CampaignClientRecoveryStage::LoadingNativeSave ||
            recoveryStage ==
                STRE::Campaign::CampaignClientRecoveryStage::NativeSaveLoaded ||
            recoveryStage ==
                STRE::Campaign::CampaignClientRecoveryStage::SnapshotApplied ||
            recoveryStage ==
                STRE::Campaign::CampaignClientRecoveryStage::Failed)
        {
            if (m_recoveryUiState.StayAndRecover())
            {
                spdlog::info(
                    "[STRE][CampaignRecoveryUi] EXISTING_RECOVERY_UI_ENTERED source=authoritative-recovery-progress");
            }
        }
    }
    const auto resumePhase = m_state.GetPhase();
    if (m_resumeRequired &&
        (resumePhase == STRE::Campaign::CampaignResumePhase::Recovery ||
         resumePhase ==
             STRE::Campaign::CampaignResumePhase::Synchronizing) &&
        m_recoveryService.GetStage() ==
            STRE::Campaign::CampaignClientRecoveryStage::Failed)
    {
        m_state.Fail("recovery_failed");
    }
    if (m_resumeRequired && m_openSurfaceWhenReady &&
        m_uiSurfaceService.GetSurface() == UiSurface::None &&
        m_uiSurfaceService.GetOverlayService().GetInGame())
    {
        m_uiSurfaceService.SetSurface(UiSurface::SkyrimTogether);
        m_openSurfaceWhenReady =
            m_uiSurfaceService.GetSurface() != UiSurface::SkyrimTogether;
    }
    PublishState();
    DispatchPendingMainMenuRequest();
}

void CampaignResumeService::DispatchPendingMainMenuRequest() noexcept
{
    if (!m_recoveryUiState.BeginMainMenuDispatch())
        return;

    spdlog::info(
        "[STRE][CampaignRecoveryUi] RETURN_TO_MAIN_MENU_NATIVE_DISPATCH_BEGIN");
    const bool dispatched = RequestSkyrimMainMenu();
    spdlog::info(
        "[STRE][CampaignRecoveryUi] RETURN_TO_MAIN_MENU_NATIVE_DISPATCH_RETURN result={}",
        dispatched);
    if (dispatched)
        return;

    m_recoveryUiState.CancelMainMenuRequest();
    spdlog::error(
        "[STRE][CampaignRecoveryUi] MAIN_MENU_REQUEST_FAILED reason=native-runtime-unavailable");
    PublishState(true);
}

void CampaignResumeService::OpenMandatorySurface() noexcept
{
    if (!m_uiSurfaceService.GetOverlayService().GetInGame())
        return;
    m_uiSurfaceService.SetSurface(UiSurface::SkyrimTogether);
}

void CampaignResumeService::OnDisconnected(
    const DisconnectedEvent&) noexcept
{
    using STRE::Campaign::CampaignResumePhase;
    if (m_state.GetPhase() != CampaignResumePhase::Unavailable &&
        m_state.GetPhase() != CampaignResumePhase::Ready)
    {
        m_state.Fail("connection_lost");
        PublishState(true);
    }

    if (!m_campaignService.IsMainMenuRuntimeDepartureDisconnect() &&
        m_recoveryUiState.OpenLocalTransportLoss())
    {
        spdlog::info(
            "[STRE][CampaignRecoveryUi] DISCONNECT_INCIDENT_OPENED campaign={} missingMembers={} localTransportLost=true",
            m_recoveryUiState.GetCampaignId(),
            m_recoveryUiState.GetMissingRemoteCount());
        OpenMandatorySurface();
        PublishState(true);
    }
}

void CampaignResumeService::OnMainMenuEntered(
    const CampaignMainMenuEnteredEvent&) noexcept
{
    if (m_recoveryUiState.GetMode() ==
        STRE::Campaign::CampaignRecoveryUiMode::Hidden)
    {
        m_recoveryUiState.Complete();
        return;
    }

    const bool requested = m_recoveryUiState.IsMainMenuRequested();
    m_recoveryUiState.Complete();
    m_state.Complete();
    m_roster.clear();
    m_uiSurfaceService.SetSurface(UiSurface::None);
    spdlog::info(
        "[STRE][CampaignRecoveryUi] UI_CLOSED reason=main-menu-entered");
    if (CampaignRuntimeGateService* const pGate =
            CampaignRuntimeGateService::TryGet();
        pGate && pGate->IsLocked())
    {
        (void)pGate->ReleaseForMainMenuRuntimeDeparture();
    }
    spdlog::info(
        "[STRE][CampaignRecoveryUi] INCIDENT_CLOSED reason={} bindingRetained=true",
        requested ? "requested-main-menu" : "main-menu-entered");
    PublishState(true);
}

void CampaignResumeService::ProcessCommandOutcome() noexcept
{
    const auto& outcome = m_campaignService.GetLastCommandOutcome();
    if (!outcome)
        return;

    const std::string token = fmt::format(
        "{}:{}:{}:{}:{}",
        static_cast<unsigned>(outcome->Operation), outcome->MutationId,
        static_cast<unsigned>(outcome->Result), outcome->CampaignId,
        outcome->StateVersion);
    if (token == m_lastOutcomeToken)
        return;
    m_lastOutcomeToken = token;

    if (outcome->Operation == CampaignProtocolOperation::Leave &&
        Succeeded(outcome->Result))
    {
        LoadCandidates(true);
        return;
    }
    if (outcome->Operation != CampaignProtocolOperation::Resume)
        return;

    if (!Succeeded(outcome->Result))
    {
        m_campaignService.SetResumeRequiresCheckpointRestore(false);
        (void)m_state.Reject(
            outcome->CampaignId, ErrorCode(outcome->Result));
        return;
    }

    const auto admission = m_campaignService.GetAdmission();
    if (!admission || admission->CampaignId != outcome->CampaignId)
    {
        m_campaignService.SetResumeRequiresCheckpointRestore(false);
        (void)m_state.Reject(outcome->CampaignId, "admission_failed");
        return;
    }
    (void)m_state.Accept(outcome->CampaignId);
}

void CampaignResumeService::ObserveCanonicalState() noexcept
{
    const auto& snapshot = m_campaignService.GetLatestSnapshot();
    if (!snapshot)
        return;
    const std::size_t presentCount = static_cast<std::size_t>(std::count_if(
        snapshot->Roster.begin(), snapshot->Roster.end(),
        [](const CampaignPublicSlotData& acSlot) { return acSlot.Present; }));
    const auto admission = m_campaignService.GetAdmission();
    if (admission && admission->CampaignId == snapshot->CampaignId.c_str())
    {
        std::vector<STRE::Campaign::CampaignRecoveryUiRosterMember>
            recoveryRoster;
        recoveryRoster.reserve(snapshot->Roster.size());
        for (const auto& slot : snapshot->Roster)
        {
            recoveryRoster.push_back({
                slot.Present,
                admission->CampaignSlotId == slot.SlotId.c_str()});
        }
        const auto beforeMode = m_recoveryUiState.GetMode();
        (void)m_recoveryUiState.ObserveSnapshot(
            snapshot->CampaignId.c_str(), snapshot->RuntimeState,
            snapshot->RosterSealed, std::move(recoveryRoster));
        if (beforeMode ==
                STRE::Campaign::CampaignRecoveryUiMode::Hidden &&
            m_recoveryUiState.GetMode() ==
                STRE::Campaign::CampaignRecoveryUiMode::DisconnectIncident)
        {
            spdlog::info(
                "[STRE][CampaignRecoveryUi] DISCONNECT_INCIDENT_OPENED campaign={} missingMembers={} localTransportLost=false",
                m_recoveryUiState.GetCampaignId(),
                m_recoveryUiState.GetMissingRemoteCount());
            OpenMandatorySurface();
            PublishState(true);
        }
    }
    if (m_resumeRequired &&
        (snapshot->RuntimeState == kCampaignWireRuntimeRecoveryLock ||
         snapshot->RuntimeState ==
             kCampaignWireRuntimeRestoringCheckpoint))
    {
        m_recoveryObserved = true;
    }
    const bool observed = m_state.ObserveRuntime(
        snapshot->CampaignId.c_str(), snapshot->RuntimeState,
        snapshot->RosterSealed, snapshot->Roster.size(), presentCount);
    if (observed)
    {
        m_roster.clear();
        m_roster.reserve(snapshot->Roster.size());
        for (const auto& slot : snapshot->Roster)
        {
            m_roster.push_back({
                slot.Present,
                admission && admission->CampaignSlotId ==
                    slot.SlotId.c_str()});
        }
    }

    if (!snapshot->RosterSealed)
        m_campaignService.SetResumeRequiresCheckpointRestore(false);

    if (m_recoveryUiState.GetMode() !=
            STRE::Campaign::CampaignRecoveryUiMode::Hidden &&
        m_recoveryUiState.GetCampaignId() == snapshot->CampaignId.c_str() &&
        snapshot->RuntimeState == kCampaignWireRuntimeActive)
    {
        CampaignRuntimeGateService* const pGate =
            CampaignRuntimeGateService::TryGet();
        const auto recoveryStage = m_recoveryService.GetStage();
        const bool authoritativelyFinished =
            recoveryStage ==
                STRE::Campaign::CampaignClientRecoveryStage::Completed ||
            recoveryStage ==
                STRE::Campaign::CampaignClientRecoveryStage::Idle;
        if (authoritativelyFinished && pGate && !pGate->IsLocked())
        {
            m_recoveryUiState.Complete();
            m_state.Complete();
            m_roster.clear();
            spdlog::info(
                "[STRE][CampaignRecoveryUi] EXISTING_RECOVERY_UI_COMPLETED action=Close");
            PublishState(true);
            if (m_uiSurfaceService.GetSurface() ==
                UiSurface::SkyrimTogether)
            {
                m_uiSurfaceService.SetSurface(UiSurface::None);
            }
        }
    }

    if (m_resumeRequired && m_recoveryObserved &&
        snapshot->RuntimeState == kCampaignWireRuntimeActive)
    {
        CampaignRuntimeGateService* const pGate =
            CampaignRuntimeGateService::TryGet();
        if (pGate && !pGate->IsLocked())
        {
            m_state.Complete();
            m_recoveryUiState.Complete();
            m_resumeRequired = false;
            m_loadedMarker.reset();
            m_loadedSaveIdentity.clear();
            m_loadError.clear();
            m_roster.clear();
            m_openSurfaceWhenReady = false;
            m_campaignService.SetResumeRequiresCheckpointRestore(false);
            spdlog::info(
                "[STRE][CampaignResume] RESUME_REQUIRED_COMPLETED action=Close");
            PublishState(true);
            if (m_uiSurfaceService.GetSurface() ==
                UiSurface::SkyrimTogether)
            {
                m_uiSurfaceService.SetSurface(UiSurface::None);
                spdlog::info(
                    "[STRE][CampaignResume] UI_CLOSED reason=authoritative-recovery-complete");
            }
        }
    }
}

STRE::Campaign::CampaignResumePhase
CampaignResumeService::GetProjectedPhase() const noexcept
{
    using STRE::Campaign::CampaignClientRecoveryStage;
    using STRE::Campaign::CampaignRecoveryUiMode;
    using STRE::Campaign::CampaignResumePhase;

    if (m_recoveryUiState.GetMode() ==
        CampaignRecoveryUiMode::DisconnectIncident)
    {
        return CampaignResumePhase::WaitingForRoster;
    }
    if (m_recoveryUiState.GetMode() !=
        CampaignRecoveryUiMode::ExistingRecovery)
    {
        return m_state.GetPhase();
    }
    if (!m_transport.IsOnline())
        return CampaignResumePhase::WaitingForRoster;

    const auto& snapshot = m_campaignService.GetLatestSnapshot();
    if (!snapshot ||
        m_recoveryUiState.GetCampaignId() != snapshot->CampaignId.c_str())
    {
        return CampaignResumePhase::WaitingForRoster;
    }
    if (snapshot->RuntimeState ==
        kCampaignWireRuntimeRestoringCheckpoint)
    {
        return CampaignResumePhase::Synchronizing;
    }
    if (m_recoveryService.GetStage() ==
        CampaignClientRecoveryStage::Failed)
    {
        return CampaignResumePhase::Error;
    }
    if (m_recoveryUiState.GetMissingRemoteCount() != 0)
        return CampaignResumePhase::WaitingForRoster;
    if (m_recoveryService.GetStage() ==
            CampaignClientRecoveryStage::NativeSaveLoaded ||
        m_recoveryService.GetStage() ==
            CampaignClientRecoveryStage::SnapshotApplied)
    {
        return CampaignResumePhase::Synchronizing;
    }
    if (snapshot->RuntimeState == kCampaignWireRuntimeActive)
        return CampaignResumePhase::Active;
    return CampaignResumePhase::Recovery;
}

STRE::Campaign::CampaignLoadTarget
CampaignResumeService::InspectNativeLoadTarget(
    std::string_view acSaveName) noexcept
{
    using STRE::Campaign::CampaignLoadTarget;
    if (acSaveName.empty())
        return CampaignLoadTarget::Unknown;
    if (!acSaveName.starts_with("stre-"))
        return CampaignLoadTarget::Ordinary;

    auto marker = m_campaignService.LoadCampaignSaveMarker(
        std::string(acSaveName));
    return marker && marker.Value
        ? CampaignLoadTarget::Campaign
        : CampaignLoadTarget::Unknown;
}

bool CampaignResumeService::OnNativeLoadEnter(
    const char* apSaveName) noexcept
{
    if (!apSaveName)
        return false;
    m_continueTransition.Cancel();
    if (!PrepareLoadedSaveIdentity(apSaveName))
        return false;

    CampaignRuntimeGateService* const pGate =
        CampaignRuntimeGateService::TryGet();
    if (!pGate || !pGate->ArmResumeRequiredLoad())
    {
        ClearLoadedSaveIdentity();
        return false;
    }
    spdlog::info(
        "[STRE][CampaignResume] campaign save load fenced identity={} marker={}",
        m_loadedSaveIdentity, m_loadedMarker ? "valid" : "invalid");
    return true;
}

void CampaignResumeService::OnNativeLoadReturn(
    bool aManaged,
    bool aSucceeded) noexcept
{
    if (!aManaged || aSucceeded)
        return;
    ClearLoadedSaveIdentity();
}

void CampaignResumeService::OnPostLoad() noexcept
{
    if (m_loadedSaveIdentity.empty() || m_resumeRequired)
        return;
    m_continueTransition.Cancel();
    m_resumeRequired = true;
    m_recoveryObserved = false;
    m_openSurfaceWhenReady = true;
    LoadCandidates(true);
    spdlog::warn(
        "[STRE][CampaignResume] gameplay remains fenced until authoritative checkpoint recovery identity={}",
        m_loadedSaveIdentity);
}

bool CampaignResumeService::PrepareLoadedSaveIdentity(
    std::string_view acNativeSaveIdentity) noexcept
{
    ClearLoadedSaveIdentity();
    if (acNativeSaveIdentity.empty() ||
        !acNativeSaveIdentity.starts_with("stre-"))
    {
        return false;
    }

    m_loadedSaveIdentity.assign(acNativeSaveIdentity);
    auto marker = m_campaignService.LoadCampaignSaveMarker(
        m_loadedSaveIdentity);
    if (!marker)
    {
        m_loadError = marker.Error ==
                STRE::Campaign::LocalIdentityError::Malformed
            ? "save_marker_invalid"
            : "save_marker_unavailable";
    }
    else if (!marker.Value)
    {
        m_loadError = "save_marker_missing";
    }
    else
    {
        m_loadedMarker = std::move(*marker.Value);
    }
    return true;
}

void CampaignResumeService::ClearLoadedSaveIdentity() noexcept
{
    m_loadedMarker.reset();
    m_loadedSaveIdentity.clear();
    m_loadError.clear();
    m_openSurfaceWhenReady = false;
}

bool CampaignResumeService::BeginContinueLoadPending(
    std::string_view acNativeSaveIdentity) noexcept
{
    m_continueTransition.Cancel();
    if (!PrepareLoadedSaveIdentity(acNativeSaveIdentity) ||
        !m_continueTransition.Begin(m_loadedSaveIdentity))
    {
        ClearLoadedSaveIdentity();
        return false;
    }
    spdlog::info(
        "[STRE][CampaignResume] RESUME_REQUIRED_PENDING identity={} marker={} gatePresentation=deferred-until-main-menu-closed",
        m_loadedSaveIdentity,
        m_loadedMarker ? "valid" : "invalid");
    return true;
}

bool CampaignResumeService::ObserveContinueNativeResult(
    bool aAccepted) noexcept
{
    const bool accepted =
        m_continueTransition.ObserveNativeResult(aAccepted);
    if (!accepted)
    {
        ClearLoadedSaveIdentity();
        spdlog::warn(
            "[STRE][CampaignResume] RESUME_REQUIRED_PENDING_CANCELLED reason=native-continue-rejected");
        return false;
    }
    spdlog::info(
        "[STRE][CampaignResume] RESUME_REQUIRED_NATIVE_TRANSITION_ACCEPTED identity={}",
        m_loadedSaveIdentity);
    return true;
}

bool CampaignResumeService::CommitContinueMainMenuClosed() noexcept
{
    std::optional<std::string> identity =
        m_continueTransition.CommitMainMenuClosed();
    if (!identity)
        return false;
    if (*identity != m_loadedSaveIdentity)
    {
        ClearLoadedSaveIdentity();
        return false;
    }

    CampaignRuntimeGateService* const pGate =
        CampaignRuntimeGateService::TryGet();
    const bool armed = pGate && pGate->ArmResumeRequiredLoad();
    const bool committed = armed &&
        pGate->CommitResumeRequiredTransition();
    if (!committed)
    {
        if (armed)
            (void)pGate->CancelManagedLoad();
        ClearLoadedSaveIdentity();
        spdlog::error(
            "[STRE][CampaignResume] RESUME_REQUIRED_PENDING_CANCELLED reason=gate-transition-unavailable");
        return false;
    }

    spdlog::info(
        "[STRE][CampaignResume] RESUME_REQUIRED_TRANSITION_COMMITTED identity={} boundary=MainMenuClosed",
        *identity);
    OnPostLoad();
    return true;
}

void CampaignResumeService::CancelContinueLoadPending() noexcept
{
    m_continueTransition.Cancel();
    if (!m_resumeRequired)
        ClearLoadedSaveIdentity();
}

void CampaignResumeService::PublishState(bool aForce) noexcept
{
    using STRE::Campaign::CampaignRecoveryUiMode;
    const bool disconnectIncident =
        m_recoveryUiState.GetMode() ==
        CampaignRecoveryUiMode::DisconnectIncident;
    const bool disconnectRecovery =
        m_recoveryUiState.GetMode() ==
        CampaignRecoveryUiMode::ExistingRecovery;
    std::string json = "{\"phase\":";
    AppendJsonString(json, PhaseName(GetProjectedPhase()));
    json += ",\"connected\":";
    json += m_transport.IsOnline() ? "true" : "false";
    json += ",\"resumeRequired\":";
    json += m_resumeRequired ? "true" : "false";
    json += ",\"disconnectIncident\":";
    json += disconnectIncident ? "true" : "false";
    json += ",\"disconnectRecovery\":";
    json += disconnectRecovery ? "true" : "false";
    json += ",\"incidentKind\":";
    AppendJsonString(
        json, IncidentKindName(m_recoveryUiState.GetIncidentKind()));
    json += ",\"missingMembers\":";
    json += std::to_string(m_recoveryUiState.GetMissingRemoteCount());
    json += ",\"loadedSaveValid\":";
    json += m_resumeRequired && m_loadedMarker ? "true" : "false";
    json += ",\"selectedToken\":";
    AppendJsonString(
        json,
        disconnectIncident || disconnectRecovery
            ? std::string_view{}
            : std::string_view{m_state.GetSelectedToken()});
    json += ",\"candidates\":[";
    const auto& candidates = m_state.GetCandidates();
    const std::size_t candidateCount =
        disconnectIncident || disconnectRecovery ? 0 : candidates.size();
    for (std::size_t index = 0; index < candidateCount; ++index)
    {
        if (index > 0)
            json.push_back(',');
        json += "{\"token\":";
        AppendJsonString(json, candidates[index].Token);
        json += ",\"ordinal\":";
        json += std::to_string(candidates[index].Ordinal);
        json.push_back('}');
    }
    json += "],\"roster\":[";
    const bool useRecoveryRoster =
        disconnectIncident || disconnectRecovery;
    const auto& projectedRoster = m_recoveryUiState.GetRoster();
    const std::size_t rosterCount = useRecoveryRoster
        ? projectedRoster.size()
        : m_roster.size();
    for (std::size_t index = 0; index < rosterCount; ++index)
    {
        if (index > 0)
            json.push_back(',');
        json += "{\"ordinal\":";
        json += std::to_string(index + 1);
        json += ",\"present\":";
        const bool present = useRecoveryRoster
            ? projectedRoster[index].Present
            : m_roster[index].Present;
        const bool local = useRecoveryRoster
            ? projectedRoster[index].Local
            : m_roster[index].Local;
        json += present ? "true" : "false";
        json += ",\"local\":";
        json += local ? "true" : "false";
        json.push_back('}');
    }
    json += "],\"error\":";
    AppendJsonString(json, m_state.GetErrorCode());
    json.push_back('}');

    if (!aForce && json == m_lastStateJson)
        return;
    m_lastStateJson = json;

    auto* const pApp =
        m_uiSurfaceService.GetOverlayService().GetOverlayApp();
    if (!pApp)
        return;
    auto arguments = CefListValue::Create();
    arguments->SetString(0, json);
    pApp->ExecuteAsync("campaignResumeState", arguments);
}
