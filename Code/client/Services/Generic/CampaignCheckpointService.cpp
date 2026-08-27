#include <Services/CampaignCheckpointService.h>

#include <CampaignNativeSave.h>
#include <CampaignSavePolicy.h>
#include <Events/DisconnectedEvent.h>
#include <Events/UpdateEvent.h>
#include <Messages/CampaignMessages.h>
#include <Messages/CampaignRequests.h>
#include <Services/CampaignRuntimeGateService.h>
#include <Services/CampaignService.h>
#include <Services/OverlayService.h>
#include <Services/TransportService.h>

#include <OverlayApp.hpp>

using namespace STRE::Campaign;

namespace
{
CampaignCheckpointService* s_pCampaignCheckpointService{};
}

CampaignCheckpointService::CampaignCheckpointService(
    entt::dispatcher& aDispatcher,
    TransportService& aTransport,
    CampaignService& aCampaignService,
    CampaignRuntimeGateService& aRuntimeGate,
    OverlayService& aOverlay) noexcept
    : m_transport(aTransport)
    , m_campaignService(aCampaignService)
    , m_runtimeGate(aRuntimeGate)
    , m_overlay(aOverlay)
    , m_requestConnection(
          aDispatcher.sink<CampaignCheckpointSaveRequest>()
              .connect<&CampaignCheckpointService::OnCheckpointRequest>(this))
    , m_stateConnection(
          aDispatcher.sink<NotifyCampaignCheckpointState>()
              .connect<&CampaignCheckpointService::OnCheckpointState>(this))
    , m_disconnectedConnection(
          aDispatcher.sink<DisconnectedEvent>()
              .connect<&CampaignCheckpointService::OnDisconnected>(this))
    , m_updateConnection(
          aDispatcher.sink<UpdateEvent>()
              .connect<&CampaignCheckpointService::OnUpdate>(this))
{
    s_pCampaignCheckpointService = this;
    auto directory = CampaignIdentityStore::ResolveDefaultDirectory();
    if (!directory)
    {
        spdlog::error(
            "[STRE][CampaignCheckpoint] local artifact store unavailable: {}",
            directory.Message);
        return;
    }
    m_store = std::make_unique<CampaignIdentityStore>(
        std::move(directory.Value));
    m_client = std::make_unique<CampaignCheckpointClient>(*m_store);
}

CampaignCheckpointService::~CampaignCheckpointService() noexcept
{
    if (s_pCampaignCheckpointService == this)
        s_pCampaignCheckpointService = nullptr;
}

CampaignCheckpointService* CampaignCheckpointService::TryGet() noexcept
{
    return s_pCampaignCheckpointService;
}

CampaignSaveDecision CampaignCheckpointService::HandleNativeSaveAttempt(
    CampaignSaveOrigin aOrigin) noexcept
{
    const auto admission = m_campaignService.GetAdmission();
    const auto& snapshot = m_campaignService.GetLatestSnapshot();
    CampaignSaveRuntimeState runtime =
        CampaignSaveRuntimeState::Unavailable;
    if (snapshot && admission &&
        snapshot->CampaignId == admission->CampaignId.c_str())
    {
        switch (snapshot->RuntimeState)
        {
        case 0:
            runtime = CampaignSaveRuntimeState::WaitingForRoster;
            break;
        case kCampaignWireRuntimeActive:
            runtime = CampaignSaveRuntimeState::Active;
            break;
        case kCampaignWireRuntimeCheckpointing:
            runtime = CampaignSaveRuntimeState::Checkpointing;
            break;
        case kCampaignWireRuntimeRecoveryLock:
            runtime = CampaignSaveRuntimeState::RecoveryLock;
            break;
        case kCampaignWireRuntimeRestoringCheckpoint:
            runtime = CampaignSaveRuntimeState::RestoringCheckpoint;
            break;
        default:
            runtime = CampaignSaveRuntimeState::Unavailable;
            break;
        }
    }

    CampaignSavePolicyContext context;
    context.InCampaign = admission.has_value() || m_runtimeGate.IsLocked();
    context.RuntimeFenced = m_runtimeGate.IsLocked();
    context.RuntimeState = runtime;
    CampaignSaveDecision decision =
        EvaluateCampaignSavePolicy(aOrigin, context);
    if (decision == CampaignSaveDecision::RequestCollectiveCheckpoint &&
        m_intentPending)
    {
        decision = CampaignSaveDecision::CoalesceWithCheckpoint;
    }

    if (decision == CampaignSaveDecision::RequestCollectiveCheckpoint)
    {
        const auto reason = aOrigin == CampaignSaveOrigin::Quick
            ? CampaignCheckpointRequestReason::Quick
            : CampaignCheckpointRequestReason::Manual;
        if (!m_campaignService.RequestCheckpoint(reason))
        {
            Notify("COMPONENT.CAMPAIGN_SAVE.CHECKPOINT_FAILED");
            return CampaignSaveDecision::BlockUnavailable;
        }
        m_intentPending = true;
        m_checkpointStateToken.clear();
        Notify("COMPONENT.CAMPAIGN_SAVE.CHECKPOINT_REQUESTED");
    }
    else if (decision == CampaignSaveDecision::BlockAutosave)
    {
        Notify("COMPONENT.CAMPAIGN_SAVE.AUTOSAVE_BLOCKED");
    }
    else if (decision == CampaignSaveDecision::BlockUnavailable)
    {
        Notify("COMPONENT.CAMPAIGN_SAVE.UNAVAILABLE");
    }
    else if (decision == CampaignSaveDecision::BlockUnknown)
    {
        Notify("COMPONENT.CAMPAIGN_SAVE.UNKNOWN_BLOCKED");
    }
    return decision;
}

void CampaignCheckpointService::OnCheckpointRequest(
    const CampaignCheckpointSaveRequest& acRequest) noexcept
{
    if (!acRequest.IsValid())
    {
        spdlog::error(
            "[STRE][CampaignCheckpoint] ignored malformed save request");
        return;
    }
    CampaignCheckpointClientRequest request{
        acRequest.CampaignId.c_str(),
        acRequest.CheckpointId.c_str(),
        acRequest.SourceRevision,
        acRequest.NativeSaveIdentity.c_str()};
    if (!m_client)
    {
        SendResult({std::move(request), false, std::nullopt});
        return;
    }

    CampaignCheckpointClientAction action = m_client->HandleRequest(
        std::move(request), m_campaignService.GetAdmission());
    switch (action.Kind)
    {
    case CampaignCheckpointClientActionKind::Wait:
        spdlog::debug(
            "[STRE][CampaignCheckpoint] duplicate request is already processing checkpoint={}",
            action.Request.CheckpointId);
        return;
    case CampaignCheckpointClientActionKind::SendFailure:
        SendFailureForAction(action);
        return;
    case CampaignCheckpointClientActionKind::StartNativeSave:
    {
        const auto accepted = CampaignNativeSave::RequestOnGameThread(
            action.Request.NativeSaveIdentity);
        if (!accepted.WasAccepted())
        {
            if (const auto failed = m_client->FailNativeSave())
                SendResult(*failed);
        }
        return;
    }
    case CampaignCheckpointClientActionKind::ValidateExisting:
    {
        if (!action.ExpectedArtifact)
        {
            if (const auto failed = m_client->FailNativeSave())
                SendResult(*failed);
            return;
        }
        const auto accepted =
            CampaignNativeSave::ValidateExistingOnGameThread(
                action.Request.NativeSaveIdentity,
                *action.ExpectedArtifact);
        if (!accepted.WasAccepted())
        {
            if (const auto failed = m_client->FailNativeSave())
                SendResult(*failed);
        }
        return;
    }
    }
}

void CampaignCheckpointService::OnUpdate(const UpdateEvent&) noexcept
{
    PublishPolicyState();
    if (!m_client || !m_client->GetActiveRequest())
        return;
    const CampaignNativeSaveLifecycleSnapshot status =
        CampaignNativeSave::GetStatus();
    if (status.Identity !=
        m_client->GetActiveRequest()->NativeSaveIdentity)
    {
        if (status.State == CampaignNativeSaveLifecycleState::Idle)
            return;
        if (const auto failed = m_client->FailNativeSave())
            SendResult(*failed);
        return;
    }
    if (status.State == CampaignNativeSaveLifecycleState::Completed &&
        status.Artifact)
    {
        if (const auto completed =
                m_client->CompleteNativeSave(*status.Artifact))
        {
            const auto admission = m_campaignService.GetAdmission();
            if (!admission ||
                admission->CampaignId != completed->Request.CampaignId)
            {
                SendResult({completed->Request, false, std::nullopt});
                return;
            }
            const LocalStoreResult marked = m_store->SaveCampaignSaveMarker({
                admission->CampaignId,
                admission->CampaignSlotId,
                admission->CharacterBindingId,
                completed->Request.CheckpointId,
                completed->Request.NativeSaveIdentity});
            if (!marked)
            {
                spdlog::error(
                    "[STRE][CampaignCheckpoint] checkpoint save marker failed safely: {}",
                    marked.Message);
                SendResult({completed->Request, false, std::nullopt});
                return;
            }
            SendResult(*completed);
        }
    }
    else if (status.State == CampaignNativeSaveLifecycleState::Failed)
    {
        if (const auto failed = m_client->FailNativeSave())
            SendResult(*failed);
    }
}

void CampaignCheckpointService::OnCheckpointState(
    const NotifyCampaignCheckpointState& acState) noexcept
{
    const auto admission = m_campaignService.GetAdmission();
    if (!acState.IsValid() || !admission ||
        admission->CampaignId != acState.CampaignId.c_str())
    {
        return;
    }

    const std::string token = fmt::format(
        "{}:{}:{}", acState.CampaignId.c_str(),
        acState.CheckpointId.c_str(),
        static_cast<unsigned>(acState.State));
    if (token == m_checkpointStateToken)
        return;
    m_checkpointStateToken = token;

    if (acState.State == CampaignCheckpointPublicState::Started)
    {
        if (!m_intentPending)
            Notify("COMPONENT.CAMPAIGN_SAVE.CHECKPOINT_REQUESTED");
        m_intentPending = true;
    }
    else if (acState.State == CampaignCheckpointPublicState::Committed)
    {
        m_intentPending = false;
        Notify("COMPONENT.CAMPAIGN_SAVE.CHECKPOINT_COMMITTED");
    }
    else
    {
        m_intentPending = false;
        Notify("COMPONENT.CAMPAIGN_SAVE.CHECKPOINT_FAILED");
    }
}

void CampaignCheckpointService::OnDisconnected(
    const DisconnectedEvent&) noexcept
{
    m_intentPending = false;
    m_checkpointStateToken.clear();
}

void CampaignCheckpointService::PublishPolicyState() noexcept
{
    const bool inCampaign =
        m_campaignService.GetAdmission().has_value() ||
        m_runtimeGate.IsLocked();
    const std::string json = inCampaign
        ? "{\"inCampaign\":true}"
        : "{\"inCampaign\":false}";
    if (json == m_lastPolicyJson)
        return;

    auto* const pApp = m_overlay.GetOverlayApp();
    if (!pApp)
        return;
    auto arguments = CefListValue::Create();
    arguments->SetString(0, json);
    pApp->ExecuteAsync("campaignSavePolicyState", arguments);
    m_lastPolicyJson = json;
}

void CampaignCheckpointService::Notify(
    std::string_view acTranslationKey) noexcept
{
    try
    {
        m_overlay.SendSystemMessage(std::string(acTranslationKey));
    }
    catch (...)
    {
    }
}

void CampaignCheckpointService::SendFailureForAction(
    const CampaignCheckpointClientAction& acAction) noexcept
{
    SendResult({acAction.Request, false, std::nullopt});
}

void CampaignCheckpointService::SendResult(
    const CampaignCheckpointClientResult& acResult) noexcept
{
    CampaignCheckpointSaveResult message;
    message.CampaignId = acResult.Request.CampaignId.c_str();
    message.CheckpointId = acResult.Request.CheckpointId.c_str();
    message.NativeSaveIdentity =
        acResult.Request.NativeSaveIdentity.c_str();
    message.Result = acResult.Succeeded
        ? CampaignCheckpointSaveResultCode::Success
        : CampaignCheckpointSaveResultCode::Failure;
    if (acResult.Succeeded && acResult.Artifact)
    {
        message.FingerprintAlgorithm =
            kNativeSaveFingerprintAlgorithm.data();
        message.FingerprintVersion = kNativeSaveFingerprintVersion;
        message.Fingerprint.assign(
            acResult.Artifact->Fingerprint.begin(),
            acResult.Artifact->Fingerprint.end());
        message.SaveMetadataCodecVersion =
            kNativeSaveMetadataCodecVersion;
        message.SaveMetadata.assign(
            acResult.Artifact->Metadata.begin(),
            acResult.Artifact->Metadata.end());
    }
    const bool sent = message.IsValid() && m_transport.Send(message);
    spdlog::log(
        sent ? spdlog::level::info : spdlog::level::err,
        "[STRE][CampaignCheckpoint] SAVE_RESULT_SENT campaign={} checkpoint={} success={} sent={}",
        acResult.Request.CampaignId,
        acResult.Request.CheckpointId,
        acResult.Succeeded,
        sent);
}
