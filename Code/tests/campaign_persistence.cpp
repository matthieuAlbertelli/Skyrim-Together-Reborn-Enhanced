#include <CampaignStore.h>
#include <SqliteCampaignStore.h>

#include <sqlite3.h>

#include <catch2/catch.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

using namespace STRE::Campaign;

namespace
{
class TemporaryDatabase
{
public:
    TemporaryDatabase()
    {
        static std::atomic<std::uint64_t> counter{};
        const auto suffix = std::to_string(
            std::chrono::high_resolution_clock::now()
                .time_since_epoch()
                .count()) + "-" + std::to_string(counter++);
        Path = std::filesystem::temp_directory_path() /
            ("stre-campaign-persistence-" + suffix + ".sqlite3");
    }

    ~TemporaryDatabase()
    {
        std::error_code error;
        std::filesystem::remove(Path, error);
        std::filesystem::remove(Path.string() + "-wal", error);
        std::filesystem::remove(Path.string() + "-shm", error);
    }

    std::filesystem::path Path;
};

std::unique_ptr<SqliteCampaignStore> OpenStore(
    const TemporaryDatabase& acDatabase,
    SqliteCampaignStoreOptions aOptions = {})
{
    StoreResult result;
    auto store = SqliteCampaignStore::Open(
        acDatabase.Path, result, std::move(aOptions));
    INFO(result.Message);
    REQUIRE(result.Succeeded());
    REQUIRE(store);
    return store;
}

bool ExecuteRaw(const std::filesystem::path& acPath, const char* apSql)
{
    sqlite3* pDatabase{};
    if (sqlite3_open(acPath.string().c_str(), &pDatabase) != SQLITE_OK)
    {
        if (pDatabase)
            sqlite3_close_v2(pDatabase);
        return false;
    }
    const int code = sqlite3_exec(pDatabase, apSql, nullptr, nullptr, nullptr);
    sqlite3_close_v2(pDatabase);
    return code == SQLITE_OK;
}

CharacterBuildState MakeBuild(
    std::string aSlot = "slot-1",
    std::string aBinding = "binding-1",
    std::string aClass = "class.mage")
{
    CharacterBuildState build;
    build.Slot = CampaignSlotId{std::move(aSlot)};
    build.CharacterBinding = CharacterBindingId{std::move(aBinding)};
    build.PersistenceCodecVersion = 7;
    build.BuildVersion = 5;
    build.RaceId = {0, 0x00013746};
    build.ClassId = std::move(aClass);
    build.Selections = {
        {"mage.destruction", "mage.destruction.fire"},
        {"mage.alteration", "mage.alteration.protection"}};
    InventoryEntry inventory;
    inventory.BaseId = {3, 0x00003B6E};
    inventory.Count = 1;
    inventory.ExtraCharge = 42.5F;
    inventory.ExtraEnchantId = {3, 0x000040DA};
    inventory.ExtraEnchantCharge = 90;
    inventory.EnchantmentIsWeapon = true;
    inventory.EnchantmentEffects.push_back(
        {12.5F, 3, 10, 55.0F, {0, 0x00012FCD}});
    inventory.ExtraHealth = 0.75F;
    inventory.ExtraPoisonId = {0, 0x00073F31};
    inventory.ExtraPoisonCount = 2;
    inventory.ExtraSoulLevel = 3;
    inventory.ExtraOwnerId = {0, 0x00000007};
    inventory.ExtraEnchantRemoveUnequip = true;
    inventory.ExtraWorn = true;
    inventory.IsQuestItem = false;
    build.CanonicalInventory.push_back(std::move(inventory));
    build.LeftHandSpell = {3, 0x000040DA};
    build.RightHandSpell = {0, 0x00012FCD};
    build.Shout = {0, 0x00013E22};
    build.InventoryHash = 0xFEDCBA9876543210ull;
    build.CanonicalSpells = {
        {0, 0x00012FCD},
        {3, 0x000040DA},
        {3, 0x00006FD1}};
    build.SpellHash = 0x0123456789ABCDEFull;
    build.Applied = true;
    return build;
}

CreateCampaignRequest MakeCampaign(
    std::string aCampaign = "campaign-1",
    bool aSealed = true)
{
    CreateCampaignRequest request;
    request.Campaign.Id = CampaignId{std::move(aCampaign)};
    request.Campaign.PersistenceSchemaVersion =
        kCampaignDatabaseSchemaVersion;
    request.Campaign.RosterSealed = aSealed;
    request.Campaign.CoreStateCodecVersion = 3;
    request.Campaign.CoreStatePayload = {0x10, 0x20, 0x30};
    request.Slots = {
        {CampaignSlotId{"slot-1"}, PlayerId{"player-1"},
         CharacterBindingId{"binding-1"}},
        {CampaignSlotId{"slot-2"}, PlayerId{"player-2"},
         CharacterBindingId{"binding-2"}}};
    request.CharacterBuilds = {MakeBuild()};
    request.AdapterStates = {
        {"stre.alternate-start", 4, 2, StateAudience::Public,
         std::nullopt, {0x40, 0x41}, 0},
        {"stre.dragonborn-secret", 2, 1, StateAudience::Private,
         PlayerId{"player-1"}, {0x99}, 0}};
    request.Mutation = MutationId{"mutation-create-" + request.Campaign.Id.Value};
    request.MutationCodecVersion = 11;
    request.MutationPayload = {0x01};
    request.Outbox = {{2, {0xA0, 0xA1}}};
    return request;
}

CheckpointSlotRecord CompleteSave(
    std::string aSlot,
    std::string aPlayer,
    std::string aBinding,
    std::uint8_t aMarker)
{
    CheckpointSlotRecord slot;
    slot.Slot = CampaignSlotId{std::move(aSlot)};
    slot.Player = PlayerId{std::move(aPlayer)};
    slot.CharacterBinding = CharacterBindingId{std::move(aBinding)};
    slot.NativeSaveIdentity = "STRE-checkpoint-save-" + std::to_string(aMarker);
    slot.FingerprintAlgorithm = "test-fingerprint";
    slot.FingerprintVersion = 1;
    slot.Fingerprint = {aMarker, static_cast<std::uint8_t>(aMarker + 1)};
    slot.SaveMetadataCodecVersion = 3;
    slot.SaveMetadata = {0x50, aMarker};
    return slot;
}

MutationResult CreateCandidate(
    ICampaignStore& aStore,
    StateVersion aExpectedRevision,
    std::string aCheckpoint = "checkpoint-1",
    std::string aSnapshot = "snapshot-1",
    std::string aMutation = "mutation-candidate-1")
{
    CreateCheckpointCandidateRequest request;
    request.Campaign = CampaignId{"campaign-1"};
    request.ExpectedRevision = aExpectedRevision;
    request.Mutation = MutationId{std::move(aMutation)};
    request.Checkpoint = CheckpointId{std::move(aCheckpoint)};
    request.Snapshot = SnapshotId{std::move(aSnapshot)};
    request.MutationPayload = {0x21};
    request.Outbox = {{1, {0x22}}};
    return aStore.CreateCheckpointCandidate(request);
}

MutationResult RecordSave(
    ICampaignStore& aStore,
    StateVersion aExpectedRevision,
    CheckpointSlotRecord aSlot,
    std::string aMutation,
    std::string aCheckpoint = "checkpoint-1")
{
    RecordCheckpointSlotSaveRequest request;
    request.Campaign = CampaignId{"campaign-1"};
    request.ExpectedRevision = aExpectedRevision;
    request.Mutation = MutationId{std::move(aMutation)};
    request.Checkpoint = CheckpointId{std::move(aCheckpoint)};
    request.Slot = std::move(aSlot);
    request.MutationPayload = {0x31};
    request.Outbox = {{1, {0x32}}};
    return aStore.RecordCheckpointSlotSave(request);
}

MutationResult Commit(
    ICampaignStore& aStore,
    StateVersion aExpectedRevision,
    std::string aCheckpoint = "checkpoint-1",
    std::string aMutation = "mutation-commit-1")
{
    CommitCheckpointRequest request;
    request.Campaign = CampaignId{"campaign-1"};
    request.ExpectedRevision = aExpectedRevision;
    request.Mutation = MutationId{std::move(aMutation)};
    request.Checkpoint = CheckpointId{std::move(aCheckpoint)};
    request.MutationPayload = {0x41};
    request.Outbox = {{1, {0x42}}};
    return aStore.CommitCheckpoint(request);
}

StateVersion CompleteCheckpoint(ICampaignStore& aStore)
{
    REQUIRE(CreateCandidate(aStore, 1).Succeeded());
    REQUIRE(RecordSave(
        aStore,
        2,
        CompleteSave("slot-1", "player-1", "binding-1", 1),
        "mutation-save-1").Succeeded());
    REQUIRE(RecordSave(
        aStore,
        3,
        CompleteSave("slot-2", "player-2", "binding-2", 2),
        "mutation-save-2").Succeeded());
    MutationResult committed = Commit(aStore, 4);
    REQUIRE(committed.Succeeded());
    return committed.Revision;
}
}

TEST_CASE("Campaign database creates schema and survives restart", "[campaign.persistence][schema]")
{
    TemporaryDatabase database;
    {
        auto store = OpenStore(database);
        auto version = store->GetSchemaVersion();
        REQUIRE(version.Succeeded());
        REQUIRE(version.Value == kCampaignDatabaseSchemaVersion);
        REQUIRE(store->CheckIntegrity().Succeeded());

        CreateCampaignRequest first = MakeCampaign();
        MutationResult created = store->CreateCampaign(first);
        INFO(created.Message);
        REQUIRE(created.Succeeded());
        REQUIRE(created.Revision == 1);

        CreateCampaignRequest second = MakeCampaign("campaign-2");
        second.Mutation = MutationId{"mutation-create-campaign-2"};
        REQUIRE(store->CreateCampaign(second).Succeeded());
    }

    auto reopened = OpenStore(database);
    auto first = reopened->LoadCampaignProjection(
        CampaignId{"campaign-1"}, ProjectionAudience::Server());
    REQUIRE(first.Succeeded());
    REQUIRE(first.Value.Campaign.CurrentRevision == 1);
    REQUIRE(first.Value.Campaign.RosterSealed);
    REQUIRE(first.Value.Slots.size() == 2);
    REQUIRE(first.Value.CharacterBuilds.size() == 1);
    CharacterBuildState expected = MakeBuild();
    expected.UpdatedRevision = 1;
    REQUIRE(first.Value.CharacterBuilds.front() == expected);
    REQUIRE(first.Value.AdapterStates.size() == 2);

    auto second = reopened->LoadCampaignProjection(
        CampaignId{"campaign-2"}, ProjectionAudience::Server());
    REQUIRE(second.Succeeded());
    REQUIRE(second.Value.Campaign.Id.Value == "campaign-2");
    REQUIRE(second.Value.Slots.size() == 2);
}

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

TEST_CASE("Schema migration is explicit transactional and fails closed", "[campaign.persistence][migration]")
{
    SECTION("supported version zero migrates")
    {
        TemporaryDatabase database;
        REQUIRE(ExecuteRaw(
            database.Path,
            "CREATE TABLE schema_metadata(singleton INTEGER PRIMARY KEY, "
            "schema_version INTEGER NOT NULL, migrated_at_unix_ms INTEGER NOT NULL);"
            "INSERT INTO schema_metadata VALUES(1,0,0);"
            "PRAGMA user_version=0;"));
        auto store = OpenStore(database);
        auto version = store->GetSchemaVersion();
        REQUIRE(version.Succeeded());
        REQUIRE(version.Value == 1);
    }

    SECTION("newer version is rejected without reset")
    {
        TemporaryDatabase database;
        REQUIRE(ExecuteRaw(database.Path, "PRAGMA user_version=2; CREATE TABLE keep_me(value TEXT);"));
        StoreResult openResult;
        auto store = SqliteCampaignStore::Open(database.Path, openResult);
        REQUIRE_FALSE(store);
        REQUIRE(openResult.Error == StoreError::IncompatibleSchema);
        REQUIRE(ExecuteRaw(database.Path, "INSERT INTO keep_me VALUES('still-present');"));
    }

    SECTION("failed migration rolls back")
    {
        TemporaryDatabase database;
        REQUIRE(ExecuteRaw(
            database.Path,
            "CREATE TABLE schema_metadata(singleton INTEGER PRIMARY KEY);"
            "INSERT INTO schema_metadata VALUES(1);"
            "PRAGMA user_version=0;"));
        StoreResult openResult;
        auto store = SqliteCampaignStore::Open(database.Path, openResult);
        REQUIRE_FALSE(store);
        REQUIRE(openResult.Error == StoreError::MigrationFailure);
        REQUIRE(ExecuteRaw(database.Path, "INSERT OR REPLACE INTO schema_metadata VALUES(1);"));
    }
}

TEST_CASE("Optimistic revisions idempotency journal and outbox are atomic", "[campaign.persistence][transaction]")
{
    TemporaryDatabase database;
    std::optional<TransactionStage> injectedStage;
    SqliteCampaignStoreOptions options;
    options.FaultInjector = [&](TransactionStage aStage)
    {
        return injectedStage == aStage;
    };
    auto store = OpenStore(database, options);
    REQUIRE(store->CreateCampaign(MakeCampaign()).Succeeded());

    CampaignMutationRequest mutation;
    mutation.Campaign = CampaignId{"campaign-1"};
    mutation.ExpectedRevision = 1;
    mutation.Mutation = MutationId{"mutation-update-1"};
    mutation.Kind = "UpdateCampaignState";
    mutation.MutationPayload = {0x60};
    mutation.CoreStateCodecVersion = 3;
    mutation.CoreStatePayload = Bytes{0x61, 0x62};
    mutation.Outbox = {{2, {0x63}}};
    MutationResult applied = store->ApplyMutation(mutation);
    REQUIRE(applied.Succeeded());
    REQUIRE(applied.Revision == 2);

    MutationResult duplicate = store->ApplyMutation(mutation);
    REQUIRE(duplicate.Succeeded());
    REQUIRE(duplicate.IdempotentReplay);
    REQUIRE(duplicate.Revision == 2);

    CampaignMutationRequest conflicting = mutation;
    conflicting.MutationPayload = {0xFF};
    REQUIRE(store->ApplyMutation(conflicting).Error == StoreError::IdempotencyConflict);

    CampaignMutationRequest stale = mutation;
    stale.Mutation = MutationId{"mutation-stale"};
    stale.MutationPayload = {0x64};
    REQUIRE(store->ApplyMutation(stale).Error == StoreError::StaleRevision);
    auto afterStale = store->LoadCampaign(CampaignId{"campaign-1"});
    REQUIRE(afterStale.Value.CurrentRevision == 2);
    REQUIRE(afterStale.Value.CoreStatePayload == Bytes{0x61, 0x62});

    injectedStage = TransactionStage::AfterJournal;
    CampaignMutationRequest faulted = mutation;
    faulted.ExpectedRevision = 2;
    faulted.Mutation = MutationId{"mutation-faulted"};
    faulted.MutationPayload = {0x70};
    faulted.CoreStatePayload = Bytes{0x71};
    REQUIRE(store->ApplyMutation(faulted).Error == StoreError::FaultInjected);
    injectedStage.reset();

    auto afterFault = store->LoadCampaign(CampaignId{"campaign-1"});
    REQUIRE(afterFault.Value.CurrentRevision == 2);
    REQUIRE(afterFault.Value.CoreStatePayload == Bytes{0x61, 0x62});
    auto journal = store->LoadJournal(CampaignId{"campaign-1"});
    REQUIRE(journal.Succeeded());
    REQUIRE(journal.Value.size() == 2);
    REQUIRE(journal.Value[0].ResultingRevision == 1);
    REQUIRE(journal.Value[1].ResultingRevision == 2);
    auto outbox = store->LoadPendingOutbox(CampaignId{"campaign-1"});
    REQUIRE(outbox.Succeeded());
    REQUIRE(outbox.Value.size() == 2);
    REQUIRE_FALSE(ExecuteRaw(
        database.Path,
        "UPDATE campaign_journal SET mutation_kind='rewritten' "
        "WHERE campaign_id='campaign-1';"));
    REQUIRE(store->LoadJournal(CampaignId{"campaign-1"}).Value.size() == 2);
    REQUIRE(store->MarkOutboxDelivered(outbox.Value.front().Id).Succeeded());
    REQUIRE(store->MarkOutboxDelivered(outbox.Value.front().Id).Succeeded());
    REQUIRE(store->LoadPendingOutbox(CampaignId{"campaign-1"}).Value.size() == 1);
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

TEST_CASE("Faulted transaction and malformed persisted state fail safely after reopen", "[campaign.persistence][robustness]")
{
    TemporaryDatabase database;
    std::optional<TransactionStage> injectedStage;
    SqliteCampaignStoreOptions options;
    options.FaultInjector = [&](TransactionStage aStage)
    {
        return injectedStage == aStage;
    };
    {
        auto store = OpenStore(database, options);
        REQUIRE(store->CreateCampaign(MakeCampaign()).Succeeded());
        injectedStage = TransactionStage::AfterCurrentState;
        CampaignMutationRequest faulted;
        faulted.Campaign = CampaignId{"campaign-1"};
        faulted.ExpectedRevision = 1;
        faulted.Mutation = MutationId{"mutation-crash-window"};
        faulted.Kind = "CrashWindow";
        faulted.MutationPayload = {0xA0};
        faulted.CoreStateCodecVersion = 3;
        faulted.CoreStatePayload = Bytes{0xA1};
        faulted.Outbox = {{1, {0xA2}}};
        REQUIRE(store->ApplyMutation(faulted).Error == StoreError::FaultInjected);
    }

    {
        auto reopened = OpenStore(database);
        auto campaign = reopened->LoadCampaign(CampaignId{"campaign-1"});
        REQUIRE(campaign.Succeeded());
        REQUIRE(campaign.Value.CurrentRevision == 1);
        REQUIRE(campaign.Value.CoreStatePayload == Bytes{0x10, 0x20, 0x30});
        REQUIRE(reopened->LoadJournal(CampaignId{"campaign-1"}).Value.size() == 1);
    }

    REQUIRE(ExecuteRaw(
        database.Path,
        "UPDATE character_build_state SET state_payload=x'00FF' "
        "WHERE campaign_id='campaign-1' AND slot_id='slot-1';"));
    auto reopened = OpenStore(database);
    auto malformed = reopened->LoadCampaignProjection(
        CampaignId{"campaign-1"}, ProjectionAudience::Server());
    REQUIRE(malformed.Error == StoreError::IntegrityFailure);
    REQUIRE(malformed.Message.find("campaign-1") != std::string::npos);
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
