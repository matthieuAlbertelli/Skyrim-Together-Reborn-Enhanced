#include <Services/CampaignCheckpointService.h>

#include <CampaignNativeSave.h>
#include <Events/UpdateEvent.h>
#include <Messages/CampaignMessages.h>
#include <Messages/CampaignRequests.h>
#include <Services/CampaignService.h>
#include <Services/TransportService.h>

using namespace STRE::Campaign;

CampaignCheckpointService::CampaignCheckpointService(
    entt::dispatcher& aDispatcher,
    TransportService& aTransport,
    CampaignService& aCampaignService) noexcept
    : m_transport(aTransport)
    , m_campaignService(aCampaignService)
    , m_requestConnection(
          aDispatcher.sink<CampaignCheckpointSaveRequest>()
              .connect<&CampaignCheckpointService::OnCheckpointRequest>(this))
    , m_updateConnection(
          aDispatcher.sink<UpdateEvent>()
              .connect<&CampaignCheckpointService::OnUpdate>(this))
{
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
            SendResult(*completed);
        }
    }
    else if (status.State == CampaignNativeSaveLifecycleState::Failed)
    {
        if (const auto failed = m_client->FailNativeSave())
            SendResult(*failed);
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
