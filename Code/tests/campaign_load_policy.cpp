#include <CampaignLoadPolicy.h>
#include <CampaignLoadUiProjection.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;

TEST_CASE(
    "Campaign load policy allows only an exactly correlated internal recovery",
    "[campaign.load-policy][recovery]")
{
    CampaignLoadPolicyContext context;
    context.Target = CampaignLoadTarget::Campaign;
    context.Authority = CampaignLoadAuthority::InternalRecovery;
    context.CampaignRuntimeSensitive = true;

    REQUIRE(EvaluateCampaignLoadPolicy(context) ==
        CampaignLoadDecision::BlockUnprovenCampaignTarget);

    context.ExactInternalRecoveryCorrelation = true;
    REQUIRE(EvaluateCampaignLoadPolicy(context) ==
        CampaignLoadDecision::AllowInternalRecovery);
}

TEST_CASE(
    "Every player load is blocked while campaign runtime is sensitive",
    "[campaign.load-policy][runtime]")
{
    for (const CampaignLoadTarget target : {
             CampaignLoadTarget::Ordinary,
             CampaignLoadTarget::Campaign,
             CampaignLoadTarget::Unknown})
    {
        CampaignLoadPolicyContext context;
        context.Target = target;
        context.CampaignRuntimeSensitive = true;
        REQUIRE(EvaluateCampaignLoadPolicy(context) ==
            CampaignLoadDecision::BlockPlayerLoad);
    }
}

TEST_CASE(
    "A cold proven campaign target requires resume while ordinary load stays vanilla",
    "[campaign.load-policy][cold]")
{
    CampaignLoadPolicyContext campaign;
    campaign.Target = CampaignLoadTarget::Campaign;
    REQUIRE(EvaluateCampaignLoadPolicy(campaign) ==
        CampaignLoadDecision::BeginResumeRequired);

    CampaignLoadPolicyContext ordinary;
    ordinary.Target = CampaignLoadTarget::Ordinary;
    REQUIRE(EvaluateCampaignLoadPolicy(ordinary) ==
        CampaignLoadDecision::AllowVanilla);
}

TEST_CASE(
    "Unknown sensitive and unproven reserved targets fail closed",
    "[campaign.load-policy][fail-closed]")
{
    CampaignLoadPolicyContext sensitive;
    sensitive.Target = CampaignLoadTarget::Unknown;
    sensitive.CampaignRuntimeSensitive = true;
    REQUIRE(EvaluateCampaignLoadPolicy(sensitive) ==
        CampaignLoadDecision::BlockPlayerLoad);

    CampaignLoadPolicyContext reserved;
    reserved.Target = CampaignLoadTarget::Unknown;
    reserved.ReservedCampaignNamespaceClaim = true;
    REQUIRE(EvaluateCampaignLoadPolicy(reserved) ==
        CampaignLoadDecision::BlockUnprovenCampaignTarget);

    CampaignLoadPolicyContext unresolvedSemanticTarget;
    unresolvedSemanticTarget.Target = CampaignLoadTarget::Unknown;
    unresolvedSemanticTarget.SemanticTargetProofRequired = true;
    REQUIRE(EvaluateCampaignLoadPolicy(unresolvedSemanticTarget) ==
        CampaignLoadDecision::BlockUnprovenCampaignTarget);

    CampaignLoadPolicyContext outsideUnknown;
    outsideUnknown.Target = CampaignLoadTarget::Unknown;
    REQUIRE(EvaluateCampaignLoadPolicy(outsideUnknown) ==
        CampaignLoadDecision::AllowVanilla);
}

TEST_CASE(
    "A stre-like filename alone never gains internal load authority",
    "[campaign.load-policy][authority]")
{
    CampaignLoadPolicyContext context;
    context.Target = CampaignLoadTarget::Unknown;
    context.ReservedCampaignNamespaceClaim = true;
    context.Authority = CampaignLoadAuthority::Player;
    REQUIRE(EvaluateCampaignLoadPolicy(context) ==
        CampaignLoadDecision::BlockUnprovenCampaignTarget);
    REQUIRE(EvaluateCampaignLoadPolicy(context) !=
        CampaignLoadDecision::AllowInternalRecovery);
}

TEST_CASE(
    "Manual load semantic enforcement reuses the common load policy",
    "[campaign.load-policy][manual-boundary]")
{
    SECTION("authoritative admission consumes the confirmation early")
    {
        CampaignLoadPolicyContext context;
        context.Target = CampaignLoadTarget::Ordinary;
        context.CampaignRuntimeSensitive = true;
        const CampaignLoadDecision decision =
            EvaluateCampaignLoadPolicy(context);
        REQUIRE(decision == CampaignLoadDecision::BlockPlayerLoad);
        REQUIRE(ProjectCampaignLoadUiAction(decision) ==
            CampaignLoadUiAction::ConsumeAndCloseJournal);
    }

    SECTION("one blocked confirmation leaves no state for the next callback")
    {
        CampaignLoadPolicyContext admitted;
        admitted.Target = CampaignLoadTarget::Unknown;
        admitted.CampaignRuntimeSensitive = true;
        const CampaignLoadDecision blocked =
            EvaluateCampaignLoadPolicy(admitted);
        REQUIRE(blocked == CampaignLoadDecision::BlockPlayerLoad);
        REQUIRE(ProjectCampaignLoadUiAction(blocked) ==
            CampaignLoadUiAction::ConsumeAndCloseJournal);

        CampaignLoadPolicyContext outsideCampaign;
        outsideCampaign.Target = CampaignLoadTarget::Ordinary;
        const CampaignLoadDecision allowed =
            EvaluateCampaignLoadPolicy(outsideCampaign);
        REQUIRE(allowed == CampaignLoadDecision::AllowVanilla);
        REQUIRE(ProjectCampaignLoadUiAction(allowed) ==
            CampaignLoadUiAction::ForwardNative);
    }

    SECTION("the final native boundary still blocks an early-seam bypass")
    {
        CampaignLoadPolicyContext bypassedEarlySeam;
        bypassedEarlySeam.Target = CampaignLoadTarget::Campaign;
        bypassedEarlySeam.CampaignRuntimeSensitive = true;
        REQUIRE(EvaluateCampaignLoadPolicy(bypassedEarlySeam) ==
            CampaignLoadDecision::BlockPlayerLoad);
    }

    SECTION("a cold marked checkpoint continues toward resume required")
    {
        CampaignLoadPolicyContext coldCampaign;
        coldCampaign.Target = CampaignLoadTarget::Campaign;
        const CampaignLoadDecision decision =
            EvaluateCampaignLoadPolicy(coldCampaign);
        REQUIRE(decision == CampaignLoadDecision::BeginResumeRequired);
        REQUIRE(ProjectCampaignLoadUiAction(decision) ==
            CampaignLoadUiAction::ForwardNative);
    }

    SECTION("a vanilla load outside campaign remains vanilla")
    {
        CampaignLoadPolicyContext vanilla;
        vanilla.Target = CampaignLoadTarget::Ordinary;
        const CampaignLoadDecision decision =
            EvaluateCampaignLoadPolicy(vanilla);
        REQUIRE(decision == CampaignLoadDecision::AllowVanilla);
        REQUIRE(ProjectCampaignLoadUiAction(decision) ==
            CampaignLoadUiAction::ForwardNative);
    }

    SECTION("the exact internal recovery remains authorized")
    {
        CampaignLoadPolicyContext recovery;
        recovery.Target = CampaignLoadTarget::Campaign;
        recovery.Authority = CampaignLoadAuthority::InternalRecovery;
        recovery.ExactInternalRecoveryCorrelation = true;
        recovery.CampaignRuntimeSensitive = true;
        const CampaignLoadDecision decision =
            EvaluateCampaignLoadPolicy(recovery);
        REQUIRE(decision == CampaignLoadDecision::AllowInternalRecovery);
        REQUIRE(ProjectCampaignLoadUiAction(decision) ==
            CampaignLoadUiAction::ForwardNative);
    }
}

TEST_CASE(
    "Manual load UI projection closes all fail-closed player confirmations",
    "[campaign.load-policy][manual-boundary][ui]")
{
    REQUIRE(ProjectCampaignLoadUiAction(
                CampaignLoadDecision::BlockPlayerLoad) ==
        CampaignLoadUiAction::ConsumeAndCloseJournal);
    REQUIRE(ProjectCampaignLoadUiAction(
                CampaignLoadDecision::BlockUnprovenCampaignTarget) ==
        CampaignLoadUiAction::ConsumeAndCloseJournal);

    for (const CampaignLoadDecision forwarded : {
             CampaignLoadDecision::AllowVanilla,
             CampaignLoadDecision::AllowInternalRecovery,
             CampaignLoadDecision::BeginResumeRequired})
    {
        REQUIRE(ProjectCampaignLoadUiAction(forwarded) ==
            CampaignLoadUiAction::ForwardNative);
    }
}

TEST_CASE(
    "Main Menu blocked-load fallback rebuilds only the owning menu",
    "[campaign.load-policy][manual-boundary][main-menu][ui]")
{
    for (const CampaignLoadDecision blocked : {
             CampaignLoadDecision::BlockPlayerLoad,
             CampaignLoadDecision::BlockUnprovenCampaignTarget})
    {
        REQUIRE(ProjectCampaignLoadUiAction(
                    blocked, CampaignLoadUiHost::MainMenu) ==
            CampaignLoadUiAction::ConsumeAndRebuildMainMenu);
        REQUIRE(ProjectCampaignLoadUiAction(
                    blocked, CampaignLoadUiHost::Journal) ==
            CampaignLoadUiAction::ConsumeAndCloseJournal);
    }

    // Projection is stateless: rebuilding a Main Menu cannot leak ownership
    // into the next in-world Journal callback.
    REQUIRE(ProjectCampaignLoadUiAction(
                CampaignLoadDecision::BlockPlayerLoad,
                CampaignLoadUiHost::Journal) ==
        CampaignLoadUiAction::ConsumeAndCloseJournal);

    for (const CampaignLoadDecision forwarded : {
             CampaignLoadDecision::AllowVanilla,
             CampaignLoadDecision::AllowInternalRecovery,
             CampaignLoadDecision::BeginResumeRequired})
    {
        REQUIRE(ProjectCampaignLoadUiAction(
                    forwarded, CampaignLoadUiHost::MainMenu) ==
            CampaignLoadUiAction::ForwardNative);
    }
}
