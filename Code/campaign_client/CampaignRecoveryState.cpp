#include <CampaignRecoveryState.h>

#include <utility>

namespace STRE::Campaign
{
bool CampaignRecoveryState::Lock(std::string aCampaignId) noexcept
{
    return LockInternal(std::move(aCampaignId), true);
}

bool CampaignRecoveryState::LockProvisional(
    std::string aCampaignId) noexcept
{
    return LockInternal(std::move(aCampaignId), false);
}

bool CampaignRecoveryState::LockInternal(
    std::string aCampaignId,
    bool aAuthoritative) noexcept
{
    if (aCampaignId.empty())
        return false;
    if (m_stage != CampaignClientRecoveryStage::Idle &&
        m_stage != CampaignClientRecoveryStage::Completed &&
        m_correlation.CampaignId != aCampaignId)
    {
        return false;
    }
    if (m_stage == CampaignClientRecoveryStage::Idle ||
        m_stage == CampaignClientRecoveryStage::Completed)
    {
        m_correlation = {};
        m_correlation.CampaignId = std::move(aCampaignId);
        m_stage = CampaignClientRecoveryStage::RecoveryLocked;
        m_authoritativeLock = aAuthoritative;
    }
    else if (aAuthoritative)
        m_authoritativeLock = true;
    return true;
}

CampaignRecoveryClientAction
CampaignRecoveryState::ObserveAuthoritativeActive(
    std::string_view acCampaignId) noexcept
{
    if (acCampaignId.empty() ||
        acCampaignId != m_correlation.CampaignId)
    {
        return CampaignRecoveryClientAction::Reject;
    }
    if (m_stage == CampaignClientRecoveryStage::SnapshotApplied &&
        !m_correlation.RestoreAttemptId.empty())
    {
        return CampaignRecoveryClientAction::ResendSnapshotApplied;
    }
    if (m_stage == CampaignClientRecoveryStage::RecoveryLocked &&
        m_correlation.RestoreAttemptId.empty() &&
        !m_authoritativeLock)
    {
        m_stage = CampaignClientRecoveryStage::Idle;
        m_correlation = {};
        m_authoritativeLock = false;
        return CampaignRecoveryClientAction::Release;
    }
    return CampaignRecoveryClientAction::Ignore;
}

bool CampaignRecoveryState::MatchesBase(
    const CampaignRecoveryCorrelation& acCorrelation) const noexcept
{
    return !acCorrelation.CampaignId.empty() &&
        !acCorrelation.RestoreAttemptId.empty() &&
        !acCorrelation.CheckpointId.empty() &&
        acCorrelation.CampaignId == m_correlation.CampaignId &&
        acCorrelation.RestoreAttemptId == m_correlation.RestoreAttemptId &&
        acCorrelation.CheckpointId == m_correlation.CheckpointId &&
        acCorrelation.SourceRevision == m_correlation.SourceRevision;
}

CampaignRecoveryClientAction CampaignRecoveryState::ObserveLoadRequest(
    CampaignRecoveryCorrelation aCorrelation) noexcept
{
    if (aCorrelation.CampaignId.empty() ||
        aCorrelation.RestoreAttemptId.empty() ||
        aCorrelation.CheckpointId.empty() ||
        aCorrelation.SourceRevision == 0 ||
        aCorrelation.RestoreRevision != 0 ||
        aCorrelation.CampaignId != m_correlation.CampaignId)
    {
        return CampaignRecoveryClientAction::Reject;
    }

    if (m_stage == CampaignClientRecoveryStage::RecoveryLocked)
    {
        m_correlation = std::move(aCorrelation);
        m_authoritativeLock = true;
        m_stage = CampaignClientRecoveryStage::LoadingNativeSave;
        return CampaignRecoveryClientAction::StartNativeLoad;
    }
    if (!MatchesBase(aCorrelation))
        return CampaignRecoveryClientAction::Reject;
    if (m_stage == CampaignClientRecoveryStage::Failed)
    {
        m_stage = CampaignClientRecoveryStage::LoadingNativeSave;
        return CampaignRecoveryClientAction::StartNativeLoad;
    }
    if (m_stage == CampaignClientRecoveryStage::LoadingNativeSave)
        return CampaignRecoveryClientAction::Ignore;
    if (m_stage == CampaignClientRecoveryStage::NativeSaveLoaded ||
        m_stage == CampaignClientRecoveryStage::SnapshotApplied)
        return CampaignRecoveryClientAction::ResendLoaded;
    return CampaignRecoveryClientAction::Reject;
}

bool CampaignRecoveryState::FinishNativeLoad(bool aSucceeded) noexcept
{
    if (m_stage != CampaignClientRecoveryStage::LoadingNativeSave)
        return false;
    m_stage = aSucceeded ? CampaignClientRecoveryStage::NativeSaveLoaded
                         : CampaignClientRecoveryStage::Failed;
    return true;
}

CampaignRecoveryClientAction CampaignRecoveryState::ObserveSnapshot(
    const CampaignRecoveryCorrelation& acCorrelation) noexcept
{
    if (!MatchesBase(acCorrelation) || acCorrelation.RestoreRevision == 0)
        return CampaignRecoveryClientAction::Reject;
    if (m_stage == CampaignClientRecoveryStage::NativeSaveLoaded)
    {
        m_correlation.RestoreRevision = acCorrelation.RestoreRevision;
        return CampaignRecoveryClientAction::ApplySnapshot;
    }
    if (m_stage == CampaignClientRecoveryStage::SnapshotApplied &&
        m_correlation.RestoreRevision == acCorrelation.RestoreRevision)
    {
        return CampaignRecoveryClientAction::ResendSnapshotApplied;
    }
    return CampaignRecoveryClientAction::Reject;
}

bool CampaignRecoveryState::FinishSnapshotApply() noexcept
{
    if (m_stage != CampaignClientRecoveryStage::NativeSaveLoaded ||
        m_correlation.RestoreRevision == 0)
    {
        return false;
    }
    m_stage = CampaignClientRecoveryStage::SnapshotApplied;
    return true;
}

CampaignRecoveryClientAction CampaignRecoveryState::ObserveComplete(
    const CampaignRecoveryCorrelation& acCorrelation) noexcept
{
    if (m_stage == CampaignClientRecoveryStage::Completed &&
        m_correlation == acCorrelation)
    {
        return CampaignRecoveryClientAction::Release;
    }
    if (m_stage != CampaignClientRecoveryStage::SnapshotApplied ||
        m_correlation != acCorrelation)
    {
        return CampaignRecoveryClientAction::Reject;
    }
    m_stage = CampaignClientRecoveryStage::Completed;
    return CampaignRecoveryClientAction::Release;
}
}
