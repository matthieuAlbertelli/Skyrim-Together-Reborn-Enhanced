#include <sqlite/SqliteJournalOutbox.h>

#include <sqlite/SqliteCampaignStore.h>
#include <sqlite/SqlitePrimitives.h>
#include <sqlite/SqliteValidation.h>

#include <sqlite3.h>

namespace STRE::Campaign::Sqlite
{
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
    const std::optional<CheckpointId>& acRestoredCheckpoint,
    const std::optional<StateVersion>& acRestoredRevision)
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
}

namespace STRE::Campaign
{
using namespace Sqlite;

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
            "SELECT sequence, mutation_id, expected_revision, resulting_revision, "
            "mutation_kind, payload_codec_version, payload, "
            "restored_from_checkpoint_id, restored_from_revision, created_at_unix_ms "
            "FROM campaign_journal WHERE campaign_id=?1 ORDER BY sequence;");
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
            if (statement.Int64(0) <= 0 || statement.Int64(2) < 0 ||
                statement.Int64(3) <= 0 || statement.Int64(5) <= 0)
            {
                result.Error = StoreError::IntegrityFailure;
                result.Message = "campaign journal contains malformed metadata";
                return result;
            }
            JournalRecord record;
            record.Sequence = static_cast<std::uint64_t>(statement.Int64(0));
            record.Campaign = acCampaign;
            record.Mutation = MutationId{statement.Text(1)};
            record.ExpectedRevision =
                static_cast<StateVersion>(statement.Int64(2));
            record.ResultingRevision =
                static_cast<StateVersion>(statement.Int64(3));
            record.Kind = statement.Text(4);
            record.PayloadCodecVersion =
                static_cast<std::uint32_t>(statement.Int64(5));
            record.Payload = statement.Blob(6);
            if (!statement.IsNull(7))
                record.RestoredFromCheckpoint = CheckpointId{statement.Text(7)};
            if (!statement.IsNull(8))
                record.RestoredFromRevision =
                    static_cast<StateVersion>(statement.Int64(8));
            record.CreatedAtUnixMs = statement.Int64(9);
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
