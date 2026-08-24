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
    "The native save request slot owns one request through processing",
    "[campaign.checkpoint][native-save]")
{
    using CampaignNativeSaveDetail::RequestSlot;
    using CampaignNativeSaveDetail::RequestSlotState;

    RequestSlot slot;
    REQUIRE(slot.GetState() == RequestSlotState::Idle);
    REQUIRE_FALSE(slot.TryRequest({}));
    REQUIRE(slot.TryRequest("stre-checkpoint_A-42"));
    REQUIRE(slot.GetState() == RequestSlotState::Requested);
    REQUIRE(slot.RequestedIdentity());
    REQUIRE(*slot.RequestedIdentity() == "stre-checkpoint_A-42");
    REQUIRE_FALSE(slot.TryRequest("stre-duplicate"));

    const std::string* const processingIdentity = slot.BeginProcessing();
    REQUIRE(processingIdentity);
    REQUIRE(*processingIdentity == "stre-checkpoint_A-42");
    REQUIRE(slot.GetState() == RequestSlotState::Processing);
    REQUIRE_FALSE(slot.BeginProcessing());
    REQUIRE_FALSE(slot.TryRequest("stre-while-processing"));

    slot.FinishProcessing();
    REQUIRE(slot.GetState() == RequestSlotState::Idle);
    REQUIRE_FALSE(slot.RequestedIdentity());
    REQUIRE(slot.TryRequest("stre-next"));
}
