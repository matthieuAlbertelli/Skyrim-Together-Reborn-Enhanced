#include <CampaignCheckpointClient.h>

namespace STRE::Campaign
{
CampaignCheckpointClient::CampaignCheckpointClient(
    CampaignIdentityStore& aStore) noexcept
    : m_store(aStore)
{
}

CampaignCheckpointClientAction CampaignCheckpointClient::HandleRequest(
    CampaignCheckpointClientRequest aRequest,
    const std::optional<CampaignClientAdmission>& acAdmission) noexcept
{
    CampaignCheckpointClientAction action;
    action.Request = aRequest;
    try
    {
        if (!acAdmission ||
            acAdmission->CampaignId != aRequest.CampaignId ||
            !CampaignIdentityStore::IsValidCacheId(aRequest.CampaignId) ||
            !CampaignIdentityStore::IsValidCacheId(aRequest.CheckpointId) ||
            aRequest.SourceRevision == 0 ||
            aRequest.NativeSaveIdentity != "stre-" + aRequest.CheckpointId)
        {
            return action;
        }
        if (m_active)
        {
            action.Kind = *m_active == aRequest
                ? CampaignCheckpointClientActionKind::Wait
                : CampaignCheckpointClientActionKind::SendFailure;
            return action;
        }

        auto cached = m_store.LoadCheckpointArtifact(
            aRequest.CampaignId, aRequest.CheckpointId);
        if (!cached)
            return action;

        m_active = aRequest;
        if (cached.Value)
        {
            action.Kind =
                CampaignCheckpointClientActionKind::ValidateExisting;
            action.ExpectedArtifact = std::move(*cached.Value);
        }
        else
        {
            action.Kind =
                CampaignCheckpointClientActionKind::StartNativeSave;
        }
        return action;
    }
    catch (...)
    {
        return action;
    }
}

std::optional<CampaignCheckpointClientResult>
CampaignCheckpointClient::CompleteNativeSave(
    const NativeSaveBundleArtifact& acArtifact) noexcept
{
    try
    {
        const NativeSaveBundleArtifactParseResult parsed =
            ParseNativeSaveBundleArtifact(
                acArtifact.Bundle.LogicalIdentity,
                acArtifact.Fingerprint,
                acArtifact.Metadata);
        if (!m_active ||
            acArtifact.Bundle.LogicalIdentity !=
                m_active->NativeSaveIdentity ||
            !parsed || parsed.Value != acArtifact)
        {
            return FailNativeSave();
        }
        const LocalStoreResult saved = m_store.SaveCheckpointArtifact(
            m_active->CampaignId, m_active->CheckpointId, acArtifact);
        if (!saved)
            return FailNativeSave();

        CampaignCheckpointClientResult result{
            *m_active, true, acArtifact};
        m_active.reset();
        return result;
    }
    catch (...)
    {
        return FailNativeSave();
    }
}

std::optional<CampaignCheckpointClientResult>
CampaignCheckpointClient::FailNativeSave() noexcept
{
    if (!m_active)
        return std::nullopt;
    CampaignCheckpointClientResult result{*m_active, false, std::nullopt};
    m_active.reset();
    return result;
}
}
