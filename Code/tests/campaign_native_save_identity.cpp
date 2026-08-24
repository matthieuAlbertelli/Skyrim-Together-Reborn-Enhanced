#include <Structs/Campaign.h>

#include "../client/Games/Skyrim/CampaignNativeSave.h"

#include <catch2/catch.hpp>

#include <string>
#include <string_view>

namespace
{
TiltedPhoques::String WireString(std::string_view acValue)
{
    TiltedPhoques::String result;
    result.assign(acValue.data(), acValue.size());
    return result;
}
}

TEST_CASE(
    "Checkpoint IDs derive one path-safe native save identity",
    "[campaign.checkpoint][native-save]")
{
    TiltedPhoques::String identity;
    REQUIRE(BuildCampaignNativeSaveIdentity(
        WireString("checkpoint_A-42"), identity));
    REQUIRE(identity == "stre-checkpoint_A-42");
    REQUIRE(IsValidCampaignNativeSaveIdentity(identity));
    REQUIRE(identity.find('/') == TiltedPhoques::String::npos);
    REQUIRE(identity.find('\\') == TiltedPhoques::String::npos);
    REQUIRE(identity.find("..") == TiltedPhoques::String::npos);
}

TEST_CASE(
    "Unsafe or malformed checkpoint IDs cannot become native save identities",
    "[campaign.checkpoint][native-save][security]")
{
    const std::string oversized(kCampaignWireMaximumIdLength + 1, 'a');
    const std::string_view invalidIds[]{
        "", "../escape", "folder/save", "folder\\save", "C:save",
        "checkpoint.ess", oversized};

    for (const std::string_view invalid : invalidIds)
    {
        TiltedPhoques::String identity = WireString("must-be-cleared");
        REQUIRE_FALSE(BuildCampaignNativeSaveIdentity(
            WireString(invalid), identity));
        REQUIRE(identity.empty());
    }

    REQUIRE_FALSE(IsValidCampaignNativeSaveIdentity(
        WireString("checkpoint_A-42")));
    REQUIRE_FALSE(IsValidCampaignNativeSaveIdentity(WireString("stre-")));
    REQUIRE_FALSE(IsValidCampaignNativeSaveIdentity(
        WireString("stre-../escape")));
}

TEST_CASE(
    "The native save request slot fails closed and resets for the next request",
    "[campaign.checkpoint][native-save]")
{
    using CampaignNativeSaveDetail::RequestSlot;
    using STRE::Campaign::BuildNativeSaveBundleArtifact;
    using STRE::Campaign::NativeSaveBundleMember;
    using STRE::Campaign::NativeSaveMemberRole;

    RequestSlot slot;
    REQUIRE(slot.Snapshot().State == CampaignNativeSaveLifecycleState::Idle);
    REQUIRE_FALSE(slot.TryRequest({}));
    REQUIRE(slot.TryRequest("stre-checkpoint_A-42"));
    REQUIRE(
        slot.Snapshot().State == CampaignNativeSaveLifecycleState::Requested);
    REQUIRE(slot.RequestedIdentity());
    REQUIRE(*slot.RequestedIdentity() == "stre-checkpoint_A-42");
    REQUIRE_FALSE(slot.TryRequest("stre-duplicate"));

    const std::optional<std::string> processingIdentity =
        slot.BeginProcessing();
    REQUIRE(processingIdentity);
    REQUIRE(*processingIdentity == "stre-checkpoint_A-42");
    REQUIRE(
        slot.Snapshot().State == CampaignNativeSaveLifecycleState::Processing);
    REQUIRE_FALSE(slot.BeginProcessing());
    REQUIRE_FALSE(slot.TryRequest("stre-while-processing"));
    REQUIRE(slot.Fail("native-save-returned-false"));
    REQUIRE(slot.Snapshot().State == CampaignNativeSaveLifecycleState::Failed);
    REQUIRE(
        slot.Snapshot().FailureReason == "native-save-returned-false");

    REQUIRE(slot.TryRequest("stre-next"));
    REQUIRE(slot.BeginProcessing());
    REQUIRE(slot.BeginAwaitingCompletion());
    REQUIRE_FALSE(slot.TryRequest("stre-while-awaiting"));
    REQUIRE(slot.Fail("completion-timeout"));

    REQUIRE(slot.TryRequest("stre-complete"));
    REQUIRE(slot.BeginProcessing());
    REQUIRE(slot.BeginAwaitingCompletion());
    auto artifact = BuildNativeSaveBundleArtifact(
        "stre-complete",
        {
            NativeSaveBundleMember{NativeSaveMemberRole::Skse, 20, {}},
            NativeSaveBundleMember{NativeSaveMemberRole::Ess, 4096, {}}
        });
    REQUIRE(artifact);
    REQUIRE(slot.Complete(std::move(artifact.Value)));
    const CampaignNativeSaveLifecycleSnapshot completed = slot.Snapshot();
    REQUIRE(completed.State == CampaignNativeSaveLifecycleState::Completed);
    REQUIRE(completed.Artifact);
    REQUIRE(completed.Artifact->Bundle.LogicalIdentity == "stre-complete");

    REQUIRE(slot.TryRequest("stre-after-completion"));
    REQUIRE_FALSE(slot.Snapshot().Artifact);
}
