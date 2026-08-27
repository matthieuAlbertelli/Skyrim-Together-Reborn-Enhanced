#include <CampaignBootstrapState.h>
#include <CampaignResumeBridge.h>
#include <CampaignResumeState.h>
#include <Structs/Campaign.h>

#include <catch2/catch.hpp>

#include <algorithm>

using namespace STRE::Campaign;

namespace
{
constexpr const char* kTokenA = "00112233445566778899aabbccddeeff";
constexpr const char* kTokenB = "ffeeddccbbaa99887766554433221100";

CampaignResumeCandidate CandidateA()
{
    return {kTokenA, "campaign-a"};
}
}

TEST_CASE(
    "Campaign resume bridge exposes only bounded refresh and token selection",
    "[campaign.resume][cef]")
{
    REQUIRE(kCampaignResumeActionFunction ==
        std::string_view{"campaignResumeAction"});
    REQUIRE(kCampaignResumeMaximumArgumentCount == 2);
    REQUIRE(std::find(
                kCampaignResumeCefFunctions.begin(),
                kCampaignResumeCefFunctions.end(),
                kCampaignResumeActionFunction) !=
        kCampaignResumeCefFunctions.end());
    REQUIRE(ParseCampaignResumeAction("refresh") ==
        CampaignResumeAction::Refresh);
    REQUIRE(ParseCampaignResumeAction("retry") ==
        CampaignResumeAction::Retry);
    REQUIRE(ParseCampaignResumeAction("select") ==
        CampaignResumeAction::Select);
    REQUIRE(ParseCampaignResumeAction("stayAndRecover") ==
        CampaignResumeAction::StayAndRecover);
    REQUIRE(ParseCampaignResumeAction("returnToMainMenu") ==
        CampaignResumeAction::ReturnToMainMenu);
    REQUIRE(ParseCampaignResumeAction("resume") ==
        CampaignResumeAction::Unknown);
}

TEST_CASE(
    "Resume retry reuses only the already selected opaque campaign",
    "[campaign.resume][campaign.recovery][idempotency]")
{
    CampaignResumeState state;
    REQUIRE(state.ReplaceCandidates({CandidateA()}));
    REQUIRE(state.Select(kTokenA));
    state.Fail("recovery_failed");
    REQUIRE_FALSE(state.RetrySelected("campaign-b"));
    REQUIRE(state.GetPhase() == CampaignResumePhase::Error);
    REQUIRE(state.RetrySelected("campaign-a"));
    REQUIRE(state.GetPhase() == CampaignResumePhase::Submitting);
    REQUIRE(state.GetSelectedToken() == kTokenA);
    REQUIRE_FALSE(state.RetrySelected("campaign-a"));
}

TEST_CASE(
    "Cold-session resume candidates never select zero one or many implicitly",
    "[campaign.resume][reconnect][security]")
{
    CampaignResumeState zero;
    REQUIRE(zero.ReplaceCandidates({}));
    REQUIRE(zero.GetPhase() == CampaignResumePhase::Ready);
    REQUIRE(zero.GetCandidates().empty());
    REQUIRE(zero.GetSelectedToken().empty());

    CampaignResumeState one;
    REQUIRE(one.ReplaceCandidates({CandidateA()}));
    REQUIRE(one.GetCandidates().size() == 1);
    REQUIRE(one.GetSelectedToken().empty());
    REQUIRE(one.GetPhase() == CampaignResumePhase::Ready);

    CampaignResumeState many;
    REQUIRE(many.ReplaceCandidates({
        CandidateA(), {kTokenB, "campaign-b"}}));
    REQUIRE(many.GetCandidates().size() == 2);
    REQUIRE(many.GetSelectedToken().empty());
    REQUIRE(many.GetPhase() == CampaignResumePhase::Ready);
}

TEST_CASE(
    "Opaque candidate selection resolves the exact native campaign once",
    "[campaign.resume][security][idempotency]")
{
    CampaignResumeState state;
    REQUIRE(state.ReplaceCandidates({
        CandidateA(), {kTokenB, "campaign-b"}}));

    const auto selected = state.Select(kTokenB);
    REQUIRE(selected == "campaign-b");
    REQUIRE(state.GetSelectedToken() == kTokenB);
    REQUIRE(state.GetPhase() == CampaignResumePhase::Submitting);
    REQUIRE_FALSE(state.Select(kTokenB));
    REQUIRE_FALSE(state.Select(kTokenA));

    const auto& views = state.GetCandidates();
    REQUIRE(views[0].Token == kTokenA);
    REQUIRE(views[1].Token == kTokenB);
    REQUIRE(views[0].Ordinal == 1);
    REQUIRE(views[1].Ordinal == 2);
}

TEST_CASE(
    "Resume admission requires the matching authoritative acceptance",
    "[campaign.resume][campaign.admission][security]")
{
    CampaignResumeState state;
    REQUIRE(state.ReplaceCandidates({CandidateA()}));
    REQUIRE(state.Select(kTokenA) == "campaign-a");

    REQUIRE_FALSE(state.Accept("campaign-b"));
    REQUIRE(state.GetPhase() == CampaignResumePhase::Submitting);
    REQUIRE(state.Accept("campaign-a"));
    REQUIRE(state.GetPhase() == CampaignResumePhase::Admitted);

    state.Fail("connection_lost");
    REQUIRE(state.GetPhase() == CampaignResumePhase::Error);
    REQUIRE(state.Accept("campaign-a"));
    REQUIRE(state.GetPhase() == CampaignResumePhase::Admitted);
    REQUIRE(state.Accept("campaign-a"));
    REQUIRE(state.GetPhase() == CampaignResumePhase::Admitted);
}

TEST_CASE(
    "Binding and identity mismatch fail resume without admission",
    "[campaign.resume][campaign.admission][security]")
{
    for (const std::string error : {
             std::string{"binding_mismatch"},
             std::string{"identity_mismatch"}})
    {
        CampaignResumeState state;
        REQUIRE(state.ReplaceCandidates({CandidateA()}));
        REQUIRE(state.Select(kTokenA));
        REQUIRE(state.Reject("campaign-a", error));
        REQUIRE(state.GetPhase() == CampaignResumePhase::Error);
        REQUIRE(state.GetErrorCode() == error);
        REQUIRE_FALSE(state.Accept("campaign-a"));
        REQUIRE_FALSE(state.Reject("campaign-a", error));
    }
}

TEST_CASE(
    "Canonical resumed snapshot distinguishes roster wait recovery sync and active",
    "[campaign.resume][campaign.recovery]")
{
    CampaignResumeState waiting;
    REQUIRE(waiting.ReplaceCandidates({CandidateA()}));
    REQUIRE(waiting.Select(kTokenA));
    REQUIRE(waiting.Accept("campaign-a"));
    REQUIRE(waiting.ObserveRuntime(
        "campaign-a", kCampaignWireRuntimeActive, false, 2, 1));
    REQUIRE(waiting.GetPhase() == CampaignResumePhase::Admitted);
    REQUIRE(waiting.ObserveRuntime(
        "campaign-a", 0, true, 2, 1));
    REQUIRE(waiting.GetPhase() == CampaignResumePhase::WaitingForRoster);
    REQUIRE(waiting.ObserveRuntime(
        "campaign-a", kCampaignWireRuntimeActive, true, 2, 2));
    REQUIRE(waiting.GetPhase() == CampaignResumePhase::Active);

    CampaignResumeState recovery;
    REQUIRE(recovery.ReplaceCandidates({CandidateA()}));
    REQUIRE(recovery.Select(kTokenA));
    REQUIRE(recovery.Accept("campaign-a"));
    REQUIRE(recovery.ObserveRuntime(
        "campaign-a", kCampaignWireRuntimeRecoveryLock, true, 2, 2));
    REQUIRE(recovery.GetPhase() == CampaignResumePhase::Recovery);
    REQUIRE(recovery.ObserveRuntime(
        "campaign-a", kCampaignWireRuntimeRestoringCheckpoint, true, 2, 2));
    REQUIRE(recovery.GetPhase() == CampaignResumePhase::Synchronizing);
}

TEST_CASE(
    "Invalid resume candidates fail closed and cannot authorize creation",
    "[campaign.resume][campaign.bootstrap][security]")
{
    CampaignBootstrapState bootstrap;
    CampaignResumeState state;
    REQUIRE_FALSE(state.ReplaceCandidates({
        {"not-a-token", "campaign-a"}}));
    state.FailCache();
    REQUIRE(state.GetPhase() == CampaignResumePhase::Error);
    REQUIRE(state.GetCandidates().empty());
    REQUIRE_FALSE(state.Select(kTokenA));
    REQUIRE(bootstrap.GetPhase() == CampaignBootstrapPhase::Inactive);
    REQUIRE_FALSE(bootstrap.IsActive());
}

TEST_CASE(
    "Loaded campaign save state contains only its exact opaque target",
    "[campaign.resume][campaign.load][security]")
{
    CampaignResumeState state;
    REQUIRE(state.ReplaceCandidates({CandidateA()}));
    REQUIRE(state.GetCandidates().size() == 1);
    REQUIRE(state.GetCandidates()[0].Token == kTokenA);
    REQUIRE_FALSE(state.Select(kTokenB));
    REQUIRE(state.GetErrorCode() == "candidate_unavailable");
    REQUIRE(state.Select(kTokenA) == "campaign-a");
}

TEST_CASE(
    "ResumeRequired completion clears its target and returns to idle",
    "[campaign.resume][campaign.load][campaign.recovery][lifecycle]")
{
    CampaignResumeState state;
    REQUIRE(state.ReplaceCandidates({CandidateA()}));
    REQUIRE(state.Select(kTokenA) == "campaign-a");
    REQUIRE(state.Accept("campaign-a"));
    REQUIRE(state.ObserveRuntime(
        "campaign-a", kCampaignWireRuntimeRecoveryLock, true, 1, 1));
    REQUIRE(state.ObserveRuntime(
        "campaign-a", kCampaignWireRuntimeRestoringCheckpoint, true, 1, 1));
    REQUIRE(state.GetPhase() == CampaignResumePhase::Synchronizing);

    state.Complete();

    REQUIRE(state.GetPhase() == CampaignResumePhase::Unavailable);
    REQUIRE(state.GetCandidates().empty());
    REQUIRE(state.GetSelectedToken().empty());
    REQUIRE(state.GetErrorCode().empty());
    REQUIRE_FALSE(state.Select(kTokenA));

    REQUIRE(state.ReplaceCandidates({
        CandidateA(), {kTokenB, "campaign-b"}}));
    REQUIRE(state.GetPhase() == CampaignResumePhase::Ready);
    REQUIRE(state.GetCandidates().size() == 2);
    REQUIRE(state.GetSelectedToken().empty());
}

TEST_CASE(
    "ResumeRequired failure remains retryable until explicit completion",
    "[campaign.resume][campaign.load][campaign.recovery][lifecycle]")
{
    CampaignResumeState state;
    REQUIRE(state.ReplaceCandidates({CandidateA()}));
    REQUIRE(state.Select(kTokenA) == "campaign-a");
    REQUIRE(state.Accept("campaign-a"));
    state.Fail("recovery_failed");

    REQUIRE(state.GetPhase() == CampaignResumePhase::Error);
    REQUIRE(state.GetCandidates().size() == 1);
    REQUIRE(state.GetSelectedToken() == kTokenA);
    REQUIRE(state.RetrySelected("campaign-a"));
    REQUIRE(state.GetPhase() == CampaignResumePhase::Submitting);
}
