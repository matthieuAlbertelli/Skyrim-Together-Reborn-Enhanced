#include <CampaignContinueLoad.h>

#include <utility>

namespace STRE::Campaign
{
bool CampaignContinueResumeTransition::Begin(
    std::string aNativeSaveIdentity) noexcept
{
    Cancel();
    if (aNativeSaveIdentity.empty())
        return false;
    m_nativeSaveIdentity = std::move(aNativeSaveIdentity);
    m_phase = CampaignContinueResumePhase::PendingNativeTransition;
    return true;
}

bool CampaignContinueResumeTransition::ObserveNativeResult(
    bool aAccepted) noexcept
{
    if (m_phase != CampaignContinueResumePhase::PendingNativeTransition)
        return false;
    if (!aAccepted)
    {
        Cancel();
        return false;
    }
    m_phase = CampaignContinueResumePhase::NativeTransitionAccepted;
    return true;
}

std::optional<std::string>
CampaignContinueResumeTransition::CommitMainMenuClosed() noexcept
{
    if (m_phase != CampaignContinueResumePhase::NativeTransitionAccepted)
        return std::nullopt;
    std::string identity = std::move(m_nativeSaveIdentity);
    Cancel();
    return identity;
}

void CampaignContinueResumeTransition::Cancel() noexcept
{
    m_nativeSaveIdentity.clear();
    m_phase = CampaignContinueResumePhase::Idle;
}

void CampaignContinueLoadAttempt::Begin(
    std::uintptr_t aRootRequest) noexcept
{
    m_rootRequest = aRootRequest;
    m_targetClaimed = false;
}

std::optional<CampaignContinueLoadClaim>
CampaignContinueLoadAttempt::ClaimTarget(
    std::uintptr_t aRootRequest,
    std::uintptr_t aParentRequest,
    std::uintptr_t aRequest,
    bool aExactLoadRequestType,
    std::string_view acNativeTarget,
    bool aTargetReadable) noexcept
{
    if (!m_rootRequest || aRootRequest != m_rootRequest ||
        aParentRequest != m_rootRequest || !aRequest ||
        aRequest == m_rootRequest || !aExactLoadRequestType ||
        m_targetClaimed)
    {
        return std::nullopt;
    }

    m_targetClaimed = true;
    CampaignContinueLoadClaim claim;
    claim.TargetReadable = aTargetReadable &&
        NormalizeSkyrimNativeSaveIdentity(
            acNativeTarget, claim.NativeSaveIdentity);
    return claim;
}

void CampaignContinueLoadAttempt::Complete(
    std::uintptr_t aRootRequest) noexcept
{
    if (aRootRequest != m_rootRequest)
        return;
    m_rootRequest = 0;
    m_targetClaimed = false;
}

bool CampaignContinueLoadAttempt::IsActive(
    std::uintptr_t aRootRequest) const noexcept
{
    return aRootRequest != 0 && aRootRequest == m_rootRequest &&
        !m_targetClaimed;
}

bool NormalizeSkyrimNativeSaveIdentity(
    std::string_view acNativeTarget,
    std::string& aNativeSaveIdentity) noexcept
{
    constexpr std::string_view cEssExtension = ".ess";
    aNativeSaveIdentity.clear();
    if (acNativeTarget.size() <= cEssExtension.size() ||
        !acNativeTarget.ends_with(cEssExtension) ||
        acNativeTarget.find_first_of("/\\") != std::string_view::npos)
    {
        return false;
    }

    acNativeTarget.remove_suffix(cEssExtension.size());
    if (acNativeTarget.empty())
        return false;
    aNativeSaveIdentity.assign(acNativeTarget);
    return true;
}

CampaignLoadDecision EvaluateCampaignContinueLoad(
    const CampaignContinueLoadClaim& acClaim,
    CampaignLoadTarget aClassifiedTarget,
    bool aCampaignRuntimeSensitive) noexcept
{
    CampaignLoadPolicyContext context;
    context.Target = acClaim.TargetReadable
        ? aClassifiedTarget
        : CampaignLoadTarget::Unknown;
    context.Authority = CampaignLoadAuthority::Player;
    context.ExactInternalRecoveryCorrelation = false;
    context.CampaignRuntimeSensitive = aCampaignRuntimeSensitive;
    context.SemanticTargetProofRequired = !acClaim.TargetReadable;
    context.ReservedCampaignNamespaceClaim =
        acClaim.TargetReadable &&
        acClaim.NativeSaveIdentity.starts_with("stre-");
    return EvaluateCampaignLoadPolicy(context);
}
}
