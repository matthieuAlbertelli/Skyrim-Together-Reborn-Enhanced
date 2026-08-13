#include <campaign_persistence_test_helpers.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

TEST_CASE("Restore materializes exact snapshot at a new monotonic revision", "[campaign.persistence][restore]")
{
    TemporaryDatabase database;
    auto store = OpenStore(database);
    REQUIRE(store->CreateCampaign(MakeCampaign()).Succeeded());
    REQUIRE(CompleteCheckpoint(*store) == 5);

    CharacterBuildState changed = MakeBuild();
    changed.ClassId = "class.warrior";
    changed.InventoryHash = 77;
    CampaignMutationRequest later;
    later.Campaign = CampaignId{"campaign-1"};
    later.ExpectedRevision = 5;
    later.Mutation = MutationId{"mutation-after-checkpoint"};
    later.Kind = "AdvanceCampaign";
    later.MutationPayload = {0x90};
    later.CoreStateCodecVersion = 3;
    later.CoreStatePayload = Bytes{0x91};
    later.CharacterBuildUpserts = {changed};
    later.Outbox = {{1, {0x92}}};
    REQUIRE(store->ApplyMutation(later).Revision == 6);

    RestoreCheckpointRequest restore;
    restore.Campaign = CampaignId{"campaign-1"};
    restore.ExpectedRevision = 6;
    restore.Mutation = MutationId{"mutation-restore-1"};
    restore.Checkpoint = CheckpointId{"checkpoint-1"};
    restore.MutationPayload = {0x93};
    MutationResult restored = store->RestoreCheckpointSnapshot(restore);
    INFO(restored.Message);
    REQUIRE(restored.Succeeded());
    REQUIRE(restored.Revision == 7);

    auto campaign = store->LoadCampaignProjection(
        CampaignId{"campaign-1"}, ProjectionAudience::Server());
    REQUIRE(campaign.Succeeded());
    REQUIRE(campaign.Value.Campaign.CurrentRevision == 7);
    REQUIRE(campaign.Value.Campaign.CoreStatePayload == Bytes{0x10, 0x20, 0x30});
    REQUIRE(campaign.Value.Campaign.LastCommittedCheckpoint == CheckpointId{"checkpoint-1"});
    REQUIRE(campaign.Value.CharacterBuilds.front().ClassId == "class.mage");
    REQUIRE(campaign.Value.CharacterBuilds.front().InventoryHash ==
        0xFEDCBA9876543210ull);
    REQUIRE(campaign.Value.CharacterBuilds.front().UpdatedRevision == 7);

    auto checkpoint = store->LoadCheckpoint(
        CampaignId{"campaign-1"}, CheckpointId{"checkpoint-1"});
    REQUIRE(checkpoint.Succeeded());
    REQUIRE(checkpoint.Value.SourceRevision == 1);
    REQUIRE(checkpoint.Value.CommittedRevision == 5);

    auto journal = store->LoadJournal(CampaignId{"campaign-1"});
    REQUIRE(journal.Succeeded());
    REQUIRE(journal.Value.back().Kind == "RestoreCheckpoint");
    REQUIRE(journal.Value.back().ResultingRevision == 7);
    REQUIRE(journal.Value.back().RestoredFromCheckpoint == CheckpointId{"checkpoint-1"});
    REQUIRE(journal.Value.back().RestoredFromRevision == 1);

    auto pending = store->LoadPendingOutbox(CampaignId{"campaign-1"});
    REQUIRE(pending.Succeeded());
    REQUIRE(pending.Value.size() == 1);
    REQUIRE(pending.Value.front().Revision == 7);
    REQUIRE(pending.Value.front().CodecVersion == kCampaignSnapshotCodecVersion);

    MutationResult replay = store->RestoreCheckpointSnapshot(restore);
    REQUIRE(replay.Succeeded());
    REQUIRE(replay.IdempotentReplay);
    REQUIRE(replay.Revision == 7);
}
