#include <CampaignLoadPolicy.h>

namespace STRE::Campaign
{
CampaignLoadDecision EvaluateCampaignLoadPolicy(
    const CampaignLoadPolicyContext& acContext) noexcept
{
    if (acContext.Authority == CampaignLoadAuthority::InternalRecovery)
    {
        return acContext.ExactInternalRecoveryCorrelation
            ? CampaignLoadDecision::AllowInternalRecovery
            : CampaignLoadDecision::BlockUnprovenCampaignTarget;
    }

    if (acContext.CampaignRuntimeSensitive)
        return CampaignLoadDecision::BlockPlayerLoad;

    if (acContext.SemanticTargetProofRequired &&
        acContext.Target == CampaignLoadTarget::Unknown)
    {
        return CampaignLoadDecision::BlockUnprovenCampaignTarget;
    }

    if (acContext.Target == CampaignLoadTarget::Campaign)
        return CampaignLoadDecision::BeginResumeRequired;

    if (acContext.ReservedCampaignNamespaceClaim)
        return CampaignLoadDecision::BlockUnprovenCampaignTarget;

    return CampaignLoadDecision::AllowVanilla;
}
}
