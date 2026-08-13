#include <campaign_persistence_test_helpers.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

TEST_CASE("Checkpoint candidate and commit metadata survive restart", "[campaign.persistence][checkpoint]")
{
    TemporaryDatabase database;
    {
        auto store = OpenStore(database);
        REQUIRE(store->CreateCampaign(MakeCampaign()).Succeeded());
        MutationResult candidate = CreateCandidate(*store, 1);
        REQUIRE(candidate.Succeeded());
        REQUIRE(candidate.Revision == 2);
        MutationResult duplicateCandidate = CreateCandidate(*store, 1);
        REQUIRE(duplicateCandidate.Succeeded());
        REQUIRE(duplicateCandidate.IdempotentReplay);
        REQUIRE(duplicateCandidate.Revision == 2);
        auto loadedCandidate = store->LoadCheckpoint(
            CampaignId{"campaign-1"}, CheckpointId{"checkpoint-1"});
        REQUIRE(loadedCandidate.Succeeded());
        REQUIRE(loadedCandidate.Value.State == CheckpointState::Candidate);
        REQUIRE(loadedCandidate.Value.SourceRevision == 1);
        REQUIRE(loadedCandidate.Value.Slots.size() == 2);
        REQUIRE(Commit(*store, 2).Error == StoreError::InvalidArgument);
        auto noCommitted = store->LoadLastCommittedCheckpoint(CampaignId{"campaign-1"});
        REQUIRE(noCommitted.Error == StoreError::NotFound);
    }

    auto store = OpenStore(database);
    auto candidate = store->LoadCheckpoint(
        CampaignId{"campaign-1"}, CheckpointId{"checkpoint-1"});
    REQUIRE(candidate.Succeeded());
    REQUIRE(candidate.Value.State == CheckpointState::Candidate);

    CheckpointSlotRecord wrong = CompleteSave(
        "slot-1", "player-wrong", "binding-1", 1);
    MutationResult wrongResult = RecordSave(
        *store, 2, wrong, "mutation-wrong-save");
    REQUIRE(wrongResult.Error == StoreError::InvalidArgument);
    REQUIRE(store->LoadCampaign(CampaignId{"campaign-1"}).Value.CurrentRevision == 2);

    CheckpointSlotRecord first = CompleteSave(
        "slot-1", "player-1", "binding-1", 1);
    REQUIRE(RecordSave(*store, 2, first, "mutation-save-1").Revision == 3);
    CheckpointSlotRecord conflicting = CompleteSave(
        "slot-1", "player-1", "binding-1", 9);
    REQUIRE(RecordSave(
        *store, 3, conflicting, "mutation-save-conflict").Error ==
        StoreError::IdempotencyConflict);
    REQUIRE(store->LoadCampaign(CampaignId{"campaign-1"}).Value.CurrentRevision == 3);
    MutationResult duplicateDifferentId = RecordSave(
        *store, 3, first, "mutation-save-1-repeat");
    REQUIRE(duplicateDifferentId.Succeeded());
    REQUIRE(duplicateDifferentId.IdempotentReplay);
    REQUIRE(duplicateDifferentId.Revision == 3);

    REQUIRE(RecordSave(
        *store,
        3,
        CompleteSave("slot-2", "player-2", "binding-2", 2),
        "mutation-save-2").Revision == 4);
    MutationResult committed = Commit(*store, 4);
    REQUIRE(committed.Succeeded());
    REQUIRE(committed.Revision == 5);
    MutationResult duplicateCommit = Commit(
        *store, 4, "checkpoint-1", "mutation-commit-repeat");
    REQUIRE(duplicateCommit.Succeeded());
    REQUIRE(duplicateCommit.IdempotentReplay);

    REQUIRE_FALSE(ExecuteRaw(
        database.Path,
        "UPDATE campaign_snapshots SET checksum='rewritten' "
        "WHERE snapshot_id='snapshot-1';"));

    store.reset();
    store = OpenStore(database);

    auto selected = store->LoadLastCommittedCheckpoint(CampaignId{"campaign-1"});
    REQUIRE(selected.Succeeded());
    REQUIRE(selected.Value.Id.Value == "checkpoint-1");
    REQUIRE(selected.Value.State == CheckpointState::Committed);
    REQUIRE(selected.Value.SourceRevision == 1);
    REQUIRE(selected.Value.CommittedRevision == 5);
    REQUIRE(store->LoadCheckpoint(
        CampaignId{"campaign-wrong"},
        CheckpointId{"checkpoint-1"}).Error == StoreError::NotFound);

    REQUIRE(CreateCandidate(
        *store, 5, "checkpoint-2", "snapshot-2", "mutation-candidate-2").Succeeded());
    REQUIRE(Commit(
        *store, 6, "checkpoint-2", "mutation-commit-2").Error ==
        StoreError::InvalidArgument);
    auto stillSelected = store->LoadLastCommittedCheckpoint(CampaignId{"campaign-1"});
    REQUIRE(stillSelected.Succeeded());
    REQUIRE(stillSelected.Value.Id.Value == "checkpoint-1");
}
