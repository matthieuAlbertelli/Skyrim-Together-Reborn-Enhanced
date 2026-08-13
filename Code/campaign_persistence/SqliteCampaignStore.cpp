#include <SqliteCampaignStore.h>

#include <CampaignCodec.h>

#include <sqlite3.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <set>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace STRE::Campaign
{
namespace
{
constexpr std::size_t kMaximumIdLength = 128;
constexpr std::size_t kMaximumKindLength = 128;
constexpr std::size_t kMaximumAdapterIdLength = 256;
constexpr std::size_t kMaximumSaveIdentityLength = 512;
constexpr std::size_t kMaximumAlgorithmLength = 128;
constexpr std::size_t kMaximumPayloadSize = 4 * 1024 * 1024;
constexpr StateVersion kMaximumRevision =
    static_cast<StateVersion>(std::numeric_limits<std::int64_t>::max());

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
    UNIQUE (campaign_id, resulting_revision),
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

std::int64_t NowUnixMs() noexcept
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

StoreResult Failure(StoreError aError, std::string aMessage)
{
    return {aError, std::move(aMessage)};
}

MutationResult MutationFailure(StoreError aError, std::string aMessage)
{
    MutationResult result;
    result.Error = aError;
    result.Message = std::move(aMessage);
    return result;
}

std::string DatabaseMessage(sqlite3* apDatabase, std::string_view acContext)
{
    std::string message(acContext);
    message += ": ";
    message += apDatabase ? sqlite3_errmsg(apDatabase) : "database unavailable";
    return message;
}

StoreResult Execute(sqlite3* apDatabase, const char* apSql)
{
    char* pError = nullptr;
    const int code = sqlite3_exec(apDatabase, apSql, nullptr, nullptr, &pError);
    if (code == SQLITE_OK)
        return {};

    std::string message = pError ? pError : sqlite3_errstr(code);
    sqlite3_free(pError);
    return Failure(StoreError::DatabaseFailure, std::move(message));
}

class Statement
{
public:
    Statement(sqlite3* apDatabase, const char* apSql)
        : m_pDatabase(apDatabase)
    {
        m_code = sqlite3_prepare_v2(
            m_pDatabase, apSql, -1, &m_pStatement, nullptr);
    }

    ~Statement()
    {
        if (m_pStatement)
            sqlite3_finalize(m_pStatement);
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    [[nodiscard]] bool Valid() const noexcept
    {
        return m_code == SQLITE_OK && m_pStatement;
    }

    [[nodiscard]] int Code() const noexcept { return m_code; }

    bool BindText(int aIndex, std::string_view acValue)
    {
        return sqlite3_bind_text(
                   m_pStatement,
                   aIndex,
                   acValue.data(),
                   static_cast<int>(acValue.size()),
                   SQLITE_TRANSIENT) == SQLITE_OK;
    }

    bool BindBlob(int aIndex, const Bytes& acValue)
    {
        return sqlite3_bind_blob64(
                   m_pStatement,
                   aIndex,
                   acValue.empty() ? nullptr : acValue.data(),
                   static_cast<sqlite3_uint64>(acValue.size()),
                   SQLITE_TRANSIENT) == SQLITE_OK;
    }

    bool BindInt(int aIndex, int aValue)
    {
        return sqlite3_bind_int(m_pStatement, aIndex, aValue) == SQLITE_OK;
    }

    bool BindInt64(int aIndex, std::int64_t aValue)
    {
        return sqlite3_bind_int64(m_pStatement, aIndex, aValue) == SQLITE_OK;
    }

    bool BindNull(int aIndex)
    {
        return sqlite3_bind_null(m_pStatement, aIndex) == SQLITE_OK;
    }

    int Step() { return sqlite3_step(m_pStatement); }

    [[nodiscard]] std::int64_t Int64(int aColumn) const
    {
        return sqlite3_column_int64(m_pStatement, aColumn);
    }

    [[nodiscard]] int Int(int aColumn) const
    {
        return sqlite3_column_int(m_pStatement, aColumn);
    }

    [[nodiscard]] std::string Text(int aColumn) const
    {
        const auto* pText = sqlite3_column_text(m_pStatement, aColumn);
        const int size = sqlite3_column_bytes(m_pStatement, aColumn);
        if (!pText || size <= 0)
            return {};
        return {
            reinterpret_cast<const char*>(pText),
            static_cast<std::size_t>(size)};
    }

    [[nodiscard]] Bytes Blob(int aColumn) const
    {
        const auto* pBlob = static_cast<const std::uint8_t*>(
            sqlite3_column_blob(m_pStatement, aColumn));
        const int size = sqlite3_column_bytes(m_pStatement, aColumn);
        if (!pBlob || size <= 0)
            return {};
        return {pBlob, pBlob + size};
    }

    [[nodiscard]] bool IsNull(int aColumn) const
    {
        return sqlite3_column_type(m_pStatement, aColumn) == SQLITE_NULL;
    }

private:
    sqlite3* m_pDatabase{};
    sqlite3_stmt* m_pStatement{};
    int m_code{SQLITE_ERROR};
};

class Transaction
{
public:
    explicit Transaction(sqlite3* apDatabase)
        : m_pDatabase(apDatabase)
    {
        const StoreResult result = Execute(m_pDatabase, "BEGIN IMMEDIATE;");
        m_active = result.Succeeded();
        m_error = result;
    }

    ~Transaction()
    {
        if (m_active)
            (void)Execute(m_pDatabase, "ROLLBACK;");
    }

    [[nodiscard]] bool Active() const noexcept { return m_active; }
    [[nodiscard]] const StoreResult& Error() const noexcept { return m_error; }

    StoreResult Commit()
    {
        if (!m_active)
            return m_error;
        StoreResult result = Execute(m_pDatabase, "COMMIT;");
        if (result)
            m_active = false;
        return result;
    }

private:
    sqlite3* m_pDatabase{};
    bool m_active{};
    StoreResult m_error;
};

bool IsValidIdentifier(std::string_view acValue)
{
    if (acValue.empty() || acValue.size() > kMaximumIdLength)
        return false;
    return std::all_of(
        acValue.begin(), acValue.end(), [](unsigned char aCharacter)
        {
            return aCharacter >= 0x21 && aCharacter <= 0x7E;
        });
}

template <class Tag> bool IsValidId(const DurableId<Tag>& acId)
{
    return IsValidIdentifier(acId.Value);
}

bool IsValidPayload(const Bytes& acPayload)
{
    return acPayload.size() <= kMaximumPayloadSize;
}

std::string Hex64(std::uint64_t aValue)
{
    std::ostringstream stream;
    stream << std::hex;
    stream.width(16);
    stream.fill('0');
    stream << aValue;
    return stream.str();
}

bool ParseHex64(std::string_view acValue, std::uint64_t& aValue)
{
    if (acValue.size() != 16)
        return false;
    std::uint64_t value{};
    for (char character : acValue)
    {
        value <<= 4;
        if (character >= '0' && character <= '9')
            value |= static_cast<std::uint64_t>(character - '0');
        else if (character >= 'a' && character <= 'f')
            value |= static_cast<std::uint64_t>(character - 'a' + 10);
        else if (character >= 'A' && character <= 'F')
            value |= static_cast<std::uint64_t>(character - 'A' + 10);
        else
            return false;
    }
    aValue = value;
    return true;
}

StoreResult ValidateOutbox(const std::vector<OutboxIntent>& acOutbox)
{
    if (acOutbox.size() > 256)
        return Failure(StoreError::InvalidArgument, "too many outbox intents");
    for (const OutboxIntent& intent : acOutbox)
    {
        if (intent.CodecVersion == 0 || !IsValidPayload(intent.Payload))
        {
            return Failure(
                StoreError::InvalidArgument,
                "outbox intent has invalid codec version or payload size");
        }
    }
    return {};
}

StoreResult ValidateSlots(const std::vector<CampaignSlotRecord>& acSlots)
{
    if (acSlots.empty() || acSlots.size() > 128)
        return Failure(StoreError::InvalidArgument, "campaign roster size is invalid");
    std::unordered_set<std::string> slots;
    std::unordered_set<std::string> players;
    std::unordered_set<std::string> bindings;
    for (const CampaignSlotRecord& slot : acSlots)
    {
        if (!IsValidId(slot.Slot) || !IsValidId(slot.Player) ||
            !IsValidId(slot.CharacterBinding))
        {
            return Failure(StoreError::InvalidArgument, "campaign roster contains an invalid identity");
        }
        if (!slots.insert(slot.Slot.Value).second ||
            !players.insert(slot.Player.Value).second ||
            !bindings.insert(slot.CharacterBinding.Value).second)
        {
            return Failure(StoreError::InvalidArgument, "campaign roster identities must be unique");
        }
    }
    return {};
}

StoreResult ValidateCharacterBuild(const CharacterBuildState& acBuild)
{
    if (!IsValidId(acBuild.Slot) || !IsValidId(acBuild.CharacterBinding) ||
        acBuild.PersistenceCodecVersion == 0 || acBuild.BuildVersion == 0 ||
        acBuild.ClassId.empty() || acBuild.ClassId.size() > 256 ||
        acBuild.Selections.size() > 256 ||
        acBuild.CanonicalInventory.size() > 4096 ||
        acBuild.CanonicalSpells.size() > 1024)
    {
        return Failure(StoreError::InvalidArgument, "character-build state is outside persistence bounds");
    }
    Bytes encoded;
    StoreResult encodedResult = Codec::EncodeCharacterBuild(acBuild, encoded);
    if (!encodedResult)
        return encodedResult;
    return {};
}

StoreResult ValidateAdapterState(const AdapterState& acState)
{
    if (acState.AdapterId.empty() ||
        acState.AdapterId.size() > kMaximumAdapterIdLength ||
        acState.AdapterVersion > kMaximumRevision || acState.CodecVersion == 0 ||
        !IsValidPayload(acState.Payload))
    {
        return Failure(StoreError::InvalidArgument, "adapter state is outside persistence bounds");
    }
    if (acState.Audience == StateAudience::Public && acState.AudiencePlayer)
        return Failure(StoreError::InvalidArgument, "public adapter state cannot name a private audience");
    if (acState.Audience == StateAudience::Private &&
        (!acState.AudiencePlayer || !IsValidId(*acState.AudiencePlayer)))
    {
        return Failure(StoreError::InvalidArgument, "private adapter state requires a valid player audience");
    }
    return {};
}

StoreResult BindFailure(sqlite3* apDatabase, std::string_view acContext)
{
    return Failure(StoreError::DatabaseFailure, DatabaseMessage(apDatabase, acContext));
}

StoreResult StepDone(sqlite3* apDatabase, Statement& aStatement, std::string_view acContext)
{
    if (!aStatement.Valid())
        return BindFailure(apDatabase, acContext);
    const int code = aStatement.Step();
    if (code != SQLITE_DONE)
        return BindFailure(apDatabase, acContext);
    return {};
}

struct ReplayResult
{
    StoreResult Result;
    bool Found{};
    bool Conflict{};
    StateVersion Revision{};
};

ReplayResult CheckReplay(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const MutationId& acMutation,
    std::string_view acDigest)
{
    ReplayResult replay;
    Statement statement(
        apDatabase,
        "SELECT command_digest, resulting_revision FROM campaign_journal "
        "WHERE campaign_id = ?1 AND mutation_id = ?2;");
    if (!statement.Valid() ||
        !statement.BindText(1, acCampaign.Value) ||
        !statement.BindText(2, acMutation.Value))
    {
        replay.Result = BindFailure(apDatabase, "check mutation replay");
        return replay;
    }
    const int code = statement.Step();
    if (code == SQLITE_DONE)
        return replay;
    if (code != SQLITE_ROW)
    {
        replay.Result = BindFailure(apDatabase, "check mutation replay");
        return replay;
    }
    replay.Found = true;
    replay.Conflict = statement.Text(0) != acDigest;
    replay.Revision = static_cast<StateVersion>(statement.Int64(1));
    return replay;
}

StoreValueResult<StateVersion> ReadCurrentRevision(
    sqlite3* apDatabase,
    const CampaignId& acCampaign)
{
    StoreValueResult<StateVersion> result;
    Statement statement(
        apDatabase,
        "SELECT current_revision FROM campaigns WHERE campaign_id = ?1;");
    if (!statement.Valid() || !statement.BindText(1, acCampaign.Value))
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = DatabaseMessage(apDatabase, "load campaign revision");
        return result;
    }
    const int code = statement.Step();
    if (code == SQLITE_DONE)
    {
        result.Error = StoreError::NotFound;
        result.Message = "campaign=" + acCampaign.Value + " was not found";
        return result;
    }
    if (code != SQLITE_ROW || statement.Int64(0) < 0)
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = DatabaseMessage(apDatabase, "load campaign revision");
        return result;
    }
    result.Value = static_cast<StateVersion>(statement.Int64(0));
    return result;
}

StoreResult AppendJournal(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const MutationId& acMutation,
    StateVersion aExpectedRevision,
    StateVersion aResultingRevision,
    std::string_view acKind,
    std::string_view acDigest,
    std::uint32_t aCodecVersion,
    const Bytes& acPayload,
    std::int64_t aCreatedAt,
    const std::optional<CheckpointId>& acRestoredCheckpoint = std::nullopt,
    const std::optional<StateVersion>& acRestoredRevision = std::nullopt)
{
    Statement statement(
        apDatabase,
        "INSERT INTO campaign_journal("
        "campaign_id, mutation_id, expected_revision, resulting_revision, "
        "mutation_kind, command_digest, payload_codec_version, payload, "
        "restored_from_checkpoint_id, restored_from_revision, created_at_unix_ms) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11);");
    const bool bound = statement.Valid() &&
        statement.BindText(1, acCampaign.Value) &&
        statement.BindText(2, acMutation.Value) &&
        statement.BindInt64(3, static_cast<std::int64_t>(aExpectedRevision)) &&
        statement.BindInt64(4, static_cast<std::int64_t>(aResultingRevision)) &&
        statement.BindText(5, acKind) &&
        statement.BindText(6, acDigest) &&
        statement.BindInt(7, static_cast<int>(aCodecVersion)) &&
        statement.BindBlob(8, acPayload) &&
        (acRestoredCheckpoint
             ? statement.BindText(9, acRestoredCheckpoint->Value)
             : statement.BindNull(9)) &&
        (acRestoredRevision
             ? statement.BindInt64(
                   10, static_cast<std::int64_t>(*acRestoredRevision))
             : statement.BindNull(10)) &&
        statement.BindInt64(11, aCreatedAt);
    if (!bound)
        return BindFailure(apDatabase, "append campaign journal");
    return StepDone(apDatabase, statement, "append campaign journal");
}

StoreResult AppendOutbox(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const MutationId& acMutation,
    StateVersion aRevision,
    const std::vector<OutboxIntent>& acIntents,
    std::int64_t aCreatedAt)
{
    for (std::size_t index = 0; index < acIntents.size(); ++index)
    {
        const OutboxIntent& intent = acIntents[index];
        Statement statement(
            apDatabase,
            "INSERT INTO campaign_outbox("
            "campaign_id, mutation_id, intent_index, revision, "
            "payload_codec_version, payload, delivery_state, "
            "superseded_by_revision, created_at_unix_ms, delivered_at_unix_ms) "
            "VALUES(?1, ?2, ?3, ?4, ?5, ?6, 0, NULL, ?7, NULL);");
        if (!statement.Valid() ||
            !statement.BindText(1, acCampaign.Value) ||
            !statement.BindText(2, acMutation.Value) ||
            !statement.BindInt(3, static_cast<int>(index)) ||
            !statement.BindInt64(4, static_cast<std::int64_t>(aRevision)) ||
            !statement.BindInt(5, static_cast<int>(intent.CodecVersion)) ||
            !statement.BindBlob(6, intent.Payload) ||
            !statement.BindInt64(7, aCreatedAt))
        {
            return BindFailure(apDatabase, "append campaign outbox");
        }
        StoreResult result = StepDone(
            apDatabase, statement, "append campaign outbox");
        if (!result)
            return result;
    }
    return {};
}

StoreResult UpdateCampaignRevision(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    StateVersion aExpectedRevision,
    StateVersion aNewRevision,
    std::int64_t aUpdatedAt)
{
    Statement statement(
        apDatabase,
        "UPDATE campaigns SET current_revision = ?1, updated_at_unix_ms = ?2 "
        "WHERE campaign_id = ?3 AND current_revision = ?4;");
    if (!statement.Valid() ||
        !statement.BindInt64(1, static_cast<std::int64_t>(aNewRevision)) ||
        !statement.BindInt64(2, aUpdatedAt) ||
        !statement.BindText(3, acCampaign.Value) ||
        !statement.BindInt64(4, static_cast<std::int64_t>(aExpectedRevision)))
    {
        return BindFailure(apDatabase, "update campaign revision");
    }
    StoreResult result = StepDone(
        apDatabase, statement, "update campaign revision");
    if (!result)
        return result;
    if (sqlite3_changes(apDatabase) != 1)
    {
        return Failure(
            StoreError::StaleRevision,
            "campaign revision changed before mutation commit");
    }
    return {};
}

MutationResult ReplayToMutationResult(const ReplayResult& acReplay)
{
    if (!acReplay.Result)
        return MutationFailure(acReplay.Result.Error, acReplay.Result.Message);
    if (acReplay.Conflict)
    {
        return MutationFailure(
            StoreError::IdempotencyConflict,
            "MutationId was already used for a different command");
    }
    MutationResult result;
    result.Revision = acReplay.Revision;
    result.IdempotentReplay = true;
    return result;
}

StoreResult ValidateCommonMutation(
    const CampaignId& acCampaign,
    StateVersion aExpectedRevision,
    const MutationId& acMutation,
    std::string_view acKind,
    std::uint32_t aCodecVersion,
    const Bytes& acPayload,
    const std::vector<OutboxIntent>& acOutbox)
{
    if (!IsValidId(acCampaign) || !IsValidId(acMutation) ||
        aExpectedRevision > kMaximumRevision || acKind.empty() ||
        acKind.size() > kMaximumKindLength || aCodecVersion == 0 ||
        !IsValidPayload(acPayload))
    {
        return Failure(StoreError::InvalidArgument, "mutation metadata is outside persistence bounds");
    }
    return ValidateOutbox(acOutbox);
}
}

SqliteCampaignStore::SqliteCampaignStore(
    sqlite3* apDatabase,
    std::filesystem::path aPath,
    SqliteCampaignStoreOptions aOptions) noexcept
    : m_pDatabase(apDatabase)
    , m_path(std::move(aPath))
    , m_options(std::move(aOptions))
{
}

SqliteCampaignStore::~SqliteCampaignStore()
{
    if (m_pDatabase)
        sqlite3_close_v2(m_pDatabase);
}

std::unique_ptr<SqliteCampaignStore> SqliteCampaignStore::Open(
    const std::filesystem::path& acPath,
    StoreResult& aResult,
    SqliteCampaignStoreOptions aOptions) noexcept
{
    aResult = {};
    try
    {
        if (acPath.empty())
        {
            aResult = Failure(StoreError::InvalidArgument, "campaign database path is empty");
            return nullptr;
        }
        if (aOptions.BusyTimeout.count() < 0 ||
            aOptions.BusyTimeout.count() > std::numeric_limits<int>::max())
        {
            aResult = Failure(StoreError::InvalidArgument, "SQLite busy timeout is outside supported bounds");
            return nullptr;
        }

        const std::filesystem::path parent = acPath.parent_path();
        if (!parent.empty())
        {
            std::error_code error;
            std::filesystem::create_directories(parent, error);
            if (error)
            {
                aResult = Failure(
                    StoreError::DatabaseFailure,
                    "cannot create campaign database directory path=" +
                        parent.string() + " reason=" + error.message());
                return nullptr;
            }
        }

        sqlite3* pDatabase = nullptr;
        const std::u8string utf8 = acPath.u8string();
        const std::string utf8Path = utf8.empty()
            ? std::string{}
            : std::string(
                  reinterpret_cast<const char*>(utf8.data()),
                  utf8.size());
        const int code = sqlite3_open_v2(
            utf8Path.c_str(),
            &pDatabase,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
            nullptr);
        if (code != SQLITE_OK)
        {
            aResult = Failure(
                StoreError::DatabaseFailure,
                DatabaseMessage(pDatabase, "open campaign database path=" + acPath.string()));
            if (pDatabase)
                sqlite3_close_v2(pDatabase);
            return nullptr;
        }

        auto store = std::unique_ptr<SqliteCampaignStore>(
            new SqliteCampaignStore(pDatabase, acPath, std::move(aOptions)));
        aResult = store->Initialize();
        if (!aResult)
            return nullptr;
        return store;
    }
    catch (const std::exception& exception)
    {
        aResult = Failure(
            StoreError::DatabaseFailure,
            "campaign database initialization exception path=" +
                acPath.string() + " reason=" + exception.what());
        return nullptr;
    }
    catch (...)
    {
        aResult = Failure(
            StoreError::DatabaseFailure,
            "campaign database initialization failed path=" + acPath.string());
        return nullptr;
    }
}

StoreResult SqliteCampaignStore::Initialize() noexcept
{
    try
    {
        if (sqlite3_busy_timeout(
                m_pDatabase,
                static_cast<int>(m_options.BusyTimeout.count())) != SQLITE_OK)
        {
            return BindFailure(m_pDatabase, "set SQLite busy timeout");
        }
        for (const char* pPragma : {
                 "PRAGMA foreign_keys = ON;",
                 "PRAGMA journal_mode = WAL;",
                 "PRAGMA synchronous = FULL;"})
        {
            StoreResult result = Execute(m_pDatabase, pPragma);
            if (!result)
                return result;
        }

        {
            Statement journalMode(m_pDatabase, "PRAGMA journal_mode;");
            if (!journalMode.Valid() || journalMode.Step() != SQLITE_ROW ||
                journalMode.Text(0) != "wal")
            {
                return Failure(StoreError::DatabaseFailure, "SQLite WAL journal mode could not be enabled");
            }
        }
        {
            Statement synchronous(m_pDatabase, "PRAGMA synchronous;");
            if (!synchronous.Valid() || synchronous.Step() != SQLITE_ROW ||
                synchronous.Int(0) != 2)
            {
                return Failure(StoreError::DatabaseFailure, "SQLite synchronous=FULL could not be enabled");
            }
        }

        Statement foreignKeys(m_pDatabase, "PRAGMA foreign_keys;");
        if (!foreignKeys.Valid() || foreignKeys.Step() != SQLITE_ROW ||
            foreignKeys.Int(0) != 1)
        {
            return Failure(StoreError::DatabaseFailure, "SQLite foreign_keys could not be enabled");
        }

        std::uint32_t userVersion{};
        Statement versionStatement(m_pDatabase, "PRAGMA user_version;");
        if (!versionStatement.Valid() || versionStatement.Step() != SQLITE_ROW ||
            versionStatement.Int64(0) < 0)
        {
            return BindFailure(m_pDatabase, "read SQLite schema version");
        }
        userVersion = static_cast<std::uint32_t>(versionStatement.Int64(0));
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
                m_pDatabase,
                "SELECT name FROM sqlite_master WHERE type = 'table' "
                "AND name NOT LIKE 'sqlite_%' ORDER BY name;");
            if (!tablesStatement.Valid())
                return BindFailure(m_pDatabase, "inspect pre-migration database");
            int tableCode{};
            for (tableCode = tablesStatement.Step(); tableCode == SQLITE_ROW;
                 tableCode = tablesStatement.Step())
            {
                tables.push_back(tablesStatement.Text(0));
            }
            if (tableCode != SQLITE_DONE)
                return BindFailure(m_pDatabase, "inspect pre-migration database");
            if (!tables.empty() &&
                !(tables.size() == 1 && tables.front() == "schema_metadata"))
            {
                return Failure(
                    StoreError::IncompatibleSchema,
                    "schema version 0 database contains unknown tables; refusing destructive reset");
            }

            Transaction transaction(m_pDatabase);
            if (!transaction.Active())
                return Failure(StoreError::MigrationFailure, transaction.Error().Message);
            StoreResult schemaResult = Execute(m_pDatabase, kSchemaSql);
            if (!schemaResult)
            {
                schemaResult.Error = StoreError::MigrationFailure;
                schemaResult.Message = "campaign schema migration 0->1 failed: " +
                    schemaResult.Message;
                return schemaResult;
            }
            Statement metadata(
                m_pDatabase,
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
                    DatabaseMessage(m_pDatabase, "write campaign schema metadata"));
            }
            StoreResult metadataResult = StepDone(
                m_pDatabase, metadata, "write campaign schema metadata");
            if (!metadataResult)
            {
                metadataResult.Error = StoreError::MigrationFailure;
                return metadataResult;
            }
            StoreResult pragmaResult = Execute(m_pDatabase, "PRAGMA user_version = 1;");
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

        auto schemaVersion = GetSchemaVersion();
        if (!schemaVersion)
            return {schemaVersion.Error, schemaVersion.Message};
        if (schemaVersion.Value != kCampaignDatabaseSchemaVersion)
        {
            return Failure(
                StoreError::IncompatibleSchema,
                "campaign schema metadata does not match supported version");
        }
        return CheckIntegrity();
    }
    catch (...)
    {
        return Failure(StoreError::DatabaseFailure, "campaign database initialization failed safely");
    }
}

bool SqliteCampaignStore::ShouldInject(TransactionStage aStage) const
{
    return m_options.FaultInjector && m_options.FaultInjector(aStage);
}

StoreValueResult<std::uint32_t> SqliteCampaignStore::GetSchemaVersion() noexcept
{
    StoreValueResult<std::uint32_t> result;
    try
    {
        Statement statement(
            m_pDatabase,
            "SELECT schema_version FROM schema_metadata WHERE singleton = 1;");
        if (!statement.Valid())
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load campaign schema metadata");
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

StoreResult SqliteCampaignStore::CheckIntegrity() noexcept
{
    try
    {
        Statement quickCheck(m_pDatabase, "PRAGMA quick_check;");
        if (!quickCheck.Valid() || quickCheck.Step() != SQLITE_ROW ||
            quickCheck.Text(0) != "ok")
        {
            return Failure(StoreError::IntegrityFailure, "SQLite quick_check failed");
        }
        Statement foreignKeyCheck(m_pDatabase, "PRAGMA foreign_key_check;");
        if (!foreignKeyCheck.Valid())
            return BindFailure(m_pDatabase, "run SQLite foreign_key_check");
        const int foreignKeyCode = foreignKeyCheck.Step();
        if (foreignKeyCode == SQLITE_ROW)
        {
            return Failure(StoreError::IntegrityFailure, "SQLite foreign_key_check found a violation");
        }
        if (foreignKeyCode != SQLITE_DONE)
            return BindFailure(m_pDatabase, "run SQLite foreign_key_check");

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
                m_pDatabase,
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

namespace
{
void AppendDigestText(Bytes& aPayload, std::string_view acValue)
{
    aPayload.insert(aPayload.end(), acValue.begin(), acValue.end());
    aPayload.push_back(0);
}

template <class T> void AppendDigestScalar(Bytes& aPayload, T aValue)
{
    using Unsigned = std::make_unsigned_t<T>;
    const Unsigned value = static_cast<Unsigned>(aValue);
    for (std::size_t index = 0; index < sizeof(Unsigned); ++index)
    {
        aPayload.push_back(static_cast<std::uint8_t>(
            (value >> (index * 8)) & 0xFF));
    }
}

void AppendDigestBlob(Bytes& aPayload, const Bytes& acValue)
{
    AppendDigestScalar<std::uint64_t>(aPayload, acValue.size());
    aPayload.insert(aPayload.end(), acValue.begin(), acValue.end());
}

void AppendDigestOutbox(
    Bytes& aPayload,
    const std::vector<OutboxIntent>& acOutbox)
{
    AppendDigestScalar<std::uint64_t>(aPayload, acOutbox.size());
    for (const OutboxIntent& intent : acOutbox)
    {
        AppendDigestScalar(aPayload, intent.CodecVersion);
        AppendDigestBlob(aPayload, intent.Payload);
    }
}

StoreResult InsertSlot(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const CampaignSlotRecord& acSlot)
{
    Statement statement(
        apDatabase,
        "INSERT INTO campaign_slots(campaign_id, slot_id, player_id, character_binding_id) "
        "VALUES(?1, ?2, ?3, ?4);");
    if (!statement.Valid() ||
        !statement.BindText(1, acCampaign.Value) ||
        !statement.BindText(2, acSlot.Slot.Value) ||
        !statement.BindText(3, acSlot.Player.Value) ||
        !statement.BindText(4, acSlot.CharacterBinding.Value))
    {
        return BindFailure(apDatabase, "insert campaign slot");
    }
    return StepDone(apDatabase, statement, "insert campaign slot");
}

StoreResult VerifySlotBinding(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const CampaignSlotId& acSlot,
    const CharacterBindingId& acBinding)
{
    Statement statement(
        apDatabase,
        "SELECT character_binding_id FROM campaign_slots "
        "WHERE campaign_id=?1 AND slot_id=?2;");
    if (!statement.Valid() ||
        !statement.BindText(1, acCampaign.Value) ||
        !statement.BindText(2, acSlot.Value))
    {
        return BindFailure(apDatabase, "verify campaign slot binding");
    }
    const int code = statement.Step();
    if (code == SQLITE_DONE)
    {
        return Failure(
            StoreError::InvalidArgument,
            "campaign=" + acCampaign.Value + " slot=" + acSlot.Value +
                " does not exist");
    }
    if (code != SQLITE_ROW)
        return BindFailure(apDatabase, "verify campaign slot binding");
    if (statement.Text(0) != acBinding.Value)
    {
        return Failure(
            StoreError::InvalidArgument,
            "campaign=" + acCampaign.Value + " slot=" + acSlot.Value +
                " CharacterBinding mismatch");
    }
    return {};
}

StoreResult UpsertCharacterBuild(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const CharacterBuildState& acBuild,
    StateVersion aRevision)
{
    StoreResult slotResult = VerifySlotBinding(
        apDatabase, acCampaign, acBuild.Slot, acBuild.CharacterBinding);
    if (!slotResult)
        return slotResult;
    Bytes encoded;
    StoreResult encodeResult = Codec::EncodeCharacterBuild(acBuild, encoded);
    if (!encodeResult)
        return encodeResult;

    Statement statement(
        apDatabase,
        "INSERT INTO character_build_state("
        "campaign_id, slot_id, character_binding_id, persistence_codec_version, "
        "build_version, class_id, inventory_hash, spell_hash, state_payload, updated_revision) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10) "
        "ON CONFLICT(campaign_id, slot_id) DO UPDATE SET "
        "character_binding_id=excluded.character_binding_id, "
        "persistence_codec_version=excluded.persistence_codec_version, "
        "build_version=excluded.build_version, class_id=excluded.class_id, "
        "inventory_hash=excluded.inventory_hash, spell_hash=excluded.spell_hash, "
        "state_payload=excluded.state_payload, updated_revision=excluded.updated_revision;");
    if (!statement.Valid() ||
        !statement.BindText(1, acCampaign.Value) ||
        !statement.BindText(2, acBuild.Slot.Value) ||
        !statement.BindText(3, acBuild.CharacterBinding.Value) ||
        !statement.BindInt(4, static_cast<int>(acBuild.PersistenceCodecVersion)) ||
        !statement.BindInt64(5, static_cast<std::int64_t>(acBuild.BuildVersion)) ||
        !statement.BindText(6, acBuild.ClassId) ||
        !statement.BindText(7, Hex64(acBuild.InventoryHash)) ||
        !statement.BindText(8, Hex64(acBuild.SpellHash)) ||
        !statement.BindBlob(9, encoded) ||
        !statement.BindInt64(10, static_cast<std::int64_t>(aRevision)))
    {
        return BindFailure(apDatabase, "upsert durable character build");
    }
    return StepDone(apDatabase, statement, "upsert durable character build");
}

StoreResult UpsertAdapterState(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const AdapterState& acState,
    StateVersion aRevision)
{
    Statement statement(
        apDatabase,
        "INSERT INTO adapter_state("
        "campaign_id, adapter_id, adapter_version, codec_version, audience, "
        "audience_player_id, state_payload, updated_revision) "
        "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8) "
        "ON CONFLICT(campaign_id, adapter_id) DO UPDATE SET "
        "adapter_version=excluded.adapter_version, codec_version=excluded.codec_version, "
        "audience=excluded.audience, audience_player_id=excluded.audience_player_id, "
        "state_payload=excluded.state_payload, updated_revision=excluded.updated_revision;");
    const bool bound = statement.Valid() &&
        statement.BindText(1, acCampaign.Value) &&
        statement.BindText(2, acState.AdapterId) &&
        statement.BindInt64(3, static_cast<std::int64_t>(acState.AdapterVersion)) &&
        statement.BindInt(4, static_cast<int>(acState.CodecVersion)) &&
        statement.BindInt(5, static_cast<int>(acState.Audience)) &&
        (acState.AudiencePlayer
             ? statement.BindText(6, acState.AudiencePlayer->Value)
             : statement.BindNull(6)) &&
        statement.BindBlob(7, acState.Payload) &&
        statement.BindInt64(8, static_cast<std::int64_t>(aRevision));
    if (!bound)
    {
        return BindFailure(apDatabase, "upsert campaign adapter state");
    }
    return StepDone(apDatabase, statement, "upsert campaign adapter state");
}

StoreResult DeleteByTextKey(
    sqlite3* apDatabase,
    const char* apSql,
    const CampaignId& acCampaign,
    std::string_view acKey,
    std::string_view acContext)
{
    Statement statement(apDatabase, apSql);
    if (!statement.Valid() ||
        !statement.BindText(1, acCampaign.Value) ||
        !statement.BindText(2, acKey))
    {
        return BindFailure(apDatabase, acContext);
    }
    return StepDone(apDatabase, statement, acContext);
}
}

StoreValueResult<CampaignRecord> SqliteCampaignStore::LoadCampaign(
    const CampaignId& acCampaign) noexcept
{
    StoreValueResult<CampaignRecord> result;
    try
    {
        if (!IsValidId(acCampaign))
        {
            result.Error = StoreError::InvalidArgument;
            result.Message = "invalid CampaignId";
            return result;
        }
        Statement statement(
            m_pDatabase,
            "SELECT persistence_schema_version, current_revision, roster_sealed, "
            "last_committed_checkpoint_id, core_state_codec_version, core_state_payload, "
            "created_at_unix_ms, updated_at_unix_ms "
            "FROM campaigns WHERE campaign_id=?1;");
        if (!statement.Valid() || !statement.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load durable campaign");
            return result;
        }
        const int code = statement.Step();
        if (code == SQLITE_DONE)
        {
            result.Error = StoreError::NotFound;
            result.Message = "campaign=" + acCampaign.Value + " was not found";
            return result;
        }
        if (code != SQLITE_ROW || statement.Int64(0) <= 0 ||
            statement.Int64(1) < 0 || statement.Int(2) < 0 ||
            statement.Int(2) > 1 || statement.Int64(4) <= 0)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "campaign=" + acCampaign.Value + " has malformed durable state";
            return result;
        }
        result.Value.Id = acCampaign;
        result.Value.PersistenceSchemaVersion =
            static_cast<std::uint32_t>(statement.Int64(0));
        result.Value.CurrentRevision =
            static_cast<StateVersion>(statement.Int64(1));
        result.Value.RosterSealed = statement.Int(2) != 0;
        if (!statement.IsNull(3))
            result.Value.LastCommittedCheckpoint = CheckpointId{statement.Text(3)};
        result.Value.CoreStateCodecVersion =
            static_cast<std::uint32_t>(statement.Int64(4));
        result.Value.CoreStatePayload = statement.Blob(5);
        result.Value.CreatedAtUnixMs = statement.Int64(6);
        result.Value.UpdatedAtUnixMs = statement.Int64(7);
        if (result.Value.PersistenceSchemaVersion >
                kCampaignDatabaseSchemaVersion ||
            !IsValidPayload(result.Value.CoreStatePayload))
        {
            result.Error = StoreError::IncompatibleSchema;
            result.Message = "campaign=" + acCampaign.Value +
                " uses unsupported persisted state";
        }
    }
    catch (...)
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = "failed to load campaign=" + acCampaign.Value;
    }
    return result;
}

StoreValueResult<CampaignProjection> SqliteCampaignStore::LoadCampaignProjection(
    const CampaignId& acCampaign,
    const ProjectionAudience& acAudience) noexcept
{
    StoreValueResult<CampaignProjection> result;
    try
    {
        auto campaignResult = LoadCampaign(acCampaign);
        if (!campaignResult)
        {
            result.Error = campaignResult.Error;
            result.Message = campaignResult.Message;
            return result;
        }
        result.Value.Campaign = std::move(campaignResult.Value);

        Statement slots(
            m_pDatabase,
            "SELECT slot_id, player_id, character_binding_id FROM campaign_slots "
            "WHERE campaign_id=?1 ORDER BY slot_id;");
        if (!slots.Valid() || !slots.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load durable campaign roster");
            return result;
        }
        int slotCode{};
        for (slotCode = slots.Step(); slotCode == SQLITE_ROW;
             slotCode = slots.Step())
        {
            CampaignSlotRecord slot{
                CampaignSlotId{slots.Text(0)},
                PlayerId{slots.Text(1)},
                CharacterBindingId{slots.Text(2)}};
            if (!IsValidId(slot.Slot) || !IsValidId(slot.Player) ||
                !IsValidId(slot.CharacterBinding))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign=" + acCampaign.Value +
                    " contains malformed roster identity";
                return result;
            }
            result.Value.Slots.push_back(std::move(slot));
        }
        if (slotCode != SQLITE_DONE)
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "iterate durable campaign roster");
            return result;
        }

        Statement builds(
            m_pDatabase,
            "SELECT slot_id, character_binding_id, persistence_codec_version, "
            "build_version, class_id, inventory_hash, spell_hash, state_payload, updated_revision "
            "FROM character_build_state WHERE campaign_id=?1 ORDER BY slot_id;");
        if (!builds.Valid() || !builds.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load durable character builds");
            return result;
        }
        int buildCode{};
        for (buildCode = builds.Step(); buildCode == SQLITE_ROW;
             buildCode = builds.Step())
        {
            auto decoded = Codec::DecodeCharacterBuild(builds.Blob(7));
            std::uint64_t inventoryHash{};
            std::uint64_t spellHash{};
            if (!decoded ||
                decoded.Value.Slot.Value != builds.Text(0) ||
                decoded.Value.CharacterBinding.Value != builds.Text(1) ||
                decoded.Value.PersistenceCodecVersion !=
                    static_cast<std::uint32_t>(builds.Int64(2)) ||
                decoded.Value.BuildVersion !=
                    static_cast<std::uint32_t>(builds.Int64(3)) ||
                decoded.Value.ClassId != builds.Text(4) ||
                !ParseHex64(builds.Text(5), inventoryHash) ||
                !ParseHex64(builds.Text(6), spellHash) ||
                decoded.Value.InventoryHash != inventoryHash ||
                decoded.Value.SpellHash != spellHash ||
                builds.Int64(8) < 0)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign=" + acCampaign.Value +
                    " contains malformed character-build state";
                return result;
            }
            decoded.Value.UpdatedRevision =
                static_cast<StateVersion>(builds.Int64(8));
            result.Value.CharacterBuilds.push_back(std::move(decoded.Value));
        }
        if (buildCode != SQLITE_DONE)
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "iterate durable character builds");
            return result;
        }

        Statement adapters(
            m_pDatabase,
            "SELECT adapter_id, adapter_version, codec_version, audience, "
            "audience_player_id, state_payload, updated_revision "
            "FROM adapter_state WHERE campaign_id=?1 ORDER BY adapter_id;");
        if (!adapters.Valid() || !adapters.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load campaign adapter state");
            return result;
        }
        int adapterCode{};
        for (adapterCode = adapters.Step(); adapterCode == SQLITE_ROW;
             adapterCode = adapters.Step())
        {
            AdapterState state;
            state.AdapterId = adapters.Text(0);
            if (adapters.Int64(1) < 0 || adapters.Int64(2) <= 0 ||
                adapters.Int(3) < 0 || adapters.Int(3) > 1 ||
                adapters.Int64(6) < 0)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign=" + acCampaign.Value +
                    " contains malformed adapter metadata";
                return result;
            }
            state.AdapterVersion = static_cast<StateVersion>(adapters.Int64(1));
            state.CodecVersion = static_cast<std::uint32_t>(adapters.Int64(2));
            state.Audience = static_cast<StateAudience>(adapters.Int(3));
            if (!adapters.IsNull(4))
                state.AudiencePlayer = PlayerId{adapters.Text(4)};
            state.Payload = adapters.Blob(5);
            state.UpdatedRevision = static_cast<StateVersion>(adapters.Int64(6));
            StoreResult validation = ValidateAdapterState(state);
            if (!validation)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign=" + acCampaign.Value +
                    " contains invalid adapter state";
                return result;
            }

            const bool visible = state.Audience == StateAudience::Public ||
                (acAudience.IncludePrivate &&
                 (!acAudience.Player ||
                  state.AudiencePlayer == acAudience.Player));
            if (visible)
                result.Value.AdapterStates.push_back(std::move(state));
        }
        if (adapterCode != SQLITE_DONE)
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "iterate campaign adapter state");
            return result;
        }
    }
    catch (...)
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = "failed to load campaign projection=" + acCampaign.Value;
    }
    return result;
}

MutationResult SqliteCampaignStore::CreateCampaign(
    const CreateCampaignRequest& acRequest) noexcept
{
    try
    {
        StoreResult common = ValidateCommonMutation(
            acRequest.Campaign.Id,
            0,
            acRequest.Mutation,
            "CreateCampaign",
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            acRequest.Outbox);
        if (!common)
            return MutationFailure(common.Error, common.Message);
        StoreResult slotValidation = ValidateSlots(acRequest.Slots);
        if (!slotValidation)
            return MutationFailure(slotValidation.Error, slotValidation.Message);
        if (acRequest.Campaign.PersistenceSchemaVersion !=
                kCampaignDatabaseSchemaVersion ||
            acRequest.Campaign.CoreStateCodecVersion == 0 ||
            !IsValidPayload(acRequest.Campaign.CoreStatePayload))
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "campaign persistence version or core state is invalid");
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuilds)
        {
            StoreResult validation = ValidateCharacterBuild(build);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }
        {
            std::unordered_set<std::string> buildSlots;
            for (const CharacterBuildState& build : acRequest.CharacterBuilds)
            {
                if (!buildSlots.insert(build.Slot.Value).second)
                    return MutationFailure(StoreError::InvalidArgument, "duplicate character-build slot");
            }
            std::unordered_set<std::string> adapterIds;
            for (const AdapterState& state : acRequest.AdapterStates)
            {
                if (!adapterIds.insert(state.AdapterId).second)
                    return MutationFailure(StoreError::InvalidArgument, "duplicate adapter-state identity");
            }
        }
        for (const AdapterState& state : acRequest.AdapterStates)
        {
            StoreResult validation = ValidateAdapterState(state);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }

        CampaignProjection digestProjection;
        digestProjection.Campaign = acRequest.Campaign;
        digestProjection.Campaign.CurrentRevision = 1;
        digestProjection.Campaign.CreatedAtUnixMs = 0;
        digestProjection.Campaign.UpdatedAtUnixMs = 0;
        digestProjection.Slots = acRequest.Slots;
        digestProjection.CharacterBuilds = acRequest.CharacterBuilds;
        digestProjection.AdapterStates = acRequest.AdapterStates;
        Bytes digestPayload;
        StoreResult encodeDigest = Codec::EncodeSnapshot(
            digestProjection, digestPayload);
        if (!encodeDigest)
            return MutationFailure(encodeDigest.Error, encodeDigest.Message);
        AppendDigestBlob(digestPayload, acRequest.MutationPayload);
        AppendDigestOutbox(digestPayload, acRequest.Outbox);
        const std::string digest = Codec::MutationDigest(
            "CreateCampaign",
            0,
            acRequest.MutationCodecVersion,
            digestPayload,
            acRequest.Campaign.Id.Value);
        if (digest.empty())
            return MutationFailure(StoreError::DatabaseFailure, "failed to compute campaign creation digest");

        Transaction transaction(m_pDatabase);
        if (!transaction.Active())
            return MutationFailure(transaction.Error().Error, transaction.Error().Message);
        ReplayResult replay = CheckReplay(
            m_pDatabase,
            acRequest.Campaign.Id,
            acRequest.Mutation,
            digest);
        if (replay.Found || !replay.Result)
            return ReplayToMutationResult(replay);

        const std::int64_t now = NowUnixMs();
        const std::int64_t createdAt = acRequest.Campaign.CreatedAtUnixMs != 0
            ? acRequest.Campaign.CreatedAtUnixMs
            : now;
        Statement campaign(
            m_pDatabase,
            "INSERT INTO campaigns("
            "campaign_id, persistence_schema_version, current_revision, roster_sealed, "
            "last_committed_checkpoint_id, core_state_codec_version, core_state_payload, "
            "created_at_unix_ms, updated_at_unix_ms) "
            "VALUES(?1, ?2, 1, ?3, NULL, ?4, ?5, ?6, ?7);");
        if (!campaign.Valid() ||
            !campaign.BindText(1, acRequest.Campaign.Id.Value) ||
            !campaign.BindInt(2, kCampaignDatabaseSchemaVersion) ||
            !campaign.BindInt(3, acRequest.Campaign.RosterSealed ? 1 : 0) ||
            !campaign.BindInt(4, static_cast<int>(acRequest.Campaign.CoreStateCodecVersion)) ||
            !campaign.BindBlob(5, acRequest.Campaign.CoreStatePayload) ||
            !campaign.BindInt64(6, createdAt) ||
            !campaign.BindInt64(7, now))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "insert durable campaign"));
        }
        StoreResult insertCampaign = StepDone(
            m_pDatabase, campaign, "insert durable campaign");
        if (!insertCampaign)
        {
            const StoreError error = sqlite3_extended_errcode(m_pDatabase) ==
                    SQLITE_CONSTRAINT_PRIMARYKEY
                ? StoreError::AlreadyExists
                : insertCampaign.Error;
            return MutationFailure(error, insertCampaign.Message);
        }
        for (const CampaignSlotRecord& slot : acRequest.Slots)
        {
            StoreResult inserted = InsertSlot(
                m_pDatabase, acRequest.Campaign.Id, slot);
            if (!inserted)
                return MutationFailure(inserted.Error, inserted.Message);
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuilds)
        {
            StoreResult upserted = UpsertCharacterBuild(
                m_pDatabase, acRequest.Campaign.Id, build, 1);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }
        for (const AdapterState& state : acRequest.AdapterStates)
        {
            StoreResult upserted = UpsertAdapterState(
                m_pDatabase, acRequest.Campaign.Id, state, 1);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }
        if (ShouldInject(TransactionStage::AfterCurrentState))
            return MutationFailure(StoreError::FaultInjected, "fault injected after current-state write");

        StoreResult journal = AppendJournal(
            m_pDatabase,
            acRequest.Campaign.Id,
            acRequest.Mutation,
            0,
            1,
            "CreateCampaign",
            digest,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            now);
        if (!journal)
            return MutationFailure(journal.Error, journal.Message);
        if (ShouldInject(TransactionStage::AfterJournal))
            return MutationFailure(StoreError::FaultInjected, "fault injected after journal write");
        StoreResult outbox = AppendOutbox(
            m_pDatabase,
            acRequest.Campaign.Id,
            acRequest.Mutation,
            1,
            acRequest.Outbox,
            now);
        if (!outbox)
            return MutationFailure(outbox.Error, outbox.Message);
        if (ShouldInject(TransactionStage::AfterOutbox) ||
            ShouldInject(TransactionStage::BeforeCommit))
        {
            return MutationFailure(StoreError::FaultInjected, "fault injected before campaign commit");
        }
        StoreResult committed = transaction.Commit();
        if (!committed)
            return MutationFailure(committed.Error, committed.Message);

        MutationResult result;
        result.Revision = 1;
        result.Applied = true;
        return result;
    }
    catch (...)
    {
        return MutationFailure(StoreError::DatabaseFailure, "CreateCampaign failed safely");
    }
}

MutationResult SqliteCampaignStore::ApplyMutation(
    const CampaignMutationRequest& acRequest) noexcept
{
    try
    {
        StoreResult common = ValidateCommonMutation(
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            acRequest.Mutation,
            acRequest.Kind,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            acRequest.Outbox);
        if (!common)
            return MutationFailure(common.Error, common.Message);
        if (acRequest.CoreStatePayload &&
            (!acRequest.CoreStateCodecVersion ||
             *acRequest.CoreStateCodecVersion == 0 ||
             !IsValidPayload(*acRequest.CoreStatePayload)))
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "core state mutation requires a valid independent codec version and bounded payload");
        }
        if (acRequest.CoreStateCodecVersion && !acRequest.CoreStatePayload)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "core state codec cannot change without a payload");
        }
        if (acRequest.ReplacementRoster)
        {
            StoreResult validation = ValidateSlots(*acRequest.ReplacementRoster);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuildUpserts)
        {
            StoreResult validation = ValidateCharacterBuild(build);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }
        {
            std::unordered_set<std::string> buildSlots;
            for (const CharacterBuildState& build : acRequest.CharacterBuildUpserts)
            {
                if (!buildSlots.insert(build.Slot.Value).second)
                    return MutationFailure(StoreError::InvalidArgument, "duplicate character-build upsert slot");
            }
            std::unordered_set<std::string> adapterIds;
            for (const AdapterState& state : acRequest.AdapterStateUpserts)
            {
                if (!adapterIds.insert(state.AdapterId).second)
                    return MutationFailure(StoreError::InvalidArgument, "duplicate adapter-state upsert identity");
            }
        }
        for (const CampaignSlotId& slot : acRequest.CharacterBuildDeletes)
        {
            if (!IsValidId(slot))
                return MutationFailure(StoreError::InvalidArgument, "invalid character-build delete slot");
        }
        for (const AdapterState& state : acRequest.AdapterStateUpserts)
        {
            StoreResult validation = ValidateAdapterState(state);
            if (!validation)
                return MutationFailure(validation.Error, validation.Message);
        }
        for (const std::string& adapter : acRequest.AdapterStateDeletes)
        {
            if (adapter.empty() || adapter.size() > kMaximumAdapterIdLength)
                return MutationFailure(StoreError::InvalidArgument, "invalid adapter-state delete identity");
        }

        Bytes digestPayload = acRequest.MutationPayload;
        if (acRequest.CoreStateCodecVersion)
            AppendDigestScalar(digestPayload, *acRequest.CoreStateCodecVersion);
        if (acRequest.CoreStatePayload)
            AppendDigestBlob(digestPayload, *acRequest.CoreStatePayload);
        if (acRequest.RosterSealed)
            AppendDigestScalar<std::uint8_t>(digestPayload, *acRequest.RosterSealed ? 1 : 0);
        if (acRequest.ReplacementRoster)
        {
            for (const CampaignSlotRecord& slot : *acRequest.ReplacementRoster)
            {
                AppendDigestText(digestPayload, slot.Slot.Value);
                AppendDigestText(digestPayload, slot.Player.Value);
                AppendDigestText(digestPayload, slot.CharacterBinding.Value);
            }
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuildUpserts)
        {
            Bytes encoded;
            StoreResult encodedResult = Codec::EncodeCharacterBuild(build, encoded);
            if (!encodedResult)
                return MutationFailure(encodedResult.Error, encodedResult.Message);
            AppendDigestBlob(digestPayload, encoded);
        }
        for (const CampaignSlotId& slot : acRequest.CharacterBuildDeletes)
            AppendDigestText(digestPayload, slot.Value);
        for (const AdapterState& state : acRequest.AdapterStateUpserts)
        {
            AppendDigestText(digestPayload, state.AdapterId);
            AppendDigestScalar(digestPayload, state.AdapterVersion);
            AppendDigestScalar(digestPayload, state.CodecVersion);
            AppendDigestScalar(digestPayload, static_cast<std::uint8_t>(state.Audience));
            AppendDigestText(
                digestPayload,
                state.AudiencePlayer ? state.AudiencePlayer->Value : std::string_view{});
            AppendDigestBlob(digestPayload, state.Payload);
        }
        for (const std::string& adapter : acRequest.AdapterStateDeletes)
            AppendDigestText(digestPayload, adapter);
        AppendDigestOutbox(digestPayload, acRequest.Outbox);
        const std::string digest = Codec::MutationDigest(
            acRequest.Kind,
            acRequest.ExpectedRevision,
            acRequest.MutationCodecVersion,
            digestPayload);
        if (digest.empty())
            return MutationFailure(StoreError::DatabaseFailure, "failed to compute campaign mutation digest");

        Transaction transaction(m_pDatabase);
        if (!transaction.Active())
            return MutationFailure(transaction.Error().Error, transaction.Error().Message);
        ReplayResult replay = CheckReplay(
            m_pDatabase, acRequest.Campaign, acRequest.Mutation, digest);
        if (replay.Found || !replay.Result)
            return ReplayToMutationResult(replay);
        auto currentRevision = ReadCurrentRevision(m_pDatabase, acRequest.Campaign);
        if (!currentRevision)
            return MutationFailure(currentRevision.Error, currentRevision.Message);
        if (currentRevision.Value != acRequest.ExpectedRevision)
        {
            return MutationFailure(
                StoreError::StaleRevision,
                "campaign=" + acRequest.Campaign.Value +
                    " expected_revision=" + std::to_string(acRequest.ExpectedRevision) +
                    " current_revision=" + std::to_string(currentRevision.Value));
        }
        if (currentRevision.Value >= kMaximumRevision)
            return MutationFailure(StoreError::InvalidArgument, "campaign revision exhausted");
        const StateVersion newRevision = currentRevision.Value + 1;

        bool rosterSealed{};
        {
            Statement statement(
                m_pDatabase,
                "SELECT roster_sealed FROM campaigns WHERE campaign_id=?1;");
            if (!statement.Valid() ||
                !statement.BindText(1, acRequest.Campaign.Value) ||
                statement.Step() != SQLITE_ROW)
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "load roster seal state"));
            }
            rosterSealed = statement.Int(0) != 0;
        }
        if (rosterSealed && acRequest.ReplacementRoster)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "campaign=" + acRequest.Campaign.Value +
                    " roster is sealed and cannot be replaced");
        }
        if (rosterSealed && acRequest.RosterSealed && !*acRequest.RosterSealed)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "campaign=" + acRequest.Campaign.Value + " roster seal is irreversible");
        }

        if (acRequest.ReplacementRoster)
        {
            Statement deleteBuilds(
                m_pDatabase,
                "DELETE FROM character_build_state WHERE campaign_id=?1;");
            if (!deleteBuilds.Valid() ||
                !deleteBuilds.BindText(1, acRequest.Campaign.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "clear pre-seal character builds"));
            }
            StoreResult deletedBuilds = StepDone(
                m_pDatabase, deleteBuilds, "clear pre-seal character builds");
            if (!deletedBuilds)
                return MutationFailure(deletedBuilds.Error, deletedBuilds.Message);

            Statement deleteSlots(
                m_pDatabase,
                "DELETE FROM campaign_slots WHERE campaign_id=?1;");
            if (!deleteSlots.Valid() ||
                !deleteSlots.BindText(1, acRequest.Campaign.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "replace pre-seal campaign roster"));
            }
            StoreResult deletedSlots = StepDone(
                m_pDatabase, deleteSlots, "replace pre-seal campaign roster");
            if (!deletedSlots)
                return MutationFailure(deletedSlots.Error, deletedSlots.Message);
            for (const CampaignSlotRecord& slot : *acRequest.ReplacementRoster)
            {
                StoreResult inserted = InsertSlot(
                    m_pDatabase, acRequest.Campaign, slot);
                if (!inserted)
                    return MutationFailure(inserted.Error, inserted.Message);
            }
        }

        for (const CampaignSlotId& slot : acRequest.CharacterBuildDeletes)
        {
            StoreResult deleted = DeleteByTextKey(
                m_pDatabase,
                "DELETE FROM character_build_state WHERE campaign_id=?1 AND slot_id=?2;",
                acRequest.Campaign,
                slot.Value,
                "delete durable character build");
            if (!deleted)
                return MutationFailure(deleted.Error, deleted.Message);
        }
        for (const std::string& adapter : acRequest.AdapterStateDeletes)
        {
            StoreResult deleted = DeleteByTextKey(
                m_pDatabase,
                "DELETE FROM adapter_state WHERE campaign_id=?1 AND adapter_id=?2;",
                acRequest.Campaign,
                adapter,
                "delete campaign adapter state");
            if (!deleted)
                return MutationFailure(deleted.Error, deleted.Message);
        }
        if (acRequest.CoreStatePayload)
        {
            Statement statement(
                m_pDatabase,
                "UPDATE campaigns SET core_state_codec_version=?1, core_state_payload=?2 "
                "WHERE campaign_id=?3;");
            if (!statement.Valid() ||
                !statement.BindInt(1, static_cast<int>(*acRequest.CoreStateCodecVersion)) ||
                !statement.BindBlob(2, *acRequest.CoreStatePayload) ||
                !statement.BindText(3, acRequest.Campaign.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "update campaign core state"));
            }
            StoreResult updated = StepDone(
                m_pDatabase, statement, "update campaign core state");
            if (!updated)
                return MutationFailure(updated.Error, updated.Message);
        }
        if (acRequest.RosterSealed)
        {
            Statement statement(
                m_pDatabase,
                "UPDATE campaigns SET roster_sealed=?1 WHERE campaign_id=?2;");
            if (!statement.Valid() ||
                !statement.BindInt(1, *acRequest.RosterSealed ? 1 : 0) ||
                !statement.BindText(2, acRequest.Campaign.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "update durable roster seal metadata"));
            }
            StoreResult updated = StepDone(
                m_pDatabase, statement, "update durable roster seal metadata");
            if (!updated)
                return MutationFailure(updated.Error, updated.Message);
        }
        for (const CharacterBuildState& build : acRequest.CharacterBuildUpserts)
        {
            StoreResult upserted = UpsertCharacterBuild(
                m_pDatabase, acRequest.Campaign, build, newRevision);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }
        for (const AdapterState& state : acRequest.AdapterStateUpserts)
        {
            StoreResult upserted = UpsertAdapterState(
                m_pDatabase, acRequest.Campaign, state, newRevision);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }

        const std::int64_t now = NowUnixMs();
        StoreResult revision = UpdateCampaignRevision(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            newRevision,
            now);
        if (!revision)
            return MutationFailure(revision.Error, revision.Message);
        if (ShouldInject(TransactionStage::AfterCurrentState))
            return MutationFailure(StoreError::FaultInjected, "fault injected after current-state write");
        StoreResult journal = AppendJournal(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            acRequest.ExpectedRevision,
            newRevision,
            acRequest.Kind,
            digest,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            now);
        if (!journal)
            return MutationFailure(journal.Error, journal.Message);
        if (ShouldInject(TransactionStage::AfterJournal))
            return MutationFailure(StoreError::FaultInjected, "fault injected after journal write");
        StoreResult outbox = AppendOutbox(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            newRevision,
            acRequest.Outbox,
            now);
        if (!outbox)
            return MutationFailure(outbox.Error, outbox.Message);
        if (ShouldInject(TransactionStage::AfterOutbox) ||
            ShouldInject(TransactionStage::BeforeCommit))
        {
            return MutationFailure(StoreError::FaultInjected, "fault injected before mutation commit");
        }
        StoreResult committed = transaction.Commit();
        if (!committed)
            return MutationFailure(committed.Error, committed.Message);

        MutationResult result;
        result.Revision = newRevision;
        result.Applied = true;
        return result;
    }
    catch (...)
    {
        return MutationFailure(
            StoreError::DatabaseFailure,
            "campaign=" + acRequest.Campaign.Value + " mutation failed safely");
    }
}

MutationResult SqliteCampaignStore::CreateCheckpointCandidate(
    const CreateCheckpointCandidateRequest& acRequest) noexcept
{
    try
    {
        StoreResult common = ValidateCommonMutation(
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            acRequest.Mutation,
            "CreateCheckpointCandidate",
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            acRequest.Outbox);
        if (!common)
            return MutationFailure(common.Error, common.Message);
        if (!IsValidId(acRequest.Checkpoint) || !IsValidId(acRequest.Snapshot))
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "checkpoint or snapshot identity is invalid");
        }
        Bytes digestPayload = acRequest.MutationPayload;
        AppendDigestText(digestPayload, acRequest.Snapshot.Value);
        AppendDigestOutbox(digestPayload, acRequest.Outbox);
        const std::string digest = Codec::MutationDigest(
            "CreateCheckpointCandidate",
            acRequest.ExpectedRevision,
            acRequest.MutationCodecVersion,
            digestPayload,
            acRequest.Checkpoint.Value);
        if (digest.empty())
            return MutationFailure(StoreError::DatabaseFailure, "failed to compute checkpoint candidate digest");

        Transaction transaction(m_pDatabase);
        if (!transaction.Active())
            return MutationFailure(transaction.Error().Error, transaction.Error().Message);
        ReplayResult replay = CheckReplay(
            m_pDatabase, acRequest.Campaign, acRequest.Mutation, digest);
        if (replay.Found || !replay.Result)
            return ReplayToMutationResult(replay);

        auto projection = LoadCampaignProjection(
            acRequest.Campaign, ProjectionAudience::Server());
        if (!projection)
            return MutationFailure(projection.Error, projection.Message);
        if (projection.Value.Campaign.CurrentRevision != acRequest.ExpectedRevision)
        {
            return MutationFailure(
                StoreError::StaleRevision,
                "checkpoint candidate expected_revision=" +
                    std::to_string(acRequest.ExpectedRevision) +
                    " current_revision=" +
                    std::to_string(projection.Value.Campaign.CurrentRevision));
        }
        if (!projection.Value.Campaign.RosterSealed || projection.Value.Slots.empty())
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "campaign=" + acRequest.Campaign.Value +
                    " requires a non-empty sealed roster before checkpointing");
        }
        if (acRequest.ExpectedRevision >= kMaximumRevision)
            return MutationFailure(StoreError::InvalidArgument, "campaign revision exhausted");
        const StateVersion newRevision = acRequest.ExpectedRevision + 1;

        Bytes snapshotPayload;
        StoreResult encoded = Codec::EncodeSnapshot(
            projection.Value, snapshotPayload);
        if (!encoded)
            return MutationFailure(encoded.Error, encoded.Message);
        const std::string checksum = Codec::Checksum(snapshotPayload);
        if (checksum.empty())
            return MutationFailure(StoreError::DatabaseFailure, "failed to compute checkpoint snapshot checksum");
        const std::int64_t now = NowUnixMs();

        Statement snapshot(
            m_pDatabase,
            "INSERT INTO campaign_snapshots("
            "snapshot_id, campaign_id, source_revision, snapshot_codec_version, "
            "snapshot_payload, checksum, created_at_unix_ms) "
            "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7);");
        if (!snapshot.Valid() ||
            !snapshot.BindText(1, acRequest.Snapshot.Value) ||
            !snapshot.BindText(2, acRequest.Campaign.Value) ||
            !snapshot.BindInt64(3, static_cast<std::int64_t>(acRequest.ExpectedRevision)) ||
            !snapshot.BindInt(4, kCampaignSnapshotCodecVersion) ||
            !snapshot.BindBlob(5, snapshotPayload) ||
            !snapshot.BindText(6, checksum) ||
            !snapshot.BindInt64(7, now))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "insert immutable campaign snapshot"));
        }
        StoreResult insertedSnapshot = StepDone(
            m_pDatabase, snapshot, "insert immutable campaign snapshot");
        if (!insertedSnapshot)
        {
            const int extendedCode = sqlite3_extended_errcode(m_pDatabase);
            const StoreError error = (extendedCode & 0xFF) == SQLITE_CONSTRAINT
                ? StoreError::AlreadyExists
                : insertedSnapshot.Error;
            return MutationFailure(error, insertedSnapshot.Message);
        }

        Statement checkpoint(
            m_pDatabase,
            "INSERT INTO campaign_checkpoints("
            "checkpoint_id, campaign_id, checkpoint_state, source_revision, snapshot_id, "
            "created_revision, committed_revision, created_at_unix_ms, committed_at_unix_ms) "
            "VALUES(?1, ?2, 0, ?3, ?4, ?5, NULL, ?6, NULL);");
        if (!checkpoint.Valid() ||
            !checkpoint.BindText(1, acRequest.Checkpoint.Value) ||
            !checkpoint.BindText(2, acRequest.Campaign.Value) ||
            !checkpoint.BindInt64(3, static_cast<std::int64_t>(acRequest.ExpectedRevision)) ||
            !checkpoint.BindText(4, acRequest.Snapshot.Value) ||
            !checkpoint.BindInt64(5, static_cast<std::int64_t>(newRevision)) ||
            !checkpoint.BindInt64(6, now))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "insert checkpoint candidate"));
        }
        StoreResult insertedCheckpoint = StepDone(
            m_pDatabase, checkpoint, "insert checkpoint candidate");
        if (!insertedCheckpoint)
        {
            const int extendedCode = sqlite3_extended_errcode(m_pDatabase);
            const StoreError error = (extendedCode & 0xFF) == SQLITE_CONSTRAINT
                ? StoreError::AlreadyExists
                : insertedCheckpoint.Error;
            return MutationFailure(error, insertedCheckpoint.Message);
        }

        for (const CampaignSlotRecord& slot : projection.Value.Slots)
        {
            Statement checkpointSlot(
                m_pDatabase,
                "INSERT INTO campaign_checkpoint_slots("
                "checkpoint_id, campaign_id, slot_id, player_id, character_binding_id, "
                "native_save_identity, fingerprint_algorithm, fingerprint_version, "
                "fingerprint, save_metadata_codec_version, save_metadata) "
                "VALUES(?1, ?2, ?3, ?4, ?5, NULL, NULL, NULL, NULL, NULL, NULL);");
            if (!checkpointSlot.Valid() ||
                !checkpointSlot.BindText(1, acRequest.Checkpoint.Value) ||
                !checkpointSlot.BindText(2, acRequest.Campaign.Value) ||
                !checkpointSlot.BindText(3, slot.Slot.Value) ||
                !checkpointSlot.BindText(4, slot.Player.Value) ||
                !checkpointSlot.BindText(5, slot.CharacterBinding.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "insert checkpoint roster slot"));
            }
            StoreResult insertedSlot = StepDone(
                m_pDatabase, checkpointSlot, "insert checkpoint roster slot");
            if (!insertedSlot)
                return MutationFailure(insertedSlot.Error, insertedSlot.Message);
        }

        StoreResult revision = UpdateCampaignRevision(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            newRevision,
            now);
        if (!revision)
            return MutationFailure(revision.Error, revision.Message);
        if (ShouldInject(TransactionStage::AfterCurrentState))
            return MutationFailure(StoreError::FaultInjected, "fault injected after checkpoint candidate write");
        StoreResult journal = AppendJournal(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            acRequest.ExpectedRevision,
            newRevision,
            "CreateCheckpointCandidate",
            digest,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            now);
        if (!journal)
            return MutationFailure(journal.Error, journal.Message);
        if (ShouldInject(TransactionStage::AfterJournal))
            return MutationFailure(StoreError::FaultInjected, "fault injected after checkpoint journal write");
        StoreResult outbox = AppendOutbox(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            newRevision,
            acRequest.Outbox,
            now);
        if (!outbox)
            return MutationFailure(outbox.Error, outbox.Message);
        if (ShouldInject(TransactionStage::AfterOutbox) ||
            ShouldInject(TransactionStage::BeforeCommit))
        {
            return MutationFailure(StoreError::FaultInjected, "fault injected before checkpoint candidate commit");
        }
        StoreResult committed = transaction.Commit();
        if (!committed)
            return MutationFailure(committed.Error, committed.Message);

        MutationResult result;
        result.Revision = newRevision;
        result.Applied = true;
        return result;
    }
    catch (...)
    {
        return MutationFailure(
            StoreError::DatabaseFailure,
            "campaign=" + acRequest.Campaign.Value +
                " checkpoint candidate creation failed safely");
    }
}

StoreValueResult<CheckpointRecord> SqliteCampaignStore::LoadCheckpoint(
    const CampaignId& acCampaign,
    const CheckpointId& acCheckpoint) noexcept
{
    StoreValueResult<CheckpointRecord> result;
    try
    {
        if (!IsValidId(acCampaign) || !IsValidId(acCheckpoint))
        {
            result.Error = StoreError::InvalidArgument;
            result.Message = "invalid campaign/checkpoint identity";
            return result;
        }
        Statement checkpoint(
            m_pDatabase,
            "SELECT c.checkpoint_state, c.source_revision, c.snapshot_id, "
            "s.snapshot_codec_version, s.checksum, s.snapshot_payload, "
            "c.created_revision, c.committed_revision, c.created_at_unix_ms, "
            "c.committed_at_unix_ms "
            "FROM campaign_checkpoints c "
            "JOIN campaign_snapshots s ON s.snapshot_id=c.snapshot_id "
            "WHERE c.campaign_id=?1 AND c.checkpoint_id=?2 AND s.campaign_id=?1;");
        if (!checkpoint.Valid() ||
            !checkpoint.BindText(1, acCampaign.Value) ||
            !checkpoint.BindText(2, acCheckpoint.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load campaign checkpoint");
            return result;
        }
        const int code = checkpoint.Step();
        if (code == SQLITE_DONE)
        {
            result.Error = StoreError::NotFound;
            result.Message = "campaign=" + acCampaign.Value +
                " checkpoint=" + acCheckpoint.Value + " was not found";
            return result;
        }
        if (code != SQLITE_ROW || checkpoint.Int(0) < 0 || checkpoint.Int(0) > 1 ||
            checkpoint.Int64(1) < 0 || checkpoint.Int64(3) <= 0 ||
            checkpoint.Int64(6) <= 0)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "campaign checkpoint metadata is malformed";
            return result;
        }
        const Bytes snapshotPayload = checkpoint.Blob(5);
        const std::string checksum = checkpoint.Text(4);
        if (checksum.size() != 16 ||
            static_cast<std::uint32_t>(checkpoint.Int64(3)) !=
                kCampaignSnapshotCodecVersion ||
            Codec::Checksum(snapshotPayload) != checksum)
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "campaign=" + acCampaign.Value +
                " checkpoint=" + acCheckpoint.Value +
                " snapshot checksum or codec mismatch";
            return result;
        }
        auto decoded = Codec::DecodeSnapshot(snapshotPayload);
        if (!decoded || decoded.Value.Campaign.Id != acCampaign ||
            decoded.Value.Campaign.CurrentRevision !=
                static_cast<StateVersion>(checkpoint.Int64(1)))
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "campaign checkpoint snapshot identity/revision mismatch";
            return result;
        }

        result.Value.Id = acCheckpoint;
        result.Value.Campaign = acCampaign;
        result.Value.State = static_cast<CheckpointState>(checkpoint.Int(0));
        result.Value.SourceRevision = static_cast<StateVersion>(checkpoint.Int64(1));
        result.Value.Snapshot = SnapshotId{checkpoint.Text(2)};
        result.Value.SnapshotCodecVersion =
            static_cast<std::uint32_t>(checkpoint.Int64(3));
        result.Value.SnapshotChecksum = checksum;
        result.Value.CreatedRevision =
            static_cast<StateVersion>(checkpoint.Int64(6));
        if (!checkpoint.IsNull(7))
            result.Value.CommittedRevision =
                static_cast<StateVersion>(checkpoint.Int64(7));
        result.Value.CreatedAtUnixMs = checkpoint.Int64(8);
        if (!checkpoint.IsNull(9))
            result.Value.CommittedAtUnixMs = checkpoint.Int64(9);

        Statement slots(
            m_pDatabase,
            "SELECT slot_id, player_id, character_binding_id, native_save_identity, "
            "fingerprint_algorithm, fingerprint_version, fingerprint, "
            "save_metadata_codec_version, save_metadata "
            "FROM campaign_checkpoint_slots "
            "WHERE campaign_id=?1 AND checkpoint_id=?2 ORDER BY slot_id;");
        if (!slots.Valid() ||
            !slots.BindText(1, acCampaign.Value) ||
            !slots.BindText(2, acCheckpoint.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load checkpoint roster metadata");
            return result;
        }
        int checkpointSlotCode{};
        for (checkpointSlotCode = slots.Step(); checkpointSlotCode == SQLITE_ROW;
             checkpointSlotCode = slots.Step())
        {
            CheckpointSlotRecord slot;
            slot.Slot = CampaignSlotId{slots.Text(0)};
            slot.Player = PlayerId{slots.Text(1)};
            slot.CharacterBinding = CharacterBindingId{slots.Text(2)};
            if (!slots.IsNull(3))
                slot.NativeSaveIdentity = slots.Text(3);
            if (!slots.IsNull(4))
                slot.FingerprintAlgorithm = slots.Text(4);
            if (!slots.IsNull(5))
            {
                if (slots.Int64(5) < 0 ||
                    slots.Int64(5) > std::numeric_limits<std::uint32_t>::max())
                {
                    result.Error = StoreError::IntegrityFailure;
                    result.Message = "checkpoint fingerprint version is invalid";
                    return result;
                }
                slot.FingerprintVersion =
                    static_cast<std::uint32_t>(slots.Int64(5));
            }
            if (!slots.IsNull(6))
                slot.Fingerprint = slots.Blob(6);
            if (!slots.IsNull(7))
            {
                if (slots.Int64(7) <= 0 ||
                    slots.Int64(7) > std::numeric_limits<std::uint32_t>::max())
                {
                    result.Error = StoreError::IntegrityFailure;
                    result.Message = "checkpoint save metadata codec is invalid";
                    return result;
                }
                slot.SaveMetadataCodecVersion =
                    static_cast<std::uint32_t>(slots.Int64(7));
            }
            if (!slots.IsNull(8))
                slot.SaveMetadata = slots.Blob(8);
            if (!IsValidId(slot.Slot) || !IsValidId(slot.Player) ||
                !IsValidId(slot.CharacterBinding) ||
                !IsValidPayload(slot.Fingerprint) ||
                !IsValidPayload(slot.SaveMetadata))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "checkpoint roster metadata is malformed";
                return result;
            }
            result.Value.Slots.push_back(std::move(slot));
        }
        if (checkpointSlotCode != SQLITE_DONE)
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "iterate checkpoint roster metadata");
            return result;
        }
        if (result.Value.Slots.size() != decoded.Value.Slots.size())
        {
            result.Error = StoreError::IntegrityFailure;
            result.Message = "checkpoint roster does not match immutable snapshot roster";
            return result;
        }
        if (result.Value.State == CheckpointState::Committed)
        {
            for (const CheckpointSlotRecord& slot : result.Value.Slots)
            {
                if (!slot.NativeSaveIdentity || slot.NativeSaveIdentity->empty() ||
                    !slot.FingerprintAlgorithm || slot.FingerprintAlgorithm->empty() ||
                    !slot.FingerprintVersion || *slot.FingerprintVersion == 0 ||
                    slot.Fingerprint.empty() || !slot.SaveMetadataCodecVersion ||
                    *slot.SaveMetadataCodecVersion == 0 || slot.SaveMetadata.empty())
                {
                    result.Error = StoreError::IntegrityFailure;
                    result.Message = "committed checkpoint contains incomplete slot save metadata";
                    return result;
                }
            }
        }
        for (std::size_t index = 0; index < result.Value.Slots.size(); ++index)
        {
            const auto& persisted = result.Value.Slots[index];
            const auto& snapshotSlot = decoded.Value.Slots[index];
            if (persisted.Slot != snapshotSlot.Slot ||
                persisted.Player != snapshotSlot.Player ||
                persisted.CharacterBinding != snapshotSlot.CharacterBinding)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "checkpoint roster identity differs from immutable snapshot";
                return result;
            }
        }
    }
    catch (...)
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = "failed to load campaign checkpoint safely";
    }
    return result;
}

StoreValueResult<CheckpointRecord>
SqliteCampaignStore::LoadLastCommittedCheckpoint(
    const CampaignId& acCampaign) noexcept
{
    auto campaign = LoadCampaign(acCampaign);
    if (!campaign)
    {
        StoreValueResult<CheckpointRecord> result;
        result.Error = campaign.Error;
        result.Message = campaign.Message;
        return result;
    }
    if (!campaign.Value.LastCommittedCheckpoint)
    {
        StoreValueResult<CheckpointRecord> result;
        result.Error = StoreError::NotFound;
        result.Message = "campaign=" + acCampaign.Value +
            " has no committed checkpoint";
        return result;
    }
    return LoadCheckpoint(acCampaign, *campaign.Value.LastCommittedCheckpoint);
}

MutationResult SqliteCampaignStore::RecordCheckpointSlotSave(
    const RecordCheckpointSlotSaveRequest& acRequest) noexcept
{
    try
    {
        StoreResult common = ValidateCommonMutation(
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            acRequest.Mutation,
            "RecordCheckpointSlotSave",
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            acRequest.Outbox);
        if (!common)
            return MutationFailure(common.Error, common.Message);
        const CheckpointSlotRecord& slot = acRequest.Slot;
        if (!IsValidId(acRequest.Checkpoint) || !IsValidId(slot.Slot) ||
            !IsValidId(slot.Player) || !IsValidId(slot.CharacterBinding) ||
            !slot.NativeSaveIdentity || slot.NativeSaveIdentity->empty() ||
            slot.NativeSaveIdentity->size() > kMaximumSaveIdentityLength ||
            !slot.FingerprintAlgorithm || slot.FingerprintAlgorithm->empty() ||
            slot.FingerprintAlgorithm->size() > kMaximumAlgorithmLength ||
            !slot.FingerprintVersion || *slot.FingerprintVersion == 0 ||
            slot.Fingerprint.empty() ||
            !IsValidPayload(slot.Fingerprint) ||
            !slot.SaveMetadataCodecVersion || *slot.SaveMetadataCodecVersion == 0 ||
            slot.SaveMetadata.empty() || !IsValidPayload(slot.SaveMetadata))
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "checkpoint slot save metadata is incomplete or outside persistence bounds");
        }

        Bytes digestPayload = acRequest.MutationPayload;
        AppendDigestText(digestPayload, slot.Slot.Value);
        AppendDigestText(digestPayload, slot.Player.Value);
        AppendDigestText(digestPayload, slot.CharacterBinding.Value);
        AppendDigestText(digestPayload, *slot.NativeSaveIdentity);
        AppendDigestText(digestPayload, *slot.FingerprintAlgorithm);
        AppendDigestScalar(digestPayload, *slot.FingerprintVersion);
        AppendDigestBlob(digestPayload, slot.Fingerprint);
        AppendDigestScalar(digestPayload, *slot.SaveMetadataCodecVersion);
        AppendDigestBlob(digestPayload, slot.SaveMetadata);
        AppendDigestOutbox(digestPayload, acRequest.Outbox);
        const std::string digest = Codec::MutationDigest(
            "RecordCheckpointSlotSave",
            acRequest.ExpectedRevision,
            acRequest.MutationCodecVersion,
            digestPayload,
            acRequest.Checkpoint.Value);
        if (digest.empty())
            return MutationFailure(StoreError::DatabaseFailure, "failed to compute checkpoint slot digest");

        Transaction transaction(m_pDatabase);
        if (!transaction.Active())
            return MutationFailure(transaction.Error().Error, transaction.Error().Message);
        ReplayResult replay = CheckReplay(
            m_pDatabase, acRequest.Campaign, acRequest.Mutation, digest);
        if (replay.Found || !replay.Result)
            return ReplayToMutationResult(replay);
        auto current = ReadCurrentRevision(m_pDatabase, acRequest.Campaign);
        if (!current)
            return MutationFailure(current.Error, current.Message);
        if (current.Value != acRequest.ExpectedRevision)
        {
            return MutationFailure(
                StoreError::StaleRevision,
                "checkpoint save metadata expected_revision=" +
                    std::to_string(acRequest.ExpectedRevision) +
                    " current_revision=" + std::to_string(current.Value));
        }
        if (current.Value >= kMaximumRevision)
            return MutationFailure(StoreError::InvalidArgument, "campaign revision exhausted");
        const StateVersion newRevision = current.Value + 1;

        Statement existing(
            m_pDatabase,
            "SELECT cp.checkpoint_state, cs.player_id, cs.character_binding_id, "
            "cs.native_save_identity, cs.fingerprint_algorithm, cs.fingerprint_version, "
            "cs.fingerprint, cs.save_metadata_codec_version, cs.save_metadata "
            "FROM campaign_checkpoints cp JOIN campaign_checkpoint_slots cs "
            "ON cs.checkpoint_id=cp.checkpoint_id AND cs.campaign_id=cp.campaign_id "
            "WHERE cp.campaign_id=?1 AND cp.checkpoint_id=?2 AND cs.slot_id=?3;");
        if (!existing.Valid() ||
            !existing.BindText(1, acRequest.Campaign.Value) ||
            !existing.BindText(2, acRequest.Checkpoint.Value) ||
            !existing.BindText(3, slot.Slot.Value))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "load checkpoint slot for save metadata"));
        }
        const int existingCode = existing.Step();
        if (existingCode == SQLITE_DONE)
        {
            return MutationFailure(
                StoreError::NotFound,
                "campaign/checkpoint/slot identity was not found");
        }
        if (existingCode != SQLITE_ROW)
            return MutationFailure(StoreError::DatabaseFailure, DatabaseMessage(m_pDatabase, "load checkpoint slot"));
        if (existing.Int(0) != static_cast<int>(CheckpointState::Candidate))
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "checkpoint=" + acRequest.Checkpoint.Value + " is not a Candidate");
        }
        if (existing.Text(1) != slot.Player.Value ||
            existing.Text(2) != slot.CharacterBinding.Value)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "checkpoint slot PlayerId or CharacterBinding mismatch");
        }
        if (!existing.IsNull(3))
        {
            const bool identical = existing.Text(3) == *slot.NativeSaveIdentity &&
                existing.Text(4) == *slot.FingerprintAlgorithm &&
                !existing.IsNull(5) &&
                static_cast<std::uint32_t>(existing.Int64(5)) ==
                    *slot.FingerprintVersion &&
                existing.Blob(6) == slot.Fingerprint &&
                !existing.IsNull(7) &&
                static_cast<std::uint32_t>(existing.Int64(7)) ==
                    *slot.SaveMetadataCodecVersion &&
                existing.Blob(8) == slot.SaveMetadata;
            if (!identical)
            {
                return MutationFailure(
                    StoreError::IdempotencyConflict,
                    "checkpoint slot save metadata was already recorded with different values");
            }
            MutationResult result;
            result.Revision = current.Value;
            result.IdempotentReplay = true;
            return result;
        }

        Statement update(
            m_pDatabase,
            "UPDATE campaign_checkpoint_slots SET "
            "native_save_identity=?1, fingerprint_algorithm=?2, fingerprint_version=?3, "
            "fingerprint=?4, save_metadata_codec_version=?5, save_metadata=?6 "
            "WHERE campaign_id=?7 AND checkpoint_id=?8 AND slot_id=?9;");
        if (!update.Valid() ||
            !update.BindText(1, *slot.NativeSaveIdentity) ||
            !update.BindText(2, *slot.FingerprintAlgorithm) ||
            !update.BindInt64(3, static_cast<std::int64_t>(*slot.FingerprintVersion)) ||
            !update.BindBlob(4, slot.Fingerprint) ||
            !update.BindInt64(5, static_cast<std::int64_t>(*slot.SaveMetadataCodecVersion)) ||
            !update.BindBlob(6, slot.SaveMetadata) ||
            !update.BindText(7, acRequest.Campaign.Value) ||
            !update.BindText(8, acRequest.Checkpoint.Value) ||
            !update.BindText(9, slot.Slot.Value))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "record checkpoint slot save metadata"));
        }
        StoreResult updated = StepDone(
            m_pDatabase, update, "record checkpoint slot save metadata");
        if (!updated)
            return MutationFailure(updated.Error, updated.Message);
        const std::int64_t now = NowUnixMs();
        StoreResult revision = UpdateCampaignRevision(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            newRevision,
            now);
        if (!revision)
            return MutationFailure(revision.Error, revision.Message);
        if (ShouldInject(TransactionStage::AfterCurrentState))
            return MutationFailure(StoreError::FaultInjected, "fault injected after checkpoint slot write");
        StoreResult journal = AppendJournal(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            acRequest.ExpectedRevision,
            newRevision,
            "RecordCheckpointSlotSave",
            digest,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            now);
        if (!journal)
            return MutationFailure(journal.Error, journal.Message);
        if (ShouldInject(TransactionStage::AfterJournal))
            return MutationFailure(StoreError::FaultInjected, "fault injected after checkpoint slot journal");
        StoreResult outbox = AppendOutbox(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            newRevision,
            acRequest.Outbox,
            now);
        if (!outbox)
            return MutationFailure(outbox.Error, outbox.Message);
        if (ShouldInject(TransactionStage::AfterOutbox) ||
            ShouldInject(TransactionStage::BeforeCommit))
        {
            return MutationFailure(StoreError::FaultInjected, "fault injected before checkpoint slot commit");
        }
        StoreResult committed = transaction.Commit();
        if (!committed)
            return MutationFailure(committed.Error, committed.Message);
        MutationResult result;
        result.Revision = newRevision;
        result.Applied = true;
        return result;
    }
    catch (...)
    {
        return MutationFailure(
            StoreError::DatabaseFailure,
            "checkpoint slot save metadata failed safely checkpoint=" +
                acRequest.Checkpoint.Value);
    }
}

MutationResult SqliteCampaignStore::CommitCheckpoint(
    const CommitCheckpointRequest& acRequest) noexcept
{
    try
    {
        StoreResult common = ValidateCommonMutation(
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            acRequest.Mutation,
            "CommitCheckpoint",
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            acRequest.Outbox);
        if (!common)
            return MutationFailure(common.Error, common.Message);
        if (!IsValidId(acRequest.Checkpoint))
            return MutationFailure(StoreError::InvalidArgument, "invalid CheckpointId");
        Bytes digestPayload = acRequest.MutationPayload;
        AppendDigestOutbox(digestPayload, acRequest.Outbox);
        const std::string digest = Codec::MutationDigest(
            "CommitCheckpoint",
            acRequest.ExpectedRevision,
            acRequest.MutationCodecVersion,
            digestPayload,
            acRequest.Checkpoint.Value);
        if (digest.empty())
            return MutationFailure(StoreError::DatabaseFailure, "failed to compute checkpoint commit digest");

        Transaction transaction(m_pDatabase);
        if (!transaction.Active())
            return MutationFailure(transaction.Error().Error, transaction.Error().Message);
        ReplayResult replay = CheckReplay(
            m_pDatabase, acRequest.Campaign, acRequest.Mutation, digest);
        if (replay.Found || !replay.Result)
            return ReplayToMutationResult(replay);

        auto checkpoint = LoadCheckpoint(acRequest.Campaign, acRequest.Checkpoint);
        if (!checkpoint)
            return MutationFailure(checkpoint.Error, checkpoint.Message);
        if (checkpoint.Value.State == CheckpointState::Committed)
        {
            auto campaign = LoadCampaign(acRequest.Campaign);
            if (campaign && campaign.Value.LastCommittedCheckpoint == acRequest.Checkpoint)
            {
                MutationResult result;
                result.Revision = checkpoint.Value.CommittedRevision.value_or(
                    campaign.Value.CurrentRevision);
                result.IdempotentReplay = true;
                return result;
            }
            return MutationFailure(
                StoreError::IdempotencyConflict,
                "checkpoint is committed but is not the campaign's selected committed checkpoint");
        }

        auto current = ReadCurrentRevision(m_pDatabase, acRequest.Campaign);
        if (!current)
            return MutationFailure(current.Error, current.Message);
        if (current.Value != acRequest.ExpectedRevision)
        {
            return MutationFailure(
                StoreError::StaleRevision,
                "checkpoint commit expected_revision=" +
                    std::to_string(acRequest.ExpectedRevision) +
                    " current_revision=" + std::to_string(current.Value));
        }
        if (current.Value >= kMaximumRevision)
            return MutationFailure(StoreError::InvalidArgument, "campaign revision exhausted");
        for (const CheckpointSlotRecord& slot : checkpoint.Value.Slots)
        {
            if (!slot.NativeSaveIdentity || slot.NativeSaveIdentity->empty() ||
                !slot.FingerprintAlgorithm || slot.FingerprintAlgorithm->empty() ||
                !slot.FingerprintVersion || slot.Fingerprint.empty() ||
                !slot.SaveMetadataCodecVersion || slot.SaveMetadata.empty())
            {
                return MutationFailure(
                    StoreError::InvalidArgument,
                    "checkpoint=" + acRequest.Checkpoint.Value +
                        " is incomplete for slot=" + slot.Slot.Value);
            }
        }

        const StateVersion newRevision = current.Value + 1;
        const std::int64_t now = NowUnixMs();
        Statement updateCheckpoint(
            m_pDatabase,
            "UPDATE campaign_checkpoints SET checkpoint_state=1, committed_revision=?1, "
            "committed_at_unix_ms=?2 WHERE campaign_id=?3 AND checkpoint_id=?4 "
            "AND checkpoint_state=0;");
        if (!updateCheckpoint.Valid() ||
            !updateCheckpoint.BindInt64(1, static_cast<std::int64_t>(newRevision)) ||
            !updateCheckpoint.BindInt64(2, now) ||
            !updateCheckpoint.BindText(3, acRequest.Campaign.Value) ||
            !updateCheckpoint.BindText(4, acRequest.Checkpoint.Value))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "mark checkpoint committed"));
        }
        StoreResult marked = StepDone(
            m_pDatabase, updateCheckpoint, "mark checkpoint committed");
        if (!marked || sqlite3_changes(m_pDatabase) != 1)
        {
            return MutationFailure(
                marked ? StoreError::IdempotencyConflict : marked.Error,
                marked ? "checkpoint candidate changed before commit" : marked.Message);
        }
        Statement updateCampaign(
            m_pDatabase,
            "UPDATE campaigns SET last_committed_checkpoint_id=?1, current_revision=?2, "
            "updated_at_unix_ms=?3 WHERE campaign_id=?4 AND current_revision=?5;");
        if (!updateCampaign.Valid() ||
            !updateCampaign.BindText(1, acRequest.Checkpoint.Value) ||
            !updateCampaign.BindInt64(2, static_cast<std::int64_t>(newRevision)) ||
            !updateCampaign.BindInt64(3, now) ||
            !updateCampaign.BindText(4, acRequest.Campaign.Value) ||
            !updateCampaign.BindInt64(5, static_cast<std::int64_t>(acRequest.ExpectedRevision)))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "select committed checkpoint"));
        }
        StoreResult selected = StepDone(
            m_pDatabase, updateCampaign, "select committed checkpoint");
        if (!selected || sqlite3_changes(m_pDatabase) != 1)
        {
            return MutationFailure(
                selected ? StoreError::StaleRevision : selected.Error,
                selected ? "campaign revision changed before checkpoint commit" : selected.Message);
        }
        if (ShouldInject(TransactionStage::AfterCurrentState))
            return MutationFailure(StoreError::FaultInjected, "fault injected after checkpoint commit state");
        StoreResult journal = AppendJournal(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            acRequest.ExpectedRevision,
            newRevision,
            "CommitCheckpoint",
            digest,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            now);
        if (!journal)
            return MutationFailure(journal.Error, journal.Message);
        if (ShouldInject(TransactionStage::AfterJournal))
            return MutationFailure(StoreError::FaultInjected, "fault injected after checkpoint commit journal");
        StoreResult outbox = AppendOutbox(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            newRevision,
            acRequest.Outbox,
            now);
        if (!outbox)
            return MutationFailure(outbox.Error, outbox.Message);
        if (ShouldInject(TransactionStage::AfterOutbox) ||
            ShouldInject(TransactionStage::BeforeCommit))
        {
            return MutationFailure(StoreError::FaultInjected, "fault injected before checkpoint commit boundary");
        }
        StoreResult committed = transaction.Commit();
        if (!committed)
            return MutationFailure(committed.Error, committed.Message);
        MutationResult result;
        result.Revision = newRevision;
        result.Applied = true;
        return result;
    }
    catch (...)
    {
        return MutationFailure(
            StoreError::DatabaseFailure,
            "checkpoint commit failed safely checkpoint=" + acRequest.Checkpoint.Value);
    }
}

MutationResult SqliteCampaignStore::RestoreCheckpointSnapshot(
    const RestoreCheckpointRequest& acRequest) noexcept
{
    try
    {
        const std::vector<OutboxIntent> noCallerOutbox;
        StoreResult common = ValidateCommonMutation(
            acRequest.Campaign,
            acRequest.ExpectedRevision,
            acRequest.Mutation,
            "RestoreCheckpoint",
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            noCallerOutbox);
        if (!common)
            return MutationFailure(common.Error, common.Message);
        if (!IsValidId(acRequest.Checkpoint))
            return MutationFailure(StoreError::InvalidArgument, "invalid CheckpointId");
        const std::string digest = Codec::MutationDigest(
            "RestoreCheckpoint",
            acRequest.ExpectedRevision,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            acRequest.Checkpoint.Value);
        if (digest.empty())
            return MutationFailure(StoreError::DatabaseFailure, "failed to compute checkpoint restore digest");

        Transaction transaction(m_pDatabase);
        if (!transaction.Active())
            return MutationFailure(transaction.Error().Error, transaction.Error().Message);
        ReplayResult replay = CheckReplay(
            m_pDatabase, acRequest.Campaign, acRequest.Mutation, digest);
        if (replay.Found || !replay.Result)
            return ReplayToMutationResult(replay);
        auto currentProjection = LoadCampaignProjection(
            acRequest.Campaign, ProjectionAudience::Server());
        if (!currentProjection)
            return MutationFailure(currentProjection.Error, currentProjection.Message);
        if (currentProjection.Value.Campaign.CurrentRevision !=
            acRequest.ExpectedRevision)
        {
            return MutationFailure(
                StoreError::StaleRevision,
                "checkpoint restore expected_revision=" +
                    std::to_string(acRequest.ExpectedRevision) +
                    " current_revision=" +
                    std::to_string(currentProjection.Value.Campaign.CurrentRevision));
        }
        if (acRequest.ExpectedRevision >= kMaximumRevision)
            return MutationFailure(StoreError::InvalidArgument, "campaign revision exhausted");

        auto checkpoint = LoadCheckpoint(acRequest.Campaign, acRequest.Checkpoint);
        if (!checkpoint)
            return MutationFailure(checkpoint.Error, checkpoint.Message);
        if (checkpoint.Value.State != CheckpointState::Committed)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "checkpoint=" + acRequest.Checkpoint.Value + " is not committed");
        }

        Bytes snapshotPayload;
        {
            Statement snapshot(
                m_pDatabase,
                "SELECT snapshot_payload, checksum, source_revision, snapshot_codec_version "
                "FROM campaign_snapshots WHERE campaign_id=?1 AND snapshot_id=?2;");
            if (!snapshot.Valid() ||
                !snapshot.BindText(1, acRequest.Campaign.Value) ||
                !snapshot.BindText(2, checkpoint.Value.Snapshot.Value) ||
                snapshot.Step() != SQLITE_ROW)
            {
                return MutationFailure(
                    StoreError::IntegrityFailure,
                    "checkpoint snapshot is missing campaign=" + acRequest.Campaign.Value +
                        " checkpoint=" + acRequest.Checkpoint.Value);
            }
            snapshotPayload = snapshot.Blob(0);
            if (snapshot.Text(1) != Codec::Checksum(snapshotPayload) ||
                snapshot.Int64(2) < 0 ||
                static_cast<StateVersion>(snapshot.Int64(2)) !=
                    checkpoint.Value.SourceRevision ||
                static_cast<std::uint32_t>(snapshot.Int64(3)) !=
                    kCampaignSnapshotCodecVersion)
            {
                return MutationFailure(
                    StoreError::IntegrityFailure,
                    "checkpoint snapshot checksum, revision, or codec mismatch");
            }
        }
        auto restoredProjection = Codec::DecodeSnapshot(snapshotPayload);
        if (!restoredProjection ||
            restoredProjection.Value.Campaign.Id != acRequest.Campaign ||
            restoredProjection.Value.Campaign.CurrentRevision !=
                checkpoint.Value.SourceRevision ||
            !restoredProjection.Value.Campaign.RosterSealed)
        {
            return MutationFailure(
                StoreError::IntegrityFailure,
                "checkpoint snapshot cannot materialize the exact sealed campaign state");
        }
        if (restoredProjection.Value.Slots != currentProjection.Value.Slots ||
            checkpoint.Value.Slots.size() != currentProjection.Value.Slots.size())
        {
            return MutationFailure(
                StoreError::IntegrityFailure,
                "checkpoint roster differs from current immutable campaign roster");
        }
        for (std::size_t index = 0; index < checkpoint.Value.Slots.size(); ++index)
        {
            const CheckpointSlotRecord& checkpointSlot = checkpoint.Value.Slots[index];
            const CampaignSlotRecord& currentSlot = currentProjection.Value.Slots[index];
            if (checkpointSlot.Slot != currentSlot.Slot ||
                checkpointSlot.Player != currentSlot.Player ||
                checkpointSlot.CharacterBinding != currentSlot.CharacterBinding)
            {
                return MutationFailure(
                    StoreError::IntegrityFailure,
                    "checkpoint slot/player/binding identity mismatch during restore");
            }
        }

        const StateVersion newRevision = acRequest.ExpectedRevision + 1;
        const std::int64_t now = NowUnixMs();
        Statement updateCore(
            m_pDatabase,
            "UPDATE campaigns SET core_state_codec_version=?1, core_state_payload=?2, "
            "current_revision=?3, updated_at_unix_ms=?4 "
            "WHERE campaign_id=?5 AND current_revision=?6;");
        if (!updateCore.Valid() ||
            !updateCore.BindInt(1, static_cast<int>(
                restoredProjection.Value.Campaign.CoreStateCodecVersion)) ||
            !updateCore.BindBlob(2, restoredProjection.Value.Campaign.CoreStatePayload) ||
            !updateCore.BindInt64(3, static_cast<std::int64_t>(newRevision)) ||
            !updateCore.BindInt64(4, now) ||
            !updateCore.BindText(5, acRequest.Campaign.Value) ||
            !updateCore.BindInt64(6, static_cast<std::int64_t>(acRequest.ExpectedRevision)))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "restore checkpoint core state"));
        }
        StoreResult coreUpdated = StepDone(
            m_pDatabase, updateCore, "restore checkpoint core state");
        if (!coreUpdated || sqlite3_changes(m_pDatabase) != 1)
        {
            return MutationFailure(
                coreUpdated ? StoreError::StaleRevision : coreUpdated.Error,
                coreUpdated ? "campaign changed before checkpoint restore" : coreUpdated.Message);
        }

        for (const char* pSql : {
                 "DELETE FROM character_build_state WHERE campaign_id=?1;",
                 "DELETE FROM adapter_state WHERE campaign_id=?1;"})
        {
            Statement clear(m_pDatabase, pSql);
            if (!clear.Valid() || !clear.BindText(1, acRequest.Campaign.Value))
            {
                return MutationFailure(
                    StoreError::DatabaseFailure,
                    DatabaseMessage(m_pDatabase, "clear current state for checkpoint restore"));
            }
            StoreResult cleared = StepDone(
                m_pDatabase, clear, "clear current state for checkpoint restore");
            if (!cleared)
                return MutationFailure(cleared.Error, cleared.Message);
        }
        for (CharacterBuildState& build : restoredProjection.Value.CharacterBuilds)
        {
            build.UpdatedRevision = newRevision;
            StoreResult upserted = UpsertCharacterBuild(
                m_pDatabase, acRequest.Campaign, build, newRevision);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }
        for (AdapterState& state : restoredProjection.Value.AdapterStates)
        {
            state.UpdatedRevision = newRevision;
            StoreResult upserted = UpsertAdapterState(
                m_pDatabase, acRequest.Campaign, state, newRevision);
            if (!upserted)
                return MutationFailure(upserted.Error, upserted.Message);
        }

        Statement supersede(
            m_pDatabase,
            "UPDATE campaign_outbox SET delivery_state=2, superseded_by_revision=?1 "
            "WHERE campaign_id=?2 AND delivery_state=0 AND revision<?1;");
        if (!supersede.Valid() ||
            !supersede.BindInt64(1, static_cast<std::int64_t>(newRevision)) ||
            !supersede.BindText(2, acRequest.Campaign.Value))
        {
            return MutationFailure(
                StoreError::DatabaseFailure,
                DatabaseMessage(m_pDatabase, "supersede obsolete outbox work"));
        }
        StoreResult superseded = StepDone(
            m_pDatabase, supersede, "supersede obsolete outbox work");
        if (!superseded)
            return MutationFailure(superseded.Error, superseded.Message);

        if (ShouldInject(TransactionStage::AfterCurrentState))
            return MutationFailure(StoreError::FaultInjected, "fault injected after checkpoint restoration");
        StoreResult journal = AppendJournal(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            acRequest.ExpectedRevision,
            newRevision,
            "RestoreCheckpoint",
            digest,
            acRequest.MutationCodecVersion,
            acRequest.MutationPayload,
            now,
            acRequest.Checkpoint,
            checkpoint.Value.SourceRevision);
        if (!journal)
            return MutationFailure(journal.Error, journal.Message);
        if (ShouldInject(TransactionStage::AfterJournal))
            return MutationFailure(StoreError::FaultInjected, "fault injected after restore journal");

        restoredProjection.Value.Campaign.CurrentRevision = newRevision;
        restoredProjection.Value.Campaign.LastCommittedCheckpoint =
            currentProjection.Value.Campaign.LastCommittedCheckpoint;
        restoredProjection.Value.Campaign.UpdatedAtUnixMs = now;
        Bytes canonicalSnapshot;
        StoreResult canonicalEncoded = Codec::EncodeSnapshot(
            restoredProjection.Value, canonicalSnapshot);
        if (!canonicalEncoded)
            return MutationFailure(canonicalEncoded.Error, canonicalEncoded.Message);
        const std::vector<OutboxIntent> restoreOutbox{
            OutboxIntent{kCampaignSnapshotCodecVersion, std::move(canonicalSnapshot)}};
        StoreResult outbox = AppendOutbox(
            m_pDatabase,
            acRequest.Campaign,
            acRequest.Mutation,
            newRevision,
            restoreOutbox,
            now);
        if (!outbox)
            return MutationFailure(outbox.Error, outbox.Message);
        if (ShouldInject(TransactionStage::AfterOutbox) ||
            ShouldInject(TransactionStage::BeforeCommit))
        {
            return MutationFailure(StoreError::FaultInjected, "fault injected before checkpoint restore commit");
        }
        StoreResult committed = transaction.Commit();
        if (!committed)
            return MutationFailure(committed.Error, committed.Message);

        MutationResult result;
        result.Revision = newRevision;
        result.Applied = true;
        return result;
    }
    catch (...)
    {
        return MutationFailure(
            StoreError::DatabaseFailure,
            "checkpoint restore failed safely campaign=" + acRequest.Campaign.Value +
                " checkpoint=" + acRequest.Checkpoint.Value);
    }
}

StoreValueResult<std::vector<JournalRecord>> SqliteCampaignStore::LoadJournal(
    const CampaignId& acCampaign) noexcept
{
    StoreValueResult<std::vector<JournalRecord>> result;
    try
    {
        if (!IsValidId(acCampaign))
        {
            result.Error = StoreError::InvalidArgument;
            result.Message = "invalid CampaignId";
            return result;
        }
        Statement statement(
            m_pDatabase,
            "SELECT sequence, mutation_id, resulting_revision, mutation_kind, "
            "payload_codec_version, payload, restored_from_checkpoint_id, "
            "restored_from_revision, created_at_unix_ms "
            "FROM campaign_journal WHERE campaign_id=?1 ORDER BY resulting_revision;");
        if (!statement.Valid() || !statement.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load campaign journal");
            return result;
        }
        int journalCode{};
        for (journalCode = statement.Step(); journalCode == SQLITE_ROW;
             journalCode = statement.Step())
        {
            if (statement.Int64(0) <= 0 || statement.Int64(2) <= 0 ||
                statement.Int64(4) <= 0)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign journal contains malformed metadata";
                return result;
            }
            JournalRecord record;
            record.Sequence = static_cast<std::uint64_t>(statement.Int64(0));
            record.Campaign = acCampaign;
            record.Mutation = MutationId{statement.Text(1)};
            record.ResultingRevision =
                static_cast<StateVersion>(statement.Int64(2));
            record.Kind = statement.Text(3);
            record.PayloadCodecVersion =
                static_cast<std::uint32_t>(statement.Int64(4));
            record.Payload = statement.Blob(5);
            if (!statement.IsNull(6))
                record.RestoredFromCheckpoint = CheckpointId{statement.Text(6)};
            if (!statement.IsNull(7))
                record.RestoredFromRevision =
                    static_cast<StateVersion>(statement.Int64(7));
            record.CreatedAtUnixMs = statement.Int64(8);
            if (!IsValidId(record.Mutation) || record.Kind.empty() ||
                record.Kind.size() > kMaximumKindLength ||
                !IsValidPayload(record.Payload))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign journal contains malformed payload metadata";
                return result;
            }
            result.Value.push_back(std::move(record));
        }
        if (journalCode != SQLITE_DONE)
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "iterate campaign journal");
            return result;
        }
    }
    catch (...)
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = "failed to load campaign journal safely";
    }
    return result;
}

StoreValueResult<std::vector<OutboxRecord>>
SqliteCampaignStore::LoadPendingOutbox(
    const CampaignId& acCampaign) noexcept
{
    StoreValueResult<std::vector<OutboxRecord>> result;
    try
    {
        if (!IsValidId(acCampaign))
        {
            result.Error = StoreError::InvalidArgument;
            result.Message = "invalid CampaignId";
            return result;
        }
        Statement statement(
            m_pDatabase,
            "SELECT outbox_id, mutation_id, intent_index, revision, "
            "payload_codec_version, payload, delivery_state, superseded_by_revision "
            "FROM campaign_outbox WHERE campaign_id=?1 AND delivery_state=0 "
            "ORDER BY revision, outbox_id;");
        if (!statement.Valid() || !statement.BindText(1, acCampaign.Value))
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "load pending campaign outbox");
            return result;
        }
        int outboxCode{};
        for (outboxCode = statement.Step(); outboxCode == SQLITE_ROW;
             outboxCode = statement.Step())
        {
            if (statement.Int64(0) <= 0 || statement.Int64(2) < 0 ||
                statement.Int64(3) <= 0 || statement.Int64(4) <= 0 ||
                statement.Int(6) != static_cast<int>(OutboxState::Pending) ||
                !statement.IsNull(7))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign outbox contains malformed pending metadata";
                return result;
            }
            OutboxRecord record;
            record.Id = static_cast<std::uint64_t>(statement.Int64(0));
            record.Campaign = acCampaign;
            record.Mutation = MutationId{statement.Text(1)};
            record.IntentIndex = static_cast<std::uint32_t>(statement.Int64(2));
            record.Revision = static_cast<StateVersion>(statement.Int64(3));
            record.CodecVersion = static_cast<std::uint32_t>(statement.Int64(4));
            record.Payload = statement.Blob(5);
            if (!IsValidId(record.Mutation) || !IsValidPayload(record.Payload))
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign outbox contains malformed payload";
                return result;
            }
            result.Value.push_back(std::move(record));
        }
        if (outboxCode != SQLITE_DONE)
        {
            result.Error = StoreError::DatabaseFailure;
            result.Message = DatabaseMessage(m_pDatabase, "iterate pending campaign outbox");
            return result;
        }
    }
    catch (...)
    {
        result.Error = StoreError::DatabaseFailure;
        result.Message = "failed to load campaign outbox safely";
    }
    return result;
}

StoreResult SqliteCampaignStore::MarkOutboxDelivered(
    std::uint64_t aOutboxId) noexcept
{
    try
    {
        if (aOutboxId == 0 || aOutboxId > static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max()))
        {
            return Failure(StoreError::InvalidArgument, "invalid outbox identity");
        }
        Transaction transaction(m_pDatabase);
        if (!transaction.Active())
            return transaction.Error();
        Statement statement(
            m_pDatabase,
            "UPDATE campaign_outbox SET delivery_state=1, delivered_at_unix_ms=?1 "
            "WHERE outbox_id=?2 AND delivery_state=0;");
        if (!statement.Valid() ||
            !statement.BindInt64(1, NowUnixMs()) ||
            !statement.BindInt64(2, static_cast<std::int64_t>(aOutboxId)))
        {
            return BindFailure(m_pDatabase, "mark campaign outbox delivered");
        }
        StoreResult updated = StepDone(
            m_pDatabase, statement, "mark campaign outbox delivered");
        if (!updated)
            return updated;
        if (sqlite3_changes(m_pDatabase) == 0)
        {
            Statement existing(
                m_pDatabase,
                "SELECT delivery_state FROM campaign_outbox WHERE outbox_id=?1;");
            if (!existing.Valid() ||
                !existing.BindInt64(1, static_cast<std::int64_t>(aOutboxId)))
            {
                return BindFailure(m_pDatabase, "inspect campaign outbox delivery state");
            }
            const int code = existing.Step();
            if (code == SQLITE_DONE)
                return Failure(StoreError::NotFound, "outbox identity was not found");
            if (code != SQLITE_ROW)
                return BindFailure(m_pDatabase, "inspect campaign outbox delivery state");
            if (existing.Int(0) == static_cast<int>(OutboxState::Superseded))
            {
                return Failure(
                    StoreError::IdempotencyConflict,
                    "superseded outbox work cannot be delivered");
            }
        }
        return transaction.Commit();
    }
    catch (...)
    {
        return Failure(StoreError::DatabaseFailure, "mark outbox delivered failed safely");
    }
}
}
