#include <campaign_persistence_test_helpers.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

TEST_CASE("Campaign projections preserve audience filtering", "[campaign.persistence][privacy]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    REQUIRE(store->CreateCampaign(MakeCampaign()).Succeeded());

    auto publicProjection = store->LoadCampaignProjection(
        CampaignId{"campaign-1"}, ProjectionAudience::PublicOnly());
    REQUIRE(publicProjection.Succeeded());
    REQUIRE(publicProjection.Value.AdapterStates.size() == 1);
    REQUIRE(publicProjection.Value.AdapterStates.front().Audience == StateAudience::Public);

    auto ownerProjection = store->LoadCampaignProjection(
        CampaignId{"campaign-1"}, ProjectionAudience::ForPlayer(PlayerId{"player-1"}));
    REQUIRE(ownerProjection.Succeeded());
    REQUIRE(ownerProjection.Value.AdapterStates.size() == 2);

    auto otherProjection = store->LoadCampaignProjection(
        CampaignId{"campaign-1"}, ProjectionAudience::ForPlayer(PlayerId{"player-2"}));
    REQUIRE(otherProjection.Succeeded());
    REQUIRE(otherProjection.Value.AdapterStates.size() == 1);
}
TEST_CASE("Roster and binding constraints fail safely", "[campaign.persistence][identity]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    REQUIRE(store->CreateCampaign(MakeCampaign()).Succeeded());

    CampaignMutationRequest wrongBinding;
    wrongBinding.Campaign = CampaignId{"campaign-1"};
    wrongBinding.ExpectedRevision = 1;
    wrongBinding.Mutation = MutationId{"mutation-wrong-binding"};
    wrongBinding.Kind = "PersistCharacterBuild";
    wrongBinding.MutationPayload = {0x80};
    wrongBinding.CharacterBuildUpserts = {
        MakeBuild("slot-1", "binding-wrong")};
    MutationResult rejected = store->ApplyMutation(wrongBinding);
    REQUIRE(rejected.Error == StoreError::InvalidArgument);
    REQUIRE(rejected.Message.find("CharacterBinding") != std::string::npos);
    REQUIRE(store->LoadCampaign(CampaignId{"campaign-1"}).Value.CurrentRevision == 1);

    CampaignMutationRequest sealedReplacement;
    sealedReplacement.Campaign = CampaignId{"campaign-1"};
    sealedReplacement.ExpectedRevision = 1;
    sealedReplacement.Mutation = MutationId{"mutation-replace-sealed"};
    sealedReplacement.Kind = "ReplaceRoster";
    sealedReplacement.MutationPayload = {0x81};
    sealedReplacement.ReplacementRoster = std::vector<CampaignSlotRecord>{
        {CampaignSlotId{"slot-new"}, PlayerId{"player-new"},
         CharacterBindingId{"binding-new"}}};
    REQUIRE(store->ApplyMutation(sealedReplacement).Error == StoreError::InvalidArgument);
}

TEST_CASE("Prepared data statements safely preserve quoted identities", "[campaign.persistence][security]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    CreateCampaignRequest quoted = MakeCampaign("campaign';drop-table--");
    quoted.Mutation = MutationId{"mutation-quoted"};
    REQUIRE(store->CreateCampaign(quoted).Succeeded());
    auto loaded = store->LoadCampaign(CampaignId{"campaign';drop-table--"});
    REQUIRE(loaded.Succeeded());

    CreateCampaignRequest normal = MakeCampaign("campaign-after-quoted");
    normal.Mutation = MutationId{"mutation-after-quoted"};
    REQUIRE(store->CreateCampaign(normal).Succeeded());

    CreateCampaignRequest oversized = MakeCampaign(std::string(129, 'x'));
    oversized.Mutation = MutationId{"mutation-oversized"};
    REQUIRE(store->CreateCampaign(oversized).Error == StoreError::InvalidArgument);
}

TEST_CASE("Empty Lobby roster persists but cannot be sealed", "[campaign.persistence][identity]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);

    CreateCampaignRequest lobby = MakeCampaign("campaign-empty-lobby", false);
    lobby.Slots.clear();
    lobby.CharacterBuilds.clear();
    lobby.AdapterStates.clear();
    lobby.Mutation = MutationId{"mutation-create-empty-lobby"};
    REQUIRE(store->CreateCampaign(lobby).Succeeded());
    REQUIRE(store->LoadCampaignProjection(
        CampaignId{"campaign-empty-lobby"},
        ProjectionAudience::Server()).Value.Slots.empty());

    CampaignMutationRequest seal;
    seal.Campaign = CampaignId{"campaign-empty-lobby"};
    seal.ExpectedRevision = 1;
    seal.Mutation = MutationId{"mutation-seal-empty-lobby"};
    seal.Kind = "CommitCampaignStart";
    seal.MutationPayload = {0x01};
    seal.RosterSealed = true;
    REQUIRE(store->ApplyMutation(seal).Error == StoreError::InvalidArgument);
    const auto unchanged = store->LoadCampaign(
        CampaignId{"campaign-empty-lobby"});
    REQUIRE(unchanged.Succeeded());
    REQUIRE(unchanged.Value.CurrentRevision == 1);
    REQUIRE_FALSE(unchanged.Value.RosterSealed);

    CreateCampaignRequest invalidSealed = MakeCampaign(
        "campaign-empty-sealed", true);
    invalidSealed.Slots.clear();
    invalidSealed.CharacterBuilds.clear();
    invalidSealed.AdapterStates.clear();
    invalidSealed.Mutation = MutationId{"mutation-create-empty-sealed"};
    REQUIRE(store->CreateCampaign(invalidSealed).Error ==
            StoreError::InvalidArgument);
}
