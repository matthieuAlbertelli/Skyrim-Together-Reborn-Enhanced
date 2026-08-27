#include <CampaignContinueLoad.h>
#include <CampaignLoadUiProjection.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;

namespace
{
constexpr std::uintptr_t cRoot = 0x1000;
constexpr std::uintptr_t cLoadRequest = 0x2000;

struct ContinueDispatchProbe
{
    unsigned NativeDispatches{};
    unsigned ResumePendingBegins{};
    unsigned GateActivations{};
    unsigned MainMenuRebuilds{};

    void Apply(CampaignLoadDecision aDecision)
    {
        if (aDecision == CampaignLoadDecision::AllowVanilla)
        {
            ++NativeDispatches;
        }
        else if (aDecision == CampaignLoadDecision::BeginResumeRequired)
        {
            ++ResumePendingBegins;
            ++NativeDispatches;
        }
        else
        {
            REQUIRE(ProjectCampaignLoadUiAction(
                        aDecision, CampaignLoadUiHost::MainMenu) ==
                CampaignLoadUiAction::ConsumeAndRebuildMainMenu);
            ++MainMenuRebuilds;
        }
    }
};
}

TEST_CASE(
    "Skyrim Continue target normalization removes exactly one ess suffix",
    "[campaign.load-policy][continue][identity]")
{
    std::string identity;
    REQUIRE(NormalizeSkyrimNativeSaveIdentity(
        "stre-checkpoint-0c36d46b7fd5d3d49d59f50435777582.ess",
        identity));
    REQUIRE(identity ==
        "stre-checkpoint-0c36d46b7fd5d3d49d59f50435777582");

    REQUIRE(NormalizeSkyrimNativeSaveIdentity(
        "Save 42 - Whiterun.ess", identity));
    REQUIRE(identity == "Save 42 - Whiterun");

    for (const std::string_view malformed : {
             "", ".ess", "save", "save.skse", "folder/save.ess",
             "folder\\save.ess"})
    {
        REQUIRE_FALSE(NormalizeSkyrimNativeSaveIdentity(
            malformed, identity));
        REQUIRE(identity.empty());
    }
}

TEST_CASE(
    "Only the exact first LoadRequest child claims a Continue target",
    "[campaign.load-policy][continue][provenance]")
{
    CampaignContinueLoadAttempt attempt;
    attempt.Begin(cRoot);

    // An identical native operation code is irrelevant without the exact
    // callback root and pointer lineage supplied to this state.
    REQUIRE_FALSE(attempt.ClaimTarget(
        0x9999, 0x9999, cLoadRequest, true, "save.ess", true));
    REQUIRE_FALSE(attempt.ClaimTarget(
        cRoot, 0x9999, cLoadRequest, true, "save.ess", true));
    REQUIRE_FALSE(attempt.ClaimTarget(
        cRoot, cRoot, cLoadRequest, false, "save.ess", true));

    const auto claim = attempt.ClaimTarget(
        cRoot, cRoot, cLoadRequest, true, "save.ess", true);
    REQUIRE(claim);
    REQUIRE(claim->TargetReadable);
    REQUIRE(claim->NativeSaveIdentity == "save");

    // Requeue or another derived request in the same attempt cannot repeat the
    // semantic decision or ResumeRequired ownership.
    REQUIRE_FALSE(attempt.ClaimTarget(
        cRoot, cRoot, cLoadRequest, true, "save.ess", true));
    REQUIRE_FALSE(attempt.ClaimTarget(
        cRoot, cRoot, 0x3000, true, "save.ess", true));

    attempt.Complete(cRoot);
    REQUIRE_FALSE(attempt.IsActive(cRoot));
    REQUIRE_FALSE(attempt.ClaimTarget(
        cRoot, cRoot, cLoadRequest, true, "save.ess", true));
}

TEST_CASE(
    "A correlated Continue dispatches ordinary and marked targets once",
    "[campaign.load-policy][continue][dispatch]")
{
    SECTION("ordinary target stays vanilla")
    {
        CampaignContinueLoadAttempt attempt;
        ContinueDispatchProbe probe;
        attempt.Begin(cRoot);
        const auto claim = attempt.ClaimTarget(
            cRoot, cRoot, cLoadRequest, true, "Save 42.ess", true);
        REQUIRE(claim);
        const CampaignLoadDecision decision =
            EvaluateCampaignContinueLoad(
                *claim, CampaignLoadTarget::Ordinary, false);
        REQUIRE(decision == CampaignLoadDecision::AllowVanilla);
        probe.Apply(decision);
        REQUIRE(probe.NativeDispatches == 1);
        REQUIRE(probe.ResumePendingBegins == 0);
        REQUIRE(probe.GateActivations == 0);
        REQUIRE(probe.MainMenuRebuilds == 0);
    }

    SECTION("marked target arms resume and keeps Skyrim native load")
    {
        CampaignContinueLoadAttempt attempt;
        ContinueDispatchProbe probe;
        attempt.Begin(cRoot);
        const auto claim = attempt.ClaimTarget(
            cRoot, cRoot, cLoadRequest, true,
            "stre-checkpoint-42.ess", true);
        REQUIRE(claim);
        const CampaignLoadDecision decision =
            EvaluateCampaignContinueLoad(
                *claim, CampaignLoadTarget::Campaign, false);
        REQUIRE(decision == CampaignLoadDecision::BeginResumeRequired);
        probe.Apply(decision);
        REQUIRE(probe.NativeDispatches == 1);
        REQUIRE(probe.ResumePendingBegins == 1);
        REQUIRE(probe.GateActivations == 0);

        REQUIRE_FALSE(attempt.ClaimTarget(
            cRoot, cRoot, cLoadRequest, true,
            "stre-checkpoint-42.ess", true));
        REQUIRE(probe.NativeDispatches == 1);
        REQUIRE(probe.ResumePendingBegins == 1);
        REQUIRE(probe.GateActivations == 0);
    }
}

TEST_CASE(
    "Unknown or campaign-sensitive Continue targets fail closed in Main Menu",
    "[campaign.load-policy][continue][fail-closed]")
{
    CampaignContinueLoadAttempt attempt;
    ContinueDispatchProbe probe;
    attempt.Begin(cRoot);
    const auto claim = attempt.ClaimTarget(
        cRoot, cRoot, cLoadRequest, true, "malformed-target", true);
    REQUIRE(claim);
    REQUIRE_FALSE(claim->TargetReadable);

    const CampaignLoadDecision decision = EvaluateCampaignContinueLoad(
        *claim, CampaignLoadTarget::Unknown, true);
    REQUIRE(decision == CampaignLoadDecision::BlockPlayerLoad);
    REQUIRE(decision != CampaignLoadDecision::AllowInternalRecovery);
    probe.Apply(decision);
    REQUIRE(probe.NativeDispatches == 0);
    REQUIRE(probe.ResumePendingBegins == 0);
    REQUIRE(probe.GateActivations == 0);
    REQUIRE(probe.MainMenuRebuilds == 1);
}

TEST_CASE(
    "Continue cannot acquire exact internal recovery authority",
    "[campaign.load-policy][continue][recovery]")
{
    CampaignContinueLoadAttempt continueAttempt;
    ContinueDispatchProbe unrelatedNativeRequest;
    REQUIRE_FALSE(continueAttempt.ClaimTarget(
        cRoot, cRoot, cLoadRequest, true,
        "stre-checkpoint-42.ess", true));
    // The native hook forwards an uncorrelated #56 request without consulting
    // Continue state.
    ++unrelatedNativeRequest.NativeDispatches;
    REQUIRE(unrelatedNativeRequest.NativeDispatches == 1);

    CampaignContinueLoadClaim claim{
        "stre-checkpoint-42", true};
    REQUIRE(EvaluateCampaignContinueLoad(
                claim, CampaignLoadTarget::Campaign, true) ==
        CampaignLoadDecision::BlockPlayerLoad);

    CampaignLoadPolicyContext internal;
    internal.Target = CampaignLoadTarget::Campaign;
    internal.Authority = CampaignLoadAuthority::InternalRecovery;
    internal.ExactInternalRecoveryCorrelation = true;
    internal.CampaignRuntimeSensitive = true;
    REQUIRE(EvaluateCampaignLoadPolicy(internal) ==
        CampaignLoadDecision::AllowInternalRecovery);
}

TEST_CASE(
    "Continue ResumeRequired remains logical until correlated MainMenuClosed",
    "[campaign.load-policy][continue][transition]")
{
    CampaignContinueResumeTransition transition;
    REQUIRE(transition.GetPhase() == CampaignContinueResumePhase::Idle);
    REQUIRE_FALSE(transition.CommitMainMenuClosed());

    REQUIRE(transition.Begin("stre-checkpoint-42"));
    REQUIRE(transition.GetPhase() ==
        CampaignContinueResumePhase::PendingNativeTransition);
    REQUIRE(transition.GetNativeSaveIdentity() == "stre-checkpoint-42");

    // No gate action exists in the pending phase while Main Menu owns input.
    ContinueDispatchProbe probe;
    REQUIRE(probe.GateActivations == 0);
    REQUIRE(transition.ObserveNativeResult(true));
    REQUIRE(transition.GetPhase() ==
        CampaignContinueResumePhase::NativeTransitionAccepted);
    REQUIRE(probe.GateActivations == 0);

    const auto identity = transition.CommitMainMenuClosed();
    REQUIRE(identity == "stre-checkpoint-42");
    ++probe.GateActivations;
    REQUIRE(probe.GateActivations == 1);
    REQUIRE(transition.GetPhase() == CampaignContinueResumePhase::Idle);
    REQUIRE_FALSE(transition.CommitMainMenuClosed());
    REQUIRE(probe.GateActivations == 1);
}

TEST_CASE(
    "Rejected or replaced Continue clears pending ResumeRequired ownership",
    "[campaign.load-policy][continue][transition][cleanup]")
{
    CampaignContinueResumeTransition transition;
    REQUIRE(transition.Begin("stre-checkpoint-rejected"));
    REQUIRE_FALSE(transition.ObserveNativeResult(false));
    REQUIRE(transition.GetPhase() == CampaignContinueResumePhase::Idle);
    REQUIRE(transition.GetNativeSaveIdentity().empty());
    REQUIRE_FALSE(transition.CommitMainMenuClosed());

    REQUIRE(transition.Begin("stre-checkpoint-first"));
    REQUIRE(transition.Begin("stre-checkpoint-second"));
    REQUIRE(transition.GetNativeSaveIdentity() ==
        "stre-checkpoint-second");
    transition.Cancel();
    REQUIRE(transition.GetPhase() == CampaignContinueResumePhase::Idle);
    REQUIRE_FALSE(transition.CommitMainMenuClosed());
}
