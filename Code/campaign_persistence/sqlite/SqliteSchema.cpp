#include <sqlite/SqliteSchema.h>

#include <sqlite/SqlitePrimitives.h>

#include <sqlite3.h>

#include <string>
#include <vector>

namespace STRE::Campaign::Sqlite
{
namespace
{
constexpr const char* kSchemaSql = R"sql(
CREATE TABLE IF NOT EXISTS schema_metadata (
    singleton INTEGER PRIMARY KEY CHECK (singleton = 1),
    schema_version INTEGER NOT NULL,
    migrated_at_unix_ms INTEGER NOT NULL
);

CREATE TABLE IF NOT EXISTS campaigns (
    campaign_id TEXT PRIMARY KEY,
    persistence_schema_version INTEGER NOT NULL,
    current_revision INTEGER NOT NULL CHECK (current_revision >= 0),
    roster_sealed INTEGER NOT NULL CHECK (roster_sealed IN (0, 1)),
    last_committed_checkpoint_id TEXT,
    core_state_codec_version INTEGER NOT NULL CHECK (core_state_codec_version > 0),
    core_state_payload BLOB NOT NULL,
    created_at_unix_ms INTEGER NOT NULL,
    updated_at_unix_ms INTEGER NOT NULL,
    FOREIGN KEY (last_committed_checkpoint_id, campaign_id)
        REFERENCES campaign_checkpoints(checkpoint_id, campaign_id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS campaign_slots (
    campaign_id TEXT NOT NULL,
    slot_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    character_binding_id TEXT NOT NULL,
    PRIMARY KEY (campaign_id, slot_id),
    UNIQUE (campaign_id, player_id),
    UNIQUE (campaign_id, character_binding_id),
    FOREIGN KEY (campaign_id) REFERENCES campaigns(campaign_id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS character_build_state (
    campaign_id TEXT NOT NULL,
    slot_id TEXT NOT NULL,
    character_binding_id TEXT NOT NULL,
    persistence_codec_version INTEGER NOT NULL CHECK (persistence_codec_version > 0),
    build_version INTEGER NOT NULL,
    class_id TEXT NOT NULL,
    inventory_hash TEXT NOT NULL,
    spell_hash TEXT NOT NULL,
    state_payload BLOB NOT NULL,
    updated_revision INTEGER NOT NULL CHECK (updated_revision >= 0),
    PRIMARY KEY (campaign_id, slot_id),
    FOREIGN KEY (campaign_id, slot_id) REFERENCES campaign_slots(campaign_id, slot_id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS adapter_state (
    campaign_id TEXT NOT NULL,
    adapter_id TEXT NOT NULL,
    adapter_version INTEGER NOT NULL CHECK (adapter_version >= 0),
    codec_version INTEGER NOT NULL CHECK (codec_version > 0),
    audience INTEGER NOT NULL CHECK (audience IN (0, 1)),
    audience_player_id TEXT,
    state_payload BLOB NOT NULL,
    updated_revision INTEGER NOT NULL CHECK (updated_revision >= 0),
    PRIMARY KEY (campaign_id, adapter_id),
    FOREIGN KEY (campaign_id) REFERENCES campaigns(campaign_id) ON DELETE RESTRICT,
    CHECK ((audience = 0 AND audience_player_id IS NULL) OR
           (audience = 1 AND audience_player_id IS NOT NULL))
);

CREATE TABLE IF NOT EXISTS campaign_snapshots (
    snapshot_id TEXT PRIMARY KEY,
    campaign_id TEXT NOT NULL,
    source_revision INTEGER NOT NULL CHECK (source_revision >= 0),
    snapshot_codec_version INTEGER NOT NULL CHECK (snapshot_codec_version > 0),
    snapshot_payload BLOB NOT NULL,
    checksum TEXT NOT NULL,
    created_at_unix_ms INTEGER NOT NULL,
    UNIQUE (campaign_id, snapshot_id),
    UNIQUE (snapshot_id, campaign_id),
    FOREIGN KEY (campaign_id) REFERENCES campaigns(campaign_id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS campaign_checkpoints (
    checkpoint_id TEXT PRIMARY KEY,
    campaign_id TEXT NOT NULL,
    checkpoint_state INTEGER NOT NULL CHECK (checkpoint_state IN (0, 1)),
    source_revision INTEGER NOT NULL CHECK (source_revision >= 0),
    snapshot_id TEXT NOT NULL,
    created_revision INTEGER NOT NULL CHECK (created_revision > 0),
    committed_revision INTEGER,
    created_at_unix_ms INTEGER NOT NULL,
    committed_at_unix_ms INTEGER,
    UNIQUE (campaign_id, checkpoint_id),
    UNIQUE (checkpoint_id, campaign_id),
    FOREIGN KEY (campaign_id) REFERENCES campaigns(campaign_id) ON DELETE RESTRICT,
    FOREIGN KEY (snapshot_id, campaign_id)
        REFERENCES campaign_snapshots(snapshot_id, campaign_id) ON DELETE RESTRICT,
    CHECK ((checkpoint_state = 0 AND committed_revision IS NULL AND committed_at_unix_ms IS NULL) OR
           (checkpoint_state = 1 AND committed_revision IS NOT NULL AND committed_at_unix_ms IS NOT NULL))
);

CREATE TABLE IF NOT EXISTS campaign_checkpoint_slots (
    checkpoint_id TEXT NOT NULL,
    campaign_id TEXT NOT NULL,
    slot_id TEXT NOT NULL,
    player_id TEXT NOT NULL,
    character_binding_id TEXT NOT NULL,
    native_save_identity TEXT,
    fingerprint_algorithm TEXT,
    fingerprint_version INTEGER,
    fingerprint BLOB,
    save_metadata_codec_version INTEGER,
    save_metadata BLOB,
    PRIMARY KEY (checkpoint_id, slot_id),
    FOREIGN KEY (checkpoint_id, campaign_id)
        REFERENCES campaign_checkpoints(checkpoint_id, campaign_id) ON DELETE RESTRICT,
    FOREIGN KEY (campaign_id, slot_id) REFERENCES campaign_slots(campaign_id, slot_id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS campaign_journal (
    sequence INTEGER PRIMARY KEY AUTOINCREMENT,
    campaign_id TEXT NOT NULL,
    mutation_id TEXT NOT NULL,
    expected_revision INTEGER NOT NULL CHECK (expected_revision >= 0),
    resulting_revision INTEGER NOT NULL CHECK (resulting_revision > 0),
    mutation_kind TEXT NOT NULL,
    command_digest TEXT NOT NULL,
    payload_codec_version INTEGER NOT NULL CHECK (payload_codec_version > 0),
    payload BLOB NOT NULL,
    restored_from_checkpoint_id TEXT,
    restored_from_revision INTEGER,
    created_at_unix_ms INTEGER NOT NULL,
    UNIQUE (campaign_id, mutation_id),
    FOREIGN KEY (campaign_id) REFERENCES campaigns(campaign_id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS campaign_outbox (
    outbox_id INTEGER PRIMARY KEY AUTOINCREMENT,
    campaign_id TEXT NOT NULL,
    mutation_id TEXT NOT NULL,
    intent_index INTEGER NOT NULL CHECK (intent_index >= 0),
    revision INTEGER NOT NULL CHECK (revision > 0),
    payload_codec_version INTEGER NOT NULL CHECK (payload_codec_version > 0),
    payload BLOB NOT NULL,
    delivery_state INTEGER NOT NULL CHECK (delivery_state IN (0, 1, 2)),
    superseded_by_revision INTEGER,
    created_at_unix_ms INTEGER NOT NULL,
    delivered_at_unix_ms INTEGER,
    UNIQUE (campaign_id, mutation_id, intent_index),
    FOREIGN KEY (campaign_id) REFERENCES campaigns(campaign_id) ON DELETE RESTRICT,
    CHECK ((delivery_state = 2 AND superseded_by_revision IS NOT NULL) OR
           (delivery_state != 2 AND superseded_by_revision IS NULL))
);

CREATE INDEX IF NOT EXISTS idx_checkpoints_campaign_state
    ON campaign_checkpoints(campaign_id, checkpoint_state);
CREATE INDEX IF NOT EXISTS idx_outbox_campaign_delivery
    ON campaign_outbox(campaign_id, delivery_state, revision, outbox_id);
CREATE INDEX IF NOT EXISTS idx_journal_campaign_revision
    ON campaign_journal(campaign_id, resulting_revision);

CREATE TRIGGER IF NOT EXISTS campaign_journal_no_update
BEFORE UPDATE ON campaign_journal
BEGIN
    SELECT RAISE(ABORT, 'campaign journal is append-only');
END;

CREATE TRIGGER IF NOT EXISTS campaign_journal_no_delete
BEFORE DELETE ON campaign_journal
BEGIN
    SELECT RAISE(ABORT, 'campaign journal is append-only');
END;

CREATE TRIGGER IF NOT EXISTS campaign_snapshot_no_update
BEFORE UPDATE ON campaign_snapshots
BEGIN
    SELECT RAISE(ABORT, 'campaign snapshots are immutable');
END;

CREATE TRIGGER IF NOT EXISTS campaign_snapshot_no_delete
BEFORE DELETE ON campaign_snapshots
BEGIN
    SELECT RAISE(ABORT, 'campaign snapshots are immutable');
END;
)sql";

constexpr const char* kMigration1To2Sql = R"sql(
DROP TRIGGER IF EXISTS campaign_journal_no_update;
DROP TRIGGER IF EXISTS campaign_journal_no_delete;
DROP INDEX IF EXISTS idx_journal_campaign_revision;

ALTER TABLE campaign_journal RENAME TO campaign_journal_v1;

CREATE TABLE campaign_journal (
    sequence INTEGER PRIMARY KEY AUTOINCREMENT,
    campaign_id TEXT NOT NULL,
    mutation_id TEXT NOT NULL,
    expected_revision INTEGER NOT NULL CHECK (expected_revision >= 0),
    resulting_revision INTEGER NOT NULL CHECK (resulting_revision > 0),
    mutation_kind TEXT NOT NULL,
    command_digest TEXT NOT NULL,
    payload_codec_version INTEGER NOT NULL CHECK (payload_codec_version > 0),
    payload BLOB NOT NULL,
    restored_from_checkpoint_id TEXT,
    restored_from_revision INTEGER,
    created_at_unix_ms INTEGER NOT NULL,
    UNIQUE (campaign_id, mutation_id),
    FOREIGN KEY (campaign_id) REFERENCES campaigns(campaign_id) ON DELETE RESTRICT
);

INSERT INTO campaign_journal(
    sequence, campaign_id, mutation_id, expected_revision, resulting_revision,
    mutation_kind, command_digest, payload_codec_version, payload,
    restored_from_checkpoint_id, restored_from_revision, created_at_unix_ms)
SELECT
    sequence, campaign_id, mutation_id, expected_revision, resulting_revision,
    mutation_kind, command_digest, payload_codec_version, payload,
    restored_from_checkpoint_id, restored_from_revision, created_at_unix_ms
FROM campaign_journal_v1
ORDER BY sequence;

DROP TABLE campaign_journal_v1;

CREATE INDEX idx_journal_campaign_revision
    ON campaign_journal(campaign_id, resulting_revision);

CREATE TRIGGER campaign_journal_no_update
BEFORE UPDATE ON campaign_journal
BEGIN
    SELECT RAISE(ABORT, 'campaign journal is append-only');
END;

CREATE TRIGGER campaign_journal_no_delete
BEFORE DELETE ON campaign_journal
BEGIN
    SELECT RAISE(ABORT, 'campaign journal is append-only');
END;

UPDATE campaigns
SET persistence_schema_version = 2
WHERE persistence_schema_version = 1;
)sql";
}

StoreResult Initialize(
    sqlite3* apDatabase,
    const SqliteCampaignStoreOptions& acOptions) noexcept
{
    try
    {
        if (sqlite3_busy_timeout(
                apDatabase,
                static_cast<int>(acOptions.BusyTimeout.count())) != SQLITE_OK)
        {
            return BindFailure(apDatabase, "set SQLite busy timeout");
        }
        for (const char* pPragma : {
                 "PRAGMA foreign_keys = ON;",
                 "PRAGMA journal_mode = WAL;",
                 "PRAGMA synchronous = FULL;"})
        {
            StoreResult result = Execute(apDatabase, pPragma);
            if (!result)
                return result;
        }

        {
            Statement journalMode(apDatabase, "PRAGMA journal_mode;");
            if (!journalMode.Valid() || journalMode.Step() != SQLITE_ROW ||
                journalMode.Text(0) != "wal")
            {
                return Failure(StoreError::DatabaseFailure, "SQLite WAL journal mode could not be enabled");
            }
        }
        {
            Statement synchronous(apDatabase, "PRAGMA synchronous;");
            if (!synchronous.Valid() || synchronous.Step() != SQLITE_ROW ||
                synchronous.Int(0) != 2)
            {
                return Failure(StoreError::DatabaseFailure, "SQLite synchronous=FULL could not be enabled");
            }
        }

        {
            Statement foreignKeys(apDatabase, "PRAGMA foreign_keys;");
            if (!foreignKeys.Valid() || foreignKeys.Step() != SQLITE_ROW ||
                foreignKeys.Int(0) != 1)
            {
                return Failure(
                    StoreError::DatabaseFailure,
                    "SQLite foreign_keys could not be enabled");
            }
        }

        std::uint32_t userVersion{};
        {
            Statement versionStatement(apDatabase, "PRAGMA user_version;");
            if (!versionStatement.Valid() ||
                versionStatement.Step() != SQLITE_ROW ||
                versionStatement.Int64(0) < 0)
            {
                return BindFailure(apDatabase, "read SQLite schema version");
            }
            userVersion = static_cast<std::uint32_t>(
                versionStatement.Int64(0));
        }
        if (userVersion > kCampaignDatabaseSchemaVersion)
        {
            return Failure(
                StoreError::IncompatibleSchema,
                "campaign database schema=" + std::to_string(userVersion) +
                    " is newer than supported=" +
                    std::to_string(kCampaignDatabaseSchemaVersion));
        }

        if (userVersion == 0)
        {
            std::vector<std::string> tables;
            Statement tablesStatement(
                apDatabase,
                "SELECT name FROM sqlite_master WHERE type = 'table' "
                "AND name NOT LIKE 'sqlite_%' ORDER BY name;");
            if (!tablesStatement.Valid())
                return BindFailure(apDatabase, "inspect pre-migration database");
            int tableCode{};
            for (tableCode = tablesStatement.Step(); tableCode == SQLITE_ROW;
                 tableCode = tablesStatement.Step())
            {
                tables.push_back(tablesStatement.Text(0));
            }
            if (tableCode != SQLITE_DONE)
                return BindFailure(apDatabase, "inspect pre-migration database");
            if (!tables.empty() &&
                !(tables.size() == 1 && tables.front() == "schema_metadata"))
            {
                return Failure(
                    StoreError::IncompatibleSchema,
                    "schema version 0 database contains unknown tables; refusing destructive reset");
            }

            Transaction transaction(apDatabase);
            if (!transaction.Active())
                return Failure(StoreError::MigrationFailure, transaction.Error().Message);
            StoreResult schemaResult = Execute(apDatabase, kSchemaSql);
            if (!schemaResult)
            {
                schemaResult.Error = StoreError::MigrationFailure;
                schemaResult.Message = "campaign schema migration 0->2 failed: " +
                    schemaResult.Message;
                return schemaResult;
            }
            Statement metadata(
                apDatabase,
                "INSERT INTO schema_metadata(singleton, schema_version, migrated_at_unix_ms) "
                "VALUES(1, ?1, ?2) "
                "ON CONFLICT(singleton) DO UPDATE SET "
                "schema_version=excluded.schema_version, "
                "migrated_at_unix_ms=excluded.migrated_at_unix_ms;");
            if (!metadata.Valid() ||
                !metadata.BindInt(1, kCampaignDatabaseSchemaVersion) ||
                !metadata.BindInt64(2, NowUnixMs()))
            {
                return Failure(
                    StoreError::MigrationFailure,
                    DatabaseMessage(apDatabase, "write campaign schema metadata"));
            }
            StoreResult metadataResult = StepDone(
                apDatabase, metadata, "write campaign schema metadata");
            if (!metadataResult)
            {
                metadataResult.Error = StoreError::MigrationFailure;
                return metadataResult;
            }
            StoreResult pragmaResult = Execute(apDatabase, "PRAGMA user_version = 2;");
            if (!pragmaResult)
            {
                pragmaResult.Error = StoreError::MigrationFailure;
                return pragmaResult;
            }
            StoreResult commitResult = transaction.Commit();
            if (!commitResult)
            {
                commitResult.Error = StoreError::MigrationFailure;
                return commitResult;
            }
        }
        else if (userVersion == 1)
        {
            {
                Statement currentMetadata(
                    apDatabase,
                    "SELECT schema_version FROM schema_metadata WHERE singleton=1;");
                if (!currentMetadata.Valid() ||
                    currentMetadata.Step() != SQLITE_ROW ||
                    currentMetadata.Int64(0) != 1)
                {
                    return Failure(
                        StoreError::IncompatibleSchema,
                        "campaign schema metadata does not match SQLite user_version before migration");
                }
            }

            Transaction transaction(apDatabase);
            if (!transaction.Active())
            {
                return Failure(
                    StoreError::MigrationFailure,
                    transaction.Error().Message);
            }
            StoreResult schemaResult = Execute(apDatabase, kMigration1To2Sql);
            if (!schemaResult)
            {
                schemaResult.Error = StoreError::MigrationFailure;
                schemaResult.Message = "campaign schema migration 1->2 failed: " +
                    schemaResult.Message;
                return schemaResult;
            }
            Statement metadata(
                apDatabase,
                "UPDATE schema_metadata SET schema_version=2, "
                "migrated_at_unix_ms=?1 WHERE singleton=1;");
            if (!metadata.Valid() || !metadata.BindInt64(1, NowUnixMs()))
            {
                return Failure(
                    StoreError::MigrationFailure,
                    DatabaseMessage(apDatabase, "write campaign schema-v2 metadata"));
            }
            StoreResult metadataResult = StepDone(
                apDatabase, metadata, "write campaign schema-v2 metadata");
            if (!metadataResult)
            {
                metadataResult.Error = StoreError::MigrationFailure;
                return metadataResult;
            }
            StoreResult pragmaResult = Execute(
                apDatabase, "PRAGMA user_version = 2;");
            if (!pragmaResult)
            {
                pragmaResult.Error = StoreError::MigrationFailure;
                return pragmaResult;
            }
            StoreResult commitResult = transaction.Commit();
            if (!commitResult)
            {
                commitResult.Error = StoreError::MigrationFailure;
                return commitResult;
            }
        }

        auto schemaVersion = GetSchemaVersion(apDatabase);
        if (!schemaVersion)
            return {schemaVersion.Error, schemaVersion.Message};
        if (schemaVersion.Value != kCampaignDatabaseSchemaVersion)
        {
            return Failure(
                StoreError::IncompatibleSchema,
                "campaign schema metadata does not match supported version");
        }
        return CheckIntegrity(apDatabase);
    }
    catch (...)
    {
        return Failure(StoreError::DatabaseFailure, "campaign database initialization failed safely");
    }
}

StoreValueResult<std::uint32_t> GetSchemaVersion(sqlite3* apDatabase) noexcept
{
    StoreValueResult<std::uint32_t> result;
    try
    {
        Statement statement(
            apDatabase,
            "SELECT schema_version FROM schema_metadata WHERE singleton = 1;");
        if (!statement.Valid())
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(apDatabase, "load campaign schema metadata");
            return result;
        }
        const int code = statement.Step();
        if (code != SQLITE_ROW || statement.Int64(0) < 0)
        {
            result.Error = code == SQLITE_DONE ? StoreError::IncompatibleSchema
                                               : StoreError::DatabaseFailure;
            result.Message = "campaign schema metadata is missing or invalid";
            return result;
        }
        result.Value = static_cast<std::uint32_t>(statement.Int64(0));
    }
    catch (...)
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = "failed to load campaign schema version";
    }
    return result;
}

StoreResult CheckIntegrity(sqlite3* apDatabase) noexcept
{
    try
    {
        Statement quickCheck(apDatabase, "PRAGMA quick_check;");
        if (!quickCheck.Valid() || quickCheck.Step() != SQLITE_ROW ||
            quickCheck.Text(0) != "ok")
        {
            return Failure(StoreError::IntegrityFailure, "SQLite quick_check failed");
        }
        Statement foreignKeyCheck(apDatabase, "PRAGMA foreign_key_check;");
        if (!foreignKeyCheck.Valid())
            return BindFailure(apDatabase, "run SQLite foreign_key_check");
        const int foreignKeyCode = foreignKeyCheck.Step();
        if (foreignKeyCode == SQLITE_ROW)
        {
            return Failure(StoreError::IntegrityFailure, "SQLite foreign_key_check found a violation");
        }
        if (foreignKeyCode != SQLITE_DONE)
            return BindFailure(apDatabase, "run SQLite foreign_key_check");

        constexpr const char* requiredTables[] = {
            "schema_metadata",
            "campaigns",
            "campaign_slots",
            "character_build_state",
            "adapter_state",
            "campaign_snapshots",
            "campaign_checkpoints",
            "campaign_checkpoint_slots",
            "campaign_journal",
            "campaign_outbox"};
        for (const char* pTable : requiredTables)
        {
            Statement statement(
                apDatabase,
                "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?1;");
            if (!statement.Valid() || !statement.BindText(1, pTable) ||
                statement.Step() != SQLITE_ROW)
            {
                return Failure(
                    StoreError::IntegrityFailure,
                    std::string("mandatory campaign table missing: ") + pTable);
            }
        }
        return {};
    }
    catch (...)
    {
        return Failure(StoreError::IntegrityFailure, "campaign database integrity validation failed");
    }
}
}
