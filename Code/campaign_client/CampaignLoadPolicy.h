#pragma once

#include <cstdint>

namespace STRE::Campaign
{
enum class CampaignLoadTarget : std::uint8_t
{
    Ordinary,
    Campaign,
    Unknown
};

enum class CampaignLoadAuthority : std::uint8_t
{
    Player,
    InternalRecovery
};

enum class CampaignLoadDecision : std::uint8_t
{
    AllowVanilla,
    AllowInternalRecovery,
    BeginResumeRequired,
    BlockPlayerLoad,
    BlockUnprovenCampaignTarget
};

struct CampaignLoadPolicyContext
{
    CampaignLoadTarget Target{CampaignLoadTarget::Unknown};
    CampaignLoadAuthority Authority{CampaignLoadAuthority::Player};
    bool ExactInternalRecoveryCorrelation{};
    bool CampaignRuntimeSensitive{};
    // A semantic boundary such as LoadMostRecent cannot safely continue when
    // it promises a target but that target could not be observed.
    bool SemanticTargetProofRequired{};
    // A reserved STRE-looking name is never positive authority. It only makes
    // a missing/corrupt marker fail closed instead of entering vanilla load.
    bool ReservedCampaignNamespaceClaim{};
};

[[nodiscard]] CampaignLoadDecision EvaluateCampaignLoadPolicy(
    const CampaignLoadPolicyContext& acContext) noexcept;
}
