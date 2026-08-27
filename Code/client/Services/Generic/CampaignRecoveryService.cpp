#include <TiltedOnlinePCH.h>

#include <Services/CampaignRecoveryService.h>

#include <Events/DisconnectedEvent.h>
#include <Events/UpdateEvent.h>
#include <Messages/CampaignMessages.h>
#include <Messages/CampaignRequests.h>
#include <Services/CampaignNativeLoadService.h>
#include <Services/CampaignRuntimeGateService.h>
#include <Services/CampaignService.h>
#include <Services/TransportService.h>
#include <Structs/Campaign.h>

using namespace STRE::Campaign;

namespace
{
const char* RecoveryActionName(
    CampaignRecoveryClientAction aAction) noexcept
{
    switch (aAction)
    {
    case CampaignRecoveryClientAction::Reject: return "Reject";
    case CampaignRecoveryClientAction::Ignore: return "Ignore";
    case CampaignRecoveryClientAction::StartNativeLoad:
        return "StartNativeLoad";
    case CampaignRecoveryClientAction::ResendLoaded:
        return "ResendLoaded";
    case CampaignRecoveryClientAction::ApplySnapshot:
        return "ApplySnapshot";
    case CampaignRecoveryClientAction::ResendSnapshotApplied:
        return "ResendSnapshotApplied";
    case CampaignRecoveryClientAction::Release: return "Release";
    }
    return "Unknown";
}
}

CampaignRecoveryService::CampaignRecoveryService(
    entt::dispatcher& aDispatcher,
    TransportService& aTransport,
    CampaignService& aCampaignService,
    CampaignRuntimeGateService& aGate,
    CampaignNativeLoadService& aNativeLoad) noexcept
    : m_transport(aTransport)
    , m_campaignService(aCampaignService)
    , m_gate(aGate)
    , m_nativeLoad(aNativeLoad)
    , m_campaignSnapshotConnection(
          aDispatcher.sink<NotifyCampaignSnapshot>()
              .connect<&CampaignRecoveryService::OnCampaignSnapshot>(this))
    , m_loadRequestConnection(
          aDispatcher.sink<CampaignRecoveryLoadRequest>()
              .connect<&CampaignRecoveryService::OnLoadRequest>(this))
    , m_recoverySnapshotConnection(
          aDispatcher.sink<CampaignRecoverySnapshot>()
              .connect<&CampaignRecoveryService::OnRecoverySnapshot>(this))
    , m_recoveryCompleteConnection(
          aDispatcher.sink<CampaignRecoveryComplete>()
              .connect<&CampaignRecoveryService::OnRecoveryComplete>(this))
    , m_disconnectedConnection(
          aDispatcher.sink<DisconnectedEvent>()
              .connect<&CampaignRecoveryService::OnDisconnected>(this))
    , m_updateConnection(
          aDispatcher.sink<UpdateEvent>()
              .connect<&CampaignRecoveryService::OnUpdate>(this))
{
}

void CampaignRecoveryService::OnCampaignSnapshot(
    const NotifyCampaignSnapshot& acNotification) noexcept
{
    if (!acNotification.IsValid() || !acNotification.Snapshot.RosterSealed)
        return;
    const auto admission = m_campaignService.GetAdmission();
    if (!admission || admission->CampaignId !=
            acNotification.Snapshot.CampaignId.c_str())
    {
        return;
    }

    const std::uint8_t runtime = acNotification.Snapshot.RuntimeState;
    m_protectOnDisconnect = runtime == kCampaignWireRuntimeActive ||
        runtime == kCampaignWireRuntimeCheckpointing ||
        runtime == kCampaignWireRuntimeRecoveryLock ||
        runtime == kCampaignWireRuntimeRestoringCheckpoint;
    if (m_protectOnDisconnect)
        m_protectedCampaign = admission->CampaignId;

    if (runtime == kCampaignWireRuntimeActive)
    {
        const CampaignRecoveryClientAction action =
            m_state.ObserveAuthoritativeActive(admission->CampaignId);
        if (action == CampaignRecoveryClientAction::ResendSnapshotApplied)
        {
            SendSnapshotApplied();
        }
        else if (action == CampaignRecoveryClientAction::Release &&
                 m_gate.IsLocked() &&
                 m_gate.ReleaseManagedLoad())
        {
            spdlog::info(
                "[STRE][CampaignRecovery] LOCAL_PROVISIONAL_LOCK_RELEASED campaign={} reason=authoritative-active",
                admission->CampaignId);
        }
        return;
    }

    if (runtime == kCampaignWireRuntimeRecoveryLock ||
        runtime == kCampaignWireRuntimeRestoringCheckpoint)
    {
        m_campaignService.SetResumeRequiresCheckpointRestore(false);
        if (m_state.Lock(admission->CampaignId))
            (void)m_gate.LockForRecovery();
    }
}

void CampaignRecoveryService::OnDisconnected(
    const DisconnectedEvent&) noexcept
{
    const CampaignRecoveryDisconnectContext disconnectContext =
        m_campaignService.IsMainMenuRuntimeDepartureDisconnect()
        ? CampaignRecoveryDisconnectContext::MainMenuRuntimeDeparture
        : CampaignRecoveryDisconnectContext::GameplayWorld;
    if (ProjectRecoveryDisconnectGate(disconnectContext) ==
        CampaignRecoveryLocalGateAction::SkipNoGameplay)
    {
        spdlog::info(
            "[STRE][CampaignRecovery] LOCAL_GATE_SKIPPED campaign={} reason=main-menu-runtime-departure",
            m_protectedCampaign.empty() ? "none" : m_protectedCampaign);
        m_protectOnDisconnect = false;
        m_protectedCampaign.clear();
        return;
    }

    if (!m_protectOnDisconnect || m_protectedCampaign.empty())
        return;
    if (m_state.LockProvisional(m_protectedCampaign))
    {
        (void)m_gate.LockForRecovery();
        spdlog::info(
            "[STRE][CampaignRecovery] LOCAL_RECOVERY_LOCK campaign={} reason=transport-disconnected",
            m_protectedCampaign);
    }
}

void CampaignRecoveryService::OnLoadRequest(
    const CampaignRecoveryLoadRequest& acRequest) noexcept
{
    if (!acRequest.IsValid())
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] LOAD_REQUEST_REJECTED reason=malformed-packet");
        return;
    }
    const auto admission = m_campaignService.GetAdmission();
    if (!admission ||
        admission->CampaignId != acRequest.CampaignId.c_str() ||
        admission->CampaignSlotId != acRequest.CampaignSlotId.c_str() ||
        admission->CharacterBindingId !=
            acRequest.CharacterBindingId.c_str())
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] LOAD_REQUEST_REJECTED reason=admission-mismatch");
        return;
    }

    if (!m_state.Lock(admission->CampaignId) ||
        !m_gate.LockForRecovery())
    {
        return;
    }
    CampaignRecoveryCorrelation correlation{
        acRequest.CampaignId.c_str(),
        acRequest.RestoreAttemptId.c_str(),
        acRequest.CheckpointId.c_str(),
        acRequest.SourceRevision,
        0};
    const CampaignRecoveryClientAction action =
        m_state.ObserveLoadRequest(correlation);
    spdlog::info(
        "[STRE][CampaignRecovery] LOAD_REQUEST_RECEIVED campaign={} attempt={} checkpoint={} sourceRevision={} action={}",
        correlation.CampaignId,
        correlation.RestoreAttemptId,
        correlation.CheckpointId,
        correlation.SourceRevision,
        RecoveryActionName(action));
    if (action == CampaignRecoveryClientAction::ResendLoaded)
    {
        SendLoaded(true);
        return;
    }
    if (action != CampaignRecoveryClientAction::StartNativeLoad)
        return;

    const auto parsed = ParseNativeSaveBundleArtifact(
        acRequest.NativeSaveIdentity.c_str(),
        std::span<const std::uint8_t>(
            acRequest.Fingerprint.data(), acRequest.Fingerprint.size()),
        std::span<const std::uint8_t>(
            acRequest.SaveMetadata.data(), acRequest.SaveMetadata.size()));
    if (!parsed)
    {
        (void)m_state.FinishNativeLoad(false);
        SendLoaded(false);
        return;
    }
    m_expectedArtifact = parsed.Value;

    if (m_nativeLoad.GetStatus().State == CampaignNativeLoadState::Failed)
        (void)m_nativeLoad.ResetForRecoveryRetry();
    if (!m_nativeLoad.RequestForRecovery(
            acRequest.NativeSaveIdentity.c_str(), *m_expectedArtifact))
    {
        (void)m_state.FinishNativeLoad(false);
        SendLoaded(false);
    }
}

void CampaignRecoveryService::OnUpdate(const UpdateEvent&) noexcept
{
    if (m_state.GetStage() !=
        CampaignClientRecoveryStage::LoadingNativeSave)
    {
        return;
    }
    const CampaignNativeLoadState nativeState =
        m_nativeLoad.GetStatus().State;
    if (nativeState == CampaignNativeLoadState::Completed)
    {
        spdlog::info(
            "[STRE][CampaignRecovery] NATIVE_LOAD_COMPLETION_OBSERVED campaign={} attempt={} checkpoint={} success=true",
            m_state.GetCorrelation().CampaignId,
            m_state.GetCorrelation().RestoreAttemptId,
            m_state.GetCorrelation().CheckpointId);
        if (m_state.FinishNativeLoad(true))
            SendLoaded(true);
    }
    else if (nativeState == CampaignNativeLoadState::Failed)
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] NATIVE_LOAD_COMPLETION_OBSERVED campaign={} attempt={} checkpoint={} success=false",
            m_state.GetCorrelation().CampaignId,
            m_state.GetCorrelation().RestoreAttemptId,
            m_state.GetCorrelation().CheckpointId);
        if (m_state.FinishNativeLoad(false))
            SendLoaded(false);
    }
}

void CampaignRecoveryService::SendLoaded(bool aSucceeded) noexcept
{
    const CampaignRecoveryCorrelation& correlation =
        m_state.GetCorrelation();
    CampaignRecoveryLoadedResult result;
    result.CampaignId = correlation.CampaignId.c_str();
    result.RestoreAttemptId = correlation.RestoreAttemptId.c_str();
    result.CheckpointId = correlation.CheckpointId.c_str();
    if (m_expectedArtifact)
    {
        result.NativeSaveIdentity =
            m_expectedArtifact->Bundle.LogicalIdentity.c_str();
    }
    if (aSucceeded && m_expectedArtifact)
    {
        result.Result = CampaignRecoveryLoadedResultCode::Success;
        result.FingerprintAlgorithm =
            kNativeSaveFingerprintAlgorithm.data();
        result.FingerprintVersion = kNativeSaveFingerprintVersion;
        result.Fingerprint.assign(
            m_expectedArtifact->Fingerprint.begin(),
            m_expectedArtifact->Fingerprint.end());
        result.SaveMetadataCodecVersion =
            kNativeSaveMetadataCodecVersion;
        result.SaveMetadata.assign(
            m_expectedArtifact->Metadata.begin(),
            m_expectedArtifact->Metadata.end());
    }
    else
    {
        result.Result = CampaignRecoveryLoadedResultCode::Failure;
    }
    const bool valid = result.IsValid();
    const bool sent = valid && m_transport.Send(result);
    spdlog::log(
        sent ? spdlog::level::info : spdlog::level::warn,
        "[STRE][CampaignRecovery] LOAD_RESULT_SENT campaign={} attempt={} checkpoint={} success={} valid={} sent={}",
        correlation.CampaignId,
        correlation.RestoreAttemptId,
        correlation.CheckpointId,
        aSucceeded,
        valid,
        sent);
}

void CampaignRecoveryService::OnRecoverySnapshot(
    const CampaignRecoverySnapshot& acSnapshot) noexcept
{
    if (!acSnapshot.IsValid())
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_REJECTED reason=malformed-packet");
        return;
    }
    CampaignRecoveryCorrelation correlation = m_state.GetCorrelation();
    correlation.RestoreRevision = acSnapshot.RestoreRevision;
    if (correlation.CampaignId != acSnapshot.CampaignId.c_str() ||
        correlation.RestoreAttemptId !=
            acSnapshot.RestoreAttemptId.c_str() ||
        correlation.CheckpointId != acSnapshot.CheckpointId.c_str())
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_REJECTED campaign={} attempt={} checkpoint={} restoreRevision={} reason=correlation-mismatch expectedCampaign={} expectedAttempt={} expectedCheckpoint={}",
            acSnapshot.CampaignId.c_str(),
            acSnapshot.RestoreAttemptId.c_str(),
            acSnapshot.CheckpointId.c_str(),
            acSnapshot.RestoreRevision,
            correlation.CampaignId,
            correlation.RestoreAttemptId,
            correlation.CheckpointId);
        return;
    }

    const CampaignRecoveryClientAction action =
        m_state.ObserveSnapshot(correlation);
    spdlog::info(
        "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_RECEIVED campaign={} attempt={} checkpoint={} restoreRevision={} action={}",
        correlation.CampaignId,
        correlation.RestoreAttemptId,
        correlation.CheckpointId,
        correlation.RestoreRevision,
        RecoveryActionName(action));
    if (action == CampaignRecoveryClientAction::ResendSnapshotApplied)
    {
        SendSnapshotApplied();
        return;
    }
    if (action != CampaignRecoveryClientAction::ApplySnapshot)
        return;
    if (!m_campaignService.ApplyRecoverySnapshot(acSnapshot.Snapshot))
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] RESTORE_SNAPSHOT_REJECTED campaign={} attempt={} checkpoint={} restoreRevision={} reason=client-apply-rejected",
            correlation.CampaignId,
            correlation.RestoreAttemptId,
            correlation.CheckpointId,
            correlation.RestoreRevision);
        return;
    }
    if (m_state.FinishSnapshotApply())
    {
        spdlog::info(
            "[STRE][CampaignRecovery] SNAPSHOT_APPLIED campaign={} attempt={} checkpoint={} restoreRevision={}",
            correlation.CampaignId,
            correlation.RestoreAttemptId,
            correlation.CheckpointId,
            correlation.RestoreRevision);
        SendSnapshotApplied();
    }
}

void CampaignRecoveryService::SendSnapshotApplied() noexcept
{
    const CampaignRecoveryCorrelation& correlation =
        m_state.GetCorrelation();
    CampaignRecoverySnapshotApplied applied;
    applied.CampaignId = correlation.CampaignId.c_str();
    applied.RestoreAttemptId = correlation.RestoreAttemptId.c_str();
    applied.CheckpointId = correlation.CheckpointId.c_str();
    applied.RestoreRevision = correlation.RestoreRevision;
    const bool valid = applied.IsValid();
    const bool sent = valid && m_transport.Send(applied);
    spdlog::log(
        sent ? spdlog::level::info : spdlog::level::warn,
        "[STRE][CampaignRecovery] APPLIED_RESULT_SENT campaign={} attempt={} checkpoint={} restoreRevision={} valid={} sent={}",
        correlation.CampaignId,
        correlation.RestoreAttemptId,
        correlation.CheckpointId,
        correlation.RestoreRevision,
        valid,
        sent);
}

void CampaignRecoveryService::OnRecoveryComplete(
    const CampaignRecoveryComplete& acComplete) noexcept
{
    if (!acComplete.IsValid())
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] RECOVERY_COMPLETION_REJECTED reason=malformed-packet");
        return;
    }
    CampaignRecoveryCorrelation correlation = m_state.GetCorrelation();
    correlation.RestoreRevision = acComplete.RestoreRevision;
    if (correlation.CampaignId != acComplete.CampaignId.c_str() ||
        correlation.RestoreAttemptId !=
            acComplete.RestoreAttemptId.c_str() ||
        correlation.CheckpointId != acComplete.CheckpointId.c_str())
    {
        spdlog::warn(
            "[STRE][CampaignRecovery] RECOVERY_COMPLETION_REJECTED campaign={} attempt={} checkpoint={} restoreRevision={} reason=correlation-mismatch expectedCampaign={} expectedAttempt={} expectedCheckpoint={}",
            acComplete.CampaignId.c_str(),
            acComplete.RestoreAttemptId.c_str(),
            acComplete.CheckpointId.c_str(),
            acComplete.RestoreRevision,
            correlation.CampaignId,
            correlation.RestoreAttemptId,
            correlation.CheckpointId);
        return;
    }
    const CampaignRecoveryClientAction action =
        m_state.ObserveComplete(correlation);
    spdlog::info(
        "[STRE][CampaignRecovery] RECOVERY_COMPLETION_RECEIVED campaign={} attempt={} checkpoint={} restoreRevision={} action={}",
        correlation.CampaignId,
        correlation.RestoreAttemptId,
        correlation.CheckpointId,
        correlation.RestoreRevision,
        RecoveryActionName(action));
    if (action != CampaignRecoveryClientAction::Release)
    {
        return;
    }
    if (m_gate.IsLocked())
        (void)m_nativeLoad.ReleaseForRecovery();
    m_campaignService.SetResumeRequiresCheckpointRestore(false);
    m_protectOnDisconnect = true;
    spdlog::info(
        "[STRE][CampaignRecovery] LOCAL_RECOVERY_COMPLETE campaign={} attempt={} checkpoint={} restoreRevision={}",
        correlation.CampaignId,
        correlation.RestoreAttemptId,
        correlation.CheckpointId,
        correlation.RestoreRevision);
}
