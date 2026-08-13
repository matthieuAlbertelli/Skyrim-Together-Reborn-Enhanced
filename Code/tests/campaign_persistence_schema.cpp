#include <campaign_persistence_test_helpers.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

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

TEST_CASE(
    "Pre-refactor schema-v1 database remains fully compatible",
    "[campaign.persistence][schema][compatibility]")
{
    TemporaryDatabase database;
    CampaignProjection expectedProjection;
    CheckpointRecord expectedCheckpoint;
    std::vector<JournalRecord> expectedJournal;
    std::vector<OutboxRecord> expectedOutbox;

    {
        auto preRefactorWriter = OpenStore(database);
        REQUIRE(preRefactorWriter->GetSchemaVersion().Value == 1);
        REQUIRE(preRefactorWriter->CreateCampaign(MakeCampaign()).Succeeded());
        REQUIRE(CompleteCheckpoint(*preRefactorWriter) == 5);

        auto projection = preRefactorWriter->LoadCampaignProjection(
            CampaignId{"campaign-1"}, ProjectionAudience::Server());
        auto checkpoint = preRefactorWriter->LoadLastCommittedCheckpoint(
            CampaignId{"campaign-1"});
        auto journal = preRefactorWriter->LoadJournal(CampaignId{"campaign-1"});
        auto outbox = preRefactorWriter->LoadPendingOutbox(
            CampaignId{"campaign-1"});
        REQUIRE(projection.Succeeded());
        REQUIRE(checkpoint.Succeeded());
        REQUIRE(journal.Succeeded());
        REQUIRE(outbox.Succeeded());

        expectedProjection = std::move(projection.Value);
        expectedCheckpoint = std::move(checkpoint.Value);
        expectedJournal = std::move(journal.Value);
        expectedOutbox = std::move(outbox.Value);
    }

    auto refactoredReader = OpenStore(database);
    REQUIRE(refactoredReader->GetSchemaVersion().Value == 1);
    auto projection = refactoredReader->LoadCampaignProjection(
        CampaignId{"campaign-1"}, ProjectionAudience::Server());
    auto checkpoint = refactoredReader->LoadLastCommittedCheckpoint(
        CampaignId{"campaign-1"});
    auto journal = refactoredReader->LoadJournal(CampaignId{"campaign-1"});
    auto outbox = refactoredReader->LoadPendingOutbox(
        CampaignId{"campaign-1"});

    REQUIRE(projection.Succeeded());
    REQUIRE(projection.Value.Campaign == expectedProjection.Campaign);
    REQUIRE(projection.Value.Slots == expectedProjection.Slots);
    REQUIRE(projection.Value.CharacterBuilds == expectedProjection.CharacterBuilds);
    REQUIRE(projection.Value.AdapterStates == expectedProjection.AdapterStates);
    REQUIRE(checkpoint.Succeeded());
    REQUIRE(checkpoint.Value.Id == expectedCheckpoint.Id);
    REQUIRE(checkpoint.Value.Snapshot == expectedCheckpoint.Snapshot);
    REQUIRE(checkpoint.Value.Slots == expectedCheckpoint.Slots);
    REQUIRE(journal.Succeeded());
    REQUIRE(journal.Value.size() == expectedJournal.size());
    REQUIRE(journal.Value.back().Mutation.Value ==
        expectedJournal.back().Mutation.Value);
    REQUIRE(journal.Value.back().ResultingRevision ==
        expectedJournal.back().ResultingRevision);
    REQUIRE(outbox.Succeeded());
    REQUIRE(outbox.Value.size() == expectedOutbox.size());
    REQUIRE(outbox.Value.back().Mutation.Value ==
        expectedOutbox.back().Mutation.Value);
    REQUIRE(outbox.Value.back().Revision == expectedOutbox.back().Revision);
    REQUIRE(outbox.Value.back().Payload == expectedOutbox.back().Payload);
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
