#include <sqlite/SqliteCampaignStore.h>

#include <codec/CampaignCodec.h>
#include <sqlite/SqliteCampaignState.h>
#include <sqlite/SqliteJournalOutbox.h>
#include <sqlite/SqlitePrimitives.h>
#include <sqlite/SqliteValidation.h>

#include <sqlite3.h>

#include <limits>
#include <utility>

namespace STRE::Campaign
{
using namespace Sqlite;

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
        if (acRequest.SnapshotCoreStateCodecVersion == 0 ||
            acRequest.SnapshotCoreStatePayload.empty() ||
            !IsValidPayload(acRequest.SnapshotCoreStatePayload))
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "checkpoint canonical core state is missing");
        }
        Bytes digestPayload = acRequest.MutationPayload;
        AppendDigestText(digestPayload, acRequest.Snapshot.Value);
        AppendDigestScalar(
            digestPayload, acRequest.SnapshotCoreStateCodecVersion);
        AppendDigestBlob(
            digestPayload, acRequest.SnapshotCoreStatePayload);
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
        if (projection.Value.Campaign.CoreStateCodecVersion !=
            acRequest.SnapshotCoreStateCodecVersion)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "checkpoint canonical core-state codec differs from current campaign state");
        }
        if (acRequest.ExpectedRevision >= kMaximumRevision)
            return MutationFailure(StoreError::InvalidArgument, "campaign revision exhausted");
        const StateVersion newRevision = acRequest.ExpectedRevision + 1;

        projection.Value.Campaign.CoreStatePayload =
            acRequest.SnapshotCoreStatePayload;
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
        result.Value.SnapshotCoreStateCodecVersion =
            decoded.Value.Campaign.CoreStateCodecVersion;
        result.Value.SnapshotCoreStatePayload =
            std::move(decoded.Value.Campaign.CoreStatePayload);
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
        if (acRequest.RestoredCoreStateCodecVersion == 0 ||
            acRequest.RestoredCoreStatePayload.empty() ||
            !IsValidPayload(acRequest.RestoredCoreStatePayload))
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "restored canonical core state is missing");
        }
        Bytes digestPayload = acRequest.MutationPayload;
        AppendDigestScalar(
            digestPayload, acRequest.RestoredCoreStateCodecVersion);
        AppendDigestBlob(
            digestPayload, acRequest.RestoredCoreStatePayload);
        const std::string digest = Codec::MutationDigest(
            "RestoreCheckpoint",
            acRequest.ExpectedRevision,
            acRequest.MutationCodecVersion,
            digestPayload,
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
        if (restoredProjection.Value.Campaign.CoreStateCodecVersion !=
            acRequest.RestoredCoreStateCodecVersion)
        {
            return MutationFailure(
                StoreError::InvalidArgument,
                "restored canonical core-state codec differs from checkpoint state");
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
            !updateCore.BindBlob(2, acRequest.RestoredCoreStatePayload) ||
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
        restoredProjection.Value.Campaign.CoreStatePayload =
            acRequest.RestoredCoreStatePayload;
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

}
