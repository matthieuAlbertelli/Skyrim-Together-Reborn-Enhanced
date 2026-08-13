#include <CampaignStore.h>
#include <sqlite/SqliteCampaignStore.h>

#include <sqlite3.h>

#include <catch2/catch.hpp>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

using namespace STRE::Campaign;

namespace STRE::Campaign::Test
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

inline std::unique_ptr<SqliteCampaignStore> OpenStore(
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

inline bool ExecuteRaw(const std::filesystem::path& acPath, const char* apSql)
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

inline CharacterBuildState MakeBuild(
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

inline CreateCampaignRequest MakeCampaign(
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

inline CheckpointSlotRecord CompleteSave(
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

inline MutationResult CreateCandidate(
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

inline MutationResult RecordSave(
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

inline MutationResult Commit(
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

inline StateVersion CompleteCheckpoint(ICampaignStore& aStore)
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
